# Falcom Engine+ ray tracing: handoff (2026-10-08)

This continues a long Claude session (claude.ai, cloud container + your PC via the desktop link) in local Claude Code. Everything needed to continue is in this file, `ROADMAP.md` next to it (full history and evidence, round by round), `../../AGENTS.md` (project rules) and `<repo>/.falcom-dev/` (native test harness, shader bytecode, the raw session transcript).

## 1. Where things stand

Goal: hardware-free (compute) ray tracing in Trails in the Sky 2nd Chapter (D3D11) on RenoDX, DevKit-gated (`falcom_world::Use`/`AddSettings` do nothing unless `IsSora2nd() && IsDevkitPresent()`). All code is in `src/games/falcomengine-plus/world/`.

Validated in game:
- Static world: classifier-based admission of rigid instances from draw-time copies (b1 + exact t15 slices, 3-slot staging ring, no GPU waits), stall-free two-phase mesh capture with capture verification (round 12 fixed the stray triangles), live BVH (CPU LBVH per mesh streamed into GPU arenas, TLAS rebuild at most every 8 frames), primary-ray debug trace, Depth Compare, inspect (middle-click in a trace view).
- Path 1 / S2: characters and water in the BVH via stream output. Owner confirmed "Characters capture properly."

Deployed, awaiting in-game validation (current step):
- Path 2 / P2a: meshes seen moving in a camera view are kept out of the static pool (no ghosts). See section 3.

Agreed plan, in order (owner's words in ROADMAP "Plan agreed 2026-10-07"):
1. Deforming meshes (stream-out): done for characters and water. Wind foliage and billboards were moved into path 3 (they are indirect, GPU-culled, alpha-tested leaves).
2. Moving rigid objects (doors, carts, the fork an NPC holds): in progress (P2a deployed, P2b next).
3. Alpha-tested geometry (foliage, leaves, fences; plus wind foliage and billboards): texture coordinates in the pool, each material's alpha texture copied into one texture array (DX11 cannot bind arbitrary textures per hit), alpha test during traversal.
4. First RT effect: RTAO. Options exactly: Radius, Strength, Samples Per Pixel, Ray Max Distance, Normal Bias, Two-Sided Geometry, Temporal Accumulation {History Weight, Depth Rejection, Normal Rejection, History Clamp}, Denoising {Spatial Filter, Filter Radius, Filter Quality}, Distance Fade {Fade Start, Fade End}. Temporal Accumulation and Denoising are not implemented yet but their UI must exist (dead code OK). Use IS-FAST blue noise (already used by the custom DOF, motion blur and GTVBAO in this addon; see `fast_noise_ea.h`, `gtvbao/`). When RTAO is on it overrides GTVBAO even if GTVBAO is toggled on, and the GTVBAO control is greyed out; when RTAO is off, GTVBAO is back under its own control.

GPU cost note (owner asked): the BVH debug trace (~17-22 ms) is a validation tracer (Morton LBVH, one triangle per leaf, test-on-pop, no child ordering, local-memory stacks, many checks/counters). RT effects will use any-hit short rays at reduced resolution with accumulation; tracer optimization is milestone M8/M13, after BVH contents are steady.

## 2. Path 1 (done): deforming meshes

- Discovery (S1, `bvh/deform_probe.hpp`): re-runs sampled camera-view draws of deforming VS classes (skinned/wind/billboard/animated) with a pass-through stream-output geometry shader created natively (`ID3D11Device::CreateGeometryShaderWithStreamOutput` from the VS bytecode, `D3D11_SO_NO_RASTERIZED_STREAM`); finds which VS output is the world position by projecting with the frame's viewProj. Result: TEXCOORD1 (most skinned) or TEXCOORD0 (3 skinned, water 0x91FD3859, all wind, billboard 0x4E30FAF1); 0x196C7DB4 packs something else in w (use xyz).
- Live (S2, `bvh/deform_live.hpp`, `shaders/world_bvh_refit.cs_5_0.hlsl`): per frame, each confirmed camera draw (direct, immediate context, no game GS/HS/DS/SO bound) is re-run with SO into a 24 MB arena (float3/vertex); first capture of a draw identity is read back to build its LBVH topology once; a compute refit updates bounds every frame; the trace walks these objects after the TLAS (`hit.instance = 0x80000000 | object`, `hit.mesh = 0xFFFFFFFE`). Panel: "Deforming meshes in the BVH (characters, water)".

## 3. Path 2 (in progress): moving rigid objects

Evidence (M1 motion probe, ROADMAP "Path 2"): `InstanceParam.prevWorld` (@48) is last frame's world in camera passes (the VS uses it with `prevViewProj_g` for motion vectors). Camera sightings of unmoved objects: 152,343, bit-exact in every family except near-zero rotation elements (~3e-8). Shadow passes leave prevWorld all zero (a few light families fill it). So a camera sighting with prevWorld != world (beyond noise) is moving. Also found: before P2a the pool never retired an admitted instance except when its VB/IB was destroyed, so an object that moved and stopped was admitted at every stop (ghosts).

P2a (deployed 2026-10-08, `bvh/bvh_pool.hpp`, panel in `bvh/world_bvh.hpp`):
- Rule `PoolSightingMoves` (camera sightings only): prevWorld filled and finite; translation component off by > max(1e-5 m, 4 ULP of the coordinate) or a basis element off by > 1e-5 x max(1, largest basis element).
- A moving sighting records its draw key in `g_pool.dynamic_keys` (`PoolDynamicMesh` evidence) and, with "Keep moving objects out of the static BVH" on (default), flags the key's mesh (`WorldMesh::dynamic`, mesh level, so same-content meshes and shadow draws are covered): admitted instances removed, admissions refused (`AdmitPoolInstance`). Flag also applied at capture if the key moved before its mesh was captured. Switch off clears flags (observations re-admit on next sighting); on re-applies (`ApplyPoolDynamicSwitch`, at each present in `DrainPoolScan`).
- Expected now: moving objects are absent from the BVH (until P2b). Known limit: each draw identity is read every 30 frames (`kPoolRecaptureFrames`), so an object that moves for less than ~0.5 s may never be sampled moving and can still leave a ghost.
- Diagnostics: panel lines "Motion (prevWorld vs world)", "camera: unmoved ... shadow: prevWorld zero ...", "Moving objects: N draw keys moving now (M seen), K meshes flagged, R instances removed, B admissions refused", and a rule line (camera sightings with prevWorld differing, and how many are stale). Dump `world_pool.json` schema 9 (schema 14 now, see 3b): `switches.exclude_moving`, `motion` (per-view counters, `rule_stale`, `rule_stale_samples`), `dynamic` (`moving_keys`, `released`; each key: VS, frames, sightings, indirect count, max meters/basis, retired instances, refused admissions, `moving_now`, `released`, mesh, sample world/prevWorld).

P2a revision (2026-10-08, not yet validated in game):
- Identity prevWorld (elements 0,5,10 = 1, others 0) counts as not filled: no motion.
- A camera sighting is Moving when prevWorld differs and its (mesh, world) was not seen in an earlier frame; Still when prevWorld is filled and matches; otherwise Unknown. A repeat sighting whose prevWorld differs is stale (`rule_stale`) and is not Moving.
- A moving key stays moving (its mesh stays flagged) until a Still sighting comes at least `kPoolMovingHoldFrames` (60 frames) after its last moving sighting. Then the key is released (`moving_now` false, `released`++) and, when no other moving key maps to its mesh, the mesh is unflagged (`dynamic_released`++): the stopped pose is admitted again. Releases do not bump the revision. A released key is not re-flagged by the switch; a new moving sighting re-arms it. A mesh flagged through a key whose resources were destroyed is unflagged at CompactPool when no remaining moving key maps to it (tests 19 and 20 in test_motion.cpp).

What to ask the owner to check for P2a (not yet done when the session moved):
1. Pool Scan on, a few minutes around moving things (doors, carts, NPCs carrying items, swaying props).
2. The rule line must say "none of an unmoved object".
3. Depth Compare: moving objects now "missing" is expected; nothing static should newly go missing (if it does: inspect it, check `dynamic` entries with large `retired_instances`, the mesh-level flag may be too broad for shared assets).
4. `world_pool.json` -> review `dynamic` entries.

P2b (next, not started), proposed design:
- For flagged meshes, every frame copy the instance slices of their draws (camera and shadow, direct and indirect; slice math from the pool's `ResolvePoolSlice`/`ReservePoolCopy`, b1 base from the CB mirror, indirect counts from the args) GPU-to-GPU into a per-frame arena, plus a small per-draw table (mesh descriptor in the live store, slice offset, count or args offset).
- A compute pass expands them into instance records with the same GPU layout as static instances (`WorldInstanceGPU`: world, inverse, bounds, mesh index, visibility flags), skipping zero-scale/parked matrices like `CheckPoolWorld`; inverse must match `ComputeMatrixInverse` (transcribe + test). Camera-drawn instances get the camera-visible flag; shadow-only ones the shadow-caster flag.
- The trace walks them as a small list next to the TLAS (like the S2 dynamic loop), reusing the static instance/BLAS traversal (one implementation). Requires the mesh to be resident in the live store even with no static instance (check `bvh_live.hpp` mesh streaming selection).
- Zero latency (copied at draw time, traced at present). Trade-off of the mesh-level policy: a flagged mesh exists in the BVH only where the game draws it that frame (camera or shadow cascades).
- Diagnostics: per-frame dynamic instance count, draws copied, skipped with reasons, arena use, GPU time; inspect lines for dynamic instances.
- Tests: extend the harness like test_deform_live (emulate the expansion + trace vs brute force).

## 3b. Diagnostics round (2026-10-08, deployed for in-game validation; no new behaviour except M0)

Plan: M0 mesh retry switch, M1 dump split, M2 staging forensics, M3 deform presence, plus the last inspect result in the pool dump. M4/M5 not started (they need in-game data first).

- Dump is now split (schema 14): `world_pool.json` (summary: stats, classifier, families, visibility, motion, dynamic, follow, mesh_failed, mesh_mismatches, mesh_retrying with hex keys, switches, `inspect`, `files`, `meshes_total`, `instances_total`, `instances_written`), `world_pool_meshes.json` (mesh entries) and `world_pool_instances.json` (instance entries; adds `mesh_uid`, `camera_seen`, `light_seen`, `near_fade`, `scale`). All three carry the same `generated_frame`. The detail files are written first, the summary last.
- `inspect`: the last middle-click result (`set`, `frame`, `hit`, `instance_id`, `mesh_uid`, `lines`). Stored in `bvh_debug.hpp` when the inspect is described; zero ids for moving (dynamic) hits.
- M0 `retry_unstable` (panel "Retry unstable meshes (diagnostic)", default OFF, `switches.retry_unstable`): OFF rejects a mesh whose captures never repeat at the first round (the pre-c86553b5 rule). ON keeps the 3 retry rounds of c86553b5. Meshes admitted after a retry: `WorldMesh::admitted_by_retry`, stats `meshes_admitted_by_retry`. Already-admitted meshes stay until Reset Pool or restart.
- M2 `poison_staging` (panel "Poison mesh staging (diagnostic)", default OFF, not in the dump switches): mesh staging becomes cpu-visible (`memory_heap::cpu_only`) and is filled with `kPoolPoisonWord` (0xFFC0DEAD) after each mesh read, outside the lock. Each mesh read records a `staging` object (slot, ordinal, staging offset, copied, begin mod 16, poison words in the copy and in its first 64 B, first 48 raw bytes, `head_class`: sentinel / same offset previous cycle / other offset previous cycle / same cycle / none; the last 2 cycles per slot are kept). Shown on capture records and failures, and in ReShade.log "staging:" on mismatch lines.
- M3 deform presence (`world_deform_live.json`, written by the same "Dump Pool JSON" button): per identity `first_frame`, `last_frame`, `gaps`, `max_gap`, `pending_frames`; stats `presents_no_objects`, `calls_max_per_frame`, `new_identities`, `new_after_warmup` (warm-up 600 frames), `resets_with_objects`, `reset_objects_dropped`, skips by reason; a ring of the last 64 `dropouts` (frame, key, vs, triangles, skips since the previous present).
- Harness: test_verify (retry off/on, dump files, schema 14), test_visibility (near-fade check reads the meshes file), test_switches, test_motion, test_pool, test_indirect, test_build, test_deform, test_deform_live, test_live all pass at this round (see the round report). Poison staging: test_verify 10b (a copy whose range is left as poison: sentinel_words > 0 and head_class sentinel in the dump). Deform presence: test_deform_live 8 (identities non-empty; gaps, max_gap, pending_frames in the dump).

## 4. Code map (`world/`)

- `world.hpp` entry; `world_settings.hpp` Ray Tracing tab; `world_state.hpp` census/camera/depth state.
- `capture/`: draw observation (`draw_census.hpp` calls the pool and deform hooks per draw), CB/SRV slot tracking, CPU mirror of small CBs (`cb_value_tracker.hpp`, gives draw-time `instanceOffset_g`), camera capture, depth source, state capture.
- `contract/`: DXBC reflection (`dxbc_reflect.hpp`: RDEF/ISGN/OSGN/SHEX), the game's geometry contract and VS/PS classifier (`shader_contract.hpp`), runtime registry keyed by pipeline (`shader_registry.hpp`).
- `bvh/bvh_pool.hpp`: the CPU world pool (draw-time copies, staging ring, resolve, observations/admission, mesh capture + verification, motion probe, P2a, dump). `bvh_live.hpp`: GPU arenas + TLAS. `bvh_build.hpp`: CPU LBVH. `bvh_resources.hpp`: GPU buffers, `WriteBufferRange`. `bvh_trace.hpp` + `shaders/world_bvh_trace.*`: trace pass (14 SRVs, 40 push constants). `bvh_debug.hpp`: per-present orchestration (`OnWorldPresentBvh`: UpdateLiveBvh, UpdateDeformLive, debug pass). `deform_probe.hpp`, `deform_live.hpp`: path 1. `camera_fade.hpp`: near fade mirror. `gpu_timer.hpp`. `world_bvh.hpp`: panel + event registration.
- `debug/`: crash log (vectored handler + CRT hooks, "Log crashes"), pool stage markers.
- `reference/.dumps/`: decompiled game shaders (3Dmigoto) used as evidence.

Present order: `OnWorldPresentBvh` (registered first) runs before `OnWorldPresent` (frame++, DrainPoolScan, DrainDeformProbe).

Game facts: all world VS read `StructuredBuffer<InstanceParam> instances_g : t15` (stride 160: world float4x3 @0 stored so world.x = dot(float4(p,1), m[0..3]), translation in m[3], m[7], m[11]; prevWorld @48; color @96; uv @112; param @128; boneAddress @144); index = SV_InstanceID + `instanceOffset_g` (b1 c0.x). Rigid VS project with `cb_scene.viewProj_g` (camera) or `cb_shadow.shadowViewProj_g` (light).

## 5. Invariants and lessons (do not relearn these)

- Lock rule (root cause of the 2026-10-06 crashes): no graphics call while holding `g_pool.mutex`, `g_deform.mutex`, `g_deform_live.mutex`... Reserve under the lock, record after it; resources moved out under the lock and destroyed after.
- ReShade D3D11: `update_buffer_region` with offset 0 passes no box (writes the whole buffer, over-reads the source) -> `WriteBufferRange`; `get_resource_desc` buffer stride is 0 -> pass strides explicitly; `create_pipeline` returns failure for stream-output state; ReShade 6.8.0 (API 20) added a `query_type` parameter to `device::get_query_heap_results` with no compat path for API-18 add-ons -> use native D3D11 queries.
- A render target is bound in shadow passes too: decide camera vs light from the VS (`contract::ClassifyVertexView`), not from bindings.
- Pool copies only on the immediate context (deferred lists may run after their slot is read).
- Mesh capture: two identical captures before admission (`verify_meshes`), GPU-written VB/IB contents can change unseen.
- Never run git inside the device VM mount (left `.git/index.lock` once, deletes are blocked there). Locally this does not apply.
- Files in the addon are CRLF; keep it.
- New shaders in `world/shaders/` are embedded automatically (`__<name>_EMBED_FILE`, `__<name>` span; code guards with `#if defined(...)`).
- Never put `.cso` files under `src/games/falcomengine-plus/` (CMake globs and embeds them).
- FXC was never available in the cloud; shaders were checked with DXC cs_6_0 there. Your build is the FXC check.

## 6. Native test harness (`<repo>/.falcom-dev/harness`)

Linux g++ (ASan/UBSan, TSan) harness with a mock ReShade device/command list (stream output emulation, ReShade's offset-0 and stride-0 quirks reproduced), stub `src/utils` (log/path/data/scene/settings), and transcriptions of the GPU shaders for CPU-vs-brute-force checks. Run it under WSL (Ubuntu, g++ 13+): see `.falcom-dev/harness/README.md`. Tests: test_pool, test_indirect, test_switches, test_visibility, test_live, test_build, test_verify, test_deform, test_deform_live, test_motion. All passed at the P2a deploy (TSan clean for switches/verify/motion). Real shader bytecode for the classifier tests is in `.falcom-dev/bytecode/`.

Working method used so far for each round: implement in a copy, add/extend a harness test that reproduces the in-game situation, run all tests (+TSan for threaded paths), compile check, then deploy and give the owner exact in-game steps and what to send back.

## 7. Owner preferences

- Step by step; each milestone validated in game before the next. "No mistakes. No regressions."
- Reports: what changed, why, what was verified, exact test steps, what to send back (panel lines, `world_pool.json`, `world_deform.json`).
- Discovery first when the design depends on an unknown.
