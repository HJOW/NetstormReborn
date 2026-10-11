# 원본 실행과 OS 입력 없이 클론의 공통 마우스 입력·화면 전환·설정 저장을 검사한다.
# 옵션 목록 좌표는 2026-10-10 의 원본 치수(화면 중심 기준 목록 왼쪽 +39·위 −90, 행 17, 구분 16, 하위 목록은 부모 오른쪽)에 맞춘 값이다.
# 전체화면 왕복과 전체화면으로 저장한 뒤 재실행하는 경로를 실제 그래픽 장치 상태로 확인한다.
param([string]$OutputDirectory = 'extracted/screens/ui-smoke-20261001')
$ErrorActionPreference = 'Stop'
$taskRoot = Split-Path -Parent $PSScriptRoot
$taskOutput = Join-Path $taskRoot $OutputDirectory
New-Item -ItemType Directory -Force -Path $taskOutput | Out-Null
$taskSettings = Join-Path $taskOutput 'settings'
New-Item -ItemType Directory -Force -Path $taskSettings | Out-Null
Set-Content -LiteralPath (Join-Path $taskSettings 'settings.json') -Encoding UTF8 -Value '{"WindowWidth":1024,"WindowHeight":768,"ViewHeight":768,"SoundVolume":3,"MusicVolume":2,"ScriptValues":{"DoneTheWarBegins":"1"}}'
$taskScript = @"
assert main; assert-detail fullscreen=false; capture $OutputDirectory/01-main.png;
click-center -120,-64; assert campaigns; capture $OutputDirectory/02-campaigns.png;
click-center 0,10; assert campaigns;
click-center 0,28; assert missions; capture $OutputDirectory/03-missions.png;
click-center 0,74; assert missions;
click-center 0,38; assert briefing; assert-detail mission=thewarbegins; capture $OutputDirectory/04-briefing.png;
click-center 82,114; assert battle; wait 90; capture $OutputDirectory/05-battle.png;
click 40,181; assert placement; capture $OutputDirectory/06-placement.png;
right-click 500,400; assert battle; capture $OutputDirectory/07-bridges.png;
click 22,47; assert holding; capture $OutputDirectory/08-held.png;
key Escape; assert mission-menu; click 109,8; click 150,77; assert leave; capture $OutputDirectory/09-leave.png;
click-center -82,48; assert main; capture $OutputDirectory/10-return.png;
click-center -120,-64; assert campaigns; click-center 0,28; assert missions;
click-center 0,56; assert briefing; assert-detail mission=masterofwhirligigs; capture $OutputDirectory/10a-briefing-1-2.png;
click-center 83,131; assert battle; wait 90; capture $OutputDirectory/10b-battle-1-2.png;
key Escape; assert mission-menu; click 109,8; click 150,77; assert leave; click-center -82,48; assert main;
click-center 38,-42; assert options; capture $OutputDirectory/11-options.png;
click-center 138,36; capture $OutputDirectory/12-volume.png; click-center 256,87; assert main;
click-center 38,-42; click-center 138,53; click-center 256,87; assert main;
click-center 38,-42; click-center 138,-65; capture $OutputDirectory/13-resolutions.png; click-center 256,-14; assert main; wait 45; assert-detail display=1280x720; capture $OutputDirectory/14-wide.png;
click-center 38,-42; click-center 138,-82; assert main; wait 45; assert-detail fullscreen=true; capture $OutputDirectory/15-fullscreen.png;
click-center 38,-42; click-center 138,-82; assert main; wait 45; assert-detail fullscreen=false; assert-detail display=1280x720; capture $OutputDirectory/16-windowed.png;
quit;
"@
# 같은 격리 설정으로 클론을 차례로 실행하고 제한 시간·종료 코드·검사 로그를 확인한다.
function Invoke-CloneUiCheck([string]$Name, [string]$Commands) {
    $taskFile = Join-Path $taskOutput "$Name.txt"
    Set-Content -LiteralPath $taskFile -Encoding UTF8 -Value $Commands
    $taskExe = Join-Path $taskRoot 'dotnetpj/src/Netstorm.Game/bin/Release/net10.0/NetstormClone.exe'
    $taskLog = Join-Path $taskOutput "$Name.stdout.log"
    $taskError = Join-Path $taskOutput "$Name.stderr.log"
    $taskProcess = Start-Process -FilePath $taskExe -WorkingDirectory $taskRoot -ArgumentList "--language korean --ui-script-file $OutputDirectory/$Name.txt" -WindowStyle Hidden -PassThru -RedirectStandardOutput $taskLog -RedirectStandardError $taskError
    # PowerShell 5.1에서도 종료 코드를 읽을 수 있도록 실행 중에 핸들을 확보한다.
    $null = $taskProcess.Handle
    if (-not $taskProcess.WaitForExit(120000)) { $taskProcess.Kill(); throw "$Name UI 검사 제한 시간 초과" }
    if ($taskProcess.ExitCode -ne 0) { throw (Get-Content -Raw $taskError) }
    Get-Content $taskLog
}
$taskOldSettings = $env:NETSTORM_SETTINGS_DIR
try {
    $env:NETSTORM_SETTINGS_DIR = $taskSettings
    Invoke-CloneUiCheck 'commands' $taskScript
    $taskSaved = Get-Content -Raw -Encoding UTF8 (Join-Path $taskSettings 'settings.json') | ConvertFrom-Json
    if ($taskSaved.Fullscreen -or $taskSaved.WindowWidth -ne 1280 -or $taskSaved.WindowHeight -ne 720 -or $taskSaved.SoundVolume -ne 4 -or $taskSaved.MusicVolume -ne 3 -or $taskSaved.StartupInProgress -or $taskSaved.ScriptValues.DoneTheWarBegins -ne '1') { throw 'Settings persistence failed' }
    Write-Output 'UI PASS: saved resolution / windowed / sound 4 / music 3'
    # 창 모드 설정을 읽어 전체화면으로 종료한 뒤, 저장된 전체화면에서 다시 시작해 창 모드로 복귀한다.
    Invoke-CloneUiCheck 'save-fullscreen' "assert main; assert-detail fullscreen=false; assert-detail display=1280x720; click-center 38,-42; click-center 138,-82; wait 45; assert-detail fullscreen=true; quit;"
    $taskSaved = Get-Content -Raw -Encoding UTF8 (Join-Path $taskSettings 'settings.json') | ConvertFrom-Json
    if (!$taskSaved.Fullscreen -or $taskSaved.StartupInProgress) { throw '전체화면 종료 설정 저장 실패' }
    Invoke-CloneUiCheck 'restart-fullscreen' "assert main; assert-detail fullscreen=true; capture $OutputDirectory/17-restarted-fullscreen.png; click-center 38,-42; click-center 138,-82; wait 45; assert-detail fullscreen=false; assert-detail display=1280x720; quit;"
    Invoke-CloneUiCheck 'restart-windowed' "assert main; assert-detail fullscreen=false; assert-detail display=1280x720; capture $OutputDirectory/18-restarted-windowed.png; quit;"
    $taskSaved = Get-Content -Raw -Encoding UTF8 (Join-Path $taskSettings 'settings.json') | ConvertFrom-Json
    if ($taskSaved.Fullscreen -or $taskSaved.StartupInProgress) { throw '창 모드 복귀·재실행 설정 저장 실패' }
    Write-Output 'UI PASS: fullscreen saved / fullscreen restart / windowed restart'
} finally { $env:NETSTORM_SETTINGS_DIR = $taskOldSettings }
