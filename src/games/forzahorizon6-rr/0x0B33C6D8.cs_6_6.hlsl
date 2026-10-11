// forzahorizon6-rr: normal/depth-weighted reconstruct — medium RT-quality variant.
//
// Original (0x0B33C6D8): 16x8 pass, 10x6 groupshared tile. Upsamples the
// sub-res radiance pair (t22/t23) to full res with a 3x3 normal/depth-weighted
// filter (per-tap packed normal + linear depth from t38). This replacement
// reproduces that exact weighted upsample (no cross-frame state); the pair is
// sampled at the game's own input scale (m[20]). Shared body in
// resolve_recon_weighted.hlsli.
#define FH6_RECON_TX 16
#define FH6_RECON_TY 8
#include "./resolve_recon_weighted.hlsli"
