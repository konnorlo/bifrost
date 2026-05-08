$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
$buildDir = Join-Path $repoRoot "build-windows"
$distDir = Join-Path $repoRoot "dist"
$config = "RelWithDebInfo"

New-Item -ItemType Directory -Force -Path $distDir | Out-Null

cmake -S $repoRoot -B $buildDir -G "Visual Studio 17 2022" -A x64 `
    -DBIFROST_BUILD_TESTS=ON `
    -DBIFROST_BUILD_STANDALONE=OFF `
    -DBIFROST_BUILD_PLUGIN_SMOKE=OFF `
    -DBIFROST_ENABLE_ONNX=OFF `
    -DBIFROST_ENABLE_MINIAUDIO=OFF

cmake --build $buildDir --config $config --target BifrostTests
ctest --test-dir $buildDir -C $config --output-on-failure
cmake --build $buildDir --config $config --target Bifrost_VST3

$vst3 = Get-ChildItem -Path $buildDir -Recurse -Filter "Bifrost.vst3" | Select-Object -First 1
if ($null -eq $vst3) {
    throw "Bifrost.vst3 was not produced under $buildDir"
}

$zipPath = Join-Path $distDir "Bifrost-windows-vst3.zip"
if (Test-Path $zipPath) {
    Remove-Item $zipPath -Force
}

Compress-Archive -Path $vst3.FullName -DestinationPath $zipPath
Write-Host "Windows VST3 bundle: $($vst3.FullName)"
Write-Host "Windows VST3 zip: $zipPath"
