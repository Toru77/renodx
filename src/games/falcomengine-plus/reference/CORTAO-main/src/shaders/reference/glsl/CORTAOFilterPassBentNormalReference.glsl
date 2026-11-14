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

//Required for 16-bit support
#extension GL_EXT_shader_explicit_arithmetic_types : require

//CORTAO defines
#define CORTAO_GLSL 1
#define CORTAO_PERSPECTIVE_PROJECTION 1
#define CORTAO_16_BIT 1 //Recommended for the filter pass
#define CORTAO_FILTER_BENT_NORMAL 1
#define CORTAO_LARGE_FILTER 1
#define CORTAO_FILTER_SYMMETRY 1

//Include the shader
#include <CORTAOFilterPass.h>

//Parameters in push constants
layout(push_constant) uniform CORTAOFilterPassPushUniforms  {
	CORTAOFilterPassParameters parameters;
};

//Texture resources
layout(set = 0, binding = 0) uniform utexture2D packedDataBuffer; //R32_UINT with mipmaps
layout(set = 0, binding = 1) uniform utexture2D ssaoBuffer; //R32_UINT (packed R8G8B8A8_SNORM)

//Sampler resources
layout(set = 0, binding = 2) uniform sampler nearestBorderSampler; //NEAREST min/mag/mip filtering, CLAMP_TO_BORDER

//Image resources
layout(set = 0, binding = 3) uniform restrict writeonly image2D outputBuffer; //R8_UNORM

//Input callback implementations
cortao_uint4 cortao_gatherPackedData(cortao_float2 texCoords, cortao_int2 offset) {
	return textureGatherOffset(usampler2D(packedDataBuffer, nearestBorderSampler), texCoords, offset);
}

cortao_half4x4 cortao_gatherSSAO(cortao_float2 texCoords, cortao_int2 offset) {
	cortao_uint4 packedOcclusionAndBentNormal = textureGatherOffset(usampler2D(ssaoBuffer, nearestBorderSampler), texCoords, offset);
	return cortao_half4x4(
		 unpackSnorm4x8(packedOcclusionAndBentNormal[0]),
		 unpackSnorm4x8(packedOcclusionAndBentNormal[1]),
		 unpackSnorm4x8(packedOcclusionAndBentNormal[2]),
		 unpackSnorm4x8(packedOcclusionAndBentNormal[3])
	);
}

//Output callback implementations
void cortao_writeFilterOutput(cortao_int2 pixelCoords, cortao_half4 ssao) {
	imageStore(outputBuffer, pixelCoords, ssao);
}

//Main function
layout(local_size_x = 8, local_size_y = 8) in; //Each thread processes a 2x2 pixel block, so effectively 16x16
void main() {
	uvec2 threadID = gl_GlobalInvocationID.xy;
	cortao_filterPass(threadID, parameters);
}