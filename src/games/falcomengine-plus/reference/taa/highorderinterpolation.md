High-Performance Graphics 2021 N. Binder and T. Ritschel (Guest Editors) 

_Volume 40_ ( _2021_ ), _Number 8_ 

# **Hardware Adaptive High-Order Interpolation for Real-Time Graphics** 

D. Lin<sup>1</sup> and L. Seiler<sup>2</sup> and C. Yuksel<sup>1</sup> 

> 1 University of Utah 2Facebook Reality Labs 

## **Abstract** 

_Interpolation is a core operation that has widespread use in computer graphics. Though higher-order interpolation provides better quality, linear interpolation is often preferred due to its simplicity, performance, and hardware support._ 

_We present a unified refactoring of quadratic and cubic interpolations as standard linear interpolation plus linear interpolations of higher-order terms and show how they can be applied to regular grids and (triangular/tetrahedral) simplexes Our formulations can provide significant reduction in computation cost, as compared to typical higher-order interpolations and prior approaches that utilize existing hardware linear interpolation support to achieve higher-order interpolation. In addition, our formulation allows approximating the results by dynamically skipping some higher order terms with low weights for further savings in both computation and storage. Thus, higher-order interpolation can be performed adaptively, as needed. We also describe how relatively minor modifications to existing GPU hardware could provide hardware support for quadratic and cubic interpolations using our approach for both texture filtering operations and barycentric interpolation. We present a variety of examples using triangular, rectangular, tetrahedral, and cuboidal interpolations, showing the effectiveness of our higher-order interpolations in different applications._ 

## **CCS Concepts** 

_•_ **_Computing methodologies_** → **_Graphics processors; Texturing;_** 

## **1. Introduction** 

Parameter interpolation is widely used in computer graphics. Most commonly, it is performed linearly (i.e. bilinearly in 2D and trilinearly in 3D). For example, 2D texture sampling on the GPU uses bilinear interpolation to blend the color of the nearest four pixels, and shading normal (or any other attribute on a triangle) is computed using a linear combination of the three triangle vertex normals (or attributes). 

However, linear interpolation is prone to visual artifacts like Mach bands. Such problems can be resolved with high-order interpolations, such as quadratic or cubic, which are known to provide superior quality. 

Yet, lack of hardware support for high-order interpolation makes it undesirable for real-time graphics applications with limited computation budgets. This can be attributed to the computation cost of high-order interpolation and the significant hardware changes needed for supporting them directly. 

In this paper, we present a unified mathematical formulation that covers quadratic and cubic interpolation, expressing them as linear interpolation plus some high-order difference terms. This provides a simpler form than common high-order interpolation formulations 

in 2D and 3D domains. We also explain how this approach can be extended to interpolation in a simplex (triangles and tetrahedrons). 

Our formulations require less computation than standard highorder interpolation approaches and the state-of-art high-order interpolation methods performed on existing hardware [Csé18]. In addition, it is suitable for an efficient hardware implementation that requires relatively minor changes to existing linear interpolation pipeline on today’s GPUs, as we describe. 

Moreover, it allows clamping high-order difference terms when they are below a threshold, saving a sizeable amount of computation when high frequency details are sparse. This leads to an adaptive high-order interpolation solution, which incurs additional computation over linear interpolation only when needed. 

In applications not suitable for a hardware implementation, our formulation allows skipping additional storage of high-order data, saving substantial amount of storage and computation cost. 

We show examples in a wide range of real-time graphics rendering domains to show that our adaptive high-order interpolation with our proposed hardware can significantly improve visual quality, using only 1× to 2× more computation than linear interpolation in typical cases. Note that this is significantly cheaper than 5× to 

> © 2021 The Author(s) Computer Graphics Forum © 2021 The Eurographics Association and John Wiley & Sons Ltd. Published by John Wiley & Sons Ltd. 

_D. Lin & L. Seiler & C. Yuksel / Hardware Adaptive High-Order Interpolation for Real-Time Graphics_ 

7× more computation required by the state-of-art high-order interpolation on existing hardware [Csé18; Csé19]. 

We begin by providing the background and related prior work in Section 2. In Section 3 we describe our high-order interpolation formulations in grids and explain how they can be used in practical applications, such as texture filtering. Section 4 presents the details of how the existing hardware texture filtering pipeline can be modified to provide support for our high-order interpolation formulations. Then, in Section 5 we describe our high-order interpolations for simplexes, such as triangles and tetrahedra. Possible hardware acceleration techniques for simplex meshes are described in Section 6. We present our evaluation and results in Section 7 and conclude in Section 8. 

## **2. Related Work** 

Before we discuss the details of our approach, we summarize the related prior work in this section. 

## **2.1. Interpolation for Grids of Data** 

Many graphics applications require reconstructing smooth signals from (1D, 2D, or 3D) grids of data, usually stored as images or textures. For that, reconstruction filters are required. Bilinear or trilinear interpolation provides a cheap way to generate continuous signal out of discrete samples and they are supported by most graphics hardware. Yet, cubic interpolation is known to significantly improve the quality of texture filtering [SH05], volume rendering [ML94], and temporal anti-aliasing [YLS20]. 

Keys [Key81] introduced a family of cubic cardinal splines that interpolates the sampling data. Mitchell and Netravali [MN88] derived BC-splines to describe a more general family of cubic reconstruction filters that may or may not interpolate the data. The family of splines is parameterized by B and C. All cardinal splines have B=0. A separable bicubic filter using the BC-spline family has been implemented in shader code [Bjo04] to provide high quality image magnification filtering. Sigg and Hadwiger [SH05] proposed refactoring a bicubic/tricubic B-Spline filter (B=1,C=0), into a linear combination of four/eight hardware bilinear/trilinear taps. 

By modulating the source image with a checkerboard pattern, Csébfalvi [Csé18] solves the problem of negative bilinear/trilinear weights, allowing Catmull-Rom spline filter to be partially accelerated by hardware in a similar way. Since Catmull-Rom spline (B=0,C=1/2) interpolates the original data, it does not have the over-blurring problem of B-Spline filters. In a survey by Moller at al. [MMMY97], Catmull-Rom splines are verified to achieve the lowest reconstruction error in the entire family of BC-Spline filters. 

More recently, Csébfalvi [Csé19] proposed a method that uses hardware trilinear interpolation results for gradient estimation to do tricubic density filtering for volumes. This method closely approximates the result of Catmull-Rom spline interpolation but uses fewer taps. However, even with partial hardware acceleration proposed by these methods, bicubic and tricubic interpolation remain significantly more expensive than bilinear and trilinear interpolation. 

Numerous works have proposed FPGA implementation of cubic interpolation. Due to the high computational complexity of bicubic interpolation, a direct FPGA implementation of bicubic interpolation requires a lot of hardware resources [NA05]. To reduce the computational complexity, many FPGA implementations [LSC*08; WDLY11; GNSS14] limit the scope to handle specific image operation like scaling, where the bicubic weight pattern is repeated across the whole image and only needs to be computed once. Orthogonal methods like quantizing the interpolation weights [ZLZ*10], approximating the cubic kernel with multiple piecewise linear function [LSC*10; GNSS14], and using a mixture of cubic and linear function [BBGB20] have been applied in FPGA implementations. Sanaullah et al. [SKH16] presented an FPGA implementation of tricubic interpolation for molecular dynamics simulations. In comparison to these FPGA implementations, our method adaptively reduces the computational cost, and only requires slight modification to the existing GPU. Thus, our method can easily utilize the power of existing texture units to provide high order interpolation for a wide range of graphics applications. 

A graphics workstation system [MBDM97] has been made to support hardware bicubic interpolation at half of the rate of trilinear interpolation [Map06]. However, modern GPUs do not provide extra hardware to support higher-order filtering. Hardware implementations of higher-order filtering into standard GPU texture units cannot be justified if they require a large amount of dedicated logic that could instead be devoted to performing more bilinear interpolations per clock. Proposals to reuse existing texture logic require at least four bilinear texture reads per sample, plus shader execution time to select the bilinear sample positions [SH05; Csé18]. 

There is a category of adaptive interpolation techniques [MH15] for image resizing that derives interpolation weights from local spatial features (e.g. edge orientation statistics) of the images to provide better visual quality than bicubic interpolation. However, these methods generally involve expensive computation and are highly specified for the task of image resizing. In comparison, our approach is similar to the hierarchical form of high-order FEM [ZTZ05], where the difference between the higher-order element node values and lower-order element interpolation results are used as part of the high-order element. We adaptively discard small highorder terms purely based on the mathematical formulation of bicubic (and other high-order) interpolation. Our method is targeted at improving the performance of high order interpolation, and our method handles a wide range of applications in real-time rendering. 

## **2.2. Interpolation for Simplexes** 

Triangles and tetrahedrons are common building blocks of computer graphics. Shading a triangular mesh relies on interpolating vertex attributes, such asposition, normal, and texture coordinates. Linear interpolation of triangle vertex attributes are widely supported by graphics hardware. 

Higher order simplex interpolation has not been supported by graphics hardware, but research work has revealed problems that could benefit from higher order interpolation in triangles. Brown [Bro99] proposed using quadratic Bézier triangles [Far93] to interpolate an cosine highlight function over a triangle to approximate Phong shading [Pho75], avoiding the cost of renormalization 

© 2021 The Author(s) Computer Graphics Forum © 2021 The Eurographics Association and John Wiley & Sons Ltd. 

_D. Lin & L. Seiler & C. Yuksel / Hardware Adaptive High-Order Interpolation for Real-Time Graphics_ 

of normal vectors. Research work has proposed hardware that directly uses quadratic interpolation in screen space to interpolate a variety of vertex attributes without the need for perspective division [Sei98; ASS*01]. PN Triangles [VPBM01] constructs cubic and quadratic Bézier patches on the fly from local triangle attributes to achieve smooth visual appearance using low-poly meshes. With a similar goal, Phong Tessellation [BA08] introduces a computationally simple way to turn a triangle into a quadratic patch. 

Tetrahedral interpolation is widely used in Finite Element Methods [ZTZ05] for various kinds of simulation. Bargteil and Cohen [BC14] proposes using quadratic elements to reduce the simulation error and artifacts of deformable bodies. To reduce computation, they adaptively choose between linear and quadratic tetrahedral elements based on the difference of the predicted values of edge midpoints interpolated by each method. Phong deformation [Jam20] blends per-tet average gradients and per-vertex deformation gradients to achieve a quadratic tetrahedral interpolator to achieve higher order of accuracy for embedded deformation. 

Different from these methods, we propose a unified mathematical formulation for quadratic and cubic interpolation of simplexes of different dimensions. Our adaptive high-order triangular interpolation can benefit from hardware acceleration by slightly modifying the existing hardware used for rasterization. If the the triangles or the tetrahedrons are structured data, our method can use modified texture units to accelerate interpolation. 

## **3. High-Order Interpolation in Grids** 

Interpolation in 1D, 2D, and 3D grids are commonplace in computer graphics for applications like texture filtering. Though highorder is known to produce better quality, linear interpolation is more popular in practice, because it has direct hardware support on GPUs. 

In this section, we discuss high-order interpolation in grids and present how we can reorder the terms in quadratic and cubic interpolations to represent them as linear interpolation plus _high-order difference terms_ . This includes simplified forms using fewer data points. We also describe how we can take advantage of our reordering to provide adaptive high-order interpolation, such that linear interpolation is used wherever high-order interpolation would not produce visible improvement. Finally, we present how our approach can be used in typical applications. Most importantly, our reordering provides a convenient mechanism for modifying the existing hardware texture filtering system on GPUs to support highorder filtering, as we describe in Section 4. 

**Notation:** We use **P** _i_ , **P** _ij_ , and **P** _ijk_ to represent the data points (i.e. grid vertices) to be interpolated in 1D, 2D, and 3D grids, respectively, where _i, j, k_ ∈ Z. The evaluation position within the interpolation domain is represented using localized parameters _s,t, q_ ∈ [0 _,_ 1]. The data points at the corners of the interpolation domain correspond to _i, j, k_ ∈ {0 _,_ 1}. For representing values at element/edge centers, we use _i, j, k_ = ½. The interpolation functions are represented as ■ **C** _m_<sup>_n_D,where</sup><sup>_n_∈{1</sup><sup>_,_2</sup><sup>_,_3}isthedimensionand</sup> _m_ ∈ N is the number of data points and high-order difference terms used in the interpolation, using **L** for linear, **Q** for quadratic, and **C** 

for cubic interpolations. We also present simpler interpolation functions that omit one or more higher-order terms and are represented as □ **C**<sup>_n_</sup> _m_<sup>D,asopposedtostandardinterpolationfunctions</sup> ■<sup>**C**</sup> _m_<sup>_n_Dthat</sup> include all terms. 

## **3.1. 1D Interpolation** 

Let _L_<sup>1</sup> represent the linear interpolation operator, such that 



Obviously, linear interpolation along an edge in 1D simply uses this operator 



For defining quadratic interpolation along this edge, we can specify the desired value **P** ½ at the center of the edge. The resulting quadratic interpolation can be written in Bézier form as 



where **P**<sup>∗</sup> = 2 **P** ½ − ( **P** 0 + **P** 1)/2. By expanding and rearranging the terms, we can write 



Here, the first two terms are the linear interpolation between **P** 0 and **P** 1, and the last term includes the difference between the desired center value **P** ½ and the linear interpolation at the center. Therefore, by defining this difference as 



we can write the quadratic interpolation as 



Thus, quadratic interpolation becomes linear interpolation plus a second-order difference term. 

For cubic interpolation, we consider the derivatives at the vertices. Let **P** ⃗0 and − **P** 1⃗ represent the desired derivatives of the interpolated value at the two vertices. Along with the values at the vertices, they uniquely define a cubic function. Similar to the quadratic case, we define the difference values **D** ⃗0 and **D** ⃗1 between the desired derivatives and the derivative of linear interpolation, such that 



Then, the _cubic interpolation along an edge_ can be written as 



where the first term is, again, linear interpolation and the second term includes the third-order components with a linear interpolation of the difference values. 

These quadratic and cubic formulations in Equations 5 and 6 are mathematically identical to standard second-degree and thirddegree interpolations. Their advantage is purely in computation, as they allow us to begin with linear interpolation and factor out all high-order terms. This formulation will be particularly helpful for modifying existing linear filtering hardware on GPUs to perform higher-order interpolation, as we explain in Section 4. 

Computer Graphics Forum © 2021 The Eurographics Association and John Wiley & Sons Ltd. 

© 2021 The Author(s) 

_D. Lin & L. Seiler & C. Yuksel / Hardware Adaptive High-Order Interpolation for Real-Time Graphics_ 



<!-- Start of picture text -->
P01 D½1 P11 P01 D01 D11 P11<br>D<br>11<br>D D<br>01 D 11<br>01<br>t D0½ D1½<br>D<br>D½½ D 10 D<br>00 D 10<br>00<br>P00 D½0 P10 P00 D00 D10 P10<br>s s<br>(a)  Biquadratic (b)  Bicubic<br><!-- End of picture text -->

**Figure 1:** _The difference terms of biquadratic and bicubic interpolations in 2D._ 

The standard bicubic interpolation with 16 control points ■ **C**<sup>2D</sup> 16 can be written in a similar form by using the second derivatives, such that the desired second derivatives 



are achieved using difference terms 



for _i, j_ ∈ {0 _,_ 1}. With these internal difference terms, standard bicubic interpolation can be written as 



## **3.2. 2D Interpolation** 

In 2D, we rely on the bilinear interpolation operator 



Bilinear interpolation simply uses this operator, such that 

Biquadratic interpolation involves 9 control points: 4 at the vertices, 4 at the edge centers, and one at the middle of the rectangle they form (Figure 1a). Similar to the 1D case, we can write biquadratic interpolation using difference values at the edge centers **D** ½0, **D** ½1, **D** 0½, **D** 1½, and the difference value at the middle position **D** ½½. If we omit this middle difference value by taking **D** ½½ = **0** , the resulting quadratic interpolation can be written as 



If the middle difference term **D** ½½ is non-zero, biquadratic interpolation becomes 



and **D** ½½ can be written using the desired middle value **P** ½½ as 



The standard bicubic interpolation involves 16 control points. Similar to the quadratic case, if we omit the four difference terms in the interior of the rectangle and only consider the edges (Figure 1b), we get 



Note that, with this formulation, bicubic interpolation turns into three linear interpolations: the first one interpolates the four vertex values and the other two interpolate the difference in the derivatives. 



In this form, standard bicubic interpolation involves an additional bilinear interpolation over □ **C**<sup>2D</sup> 12<sup>.</sup> 

## **3.3. 3D Interpolation** 

This concept of representing higher-order interpolation as a sum of linear interpolation and higher-order terms can be extended to 3D as well. In 3D, we can use the trilinear interpolation operator 



This operator linearly blends two bilinear operators and trilinear interpolation simply uses it, such that 



where **P** _i jk_ with _i, j, k_ ∈ {0 _,_ 1} are the data values at the eight vertices of a cube. 

Again, for our quadratic and cubic interpolation functions, we omit the higher-order difference terms inside the cube and on the face centers of the cube, resulting 





© 2021 The Author(s) Computer Graphics Forum © 2021 The Eurographics Association and John Wiley & Sons Ltd. 

_D. Lin & L. Seiler & C. Yuksel / Hardware Adaptive High-Order Interpolation for Real-Time Graphics_ 

Note that standard triquadratic and tricubic interpolations ■ **Q**<sup>3D</sup> 27<sup>and</sup> ■<sup>**C**3D</sup> 64<sup>use 27 and 64 control points, respectively. Therefore, the ver-</sup> sions above that skip the interior difference terms save 7 and 32 control points for quadratic and cubic interpolation, respectively. 

## **3.4. Adaptive High-Order Interpolation** 

Notice that all our high-order interpolation formulations contain high-order difference terms, i.e. **D** _-terms_ , defined as the difference between a quantity approximated by lower-order interpolation and the desired value. These **D** -terms are indicators of how well linear interpolation approximates the desire values. 

When the **D** -terms are close to zero, high-order interpolation produces results with relatively small difference from linear interpolation. In such cases, simply using linear interpolation instead may be an acceptable approximation. This opens up the possibility of _adaptive high-order interpolation_ that skips the high-order difference terms when they are close to zero, determined by a user-defined threshold **D** min. 

At first glance, this may appear as a minor simplification, particularly considering software interpolation. However, as we explain in Section 4, adaptive interpolation can be used for more than doubling the throughput of a hardware implementation. 

## **3.5. High-Order Texture Filtering** 

Bicubic image filtering is known to produce superior image quality, as compared to bilinear, and it is often used for enlarging raster images. Using a Catmull-Rom spline, interpolation along 1D can be written as 



1D cubic interpolation can simply use this function 



For our cubic interpolation, however, we must first compute the **D** -terms. Using a Catmull-Rom spline (with uniform parameterization) that interpolates the data points **P** −1, **P** 0, **P** 1, and **P** 2, the **D** -terms can be written as 





<!-- Start of picture text -->
P-12 P02 P12 P22<br>P-11 P01 P11 P21<br>t<br>P-10 P00 P10 P20<br>P-1-1 P0 -1 P1-1 P2 -1<br>s<br><!-- End of picture text -->

**Figure 2:** _The texel data used for high-order filtering in 2D._ 

In our formulation, the **D** ⃗ _i j_ and **D** _i_ ⃗ _j_ terms can be computed using Equation 19. These are sufficient for evaluating □ **C**<sup>2D</sup> 12<sup>in Equa-</sup> tion 14. For ■ **C**<sup>2D</sup> 16<sup>we also need</sup><sup>**D**</sup> _i_<sup>⃗</sup> ⃗ _j_<sup>with</sup><sup>_i, j_∈{0</sup><sup>_,_1}. They can be</sup> computed using the desired second derivatives 



Note that computing these last four **D** -terms for ■ **C**<sup>2D</sup> 16<sup>involves com-</sup> bining 9 data points **P** _i j_ within a 3 × 3 block. 

Quadratic interpolation for texture filtering is not as popular. This is because it involves accessing the same amount of texture data as cubic interpolation and it cannot deliver the same quality. Nonetheless, it is still superior to linear filtering and requires less computation than cubic. Therefore, it might be preferable for some applications. 

We define quadratic interpolation similarly, using Catmull-Rom splines. In this case, the **D** -terms ensure that the interpolation matches the Catmull-Rom spline at the middle points. Thus, in 1D we can write 



In 2D, we can compute the **D** -terms for □ **Q**<sup>2D</sup> 8 similarly. As for ■<sup>**Q**2D</sup> 9<sup>,thecomputationofthemiddledifferenceterminvolvesall</sup> 16 data points, using 



for _i_ ∈ {0 _,_ 1}. Then, we can use our ■ **C**<sup>1D</sup> 4<sup>formulation in Equation 6.</sup> 

Similarly, bicubic interpolation using a 4 × 4 block of texel samples shown in Figure 2 can be defined using Catmull-Rom splines, such that 



Note that in both biquadratic and bicubic interpolations, evaluating the **D** -terms for □ **Q**<sup>2D</sup> 8<sup>and</sup> □<sup>**C**</sup> 12<sup>2Dare much simpler than the ad-</sup> ditional **D** -terms needed for ■ **Q**<sup>2D</sup> 9<sup>and</sup> ■<sup>**C**2D</sup> 16<sup>. Also,</sup> □<sup>**Q**</sup> 8<sup>2Dand</sup> □<sup>**C**</sup> 12<sup>2D</sup> do not need to access the corner texels **P** -1-1, **P** 2-1, **P** -12, and **P** 22. These corner texels are only needed for computing the internal **D** - terms used by ■ **Q**<sup>2D</sup> 9<sup>and</sup> ■<sup>**C**2D</sup> 16<sup>.</sup> 

The equations for triquadratic and tricubic cases are similar. Again, the **D** -terms for □ **Q**<sup>3D</sup> 20<sup>and</sup> □<sup>**C**3D</sup> 32<sup>aremuchsimplertocom-</sup> pute than ■ **Q**<sup>3D</sup> 27<sup>and</sup> ■<sup>**C**3D</sup> 64<sup>.</sup> 

© 2021 The Author(s) 

Computer Graphics Forum © 2021 The Eurographics Association and John Wiley & Sons Ltd. 

_D. Lin & L. Seiler & C. Yuksel / Hardware Adaptive High-Order Interpolation for Real-Time Graphics_ 

## **4. High-Order Hardware Texture Filtering** 

The versions for quadratic and cubic interpolations we present in Section 3 provide convenient mechanisms for hardware implementation. In this section, we discuss the details of existing GPU texture filtering hardware and how it can be modified to support high-order interpolation using our formulations. 

## **4.1. Texture Filtering on Current GPUs** 

Bilinear interpolation is a fundamental texture filtering operation on the GPU. Current GPUs implement it in one of two ways [MSY19]. The first way is to linearly interpolate pairs of texels along one dimension, then linearly interpolate the results along the other dimension, using 3 linear interpolations. The second way is to compute a weight for each of the four texels, then multiply them and add the four results. This requires an extra multiplier but allows more parallelism. It also requires only one renormalization for floating point textures instead of three. Our adaptive higher order interpolation method works with both of these implementation methods. 

Texture units in GPUs perform certain filtering operations in multiple steps. For example, trilinear filtering uses two bilinear operations. The result of each bilinear operation, or _BOP_ for short, is scaled and accumulated to produce the trilinear result. This is illustrated in Figure 3. Alternately, the GPU could perform the two BOPs in parallel, linearly interpolating the pair of results. The area involved is similar and the former method allows pure bilinear filtering operations to go twice as fast, while supporting two two-step trilinear operations in parallel, so the raw throughput of trilinear operations is the same. Therefore, we will assume that the GPU implements BOPs, although our method can also be adapted to work with a GPU designed with trilinear filtering as the basic operation. 



<!-- Start of picture text -->
Texture L1 Cache Weights Weights<br>Even U<br>Even V Accumulation<br>Odd U 2 Weights 5<br>Even V ose s<br>Even U gee 5<br>Odd U<br>Odd v<br><!-- End of picture text -->

**Figure 3:** _Texture filter unit on current GPUs. Bilinear operations (BOPs) can be scaled and accumulted to perform more complex filtering. The L1 texture cache is divided into four interleaved banks to allow parallel access to an unaligned 2_ × _2 of texels._ 

Another multi-step texture filtering operation is anisotropic filtering. In this case, up to 16 individual bilinear or trilinear filter results are blended to approximate an elongated filter region. Therefore, even if a texture unit implements trilinear filtering as its basic operation, it still needs to scale and accumulate the results of multiple BOPs to support anisotropic filtering. Since the number of BOPs is the bottleneck for computation, we use that to represent the computational cost of our texture interpolation techniques. 

## **4.2. Current Texture Cache Access Methods** 

Texture units in GPUs are fed by deep queues of pending filter operations. This provides time to compute filter weights, determine which texels will be accessed, and load the texels into caches. Typically there are multiple cache levels, e.g. a large cache shared over the whole GPU that feeds L2 and specialized L1 caches that are dedicated to individual texture units. The operation queue is typically sized to cover the latency of accessing off-chip memory, so that BOPs can usually occur without off-chip memory read delays. 

The L1 caches in texture units are specialized to allow a BOP per cycle without cache read delays. Bilinear texture filtering requires accessing an unaligned 2 × 2 of texels. For a standard cache or memory this could require 1, 2, or 4 memory accesses, depending on how the unaligned 2 × 2 block maps onto the aligned blocks that are stored in cache or memory. This is not a problem when reading texels from memory or from the L2 cache, since nearby texel values are likely to be used in later texture filter operations. But requiring multiple read cycles from the L1 cache has obvious problems for maintaining the desired texture processing rate of one BOP per clock. 

The standard solution is to divide the L1 texture cache into four banks, based on the low order bits of the _U_ and _V_ indices of the stored texture coordinates. The left side of Figure 3 illustrates how this works: texels are stored in banks based on whether their _U_ and _V_ indices are even or odd. Texture dimensions are padded to some tile size, typically at least 4 texels, so for each texture, one quarter of its texels fall into each of the four buffers. As a result, a bilinear interpolation unit can receive an unaligned 2 × 2 of texels on each clock by computing the appropriate memory addresses for each of the four banks of texture L1 cache. 

Finally, the number of L1 cache banks used depends on the number of texels that must be accessed in parallel. For example, GPUs that perform a trilinear filter in a single clock cycle require eight banks. Four of the banks provide an unaligned 2 × 2 access for even mip levels or even slice numbers. The other four banks provide an unaligned 2 × 2 access for odd mip levels or odd slice numbers. 

## **4.3. High-Order Texture Filtering on Hardware** 

High-order filtering with our formulations begins with linear interpolation (using ■ **L**<sup>2D</sup> 4<sup>in 2D and</sup> ■<sup>**L**3D</sup> 8<sup>in 3D). This is exactly what</sup> the current hardware is designed to do. Then, we add the **D** -terms. Notice that with all our quadratic and cubic formulations, except for ■ **Q**<sup>2D</sup> 9<sup>,the</sup><sup>**D**-termscanbeprocessedingroupsof4.Thecom-</sup> putation of each group is the same as a BOP multiplied by a scale factor. 

Computing the scale factors (e.g. (1 − _s_ ) _s_ , (1 − _t_ ) _t_ , etc.) needed for the steps involving the **D** -terms can be pipelined, so they do not require extra clock cycles. The amount of additional logic required is relatively small as well. For example, with 8-bit precision (1− _s_ ) _s_ can be computed with an 8 × 8 multiplier, though we used 32-bit floating point numbers for tests in Section 7. 

Computing the **D** -terms is also straightforward, given access to the necessary texels. Figure 4 shows that the **D** -term calculation is 

© 2021 The Author(s) Computer Graphics Forum © 2021 The Eurographics Association and John Wiley & Sons Ltd. 

_D. Lin & L. Seiler & C. Yuksel / Hardware Adaptive High-Order Interpolation for Real-Time Graphics_ 



<!-- Start of picture text -->
Texture L1 Cache Weights Weights<br>TH.<br>tit | 2 Accumulaion<br>aan 5 § Weights 5<br>Loy elalees 7 z<br>fay s2e 3<br>fH. | 2 aes 3<br>LEY]<br><!-- End of picture text -->

**Figure 4:** _Proposed texture filter unit for higher order interpolation. The L1 texture cache is divided into 4, 8, 16, or 32 interleaved banks to allow parallel access._ 

pipelined between the texel L1 cache read and the bilinear operation. Therefore, utilizing the existing BOP logic unit, we can include the **D** -terms for high-order filtering by simply using the same unit multiple times. This way, high-order filtering simply involves additional steps, similar to computing trilinear filtering with two bilinear filtering steps or anisotropic filtering in multiple steps on current hardware. 

The cost of computing the **D** -terms depends on the filtering operation being performed. Computing the **D** -terms along the edges needed for quadratic and cubic interpolations □ **Q**<sup>2D</sup> 8<sup>,</sup> □<sup>**C**</sup> 12<sup>2D,</sup> □<sup>**Q**3D</sup> 20<sup>,</sup> and □ **C**<sup>3D</sup> 32<sup>,onlyrequireaddersandbit-shifters(fordivisionbya</sup> power of 2), which do not require much extra area or power. The internal **D** -terms for quadratic and cubic interpolations ■ **Q**<sup>2D</sup> 9<sup>,</sup> ■<sup>**C**2D</sup> 16<sup>,</sup> ■<sup>**Q**3D</sup> 27<sup>,and</sup> ■<sup>**C**3D</sup> 64<sup>involvemoreexpensiveoperationstoevaluate</sup> from the data points **P** , so they require a greater area cost when modifying existing texture filtering hardware. Based on the number of **D** -terms, we can process □ **Q**<sup>2D</sup> 8 with 2 BOPs, □ **C**<sup>2D</sup> 12<sup>with3</sup> BOPs, □ **Q**<sup>3D</sup> 20<sup>with 5 BOPs, and</sup> □<sup>**C**3D</sup> 32<sup>with 8 BOPs in total. The in-</sup> terpolations that involve the internal **D** -terms, such as ■ **Q**<sup>2D</sup> 9<sup>,</sup> ■<sup>**C**2D</sup> 16<sup>,</sup> ■<sup>**Q**3D</sup> 27<sup>, and</sup> ■<sup>**C**3D</sup> 64<sup>, would require 3, 4, 6, and 16 BOPs, respectively.</sup> 

These numbers imply a dramatic reduction of performance compared to bilinear interpolation, but we can improve performance by using adaptive high-order interpolation, as we discussed in Section 3.4. Computation of the **D** -terms is pipelined in advance of performing BOPs, so adaptive filtering can be implemented by checking the four **D** -terms to be used for the next step. If they all are below the given threshold **D** min, the bilinear step can be skipped. If all of the **D** -terms are below the threshold, higher order interpolation requires just 1 BOP and so has the same performance as linear interpolation. 

For high-order interpolation involving more than 2 steps, instead of using pre-defined groups of **D** -terms, it is possible to group the **D** -terms that pass the threshold for minimizing the number of steps. However, this would require more complex logic, so we assume pre-defined groups of **D** -terms in our evaluation (Section 7). Still, it is possible to save power by simply turning off the multiplier for any **D** -term that is below the threshold. 

## **4.4. Texture Cache Access for Higher Order Filtering** 

High-order texture interpolation involves accessing more data, but this does not necessarily inflate the off-chip memory bandwidth, 

since the filter kernels near neighboring texels largely overlap. Therefore, we can expect the higher level caches to efficiently handle the data flow. However, the L1 cache must be changed to allow accessing more texels in parallel. 

As illustrated in Figure 4, using more banks in the L1 cache allows computing the **D** -terms in parallel to achieve peak performance. Then, groups of 4 **D** -terms can be passed to the existing bilinear interpolation logic. The L1 cache can be implemented using 4, 8, 16, or 32 banks with different levels of performance, as we discuss in Appendix A. 

## **5. Extensions to Interpolation in Simplexes** 

In Section 3 we described our difference-based formulations of quadratic and cubic interpolations for structured sample data in a grid. In this section, we extend this concept to arbitrary simplexes, such as line segments (1D), triangles (2D), and tetrahedra (3D). Similar to our notation for grids, (cubic) interpolations are represented as △ **C** _m_<sup>_n_D, if they omit some terms, and</sup> ▲<sup>**C**</sup> _m_<sup>_n_D, if they include</sup> all terms, where _n_ is the dimension and _m_ is the total number of interpolated data values, including the simplex vertices. 

## **5.1. Interpolation in a Simplex** 

A simplex in _n_ -dimensions is defined by _n_ + 1 vertices. Let **P** _i_ where _i_ ∈ {0 _,..., n_ } represent the data values at the vertices and **w** = [ _w_ 0 _,..., wn_ ]<sup>_T_</sup> is the barycentric coordinates of the interpolation point, forming a partition of unity, such that 



Linear interpolation is defined as a simple weighted average, using 





<!-- Start of picture text -->
P2 P2<br>D23<br>D02<br>D02 @‘ D12 f D12 y<br>P3<br>P0 D13<br>D01<br>P0 D01 P1 e P1<br><!-- End of picture text -->

**Figure 5:** _The high-order difference terms for quadratic interpolation on a triangle and in a tetrahedron._ 

For quadratic interpolation, we define high-order difference terms **D** _i j_ between each pair of vertices _i_ and _j_ with _i_ < _j_ , as shown in Figure 5. Then, quadratic interpolation can be written as 



There are _n_ ( _n_ + 1)/2 quadratic **D** -terms: a line has 1, a triangle 

> © 2021 The Author(s) 

Computer Graphics Forum © 2021 The Eurographics Association and John Wiley & Sons Ltd. 

_D. Lin & L. Seiler & C. Yuksel / Hardware Adaptive High-Order Interpolation for Real-Time Graphics_ 



<!-- Start of picture text -->
P2 P2<br>D20 D21 D20 D23<br>D02 D21 D32<br>D02 D012 D12 P0 D012 D12 P3<br>D<br>01<br>P0 D01 D10 P1 D10 P1<br><!-- End of picture text -->

**Figure 6:** _The high-order difference terms for cubic interpolation on a triangle and in a tetrahedron._ 

has 3, and a tetrahedron has 6. Thus, we get ▲ **Q** 6<sup>2D</sup> and ▲ **Q** 10<sup>3Dfor</sup> triangles and tetrahedra, respectively. 

For defining cubic interpolation, we use **D** ⃗ _ij_ to specify the desired derivatives along each edge, as shown in Figure 6. Then, we can write our cubic interpolation formulation as 



This formulation defines △ **C**<sup>2D</sup> 9<sup>and</sup> △<sup>**C**3D</sup> 16<sup>for triangles and tetrahe-</sup> dra, respectively, only considering the derivatives along the edges of the simplex. For triangles, however, cubic interpolation can also specify a desired value **P** 012 at the center using an interior difference term **D** 012 = **P** 012 − △ **C** 9<sup>2D(</sup><sup>**w**</sup> _center_<sup>), where</sup><sup>**w**</sup> _center_<sup>=</sup><sup>**1**/3 is</sup> the barycentric coordinates of the center of the triangle. Including higher-dimensional simplexes, we can write 



Here, **D** _i jk_ = **P** _i jk_ − △ **C**<sup>_n_</sup> (<sup>D</sup> _n_ +1)<sup>2(</sup><sup>**w**</sup> _center_<sup>_ijk_),where</sup><sup>**w**</sup><sup>_ijk_</sup> _center_<sup>isthe</sup> barycentric coordinates for the center of the triangle formed by verices _i, j, k_ of the simplex (e.g. for a tetrahedron, **w** _center_<sup>012=</sup> [<sup>1</sup> /3<sup>1</sup> /3<sup>1</sup> /3 0]<sup>_T_</sup> ). Note that this formulation provides _n_ ( _n_ − 1)( _n_ + 1)/6 interior **D** -terms: a triangle has 1 and a tetrahedron has 4. The resulting interpolations for triangles and tetrahedra are ▲ **C**<sup>2D</sup> 10<sup>and</sup> ▲<sup>**C**3D</sup> 20<sup>,re-</sup> spectively. 

## **5.2. Practical Interpolation Applications Using Simplexes** 

High-order interpolations within a grid described in Section 3 allow computing the **D** -terms from the data points on-the-fly. In the case of arbitrary simplex meshes, however, a typical applicationneeds to pre-compute the **D** -terms. This is because determining the desired derivatives or the edge/triangle center values typically requires traversing the simplex topology, using discrete differential geometry operators [MDSB03]. 

There are exception, however. For example, reconstructing 

smooth normals for shading a triangle, the **D** -terms can be computed on-the-fly from the triangle’s vertex positions and normals [VPBM01]. Also, the **D** -terms can be computed on-the-fly for barycentric filtering using mesh color textures [Yuk17] or patch textures [MSY19] for providing hardware texture filtering support for mesh colors [YKH10], as they use structured triangular texel distributions. 

Another example is a regular simplex mesh in a grid, such as a triangular mesh or a tetrahedral mesh with vertices on a regular grid. A 2D grid cell can be represented using two triangles and a 3D grid cell can be formed by 5 or 6 tetrahedra. Indeed, the vertex data for such meshes can be stored in 2D or 3D textures. This offers a cheaper alternative to texture filtering using fewer data points, and it can be used for applications like color space conversion [KNPH95] that can benefit from high-order filtering. 

When the **D** -terms cannot be computed during interpolation and must be pre-computed, adaptively skipping some **D** -terms can provide storage, memory bandwidth, and computation savings at run time. 

## **6. Hardware Interpolation for Simplex Meshes** 

For providing hardware-accelerated interpolation, there are two cases to consider: regular simplex meshes on a grid and arbitrary simplex meshes. 

The data for regular meshes of triangles or tetrahedra can be stored in 2D or 3D grids. In that case, hardware interpolation can be supported in a similar way as described in Section 4. The only differences are the interpolation functions and the subsets of the texel data blocks used in the interpolation (Section 6.1). 

Arbitrary simplex meshes, however, cannot be handled similarly and require a different treatment (Section 6.2). 

## **6.1. Hardware Interpolation for Regular Simplexes** 

For regular triangles on a grid, the existing bilinear interpolation unit can be modified to support barycentric linear interpolation [MSY19]. One of the four texels is weighted as zero and the other three use weights that produce the same result as barycentric interpolation. This can be done for both methods of designing the bilinear operation block, that is using three linear interpolation units or using four parallel multipliers. 

Our method for supporting nonlinear triangular interpolation extends this method. Reading an unaligned 4 × 4 array of texels allows **D** -terms to be computed. These are then fed into the bilinear operation block, along with appropriate weights. In general, up to three **D** -terms can be retired per cycle through the bilinear operation logic. 

A similar method may be used for regular tetrahedra in a 3D grid. Linear interpolation can be achieved with an unaligned 2 × 2 × 2 array of texels. Four of the texel values are used and the rest are ignored. The four chosen vertices are gathered into a single BOP. If the BOP is implemented using four multipliers, these get the four barycentric parameters. If the BOP is implemented using three linear interpolations, the weights for the first pair are _w_ 0/( _w_ 0 + _w_ 1) 

© 2021 The Author(s) Computer Graphics Forum © 2021 The Eurographics Association and John Wiley & Sons Ltd. 

_D. Lin & L. Seiler & C. Yuksel / Hardware Adaptive High-Order Interpolation for Real-Time Graphics_ 



<!-- Start of picture text -->
Reference □ C 2D 12 , Ratio of ∣ D ∣ ≥ 0 . 048 □ C 2D 12 , Ratio of ∣ D ∣ ≥ 0 . 2 □ Q 2D 8 , Ratio of ∣ D ∣ ≥ 0 . 0095 □ Q 2D 8 , Ratio of ∣ D ∣ ≥ 0 . 0315<br>240(cos(0 . 0008( x 2 + y 2 )) + 1) (25% clamped) (50% clamped) (25% clamped) (50% clamped)<br>\ | ZN R S e y ow<br>(a) (b) (c) (d) (e)<br><!-- End of picture text -->

**Figure 7:** _We test with a sinusoidal function (the image spans a parameter range from_ (0 _,_ 0) _to_ (1 _,_ 1) _). The reference image (left) is sampled using_ 1024 × 1024 _resolution (the compared insets are highlighted), while our tested methods upsample a_ 128 × 128 _resolution input to_ 1024 × 1024 _resolution. We visualize the ratio of_ **D** _-terms in bicubic interpolation that are above a threshold (middle and right). Dark red stands for 1 (all 8_ **D** _-terms for the pixel are above the threshold), and dark blue stands for 0._ 

and _w_ 2/( _w_ 2 + _w_ 3). The weight for the linear interpolation that combines them is ( _w_ 0 + _w_ 1)/( _w_ 0 + _w_ 1 + _w_ 2 + _w_ 3). 

Our method for supporting nonlinear tetrahedral interpolation extends this method. All **D** -terms to be computed using an unaligned 4 × 4 × 4 array of texels. They are then fed into the bilinear operation block as barycentric operations, along with appropriate weights. In general, up to four **D** -terms can be retired per cycle through the bilinear operation logic. 

## **6.2. Hardware Interpolation for Arbitrary Simplexes** 

Linear barycentric interpolation for triangles can be performed either in hardware or using code attached to the start of the fragment shader. Either way, it typically supports multiplying three barycentric coefficients times three parameter values. 

**D** -terms for quadratic and cubic triangular interpolation can be generated e.g. in a geometry shader and then passed to the fragment shader. The interpolation can be performed using either logic or shader code that computes products of the barycentric terms and multiplies them in turn by the **D** -terms. In hardware this could be performed using multiple passes through the existing logic, eliminating the extra hardware multiplies where the **D** -terms are zero. Typically, GPU shader instructions support testing conditionals in parallel with ALU operations, so testing to see if **D** -terms can be eliminated does not need to reduce the performance of a shader code implementation, either. 

Tetrahedral interpolation is performed in the same way, except with four barycentric terms instead of three. This is not needed in pixel shaders, but can be useful in vertex shaders (as illustrated in Section 7.5) as well as geometry shaders. As for the software implementation of unstructured triangles, shader code computes **D** - terms for unstructured tetrahedra and then performs the necessary multiplies and adds for non-zero sets of **D** -terms. 

## **7. Evaluation** 

We demonstrate the effectiveness of our hardware adaptive highorder interpolation method in 5 different real-time rendering applications to cover all of rectangular, triangular, tetrahedral, and 

cuboidal cases. In each application, we compare the quality of our result to linear interpolation. We show how our result achieves similar quality as non-adaptive high-order interpolation by only computing high-order interpolation when necessary. By exploiting the sparseness of high-frequency information, our method can discard most high-order terms in many applications, making it run at a comparative performance or use similar amount of storage as linear interpolation. We also compare with the state-of-art high-order interpolation method for the application, if one exists and show how our method improves the performance while delivering similar quality. 

For interpolations in grids, we use the number of BOPs computed as the performance metric for comparing different methods. For simplex mesh examples targetting software implementation, we report shader execution times on current hardware. 

## **7.1. 2D Texture Filtering** 

First, we present results using a synthetic texture that contains a variety of frequencies, shown in Figure 7. Given a threshold **D** min = 0 _._ 048 for bicubic interpolation □ **C**<sup>2D</sup> 12<sup>,25%ofthe</sup><sup>**D**-terms</sup> are eliminated (Figure 7b-c). The higher frequency region shows higher ratio of **D** -terms with magnitude greater than the threshold. Below a certain frequency, all **D** -terms are below the threshold. Using **D** min = 0 _._ 2, 50% of the **D** -terms are clamped. The observation is similar for biquadratic interpolation □ **Q**<sup>2D</sup> 8 (Figure 7d-e). With **D** min = 0 we turn off clamping and all **D** -terms are used. 

A comparison between different interpolation methods using parts of the same synthetic texture can be found in Figure 8. Our bicubic interpolation □ **C**<sup>2D</sup> 12<sup>with</sup><sup>**D**</sup> min<sup>= 0 generates almost identical</sup> result as standard bicubic interpolation ■ **C**<sup>2D</sup> 16<sup>.Thisshowsthatthe</sup> impact of omitting the 5th and 6th order terms is relatively minor in this example. When increasing **D** min to 0 _._ 048 to clamp 25% of the **D** -terms, the difference is hard to notice. With **D** min = 0 _._ 2, the low-frequency inset begins to show patterns related to bilinear interpolation, because most **D** -terms are clamped. However, the highfrequency inset still looks unchanged, because higher-frequency regions contain more pixels with larger **D** -terms. Our □ **Q**<sup>2D</sup> 8 biquadratic interpolation also produces almost identical result as the more expensive ■ **Q**<sup>2D</sup> 9<sup>version.</sup><sup>**D**</sup> min<sup>affects the results similarly to</sup> the bicubic case. 

> © 2021 The Author(s) 

Computer Graphics Forum © 2021 The Eurographics Association and John Wiley & Sons Ltd. 



<!-- Start of picture text -->
D. Lin & L. Seiler & C. Yuksel / Hardware Adaptive High-Order Interpolation for Real-Time Graphics<br><!-- End of picture text -->



<!-- Start of picture text -->
Our Bicubic Interpolation Our Biquadratic Interpolation<br>Reference ■ C 2D 16 □ C 2D 12 □ C 2D 12 (-25% D ) □ C 2D 12 (-50% D ) ■ Q 2D 9 □ Q 2D 8 □ Q 2D 8 (-25% D ) □ Q 2D 8 (-50% D ) Bilinear<br>(Standard Bicubic) No Clamp D min = 0.048 D min = 0.2 No Clamp No Clamp D min = 0.0095 D min = 0.0315<br>MSE: 0 (reference) 0.01304 0.01362 0.01365 0.01408 0.01413 0.01458 0.01462 0.01520 0.02429<br>Figure 8:  A comparison of different interpolation method for upsampling the sinusoidal function from  128 × 128 × 128 128  to  1024 × 1024 × 1024 1024 . The first<br>and third row: insets. The second and fourth row: 4  ×  and 1  ×  difference images with respect to the reference.<br>0.024 5<br>Bilinear [Csébfalvi 2018]<br>0.022<br>(Bicubic) 4 (Bicubic)<br>0.02 (Biquadratic)  Ideal Case<br>3 (Biquadratic)<br>0.018 Standard Bicubic Bilinear<br>2<br>0.016<br>1<br>0.014<br>0.012 0<br>0% 20% 40%  60% 80% 100% 0% 20% 40%  60% 80% 100%<br>Percentage of clamped  Percentage of clamped<br>MSE<br>Bilinear operations per pixel<br><!-- End of picture text -->

**Figure 8:** _A comparison of different interpolation method for upsampling the sinusoidal function from_ 128 × 128 × 128 128 _to_ 1024 × 1024 × 1024 1024 _. The first and third row: insets. The second and fourth row: 4_ × _and 1_ × _difference images with respect to the reference._ 

**Figure 9:** _Comparing MSE of different interpolation method using different_ **D** min _(only our bicubic and biquadratic interpolation is affected) for the sinusoidal function upsampling._ 

**Figure 10:** _Comparing average bilinear operations per pixel of different methods for the sinusoidal function upsampling._ 

In Figure 9, we compare mean square error (MSE) of different interpolation methods. With **D** min = 0, our bicubic interpolation □ **C**<sup>2D</sup> 12<sup>produces a slightly higher MSE than the standard bicu-</sup> bic interpolation ■ **C**<sup>2D</sup> 16<sup>,duetotheomissionof5thand6thor-</sup> der terms. Our biquadratic interpolation □ **Q**<sup>2D</sup> 8<sup>producesaslightly</sup> higher MSE. Nonetheless, MSE for all high-order interpolations is much lower than bilinear. As expected, MSE grows with increasing **D** min. 

We compare the performance of our approach to Csébfalvi’s method [Csé18], the most efficient implementation of standard bicubic interpolation on current GPU hardware. This method uses 5 bilinear operations (4 bilinear texture access on the GPU plus 1 bilinear operation for combining 4 terms with weights) to produce the same result as standard Catmull-Rom bicubic interpolation. The 

texture is modified by modulating the input texture with a checkboard pattern of 1 and −1 values. 

In Figure 10, we visualize the average BOPs per pixel for each method. We see that even using **D** min = 0, our bicubic interpolation (3 per pixel) has fewer number of BOPs than Csébfalvi’s method (5 per pixel). Using adaptive higher-order filtering with **D** min > 0, we can improve the performance further. The solid line for □ **C**<sup>2D</sup> 12 shows the performance when groups of 4 **D** -terms are formed in a predefined order and a BOP is skipped only when _all_ **D** -terms in a group are below the threshold. The dashed line shows the ideal case that dynamically groups the **D** -terms for maximum performance. Our biquadratic interpolation starts at a cheaper cost of 2 BOPs at **D** min = 0 and decreases slowly with increasing **D** min. 

To see how our bicubic rectangular interpolation works in prac- 

© 2021 The Author(s) Computer Graphics Forum © 2021 The Eurographics Association and John Wiley & Sons Ltd. 

_D. Lin & L. Seiler & C. Yuksel / Hardware Adaptive High-Order Interpolation for Real-Time Graphics_ 



<!-- Start of picture text -->
Our Bicubic Interpolation (□ C 12 2D) D min = 0 . 2 bilinear operation heatmap<br>e e | Sere<br>Bilinear □ C 2D 12 , D min = 0 . 2 □ C 2D 12 ,  No Clamp Standard Bicubic<br>MSE: 6 . 9 × 10 −4 MSE: 1 . 6 × 10 −4 MSE: 4 . 6 × 10 −6 MSE: 0 (reference)<br>BOPs/read: 1 BOPs/read: 1.57 BOPs/read: 3 BOPs/read: 5<br><!-- End of picture text -->

































**Figure 11:** _Comparing texture magnification filtering quality of bilinear interpolation and our bicubic interpolation with no clamping and adaptive clamping with_ **D** min = 0 _._ 2 _. A bilinear operation heatmap at_ **D** min = 0 _._ 2 _is provided (Red: 3 bops, Yellow: 2 bops, Blue: 1 bop). Third and fifth row:_ 4× _difference images with respect to standard bicubic interpolation of the insets in the rows above._ 

tice, we test the texture filtering quality in the San Miguel scene, shown in Figure 11. We observe that our □ **C**<sup>2D</sup> 12<sup>producesindis-</sup> tinguishable result to standard bicubic interpolation ■ **C**<sup>2D</sup> 16<sup>.Setting</sup> **D** min = 0 _._ 2 produces visually very similar result with only 1.57 BOPs on average, in contrast to 3 BOPs without clamping, and 5 BOPs with Csébfalvi’s method [Csé18] for standard bicubic interpolation. 

## **7.2. Temporal Anti-aliasing** 

Another use case of bicubic interpolation is temporal anti-aliasing (TAA). TAA is widely used in today’s game engines. It reuses subpixel samples accumulated in previous frames to achieve the effect of supersampling. This is done by jittering the camera to cover many sample positions in each pixel across several frames. When the camera or the scene objects are moving, motion vectors need to be calculated to fetch the pixel that corresponds to the same shading point in the history buffer (i.e. _reprojection_ ). 

One problem of TAA is known as _resampling blur_ [YLS20]. This 



<!-- Start of picture text -->
No TAA TAA TAA 64×<br>TAA (bilinear lookup) (bicubic, adaptive) (bicubic lookup) Supersampling<br><!-- End of picture text -->











**Figure 12:** _A comparison of 3 different history buffer lookup methods. Note that jagged edges can be seen when not using TAA. A rendering using_ 64× _supersampling is provided as the ground truth. Note that bicubic lookup avoids the blurring of object edges and texture details in as can be seen in result using bilinear lookup. Bicubic adaptive lookup using_ **D** min = 0 _._ 003 _produces highly similar result to using no clamping._ 

is due to reprojected positions landing at fractional pixel locations, which requires interpolation to gather colors from nearby pixels. As can be seen in Figure 12, using bicubic interpolation reduces resampling blur as compared to bilinear interpolation. 

Using our method with hardware modification, it is possible to bring down the cost of bicubic interpolation to a level close to bilinear interpolation, while preserving most of the quality, as shown in Figure 13. Without clamping, our □ **C**<sup>2D</sup> 12<sup>bringsdowntheBOPs</sup> per pixel to 3 to produce visually identical results standard bicubic interpolation. Using **D** min = 0 _._ 003, more than 80% of bilinear operations related to higher-order terms can be skipped. The result is that only 39% more bilinear operations than pure bilinear interpolation are executed to achieve a result almost identical to full bicubic interpolation. The bilinear operation heatmap in Figure 13 shows that these discarded bilinear operations are mainly in regions with slowly varying color, while bicubic interpolation are preserved at high frequency regions like object edges and texture gradient edges, which suffers more from resampling blur. Even though these results use many state-of-art TAA improvement techniques provided by the Falcor engine [Kar14], including neighborhood clamping and adaptive blending, they are insufficient for removing the resampling blur caused by bilinear interpolation. Bicubic interpolation, however, visibly improves the results. 

## **7.3. Volume Rendering** 

We use volume rendering as an application to test our tricubic interpolation □ **C**<sup>3D</sup> 32<sup>. Our test scene contains a volumetric teapot with</sup> isotropic scattering and a directional light shown in Figure 14. The density data of the volume is defined in a 3D grid. We use ray marching to compute the radiance reaching the camera. At each ray marching step, a density value is queried for computing the extinction coefficient. The lighting is gathered from a directional light with the visibility along the shadow ray computed via ray marching. We compare our performance to Csébfalvi’s tricubic volume density filtering method [Csé19]. Note that the Csébfalvi’s method uses 7 trilinear taps on the current hardware, which is equivalent to 14 bilinear operations. In comparison, our method only uses 8 

> Computer Graphics Forum © 2021 The Eurographics Association and John Wiley & Sons Ltd. 

© 2021 The Author(s) 



<!-- Start of picture text -->
D. Lin & L. Seiler & C. Yuksel / Hardware Adaptive High-Order Interpolation for Real-Time Graphics<br>Our Bicubic Interpolation (□ C 12 2D) D min = 0 . 003 bilinear operation heatmap Our Tricubic Interpolation (□ C 32 3D) D min = 0 . 01 bilinear operation heat map<br>= i<br>=<br>Bilinear □ C 2D 12 , D min = 0 . 003 □ C 2D 12 ,  No Clamp Standard Bicubic Trilinear □ C 3D 32 , D min = 0 . 01 □ C 3D 32 ,  No Clamp<br>MSE: 4 . 0 × 10 −5 MSE: 7 . 7 × 10 −6 MSE: 7 . 2 × 10 −7 MSE: 0 (reference) MSE: 2 . 3 × 10 −4 MSE: 9 . 0 × 10 −7 MSE: 0 (reference)<br>BOPs/read: 1 BOPs/read: 1.39 BOPs/read: 3 BOPs/read: 5 BOPs/read: 2 BOPs/read: 2.98 BOPs/read: 8<br><!-- End of picture text -->

















**Figure 13:** _Comparing TAA results using bilinear history lookup and our bicubic history lookup with and without clamping. The results are obtained by repeatly shaking the camera horizontally and freezing the rendering. A bilinear operation heatmap at_ **D** min = 0 _._ 003 _is provided (Red: 3 bops, Yellow: 2 bops, Blue: 1 bop). Third and fifth row:_ 8× _difference images in comparison to the standard bicubic interpolation._ 

bilinear operations. Note that our □ **C**<sup>3D</sup> 32<sup>function is mathematically</sup> identical to Csébfalvi’s method, but our formulation needs fewer bilinear operations. 









<!-- Start of picture text -->
2<br>ma) |<br><!-- End of picture text -->





**Figure 14:** _Comparison of trilinear and our tribubic interpolation with and without clamping. The heatmap shows the number of bilinear operations per pixel. For clamping with_ **D** min = 0 _._ 01 _, only 16.5% bilinear operations related to high-order interpolation are executed. Only 11% of total_ **D** _-terms are non-zero. Third and fifth rows: 8_ × _differences from tricubic interpolation with no clamping._ 

## **7.4. Precomputed Radiance Transfer** 

The performance advantage of our method is improved with adaptive high-order interpolation. By using a low threshold of **D** min = 0 _._ 01, our method reduces more than 80% of bilinear interpolations related to high-order interpolation, most of which are at the center region of the teapot (with almost uniform density) and the region outside the teapot (with zero density). By exploiting the sparseness of high frequency signal, our adaptive tricubic interpolation only uses about 1.5× the bilinear operation required for trilinear interpolation, while producing visually identical quality as full tricubic interpolation using □ **C**<sup>3D</sup> 32<sup>(4× more bilinear operation</sup> than trilinear). 

In precomputed radiance transfer (PRT) [SKS02], transfer vectors and transfer matrices are precomputed for all vertices in spherical harmonic (SH) basis such that given an incoming radiance field expressed in SH basis, the exitant radiance field can be easily obtained using vector dot product (for diffuse reflection) or matrix-vector product (for specular refleciton). This allows real-time, noise-free, high-fidelity self-shadowing and self-reflection of a object in any dynamic, low frequency lighting environments (e.g. a rotating environment map). 

We limit our evaluation to 1-bounce diffuse reflection with self- 

© 2021 The Author(s) Computer Graphics Forum © 2021 The Eurographics Association and John Wiley & Sons Ltd. 

_D. Lin & L. Seiler & C. Yuksel / Hardware Adaptive High-Order Interpolation for Real-Time Graphics_ 



<!-- Start of picture text -->
Our Quadratic Interpolation (▲ Q 6 2D) D min = 0 . 015 heat map of edge storage<br><!-- End of picture text -->



<!-- Start of picture text -->
Linear ▲ Q 6 2D , D min = 0 . 015 ▲ Q 6 2D, No Clamp Path Tracing<br>MSE: 2 . 4 × 10 −5 MSE: 1 . 6 × 10 −5 MSE: 1 . 4 × 10 −5 MSE: 0 (reference)<br>14.3 MB (1) 26.4 MB (1.84×) 58.4 MB (4.08×) Storage: N/A<br>0.06 ms (1) 0.19 (3.17×) 0.30 ms (5.00×) Shader time: N/A<br>If i I} |<br><!-- End of picture text -->











<!-- Start of picture text -->
Mi<br><!-- End of picture text -->



<!-- Start of picture text -->
Vie<br><!-- End of picture text -->



<!-- Start of picture text -->
 Vie<br><!-- End of picture text -->



<!-- Start of picture text -->
 Wis<br><!-- End of picture text -->



<!-- Start of picture text -->
TL<br><!-- End of picture text -->



<!-- Start of picture text -->
ya<br><!-- End of picture text -->





**Figure 15:** _Comparison of linear triangular interpolation of transfer vectors to our quadratic triangular interpolation with and without clamping for precomputed radiance transfer. With_ **D** min = 0 _._ 015 _, only 25% of edge data are stored and 23% three-multiplies related to quadratic terms are executed in the fragment shader, as compared to no clamping. The heatmap visualizes the number of edges with storage in the corresponding triangle of the pixel. The third and fifth rows: 4_ × _differences from path tracing reference._ 

shadowing, using the Happy Buddha model, shown in Figure 15. We precompute a (RGB) transfer vector using the first 5 SH bands (5<sup>2</sup> × 3 = 75 floats) for each vertex. To enable quadratic interpolation, we also precompute a transfer vector (in fact, the difference terms) at the mid point of each edge. We traverse 4096 shadow rays to precompute a shadowed diffuse transfer vector in SH basis. When using linear interpolation, dot product between transfer vector and the lighting vector is computed in vertex shader, and the resulting color is interpolated automatically for the fragment shader. When using quadratic interpolation, we perform manual interpolation of quadratic **D** -terms (of ▲ **Q** 6<sup>2D)inthefragmentshaderand</sup> dot product with lighting vector, then add the result on top of the color from hardware linear interpolation. Thus, only the high-order terms are handled in software. Notice that quadratic interpolation avoids the faceted appearance produced by linear interpolation and provides closer results to the path tracing reference (Figure 15). 

Our adaptive quadratic interpolation both reduces the edge storage and the computation in the fragment shader. Figure 15 shows that using **D** min = 0 _._ 015, edge storage is eliminated in relatively flat surface regions such that only 25% of the edge storage remains and the quality is almost identical to full quadratic interpolation. This makes the total edge and vertex storage for adaptive quadratic interpolation less than twice the vertex storage for linear interpolation. This is a significant reduction compared to non-adaptive quadratic interpolation that requires more than four times total storage than linear interpolation. Even only testing the performance on the current hardware, skipping reading and computing quadratic terms adaptively in the fragment shader translates to actual performance improvement. The shader execution time on the GPU is reduced from 0.30 ms to 0.19 ms when using adaptive interpolation, making it closer to the 0.06 ms required by linear interpolation. With the proposed hardware support, edge colors can be computed in geometry shader and the fragment color would be automatically interpolated. Without clamping, this would double the three-multiplier passes required by linear interpolation. With **D** min = 0 _._ 015, only 1.43× more passes are required (since only 43% of the triangles have non-zero edge storages). 

## **7.5. Embedded Deformation** 

FEM simulation on high-poly tetrahedral mesh (tet-mesh) can be expensive. A popular way to overcome this problem is _embedded deformation_ that simulates on a low-poly tet-mesh and interpolates the deformation to produce the vertex position for the high-poly triangular mesh that is used for rendering. 



<!-- Start of picture text -->
Quadratic interpolation<br><!-- End of picture text -->

Linear interpolation 

**Figure 16:** _A comparison of linear interpolation and our quadratic interpolation for embedded deformation of a sphere. Edge mid points generated using vertex gradients computed using the method in Phong Deformation [Jam20]._ 

Linear interpolation with embedded deformation produces undesirable faceted artifacts when the deformation is large, as can be seen in Figure 16. Phong deformation [Jam20] solves this problem by computing per-vertex gradients from neighboring tetrahedra and blending them with per-tetrahedron gradients used for linear interpolation to provide a result with a reduced truncation error than linear interpolation. 

Figure 17 shows a more complex example. Our quadratic tetrahedral interpolation ▲ **Q** 10<sup>3DproducesidenticalresultstoPhongde-</sup> formation, but requires more storage, as shown in Table 1. Phong deformation stores a 3×3 matrix (vertex gradient) at each vertex, while ▲ **Q** 10<sup>3Dstores 3D vector positions on each edge of the tet mesh.</sup> Although each vertex stores three times more data than each edge, a tetrahedral mesh can have 7× as many edges as vertices, requiring 

Computer Graphics Forum © 2021 The Eurographics Association and John Wiley & Sons Ltd. 

© 2021 The Author(s) 



<!-- Start of picture text -->
D. Lin & L. Seiler & C. Yuksel / Hardware Adaptive High-Order Interpolation for Real-Time Graphics<br>Our Quadratic Tetrahderal mesh D min = 0 . 0015 Frame 0 Frame 24 Frame 49 Frame 74<br>Interpolation (▲ Q 10 3D) edge storage heat map<br>Linear ▲ Q 10 3D , D min = 0 . 0015 ▲ Q 10 3D ,  No Clamp<br>MSE: 1 . 5 × 10 −5 MSE: 2 . 8 × 10 −7 MSE: 0 (reference)<br><!-- End of picture text -->



<!-- Start of picture text -->
Frame 0 Frame 24 Frame 49 Frame 74 Frame 99<br><!-- End of picture text -->





**Figure 18:** _A visualization of the entire deformation of 100 frames. The tet-meshes are generated from FEM simulation in Houdini 18.5. Note that less edge data are clamped as the amount of deformation increases (Frame 0 is the rest pose). The color legend is the same as in Figure 17._ 







**Table 1:** _Average shader execution times and total storage cost of interpolation methods for embedded deformation shown in Figure 18._ 







|**Method**|**Time**|**Storage**|
|---|---|---|
|Linear Interpolation<br>Our ▲**Q**<sup>3D</sup><br>10 <sup>(No clamp)</sup><br>|109_µ_s<br>141_µ_s|2.0 MB<br>12.7 MB|
|Our ▲**Q**<sup>3D</sup><br>10 <sup>(</sup><sup>**D**</sup>min <sup>= 0</sup><sup>_._0015)</sup>|147_µ_s|9.8 MB|
|Phong Deformation|166_µ_s|7.8 MB|









done at each frame if the deformation is simulated on the fly. This introduces an additional step, as compared to Phong deformation. In our evaluation, however, we do not consider the additional precomputation time and only compare the shader execution time with precomputed values for the entire sequence. Since we have shorter vertex shader execution time, we expect that when the embedded triangular mesh is much more complex than the enclosing tet-mesh, the vertex shader cost will be much higher than computing and storing the edge data for the tet-mesh, making our method faster for the on-the-fly deformation. 









<!-- Start of picture text -->
140<br>120<br>100<br>80<br>60<br>40<br>20<br>0<br>0 10 20 30 40 50  60 70 80 90<br>Frame ID<br>Ours [James 2020] Ours (Adaptive) Linear Ours (Adaptive,<br>edge data only)<br>Storage (KB)<br><!-- End of picture text -->

**Figure 17:** _Embedded deformation of a mesh (69664 vertices) using a tet-mesh with only 1654 vertices. We compare the quality of linear tetrahedral interpolation to our quadratic interpolation with and without clamping. Notice that quadratic interpolation reduces the faceted appearance (looks like darkend "dents" under lighting). The third and fifth rows: 4_ × _differences w.r.t to No Clamp. The heat map visualizes the density of edge storage (number of edges in the corresponding tetrahedra that are not clamped)._ 

more storage when using ▲ **Q** 10<sup>3D[Jam20].However,weshowthat</sup> using ▲ **Q** 10<sup>3D, the shader execution time for embedded deformation</sup> can be reduced, as compared to Phong deformation. 

**Figure 19:** _Per-frame storage of different interpolation methods for the test scene in Figure 18. Ours (Adaptive) uses_ **D** min = 0 _._ 0015 _._ 

Frames from an animation sequence are shown in Figure 18. In our implementation, we precompute and store the **D** -terms on edges for a pre-generated deformation sequence of the enclosing tet-mesh and invoke a vertex shader each frame to deform the embedded triangular mesh. Such edge value precomputation would need to be 

Figure 19 shows the storage costs of different methods for the sequence in Figure 18. As expected, our quadratic interpolation with no clamping requires more storage than Phong deformation on average. We can reduce the storage using adaptive interpolation 

© 2021 The Author(s) Computer Graphics Forum © 2021 The Eurographics Association and John Wiley & Sons Ltd. 

_D. Lin & L. Seiler & C. Yuksel / Hardware Adaptive High-Order Interpolation for Real-Time Graphics_ 

by setting **D** min = 0 _._ 0015 to obtain a result almost identical to no clamping. This reduces the storage by more than 50% in the best case, though we still require more storage than Phong deformation. In Figure 19, we also include a dashed line that shows the storage of only the edge data of our method. Notice that at Frame 0, no edge data is required, because all **D** -terms are zero. Adaptive interpolation also needs an additional buffer to keep the offsets in the compressed edge data for each edge. This results in additional storage and shader execution times. 



<!-- Start of picture text -->
1.6E-5<br>1.2E-5<br>8.0E-6<br>4.0E-6<br>0<br>0 10 20 30 40  50 60 70 80 90<br>Frame ID<br>Linear Ours (Adaptive)<br>MSE<br><!-- End of picture text -->

**Figure 20:** _A comparison between linear interpolation and our adaptive quadratic interpolation using_ **D** min _in terms of mean square error (MSE) of the triangular mesh vertex positions with respect to the full quadratic interpolation positionin all 100 frames. The test scene is shown in Figure 18._ 

The interpolation errors for linear and our adaptive interpolation for the animation squence in Figure 18 are shown in Figure 20. In comparison to linear interpolation, which shows increasingly larger error as the deformation progresses, our quadratic adaptive interpolation shows a stable amount of error that is also much smaller than the average error of linear interpolation. 

## **8. Conclusions** 

We have presented high-order interpolation for grids and simplexes. We have introduced a unified, computationally-efficient mathematical formulation which adds higher-order terms on top of linear interpolation to achieve quadratic and cubic interpolation. Our formulation allows adaptively skipping computation of highorder terms. We propose relatively minor modifications to existing graphics hardware to support our adaptive high-order interpolation. Our proposed hardware implementation can bring the cost of adaptive high-order interpolation close to linear interpolation, dramatically improving the visual quality of a wide range of real-time graphics applications at low cost. 

## **References** 

- [ASS*01] ABBAS, AM, SZIRMAY-KALOS, L, SZIJARTO, G, et al. “Quadratic interpolation in hardware rendering”. _Spring Conference of Computer Graphics_ . 2001 3. 

- [BA08] BOUBEKEUR, TAMY and ALEXA, MARC. “Phong Tessellation”. _ACM Trans. Graph._ 27.5 (Dec. 2008). ISSN: 0730-0301 3. 

- [BBGB20] BOUKHTACHE, SEYFEDDINE, BLAYSAT, BENOIT, GRÉDIAC, MICHEL, and BERRY, FRANCOIS. “Alternatives to Bicubic Interpolation Considering FPGA Hardware Resource Consumption”. _IEEE Transactions on Very Large Scale Integration (VLSI) Systems_ (2020) 2. 

- [BC14] BARGTEIL, ADAM W and COHEN, ELAINE. “Animation of deformable bodies with quadratic Bézier finite elements”. _ACM Transactions on Graphics (TOG)_ 33.3 (2014), 1–10 3. 

- [Bjo04] BJORKE, KEVIN. “High-quality filtering”. _GPU Gems_ 2 (2004), 391–415 2. 

- [Bro99] BROWN, RUSS. _Modeling specular highlights using Bézier triangles_ . Sun Microsystems, Inc., 1999 2. 

- [Csé18] CSÉBFALVI, BALÁZS. “Fast Catmull-Rom Spline Interpolation for High-Quality Texture Sampling”. _Computer Graphics Forum_ . Vol. 37. 2. Wiley Online Library. 2018, 455–462 1, 2, 10, 11. 

- [Csé19] CSÉBFALVI, BALÁZS. “Beyond trilinear interpolation: higher quality for free”. _ACM Transactions on Graphics (TOG)_ 38.4 (2019), 1– 8 2, 11. 

- [Far93] FARIN, GERALD. “Chapter 18 - Bézier Triangles”. _Curves and Surfaces for Computer-Aided Geometric Design (Third Edition)_ . Ed. by FARIN, GERALD. Third Edition. Boston: Academic Press, 1993, 321– 351. ISBN: 978-0-12-249052-1 2. 

- [GNSS14] GOUR, PRANAV NARAYAN, NARUMANCHI, SUJAY, SAURAV, SUMEET, and SINGH, SANJAY. “Hardware accelerator for real-time image resizing”. _18th International Symposium on VLSI Design and Test_ . IEEE. 2014, 1–6 2. 

- [Jam20] JAMES, DOUG L. “Phong deformation: a better C 0 interpolant for embedded deformation”. _ACM Transactions on Graphics (TOG)_ 39.4 (2020), 56–1 3, 13, 14. 

- [Kar14] KARIS, BRIAN. “High Quality Temporal Supersampling”. _ACM SIGGRAPH Courses: Advances in Real-Time Rendering in Games (2014)_ . SIGGRAPH ’14. New York, NY, USA: Association for Computing Machinery, 2014 11. 

- [Key81] KEYS, ROBERT. “Cubic convolution interpolation for digital image processing”. _IEEE transactions on acoustics, speech, and signal processing_ 29.6 (1981), 1153–1160 2. 

- [KNPH95] KASSON, JAMES M, NIN, SIGFREDO I, PLOUFFE, WIL, and HAFNER, JAMES LEE. “Performing color space conversions with threedimensional linear interpolation”. _Journal of Electronic Imaging_ 4.3 (1995), 226–250 8. 

- [LSC*08] LIN, CHUNG-CHI, SHEU, MING-HWA, CHIANG, HUANNKENG, et al. “The efficient VLSI design of BI-CUBIC convolution interpolation for digital image processing”. _2008 IEEE International Symposium on Circuits and Systems_ . IEEE. 2008, 480–483 2. 

- [LSC*10] LIN, CHUNG-CHI, SHEU, MING-HWA, CHIANG, HUANNKENG, et al. “An Efficient Architecture of Extended Linear Interpolation for Image Processing.” _J. Inf. Sci. Eng._ 26.2 (2010), 631–648 2. 

- [Map06] MAPLESON, IAN. _Infinite Reality Technical Report_ . 2006. URL: http://www.sgidepot.co.uk/onyx/IR_techreport. pdf 2. 

- [MBDM97] MONTRYM, JOHN S, BAUM, DANIEL R, DIGNAM, DAVID L, and MIGDAL, CHRISTOPHER J. “InfiniteReality: A real-time graphics system”. _Proceedings of the 24th annual conference on Computer graphics and interactive techniques_ . 1997, 293–302 2. 

- [MDSB03] MEYER, MARK, DESBRUN, MATHIEU, SCHRÖDER, PETER, and BARR, ALAN H. “Discrete differential-geometry operators for triangulated 2-manifolds”. _Visualization and mathematics III_ . Springer, 2003, 35–57 8. 

- [MH15] MAHAJAN, SHRUTI H and HARPALE, VARSHA K. “Adaptive and non-adaptive image interpolation techniques”. _2015 International Conference on Computing Communication Control and Automation_ . IEEE. 2015, 772–775 2. 

- [ML94] MARSCHNER, STEPHEN R and LOBB, RICHARD J. “An evaluation of reconstruction filters for volume rendering”. _Proceedings Visualization’94_ . IEEE. 1994, 100–107 2. 

> © 2021 The Author(s) 

Computer Graphics Forum © 2021 The Eurographics Association and John Wiley & Sons Ltd. 

_D. Lin & L. Seiler & C. Yuksel / Hardware Adaptive High-Order Interpolation for Real-Time Graphics_ 

- [MMMY97] MOLLER, TORSTEN, MACHIRAJU, RAGHU, MUELLER, KLAUS, and YAGEL, RONI. “Evaluation and design of filters using a Taylor series expansion”. _IEEE transactions on Visualization and Computer Graphics_ 3.2 (1997), 184–199 2. 

- [MN88] MITCHELL, DON P and NETRAVALI, ARUN N. “Reconstruction filters in computer-graphics”. _ACM Siggraph Computer Graphics_ 22.4 (1988), 221–228 2. 

- [MSY19] MALLETT, IAN, SEILER, LARRY, and YUKSEL, CEM. “Patch textures: Hardware implementation of mesh colors”. (2019) 6, 8. 

- [NA05] NUÑO-MAGANDA, MARCO AURELIO and ARIAS-ESTRADA, MIGUEL O. “Real-time FPGA-based architecture for bicubic interpolation: an application for digital image scaling”. _2005 International Conference on Reconfigurable Computing and FPGAs (ReConFig’05)_ . IEEE. 2005, 8–pp 2. 

- [Pho75] PHONG, BUI TUONG. “Illumination for Computer Generated Pictures”. _Communications of the ACM_ 18.6 (June 1975), 311–317. ISSN: 0001-0782 2. 

- [Sei98] SEILER, LARRY. “Quadratic interpolation for near-Phong quality shading”. _ACM SIGGRAPH 98 Conference Abstracts and Applications_ . 1998, 268 3. 

- [SH05] SIGG, CHRISTIAN and HADWIGER, MARKUS. “Fast third-order texture filtering”. _GPU gems_ 2 (2005), 313–329 2. 

- [SKH16] SANAULLAH, AHMED, KHOSHPARVAR, ARASH, and HERBORDT, MARTIN C. “FPGA-Accelerated Particle-Grid Mapping”. _2016 IEEE 24th Annual International Symposium on Field-Programmable Custom Computing Machines (FCCM)_ . IEEE. 2016, 192–195 2. 

- [SKS02] SLOAN, PETER-PIKE, KAUTZ, JAN, and SNYDER, JOHN. “Precomputed radiance transfer for real-time rendering in dynamic, lowfrequency lighting environments”. _Proceedings of the 29th annual conference on Computer graphics and interactive techniques_ . 2002, 527– 536 12. 

- [VPBM01] VLACHOS, ALEX, PETERS, JÖRG, BOYD, CHAS, and MITCHELL, JASON L. “Curved PN triangles”. _Proceedings of the 2001 symposium on Interactive 3D graphics_ . 2001, 159–166 3, 8. 

- [WDLY11] WANG, XIANG, DING, YONG, LIU, MING-YU, and YAN, XIAO-LANG. “Efficient implementation of a cubic-convolution based image scaling engine”. _Journal of Zhejiang University SCIENCE C_ 12.9 (2011), 743–753 2. 

- [YKH10] YUKSEL, CEM, KEYSER, JOHN, and HOUSE, DONALD H. “Mesh colors”. _ACM Transactions on Graphics_ 29.2 (2010), 15:1–15:11. ISSN: 0730-0301 8. 

- [YLS20] YANG, LEI, LIU, SHIQIU, and SALVI, MARCO. “A survey of temporal antialiasing techniques”. _Computer Graphics Forum_ . Vol. 39. 2. Wiley Online Library. 2020, 607–621 2, 11. 

Note that □ **C**<sup>2D</sup> 12<sup>does not need the four corner texels of the 4×4</sup> block, so a small amount of power can be saved by not reading from those banks. 4 × 4 banks are still required since the texels emerge from the banks in rotated order, as described at the end of Section 4.2. Therefore, the unused texels could come from any of the 16 banks, depending on the alignment of the 4 × 4 block. 

Next, consider 3D cubic interpolations □ **C**<sup>3D</sup> 32<sup>and</sup> ■<sup>**C**3D</sup> 64<sup>. They re-</sup> quire reading texels in an unaligned 4 × 4 × 4 block (though □ **C**<sup>3D</sup> 32 does not use the corner values). However, if all **D** -terms are below **D** min and the filter operation reduces to trilinear interpolation, they require two BOP cycles to process 2 × 2 × 2 texels. Therefore, it is only necessary to be able to read 4 × 4 × 2 texels in parallel on each of two clocks to allow cubic cuboid interpolation to perform at the same speed as trilinear interpolation. 

Biquadratic and triquadratic filtering are also supported using unaligned 4 × 4 or 4 × 4 × 4 arrays of texels. The mid-edge values are computed from the larger array. So, although the quadratic filtering modes require fewer **D** -terms, they require accessing the same number of texels. 

Therefore, to accommodate peak performance we must access up to 32 texels per clock from the texel L1 cache. The left side of Figure 4 illustrates this. The 32 banks are interleaved based on the low order two bits of each of the _U_ and _V_ indices and the low order bit of the slice number for the 3D texel array. Then, two accesses provide the unaligned 4 × 4 × 4 of texels needed to perform tricubic interpolation. 

It is not necessary for an implementation to support 32 banks of texel L1 cache in order to support tricubic interpolation. E.g. if only 16 banks were supported, tricubic interpolation would require a minimum of four cycles instead of two. This would also reduce the cost of computing the **D** -terms, since only one quarter of them would need to be computed per clock. 

Similarly, an implementation could choose to provide only 8 banks. This would cause bicubic interpolation to take a minimum of two cycles because it takes two accesses to read an unaligned 4 × 4 of texels from the texel L1 cache. Therefore, the cost of increasing the number of texel L1 cache banks can be traded off against the desired performance for higher order filtering. 

- [Yuk17] YUKSEL, CEM. “Mesh color textures”. _Proceedings of High Performance Graphics_ . 2017, 1–11 8. 

- [ZLZ*10] ZHANG, YUNSHAN, LI, YUHUI, ZHEN, JIE, et al. “The hardware realization of the bicubic interpolation enlargement algorithm based on FPGA”. _2010 Third International Symposium on Information Processing_ . IEEE. 2010, 277–281 2. 

- [ZTZ05] ZIENKIEWICZ, OLEK C, TAYLOR, ROBERT L, and ZHU, JIAN Z. _The finite element method: its basis and fundamentals_ . Elsevier, 2005 2, 3. 

## **Appendix A:** L1 Cache Banking for High Order Filtering 

Consider 2D cubic interpolations □ **C**<sup>2D</sup> 12<sup>and</sup> ■<sup>**C**2D</sup> 16<sup>.Theyrequire</sup> reading texels in an unaligned 4 × 4 block in parallel so that _all_ **D** -terms can be computed in one clock cycle. That in turn permits the filter operation to be completed in one clock if all **D** -terms are below the threshold **D** min, so that the operation reduces to linear interpolation. 

© 2021 The Author(s) Computer Graphics Forum © 2021 The Eurographics Association and John Wiley & Sons Ltd. 

