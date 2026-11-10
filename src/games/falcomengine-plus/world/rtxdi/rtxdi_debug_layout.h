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
};

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
#endif

#endif  // FALCOM_WORLD_RTXDI_DEBUG_LAYOUT_H_
