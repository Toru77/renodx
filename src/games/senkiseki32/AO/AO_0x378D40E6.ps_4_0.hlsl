// ---- Created with 3Dmigoto v1.4.1 on Mon Feb 16 23:43:56 2026
#include "../shared.h"
#define cmp -

cbuffer _Globals : register(b0)
{
  float3 scene_EyePosition : packoffset(c0);
  float4x4 scene_View : packoffset(c1);
  float4x4 scene_ViewProjection : packoffset(c5);
  float3 scene_GlobalAmbientColor : packoffset(c9);
  float scene_GlobalTexcoordFactor : packoffset(c9.w);
  float4 scene_FogRangeParameters : packoffset(c10);
  float3 scene_FogColor : packoffset(c11);
  float3 scene_FakeRimLightDir : packoffset(c12);
  float4 scene_MiscParameters2 : packoffset(c13);
  float scene_AdditionalShadowOffset : packoffset(c14);
  float4 scene_cameraNearFarParameters : packoffset(c15);
  float4 FilterColor : packoffset(c16) = {1,1,1,1};
  float4 FadingColor : packoffset(c17) = {1,1,1,1};
  float4 MonotoneMul : packoffset(c18) = {1,1,1,1};
  float4 MonotoneAdd : packoffset(c19) = {0,0,0,0};
  float4 GlowIntensity : packoffset(c20) = {1,1,1,1};
  float4 ToneFactor : packoffset(c21) = {1,1,1,1};
  float4 UvScaleBias : packoffset(c22) = {1,1,0,0};
  float4 GaussianBlurParams : packoffset(c23) = {0,0,0,0};
  float4 DofParams : packoffset(c24) = {0,0,0,0};
  float4 GammaParameters : packoffset(c25) = {1,1,1,0};
  float4 WhirlPinchParams : packoffset(c26) = {0,0,0,0};
  float4 UVWarpParams : packoffset(c27) = {0,0,0,0};
  float4 MotionBlurParams : packoffset(c28) = {0,0,0,0};
  float GlobalTexcoordFactor : packoffset(c29);
}


SamplerState LinearClampSampler_s : register(s0);
Texture2D<float4> AOBuffer : register(t0);
Texture2D<float4> AOColorBuffer : register(t1);
Texture2DMS<float4,4> DepthBuffer : register(t2);


// [NEW] Helper Function for Fake GI with Dithering
float3 CalculateGI(Texture2D<float4> colorTex, Texture2DMS<float4,4> depthTex, float2 centerUV, float centerDepth, float2 screenSize)
{
    float3 accumulatedGI = float3(0, 0, 0);
    float totalWeight = 0.0;

    // CONFIG: Reduced radius to prevent "Double Vision"
    float offsetSize = 4.0; 
    
    // Create a random rotation based on pixel position (Dithering)
    // This breaks up the "ghosting" into fine noise
    float noise = frac(sin(dot(centerUV, float2(12.9898, 78.233))) * 43758.5453);
    float angle = noise * 6.283185; // 2 * PI
    float s, c;
    sincos(angle, s, c);

    // Standard cross pattern, but we will rotate it
    float2 baseOffsets[4] = {
        float2(offsetSize, 0),
        float2(-offsetSize, 0),
        float2(0, offsetSize),
        float2(0, -offsetSize)
    };

    [unroll]
    for(int i = 0; i < 4; i++)
    {
        // Apply Rotation to the offset
        float2 rotatedOffset;
        rotatedOffset.x = baseOffsets[i].x * c - baseOffsets[i].y * s;
        rotatedOffset.y = baseOffsets[i].x * s + baseOffsets[i].y * c;

        // Correct for aspect ratio (assuming 16:9 roughly, or just square pixels)
        // We divide by screenSize to get UV space
        float2 neighborUV = centerUV + (rotatedOffset / screenSize);
        
        // Clamp UV to prevent wrapping artifacts at screen edges
        neighborUV = clamp(neighborUV, 0.0, 1.0);

        int2 neighborCoords = int2(neighborUV * screenSize);

        // Sample Neighbor Depth
        float neighborDepth = depthTex.Load(neighborCoords, 0).x;

        // Depth Check (Stricter check to keep details sharp)
        float depthDiff = abs(centerDepth - neighborDepth);
        
        // If the neighbor is too far away (depth gap), ignore it
        if(depthDiff < 0.002) 
        {
            float3 neighborColor = colorTex.SampleLevel(LinearClampSampler_s, neighborUV, 0).rgb;
            accumulatedGI += neighborColor;
            totalWeight += 1.0;
        }
    }

    // Return average, or Black if no valid neighbors
    return (totalWeight > 0) ? (accumulatedGI / totalWeight) : float3(0,0,0);
}
// [Main Function]
void main(
  float4 v0 : SV_POSITION0,
  float2 v1 : TEXCOORD0,
  out float4 o0 : SV_TARGET0)
{
  float4 r0,r1,r2;
  uint4 bitmask, uiDest;
  float4 fDest;

  // --- EXISTING LOGIC START (Preserved for compatibility) ---
  DepthBuffer.GetDimensions(uiDest.x, uiDest.y, uiDest.z);
  r0.xyzw = uiDest.xyzw;
  r0.xy = (uint2)r0.xy;
  float2 screenRes = r0.xy; // Save resolution for GI later
  
  r0.xy = v1.xy * r0.xy;
  r0.xy = (uint2)r0.xy;
  r0.zw = float2(0,0);
  
  // Save Raw Depth for GI Check
  float rawDepth = DepthBuffer.Load(r0.xy, 0).x;
  
  r0.xyzw = DepthBuffer.Load(r0.xy, 0).xyzw;
  r0.x = r0.x * 2 + -1;
  r0.y = scene_cameraNearFarParameters.y + scene_cameraNearFarParameters.x;
  r0.z = scene_cameraNearFarParameters.y + -scene_cameraNearFarParameters.x;
  r0.x = -r0.x * r0.z + r0.y;
  r0.y = dot(scene_cameraNearFarParameters.yy, scene_cameraNearFarParameters.xx);
  r0.x = r0.y / r0.x;
  r0.y = min(400, scene_FogRangeParameters.y);
  r0.z = -r0.y * 0.5 + r0.x;
  r0.y = 0.5 * r0.y;
  r0.z = saturate(r0.z / r0.y);
  r0.y = cmp(r0.y < r0.x);
  r1.xyzw = AOBuffer.SampleLevel(LinearClampSampler_s, v1.xy, 0).xyzw;
  r0.w = 1 + -r1.x;
  r0.z = r0.z * r0.w + r1.x;
  r0.y = r0.y ? r0.z : r1.x;
  r0.z = -1 + r1.x;
  r0.w = cmp(r0.x < 2);
  r1.x = saturate(-1 + r0.x);
  r0.x = -scene_FogRangeParameters.x + r0.x;
  r0.z = r1.x * r0.z + 1;
  r0.y = r0.w ? r0.z : r0.y;
  r0.y = 1 + -r0.y;
  r1.xyzw = AOColorBuffer.SampleLevel(LinearClampSampler_s, v1.xy, 0).xyzw;
  
  // Existing "Beige Color Mask" Logic
  r2.xyz = float3(-0.929411769,-0.854901969,-0.709803939) + r1.xyz;
  r0.z = dot(r2.xyz, r2.xyz);
  r0.z = sqrt(r0.z);
  r0.z = r0.z + r0.z;
  r0.z = min(1, r0.z);
  r0.y = -r0.y * r0.z + 1;
  r0.y = 1 + -r0.y;
  r0.z = scene_FogRangeParameters.y + -scene_FogRangeParameters.x;
  r0.z = 1 / r0.z;
  r0.x = saturate(r0.x * r0.z);
  r0.z = r0.x * -2 + 3;
  r0.x = r0.x * r0.x;
  r0.x = -r0.z * r0.x + 1;
  r0.x = -r0.y * r0.x + 1;
  // --- EXISTING LOGIC END ---

  // --- NEW GI COMPOSITION START ---
  
  // 1. Extract clean variables from the registers
  float3 sceneColor = r1.xyz;
  float aoFactor = r0.x; // 0.0 = Fully Occluded, 1.0 = No Occlusion
  
  // 2. Read toggle and slider values
  bool giEnabled = (shader_injection.gi_enabled != 0.0);
  bool aoEnabled = (shader_injection.ao_enabled != 0.0);
  float giIntensity = shader_injection.gi_intensity;
  float shadowSaturation = shader_injection.shadow_saturation;
  float aoPower = shader_injection.ao_power;
  
  // 3. Apply AO Power
  if (aoEnabled) {
    // When GI is on, AO runs at 2x power internally for deeper shadows
    float effectivePower = giEnabled ? (aoPower * 2.0) : aoPower;
    aoFactor = pow(aoFactor, effectivePower);
  } else {
    aoFactor = 1.0; // AO off = no occlusion
  }
  
  // 4. GI path (only when GI toggle is on)
  if (giEnabled) {
    // Calculate "Fake GI" (Bounce Light)
    float3 bounceLight = CalculateGI(AOColorBuffer, DepthBuffer, v1.xy, rawDepth, screenRes);
    
    // Shadow Saturation (boost color in occluded areas to mimic subsurface scattering)
    float occlusionAmount = 1.0 - aoFactor;
    float luma = dot(sceneColor, float3(0.2126, 0.7152, 0.0722));
    float3 saturatedScene = luma + (sceneColor - luma) * lerp(1.0, shadowSaturation, occlusionAmount);
    
    // Combine direct + indirect
    float3 directLighting = saturatedScene * aoFactor;
    // Indirect bounce — unclamped for HDR
    float3 indirectLighting = bounceLight * occlusionAmount * giIntensity;
    
    o0.xyz = directLighting + indirectLighting;
  } else {
    // GI off — standard AO multiply only
    o0.xyz = sceneColor * aoFactor;
  }
  
  o0.w = r1.w; // Preserve original Alpha
  
  return;
}
