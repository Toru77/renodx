// forzahorizon6-rr: normal/depth-weighted reconstruct — low RT-quality variant.
//
// Original (0xE08B5474): 8x8 pass, 4x4 groupshared tile. Same 3x3
// normal/depth-weighted upsample of the radiance pair (t22/t23) as the medium
// tier. This replacement reproduces that exact weighted upsample (no
// cross-frame state); the pair is sampled at the game's own input scale
// (m[20]). Shared body in resolve_recon_weighted.hlsli.
#define FH6_RECON_TX 8
#define FH6_RECON_TY 8
#include "./resolve_recon_weighted.hlsli"
