@echo off
rem Windows (MSVC) build of one harness test.
rem   build_win.bat test_pool         -> test_pool.exe       (AddressSanitizer, /O2)
rem   build_win.bat test_pool fast    -> test_pool_fast.exe  (/O2, no sanitizer)
rem Needs Visual Studio 2022 with the C++ toolset (vcvars64.bat).
setlocal
set "NAME=%~1"
if "%NAME%"=="" (echo usage: build_win.bat ^<test^> [fast] & exit /b 2)
set "HERE=%~dp0"
set "VC=%ProgramFiles%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
if not exist "%VC%" set "VC=%ProgramFiles%\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat"
call "%VC%" >nul || exit /b 1
pushd "%HERE%"
set "REPO=%HERE%..\..\"
set "OUT=%NAME%"
set "ASAN=/fsanitize=address"
if /i "%~2"=="fast" (
  set "OUT=%NAME%_fast"
  set "ASAN="
)
cl /nologo /std:c++20 /EHsc /DNOMINMAX /DWIN32_LEAN_AND_MEAN /O2 /Zi /MD /FS %ASAN% /W1 /wd4996 ^
  /I. /Iinc /I"%REPO%external\reshade\include" ^
  /DFALCOM_BYTECODE_DIR=\"../bytecode/\" /DFALCOM_WINDOWS_HARNESS ^
  %NAME%.cpp /Fe:%OUT%.exe /Fo:%OUT%.obj /Fd:%OUT%.pdb
set RC=%ERRORLEVEL%
rem ASan runtime DLL must sit next to the executable; copied once.
if not exist clang_rt.asan_dynamic-x86_64.dll copy /Y "%VCToolsInstallDir%bin\Hostx64\x64\clang_rt.asan_dynamic-x86_64.dll" . >nul
popd
exit /b %RC%
