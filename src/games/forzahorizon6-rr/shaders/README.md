# FH6 shader dumps — inventory

Decompiled from the game (DXIL). Identification below is by behavior; shader
names are mostly stripped, but some semantic/interpolant names survive in the
decompiled HLSL (e.g. `MOTIONBLUR_POS`, `WORLDPOS`, `DAMAGEIMPACT`).

## Sources

- `F:\SteamLibrary\steamapps\common\ForzaHorizon6\renodx-dev\dump\decompiled_all\`
  — full decompiled corpus of the compiled-shader dump (3,477 shaders):
  2,912 full HLSL + 565 LLVM-only in `_llvm\` (562 `SV_ShadingRate`/VRS pixel
  shaders and 3 dynamic-cbuffer-indexed compute shaders that spirv-cross and
  the repo `decomp` cannot express as HLSL). This is the scan corpus — only
  curated files are kept in this repo folder.
- `F:\SteamLibrary\steamapps\common\ForzaHorizon6\renodx-dev\big_dump\*.cso`
  — earlier full dump (~4,900 incl. VS/PS/CS).
- Decompiler bundle: `F:\SteamLibrary\steamapps\common\ForzaHorizon6\renodx-dev\dump\decompiler\`
  (`dxc.exe`, `dxil-spirv.exe`, `spirv-cross.exe`). The repo `decomp` target
  (`build\Release\decomp.exe`) is a second opinion; both choke on VRS and
  dynamic cbuffer indexing — use LLVM disassembly for those.

## Regeneration recipe

Per `{HASH}.{stage}.cso`:

```
dxc.exe -dumpbin {HASH}.{stage}.cso > {HASH}.{stage}.llvm
dxil-spirv.exe {HASH}.{stage}.cso --use-reflection-names --output {HASH}.spv
spirv-cross.exe {HASH}.spv --hlsl --relax-nan-checks --shader-model 66 --hlsl-preserve-structured-buffers --output {HASH}.{stage}.hlsl
```

Caveats: spirv-cross output is not the original source (normalized control
flow, `_123` style locals) but preserves registers and spaces. The dumps were
compiled with `dxcoob 1.8.2505.28`, which keeps the resource binding table in
the DXIL — `.llvm` disassemblies carry registers + spaces for verification.

## dxr-trace/ — inline RayQuery reflection traces (16)

Common shape: `[numthreads(8,8,1)]`; work-list dispatch (`idx = wg*64 + lane`,
`if (idx < listCount)`, list count read from `u1`); one `TraceRayInline`
(flags 537) + one `Proceed()` + `CommittedRayT()`. Ray = `reflect(I, N)` in
world space, origin offset ~1 mm, length capped. On hit the world hit position
is reconstructed and **reprojected to screen UV** with a border fade:
`u0 = float4(hitUV, _, weight)` (values clamped to 0.996 — unorm-friendly,
so `u0` is likely re-read as a texture later), `u2 = float4(v, 0, 0, v)`
(a normalized scalar; candidate hit-distance/LOD metric — verify live).

Permutation clusters (by binding profile; which feature maps to which variant
is still TODO):
- A: `0x12886018, 0x2A602F59, 0x44D4E258, 0x489E04F8, 0x50067031, 0xD4B4DB6E`
- B: `0x1A7101EC, 0x2740BA98, 0x43453554, 0x5408243D, 0x7BD3C79E, 0xD844655D`
- C: `0x490914DC, 0x5CF02E66` (no env-cube sampling, fewer material loads)
- D: `0xBA7D5A3F, 0xBF9D9135` (leanest)

These are the prime RR guide sources: hit distance
(`DLSSD.SpecularHitDistance`) and — with the stored ray direction and the
camera matrices found in their cbuffers — specular motion vectors.

## gbuffer-writers/ — material/geometry passes writing the G-buffer

The dump contains **615 PS shaders** in this family (scan: match `2047.5f`).
It is the geometry/material pass that writes FH6's **8-target G-buffer**.

Verified layout (from `0x007D9BEB`, a car-paint variant):

| Target | Content |
|---|---|
| T0 | **Base color / diffuse albedo** — texture albedo × paint flakes × fade/damage |
| T1 | **Velocity** — sqrt-compressed: `(sign·32·√(\|mv\|)+512)/1023`, `.z` = depth delta ×32 (motion-blur style) |
| T2 | Packed material flags/ids (`_1487>>7`, `clamp(x)*127 \| 128`, `_1487&127`) |
| T3 | **Packed normal + gloss** (see below) |
| T4 | Secondary color (lerp of two sampled colors; clearcoat/specular-ish) |
| T5 | Damage/extra params |
| T6/T7 | Packed ids / half-packed bit-fields |

The normal+gloss pack (T3, R32_UINT):

```hlsl
SV_Target_3 = ((uint(min(max(round((y * 2047.5f) + 2047.5f), 0.0f), 4095.0f)) << 20u)
             | (uint(min(max(round((x * 2047.5f) + 2047.5f), 0.0f), 4095.0f)) << 8u))
             | (uint(clamp(gloss, 0.0f, 1.0f) * 255.0f) & 255u);
```

- x/y: octahedral-encoded normal in 12-bit fields (bits 8–19 / 20–31).
- Low byte = **GLOSS** (verified: `1 - value` is used as roughness for the
  environment-map LOD in the same shader). For RR guides: roughness =
  `1 - byte/255` (confirm live).
- Variants: some write `| 31u` (constant gloss), some omit the low byte
  entirely (`0x0048CF38`).

Curated files:
- `0x007D9BEB` — car paint, fully decoded (flakes, damage, env LOD)
- `0x02357101` — typical full-width variant
- `0x0D39E147` — simpler variant with gloss byte
- `0x06AA4B94` — constant-gloss variant (`| 31u`)
- `0x0048CF38` — minimal normal-only writer (no gloss byte)

This family is the source for RR's normals + roughness and a diffuse-albedo
candidate (T0). The rest of the 615 variants stay in the scan corpus.

## gbuffer-prep/ — packed normal+depth for the RT passes

- `0xBF794558` — packs `float4(normalBits, linearDepth, normalBits, normalBits)`
  from the `uint` normal texture + depth, with per-quadrant hash offsets.
  Feeds the `space36` family (t0/t1 reads in the other passes).

## resolve-denoise/ — spatial resolves of ray results

- `0x596D3E8F` — large spatial filter. 10×5 groupshared cache; weights by
  normal dot + depth + exp falloff; radiance stored in YCoCg
  (`Y=.25/.5/.25`, `Co=.5/0/-.5`, `Cg=-.25/.5/-.25`). Writes `u2` = filtered
  radiance, `u3` = weights, `u4` = sample count.
- `0x209AB6A4` — 4-tap stochastic spatial gather/reprojection of ray results
  (`t22`/`t23`), per-pixel hash jitter, depth + normal rejection, weight
  normalization; writes `u2`/`u3`.
- `0x0B33C6D8` — 6×10 bilateral neighborhood gather, per-tap
  `clamp(dot(n, n_i))` × (1.666 / 2.5 / 5.0) weights; single RGBA output.
  Possibly probe interpolation rather than a denoiser.

## gi-probes/ — 6-lobe probe accumulation / blending / visibility

- `0xBEB68B69`, `0x1324EF7E` — 6-lobe radiance accumulation per probe entry:
  64-thread group sums contributions; `0xBEB68B69` weights by half-packed table
  values. Outputs 6 × float4 per entry; one buffer scaled by `1/(1000π)`.
- `0xD9CDA0AC` — 6-lobe blend: `out = A + k·B`.
- `0xC026B375` — per-entry list management / counters over packed entries.
- `0x9848AF45` — per-entry visibility: sky cubemap, cloud/global shadow,
  CSM cascades (`GatherCmp` PCF), per-light shadow matrices from buffer `t109`;
  samples a bindless radiance volume (Texture3D) and a probe atlas
  (Texture2DArray); writes `float4` per entry.

## ssgi/ — screen-space GI

- `0x8E8540A0` — stochastic screen-space ray march. Per-pixel random direction
  (cone around the decoded normal, driven by a 256×256 noise texture),
  screen-space DDA over the depth buffer with adaptive steps, hit-normal
  rejection, firefly luminance rejection. Output `(color, distance)`;
  `(-1,-1,-1,dist)` = miss/rejected. Likely the screen-space GI ray pass
  (possibly also a fallback/validation path).

## ssr/ — screen-space reflections

No confirmed SSR shader yet. There is no `reflect()`-based march anywhere in
the corpus. See `../ssr/README.md` for the hunt plan (deprioritized for now).

## infra/ — plumbing

- `0x16478515` — max-mip pyramid (12 levels, single channel, `QuadRead*`
  reductions) with a counter-guarded serial tail. Likely the Hi-Z used by
  screen-space marching / ray validation.
- `0x7A3FD6D7` — wave-ballot pixel compaction into two work lists (category
  flag read from bindless `space40`); writes list entries + global counters.
- `0x71D374B7` — 1-thread indirect dispatch / counter setup.
- `0x5175B738` — buffer clear (counter reset).

## potential/ — promising, role not yet confirmed

- `0x139B9597.ps` — fullscreen surface-shading candidate (9.4k lines). Binds
  material textures (space6/7), material bindless sets (space37/38/41),
  GI probes (space12), env cube (t118), volumes (t123/t124), instance buffers
  (t96–98). Writes color + a scalar target. One of **126 PS shaders** sharing
  this binding signature — likely the pass(es) that apply lights + GI +
  reflections onto surfaces, but the exact role needs live frame correlation.
- `0x28508E02.ps` — same family (14k lines), larger output set incl. uint targets.
- `0x0052C157.ps` — smaller member of the same family (3.3k lines), easier to read.
- `0x04C1F302.cs` — space12 probe-system compute candidate (binds space7/9/12,
  buffers t108/109; 2.8k lines).
- `0x0E548F62.cs` — shadow/visibility compute candidate
  (`SamplerComparisonState` + space12 buffers; 473 lines).

## Leads (not copied yet)

From the DXIL resource tables (scan of all 572 compute shaders):

- `space36` core family (RT/GI screen-space + filter core) =
  `0x0B33C6D8, 0x209AB6A4, 0x596D3E8F, 0x8E8540A0, 0xBF794558`.
- `space12` family (25 cs, includes `0x9848AF45`, `0x04C1F302`, `0x0E548F62`)
  — probe/volume subsystem: `0x04C1F302, 0x0E548F62, 0x1040255D, 0x1CF12AA7,
  0x1DA70060, 0x32FDD785, 0x3517E6C9, 0x4E9B4AF4, 0x50869381, 0x53975C50,
  0x7959167E, 0x85DE006E, 0x9399E76A, 0x9848AF45, 0xBEA209EC, 0xC50BF63A,
  0xCE2AFF2B, 0xD0EAA27D, 0xDE4EC37D, 0xE11F58D8, 0xE1A7087B, 0xE27242C0,
  0xE6152210, 0xE91C4F45, 0xF41D4ED6`.
- `space40` family (5 cs) — work-list / image-state plumbing:
  `0x0FEFBAD2, 0x23B22D11, 0x4C29B60A, 0x7A3FD6D7, 0xBAB27FF1`.
- Population counts in the full corpus: **245 PS** decode the packed normal,
  **126 PS** bind material + probe GI (the fullscreen family), **778 PS** do
  shadow comparisons, **898 PS** bind space12 (probe GI), **1,364 PS** bind
  space4.
- No `dlss`/`ngx` strings anywhere in the shader corpus (DLSS is DLL-side).

## TODO

- [ ] Find the specular albedo / F0 source (not exposed as a ready G-buffer
      target; may only exist inside the shading family).
- [ ] Live-confirm: gloss vs roughness semantics, T0 albedo format, T3 resource
      format, trace `u0`/`u2` meaning.
- [ ] Live-correlate the 126 fullscreen family (dispatch/draw order, which one
      runs where) and decide the RR "mute set".
- [ ] Identify the DLSS motion-vector texture + jitter (runtime, not in dumps).
- [ ] SSR hunt (deprioritized — see `../ssr/README.md`).
