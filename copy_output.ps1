<#
    Gather the UF2s from build folders into .\output\
    and rename them to left.uf2, right.uf2, dongle.uf2, reset.uf2.
#>

$ErrorActionPreference = 'Stop'    # fail fast on any error

# Map build sub-directory -> desired output filename
$fwMap = @{
    left   = 'left.uf2'
    right  = 'right.uf2'
    dongle = 'dongle.uf2'
    reset  = 'reset.uf2'
}

# Destination folder
$destDir = Join-Path $PSScriptRoot 'output'
if (-not (Test-Path $destDir)) { New-Item -ItemType Directory -Path $destDir | Out-Null }

foreach ($key in $fwMap.Keys) {
    $src = Join-Path $PSScriptRoot "build\$key\zephyr\zmk.uf2"
    $dst = Join-Path $destDir     $fwMap[$key]

    if (Test-Path $src) {
        Copy-Item $src $dst -Force
        Write-Host "Copied $src -> $dst"
    } else {
        Write-Warning "Skipped $key  (source not found: $src)"
    }
}

Write-Host "`nDone. Files are in $destDir"
