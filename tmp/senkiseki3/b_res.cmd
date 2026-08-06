@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
cd /d e:\RenoDX\renodx\tmp\senkiseki3
cl /nologo /std:c++17 /EHsc /O2 /I e:\RenoDX\renodx\src\games\senkiseki-dlaa reserialize.cpp /Fe:reserialize.exe 2>&1 | findstr /i "error"
if exist reserialize.exe (
  reserialize.exe original.cso t_identity.cso
  reserialize.exe t_noout.cso t_identity_noout.cso
  draw_repro.exe t_identity.cso fresh_ps.cso
)
