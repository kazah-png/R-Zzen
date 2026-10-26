$ErrorActionPreference = 'Stop'
Set-Location $PSScriptRoot
$vswhere = 'C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe'
$vs = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vs) { throw 'Se necesita Visual Studio Build Tools con C++.' }
$vc = Join-Path $vs 'VC\Auxiliary\Build\vcvars64.bat'
New-Item -ItemType Directory -Force -Path 'native' | Out-Null
$build = '@echo off' + "`r`n" + 'call "' + $vc + '" >nul' + "`r`n" + 'rc /nologo /I assets /fo native\Zzen.res assets\Zzen.rc' + "`r`n" + 'if errorlevel 1 exit /b 1' + "`r`n" + 'cl /nologo /std:c++17 /EHsc /utf-8 /MT /O2 /W4 /DUNICODE /D_UNICODE cpp\main.cpp native\Zzen.res /Fo:native\main.obj /Fe:native\Zzen.exe /link /SUBSYSTEM:WINDOWS user32.lib gdi32.lib ole32.lib windowscodecs.lib windowsapp.lib' + "`r`n" + 'if errorlevel 1 exit /b 1' + "`r`n" + 'cl /nologo /std:c++17 /EHsc /utf-8 /MT /O2 /W4 /DUNICODE /D_UNICODE cpp\tests.cpp /Fo:native\tests.obj /Fe:native\tests.exe /link ole32.lib windowscodecs.lib windowsapp.lib' + "`r`n" + 'exit /b %errorlevel%'
[IO.File]::WriteAllText((Join-Path $PSScriptRoot 'native\compile.cmd'),$build,[Text.Encoding]::ASCII)
& cmd.exe /d /c native\compile.cmd
if ($LASTEXITCODE -ne 0) { throw 'Falló la compilación C++.' }
& .\native\tests.exe
if ($LASTEXITCODE -ne 0) { throw 'Fallaron las pruebas C++.' }
