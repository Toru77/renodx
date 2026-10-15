# Panel cleanup (plan)

Status: implemented (steps 1 to 4), not validated in game. Owner request (approved): the BVH panel shows only ms costs and controls. Diagnostic only: no rendering, TLAS, pool or revision change; pinned fingerprint `0x55593b70e070e7fe` (`test_live` `TestCacheOnOff`) must stay equal. Build id to set: `panel-cleanup-1` (`kWorldBuildId`, `bvh/bvh_pool.hpp:141`). Line numbers are `world_bvh.hpp` today (LF). No test reads panel text (grep of `.falcom-dev/harness`: no match).

## Decisions (the implementer must not change them)

D1 The ms values inside three listed lines are costs, so they stay as ONE ms-only line: `Alpha and moving-object ms: alpha blit %.3f, alpha trace %.3f, expand cpu %.3f, expand gpu %.3f` (from `alpha_gpu.blit_ms/trace_ms`, `stats.dynamic_expand_cpu_ms/gpu_ms`). Owner can drop it later; the same values are in `world_pool.json`.
D2 Count-only lines outside the screenshot also go: `TLAS: N instances, last rebuild ...` (507-509), `Follow lag (frames)` (530-531), the `Deforming: traced ...` text (107-108). Kept: all controls, the `Deforming meshes in the BVH` checkbox (mode switch), `Frame`, the TLAS phase line, `CPU:`, `TLAS CPU ms`, `Build/capture CPU ms`, `Rebuild cost by cause`, `Refit phases`, `GPU:`, `Pool CPU per frame`, `GPU contents` (match/MISMATCH), `Errors recorded`, perf status line, last result line, placement buttons and their result lines, Label box, `prevWorld trace` button status, Inspect lines, `DrawDepthCompareStats` (only shown inside a trace view the owner turns on; output of a mode switch).
D3 The `Diagnostics:` status line goes, but the owner must still see that "Diagnostics (all)" finished: p2b_diag.hpp sets the one shared result line (`g_cache_diag.data.last_result`, under `g_cache_diag.mutex`, no graphics call under it) to `Diagnostics (all): wrote world_p2b_diag.json at f<N>, verdict <verdict>` or `FAILED readback at f<N>: <reason>`.

## 1. world_bvh.hpp (LF) - delete by label

- 378-383 `Meshes:` (with its red PushStyleColor/Pop pair); 384-386 `Instances:`; 387-391 `Alpha foliage`; 392-396 `gpu` copy block; 397-402 `Alpha GPU:`; 403-405 `Alpha TLAS:`; 406-408 `Indirect/wind:`; 409-413 `Wind sway:`; 414-418 `Wind draws:`; 419-425 `Moving objects (P2b):` (replaced by the D1 line); 426-436 `Diagnostics:` block.
- 107-108: in `DrawDeformLivePanel` delete the `if (!enabled) return;` and the `Deforming:` text; signature becomes `DrawDeformLivePanel()` (call at 437). 507-509 `TLAS:` header; 530-531 `Follow lag`.
- Add the D1 line where `Moving objects (P2b)` was.

## 2. Locals removed

`deform` (DeformLiveStats) and `deform_traced` (360-376; keep `deform_refit_ms` and its lock), `diag_verdict`, `diag_state`, `diag_state_text`, `live_stats`/`frame` stay only if still read (`frame` was used only by the TLAS header: remove it; `live_stats` still feeds `CPU:` and the ms lines). `UpdatePoolStats()` call (364): nothing in the panel reads the refreshed fields any more (`DumpWorldPool` and `CollectCacheErrors` call it themselves): remove. `stats` and `tlas_timing` copies stay (D1 and the TLAS ms lines); the `alpha_gpu` copy moves into that same single lock block (one block, not two).
`p2b_diag.hpp` (CRLF): `state`, `written_frame`, `verdict_hint`, `mutex` of `P2bDiagState` lose their only reader: remove them and their writes (644-649) and `g_p2b_diag.state.store(1u)` at world_bvh.hpp:143; keep `requested`, `armed_frame` (read at 460).

## 3. JSON: every removed number and where it lives

Already in `world_pool.json` (confirmed in `DumpWorldPool`, `bvh_pool.hpp`):
- Meshes: `stats.meshes`, `meshes_admitted_by_retry`, `mesh_failures`, `mesh_unstable`, `mesh_queue`; "retrying" = length of `mesh_retrying[]` (7303).
- Instances: `stats.admitted_instances`, `region_instances`, `follow.orphans_retired`. "following": NOT in JSON, added (below).
- Alpha foliage: `alpha.alpha_foliage/draws/conflicts/meshes_conflicted/refused_off/conflict_refused/no_uv/removed`.
- Alpha GPU: `alpha_gpu.slices_used/slices_total/blits/blits_frame/copies/proxies/proxy_bytes/source_refused/source_refused_bytes/source_refused_format/uv_requeues/cap_refused`.
- Alpha TLAS: `alpha_gpu.tlas_alpha_instances/alpha_waiting/alpha_tests/alpha_cut/blit_ms/trace_ms`.
- Indirect/wind: `alpha_gpu.indirect_copies/orphans_expired/wind_rest_draws/wind_refused_off`.
- Wind sway: `wind_sway.switch/instances_last_pass/max_displacement_m/amplitude_bound_m/unread_draws/skipped_nonfinite/skipped_range`; "capped" = `wind_sway.pass.clamped`.
- Wind draws: `wind_sway.draws.{direct,indirect}.draws/rest/cb_read.b0[0]`.
- Moving objects: `p2b.per_frame/copied_total/draws_last_present/refused_total/deferred_total/records_last_present/not_resident_last_present/arena_bytes_last_present/expand_cpu_ms/expand_gpu_ms`.
- Diagnostics: `verdict_hint` and `armed_frame` in `world_p2b_diag.json` (`p2b_diag.hpp:347`); idle/armed/written state is transient UI state (D3 replaces it).
- Follow lag "on screen": `dynamic.on_screen_lag_last/max`.

NOT in JSON, added in this change:
- `bvh_pool.hpp` (CRLF), `stats` object next to `region_instances` (~7034): `"following_instances": stats.following`.
- `cache_diag.hpp` (CRLF), new top-level `"live"` block after `gpu_check` (~957): `tlas_instances`, `tlas_rebuilds`, `tlas_reason` (`PoolJsonEscape`), `tlas_frame`, `refit_lag_last`, `refit_lag_max` (all from `data->live`; "frames ago" = existing `frame` minus `tlas_frame`).
- `deform_live.hpp` (CRLF), `DumpDeformLive` `stats` object (~1276): `frame_captures`, `frame_objects` (the old "traced"), `frame_pending`. `failed` is already there.

## 4. Tests (harness; none reads panel text)

- `test_live` `TestCacheDiagJson`: assert the `live` block keys and that `tlas_instances` equals `data->live.tlas_instances`.
- `test_deform_live` (near line 485): assert `frame_captures`, `frame_objects`, `frame_pending` in `world_deform_live.json` and `frame_objects + frame_pending == frame_captures`.
- `test_pool` (near line 794): assert `stats.following_instances` equals the count of `dynamic` instances. Fingerprint line must still print `0x55593b70e070e7fe`.

## 5. Commands

Harness (Windows): `powershell -File E:\RenoDX\renodx\.falcom-dev\harness\run_win_some.ps1 -Tests @("test_live","test_pool","test_build","test_deform_live")`, then `-All` (`test_motion` red is pre-existing). WSL form: not used here; same script from `E:\RenoDX\renodx\.falcom-dev\harness`.
Build (the only compile check for `world_bvh.hpp`, inside vcvars64): `cmake --build E:\RenoDX\renodx\build --config Release --target falcomengine-plus` (exit 0, no new warnings, no unused-variable warning).

## 6. Order and review gate

1. JSON additions + tests, harness. 2. `p2b_diag.hpp` D3 + struct trim. 3. Panel deletions, addon build. 4. `ROADMAP.md` (LF): one entry, panel line list now = ms only. Gate: `planner-reviewer` reads `git diff` and checks: CRLF on `.hpp` except `world_bvh.hpp` (LF) via `file`; every deleted line's number is in JSON (table above, JSON text shown in the real output of one dump); no ImGui call under a lock and no graphics call under `g_cache_diag.mutex`; no unused locals/fields left (`deform`, `deform_traced`, `frame`, the p2b_diag fields); no non-const reference parameters, no new single-use helpers; real harness `RESULT` lines and fingerprint pasted; the panel has no count-only line except the D2 keep list.

Owner in-game test (written after the gate): open the panel with Live BVH on; only ms lines, switches, buttons, result line remain; press Dump cache diagnostics and Diagnostics (all); send `diag/cache_diag_*.json` (`live`), `world_pool.json` (`stats.following_instances`) and the result line text.
