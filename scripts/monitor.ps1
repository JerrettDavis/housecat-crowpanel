param(
  [Parameter(Mandatory=$true)][string]$Port
)
$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent $PSScriptRoot
Push-Location $Root
try {
  py -m platformio device monitor --port $Port --baud 115200 --filter time --filter colorize --filter esp32_exception_decoder
  if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
} finally {
  Pop-Location
}
