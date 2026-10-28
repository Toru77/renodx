# Streamline headers for Far Cry 6

The headers in `include` are the unmodified NVIDIA Streamline SDK 2.14.16
headers needed for DLSS Frame Generation, Reflex, manual hooks and signature
verification. They are distributed under the accompanying `LICENSE.txt`.
Source: https://github.com/NVIDIA-RTX/Streamline/releases/tag/v2.14.16

The addon verifies and loads `sl.interposer.dll` from the directory containing
`FarCry6.exe` before D3D12 device creation. Copy Report shows the load result.
Loading the interposer does not initialize Streamline or enable Frame Generation;
the swapchain, frame inputs, Reflex and DLSS-G integration remain to be implemented.
The production SDK provides `sl.interposer.dll`,
`sl.common.dll`, `sl.dlss_g.dll`, `nvngx_dlssg.dll`, `sl.reflex.dll` and
`sl.pcl.dll` for that integration.
