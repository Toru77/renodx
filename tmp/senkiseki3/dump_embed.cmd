@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
cd /d e:\RenoDX\renodx\build\senkiseki-dlaa.include\embed
fxc /dumpbin 0x0D5DABC6.cso > 0x0D5DABC6.dump.txt 2>&1 & echo EXIT=%ERRORLEVEL%
