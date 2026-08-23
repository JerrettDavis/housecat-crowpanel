param(
  [Parameter(Mandatory=$true)][string]$Port,
  [string]$Environment = 'crowpanel_idf5'
)
$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent $PSScriptRoot
Push-Location $Root
try {
  py -m platformio run -e $Environment -t upload --upload-port $Port
  if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
} finally {
  Pop-Location
}
