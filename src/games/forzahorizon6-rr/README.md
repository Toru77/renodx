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
5. **Bypasses the game's resolve denoisers on High RTGI** so RR receives the raw
   signal: the 4-tap reprojected gather and the spatial filter (world cache,
   history and neighbourhood) are replaced with raw centre-sample variants (see
   `denoise.hpp` and the `0x*.cs_6_6.hlsl` files). The High RTGI tier is detected
   from the spatial filter the game binds; without it the bypasses stay off and a
   red notice is shown, but RR still runs on the game's own resolve.

All of this is driven by one switch, **Enable DLSS Ray Reconstruction**.

## Overlay controls

| Control | Effect |
|---|---|
| **Enable DLSS Ray Reconstruction** | The whole feature: runtime load, SR->RR redirect, real guides, and the High-RTGI resolve bypasses. RR runs at any RTGI tier and at any output resolution; without Ray Traced Global Illumination at High a red notice is shown and RR receives the game's own filtered resolve. Off = the game runs its own DLSS SR and denoisers unchanged. |
| **RR preset** (D-F) | DLSS-RR render preset applied to every DLSSD preset hint. `D` is the default transformer model, `E` the latest transformer model, `F` the current RR 4.5 default. |
| **Debug logging** | Bug reports only. One line per distinct outcome in `ReShade.log`, plus Streamline's and DLSS-RR's own text routed in through `Preferences::logMessageCallback`. Streamline never writes to ReShade's log on its own (`sl.log/log.cpp:197-222`: console, `OutputDebugStringA`, or a separate file), so the callback is the only way its text reaches `ReShade.log`. Must be on **before** launch, since `slInit` runs at startup. Verbose also disables Streamline's own message dedup (`log.cpp:372`), so nothing is dropped at the source. Lines are instead rate limited on our side by message text, 5 s apart, folding repeats into a `(+N more)` count. Measured on a real log: 138,330 Streamline lines collapse to 5,311 distinct, a 96% reduction with every distinct line preserved. Streamline stamps each line with a free-running elapsed counter (`[234s:289ms:021us]`), which is stripped before keying because it advances on every call and would make every line unique. |

## Instructions

1. Turn DLSS on.
2. Turn Ray Traced Global Illumination at High.

## Files

- `addon.cpp` - overlay settings + add-on entry point.
- `sl_rr.hpp` - Streamline interception: hooks, the SR->RR redirect, guide
  resources, the DLSS option capture.
- `guides.hpp` + `fh6_guide_nr.cs_6_6.hlsl` - real RR guide buffers from the
  game's G-buffer.
- `denoise.hpp` + `0x209AB6A4.cs_6_6.hlsl` + `0x4DAF8A48_both.cs_6_6.hlsl` +
  `resolve_spatial_raw.hlsli` - live per-pipeline replacement of the High-RTGI
  resolve/denoise stages.

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

In gameplay with DLSS on and Ray Traced Global Illumination at High:

- The overlay shows **Enable DLSS Ray Reconstruction** on and the RR preset.
- The red "Turn on Ray Traced Global Illumination at High." notice is gone.
- The image is denoised by RR (compare against the switch off - the game's own
  DLSS SR).
- Switching RTGI to Medium or Low brings the red notice back; RR keeps running
  but no longer receives the raw signal.
- `ReShade.log` shows `sl-rr: Streamline entry points hooked`.
