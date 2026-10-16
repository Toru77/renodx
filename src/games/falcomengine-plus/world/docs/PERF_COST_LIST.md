# Performance tab: cost list (analysis only, nothing implemented)

Code read: `bvh/perf_run.hpp`, `bvh_live.hpp`, `bvh_pool.hpp`, `deform_live.hpp`, `alpha_live.hpp`, `rtao/rtao.hpp`, `bvh_trace.hpp`, `gpu_timer.hpp`, `bvh_debug.hpp`, `capture/draw_census.hpp`. Line numbers are from the tree at commit 433f2d55.
"Now" = already in the tab total and the perf file (the 15 `kPerfCost*` entries). "Cheap" = value exists, only display or a `+=` is needed. All CPU timers use `steady_clock`; all GPU timers are `GpuTimer` (D3D11 timestamp queries, 4 slots, read without waiting, resolved a few presents late).

## 1. BVH build (CPU ms)
| # | Cost | Unit | Now | New timer | Notes |
|---|---|---|---|---|---|
| 1 | TLAS rebuild | CPU | yes | no | `rebuild_ms_sum`, `bvh_live.hpp:1060-1075`. Includes the time of a failed slack-apply attempt (same `start`). |
| 2 | TLAS skip | CPU | yes | no | `tlas_skip_ms_sum`, `:1015`. Covers snapshot, filter and the compare. |
| 3 | Slack apply | CPU | yes | no | `slack_apply_ms_sum`, `:1043`. Zero-op applies are ALSO counted as skips (see finding F2). |
| 4 | TLAS refit | CPU | yes | no | `refit_ms_sum` (every call, incl. after-drain and failed). `after_drain_ms_sum` is a subset, not added. |
| 5 | Rebuild cost by cause (retire / admit / other) | CPU | partly | no | `rebuild_ms_by_cause`, always on, panel only. Cheap: 3 sub-rows. |
| 6 | Rebuild phases: snapshot+lock, filter+descriptors, sort+build+validate, buffer create, buffer destroy | CPU | partly | no clock | Laps are always measured (`diag.phase_ms`) but stored only with Collect ON (`cache_diag.hpp:500`). Cheap: `+=` 5 floats into `PoolTlasTiming` in `finish()`. |
| 7 | Refit phases (copy, descriptors, nodes+validate, upload); slack phases (diff, ops, refit+validate, upload) | CPU | last only | no clock | `refit_phase_ms`, `slack_phase_ms` keep the last call. Cheap: add sums. |
| 8 | BLAS build (CPU BVH of a mesh) | CPU | yes | no | `blas_build_ms_sum`, `:290`. Only `BuildMeshBlas`, not the upload. Counters reset by "Reset diag counters". |
| 9 | Mesh sync remainder (scan, retire, buffer + descriptor upload) | CPU | stale | no clock | `SyncLiveMeshes` already has a stopwatch (`:465-496`) but keeps only last/max. Cheap: add `mesh_sync_ms_sum`; remainder = sync - BLAS build. |
| 10 | TLAS/BLAS upload GPU copies | GPU | no | cannot | Plain buffer writes on the immediate list, no pass to bracket. |

## 2. Mesh capture and pool (CPU ms)
| # | Cost | Unit | Now | New timer | Notes |
|---|---|---|---|---|---|
| 11 | Capture (decode + apply) | CPU | yes | no | `cpu_capture_us`, `bvh_pool.hpp:5335,5471`, read at `UpdateCacheDiag`. NESTED inside #14 (F1). |
| 12 | Capture wait | presents | diag only | n/a | `capture_wait_sum` counts presents waiting for the GPU read-back, Collect-gated. Not a CPU cost, do not show as ms. |
| 13 | Pool hook copy | CPU | yes | no | `cpu_hook_us`, per draw, only while Pool Scan is on (0 otherwise). |
| 14 | Pool drain (whole `DrainPoolScan`) | CPU | yes | no | Includes #11. Value shown is the previous present's (`:5711`). Cheap derived row: drain - capture. |
| 15 | Pool admission (`GatePoolDraw`) | CPU | no | yes, not advised | Inside #13. Needs a nested per-draw timer (2 clock reads + atomic per draw, thousands of draws): measurable overhead. |

## 3. Deform, alpha, P2b
| # | Cost | Unit | Now | New timer | Notes |
|---|---|---|---|---|---|
| 16 | Deform CPU (draw hook + `UpdateDeformLive`) | CPU | yes | no | `cpu_us`, one present late. Hook vs update split needs a 2nd atomic (cheap, same `PoolCpuTimer`). |
| 17 | Deform refit | GPU | yes | no | `refit_timer`; samples only when objects > 0. Timer is reset (`GpuTimer{}`) on release: deltas are clamped. |
| 18 | Deform stream-output capture | GPU | no | cannot | Issued inside game command lists (possibly deferred); the timer uses the immediate context. |
| 19 | Alpha blit | GPU | yes | no | `alpha.timer`; only frames that blit a new alpha mesh (rare: shows "--" most of the time). |
| 20 | `SyncLiveAlpha` CPU | CPU | no | yes (CPU) | `PoolCpuTimer`, 2 clock reads per present. Alpha TLAS work is inside #1-#4, not separable. |
| 21 | P2b expand | GPU | yes | no | `dynamic.timer`; only with a non-empty draw table and Live BVH on. |
| 22 | P2b expand CPU | CPU | last only | no clock | `dynamic_expand_cpu_ms` (last, table build only). Cheap: add a sum. |
| 23 | P2b traversal cost | GPU | no | cannot | Inside the trace/RTAO dispatch; no sub-dispatch timing in D3D11. |

## 4. RTAO and trace view
| # | Cost | Unit | Now | New timer | Notes |
|---|---|---|---|---|---|
| 24 | RTAO trace + temporal | GPU | yes | no | One timer around pass A + pass B (`rtao.hpp:222-283`). Split B out: +1 GPU timer, sequential (not nested). |
| 25 | RTAO spatial filter | GPU | yes | no | `filter_timer`; only with the filter on. |
| 26 | RTAO command recording | CPU | no | yes (CPU) | Stopwatch around `rtao::Dispatch` (`addon.cpp:9223`) or `RunRtaoInline`. Cheap. |
| 27 | Trace view (debug) | GPU | yes | no | `trace_timer`, only with GPU Debug view on (`bvh_trace.hpp:302`). RTAO does not use it. |
| 28 | Debug blit + stats read-back stall | GPU/CPU | no | no | Debug-only, on request; not worth timing. |

## 5. Totals
| # | Cost | Unit | Now | New timer | Notes |
|---|---|---|---|---|---|
| 29 | Unattributed CPU (everything else in the present handlers) | CPU | no | yes (CPU) | 2 stopwatches (`OnWorldPresent`, `OnWorldPresentBvh`) minus the sum of the rows = honest remainder. Today unmeasured: draw census, deform probe, `ResolveIndirectDraws`, `RunP2bDiagnostics`, `UpdateCacheDiag`, `RunBvhDebugPass`. |
| 30 | Frame time (present interval) | wall | yes | no | In the perf file; not on the tab. Cheap. |

## Findings
- F1 (bug): the CPU total counts capture twice. `ResolvePoolSlot` (capture timer) runs inside `DrainPoolScan` (drain timer, `:5763` vs `:5710`), and `UpdatePerfTab` sums both.
- F2 (bug): a zero-op slack apply adds the same ms to `slack_apply_ms_sum` and `tlas_skip_ms_sum` (`bvh_live.hpp:1043-1047`); the CPU total counts it twice.
- F3: hook and drain/deform CPU are the previous present's value, capture and TLAS are this present's. Averages are fine, "now" mixes two frames.
- F4: "GPU total" sums only the timers that gained samples this present; timers resolve a few presents late. It is not one frame's GPU time.
- F5: "CPU total" is the sum of measured items, not the addon's full CPU (see #29).

## Risks
- Every GPU timer already runs whenever its pass runs, tab shown or not (`Begin` polls up to 12 queries). A new GPU timer adds 12 queries and that poll; gate new ones on a "Performance tab or test active" atomic.
- CPU stopwatches cost two clock reads (about 50 ns); only per-present ones are acceptable, never per-draw (#15).
- Showing per-cost rows needs a 15 x 30 double ring in `UpdatePerfTab`, which already runs every present. Negligible.
- A GPU timer must stay sequential with the others, not nested (the filter timer is already outside the RTAO timer).

## Questions for the owner
1. Show the BVH phase split (#6, #7) on the panel, or only in the perf file?
2. Fix F1 and F2 (so the totals add up) before adding the separate rows, or show the rows first?
3. Add the unattributed row (#29) so the total can be trusted, or keep the total as the sum of measured items?
