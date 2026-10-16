# Performance tab rows and the "Gather performance metrics" switch (plan only, nothing implemented)

Base: commit 433f2d55 plus PERF_COST_LIST.md (approved). Line numbers are from that tree. Files: CRLF for .hpp/.cpp/.h (addon.cpp keeps its BOM), LF for `world_bvh.hpp` and docs.

Revision 2 (owner decision): performance timings appear ONLY on the Performance tab. Every other on-panel timing readout is removed (section 1b). Timings stay in the JSON dumps (section 1c). The switch gates the timings at the source, including the ones in the dumps (section 1d). Everything else in this plan is unchanged except where sections 1, 9 and 10 say so.

## 0. Facts the plan depends on (read from the code)
- Present order: `OnWorldPresentBvh` is registered first (`world_bvh.hpp:47`, via `bvh::Use` at `world.hpp:45`), then `OnWorldPresent` (`world.hpp:69`). Per present: BVH update + `UpdateCacheDiag` + `UpdateDeformLive`, then drain, P2b expand, refit-after-drain. So drain, capture, deform and the brackets are never frame-aligned with each other in one `UpdateCacheDiag` call.
- Draw-time costs run OUTSIDE both present handlers: pool hooks (`bvh_pool.hpp:4068,4240`), deform capture hook (`deform_live.hpp:453`), RTAO command recording (`addon.cpp:9223`, from the lighting-draw hook `:9293`). They must not be subtracted from the present stopwatches.
- All six GPU timers go through `BeginGpuTimer` (`gpu_timer.hpp:119`); that is the only place a query is created or polled.
- `g_cache_diag.enabled` (Collect) defaults to true. Collect-only data (`RecordCacheTlasRebuild`, episodes, `work.*_ms`) reads the same `ms` values.

## 1. The switch
- New header `world/bvh/perf_gate.hpp` (CRLF, included by `bvh_pool.hpp` and `gpu_timer.hpp`): `struct PerfGate { std::atomic_bool gather{false}; std::atomic_uint64_t present_world_us, present_bvh_us, alpha_sync_us, rtao_record_us, deform_update_us; }; inline PerfGate g_perf_gate;` and `PerfNow()` = `gather ? steady_clock::now() : time_point{}` (reused at 9 sites). Session only, never saved, default false. `refresh_world.ps1` copies every `.hpp`, so the harness sees it.
- `PoolCpuTimer` (`bvh_pool.hpp:1253`): add `const bool armed = g_perf_gate.gather.load(relaxed)`; `start = armed ? now : {}`; the destructor returns when `!armed`. OFF cost: one relaxed load, no clock read, no atomic write. Covers all existing and new CPU stopwatches.
- `LiveElapsedMs` (`bvh_live.hpp:99`): return 0 when `start` is the epoch value. Replace `steady_clock::now()` with `PerfNow()` for `start`/`phase_start` in `SyncLiveMeshes:465`, BLAS `:286`, `TryApplyLiveTlasSlack:686,692`, `RebuildLiveTlas:873,880`, `RefitLiveTlas:1228,1240`, `ExpandLiveDynamic:2002`, `RefitLiveTlasAfterDrain:2203`. Every `lap` then reads no clock. The Collect diagnostic start shift (`:1140`) stays Collect-gated.
- `BeginGpuTimer` (windows variant): first line `if (!g_perf_gate.gather.load(relaxed)) return;`. OFF: no query created, none polled. `EndGpuTimer` already no-ops when `open < 0`. No new GPU timer is added (nothing to gate on the tab).
- Checkbox "Gather performance metrics" is the first widget of `DrawPerformanceTab` (`world_bvh.hpp:178`), tooltip says it costs a few clock reads per present and changes no rendering. On click ON (UI thread, no lock held while calling ImGui): store 0 to `g_pool.cpu_hook_us/cpu_drain_us/cpu_capture_us/cpu_hook_frame_us/cpu_drain_frame_us/cpu_capture_frame_us` and `g_deform_live.cpu_us/cpu_frame_us` (so stale values from before OFF are not read), then under `g_perf.mutex` `g_perf.tab = PerfTabState{}` (next present re-primes). OFF click: only the store.
- OFF tab body: the checkbox plus one `TextDisabled` line "Metrics are off. Switch on to measure. Dumps written while off carry no timings (see perf_gather in the file)." Everything else (table, button, run status, result line) is hidden.
- The BVH panel and the RTAO panel no longer show any timing (section 1b), so there is nothing else to switch to a "metrics are off" line. (Replaces the earlier idea of a single disabled line in `DrawBvhPanel`.)

## 1b. Removal list (panel text, exact labels and lines at 433f2d55)
Principle: only switches, controls and result lines stay outside the Performance tab. The implementer deletes exactly the rows below, then greps `\bms\b` and `%.*f ?ms` in `world_bvh.hpp`, `rtao_panel.hpp` and `world_settings.hpp`; the only hits left may be `PerfMsText`, `DrawPerformanceTab` (tab text and tooltip) and comments. Show that grep output in the report.

`world/bvh/world_bvh.hpp`, `DrawBvhPanel`:
| Lines | Label removed |
|---|---|
| `:410-418` | Locals `stats`, `tlas_timing`, `gpu` and their `g_pool.mutex` copy block (their only readers are `:424-425`, `:502-513`). One pool-lock acquisition less per drawn frame. |
| `:419-423` | Local `deform_refit_ms` and its `g_deform_live.mutex` block (only reader `:514`). One deform-lock acquisition less. |
| `:424-425` | "Alpha and moving-object ms: alpha blit %.3f, alpha trace %.3f, expand cpu %.3f, expand gpu %.3f" |
| `:495-496` | "CPU: meshes %.2f ms, TLAS %.2f ms (refit %.2f ms)" |
| `:497-501` | Local `CacheDiagData diag` and its `g_cache_diag.mutex` copy (readers `:509`, `:534-536` only). |
| `:502-506` | "TLAS CPU ms: rebuild avg / max, skip avg / max, apply avg / max, refit last / max" |
| `:507-509` | "Build/capture CPU ms: BLAS avg / max (since reset), capture total" |
| `:510-511` | "Rebuild cost by cause (ms): retire, admit, other" |
| `:512-513` | "Refit phases (ms): copy / desc / nodes+validate / upload" |
| `:514` | "GPU: deform refit %.3f ms, trace %.2f ms" |
| `:515-518` | "Pool CPU per frame: hooks, drain, deform" |
| `:534-535` | "Frame: last %.2f ms, avg %.2f ms (since reset)" |
| `:536` | The `CacheTlasPhaseLine(diag)` line ("TLAS rebuild phases (ms) avg/max/last: ..."). The function stays: `CacheDiagLines` (`cache_diag.hpp:616`) still uses it for the clipboard text and the JSON `lines` key. It becomes a single-use helper, which AGENTS.md discourages; it is pre-existing and not touched here (inlining it is an unrelated cleanup, optional follow-up). |
| `:529-530` | Comment reworded: "Counts and timings are in the dump (timings when Gather performance metrics is on); costs are on the Performance tab." |

`world/rtao/rtao_panel.hpp`, `DrawRtaoPanel` (the Ray Tracing tab, RTAO status row):
| Lines | Label removed |
|---|---|
| `:28-33` | "RTAO pass A + B: %.3f ms", "RTAO pass A: ...", "...: no measurement yet" and the local `passes`. |
| `:37-41` | "Spatial filter: no measurement yet" and "Spatial filter: %.3f ms". |
| `:42-44` | "Total: %.3f ms". |
The "Spatial filter: not run (filter_failed / debug modes 1 to 7 bypass it)" text at `:36` is a status, not a timing: it stays. The block becomes `if (frame.filter_requested && !frame.filter_ran) ImGui::Text("Spatial filter: not run (%s)", ...);`. `frame.gpu_ms` and `frame.filter_gpu_ms` stay in `RtaoFrameState` (the JSON and the ReShade.log stats line at `rtao.hpp:494` still use them).

Distance Fade: searched, no timing text exists there (`world_settings.hpp:419,432` are sliders; the "Fade: effective start ..." line at `rtao_panel.hpp:48` is a distance, not a timing). Nothing to remove; the request is covered by the RTAO rows above.

## 1c. What stays on the panel, and the JSON
Remaining BVH panel: Pool Scan, Dump Pool JSON, Reset, the pool lines that are counts, Live BVH, Rebuild BVH, Skip no-op, Slack, GPU Debug, Hide camera-hidden, "Errors recorded", "GPU contents: match/MISMATCH", inspect and depth-compare lines, Collect cache diagnostics, Label, Dump cache diagnostics, Reset diag counters, the result line, the placement probe. Remaining RTAO panel: status, reason, counts, fade, texture sizes, temporal and motion rows, filter configuration, discovery counts. The Performance tab holds every timing.

Decision: keep every removed timing in the JSON dumps (recommended; the owner asked for the rest to be in JSON). Where each removed line lives now:
| Removed panel line | JSON (already there, unchanged key) | Lost? |
|---|---|---|
| Alpha and moving-object ms | `world_pool.json`: `expand_cpu_ms`, `expand_gpu_ms` (`bvh_pool.hpp:6166-6167`), `blit_ms`, `trace_ms` (`:6833-6834`) | no |
| CPU: meshes / TLAS / refit | TLAS and refit last: `world_pool.json` `tlas_timing.rebuild_ms.last`, `refit_ms.last` (`:7075,7078`). Meshes (`mesh_ms_last`, `mesh_ms_max`): in NO file | YES, meshes |
| TLAS CPU ms rebuild avg/max | `cache_diag.json` `tlas.ms_sum/ms_max` (Collect only), `world_pool.json` `tlas_timing.rebuild_ms` last/max/sum, perf file `tlas_rebuild` | no |
| TLAS CPU ms skip avg/max | `cache_diag.json` `tlas.noop.skip_ms` is Collect-gated and has no max; perf file `tlas_skip` mean only. `tlas_skip_ms_max` and the always-on sum: in NO dump | YES, skip max and always-on sum |
| TLAS CPU ms apply avg/max | `cache_diag.json` `slack.apply_ms_sum/max` | no |
| TLAS CPU ms refit last/max | `world_pool.json` `tlas_timing.refit_ms` | no |
| Build/capture CPU ms | `cache_diag.json` `blas.ms_sum/ms_max`, `capture.cpu_ms` | no |
| Rebuild cost by cause | `world_pool.json` `tlas_timing.rebuild_cause.ms` | no |
| Refit phases | `world_pool.json` `tlas_timing.refit_phase_ms` (last), plus the new phase sums in the perf file (section 6) | no |
| GPU: deform refit, trace | trace: `cache_diag.json` `gpu.trace`; deform refit: perf file `gpu_deform_refit` (window mean). The "last single sample" is not dumped | immaterial |
| Pool CPU per frame | perf file `pool_hooks`, `pool_drain`, `deform_cpu` (window means). No instantaneous value | immaterial (the tab shows "now") |
| Frame: last / avg since reset | perf file `frame_ms` avg (window only). The since-reset value (`CacheDiagData::frame_ms_*`) is in NO dump | YES |
| TLAS rebuild phases line | `cache_diag.json` `tlas.phase_ms` (names, last, sum, max) and the `lines` key | no |
| RTAO pass / filter / Total ms | `world_rtao.json` `gpu_ms` and `spatial_filter.gpu_ms` (`rtao.hpp:408,461`); Total is the sum of the two; perf file `gpu_rtao`, `gpu_rtao_filter` | no |

So the file WOULD lose data for three readouts if nothing is added: meshes ms, skip max and always-on skip sum, and the since-reset frame time. Fix, additive keys only (schema numbers stay: `cache_diag` 4, `world_pool` 15, perf 1, rtao 2; `test_live.cpp:1395` checks `"schema": 4` and stays green):
- `cache_diag.json` top level, written in `WriteCacheDiagFile` (`cache_diag.hpp:831-833` area): `"perf_gather": true|false`.
- `cache_diag.json` `live` block (`cache_diag.hpp:958-960`): `"mesh_ms": {"last": live.mesh_ms_last, "max": live.mesh_ms_max}`, `"tlas_skips": live.tlas_skips`, `"skip_ms": {"sum": live.tlas_skip_ms_sum, "max": live.tlas_skip_ms_max}`.
- `cache_diag.json` top level: `"frame_ms": {"last": d.frame_ms_last, "avg": intervals ? sum/intervals : -1, "intervals": d.frame_intervals}` (`d` is already copied at `:822-826`).
- `world_pool.json` (`bvh_pool.hpp:6790`, after the schema line) and `world_rtao.json` (`rtao.hpp:405`, new `RtaoJsonInput::perf_gather` set where `MaybeCaptureRtaoStats` fills `in` at `:496-503`): `"perf_gather"`, because these dumps also carry gated timings. Perf file (`perf_run.hpp:184`): `"perf_gather": true` (a run only exists while on).
- `CacheDiagLines` header line (`cache_diag.hpp:583`): append ` (performance metrics OFF: timings read 0)` only when the switch is off, so the clipboard text and the `lines` key say it too. With the switch on the text is byte-identical, so `test_live.cpp:1764` and `:1772` keep passing.
No other panel timing is duplicated into JSON (DRY): the other rows are already there.

## 1d. Gating the dumps at the source (owner intent, confirmed with one caveat)
Gating at the source means the cache-diag, pool, rtao and perf dumps carry zeros (CPU sums, last, max, phase sums) or the existing "no measurement" value (GPU: `-1` where the writer already writes `-1` without samples, `samples: 0`) while the switch is off, and `perf_gather: false` says why. This matches the owner's statement that performance metrics live only in the Performance tab. I agree with it. Consequences the implementer must document and not "fix":
- Event counts, tiers, reasons, episodes and every non-timing key are unaffected.
- Collect-only timing data (`work.*_ms`, episode `work`, `t80_work`, `tlas.ms_sum/ms_max`, `phase_ms`, `noop.ms`) needs BOTH "Collect cache diagnostics" and "Gather performance metrics" for real values. With Collect on and gather off, the episode ms read 0 and `perf_gather` is false. The in-game test steps (section 10) and ROADMAP must say "turn both on".
- Episode timeline offsets (`t80_ms`, `ms_offset`, `t80_frames`) are event times, not costs; they stay unchanged and are not gated.
- Switching OFF clears what was gathered (owner decision, supersedes the earlier "no clear" text): the clear runs once on the next present and zeroes the pool, live, cache-diag, GPU timer and RTAO timing state and the tab window. Event counts are not cleared. A dump written while OFF has `perf_gather: false` and zero timings.

## 2. What moves under the switch, and what stays
| Item | Now | After |
|---|---|---|
| `start`/lap clock reads in rebuild, refit, slack, skip, apply, mesh sync, BLAS, expand, after-drain | always | gather only (`PerfNow`) |
| `PoolCpuTimer` hook, drain, capture, deform (draw + update) | always | gather only |
| `DrainPoolScan` moves `:5711-5712`, `UpdateDeformLive:1004` | always (atomic writes) | `if (gather)` |
| `capture_us` exchange in `UpdateCacheDiag:2280` | always | gather only, read from the new `cpu_capture_frame_us` (no exchange) |
| `ReadPerfCounters`, `UpdatePerfTab`, `PerfExtraGpu` fill in `OnWorldPresentBvh:228-237` (RTAO device data, deform lock) | always | gather only |
| `present_time` clock and `frame_ms_*` in `UpdateCacheDiag:2302-2311` | always | gather only (`last_present` stays the epoch while OFF, so the first interval after ON is skipped) |
| `start_requested.exchange` in `UpdatePerfRun:252` | atomic write per present | `load()` first, exchange only when true |
| Event counters (`tlas_rebuilds`, `tlas_skips`, `slack_applies`, `refits`, `blas_builds`, `rebuilds_by_cause`, cause bits `:1014`), `capture_wait_sum`, episode clocks (Collect-gated, per event) | always | unchanged (no clock per present; some are logic inputs) |
| `+= ms` sums | always | unchanged code; they add 0 while OFF (no branch added) |
| Per-draw timers (hook x2, deform capture) | always | one relaxed load per draw when OFF; remain per draw when ON (pre-existing, flagged) |

## 3. Double-count fixes (do these first, with their tests)
- F2, `bvh_live.hpp:1043-1052`: move `stats.slack_apply_ms_sum += apply_ms;` and the `slack_apply_ms_max` line from before the `if (ops == 0u)` into the `else` branch. A zero-op apply is then only a skip. Update the comment at `bvh_resources.hpp:178` ("successful apply with at least one op"). Panel avg `world_bvh.hpp:505` (sum / `slack_applies`) becomes consistent. The cache JSON `apply_ms_sum` (`cache_diag.hpp:947`) changes meaning the same way; note it in ROADMAP.
- F1 pairing: `PoolState` gets `cpu_capture_frame_us` (`bvh_pool.hpp:1395` area). In `DrainPoolScan:5711-5712`, next to the hook and drain moves, add `cpu_capture_frame_us = cpu_capture_us.exchange(0)`. Drain and capture then describe the same drain call. `UpdateCacheDiag:2280` reads `cpu_capture_frame_us.load()`.
- F1 value: new entry `pool_drain_excl_capture` = `cpu_drain_frame_us/1000 - capture_us/1000` in `ReadPerfCounters` (no clamp: capture is nested in drain, `:5763` inside `:5710`, and per-scope microsecond truncation cannot make it negative; the test asserts >= 0). The tab sums this, never the whole drain.

## 4. Unattributed CPU
- Brackets: `const PoolCpuTimer` as the first statement of `OnWorldPresent` (`draw_census.hpp:455`, atomic `present_world_us`) and of `OnWorldPresentBvh` (`bvh_debug.hpp:205`, `present_bvh_us`). Cumulative atomics, so both `ReadPerfCounters` callers read them without consuming.
- Each entry has a role: InPresent (runs inside a bracket), DrawTime (outside), Bracket, Subset (phases, legacy keys), Gpu.
- `unattributed = max(0, mean(bracket_world) + mean(bracket_bvh) - sum of InPresent row means)`, computed on window averages (tab) and on window totals / presents (file), and the same formula on the newest sample for "now". Never per-sample-then-averaged: the rows are not frame-aligned (section 0), so one sample can be off by a frame while the window mean is exact.
- It would go negative when a row is counted that ran outside the brackets (none known), on the one-present row lag, or by microsecond truncation. Clamp to 0; show "(clamped)" and the raw value in the tooltip only when raw < -0.01 ms (quantization noise below that is silent). The file writes `"clamped": true|false` and `"raw": x`.
- CPU total = sum of the DrawTime rows + InPresent rows + unattributed (clamped), so it always equals the sum of the displayed rows. GPU total = sum of the GPU rows, each at its own value (now: last measured; avg: its measured mean). This replaces the per-sample sum of today (`UpdatePerfTab:358-362`); `TestPerfTab` expectations change.
- Not separately measured, therefore inside unattributed: draw census, `ResolveIndirectDraws`, deform probe, `RunP2bDiagnostics`, `UpdateCacheDiag`, `RunBvhDebugPass`, mesh sync minus BLAS (scan, retire, upload; `mesh_ms` already exists, adding its sum is the cheapest follow-up).

## 5. Rows: source, unit, gathering when ON, nothing when OFF
All entries live in one table `kPerfCostInfo[]` in `perf_run.hpp` (name, cumulative, gpu, role). It replaces `kPerfCostNames/Cumulative/Gpu` (single source). Tab shows now and 30-present average. OFF for every row: no clock read, no atomic write, no ring write (section 2).
| Row (file key) | Role | Source | Gathered by (ON) |
|---|---|---|---|
| TLAS rebuild (`tlas_rebuild`) | CPU InPresent | `timing.rebuild_ms_sum` `:1073` | `start` stopwatch of `RebuildLiveTlas`; includes a failed slack attempt (`start` is shared) |
| TLAS skip (`tlas_skip`) | CPU InPresent | `live.tlas_skip_ms_sum` | `count_skip`; no longer fed by zero-op applies |
| Slack apply (`tlas_apply`) | CPU InPresent | `live.slack_apply_ms_sum` | apply with ops > 0 only |
| TLAS refit (`tlas_refit`) | CPU InPresent | `timing.refit_ms_sum` | `RefitLiveTlas` (also the after-drain call; `after_drain_ms_sum` stays a subset) |
| BLAS build (`blas_build`) | CPU InPresent | `live.blas_build_ms_sum` | per-build stopwatch `:286` |
| Mesh capture (`mesh_capture`) | CPU InPresent | `cpu_capture_frame_us` | capture `PoolCpuTimer` `:5335,5471` |
| Pool drain excl. capture (`pool_drain_excl_capture`, new) | CPU InPresent | drain frame minus capture frame | drain `PoolCpuTimer` `:5710` |
| P2b expand CPU (`p2b_expand_cpu`, new) | CPU InPresent | new `PoolStats::dynamic_expand_cpu_ms_sum` | `ExpandLiveDynamic` stopwatch `:2032`, `+=` under the pool lock already held |
| SyncLiveAlpha CPU (`alpha_sync_cpu`, new) | CPU InPresent | `g_perf_gate.alpha_sync_us` | `PoolCpuTimer` as the first line of `SyncLiveAlpha` (`alpha_live.hpp:111`), once per present |
| Deform CPU, present update (`deform_update_cpu`, new) | CPU InPresent | `g_perf_gate.deform_update_us` | `UpdateDeformLive:1003` timer retargeted to this atomic |
| Pool hooks (`pool_hooks`) | CPU DrawTime | `cpu_hook_frame_us` | existing per-draw timers, Pool Scan on only |
| Deform CPU, draw hook (`deform_hook_cpu`, new) | CPU DrawTime | `cpu_frame_us` (hook only after the retarget) | `OnDeformCaptureDraw:453` timer |
| RTAO command recording (`rtao_record_cpu`, new) | CPU DrawTime | `g_perf_gate.rtao_record_us` | `PoolCpuTimer` first line of `rtao::Dispatch` (`rtao.hpp:194`, once per RTAO frame, covers the filter call and timer polling) |
| Unattributed CPU (`unattributed_cpu`) | CPU derived | section 4 | no source of its own |
| Brackets (`present_world_cpu`, `present_bvh_cpu`) | CPU Bracket, file only | `present_*_us` | section 4; shown in a collapsed "Present handlers" line |
| Trace view GPU (`gpu_trace`), alpha blit (`gpu_alpha_blit`), P2b expand (`gpu_p2b_expand`), deform refit (`gpu_deform_refit`), RTAO trace + temporal (`gpu_rtao`), RTAO filter (`gpu_rtao_filter`) | GPU | the six `GpuTimer` `ms_sum/samples` (unchanged) | `BeginGpuTimer`, gated; per measured frame, "--" without samples |
| CPU total, GPU total | derived | section 4 | none |
Legacy file keys kept unchanged in meaning, written derived (`"in_total": false`): `pool_drain` = excl + capture, `deform_cpu` = hook + update. Every key gets an additive `"in_total"` field. Schema stays 1.

## 6. BVH phase rows (cumulative sums, Subset role, not in the totals)
- Storage (all `float`, same cumulative style as `refit_ms_sum`): `PoolTlasTiming::rebuild_phase_ms_sum[5]` and `refit_phase_ms_sum[4]` (added next to `refit_phase_ms`, `bvh_pool.hpp:1315`), `LiveBvhStats::slack_phase_ms_sum[4]` (`bvh_resources.hpp:177`).
- Rebuild: inside `finish()` under the existing pool lock (`bvh_live.hpp:1066`): `for p: rebuild_phase_ms_sum[p] += diag.phase_ms[p]`, independent of Collect. Refit: `finish()` `:1289` adds `phase_ms`. Slack: at `:860` next to the existing `memcpy`, only for ops > 0 (matches the apply row).
- Names: `kCachePhaseNames` (snapshot+lock, filter+descriptors, sort+build+validate, buffer create, buffer destroy); refit: copy, descriptors, nodes+validate, upload; slack: diff, ops, refit+validate, upload. File keys `rebuild_phase_0..4`, `refit_phase_0..3`, `slack_phase_0..3` (names in a `"phase"` field). The tab shows them in a collapsed tree "BVH phases (subsets of the rows above)", averages per present.
- Note: a failed slack attempt is absorbed by rebuild phase 2 (sort+build+validate), because the laps are consecutive.

## 7. Tab and file wiring
- `PerfTabSample` becomes `double value[kPerfCostCount]` plus `bool measured[kPerfCostCount]` (GPU rows). `PerfTabView` keeps `cpu_now/cpu_avg/gpu_now/gpu_avg` (now the totals) and adds `now[]`, `avg[]`, `unattr_now/avg`, `unattr_clamped`. `UpdatePerfTab` stays a single lock on `g_perf.mutex` (leaf), delta logic unchanged for cumulative entries, ring write once per present, gather only.
- `DrawPerformanceTab` copies the view under the lock, then draws with ImGui after the unlock. A 4-column table (Cost, Unit, Now, Avg 30) in the order of the cost list, totals last.
- `WritePerfFile`: same loop over the table (units, events, rate as today), then derived keys `unattributed_cpu`, `cpu_total`, `gpu_total`, and the legacy keys. `UpdatePerfRun` aborts a running window when gather turns OFF ("Gather performance metrics switched off at fN", through the existing abort branch `:273`) and refuses a start while OFF.

## 8. Locks and ordering
- No new lock. `g_perf.mutex` stays a leaf: taken alone in `UpdatePerfTab`, `UpdatePerfRun` (after the pool lock is released), the checkbox handler and the draw. Not nested with the pool, cache-diag or deform locks. File I/O and ImGui outside every lock.
- Phase sums are added inside the pool-lock blocks that already exist (`finish()`); no new lock site. `ExpandLiveDynamic` adds its sum inside the existing pool-lock block. The new atomics are relaxed counters.
- Section 1b removes three lock acquisitions from `DrawBvhPanel` (pool, deform, cache-diag copies). `WriteCacheDiagFile` reads only `data->live` and its own `g_cache_diag` copy for the new keys; no pool lock is added there (that is why rebuild-by-cause ms is left in `world_pool.json` instead of being copied into `cache_diag.json`).
- `OnWorldPresentBvh` reads the deform lock for `extra.deform_refit` only when gather is on, after `UpdateLiveBvh`, as today.

## 9. Harness (all under `.falcom-dev/harness`; the harness main sets gather ON first so existing tests keep their coverage; one CHECK before that asserts the default is false)
- `test_live`: `TestPerfGateOff`. Gather OFF, run `UpdateCacheDiag` for N presents and a `RebuildLiveTlas`, a refit, a slack apply and a BLAS build: `g_perf.tab.count == 0` and not primed; `g_perf.run.presents` unchanged; `frame_intervals` and `frame_ms_sum` unchanged; all `*_ms_sum`, `*_phase_ms_sum`, `tlas_ms_last`, `refit_ms_last` stay 0; `g_perf_gate` atomics, `cpu_*_us` sentinels (set to 777 first) unchanged; event counters (`tlas_rebuilds`, `blas_builds`) still advance. Same sequence ON: sums > 0, tab count == N - 1, phases > 0.
- `test_live`: zero-op slack apply adds to `tlas_skip_ms_sum` and not to `slack_apply_ms_sum`; an ops > 0 apply the reverse (ties into `TestSlackAppliesDiag`, whose `:2509` check is kept).
- `test_live`: `TestPerfTab` extended with hand values: bracket 10 ms and in-present rows 7 ms gives unattributed 3 and CPU total = sum of rows exactly; rows 12 ms gives 0 with `unattr_clamped`; rows 10.004 ms gives 0 without the flag; GPU total = sum of rows; a row with no sample shows NaN and adds nothing; toggle ON resets the window (first present primes).
- `test_live`: `TestPerfWindow` keys: every new key present, legacy `pool_drain` = excl + capture and `deform_cpu` = hook + update, `cpu_total` equals the sum of its rows, phases sum to <= their parent row, `"perf_gather"` in the cache JSON. `UpdatePerfRun` aborts when gather goes OFF mid-run and refuses a start while OFF. The `:1374` capture test uses `cpu_capture_frame_us`.
- `test_live`: `TestCacheOnOff` still pins `0x55593b70e070e7fe` with gather ON and OFF (timing never feeds the TLAS descriptors); default switches unchanged.
- `test_pool`: `PoolCpuTimer` OFF adds 0 and ON adds > 0 (2 ms sleep); after drains with captures, `cpu_capture_frame_us <= cpu_drain_frame_us`, excl + capture == whole drain, and the tab total counts the capture once (the F1 fix); sentinels unchanged OFF.
- `test_rtao`: `rtao::Dispatch` with gather OFF leaves `rtao_record_us` unchanged, ON increases it. GPU gating cannot be observed in the mock (not a D3D11 device): `BeginGpuTimer` OFF is verified by review of the first line plus `created == false`; say so in the result.
- `test_build`: compile and run unchanged.
- Tests that check removed panel text: NONE exist. Read: no `test_*.cpp` includes `world_bvh.hpp` or `rtao_panel.hpp`, and none references the removed labels; the harness does not compile either file (only `test_rtao.cpp:54` includes `rtao.hpp`). So the panel removal has no harness coverage. It is verified by (a) the Windows build of `falcomengine-plus` (the only compile of the panels), (b) the grep in section 1b with its output shown, (c) in game. Do not claim a harness pass for it.
- Existing tests that read timing TEXT or keys and stay valid with gather ON (the harness main sets it): `test_live.cpp:1772` "Time since reset" line (zeros), `:1395` key list (extend with `"perf_gather"`, `"mesh_ms"`, `"skip_ms"`, `"frame_ms"`), `test_rtao.cpp:1234` root keys (`gpu_ms` stays; add `"perf_gather"` to the list) and `:1906` (`spatial_filter.gpu_ms` stays).
- New in `test_live`: `TestCacheLines` OFF case: gather OFF, `CacheDiagLines(nullptr)[0]` ends with "(performance metrics OFF: timings read 0)"; ON: no suffix and `lines.size() == 12u` unchanged. `TestCacheFile`-style key test: `"perf_gather": true` with ON and `false` with OFF; `"mesh_ms"` last/max equal hand values (set `data->live.mesh_ms_last/max`), `"skip_ms"` sum/max equal `tlas_skip_ms_sum/max`, `"frame_ms"` avg equals sum/intervals and `-1` with 0 intervals.
- New in `test_rtao`: `BuildRtaoJson` with `in.perf_gather` true and false writes the matching `"perf_gather"` and still parses with the existing validator.

## 10. Order of work and commands
1. `perf_gate.hpp`, `PoolCpuTimer`, `LiveElapsedMs`, `BeginGpuTimer`, then the `PerfNow` call sites; harness main sets gather ON; `TestPerfGateOff`. 2. F2 fix + test. 3. F1 pairing + `pool_drain_excl_capture` + test. 4. New stopwatches (alpha sync, rtao record, deform update/hook, expand sum, brackets). 5. Phase sums. 6. Table, tab view, `UpdatePerfTab`, unattributed, draw, file. 7. Panel removals (section 1b: `world_bvh.hpp`, `rtao_panel.hpp`), then the JSON additions (section 1c: `perf_gather` in the cache, pool, rtao and perf dumps, `mesh_ms`, `skip_ms`, `frame_ms`, the `CacheDiagLines` OFF suffix) and their tests. 8. ROADMAP entry (status, changed `apply_ms_sum` meaning, switch default OFF, "timings only on the Performance tab", Collect-only timings need both switches, dumps carry `perf_gather`, the three added JSON keys).
After every step: Windows build of `falcomengine-plus` (the harness does not compile the panel files), then the harness. Step 7 additionally shows the section 1b grep output.
- Build (Windows only): in a vcvars64 prompt (Build Tools 18, see harness README) `cmake --build E:\RenoDX\renodx\build --config Release --target falcomengine-plus`.
- Harness: `powershell -File E:\RenoDX\renodx\.falcom-dev\harness\run_win_some.ps1 -Tests @("test_live","test_rtao","test_pool","test_build")`, then `-All`. `test_motion:977` is a known red; any other red blocks the round.
- In game: (1) BVH panel and RTAO status row show no ms anywhere (only counts, switches, buttons, results). (2) Performance tab with the switch OFF shows the checkbox and one line; ON shows all rows. (3) Turn "Gather performance metrics" AND "Collect cache diagnostics" on, let the map settle, run the 30 s test, press Dump cache diagnostics and Dump Pool JSON. (4) Repeat Dump cache diagnostics with the switch OFF. Send back: the tab lines, `perf_*.json`, both `cache_diag_*.json` (the ON one has `perf_gather: true` and non-zero `tlas.ms_sum`, `mesh_ms`, `skip_ms`, `frame_ms`; the OFF one has `perf_gather: false`), `world_pool.json` and, for RTAO, `world_rtao.json`.
