// Copyright 2023 Sony Interactive Entertainment.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Modified for falcomengine-plus: HLSL port of BuildDispatchList() from Bend's bend_sss_cpu.h (Apache-2.0), run on
// the GPU because viewProj_g, lightDirection_g and the lamp buffer have no CPU copy outside DevKit builds. Adds the
// light world value to the block. inExpandedZRange = false (D3D). Include after shadows_common.hlsli.

void BendWriteDispatchList(RWBuffer<uint> outDispatch, uint blockBase, bool valid, float4 lightProjection,
                           float4 lightWorld, float2 viewport, int2 minBounds, int2 maxBounds)
{
  const int waveSize = BEND_WAVE_SIZE;
  int3 waveCount[BEND_MAX_DISPATCHES];
  int2 waveOffset[BEND_MAX_DISPATCHES];
  [unroll] for (int k = 0; k < BEND_MAX_DISPATCHES; ++k) {
    waveCount[k] = int3(0, 0, 0);
    waveOffset[k] = int2(0, 0);
  }
  int dispatchCount = 0;
  float4 lightCoordinate = float4(0.0, 0.0, 0.0, 0.0);
  if (valid) {
    // Bend: clamp w away from 0 so the XY divide keeps precision for far off-screen lights.
    const float fpLimit = 0.000002 * (float)waveSize;
    float xyLightW = lightProjection.w;
    if (xyLightW >= 0.0 && xyLightW < fpLimit) xyLightW = fpLimit;
    else if (xyLightW < 0.0 && xyLightW > -fpLimit) xyLightW = -fpLimit;
    lightCoordinate.x = ((lightProjection.x / xyLightW) * 0.5 + 0.5) * viewport.x;
    lightCoordinate.y = ((lightProjection.y / xyLightW) * -0.5 + 0.5) * viewport.y;
    lightCoordinate.z = lightProjection.w == 0.0 ? 0.0 : (lightProjection.z / lightProjection.w);
    lightCoordinate.w = lightProjection.w > 0.0 ? 1.0 : -1.0;
    const int2 lightXY = int2((int)(lightCoordinate.x + 0.5), (int)(lightCoordinate.y + 0.5));
    const int4 biased = int4(minBounds.x - lightXY.x, -(maxBounds.y - lightXY.y),
                             maxBounds.x - lightXY.x, -(minBounds.y - lightXY.y));
    // Bend: 4 quadrants around the light, each a rectangle with one corner on the light; a quadrant is split in two
    // where the diagonal light ray crosses its edge.
    [unroll] for (int q = 0; q < 4; ++q) {
      const bool vertical = q == 0 || q == 3;
      const int4 bounds = int4(
          max(0, ((q & 1) ? biased.x : -biased.z)) / waveSize,
          max(0, ((q & 2) ? biased.y : -biased.w)) / waveSize,
          max(0, (((q & 1) ? biased.z : -biased.x) + waveSize * (vertical ? 1 : 2) - 1)) / waveSize,
          max(0, (((q & 2) ? biased.w : -biased.y) + waveSize * (vertical ? 2 : 1) - 1)) / waveSize);
      if ((bounds.z - bounds.x) > 0 && (bounds.w - bounds.y) > 0) {
        const int biasX = (q == 2 || q == 3) ? 1 : 0;
        const int biasY = (q == 1 || q == 3) ? 1 : 0;
        const int d0 = dispatchCount++;
        waveCount[d0] = int3(waveSize, bounds.z - bounds.x, bounds.w - bounds.y);
        waveOffset[d0] = int2(((q & 1) ? bounds.x : -bounds.z) + biasX, ((q & 2) ? -bounds.w : bounds.y) + biasY);
        int axisDelta = +biased.x - biased.y;
        if (q == 1) axisDelta = +biased.z + biased.y;
        if (q == 2) axisDelta = -biased.x - biased.w;
        if (q == 3) axisDelta = -biased.z + biased.w;
        axisDelta = (axisDelta + waveSize - 1) / waveSize;
        if (axisDelta > 0) {
          const int d1 = dispatchCount++;
          waveCount[d1] = waveCount[d0];
          waveOffset[d1] = waveOffset[d0];
          if (q == 0) {
            waveCount[d1].z = min(waveCount[d0].z, axisDelta);
            waveCount[d0].z -= waveCount[d1].z;
            waveOffset[d1].y = waveOffset[d0].y + waveCount[d0].z;
            waveOffset[d1].x--;
            waveCount[d1].y++;
          }
          if (q == 1) {
            waveCount[d1].y = min(waveCount[d0].y, axisDelta);
            waveCount[d0].y -= waveCount[d1].y;
            waveOffset[d1].x = waveOffset[d0].x + waveCount[d0].y;
            waveCount[d1].z++;
          }
          if (q == 2) {
            waveCount[d1].y = min(waveCount[d0].y, axisDelta);
            waveCount[d0].y -= waveCount[d1].y;
            waveOffset[d0].x += waveCount[d1].y;
            waveCount[d1].z++;
            waveOffset[d1].y--;
          }
          if (q == 3) {
            waveCount[d1].z = min(waveCount[d0].z, axisDelta);
            waveCount[d0].z -= waveCount[d1].z;
            waveOffset[d0].y += waveCount[d1].z;
            waveCount[d1].y++;
          }
          // Bend: remove if too small; the last entry moves into the removed slot.
          if (waveCount[d1].y <= 0 || waveCount[d1].z <= 0) {
            dispatchCount--;
            waveCount[d1] = waveCount[dispatchCount];
            waveOffset[d1] = waveOffset[dispatchCount];
          }
          if (waveCount[d0].y <= 0 || waveCount[d0].z <= 0) {
            dispatchCount--;
            waveCount[d0] = waveCount[dispatchCount];
            waveOffset[d0] = waveOffset[dispatchCount];
          }
        }
      }
    }
  }
  [unroll] for (int s = 0; s < BEND_MAX_DISPATCHES; ++s) {
    const bool live = s < dispatchCount;
    outDispatch[blockBase + BEND_ARGS_OFFSET + s * 3 + 0] = live ? (uint)waveCount[s].x : 0u;
    outDispatch[blockBase + BEND_ARGS_OFFSET + s * 3 + 1] = live ? (uint)waveCount[s].y : 0u;
    outDispatch[blockBase + BEND_ARGS_OFFSET + s * 3 + 2] = live ? (uint)waveCount[s].z : 0u;
    // Bend scales the shader offsets by the wave size.
    outDispatch[blockBase + BEND_WAVE_OFFSET + s * 2 + 0] = asuint(waveOffset[s].x * waveSize);
    outDispatch[blockBase + BEND_WAVE_OFFSET + s * 2 + 1] = asuint(waveOffset[s].y * waveSize);
  }
  outDispatch[blockBase + BEND_LIGHT_OFFSET + 0] = asuint(lightCoordinate.x);
  outDispatch[blockBase + BEND_LIGHT_OFFSET + 1] = asuint(lightCoordinate.y);
  outDispatch[blockBase + BEND_LIGHT_OFFSET + 2] = asuint(lightCoordinate.z);
  outDispatch[blockBase + BEND_LIGHT_OFFSET + 3] = asuint(lightCoordinate.w);
  outDispatch[blockBase + BEND_LIGHT_WORLD_OFFSET + 0] = valid ? asuint(lightWorld.x) : 0u;
  outDispatch[blockBase + BEND_LIGHT_WORLD_OFFSET + 1] = valid ? asuint(lightWorld.y) : 0u;
  outDispatch[blockBase + BEND_LIGHT_WORLD_OFFSET + 2] = valid ? asuint(lightWorld.z) : 0u;
  outDispatch[blockBase + BEND_LIGHT_WORLD_OFFSET + 3] = valid ? asuint(lightWorld.w) : 0u;
}
