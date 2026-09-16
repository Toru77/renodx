/*
 * Copyright (C) 2026 Carlos Lopez
 * SPDX-License-Identifier: MIT
 *
 * Beyond The Falcom Engine: standalone DX11 scene-extraction addon.
 *
 * Deliberately independent from the rasterized `falcomengine` RenoDX addon:
 * no tonemapping, no swapchain upgrades, no shader replacements.
 * Milestone 1: record DrawIndexed calls -> capture one mesh -> OBJ + JSON.
 */

#define ImTextureID ImU64

#define DEBUG_LEVEL_0

#include <deps/imgui/imgui.h>
#include <include/reshade.hpp>

#include "../../utils/scene.hpp"
#include "../../utils/settings.hpp"
#include "./shared.h"

namespace {

BeyondFalcomSettings falcom_settings;

int panel_draw_index = 0;

renodx::utils::settings::Settings settings = {
    new renodx::utils::settings::Setting{
        .key = "SceneCapturePanel",
        .value_type = renodx::utils::settings::SettingValueType::CUSTOM,
        .label = "Scene Capture",
        .section = "Scene Extraction",
        .tooltip = "Capture one DrawIndexed mesh. Output: <game>/renodx-dev/beyondthefalcom/draw_XXXX.obj + .json",
        .on_draw = []() {
          auto* data = renodx::utils::scene::shared.data;
          bool enabled = (data != nullptr) ? data->enabled.load() : true;
          if (ImGui::Checkbox("Scene extraction", &enabled)) {
            if (data != nullptr) data->enabled.store(enabled);
          }
          ImGui::SetItemTooltip("Record lightweight draw metadata (no GPU readbacks until capture).");
          ImGui::InputInt("Draw index", &panel_draw_index);
          if (panel_draw_index < 0) panel_draw_index = 0;
          if (ImGui::Button("Capture draw")) {
            renodx::utils::scene::RequestCapture(static_cast<uint32_t>(panel_draw_index));
          }
          ImGui::SetItemTooltip("Queue a mesh capture for the selected DrawIndexed call. Runs at the next present; the game may hitch briefly during readback.");
          const uint32_t frame = (data != nullptr) ? data->frame_id.load() : 0u;
          const uint32_t draws = (data != nullptr) ? data->last_frame_draws.load() : 0u;
          const uint32_t layouts = (data != nullptr) ? data->layout_count.load() : 0u;
          ImGui::Text("Frame: %u  Draws last frame: %u  Layouts: %u", frame, draws, layouts);
          ImGui::TextWrapped("Status: %s", renodx::utils::scene::GetStatus().c_str());
          return false;
        },
    },
};

void OnPresetOff() {}

bool initialized = false;

}  // namespace

extern "C" __declspec(dllexport) constexpr const char* NAME = "Beyond The Falcom Engine";
extern "C" __declspec(dllexport) constexpr const char* DESCRIPTION = "Beyond The Falcom Engine (DX11 scene extraction)";

BOOL APIENTRY DllMain(HMODULE h_module, DWORD fdw_reason, LPVOID lpv_reserved) {
  (void)lpv_reserved;
  switch (fdw_reason) {
    case DLL_PROCESS_ATTACH:
      if (!reshade::register_addon(h_module)) return FALSE;
      if (!initialized) {
        falcom_settings.capture_draw_index = 0.f;
        initialized = true;
      }
      break;
    case DLL_PROCESS_DETACH:
      break;
  }

  renodx::utils::settings::Use(fdw_reason, &settings, &OnPresetOff);
  renodx::utils::scene::Use(fdw_reason);

  if (fdw_reason == DLL_PROCESS_DETACH) {
    reshade::unregister_addon(h_module);
  }

  return TRUE;
}
