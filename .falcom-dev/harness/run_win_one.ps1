# Build (only when stale), run and time one harness test on Windows.
#   powershell -File run_win_one.ps1 -Test test_pool [-Quick] [-Force]
# -Quick builds <test>_fast.exe (no ASan, /O2) and sets FALCOM_QUICK=1. It is not a full verification.
# -Force rebuilds even when the exe is newer than every input.
# The last line printed is RESULT test=... for run_win_some.ps1 to parse.
param(
  [Parameter(Mandatory = $true)][string]$Test,
  [switch]$Quick,
  [switch]$Force
)
$h = "E:\RenoDX\renodx\.falcom-dev\harness"
Set-Location $h
& powershell -NoProfile -ExecutionPolicy Bypass -File "$h\refresh_world.ps1" | Out-Null
$exeName = if ($Quick) { "$Test`_fast.exe" } else { "$Test.exe" }
$exePath = Join-Path $h $exeName

$inputs = @((Join-Path $h "$Test.cpp"), (Join-Path $h "build_win.bat"), (Join-Path $h "prelude.h"))
$inputs += Get-ChildItem -Path $h -File | Where-Object { $_.Extension -eq ".hpp" -or $_.Extension -eq ".h" } | ForEach-Object { $_.FullName }
foreach ($dir in @("gen", "inc", "src")) {
  $inputs += Get-ChildItem -Path (Join-Path $h $dir) -Recurse -File | ForEach-Object { $_.FullName }
}
$newestInput = ($inputs | ForEach-Object { (Get-Item $_).LastWriteTime } | Measure-Object -Maximum).Maximum
$stale = (-not (Test-Path $exePath)) -or ((Get-Item $exePath).LastWriteTime -lt $newestInput)

$buildSeconds = 0.0
if ($stale -or $Force) {
  $buildArg = if ($Quick) { "$Test fast" } else { $Test }
  $sw = [System.Diagnostics.Stopwatch]::StartNew()
  $out = & cmd.exe /c "`"$h\build_win.bat`" $buildArg" 2>&1
  $buildSeconds = $sw.Elapsed.TotalSeconds
  ($out -join "`n") | Out-File -Encoding utf8 "$Test.win_build.log"
  if ($LASTEXITCODE -ne 0) {
    "RESULT test=$Test build_s=$([math]::Round($buildSeconds, 1)) run_s=0.0 rc=build slowest_s=0.00 stage=`"build failed: $Test.win_build.log`""
    exit 1
  }
}

$env:FALCOM_TEST_OUT = $Test
if ($Quick) { $env:FALCOM_QUICK = "1" } else { Remove-Item Env:FALCOM_QUICK -ErrorAction SilentlyContinue }
$sw = [System.Diagnostics.Stopwatch]::StartNew()
$run = & $exePath 2>&1
$rc = $LASTEXITCODE
$runSeconds = $sw.Elapsed.TotalSeconds
($run -join "`n") | Out-File -Encoding utf8 "$Test.win_run.log"

$timing = $run | Where-Object { "$_" -like "TIMING test=*" } | Select-Object -Last 1
$slowestS = "0.00"
$stage = "none"
if ("$timing" -match 'slowest=([0-9.]+) "(.*)"$') { $slowestS = $matches[1]; $stage = $matches[2] }
$last = $run | Where-Object { "$_" -notlike "TIMING*" -and "$_" -notlike "slow *" } | Select-Object -Last 1
"RESULT test=$Test build_s=$([math]::Round($buildSeconds, 1)) run_s=$([math]::Round($runSeconds, 1)) rc=$rc slowest_s=$slowestS stage=`"$stage`" last=`"$last`""
exit $rc
