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
#define CORTAO_16_BIT 0 //Not recommended for main pass, requires enable-16bit-types
#define CORTAO_SAMPLE_COUNT 16
#define CORTAO_RAY_COUNT 16
#define CORTAO_OCCLUDER_LOOKUP_TABLE 1

//Include the shader
#include <CORTAOMainPass.h>

//Parameters as root constants
ConstantBuffer<CORTAOMainPassParameters> parameters : register(b0);

//Texture resources
Texture2D<cortao_uint> packedDataBuffer : register(t0); //R32_UINT
Texture2D<cortao_half3> normalBuffer : register(t1); //User defined format
Texture3D<cortao_uint> occluderLookupTable : register(t2); //R16_UINT to match CORTAO_RAY_COUNT

//Sampler resources
SamplerState nearestBorderSampler : register(s0); //FILTER_MIN_MAG_MIP_POINT, TEXTURE_ADDRESS_BORDER
SamplerState nearestClampSampler : register(s1); //FILTER_MIN_MAG_MIP_POINT, TEXTURE_ADDRESS_CLAMP

//Image resources
RWTexture2D<cortao_half> outputBuffer : register(u0); //R8_UNORM

//Input callback implementations
cortao_uint cortao_samplePackedData(cortao_float2 texCoords, cortao_float mipmap) {
	return packedDataBuffer.SampleLevel(nearestBorderSampler, texCoords, mipmap); //requires SM6.7 for integer sampling
}

cortao_half3 cortao_loadNormal(cortao_int2 pixelCoords) {
	cortao_half3 normal = normalBuffer.Load(int3(pixelCoords, 0));
	//Do any normal transformation/unpacking/decoding here.
	return normal;
}

cortao_uint cortao_sampleOccluderLookupTable(cortao_float3 texCoords) {
	return occluderLookupTable.SampleLevel(nearestClampSampler, texCoords, 0.0); //requires SM6.7 for integer sampling
}

//Output callback implementation
void cortao_writeResult(cortao_int2 pixelCoords, cortao_half occlusion, cortao_half3 bentNormal, cortao_half3x3 tangentSpaceTransform, cortao_uint rayBits) {
	outputBuffer[pixelCoords] = occlusion;
}

//Reorders the threads inside the workgroup to be more cache coherent
uint2 deinterleave(uint index) {
	uint2 p = uint2(index, index) >> uint2(0, 1);
	
	p &= 0x5555u;
	p = (p ^ (p >> 1u)) & 0x3333u;
	p = (p ^ (p >> 2u)) & 0x0f0fu;
	p = (p ^ (p >> 4u)) & 0x00ffu;

	return p;
}

#define WORKGROUP_SIZE 16 //or 32
[numthreads(WORKGROUP_SIZE, WORKGROUP_SIZE, 1)]
void main(uint2 workgroupID : SV_GroupID, uint localIndex : SV_GroupIndex) {
	uint2 pixelCoords = workgroupID * uint2(WORKGROUP_SIZE, WORKGROUP_SIZE) + deinterleave(localIndex);
	cortao_mainPass(pixelCoords, parameters);
}

