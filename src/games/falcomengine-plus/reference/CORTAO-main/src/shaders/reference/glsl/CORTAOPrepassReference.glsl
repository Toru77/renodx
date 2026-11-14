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

//CORTAO defines
#define CORTAO_GLSL 1
#define CORTAO_PERSPECTIVE_PROJECTION 1

//Include the shader
#include <CORTAOPrepass.h>

//Parameters in push constants
layout(push_constant) uniform CORTAOPrepassPushUniforms  {
	CORTAOPrepassParameters parameters;
};

//Texture resources
layout(set = 0, binding = 0) uniform texture2D depthBuffer; //User defined format
layout(set = 0, binding = 1) uniform texture2D normalBuffer; //User defined format

//Image resources
layout(set = 0, binding = 2) uniform restrict writeonly uimage2D[8] packedDataBufferMipmaps; //R32_UINT mipmaps

//Input callback implementations
cortao_float cortao_loadDepth(cortao_int2 pixelCoords) {
	return texelFetch(depthBuffer, pixelCoords, 0).r;
}

cortao_float3 cortao_loadNormal(cortao_int2 pixelCoords) {
	cortao_float3 normal = texelFetch(normalBuffer, pixelCoords, 0).xyz;
	//Do any normal transformation/unpacking/decoding here.
	return normal;
}

//Output callback implementation
void cortao_writePackedData(cortao_int2 pixelCoords, cortao_int mipLevel, cortao_uint data) {
	imageStore(packedDataBufferMipmaps[mipLevel], pixelCoords, cortao_uint4(data, 0, 0, 0));
}

//Main function
layout(local_size_x = 8, local_size_y = 8) in;
void main() {
	uvec2 pixelCoords = gl_GlobalInvocationID.xy;
	cortao_prepass(pixelCoords, parameters);
}
