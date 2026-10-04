# 원본 실행·자산 복사·녹화 없이 클론의 생산 자원 운송과 도착 후 실체화를 검사한다.
# 각 시나리오는 기존 Release 출력과 작은 PNG만 사용한다(디스크 여유 공간이 적은 환경용).
param([string]$OutputDirectory = 'extracted/screens/production-smoke-20261004')
$ErrorActionPreference = 'Stop'
# 저장소 루트·기존 클론 실행 파일·격리된 결과 및 설정 경로.
$taskRoot = Split-Path -Parent $PSScriptRoot
$taskExe = Join-Path $taskRoot 'src/Netstorm.Game/bin/Release/net10.0/NetstormClone.exe'
$taskOutput = Join-Path $taskRoot $OutputDirectory
$taskSettings = Join-Path $taskOutput 'settings'
New-Item -ItemType Directory -Force -Path $taskSettings | Out-Null
Set-Content -LiteralPath (Join-Path $taskSettings 'settings.json') -Encoding UTF8 -Value '{"WindowWidth":1024,"WindowHeight":768,"ViewHeight":768,"PlayMusic":false,"WindNoise":false}'
# 골렘은 템플, 포대는 덱 등록에 사용한 워크샵에서 자원을 받아야 한다.
$taskScenarios = [ordered]@{
    'golem' = @{
        Setup = ''
        Commands = @"
wait 40; assert briefing; click-center 83,113; wait 20; assert battle; wait 60;
click 40,181; wait 6; assert placement; click 820,420; wait 2;
assert-detail sunwalker:delivery; assert-detail origin=rainvortex; assert-detail productioncount=1;
assert-detail-not sunwalker:-:idle; capture $OutputDirectory/golem-delivery.png;
wait-detail sunwalker:materializing; capture $OutputDirectory/golem-materializing.png;
wait-detail productioncount=0; assert-detail sunwalker:-:idle;
capture $OutputDirectory/golem-complete.png; quit;
"@
    }
    'workshop' = @{
        Setup = 'construct sunFactory 98,98; wait 60; register sunCannon'
        Commands = @"
wait 40; assert briefing; click-center 83,113; wait 20; assert battle; wait 30;
click 40,275; wait 6; assert placement; assert-detail cursor=suncannon; click 700,340; wait 2;
assert-detail suncannon:delivery; assert-detail origin=sunfactory; assert-detail productioncount=1;
capture $OutputDirectory/cannon-delivery.png;
wait-detail suncannon:materializing; capture $OutputDirectory/cannon-materializing.png;
wait-detail productioncount=0; capture $OutputDirectory/cannon-complete.png; quit;
"@
    }
}
$taskOldSettings = $env:NETSTORM_SETTINGS_DIR
try {
    $env:NETSTORM_SETTINGS_DIR = $taskSettings
    # 한 번에 한 클론만 실행해 GPU·설정·출력 파일이 서로 충돌하지 않게 한다.
    foreach ($taskName in $taskScenarios.Keys) {
        $taskScenario = $taskScenarios[$taskName]
        $taskFile = Join-Path $taskOutput "$taskName.txt"
        Set-Content -LiteralPath $taskFile -Encoding UTF8 -Value $taskScenario.Commands
        $taskArguments = "--language korean --mission thewarbegins --ui-script-file $OutputDirectory/$taskName.txt"
        if ($taskScenario.Setup) { $taskArguments += " --script `"$($taskScenario.Setup)`"" }
        $taskLog = Join-Path $taskOutput "$taskName.stdout.log"
        $taskError = Join-Path $taskOutput "$taskName.stderr.log"
        $taskProcess = Start-Process -FilePath $taskExe -WorkingDirectory $taskRoot -ArgumentList $taskArguments -WindowStyle Hidden -PassThru -RedirectStandardOutput $taskLog -RedirectStandardError $taskError
        # 종료 전에 핸들을 확보해야 PowerShell 5.1에서도 종료 코드를 읽을 수 있다.
        $null = $taskProcess.Handle
        if (-not $taskProcess.WaitForExit(90000)) { $taskProcess.Kill(); throw "$taskName 생산 검사 시간 초과" }
        if ($taskProcess.ExitCode -ne 0) { throw (Get-Content -LiteralPath $taskError -Raw -Encoding UTF8) }
        $taskPassed = @(Select-String -LiteralPath $taskLog -Pattern 'UI PASS').Count
        Write-Output "PRODUCTION PASS: $taskName ($taskPassed checks)"
    }
} finally { $env:NETSTORM_SETTINGS_DIR = $taskOldSettings }
