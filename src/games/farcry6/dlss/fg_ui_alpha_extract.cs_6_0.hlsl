Texture2D<float4> scene_with_ui : register(t0);
RWTexture2D<float> ui_alpha : register(u0);

cbuffer Parameters : register(b0) {
  uint2 extent;
};

[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID) {
  if (any(id.xy >= extent)) return;
  ui_alpha[id.xy] = saturate(scene_with_ui.Load(int3(id.xy, 0)).a);
}
