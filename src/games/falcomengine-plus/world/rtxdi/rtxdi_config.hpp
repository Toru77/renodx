#pragma once

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>

#include <Rtxdi/DI/ReSTIRDI.h>

#include "rtxdi_contract.h"
#include "rtxdi_di_layout.h"
#include "rtxdi_frame_parameters.h"
#include "rtxdi_state.hpp"

namespace falcom_world::rtxdi {

inline constexpr uint32_t kNeighborCount = 8192u;
inline constexpr uint64_t kNeighborBytes = uint64_t{kNeighborCount} * 2u;
static_assert((kNeighborCount & (kNeighborCount - 1u)) == 0u);
static_assert(kNeighborBytes == 16384u);
inline constexpr uint32_t kSpatialTargetHistoryLength = 2u;

struct DiConfig {
  uint32_t width = 0u, height = 0u;
  uint32_t row_pitch = 0u, array_pitch = 0u;
  uint64_t reservoir_bytes = 0u;
};

struct EffectiveRtxdiSettings {
  uint32_t quality_preset = 1u;
  uint32_t local_candidates = 8u;
  bool temporal_enabled = true;
  bool spatial_enabled = true;
  uint32_t spatial_samples = 4u;
  uint32_t spatial_disocclusion = 8u;
  float spatial_radius = 4.0f;
  uint32_t history_length = 20u;
  float temporal_depth_threshold = 0.1f;
  float temporal_normal_threshold = 0.5f;
  bool boiling_filter = true;
  float boiling_filter_strength = 0.2f;
  bool permutation_sampling = false;
  bool initial_visibility = false;
  bool stable_light_remap = false;
  float spatial_depth_threshold = 0.05f;
  float spatial_normal_threshold = 0.9f;
  bool relax_diffuse = true;
  uint32_t relax_diffuse_history = 30u;
  uint32_t relax_fast_history = 6u;
  uint32_t relax_prepass_radius = 30u;
  uint32_t relax_atrous_iterations = 5u;
  uint64_t identity = 0u;
};

inline EffectiveRtxdiSettings CaptureEffectiveRtxdiSettings() {
  EffectiveRtxdiSettings effective;
  const auto integer_setting = [](float value, float fallback, long minimum, long maximum) {
    return static_cast<uint32_t>(std::clamp(std::lround(std::isfinite(value) ? value : fallback), minimum, maximum));
  };
  effective.quality_preset = integer_setting(g_quality_preset, 1.0f, 0l, 3l);
  effective.local_candidates = integer_setting(g_local_candidates, 8.0f, 1l, 32l);
  effective.temporal_enabled = g_temporal_reuse > 0.5f;
  effective.spatial_enabled = g_spatial_reuse > 0.5f;
  effective.spatial_samples = integer_setting(g_spatial_samples, 4.0f, 1l, 16l);
  effective.spatial_disocclusion = integer_setting(g_spatial_disocclusion, 8.0f, 1l, 16l);
  effective.spatial_radius = std::clamp(std::isfinite(g_spatial_radius) ? g_spatial_radius : 4.0f, 1.0f, 32.0f);
  effective.history_length = integer_setting(g_history_length, 20.0f, 1l, 32l);
  effective.temporal_depth_threshold = std::clamp(std::isfinite(g_temporal_depth_threshold) ? g_temporal_depth_threshold : 0.1f, 0.0f, 1.0f);
  effective.temporal_normal_threshold = std::clamp(std::isfinite(g_temporal_normal_threshold) ? g_temporal_normal_threshold : 0.5f, -1.0f, 1.0f);
  effective.boiling_filter = g_boiling_filter > 0.5f;
  effective.boiling_filter_strength = std::clamp(std::isfinite(g_boiling_filter_strength) ? g_boiling_filter_strength : 0.2f, 0.01f, 1.0f);
  effective.permutation_sampling = g_permutation_sampling > 0.5f;
  effective.initial_visibility = g_initial_visibility > 0.5f;
  effective.stable_light_remap = g_local_stable_light_remap > 0.5f;
  effective.spatial_depth_threshold = std::clamp(std::isfinite(g_spatial_depth_threshold) ? g_spatial_depth_threshold : 0.05f, 0.0f, 1.0f);
  effective.spatial_normal_threshold = std::clamp(std::isfinite(g_spatial_normal_threshold) ? g_spatial_normal_threshold : 0.9f, -1.0f, 1.0f);
  effective.relax_diffuse = g_diffuse_denoiser > 0.5f;
  effective.relax_diffuse_history = integer_setting(g_relax_diffuse_history, 30.0f, 1l, 128l);
  effective.relax_fast_history = integer_setting(g_relax_fast_history, 6.0f, 1l, 32l);
  effective.relax_fast_history = (std::min)(effective.relax_fast_history, effective.relax_diffuse_history);
  effective.relax_prepass_radius = integer_setting(g_relax_prepass_radius, 30.0f, 0l, 64l);
  effective.relax_atrous_iterations = integer_setting(g_relax_atrous_iterations, 5.0f, 2l, 8l);
  if (effective.quality_preset != 3u) {
    constexpr uint32_t candidates[] = {16u, 32u, 32u};
    constexpr uint32_t spatial[] = {1u, 1u, 2u};
    constexpr uint32_t disocclusion[] = {4u, 8u, 16u};
    constexpr float radius[] = {16.f, 30.f, 30.f};
    constexpr uint32_t history[] = {20u, 20u, 20u};
    constexpr float boiling[] = {0.2f, 0.2f, 0.2f};
    constexpr bool permutation[] = {false, true, true};
    constexpr bool visibility[] = {false, true, true};
    effective.local_candidates = candidates[effective.quality_preset];
    effective.spatial_samples = spatial[effective.quality_preset];
    effective.spatial_disocclusion = disocclusion[effective.quality_preset];
    effective.spatial_radius = radius[effective.quality_preset];
    effective.history_length = history[effective.quality_preset];
    effective.boiling_filter_strength = boiling[effective.quality_preset];
    effective.permutation_sampling = permutation[effective.quality_preset];
    effective.initial_visibility = visibility[effective.quality_preset];
  }
  uint64_t hash = 1469598103934665603ull;
  const auto add = [&hash](uint32_t value) {
    hash ^= value;
    hash *= 1099511628211ull;
  };
  add(effective.quality_preset);
  add(effective.local_candidates);
  add(effective.temporal_enabled ? 1u : 0u);
  add(effective.spatial_enabled ? 1u : 0u);
  add(effective.spatial_samples);
  add(effective.spatial_disocclusion);
  add(std::bit_cast<uint32_t>(effective.spatial_radius));
  add(effective.history_length);
  add(std::bit_cast<uint32_t>(effective.temporal_depth_threshold));
  add(std::bit_cast<uint32_t>(effective.temporal_normal_threshold));
  add(effective.boiling_filter ? 1u : 0u);
  add(std::bit_cast<uint32_t>(effective.boiling_filter_strength));
  add(effective.permutation_sampling ? 1u : 0u);
  add(effective.initial_visibility ? 1u : 0u);
  add(effective.stable_light_remap ? 1u : 0u);
  add(std::bit_cast<uint32_t>(effective.spatial_depth_threshold));
  add(std::bit_cast<uint32_t>(effective.spatial_normal_threshold));
  add(effective.relax_diffuse ? 1u : 0u);
  add(effective.relax_diffuse_history);
  add(effective.relax_fast_history);
  add(effective.relax_prepass_radius);
  add(effective.relax_atrous_iterations);
  effective.identity = hash;
  return effective;
}

enum class DiResourceAction { Skip,
                              Reuse,
                              Create,
                              OwnerMismatch };

struct DiResourceIdentity {
  DiConfig config;
  uint64_t owner_generation = 0u;
  uint64_t configuration_generation = 0u;
  uint64_t settings_identity = 0u;
  bool present = false;
  bool complete = false;
};

inline DiResourceAction SelectDiResourceAction(bool local_requested, uint64_t owner_generation,
                                               const DiResourceIdentity& current, const DiConfig& requested,
                                               uint64_t settings_identity) {
  if (!local_requested) return DiResourceAction::Skip;
  if (owner_generation == 0u || (current.present && current.owner_generation != owner_generation)) return DiResourceAction::OwnerMismatch;
  if (current.complete && current.configuration_generation != 0u && current.owner_generation == owner_generation && current.config.width == requested.width
      && current.config.height == requested.height && current.config.row_pitch == requested.row_pitch
      && current.config.array_pitch == requested.array_pitch && current.config.reservoir_bytes == requested.reservoir_bytes
      && current.settings_identity == settings_identity) return DiResourceAction::Reuse;
  return DiResourceAction::Create;
}

inline bool NextDiConfigurationGeneration(uint64_t current, uint64_t* next) {
  if (!next || current == UINT64_MAX) return false;
  *next = current + 1u;
  return true;
}

inline bool CalculateDiConfig(uint32_t width, uint32_t height, DiConfig* out) {
  if (!out || width == 0u || height == 0u || width > UINT32_MAX - 15u || height > UINT32_MAX - 15u) return false;
  const uint64_t row = ((uint64_t{width} + 15u) / 16u) * 256u;
  const uint64_t pitch = row * ((uint64_t{height} + 15u) / 16u);
  if (row > UINT32_MAX || pitch > UINT32_MAX / ::rtxdi::c_NumReSTIRDIReservoirBuffers) return false;
  const uint64_t elements = pitch * ::rtxdi::c_NumReSTIRDIReservoirBuffers;
  if (elements > (std::numeric_limits<uint64_t>::max)() / sizeof(RTXDI_PackedDIReservoir)) return false;
  *out = {.width = width, .height = height, .row_pitch = static_cast<uint32_t>(row), .array_pitch = static_cast<uint32_t>(pitch), .reservoir_bytes = elements * sizeof(RTXDI_PackedDIReservoir)};
  return true;
}

inline bool ValidSpatialBufferIndices(const RTXDI_DIBufferIndices& indices) {
  const uint32_t initial = indices.initialSamplingOutputBufferIndex;
  const uint32_t spatial_input = indices.spatialResamplingInputBufferIndex;
  const uint32_t spatial_output = indices.spatialResamplingOutputBufferIndex;
  const uint32_t shading_input = indices.shadingInputBufferIndex;
  const uint32_t previous = indices.temporalResamplingInputBufferIndex;
  return initial < ::rtxdi::c_NumReSTIRDIReservoirBuffers && spatial_input == initial
         && spatial_input < ::rtxdi::c_NumReSTIRDIReservoirBuffers
         && spatial_output < ::rtxdi::c_NumReSTIRDIReservoirBuffers && shading_input == spatial_output
         && previous < ::rtxdi::c_NumReSTIRDIReservoirBuffers && initial != spatial_output
         && previous != initial && previous != spatial_output;
}

inline bool ValidTemporalSpatialBufferIndices(const RTXDI_DIBufferIndices& indices) {
  const uint32_t initial = indices.initialSamplingOutputBufferIndex;
  const uint32_t temporal_input = indices.temporalResamplingInputBufferIndex;
  const uint32_t temporal_output = indices.temporalResamplingOutputBufferIndex;
  const uint32_t spatial_input = indices.spatialResamplingInputBufferIndex;
  const uint32_t spatial_output = indices.spatialResamplingOutputBufferIndex;
  const uint32_t shading_input = indices.shadingInputBufferIndex;
  const uint32_t count = ::rtxdi::c_NumReSTIRDIReservoirBuffers;
  return initial < count && temporal_input < count && temporal_output < count && spatial_input < count
         && spatial_output < count && shading_input < count && initial == temporal_output && initial == spatial_input
         && temporal_input != initial && spatial_output == shading_input && spatial_output != initial
         && spatial_output != temporal_input;
}

inline bool ValidDiBufferIndices(const RTXDI_DIBufferIndices& indices, const EffectiveRtxdiSettings& settings) {
  const uint32_t count = ::rtxdi::c_NumReSTIRDIReservoirBuffers;
  const uint32_t initial = indices.initialSamplingOutputBufferIndex;
  const uint32_t temporal_input = indices.temporalResamplingInputBufferIndex;
  const uint32_t temporal_output = indices.temporalResamplingOutputBufferIndex;
  const uint32_t spatial_input = indices.spatialResamplingInputBufferIndex;
  const uint32_t spatial_output = indices.spatialResamplingOutputBufferIndex;
  const uint32_t shading_input = indices.shadingInputBufferIndex;
  return initial < count && temporal_input < count && temporal_output < count && spatial_input < count
         && spatial_output < count && shading_input < count && temporal_output == initial && spatial_input == initial
         && shading_input == (settings.spatial_enabled ? spatial_output : temporal_output)
         && (!settings.temporal_enabled || temporal_input != initial)
         && (!settings.spatial_enabled || spatial_output != initial)
         && (!settings.temporal_enabled || !settings.spatial_enabled || temporal_input != spatial_output);
}

inline std::unique_ptr<::rtxdi::ReSTIRDIContext> CreateDiContext(const DiConfig& config, const EffectiveRtxdiSettings& settings) {
  DiConfig checked;
  if (!CalculateDiConfig(config.width, config.height, &checked) || checked.row_pitch != config.row_pitch
      || checked.array_pitch != config.array_pitch || checked.reservoir_bytes != config.reservoir_bytes) return nullptr;
  auto context = std::make_unique<::rtxdi::ReSTIRDIContext>(::rtxdi::ReSTIRDIStaticParameters{
      .NeighborOffsetCount = kNeighborCount, .RenderWidth = config.width, .RenderHeight = config.height, .CheckerboardSamplingMode = ::rtxdi::CheckerboardMode::Off});
  context->SetResamplingMode(settings.temporal_enabled && settings.spatial_enabled ? ::rtxdi::ReSTIRDI_ResamplingMode::TemporalAndSpatial
                             : settings.temporal_enabled                           ? ::rtxdi::ReSTIRDI_ResamplingMode::Temporal
                             : settings.spatial_enabled                            ? ::rtxdi::ReSTIRDI_ResamplingMode::Spatial
                                                                                   : ::rtxdi::ReSTIRDI_ResamplingMode::None);
  auto initial = context->GetInitialSamplingParameters();
  initial.numLocalLightSamples = settings.local_candidates;
  initial.numInfiniteLightSamples = 0u;
  initial.numEnvironmentSamples = 0u;
  initial.numBrdfSamples = 0u;
  initial.localLightSamplingMode = ReSTIRDI_LocalLightSamplingMode::Uniform;
  initial.environmentMapImportanceSampling = 0u;
  initial.enableInitialVisibility = settings.initial_visibility ? 1u : 0u;
  context->SetInitialSamplingParameters(initial);
  auto spatial = context->GetSpatialResamplingParameters();
  spatial.numSamples = settings.spatial_samples;
  spatial.numDisocclusionBoostSamples = settings.spatial_disocclusion;
  spatial.samplingRadius = settings.spatial_radius;
  spatial.biasCorrectionMode = ReSTIRDI_SpatialBiasCorrectionMode::Basic;
  spatial.depthThreshold = settings.spatial_depth_threshold;
  spatial.normalThreshold = settings.spatial_normal_threshold;
  spatial.targetHistoryLength = (settings.temporal_enabled ? kSpatialTargetHistoryLength : 0u);
  spatial.enableMaterialSimilarityTest = 1u;
  spatial.discountNaiveSamples = 0u;
  context->SetSpatialResamplingParameters(spatial);
  auto temporal = context->GetTemporalResamplingParameters();
  temporal.enableVisibilityShortcut = 0u;
  temporal.enablePermutationSampling = settings.permutation_sampling ? 1u : 0u;
  temporal.maxHistoryLength = settings.history_length;
  temporal.depthThreshold = settings.temporal_depth_threshold;
  temporal.normalThreshold = settings.temporal_normal_threshold;
  context->SetTemporalResamplingParameters(temporal);
  auto boiling = context->GetBoilingFilterParameters();
  boiling.enableBoilingFilter = settings.boiling_filter ? 1u : 0u;
  boiling.boilingFilterStrength = settings.boiling_filter_strength;
  context->SetBoilingFilterParameters(boiling);
  auto shading = context->GetShadingParameters();
  shading.enableFinalVisibility = 1u;
  shading.reuseFinalVisibility = 0u;
  shading.enableDenoiserInputPacking = 0u;
  context->SetShadingParameters(shading);
  context->SetFrameIndex(0u);
  const auto indices = context->GetBufferIndices();
  if (indices.initialSamplingOutputBufferIndex >= ::rtxdi::c_NumReSTIRDIReservoirBuffers
      || indices.temporalResamplingOutputBufferIndex != indices.initialSamplingOutputBufferIndex
      || indices.spatialResamplingInputBufferIndex != indices.initialSamplingOutputBufferIndex
      || indices.spatialResamplingOutputBufferIndex >= ::rtxdi::c_NumReSTIRDIReservoirBuffers
      || indices.shadingInputBufferIndex >= ::rtxdi::c_NumReSTIRDIReservoirBuffers
      || !ValidDiBufferIndices(indices, settings)) return nullptr;
  return context;
}

inline RtxdiFrameParameters MakeDiFrameParameters(const DiConfig& config, const ::rtxdi::ReSTIRDIContext& context) {
  RtxdiFrameParameters frame = {};
  frame.width = config.width;
  frame.height = config.height;
  frame.flags = RTXDI_FRAME_FLAG_SPATIAL_AFTER_INITIAL
                | (context.GetResamplingMode() == ::rtxdi::ReSTIRDI_ResamplingMode::Spatial
                           || context.GetResamplingMode() == ::rtxdi::ReSTIRDI_ResamplingMode::TemporalAndSpatial
                       ? RTXDI_FRAME_FLAG_SPATIAL_ENABLED
                       : 0u);
  const auto runtime = context.GetRuntimeParams();
  frame.runtime.neighborOffsetMask = runtime.neighborOffsetMask;
  frame.runtime.activeCheckerboardField = runtime.activeCheckerboardField;
  frame.runtime.frameIndex = runtime.frameIndex;
  frame.di.reservoirBufferParams.reservoirBlockRowPitch = config.row_pitch;
  frame.di.reservoirBufferParams.reservoirArrayPitch = config.array_pitch;
  frame.di.bufferIndices = context.GetBufferIndices();
  frame.di.bufferIndices.pad1 = frame.di.bufferIndices.pad2 = 0u;
  frame.di.initialSamplingParams = context.GetInitialSamplingParameters();
  frame.di.initialSamplingParams.pad1 = frame.di.initialSamplingParams.pad2 = frame.di.initialSamplingParams.pad3 = 0u;
  frame.di.temporalResamplingParams = context.GetTemporalResamplingParameters();
  frame.di.boilingFilterParams = context.GetBoilingFilterParameters();
  frame.di.boilingFilterParams.pad1 = frame.di.boilingFilterParams.pad2 = 0u;
  frame.di.spatialResamplingParams = context.GetSpatialResamplingParameters();
  frame.di.spatialResamplingParams.pad1 = frame.di.spatialResamplingParams.pad2 = frame.di.spatialResamplingParams.pad3 = 0u;
  frame.di.spatioTemporalResamplingParams = context.GetSpatioTemporalResamplingParameters();
  frame.di.shadingParams = context.GetShadingParameters();
  frame.di.shadingParams.pad1 = frame.di.shadingParams.pad2 = frame.di.shadingParams.pad3 = 0u;
  return frame;
}

}  // namespace falcom_world::rtxdi
