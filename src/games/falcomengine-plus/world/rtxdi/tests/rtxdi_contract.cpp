#include "../rtxdi_contract.h"
#include "../../bvh/alpha_atlas.hpp"
#include "../rtxdi_config.hpp"
#include "../rtxdi_di_contract.h"
#include "../rtxdi_input_validation.hpp"
#include "../rtxdi_state.hpp"
#include "../rtxdi_surface_layout.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <limits>

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

std::array<float, 4> TransformRowVector(const std::array<float, 4>& vector, const SurfaceSceneRows& rows, uint32_t first_row) {
  std::array<float, 4> result = {};
  for (uint32_t column = 0u; column < 4u; ++column) {
    for (uint32_t row = 0u; row < 4u; ++row) result[column] += vector[row] * SceneFloat(rows, first_row + row, column);
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
  const auto world_h = TransformRowVector({u * 2.f - 1.f, 1.f - v * 2.f, raw_depth, 1.f}, scene_rows,
                                          RTXDI_SURFACE_INVERSE_VIEW_PROJECTION_ROW);
  if (!std::isfinite(world_h[3]) || std::abs(world_h[3]) <= 1e-8f) return reject(RTXDI_SURFACE_REJECT_HOMOGENEOUS_W);
  if (!std::isfinite(world_h[0]) || !std::isfinite(world_h[1]) || !std::isfinite(world_h[2])) {
    return reject(RTXDI_SURFACE_REJECT_POSITION);
  }
  for (uint32_t axis = 0u; axis < 3u; ++axis) result.world_position[axis] = world_h[axis] / world_h[3];
  if (!std::isfinite(result.world_position[0]) || !std::isfinite(result.world_position[1])
      || !std::isfinite(result.world_position[2])) return reject(RTXDI_SURFACE_REJECT_POSITION);

  const std::array<float, 3> camera = {
      SceneFloat(scene_rows, RTXDI_SURFACE_INVERSE_VIEW_ROW + 3u, 0u),
      SceneFloat(scene_rows, RTXDI_SURFACE_INVERSE_VIEW_ROW + 3u, 1u),
      SceneFloat(scene_rows, RTXDI_SURFACE_INVERSE_VIEW_ROW + 3u, 2u)};
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

  const auto view_position = TransformRowVector({result.world_position[0], result.world_position[1], result.world_position[2], 1.f},
                                                scene_rows, RTXDI_SURFACE_VIEW_ROW);
  result.linear_view_depth = -view_position[2];
  if (!std::isfinite(result.linear_view_depth) || result.linear_view_depth <= 0.f) return reject(RTXDI_SURFACE_REJECT_VIEW_DEPTH);
  result.flags = RTXDI_SURFACE_FLAG_VALID;
  if ((mrt0[3] & 8u) == 0u) result.flags |= RTXDI_SURFACE_FLAG_CHARACTER;
  return result;
}

SurfaceSceneRows MakeSurfaceSceneRows() {
  SurfaceSceneRows rows = {};
  SetSceneRow(&rows, RTXDI_SURFACE_VIEW_ROW + 0u, {0.f, 0.f, -1.f, 0.f});
  SetSceneRow(&rows, RTXDI_SURFACE_VIEW_ROW + 1u, {0.f, 1.f, 0.f, 0.f});
  SetSceneRow(&rows, RTXDI_SURFACE_VIEW_ROW + 2u, {1.f, 0.f, 0.f, 0.f});
  SetSceneRow(&rows, RTXDI_SURFACE_VIEW_ROW + 3u, {-10.f, -2.f, 3.f, 1.f});
  SetSceneRow(&rows, RTXDI_SURFACE_INVERSE_VIEW_ROW + 3u, {3.f, 2.f, 10.f, 1.f});
  SetSceneRow(&rows, RTXDI_SURFACE_INVERSE_VIEW_PROJECTION_ROW + 0u, {1.f, 0.f, 0.f, 0.f});
  SetSceneRow(&rows, RTXDI_SURFACE_INVERSE_VIEW_PROJECTION_ROW + 1u, {0.f, 1.f, 0.f, 0.f});
  SetSceneRow(&rows, RTXDI_SURFACE_INVERSE_VIEW_PROJECTION_ROW + 2u, {0.f, 0.f, -4.f, 0.f});
  SetSceneRow(&rows, RTXDI_SURFACE_INVERSE_VIEW_PROJECTION_ROW + 3u, {5.f, 2.f, 5.f, 1.f});
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
  using falcom_world::rtxdi::CanReuseDiOutput;
  using falcom_world::rtxdi::CheckDiOutputIdentity;
  using falcom_world::rtxdi::CheckDiSubmissionIdentity;
  using falcom_world::rtxdi::DiOutputGate;
  using falcom_world::rtxdi::DiOutputIdentity;
  using falcom_world::rtxdi::DiPostSubmitSignalDisposition;
  using falcom_world::rtxdi::DiSignalDisposition;
  using falcom_world::rtxdi::DiSubmissionGate;
  using falcom_world::rtxdi::DiSubmissionIdentity;
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
  if (CheckDiOutputIdentity(staged_output) != DiOutputGate::Ready
      || CheckDiOutputIdentity(before_output_b) != DiOutputGate::ProducerPending
      || CheckDiOutputIdentity(stale_output) != DiOutputGate::StaleIdentity
      || CheckDiOutputIdentity(stale_output_frame) != DiOutputGate::StaleIdentity
      || CheckDiOutputIdentity(stale_output_scene) != DiOutputGate::StaleIdentity
      || CheckDiOutputIdentity(stale_output_transfer) != DiOutputGate::StaleIdentity
      || CheckDiOutputIdentity(resize_output) != DiOutputGate::ConfigurationMismatch
      || CheckDiOutputIdentity(configuration_output) != DiOutputGate::ConfigurationMismatch
      || CheckDiOutputIdentity(not_staged_output) != DiOutputGate::NotStaged
      || CanReuseDiOutput(true, 0u, 100u) || CanReuseDiOutput(true, 27u, 26u)
      || CanReuseDiOutput(true, 27u, std::numeric_limits<uint64_t>::max())
      || !CanReuseDiOutput(true, 27u, 27u) || !CanReuseDiOutput(false, 0u, 0u)) return EXIT_FAILURE;

  using falcom_world::AlphaUvRangeStatus;
  using falcom_world::rtxdi::CanReuseScene;
  using falcom_world::rtxdi::IsDuplicate;
  using falcom_world::rtxdi::SceneReuseInputs;
  using falcom_world::rtxdi::SceneSnapshot;
  using falcom_world::rtxdi::SunVisibilityStatus;
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
  const DiResourceIdentity installed = {.config = config, .owner_generation = 4u, .configuration_generation = 7u, .present = true, .complete = true};
  if (SelectDiResourceAction(false, 4u, {}, config) != DiResourceAction::Skip
      || SelectDiResourceAction(true, 4u, {}, config) != DiResourceAction::Create
      || SelectDiResourceAction(true, 4u, installed, config) != DiResourceAction::Reuse
      || SelectDiResourceAction(true, 5u, installed, config) != DiResourceAction::OwnerMismatch
      || SelectDiResourceAction(true, 0u, installed, config) != DiResourceAction::OwnerMismatch) return EXIT_FAILURE;
  auto resize_request = config;
  resize_request.width = 1920u;
  if (SelectDiResourceAction(true, 4u, installed, resize_request) != DiResourceAction::Create) return EXIT_FAILURE;
  auto incomplete = installed;
  incomplete.complete = false;
  if (SelectDiResourceAction(true, 4u, incomplete, config) != DiResourceAction::Create
      || SelectDiResourceAction(true, 5u, incomplete, config) != DiResourceAction::OwnerMismatch) return EXIT_FAILURE;
  auto uncommitted = installed;
  for (int i = 0; i < 2; ++i) {
    if (SelectDiResourceAction(true, 4u, uncommitted, resize_request) != DiResourceAction::Create
        || uncommitted.owner_generation != 4u || uncommitted.configuration_generation != 7u) return EXIT_FAILURE;
  }
  uncommitted.configuration_generation = 0u;
  if (SelectDiResourceAction(true, 4u, uncommitted, config) != DiResourceAction::Create) return EXIT_FAILURE;
  uint64_t next_generation = 0u;
  if (!falcom_world::rtxdi::NextDiConfigurationGeneration(installed.configuration_generation, &next_generation)
      || next_generation != 8u || installed.owner_generation != 4u || installed.configuration_generation != 7u
      || falcom_world::rtxdi::NextDiConfigurationGeneration(UINT64_MAX, &next_generation)) return EXIT_FAILURE;
  auto context = falcom_world::rtxdi::CreateDiContext(config);
  if (!context || context->GetStaticParameters().CheckerboardSamplingMode != rtxdi::CheckerboardMode::Off
      || context->GetResamplingMode() != rtxdi::ReSTIRDI_ResamplingMode::TemporalAndSpatial
      || context->GetFrameIndex() != 0u) return EXIT_FAILURE;
  if (!falcom_world::rtxdi::ValidTemporalSpatialBufferIndices(context->GetBufferIndices())) return EXIT_FAILURE;
  const auto parameters = falcom_world::rtxdi::MakeDiFrameParameters(config, *context);
  if (parameters.width != 1280u || parameters.height != 720u || parameters.history_valid != 0u
      || parameters.flags != RTXDI_FRAME_FLAG_SPATIAL_AFTER_INITIAL
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
  RtxdiFrameUpload upload = {};
  upload.parameters = parameters;
  for (const auto byte : upload.padding)
    if (byte != 0u) return EXIT_FAILURE;
  falcom_world::rtxdi::DiConfig resized;
  if (!falcom_world::rtxdi::CalculateDiConfig(17u, 33u, &resized)
      || resized.array_pitch != 1536u || resized.reservoir_bytes != 110592u
      || falcom_world::rtxdi::CreateDiContext({}) != nullptr) return EXIT_FAILURE;
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
      || ValidStructuredInput({.buffer_bytes = 1120u, .first_element = 3u, .element_count = 7u, .stride = 144u}, 112u)
      || ValidStructuredInput({.buffer_bytes = 4096u, .first_element = UINT32_MAX, .element_count = UINT32_MAX, .stride = 112u}, 112u)
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
  promoted.size = 1792u;
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
  for (uint32_t row = 0u; row < 4u; ++row) {
    auto values = std::array<float, 4>{SceneFloat(invalid_surface_rows, RTXDI_SURFACE_INVERSE_VIEW_PROJECTION_ROW + row, 0u),
                                       SceneFloat(invalid_surface_rows, RTXDI_SURFACE_INVERSE_VIEW_PROJECTION_ROW + row, 1u),
                                       SceneFloat(invalid_surface_rows, RTXDI_SURFACE_INVERSE_VIEW_PROJECTION_ROW + row, 2u), 0.f};
    SetSceneRow(&invalid_surface_rows, RTXDI_SURFACE_INVERSE_VIEW_PROJECTION_ROW + row, values);
  }
  if (!expect_surface_rejection(0.5f, negative_z_non_character, 0u, 0u, 2u, 2u, invalid_surface_rows,
                                RTXDI_SURFACE_REJECT_HOMOGENEOUS_W)) return EXIT_FAILURE;
  invalid_surface_rows = surface_rows;
  SetSceneRow(&invalid_surface_rows, RTXDI_SURFACE_INVERSE_VIEW_PROJECTION_ROW + 3u,
              {5.f, 2.f, 5.f, std::numeric_limits<float>::quiet_NaN()});
  if (!expect_surface_rejection(0.5f, negative_z_non_character, 0u, 0u, 2u, 2u, invalid_surface_rows,
                                RTXDI_SURFACE_REJECT_HOMOGENEOUS_W)) return EXIT_FAILURE;
  invalid_surface_rows = surface_rows;
  SetSceneRow(&invalid_surface_rows, RTXDI_SURFACE_INVERSE_VIEW_PROJECTION_ROW + 3u,
              {std::numeric_limits<float>::infinity(), 2.f, 5.f, 1.f});
  if (!expect_surface_rejection(0.5f, negative_z_non_character, 0u, 0u, 2u, 2u, invalid_surface_rows,
                                RTXDI_SURFACE_REJECT_POSITION)) return EXIT_FAILURE;
  invalid_surface_rows = surface_rows;
  SetSceneRow(&invalid_surface_rows, RTXDI_SURFACE_INVERSE_VIEW_PROJECTION_ROW + 3u, {3e38f, 2.f, 5.f, 1e-7f});
  if (!expect_surface_rejection(0.5f, negative_z_non_character, 0u, 0u, 2u, 2u, invalid_surface_rows, RTXDI_SURFACE_REJECT_POSITION)) return EXIT_FAILURE;
  invalid_surface_rows = surface_rows;
  SetSceneRow(&invalid_surface_rows, RTXDI_SURFACE_INVERSE_VIEW_ROW + 3u,
              {std::numeric_limits<float>::infinity(), 2.f, 10.f, 1.f});
  if (!expect_surface_rejection(0.5f, negative_z_non_character, 0u, 0u, 2u, 2u, invalid_surface_rows,
                                RTXDI_SURFACE_REJECT_VIEW_DIRECTION)) return EXIT_FAILURE;
  invalid_surface_rows = surface_rows;
  SetSceneRow(&invalid_surface_rows, RTXDI_SURFACE_INVERSE_VIEW_ROW + 3u, {4.5f, 2.5f, 3.f, 1.f});
  if (!expect_surface_rejection(0.5f, negative_z_non_character, 0u, 0u, 2u, 2u, invalid_surface_rows,
                                RTXDI_SURFACE_REJECT_VIEW_DIRECTION)) return EXIT_FAILURE;
  invalid_surface_rows = surface_rows;
  SetSceneRow(&invalid_surface_rows, RTXDI_SURFACE_VIEW_ROW + 3u, {-10.f, -2.f, 5.f, 1.f});
  if (!expect_surface_rejection(0.5f, negative_z_non_character, 0u, 0u, 2u, 2u, invalid_surface_rows,
                                RTXDI_SURFACE_REJECT_VIEW_DEPTH)) return EXIT_FAILURE;
  invalid_surface_rows = surface_rows;
  SetSceneRow(&invalid_surface_rows, RTXDI_SURFACE_VIEW_ROW + 3u,
              {-10.f, -2.f, std::numeric_limits<float>::infinity(), 1.f});
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
  return EXIT_SUCCESS;
}
