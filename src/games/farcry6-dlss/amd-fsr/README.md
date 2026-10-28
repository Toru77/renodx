# AMD FSR SDK headers

These headers come from AMD FidelityFX SDK 2.0.0, tag `v2.0.0`:
https://github.com/GPUOpen-LibrariesAndSDKs/FidelityFX-SDK/tree/v2.0.0/Kits/FidelityFX

Only trailing whitespace has been removed. They are distributed under the MIT license included in each header. The addon
loads `amd_fidelityfx_loader_dx12.dll` from the Far Cry 6 executable directory.
Place AMD's matching `amd_fidelityfx_upscaler_dx12.dll` in the same directory.
The SDK 2.0 runtime selects FSR 3.1.5 on supported non-FSR-4 hardware and may
select FSR 4 on supported AMD hardware. Check Copy Report for the provider.

This integration supplies upscaling only. It does not enable frame generation.
