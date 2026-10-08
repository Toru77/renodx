# Falcom Engine+ ray tracing: handoff (2026-10-08)

This continues a long Claude session (claude.ai, cloud container + your PC via the desktop link) in local Claude Code. Everything needed to continue is in this file, `ROADMAP.md` next to it (full history and evidence, round by round), `../../AGENTS.md` (project rules) and `<repo>/.falcom-dev/` (native test harness, shader bytecode, the raw session transcript).

## 1. Where things stand

Goal: hardware-free (compute) ray tracing in Trails in the Sky 2nd Chapter (D3D11) on RenoDX, DevKit-gated (`falcom_world::Use`/`AddSettings` do nothing unless `IsSora2nd() && IsDevkitPresent()`). All code is in `src/games/falcomengine-plus/world/`.

Validated in game:
- Static world: classifier-based admission of rigid instances from draw-time copies (b1 + exact t15 slices, 3-slot staging ring, no GPU waits), stall-free two-phase mesh capture with capture verification (round 12 fixed the stray triangles), live BVH (CPU LBVH per mesh streamed into GPU arenas, TLAS rebuild at most every 8 frames), primary-ray debug trace, Depth Compare, inspect (middle-click in a trace view).
- Path 1 / S2: characters and water in the BVH via stream output. Owner confirmed "Characters capture properly."

Deployed, awaiting in-game validation (current step):
- Path 2 / P2a: meshes seen moving in a camera view are kept out of the static pool (no ghosts). See section 3.

Working tree, not committed, not validated in game (2026-10-08):
- Alpha-tested foliage (experimental). The Advanced toggle "Alpha-tested foliage (experimental)" is session-only, default OFF. The CPU stage is committed (HEAD 241c0fde). The GPU stage (UV arena, atlas, blit, trace cutout, TLAS slot, diagnostics) is in the working tree, not committed, and not validated in game. Harness and addon build pass. See section 3c.

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

## 3c. Path 3 (GPU stage built, awaiting in-game validation): alpha-tested foliage

Scope (owner): rigid alpha-tested geometry (fences, static leaves) and wind foliage as its rigid rest pose. Billboards are refused (`not_rigid`): their orientation depends on the camera. Deforming wind is not routed through `deform_live` (ROADMAP S1c).

Toggle: "Alpha-tested foliage (experimental)" in Advanced, session only, default OFF. OFF: alpha-tested meshes are not traced; the TLAS drops their instances on the present after the switch (the rebuild bypasses the 8-frame wait, whatever else changed); the atlas is destroyed, and the source copies are freed at the same present. The blit does not run with OFF. ON: their cutouts are traced through the atlas.

Git state: the CPU stage is committed (HEAD 241c0fde). Everything under "GPU stage" is in the working tree: not committed, not validated in game.

CPU stage (HEAD 241c0fde): contract `kAlphaThresholdOffset = 112` (`alphaTestThreshold_g` is `packoffset(c7)`), `kAlphaTexSlot` t0, `kAlphaMaterialSlot` b5, `kAlphaSwizzleSlot` b10, `kTraitAlphaMaterial` on AlphaTested pixel shaders. Pool gate, material per key and per mesh with conflicts (`PoolAlphaState`), UV decode, the refusals, `alpha_atlas.hpp` (slice table, texel packing), the blit shader compiled by FXC.

GPU stage (working tree):
- UV arena (`bvh_resources.hpp`, `bvh_live.hpp`): `uvs` LiveArena (float2, stride 8), in lockstep with the vertices, written only for meshes that have UVs. `LiveMeshSlot.uv_offset/uv_count`. Overload (decided): the mesh descriptor's `bbox_min.w` holds `uv_offset + 1`, 0 = no UVs. Only the trace reads it, and only for alpha. OFF writes 0 everywhere, so the descriptors and arenas are byte-identical to HEAD when no mesh has UVs.
- Recapture (`bvh_pool.hpp`): with ON, a mesh captured without UVs whose key has a readable material is requeued once (`RequeuePoolMeshKey`, `alpha_uv_requeued`, `alpha_uv_requeues`). The key keeps its material, observations and resource keys.
- Source copies (`bvh_pool.hpp`, `CapturePoolAlphaSource`, called from `OnPoolScanDraw`): on the immediate context at draw time, mip 0 is copied into an owned proxy outside the pool lock. Only texture_2d, one layer, non-multisampled. Copied only for a draw that is queued (skip None), so a draw skipped as TooManyInstances, SliceRange etc. takes no copy. At most 4 copies a frame, 64 live and 256 MB of mip 0 (`kAlphaCopiesPerFrame`, `kAlphaSourcesMax`, `kAlphaSourceBytesMax`). Other sources are refused and counted (`alpha_source_refused`, with `source_refused_bytes` and `source_refused_format`). The proxy has the format of the view the game bound (`get_resource_view_desc`), not the texture's; an unknown or typeless view format is refused, logged once per texture. No copy while the live BVH is off (`g_pool.live_on`); copies made before are freed at the present. A copy stays after its blit (blitted), so a slice dropped later (live store reset, quarantine) is filled again from it; it is freed when its draw key is invalidated or alpha_foliage goes OFF. The texture is registered with its draw key (AddPoolResourceKey), so its destruction runs InvalidatePoolMeshKey. Proxies are freed at the next present (`alpha_dead_proxies`), never inside a destroy event. `PoolAlphaMaterial.texture` is an SRV handle, not a texture: the texture is resolved with `get_resource_from_view` at capture.
- Atlas (`bvh_resources.hpp` `AlphaGpu`, `alpha_live.hpp`): Texture2DArray<uint> 256 x 256 x 256 (r32_uint, 64 MB) with SRV and UAV; a StructuredBuffer of 256 AlphaMaterialGPU (32 bytes, zero-initialised); a sampler; the blit (`shaders/world_alpha_blit.cs_5_0.hlsl`: t0 source, s0 sampler, u0 atlas, push constant slice). Created on the first blit, destroyed when the toggle goes OFF and at device destroy. `SyncLiveAlpha` runs in UpdateLiveBvh after the mesh sync and before the TLAS: at most 4 blits a present, a UAV-to-SRV barrier, `store_version` bumped. A full atlas or a failed blit keeps the copy (retried at the next present). The blit only goes into a mesh resident in the live store. A live store reset keeps the slices (pool state decides, not the live store). A released slice is reused after 16 frames (quarantine).
- Trace (`world_bvh_trace.hlsli`, `world_bvh_trace.cs_5_0.hlsl`, `bvh_trace.hpp`): t14 UVs, t15 atlas, t16 materials; `kTraceSrvCount` 17; `TRACE_STATS_COUNT` 17 (alpha_tests 15, alpha_cut 16; invalid reads go to the existing invalid_refs); `TRACE_INSPECT_BASE` 17. In `TraceBlas`, a triangle hit in range goes to `AlphaCutHit` with the instance's material (header.z = slice + 1): UV from the barycentrics, tex = (u + scroll.x, 1 - (v + scroll.y)), frac wrap, point sampled at LOD 0 in the atlas (texel x = floor(frac * 1024), y = floor(frac * 256), word x >> 2, byte x & 3), cut when byte / 255 - threshold < 0. A cut hit is skipped without touching t_max, hidden_t or the shown logic. Material 0, or swizzle bit 0 (alpha forced to 1): solid. An unreadable material or UV: invalid_refs, solid.
- TLAS (`bvh_live.hpp`): an alpha instance gets header.z at the build from `slot_of_uid`. With no slice yet it is left out (`tlas_alpha_waiting`). OFF drops alpha instances. A switch of alpha_foliage is a TLAS change ("alpha switch"): OFF rebuilds at once, ON waits for the interval. `sizeof(WorldInstanceGPU)` stays 192.
- Admission (`bvh_pool.hpp`, `AdmitPoolInstance`): with ON, a conflict-free alpha mesh with UVs is admitted; the `alpha_not_ready` refusal is removed. Refused_off, conflict_refused and no_uv remain.
- Diagnostics: panel lines "Alpha foliage", "Alpha GPU", "Alpha TLAS". `world_pool.json`: `alpha` (`not_ready` is now `alpha_waiting` in `alpha_gpu`), and an `alpha_gpu` object (slices_used of 256, blits, blits_frame, proxies, proxy_bytes, copies, source_refused, source_refused_bytes, source_refused_format, uv_requeues, cap_refused, tlas_alpha_instances, alpha_waiting, alpha_tests, alpha_cut, blit_ms, trace_ms). Schema 15 (unreleased, no bump). The `alpha_gpu` numbers are copied under the pool lock by the present (one present behind; the trace fields come from the last readback). ReShade.log: "alpha atlas: first slice filled" once, and the trace line carries alpha_tests and alpha_cut.

Decided (owner, option b): indirect alpha draws get no source copy (their mesh key depends on draw arguments read at resolve time, and their source view can be dead by then). A mesh whose only draws are indirect stays without a slice: it is counted as alpha_waiting in the TLAS (fail closed), not admitted as opaque and not silently dropped. The owner tests this in game.

Default path (toggle OFF): the trace shader binary and its SRV table change (17 SRVs, t14-t16 null with OFF), so "identical" applies to behaviour only.

Verified in the harness (not in game): test_alpha: CPU atlas bookkeeping, classifier, gate, admission, requeue; mock device: copy at draw time, blit at present, dead source freed at present (no blit), full atlas kept, at most 4 blits a present, OFF creates and blits nothing, end to end through UpdateLiveBvh (waiting until the blit, then header.z = slice + 1; OFF out). test_live: UV arena bytes and descriptors (no UV arena without UVs), GPU check with UVs read back, CPU cutout and barycentrics against brute force (27k triangle hits, no mismatches), blit packing against a double-precision reference (worst 1 LSB), MakeLiveInstanceGPU header.z. Addon build and FXC of world_alpha_blit.cs_5_0.hlsl and world_bvh_trace.cs_5_0.hlsl pass.

Not verified here: the D3D11/ReShade calls at runtime (create_resource, copy_texture_region, views, UAV/SRV bindings, barriers, push constants); the trace and the blit in game; FXC register limits at runtime; GPU time; TSan/ASan on the new lock paths (not run).

In-game steps (owner): (1) Advanced, switch "Alpha-tested foliage (experimental)" ON; the panel "Alpha GPU" slices count rises and "Alpha TLAS" instances rise, waiting falls. (2) Depth Compare and trace ms with OFF, then ON. (3) One screenshot of a fence or leaf cutout, ON against OFF. (4) Switch OFF: "Alpha TLAS" instances go to 0 on the next present (the panel lines lag one present, so read them one present later). (5) Send `world_pool.json` (`alpha`, `alpha_gpu`), the ReShade.log lines "alpha atlas: first slice filled" and "trace: ... alpha_tests=... alpha_cut=...", and the panel lines.

## 4. Code map (`world/`)

- `world.hpp` entry; `world_settings.hpp` Ray Tracing tab; `world_state.hpp` census/camera/depth state.
- `capture/`: draw observation (`draw_census.hpp` calls the pool and deform hooks per draw), CB/SRV slot tracking, CPU mirror of small CBs (`cb_value_tracker.hpp`, gives draw-time `instanceOffset_g`), camera capture, depth source, state capture.
- `contract/`: DXBC reflection (`dxbc_reflect.hpp`: RDEF/ISGN/OSGN/SHEX), the game's geometry contract and VS/PS classifier (`shader_contract.hpp`), runtime registry keyed by pipeline (`shader_registry.hpp`).
- `bvh/alpha_atlas.hpp`: alpha slice table and texel packing (CPU). `bvh/alpha_live.hpp`: atlas GPU objects, the blit and `SyncLiveAlpha` (UpdateLiveBvh). `AlphaGpu` and the destroy path are in `bvh_resources.hpp`.
- `bvh/bvh_pool.hpp`: the CPU world pool (draw-time copies, staging ring, resolve, observations/admission, mesh capture + verification, motion probe, P2a, dump). `bvh_live.hpp`: GPU arenas + TLAS. `bvh_build.hpp`: CPU LBVH. `bvh_resources.hpp`: GPU buffers, `WriteBufferRange`. `bvh_trace.hpp` + `shaders/world_bvh_trace.*`: trace pass (17 SRVs, 40 push constants). `bvh_debug.hpp`: per-present orchestration (`OnWorldPresentBvh`: UpdateLiveBvh, UpdateDeformLive, debug pass). `deform_probe.hpp`, `deform_live.hpp`: path 1. `camera_fade.hpp`: near fade mirror. `gpu_timer.hpp`. `world_bvh.hpp`: panel + event registration.
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
- FXC was never available in the cloud; shaders were checked with DXC cs_6_0 there. On this PC the addon build runs FXC (`fxc.exe` in the repo `bin/`); a clean addon build is the compile check.

## 6. Native test harness (`<repo>/.falcom-dev/harness`)

Windows (MSVC) harness only. The Linux/WSL setup (`build.sh`, `run_all.sh`) is retired; do not use it. A mock ReShade device/command list (stream output emulation, ReShade's offset-0 and stride-0 quirks reproduced), stub `src/utils` (log/path/data/scene/settings), and transcriptions of the GPU shaders for CPU-vs-brute-force checks. See `.falcom-dev/harness/README.md`.

Workflow: `powershell -File refresh_world.ps1` (copies the repo's `world/` sources into the harness; always before a build), then `powershell -File run_win_all.ps1`, or `run_win_some.ps1 -Tests ...` / `run_win_one.ps1 -Test ...`. Tests: test_pool, test_indirect, test_switches, test_visibility, test_live, test_build, test_verify, test_deform, test_deform_live, test_motion, test_alpha. All 11 passed on current code after the last refresh (2026-10-08). No ThreadSanitizer or ASan run on Windows in this round; the earlier TSan results were from the Linux setup and do not cover later changes. Real shader bytecode for the classifier tests is in `.falcom-dev/bytecode/`.

Working method for each round: implement, add/extend a harness test that reproduces the in-game situation, refresh and run all tests, build the addon target (the real compile check and FXC for shaders), then give the owner exact in-game steps and what to send back. Compare the default path (toggle OFF) against HEAD when a change touches shared pool logic.

## 7. Owner preferences

- Step by step; each milestone validated in game before the next. "No mistakes. No regressions."
- Reports: what changed, why, what was verified, exact test steps, what to send back (panel lines, `world_pool.json`, `world_deform.json`).
- Discovery first when the design depends on an unknown.
