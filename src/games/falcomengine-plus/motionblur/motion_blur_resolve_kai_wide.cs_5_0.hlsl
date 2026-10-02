// Motion Blur — wide resolve variant (camera-term debug path). KAI.
//
// The shipping resolve writes only the blended .xy into an RG16F surface, which
// halves the bytes per texel on the texture the gather fetches once per tap.
// Debug views 8 (Camera Velocity) and 9 (Object Residual) read the camera-only
// term from .zw, so while either is selected the addon allocates the resolve
// surface as RGBA16F and binds this pipeline instead. Both pipelines exist from
// startup; RunMotionBlur only selects between them, and the surface is rebuilt
// on the toggle (the same one-frame cost as changing Max Radius).
//
// Same source, one macro. The whole rationale lives in
// motion_blur_resolve_kai.cs_5_0.hlsl; this file exists only so the preprocessor
// sees the wide branch at compile time.
#define MB_RESOLVE_WIDE 1
#include "motion_blur_resolve_kai.cs_5_0.hlsl"
