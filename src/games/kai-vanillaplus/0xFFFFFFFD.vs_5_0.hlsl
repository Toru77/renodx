struct V2P {
  float4 position : SV_Position;
  float2 uv : TEXCOORD0;
};

V2P main(uint vertex_id : SV_VertexID) {
  V2P output;
  float2 uv = float2((vertex_id << 1) & 2, vertex_id & 2);
  output.uv = uv;
  output.position = float4(uv * float2(2, -2) + float2(-1, 1), 0, 1);
  return output;
}
