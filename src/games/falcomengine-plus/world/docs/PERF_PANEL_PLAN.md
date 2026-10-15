# Perf panel, performance test and separate dumps (plan)

Status: plan only, owner-approved scope, nothing implemented. Build id to set: `perf-panel-1` (`kWorldBuildId`, `bvh/bvh_pool.hpp:141`). Read `ROADMAP.md` "Current state" first. Diagnostic only: no rendering, TLAS, pool or revision change. Pinned fingerprint `0x55593b70e070e7fe` (`test_live` `TestCacheOnOff`) must stay equal.

## Findings that shape the plan (read from the code)

- `world_bvh.hpp` is NOT compiled by the harness (it needs ImGui). Every computation must live in harness-compiled code (`cache_diag.hpp`, `bvh_live.hpp`); the panel only draws and forwards button presses.
- Slack apply cost is dropped today: `RecordCacheTlasRebuild` returns at `extra.applied` before `ms` is used (`cache_diag.hpp`, `if (extra.applied)`), and `LiveBvhStats` has `slack_applies` but no ms (`bvh_resources.hpp:169-177`). The slack counters are panel-only (no JSON).
- `GpuTimer` (`gpu_timer.hpp`) keeps only `last_ms`, `samples`, `skipped`. No sum, so no per-window GPU average. `trace_timer` only measures when a trace pass runs (debug view or RTAO); otherwise "not measured".
- Error counts, GPU check result and the switch states are only drawn in the panel (`world_bvh.hpp` Errors / GPU contents lines); none are in `world_cache_diag.json`. `presents`, `tiers_frame` and `tiers_valid` are also missing from the JSON.
- `cache_diag.hpp` is included before `deform_live.hpp` in `bvh_live.hpp`, but `deform_live.hpp` does not use `g_pool` or `cache_diag.hpp`, so `cache_diag.hpp` may include it (needed for the deform error counts). If a cycle appears, pass a prebuilt `CacheErrors` struct into the writers instead.
- `UpdateCacheDiag` returns early when Collect is OFF, and `RecordCacheTlasRebuild` is gated on Collect. The perf test must work with Collect OFF (cleanest numbers), so it reads only always-on counters: `LiveBvhStats`, `g_pool.tlas_timing`, `trace_timer`, the `cpu_*_us` atomics.
- `mesh_ms_last` is a stale "last" value (set only when a sync ran), so it must not be averaged. BLAS build ms (`blas_build_ms_sum`) covers the mesh-sync cost.
- `refit_ms_sum` (`tlas_timing`) already includes the after-drain refits (`after_drain_ms_sum` is a subset): use `refit_ms_sum` only.
- `ResetWorldPool` and map change both bump `g_pool.episode_serial` and set `episode_start_frame` (`bvh_pool.hpp:4594-4597`, `5967-5971`); `map_change_frame != 0` until the sweep.

## Decisions (resolved here; the implementer must not change them)

D1 Frame time = wall clock between two successive `UpdateCacheDiag` calls (one per present, `OnWorldPresentBvh`), `std::chrono::steady_clock`, passed in as a parameter so the harness can drive it. No new hook.
D2 The 30 s window is wall time, not a present count: the run starts at the first present after the request (that present has no interval), and ends at the first present where `now - t0 >= 30.0 s`; that last interval is included. Reason: two A/B runs get the same real duration whatever the frame rate; every average is per present, so differing present counts do not bias them. The file records the actual window (>= 30 s, error < one frame) and the present count.
D3 Refusal at start: a run is active, or `g_pool.map_change_frame != 0`, or `frame >= g_pool.episode_start_frame && frame - episode_start_frame < kCacheSettlePresents` (300, reuse the constant; covers the refill after a map change, pool reset or boot). Independent of the Collect switch.
D4 Abort: if `g_pool.episode_serial` changes during a run (map change or pool reset), the run ends at once, status `aborted`, reason `map change or pool reset at f<N>`; the file is still written (partial averages, flagged), never silently dropped.
D5 Switches are recorded at start and at end; if they differ the file has `"stable": false` (not aborted). `Reset diag counters` is ignored while a run is active (result line says so) because it zeroes BLAS counters mid-window.
D6 Separate dumps: folder `PoolOutputDir()/diag/`; names `cache_diag_<YYYYMMDD-HHMMSS>[_<label>].json` and `perf_<YYYYMMDD-HHMMSS>[_<label>].json`. Local time (`localtime_s`). If the path exists, append `-2`, `-3`, ... before `.json` (never overwrite). The fixed name `world_cache_diag.json` is dropped, no compatibility copy: the only readers are the docs and `TestCacheDiagJson`, both updated. Pool dumps (`world_pool*.json`) are untouched.
D7 Label: typed in an ImGui text box (max 63 chars, one box shared by Dump and Performance test). Sanitised for the file name only (the raw text is kept, escaped, in the JSON `label_raw`): keep `[A-Za-z0-9._-]`; space becomes `_`; every other byte (including non-ASCII bytes, `< > : " / \ | ? *`, control chars) becomes `_`; collapse `_` runs; trim leading and trailing `_`, `.` and `-`; truncate to 40; empty result means no label part. Reserved device names are harmless because the stem always starts with `cache_diag_` or `perf_`.
D8 Panel scope: the "Cache diagnostics" section plus the four count-bearing live lines (`TLAS skips`, `Slack:`, `TLAS timing`, `TLAS rebuild cause`) become ms-only. The red `Errors:` count line becomes a count-free red line "Errors recorded: see the dump (errors block)". The `GPU contents:` line shows only match/MISMATCH. All other panel lines outside these (Meshes, Instances, Alpha, Wind, Moving objects, Follow lag, TLAS header) are not touched this round; see optional O4.
D9 `CacheDiagLines` is unchanged (it still feeds the JSON `lines` key and the clipboard text); the panel no longer calls it. The ms-only panel lines are written inline in the panel (no new helper, like the existing `TLAS skips` line).
D10 Perf averages use only always-on counters (see Findings). Per present values for CPU ms; `gpu_trace` is per measured frame (`delta ms_sum / delta samples`), `null` value with `"basis": "not measured"` when no sample landed.

## Files and changes (order = implementation order)

1. `bvh/gpu_timer.hpp` (CRLF): add `double ms_sum = 0.0; float ms_max = 0.f;` to `GpuTimer`; in `PollGpuTimer` next to `timer->samples += 1u` add `ms_sum += last_ms; ms_max = max`. `DestroyGpuTimer` leaves them (cumulative; consumers use deltas, clamped).
2. `bvh/bvh_resources.hpp` (CRLF): `LiveBvhStats` add `float slack_apply_ms_sum = 0.f, slack_apply_ms_max = 0.f;` and `uint32_t gpu_check_frame = 0u;`.
3. `bvh/bvh_live.hpp` (CRLF), `RebuildLiveTlas` slack branch (about line 1046): compute `const float apply_ms = LiveElapsedMs(start);` (used twice: stats and `RecordCacheTlasRebuild`), add `stats.slack_apply_ms_sum += apply_ms; stats.slack_apply_ms_max = max(...)` immediately after `TryApplyLiveTlasSlack` returns true and before the `ops == 0` branch, so the count equals `slack_applies` (zero-op applies included; they are also counted as skips, as today). `CheckLiveBvhOnGpu`: set `stats.gpu_check_frame = g_state.frame.load()` where `gpu_checked = true` is set (line ~1415). No other logic change in `RebuildLiveTlas`.
4. `bvh/cache_diag.hpp` (CRLF): (a) add `#include "deform_live.hpp"`; (b) `struct CacheSwitches {bool collect, skip_noop, slack, live_bvh;}` read from `g_cache_diag.enabled` and `g_live_bvh.*`, and a shared `WriteSwitchesJson(std::ostringstream* out, const CacheSwitches&)` (used by both writers); (c) `struct CacheErrors` (meshes, readback{map_failures, staging_failures, base_mismatch}, bvh{blas_invalid, tlas_invalid, upload_failures, refit_failures, invalid_refs, stack_overflow}, deform{failed, resource_failures, map_failures}, total) filled by `CollectCacheErrors(data)` (pool stats under `g_pool.mutex` after `UpdatePoolStats()`, deform stats under `g_deform_live.mutex`, never nested; same sums as the panel's current Errors line; the panel calls this too, so one definition); (d) `SanitizeDiagLabel(const std::string&)` and `DiagFilePath(const char* prefix, const std::string& label, std::chrono::system_clock::time_point when)` (creates `diag/`, applies D6, existence check loop); (e) `WriteCacheDiagFile(data, frame, label)` writes to `DiagFilePath("cache_diag", ...)`, drops the `remove()` of the old file and the fixed name, and adds JSON keys listed in "JSON schema" below.
5. NEW `bvh/perf_run.hpp` (CRLF, included from `bvh_live.hpp` after `cache_diag.hpp`; the harness copy picks up new `.hpp` files automatically): perf state, counters table, `UpdatePerfRun`, `WritePerfFile` (see "Perf run"). Own leaf mutex `g_perf.mutex`; never held together with `g_cache_diag.mutex` or `g_pool.mutex`.
6. `bvh/bvh_live.hpp` `UpdateCacheDiag`: (a) reset branch: skip the reset while a perf run is active (D5); (b) right after `capture_us` is read and before the `g_cache_diag` lock, call `UpdatePerfRun(data, frame, capture_us, std::chrono::steady_clock::now())`; this is above the Collect early return, so a run works with Collect OFF. No other change; no call into graphics under any lock.
7. `bvh/world_bvh.hpp` (LF, panel): the section rewrite listed under "Panel" below. Buttons: keep the three switches, `Reset diag counters`, `Check GPU Contents`; `Dump cache diagnostics` keeps its two-press arm flow (needed so the tiers are computed) but passes the label; add `Performance test (30 s)`; add the label box. File I/O of the dump stays outside every lock (as now).
8. Docs (LF): `ROADMAP.md` (Current state: new round entry, harness list, new file names; replace `world_cache_diag.json` mentions), `TLAS_PLAN.md` / `CACHE_DESIGN.md` (file name mentions). Do not rewrite history rounds beyond the file-name mentions.

## Perf run (`perf_run.hpp`)

- State: `g_perf.start_requested` (atomic, set by the button), `g_perf.active` (atomic, read by the panel), `g_perf.elapsed_s` (atomic float for the status line), and under `g_perf.mutex`: label, `PerfRun run` (t0, last time, presents, start frame, episode serial, start switches, `prev` and summed `PerfCounters`, gauge sums, frame-ms sum).
- `ReadPerfCounters(data, capture_us)` returns a `PerfCounters` {cost ms[], events[], gauges[]} from a table `kPerfStats[] = {name, source, cumulative?, per_event?}`. Cumulative sources: `tlas_rebuild` (`tlas_timing.rebuild_ms_sum`, events `rebuilds`), `tlas_skip` (`live.tlas_skip_ms_sum`, `tlas_skips`), `tlas_apply` (`live.slack_apply_ms_sum`, `slack_applies`), `tlas_refit` (`tlas_timing.refit_ms_sum`, `refits`), `blas_build` (`live.blas_build_ms_sum`, `blas_builds`), `gpu_trace` (`trace_timer.ms_sum`, `samples`). Per present values: `mesh_capture` (`capture_us / 1000`), `pool_hooks`, `pool_drain` (`cpu_*_frame_us / 1000`), `deform_cpu` (`g_deform_live.cpu_frame_us / 1000`). Gauges (averaged): `tlas_instances`, `pool_instances` (admitted), `pool_meshes`, `slack_free_holes`. The pool lock is taken only while a run is active or a start is pending.
- Per present: delta = `now >= prev ? now - prev : now` (counter cleared by a device destroy) for cumulative entries, direct value for per present entries; frame interval = `now_time - last_time`.
- Start (request seen by `UpdatePerfRun`): apply D3, then seed `prev`, `t0`, switches, serial; result line "Performance test started". Refusal writes the reason into `g_cache_diag.data.last_result` (one result line for everything).
- End: window reached or D4 abort. Build the result under the lock, release it, then write the file (file I/O outside locks), then store the result line (path, bytes) in `g_cache_diag.data.last_result` under the cache lock.

## JSON schema

Every numeric value in the perf file is an object `{"value": x, "unit": "..."}` (units: `ms`, `ms/present`, `ms/event`, `ms/measured frame`, `count`, `count/s`, `fps`, `s`). `null` value when undefined.
Perf file (`schema 1`): `kind: "performance_test"`, `build`, `label`, `label_raw`, `timestamp`, `status` (`complete`|`aborted`), `abort_reason`, `start_frame`, `end_frame`, `window {requested_s, actual_s, presents}`, `frame_time {avg (ms), avg_fps (fps)}`, `switches {start{...}, end{...}, stable}`, `stats { "<name>": {avg_per_present, total, events, avg_per_event, rate} }` for the cost entries (gpu_trace carries `avg_per_measured_frame` and `samples`), `gauges { "<name>": {avg (count)} }`.
Cache dump (`schema 4`, existing keys unchanged): add `label`, `label_raw`, `timestamp`, `presents`, `tiers_valid`, `tiers_frame`, `switches {collect_cache_diagnostics, skip_noop_tlas, slack_tlas, live_bvh}`, `slack {applies, apply_ms_sum, apply_ms_max, added, removed, changed, exhausted, structural, maintenance, free_holes, phase_ms[4] (last)}`, `gpu {trace {last_ms, avg_ms, max_ms, samples, skipped}}`, `gpu_check {checked, ok, frame, result}`, `errors {total, meshes, readback{...}, bvh{...}, deform{...}}`, `work` (since reset, via the existing `work_json`).

## Panel (ms only)

Lines kept as they are (already ms): `CPU:`, `GPU: deform refit ..., trace ...`, `Pool CPU per frame`, `Refit phases (ms)`, `TLAS rebuild phases (ms)` (moves out of `CacheDiagLines` into an inline line).
Rewritten ms-only: `Frame: last/avg ms` (from the perf-independent last interval, avg since reset); `TLAS CPU ms: rebuild avg/max, skip avg/max, apply avg/max, refit last/max`; `Build/capture CPU ms: BLAS avg/max, capture total`; `Rebuild cost by cause (ms)` (drops the counts and the bump counters). Removed from the panel and confirmed present in JSON: counts (`tlas` rebuilds/added/removed/moved/by_tier/noop/skips), histogram (`histogram`), reasons (`tlas.reasons`), episode (`episode`), lighting (`lighting`), entry latency (`entry_latency`), tiers and size buckets (`tiers[]`, `extent`, `ratio`, `hist_check`, `self_check`), re-sightings (`resight`, `capture`), `Last file`. While running: one line `Performance test running: <s> / 30.0 s [label]`; otherwise the last result line.

## Harness tests (all `test_live`, drivable: `UpdatePerfRun` takes the clock)

- `TestPerfRefusals`: refused while active; refused with `map_change_frame != 0`; refused at `episode_start_frame = frame-10`; allowed at `frame-400`; refusal text in `last_result`; no state change on refusal.
- `TestPerfWindow`: synthetic 16 ms clock, counters advanced by known amounts per present (rebuild +0.5 ms, skip +0.1, apply +0.2, trace timer +2.0 ms per sample, hooks 0.3); run ends exactly at the first present with elapsed >= 30 s; file parses (`JsonError` empty); `frame_time.avg.value` 16.0 +- 0.01; every stat equals its hand-derived value; every value object has `unit`; presents and actual_s match; Collect OFF run gives the same numbers.
- `TestPerfAbort`: bump `episode_serial` mid-run: `aborted`, file written, reason text; `TestPerfSwitchesStable`: toggle `slack` mid-run: `stable` false; reset request ignored while active (BLAS counters untouched).
- `TestDiagLabel`: `"a b/c"` -> `a_b_c`; `"..\\x:*?"` -> `x`; `""` -> no label; 100 chars -> 40; non-ASCII bytes -> `_` collapsed; trim rules.
- `TestDiagSeparateFiles`: two dumps in the same second with the same label give two distinct existing files (`-2` suffix), the first keeps its own frame value, different labels differ, no `world_cache_diag.json` is created; a perf file and a dump never collide.
- `TestSlackApplyCost` (extends `TestSlackAppliesDiag`): `slack_apply_ms_sum >= 0`, `max <= sum`, count equals `slack_applies`, JSON `slack` block equals the live values.
- `TestDiagSwitches`: dumps with two switch combinations carry the exact four booleans. `TestDiagGpuAndErrors`: set `gpu_checked/ok/result/frame`, pool/live/trace/deform error fields, trace timer samples and sum: JSON shows each field and `errors.total` equals the sum; avg trace = sum/samples.
- `TestCacheOnOff` additions: fingerprint equals `0x55593b70e070e7fe` with a perf run active (Collect on and off, skip on and off).
- `TestCacheDiagJson`: schema 4 and the new keys. `test_pool` (existing mass-retire block, near line 325): assert `episode_start_frame == map_change_frame` and `map_change_frame != 0` until the sweep, then 0, so the refusal predicate cannot drift. `test_build` unaffected.

## Build and harness commands

Build (inside vcvars64): `cmake --build E:\RenoDX\renodx\build --config Release --target falcomengine-plus` (exit 0, no new warnings). The addon build is the only compile check for `world_bvh.hpp`.
Harness (Windows, from `E:\RenoDX\renodx\.falcom-dev\harness`): `powershell -File run_win_some.ps1 -Tests @("test_live","test_build","test_pool")`, then `powershell -File run_win_some.ps1 -All`. Both must show `PASS (0 failures)` with the real `RESULT` lines pasted. The harness copies sources with CR stripped; run `refresh_world.ps1` (done by the runner) so the copy is current.

## Implementation order and review gate

1. Steps 1-3 (counters), build + `test_live`. 2. Step 4 (writers, label, switches, errors) + tests. 3. Steps 5-6 (perf run) + tests. 4. Step 7 (panel), addon build. 5. Step 8 (docs), `-All`. Gate: `planner-reviewer` reads `git diff`, checks: CRLF on all `.hpp` except `world_bvh.hpp` (LF) and docs (LF) via `file`; lock rule (no graphics call, no file I/O under a lock; `g_perf.mutex` never nested); no non-const reference parameters, no single-use helpers or named single-use locals; the fingerprint line in the real output; every claimed pass has pasted output; the panel has no count line left in the listed sections. Owner in-game test steps are written after the gate.

## Risks and what would make the plan wrong

- A refill that never settles (`episode_start_frame` is only bumped by map change/reset, so this is bounded) is fine; but if `map_change_frame` stays non-zero (sweep held by `BudgetFull`, up to `kPoolRecaptureFrames`) the button refuses for a while: the message must say why and when to retry.
- `trace_timer` is `null` unless a trace pass runs; the owner must turn on a GPU Debug view or RTAO for GPU numbers. If the owner expects GPU cost with both off, there is nothing to measure (other GPU timers are optional O3).
- Float sums (`tlas_skip_ms_sum`, `blas_build_ms_sum`) lose precision over very long sessions; deltas over 30 s stay within about 0.01 ms.
- Wall-clock frame time includes anything the game does (loading hitches, alt-tab). The window is only comparable at the same spot and with the same activity; the file cannot prove that (see O5).
- Pressing `Check GPU Contents` or a dump during a run adds a hitch (waits for the GPU); the status line warns, the file does not detect it.
- If `cache_diag.hpp` cannot include `deform_live.hpp` (cycle), switch to passing `CacheErrors` from the caller; the schema is unchanged.
- The plan is wrong if the owner meant a present-count window, or wants Collect ON during the test (then numbers include diagnostic cost; the file records the switch, so this is visible).

## Optional recommendations (not in scope; owner decides)

- O1 (recommended) Add frame-time max, p99 and a hitch count (frames > 2x average) to the perf file: a mean hides the TLAS-rebuild hitches that matter here. Cost: a vector of about 2000 floats during a run.
- O2 (recommended) Record camera position and map identity in the perf file, so A/B pairs can be rejected when taken at different spots (cheap: `PoolCameraPosition`).
- O3 Include the other GPU timers (alpha, dynamic expand, deform refit) as `gpu_*` stats.
- O4 Strip the remaining count lines from the panel (Meshes, Instances, Alpha, Wind, Moving objects, Follow lag, TLAS header) in a follow-up round; they are in `world_pool.json` already.
- O5 A small script that diffs two `perf_*.json` files (A/B table with percent change).
- O6 Run the three switch states back to back automatically (a guided A/B/C), with a fixed settle gap.
- O7 Disable the other buttons while a run is active (`ImGui::BeginDisabled`).
