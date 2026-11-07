# SPDX-License-Identifier: GPL-3.0-or-later
# build-windows.ps1: Build and package localplex for Windows (Portable ZIP + Installer)

param(
    [string]$BuildType = "Release",
    [string]$QtDir = $env:QT_DIR,
    [string]$MpvDir = $env:MPV_DIR
)

$ErrorActionPreference = "Stop"
$RepoRoot = Resolve-Path "$PSScriptRoot\..\.."
$BuildDir = Join-Path $RepoRoot "build-windows"
$DistDir = Join-Path $RepoRoot "dist"
$PackageDir = Join-Path $BuildDir "package"

Write-Host "=== 1. Creating directories ==="
New-Item -ItemType Directory -Force -Path $DistDir | Out-Null
New-Item -ItemType Directory -Force -Path $PackageDir | Out-Null

Write-Host "=== 2. Configuring CMake ==="
cmake -B $BuildDir -S $RepoRoot `
    -DCMAKE_BUILD_TYPE=$BuildType `
    -DCMAKE_PREFIX_PATH="$QtDir;$MpvDir"

Write-Host "=== 3. Building executable ==="
cmake --build $BuildDir --config $BuildType --parallel

Write-Host "=== 4. Staging files ==="
$BinDir = Join-Path $BuildDir $BuildType
if (-not (Test-Path $BinDir)) {
    $BinDir = $BuildDir
}
Copy-Item (Join-Path $BinDir "localplex.exe") -Destination $PackageDir

Write-Host "=== 5. Running windeployqt ==="
$Windeployqt = Join-Path $QtDir "bin\windeployqt.exe"
if (Test-Path $Windeployqt) {
    & $Windeployqt --qmldir (Join-Path $RepoRoot "src\qml") --release (Join-Path $PackageDir "localplex.exe")
} else {
    Write-Warning "windeployqt not found at $Windeployqt. Ensure Qt bin directory is in PATH."
}

if ($MpvDir -and (Test-Path "$MpvDir\mpv-2.dll")) {
    Copy-Item "$MpvDir\mpv-2.dll" -Destination $PackageDir
}

Write-Host "=== 6. Creating Portable ZIP ==="
$ZipPath = Join-Path $DistDir "localplex-windows-x86_64.zip"
if (Test-Path $ZipPath) { Remove-Item $ZipPath }
Compress-Archive -Path "$PackageDir\*" -DestinationPath $ZipPath
Write-Host "Portable archive created: $ZipPath"

Write-Host "=== 7. Building NSIS Installer (optional) ==="
$Makensis = Get-Command "makensis" -ErrorAction SilentlyContinue
if ($Makensis) {
    & makensis /DPRODUCT_VERSION="0.1.0" /DSOURCE_DIR="$PackageDir" /DOUT_FILE="$DistDir\localplex-0.1.0-setup.exe" "$PSScriptRoot\installer.nsi"
    Write-Host "Installer created: $DistDir\localplex-0.1.0-setup.exe"
} else {
    Write-Host "makensis not detected in PATH. Skipping installer generation."
}
