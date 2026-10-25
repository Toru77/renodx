#pragma once

// RT Sun Shadows (world/docs/ROADMAP.md, RT Sun Shadows): settings, reason codes. Inert while the toggle is off.
// Independent of RTAO, RTGI and RTLS. R3a traces the sun shadow rays; R3c denoises the traced term (temporal and spatial,
// the shared rtgi chain) and the lighting shader reads the denoised output.

#include <cstdint>

namespace falcom_world::rtsun {

// Settings bindings (Ray Tracing tab, section RT Sun Shadows).
inline float g_rtsun_enabled = 0.f;           // RT Sun Shadows: BOOLEAN row RtSunEnabled
inline float g_rtsun_debug = 0.f;             // RT Sun Debug view: 0 Off, 1 Raw shadow, 2 Pixel classes, 3 unused (reserved for the range weight), 4 Accumulated, 5 History confidence, 6 Final
inline float g_rtsun_cascade_range = 2.f;     // Cascade Range: INTEGER 1 to 4, cascades RT owns (capped by the camera's count)
// Keep Cascade Maps For Fog: 1 keeps the game's cascade maps drawn for the RT-owned cascades (the volumetric fog reads them), 0 skips
// those draws. The sun shadow resolve is skipped either way.
inline float g_rtsun_keep_cascade_maps = 1.f;
inline float g_rtsun_strength = 1.f;          // RT Sun Strength [0..1]: scales the RT sun shadow inside the RT range (R3b); 0 = no RT sun shadow there
inline float g_rtsun_max_darkening = 1.f;     // RT Sun Max Darkening [0..1]: the RT sun darkening is capped at this (R3b)
inline float g_rtsun_trace_characters = 1.f;  // Trace characters: character casters block the sun
inline float g_rtsun_receive_characters = 1.f;      // Receive on characters: character pixels take the RT sun term (R4)
inline float g_rtsun_character_normal_bias = 0.02f; // Character Normal Bias (m): origin offset of character pixels (R4)
inline float g_rtsun_character_intensity = 1.f;    // Character Intensity [0..1]: scales the character RT term; not in the snapshot (R4)
inline float g_rtsun_character_max_darkening = 1.f; // Character Max Darkening [0..1]: hard cap on the character RT darkening (min form); not in the snapshot
inline float g_rtsun_samples = 1.f;           // Samples per pixel
inline float g_rtsun_ray_max = 100.f;         // Max Ray Distance (m)
inline float g_rtsun_light_size = 0.53f;      // Light Size (degrees)
inline float g_rtsun_normal_bias = 0.02f;     // Normal Bias (m)
inline float g_rtsun_isfast = 1.f;            // IS-FAST Noise
inline float g_rtsun_trace_resolution = 1.f;  // Trace Resolution: 0 Full, 1 Half, 2 Quarter
inline float g_rtsun_temporal_enabled = 1.f;  // RT Sun Temporal Accumulation rows
inline float g_rtsun_history_weight = 0.9f;
inline float g_rtsun_depth_rejection = 0.05f;
inline float g_rtsun_normal_rejection = 0.9f;
inline float g_rtsun_history_clamp = 1.f;
inline float g_rtsun_spatial_enabled = 1.f;   // RT Sun Denoising rows
inline float g_rtsun_filter_type = 1.f;       // 0 separable bilateral, 1 a-trous
inline float g_rtsun_filter_radius = 2.f;
inline float g_rtsun_filter_passes = 1.f;
inline float g_rtsun_filter_taps_separable = 5.f;
inline float g_rtsun_filter_taps_atrous = 5.f;
inline float g_rtsun_filter_value_stop = 4.f; // Penumbra Edge Stop k (spatial filter only; not in the snapshot)

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

// ShaderInjection rtsun_flags bits (mirrors RTSUN_FLAG_* in the lighting shader): applied = the RT sun term is bound this draw;
// receive = characters take the RT sun term (R4).
inline constexpr uint32_t kRtSunFlagApplied = 1u;
inline constexpr uint32_t kRtSunFlagReceive = 2u;

// RT sun settings that shape the traced term or its history; a change resets the RT sun history for one frame
// (rtao::TemporalResetReason). Strength, Max Darkening and the spatial filter act after the history and are not part of it.
struct ParameterSnapshot {
  float cascade_range, samples, ray_max, light_size, normal_bias, trace_characters, isfast, trace_resolution;
  float temporal_enabled, history_weight, depth_rejection, normal_rejection, history_clamp, debug;
  float receive_characters, character_normal_bias;
  bool operator==(const ParameterSnapshot&) const = default;
};

}  // namespace falcom_world::rtsun
