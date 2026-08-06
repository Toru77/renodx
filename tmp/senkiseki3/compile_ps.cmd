@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
cd /d e:\RenoDX\renodx\tmp\senkiseki3
fxc /T ps_4_1 /E main /Fo fresh_ps.cso "e:\RenoDX\renodx\src\games\senkiseki-dlaa\senkiseki3\boot\0xFEA2B509.ps_4_1.hlsl" 2>&1 | findstr /i "error"
