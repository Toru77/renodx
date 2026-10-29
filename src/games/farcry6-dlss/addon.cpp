/*
 * Copyright (C) 2026
 * SPDX-License-Identifier: MIT
 *
 * Far Cry 6 DLSS — replaces the game's TAA pass with NVIDIA DLSS Super
 * Resolution via the D3D12 NGX path.
 *
 * NOTE: the HLSL files under antialiasing/ and uishaders/ are reference
 * decompilations only. They are intentionally NOT registered as custom
 * shaders; the TAA draw is intercepted at the draw event, DLSS evaluates into
 * the TAA output target, and the original draw is skipped.
 */

#define ImTextureID ImU64

#define DEBUG_LEVEL_0

// gpu_fence uses NTSTATUS definitions that must precede Windows headers.
#include <ntstatus.h>
#define WIN32_NO_STATUS
#include <Windows.h>
#undef WIN32_NO_STATUS

#include <deps/imgui/imgui.h>
#include <include/reshade.hpp>

#include <embed/shaders.h>

#include "../../utils/settings.hpp"
#include "./dlss.hpp"

namespace {

inline const char* SlotState(int i) {
  return farcry6_dlss::dlss::slot_hit[i].load() == 1 ? "ok" : "-";
}

static bool DlssStatusDraw() {
  using namespace farcry6_dlss::dlss;
  const std::string devices = DeviceSummary();
  ImGui::Text("Devices: %s", devices.c_str());
  ImGui::Text(
      "TAA draws: %llu  upscaler evaluations: %llu",
      static_cast<unsigned long long>(stat_taa_draws.load()),
      static_cast<unsigned long long>(stat_dlss_ok.load()));
  ImGui::Text("State: %s", FailCodeText(stat_fail_code.load()));
  ImGui::Text(
      "Slots: b0:%s t0:%s t2:%s t5:%s t6:%s", SlotState(0), SlotState(1), SlotState(2),
      SlotState(3), SlotState(4));
  ImGui::Text(
      "Descriptor fingerprints: %llu",
      static_cast<unsigned long long>(CountFingerprints()));
  if (dlss_bypass_ngx != 0.f) {
    ImGui::TextUnformatted("Inputs: bypass (known-good grey through the output path)");
  } else {
    ImGui::Text(
        "Inputs: colour=%s motion=%s depth=%s",
        dlss_color_source >= 0.5f ? "placeholder" : "game",
        dlss_motion_source >= 0.5f ? "placeholder" : "game",
        dlss_depth_source >= 2.5f ? "placeholder"
            : (dlss_depth_source > 1.5f ? "t6" : (dlss_depth_source > 0.5f ? "t5" : "scene")));
  }
  if (scene_jitter.valid) {
    ImGui::Text(
        "Jitter: scene view constants (%s cb%u+0x%X) raw=(%.6f,%.6f) px=(%.4f,%.4f) moved=%llu/%llu",
        scene_jitter.from_projection ? "projection" : "pair",
        scene_jitter.cb, scene_jitter.offset,
        static_cast<double>(scene_jitter.raw_a), static_cast<double>(scene_jitter.raw_b),
        static_cast<double>(scene_jitter.pixels_x), static_cast<double>(scene_jitter.pixels_y),
        static_cast<unsigned long long>(scene_jitter.moved),
        static_cast<unsigned long long>(scene_jitter.reads));
  } else {
    ImGui::TextUnformatted("Jitter: no scene view constants read yet");
  }
  ImGui::Text(
      "NGX log: %llu lines   hdr=%s   restore bindings=%s   depth transition=%s",
      static_cast<unsigned long long>(ngx_log.seen.load()),
      dlss_flag_is_hdr != 0.f ? "on" : "off",
      dlss_restore_state != 0.f ? "on" : "off",
      dlss_transition_depth != 0.f ? "on" : "off");
  {
    const std::lock_guard<std::mutex> lock(report_mutex);
    ImGui::Text(
        "Last eval: in=%ux%u target=%ux%u mv=%ux%u depth=%ux%u %s",
        deploy_report.input_width, deploy_report.input_height,
        deploy_report.target_width, deploy_report.target_height,
        deploy_report.mv_width, deploy_report.mv_height,
        deploy_report.depth_width, deploy_report.depth_height,
        GetPerfQualityName(deploy_report.quality));
  }
  ImGui::TextWrapped("Probe: %s", BuildProbeReport().c_str());
  return false;
}

renodx::utils::settings::Settings settings = {
    new renodx::utils::settings::Setting{
        .key = "DLSSCopyReport",
        .binding = nullptr,
        .value_type = renodx::utils::settings::SettingValueType::BUTTON,
        .default_value = 0.f,
        .label = "Copy Report",
        .section = "Antialiasing",
        .tooltip = "Copies the live DLSS pipeline state (inputs, sizes, jitter, counters) to the clipboard.",
        .on_click = [] {
          ImGui::SetClipboardText(farcry6_dlss::dlss::BuildDebugReport().c_str());
          return true;
        },
    },
    new renodx::utils::settings::Setting{
        .key = "FGPresentationTest",
        .binding = &farcry6_dlss::dlss::fg_bootstrap_test,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 0.f,
        .label = "Frame Generation Presentation Test",
        .section = "Antialiasing",
        .tooltip = "Tests Streamline presentation with Frame Generation off. Restart the game after changing this setting. Copy Report shows the result.",
        .is_visible = []() { return farcry6_dlss::dlss::IsSupported(); },
    },
    new renodx::utils::settings::Setting{
        .key = "FGInputTest",
        .binding = &farcry6_dlss::dlss::fg_input_test,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 0.f,
        .label = "Frame Generation Input Test",
        .section = "Antialiasing",
        .tooltip = "Tests Streamline camera, depth, motion, and present markers with frame generation off. Requires the presentation test and Halton motion offsets. See Copy Report.",
        .is_visible = []() { return farcry6_dlss::dlss::IsSupported(); },
    },
    new renodx::utils::settings::Setting{
        .key = "FGStartExperiment",
        .binding = nullptr,
        .value_type = renodx::utils::settings::SettingValueType::BUTTON,
        .default_value = 0.f,
        .label = "Start Frame Generation Experiment",
        .section = "Antialiasing",
        .tooltip = "Experimental HDR10 gameplay test. Requires presentation and input tests plus Halton motion offsets. Stop the experiment and confirm FG is off before changing display settings or leaving gameplay.",
        .on_click = [] {
          farcry6_dlss::dlss::fg_enable_requested.store(true);
          return true;
        },
        .is_visible = []() { return farcry6_dlss::dlss::IsSupported(); },
    },
    new renodx::utils::settings::Setting{
        .key = "FGStopExperiment",
        .binding = nullptr,
        .value_type = renodx::utils::settings::SettingValueType::BUTTON,
        .default_value = 0.f,
        .label = "Stop Frame Generation Experiment",
        .section = "Antialiasing",
        .tooltip = "Requests FG off. Copy Report confirms when an off present has completed.",
        .on_click = [] {
          farcry6_dlss::dlss::fg_enable_requested.store(false);
          return true;
        },
        .is_visible = []() { return farcry6_dlss::dlss::IsSupported(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DLSSCpuTiming",
        .binding = &farcry6_dlss::dlss::dlss_cpu_timing,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 0.f,
        .label = "CPU Stutter Probe (test)",
        .section = "Antialiasing",
        .tooltip = "Sample addon callback elapsed times, including waits, for Copy Report. Disable after testing. Counts accumulate until restart.",
        .is_visible = []() { return farcry6_dlss::dlss::IsSupported(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DLSSDetailedDiagnostics",
        .binding = &farcry6_dlss::dlss::dlss_detailed_diagnostics,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 0.f,
        .label = "Detailed Diagnostics",
        .section = "Antialiasing",
        .tooltip = "Collect extra camera and motion details in Copy Report. Leave off during normal play. Capture buttons turn it on when needed.",
        .is_visible = []() { return farcry6_dlss::dlss::IsSupported(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DLSSCameraCoverage",
        .binding = nullptr,
        .value_type = renodx::utils::settings::SettingValueType::BUTTON,
        .default_value = 0.f,
        .label = "Capture Camera Coverage",
        .section = "Antialiasing",
        .tooltip = "Observe camera bindings for four present intervals. Then use Copy Report.",
        .on_click = [] {
          using namespace farcry6_dlss::dlss;
          dlss_detailed_diagnostics = 1.f;
          std::unique_lock lock(camera_coverage.mutex);
          if (camera_coverage.state != CameraCoverage::State::active) {
            camera_coverage.state = CameraCoverage::State::requested;
          }
          return true;
        },
    },
    new renodx::utils::settings::Setting{
        .key = "DLSSHaltonJitter",
        .binding = &farcry6_dlss::dlss::dlss_halton_jitter,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "Halton Jitter",
        .section = "Antialiasing",
        .tooltip = "Experimental: replace the game camera's jitter with a 16-step Halton pattern and send the matching offset to DLSS. Off keeps the game's original jitter. Copy Report shows camera and scene matching.",
        .is_enabled = []() { return farcry6_dlss::dlss::dlss_enabled != 0.f; },
        .is_visible = []() { return farcry6_dlss::dlss::IsSupported(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DLSSHaltonMotionOffsets",
        .binding = &farcry6_dlss::dlss::dlss_halton_motion_offsets,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "Halton Object Motion Offsets (test)",
        .section = "Antialiasing",
        .tooltip = "Experimental: match the game's object motion offsets to Halton jitter when the camera provider is recognized. Copy Report shows patches and scene buffer values.",
        .is_enabled = []() { return farcry6_dlss::dlss::dlss_enabled != 0.f && farcry6_dlss::dlss::dlss_halton_jitter != 0.f; },
        .is_visible = []() { return farcry6_dlss::dlss::IsSupported(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DLSSCaptureCamera",
        .binding = nullptr,
        .value_type = renodx::utils::settings::SettingValueType::BUTTON,
        .default_value = 0.f,
        .label = "Capture Camera Buffer",
        .section = "Antialiasing",
        .tooltip = "Compare a scene camera buffer's CPU and GPU contents once. Then use Copy Report.",
        .on_click = [] {
          using namespace farcry6_dlss::dlss;
          dlss_detailed_diagnostics = 1.f;
          std::unique_lock lock(scene_buffer_capture.mutex);
          if (scene_buffer_capture.state == SceneBufferCapture::State::idle
              || scene_buffer_capture.state == SceneBufferCapture::State::complete
              || scene_buffer_capture.state == SceneBufferCapture::State::failed) {
            scene_buffer_capture.state = SceneBufferCapture::State::requested;
          }
          return true;
        },
    },
    new renodx::utils::settings::Setting{
        .key = "DLSSCaptureMotion",
        .binding = nullptr,
        .value_type = renodx::utils::settings::SettingValueType::BUTTON,
        .default_value = 0.f,
        .label = "Capture Motion Samples",
        .section = "Antialiasing",
        .tooltip = "Captures four synchronized center-pixel motion samples. Hold the camera still, then use Copy Report.",
        .on_click = [] {
          using namespace farcry6_dlss::dlss;
          dlss_detailed_diagnostics = 1.f;
          std::unique_lock lock(motion_capture.mutex);
          if (motion_capture.state == MotionCapture::State::idle
              || motion_capture.state == MotionCapture::State::complete
              || motion_capture.state == MotionCapture::State::failed) {
            motion_capture.state = MotionCapture::State::requested;
          }
          return true;
        },
    },
    new renodx::utils::settings::Setting{
        .key = "DLSSMotionPairingProbe",
        .binding = &farcry6_dlss::dlss::dlss_motion_pair_probe,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 0.f,
        .label = "Motion Pairing Probe",
        .section = "Antialiasing",
        .tooltip = "Experimental diagnostic: checks whether the game motion producer and DLSS use the same texture and command list. Copy Report shows the results.",
        .is_enabled = []() { return farcry6_dlss::dlss::dlss_enabled != 0.f; },
        .is_visible = []() { return farcry6_dlss::dlss::IsSupported(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DLSSObjectMotionCorrection",
        .binding = &farcry6_dlss::dlss::dlss_object_motion_correction,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 0.f,
        .label = "Object Motion Correction (test)",
        .section = "Antialiasing",
        .tooltip = "Experimental Halton object-motion correction. Enable, restart the game, and keep DevKit loaded for this test. Copy Report shows whether it ran.",
        .is_enabled = []() { return farcry6_dlss::dlss::dlss_enabled != 0.f; },
        .is_visible = []() { return farcry6_dlss::dlss::IsSupported(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DLSSObjectMotionStrength",
        .binding = &farcry6_dlss::dlss::dlss_object_motion_strength,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 0.f,
        .label = "Object Motion Correction Strength (test)",
        .section = "Antialiasing",
        .tooltip = "Compare jitter correction strengths on object motion. Changing this resets DLSS history once. Copy Report records the selected mode.",
        .labels = {"+1 (current)", "0 (none)", "-1 (opposite)", "+0.5", "-0.5"},
        .is_enabled = []() { return farcry6_dlss::dlss::dlss_enabled != 0.f && farcry6_dlss::dlss::dlss_object_motion_correction != 0.f; },
        .is_visible = []() { return farcry6_dlss::dlss::IsSupported(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DLSSStatus",
        .binding = nullptr,
        .value_type = renodx::utils::settings::SettingValueType::CUSTOM,
        .default_value = 0.f,
        .label = "Status",
        .section = "Antialiasing",
        .tooltip = "Live DLSS pipeline status. Always visible, even when no NVIDIA device is present.",
        .on_draw = [] { return DlssStatusDraw(); },
    },
    new renodx::utils::settings::Setting{
        .key = "UpscalerType",
        .binding = &farcry6_dlss::dlss::upscaler_type,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 0.f,
        .label = "Upscaler",
        .section = "Antialiasing",
        .tooltip = "Choose NVIDIA DLSS or AMD FSR upscaling. FSR requires AMD's loader and upscaler DLLs next to FarCry6.exe.",
        .labels = {"DLSS", "FSR"},
        .is_visible = []() { return farcry6_dlss::dlss::IsSupported(); },
    },
    new renodx::utils::settings::Setting{
        .key = "FSRVersion",
        .binding = &farcry6_dlss::dlss::fsr_version,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 1.f,
        .label = "FSR Version",
        .section = "Antialiasing",
        .tooltip = "FSR 3.1 is the default for stability. Auto lets AMD's runtime choose, which may pick FSR 4 on supported GPUs. FSR 4 forces FSR 4 if the runtime has it. Changes apply after restart.",
        .labels = {"Auto", "FSR 3.1", "FSR 4"},
        .is_visible = []() { return farcry6_dlss::dlss::IsSupported() && farcry6_dlss::dlss::upscaler_type >= 0.5f; },
    },
    new renodx::utils::settings::Setting{
        .key = "DLSSEnabled",
        .binding = &farcry6_dlss::dlss::dlss_enabled,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "Upscaler Replacement",
        .section = "Antialiasing",
        .tooltip = "Runs the selected upscaler when its inputs are available. DLSS and FSR require their respective runtime DLLs next to the game executable.",
        .is_visible = []() { return farcry6_dlss::dlss::IsSupported(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DLSSOutputMode",
        .binding = &farcry6_dlss::dlss::dlss_output_mode,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 0.f,
        .label = "Upscaler Output",
        .section = "Antialiasing",
        .tooltip = "Replace uses the selected upscaler output. Evaluate only runs the upscaler but keeps the game's own TAA output. Resolve only checks the inputs without running an upscaler.",
        .labels = {"Replace", "Evaluate only", "Resolve only"},
        .is_enabled = []() { return farcry6_dlss::dlss::dlss_enabled != 0.f; },
        .is_visible = []() { return farcry6_dlss::dlss::IsSupported(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DLSSPreset",
        .binding = &farcry6_dlss::dlss::dlss_render_preset,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 1.f,
        .label = "DLSS Preset",
        .section = "Antialiasing",
        .labels = {"Default", "F - CNN", "J - Transformer 1", "K - Transformer 1", "L - Transformer 2", "M - Transformer 2"},
        .is_enabled = []() { return farcry6_dlss::dlss::dlss_enabled != 0.f; },
        .is_visible = []() { return farcry6_dlss::dlss::IsSupported() && farcry6_dlss::dlss::upscaler_type < 0.5f; },
    },
    new renodx::utils::settings::Setting{
        .key = "DLSSJitterSource",
        .binding = &farcry6_dlss::dlss::dlss_jitter_source,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 3.f,
        .label = "Jitter Source",
        .section = "Antialiasing",
        .tooltip = "What DLSS is told about the game's render jitter. Zero passes none. Extracted reads the scene geometry passes' own view constants - the draws that apply the jitter - preferring the projection that carries an offset and falling back to the sub-pixel pair that moves between frames; it is read, never inferred from a value's size. Cycled pattern is synthetic. TAA temporal buffer centers the observed JitterParameters xy values as an experimental per-frame source; use Flip Jitter Sign to compare conventions.",
        .labels = {"Zero", "Extracted", "Cycled pattern (synthetic)", "TAA temporal buffer (test)"},
        .is_enabled = []() { return farcry6_dlss::dlss::dlss_enabled != 0.f; },
        .is_visible = []() { return farcry6_dlss::dlss::IsSupported(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DLSSDepthSource",
        .binding = &farcry6_dlss::dlss::dlss_depth_source,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 0.f,
        .label = "Depth Source",
        .section = "Antialiasing",
        .tooltip = "What feeds DLSS as depth. Scene depth is the game's own depth buffer, identified by how the engine binds it. t5/t6 are the TAA anti-ghosting masks, which are not depth. Placeholder feeds a far plane, so nothing occludes anything: a valid input with neutral content, for isolating a rendering fault.",
        .labels = {"Scene depth", "t5 (mask)", "t6 (mask)", "Placeholder (far plane)"},
        .is_enabled = []() { return farcry6_dlss::dlss::dlss_enabled != 0.f; },
        .is_visible = []() { return farcry6_dlss::dlss::IsSupported(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DLSSColourSource",
        .binding = &farcry6_dlss::dlss::dlss_color_source,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 0.f,
        .label = "Colour Source",
        .section = "Antialiasing",
        .tooltip = "What feeds DLSS as colour. Placeholder substitutes a flat mid grey of the input size, so DLSS runs normally on input that cannot contain scene content. If artifacts disappear with it, the game's colour is implicated; if they survive, they are not coming from the colour.",
        .labels = {"Game (t2)", "Placeholder (grey)"},
        .is_enabled = []() { return farcry6_dlss::dlss::dlss_enabled != 0.f; },
        .is_visible = []() { return farcry6_dlss::dlss::IsSupported(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DLSSMotionSource",
        .binding = &farcry6_dlss::dlss::dlss_motion_source,
        .value_type = renodx::utils::settings::SettingValueType::INTEGER,
        .default_value = 0.f,
        .label = "Motion Source",
        .section = "Antialiasing",
        .tooltip = "What feeds DLSS as motion vectors. Placeholder substitutes zero motion at the input size, with the jittered-motion flag dropped to match. If artifacts disappear with it, the decoded game motion is implicated.",
        .labels = {"Game (decoded)", "Placeholder (zero motion)"},
        .is_enabled = []() { return farcry6_dlss::dlss::dlss_enabled != 0.f; },
        .is_visible = []() { return farcry6_dlss::dlss::IsSupported(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DLSSBypassNgx",
        .binding = &farcry6_dlss::dlss::dlss_bypass_ngx,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 0.f,
        .label = "Bypass NGX",
        .section = "Antialiasing",
        .tooltip = "Writes a known-good flat grey through the same transitions and copy the DLSS path uses, with NGX not involved at all, and skips the game's TAA draw. If the artifacts survive this, they are ours rather than DLSS's. Overrides DLSS Output and all input sources.",
        .is_enabled = []() { return farcry6_dlss::dlss::dlss_enabled != 0.f; },
        .is_visible = []() { return farcry6_dlss::dlss::IsSupported(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DLSMVJittered",
        .binding = &farcry6_dlss::dlss::dlss_motion_vectors_jittered,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 0.f,
        .label = "Motion Vectors Jittered",
        .section = "Antialiasing",
        .tooltip = "Tell DLSS the motion vectors span jittered frames (the scene is rendered jittered). Toggle for A/B testing if motion looks wrong.",
        .is_enabled = []() { return farcry6_dlss::dlss::dlss_enabled != 0.f; },
        .is_visible = []() { return farcry6_dlss::dlss::IsSupported(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DLSSDepthInverted",
        .binding = &farcry6_dlss::dlss::dlss_depth_inverted,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "Depth Inverted",
        .section = "Antialiasing",
        .tooltip = "Tell DLSS the depth buffer is reverse-Z. Toggle for A/B testing if disocclusion handling looks wrong.",
        .is_enabled = []() { return farcry6_dlss::dlss::dlss_enabled != 0.f; },
        .is_visible = []() { return farcry6_dlss::dlss::IsSupported(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DLSSRestoreState",
        .binding = &farcry6_dlss::dlss::dlss_restore_state,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "Restore Bindings After DLSS",
        .section = "Antialiasing",
        .tooltip = "Re-applies the game's descriptor heaps and root signature after every DLSS evaluation. NGX dispatches on the command list directly, behind ReShade's hooks, and the descriptor heap binding is shared between compute and graphics, so the game's following draws would otherwise read their descriptors out of NGX's heap. Turn this off only to A/B whether it is the difference.",
        .is_enabled = []() { return farcry6_dlss::dlss::dlss_enabled != 0.f; },
        .is_visible = []() { return farcry6_dlss::dlss::IsSupported(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DLSSTransitionDepth",
        .binding = &farcry6_dlss::dlss::dlss_transition_depth,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "Transition Depth For DLSS",
        .section = "Antialiasing",
        .tooltip = "Moves the scene depth into a shader-readable state for the DLSS read and restores the state the engine had it in. The engine leaves it in a depth-stencil state, where a shader read returns zeros - a depth view that is black in DLSS's debug overlay. Turn this off to see exactly that.",
        .is_enabled = []() { return farcry6_dlss::dlss::dlss_enabled != 0.f; },
        .is_visible = []() { return farcry6_dlss::dlss::IsSupported(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DLSSFlagIsHdr",
        .binding = &farcry6_dlss::dlss::dlss_flag_is_hdr,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 1.f,
        .label = "Input Is HDR",
        .section = "Antialiasing",
        .tooltip = "Declares the colour input as HDR, meaning values above 1.0. Far Cry 6's TAA target is 16-bit float linear, which suggests it is, but every previous build ran with this off, so A/B it rather than assume.",
        .is_enabled = []() { return farcry6_dlss::dlss::dlss_enabled != 0.f; },
        .is_visible = []() { return farcry6_dlss::dlss::IsSupported(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DLSSMVScale",
        .binding = &farcry6_dlss::dlss::dlss_mv_scale,
        .default_value = 1.f,
        .label = "Motion Vector Scale",
        .section = "Antialiasing",
        .tooltip = "Calibration multiplier for the decoded motion vectors. 1.0 is nominal (decode x 0.1 x resolution).",
        .min = 0.1f,
        .max = 5.f,
        .is_enabled = []() { return farcry6_dlss::dlss::dlss_enabled != 0.f; },
        .is_visible = []() { return farcry6_dlss::dlss::IsSupported(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DLSSResetEveryFrame",
        .binding = &farcry6_dlss::dlss::dlss_reset_every_frame,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 0.f,
        .label = "Reset Every Frame",
        .section = "Antialiasing",
        .tooltip = "Tells DLSS to ignore its history on every evaluation. If the image looks identical with this on, the history was already being discarded each frame and the fault is upstream of it. If it looks worse, history is accumulating and the problem is what it accumulates from.",
        .is_enabled = []() { return farcry6_dlss::dlss::dlss_enabled != 0.f; },
        .is_visible = []() { return farcry6_dlss::dlss::IsSupported(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DLSSMotionVectorsFlipped",
        .binding = &farcry6_dlss::dlss::dlss_mv_flip,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 0.f,
        .label = "Flip Motion Vector Sign",
        .section = "Antialiasing",
        .tooltip = "Negates the decoded motion vectors. A flipped sign misreprojects as badly as a wrong scale, and this is the only way to test it: the scale setting has no negative values. If the image holds still and resolves detail with this on, the sign was wrong.",
        .is_enabled = []() { return farcry6_dlss::dlss::dlss_enabled != 0.f; },
        .is_visible = []() { return farcry6_dlss::dlss::IsSupported(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DLSSJitterFlip",
        .binding = &farcry6_dlss::dlss::dlss_jitter_flip,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 0.f,
        .label = "Flip Jitter Sign",
        .section = "Antialiasing",
        .tooltip = "Negates the original game jitter handed to DLSS. Halton jitter ignores this setting and uses its own sign.",
        .is_enabled = []() { return farcry6_dlss::dlss::dlss_enabled != 0.f; },
        .is_visible = []() { return farcry6_dlss::dlss::IsSupported(); },
    },
    new renodx::utils::settings::Setting{
        .key = "DLSSDebugLogging",
        .binding = &farcry6_dlss::dlss::dlss_debug_logging,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 0.f,
        .label = "Debug Logging",
        .section = "Antialiasing",
        .tooltip = "Log DLSS evaluation parameters and jitter detection to ReShade.log (throttled).",
        .is_enabled = []() { return farcry6_dlss::dlss::dlss_enabled != 0.f; },
        .is_visible = []() { return farcry6_dlss::dlss::IsSupported(); },
    },
    new renodx::utils::settings::Setting{
        .key = "FSRDebugLogging",
        .binding = &farcry6_dlss::dlss::fsr_debug_logging,
        .value_type = renodx::utils::settings::SettingValueType::BOOLEAN,
        .default_value = 0.f,
        .label = "FSR Debug Logging",
        .section = "Antialiasing",
        .tooltip = "Logs the whole addon path (device, inputs, FSR, present, Streamline) to ReShade.log. Set before launching the game.",
        .is_visible = []() { return farcry6_dlss::dlss::IsSupported() && farcry6_dlss::dlss::upscaler_type >= 0.5f; },
    },
};

void OnPresetOff() {}

bool initialized = false;

}  // namespace

extern "C" __declspec(dllexport) constexpr const char* NAME = "RenoDX - Far Cry 6 DLSS";
extern "C" __declspec(dllexport) constexpr const char* DESCRIPTION = "Replaces Far Cry 6 TAA with NVIDIA DLSS Super Resolution";

BOOL APIENTRY DllMain(HMODULE h_module, DWORD fdw_reason, LPVOID lpv_reserved) {
  switch (fdw_reason) {
    case DLL_PROCESS_ATTACH:
      if (!reshade::register_addon(h_module)) return FALSE;
      initialized = true;
      break;
    case DLL_PROCESS_DETACH:
      reshade::unregister_addon(h_module);
      break;
  }

  renodx::utils::settings::Use(fdw_reason, &settings, &OnPresetOff);
  farcry6_dlss::dlss::Use(fdw_reason);

  return TRUE;
}
