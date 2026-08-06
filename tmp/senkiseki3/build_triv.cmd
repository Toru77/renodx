@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
cd /d e:\RenoDX\renodx\tmp\senkiseki3
echo float4 main() : SV_TARGET { return 0; } > trivial.hlsl
fxc /T ps_4_1 /E main /Fo trivial_ps.cso trivial.hlsl 2>&1 | findstr /i "error"
echo trivial PS built
