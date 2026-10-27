$ErrorActionPreference = 'Stop'
$releaseDir = Join-Path $PSScriptRoot 'release/Zzen'
New-Item -ItemType Directory -Force -Path $releaseDir | Out-Null
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'native/Zzen.exe') -Destination (Join-Path $releaseDir 'Zzen.exe')
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'LEEME.txt') -Destination (Join-Path $releaseDir 'README.txt')
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'VALIDACION.txt') -Destination (Join-Path $releaseDir 'VALIDACION.txt')
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'LICENSE') -Destination (Join-Path $releaseDir 'LICENSE')
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'NOTICE.txt') -Destination (Join-Path $releaseDir 'NOTICE.txt')
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'output/pdf/Zzen-v0.2-User-Guide.pdf') -Destination (Join-Path $releaseDir 'Zzen-User-Guide.pdf')
Get-FileHash -LiteralPath (Join-Path $releaseDir 'Zzen.exe') -Algorithm SHA256 | ForEach-Object { $_.Hash + '  Zzen.exe' } | Set-Content (Join-Path $releaseDir 'SHA256.txt') -Encoding ascii
Compress-Archive -LiteralPath (Join-Path $releaseDir 'Zzen.exe'),(Join-Path $releaseDir 'README.txt'),(Join-Path $releaseDir 'VALIDACION.txt'),(Join-Path $releaseDir 'LICENSE'),(Join-Path $releaseDir 'NOTICE.txt'),(Join-Path $releaseDir 'Zzen-User-Guide.pdf'),(Join-Path $releaseDir 'SHA256.txt') -DestinationPath (Join-Path $PSScriptRoot 'release/Zzen-0.2-Windows-x64.zip') -Force
