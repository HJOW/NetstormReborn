# 원본 실행과 OS 입력 없이 클론의 공통 마우스 입력·화면 전환·설정 저장을 검사한다.
param([string]$OutputDirectory = 'extracted/screens/ui-smoke-20261001')
$ErrorActionPreference = 'Stop'
$taskRoot = Split-Path -Parent $PSScriptRoot
$taskOutput = Join-Path $taskRoot $OutputDirectory
New-Item -ItemType Directory -Force -Path $taskOutput | Out-Null
$taskSettings = Join-Path $taskOutput 'settings'
New-Item -ItemType Directory -Force -Path $taskSettings | Out-Null
Set-Content -LiteralPath (Join-Path $taskSettings 'settings.json') -Encoding UTF8 -Value '{"WindowWidth":1024,"WindowHeight":768,"ViewHeight":768,"SoundVolume":3,"MusicVolume":2}'
$taskScript = @"
assert main; capture $OutputDirectory/01-main.png;
click-center -120,-64; assert campaigns; capture $OutputDirectory/02-campaigns.png;
click-center 0,-19; assert campaigns;
click-center 0,28; assert missions; capture $OutputDirectory/03-missions.png;
click-center 0,64; assert missions;
click-center 0,27; assert briefing; capture $OutputDirectory/04-briefing.png;
click-center 82,114; assert battle; wait 90; capture $OutputDirectory/05-battle.png;
click 20,176; assert placement; capture $OutputDirectory/06-placement.png;
click 20,344; assert battle;
click 20,260; assert bridges; capture $OutputDirectory/07-bridges.png;
click 22,47; assert holding; capture $OutputDirectory/08-held.png;
click 105,8; assert mission-menu; click 150,78; assert leave; capture $OutputDirectory/09-leave.png;
click-center -82,48; assert main; capture $OutputDirectory/10-return.png;
click-center -120,-64; assert campaigns; click-center 0,28; assert missions;
click-center 0,46; assert briefing; capture $OutputDirectory/10a-briefing-1-2.png;
click-center 83,121; assert battle; wait 90; capture $OutputDirectory/10b-battle-1-2.png;
click 105,8; assert mission-menu; click 150,78; assert leave; click-center -82,48; assert main;
click-center 38,-42; assert options; capture $OutputDirectory/11-options.png;
click-center 148,44; capture $OutputDirectory/12-volume.png; click-center 238,107; assert options;
click-center 148,63; click-center 238,99;
click-center 108,-58; capture $OutputDirectory/13-resolutions.png; click-center 278,-4; assert options; wait 45; capture $OutputDirectory/14-wide.png;
click-center 110,-79; assert options; wait 45; capture $OutputDirectory/15-fullscreen.png; click-center 110,-79; assert options; wait 45; capture $OutputDirectory/16-windowed.png;
quit;
"@
$taskFile = Join-Path $taskOutput 'commands.txt'
Set-Content -LiteralPath $taskFile -Encoding UTF8 -Value $taskScript
$taskOldSettings = $env:NETSTORM_SETTINGS_DIR
try {
    $env:NETSTORM_SETTINGS_DIR = $taskSettings
    $taskExe = Join-Path $taskRoot 'src/Netstorm.Game/bin/Release/net10.0/NetstormClone.exe'
    $taskProcess = Start-Process -FilePath $taskExe -WorkingDirectory $taskRoot -ArgumentList "--language korean --ui-script-file $OutputDirectory/commands.txt" -WindowStyle Hidden -PassThru -Wait -RedirectStandardOutput (Join-Path $taskOutput 'stdout.log') -RedirectStandardError (Join-Path $taskOutput 'stderr.log')
    if ($taskProcess.ExitCode -ne 0) { throw (Get-Content -Raw (Join-Path $taskOutput 'stderr.log')) }
    Get-Content (Join-Path $taskOutput 'stdout.log')
    $taskSaved = Get-Content -Raw -Encoding UTF8 (Join-Path $taskSettings 'settings.json') | ConvertFrom-Json
    if ($taskSaved.Fullscreen -or $taskSaved.WindowWidth -ne 1280 -or $taskSaved.WindowHeight -ne 720 -or $taskSaved.SoundVolume -ne 4 -or $taskSaved.MusicVolume -ne 3) { throw 'Settings persistence failed' }
    Write-Output 'UI PASS: saved resolution / windowed / sound 4 / music 3'
} finally { $env:NETSTORM_SETTINGS_DIR = $taskOldSettings }
