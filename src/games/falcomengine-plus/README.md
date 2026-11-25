# Falcom Engine+

Falcom Engine+ is RenoDX's game-specific graphics addon for Falcom-engine games. Use this page to find the implementation, the game or generated resources it reads, and the research or upstream source behind a technique. Each technique description is kept to three sentences or fewer; linked source files remain authoritative for details.

## Technique index

### Framework compatibility

An optional restart-required setting keeps Falcom Engine+'s pipeline-layout tracking separate from other RenoDX add-ons, allowing incompatible tracker layouts to be tested together. It is off by default.

**Code:** [`framework_compat.hpp`](framework_compat.hpp), [`addon.cpp`](addon.cpp).


### World capture and ray tracing

#### Unified path tracing (ReSTIR PT)

Sora 2nd uses one RTXDI ReSTIR PT path tree per pixel to shade direct light, diffuse GI and reflections from the captured world, RTXDI Local lamps and the game sun; that reservoir replaces the game's sun, lamp and ambient terms for environment and characters, whose direct shading uses the PT GGX/Lambert BRDF instead of toon terms, while game-toon characters and pixels without PT direct light receive one world-traced sun visibility sample on top of the game's CSM and toon sun plus shadowed reservoir lamp light through the character reflection channel. Temporal and spatial reservoir reuse, a boiling filter, duplication-based history reduction, stagnancy decorrelation and compatibility-guided neighbours are selectable under Path Tracing Resampling; SHARC, ReGIR, Sparse Rendering, the material cache, the Water Rule and IS-FAST are kept on the unified path; it needs RTXDI Local (scene transfer and lamps) and works with RTXDI Sun off. Water Surface reflection runs the same ReSTIR PT passes on the SSR-time water surface with its own reservoir and camera-reprojected history.

**Code:** [world/pt](world/pt/), [ReGIR registration](world/shaders/world_regir_register.cs_6_5.hlsl), [ReGIR weights](world/shaders/world_regir_weights.cs_6_5.hlsl), [ReGIR build](world/shaders/world_regir_build.cs_6_5.hlsl), [world/pt/pt_regir_contract.hpp](world/pt/pt_regir_contract.hpp), [world/shaders/world_pt_initial.cs_6_6.hlsl](world/shaders/world_pt_initial.cs_6_6.hlsl), [world/shaders/world_pt_temporal.cs_6_6.hlsl](world/shaders/world_pt_temporal.cs_6_6.hlsl), [world/shaders/world_pt_spatial.cs_6_6.hlsl](world/shaders/world_pt_spatial.cs_6_6.hlsl), [world/shaders/world_pt_duplication.cs_6_6.hlsl](world/shaders/world_pt_duplication.cs_6_6.hlsl), [world/shaders/world_pt_smoothed_duplication.cs_6_6.hlsl](world/shaders/world_pt_smoothed_duplication.cs_6_6.hlsl), [world/shaders/world_pt_neighbor_selection.cs_6_6.hlsl](world/shaders/world_pt_neighbor_selection.cs_6_6.hlsl), [world/shaders/world_pt_final.cs_6_6.hlsl](world/shaders/world_pt_final.cs_6_6.hlsl), [world/shaders/world_pt_water_initial.cs_6_6.hlsl](world/shaders/world_pt_water_initial.cs_6_6.hlsl), [world/shaders/world_pt_water_temporal.cs_6_6.hlsl](world/shaders/world_pt_water_temporal.cs_6_6.hlsl), [world/shaders/world_pt_water_spatial.cs_6_6.hlsl](world/shaders/world_pt_water_spatial.cs_6_6.hlsl), [world/shaders/world_pt_water_neighbor_selection.cs_6_6.hlsl](world/shaders/world_pt_water_neighbor_selection.cs_6_6.hlsl), [world/shaders/world_pt_water_duplication.cs_6_6.hlsl](world/shaders/world_pt_water_duplication.cs_6_6.hlsl), [world/shaders/world_pt_water_smoothed_duplication.cs_6_6.hlsl](world/shaders/world_pt_water_smoothed_duplication.cs_6_6.hlsl), [world/shaders/world_pt_water_final.cs_6_6.hlsl](world/shaders/world_pt_water_final.cs_6_6.hlsl), [world/shaders/world_pt_sharc_update.cs_6_6.hlsl](world/shaders/world_pt_sharc_update.cs_6_6.hlsl), [world/shaders/world_pt_sharc_resolve.cs_6_6.hlsl](world/shaders/world_pt_sharc_resolve.cs_6_6.hlsl), [world/shaders/world_pt_material_write.cs_6_6.hlsl](world/shaders/world_pt_material_write.cs_6_6.hlsl).
**Reference:** vendored RTXDI [`README`](world/rtxdi/vendor/rtxdi/README.md), [`license`](licenses/RTXDI-LICENSE.txt); port of NVIDIA FullSample `LightingPasses/PT`.

#### PT AO

PT AO traces short occlusion rays inside the initial path-tracing pass and applies them through the game's own AO tint. Environment AO Strength controls non-character, non-foliage surfaces and Foliage AO Strength controls the mod's foliage, grass and plants, while Character AO Strength scales the combined contact and self AO on characters. Character self-AO adds a short second ray that includes character meshes beyond a small gap, so chin, hair and arm creases darken without proxy self-hit blotches.

**Code:** [AO rays](world/shaders/world_pt_initial.cs_6_6.hlsl), [Sora 2nd lighting](games/sora2nd/lighting/lighting_0xCA3D8596.ps_5_0.hlsl), [settings](world/world_settings.hpp), [shared fields](shared.h).

#### World capture and scene construction

The addon observes game draws and lighting inputs, classifies geometry from shader contracts, and builds CPU/GPU scene data for ray-traced effects. Static, alpha-tested, deforming, and moving geometry have different capture/admission paths, so start with the roadmap before changing this system.

**Code:** [`world/capture`](world/capture/), [`world/contract`](world/contract/), [`world/bvh`](world/bvh/), [`world/rt12`](world/rt12/), [`world/docs/ROADMAP.md`](world/docs/ROADMAP.md).

#### IS-FAST spatio-temporal noise

IS-FAST supplies precomputed blue-noise samples to opted-in effects so spatial samples vary across pixels and frames. Its texture payload is embedded in the addon rather than loaded from a game directory.

**Code:** [`addon.cpp`](addon.cpp); the embedded asset and consumers are listed in the resource map below.

#### NRD RELAX adapter

The world D3D12 path prepares diffuse/specular signals and guide data for NVIDIA NRD's RELAX denoiser. Its input preparation and dispatch are separate from the shared SVGF chain.

**Code:** [`world/rt12/rt12_nrd.hpp`](world/rt12/rt12_nrd.hpp), [`world/rt12/rt12_rtxdi_transfer.hpp`](world/rt12/rt12_rtxdi_transfer.hpp), [`world/shaders`](world/shaders/).
**Context:** [`world/docs/ROADMAP.md`](world/docs/ROADMAP.md) describes its input contract and integration limits.
The NRD 4.17.1 D3D12 adapter (`world/rt12/rt12_nrd.hpp`) and its bridge test with fixture shaders are kept for a future NRD integration; the addon currently denoises with Ray Reconstruction only.

#### SVGF denoising

SVGF reprojects and validates temporal history, estimates variance, then applies an edge-aware a-trous filter. The shared chain serves GTVBAO and VBGI.

**Code:** [`world/shaders/world_svgf.hlsli`](world/shaders/world_svgf.hlsli), [`world/denoise/denoise.hpp`](world/denoise/denoise.hpp).
**Reference:** [`reference/svgf`](reference/svgf/).

#### DLSS Ray Reconstruction

The RR integration observes the game's D3D11 NGX session and initializes a separate D3D12 NGX runtime for the add-on. Its ownership and denoiser-bypass logic is isolated from the effect implementations.

**Code:** [`world/rr`](world/rr/).

### Screen-space lighting and reflections

#### GTVBAO

GT-VBAO computes screen-space ambient occlusion by evaluating visibility bitmasks from depth and surface data, then filters and upscales the result. The local source identifies the upstream project, paper basis, and MIT license.

**Code:** [`gtvbao`](customshaders/gtvbao/), [`gtvbao/GTVBAO.h`](customshaders/gtvbao/GTVBAO.h).
**Reference:** [GameTechDev/GT-VBAO](https://github.com/GameTechDev/GT-VBAO), linked from the source header.

#### GTVBGI / VBGI

GTVBGI also uses visibility bitmasks to estimate indirect illumination. Far-field and multi-bounce paths have their own passes and history resources.

**Code:** [`gtvbao`](customshaders/gtvbao/), especially [`gtvbao_vbgi_main.cs_5_0.hlsl`](customshaders/gtvbao/gtvbao_vbgi_main.cs_5_0.hlsl), [`vbgi_radiance.cs_5_0.hlsl`](customshaders/gtvbao/vbgi_radiance.cs_5_0.hlsl), and [`vbgi_far_temporal.cs_5_0.hlsl`](customshaders/gtvbao/vbgi_far_temporal.cs_5_0.hlsl).
**Source note:** the main-pass comment credits Mirko Salm's Shadertoy GT-VBGI implementation.

#### Falcom SSGI

Falcom SSGI reshapes Kai's existing screen-space indirect-light signal in supported lighting shaders; it does not run GTVBGI's visibility-bitmask gather.

**Code:** [`games/kai/lighting`](games/kai/lighting/), shared settings in [`shared.h`](shared.h).

#### Character SSGI

Character SSGI adjusts how Falcom's sampled SSGI signal contributes to character lighting. It is part of the Kai lighting shader path, not a separate GI pass.

**Code:** [`games/kai/lighting`](games/kai/lighting/), settings and shared data in [`addon.cpp`](addon.cpp) and [`shared.h`](shared.h).

#### Contact shadows

Contact shadows ray-march the screen-space depth buffer from a shaded surface toward the light and write a visibility term. The compute variant adds local shadowing and a separate denoise stage; TAA can stabilize the jittered samples downstream.

**Code:** [`shadows`](customshaders/shadows/), [`addon.cpp`](addon.cpp) for input capture and dispatch.
**Reference:** [`reference/contact_hardening_soft_shadows.txt`](reference/contact_hardening_soft_shadows.txt) and the source comments.

#### Screen-space reflections

The game-specific SSR shaders trace or sample the screen-space scene for reflections and apply game-specific history logic. Their inputs and reprojection details differ by game, so inspect the matching shader before changing its resource bindings.

**Code:** [`sora1st` SSR](games/sora1st/ssr/), [`sora2nd` SSR](games/sora2nd/ssr/), [`Kai` SSR](games/kai/SSR/).
**Reference:** [`reference/SSR`](reference/SSR/) and [`SSR`](customshaders/SSR/).

#### Vanilla SSR improvements

These controls fix or refine the game's existing SSR march, history blend, and disocclusion checks. The feature is implemented inside the Sora 2nd SSR shader replacement and selected by addon hooks.

**Code:** [`games/sora2nd/ssr`](games/sora2nd/ssr/), control and hook selection in [`addon.cpp`](addon.cpp).

#### Dynamic cubemaps

Dynamic cubemaps capture and filter scene radiance into environment textures used for reflections, including specular irradiance and parallax-corrected sampling. This code is a Falcom Engine+ port and modification of Skyrim Community Shaders' Dynamic Cubemaps.

**Code:** [`dyncube`](customshaders/dyncube/).
**Source and terms:** [`licenses/dyncube-license.txt`](licenses/dyncube-license.txt), [`reference/Dynamic Cubemaps`](reference/Dynamic%20Cubemaps/).

### Temporal, post-processing, and game-specific techniques

#### Temporal anti-aliasing (TAA)

The custom TAA passes reproject the previous frame, validate history using depth/color information, and accumulate it with a motion-sensitive feedback path. Both Sora games share the reprojection helper while keeping game-specific shader entry points.

**Code:** [`taa`](customshaders/taa/), [`taa/taa_reproject.hlsli`](customshaders/taa/taa_reproject.hlsli), [`games/sora1st/taa`](games/sora1st/taa/), [`games/sora2nd/taa`](games/sora2nd/taa/).
**Reference:** [`reference/taa`](reference/taa/).

#### Motion blur

Motion blur converts the game's motion vectors to UV velocity, reduces the field to tile and neighborhood maxima, then gathers and composites along motion directions. Its motion-source options and capture constraints are mapped below.

**Code:** [`motionblur`](customshaders/motionblur/), input selection and capture in [`addon.cpp`](addon.cpp).
**Reference:** [`reference/motionblur`](reference/motionblur/).

#### Depth of field (DoF)

DoF generates a circle-of-confusion field and gathers nearby color samples to form the bokeh result. Shared shader logic is used by per-game entry points.

**Code:** [`dof`](customshaders/dof/), [`games/sora1st/dof`](games/sora1st/dof/), [`games/sora2nd/dof`](games/sora2nd/dof/), [`games/kai/DOF`](games/kai/DOF/), [`games/daybreak2/dof`](games/daybreak2/dof/).

#### FXAA

FXAA provides three shader variants for edge detection and quality tradeoffs. The game-specific replacement points are selected by shader hash.

**Code:** [`fxaa`](customshaders/fxaa/), game-specific registration in [`addon.cpp`](addon.cpp).

#### RCAS sharpening

RCAS sharpens the post-TAA image and can adapt its strength using captured motion. The resource map below identifies that input.

**Code:** [`rcas`](customshaders/rcas/), setup and motion capture in [`addon.cpp`](addon.cpp).
**Reference:** [`reference/rcas`](reference/rcas/).

#### Volumetric fog

Volumetric fog is implemented through game-specific shader replacements, with IS-FAST jitter and haze anti-aliasing available in supported variants. The game-specific constant-buffer layouts and shader hooks are kept with each game's shader.

**Code:** [`games/sora1st/volfog`](games/sora1st/volfog/), [`games/sora2nd/volfog`](games/sora2nd/volfog/), [`games/kai/volfog`](games/kai/volfog/), [`games/daybreak2/volfog`](games/daybreak2/volfog/), controls in [`addon.cpp`](addon.cpp).

#### Fog color correction

Fog Color Correction adjusts the fog contribution in supported game lighting shaders. Its controls are passed through the add-on shader-injection data.

**Code:** game lighting and fog shaders in [`kai`](games/kai/), [`sora1st`](games/sora1st/), [`sora2nd`](games/sora2nd/), settings in [`addon.cpp`](addon.cpp) and fields in [`shared.h`](shared.h).

#### Character shadowing

Character Shadowing adds a camera-space contact pass and combines its result in supported character-lighting shaders. The mode and input resources are handled by the add-on and per-game lighting replacements.

**Code:** [`addon.cpp`](addon.cpp), [`shadows`](customshaders/shadows/), [`games/sora1st/lighting`](games/sora1st/lighting/), [`games/sora2nd/lighting`](games/sora2nd/lighting/), [`games/kai/charlighting`](games/kai/charlighting/), [`games/daybreak2/char`](games/daybreak2/char/).

#### Character outline

The character-outline replacement adjusts the Sora 2nd outline pixel shader. Its shader is grouped with other Sora 2nd character passes.

**Code:** [`games/sora2nd/char`](games/sora2nd/char/).

#### Kai hero light

The hero-light feature identifies configured player lights in Kai's light records and patches their contribution. The player-light mapping and patching logic are explicit rather than tied to a shader hash.

**Code:** [`kai_hero_light.hpp`](kai_hero_light.hpp), [`kai_player_light_table.h`](kai_player_light_table.h), settings and light-data handling in [`addon.cpp`](addon.cpp).

#### BRDF improvements

Supported lighting shaders can blend in Hammon diffuse, multiscattering GGX specular, and roughness/highlight limits. These are game-lighting replacements, not a standalone post-process.

**Code:** [`games/sora1st/lighting`](games/sora1st/lighting/), [`games/sora2nd/lighting`](games/sora2nd/lighting/), [`games/kai/lighting`](games/kai/lighting/), [`reference/brdf.hlsli`](reference/brdf.hlsli).

#### Foliage shader replacements

Game-specific foliage shaders adjust alpha-tested, static, and moving foliage materials. This material shading work is separate from world ray-tracing admission and geometry updates.

**Code:** [`games/sora1st/foliage`](games/sora1st/foliage/), [`games/sora2nd/foliage`](games/sora2nd/foliage/), [`games/kai/foliage`](games/kai/foliage/), world path: [`world/bvh`](world/bvh/).

#### Foliage grass AO

A shared helper applies a root-to-tip occlusion gradient to supported grass and clutter shaders. Its base, tip, and curve values are passed through shader-injection data.

**Code:** [`foliage/grass_ao.hlsli`](customshaders/foliage/grass_ao.hlsli), including shaders in [`games/kai/foliage`](games/kai/foliage/) and [`games/sora1st/foliage`](games/sora1st/foliage/).

#### Shadow-map filtering

Game-specific shadow shaders adjust filtering and shadow response while preserving the game's shadow-map passes. Sora shadow shaders and Kai's PCSS shader are kept with their respective game lighting code.

**Code:** [`games/sora1st/shadow`](games/sora1st/shadow/), [`games/sora2nd/shadow`](games/sora2nd/shadow/), [`games/kai/lighting/pcss_0x30190562.ps_5_0.hlsl`](games/kai/lighting/pcss_0x30190562.ps_5_0.hlsl), settings and hooks in [`addon.cpp`](addon.cpp).

#### Game SSAO shader replacements

These replace or modify the games' existing SSAO and combined character-shadow pixel shaders. They are separate from the standalone GTVBAO compute passes.

**Code:** [`games/sora1st/ssao`](games/sora1st/ssao/), [`games/sora2nd/ssao`](games/sora2nd/ssao/), lighting-resource capture in [`addon.cpp`](addon.cpp).

#### Sky shader replacement

The Sora 1st sky pass has a game-specific pixel-shader override. Its shader is isolated from screen-space reflection and dynamic-cubemap code.

**Code:** [`games/sora1st/sky`](games/sora1st/sky/).

## Shared game resources

The addon captures resources from specific game shader bindings and draw events; a register number alone is not enough to identify a resource. Capture and validation logic is in [`addon.cpp`](addon.cpp) and [`world/capture`](world/capture/).

| Resource | Where the addon gets it | Main consumers and tracing pointers |
|---|---|---|
| Scene depth | Captured from a recognized lighting shader's depth SRV: t4 in Sora shaders and t3 in Kai shaders. Binding constants and hash-gated capture are in [`addon.cpp`](addon.cpp); world lifetime/size tracking is in [`world/capture/depth_source.hpp`](world/capture/depth_source.hpp). | World ray tracing, GTVBAO/VBGI, contact shadows, and depth-aware temporal validation. Other passes may bind their own depth input; check that pass's shader and hook. |
| G-buffer normal | Captured from t1 (`mrtTexture0`) on recognized lighting shaders. The game resource is held in add-on state and bound to passes that need surface orientation. | GTVBAO/VBGI, contact shadows, and edge-aware filters. Search `captured_mrt_normal_srv` in [`addon.cpp`](addon.cpp). |
| Vanilla SSAO | Captured from t5 for Sora or t4 for Kai on the recognized lighting shader. This lets replacement/composite paths inspect or preserve the game's AO input. | GTVBAO activation and the game-specific lighting paths. See the register constants and `captured_ssao_srv` in [`addon.cpp`](addon.cpp). |
| Scene constants (`cb_scene`) | Captured from b0 on lighting draws; draw-time values and ranges are tracked rather than assuming one stable buffer offset. | Camera transforms and reconstruction in world effects, shadow/fog passes, and game shader replacements. See [`world/capture/cb_tracking.hpp`](world/capture/cb_tracking.hpp), [`world/capture/cb_value_tracker.hpp`](world/capture/cb_value_tracker.hpp), and `captured_scene_cbv` in [`addon.cpp`](addon.cpp). |
| Motion vectors: G-buffer RTV4 | Captured from the engine's RTV4 during render-target binding. `MBMotionRtv4Ensure` validates it, creates an add-on-owned SRV, and checks that dimensions match depth before world temporal passes use it. | GTVBAO/VBGI, Unified PT temporal reuse, and the default motion-blur input. If it conflicts with TAA t3, world temporal reuse is rejected; see `UpdateRtMotion` in [`addon.cpp`](addon.cpp). |
| Motion vectors: TAA t3 | Captured from t3 only on recognized Sora TAA shader draws; this resource is not assumed ready before lighting. | Custom TAA, motion-adaptive RCAS, and the optional Sora motion-blur input. Kai motion blur uses RTV4. See the t3 capture in [`addon.cpp`](addon.cpp), [`taa_reproject.hlsli`](customshaders/taa/taa_reproject.hlsli), and RCAS/motion-blur dispatch setup there. |
| World geometry and transforms | Draw callbacks and per-command-list SRV/constant-buffer tracking observe game vertex/index buffers, instance data, and draw-time transforms. The pool and scene builders copy/convert admitted data into add-on-owned CPU/GPU structures. | World BVH and DXR paths. Start at [`world/capture/draw_census.hpp`](world/capture/draw_census.hpp), [`world/capture/cb_tracking.hpp`](world/capture/cb_tracking.hpp), [`world/bvh/bvh_pool.hpp`](world/bvh/bvh_pool.hpp), and [`world/rt12`](world/rt12/). |
| Sora 2nd lighting and RTXDI inputs | The lighting callback gathers depth, normal, scene constants, color, MRT material buffers, motion, tiled-light indices, local-light records, and deferred data. It validates and packs these views for the separate D3D12 path. | Unified PT scene/light-table transfer and final shading. Follow input assembly in [`addon.cpp`](addon.cpp), resource transfer in [`world/rt12/rt12_rtxdi_transfer.hpp`](world/rt12/rt12_rtxdi_transfer.hpp), and shader input contracts in [`world/rtxdi`](world/rtxdi/). |
| Local-light records | The Sora 2nd lighting path exposes `dynamicLights_g` and `lightIndices_g`; the RTXDI input bridge snapshots the game data for its D3D12 work. | RTXDI direct-light sampling and the game lamp layouts in [`world/game_lights.hpp`](world/game_lights.hpp). Kai hero-light records are handled separately by [`kai_hero_light.hpp`](kai_hero_light.hpp) and the Kai lighting path. |
| IS-FAST blue noise | The precomputed `fast_noise_ea.dds` data is embedded in [`fast_noise_ea.h`](fast_noise_ea.h), so no external game-folder texture is needed. Add-on helpers create/bind the texture for features that opt in. | Contact shadows, DoF, fog, VBGI/GTVBAO, SSR, shadows, and other sampling passes; search `isfast_noise_srv` and `IS-FAST` in [`addon.cpp`](addon.cpp) for a consumer's exact binding. |
| Effect histories and intermediate outputs | Allocated by the effect that owns them; they are not game-provided textures. Per-feature resource headers define their formats, lifetime, and ping-pong/history slots. | See `*_resources.hpp` beside each world effect, [`world/compute/compute_resources.hpp`](world/compute/compute_resources.hpp) for the shared pass layout, [`world/denoise/denoise.hpp`](world/denoise/denoise.hpp) for shared denoising resources, [`world/shaders/world_svgf_upsample.cs_5_0.hlsl`](world/shaders/world_svgf_upsample.cs_5_0.hlsl) for the shared upsample, and each effect's resource setup in [`addon.cpp`](addon.cpp). |

### Motion vectors versus camera reprojection

Do not assume every temporal effect samples the same motion texture. World temporal effects, including RTXDI paths, use validated RTV4; motion blur can select RTV4 or TAA t3, and RCAS reads TAA t3 for motion-adaptive sharpening. SSR shader variants reproject with previous view/projection constants, so they do not consume either shared motion texture for that step.

## Reference library

General papers, research notes, and reference implementations are grouped under [`reference`](reference/) and [`SSR`](customshaders/SSR/). Each technique entry above points to its specific source or license when one is tracked; game shader evidence is under [`world/reference`](world/reference/) and [`world/research`](world/research/).

## Where to continue

- Read [`AGENTS.md`](AGENTS.md) for the required workflow, scope rules, and the README maintenance rules.
- Read [`world/docs/ROADMAP.md`](world/docs/ROADMAP.md) for current world-ray-tracing state, open work, invariants, and subsystem design notes; [`world/docs/HISTORY.md`](world/docs/HISTORY.md) preserves prior decisions and evidence.
- Read [`settings_ui.h`](settings_ui.h) for settings categories and [`addon.cpp`](addon.cpp) for feature registration, resource capture, and dispatch wiring.

This README is a navigation and data-flow index, not a status log or a replacement for subsystem documentation.
