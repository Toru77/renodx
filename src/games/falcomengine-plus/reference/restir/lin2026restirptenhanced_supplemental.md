# **ReSTIR PT Enhanced: Algorithmic Advances for Faster and More Robust ReSTIR Path Tracing – Supplemental Document** 

DAQI LIN, NVIDIA, USA MARKUS KETTUNEN, NVIDIA, Finland CHRIS WYMAN, NVIDIA, USA 

CCS Concepts: • **Computing methodologies** → **Ray tracing** . 

Additional Key Words and Phrases: Optimization, Sampling, Light Transport, Real Time 

### **ACM Reference Format:** 

Daqi Lin, Markus Kettunen, and Chris Wyman. 2026. ReSTIR PT Enhanced: Algorithmic Advances for Faster and More Robust ReSTIR Path Tracing – Supplemental Document. _Proc. ACM Comput. Graph. Interact. Tech._ 9, 1 (May 2026), 13 pages. https://doi.org/10.1145/3804494 

## **1 LOBE-INDEXED PATHS AND PRIMARY SAMPLE SPACE** 

Modern path tracers often evaluate only a single sampled lobe for each path vertex, effectively splitting the integrand associated with the same geometric path across all lobe combinations. In ReSTIR PT, this separation allows more effective path reuse, as instead of classifying the _whole vertex_ as either rough or (near-)specular, leading to suboptimal reuse decisions, only (near-)specular _lobes_ are postponed with random replay. 

In addition to sampling a single BSDF lobe to extend the path, each vertex also performs next event estimation (NEE) with all lobes enabled. Therefore, the path space integral can be written as 



where each path x¯ now contains a lobe sequence _ℓ_<sup>¯</sup> = ( _ℓ𝑗_ )<sup>_𝑑_</sup> _𝑗_ =<sup>−</sup> 1<sup>1with</sup><sup>_ℓ𝑗_∈{0</sup><sup>_,_1</sup><sup>_,_2</sup><sup>_, ..., 𝑁_lobe} defining</sup> the lobe used at vertex x _𝑗_ , with a NEE sample (at the last vertex x _𝑑_ ) encoded by index 0. _𝑁_ lobe is the maximum number of BSDF lobes in a vertex, _𝐿𝑑_ is the set of all possible lobe sequences, and _𝑛_ ( _ℓ_<sup>¯</sup> ) ∈{1 _,_ 2} differentiates between BSDF and NEE sampling (based on _ℓ𝑑_ −1) for MIS weights like _<u>𝑝𝑡</u>_ <u>(x¯ )</u> _𝜔𝑡_ (x¯) = _𝑝_ 1 (x¯ )+ _𝑝_ 2 (x¯ )<sup>. Written out explicitly, the lobe-specific path contribution is</sup> 



where _𝜌ℓ𝑗_ is the chosen BSDF lobe _ℓ𝑗_ at x _𝑗_ for vertices _𝑗_ ≥ 1, and the sensor response _𝑊_ e(x1 → x0) for the sensor vertex _𝑗_ = 0. 

In principle, this allows a unique mapping to primary sample spaces (PSS) can then be established, turning the (x¯ _, ℓ_<sup>¯</sup> ) pair into a random number sequence ¯u ∈ 𝒰 _𝑡,𝑑_ where _𝑑_ (path length) and _𝑡_ ∈{1 _,_ 2} 

Authors’ addresses: Daqi Lin, daqil@nvidia.com, NVIDIA, USA; Markus Kettunen, mkettunen@nvidia.com, NVIDIA, Finland; Chris Wyman, chris.wyman@acm.org, NVIDIA, USA. 

Please use nonacm option or ACM Engage class to enable CC licenses This work is licensed under a Creative Commons Attribution 4.0 International License. 

> © 2026 Copyright held by the owner/author(s). ACM 2577-6193/2026/5-ART https://doi.org/10.1145/3804494 

Proc. ACM Comput. Graph. Interact. Tech., Vol. 9, No. 1, Article . Publication date: May 2026. 

Daqi Lin, Markus Kettunen, and Chris Wyman 

2 

(BSDF or NEE light sampling) specifies the dimensionality of the primary sample space, giving the integral 



_𝜔𝑛_ <u>( ¯</u> _ℓ_ <u>) (𝒳</u> _𝑡,𝑑_ (u¯ )) _<u>𝑓ℓ</u>_ ¯(𝒳 _𝑡,𝑑_ (u¯ )) where _𝐹_ (u¯) = _𝑝𝑡_ (𝒳 _𝑡,𝑑_ (u¯ )) is the PSS integrand and 𝒳 _𝑡,𝑑_ is a function that warps the random sequence into the lobe-indexed path. Note that _𝑝𝑡_ (𝒳 _𝑡,𝑑_ (u¯)) is the path-space sampling PDF in the path tracer that accounts for the change of variables between the path space and the PSS. The PSS source PDF _𝑝_ (u¯) = 1 for paths directly produced by the path tracer. 

With PSS parameterization, the Jacobian determinant for the hybrid shift can be expressed as 



where _𝑝𝑘_<sup>_𝑥_(</sup><sup>_𝜔𝑘, ℓ𝑘_)≡</sup><sup>_𝑝_(</sup><sup>_𝜔𝑘, ℓ𝑘_|x</sup><sup>_𝑘,_−</sup><sup>_𝜔𝑘_−1)is the joint PDF of sampling the direction</sup><sup>_𝜔𝑘_=</sup> x<sup>�</sup> _𝑘_ x _𝑘_ +1 = −−−−−→x _𝑘_ x _𝑘_ +1/∥<sup>−−−−−→</sup> x _𝑘_ x _𝑘_ +1∥ and lobe _ℓ𝑘_ at x _𝑘_ with outgoing direction − _𝜔𝑘_ −1. Other terms like _𝑝𝑘_<sup>_𝑥_</sup> −1<sup>(</sup><sup>_𝜔𝑘_−1</sup><sup>_, ℓ𝑘_−1)</sup> are similarly defined. _𝜔𝑘_<sup>′</sup> −1<sup>is a shorthand for</sup> y<sup>�</sup> _𝑘_ −1x _𝑘_ , the direction formed by reconnection. The single-sided geometry term _𝐺_ (x → y) is defined as cos _𝜃_ /||x − y||<sup>2</sup> where _𝜃_ is the angle made by y� _𝑘_ −1x _𝑘_ and the normal at y _𝑘_ −1. Note that the Jacobian is simply the result of reconnection. Random replay is an identity shift in PSS, therefore having a Jacobian of 1. 

## **2 DETAILED DERIVATION OF THE FOOTPRINT THRESHOLDS** 

The dual footprint threshold is 



where _𝑝𝑘_<sup>_𝑥_</sup> −1<sup>(</sup><sup>_𝜔𝑘_−1)and</sup><sup>_𝑝_</sup> _𝑘_<sup>_𝑥_(</sup><sup>_𝜔𝑘_)are PDFs of sampling</sup><sup>_𝜔𝑘_−1and</sup><sup>_𝜔𝑘_at x</sup><sup>_𝑘_−1and x</sup><sup>_𝑘_, respectively. The</sup> analysis assumes samples without lobe indices, with marginal PDF for direction sampling available. The lobe-indexed case is discussed in Section 3. We consider shifting a path x¯ starting from pixel A to a path y¯ in a neighboring pixel B. For any pair of corresponding primary hits x1 and y1, we assume their world-space distance is bounded by the primary ray footprint radius _𝑅_ pri<sup>x¯(“spread at the primary vertex” [Müller et al. 2021]) up to a</sup> constant factor, 



and similarly |x1 − y1| ≤ _𝐶_ 1 _𝑅_ pri<sup>y¯.Beforereconnection,wefurtherassumethatrandomreplay</sup> produces corresponding vertices x _𝑖_ and y _𝑖_ on the same (locally flat) surface and that the distances between them grow at most linearly: 



These bounds reflect that neighboring pixels cannot diverge arbitrarily in a finite scene. 

For a potential reconnection vertex x _𝑘_ on x¯, connecting from y _𝑘_ −1 instead of x _𝑘_ −1 perturbs the Jacobian of the shift _𝑇_ . A sufficient condition for a stable mapping is that the relative change in the area density at x _𝑘_ and the relative change in the BSDF sampling density at the next vertex are both 

Proc. ACM Comput. Graph. Interact. Tech., Vol. 9, No. 1, Article . Publication date: May 2026. 

ReSTIR PT Enhanced – Supplemental Document 

3 

small. We express this as 



for a small _𝜖_ , which ensures (1 − _𝜖_ )<sup>2</sup> _<_ �� _𝜕𝑇_ / _𝜕_ u¯ _𝑥_ �� _<_ (1 + _𝜖_ )2. Our goal is to turn these conditions into simple, local thresholds based on ray footprints and inverse ray footprints. 

_Ray footprint threshold._ The term _𝑝𝑘_<sup>_𝑥_</sup> −1<sup>(</sup><sup>_𝜔𝑘_−1)</sup><sup>_𝐺_(x</sup><sup>_𝑘_−1↔x</sup><sup>_𝑘_)is the area density of the vertex x</sup><sup>_𝑘_</sup> when the path x¯ is generated by path tracing. We denote it by 



and refer to its inverse 1/ _𝑝𝑘_<sup>x¯(x</sup><sup>_𝑘_) as the</sup><sup>_ray footprint_at x</sup><sup>_𝑘_. Intuitively, this footprint measures the</sup> typical spacing between nearby area samples. Analogously, tracing from y _𝑘_ −1 induces 



and the left-hand side of (8) becomes 



Directly comparing _𝑝𝑘_<sup>x¯and</sup><sup>_𝑝_</sup> _𝑘_<sup>y¯isawkward.Instead,wemaketheempiricalassumptionthat</sup> random replay preserves the area density along y¯, i.e., 



where _𝑇_ replay denotes continuing random replay from y _𝑘_ −1 to produce a vertex near x _𝑘_ . Under this approximation, (8) is controlled by the variation of a single density function _𝑝𝑘_<sup>y¯(</sup><sup>_𝑥_) under a small</sup> spatial shift: 



We now assume a footprint-based smoothness condition on the area density: for any area sample _𝑥_ with density _<u>𝑝</u>_ area <u>(</u> _𝑥_ ), the relative change of _𝑝_ area inside a disk of radius proportional to the footprint √︁1/ _𝑝_ area( _𝑥_ ) is bounded by _𝜖_ . Formally, there exists a constant _𝑐_ 2 such that if _𝑦,𝑧_ satisfy 



then 



In words, the density is approximately constant within a footprint-sized neighborhood. 

Since (according to Equation 7) the distance between x _𝑘_ and _𝑇_ replay(x _𝑘_ ) can be bounded by the primary footprint, 



Proc. ACM Comput. Graph. Interact. Tech., Vol. 9, No. 1, Article . Publication date: May 2026. 

Daqi Lin, Markus Kettunen, and Chris Wyman 

4 

Combining this geometric bound, the smoothness assumption (Equation 15, 16), and all the derivations above, a sufficient condition for (8) is 



By shift invertibility, the same condition must hold for x¯, yielding 



In practice, we collapse the constants into a single user parameter _𝑐_ and obtain the _ray footprint threshold_ , which corresponds to the first term in Equation 5. Increasing _𝑐_ effectively shrinks the admissible footprint and enforces a more conservative notion of local smoothness, making reconnections rarer but more robust. 

_Inverse ray footprint threshold._ The ray footprint threshold controls the change in area density at x _𝑘_ , but it does not account for how reconnection affects the BSDF sampling density _𝑝𝑘_<sup>_𝑥_(</sup><sup>_𝜔𝑘_)at the</sup> next vertex. This effect depends only on the change in outgoing direction − _𝜔𝑘_ −1, and can be severe for low-roughness glossy materials where the BSDF PDF is sharply peaked. 

We therefore introduce the _inverse ray footprint_ , 



which we interpret as the footprint of a reverse-traced ray from x _𝑘_ towards x _𝑘_ −1. Assuming approximate reciprocity of the BSDF sampling PDFs for glossy materials, 



this inverse footprint is proportional to the area density _𝑝_ (x _𝑘_ −1|x _𝑘,𝜔𝑘_ ) of the hit point of a reverse ray, via 



Reconnecting from y _𝑘_ −1 instead of x _𝑘_ −1 perturbs this density, and we can reuse the same footprintsmoothness argument as above: if 



then 



Again using the geometric bound and assuming that the geometry term _𝐺_ (x _𝑘_ → x _𝑘_ −1) ≈ _𝐺_ (x _𝑘_ → y _𝑘_ −1) for distant reconnections, we arrive at a condition analogous to the ray footprint case: 



## **3 CONDITIONAL VS. MARGINAL PDF IN FOOTPRINT THRESHOLD** 

To handle multi-layer material (e.g. material that contains both diffuse and specular components), ReSTIR PT uses lobe-specific connectability [Lin et al. 2022] – only the roughness of the sampled lobe is used to test against the roughness threshold. Correspondingly, the lobe index is preserved in reconnection. This yields a Jacobian determinant expressed in Equation 4. Since reconnection does not change the lobe type, it is reasonable to assume that the lobe sampling PMF remains nearly constant (as a result of shifting to a neighboring pixel that has similar material and similar viewing 

Proc. ACM Comput. Graph. Interact. Tech., Vol. 9, No. 1, Article . Publication date: May 2026. 

ReSTIR PT Enhanced – Supplemental Document 

5 







**(a)** _Marginal PDF (default)_ **(b)** _Conditional PDF_ **(c)** _Reference FLIP: 0.171 FLIP: 0.169_ 

**Fig. 1.** _Replacing the ideal conditional PDF with marginal PDF for directional sampling results in shorter reconnection distances for glossy reflection for multi-lobed material as shown in this modified Veach Ajar scene, where the reconnection distances shrinks as the diffuse component strength grows. This only results in slight quality loss. 𝑐_ = 0 _._ 02 _is used in this example._ 





**Fig. 2.** _Comparison between applying reconnection shift (left) and hybrid shift (right) on a teapot lid with a neural material that represents multiple material layers without explicit lobe or roughness information. Our hybrid shift uses the PDF-based threshold in place of the single-vertex roughness threshold to achieve a similar shift mapping effect, and provides significantly higher quality for the specular reflection._ 

direction) and focus on the change of the direction sampling PDF within the lobe when analyzing the threshold. Ideally, directional sampling PDFs _𝑝𝑘_<sup>_𝑥_</sup> −1<sup>(</sup><sup>_𝜔𝑘_−1|</sup><sup>_ℓ𝑘_−1)and</sup><sup>_𝑝_</sup> _𝑘_<sup>_𝑥_(</sup><sup>_𝜔𝑘_|</sup><sup>_ℓ𝑘_)conditioned on</sup> the lobes should be used instead of _𝑝𝑘_<sup>_𝑥_</sup> −1<sup>(</sup><sup>_𝜔𝑘_−1) and</sup><sup>_𝑝_</sup> _𝑘_<sup>_𝑥_(</sup><sup>_𝜔𝑘_), respectively.</sup> 

In practice, we simply use the marginal directional sampling PDFs (over all lobes) as we found that the quality difference is negligible. This results in the dual footprint threshold in Equation 5 we presented earlier. While this can sometimes cause premature reconnection on materials mixing diffuse and specular components (e.g. plastics), where the specular PDF tested against the threshold is reduced by the diffuse component, the effect is minor (Figure 1): a stronger reduction corresponds to a weaker specular contribution. The practical benefit is that we can reuse the same marginal PDF evaluation already needed for MIS weights, avoiding extra API complexity in the rendering system. 

## **4 PDF-BASED CONNECTIBILITY FOR BLACK-BOX MATERIAL** 

For general materials (BSDF), we do not always have a separation of lobes available, and roughness values might not be available. Still, we can modify our thresholds such that it can be used for any black-box material with only BSDF and PDF queries available. The footprint thresholds work as is with BSDF-sampling PDF substituted. To use the roughness threshold, we replace the roughness value with the reciprocal squared solid angle PDF that samples the _𝜔𝑘_ −1 direction (inspired by 

Proc. ACM Comput. Graph. Interact. Tech., Vol. 9, No. 1, Article . Publication date: May 2026. 

6 

Daqi Lin, Markus Kettunen, and Chris Wyman 



<!-- Start of picture text -->
Path Tracing + ReSTIR<br>No RR RR No RR RR Reference<br>Path Tracing PT + RR ReSTIR PT ReSTIR PT + RR<br>Time: 6.1 ms 4.8 ms 12.1 ms 10.2 ms<br>FLIP: 0.914 0.942 0.207 0.222<br><!-- End of picture text -->

**Fig. 3.** _Applying Russian roulette (RR) on path tracing yields much fewer non-zero pixels in the Veach Ajar scene. However, with spatiotemporal reuse there is only slight noise increase._ spherical gaussians): 



We find that using the same _𝛼_ min tuned for the original roughness threshold yields satisfying results. Intuitively, a large PDF value suggests a glossy reflection which is likely to cause problem for reconnection. For multi-layer black-box material, large PDFs also suggest the "specular lobe" is being sampled. An example on neural material [Zeltner et al. 2024] is shown in Figure 2. 

## **5 RIS-BASED NEE IN PRIMARY SPACE PARAMETERIZATION** 

_MIS Weights with RIS-Based NEE._ With _𝑀 >_ 1 ( _𝑀_ being the candidate count) RIS for NEE, we replace the original single-sample MIS weight in the integrand to multi-sample MIS weight, i.e. for an NEE path x¯, _𝜔_ 1(x¯) = _𝑀𝑝_ 1(x¯)/( _𝑀𝑝_ 1(x¯) + _𝑝_ 2(x¯)) where 1 and 2 represents NEE and BSDF light sampling strategy, respectively. _𝜔_ 2( _𝑥_ ) is defined similarly. Note that the actual NEE PDF is unknown due to the RIS process and we use the _𝑀_ -sample MIS formulation with the light sampling source PDF _𝑝_ 1 assuming the RIS can approximate the quality of fully evaluating all _𝑀_ lights in NEE. 

_PSS Implication._ We treat the RIS process as a black box and stick to our original primary sample space definition which assumes only one NEE sample is produced with a known PDF. Note that the actual sampling process does not have to be the same as the sampling process the PSS assumes. Therefore, the integrand (without lobe indexing) is still _𝜔_ 1(x¯) _𝑓_ (x¯)/ _𝑝_ 1(x¯) for an NEE light sample, just with a modified MIS weight _𝜔_ 1 compared to the single sample NEE. The effect of the RIS is incorporated into the UCW. Instead of 1 (assuming no Russian roulette), the UCW of the selected NEE sample becomes _𝑊𝑈_ = _𝑊_ 𝒳<sup>RIS</sup> ( _𝑈_ )<sup>_𝑝_1(𝒳(</sup><sup>_𝑈_)) where</sup><sup>_𝑈_is the hypothetical random numbers that</sup> could have generate the same light sample in the original single-sample NEE sampling procedure and _𝑊_ 𝒳<sup>RIS</sup> ( _𝑈_ )<sup>is the path-space UCW produced by the RIS process, assuming area measure like in ReSTIR</sup> DI is used. The _𝑝_ 1(𝒳( _𝑈_ )) term is also the Jacobian that converts between the parameterizations and we can verify that the original NEE sampling has _𝑊_ 𝒳 ( _𝑈_ ) = 1/ _𝑝_ 1(𝒳( _𝑈_ )) which cancels the Jacobian to yield 1. 

## **6 RUSSIAN ROULETTE IMPLEMENTATION DETAILS** 

The initial resampling can often be the most expensive stage in ReSTIR PT due to the code and data divergence in a path tracer. To reduce thread divergence in initial resampling, we apply Russian roulette to reduce the average path length and also lower the chance of excessively long paths. Usually, the primary sample space (PSS) is extended with random numbers for Russian roulette, which can create caveats for shift mapping: naively replaying the random numbers to perform Russian roulette tests in hybrid shift can increase shift failure, as a survived path can sometimes be shifted to a killed path, invalidating the sample. 

Proc. ACM Comput. Graph. Interact. Tech., Vol. 9, No. 1, Article . Publication date: May 2026. 

ReSTIR PT Enhanced – Supplemental Document 

7 



<!-- Start of picture text -->
mo<br><!-- End of picture text -->



<!-- Start of picture text -->
um<br><!-- End of picture text -->



<!-- Start of picture text -->
CJ<br><!-- End of picture text -->



<!-- Start of picture text -->
ReSTIR PT (with ReSTIR DI) ReSTIR PT (Unified DI+GI)<br>pen em” ae) _<br>Time: 16.0 ms Time: 13.4 ms Reference<br>FLIP: 0.390 FLIP: 0.332<br><!-- End of picture text -->

**Fig. 4.** _By including direct illumination in sampling and disabling ReSTIR DI [Bitterli et al. 2020], the Unified version significantly improves the DI sampling, especially for specular reflection, thanks to hybrid shift and pairwise MIS. Using light tiles for NEE also improved lighting quality of subsequent bounces (reflection of the chair in the second inset). Frame time also reduces due to eliminating a separate ReSTIR pass._ 



<!-- Start of picture text -->
—— a<br>E | m<br><!-- End of picture text -->



<!-- Start of picture text -->
ts<br>oF am<br><!-- End of picture text -->



<!-- Start of picture text -->
es<br>| teA pm<br><!-- End of picture text -->



<!-- Start of picture text -->
(a)  Original (b)  Our color noise reduction (c)  Reference<br>(17.0 ms, FLIP: 0.376) (17.0 ms, FLIP: 0.284)<br><!-- End of picture text -->

**Fig. 5.** _Evaluation of our color noise reduction method in Zero Day. 𝑀_ = 4 _for spatial resampling (3 spatial neighbors) in this case. By accumulating the vectorized form of resampling weight in spatial reuse, we achieve significant color noise reduction almost for free._ 

Our approach is to deterministically select survival when shifting a path. This would require extra Jacobian calculation in a PSS extended with Russian roulette dimensions. We circumvent this complexity by decoupling the definition of primary sample space from the actual sampling process – the paths are defined in the original primary sample space without Russian roulette and the Russian roulette only alters the initial sample’s PDF. In practice, we change the uniform _𝑝_ (u¯) = 1 to _𝑝_ (u¯) =<sup>�</sup> n<sup>_𝑑_</sup> _𝑖_ =<sup>−</sup> 1<sup>1</sup><sup>_𝑞𝑖_(u¯)where</sup><sup>_𝑞𝑖_(u¯)are survival probabilities at each bounce. This way,</sup> although Russian roulette can make the initial samples noisier, the ReSTIR PT output quality is affected to a much smaller extent thanks to spatiotemporal reuse (Figure 3). 

## **7 INDIVIDUAL EVALUATIONS ON DI-GI UNIFICATION, COLOR NOISE AND DISOCCLUSION NOISE REDUCTION** 

Figure 4, Figure 5, and Figure 6 present evaluations of our DI–GI unification, color noise reduction, and disocclusion noise reduction methods, respectively. 

Proc. ACM Comput. Graph. Interact. Tech., Vol. 9, No. 1, Article . Publication date: May 2026. 

Daqi Lin, Markus Kettunen, and Chris Wyman 

8 









<!-- Start of picture text -->
(a)  Traditional Motion Vectors (b)  Dual Motion Vectors (c)  Reference<br>(8.8 ms, FLIP: 0.259) (9.1 ms, FLIP: 0.231)<br><!-- End of picture text -->

**Fig. 6.** _Applying dual motion vectors [Zeng et al. 2021] provides a dramatic reduction of disocclusion noise which is severe in the Veach Ajar due to undersampling. The result contains some correlated noise but does not show "copy-pasting" artifacts._ 

## **8 RESERVOIR COMPRESSION** 

Reducing reservoir sizes helps reduce the bandwidth cost inside the reuse passes where each pixel reads its own reservoir from the memory and randomly access the reservoirs in a local neighborhood. Lin et al. [2022] uses 88-byte reservoirs for ReSTIR PT where attributes related to the reconnection vertex occupy a lot of the reservoir size. We found that many of the attributes can be compressed in a lossy format without affecting the quality. We list the details in Algorithm 1 where we shrink the reservoir size to 64 bytes. 

**Algorithm 1** A compression of the reservoir struct shrinks its size from 88 bytes to 64 bytes. **struct** Reservoir { float W; float3 F; // F is the integrand, pHat = luminance(F). uint initRandomSeed, rcVertexRandomSeed; ~~float M;~~ uint pathFlags; // Pack M as 8 bit integer into pathflags. uint rcVertexInstanceID, rcVertexPrimitiveIndex; ~~float2~~ → uint rcVertexBarycentrics; // Compress as two 16-bit Unorms. ~~float3~~ → uint rcVertexWi; // Octahedron encoding as two 16-bit Unorms. float3 rcVertexRadiance; ~~float4~~ → float2 rcVertexCachedValues; // Combine the separately stored Jacobian terms _𝑝_ ( _𝜔𝑘_ ) _,𝐺_ ( _𝑥𝑘_ −1 _,𝑥𝑘_ ) _, 𝑝_ ( _𝜔𝑘_ +1 ) (float3) of the base path into their product (float); NEE Light PDF (float) which is separately used to compute path MIS weight is unchanged. } 

## **9 COMPLETE PERFORMANCE RESULTS** 

Table 1 provides complete performance results of the four tested scenes at the default 1920×1080 resolution. Table 2 studies performance scaling across 540p, 1080p, and 2160p render resolutions. Compared with the baseline ReSTIR PT, our ReSTIR PT Enhanced has a smaller cost increase as resolution rises. In particular, our frame time increases by 4.03× from 1080p to 2160p (a 4× increase of pixels), whereas the baseline’s frame time increases by 4.57×. This is likely because our optimizations reduce pressure on the memory system, leading to better scaling at higher resolutions. 

## **10 ABLATION OF FOOTPRINT THRESHOLD PARAMETER CHOICE** 

A parameter sweep for _𝑐_ for the dual ray footprint threshold is provided in Figure 8. The optimal _𝑐_ are in the range of 0.005-0.04 across tested scenes with a wide variety of scales and materials, and 

Proc. ACM Comput. Graph. Interact. Tech., Vol. 9, No. 1, Article . Publication date: May 2026. 

ReSTIR PT Enhanced – Supplemental Document 

9 

**Table 1.** _Performance Results: Frame Time and Pass Times (in milliseconds)_ 

|Scene|Method|Frame|Initial|Temporal|Spatial|ReSTIR DI|
|---|---|---|---|---|---|---|
|||Time|Sampling|Reuse|Reuse|and others|
|Veach Ajar|Baseline|21.99|3.57|2.99|12.23|3.20|
||+Code microop|19.34|3.54|2.19|10.44|3.17|
||+Forced NEE reconnect|15.75|3.98|1.93|6.89|2.95|
||+Replay compaction|13.02|4.16|1.40|4.50|2.96|
||+Paired spatial reuse|12.10|4.56|1.53|2.88|3.13|
||+Russian roulette|10.20|3.17|1.34|2.67|3.02|
||+UnifyDI & GI|7.64|2.89|1.29|2.71|0.75|
||+New thresholds|8.74|3.09|1.72|2.92|1.01|
||+All improvements|9.89|3.14|1.81|3.17|1.77|
|Carousel|Baseline|46.76|17.74|6.88|15.92|6.22|
||+Code microop|43.82|17.74|5.70|14.25|6.13|
||<br>+Forced NEE reconnect|40.55|18.02|5.00|11.45|6.08|
||+Replay compaction|36.51|17.17|4.29|8.81|6.24|
||+Paired spatial reuse|32.85|17.35|4.36|4.81|6.33|
||+Russian roulette|20.16|6.29|3.07|4.50|6.30|
||+UnifyDI & GI|17.47|8.98|3.10|3.93|1.46|
||+New thresholds<br>|19.43|10.01|3.62|4.23|1.57|
||+All improvements|20.70|10.12|4.17|4.33|2.08|
|Opera House|Baseline|49.29|15.69|7.25|19.10|7.25|
||+Code Microop|46.87|16.25|5.82|17.17|7.63|
||+Forced NEE reconnect|43.61|17.78|4.45|13.21|8.17|
||+Replay compaction|40.30|19.35|3.10|9.10|8.75|
||+Paired spatial reuse|38.63|21.13|3.31|5.11|9.08|
||+Russian roulette|22.14|6.30|2.78|4.78|8.28|
||+UnifyDI & GI<br>|16.69|9.33|2.48|3.88|1.00|
||+New thresholds|18.67|8.81|4.30|3.97|1.59|
||+All improvements|19.54|8.96|4.67|4.07|1.84|
|Bathroom|Baseline|24.87|5.34|4.09|11.91|3.53|
||+Code microop|21.90|5.26|2.94|10.37|3.33|
||+Forced NEE reconnect|19.10|5.99|2.20|7.36|3.55|
||+Replay compaction|17.41|6.50|1.84|5.55|3.52|
||+Paired spatial reuse|16.49|7.21|1.99|3.63|3.66|
||+Russian roulette|13.59|5.06|1.78|3.38|3.37|
||+UnifyDI & GI|10.35|4.69|1.68|3.18|0.80|
||+New thresholds|11.20|4.80|2.04|3.27|1.09|
||+All improvements|11.99|4.84|2.15|3.35|1.65|
|Average|Baseline|35.73|10.59|5.30|14.79|5.05|
||+Code microop|32.98|10.70|4.16|13.06|5.07|
||<br>+Forced NEE reconnect|29.75|11.44|3.40|9.73|5.19|
||+Replay compaction|26.81|11.79|2.66|6.99|5.37|
||+Paired spatial reuse|25.02|12.56|2.80|4.11|5.55|
||+Russian roulette|16.52|5.21|2.24|3.83|5.24|
||+UnifyDI & GI|13.04|6.47|2.14|3.43|1.00|
||+New thresholds|14.51|6.68|2.92|3.60|1.31|
||+All improvements|15.53|6.77|3.20|3.73|1.83|



**Table 2.** _Performance vs. Resolution. "Ours" includes all our optimizations and quality improvements. Frame time (milliseconds) is reported for each scene and resolution combination._ 

|Method|Resolution|Veach Ajar|Carousel|Opera House|Bathroom|Avg. time scaling|
|---|---|---|---|---|---|---|
|Baseline|960×540|5.75|14.28|13.34|6.53||
||1920×1080|21.99|46.76|49.29|24.87|3.65×|
||3840×2160|97.66|208.63|234.12|114.63|4.57×|
|Ours|960×540|2.70|6.66|5.26|3.16||
||1920×1080|15.53|20.70|19.54|11.99|3.57×|
||3840×2160|62.34|77.99|82.10|50.13|4.03×|



Proc. ACM Comput. Graph. Interact. Tech., Vol. 9, No. 1, Article . Publication date: May 2026. 

Daqi Lin, Markus Kettunen, and Chris Wyman 

10 



<!-- Start of picture text -->
(a)  Kitchen (b)  Crown (c)  Bathroom<br>Fig. 7. Scenes<br>(d)  Veach Ajar (e)  Burger Restaurant (f)  Bistro Exterior<br><!-- End of picture text -->

**Fig. 8.** _Ablation of constant 𝑐 values for the dual footprint threshold using the FLIP metric, testing 𝑐_ = 0 _._ 005 _–_ 0 _._ 64 _across six scenes with very different scene dimensions and materials. Kitchen, Crown, Bathroom, Veach Ajar, Burger Restaurant, Bistro Exterior achieve minimum FLIP at 𝑐_ = 0 _._ 04 _,_ 0 _._ 01 _,_ 0 _._ 02 _,_ 0 _._ 02 _,_ 0 _._ 01 _,_ 0 _._ 005 _, respectively. This gives a 𝑐_ = 0 _._ 0175 _scene average, proving the nearoptimal choice of 𝑐_ = 0 _._ 02 _._ 



<!-- Start of picture text -->
𝑐 = 0 . 005 𝑐 = 0 . 02 Reference<br>Eine ae Ee = eel<br><!-- End of picture text -->

**Fig. 9.** _𝑐_ = 0 _._ 02 _and 𝑐_ = 0 _._ 005 _do not have obvious difference for the dual footprint threshold in Burger Restaurant (no correlation reduction technique used in the result)._ 

the cross-scene average optimal _𝑐_ = 0 _._ 0175 ≈ 0 _._ 02. We can see that diffuse scenes (e.g. Figure 9) are less sensitive to the _𝑐_ choice than glossy scenes (e.g. Figure 10). 

## **11 ADDITIONAL ABLATION STUDIES OF OUR DUAL FOOTPRINT THRESHOLD** 

To further validate the effectiveness of our method, we also compare against two alternative methods to define adaptive thresholds that are directly derived from prior work. 

The first alternative is directly using the path footprint threshold as in Neural Radiance Cache [Müller et al. 2021]. While this is a good approach for determining when to stop path tracing to read from the radiance cache, it is insufficient to function as a robust reconnection threshold, mainly because it does not consider the BSDF of the reconnection vertex. Figure 11 shows a case where the path footprint threshold allows reconnection vertex to be on a delta surface, causing poor caustics quality. 

Proc. ACM Comput. Graph. Interact. Tech., Vol. 9, No. 1, Article . Publication date: May 2026. 

ReSTIR PT Enhanced – Supplemental Document 

11 









<!-- Start of picture text -->
𝑐 = 0 . 005<br><!-- End of picture text -->















<!-- Start of picture text -->
𝑐 = 0 . 02 Reference<br><!-- End of picture text -->

**Fig. 10.** _𝑐_ = 0 _._ 02 _is significantly better than 𝑐_ = 0 _._ 005 _for the dual footprint threshold in Kitchen (no correlation reduction technique used in the result)._ 



























<!-- Start of picture text -->
Path footprint Path footprint Ours<br>Reference<br>( 𝑐 = 0 . 02) FLIP: 0.173 ( 𝑐 = 2) FLIP: 0.219 FLIP: 0.159<br><!-- End of picture text -->

**Fig. 11.** _Ignoring the reconnection vertex material, the path footprint threshold causes invalid reconnections on the glass egg and noisy caustics_ ( _𝑐_ = 0 _._ 02) _in regions farther from the refractor. Achieving similar caustic quality as ours needs at least 𝑐_ = 2 _, which degrades diffuse reflection quality. This means no 𝑐 value for the path footprint threshold can match our quality in this case._ 















<!-- Start of picture text -->
Jacobian bounding Jacobian bounding<br>( 𝑐 = 0 . 02) FLIP: 0.237 ( 𝑐 = 0 . 0002) FLIP: 0.234<br><!-- End of picture text -->













Ours Reference FLIP: 0.229 

**Fig. 12.** _As can be shown in this case in Kitchen scene, having two BSDF-sampling PDFs in the threshold test rejects reconnection between glossy surfaces over-aggressively (𝑐_ = 0 _._ 02 _right inset). When the thresholding constant 𝑐 is lowered to 0.0002, the glossy reflection quality is still a bit worse than our method, meanwhile the noise at the diffuse wall corner (left inset) starts to increase. This means no 𝑐 value for Jacobian bounding can match our quality in this case._ 

The second alternative is replacing the dual footprint threshold with a single "Jacobian bounding" threshold. Basically Equation 5 is changed into 



Proc. ACM Comput. Graph. Interact. Tech., Vol. 9, No. 1, Article . Publication date: May 2026. 

Daqi Lin, Markus Kettunen, and Chris Wyman 

12 

where the RHS is that of Equation 5 multiplied by 2 _𝜋_ (the reciprocal PDF of sampling a hemisphere uniformly) to match the threshold scales after including an additional PDF value in LHS. Intuitively, this bounds the PSS-to-area Jacobian determinant, so that the reconnection is less likely to have extreme Jacobians. This method also imitates clamping the spike-inducing _𝜌_ (x0 → x1 → x2) _𝐺_ (x1 ↔ x2) _𝜌_ (x1 → x2 → x3) term in VPL contribution [Davidovič et al. 2010; Hašan et al. 2009]. As can be seen in Figure 12, the Jacobian-bounding method is suboptimal compared to our method as it discourages reconnection between two glossy surfaces too strongly. 

## **12 BASELINE COMPARISON WITH AND WITHOUT DECORRELATION** 

Figure 13 provides a three-way comparison between the baseline, our method without the decorrelation technique (Section 5 of the main paper), and our default method with decorrelation enabled. Compared to the baseline, both versions of our method provides the same level of speedup and enhancement of image quality (when correlation is ignored) due to various algorithmic improvements. Our decorrelation technique further suppresses correlated noise at a slight performance cost. 

## **REFERENCES** 

- Benedikt Bitterli, Chris Wyman, Matt Pharr, Peter Shirley, Aaron Lefohn, and Wojciech Jarosz. 2020. Spatiotemporal reservoir resampling for real-time ray tracing with dynamic direct lighting. _ACM Transactions on Graphics (Proceedings of SIGGRAPH)_ 39, 4 (July 2020). 

- Tomáš Davidovič, Jaroslav Křivánek, Miloš Hašan, Philipp Slusallek, and Kavita Bala. 2010. Combining global and local virtual lights for detailed glossy illumination. _ACM Transactions on Graphics (TOG)_ 29, 6 (2010), 1–8. 

- Miloš Hašan, Jaroslav Křivánek, Bruce Walter, and Kavita Bala. 2009. Virtual spherical lights for many-light rendering of glossy scenes. In _ACM SIGGRAPH Asia 2009 papers_ . 1–6. 

- Daqi Lin, Markus Kettunen, Benedikt Bitterli, Jacopo Pantaleoni, Cem Yuksel, and Chris Wyman. 2022. Generalized Resampled Importance Sampling: Foundations of ReSTIR. 41, 4 (2022), 75:1–75:23. https://doi.org/10.1145/3528223.3530158 

- Thomas Müller, Fabrice Rousselle, Jan Novák, and Alexander Keller. 2021. Real-time neural radiance caching for path tracing. _arXiv preprint arXiv:2106.12372_ (2021). 

- Tizian Zeltner, Fabrice Rousselle, Andrea Weidlich, Petrik Clarberg, Jan Novák, Benedikt Bitterli, Alex Evans, Tomáš Davidovič, Simon Kallweit, and Aaron Lefohn. 2024. Real-time neural appearance models. _ACM Transactions on Graphics_ 43, 3 (2024), 1–17. 

- Zheng Zeng, Shiqiu Liu, Jinglei Yang, Lu Wang, and Ling-Qi Yan. 2021. Temporally Reliable Motion Vectors for Real-time Ray Tracing. In _Computer Graphics Forum_ , Vol. 40. Wiley Online Library, 79–90. 

Proc. ACM Comput. Graph. Interact. Tech., Vol. 9, No. 1, Article . Publication date: May 2026. 

ReSTIR PT Enhanced – Supplemental Document 

13 



<!-- Start of picture text -->
Spaceship<br>Original ReSTIR PT Ours (no decorrelation) Our ReSTIR PT Enhanced<br>(37.1 ms, FLIP: 0.321) (12.1 ms, FLIP: 0.256) (12.6 ms, FLIP: 0.263)<br>Watercolor<br>Original ReSTIR PT Ours (no decorrelation) Our ReSTIR PT Enhanced<br>(30.1 ms, FLIP: 0.171) (13.9 ms, FLIP: 0.166) (14.5 ms, FLIP: 0.166)<br>Zero Day<br>rg’ e- i te- Py [ on<br>Sa eo. | ee<br>Ss  S-=  S-=<br>— b. <a ae<br>44-Z-<br>aSo as<br>Original ReSTIR PT Ours (no decorrelation) Our ReSTIR PT Enhanced<br>(47.1 ms, FLIP: 0.439) (20.3 ms, FLIP: 0.407) (21.4 ms, FLIP: 0.414)<br>Crown<br>Original ReSTIR PT Ours (no decorrelation) Our ReSTIR PT Enhanced<br>(50.0 ms, FLIP: 0.483) (16.0 ms, FLIP: 0.436) (16.4 ms, FLIP: 0.444)<br><!-- End of picture text -->

**Fig. 13.** _Comparison between original ReSTIR PT [Lin et al. 2022], our method with no decorrelation, and our default method. FLIP error increases slightly because FLIP is relatively insensitive to correlation artifacts, but sensitive to luminance differences introduced by bias._ 

Proc. ACM Comput. Graph. Interact. Tech., Vol. 9, No. 1, Article . Publication date: May 2026. 

