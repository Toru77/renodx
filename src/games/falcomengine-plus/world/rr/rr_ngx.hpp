#pragma once

// DLSS Ray Reconstruction (world/rr): detours on the NGX core (_nvngx.dll) D3D11 exports. The game links the NGX SDK
// statically, so its NVSDK_NGX_D3D11_* calls land in the driver's _nvngx.dll.

#include <Windows.h>
#include <d3d11.h>
#include <d3d12.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <cwchar>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <excpt.h>
#include <filesystem>
#include <optional>
#include <shared_mutex>
#include <string_view>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <nvsdk_ngx.h>
#include <nvsdk_ngx_defs_dlssd.h>
#include <include/reshade.hpp>

#include "../../../../utils/detour.hpp"
#include "../../../../utils/directx.hpp"
#include "../../../../utils/log.hpp"
#include "../../../../utils/platform.hpp"
#include "../world_state.hpp"
#include "rr_guides.hpp"
#include "rr_state.hpp"

namespace falcom_world::rr {

inline decltype(&NVSDK_NGX_D3D11_CreateFeature) g_real_create_feature = nullptr;
inline decltype(&NVSDK_NGX_D3D11_EvaluateFeature) g_real_evaluate_feature = nullptr;
inline decltype(&NVSDK_NGX_D3D11_ReleaseFeature) g_real_release_feature = nullptr;
// nvsdk_ngx.h declares NVSDK_NGX_D3D11_Shutdown only under NGX_ENABLE_DEPRECATED_SHUTDOWN; the driver still exports it.
using NgxD3D11ShutdownFn = NVSDK_NGX_Result(NVSDK_CONV*)();
inline NgxD3D11ShutdownFn g_real_shutdown = nullptr;
inline decltype(&NVSDK_NGX_D3D11_Shutdown1) g_real_shutdown1 = nullptr;
// The game calls these Init entry points before the first RR create; the hooks hand NGX the native device.
using NgxD3D11InitFn = NVSDK_NGX_Result(NVSDK_CONV*)(uint64_t, uint64_t, ID3D11Device*, uint64_t);
using NgxD3D11InitExtFn = NVSDK_NGX_Result(NVSDK_CONV*)(uint64_t, uint64_t, ID3D11Device*, uint64_t, uint64_t);
using NgxInitProjectIdRawFn = NVSDK_NGX_Result(NVSDK_CONV*)(uint64_t, uint64_t, uint64_t, uint64_t, ID3D11Device*, uint64_t, uint64_t);
inline NgxD3D11InitFn g_real_init = nullptr;
inline NgxD3D11InitExtFn g_real_init_ext = nullptr;
inline NgxInitProjectIdRawFn g_real_init_with_project_id = nullptr;
inline std::atomic_bool g_ngx_init_seen{false};
// True when the game's Init reached NGX with the native device (unwrapped and succeeded). Only then are the
// contexts unwrapped and RR attempted.
inline std::atomic<bool> g_ngx_init_native{false};

// Resolved for the probe, not detoured.
inline decltype(&NVSDK_NGX_D3D11_GetCapabilityParameters) g_get_capability_parameters = nullptr;
inline decltype(&NVSDK_NGX_D3D11_AllocateParameters) g_allocate_parameters = nullptr;
inline decltype(&NVSDK_NGX_D3D11_DestroyParameters) g_destroy_parameters = nullptr;

// Opt-in for the one-time RR probe (creates and releases a full RR feature): Ray Tracing tab row RrProbeEnable.
inline bool RrProbeRequested() { return g_rr_probe_enabled > 0.5f; }

struct NgxFeatureRecord {
  int feature_id = -1;
  unsigned int width = 0u, height = 0u, out_width = 0u, out_height = 0u;
  int perf_quality = -1, create_flags = 0, output_subrects = 0;
  bool eval_logged = false;
  bool fail_logged = false;
  NVSDK_NGX_Handle* rr_handle = nullptr;
  NVSDK_NGX_Parameter* rr_params = nullptr;
  unsigned int rr_preset = 0u;
  ID3D11DeviceContext* rr_context = nullptr;
  DWORD rr_thread = 0u;
};

struct JitterDiag {
  unsigned int handle_id = UINT_MAX;
  uint32_t samples = 0u;
  std::vector<uint64_t> keys;
  bool logged = false;
};

struct NgxState {
  std::shared_mutex mutex;
  std::unordered_map<unsigned int, NgxFeatureRecord> features;
  JitterDiag jitter;
};

inline NgxState g_ngx;
inline std::shared_mutex g_ngx_arm_mutex;
// Set while ArmNgxHooks runs on this thread: its preload LoadLibraryW passes through HookLoadLibrary*, which must not
// arm again. Restored on every exit.
inline thread_local bool g_in_arm = false;
struct ArmScope {
  ArmScope() : previous(g_in_arm) { g_in_arm = true; }
  ~ArmScope() { g_in_arm = previous; }
  const bool previous;
};

inline std::atomic_bool g_ngx_armed{false};
inline std::atomic_bool g_ngx_arm_attempted{false};
inline std::atomic_bool g_ngx_probe_done{false};
inline std::atomic<uint32_t> g_ngx_create_calls{0u};
inline std::atomic<uint32_t> g_ngx_evaluate_calls{0u};
inline std::atomic<uint32_t> g_ngx_release_calls{0u};
inline std::atomic<uint32_t> g_ngx_logged_evaluates{UINT32_MAX};

inline std::string NgxResultHex(NVSDK_NGX_Result result) {
  char buffer[16];
  std::snprintf(buffer, sizeof(buffer), "0x%08X", static_cast<unsigned int>(result));
  return buffer;
}

inline std::string NgxModulePath(HMODULE module) {
  if (module == nullptr) {
    return "none";
  }
  wchar_t path[MAX_PATH];
  GetModuleFileNameW(module, path, MAX_PATH);
  return std::filesystem::path(path).string();
}

// A C++ exception thrown inside NGX (an outdated nvngx_dlssd.dll resolving a preset hint) would otherwise end the
// process, and no Streamline handler sits on this path. Separate function because __try cannot share a frame with
// objects needing unwinding (MSVC C2712). M2a will be its second caller.
inline NVSDK_NGX_Result CreateFeatureGuarded(
    ID3D11DeviceContext* context,
    NVSDK_NGX_Feature feature,
    NVSDK_NGX_Parameter* parameters,
    NVSDK_NGX_Handle** out_handle,
    DWORD* seh_code) {
  *seh_code = 0u;
  __try {
    return g_real_create_feature(context, feature, parameters, out_handle);
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    *seh_code = GetExceptionCode();
    return NVSDK_NGX_Result_Fail;
  }
}

// What the RR evaluate's guard recorded: the exception code, for an access violation the access kind (0 read, 1 write)
// and the faulting data address, and the faulting instruction address.
struct RrFault {
  DWORD code = 0u;
  ULONG_PTR access = 0u;
  ULONG_PTR address = 0u;
  uintptr_t at = 0u;
};

inline int RecordRrFault(EXCEPTION_POINTERS* pointers, RrFault* fault) {
  const EXCEPTION_RECORD* record = pointers->ExceptionRecord;
  fault->code = record->ExceptionCode;
  fault->at = reinterpret_cast<uintptr_t>(record->ExceptionAddress);
  if (record->ExceptionCode == EXCEPTION_ACCESS_VIOLATION && record->NumberParameters >= 2u) {
    fault->access = record->ExceptionInformation[0];
    fault->address = record->ExceptionInformation[1];
  }
  return EXCEPTION_EXECUTE_HANDLER;
}

// The RR evaluate's own parameter block, filled as NVIDIA's D3D11 RR helper fills it (nvsdk_ngx_helpers_dlssd.h,
// NGX_D3D11_EVALUATE_DLSSD_EXT): every resource key nullptr and every subrect base 0 first, then the values the RR path
// takes from the SR block, then our guide inputs. The SR block is only read.
inline void FillRrParameters(
    NVSDK_NGX_Parameter* rr,
    const NVSDK_NGX_Parameter* game,
    ID3D11Resource* albedo,
    ID3D11Resource* specular,
    ID3D11Resource* normal_roughness,
    ID3D11Resource* depth_copy,
    float* world_to_view,
    float* view_to_clip) {
  for (const char* key : {
           NVSDK_NGX_Parameter_Color,
           NVSDK_NGX_Parameter_Output,
           NVSDK_NGX_Parameter_Depth,
           NVSDK_NGX_Parameter_MotionVectors,
           NVSDK_NGX_Parameter_TransparencyMask,
           NVSDK_NGX_Parameter_ExposureTexture,
           NVSDK_NGX_Parameter_DLSS_Input_Bias_Current_Color_Mask,
           NVSDK_NGX_Parameter_GBuffer_Albedo,
           NVSDK_NGX_Parameter_GBuffer_Roughness,
           NVSDK_NGX_Parameter_GBuffer_Metallic,
           NVSDK_NGX_Parameter_GBuffer_Specular,
           NVSDK_NGX_Parameter_GBuffer_Subsurface,
           NVSDK_NGX_Parameter_GBuffer_Normals,
           NVSDK_NGX_Parameter_GBuffer_ShadingModelId,
           NVSDK_NGX_Parameter_GBuffer_MaterialId,
           NVSDK_NGX_Parameter_GBuffer_Atrrib_8,
           NVSDK_NGX_Parameter_GBuffer_Atrrib_9,
           NVSDK_NGX_Parameter_GBuffer_SpecularMvec,
           NVSDK_NGX_Parameter_GBuffer_Atrrib_11,
           NVSDK_NGX_Parameter_GBuffer_Atrrib_12,
           NVSDK_NGX_Parameter_GBuffer_Atrrib_13,
           NVSDK_NGX_Parameter_GBuffer_Atrrib_14,
           NVSDK_NGX_Parameter_GBuffer_Atrrib_15,
           NVSDK_NGX_Parameter_MotionVectors3D,
           NVSDK_NGX_Parameter_IsParticleMask,
           NVSDK_NGX_Parameter_AnimatedTextureMask,
           NVSDK_NGX_Parameter_DepthHighRes,
           NVSDK_NGX_Parameter_Position_ViewSpace,
           NVSDK_NGX_Parameter_RayTracingHitDistance,
           NVSDK_NGX_Parameter_DiffuseAlbedo,
           NVSDK_NGX_Parameter_SpecularAlbedo,
           NVSDK_NGX_Parameter_GBuffer_Emissive,
           NVSDK_NGX_Parameter_DLSSD_Alpha,
           NVSDK_NGX_Parameter_DLSSD_OutputAlpha,
           NVSDK_NGX_Parameter_DLSSD_ReflectedAlbedo,
           NVSDK_NGX_Parameter_DLSSD_ColorBeforeParticles,
           NVSDK_NGX_Parameter_DLSSD_ColorAfterParticles,
           NVSDK_NGX_Parameter_DLSSD_ColorBeforeTransparency,
           NVSDK_NGX_Parameter_DLSSD_ColorAfterTransparency,
           NVSDK_NGX_Parameter_DLSSD_ColorBeforeFog,
           NVSDK_NGX_Parameter_DLSSD_ColorAfterFog,
           NVSDK_NGX_Parameter_DLSSD_ScreenSpaceSubsurfaceScatteringGuide,
           NVSDK_NGX_Parameter_DLSSD_ColorBeforeScreenSpaceSubsurfaceScattering,
           NVSDK_NGX_Parameter_DLSSD_ColorAfterScreenSpaceSubsurfaceScattering,
           NVSDK_NGX_Parameter_DLSSD_ScreenSpaceRefractionGuide,
           NVSDK_NGX_Parameter_DLSSD_ColorBeforeScreenSpaceRefraction,
           NVSDK_NGX_Parameter_DLSSD_ColorAfterScreenSpaceRefraction,
           NVSDK_NGX_Parameter_DLSSD_DepthOfFieldGuide,
           NVSDK_NGX_Parameter_DLSSD_ColorBeforeDepthOfField,
           NVSDK_NGX_Parameter_DLSSD_ColorAfterDepthOfField,
           NVSDK_NGX_Parameter_DLSSD_DiffuseHitDistance,
           NVSDK_NGX_Parameter_DLSSD_SpecularHitDistance,
           NVSDK_NGX_Parameter_DLSSD_DiffuseRayDirection,
           NVSDK_NGX_Parameter_DLSSD_SpecularRayDirection,
           NVSDK_NGX_Parameter_DLSSD_DiffuseRayDirectionHitDistance,
           NVSDK_NGX_Parameter_DLSSD_SpecularRayDirectionHitDistance,
           NVSDK_NGX_Parameter_DLSS_TransparencyLayer,
           NVSDK_NGX_Parameter_DLSS_TransparencyLayerOpacity,
           NVSDK_NGX_Parameter_DLSS_TransparencyLayerMvecs,
           NVSDK_NGX_Parameter_DLSS_DisocclusionMask,
       }) {
    rr->Set(key, static_cast<ID3D11Resource*>(nullptr));
  }
  for (const char* key : {
           NVSDK_NGX_Parameter_DLSS_Input_Color_Subrect_Base_X,
           NVSDK_NGX_Parameter_DLSS_Input_Color_Subrect_Base_Y,
           NVSDK_NGX_Parameter_DLSS_Input_Depth_Subrect_Base_X,
           NVSDK_NGX_Parameter_DLSS_Input_Depth_Subrect_Base_Y,
           NVSDK_NGX_Parameter_DLSS_Input_MV_SubrectBase_X,
           NVSDK_NGX_Parameter_DLSS_Input_MV_SubrectBase_Y,
           NVSDK_NGX_Parameter_DLSS_Input_Translucency_SubrectBase_X,
           NVSDK_NGX_Parameter_DLSS_Input_Translucency_SubrectBase_Y,
           NVSDK_NGX_Parameter_DLSS_Input_Bias_Current_Color_SubrectBase_X,
           NVSDK_NGX_Parameter_DLSS_Input_Bias_Current_Color_SubrectBase_Y,
           NVSDK_NGX_Parameter_DLSS_Output_Subrect_Base_X,
           NVSDK_NGX_Parameter_DLSS_Output_Subrect_Base_Y,
           NVSDK_NGX_Parameter_DLSS_Input_DiffuseAlbedo_Subrect_Base_X,
           NVSDK_NGX_Parameter_DLSS_Input_DiffuseAlbedo_Subrect_Base_Y,
           NVSDK_NGX_Parameter_DLSS_Input_SpecularAlbedo_Subrect_Base_X,
           NVSDK_NGX_Parameter_DLSS_Input_SpecularAlbedo_Subrect_Base_Y,
           NVSDK_NGX_Parameter_DLSS_Input_Normals_Subrect_Base_X,
           NVSDK_NGX_Parameter_DLSS_Input_Normals_Subrect_Base_Y,
           NVSDK_NGX_Parameter_DLSS_Input_Roughness_Subrect_Base_X,
           NVSDK_NGX_Parameter_DLSS_Input_Roughness_Subrect_Base_Y,
           NVSDK_NGX_Parameter_DLSSD_Alpha_Subrect_Base_X,
           NVSDK_NGX_Parameter_DLSSD_Alpha_Subrect_Base_Y,
           NVSDK_NGX_Parameter_DLSSD_OutputAlpha_Subrect_Base_X,
           NVSDK_NGX_Parameter_DLSSD_OutputAlpha_Subrect_Base_Y,
           NVSDK_NGX_Parameter_DLSSD_ReflectedAlbedo_Subrect_Base_X,
           NVSDK_NGX_Parameter_DLSSD_ReflectedAlbedo_Subrect_Base_Y,
           NVSDK_NGX_Parameter_DLSSD_ColorAfterParticles_Subrect_Base_X,
           NVSDK_NGX_Parameter_DLSSD_ColorAfterParticles_Subrect_Base_Y,
           NVSDK_NGX_Parameter_DLSSD_ColorBeforeParticles_Subrect_Base_X,
           NVSDK_NGX_Parameter_DLSSD_ColorBeforeParticles_Subrect_Base_Y,
           NVSDK_NGX_Parameter_DLSSD_ColorBeforeTransparency_Subrect_Base_X,
           NVSDK_NGX_Parameter_DLSSD_ColorBeforeTransparency_Subrect_Base_Y,
           NVSDK_NGX_Parameter_DLSSD_ColorAfterTransparency_Subrect_Base_X,
           NVSDK_NGX_Parameter_DLSSD_ColorAfterTransparency_Subrect_Base_Y,
           NVSDK_NGX_Parameter_DLSSD_ColorBeforeFog_Subrect_Base_X,
           NVSDK_NGX_Parameter_DLSSD_ColorBeforeFog_Subrect_Base_Y,
           NVSDK_NGX_Parameter_DLSSD_ColorAfterFog_Subrect_Base_X,
           NVSDK_NGX_Parameter_DLSSD_ColorAfterFog_Subrect_Base_Y,
           NVSDK_NGX_Parameter_DLSSD_ScreenSpaceSubsurfaceScatteringGuide_Subrect_Base_X,
           NVSDK_NGX_Parameter_DLSSD_ScreenSpaceSubsurfaceScatteringGuide_Subrect_Base_Y,
           NVSDK_NGX_Parameter_DLSSD_ColorBeforeScreenSpaceSubsurfaceScattering_Subrect_Base_X,
           NVSDK_NGX_Parameter_DLSSD_ColorBeforeScreenSpaceSubsurfaceScattering_Subrect_Base_Y,
           NVSDK_NGX_Parameter_DLSSD_ColorAfterScreenSpaceSubsurfaceScattering_Subrect_Base_X,
           NVSDK_NGX_Parameter_DLSSD_ColorAfterScreenSpaceSubsurfaceScattering_Subrect_Base_Y,
           NVSDK_NGX_Parameter_DLSSD_ScreenSpaceRefractionGuide_Subrect_Base_X,
           NVSDK_NGX_Parameter_DLSSD_ScreenSpaceRefractionGuide_Subrect_Base_Y,
           NVSDK_NGX_Parameter_DLSSD_ColorBeforeScreenSpaceRefraction_Subrect_Base_X,
           NVSDK_NGX_Parameter_DLSSD_ColorBeforeScreenSpaceRefraction_Subrect_Base_Y,
           NVSDK_NGX_Parameter_DLSSD_ColorAfterScreenSpaceRefraction_Subrect_Base_X,
           NVSDK_NGX_Parameter_DLSSD_ColorAfterScreenSpaceRefraction_Subrect_Base_Y,
           NVSDK_NGX_Parameter_DLSSD_DepthOfFieldGuide_Subrect_Base_X,
           NVSDK_NGX_Parameter_DLSSD_DepthOfFieldGuide_Subrect_Base_Y,
           NVSDK_NGX_Parameter_DLSSD_ColorBeforeDepthOfField_Subrect_Base_X,
           NVSDK_NGX_Parameter_DLSSD_ColorBeforeDepthOfField_Subrect_Base_Y,
           NVSDK_NGX_Parameter_DLSSD_ColorAfterDepthOfField_Subrect_Base_X,
           NVSDK_NGX_Parameter_DLSSD_ColorAfterDepthOfField_Subrect_Base_Y,
           NVSDK_NGX_Parameter_DLSSD_DiffuseHitDistance_Subrect_Base_X,
           NVSDK_NGX_Parameter_DLSSD_DiffuseHitDistance_Subrect_Base_Y,
           NVSDK_NGX_Parameter_DLSSD_SpecularHitDistance_Subrect_Base_X,
           NVSDK_NGX_Parameter_DLSSD_SpecularHitDistance_Subrect_Base_Y,
           NVSDK_NGX_Parameter_DLSSD_DiffuseRayDirection_Subrect_Base_X,
           NVSDK_NGX_Parameter_DLSSD_DiffuseRayDirection_Subrect_Base_Y,
           NVSDK_NGX_Parameter_DLSSD_SpecularRayDirection_Subrect_Base_X,
           NVSDK_NGX_Parameter_DLSSD_SpecularRayDirection_Subrect_Base_Y,
           NVSDK_NGX_Parameter_DLSSD_DiffuseRayDirectionHitDistance_Subrect_Base_X,
           NVSDK_NGX_Parameter_DLSSD_DiffuseRayDirectionHitDistance_Subrect_Base_Y,
           NVSDK_NGX_Parameter_DLSSD_SpecularRayDirectionHitDistance_Subrect_Base_X,
           NVSDK_NGX_Parameter_DLSSD_SpecularRayDirectionHitDistance_Subrect_Base_Y,
           NVSDK_NGX_Parameter_DLSS_TransparencyLayer_Subrect_Base_X,
           NVSDK_NGX_Parameter_DLSS_TransparencyLayer_Subrect_Base_Y,
           NVSDK_NGX_Parameter_DLSS_TransparencyLayerOpacity_Subrect_Base_X,
           NVSDK_NGX_Parameter_DLSS_TransparencyLayerOpacity_Subrect_Base_Y,
           NVSDK_NGX_Parameter_DLSS_TransparencyLayerMvecs_Subrect_Base_X,
           NVSDK_NGX_Parameter_DLSS_TransparencyLayerMvecs_Subrect_Base_Y,
           NVSDK_NGX_Parameter_DLSS_DisocclusionMask_Subrect_Base_X,
           NVSDK_NGX_Parameter_DLSS_DisocclusionMask_Subrect_Base_Y,
       }) {
    rr->Set(key, 0u);
  }

  for (const char* key : {
           NVSDK_NGX_Parameter_Color,
           NVSDK_NGX_Parameter_Output,
           NVSDK_NGX_Parameter_Depth,
           NVSDK_NGX_Parameter_MotionVectors,
           NVSDK_NGX_Parameter_ExposureTexture,
       }) {
    ID3D11Resource* resource = nullptr;
    if (NVSDK_NGX_SUCCEED(game->Get(key, &resource))) {
      rr->Set(key, resource);
    }
  }
  for (const char* key : {
           NVSDK_NGX_Parameter_Jitter_Offset_X,
           NVSDK_NGX_Parameter_Jitter_Offset_Y,
           NVSDK_NGX_Parameter_MV_Scale_X,
           NVSDK_NGX_Parameter_MV_Scale_Y,
           NVSDK_NGX_Parameter_DLSS_Pre_Exposure,
           NVSDK_NGX_Parameter_DLSS_Exposure_Scale,
       }) {
    float value = 0.f;
    if (NVSDK_NGX_SUCCEED(game->Get(key, &value))) {
      rr->Set(key, value);
    }
  }
  int reset = 0;
  if (NVSDK_NGX_SUCCEED(game->Get(NVSDK_NGX_Parameter_Reset, &reset))) {
    rr->Set(NVSDK_NGX_Parameter_Reset, reset);
  }
  for (const char* key : {
           NVSDK_NGX_Parameter_DLSS_Input_Color_Subrect_Base_X,
           NVSDK_NGX_Parameter_DLSS_Input_Color_Subrect_Base_Y,
           NVSDK_NGX_Parameter_DLSS_Input_Depth_Subrect_Base_X,
           NVSDK_NGX_Parameter_DLSS_Input_Depth_Subrect_Base_Y,
           NVSDK_NGX_Parameter_DLSS_Input_MV_SubrectBase_X,
           NVSDK_NGX_Parameter_DLSS_Input_MV_SubrectBase_Y,
           NVSDK_NGX_Parameter_DLSS_Output_Subrect_Base_X,
           NVSDK_NGX_Parameter_DLSS_Output_Subrect_Base_Y,
           NVSDK_NGX_Parameter_DLSS_Render_Subrect_Dimensions_Width,
           NVSDK_NGX_Parameter_DLSS_Render_Subrect_Dimensions_Height,
       }) {
    unsigned int value = 0u;
    if (NVSDK_NGX_SUCCEED(game->Get(key, &value))) {
      rr->Set(key, value);
    }
  }

  rr->Set(NVSDK_NGX_Parameter_DiffuseAlbedo, albedo);
  rr->Set(NVSDK_NGX_Parameter_GBuffer_Albedo, albedo);
  rr->Set(NVSDK_NGX_Parameter_SpecularAlbedo, specular);
  rr->Set(NVSDK_NGX_Parameter_GBuffer_Normals, normal_roughness);
  rr->Set(NVSDK_NGX_Parameter_Depth, depth_copy);
  rr->Set(NVSDK_NGX_Parameter_GBuffer_Roughness, normal_roughness);
  rr->Set(NVSDK_NGX_Parameter_TonemapperType, 0u);
  rr->Set(NVSDK_NGX_Parameter_FrameTimeDeltaInMsec, 0.f);
  rr->Set(NVSDK_NGX_Parameter_DLSS_Indicator_Invert_X_Axis, 0);
  rr->Set(NVSDK_NGX_Parameter_DLSS_Indicator_Invert_Y_Axis, 0);
  rr->Set(NVSDK_NGX_Parameter_DLSS_WORLD_TO_VIEW_MATRIX, static_cast<void*>(world_to_view));
  rr->Set(NVSDK_NGX_Parameter_DLSS_VIEW_TO_CLIP_MATRIX, static_cast<void*>(view_to_clip));
}

// Checks the RR inputs (already filled in rr) against the sizes, bind flags, formats and devices NVIDIA's DLSS-D path
// expects. Every check runs; each failing key is logged once per reason. Returns false when any check failed, and the
// caller then skips RR for the frame. Only devices and contexts are proxies that get unwrapped; resources are not.
inline bool ValidateRrInputs(
    const NVSDK_NGX_Parameter* rr,
    ID3D11DeviceContext* context,
    const NgxFeatureRecord& record,
    const float* world_to_view,
    const float* view_to_clip) {
  const auto report = [](const char* key, const std::string& reason, const std::string& value) {
    static std::unordered_set<std::string> logged;
    if (logged.insert(std::string(key) + " " + reason).second) {
      renodx::utils::log::w("[world-rr] input invalid: ", key, " ", reason, " ", value);
    }
  };
  // COM identity of two devices: the IUnknown pointers are equal only for the same object.
  const auto same_device = [](ID3D11Device* a, ID3D11Device* b) {
    IUnknown* unknown_a = nullptr;
    IUnknown* unknown_b = nullptr;
    a->QueryInterface(__uuidof(IUnknown), reinterpret_cast<void**>(&unknown_a));
    b->QueryInterface(__uuidof(IUnknown), reinterpret_cast<void**>(&unknown_b));
    const bool same = unknown_a != nullptr && unknown_a == unknown_b;
    if (unknown_a != nullptr) {
      unknown_a->Release();
    }
    if (unknown_b != nullptr) {
      unknown_b->Release();
    }
    return same;
  };

  bool all_ok = true;
  if (world_to_view == nullptr || view_to_clip == nullptr) {
    report("DLSS_VIEW_MATRICES", "null", "null");
    all_ok = false;
  }

  struct Spec {
    const char* key;
    bool uav;
    bool output_size;
    bool depth;
    bool motion;
  };
  constexpr Spec kSpecs[] = {
      {NVSDK_NGX_Parameter_Color, false, false, false, false},
      {NVSDK_NGX_Parameter_Depth, false, false, true, false},
      {NVSDK_NGX_Parameter_MotionVectors, false, false, false, true},
      {NVSDK_NGX_Parameter_DiffuseAlbedo, false, false, false, false},
      {NVSDK_NGX_Parameter_SpecularAlbedo, false, false, false, false},
      {NVSDK_NGX_Parameter_GBuffer_Normals, false, false, false, false},
      {NVSDK_NGX_Parameter_GBuffer_Roughness, false, false, false, false},
      {NVSDK_NGX_Parameter_Output, true, true, false, false},
  };

  ID3D11Device* context_raw = nullptr;
  context->GetDevice(&context_raw);
  ID3D11Device* context_device = context_raw;
  const bool context_unwrapped = renodx::utils::directx::NativeFromReShadeProxy(&context_device);
  const bool mv_output_size_ok = (record.create_flags & static_cast<int>(NVSDK_NGX_DLSS_Feature_Flags_MVLowRes)) == 0;
  for (const Spec& spec : kSpecs) {
    ID3D11Resource* resource = nullptr;
    if (NVSDK_NGX_FAILED(rr->Get(spec.key, &resource)) || resource == nullptr) {
      report(spec.key, "missing", "null");
      all_ok = false;
      continue;
    }
    ID3D11Texture2D* texture = nullptr;
    if (FAILED(resource->QueryInterface(IID_PPV_ARGS(&texture)))) {
      report(spec.key, "not-tex2d", "-");
      all_ok = false;
      continue;
    }
    D3D11_TEXTURE2D_DESC desc = {};
    texture->GetDesc(&desc);
    texture->Release();

    const unsigned int expect_w = spec.output_size ? record.out_width : record.width;
    const unsigned int expect_h = spec.output_size ? record.out_height : record.height;
    const bool size_ok = (desc.Width == expect_w && desc.Height == expect_h)
                         || (spec.motion && mv_output_size_ok && desc.Width == record.out_width
                             && desc.Height == record.out_height);
    const UINT bind = spec.uav ? D3D11_BIND_UNORDERED_ACCESS : D3D11_BIND_SHADER_RESOURCE;
    if (!size_ok) {
      report(spec.key, "size",
             renodx::utils::log::BuildString(desc.Width, "x", desc.Height, " expected ", expect_w, "x", expect_h));
      all_ok = false;
    }
    if (desc.SampleDesc.Count != 1u) {
      report(spec.key, "samples", renodx::utils::log::BuildString(desc.SampleDesc.Count));
      all_ok = false;
    }
    if ((desc.BindFlags & bind) == 0u) {
      report(spec.key, "bind", renodx::utils::log::BuildString(renodx::utils::log::AsPtr(desc.BindFlags)));
      all_ok = false;
    }
    if (spec.depth && desc.Format != DXGI_FORMAT_R32_FLOAT) {
      report(spec.key, "format", renodx::utils::log::BuildString(static_cast<int>(desc.Format)));
      all_ok = false;
    }

    ID3D11Device* resource_raw = nullptr;
    resource->GetDevice(&resource_raw);
    ID3D11Device* resource_device = resource_raw;
    const bool resource_unwrapped = renodx::utils::directx::NativeFromReShadeProxy(&resource_device);
    const std::string devices = renodx::utils::log::BuildString(
        "res_dev=", renodx::utils::log::AsPtr(resource_raw), " ctx_dev=", renodx::utils::log::AsPtr(context_raw));
    if (same_device(resource_device, context_device)) {
      static std::unordered_set<std::string> device_logged;
      if (g_rr_debug_log > 0.5f && device_logged.insert(spec.key).second) {
        renodx::utils::log::i(
            "[world-rr] input ", spec.key, " device=", (resource_unwrapped || context_unwrapped ? "proxy-unwrapped" : "native"),
            " ", devices);
      }
    } else {
      report(spec.key, "device=mismatch", devices);
      all_ok = false;
    }
    if (resource_raw != nullptr) {
      resource_raw->Release();
    }

    static std::unordered_set<std::string> formats_logged;
    if (g_rr_debug_log > 0.5f && formats_logged.insert(spec.key).second) {
      renodx::utils::log::i(
          "[world-rr] input ", spec.key, " format=", static_cast<int>(desc.Format), " size=", desc.Width, "x", desc.Height);
    }
  }
  if (context_raw != nullptr) {
    context_raw->Release();
  }

  static std::atomic_bool absent_logged{false};
  if (g_rr_debug_log > 0.5f && !absent_logged.exchange(true)) {
    renodx::utils::log::i("[world-rr] input absent (optional): GBuffer.SpecularMvec, RayTracingHitDistance");
  }
  return all_ok;
}

// The RR evaluate under the same guard as CreateFeatureGuarded: an exception or access violation inside NGX returns a
// failure (latched, SR evaluate runs) instead of ending the process. Separate function for the same __try reason.
inline NVSDK_NGX_Result EvaluateRrGuarded(
    ID3D11DeviceContext* context,
    const NVSDK_NGX_Handle* handle,
    const NVSDK_NGX_Parameter* parameters,
    PFN_NVSDK_NGX_ProgressCallback callback,
    RrFault* fault) {
  *fault = RrFault{};
  __try {
    return g_real_evaluate_feature(context, handle, parameters, callback);
  } __except (RecordRrFault(GetExceptionInformation(), fault)) {
    return NVSDK_NGX_Result_Fail;
  }
}

// One RR feature create at the given preset; both callers (the probe and the RR redirect) log and own the handle.
// *allocated is false when the parameter block could not be allocated (the result is then the allocate result).
inline NVSDK_NGX_Result CreateRrFeature(
    ID3D11DeviceContext* context,
    const NgxFeatureRecord& record,
    unsigned int preset,
    int create_flags,
    NVSDK_NGX_Handle** out_handle,
    DWORD* exception_code,
    bool* allocated) {
  *allocated = false;
  *exception_code = 0u;
  *out_handle = nullptr;
  NVSDK_NGX_Parameter* parameters = nullptr;
  const NVSDK_NGX_Result allocate_result = g_allocate_parameters(&parameters);
  if (NVSDK_NGX_FAILED(allocate_result) || parameters == nullptr) {
    if (parameters != nullptr) {
      g_destroy_parameters(parameters);
    }
    return allocate_result;
  }
  *allocated = true;
  parameters->Set(NVSDK_NGX_Parameter_Width, record.width);
  parameters->Set(NVSDK_NGX_Parameter_Height, record.height);
  parameters->Set(NVSDK_NGX_Parameter_OutWidth, record.out_width);
  parameters->Set(NVSDK_NGX_Parameter_OutHeight, record.out_height);
  parameters->Set(NVSDK_NGX_Parameter_PerfQualityValue, record.perf_quality);
  parameters->Set(NVSDK_NGX_Parameter_DLSS_Feature_Create_Flags, create_flags);
  parameters->Set(NVSDK_NGX_Parameter_DLSS_Enable_Output_Subrects, record.output_subrects);
  parameters->Set(NVSDK_NGX_Parameter_DLSS_Denoise_Mode, static_cast<int>(NVSDK_NGX_DLSS_Denoise_Mode_DLUnified));
  parameters->Set(
      NVSDK_NGX_Parameter_DLSS_Roughness_Mode,
      static_cast<unsigned int>(NVSDK_NGX_DLSS_Roughness_Mode_Packed));
  parameters->Set(NVSDK_NGX_Parameter_Use_HW_Depth, 1u);
  for (const char* key : {
           NVSDK_NGX_Parameter_RayReconstruction_Hint_Render_Preset_DLAA,
           NVSDK_NGX_Parameter_RayReconstruction_Hint_Render_Preset_Quality,
           NVSDK_NGX_Parameter_RayReconstruction_Hint_Render_Preset_Balanced,
           NVSDK_NGX_Parameter_RayReconstruction_Hint_Render_Preset_Performance,
           NVSDK_NGX_Parameter_RayReconstruction_Hint_Render_Preset_UltraPerformance,
           NVSDK_NGX_Parameter_RayReconstruction_Hint_Render_Preset_UltraQuality}) {
    parameters->Set(key, preset);
  }
  const NVSDK_NGX_Result result = CreateFeatureGuarded(
      context, NVSDK_NGX_Feature_RayReconstruction, parameters, out_handle, exception_code);
  g_destroy_parameters(parameters);
  return result;
}

// The SuperSamplingDenoising capability, read once per session (the probe or the first SR evaluate). Returns 1 when RR can
// be created on this driver, 0 otherwise, including a failed query.
inline int QueryRrAvailable() {
  if (g_get_capability_parameters == nullptr) {
    renodx::utils::log::w("[world-rr] capability query failed: capability function not resolved");
    return 0;
  }
  NVSDK_NGX_Parameter* caps = nullptr;
  const NVSDK_NGX_Result caps_result = g_get_capability_parameters(&caps);
  if (NVSDK_NGX_FAILED(caps_result)) {
    if (caps != nullptr) {
      g_destroy_parameters(caps);
    }
    renodx::utils::log::w("[world-rr] capability query failed: call result=", NgxResultHex(caps_result));
    return 0;
  }
  if (caps == nullptr) {
    renodx::utils::log::w("[world-rr] capability query failed: capability parameters null");
    return 0;
  }
  int available = -1, needs_driver = -1, feature_init = -1;
  unsigned int min_major = 0u, min_minor = 0u;
  caps->Get(NVSDK_NGX_Parameter_SuperSamplingDenoising_Available, &available);
  caps->Get(NVSDK_NGX_Parameter_SuperSamplingDenoising_NeedsUpdatedDriver, &needs_driver);
  caps->Get(NVSDK_NGX_Parameter_SuperSamplingDenoising_MinDriverVersionMajor, &min_major);
  caps->Get(NVSDK_NGX_Parameter_SuperSamplingDenoising_MinDriverVersionMinor, &min_minor);
  caps->Get(NVSDK_NGX_Parameter_SuperSamplingDenoising_FeatureInitResult, &feature_init);
  if (g_rr_debug_log > 0.5f) {
    renodx::utils::log::i(
        "[world-rr] probe caps: denoising_available=", available, " needs_updated_driver=", needs_driver,
        " min_driver=", min_major, ".", min_minor, " feature_init=", feature_init);
  }
  g_destroy_parameters(caps);
  return available == 1 ? 1 : 0;
}

inline NVSDK_NGX_Result NVSDK_CONV HookCreateFeature(
    ID3D11DeviceContext* InDevCtx,
    NVSDK_NGX_Feature InFeatureID,
    NVSDK_NGX_Parameter* InParameters,
    NVSDK_NGX_Handle** OutHandle) {
  g_ngx_create_calls.fetch_add(1u, std::memory_order_relaxed);
  ID3D11DeviceContext* native_context = InDevCtx;
  if (g_ngx_init_native.load()) {
    renodx::utils::directx::NativeFromReShadeProxy(&native_context);
  }
  const NVSDK_NGX_Result result = g_real_create_feature(native_context, InFeatureID, InParameters, OutHandle);
  if (NVSDK_NGX_SUCCEED(result) && InParameters != nullptr && OutHandle != nullptr && *OutHandle != nullptr) {
    unsigned int width = 0u, height = 0u, out_width = 0u, out_height = 0u;
    int perf_quality = -1, create_flags = 0, output_subrects = 0;
    InParameters->Get(NVSDK_NGX_Parameter_Width, &width);
    InParameters->Get(NVSDK_NGX_Parameter_Height, &height);
    InParameters->Get(NVSDK_NGX_Parameter_OutWidth, &out_width);
    InParameters->Get(NVSDK_NGX_Parameter_OutHeight, &out_height);
    InParameters->Get(NVSDK_NGX_Parameter_PerfQualityValue, &perf_quality);
    InParameters->Get(NVSDK_NGX_Parameter_DLSS_Feature_Create_Flags, &create_flags);
    InParameters->Get(NVSDK_NGX_Parameter_DLSS_Enable_Output_Subrects, &output_subrects);

    const NgxFeatureRecord record{
        .feature_id = static_cast<int>(InFeatureID),
        .width = width,
        .height = height,
        .out_width = out_width,
        .out_height = out_height,
        .perf_quality = perf_quality,
        .create_flags = create_flags,
        .output_subrects = output_subrects,
    };
    const unsigned int handle_id = (*OutHandle)->Id;
    {
      std::unique_lock lock(g_ngx.mutex);
      g_ngx.features[handle_id] = record;
    }
    renodx::utils::log::i(
        "[world-rr] create feature=", record.feature_id, " handle=", handle_id,
        " in=", width, "x", height, " out=", out_width, "x", out_height,
        " perf=", perf_quality, " flags=", renodx::utils::log::AsPtr(static_cast<uint32_t>(create_flags)),
        " subrects=", output_subrects, " result=", NgxResultHex(result),
        " ctx=", renodx::utils::log::AsPtr(InDevCtx), " passed=", renodx::utils::log::AsPtr(native_context),
        " thread=", GetCurrentThreadId());
  }
  return result;
}

inline NVSDK_NGX_Result NVSDK_CONV HookEvaluateFeature(
    ID3D11DeviceContext* InDevCtx,
    const NVSDK_NGX_Handle* InFeatureHandle,
    const NVSDK_NGX_Parameter* InParameters,
    PFN_NVSDK_NGX_ProgressCallback InCallback) {
  g_ngx_evaluate_calls.fetch_add(1u, std::memory_order_relaxed);
  const unsigned int handle_id = InFeatureHandle != nullptr ? InFeatureHandle->Id : UINT_MAX;

  NgxFeatureRecord record;
  bool known = false;
  bool jitter_sample = false;
  {
    std::shared_lock lock(g_ngx.mutex);
    const auto found = g_ngx.features.find(handle_id);
    known = found != g_ngx.features.end();
    if (known) {
      record = found->second;
    }
    jitter_sample = g_rr_debug_log > 0.5f && known && InParameters != nullptr && record.feature_id == NVSDK_NGX_Feature_SuperSampling
                    && (g_ngx.jitter.handle_id == UINT_MAX || g_ngx.jitter.handle_id == handle_id)
                    && g_ngx.jitter.samples < 256u;
  }

  if (g_rr_debug_log > 0.5f && known && InParameters != nullptr && !record.eval_logged) {
    const auto describe_resource = [&](const char* name) -> std::string {
      ID3D11Resource* resource = nullptr;
      if (NVSDK_NGX_FAILED(InParameters->Get(name, &resource)) || resource == nullptr) {
        return "null";
      }
      ID3D11Texture2D* texture = nullptr;
      if (FAILED(resource->QueryInterface(IID_PPV_ARGS(&texture)))) {
        return "not-tex2d";
      }
      D3D11_TEXTURE2D_DESC desc = {};
      texture->GetDesc(&desc);
      texture->Release();
      return renodx::utils::log::BuildString(
          desc.Width, "x", desc.Height, " fmt=", static_cast<int>(desc.Format),
          " bind=", renodx::utils::log::AsPtr(desc.BindFlags));
    };
    const auto describe_scalar = [&](const char* name, auto zero) -> std::string {
      decltype(zero) value = zero;
      if (NVSDK_NGX_FAILED(InParameters->Get(name, &value))) {
        return "null";
      }
      return renodx::utils::log::BuildString(value);
    };
    renodx::utils::log::i(
        "[world-rr] eval feature=", record.feature_id, " handle=", handle_id,
        " ctx=", renodx::utils::log::AsPtr(InDevCtx), " thread=", GetCurrentThreadId(),
        " Color=", describe_resource(NVSDK_NGX_Parameter_Color),
        " Output=", describe_resource(NVSDK_NGX_Parameter_Output),
        " Depth=", describe_resource(NVSDK_NGX_Parameter_Depth),
        " MotionVectors=", describe_resource(NVSDK_NGX_Parameter_MotionVectors),
        " ExposureTexture=", describe_resource(NVSDK_NGX_Parameter_ExposureTexture),
        " TransparencyMask=", describe_resource(NVSDK_NGX_Parameter_TransparencyMask),
        " BiasCurrentColorMask=", describe_resource(NVSDK_NGX_Parameter_DLSS_Input_Bias_Current_Color_Mask),
        " jitter=", describe_scalar(NVSDK_NGX_Parameter_Jitter_Offset_X, 0.f), ",",
        describe_scalar(NVSDK_NGX_Parameter_Jitter_Offset_Y, 0.f),
        " mv_scale=", describe_scalar(NVSDK_NGX_Parameter_MV_Scale_X, 0.f), ",",
        describe_scalar(NVSDK_NGX_Parameter_MV_Scale_Y, 0.f),
        " pre_exposure=", describe_scalar(NVSDK_NGX_Parameter_DLSS_Pre_Exposure, 0.f),
        " exposure_scale=", describe_scalar(NVSDK_NGX_Parameter_DLSS_Exposure_Scale, 0.f),
        " reset=", describe_scalar(NVSDK_NGX_Parameter_Reset, 0),
        " subrect=", describe_scalar(NVSDK_NGX_Parameter_DLSS_Render_Subrect_Dimensions_Width, 0u), "x",
        describe_scalar(NVSDK_NGX_Parameter_DLSS_Render_Subrect_Dimensions_Height, 0u));
    std::unique_lock lock(g_ngx.mutex);
    const auto found = g_ngx.features.find(handle_id);
    if (found != g_ngx.features.end()) {
      found->second.eval_logged = true;
    }
  }

  if (jitter_sample) {
    float jitter_x = 0.f, jitter_y = 0.f;
    InParameters->Get(NVSDK_NGX_Parameter_Jitter_Offset_X, &jitter_x);
    InParameters->Get(NVSDK_NGX_Parameter_Jitter_Offset_Y, &jitter_y);
    const uint64_t key = (static_cast<uint64_t>(std::bit_cast<uint32_t>(jitter_x)) << 32)
                         | std::bit_cast<uint32_t>(jitter_y);
    bool log_now = false;
    std::size_t distinct = 0u;
    {
      std::unique_lock lock(g_ngx.mutex);
      JitterDiag& jitter = g_ngx.jitter;
      if (jitter.handle_id == UINT_MAX) {
        jitter.handle_id = handle_id;
      }
      if (jitter.handle_id == handle_id && jitter.samples < 256u) {
        jitter.samples++;
        if (std::find(jitter.keys.begin(), jitter.keys.end(), key) == jitter.keys.end()) {
          jitter.keys.push_back(key);
        }
        if (jitter.samples == 256u && !jitter.logged) {
          jitter.logged = true;
          log_now = true;
          distinct = jitter.keys.size();
        }
      }
    }
    if (log_now) {
      renodx::utils::log::i("[world-rr] jitter: ", distinct, " distinct positions over 256 evaluates");
    }
  }

  if (known && record.feature_id == NVSDK_NGX_Feature_SuperSampling && !RrProbeRequested()) {
    static std::atomic_bool skip_logged{false};
    if (g_rr_debug_log > 0.5f && !skip_logged.exchange(true)) {
      renodx::utils::log::i("[world-rr] probe skipped (RrProbe not set)");
    }
  }

  if (known && record.feature_id == NVSDK_NGX_Feature_SuperSampling && RrProbeRequested() && !g_ngx_probe_done.exchange(true)) {
    if (g_get_capability_parameters == nullptr || g_allocate_parameters == nullptr || g_destroy_parameters == nullptr
        || g_real_create_feature == nullptr || g_real_release_feature == nullptr) {
      renodx::utils::log::w("[world-rr] probe skipped: NGX parameter functions not resolved");
    } else {
      if (g_rr_available.load() == -1) {
        g_rr_available.store(QueryRrAvailable());
      }
      if (g_rr_available.load() != 1) {
        if (g_rr_debug_log > 0.5f) {
          renodx::utils::log::i("[world-rr] probe: SuperSamplingDenoising not available, skipping RR creates");
        }
      } else {
        const bool auto_exposure = (record.create_flags & static_cast<int>(NVSDK_NGX_DLSS_Feature_Flags_AutoExposure)) != 0;
        bool allocated = true;
        const auto attempt = [&](unsigned int preset, int flags) -> bool {
          const auto start = std::chrono::steady_clock::now();
          DWORD seh_code = 0u;
          NVSDK_NGX_Handle* handle = nullptr;
          bool attempt_allocated = false;
          const NVSDK_NGX_Result result =
              CreateRrFeature(InDevCtx, record, preset, flags, &handle, &seh_code, &attempt_allocated);
          const auto elapsed_ms =
              std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
          if (!attempt_allocated) {
            renodx::utils::log::w(
                "[world-rr] probe: RR create preset=", preset, " allocate result=", NgxResultHex(result), " failed");
            allocated = false;
            return false;
          }
          const bool created = NVSDK_NGX_SUCCEED(result) && handle != nullptr;
          if (g_rr_debug_log > 0.5f) {
            renodx::utils::log::i(
                "[world-rr] probe: RR create preset=", preset, " flags=", renodx::utils::log::AsPtr(static_cast<uint32_t>(flags)),
                " result=", NgxResultHex(result), " exception=",
                renodx::utils::log::AsPtr(static_cast<uint32_t>(seh_code)), " ms=", elapsed_ms);
          }
          if (created) {
            g_real_release_feature(handle);
          }
          return created;
        };

        bool created = attempt(6u, record.create_flags);
        if (!created && allocated) {
          created = attempt(0u, record.create_flags);
        }
        if (!created && allocated && auto_exposure) {
          created = attempt(6u, record.create_flags & ~static_cast<int>(NVSDK_NGX_DLSS_Feature_Flags_AutoExposure));
        }
      }
    }
  }

  // RR redirect (M2a): the SR evaluate becomes the RR evaluate when RR is requested and its inputs are ready. Every
  // fallback returns nullopt, and the real SR evaluate below runs with the original handle.
  ID3D11DeviceContext* native_context = InDevCtx;
  bool native_unwrapped = false;
  if (g_ngx_init_native.load()) {
    native_unwrapped = renodx::utils::directx::NativeFromReShadeProxy(&native_context);
  }
  const auto try_rr = [&]() -> std::optional<NVSDK_NGX_Result> {
    const uint32_t frame = g_state.frame.load(std::memory_order_relaxed);
    g_rr_last_sr_eval_frame.store(frame, std::memory_order_relaxed);
    if (RrRequested() && !g_ngx_init_native.load()) {
      g_rr_reason.store(RrReason::InitNotNative);
      return std::nullopt;
    }
    const int preset_index = std::clamp(static_cast<int>(g_rr_preset_index), 0, 3);
    const unsigned int preset = kRrPresetValues[preset_index];
    if (g_rr_preset_seen.exchange(preset_index) != preset_index) {
      ClearRrFailure();
    }
    if (g_rr_available.load() == -1) {
      g_rr_available.store(QueryRrAvailable());
    }
    const auto release_rr_handle = [&]() {
      g_real_release_feature(record.rr_handle);
      if (record.rr_params != nullptr) {
        g_destroy_parameters(record.rr_params);
      }
      record.rr_handle = nullptr;
      record.rr_params = nullptr;
      std::unique_lock lock(g_ngx.mutex);
      const auto found = g_ngx.features.find(handle_id);
      if (found != g_ngx.features.end()) {
        found->second.rr_handle = nullptr;
        found->second.rr_params = nullptr;
      }
    };
    if (record.rr_handle != nullptr && record.rr_preset != preset) {
      release_rr_handle();
    }
    const bool want = RrRequested() && !g_rr_failed.load() && g_rr_available.load() == 1;
    if (!want) {
      if (!RrRequested()) {
        ClearRrFailure();
      }
      if (record.rr_handle != nullptr) {
        release_rr_handle();
      }
      if (RrRequested() && g_rr_available.load() == 0) {
        g_rr_reason.store(RrReason::Unsupported);
      }
      return std::nullopt;
    }

    uint32_t guide_width = 0u, guide_height = 0u;
    ID3D11Resource* normals = nullptr;
    ID3D11Resource* diffuse = nullptr;
    ID3D11Resource* specular = nullptr;
    ID3D11Resource* depth_copy = nullptr;
    {
      std::shared_lock lock(g_rr_guides.mutex);
      guide_width = g_rr_guides.width;
      guide_height = g_rr_guides.height;
      if (g_rr_guides.frame == frame) {
        normals = reinterpret_cast<ID3D11Resource*>(g_rr_guides.normal_roughness.handle);
        diffuse = reinterpret_cast<ID3D11Resource*>(g_rr_guides.albedo.handle);
        specular = reinterpret_cast<ID3D11Resource*>(g_rr_guides.specular.handle);
        depth_copy = reinterpret_cast<ID3D11Resource*>(g_rr_guides.depth.handle);
      }
    }
    bool camera_valid = false;
    {
      std::lock_guard lock(g_state.mutex);
      camera_valid = g_state.camera.valid;
    }
    const bool guides_ready = normals != nullptr && diffuse != nullptr && specular != nullptr && depth_copy != nullptr
                              && guide_width == record.width && guide_height == record.height && camera_valid;
    if (!guides_ready) {
      g_rr_reason.store(RrReason::GuidesNotReady);
      static std::atomic_bool guides_logged{false};
      if (!guides_logged.exchange(true)) {
        renodx::utils::log::i(
            "[world-rr] guides not ready: sr ", record.width, "x", record.height, " guides ", guide_width, "x",
            guide_height, " frame=", frame, " camera_valid=", camera_valid);
      }
      return std::nullopt;
    }

    NVSDK_NGX_Handle* rr_handle = record.rr_handle;
    if (rr_handle == nullptr) {
      const auto start = std::chrono::steady_clock::now();
      DWORD exception_code = 0u;
      NVSDK_NGX_Handle* created = nullptr;
      bool allocated = false;
      if (!g_ngx_init_seen.load()) {
        renodx::utils::log::w("[world-rr] NGX Init not hooked: game initialized NGX before arming");
      }
      const NVSDK_NGX_Result create_result =
          CreateRrFeature(native_context, record, preset, record.create_flags, &created, &exception_code, &allocated);
      const auto elapsed_ms =
          std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
      if (!(allocated && NVSDK_NGX_SUCCEED(create_result) && created != nullptr)) {
        g_rr_failed.store(true);
        g_rr_failed_result.store(static_cast<uint32_t>(create_result));
        g_rr_failed_exception.store(static_cast<uint32_t>(exception_code));
        g_rr_reason.store(RrReason::CreateFailed);
        const std::filesystem::path game_dir = renodx::utils::platform::GetCurrentProcessPath().parent_path();
        renodx::utils::log::w(
            "[world-rr] RR create failed sr=", handle_id, " preset=", preset, " allocate=", allocated,
            " result=", NgxResultHex(create_result),
            " exception=", renodx::utils::log::AsPtr(static_cast<uint32_t>(exception_code)), " ms=", elapsed_ms,
            " nvngx_dlssd.dll version=", renodx::utils::platform::GetFileVersion(game_dir / L"nvngx_dlssd.dll"));
        return std::nullopt;
      }
      NVSDK_NGX_Parameter* rr_params = nullptr;
      g_allocate_parameters(&rr_params);
      if (rr_params == nullptr) {
        g_real_release_feature(created);
        g_rr_failed.store(true);
        g_rr_failed_result.store(static_cast<uint32_t>(NVSDK_NGX_Result_Fail));
        g_rr_failed_exception.store(0u);
        g_rr_reason.store(RrReason::CreateFailed);
        renodx::utils::log::w("[world-rr] RR parameter block allocate failed sr=", handle_id);
        return std::nullopt;
      }
      bool stored = false;
      {
        std::unique_lock lock(g_ngx.mutex);
        const auto found = g_ngx.features.find(handle_id);
        if (found != g_ngx.features.end()) {
          found->second.rr_handle = created;
          found->second.rr_params = rr_params;
          found->second.rr_preset = preset;
          found->second.rr_context = native_context;
          found->second.rr_thread = GetCurrentThreadId();
          stored = true;
        }
      }
      if (!stored) {
        g_real_release_feature(created);
        g_destroy_parameters(rr_params);
        return std::nullopt;
      }
      rr_handle = created;
      record.rr_handle = created;
      record.rr_params = rr_params;
      record.rr_preset = preset;
      record.rr_context = native_context;
      record.rr_thread = GetCurrentThreadId();
      const std::string dlssd_path = NgxModulePath(GetModuleHandleW(L"nvngx_dlssd.dll"));
      renodx::utils::log::i(
          "[world-rr] RR created sr=", handle_id, " preset=", preset, " ms=", elapsed_ms, " dlssd=", dlssd_path,
          " version=", renodx::utils::platform::GetFileVersion(std::filesystem::path(dlssd_path)),
          " ctx=", renodx::utils::log::AsPtr(InDevCtx), " passed=", renodx::utils::log::AsPtr(native_context),
          " unwrap=", (native_unwrapped ? "ok" : "failed"),
          " type=", (native_context->GetType() == D3D11_DEVICE_CONTEXT_IMMEDIATE ? "immediate" : "deferred"),
          " thread=", GetCurrentThreadId());
    }

    const bool context_ok = native_context->GetType() == D3D11_DEVICE_CONTEXT_IMMEDIATE
                            && native_context == record.rr_context && GetCurrentThreadId() == record.rr_thread;
    if (!context_ok) {
      static std::atomic_bool context_logged{false};
      if (!context_logged.exchange(true)) {
        renodx::utils::log::w(
            "[world-rr] input invalid: context lifetime immediate=",
            (native_context->GetType() == D3D11_DEVICE_CONTEXT_IMMEDIATE ? "yes" : "no"),
            " same=", (native_context == record.rr_context ? "yes" : "no"),
            " thread=", GetCurrentThreadId(), " create_thread=", record.rr_thread);
      }
      g_rr_reason.store(RrReason::InputsInvalid);
      return std::nullopt;
    }

    float world_to_view[16] = {};
    float view_to_clip[16] = {};
    {
      std::lock_guard lock(g_state.mutex);
      for (unsigned int row = 0u; row < 4u; ++row) {
        for (unsigned int col = 0u; col < 4u; ++col) {
          world_to_view[row * 4u + col] = g_state.camera.view[col * 4u + row];
          view_to_clip[row * 4u + col] = g_state.camera.proj[col * 4u + row];
        }
      }
    }

    FillRrParameters(record.rr_params, InParameters, diffuse, specular, normals, depth_copy, world_to_view, view_to_clip);
    if (!ValidateRrInputs(record.rr_params, native_context, record, world_to_view, view_to_clip)) {
      g_rr_reason.store(RrReason::InputsInvalid);
      return std::nullopt;
    }
    RrFault fault;
    const NVSDK_NGX_Result rr_result =
        EvaluateRrGuarded(native_context, rr_handle, record.rr_params, InCallback, &fault);
    if (NVSDK_NGX_FAILED(rr_result)) {
      g_rr_failed.store(true);
      g_rr_failed_result.store(static_cast<uint32_t>(rr_result));
      g_rr_failed_exception.store(static_cast<uint32_t>(fault.code));
      g_rr_reason.store(RrReason::EvaluateFailed);
      const auto ngx_dlssd = reinterpret_cast<uintptr_t>(GetModuleHandleW(L"nvngx_dlssd.dll"));
      const bool in_ngx_dlssd = ngx_dlssd != 0u && fault.at >= ngx_dlssd;
      const uintptr_t at_offset = in_ngx_dlssd ? fault.at - ngx_dlssd : fault.at;
      const char* access = "n/a";
      if (fault.code == EXCEPTION_ACCESS_VIOLATION) {
        access = (fault.access == 1u ? "w" : "r");
      }
      renodx::utils::log::w(
          "[world-rr] RR evaluate failed sr=", handle_id, " result=", NgxResultHex(rr_result),
          " exception=", renodx::utils::log::AsPtr(static_cast<uint32_t>(fault.code)), " access=", access,
          " addr=", renodx::utils::log::AsPtr(static_cast<uint64_t>(fault.address)),
          " at=", (in_ngx_dlssd ? "nvngx_dlssd.dll+" : "absolute "),
          renodx::utils::log::AsPtr(static_cast<uint64_t>(at_offset)));
      return std::nullopt;
    }
    g_rr_last_ok_frame.store(frame, std::memory_order_relaxed);
    g_rr_reason.store(RrReason::Running);
    return rr_result;
  };
  if (known && InParameters != nullptr && record.feature_id == NVSDK_NGX_Feature_SuperSampling) {
    if (const std::optional<NVSDK_NGX_Result> rr_result = try_rr()) {
      return *rr_result;
    }
  } else if (!known && RrRequested()) {
    g_rr_reason.store(RrReason::CreateNotSeen);
  }

  const NVSDK_NGX_Result result = g_real_evaluate_feature(native_context, InFeatureHandle, InParameters, InCallback);
  if (NVSDK_NGX_FAILED(result)) {
    bool log_failure = false;
    {
      std::unique_lock lock(g_ngx.mutex);
      const auto found = g_ngx.features.find(handle_id);
      if (found != g_ngx.features.end() && !found->second.fail_logged) {
        found->second.fail_logged = true;
        log_failure = true;
      }
    }
    if (log_failure) {
      renodx::utils::log::w("[world-rr] evaluate failed handle=", handle_id, " result=", NgxResultHex(result));
    }
  }
  return result;
}

inline NVSDK_NGX_Result NVSDK_CONV HookReleaseFeature(NVSDK_NGX_Handle* InHandle) {
  g_ngx_release_calls.fetch_add(1u, std::memory_order_relaxed);
  const unsigned int handle_id = InHandle != nullptr ? InHandle->Id : UINT_MAX;
  bool tracked = false;
  NVSDK_NGX_Handle* rr_handle = nullptr;
  NVSDK_NGX_Parameter* rr_params = nullptr;
  if (InHandle != nullptr) {
    std::unique_lock lock(g_ngx.mutex);
    const auto found = g_ngx.features.find(handle_id);
    if (found != g_ngx.features.end()) {
      tracked = true;
      rr_handle = found->second.rr_handle;
      rr_params = found->second.rr_params;
      g_ngx.features.erase(found);
    }
  }
  renodx::utils::log::i("[world-rr] release handle=", handle_id, " tracked=", tracked);
  if (rr_handle != nullptr) {
    g_real_release_feature(rr_handle);
    if (rr_params != nullptr) {
      g_destroy_parameters(rr_params);
    }
  }
  return g_real_release_feature(InHandle);
}

// Drops every tracked SR record and releases its RR handles and parameter blocks outside the lock.
inline void ReleaseTrackedRrHandles() {
  std::vector<std::pair<NVSDK_NGX_Handle*, NVSDK_NGX_Parameter*>> rr_handles;
  {
    std::unique_lock lock(g_ngx.mutex);
    for (const auto& entry : g_ngx.features) {
      if (entry.second.rr_handle != nullptr) {
        rr_handles.emplace_back(entry.second.rr_handle, entry.second.rr_params);
      }
    }
    g_ngx.features.clear();
  }
  for (const auto& [rr_handle, rr_params] : rr_handles) {
    g_real_release_feature(rr_handle);
    if (rr_params != nullptr) {
      g_destroy_parameters(rr_params);
    }
  }
}

inline NVSDK_NGX_Result NVSDK_CONV HookShutdown() {
  ReleaseTrackedRrHandles();
  renodx::utils::log::i("[world-rr] shutdown");
  return g_real_shutdown();
}

inline NVSDK_NGX_Result NVSDK_CONV HookShutdown1(ID3D11Device* InDevice) {
  renodx::utils::directx::NativeFromReShadeProxy(&InDevice);
  ReleaseTrackedRrHandles();
  renodx::utils::log::i("[world-rr] shutdown1 device=", renodx::utils::log::AsPtr(InDevice));
  return g_real_shutdown1(InDevice);
}

// The Init hooks unwrap the device the game passes (a ReShade proxy) before NGX sees it.
inline NVSDK_NGX_Result NVSDK_CONV HookInit(
    uint64_t InApplicationId,
    uint64_t InApplicationDataPath,
    ID3D11Device* InDevice,
    uint64_t InSDKVersion) {
  g_ngx_init_seen.store(true);
  const bool unwrapped = renodx::utils::directx::NativeFromReShadeProxy(&InDevice);
  if (g_rr_debug_log > 0.5f) {
    renodx::utils::log::i("[world-rr] init export=NVSDK_NGX_D3D11_Init device unwrap=", unwrapped ? "ok" : "not-proxy");
  }
  const NVSDK_NGX_Result result = g_real_init(InApplicationId, InApplicationDataPath, InDevice, InSDKVersion);
  g_ngx_init_native.store(unwrapped && NVSDK_NGX_SUCCEED(result));
  return result;
}

inline NVSDK_NGX_Result NVSDK_CONV HookInitExt(
    uint64_t InApplicationId,
    uint64_t InApplicationDataPath,
    ID3D11Device* InDevice,
    uint64_t InSDKVersion,
    uint64_t InParameters) {
  g_ngx_init_seen.store(true);
  const bool unwrapped = renodx::utils::directx::NativeFromReShadeProxy(&InDevice);
  if (g_rr_debug_log > 0.5f) {
    renodx::utils::log::i("[world-rr] init export=NVSDK_NGX_D3D11_Init_Ext device unwrap=", unwrapped ? "ok" : "not-proxy");
  }
  const NVSDK_NGX_Result result =
      g_real_init_ext(InApplicationId, InApplicationDataPath, InDevice, InSDKVersion, InParameters);
  g_ngx_init_native.store(unwrapped && NVSDK_NGX_SUCCEED(result));
  return result;
}

inline NVSDK_NGX_Result NVSDK_CONV HookInitWithProjectId(
    uint64_t InProjectId,
    uint64_t InEngineType,
    uint64_t InEngineVersion,
    uint64_t InApplicationDataPath,
    ID3D11Device* InDevice,
    uint64_t InFeatureInfo,
    uint64_t InSDKVersion) {
  g_ngx_init_seen.store(true);
  ID3D11Device* const proxy_device = InDevice;
  const bool unwrapped = renodx::utils::directx::NativeFromReShadeProxy(&InDevice);
  static std::atomic_bool enter_logged{false};
  if (!enter_logged.exchange(true)) {
    renodx::utils::log::i(
        "[world-rr] init export=NVSDK_NGX_D3D11_Init_ProjectID enter device=", renodx::utils::log::AsPtr(proxy_device),
        " native=", renodx::utils::log::AsPtr(InDevice), " unwrap=", (unwrapped ? "ok" : "not-proxy"));
  }
  const NVSDK_NGX_Result result = g_real_init_with_project_id(
      InProjectId, InEngineType, InEngineVersion, InApplicationDataPath, InDevice, InFeatureInfo, InSDKVersion);
  g_ngx_init_native.store(unwrapped && NVSDK_NGX_SUCCEED(result));
  static std::atomic_bool exit_logged{false};
  if (!exit_logged.exchange(true)) {
    renodx::utils::log::i("[world-rr] init exit result=", NgxResultHex(result));
  }
  return result;
}

inline const std::array<renodx::utils::detour::Export, 8> kNgxHookExports = {{
    {"NVSDK_NGX_D3D11_CreateFeature", &g_real_create_feature, &HookCreateFeature},
    {"NVSDK_NGX_D3D11_EvaluateFeature", &g_real_evaluate_feature, &HookEvaluateFeature},
    {"NVSDK_NGX_D3D11_ReleaseFeature", &g_real_release_feature, &HookReleaseFeature},
    {"NVSDK_NGX_D3D11_Shutdown", &g_real_shutdown, &HookShutdown},
    {"NVSDK_NGX_D3D11_Shutdown1", &g_real_shutdown1, &HookShutdown1},
    {"NVSDK_NGX_D3D11_Init", &g_real_init, &HookInit},
    {"NVSDK_NGX_D3D11_Init_Ext", &g_real_init_ext, &HookInitExt},
    {"NVSDK_NGX_D3D11_Init_ProjectID", &g_real_init_with_project_id, &HookInitWithProjectId},
}};

inline void ArmNgxHooks(bool preload) {
  if (g_ngx_armed.load() || g_ngx_arm_attempted.load()) {
    return;
  }
  const ArmScope arm_scope;

  // The preload LoadLibraryW runs before g_ngx_arm_mutex is taken: no lock is held across the loader call.
  HMODULE ngx = GetModuleHandleW(L"_nvngx.dll");
  if (ngx == nullptr && preload) {
    const HMODULE driver = GetModuleHandleW(L"nvwgf2umx.dll");
    if (driver != nullptr) {
      wchar_t driver_path[MAX_PATH];
      GetModuleFileNameW(driver, driver_path, MAX_PATH);
      const std::filesystem::path ngx_path = std::filesystem::path(driver_path).parent_path() / L"_nvngx.dll";
      if (std::filesystem::exists(ngx_path)) {
        ngx = LoadLibraryW(ngx_path.c_str());
      }
    }
  }

  std::unique_lock arm_lock(g_ngx_arm_mutex);
  if (g_ngx_armed.load() || g_ngx_arm_attempted.load()) {
    return;
  }
  if (ngx == nullptr) {
    return;
  }
  g_ngx_arm_attempted.store(true);

  renodx::utils::detour::ExportInstallResult install{};
  std::string error = "none";
  try {
    install = renodx::utils::detour::Install(ngx, kNgxHookExports);
    if (install.error) {
      error = install.error.message();
    }
  } catch (const std::exception& exception) {
    error = exception.what();
  }

  g_get_capability_parameters = reinterpret_cast<decltype(g_get_capability_parameters)>(
      GetProcAddress(ngx, "NVSDK_NGX_D3D11_GetCapabilityParameters"));
  g_allocate_parameters = reinterpret_cast<decltype(g_allocate_parameters)>(
      GetProcAddress(ngx, "NVSDK_NGX_D3D11_AllocateParameters"));
  g_destroy_parameters = reinterpret_cast<decltype(g_destroy_parameters)>(
      GetProcAddress(ngx, "NVSDK_NGX_D3D11_DestroyParameters"));

  const std::filesystem::path game_dir = renodx::utils::platform::GetCurrentProcessPath().parent_path();
  renodx::utils::log::i(
      "[world-rr] nvngx_dlss.dll version=", renodx::utils::platform::GetFileVersion(game_dir / L"nvngx_dlss.dll"),
      " nvngx_dlssd.dll version=", renodx::utils::platform::GetFileVersion(game_dir / L"nvngx_dlssd.dll"));

  if (install.Active() > 0u) {
    g_ngx_armed.store(true);
    renodx::utils::log::i(
        "[world-rr] hooks armed path=", NgxModulePath(ngx), " requested=", install.requested,
        " installed=", install.installed, " missing=", install.missing, " failed=", install.failed, " error=", error);
  } else {
    renodx::utils::log::w(
        "[world-rr] hooks NOT armed path=", NgxModulePath(ngx), " requested=", install.requested,
        " installed=", install.installed, " missing=", install.missing, " failed=", install.failed, " error=", error);
  }
}

inline void OnInitDeviceRr(reshade::api::device* device) {
  if (device->get_api() == reshade::api::device_api::d3d11 && RrRequested()) {
    ArmNgxHooks(true);
  }
}

inline void OnRrPresent(
    reshade::api::command_queue* queue,
    reshade::api::swapchain* swapchain,
    const reshade::api::rect* source_rect,
    const reshade::api::rect* dest_rect,
    uint32_t dirty_rect_count,
    const reshade::api::rect* dirty_rects) {
  if (g_rr_perf_present < 0.5f) return;  // TEMP perf test (remove)
  if (!RrRequested()) {
    g_rr_running.store(false);
    g_rr_reason.store(RrReason::Off);
    ClearRrFailure();
    return;
  }
  ArmNgxHooks(false);
  const uint32_t frame = g_state.frame.load();
  const bool failed = g_rr_failed.load();
  const uint32_t last_ok = g_rr_last_ok_frame.load();
  const uint32_t last_sr = g_rr_last_sr_eval_frame.load();
  g_rr_running.store(!failed && last_ok != UINT32_MAX && frame - last_ok <= kRrMissFrames);
  if (!failed) {
    if (!g_ngx_armed.load()) {
      g_rr_reason.store(RrReason::HooksNotArmed);
    } else if (last_sr == UINT32_MAX || frame - last_sr > kRrMissFrames) {
      g_rr_reason.store(RrReason::NoDlss);
    }
  }
  if (g_rr_debug_log < 0.5f || g_state.frame.load() % 600u != 0u) {
    return;
  }
  const uint32_t evaluates = g_ngx_evaluate_calls.load(std::memory_order_relaxed);
  if (evaluates == g_ngx_logged_evaluates.load(std::memory_order_relaxed)) {
    return;
  }
  g_ngx_logged_evaluates.store(evaluates, std::memory_order_relaxed);
  renodx::utils::log::i(
      "[world-rr] ngx: armed=", g_ngx_armed.load(), " creates=", g_ngx_create_calls.load(std::memory_order_relaxed),
      " evaluates=", evaluates, " releases=", g_ngx_release_calls.load(std::memory_order_relaxed),
      " module=", NgxModulePath(GetModuleHandleW(L"_nvngx.dll")));
}

// Loader hooks: arm the NGX hooks the moment _nvngx.dll loads, before the game's first NGX call (its Init). Each hook
// calls the real loader first and holds no lock across it or across ArmNgxHooks.
inline decltype(&LoadLibraryW) g_real_load_library_w = nullptr;
inline decltype(&LoadLibraryExW) g_real_load_library_ex_w = nullptr;
inline decltype(&LoadLibraryA) g_real_load_library_a = nullptr;
inline decltype(&LoadLibraryExA) g_real_load_library_ex_a = nullptr;
inline std::atomic_bool g_loader_hooks_active{false};

inline void ArmOnNgxLoad(HMODULE module) {
  if (module == nullptr || !RrRequested()) return;
  wchar_t path[MAX_PATH] = {};
  if (GetModuleFileNameW(module, path, MAX_PATH) == 0) return;
  const std::wstring name = std::filesystem::path(path).filename().wstring();
  constexpr std::wstring_view kSuffix = L"_nvngx.dll";
  if (name.size() < kSuffix.size() || _wcsicmp(name.c_str() + name.size() - kSuffix.size(), kSuffix.data()) != 0) return;
  static std::atomic_bool armed_logged{false};
  if (!armed_logged.exchange(true)) {
    renodx::utils::log::i("[world-rr] armed on load of _nvngx.dll");
  }
  ArmNgxHooks(false);
}

inline HMODULE WINAPI HookLoadLibraryW(LPCWSTR file_name) {
  const HMODULE result = g_real_load_library_w(file_name);
  if (!g_in_arm) {
    ArmOnNgxLoad(result);
  }
  return result;
}

inline HMODULE WINAPI HookLoadLibraryExW(LPCWSTR file_name, HANDLE file, DWORD flags) {
  const HMODULE result = g_real_load_library_ex_w(file_name, file, flags);
  if (!g_in_arm) {
    ArmOnNgxLoad(result);
  }
  return result;
}

inline HMODULE WINAPI HookLoadLibraryA(LPCSTR file_name) {
  const HMODULE result = g_real_load_library_a(file_name);
  if (!g_in_arm) {
    ArmOnNgxLoad(result);
  }
  return result;
}

inline HMODULE WINAPI HookLoadLibraryExA(LPCSTR file_name, HANDLE file, DWORD flags) {
  const HMODULE result = g_real_load_library_ex_a(file_name, file, flags);
  if (!g_in_arm) {
    ArmOnNgxLoad(result);
  }
  return result;
}

inline const std::array<renodx::utils::detour::Export, 4> kLoaderHookExports = {{
    {"LoadLibraryW", &g_real_load_library_w, &HookLoadLibraryW},
    {"LoadLibraryExW", &g_real_load_library_ex_w, &HookLoadLibraryExW},
    {"LoadLibraryA", &g_real_load_library_a, &HookLoadLibraryA},
    {"LoadLibraryExA", &g_real_load_library_ex_a, &HookLoadLibraryExA},
}};

inline void InstallLoaderHooks() {
  try {
    const auto install = renodx::utils::detour::Install(GetModuleHandleW(L"kernel32.dll"), kLoaderHookExports);
    g_loader_hooks_active.store(install.Active() > 0u);
    if (!g_loader_hooks_active.load()) {
      renodx::utils::log::w("[world-rr] loader hooks NOT installed: init_device preload is the only arming path");
    }
  } catch (const std::exception& exception) {
    renodx::utils::log::w("[world-rr] loader hooks failed: ", exception.what());
  }
}

inline void Use(DWORD fdw_reason) {
  switch (fdw_reason) {
    case DLL_PROCESS_ATTACH: {
      reshade::register_event<reshade::addon_event::init_device>(OnInitDeviceRr);
      reshade::register_event<reshade::addon_event::present>(OnRrPresent);
      InstallLoaderHooks();
      break;
    }
    case DLL_PROCESS_DETACH: {
      reshade::unregister_event<reshade::addon_event::init_device>(OnInitDeviceRr);
      reshade::unregister_event<reshade::addon_event::present>(OnRrPresent);
      if (g_loader_hooks_active.load()) {
        try {
          renodx::utils::detour::Uninstall(kLoaderHookExports);
        } catch (const std::exception& exception) {
          renodx::utils::log::w("[world-rr] loader uninstall failed: ", exception.what());
        }
        g_loader_hooks_active.store(false);
      }
      if (g_ngx_armed.load()) {
        std::unique_lock arm_lock(g_ngx_arm_mutex);
        try {
          renodx::utils::detour::Uninstall(kNgxHookExports);
        } catch (const std::exception& exception) {
          renodx::utils::log::w("[world-rr] uninstall failed: ", exception.what());
        }
        g_ngx_armed.store(false);
      }
      break;
    }
    default:
      break;
  }
}

}  // namespace falcom_world::rr
