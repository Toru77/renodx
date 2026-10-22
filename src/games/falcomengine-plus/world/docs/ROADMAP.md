# Falcom Engine+ Ray Tracing Roadmap (snapshot 2026-10-22)

Canonical, editable version: the "Falcom Engine+ Ray Tracing Roadmap" doc (https://claude.ai/code/artifact/f99ea9fb-cd29-48ca-a166-a8e25db546c3). This file is a snapshot for future sessions.

This is the only handoff document for `world/`. Read "Open problems", "Current state" and "Reference" before working. Round-by-round history and the evidence behind each decision is in `HISTORY.md`. RTAO owner test scripts are in `RTAO_DIAG_STEPS.md`. Project rules are in `../../AGENTS.md`.

## Open problems (owner reports, 2026-10-14)

Tick a box only after the owner confirms the fix in game. The harness is not enough. Evidence: `renodx-dev\falcomengine-plus\world\<folder>\` (`world_pool.json`, `world_pool_meshes.json`, `world_pool_instances.json`, `world_deform_live.json`).

- [ ] P-1 `foliage_animate`: wind foliage is in the BVH but does not move. Cause: wind is admitted at its rest pose by design (`GatePoolDraw`, `bvh/bvh_pool.hpp`). Fix needs a design decision (Step 5). Status: alpha-tested wind leaves are now swayed too (same CPU sway path); option A done: weight 1/(flexibility+1), cap 10% of the instance extent or 0.75 m; not validated in game.
- [ ] P-2 `missing_random`: a mesh that should be in the BVH is missing. After a rebuild, a different object drops out instead. Cause (logs, confirmed): an alpha-texture conflict retires every instance of a mesh (`ApplyPoolMesh` dedup, `AbsorbPoolAlphaMaterial`, `MarkPoolMeshAlpha`). Fix: Step 2.
- [ ] P-3 `dynamic_object_slow`: a sign moving in the wind updates slowly and leaves red/blue ghosts in the debug view, so RTAO over-darkens. Cause (likely): about 4 presents of pose latency (staging ring, resolve at F+2, BVH update at F+3), plus orphan ghosts kept for 40 frames (static) or 60 frames (dynamic). Fix: Steps 3 and 4.
- [ ] P-4 `tree_leaves_missing`: tree leaves are not in the BVH. Cause (logs): the same alpha conflicts as P-2 (leaf meshes conflicted, `slice: -1`, `tlas_waiting`). The missing pixel is not yet identified. Fix: Step 2, then Step 3 to identify the pixel. Status: alpha-tested wind leaves are now swayed too (see P-1); not validated in game.
- [x] P-5 `random_plane`: a plane/wall in interiors sits at the wrong position and shows red in the debug view. Cause (likely, inspect shows it 1207 frames stale): static instances are never retired on map change (`CompactPool`, mass-retirement branch only counts). Fix: Step 1.

Also found (not owner reports, unchecked):
- [ ] A-1 `world_pool.json` is invalid JSON in most dumps: `bvh_pool.hpp` writes `samples[s].matrix[k]` raw, so NaN prints as `nan`. Use `PoolJsonFloat`.
- [ ] A-2 About 14 meshes fail capture in every log (9 with "vertex range larger than the mesh staging", indices that decode as float positions).
- [ ] A-3 Follow-moving mode (`follow_moving`, commit d79c656e) is not described above. ROADMAP "Path 2" still says moving objects are absent until P2b. Update when Step 4 lands.

## Current state (2026-10-22)

Goal: hardware-free (compute) ray tracing in Trails in the Sky 2nd Chapter (D3D11) on RenoDX. DevKit-gated: `falcom_world::Use`/`AddSettings` do nothing unless `IsSora2nd() && IsDevkitPresent()`. All code is in `src/games/falcomengine-plus/world/`.

Confirmed in game:
- Static world: classifier admission, live BVH, primary-ray trace, Depth Compare, inspect (HISTORY: Round 8 result).
- Characters in the BVH (HISTORY: Round S2). The owner confirmed "Characters capture properly". Water is not stated.
- random_plane fix P-5 (HISTORY: Owner decisions and dump findings).

Implemented, not validated in game. Each feature's owner test list is in its HISTORY section. Reference sections are in this file.

| Feature | Owner test list | Where |
|---|---|---|
| Path 2: moving objects out of the static pool (P2a, P2a-rev) | Pool Scan near moving objects, rule line, Depth Compare | HISTORY: Path 2, round P2a; round P2a-rev |
| Path 2: per-frame path for flagged moving meshes (P2b) | Working tree; test after P-3 | Reference: Path 2; HISTORY: Owner decisions and dump findings |
| Path 3: alpha-tested foliage (default ON since 2026-10-09) | Six-step owner order | Reference: Path 3 |
| Foliage motion: CPU sway approximation (P-1, P-4) | Not planned yet (see Open decisions) | HISTORY: Owner decisions and dump findings |
| Sign latency (P-3): P2b and orphan timing | Follow-lag readout, ghosts | Reference: Path 2; HISTORY: Work plan |
| RTAO round 1 and round 2 (temporal, D1 discovery) | Round 2 steps; D2 to D5 | Reference: RTAO; RTAO_DIAG_STEPS.md |
| RTAO trace resolution (M5) | Trace Resolution Full, Half, Quarter | HISTORY: RTAO trace resolution (M5) |
| RTAO on characters, Character Intensity, Max Darkening | Slider checks | HISTORY: RTAO on characters |
| RTAO Foliage skip, alpha-tested skip (M6a) | Foliage skip On and Off | HISTORY: RTAO Foliage skip; Foliage skip covers alpha-tested meshes |
| Trace characters (RTAO and RTGI) | Four-step list | HISTORY: Trace characters |
| Wall normal decode (M2b) | Magenta patches on -Z walls | HISTORY: Wall normal decode (M2b) |
| Sun and lamps for RTGI: M3 sun, M4 lamps, C2b on-screen bounce, C3 game-style sun | Owner lists per round | HISTORY: Sun and fallback; Off-screen dynamic lights; On-screen sun and lamp bounce; Game-style fallback sun |
| RTGI extra bounces (Feedback, Real Extra Rays, Bounce Saturation, Debug 10) and GI Saturation | Owner lists per round | HISTORY: Extra bounces, Feedback; Real Extra Rays; Bounce Saturation; Bounce debug view; GI Saturation |
| Debug 9 gate, RTGI discovery round | Debug 9 check; discovery list | HISTORY: Debug 9 overlay fix; RTGI discovery round |
| Perf sampling, block timers, Max 30 column, perf file | Performance tab, 30 s test | HISTORY: CPU spike step 1; Performance tab rows; Performance test and separate dumps |
| Panel cleanup | Panel lines moved to JSON | HISTORY: Panel cleanup |
| RT Local Shadows M2 (trace and discovery; image unchanged) | Owner list in HISTORY: RT Local Shadows M2 | Reference: RT Local Shadows |
| RT Local Shadows M3 (applied in the lighting shader, full resolution, raw) | Owner list in HISTORY: RT Local Shadows M3 | Reference: RT Local Shadows |
| TLAS rebuild reduction (skip no-op, slack TLAS, default OFF) | A/B in game | HISTORY: TLAS rebuild reduction (cache-diag-2) |
| Cache diagnostics (Phase 0, 0b) | Dump twice; placement compare | HISTORY: Phase 0 cache diagnostics; Step 0b (cache-diag-1) |
| Refit and drain cleanup; refit diet; rebuild diet; housekeeping | Regression only; no pins changed | HISTORY: Refit and drain cleanup; Rebuild diet; Refit diet; Housekeeping |
| Wind sway range slider | Slider check and dumps | HISTORY: Wind sway range slider |
| Diagnostics round 3b and the M0 to M3 switches | Dump split checks | HISTORY: Round: diagnostics (2026-10-08) |

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
- A render target is bound in shadow passes too: decide camera vs light from the VS (`contract::ClassifyVertexView`), not from bindings.
- Pool copies only on the immediate context (deferred lists may run after their slot is read).
- Mesh capture: two identical captures before admission (`verify_meshes`), GPU-written VB/IB contents can change unseen.
- Never run git inside the device VM mount (left `.git/index.lock` once, deletes are blocked there). Locally this does not apply.
- Files in the addon are CRLF; keep it.
- New shaders in `world/shaders/` are embedded automatically (`__<name>_EMBED_FILE`, `__<name>` span; code guards with `#if defined(...)`).
- Never put `.cso` files under `src/games/falcomengine-plus/` (CMake globs and embeds them).
- FXC was never available in the cloud; shaders were checked with DXC cs_6_0 there. On this PC the addon build runs FXC (`fxc.exe` in the repo `bin/`); a clean addon build is the compile check.

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
- One frame of latency: the slices are copied at the draw of frame N and expanded at present N, after that present's trace, so the trace of present N+1 and RTAO in frame N+1 read the poses of frame N. There is no staging-ring delay (the static follow path is about 3 to 4 presents behind). Trade-off of the mesh-level policy: a flagged mesh exists in the BVH only where the game draws it that frame (camera or shadow cascades).
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

### RTAO (round 1): reference

- Skinned characters in RTAO: dropped by the owner (2026-10-20). Characters as occluders stay out of RTAO. The planned design for the first occlusion effect that needs them (RT shadows) is one per-frame refit of this frame's skinned captures at lighting time, shared by consumers and cached per frame, with RTAO as one consumer switch. Tracked as a future item, not implemented.
- Known cost to check: with RTAO on, character and water capture and the present refit are switched on (`RtaoEnableBvhInputs`), although RTAO does not trace them. Owner to compare the Performance tab with "Deforming meshes" off.
- Code: `world/rtao/` (`rtao_state.hpp` settings, reason codes, `RtaoEffectiveFade`; `rtao_resources.hpp` pipeline, AO target, stats; `rtao.hpp` dispatch and stats readback; `rtao_panel.hpp` panel). Shader: `world/shaders/world_rtao.cs_5_0.hlsl`. Hook wrapper: `RunRtaoInline` in `addon.cpp`, called from `OnBeforeLightingShaderDraw` before `DeployShadows`.
- Placement: inline in the lighting hook (zero latency). Micro shadows read this frame's RTAO AO when it is produced. The shader reads the game scene CBV (b0), not `g_state.camera`.
- Early-out: the sample loop stops once the pixel's camera distance reaches Fade End (`g_rtao_fade.y`). Beyond Fade End the fade is 0 and ao is exactly 1.0, so no ray is traced there.
- The RTAO `rays` and `hits` stats now count only the faded range (pixels beyond Fade End add nothing); the AO output is unchanged.
- Gate: `shader_injection.rtao_active` is latched once per present in `OnPresent`: 0 off, 1 requested but not producing (vanilla SSAO), 2 producing (neutral SSAO, RTAO AO at t22). When RTAO is on, GTVBAO and VBGI do not run, their modes are greyed, and `gtvbao_vbgi_bound` and `gtvbao_vbgi_debug` are 0. Their stored values are untouched.
- Range rule (rt-panel-1): Region is the BVH range R (64-512 m, default 256). Every pooled instance within R of the camera, plus the ray margin `kPoolRegionRayMargin` (10 m), is in the TLAS: the box edge is 3R + 20 m (cell R, camera at least R + 10 m from every face). Only the built BVH range counts (`tlas_region_range`, `tlas_region_size`, `tlas_region_min`). Fade percents (`RtaoFadeStartPct` / `RtaoFadeEndPct`, defaults 15.625 / 31.25, 0-100) are of the slider range: requested = percent * Region / 100; Fade End = min(requested End, built range); Fade Start = min(requested Start, Fade End). Not ready with no built range (`range_too_small`). Stored percents are never rewritten; the panel shows requested and effective metres. A pixel closer than 10 m to any region face is neutral (the shader's `ray_max` margin, `world_rtao.cs_5_0.hlsl`). Coverage tests: `test_live` TestRegionCoverage; fade: `test_rtao` TestFade.
- Panel (rt-panel-1): the "Ray Tracing" tab starts with "What enters the BVH" (Region, alpha foliage, foliage motion = wind sway only (off freezes foliage in its rest pose), deforming meshes, follow, per-frame P2b, keep moving objects out, scan indirect); "BVH Diagnostics" holds Draw Census, the BVH panel (Advanced keeps the diagnostic switches); "Performance" shows CPU and GPU ms over the last 30 presents and the 30 s test (`test_live` TestPerfTab). The perf file (`perf_*.json`) keys for GPU, all per measured frame: `gpu_trace` (BVH debug view trace only, not RTAO), `gpu_alpha_blit`, `gpu_p2b_expand`, `gpu_deform_refit`, `gpu_rtao`, `gpu_rtao_filter`. The Performance GPU column sums these six; "--" when none had a sample.
- Dynamic objects are not traced in round 1: characters and water do not occlude RTAO (dynamic count 0, t10-t13 null). Round 3 adds them.
- Auto-enable: turning RTAO on enables Live BVH and Deforming meshes (`RtaoEnableBvhInputs`); Pool Scan is not enabled (the panel shows `bvh_empty` instead). Turning RTAO off leaves them alone. A persisted ON is applied at the first present.
- Sampling: IS-FAST blue noise (t18, Load only) when the texture is loaded, else IGN (`isfast_unavailable`). Scaled-resolution frames (`resolutionScaling_g != (1,1)`) are neutral and counted by the shader.
- Diagnostics: panel row "RTAO status" (state and reason code, GPU ms, fade, texture bytes, stats after "Read RTAO Stats"), `[world-rtao]` lines in ReShade.log, `world_rtao.json` on each stats read.
- Harness: `test_rtao` (gate table, fade rule, region margin, b12 layout, ray rules against a wall reference, pixel classes, mock device lifecycle). Not covered: GPU execution of the shader, FXC runtime, in-game behaviour.

### RTAO round 2 (temporal) and D1 (Two-Sided discovery): reference

- Pass B (`world/shaders/world_rtao_temporal.cs_5_0.hlsl`) runs after pass A when Temporal is on and the motion and resources are available. Pass A writes the raw AO (r16_float) instead of the texel. Pass B reprojects with `CustomTAA_Reproject` (`taa/taa_common.hlsli`, prevResolutionScale 1), gathers 2x2 bilinear history taps with depth and normal accept flags, renormalises, clamps to the 3x3 raw neighbourhood (History Clamp sigma, 0 = off), blends `ao = lerp(raw, history, History Weight * valid fraction)`, writes the history and the AO texel.
- History: two rgba16f targets used ping-pong (R accumulated AO, G distance from camera, BA octahedral normal), zero-initialised (distance 0 = invalid). Created only while Temporal is on. Raw AO r16_float. Memory is about 18 bytes per pixel (37 MB at 1080p).
- Motion: the G-buffer RTV4 owned SRV only (`MBMotionRtv4Ensure`). TAA t3 is not used (it is bound after lighting and is not shown to be written before the hook). If TAA t3 and RTV4 are different resources in the session, Temporal stops for the session (`no_motion`, round-1 AO).
- Reset rule (`TemporalResetReason`, rtao_state.hpp, priority): off; off to on; AO size change; frame gap (no RTAO output in the previous frame); loading wipe or resize guard; parameter change (ParameterSnapshot); no motion. A reset means one frame of raw AO. Counts per reason are in the panel and json.
- Pass failure: pass B's pipeline, targets and motion are checked before pass A. If any is missing, pass A writes the round-1 texel and the history is not swapped.
- Unbinding: after each pass the compute slots are nulled with push_descriptors (`NullComputeSlots`), so no UAV stays bound at u0..u2 and the raw texture is free as a shader resource (a barrier between the passes).
- Debug modes (RtaoDebug, advanced): 1 raw AO, 2 accumulated AO (white where there is no valid history), 3 history confidence. They apply with Temporal on and are written by pass B. They show full screen while RTAO Debug is set (no GTVBAO Debug View).
- Two-Sided discovery D1 (advanced, diagnostic, no culling; the row stays locked on): CPU histogram of camera-view opaque candidate draws by (cull mode, front CCW), counted only while the discovery setting is on. GPU counters (stats 16..23): index = 16 + back winding * 4 + G-buffer normal toward camera * 2 + alpha-tested * 1, where back winding means the hit facing is positive, toward means dot(N, -view) > 0, alpha means the instance material is non-zero. Names: 16 front_away_opaque, 17 front_away_alpha, 18 front_toward_opaque, 19 front_toward_alpha, 20 back_away_opaque, 21 back_away_alpha, 22 back_toward_opaque, 23 back_toward_alpha. Only hits within max(2%, 5 cm) of the G-buffer distance count.
- Persistence: RtaoDebug and RtaoTwoSidedDiscovery are saved with the other settings (owner decision pending whether they should be session-only).
- Known limits: a game that alternates two motion buffers can look like a conflict (the CONFLICT line prints both handles); the GPU time is one timer around both passes.

#### In-game steps for round 2 (owner)

1. Temporal Accumulation is on by default for new configs. A saved config may keep it off: switch it on by hand in the RTAO section.
2. Off gives the round-1 image. On should show less noise. Check standing still, a moving camera, and a character walking through: look for lag, smear, silhouette flicker and halos at disocclusions. Compare with TAA on and off.
3. Debug: set RTAO Debug (advanced) to 1, 2 or 3 with Temporal on; the mode shows full screen.
4. Motion: check the log for `[world-rtao] motion: ... same resource: RTV4 owned SRV used` (good) or `[world-rtao] motion CONFLICT: ...` (Temporal is off for the session; send both handles).
5. D1: enable Two-Sided discovery (advanced), stand in a street, an indoor scene and a foliage scene, press Read RTAO Stats, and send the CPU lines ("Camera opaque candidate draws: cull N, front CW/CCW: count"), the eight GPU counters, and `world_rtao.json`.
6. CS Dispatch Fix: set "CS Dispatch Fix" to 0, then 1, and confirm the RTAO AO still appears (not black).

What to send back after a session: the RTAO status panel lines, `world_rtao.json`, the `[world-rtao]` log lines, the D1 counters, the GPU ms, and screenshots with Temporal on and off.



### RT Local Shadows: reference

- Code: `world/rtls/` (`rtls_state.hpp` settings and reasons, `rtls_resources.hpp` pipeline and textures, `rtls.hpp` dispatch, stats and JSON, `rtls_panel.hpp` panel). Shader: `world/shaders/world_rtls.cs_5_0.hlsl`. Hook: `RunRtlsInline` in `addon.cpp`, called between `RunRtaoInline` and `RunRtgiInline`.
- Order RTAO -> RTLS -> RTGI. Independent of RTAO: the shared guards are `RtBvhReady` and `RtFrameInputsReady` (addon.cpp), used by RTAO and RTLS.
- Lamp selection: all contributing spots of the pixel's tile cell, the first 8 in list order (`RTLS_MAX_SPOTS`); points strongest first, budget Max Point Lights Per Pixel (default 6), hard cap 10. Lamp index at or above the lamp count is counted as out of range.
- Ratio output: x point shadow and y spot shadow are the weight-averaged share of blocked samples, weighted by each lamp's diffuse weight, times the distance fade z. a = 1 traced, -1 not traced (u2, rgba16f). Lamps over the point budget stay in the denominator and count as unblocked (they keep the game's lighting); M3 scales every lamp of the kind by the same value.
- Stats layout (u1, 96 uints): 0..19 pixel, lamp and ray counters (`kRtlsStat*`); 20..22 reserved for the RTGI temporal pass (M4b); 23..31 reserved (M3b: 24 character traced, 25 character self-blocked); 32..63 per-lamp rays and 64..95 per-lamp near-light blocked, for lamp index < 32.
- Debug views (RTLS Debug, session only): 1 shadow term, 2 lamps per pixel, 3 over budget and traced, 4 blocked near the lamp, 5 pixel classes. RTGI Debug draws over RTLS Debug when both are set.
- `world_rtls.json` is written by Read RTLS Stats (schema 1; stats, per_light, settings, fade and ray max).
- M3: applied in the Sora 2nd lighting shader (t38 = the raw term, full resolution, no denoising). Flags in shader_injection.rtls_flags (kRtlsFlag*): Requested turns the local contact march off (the Contact Shadows Local Lights rows are greyed while RTLS is on); Bound, then Point/Spot (specular material path) and EnvPoint/EnvSpot (diffuse-only path) select the kinds. Each lamp of a traced kind is scaled by v = 1 - min(shadow x Strength, Max Darkening) at its colour; the game's spot shadow maps are skipped where the coverage is 1 and blended by lerp(pcf, 1, coverage) in the fade band. Character pixels are not touched (M3b). Off: rtls_flags 0, identical to HEAD.

## Open decisions

1. Foliage motion (P-1, P-4): option (b), a CPU sway approximation, was chosen on 2026-10-14. It still needs a planner pass and harness tests before code. Options are in HISTORY: Work plan, Step 5.
2. P-2 (missing_random): the cause is not yet identified. Decide how dynamic buffers are captured before a fix is designed (HISTORY: Owner decisions and dump findings).
3. Harness pins that fail at HEAD, owner decision: `test_rtao.cpp:1881` pass A FNV pin (0xbcf4d7beeb61f2b6), and `test_live` goldens 2758, 3009 and 3470. The foliage-skip pin procedure is in HISTORY: RTAO Foliage skip. When the pass A pin is re-recorded, world_rtao.cs_5_0.hlsl's IGN line moves to TraceIgnNoise2 (world_bvh_trace.hlsli).
   Also allowed: `test_motion.cpp:1091` ("re-flagged within one capture interval"), which fails on clean HEAD (995ce421 and eff22a05), so it predates RTLS.
4. Whether `RtaoDebug` and `RtaoTwoSidedDiscovery` stay saved with settings or become session-only (Reference: RTAO round 2).
5. Classifier reason for `pixel_shaders[]` on non-alpha shaders (HISTORY: Round A3c).
6. Slack TLAS and skip no-op TLAS stay default OFF until the in-game A/B (HISTORY: TLAS rebuild reduction).
