///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// sora-vanillaplus SSGI — Visibility Bitmask Indirect Diffuse
//
// Based on: "Screen Space Indirect Lighting with Visibility Bitmask"
// Therrien, Levesque, Gilet (2023)
//
// Reuses XeGTAO depth MIP chain (t0), MRT normals (t1), scene CBV (b0).
// Algorithm 1: visibility bitmask replaces GTAO horizon angles.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "xegtao_common.hlsl"

Texture2D<lpfloat>     g_srcWorkingDepth   : register(t0);  // XeGTAO depth MIPs
Texture2D<uint4>       g_srcMrtNormal      : register(t1);  // Game MRT normals
Texture2D<float4>      g_srcLightBuffer    : register(t2);  // Previous frame HDR
SamplerState           g_samplerPointClamp : register(s0);

RWTexture2D<float4>    g_outGI             : register(u0);

// ── SSGI push constants (b13) ──
// The first 23 fields come from xegtao_common.hlsl's cb_xegtao.
// We add SSGI-specific fields at the end (c[24]..c[31] are repurposed).

#ifndef XE_GTAO_SSGI_BITMASK_SIZE
#define XE_GTAO_SSGI_BITMASK_SIZE 32u
#endif

// ── Helpers ──
float3 SafeNormalize3(float3 v, float3 fallback)
{
  float len2 = dot(v, v);
  return (len2 < 1e-5) ? fallback : v * rsqrt(len2);
}

float3 DecodeMrtNormalAsIs(uint2 texel)
{
  uint4 sample = g_srcMrtNormal.Load(int3(texel, 0));
  float2 enc = float2((float)sample.x, (float)sample.y) * (1.0 / 32767.5) + float2(-1.0, -1.0);
  float azimuth = 3.14159274 * enc.x;
  float sin_a, cos_a;
  sincos(azimuth, sin_a, cos_a);
  float ring = sqrt(saturate(1.0 - enc.y * enc.y));
  float3 n = float3(cos_a * ring, sin_a * ring, enc.y);
  return SafeNormalize3(n, float3(0, 0, 1));
}

float3 TransformNormalToView(float3 decoded)
{
  float3x3 m = (float3x3)view_g;
  if (xegtao_normal_transform_mode > 1.5) return SafeNormalize3(decoded, float3(0, 0, 1));
  if (xegtao_normal_transform_mode > 0.5) m = (float3x3)viewInv_g;
  float3 vn = mul(m, decoded);
  return SafeNormalize3(vn, float3(0, 0, 1));
}

// ── Spatio-temporal noise (same as XeGTAO) ──
static lpfloat2 SpatioTemporalNoise(uint2 p, uint t)
{
  uint i = HilbertIndex(p.x % XE_HILBERT_WIDTH, p.y % XE_HILBERT_WIDTH) + 288u * (t % 64u);
  return lpfloat2(frac(0.5 + (float)i * 0.75487766624669276005),
                  frac(0.5 + (float)i * 0.5698402909980532659114));
}

// ── Popcount (count set bits in uint) ──
uint PopCount(uint v)
{
  v = v - ((v >> 1u) & 0x55555555u);
  v = (v & 0x33333333u) + ((v >> 2u) & 0x33333333u);
  v = (v + (v >> 4u)) & 0x0F0F0F0Fu;
  v = v + (v >> 8u);
  v = v + (v >> 16u);
  return v & 0x3Fu;
}

// ── Compute bitmask for a sample pair (front, back) ──
// theta_f, theta_b: angles relative to projected normal on the slice plane.
// Returns bitmask with set bits for occluded sectors.
uint ComputeSampleBitmask(float theta_f, float theta_b, uint N_b)
{
  // Convert angles from cosine-space [-π/2, π/2] to sector indices [0, N_b-1].
  float theta_min = min(theta_f, theta_b);
  float theta_max = max(theta_f, theta_b);

  // Map [-π/2, π/2] → [0, N_b]
  float inv_sector_width = (float)N_b / XE_GTAO_PI;
  int sector_a = (int)floor((theta_min + XE_GTAO_PI_HALF) * inv_sector_width);
  int sector_b = (int)ceil ((theta_max + XE_GTAO_PI_HALF) * inv_sector_width);
  sector_a = clamp(sector_a, 0, (int)N_b);
  sector_b = clamp(sector_b, 0, (int)N_b);

  if (sector_b <= sector_a) return 0u;

  uint count = (uint)(sector_b - sector_a);
  // Create bitmask: count bits set starting at sector_a
  uint mask = (count >= 32u) ? 0xFFFFFFFFu : ((1u << count) - 1u);
  return mask << (uint)sector_a;
}

// ── Compute angle relative to projected normal on the slice plane ──
// sliceDir: normalized direction vector along the slice on the image plane.
// sampleViewPos: view-space position of the depth sample.
// pixelViewPos: view-space position of the shaded pixel.
// viewspaceNormal: view-space normal at the shaded pixel.
float ComputeSliceAngle(float3 sampleViewPos, float3 pixelViewPos,
                        float3 viewspaceNormal, float3 sliceDir)
{
  // Vector from pixel to sample in view space
  float3 toSample = sampleViewPos - pixelViewPos;
  float dist = length(toSample);
  if (dist < 1e-6) return 0.0;

  // Project onto slice plane: cross(view_vec, sliceDir) gives the
  // "up" direction on the hemisphere slice, perpendicular to the slice.
  float3 viewVec = normalize(pixelViewPos);
  float3 sliceNormal = normalize(cross(viewVec, sliceDir)); // perpendicular to slice

  // Angle of toSample relative to the projected normal on the slice plane.
  // We use the signed angle between the projected normal and toSample.
  float3 projNormal = normalize(viewspaceNormal - sliceNormal * dot(viewspaceNormal, sliceNormal));
  float cosAngle = dot(normalize(toSample), projNormal);
  // Signed angle: positive = above projected normal, negative = below.
  float signVal = dot(cross(projNormal, normalize(toSample)), sliceNormal);
  float angle = acos(clamp(cosAngle, -1.0, 1.0));
  return signVal >= 0.0 ? angle : -angle;
}

// ── Main SSGI compute ──
[numthreads(XE_GTAO_NUMTHREADS_X, XE_GTAO_NUMTHREADS_Y, 1)]
void main(uint2 pixCoord : SV_DispatchThreadID)
{
  uint width, height;
  g_srcWorkingDepth.GetDimensions(width, height);
  if (pixCoord.x >= width || pixCoord.y >= height) return;

  GTAOConstants consts = BuildGTAOConstants(uint2(width, height));

  // ── Read SSGI parameters from push constants ──
  // c[24] = ssgi_radius, c[25] = ssgi_steps, c[26] = ssgi_directions,
  // c[27] = ssgi_thickness, c[28] = ssgi_intensity, c[29] = ssgi_step_distribution
  float ssgi_radius         = xegtao_copyback_preserve_yzw;       // c[24]
  float ssgi_steps          = xegtao_isfast_passes;               // c[25]
  float ssgi_directions     = xegtao_isfast_samples;              // c[26]
  float ssgi_thickness      = xegtao_isfast_radius;               // c[27]
  float ssgi_intensity      = xegtao_isfast_edge_sensitivity;     // c[28]
  float ssgi_step_dist_mode = xegtao_isfast_hybrid_blend;         // c[30]

  // Clamp to valid ranges
  uint N_b  = XE_GTAO_SSGI_BITMASK_SIZE;
  uint N_s  = (uint)clamp(ssgi_steps, 2.0, 32.0);
  uint N_d  = (uint)clamp(ssgi_directions, 1.0, 6.0);
  float radius      = max(0.01, ssgi_radius);
  float thickness   = max(0.001, ssgi_thickness);
  float intensity   = saturate(ssgi_intensity);
  bool  exp_steps   = ssgi_step_dist_mode > 0.5;

  // ── Pixel view-space position & normal ──
  float2 normalizedScreenPos = (float2(pixCoord) + 0.5) * consts.ViewportPixelSize;
  float4 depthsUL = g_srcWorkingDepth.GatherRed(g_samplerPointClamp,
      float2(pixCoord) * consts.ViewportPixelSize);
  float viewspaceZ = XeGTAO_ScreenSpaceToViewSpaceDepth(depthsUL.y, consts);
  float3 pixelViewPos = XeGTAO_ComputeViewspacePosition(normalizedScreenPos, viewspaceZ, consts);

  // ── Get view-space normal ──
  float3 viewspaceNormal;
  if (xegtao_normal_input_mode > 0.5 && xegtao_mrt_normal_available > 0.5)
  {
    uint mw, mh;
    g_srcMrtNormal.GetDimensions(mw, mh);
    float2 scale = float2(mw, mh) / max(float2(width, height), 1.0.xx);
    int2 mrt_tc = min(int2(floor((float2(pixCoord) + 0.5) * scale)), int2(mw - 1, mh - 1));
    float3 decoded = DecodeMrtNormalAsIs((uint2)mrt_tc);
    viewspaceNormal = TransformNormalToView(decoded);
  }
  else
  {
    // Depth-derived fallback
    float4 depthsBR = g_srcWorkingDepth.GatherRed(g_samplerPointClamp,
        float2(pixCoord) * consts.ViewportPixelSize, int2(1, 1));
    float lz = depthsUL.x, tz = depthsUL.z, rz = depthsBR.z, bz = depthsBR.x;
    float4 edges = XeGTAO_CalculateEdges((lpfloat)viewspaceZ, (lpfloat)lz,
        (lpfloat)rz, (lpfloat)tz, (lpfloat)bz);
    float3 C = pixelViewPos;
    float3 L = XeGTAO_ComputeViewspacePosition(
        normalizedScreenPos + float2(-1, 0) * consts.ViewportPixelSize, lz, consts);
    float3 R = XeGTAO_ComputeViewspacePosition(
        normalizedScreenPos + float2( 1, 0) * consts.ViewportPixelSize, rz, consts);
    float3 T = XeGTAO_ComputeViewspacePosition(
        normalizedScreenPos + float2( 0,-1) * consts.ViewportPixelSize, tz, consts);
    float3 B = XeGTAO_ComputeViewspacePosition(
        normalizedScreenPos + float2( 0, 1) * consts.ViewportPixelSize, bz, consts);
    viewspaceNormal = XeGTAO_CalculateNormal(edges, C, L, R, T, B);
  }

  // ── Skip sky pixels ──
  float viewDist = length(pixelViewPos);
  if (viewDist > 500.0) {
    g_outGI[pixCoord] = float4(0, 0, 0, 0);
    return;
  }

  // ── Noise for direction jitter ──
  uint noise_idx = consts.NoiseIndex < 0 ? 0u : (uint)consts.NoiseIndex;
  lpfloat2 noise = SpatioTemporalNoise(pixCoord, noise_idx);

  // ── Projected radius on image plane ──
  float projectedRadius = radius / max(abs(viewspaceZ), 0.001);

  // ── Accumulate GI over directions ──
  float3 giAccum = float3(0, 0, 0);
  float aoAccum = 0.0;
  float viewVecLen = length(pixelViewPos);
  float3 viewDir = viewVecLen > 1e-6 ? pixelViewPos / viewVecLen : float3(0, 0, 1);

  for (uint d = 0; d < N_d; ++d)
  {
    // Direction angle on the image plane
    float dirAngle = (float)d / (float)N_d * XE_GTAO_PI + noise.x * (XE_GTAO_PI / (float)N_d);

    float sinDir, cosDir;
    sincos(dirAngle, sinDir, cosDir);
    float3 sliceDir = float3(cosDir, sinDir, 0); // on XY image plane

    // Bitmask for this direction (0 = unoccluded, 1 = occluded)
    uint bitmask = 0u;

    // ── Step along the slice direction ──
    for (uint s = 1; s <= N_s; ++s)
    {
      // Step distance: constant or exponential
      float stepFrac;
      if (exp_steps)
        stepFrac = pow((float)s / (float)N_s, 2.0);  // x² distribution
      else
        stepFrac = (float)s / (float)N_s;

      float stepDist = projectedRadius * stepFrac;

      // Screen-space step offset
      float2 stepUV = normalizedScreenPos + float2(cosDir, sinDir) * stepDist;
      float2 stepTexel = stepUV / consts.ViewportPixelSize - 0.5;

      // Sample depth at step location
      float stepDepth = g_srcWorkingDepth.SampleLevel(g_samplerPointClamp, stepUV, 0);
      float stepViewZ = XeGTAO_ScreenSpaceToViewSpaceDepth(stepDepth, consts);
      float3 sampleViewPos = XeGTAO_ComputeViewspacePosition(stepUV, stepViewZ, consts);

      // Back sample (behind front sample by thickness along view direction)
      float3 sampleViewDir = length(sampleViewPos) > 1e-6
          ? normalize(sampleViewPos) : viewDir;
      float3 backViewPos = sampleViewPos - sampleViewDir * thickness;

      // Angles relative to projected normal on slice plane
      float theta_f = ComputeSliceAngle(sampleViewPos, pixelViewPos, viewspaceNormal, sliceDir);
      float theta_b = ComputeSliceAngle(backViewPos, pixelViewPos, viewspaceNormal, sliceDir);

      // Bitmask for this sample
      uint sampleMask = ComputeSampleBitmask(theta_f, theta_b, N_b);

      // Newly occluded sectors (sample sectors NOT already occluded)
      uint newOcclusion = sampleMask & ~bitmask;
      uint unoccludedCount = N_b - PopCount(bitmask);
      uint newCount = PopCount(newOcclusion);

      // ── GI contribution from this sample ──
      if (newCount > 0u && unoccludedCount > 0u)
      {
        // Sample light buffer at step location
        uint lw, lh;
        g_srcLightBuffer.GetDimensions(lw, lh);
        int2 lightTexel = int2(stepUV * float2(lw, lh));
        lightTexel = clamp(lightTexel, int2(0, 0), int2(lw - 1, lh - 1));
        float3 lightColor = g_srcLightBuffer.Load(int3(lightTexel, 0)).rgb;

        // Light direction from pixel to sample
        float3 lightDir = sampleViewPos - pixelViewPos;
        float lightDist = length(lightDir);
        if (lightDist > 1e-6)
        {
          lightDir /= lightDist;

          // Sample normal at step location
          float3 sampleNormal;
          if (xegtao_normal_input_mode > 0.5 && xegtao_mrt_normal_available > 0.5)
          {
            uint mw2, mh2;
            g_srcMrtNormal.GetDimensions(mw2, mh2);
            float2 s2 = float2(mw2, mh2) / max(float2(width, height), 1.0.xx);
            int2 mrt2 = min(int2(floor((float2((uint2)stepTexel) + 0.5) * s2)), int2(mw2 - 1, mh2 - 1));
            sampleNormal = TransformNormalToView(DecodeMrtNormalAsIs((uint2)mrt2));
          }
          else
          {
            sampleNormal = viewspaceNormal; // fallback
          }

          // Geometric weighting
          float NdotL = saturate(dot(viewspaceNormal, lightDir));
          float NsDotL = saturate(dot(sampleNormal, -lightDir));

          // GI contribution: weighted by new unoccluded fraction
          float weight = (float)newCount / (float)unoccludedCount;
          giAccum += weight * lightColor * NdotL * NsDotL * intensity;
        }
      }

      // Accumulate occlusion
      bitmask |= sampleMask;
    }

    // AO for this direction
    aoAccum += 1.0 - (float)PopCount(bitmask) / (float)N_b;
  }

  float3 gi = giAccum / max((float)N_d, 1.0);
  float ao = aoAccum / max((float)N_d, 1.0);

  g_outGI[pixCoord] = float4(gi, ao);
}
