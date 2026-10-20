#pragma once

// RTGI (world/docs/ROADMAP.md, RTGI): settings, reason codes, the trace-size and ray-range rules. Inert while the
// RTGI toggle is off. RTGI runs only while RTAO produces: it reuses RTAO's guards, Normal Bias and IS-FAST row.

#include <algorithm>
#include <cstdint>

#include "../rtao/rtao_state.hpp"

namespace falcom_world::rtgi {

// Settings bindings (Ray Tracing tab, section RTGI and its cards).
inline float g_rtgi_enabled = 0.f;
inline float g_rtgi_samples = 1.f;
inline float g_rtgi_ray_max = 16.f;
inline float g_rtgi_trace_resolution = 1.f;    // 0 Full, 1 Half, 2 Quarter
inline float g_rtgi_fade_start_pct = 15.625f;  // Fade Start: percent of the BVH range (Region)
inline float g_rtgi_fade_end_pct = 31.25f;     // Fade End: percent of the BVH range (Region)
inline float g_rtgi_debug = 0.f;               // RTGI Debug (advanced), see the RtgiDebug row
inline float g_rtgi_radiance_clamp = 0.f;  // Radiance Clamp: luminance cap per GI sample (0 = off)
inline float g_rtgi_ambient = 1.f;         // Ambient Contribution: probe ambient weight at hidden and off-screen hits

enum class Reason : uint8_t {
  Off = 0,
  RtaoOff,           // RTGI on while RTAO is off: RTGI needs RTAO
  RtaoNotProducing,  // RTAO did not dispatch on this lighting draw (its reason is in the RTAO panel)
  PipelineFailed,
};

inline const char* ReasonName(Reason reason) {
  switch (reason) {
    case Reason::Off: return "off";
    case Reason::RtaoOff: return "rtao_off";
    case Reason::RtaoNotProducing: return "rtao_not_producing";
    case Reason::PipelineFailed: return "pipeline_failed";
  }
  return "unknown";
}

// Trace Resolution setting (0 Full, 1 Half, 2 Quarter) to the block side d: one trace pixel per d x d pixels.
inline uint32_t RtgiTraceDivisor(float setting) {
  return 1u << std::clamp(static_cast<int>(setting + 0.5f), 0, 2);
}

// Trace pixels along one axis of full pixels (the last block may be partial).
inline uint32_t RtgiTraceExtent(uint32_t full, uint32_t divisor) { return (full + divisor - 1u) / divisor; }

// Effective GI ray length. The camera is at least built_range + margin from every region face (bvh_pool.hpp
// kPoolRegionRayMargin), so a pixel inside the effective Fade End keeps built_range + margin - fade_end_eff to the
// nearest face: rays of at most that length never reach the shader's region guard.
inline float RtgiEffectiveRayMax(float requested, float built_range, float fade_end_eff, float margin) {
  return std::min(requested, built_range + margin - fade_end_eff);
}

}  // namespace falcom_world::rtgi
