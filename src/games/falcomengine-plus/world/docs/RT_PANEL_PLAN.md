# Ray Tracing section, Performance tab, RTAO fade in percent, BVH range (plan, revision 2)

Status: plan only, nothing implemented. Build id to set: `rt-panel-1` (`kWorldBuildId`, `bvh/bvh_pool.hpp:141`). Read `ROADMAP.md` "Current state" first. Not just panel work any more: the BVH region geometry (`CurrentPoolRegion`) changes so that RTAO can reach the full Region range. The RTAO shader, its constant buffer and the TLAS build code do not change. Pinned fingerprint `0x55593b70e070e7fe` (`test_live` `TestCacheOnOff`) and every switch default stay; the only default that changes is the Region slider, 512 to 256. Line numbers are today's. Files: CRLF for `.hpp/.cpp/.h` (addon.cpp keeps its BOM); LF for `world_bvh.hpp` and docs; harness files keep their present endings (check with `file`).

## Owner answers applied, and where they conflict (owner please read)

- A1 RTAO range = Region at 100%: needs a new region box (see "BVH range"), not a margin tweak. Region now means RANGE (guaranteed radius), not box edge. Cost: box edge 3R+20 m (788 m at 256, 1556 m at 512); the old default box was 512 m. A 512 m range holds about 28 times the volume the old maximum did.
- A2 default 256: changes rendering by default, see "Default 256". A3/A4 CPU/GPU sums and "--": as the plan. A6 rename: done in the plan. A7 perf doc error and perf file: in the plan.
- A5 grouping: see the table. Conflict C3: "Foliage motion" OFF does not freeze foliage, it removes wind foliage from the BVH (`bvh_pool.hpp:2209-2213`: `alpha_wind_opaque` off refuses wind keys and drops their instances). The description says so. If the owner wants OFF = static rest pose, the row must drive only `alpha_wind_sway` and `alpha_wind_opaque` leaves the panel.
- C1 percent defaults: the earlier 7.8125 / 15.625 would halve today's default fade at range 256 (20 / 40 m instead of 40 / 80 m). The plan uses 15.625 / 31.25 so the default render keeps 40 / 80 m. At range 512 these give 80 / 160 m.
- C2 perf file scope: the owner asked for the RTAO timers; the Performance tab sums six timers, so the file gets all six (`gpu_trace`, `gpu_alpha_blit`, `gpu_p2b_expand`, `gpu_deform_refit`, `gpu_rtao`, `gpu_rtao_filter`) or the tab and the dump would disagree. Additive keys only. Drop to RTAO-only only if the owner says so (then the tab must also drop to trace + RTAO).
- C4 grouping alpha foliage with indirect source loses the state "foliage on, indirect off" from the Ray Tracing rows; the single indirect checkbox stays in Advanced for that A/B.
- C5 the ray margin constant is tied to the RTAO Ray Max slider maximum (10 m). With Ray Max at 10 there is no slack for camera lag during the up to 8 present TLAS rebuild delay: pixels within a few metres of the fade end can turn neutral for a moment (counted in the panel's region line, fade there is about 0, invisible).
- Still the plan's assumption (no answer yet): Q3 in-slider text `%.2f` stays; Q6 saved metre values `RtaoFadeStart/End` are dropped; Q7 the Performance tab shows the running and last-result text lines.

## Findings (read from the code)

- F1 A "tab" is a `settings_ui.h` category (`kCategories` 35-46, `kSectionCategory` 49-75); a "section" is a card, drawn in order of first appearance (`SectionsForCategory` 320-329). A `CUSTOM` row draws itself (`on_draw`). `world_settings.hpp`, `settings_ui.h`, `world_bvh.hpp`, `rtao_panel.hpp`, `addon.cpp` are compiled only by the addon build.
- F2 The numbers beside RTAO sliders come from `kSliderReadouts` (`settings_ui.h:484-495`) and the branches in `DrawSettingRow` (523-551, 590-596); no other key uses them.
- F3 Today `region_size` is the box EDGE (`bvh_pool.hpp:1716-1735`): cells of size/2, camera at least size/4 from every face. RTAO clamps Fade End to `size*0.25 - ray_max` (`rtao_state.hpp:377`) and the shader neutralises pixels outside the box shrunk by `ray_max` (`world_rtao.cs_5_0.hlsl:93-98`). `size*0.25` is also the Depth Compare range (`bvh_debug.hpp:109`, `world_bvh.hpp:85`).
- F4 Region only filters which pooled instances go into the TLAS (`PoolInstanceInRegion`, `bvh_live.hpp:904`). The pool itself admits regardless of region (cap `kPoolMaxInstances` 2,000,000, `bvh_pool.hpp:172`). The mesh store (`UploadLiveMeshes`) is not region filtered. Nothing is sized from the region: the TLAS instance (192 B), node (32 B) and active buffers are created at each rebuild from the snapshot count (`bvh_live.hpp:1154-1216`); trace stack is 64 (`world_bvh_trace.hlsli:31`, enough for 2M leaves). Only the instance count in the box, and so rebuild time and wind/dynamic refit time, grows with the box.
- F5 Limit that no code change removes: the BVH holds what the pool has seen. Geometry never drawn by the game (frustum culled, behind the camera, never a shadow caster) is not in the pool, so "instances to the full range" means all pooled instances.
- F6 `trace_timer` is the BVH debug view only (`bvh_trace.hpp:302`); RTAO timers are `RtaoDeviceData::timer/filter_timer` (`rtao_resources.hpp:123,156`). `PERF_PANEL_PLAN.md` D10 and the tooltip `world_bvh.hpp:537` are wrong where they imply the trace timer covers RTAO.
- F7 `UpdateCacheDiag` (`bvh_live.hpp:2230`) runs once per present, already takes `g_pool.mutex` (2287-2296) and `exchange(0)`s `capture_us` (2297). `ReadPerfCounters` (`perf_run.hpp:82`) needs the pool lock. Zero-op slack applies are counted in both `slack_apply_ms_sum` and `tlas_skip_ms_sum` (`bvh_live.hpp:1042-1046`); CPU is overstated only with the experimental Slack switch ON.
- F8 `scan_indirect` changes what enters the BVH: `OnPoolScanIndirectDraw` returns at `bvh_pool.hpp:4241` before any admission, so no indirect draw (opaque or alpha) becomes an instance; `test_switches.cpp:240-264` proves the pool copies nothing from indirect draws with it off. `alpha_indirect_source` (3763) only governs the alpha subset of those draws and needs `alpha_foliage`.

## BVH range (new region geometry)

Region slider = range R (m): every pooled instance within R of the camera on all sides, plus a ray margin M for AO rays, is in the TLAS. Constants in `bvh_pool.hpp` next to 216-217: `kPoolRegionRayMargin = 10.f` (equals the RTAO Ray Max slider maximum; `RtaoRayMax .max` in `world_settings.hpp:210` uses this constant, one source). `kPoolRegionMinSize/MaxSize` stay 64 / 512 (now a range).
`PoolRegion` (1710) gets `float range`; `size` stays the box EDGE (shader, push constants, `tlas_region_size`, the harness PoolRegion fixtures keep working). `CurrentPoolRegion` (1716): `range = g_pool.region_size`; cell = R; `center = floor(cam / R) * R + R/2` per axis; `half = 1.5 R + M`; `min = center - half`, `max = center + half`, `size = 2 half = 3R + 2M`. Proof: camera is within R/2 of the centre, so it is at least `1.5R + M - R/2 = R + M` from every face. Rebuild cadence is unchanged for the default: a cell of 256 m, as today (old edge 512 = cell 256).

| R | cell | half | edge | min camera-to-face | volume vs old 512 box |
|---|---|---|---|---|---|
| 64 | 64 | 106 | 212 | 74 | 0.06x |
| 256 (default) | 256 | 394 | 788 | 266 | 3.6x |
| 512 (max) | 512 | 778 | 1556 | 522 | 28.6x |

Camera (10, 5, 10): R=64 box [-74, 138] (centre 32); R=256 box [-266, 522] (centre 128); R=512 box [-522, 1034] (centre 256). New default box contains the old default box (same cell grid, wider), so no existing instance leaves the TLAS at the default.
`BvhDeviceData` (`bvh_resources.hpp:379`) gets `float tlas_region_range = 0.f;` set in `commit` next to `tlas_region_size` (`bvh_live.hpp:955`). `LiveTlasChange` (516) keeps comparing `size` and `min` (size is a pure function of R). `world_pool.json` region block (`bvh_pool.hpp:6930`) gets additive `"range"` via `PoolJsonFloat` (NaN safe, `test_motion.cpp:1671` sets NaN). Depth Compare range: `bvh_debug.hpp:109` becomes `g_pool.region_size`, `world_bvh.hpp:85` prints `g_pool.region_size`.
Other code that assumes the default or the old meaning, all checked: `g_pool.region_size = 512.f` default (`bvh_pool.hpp:1398`, becomes 256.f); `test_live.cpp:2521` pins 512 itself (still valid: box [-522, 1034], the 4 instances at 2000 stay outside, 35 inside); `test_live.cpp:1067,1342` use hand-made `PoolRegion` +-256 (unaffected); `test_motion.cpp:1664-1694` NaN round trip (still valid JSON); `stats.region`, `ComputeCacheTiers`, `DumpWorldPoolObj` (2M vertex cap) only count more instances. Not region based: P2b arena, indirect windows, mesh arenas.
Cost and risk: larger snapshot (rebuild ms scales with N log N), more wind-sway instances refitted every present (`WindSwayPending`), bigger Depth Compare range. No new array limit is reachable below the existing 2M cap. Measure with the perf test A/B (R=256 against R=512, `tlas_instances` gauge and `tlas_rebuild`, `tlas_refit` averages) before the owner raises the range.
Optional later (not in scope): skip pixels with `distance >= fade_end` in the shader (fade is 0 there, AO is 1) to save rays; it changes the stats and is a shader change.

## RTAO fade (percent of the Region range)

`rtao_state.hpp:377` becomes `RtaoEffectiveFade(built_range, slider_range, start_pct, end_pct)`: `start_req = start_pct * slider_range / 100.f`, `end_req = end_pct * slider_range / 100.f` (multiply first); `end_eff = min(end_req, built_range)`; `start_eff = min(start_req, end_eff)`; `coverage = built_range`. `Fade` gets trailing `start_req`, `end_req`. Caller `addon.cpp:9050` passes `bvh_data->tlas_region_range` and `g_pool.region_size`; the `coverage < 1` gate (9053) stays and now means "no TLAS built yet". The cap by `built_range` only acts while the TLAS lags the slider (up to the 8 present rebuild interval). Push constants and shader still receive metres (`addon.cpp:9108`). No ray-max term: the margin M covers it.
Settings (`world_settings.hpp:389-414`): `RtaoFadeStartPct` / `RtaoFadeEndPct`, FLOAT, 0 to 100, format `%.1f%%`, defaults 15.625 and 31.25 (exact in float), section "Distance Fade". Remove `RtaoFadeStart/End`, `kSliderReadouts`. Bindings `g_rtao_fade_start_pct/_end_pct` (`rtao_state.hpp:33-34`, `addon.cpp:9050,9182-9183`, `rtao_panel.hpp:49`). `ParameterSnapshot` keeps 18 floats and names (now percent; comment it). `rtao.hpp:413` JSON `fade` gets additive `requested_start`, `requested_end`, `range`. `rtao_panel.hpp:48`: `Fade: effective start %.0f m, end %.0f m (requested %.0f / %.0f m, BVH range %.0f m; stored %.1f%% / %.1f%%)`.
Harness numbers (built, slider, start%, end% -> req start/end, eff start/end): (256,256,15.625,31.25) 40/80, 40/80 (today's default); (512,512,15.625,31.25) 80/160, 80/160; (256,256,100,100) 256/256, 256/256; (512,512,100,100) 512/512, 512/512; (256,256,50,100) 128/256, 128/256; (256,512,60,100) 307.2/512, 256/256 (TLAS lag); (512,256,100,100) 256/256, 256/256 (slider shrunk); (256,256,90,50) 230.4/128, 128/128 (start > end); built 0 -> coverage 0 (< 1, `range_too_small`). Tolerance 1e-3.

## Coverage tests (instances present to the full range on all sides)

- `test_live` `TestRegionCoverage`, for R in {64, 256, 512} and cameras at (10,5,10), (255.99,0,-255.99), (256,0,-256), (-0.01,0,0.01), (511.99,1000,3) (cell edges both sides, negative coordinates): (a) on every axis `cam - min >= R + M - 1e-3` and `max - cam >= R + M - 1e-3`; (b) pooled instances placed at cam +- R on each axis alone, the 8 corners (+-R, +-R, +-R) and at +-(R + M) on each axis alone are all `PoolInstanceInRegion`; one instance at `max + 1` is not; (c) `SetCamera`, `AddInstance` for the 6 axis directions at +-R, `present()` until `tlas_instances == 6` and equal to `BuildRefInstances`; (d) move the camera across a cell edge by 0.02 m: reason `region moved`, after the rebuild `tlas_instances` still 6, and during the 8 present interval the six instances stay (margin loss is below M).
- Shader skip transcription (the `InsideRegion` of `test_rtao.cpp:145`, same file include of `bvh_pool.hpp` or the check placed in `test_live` with `rtao/rtao_state.hpp` included): for every point P with `|P - cam| <= R` sampled on the sphere and the cube corners, `InsideRegion(P, region.min, region.size, 10)` is true at R = 64, 256, 512.
- `test_rtao` `TestFade` rewritten with the numbers above plus `sizeof(ParameterSnapshot) == 18 floats` kept; `TestPushLayout` unchanged.

## Default 256: what it changes

Region now means range, so 256 gives a 788 m box (old default box 512 m, guaranteed radius 128 m, now 256 m). Effects at the default: (1) RTAO image near the camera unchanged (fade 40 / 80 m by the 15.625 / 31.25 defaults); (2) the BVH debug views show geometry out to +-394 m instead of +-256 m, and Depth Compare judges within 256 m instead of 128 m, so its percentages change; (3) more instances in the TLAS, so TLAS rebuild and refit cost rises with the scene, not with a fixed amount; (4) nothing is persisted (`region_size` is not a `Setting`), so no saved value is reinterpreted. Yes: tell the owner explicitly, with the perf A/B request.

## Inclusion switches ("Ray Tracing" section, above the diagnostics)

Rows are `CUSTOM` (not float-bound `Setting`s, which would persist), inserted in `world_settings.hpp` BEFORE `WorldResearchEnabled` (line 56) so the section is drawn first. One loop over a table `{label, description, tooltip, flags[]}`; side effects as today (`LogPoolSwitches()` on any change; the deform row keeps `g_deform.enabled.store` and `RefreshPoolCaptureRequest()`). A row shows ON when its first flag is on; a click writes all its flags. Old long tooltips are moved verbatim.

| Row label (default) | Flags written | One-line description |
|---|---|---|
| Region (m) slider 64-512 (256) | `g_pool.region_size` | BVH range: every pooled object within this distance of the camera is in the BVH; RTAO reaches it at 100% fade. |
| Alpha foliage (leaves, grass, fences) (on) | `alpha_foliage` + `alpha_indirect_source` | Adds alpha-tested foliage, including foliage drawn by indirect (GPU driven) calls. |
| Foliage motion (wind and sway) (on) | `alpha_wind_opaque` + `alpha_wind_sway` | Adds wind foliage and sways it; off removes wind foliage from the BVH. Needs Alpha foliage. |
| Deforming meshes (characters, water) (on) | `g_deform_live.enabled` + `g_deform.enabled` | Adds skinned and animated meshes. |
| Moving objects follow their pose (on) | `follow_moving` | Keeps moving objects in the BVH at their current pose. |
| Moving objects: per-frame (P2b) (on) | `dynamic_per_frame` | Adds moving objects at the pose of the previous frame. |
| Keep moving objects out of the static BVH (off) | `exclude_moving` | Removes moving objects from the static BVH. |
| Scan indirect draws (on) | `scan_indirect` | Lets indirect (GPU driven) draws, opaque or alpha, enter the BVH (F8). |

Kept separate: follow, per-frame, exclude (different mechanisms, different failure modes). Alpha and indirect are grouped (indirect is a subset of alpha foliage and needs it); wind admit and sway are grouped (sway needs admit). Stay in Advanced under "BVH Diagnostics", unchanged: `Pool Scan`, `Live BVH`, `Capture meshes`, `Verify mesh captures`, `Legacy instance scale limits`, `Retry unstable meshes`, `Poison mesh staging`, `Log mesh captures`, `Hide what the game camera does not show`, and the single `Indirect foliage source copy (experimental)` checkbox (C4). Searched `std::atomic_bool` in `world/`: no other foliage-motion or range switch exists.

## Rename and sections

`world_settings.hpp:62,71`: `.section = "BVH Diagnostics"` on `WorldResearchEnabled` (Draw Census) and `WorldBvhPanel` (label "Phase 1 World BVH" kept). `settings_ui.h:69`: keep `{"Ray Tracing","Ray Tracing"}` and add `{"BVH Diagnostics","Ray Tracing"}`. Order in the tab: Ray Tracing, BVH Diagnostics, RTAO, Temporal Accumulation, Denoising, Distance Fade. New category "Performance" after "Ray Tracing" in `kCategories`, `{"Performance","Performance"}` in the map.

## Performance tab

One `CUSTOM` row in section "Performance": a 2-column table CPU | GPU, each cell `x.xx ms` and `avg y.yy ms`; GPU shows `--` when no GPU timer produced a sample in the window; the "Performance test (30 s)" button and its tooltip, the running line and the last-result line. The label buffer becomes `inline char g_diag_label[64]` in `world_bvh.hpp`; the BVH Diagnostics panel keeps the Label box and Dump, the tab button reads the same buffer.
- CPU per present = sum of the non-GPU `PerfCounters` entries (`TlasRebuild, Skip, Apply, Refit, BlasBuild, MeshCapture, PoolHooks, PoolDrain, DeformCpu`); cumulative ones as deltas (`now >= prev ? now - prev : now`, as `UpdatePerfRun`), per-present ones directly.
- GPU per present = sum over the six GPU entries that gained samples of `delta cost / delta events`. "Measured" = any gained samples. Current = newest measured present in the window; average = mean over measured presents. None = `--`.
- Window: `std::array<..., kPerfTabWindow = 30>` ring in `PerfState` under `g_perf.mutex` (leaf lock, never with another lock). The first present only primes `prev`. `UpdatePerfTab(const PerfCounters&)` precomputes now/avg (harness testable); the panel copies four numbers under the lock, releases, then draws. No ImGui call and no file I/O under a lock.

## Perf file (world_perf json)

`perf_run.hpp`: `PerfCost` gains `kPerfGpuAlphaBlit, kPerfGpuP2bExpand, kPerfGpuDeformRefit, kPerfGpuRtao, kPerfGpuRtaoFilter` after `kPerfGpuTrace`; names, a `kPerfCostGpu[]` table (cumulative true) and the existing `i == kPerfGpuTrace` branch in `WritePerfFile` (158) becomes `kPerfCostGpu[i]` (basis "measured" / "not measured", `avg_per_measured_frame`, `total`, `samples`). `struct PerfTimerSample { double ms_sum = 0.0; uint32_t samples = 0u; }` and `struct PerfExtraGpu { PerfTimerSample rtao, rtao_filter, deform_refit; }`; `ReadPerfCounters(data, capture_us, const PerfExtraGpu& extra = {})` reads `data->trace_timer`, `data->alpha.timer`, `data->dynamic.timer` and `extra`; `UpdatePerfRun(..., switches, const PerfExtraGpu& extra = {})` and `UpdateCacheDiag(device, const PerfExtraGpu& extra = {})` pass it on (existing harness calls keep compiling). bvh code never includes rtao: `OnWorldPresentBvh` (`bvh_debug.hpp:205`) builds `extra` from `rtao::GetRtaoDeviceData(device)` and, under `g_deform_live.mutex` alone, released before `UpdateCacheDiag`, from `g_deform_live.refit_timer`. Fix `PERF_PANEL_PLAN.md` D10 and the tooltip text at `world_bvh.hpp:537` (state which timers each key covers).

## Files and ordered steps

1. `bvh_pool.hpp`: constants, `PoolRegion.range`, `CurrentPoolRegion`, default 256, JSON `range`, `kWorldBuildId`. `bvh_resources.hpp` + `bvh_live.hpp:955`: `tlas_region_range`. `bvh_debug.hpp:109`, `world_bvh.hpp:85`. Tests: `TestRegionCoverage`; `-All` for default-region fallout.
2. `rtao_state.hpp`, `addon.cpp` (9050, 9182-9183), `rtao.hpp:413`, `rtao_panel.hpp:48-49`, `world_settings.hpp` (fade rows, `RtaoRayMax .max`). Tests: `TestFade`.
3. `perf_run.hpp`, `bvh_live.hpp` `UpdateCacheDiag` (hoist the `capture_us` exchange above the pool-lock block, call `ReadPerfCounters` inside it, `UpdatePerfTab` before `UpdatePerfRun`; no extra pool-lock acquisition), `bvh_debug.hpp` `OnWorldPresentBvh`. Tests: `TestPerfTab`, extended `TestPerfWindow`.
4. `settings_ui.h`: readouts removed (`kSliderReadouts`, `has_readout`, `row_hovered`, FLOAT case becomes plain `SliderFloat`, tooltip reduced); Performance category; section map. Grep for the removed names first.
5. `world_bvh.hpp` (LF) + `world_settings.hpp`: move the controls (117-119, 122-131, 229-288, call at 372), add the Ray Tracing draw function with the table, `DrawPerformanceTab`, `g_diag_label`; remove button/tooltip/running line (484-496, 535-539). Rename sections. Addon build.
6. Docs: `ROADMAP.md` (range rule at line 296 rewritten: Region = range, box 3R+20, fade percent of range, new tab, perf keys), `PERF_PANEL_PLAN.md` D10.

## Tests (harness; the runner calls `refresh_world.ps1`)

- `test_live` `TestPerfTab`: synthetic counters (rebuild +0.5, hooks 0.3 per present): first call primes; 40 presents give now 0.8, avg 0.8; ten at 1.8 give avg (20*0.8+10*1.8)/30; ring wraps; counter reset takes `now`; GPU: no samples is not measured; trace +2.0 and RTAO +1.0 in one present gives 3.0; one timer with 2 samples / 4.0 ms gives 2.0; average over measured presents only; 30 presents without samples is not measured; `UpdateCacheDiag(&dev)` twice fills the ring; `g_under_lock == 0`.
- `TestPerfWindow` additions: with `extra.rtao` growing 1.0 ms / present and `data->alpha.timer` set, the file has `"gpu_rtao": {"basis": "measured", "avg_per_measured_frame": {"value": 1,` and `"gpu_alpha_blit"`; without extras `"gpu_rtao": {"basis": "not measured"`; the unit-count check (`"value": ` equals `"unit": `) still holds; existing `gpu_trace` check unchanged.
- Must stay green: `TestPerf*`, `TestCacheOnOff` (the fingerprint line `0x55593b70e070e7fe` in the real output, four legs), `TestCacheDiagJson`, `test_switches` step 4, `test_pool`, `test_build`. `test_motion:977` is a known pre-existing red.

## Commands

Addon build (the only compile check for the panel files), inside vcvars64: `cmake --build E:\RenoDX\renodx\build --config Release --target falcomengine-plus` (exit 0, no new warnings). Harness from `E:\RenoDX\renodx\.falcom-dev\harness`: `powershell -File run_win_some.ps1 -Tests @("test_live","test_rtao","test_pool","test_build","test_switches")`, then `-All`. Paste the real `RESULT` / `PASS (0 failures)` lines.

## Order and review gate

1. Step 1 + `TestRegionCoverage`, `-All`. 2. Step 2 + `test_rtao`. 3. Step 3 + `test_live`. 4. Steps 4-5 + addon build. 5. Step 6 docs, `-All`. Gate: `planner-reviewer` reads `git diff` and checks: `file` shows CRLF on all `.hpp/.cpp/.h` (addon.cpp BOM kept), LF on `world_bvh.hpp` and docs; every switch default unchanged and only `region_size` 256; `LogPoolSwitches()` and deform side effects intact; no ImGui call under a lock, ring written only under `g_perf.mutex`, `g_deform_live.mutex` never held with the pool lock, no file I/O under a lock; no non-const reference parameters, no single-use helpers, no named single-use locals; the shader file and TLAS build code untouched; fingerprint and the `TestFade` default case in the real output; grep shows no remaining reference to `RtaoFadeStart`, `kSliderReadouts`, `tlas_region_size * 0.25`, `region_size * 0.25`. Owner in-game test only after the gate: R = 256 and 512, Fade End 100%, panel line shows requested = effective and the region (skipped pixels) count near the fade end; send the perf file for both ranges.

## Risks

- Box grows with R (see table): rebuild and refit cost scales with the scene; the owner decides the maximum after the A/B.
- Float cumulative sums lose precision over hours; per-present deltas jitter by about 0.01-0.06 ms; the 30 present average smooths it. CPU excludes RTAO command recording; GPU excludes the game's own passes.
- A Region change moves the effective fade but not `ParameterSnapshot` (stored percent), so history is not reset; the old clamp behaved the same.
- `GetRtaoDeviceData` creates an empty entry per device on first call (the lighting hook does the same).
- Follow-up (not in scope): `BeginDisabled` for dependent switches when Alpha foliage is off.
