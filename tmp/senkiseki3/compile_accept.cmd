@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
cd /d e:\RenoDX\renodx\tmp\senkiseki3
cl /nologo /EHsc /O2 shader_accept.cpp /Fe:shader_accept.exe /link d3d11.lib 2>&1 | findstr /i "error"
echo BUILD_DONE
