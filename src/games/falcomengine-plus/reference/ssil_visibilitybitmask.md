This article has been accepted for publication in The Visual Computer. This is the author’s version which has not been fully edited and content may change prior to final publication. Citation information: DOI 10.1007/s00371-022-02703-y 

# **Screen Space Indirect Lighting with Visibility Bitmask** 

Olivier Therrien<sup>1</sup><sup>_†_</sup> Yannick Levesque<sup>2</sup><sup>_‡_</sup> Guillaume Gilet<sup>3</sup><sup>_§_</sup> 

1CDRIN, QC, Canada 

2Cégep de Matane, QC, Canada 

> 3 University of Sherbrooke 



**Figure 1:** _Left: Direct illumination of the scene. Middle: Indirect lighting produced by our method (without texture). Right: Final frame rendered with our method, exhibiting directionally occluded ambient lighting, and a GI bounce that avoids typical thin surface artifacts._ 

## **Abstract** 

_Horizon-based indirect illumination efficiently estimates a diffuse light bounce in screen space by analytically integrating the horizon angle difference between samples along a given direction. Like other horizon-based methods, this technique cannot properly simulate light passing behind thin surfaces. We propose the concept of a visibility bitmask that replaces the two horizon angles by a bit field representing the binary state (occluded / un-occluded) of N sectors uniformly distributed around the hemisphere slice. It allows light to pass behind surfaces of constant thickness while keeping the efficiency of horizon-based methods. It can also do more accurate ambient lighting than bent normal by sampling more than one visibility cone. This technique improves the visual quality of ambient occlusion, indirect diffuse, and ambient light compared to previous screen space methods while minimizing noise and keeping a low performance overhead._ 

**Keywords:** Real-Time Rendering, Indirect Lighting, Ambient Occlusion, Visibility 

## **1. Introduction** 

Indirect diffuse lighting is challenging to compute in real-time. Screen space approximations can be attractive as they reduce the dimensionality of the problem and make the execution cost constant regardless of the scene’s geometric complexity. Modern Screen Space Global Illumination (SSGI) implementations often gather indirect light by doing ray marching on screen pixels similar to 

Screen Space Reflections (SSR) [SKS11]. This approach tends to generate a lot of noise because it implies the numerical integration of irradiance over the entire hemisphere around the surface. Horizon-Based Indirect Illumination (HBIL) [May18], based on Horizon-Based Ambient Occlusion (HBAO) [BSD08, Bav11] and Ground Truth Ambient Occlusion (GTAO) [JWPJ16], improve the efficiency by numerically integrating over a set of directions around the view vector _v_ (Figure 2) while doing analytic integration of the horizon angle difference between samples. 

- therrien.olivier@cdrin.com 

> _‡_ levesqueyannick@cgmatane.qc.ca 

> _§_ guillaume.gilet@usherbrooke.ca 

Fundamentaly, the core principle of these methods lies in the estimation of the scene local geometry around each shading sample 

_O. Therrien, Y. Levesque, G. Gilet / Screen Space Indirect Lighting with Visibility Bitmask_ 

2 



<!-- Start of picture text -->
ee<br><!-- End of picture text -->

**Figure 2:** _3D view of the scene, centered on the shaded pixel. GTAO/HBIL generates a set of hemisphere slices (in green) in various directions around the view vector v. Occluders (in black) intersect some of the slices, producing occlusion cones (in red)._ 

by relying on readily-available screen-space information, such as the discrete depth buffer. However, such information is by essence discrete and incomplete, and must be reconstructed. All those techniques evaluate Ambient Occlusion, the modulation of indirect irradiance due to local geometry, in screen space from a single layer depth buffer, and assume infinite surface thickness by treating it as a height-field (see Figure 3). While this is a valid assumption in some cases, not knowing what the real geometry looks like, it causes halos and over-darkening around thin surfaces (see figure 4). Falloff heuristics are used to mitigate those artifacts but fail when using a large sampling radius. 



<!-- Start of picture text -->
SV nda<br><!-- End of picture text -->

**Figure 3:** _Side view of one slice centered on the view vector v. Left: Ground truth scene with multiple occluders (in black) producing multiple visibility cones (in green). Right: GTAO takes a fixed number of samples (red dots) in the depth buffer on both sides of the hemisphere to find highest elevation angles_ θ1 _and_ θ2 _._ 

Our proposed method rejects the assumption that the depth buffer is strictly a height-field and models the behavior of light passing behind surfaces. Additionally, to preserve performance, we want to avoid explicitly tracing new rays from scratch to adequately sample multiple elevation angles. To do so, we introduce the concept of visibility bitmask, which is essentially a discretization of the hemisphere slice in _Nb_ sectors, that allows us to approximate the tracing of _Nb_ rays at the same performance cost as one horizon search. 



**Figure 4:** _Left: GTAO produces halos around the poles. Right: Our method is able let light pass behind the poles without introducing halos artifacts._ 

A common limitation of single layer depth buffers is that occluded surfaces are not represented, which can cause missing occlusion or lighting. Approaches like Deep G-Buffer [MMNL16] or Multi-layer SSRT [HBSS17] alleviate this issue by storing multiple layers or sample per pixel, to provide more information on background surfaces. As rendering multiple layers is very expensive, we chose to limit ourselves to only one layer. This means that some artifacts caused by inaccurate background surface estimation will remain, but using a constant thickness _t_ at each depth sample with a visibility bitmask greatly improves quality around thin surfaces compared to horizon-based techniques. In this study, we show that visibility bitmasks can tremendously reduce noise in the image compared to SSR-like tracing, while handling the light passing behind surfaces much more accurately than horizon-based techniques. We also show that using a visibility bitmask to sample ambient light gives a more precise ambient estimation than traditional methods of sampling along the surface normal or even bent normal. 

The main contributions of this paper revolve around the introduction of a visibility bitmask in traditional SSGI methods, retaining the efficiency and noise reduction qualities of horizon sampling, while handling light passing behind surfaces of constant thickness. We demonstrate the capabilities of our method through several SSGI applications, such as ambient occlusion, directional ambient occlusion and indirect diffuse lighting. 

## **2. Previous Work** 

Estimating ambient occlusion and indirect diffuse lighting from screen space information is a well-known idea. It mainly stems from the seminal Screen Space Ambient Occlusion method [Mit07], approximating ambient occlusion by sampling random points in the depth buffer in a circle around each pixel and has been thoroughly extended over the years [Mit07, MOBH11, MML12]. The keypoint of these methods is to estimate the local geometry around a sample using the incomplete information contained in the various buffers (geometrical normals, depth...) while maintaining high rendering performance. 

Reconstruction of local geometry can be improved by gathering more information from the scene during the rendering passes. Reflective Shadow Maps (RSM) [DS05] approximate indirect diffuse lighting coming from a point light source using essentially 

_O. Therrien, Y. Levesque, G. Gilet / Screen Space Indirect Lighting with Visibility Bitmask_ 

3 

a G-Buffer generated from the light’s view point. This technique is costly so in practice it’s usage is limited to less than a handful of light simultaneously, and doesn’t take into account indirect light occlusion. Deep Screen Space [NRS14] adaptively tessellate scene geometry into an unstructured surfel cloud used for rendering different effects like AO, GI and more. It bypasses major screen-space limitations like hidden surfaces and under-sampling of oblique geometry, but is expensive to compute and cannot handle indirect light occlusion between surfels. 

More recently, Stochastic-Depth Ambient Occlusion (SDAO) [VSE21] introduced the notion of stochastic depth map, capturing multiple scene layers per pixel at random. This technique is effective at detecting hidden surfaces, but, since it’s used in conjunction with HBAO [BSD08], it doesn’t prevent over-darkening around thin objects. However, while these methods improve reconstruction of local geometry, they are more computationally expensive than single-layer approaches, both during sample capture (by forgoing early-z optimization) and reconstruction (by having to evaluate multiple layers). 

Improving the quality of the reconstruction from a single layer is a difficult problem that has been widely studied. Alchemy ambient obscurance [MOBH11] is based on a similar approach than SSAO and improves robustness and artistic control, with the follow up Scalable Ambient Obscurance (SAO) [MML12] that also improves performance. Horizon-Based techniques [BSD08, Bav11] generates high quality results with low amount of noise by sampling elevation along a set of directions but causes over-darkening around thin surface. Several methods focus on improving performance, such as Line Sweep Ambient Obscurance (LSAO) [Tim13], which pre-caches sample information along azimuthal lines in GPU shared memory and reuses the same samples to shade multiple pixels. More recently, Ground Truth Ambient Occlusion [JWPJ16] improved the accuracy of HBAO by making it match a path-traced reference, and support a multi-bounce occlusion approximation. 

These techniques has been derived to propose more advanced indirect illumination features, taking advantage of the local geometry reconstruction. Silvennoinen _et al._ [ST15] added support for indirect lighting in real-time via an SSGI implementation using LSAO as a basis. This method is approximative because only one color sample is taken per horizon highpoint, and it doesn’t take into account partial occlusion that could have occurred along the way. Other SSGI variants like Screen Space Ray Tracing Global Illumination (SSRTGI) [SVF17] use an SSR-like technique to sample GI at the ray hit location. However, this approach introduces a lot of noise which is difficult to remove without over-blurring. Another technique known as HBIL (which is based on HBAO and GTAO) showed how to compute GI accurately from multiple samples by weighting the sample contribution by the angle difference relative to the previous sample. While this method gives accurate results within the visibility cone, it’s based on the assumption that the depth buffer is a height field, and it cannot take into account light bounces that would pass behind surfaces. 

Finally, Bitmask Soft Shadows (BMSS) [SS07] determine the visibility of an area light source with a bit field where each bit tracks the visibility of a sample point on the light source. Our method solves a slightly different problem but nonetheless shares many similarities regarding surface shape estimation from a depth map and addresses overlapping sample visibility in the same way using a bitmask. 

## **3. Proposed Algorithm** 

In this section, we present how our method propose to improve the quality of reconstruction of local geometry, especially in the case of thin surfaces, by treating the depth buffer as a set of unconnected samples each associated with an arbitrary thickness. 

## **3.1. Ambient Occlusion** 

Ambient Occlusion (AO) [ZIK98] is a non-physically based lighting approximation of global illumination that assumes that the scene is lit by uniform ambient lighting and that all objects are occluders. It’s a very common effect in real-time applications because it can be computed efficiently in screen space and adds a lot of perceived realism to the scenes. It can be expressed as a an estimation of the visibility function _V_ in the hemisphere around each sample : 



By using a parameterization of the hemisphere, it can be decomposed into : 



and 



In practice, the integral of equation 2 is computed using Monte Carlo integration over a few slices. Method such as GTAO and HBAO propose an analytic solution of equation 3 by estimating, through depth sampling, two horizons θ1 and θ2. 

In our proposed algorithm, the two horizon angles θ1 and θ2 of GTAO are replaced by a bit field of size _Nb_ , representing the binary state (occluded/un-occluded) of _Nb_ visibility sectors uniformly distributed around the hemisphere slice. Samples are still taken on each side of the view vector _v_ , but the bit field is centered on projected normal _n_ : 



Each sample taken along the hemisphere slice will determine a potential occluder and impact the visibility function _V_ of the given sector. To determine the occluded state of a sector, we consider each sample as a local thin geometry having a thickness _t_ , acting as an occluding geometry between angles θ _f_ and θ _b_ . The activation of a sector depends on the _hit criterion_ which ensures sufficient overlap of these angles with the sector to get registered. θ _f_ is equivalent 

_O. Therrien, Y. Levesque, G. Gilet / Screen Space Indirect Lighting with Visibility Bitmask_ 

4 

to θ used in GTAO (directly at the sample), and θ _b_ depends on sample thickness _t_ , (figure 3). For the proposed algorithm, we used the round criterion which requires the sector to be half covered by the sample. The pseudo-code at line 15 to 17 in Algorithm 1 shows how (θ _f ,_ θ _b_ ) are inferred from sample position and thickness. All occluded sectors are set at once, making the algorithm perform in O(1) for any sector count. Note that we must convert the angles from cosine space to angular space for the samples to be properly distributed around the hemisphere. 

This enables fast directional occlusion and partial integration, at the cost of precision. In the depth buffer, we take a fixed number of azimuthal directions around each pixel and sample along these directions to find (θ _f_ ,θ _b_ ) pairs that can be integrated into the hemisphere slice (see Figure 5). Like GTAO we distribute the occlusion integral spatially and temporally to increase the number of effective samples. 



**Figure 5:** _The hemisphere is divided into Nb uniform sectors that can be either occluded (in red) or un-occluded (in green)._ θ _b is derived from_ θ _f and thickness t. Sectors that are at least half covered by the_ (θ _f ,_ θ _b_ ) _pair get occluded by the sample. We assume v and n are aligned in this diagram for simplicity._ 

The choice of _t_ has a big impact on visual fidelity. GTAO without falloff is equivalent to using an infinite value for _t_ . Ideally we would want to use the per-pixel surface thickness of objects as value for _t_ , unfortunately this is more expensive to compute in realtime and impossible to know precisely using a single layer depth buffer. We propose using a small constant value since artifacts of over-occlusion around thin objects are much more noticeable than light leaks behind thick objects. This is because bigger objects usually occlude completely what is behind them so a light leak can still look plausible, whereas over-occlusion around thin objects is 



**Figure 6:** _Left: GTAO using horizon angles without falloff term exhibits no light leaks. Middle: our method using visibility bitmasks with fixed thickness causes light leaks at depth discontinuities. Right: GTAO using horizon angles with falloff term also causes some light leaks at depth discontinuities._ 

directly visible (see Figures 6 and 7). Using a fixed world-space thickness can cause an over-attenuation of occlusion for objects far away from the camera, so we give the option to increase _t_ linearly over the distance to counter this effect. This causes a slight change in occlusion when the camera moves, but is barely noticeable and fixes the problem effectively. Finding an efficient heuristic to estimate an accurate thickness for each sample would further improve the accuracy of the method but remains a difficult problem we leave for future work. 

**Algorithm 1** Generate AO and GI using visibility bitmasks 

- 1: _t ←_ constant thickness 2: _Nb ←_ bitmask size 3: _p ←_ view space fragment position 4: _np ←_ view space fragment normal 5: _r ←_ projected radius onto image plane 6: Determine stepsize as _r/_ ( _Ns_ + 1) 7: Determine directions with random offset 8: _AO, GI ←_ 0 9: **for** direction _di_ where _i_ = 0 to _Nd_ **do** 

- 10: _tp ←_ slice plane tangent vector in direction _di_ 11: _t_ θ _←_ angle of _tp_ with XY-plane 12: Bitmask _bi ←_ 0 13: **for** step _s j_ where _j_ = 0 to _Ns_ **do** 14: Front sample _s f ←_ view-space position at step j 15: Back sample _sb ← s f − ∥pp∥_<sup>_t_</sup> 16: θ _f_ , θ _b ←_ angles of _s f_ and _sb_ on XY-plane 17: θ _min_ , θ _max ←_ min(θ _f ,_ θ _b_ ) _,_ max(θ _f ,_ θ _b_ ) 18: _a_ , _b ←⌊_<sup>θ</sup><sup>_min_</sup> π<sup>+</sup><sup><u>π</u></sup> <u>2</u> _Nb⌋_ , _⌈_<sup>θ</sup><sup>_max−_</sup> π<sup>θ</sup><sup>_min_+</sup><sup><u>π</u></sup> <u>2</u> _Nb⌉_ 19: _b j ←_ 2<sup>_b_</sup> _−_ 1 _≪ a_ 20: _c j ←_ direct lighting at step _j_ (from GBuffer) 21: _n j ←_ normal at step _j_ (from GBuffer) 22: _l j ← ∥ss_ _<u>ff −−pp∥</u>_ 23: _GI ← GI_ +<sup>COUNTBITS</sup> _Nb_<sup><u>(</u></sup><sup>_b_</sup><sup>_<u>j</u>_&</sup><sup>_∼bi_</sup><sup><u>)</u></sup> _c j_ ( _np · l j_ )( _n j ·−l j_ ) 24: _bi ← bi | b j_ 25: **end for** 26: _AO ← AO_ + 1 _−_ COUNTBITS( _bi_ ) _/Nb_ 

27: **end for** 

28: **return** _AO/Nd_ , _GI/Nd_ 

The following sections explain the usage of the above-described core algorithm in implementing ambient occlusion, directionally occluded ambient lighting, and indirect diffuse bounce. 

## **3.2. Directionally Occluded Ambient Lighting** 

Ambient lighting in real-time applications is usually sampled using the surface normal, but tends to give poor results because it doesn’t take into account the directional occlusion of lighting. Screen space bent normal [KRES11] addresses this problem by sampling towards the largest non-occluded direction, but is limited to a single ambient direction per pixel and does not handle thickness properly. Visibility bitmasks can improve this by weighting the ambient lighting in a given direction by the directional occlusion while allowing light to pass behind surfaces. To this end, we divide the 

5 

_O. Therrien, Y. Levesque, G. Gilet / Screen Space Indirect Lighting with Visibility Bitmask_ 



<!-- Start of picture text -->
Reference GTAO (no falloff) | GTAO (falloff) Ours (t = 1) Ours (t = 0.5) Ours (t = 0.25) Ours (t = 0.1)<br>Vie<br>rr ae a \ ae \4 ae<br><!-- End of picture text -->

**Figure 7:** _From left to right: Ray tracing reference, GTAO without falloff, GTAO with falloff, visibiliy bitmask using a thickness of 1, 0.5, 0.25, 0.1. All methods use a radius of 2._ 

hemisphere into as many subregions as the number of ambient samples, generating a sampling direction vector at the center of each subregion. The ambient source is then sampled with this vector and multiplied the lighting intensity by the amount of un-occluded sectors over the total sector count (see Figure 8). It made the ambient light color vary smoothly according to changes in occlusion directionality. 



<!-- Start of picture text -->
i) feo<br><!-- End of picture text -->

**Figure 8:** _In this example, 4 ambient samples per slice per pixel are taken so the hemisphere is divided into 4 subregions. Vectors a_ 1 _, a_ 2 _, a_ 3 _, a_ 4 _are generated at the center of their respective subregion._ 

## **3.3. Indirect Diffuse** 

Indirect diffuse lighting is the bouncing of light on nearby surfaces. It’s traditionally expensive to compute accurately, even in screen space. HBIL can do it efficiently but fails to account for light passing behind surfaces. Figure 9 shows how we computed this effect with better thickness handling using visibility bitmasks. Samples were taken along the slice direction and detected (in O(1)) how many un-occluded sectors are covered to estimate lighting contribution. These sectors were then set to an occluded state (also in O(1)) to handle partial or total occlusion of light coming from subsequent samples. The more the visibility sectors, the more precise the estimation of lighting and occlusion. It was observed that 32 sectors gave good quality and makes the bit field fit nicely within a single unsigned integer. 



<!-- Start of picture text -->
QA AP<br><!-- End of picture text -->

**Figure 9:** _Left: The yellow sample intersects one un-occluded sector and can contribute lighting equivalent to one over the total number of visibility sectors. The sector is set to an occluded state for subsequent samples. Right: Sampling continues and a new object on the right is found, but it intersects an already occluded sector, so it cannot contribute lighting. The yellow sample on the left crosses an un-occluded sector and can contribute._ 

The pseudo-code at line 23 in Algorithm 1 shows how the lighting contribution of a sample is implemented, using the number of occluded zones by the current sample. If one or more sectors are covered, the sample contributed lighting and the light buffer is sampled at the sample location. Then _n · l_ and _nl · l_ are computed and used for weighting light intensity. The light is furthermore weighted by the occluded sector count over the total sector count. 

## **4. Results and evaluation** 

## **4.1. Ambient Occlusion** 

In this subsection, the core algorithm will be demonstrated conjointly with AO, as it’s much easier to discern the properties of visibility bitmasks in this mode than using indirect diffuse. The renders in Figure 10 are produced by our extension of Unity’s GTAO implementation, where the two horizon angles θ1 and θ2 have been replaced by a single visibility bitmask. 

6 

_O. Therrien, Y. Levesque, G. Gilet / Screen Space Indirect Lighting with Visibility Bitmask_ 



<!-- Start of picture text -->
aww gy SMO pares MeO ae RayTased Reference,<br>Bi eoypSage 7<br>im) \<br>ea le wee | /<br>iyeae ne Ea ae<br>m)<br>Radu 1.0 FO dat a ay, *<br>16 samples 4 | ie |<br>“14 1) | Lo ~ L = a<br>a= IP Wats<br>om A "I<br>16 samples | 4 " I<br>Radius 3.0a-| -_eT eeel<br><!-- End of picture text -->

**Figure 10:** _Comparison of GTAO, our method based on visibility bitmasks, and a ray-traced AO reference at different radius and sample count values._ 

It highlights how easy it is to implement our method on top of an existing horizon-based technique, and how this single modification can dramatically enhance visual quality compared to a ray-traced reference. The Lumberyard Bistro scene has been chosen because it contains a lot of thin and shallow surfaces that are typically a problem with horizon-based techniques, but are improved by visibility bitmasks. All benchmarks use one hemisphere slices per pixel jittered over multiple frames. 

The radius parameter (Figure 10) is the radius of the hemisphere aligned to the screen in world units. Wider hemispheres will cover wider regions of the screen, casting farther-reaching occlusion. A wide radius is problematic for GTAO because the single cone approximation tends to cast too much occlusion in regions enclosed by thin objects. Even around not-so-thin objects, a wide radius tends to produce a blurry occlusion blob that does not capture fine detail. In contrast, our method (with the exact same samples) is able to let light pass behind surfaces, avoiding over-occlusion and capturing a lot of small details. 

Figure 11 demonstrates an even more difficult case for GTAO where most of the objects are behind a fence. The light gets 



<!-- Start of picture text -->
Ne<br><!-- End of picture text -->

**Figure 11:** _Left: GTAO exhibit too much occlusion behind the bars._ 

_Right: Our method is able let light pass behind the bars, minimizing the over-occlusion artifact._ 

trapped behind the bars instead of passing by, causing a lot of overocclusion. Our method is able to handle this situation much better. When using a wide radius, GTAO has a tendency to produce halos around objects (Figure 4). Our method doesn’t have this problem, and capture more geometric and normal details. 

The number of samples (Figure 10) indicates the number of 

_O. Therrien, Y. Levesque, G. Gilet / Screen Space Indirect Lighting with Visibility Bitmask_ 

7 

fetches taken in the depth buffer along one horizon side. Therefore, for one slice, the actual number of samples taken is twice that number. A wider radius causes the samples to be sparser on-screen, so it’s typical to increase sample count for a wider radius to maintain the same sampling density. A low sampling density increases the likelihood of missing potential occluders, especially if they are thin on-screen. Increasing the number of samples has a big impact on performance, so it’s a tradeoff. 

Performance depends primarily on the number of samples taken and the radius of the effect. A wider radius increases the probability of occurrence of a cache miss (sample not being present in the cache) and consequently can lower performance. The execution time of the technique tends to scale linearly with the number of samples. Table 1 compares the performance of the horizon-based GTAO implementation versus our method using visibility bitmasks. Both techniques are composed of a sampling pass and a denoising pass. Only the results of the sampling pass are included in the table since it’s the only one impacted by our method. The denoising pass has a constant cost of 0.3 ms in 1080p. It can be observed that our method has a modest impact on performance around 0.01-0.02 milliseconds, with a fixed ALU overhead of about 15 GPU instructions per sample. Increasing the radius masks this overhead as the execution becomes bandwidth-limited. 

|Radius|Sample Count|GTAO|Our Method|
|---|---|---|---|
|0.8|8|0.49 ms|0.51 ms|
|1|12|0.75 ms|0.77 ms|
|1|16|0.95 ms|0.97 ms|
|2|16|1.12 ms|1.13 ms|
|3|16|1.12 ms|1.13 ms|



**Table 1:** _Render time of the sampling pass for GTAO and our method with various radius and sample parameters, at 1920x1080, with 32 visibility sectors per hemisphere slice. Benchmarks are done on an Nvidia RTX 2080 GPU._ 

Another parameter that impacts image quality is the number of visibility sectors. When the sector count is too low, banding artifacts can appear, particularly around thin objects. The proposed implementation used 32 visibility sectors because it just crossed the threshold where the artifacts became almost invisible. Additionally, it nicely fits into a single unsigned integer which gives good performance on the GPU. By contrast, a 128 bits version require four unsigned integers and the use of vector instructions, which limits the amount of instruction packing that the compiler could do, negatively impacting the performance. A performance overhead of around 5-10% was observed with 128 visibility sectors compared to 32. 

## **4.2. Directionally Occluded Ambient Lighting** 

In this subsection, we compare different ambient sampling strategies and show how they can dramatically influence the resulting lighting and occlusion. Figure 12 shows the average normal on the left, and the resulting ambient lighting on the right for each strategy. In most 3D applications, ambient lighting is sampled in the direction of the surface normal. This approach does not take into ac- 

count the fact that some light could be occluded in some direction and tend to make ambient lighting change sharply with the scene geometry. A better approach is to use a screen space bent normal per pixel that is modulated according to nearby occlusion. It points towards the direction of incoming light and gives a smoother result. However, it cannot handle multiple light directions and will misrepresent the ambient lighting of surfaces enclosed by thin objects. 



**Figure 12:** _Comparison of ambient lighting using the surface normal, bent normals, and visibility bitmasks. The average sampling normal direction is shown on the left to make the difference more visible. Top row: G-Buffer normals. Middle row: Bent normals. Bottom row: visibility bitmask._ 

Our approach used visibility bitmasks to take multiple samples along each hemisphere slice in the directions that were not occluded. Doing so allow ambient light to pass behind surfaces, giving smooth lighting from multiple directions. It’s also worth noting that each ambient sample is weighted according to the occlusion in that specific direction. If the ambient color had been simply averaged out and then multiplied by ambient occlusion, a lot of the color variation in the lighting would have been lost. 

## **4.3. Indirect Diffuse** 

In this subsection, we look at the visual quality and performance of the indirect diffuse portion of the algorithm. In addition to sampling the depth buffer, we also sample the HDR light buffer and the screen space normal buffer for every sample taken. The light buffer contains only the direct lighting (with shadows), as the ambient light is computed by our method. Figure 13 compares our result with a path tracing reference for single and multi-bounce indirect diffuse lighting. The direct lighting coming from the sun on the left wall bounces on the brick wall, illuminating it and casting indirect shadows. With multiple bounces, the light is even able to bounce back on the left wall, illuminating a shadowed region of 

8 

_O. Therrien, Y. Levesque, G. Gilet / Screen Space Indirect Lighting with Visibility Bitmask_ 



<!-- Start of picture text -->
Visibility Sector GI Path Tracing Reference<br>Single-<br>Bounce 4 \,<br>ie<br>Mult Vl<br>Bounce “4 7 Wi<br><!-- End of picture text -->

**Figure 13:** _Comparison of indirect diffuse algorithm and path tracing reference with single and multi-bounce indirect diffuse._ 

the wall. Multiple bounces are implemented by injecting the indirect illumination into the light buffer to be used as input for the next frame. Light intensity needs to be properly balanced when using multi-bounce, or it can cause a feedback loop resulting in lighting accumulation over time. The result cannot match perfectly the path-traced reference since the algorithm operates only on the screen pixels as opposed to the entire scene geometry. One major drawback of our technique, along with screen-space methods, is that if direct light leaves the screen, the indirect lighting disappears. 



<!-- Start of picture text -->
Non-Jittered \, Jittered h<br>]<br><!-- End of picture text -->

**Figure 14:** _When samples are evenly spaced along the sampling direction, a banding pattern can appear. Jittering the samples along the sampling direction helps mask the banding artifact._ 

Low sample density can also cause a loss of detail around small objects. To improve this, we distribute the samples exponentially around the shaded pixel, as nearby surfaces usually have more influence on the result than farther ones. 

Figure 16 compares the amount of noise incurred when sampling the scene with SSR-like tracing and our method based on visibility bitmasks. Both techniques are taking the same maximum number of samples but the visibility bitmask approach has a lot less noise. Rays in SSR are doing at most one accumulation operation (when a hit is found). In contrast, the visibility bitmask approach is accumulating each sample that is visible from the current pixel. 

This algorithm is bandwidth-intensive because we need to sample the HDR light buffer and the screen space normal buffer for every sample taken. Those sample locations are not correlated and impair caching. Table 2 compares the performances of the indirect diffuse part of the algorithm with different sampling parameters. The corresponding renders are shown in Figure 15. Full resolution means that the shader is executed for every pixel of the final render resolution, whereas half-resolution executes it on a screen that is half the size (a quarter the number of pixels). The image is then upscaled with a classic bilateral upsampler to avoid aliasing. Rendering in half resolution is much more efficient (around 4x), but can introduce more flickering in the image and a blurrier result. 

## **5. Conclusion and future work** 

The number of samples taken has a big impact on quality. Having a too low sampling density can introduce banding artifacts. To mitigate the issue, samples are jittered along the sampling direction, as shown in Figure 14. 

Previous screen space GI methods that rely on HBAO cannot handle thickness because of the assumption that the depth buffer is strictly a height field. Other techniques based on SSR tracing squander a lot of rays that end up passing behind and over surfaces, exiting the screen without hitting anything, thereby introduc- 

9 

_O. Therrien, Y. Levesque, G. Gilet / Screen Space Indirect Lighting with Visibility Bitmask_ 



**Figure 15:** _Resulting renders using different sample, radius, stepping, and resolution parameters for the indirect diffuse lighting. The parameters of each figure are indicated in Table 2._ 



**Figure 16:** _Left: 2 rays per pixel SSR tracing, with 16 steps per ray. There is a lot of noise, as a lot of rays exit the screen or pass behind surfaces without hitting anything. Right: Horizon search on both sides of the pixel using a visibility bitmask with 16 steps per horizon is a lot less noisy._ 

|Configuration|Sampling|Denoising|Total|
|---|---|---|---|
|a) 8 samples, radius 1,<br>const. steps, full res.|0.9 ms|0.33 ms|1.23 ms|
|b) 8 samples, radius 4,<br>const. steps, full res.|1.7 ms|0.33 ms|2.03 ms|
|c) 16 samples, radius 4,<br>const. steps, full res.|2.3 ms|0.33 ms|2.63 ms|
|d) 16 samples, radius 4,<br>exp. steps, full res.|2.6 ms|0.33 ms|2.93 ms|
|e) 32 samples, radius 4,<br>exp. steps, full res.|4.0 ms|0.33 ms|4.33 ms|
|f) 16 samples, radius 4,<br>exp. steps, half res.|0.97 ms|0.1 ms|1.07 ms|



**Table 2:** _Render time at 1920x1080, with a constant thickness value of 0.2._ 

ing a lot of noise. In contrast, our method combined the sampling efficiency of horizon-based methods, while retaining the capability of handling thickness properly along the way. The proposed algorithm is easy to understand and implement on modern GPUs and can be integrated into any horizon-based technique with only a mild performance overhead. Moreover, this method can also be used to improve ambient light sampling by taking into account the directionality of occlusion when integrating ambient light. 

Even though the ray-tracing part of the algorithm handles thickness properly, since we operate on a single layer of the depth buffer, the actual object thickness is unknown. We are forced to rely on a 

constant thickness value that optionally increases linearly over the distance. A thickness heuristic that would give a plausible thickness per pixel would help improve the occlusion leaks around some very thin objects or light leaks behind very thick ones and would be interesting to explore in future work. At the performance level, SSGI can be an expensive algorithm because it is very demanding on GPU bandwidth and utilizes the cache poorly. Using a caching method similar to LSAO could potentially improve this. Finally, 

10 

_O. Therrien, Y. Levesque, G. Gilet / Screen Space Indirect Lighting with Visibility Bitmask_ 

the common ambient light sources are either static (constant color, light probes) or expensive to update at runtime. We might investigate in the future ways to approximate low-frequency ambient irradiance dynamically. 

## **6. Acknowledgments and data statements** 

Special thanks to Deepti Joshi (CDRIN) and Peter Shirley (NVIDIA) for their guidance in the redaction of this paper. We also want to thank our other colleagues namely Antoine Fortin (CDRIN), Olivier Leclerc (CDRIN), Steven Pigeon (UQAR), and Vahe Vardanyan (CDRIN) who have actively supported the current body of work. Most of the models are from the Amazon Lumberyard Bistro scene. This research is financed in part by the province of Quebec (Canada) via the grant "Programme d’aide à la recherche et au transfert (PART)". Data sharing not applicable to this article as no datasets were generated or analysed during the current study. 

- [SVF17] SHERGIN D., VIDIGER D., FOFANOVA A.: Superposition benchmark: innovative ssrtgi lighting in real time. In _ACM SIGGRAPH 2017 Real Time Live!_ 2017, pp. 25–25. 

- [Tim13] TIMONEN V.: Line-sweep ambient obscurance. In _Computer graphics forum_ (2013), vol. 32, Wiley Online Library, pp. 97–105. 

- [VSE21] VERMEER J., SCANDOLO L., EISEMANN E.: Stochastic-depth ambient occlusion. _Proceedings of the ACM on Computer Graphics and Interactive Techniques 4_ , 1 (2021), 1–15. 

- [ZIK98] ZHUKOV S., IONES A., KRONIN G.: An ambient light illumination model. In _Eurographics Workshop on Rendering Techniques_ (1998), Springer, pp. 45–55. 

## **References** 

[Bav11] BAVOIL L.: Horizon-based ambient occlusion using compute shaders. _Nvidia DirectX 11_ (2011). 

[BSD08] BAVOIL L., SAINZ M., DIMITROV R.: Image-space horizonbased ambient occlusion. In _ACM SIGGRAPH 2008 talks_ . 2008, pp. 1–1. 

- [DS05] DACHSBACHER C., STAMMINGER M.: Reflective shadow maps. In _Proceedings of the 2005 symposium on Interactive 3D graphics and games_ (2005), pp. 203–231. 

- [HBSS17] HOFMANN N., BOGENDÖRFER P., STAMMINGER M., SELGRAD K.: Hierarchical multi-layer screen-space ray tracing. In _Proceedings of High Performance Graphics_ . 2017, pp. 1–10. 

[JWPJ16] JIMÉNEZ J., WU X., PESCE A., JARABO A.: Practical real-time strategies for accurate indirect occlusion. _SIGGRAPH 2016 Courses: Physically Based Shading in Theory and Practice_ (2016). 

- [KRES11] KLEHM O., RITSCHEL T., EISEMANN E., SEIDEL H.-P.: Bent normals and cones in screen-space. In _VMV_ (2011), Citeseer, pp. 177–182. 

[May18] MAYAUX B.: Horizon-based indirect lighting. 2018. 

[Mit07] MITTRING M.: Finding next gen: Cryengine 2. In _ACM SIGGRAPH 2007 courses_ . 2007, pp. 97–121. 

[MML12] MCGUIRE M., MARA M., LUEBKE D. P.: Scalable ambient obscurance. In _High Performance Graphics_ (2012), Citeseer, pp. 97– 103. 

- [MMNL16] MARA M., MCGUIRE M., NOWROUZEZAHRAI D., LUEBKE D. P.: Deep g-buffers for stable global illumination approximation. In _High Performance Graphics_ (2016), pp. 87–98. 

- [MOBH11] MCGUIRE M., OSMAN B., BUKOWSKI M., HENNESSY P.: The alchemy screen-space ambient obscurance algorithm. In _Proceedings of the ACM SIGGRAPH Symposium on High Performance Graphics_ (2011), pp. 25–32. 

- [NRS14] NALBACH O., RITSCHEL T., SEIDEL H.-P.: Deep screen space. In _Proceedings of the 18th meeting of the ACM SIGGRAPH Symposium on Interactive 3D Graphics and Games_ (2014), pp. 79–86. 

[SKS11] SOUSA T., KASYAN N., SCHULZ N.: Secrets of cryengine 3 graphics technology. In _ACM SIGGRAPH_ (2011), vol. 1. 

- [SS07] SCHWARZ M., STAMMINGER M.: Bitmask soft shadows. In _Computer Graphics Forum_ (2007), vol. 26, Wiley Online Library, pp. 515–524. 

- [ST15] SILVENNOINEN A., TIMONEN V.: Multi-scale global illumination in quantum break. In _ACM SIGGRAPH_ (2015). 

