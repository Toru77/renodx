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
   the in-game resolution/settings keep controlling internal resolution. Any
   failure for a frame falls back to the untouched SR call.
4. **Feeds RR real guide buffers** captured from the game's live G-buffer
   (albedo, packed normals, material roughness). A small compute pass builds the
   NormalRoughness/Albedo/SpecularAlbedo guides right before the evaluate; if
   capture is unavailable it falls back to constant placeholder guides.
5. **Bypasses the game's resolve denoisers** so RR receives the raw signal: the
   4-tap reprojected gather, the spatial filter and the bilateral reconstruct
   are replaced with raw per-pixel variants (see `denoise.hpp` and the
   `0x*.cs_6_6.hlsl` files).

All of this is driven by one switch, **Enable DLSS Ray Reconstruction**.

## Overlay controls

| Control | Effect |
|---|---|
| **Enable DLSS Ray Reconstruction** | The whole feature: runtime load, SR->RR redirect, real guides, and the raw-signal resolve bypasses. Off = the game runs its own DLSS SR and denoisers unchanged. |
| **RR preset** (A-F) | DLSS-RR render preset applied to every DLSSD preset hint. `F` is the current RR 4.5 default; `D`/`E` are the transformer models. |

## Files

- `addon.cpp` - overlay settings + add-on entry point.
- `sl_rr.hpp` - Streamline interception: hooks, the SR->RR redirect, guide
  resources, the DLSS option capture.
- `guides.hpp` + `fh6_guide_nr.cs_6_6.hlsl` - real RR guide buffers from the
  game's G-buffer.
- `denoise.hpp` + `0x*.cs_6_6.hlsl` + `resolve_spatial_raw.hlsli` - live
  per-pipeline replacement of the RT resolve/denoise stages.

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

In gameplay with RR enabled:

- The overlay shows **Enable DLSS Ray Reconstruction** on and the RR preset.
- The image is denoised by RR (compare against the switch off - the game's own
  DLSS SR).
- `ReShade.log` shows `sl-rr: Streamline entry points hooked`.
