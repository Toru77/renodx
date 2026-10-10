# Forza Horizon 6 — DLSS Ray Reconstruction (forzahorizon6-rr)

Ray Reconstruction mod for FH6 via the game's own Streamline instance
(`sl.interposer.dll`): intercept the DLSS-SR evaluation, answer it with
`kFeatureDLSS_RR`, and supply the guide buffers the game does not provide.

**Status: M1.5 implemented** — hooks observe and forward; on top of M1, the mod
appends `kFeatureDLSS_RR` to the game's `slInit` feature list (with a runtime
`slSetFeatureLoaded` fallback) and reports the full DLSS-RR load path. No
rendering behavior is changed yet.

## Goal roadmap

1. ~~Organize dumped RT/SSR/SSGI shaders~~ (see `shaders/README.md`).
2. ~~Find the remaining pieces: G-buffer writers, NGX/Streamline call path.~~
3. ~~M1: Streamline interception with UI-first diagnostics.~~
4. ~~M1.5: DLSS-RR load path (slInit injection + runtime fallback) + full
   option/requirement capture in the report.~~
5. **M2:** SR → RR redirect skeleton + fallback (prove the swap, live A/B).
6. M3: NormalRoughness guide from G-buffer T3 (gloss → roughness).
7. M4: Specular motion vectors + hit distance from the DXR trace outputs.
8. M5: Albedo/specular-albedo refinement, dev dumps, verification matrix.
9. Later: muting SSR layering on RT reflections; denoiser bypass.

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

## Diagnostics UI (M1.5)

Everything is reported in the ReShade overlay under "Ray Reconstruction" —
no log reading required:

- **Status** — Streamline found/armed, slInit result + requested/effective
  features (`+DLSS-RR, mod` when injected), device/LUID, frame token count,
  buffer tag counts, RR load line (slInit injected / runtime load attempts +
  result) and the serving plugin path.
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
  wrappers around `slDLSSSetOptions` / `slDLSSGetOptimalSettings`.
- **DLSS-RR probe** — RR requirements (`slGetFeatureRequirements`: flags,
  required tags, driver), `slDLSSDSetOptions` / `GetOptimalSettings` /
  `GetState` availability, and a trial `slDLSSDGetOptimalSettings` call
  mirroring the game's own DLSS mode.
- **Features** — per-feature evaluate/query results (DLSS, DLSS-G, DLSS-RR…).
- **Feature functions** — which `slGetFeatureFunction` requests the game makes.
- **Recent evaluates** — last `slEvaluateFeature` calls (inputs resolved to
  struct names + versions) + caller module.
- **Hook activity** — call counts per hooked entry point.

Buttons: **Probe DLSS-RR** / **Probe all features** (runs at the next in-game
Streamline call), **Copy report** / **Write report to log**, **Reset capture**.

## Build & verify (M1.5)

- Build target: `forzahorizon6-rr` → `build/<config>/renodx-forzahorizon6-rr.addon64`.
- Deploy next to ReShade (`dxgi.dll` slot) as an add-on DLL; keep the game's
  Streamline files untouched.
- Verification: start the game with DLSS on and ray tracing enabled, reach
  gameplay (~10 s), open the overlay's Ray Reconstruction section, press
  **Copy report**. Expect Streamline hooked (15/15), slInit seen with
  `featuresToLoad (effective)` ending in `DLSS_RR [DLSS-RR appended by mod]`,
  the RR probe reporting `supported/loaded = eOk/yes` for DLSS-RR, the
  `sl.dlss_d.dll` module path, RR requirements with the required tag list, and
  a successful trial `slDLSSDGetOptimalSettings`. The game must run unchanged
  (hooks still forward everything; the only behavior change is requesting the
  RR plugin at init/load).

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
