param([string]$Environment = 'crowpanel_idf5')
$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent $PSScriptRoot
Push-Location $Root
try {
  py -m platformio run -e $Environment
  if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
  py tools/export_firmware.py --env $Environment
  if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
} finally {
  Pop-Location
}
