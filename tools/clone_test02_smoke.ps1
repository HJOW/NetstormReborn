# 원본 실행·OS 입력 없이 TEST02 분석을 반영한 클론의 네이티브 커서·배치·취소·완공을 검사한다.
# 언어별 글자 폭이 달라도 건설 목록과 원본 커서 전환을 같은 좌표로 검사한다.
param([string]$OutputDirectory = 'extracted/screens/test02-implementation/test02',
    [ValidateSet('korean', 'english')][string]$Language = 'korean')
$ErrorActionPreference = 'Stop'
# 저장소와 기존 Release 출력, 격리된 설정 및 PNG 경로.
$taskRoot = Split-Path -Parent $PSScriptRoot
$taskOutput = Join-Path $taskRoot $OutputDirectory
$taskSettings = Join-Path $taskOutput 'settings'
New-Item -ItemType Directory -Force -Path $taskSettings | Out-Null
Set-Content -LiteralPath (Join-Path $taskSettings 'settings.json') -Encoding UTF8 -Value '{"WindowWidth":1024,"WindowHeight":768,"ViewHeight":768,"PlayMusic":false,"WindNoise":false}'
# TEST02 시작 화면의 사제·집·넓은 지면을 누른다. 건설 메뉴 좌표는 원본과 같은 상대 배치를 사용한다.
$taskScenarios = [ordered]@{
    'cursors' = @"
wait 40; assert briefing; click-center 82,79; wait 20; assert battle; assert-detail sp=500000;
assert-detail pointer=arrow; assert-detail pointer-active=True;
click-center 8,-4; assert-detail selected=priest; assert-detail pointer=forbidden;
move-center 48,48; assert-detail pointer=move;
move 110,70; assert-detail pointer=forbidden;
move 595,288; assert-detail pointer=forbidden;
right-click 526,393; assert battle; assert-detail selected=none; assert-detail pointer=arrow;
right-click 526,393; assert context:main; click 577,414; assert context:construct;
click 745,431; assert context:workshops; assert-detail context-panels=3; capture $OutputDirectory/cursors-menu.png;
click 745,434; assert placement; assert-detail audio-last=openSubGump.wav;
move 760,470; assert-detail pointer=place; assert-detail pointer-active=True; assert-detail preview-size=7x8;
assert-detail placement=allowed; capture $OutputDirectory/cursors-preview.png;
move 110,70; assert-detail pointer=place; assert-detail-not placement=allowed;
right-click 110,70; assert battle; assert-detail pointer=arrow; quit;
"@
    'cancel' = @"
wait 40; assert briefing; click-center 82,79; wait 20; assert battle;
right-click 526,393; click 577,414; click 745,431; click 745,434; assert placement;
move 760,470; assert-detail placement=allowed; click 760,470; wait 8;
assert-detail sites=sunfactory:waiting; assert-detail sp=499200; assert-detail audio-last=dropPiece.wav;
capture $OutputDirectory/cancel-site.png;
key P; click 560,450; wait 2; assert-detail-not sunfactory:waiting;
assert-detail sp=500000; assert-detail refunds=1; capture $OutputDirectory/cancel-refund-pending.png;
wait 160; assert-detail refunds=0; assert-detail shown-sp=500000;
capture $OutputDirectory/cancel-refunded.png; quit;
"@
    'build' = @"
wait 40; assert briefing; click-center 82,79; wait 20; assert battle;
right-click 526,393; click 577,414; click 745,414; assert context:temples;
click 745,439; assert placement; move 780,220; assert-detail placement=allowed;
capture $OutputDirectory/temple-preview.png; click 780,220; wait 8;
assert-detail sites=rainvortex:waiting; assert-detail sp=495000; assert-detail pointer=arrow;
capture $OutputDirectory/temple-waiting.png;
wait-detail rainvortex:building; capture $OutputDirectory/temple-started.png;
wait 120; capture $OutputDirectory/temple-rising.png;
key P; click 560,470; wait 6; assert-detail sites=rainvortex:building;
wait 700; assert-detail-not rainvortex:building; assert-detail-not rainvortex:waiting;
assert-detail audio-last=templeComplete.wav; capture $OutputDirectory/temple-complete.png;
key P; move 780,240; assert-detail pointer=temple; assert-detail pointer-active=True; quit;
"@
}
$taskOldSettings = $env:NETSTORM_SETTINGS_DIR
try {
    $env:NETSTORM_SETTINGS_DIR = $taskSettings
    $taskExe = Join-Path $taskRoot 'dotnetpj/src/Netstorm.Game/bin/Release/net10.0/NetstormClone.exe'
    # 클론을 하나씩 실행해 설정·그래픽 장치가 서로 충돌하지 않게 한다.
    foreach ($taskName in $taskScenarios.Keys) {
        $taskScript = Join-Path $taskOutput "$taskName.txt"
        Set-Content -LiteralPath $taskScript -Encoding UTF8 -Value $taskScenarios[$taskName]
        $taskLog = Join-Path $taskOutput "$taskName.stdout.log"
        $taskError = Join-Path $taskOutput "$taskName.stderr.log"
        $taskProcess = Start-Process -FilePath $taskExe -WorkingDirectory $taskRoot -WindowStyle Hidden -PassThru -ArgumentList "--language $Language --test-battle TEST02 --no-sound --no-music --ui-script-file $OutputDirectory/$taskName.txt" -RedirectStandardOutput $taskLog -RedirectStandardError $taskError
        $null = $taskProcess.Handle
        if (!$taskProcess.WaitForExit(90000)) { $taskProcess.Kill(); throw "TEST02 $taskName 검사 시간 초과" }
        if ($taskProcess.ExitCode -ne 0) { throw (Get-Content -LiteralPath $taskError -Raw -Encoding UTF8) }
        $taskPassed = @(Select-String -LiteralPath $taskLog -Pattern 'UI PASS').Count
        Write-Output "TEST02 PASS: $Language/$taskName ($taskPassed checks)"
    }
} finally { $env:NETSTORM_SETTINGS_DIR = $taskOldSettings }
