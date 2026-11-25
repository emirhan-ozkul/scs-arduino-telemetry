param(
    [ValidateSet("Release", "Debug")]
    [string]$Configuration = "Release"
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
Set-Location $root

New-Item -ItemType Directory -Force "$root\dist" | Out-Null

python -m pip install --disable-pip-version-check -r "$root\python\requirements.txt"
python -m unittest discover -s "$root\tests" -p "test_*.py"

$pyInstaller = python -m PyInstaller --version
Write-Host "PyInstaller $pyInstaller"
python -m PyInstaller --noconfirm --clean "$root\python\app.spec"

$package = "$root\dist\ETS2TelemetryPortable"
Remove-Item -Recurse -Force $package -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force "$package" | Out-Null

Copy-Item -Recurse -Force "$root\dist\ETS2Telemetry\*" "$package"
Copy-Item -Recurse -Force "$root\docs" "$package\docs"
Copy-Item -Recurse -Force "$root\native_plugin" "$package\native_plugin"
Copy-Item -Recurse -Force "$root\arduino" "$package\arduino"
Copy-Item -Force "$root\build\native_plugin-x64\Release\ETS2TelemetryPlugin.dll" "$package\ETS2TelemetryPlugin.dll" -ErrorAction SilentlyContinue

New-Item -ItemType Directory -Force "$package\logs" | Out-Null
New-Item -ItemType Directory -Force "$package\config" | Out-Null

$zip = "$root\dist\ETS2Telemetry-$Configuration.zip"
Remove-Item -Force $zip -ErrorAction SilentlyContinue
Compress-Archive -Path "$package\*" -DestinationPath $zip

Write-Host "Portable folder: $package"
Write-Host "ZIP: $zip"
