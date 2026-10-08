# Copies repo world/ sources (*.hpp, *.h, *.hlsl, *.hlsli) into the harness copy with CR bytes stripped.
# Rewrites a destination only when its bytes differ; deletes destination files whose source is gone.
$ErrorActionPreference = "Stop"
$repo = "E:\RenoDX\renodx\src\games\falcomengine-plus\world"
$dest = "E:\RenoDX\renodx\.falcom-dev\harness\src\games\falcomengine-plus\world"
$exts = @(".hpp", ".h", ".hlsl", ".hlsli")
$changed = 0
$sources = @{}
foreach ($f in Get-ChildItem -Path $repo -Recurse -File | Where-Object { $exts -contains $_.Extension }) {
  $rel = $f.FullName.Substring($repo.Length).TrimStart('\')
  $sources[$rel] = $true
  $bytes = [System.IO.File]::ReadAllBytes($f.FullName)
  $stripped = New-Object System.Collections.Generic.List[byte]
  foreach ($b in $bytes) { if ($b -ne 13) { $stripped.Add($b) } }
  $out = $stripped.ToArray()
  $target = Join-Path $dest $rel
  $same = $false
  if (Test-Path $target) {
    $cur = [System.IO.File]::ReadAllBytes($target)
    $same = ($cur.Length -eq $out.Length) -and ([System.Linq.Enumerable]::SequenceEqual([byte[]]$cur, [byte[]]$out))
  }
  if (-not $same) {
    New-Item -ItemType Directory -Force -Path (Split-Path $target) | Out-Null
    [System.IO.File]::WriteAllBytes($target, $out)
    $changed++
  }
}
foreach ($f in Get-ChildItem -Path $dest -Recurse -File | Where-Object { $exts -contains $_.Extension }) {
  $rel = $f.FullName.Substring($dest.Length).TrimStart('\')
  if (-not $sources.ContainsKey($rel)) { Remove-Item -LiteralPath $f.FullName -Force; $changed++ }
}
"refresh_world: $changed file(s) changed"
