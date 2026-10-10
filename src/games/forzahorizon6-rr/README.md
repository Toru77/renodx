# Forza Horizon 6 — DLSS Ray Reconstruction (forzahorizon6-rr)

Ray Reconstruction mod for FH6 via the game's own Streamline instance
(`sl.interposer.dll`): intercept the DLSS-SR evaluation, answer it with
`kFeatureDLSS_RR`, and supply the guide buffers the game does not provide.

**Status: M2c implemented** — the game's `slEvaluateFeature(DLSS)` is redirected to
`kFeatureDLSS_RR` with `DLSSDOptions` mirrored from the game's own settings
(internal resolution stays game-controlled), the world/view matrices derived
per frame from `sl::Constants`, and the guide tags DLSS-RR **requires**
(Albedo, SpecularAlbedo, NormalRoughness) supplied as placeholder textures
until the M3 rrg pass provides real content. Strict per-frame fallback to SR
on any failure, live A/B toggle, and full Streamline log capture.

## Goal roadmap

1. ~~Organize dumped RT/SSR/SSGI shaders~~ (see `shaders/README.md`).
2. ~~Find the remaining pieces: G-buffer writers, NGX/Streamline call path.~~
3. ~~M1: Streamline interception with UI-first diagnostics.~~
4. ~~M1.5: DLSS-RR load path (slInit injection + runtime fallback) + full
   option/requirement capture in the report.~~
5. ~~M2: SR → RR redirect + strict fallback (prove the swap, live A/B).~~
6. ~~M2c: guide provisioning (RR refuses to evaluate without Albedo,
   SpecularAlbedo, NormalRoughness) with placeholder content.~~
7. **M3:** Real guides — NormalRoughness from G-buffer T3 (gloss → roughness),
   Albedo from T0, SpecularAlbedo approximation.
8. M4: Specular motion vectors + hit distance from the DXR trace outputs.
9. M5: Albedo/specular-albedo refinement, dev dumps, verification matrix.
10. Later: muting SSR layering on RT reflections; denoiser bypass.

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
   HDR flag, exposures, presets), `worldToCameraView` / `cameraViewToWorld`
   are rebuilt from the game's `sl::Constants` camera basis using Streamline's
   own convention (`sl_matrix_helpers.h`), and `normalRoughnessMode` is set to
   packed. The game's in-game DLSS settings therefore keep controlling
   internal resolution.
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
| `Albedo` | RGBA8 | gray (0.5) | G-buffer T0 |
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
