@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
cd /d e:\RenoDX\renodx\tmp\senkiseki3
fxc /dumpbin fx_full3.cso > fx_full3.dump.txt 2>&1
echo EXIT=%ERRORLEVEL%
