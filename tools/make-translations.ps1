# Generates data/Interface/Translations/StancesNGCombatExpansion_<LANGUAGE>.txt from translations.tsv.
# FLICK reads these files itself (UTF-16 LE with BOM, "key<TAB>text" lines); the game language falls back to ENGLISH.
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$rows = Get-Content -Encoding UTF8 (Join-Path $PSScriptRoot 'translations.tsv')
$languages = $rows[0].Split("`t") | Select-Object -Skip 1
$outDir = Join-Path $root 'data\Interface\Translations'
New-Item -ItemType Directory -Force $outDir | Out-Null

for ($column = 0; $column -lt $languages.Count; $column++) {
    $lines = foreach ($row in ($rows | Select-Object -Skip 1)) {
        if (-not $row.Trim()) { continue }
        $cells = $row.Split("`t")
        if ($cells.Count -ne $languages.Count + 1) { throw "Bad row: $row" }
        "$($cells[0])`t$($cells[$column + 1])"
    }
    $path = Join-Path $outDir "StancesNGCombatExpansion_$($languages[$column]).txt"
    [IO.File]::WriteAllText($path, ($lines -join "`r`n") + "`r`n", [Text.UnicodeEncoding]::new($false, $true))
    Write-Output "$path ($($lines.Count) keys)"
}
