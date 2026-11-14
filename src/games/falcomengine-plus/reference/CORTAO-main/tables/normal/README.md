<h1>CORTAO bent normal lookup tables</h1>

This folder contains pre-generated 1D lookup tables meant to be used with the CORTAO main pass to optimize the bent normal calculation, which are used when CORTAO_USE_BENT_NORMAL_LOOKUP_TABLE is set to 1. The occluder lookup table improves the performance of the main pass by precomputing the bent normal in tangent space for all combinations of occluded rays. As the lookup table needs 2^rayCount entries, it is only feasible for a ray count of 8 or 16; there is no lookup table for 32 rays.

The precomputed bent normals are stored as 8-bit signed normalized values. As the bent normal in tangent space always has a positive z-value, only X and Y are stored as 2-byte R8G8_SNORM entries, with Z expected to be reconstructed as ``sqrt(max(0.0, 1 - x\*x - y\*y))`` in the shader after reading from the table. The table is ideally stored as a typed buffer / texel buffer, to automatically decode the R8G8_SNORM into floats, but the data can also be manually converted to floats by converting the raw signed bytes to floats and then dividing by 127.0.

The precomputed lookup table data is available in C header files, each containing a single variable: CORTAO::bentNormalLookupTableXRays, where X is the ray count.

