$ErrorActionPreference = 'Stop'
Push-Location $PSScriptRoot
try {
  if (-not (Test-Path 'native/tests.exe')) { throw 'Run build.ps1 first.' }
  & ./native/tests.exe
  if ($LASTEXITCODE -ne 0) { throw 'C++ tests failed.' }
} finally { Pop-Location }
