#include "../rtxdi_frame_parameters.h"
#include "../rtxdi_input_layout.h"
ConstantBuffer<RtxdiFrameParameters> frame : register(b0);
ConstantBuffer<RtxdiInputLayout> inputs : register(b1);
RWStructuredBuffer<uint> output : register(u0);
[numthreads(1, 1, 1)]
void main() {
  output[0] = inputs.width + inputs.lights_copied + frame.width + frame.height + frame.history_valid + frame.flags
      + frame.runtime.neighborOffsetMask + frame.lights.localLightBufferRegion.numLights
      + frame.di.initialSamplingParams.numLocalLightSamples + frame.di.shadingParams.enableFinalVisibility;
}
