# RTAO owner test scripts (D2 to D5)

Moved out of ROADMAP.md on 2026-10-22. Each step is an in-game test. Send back the listed lines and files.

#### D2 owner steps (temporal diagnostics, D0 and D1)

Settings: History Clamp 0, Debug Off, Temporal on, Two-Sided discovery off. The test rows are session only: after a restart they are off.

1. Still camera, scene still. Record the panel: mean |motion|, share moving, clamp active, mean alpha, mean |raw - AO|, no-history share, "Output differs from raw by more than 1 LSB" (texel_differs share). Send the panel and world_rtao.json.
2. Moving camera (same settings). Record the same panel lines and send the json.
3. Temporal test "force zero motion" on (still scene and camera). Record the panel and json, then turn it off.
4. "Temporal test: freeze noise" and "force zero motion" both on, still camera, still scene. Expected: the output equals the raw AO, so texel_differs should be near 0 %. Record the panel and json. A small share at depth and normal edges is expected: the TAA jitter still moves the reconstructed positions.
5. "Freeze noise" alone (motion applied). A larger texel_differs share is expected here. Record the panel and json.
6. Debug modes 4 (motion px), 5 (blend alpha) and 6 (|raw - AO|) with Temporal on: the mode shows full screen. Take a screenshot or a short video of each.

7. D1b (still camera, scene still, Debug Off, History Clamp 0): panel "Motion vs camera-matrix difference". Expected for a static scene: most traced pixels within 0.25 px and jitterDiff_g small. A large share above 1 px on a static scene means the camera matrix and the motion disagree (report it). Then Debug 7 ("Motion vs matrix (px)"), shown full screen: dark = agreement, bright = difference.
8. Test row "camera-matrix reprojection" on (still scene): the history uses the matrix, so the panel share within 0.25 px and the output should not change much from step 1. Then turn it off. "Force zero motion" wins if both are on.
9. texel_differs is valid with Debug Off only (debug modes change the written texel).
Send back for each case: the RTAO status panel lines, world_rtao.json, and the screenshots or video of modes 4 to 6.

#### D3 owner steps (temporal diagnostics, P0)

Settings: History Clamp 0, Debug Off, Temporal on, Two-Sided discovery off, test rows off unless a step says otherwise.

1. Still camera, scene still, SPP 2: capture the RTAO panel and world_rtao.json. Read: the "Last dispatch" lines (captured parameters), traced pixels, the weighted shares and "other", the depth-ratio histogram, the frame-to-frame raw difference (mean and identical share; for an independent binary raw at p 0.217 the mean is about 0.34), the unweighted tap lines.
2. Moving camera, same settings: the same capture.
3. Still camera, Freeze noise on: the same capture (frozen ray directions; the raw difference should fall if the noise is the source of the frame-to-frame change).
4. Still camera, Force zero motion on: the same capture.
5. Still camera, SPP 4: the same capture (the captured parameters line shows SPP 4).

Send back for each case: the RTAO panel lines and world_rtao.json.

#### D4 owner steps (soft normal weight, normal-dot histogram)

Settings: History Clamp 0, Debug Off, Temporal on, Two-Sided discovery off, test rows off. Normal Rejection 0.9 unless a step says otherwise.

1. Still camera, scene still, SPP 1: capture the RTAO panel and world_rtao.json. Read: "no-history" share (the criterion below), weighted accepted, rejected by normal and other, and the normal-dot histogram (share of pixels whose highest-weight tap has dot <0, <0.5, <0.7, <0.9, <0.97, >=0.97).
2. Moving camera (slow pan), same settings: the same capture. Roughness and change lines are not valid here.
3. Still camera, Normal Rejection 0: the same capture. Every facing tap is accepted, so this is the reference for what the ramp costs.

Criterion: on the still camera (step 1) the no-history share should be below 2%. Fallback: if it stays at or above 2%, the normal-dot histogram from step 1 decides a wider ramp (for example 0.6) and the three captures are repeated.

Send back for each case: the RTAO panel lines and world_rtao.json.

Limit: the stats are 32-bit sums of values x1000 and overflow above about 4.29 million traced pixels (4K is above that; 1080p is below it).

#### D5 owner steps (spatial filter)

Settings: History Clamp 0, Debug Off, Temporal on (unless a step says otherwise), Two-Sided discovery off, test rows off. The defaults are Spatial Filter off, Filter Type Separable bilateral, Radius 2, Passes 1, Taps 5 (equal to the earlier Medium).

Rows: Filter Type (separable bilateral or a-trous). Filter Radius 1 to 8 (separable: Gaussian sigma = radius / 2; a-trous: the ceiling of the step, max(1, radius / 2), no effect with 1 pass). Filter Passes 1 to 4 (separable: each pass is a horizontal and a vertical dispatch; a-trous: one 2D dispatch per pass, step min(2^i, ceiling), so 1, 2, 4 ...). Filter Taps (separable: samples per direction, 3 to 17, default 5; a-trous: kernel side, 3 to 9, default 5, binomial weights; an even value rounds up to the next odd one; Taps = 2 x radius + 1 reproduces the old separable Medium at any radius). Slot assignment: dispatch d reads A (d = 0) or the previous output and writes B on the last dispatch, otherwise F0 (even d) or F1 (odd d); it never reads and writes one texture.

Square pattern: cause and fix. The first version used the same step s = max(1, radius / 2) for every a-trous pass, so each output pixel depended only on pixels of its own (x mod s, y mod s) class; the response cos^(T-1)(s w / 2) has gain 1 at w = 2 pi / s, which left an s x s cell pattern from radius 4 up (s = 2 at radius 4 and 5, 3 at 6 and 7, 4 at 8); more passes at the same step did not reduce it. Now step_i = min(2^i, max(1, radius / 2)): the first pass is always dense, so |H_total(w)| <= cos^(T-1)(w / 2), and every lattice replica at 2 pi / 2^i falls on a zero of the previous step. Residual only at step 3 (radius 6 and 7, passes 3 or 4): taps 5 leaks 0.4 % of amplitude, taps 3 leaks 6.25 % (period 3). Separable stride 2 (old Low) is removed for the same reason. Harness guard: TestSpatialGridFree.

Old presets: old Medium = Passes 1, Taps 5 (separable at radius 2; set Taps = 2 x radius + 1 at other radii); old separable High = Passes 2; old Low and old a-trous at radius 4 and up are not reproduced (intended). A saved RtaoFilterQuality value is ignored (key removed).

1. Spatial Filter on with the defaults and Temporal on; then Temporal off. Compare each with Spatial Filter off on the same still frame (screenshots).
2. Filter Type separable bilateral against a-trous, at Radius 2 and Radius 6, Passes 1, 2 and 4, Taps 3, 5 and 9 for a-trous and 5, 9 and 17 for separable. A-trous at Radius 4 and up with Passes 2 to 4 must show no square or grid pattern (before: squares from Radius 4). Check edges (walls, foliage, thin objects) for halos or leaks.
3. Radius 1, 4 and 8 (separable, Passes 1, Taps 5).
4. Debug mode 8 "Filter change" with the filter on: a screenshot (shows where the filter changes the AO; scale x8).
5. Read the three cost lines at the top of the RTAO panel (RTAO pass A + B, Spatial filter, Total) and Mean change, passes, dispatches, taps and the a-trous steps line in the Denoising block for each case above.
6. Spatial Filter off again: the image must equal the one from before the feature (the "off" screenshot of step 1). With the filter off, debug mode 8 shows the unfiltered AO as a gray image; that is expected.
7. Send the RTAO panel lines (Denoising block), world_rtao.json (key spatial_filter) and the screenshots for each case. The spatial_filter object now has tap_count (effective odd taps) instead of quality.

Expected cost: a fraction of a millisecond to about 1 ms at 720p; the panel shows the measured value. The cost scales with passes and taps: separable 2 x passes dispatches of taps samples; a-trous passes dispatches of taps x taps samples (Passes 4 and Taps 9 = 4 x 81 samples per pixel). The constants (plane sigma 0.02, normal power 16) are initial values to be tuned from the A/B captures.

Known limits: (a) the filter targets (about 16.6 MB at 1080p) stay allocated while Spatial Filter is off, until a resize or device destroy. (b) Debug mode 8 with Spatial Filter off shows the unfiltered AO as a gray image. (c) The filter statistics are 32-bit sums of values x1000, like the other sums: they overflow above about 4.29 million filtered pixels (4K is above that; 1080p is below it). (d) a-trous Radius has no effect with 1 pass; (e) the step-3 residual above; (f) the Total line adds two timers that may come from different frames.

Verification (2026-10-09): addon build links (FXC: only X3556 integer-divide warnings); harness 12 of 12 programs PASS; the new test_rtao guards are TestSpatialPlan (grid of 896 combinations), TestSpatialDispatch (per-dispatch identity and real-plan pushes), TestSpatialKernelWeights, TestSpatialGridFree, TestSpatialLegacyEquality. Not run: the shader on the GPU, in-game validation.
