# SSR — screen-space reflections

Status: **SSR is confirmed in the live DevKit capture.** The current build has
a checkerboard cleanup pass and several spatial reflection filters. Source
replacements for the in-place checkerboard cleanup and separable roughness-mip
blur pair are present; their overlay controls are currently hidden. No separate
temporal SSR denoiser or inline-DXR reflection denoiser has been confirmed yet.

The offline dump corpus still has no confirmed screen-space reflection march:

- The RT-frame dump contains no reflection-direction screen-space march.
  `../ssgi/0x8E8540A0` marches around the *normal* (GI-style), not the
  reflection vector.
- FH6's mirror-like reflections go through the hardware trace family in
  `../dxr-trace/` (`reflect(I, N)` + `TraceRayInline`), not a screen-space march.

## Live DevKit capture

The current snapshot shows the following SSR dispatch sequence. Draw numbers are
snapshot-local; use the PDB name and pass order to rediscover them after updates.

| Draw | Hash | PDB pass | Finding |
|---|---|---|---|
| 5425–5426 | `0x2F437CE7` | `SSR_FullScreen_RayTrace_InverseDepth_Rough_ContactHardened_MipFiltered_MixWithPlanar` | SSR trace stage; not a denoiser. |
| 5440–5441 | `0x313E9A06` | `SSR_FullScreen_TidyCheckerboard_Rough_MipFiltered` | Strongest denoiser candidate: processes a compacted checkerboard-pixel list and clamps each pixel to the min/max of its four axial neighbors. No history or motion input. |
| 5442 | `0x5D06E329` | `SSR_FullScreen_FilterRoughChosenMip_Horizontal_MixWithPlanar` | Horizontal filter/reconstruction pass; retained because it participates in planar mixing. |
| 5443 | `0x9D5AD92B` | `SSR_FullScreen_FilterRoughChosenMip_Vertical_MixWithPlanar` | Vertical filter/reconstruction pass; retained because it upsamples and uses bindless inputs for planar mixing. |
| 5444 | `0xDA5363C5` | `SSR_FullScreen_MipGeneration_Reflection` | Builds the reflection mip chain. |
| 5445–5460 | `0xF0DE4E22` / `0x24848E0B` | `SSR_FullScreen_FilterMip_Horizontal` / `..._Vertical` | Eight separable mip-filter pairs. The shader uses a 9-tap Gaussian-like spatial kernel; this is roughness/mip prefiltering, not temporal denoising. |

The `TidyCheckerboard` pass is the best current SSR denoiser candidate. Its
decompiled code reads the current reflection result and a compacted pixel list,
then clamps the selected pixels against neighboring values in place. The
The checkerboard replacement skips that in-place operation. The roughness-mip
replacements replace the 9-tap `FilterMip_Horizontal` /
`FilterMip_Vertical` passes with direct copies. These filters run eight times
each to build roughness mips; they are spatial only and read no previous-frame
result or motion vectors.

The separate `FilterRoughChosenMip_*_MixWithPlanar` pair is intentionally not
disabled by that toggle. The horizontal pass combines filtering with planar
reflection handling, and the vertical pass also writes an upscaled result using
bindless inputs. Replacing either with a generic copy would change reflection
composition, so they remain the game's original shaders.

The separate `RTGI_TemporalFilter`/`RTGI_SpatialFilter_Disk` passes are named
RTGI in their PDBs and remain classified as GI denoisers, not reflection
denoisers. I scanned the 112 unique compute shaders in this snapshot for DXIL
RayQuery/TraceRay operations and found none, so no active hardware-DXR reflection
denoiser is identified in this capture. The older inline-ray-query hashes in
`../README.md` are from an earlier dump and must be rediscovered before reuse.

There is also a separate `GroundReflection` chain at draws 2775–2804:
`0x23253C24` downsample, `0x30E61133` vertical blur, and `0xD7BC2E89`
horizontal blur, repeated over ten levels. These are spatial downsample/blur
passes; the current capture does not prove that they denoise ray-traced
reflections.

How to find it:

1. **Session diff** — dump with RT OFF in a reflective spot (wet road, water,
   glass), same location/angle as an RT ON dump, and diff the shader-hash sets.
   The devkit dump folders live under
   `F:\SteamLibrary\steamapps\common\ForzaHorizon6\renodx-dev\`.
2. **Grep candidates** — look for shaders containing reflection-vector math
   (`V - 2·dot(V,N)·N`) plus a depth-compare march loop, using the
   `dxc -dumpbin` + `spirv-cross` recipe in `../README.md`.
3. **RenderDoc** — capture an RT ON frame near water and look for a pass that
   steps through the depth buffer along screen-space offsets, and for a later
   blend pass that combines screen-space and ray-traced reflection results.

Watch for:

- a combine/blend pass that mixes "SSR result + RT result" — that is the
  layering to mute;
- a fallback SSR possibly invoked when a trace misses (`u0.a` visibility
  weight == 0 in `../dxr-trace` output) — hypothesis, not verified.
