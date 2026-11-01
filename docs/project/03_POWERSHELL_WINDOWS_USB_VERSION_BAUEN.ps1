$ErrorActionPreference = 'Stop'
$root = 'C:\Users\LokalAdmin_2\Ipponboard-Meschede-Build'
$source = Join-Path $root 'USB-Source'
$out = Join-Path $root 'Ausgabe'
$version = '0.2.52'
$package = Join-Path $out "Ipponboard-Meschede_V${version}_Portable"
function Check($label) { if ($LASTEXITCODE -ne 0) { throw "$label fehlgeschlagen (Exitcode $LASTEXITCODE)" } }
if (!(Get-Command git -ErrorAction SilentlyContinue)) { throw 'Git fehlt.' }
if (!(Get-Command cmake -ErrorAction SilentlyContinue)) { throw 'CMake fehlt.' }
New-Item -ItemType Directory -Force -Path $root,$out | Out-Null
if (Test-Path (Join-Path $source '.git')) {
  git -C $source fetch origin main; Check 'Git fetch'
  git -C $source reset --hard origin/main; Check 'Git reset'
} else {
  git clone --branch main --single-branch https://github.com/Judo-Meschede/Ipponboard-Meschede.git $source; Check 'Git clone'
}
$desktop = Join-Path $source 'desktop'
$envCfg = Join-Path $desktop 'env_cfg.bat'
if (!(Test-Path $envCfg)) {
  $existing = Get-Item (Join-Path $root 'Source\Ipponboard-Meschede\env_cfg.bat') -ErrorAction SilentlyContinue
  if ($existing) { Copy-Item $existing.FullName $envCfg }
}
if (!(Test-Path $envCfg)) { throw 'env_cfg.bat fehlt. Bitte die vorhandene Build-Konfiguration in desktop\env_cfg.bat bereitstellen.' }
$cfg = @{}
Get-Content $envCfg | ForEach-Object { if ($_ -match '^set "([^=]+)=(.*)"') { $cfg[$matches[1]] = $matches[2] } }
$qt = $cfg['QTDIR']
if (!$qt -or !(Test-Path (Join-Path $qt 'bin\windeployqt.exe'))) { throw 'Qt/windeployqt nicht in QTDIR gefunden.' }
$build = Join-Path $root 'USB-Build'
cmake -S $desktop -B $build -G 'Visual Studio 17 2022' -A Win32; Check 'CMake Konfiguration'
cmake --build $build --config Release --target Ipponboard-Meschede; Check 'CMake Build'
$exe = Join-Path $desktop '_bin\Ipponboard-Meschede-Release\Ipponboard-Meschede.exe'
if (!(Test-Path $exe)) { throw "EXE fehlt: $exe" }
if (Test-Path $package) { Remove-Item $package -Recurse -Force }
$app = Join-Path $package 'app'
New-Item -ItemType Directory -Force -Path $app | Out-Null
Copy-Item (Join-Path (Split-Path $exe) '*') $app -Recurse -Force
& (Join-Path $qt 'bin\windeployqt.exe') --release --no-translations --dir $app $exe; Check 'Qt Deployment'
@'
@echo off
cd /d "%~dp0app"
start "" "Ipponboard-Meschede.exe"
'@ | Set-Content (Join-Path $package 'START_IPPONBOARD_MESCHEDE.cmd') -Encoding Ascii
Copy-Item (Join-Path $source 'LICENSE.txt') $package
New-Item -ItemType Directory -Force -Path (Join-Path $app 'data') | Out-Null
$zip = "$package.zip"
if (Test-Path $zip) { Remove-Item $zip -Force }
Compress-Archive -Path (Join-Path $package '*') -DestinationPath $zip -CompressionLevel Optimal
Write-Host "FERTIG: $zip"
Write-Host 'USB-Test: ZIP entpacken, START_IPPONBOARD_MESCHEDE.cmd auf fremdem Windows-PC starten.'
