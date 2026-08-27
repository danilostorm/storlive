param(
    [string]$Version = "30.0.2",
    [string]$WorkDir = ".obs-runtime"
)

$ErrorActionPreference = "Stop"
$ProgressPreference = "SilentlyContinue"

$root = [System.IO.Path]::GetFullPath($WorkDir)
$runtimeArchive = Join-Path $root "obs-runtime.zip"
$sourceArchive = Join-Path $root "obs-source.zip"
$runtimeExtract = Join-Path $root "runtime-extract"
$sourceExtract = Join-Path $root "source-extract"
$runtimeRoot = Join-Path $root "runtime"
$sdkRoot = Join-Path $root "sdk"

Remove-Item $root -Recurse -Force -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force -Path $root | Out-Null

function Download-WithRetry([string]$Uri, [string]$OutFile) {
    $last = $null
    for ($attempt = 1; $attempt -le 3; $attempt++) {
        try {
            Invoke-WebRequest -Uri $Uri -OutFile $OutFile -UseBasicParsing
            return
        } catch {
            $last = $_
            if ($attempt -lt 3) { Start-Sleep -Seconds (2 * $attempt) }
        }
    }
    throw $last
}

$runtimeUrl = "https://github.com/obsproject/obs-studio/releases/download/$Version/OBS-Studio-$Version.zip"
$sourceUrl = "https://github.com/obsproject/obs-studio/archive/refs/tags/$Version.zip"

Write-Host "Downloading OBS Studio $Version portable runtime..."
Download-WithRetry $runtimeUrl $runtimeArchive
Write-Host "Downloading OBS Studio $Version source headers..."
Download-WithRetry $sourceUrl $sourceArchive

Expand-Archive -Path $runtimeArchive -DestinationPath $runtimeExtract -Force
Expand-Archive -Path $sourceArchive -DestinationPath $sourceExtract -Force

$obsDll = Get-ChildItem -Path $runtimeExtract -Recurse -File -Filter "obs.dll" |
    Where-Object { $_.FullName -match '[\\/]bin[\\/]64bit[\\/]obs\.dll$' } |
    Select-Object -First 1
if (-not $obsDll) { throw "obs.dll not found in OBS runtime archive" }

$detectedRuntimeRoot = Split-Path (Split-Path (Split-Path $obsDll.FullName -Parent) -Parent) -Parent
New-Item -ItemType Directory -Force -Path $runtimeRoot | Out-Null
foreach ($folder in @("bin", "obs-plugins", "data")) {
    $source = Join-Path $detectedRuntimeRoot $folder
    if (Test-Path $source) {
        Copy-Item $source -Destination $runtimeRoot -Recurse -Force
    }
}

$obsHeader = Get-ChildItem -Path $sourceExtract -Recurse -File -Filter "obs.h" |
    Where-Object { $_.Directory.Name -eq "libobs" } |
    Select-Object -First 1
if (-not $obsHeader) { throw "libobs/obs.h not found in OBS source archive" }

$includeDir = Join-Path $sdkRoot "include/obs"
$libDir = Join-Path $sdkRoot "lib/64bit"
New-Item -ItemType Directory -Force -Path $includeDir, $libDir | Out-Null
Copy-Item (Join-Path $obsHeader.Directory.FullName "*") -Destination $includeDir -Recurse -Force

$normalizedObsDll = Join-Path $runtimeRoot "bin/64bit/obs.dll"
if (-not (Test-Path $normalizedObsDll)) { throw "Normalized obs.dll not found" }

$dumpbin = Get-Command dumpbin.exe -ErrorAction Stop
$libTool = Get-Command lib.exe -ErrorAction Stop
$exports = & $dumpbin.Source /nologo /exports $normalizedObsDll
$names = [System.Collections.Generic.List[string]]::new()
foreach ($line in $exports) {
    if ($line -match '^\s*\d+\s+[0-9A-Fa-f]+\s+[0-9A-Fa-f]+\s+([^\s=]+)') {
        $name = $Matches[1]
        if ($name -and -not $names.Contains($name)) { $names.Add($name) }
    }
}
if ($names.Count -lt 50) { throw "Unexpectedly few exports found in obs.dll ($($names.Count))" }

$defPath = Join-Path $libDir "obs.def"
$libPath = Join-Path $libDir "obs.lib"
$defLines = @("LIBRARY obs.dll", "EXPORTS") + ($names | Sort-Object | ForEach-Object { "    $_" })
Set-Content -Path $defPath -Value $defLines -Encoding ASCII
& $libTool.Source /nologo "/def:$defPath" /machine:x64 "/out:$libPath"
if ($LASTEXITCODE -ne 0 -or -not (Test-Path $libPath)) { throw "Failed to generate obs.lib" }

Copy-Item $normalizedObsDll -Destination (Join-Path $sdkRoot "obs.dll") -Force
Set-Content -Path (Join-Path $root "VERSION") -Value $Version -NoNewline -Encoding ASCII

Write-Host "OBS runtime: $runtimeRoot"
Write-Host "OBS SDK:     $sdkRoot"
