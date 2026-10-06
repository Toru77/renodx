#pragma once

// Sora 2nd geometry contract, recognized from shader bytecode.
//
// Every geometry vertex shader in the dumps (43 decompiled, 103 seen in one
// live frame) reads its object transform the same way:
//
//   StructuredBuffer<InstanceParam> instances_g : register(t15);  // stride 160
//   cbuffer cb_instance : register(b1) { int instanceOffset_g; int maxBoneCount_g; }
//   element = SV_InstanceID + instanceOffset_g
//   InstanceParam.world     float4x3 (column-major) at byte 0
//   InstanceParam.prevWorld float4x3 at byte 48
//   world.x = dot(float4(p, 1), floats[0..3]); rows 1..2 likewise (row-dot)
//
// Instead of a per-hash research verdict, each shader is classified once at
// creation from its own reflection data (resource bindings, cbuffer variable
// usage, struct layout, input signature). Hashes never decide admission; they
// are only carried for display. The classification answers one question:
// can this vertex shader's draws be reproduced as "static mesh x instance
// world matrix"?

#include <cstdint>
#include <string_view>

#include "dxbc_reflect.hpp"

namespace falcom_world::contract {

inline constexpr uint32_t kInstanceSlot = 15u;
inline constexpr uint32_t kInstanceStride = 160u;
inline constexpr uint32_t kInstanceCbSlot = 1u;
inline constexpr uint32_t kWorldOffset = 0u;
inline constexpr uint32_t kPrevWorldOffset = 48u;
inline constexpr uint32_t kWorldFloats = 12u;
inline constexpr uint32_t kSceneCbSlot = 0u;
inline constexpr uint32_t kBonesSlot = 0u;
inline constexpr uint32_t kBonesStride = 48u;
inline constexpr std::string_view kSceneCbName = "cb_scene";

enum class VsClass : uint8_t {
  Unclassified = 0,  // pipeline created before the classifier was listening
  Rigid,             // static mesh x instance world: admitted to the BVH
  Skinned,           // bones_g / BLENDINDICES
  Wind,              // world-space wind sway (foliage)
  Billboard,         // camera-facing (reads view/viewInv)
  Animated,          // other time-driven vertex motion (water waves)
  OtherView,         // instanced, but projects with a non-scene camera (minimap)
  ExtraInputs,       // reads more buffers than the contract (index indirection, particles)
  BadLayout,         // instances_g present but world is not float4x3 at byte 0
  NotInstanced,      // no t15 stride-160 instances + b1 offset (fullscreen, UI, VFX)
  NoPosition,
  ParseFailed,
  Count,
};

enum class PsClass : uint8_t {
  Unclassified = 0,
  Opaque,       // writes surfaces as-is (dithered near-fade discard is still opaque)
  AlphaTested,  // cuts holes with alphaTestThreshold_g (leaves, fences): excluded until alpha is traced
  ParseFailed,
  Count,
};

inline const char* VsClassName(VsClass value) {
  switch (value) {
    case VsClass::Rigid:        return "rigid";
    case VsClass::Skinned:      return "skinned";
    case VsClass::Wind:         return "wind";
    case VsClass::Billboard:    return "billboard";
    case VsClass::Animated:     return "animated";
    case VsClass::OtherView:    return "other_view";
    case VsClass::ExtraInputs:  return "extra_inputs";
    case VsClass::BadLayout:    return "bad_layout";
    case VsClass::NotInstanced: return "not_instanced";
    case VsClass::NoPosition:   return "no_position";
    case VsClass::ParseFailed:  return "parse_failed";
    default:                    return "unclassified";
  }
}

inline const char* PsClassName(PsClass value) {
  switch (value) {
    case PsClass::Opaque:      return "opaque";
    case PsClass::AlphaTested: return "alpha_tested";
    case PsClass::ParseFailed: return "parse_failed";
    default:                   return "unclassified";
  }
}

// The instance element must declare `world` as a float4x3 stored column-major
// at byte 0: that storage is what makes floats[0..3] the row-dot weights for
// world.x. Any other layout means the pool would read the wrong numbers.
inline bool HasContractWorldLayout(const dxbc::Reflection& reflection, const dxbc::ResourceBinding& instances) {
  const dxbc::ConstantBuffer* layout = reflection.FindConstantBuffer(instances.name, dxbc::kCBufferResourceBindInfo);
  if (layout == nullptr || layout->size != kInstanceStride || layout->variables.empty()) return false;
  for (const auto& member : layout->variables.front().members) {
    if (member.name != "world") continue;
    return member.offset == kWorldOffset
           && member.var_class == dxbc::kClassMatrixColumns
           && member.var_type == dxbc::kTypeFloat
           && member.rows == 4u
           && member.columns == 3u;
  }
  return false;
}

inline VsClass ClassifyVertexShader(const dxbc::Reflection& reflection) {
  if (!reflection.valid) return VsClass::ParseFailed;

  const dxbc::ResourceBinding* instances = reflection.FindResource(dxbc::kInputStructured, kInstanceSlot);
  if (instances == nullptr || instances->stride != kInstanceStride) return VsClass::NotInstanced;

  // cb_instance at b1 must feed the element index from its first int.
  const dxbc::ResourceBinding* instance_cb = reflection.FindResource(dxbc::kInputCBuffer, kInstanceCbSlot);
  if (instance_cb == nullptr) return VsClass::NotInstanced;
  const dxbc::ConstantBuffer* instance_cb_layout = reflection.FindConstantBuffer(instance_cb->name, 0u);
  bool reads_offset = false;
  if (instance_cb_layout != nullptr) {
    for (const auto& variable : instance_cb_layout->variables) {
      if (variable.offset == 0u && variable.used) reads_offset = true;
    }
  }
  if (!reads_offset) return VsClass::NotInstanced;

  if (!HasContractWorldLayout(reflection, *instances)) return VsClass::BadLayout;
  if (!reflection.HasInput("POSITION", 0u)) return VsClass::NoPosition;

  // Any buffer or texture input besides the instances (and the bones of the
  // skinned path) means the transform or the instance index depends on data
  // the contract does not model, e.g. instanceIndices_g indirection.
  bool has_bones = false;
  for (const auto& resource : reflection.resources) {
    const bool buffer_or_texture = resource.type == dxbc::kInputTBuffer
                                   || resource.type == dxbc::kInputTexture
                                   || resource.type == dxbc::kInputStructured
                                   || resource.type == dxbc::kInputByteAddress;
    if (!buffer_or_texture || &resource == instances) continue;
    if (resource.type == dxbc::kInputStructured && resource.bind_point == kBonesSlot && resource.stride == kBonesStride) {
      has_bones = true;
      continue;
    }
    return VsClass::ExtraInputs;
  }
  if (has_bones || reflection.HasInput("BLENDINDICES")) return VsClass::Skinned;

  const dxbc::ResourceBinding* camera_cb = reflection.FindResource(dxbc::kInputCBuffer, kSceneCbSlot);
  if (camera_cb == nullptr || camera_cb->name != kSceneCbName) return VsClass::OtherView;

  if (reflection.UsesVariable("windWaveTime_g") || reflection.UsesVariable("windForce_g")
      || reflection.UsesVariable("windWaveFrequency_g") || reflection.UsesVariable("windDirection_g")) {
    return VsClass::Wind;
  }
  if (reflection.UsesVariable("viewInv_g") || reflection.UsesVariable("view_g")) return VsClass::Billboard;
  if (reflection.UsesVariable("gameTime_g") || reflection.UsesVariable("sceneTime_g")) return VsClass::Animated;
  return VsClass::Rigid;
}

inline PsClass ClassifyPixelShader(const dxbc::Reflection& reflection) {
  if (!reflection.valid) return PsClass::ParseFailed;
  // Discard alone is not enough: opaque map objects discard for the dithered
  // near-camera fade (ditherMtx_g / disableMapObjNearFade_g). Only the
  // material alpha-test threshold marks cut-out geometry.
  if (reflection.UsesVariable("alphaTestThreshold_g")) return PsClass::AlphaTested;
  return PsClass::Opaque;
}

}  // namespace falcom_world::contract
