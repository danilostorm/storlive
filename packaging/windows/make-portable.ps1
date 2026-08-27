param(
    [string]$InstallDir = "dist",
    [string]$OutputDir = "artifacts",
    [string]$ObsRuntimeRoot = ""
)

$ErrorActionPreference = "Stop"

$install = [System.IO.Path]::GetFullPath($InstallDir)
if (-not (Test-Path $install)) { throw "Install directory not found: $install" }

$exe = Join-Path $install "storlive.exe"
if (-not (Test-Path $exe)) { throw "StorLive executable not found: $exe" }

$windeployqt = Get-Command windeployqt.exe -ErrorAction SilentlyContinue
if ($windeployqt) {
    & $windeployqt.Source --qmldir (Join-Path $PSScriptRoot "../../resources/qml") --release --compiler-runtime $exe
    if ($LASTEXITCODE -ne 0) { throw "windeployqt failed" }
}

# windeployqt does not reliably copy the MSVC runtime on every CI image.
# Bundle the exact x64 runtime DLLs explicitly so the portable ZIP starts on a clean Windows install.
$crtFiles = @(
    "MSVCP140.dll",
    "MSVCP140_1.dll",
    "MSVCP140_2.dll",
    "VCRUNTIME140.dll",
    "VCRUNTIME140_1.dll"
)

$crtSearchRoots = @()
if ($env:VCToolsRedistDir) { $crtSearchRoots += $env:VCToolsRedistDir }
if ($env:SystemRoot) { $crtSearchRoots += (Join-Path $env:SystemRoot "System32") }

foreach ($crt in $crtFiles) {
    if (Test-Path (Join-Path $install $crt)) { continue }

    $found = $null
    foreach ($root in $crtSearchRoots) {
        if (-not (Test-Path $root)) { continue }
        $candidate = Get-ChildItem -Path $root -Recurse -File -Filter $crt -ErrorAction SilentlyContinue |
            Where-Object { $_.FullName -match 'x64|System32' } |
            Select-Object -First 1
        if ($candidate) {
            $found = $candidate.FullName
            break
        }
    }

    if (-not $found) { throw "Unable to locate required MSVC runtime DLL: $crt" }
    Copy-Item $found -Destination (Join-Path $install $crt) -Force
}

if ($ObsRuntimeRoot) {
    $obsRoot = [System.IO.Path]::GetFullPath($ObsRuntimeRoot)
    $obsBin = Join-Path $obsRoot "bin/64bit"
    $obsPlugins = Join-Path $obsRoot "obs-plugins/64bit"
    $obsData = Join-Path $obsRoot "data"
    if (-not (Test-Path (Join-Path $obsBin "obs.dll"))) { throw "OBS runtime is invalid: obs.dll missing" }

    # Keep StorLive's Qt deployment. OBS plugins selected below do not require OBS's Qt runtime.
    Get-ChildItem -Path $obsBin -File -Filter "*.dll" | Where-Object {
        $_.Name -notlike "Qt6*" -and
        $_.Name -notin @("libEGL.dll", "libGLESv2.dll", "opengl32sw.dll")
    } | ForEach-Object {
        Copy-Item $_.FullName -Destination (Join-Path $install $_.Name) -Force
    }

    $requiredPlugins = @(
        "obs-outputs",
        "rtmp-services",
        "obs-x264",
        "obs-ffmpeg",
        "win-capture",
        "win-dshow",
        "win-wasapi"
    )
    $optionalPlugins = @("obs-nvenc", "obs-qsv11")
    $pluginDest = Join-Path $install "obs-plugins/64bit"
    New-Item -ItemType Directory -Force -Path $pluginDest | Out-Null

    foreach ($plugin in $requiredPlugins) {
        $dll = Join-Path $obsPlugins "$plugin.dll"
        if (-not (Test-Path $dll)) { throw "Required OBS plugin missing: $plugin.dll" }
        Copy-Item $dll -Destination $pluginDest -Force
    }
    foreach ($plugin in $optionalPlugins) {
        $dll = Join-Path $obsPlugins "$plugin.dll"
        if (Test-Path $dll) { Copy-Item $dll -Destination $pluginDest -Force }
    }

    $libobsData = Join-Path $obsData "libobs"
    if (-not (Test-Path $libobsData)) { throw "OBS data/libobs directory missing" }
    $dataRoot = Join-Path $install "data"
    New-Item -ItemType Directory -Force -Path $dataRoot | Out-Null
    Copy-Item $libobsData -Destination $dataRoot -Recurse -Force

    $pluginDataDest = Join-Path $dataRoot "obs-plugins"
    New-Item -ItemType Directory -Force -Path $pluginDataDest | Out-Null
    foreach ($plugin in ($requiredPlugins + $optionalPlugins)) {
        $sourceData = Join-Path $obsData "obs-plugins/$plugin"
        if (Test-Path $sourceData) {
            Copy-Item $sourceData -Destination $pluginDataDest -Recurse -Force
        }
    }

    # Ensure the executable is really linked against libobs, not the UI-only stub.
    $dumpbin = Get-Command dumpbin.exe -ErrorAction Stop
    $imports = (& $dumpbin.Source /nologo /imports $exe) -join "`n"
    if ($imports -notmatch '(?im)^\s*obs\.dll\s*$') {
        throw "storlive.exe does not import obs.dll; refusing to publish a stub portable build"
    }
}

# Force Qt to resolve plugins and QML imports relative to the portable directory.
$qtConf = @"
[Paths]
Prefix=.
Plugins=.
Qml2Imports=qml
Translations=translations
"@
Set-Content -Path (Join-Path $install "qt.conf") -Value $qtConf -Encoding ASCII

$debugLauncher = @"
@echo off
set QT_DEBUG_PLUGINS=1
set QML_IMPORT_TRACE=1
storlive.exe > storlive-debug.log 2>&1
type storlive-debug.log
pause
"@
Set-Content -Path (Join-Path $install "Run-StorLive-Debug.cmd") -Value $debugLauncher -Encoding ASCII

$licenseSource = Join-Path $PSScriptRoot "../../LICENSE"
if (Test-Path $licenseSource) { Copy-Item $licenseSource -Destination (Join-Path $install "LICENSE.txt") -Force }

$notice = @"
StorLive includes and dynamically links to libobs / OBS Studio components.
OBS Studio is Copyright (C) the OBS Project contributors and is licensed under GPL-2.0-or-later.
StorLive source: https://github.com/danilostorm/storlive
OBS Studio source: https://github.com/obsproject/obs-studio
"@
Set-Content -Path (Join-Path $install "NOTICE-OBS.txt") -Value $notice -Encoding UTF8

$out = [System.IO.Path]::GetFullPath($OutputDir)
New-Item -ItemType Directory -Force -Path $out | Out-Null
$zip = Join-Path $out "StorLive-Windows-x64-portable.zip"
if (Test-Path $zip) { Remove-Item $zip -Force }
Compress-Archive -Path (Join-Path $install "*") -DestinationPath $zip -CompressionLevel Optimal

$requiredPortable = @(
    "storlive.exe",
    "qt.conf",
    "MSVCP140.dll",
    "MSVCP140_1.dll",
    "MSVCP140_2.dll",
    "VCRUNTIME140.dll",
    "VCRUNTIME140_1.dll",
    "obs.dll",
    "libobs-d3d11.dll",
    "obs-plugins/64bit/obs-outputs.dll",
    "obs-plugins/64bit/rtmp-services.dll",
    "obs-plugins/64bit/win-capture.dll",
    "obs-plugins/64bit/win-dshow.dll",
    "obs-plugins/64bit/win-wasapi.dll",
    "data/libobs"
)
foreach ($relative in $requiredPortable) {
    if (-not (Test-Path (Join-Path $install $relative))) { throw "Portable validation failed: missing $relative" }
}

Write-Host "Portable package: $zip"
