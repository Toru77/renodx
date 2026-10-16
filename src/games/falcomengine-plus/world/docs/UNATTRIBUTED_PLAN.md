# Plan: attribute the unattributed CPU spikes (diagnostic only, no output change)

Tree at 82ad30ca. Line numbers are that tree. Reads: PERF_INVESTIGATION.md section 5, AGENTS.md (root, falcomengine-plus), the files named below. Nothing here touches rendering, TLAS, pool or revisions.

## 0. What the code does today (new finding, drives the lag fix)

`UpdatePerfTab`/`UpdatePerfRun` run inside `UpdateCacheDiag` (S_N, `bvh_live.hpp:2319-2320`), mid `OnWorldPresentBvh` (`bvh_debug.hpp:205-250`), after `UpdateLiveBvh` and before `UpdateDeformLive`, `RunBvhDebugPass` and all of `OnWorldPresent`. Rows in one sample come from different presents:
- lag 0: rebuild, skip, apply, BLAS, alpha sync (UpdateLiveBvh of N), RTAO record.
- lag 1: both brackets (`PoolCpuTimer` adds on exit), `deform_update_cpu`, `p2b_expand_cpu`, the after-drain refit.
- lag 2: `pool_drain_excl_capture`, `mesh_capture`: `DrainPoolScan` exchanges `cpu_drain_us` at its START (`bvh_pool.hpp:5718-5721`), so it publishes the previous drain's duration, read one present later. `pool_hooks` and `deform_hook_cpu` lag 1.
So a drain sweep (RetirePoolOrphans, `i % 10`) shows as +N ms unattributed one present, then as a negative raw value (clamped to 0) one present later. The 10 to 13 ms spikes are therefore partly bookkeeping, partly real blocks that have no row (section 2).

## 1. Lag fix (item 3). Decision: sample at the START of the next present, all rows over one window

1. New function `SamplePerfPresent(device, extra)` in `bvh_live.hpp` right before `UpdateCacheDiag`. It is the code MOVED out of `UpdateCacheDiag` (lines 2290, 2309, 2311-2320: the gather read, `ReadPerfCounters`, `UpdatePerfTab`, `UpdatePerfRun`), not new logic. Called once from `OnWorldPresentBvh` after `rtao_startup_enable` and BEFORE `UpdateLiveBvh` (`bvh_debug.hpp:232`). The `PerfExtraGpu extra` block (`bvh_debug.hpp:233-244`) moves up with it. `UpdateCacheDiag` loses the `extra` parameter and keeps `present_time`/`d.last_present` for the cache dump unchanged. `UpdatePerfRun` is still called every present (gather off or on); `UpdatePerfTab` only when gather is on.
2. At that point both brackets of present N (Bvh and World handlers) have run their destructors, and every InPresent row of N is final. The sample for present N is therefore taken at the start of present N+1 and describes present N completely. Its frame interval is `now - previous sample` (new `PerfTabState::last_sample`, cleared with the tab, touched only on the present thread, read/written under `g_perf.mutex` briefly, then `UpdatePerfTab`/`UpdatePerfRun` called with no lock held). Series entry i = the interval that ENDS at sample i and the work of the handlers inside it: a hitch in `frame_ms[i]` pairs with the blocks of `series.blocks[*][i]`. Document this in the perf file as `"sample_alignment": "work of present N, sampled at the start of present N+1"` (additive string).
3. Move the four frame-value exchanges into `SamplePerfPresent` (gather only): `cpu_hook_us`, `cpu_drain_us`, `cpu_capture_us` out of `DrainPoolScan` (`bvh_pool.hpp:5718-5722`, delete the block), `g_deform_live.cpu_us` out of `UpdateDeformLive` (`deform_live.hpp:1004`, delete). `ClearPerfGathered` keeps zeroing the `_frame_us` fields. Every row is then a delta over the same window as the frame interval. Side effect: `UpdateCacheDiag` `delta.capture_ms` (`bvh_live.hpp:2372`) now holds the previous drain instead of the one before; diagnostic only, state it in the commit.
4. The reported `start_frame/end_frame` are `g_state.frame` at the sample call (one higher than before). State it in the file note. Not an output change.

## 2. Blocks to instrument (item 1, owner list). Key, site, cost in principle

Mechanism: `g_perf_gate.block_us[kPerfBlockCount]` (array of `std::atomic_uint64_t`, `perf_gate.hpp`), written by the existing `PoolCpuTimer{&g_perf_gate.block_us[k]}` (armed only if gather ON: no clock read OFF). Two sites need a subtraction and use one reused helper `AddPerfBlockMs(k, ms)` (gather-guarded, ms from `LiveElapsedMs(PerfNow())`). No new lock. Each block is a new `kPerfCost` entry appended after `kPerfSlackPhase3` (file key order of existing rows unchanged), `cumulative=true`, `ReadPerfCounters` reads `PerfMsOf(block_us[k])`.
InPresent (disjoint, summed, they REDUCE unattributed; what is left is the true residue):
1. `perf_sample`: `SamplePerfPresent` itself (a PoolCpuTimer inside it). Counted in the NEXT sample (explicit lag by design). Locks plus series push per present.
2. `sync_live_meshes`: `bvh_live.hpp:2145-2147`, minus the BLAS time inside (`blas_build_ms_sum` delta, `AddPerfBlockMs`). ScanLiveMeshes under the pool lock, retire, vertex/index uploads. Spikes while meshes stream in.
3. `rebuild_noop_tests`: `bvh_live.hpp:1145-1154`, reuse the existing `diag_time` (`AddPerfBlockMs`). Two O(N) compares of about 15k instances, Collect ON, every rebuild.
4. `record_tlas_rebuild`: `cache_diag.hpp:439` (PoolCpuTimer after the line-441 early return; called `bvh_live.hpp:1056, 1070`). Two sorts of about 15k pointers, `CacheEnteredIds`, a `fresh_ids` set over all pool instances while the episode is open (`:469-481`). Collect ON, every rebuild.
5. `gpu_check`: `bvh_live.hpp:2188-2191` (timer inside the `if`). Wait plus readback; on request only.
6. `cache_diag`: all of `UpdateCacheDiag` (`bvh_live.hpp:2241`, top). Locks and map build; the tiers are block 14.
7. `rtao_stats`: `rtao.hpp:477`, after the `requested.exchange` early return. Readback stall; on request only.
8. `debug_pass`: `bvh_debug.hpp:113`, after the view check. CPU recording of the GPU Debug view; may nest draw hooks (a small double count while the view is on; note it).
9. `resolve_indirect`: `draw_census.hpp:395`, after `pending.empty()` return. `flush_immediate_command_list()` plus `queue->wait_idle()` = a CPU stall until the GPU finishes the frame. Every present while scan-indirect has pending draws (default ON). Prime suspect for 10 ms spikes: its cost equals the GPU work still queued.
10. `p2b_record`: `bvh_live.hpp:2062-2106` (timer after the `enabled` check). `dynamic_expand_cpu_ms` stops at line 2041, so `EnsureDynamicExpand`, the draw-table write, barriers, dispatch and descriptor update are unmeasured.
11. `p2b_diag`: `p2b_diag.hpp:454`, after the `requested` check. Request only.
12. `deform_probe`: `deform_probe.hpp:973`, after the `enabled` check. Every present with deforming meshes ON (RTAO enables it): staging copy and `ResolveDeformSlot` map/decode (possible GPU wait).
Subset (parts of an attributed row, never summed; role `kPerfSubset`, shown in the collapsed section, renamed "Subsets (parts of the rows above; not in the totals)"):
13. `rebuild_call`: whole `RebuildLiveTlas` (`bvh_live.hpp:869`, top) including the destructors of the snapshot vectors after `finish()`. `rebuild_call - tlas_rebuild - rebuild_noop_tests - record_tlas_rebuild` is the post-timer tail; the file carries all four.
14. `cache_tiers`: `bvh_live.hpp:2282-2288` and `:2308` (`ComputeCacheTiers`, `cache_diag.hpp:130`), every 30 presents or on a dump. Inside `cache_diag`.
15. `drain_compact`: `bvh_pool.hpp:5752-5756` (CompactPool and the four Apply*Switch, under the pool lock; per present).
16. `drain_resolve`: `ResolvePoolSlot` call `:5773` (staging map, decode; contains `mesh_capture` and `follow_scan`).
17. `drain_prune`: `PrunePool` `:5785`, every 60 presents.
18. `drain_retire_orphans`: `PoolNewestSeenByKey` plus `RetirePoolOrphans` `:5802-5804` (builds a map over all 18 to 26k instances, every 10 presents).
19. `drain_pool_stats`: `UpdatePoolStats` `:5807` (`:1970`, loops pool, slots, meshes, keys, every present).
20. `follow_scan`: the `for (WorldInstance& instance : g_pool.instances)` scan of `FollowPoolMovingInstance`, `bvh_pool.hpp:2884` (called from `:5663`), timer scoped to the loop, after the early returns.
Already attributed and left alone: `tlas_rebuild/skip/apply/refit` (incl. the after-drain refit), `blas_build`, `alpha_sync_cpu`, `p2b_expand_cpu` (first half only), `deform_update_cpu`, drain and capture rows, hooks.

## 3. Per-present sample, max, series, over-2 ms count (item 2)

- Blocks flow through the existing `PerfCounters` -> `MakePerfSample` -> `PerfMaxima` (`MaxInto`), so per-block max per present and the tab need no new path.
- `PerfSeries` gets `blocks[kPerfBlockCount]` (vector<double>, only the 20 new rows, same capacity rule, reserved at run start). `PerfRunState` gets `uint32_t over_2ms[kPerfBlockCount]` (sample value > 2.0 ms) and `PerfSpike spike` (see 4). Written under the existing `g_perf.mutex` in `UpdatePerfRun`.

## 4. Spike attribution (item 4)

- `PerfSpike` {run presents index, frame, frame_ms, unattr, value[kPerfBlockCount]}: in `UpdatePerfRun` replace it when `totals.unattr > run.max.unattr` (compare before `MaxInto`).
- Tab: `UpdatePerfTab` already scans the ring for `max.unattr`; also keep that sample's block values and its age in `PerfTabView` (`spike_age`, `spike_unattr`, `spike_value[]`). `world_bvh.hpp` (LF) prints ONE line under the table: `Largest unattributed in the last 30: 11.2 ms, 3 presents ago = resolve_indirect 9.8, cache_diag 0.9 (top 4 InPresent blocks), unexplained 0.5`. Unexplained = unattr minus the sum of InPresent blocks (with the lag fix it is genuine residue). No counts, no control.

## 5. JSON (item 5): additive keys, `"schema": 1` unchanged (`WritePerfFile`, `perf_run.hpp:394`)

All via `WritePerfValue` (value and unit): `stats.<new key>` (automatic from the table: avg, max, total, in_total, phase), `series.blocks.<key>` arrays, and a top-level `"attribution": {"threshold_ms": 2, "blocks": {key: {"max_per_present", "avg_per_present", "presents_over_2ms", "role", "inside"}}, "worst_present": {"index", "frame", "frame_ms", "unattributed_ms", "blocks": {key: ms}, "explained_ms", "unexplained_ms"}}`, plus `"sample_alignment"` (item 1.2). Existing keys, their meaning and order stay.

## 6. Cost of each Ray Tracing inclusion setting (owner addition)

Rule: LIVE = a gated stopwatch exists or can exist without a per-draw clock read. A/B = only two runs (switch ON vs OFF, same route, same 30 s, compare `frame_time`, `cpu_total`, rows) can give it. Files record `inclusion_switches` so a pair is self-describing. No new control.
| Setting | LIVE rows (summed in the file) | A/B only |
|---|---|---|
| Alpha foliage (+ indirect source, wind opaque) | `alpha_sync_cpu`, `gpu_alpha_blit` | admission/UV capture at draw time (inside `pool_hooks`, per draw: no stopwatch); alpha instances in TLAS (share of `tlas_rebuild`/`tlas_refit`); any-hit cost inside `gpu_rtao`. Estimate (labelled, live): `tlas_rebuild + tlas_refit` avg x `tlas_alpha_instances / tlas_instances` |
| Foliage motion (wind sway) | none exact; `refit_phase_0` (the sway maths loop) and `tlas_refit` | exact sway cost = refit with sway OFF (`WindSwayPending` false). Estimate (live): `refit_phase_0` avg x `tlas_wind_instances / tlas_dynamic_instances`; wind pass counters in `world_pool.json` |
| Deforming meshes | `deform_hook_cpu`, `deform_update_cpu`, `deform_probe`, `gpu_deform_refit` | their instances' share of TLAS and trace |
| Follow and moving objects | `follow_scan` (per moving sighting, not per draw), `p2b_expand_cpu`, `p2b_record`, `gpu_p2b_expand` | exclude-moving (admission and TLAS size effect), P2b TLAS exclusion savings (`tlas_flagged_skipped`), trace cost of expanded draws |
| Scan indirect | `resolve_indirect` (wait_idle included) | indirect capture staging at draw time (inside `pool_hooks`) |
Implement as one `kPerfSettingCost[]` table in `perf_run.hpp` (name, live row indices, estimate kind, A/B text), a `"settings_cost"` object (switch state, `live_cpu_avg`, `live_gpu_avg`, `estimate`, `ab_only` text) and `"inclusion_switches"` in the file, and a collapsed header "Inclusion settings (live costs; A/B parts in the file)" on the tab, table only (Setting, CPU avg, GPU avg). Add two gauges (`PerfGauge`, `kPerfGaugeNames`): `tlas_alpha_instances` (`live.tlas_alpha_instances`, `bvh_resources.hpp:188`) and `tlas_wind_instances` (`data->tlas_wind_count`). Decision for the owner: if a table on the tab counts as too much, drop the header; the file keeps it.

## 7. Tests (item 6), `test_live.cpp` in `.falcom-dev/harness` (LF, as that file is)

- `TestPerfBlockGate`: gate OFF, 4 presents of `RegionPresent` + `UpdateCacheDiag`: every `block_us[k]` is 0 and `AddPerfBlockMs` adds 0; a `PoolCpuTimer` over a 2 ms sleep adds 0. Gate ON, 12 presents: `block_us[cache_diag] + block_us[sync_live_meshes] > 0`; a timed 2 ms scope adds >= 1000 us to only its key.
- `TestPerfLagFix`: prime with `SamplePerfPresent`, then add 10 ms to `block_us[resolve_indirect]` and 10 ms to `present_world_us`, call it again: that sample has `unattr_raw` within 1e-3 and `cpu_now` 10; the next sample (2 ms bracket, no block) has unattr 2.0, no negative carry. Also set `cpu_drain_us` 5000: the same sample shows the drain row 5 ms and `cpu_drain_us` is 0 afterwards; `DrainPoolScan` with gate ON no longer exchanges.
- `TestPerfAttribution`: a run window where present k has blocks 10 ms and 3 ms and a 14 ms bracket: `spike` holds k with the 10 ms block largest, `view.spike_value` agrees, JSON `attribution.blocks` max 10 / 3, `presents_over_2ms` 1 / 1, `explained_ms` 13, `unexplained_ms` 1.
- Update the tests that expect `UpdateCacheDiag` to feed the tab (`test_live.cpp` 2840, 2841, 2956, 2983; check the other 12 call sites): call `SamplePerfPresent` first. Extend `TestPerfJsonKeys` (keys `attribution`, `settings_cost`, `inclusion_switches`, `sample_alignment`, schema still 1, value/unit counts equal). `TestPerfGateOff` must still see `_frame_us` untouched.
- Byte-identical: `TestCacheOnOff` pins `0x55593b70e070e7fe` and the TLAS/descriptor/revision equality; they must pass unchanged.

## 8. Risks

- Clock reads: only when gather is ON, two per timer, about 12 InPresent + 8 Subset blocks per present, most behind an early-return gate; roughly 20 pairs (under 2 us) per present. None per draw. `follow_scan` is per moving followed sighting (tens per frame), not per draw of the pool; A/B is the fallback if that is unwelcome.
- `wait_idle` in `resolve_indirect` will show GPU wait as CPU time; that is the point, it must not be "optimised" in this round.
- The tab and file lose the old one-present misalignment: old and new perf files do not compare per present (averages still compare). `start_frame` shifts by one.
- Subset blocks are not additive; sync_live_meshes subtracts BLAS to avoid double counting; `debug_pass` may nest draw hooks. If any InPresent block nests in another InPresent row, `cpu_total` over-counts: check `unattributed raw_total` stays >= 0 after the change.
- Moved exchanges are the only edits outside diagnostics code paths; the rest is stopwatches. No lock added, no graphics call under a lock.

## 9. Build, run, line endings

- Build (Windows, in vcvars64): `cmake --build E:\RenoDX\renodx\build --config Release --target falcomengine-plus`.
- Harness (PowerShell, from `E:\RenoDX\renodx\.falcom-dev\harness`): `powershell -File run_win_some.ps1 -Tests @("test_live","test_pool","test_build")`, then `powershell -File run_win_some.ps1 -All`. Known red: `test_motion:977`. It refreshes the world copy itself. A pass counts only with its printed `PASS (0 failures)` / `RESULT` lines.
- CRLF for every `.hpp`/`.cpp` edited (`perf_run.hpp`, `perf_gate.hpp`, `bvh_live.hpp`, `bvh_debug.hpp`, `bvh_pool.hpp`, `cache_diag.hpp`, `deform_live.hpp`, `deform_probe.hpp`, `p2b_diag.hpp`, `rtao.hpp`, `draw_census.hpp`); LF for `world_bvh.hpp` and docs. Correction: `test_live.cpp` and `ROADMAP.md` are LF (index and working copy, byte-checked), not CRLF. Check with `git ls-files --eol` before and after. Do not touch other `.hpp`; update ROADMAP.md only after the owner's run.
- In-game test: Gather ON, Collect cache diagnostics ON (default), RTAO ON, settled map, press "Performance test (30 s)". Send back the perf file's `attribution` block, `settings_cost`, and the tab's spike line. Then A/B runs with Collect OFF and with Scan indirect OFF.
