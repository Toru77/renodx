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

//Include the shader
#include <CORTAOPrepass.h>

//Parameters as root constants
ConstantBuffer<CORTAOPrepassParameters> parameters : register(b0);

//Texture resources
Texture2D<cortao_float> depthBuffer : register(t0); //User defined format
Texture2D<cortao_float3> normalBuffer : register(t1); //User defined format

//Image resources
RWTexture2D<cortao_uint> packedDataBufferMipmaps[8] : register(u0); //R32_UINT mipmaps

//Input callback implementations
cortao_float cortao_loadDepth(cortao_int2 pixelCoords) {
	return depthBuffer.Load(int3(pixelCoords, 0));
}

cortao_float3 cortao_loadNormal(cortao_int2 pixelCoords) {
	cortao_float3 normal = normalBuffer.Load(int3(pixelCoords, 0)).xyz;
	//Do any normal transformation/unpacking/decoding here.
	return normal;
}

//Output callback implementation
void cortao_writePackedData(cortao_int2 pixelCoords, cortao_int mipLevel, cortao_uint data) {
	packedDataBufferMipmaps[mipLevel][pixelCoords] = data;
}

[numthreads(8, 8, 1)]
void main(uint2 pixelCoords : SV_DispatchThreadID) {
	cortao_prepass(pixelCoords, parameters);
}
