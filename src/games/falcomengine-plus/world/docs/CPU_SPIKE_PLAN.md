# CPU cost and spike plan (2026-10-16, planning only, nothing implemented)

Scope: `src/games/falcomengine-plus/world/bvh/` and harness `.falcom-dev/harness/test_live.cpp`. Build id of the dumps: `rt-panel-1`. Owner report: north/south runs, RTAO on, GPU Debug off; the first turn to the south spikes CPU on every test; saw-tooth frame time in a third-party tool. Line refs are the working tree at commit 074f2eac.
Evidence: `renodx-dev\falcomengine-plus\world\{baseline,slack tlas,skip no op}\` (one `perf_*.json` and one `cache_diag_*.json` each; no world_pool.json or world_rtao.json was supplied).

## 1. Data limits (read first)

- A perf file holds only 30 s averages. It has NO per-present values, no max, no p99, no hitch count. Spikes and the saw-tooth cannot be read from it. Indirect evidence is only `cache_diag` maxima (since reset) and the 245-sample `episode` block (slack run only).
- No run was settled: `tlas_instances` grew from about 8k to 25k during the windows (baseline gauge avg 16.5k, diag at f17460: 25.4k; slack episode samples f+281 7.9k, f+1028 16.3k, f+1604 24.1k). The slack window began about 300 presents after a map change (episode serial 18, start f25400). The growth happened in all three runs, so the slack penalty stands as one run per setting and is not settled.
- Frame time (16.2 ms) is larger than cpu_total + gpu_total (about 11.8 ms): the addon is not the whole frame; the game thread and any cap are not measured.

## 2. Per run (ms per present; GPU per measured frame)

| Row | baseline | skip no op | slack tlas |
|---|---|---|---|
| frame_time avg (fps) | 16.197 (61.7) | 16.225 (61.6) | 17.234 (58.0) |
| presents in 30 s | 1853 | 1850 | 1741 |
| tlas_refit (events) | 4.160 (2007) | 4.284 (2263) | 4.176 (2157) |
| tlas_rebuild (events, ms/event) | 1.403 (231, 11.26) | 1.307 (216, 11.19) | 1.057 (117, 15.72) |
| tlas_skip / tlas_apply | 0 / 0 | 0.058 (13) / 0 | 0.036 (5) / 0.813 (505, 2.80) |
| pool_drain_excl_capture | 0.970 | 0.956 | 0.978 |
| pool_hooks (draw time) | 0.391 | 0.383 | 0.381 |
| deform_update_cpu | 0.153 | 0.161 | 0.157 |
| mesh_capture | 0.085 | 0.077 | 0.078 |
| alpha_sync / deform_hook / blas / p2b | 0.024 / 0.011 / 0.004 / 0.003 | same order | same order |
| rtao_record_cpu | 0.039 | 0.038 | 0.037 |
| unattributed_cpu | 0.603 | 0.593 | 1.117 |
| cpu_total | 7.847 | 7.898 | 8.870 |
| gpu_total (rtao 3.29 + filter 0.31 of it) | 3.986 | 4.062 | 4.057 |

Largest rows, baseline share of cpu_total: tlas_refit 53.0 %, tlas_rebuild 17.9 %, pool_drain_excl_capture 12.4 %, unattributed 7.7 %, pool_hooks 5.0 %, deform 2.0 %. Skip: refit 54.2 %, rebuild+skip 17.3 %. Slack: refit 47.1 %, rebuild+skip+apply 21.5 %, unattributed 12.6 %.

## 3. Findings

1. Refit is the biggest steady cost and runs every present: 2007 refits in 1853 presents (1.08 per present, `UpdateLiveBvh` plus `RefitLiveTlasAfterDrain`), 3.84 ms each. Its `copy` phase is 2.73 of 4.16 ms/present (66 %), spent under `g_pool.mutex` (`RefitLiveTlas`, `bvh_live.hpp:1250-1289`: one `lower_bound` in 20k pool instances plus 5 memcpy, visibility and sway per dynamic instance). Cause of the every-present cadence: wind-swayed instances are marked dynamic (`bvh_live.hpp:932`) and `WindSwayPending` (`:504`) keeps it true while sway is on (default ON, `bvh_pool.hpp:1382`). The dynamic count is not in any dump (gap).
2. Saw-tooth source: the TLAS rebuild. 231 rebuilds in 1853 presents = one per 8.02 presents (216+13 skips in 1850 = 8.08). The data matches `kLiveTlasInterval = 8` (`bvh_live.hpp:75`); the gate fires at the floor because admits and mesh landings keep `change` set (reasons since reset: pool changed 277, store changed 85, visibility 8 of 371). A rebuild present costs about 7.4 + 11.3 = 18.7 ms CPU against 7.4 ms for the other seven (non-rebuild avg = (14540-2600)/(1853-231) = 7.36). Amplitude +11.3 ms avg, up to 48.7 ms (max since reset; skip 48.1, slack 67.7). Period 8 to 9 as the owner sees. Capture batches and deform bursts are not periodic (capture 0.085 ms/present).
3. Rebuild phases (cache_diag avg/max ms, baseline): snapshot+lock 7.59/13.7 (60 % of a rebuild), filter+descriptors 1.93/5.4, sort+build+validate 1.88/3.3, buffer create 1.03/34.3, buffer destroy 0.23/0.5. Two parts matter: (a) the snapshot copies every in-region instance (`LiveTlasInstance`, about 260 bytes, 16-25k instances, about 6 MB) under the pool lock with no `reserve` (`bvh_live.hpp:886,913`); (b) one buffer-create outlier per run (34.3, 33.5, 46.4 ms) while the average is 1.0-2.6 ms: a one-off allocation stall, the likeliest part of the first-south-turn spike (more instances than ever before, bigger buffers). Not proven: no timestamps.
4. First turn to the south: the files cannot isolate it. Consistent evidence: `added` 11388 instances in 3008 presents; 239 new meshes (BLAS 239, 15.5 ms total, 0.92 ms max; capture 253.6 ms = 1.06 ms/mesh); `live.mesh_ms.max` 5.6 ms at f17460 but 30.7 ms in the later two dumps (a mesh-sync spike between f17460 and f24114); episode capture bursts up to 32 ms per 24 presents (13 meshes). BLAS is already budgeted (`kLiveTrianglesPerFrame` 16384 per present, 64 meshes). So the burst-driving costs are the rebuild (scales with instance count), the buffer-create stall and one mesh-sync outlier, not capture or BLAS.
5. RTAO: CPU record 0.039 ms/present (0.5 % of cpu_total, in all three runs): not significant. GPU: gpu_rtao 3.29 + filter 0.31 = 3.6 of 3.99 ms (90 % of gpu_total). RTAO is a GPU cost, not a CPU one.
6. Skip no-op: rebuild+skip 1.364 vs 1.403 ms/present, cpu_total +0.05, frame +0.03 ms, max rebuild 48.1 vs 48.7: no effect on average or spikes. Only 13 of 229 gates skipped (5.7 %), and a skip still costs 8.2 ms (snapshot+filter+compare; max 17.8): the snapshot is the floor. Slack: rebuild avg worse (15.7 ms/event; sort+build 5.0 vs 1.9, create 2.6 vs 1.0, destroy 1.4 vs 0.2), 219 of 249 rebuilds were "slack structural" (the dynamic block changed), 505 applies at 2.8 ms; rebuild+skip+apply 1.906 vs 1.403 ms/present and cpu_total +1.02. Slack is one run only, and the growth happened in all three runs, so this cost gap is not settled; there is no sign slack lowers spikes (max rebuild 67.7 ms). Neither switch touches the snapshot or the refit. Keep both OFF.
7. Unattributed CPU rose to 1.117 in the slack run (+0.51): unexplained; the rows do not cover it.

## 4. Performance tab: Max 30 column (owner addition; display only, gated on gather)

- Column: "Max 30" after "Avg 30" in every Performance tab table (`world_bvh.hpp:200-261`: main rows, brackets, phases). Rows: frame_time, every CPU row, unattributed_cpu, every GPU row, cpu_total, gpu_total. Shown as "--" when NaN. The tooltip says: maxima are per row and need not add up to the total's maximum.
- Source: `PerfTabView` gains `max[kPerfCostCount]`, `frame_max`, `unattr_max`, `cpu_max`, `gpu_max`. They are computed in the existing newest-first loop of `UpdatePerfTab` (`perf_run.hpp:620-633`): max over the ring samples (`kPerfTabWindow` = 30), NaN skipped; a GPU row counts only samples with `measured`; a row with no value is NaN. Totals are the max of the per-sample totals (not a sum of row maxima): per sample cpu = DrawTime + InPresent rows + max(0, brackets - InPresent), gpu = sum of that sample's measured GPU rows.
- DRY decision: extract the totals block (`perf_run.hpp:638-671`) into one `PerfTotals(values)` taking a row array with NaN for unmeasured; call it for now, avg, each ring sample, and the run (4 call sites). Also extract the per-present sample build (`:594-607`) into `MakePerfSample(now, prev, frame_ms)`, used by `UpdatePerfTab` and `UpdatePerfRun` (2 call sites, same delta rules).
- Perf-file keys (additive, `schema` stays 1, value/unit objects): per stats row `max_per_present` (CPU rows, subsets, brackets) or `max_per_measured_frame` (GPU rows, null if never measured), over all presents of the 30 s run; `cpu_total`, `unattributed_cpu`, `gpu_total` get the same key. `frame_time` gets `max`, `p99`, `hitches_over_25ms`, `hitches_over_33_3ms` (counts). New top-level `series`: arrays per present of `frame_ms`, `cpu_total`, `tlas_rebuild`, `tlas_refit` (about 1900 numbers each, under 80 KB), so the 8-present period can be read directly. Gauge `tlas_dynamic_instances` (average) is added for finding 1. `PerfRunState` holds the maxima and series; no new lock (`g_perf.mutex` stays a leaf).
- Gate: nothing new runs when gather is OFF (`UpdatePerfTab` and the run counters are already gather-only; ring, maxima and series stay empty; keys are null). No new clock read or atomic. No rendering, TLAS or pool change; fingerprint `0x55593b70e070e7fe` unchanged.
- Tests (`test_live.cpp`, CRLF): extend `TestPerfTab`: 30+ synthetic presents with one spike, expect it as Max for a CPU row, frame, cpu_total (per-sample) and unattributed (clamped samples), and gone after 31 more presents; a GPU row measured in some presents (max over measured only); a never-measured row NaN; NaN frame skipped. Extend `TestPerfWindow` (hand-derived): one 40 ms present gives `frame_time.max` 40, hitch counts 1, `max_per_present` equals the spike, `series` length equals presents. `TestPerfJsonKeys`: the new keys exist. `TestPerfGateOff`/`TestPerfTimerGate`: gather OFF leaves view max NaN and keys null. `TestPerfUnattributed`: the shared totals helper keeps its results.

## 5. Options to reduce CPU and spikes (decisions fixed here; implementer makes none)

- O1 Refit copy without the per-instance search. Keep a vector of pool indices parallel to `tlas_info[df..]`, rebuilt by `RebuildLiveTlas`; `RefitLiveTlas` checks `instances[idx].id == copy.id` and falls back to `lower_bound` on a mismatch (always correct). Output byte-identical. Gain: estimated 0.5-1.2 ms/present of the 2.73 ms copy phase (not proven; scales with the dynamic count). Risk low. Default ON (identical TLAS). Proof: `test_live` refit tests unchanged plus a case that retires an instance between rebuild and refit; fingerprint pin; in game `refit_phase_0` and `tlas_refit` fall, `max_per_present` of tlas_refit falls.
- O2 Cheaper snapshot. `snapshot.reserve(g_pool.instances.size())` before the loop at `bvh_live.hpp:907`, and keep the four large vectors (snapshot, info, instances, leaves) as scratch members reused across rebuilds (no page faults, no regrowth copies). Output identical. Gain: estimated 1-3 ms per rebuild (0.1-0.4 ms/present, and a lower rebuild peak and shorter pool-lock hold, which also helps `pool_hooks`); unproven until `rebuild_phase_0` is read. Risk low. Default ON. Proof: `TestRebuildPhases`, fingerprint pin, in game `rebuild_phase_0` avg and `max_per_present` of tlas_rebuild.
- O3 Wind sway refit every second present (sway poses update at half rate). Visible change (sway AO and shadow stepping), so an owner decision: default OFF, session-only switch next to "Wind sway". Gain: about half of tlas_refit, 1.5-2.1 ms/present. Risk medium (visual). Proof: harness case that the refit runs on alternate presents and the TLAS equals the every-present result at even presents; in game A/B with `tlas_refit` and a sway clip.
- O4 Split a rebuild over two presents: snapshot (and filter) at present N, sort, build and upload at N+1; commit records the snapshot's revisions so later changes are caught. Adds one present of latency for new objects (nothing waits for a batch). Gain: no average change; rebuild peak about 11-13 ms falls to about 6-9 ms (halves the saw-tooth amplitude). Risk medium-high (state across presents, lock rule: snapshot lock released before any graphics call). Default OFF, session-only. Proof: `TestSkipCadence`-style cadence test, fingerprint pin with the switch off, in game Max 30 and `series`.
- O5 Remove the buffer-create stall only if `series` shows it on the first south turn: reuse the four TLAS GPU buffers when the new size equals or is smaller than the old (WriteBufferRange into the existing buffer) and pre-size nothing else. Exact-size buffers are required because the shader reads counts from buffer sizes (TLAS_PLAN section A.3), so no capacity trick without a shader change; do not do that. Gain: up to 34 ms on one present. Defer until the data proves it.
- Not recommended: more skip/slack work (finding 6), limiting admissions or capture (capture 0.085 ms/present, BLAS already budgeted), RTAO CPU work (0.039 ms/present).

## 6. Recommended first change

Round 1 = section 4 (Max 30 column, perf-file maxima, p99, hitch counts, series, `tlas_dynamic_instances` gauge). Why first: it is diagnostic only (no default or rendering change, owner asked for it), and the current files cannot say which row spikes at the south turn, how big the dynamic block is (decides O1 and O3), or whether the saw-tooth period is exactly 8. Round 2 = O2 then O1 (both byte-identical, default ON, one per round). O3 and O4 only after the owner sees Max 30 and decides.

## 7. Owner test (in game, DevKit build, after each round)

1. Performance tab: Gather performance metrics ON. Let the map settle (no map change for 300 presents). Reset diag counters.
2. Run north then south three times. On the first south turn read the Max 30 column and note which rows spike (screenshot the tab at the peak). Then press "Performance test (30 s)" during a repeated north/south run; send `diag/perf_*.json`.
3. Press Dump cache diagnostics twice; send `diag/cache_diag_*.json`. Send the tab screenshot for each A/B (baseline, then each switch).
4. Compare per option: `cpu_total` avg and `max_per_present`, `frame_time.max`/`p99`/hitch counts, `series` period.

## 8. Build and harness (every round; CRLF for `perf_run.hpp`, `bvh_live.hpp`, `test_live.cpp`; LF for `world_bvh.hpp` and docs)

- `powershell -File E:\RenoDX\renodx\.falcom-dev\harness\run_win_some.ps1 -Tests @("test_live","test_rtao","test_pool","test_build")`, then `-All` (known red: `test_motion.cpp:977`). WSL form: `pwsh.exe -File` with the same arguments.
- Addon build in vcvars64: `cmake --build E:\RenoDX\renodx\build --config Release --target falcomengine-plus`.
- A check counts only with its real output shown. Lock rule unchanged (no graphics call under a module mutex; `g_perf.mutex` a leaf). No framework, CMake or shader change. No default rendering change without an owner decision (O3, O4 default OFF). After each round add a status block to `ROADMAP.md` (not validated in game, no box ticked).
