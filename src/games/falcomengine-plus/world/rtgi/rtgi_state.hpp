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
inline float g_rtgi_ray_max = 8.f;
inline float g_rtgi_trace_resolution = 1.f;    // 0 Full, 1 Half, 2 Quarter
inline float g_rtgi_fade_start_pct = 10.f;     // Fade Start: percent of the BVH range (Region)
inline float g_rtgi_fade_end_pct = 20.f;       // Fade End: percent of the BVH range (Region)
inline float g_rtgi_debug = 0.f;               // RTGI Debug (advanced), see the RtgiDebug row
inline float g_rtgi_radiance_clamp = 0.f;  // Radiance Clamp: luminance cap per GI sample (0 = off)
inline float g_rtgi_ambient = 1.f;         // Ambient Contribution: probe ambient weight at hidden and off-screen hits
inline float g_rtgi_sun_distance = 64.f;   // Sun Shadow Distance (advanced): length of the fallback sun shadow ray, metres
inline float g_rtgi_sun_casters_only = 0.f;   // Sun Shadows From Shadow Casters Only: sun rays hit shadow casters
inline float g_rtgi_sun_contribution = 1.f;   // Sun Contribution: scales only RTGI's sun bounce light
inline float g_rtgi_fallback_no_albedo = 0.f;  // Off-screen Hits: No Albedo Guess: fallback hits use albedo 1.0, not 0.5
inline float g_rtgi_dynamic_lights = 0.f;   // Off-screen Dynamic Lights: lamps add light to fallback hits
inline float g_rtgi_dynamic_light_contribution = 1.f;  // Dynamic Light Contribution: scales only RTGI's lamp bounce
inline float g_rtgi_temporal_enabled = 1.f;
inline float g_rtgi_history_weight = 0.95f;
inline float g_rtgi_depth_rejection = 0.05f;
inline float g_rtgi_normal_rejection = 0.9f;
inline float g_rtgi_history_clamp = 1.f;
inline float g_rtgi_spatial_enabled = 1.f;
inline float g_rtgi_filter_type = 1.f;            // Filter Type: 0 separable bilateral, 1 a-trous
inline float g_rtgi_filter_radius = 4.f;
inline float g_rtgi_filter_passes = 2.f;          // Filter Passes: 1..4
inline float g_rtgi_filter_taps_separable = 5.f;  // Filter Taps (separable): 3..17
inline float g_rtgi_filter_taps_atrous = 5.f;     // Filter Taps (a-trous): 3..9
inline float g_rtgi_intensity = 1.f;              // GI Intensity: scale of the bounce added to the image (0 = none)
inline float g_rtgi_char_intensity = 1.f;         // Character GI Intensity: on the character shading path
inline float g_rtgi_trace_characters = 0.f;  // Trace characters: RTGI rays hit this frame's skinned characters
inline float g_rtgi_rtao_restore = 0.f;           // RTAO value to put back when RTGI is turned off (hidden row RtgiRtaoRestore)

enum class Reason : uint8_t {
  Off = 0,
  RtaoOff,           // RTGI on while RTAO is off: RTGI needs RTAO
  RtaoNotProducing,  // RTAO did not dispatch on this lighting draw (its reason is in the RTAO panel)
  PipelineFailed,
  SceneLayoutUnknown,  // the running game's scene-buffer layout is not in RtgiPrevViewProjRegister
};

inline const char* ReasonName(Reason reason) {
  switch (reason) {
    case Reason::Off: return "off";
    case Reason::RtaoOff: return "rtao_off";
    case Reason::RtaoNotProducing: return "rtao_not_producing";
    case Reason::PipelineFailed: return "pipeline_failed";
    case Reason::SceneLayoutUnknown: return "scene_layout_unknown";
  }
  return "unknown";
}

// Scene-buffer layout of a lighting variant (cb_scene, b0 of the game's lighting shader). Between the supported
// layouts only prevViewProj_g moves; viewInv_g c4, viewProj_g c16, viewProjInv_g c20, lightColor_g c25,
// lightDirection_g c26, lightProbe_g c37 and resolutionScaling_g c49 are shared (world_rtgi.cs_5_0.hlsl cb_scene).
// To add a game: add an enumerator, its register below, its exe in addon.cpp RunRtgiInline, and its lighting shader
// to test_rtgi TestCbSceneOffsets. The register must lie in c50..c92 (the shader's g_scene_rows window).
enum class SceneLayout : uint8_t {
  Unknown = 0,  // Kyoto and Daybreak 2 also move lightProbe_g and resolutionScaling_g: not supported
  Sora1st,      // lighting 0xFDAAF80E
  Sora2nd,      // lighting 0xCA3D8596
  Kai,          // lighting 0x430ED091 and lightingsoft 0xF6C55E5F
};

// First register of prevViewProj_g for a layout; 0 = unknown (RTGI does not run).
inline uint32_t RtgiPrevViewProjRegister(SceneLayout layout) {
  switch (layout) {
    case SceneLayout::Sora1st: return 74u;
    case SceneLayout::Sora2nd: return 75u;
    case SceneLayout::Kai: return 85u;
    case SceneLayout::Unknown: break;
  }
  return 0u;
}

// Effective GI ray length. The camera is at least built_range + margin from every region face (bvh_pool.hpp
// kPoolRegionRayMargin), so a pixel inside the effective Fade End keeps built_range + margin - fade_end_eff to the
// nearest face: rays of at most that length never reach the shader's region guard.
inline float RtgiEffectiveRayMax(float requested, float built_range, float fade_end_eff, float margin) {
  return std::min(requested, built_range + margin - fade_end_eff);
}

// RTAO Max Darkening on lighting draws that get the RTGI bounce (addon.cpp push_gi): 1.8x, at most 1. RTAO alone keeps
// its own value.
inline float RtgiRtaoMaxDarkening(float rtao_max_darkening) {
  return std::min(1.f, 1.8f * rtao_max_darkening);
}

// RTGI settings that shape the GI; a change resets the RTGI history for one frame. Never part of RTAO's snapshot.
struct ParameterSnapshot {
  float samples, ray_max, normal_bias, trace_resolution, isfast, fade_start, fade_end, radiance_clamp, ambient;
  float temporal_enabled, history_weight, depth_rejection, normal_rejection, history_clamp, debug, sun_distance;
  float trace_characters;
  float sun_casters_only, sun_contribution, fallback_no_albedo, dynamic_lights, dynamic_light_contribution;
  bool operator==(const ParameterSnapshot&) const = default;
};

}  // namespace falcom_world::rtgi
