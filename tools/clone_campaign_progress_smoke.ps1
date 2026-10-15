# 클론의 캠페인 진행 저장 흐름(1-1 승리 창 → 설정 저장 → 재실행 → 1-2 해제)을 창 모드에서 검사한다. 원본 게임은 실행하지 않는다.
# 승리는 실제 승리 알림과 같은 경로(mission-section Succeeded)로 열어 오래 걸리는 플레이를 건너뛴다.
param([string]$OutputDirectory = 'extracted\screens\campaign-progress-smoke')
$ErrorActionPreference = 'Stop'
$taskRoot = Split-Path -Parent $PSScriptRoot
$taskOutput = Join-Path $taskRoot $OutputDirectory
New-Item -ItemType Directory -Force -Path $taskOutput | Out-Null
$taskExe = Join-Path $taskRoot 'dotnetpj/src/Netstorm.Game/bin/Release/net10.0/NetstormClone.exe'
$taskOldSettings = $env:NETSTORM_SETTINGS_DIR
# 두 실행이 같은 설정 폴더를 공유해야 저장한 완료값이 재실행에 이어진다. 사용자 설정은 건드리지 않는다.
$taskSettings = Join-Path $taskOutput 'settings'
if (Test-Path -LiteralPath $taskSettings) { Remove-Item -LiteralPath $taskSettings -Recurse -Force }
New-Item -ItemType Directory -Force -Path $taskSettings | Out-Null
Set-Content -LiteralPath (Join-Path $taskSettings 'settings.json') -Encoding UTF8 -Value '{"WindowWidth":1024,"WindowHeight":768,"ViewHeight":768,"SoundOn":false,"PlayMusic":false}'
$taskRuns = [ordered]@{
    first = @"
assert main; click-center -120,-64; assert campaigns; click-center 0,28; assert missions;
click-center 0,69; assert missions; capture $OutputDirectory/first-locked.png;
click-center 0,51; wait 15; assert briefing; assert-detail mission=thewarbegins;
mission-section Succeeded; wait 12; assert briefing; capture $OutputDirectory/first-succeeded.png; quit;
"@
    second = @"
assert main; click-center -120,-64; assert campaigns; click-center 0,28; assert missions;
capture $OutputDirectory/second-unlocked.png;
click-center 0,69; wait 15; assert briefing; assert-detail mission=masterofwhirligigs; quit;
"@
}
try {
    $env:NETSTORM_SETTINGS_DIR = $taskSettings
    foreach ($taskRun in $taskRuns.Keys) {
        $taskCommandFile = Join-Path $taskOutput "$taskRun.txt"
        Set-Content -LiteralPath $taskCommandFile -Encoding UTF8 -Value $taskRuns[$taskRun]
        $taskLog = Join-Path $taskOutput "$taskRun.stdout.log"
        $taskProcess = Start-Process -FilePath $taskExe -WorkingDirectory $taskRoot `
            -ArgumentList "--language english --no-tips --ui-script-file $OutputDirectory/$taskRun.txt" `
            -WindowStyle Hidden -PassThru -Wait -RedirectStandardOutput $taskLog `
            -RedirectStandardError (Join-Path $taskOutput "$taskRun.stderr.log")
        if ($taskProcess.ExitCode -ne 0) { throw "$taskRun run failed: " + (Get-Content -Raw (Join-Path $taskOutput "$taskRun.stderr.log")) }
        Get-Content $taskLog
        if ($taskRun -eq 'first') {
            # 첫 실행이 끝난 직후 완료값이 설정 파일에 남았는지 확인한다.
            $taskSaved = Get-Content -Raw -LiteralPath (Join-Path $taskSettings 'settings.json') | ConvertFrom-Json
            $taskDone = @($taskSaved.ScriptValues.PSObject.Properties | Where-Object { $_.Name -ieq 'DoneTheWarBegins' -and $_.Value -eq '1' })
            if ($taskDone.Count -ne 1) { throw 'settings.json does not contain DoneTheWarBegins=1 after the 1-1 victory dialog' }
            Write-Output 'PROGRESS PASS: DoneTheWarBegins=1 saved in settings.json'
        }
    }
    Write-Output 'PROGRESS PASS: 1-2 unlocked after relaunch and briefing opened'
} finally {
    $env:NETSTORM_SETTINGS_DIR = $taskOldSettings
}
