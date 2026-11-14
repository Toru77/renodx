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

#version 460 core

//Enable extension for loop unrolling, if supported.
#extension GL_EXT_control_flow_attributes : enable

//Allows texelFetch() without a sampler in input callbacks.
#extension GL_EXT_samplerless_texture_functions : require

//Required for 16-bit support
#extension GL_EXT_shader_explicit_arithmetic_types : require

//CORTAO defines
#define CORTAO_GLSL 1
#define CORTAO_PERSPECTIVE_PROJECTION 1
#define CORTAO_16_BIT 0 //Not recommended for main pass
#define CORTAO_SAMPLE_COUNT 16
#define CORTAO_RAY_COUNT 16
#define CORTAO_OCCLUDER_LOOKUP_TABLE 1
#define CORTAO_BENT_NORMAL_LOOKUP_TABLE 1

//Include the shader
#include <CORTAOMainPass.h>

//Parameters in push constants
layout(push_constant) uniform CORTAOMainPassPushUniforms {
	CORTAOMainPassParameters parameters;
};

//Texture resources
layout(set = 0, binding = 0) uniform utexture2D packedDataBuffer; //R32_UINT with mipmaps
layout(set = 0, binding = 1) uniform texture2D normalBuffer; //User defined format
layout(set = 0, binding = 2) uniform utexture3D occluderLookupTable; //R16_UINT to match CORTAO_RAY_COUNT
layout(set = 0, binding = 3) uniform samplerBuffer bentNormalLookupTable; //R8G8_SNORM

//Sampler resources
layout(set = 0, binding = 4) uniform sampler nearestBorderSampler; //NEAREST min/mag/mip filtering, CLAMP_TO_BORDER
layout(set = 0, binding = 5) uniform sampler nearestClampSampler; //NEAREST min/mag/mip filtering, CLAMP_TO_EDGE

//Image resources
layout(set = 0, binding = 6) uniform restrict writeonly image2D outputBuffer; //R8G8B8A8_SNORM

//Input callback implementations
cortao_uint cortao_samplePackedData(cortao_float2 texCoords, cortao_float mipmap) {
	return textureLod(usampler2D(packedDataBuffer, nearestBorderSampler), texCoords, mipmap).r;
}

cortao_half3 cortao_loadNormal(cortao_int2 pixelCoords) {
	cortao_half3 normal = cortao_half3(texelFetch(normalBuffer, pixelCoords, 0).xyz);
	//Do any normal transformation/unpacking/decoding here.
	return normal;
}

cortao_uint cortao_sampleOccluderLookupTable(cortao_float3 texCoords) {
	return textureLod(usampler3D(occluderLookupTable, nearestClampSampler), texCoords, 0.0f).r;
}

cortao_half3 cortao_loadBentNormalLookupTable(cortao_uint rayBits) {
	cortao_half2 bentNormal = texelFetch(bentNormalLookupTable, cortao_int(rayBits)).xy;
	return cortao_half3(bentNormal, sqrt(cortao_half(1.0) - dot(bentNormal, bentNormal))); //reconstruct z
}

//Output callback implementation
void cortao_writeResult(cortao_int2 pixelCoords, cortao_half occlusion, cortao_half3 bentNormal, cortao_half3x3 tangentSpaceTransform, cortao_uint rayBits) {
	imageStore(outputBuffer, pixelCoords, cortao_float4(occlusion, bentNormal));
}

//Reorders the threads inside the workgroup to be more cache coherent
uvec2 deinterleave(uint index) {
	uvec2 p = uvec2(index, index) >> uvec2(0, 1);
	
	p &= 0x5555u;
	p = (p ^ (p >> 1u)) & 0x3333u;
	p = (p ^ (p >> 2u)) & 0x0f0fu;
	p = (p ^ (p >> 4u)) & 0x00ffu;

	return p;
}

//Main function
layout(local_size_x = 16, local_size_y = 16) in; //or 32x32
void main() {
	uvec2 pixelCoords = gl_WorkGroupID.xy * gl_WorkGroupSize.xy + deinterleave(gl_LocalInvocationIndex);
	cortao_mainPass(pixelCoords, parameters);
}
