# 원본 실행·OS 입력 없이 영어/한국어와 메뉴/미션 도움말의 실제 링크·스크롤·고정 능력치 머리를 검사한다.
param([string]$OutputDirectory = 'extracted/screens/dotnet-help-layout')
$ErrorActionPreference = 'Stop'
$taskRoot = Split-Path -Parent $PSScriptRoot
$taskExe = Join-Path $taskRoot 'dotnetpj/src/Netstorm.Game/bin/Release/net10.0/NetstormClone.exe'
Add-Type -AssemblyName System.Drawing

# 사제 도움말의 고정 능력치 머리 또는 복원 본문에서 달라진 화소 수를 센다. 같은 위치·크기의 두 클론 캡처를 받는다.
function Get-CloneHelpPixelDifference([string]$First, [string]$Second, [int]$Top, [int]$Bottom) {
    $taskFirst = [System.Drawing.Bitmap]::new($First)
    $taskSecond = [System.Drawing.Bitmap]::new($Second)
    try {
        if ($taskFirst.Width -ne 1024 -or $taskFirst.Height -ne 768 -or $taskSecond.Width -ne 1024 -or $taskSecond.Height -ne 768) {
            throw 'Clone help image size mismatch'
        }
        $taskDifferent = 0
        # 세로는 호출자가 정한 머리/본문 범위만 검사한다.
        for ($taskY = $Top; $taskY -lt $Bottom; $taskY++) {
            # 본문과 같은 가로 범위(303~706)를 쓰고 움직이는 스크롤바는 제외한다.
            for ($taskX = 303; $taskX -lt 707; $taskX++) {
                if ($taskFirst.GetPixel($taskX,$taskY).ToArgb() -ne $taskSecond.GetPixel($taskX,$taskY).ToArgb()) { $taskDifferent++ }
            }
        }
        return $taskDifferent
    } finally { $taskFirst.Dispose(); $taskSecond.Dispose() }
}
# 1024×768 실제 캡처에서 확인한 첫 내부 링크 중심이다. 이후 링크들은 15픽셀 간격이다.
$taskScenarios = @(
    @{ Language = 'english'; Mission = $false; FirstLink = 144 },
    @{ Language = 'english'; Mission = $true; FirstLink = 159 },
    @{ Language = 'korean'; Mission = $false; FirstLink = 151 },
    @{ Language = 'korean'; Mission = $true; FirstLink = 166 }
)
# 원본 F1Help의 여덟 내부 링크 목적지를 표시된 순서대로 검사한다. 외부 주소는 열지 않는다.
$taskLinks = @('campaignHelp','interfaceHelp','sphereHelp','themeHelp','priestType','unitHelp','vesselpriestHelp','sacrificeOutline')
$taskOldSettings = $env:NETSTORM_SETTINGS_DIR
try {
    # 각 언어/모드를 별도 클론 설정 폴더에서 실행하여 사용자의 저장 상태와 섞이지 않게 한다.
    foreach ($taskScenario in $taskScenarios) {
        $taskMode = if ($taskScenario.Mission) { 'mission' } else { 'menu' }
        $taskRelative = "$OutputDirectory/$($taskScenario.Language)-$taskMode"
        $taskOutput = Join-Path $taskRoot $taskRelative
        $taskSettings = Join-Path $taskOutput 'settings'
        New-Item -ItemType Directory -Force -Path $taskSettings | Out-Null
        Set-Content -LiteralPath (Join-Path $taskSettings 'settings.json') -Encoding UTF8 -Value '{"WindowWidth":1024,"WindowHeight":768,"ViewHeight":768,"SoundOn":false,"PlayMusic":false,"WindNoise":false,"TellTips":false}'
        $taskArguments = "--language $($taskScenario.Language) --no-tips --no-sound --no-music --ui-script-file $taskRelative/commands.txt"
        $taskCommands = if ($taskScenario.Mission) { 'click-center 82,114; assert battle; ' } else { 'assert main; ' }
        if ($taskScenario.Mission) { $taskArguments += ' --mission thewarbegins' }
        $taskMissionValue = if ($taskScenario.Mission) { 'true' } else { 'false' }
        $taskReturnState = if ($taskScenario.Mission) { 'battle' } else { 'main' }
        $taskCommands += "key F1; assert help:F1Help; assert-detail help-line=15; assert-detail help-mission=$taskMissionValue; assert-detail help-body-top=58; assert-detail help-scroll=0; capture $taskRelative/01-contents.png; "
        # 실제 좌표 클릭과 Back으로 모든 내부 목적지와 방문 전 스크롤 복원을 확인한다.
        for ($taskIndex = 0; $taskIndex -lt $taskLinks.Count; $taskIndex++) {
            $taskY = $taskScenario.FirstLink + $taskIndex * 15
            $taskCommands += "click 340,$taskY; assert help:$($taskLinks[$taskIndex]); assert-detail help-scroll=0; capture $taskRelative/topic-$taskIndex.png; click 470,371; assert help:F1Help; assert-detail help-scroll=0; "
        }
        $taskPriestY = $taskScenario.FirstLink + 60
        $taskCommands += @"
click 340,$taskPriestY; assert help:priestType; assert-detail help-info=priest; assert-detail help-body-top=192;
remember-detail help-max; assert-unchanged-detail help-max; capture $taskRelative/02-priest-top.png;
click 715,341; assert-detail help-scroll=15; click 715,197; assert-detail help-scroll=0;
move 450,260; wheel -1; assert-detail help-scroll=45; move 460,261; assert-detail help-scroll=45;
drag 450,280,450,220,20; assert-detail help-scroll=105; wheel 1; assert-detail help-scroll=60;
drag 715,320,715,500,20; remember-detail help-scroll; assert-unchanged-detail help-scroll; capture $taskRelative/03-priest-bottom.png;
drag 450,230,450,2000,20; assert-detail help-scroll=0; capture $taskRelative/04-priest-restored.png;
click 470,371; assert help:F1Help; assert-detail help-scroll=0;
move 450,260; wheel -1; remember-detail help-scroll; assert-unchanged-detail help-scroll; capture $taskRelative/05-contents-scrolled.png;
key Escape; assert help:F1Help; click 550,370; assert $taskReturnState; quit;
"@
        Set-Content -LiteralPath (Join-Path $taskOutput 'commands.txt') -Encoding UTF8 -Value $taskCommands
        $env:NETSTORM_SETTINGS_DIR = $taskSettings
        $taskProcess = Start-Process -FilePath $taskExe -WorkingDirectory $taskRoot -ArgumentList $taskArguments -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $taskOutput 'stdout.log') -RedirectStandardError (Join-Path $taskOutput 'stderr.log')
        $null = $taskProcess.Handle
        if (!$taskProcess.WaitForExit(45000)) { $taskProcess.Kill(); throw 'Clone help check timeout' }
        if ($taskProcess.ExitCode -ne 0) { throw (Get-Content -Raw (Join-Path $taskOutput 'stderr.log')) }
        $taskLog = Get-Content -Raw (Join-Path $taskOutput 'stdout.log')
        $taskMaximum = [regex]::Match($taskLog, 'unchanged help-max=(\d+)').Groups[1].Value
        $taskBottom = [regex]::Match($taskLog, 'unchanged help-scroll=(\d+)').Groups[1].Value
        if (!$taskMaximum -or $taskMaximum -ne $taskBottom) { throw "Scrollbar did not reach bottom: $taskBottom / $taskMaximum" }
        $taskTopImage = Join-Path $taskOutput '02-priest-top.png'
        $taskHeaderDiff = Get-CloneHelpPixelDifference $taskTopImage (Join-Path $taskOutput '03-priest-bottom.png') 58 182
        $taskRestoreDiff = Get-CloneHelpPixelDifference $taskTopImage (Join-Path $taskOutput '04-priest-restored.png') 192 346
        if ($taskHeaderDiff -ne 0 -or $taskRestoreDiff -ne 0) { throw "Help pixels changed: header=$taskHeaderDiff, restored=$taskRestoreDiff" }
        Write-Output "$($taskScenario.Language)-$taskMode : $([regex]::Matches($taskLog,'UI PASS:').Count) checks passed; scroll bottom=$taskBottom; header/restored pixels match"
    }
} finally { $env:NETSTORM_SETTINGS_DIR = $taskOldSettings }
