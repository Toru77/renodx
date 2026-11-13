#include "world_regir_shared.hlsli"

struct SelectionInput {
  uint x24;
  uint light_count;
  uint slots[8];
};

StructuredBuffer<SelectionInput> g_inputs : register(t0);
RWStructuredBuffer<PtReGirSelection> g_outputs : register(u0);

[numthreads(64, 1, 1)]
void main(uint3 dispatch_id : SV_DispatchThreadID) {
  uint input_count, input_stride, output_count, output_stride;
  g_inputs.GetDimensions(input_count, input_stride);
  g_outputs.GetDimensions(output_count, output_stride);
  if (dispatch_id.x >= min(input_count, output_count)) return;
  const SelectionInput input = g_inputs[dispatch_id.x];
  g_outputs[dispatch_id.x] = PtReGirSelect(input.x24, input.light_count, input.slots);
}
