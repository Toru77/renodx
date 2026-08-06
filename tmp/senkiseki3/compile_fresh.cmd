@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
cd /d e:\RenoDX\renodx\tmp\senkiseki3
fxc /T vs_4_1 /E main /Fc fresh_face.asm /Fo fresh_face.cso "e:\RenoDX\renodx\src\games\senkiseki-dlaa\senkiseki3\boot\chr_0x0D5DABC6.vs_4_1.hlsl" 2>&1 & echo EXIT=%ERRORLEVEL%
