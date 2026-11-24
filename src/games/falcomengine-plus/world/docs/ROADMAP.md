# Falcom Engine+ Ray Tracing Handoff (snapshot 2026-11-09)

This is the repository handoff for `world/`. Read the relevant "Open problems", "Current state" and "Reference" sections before changing a subsystem. Round-by-round history and the evidence behind past decisions are in `HISTORY.md`. Project entry point and source map: `../../README.md`; project rules: `../../AGENTS.md`.

Four ray-traced screen-space techniques (AO, GI, local-light shadows, sun shadows) were removed. `world/compute` and `world/denoise` hold the shared compute-pass and SVGF denoise infrastructure, and `world/game_lights.hpp` holds the game lamp layouts. GPU-cache ownership of the traced scene follows `rtxdi::SceneRequested()`. RTXDI sun has its own `RtxdiSunMaxRayDistance` setting (default 100 m). `shared.h` slots `reserved_slot_01` to `reserved_slot_18` are reserved so later packoffsets do not move.

## Open problems (owner reports, 2026-10-14)

Tick a box only after the owner confirms the fix in game. The harness is not enough. Evidence: `renodx-dev\falcomengine-plus\world\<folder>\` (`world_pool.json`, `world_pool_meshes.json`, `world_pool_instances.json`, `world_deform_live.json`).

- [ ] P-1 `foliage_animate`: wind foliage is in the BVH but does not move. Cause: wind is admitted at its rest pose by design (`GatePoolDraw`, `bvh/bvh_pool.hpp`). Fix needs a design decision (Step 5). Status: alpha-tested wind leaves are now swayed too (same CPU sway path); option A done: weight 1/(flexibility+1), cap 10% of the instance extent or 0.75 m; not validated in game.
- [ ] P-2 `missing_random`: a mesh that should be in the BVH is missing. After a rebuild, a different object drops out instead. Cause (logs, confirmed): an alpha-texture conflict retires every instance of a mesh (`ApplyPoolMesh` dedup, `AbsorbPoolAlphaMaterial`, `MarkPoolMeshAlpha`). Fix: Step 2.
- [ ] P-3 `dynamic_object_slow`: a sign moving in the wind updates slowly and leaves red/blue ghosts in the debug view. Cause (likely): about 4 presents of pose latency (staging ring, resolve at F+2, BVH update at F+3), plus orphan ghosts kept for 40 frames (static) or 60 frames (dynamic). Fix: Steps 3 and 4.
- [ ] P-4 `tree_leaves_missing`: tree leaves are not in the BVH. Cause (logs): the same alpha conflicts as P-2 (leaf meshes conflicted, `slice: -1`, `tlas_waiting`). The missing pixel is not yet identified. Fix: Step 2, then Step 3 to identify the pixel. Status: alpha-tested wind leaves are now swayed too (see P-1); not validated in game.
- [x] P-5 `random_plane`: a plane/wall in interiors sits at the wrong position and shows red in the debug view. Cause (likely, inspect shows it 1207 frames stale): static instances are never retired on map change (`CompactPool`, mass-retirement branch only counts). Fix: Step 1.

Also found (not owner reports, unchecked):
- [ ] A-1 `world_pool.json` is invalid JSON in most dumps: `bvh_pool.hpp` writes `samples[s].matrix[k]` raw, so NaN prints as `nan`. Use `PoolJsonFloat`.
- [ ] A-2 About 14 meshes fail capture in every log (9 with "vertex range larger than the mesh staging", indices that decode as float positions).
- [ ] A-3 Follow-moving mode (`follow_moving`, commit d79c656e) is not described above. ROADMAP "Path 2" still says moving objects are absent until P2b. Update when Step 4 lands.

## Current state (2026-11-09)

Unified PT M1: RTXDI ReSTIR PT initial sampling and final shading replace the plain path tracer; GI and reflections come from one path tree per pixel (no reuse). SHARC, ReGIR, Sparse Rendering, the material cache and Water Surface use the shared bridge. SHARC terminates a replayed path at a cached vertex: a cache hit before the last vertex gives the shift zero integrand, and a cache hit at the reconnection vertex is exact because the cache is isotropic; the narrow case of a reconnection shorter than about 1.7 voxels to that vertex remains a small bias. Not validated in game.

Unified PT M3: RTXDI ReSTIR PT temporal and spatial reuse run between initial sampling and final shading, with a boiling filter, previous-frame surfaces and motion, a PT-owned configuration generation, and history rejection on cache, sparse and identity changes. Sparse-unselected pixels write an empty reservoir and skip both reuse passes. `prevPrevCameraPos = prevCameraPos` because no third scene slot is kept; this only feeds the footprint-threshold jitter. Not validated in game.

Unified PT M4: duplication maps count spatially shared samples and smooth reservoir stagnancy for decorrelation, and compatibility-guided spatial neighbours are preselected from a dedicated G-buffer. Decorrelation mode and firefly replacement follow the SDK's Ray Reconstruction defaults; sparse-unselected pixels write sample ID 0. Not validated in game.

Unified PT M5: Path Tracing owns the scene request and input transfer, so the light table and lamp status are produced with RTXDI Local and Sun off; the local DI passes and the t41 output are skipped unless RTXDI Local is requested. RTXDI Local and Sun keep their previous behavior when enabled, and the Path Tracing `needs_rtxdi_local` reason is gone. Not validated in game.

PT M0: SHARC 1.8.3 vendored (`world/sharc/vendor`), licences moved to `licenses/`, SM 6.6 support reported on the DXR panel line and in `world_rt12.json`; no behaviour change. Not validated in game.

PT M1: Sora 2nd diffuse path tracing uses the DXR scene, the game sun and RTXDI Local lamps, and the ambient probe on ray misses; its settings and status are available in the Path Tracing section. Output is raw unless Ray Reconstruction is enabled. It needs RTXDI Local (scene transfer and lamps) and works with RTXDI Sun off; sun light at path vertices comes from scene constants and the path tracer's own shadow rays. Not validated in game.

PT M2: SHARC adds a world-space radiance cache to the path tracer, with update and resolve passes, configurable cache settings, and automatic or manual cache clearing. Not validated in game.

PT M3: Sora 2nd path tracing takes bounce-hit albedo from the G-buffer when the hit is visible on screen, otherwise from a world-space material cache learned from visible pixels (one pixel per N x N writes each frame; the cache clears with the path tracing caches on map changes, loading screens and cell-scale or capacity changes). Characters have no roughness or F0 in the G-buffer, so they use the Character Roughness and Character F0 settings. Not validated in game.

PT M4 step 1: Sora 2nd lighting traces per-pixel path-traced reflection at the environment and transmission sites (mirrors keep t21), and the ssr2 water output takes the same reflection. The Reflection Samples, Intensity and Roughness Scale settings tune it; while Path-Traced Reflection is on, dynamic cubemaps and game SSR are overridden at runtime (saved settings unchanged). Not validated in game.

PT M4 water surface (option 2): the Water Surface Reflection setting (default off) makes the path tracer trace water reflections from a copy of the SSR-time depth and mrt0, taken at the SSR draw, into a separate water reflection target that ssr2 reads. The water dispatch has its own counters (water pixels and traces on the panel), the water panel state is cleared on each water trace fallback and at the next SSR2 draw after path-traced reflection or the water setting is turned off, but it can stay stale while the SSR2 draw returns early (for example, when the device is unavailable). The Water roughness slider (default 0.05) sets the base roughness of water reflections, which Reflection Roughness Scale still multiplies. The path is not validated in game.

Unified PT M6: primary-vertex NEE streams a d = 2 direct-light sample from the pixel's game tile cell plus one sun sample into the same path reservoir; final shading resolves the light direction and applies shading-time foliage/wind transmittance with darkening on visible sun samples, splitting indirect intensity by path length. Environment and character pixels both take direct light from the reservoir, with characters using the PT GGX/Lambert material; the game's sun, lamp and ambient terms are replaced under pt_direct_bound. The RTXDI direct-light settings moved to the Path Tracing Direct section and keep their values; character direct light loses the toon ramp. Not validated in game.

Unified PT M2 (water reuse): the water dispatch runs initial sampling, optional compatibility-guided neighbour selection, temporal and spatial reservoir reuse, optional duplication maps, then final shading, using its own context, reservoirs, history and camera reprojection because RTV4 motion does not cover the SSR-time water surface. While Water Surface is on, the water reservoirs cost about 400 MB at 1080p (3 × w·h × 64 B) and about 1.6 GB at 4K, plus about 110 MB of M4 maps at 1080p; the main pass already has the same reservoir cost. Not validated in game.

RTXDI M3.5a raw diffuse is a pre-material local-light estimate: lamp radiance times the game diffuse lighting factor and the reservoir inverse-PDF. Later shading applies base color and dynamic light intensity once; the proposal-only GGX support floor is never emitted as raw energy. Initial local-light candidates are drawn uniformly from the entries of the pixel's game tile-cell point and spot lists (each capped at 63) whose classified lamp type matches their list, which covers every light with a non-zero target; reservoirs and the t41 tuple keep full light-buffer indices. RTXDI surface reconstruction, N1 NRD preparation, and temporal DI now read cb_scene matrices column-major (register r + j is column j), as the game lighting shader does; earlier synthetic tests had encoded the transposed layout.

RTXDI M1–M2d.3 implemented and reviewed: the pinned SDK and input/frame contracts, exact scene-CBV tracker, scene preparation, DI resource setup, capture validation, and D3D11→D3D12 input transfer are in place. The scene input requires the validated first 80 rows (1280 bytes) while retaining the 96-row (1536-byte) carrier; native CB range identity is preserved, copied-row count comes from physical range intersection, and unused carrier rows are zeroed. Native bridge readbacks cover whole-buffer ByteWidths 1280/1296/1520/1536; CPU contracts cover 79/80/81/95/96-row boundaries. M3.1a adds and tests the surface reconstruction contract, including its HLSL compile fixture. M3.1b adds one inline hardware DXR sun visibility query per valid reconstructed surface, with a private raw status buffer and conservative unknown results for invalid metadata or incomplete scene coverage. M3.1c validates the production surface and visibility shaders with a synthetic hardware BLAS/TLAS bridge harness (surface reconstruction, masks, endpoints, coverage, invalid scene and surface, biases, packed alpha cutouts, and output bounds passed on the test device). M3.2 adds tile-list point/spot classification and one RTXDI SDK initial local-light sampling pass, including selected-light hardware visibility, private raw diffuse/status results, and deterministic writes to the three SDK reservoir arrays. The synthetic hardware bridge passes point/spot, ambiguous/untyped classification, visible/blocked/alpha occluder, zero-light, invalid receiver, incomplete-coverage, odd-size guard, and packed-reservoir readback cases. These checks use synthetic scene data; captured game scenes and in-game behavior remain unvalidated. In the current pipeline the spatial pass always runs after the initial pass (RTXDI_FRAME_FLAG_SPATIAL_AFTER_INITIAL), so the initial-pass trace is skipped and final visibility comes from the spatial pass; the Initial visibility setting (the Balanced preset, the default, enables it) traces the initial sample and drops it when blocked, before temporal and spatial reuse. M3.3 adds same-frame SDK spatial DI resampling and one selected-light DXR visibility query per valid surface, with Basic bias correction, compatible-surface merging, material/depth/normal/invalid-neighbor rejection, preserved third-array contents across two frame rotations, and conservative empty/blocked/unknown output cases. The synthetic bridge passes compatible and rejected neighbors, spatial visible/opaque/alpha/incomplete-coverage/empty cases, odd-size guards, and InfoQueue checks. M3.4 adds current-frame motion capture and SDK temporal DI reuse ahead of spatial resampling, with previous-frame inputs retained through the accepted DI fence and light identity restricted to exact same-index matches. The bridge executes two reservoir rotations and checks static zero-motion reuse, no-history and out-of-view-motion fallback, moved/reordered light tables, camera-depth change, disocclusion, and normal/material rejection. M3.5a stages a full-resolution R32G32B32A32_UINT light-index/type/visibility/inverse-PDF result with frame, scene, transfer, configuration, and dimensions identity. D3D12 producer fence B is queued before D3D11 readiness; present-end consumer fence C protects output-slot reuse. The contract tests cover B-before-read and C-before-reuse. The hardware bridge passes the zero-albedo specular proposal, exact tile membership/slice/cap cases, valid/blocked/unknown/empty results, odd-size output readback and guards, with zero D3D12 InfoQueue warnings/errors. The game lighting shader does not consume this output, so existing lighting behavior is unchanged. Captured game scenes and in-game behavior remain unvalidated.

**Superseded by Unified PT M6.** M3.5b adds a guarded Sora 2nd lighting consumer for the staged local-DI tuple at t41. Only a current frame/owner/configuration/scene/transfer match after producer fence B can bind it; the shader independently validates status, light identity, inverse-PDF, and exact same tile/slice list membership capped at 63. Valid visible tuples replace the point/spot loops with the selected lamp's game attenuation and BRDF terms, valid blocked tuples zero those terms, and invalid/unknown/unbound tuples keep the original loops. A synthetic GPU fixture runs the same HLSL selection/loop-count helper for point and spot visible, blocked, unknown, unbound, invalid membership/type, and empty cases; it also checks inverse-PDF scaling and one post-material/intensity application. RTXDI output was bound for a Sora2nd lighting draw; per-pixel application is unmeasured. The panel distinguishes staged output, tuple binding, and a matching lighting draw callback. Captured game scenes and in-game behavior remain unvalidated.

N3c adds a per-transfer-slot RGBA16F publication of RELAX-filtered local diffuse for environment and character pixels. Publication is gated on valid prepared inputs, known N1 visibility status, and N3b distance validity; invalid pixels publish zero. A synthetic 13×9 D3D12 bridge validates environment/character class markers and invalid-zero output, and a shared lighting-helper GPU fixture checks class/status validation, raw fallback, specular preservation, and one application of base color/dynamic intensity. Production staging/binding checks include frame, owner, scene/transfer slot, dimensions, N1/NRD configuration identities, and producer fence B; present-end C protects reuse. In the latest diagnostic capture, the earlier `SceneBindingChanged` rejection was not reproduced: scene promotion and input transfer reached Ready. A separate local-enabled capture reports `InvalidStructuredView` at the Lights input check, so local-DI activation remains unverified pending descriptor-root-cause investigation. Staged output and callbacks do not prove per-pixel application or visual correctness. The panel reports staged output, tuple binding, and a matching lighting draw callback only; it does not claim per-pixel application. N1 specular remains raw. No game-image or performance claim is made; runtime behavior and representative performance remain unvalidated.

N4b adds optional session-only RTXDI diagnostics with a fixed-capacity transition ring, first-failure/reason counters, identity tokens, bounded report serialization, and Copy/Clear controls in the RTXDI panel. Instrumentation observes existing promotion, input-validation, transfer, DI, output-binding, draw-callback, and consumer-fence decisions without changing their predicates; native scene-CBV range evidence is queried only at the existing immediate-context validation point. When structured Lights, Tiles, or Deferred validation fails, the actual validator records the expected record stride, precise first failed predicate, SRV dimension/format/elements/BUFFEREX flags, GetResource and buffer-type outcome, buffer descriptor flags/stride/ByteWidth, and expected-layout byte bounds. The report uses a session-local resource token, never a raw address; source registers (including Lights t11) are captured from the inline binding map, while a source shader hash is unavailable at that callback. The panel and report separate NRD compile availability, backend initialization state/failure stage, effective Raw/Ray-Reconstruction-suppressed/RELAX policy, and whether private diffuse output was staged; staged output is not labeled per-pixel application. Reports exclude raw native addresses and scene content, and explicitly label unavailable data. The latest diagnostic capture passed scene promotion and input transfer (the earlier `SceneBindingChanged` rejection was not reproduced). A separate local-enabled capture fails at the Lights structured SRV check with `InvalidStructuredView`; its descriptor cause remains under investigation. These captures are distinct; do not treat a historical `ConfigurationMismatch` report as evidence for the latest run. Captured-game local activation and runtime performance remain unverified.

N4c adds a session-only selector for RTXDI local status, receiver class, selected light, raw/filtered diffuse, N3b distance validity/magnitude, consumer selection, and sun visibility. It reads current per-slot outputs only after their existing producer identity and B handoff checks; distance export runs only when the normal N3b pass already ran, and slot C/retirement continues to protect reuse. The Sora 2nd debug view changes only output color, preserves auxiliary outputs and lighting eligibility, and follows BVH > RTXDI > legacy presentation precedence; unavailable sources use a checker distinct from valid black. Synthetic GPU tests cover the shared visualization helpers and dispatch the exact odd-size RGBA16F distance-export shader with invalid, certified-miss, class, FP16-rounding, and bounds cases; they do not prove captured-game activation or output. The earlier `SceneBindingChanged` rejection was not reproduced in the latest diagnostic capture, which passed scene promotion and input transfer. Local-DI activation remains unverified because a separate local-enabled capture fails at the Lights structured SRV check (`InvalidStructuredView`). Runtime behavior and performance remain unverified.

N4d adds Gather-gated CPU timings for RTXDI input transfer and local DI recording, plus asynchronous D3D12 timestamp intervals for input unpack/sun work, full local-DI work, and the optional N3b distance and NRD prep/RELAX/publication subsets. Timestamp readback occurs only after the accepted producer fence completes, without waiting or flushing; stale epochs/identities and invalid timestamp pairs are discarded. The subset rows are excluded from `gpu_total`, and the CPU timings are removed from the outer `lighting_rt` attribution. The Performance panel distinguishes accepted producer-B submissions from completed timing batches and interval samples; timing skip events count rejected intervals or unavailable batches. Binding and draw callbacks are labeled as callbacks, not proof of pixel application. These intervals exclude D3D11 pack/copy and cross-queue waits, so they are not end-to-end latency. Synthetic contracts and bridges do not validate captured-game timing; runtime behavior and representative performance remain unverified.

**Superseded by Unified PT M6.** M3.6 adds a separate single-sample RTXDI infinite-light adapter for the directional sun, one DXR visibility query per valid surface, and a per-transfer-slot R32_UINT t42 status texture. Only a matching current sun output after producer B can bind; present-end consumer C protects the slot from reuse. Known visible/blocked status replaces CSM visibility for environment pixels and multiplies the existing character toon visibility once; unknown, unbound, or out-of-bounds status retains the CSM/toon fallback. RTXDI sun requests force the CSM skip latch to zero, so unknown pixels keep a rendered CSM fallback. Local DI remains independently controlled at t41. Synthetic GPU cases cover the SDK one-light unit inverse-PDF, visible/blocked DXR outcomes, masks, finite endpoint, alpha, incomplete coverage, invalid surface/scene/radiance/direction, output bounds, and zero InfoQueue messages; a shared HLSL consumer fixture checks visible/blocked/unknown/unbound sun semantics and environment/character fallback. The bridge and contracts use synthetic data; game capture and in-game appearance remain unvalidated.

**Superseded by Unified PT M6.** M3.7 enables the staged t41 local-DI tuple in the character branch using `charaColor`, the game's distance attenuation, spot cone, and `max(translucency, NdotL)`. Visible selections add the selected point/spot term once; blocked selections suppress the selected lamp term; unknown, empty, invalid, or unbound selections keep the game's lamp lighting. Character surfaces no longer require deferred material rows and remain isolated from environment reservoirs in spatial and temporal reuse. The temporal shader preflights the pinned SDK's first accepted history candidate for exact material/class compatibility and verifies its selected pixel for age-one history. The preflight's GPU cost is unmeasured and requires representative performance measurement in a later validation pass. Synthetic bridge cases cover zero-deferred character sampling, point/spot, blockers, incomplete coverage, both temporal class directions, and lighting fallback behavior; InfoQueue is clean. This is synthetic-harness evidence only; captured game scenes and in-game appearance remain unvalidated.

RTXDI M2 adds one 32-byte light-table entry per copied local light and builds it on the DI queue for both path tracing and RTXDI. Stable Light Remap is Off by default, so temporal translation keeps the existing exact same-index comparison; when enabled, the slot-owned maps use quantized position/radius/colour/type identity, full-tuple collision checks, evaluation compatibility, and deterministic duplicate ordering. Counters, reason codes, readback freshness, panel lines, and `world_rt12.json` ranges cover table production and matching. CPU/GPU contract and synthetic bridge validation are required before owner in-game checks; captured-game behavior and performance remain unvalidated.

ReGIR M4c introduces default-Off GPU pool construction from primary surfaces and lookup at actual path-traced hit positions, falling back to uniform sampling when the current pool is unavailable or invalid. Its discrete mixture PMF assumes an equiprobable 24-bit input; CPU fixtures and GPU readback cover the shared selector, registration reconstruction/deduplication/overflow, reordered two-light weights/build, zero lights, invalid headers, invalid lights, and over-capacity counts; they do not establish game RNG statistics. Runtime diagnostics asynchronously report registered and valid cells and rejected and overflow positions when a readback completes. The CPU identity mismatch matrix does not exercise the production RunPtTrace/RunPtWaterTrace identity gate or bindings. Independent source review of identity, bindings, fallback, barriers, fence publication, last-use extension, quarantine, and teardown is complete; the three P2 blockers identified by review have been corrected and independently re-checked. Production runtime validation of main/water behavior and stale fallback, fence failure injection, and owner in-game validation remain deferred.

N1 adds a private full-resolution reconstruction pass after local DI visibility. Per-transfer-slot RGBA16F diffuse/specular outputs contain linear HDR, unexposed local-light radiance before base color and dynamic-light intensity, with receiver-to-sampled-light distance in alpha for each evaluated lobe with a valid visible sample, even when material scaling makes that lobe's RGB zero. Skipped or unevaluated lobes and blocked, unknown, empty, or invalid results have zero hit distance. A separate status mask distinguishes visible, blocked, unknown, empty, invalid, and receiver class. Environment specular includes the game's MRT1.z/255 scale (GPU readbacks at 0/128/255 preserve diffuse and scale specular); character output uses `charaColor` and has no specular term. These are staged game-shaded pre-composite signals: they are not consumed by the game or denoised, and are not yet NRD-ready.

N2 is a native D3D12 NRD 4.17.1 adapter smoke using synthetic inputs only; it is not connected to N1 or game lighting. N3a adds private N1 preparation and RELAX dispatch experiments with synthetic bridge evidence for execution, packing, synchronization, and finite outputs. That evidence does not establish NRD-valid hit distances for RIS/ReSTIR-selected samples, and no game consumer uses these outputs. The N1 selected-light distance must not be passed to NRD as if it were a valid per-lobe first-hit distance. N3b now records one independent cosine-weighted diffuse first-hit ray per eligible N1-visible/blocked pixel. It uses closest alpha-accepted DXR traversal, current dynamic and shadow-only geometry masks, a separate distance-valid mask, and the prepared scene slot's AABB snapshot. A ray is traced only to its AABB exit; hits near the edge are rejected, a truncated miss is invalid, and 65504 is published as a miss only when coverage reaches the full trace range. The synthetic D3D12 bridge verifies 3x2 readback, alpha continuation inside coverage, alpha continuation beyond coverage invalidation, empty/full-range miss, bounds/invalid inputs, and cosine-distribution moments. The bridge has not measured 720p/1440p distance, prep, or RELAX GPU timings or per-resolution ray/validity counters; performance remains unvalidated and must be measured in a representative runtime. N3c implements private RELAX diffuse publication and a guarded Sora2nd t43 consumer; the synthetic bridge and identity checks pass, but captured-game activation remains unverified. The latest diagnostic is frame 7937: scene promotion and input transfer reached Ready, DI was Off, sun output was Staged, and a matching lighting draw callback was observed. A separate local-enabled capture fails at the Lights structured SRV check (`InvalidStructuredView`). Specular remains raw and outside N3a denoising until its signal contract is separately resolved. Synthetic bridge validation is not captured-game or performance evidence.

RTXDI M6: the RTXDI scene pass copies the IS-FAST noise itself when sun or local Light Size and IS-FAST are on, so the scene pass owns the jitter copy. Not validated in game.


Goal: hardware DXR ray tracing in Trails in the Sky 2nd Chapter, with compute paths retained where a feature still needs them or as a counted fallback. The game API is D3D11; the add-on uses a separate D3D12 device and queue for its DXR path. DevKit-gated: `falcom_world::Use`/`AddSettings` do nothing unless `IsSora2nd() && IsDevkitPresent()`. All code is in `src/games/falcomengine-plus/world/`.

Confirmed in game:
- Static world: classifier admission, live BVH, primary-ray trace, Depth Compare, inspect (HISTORY: Round 8 result).
- Characters in the BVH (HISTORY: Round S2). The owner confirmed "Characters capture properly". Water is not stated.
- random_plane fix P-5 (HISTORY: Owner decisions and dump findings).

Implemented, not validated in game. Each feature's owner test list is in its HISTORY section. Reference sections are in this file.

SVGF M1 (the shared denoise chain of GTVBAO and VBGI), implemented, not validated in game: the temporal and spatial passes are replaced by a reprojection pass (paper Sec. 4.1, with the guide data of Sec. 4.2 and 4.4), a 7x7 spatial variance pass for histories under 4 frames (Sec. 4.2), and a-trous iterations (Sec. 4.3, Eq. 1 to 3, edge stops Eq. 4 to 6; Sec. 4.4 values sigma_z = 1, sigma_n = 128). Settings: History Weight (alpha = 1 - weight), Depth and Normal Rejection, Iterations (1 to 5; the panel shows the divisor-reduced count), Luminance Edge Stop (advanced). The Penumbra Edge Stop rows and the History Clamp rows are removed. Owner test list (Sora 1st, Sora 2nd, Kai where each effect runs):
1. A character in front of a wall facing the same way: no GI or shadow bleeding across the silhouette, still and panning.
2. GTVBGI Multi-Bounce on and off.
3. 60 seconds static: no brightness creep. Start and end screenshots and the luminance stats.
4. Iterations 1 to 5 at Full, Half and Quarter (GTVBGI Half, Quarter and 1/8). The panel shows the effective count.
5. Temporal off, spatial off, both off.
6. Debug views: the GTVBGI Temporal Confidence (GTVBAO debug 34) ramps from black to white over about 32 frames after a camera cut.
7. Perf tab: gpu_*_denoise from a 30-second performance test.
Non-paper choices (each marked in the code): history length cap 32, alpha floor 1/h, eps_z = 1e-3 z, eps_l = 1e-6. Falcor differences recorded in code comments (reference\svgf\.referencecode), not implemented: depth gradient form, spatial luminance term and 4/h boost, moments alpha, tap renormalisation, fwidth consistency tests.

SVGF M2a (GTVBAO), implemented, not validated in game: GTVBAO AO runs the main pass, which writes the SVGF raw layout directly (the AO byte x 1.5, applied once; perf P4), then the shared SVGF chain (`EnsureDenoise` and `DispatchDenoise`, r32_uint output), then the byte upsample. It runs at Full and Half on every game that runs GTVBAO. The main pass is never final; the x 1.5 scale is applied only in `GTVBAO_WriteSvgfRaw`. Removed: the GTVBAO temporal, filter, upscale and normal-prep shaders with their pipelines, textures and settings rows (GtvbaoHistoryClamp, GtvbaoFilterType, GtvbaoFilterRadius, GTVBAOAtrous*, GtvbaoFilterTaps*). New rows: GtvbaoSpatialEnabled, GtvbaoSvgfIterations, GtvbaoSvgfLuminanceEdgeStop. GTVBAO Half Res Iterations (`GtvbaoSvgfHalfResIterations`, 1 to 4, default 4) sets the a-trous count at Half; Iterations (label Full Res Iterations) applies at Full only. Owner test list (Sora 1st, Sora 2nd and Kai):
1. Defaults at Full, then at Half: screenshots of the final image and of Debug AO Only against HEAD; no seams and no white or black blocks at silhouettes.
2. Temporal off, spatial off, both off.
3. Iterations 1 to 5 at Full and at Half.
4. Debug Temporal Confidence ramps from black to white over about 32 frames after a camera cut; Debug Raw AO matches HEAD.
5. 60 seconds standing still: no creep. Then pan, and let a character walk past: no ghosting.
6. GTVBGI on: GI is unchanged, and debug 34 still ramps.
7. Deferred Dispatch on: AO renders and the game does not crash.
Send back: screenshots, the `[gtvbao]` and `[world-denoise]` log lines (no "svgf chain not ready" line), FPS against HEAD, and GPU memory.

GTVBAO perf P1 (skip texels the upsample does not read), implemented, not validated in game: the GTVBAO main pass skips the texels the AO upsample does not read, writing byte 255 without tracing. Mode 2 (the SVGF chain does not reproject, or resolution scaling is active) uses the upsample's tap test at the texel's representative pixel; mode 1 (the chain reprojects) skips only texels without depth. The main pass reads the game depth at t4 (AO only), and the mode, full size and divisor come from `GtvbaoPassPush` (c[31..34]). The tap test's normal term never rejects today (`DecodeFalcomMrtNormalAsGame` returns a unit vector); it is kept so the skip uses the upsample's own rule (`RtUpsampleTapRejected`), and test_gpu_gtvbao models mode 2 by depth only. Off while a debug view (22 and up) is shown. Owner test list (Sora 1st, Sora 2nd and Kai):
1. Full with Temporal off and Spatial off: identical to HEAD (final image and Debug Raw AO).
2. Defaults identical to HEAD at Full and at Half.
3. Half with the denoiser on and off: identical to HEAD.
4. Debug Raw AO (22) matches HEAD.
5. Deferred Dispatch on: AO renders and the game does not crash.
Send back: screenshots, the `[gtvbao]` log lines, FPS against HEAD.

GTVBAO perf P2 (foliage early exit), implemented, not validated in game: with Foliage AO Value 0, a foliage pixel is written as byte 255 and the main pass returns before the AO work (the blend would give 255 anyway). Debug views (22 and up) keep the full pass. Owner test list (Sora 1st, Sora 2nd and Kai):
1. Foliage AO Value 0 in a foliage scene: identical to HEAD (final image and Debug Raw AO).
2. Foliage AO Value 0.5 and Exclude Foliage off: identical to HEAD.
3. FPS in a foliage scene against HEAD.
Send back: screenshots and FPS against HEAD.

GTVBAO perf P3 (AO upsample at Full), implemented, not validated in game: at d = 1 the AO byte upsample skips the taps after the first used one (their weight is exactly 0). It applies to GTVBAO (one shader) and is bit-identical (test_gpu_gtvbao on WARP, 0 mismatches) while the skipped taps have finite weights and values; a NaN or infinite depth, normal or AO value at a skipped tap made the HEAD sums NaN, so only then can the result differ from HEAD. Owner test list (Sora 1st, Sora 2nd and Kai):
1. GTVBAO at Full: identical to HEAD (final image and Debug Raw AO).
2. GTVBAO at Half: identical to HEAD.
3. FPS at Full against HEAD.
Send back: screenshots and FPS against HEAD.

GTVBAO perf P4 (AO written in the SVGF raw layout), implemented, not validated in game: the GTVBAO main pass writes the SVGF raw layout (`GTVBAO_WriteSvgfRaw`) directly, and the separate input pass is gone. `ao_term_a` is still allocated but no longer written; removing it is deferred cleanup. Owner test: the SVGF M2a list items 1, 2, 4 and 6 against HEAD.

GTVBAO perf P5 (CDF hoist, edge metric skip, full-mask stop), implemented, not validated in game: the per-slice cosine CDF terms are computed once per slice; the normal edge metric is skipped at Normal Edge Rejection 0 (its term is exactly 1); the horizon search stops once the bitmask is full (AO only). The GI output is bit-identical (test_gpu_gtvbao on WARP, 0 mismatches); its code generation changes. Item 6 (fast acos) dropped: the FXC /O3 listings of gtvbao_main, gtvbao_vbgi_main and gtvbao_vbgi_far_main already expand acos to the 4-coefficient Abramowitz-Stegun polynomial (its constants 0.018729, 0.074261, 0.212114 and 1.570729 appear in each listing; published error bound about 6.7e-5 rad, not measured here); a cheaper form would trade accuracy for an estimated 2 ALU per call. R16_FLOAT depth mips deferred: the GTVBGI far-field Thin hit window (bitmask thickness, default 0.3) is below the fp16 depth spacing beyond z = 512, and the depth-fallback normal precision changes AO at Normal Detail Response 1.0. Owner test list (Sora 1st, Sora 2nd and Kai):
1. Defaults identical to HEAD (final image and Debug Raw AO).
2. Cosine Sampling modes and Hemisphere Uniform identical to HEAD.
3. Normal Edge Rejection 0 and 0.5 identical to HEAD.
4. GTVBGI far field GI identical to HEAD.
5. FPS against HEAD.
Send back: screenshots and FPS against HEAD.

GTVBAO/GTVBGI upsample controls, implemented, not validated in game: the new advanced-only sections select the current 2x2 joint-bilateral filter, bilinear, nearest valid, or a 4x4 Gaussian joint-bilateral filter. Depth Plane Sigma defaults to 0.02, Normal Edge Power to 16, Bilinear Weight Sharpness to 1, and Nearest Valid Fallback to On; the current algorithm and these defaults preserve the existing reconstruction. At d = 1 all algorithms use the existing pass-through path. GTVBAO disables its mode-2 unread-texel optimization for the wider filter because it reads beyond the current 2x2 footprint. Other RT denoise-chain consumers retain the current algorithm and edge-weight defaults. Owner test list (Sora 1st, Sora 2nd and Kai where each effect runs):
1. GTVBAO at Full and Half and GTVBGI at 1/2, 1/4 and 1/8 with all upsample settings at defaults: compare final output to HEAD; no silhouette blocks or missing AO/GI.
2. Select Bilinear, Nearest Valid and Wider Joint Bilateral at the lower resolutions; check thin geometry, silhouettes, screen edges and the AO/GI debug views.
3. Sweep Depth Plane Sigma, Normal Edge Power and Bilinear Weight Sharpness through their ranges; toggle Nearest Valid Fallback off and on.
4. GTVBAO Wider Joint Bilateral with temporal and spatial denoising separately on and off: no missing AO from skipped raw samples.
Send back: screenshots, the `[world-denoise]` and `[gtvbao]` logs, and GPU timing rows for the upsample variants.


SVGF M3a (tiled a-trous, bit-exact), implemented, not validated in game: the a-trous iterations 0 to 2 (steps 1, 2 and 4) run `world_svgf_atrous_s1`, `_s2` and `_s4`, which stage their taps in group-shared memory (`world_svgf_atrous_tile.hlsli`; reference/svgf/optimizing_svgf.md Sec. 3.3). Their output is bit-identical to `world_svgf_atrous` at the same step (test_gpu_svgf on WARP, 0 mismatches). Steps 8 and 16 still run `world_svgf_atrous`. No setting, storage or perf key changed.

SVGF M3b (tiled variance, bit-exact), implemented, not validated in game: `world_svgf_variance` stages its 7x7 taps in a 14x14 group-shared tile (reference/svgf/optimizing_svgf.md Sec. 3.3). Its outputs u0 and u1 (debug 0 and 11) are identical to the M2 variance (test_gpu_svgf on WARP, 0 mismatches). The early return before the taps is gone; the copy-through path writes the same output.

SVGF M3c (execution reordering, identity storage, bit-exact), implemented, not validated in game: `world_svgf_atrous` reorders the threads of a group by remainder class of its step (optimizing_svgf.md Sec. 3.1-3.2, `PosReordToNorm`), so the 5x5 taps come from a 12x12 group-shared tile. The textures stay in pixel layout. Output is bit-identical to the M2 a-trous at steps 1 to 16 (test_gpu_svgf on WARP, 0 mismatches, including sizes smaller than the step and uneven remainders).

RR D3D12 migration M1–M5, implemented and reviewed; owner runtime validation pending. The game remains D3D11: the add-on observes game NGX initialization and keeps the game's DLSS Super Resolution evaluate as the fallback. RR uses a separate same-adapter D3D12 NGX runtime and direct queue. Shared image copies carry color, motion, packed normal/roughness, albedo, specular, depth and output across the API boundary. The handoff is D3D11 input copy and signal A, D3D12 wait/evaluate and signal B, then D3D11 wait/output copy and signal C. Frame owners and feature retirement remain protected until the relevant completions are observed; unknown completion/device-loss states are quarantined. RR Debug Log gates bounded RR-tagged begin/phase/end/failure evidence and failure dumps; these records use the RR A/B/C timelines and identify RT12 values as unavailable. The RR path does not change the add-on RT, GTVBAO or GTVBGI denoisers. M1 captures D3D12 runtime identity; M2 stages guides/resources; M3 evaluates RR and hands output back; M4 handles resets, retries, device loss and deferred lifetime cleanup; M5 adds failure-stage/status reporting and final documentation. Validation after code review: owner enables RR with DLSS selected, checks the status row and output, switches presets and toggles RR, and confirms game SR fallback on failures. The build/review are complete; runtime behavior is not yet validated by the owner.

R1 (zero-cost BVH gating), implemented, not validated in game: with ray-traced effects off, the BVH, pool, deform and census path does not run (`RtBvhDemanded`, `ApplyBvhInputs` in `world/bvh/bvh_debug.hpp`). Owner test list:
1. Ray-traced effects off, Pool Scan unticked, no debug view: the BVH panel shows "BVH inputs: idle (nothing runs)"; the Pool Scan, Live BVH and Deforming meshes rows are unticked, and the Performance tab (Gather on) shows about zero for the pool and deform CPU rows.
2. Turn RTXDI Local on: the panel shows "forced on by RT effects" and the BVH inputs run as before.
3. Turn all RT effects off: the panel returns to idle. Restart with an RT effect saved as ON: the inputs are on from the first present at the latest.
4. Pool Scan on with all RT off: the panel shows "Pool Scan or debug view"; the scan runs.
5. The M1 census toggle and the depth log still record draws with all RT off.
6. With all RT effects off, the Live BVH and Deforming meshes rows return to unticked at the next present unless Pool Scan is ticked or a BVH debug view is selected; that is intended.

R1b-0 (diagnostic, startup hook-off mask), implemented, not validated in game: `[FalcomWorldProbe] HookOffMask=<n>` in ReShade.ini skips the listed always-on hook groups at startup. It is read once; absent or 0 keeps every hook on. A nonzero mask logs `[world] probe HookOffMask=<n>`. Measure one bit at a time (set the bit, restart, compare the Performance tab rows).

| Bit | Value | Hook group skipped when the bit is set |
|---|---|---|
| 0 | 1 | scene::Use and the enabled store |
| 1 | 2 | RegisterCbTracker (cb tracker) |
| 2 | 4 | constants capture and constants::Use |
| 3 | 8 | push_descriptors and command list init, reset, destroy |
| 4 | 16 | RegisterCensus |
| 5 | 32 | BVH pool hooks (destroy resource, buffer updates, map, copy) |
| 6 | 64 | present handlers (OnWorldPresent, OnCrashLogPresent, OnWorldPresentBvh) |
| 7 | 128 | everything else: shader registry, depth-log events, pipeline state, resource destroy, remaining BVH events |

CPU gap diagnostic (timed rows only, no behaviour change), implemented, not validated in game: PoolCpuTimer counts nanoseconds (was truncated to whole microseconds per scope); new Subset rows draw_hook_total, draw_hook_record, deform_probe_draw, cb_hooks, bind_hook, pool_write_hooks, lighting_rt, rt12_signal_flush, rt12_submit, rt12_wait_b and the wall-clock segments seg_to_rt, seg_after_rt, seg_present (finish_present event); gauge hook_draws. Owner test: three 30 s performance tests at one spot (RT off, inline compute pass, DXR scene transaction), overlay closed.

M2 (CPU, draw records), implemented, not validated in game: QueryBufferInfo and LayoutHasSkinInputs read per-thread caches keyed on the same handles; the caches are cleared when g_record_cache_generation changes, which OnInitPipelineState, OnDestroyPipelineState and OnDestroyResourceWorld (after its buffer_info erase) increment. New Subset timing rows pool_gate, pool_lock, pool_copy and pool_gc (OnPoolScanDraw, direct draws; timers only). test_live checks the generation bumps; the cache lookups in draw_census.hpp are not unit-tested because the harness cannot compile utils/command_action.hpp, so the owner perf run (pool_meshes and instance counts at the same spot) is the parity evidence. Historical harness pins are recorded in `world/docs/HISTORY.md`.

M3a (CPU, pool hook timing), implemented, not validated in game: new Subset timing rows pool_tail (OnPoolScanDraw: P2b slice copy and alpha source capture after the lock) and pool_ind_gate, pool_ind_lock, pool_ind_tail, pool_ind_copy, pool_ind_gc (OnPoolScanIndirectDraw, same phases as the direct rows); timers only. Owner check: a 30 s scene-transaction run at the village spot; pool_gate+pool_lock+pool_tail+pool_copy+pool_gc+the five pool_ind_* rows should cover at least 80% of pool_hooks, with pool_meshes about 726.

M3b (timing only), implemented, not validated in game: with GC owning the traced scene and the pool reduced (G5), the CPU live TLAS is not rebuilt or refit, so tlas_rebuild/refit/skip/apply, blas_build and the phase rows are 0 and the tlas_instances gauge is stale. New Subset rows dxr_tlas_gpu, dxr_records_gpu, dxr_characters_gpu (rt12 lighting D3D12 timestamps) and gc_blas_build_gpu (GC BLAS build timestamps), not in gpu_total because the scene-transaction GPU row spans the fence-B wait; Subset blocks gc_present and rt12_present (OnWorldPresentBvh); gauge pool_reduced (1 while reduced). Owner check: a 30 s DXR scene-transaction run at the village spot.

CPU coverage (timing only), implemented, not validated in game: cpu_total now adds hook_outer_cpu = (draw_hook_total − pool_hooks − deform_hook_cpu, clamped at 0) + (lighting_rt − the four *_record_cpu rows, clamped at 0) + bind_hook + cb_hooks + pool_write_hooks. Perf files without the hook_outer_cpu key used the old total. New Subset rows: draw_hook_pre, rt12_pass_a, rt12_d3d11_passes, rt12_record, rt12_resolve. New gauges: bind_calls, bind_srv_resolves. New top-level keys: gc_trace, pool_reduce_full_reason_at_end. Owner test: (1) the 30 s perf test at the outdoor DXR scene-transaction spot of perf_20261029-195940; (2) the same spot with ray-traced effects off; (3) at the same spot with the scene transaction on and the overlay closed, the ReShade FPS with Gather off, then with Gather on (10 s each). Send both perf files and the two FPS readings.

Deform store compaction (skinned-mesh flicker fix), implemented, not validated in game: when retired BLAS garbage in bvh/deform_live.hpp passes 8 MB and half of the store, the store is compacted instead of reset: the resident BLAS are copied on the GPU to fresh arenas (sized to the live data), their offsets move, and they stay resident, so characters stay traced (compute and DXR) in the reset present. world_deform_live.json stats: resets still counts compactions; reset_objects_dropped stays 0; new compact_failures, reclaimed_bytes, used_bytes, garbage_bytes. Harness: test_deform_live step 6b. Owner check: play long enough for resets > 0, send world_deform_live.json and the ReShade log (lines "BLAS store compacted").

Tiny mesh copies (staging guard G1, guard scan, offset-0 lane), implemented, not validated in game: every mesh copy except the lane copy starts at staging offset 4096 or later (kPoolMeshGuardBytes); the lane copy is described in the next sentence. A whole-buffer vertex or uv copy of at most 64 bytes is staged at offset 0, one per slot, because these copies land at offset 0 whatever offset they are given (3- and 4-vertex meshes failed with "vertex position not decodable"). The cause is below ReShade and not proven. world_pool.json: mesh_guard_hits, mesh_guard_explained, mesh_guard_scan, mesh_lane_copies, mesh_lane_busy. guard sentinel_words counts poison words in the guard (the same meaning as sentinel_words elsewhere); the non-poison count is mesh_guard_scan.written_words, after the lane bytes are excluded. Owner check: Poison on, Reset Pool, wait 10 s, Dump; quad 0x31096f65275eb9bf admitted with an instance.

| Feature | Owner test list | Where |
|---|---|---|
| Path 2: moving objects out of the static pool (P2a, P2a-rev) | Pool Scan near moving objects, rule line, Depth Compare | HISTORY: Path 2, round P2a; round P2a-rev |
| Path 2: per-frame path for flagged moving meshes (P2b) | Working tree; test after P-3 | Reference: Path 2; HISTORY: Owner decisions and dump findings |
| Path 3: alpha-tested foliage (default ON since 2026-10-09) | Six-step owner order | Reference: Path 3 |
| Foliage motion: CPU sway approximation (P-1, P-4) | Not planned yet (see Open decisions) | HISTORY: Owner decisions and dump findings |
| Sign latency (P-3): P2b and orphan timing | Follow-lag readout, ghosts | Reference: Path 2; HISTORY: Work plan |
| Wall normal decode (M2b) | Magenta patches on -Z walls | HISTORY: Wall normal decode (M2b) |
| Perf sampling, block timers, Max 30 column, perf file | Performance tab, 30 s test | HISTORY: CPU spike step 1; Performance tab rows; Performance test and separate dumps |
| Panel cleanup | Panel lines moved to JSON | HISTORY: Panel cleanup |
| TLAS rebuild reduction (skip no-op, slack TLAS, default OFF) | A/B in game | HISTORY: TLAS rebuild reduction (cache-diag-2) |
| Cache diagnostics (Phase 0, 0b) | Dump twice; placement compare | HISTORY: Phase 0 cache diagnostics; Step 0b (cache-diag-1) |
| Refit and drain cleanup; refit diet; rebuild diet; housekeeping | Regression only; no pins changed | HISTORY: Refit and drain cleanup; Rebuild diet; Refit diet; Housekeeping |
| Wind sway range slider | Slider check and dumps | HISTORY: Wind sway range slider |
| Diagnostics round 3b and the M0 to M3 switches | Dump split checks | HISTORY: Round: diagnostics (2026-10-08) |
| DX12 probe (M2a, diagnostic) | Run DX12 probe; send dx12_probe_f<frame>.json from renodx-dev\falcomengine-plus\world | world/rt12/rt12_probe.hpp |
| DX12 probe M2b (DXR round trip, transfer paths) | Run DX12 probe, send dx12_probe_f<frame>.json | world/rt12/rt12_probe.hpp, world/shaders/world_rt12_probe.cs_6_5.hlsl |
| DXR backend M3 (DXR scene transaction on D3D12, scene mirror, alpha mirror) | Owner list in `world/docs/HISTORY.md` | world/rt12/rt12_backend.hpp, rt12_mirror.hpp, world/shaders/world_rt12_*.hlsl |
| DXR backend M4 (characters on DXR: carrier, character BLAS, mask 8) | Owner test in `world/docs/HISTORY.md` | world/rt12/rt12_backend.hpp, world/shaders/world_rt12_carrier.cs_5_0.hlsl, world_rt12_deform_unpack.cs_6_5.hlsl |
| DXR backend M5 (P2b moving objects on DXR: records carrier, records pass, instance table at S + r) | Owner test in `world/docs/HISTORY.md` | world/rt12/rt12_backend.hpp, world/rt12/rt12_mirror.hpp, world/shaders/world_rt12_records_carrier.cs_5_0.hlsl, world_rt12_records.cs_6_5.hlsl |
| GPU geometry cache G2 (validation only, not traced) | Owner test in Reference: GPU geometry cache (G2) | world/bvh/gpu_capture.hpp, world/rt12/rt12_capture.hpp, world/shaders/world_gc_range.cs_5_0.hlsl, world_gc_decode.cs_5_0.hlsl |
| GPU instance set G3 (validation only, not traced) | Owner test in Reference: GPU instance set (G3) | world/bvh/gpu_capture.hpp, world/rt12/rt12_capture.hpp, world/shaders/world_gc_instance.hlsli, world_gc_insert.cs_5_0.hlsl, world_gc_compact.cs_5_0.hlsl, world_instance_math.hlsli |
| GPU instance set G3b (indirect draws; validation only, not traced) | Owner test in Reference: indirect draws in the GPU instance set (G3b) | world/bvh/bvh_pool.hpp, world/bvh/gpu_capture.hpp, world/shaders/world_gc_insert.cs_5_0.hlsl |
| GPU scene on DXR G4 (sub-switch, default ON with the cache) | Owner test in Reference: GPU scene on DXR (G4) | world/rt12/rt12_backend.hpp, world/rt12/rt12_capture.hpp, world/bvh/gpu_capture.hpp, world/shaders/world_gc_*.hlsl* |
| Reduced CPU pool G5 (switch, default ON, effective with the cache and trace on) | Owner test in Reference: Reduced CPU pool (G5) | world/bvh/gpu_capture.hpp, world/bvh/bvh_pool.hpp, world/bvh/bvh_live.hpp, world/bvh/bvh_debug.hpp, world/compute/compute.hpp, addon.cpp |

**Superseded by Unified PT M6.** RTXDI M7: Character Max Darkening (sun, local) is merged into Character Shadow Darkening; the old max values are dropped. Not validated in game.

**Superseded by Unified PT M6.** RTXDI M8: optional gate (default On) multiplies the RTXDI sun character darkening by 1 - the game's t7 sun visibility. Not validated in game.

**Superseded by Unified PT M6.** RTXDI M9: wind-foliage (INSTANCE_FOLIAGE) hits in the RTXDI sun ray pass Wind Foliage Transmission per layer (visibility bits 12-15). Not validated in game.

**Superseded by Unified PT M6.** RTXDI M10: blocked final local samples are discarded from the shading reservoir (toggle, default On); the final shadow ray redraws the light-size jitter. Not validated in game.

**Superseded by Unified PT M6.** RTXDI M11: the RTXDI rows are split into the sections RTXDI Local, RTXDI Sun and RTXDI Shared (Shadows category); setting keys and saved values are unchanged. Not validated in game.

**Superseded by Unified PT M6.** RTXDI M12: the M10 toggle Drop Blocked Samples From Reuse (Local) and the M8 gate Character Shadows Follow Game Shadow (Sun) are removed after in-game tests (thicker lamp shadows and no noise gain; the gate changed no pixel because t7 has no character shadow). Not validated in game.

**Superseded by Unified PT M6.** RTXDI M13: Sun Detail gains Layers (wind and leaf layer counts) and Blocker (foliage, alpha, shadow-only or character blocker) views; the image is unchanged. Not validated in game.

**Superseded by Unified PT M6.** RTXDI M14/M14b: wind-foliage (INSTANCE_FOLIAGE) shadows count one object per ray; with Wind Foliage Softening > 0 the main ray ignores wind objects and a second ray, offset up to that radius perpendicular to the sun (noise, like Light Size), counts them, so their shadow edge blurs; the sun visibility word carries the wind occlusion in bits 16-23 for visible pixels. Not validated in game.

**Superseded by Unified PT M6.** RTXDI M14c: P2b records of wind rest-pose draws carry INSTANCE_FOLIAGE (bvh_live.hpp dynamic table, world_dynamic_expand), so wind objects flagged moving keep wind shadow softening. Not validated in game.

**Superseded by Unified PT M6.** RTXDI M15: Character Self-Shadow Softening (Local, m): for character receivers the lamp shadow ray origin moves sideways by the light-disk sample times min(D / distance, 1), with the into-surface part removed, so self-shadows soften as if the blocker were D m away; 0 or Light Size 0 = unchanged. Not validated in game.

**Superseded by Unified PT M6.** RTXDI M16: Character Self-Shadow Softening (Sun, m): for character receivers the sun shadow ray origin moves sideways by the sun-disk jitter times D, with the into-surface part removed, so self-shadows soften as if the blocker were D m away; 0 or Light Size 0 = unchanged. Not validated in game.

**Superseded by Unified PT M6.** RTXDI M17: Temporal Accumulation (Sun) (default On) and Accumulation Frames (Sun) (2-32, default 8): with Light Size (Sun) > 0, world_rtxdi_sun_accumulate runs after the sun rays, blends this frame's visibility with the reprojected history (motion vectors, previous depth/normal/class, 3x3 clamp, weight max(1/(n+1), 1/N)) and writes status FILTERED (3) with the filtered occlusion in bits 16-23 (layer bits kept for tint); UNKNOWN, rejected history and the first frame keep the raw word; at Light Size 0 no pass runs. Sun-only RTXDI captures motion vectors when accumulation is wanted. Deferred, found while planning: with a Sun Detail view active, sun_results ends the detail block in COMMON (rt12_rtxdi_transfer.hpp:2012) while the closing barrier (2031) says UNORDERED_ACCESS. Not validated in game.

RTXDI M18: Boiling Filter (Local) (default On) and Boiling Filter Strength (Local) (default 0.2, the SDK values): the temporal DI pass runs RTXDI_BoilingFilter over 8x8 groups before storing; all threads reach the filter, and out-of-bounds threads carry an empty reservoir. Not validated in game.

RTXDI M19: Spatial disocclusion samples work as in the SDK: targetHistoryLength is 2 with temporal reuse (0 without), so pixels whose temporal reuse found no history (M = 1) take max(disocclusion, spatial samples) neighbours; default 8, presets 4/8/16. Not validated in game.

Ring depth `kRt12Slots` is 4. At 3 the scene and RTXDI transfer slots were still in flight when reused 3 frames later, so the frame was skipped (Busy). Raise it again if the RTXDI report shows `reason=16` growing.

## Reference

### Code map (`world/`)

- `world.hpp` entry; `world_settings.hpp` Ray Tracing tab; `world_state.hpp` census/camera/depth state.
- `capture/`: draw observation (`draw_census.hpp` calls the pool and deform hooks per draw), CB/SRV slot tracking, CPU mirror of small CBs (`cb_value_tracker.hpp`, gives draw-time `instanceOffset_g`), camera capture (`camera_capture.hpp`), depth source, state capture.
- `contract/`: DXBC reflection (`dxbc_reflect.hpp`: RDEF/ISGN/OSGN/SHEX), the game's geometry contract and VS/PS classifier (`shader_contract.hpp`), runtime registry keyed by pipeline (`shader_registry.hpp`).
- `bvh/alpha_atlas.hpp`: alpha slice table and texel packing (CPU). `bvh/alpha_live.hpp`: atlas GPU objects, the blit and `SyncLiveAlpha` (UpdateLiveBvh). `AlphaGpu` and the destroy path are in `bvh_resources.hpp`.
- `bvh/bvh_pool.hpp`: the CPU world pool (draw-time copies, staging ring, resolve, observations/admission, mesh capture + verification, motion probe, P2a, alpha source copies, dump). `bvh_live.hpp`: GPU arenas + TLAS. `bvh_build.hpp`: CPU LBVH. `bvh_resources.hpp`: GPU buffers, `WriteBufferRange`. `bvh_trace.hpp` + `shaders/world_bvh_trace.*`: trace pass (17 SRVs, 40 push constants). `bvh_debug.hpp`: per-present orchestration (`OnWorldPresentBvh`: UpdateLiveBvh, UpdateDeformLive, debug pass). `deform_probe.hpp`, `deform_live.hpp`: path 1. `camera_fade.hpp`: near fade mirror. `gpu_timer.hpp`. `world_bvh.hpp`: panel + event registration. `world_bvh_refit.cs_5_0.hlsl`: path 1 refit.
- `debug/`: crash log (vectored handler + CRT hooks, "Log crashes"), pool stage markers.
- `reference/.dumps/`: decompiled game shaders (3Dmigoto) used as evidence.
- Present order: `OnWorldPresentBvh` (registered first) runs before `OnWorldPresent` (frame++, DrainPoolScan, DrainDeformProbe).

Game facts: all world VS read `StructuredBuffer<InstanceParam> instances_g : t15` (stride 160: world float4x3 @0 stored so world.x = dot(float4(p,1), m[0..3]), translation in m[3], m[7], m[11]; prevWorld @48; color @96; uv @112; param @128; boneAddress @144); index = SV_InstanceID + `instanceOffset_g` (b1 c0.x). Rigid VS project with `cb_scene.viewProj_g` (camera) or `cb_shadow.shadowViewProj_g` (light).

### Invariants and lessons (do not relearn these)

- Lock rule (root cause of the 2026-10-06 crashes): no graphics call while holding `g_pool.mutex`, `g_deform.mutex`, `g_deform_live.mutex`... Reserve under the lock, record after it; resources moved out under the lock and destroyed after.
- ReShade D3D11: `update_buffer_region` with offset 0 passes no box (writes the whole buffer, over-reads the source) -> `WriteBufferRange`; `get_resource_desc` buffer stride is 0 -> pass strides explicitly; `create_pipeline` returns failure for stream-output state; ReShade 6.8.0 (API 20) added a `query_type` parameter to `device::get_query_heap_results` with no compat path for API-18 add-ons -> use native D3D11 queries.
- `constant_buffer` descriptors in `update_descriptor_tables` are `api::buffer_range {buffer, offset, size}`, not a `resource_view`. Passing a view makes ReShade read the next struct members as offset and size; a window that passes D3D11.1 alignment points at the wrong constants (the removed ray-traced AO, GI, local-shadow and sun-shadow passes; see `world/docs/HISTORY.md`). Use `SceneCbvRange` (`world/compute/compute_resources.hpp`). The legacy `addon.cpp` sites (15, `captured_scene_cbv_view`) use the same pattern and are deferred.
- A render target is bound in shadow passes too: decide camera vs light from the VS (`contract::ClassifyVertexView`), not from bindings.
- Pool copies only on the immediate context (deferred lists may run after their slot is read).
- Mesh capture: two identical captures before admission (`verify_meshes`), GPU-written VB/IB contents can change unseen.
- Never run git inside the device VM mount (left `.git/index.lock` once, deletes are blocked there). Locally this does not apply.
- Files in the addon are CRLF; keep it.
- New shaders in `world/shaders/` are embedded automatically (`__<name>_EMBED_FILE`, `__<name>` span; code guards with `#if defined(...)`).
- Never put `.cso` files under `src/games/falcomengine-plus/` (CMake globs and embeds them).
- FXC was never available in the cloud; shaders were checked with DXC cs_6_0 there. On this PC the addon build runs FXC (`fxc.exe` in the repo `bin/`); a clean addon build is the compile check.
- clang-tidy cannot run on addon.cpp on this PC: the VS-bundled clang-tidy 19.1.5 fails on the MSVC 18 STL (STL1000 requires Clang 20+). Until a clang-tidy 20+ is installed, the addon build is the only C++ check; clang-format 19.1.5 still applies to changed statements.
- Build the addon from an MSVC x64 environment: run the build command inside vcvars64 (cmd /c "call C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat && cmake --build E:\RenoDX\renodx\build --config Release --target falcomengine-plus"); a plain shell fails.

### Native test harness (`<repo>/.falcom-dev/harness`)

Windows (MSVC) harness only. The Linux/WSL setup (`build.sh`, `run_all.sh`) is retired; do not use it. A mock ReShade device/command list (stream output emulation, ReShade's offset-0 and stride-0 quirks reproduced), stub `src/utils` (log/path/data/scene/settings), and transcriptions of the GPU shaders for CPU-vs-brute-force checks. See `.falcom-dev/harness/README.md`.

Workflow: `powershell -File refresh_world.ps1` (copies the repo's `world/` sources into the harness; always before a build), then `powershell -File run_win_all.ps1`, or `run_win_some.ps1 -Tests ...` / `run_win_one.ps1 -Test ...`. Tests: test_pool, test_indirect, test_switches, test_visibility, test_live, test_build, test_verify, test_deform, test_deform_live, test_motion, test_alpha (11 programs). Real shader bytecode for the classifier tests is in `.falcom-dev/bytecode/`. No ThreadSanitizer or ASan run on Windows in the 2026-10-08 round; the earlier TSan results were from the Linux setup and do not cover later changes.

Working method for each round: implement, add/extend a harness test that reproduces the in-game situation, refresh and run all tests, build the addon target (the real compile check and FXC for shaders), then give the owner exact in-game steps and what to send back. Compare the default path against HEAD when a change touches shared pool logic.

Harness result after the 2026-10-09 default change: the full run had 4 of 11 programs failing (test_pool, test_switches, test_verify, test_alpha). They assumed the old OFF defaults (an alpha-skip count, a WindOpaque refusal, the indirect-waiting case, and dump strings with `false`). They now pin the switch they test or expect `true`; the logic is unchanged. Re-run: all four PASS; the other seven PASS in the full run. Full run not repeated after the fix.

### Owner preferences

- Step by step; each milestone validated in game before the next. "No mistakes. No regressions."
- Reports: what changed, why, what was verified, exact test steps, what to send back (panel lines, `world_pool.json`, `world_deform.json`).
- Discovery first when the design depends on an unknown.

### Path 1 mechanics (done): deforming meshes

- Discovery (S1, `bvh/deform_probe.hpp`): re-runs sampled camera-view draws of deforming VS classes with a pass-through stream-output geometry shader, finds which VS output is the world position by projecting with the frame's viewProj. Results in "Round S1b" in the history.
- Live (S2, `bvh/deform_live.hpp`, `shaders/world_bvh_refit.cs_5_0.hlsl`): per frame, each confirmed camera draw (direct, immediate context, no game GS/HS/DS/SO bound) is re-run with SO into a 24 MB arena (float3/vertex); first capture of a draw identity is read back to build its LBVH topology once; a compute refit updates bounds every frame; the trace walks these objects after the TLAS (`hit.instance = 0x80000000 | object`, `hit.mesh = 0xFFFFFFFE`). Panel: "Deforming meshes in the BVH (characters, water)".

### Path 2 (in progress): moving rigid objects

Evidence (M1 motion probe, see history "Path 2"): `InstanceParam.prevWorld` (@48) is last frame's world in camera passes (the VS uses it with `prevViewProj_g` for motion vectors). Camera sightings of unmoved objects: 152,343, bit-exact in every family except near-zero rotation elements (~3e-8). Shadow passes leave prevWorld all zero (a few light families fill it). So a camera sighting with prevWorld != world (beyond noise) is moving. Also: before P2a the pool never retired an admitted instance except when its VB/IB was destroyed, so an object that moved and stopped was admitted at every stop (ghosts).

P2a and P2a-rev are described in the history (round P2a and round P2a-rev). Current diagnostics: panel lines "Motion (prevWorld vs world)", "camera: unmoved ... shadow: prevWorld zero ...", "Moving objects: N draw keys moving now (M seen), K meshes flagged, R instances removed, B admissions refused", and a rule line (camera sightings with prevWorld differing, and how many are stale). `world_pool.json` schema 15: `switches.exclude_moving`, `motion` (per-view counters, `rule_stale`, `rule_stale_samples`), `dynamic` (`moving_keys`, `released`; each key: VS, frames, sightings, indirect count, max meters/basis, retired instances, refused admissions, `moving_now`, `released`, mesh, sample world/prevWorld).

Known limit: each draw identity is read every 30 frames (`kPoolRecaptureFrames`), so an object that moves for less than ~0.5 s may never be sampled moving and can still leave a ghost.

Owner checks for P2a (not yet done):
1. Pool Scan on, a few minutes around moving things (doors, carts, NPCs carrying items, swaying props).
2. The rule line must say "none of an unmoved object".
3. Depth Compare: moving objects now "missing" is expected; nothing static should newly go missing (if it does: inspect it, check `dynamic` entries with large `retired_instances`, the mesh-level flag may be too broad for shared assets).
4. `world_pool.json` -> review `dynamic` entries.

P2b (implemented in the working tree, not validated in game), proposed design:
- For flagged meshes, every frame copy the instance slices of their draws (camera and shadow, direct and indirect; slice math from the pool's `ResolvePoolSlice`/`ReservePoolCopy`, b1 base from the CB mirror, indirect counts from the args) GPU-to-GPU into a per-frame arena, plus a small per-draw table (mesh descriptor in the live store, slice offset, count or args offset).
- A compute pass expands them into instance records with the same GPU layout as static instances (`WorldInstanceGPU`: world, inverse, bounds, mesh index, visibility flags), skipping zero-scale/parked matrices like `CheckPoolWorld`; inverse must match `ComputeMatrixInverse` (transcribe + test). Camera-drawn instances get the camera-visible flag; shadow-only ones the shadow-caster flag.
- The trace walks them as a small list next to the TLAS (like the S2 dynamic loop), reusing the static instance/BLAS traversal (one implementation). Requires the mesh to be resident in the live store even with no static instance (check `bvh_live.hpp` mesh streaming selection).
- One frame of latency: the slices are copied at the draw of frame N and expanded at present N, after that present's trace, so the trace of present N+1 and the inline ray-traced passes in frame N+1 read the poses of frame N. There is no staging-ring delay (the static follow path is about 3 to 4 presents behind). Trade-off of the mesh-level policy: a flagged mesh exists in the BVH only where the game draws it that frame (camera or shadow cascades).
- Diagnostics: per-frame dynamic instance count, draws copied, skipped with reasons, arena use, GPU time; inspect lines for dynamic instances.
- Tests: extend the harness like test_deform_live (emulate the expansion + trace vs brute force).

Live store limits (still true): upload + build run per present through the live BVH; BLAS work is capped at 16,384 triangles per present; TLAS rebuild at most every 8 frames (about 8 ms CPU at 16.7k instances on the user's CPU).

### Path 3 (alpha-tested foliage): reference

Scope (owner): rigid alpha-tested geometry (fences, static leaves) and wind foliage as its rigid rest pose. Billboards are refused (`not_rigid`): their orientation depends on the camera. Deforming wind is not routed through `deform_live` (ROADMAP S1c).

Switches (all session only, not saved, no WorldMesh field; panel "Advanced"):
- Main: "Alpha-tested foliage (experimental)" (`alpha_foliage`). OFF: alpha-tested meshes are not traced; the TLAS drops their instances on the present after the switch (the rebuild bypasses the 8-frame wait, whatever else changed); the atlas is destroyed, and the source copies are freed at the same present. The blit does not run with OFF. ON: their cutouts are traced through the atlas. Default ON since 2026-10-09 (was OFF).
- d1: "Indirect foliage source copy (experimental)" (`alpha_indirect_source`), effective only with main ON. OFF: indirect alpha draws get no source copy and no keyless copy. A mesh whose only draws are indirect has no slice: the TLAS counts it as `alpha_waiting` (fail closed). It is neither admitted as opaque nor dropped. Switching d1 OFF (after ON) frees the orphan sources at the next present and drops the slices filled from indirect keys (the mesh waits again). ON: one source copy per texture, attached to the indirect key at resolve; the slice appears after the blit (about 3 presents). The harness test `TestIndirectAlpha` runs (a) and (b) with d1 unset and Test 1 and Test 2 with d1 ON. So "alpha_waiting for indirect foliage" is correct only under d1 OFF. Default ON since 2026-10-09.
- d2: "Admit wind with opaque pixel shader (experimental)" (`alpha_wind_opaque`), effective only with main ON. OFF: a wind vertex shader with an opaque pixel shader is refused at the gate as `wind_opaque` (not copied, not admitted); the next present retires the instances of the wind rest-pose keys (`ApplyPoolWindSwitch`); `AdmitPoolInstance` refuses a wind rest-pose key as `wind_refused_off` while d2 or the main switch is OFF. The key stays in `wind_keys` while OFF (also for a copy recorded while ON and resolved after the switch); it is forgotten when its draw key is invalidated, and at ResetWorldPool. ON: the wind draw is captured as its rest pose (instance world, no sway) and admitted (`kPoolPassWind`, `wind_keys`). Main OFF: wind is `not_rigid` whatever d2 says. Default ON since 2026-10-09.
- Hide: "Hide what the game camera does not show" (`hide_camera_hidden` in `bvh_trace.hpp`). Default ON since 2026-10-09 (was OFF). Shadow-only casters and near-faded surfaces are skipped by camera rays; they stay in the BVH.

Contract and CPU stage: `alphaTestThreshold_g` is `packoffset(c7)` (byte 112 in the reference clutter shader). The offset now comes from the classifier (`alpha_offset`, `PoolDrawGate::alpha_threshold_offset`); a misaligned threshold, one outside the cb or overlapping a used variable (ThresholdOffsetMismatch), and a uvScroll0_g that is missing or not at offset 0 (UvScrollMismatch) are refused. Bindings: `kAlphaTexSlot` t0, `kAlphaMaterialSlot` b5, `kAlphaSwizzleSlot` b10; trait `kTraitAlphaMaterial` on AlphaTested pixel shaders. Pool gate, material per key and per mesh with conflicts (`PoolAlphaState`), UV decode, refusals, `alpha_atlas.hpp`, the blit shader compiled by FXC. Swizzle: only bit 0 of swizzle_flags_g is stored (the trace reads only bit 0). Admitted with the classifier offsets: 0x049B0385 (offset 128), 0x2807FFC9 (120), 0x2DADE2B8 (128) in addition to the 112 ones.

GPU stage:
- UV arena (`bvh_resources.hpp`, `bvh_live.hpp`): `uvs` LiveArena (float2, stride 8), in lockstep with the vertices, written only for meshes that have UVs. `LiveMeshSlot.uv_offset/uv_count`. Overload: the mesh descriptor's `bbox_min.w` holds `uv_offset + 1`, 0 = no UVs. Only the trace reads it, and only for alpha. OFF writes 0 everywhere, so the descriptors and arenas are byte-identical to HEAD when no mesh has UVs.
- Recapture (`bvh_pool.hpp`): with ON, a mesh captured without UVs whose key has a readable material is requeued once (`RequeuePoolMeshKey`, `alpha_uv_requeued`, `alpha_uv_requeues`). The key keeps its material, observations and resource keys.
- Two-stream UVs: TEXCOORD0 in a slot other than 0 is captured as its own stream (`PoolUvStream`: buffer, offset, stride, element offset, format, size). Phases indices, vertices, uvs, then decode. Held vertex bytes are capped (`alpha_held_bytes_max`, 64 MB; a refusal is a mesh failure). A short UV buffer is a failure. PoolBufferKey mixes the UV buffer handle, offset, stride and element offset (only when a stream is used). A queued key seen again with another UV buffer is counted (`uv_stream_mismatch`); the request keeps its first stream.
- Source copies (`CapturePoolAlphaSource`, called from `OnPoolScanDraw`): one copy per source texture, not per draw key. Made on the immediate context after the draw is queued (ReservePoolCopy None and QueuePoolMesh ran), outside the pool lock, so a refused draw takes no copy. Only texture_2d, one layer, non-multisampled. Refcounted by its draw keys (`PoolAlphaSource::keys`, `alpha_key_source`); freed when the last key is invalidated, or when no key has a mesh any more. At most 4 copies a frame, 64 live textures, 256 MB of mip 0 (`kAlphaCopiesPerFrame`, `kAlphaSourcesMax`, `kAlphaSourceBytesMax`). Other sources are refused and counted (`alpha_source_refused` with `_cap`, `_deferred`, `_type`, `_failed`; `source_refused_bytes`, `source_refused_format`). The proxy has the format of the view the game bound; an unknown or typeless view format is refused. No copy while the live BVH is off; copies made before are freed at the present. A copy stays after its blit, so a dropped slice is filled again from it. The texture is registered with its draw keys (`AddPoolResourceKey`). Proxies are freed at the next present (`alpha_dead_proxies`), never inside a destroy event. `PoolAlphaMaterial.view` is the SRV handle bound to t0 (`ps_srv_view`); the texture is resolved with `get_resource_from_view`, and the draw is refused (`alpha_source_refused_view`) unless that resource is the bound t0 resource.
- Atlas (`AlphaGpu`, `alpha_live.hpp`): Texture2DArray<uint> 256 x 256 x 256 (r32_uint, 64 MB) with SRV and UAV; a StructuredBuffer of 256 AlphaMaterialGPU (32 bytes, zero-initialised); a sampler; the blit (`shaders/world_alpha_blit.cs_5_0.hlsl`: t0 source, s0 sampler, u0 atlas, push constant slice). Created on the first blit, destroyed when the toggle goes OFF and at device destroy. `SyncLiveAlpha` runs in UpdateLiveBvh after the mesh sync and before the TLAS: at most 4 blits a present, a UAV-to-SRV barrier, the atlas UAV at u0 replaced by a null UAV after the blit loop, `store_version` bumped. A full atlas or a failed blit keeps the copy (retried at the next present). The blit only goes into a mesh resident in the live store. A live store reset keeps the slices. A released slice is reused after 16 frames (quarantine). Atlas maps for the dump are copied only when `store_version` changes and alpha is on (`PoolAlphaDumpMaps`).
- Trace (`world_bvh_trace.hlsli`, `world_bvh_trace.cs_5_0.hlsl`, `bvh_trace.hpp`): t14 UVs, t15 atlas, t16 materials; `kTraceSrvCount` 17; `TRACE_STATS_COUNT` 21 (alpha_tests 15, alpha_cut 16, P2b 17..20; invalid reads go to invalid_refs); `TRACE_INSPECT_BASE` 21. In `TraceBlas`, a triangle hit in range goes to `AlphaCutHit` with the instance's material (header.z = slice + 1): UV from the barycentrics, tex = (u + scroll.x, 1 - (v + scroll.y)), frac wrap, point sampled at LOD 0 (texel x = floor(frac * 1024), y = floor(frac * 256), word x >> 2, byte x & 3), cut when byte / 255 - threshold < 0. A cut hit is skipped without touching t_max, hidden_t or the shown logic. Material 0, or swizzle bit 0 (alpha forced to 1): solid. An unreadable material or UV: invalid_refs, solid. The default path (OFF) has a different shader binary and SRV table (17 SRVs, t14-t16 null with OFF), so "identical" applies to behaviour only.
- TLAS (`bvh_live.hpp`): an alpha instance gets header.z at the build from `slot_of_uid`. With no slice yet it is left out (`tlas_alpha_waiting`). OFF drops alpha instances. A switch of alpha_foliage is a TLAS change ("alpha switch"): OFF rebuilds at once, ON waits for the interval. `sizeof(WorldInstanceGPU)` stays 192.
- Admission (`AdmitPoolInstance`): with ON, a conflict-free alpha mesh with UVs is admitted. Refused_off, conflict_refused, no_uv and wind_opaque (d2 OFF) remain.
- Memory: PoolPendingCopy 120 B, WorldMesh 288 B (guarded in test_alpha). The vertex-input layout lives with the draw keys (PoolAlphaState, ON only).

Known limits (not changed):
- Identical geometry shares one mesh through the content signature (`mesh_by_signature`). d2 OFF retires that mesh, so a rigid instance sharing it is removed too. By reading the code (not measured) it returns at its next sighting, about 30 frames later; it is not lost. The harness test `TestWindScanPath` uses separate geometry for the rigid draw. Dedup is not changed.
- A mesh shared by an indirect and a direct key loses its slice when d1 goes OFF and is blitted again when ON.
- The wind retire is counted in `alpha_removed` (cosmetic, it is not an alpha removal).
- The wind keys carry no layout in the dump (layouts are kept only for alpha keys).
- Classifier reason (`ClassifyAlphaMaterial`, `AlphaMaterialReasonName`) is computed at pipeline creation for AlphaTested pixel shaders; it is written to `pixel_shaders[]` only for AlphaTested entries (open question in A3c).
- A draw recorded while ON still flags its mesh if the switch flips OFF before its copy resolves (`NotePoolAlphaMaterial` has no toggle check); a draw recorded while OFF never reaches it.

Diagnostics (dump schema 15, unreleased, no bump):
- Panel: "Alpha foliage", "Alpha GPU", "Alpha TLAS", "Indirect/wind" (indirect copies, orphans expired, wind rest draws, wind refused off). The alpha GPU and TLAS lines lag one present.
- `world_pool.json`: `alpha` (`alpha_waiting` in `alpha_gpu`; `draws`, `billboard_draws`, `meshes_conflicted`, `conflict_refused`, `alpha_keys_total`, `alpha_keys` (first 256 by key, each with `key`, `mesh_id`, and the mesh fields below)), `alpha_gpu` (slices_used of 256, blits, blits_frame, proxies, proxy_bytes, copies, source_refused and its four reasons, source_refused_bytes, source_refused_format, uv_requeues, cap_refused, uv_stream_mismatch, held_vertex_bytes, held_vertex_bytes_max, tlas_alpha_instances, alpha_waiting, alpha_tests, alpha_cut, blit_ms, trace_ms), `alpha_indirect_source`, `alpha_wind_opaque`, `wind_rest_draws`, `wind_keys` (with mesh_id).
- `world_pool_meshes.json`, every mesh: `has_source`, `slice` (-1 none), `resident`, `tlas_waiting`, `indirect_sourced`. Alpha meshes also carry `uv_verdict` (not_evaluated, ok, no_texcoord, texcoord_other_slot, texcoord_unsupported_format, texcoord_beyond_stride), `uv` {exists, slot, offset, format}, `vertex_stride`, `layout_element_total`, `layout_elements` (first 16), `vbs_known`, `vertex_buffer_slots` (first 4), `conflict_fields` (1 view, 2 swizzle, 4 threshold, 8 scroll, 16 view of another resource), `conflict_first`, `conflict_incoming`, `alpha_conflict`.
- `world_pool_instances.json`: `alpha`, `wind_rest`, `slice` per instance; alpha and wind rest-pose instances are written first (`PoolDumpOrder`), so they appear before the instance cap. `instances_written` is the number written.
- ReShade.log: "alpha atlas: first slice filled" once; "[world-bvh] alpha diag: keys=N ok=.. no_texcoord=.. other_slot=.. conflict_view=.. conflict_threshold=.." once per dump; "[world-bvh] pixel_unknown: vs=0x.. ps=0x.. reason=.." once per (vs, ps) pair; "trace: ... alpha_tests=... alpha_cut=...".
- Not verified in the harness or in game: the D3D11/ReShade calls at runtime (create_resource, copy_texture_region, views, UAV/SRV bindings, the null UAV at u0, barriers, push constants, the UV copy and staging), the trace and the blit in game, FXC register limits at runtime, GPU time, snapshot and panel copy cost, TSan/ASan on the new lock paths and the Uvs phase.

Verified in the harness (not in game): test_alpha (CPU atlas bookkeeping, classifier and six shaders by offset, gate, admission, requeue, two-stream UV capture, held-byte cap, swizzle and conflicts, the pixel_unknown log, dump fields; mock device: copy after the draw is queued, blit at present, dead source freed, full atlas kept, at most 4 blits a present, OFF creates and blits nothing, 70 keys over 3 textures share 3 copies, the view guard, Cooldown-refused key makes at most one copy, the atlas UAV null at u0 after the blit, end to end through UpdateLiveBvh; wind d2 scan path; indirect d1; refusal reasons summed); test_live (UV arena bytes and descriptors, GPU check with UVs read back, CPU cutout and barycentrics against brute force, blit packing against a double-precision reference (worst 1 LSB), MakeLiveInstanceGPU header.z). Addon build and FXC of world_alpha_blit.cs_5_0.hlsl and world_bvh_trace.cs_5_0.hlsl pass. Earlier "all pass" claims for the 2026-10-08 round were on a stale harness copy; the run after `refresh_world.ps1` is the reference.

Owner test order (in game). Since 2026-10-09 the switches start ON; to get a baseline, untick all four first.
1. Baseline, all four OFF: trace ms and Depth Compare as before; panel "Alpha foliage" unchanged.
2. Main ON only: panel "Alpha GPU" slices rise and "Alpha TLAS" instances rise (waiting falls) for the fence and leaf meshes that were refused as no_uv before. Read Depth Compare and trace ms with OFF, then ON.
3. Main + d1 (indirect): panel "Indirect/wind" (indirect copies, orphans expired); `world_pool_meshes.json` `indirect_sourced`, `tlas_waiting`, `slice`; `alpha_gpu` `source_refused_cap/deferred/type/failed`; ReShade.log "alpha atlas: first slice filled".
4. Main + d1 + d2 (wind): panel "Indirect/wind" (wind rest draws, wind refused off); `world_pool_instances.json` `wind_rest`, `alpha`, `slice`; `wind_keys` and `wind_keys_and_alpha_keys` (the shadow/camera key overlap question); the trace line alpha_tests and alpha_cut.
5. One screenshot of a fence or leaf cutout, ON against OFF.
6. Switch the main toggle OFF: "Alpha TLAS" instances go to 0 on the next present (read the panel one present later).
7. Send back: `world_pool.json` (`alpha`, `alpha_gpu`), `world_pool_meshes.json`, `world_pool_instances.json` as listed above, the ReShade.log lines, the panel lines.

### GPU geometry cache (G2): reference

Scope: validation only, not traced. The static mesh geometry is copied on the GPU at the draw (index, vertex and UV ranges), decoded by a D3D11 pass into a carrier ring, and copied by D3D12 into its own arenas with one BLAS per mesh. Nothing traces it; the CPU pool, the M3 mirror and the DXR scene transaction are unchanged. Switch "GPU geometry cache (experimental)": session only, default OFF, effective only with the DXR switch on. Code: `world/bvh/gpu_capture.hpp` (draw side and the transcriptions the harness tests), `world/rt12/rt12_capture.hpp` (present side), `world/shaders/world_gc_range.cs_5_0.hlsl` and `world_gc_decode.cs_5_0.hlsl` (FXC; no cs_6_5 shader).

Design (short form):
- D1, D2 (draw): `GpuCaptureOnDraw` runs after `LogPoolMeshCopies` in `OnPoolScanDraw` for direct draws whose VB, IB and UV buffers the pool tracks for the key. Off: one relaxed load. Entries are keyed by `mesh_key`. `g_gpu_capture.mutex` and `g_pool.mutex` are never nested, and no graphics call is made under either.
- D3, D5: phases Range (index copy at a draw) -> RangeWait (16-byte min/max result read two presents later) -> Data (vertex, index and UV copies at a later draw of the same key) -> DataWait (decode at that present) -> Done (in the carrier, handed to D3D12). A write (`NotePoolBufferWrite`) resets the entry to Range; the third write refuses it (`rewritten`). A destroyed buffer erases it. Each creation or reset takes a new generation; stale jobs are dropped (`stale_jobs`).
- D7 to D9: staging is one 16 MB raw buffer; copies are 4-byte aligned, starts 16-aligned. The range pass takes InterlockedMax of ~abs and abs, and counts invalid indices. The decode pass has one thread per texel slot; texel n is at (n % 4096, n / 4096); vertices are float4 with w = 1; indices are relative to the min vertex (out of range -> 0); UVs are pairs (u0, v0, u1, v1).
- D10: carrier ring of 3 shared RGBA32_UINT textures, 4096 x 512 each. A decode runs only when the carrier and the GC slot are free (else `carrier_busy` / `carrier_full`); the cache never waits on the CPU per frame.
- D11 to D13: D3D12 uses copies only: one CopyTextureRegion of the carrier rows into a 32 MB linear buffer, then CopyBufferRegion per mesh into arena 0 (vertices, 16 B), 1 (indices, 4 B) and 2 (UVs, 8 B). Arenas grow by copy as in SubmitPresent. Caps: 256 MB geometry (`vram_cap`), 512 MB BLAS (`blas_cap_waits`). When dead bytes exceed half of a used arena above 64 MB, the cache resets (`fragmented`). One BLAS per mesh (PREFER_FAST_TRACE), at most 1,048,576 triangles per present.
- D14, D15: `OnGcPresent` runs after `UpdateLiveBvh` and before `rt12::OnPresent`. The GC submission waits on fence11 (when it placed meshes) and on `last_signal`, then signals and becomes `last_signal`. A failure before ExecuteCommandLists deactivates the cache (`[world-gc] failed at <stage>`; the M5 path continues); a failed signal after execute stops DXR (FailSession).
- D17: "Check GPU geometry" copies up to 64 resident meshes back into a readback buffer, waits at most 2000 ms, and compares bitwise per triangle, in object space, with the CPU pool mesh of the same key (`GcCompareTriangles`). Mismatches are split by VB usage and flags, IB usage and flags, position format and index size. Keys with no CPU mesh are split: `cpu_failed` (a histogram of the pool's failure reason for the key) and `cpu_absent` (no pool mesh and no failure record). The UV compare runs only when the CPU has one UV per position; an index outside the CPU positions is a mismatch.
- P-2 note: `cpu_absent` (keys the GPU holds with no pool mesh and no pool failure record) and the `cpu_dynamic` and `rewritten` counts are new evidence; `cpu_failed` is not. G2 does not resolve the P-2 contradiction.
- G2 note (a): the owner-mismatch compare (`g_rt12_gc->owner != g_rt12.get()`) is a raw-pointer compare with no generation id. This is accepted: `OnGcPresent` runs before `rt12::OnPresent` and `DestroyGc` before `DestroyRt12`, so a null `g_rt12` always precedes a reuse of the address.
- G2 note (b): the owner-mismatch teardown path (`[world-gc] teardown (owner mismatch): leaked`) is untested. It is defensive and not reachable in game; plan section 7 step 6 covers the reachable teardowns.
- Ordering invariant (item 2): the draw copies are issued only on the immediate context, in the order of the draws. A deferred draw serves none (`deferred_skips`). A present's range and decode passes are submitted before the next frame's draw copies on that context, so they read this present's staging.
- staging_used reset (item 8, disclosure): `staging_used` is reset to 0 when the present takes its jobs (OnGcPresent step 6), and in FailGc and TeardownGc. The 16 MB staging is reused by each present; this is safe only because of the ordering invariant above.
- G4 to-do (item 10): `pending` (entries in Range, RangeWait, Data and DataWait, plus `build_queue`) is the settle signal. Before G4 gates on it, confirm it covers every job still in flight, including range results waiting in `readback_jobs`.

Refusals (each counted by reason in `refused`; the CPU pool still captures these meshes): the `ResolvePoolMeshLayout` errors (not indexed, triangle list, stride 0, no input layout, ...), `gpu_written`, `cpu_dynamic`, `format`, `uv_format`, `rewritten`, `range`, `too_large`, `bad_index`, `vram_cap`. Indirect draws are not hooked (no entry): their meshes show as `pool_meshes - resident`. A draw on a deferred context serves no copy (`deferred_skips`). `cap_attempts`, `staging_full_attempts` and `jobs_cap_attempts` count attempts, not meshes (a key refused for capacity is tried again at its next draw and counts again); they are not in `refused`. A Data-phase draw that binds another UV stream buffer than the entry's is skipped and counted (`uv_stream_mismatch`).

Defaults chosen because G1 (the census) was skipped by owner decision; all unverified until the data in `world_gc.json` is read:
| Item | Default | Data |
|---|---|---|
| Duplicates | no GPU dedup; each draw key gets its own mesh | `duplicates` (`dup_ratio`; the 25% rule is decided after G2) |
| VRAM | caps above; D3D11 16 MB staging, 3 x 32 MB carriers; 32 MB linear | `memory`, `largest` |
| Formats | the five float formats of the CPU decoder; R8G8B8A8_UNORM refused (x/255 is not bit-exact on the GPU) | `formats`, `buffers` |
| Verify | UAV and stream-output buffers, and dynamic buffers, refused; a key written 3 times refused | `rewritten_by_buffer`, check `split` |
| Settle | `settled` logged when pending falls to 0 from 32 meshes or more; last 8 kept | `settle` |

Half subnormal (matched, not fixed): `renodx::utils::scene::HalfToFloat` (`src/utils/scene.hpp`) increments the exponent in its subnormal branch where it should decrement it, so 0x0200 decodes as 2^-13 instead of 2^-15. The CPU pool and the GPU decode both use this behaviour and so agree. The fix belongs in `src/utils` as its own change.

Owner test (DevKit build; RTXDI Local on, DXR on, Trace characters on, P2b on, Performance Gather on):
1. Cache OFF: 30 s Performance test at a town street; screenshot; Dump DXR JSON (rename to `world_rt12_off.json`).
2. Tick "GPU geometry cache (experimental)". The log shows `[world-gc] active ...`. Within a few seconds `pending` drops to 0 and `resident` approaches the pool mesh count (minus refused and indirect-only meshes). The log shows `settled`.
3. Press "Check GPU geometry" three times, at an interior, the street and a large outdoor area. Expected: 0 mismatched each time, and a wait under 50 ms. Then "Dump GPU cache JSON".
4. Repeat the step 1 captures with the cache ON: the same images, rays and hits within a few percent (nothing traces the cache). Send both perf files.
5. Change map twice. Each time: `settled`, then a check with 0 mismatched.
6. Toggle the cache OFF, ON, OFF. Toggle DXR OFF with the cache ON (expect `[world-gc] teardown (dxr off)` before `[world-rt12] teardown (switch off)`). Exit with both ON: no crash, no `STUCK`.
7. Send: `world_gc.json` (after steps 3 and 5), both `world_rt12*.json`, the perf files, the screenshots, the `[world-gc]` and `[world-rt12]` log lines, and the three GPU-cache panel lines.

Pass: 0 mismatched at every check; resident covers the pool minus the counted refusals; step 4 images match; no crash. Mismatches are not fixed in G2; they are reported with their split.

Status (first enable, owner run): first enable works. 694 resident; check of 63 meshes with 0 mismatched triangles, 0 refusals, 0 timeouts.
- Known issue, open: after DXR is turned off and on, re-enabling G2 can stall after frame 6 (5 s watchdog, then crash). Repro: DXR off with G2 on (clean), then G2 on again. Logs: 12:31:57 to 12:32:03 in ReShade.log. Not diagnosed.
- The switch stays default OFF until this is fixed or the user decides otherwise.
- Fixes applied: the readback query gate (no unbounded map), and the UAV unbind descriptor count fix (null_views, count 2).

### GPU instance set (G3): reference

Scope: validation only, not traced. While the G2 switch is on, the instance slice of each direct draw of a Done G2 mesh is copied at the draw into a per-present arena (D2). At present step 7b, D3D11 passes build a persistent instance set keyed by (GPU mesh slot, quantized pose): clear, mark, rehash, insert, then a compact pass that writes the set as `WorldInstanceGPU` records. "Compare GPU instances" reads the records back (bounded) and counts matched, GPU-only and CPU-only instances against the CPU pool (D9, D10). Nothing traces the set: the TLAS, DXR and the compute trace never read it. The CPU pool, the live store, the M3 mirror and the DXR scene transaction are unchanged.

Design (short form):
- D1: each G2 entry gets a dense `slot` (< 8192) from a free list. `slot_epoch[slot]` is bumped on write invalidation, destroy, refusal and reset; a destroyed slot returns to the free list. Set entries and draw records carry (slot, epoch); a mismatch drops them on the GPU. The 32 KB epoch table is uploaded when dirty.
- D2: `GpuCaptureOnDraw(..., instances)` copies `count * 160` bytes into the arena for a Done, non-deferred entry within the caps (16384 draws, 16 MB). Other cases are counted: `inst_no_geometry`, `inst_deferred`, `inst_draws_cap`, `inst_arena_full`.
- D3: step 7b (immediate context, after the G2 passes): upload epochs and draws, zero the 16 counters, dispatch clear, mark, rehash (src to dst), insert, then compact; unbind the 5 UAVs; copy the counters to readback slot `present % 3` (an unread slot counts `inst_readback_busy`). No CPU wait; counters are read two presents later by query. The halves swap every present.
- D4: the set is double buffered, 65536 entries per half. `meta` = (tag, last_seen, flags, 0), `entries` = 64-byte `GcSetEntryGPU`. Open addressing, linear probing, 32 probes; tag = pose_hi | 1 (CAS from 0); probe start = pose_lo & (capacity - 1). The rehash drops entries aged more than 600 presents, stale-epoch entries and moving-held entries. No tombstones.
- D5: pose key: 12 world floats, translation rounded to 1/256 m, basis to 1/4096, round half to even; FNV-style words with a leading word (the slot on the GPU). `GcPoseHash` (gpu_capture.hpp) is the C++ reference; `world_gc_insert.cs_5_0.hlsl` must match. Both halves then get the fmix32 finalizer (same five steps in C++ and HLSL), so the probe start and the tag use mixed bits.
- D6: validity: `abs(det) >= 1e-12` and `WorldRowsPlausible` with the P2b limits (legacy scale honoured). Invalid sightings only count.
- D7: moving: a camera sighting with `PoolSightingMoves` (transcribed inline) sets `moving[slot] = frame + 1` in mark mode. A slot with a moving sighting in the last `kPoolMovingHoldFrames` (60) presents keeps all its sightings as per-present records at [65536, +8192) and its set entries are dropped at the rehash.
- D8: records (`WorldInstanceGPU`): header (slot, 0, 0, 1); world rows = the pose; inverse from `WorldAffineInverse`; bounds = the translation point; visibility = (0, 0, flags, 0) with flags camera 2, light 4, wind 8, alpha 16. Set records are at [0, n), moving records at [65536, +n).
- D9, D10: the compare runs on request (step 11): the records and counters are copied to a gpu_to_cpu buffer, an event query ends, and the present polls for at most `kRt12TeardownMs`; a timeout is counted and the buffer is not read. Three non-nested lock phases: slot to key; key to CPU mesh id and alpha conflict, plus a CPU snapshot with `recently_seen`; CPU key to entry reason. The pure `GcCompareInstances` then buckets matched, gpu_only (with alpha and alpha-conflict counts, the P-2 hypothesis), and cpu_only with reason `dynamic`, `no_entry`, `pending`, `done`, `done_unseen` (not seen in 600 frames) or `refused:<reason>`.
- Extra output (beyond the plan's JSON keys): `world_gc_compare_f<present>.json` is written only when a compare is requested, in the same folder as `world_gc.json`.
- D11: the G2 lock rule holds: no graphics call under either mutex, and the two mutexes are never nested.
- D12: OFF path: every new draw-side branch is inside `if (gc_tracked)`. The P2b shader refactor (`world_instance_math.hlsli`, shared by `world_dynamic_expand`) leaves its output unchanged (test_gpu_p2b T2).

Assumed programme decisions (owner has not approved them in writing): 1, 4, 5, 6, 7 and 8 as written; G3 does not act on 7 or 8. Decision 3 is assumed with one extension (the D7 hold): shadow passes leave prevWorld zero, so without the hold a swaying sign's shadow sightings would insert a new key every present. Decision 2 is assumed in part: header.z is 0 in G3 (G4 fills the material).

Deviations (planner decisions, with reasons):
- Direct draws only. `OnPoolScanIndirectDraw` is untouched: G2 has no entry for keys drawn only indirectly, so their CPU instances show as `no_entry` in the compare. A G3b adds indirect draws if that bucket is large. Superseded by G3b (see below).
- No region filter in the compact pass: the compare covers the whole CPU pool. G4 owns the region.
- Record bounds are the translation point (no GPU mesh box yet). The DXR path does not read record bounds; G3 records must never be handed to the compute trace.
- Harness split: pure logic in `test_live.cpp`; the real shaders on WARP in `test_gpu_p2b.cpp`.

### G4-0: compare splits
- Compare splits: no_entry is split into indirect, untracked, cap and other (other is the remainder).
- gpu_no_cpu_mesh and gpu_only are split by state (the CPU observation state); both state maps are written as null on a timeout.
- Note-map cap rule: once the note map holds kGcNoEntryKeys (16384) keys, further untracked keys count as no_entry (other).
- Closed G3 to-dos: the hash-pose check (to-do a) and deferred counting (to-do b), as marked above.
- Pin check: test_live 2760, 3011, 3472 and test_motion 700, 1092 are the same checks as HEAD, with unchanged text.

G4 to-do list: G3b (indirect draws): done; header.z material and record ranges (compacted to [0, out)): done in G4; a region filter in compact and record bounds from a mesh box stay open; the 60-present hold rule versus the CPU repeat/stale rule (`NotePoolMotion`, not transcribed).
- G4 to-do (a): one pose per hash tested; widen the CPU/GPU hash check. Closed in G4-0: test_gpu_p2b P6 checks every live entry's pose hash against its own world.
- G4 to-do (b): deferred instance draws of entries not yet Done are counted only in `deferred_skips`. Closed in G4-0: a deferred draw with instances also counts `inst_deferred`.
- G4 to-do (c): flags are merged across all sightings of an entry. Accepted, no code: the union of all sightings is the compute path's rule (G4 design D12).
- G4 to-do (d): a reused slot inherits the previous mesh's moving mark. Accepted, no code: the inherited mark lasts at most `kPoolMovingHoldFrames`, and its sightings become per-present records that G4 traces (G4 design D12).
- G4 prerequisite: the `no_entry` split (indirect vs direct) must be added before G4 is planned; `gpu_no_cpu_mesh` must be split by pool failure reason before it is called P-2 evidence.

Owner test (DevKit build; RTXDI Local on, DXR on, Trace characters on, P2b on, Performance Gather on). Do not toggle DXR off and on and then the cache on (known G2 stall).
1. Cache off: 30 s Performance test at a town street.
2. Tick "GPU geometry cache (experimental)". Wait for `[world-gc] settled`. The "GPU instances" line shows the set rising, with overflow 0 / 0 / 0.
3. At an interior, the street and a large outdoor area: stand still 10 s and press "Compare GPU instances". Expect a wait under 100 ms, no TIMEOUT, and matched at 95% or more of matched + done; GPU-only small. Then "Dump GPU cache JSON".
4. Near a moving object (swaying sign, door, cart): the line shows moving above 0; standing still, the set stays stable (no growth from ghosts).
5. 30 s Performance test with the cache on at the street (the D3D11 ms on the line and `pool_hooks` show the cost).
6. Change map twice; after `settled` plus 10 s, compare again.
7. Toggle the cache off, on, off; DXR off with the cache on (expect `[world-gc] teardown (dxr off)`); exit with both on. No crash, no STUCK.
8. Send `world_gc.json` (after steps 3 and 6), both performance files, the `[world-gc]` lines (`active`, `compare`, `teardown`) and the GPU-cache panel lines.

Pass: overflow 0, no timeout, matched at least 95% of matched + done at every spot. CPU-only `done` samples go to the planner. Not fixed in G3: CPU-only `no_entry`, `refused` and `pending` (G2 coverage), and GPU-only (CPU admission, the P-2 evidence).

### Indirect draws in the GPU instance set (G3b): reference

Goal: while the GPU geometry cache is active, each gated rigid indirect sub-draw whose args the pool resolve has learned gets a G2 entry (mesh key rebuilt from the learned index count, first index and base vertex) and, once Done, a G3 instance draw. The 20 args bytes are copied GPU-to-GPU into the arena at the draw (32-byte slot right before the window); the insert shader takes the instance count from the copy. The CPU readback only learns the range, never the count. Cache OFF: no new work.

Answers:
- Q1 args: GPU copy at the draw, not a CPU readback. UAV-capable args buffers are not refused; they are counted (`inst_indirect_args_uav`). Deferred lists stay refused (`inst_deferred`). VB/IB written by UAV/SO or CPU-dynamic stay refused by G2.
- Q2 count = min(args instance count, window) on the GPU; start instance ignored (as the CPU); slice base = b1 at the draw. The GPU checks the copied index count, first index and base vertex against the learned range; a different range drops the draw (`indirect_mismatch`).
- Q3 counters: draw side `inst_indirect_draws`, `inst_indirect_unlearned`, `inst_indirect_args_uav`; GPU (insert mode, thread 0) `indirect_empty`, `indirect_mismatch`, `indirect_truncated` = GPU counters 15, 16, 17. Learned sub-draw with untracked VB/IB/UV: note `indirect_untracked`. Compare buckets: the noted reason wins; `no_entry_indirect_untracked` is new; `no_entry_indirect` is the residual.
- Q4 caps: 64 sub-draws per indirect call, 512 instances each. Per present, direct and indirect draws share 16384 draws and the instance arena, now 32 MB (was 16 MB). At a cap the draw copies nothing this present (`inst_draws_cap` / `inst_arena_full`); set overflow stays `set_overflow`.
- Q5 tests: test_live `TestGcInstanceHook` and `TestGcCompareInstances`; test_gpu_p2b P9; test_indirect stage "G3b: an indirect draw with learned args reaches the GPU cache".
- Q6 re-enable stall: G3b changes no create, teardown or present-order code. More draw-time copies happen only while the cache is active. The known G2 re-enable stall is not changed or debugged here; the owner test still avoids DXR off/on followed by the cache on.

Layout: `GcInstanceDrawGPU` and HLSL `GcInstanceDraw` are 48 bytes (`args_float4`, `index_count`, `first_index`, `vertex_offset` added; `kGcNoArgs` = direct). `kGcCounterCount` = 32.

Counter numbering for G4: G3b takes GPU counters 15-17, so G4 must use 18 and 19 (`kGcCounterOut` = 18, `kGcCounterAlphaWaiting` = 19; shader `GC_COUNTER_OUT` 18u, `GC_COUNTER_ALPHA_WAITING` 19u). The G4 plan must use these numbers; its owner test waits for the G3b owner pass.

Owner test (DevKit build; RTXDI Local on, DXR on, Trace characters on, P2b on). Avoid DXR off/on then cache on.
1. Village spot of `world_gc_compare_f295`: cache ON, wait for `[world-gc] settled` plus 10 s, press "Compare GPU instances", then "Dump GPU cache JSON".
2. Same at the large outdoor area.
3. 30 s Performance test with the cache ON at the village (report only).
4. Toggle the cache OFF, ON, OFF; exit with the cache ON. No crash, no STUCK.
5. Send both `world_gc_compare_f*.json`, `world_gc.json`, the GPU-instances and compare panel lines and the `[world-gc]` lines.

Pass (village): no TIMEOUT; `no_entry_indirect` + `no_entry_indirect_untracked` at most 1% of `cpu_instances`; `matched / cpu_instances` at least 0.90 (0.49 before G3b); `match_ratio` at least 0.99; `inst_arena_full` 0 and `inst_draws_cap` 0; overflow 0 / 0 / 0; indirect mismatch at most 1% of indirect draws. Anything else: send the files to the planner.

### GPU scene on DXR (G4): reference

Switch: "GPU geometry cache (experimental)" with the sub-switch "Trace the GPU scene" (`bvh::g_gpu_capture.trace`, default ON, session only). With DXR on, the DXR scene transaction traces a TLAS built from the G3 instance records (set and moving, compacted to `[0, out)`) plus the characters, against the G2 BLAS, instead of the M3 mirror and the M5 P2b records. "Trace meshes the CPU pool lacks" (`trace_no_cpu_mesh`, default ON) withholds the GPU records whose key has no CPU mesh; OFF is the parity run against M5. Switch OFF is the M5 path (the mirror is re-streamed once).

Design (short form of the G4 plan):
- D1 `GcOwnsScene()` = cache enabled AND trace (user intent, not readiness).
- D2 the mirror arenas and BLAS are dropped while tracing (VRAM) and re-streamed when tracing stops (`mirror off (GPU capture traces)` / `mirror on (re-stream)`). `SubmitPresent` reads the vertex and index arenas null-safely; nothing else there changed.
- D3 records: set and moving records compacted to `[0, out)`; moving records are staged at `[capacity + moving_capacity, +moving_capacity)` and have `visibility.w = 1`. Clear zeroes headers of `[0, capacity + moving_capacity)`.
- D4 TLAS bound without readback: `GcRecordsBound` = `largest + max(2048, largest / 4)` over the last 8 counter reads, aligned to 1024, capped at 73728. `bound_misses` counts reads above the bound in force.
- D5 readiness: `ready` when the settle latch is set (set at `pending == 0`, cleared by `ResetGpuCaptureEntries`), at least one counter read exists and both GC arenas exist and a GC table was published (version != 0). Sub-reasons in order: `not_settled`, `no_counters`, `no_geometry`, `no_tables`; the frame then runs the compute pass with reason `gpu_capture`.
- D6 material word per GC slot (`g_gc_materials`, t3): bits 0-23 alpha slice + 1 (0 none); bit 29 `GC_MATERIAL_ALPHA_MESH` (CPU mesh alpha); bit 30 `GC_MATERIAL_WITHHELD`; bit 31 `GC_MATERIAL_NO_CPU_MESH`. Rebuilt when the pool store, the slot keys or a switch changed, and at least every 64 presents. A record is waiting (count 19, invalid) when its CPU mesh is alpha (bit 29; draw flag 16 is ignored, as M5) and has no slice. The record's flag 16 is the mesh alpha bit, as M5.
- D7 `g_rt12_gc_scene` (raw pointers, written in `OnGcPresent` 12b, read by the DXR scene transaction); reset in `TeardownGc`, `FailGc`, the owner mismatch, the failed branch and `TeardownRt12`. `version` comes from `g_rt12_gc_epoch` (never reset), so a lighting slot's `gc_version` never matches a later session; 0 = no table yet.
- D8 the DXR scene transaction with GC mode: no static part and no P2b; descs = characters; the GC records are copied into `s.gc_records` (4096 x 216 texels); per-lighting-slot GC mesh and address tables (`L.gc_meshes`, `L.gc_addresses`) are uploaded when the scene version moved.
- D9 ordering: unchanged from G3 (records written in present N are copied by lighting N+1 before present N+1 rewrites them).
- D10 `Rt12Reason::GpuCapture` ("gpu_capture") appended last; `kRt12ReasonCount` = 17.
- D11 GC mode does not trace the CPU P2b records on DXR; compute still traces P2b.

Counters (`kGcCounterCount` stays 32): 18 `kGcCounterOut` (records written this present), 19 `kGcCounterAlphaWaiting` (alpha records without a slice), 20 `kGcCounterNoCpuMesh` (records written for a slot without a CPU mesh, traced or withheld). Panel: DXR line `GPU scene traced N` (and `(not ready: <sub-reason>)`), GPU instances line `out, bound (misses), alpha waiting, no CPU mesh, scene`, compare line `untraced`. JSON: `gpu_capture` in `rt12_*.json`, `instances.scene` and counters 18-20 in `world_gc.json`, `untraced`, `camera` and the distance maps (`lt16m`, `lt64m`, `lt256m`, `far`) in `world_gc_compare_f*.json`. Per-slot no-CPU-mesh states (`failed:<reason>`, `queued`, `absent`) are in `scene.no_cpu_mesh_slots`.

Q2 (476 `failed:vertex range larger than the mesh staging` records): GPU records of keys the CPU pool failed (a failed key is never requeued). G2 accepts them under the same 4 MB rule, so the GPU geometry is valid by G2's own checks; the CPU failure cause is not known. G4 traces them by default and counts them (counter 20 and the per-state map); the parity run withholds them.
Q3 (re-enable stall): G4 changes no create, teardown or present order of the cache beyond the new objects inside the existing G3 block, and the two new D3D11 buffers are destroyed in the existing lists. The stall mechanism is not diagnosed.

Owner test (DevKit build; RTXDI Local on, Trace characters on, P2b on, Performance Gather on). Never turn DXR off and on and then the cache on. Compute runs last in a session.
1. Start with DXR ON, cache OFF. Wait for `DXR: tracing`. M5 captures: 30 s Performance test, screenshot, Dump DXR JSON.
2. Cache ON (trace ON). Log `mirror off (GPU capture traces)`. Wait for `[world-gc] settled` and `GPU scene traced` rising with no `not ready`. GC captures, then Compare GPU instances and Dump GPU cache JSON.
3. Untick "Trace meshes the CPU pool lacks": GC-withheld captures and Compare again. Tick it again.
4. Near a moving object: occlusion follows with no lag beyond one frame, no ghost.
5. Trace OFF: log `mirror on (re-stream)`; DXR back to M5 after a few frames; trace ON again.
6. Session 1 exits with DXR, cache and trace ON. Session 2: cache OFF, ON, OFF, then DXR OFF and compute captures; exit.

Pass: no crash or STUCK; `gpu_capture` frames only before `settled` and during loads; `bound misses` 0 after settle; GC-withheld rays within 1% and hits within 5% of M5 at both spots; differences of GC versus GC-withheld match `gpu_no_cpu_mesh_camera_by_distance` near buckets; `untraced` and alpha waiting small after settle. Frame time and GPU ms are reported for the planner.

### Reduced CPU pool (G5): reference

Switch: "Reduce the CPU pool (experimental)" (`bvh::g_gpu_capture.reduce_pool`, default ON, session only, not saved). Effective only
while the G2 cache and "Trace the GPU scene" are on (`GcOwnsScene()`), so with either off the code takes the G4 path.

Goal: while the DXR scene transaction traces the GPU scene (G4) and nothing else ray traces, the CPU pool stops its per-draw instance work for direct
draws (slice staging, observation, admission, P2b records, wind CB reads) and the CPU TLAS is not rebuilt or refitted. Mesh capture,
buffer-key tracking, alpha sources, `SyncLiveMeshes`, `SyncLiveAlpha` and the indirect path (except the P2b record) keep running:
the GPU path needs them. Characters on DXR come from deform live, untouched.

Kept while reduced: the gate, alpha read, UV stream, slice resolve (GC instance copy), the pool lock, `NotePoolAlphaMaterial`, the
mesh queue on the schedule cadence (`QueuePoolMesh` without staging), mesh copies, alpha source capture, the GC hooks, the drain,
`SyncLiveMeshes`, `SyncLiveAlpha`, the whole indirect path except P2b.
Removed while reduced: direct-draw slice staging and its resolve, observation and admission, P2b records (direct and indirect), wind
CB reads for direct draws, the prevWorld trace, CPU TLAS rebuilds and refits.

Blockers, first match wins (`full_reason`): `switch_off`, `gc_off`, `dxr_off`, `dxr_failed` (no DXR session, failed or stuck),
`gc_inactive`, `not_ready` (GC scene not settled), `debug_view`, `pool_scan`, `rtxdi_scene`,
`fallback`. Reduced after 120 presents in a row with no blocker (`streak`, `kGcPoolReduceStreak`). Leaving is immediate. Re-entry
needs the streak again.

Fallback (decision 8): a frame whose DXR pass falls back while reduced shows vanilla SSAO (reason `pool_reduced`, counted in
`vanilla_frames`), and the next present turns the full pool back on (`fallback`). No compute trace while reduced.
DXR session failure: the first present that sees no running session (`dxr_failed` or `dxr_off`) leaves at once. Lock order in
`UpdateGcPoolReduce`: `g_gpu_capture.mutex` alone, then `g_pool.mutex` alone (every schedule's `next_frame = frame + 1`, a re-stream
as at a map change), never nested, no graphics call.

While reduced: the gate needs only `bvh_data != nullptr` (`RtBvhReady` is skipped) and the region is `CurrentPoolRegion()`.
With the pool full, behavior is unchanged.

Counters and names: `world_gc.json` `pool_reduce` {switch, state, full_reason, streak, streak_needed, entered, left,
reduced_presents, vanilla_frames, left_by_reason}. The panel shows `CPU pool: reduced ...` or `CPU pool: full (<reason>, streak n of 120)
...`. The reduced-pool reason `pool_reduced` is reported by the GPU cache. Logs `[world-gc] pool reduced (after N presents)` and
`[world-gc] pool full (<reason>): re-stream`.

Re-enable stall (G2): G5 creates, destroys and reorders no graphics object. DXR or the cache off while reduced re-streams in the
same frames as the teardown. Re-test: DXR off while reduced logs `pool full (dxr_off): re-stream` before `teardown (dxr off)`,
no crash; the stall repro gives the same outcome with Reduce OFF and ON.

"Compare GPU instances" is meaningful only with the pool full (the CPU instances are stale while reduced).

Owner test (DevKit build; RTXDI Local on, Trace characters on, P2b on, Performance Gather on, alpha foliage default). Precondition: G4 parity
accepted (rerun B). Two sessions (village spot of the G4 compare; large outdoor area). Each: (1) DXR, cache, trace ON, Reduce
unticked, stand still: Full captures (two 30 s Performance tests, screenshot,
`world_rt12_full.json`, `world_gc_full.json`). (2) Tick Reduce: `pool reduced (after 120 presents)` and `CPU pool: reduced` within
about 2 s; same captures as `_reduced`. (3) Walk 60 s near moving objects, alpha foliage, NPCs: no flicker, `vanilla frames 0`,
`left 0`. (4) Change map while reduced: stays reduced. (5) With RTXDI Local on, `pool full (rtxdi_scene)`; with it off, reduced again after about 2 s.
(6) Cache OFF: `pool full (gc_off)`; cache ON: reduced again. (7) Exit; session 2: DXR OFF while reduced, compute captures.
(8) Last: the G2 stall repro with Reduce ON. (9) Send perf files, all `world_rt12_*`, `world_gc_*`, screenshots and the
`[world-gc]` and `[world-rt12]` log lines, with the CPU pool and DXR panel lines.

Pass: (a) both Reduced runs at both spots have a lower mean frame time than both Full runs, with lower `pool_hooks`,
`pool_drain_excl_capture`, `tlas_rebuild` and `present_bvh_cpu`; (b) scene-transaction parity with Full: rays within 1%, hits within 5%, debug
images match; (c) after `pool reduced`: `vanilla_frames` 0 and `left_by_reason` holds only the owner's toggles; rt12
`fallback_frames` no higher than in the Full run; (d) no crash or STUCK; the stall step behaves as before G5. A smaller or
unrepeatable gain goes to the planner with the perf rows.

### RR M1: GPU completion evidence (historical D3D11 diagnostic)

The former M1 implementation used a persistent D3D11 diagnostic fence and RT12 A/B breadcrumbs. That D3D11 fence is no longer part of RR and has been removed. Current RR has an independent D3D12 runtime timeline: A is the D3D11 input-copy completion, B is D3D12 RR command completion, and C is the D3D11 output-copy completion. These values belong to the RR runtime/bridge generations and must not be read as RT12 queue values. RR Debug Log controls detailed RR event logging; the panel's basic status and last failure codes remain available with logging off. `UINT64_MAX` means completion is unavailable/device loss, never a completed fence value.

## Open decisions

1. Foliage motion (P-1, P-4): option (b), a CPU sway approximation, was chosen on 2026-10-14. It still needs a planner pass and harness tests before code. Options are in HISTORY: Work plan, Step 5.
2. P-2 (missing_random): the cause is not yet identified. Decide how dynamic buffers are captured before a fix is designed (HISTORY: Owner decisions and dump findings).
3. Harness pins that fail at HEAD, owner decision: see `world/docs/HISTORY.md` for the retired harness pins and pin procedures.
   Also allowed: `test_motion.cpp:1091` ("re-flagged within one capture interval"), which fails on clean HEAD (995ce421 and eff22a05), so it predates the removed ray-traced AO, GI, local-shadow and sun-shadow passes; see `world/docs/HISTORY.md`.
4. Classifier reason for `pixel_shaders[]` on non-alpha shaders (HISTORY: Round A3c).
5. Slack TLAS and skip no-op TLAS stay default OFF until the in-game A/B (HISTORY: TLAS rebuild reduction).
