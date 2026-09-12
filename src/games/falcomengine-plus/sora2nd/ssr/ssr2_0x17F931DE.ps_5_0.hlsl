// ---- Created with 3Dmigoto v1.4.1 on Fri Aug 21 11:50:58 2026
// RenoDX PASSTHROUGH (hash kept): when SSR Replacement is on, ssr1's slot runs the
// DynCube composite and this pass forwards it untouched to RTV0 (no vanilla march
// input to denoise, no temporal history read). Vanilla temporal denoise is intentionally
// NOT run on replacement output. When the toggle is off, the game draws vanilla
// (this file is bypassed/replaced only under the same gate).

SamplerState samLinear_s : register(s0);
Texture2D<float4> ssrTexture : register(t0);

void main(
  float4 v0 : SV_Position0,
  float4 v1 : TEXCOORD0,
  out float4 o0 : SV_Target0)
{
  o0 = ssrTexture.SampleLevel(samLinear_s, v1.xy, 0);
  return;
}
