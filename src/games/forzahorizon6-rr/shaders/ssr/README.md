# SSR — screen-space reflections

Status: **no confirmed SSR shader in the collection yet.** This folder is the
target for the passes that will later need muting so SSR does not layer on top
of RT reflections.

Why nothing is here yet:

- The RT-frame dump contains no reflection-direction screen-space march.
  `../ssgi/0x8E8540A0` marches around the *normal* (GI-style), not the
  reflection vector.
- FH6's mirror-like reflections go through the hardware trace family in
  `../dxr-trace/` (`reflect(I, N)` + `TraceRayInline`), not a screen-space march.

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
