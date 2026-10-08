#pragma once

// Runtime registry of classified game shaders.
//
// init_pipeline hands over every vertex/pixel shader's bytecode as the game
// creates it (after any RenoDX replacement, which keeps the original
// reflection). Each one is classified once with the bytecode contract in
// shader_contract.hpp and remembered by pipeline handle. Draw-time lookups
// use the pipeline handles RenoDX's shader state already tracks, so no hash
// list is consulted and unknown shaders are classified on sight.

#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <unordered_map>
#include <vector>

#include <include/reshade.hpp>

#include "../../../../utils/hash.hpp"
#include "shader_contract.hpp"

namespace falcom_world::contract {

// ShaderTraits::flags.
inline constexpr uint8_t kTraitVisibilityLayout = 1u;  // vertex: instance element has the color/param contract layout
inline constexpr uint8_t kTraitNearFade = 2u;          // pixel: applies the map-object near fade (UsesContractNearFade)
inline constexpr uint8_t kTraitCameraView = 4u;        // vertex: projects with the scene camera (VsView::Camera)
inline constexpr uint8_t kTraitLightView = 8u;         // vertex: projects with the light (VsView::Light)

struct ShaderTraits {
  uint8_t cls = 0u;    // VsClass or PsClass, by the map it lives in
  uint8_t flags = 0u;  // kTrait* bits
  uint32_t hash = 0u;  // CRC32 of the bytecode seen at creation (display only)
};

// Bytecode and output signature of a deforming vertex shader
// (IsDeformingClass), kept so a stream-out geometry shader can be built from
// it later (bvh/deform_probe.hpp).
struct ShaderCode {
  uint32_t hash = 0u;
  uint8_t cls = 0u;
  uint8_t flags = 0u;
  std::vector<uint8_t> bytecode;
  std::vector<dxbc::SignatureElement> outputs;
};

struct ShaderRegistry {
  std::shared_mutex mutex;
  std::unordered_map<uint64_t, ShaderTraits> vertex;
  std::unordered_map<uint64_t, ShaderTraits> pixel;
  std::unordered_map<uint64_t, std::shared_ptr<const ShaderCode>> deforming_code;  // by VS pipeline handle
  std::array<uint32_t, static_cast<size_t>(VsClass::Count)> vertex_counts = {};
  std::array<uint32_t, static_cast<size_t>(PsClass::Count)> pixel_counts = {};
};

inline ShaderRegistry& Registry() {
  static ShaderRegistry registry;
  return registry;
}

inline void OnInitPipelineClassify(
    reshade::api::device* device,
    reshade::api::pipeline_layout layout,
    uint32_t subobject_count,
    const reshade::api::pipeline_subobject* subobjects,
    reshade::api::pipeline pipeline) {
  (void)device;
  (void)layout;
  if (subobjects == nullptr || pipeline.handle == 0u) return;
  // D3D11 input layouts arrive as pipelines too, carrying the vertex shader
  // bytecode they were validated against. They are not shaders that get
  // bound, so they must not enter the registry or its counts.
  for (uint32_t i = 0; i < subobject_count; ++i) {
    if (subobjects[i].type == reshade::api::pipeline_subobject_type::input_layout) return;
  }
  for (uint32_t i = 0; i < subobject_count; ++i) {
    const auto& subobject = subobjects[i];
    const bool vertex = subobject.type == reshade::api::pipeline_subobject_type::vertex_shader;
    const bool pixel = subobject.type == reshade::api::pipeline_subobject_type::pixel_shader;
    if ((!vertex && !pixel) || subobject.count == 0u || subobject.data == nullptr) continue;
    const auto* desc = static_cast<const reshade::api::shader_desc*>(subobject.data);
    if (desc->code == nullptr || desc->code_size == 0u) continue;

    const dxbc::Reflection reflection = dxbc::Parse(desc->code, desc->code_size);
    ShaderTraits traits;
    traits.hash = renodx::utils::hash::ComputeCRC32(static_cast<const uint8_t*>(desc->code), desc->code_size);

    traits.cls = vertex ? static_cast<uint8_t>(ClassifyVertexShader(reflection))
                        : static_cast<uint8_t>(ClassifyPixelShader(reflection));
    if (vertex) {
      const dxbc::ResourceBinding* instances = reflection.FindResource(dxbc::kInputStructured, kInstanceSlot);
      if (instances != nullptr && HasContractVisibilityLayout(reflection, *instances)) {
        traits.flags |= kTraitVisibilityLayout;
      }
      const VsView view = ClassifyVertexView(reflection);
      if (view == VsView::Camera) traits.flags |= kTraitCameraView;
      if (view == VsView::Light) traits.flags |= kTraitLightView;
    } else if (UsesContractNearFade(reflection)) {
      traits.flags |= kTraitNearFade;
    }
    std::shared_ptr<const ShaderCode> code;
    if (vertex && IsDeformingClass(static_cast<VsClass>(traits.cls))) {
      auto copy = std::make_shared<ShaderCode>();
      copy->hash = traits.hash;
      copy->cls = traits.cls;
      copy->flags = traits.flags;
      const auto* bytes = static_cast<const uint8_t*>(desc->code);
      copy->bytecode.assign(bytes, bytes + desc->code_size);
      copy->outputs = reflection.outputs;
      code = std::move(copy);
    }

    auto& registry = Registry();
    std::unique_lock lock(registry.mutex);
    if (vertex) {
      if (code != nullptr) {
        registry.deforming_code.insert_or_assign(pipeline.handle, std::move(code));
      } else {
        registry.deforming_code.erase(pipeline.handle);
      }
    }
    auto& map = vertex ? registry.vertex : registry.pixel;
    // A handle can be reused without a destroy event reaching us; keep the
    // per-class counts consistent with what the map holds.
    if (const auto it = map.find(pipeline.handle); it != map.end()) {
      auto& previous = vertex ? registry.vertex_counts[it->second.cls] : registry.pixel_counts[it->second.cls];
      if (previous != 0u) previous -= 1u;
    }
    map.insert_or_assign(pipeline.handle, traits);
    (vertex ? registry.vertex_counts[traits.cls] : registry.pixel_counts[traits.cls]) += 1u;
  }
}

inline void OnDestroyPipelineClassify(reshade::api::device* device, reshade::api::pipeline pipeline) {
  (void)device;
  if (pipeline.handle == 0u) return;
  auto& registry = Registry();
  std::unique_lock lock(registry.mutex);
  if (const auto it = registry.vertex.find(pipeline.handle); it != registry.vertex.end()) {
    auto& count = registry.vertex_counts[it->second.cls];
    if (count != 0u) count -= 1u;
    registry.vertex.erase(it);
  }
  registry.deforming_code.erase(pipeline.handle);
  if (const auto it = registry.pixel.find(pipeline.handle); it != registry.pixel.end()) {
    auto& count = registry.pixel_counts[it->second.cls];
    if (count != 0u) count -= 1u;
    registry.pixel.erase(it);
  }
}

inline VsClass LookupVertexClass(uint64_t pipeline) {
  if (pipeline == 0u) return VsClass::Unclassified;
  auto& registry = Registry();
  std::shared_lock lock(registry.mutex);
  const auto it = registry.vertex.find(pipeline);
  return it != registry.vertex.end() ? static_cast<VsClass>(it->second.cls) : VsClass::Unclassified;
}

// A draw without a pixel shader (depth-only shadow casters) writes its whole
// surface, so it counts as opaque.
inline PsClass LookupPixelClass(uint64_t pipeline) {
  if (pipeline == 0u) return PsClass::Opaque;
  auto& registry = Registry();
  std::shared_lock lock(registry.mutex);
  const auto it = registry.pixel.find(pipeline);
  return it != registry.pixel.end() ? static_cast<PsClass>(it->second.cls) : PsClass::Unclassified;
}

// Full traits (class as in LookupVertexClass / LookupPixelClass, plus flags)
// with one lookup each.
inline ShaderTraits LookupVertexTraits(uint64_t pipeline) {
  ShaderTraits traits;
  traits.cls = static_cast<uint8_t>(VsClass::Unclassified);
  if (pipeline == 0u) return traits;
  auto& registry = Registry();
  std::shared_lock lock(registry.mutex);
  const auto it = registry.vertex.find(pipeline);
  return it != registry.vertex.end() ? it->second : traits;
}

inline ShaderTraits LookupPixelTraits(uint64_t pipeline) {
  ShaderTraits traits;
  traits.cls = static_cast<uint8_t>(pipeline == 0u ? PsClass::Opaque : PsClass::Unclassified);
  if (pipeline == 0u) return traits;
  auto& registry = Registry();
  std::shared_lock lock(registry.mutex);
  const auto it = registry.pixel.find(pipeline);
  return it != registry.pixel.end() ? it->second : traits;
}

// Bytecode of a deforming vertex shader pipeline, or null.
inline std::shared_ptr<const ShaderCode> LookupDeformingCode(uint64_t pipeline) {
  if (pipeline == 0u) return nullptr;
  auto& registry = Registry();
  std::shared_lock lock(registry.mutex);
  const auto it = registry.deforming_code.find(pipeline);
  return it != registry.deforming_code.end() ? it->second : nullptr;
}

struct RegistryCounts {
  std::array<uint32_t, static_cast<size_t>(VsClass::Count)> vertex = {};
  std::array<uint32_t, static_cast<size_t>(PsClass::Count)> pixel = {};
};

inline RegistryCounts SnapshotRegistryCounts() {
  RegistryCounts counts;
  auto& registry = Registry();
  std::shared_lock lock(registry.mutex);
  counts.vertex = registry.vertex_counts;
  counts.pixel = registry.pixel_counts;
  return counts;
}

// Distinct (hash, class) pairs per stage, for dumps. Several pipelines can
// share one bytecode hash; they classify identically, so one row each.
struct RegistryEntry {
  uint32_t hash = 0u;
  uint8_t cls = 0u;
  uint8_t flags = 0u;
};

struct RegistryEntries {
  std::vector<RegistryEntry> vertex;
  std::vector<RegistryEntry> pixel;
};

inline RegistryEntries SnapshotRegistryEntries() {
  RegistryEntries entries;
  auto& registry = Registry();
  std::shared_lock lock(registry.mutex);
  const auto collect = [](const std::unordered_map<uint64_t, ShaderTraits>& map, std::vector<RegistryEntry>* out) {
    std::unordered_map<uint32_t, ShaderTraits> unique;
    for (const auto& [handle, traits] : map) {
      (void)handle;
      unique.try_emplace(traits.hash, traits);
    }
    out->reserve(unique.size());
    for (const auto& [hash, traits] : unique) out->push_back({hash, traits.cls, traits.flags});
    std::sort(out->begin(), out->end(), [](const RegistryEntry& a, const RegistryEntry& b) {
      return a.cls != b.cls ? a.cls < b.cls : a.hash < b.hash;
    });
  };
  collect(registry.vertex, &entries.vertex);
  collect(registry.pixel, &entries.pixel);
  return entries;
}

inline void RegisterShaderRegistry() {
  reshade::register_event<reshade::addon_event::init_pipeline>(OnInitPipelineClassify);
  reshade::register_event<reshade::addon_event::destroy_pipeline>(OnDestroyPipelineClassify);
}

inline void UnregisterShaderRegistry() {
  reshade::unregister_event<reshade::addon_event::init_pipeline>(OnInitPipelineClassify);
  reshade::unregister_event<reshade::addon_event::destroy_pipeline>(OnDestroyPipelineClassify);
}

}  // namespace falcom_world::contract
