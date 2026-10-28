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

R1 (zero-cost BVH gating), implemented, not validated in game: with RTAO, RTGI and RTLS all off, the BVH, pool, deform and census path does not run (`RtBvhDemanded`, `ApplyBvhInputs` in `world/bvh/bvh_debug.hpp`). Owner test list:
1. RTAO, RTGI and RTLS off, Pool Scan unticked, no debug view: the BVH panel shows "BVH inputs: idle (nothing runs)"; the Pool Scan, Live BVH and Deforming meshes rows are unticked, and the Performance tab (Gather on) shows about zero for the pool and deform CPU rows.
2. Turn RTAO on: the panel shows "forced on by RT effects" and RTAO works as before. Repeat with RTGI and with RT Local Shadows.
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
| RT Local Shadows M4b (denoise and trace resolution) | Owner list in HISTORY: RT Local Shadows M4b | Reference: RT Local Shadows |
| RT Local Shadows M5 item 2 (penumbra edge stop in the spatial filter) | Owner list in HISTORY: RT Local Shadows M5 item 2 | Reference: RT Local Shadows |
| RT Local Shadows M5 item 3 (RTGI off-screen lamp shadows) | Owner list in HISTORY: RT Local Shadows M5 item 3 | Reference: RT Local Shadows |
| TLAS rebuild reduction (skip no-op, slack TLAS, default OFF) | A/B in game | HISTORY: TLAS rebuild reduction (cache-diag-2) |
| Cache diagnostics (Phase 0, 0b) | Dump twice; placement compare | HISTORY: Phase 0 cache diagnostics; Step 0b (cache-diag-1) |
| Refit and drain cleanup; refit diet; rebuild diet; housekeeping | Regression only; no pins changed | HISTORY: Refit and drain cleanup; Rebuild diet; Refit diet; Housekeeping |
| Wind sway range slider | Slider check and dumps | HISTORY: Wind sway range slider |
| Diagnostics round 3b and the M0 to M3 switches | Dump split checks | HISTORY: Round: diagnostics (2026-10-08) |
| DX12 probe (M2a, diagnostic) | Run DX12 probe; send dx12_probe_f<frame>.json from renodx-dev\falcomengine-plus\world | world/rt12/rt12_probe.hpp |
| DX12 probe M2b (DXR round trip, transfer paths) | Run DX12 probe, send dx12_probe_f<frame>.json | world/rt12/rt12_probe.hpp, world/shaders/world_rt12_probe.cs_6_5.hlsl |
| DXR backend M3 (RTAO pass A on D3D12, scene mirror, alpha mirror) | Owner list in Reference: DXR backend (M3) | world/rt12/rt12_backend.hpp, rt12_mirror.hpp, world/shaders/world_rt12_*.hlsl |
| DXR backend M4 (characters on DXR: carrier, character BLAS, mask 8) | Owner test in Reference: DXR backend (M4) | world/rt12/rt12_backend.hpp, world/shaders/world_rt12_carrier.cs_5_0.hlsl, world_rt12_deform_unpack.cs_6_5.hlsl |
| DXR backend M5 (P2b moving objects on DXR: records carrier, records pass, instance table at S + r) | Owner test in Reference: DXR backend (M5) | world/rt12/rt12_backend.hpp, world/rt12/rt12_mirror.hpp, world/shaders/world_rt12_records_carrier.cs_5_0.hlsl, world_rt12_records.cs_6_5.hlsl |
| GPU geometry cache G2 (validation only, not traced) | Owner test in Reference: GPU geometry cache (G2) | world/bvh/gpu_capture.hpp, world/rt12/rt12_capture.hpp, world/shaders/world_gc_range.cs_5_0.hlsl, world_gc_decode.cs_5_0.hlsl |
| GPU instance set G3 (validation only, not traced) | Owner test in Reference: GPU instance set (G3) | world/bvh/gpu_capture.hpp, world/rt12/rt12_capture.hpp, world/shaders/world_gc_instance.hlsli, world_gc_insert.cs_5_0.hlsl, world_gc_compact.cs_5_0.hlsl, world_instance_math.hlsli |
| GPU instance set G3b (indirect draws; validation only, not traced) | Owner test in Reference: indirect draws in the GPU instance set (G3b) | world/bvh/bvh_pool.hpp, world/bvh/gpu_capture.hpp, world/shaders/world_gc_insert.cs_5_0.hlsl |

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
- `constant_buffer` descriptors in `update_descriptor_tables` are `api::buffer_range {buffer, offset, size}`, not a `resource_view`. Passing a view makes ReShade read the next struct members as offset and size; a window that passes D3D11.1 alignment points at the wrong constants (RT sun Full at 1280x720 and 2560x1440 depth). Use `SceneCbvRange` (`rtao_resources.hpp`). The legacy `addon.cpp` sites (15, `captured_scene_cbv_view`) use the same pattern and are deferred.
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
- Known cost to check: with RTAO on, character and water capture and the present refit are switched on (`ApplyBvhInputs`, which forces them on while any RT effect is on), although RTAO does not trace them. Owner to compare the Performance tab with "Deforming meshes" off.
- Code: `world/rtao/` (`rtao_state.hpp` settings, reason codes, `RtaoEffectiveFade`; `rtao_resources.hpp` pipeline, AO target, stats; `rtao.hpp` dispatch and stats readback; `rtao_panel.hpp` panel). Shader: `world/shaders/world_rtao.cs_5_0.hlsl`. Hook wrapper: `RunRtaoInline` in `addon.cpp`, called from `OnBeforeLightingShaderDraw` before `DeployShadows`.
- Placement: inline in the lighting hook (zero latency). Micro shadows read this frame's RTAO AO when it is produced. The shader reads the game scene CBV (b0), not `g_state.camera`.
- Early-out: the sample loop stops once the pixel's camera distance reaches Fade End (`g_rtao_fade.y`). Beyond Fade End the fade is 0 and ao is exactly 1.0, so no ray is traced there.
- The RTAO `rays` and `hits` stats now count only the faded range (pixels beyond Fade End add nothing); the AO output is unchanged.
- Gate: `shader_injection.rtao_active` is latched once per present in `OnPresent`: 0 off, 1 requested but not producing (vanilla SSAO), 2 producing (neutral SSAO, RTAO AO at t22). When RTAO is on, GTVBAO and VBGI do not run, their modes are greyed, and `gtvbao_vbgi_bound` and `gtvbao_vbgi_debug` are 0. Their stored values are untouched.
- Range rule (rt-panel-1): Region is the BVH range R (64-512 m, default 256). Every pooled instance within R of the camera, plus the ray margin `kPoolRegionRayMargin` (10 m), is in the TLAS: the box edge is 3R + 20 m (cell R, camera at least R + 10 m from every face). Only the built BVH range counts (`tlas_region_range`, `tlas_region_size`, `tlas_region_min`). Fade percents (`RtaoFadeStartPct` / `RtaoFadeEndPct`, defaults 15.625 / 31.25, 0-100) are of the slider range: requested = percent * Region / 100; Fade End = min(requested End, built range); Fade Start = min(requested Start, Fade End). Not ready with no built range (`range_too_small`). Stored percents are never rewritten; the panel shows requested and effective metres. A pixel closer than 10 m to any region face is neutral (the shader's `ray_max` margin, `world_rtao.cs_5_0.hlsl`). Coverage tests: `test_live` TestRegionCoverage; fade: `test_rtao` TestFade.
- Panel (rt-panel-1): the "Ray Tracing" tab starts with "What enters the BVH" (Region, alpha foliage, foliage motion = wind sway only (off freezes foliage in its rest pose), deforming meshes, follow, per-frame P2b, keep moving objects out, scan indirect); "BVH Diagnostics" holds Draw Census, the BVH panel (Advanced keeps the diagnostic switches); "Performance" shows CPU and GPU ms over the last 30 presents and the 30 s test (`test_live` TestPerfTab). The perf file (`perf_*.json`) keys for GPU, all per measured frame: `gpu_trace` (BVH debug view trace only, not RTAO), `gpu_alpha_blit`, `gpu_p2b_expand`, `gpu_deform_refit`, `gpu_rtao`, `gpu_rtao_filter`, `gpu_rtgi`, `gpu_rtgi_denoise`, `gpu_rtls` (RTLS trace), `gpu_rtls_denoise` (RTLS temporal, filter and upsample). The Performance GPU column sums these ten (every kPerfGpu row); "--" when none had a sample. Command-recording CPU rows: `rtao_record_cpu`, `rtgi_record_cpu`, `rtls_record_cpu`.
- Dynamic objects are not traced in round 1: characters and water do not occlude RTAO (dynamic count 0, t10-t13 null). Round 3 adds them.
- Auto-enable: while RTAO, RTGI or RT Local Shadows is on, `ApplyBvhInputs` (`bvh/bvh_debug.hpp`, at startup and every present) forces Pool Scan, Live BVH and Deforming meshes on. With all three off they follow the Pool Scan tick or a BVH debug view and are otherwise off, so turning the last RT effect off switches them off again. A persisted ON is applied by the first present at the latest.
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
- M3: applied in the Sora 2nd lighting shader (t38 = the raw term, full resolution, no denoising). Flags in shader_injection.rtls_flags (kRtlsFlag*): Requested turns the local contact march off (the Contact Shadows Local Lights rows are greyed while RTLS is on); Bound, then Point/Spot (specular material path) and EnvPoint/EnvSpot (diffuse-only path) select the kinds. Each lamp of a traced kind is scaled by v = 1 - min(shadow x Strength, Max Darkening) at its colour; the game's spot shadow maps are skipped where the coverage is 1 and blended by lerp(pcf, 1, coverage) in the fade band. Character pixels get the M3b lamp light when Receive on characters is on. Off: rtls_flags 0, identical to HEAD.
- M4b: Trace Resolution (Full, Half, Quarter; default Half) and the shared RTGI denoise chain (RtgiDenoiseSet; EnsureRtgiDenoise, DispatchRtgiDenoise and DestroyRtgiDenoiseSet in world/rtgi, used by RTGI and RTLS): temporal (rtao::TemporalResetReason over rtls::ParameterSnapshot; Strength, Max Darkening and the filter are not in it), spatial filter, joint-bilateral upsample to the full-size r8g8b8a8_unorm output that the lighting shader reads at t38. Motion: UpdateRtMotion (addon.cpp; RTAO, RTLS, RTGI). RTLS Debug 6 accumulated, 7 history confidence, 8 final. Defaults: Temporal on, History Weight 0.90, Depth Rejection 0.05, Normal Rejection 0.90, History Clamp 1; Spatial on, a-trous, radius 2, 1 pass, 5 taps.
- M3b: Receive on characters (default Off) adds the game's point/spot lamp light (charaColor, cell lists, the same falloff and cone) times RTLS visibility and coverage to character pixels (after charColor, before RTAO); the trace traces character pixels with charaColor weights and Character Normal Bias (c6.y). Stats 24 character traced, 25 character self-blocked. Off: identical to HEAD.
- M5 item 2: the RTLS spatial filter has a third edge stop on the shadow value: weight x exp(-k x max(|point - centre|, |spot - centre|)), k = Penumbra Edge Stop (RtlsFilterValueStop, default 4, 0 = off, saved, not in the history snapshot). It is built into world_rtls_filter.cs_5_0.hlsl (the shared RTGI filter source with WORLD_FILTER_VALUE_STOP defined; push c2.w); RTGI compiles the same source without the define and pushes 0, so its bytecode and the dispatch golden are unchanged.

### DXR backend (M3): reference

Switch "DXR backend (experimental)" (GPU BVH (live) section of the panel): session only, default OFF, not saved. OFF is the compute pass A exactly as before (no rt12 work, hooks inactive). Design: `world/rt12/rt12_backend.hpp` (D3D12 side, pass A, panel, JSON), `world/rt12/rt12_mirror.hpp` (std-only queue and pure helpers, tested by the harness), shaders `world_rt12_pack`, `world_rt12_noise`, `world_rt12_resolve` (FXC) and `world_rt12_rtao` (DXC, `TracePixel` of `world_rtao.cs_5_0.hlsl` with `TraceWorldClosestDxr`).

Architecture, per frame:
1. Present (`rt12::OnPresent`, after `UpdateLiveBvh`): switch, stuck check, retired releases, timestamps, drain of the mirror (fed by hooks in `bvh_live.hpp`, `bvh_resources.hpp`, `alpha_live.hpp`), present submission (arenas at the D3D11 element offsets, one BLAS per new live slot, mesh table, materials), D3D11 copy of the alpha atlas when it changed.
2. Lighting (`rtao::Dispatch` pass A, `rt12::RunRtaoPassA`): D3D11 pack, signal A, lighting submission (TLAS rebuilt when the static key (`tlas_cpu_version`, `blas_generation`) moved, while characters exist, and once after they go; then the DXR trace), signal B, D3D11 waits B, D3D11 resolve into pass A's own targets. Any failed precondition runs the compute pass A that frame and counts the reason.

Decisions: sync through `renodx::utils::gpu_fence` only (no CPU wait per frame). Every D3D12 submission first waits on the previous one (`QueueWait` on `last_signal`), so buffers are reused in place after their slot's fence completed. A busy slot means a compute fallback (lighting) or a deferral with order kept (present). Stuck = no fence progress for 2000 ms with work pending: DXR disabled for the session, objects leaked on purpose. Teardown waits at most 2000 ms, then leaks. Instances are `data->tlas_instances_cpu` minus holes, instances without a BLAS and bad mesh ids; `InstanceID` is the index into the full array. Masks: 1 camera-visible plain, 2 foliage or alpha-tested, 4 shadow-only. `TRIANGLE_CULL_DISABLE` always; `FORCE_NON_OPAQUE` for alpha materials. Invalid triangles are degenerate `0,0,0` in the D3D12 index copy only. Alpha: the atlas is copied into a shared mirror, the materials are mirrored on CPU writes (kept whether or not DXR is on). Near fade is not applied on DXR (RTAO's view uses floor 1.0). Characters are traced on DXR since M4 (see the M4 section); P2b records are not (M5). Inputs and outputs cross as textures only (buffer sharing does not work).

Fallback reasons (`Rt12ReasonName`, panel `fallback_frames`): off, not_created, create_failed, no_dxr, fence_error, stuck, discovery, busy, context_mismatch, tlas_empty, alpha_mirror, target_failed, submit_failed, characters, tracing (a traced frame).

Panel lines (GPU BVH (live) section): `DXR: <reason>  traced <n>  busy <n>  wrapped <yes|no>` (+ `STUCK (objects leaked)`, + `failed at <stage> hr=`); `DXR scene: <meshes> meshes, <BLAS> BLAS (<MB>), TLAS <n> (no BLAS, holes, bad mesh), sanitized triangles, alpha mirror`; `DXR GPU ms (D3D12 timestamps, avg): BLAS, TLAS, trace` (`--` without a sample); line 2 adds `characters <n> (<BLAS> BLAS, <MB>, builds, refits)` and the GPU ms line adds `characters <ms>`; the P2b note appears when the last traced frame had P2b records.

`world_rt12.json` (Dump DXR JSON, schema 1): switch_on, reason, failed_stage, hr, wrapped, stuck, leaked, alpha_mirror, traced_frames, fallback_frames, busy_frames, present_submits, present_skipped_busy, lighting_submits, blas_count, blas_bytes, mesh_count, arena_bytes, tlas_instances, tlas_no_blas, tlas_holes, tlas_bad_mesh, tlas_bytes, sanitized_triangles, atlas_copies, noise_copies, gpu_ms (blas, tlas, trace, dynamic {last, avg}, avg), dynamic_objects, dynamic_vertices, dynamic_builds, dynamic_refits, dynamic_blas_count, dynamic_blas_bytes, p2b_records_not_traced (schema 1 is kept: M4 added keys and removed characters_not_traced).

Owner test (DevKit build; RTAO on, Trace characters OFF, P2b OFF, Two-Sided discovery OFF): (1) baseline with DXR OFF: 30 s Performance test at a fixed spot, screenshot, Read RTAO Stats. (2) Tick DXR: a few frames without RT, panel `DXR: tracing`, log `[world-rt12] created (wrapped yes, ...)`. (3) Same spot: screenshots and RTAO Debug 1 ON and OFF, Read RTAO Stats, 30 s test: same AO (edge differences allowed), rays and hits within a few percent. (4) Alpha-tested foliage: screenshots ON and OFF, panel `alpha mirror yes`. (5) Two map transitions: `no BLAS` back to 0, busy small, no STUCK. (6) Switch OFF, ON, OFF quickly: no crash, `[world-rt12] teardown (switch off)`. (7) Exit with the switch ON: no crash. (8) Send `world_rt12.json` (after steps 3 and 5), both `world_rtao.json`, both perf files, the screenshots, the `[world-rt12]` log lines and the four panel lines.

### DXR backend (M4): characters on DXR

Scope: with the DXR switch on and RTAO Trace characters on, the DXR pass A traces the characters `RefitDeformLiveCharacters` traced for the lighting frame (`bvh::GetDeformLiveCharacters`: the same set and the same arena as the compute pass, no second selection rule). Moving objects (P2b) stay compute-only until M5.

Design: (1) carrier pass (D3D11, `world_rt12_carrier.cs_5_0.hlsl`): arena vertices `[0, vertex_end)` into a shared RGBA32_UINT carrier of 4096 x 512 (vertex v at texel (v % 4096, v / 4096)); (2) unpack (D3D12, `world_rt12_deform_unpack.cs_6_5.hlsl`) into a float4 vertex buffer at the same indices; a non-finite vertex gets a NaN x, so its triangles are inactive. (3) One BLAS per deform identity key: a full build (`ALLOW_UPDATE | PREFER_FAST_TRACE`) when the key is new, its triangle count changed, or 30 frames passed (`kRt12DynamicRebuildFrames`); otherwise an in-place refit (`PERFORM_UPDATE`). Entries unseen for 600 frames (`bvh::kDynamicRetireFrames`) are retired. (4) The TLAS gets one identity instance per character: `InstanceID 0xFFFFFF` (`kRt12DynamicInstanceId`), mask 8 (`kRt12MaskDynamic`), opaque, cull disabled. The TLAS is rebuilt every frame while characters exist and once when they disappear; each slot's instance table is uploaded only when the static part changed. (5) The RTAO DXR shader adds mask 8 only when its character count is non-zero; a character hit reports `DYNAMIC_INSTANCE_FLAG` and `DYNAMIC_MESH_MARKER`, as the compute path does for dynamic objects.

Fallback reason `characters` (new): the character set or the arena could not be read this frame. Lighting timestamps: slot 2 start, 3 after the character BLAS, 4 after the TLAS, 5 after the trace (`dynamic_ms`, `tlas_ms`, `trace_ms`).

Owner test (DevKit build; RTAO on, Trace characters ON, P2b OFF, Two-Sided discovery OFF, Performance Gather on):
1. A spot with several NPCs and the player near a wall. DXR OFF: 30 s Performance test, Read RTAO Stats (rename `world_rtao.json` to `world_rtao_off.json`), screenshot, RTAO Debug 1 screenshot.
2. DXR ON, wait for `DXR: tracing`. Panel line 2 shows `characters N`, with N equal to the character object count in the RTAO panel. Repeat the captures (`world_rtao_on.json`), then Dump DXR JSON.
3. Walk the player and watch walking NPCs with RTAO Debug 1: the contact darkening under and around characters follows them without lag or ghosts, as with DXR OFF.
4. Change map twice: `characters` keeps matching; no fallback reason other than `off` in `world_rt12.json` (a few `tlas_empty` or `characters` during loads are acceptable).
5. Toggle the switch OFF, ON, OFF; exit with it ON: no crash.
6. Send both perf files, both `world_rtao*.json`, `world_rt12.json`, the screenshots, the `[world-rt12]` log lines and the four DXR panel lines.

### DXR backend (M5): P2b records on DXR

Scope: with the DXR switch on, the DXR pass A traces the P2b records the compute pass A traces (`t19`, `dynamic.instances`, the records of this present), so DXR RTAO covers the static TLAS, alpha foliage, characters and P2b. R = the record count (`push.fade[3]`, the same count the compute pass uses); R = 0 is the M4 path, unchanged.

Design: (1) records carrier (D3D11, `world_rt12_records_carrier.cs_5_0.hlsl`): record r, float4 j (0..11) at texel 12 r + j of a shared RGBA32_UINT texture of 4096 x 96 (`kRt12RecordCarrierHeight`). (2) records pass (D3D12, `world_rt12_records.cs_6_5.hlsl`, after the M4 character block): one `D3D12_RAYTRACING_INSTANCE_DESC` per record into the slot's descs buffer after the static and character descs, and the record itself into the instance table at `S + r` (`S` = `tlas_instances_cpu.size()`), so `InstanceID = S + r` resolves in the trace like a static instance. The packing is `rt12::Rt12RecordDescWords` (`rt12_mirror.hpp`), tested against `MakeRt12InstanceFields` in the harness (`TestRt12Records`). (3) Mask as the static path (camera visible 1, foliage or alpha-tested 2, else 4). An invalid record (`header.w < 0.5`), a slot out of range or a slot without a D3D12 BLAS gets mask 0 and BLAS address (0, 0), so it is never hit. The address table (one uint64 per live store slot, `Rt12Slot::addresses`) is rewritten when `blas_generation` moves. (4) The TLAS is built from the slot's descs buffer with static + characters + R instances; it is rebuilt every frame while records exist and once after they disappear.

Counted fallback `records` (new): R above `kPoolDynamicInstances`, or the expansion not ready. Panel line 2 gets `, P2b records N`; line 3 gets `P2b <ms>` (the records pass, timestamp 4 minus 3). Lighting timestamps are now 2 start, 3 after the character BLAS, 4 after the records, 5 after the TLAS, 6 after the trace (`dynamic_ms`, `records_ms`, `tlas_ms`, `trace_ms`). `world_rt12.json`: the `records` key replaces `p2b_records_not_traced`, and `gpu_ms.records` has `last` and `avg`.

Owner test (DevKit build; RTAO on, Trace characters ON, P2b ON, Two-Sided discovery OFF, Performance Gather on):
1. Near moving objects (a swaying sign, carts, doors). DXR OFF: 30 s Performance test, Read RTAO Stats (rename to `world_rtao_off.json`), screenshot, RTAO Debug 1 screenshot.
2. DXR ON, wait for `DXR: tracing`. Panel line 2 shows `P2b records N`, with N equal to the P2b record count (`dynamic_instance_count` in `world_pool.json`). Repeat the captures (`world_rtao_on.json`), then Dump DXR JSON.
3. Watch a moving object with RTAO Debug 1: the occlusion follows it the same way with DXR ON and OFF.
4. A large outdoor area: repeat steps 1 and 2.
5. Change map twice; toggle the switch OFF, ON, OFF; exit with it ON: no crash, no `stuck`.
6. Send: perf files, `world_rtao_*.json`, `world_rt12.json`, screenshots, `[world-rt12]` log lines, the panel lines.

Decision (recommendation only, not implemented in M5): keep the compute pass A as the fallback for every counted reason and as the A/B reference. After the A/B confirms parity (rays and hits within a few percent, images match) in an interior and a large outdoor area, make the DXR backend the default for RTAO on DXR 1.1 devices (a saved setting, still with the counted fallback) in its own milestone. Retire the compute traversal and the CPU LBVH only when every RT consumer (RTAO, RTGI, RTLS, RT sun, debug views) traces on DXR, in one cleanup milestone.

### GPU geometry cache (G2): reference

Scope: validation only, not traced. The static mesh geometry is copied on the GPU at the draw (index, vertex and UV ranges), decoded by a D3D11 pass into a carrier ring, and copied by D3D12 into its own arenas with one BLAS per mesh. Nothing traces it; the CPU pool, the M3 mirror, M5 and the compute pass A are unchanged. Switch "GPU geometry cache (experimental)": session only, default OFF, effective only with the DXR switch on. Code: `world/bvh/gpu_capture.hpp` (draw side and the transcriptions the harness tests), `world/rt12/rt12_capture.hpp` (present side), `world/shaders/world_gc_range.cs_5_0.hlsl` and `world_gc_decode.cs_5_0.hlsl` (FXC; no cs_6_5 shader).

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

Owner test (DevKit build; RTAO on, DXR on, Trace characters on, P2b on, Performance Gather on):
1. Cache OFF: 30 s Performance test at a town street; screenshot; RTAO Debug 1 screenshot; Dump DXR JSON (rename to `world_rt12_off.json`).
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

Scope: validation only, not traced. While the G2 switch is on, the instance slice of each direct draw of a Done G2 mesh is copied at the draw into a per-present arena (D2). At present step 7b, D3D11 passes build a persistent instance set keyed by (GPU mesh slot, quantized pose): clear, mark, rehash, insert, then a compact pass that writes the set as `WorldInstanceGPU` records. "Compare GPU instances" reads the records back (bounded) and counts matched, GPU-only and CPU-only instances against the CPU pool (D9, D10). Nothing traces the set: the TLAS, DXR and the compute trace never read it. The CPU pool, the live store, the M3 mirror, M5 and compute pass A are unchanged.

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

G4 to-do list: G3b (indirect draws): done; a region filter in compact; record bounds from a mesh box; header.z material; record ranges are [0, set) and [65536, +moving); the 60-present hold rule versus the CPU repeat/stale rule (`NotePoolMotion`, not transcribed).
- G4 to-do (a): one pose per hash tested; widen the CPU/GPU hash check. Closed in G4-0: test_gpu_p2b P6 checks every live entry's pose hash against its own world.
- G4 to-do (b): deferred instance draws of entries not yet Done are counted only in `deferred_skips`. Closed in G4-0: a deferred draw with instances also counts `inst_deferred`.
- G4 to-do (c): flags are merged across all sightings of an entry. Accepted, no code: the union of all sightings is the compute path's rule (G4 design D12).
- G4 to-do (d): a reused slot inherits the previous mesh's moving mark. Accepted, no code: the inherited mark lasts at most `kPoolMovingHoldFrames`, and its sightings become per-present records that G4 traces (G4 design D12).
- G4 prerequisite: the `no_entry` split (indirect vs direct) must be added before G4 is planned; `gpu_no_cpu_mesh` must be split by pool failure reason before it is called P-2 evidence.

Owner test (DevKit build; RTAO on, DXR on, Trace characters on, P2b on, Performance Gather on). Do not toggle DXR off and on and then the cache on (known G2 stall).
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

Owner test (DevKit build; RTAO on, DXR on, Trace characters on, P2b on). Avoid DXR off/on then cache on.
1. Village spot of `world_gc_compare_f295`: cache ON, wait for `[world-gc] settled` plus 10 s, press "Compare GPU instances", then "Dump GPU cache JSON".
2. Same at the large outdoor area.
3. 30 s Performance test with the cache ON at the village (report only).
4. Toggle the cache OFF, ON, OFF; exit with the cache ON. No crash, no STUCK.
5. Send both `world_gc_compare_f*.json`, `world_gc.json`, the GPU-instances and compare panel lines and the `[world-gc]` lines.

Pass (village): no TIMEOUT; `no_entry_indirect` + `no_entry_indirect_untracked` at most 1% of `cpu_instances`; `matched / cpu_instances` at least 0.90 (0.49 before G3b); `match_ratio` at least 0.99; `inst_arena_full` 0 and `inst_draws_cap` 0; overflow 0 / 0 / 0; indirect mismatch at most 1% of indirect draws. Anything else: send the files to the planner.

## Open decisions

1. Foliage motion (P-1, P-4): option (b), a CPU sway approximation, was chosen on 2026-10-14. It still needs a planner pass and harness tests before code. Options are in HISTORY: Work plan, Step 5.
2. P-2 (missing_random): the cause is not yet identified. Decide how dynamic buffers are captured before a fix is designed (HISTORY: Owner decisions and dump findings).
3. Harness pins that fail at HEAD, owner decision: `test_rtao.cpp:1881` pass A FNV pin (0xbcf4d7beeb61f2b6), and `test_live` goldens 2758, 3009 and 3470. The foliage-skip pin procedure is in HISTORY: RTAO Foliage skip. When the pass A pin is re-recorded, world_rtao.cs_5_0.hlsl's IGN line moves to TraceIgnNoise2 (world_bvh_trace.hlsli).
   Also allowed: `test_motion.cpp:1091` ("re-flagged within one capture interval"), which fails on clean HEAD (995ce421 and eff22a05), so it predates RTLS.
4. Whether `RtaoDebug` and `RtaoTwoSidedDiscovery` stay saved with settings or become session-only (Reference: RTAO round 2).
5. Classifier reason for `pixel_shaders[]` on non-alpha shaders (HISTORY: Round A3c).
6. Slack TLAS and skip no-op TLAS stay default OFF until the in-game A/B (HISTORY: TLAS rebuild reduction).
