# **ReSTIR PT Enhanced: Algorithmic Advances for Faster and More Robust ReSTIR Path Tracing** 

DAQI LIN, NVIDIA, USA MARKUS KETTUNEN, NVIDIA, Finland CHRIS WYMAN, NVIDIA, USA 



**_Original ReSTIR PT (37.1 ms)_** _FLIP: 0.321_ 



<!-- Start of picture text -->
Orig. Ref. Ours<br><!-- End of picture text -->





















**_ReSTIR PT Enhanced (12.6 ms)_** _FLIP: 0.263_ 

**Fig. 1.** _We enhance the performance and robustness of ReSTIR PT [Lin et al. 2022] with a set of novel algorithms. Our techniques provide a significant speedup, reduce correlation artifacts (first inset row), reduce color noise (third inset row), and improve the quality of glossy reflections (e.g., on the ground plane) and refractions (second inset row), as seen in this Spaceship scene._ 

Algorithms leveraging ReSTIR-style spatiotemporal reuse have recently proliferated, hugely increasing effective sample count for light transport in real-time ray and path tracers. Many papers have explored novel theoretical improvements, but algorithmic improvements and engineering insights toward optimal implementation have largely been neglected. We demonstrate enhancements to ReSTIR PT that make it 2–3× faster, decrease both visual and numerical error, and improve its robustness, making it closer to production-ready. We halve the spatial reuse cost by reciprocal neighbor selection, robustify shift mappings with new footprint-based reconnection criteria, and reduce spatiotemporal correlation with duplication maps. We further improve both performance and quality by extensive optimization, unifying direct and global illumination into the same reservoirs, and utilizing existing techniques for color noise and disocclusion noise reduction. 

### CCS Concepts: • **Computing methodologies** → **Ray tracing** . 

Additional Key Words and Phrases: Optimization, Sampling, Light Transport, Real Time 

### **ACM Reference Format:** 

Daqi Lin, Markus Kettunen, and Chris Wyman. 2026. ReSTIR PT Enhanced: Algorithmic Advances for Faster and More Robust ReSTIR Path Tracing. _Proc. ACM Comput. Graph. Interact. Tech._ 9, 1, Article 13 (May 2026), 19 pages. https://doi.org/10.1145/3804494 

Authors’ addresses: Daqi Lin, daqil@nvidia.com, NVIDIA, USA; Markus Kettunen, mkettunen@nvidia.com, NVIDIA, Finland; Chris Wyman, chris.wyman@acm.org, NVIDIA, USA. 

Please use nonacm option or ACM Engage class to enable CC licenses This work is licensed under a Creative Commons Attribution 4.0 International License. 

© 2026 Copyright held by the owner/author(s). ACM 2577-6193/2026/5-ART13 https://doi.org/10.1145/3804494 

Proc. ACM Comput. Graph. Interact. Tech., Vol. 9, No. 1, Article 13. Publication date: May 2026. 

Daqi Lin, Markus Kettunen, and Chris Wyman 

13:2 

## **1 INTRODUCTION** 

ReSTIR Path Tracing [Lin et al. 2022] greatly improves path quality by spatiotemporally reusing path samples. But their prototype has high compute and memory overheads, yielding sluggish performance even on high-end GPUs (originally 40-80 ms per frame on an RTX 3090). 

As published, ReSTIR PT exhibits robustness problems: it suffers spatiotemporal correlations, variance near disocclusions, and color noise. The hybrid shift, which enables ReSTIR PT to handle specular light transport, relies on simple roughness and distance thresholds to determine reuse strategy; these thresholds often benefit from adjustment per scene. While further research proposes partial solutions [Kettunen et al. 2023; Sawhney et al. 2024; Zhang et al. 2024], they do not fix the poor performance. In this paper, we propose techniques that greatly optimize ReSTIR PT’s performance and improve its robustness and quality, reducing the gap to productization. Our specific contributions include: 

- Halving shift mapping costs in spatial reuse by reciprocal neighbor selection<sup>∗</sup> (Section 3); 

- New ray footprint thresholds that adapt to scene and materials<sup>∗</sup> (Section 4); 

- Reducing correlation artifacts by sample duplication maps<sup>∗</sup> (Section 5); 

- Improving quality and cost by unifying ReSTIR for direct and indirect light (Section 6); and 

- Other optimizations that boost performance and improve robustness by reducing color and disocclusion noise. (Section 6) 

## **2 BACKGROUND** 

Recent advances in hardware and denoising, like SVGF [Schied et al. 2017], NVIDIA NRD [NVIDIA 2020], and DLSS Ray Reconstruction [NVIDIA 2025], have made real-time ray-traced global illumination (GI) practical. Many real-time solutions today rely on radiosity-like recursive gathering with discretization, caching, and interpolation (e.g., Lumen [Wright et al. 2022], DDGI [Majercik et al. 2019], and Surfel GI [Halen et al. 2021]); this limits accuracy and responsiveness. Brute force path tracing is accurate but slow, requiring many samples for a denoisable image. 

More recently, spatiotemporal reservoir resampling (ReSTIR) [Bitterli et al. 2020] aggressively reuses Monte Carlo samples from other pixels and frames in an unbiased way. When applied to path samples, ReSTIR amortizes the cost of tracing paths over space and time. This preserves path tracing’s accuracy but delivers tens to hundreds more samples at the same cost. ReSTIR DI [Bitterli et al. 2020] enables real-time, many-light rendering and ReSTIR GI [Ouyang et al. 2021] targets diffuse/mid-glossy surfaces. Our work speeds and improves robustness of ReSTIR PT [Lin et al. 2022], improving the unbiased rendering of general paths, including multi-bounce specular reflection and caustics. Below, we review the resampling theory behind these ReSTIR algorithms. 

## **2.1 RIS and GRIS** 

Resampled importance sampling (RIS) [Talbot et al. 2005] aims to sample from a low-variance target PDF _𝑝_ ¯ ∝ _𝑝_ ˆ ≈ _𝑓_ to estimate an integral ∫Ω<sup>_𝑓_(</sup><sup>_𝑥_) d</sup><sup>_𝑥_. As</sup><sup>_𝑝_¯is often difficult to sample directly,</sup> RIS draws _𝑀_ i.i.d. candidates _𝑋𝑖_ ∼ _𝑝_ (a suboptimal source PDF), selecting one _𝑌_ proportional to resampling weights _𝑤𝑖_ = (1/ _𝑀_ ) ˆ _𝑝_ ( _𝑋𝑖_ )/ _𝑝_ ( _𝑋𝑖_ ). As _𝑀_ →∞, _𝑌_ ’s distribution converges to _𝑝_ ¯ = _𝑝_ ˆ/∫Ω<sup>_𝑝_ˆ(</sup><sup>_𝑥_) d</sup><sup>_𝑥_. Talbot et al. [2005] construct</sup><sup>_𝑝_ˆby dropping the visibility term in direct illumination</sup> to improve sampling efficiency. The expression _𝑓_ ( _𝑌_ ) _𝑊𝑌_ unbiasedly estimates integral ∫Ω<sup>_𝑓_(</sup><sup>_𝑥_) d</sup><sup>_𝑥_,</sup> where _𝑊𝑌_ = (1/ _𝑝_ ˆ( _𝑌_ ))<sup>�</sup> _𝑖_<sup>_𝑀_</sup> =1<sup>_𝑤𝑖_estimates 1/</sup><sup>_𝑝𝑌_(</sup><sup>_𝑌_), the reciprocal of</sup><sup>_𝑌_’s unknown PDF.</sup> 

Generalized resampled importance sampling (GRIS) [Lin et al. 2022] allows random variables like _𝑌_ to be used as candidates in another resampling pass. This chained resampling forms the 

> ∗Asterisked contributions apply to other spatiotemporal reuse algorithms, e.g., Bauszat et al. [2017]; Ouyang et al. [2021] 

Proc. ACM Comput. Graph. Interact. Tech., Vol. 9, No. 1, Article 13. Publication date: May 2026. 

ReSTIR PT Enhanced: Algorithmic Advances for Faster and More Robust ReSTIR Path Tracing 

13:3 

basis of ReSTIR. In GRIS, candidate samples can have different distributions, come from different domains Ω _𝑖_ , and be correlated. The samples are mapped to the target domain Ω by bijective shift mappings _𝑇𝑖_ : Ω _𝑖_ → Ω to compute _𝑌𝑖_ = _𝑇𝑖_ ( _𝑋𝑖_ ), which are used in the resampling weights 



with the resampling output _𝑌_ sampled proportionally to the _𝑤𝑖_ . Here, _𝑊𝑋𝑖_ are _unbiased contribution weights_ , random variables that satisfy E[ _𝑊𝑋𝑖_ | _𝑋𝑖_ ] = 1/ _𝑝𝑋𝑖_ ( _𝑋𝑖_ ), usually computed from prior RIS or GRIS passes, | _𝜕𝑇𝑖_ / _𝜕𝑋𝑖_ | is the Jacobian determinant of the shift mapping, and _𝑚𝑖_ are _resampling MIS weights_ that normalize the coverage of the target domain. Convergence to _𝑝_ ¯ happens when Var[<sup>�</sup> _𝑖_<sup>_𝑀_</sup> =1<sup>_𝑤𝑖_]→0.Unbiasedintegrationof</sup><sup>_𝑓_using</sup><sup>_𝑌_istypicallyguaranteedbyincludinga</sup> _canonical sample_ that completely covers the support of _𝑝_ ˆ as one of the inputs to GRIS. 

## **2.2 ReSTIR** 

Reservoir-based spatiotemporal importance resampling (ReSTIR) applies chained GRIS passes across frames to progressively improve the sample distribution, greatly improving real-time image quality. Each pixel maintains a reservoir, which is essentially a tuple ( _𝑋,𝑊𝑋 ,𝑐_ ) carrying a sample, its unbiased contribution weight, and a confidence weight (originally called an “effective sample count”) used to weigh domains/techniques when computing resampling MIS. In a GRIS pass, the current pixel’s reservoir provides the canonical sample, and other pixels contribute neighboring samples. The reservoir is then updated to the selected output sample _𝑌_ , whose confidence weight accumulates those of all input samples. 

A ReSTIR frame starts by generating per-pixel initial candidates and using RIS to select an initial sample _𝑌_ . An efficient streaming implementation uses weighted reservoir sampling [Chao 1982] and initializes a reservoir with _𝑐_ = 1. This is followed by temporal and spatial reuse passes, which are GRIS passes including neighbor samples as candidates. Finally, reservoirs are used to shade the current frame using estimator _𝑓_ ( _𝑋_ ) _𝑊𝑋_ and these reservoirs get passed to the next frame to serve as temporal neighbors. Below are key details of temporal and spatial reuse. 

_Temporal Reuse._ In the temporal GRIS, a neighbor is determined using temporal reprojection following the shading point’s motion. The confidence weight _𝑐_ temp of the temporal reservoir is capped by a confidence cap _𝑐_ Cap giving a new confidence after resampling of min( _𝑐_ Cap _,𝑐_ temp) + 1. This avoids slow sample turnover in dynamic scenes and unlimited build ups of correlation. 

_Spatial Reuse._ In spatial GRIS, _𝑀_ − 1 spatial neighbors are randomly drawn from a disk with a set radius (often 30 pixels). Neighbors whose G-Buffer attributes (e.g., normal, depth) differ too much are rejected. After spatial reuse, the confidence weight sums all accepted neighbors’ confidences. 

## **2.3 ReSTIR PT** 

ReSTIR PT [Lin et al. 2022] aims to reuse all types of paths produced by a path tracer. For a length- _𝑑_ path x¯ = [x0 _,_ x1 _,_ x2 _, ...,_ x _𝑑_ ], ReSTIR GI [Ouyang et al. 2021] reuses x2 and later vertices using the same _reconnection shift_ as ReSTIR DI [Bitterli et al. 2020]. But this fails when x1 or x2 are specular. ReSTIR PT introduced a _hybrid shift_ that postpones reconnection until vertices satisfy certain reconnection criteria; until reconnection, the new path is traced by reusing the same random numbers. ReSTIR PT only evaluates a single BSDF lobe at each vertex, allowing shifts to each BSDF lobe separately; here, we omit lobe indices for clarity and defer details to our supplemental. 

_Path space definition._ The path space integral<sup>�</sup> _𝑑_<sup>∞</sup> =1 ∫Ω _𝑑_<sup>_𝑓_(x¯) d¯x spans paths of all lengths, where</sup> _𝑓_ (x¯) contains BSDF, geometry, sensor, and emitter terms. Starting from primary hit x1, the path 

Proc. ACM Comput. Graph. Interact. Tech., Vol. 9, No. 1, Article 13. Publication date: May 2026. 

Daqi Lin, Markus Kettunen, and Chris Wyman 

13:4 

tracer samples each vertex’s BSDF for a path continuation direction and each vertex also performs next event estimation (NEE). To enable different shift mappings in these cases, ReSTIR PT attaches a technique index to x¯ to indicate if the final path vertex is sampled by NEE or a BSDF-sampled continuation ray; this requires splitting the integrand and using MIS weights. 

_Hybrid Shift._ To shift a path x¯ to y¯ = _𝑇_ (x¯), prior gradient-domain rendering work (e.g. Kettunen et al. [2015]) requires the reconnection vertex x _𝑘_ and the preceding vertices x _𝑘_ −1 and y _𝑘_ −1 to be all “rough”. To avoid storing the base path vertices or replaying the base path, ReSTIR PT precomputes a reconnection vertex x _𝑘_ on x¯ by finding the first pair (x _𝑘_ −1, x _𝑘_ ) that satisfies two reconnection criteria: a roughness threshold min( _𝛼_ x _𝑘_ −1 _, 𝛼_ x _𝑘_ ) ≥ _𝛼_ min (with _𝛼_ from the path’s sampled lobes) and a distance threshold ∥x _𝑘_ − x _𝑘_ −1∥≥ _𝑑_ min that avoids geometric singularities. This predetermines the replayed path length and allows GPU optimization. Shift invertibility is checked when connecting from y _𝑘_ −1 to x _𝑘_ . 

_Jacobian._ ReSTIR PT implementations often use a primary sample space (PSS) parameterization. Given path length _𝑑_ and sampling technique _𝑡_ , the path tracer turns random number sequences u¯ into paths. With x¯ = 𝒳 _𝑡,𝑑_ (u¯) denoting the produced path, we integrate _𝐹_ (u¯) = _𝜔𝑡_ (x¯) _𝑓_ (x¯)/ _𝑝𝑡_ (x¯) over a unit hypercube, where the MIS weight _𝜔𝑡_ splits the path space between techniques and _𝑝𝑡_ (x¯) is the path space PDF. We assume u¯ is associated with x¯ and _𝑇_ (u¯) maps it to a sequence associated with y¯, and denote quantities related to x¯ and y¯ via superscripts _𝑥_ and _𝑦_ . Then, the PSS Jacobian determinant for the hybrid shift is 



which corresponds to reconnecting from y _𝑘_ −1 to x _𝑘_ ; the Jacobian is 1 if no reconnection happens. Above, _𝑝𝑘_<sup>_𝑥_(</sup><sup>_𝜔𝑘_)≡</sup><sup>_𝑝_(</sup><sup>_𝜔𝑘_|x</sup><sup>_𝑘,_−</sup><sup>_𝜔𝑘_−1)isthesolid-anglePDFofsamplingthedirection−</sup><sup>_𝜔𝑘_−1=</sup> x� _𝑘_ x _𝑘_ −1 =<sup>−−−−−→</sup> x _𝑘_ x _𝑘_ −1/∥<sup>−−−−−→</sup> x _𝑘_ x _𝑘_ −1∥, _𝑝𝑘_<sup>_𝑥_(</sup><sup>_𝜔𝑘_)isreplacedwith1for</sup><sup>_𝑘_=</sup><sup>_𝑑_,and</sup><sup>_𝜔_</sup> _𝑘_<sup>′</sup> −1<sup>isashorthandfor</sup> y� _𝑘_ −1x _𝑘_ . The single-sided geometry term _𝐺_ (x → y) is cos _𝜃_ /||x − y||<sup>2</sup> , where _𝜃_ is the angle between y� _𝑘_ −1x _𝑘_ and y _𝑘_ −1’s normal. 

_Implementation._ ReSTIR PT assumes a direct illumination (DI) sampling method exists and skips DI paths in its own estimator [Lin et al. 2022]. At initial resampling, a path tree is generated by the path tracer and the indirect lighting paths are fed through RIS to resample one representative path. The associated random seed, reconnection vertex x _𝑘_ , and the incident radiance and direction on x _𝑘_ are stored in the reservoir to facilitate the shift mapping in temporal and spatial reuse. 

Our _ReSTIR PT Enhanced_ retains the baseline architecture [Lin et al. 2022] while halving the spatial reuse cost by neighbor pairing (Section 3), introducing scene-independent reconnection criteria (Section 4), applying adaptive _𝑐_ Cap reduction for sample decorrelation (Section 5), and it contains various optimizations (Section 6) that enhance ReSTIR PT’s performance. 

## **3 A MORE EFFICIENT PAIRED SPATIAL REUSE** 

ReSTIR PT’s spatial and temporal reuse rely on GRIS resampling. Neighbor reuse requires not only shifting neighbors to the current pixel but also needs to shift the current pixel to neighbors to compute good MIS weights. Even with efficient pairwise MIS [Lin et al. 2022], all spatial neighbors require two shifts, doubling the work (Figure 2, left). 

For pixel _𝐴_ to reuse from _𝐵_ , we must shift both pixels’ paths to the other. We observe that after _𝐴_ reuses from _𝐵_ , _𝐵_ can reuse from _𝐴_ for free, saving 50% by amortizing duplicate spatial shifts (Figure 2, right). To efficiently pair pixels _𝐴_ and _𝐵_ , we precompute a random reuse texture storing a 

Proc. ACM Comput. Graph. Interact. Tech., Vol. 9, No. 1, Article 13. Publication date: May 2026. 

ReSTIR PT Enhanced: Algorithmic Advances for Faster and More Robust ReSTIR Path Tracing 

13:5 



<!-- Start of picture text -->
is Py<br><!-- End of picture text -->

**Fig. 2.** **_Left_** _: When performing spatial reuse for the current pixel (blue), the chosen neighbors (red) must shift their paths to the current pixel, while the current pixel must shift its path to the neighbors for MIS weights._ **_Right_** _: Blue pixel reusing from red requires the same shift mappings as the red pixel reusing from blue. Hence, if blue reuses from red, red can reuse from blue without paying for shifts._ 



<!-- Start of picture text -->
Initialization Alternating shuffles<br>1[1[2]2 3124<br>3[3/4/4)||3°3/4°4| 1[3,2]4,<br>Shufflen, times<br><!-- End of picture text -->

**Fig. 3.** **_Left_** _: We initialize the image with consecutive link indices._ **_Right_** _: We shuffle each 2_ × _2 pixel block with a random permutation and repeat 𝑛𝜎 times to reach the desired standard deviation, offsetting every other shuffle diagonally by one._ 

coordinate offset to each pixel’s coupled neighbor; _𝐴_ stores the delta to _𝐵_ , and _𝐵_ stores the delta to _𝐴_ . To support reuse from multiple neighbors, we prepare multiple independent textures. 

## **3.1 Computing reuse textures** 

Producing a reuse texture with coordinate deltas following a given distance distribution is a nontrivial problem with global constraints, and we found optimization-based approaches slow and unreliable. Instead, we permute an initial texture with random shuffles; the deltas produced by the random walks follow a normal distribution. 

We assume a target standard deviation _𝜎_ ≥ 0 _._ 8 pixels<sup>1</sup> . We then choose an even image-size like<sup>2</sup> 254 × 254, and fill it with consecutive link indices (Figure 3, left). We then shuffle the link indices with _𝑛𝜎_ tiled 2 × 2 shuffles, every other offset by 1 while looping over the edge (Figure 3, right). The repeat count is 



The repeated shuffles move each link index roughly according to a normal distribution with standard deviation _𝜎_ /√2. We then pair the pixels with matching link indices. With correlation decreasing at each shuffle, the coordinate delta roughly follows a normal distribution with standard deviation <u>√</u> _𝜎_<sup>2</sup> /2 + _𝜎_<sup>2</sup> /2 = _𝜎_ . The negative powers in Equation 3 are function fit correction for small _𝜎_ . 

> 1Smallest supported _𝜎_ is 0.8, given by applying one shuffle iteration. 

> 2Resolutions 256 × 256 require 16-bit channels to represent coordinate deltas 128 and -128 and past. 

Proc. ACM Comput. Graph. Interact. Tech., Vol. 9, No. 1, Article 13. Publication date: May 2026. 

Daqi Lin, Markus Kettunen, and Chris Wyman 

13:6 

The links are now contained within the reuse texture, and links near edges can be long. To make the reuse texture tileable, we break long links by subtracting the width of the square texture from coordinate deltas greater than half the width, and adding it to coordinate deltas less than minus half the width. Finally, we pack these _𝑥_ and _𝑦_ directional deltas into a 16-bit luminance texture. 

_Finding links._ To find coordinate deltas, each pixel locates the other pixel with its link index. Our CUDA implementation constructs an _index table_ of size _𝑁_ /2 × 2, where _𝑁_ is the pixel count of the reuse texture. First, every pixel reads its own link index, and writes its pixel location into the index table at location (link index, 0). This is a parallel write with a data race, so only half the pixels succeed. Next, each pixel whose location is not at (link index, 0), writes it to (link index, 1). This enables every pixel to find its linked neighbor in the index table. 

## **3.2 Using reuse textures** 

Assuming spatial reuse with _𝑁_ neighbors, we preload _𝑁_ reuse textures. To avoid correlation artifacts from texture repeats, we pick a different size for each, e.g., 254, 230 and 210 for three neighbors<sup>3</sup> . Our reuse texture is self-inverting: _𝐴_ links to _𝐵_ , and _𝐵_ links to _𝐴_ . Hence, the reuse texture should be changed every frame. Rather than generating many reuse textures, we randomly flip, mirror, transpose, and offset each reuse texture each frame. Empirically, this completely solves the problem. 

## **3.3 Discussion** 

Bekaert et al. [2002] split 16 × 16 pixel blocks into groups of 16 pixels in an _𝑁_ -rooks pattern and perform all-to-all reuse within each group. This also avoids the additional work for MIS weights, but produces structured artifacts at low sample counts. We avoid such structure by overlaying large different-sized Gaussian-permuted reuse textures; each pixel follows a different, random reuse pattern with no block borders. Our reuse textures can also be used in classical path reuse. 

## **4 SCENE-INDEPENDENT RECONNECTION CRITERIA** 

During ReSTIR’s sample reuse, neighbor samples are shifted into the current domain prior to reuse. Ideal shift mapping preserves the target PDF value of the path [Wyman et al. 2023], i.e. _𝑝_ ¯ _𝑗_ ( _𝑇_ (u¯)) �� _<u>𝜕𝑇𝜕</u>_ u¯ �� ≈ _𝑝_ ¯ _𝑖_ (u¯). Without loss of generality, we analyze the reconnection criteria for ReSTIR PT’s hybrid shift in primary sample space; we assume _𝑝_ ¯ _𝑗_ ( _𝑇_ (u¯)) ≈ _𝑝_ ¯ _𝑖_ (u¯)<sup>4</sup> and focus on �� _<u>𝜕𝑇𝜕</u>_ u¯ ��, which is only affected by the choice of reconnection vertex. A good shift mapping should have <u>�</u> _<u>𝜕𝑇𝜕</u>_ u¯ �� ≈ 1. 

The original hybrid shift [Lin et al. 2022] selects reconnection vertex x _𝑘_ using a distance threshold ||x _𝑘_ − x _𝑘_ −1|| ≥ _𝑑_ min that forbids short path reconnections (i.e., near corners) and a roughness threshold min( _𝛼_ x _𝑘_ −1 _, 𝛼_ x _𝑘_ ) ≥ _𝛼_ min to forbid connections on highly glossy surfaces. These thresholds can be suboptimal and often require scene-specific tuning, since suitable reconnection distances and roughness levels are interdependent. 

We introduce a _dual ray footprint threshold_ and a new _single-vertex roughness threshold_ , giving reconnection constraints that are more robust in the presence of specular materials, do not require scene-specific tuning, and enable earlier (and hence cheaper) reconnection opportunities. 



3Our heuristic algorithm tries to minimize near-periods within 3840 pixels, i.e., cases where a multiple of a texture’s width is close to a multiple of another texture’s width. 

> 4Geometry and microfacet NDF terms are cancelled in _𝜔𝑡_ (x¯ ) _𝑓_ (x¯ )/ _𝑝𝑡_ (x¯ ), which avoids extreme values in the shift. 

Proc. ACM Comput. Graph. Interact. Tech., Vol. 9, No. 1, Article 13. Publication date: May 2026. 

ReSTIR PT Enhanced: Algorithmic Advances for Faster and More Robust ReSTIR Path Tracing 

13:7 

The first one is between the area densities of vertex x _𝑘_ when hypothetically traced from y _𝑘_ −1 and when originally traced from x _𝑘_ −1. The second one is between the solid angle densities of _𝜔𝑘_ when hypothetically sampled with outgoing direction y� _𝑘_ −1x _𝑘_ and when originally sampled with outgoing direction x� _𝑘_ −1x _𝑘_ . Our threshold aims to bound the change of these densities. 

The reciprocal of area probability density of a vertex on a surface has the geometric meaning of the footprint, i.e., the area represented by the sample. Intuitively, a longer reconnection distance or rougher material at x _𝑘_ −1 enlarges the ray footprints and reduces the density change at shift mapping, making a shift reconnecting _from_ x _𝑘_ −1 more useful. To bound Equation 4b, we look at _inverse ray footprint_ ( _𝑝𝑘_<sup>_𝑥_(</sup><sup>_𝜔𝑘_)</sup><sup>_𝐺_(x</sup><sup>_𝑘_→x</sup><sup>_𝑘_−1))−1,whichisthereciprocalareadensityofx</sup><sup>_𝑘_−1</sup> produced by tracing from x _𝑘_ . Our analysis shows that thresholding this footprint equivalently bounds the change of solid angle density in Equation 4b. 

Specifically, our new criterion for reconnection is then defined by a dual ray footprint threshold: 



where the RHS is a constant multiple of the primary ray footprint that measures the distance between the nearby primary hits. We found the constant multiplier _𝑐_ = 0 _._ 02 (i.e., _𝑐_ /100 = 0 _._ 0002) to work well in diverse scenes. We also enforce a single-vertex threshold ( _𝛼_ x _𝑘_ −1 ≥ _𝛼_ min) at vertex x _𝑘_ −1 to cheaply avoid trying to reconnect in some tricky edge cases. 

## **4.1 Explanation of the dual footprint thresholds** 

We provide a brief explanation of how our dual footprint thresholds achieve the bounding and provide a full derivation in the supplemental material. Consider shifting a path x¯ in one pixel to y¯ = _𝑇_ (x¯) in another pixel. For a candidate reconnection vertex x _𝑘_ on x¯, connecting from y _𝑘_ −1 instead of x _𝑘_ −1 perturbs the _path-traced area density_ of x _𝑘_ and the angular density of _𝜔𝑘_ . We want these relative changes to be small: for small _𝜖_ , 



_Ray footprint threshold._ Equation 6a requires bounding the change in area density at x _𝑘_ when reconnecting from y _𝑘_ −1 instead of x _𝑘_ −1. Assuming random replay preserves local area density (Figure 4), we analyze how the area density of x _𝑘_ changes under a spatial displacement Δx _𝑘_ = y _𝑘_ −x _𝑘_ , where y _𝑘_ is obtained by replaying from y _𝑘_ −1. 

We assume ∥Δx _𝑘_ ∥ is bounded by a constant multiple of the _primary ray footprint_ [Müller et al. 2021], ∥Δx _𝑘_ ∥ _< 𝑐_ 1 _𝑅_<sup>x¯</sup> <u>∥x0</u> −x1 <u>∥</u><sup>2</sup> pri<sup>=</sup><sup>_𝑐_1</sup> ~~<u>√</u>~~ ⟨ _𝑛_ x1 _,_ �x1x0 ⟩/(4 _𝜋_ )<sup>. Further assuming that relative density variation within</sup> a footprint of radius<sup>~~√~~</sup> _𝑐_ 2/( _𝑝𝑘_<sup>_<u>𝑥</u>_</sup> −1<sup>(</sup><sup>_𝜔𝑘_−1)</sup><sup>_𝐺_(x</sup><sup>_𝑘_−1 →x</sup><sup>_𝑘_))is bounded by</sup><sup>_𝜖_, a sufficient condition for</sup> Equation 6a is ( _𝑝𝑘_<sup>_𝑥_</sup> −1<sup>(</sup><sup>_𝜔𝑘_−1)</sup><sup>_𝐺_(x</sup><sup>_𝑘_−1→x</sup><sup>_𝑘_))−1</sup><sup>_>_</sup> _𝑐𝑐_ <u>12</u><sup>2(</sup><sup>_𝑅_</sup> pri<sup>x¯)2.This</sup><sup>_rayfootprint_thresholdavoids</sup> reconnections near density peaks caused by glossy BSDFs or geometric singularities. In practice, we collapse the constants into a single parameter _𝑐_ in Equation 5; larger _𝑐_ makes reconnection more conservative, i.e., favor postponing. 

_Inverse ray footprint threshold._ The ray footprint bounds changes in area density at x _𝑘_ but does not capture changes in the BSDF sampling PDF _𝑝𝑘_<sup>_𝑥_(</sup><sup>_𝜔𝑘_) caused by altering the outgoing direction</sup> − _𝜔𝑘_ −1, which can be significant for low-roughness glossy materials. 

Proc. ACM Comput. Graph. Interact. Tech., Vol. 9, No. 1, Article 13. Publication date: May 2026. 

Daqi Lin, Markus Kettunen, and Chris Wyman 

13:8 











<!-- Start of picture text -->
(a)  GGX roughness  0 . 3 (b)  (zoomed in) (c)  GGX roughness  0 . 7 (d)  (zoomed in)<br><!-- End of picture text -->

**Fig. 4.** _Visualizing secondary hits (red and blue points) sampled for two pixels (red and blue triangles on the floor) by BSDF sampling with the same random numbers reveals a near-exact spatial shift in the sample populations and thus sample density. Green circles show radii proportional to sample footprints, i.e., reciprocal probability densities. At a fixed surface point, larger footprints generally imply a smaller density change between the red and blue populations and thus safer reconnection. Studying different GGX roughnesses (a-b, c-d) shows larger roughnesses generally allow safer reconnection._ 

We therefore define the _inverse ray footprint_ ( _𝑝𝑘_<sup>_𝑥_(</sup><sup>_𝜔𝑘_)</sup><sup>_𝐺_(x</sup><sup>_𝑘_→x</sup><sup>_𝑘_−1))−1. Assuming approximate</sup> BSDF PDF reciprocity for glossy materials,<sup>5</sup> it can be interpreted as the area per sample of a reverse-traced ray (as if the path is traced from the light). Under the same smoothness assumptions as above, and with _𝐺_ (x _𝑘_ → x _𝑘_ −1) ≈ _𝐺_ (x _𝑘_ → y _𝑘_ −1) for distant reconnections, a sufficient condition for Equation 6b is ( _𝑝𝑘_<sup>_𝑥_(</sup><sup>_𝜔𝑘_)</sup><sup>_𝐺_(x</sup><sup>_𝑘_→x</sup><sup>_𝑘_−1))−1</sup><sup>_>𝑐_</sup> _𝑐_ <u>12</u><sup>2(</sup><sup>_𝑅_</sup> pri<sup>x¯)2. We reuse the same parameter</sup><sup>_𝑐_for both</sup> thresholds for simplicity, yielding the combined test in Equation 5. This enables safe reconnection to distant glossy surfaces without enforcing a roughness threshold at x _𝑘_ .<sup>6</sup> 

## **4.2 Single-vertex roughness threshold** 

We retain a roughness threshold at x _𝑘_ −1 to guard against cases where parallax, curvature, or very low roughness can make footprint-based bounds unreliable. These cases would otherwise require excessively large geometric bounds or overly large footprint tolerances. 

The roughness threshold also handles reconnection to environment lights, where Δx _𝑘_ is unbounded and sharp changes in angular density may occur. With this safeguard, we can use a tighter footprint parameter ( _𝑐_ ) that performs well in typical scenes. For non-parametric materials, we discuss using the BSDF sampling PDF as a roughness proxy in the supplemental material. 

## **5 CORRELATION REDUCTION WITH DUPLICATION MAPS** 

One of the greatest challenges to ReSTIR’s stability is sample correlation. The main sources for sample correlation are 

- Low-probability, high-energy fireflies from initial sampling, and 

- Imperfect shift mappings that artificially shift samples into high-energy fireflies. 

These fireflies spread to neighbor pixels via spatiotemporal reuse, forming correlation blobs (e.g., disks or streaks) sometimes occupying numerous pixels and persisting over many frames. Poorly chosen parameters exacerbate visual artifacts (e.g., a small spatial reuse radius). 

> 5We assume _𝑝𝑘𝑥_<sup>(</sup><sup>_𝜔𝑘_)=</sup><sup>_𝑝_(</sup><sup>_𝜔𝑘_|x</sup><sup>_𝑘,_−</sup><sup>_𝜔𝑘_−1)≈</sup><sup>_𝑝_(−</sup><sup>_𝜔𝑘_−1|x</sup><sup>_𝑘,𝜔𝑘_), which is exact for NDF sampling of microfacet glossy</sup> material and approximate for VNDF sampling [Heitz 2018]. 6For diffuse or emissive x _𝑘_ , reconnection does not affect _𝑝𝑘𝑥_<sup>(</sup><sup>_𝜔𝑘_)and the inverse ray footprint test is skipped.</sup> 

Proc. ACM Comput. Graph. Interact. Tech., Vol. 9, No. 1, Article 13. Publication date: May 2026. 

ReSTIR PT Enhanced: Algorithmic Advances for Faster and More Robust ReSTIR Path Tracing 

13:9 









<!-- Start of picture text -->
(a)  Ours (no decorrelation) (b)  + Correlation reduction (c)  Reference<br>(12.9 ms, FLIP: 0.297) (13.6 ms, FLIP: 0.295)<br>onl<br>(d)  Duplication map of (a) (e)  Duplication map of (b) (f)  Bias visualization<br><!-- End of picture text -->

**Fig. 5.** _Evaluating our de-duplication method (b) in Kitchen, where camera motion along the ground introduces many correlations (a) due to reuse of suboptimal temporal neighbors. We compute a duplication map (d, e), counting duplicate samples in each 17_ × _17 neighborhood (black to red represents 0–20% duplication in the visualization) and use this to adaptively decrease 𝑐Cap. This introduces some bias, appearing as energy loss. Our average absolute relative bias_ ( _mean_ (| _bias_ |/ _ref._ )) _is 3.25% in (b). We also visualize this bias per-pixel (f), where more glossy surfaces exhibit larger bias._ 

Correlation artifacts make denoising ReSTIR challenging, as artifacts get detected as signal. Previous work fights sample correlation by partially reusing paths while keeping prefixes independent [Kettunen et al. 2023] or mutating samples to reduce impoverishment [Sawhney et al. 2024]. But these methods are expensive and only reduce certain types of correlation. 

We introduce a simple but effective solution based on _𝑐_ Cap reduction, essentially limiting the temporal confidence weights adaptively. The motivation is simple—how high we weight temporal samples directly impacts potential correlation strength. Highly weighting temporal reuse provides more chances for fireflies to spread spatially and makes them more temporally persistent. 

Our solution measures prior frame’s correlations around each pixel and reduces _𝑐_ Cap in highly correlated areas. Our correlation measure simply counts pixels in a spatial neighborhood that share the current pixel’s sample, by which we mean the samples are shifted copies from the same initial candidate. We detect this by comparing the random seeds already stored for (potential) random replay. We implement our method as follows: 

- (1) After finishing each frame, compute a _sample duplication map_ . Each pixel counts how many reservoirs in the surrounding 17×17 region share its random seed. This count is divided by 288 to compute a duplication score _𝐷_ ∈[0 _,_ 1]. 

- (2) When doing temporal resampling, look up the duplication score corresponding to the temporal reservoir used (i.e., backprojecting the current pixel into prior frame). 

- (3) We compute a modified _𝑐_ Cap for temporal reuse as _𝑐_ Cap = lerp( _𝑐_ Cap<sup>Default</sup> _,𝑐_ Cap<sup>min</sup><sup>_, 𝐷𝛼_).Where</sup> _𝑐_ Cap<sup>Default</sup> is the default (e.g., 20 in Lin et al. [2022]) and _𝑐_ Cap<sup>minis the minimum</sup><sup>_𝑐_Capused when</sup> encountering _𝐷_ = 1. _𝛼_ sets the sensitivity of the reduction. _𝛼_ = 1 gives a linear reduction, while _𝛼_ closer to 0 gives a quick reduction as _𝐷_ increases. 

In our tests, we found _𝑐_ Cap<sup>min= 1 and</sup><sup>_𝛼_= 0</sup><sup>_._1 yield good results. Our adaptive</sup><sup>_𝑐_Capdramatically</sup> reduces correlation artifacts caused by high-energy samples produced by sampling or resampling. 

Proc. ACM Comput. Graph. Interact. Tech., Vol. 9, No. 1, Article 13. Publication date: May 2026. 

Daqi Lin, Markus Kettunen, and Chris Wyman 

13:10 

Because this approach adjusts confidence weights based on specific samples, the partition of unity of MIS weights _𝑚𝑖_ is violated, introducing a small bias. This bias only occurs in correlated areas where we modify _𝑐_ Cap; essentially our approach trades correlation for bias. Figure 5 shows our method in a very difficult scene, where it averages 3.25% absolute relative bias. 

## **6 IMPLEMENTATION IMPROVEMENTS** 

Beyond our algorithmic contributions, we introduce implementation-focused improvements that further enhance performance and robustness. We reduce redundancy by combining separate ReSTIR direct and indirect lighting passes into one unified algorithm (Section 6.1), apply GPU optimizations to reduce thread divergence (Section 6.2), and adopt existing techniques to reduce color (Section 6.3) and disocclusion noise (Section 6.4). The quality improvements these bring can be seen aggregated in Figure 1 and Figure 13, but more individual ablations are provided in the supplemental. 

## **6.1 Unifying Direct Lighting and Global Illumination** 

Real-time renderers often separately compute direct and indirect lighting, with indirect light often approximated more aggressively. But using ReSTIR, we can efficiently sample high-quality indirect illumination. Rather than running separate direct and indirect ReSTIR passes, as in Lin et al. [2022], we combine both using a single reservoir. 

The idea is simple. Lin et al. [2022] start by initially sampling a _path tree_ by emanating NEE rays from BSDF-sampled vertices [x2 _, ...,_ x _𝑛_ ] where _𝑛_ is limited by the maximum allowed bounces. Initial resampling selects and stores a single _path_ in the reservoir, i.e., x¯ = [x0 _,_ x1 _,_ x2 _, ...,_ x _𝑑_ ] with a _single_ NEE/BSDF ray connecting to a light at x _𝑑_ (see Section 2.3) with _𝑑_ ≥ 3 originally. Direct light can be handled by simply tracing another NEE ray from x1 when generating the path tree, and giving the (unified) initial resampling a chance to select a shorter direct lighting path ( _𝑑_ = 2) from the tree. The selected path is thus sampled from the full path space. 

This removes the cost and storage of separate ReSTIR passes for direct and indirect light. Somewhat surprisingly, it also improves quality as direct lighting now benefits from ReSTIR PT’s built-in shift mapping (especially on glossy highlights). Implementation details for modifying the MIS weights and source PDFs in primary sample space are provided in the supplemental material. 

For scenes with many lights, we optionally apply RIS when generating NEE samples at x1. As in ReSTIR DI, visibility is excluded from the target function and tested only for the selected light. We reuse the presampled light-tile scheme from Wyman and Panteleev [2021] to improve cache coherence: each frame precomputes 128 tiles with 1024 lights, from which each 8 × 8 screen tile draws a light tile to generate candidates per bounce. 

## **6.2 Improving GPU Performance by Reducing Divergence** 

Monte Carlo path tracing incurs substantial GPU divergence as neighboring paths often traverse very different geometry and materials. ReSTIR PT further increases divergence due to additional control flow and intermediate data, e.g., reservoirs. We apply a set of optimizations that reduce both code and data divergence, substantially improving GPU efficiency. 

_6.2.1 Low-level Code Optimization._ A naive ReSTIR PT implementation contains many branches arising from reservoir sampling and heterogeneous path cases. We reduce divergence by replacing most branches with conditional moves and simplifying computations while preserving equivalent results. We also reduce reservoir storage from 88 bytes to 64 bytes by removing unnecessary fields and applying lossy compression to selected quantities (details in the supplemental material). 

_6.2.2 Stream Compaction for Random Replay._ In many scenes, only a few pixel–neighbor pairs require random replay, yet warp-level execution forces all threads to wait. We parallelize over 

Proc. ACM Comput. Graph. Interact. Tech., Vol. 9, No. 1, Article 13. Publication date: May 2026. 

ReSTIR PT Enhanced: Algorithmic Advances for Faster and More Robust ReSTIR Path Tracing 

13:11 

pixel–neighbor pairs, applying stream compaction to discard pairs that do not require replay. This reduces warp divergence and the number of active warps, yielding a substantial speedup. 

_6.2.3 Forced NEE Light Reconnection._ During random replay, paths ending at NEE-sampled light vertices incur expensive light sampling that other path types avoid. Since replayed random numbers usually select the same light (e.g., with power-based sampling), we force reconnection to such vertices if no earlier reconnection is found. This removes light sampling from random replay, improving performance. Any potential variance increase is mitigated by path MIS weights, which already downweight these cases (as such surfaces are typically glossy). 

_6.2.4 Russian Roulette._ Initial resampling is often the most expensive stage due to long, divergent paths. We apply Russian roulette to reduce average path length at initial sampling, but remove it from random replay to avoid killing valid paths during reuse. This effectively removes the roulette dimensions from the path parametrization and applies an external roulette at initial path sampling, modifying the sampling PDF in primary sample space (see supplemental). Although this slightly increases noise in initial samples, spatiotemporal reuse largely suppresses its impact in the final result, and the roulette failures at shift mapping are completely avoided. 

## **6.3 Reducing Color Noise** 

ReSTIR inherently suffers from color noise because resampling operates on a scalar target function _𝑝_ ˆ, while the integrand **_F_** ( _𝑦_ ) is RGB-valued. GRIS therefore samples primarily according to luminance, leaving chroma poorly importance-sampled. 

Prior work [Kettunen et al. 2023; Lin et al. 2022] noted that the estimate **_F_** ( _𝑌_ ) _𝑊𝑌_ can be improved by marginalizing over the random index choice. In ReSTIR PT, this improvement comes at almost no extra cost: since _𝑝_ ˆ = | **_F_** |, **_F_** ( _𝑌𝑖_ ) is already evaluated during spatial reuse. We accumulate vectorvalued resampling weights **_w_** _𝑖_ = _𝑚𝑖_ ( _𝑌𝑖_ ) **_F_** ( _𝑌𝑖_ ) _𝑊𝑋𝑖_ | _𝜕𝑇_ / _𝜕𝑋𝑖_ | and use<sup>�</sup> _𝑖_<sup>_𝑀_</sup> =1<sup>**_w_**</sup><sup>_𝑖_for shading. As spatial</sup> neighbors typically contain uncorrelated chroma noise<sup>7</sup> , spatial reuse naturally averages it out. This decouples resampling and shading: scalar weights drive future resampling, while vector weights are used for shading. Unlike similar techniques for direct lighting [Wyman and Panteleev 2021], our method incurs no additional computations. 

## **6.4 Reducing Disocclusion Noise with Dual Motion Vectors** 

Disocclusion causes severe noise in ReSTIR as newly visible surfaces lack any temporal history. Dual motion vectors [Zeng et al. 2021] address this by reprojecting pixels using an alternative motion vector that assumes consistent relative motion between occluding and disoccluded surfaces. 

We apply this directly to ReSTIR’s temporal resampling and observe a substantial reduction in disocclusion noise. While prior work avoids copy-paste artifacts by storing incident radiance fields, this issue does not arise in ReSTIR PT: unbiased path resampling prevents pattern cloning. The remaining correlated noise is mild and can be further reduced using our decorrelation strategy (Section 5). 

## **7 EVALUATION** 

Our evaluation uses a PC with NVIDIA RTX 5880 Ada graphics card and AMD Ryzen Threadripper PRO 3975WX 32-Cores CPU. We use the Falcor [Benty et al. 2020] real-time rendering framework and start optimization from the original ReSTIR PT source code [Lin et al. 2022]. All images are rendered in 1920×1080. We use the HDR version of FLIP [Andersson et al. 2021] as the metric for 

> 7Although spatiotemporal reuse can also create spatially correlated color noise, such correlation is usually not among the randomly sampled spatial neighbors in the new frame. 

Proc. ACM Comput. Graph. Interact. Tech., Vol. 9, No. 1, Article 13. Publication date: May 2026. 

Daqi Lin, Markus Kettunen, and Chris Wyman 

13:12 

**Table 1.** _Frame and pass costs (in milliseconds) averaged over four scenes (Veach Ajar, Carousel, Opera House, and Bathroom) of varying complexity. Methods under the dashed line incur added cost._ 

|Method|Total<br>frame|Initial<br>sampling|Temporal<br>reuse|Spatial<br>reuse|ReSTIR DI<br>and others|
|---|---|---|---|---|---|
|Baseline|35.73|10.59|5.30|14.79|5.05|
|+Code microop (Section 6.2.1)|32.98|10.70|4.16|13.06|5.07|
|+Forced NEE reconnect (Section 6.2.3)|29.75|11.44|3.40|9.73|5.19|
|+Replay compaction (Section 6.2.2)|26.81|11.79|2.66|6.99|5.37|
|+Paired spatial reuse (Section 3)|25.02|12.56|2.80|4.11|5.55|
|+Russian roulette (Section 6.2.4)|16.52|5.21|2.24|3.83|5.24|
|+Unify DI & GI (Section 6.1)<br>- - - - - - - - - - - - - - - - - - - - - - - - - - - -|13.04<br>- - - - - -|6.47<br>- - - - - - - - -|2.14<br>- - - - - - - - -|3.43<br>- - - - - - -|1.00<br>- - - - - - - -|
|<br>+New thresholds (Section 4)|<br>14.51|<br>6.68|<br>2.92|<br>3.60|<br>1.31|
|+All improvements (Section 5,6.3,6.4)|15.53|6.77|3.20|3.73|1.83|



image error, as it closely aligns with the perceptual difference when flipping to compare images. We borrow ReSTIR PT’s default temporal and spatial parameters, namely _𝑐_ Cap = 20 for temporal resampling and use of 3 random spatial neighbors in a 30-pixel radius. 

For paired spatial reuse, we compute 3 pairing textures that realize Gaussian distributions with the same average sample radial distance for spatial resampling. As the mean sample distance of an isotropic Gaussian with standard deviation _𝜎_ is _𝜎_ √︁ _𝜋_ /2, and the mean sample distance of a uniform disk of radius _𝑅_ is 2 _𝑅_ /3, we propose choosing _𝜎_ = √︁8/(9 _𝜋_ ) _𝑅_ to match prior work’s empirically chosen _𝑅_ . For the default _𝑅_ = 30 in ReSTIR, this gives _𝜎_ = 16 _._ 0. Our paired spatial reuse does not fully halve costs, due to overhead from splitting spatial reuse in two: a pre-pass shifts each path to all paired neighbors, and a second pass performs resampling. 

We use 1 spp path tracing (one path tree per pixel) to produce the initial samples. For RIS of NEE samples (Section 6.1), we draw 32 light samples from the light tiles for NEE candidates at the primary hit, following Wyman and Panteleev [2021]. We reduce the number of NEE light candidates at deeper bounces using an inverse-square decay schedule, allocating 32/ _𝐵_<sup>2</sup> samples (clamped to a minimum of 1) at bounce index _𝐵_ . 

## **7.1 Performance** 

Table 1 shows performance of our techniques, with each row adding one new feature/optimization on top of a baseline of Lin et al.’s [2022] public source code. We first measure the speedup from our cost-reduction techniques, which provide an average 2.74× speedup across the four tested scenes. These scenes were chosen to reflect a range of geometry and material complexity. Results for individual scenes are provided in the supplemental material. To provide further insight into the effect of our low-level GPU optimizations, we profiled Opera House using NSight Graphics. The profiler data indicate that the optimizations in Section 6.2.1–6.2.3 reduce thread divergence and improve GPU computation efficiency. Specifically, SM warp occupancy increases from 22.4% to 31.1%, active threads per warp increase from 15.3 to 19.9, and warp latency decreases from 347k to 241k cycles, all without changing the sampler behavior. Applying Russian roulette (Section 6.2.4) further improves these metrics to 34.9%, 20.6, and 82k cycles, respectively. 

Under the dashed line we show the added cost brought by our techniques from Section 4, 5, and 6. _New thresholds_ reflects the cost of our scene-independent reconnection criteria that improves shift mapping quality. The _all improvements_ row applies all other techniques (decorrelation, color noise and disocclusion noise reduction). These increase average cost by 19% versus our fastest version. But even with these quality improvements, we are still 2.30× faster than Lin et al. [2022]. Our performance also scales better with resolution, as reported in the supplemental document. 

Proc. ACM Comput. Graph. Interact. Tech., Vol. 9, No. 1, Article 13. Publication date: May 2026. 

ReSTIR PT Enhanced: Algorithmic Advances for Faster and More Robust ReSTIR Path Tracing 

13:13 



<!-- Start of picture text -->
13D symposium<br><!-- End of picture text -->



<!-- Start of picture text -->
8 segs<br><!-- End of picture text -->



<!-- Start of picture text -->
13D symposium<br><!-- End of picture text -->



<!-- Start of picture text -->
(a)  Original image (b)  Swap once (c)  Swap twice<br><!-- End of picture text -->

**Fig. 6.** _Our pairing texture enables an invertible stochastic Gaussian “blur.” Swapping twice recovers the original image. Here we use a pairing texture with 𝜎_ = 16 _._ 0 _._ 





<!-- Start of picture text -->
 |<br><!-- End of picture text -->



<!-- Start of picture text -->
(a)  Pairing texture (b)  Histogram<br><!-- End of picture text -->

**Fig. 7.** _Visualizing 𝜎_ = 16 _._ 0 _, which has the same average sample distance as 𝑅_ = 30 _uniform disk sampling._ 



<!-- Start of picture text -->
𝜎 = 2 . 7 𝜎 = 5 . 3 𝜎 = 10 . 6 𝜎 = 16 . 0 Reference<br>PairedPairedPaired SpatialSpatialSpatial ReuseReuseReuse<br>((( 𝜎𝜎𝜎 === 161616 ... 00)0))<br>Time:/FLIP: 3.72 ms / 0.313 3.99 ms / 0.289 3.77 ms / 0.283 3.79 ms / 0.287<br>𝑅 = 5 𝑅 = 10 𝑅 = 20 𝑅 = 30 Reference<br>((( 𝑅𝑅𝑅 === 30)30)30)<br>RandomRandomRandom SpatialSpatialSpatial ReuseReuseReuse Spatial Reuse Time:/FLIP: 6.24 ms / 0.329 6.13 ms / 0.294 6.24 ms / 0.285 6.21 ms / 0.289<br><!-- End of picture text -->

**Fig. 8.** _Comparing our paired spatial reuse (top) with traditional random spatial reuse from a uniform disk (bottom) in the Opera House. In each column, we choose a 𝜎 value that produces the same average sample distance as a uniformly-sampled disk of radius 𝑅._ 

_Storage._ Our method also reduces storage relative to the baseline through two changes: compressing the ReSTIR PT reservoir and unifying the reservoirs for direct and indirect lighting. Because each ReSTIR pass requires two sets of reservoirs to support temporal reuse, these changes reduce per-pixel storage from 2 × (88 + 16) bytes in the baseline implementation (which uses 16-byte reservoirs for ReSTIR DI) to 2 × 64 bytes. With a 1920×1080 render resolution, this lowers memory consumption from 431 MB to 265 MB. 

## **7.2 Ablation Studies** 

We study our paired spatial reuse (Section 3), new reconnection criteria (Section 4), and decorrelation technique (Section 5) more closely here. Isolated studies of techniques in Section 6 are included in the supplemental document. 

_Evaluation of paired spatial reuse._ We encode pairing textures as two-channel tilable images, storing integer offsets (Δ _𝑥,_ Δ _𝑦_ ) ∈[−127 _,_ 127]<sup>2</sup> per pixel (wrapped by tiling). Figure 7a visualizes a 254 × 254 pairing texture. In Figure 6 we validate our texture by swapping each pixel with its paired neighbor; one swap gives a blur with Gaussian noise while a second recovers the original image. 

Figure 8 compares our paired spatial reuse against traditional uniform disk samples with disk radii _𝑅_ = 5 _,_ 10 _,_ 20 _,_ 30, matched to Gaussian kernels with equal average sample distances. Paired reuse speeds ReSTIR’s spatial reuse by 1 _._ 63× on average by halving shift mapping costs with only a modest overhead. It also lowers FLIP error, likely because a Gaussian distribution concentrates samples closer to the center (i.e., Figure 7b), increasing the probability to select compatible neighbors. 

Proc. ACM Comput. Graph. Interact. Tech., Vol. 9, No. 1, Article 13. Publication date: May 2026. 

Daqi Lin, Markus Kettunen, and Chris Wyman 

13:14 



<!-- Start of picture text -->
Reference Ours  𝑐 = 0 . 5 𝑐 = 0 . 1 𝑐 = 0 . 02<br>OurOurOur decorrelationdecorrelationdecorrelation methodmethodmethod<br>((( 𝑐𝑐𝑐 === 000 ... 11)1))<br>0.372 0.405 0.451<br>Unfiltered RTXDI  𝑠 = 0 . 05 𝑠 = 0 . 2 𝑠 = 0 . 8<br>RTXDIRTXDIRTXDI((( 𝑠𝑠𝑠 boilingboilingboiling=== 000 ... 22)2)) ffflterlter ii i lter FLIP: 0.352 0.402 0.499 0.576<br><!-- End of picture text -->

**Fig. 9.** _Comparing our duplication-based decorrelation (top) and RTXDI’s boiling filter (bottom) in the Tower Bridge scene, which exhibits strong correlations (see_ unfiltered _inset) due to poor initial sample quality. The parameters reflect weak, medium (default), and strong correlation reduction in both methods. Increasing FLIP errors reflect the darkening bias the methods introduce._ 

_Evaluation of the new reconnection criteria._ We comprehensively evaluated our new reconnection criteria. For the single-vertex roughness threshold, we use ReSTIR PT’s default _𝛼_ = 0 _._ 2. For dual footprint thresholds, we empirically found _𝑐_ = 0 _._ 02 (see Equation 5) to be near-optimal across a wide range of scenes. Ablation results for different _𝑐_ values are provided in the supplemental. 

Figures 10, 11, and 12 compare our criteria with Lin et al.’s [2022] reconnection rules, which use a world-space distance threshold and a two-vertex roughness test. We set the distance threshold to 2% of the shortest scene dimension, typical to prior work. Across variations in viewing distance, material roughness, and lighting conditions, our thresholds consistently deliver higher-quality resampling with improved robustness. 

Our ray footprint thresholds resemble the path footprint criteria of Bekaert et al. [2003] and Müller et al. [2021]. However, we found path footprints alone often fail to eliminate unconnectable paths because they do not consider material properties at the reconnection vertex. Additional comparisons are included in our supplemental. 

_Ablation study of duplication-based decorrelation._ In Figure 9, we compare our duplication-based correlation reduction method to the boiling filter in RTXDI SDK [NVIDIA 2021]. This filter computes an average resampling weight _𝑤_ ¯ over each warp’s pixels and clears any reservoir with weight _𝑤 𝑗 > 𝑎𝑤_ ¯ where _𝑎_ = −9 + 10/ _𝑠_ is controlled by _strength factor 𝑠_ ∈(0 _,_ 1]. Note that RTXDI’s boiling filter causes substantial energy loss to remove most artifacts. 

## **7.3 Overall quality evaluation** 

Figure 1 and Figure 13 show the overall performance and quality improvement with our enhanced ReSTIR PT. Compared to Lin et al. [2022], our enhanced ReSTIR PT reduces correlation artifacts, color and disocclusion noise. It exhibits better quality thanks to our new reconnection criteria (e.g., see the surfaces inside the glass in Figure 1) and the unification of direct and indirect lighting, yet runs significantly faster, reaching 2.08×-3.05× speedup over Lin et al. [2022]. 

## **7.4 Bias and Convergence** 

Our decorrelation technique (Section 5) introduces bias, but disabling it restores unbiasedness while retaining all remaining benefits of ReSTIR PT Enhanced (see Figure 14 and additional comparisons at the end of the supplemental document). In practice, this bias is usually small relative to the noise level. As shown in Figure 15, the biased and unbiased variants of our method have nearly overlapping convergence curves under both MSE and FLIP over the time range relevant to real-time 

Proc. ACM Comput. Graph. Interact. Tech., Vol. 9, No. 1, Article 13. Publication date: May 2026. 

ReSTIR PT Enhanced: Algorithmic Advances for Faster and More Robust ReSTIR Path Tracing 

13:15 











<!-- Start of picture text -->
(a)  Near view (reference) (b)  Baseline (FLIP: 0.223) (c)  Ours (FLIP: 0.223) (d)  Inset Reference<br>(e)  Far view (reference) (f)  Baseline (FLIP: 0.296) (g)  Ours (FLIP: 0.291) (h)  Inset Reference<br><!-- End of picture text -->

**Fig. 10.** _Our new reconnection thresholds adapt to varying viewing distances, improving consistency in the Kitchen scene. In the baseline [Lin et al. 2022], thresholds good for the near view behave poorly for the far view._ 









<!-- Start of picture text -->
(a)  Lin et al. [2022] (FLIP: 0.339) (b)  Our thresholds (FLIP: 0.322) (c)  Reference<br><!-- End of picture text -->

**Fig. 11.** _A failure case shown by Lin et al. [2022] in San Miguel. Previously, the hybrid shift sampled caustics poorly (pointed by the arrow) from near-delta distant highlights (the lamp reflection on the far window). Our new threshold effectively adjusts “roughness” by distance, here allowing reconnection to the window._ 









<!-- Start of picture text -->
(a)  Lin et al. [2022] (FLIP: 0.185) (b)  Our thresholds (FLIP: 0.185) (c)  Reference<br>(d)  Lin et al. [2022] (FLIP: 0.271) (e)  Our thresholds (FLIP: 0.252) (f)  Reference<br><!-- End of picture text -->

**Fig. 12.** _Lin et al.’s [2022] reconnections perform similarly to our work in diffuse scenes like Veach Ajar. With metallic walls (roughness of 0.3), their baseline shows significantly more noise. Our new thresholds automatically enlarge reconnection distances, giving more consistent noise._ 

Proc. ACM Comput. Graph. Interact. Tech., Vol. 9, No. 1, Article 13. Publication date: May 2026. 

13:16 



<!-- Start of picture text -->
Daqi Lin, Markus Kettunen, and Chris Wyman<br><!-- End of picture text -->



<!-- Start of picture text -->
Watercolor Org. Ref. Ours<br>Bae<br>i al<br>ad td<br>——<br>Original ReSTIR PT (30.1 ms) FLIP: 0.171 ReSTIR PT Enhanced (14.5 ms)  FLIP: 0.166<br>Zero Day Org. Ref. Ours<br>er ey ee<br>wl ~n= a a w naa=<br>Ip itil al oe he x<br>= & ae<br>—* a<br>Original ReSTIR PT (47.1 ms) FLIP: 0.439 ReSTIR PT Enhanced (21.4 ms)  FLIP: 0.414<br>Crown Org. Ref. Ours<br>‘ 4 ae,eo a aea Liatyo os 2ie<br>7 - & j 7 ze ps ‘<br>fT) ene: — | |<br>Original ReSTIR PT (50.0 ms) FLIP: 0.483 ReSTIR PT Enhanced (16.4 ms)  FLIP: 0.444<br><!-- End of picture text -->

**Fig. 13.** _Comparing Lin et al. [2022] to our enhancements in the Watercolor, Zero Day, and Crown scenes. We sample glossy surfaces noticeably better, thanks to improved MIS on primary hits and better reconnection thresholds. We also reduce color and disocclusion noise with fewer correlation artifacts._ 

rendering (< 0.1 s). Our default biased version even achieves lower MSE initially as it prevents the spread of outliers. With longer accumulation, however, its convergence curve gradually flattens as the bias emerges. In more typical scenes such as Watercolor, where correlation is less severe than in Kitchen (Section 5), the convergence curve flattens more slowly with an overall lower bias level. Overall, we find the default decorrelated, biased variant more preferable for real-time image quality. When a noise-free unbiased image is desired, accumulating it with temporal reuse disabled is typically more efficient [Lin et al. 2022]. 

## **8 CONCLUSION AND FUTURE WORK** 

We presented a suite of new ideas to enhance ReSTIR PT, improving both performance and robustness. Optimizations range from low-level GPU code improvements to larger algorithmic changes, showing up to a 3× speedup. We improve robustness via better shift mapping, correlation reduction, and reductions in color and disocclusion noise, providing consistently higher visual quality. 

Proc. ACM Comput. Graph. Interact. Tech., Vol. 9, No. 1, Article 13. Publication date: May 2026. 

ReSTIR PT Enhanced: Algorithmic Advances for Faster and More Robust ReSTIR Path Tracing 

13:17 











<!-- Start of picture text -->
(a)  Ours (no decorrelation) (b)  ((a)-ref.)/ref. (c)  avg. of 1024 runs of (a) (d)  ((c)-ref.)/ref.<br>(e)  Ours (f)  ((e)-ref.)/ref. (g)  avg. of 1024 runs of (e) (h)  ((g)-ref.)/ref.<br><!-- End of picture text -->

**Fig. 14.** _Our method without decorrelation (a) exhibits more correlation artifacts in Watercolor under an upward-moving camera, including noise clusters on the lamp and correlated dark and bright trails in the disocclusion region. After averaging 1024 independent runs, our default version reveals systematic error (due to bias) as can be seen in (h), shown using the same color scale as in Figure 5._ 



<!-- Start of picture text -->
Watercolor Kitchen Watercolor Kitchen<br>10 0<br>BaselineOurs (no decorr.) 10 3 Baseline Ours (no decorr.) BaselineOurs (no decorr.) Baseline Ours (no decorr.)<br>10 1 Ours Ours 10 1 Ours Ours<br>10 4<br>10 1<br>10 2<br>10 5<br>10 3 10 6<br>10 0 10 1 10 2 10 3 10 0 10 1 10 2 10 3 10 2 10 1 10 0 10 1 10 2 10 1 10 0 10 1<br>Iteration Iteration Cumulative Time (s) Cumulative Time (s)<br>(a)  MSE vs. iteration count for two scenes (b)  FLIP vs. cumulative time for two scenes<br>MSE MSE FLIP FLIP<br><!-- End of picture text -->

**Fig. 15.** _Convergence plots for Watercolor (Figure 13, 14) and Kitchen (Figure 5), comparing baseline ReSTIR PT, our method without decorrelation (Ours no decorr.), and our default method with decorrelation (Ours). Our default method exhibits more bias in Kitchen due to heavier correlation reduction. For each scene and method, we run the same camera animation 1024 times with different random seeds for sampling, capture the same view during camera motion, and average the results across runs. (a) shows mean squared error versus iteration count. (b) remaps iteration count of the data points in (a) to cumulative rendering time of the captured frames and reports FLIP error. FLIP is less sensitive to fireflies and correlation artifacts, but still sensitive to regional brightness differences._ 

Several contributions generalize to other reuse algorithms: paired spatial reuse, footprint-based reconnection thresholds, and temporal history control via duplication maps. We further found that a unified ReSTIR, sampling direct and indirect lighting together, improves both speed and quality. 

Despite these advances, ReSTIR PT faces open challenges motivating future work. High-frequency features remain sensitive to temporal noise during motion, and overall quality is bounded by the effectiveness of initial sampling, especially for low-probability light transport effects such as caustics. Some issues are partially addressed by recent work such as Area ReSTIR [Zhang et al. 2024], suggesting exploring smooth transitions from ReSTIR PT Enhanced to an enhanced Area ReSTIR within a shared framework. Additional opportunities also remain for further algorithmic and hardware-aware optimizations on newer GPU architectures. 

Proc. ACM Comput. Graph. Interact. Tech., Vol. 9, No. 1, Article 13. Publication date: May 2026. 

Daqi Lin, Markus Kettunen, and Chris Wyman 

13:18 

## **ACKNOWLEDGMENTS** 

We thank Miika Aittala for useful discussions on pairwise reuse textures, Aaron Lefohn for general support, and the anonymous reviewers for their constructive feedback. 

## **REFERENCES** 

Pontus Andersson, Jim Nilsson, Peter Shirley, and Tomas Akenine-Möller. 2021. Visualizing errors in rendered high dynamic range images. In _Eurographics-Short Papers_ . Eurographics-European Association for Computer Graphics, 25–28. Pablo Bauszat, Victor Petitjean, and Elmar Eisemann. 2017. Gradient-domain path reusing. _ACM Transactions on Graphics (TOG)_ 36, 6 (2017), 1–9. 

Philippe Bekaert, Mateu Sbert, and John H Halton. 2002. Accelerating Path Tracing by Re-Using Paths. _Rendering Techniques_ 2, 2002 (2002), 125–134. 

Philippe Bekaert, Philipp Slusallek, Ronald Cools, Vlastimil Havran, and Hans-Peter Seidel. 2003. A custom designed density estimation method for light transport. (2003). 

Nir Benty, Kai-Hwa Yao, Petrik Clarberg, Lucy Chen, Simon Kallweit, Tim Foley, Matthew Oakes, Conor Lavelle, and Chris Wyman. 2020. The Falcor Rendering Framework. https://github.com/NVIDIAGameWorks/Falcor 

Benedikt Bitterli, Chris Wyman, Matt Pharr, Peter Shirley, Aaron Lefohn, and Wojciech Jarosz. 2020. Spatiotemporal reservoir resampling for real-time ray tracing with dynamic direct lighting. _ACM Transactions on Graphics (Proceedings of SIGGRAPH)_ 39, 4 (July 2020). 

Min-Te Chao. 1982. A general purpose unequal probability sampling plan. _Biometrika_ 69, 3 (1982), 653–656. 

Henrik Halen, Andreas Brinck, Kyle Hayward, and Xiangshun Bei. 2021. Global Illumination Based on Surfels. Talk at 

ACM SIGGRAPH 2021, Advances in Real-Time Rendering in Games. https://www.ea.com/seed/news/siggraph21-globalillumination-surfels 

Eric Heitz. 2018. Sampling the ggx distribution of visible normals. _Journal of Computer Graphics Techniques (JCGT)_ 7, 4 (2018), 1–13. 

Markus Kettunen, Daqi Lin, Ravi Ramamoorthi, Thomas Bashford-Rogers, and Chris Wyman. 2023. Conditional resampled importance sampling and ReSTIR. In _SIGGRAPH Asia 2023 Conference Papers_ . 1–11. 

Markus Kettunen, Marco Manzi, Miika Aittala, Jaakko Lehtinen, Frédo Durand, and Matthias Zwicker. 2015. Gradient-domain path tracing. _ACM Transactions on Graphics (TOG)_ 34, 4 (2015), 1–13. 

Daqi Lin, Markus Kettunen, Benedikt Bitterli, Jacopo Pantaleoni, Cem Yuksel, and Chris Wyman. 2022. Generalized Resampled Importance Sampling: Foundations of ReSTIR. 41, 4 (2022), 75:1–75:23. https://doi.org/10.1145/3528223.3530158 Zander Majercik, Jean-Philippe Guertin, Derek Nowrouzezahrai, and Morgan McGuire. 2019. Dynamic Diffuse Global 

Illumination with Ray-Traced Irradiance Fields. _Journal of Computer Graphics Techniques (JCGT)_ 8, 2 (5 June 2019), 1–30. Thomas Müller, Fabrice Rousselle, Jan Novák, and Alexander Keller. 2021. Real-time neural radiance caching for path tracing. _arXiv preprint arXiv:2106.12372_ (2021). 

NVIDIA. 2020. NVIDIA Real-Time Denoiser Delivers Best-in-Class Denoising in Ubisoft’s Watch Dogs Legion. https: 

//developer.nvidia.com/blog/nvidia-real-time-denoiser-delivers-best-in-class-denoising-in-watch-dogs-legion/ 

NVIDIA. 2021. RTXDI SDK – Light Resampling In Practice. https://developer.nvidia.com/blog/new-video-light-resamplingin-practice-with-rtxdi/. Accessed: 2025-11-29. 

NVIDIA. 2025. DLSS 4: Transforming Real-Time Graphics with AI. https://research.nvidia.com/labs/adlr/DLSS4/. Technical Report. 

Yaobin Ouyang, Shiqiu Liu, Markus Kettunen, Matt Pharr, and Jacopo Pantaleoni. 2021. ReSTIR GI: Path Resampling for Real-Time Path Tracing. _Computer Graphics Forum_ (2021). 

- Rohan Sawhney, Daqi Lin, Markus Kettunen, Benedikt Bitterli, Ravi Ramamoorthi, Chris Wyman, and Matt Pharr. 2024. Decorrelating restir samplers via mcmc mutations. _ACM Transactions on Graphics_ 43, 1 (2024), 1–15. 

Christoph Schied, Anton Kaplanyan, Chris Wyman, Anjul Patney, Chakravarty R Alla Chaitanya, John Burgess, Shiqiu Liu, Carsten Dachsbacher, Aaron Lefohn, and Marco Salvi. 2017. Spatiotemporal variance-guided filtering: real-time reconstruction for path-traced global illumination. In _Proceedings of High Performance Graphics_ . 1–12. 

Justin Talbot, David Cline, and Parris Egbert. 2005. Importance Resampling for Global Illumination. In _Eurographics Symposium on Rendering (2005)_ , Kavita Bala and Philip Dutre (Eds.). The Eurographics Association. 

Daniel Wright, Krzysztof Narkowicz, and Patrick Kelly. 2022. Lumen: Real-time Global Illumination in Unreal Engine 5. Talk at ACM SIGGRAPH 2022, Advances in Real-Time Rendering in Games. https://advances.realtimerendering.com/ s2022/SIGGRAPH2022-Advances-Lumen-Wright%20et%20al.pdf 

Chris Wyman, Markus Kettunen, Daqi Lin, Benedikt Bitterli, Cem Yuksel, Wojciech Jarosz, and Pawel Kozlowski. 2023. A gentle introduction to restir path reuse in real-time. In _ACM SIGGRAPH 2023 Courses_ . 1–38. 

Chris Wyman and Alexey Panteleev. 2021. Rearchitecting Spatiotemporal Resampling for Production. In _High-Performance Graphics - Symposium Papers_ . 

Proc. ACM Comput. Graph. Interact. Tech., Vol. 9, No. 1, Article 13. Publication date: May 2026. 

- ReSTIR PT Enhanced: Algorithmic Advances for Faster and More Robust ReSTIR Path Tracing 13:19 Zheng Zeng, Shiqiu Liu, Jinglei Yang, Lu Wang, and Ling-Qi Yan. 2021. Temporally Reliable Motion Vectors for Real-time Ray Tracing. In _Computer Graphics Forum_ , Vol. 40. Wiley Online Library, 79–90. 

- Song Zhang, Daqi Lin, Markus Kettunen, Cem Yuksel, and Chris Wyman. 2024. Area ReSTIR: Resampling for Real-Time Defocus and Antialiasing. _ACM Transactions on Graphics (TOG)_ 43, 4 (2024), 1–13. 

Proc. ACM Comput. Graph. Interact. Tech., Vol. 9, No. 1, Article 13. Publication date: May 2026. 

