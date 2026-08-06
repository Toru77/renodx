@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
cd /d e:\RenoDX\renodx\tmp\senkiseki3
fxc /dumpbin 0x0D5DABC6.nooutput.cso > 0x0D5DABC6.nooutput.dump.txt 2>&1 & echo NOOUTPUT_EXIT=%ERRORLEVEL%
fxc /dumpbin 0x0D5DABC6.constant.cso > 0x0D5DABC6.constant.dump.txt 2>&1 & echo CONSTANT_EXIT=%ERRORLEVEL%
