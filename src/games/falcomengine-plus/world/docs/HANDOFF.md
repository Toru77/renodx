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
- Alpha-tested foliage, Stage B, partial. The Advanced toggle "Alpha-tested foliage (not functional yet)" exists, default OFF. CPU pool logic and CPU atlas bookkeeping are done; GPU alpha atlas, UV arena, trace cutout and the test_live extension are NOT done. With the toggle ON nothing new is admitted yet (`alpha_not_ready`). See section 3c.

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

## 3c. Path 3 (in progress, unvalidated): alpha-tested foliage

Scope agreed by the owner: rigid alpha-tested geometry (fences, static leaves), plus wind foliage as a rigid rest pose. Billboards are not admitted: the gate takes only rigid vertex shaders, and wind only while the toggle is on, so billboards are `not_rigid` in both modes (their orientation depends on the camera, so they have no stable rest pose). Deforming wind is NOT routed through `deform_live` (it is indirect and GPU-culled; per-frame triangle count changes; see ROADMAP S1c).

Toggle: "Alpha-tested foliage (not functional yet)" in Advanced, session only, default OFF. Tooltip: "Work in progress: the GPU stage does not exist, ON admits nothing new (alpha meshes stay refused)."

Done (working tree):
- Contract: `kAlphaThresholdOffset = 112` (`alphaTestThreshold_g` is `packoffset(c7)`, confirmed in `sora1st/foliage/clutter_0x68C07DEA.ps_5_0.hlsl`; the earlier plan's 128 was wrong), `kAlphaTexSlot` t0, `kAlphaMaterialSlot` b5, `kAlphaSwizzleSlot` b10, `kTraitAlphaMaterial` (16) on AlphaTested pixel shaders with that layout.
- CB mirror widened to 160 bytes (`kTrackedCbBytes`); `valid_bytes` contiguous prefix; `ps_cb_offset` recorded so an offset bind fails closed.
- Pool gate (`GatePoolDraw`, `alpha_on` read once). Toggle OFF is the HEAD gate: a rigid AlphaTested draw is `alpha_tested`, wind and billboard draws are `not_rigid`; no material is read and nothing is flagged. Toggle ON: the material is read only for a pixel shader with kTraitAlphaMaterial (else `pixel_unknown`), and an opaque pixel shader on wind is `wind_opaque`. The alpha flag and its stats come only from draws recorded while ON; such a draw still flags its mesh if the switch flips OFF before its copy resolves, and a flag persists after OFF until Reset Pool.
- Material per draw key (`PoolAlphaState`, `AbsorbPoolAlphaMaterial`): the first readable material wins; an unreadable one (texture 0) never conflicts; a differing readable one sets `conflict`. `alpha_conflicts` counts conflicted keys.
- Material per mesh: `WorldMesh::alpha_state` folds the states of its keys with the same rule. A conflicted mesh is refused (`alpha_conflict_refused`); `alpha_meshes_conflicted` counts meshes that took a conflict.
- Clearing: the mesh flag and conflict are cleared in `CompactPool` only when no alpha key maps to the mesh (one invalidated key of several keeps them). Toggle OFF/ON does not clear them; `ResetWorldPool` clears everything.
- Admission refusals while the mesh is flagged, in order: `alpha_refused_off` (OFF), `alpha_conflict_refused`, `alpha_no_uv`, `alpha_not_ready`. A mesh captured while OFF has no UVs (its alpha key did not exist at the capture), so with ON it is refused as `alpha_no_uv`. A mesh whose key is noted before its capture gets its UVs and is refused as `alpha_not_ready`.
- UV decode (`DecodePoolMeshVertices`): UVs are dropped unless the UV element lies inside the stride and the copy holds it for every vertex, and all UVs are dropped unless there is one per position.
- Blit shader `shaders/world_alpha_blit.cs_5_0.hlsl` compiles with FXC (repo `bin/fxc.exe`) and in the addon build. It is embedded by the CMake glob (CONFIGURE_DEPENDS), so no CMake change was needed. `bvh/alpha_atlas.hpp` is CPU only and harness-only (nothing in `world/` includes it): `AlphaMaterialGPU` (32 bytes), 4 alpha texels per uint, `AlphaSliceTable` (256 slices; `Acquire` returns -1 at the cap; double and out-of-range release ignored).
- Dump: schema 15 (unreleased, no bump), `alpha` object on the main file (`alpha_foliage`, draws, cb_unavailable, conflicts, meshes_conflicted, refused_off, conflict_refused, no_uv, not_ready, removed, keys, billboard_draws, wind_opaque_skips), per-mesh `alpha`, `alpha_conflict` and `uvs` in the meshes file.
- Harness: `test_alpha.cpp` covers the CPU atlas; the classifier on real bytecode (0x137F316A, 0x81F5709F, 0xAA835FE0 with kTraitAlphaMaterial; 0x049B0385, 0x2807FFC9, 0x2DADE2B8 without; 0x2162672F opaque); NotePoolAlphaMaterial (first wins, four kinds of conflict, unreadable then readable, an ON record still flags while OFF); the material read (fails closed on a partial mirror, unbound b5, offset b5, null t0, bad swizzle offset; full 160 bytes decodes); the switch ON/OFF/ON/OFF; UV decode cases; the gate in both modes; the admission refusals; per-mesh conflict in both capture orders; clearing; and the scan path OFF, ON, OFF; the indirect alpha path (an ON copy carries the pass and material and flags its real key, an ON record flags after OFF, OFF records nothing). Suite: 11 tests (10 existing + test_alpha), all pass.
- Default-path check (toggle OFF against HEAD): the implementer reports identical counts on the 10 existing tests (no output file kept); see ROADMAP Round A3b.
- Accepted for this round (owner): `ComparePoolMeshes` reports a UV count mismatch only when ON. Meshes captured while OFF have no UVs and stay `alpha_no_uv` after ON (deviation 3(b)); the in-game dump decides whether a recapture fix is needed before the GPU stage.

Not done:
1. GPU UV arena (stride 8, in lockstep with vertices in `bvh_resources.hpp`/`bvh_live.hpp`), the `Texture2DArray<uint>` atlas lifecycle, the per-frame blit dispatch from `UpdateLiveBvh`, and the SRV lifetime for each source texture. The lifetime is not safe yet: a texture handle could be reused after release. Nothing binds these resources today.
2. Trace shader: t14 UVs, t15 atlas, t16 materials, `kTraceSrvCount` 14 -> 17, bary output from `IntersectTriangle`, any-hit cutout in `TraceBlas` (not `TraceDynamicBlas`), stats. Material slot as `header[2]` stored as slot+1 (0 = none). Must not start until item 1 exists: binding empty views would cut every alpha sample to zero.
3. test_live extension (CPU transcription of AlphaPasses and any-hit trace vs brute force), the remaining alpha cases (UV mismatch on a changed buffer, dynamic and follow with alpha), test_pool case E update, dump swizzle field.

In-game check for the current state (no visible alpha cutouts expected yet): Advanced checkbox present, labelled "Alpha-tested foliage (not functional yet)", and off; ReShade.log switches line says "alpha foliage off"; toggling ON and OFF changes nothing visible; dump schema 15 `alpha` counts (`not_ready`, `no_uv`, `refused_off`, `conflict_refused`); Depth Compare unchanged versus the previous build with the toggle OFF.

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
- FXC was never available in the cloud; shaders were checked with DXC cs_6_0 there. On this PC the addon build runs FXC (`fxc.exe` in the repo `bin/`); a clean addon build is the compile check.

## 6. Native test harness (`<repo>/.falcom-dev/harness`)

Windows (MSVC) harness only. The Linux/WSL setup (`build.sh`, `run_all.sh`) is retired; do not use it. A mock ReShade device/command list (stream output emulation, ReShade's offset-0 and stride-0 quirks reproduced), stub `src/utils` (log/path/data/scene/settings), and transcriptions of the GPU shaders for CPU-vs-brute-force checks. See `.falcom-dev/harness/README.md`.

Workflow: `powershell -File refresh_world.ps1` (copies the repo's `world/` sources into the harness; always before a build), then `powershell -File run_win_all.ps1`, or `run_win_some.ps1 -Tests ...` / `run_win_one.ps1 -Test ...`. Tests: test_pool, test_indirect, test_switches, test_visibility, test_live, test_build, test_verify, test_deform, test_deform_live, test_motion, test_alpha. All 11 passed on current code after the last refresh (2026-10-08). No ThreadSanitizer or ASan run on Windows in this round; the earlier TSan results were from the Linux setup and do not cover later changes. Real shader bytecode for the classifier tests is in `.falcom-dev/bytecode/`.

Working method for each round: implement, add/extend a harness test that reproduces the in-game situation, refresh and run all tests, build the addon target (the real compile check and FXC for shaders), then give the owner exact in-game steps and what to send back. Compare the default path (toggle OFF) against HEAD when a change touches shared pool logic.

## 7. Owner preferences

- Step by step; each milestone validated in game before the next. "No mistakes. No regressions."
- Reports: what changed, why, what was verified, exact test steps, what to send back (panel lines, `world_pool.json`, `world_deform.json`).
- Discovery first when the design depends on an unknown.
