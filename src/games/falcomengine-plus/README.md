# Falcom Engine+

Falcom Engine+ is RenoDX's game-specific graphics addon for Falcom-engine games. Use this page to find the implementation, the game or generated resources it reads, and the research or upstream source behind a technique. Each technique description is kept to three sentences or fewer; linked source files remain authoritative for details.

## Technique index

### World capture and ray tracing

#### Path-traced diffuse GI

Sora 2nd path-traces diffuse indirect lighting using the captured world, the game sun and RTXDI Local lamps, the game ambient probe for sky light, and bounce albedo from the G-buffer on screen or a material cache learned from visible pixels off screen, keyed by each surface's dominant normal axis; with Path-Traced Reflection on, environment reflections and water use traced reflected rays (the optional Water Surface Reflection toggle traces water from a copy of the SSR surface), and game SSR and dynamic cubemaps are switched off at runtime. It needs RTXDI Local (scene transfer and lamps) and works with RTXDI Sun off; sun light at path vertices comes from scene constants and the path tracer's own shadow rays. SHARC (NVIDIA's radiance cache, world/sharc/vendor) ends paths early on cached light; its colour scale is tunable, it clears on map changes, loading screens, large sun changes and scale changes, and optional clamps on reflected rays (on by default) and bounce light (off by default) trade energy for fewer fireflies, and an optional SHARC update clamp (off by default) limits cache updates far above their cell's average.

Sparse Rendering, adapted from OpenRemix and off by default, uses IS-FAST blue noise to select opaque pixels when Ray Reconstruction is active; skipped lighting is zero and water remains dense.

**Code:** [world/pt](world/pt/), [world/shaders/world_pt_diffuse.cs_6_6.hlsl](world/shaders/world_pt_diffuse.cs_6_6.hlsl), [world/shaders/world_pt_sharc_update.cs_6_6.hlsl](world/shaders/world_pt_sharc_update.cs_6_6.hlsl), [world/shaders/world_pt_sharc_resolve.cs_6_6.hlsl](world/shaders/world_pt_sharc_resolve.cs_6_6.hlsl), [world/shaders/world_pt_material_write.cs_6_6.hlsl](world/shaders/world_pt_material_write.cs_6_6.hlsl).
**Reference:** Open Remix `pathtrace.comp`.

#### World capture and scene construction

The addon observes game draws and lighting inputs, classifies geometry from shader contracts, and builds CPU/GPU scene data for ray-traced effects. Static, alpha-tested, deforming, and moving geometry have different capture/admission paths, so start with the roadmap before changing this system.

**Code:** [`world/capture`](world/capture/), [`world/contract`](world/contract/), [`world/bvh`](world/bvh/), [`world/rt12`](world/rt12/), [`world/docs/ROADMAP.md`](world/docs/ROADMAP.md).

#### IS-FAST spatio-temporal noise

IS-FAST supplies precomputed blue-noise samples to opted-in effects so spatial samples vary across pixels and frames. Its texture payload is embedded in the addon rather than loaded from a game directory.

**Code:** [`addon.cpp`](addon.cpp); the embedded asset and consumers are listed in the resource map below.

#### Ray-traced ambient occlusion (RTAO)

RTAO traces visibility rays against the captured world scene and writes an AO term into the game's lighting path. Temporal reprojection and edge-aware spatial filtering are optional stages in the shared SVGF chain.

**Code:** [`world/rtao`](world/rtao/), [`world/shaders/world_rtao.cs_5_0.hlsl`](world/shaders/world_rtao.cs_5_0.hlsl), [`world/shaders/world_svgf.hlsli`](world/shaders/world_svgf.hlsli).

#### Ray-traced global illumination (RTGI)

RTGI traces diffuse indirect light and composites it into game lighting, with game sun and local-light data supplying direct illumination at ray hits. Its trace, history, denoising, and composite stages are separated in the world module and shader directory.

**Code:** [`world/rtgi`](world/rtgi/), [`world/shaders/world_rtgi.cs_5_0.hlsl`](world/shaders/world_rtgi.cs_5_0.hlsl).

#### ReSTIR GI and direct-light resampling

ReSTIR GI reuses weighted lighting samples across pixels and frames to reduce noise; RTXDI provides the SDK resampling structures and algorithms for the direct-light path. The game adapter validates captured surfaces and light identities before consuming a selected sample. The lamp jitter point is redrawn for every final shadow ray.

The RTXDI temporal pass runs the SDK boiling filter, which empties a reservoir whose weight is far above its 8x8 group's average, so rare bright samples do not spread into blotches. Spatial reuse applies the SDK disocclusion boost: pixels without temporal history take more neighbour samples. The Balanced preset (default) also enables SDK permutation sampling for temporal history lookups, which needs valid motion vectors, and initial visibility, which shadow-tests each pixel's initial light sample and drops it when blocked before reuse; the Performance preset turns both off, and the Quality preset keeps both on.

RTXDI local lights share a per-transfer-slot table across PT and RTXDI; its optional Stable Light Remap setting is Off by default and preserves the exact same-index temporal rule until enabled. The table hashes quantized point/spot identity and validates the original light tuple before mapping reordered lights; owner validation is pending.

The RTXDI panel provides session-only debug views over existing local-DI, sun, denoised-diffuse, and qualified distance outputs, plus bounded CPU diagnostics that can be copied or cleared without including scene contents or raw native addresses. Structured Lights/Tiles/Deferred rejections report the failing validation predicate and bounded SRV/resource descriptor evidence with session-local identity tokens. BVH debug presentation takes precedence over RTXDI views, which take precedence over legacy overlays. The earlier `SceneBindingChanged` rejection was not reproduced in the latest diagnostic capture: scene promotion and input transfer reached Ready. A separate local-enabled capture reaches the Lights structured-view check and reports `InvalidStructuredView`; its precise descriptor failure is still under investigation.

RTXDI shadow darkening scales how dark blocked lamp and sun shadows get on the environment and on characters, with one character slider per light. The raw lamp and sun lighting paths apply these settings; the RELAX denoiser input does not yet.

RTXDI sun visibility can be traced at half resolution, with a resolve pass that fills the other pixels from neighbours and falls back to full-resolution traces at edges. RTXDI sun and lamp shadows can sample a disk-shaped light for soft penumbras, using the shared IS-FAST noise, which the RTXDI scene pass copies itself when RTAO does not; on characters, lamp and sun shadow rays can also start from a point shifted with the disk sample so close self-shadows from arms, hair and clothing get soft edges. Sun shadows can also let tinted sunlight through alpha-tested leaf layers, and a share of sunlight through each wind-moved foliage object (opaque or alpha-tested) so moving foliage casts softer shadows.

RTXDI sun shadows with Light Size above 0 are averaged over recent frames, reprojected with motion vectors, history-tested against the previous depth and normal and clamped to the 3x3 neighbourhood, so soft penumbras are less noisy; at Light Size 0 nothing changes.

An optional RTXDI local light clamp (off by default) caps the luminance of each pixel's selected lamp by scaling its published weight, so lamp fireflies drop at the cost of energy; stored reservoirs are not changed.

**Code:** [`world/rtgi`](world/rtgi/), [`world/rtxdi`](world/rtxdi/), [`world/shaders`](world/shaders/), [`world/shaders/world_rtxdi_sun_visibility.cs_6_5.hlsl`](world/shaders/world_rtxdi_sun_visibility.cs_6_5.hlsl), [world/shaders/world_rtxdi_sun_accumulate.cs_6_5.hlsl](world/shaders/world_rtxdi_sun_accumulate.cs_6_5.hlsl), [`world_settings.hpp`](world/world_settings.hpp), [`lighting_0xCA3D8596.ps_5_0.hlsl`](sora2nd/lighting/lighting_0xCA3D8596.ps_5_0.hlsl).
**References:** [`reference/restir`](reference/restir/), vendored RTXDI [`README`](world/rtxdi/vendor/rtxdi/README.md) and [`license`](licenses/RTXDI-LICENSE.txt).

#### NRD RELAX adapter

The world D3D12 path prepares diffuse/specular signals and guide data for NVIDIA NRD's RELAX denoiser. Its input preparation and dispatch are separate from the shared SVGF chain.

**Code:** [`world/rt12/rt12_nrd.hpp`](world/rt12/rt12_nrd.hpp), [`world/rt12/rt12_rtxdi_transfer.hpp`](world/rt12/rt12_rtxdi_transfer.hpp), [`world/shaders`](world/shaders/).
**Context:** [`world/docs/ROADMAP.md`](world/docs/ROADMAP.md) describes its input contract and integration limits.

#### Ray-traced local shadows (RTLS)

RTLS traces visibility from shaded surfaces toward local lights. It keeps its own trace resources and can use the shared temporal/spatial denoising path.

**Code:** [`world/rtls`](world/rtls/), [`world/shaders/world_rtls.cs_5_0.hlsl`](world/shaders/world_rtls.cs_5_0.hlsl).

#### Ray-traced sun shadows

RT Sun traces visibility toward the directional sun and supplies a shadow term to game lighting. It can use the shared temporal/spatial denoising path.

**Code:** [`world/rtsun`](world/rtsun/), [`world/shaders/world_rtsun.cs_5_0.hlsl`](world/shaders/world_rtsun.cs_5_0.hlsl).

#### SVGF denoising

SVGF reprojects and validates temporal history, estimates variance, then applies an edge-aware a-trous filter. The shared chain is used by RTGI, RTAO, RTLS, RT Sun, GTVBAO, and GTVBGI paths that enable it.

**Code:** [`world/shaders/world_svgf.hlsli`](world/shaders/world_svgf.hlsli), [`world/rtgi/rtgi.hpp`](world/rtgi/rtgi.hpp).
**Reference:** [`reference/svgf`](reference/svgf/).

#### DLSS Ray Reconstruction

The RR integration observes the game's D3D11 NGX session and initializes a separate D3D12 NGX runtime for the add-on. Its ownership and denoiser-bypass logic is isolated from the effect implementations.

**Code:** [`world/rr`](world/rr/).

### Screen-space lighting and reflections

#### GTVBAO

GT-VBAO computes screen-space ambient occlusion by evaluating visibility bitmasks from depth and surface data, then filters and upscales the result. The local source identifies the upstream project, paper basis, and MIT license.

**Code:** [`gtvbao`](gtvbao/), [`gtvbao/GTVBAO.h`](gtvbao/GTVBAO.h).
**Reference:** [GameTechDev/GT-VBAO](https://github.com/GameTechDev/GT-VBAO), linked from the source header.

#### GTVBGI / VBGI

GTVBGI also uses visibility bitmasks to estimate indirect illumination. Far-field and multi-bounce paths have their own passes and history resources.

**Code:** [`gtvbao`](gtvbao/), especially [`gtvbao_vbgi_main.cs_5_0.hlsl`](gtvbao/gtvbao_vbgi_main.cs_5_0.hlsl), [`vbgi_radiance.cs_5_0.hlsl`](gtvbao/vbgi_radiance.cs_5_0.hlsl), and [`vbgi_far_temporal.cs_5_0.hlsl`](gtvbao/vbgi_far_temporal.cs_5_0.hlsl).
**Source note:** the main-pass comment credits Mirko Salm's Shadertoy GT-VBGI implementation.

#### Falcom SSGI

Falcom SSGI reshapes Kai's existing screen-space indirect-light signal in supported lighting shaders; it does not run GTVBGI's visibility-bitmask gather.

**Code:** [`kai/lighting`](kai/lighting/), shared settings in [`shared.h`](shared.h).

#### Character SSGI

Character SSGI adjusts how Falcom's sampled SSGI signal contributes to character lighting. It is part of the Kai lighting shader path, not a separate GI pass.

**Code:** [`kai/lighting`](kai/lighting/), settings and shared data in [`addon.cpp`](addon.cpp) and [`shared.h`](shared.h).

#### Contact shadows

Contact shadows ray-march the screen-space depth buffer from a shaded surface toward the light and write a visibility term. The compute variant adds local shadowing and a separate denoise stage; TAA can stabilize the jittered samples downstream.

**Code:** [`shadows`](shadows/), [`addon.cpp`](addon.cpp) for input capture and dispatch.
**Reference:** [`reference/contact_hardening_soft_shadows.txt`](reference/contact_hardening_soft_shadows.txt) and the source comments.

#### Screen-space reflections

The game-specific SSR shaders trace or sample the screen-space scene for reflections and apply game-specific history logic. Their inputs and reprojection details differ by game, so inspect the matching shader before changing its resource bindings.

**Code:** [`sora1st` SSR](sora1st/ssr/), [`sora2nd` SSR](sora2nd/ssr/), [`Kai` SSR](kai/SSR/).
**Reference:** [`reference/SSR`](reference/SSR/) and [`SSR`](SSR/).

#### Vanilla SSR improvements

These controls fix or refine the game's existing SSR march, history blend, and disocclusion checks. The feature is implemented inside the Sora 2nd SSR shader replacement and selected by addon hooks.

**Code:** [`sora2nd/ssr`](sora2nd/ssr/), control and hook selection in [`addon.cpp`](addon.cpp).

#### Dynamic cubemaps

Dynamic cubemaps capture and filter scene radiance into environment textures used for reflections, including specular irradiance and parallax-corrected sampling. This code is a Falcom Engine+ port and modification of Skyrim Community Shaders' Dynamic Cubemaps.

**Code:** [`dyncube`](dyncube/).
**Source and terms:** [`licenses/dyncube-license.txt`](licenses/dyncube-license.txt), [`reference/Dynamic Cubemaps`](reference/Dynamic%20Cubemaps/).

### Temporal, post-processing, and game-specific techniques

#### Temporal anti-aliasing (TAA)

The custom TAA passes reproject the previous frame, validate history using depth/color information, and accumulate it with a motion-sensitive feedback path. Both Sora games share the reprojection helper while keeping game-specific shader entry points.

**Code:** [`taa`](taa/), [`taa/taa_reproject.hlsli`](taa/taa_reproject.hlsli), [`sora1st/taa`](sora1st/taa/), [`sora2nd/taa`](sora2nd/taa/).
**Reference:** [`reference/taa`](reference/taa/).

#### Motion blur

Motion blur converts the game's motion vectors to UV velocity, reduces the field to tile and neighborhood maxima, then gathers and composites along motion directions. Its motion-source options and capture constraints are mapped below.

**Code:** [`motionblur`](motionblur/), input selection and capture in [`addon.cpp`](addon.cpp).
**Reference:** [`reference/motionblur`](reference/motionblur/).

#### Depth of field (DoF)

DoF generates a circle-of-confusion field and gathers nearby color samples to form the bokeh result. Shared shader logic is used by per-game entry points.

**Code:** [`dof`](dof/), [`sora1st/dof`](sora1st/dof/), [`sora2nd/dof`](sora2nd/dof/), [`kai/DOF`](kai/DOF/), [`daybreak2/dof`](daybreak2/dof/).

#### FXAA

FXAA provides three shader variants for edge detection and quality tradeoffs. The game-specific replacement points are selected by shader hash.

**Code:** [`fxaa`](fxaa/), game-specific registration in [`addon.cpp`](addon.cpp).

#### RCAS sharpening

RCAS sharpens the post-TAA image and can adapt its strength using captured motion. The resource map below identifies that input.

**Code:** [`rcas`](rcas/), setup and motion capture in [`addon.cpp`](addon.cpp).
**Reference:** [`reference/rcas`](reference/rcas/).

#### Volumetric fog

Volumetric fog is implemented through game-specific shader replacements, with IS-FAST jitter and haze anti-aliasing available in supported variants. The game-specific constant-buffer layouts and shader hooks are kept with each game's shader.

**Code:** [`sora1st/volfog`](sora1st/volfog/), [`sora2nd/volfog`](sora2nd/volfog/), [`kai/volfog`](kai/volfog/), [`daybreak2/volfog`](daybreak2/volfog/), controls in [`addon.cpp`](addon.cpp).

#### Fog color correction

Fog Color Correction adjusts the fog contribution in supported game lighting shaders. Its controls are passed through the add-on shader-injection data.

**Code:** game lighting and fog shaders in [`kai`](kai/), [`sora1st`](sora1st/), [`sora2nd`](sora2nd/), settings in [`addon.cpp`](addon.cpp) and fields in [`shared.h`](shared.h).

#### Character shadowing

Character Shadowing adds a camera-space contact pass and combines its result in supported character-lighting shaders. The mode and input resources are handled by the add-on and per-game lighting replacements.

**Code:** [`addon.cpp`](addon.cpp), [`shadows`](shadows/), [`sora1st/lighting`](sora1st/lighting/), [`sora2nd/lighting`](sora2nd/lighting/), [`kai/charlighting`](kai/charlighting/), [`daybreak2/char`](daybreak2/char/).

#### Character outline

The character-outline replacement adjusts the Sora 2nd outline pixel shader. Its shader is grouped with other Sora 2nd character passes.

**Code:** [`sora2nd/char`](sora2nd/char/).

#### Kai hero light

The hero-light feature identifies configured player lights in Kai's light records and patches their contribution. The player-light mapping and patching logic are explicit rather than tied to a shader hash.

**Code:** [`kai_hero_light.hpp`](kai_hero_light.hpp), [`kai_player_light_table.h`](kai_player_light_table.h), settings and light-data handling in [`addon.cpp`](addon.cpp).

#### BRDF improvements

Supported lighting shaders can blend in Hammon diffuse, multiscattering GGX specular, and roughness/highlight limits. These are game-lighting replacements, not a standalone post-process.

**Code:** [`sora1st/lighting`](sora1st/lighting/), [`sora2nd/lighting`](sora2nd/lighting/), [`kai/lighting`](kai/lighting/), [`reference/brdf.hlsli`](reference/brdf.hlsli).

#### Foliage shader replacements

Game-specific foliage shaders adjust alpha-tested, static, and moving foliage materials. This material shading work is separate from world ray-tracing admission and geometry updates.

**Code:** [`sora1st/foliage`](sora1st/foliage/), [`sora2nd/foliage`](sora2nd/foliage/), [`kai/foliage`](kai/foliage/), world path: [`world/bvh`](world/bvh/).

#### Foliage grass AO

A shared helper applies a root-to-tip occlusion gradient to supported grass and clutter shaders. Its base, tip, and curve values are passed through shader-injection data.

**Code:** [`foliage/grass_ao.hlsli`](foliage/grass_ao.hlsli), including shaders in [`kai/foliage`](kai/foliage/) and [`sora1st/foliage`](sora1st/foliage/).

#### Shadow-map filtering

Game-specific shadow shaders adjust filtering and shadow response while preserving the game's shadow-map passes. Sora shadow shaders and Kai's PCSS shader are kept with their respective game lighting code.

**Code:** [`sora1st/shadow`](sora1st/shadow/), [`sora2nd/shadow`](sora2nd/shadow/), [`kai/lighting/pcss_0x30190562.ps_5_0.hlsl`](kai/lighting/pcss_0x30190562.ps_5_0.hlsl), settings and hooks in [`addon.cpp`](addon.cpp).

#### Game SSAO shader replacements

These replace or modify the games' existing SSAO and combined character-shadow pixel shaders. They are separate from the standalone GTVBAO compute passes.

**Code:** [`sora1st/ssao`](sora1st/ssao/), [`sora2nd/ssao`](sora2nd/ssao/), lighting-resource capture in [`addon.cpp`](addon.cpp).

#### Sky shader replacement

The Sora 1st sky pass has a game-specific pixel-shader override. Its shader is isolated from screen-space reflection and dynamic-cubemap code.

**Code:** [`sora1st/sky`](sora1st/sky/).

## Shared game resources

The addon captures resources from specific game shader bindings and draw events; a register number alone is not enough to identify a resource. Capture and validation logic is in [`addon.cpp`](addon.cpp) and [`world/capture`](world/capture/).

| Resource | Where the addon gets it | Main consumers and tracing pointers |
|---|---|---|
| Scene depth | Captured from a recognized lighting shader's depth SRV: t4 in Sora shaders and t3 in Kai shaders. Binding constants and hash-gated capture are in [`addon.cpp`](addon.cpp); world lifetime/size tracking is in [`world/capture/depth_source.hpp`](world/capture/depth_source.hpp). | World ray tracing, GTVBAO/VBGI, contact shadows, and depth-aware temporal validation. Other passes may bind their own depth input; check that pass's shader and hook. |
| G-buffer normal | Captured from t1 (`mrtTexture0`) on recognized lighting shaders. The game resource is held in add-on state and bound to passes that need surface orientation. | RTAO, RTGI, RTLS, RT Sun, GTVBAO/VBGI, contact shadows, and edge-aware filters. Search `captured_mrt_normal_srv` in [`addon.cpp`](addon.cpp). |
| Vanilla SSAO | Captured from t5 for Sora or t4 for Kai on the recognized lighting shader. This lets replacement/composite paths inspect or preserve the game's AO input. | GTVBAO activation and the game-specific lighting paths. See the register constants and `captured_ssao_srv` in [`addon.cpp`](addon.cpp). |
| Scene constants (`cb_scene`) | Captured from b0 on lighting draws; draw-time values and ranges are tracked rather than assuming one stable buffer offset. | Camera transforms and reconstruction in world effects, shadow/fog passes, and game shader replacements. See [`world/capture/cb_tracking.hpp`](world/capture/cb_tracking.hpp), [`world/capture/cb_value_tracker.hpp`](world/capture/cb_value_tracker.hpp), and `captured_scene_cbv` in [`addon.cpp`](addon.cpp). |
| Motion vectors: G-buffer RTV4 | Captured from the engine's RTV4 during render-target binding. `MBMotionRtv4Ensure` validates it, creates an add-on-owned SRV, and checks that dimensions match depth before world temporal passes use it. | Temporal RTAO, RTGI/ReSTIR GI, RTLS, RT Sun, GTVBAO/VBGI, RTXDI temporal DI, and the default motion-blur input. If it conflicts with TAA t3, world temporal reuse is rejected; see `UpdateRtMotion` in [`addon.cpp`](addon.cpp). |
| Motion vectors: TAA t3 | Captured from t3 only on recognized Sora TAA shader draws; this resource is not assumed ready before lighting. | Custom TAA, motion-adaptive RCAS, and the optional Sora motion-blur input. Kai motion blur uses RTV4. See the t3 capture in [`addon.cpp`](addon.cpp), [`taa_reproject.hlsli`](taa/taa_reproject.hlsli), and RCAS/motion-blur dispatch setup there. |
| World geometry and transforms | Draw callbacks and per-command-list SRV/constant-buffer tracking observe game vertex/index buffers, instance data, and draw-time transforms. The pool and scene builders copy/convert admitted data into add-on-owned CPU/GPU structures. | World BVH and DXR paths. Start at [`world/capture/draw_census.hpp`](world/capture/draw_census.hpp), [`world/capture/cb_tracking.hpp`](world/capture/cb_tracking.hpp), [`world/bvh/bvh_pool.hpp`](world/bvh/bvh_pool.hpp), and [`world/rt12`](world/rt12/). |
| Sora 2nd lighting and RTXDI inputs | The lighting callback gathers depth, normal, scene constants, color, MRT material buffers, motion, tiled-light indices, local-light records, and deferred data. It validates and packs these views for the separate D3D12 path. | RTXDI local-light/sun visibility and its temporal DI pass. Follow input assembly in [`addon.cpp`](addon.cpp), resource transfer in [`world/rt12/rt12_rtxdi_transfer.hpp`](world/rt12/rt12_rtxdi_transfer.hpp), and shader input contracts in [`world/rtxdi`](world/rtxdi/). |
| Local-light records | The Sora 2nd lighting path exposes `dynamicLights_g` and `lightIndices_g`; the RTXDI input bridge snapshots the game data for its D3D12 work. | RTGI local lights and RTXDI direct-light sampling. Kai hero-light records are handled separately by [`kai_hero_light.hpp`](kai_hero_light.hpp) and the Kai lighting path. |
| IS-FAST blue noise | The precomputed `fast_noise_ea.dds` data is embedded in [`fast_noise_ea.h`](fast_noise_ea.h), so no external game-folder texture is needed. Add-on helpers create/bind the texture for features that opt in. | Contact shadows, DoF, fog, VBGI/GTVBAO, SSR, shadows, and other sampling passes; search `isfast_noise_srv` and `IS-FAST` in [`addon.cpp`](addon.cpp) for a consumer's exact binding. |
| Effect histories and intermediate outputs | Allocated by the effect that owns them; they are not game-provided textures. Per-feature resource headers define their formats, lifetime, and ping-pong/history slots. | See `*_resources.hpp` beside each world effect, [`world/rtgi/rtgi.hpp`](world/rtgi/rtgi.hpp) for shared denoising resources, and each effect's resource setup in [`addon.cpp`](addon.cpp). |

### Motion vectors versus camera reprojection

Do not assume every temporal effect samples the same motion texture. World temporal effects, including custom ReSTIR/RTXDI paths, use validated RTV4; motion blur can select RTV4 or TAA t3, and RCAS reads TAA t3 for motion-adaptive sharpening. SSR shader variants reproject with previous view/projection constants, so they do not consume either shared motion texture for that step.

## Reference library

General papers, research notes, and reference implementations are grouped under [`reference`](reference/) and [`SSR`](SSR/). Each technique entry above points to its specific source or license when one is tracked; game shader evidence is under [`world/reference`](world/reference/) and [`world/research`](world/research/).

## Where to continue

- Read [`AGENTS.md`](AGENTS.md) for the required workflow, scope rules, and the README maintenance rules.
- Read [`world/docs/ROADMAP.md`](world/docs/ROADMAP.md) for current world-ray-tracing state, open work, invariants, and subsystem design notes; [`world/docs/HISTORY.md`](world/docs/HISTORY.md) preserves prior decisions and evidence.
- Read [`settings_ui.h`](settings_ui.h) for settings categories and [`addon.cpp`](addon.cpp) for feature registration, resource capture, and dispatch wiring.

This README is a navigation and data-flow index, not a status log or a replacement for subsystem documentation.
