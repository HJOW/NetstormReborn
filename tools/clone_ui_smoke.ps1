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
click 300,371; assert campaigns; capture $OutputDirectory/02-campaigns.png;
click 512,240; assert campaigns; capture $OutputDirectory/03-locked-group.png;
click 512,325; assert missions; capture $OutputDirectory/04-missions.png;
click 512,343; assert missions; capture $OutputDirectory/05-locked-mission.png;
click 512,305; assert briefing; capture $OutputDirectory/06-briefing.png;
click 607,656; assert battle; capture $OutputDirectory/07-battle.png;
click 70,44; assert placement; capture $OutputDirectory/08-build-cursor.png;
click 803,75; click 950,44; assert bridges; capture $OutputDirectory/09-bridges.png;
click 35,110; assert holding; capture $OutputDirectory/10-bridge-picked.png;
click 40,12; assert mission-menu; click 200,110; assert leave; capture $OutputDirectory/11-leave-confirm.png;
click 315,438; assert main; capture $OutputDirectory/12-main-return.png;
click 580,411; assert options; capture $OutputDirectory/13-options.png;
click 512,387; click 512,461;
click 512,276; assert options; capture $OutputDirectory/14-wide-options.png;
click 640,215; assert options; capture $OutputDirectory/15-fullscreen.png;
click 640,215; assert options; capture $OutputDirectory/16-windowed.png;
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
