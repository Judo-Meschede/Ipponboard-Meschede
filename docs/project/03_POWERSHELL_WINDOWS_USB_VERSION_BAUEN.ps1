$ErrorActionPreference = 'Stop'
$root = Join-Path ([Environment]::GetFolderPath('MyDocuments')) 'Ipponboard-Meschede-Build'
$source = Join-Path $root 'GitHub\Ipponboard-Meschede'
$out = Join-Path $root 'Ausgabe'
$version = '0.2.57-preview'
$package = Join-Path $out "Ipponboard-Meschede_V${version}_Portable"
function Check($label) { if ($LASTEXITCODE -ne 0) { throw "$label fehlgeschlagen (Exitcode $LASTEXITCODE)" } }
if (!(Get-Command git -ErrorAction SilentlyContinue)) { throw 'Git fehlt.' }
if (!(Get-Command cmake -ErrorAction SilentlyContinue)) { throw 'CMake fehlt.' }
New-Item -ItemType Directory -Force -Path $root,$out | Out-Null
if (!(Test-Path (Join-Path $source '.git'))) {
  New-Item -ItemType Directory -Force -Path (Split-Path $source) | Out-Null
  git clone https://github.com/Judo-Meschede/Ipponboard-Meschede.git $source
  Check 'GitHub-Arbeitskopie erstellen'
}
# Never reset/fetch here: release must reflect the exact locally tested commit.
$dirty = git -C $source status --porcelain
Check 'Git status'
if ($dirty) { Write-Warning 'Lokale Änderungen vorhanden; diese fließen in den Build ein.' }
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
if (!$qt) { $qt = $cfg['QTDIR'] }
$env:QTDIR = $qt
$env:BOOST_DIR = $cfg['BOOST_DIR']
$env:INNO_DIR = $cfg['INNO_DIR']
if (!$qt -or !(Test-Path (Join-Path $qt 'bin\windeployqt.exe'))) { throw 'Qt/windeployqt nicht in QTDIR gefunden.' }
$build = Join-Path $root 'USB-Build'
$base = Join-Path $desktop 'base'
& (Join-Path $desktop 'scripts\create-versioninfo.cmd') $base; Check 'Versionsdateien erzeugen'
$buildDir = Join-Path $desktop 'build-x86'
$cache = Join-Path $buildDir 'CMakeCache.txt'
if (Test-Path $cache) {
  $cacheSource = Select-String -LiteralPath $cache -Pattern '^CMAKE_HOME_DIRECTORY:INTERNAL=' | Select-Object -First 1
  if ($cacheSource -and $cacheSource.Line.Split('=',2)[1].Replace('\\','/').TrimEnd('/') -ne $desktop.Replace('\\','/').TrimEnd('/')) {
    Remove-Item $buildDir -Recurse -Force
    Write-Host 'Veralteten CMake-Buildcache mit fremdem Quellpfad entfernt.'
  }
}
cmake -S $desktop -B $buildDir -G 'Visual Studio 17 2022' -A Win32; Check 'CMake Konfiguration'
cmake --build (Join-Path $desktop 'build-x86') --config Release --target Ipponboard-Meschede; Check 'CMake Build'
$exe = Join-Path $desktop '_bin\Ipponboard-Meschede-Release\Ipponboard-Meschede.exe'
if (!(Test-Path $exe)) { throw "EXE fehlt: $exe" }
if (Test-Path $package) { Remove-Item $package -Recurse -Force }
$app = Join-Path $package 'app'
New-Item -ItemType Directory -Force -Path $app | Out-Null
Copy-Item (Join-Path (Split-Path $exe) '*') $app -Recurse -Force
Copy-Item (Join-Path $desktop 'base\TournamentModes.ini') (Join-Path $app 'TournamentModes.ini') -Force
& (Join-Path $qt 'bin\windeployqt.exe') --release --no-translations --dir $app (Join-Path $app 'Ipponboard-Meschede.exe'); Check 'Qt Deployment'
# Embed the complete web administration and its Node dependencies. The USB server only binds to localhost.
$localServer = Join-Path $package 'local-server'
New-Item -ItemType Directory -Force -Path $localServer | Out-Null
Copy-Item (Join-Path $source 'server\server.js'),(Join-Path $source 'server\usb-export.js'),(Join-Path $source 'server\package.json') $localServer
Copy-Item (Join-Path $source 'server\public') (Join-Path $localServer 'public') -Recurse
Remove-Item (Join-Path $localServer 'public\data') -Recurse -Force -ErrorAction SilentlyContinue
Remove-Item (Join-Path $localServer 'public\downloads') -Recurse -Force -ErrorAction SilentlyContinue
$nodeVersion = 'v24.11.0'
$nodeArchive = "node-$nodeVersion-win-x64.zip"
$nodeCache = Join-Path $root "Tools\node-$nodeVersion-win-x64"
if (!(Test-Path (Join-Path $nodeCache 'node.exe'))) {
  $downloads = Join-Path $root 'Downloads'
  New-Item -ItemType Directory -Force -Path $downloads | Out-Null
  $zipPath = Join-Path $downloads $nodeArchive
  $baseUrl = "https://nodejs.org/dist/$nodeVersion"
  Invoke-WebRequest "$baseUrl/$nodeArchive" -OutFile $zipPath
  $sums = (Invoke-WebRequest "$baseUrl/SHASUMS256.txt").Content
  $line = ($sums -split "[\r\n]+" | Where-Object { $_ -match ([regex]::Escape($nodeArchive) + '$') } | Select-Object -First 1)
  if (!$line) { throw 'Node SHA256 fehlt.' }
  $expected = ($line -split '\s+')[0].ToLowerInvariant()
  $actual = (Get-FileHash $zipPath -Algorithm SHA256).Hash.ToLowerInvariant()
  if ($expected -ne $actual) { throw 'Node SHA256-Prüfung fehlgeschlagen.' }
  Expand-Archive $zipPath -DestinationPath (Join-Path $root 'Tools') -Force
}
$nodePath = Join-Path $nodeCache 'node.exe'
$nodeNpm = Join-Path $nodeCache 'node_modules\npm\bin\npm-cli.js'
if (!(Test-Path $nodePath) -or !(Test-Path $nodeNpm)) { throw 'Node-Laufzeit unvollständig.' }
$runtime = Join-Path $package 'runtime'
New-Item -ItemType Directory -Force -Path $runtime | Out-Null
Copy-Item $nodePath $runtime
& $nodePath $nodeNpm install --omit=dev --no-audit --no-fund --prefix $localServer
Check 'USB-Webserver-Abhängigkeiten'
@'
@echo off
setlocal
cd /d "%~dp0"
if not exist "data\usb-profile.json" (
  echo USB-Profil fehlt. Bitte eine der drei USB-Exportvarianten verwenden.
  pause
  exit /b 1
)
set "IPPONBOARD_USB_OFFLINE=1"
set "IPPONBOARD_DATA_DIR=%~dp0data"
set "PORT=43181"
set "HOST=127.0.0.1"
start "" /B "%~dp0runtime\node.exe" "%~dp0local-server\server.js" >NUL 2>&1
start "" "%~dp0app\Ipponboard-Meschede.exe"
'@ | Set-Content (Join-Path $package 'START_IPPONBOARD_MESCHEDE.cmd') -Encoding Ascii
# Build a GUI launcher without requiring installation on the target machine.
$csc = Join-Path $env:WINDIR 'Microsoft.NET\\Framework\\v4.0.30319\\csc.exe'
if (!(Test-Path $csc)) { throw 'Windows .NET Framework C# compiler fehlt.' }
$launcher = Join-Path $source 'docs\\project\\UsbLauncher.cs'
$launcherExe = Join-Path $package 'Ipponboard-Meschede.exe'
# Create one scoreboard emblem for the launcher EXE and the Qt windows.
$iconPath = Join-Path $root 'USB-Build\\ipponboard-meschede.ico'
$pngPath = Join-Path $root 'USB-Build\\ipponboard-meschede.png'
& (Join-Path $source 'docs\\project\\04_SCOREBOARD_ICON_ERZEUGEN.ps1') -IconPath $iconPath -PngPath $pngPath
if (!(Test-Path $iconPath) -or !(Test-Path $pngPath)) { throw 'Scoreboard-Symbol nicht erstellt.' }
Copy-Item $pngPath (Join-Path $app 'app-icon.png') -Force
& $csc /nologo /target:winexe "/win32icon:$iconPath" "/out:$launcherExe" /reference:System.Windows.Forms.dll $launcher
Check 'Portable EXE Starter'
if (!(Test-Path (Join-Path $package 'Ipponboard-Meschede.exe'))) { throw 'USB-Start-EXE fehlt.' }
if (Test-Path (Join-Path $desktop 'LICENSE.txt')) { Copy-Item (Join-Path $desktop 'LICENSE.txt') $package }
# Preserve the downloaded masterdata snapshot for an offline-first USB test.
$dataDir = Join-Path $package 'data'
New-Item -ItemType Directory -Force -Path $dataDir | Out-Null
$localSnapshot = Join-Path $desktop '_bin\data\masterdata.json'
if (!(Test-Path $localSnapshot)) { $localSnapshot = Join-Path $desktop '_bin\Ipponboard-Meschede-Release\data\masterdata.json' }
# Do not copy developer snapshots into any USB export template.
# Empty USB bundles carry no server snapshot or active tournament records.
# The application must not silently inherit data from its creator's machine.

$zip = "$package.zip"
if (Test-Path $zip) { Remove-Item $zip -Force }
Compress-Archive -Path (Join-Path $package '*') -DestinationPath $zip -CompressionLevel Optimal
Write-Host "FERTIG: $zip"
Write-Host 'USB-Test: ZIP entpacken, START_IPPONBOARD_MESCHEDE.cmd auf fremdem Windows-PC starten.'
