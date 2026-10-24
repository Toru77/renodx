#pragma once

// RT Sun Shadows (world/docs/ROADMAP.md, RT Sun Shadows): settings, reason codes. Inert while the toggle is off.
// Independent of RTAO, RTGI and RTLS. R3a traces the sun shadow rays only; the image is not changed.

#include <cstdint>

namespace falcom_world::rtsun {

// Settings bindings (Ray Tracing tab, section RT Sun Shadows).
inline float g_rtsun_enabled = 0.f;           // RT Sun Shadows: BOOLEAN row RtSunEnabled
inline float g_rtsun_debug = 0.f;             // RT Sun Debug view: 0 Off, 1 Raw shadow, 2 Pixel classes (3 reserved for the range weight; 4 to 6 reserved for R3c denoise views)
inline float g_rtsun_cascade_range = 2.f;     // Cascade Range: INTEGER 1 to 4, cascades RT owns (capped by the camera's count)
inline float g_rtsun_trace_characters = 1.f;  // Trace characters: character casters block the sun; characters do not receive
inline float g_rtsun_samples = 1.f;           // Samples per pixel
inline float g_rtsun_ray_max = 100.f;         // Max Ray Distance (m)
inline float g_rtsun_light_size = 0.53f;      // Light Size (degrees)
inline float g_rtsun_normal_bias = 0.02f;     // Normal Bias (m)
inline float g_rtsun_isfast = 1.f;            // IS-FAST Noise

enum class Reason : uint8_t { Off = 0, NotSora2nd, InputsMissing, DeferredList, NoCascades, PipelineFailed };

inline const char* ReasonName(Reason reason) {
  switch (reason) {
    case Reason::Off: return "off";
    case Reason::NotSora2nd: return "not_sora2nd";
    case Reason::InputsMissing: return "inputs_missing";
    case Reason::DeferredList: return "deferred_list";
    case Reason::NoCascades: return "no_cascades";
    case Reason::PipelineFailed: return "pipeline_failed";
  }
  return "unknown";
}

inline bool RtSunRequested() { return g_rtsun_enabled > 0.5f; }

}  // namespace falcom_world::rtsun
