#include "../rtxdi_contract.h"
#include "../../bvh/alpha_atlas.hpp"
#include "../rtxdi_camera_qualification.hpp"
#include "../rtxdi_config.hpp"
#include "../rtxdi_debug_layout.h"
#include "../rtxdi_diag_layout.h"
#include "../rtxdi_di_contract.h"
#include "../rtxdi_diagnostics.hpp"
#include "../rtxdi_input_validation.hpp"
#include "../rtxdi_perf_contract.hpp"
#include "../rtxdi_state.hpp"
#include "../rtxdi_surface_layout.h"

#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <memory>

#include <Rtxdi/DI/ReSTIRDI.h>
#include <Rtxdi/DI/ReSTIRDIParameters.h>
#include <Rtxdi/RtxdiParameters.h>
#include <Rtxdi/RtxdiUtils.h>

namespace {
using SurfaceSceneRows = std::array<std::array<uint32_t, 4>, RTXDI_SURFACE_SCENE_ROW_COUNT>;

struct TestSurfaceResult {
  std::array<float, 3> world_position = {};
  std::array<float, 3> shading_normal = {};
  std::array<float, 3> view_direction = {};
  float linear_view_depth = 0.f;
  uint32_t flags = 0u;
  uint32_t rejection = RTXDI_SURFACE_REJECT_NONE;
};

float SceneFloat(const SurfaceSceneRows& rows, uint32_t row, uint32_t column) {
  float value = 0.f;
  std::memcpy(&value, &rows[row][column], sizeof(value));
  return value;
}

void SetSceneRow(SurfaceSceneRows* rows, uint32_t row, const std::array<float, 4>& values) {
  std::memcpy((*rows)[row].data(), values.data(), sizeof(values));
}

std::array<float, 4> TransformSceneRegisters(const std::array<float, 4>& vector, const SurfaceSceneRows& rows, uint32_t first_row) {
  std::array<float, 4> result = {};
  for (uint32_t row = 0u; row < 4u; ++row) {
    for (uint32_t column = 0u; column < 4u; ++column) result[row] += vector[column] * SceneFloat(rows, first_row + row, column);
  }
  return result;
}

TestSurfaceResult ReconstructTestSurface(float raw_depth, const std::array<uint32_t, 4>& mrt0, uint32_t pixel_x,
                                         uint32_t pixel_y, uint32_t width, uint32_t height,
                                         const SurfaceSceneRows& scene_rows) {
  TestSurfaceResult result = {};
  const auto reject = [&](uint32_t reason) {
    return TestSurfaceResult{.rejection = reason};
  };
  if (width == 0u || height == 0u) return reject(RTXDI_SURFACE_REJECT_DIMENSIONS);
  if (pixel_x >= width || pixel_y >= height) return reject(RTXDI_SURFACE_REJECT_PIXEL);
  const float scaling_x = SceneFloat(scene_rows, RTXDI_SURFACE_RESOLUTION_SCALING_ROW, 0u);
  const float scaling_y = SceneFloat(scene_rows, RTXDI_SURFACE_RESOLUTION_SCALING_ROW, 1u);
  if (!std::isfinite(scaling_x) || !std::isfinite(scaling_y) || scaling_x != 1.f || scaling_y != 1.f) return reject(RTXDI_SURFACE_REJECT_SCALING);
  if (!std::isfinite(raw_depth) || raw_depth <= 0.f || raw_depth > 1.f) return reject(RTXDI_SURFACE_REJECT_DEPTH);

  constexpr float kPi = 3.14159274f;
  const float encoded_x = static_cast<float>(mrt0[0]) * 3.05180438e-05f - 1.f;
  const float encoded_z = static_cast<float>(mrt0[1]) * 3.05180438e-05f - 1.f;
  const float angle = kPi * encoded_x;
  const float ring = std::sqrt(1.f - encoded_z * encoded_z);
  result.shading_normal = {std::cos(angle) * ring, std::sin(angle) * ring, encoded_z};
  const float normal_length_squared = result.shading_normal[0] * result.shading_normal[0]
                                      + result.shading_normal[1] * result.shading_normal[1]
                                      + result.shading_normal[2] * result.shading_normal[2];
  if (!std::isfinite(result.shading_normal[0]) || !std::isfinite(result.shading_normal[1])
      || !std::isfinite(result.shading_normal[2]) || normal_length_squared < 0.5f) return reject(RTXDI_SURFACE_REJECT_NORMAL);

  const float u = (static_cast<float>(pixel_x) + 0.5f) / static_cast<float>(width);
  const float v = (static_cast<float>(pixel_y) + 0.5f) / static_cast<float>(height);
  const auto world_h = TransformSceneRegisters({u * 2.f - 1.f, 1.f - v * 2.f, raw_depth, 1.f}, scene_rows,
                                               RTXDI_SURFACE_INVERSE_VIEW_PROJECTION_ROW);
  if (!std::isfinite(world_h[3]) || std::abs(world_h[3]) <= 1e-8f) return reject(RTXDI_SURFACE_REJECT_HOMOGENEOUS_W);
  if (!std::isfinite(world_h[0]) || !std::isfinite(world_h[1]) || !std::isfinite(world_h[2])) {
    return reject(RTXDI_SURFACE_REJECT_POSITION);
  }
  for (uint32_t axis = 0u; axis < 3u; ++axis) result.world_position[axis] = world_h[axis] / world_h[3];
  if (!std::isfinite(result.world_position[0]) || !std::isfinite(result.world_position[1])
      || !std::isfinite(result.world_position[2])) return reject(RTXDI_SURFACE_REJECT_POSITION);

  const std::array<float, 3> camera = {
      SceneFloat(scene_rows, RTXDI_SURFACE_INVERSE_VIEW_ROW + 0u, 3u),
      SceneFloat(scene_rows, RTXDI_SURFACE_INVERSE_VIEW_ROW + 1u, 3u),
      SceneFloat(scene_rows, RTXDI_SURFACE_INVERSE_VIEW_ROW + 2u, 3u)};
  std::array<float, 3> to_camera = {};
  float to_camera_length_squared = 0.f;
  for (uint32_t axis = 0u; axis < 3u; ++axis) {
    to_camera[axis] = camera[axis] - result.world_position[axis];
    to_camera_length_squared += to_camera[axis] * to_camera[axis];
  }
  if (!std::isfinite(camera[0]) || !std::isfinite(camera[1]) || !std::isfinite(camera[2])
      || !std::isfinite(to_camera_length_squared) || to_camera_length_squared <= 1e-12f) return reject(RTXDI_SURFACE_REJECT_VIEW_DIRECTION);
  const float inverse_length = 1.f / std::sqrt(to_camera_length_squared);
  for (uint32_t axis = 0u; axis < 3u; ++axis) result.view_direction[axis] = to_camera[axis] * inverse_length;
  if (!std::isfinite(result.view_direction[0]) || !std::isfinite(result.view_direction[1])
      || !std::isfinite(result.view_direction[2])) return reject(RTXDI_SURFACE_REJECT_VIEW_DIRECTION);

  const auto view_position = TransformSceneRegisters({result.world_position[0], result.world_position[1], result.world_position[2], 1.f},
                                                     scene_rows, RTXDI_SURFACE_VIEW_ROW);
  result.linear_view_depth = -view_position[2];
  if (!std::isfinite(result.linear_view_depth) || result.linear_view_depth <= 0.f) return reject(RTXDI_SURFACE_REJECT_VIEW_DEPTH);
  result.flags = RTXDI_SURFACE_FLAG_VALID;
  if ((mrt0[3] & 8u) == 0u) result.flags |= RTXDI_SURFACE_FLAG_CHARACTER;
  return result;
}

SurfaceSceneRows MakeSurfaceSceneRows() {
  SurfaceSceneRows rows = {};
  SetSceneRow(&rows, RTXDI_SURFACE_VIEW_ROW + 0u, {0.f, 0.f, 1.f, -10.f});
  SetSceneRow(&rows, RTXDI_SURFACE_VIEW_ROW + 1u, {0.f, 1.f, 0.f, -2.f});
  SetSceneRow(&rows, RTXDI_SURFACE_VIEW_ROW + 2u, {-1.f, 0.f, 0.f, 3.f});
  SetSceneRow(&rows, RTXDI_SURFACE_VIEW_ROW + 3u, {0.f, 0.f, 0.f, 1.f});
  SetSceneRow(&rows, RTXDI_SURFACE_INVERSE_VIEW_ROW + 0u, {0.f, 0.f, 0.f, 3.f});
  SetSceneRow(&rows, RTXDI_SURFACE_INVERSE_VIEW_ROW + 1u, {0.f, 0.f, 0.f, 2.f});
  SetSceneRow(&rows, RTXDI_SURFACE_INVERSE_VIEW_ROW + 2u, {0.f, 0.f, 0.f, 10.f});
  SetSceneRow(&rows, RTXDI_SURFACE_INVERSE_VIEW_PROJECTION_ROW + 0u, {1.f, 0.f, 0.f, 5.f});
  SetSceneRow(&rows, RTXDI_SURFACE_INVERSE_VIEW_PROJECTION_ROW + 1u, {0.f, 1.f, 0.f, 2.f});
  SetSceneRow(&rows, RTXDI_SURFACE_INVERSE_VIEW_PROJECTION_ROW + 2u, {0.f, 0.f, -4.f, 5.f});
  SetSceneRow(&rows, RTXDI_SURFACE_INVERSE_VIEW_PROJECTION_ROW + 3u, {0.f, 0.f, 0.f, 1.f});
  SetSceneRow(&rows, RTXDI_SURFACE_RESOLUTION_SCALING_ROW, {1.f, 1.f, 0.f, 0.f});
  return rows;
}

bool SurfaceIsZero(const TestSurfaceResult& result) {
  return result.world_position == std::array<float, 3>{}
         && result.shading_normal == std::array<float, 3>{}
         && result.view_direction == std::array<float, 3>{} && result.linear_view_depth == 0.f && result.flags == 0u;
}
}  // namespace

int main() {
  if (ResolveRtxdiDebugEffectiveView(RTXDI_DEBUG_VIEW_OFF, false) != RTXDI_DEBUG_VIEW_OFF
      || ResolveRtxdiDebugEffectiveView(RTXDI_DEBUG_VIEW_RAW_DIFFUSE, true) != RTXDI_DEBUG_VIEW_OFF
      || ResolveRtxdiDebugEffectiveView(RTXDI_DEBUG_VIEW_FILTERED_DIFFUSE, false)
             != RTXDI_DEBUG_VIEW_FILTERED_DIFFUSE
      || RtxdiDebugDistanceExportRequested(
          ResolveRtxdiDebugEffectiveView(RTXDI_DEBUG_VIEW_DISTANCE_MAGNITUDE, true))
      || !RtxdiDebugDistanceExportRequested(
          ResolveRtxdiDebugEffectiveView(RTXDI_DEBUG_VIEW_DISTANCE_MAGNITUDE, false))
      || RtxdiDebugDistanceExportRequested(ResolveRtxdiDebugEffectiveView(RTXDI_DEBUG_VIEW_OFF, false))
      || ResolveRtxdiDebugEffectiveView(RTXDI_DEBUG_VIEW_OFF, true) != RTXDI_DEBUG_VIEW_OFF
      || ResolveRtxdiDebugEffectiveView(RTXDI_DEBUG_VIEW_LOCAL_STATUS, false)
             != RTXDI_DEBUG_VIEW_LOCAL_STATUS) {
    return EXIT_FAILURE;
  }
  std::printf("PASS RTXDI shader view precedence: BVH forces OFF, unavailable keeps requested checker, Off/restore\n");
  using falcom_world::rtxdi::DiagnosticDebugFreshness;
  using falcom_world::rtxdi::GetDiagnosticDebugFreshness;
  const float saved_debug_view = falcom_world::rtxdi::g_debug_view;
  falcom_world::rtxdi::g_debug_view = static_cast<float>(RTXDI_DEBUG_VIEW_SUN_VISIBILITY);
  const uint32_t live_sun_view = static_cast<uint32_t>(falcom_world::rtxdi::g_debug_view);
  if (GetDiagnosticDebugFreshness(9u, 9u, 40u, RTXDI_DEBUG_VIEW_OFF, RTXDI_DEBUG_VIEW_OFF, 40u)
          != DiagnosticDebugFreshness::Pending
      || GetDiagnosticDebugFreshness(live_sun_view, live_sun_view, 40u, live_sun_view, live_sun_view, 40u)
             != DiagnosticDebugFreshness::Matched
      || GetDiagnosticDebugFreshness(live_sun_view, RTXDI_DEBUG_VIEW_OFF, 40u, live_sun_view,
                                     RTXDI_DEBUG_VIEW_OFF, 40u)
             != DiagnosticDebugFreshness::Matched
      || GetDiagnosticDebugFreshness(RTXDI_DEBUG_VIEW_OFF, RTXDI_DEBUG_VIEW_OFF, 40u, live_sun_view,
                                     live_sun_view, 40u)
             != DiagnosticDebugFreshness::Pending) {
    falcom_world::rtxdi::g_debug_view = saved_debug_view;
    return EXIT_FAILURE;
  }
  falcom_world::rtxdi::g_debug_view = static_cast<float>(RTXDI_DEBUG_VIEW_OFF);
  const uint32_t reloaded_off_view = static_cast<uint32_t>(falcom_world::rtxdi::g_debug_view);
  if (GetDiagnosticDebugFreshness(reloaded_off_view, reloaded_off_view, 40u, live_sun_view, live_sun_view, 40u)
          != DiagnosticDebugFreshness::Pending
      || GetDiagnosticDebugFreshness(reloaded_off_view, reloaded_off_view, 40u, reloaded_off_view,
                                     reloaded_off_view, 40u)
             != DiagnosticDebugFreshness::Matched
      || GetDiagnosticDebugFreshness(live_sun_view, live_sun_view, 40u, 0u, 0u, UINT32_MAX)
             != DiagnosticDebugFreshness::Unobserved) {
    falcom_world::rtxdi::g_debug_view = saved_debug_view;
    return EXIT_FAILURE;
  }
  falcom_world::rtxdi::g_debug_view = saved_debug_view;
  std::printf("PASS RTXDI debug live/render-observed freshness pending/matched/unobserved, BVH supersession, selector reset/reload\n");
  if (ResolveRtxdiDebugPresentationOwner(RTXDI_DEBUG_VIEW_OFF, false)
          != RtxdiDebugPresentationOwner::Lighting
      || ResolveRtxdiDebugPresentationOwner(RTXDI_DEBUG_VIEW_LOCAL_STATUS, false)
             != RtxdiDebugPresentationOwner::Rtxdi
      || ResolveRtxdiDebugPresentationOwner(RTXDI_DEBUG_VIEW_LOCAL_STATUS, true)
             != RtxdiDebugPresentationOwner::Bvh) {
    return EXIT_FAILURE;
  }
  const bool render_observed_bvh_supersession = true;
  for (const bool live_bvh_active : {false, true}) {
    const uint32_t live_effective_view =
        ResolveRtxdiDebugEffectiveView(RTXDI_DEBUG_VIEW_SUN_VISIBILITY, live_bvh_active);
    if (live_effective_view != (live_bvh_active ? RTXDI_DEBUG_VIEW_OFF : RTXDI_DEBUG_VIEW_SUN_VISIBILITY)
        || ResolveRtxdiDebugPresentationOwner(RTXDI_DEBUG_VIEW_SUN_VISIBILITY,
                                              render_observed_bvh_supersession)
               != RtxdiDebugPresentationOwner::Bvh) {
      return EXIT_FAILURE;
    }
  }
  if (ResolveRtxdiDebugPresentationOwner(RTXDI_DEBUG_VIEW_SUN_VISIBILITY, false)
      != RtxdiDebugPresentationOwner::Rtxdi) {
    return EXIT_FAILURE;
  }
  std::printf("PASS RTXDI debug precedence: BVH > RTXDI > lighting; observed owner stays latched until callback\n");
  if (RtxdiSunDetailExportRequested(RTXDI_DEBUG_VIEW_SUN_VISIBILITY, RTXDI_SUN_DETAIL_STATUS)
      || !RtxdiSunDetailExportRequested(RTXDI_DEBUG_VIEW_SUN_VISIBILITY, RTXDI_SUN_DETAIL_REASON)
      || !RtxdiSunDetailExportRequested(RTXDI_DEBUG_VIEW_SUN_VISIBILITY, RTXDI_SUN_DETAIL_HIT_DISTANCE)
      || RtxdiSunDetailExportRequested(RTXDI_DEBUG_VIEW_LOCAL_STATUS, RTXDI_SUN_DETAIL_REASON)
      || RtxdiSunDetailExportRequested(ResolveRtxdiDebugEffectiveView(RTXDI_DEBUG_VIEW_SUN_VISIBILITY, true),
                                      RTXDI_SUN_DETAIL_HIT_DISTANCE)) {
    return EXIT_FAILURE;
  }
  std::printf("PASS RTXDI sun detail export gate: Status/Off/BVH perform no optional work; Reason/Hit distance only under effective Sun Visibility\n");
  {
    using namespace falcom_world::rtxdi;
    auto diagnostics_owner = std::make_unique<DiagnosticState>();
    auto& diagnostics = *diagnostics_owner;
    const DiagnosticEvent disabled_event = {.frame = 1u, .stage = DiagnosticStage::InputValidate, .reason = DiagnosticReason::MissingEntry};
    RecordDiagnostic(diagnostics, 0u, disabled_event);
    if (CopyDiagnosticSnapshot(diagnostics).transition_size != 0u) return EXIT_FAILURE;
    SetDiagnosticCapture(diagnostics, true);
    const uint64_t first_epoch = DiagnosticCaptureEpoch(diagnostics);
    DiagnosticEvent repeated_failure = {.owner_generation = 7u, .frame = 10u, .capture_frame = 9u, .value0 = 256u, .value1 = 1536u, .identity_tokens = {1u, 2u, 0u, 0u}, .flags = 0x15u, .stage = DiagnosticStage::InputValidate, .reason = DiagnosticReason::NativeB0Mismatch, .operation = DiagnosticOperation::Validate};
    for (uint32_t i = 0u; i < 5000u; ++i) {
      auto event = repeated_failure;
      event.frame += i;
      event.fence = i;
      RecordDiagnostic(diagnostics, first_epoch, event);
    }
    auto snapshot = CopyDiagnosticSnapshot(diagnostics);
    if (snapshot.failure_size != 1u || snapshot.reason_size != 1u
        || snapshot.first_failures[0].count != 5000u || snapshot.first_failures[0].first.frame != 10u
        || snapshot.first_failures[0].latest_frame != 5009u || snapshot.reason_counts[0].first_frame != 10u
        || snapshot.reason_counts[0].latest_frame != 5009u || snapshot.transition_size != 1u) return EXIT_FAILURE;
    for (uint64_t i = 0u; i < 100u; ++i) {
      auto event = repeated_failure;
      event.reason = DiagnosticReason::None;
      event.value0 = i;
      RecordDiagnostic(diagnostics, first_epoch, event);
    }
    snapshot = CopyDiagnosticSnapshot(diagnostics);
    if (snapshot.transition_size != kDiagnosticTransitionCapacity || snapshot.transition_overwritten != 37u) {
      return EXIT_FAILURE;
    }
    if (DiagnosticIdentityToken(diagnostics, first_epoch, 0x1234u, 0u) == 0u
        || DiagnosticIdentityToken(diagnostics, first_epoch, 0x1234u, 1u) == 0u) return EXIT_FAILURE;
    for (uint64_t i = 1u; i < kDiagnosticIdentityCapacity - 1u; ++i) {
      if (DiagnosticIdentityToken(diagnostics, first_epoch, 0x10000u + i, 0u) == 0u) return EXIT_FAILURE;
    }
    if (DiagnosticIdentityToken(diagnostics, first_epoch, 0xfeedu, 0u) != 0u) return EXIT_FAILURE;
    snapshot = CopyDiagnosticSnapshot(diagnostics);
    if (snapshot.identity_size != kDiagnosticIdentityCapacity || snapshot.untracked_identities != 1u) return EXIT_FAILURE;
    snapshot.latest.debug_live_requested_view = RTXDI_DEBUG_VIEW_SUN_VISIBILITY;
    snapshot.latest.debug_live_effective_view = RTXDI_DEBUG_VIEW_OFF;
    snapshot.latest.debug_live_frame = 40u;
    snapshot.latest.debug_freshness = static_cast<uint32_t>(DiagnosticDebugFreshness::Matched);
    snapshot.latest.debug_observed_requested_view = RTXDI_DEBUG_VIEW_SUN_VISIBILITY;
    snapshot.latest.debug_observed_effective_view = RTXDI_DEBUG_VIEW_OFF;
    snapshot.latest.debug_observed_frame = 40u;
    snapshot.latest.debug_rejection_reason = 8u;
    snapshot.latest.debug_superseded_by_bvh = 1u;
    const std::string report = SerializeDiagnosticReport(snapshot);
    if (report.size() > kDiagnosticReportCapacity || report.find("0x1234") != std::string::npos
        || report.find("0xfeed") != std::string::npos || report.find("matching_draw_callbacks=") == std::string::npos
        || report.find("14:NativeB0Mismatch") == std::string::npos
        || report.find("12:SignalC") == std::string::npos
        || report.find("n1_config=unavailable") == std::string::npos
        || report.find("debug live_requested=9 live_effective=0 live_frame=40 freshness=2 render_observed_requested=9 render_observed_effective=0 render_observed_frame=40")
               == std::string::npos
        || report.find("reason=8") == std::string::npos || report.find("bvh_supersedes=1") == std::string::npos
        || report.find("cpu_lights source=") == std::string::npos
        || report.find("gpu_sample unavailable") == std::string::npos) {
      return EXIT_FAILURE;
    }
    snapshot.gpu_samples = 1u;
    snapshot.gpu_sample_frame = 40u;
    snapshot.gpu_counters[RTXDI_DIAG_SURFACE_PIXELS] = 4u;
    snapshot.gpu_counters[RTXDI_DIAG_TUPLE_STATUS_BASE] = 4u;
    const std::string gpu_report = SerializeDiagnosticReport(snapshot);
    if (gpu_report.find("gpu_tuple empty=4 visible=0") == std::string::npos
        || gpu_report.find(" tuples=1") == std::string::npos
        || gpu_report.find("gpu_blocked near_lamp=0 elsewhere=0") == std::string::npos
        || gpu_report.find("radiance_zero=0") == std::string::npos
        || gpu_report.find("gpu_sample unavailable") != std::string::npos) {
      return EXIT_FAILURE;
    }
    SetDiagnosticCapture(diagnostics, false);
    if (CopyDiagnosticSnapshot(diagnostics).transition_size != kDiagnosticTransitionCapacity) return EXIT_FAILURE;
    RecordDiagnostic(diagnostics, first_epoch, repeated_failure);
    if (CopyDiagnosticSnapshot(diagnostics).first_failures[0].count != 5000u) return EXIT_FAILURE;
    SetDiagnosticCapture(diagnostics, true);
    const uint64_t next_epoch = DiagnosticCaptureEpoch(diagnostics);
    RecordDiagnostic(diagnostics, first_epoch, repeated_failure);
    if (CopyDiagnosticSnapshot(diagnostics).transition_size != 0u) return EXIT_FAILURE;
    RecordDiagnostic(diagnostics, next_epoch, repeated_failure);
    snapshot = CopyDiagnosticSnapshot(diagnostics);
    if (snapshot.capture_epoch != next_epoch || snapshot.first_failures[0].count != 1u) return EXIT_FAILURE;
    ClearDiagnosticCapture(diagnostics);
    snapshot = CopyDiagnosticSnapshot(diagnostics);
    if (snapshot.transition_size != 0u || snapshot.failure_size != 0u
        || snapshot.capture_epoch != DiagnosticCaptureEpoch(diagnostics)) return EXIT_FAILURE;
    const uint64_t overflow_epoch = snapshot.capture_epoch;
    for (uint32_t i = 0u; i < kDiagnosticFailureCapacity + 1u; ++i) {
      const auto stage = static_cast<DiagnosticStage>(1u + (i % 14u));
      const auto reason = i < 14u ? DiagnosticReason::MissingEntry : DiagnosticReason::MissingInput;
      RecordDiagnostic(diagnostics, overflow_epoch,
                       {.frame = i,
                        .stage = stage,
                        .reason = reason,
                        .operation = DiagnosticOperation::Validate});
    }
    snapshot = CopyDiagnosticSnapshot(diagnostics);
    if (snapshot.failure_size != kDiagnosticFailureCapacity || snapshot.failure_records_dropped != 1u
        || snapshot.first_failures[0].first.frame != 0u || snapshot.reason_size != 2u) return EXIT_FAILURE;
  }
  {
    using namespace falcom_world::rtxdi;
    auto diagnostics_owner = std::make_unique<DiagnosticState>();
    auto& diagnostics = *diagnostics_owner;
    SetDiagnosticCapture(diagnostics, true);
    const uint64_t epoch = DiagnosticCaptureEpoch(diagnostics);
    DiagnosticEvent event = {.frame = 42u,
                             .stage = DiagnosticStage::InputValidate,
                             .reason = DiagnosticReason::InvalidResource,
                             .operation = DiagnosticOperation::Validate,
                             .input_kind = DiagnosticInputKind::Depth,
                             .validation_code = DiagnosticValidationCode::InvalidTextureView};
    event.input_status = 3u;
    event.input_stage = 1u;
    RecordDiagnostic(diagnostics, epoch, event);
    event.input_kind = DiagnosticInputKind::Normal;
    RecordDiagnostic(diagnostics, epoch, event);
    event.input_kind = DiagnosticInputKind::Depth;
    event.validation_code = DiagnosticValidationCode::OptionalFallback;
    RecordDiagnostic(diagnostics, epoch, event);
    event.frame++;
    RecordDiagnostic(diagnostics, epoch, event);
    const auto snapshot = CopyDiagnosticSnapshot(diagnostics);
    if (snapshot.transition_size != 3u || snapshot.first_failures[0].count != 4u
        || snapshot.first_failures[0].first.input_kind != DiagnosticInputKind::Depth
        || snapshot.first_failures[0].first.validation_code != DiagnosticValidationCode::InvalidTextureView) {
      return EXIT_FAILURE;
    }
    const std::string report = SerializeDiagnosticReport(snapshot);
    if (report.find("input_status=3 input_stage=1") == std::string::npos
        || report.find("input_status=0:Off") == std::string::npos) return EXIT_FAILURE;
  }
  {
    using namespace falcom_world::rtxdi;
    auto diagnostics_owner = std::make_unique<DiagnosticState>();
    auto& diagnostics = *diagnostics_owner;
    SetDiagnosticCapture(diagnostics, true);
    const uint64_t epoch = DiagnosticCaptureEpoch(diagnostics);
    DiagnosticEvent event = {.frame = 88u,
                             .stage = DiagnosticStage::InputValidate,
                             .reason = DiagnosticReason::InvalidResource,
                             .operation = DiagnosticOperation::Validate,
                             .input_kind = DiagnosticInputKind::Lights,
                             .validation_code = DiagnosticValidationCode::InvalidStructuredView};
    event.identity_tokens[0] = DiagnosticIdentityToken(diagnostics, epoch, 0xfedcba9876543210u, 7u);
    event.normalized_size = uint64_t{112u} | (uint64_t{64u} << 32u);
    event.required_rows = 11u;
    event.native_offset_bytes = 1u;
    event.native_count_bytes = 42u;
    event.first_constant = 3u;
    event.constant_count = 2u;
    event.native_bind_flags = 8u;
    event.available_rows = 128u;
    event.flags = 0x80u | 47u
                  | (static_cast<uint32_t>(DiagnosticStructuredFailure::StructureStrideMismatch) << 16u);
    event.buffer_bytes = 1024u;
    event.value0 = 384u;
    event.value1 = 256u;
    RecordDiagnostic(diagnostics, epoch, event);
    event.flags = 0x80u | 47u | (static_cast<uint32_t>(DiagnosticStructuredFailure::ElementBoundsInvalid) << 16u);
    event.constant_count = 9u;
    event.value1 = 1152u;
    RecordDiagnostic(diagnostics, epoch, event);
    event.frame++;
    RecordDiagnostic(diagnostics, epoch, event);
    const auto snapshot = CopyDiagnosticSnapshot(diagnostics);
    const std::string report = SerializeDiagnosticReport(snapshot);
    if (snapshot.transition_size != 2u || snapshot.first_failures[0].count != 3u
        || report.find("structured failure=7 evidence=0x0000002f stride=112 source_register=t11 dimension=1 format=42 first=3 count=2")
               == std::string::npos
        || report.find("resource_token=1 bytes=1024 bind=0x00000008 misc=0x00000040 structure_stride=128 offset=384 length=256")
               == std::string::npos
        || report.find("fedcba9876543210") != std::string::npos) return EXIT_FAILURE;
  }
  {
    using namespace falcom_world::rtxdi;
    auto diagnostics_owner = std::make_unique<DiagnosticState>();
    auto& diagnostics = *diagnostics_owner;
    SetDiagnosticCapture(diagnostics, true);
    const uint64_t epoch = DiagnosticCaptureEpoch(diagnostics);
    for (const auto operation : {DiagnosticOperation::BindLocal, DiagnosticOperation::BindSun}) {
      RecordDiagnostic(diagnostics, epoch,
                       {.frame = 8u,
                        .flags = 0u,
                        .stage = DiagnosticStage::LightingBind,
                        .reason = DiagnosticReason::None,
                        .operation = operation});
    }
    auto snapshot = CopyDiagnosticSnapshot(diagnostics);
    if (snapshot.reason_size != 0u || snapshot.failure_size != 0u) return EXIT_FAILURE;
    RecordDiagnostic(diagnostics, epoch,
                     {.frame = 9u,
                      .flags = 4u,
                      .stage = DiagnosticStage::LightingBind,
                      .reason = DiagnosticReason::None,
                      .operation = DiagnosticOperation::BindLocal});
    RecordDiagnostic(diagnostics, epoch,
                     {.frame = 10u,
                      .flags = 4u,
                      .stage = DiagnosticStage::LightingBind,
                      .reason = DiagnosticReason::None,
                      .operation = DiagnosticOperation::BindSun});
    snapshot = CopyDiagnosticSnapshot(diagnostics);
    if (snapshot.reason_size != 0u || snapshot.failure_size != 0u) return EXIT_FAILURE;
    RecordDiagnostic(diagnostics, epoch,
                     {.frame = 11u,
                      .flags = 4u,
                      .stage = DiagnosticStage::LightingBind,
                      .reason = DiagnosticReason::MissingInput,
                      .operation = DiagnosticOperation::BindLocal});
    RecordDiagnostic(diagnostics, epoch,
                     {.frame = 12u,
                      .flags = 4u,
                      .stage = DiagnosticStage::LightingBind,
                      .reason = DiagnosticReason::MissingInput,
                      .operation = DiagnosticOperation::BindSun});
    snapshot = CopyDiagnosticSnapshot(diagnostics);
    if (snapshot.reason_size != 1u || snapshot.failure_size != 1u
        || snapshot.first_failures[0].count != 2u || snapshot.first_failures[0].first.flags != 4u) {
      return EXIT_FAILURE;
    }
  }
  {
    using namespace falcom_world::rtxdi;
    auto diagnostics_owner = std::make_unique<DiagnosticState>();
    auto& diagnostics = *diagnostics_owner;
    SetDiagnosticCapture(diagnostics, true);
    const uint64_t epoch = DiagnosticCaptureEpoch(diagnostics);
    RecordDiagnostic(diagnostics, epoch,
                     {.frame = 1u,
                      .stage = DiagnosticStage::InputSubmit,
                      .reason = DiagnosticReason::SubmitFailed,
                      .operation = DiagnosticOperation::Execute});
    DiagnosticSummary current = {};
    current.frame = 2u;
    current.input_status = 9u;
    current.current_status_reports_failure = 0u;
    UpdateDiagnosticSummary(diagnostics, epoch, current);
    const auto snapshot = CopyDiagnosticSnapshot(diagnostics);
    const std::string report = SerializeDiagnosticReport(snapshot);
    if (snapshot.failure_size != 1u || snapshot.first_failures[0].count != 1u
        || report.find("current_status_reports_failure=0") == std::string::npos
        || report.find("false does not clear failure history") == std::string::npos) return EXIT_FAILURE;
  }
  {
    using namespace falcom_world::rtxdi;
    DiagnosticSummary nrd = {};
    nrd.available_mask = DiagnosticNrdStatusAvailable;
    nrd.nrd_compiled = 1u;
    nrd.nrd_status = static_cast<uint32_t>(DiagnosticNrdBackend::Failed);
    nrd.nrd_failure_stage = 2u;
    nrd.nrd_policy = static_cast<uint32_t>(DiagnosticNrdPolicy::BackendUnavailable);
    nrd.nrd_output_staged = 0u;
    const auto report = SerializeDiagnosticReport(DiagnosticSnapshot{.latest = nrd});
    if (report.find("nrd_backend=3 nrd_failure_stage=2 nrd_policy=3 nrd_diffuse_staged=0") == std::string::npos
        || report.find("nrd_compiled=1") == std::string::npos
        || report.find("staged is private producer evidence, not per-pixel application") == std::string::npos) return EXIT_FAILURE;
    nrd.nrd_compiled = 0u;
    nrd.nrd_status = static_cast<uint32_t>(DiagnosticNrdBackend::NotCompiled);
    nrd.nrd_failure_stage = 0u;
    nrd.nrd_policy = static_cast<uint32_t>(DiagnosticNrdPolicy::NoSdk);
    const auto no_sdk_report = SerializeDiagnosticReport(DiagnosticSnapshot{.latest = nrd});
    if (no_sdk_report.find("nrd_backend=0 nrd_failure_stage=0 nrd_policy=2 nrd_diffuse_staged=0") == std::string::npos
        || no_sdk_report.find("nrd_compiled=0") == std::string::npos) return EXIT_FAILURE;
    nrd.nrd_compiled = 1u;
    nrd.nrd_status = static_cast<uint32_t>(DiagnosticNrdBackend::Ready);
    nrd.nrd_policy = static_cast<uint32_t>(DiagnosticNrdPolicy::RelaxSelected);
    nrd.nrd_output_staged = 1u;
    const auto relax_report = SerializeDiagnosticReport(DiagnosticSnapshot{.latest = nrd});
    if (relax_report.find("nrd_backend=2 nrd_failure_stage=0 nrd_policy=4 nrd_diffuse_staged=1") == std::string::npos
        || relax_report.find("raw local/sun lighting remains independent") == std::string::npos
        || relax_report.find("staged is private producer evidence") == std::string::npos) return EXIT_FAILURE;
    DiagnosticSummary gate = {};
    gate.nrd_gate.frame = 7u;
    for (uint32_t link = 0u; link < static_cast<uint32_t>(NrdGateLink::PreviousFrame); ++link) {
      gate.nrd_gate.status[link] = kNrdGatePass;
    }
    gate.nrd_gate.status[static_cast<uint32_t>(NrdGateLink::PreviousFrame)] = kNrdGateFail;
    gate.nrd_gate.reason[static_cast<uint32_t>(NrdGateLink::PreviousFrame)] = 2u;
    const auto gate_report = SerializeDiagnosticReport(DiagnosticSnapshot{.latest = gate});
    if (gate_report.find("first_fail=previous_frame reason=2") == std::string::npos) return EXIT_FAILURE;
    std::printf("PASS NRD report separates compiled/backend/policy/private staged output and no-SDK fallback\n");
  }
  {
    using namespace falcom_world::rtxdi;
    for (const uint32_t rows : {79u, 80u, 81u, 95u, 96u}) {
      SceneCopyWindow window = {};
      const bool accepted = MakeSceneCopyWindow(256u + 96u * 16u, 256u, UINT64_MAX, 16u,
                                                rows, &window);
      if (accepted != (rows >= RTXDI_INPUT_SCENE_REQUIRED_ROWS) || window.available_rows != rows
          || window.copied_rows != rows || window.normalized_bytes != 1536u
          || window.available_bytes != uint64_t{rows} * 16u) return EXIT_FAILURE;
    }
    SceneCopyWindow window = {};
    if (!MakeSceneCopyWindow(512u + 128u * 16u, 512u, 1296u, 32u, 100u, &window)
        || window.normalized_bytes != 1296u || window.available_bytes != 1296u
        || window.available_rows != 81u || window.copied_rows != 81u
        || !MakeSceneCopyWindow(256u + 200u * 16u, 256u, UINT64_MAX, 16u, 128u, &window)
        || window.normalized_bytes != 3200u || window.available_rows != 128u || window.copied_rows != 96u
        || MakeSceneCopyWindow(256u + 96u * 16u, 256u, 1281u, 16u, 96u, &window)
        || MakeSceneCopyWindow(256u + 96u * 16u, 256u, UINT64_MAX, 15u, 96u, &window)
        || MakeSceneCopyWindow(256u + 96u * 16u, 256u, UINT64_MAX, 16u, 79u, &window)) return EXIT_FAILURE;
    std::printf("PASS RTXDI scene prefix rows 79/80/81/95/96, sentinel, offset, cap, malformed range\n");
  }
  {
    using namespace falcom_world::rtxdi;
    const SceneRangeSnapshot promoted = {.buffer = 11u, .offset = 256u, .size = 1536u, .frame = 5u, .normalized_size = 1536u, .command = 7u, .live = true, .known = true, .sequence = 8u, .reset_generation = 3u};
    SceneBindingEvidence evidence;
    if (CheckPromotedSceneBinding(promoted, promoted, 5u, 7u, 4096u, &evidence) != SceneBindingStatus::Ready
        || evidence.reason != DiagnosticReason::None || !evidence.normalized_valid
        || evidence.normalized_size != 1536u) return EXIT_FAILURE;
    auto changed = promoted;
    changed.known = false;
    if (CheckPromotedSceneBinding(promoted, changed, 5u, 7u, 4096u, &evidence) != SceneBindingStatus::SceneBindingChanged
        || evidence.reason != DiagnosticReason::UnknownBinding) return EXIT_FAILURE;
    changed = promoted;
    changed.live = false;
    if (CheckPromotedSceneBinding(promoted, changed, 5u, 7u, 4096u, &evidence) != SceneBindingStatus::SceneBindingChanged
        || evidence.reason != DiagnosticReason::NonliveBinding) return EXIT_FAILURE;
    changed = promoted;
    changed.frame--;
    if (CheckPromotedSceneBinding(promoted, changed, 5u, 7u, 4096u, &evidence) != SceneBindingStatus::SceneBindingChanged
        || evidence.reason != DiagnosticReason::StaleFrame) return EXIT_FAILURE;
    changed = promoted;
    changed.command++;
    if (CheckPromotedSceneBinding(promoted, changed, 5u, 7u, 4096u, &evidence) != SceneBindingStatus::SceneBindingChanged
        || evidence.reason != DiagnosticReason::CommandMismatch) return EXIT_FAILURE;
    changed = promoted;
    changed.sequence++;
    if (CheckPromotedSceneBinding(promoted, changed, 5u, 7u, 4096u, &evidence) != SceneBindingStatus::SceneBindingChanged
        || evidence.reason != DiagnosticReason::SequenceMismatch) return EXIT_FAILURE;
    changed = promoted;
    changed.reset_generation++;
    if (CheckPromotedSceneBinding(promoted, changed, 5u, 7u, 4096u, &evidence) != SceneBindingStatus::SceneBindingChanged
        || evidence.reason != DiagnosticReason::ResetMismatch) return EXIT_FAILURE;
    changed = promoted;
    changed.buffer++;
    if (CheckPromotedSceneBinding(promoted, changed, 5u, 7u, 4096u, &evidence) != SceneBindingStatus::SceneBindingChanged
        || evidence.reason != DiagnosticReason::BufferMismatch) return EXIT_FAILURE;
    changed = promoted;
    changed.offset += 256u;
    if (CheckPromotedSceneBinding(promoted, changed, 5u, 7u, 4096u, &evidence) != SceneBindingStatus::SceneBindingChanged
        || evidence.reason != DiagnosticReason::OffsetMismatch) return EXIT_FAILURE;
    changed = promoted;
    changed.size += 256u;
    if (CheckPromotedSceneBinding(promoted, changed, 5u, 7u, 4096u, &evidence) != SceneBindingStatus::SceneBindingChanged
        || evidence.reason != DiagnosticReason::NormalizedSizeMismatch) return EXIT_FAILURE;
    changed = promoted;
    changed.offset = 1u;
    auto malformed_promoted = promoted;
    malformed_promoted.offset = 1u;
    if (CheckPromotedSceneBinding(malformed_promoted, changed, 5u, 7u, 4096u, &evidence) != SceneBindingStatus::SceneBindingChanged
        || evidence.reason != DiagnosticReason::InvalidRange || evidence.normalized_valid) return EXIT_FAILURE;
    std::printf("PASS RTXDI promoted native scene range identity and recheck\n");
  }
  using falcom_world::rtxdi::CanReuseDiOutput;
  using falcom_world::rtxdi::CheckDiOutputIdentity;
  using falcom_world::rtxdi::CheckDiSubmissionIdentity;
  using falcom_world::rtxdi::DiOutputGate;
  using falcom_world::rtxdi::DiOutputIdentity;
  using falcom_world::rtxdi::DiPostSubmitSignalDisposition;
  using falcom_world::rtxdi::DiSignalDisposition;
  using falcom_world::rtxdi::DiSubmissionGate;
  using falcom_world::rtxdi::DiSubmissionIdentity;
  using falcom_world::rtxdi::DiTileContainsLight;
  using falcom_world::rtxdi::ValidDiResolvedMembership;
  using falcom_world::rtxdi::ValidDiResolvedTuple;
  const DiSubmissionIdentity di_identity = {.requested_frame = 8u,
                                            .transfer_frame = 8u,
                                            .requested_generation = 3u,
                                            .owner_generation = 3u,
                                            .requested_scene_slot = 1,
                                            .transfer_scene_slot = 1,
                                            .requested_transfer_slot = 2,
                                            .transfer_slot = 2,
                                            .requested_fence = 17u,
                                            .transfer_fence = 17u,
                                            .requested_width = 13u,
                                            .requested_height = 9u,
                                            .config_width = 13u,
                                            .config_height = 9u,
                                            .local_requested = true,
                                            .input_ready = true,
                                            .transfer_available = true,
                                            .reservoir_available = true};
  auto stale_di_identity = di_identity;
  stale_di_identity.transfer_frame++;
  auto stale_di_scene = di_identity;
  stale_di_scene.transfer_scene_slot++;
  auto stale_di_transfer = di_identity;
  stale_di_transfer.transfer_slot++;
  auto stale_di_fence = di_identity;
  stale_di_fence.transfer_fence++;
  auto resize_di_config = di_identity;
  resize_di_config.config_width++;
  auto config_di_mismatch = di_identity;
  config_di_mismatch.config_height++;
  auto busy_transfer = di_identity;
  busy_transfer.transfer_available = false;
  auto busy_reservoir = di_identity;
  busy_reservoir.reservoir_available = false;
  auto waiting_di_input = di_identity;
  waiting_di_input.input_ready = false;
  if (CheckDiSubmissionIdentity(di_identity) != DiSubmissionGate::Ready
      || CheckDiSubmissionIdentity(stale_di_identity) != DiSubmissionGate::StaleIdentity
      || CheckDiSubmissionIdentity(stale_di_scene) != DiSubmissionGate::StaleIdentity
      || CheckDiSubmissionIdentity(stale_di_transfer) != DiSubmissionGate::StaleIdentity
      || CheckDiSubmissionIdentity(stale_di_fence) != DiSubmissionGate::StaleIdentity
      || CheckDiSubmissionIdentity(resize_di_config) != DiSubmissionGate::ConfigurationMismatch
      || CheckDiSubmissionIdentity(config_di_mismatch) != DiSubmissionGate::ConfigurationMismatch
      || CheckDiSubmissionIdentity(busy_transfer) != DiSubmissionGate::Busy
      || CheckDiSubmissionIdentity(busy_reservoir) != DiSubmissionGate::Busy
      || CheckDiSubmissionIdentity(waiting_di_input) != DiSubmissionGate::WaitingInput
      || DiPostSubmitSignalDisposition(true, true) != DiSignalDisposition::KeepSession
      || DiPostSubmitSignalDisposition(false, false) != DiSignalDisposition::KeepSession
      || DiPostSubmitSignalDisposition(true, false) != DiSignalDisposition::QuarantineSession) return EXIT_FAILURE;
  const DiOutputIdentity staged_output = {.requested_frame = 41u,
                                          .output_frame = 41u,
                                          .requested_owner_generation = 7u,
                                          .output_owner_generation = 7u,
                                          .requested_configuration_generation = 3u,
                                          .output_configuration_generation = 3u,
                                          .requested_width = 13u,
                                          .requested_height = 9u,
                                          .output_width = 13u,
                                          .output_height = 9u,
                                          .requested_scene_slot = 2,
                                          .output_scene_slot = 2,
                                          .requested_transfer_slot = 1,
                                          .output_transfer_slot = 1,
                                          .producer_fence_b = 22u,
                                          .staged = true,
                                          .producer_wait_accepted = true};
  auto before_output_b = staged_output;
  before_output_b.producer_wait_accepted = false;
  auto stale_output = staged_output;
  stale_output.output_owner_generation++;
  auto stale_output_frame = staged_output;
  stale_output_frame.output_frame++;
  auto stale_output_scene = staged_output;
  stale_output_scene.output_scene_slot++;
  auto stale_output_transfer = staged_output;
  stale_output_transfer.output_transfer_slot++;
  auto resize_output = staged_output;
  resize_output.output_width++;
  auto configuration_output = staged_output;
  configuration_output.output_configuration_generation++;
  auto not_staged_output = staged_output;
  not_staged_output.staged = false;
  const DiOutputIdentity sun_only_staged = staged_output;
  const DiOutputIdentity local_and_sun_staged = staged_output;
  const bool sun_only_b_before_read = CheckDiOutputIdentity(sun_only_staged) == DiOutputGate::Ready;
  const bool local_and_sun_b_before_read = CheckDiOutputIdentity(local_and_sun_staged) == DiOutputGate::Ready;
  if (CheckDiOutputIdentity(staged_output) != DiOutputGate::Ready
      || CheckDiOutputIdentity(before_output_b) != DiOutputGate::ProducerPending
      || CheckDiOutputIdentity(stale_output) != DiOutputGate::StaleIdentity
      || CheckDiOutputIdentity(stale_output_frame) != DiOutputGate::StaleIdentity
      || CheckDiOutputIdentity(stale_output_scene) != DiOutputGate::StaleIdentity
      || CheckDiOutputIdentity(stale_output_transfer) != DiOutputGate::StaleIdentity
      || CheckDiOutputIdentity(resize_output) != DiOutputGate::ConfigurationMismatch
      || CheckDiOutputIdentity(configuration_output) != DiOutputGate::ConfigurationMismatch
      || CheckDiOutputIdentity(not_staged_output) != DiOutputGate::NotStaged
      || !sun_only_b_before_read || !local_and_sun_b_before_read
      || CanReuseDiOutput(true, 0u, 100u) || CanReuseDiOutput(true, 27u, 26u)
      || CanReuseDiOutput(true, 27u, std::numeric_limits<uint64_t>::max())
      || !CanReuseDiOutput(true, 27u, 27u) || !CanReuseDiOutput(false, 0u, 0u)) return EXIT_FAILURE;
  if (CanReuseDiOutput(true, sun_only_staged.producer_fence_b, sun_only_staged.producer_fence_b - 1u)
      || CanReuseDiOutput(true, local_and_sun_staged.producer_fence_b, local_and_sun_staged.producer_fence_b - 1u)
      || !CanReuseDiOutput(true, sun_only_staged.producer_fence_b, sun_only_staged.producer_fence_b)
      || !CanReuseDiOutput(true, local_and_sun_staged.producer_fence_b, local_and_sun_staged.producer_fence_b)) {
    return EXIT_FAILURE;
  }

  bool output_bound = true;
  bool lighting_draw_seen = false;
  uint32_t bound_frame = 41u;
  int32_t bound_slot = 1;
  uint64_t bound_fence = 22u;
  if (!output_bound || lighting_draw_seen) return EXIT_FAILURE;
  falcom_world::rtxdi::RecordOutputLightingDrawSeen(&output_bound, &bound_frame, &bound_slot, &bound_fence,
                                                    &lighting_draw_seen, 41u, 41u, 1, 22u);
  if (!output_bound || !lighting_draw_seen) return EXIT_FAILURE;
  // Callback evidence is independent of per-pixel tuple acceptance, including character-only or all-invalid draws.
  bool character_only_bound = true, character_only_seen = false;
  uint32_t character_only_frame = 41u;
  int32_t character_only_slot = 1;
  uint64_t character_only_fence = 22u;
  bool all_invalid_bound = true, all_invalid_seen = false;
  uint32_t all_invalid_frame = 41u;
  int32_t all_invalid_slot = 1;
  uint64_t all_invalid_fence = 22u;
  bool stale_frame_bound = true, stale_frame_seen = false;
  uint32_t stale_frame_id = 41u;
  int32_t stale_frame_slot = 1;
  uint64_t stale_frame_fence = 22u;
  bool stale_slot_bound = true, stale_slot_seen = false;
  uint32_t stale_slot_frame = 41u;
  int32_t stale_slot_id = 1;
  uint64_t stale_slot_fence = 22u;
  falcom_world::rtxdi::RecordOutputLightingDrawSeen(&character_only_bound, &character_only_frame, &character_only_slot,
                                                    &character_only_fence, &character_only_seen, 41u, 41u, 1, 22u);
  falcom_world::rtxdi::RecordOutputLightingDrawSeen(&all_invalid_bound, &all_invalid_frame, &all_invalid_slot,
                                                    &all_invalid_fence, &all_invalid_seen, 41u, 41u, 1, 22u);
  falcom_world::rtxdi::RecordOutputLightingDrawSeen(&stale_frame_bound, &stale_frame_id, &stale_frame_slot,
                                                    &stale_frame_fence, &stale_frame_seen, 42u, 41u, 1, 22u);
  falcom_world::rtxdi::RecordOutputLightingDrawSeen(&stale_slot_bound, &stale_slot_frame, &stale_slot_id,
                                                    &stale_slot_fence, &stale_slot_seen, 41u, 41u, 2, 22u);
  if (!character_only_bound || !character_only_seen || !all_invalid_bound || !all_invalid_seen || stale_frame_bound
      || stale_frame_seen || stale_slot_bound || stale_slot_seen) return EXIT_FAILURE;
  bool sun_only_bound = true, sun_only_draw_seen = false;
  uint32_t sun_only_bound_frame = 41u;
  int32_t sun_only_bound_slot = 1;
  uint64_t sun_only_bound_fence = 22u;
  falcom_world::rtxdi::RecordOutputLightingDrawSeen(&sun_only_bound, &sun_only_bound_frame, &sun_only_bound_slot,
                                                    &sun_only_bound_fence, &sun_only_draw_seen, 41u, 41u, 1, 22u);
  if (!sun_only_bound || !sun_only_draw_seen
      || !CanReuseDiOutput(true, sun_only_bound_fence, sun_only_bound_fence)) return EXIT_FAILURE;

  const std::array<int32_t, 64> di_point_indices = [] {
    std::array<int32_t, 64> indices = {};
    indices.fill(-1);
    indices[0] = 7;
    indices[62] = 19;
    indices[63] = 23;
    return indices;
  }();
  const std::array<int32_t, 2> di_spot_indices = {11, 19};
  if (!ValidDiResolvedTuple(RTXDI_DI_STATUS_VISIBLE, 1u, 7u, 24u, 0.5f)
      || !ValidDiResolvedTuple(RTXDI_DI_STATUS_BLOCKED, 2u, 11u, 24u, 1.f)
      || ValidDiResolvedTuple(RTXDI_DI_STATUS_UNKNOWN, 1u, 7u, 24u, 0.5f)
      || ValidDiResolvedTuple(RTXDI_DI_STATUS_VISIBLE, 3u, 7u, 24u, 0.5f)
      || ValidDiResolvedTuple(RTXDI_DI_STATUS_VISIBLE, 1u, 24u, 24u, 0.5f)
      || ValidDiResolvedTuple(RTXDI_DI_STATUS_VISIBLE, 1u, 7u, 24u, 0.f)
      || ValidDiResolvedTuple(RTXDI_DI_STATUS_VISIBLE, 1u, 7u, 24u, std::numeric_limits<float>::infinity())
      || !ValidDiResolvedMembership(1u, 7u, di_point_indices.data(), 63u, di_spot_indices.data(), 2u)
      || !DiTileContainsLight(di_point_indices.data(), 64u, 19u)
      || DiTileContainsLight(di_point_indices.data(), 63u, 23u)
      || ValidDiResolvedMembership(1u, 19u, di_point_indices.data(), 64u, di_spot_indices.data(), 2u)
      || !ValidDiResolvedMembership(2u, 11u, di_point_indices.data(), 64u, di_spot_indices.data(), 2u)) return EXIT_FAILURE;

  using falcom_world::AlphaUvRangeStatus;
  using falcom_world::rtxdi::CanReuseScene;
  using falcom_world::rtxdi::IsDuplicate;
  using falcom_world::rtxdi::MatchesStagedSunIdentity;
  using falcom_world::rtxdi::MatchesStagedSunSlotIdentity;
  using falcom_world::rtxdi::SceneReuseInputs;
  using falcom_world::rtxdi::SceneSnapshot;
  using falcom_world::rtxdi::SunAttemptAction;
  using falcom_world::rtxdi::SunSnapshot;
  using falcom_world::rtxdi::SunStatus;
  using falcom_world::rtxdi::SunVisibilityStatus;
  using falcom_world::rtxdi::TransitionSunAttempt;
  uint32_t alpha_uv_base = 0xA5A5A5A5u;
  const auto validate_alpha_uv = falcom_world::ValidateAlphaUvRange;
  if (falcom_world::ValidAlphaAtlasLayout(falcom_world::kAlphaSliceWords, falcom_world::kAlphaSliceWords,
                                          falcom_world::kAlphaAtlasSlices)
          == false
      || falcom_world::ValidAlphaAtlasLayout(falcom_world::kAlphaSliceTexelsX, falcom_world::kAlphaSliceTexelsY,
                                             falcom_world::kAlphaAtlasSlices)
      || falcom_world::ValidAlphaAtlasLayout(falcom_world::kAlphaSliceWords, falcom_world::kAlphaSliceWords,
                                             falcom_world::kAlphaAtlasSlices - 1u)) return EXIT_FAILURE;
  if (!falcom_world::ValidAlphaAtlasSlice(falcom_world::kAlphaAtlasSlices - 1u, falcom_world::kAlphaAtlasSlices)
      || falcom_world::ValidAlphaAtlasSlice(falcom_world::kAlphaAtlasSlices, falcom_world::kAlphaAtlasSlices)
      || falcom_world::ValidAlphaAtlasSlice(0u, falcom_world::kAlphaAtlasSlices - 1u)) return EXIT_FAILURE;
  if (validate_alpha_uv(0u, false, std::numeric_limits<float>::quiet_NaN(), 0u, UINT32_MAX, UINT32_MAX, UINT32_MAX,
                        &alpha_uv_base)
          != AlphaUvRangeStatus::NotRequired
      || alpha_uv_base != 0xA5A5A5A5u
      || validate_alpha_uv(1u, true, std::numeric_limits<float>::infinity(), 0u, UINT32_MAX, UINT32_MAX, UINT32_MAX,
                           &alpha_uv_base)
             != AlphaUvRangeStatus::NotRequired) return EXIT_FAILURE;
  for (const float invalid_uv_slot : {std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::infinity(),
                                      -1.f, 0.f, 1.5f, 4294967296.f}) {
    if (validate_alpha_uv(1u, false, invalid_uv_slot, UINT32_MAX, 0u, 0u, 0u, &alpha_uv_base)
        != AlphaUvRangeStatus::Invalid) return EXIT_FAILURE;
  }
  if (validate_alpha_uv(1u, false, 1.f, 1u, 0u, 0u, 0u, &alpha_uv_base) != AlphaUvRangeStatus::Valid
      || alpha_uv_base != 0u
      || validate_alpha_uv(1u, false, 4.f, 4u, 0u, 0u, 0u, &alpha_uv_base) != AlphaUvRangeStatus::Valid
      || alpha_uv_base != 3u
      || validate_alpha_uv(1u, false, 5.f, 4u, 0u, 0u, 0u, &alpha_uv_base) != AlphaUvRangeStatus::Invalid
      || validate_alpha_uv(1u, false, 1.f, UINT32_MAX, UINT32_MAX, 0u, 0u, &alpha_uv_base)
             != AlphaUvRangeStatus::Invalid
      || validate_alpha_uv(1u, false, 4294967040.f, UINT32_MAX, 255u, 0u, 0u, &alpha_uv_base)
             != AlphaUvRangeStatus::Valid
      || alpha_uv_base != 4294967039u
      || validate_alpha_uv(1u, false, 4294967040.f, UINT32_MAX, 256u, 0u, 0u, &alpha_uv_base)
             != AlphaUvRangeStatus::Invalid) return EXIT_FAILURE;
  const auto alpha_word_and_byte = [](uint32_t logical_x) {
    return std::array<uint32_t, 2>{logical_x >> 2u, logical_x & 3u};
  };
  if (alpha_word_and_byte(0u) != std::array<uint32_t, 2>{0u, 0u}
      || alpha_word_and_byte(3u) != std::array<uint32_t, 2>{0u, 3u}
      || alpha_word_and_byte(4u) != std::array<uint32_t, 2>{1u, 0u}
      || alpha_word_and_byte(1023u) != std::array<uint32_t, 2>{255u, 3u}) return EXIT_FAILURE;
  const auto alpha_is_cut = [](uint8_t alpha, float threshold) {
    return static_cast<float>(alpha) * (1.f / 255.f) - threshold < 0.f;
  };
  if (!alpha_is_cut(0u, 0.5f) || alpha_is_cut(255u, 0.5f) || alpha_is_cut(0u, 0.f)
      || alpha_is_cut(255u, 1.f)) return EXIT_FAILURE;
  SceneReuseInputs scene = {.healthy = true, .slot_valid = true, .tlas_valid = true, .frame = 4u, .scene_frame = 4u, .records = 2u, .scene_records = 2u, .characters = 1u, .scene_characters = 1u, .generation = 3u, .scene_generation = 3u, .snapshot = 5u, .scene_snapshot = 5u};
  if (!CanReuseScene(scene)) return EXIT_FAILURE;
  scene.slot_valid = false;
  if (CanReuseScene(scene)) return EXIT_FAILURE;
  scene.slot_valid = true;
  scene.gc = true;
  if (CanReuseScene(scene)) return EXIT_FAILURE;
  scene.scene_gc = true;
  scene.gc_version = 7u;
  scene.scene_gc_version = 7u;
  if (!CanReuseScene(scene)) return EXIT_FAILURE;
  scene.noise_pending = true;
  if (CanReuseScene(scene)) return EXIT_FAILURE;
  scene.noise_pending = false;
  scene.scene_gc_version = 8u;
  if (CanReuseScene(scene)) return EXIT_FAILURE;
  const SceneSnapshot attempted = {.attempted_frame = 4u, .attempted_generation = 3u};
  if (!IsDuplicate(attempted, 4u, 3u) || IsDuplicate(attempted, 4u, 4u) || IsDuplicate(attempted, 5u, 3u)) return EXIT_FAILURE;
  const SceneSnapshot initial = {};
  if (falcom_world::rtxdi::PreserveAttemptForNonImmediate(initial, 4u, true, true)) return EXIT_FAILURE;
  if (!falcom_world::rtxdi::PreserveAttemptForNonImmediate(attempted, 4u, true, true)
      || !falcom_world::rtxdi::PreserveAttemptForNonImmediate(attempted, 4u, false, false)
      || falcom_world::rtxdi::PreserveAttemptForNonImmediate(attempted, 5u, true, true)) return EXIT_FAILURE;
  SceneSnapshot ready = {.status = falcom_world::rtxdi::SceneStatus::Submitted, .request_mask = 3u, .attempted_frame = 4u, .attempted_generation = 3u, .prepared_frame = 4u, .prepared_slot = 2, .prepared_fence = 9u, .prepared_generation = 3u, .attempts = 1u, .submissions = 1u};
  falcom_world::rtxdi::RefreshOffState(&ready, 4u);
  if (ready.status != falcom_world::rtxdi::SceneStatus::Submitted || ready.request_mask != 3u
      || ready.prepared_frame != 4u || ready.prepared_slot != 2 || ready.prepared_fence != 9u
      || ready.prepared_generation != 3u || !IsDuplicate(ready, 4u, 3u)) return EXIT_FAILURE;
  SceneSnapshot reused = ready;
  reused.status = falcom_world::rtxdi::SceneStatus::Reused;
  falcom_world::rtxdi::RefreshOffState(&reused, 4u);
  if (reused.status != falcom_world::rtxdi::SceneStatus::Reused || reused.request_mask != 3u
      || reused.prepared_slot != 2 || reused.prepared_fence != 9u) return EXIT_FAILURE;
  falcom_world::rtxdi::RefreshOffState(&ready, 5u);
  if (ready.status != falcom_world::rtxdi::SceneStatus::Off || ready.request_mask != 0u
      || ready.prepared_frame != UINT32_MAX || ready.prepared_slot != -1 || ready.prepared_fence != 0u
      || ready.prepared_generation != 0u || ready.attempted_frame != UINT32_MAX
      || ready.attempts != 1u || ready.submissions != 1u) return EXIT_FAILURE;
  SceneSnapshot failed = {.status = falcom_world::rtxdi::SceneStatus::Busy, .request_mask = 1u, .attempted_frame = 4u, .attempted_generation = 3u, .attempts = 1u, .skips = 1u};
  falcom_world::rtxdi::RefreshOffState(&failed, 4u);
  if (failed.status != falcom_world::rtxdi::SceneStatus::Off || failed.request_mask != 0u
      || !IsDuplicate(failed, 4u, 3u) || failed.attempts != 1u || failed.skips != 1u) return EXIT_FAILURE;
  SunSnapshot sun_staged = {.status = SunStatus::Staged,
                            .frame = 4u,
                            .width = 1280u,
                            .height = 720u,
                            .owner_generation = 3u,
                            .configuration_generation = 7u,
                            .fence_b = 9u,
                            .scene_slot = 1,
                            .transfer_slot = 2};
  if (!MatchesStagedSunIdentity(sun_staged, 4u, 3u, 7u, 0u)
      || MatchesStagedSunIdentity(sun_staged, 5u, 3u, 7u, 0u)
      || MatchesStagedSunIdentity(sun_staged, 4u, 4u, 7u, 0u)
      || MatchesStagedSunIdentity(sun_staged, 4u, 3u, 8u, 0u)
      || MatchesStagedSunIdentity(sun_staged, 4u, 3u, 7u, 1u)) return EXIT_FAILURE;
  if (!MatchesStagedSunSlotIdentity(sun_staged, 4u, 3u, 7u, 0u, 7u)
      || MatchesStagedSunSlotIdentity(sun_staged, 4u, 3u, 8u, 0u, 7u)
      || MatchesStagedSunSlotIdentity(sun_staged, 4u, 3u, 7u, 1u, 7u)
      || MatchesStagedSunSlotIdentity(sun_staged, 4u, 3u, 7u, 0u, 8u)) return EXIT_FAILURE;
  std::printf("PASS sun consumer identity rejects transfer config, DI config, and slot config mismatches\n");
  TransitionSunAttempt(&sun_staged, 4u, SunAttemptAction::PreserveDuplicate);
  if (sun_staged.status != SunStatus::Staged || sun_staged.frame != 4u || sun_staged.fence_b != 9u
      || sun_staged.transfer_slot != 2) return EXIT_FAILURE;
  TransitionSunAttempt(&sun_staged, 5u, SunAttemptAction::Invalidate);
  if (sun_staged.status != SunStatus::WaitingInput || sun_staged.frame != 5u || sun_staged.fence_b != 0u
      || sun_staged.transfer_slot != -1) return EXIT_FAILURE;
  sun_staged = {.status = SunStatus::Staged, .frame = 4u, .owner_generation = 3u, .configuration_generation = 7u, .fence_b = 9u, .transfer_slot = 2};
  TransitionSunAttempt(&sun_staged, 4u, SunAttemptAction::Invalidate);
  if (sun_staged.status != SunStatus::WaitingInput || sun_staged.fence_b != 0u) return EXIT_FAILURE;
  sun_staged = {.status = SunStatus::Staged, .frame = 4u, .owner_generation = 3u, .configuration_generation = 7u, .fence_b = 9u, .transfer_slot = 2};
  TransitionSunAttempt(&sun_staged, 4u, SunAttemptAction::Off);
  TransitionSunAttempt(&sun_staged, 4u, SunAttemptAction::PreserveDuplicate);
  if (sun_staged.status != SunStatus::Off || sun_staged.frame != 4u || sun_staged.fence_b != 0u
      || sun_staged.transfer_slot != -1) return EXIT_FAILURE;
  sun_staged = {.status = SunStatus::Staged, .frame = 4u, .owner_generation = 3u, .configuration_generation = 7u, .fence_b = 9u, .transfer_slot = 2};
  TransitionSunAttempt(&sun_staged, 4u, SunAttemptAction::Invalidate);
  if (sun_staged.status == SunStatus::Staged || sun_staged.fence_b != 0u) return EXIT_FAILURE;
  falcom_world::rtxdi::DiConfig config;
  if (falcom_world::rtxdi::CalculateDiConfig(0u, 1u, &config)
      || falcom_world::rtxdi::CalculateDiConfig(UINT32_MAX, 1u, &config)
      || falcom_world::rtxdi::CalculateDiConfig(100000u, 100000u, &config)
      || !falcom_world::rtxdi::CalculateDiConfig(1280u, 720u, &config)
      || config.reservoir_bytes != 66355200u || config.array_pitch != 921600u
      || falcom_world::rtxdi::kNeighborBytes != 16384u) return EXIT_FAILURE;
  using falcom_world::rtxdi::DiResourceAction;
  using falcom_world::rtxdi::DiResourceIdentity;
  using falcom_world::rtxdi::SelectDiResourceAction;
  const auto balanced_settings = falcom_world::rtxdi::CaptureEffectiveRtxdiSettings();
  const DiResourceIdentity installed = {.config = config, .owner_generation = 4u, .configuration_generation = 7u, .settings_identity = balanced_settings.identity, .present = true, .complete = true};
  if (SelectDiResourceAction(false, 4u, {}, config, balanced_settings.identity) != DiResourceAction::Skip
      || SelectDiResourceAction(true, 4u, {}, config, balanced_settings.identity) != DiResourceAction::Create
      || SelectDiResourceAction(true, 4u, installed, config, balanced_settings.identity) != DiResourceAction::Reuse
      || SelectDiResourceAction(true, 4u, installed, config, balanced_settings.identity + 1u) != DiResourceAction::Create
      || SelectDiResourceAction(true, 5u, installed, config, balanced_settings.identity) != DiResourceAction::OwnerMismatch
      || SelectDiResourceAction(true, 0u, installed, config, balanced_settings.identity) != DiResourceAction::OwnerMismatch) return EXIT_FAILURE;
  auto resize_request = config;
  resize_request.width = 1920u;
  if (SelectDiResourceAction(true, 4u, installed, resize_request, balanced_settings.identity) != DiResourceAction::Create) return EXIT_FAILURE;
  auto incomplete = installed;
  incomplete.complete = false;
  if (SelectDiResourceAction(true, 4u, incomplete, config, balanced_settings.identity) != DiResourceAction::Create
      || SelectDiResourceAction(true, 5u, incomplete, config, balanced_settings.identity) != DiResourceAction::OwnerMismatch) return EXIT_FAILURE;
  auto uncommitted = installed;
  for (int i = 0; i < 2; ++i) {
    if (SelectDiResourceAction(true, 4u, uncommitted, resize_request, balanced_settings.identity) != DiResourceAction::Create
        || uncommitted.owner_generation != 4u || uncommitted.configuration_generation != 7u) return EXIT_FAILURE;
  }
  uncommitted.configuration_generation = 0u;
  if (SelectDiResourceAction(true, 4u, uncommitted, config, balanced_settings.identity) != DiResourceAction::Create) return EXIT_FAILURE;
  uint64_t next_generation = 0u;
  if (!falcom_world::rtxdi::NextDiConfigurationGeneration(installed.configuration_generation, &next_generation)
      || next_generation != 8u || installed.owner_generation != 4u || installed.configuration_generation != 7u
      || falcom_world::rtxdi::NextDiConfigurationGeneration(UINT64_MAX, &next_generation)) return EXIT_FAILURE;
  auto context = falcom_world::rtxdi::CreateDiContext(config, balanced_settings);
  if (!context || context->GetStaticParameters().CheckerboardSamplingMode != rtxdi::CheckerboardMode::Off
      || context->GetResamplingMode() != rtxdi::ReSTIRDI_ResamplingMode::TemporalAndSpatial
      || context->GetFrameIndex() != 0u) return EXIT_FAILURE;
  if (!falcom_world::rtxdi::ValidTemporalSpatialBufferIndices(context->GetBufferIndices())) return EXIT_FAILURE;
  const auto parameters = falcom_world::rtxdi::MakeDiFrameParameters(config, *context);
  if (parameters.width != 1280u || parameters.height != 720u || parameters.history_valid != 0u
      || parameters.flags != (RTXDI_FRAME_FLAG_SPATIAL_AFTER_INITIAL | RTXDI_FRAME_FLAG_SPATIAL_ENABLED)
      || parameters.di.initialSamplingParams.numLocalLightSamples != 8u || parameters.di.initialSamplingParams.numInfiniteLightSamples != 0u
      || parameters.di.initialSamplingParams.numEnvironmentSamples != 0u || parameters.di.initialSamplingParams.numBrdfSamples != 0u
      || parameters.di.initialSamplingParams.environmentMapImportanceSampling != 0u || !parameters.di.shadingParams.enableFinalVisibility
      || parameters.di.shadingParams.reuseFinalVisibility || parameters.di.shadingParams.enableDenoiserInputPacking
      || parameters.di.temporalResamplingParams.enableVisibilityShortcut
      || parameters.di.temporalResamplingParams.enablePermutationSampling || parameters.runtime.pad2
      || parameters.di.reservoirBufferParams.pad1 || parameters.di.reservoirBufferParams.pad2) return EXIT_FAILURE;
  if (parameters.di.bufferIndices.pad1 || parameters.di.bufferIndices.pad2
      || parameters.di.initialSamplingParams.pad1 || parameters.di.initialSamplingParams.pad2 || parameters.di.initialSamplingParams.pad3
      || parameters.di.boilingFilterParams.pad1 || parameters.di.boilingFilterParams.pad2
      || parameters.di.spatialResamplingParams.pad1 || parameters.di.spatialResamplingParams.pad2 || parameters.di.spatialResamplingParams.pad3
      || parameters.di.spatialResamplingParams.numSamples != 4u
      || parameters.di.spatialResamplingParams.numDisocclusionBoostSamples != 4u
      || parameters.di.spatialResamplingParams.samplingRadius != 4.0f
      || parameters.di.spatialResamplingParams.biasCorrectionMode != ReSTIRDI_SpatialBiasCorrectionMode::Basic
      || parameters.di.spatialResamplingParams.discountNaiveSamples != 0u
      || !parameters.di.spatialResamplingParams.enableMaterialSimilarityTest
      || parameters.di.shadingParams.pad1 || parameters.di.shadingParams.pad2 || parameters.di.shadingParams.pad3
      || parameters.lights.localLightBufferRegion.numLights || parameters.lights.infiniteLightBufferRegion.numLights
      || parameters.lights.environmentLightParams.lightPresent) return EXIT_FAILURE;
  const float saved_preset = falcom_world::rtxdi::g_quality_preset;
  const float saved_candidates = falcom_world::rtxdi::g_local_candidates;
  const float saved_spatial_samples = falcom_world::rtxdi::g_spatial_samples;
  const float saved_spatial_disocclusion = falcom_world::rtxdi::g_spatial_disocclusion;
  falcom_world::rtxdi::g_quality_preset = 0.f;
  auto performance = falcom_world::rtxdi::CaptureEffectiveRtxdiSettings();
  falcom_world::rtxdi::g_quality_preset = 2.f;
  auto quality = falcom_world::rtxdi::CaptureEffectiveRtxdiSettings();
  falcom_world::rtxdi::g_quality_preset = 3.f;
  falcom_world::rtxdi::g_local_candidates = 900.f;
  falcom_world::rtxdi::g_spatial_samples = std::numeric_limits<float>::quiet_NaN();
  falcom_world::rtxdi::g_spatial_disocclusion = -3.f;
  const auto custom = falcom_world::rtxdi::CaptureEffectiveRtxdiSettings();
  falcom_world::rtxdi::g_quality_preset = saved_preset;
  falcom_world::rtxdi::g_local_candidates = saved_candidates;
  falcom_world::rtxdi::g_spatial_samples = saved_spatial_samples;
  falcom_world::rtxdi::g_spatial_disocclusion = saved_spatial_disocclusion;
  if (performance.quality_preset != 0u || performance.local_candidates != 4u || performance.spatial_samples != 2u
      || performance.spatial_disocclusion != 2u || balanced_settings.quality_preset != 1u
      || balanced_settings.local_candidates != 8u || balanced_settings.spatial_samples != 4u
      || balanced_settings.spatial_disocclusion != 4u || quality.local_candidates != 16u || quality.spatial_samples != 8u
      || quality.spatial_disocclusion != 8u || custom.local_candidates != 32u || custom.spatial_samples != 4u
      || custom.spatial_disocclusion != 1u || custom.identity == balanced_settings.identity) return EXIT_FAILURE;
  for (uint32_t mode = 0u; mode < 4u; ++mode) {
    auto mode_settings = balanced_settings;
    mode_settings.temporal_enabled = (mode & 1u) != 0u;
    mode_settings.spatial_enabled = (mode & 2u) != 0u;
    const auto mode_context = falcom_world::rtxdi::CreateDiContext(config, mode_settings);
    if (!mode_context) return EXIT_FAILURE;
    auto* rotating_context = mode_context.get();
    uint32_t prior_output = rotating_context->GetBufferIndices().temporalResamplingInputBufferIndex;
    for (uint32_t frame = 0u; frame < 6u; ++frame) {
      const auto indices = rotating_context->GetBufferIndices();
      const uint32_t expected_initial = (prior_output + 1u) % rtxdi::c_NumReSTIRDIReservoirBuffers;
      const uint32_t expected_spatial_output = (expected_initial + 1u) % rtxdi::c_NumReSTIRDIReservoirBuffers;
      const bool temporal_enabled = mode_settings.temporal_enabled;
      const bool spatial_enabled = mode_settings.spatial_enabled;
      if (!falcom_world::rtxdi::ValidDiBufferIndices(indices, mode_settings)
          || indices.initialSamplingOutputBufferIndex != expected_initial
          || indices.temporalResamplingInputBufferIndex != prior_output
          || indices.temporalResamplingOutputBufferIndex != expected_initial
          || indices.spatialResamplingInputBufferIndex != expected_initial
          || indices.spatialResamplingOutputBufferIndex != expected_spatial_output
          || indices.shadingInputBufferIndex != (spatial_enabled ? expected_spatial_output : expected_initial)
          || (temporal_enabled && indices.temporalResamplingInputBufferIndex == expected_initial)
          || (spatial_enabled && indices.spatialResamplingOutputBufferIndex == expected_initial)) {
        std::fprintf(stderr,
                     "FAIL mode rotation mode=%u frame=%u indices init=%u temporal=%u/%u spatial=%u/%u shade=%u expected=%u/%u prior=%u valid=%u\n",
                     mode, frame, indices.initialSamplingOutputBufferIndex,
                     indices.temporalResamplingInputBufferIndex, indices.temporalResamplingOutputBufferIndex,
                     indices.spatialResamplingInputBufferIndex, indices.spatialResamplingOutputBufferIndex,
                     indices.shadingInputBufferIndex, expected_initial, expected_spatial_output, prior_output,
                     static_cast<unsigned>(falcom_world::rtxdi::ValidDiBufferIndices(indices, mode_settings)));
        return EXIT_FAILURE;
      }
      prior_output = indices.shadingInputBufferIndex;
      rotating_context->SetFrameIndex(rotating_context->GetFrameIndex() + 1u);
    }
  }
  RtxdiFrameUpload upload = {};
  upload.parameters = parameters;
  for (const auto byte : upload.padding)
    if (byte != 0u) return EXIT_FAILURE;
  falcom_world::rtxdi::DiConfig resized;
  if (!falcom_world::rtxdi::CalculateDiConfig(17u, 33u, &resized)
      || resized.array_pitch != 1536u || resized.reservoir_bytes != 110592u
      || falcom_world::rtxdi::CreateDiContext({}, balanced_settings) != nullptr) return EXIT_FAILURE;
  using falcom_world::rtxdi::MakeInputLayout;
  using falcom_world::rtxdi::TextureInputExtent;
  using falcom_world::rtxdi::ValidSceneInputRange;
  using falcom_world::rtxdi::ValidTextureInput;
  TextureInputExtent texture = {.width = 2560u, .height = 1440u, .mip_levels = 2u, .mip = 1u, .samples = 1u, .array_size = 1u, .texture_2d_view = true};
  if (!ValidTextureInput(texture, 1280u, 720u)) return EXIT_FAILURE;
  texture.samples = 4u;
  if (ValidTextureInput(texture, 1280u, 720u)) return EXIT_FAILURE;
  texture.samples = 1u;
  texture.array_size = 2u;
  if (ValidTextureInput(texture, 1280u, 720u)) return EXIT_FAILURE;
  using falcom_world::rtxdi::ValidStructuredInput;
  if (!ValidStructuredInput({.buffer_bytes = 1120u, .first_element = 3u, .element_count = 7u, .stride = 112u}, 112u)
      || ValidStructuredInput({.buffer_bytes = 1119u, .first_element = 3u, .element_count = 7u, .stride = 112u}, 112u)
      || ValidStructuredInput({.buffer_bytes = 224u, .first_element = 1u, .element_count = 2u, .stride = 112u}, 112u)
      || !ValidStructuredInput({.buffer_bytes = 224u, .first_element = 1u, .element_count = 1u, .stride = 112u}, 112u)
      || ValidStructuredInput({.buffer_bytes = 1120u, .first_element = 3u, .element_count = 7u, .stride = 144u}, 112u)
      || ValidStructuredInput({.buffer_bytes = 4096u, .first_element = UINT32_MAX, .element_count = UINT32_MAX, .stride = 112u}, 112u)
      || falcom_world::rtxdi::StructuredInputFailure(
             {.buffer_bytes = 224u, .first_element = 1u, .element_count = 2u, .stride = 112u}, 112u)
             != falcom_world::rtxdi::DiagnosticStructuredFailure::ElementBoundsInvalid
      || falcom_world::rtxdi::StructuredInputFailure(
             {.buffer_bytes = 224u, .first_element = 1u, .element_count = 1u, .stride = 144u}, 112u)
             != falcom_world::rtxdi::DiagnosticStructuredFailure::StructureStrideMismatch
      || falcom_world::rtxdi::StructuredInputFailure(
             {.buffer_bytes = 224u, .first_element = 1u, .element_count = 1u, .stride = 112u}, 112u)
             != falcom_world::rtxdi::DiagnosticStructuredFailure::None
      || !ValidSceneInputRange(2048u, 256u, 1536u) || ValidSceneInputRange(1791u, 256u, 1536u)
      || ValidSceneInputRange(2048u, 256u, 1535u)) return EXIT_FAILURE;
  uint64_t normalized = 0u;
  uint32_t descriptor_index = 0u;
  if (!falcom_world::rtxdi::NormalizeSceneInputRange(2048u, 256u, UINT64_MAX, &normalized) || normalized != 1792u
      || !falcom_world::rtxdi::NormalizeSceneInputRange(1792u, 256u, 1536u, &normalized)
      || falcom_world::rtxdi::NormalizeSceneInputRange(1792u, 257u, 1536u, &normalized)
      || falcom_world::rtxdi::NormalizeSceneInputRange(1792u, 256u, 1792u, &normalized)
      || !falcom_world::rtxdi::SceneDescriptorIndex(2u, 3u, 4u, &descriptor_index) || descriptor_index != 2u
      || falcom_world::rtxdi::SceneDescriptorIndex(2u, 2u, 4u, &descriptor_index)
      || !falcom_world::rtxdi::SceneCaptureCurrent(4u, 4u, 9u, 9u, true)
      || falcom_world::rtxdi::SceneCaptureCurrent(4u, 5u, 9u, 9u, true)
      || falcom_world::rtxdi::SceneCaptureCurrent(4u, 4u, 9u, 8u, true)
      || falcom_world::rtxdi::SceneCaptureCurrent(4u, 4u, 9u, 9u, false)) return EXIT_FAILURE;
  using falcom_world::rtxdi::MapSceneDescriptorRange;
  using falcom_world::rtxdi::SceneDescriptorRange;
  using falcom_world::rtxdi::SceneRangeMapping;
  SceneDescriptorRange mapping = {.binding = 5u, .register_index = 0u, .register_space = 0u, .count = 3u, .array_size = 1u, .constant_buffer = true, .pixel_visible = true};
  if (MapSceneDescriptorRange(mapping, 0u, &descriptor_index) != SceneRangeMapping::Mapped || descriptor_index != 5u
      || !falcom_world::rtxdi::SceneDescriptorIndex(4u, 3u, descriptor_index, &descriptor_index) || descriptor_index != 1u) return EXIT_FAILURE;
  mapping.register_space = 1u;
  if (MapSceneDescriptorRange(mapping, 0u, &descriptor_index) != SceneRangeMapping::Unrelated) return EXIT_FAILURE;
  mapping.register_space = 0u;
  mapping.pixel_visible = false;
  if (MapSceneDescriptorRange(mapping, 0u, &descriptor_index) != SceneRangeMapping::Unrelated) return EXIT_FAILURE;
  mapping.pixel_visible = true;
  mapping.constant_buffer = false;
  if (MapSceneDescriptorRange(mapping, 0u, &descriptor_index) != SceneRangeMapping::Unrelated) return EXIT_FAILURE;
  mapping.constant_buffer = true;
  mapping.array_size = 2u;
  if (MapSceneDescriptorRange(mapping, 0u, &descriptor_index) != SceneRangeMapping::Unsupported) return EXIT_FAILURE;
  falcom_world::rtxdi::SceneRangeCapture tracker;
  falcom_world::rtxdi::TrackSceneBinding(&tracker, {.buffer = 21u, .offset = 256u, .size = 1536u, .frame = 4u, .command = 9u, .source = falcom_world::rtxdi::SceneCaptureSource::Push, .live = true, .known = true});
  auto tracked = tracker.bindings.at(9u);
  if (falcom_world::rtxdi::CanPromoteSceneBinding(tracked, 4u, 9u, false)
      || !falcom_world::rtxdi::CanPromoteSceneBinding(tracked, 4u, 9u, true)
      || falcom_world::rtxdi::CanPromoteSceneBinding(tracked, 5u, 9u, true)) return EXIT_FAILURE;
  falcom_world::rtxdi::TrackSceneBinding(&tracker, {.buffer = 22u, .frame = 4u, .command = 9u, .source = falcom_world::rtxdi::SceneCaptureSource::DescriptorTable, .live = true, .known = true});
  if (tracker.bindings.at(9u).buffer != 22u || tracker.bindings.at(9u).sequence == tracked.sequence) return EXIT_FAILURE;
  falcom_world::rtxdi::InvalidateSceneBuffer(&tracker, 22u);
  if (falcom_world::rtxdi::CanPromoteSceneBinding(tracker.bindings.at(9u), 4u, 9u, true)) return EXIT_FAILURE;
  falcom_world::rtxdi::TrackSceneBinding(&tracker, {.frame = 4u, .command = 9u});
  if (falcom_world::rtxdi::CanPromoteSceneBinding(tracker.bindings.at(9u), 4u, 9u, true)) return EXIT_FAILURE;
  falcom_world::rtxdi::ResetSceneBinding(&tracker, 9u);
  if (tracker.bindings.contains(9u)) return EXIT_FAILURE;
  falcom_world::rtxdi::SceneRangeCapture guarded;
  falcom_world::rtxdi::TrackSceneBinding(&guarded, {.buffer = 55u, .offset = 256u, .size = UINT64_MAX, .frame = 4u, .command = 9u, .live = true, .known = true});
  auto promoted = guarded.bindings.at(9u);
  promoted.normalized_size = 1792u;
  guarded.snapshot = promoted;
  using falcom_world::rtxdi::CheckPromotedSceneBinding;
  using falcom_world::rtxdi::SceneBindingStatus;
  if (CheckPromotedSceneBinding(promoted, guarded.bindings.at(9u), 4u, 9u, 2048u) != SceneBindingStatus::Ready) return EXIT_FAILURE;
  // An unrelated register does not call TrackSceneBinding, so the snapshot remains valid.
  if (MapSceneDescriptorRange({.binding = 1u, .register_index = 1u, .count = 1u, .constant_buffer = true, .pixel_visible = true}, 0u, &descriptor_index) != SceneRangeMapping::Unrelated
      || CheckPromotedSceneBinding(promoted, guarded.bindings.at(9u), 4u, 9u, 2048u) != SceneBindingStatus::Ready) return EXIT_FAILURE;
  falcom_world::rtxdi::TrackSceneBinding(&guarded, {.buffer = 55u, .offset = 0u, .size = 1536u, .frame = 4u, .command = 9u, .live = true, .known = true});
  if (guarded.snapshot.live || CheckPromotedSceneBinding(promoted, guarded.bindings.at(9u), 4u, 9u, 2048u) != SceneBindingStatus::SceneBindingChanged) return EXIT_FAILURE;
  auto after_rebind = guarded.bindings.at(9u);
  guarded.snapshot = after_rebind;
  falcom_world::rtxdi::ResetSceneBinding(&guarded, 9u);
  falcom_world::rtxdi::TrackSceneBinding(&guarded, {.buffer = 55u, .size = 1536u, .frame = 4u, .command = 9u, .live = true, .known = true});
  if (guarded.snapshot.live || guarded.bindings.at(9u).reset_generation == after_rebind.reset_generation
      || CheckPromotedSceneBinding(after_rebind, guarded.bindings.at(9u), 4u, 9u, 2048u) != SceneBindingStatus::SceneBindingChanged) return EXIT_FAILURE;
  auto after_reset = guarded.bindings.at(9u);
  guarded.snapshot = after_reset;
  falcom_world::rtxdi::InvalidateSceneBuffer(&guarded, 55u);
  if (guarded.snapshot.live || CheckPromotedSceneBinding(after_reset, guarded.bindings.at(9u), 4u, 9u, 2048u) != SceneBindingStatus::SceneBindingChanged) return EXIT_FAILURE;
  if (!falcom_world::rtxdi::TransferSlotAvailable(8u, 8u) || falcom_world::rtxdi::TransferSlotAvailable(9u, 8u)
      || !falcom_world::rtxdi::TransferAttemptDuplicate(4u, 3u, 4u, 3u)
      || falcom_world::rtxdi::TransferAttemptDuplicate(4u, 3u, 4u, 4u)) return EXIT_FAILURE;
  uint64_t sun_result_bytes = 0u;
  if (!falcom_world::rtxdi::SunVisibilityRequested(2u) || !falcom_world::rtxdi::SunVisibilityRequested(3u)
      || falcom_world::rtxdi::SunVisibilityRequested(1u)
      || !falcom_world::rtxdi::ValidSceneLease(1, 4u, 1, 4u, 17u, 17u)
      || falcom_world::rtxdi::ValidSceneLease(1, 4u, 2, 4u, 17u, 17u)
      || falcom_world::rtxdi::ValidSceneLease(1, 4u, 1, 5u, 17u, 17u)
      || falcom_world::rtxdi::ValidSceneLease(1, 4u, 1, 4u, 17u, 18u)
      || !falcom_world::rtxdi::SceneCoverageSnapshotMatches(1, 4u, 3u, 17u, 5u, 1, 4u, 3u, 17u, 5u)
      || falcom_world::rtxdi::SceneCoverageSnapshotMatches(1, 4u, 3u, 17u, 5u, 1, 4u, 3u, 17u, 6u)
      || falcom_world::rtxdi::SceneCoverageSnapshotMatches(1, 4u, 3u, 17u, 5u, 1, 4u, 4u, 17u, 5u)
      || !falcom_world::rtxdi::CheckedSunResultBytes(1920u, 1080u, &sun_result_bytes)
      || sun_result_bytes != uint64_t{1920u} * 1080u * RTXDI_SUN_RESULT_STRIDE
      || falcom_world::rtxdi::CheckedSunResultBytes(UINT32_MAX, UINT32_MAX, &sun_result_bytes)
      || sizeof(RtxdiSunVisibilityResult) != RTXDI_SUN_RESULT_STRIDE
      || falcom_world::rtxdi::ResolveSunVisibility(false, 0.f, true, 10.f, 10.f) != SunVisibilityStatus::Visible
      || falcom_world::rtxdi::ResolveSunVisibility(false, 0.f, true, 10.f, 9.99f) != SunVisibilityStatus::Unknown
      || falcom_world::rtxdi::ResolveSunVisibility(true, 10.f, true, 10.f, 10.f) != SunVisibilityStatus::Blocked
      || falcom_world::rtxdi::ResolveSunVisibility(true, 2.f, false, 10.f, 10.f) != SunVisibilityStatus::Unknown
      || falcom_world::rtxdi::ResolveSunVisibility(false, 0.f, true, 10.f, 10.f - 1e-4f) != SunVisibilityStatus::Unknown) return EXIT_FAILURE;
  RtxdiInputLayout inputs = {};
  RtxdiInputLayout sun_only_inputs = {}, local_only_inputs = {};
  if (!MakeInputLayout(13u, 9u, false, false, 4096u, 4608u, 20000u, &sun_only_inputs)
      || sun_only_inputs.flags != 0u || sun_only_inputs.lights_copied != 0u || sun_only_inputs.tiles_copied != 0u
      || sun_only_inputs.deferred_copied != 0u
      || !MakeInputLayout(13u, 9u, false, true, 1u, 1u, 1u, &local_only_inputs)
      || local_only_inputs.flags != 1u || !MakeInputLayout(13u, 9u, false, true, 1u, 1u, 1u, &inputs)) {
    return EXIT_FAILURE;
  }
  if (!MakeInputLayout(1280u, 720u, false, true, 1050u, 4608u, 20000u, &inputs)
      || inputs.lights_copied != 1024u || inputs.lights_truncated != 26u
      || MakeInputLayout(1280u, 720u, false, true, 0u, 4609u, 1u, &inputs)
      || MakeInputLayout(1280u, 720u, false, true, 0u, 1u, 20001u, &inputs)
      || !MakeInputLayout(1280u, 720u, false, true, 0u, 0u, 0u, &inputs)
      || !MakeInputLayout(1280u, 720u, false, false, 9999u, 9999u, 99999u, &inputs)
      || inputs.lights_copied != 0u) return EXIT_FAILURE;
  auto surface_rows = MakeSurfaceSceneRows();
  const std::array<uint32_t, 4> negative_z_non_character = {0u, 0u, 0u, 8u};
  if (RTXDI_SURFACE_SCENE_ROW_COUNT != 96u || RTXDI_SURFACE_VIEW_ROW != 0u
      || RTXDI_SURFACE_INVERSE_VIEW_ROW != 4u || RTXDI_SURFACE_INVERSE_VIEW_PROJECTION_ROW != 20u
      || RTXDI_SURFACE_SUN_RESERVED_ROW != 26u || RTXDI_SURFACE_RESOLUTION_SCALING_ROW != 49u) return EXIT_FAILURE;
  const auto surface = ReconstructTestSurface(0.5f, negative_z_non_character, 0u, 0u, 2u, 2u, surface_rows);
  if (surface.rejection != RTXDI_SURFACE_REJECT_NONE || surface.flags != RTXDI_SURFACE_FLAG_VALID
      || std::abs(surface.world_position[0] - 4.5f) > 1e-6f || std::abs(surface.world_position[1] - 2.5f) > 1e-6f
      || std::abs(surface.world_position[2] - 3.f) > 1e-6f || std::abs(surface.shading_normal[2] + 1.f) > 1e-6f
      || std::abs(surface.linear_view_depth - 1.5f) > 1e-6f) return EXIT_FAILURE;
  const float euclidean_distance = std::sqrt((surface.world_position[0] - 3.f) * (surface.world_position[0] - 3.f)
                                             + (surface.world_position[1] - 2.f) * (surface.world_position[1] - 2.f)
                                             + (surface.world_position[2] - 10.f) * (surface.world_position[2] - 10.f));
  const float view_direction_length = std::sqrt(surface.view_direction[0] * surface.view_direction[0]
                                                + surface.view_direction[1] * surface.view_direction[1]
                                                + surface.view_direction[2] * surface.view_direction[2]);
  const float inverse_camera_distance = 1.f / std::sqrt(51.5f);
  if (std::abs(euclidean_distance - surface.linear_view_depth) < 1.f || std::abs(view_direction_length - 1.f) > 1e-6f
      || std::abs(surface.view_direction[0] + 1.5f * inverse_camera_distance) > 1e-6f
      || std::abs(surface.view_direction[1] + 0.5f * inverse_camera_distance) > 1e-6f
      || std::abs(surface.view_direction[2] - 7.f * inverse_camera_distance) > 1e-6f) return EXIT_FAILURE;
  const auto y_pixel_surface = ReconstructTestSurface(0.5f, negative_z_non_character, 1u, 0u, 2u, 2u, surface_rows);
  if (y_pixel_surface.rejection != RTXDI_SURFACE_REJECT_NONE || std::abs(y_pixel_surface.world_position[0] - 5.5f) > 1e-6f
      || std::abs(y_pixel_surface.world_position[1] - 2.5f) > 1e-6f) return EXIT_FAILURE;
  const auto bottom_pixel_surface = ReconstructTestSurface(0.5f, negative_z_non_character, 0u, 1u, 2u, 2u, surface_rows);
  if (bottom_pixel_surface.rejection != RTXDI_SURFACE_REJECT_NONE
      || std::abs(bottom_pixel_surface.world_position[1] - 1.5f) > 1e-6f) return EXIT_FAILURE;
  auto character_mrt0 = negative_z_non_character;
  character_mrt0[3] = 0u;
  const auto character_surface = ReconstructTestSurface(0.5f, character_mrt0, 0u, 0u, 2u, 2u, surface_rows);
  if (character_surface.rejection != RTXDI_SURFACE_REJECT_NONE
      || character_surface.flags != (RTXDI_SURFACE_FLAG_VALID | RTXDI_SURFACE_FLAG_CHARACTER)) return EXIT_FAILURE;
  const auto expect_surface_rejection = [&](float depth, const std::array<uint32_t, 4>& mrt0, uint32_t px,
                                            uint32_t py, uint32_t width, uint32_t height,
                                            const SurfaceSceneRows& rows, uint32_t reason) {
    const auto rejected = ReconstructTestSurface(depth, mrt0, px, py, width, height, rows);
    return rejected.rejection == reason && SurfaceIsZero(rejected);
  };
  if (!expect_surface_rejection(0.5f, negative_z_non_character, 0u, 0u, 0u, 2u, surface_rows, RTXDI_SURFACE_REJECT_DIMENSIONS)
      || !expect_surface_rejection(0.5f, negative_z_non_character, 2u, 0u, 2u, 2u, surface_rows, RTXDI_SURFACE_REJECT_PIXEL)) return EXIT_FAILURE;
  auto invalid_surface_rows = surface_rows;
  SetSceneRow(&invalid_surface_rows, RTXDI_SURFACE_RESOLUTION_SCALING_ROW, {0.5f, 1.f, 0.f, 0.f});
  if (!expect_surface_rejection(0.5f, negative_z_non_character, 0u, 0u, 2u, 2u, invalid_surface_rows, RTXDI_SURFACE_REJECT_SCALING)) return EXIT_FAILURE;
  SetSceneRow(&invalid_surface_rows, RTXDI_SURFACE_RESOLUTION_SCALING_ROW, {std::numeric_limits<float>::quiet_NaN(), 1.f, 0.f, 0.f});
  if (!expect_surface_rejection(0.5f, negative_z_non_character, 0u, 0u, 2u, 2u, invalid_surface_rows, RTXDI_SURFACE_REJECT_SCALING)
      || !expect_surface_rejection(0.f, negative_z_non_character, 0u, 0u, 2u, 2u, surface_rows, RTXDI_SURFACE_REJECT_DEPTH)
      || !expect_surface_rejection(-0.1f, negative_z_non_character, 0u, 0u, 2u, 2u, surface_rows, RTXDI_SURFACE_REJECT_DEPTH)
      || !expect_surface_rejection(1.01f, negative_z_non_character, 0u, 0u, 2u, 2u, surface_rows, RTXDI_SURFACE_REJECT_DEPTH)
      || !expect_surface_rejection(std::numeric_limits<float>::quiet_NaN(), negative_z_non_character, 0u, 0u, 2u, 2u, surface_rows, RTXDI_SURFACE_REJECT_DEPTH)) return EXIT_FAILURE;
  auto invalid_normal = negative_z_non_character;
  invalid_normal[1] = 65536u;
  if (!expect_surface_rejection(0.5f, invalid_normal, 0u, 0u, 2u, 2u, surface_rows, RTXDI_SURFACE_REJECT_NORMAL)) return EXIT_FAILURE;
  invalid_surface_rows = surface_rows;
  SetSceneRow(&invalid_surface_rows, RTXDI_SURFACE_INVERSE_VIEW_PROJECTION_ROW + 3u, {0.f, 0.f, 0.f, 0.f});
  if (!expect_surface_rejection(0.5f, negative_z_non_character, 0u, 0u, 2u, 2u, invalid_surface_rows,
                                RTXDI_SURFACE_REJECT_HOMOGENEOUS_W)) return EXIT_FAILURE;
  invalid_surface_rows = surface_rows;
  SetSceneRow(&invalid_surface_rows, RTXDI_SURFACE_INVERSE_VIEW_PROJECTION_ROW + 3u,
              {0.f, 0.f, 0.f, std::numeric_limits<float>::quiet_NaN()});
  if (!expect_surface_rejection(0.5f, negative_z_non_character, 0u, 0u, 2u, 2u, invalid_surface_rows,
                                RTXDI_SURFACE_REJECT_HOMOGENEOUS_W)) return EXIT_FAILURE;
  invalid_surface_rows = surface_rows;
  SetSceneRow(&invalid_surface_rows, RTXDI_SURFACE_INVERSE_VIEW_PROJECTION_ROW + 0u,
              {std::numeric_limits<float>::infinity(), 0.f, 0.f, 5.f});
  if (!expect_surface_rejection(0.5f, negative_z_non_character, 0u, 0u, 2u, 2u, invalid_surface_rows,
                                RTXDI_SURFACE_REJECT_POSITION)) return EXIT_FAILURE;
  invalid_surface_rows = surface_rows;
  SetSceneRow(&invalid_surface_rows, RTXDI_SURFACE_INVERSE_VIEW_PROJECTION_ROW + 0u, {3e38f, 0.f, 0.f, 5.f});
  SetSceneRow(&invalid_surface_rows, RTXDI_SURFACE_INVERSE_VIEW_PROJECTION_ROW + 3u, {0.f, 0.f, 0.f, 1e-7f});
  if (!expect_surface_rejection(0.5f, negative_z_non_character, 0u, 0u, 2u, 2u, invalid_surface_rows, RTXDI_SURFACE_REJECT_POSITION)) return EXIT_FAILURE;
  invalid_surface_rows = surface_rows;
  SetSceneRow(&invalid_surface_rows, RTXDI_SURFACE_INVERSE_VIEW_ROW + 0u,
              {0.f, 0.f, 0.f, std::numeric_limits<float>::infinity()});
  if (!expect_surface_rejection(0.5f, negative_z_non_character, 0u, 0u, 2u, 2u, invalid_surface_rows,
                                RTXDI_SURFACE_REJECT_VIEW_DIRECTION)) return EXIT_FAILURE;
  invalid_surface_rows = surface_rows;
  SetSceneRow(&invalid_surface_rows, RTXDI_SURFACE_INVERSE_VIEW_ROW + 0u, {0.f, 0.f, 0.f, 4.5f});
  SetSceneRow(&invalid_surface_rows, RTXDI_SURFACE_INVERSE_VIEW_ROW + 1u, {0.f, 0.f, 0.f, 2.5f});
  SetSceneRow(&invalid_surface_rows, RTXDI_SURFACE_INVERSE_VIEW_ROW + 2u, {0.f, 0.f, 0.f, 3.f});
  if (!expect_surface_rejection(0.5f, negative_z_non_character, 0u, 0u, 2u, 2u, invalid_surface_rows,
                                RTXDI_SURFACE_REJECT_VIEW_DIRECTION)) return EXIT_FAILURE;
  invalid_surface_rows = surface_rows;
  SetSceneRow(&invalid_surface_rows, RTXDI_SURFACE_VIEW_ROW + 2u, {-1.f, 0.f, 0.f, 5.f});
  if (!expect_surface_rejection(0.5f, negative_z_non_character, 0u, 0u, 2u, 2u, invalid_surface_rows,
                                RTXDI_SURFACE_REJECT_VIEW_DEPTH)) return EXIT_FAILURE;
  invalid_surface_rows = surface_rows;
  SetSceneRow(&invalid_surface_rows, RTXDI_SURFACE_VIEW_ROW + 2u,
              {-1.f, 0.f, 0.f, std::numeric_limits<float>::infinity()});
  if (!expect_surface_rejection(0.5f, negative_z_non_character, 0u, 0u, 2u, 2u, invalid_surface_rows,
                                RTXDI_SURFACE_REJECT_VIEW_DEPTH)) return EXIT_FAILURE;
  for (const auto dimensions : std::array{std::array<uint32_t, 2>{1, 1}, std::array<uint32_t, 2>{17, 33}}) {
    rtxdi::ReSTIRDIContext committed({.RenderWidth = dimensions[0], .RenderHeight = dimensions[1], .CheckerboardSamplingMode = rtxdi::CheckerboardMode::Off});
    committed.SetResamplingMode(rtxdi::ReSTIRDI_ResamplingMode::Spatial);
    auto spatial = committed.GetSpatialResamplingParameters();
    spatial.discountNaiveSamples = 0u;
    spatial.enableMaterialSimilarityTest = 1u;
    committed.SetSpatialResamplingParameters(spatial);
    committed.SetFrameIndex(0);
    auto initial = committed.GetInitialSamplingParameters();
    initial.numLocalLightSamples = 8;
    initial.numInfiniteLightSamples = 0;
    initial.numEnvironmentSamples = 0;
    initial.numBrdfSamples = 0;
    initial.localLightSamplingMode = ReSTIRDI_LocalLightSamplingMode::Uniform;
    initial.environmentMapImportanceSampling = 0;
    committed.SetInitialSamplingParameters(initial);
    auto shading = committed.GetShadingParameters();
    shading.enableFinalVisibility = 1;
    shading.reuseFinalVisibility = 0;
    shading.enableDenoiserInputPacking = 0;
    committed.SetShadingParameters(shading);
    auto temporal = committed.GetTemporalResamplingParameters();
    temporal.enableVisibilityShortcut = 0;
    committed.SetTemporalResamplingParameters(temporal);
    const auto params = committed.GetReservoirBufferParameters();
    const uint64_t bytes = uint64_t{params.reservoirArrayPitch} * rtxdi::c_NumReSTIRDIReservoirBuffers * sizeof(RTXDI_PackedDIReservoir);
    if (bytes < uint64_t{dimensions[0]} * dimensions[1] * 3 * sizeof(RTXDI_PackedDIReservoir)) return EXIT_FAILURE;
    const auto before = committed.GetBufferIndices();
    if (!falcom_world::rtxdi::ValidSpatialBufferIndices(before)) return EXIT_FAILURE;
    const uint32_t accepted_frame = committed.GetFrameIndex();
    auto rejected = committed;
    if (CheckDiSubmissionIdentity(busy_transfer) != DiSubmissionGate::Busy) return EXIT_FAILURE;
    if (CheckDiSubmissionIdentity(busy_transfer) == DiSubmissionGate::Ready) rejected.SetFrameIndex(accepted_frame + 1u);
    if (rejected.GetFrameIndex() != accepted_frame || rejected.GetBufferIndices().shadingInputBufferIndex != before.shadingInputBufferIndex) return EXIT_FAILURE;
    auto accepted = committed;
    if (CheckDiSubmissionIdentity(di_identity) != DiSubmissionGate::Ready) return EXIT_FAILURE;
    accepted.SetFrameIndex(accepted_frame + 1u);
    if (accepted.GetFrameIndex() != accepted_frame + 1u
        || accepted.GetBufferIndices().temporalResamplingInputBufferIndex != before.shadingInputBufferIndex) return EXIT_FAILURE;
    auto candidate = committed;
    candidate.SetFrameIndex(1);
    const auto first = candidate.GetBufferIndices();
    candidate.SetFrameIndex(2);
    const auto second = candidate.GetBufferIndices();
    if (!falcom_world::rtxdi::ValidSpatialBufferIndices(first) || !falcom_world::rtxdi::ValidSpatialBufferIndices(second)
        || first.shadingInputBufferIndex == second.shadingInputBufferIndex
        || committed.GetBufferIndices().shadingInputBufferIndex != before.shadingInputBufferIndex) return EXIT_FAILURE;
    for (const auto index : {second.initialSamplingOutputBufferIndex, second.temporalResamplingInputBufferIndex, second.temporalResamplingOutputBufferIndex, second.spatialResamplingInputBufferIndex, second.spatialResamplingOutputBufferIndex, second.shadingInputBufferIndex}) {
      if (index >= rtxdi::c_NumReSTIRDIReservoirBuffers) return EXIT_FAILURE;
    }
  }
  const falcom_world::rtxdi::DiHistoryIdentity adjacent_history = {
      .previous_frame = 40u, .current_frame = 41u, .previous_width = 1920u, .previous_height = 1080u, .current_width = 1920u, .current_height = 1080u, .previous_transfer_slot = 1, .current_transfer_slot = 2, .previous_owner_generation = 7u, .current_owner_generation = 7u, .previous_configuration_generation = 3u, .current_configuration_generation = 3u, .motion_available = true, .previous_inputs_available = true};
  if (!falcom_world::rtxdi::ValidDiHistoryIdentity(adjacent_history)) return EXIT_FAILURE;
  auto no_motion_history = adjacent_history;
  no_motion_history.motion_available = false;
  auto gap_history = adjacent_history;
  gap_history.current_frame++;
  auto resized_history = adjacent_history;
  resized_history.current_width++;
  auto changed_config_history = adjacent_history;
  changed_config_history.current_configuration_generation++;
  auto reused_slot_history = adjacent_history;
  reused_slot_history.current_transfer_slot = reused_slot_history.previous_transfer_slot;
  auto missing_slot_history = adjacent_history;
  missing_slot_history.previous_inputs_available = false;
  if (falcom_world::rtxdi::ValidDiHistoryIdentity(no_motion_history)
      || falcom_world::rtxdi::ValidDiHistoryIdentity(gap_history)
      || falcom_world::rtxdi::ValidDiHistoryIdentity(resized_history)
      || falcom_world::rtxdi::ValidDiHistoryIdentity(changed_config_history)
      || falcom_world::rtxdi::ValidDiHistoryIdentity(reused_slot_history)
      || falcom_world::rtxdi::ValidDiHistoryIdentity(missing_slot_history)) return EXIT_FAILURE;
  using falcom_world::rtxdi::BeginSceneCpuMap;
  using falcom_world::rtxdi::FinishSceneCpuMap;
  using falcom_world::rtxdi::InvalidateSceneCpuMirror;
  using falcom_world::rtxdi::MirrorSceneCpuWrite;
  using falcom_world::rtxdi::ReadSceneCpuMirror;
  using falcom_world::rtxdi::RegisterSceneCpuMirrorRange;
  using falcom_world::rtxdi::ResetSceneCpuMirrorsForDevice;
  using falcom_world::rtxdi::SeedSceneCpuMirrorInitial;
  constexpr uintptr_t camera_device = 0xA11u;
  constexpr uint64_t camera_buffer = 0xB22u;
  constexpr uint64_t camera_offset = 64u;
  constexpr uint64_t camera_bytes = RTXDI_SURFACE_SCENE_ROW_COUNT * 16u;
  std::array<uint8_t, camera_bytes> camera_source = {};
  std::array<uint8_t, camera_bytes> camera_copy = {};
  for (uint32_t index = 0u; index < camera_source.size(); ++index) camera_source[index] = static_cast<uint8_t>(index * 17u);
  RegisterSceneCpuMirrorRange(camera_device, camera_buffer, camera_offset, camera_bytes, 4u, 12u);
  if (ReadSceneCpuMirror(camera_device, camera_buffer, camera_offset, camera_bytes, 4u, 12u,
                         camera_copy.data(), nullptr, nullptr)) return EXIT_FAILURE;
  MirrorSceneCpuWrite(camera_device, camera_buffer, camera_source.data(), camera_offset, 700u, 4u);
  MirrorSceneCpuWrite(camera_device, camera_buffer, camera_source.data() + 700u, camera_offset + 700u,
                      camera_bytes - 700u, 4u);
  RegisterSceneCpuMirrorRange(camera_device, camera_buffer, camera_offset, camera_bytes, 5u, 13u);
  uint64_t camera_write_sequence = 0u;
  uint64_t camera_write_frame = UINT64_MAX;
  if (!ReadSceneCpuMirror(camera_device, camera_buffer, camera_offset, camera_bytes, 5u, 13u,
                          camera_copy.data(), &camera_write_sequence, &camera_write_frame)
      || camera_copy != camera_source || camera_write_sequence != 2u || camera_write_frame != 4u
      || ReadSceneCpuMirror(camera_device, camera_buffer, camera_offset, camera_bytes, 5u, 12u,
                            camera_copy.data(), nullptr, nullptr)) return EXIT_FAILURE;
  if (ReadSceneCpuMirror(camera_device, camera_buffer, camera_offset, camera_bytes, 5u, 13u,
                         camera_copy.data(), nullptr, nullptr)) return EXIT_FAILURE;
  MirrorSceneCpuWrite(camera_device, camera_buffer, camera_source.data(), camera_offset, 128u, 5u);
  RegisterSceneCpuMirrorRange(camera_device, camera_buffer, camera_offset, camera_bytes, 6u, 14u);
  if (ReadSceneCpuMirror(camera_device, camera_buffer, camera_offset, camera_bytes, 6u, 14u,
                         camera_copy.data(), nullptr, nullptr)) return EXIT_FAILURE;
  MirrorSceneCpuWrite(camera_device, camera_buffer, camera_source.data() + 128u, camera_offset + 128u,
                      camera_bytes - 128u, 5u);
  if (!ReadSceneCpuMirror(camera_device, camera_buffer, camera_offset, camera_bytes, 6u, 14u,
                          camera_copy.data(), &camera_write_sequence, &camera_write_frame)
      || camera_copy != camera_source || camera_write_sequence != 4u || camera_write_frame != 5u) return EXIT_FAILURE;
  constexpr uint64_t initial_buffer = 0xB24u;
  RegisterSceneCpuMirrorRange(camera_device, initial_buffer, 0u, camera_bytes, 7u, 16u);
  SeedSceneCpuMirrorInitial(camera_device, initial_buffer, 0u, camera_bytes, camera_source.data());
  if (!ReadSceneCpuMirror(camera_device, initial_buffer, 0u, camera_bytes, 7u, 16u,
                          camera_copy.data(), &camera_write_sequence, &camera_write_frame)
      || camera_copy != camera_source || camera_write_frame != UINT64_MAX) return EXIT_FAILURE;
  RegisterSceneCpuMirrorRange(camera_device, initial_buffer, 0u, camera_bytes, 8u, 17u);
  if (!ReadSceneCpuMirror(camera_device, initial_buffer, 0u, camera_bytes, 8u, 17u,
                          camera_copy.data(), nullptr, nullptr)) return EXIT_FAILURE;
  MirrorSceneCpuWrite(camera_device, initial_buffer, camera_source.data(), 0u, 128u, 8u);
  RegisterSceneCpuMirrorRange(camera_device, initial_buffer, 0u, camera_bytes, 9u, 18u);
  if (ReadSceneCpuMirror(camera_device, initial_buffer, 0u, camera_bytes, 9u, 18u,
                         camera_copy.data(), nullptr, nullptr)) return EXIT_FAILURE;
  std::array<uint8_t, camera_offset + camera_bytes> camera_mapped = {};
  for (uint32_t index = 0u; index < camera_bytes; ++index) camera_mapped[camera_offset + index] = static_cast<uint8_t>(255u - camera_source[index]);
  constexpr uint64_t mapped_buffer = 0xB23u;
  RegisterSceneCpuMirrorRange(camera_device, mapped_buffer, camera_offset, camera_bytes, 5u, 14u);
  BeginSceneCpuMap(camera_device, mapped_buffer, 0u, camera_mapped.size(), camera_mapped.data());
  FinishSceneCpuMap(camera_device, mapped_buffer, 5u);
  if (!ReadSceneCpuMirror(camera_device, mapped_buffer, camera_offset, camera_bytes, 5u, 14u,
                          camera_copy.data(), &camera_write_sequence, nullptr)
      || camera_write_sequence != 1u) return EXIT_FAILURE;
  for (uint32_t index = 0u; index < camera_bytes; ++index) {
    if (camera_copy[index] != camera_mapped[camera_offset + index]) return EXIT_FAILURE;
  }
  InvalidateSceneCpuMirror(camera_device, camera_buffer);
  if (ReadSceneCpuMirror(camera_device, camera_buffer, camera_offset, camera_bytes, 5u, 13u,
                         camera_copy.data(), nullptr, nullptr)) return EXIT_FAILURE;
  RegisterSceneCpuMirrorRange(camera_device, camera_buffer, camera_offset, camera_bytes, 6u, 15u);
  if (ReadSceneCpuMirror(camera_device, camera_buffer, camera_offset, camera_bytes, 6u, 15u,
                         camera_copy.data(), nullptr, nullptr)) return EXIT_FAILURE;
  ResetSceneCpuMirrorsForDevice(camera_device);
  std::printf("PASS RTXDI 96-row mirror sequence/reset/map contracts\n");
  {
    using namespace falcom_world::rtxdi;
    constexpr uintptr_t prefix_device = 0xA12u;
    constexpr uint64_t prefix_buffer = 0xB25u;
    constexpr uint64_t prefix_offset = 256u;
    constexpr uint64_t prefix_bytes = RTXDI_INPUT_SCENE_REQUIRED_ROWS * 16u;
    std::array<uint8_t, prefix_bytes> prefix_source = {};
    std::array<uint8_t, prefix_bytes> prefix_copy = {};
    for (uint32_t index = 0u; index < prefix_bytes; ++index) prefix_source[index] = static_cast<uint8_t>(index * 29u);
    RegisterSceneCpuMirrorRange(prefix_device, prefix_buffer, prefix_offset, prefix_bytes, 10u, 20u);
    SeedSceneCpuMirrorInitial(prefix_device, prefix_buffer, prefix_offset, prefix_bytes, prefix_source.data());
    if (!ReadSceneCpuMirror(prefix_device, prefix_buffer, prefix_offset, prefix_bytes, 10u, 20u,
                            prefix_copy.data(), nullptr, nullptr)
        || prefix_copy != prefix_source) return EXIT_FAILURE;
    RegisterSceneCpuMirrorRange(prefix_device, prefix_buffer, prefix_offset, prefix_bytes, 11u, 21u);
    MirrorSceneCpuWrite(prefix_device, prefix_buffer, prefix_source.data(), prefix_offset, 640u, 11u);
    if (ReadSceneCpuMirror(prefix_device, prefix_buffer, prefix_offset, prefix_bytes, 11u, 21u,
                           prefix_copy.data(), nullptr, nullptr)) return EXIT_FAILURE;
    MirrorSceneCpuWrite(prefix_device, prefix_buffer, prefix_source.data() + 640u,
                        prefix_offset + 640u, prefix_bytes - 640u, 11u);
    if (!ReadSceneCpuMirror(prefix_device, prefix_buffer, prefix_offset, prefix_bytes, 11u, 21u,
                            prefix_copy.data(), nullptr, nullptr)
        || prefix_copy != prefix_source) return EXIT_FAILURE;
    InvalidateSceneCpuMirror(prefix_device, prefix_buffer);
    ResetSceneCpuMirrorsForDevice(prefix_device);
    std::printf("PASS RTXDI 80-row initial seed and partial-write coverage at offset 256\n");
  }
  using falcom_world::rtxdi::BuildCameraCandidate;
  using falcom_world::rtxdi::CameraCandidate;
  using falcom_world::rtxdi::CameraMatrix;
  using falcom_world::rtxdi::CameraQualification;
  using falcom_world::rtxdi::CameraQualificationMatches;
  using falcom_world::rtxdi::InvertCameraMatrix;
  using falcom_world::rtxdi::MultiplyCameraMatrices;
  using falcom_world::rtxdi::ObserveCameraProbe;
  using falcom_world::rtxdi::ShouldRunCameraProbe;
  constexpr uint32_t probe_width = 64u, probe_height = 32u;
  std::array<uint8_t, RTXDI_SURFACE_SCENE_ROW_COUNT * 16u> previous_scene = {};
  std::array<uint8_t, RTXDI_SURFACE_SCENE_ROW_COUNT * 16u> current_scene = {};
  CameraMatrix previous_view = {1.0f, 0.0f, 0.0f, 0.0f,
                                0.0f, 1.0f, 0.0f, 0.0f,
                                0.0f, 0.0f, 1.0f, 0.0f,
                                -2.0f, 1.0f, -3.0f, 1.0f};
  CameraMatrix current_view = {1.0f, 0.0f, 0.0f, 0.0f,
                               0.0f, 1.0f, 0.0f, 0.0f,
                               0.0f, 0.0f, 1.0f, 0.0f,
                               -2.5f, 1.25f, -3.0f, 1.0f};
  CameraMatrix previous_projection = {1.2f, 0.0f, 0.0f, 0.0f,
                                      0.0f, 1.4f, 0.0f, 0.0f,
                                      0.00625f, 0.00625f, 1.01f, 1.0f,
                                      0.0f, 0.0f, -0.1f, 0.0f};
  CameraMatrix current_projection = {1.2f, 0.0f, 0.0f, 0.0f,
                                     0.0f, 1.4f, 0.0f, 0.0f,
                                     0.003125f, -0.009375f, 1.01f, 1.0f,
                                     0.0f, 0.0f, -0.1f, 0.0f};
  const auto store_game_matrix = [](auto* scene, uint32_t row, const CameraMatrix& matrix) {
    std::array<float, 16u> packed = {};
    for (uint32_t y = 0u; y < 4u; ++y) {
      for (uint32_t x = 0u; x < 4u; ++x) packed[x * 4u + y] = matrix[y * 4u + x];
    }
    std::memcpy(scene->data() + row * 16u, packed.data(), sizeof(packed));
  };
  const auto store_scene = [&](auto* scene, const CameraMatrix& view, const CameraMatrix& projection,
                               const CameraMatrix& previous_view_projection, std::array<float, 2u> jitter_delta) {
    CameraMatrix view_inverse = {}, projection_inverse = {};
    const CameraMatrix view_projection = MultiplyCameraMatrices(view, projection);
    CameraMatrix view_projection_inverse = {};
    if (!InvertCameraMatrix(view, &view_inverse) || !InvertCameraMatrix(projection, &projection_inverse)
        || !InvertCameraMatrix(view_projection, &view_projection_inverse)) return false;
    store_game_matrix(scene, 0u, view);
    store_game_matrix(scene, 4u, view_inverse);
    store_game_matrix(scene, 8u, projection);
    store_game_matrix(scene, 12u, projection_inverse);
    store_game_matrix(scene, 16u, view_projection);
    store_game_matrix(scene, 20u, view_projection_inverse);
    store_game_matrix(scene, 75u, previous_view_projection);
    const std::array<float, 4u> packed_jitter = {jitter_delta[0], jitter_delta[1], 0.0f, 0.0f};
    std::memcpy(scene->data() + 79u * 16u, packed_jitter.data(), sizeof(packed_jitter));
    return true;
  };
  const CameraMatrix previous_view_projection = MultiplyCameraMatrices(previous_view, previous_projection);
  const CameraMatrix current_view_projection = MultiplyCameraMatrices(current_view, current_projection);
  if (!store_scene(&previous_scene, previous_view, previous_projection, previous_view_projection, {0.0f, 0.0f})
      || !store_scene(&current_scene, current_view, current_projection, previous_view_projection, {0.1f, -0.25f})) return EXIT_FAILURE;
  CameraCandidate camera_candidate;
  if (!BuildCameraCandidate(current_scene.data(), previous_scene.data(), probe_width, probe_height, &camera_candidate)
      || !camera_candidate.matrices_valid || !camera_candidate.absolute_jitter_valid
      || std::abs(camera_candidate.jitter_pixels[0] - 0.1f) > 1.0e-5f
      || std::abs(camera_candidate.jitter_pixels[1] - 0.15f) > 1.0e-5f
      || std::abs(camera_candidate.jitter_pixels_previous[0] - 0.2f) > 1.0e-5f
      || std::abs(camera_candidate.jitter_pixels_previous[1] + 0.1f) > 1.0e-5f
      || std::abs(camera_candidate.motion_correction_candidates[0][0] - 0.1f) > 1.0e-5f
      || std::abs(camera_candidate.motion_correction_candidates[0][1] + 0.25f) > 1.0e-5f
      || std::abs(camera_candidate.motion_correction_candidates[1][0] - 0.2f) > 1.0e-5f
      || std::abs(camera_candidate.motion_correction_candidates[1][1] + 0.5f) > 1.0e-5f
      || camera_candidate.world_to_view[12] != current_view[12]
      || camera_candidate.view_to_clip[8] != 0.0f || camera_candidate.view_to_clip[9] != 0.0f) return EXIT_FAILURE;
  std::array<uint8_t, RTXDI_SURFACE_SCENE_ROW_COUNT * 16u> bad_previous_scene = previous_scene;
  CameraMatrix bad_previous_view_projection = previous_view_projection;
  bad_previous_view_projection[12] += 10.0f;
  store_game_matrix(&bad_previous_scene, 16u, bad_previous_view_projection);
  if (BuildCameraCandidate(current_scene.data(), bad_previous_scene.data(), probe_width, probe_height,
                           &camera_candidate)) return EXIT_FAILURE;
  std::array<uint8_t, RTXDI_SURFACE_SCENE_ROW_COUNT * 16u> singular_camera_scene = current_scene;
  CameraMatrix singular_projection = current_projection;
  singular_projection.fill(0.0f);
  store_game_matrix(&singular_camera_scene, 8u, singular_projection);
  if (BuildCameraCandidate(singular_camera_scene.data(), previous_scene.data(), probe_width, probe_height,
                           &camera_candidate)) return EXIT_FAILURE;
  CameraQualification qualification;
  for (uint32_t frame_index = 0u; frame_index < 3u; ++frame_index) {
    if (ObserveCameraProbe(&qualification, frame_index, 1u, 1u, 7u, probe_width, probe_height, true, true, 0u)) return EXIT_FAILURE;
  }
  if (!ObserveCameraProbe(&qualification, 3u, 1u, 1u, 7u, probe_width, probe_height, true, true, 0u)
      || !CameraQualificationMatches(qualification, 1u, 1u, 7u, probe_width, probe_height, true, true)
      || ShouldRunCameraProbe(qualification, 1u, 1u, 7u, probe_width, probe_height, true, true)
      || !ShouldRunCameraProbe(qualification, 1u, 1u, 7u, probe_width, probe_height, true, false)
      || !ShouldRunCameraProbe(qualification, 1u, 2u, 7u, probe_width, probe_height, true, true)
      || !ShouldRunCameraProbe(qualification, 1u, 1u, 8u, probe_width, probe_height, true, true)
      || !ShouldRunCameraProbe(qualification, 1u, 1u, 7u, probe_width + 1u, probe_height, true, true)) return EXIT_FAILURE;
  const auto skip_path_start = std::chrono::steady_clock::now();
  for (uint32_t index = 0u; index < 1000u; ++index) {
    CameraCandidate repeated_candidate;
    if (!BuildCameraCandidate(current_scene.data(), previous_scene.data(), probe_width, probe_height,
                              &repeated_candidate)
        || ShouldRunCameraProbe(qualification, 1u, 1u, 7u, probe_width, probe_height,
                                repeated_candidate.matrices_valid && repeated_candidate.absolute_jitter_valid, true)) {
      return EXIT_FAILURE;
    }
  }
  const double skip_path_microseconds =
      std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - skip_path_start).count() / 1000.0;
  std::printf("PASS NRD qualified skip path matrix checks average=%.2f us without probe dispatch\n", skip_path_microseconds);
  if (ObserveCameraProbe(&qualification, 4u, 1u, 1u, 7u, probe_width, probe_height, false, true, 0u)
      || qualification.qualified || ObserveCameraProbe(&qualification, 5u, 1u, 1u, 7u, probe_width, probe_height, true, true, 0u)
      || ObserveCameraProbe(&qualification, 6u, 1u, 1u, 8u, probe_width, probe_height, true, true, 1u)
      || ObserveCameraProbe(&qualification, 7u, 1u, 1u, 8u, probe_width, probe_height, true, true, 1u)
      || ObserveCameraProbe(&qualification, 8u, 1u, 1u, 8u, probe_width, probe_height, true, true, 1u)
      || !ObserveCameraProbe(&qualification, 9u, 1u, 1u, 8u, probe_width, probe_height, true, true, 1u)) return EXIT_FAILURE;
  using namespace falcom_world::rtxdi;
  if (CanStartGpuTiming(false, 1000000u, false) || CanStartGpuTiming(true, 0u, false)
      || CanStartGpuTiming(true, 1000000u, true) || !CanStartGpuTiming(true, 1000000u, false)) return EXIT_FAILURE;
  uint32_t accepted_normal = 0u, accepted_zero_lights = 0u, accepted_wait_failure = 0u, rejected = 0u;
  if (ShouldCountAcceptedGpuSubmission(true, true, 7u, UINT64_MAX)) ++accepted_normal;
  if (ShouldCountAcceptedGpuSubmission(true, true, 8u, UINT64_MAX)) ++accepted_zero_lights;
  if (ShouldCountAcceptedGpuSubmission(true, true, 9u, UINT64_MAX)) ++accepted_wait_failure;
  if (accepted_normal != 1u || accepted_zero_lights != 1u || accepted_wait_failure != 1u
      || ShouldCountAcceptedGpuSubmission(true, true, 0u, UINT64_MAX)
      || ShouldCountAcceptedGpuSubmission(true, true, UINT64_MAX, UINT64_MAX)
      || ShouldCountAcceptedGpuSubmission(false, true, 10u, UINT64_MAX)
      || ShouldCountAcceptedGpuSubmission(true, false, 11u, UINT64_MAX)) return EXIT_FAILURE;
  if (ShouldCountAcceptedGpuSubmission(true, true, 0u, UINT64_MAX)) ++rejected;
  if (rejected != 0u) return EXIT_FAILURE;  // Preflight rejection has no accepted B and is never status-counted.
  const uint32_t accepted_producer_b_signals = accepted_normal + accepted_zero_lights;
  uint32_t completed_batches = 0u;
  std::array<uint32_t, 4> completed_interval_samples = {};
  ++completed_interval_samples[0];  // Input interval.
  ++completed_interval_samples[1];  // DI interval in the same readback batch.
  if (!CountCompletedGpuTimingBatch(true, &completed_batches)
      || CountCompletedGpuTimingBatch(false, &completed_batches) || accepted_producer_b_signals != 2u
      || completed_batches != 1u || completed_interval_samples[0] != 1u || completed_interval_samples[1] != 1u)
    return EXIT_FAILURE;
  std::atomic_uint64_t timing_skip_events{0u};
  RecordGpuTimingSkipEvent(&timing_skip_events);
  RecordGpuTimingSkipEvent(&timing_skip_events);  // Two invalid intervals in a partially valid batch.
  if (!CountCompletedGpuTimingBatch(true, &completed_batches) || completed_batches != 2u
      || timing_skip_events.load(std::memory_order_relaxed) != 2u) return EXIT_FAILURE;
  for (uint32_t invalid_interval = 0u; invalid_interval < 4u; ++invalid_interval) {
    RecordGpuTimingSkipEvent(&timing_skip_events);
  }
  RecordGpuTimingSkipEvent(&timing_skip_events);  // Stale identity, unavailable frequency, allocation failure.
  RecordGpuTimingSkipEvent(&timing_skip_events);
  RecordGpuTimingSkipEvent(&timing_skip_events);
  if (CountCompletedGpuTimingBatch(false, &completed_batches) || completed_batches != 2u
      || timing_skip_events.load(std::memory_order_relaxed) != 9u) return EXIT_FAILURE;
  bool perf_pending = true;
  if (ClaimCompletedGpuTiming(&perf_pending, false, 7u, 7u)
      || ClaimCompletedGpuTiming(&perf_pending, true, 8u, 7u) || !perf_pending
      || !ClaimCompletedGpuTiming(&perf_pending, true, 7u, 7u) || perf_pending
      || ClaimCompletedGpuTiming(&perf_pending, true, 7u, 7u)) return EXIT_FAILURE;
  const GpuPerfIdentity perf_identity = {2u, 3u, 4u, 5u, 6u, 7u, 8u, 13u, 9u, 0x0bu};
  if (!MatchesGpuPerfIdentity(perf_identity, perf_identity)) return EXIT_FAILURE;
  std::array<GpuPerfIdentity, 10> stale_perf_identities = {};
  stale_perf_identities.fill(perf_identity);
  ++stale_perf_identities[0].gather_epoch;
  ++stale_perf_identities[1].owner_generation;
  ++stale_perf_identities[2].transfer_configuration;
  ++stale_perf_identities[3].di_configuration;
  ++stale_perf_identities[4].settings_identity;
  ++stale_perf_identities[5].accepted_fence;
  ++stale_perf_identities[6].frame;
  ++stale_perf_identities[7].width;
  ++stale_perf_identities[8].height;
  stale_perf_identities[9].valid_mask ^= 1u;
  for (const GpuPerfIdentity& stale : stale_perf_identities) {
    if (MatchesGpuPerfIdentity(perf_identity, stale)) return EXIT_FAILURE;
  }
  double gpu_ms = -1.0;
  if (!DecodeGpuTimestampPair(100u, 3700u, 1000000u, &gpu_ms) || std::abs(gpu_ms - 3.6) > 1.0e-9
      || DecodeGpuTimestampPair(1u, 1u, 1000000u, &gpu_ms)
      || DecodeGpuTimestampPair(2u, 1u, 1000000u, &gpu_ms)
      || DecodeGpuTimestampPair(1u, 2u, 0u, &gpu_ms)) return EXIT_FAILURE;
  double gpu_sum = 0.0;
  uint32_t gpu_samples = 0u;
  if (!AccumulateGpuSample(1.25, &gpu_sum, &gpu_samples) || !AccumulateGpuSample(0.0, &gpu_sum, &gpu_samples)
      || AccumulateGpuSample(std::numeric_limits<double>::infinity(), &gpu_sum, &gpu_samples)
      || std::abs(gpu_sum - 1.25) > 1.0e-9 || gpu_samples != 2u) return EXIT_FAILURE;
  std::printf("PASS RTXDI performance contracts: producer B signals vs completed batches/interval samples, per-event timing skips, gather gate, completion/poll-once, full identity matrix, timestamp validity, finite accumulation\n");
  return EXIT_SUCCESS;
}
