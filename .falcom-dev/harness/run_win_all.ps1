$h = "E:\RenoDX\renodx\.falcom-dev\harness"
$tests = "test_indirect","test_switches","test_visibility","test_live","test_build","test_verify","test_deform","test_deform_live","test_motion"
Set-Location $h
foreach ($t in $tests) {
  $out = & cmd.exe /c "`"$h\build_win.bat`" $t" 2>&1
  ($out -join "`n") | Out-File -Encoding utf8 "$t.win_build.log"
  if ($LASTEXITCODE -ne 0) { "$t BUILD FAILED (see $t.win_build.log)" | Out-File -Append -Encoding utf8 win_results.txt; continue }
  $run = & ".\$t.exe" 2>&1
  $rc = $LASTEXITCODE
  ($run -join "`n") | Out-File -Encoding utf8 "$t.win_run.log"
  "$t rc=$rc last=$(($run | Select-Object -Last 1))" | Out-File -Append -Encoding utf8 win_results.txt
}
"DONE" | Out-File -Append -Encoding utf8 win_results.txt
