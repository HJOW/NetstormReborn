# 원본 실행·OS 입력 없이 새 도움말과 옵션의 실제 화면·클릭·저장을 검사한다.
param(
    [string]$OutputDirectory = 'extracted/screens/ui-analysis-20261002',
    [ValidateSet('Menu', 'Mission', 'Workshop')][string]$Mode = 'Menu'
)
$ErrorActionPreference = 'Stop'

# 고정 1024×768 검사 화면의 오른쪽 위 100×18 영역에서 타이머 픽셀을 읽는다.
function Get-CloneTimerSignature([string]$Path) {
    $taskBitmap = [System.Drawing.Bitmap]::new($Path)
    try {
        if ($taskBitmap.Width -lt 1024 -or $taskBitmap.Height -lt 18) { throw '타이머 검사 화면 크기 부족' }
        $taskSignature = [System.Text.StringBuilder]::new()
        # 상단 18개 행만 비교하여 정지 중에도 변할 수 있는 전투 화면은 제외한다.
        for ($taskRow = 0; $taskRow -lt 18; $taskRow++) {
            # 오른쪽 100개 열의 ARGB 값을 같은 순서로 연결한다.
            for ($taskColumn = 924; $taskColumn -lt 1024; $taskColumn++) {
                $null = $taskSignature.Append($taskBitmap.GetPixel($taskColumn, $taskRow).ToArgb().ToString('X8'))
            }
        }
        return $taskSignature.ToString()
    } finally { $taskBitmap.Dispose() }
}

$taskRoot = Split-Path -Parent $PSScriptRoot
$taskOutput = Join-Path $taskRoot $OutputDirectory
New-Item -ItemType Directory -Force -Path $taskOutput | Out-Null
$taskSettings = Join-Path $taskOutput 'settings'
New-Item -ItemType Directory -Force -Path $taskSettings | Out-Null
Set-Content -LiteralPath (Join-Path $taskSettings 'settings.json') -Encoding UTF8 -Value '{"WindowWidth":1024,"WindowHeight":768,"ViewHeight":768,"SoundOn":true,"PlayMusic":false,"WindNoise":false}'
$taskScript = @"
assert tips; capture $OutputDirectory/00-tip.png; click 462,441; assert tips; click 662,441; assert main;
click 630,320; assert help; click 699,358; assert version; capture $OutputDirectory/00-version.png; click 512,447; assert main;
key F1; assert help:F1Help; capture $OutputDirectory/01-help-main.png;
key Escape; assert help:F1Help;
click 380,215; assert help:priestType; capture $OutputDirectory/02-priest-help.png;
drag 460,330,460,210,20; capture $OutputDirectory/03-priest-scrolled.png;
click 470,371; assert help:F1Help; click 550,370; assert main;
click 550,343; assert options; capture $OutputDirectory/04-options-on.png;
click 610,355; assert main; click 550,343; move 610,355; capture $OutputDirectory/05-options-off.png;
click 610,445; capture $OutputDirectory/06-volume-before.png;
click 750,483; assert main; click 550,343; click 610,445; capture $OutputDirectory/07-volume-after.png;
click 850,560; assert main;
click 550,343; click 610,409; assert main; click 550,343; capture $OutputDirectory/08-speaker-swap.png;
click 850,560; assert main;
quit;
"@
$taskArguments = "--language english --tips --ui-script-file $OutputDirectory/commands.txt"
if ($Mode -eq 'Mission') {
    $taskScript = @"
click-center 82,114; assert battle; key T; wait 90;
key F2; capture $OutputDirectory/10-hide-buildings.png; key F2;
key F7; capture $OutputDirectory/11-island-colors.png; key F7;
right-click 526,393; assert context:main; capture $OutputDirectory/12-priest-context.png;
click 600,493; assert context:construct; click 745,473; assert context:workshops;
capture $OutputDirectory/14-workshops.png; click 745,458; assert placement; right-click 500,400; assert battle;
key F1; assert help:F1Help; capture $OutputDirectory/15-battle-help.png; click 550,370; assert battle;
key Escape; assert mission-menu; click 210,8; assert options; capture $OutputDirectory/16-battle-options.png;
click 230,77; assert battle; key F6; assert knowledge; capture $OutputDirectory/17-knowledge.png;
click 500,210; capture $OutputDirectory/18-knowledge-help.png; click 470,371; assert knowledge; key F6; assert battle;
key LeftShift+F9; capture $OutputDirectory/19-paused.png; wait 120; capture $OutputDirectory/20-paused-still.png;
key LeftShift+F9; wait 120; capture $OutputDirectory/21-resumed.png; quit;
"@
    $taskArguments = "--mission thewarbegins --language korean --no-sound --no-music --ui-script-file $OutputDirectory/commands.txt"
} elseif ($Mode -eq 'Workshop') {
    # 사제가 걸어가 워크샵을 지은 뒤 그 옆에 서 있으므로(원본과 같다) 사제를 피해 워크샵 바닥 중앙(530,250)을 우클릭한다.
    # 메뉴·하위 창의 행 좌표는 우클릭 위치에서 상대 거리가 같다: 현재 생산 (+100,+100), 지식 등록 (+100,+117), 지식 목록 첫·둘째 (+218,+77)·(+218,+94).
    $taskScript = @"
click-center 82,114; assert battle; right-click 530,250; assert context:main; capture $OutputDirectory/22-workshop.png;
click 630,367; assert context:knowledge; capture $OutputDirectory/23-available-before.png;
click 748,327; assert battle; right-click 530,250; click 630,350; assert context:production;
capture $OutputDirectory/24-production-one.png; click 950,500; assert battle;
right-click 530,250; click 630,367; assert context:knowledge; capture $OutputDirectory/25-available-after.png;
click 748,344; assert battle; right-click 530,250; click 630,350; assert context:production;
capture $OutputDirectory/26-production-two.png; click 950,500; assert battle;
capture $OutputDirectory/27-production-sidebar.png; right-click 40,275; assert context:main;
capture $OutputDirectory/28-production-cost.png; quit;
"@
    $taskArguments = "--mission thewarbegins --language korean --no-sound --no-music --script `"construct sunFactory 98,98; wait 60`" --ui-script-file $OutputDirectory/commands.txt"
}
$taskFile = Join-Path $taskOutput 'commands.txt'
Set-Content -LiteralPath $taskFile -Encoding UTF8 -Value $taskScript
$taskOldSettings = $env:NETSTORM_SETTINGS_DIR
try {
    $env:NETSTORM_SETTINGS_DIR = $taskSettings
    $taskExe = Join-Path $taskRoot 'src/Netstorm.Game/bin/Release/net10.0/NetstormClone.exe'
    $taskProcess = Start-Process -FilePath $taskExe -WorkingDirectory $taskRoot -ArgumentList $taskArguments -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $taskOutput 'stdout.log') -RedirectStandardError (Join-Path $taskOutput 'stderr.log')
    # 종료 전에 프로세스 핸들을 확보해야 PowerShell 5.1에서도 종료 코드를 읽을 수 있다.
    $null = $taskProcess.Handle
    # 입력 스크립트 오류로 창이 계속 열려 있는 경우도 제한 시간 안에 검출한다.
    if (!$taskProcess.WaitForExit(45000)) { $taskProcess.Kill(); throw '클론 UI 검사 제한 시간 초과' }
    if ($taskProcess.ExitCode -ne 0) { throw (Get-Content -Raw (Join-Path $taskOutput 'stderr.log')) }
    Get-Content (Join-Path $taskOutput 'stdout.log')
    if ($Mode -eq 'Menu') {
        $taskSaved = Get-Content -Raw -Encoding UTF8 (Join-Path $taskSettings 'settings.json') | ConvertFrom-Json
        if ($taskSaved.SoundOn -or $taskSaved.MusicVolume -ne 3 -or !$taskSaved.SpeakerSwap) { throw '소리 상태·음량·스피커 교환 저장 실패' }
        # 시작 팁 0번에서 Next Tip → OK 를 눌렀으므로 다음 시작 팁은 2번이고 팁 표시는 켜진 채다.
        if ($taskSaved.TipNumber -ne 2 -or !$taskSaved.TellTips) { throw '시작 팁 번호·표시 설정 저장 실패' }
        Write-Output 'UI PASS: saved sound off / music 3 / speaker swap on / next tip 2'
    } elseif ($Mode -eq 'Mission') {
        Add-Type -AssemblyName System.Drawing
        $taskPaused = Get-CloneTimerSignature (Join-Path $taskOutput '19-paused.png')
        $taskStill = Get-CloneTimerSignature (Join-Path $taskOutput '20-paused-still.png')
        $taskResumed = Get-CloneTimerSignature (Join-Path $taskOutput '21-resumed.png')
        if ($taskPaused -ne $taskStill) { throw '일시정지 중 타이머 픽셀이 변경됨' }
        if ($taskStill -eq $taskResumed) { throw '재개 후 타이머 픽셀이 변경되지 않음' }
        Write-Output 'UI PASS: 일시정지 중 타이머 고정 / 재개 후 타이머 변화'
    }
} finally { $env:NETSTORM_SETTINGS_DIR = $taskOldSettings }
