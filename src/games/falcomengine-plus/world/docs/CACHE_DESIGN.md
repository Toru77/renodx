# Falcom Engine+ BVH Cache Design (phase 1, 2026-10-15)

Status: design only. Nothing here is implemented. The ROADMAP section "Phase 0 cache diagnostics" is the evidence source (build `cache-diag-0`, not validated in game). Read `ROADMAP.md` first; this file adds one design and its gates. Project rules are in `../../AGENTS.md`. Citations are `file:line` at HEAD d8b9328a; `bvh/` means `world/bvh/`.

Goal (owner): a BVH cache held in RAM or VRAM only, never on disk. Distance tiers: 0-128 m full detail, 128-256 m reduced, 256 m and beyond lesser. Map-change and loading-screen detection invalidates the cache when the fit check fails. World-space validity must be verified.

## What exists today (the cache is half there)

- Per-mesh BLAS is already an in-session cache. The live store (`bvh/bvh_live.hpp:8-19`) uploads each pool mesh once into append-only GPU arenas, keyed by `WorldMesh::uid` (never reused, `bvh/bvh_pool.hpp:466`, assigned at `:4610`). The BLAS is built on the CPU (`UploadLiveMeshes`, `bvh_live.hpp:262-311`) and only the GPU copy is kept.
- It is dropped too early. A mesh leaves the pool when its last draw key is invalidated (a game buffer destroy: `OnDestroyResourcePool`, `bvh_pool.hpp:4409-4433`; `live_keys == 0`, `:3222-3223`, `:4457-4461`). Re-created buffers decode to a new `WorldMesh` with a new uid (`:4610`), so the BLAS is built and uploaded again. `RetireLiveSlots` zeroes the descriptor at once (`bvh_live.hpp:186-205`). Retired slots count as garbage (`:104-127`). The store resets when garbage is over 16 MB and over half the store, when no live mesh is left (a map change), or when full (`LiveStoreResetReason`, `:207-215`).
- The TLAS is rebuilt whole, never cached. `RebuildLiveTlas` (`bvh_live.hpp:646-903`) copies every in-region instance under the pool lock (`:675-701`), sorts, builds, validates and creates four new exact-size GPU buffers (`:855-863`). The trigger is any pool revision, visibility, store, region, alpha or sway change (`LiveTlasChange`, `:501-514`), at most every 8 presents (`kLiveTlasInterval`, `:74`, gate at `:1835-1838`).
- The region is a 512 m box (`kPoolRegionMaxSize`, `bvh_pool.hpp:217`; `region_size`, `:1351`) snapped so the camera is 128 to 384 m from each face (`CurrentPoolRegion`, `:1669-1688`). "Region moved" is already a hysteresis (`bvh_live.hpp:507`). Consequence: T2 (256 m and beyond) exists only on the far side of the region, so it is partial and lopsided. Phase 0 must show it is populated (gate G0).
- Tiers are diagnostic only: Chebyshev distance 128/256, `kCacheTierEdges` (`bvh/cache_diag.hpp:37`), `CacheInstanceTier` (`:55-59`), `ComputeCacheTiers` (`:100-145`, mesh tier = nearest instance, `:114-117`).
- RTAO needs only T0. Its coverage is region size * 0.25 minus Ray Max Distance, so at most 128 m (ROADMAP "RTAO (round 1)", Range rule). T1 and T2 serve only the debug trace and future effects. Tier work must never touch T0 content (rule 6 below).
- Map change today: a compaction that retires 32 or more meshes (`kPoolMassRetireMeshes`, `bvh_pool.hpp:146`, `:4468-4480`) bumps `episode_serial`, then static instances not seen since are swept (`:5601-5615`). This sweep is already the "fit check by non-sighting" for instances.

## 1. What is cached: BLAS only in 1a, tier-filtered TLAS content in 1b

Two layers with different validity:

- Object space (BLAS, mesh arenas): valid whenever the mesh content is identical. No world-space question. This is the cache of 1a.
- World space (instances, TLAS): valid only while placements are the same. Not cached in phase 1 (instances come from the pool and the existing sweep). 1b changes what the TLAS contains per tier and when it is rebuilt, not how placements are stored.

Cost, from the roadmap numbers (different machines, so orders of magnitude only):

- BLAS: about 95 ns per triangle, 16k triangles about 1.5 ms (ROADMAP Round "Timing", cloud Xeon 2.1 GHz). Capped at 16,384 triangles and 64 meshes per present (`bvh_live.hpp:72-73`), at least one mesh. One-off mesh upload max 22.6 ms (ROADMAP round 8, user CPU). Counters: `blas_builds`, `blas_built_triangles`, `blas_build_ms_sum`, `blas_build_ms_max` (`bvh/bvh_resources.hpp:183-186`), timed around `BuildMeshBlas` only (`bvh_live.hpp:281-286`); `mesh_ms_last/max` (`bvh_resources.hpp:178-179`) cover the whole sync.
- TLAS: 7.8 ms per rebuild at 16.7k instances on the user's CPU (ROADMAP round 8; also "Live store limits"), recurring every 8 presents while the set changes. Counters: `tlas_ms_last/max` (`bvh_resources.hpp:180-181`), `PoolTlasTiming::rebuild_ms_by_cause` (`bvh_pool.hpp:1309-1310`).
- Which dominates (estimate, to be replaced by the owner's data): per event the TLAS (7.8 ms) is above the per-present BLAS bound (about 1.5 ms). Over a 300-present refill with a rebuild every 8 presents the TLAS costs about 37 * 7.8 = 290 ms; the BLAS total is `pool_tris` * 95 ns (1M triangles = about 95 ms, spread over 61 presents). BLAS dominates only for one huge mesh (the 22.6 ms case).
- Unknown and decisive: capture. A recreated buffer must still be captured (two-phase read-back, two identical captures, `verify_meshes`, ROADMAP invariants) before its signature exists. Retention saves the BLAS build and upload, not the capture. If t80 is mostly capture and observation wait, 1a gains little. Step 0b measures this before any code (gate G1).

## 2. Key: in-session only

- Cross-session is rejected: no disk (owner), `WorldMesh::signature` drifts about 0.75% across reloads (owner report), and alpha signatures mix the SRV handle (`PoolMix(signature, material.view)`, `bvh_pool.hpp:4585-4587`), which is not stable. Placement compare is not yet a trusted pass either. Nothing keyed on a signature may outlive the process.
- BLAS key: `WorldMesh::signature` (content hash of positions, UVs, triangles, `:4548-4570`), in-session only. It is the pool's own dedup key (`mesh_by_signature`, `:4595-4599`), so within one session the same decoded content maps to one mesh. `uid` is not usable as the cross-retirement key because a re-captured mesh gets a new uid.
- Instance key (for diagnostics and any later instance cache): (mesh signature, `MatrixHash` of the 12 floats), the same pair the placement exact test uses (`bvh/placement_probe.hpp:277`). Do not use `PoolAdmittedKey` (`bvh_pool.hpp:1707`): it mixes `mesh_id`, which is renumbered at every compaction (`:4462-4465`, rebuilt at `:4492`).
- Excluded from retention (decided here): alpha-tested meshes (unstable signature), meshes with `!verified` or `writes_after_capture != 0` (`:482-484`), failed meshes.

## 3. Tier policy

Tiers by Chebyshev distance from the camera to the instance bounds (`PoolChebyshevDistance`, `bvh_pool.hpp:1698`), in the region only. Edges 128 m and 256 m.

Hysteresis (fixed here, constants in one place):

- An instance moves to a nearer tier when its distance is below edge - 8 m, and to a farther tier only when it is at or above edge + 8 m. Band 8 m at both edges. The previous tier is read from the last TLAS (`data->tlas_info`, ids), not stored per instance.
- A tier-only change (no pool change) may rebuild the TLAS at most every 64 presents. Pool changes keep the 8-present rule.
- A T0 instance is never thinned and never delayed by a tier rule.

Content per tier (1b only; thresholds provisional, confirmed from the 0b size histogram):

- T0 (below 128 m): every instance, as today.
- T1 (128 to 256 m), "reduced": an instance stays when extent / distance >= 0.005 (projected size, extent = largest bounds side), plus every dynamic and wind instance (few, and they refit).
- T2 (256 m and beyond), "lesser": extent / distance >= 0.010, plus dynamic and wind instances.
- Reduced and lesser mean instance thinning, not geometry decimation. No mesh simplifier exists in the repo and adding one is a new subsystem; BLAS LOD is "Later", not phase 1. Owner decision recorded here: if "reduced" should mean decimated meshes, say so before 1b.

Retention priority (1a): by the tier of the mesh's nearest instance at its last TLAS rebuild, T0 kept longest. A map change sets every slot's last tier to T2 (distances are meaningless after a jump), so old-map meshes are evicted first and shared assets are still revived.

## 4. Storage and eviction (VRAM only for 1a)

- Retained meshes stay in the existing arenas as slots marked `retained` (new field next to `live`, `bvh_resources.hpp:126-142`). No CPU copy: the pool drops the vectors at compaction (`bvh_pool.hpp:4466`), RAM cost is the slot metadata only (about 100 B). If the store is reset the retained set is gone; the pool restreams what it still holds.
- Slot extra fields: `signature`, `last_tier`, `retire_frame`, `retainable` (set at upload from the scan, under the pool lock, `ScanLiveMeshes` `bvh_live.hpp:160-183`).
- Budget (provisional, confirm against the Phase 0 `resident_bytes` rows): retained bytes at most min(32 MB, one quarter of `used_bytes`) (`LiveSlotBytes`, `bvh_live.hpp:98-102`). Arenas are append-only (`bvh_live.hpp:13-19`), so evicting a slot frees nothing until a store reset. Rule: eviction order is last tier (T2 first), then oldest `retire_frame`; when retained slots exceed the budget or `retire_frame` is older than the hold (provisional 1800 presents, set from the 0b gap histogram), they become garbage and the existing garbage rule (`:207-215`) reclaims space. No new compaction or free list.
- Required edit of the reset rules: "no live mesh left" (`:209`) must count retained slots, or every map change wipes the retained set and 1a does nothing. Retained bytes must not count as garbage in `UpdateLiveStoreStats` (`:113-115`).
- On map change (mass retirement): retained slots stay (see tier policy above); the pool's static instances are swept as today (`bvh_pool.hpp:5601-5615`); nothing else changes.
- Revive: in `SyncLiveMeshes` (`bvh_live.hpp:451-488`), a pool mesh with no slot whose signature matches a retained slot reuses it: check `vertex_count`, `index_count` and bbox equal the new mesh, then set the slot's uid, restore the descriptor, `slot_by_uid`, bump `store_version`, skip the BLAS build and upload. The descriptor fill in `UploadLiveMeshes` (`:396-410`) becomes one helper used by both paths (two uses, allowed by AGENTS.md).
- Mismatch on revive: count it, drop the retained slot, build normally.

## 5. Invalidation

| Trigger | Signal and source | Effect on the cache | Must be validated in game first |
| --- | --- | --- | --- |
| Map change | `episode_serial` bump on mass retirement (`bvh_pool.hpp:4468-4476`), episode line, t80 | Retained slots keep, last tier set to T2; instances swept as today | t80 valid and "settled" on 3 map changes (G0) |
| Loading screen | Lighting gap dark run (`bvh_live.hpp:1912-1917`, `cache_diag.hpp:252-278`) with a map-change episode within 60 presents | Same as map change; no extra action in 1a, a hold on tier-only rebuilds during the dark run in 1b | dark count 0 while idle, one dark run per load screen (G0) |
| Failed fit check | A retained mesh whose revive check fails (counts differ) | That slot dropped, normal build | harness only |
| Failed world fit (later 1c) | exact share under 99% of the first 64 re-sightings in 96 m (`placement_probe.hpp:30,36`) | Instance retention dropped, as the sweep does | placement PASS on same-world reload (G3) |
| Device reset or destroy | `OnDestroyDevicePool` (`bvh_pool.hpp:2072`), `DestroyBvhDeviceData` -> `DestroyLiveStore` (`bvh_resources.hpp:435-449`) | Slots, arenas and retained set cleared by construction (all inside `BvhDeviceData`) | none |
| Panel "reset live store" / `ResetLiveStore` | `bvh_live.hpp:129-136` | Clears retained set | none |

World-space validity rule (for any later instance reuse): a cached instance is valid only if a fresh sighting reproduced its exact (signature, matrix bits); the rebase detector (`placement_probe.hpp:33-40`) must not report a rebase. Unseen instances are not trusted; the existing sweep removes them.

Signal trust: the lighting-gap line and the episode t80 are unvalidated, and the placement verdict rule is being changed. Phase 1a does not depend on the placement verdict (object space only). 1b uses the dark run only to hold tier-only rebuilds. Any instance-level cache (1c) is blocked until the placement verdict is a trusted PASS.

## 6. Rendering-off path

- One session-only switch per feature (like the alpha switches, not saved): `cache_retain` (1a) and `cache_tiers` (1b), both default OFF, atomic bools in a small struct beside `g_cache_diag`.
- OFF must leave these unchanged: `RetireLiveSlots` (no retained marking), `LiveStoreResetReason` (same inputs), `LiveTlasChange` (no new reason), the membership loop at `bvh_live.hpp:675-680` (no tier predicate), `UpdateLiveStoreStats`. Every new branch is behind the switch.
- Harness: extend `TestCacheOnOff` (`.falcom-dev/harness/test_live.cpp:1303-1367`). Today it compares diagnostics on/off with the same code. Before editing, the implementer runs `RunCacheScenario(false)` at HEAD, prints a 64-bit FNV of `tlas_instances`, `tlas_nodes`, `mesh_descriptors` plus `tlas_rebuilds`, `resident`, `meshes_uploaded`, `revision_delta`, and pins those as constants. OFF must reproduce them byte for byte. A second case: switches ON on a scenario with no retirement and no far instance must equal OFF.
- In game, switch OFF: panel TLAS instances, tlas rebuild count and ms, Depth Compare percentages equal the previous build within noise.

## 7. Phased steps (each with a harness test and a BVH-panel-only check)

Step 0b (diagnostic only, no behaviour change; before any cache code):
- Retired-signature set (bounded by `kPoolMaxMeshes`, `bvh_pool.hpp:145`) filled at retirement (`:4457-4461`); at the new-mesh branch (`:4600-4610`) count re-sightings, the retire-to-resight gap histogram (under 120, 600, 1800, 3600, more presents), triangles that would not have been rebuilt, and near misses (same triangle count and quantized bbox, different signature: measures the drift inside one session).
- Size ratio histogram (extent / distance) per tier in `ComputeCacheTiers`.
- Per episode: summed BLAS ms and TLAS rebuild ms next to t80 ms.
- Harness: `test_pool` (retire, recreate same content, count), `test_live` (histogram sums equal tier totals, `Self-check`). Panel: three lines in "Cache diagnostics". Build id `cache-diag-1`.
- In game: Reset diag counters, play two map changes and walk away and back across a street, Dump. Send the `diag/cache_diag_*.json` file and the new lines.

Step 0b as built: a retired mesh is remembered by its signature (`PoolResightTracker`, `bvh_pool.hpp`), capped at `2 * kPoolMaxMeshes` entries with a 7200-present horizon for the sweep; a retirement over the cap is counted as dropped. A new mesh with a remembered signature is a re-sighting; its gap is the presents since retirement, bucketed at 120, 600, 1800 and 3600. A near miss is a new mesh whose signature is not remembered but whose shape (triangle and vertex counts, and bounds rounded to 0.01 m) matches a remembered one: the split separates same-count content from content that only drifts in bounds. Extent is the largest object-space bbox side; the ratio is the largest side of the instance world bounds over `max(Chebyshev distance, 1 m)`, bucketed at 0.0025, 0.005, 0.01 and 0.02. Capture CPU (decode and apply) is timed separately from the queue-to-applied wait in presents, which includes the GPU read-back that is not CPU time. The diagnostics are collected only while "Collect cache diagnostics" is on; the tracker is off with it.

Step 1a (VRAM retention, `cache_retain`, default OFF). Files: `bvh_resources.hpp` (slot fields), `bvh_live.hpp` (retire, revive, reset rules, stats), `bvh_pool.hpp` (scan passes signature and `retainable`; no other pool change), `world_bvh.hpp` (checkbox and counters). Counters: retained meshes, retained MB, revived, revive mismatches, evicted, BLAS builds avoided, triangles avoided.
- Harness (`test_live`): retire a mesh, re-add the same content, expect revive and no BLAS build (`blas_builds` unchanged); different content with equal counts: mismatch and build; budget exceeded: eviction order; store reset clears; OFF equals the pinned HEAD bytes; the GPU check (`CheckLiveBvhOnGpu`) still passes with revived slots (skip retained slots in that check, count them).
- In game: tick "Collect cache diagnostics" and "Tier-aware mesh retention". Walk a loop that leaves and re-enters an area. Read: revived > 0, "BLAS builds" lower than the same route with the switch OFF, Self-check PASS, TLAS instances and Depth Compare unchanged, retained MB at or under the budget.

Step 1b (tier-filtered TLAS content and tier-aware rebuild triggers, `cache_tiers`, default OFF). Files: `bvh_live.hpp` (membership predicate in the loop at `:675-680`, a "tier moved" reason in `LiveTlasChange`), `cache_diag.hpp` (tier edges, hysteresis constants and `CacheInstanceTier` reused, not copied). One predicate function reused by the TLAS loop and the diagnostics.
- Harness: hysteresis (an instance oscillating +-6 m around an edge never changes tier; +-10 m does), T0 content byte-identical to OFF, tier-only rebuild spacing 64 presents, dynamic and wind never thinned, thinned count per tier equals the histogram prediction.
- In game: panel "TLAS instances" falls by roughly the T1/T2 thinned count, "TLAS rebuilds" per minute falls, tlas ms falls, T0 view unchanged (debug trace near, RTAO unchanged). Depth Compare "missing" may rise only beyond 128 m; do not read its overall match percent as a pass.

Later (each needs its own plan and gates): 1c instance retention through a loading screen using the world fit check; 1d two TLAS (near and far) with a second traversal, which needs a trace shader change (17 SRVs, `kTraceSrvCount`, FXC limits unverified, ROADMAP Path 3) and is justified only if T0 holds well under half the instances; 1e BLAS LOD per tier (needs a mesh simplifier).

## 8. Gates (evidence required before each step)

- G0 before Step 0b: none beyond the Phase 0 build installed.
- G0 before 1a (owner data from Phase 0 and 0b): Self-check PASS on at least 3 dumps; episode t80 valid and "settled" on at least 3 map changes; lighting dark count 0 while standing idle 60 s and one dark run per load screen; T0, T1 and T2 all populated in a walking dump (else the 256 m edge is moot).
- G1 before 1a (0b data decides): re-sightings of retired signatures are a meaningful share of new meshes (provisional: at least 20% of new meshes within the hold), count mismatches under 10% of re-sightings (gate counter: `count_mismatch` / `resights`; drift near misses are a raw count, reported and not gated), and summed BLAS ms is at least 15% of t80 ms in an episode. Otherwise 1a is dropped and the work goes to 1b.
- G2 before 1b: 1a validated in game (or dropped by G1), the 0b size histogram reviewed with the owner, thresholds 0.005 and 0.010 confirmed or replaced, and the owner states whether "reduced" is thinning (this plan) or decimation.
- G3 before any instance-level reuse (1c): placement verdict PASS for a same-world reload (exact at least 99%, only-A and only-B at most 1%, `placement_probe.hpp:36`), after the verdict rule change lands and is validated.
- Always: harness full run green (the pre-existing `test_motion.cpp:977` failure is documented, ROADMAP "Known failure"), addon build exit 0, no box ticked until the owner confirms in game.

## 9. Risks and what would prove the design wrong

- Capture or observation dominates refill (G1 fails). Then retention cannot help; keep only 1b.
- Signature drift happens inside a session too (near-miss share high). Then the signature key fails; the content check at revive stays but the benefit is gone.
- A wrong revive (hash collision or drift to a different mesh with equal counts and bbox) draws wrong geometry. Mitigation: counts and bbox check, `verified` only, GPU check button, harness mismatch case. A bitwise compare of the new mesh against the CPU BLAS is not possible because the CPU copy is dropped; accepting the hash plus counts is the trade-off, flagged for the owner.
- Retention raises store resets: retained bytes are inside `used_bytes` and the 2^24-element float-exact limit (`kLiveFloatExact`, `bvh_live.hpp:76`), so too much retention triggers "store full" and loses everything. The budget caps this; watch `live.resets` and `last_reset_reason`.
- T2 is nearly empty because of the region geometry; the 256 m tier then costs code for nothing. G0 checks it.
- Thinning removes far occluders that a future effect needs (reflections, GI). RTAO is unaffected (128 m cap). Keep `cache_tiers` a switch and revisit per effect.
- Hysteresis too wide for a thin T1 (band 8 m on a 128 m tier is fine; revisit if tiers change).
- Tier-only rebuilds every 64 presents add 8 ms hitches; if the trigger fires while the camera moves it can add rebuilds instead of removing them. Check "TLAS rebuilds" per minute against OFF; abandon 1b if it is not lower.
- Lock rule: the tier of a retired mesh is taken in the live layer from the TLAS snapshot (camera read before the pool lock, `bvh_live.hpp:1908`), never inside `CompactPool`, which runs under `g_pool.mutex` and cannot read the camera (`g_state.mutex` order).
- `CheckLiveBvhOnGpu` rebuilds slots from pool meshes and would flag retained slots as unknown; it must skip them.
- Unchanged and out of scope: pool admission, alpha atlas, dynamic/P2b paths, the trace shader, global CMake, the framework.

## Decisions fixed here (the implementer must not change them)

1. In-session only; no disk; signature is the BLAS key, never `uid`, `mesh_id` or `mesh_key`.
2. 1a retains BLAS in VRAM only, no CPU copy, no new free list; eviction by tier then age; alpha, unverified and failed meshes excluded.
3. Hysteresis 8 m at 128 m and 256 m; tier-only rebuild spacing 64 presents; T0 is never thinned.
4. 1b is thinning inside one TLAS, not two TLAS and not decimation.
5. All switches default OFF, session only; OFF pinned byte-identical to HEAD by constants captured before editing.
6. Provisional numbers (32 MB budget, 1800-present hold, 0.005 and 0.010 ratios, 64-sighting fit window) are set by G0/G1/G2 data; they live as named constants in one place.
