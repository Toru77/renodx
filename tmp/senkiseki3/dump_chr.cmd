@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
cd /d e:\RenoDX\renodx\src\games\senkiseki-dlaa\senkiseki3\boot
fxc /dumpbin chr_0x0D5DABC6.vs_4_1.cso > chr_0x0D5DABC6.vs_4_1.dump.txt 2>&1
echo EXIT=%ERRORLEVEL%
