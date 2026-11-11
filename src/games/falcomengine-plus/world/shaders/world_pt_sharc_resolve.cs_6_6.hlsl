#define SHARC_UPDATE 0
#include "world_pt_common.hlsli"
// g_sharc_mode 1: zero every entry (cache clear). 0: SHARC resolve (OpenRemix sharc-resolve.comp).
[numthreads(128, 1, 1)]
void main(uint3 id : SV_DispatchThreadID) {
  if (g_sharc_mode == 1u) {
    if (id.x < g_sharc_capacity) {
      g_pt_sharc_hash[id.x] = 0;
      g_pt_sharc_accumulation[id.x] = (SharcAccumulationData)0;
      g_pt_sharc_resolved[id.x] = (SharcPackedData)0;
    }
    return;
  }
  SharcResolveParameters resolve;
  resolve.cameraPositionPrev = g_sharc_camera_prev;
  resolve.accumulationFrameNum = g_sharc_window;
  resolve.responsiveFrameNum = 1u;  // OpenRemix sharc-resolve.comp:22
  resolve.staleFrameNumMax = g_sharc_stale_frames;
  resolve.frameIndex = g_frame;
  SharcResolveEntry(id.x, PtSharcParameters(), resolve);
}
