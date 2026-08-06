@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
cd /d e:\RenoDX\renodx\tmp\senkiseki3
fxc /dumpbin s1_bt.cso > s1bt.dump.txt 2>&1
echo EXIT=%ERRORLEVEL%
