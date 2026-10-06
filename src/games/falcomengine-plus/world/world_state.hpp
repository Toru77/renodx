#pragma once

// Phase 0 world-transform research state. DevKit-only, Sora 2nd only.
// Everything here is inert unless falcom_world::Use() activated the module.
//
// Model hierarchy (do not collapse it):
//   VS family -> draw -> mesh (VB/IB) -> instance record -> per-instance world.
// A VS hash is a geometry/material class, never a unique mesh.

#include <array>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <include/reshade.hpp>

#include "../../../utils/scene.hpp"
#include "../../../utils/settings.hpp"

namespace falcom_world {

inline constexpr uint32_t kCbSlotCapacity = 16u;
inline constexpr uint32_t kSrvSlotCapacity = 32u;
inline constexpr uint32_t kCensusRingSize = 4096u;
inline constexpr uint32_t kMaxCbBytes = 64u * 1024u;
inline constexpr uint32_t kMaxCandidates = 64u;
inline constexpr uint32_t kMaxCandidateInstances = 64u;
inline constexpr uint32_t kMaxCbSnapshots = 32u;
inline constexpr uint32_t kMaxSrvSnapshots = 8u;
inline constexpr uint64_t kMaxSrvReadBytes = 4ull * 1024ull * 1024ull;
inline constexpr uint32_t kLightingHashSora2nd = 0xCA3D8596u;
inline constexpr uint32_t kLightingDepthRegisterSora2nd = 4u;
inline constexpr uint32_t kProbeTopCandidates = 8u;
inline constexpr uint32_t kProbeInstances = 4u;
inline constexpr uint32_t kProbeVertices = 32u;
inline constexpr uint32_t kProbeMaxSamples = kProbeInstances * kProbeVertices;
inline constexpr uint32_t kMinDepthMatches = 6u;
inline constexpr uint32_t kProbeRetryBudget = 12u;
inline constexpr uint32_t kNoDrawDeferralWindows = 4u;
inline constexpr uint64_t kAutoDeadlineMarginFrames = 1800u;  // ~30 seconds at 60 fps
inline constexpr uint64_t kAutoCaptureSpacingFrames = 120u; // ~2 seconds at 60 fps
// Deferred (no-draw) families become eligible again after this cooldown so a
// later camera position can observe their draws; retries are bounded.
inline constexpr uint64_t kAutoDeferredRetryCooldownFrames = 600u;  // ~10 seconds at 60 fps
inline constexpr uint32_t kAutoDeferredMaxRetries = 2u;

enum class CandidateKind : uint8_t {
  Cb4x4Row = 0,
  Cb4x4Col,
  Inst4x3Row,
  Inst4x3Col,
  Inst4x4Row,
  Inst4x4Col,
  Count,
};

enum class CandidateSource : uint8_t {
  Generic = 0,
  HintDiagnostic,
  PreferredSignature,
};

inline const char* CandidateSourceName(uint8_t source) {
  switch (static_cast<CandidateSource>(source)) {
    case CandidateSource::HintDiagnostic: return "hint_diagnostic";
    case CandidateSource::PreferredSignature: return "preferred_signature";
    default: return "generic";
  }
}

enum class FamilyLabel : uint8_t {
  Unknown = 0,
  Terrain,
  Buildings,
  Props,
  Glass,
  Character,
  Foliage,
  Transparent,
  Other,
  Count,
};

enum class LabelSource : uint8_t {
  Unknown = 0,
  Hint,
  Manual,
};

struct DrawRecord {
  uint32_t frame = 0u;
  uint32_t serial = 0u;
  uint8_t method = 0u;
  uint32_t vertex_count = 0u;
  uint32_t index_count = 0u;
  uint32_t instance_count = 0u;
  uint32_t first_vertex = 0u;
  uint32_t first_index = 0u;
  int32_t vertex_offset = 0;
  uint32_t first_instance = 0u;
  uint32_t vs_hash = 0u;
  uint32_t ps_hash = 0u;
  // Bound shader objects (pipeline handles); the shader registry classifies
  // them from their bytecode, which is what the pool admits on.
  uint64_t vs_pipeline = 0u;
  uint64_t ps_pipeline = 0u;
  reshade::api::resource vb = {0u};
  uint64_t vb_offset = 0u;
  uint32_t vb_stride = 0u;
  uint64_t vb_size = 0u;
  reshade::api::resource ib = {0u};
  uint64_t ib_offset = 0u;
  uint32_t index_size = 0u;
  uint64_t ib_size = 0u;
  bool has_index_buffer = false;
  reshade::api::pipeline input_layout = {0u};
  reshade::api::primitive_topology topology = reshade::api::primitive_topology::undefined;
  reshade::api::resource_view rtv0 = {0u};
  reshade::api::resource_view dsv = {0u};
  reshade::api::viewport viewport = {};
  bool has_viewport = false;
  bool blend_enable = false;
  bool depth_enable = false;
  bool depth_write = false;
  uint32_t cull_mode = 0u;
  bool has_skin_inputs = false;
  bool is_candidate = false;
  uint32_t vs_cb_mask = 0u;
  uint32_t ps_cb_mask = 0u;
  uint32_t vs_srv_mask = 0u;
  reshade::api::resource vs_cb0 = {0u};
  reshade::api::resource ps_cb0 = {0u};
  reshade::api::resource vs_srv0 = {0u};
  int32_t instance_offset_g = 0;
  bool instance_offset_found = false;
};

struct CbSnapshot {
  uint8_t stage = 0u;  // 1 = vertex, 2 = pixel
  uint32_t slot = 0u;
  std::vector<uint8_t> bytes;
  bool gpu_snapshot = false;
  uint64_t resource_size = 0u;
  uint64_t snapshot_size = 0u;
};

struct SrvBufferSnapshot {
  uint8_t stage = 0u;
  uint32_t slot = 0u;
  reshade::api::resource resource = {0u};
  uint32_t stride = 0u;
  uint64_t size = 0u;
  uint64_t read_offset = 0u;
  uint64_t read_size = 0u;
  std::vector<uint8_t> bytes;
  bool truncated = false;
  bool valid = false;
};

struct CapturedDraw {
  DrawRecord draw;
  std::vector<CbSnapshot> cbs;
  std::vector<SrvBufferSnapshot> srv_buffers;
  bool mesh_valid = false;
  renodx::utils::scene::CapturedMesh mesh;
  std::string mesh_status;
  uint32_t rtv_w = 0u;
  uint32_t rtv_h = 0u;
  int32_t instance_offset_g = 0;
  bool instance_offset_found = false;
};

struct CameraSnapshot {
  bool valid = false;
  uint32_t frame = 0u;
  uint8_t source = 0u;  // 0 = unknown, 1 = cache, 2 = gpu snapshot
  float view[16] = {};
  float view_inv[16] = {};
  float proj[16] = {};
  float view_proj[16] = {};
  float view_proj_inv[16] = {};
  float prev_view_proj[16] = {};
  bool has_prev = false;
};

struct TransformCandidate {
  CandidateKind kind = CandidateKind::Cb4x4Row;
  CandidateSource source = CandidateSource::Generic;
  bool row_dot = false;  // audited row-dot convention (hint diagnostics only)
  uint8_t stage = 0u;
  uint32_t slot = 0u;
  uint32_t matrix_offset = 0u;  // bytes in a CB, or within the element for structured
  uint32_t stride = 0u;         // structured buffer stride (0 for CB)
  uint32_t base_offset = 0u;    // instanceOffset candidate
  uint32_t element_count = 0u;  // instances covered by this candidate
  uint32_t instances_ok = 0u;
  bool has_prev = false;
  float set_score = 0.f;
  float mean_inside = 0.f;
  float bbox_area = 0.f;
  float ndc_z_valid_ratio = 0.f;
  bool projection_valid = false;
  std::vector<float> matrices;       // 12 (4x3) or 16 (4x4) per instance
  std::vector<float> prev_matrices;  // 12 per instance when has_prev
  bool valid = false;
};

struct CandidateSummary {
  CandidateKind kind = CandidateKind::Cb4x4Row;
  CandidateSource source = CandidateSource::Generic;
  uint8_t stage = 0u;
  uint32_t slot = 0u;
  uint32_t matrix_offset = 0u;
  uint32_t stride = 0u;
  uint32_t base_offset = 0u;
  uint32_t element_count = 0u;
  uint32_t instances_ok = 0u;
  bool has_prev = false;
  float set_score = 0.f;
  float mean_inside = 0.f;
  float ndc_z_valid_ratio = 0.f;
};

struct SrvBufferSummary {
  uint8_t slot = 0u;
  uint32_t stride = 0u;
  uint64_t size = 0u;
  uint64_t read_offset = 0u;
  uint64_t read_size = 0u;
  bool truncated = false;
};

struct FamilyStats {
  uint32_t vs_hash = 0u;
  uint64_t draws = 0u;
  uint64_t triangles = 0u;
  uint64_t indices = 0u;
  uint64_t instances = 0u;
  uint32_t depth_write_draws = 0u;
  uint32_t blend_draws = 0u;
  uint32_t candidate_draws = 0u;
  uint32_t skin_draws = 0u;
  std::unordered_set<uint64_t> meshes;
  std::unordered_set<uint32_t> ps_hashes;
  bool candidate = false;
  bool verified = false;
  bool transform_found = false;
  uint8_t label = 0u;
  uint8_t label_source = 0u;
  bool label_prefilled = false;
  // Confirmed transform source, recorded when the family is marked verified.
  uint8_t transform_kind = 0u;
  uint32_t transform_slot = 0u;
  uint32_t transform_offset = 0u;
  uint32_t transform_stride = 0u;
  uint32_t transform_base = 0u;
  bool transform_has_prev = false;
  bool transform_cross_capture = false;
  // Automated runtime verification (decompilation remains a separate step).
  uint8_t auto_verdict = 0u;
  uint32_t captures_attempted = 0u;
  uint32_t captures_ok = 0u;
  float depth_match_ratio = 0.f;
};

struct DepthSource {
  bool valid = false;
  uint32_t frame = UINT32_MAX;
  reshade::api::resource_view view = {0u};
  reshade::api::resource resource = {0u};
  uint32_t width = 0u;
  uint32_t height = 0u;
};

struct ProbeSampleDiag {
  uint32_t instance = 0u;
  float uv[2] = {};
  float expected_linear = 0.f;
  float sampled_linear = 0.f;
  float error = 0.f;
  uint8_t classification = 0u;  // 0 out-of-screen, 1 match, 2 occluded, 3 mismatch
};

struct ProbeMetrics {
  uint32_t samples = 0u;
  uint32_t out_of_screen = 0u;
  uint32_t match = 0u;
  uint32_t occluded = 0u;
  uint32_t mismatch = 0u;
  float match_ratio = 0.f;
  float mean_abs_error = 0.f;
  float mean_expected = 0.f;
  bool valid = false;
  std::vector<ProbeSampleDiag> diag;
};

struct HintDiagnosticRecord {
  bool valid = false;
  CandidateKind kind = CandidateKind::Inst4x3Row;
  uint8_t slot = 0u;
  uint32_t matrix_offset = 0u;
  uint32_t stride = 0u;
  int32_t base = 0;
  bool projection_valid = false;
  float projection_score = 0.f;
  float mean_inside = 0.f;
  float ndc_z_valid_ratio = 0.f;
  uint32_t instance_first = 0u;
  uint32_t instance_count = 0u;
  uint32_t element_first = 0u;
  uint32_t element_count = 0u;
  ProbeMetrics probe;
  bool pass = false;
};

struct HintProbePipelineInfo {
  bool frame_match = false;
  bool candidate_created = false;
  uint32_t candidate_skipped = 0u;
  std::vector<std::string> skip_reasons;
  bool probe_dispatched = false;
  bool probe_readback = false;
  std::string probe_skip_reason;
};

struct CandidateDiagnosticRecord {
  CandidateSource source = CandidateSource::Generic;
  CandidateKind kind = CandidateKind::Cb4x4Row;
  uint8_t stage = 0u;
  uint32_t slot = 0u;
  uint32_t matrix_offset = 0u;
  uint32_t stride = 0u;
  uint32_t base_offset = 0u;
  bool projection_valid = false;
  float projection_score = 0.f;
  float ndc_z_valid_ratio = 0.f;
  uint32_t match = 0u;
  uint32_t occluded = 0u;
  uint32_t mismatch = 0u;
  uint32_t out_of_screen = 0u;
  float match_ratio = 0.f;
  bool pass = false;
  bool selected = false;
};

enum class AutoVerdict : uint8_t {
  Pending = 0,
  Verified,
  VerifiedSingleCapture,
  Failed,
  NotObserved,
};

inline const char* AutoVerdictName(uint8_t verdict) {
  switch (static_cast<AutoVerdict>(verdict)) {
    case AutoVerdict::Verified: return "Verified";
    case AutoVerdict::VerifiedSingleCapture: return "Verified (single capture)";
    case AutoVerdict::Failed: return "Failed";
    case AutoVerdict::NotObserved: return "Not observed";
    default: return "Pending";
  }
}

struct AutoCapture {
  uint32_t frame = 0u;
  uint32_t draw_serial = 0u;
  uint32_t camera_frame = 0u;
  CandidateKind kind = CandidateKind::Cb4x4Row;
  CandidateSource source = CandidateSource::Generic;
  uint8_t stage = 0u;
  uint32_t slot = 0u;
  uint32_t matrix_offset = 0u;
  uint32_t stride = 0u;
  uint32_t base_offset = 0u;
  uint32_t instances_tested = 0u;
  uint32_t instances_ok = 0u;
  uint32_t draw_instance_count = 0u;
  int32_t instance_offset_g = 0;
  bool instance_offset_found = false;
  bool dsv_matches_depth_source = false;
  float projection_score = 0.f;
  float ndc_z_valid_ratio = 0.f;
  ProbeMetrics probe;
  std::vector<float> matrices;       // bounded: up to 4 instances
  std::vector<float> prev_matrices;  // bounded: up to 4 instances when available
  std::vector<HintDiagnosticRecord> hint_diagnostics;
  std::vector<CandidateDiagnosticRecord> candidate_diagnostics;
  std::vector<SrvBufferSummary> srv_buffers;
  bool candidate_valid = false;
  bool depth_ok = false;
  bool pass = false;
  std::string failure_reason;
};

struct PassingLayout {
  CandidateKind kind = CandidateKind::Cb4x4Row;
  uint8_t stage = 0u;
  uint32_t slot = 0u;
  uint32_t matrix_offset = 0u;
  uint32_t stride = 0u;
  uint32_t base_offset = 0u;
  uint32_t count = 0u;
};

struct AutoFamilyResult {
  uint32_t vs_hash = 0u;
  std::vector<AutoCapture> captures;
  uint8_t verdict = 0u;
  std::string failure_reason;
  bool runtime_verified = false;
  bool depth_pass = false;
  bool cross_capture_pass = false;
  std::vector<PassingLayout> passing_layouts;
  uint32_t srv_mask = 0u;
  int32_t instance_offset = 0;
  bool instance_offset_found = false;
  uint32_t read_stride = 0u;
  uint64_t read_offset = 0u;
  uint64_t read_size = 0u;
  std::vector<SrvBufferSummary> srv_buffers;
  std::vector<HintDiagnosticRecord> hint_diagnostics;
  bool pending_at_end = false;
  uint32_t armed_draws = 0u;
  std::string skip_reason;
  bool deferred_no_draw = false;
  uint32_t probe_retries = 0u;
};

struct AutoState {
  bool active = false;
  bool stop_requested = false;
  bool rerun_all = false;
  uint32_t rounds = 3u;  // target captures per family
  std::vector<uint32_t> queue;
  std::vector<uint32_t> pending;
  uint32_t last_vs = 0u;
  uint64_t deadline_frame = 0u;
  uint64_t start_frame = 0u;
  uint64_t next_capture_frame = 0u;
  uint32_t families_done = 0u;
  uint32_t last_processed_serial = 0u;
  std::vector<AutoFamilyResult> results;
  bool finalized = false;
  std::string finalize_reason;
  uint32_t arm_windows = 0u;
  uint32_t arm_set_size = 0u;
  uint32_t pending_at_end = 0u;
  std::unordered_map<uint32_t, uint32_t> armed_draws;
  std::unordered_set<uint32_t> deferred;
  std::unordered_map<uint32_t, uint64_t> deferred_until;
  std::unordered_map<uint32_t, uint32_t> deferred_retries;
  bool round_active = false;
  uint32_t round_target_count = 0u;
  uint32_t round_budget = 0u;
  uint32_t round_windows_used = 0u;
  uint32_t rounds_started = 0u;
  uint32_t windows_retired = 0u;
  std::vector<uint32_t> round_cohort;
  std::unordered_map<uint32_t, uint32_t> round_armed_draws;
  std::unordered_map<uint32_t, uint32_t> probe_retry_counts;
  std::unordered_map<uint32_t, std::unordered_map<uint64_t, uint32_t>> probe_rejected_draws;
  std::unordered_map<uint32_t, uint32_t> probe_retry_totals;
  std::unordered_map<uint32_t, uint32_t> window_armed_draws;
  std::unordered_map<uint32_t, uint32_t> no_draw_windows;
  bool last_capture_blind = false;
  bool last_capture_pass = true;
  bool window_continuation = false;
};

struct State {
  bool supported = false;

  float setting_enabled = 0.f;
  float setting_overlay_mode = 0.f;
  float setting_overlay_stride = 8.f;
  float setting_overlay_show_prev = 0.f;
  float setting_probe_threshold = 0.35f;
  float setting_probe_tol_rel = 0.05f;
  float setting_probe_tol_abs = 0.002f;
  float setting_auto_rounds = 3.f;
  float setting_auto_rerun = 0.f;

  std::atomic<uint32_t> frame{0u};
  std::atomic<uint32_t> next_serial{0u};
  // Set by the BVH module while Pool Scan or a GPU debug view needs draws
  // observed (pool capture, live camera) without the research census.
  std::atomic_bool pool_capture_requested{false};
  uint32_t draw_counter = 0u;

  std::mutex mutex;
  std::vector<DrawRecord> ring;
  uint32_t ring_head = 0u;
  std::unordered_map<uint32_t, FamilyStats> families;
  std::unordered_map<uint64_t, uint64_t> resource_sizes;

  bool arm_active = false;
  bool arm_window_open = false;  // diagnostic observation window for armed draws
  uint32_t arm_vs_hash = 0u;
  uint32_t arm_serial = 0u;
  std::vector<uint32_t> arm_vs_set;  // non-empty = opportunistic multi-family arming

  bool mesh_capture_pending = false;
  bool probe_hint_requested = false;
  uint32_t probe_hint_vs = 0u;
  uint64_t probe_hint_deadline_frame = 0u;
  uint32_t probe_hint_last_serial = 0u;
  CapturedDraw captured;
  CameraSnapshot camera;
  DepthSource depth_source;
  AutoState auto_research;

  std::vector<TransformCandidate> candidates;
  uint32_t selected_family = 0u;
  int selected_candidate = 0;

  std::string status;
};

inline State g_state;

inline bool CensusEnabled() { return g_state.setting_enabled > 0.5f; }

// Draw observation runs when the census or the BVH module needs it.
inline bool CaptureActive() {
  return CensusEnabled() || g_state.pool_capture_requested.load(std::memory_order_relaxed);
}

inline uint32_t OverlayMode() {
  const int mode = static_cast<int>(g_state.setting_overlay_mode + 0.5f);
  return static_cast<uint32_t>(mode < 0 ? 0 : mode);
}

inline uint32_t OverlayStride() {
  const int stride = static_cast<int>(g_state.setting_overlay_stride + 0.5f);
  return static_cast<uint32_t>(stride < 1 ? 1 : (stride > 64 ? 64 : stride));
}

inline bool OverlayShowPrev() { return g_state.setting_overlay_show_prev > 0.5f; }

inline bool IsStructuredKind(CandidateKind kind) {
  return kind == CandidateKind::Inst4x3Row || kind == CandidateKind::Inst4x3Col
         || kind == CandidateKind::Inst4x4Row || kind == CandidateKind::Inst4x4Col;
}

inline bool IsRowVectorKind(CandidateKind kind) {
  switch (kind) {
    case CandidateKind::Cb4x4Row:
    case CandidateKind::Inst4x3Row:
    case CandidateKind::Inst4x4Row:
      return true;
    default:
      return false;
  }
}

inline uint32_t KindMatrixFloats(CandidateKind kind) {
  switch (kind) {
    case CandidateKind::Inst4x3Row:
    case CandidateKind::Inst4x3Col:
      return 12u;
    default:
      return 16u;
  }
}

inline const char* CandidateKindName(CandidateKind kind) {
  switch (kind) {
    case CandidateKind::Cb4x4Row: return "CB 4x4 (row)";
    case CandidateKind::Cb4x4Col: return "CB 4x4 (col)";
    case CandidateKind::Inst4x3Row: return "Instance 4x3 (row)";
    case CandidateKind::Inst4x3Col: return "Instance 4x3 (col)";
    case CandidateKind::Inst4x4Row: return "Instance 4x4 (row)";
    case CandidateKind::Inst4x4Col: return "Instance 4x4 (col)";
    default: return "?";
  }
}

inline const char* FamilyLabelName(uint8_t label) {
  switch (static_cast<FamilyLabel>(label)) {
    case FamilyLabel::Terrain: return "Terrain";
    case FamilyLabel::Buildings: return "Buildings";
    case FamilyLabel::Props: return "Props";
    case FamilyLabel::Glass: return "Glass/Windows";
    case FamilyLabel::Character: return "Character";
    case FamilyLabel::Foliage: return "Foliage";
    case FamilyLabel::Transparent: return "Transparent";
    case FamilyLabel::Other: return "Other";
    default: return "Unknown";
  }
}

inline const char* LabelSourceName(uint8_t source) {
  switch (static_cast<LabelSource>(source)) {
    case LabelSource::Hint: return "hint";
    case LabelSource::Manual: return "manual";
    default: return "unknown";
  }
}

}  // namespace falcom_world
