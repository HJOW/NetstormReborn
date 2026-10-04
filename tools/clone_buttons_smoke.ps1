# 원본 실행과 OS 입력 없이 클론의 공통 버튼 동작을 검사한다.
# 근거: 2026-10-03 원본 메인 메뉴 자동 분석(docs/videos/menu-buttons-20261003.md) — 돌 버튼은 누르는 순간 소리·눌린 모양,
# 눌린 채 안쪽에서 뗄 때 실행, 메뉴 항목(펼침 목록·목록 행)은 누르는 순간 실행, 호버 표시·키보드 조작 없음.
# 1024x768 창의 고정 좌표를 쓰므로 창 크기를 바꿔 실행하지 않는다.
param([string]$OutputDirectory = 'extracted/screens/buttons-smoke-20261004')
$ErrorActionPreference = 'Stop'
# 저장소 루트와 결과 폴더
$taskRoot = Split-Path -Parent $PSScriptRoot
$taskOutput = Join-Path $taskRoot $OutputDirectory
New-Item -ItemType Directory -Force -Path $taskOutput | Out-Null
# 시나리오 이름 → (설정 JSON, 명령). 시나리오마다 새 설정 폴더를 써서 사용자 설정을 건드리지 않는다.
$taskScenarios = [ordered]@{
    # 시작 팁 창의 버튼: 눌린 채 바깥에서 떼면 불발, 정상 클릭은 다음 팁, OK 는 눌러서 안쪽에서 뗄 때 닫힘
    'tips' = @{
        # 자동 UI 검사는 --tips 를 줘야 시작 팁 창을 연다
        Arguments = '--tips'
        Settings = '{"WindowWidth":1024,"WindowHeight":768,"ViewHeight":768,"SoundVolume":3,"MusicVolume":2,"TellTips":true,"TipNumber":3}'
        Commands = @"
assert tips; wait 20;
down 462,441; wait 4; assert-detail pressed=Next Tip; assert-detail audio-buttons=1; assert-detail audio-last=button.wav;
held-move 200,441; wait 4; assert-detail pressed=none; up; wait 6; assert tips; assert-detail audio-buttons=1;
down 462,441; wait 4; up; wait 6; assert tips; assert-detail audio-buttons=2;
down 662,441; wait 4; assert tips; assert-detail pressed=OK; wait 60; assert tips; assert-detail audio-buttons=3; up; wait 6; assert main;
capture $OutputDirectory/tips-closed.png; quit;
"@
    }
    # 메인 메뉴 돌 버튼과 펼침 메뉴·목록 행
    'menu' = @{
        Arguments = '--no-tips'
        Settings = '{"WindowWidth":1024,"WindowHeight":768,"ViewHeight":768,"SoundVolume":3,"MusicVolume":2,"TellTips":false}'
        Commands = @"
assert main; wait 20; capture $OutputDirectory/menu-normal.png;
move 393,320; wait 10; assert-detail pressed=none; capture $OutputDirectory/menu-hover.png;
down 393,320; wait 4; assert main; assert-detail pressed=Campaign; assert-detail audio-buttons=1; assert-detail audio-last=button.wav; capture $OutputDirectory/menu-pressed.png;
wait 90; assert main; assert-detail pressed=Campaign; assert-detail audio-buttons=1;
up; wait 6; assert campaigns; assert-detail pressed=none; assert-detail audio-buttons=1;
click 512,514; wait 6; assert main; assert-detail audio-buttons=2;
down 393,320; wait 3; held-move 393,430; wait 4; assert-detail pressed=none; up; wait 6; assert main; assert-detail audio-buttons=3;
down 393,320; wait 3; held-move 393,430; wait 24; assert-detail pressed=none; held-move 393,322; wait 4; assert-detail pressed=Campaign; assert-detail audio-buttons=4; up; wait 6; assert campaigns;
click 512,514; wait 6; assert main; assert-detail audio-buttons=5;
down 393,430; wait 3; held-move 393,320; wait 4; assert-detail pressed=none; up; wait 6; assert main; assert-detail audio-buttons=5;
down 393,320; wait 3; held-move 551,343; wait 4; assert-detail pressed=none; up; wait 6; assert main; assert-detail audio-buttons=6;
right-click 393,320; wait 6; assert main; middle-click 393,320; wait 6; assert main; assert-detail audio-buttons=6;
click 551,320; wait 6; assert main; assert-detail audio-buttons=6;
click 355,320; wait 6; assert main; click 431,320; wait 6; assert main; click 393,310; wait 6; assert main; click 393,331; wait 6; assert main; assert-detail audio-buttons=6;
click 356,320; wait 6; assert campaigns; click 512,514; wait 6; assert main;
click 430,320; wait 6; assert campaigns; click 512,514; wait 6; assert main;
click 393,311; wait 6; assert campaigns; click 512,514; wait 6; assert main;
click 393,330; wait 6; assert campaigns; click 512,514; wait 6; assert main; assert-detail audio-buttons=14;
click 551,343; wait 6; assert options; assert-detail audio-buttons=15;
down 620,409; wait 3; assert main; assert-detail audio-last=openSubGump.wav; assert-detail audio-buttons=15; held-move 900,550; wait 4; up; wait 6; assert main;
click 551,343; wait 6; assert options; assert-detail audio-buttons=16;
down 900,550; wait 3; assert main; up; wait 6; assert main; assert-detail audio-buttons=16;
click 551,343; wait 6; assert options; assert-detail audio-buttons=17;
down 551,343; wait 3; assert main; assert-detail pressed=none; up; wait 6; assert main; assert-detail audio-buttons=17;
click 393,320; wait 6; assert campaigns; assert-detail audio-buttons=18;
down 512,415; wait 3; assert missions; assert-detail audio-last=openSubGump.wav; assert-detail audio-buttons=18; up; wait 6; assert missions;
quit;
"@
    }
}
$taskOldSettings = $env:NETSTORM_SETTINGS_DIR
try {
    $taskExe = Join-Path $taskRoot 'dotnetpj/src/Netstorm.Game/bin/Release/net10.0/NetstormClone.exe'
    # 시나리오마다 클론을 따로 실행하고 종료 코드·통과 줄 수를 확인한다.
    foreach ($taskName in $taskScenarios.Keys) {
        $taskSettings = Join-Path $taskOutput "$taskName-settings"
        New-Item -ItemType Directory -Force -Path $taskSettings | Out-Null
        Set-Content -LiteralPath (Join-Path $taskSettings 'settings.json') -Encoding UTF8 -Value $taskScenarios[$taskName].Settings
        $taskFile = Join-Path $taskOutput "$taskName.txt"
        Set-Content -LiteralPath $taskFile -Encoding UTF8 -Value $taskScenarios[$taskName].Commands
        $taskLog = Join-Path $taskOutput "$taskName.stdout.log"
        $env:NETSTORM_SETTINGS_DIR = $taskSettings
        $taskProcess = Start-Process -FilePath $taskExe -WorkingDirectory $taskRoot -ArgumentList "--language english $($taskScenarios[$taskName].Arguments) --ui-script-file $OutputDirectory/$taskName.txt" -WindowStyle Hidden -PassThru -Wait -RedirectStandardOutput $taskLog -RedirectStandardError (Join-Path $taskOutput "$taskName.stderr.log")
        if ($taskProcess.ExitCode -ne 0) { throw "$taskName 시나리오 실패: " + (Get-Content -Raw (Join-Path $taskOutput "$taskName.stderr.log")) }
        $taskPassed = @(Select-String -LiteralPath $taskLog -Pattern 'UI PASS').Count
        Write-Output "BUTTONS PASS: $taskName ($taskPassed checks)"
    }
    # 저장된 설정으로 두 가지를 확인한다: 팁 번호(드래그로 취소한 Next Tip 은 번호를 올리지 않음)와 눌러서 실행한 목록 행(Speaker Swap 한 번 토글)
    $taskTips = Get-Content -Raw -Encoding UTF8 (Join-Path $taskOutput 'tips-settings/settings.json') | ConvertFrom-Json
    if ($taskTips.TipNumber -ne 5 -or -not $taskTips.TellTips) { throw "팁 번호 검사 실패: TipNumber=$($taskTips.TipNumber)" }
    Write-Output 'BUTTONS PASS: tip number 5 (drag-out Next Tip cancelled, click advanced once)'
    $taskMenu = Get-Content -Raw -Encoding UTF8 (Join-Path $taskOutput 'menu-settings/settings.json') | ConvertFrom-Json
    if (-not $taskMenu.SpeakerSwap) { throw 'Speaker Swap 행이 누르는 순간 한 번 켜지지 않았습니다.' }
    Write-Output 'BUTTONS PASS: Speaker Swap toggled once on press'
} finally { $env:NETSTORM_SETTINGS_DIR = $taskOldSettings }
