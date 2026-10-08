# Runs harness tests through run_win_one.ps1, up to -Jobs at a time, longest (test_live) first.
#   powershell -File run_win_some.ps1 -Tests test_live,test_motion [-Jobs 4] [-Quick] [-Force]
#   powershell -File run_win_some.ps1 -All
# Refreshes the world copy once, then prints a table: build s, run s, rc, slowest stage.
param(
  [string[]]$Tests = @(),
  [int]$Jobs = 4,
  [switch]$Quick,
  [switch]$Force,
  [switch]$All
)
$h = "E:\RenoDX\renodx\.falcom-dev\harness"
Set-Location $h
$everyTest = @("test_live", "test_motion", "test_pool", "test_switches", "test_visibility", "test_verify", "test_build", "test_indirect", "test_deform", "test_deform_live")
if ($All) { $Tests = $everyTest }
$Tests = @($Tests | Where-Object { $_ -eq "test_live" }) + @($Tests | Where-Object { $_ -ne "test_live" })

& powershell -NoProfile -ExecutionPolicy Bypass -File "$h\refresh_world.ps1"

$queue = New-Object System.Collections.Queue
foreach ($t in $Tests) { $queue.Enqueue($t) }
$running = @{}
$rows = @()
while ($queue.Count -gt 0 -or $running.Count -gt 0) {
  while ($queue.Count -gt 0 -and $running.Count -lt $Jobs) {
    $t = [string]$queue.Dequeue()
    $childArgs = @("-NoProfile", "-ExecutionPolicy", "Bypass", "-File", "$h\run_win_one.ps1", "-Test", $t)
    if ($Quick) { $childArgs += "-Quick" }
    if ($Force) { $childArgs += "-Force" }
    $running[$t] = Start-Process powershell -ArgumentList $childArgs -PassThru -NoNewWindow -RedirectStandardOutput "$h\$t.one.out" -RedirectStandardError "$h\$t.one.err"
  }
  Start-Sleep -Seconds 1
  foreach ($t in @($running.Keys)) {
    if ($running[$t].HasExited) {
      $running.Remove($t)
      $line = Get-Content "$h\$t.one.out" | Where-Object { $_ -like "RESULT *" } | Select-Object -Last 1
      if (-not $line) { $line = "RESULT test=$t build_s=0 run_s=0 rc=crash slowest_s=0 stage=`"no result`"" }
      $rows += $line
    }
  }
}

$table = foreach ($r in $rows) {
  $m = [regex]::Match($r, 'test=(\S+) build_s=(\S+) run_s=(\S+) rc=(\S+) slowest_s=(\S+) stage="(.*?)"( last=|$)')
  if ($m.Success) {
    [pscustomobject]@{ test = $m.Groups[1].Value; build_s = $m.Groups[2].Value; run_s = $m.Groups[3].Value; rc = $m.Groups[4].Value; slowest_s = $m.Groups[5].Value; slowest_stage = $m.Groups[6].Value }
  } else {
    [pscustomobject]@{ test = "?"; build_s = "?"; run_s = "?"; rc = "?"; slowest_s = "?"; slowest_stage = $r }
  }
}
$table | Format-Table -AutoSize | Out-String -Width 300
$rows | ForEach-Object { $_ }
