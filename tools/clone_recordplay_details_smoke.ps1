# 원본 실행·OS 입력 없이 녹화에서 복원한 버튼음과 공격 건물 선택·배치 사거리를 검사한다.
param([string]$OutputDirectory = 'extracted/record-play-details-20261003/clone', [string[]]$Cases = @())
$ErrorActionPreference = 'Stop'
# 검사별 설정·입력·로그·PNG는 Git 제외 폴더에 저장하고 평소 사용자 설정은 복원한다.
$taskRoot = Split-Path -Parent $PSScriptRoot
$taskExe = Join-Path $taskRoot 'dotnetpj/src/Netstorm.Game/bin/Release/net10.0/NetstormClone.exe'
$taskOldSettings = $env:NETSTORM_SETTINGS_DIR
$taskCases = @()
# 같은 메뉴·브리핑·미션 종료 입력을 실제 소리 켜짐·꺼짐 두 상태에서 확인한다.
foreach ($taskSound in @($true, $false)) {
    $taskResult = if ($taskSound) { 'played' } else { 'disabled' }
    $taskName = if ($taskSound) { 'audio-on' } else { 'audio-off' }
    $taskCommands = @"
wait 15; assert main; assert-detail audio-buttons=0;
click 470,320; assert main; assert-detail audio-buttons=0;
click 392,320; assert campaigns; assert-detail audio-buttons=1; assert-detail audio-result=$taskResult;
click 512,365; assert campaigns; assert-detail audio-buttons=1;
click 512,412; assert missions; assert-detail audio-buttons=1;
click 512,411; assert briefing; assert-detail audio-buttons=1;
click-center 82,114; assert battle; assert-detail audio-buttons=2; assert-detail audio-result=$taskResult;
key Escape; assert mission-menu; click 109,8; click 150,77; assert leave;
click-center -82,48; assert main; assert-detail audio-buttons=3; assert-detail audio-result=$taskResult; quit;
"@
    $taskCases += @{ Name = $taskName; Sound = $taskSound; Language = 'korean'; Arguments = ''; Commands = $taskCommands }
}
# 동일한 빈 자리(182,209)에 각 공격 유닛을 놓아 원본 TEST01 지면·공급원에서 실제 선택을 검사한다.
foreach ($taskUnit in @(
    @{ Type = 'sunArcher'; Shape = 'circle'; Direction = -1 },
    @{ Type = 'rainCannon'; Shape = 'ray'; Direction = 1 },
    @{ Type = 'thunderCannon'; Shape = 'ray'; Direction = 3 },
    @{ Type = 'sunCannon'; Shape = 'ray'; Direction = -1 },
    @{ Type = 'sunFence'; Shape = 'ray'; Direction = -1 }
)) {
    $taskName = $taskUnit.Type.ToLowerInvariant()
    $taskRelative = "$OutputDirectory/$taskName"
    $taskRange = "${taskName}:$($taskUnit.Shape):$($taskUnit.Direction)"
    $taskRotation = [Math]::Max(0, $taskUnit.Direction)
    $taskCommands = @"
wait 15; assert briefing; click-center 82,79; assert battle;
click-center 176,78; assert-detail selected=$taskName; assert-detail range=$taskRange;
wait 90; capture $taskRelative/01-selected-range.png;
key LeftShift+F9; capture $taskRelative/02-paused.png; wait 30; capture $taskRelative/03-paused-still.png;
key LeftShift+F9; click-center 8,-4; assert-detail selected=priest; assert-detail range=none;
wait 30; capture $taskRelative/04-unselected.png; quit;
"@
    $taskCases += @{ Name = $taskName; Sound = $false;
        Arguments = "--test-battle TEST01 --script `"rules 0; combat 0; place $($taskUnit.Type) 182,209 $taskRotation`"";
        Commands = $taskCommands }
}
# Crossbow는 시작 섬에 Wind 공급원이 없으므로 맵에 저장된 동쪽 방위(M00) 건물을 선택해 검사한다.
$taskCases += @{ Name = 'windarcher'; Sound = $false;
    Arguments = '--test-battle TEST01 --script "combat 0; select 100,157"';
    Commands = @"
wait 15; assert briefing; click-center 82,79; assert battle;
assert-detail selected=windarcher; assert-detail range=windarcher:sector:1;
click 7,704; wait 60; capture $OutputDirectory/windarcher/01-selected-range.png;
key Home; click-center 8,-4; assert-detail selected=priest; assert-detail range=none; quit;
"@ }
# 배치 전 우클릭은 고정 캐논만 회전하고 썬 캐논의 네 방향 표시는 그대로다.
$taskCases += @{ Name = 'placement'; Sound = $false;
    Arguments = '--test-battle TEST01 --script "combat 0; register sunArcher; register rainCannon; register thunderCannon; register sunCannon; register windArcher"';
    Commands = @"
wait 15; click-center 82,79; assert battle; capture $OutputDirectory/placement/00-deck.png;
click 40,181; assert placement; move-center 188,66; assert-detail range=none; key Escape; key Escape;
click 40,368; move-center 188,66; assert-detail cursor=thundercannon; assert-detail range=thundercannon:ray:0;
right-click 700,450; assert-detail range=thundercannon:ray:1;
capture $OutputDirectory/placement/01-thunder-east.png; key Escape; key Escape;
click 40,461; move-center 188,66; assert-detail cursor=raincannon; assert-detail range=raincannon:ray:0;
right-click 700,450; assert-detail range=raincannon:ray:1;
capture $OutputDirectory/placement/02-ice-east.png; key Escape; key Escape;
click 40,648; move-center 188,66; assert-detail cursor=suncannon; assert-detail range=suncannon:ray:-1;
right-click 700,450; assert battle; assert-detail range=none;
click 40,275; move-center 188,66; assert-detail cursor=windarcher; assert-detail range=windarcher:sector:0;
wait 60; capture $OutputDirectory/placement/03-crossbow-north.png;
right-click 700,450; assert placement; assert-detail range=windarcher:sector:1;
wait 60; capture $OutputDirectory/placement/04-crossbow-east.png; key Escape; key Escape;
click 40,554; move-center 188,66; assert-detail cursor=sunarcher; assert-detail range=sunarcher:circle:-1;
wait 30; capture $OutputDirectory/placement/05-disc-thrower.png; quit;
"@ }
# Wind Tower의 배치 전 우클릭은 취소 대신 네 면을 순환하고 사거리 표시는 만들지 않는다.
$taskCases += @{ Name = 'windtower'; Sound = $false;
    Arguments = '--test-battle TEST01 --placement windBlocker --script "combat 0"';
    Commands = @"
wait 15; assert briefing; click-center 82,79; assert placement;
move-center 188,66; assert-detail cursor=windblocker; assert-detail rotation=0; assert-detail range=none;
capture $OutputDirectory/windtower/00-north.png;
right-click 700,450; assert placement; assert-detail rotation=1; capture $OutputDirectory/windtower/01-east.png;
right-click 700,450; assert placement; assert-detail rotation=2; capture $OutputDirectory/windtower/02-south.png;
right-click 700,450; assert placement; assert-detail rotation=3; capture $OutputDirectory/windtower/03-west.png;
right-click 700,450; assert placement; assert-detail rotation=0; quit;
"@ }
try {
    # 각 검사를 독립 설정 폴더와 새 클론 프로세스로 실행해 이전 상태를 넘기지 않는다.
    foreach ($taskCase in $taskCases) {
        if ($Cases.Count -gt 0 -and $taskCase.Name -notin $Cases) { continue }
        $taskRelative = "$OutputDirectory/$($taskCase.Name)"
        $taskOutput = Join-Path $taskRoot $taskRelative
        $taskSettings = Join-Path $taskOutput 'settings'
        New-Item -ItemType Directory -Force -Path $taskSettings | Out-Null
        $taskConfig = @{ WindowWidth = 1024; WindowHeight = 768; ViewHeight = 768;
            SoundOn = $taskCase.Sound; PlayMusic = $false; WindNoise = $false; TellTips = $false }
        Set-Content -LiteralPath (Join-Path $taskSettings 'settings.json') -Encoding UTF8 -Value ($taskConfig | ConvertTo-Json)
        Set-Content -LiteralPath (Join-Path $taskOutput 'commands.txt') -Encoding UTF8 -Value $taskCase.Commands
        $env:NETSTORM_SETTINGS_DIR = $taskSettings
        $taskLanguage = if ($taskCase.Language) { $taskCase.Language } else { 'english' }
        $taskArguments = "--language $taskLanguage $($taskCase.Arguments) --ui-script-file $taskRelative/commands.txt"
        $taskProcess = Start-Process -FilePath $taskExe -WorkingDirectory $taskRoot -ArgumentList $taskArguments -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $taskOutput 'stdout.log') -RedirectStandardError (Join-Path $taskOutput 'stderr.log')
        # PowerShell 5.1에서도 종료 코드를 읽을 수 있도록 프로세스 핸들을 먼저 확보한다.
        $null = $taskProcess.Handle
        if (!$taskProcess.WaitForExit(45000)) { $taskProcess.Kill(); throw "세부 구현 $($taskCase.Name) 검사 제한 시간 초과" }
        if ($taskProcess.ExitCode -ne 0) { throw (Get-Content -Raw -LiteralPath (Join-Path $taskOutput 'stderr.log')) }
        $taskPassed = @(Select-String -LiteralPath (Join-Path $taskOutput 'stdout.log') -Pattern 'UI PASS').Count
        Write-Output "DETAIL PASS: $($taskCase.Name) ($taskPassed checks)"
    }
} finally { $env:NETSTORM_SETTINGS_DIR = $taskOldSettings }
