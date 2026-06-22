$file = "E:\RenoDX\renodx\src\games\sora-vanillaplus\addon.cpp"
$content = [System.IO.File]::ReadAllText($file)

$basicKeys = @('SettingsMode','VolFogHazeAAMode','CharShadowMode','XeGTAOMode',
    'SSGIEnable','ISFASTEnable','ShadowFilterMethod','ShadowEdgeTint')

$lines = $content -split "`r`n"
$inSetting = $false
$hasVisible = $false
$currentKey = ""
$newLines = @()

foreach ($line in $lines) {
    if ($line -match '\.key\s*=\s*"([^"]+)"') {
        $currentKey = $matches[1]
        $inSetting = $true
        $hasVisible = $false
    }
    if ($inSetting -and $line -match '\.is_visible') {
        $hasVisible = $true
    }
    if ($inSetting -and ($line -match '^\s*\},?\s*$')) {
        if (-not $hasVisible -and ($basicKeys -notcontains $currentKey)) {
            $indent = "      "
            $newLines += "$indent.is_visible = []() { return IsAdvancedSettingsMode(); },"
        }
        $inSetting = $false
    }
    $newLines += $line
}

[System.IO.File]::WriteAllText($file, ($newLines -join "`r`n"))
Write-Output "Added is_visible to settings not in Basic list."
