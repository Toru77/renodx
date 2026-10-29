// Matches the game's SDR copy or HDR10 output pass for the HUD-less image.
// HDR10 uses the output pass's b1 c18.y; zero selects the SDR copy.
Texture2D<float4> source : register(t0);
RWTexture2D<float4> target : register(u0);

cbuffer OutputParameters : register(b0) {
  uint2 output_size;
  float output_brightness;
};

[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID) {
  if (any(id.xy >= output_size)) return;

  float3 encoded = source.Load(int3(id.xy, 0)).rgb;
  // The SDR output pass copies this image directly to an UNORM backbuffer.
  if (output_brightness <= 0.f) {
    target[id.xy] = float4(saturate(encoded), 1.f);
    return;
  }
  float3 linear_color = float3(
      encoded.r >= 0.03928571194410324f ? pow(abs((encoded.r + 0.055f) * 0.9478673338890076f), 2.4f) : encoded.r * 0.0773801133f,
      encoded.g >= 0.03928571194410324f ? pow(abs((encoded.g + 0.055f) * 0.9478673338890076f), 2.4f) : encoded.g * 0.0773801133f,
      encoded.b >= 0.03928571194410324f ? pow(abs((encoded.b + 0.055f) * 0.9478673338890076f), 2.4f) : encoded.b * 0.0773801133f);

  linear_color *= output_brightness * 0.0002f;
  float3 intermediate = float3(
      mad(0.04578f, linear_color.b, mad(0.31119f, linear_color.g, linear_color.r * 0.64304f)),
      mad(0.0093f, linear_color.b, mad(0.93144f, linear_color.g, linear_color.r * 0.05927f)),
      mad(0.93012f, linear_color.b, mad(0.06392f, linear_color.g, linear_color.r * 0.00596f)));
  float3 display = float3(
      mad(-0.00181166f, intermediate.b, mad(0.0251869f, intermediate.g, intermediate.r * 0.966957986f)),
      mad(0.00182386f, intermediate.b, mad(0.981899f, intermediate.g, intermediate.r * 0.0168175f)),
      mad(0.942865f, intermediate.b, mad(0.0234305f, intermediate.g, intermediate.r * 0.0140284f)));
  display = saturate(display);
  float3 power = pow(abs(display), 0.1593017578125f);
  target[id.xy] = float4(pow(abs((power * 18.8515625f + 0.8359375f) / (power * 18.6875f + 1.f)), 78.84375f), 1.f);
}
