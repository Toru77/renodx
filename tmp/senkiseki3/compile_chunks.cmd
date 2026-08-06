@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
cd /d e:\RenoDX\renodx\tmp\senkiseki3
cl /nologo /W3 /EHsc /std:c++17 /I..\..\src chunks.cpp /Fe:chunks.exe
