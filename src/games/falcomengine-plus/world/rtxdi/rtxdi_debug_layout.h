#ifndef FALCOM_WORLD_RTXDI_DEBUG_LAYOUT_H_
#define FALCOM_WORLD_RTXDI_DEBUG_LAYOUT_H_

#define RTXDI_DEBUG_VIEW_OFF                0u
#define RTXDI_DEBUG_VIEW_LOCAL_STATUS       1u
#define RTXDI_DEBUG_VIEW_RECEIVER_CLASS     2u
#define RTXDI_DEBUG_VIEW_SELECTED_LIGHT     3u
#define RTXDI_DEBUG_VIEW_RAW_DIFFUSE        4u
#define RTXDI_DEBUG_VIEW_FILTERED_DIFFUSE   5u
#define RTXDI_DEBUG_VIEW_DISTANCE_VALIDITY  6u
#define RTXDI_DEBUG_VIEW_DISTANCE_MAGNITUDE 7u
#define RTXDI_DEBUG_VIEW_CONSUMER_SELECTION 8u
#define RTXDI_DEBUG_VIEW_SUN_VISIBILITY     9u
// Unified PT views (A18): SHARC, Material and AO are written by the initial pass, the others by final shading.
#define PT_DEBUG_VIEW_SHARC                 10u
#define PT_DEBUG_VIEW_MATERIAL              11u
#define PT_DEBUG_VIEW_RAW                   12u
#define PT_DEBUG_VIEW_RESTIR                13u
#define PT_DEBUG_VIEW_FINAL_DIFFUSE         14u
#define PT_DEBUG_VIEW_FINAL_SPECULAR        15u
#define PT_DEBUG_VIEW_AO                    16u

// PT ReSTIR view channel (PtDebugChannel).
#define PT_DEBUG_CHANNEL_UCW               0u
#define PT_DEBUG_CHANNEL_M                 1u
#define PT_DEBUG_CHANNEL_AGE               2u
#define PT_DEBUG_CHANNEL_PATH_LENGTH       3u
#define PT_DEBUG_CHANNEL_RC_LENGTH         4u
#define PT_DEBUG_CHANNEL_INITIAL_VS_REUSED 5u

#define RTXDI_SUN_DETAIL_STATUS       0u
#define RTXDI_SUN_DETAIL_REASON       1u
#define RTXDI_SUN_DETAIL_HIT_DISTANCE 2u
#define RTXDI_SUN_DETAIL_LAYERS       3u
#define RTXDI_SUN_DETAIL_BLOCKER      4u

#define RTXDI_DEBUG_CONSUMER_UNAVAILABLE 0u
#define RTXDI_DEBUG_CONSUMER_FILTERED    1u
#define RTXDI_DEBUG_CONSUMER_RAW         2u
#define RTXDI_DEBUG_CONSUMER_BLOCKED     3u
#define RTXDI_DEBUG_CONSUMER_EMPTY       4u
#define RTXDI_DEBUG_CONSUMER_FALLBACK    5u
#define RTXDI_DEBUG_CONSUMER_INVALID     6u

#define RTXDI_DEBUG_REASON_NONE                0u
#define RTXDI_DEBUG_REASON_BACKEND_UNAVAILABLE 1u
#define RTXDI_DEBUG_REASON_NOT_PRODUCED        2u
#define RTXDI_DEBUG_REASON_NO_SDK              3u
#define RTXDI_DEBUG_REASON_RR_SUPPRESSED       4u
#define RTXDI_DEBUG_REASON_WAITING_INPUT       5u
#define RTXDI_DEBUG_REASON_STALE_IDENTITY      6u
#define RTXDI_DEBUG_REASON_INVALID_RESOURCE    7u
#define RTXDI_DEBUG_REASON_BVH_SUPERSEDED      8u
#define RTXDI_DEBUG_REASON_PRODUCER_DISABLED   9u

#ifdef __cplusplus
#include <cstdint>
enum class RtxdiDebugView : uint32_t {
  Off = RTXDI_DEBUG_VIEW_OFF,
  LocalStatus = RTXDI_DEBUG_VIEW_LOCAL_STATUS,
  ReceiverClass = RTXDI_DEBUG_VIEW_RECEIVER_CLASS,
  SelectedLight = RTXDI_DEBUG_VIEW_SELECTED_LIGHT,
  RawDiffuse = RTXDI_DEBUG_VIEW_RAW_DIFFUSE,
  FilteredDiffuse = RTXDI_DEBUG_VIEW_FILTERED_DIFFUSE,
  DistanceValidity = RTXDI_DEBUG_VIEW_DISTANCE_VALIDITY,
  DistanceMagnitude = RTXDI_DEBUG_VIEW_DISTANCE_MAGNITUDE,
  ConsumerSelection = RTXDI_DEBUG_VIEW_CONSUMER_SELECTION,
  SunVisibility = RTXDI_DEBUG_VIEW_SUN_VISIBILITY,
  PtSharc = PT_DEBUG_VIEW_SHARC,
  PtMaterial = PT_DEBUG_VIEW_MATERIAL,
  PtRaw = PT_DEBUG_VIEW_RAW,
  PtRestir = PT_DEBUG_VIEW_RESTIR,
  PtFinalDiffuse = PT_DEBUG_VIEW_FINAL_DIFFUSE,
  PtFinalSpecular = PT_DEBUG_VIEW_FINAL_SPECULAR,
  PtAo = PT_DEBUG_VIEW_AO,
};

inline constexpr const char* kRtxdiDebugViewNames[] = {
    "Off", "Local status", "Receiver class", "Selected light/type", "Raw diffuse", "Filtered diffuse",
    "Distance validity", "Distance magnitude", "Consumer selection", "Sun visibility", "PT SHARC", "PT Material",
    "PT Raw", "PT ReSTIR", "PT Final Diffuse", "PT Final Specular", "PT AO"};
static_assert(sizeof(kRtxdiDebugViewNames) / sizeof(kRtxdiDebugViewNames[0]) == PT_DEBUG_VIEW_AO + 1u);

inline constexpr const char* kPtDebugChannelNames[] = {"UCW", "M", "Age", "Path length", "Rc length",
                                                       "Initial vs reused"};
static_assert(sizeof(kPtDebugChannelNames) / sizeof(kPtDebugChannelNames[0]) == PT_DEBUG_CHANNEL_INITIAL_VS_REUSED + 1u);

enum class RtxdiDebugPresentationOwner : uint32_t {
  Lighting,
  Rtxdi,
  Bvh,
};

constexpr RtxdiDebugPresentationOwner ResolveRtxdiDebugPresentationOwner(uint32_t requested_view, bool bvh_active) {
  if (bvh_active) return RtxdiDebugPresentationOwner::Bvh;
  if (requested_view != RTXDI_DEBUG_VIEW_OFF) return RtxdiDebugPresentationOwner::Rtxdi;
  return RtxdiDebugPresentationOwner::Lighting;
}

constexpr uint32_t ResolveRtxdiDebugEffectiveView(uint32_t requested_view, bool bvh_active) {
  return bvh_active ? RTXDI_DEBUG_VIEW_OFF : requested_view;
}

constexpr bool RtxdiDebugDistanceExportRequested(uint32_t effective_view) {
  return effective_view == RTXDI_DEBUG_VIEW_DISTANCE_VALIDITY
         || effective_view == RTXDI_DEBUG_VIEW_DISTANCE_MAGNITUDE;
}

constexpr bool RtxdiSunDetailExportRequested(uint32_t effective_view, uint32_t detail_view) {
  return effective_view == RTXDI_DEBUG_VIEW_SUN_VISIBILITY
         && (detail_view == RTXDI_SUN_DETAIL_REASON || detail_view == RTXDI_SUN_DETAIL_HIT_DISTANCE
             || detail_view == RTXDI_SUN_DETAIL_LAYERS || detail_view == RTXDI_SUN_DETAIL_BLOCKER);
}
#endif

#endif  // FALCOM_WORLD_RTXDI_DEBUG_LAYOUT_H_
