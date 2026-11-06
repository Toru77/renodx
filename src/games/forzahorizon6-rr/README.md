# Forza Horizon 6 - DLSS Ray Reconstruction (forzahorizon6-rr)

Replaces Forza Horizon 6's DLSS Super Resolution with **DLSS Ray
Reconstruction**, so RR denoises the game's ray-traced GI/reflection signal
instead of the game's own resolve chain. RR is fed the game's real G-buffer
guides and the raw RT resolve signal.

## How it works

The game loads Streamline's DLSS SR. This add-on:

1. **Requests DLSS-RR at `slInit`** so the `sl.dlss_d` plugin loads with the
   game (restart required after installing/updating the add-on).
2. **Loads RR at runtime** if the `slInit` request did not stick.
3. **Redirects the game's DLSS-SR `slEvaluateFeature` call to DLSS-RR**,
   mirroring the game's own `slDLSSSetOptions` (mode, output size, exposure), so
   the in-game resolution/settings keep controlling internal resolution. A
   failure detected before the guide pass is recorded falls back to the
   untouched SR call; a failure detected after it latches RR off for the
   session instead and hands the frame back with its own error code, because
   the command list already carries our dispatch and must not be evaluated
   again. Every later frame then takes the untouched SR path. The latch is not
   cleared by the toggle — the reporter's log shows it holding for two minutes
   of clean play, then dying the instant an off>on toggle re-armed RR into the
   evaluate that had just thrown. Retrying requires a restart, which recreates
   the Streamline state the failure came from.
   `eErrorExceptionHandler` has a known environmental cause: an outdated
   `nvngx_dlssd.dll` next to the game executable. NGX then resolves RR's preset
   hint onto a deprecated preset and throws out of `CreateDlssInstance`.
   Confirmed by replacing the DLL (310.6.0 -> 310.9.0 fixed it). That result
   raises a red notice naming the DLL, and the log records the version found.
   `eWarnOutOfVRAM` counts as success: Streamline only downgrades to it from
   `eOk` when the VRAM budget is exhausted (`commonInterface.cpp:542`), and the
   frame is rendered. Treating it as a failure discarded working RR frames at 4K
   output; it now enables a stutter notice instead. RR works at 4K — the
   redirect takes `Width` from NGX's `optimalRenderWidth` and `OutWidth` from our
   option (`dlss_dEntry.cpp:470-473`), so upscaling needs no special handling.
4. **Feeds RR real guide buffers** captured from the game's live G-buffer
   (albedo, packed normals, material roughness). A small compute pass builds the
   NormalRoughness/Albedo/SpecularAlbedo guides right before the evaluate; if
   capture is unavailable it falls back to constant placeholder guides.
   The G-buffer capture runs on every recording thread every frame, so its log
   line is rate-limited rather than per-event (`guides::LogThrottled`).
5. **Bypasses the game's denoising stages on Medium and High RTGI** so RR
   receives the raw signal: the shared spatial-disk filter, the tier's temporal
   filter (world cache, history and neighbourhood), and — on Medium, whose
   chain runs at half resolution — the `RTGI_Upscale2x` nine-tap bilateral
   reconstruct that also upsamples the half-res pair to the full-res feed (kept
   as a plain bilinear 2x upsample) are replaced with raw per-frame variants (see `denoise.hpp` and the
   `0x*.cs_6_6.hlsl` files). The tier is detected from its RTGI filter or
   Resolve pass; without Medium or High the bypasses stay off and a red notice is
   shown, but RR still runs on the game's own resolve. High `0x019CE131`
   `RTGI_Resolve` is also replaced by a source-controlled, binding-compatible
   equivalent that preserves its pointwise depth, normal, and CoCg transform.
   That pass has no denoising neighborhood/history, so its math is unchanged.

The RR and RTGI replacement behavior is driven by **Enable DLSS Ray
Reconstruction**. The SSR cleanup and roughness-mip filtering controls below
are independent and leave the game's SSR path unchanged when off.

Known DevKit shader tags:

- `temporalfilter_0xAF4E3D44` — High RTGI temporal filter.
- `temporalfilter_0x95FA8898` — Medium neighborhood-search temporal filter.
- `spatialfilter_0x480011D5` — shared `RTGI_SpatialFilter_Disk` pass.
- `resolve_0x019CE131` — High pointwise resolve; replaced by an equivalent
  source-controlled shader, with its original math preserved.
- `upscale2x_0x4067B14C` — Medium guided resolve/upscale pass.

## Re-discovering RTGI shaders after a game update

Hashes and draw indices can change between game builds. Capture High and
Medium separately, because their temporal-filter and resolve/upscale passes
can have different hashes. Keep DLSS-RR and the same scene/settings active for
each capture so the pass sequence is comparable.

Use `devkit_get_draw` to inspect the RTGI dispatches around the trace pass, then
`devkit_get_shader` on each candidate. Prefer the shader's PDB debug name from
the disassembly; confirm it against the compute thread size and resource
bindings. Draw numbers are examples from the current snapshots, not stable IDs:

| Tier / draw | Hash | PDB pass | How to recognize it |
|---|---|---|---|
| High 5437 | `0x9C7463F8` | `RTGI_ScreenSpaceTrace` | Trace pass; use as an anchor, not a denoiser replacement. |
| High 5438 | `0xAF4E3D44` | `RTGI_TemporalFilter` | History/cache bindings; 8×8 threads. |
| High 5439–5440 | `0x480011D5` | `RTGI_SpatialFilter_Disk` | Reads `t0`, `t1`, `t22`, `t23`, `t28`; writes `u2`/`u3`. |
| High 5441–5443 | `0x019CE131` | `RTGI_Resolve` | Reads `t0`, `t1`, `t22`, `t23`; writes `u0`; source-controlled equivalent preserves its pointwise transform. |
| Medium 4649 | `0x9C7463F8` | `RTGI_ScreenSpaceTrace` | Trace pass; use as an anchor. |
| Medium 4650 | `0x95FA8898` | `RTGI_TemporalFilter_Neighborhood_Search` | History/cache bindings; 8×8 threads. |
| Medium 4651–4652 | `0x480011D5` | `RTGI_SpatialFilter_Disk` | Same shared hash and bindings as High. |
| Medium 4653–4655 | `0x4067B14C` | `RTGI_Upscale2x` | Reads `t0`, `t1`, `t22`, `t23`, `t38`; writes `u0`; 16×8 threads. |

When hashes change, update the tier-specific constants and replacement entries
in `denoise.hpp`, their references in `addon.cpp`, and the shader tags in the
HLSL comments and this table. Keep shader filenames in the loader's canonical
`0xHASH.cs_6_6.hlsl` form; putting a tag or suffix in the filename can prevent
DevKit from finding it. Tags such as `temporalfilter_0x95FA8898` belong in code
identifiers, comments, and logs instead. If two tiers use the same hash, keep
one shared entry; don't infer sharing from similar names or bindings alone.

After editing, point DevKit at the mod's live shader directory and run
`devkit_load_live_shaders`. Check that each intended hash reports `source: File`
and `isCompilationOk: true`, then confirm the live draw still has the expected
PDB pass name and bindings. A successful compile alone does not prove the hash
was mapped to the right pass. Build the `forzahorizon6-rr` target and verify in
game only after closing the game or unloading this add-on.

## Overlay controls

| Control | Effect |
|---|---|
| **Enable DLSS Ray Reconstruction** | The whole feature: runtime load, SR->RR redirect, real guides, and the Medium/High-RTGI denoiser replacements. RR runs at any RTGI tier and at any output resolution; without Ray Traced Global Illumination at Medium or High a red notice is shown and RR receives the game's own filtered resolve. Off = the game runs its own DLSS SR and denoisers unchanged. |
| **RR preset** (D-F) | DLSS-RR render preset applied to every DLSSD preset hint. `D` is the default transformer model, `E` the latest transformer model, `F` the current RR 4.5 default. |
| **Debug logging** | Bug reports only. One line per distinct outcome in `ReShade.log`, plus Streamline's and DLSS-RR's own text routed in through `Preferences::logMessageCallback`. Streamline never writes to ReShade's log on its own (`sl.log/log.cpp:197-222`: console, `OutputDebugStringA`, or a separate file), so the callback is the only way its text reaches `ReShade.log`. Must be on **before** launch, since `slInit` runs at startup. Verbose also disables Streamline's own message dedup (`log.cpp:372`), so nothing is dropped at the source. Lines are instead rate limited on our side by message text, 5 s apart, folding repeats into a `(+N more)` count. Measured on a real log: 138,330 Streamline lines collapse to 5,311 distinct, a 96% reduction with every distinct line preserved. Streamline stamps each line with a free-running elapsed counter (`[234s:289ms:021us]`), which is stripped before keying because it advances on every call and would make every line unique. |

## Instructions

1. Turn DLSS on.
2. Turn Ray Traced Global Illumination at Medium or High.

## Files

- `addon.cpp` - overlay settings + add-on entry point.
- `sl_rr.hpp` - Streamline interception: hooks, the SR->RR redirect, guide
  resources, the DLSS option capture.
- `guides.hpp` + `fh6_guide_nr.cs_6_6.hlsl` - real RR guide buffers from the
  game's G-buffer.
- `denoise.hpp` + `0x019CE131.cs_6_6.hlsl` + `0x480011D5.cs_6_6.hlsl` +
  `0xAF4E3D44.cs_6_6.hlsl` + `0x95FA8898.cs_6_6.hlsl` +
  `0x4067B14C.cs_6_6.hlsl` + `0x313E9A06.cs_6_6.hlsl` +
  `0xF0DE4E22.cs_6_6.hlsl` + `0x24848E0B.cs_6_6.hlsl` +
  `resolve_spatial_raw.hlsli` - live per-pipeline replacement of the
  Medium/High-RTGI denoising stages and optional SSR cleanup/mip filtering.

## Requirements

- DirectX 12, an NVIDIA RTX GPU (driver 535.15+), and ReShade with full add-on
  support.
- `sl.dlss_d.dll` (and `nvngx_dlssd.dll`) present next to the game executable.

## Build

```
cmake --build <build-dir> --config Release --target forzahorizon6-rr
```

Produces `renodx-forzahorizon6-rr.addon64`. Copy/link it into the game folder
next to the ReShade DLL.

## Verify

In gameplay with DLSS on and Ray Traced Global Illumination at Medium or High:

- The overlay shows **Enable DLSS Ray Reconstruction** on and the RR preset.
- The red "Turn on Ray Traced Global Illumination at Medium or High." notice is
  gone.
- The image is denoised by RR (compare against the switch off - the game's own
  DLSS SR).
- At Medium RTGI the half-res resolve chain is bypassed the same way; the
  full-res feed keeps coming from its bilinear 2x upsample, not a 1:1 copy.
- Switching RTGI to Low brings the red notice back; RR keeps running
  but no longer receives the raw signal.
- `ReShade.log` shows `sl-rr: Streamline entry points hooked`.
