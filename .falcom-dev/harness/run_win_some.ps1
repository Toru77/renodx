$h = "E:\RenoDX\renodx\.falcom-dev\harness"
Set-Location $h
$tests = $args
foreach ($t in $tests) {
  $out = & cmd.exe /c "`"$h\build_win.bat`" $t" 2>&1
  ($out -join "`n") | Out-File -Encoding utf8 "$t.win_build.log"
  if ($LASTEXITCODE -ne 0) { "$t BUILD FAILED (see $t.win_build.log)"; continue }
  $run = & ".\$t.exe" 2>&1
  ($run -join "`n") | Out-File -Encoding utf8 "$t.win_run.log"
  "$t rc=$LASTEXITCODE last=$(($run | Select-Object -Last 1))"
}
