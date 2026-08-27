param(
    [string]$InstallDir = "dist",
    [string]$OutputDir = "artifacts"
)

$ErrorActionPreference = "Stop"

if (-not (Test-Path $InstallDir)) {
    throw "Install directory not found: $InstallDir"
}

$exe = Join-Path $InstallDir "bin/storlive.exe"
if (-not (Test-Path $exe)) {
    throw "StorLive executable not found: $exe"
}

$windeployqt = Get-Command windeployqt.exe -ErrorAction SilentlyContinue
if ($windeployqt) {
    & $windeployqt.Source --qmldir (Join-Path $PSScriptRoot "../../resources/qml") --release $exe
}

New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null
$zip = Join-Path $OutputDir "StorLive-Windows-x64-portable.zip"
if (Test-Path $zip) { Remove-Item $zip -Force }
Compress-Archive -Path (Join-Path $InstallDir "*") -DestinationPath $zip
Write-Host "Portable package: $zip"
