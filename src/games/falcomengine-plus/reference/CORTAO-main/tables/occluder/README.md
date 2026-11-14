<h1>CORTAO occluder lookup tables</h1>

This folder contains pre-generated 3D lookup tables meant to be used with the CORTAO main pass to optimize occluder raytracing, which are used when CORTAO_USE_OCCLUDER_LOOKUP_TABLE is set to 1 and makes CORTAO's performance competitive with existing techniques. The occluder lookup table improves the performance of the main pass by precomputing which rays are blocked by an occluder with various tangent-space directions and radii.

In almost all cases, only the following lookup tables are recommended:

- 8 rays: 24x24x16x8_R8_UINT
- 16 rays: 24x24x16x16_R16_UINT
- 32 rays: 24x24x16x32_R32_UINT

Note that the ray count in the lookup table MUST match the ray count specified in the main pass using the CORTAO_RAY_COUNT define.

Higher resolution lookup tables are only useful for very high sample counts (>32, not to be confused with ray count) and are therefore rarely needed.

<h2>Shader Model 6.7 workaround</h2>

For HLSL, these lookup tables are intended to be sampled with SampleLevel() when used in the CORTAO main pass. While sampling integer texture formats is supported in GLSL, this was only added with Shader Model 6.7. If your workflow doesn't make it possible to update to SM 6.7, as a workaround R8_UNORM and R16_UNORM versions of the lookup table are also available, which can be sampled as a floating point value:

- 8 rays: Use an R8_UNORM lookup table, convert to uint as uint(value \* 255.0 + 0.5).
- 16 rays: Use an R16_UNORM lookup table, convert to uint as uint(value \* 65535.0 + 0.5).
- 32 rays: No great workaround exists. Do NOT create an SFLOAT view of an R32_UINT lookup table, as some GPU texture units set the mantissa bits of NaNs to zero and/or denormal values to zero, even when using NEAREST filtering. It is recommended to use 16 rays in this case instead.

<h2>File explanation</h2>

The naming format of these files contains the resolution, ray count and format of the file, e.g.
- 24x24x16x16_R16_UINT: a 24x24x16 lookup table for 16 rays, in the R16_UINT format.

Each lookup table is available as both a raw binary texture data .bin file, containing only texture data that can be dumped directly to a 3D texture, and a standard .dds file, which also contains meta data such as resolution and format in addition to the same texture data.

