# Far Cry 6 camera jitter investigation

Target: `FC_m64d3d12.dll`, SHA-256
`efe842af2a4b1cd78c56ca6d2d9b98669099f865ccbb1e3878578a071bf2a7ae`.
Addresses below are preferred-image virtual addresses, not ASLR-adjusted runtime
addresses. They apply only to that binary.

The camera constant-buffer captures show 1280x720 perspective data at b0:

- `0x80`: projection matrix.
- `0x140`: current view-rotation projection matrix.
- `0x180`: previous view-rotation projection matrix.
- `0x200`: viewport dimensions.
- `0x2c0`: `MotionVectorOffsets` (semantics and effect not yet established).
- `0x334`: `JitterFrame`, the observed two-phase frame value.
- `0x338`: `IsViewPortOffScreenRendering`.

The DLL registers the name `JitterFrame` with offset `0x334` at instruction
`0x1808549e2`. It registers `JitterParameters` with offset `0x60` at
`0x180853354`, consistent with the observed TAA constants. It also registers
`JitterDistance` with offset `0x124` at `0x180853c1a`, but its structure and
meaning are not yet established. These are metadata registration sites, not
proof of the code that computes jitter.

The camera names and offsets above are registered together by the function at
`0x1808543f0`, which declares a `0x340`-byte layout; this corroborates the
camera layout rather than identifying its writer. The sampled terrain vertex shader `0xA38182D8` reads the current matrix
at b0 registers 20-23, but its disassembly does not read b0 register 44, where
`MotionVectorOffsets` begins. Other shaders and passes have not been ruled out.

The DLL names the containing type `CViewportShaderParameterProvider`; its
constructor is at `0x180532450` and initializes an object of `0x470` bytes.
The first `0x340` bytes after the `0x20`-byte base appear to contain the camera
layout, followed by textures whose first registered offset is `0x360`. In the
function beginning `0x18089d350`, instructions starting at
`0x18089d7a0` copy `0x340` bytes from one provider's `+0x20` camera data to a
new provider. This is a candidate upstream copy, not yet established as the
main gameplay camera or the sole upload path. It must not be hooked based on
the static match alone.

`0x1808a7b90` is a stronger candidate: it receives a
`CViewportShaderParameterProvider` as its first argument, writes the declared
`JitterFrame` at object offset `0x354`, and writes the declared projection and
view-rotation projection at `0xa0` and `0x160`. Its eighth argument points to
two floats used in matrix construction; its ninth argument is written to
`JitterFrame`. Direct callers include `0x1806c5419` and `0x180864196`. This
establishes their static use, not the eighth argument's units or whether a
particular call is the main gameplay view. Runtime validation is required.

In four recorded intervals, 610 observed scene draws per interval, including
88 terrain draws, used the same camera-buffer range. The physical upload-ring
offset alternated between `1024` and `5120`. Those offsets and handles are
runtime observations, not stable identifiers. Eight other draw bindings per
interval were unresolved, and two more perspective candidates used different
ranges. CPU and GPU copies matched in an earlier one-buffer capture; that does
not establish every later copy's identity.

The descriptor-copy experiment created zero-jitter camera copies and passed
zero to DLSS. It observed successful descriptor substitutions, but full scene
coverage and corresponding motion-vector behavior were not verified. It caused
an unacceptable FPS drop even after filtering. Avoid treating descriptor-copy
interception as a completed integration or enabling it by default.

Next, identify and verify the code that generates or uploads the camera block
before its descriptors are copied. Any candidate hook must be tested for camera
identity, thread ordering, temporal consistency, and motion-vector behavior.
The private matrix-adjustment math in `camera_jitter.hpp` has been checked on a
captured camera sample, but has not established an upstream hook or a working
Halton sequence.

## Object motion and Halton

With Halton active, the report dated 2026-10-27 recorded 19,804 matching
camera and scene samples, no mismatches, and one missing sample. A four-frame
center-pixel capture on a character showed decoded motion varying against the
Halton phase while the camera reprojection estimate was near zero. Character
animation, filtering and selected depth can also affect these values, so this
is evidence of a possible jitter component, not a measurement of its exact
size.

The user tested DLSS `MVJittered`: on improved characters and foliage but made
the background worse. Leave it off for the current setup. A global flag cannot
describe both paths if their motion conventions differ.

DevKit capture identified compute shader `0xD9289B6A`, which produces the
640x360 packed motion buffer consumed by DLSS. It selects a depth-neighborhood
sample, tests bit 3 in alpha of its full-resolution t2 input, and on that branch
copies t3.xy; otherwise it derives camera motion from a reprojection matrix.
The t2 bit is therefore a promising classification signal, but its coverage and
the exact correction sign and size remain unverified. This pass selects and
downsamples before the addon linearly filters the packed motion, so simply
sampling a full-resolution flag at the output pixel would not reproduce the
producer's choices at edges. DevKit's post-frame readback of t2/t3 was not a
reliable copy of their contents at dispatch time.

The next implementation must preserve the producer's selected-pixel flag at
dispatch time and apply a matching per-pixel jitter correction while decoding
motion. It must account for filtering, frame correspondence and the lifetime
of the game resources, and report whether the mask and correction were used.
Validate it against the captured object and background before enabling it by
default; do not infer object coverage or correctness from the existing camera
match counters.

## Low-overhead implementation investigation

In the live DevKit snapshot, the motion producer's u0 resource was the same
640x360 resource reported at TAA t0. The report shows that t0 alternates between
two resource handles. A candidate design is to associate a half-resolution
object mask with the actual u0 resource, and resolve it by the actual t0 resource
at evaluation. This is stronger than using a global last-frame mask, but does
not by itself prove execution order or safe reuse when multiple command lists,
queues, or frames are in flight. The addon must verify those lifetimes and
fall back when a matching mask cannot be established.

One R8 mask at 640x360 is 230,400 bytes (225 KiB); two consume 450 KiB before
GPU allocation overhead. A GPU-only mask generated at the producer avoids CPU
readback, `wait_idle`, and the expensive descriptor-copy callback. GPU time and
the cost of state tracking are not measured yet. The cheapest potential design
is to emit the selected flag in the producer itself, but an extra UAV requires
a compatible pipeline/root signature and shader replacement. A separate compute
pass could use the same depth and flags inputs, but must reproduce the producer's
depth selection and restore its compute state. RenoDX provides
`state::GetSnapshot` and `CommandListSnapshot::ApplyCompute` for state capture
and replay; use requires checking that snapshot tracking is active and complete.
The game addon currently enables shader pipeline tracking, but does not itself
enable `state::use_snapshot`; `GetSnapshot` can therefore return no snapshot.
Enabling complete tracking may add CPU cost and must be measured. Do not assume
that restoring the graphics descriptor state after DLSS also restores the game's
compute pipeline and root bindings at the earlier producer dispatch.

The unpack shader currently linearly filters *encoded* motion before a nonlinear
decode. Filtering a binary flag at the output pixel and adding an offset after
decoding is therefore an approximation near object edges; even a filtered mask
does not make the nonlinear decode commute with correction. Compare candidate
edge treatments against the original path with a fixed synthetic input and in
the game. Neither marker coverage nor correction magnitude/sign is established
for all objects. A guarded toggle, frame/mask identity counters in Copy Report,
and an FPS comparison are required before calling the correction working.

Implementation started: `dlss_object_motion_mask.cs_6_0.hlsl` generates an R8
mask from the producer's linear depth and flags. It mirrors the producer's
ordered 3x3 neighborhood of 2x2 gathers and tests bit 3 at the selected sample.
The shader compiles with DXC, and synthetic gather/tie-order cases were checked.
GPU comparison against the original producer and sampler/addressing details
remain open.

`Motion Pairing Probe` is an opt-in CPU-side diagnostic. It matches the
producer's u0 resource against TAA's resolved motion input within the same
present interval and counts same-list, different-list, and unresolved cases in
Copy Report. A match indicates command recording identity only; it does not
prove command submission order, GPU completion, queue dependencies, or mask
lifetime. The probe neither dispatches the mask shader nor changes motion.

The 2026-10-27 pairing report recorded 428 producer/evaluation pairs on different
command lists, all 428 submitted to the same queue with the producer callback
first, and no different-queue or reversed pairs. This supports investigating an
ordered GPU-only handoff for this session; it does not prove all future frames
or safe reuse of descriptors and mask resources while the GPU is behind.

`dlss_mv_masked_unpack.cs_6_0.hlsl` is an experimental unpack shader.
It preserves the existing packed-motion filter and adds an object-mask-weighted
current-minus-previous jitter delta, with an explicit correction scale. The
sign, edge interpolation and actual mask coverage remain unverified. The addon
now dispatches both shaders under the opt-in Object Motion Correction test,
which requires DevKit descriptor tracking and a restart after enabling state
snapshots. A mask is paired by packed resource and present interval; correction
falls back when the queue, Halton history or mask is unavailable. The producer
compute state is captured and restored around mask generation. These mechanisms
and the FPS cost have not been verified in game, and a prior same-queue observation
does not prove queue identity or resource safety for every later frame.

## Object motion shader follow-up

The 2026-10-28 report with strength 0 confirmed that the masked unpack output
matched the decoded game motion, and the user reported less shaking than with
the other tested strengths. This does not prove that the original object
motion is correct or rule out edge filtering and animation effects.

In the active DevKit snapshot, draw 1670 bound 0xD8F98738 as its pixel shader
and the motion producer's full-resolution t3 resource as RTV2. Its DXIL writes
encoded motion to target 2 xy. The calculation reads viewport cb0 register 44
(offset 0x2c0, MotionVectorOffsets) and uses its xy components with SV_Position
and the viewport scale before encoding. Its viewport cb0 used the same buffer
and offset as a sampled scene camera buffer in this snapshot. Other object
shaders and the exact meaning of this field remain unverified. Copy Report now
records all four CPU-read values beside the scene matrix for comparison; these
values are candidates, not a per-draw GPU verification.

The 2026-10-28 report shows sampled `MotionVectorOffsets.xy` alternating
between (+0.25,-0.25) and (-0.25,+0.25), while the camera follows Halton.
The guarded experimental camera-hook patch attempts to replace provider
`+0x2e0` with Halton pixel offsets only when its existing xy match the original
camera input converted to pixels. The buffer observation and one object shader
make this a candidate, not proof of timing, coverage, sign or visual improvement.
Copy Report records guard counts and the later sampled camera-buffer values; a
patch count alone does not prove that the GPU consumed the new value.

The following report found the first camera-hook attempt rejected 515/517
providers and the scene buffer retained the original two-phase values. A
bounded scan of the DLL's PE unwind-described functions found a later candidate
writer: function `0x1806cbe10`, instruction `0x1806cbeed`, copies eight bytes
from its second argument +0x70 to its first argument +0x2e0. Nearby writes set
the provider's viewport at +0x220. Static analysis does not establish which
runtime view invokes it or its ordering relative to the Halton camera builder.
The experimental hook now runs after this writer and requires matching render
size, projection, current-frame Halton history and original two-phase offsets.
Copy Report must establish whether it matched and whether the later scene
buffer actually contains the desired offsets; visual and GPU verification remain.

## Aiming-down-sights investigation

The 2026-10-29 aiming report captured four decoded center-pixel motion samples
with the Halton camera and motion-offset patch enabled. The samples correlated
with the current jitter: separate least-squares slopes were approximately 0.55
for x and 0.45 for y, with correlations 0.98 and 0.88. The camera reprojection
estimate was close to zero. This is evidence of a jitter-related component at
that pixel, not a calibrated correction: four samples cannot distinguish weapon
animation, filtered object edges, prior-frame jitter or GPU frame mismatches.
The object mask was unavailable, so the samples do not establish which motion
branch supplied the pixel.

In a live aiming snapshot, the motion producer `0xD9289B6A` u0 and the TAA
draw's t0 were the same half-resolution texture. A captured geometry draw using
pixel shader `0xD8F98738` wrote the full-resolution texture bound as the motion
producer's t3 and also wrote its flags input t2. Its vertex shader contains leaf
data, so this draw cannot be identified as the weapon. The producer selects t3
only for the depth-selected sample whose t2 alpha has bit 3 set; otherwise it
computes camera motion. Live readbacks of these transient resources are not
frozen at the producer dispatch and must not be used to classify the weapon.
Comparing snapshots while changing aim also changes visible scene draws; shader
counts alone did not identify the weapon. A dispatch-time capture of the chosen
depth, flag, source motion and decoded motion at the same pixel over more Halton
phases is needed before choosing a correction or modifying the weapon path.

In a subsequent weapon-out / holstered comparison, pixel shader `0xF0520574`
appeared only with the weapon out and wrote the producer's t2/t3 resources, but
the user bypassed it and identified it as likely foliage, not the weapon. Pixel
shader `0x8A7C5124` also appeared only with the weapon out, but its bypass did
not identify the weapon or arms. Its render target was not the TAA color input
in the captured frame. Those snapshot differences alone do not identify the
first-person model; shared shaders, culling and transient resource reuse remain
possible. Find the first-person draws by visual isolation and then trace their
actual outputs before choosing a correction.

The next comparison included per-draw bindings. In the weapon-out snapshot,
the user confirmed vertex shader `0xD1E29DD9` as weapon geometry by bypassing
it; nearby draws 60 and 64 were identified as arm and accessory geometry. The
three captured pipelines used vertex/pixel pairs `0x0928F35C/0x74B8E3E2`,
`0xCBF3A279/0x15CED94D`, and `0xD1E29DD9/0x86D6B1FC`. All three used the same
viewport b0 buffer range, distinct from the main scene range in that snapshot,
and wrote the flags and full-resolution motion textures consumed by the motion
producer. The two arm/weapon vertex shaders `0xCBF3A279` and `0xD1E29DD9`
read current matrix registers 20-23 and previous matrix registers 24, 25 and
27 from b0. The weapon pixel shader also reads b0 register 44. This supports a
separate first-person camera view, but neither the camera-buffer contents at
draw execution nor consistent handling across other weapons is yet proven.
Snapshot draw indices, buffer offsets and material handles are not stable IDs.

The subsequent four-interval Camera Coverage report read the confirmed weapon
and accessory viewport b0 on the CPU. In every interval their projection jitter
was the game's alternating `(-0.25, 0.25)` and `(0.25, -0.25)` pixels; main
scene draws in the same CPU intervals instead used Halton samples
`(-0.375, -0.0556)`, `(0.4375, -0.3889)`, `(-0.0625, 0.4630)`, and
`(0.1875, 0.1296)`. DLSS receives the scene Halton value. This is a concrete
first-person/world jitter mismatch and a strong candidate for the observed
shaking. The readings are CPU-side and do not by themselves prove the GPU read
the same bytes. Identify where the first-person provider is built or copied and
verify its own matrices and motion offsets before changing it; the existing
scene-camera hook's match counters do not establish first-person coverage.
The next report recorded zero matching two-phase perspective projections at
the existing post-parameter writer with Halton motion offsets and detailed
diagnostics enabled. This does not establish whether the weapon was visible
throughout the session or whether its provider traverses that writer at a
different stage. A temporary diagnostic at the existing camera builder also showed zero matching builder
calls with detailed diagnostics enabled; this does not prove the first-person
view was active during the whole measurement. In a short live trace of the
provider copy constructor at `0x18063ea50`, 852 calls produced no source
matching the observed full-size two-phase projection. Monitoring the mapped
upload range produced no access events, so it did not identify its writer. The
temporary instrumentation was detached. None of these negative results proves
that the first-person provider can never traverse those paths.

Offline x64 analysis identified another camera builder at `0x1806cb5b0`.
It copies a viewport provider through `0x180644930`, then builds projection,
view-projection, current clip and previous clip fields using the jitter pair at
its third argument +`0x68`. The third argument's +`0x20` and +`0x24` dimensions
are also read by the existing parameter writer, and direct call sites pass the
same view object to both routines. The builder's fifth argument selects a
no-jitter branch. These static facts do not yet prove that it creates the
weapon view. An opt-in, version-checked experiment now supplies a copied view
with Halton jitter to that builder only for full-render-size, two-phase views;
it reports attempted calls, projection matches and motion-offset patches. The
engine constructs the matrices itself. The user reported that this fixed the
first-person shaking. Coverage across other weapons, motion, and unrelated views
has not been established. Its source is specific to the DLL identified above.
