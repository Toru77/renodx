# Forza Horizon 6 — DLSS Ray Reconstruction (forzahorizon6-rr)

Ray Reconstruction mod for FH6 via the game's own Streamline instance
(`sl.interposer.dll`): intercept the DLSS-SR evaluation, answer it with
`kFeatureDLSS_RR`, and supply the guide buffers the game does not provide.

**Status: M2c + RT pass map implemented** — the game's `slEvaluateFeature(DLSS)` is
redirected to `kFeatureDLSS_RR` with `DLSSDOptions` mirrored from the game's
own settings (internal resolution stays game-controlled), the world/view
matrices derived per frame from `sl::Constants`, and the guide tags DLSS-RR
**requires** (Albedo, SpecularAlbedo, NormalRoughness) supplied as placeholder
textures until the M3 rrg pass provides real content. Strict per-frame
fallback to SR on any failure, live A/B toggle, full Streamline log capture,
and an RT pass map (per-frame compute dispatch/hash inventory) that identifies
FH6's resolve/denoise chain for the planned bypass.

## Goal roadmap

1. ~~Organize dumped RT/SSR/SSGI shaders~~ (see `shaders/README.md`).
2. ~~Find the remaining pieces: G-buffer writers, NGX/Streamline call path.~~
3. ~~M1: Streamline interception with UI-first diagnostics.~~
4. ~~M1.5: DLSS-RR load path (slInit injection + runtime fallback) + full
   option/requirement capture in the report.~~
5. ~~M2: SR → RR redirect + strict fallback (prove the swap, live A/B).~~
6. ~~M2c: guide provisioning (RR refuses to evaluate without Albedo,
   SpecularAlbedo, NormalRoughness) with placeholder content.~~
7. ~~M2d (phase 1): RT pass map diagnostics — per-frame dispatch/hash
   inventory to identify the resolve/denoise chain (denoise-bypass evidence).~~
8. **M3:** Real guides — NormalRoughness from G-buffer T3 (gloss → roughness),
   Albedo from T0, SpecularAlbedo approximation.
9. M4: Specular motion vectors + hit distance from the DXR trace outputs.
10. M5: Albedo/specular-albedo refinement, dev dumps, verification matrix.
11. M2d (phase 2, after M3/M4): RT denoise bypass ("mute set", shader
    replacement keyed by the pass-map hashes).

FH6 ships DLSS Super Resolution only — RR does not exist in-game, so it has to
be injected. Streamline provides the RR plugin (`sl.dlss_d.dll`) and runtime
(`nvngx_dlssd.dll`, 310.6) already; the mod only has to feed it.

## Key facts learned so far

- FH6 ray tracing is **compute-based with inline RayQuery** (`TraceRayInline`).
  A scan of every `cs_6_6` (509) and pixel shader (2911) in the dump found
  exactly **16** ray-tracing shaders — all compute, all reflection traces
  (`reflect(I, N)`) with hit-to-screen reprojection. No DXR library or
  raygeneration stages exist anywhere in the dump.
- The RT frame's support stack (G-buffer pack, work-list compaction, spatial
  resolves, 6-lobe probe accumulation, visibility) is organized under
  `shaders/ssgi`, `shaders/resolve-denoise`, `shaders/gi-probes`,
  `shaders/gbuffer-prep`, `shaders/infra`.
- The **G-buffer writer family is identified** (615 PS shaders, curated under
  `shaders/gbuffer-writers`): an 8-target G-buffer with T0 = base color
  (albedo candidate), T1 = sqrt-compressed velocity + depth delta, T2 = packed
  material flags/ids, **T3 = 12+12-bit octahedral normal + 8-bit gloss in the
  low byte**, T4 = secondary color, T5 = damage/extra, T6/T7 = packed ids.
- A **126-shader fullscreen family** binds material textures + probe GI + env +
  volumes (surface-shading candidates, curated examples under
  `shaders/potential`); its exact role needs live frame correlation.
- No `reflect()`-based SSR march and no DLSS/NGX references exist in the shader
  dumps (DLSS is DLL-side; SSR hunt deprioritized).
- G-buffer conventions used by the RT family: `uint`-packed normals
  (12/12-bit fields in bits 8–19 and 20–31, octahedral decode, `v/2048 - 1`),
  separate linear depth; the spatial filter stores radiance in YCoCg; probes
  are 6-lobe (ambient-cube style); work lists are built via wave-ballot
  compaction and consumed with indirect dispatches.
- The dumps were compiled with `dxcoob` (a dxc variant), so the DXIL kept its
  resource binding tables (registers + spaces). That is how the `space12` /
  `space40` lead lists in `shaders/README.md` were produced.

## Layout

```
src/games/forzahorizon6-rr/
├── README.md                    this file
├── addon.cpp                    ReShade addon: settings UI, diagnostics panels, present poll
├── sl_rr.hpp                    Streamline hook layer + diagnostics state + report builder
├── metadata.json                deploy metadata
├── rrg/specmv                   (M3/M4) guide + spec-MV passes — not written yet
└── shaders/
    ├── README.md                inventory, tooling, leads, TODO
    ├── ssr/                     screen-space reflections (not located yet)
    ├── ssgi/                    screen-space GI (stochastic screen-space march)
    ├── dxr-trace/               inline RayQuery reflection traces (16 permutations)
    ├── gbuffer-writers/         material/geometry passes writing the G-buffer
    ├── gbuffer-prep/            packed normal+depth prep for the RT passes
    ├── resolve-denoise/         spatial resolves / reprojection of ray results
    ├── gi-probes/               6-lobe probe accumulation, blending, visibility
    ├── infra/                   Hi-Z pyramid, work-list compaction, indirect args
    └── potential/               promising, role not confirmed yet
```

Each category folder holds its raw decompiled dumps in a `.dumps` subfolder.

## Diagnostics UI (M2)

Everything is reported in the ReShade overlay under "Ray Reconstruction" —
no log reading required:

- **Status** — Streamline found/armed, slInit result + requested/effective
  features (`+DLSS-RR, mod` when injected), device/LUID, frame token count,
  buffer tag counts, RR load line (slInit injected / runtime load attempts +
  result), serving plugin/NGX module paths, and the redirect line
  (`N RR frames, M fallbacks` + last attempt results or last fallback reason).
- **Replace DLSS SR with Ray Reconstruction** — live A/B toggle (default on).
  Off: the game's DLSS SR runs unchanged.
- **RR preset** — slider A–F applied to every DLSSD mode (default F, the
  current RR 4.5 preset; D/E are the transformer models). Changes apply on
  the next frame.
- **Request DLSS-RR at slInit** — appends `kFeatureDLSS_RR` to the game's
  `featuresToLoad` in the `slInit` hook (default on; restart required).
- **Auto-load DLSS-RR at runtime** — if the feature is still not loaded when
  the probe runs, calls `slSetFeatureLoaded(kFeatureDLSS_RR, true)` and reports
  the result (default on).
- **Game buffers (tags)** — every buffer the game hands Streamline, with
  format, size (mips/layers), D3D12 state, lifecycle, full extent (left/top)
  and last frame seen; cleared tags keep their last set resource.
- **Frame constants** — latest `slSetConstants` (jitter, mvec scale, camera,
  matrices, depth/MV convention flags).
- **DLSS options (captured)** — mode, output size, HDR flag, exposures,
  presets, and the optimal-settings answer, captured through forward-only
  wrappers around `slDLSSSetOptions` / `slDLSSGetOptimalSettings`. These
  captured values are what the redirect mirrors into `DLSSDOptions`.
- **DLSS-RR probe** — RR requirements (`slGetFeatureRequirements`: flags,
  required tags, driver), `slDLSSDSetOptions` / `GetOptimalSettings` /
  `GetState` availability, a trial `slDLSSDGetOptimalSettings` call mirroring
  the game's own DLSS mode, and the list of loaded dlss/nvngx/NGX-store
  modules (identifies OTA plugin file names).
- **SL log (captured)** — Streamline's own log messages (last ring, plus a
  warn/error-only ring with repeat counts).
- **RT pass map** — compute dispatches per frame (direct + indirect) with
  shader hashes: candidate RT passes with counts/order, top dispatches of the
  last frame, and cumulative totals. This is the evidence for choosing the
  RT denoise bypass ("mute set").
- **Features** — per-feature evaluate/query results (DLSS, DLSS-G, DLSS-RR…).
  With the redirect on, the DLSS row counts only fallback frames; the DLSS-RR
  row counts the redirected frames.
- **Feature functions** — which `slGetFeatureFunction` requests the game makes.
- **Recent evaluates** — last `slEvaluateFeature` calls (inputs resolved to
  struct names + versions) + caller module.
- **Hook activity** — call counts per hooked entry point.

Buttons: **Probe DLSS-RR** / **Probe all features** (runs at the next in-game
Streamline call), **Copy report** / **Write report to log**, **Reset capture**.

## M2c redirect behavior

Per frame, when the game evaluates `kFeatureDLSS` with the toggle on:

1. Captured SR options are mirrored into `DLSSDOptions` (mode, output size,
   HDR flag, exposures), the render preset comes from the **RR preset**
   slider (A–F, default F — the RR 4.5 preset — applied to every mode),
   `worldToCameraView` / `cameraViewToWorld` are rebuilt from the game's
   `sl::Constants` camera basis using Streamline's own convention
   (`sl_matrix_helpers.h`), and `normalRoughnessMode` is set to packed. The
   game's in-game DLSS settings therefore keep controlling internal
   resolution.
2. Guide textures (render-sized, created once per resolution via the game's
   D3D12 device) are ensured: `NormalRoughness` RGBA16F (flat view normal +
   roughness 1), `Albedo` RGBA8 gray, `SpecularAlbedo` RGBA8 black. DLSS-RR's
   plugin refuses to evaluate without these three tags — they are **required
   inputs**, not quality options.
3. `slDLSSDSetOptions` is called for the game's viewport, then
   `slEvaluateFeature(kFeatureDLSS_RR, ...)` runs with the game's own frame
   token and command list, extended with local `ResourceTag` entries for the
   three guides (sl.common checks local tags before the frame's global tag
   store). The game's own tags/constants for that frame are already in place.
4. Any failure (functions, options, constants, guides, SetOptions or evaluate
   error) falls back to the untouched SR evaluate for that frame, counted with
   a reason in the report; redirect stays on and retries next frame.

Expected visual result at M2c: RR evaluates end-to-end and replaces DLSS-SR,
but since the guides are constants the image will be soft/mushy — this step
proves the full RR pipeline. The real gains land with M3 (rrg pass decoding
G-buffer T3 into the same NormalRoughness/Albedo textures) and M4.

## Streamline log capture

At `slInit` the mod installs its own `logMessageCallback` and chains the
game's callback (if any) behind it, so the engine's logging stays intact. It
can also raise SL's log level to verbose (setting, restart required, default
on); Streamline's warnings/errors are always delivered regardless of level.
The last messages (plus a warn/error-only ring and repeat counts) appear in
the **SL log (captured)** overlay panel and the `[SL log]` report section —
this is how evaluate failures get their exact reason (e.g. missing tag names).
The `[DLSS-RR probe]` report block also lists every loaded dlss/nvngx/NGX-store
module, which identifies OTA plugin files whose names are hashes
(e.g. `190_E658703.dll`).

## RT pass map (M2d)

Counts every compute dispatch per frame (direct and indirect) and records the
first-seen shader-hash order, using the same CRC32 hashes as the dump
filenames (`shaders/README.md`). The **RT pass map** overlay panel and the
`[RT pass map]` report section (included in **Copy report**) show:

- candidate passes that ran (the 16 traces, `resolve-denoise`, `gi-probes`,
  `infra`, `ssgi`) with per-frame counts, direct/indirect split, and
  **cumulative counts** (cumulative catches intermittent passes like the
  reflection traces),
- the top indirect dispatches that are not curated (work-list passes),
- unresolved dispatches (shader hash 0, untracked pipeline) for both the last
  frame and the session — resolved through two fallbacks: an **independent
  pipeline→shader-hash tracker** (CRC32 computed from the compute-shader
  subobject at pipeline creation) and, for pipelines whose creation never
  reached the hooks, **PSO-blob identification** (the cached blob is scanned
  for DXBC containers and CRC32-ed with the dump convention). Report shows the
  fallback/blob-identified counts, the tracked-pipeline total, and any
  remaining unresolved pipeline handles,
- the first-seen order of the last frame (up to 96 distinct shaders) and the
  top dispatches, plus cumulative totals.

Shader-utils pipeline tracking is activated for this (`renodx::utils::shader`),
which also primes the runtime shader-replacement path the denoise bypass uses.

## RT denoise bypass (M4)

Live, togglable replacement of the confirmed-live resolve shaders with
minimal per-frame variants that skip the game's neighborhood filtering
(`denoise.hpp` plus the root shader files `0x*.cs_6_6.hlsl`):

| Hash | RT tier | Live role (from the dump) | Bypass |
|---|---|---|---|
| `0x209AB6A4` | all | resolve-4tap: four-pixel stochastic reprojected gather with weight normalize | passes the pixel's own samples straight through (no gather, no reprojection weighting), with the original's hash-jittered depth guard and invalid-depth fallbacks |
| `0x596D3E8F` | medium | resolve-spatial: large shared-memory spatial filter with per-material paths and counters | faithful current-frame port of the original's per-pixel paths (material decode, 3x3 normal/depth-weighted reconstruction, YCoCg + normal-lobe output, confidence gates, NaN guards) with the history blend, world-cache blend and wave smoothing removed; shared body in `resolve_spatial_raw.hlsli` |
| `0x4DAF8A48` | high | same spatial filter stage, recompiled permutation (identical bindings and per-pixel path) | same shared body |
| `0x0B33C6D8` | medium | resolve-bilateral: nine-tap bilateral gather with Co/Cg + normal-lobe reconstruction | raw nearest sample of the half-res pair at the original's center-tap coordinate (no bilateral weighting, no averaging) |
| `0x14FA42AB` | high | reconstruct stage, recompiled permutation: nearest `px / m[20]` sampling | raw nearest sample at the original's own `px / m[20]` coordinate |
| `0xD9CDA0AC` | all | probe accumulation blend: `out = A + w·B`, six SH coefficients per probe, runs in budgeted slices (~5x/s) — the multi-second GI refinement ramp | full-weight integration (`out = A + B`) per A/B toggle "Bypass probe smoothing"; revert if the brightness balance shifts |

RT quality tiers compile different permutations of the spatial/reconstruct
stages, so each switch replaces every known permutation hash; whichever
variant the game actually dispatches is the one that swaps (the other Add
entries are inert until that pipeline exists).

Mechanics: three overlay switches — **Bypass 4-tap gather (0x209AB6A4)**,
**Bypass spatial filter (0x596D3E8F / 0x4DAF8A48)**, **Bypass bilateral
gather (0x0B33C6D8 / 0x14FA42AB)**, all default off — each call
`AddRuntimeReplacement`/`RemoveRuntimeReplacements` for its hashes on the
ReShade device, so every stage can be A/B-tested in isolation. The swap is a
bind-time pipeline clone (`use_replace_async`), so it applies within a frame
and never modifies the game's own PSOs; switching off restores the original on
the next bind. The status panel and the report's `[RT denoise bypass]` section
show each switch, its applied state and the covered hashes.

These bypasses are structure-preserving: each keeps the original shader's
bindings, register spaces, guards, energy clamps and output semantics, and
removes only the filtering. The 4-tap bypass passes the pixel's own samples
straight through; the bilateral switch takes a raw nearest sample of the
half-res pair at the original's own tap coordinate (medium `px >> 1`, high
`px / m[20]`), so the resolve feed keeps the rawest per-frame noise at the
cost of 2x2 half-res block structure.

Bypass verify (A/B): in gameplay with RR on, flip one switch at a time and
watch the resolve feed change (filtering off → rawer/noisier reflection+GI
resolve); the report should show the corresponding line as
`on, applied: yes` with the covered hashes; flip it back and the next binds
return to the game's original for that pass (`off, applied: no`).

## Real guide buffers (M3a, experimental)

Captures the game's G-buffer and feeds DLSS-RR real guides instead of the M2c
placeholders (`guides.hpp` + `fh6_guide_nr.cs_6_6.hlsl`; overlay switch
**Real guide buffers (experimental)**, default off).

Sources (live-verified via DevKit on a garage snapshot — the writer shader
hashes drift per game build, so capture keys on the render-target format
signature instead):

| Guide | Source | Notes |
|---|---|---|
| NormalRoughness (RGBA16F, packed mode) | our compute pass decodes the packed-normal target (RT3, `r32_uint`) and packs a roughness candidate from material bits (RT2, `r8g8b8a8_uint`) | normal packing is the same 12+12-bit layout the game's own resolve chain decodes; dump the generated texture to inspect |
| DiffuseAlbedo | linearized copy of the captured albedo target (RT4, `r8g8b8a8_srgb`) written by the same pass (RGBA16F) | DLSS-RR requires linear albedo and rejects sRGB; the sRGB G-buffer target is decoded by hardware on read and stored linear |
| SpecularAlbedo | generated in the same pass from albedo: `0.04 + albedo²·0.5` | approximation — no first-class live specular target confirmed yet; RT5 is the paint/clearcoat layer, RT1 is flat |

Mechanics: both `bind_render_targets_and_depth_stencil` and `begin_render_pass`
are watched; a bound target list matching the format signature (albedo
`r8g8b8a8_srgb`, material `r8g8b8a8_uint`, normal `r32_uint`, same render
resolution) is captured. The guide pass is dispatched on the game's command
list inside the SL evaluate redirect, right before the RR evaluate (control-rr
`rrg` pattern: root constants + root SRVs/UAVs, no descriptor heaps). Every
step fails closed — placeholders stay in charge whenever a capture, format or
pipeline is unavailable.

Live-verification checklist (with DevKit MCP):
- `[RT guides]` report section: signature seen, captures, captured handles /
  formats / sizes, guide pass ready, last evaluate used real vs placeholders.
- Dump any of the handles (captured G-buffer targets or the generated guides)
  through DevKit to inspect content; the generated NormalRoughness texture is
  RGBA16F and readback-friendly.
- Live toggles: **Guide normals: world -> view transform** (default on) and
  **Guide roughness source** (material.y / material.z / constant 0.5).

## Guide textures (M2c)

Streamline's RR plugin (`sl.dlss_d`) treats these tags as mandatory on every
evaluate: `Albedo`, `SpecularAlbedo`, and `NormalRoughness` (packed mode) or
`Normals`+`Roughness` (unpacked). Missing any returns
`eErrorMissingInputParameter` ("Failed to find global tag '%s'" in the SL
log). M2c therefore creates three render-sized textures once per resolution
via the game's D3D12 device (upload staging + copy + barrier on the game's
command list) and passes them as local `ResourceTag` entries in the evaluate
inputs:

| Tag | Format | Placeholder content | M3 replaces with |
|---|---|---|---|
| `NormalRoughness` | RGBA16F | flat view normal (0,0,1), roughness 1.0 | rrg decode of G-buffer T3 |
| `Albedo` | RGBA8 | gray (0.5) | linearized G-buffer T0 (RGBA16F) |
| `SpecularAlbedo` | RGBA8 | black | material approximation |

Old resources are retained for the session (Streamline may still hold raw
pointers from earlier frames); they are recreated on resolution change.

## Build & verify (M2c)

- Build target: `forzahorizon6-rr` → `build/<config>/renodx-forzahorizon6-rr.addon64`.
- Deploy next to ReShade (`dxgi.dll` slot) as an add-on DLL; keep the game's
  Streamline files untouched.
- Verification: start the game with DLSS on and ray tracing enabled, reach
  gameplay (~10 s), open the overlay's Ray Reconstruction section, press
  **Copy report**. Expect:
  - Streamline hooked (15/15), slInit injected, RR probe `loaded=yes` and a
    module list that shows the serving plugin (driver-store OTA files have
    hash names like `190_E658703.dll`).
  - `[RR redirect]` with `setting: on`, `redirected frames` growing at ~1 per
    frame, `fallbacks` staying flat, `guides (placeholder)` with the render
    size, and the `worldToCameraView row0/row3` line filled from constants.
  - `[evaluates]` DLSS-RR row counting evals with `lastResult=Result::eOk`.
  - `[SL log]` with the mod's callback installed (game callback chained) and a
    warn/error ring free of `slEvaluateFeature` failures (if anything still
    fails, it names the exact input).
  - An A/B toggle check: turning the switch off restores the previous DLSS-SR
    image; turning it on changes it again (expect the RR image to look
    softer/mushier than SR — the guides are constants for now).
  - Optional visual confirmation: NVIDIA's DLSS indicator overlay shows the
    RR feature while redirect is on
    (`ShowDlssIndicator=1024` under `HKLM\SOFTWARE\NVIDIA Corporation\Global\NGXCore`).

## Build-safety / conventions

- Files are named `0xHASH.cs_6_6.hlsl` (repo convention `{CRC32}.{TARGET}.hlsl`).
- Raw dumps live under `.dumps` folders: the addon build globs `**/*.hlsl`
  and `**/*.cso` recursively, but **skips folders whose name starts with a
  dot**. Keep raw dumps in dot folders; only reworked, compiling shaders should
  sit next to a future `addon.cpp`.
- Original `.cso` binaries are deliberately **not** committed here — the
  `.cso` glob has no dot-skip and would embed them (and can collide with
  compiled shader outputs). They stay in the game dump folder; see
  `shaders/README.md` for paths and regeneration recipes.
