# 원본 실행과 OS 입력 없이 클론의 사제·골렘 선택(그림 몸통 클릭)·이동(대각선 우선·방향)·배치 커서 해제를 검사한다.
# 캠페인 1-1 시작 화면의 고정 좌표(1024x768 창)를 쓰므로 창 크기를 바꿔 실행하지 않는다.
param([string]$OutputDirectory = 'extracted/screens/move-smoke-20261003')
$ErrorActionPreference = 'Stop'
# 저장소 루트와 결과 폴더
$taskRoot = Split-Path -Parent $PSScriptRoot
$taskOutput = Join-Path $taskRoot $OutputDirectory
New-Item -ItemType Directory -Force -Path $taskOutput | Out-Null
# 실행마다 새 설정 폴더(1024x768 창)를 써서 사용자 설정을 건드리지 않는다.
$taskSettings = Join-Path $taskOutput 'settings'
New-Item -ItemType Directory -Force -Path $taskSettings | Out-Null
Set-Content -LiteralPath (Join-Path $taskSettings 'settings.json') -Encoding UTF8 -Value '{"WindowWidth":1024,"WindowHeight":768,"ViewHeight":768,"SoundVolume":3,"MusicVolume":2}'
# 시나리오 이름 → 명령. 사제: 몸통 선택 → 동쪽·남동쪽 이동 → 허공 거부. 골렘: 놓기 → 커서 해제 → 몸통 선택 → 이동.
$taskScenarios = [ordered]@{
    'priest' = @"
wait 40; assert briefing; click-center 83,113; wait 20; assert battle; wait 60;
click 525,380; wait 6; assert-detail selected=priest;
click 840,372; wait 6; assert-detail selected=none; assert-detail priest:B:moving; wait 20; capture $OutputDirectory/priest-walk.png;
wait 700; assert-detail priest:C:idle;
click 840,360; wait 6; assert-detail selected=priest;
click 900,500; wait 6; assert-detail selected=none; assert-detail priest:D:moving; wait 500; assert-detail :idle;
click 900,488; wait 6; assert-detail selected=priest;
click 300,200; wait 6; assert-detail selected=none; assert-detail :idle; quit;
"@
    'golem' = @"
wait 40; assert briefing; click-center 83,113; wait 20; assert battle; wait 60;
click 40,181; wait 6; assert placement; click 500,390; wait 90; assert battle; assert-detail sunwalker:-:idle;
click 497,378; wait 6; assert-detail selected=sunwalker;
click 700,392; wait 6; assert-detail selected=none; assert-detail :moving; wait 600; assert-detail :idle; capture $OutputDirectory/golem-arrived.png; quit;
"@
}
$taskOldSettings = $env:NETSTORM_SETTINGS_DIR
try {
    $env:NETSTORM_SETTINGS_DIR = $taskSettings
    $taskExe = Join-Path $taskRoot 'src/Netstorm.Game/bin/Release/net10.0/NetstormClone.exe'
    # 시나리오마다 클론을 따로 실행하고 종료 코드·통과 줄 수를 확인한다.
    foreach ($taskName in $taskScenarios.Keys) {
        $taskFile = Join-Path $taskOutput "$taskName.txt"
        Set-Content -LiteralPath $taskFile -Encoding UTF8 -Value $taskScenarios[$taskName]
        $taskLog = Join-Path $taskOutput "$taskName.stdout.log"
        $taskProcess = Start-Process -FilePath $taskExe -WorkingDirectory $taskRoot -ArgumentList "--language korean --mission thewarbegins --ui-script-file $OutputDirectory/$taskName.txt" -WindowStyle Hidden -PassThru -Wait -RedirectStandardOutput $taskLog -RedirectStandardError (Join-Path $taskOutput "$taskName.stderr.log")
        if ($taskProcess.ExitCode -ne 0) { throw "$taskName 시나리오 실패: " + (Get-Content -Raw (Join-Path $taskOutput "$taskName.stderr.log")) }
        $taskPassed = @(Select-String -LiteralPath $taskLog -Pattern 'UI PASS').Count
        Write-Output "MOVE PASS: $taskName ($taskPassed checks)"
    }
} finally { $env:NETSTORM_SETTINGS_DIR = $taskOldSettings }
