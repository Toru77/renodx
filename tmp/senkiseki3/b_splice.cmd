@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
cd /d e:\RenoDX\renodx\tmp\senkiseki3
cl /nologo /std:c++17 /EHsc /O2 /I e:\RenoDX\renodx\src\games\senkiseki-dlaa splice_inject.cpp /Fe:splice_inject.exe 2>&1 | findstr /i "error"
