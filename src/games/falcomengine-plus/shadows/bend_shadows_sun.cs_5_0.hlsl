// Bend screen-space shadows for the sun (t34 contact term). The kernel is bend_shadows.hlsli.
#include "../shared.h"
#define BEND_SAMPLE_COUNT_VALUE shader_injection_data.cs_bend_sample_count
#define BEND_SURFACE_THICKNESS shader_injection_data.cs_bend_surface_thickness
#define BEND_BILINEAR_THRESHOLD shader_injection_data.cs_bend_bilinear_threshold
#define BEND_SHADOW_CONTRAST shader_injection_data.cs_bend_shadow_contrast
#define BEND_IGNORE_EDGE_PIXELS (shader_injection_data.cs_bend_ignore_edge_pixels > 0.5f)
#define BEND_USE_PRECISION_OFFSET (shader_injection_data.cs_bend_use_precision_offset > 0.5f)
#define BEND_BILINEAR_OFFSET_MODE (shader_injection_data.cs_bend_bilinear_offset_mode > 0.5f)
#define BEND_WORLD_SPACE (shader_injection_data.cs_bend_world_space > 0.5f)
#define BEND_WORLD_THICKNESS shader_injection_data.cs_bend_world_thickness
#define BEND_WORLD_LENGTH shader_injection_data.cs_bend_world_length
#define BEND_ISFAST (shader_injection_data.cs_bend_isfast > 0.5f)
#define BEND_ISFAST_AMOUNT shader_injection_data.cs_bend_isfast_amount
#define BEND_CHAR_RECEIVE (shader_injection_data.cs_bend_char_receive > 0.5f)
#define BEND_CHAR_BIAS shader_injection_data.cs_bend_char_bias
#define BEND_DEBUG_VIEW(slot) ((int)shader_injection_data.cs_contact_debug)
RWTexture2D<float4> g_outContact : register(u0);
#define BEND_WRITE(slot, xy, result) g_outContact[(xy)] = float4((result), 0.0, 0.0, 1.0)
#include "bend_shadows.hlsli"
