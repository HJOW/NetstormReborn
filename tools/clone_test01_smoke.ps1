# 원본 실행·OS 입력 없이 TEST01의 선택·배치·미니맵·카메라 저장을 네 화면/언어 조합으로 검사한다.
param([string]$OutputDirectory = 'extracted/screens/test01-verify-20261003/test01')
$ErrorActionPreference = 'Stop'
# 저장소와 Release 실행 파일. 설정과 PNG는 Git 제외 검사 폴더에만 쓴다.
$taskRoot = Split-Path -Parent $PSScriptRoot
$taskExe = Join-Path $taskRoot 'dotnetpj/src/Netstorm.Game/bin/Release/net10.0/NetstormClone.exe'
$taskOldSettings = $env:NETSTORM_SETTINGS_DIR
# 125% 원본 녹화 크기·두 와이드 화면비와 영어/한국어를 번갈아 확인한다.
$taskCases = @(
    @{ Name = '4x3-english'; Width = 1280; Height = 960; Language = 'english' },
    @{ Name = '4x3-korean'; Width = 1024; Height = 768; Language = 'korean' },
    @{ Name = '16x9-english'; Width = 1280; Height = 720; Language = 'english' },
    @{ Name = '16x10-korean'; Width = 1280; Height = 800; Language = 'korean' }
)
try {
    # UI는 논리 높이 768로 고정하고 중심 기준 입력으로 같은 월드 위치를 누른다.
    foreach ($taskCase in $taskCases) {
        $taskRelative = "$OutputDirectory/$($taskCase.Name)"
        $taskOutput = Join-Path $taskRoot $taskRelative
        $taskSettings = Join-Path $taskOutput 'settings'
        New-Item -ItemType Directory -Force -Path $taskSettings | Out-Null
        $taskConfig = @{ WindowWidth = $taskCase.Width; WindowHeight = $taskCase.Height; ViewHeight = 768;
            PlayMusic = $false; WindNoise = $false }
        Set-Content -LiteralPath (Join-Path $taskSettings 'settings.json') -Encoding UTF8 -Value ($taskConfig | ConvertTo-Json)
        $taskScript = @"
wait 60; assert briefing; capture $taskRelative/00-briefing.png;
click-center 82,79; wait 40; assert battle; assert-detail camera=2720,2210;
assert-detail sunfactory:3; assert-detail thunderfactory:2; assert-detail thunderfactory:3;
capture $taskRelative/01-start.png;
click-center 8,-4; assert-detail selected=priest; capture $taskRelative/02-priest.png;
click-center 32,-111; assert-detail selected=rainballoon; capture $taskRelative/03-balloon.png;
click-center -20,-204; assert-detail selected=windwalker; capture $taskRelative/04-sail.png;
click-center 388,-54; assert-detail selected=sunfactory; capture $taskRelative/05-workshop.png;
click-center 308,-19; assert-detail selected=thundervortex; capture $taskRelative/06-temple.png;
key F7; capture $taskRelative/07-neutral-rims.png; key F7;
key LeftShift+D1; click 70,700; assert battle; assert-detail-not camera=2720,2210;
capture $taskRelative/08-minimap-click.png;
drag 10,700,70,740,20; assert-detail-not camera=2720,2210; capture $taskRelative/09-minimap-drag.png;
key D1; assert-detail camera=2720,2210; capture $taskRelative/10-camera-restored.png;
click 40,181; assert placement; move-center 188,66; assert-detail placement=allowed;
capture $taskRelative/11-placement.png; move 130,70; capture $taskRelative/12-blocked.png;
right-click 500,400; assert battle;
click 40,368; assert placement; assert-detail cursor=raincannon; move-center 188,66;
capture $taskRelative/13-ice-cannon.png; right-click 700,450; assert-detail rotation=1;
capture $taskRelative/14-ice-rotated.png; key Escape; key Escape;
click 40,275; assert placement; assert-detail cursor=thundercannon; move-center 188,66;
capture $taskRelative/15-thunder-cannon.png; right-click 700,450; assert-detail rotation=1;
capture $taskRelative/16-thunder-rotated.png; quit;
"@
        Set-Content -LiteralPath (Join-Path $taskOutput 'commands.txt') -Encoding UTF8 -Value $taskScript
        $env:NETSTORM_SETTINGS_DIR = $taskSettings
        $taskArguments = "--test-battle TEST01 --language $($taskCase.Language) --no-sound --no-music --script `"register rainCannon; register thunderCannon`" --ui-script-file $taskRelative/commands.txt"
        $taskProcess = Start-Process -FilePath $taskExe -WorkingDirectory $taskRoot -ArgumentList $taskArguments -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $taskOutput 'stdout.log') -RedirectStandardError (Join-Path $taskOutput 'stderr.log')
        # 핸들을 미리 확보해야 PowerShell 5.1에서 종료 코드를 확인할 수 있다.
        $null = $taskProcess.Handle
        if (!$taskProcess.WaitForExit(45000)) { $taskProcess.Kill(); throw "TEST01 $($taskCase.Name) 제한 시간 초과" }
        if ($taskProcess.ExitCode -ne 0) { throw (Get-Content -Raw -LiteralPath (Join-Path $taskOutput 'stderr.log')) }
        $taskPassed = @(Select-String -LiteralPath (Join-Path $taskOutput 'stdout.log') -Pattern 'UI PASS').Count
        Write-Output "TEST01 PASS: $($taskCase.Name) ($taskPassed checks)"
    }
} finally { $env:NETSTORM_SETTINGS_DIR = $taskOldSettings }
