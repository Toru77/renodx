@echo off
rem Windows (MSVC) build of one harness test, with AddressSanitizer.
rem   build_win.bat test_pool            -> test_pool.exe
rem Needs Visual Studio 2022 with the C++ toolset (vcvars64.bat).
setlocal
set "NAME=%~1"
if "%NAME%"=="" (echo usage: build_win.bat ^<test^> & exit /b 2)
set "HERE=%~dp0"
set "VC=%ProgramFiles%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
if not exist "%VC%" set "VC=%ProgramFiles%\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat"
call "%VC%" >nul || exit /b 1
pushd "%HERE%"
set "REPO=%HERE%..\..\"
cl /nologo /std:c++20 /EHsc /DNOMINMAX /DWIN32_LEAN_AND_MEAN /Od /Zi /MD /fsanitize=address /W1 /wd4996 ^
  /I. /Iinc /I"%REPO%external\reshade\include" ^
  /DFALCOM_BYTECODE_DIR=\"../bytecode/\" /DFALCOM_WINDOWS_HARNESS ^
  %NAME%.cpp /Fe:%NAME%.exe /Fo:%NAME%.obj /Fd:%NAME%.pdb
set RC=%ERRORLEVEL%
rem ASan runtime DLL must sit next to the executable.
copy /Y "%VCToolsInstallDir%bin\Hostx64\x64\clang_rt.asan_dynamic-x86_64.dll" . >nul
popd
exit /b %RC%



