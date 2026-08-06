@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
cd /d e:\RenoDX\renodx\tmp\senkiseki3
fxc /dumpbin f3_full.cso > f3full.dump.txt 2>&1
echo DUMP_EXIT=%ERRORLEVEL%
