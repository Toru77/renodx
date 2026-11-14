// CORTAO Reference Implementation v1.0.1
//
// MIT License
//
// Copyright (c) 2026 Daniel Isheden
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

//CORTAO defines
#define CORTAO_HLSL 1
#define CORTAO_PERSPECTIVE_PROJECTION 1
#define CORTAO_16_BIT 1 //Recommended for the filter pass, requires enable-16bit-types
#define CORTAO_LARGE_FILTER 1
#define CORTAO_FILTER_SYMMETRY 1

//Include the shader
#include <CORTAOFilterPass.h>

//Parameters as root constants
ConstantBuffer<CORTAOFilterPassParameters> parameters : register(b0);

//Texture resources
Texture2D<cortao_uint> packedDataBuffer : register(t0); //R32_UINT
Texture2D<cortao_half> ssaoBuffer; //R8_UNORM

//Sampler resources
SamplerState nearestBorderSampler : register(s0); //FILTER_MIN_MAG_MIP_POINT, TEXTURE_ADDRESS_BORDER

//Image resources
RWTexture2D<cortao_half> outputBuffer : register(u0); //R8_UNORM

//Input callback implementations
cortao_uint4 cortao_gatherPackedData(cortao_float2 texCoords, cortao_int2 offset) {
	return packedDataBuffer.Gather(nearestBorderSampler, texCoords, offset); //requires SM6.7 for integer sampling
}

cortao_half4 cortao_gatherSSAO(cortao_float2 texCoords, cortao_int2 offset) {
	return ssaoBuffer.Gather(nearestBorderSampler, texCoords, offset);
}

//Output callback implementation
void cortao_writeFilterOutput(cortao_int2 pixelCoords, cortao_half ssao) {
	outputBuffer[pixelCoords] = ssao;
}

//Main function
[numthreads(8, 8, 1)] //Each thread processes a 2x2 pixel block, so effectively 16x16
void main(uint2 threadID : SV_DispatchThreadID) {
	cortao_filterPass(threadID, parameters);
}
