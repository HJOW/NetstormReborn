# 원본 실행·OS 입력 없이 클론의 사제 건설 규칙을 실제 화면에서 검사한다.
#  menu : 이미 템플이 있으면 Temple 줄이 어둡고(눌러도 하위 창이 열리지 않음), 지식이 없는 원소의 워크샵 줄도 어둡다. Sun Workshop 줄만 활성이다.
#  build: 먼 자리에 워크샵을 설치하면 비용이 바로 나가고 공사장이 생기지만, 사제가 걸어가 도착해야 건설이 시작되어 완공된다.
# 캠페인 1-1 시작 화면의 고정 좌표(1024x768 창)를 쓰므로 창 크기를 바꿔 실행하지 않는다.
param([string]$OutputDirectory = 'extracted/screens/construct-smoke-20261004')
$ErrorActionPreference = 'Stop'
# 저장소 루트와 결과 폴더
$taskRoot = Split-Path -Parent $PSScriptRoot
$taskOutput = Join-Path $taskRoot $OutputDirectory
New-Item -ItemType Directory -Force -Path $taskOutput | Out-Null
# 실행마다 새 설정 폴더(1024x768 창)를 써서 사용자 설정을 건드리지 않는다.
$taskSettings = Join-Path $taskOutput 'settings'
New-Item -ItemType Directory -Force -Path $taskSettings | Out-Null
Set-Content -LiteralPath (Join-Path $taskSettings 'settings.json') -Encoding UTF8 -Value '{"WindowWidth":1024,"WindowHeight":768,"ViewHeight":768,"SoundVolume":3,"MusicVolume":2}'
# 시나리오 이름 → 명령. 사제 우클릭(526,393) → Construct(577,414) → Workshop(745,431) → Sun Workshop(745,434).
# 건설 하위 창의 줄: Temple(745,414)·Workshop(745,431), 워크샵 하위 창의 줄: Sun(434)·Wind(455)·Rain(476)·Thunder(497).
# build 는 사제(525,380)에서 약 23칸 떨어진 섬 오른쪽 (920,470)에 짓는다. 사제는 약 13초를 걸은 뒤 약 10초 건설한다.
$taskScenarios = [ordered]@{
    'menu' = @"
wait 40; assert briefing; click-center 83,113; wait 20; assert battle; wait 60;
right-click 526,393; assert context:main; click 577,414; assert context:construct; capture $OutputDirectory/menu-construct.png;
click 745,414; assert context:construct;
click 745,431; assert context:workshops; capture $OutputDirectory/menu-workshops.png;
click 745,455; assert context:workshops; click 745,476; assert context:workshops; click 745,497; assert context:workshops;
click 745,434; assert placement; quit;
"@
    'build' = @"
wait 40; assert briefing; click-center 83,113; wait 20; assert battle; wait 60;
assert-detail-not waiting; assert-detail-not building;
right-click 526,393; assert context:main; click 577,414; assert context:construct; click 745,431; assert context:workshops; click 745,434; assert placement;
move 920,470; wait 8; assert-detail placement=allowed; capture $OutputDirectory/build-00-preview.png;
click 920,470; wait 6; assert battle; assert-detail sites=sunfactory:waiting; assert-detail sp=2200; capture $OutputDirectory/build-01-waiting.png;
wait 120; assert-detail sites=sunfactory:waiting; assert-detail :moving; capture $OutputDirectory/build-02-walking.png;
wait 900; capture $OutputDirectory/build-03-arrived.png;
wait 4000; assert-detail-not waiting; assert-detail-not building; assert-detail workshops=sunfactory:1; capture $OutputDirectory/build-04-done.png; quit;
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
        $taskProcess = Start-Process -FilePath $taskExe -WorkingDirectory $taskRoot -ArgumentList "--language korean --mission thewarbegins --no-sound --no-music --ui-script-file $OutputDirectory/$taskName.txt" -WindowStyle Hidden -PassThru -RedirectStandardOutput $taskLog -RedirectStandardError (Join-Path $taskOutput "$taskName.stderr.log")
        # 종료 전에 프로세스 핸들을 확보해야 PowerShell 5.1에서도 종료 코드를 읽을 수 있다.
        $null = $taskProcess.Handle
        # 입력 스크립트 오류로 창이 계속 열려 있는 경우도 제한 시간 안에 검출한다.
        if (!$taskProcess.WaitForExit(120000)) { $taskProcess.Kill(); throw "$taskName 시나리오 제한 시간 초과" }
        if ($taskProcess.ExitCode -ne 0) { throw "$taskName 시나리오 실패: " + (Get-Content -Raw (Join-Path $taskOutput "$taskName.stderr.log")) }
        $taskPassed = @(Select-String -LiteralPath $taskLog -Pattern 'UI PASS').Count
        Write-Output "CONSTRUCT PASS: $taskName ($taskPassed checks)"
    }
} finally { $env:NETSTORM_SETTINGS_DIR = $taskOldSettings }
