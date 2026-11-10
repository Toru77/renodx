#pragma once

#ifdef __cplusplus
#include <cstdint>
#else
#define uint32_t uint
#endif

#define RTXDI_INPUT_CARRIER_WIDTH       4096u
#define RTXDI_INPUT_SCENE_REQUIRED_ROWS 80u
#define RTXDI_INPUT_SCENE_ROWS          96u
#define RTXDI_INPUT_LIGHT_STRIDE        112u
#define RTXDI_INPUT_TILE_STRIDE         576u
#define RTXDI_INPUT_DEFERRED_STRIDE     144u
#define RTXDI_INPUT_MAX_LIGHTS          1024u
#define RTXDI_INPUT_MAX_TILES           4608u
#define RTXDI_INPUT_MAX_DEFERRED        20000u
#define RTXDI_INPUT_LIGHT_ROWS          2u
#define RTXDI_INPUT_TILE_ROWS           41u
#define RTXDI_INPUT_DEFERRED_ROWS       44u

struct RtxdiInputLayout {
  uint32_t width, height, motion_available, flags;
  uint32_t lights_source, lights_copied, lights_truncated, tiles_source;
  uint32_t tiles_copied, deferred_source, deferred_copied, reserved;
};

#ifdef __cplusplus
static_assert(sizeof(RtxdiInputLayout) == 48u);
static_assert(RTXDI_INPUT_SCENE_REQUIRED_ROWS * 16u == 1280u);
static_assert(RTXDI_INPUT_SCENE_ROWS * 16u == 1536u);
static_assert(RTXDI_INPUT_LIGHT_ROWS * RTXDI_INPUT_CARRIER_WIDTH * 16u >= RTXDI_INPUT_MAX_LIGHTS * RTXDI_INPUT_LIGHT_STRIDE);
static_assert(RTXDI_INPUT_TILE_ROWS * RTXDI_INPUT_CARRIER_WIDTH * 16u >= RTXDI_INPUT_MAX_TILES * RTXDI_INPUT_TILE_STRIDE);
static_assert(RTXDI_INPUT_DEFERRED_ROWS * RTXDI_INPUT_CARRIER_WIDTH * 16u >= RTXDI_INPUT_MAX_DEFERRED * RTXDI_INPUT_DEFERRED_STRIDE);
#endif
