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
//
// Camera visibility. The same element also carries what decides whether the
// game camera shows an instance. Every decompiled map-object pixel shader that
// reads disableMapObjNearFade_g (e.g. 0x2F107485, 0x5F527E52, 0x533C1853)
// discards with
//
//   fade  = max(disableMapObjNearFade_g, min(1, (|camera - p| - param.x) * param.y))
//           * mapColor_g.w
//   alpha = opacity_g * color.w
//   discard if fade * alpha < dither(4x4)   (pattern flipped when param.z > 0)
//
// with InstanceParam.color float4 at byte 96 and InstanceParam.param float4 at
// byte 128. Shadow-pass vertex shaders (0xE35C18B9) pass only (param.z,
// color.w) on, so shadow casters skip the near fade: an object the camera is
// too close to is hidden in the main view yet still drawn into shadow maps.
//
// View. Every rigid vertex shader in the dumps projects either with the scene
// camera (cb_scene.viewProj_g: G-buffer and other camera passes) or with the
// light (cb_shadow.shadowViewProj_g: shadow maps, e.g. 0xE35C18B9,
// 0xC3B9E234, 0x3168EA98). Shadow passes also have a render target bound, so
// the vertex shader's view is what tells the passes apart. Geometry the game
// only draws through light views (shadow-only casters) is never seen by the
// camera.

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
inline constexpr uint32_t kColorOffset = 96u;    // InstanceParam.color: w = opacity
inline constexpr uint32_t kParamOffset = 128u;   // InstanceParam.param: x near-fade start, y near-fade 1/range, z dither flip
inline constexpr uint32_t kSceneNearFadeFloorOffset = 412u;  // cb_scene.disableMapObjNearFade_g (c25.w)
inline constexpr uint32_t kSceneMapColorOffset = 752u;       // cb_scene.mapColor_g (c47), w = map alpha

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

// Classes whose vertex shader moves vertices with more than the instance
// world matrix (bones, wind, camera facing, waves). Their world-space
// triangles can only be taken from the shader's own outputs (stream out).
inline bool IsDeformingClass(VsClass value) {
  return value == VsClass::Skinned || value == VsClass::Wind || value == VsClass::Billboard
         || value == VsClass::Animated;
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

inline bool IsContractFloat4(const dxbc::StructMember& member, uint32_t offset) {
  return member.offset == offset && member.var_class == dxbc::kClassVector && member.var_type == dxbc::kTypeFloat
         && member.rows == 1u && member.columns == 4u;
}

// The instance element declares `color` and `param` as float4 at the contract
// offsets: the camera-visibility inputs above can be read from its bytes.
inline bool HasContractVisibilityLayout(const dxbc::Reflection& reflection, const dxbc::ResourceBinding& instances) {
  if (instances.stride != kInstanceStride) return false;
  const dxbc::ConstantBuffer* layout = reflection.FindConstantBuffer(instances.name, dxbc::kCBufferResourceBindInfo);
  if (layout == nullptr || layout->size != kInstanceStride || layout->variables.empty()) return false;
  bool color = false;
  bool param = false;
  for (const auto& member : layout->variables.front().members) {
    if (member.name == "color") color = IsContractFloat4(member, kColorOffset);
    if (member.name == "param") param = IsContractFloat4(member, kParamOffset);
  }
  return color && param;
}

// A pixel shader applies the map-object near fade above when it reads
// disableMapObjNearFade_g from cb_scene (b0) at the contract offset and reads
// the instance element (t15) with the contract visibility layout.
inline bool UsesContractNearFade(const dxbc::Reflection& reflection) {
  if (!reflection.valid) return false;
  const dxbc::ResourceBinding* scene = reflection.FindResource(dxbc::kInputCBuffer, kSceneCbSlot);
  if (scene == nullptr || scene->name != kSceneCbName) return false;
  const dxbc::ConstantBuffer* scene_layout = reflection.FindConstantBuffer(kSceneCbName, 0u);
  if (scene_layout == nullptr) return false;
  bool near_fade = false;
  for (const auto& variable : scene_layout->variables) {
    if (variable.name == "disableMapObjNearFade_g") {
      near_fade = variable.used && variable.offset == kSceneNearFadeFloorOffset;
    }
  }
  if (!near_fade) return false;
  const dxbc::ResourceBinding* instances = reflection.FindResource(dxbc::kInputStructured, kInstanceSlot);
  return instances != nullptr && HasContractVisibilityLayout(reflection, *instances);
}

// Which camera a vertex shader projects with (see "View" above).
enum class VsView : uint8_t {
  Other = 0,  // neither, or both
  Camera,     // cb_scene.viewProj_g
  Light,      // cb_shadow.shadowViewProj_g
};

inline const char* VsViewName(VsView view) {
  switch (view) {
    case VsView::Camera: return "camera";
    case VsView::Light:  return "light";
    default:             return "other";
  }
}

inline bool UsesBufferVariable(const dxbc::Reflection& reflection, std::string_view buffer, std::string_view name) {
  const dxbc::ConstantBuffer* layout = reflection.FindConstantBuffer(buffer, 0u);
  if (layout == nullptr) return false;
  for (const auto& variable : layout->variables) {
    if (variable.used && variable.name == name) return true;
  }
  return false;
}

inline VsView ClassifyVertexView(const dxbc::Reflection& reflection) {
  if (!reflection.valid) return VsView::Other;
  const bool camera = UsesBufferVariable(reflection, kSceneCbName, "viewProj_g");
  const bool light = UsesBufferVariable(reflection, "cb_shadow", "shadowViewProj_g");
  if (camera == light) return VsView::Other;
  return camera ? VsView::Camera : VsView::Light;
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
