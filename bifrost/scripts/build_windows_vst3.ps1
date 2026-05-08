$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
$buildDir = Join-Path $repoRoot "build-windows"
$distDir = Join-Path $repoRoot "dist"
$config = "RelWithDebInfo"
$versionLine = python (Join-Path $repoRoot "scripts/print_project_version.py")
$version = ($versionLine -replace "^version=", "").Trim()
if ([string]::IsNullOrWhiteSpace($version)) {
    throw "Could not read project version"
}

New-Item -ItemType Directory -Force -Path $distDir | Out-Null

cmake -S $repoRoot -B $buildDir -G "Visual Studio 17 2022" -A x64 `
    -DBIFROST_BUILD_TESTS=ON `
    -DBIFROST_BUILD_STANDALONE=OFF `
    -DBIFROST_BUILD_PLUGIN_SMOKE=OFF `
    -DBIFROST_ENABLE_ONNX=OFF `
    -DBIFROST_ENABLE_MINIAUDIO=OFF `
    -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded

cmake --build $buildDir --config $config --target BifrostTests
ctest --test-dir $buildDir -C $config --output-on-failure
cmake --build $buildDir --config $config --target Bifrost_VST3

$vst3 = Get-ChildItem -Path $buildDir -Recurse -Filter "Bifrost.vst3" | Select-Object -First 1
if ($null -eq $vst3) {
    throw "Bifrost.vst3 was not produced under $buildDir"
}

$packageDir = Join-Path $distDir "Bifrost-$version-windows-vst3"
$zipPath = Join-Path $distDir "Bifrost-$version-windows-vst3.zip"
if (Test-Path $packageDir) {
    Remove-Item $packageDir -Recurse -Force
}
if (Test-Path $zipPath) {
    Remove-Item $zipPath -Force
}

New-Item -ItemType Directory -Force -Path $packageDir | Out-Null
Copy-Item -Path $vst3.FullName -Destination (Join-Path $packageDir "Bifrost.vst3") -Recurse
@"
Install Bifrost.vst3 to:
C:\Program Files\Common Files\VST3\

Then rescan plugins in your DAW.
"@ | Set-Content -Path (Join-Path $packageDir "INSTALL.txt")

Compress-Archive -Path (Join-Path $packageDir "*") -DestinationPath $zipPath
Write-Host "Windows VST3 bundle: $($vst3.FullName)"
Write-Host "Windows VST3 zip: $zipPath"
