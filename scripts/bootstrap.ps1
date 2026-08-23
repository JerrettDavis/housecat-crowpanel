$ErrorActionPreference = 'Stop'
$ProjectRoot = Split-Path -Parent $PSScriptRoot

Write-Host 'Installing the pinned PlatformIO Core used by House Cat...'
py -m pip install --upgrade -r (Join-Path $ProjectRoot 'requirements-platformio.txt')
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
py -m platformio --version
