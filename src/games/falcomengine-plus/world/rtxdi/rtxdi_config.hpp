#pragma once

#include <cstdint>
#include <limits>
#include <memory>

#include <Rtxdi/DI/ReSTIRDI.h>

#include "rtxdi_contract.h"
#include "rtxdi_di_layout.h"
#include "rtxdi_frame_parameters.h"

namespace falcom_world::rtxdi {

inline constexpr uint32_t kNeighborCount = 8192u;
inline constexpr uint64_t kNeighborBytes = uint64_t{kNeighborCount} * 2u;
static_assert((kNeighborCount & (kNeighborCount - 1u)) == 0u);
static_assert(kNeighborBytes == 16384u);

struct DiConfig {
  uint32_t width = 0u, height = 0u;
  uint32_t row_pitch = 0u, array_pitch = 0u;
  uint64_t reservoir_bytes = 0u;
};

enum class DiResourceAction { Skip,
                              Reuse,
                              Create,
                              OwnerMismatch };

struct DiResourceIdentity {
  DiConfig config;
  uint64_t owner_generation = 0u;
  uint64_t configuration_generation = 0u;
  bool present = false;
  bool complete = false;
};

inline DiResourceAction SelectDiResourceAction(bool local_requested, uint64_t owner_generation,
                                               const DiResourceIdentity& current, const DiConfig& requested) {
  if (!local_requested) return DiResourceAction::Skip;
  if (owner_generation == 0u || (current.present && current.owner_generation != owner_generation)) return DiResourceAction::OwnerMismatch;
  if (current.complete && current.configuration_generation != 0u && current.owner_generation == owner_generation && current.config.width == requested.width
      && current.config.height == requested.height && current.config.row_pitch == requested.row_pitch
      && current.config.array_pitch == requested.array_pitch && current.config.reservoir_bytes == requested.reservoir_bytes) return DiResourceAction::Reuse;
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

inline std::unique_ptr<::rtxdi::ReSTIRDIContext> CreateDiContext(const DiConfig& config) {
  DiConfig checked;
  if (!CalculateDiConfig(config.width, config.height, &checked) || checked.row_pitch != config.row_pitch
      || checked.array_pitch != config.array_pitch || checked.reservoir_bytes != config.reservoir_bytes) return nullptr;
  auto context = std::make_unique<::rtxdi::ReSTIRDIContext>(::rtxdi::ReSTIRDIStaticParameters{
      .NeighborOffsetCount = kNeighborCount, .RenderWidth = config.width, .RenderHeight = config.height, .CheckerboardSamplingMode = ::rtxdi::CheckerboardMode::Off});
  context->SetResamplingMode(::rtxdi::ReSTIRDI_ResamplingMode::TemporalAndSpatial);
  auto initial = context->GetInitialSamplingParameters();
  initial.numLocalLightSamples = 8u;
  initial.numInfiniteLightSamples = 0u;
  initial.numEnvironmentSamples = 0u;
  initial.numBrdfSamples = 0u;
  initial.localLightSamplingMode = ReSTIRDI_LocalLightSamplingMode::Uniform;
  initial.environmentMapImportanceSampling = 0u;
  context->SetInitialSamplingParameters(initial);
  auto spatial = context->GetSpatialResamplingParameters();
  spatial.numSamples = 4u;
  spatial.numDisocclusionBoostSamples = 4u;
  spatial.samplingRadius = 4.0f;
  spatial.biasCorrectionMode = ReSTIRDI_SpatialBiasCorrectionMode::Basic;
  spatial.depthThreshold = 0.05f;
  spatial.normalThreshold = 0.9f;
  spatial.targetHistoryLength = 1u;
  spatial.enableMaterialSimilarityTest = 1u;
  spatial.discountNaiveSamples = 0u;
  context->SetSpatialResamplingParameters(spatial);
  auto temporal = context->GetTemporalResamplingParameters();
  temporal.enableVisibilityShortcut = 0u;
  temporal.enablePermutationSampling = 0u;
  context->SetTemporalResamplingParameters(temporal);
  auto shading = context->GetShadingParameters();
  shading.enableFinalVisibility = 1u;
  shading.reuseFinalVisibility = 0u;
  shading.enableDenoiserInputPacking = 0u;
  context->SetShadingParameters(shading);
  context->SetFrameIndex(0u);
  if (!ValidTemporalSpatialBufferIndices(context->GetBufferIndices())) return nullptr;
  return context;
}

inline RtxdiFrameParameters MakeDiFrameParameters(const DiConfig& config, const ::rtxdi::ReSTIRDIContext& context) {
  RtxdiFrameParameters frame = {};
  frame.width = config.width;
  frame.height = config.height;
  frame.flags = RTXDI_FRAME_FLAG_SPATIAL_AFTER_INITIAL;
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
