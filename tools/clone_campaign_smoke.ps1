# 클론의 스크립트 기반 캠페인 목록·선행 조건을 창 모드에서 검사하며 원본 게임은 실행하지 않는다.
param([string]$OutputDirectory = 'extracted\screens\campaign-smoke')
$ErrorActionPreference = 'Stop'
$taskRoot = Split-Path -Parent $PSScriptRoot
$taskOutput = Join-Path $taskRoot $OutputDirectory
New-Item -ItemType Directory -Force -Path $taskOutput | Out-Null
$taskExe = Join-Path $taskRoot 'dotnetpj/src/Netstorm.Game/bin/Release/net10.0/NetstormClone.exe'
$taskOldSettings = $env:NETSTORM_SETTINGS_DIR
$taskCases = [ordered]@{
    locked = '{"WindowWidth":1024,"WindowHeight":768,"ViewHeight":768,"SoundOn":false,"PlayMusic":false}'
    unlocked = '{"WindowWidth":1024,"WindowHeight":768,"ViewHeight":768,"SoundOn":false,"PlayMusic":false,"ScriptValues":{"DoneTheWarBegins":"1"}}'
}
try {
    # 잠금 상태와 완료값이 있는 상태를 별도 설정 폴더로 실행해 사용자 설정에 영향을 주지 않는다.
    foreach ($taskCase in $taskCases.Keys) {
        $taskSettings = Join-Path $taskOutput "$taskCase-settings"
        New-Item -ItemType Directory -Force -Path $taskSettings | Out-Null
        Set-Content -LiteralPath (Join-Path $taskSettings 'settings.json') -Encoding UTF8 -Value $taskCases[$taskCase]
        $taskCommands = @"
assert main; click-center -120,-64; assert campaigns; capture $OutputDirectory/$taskCase-campaigns.png;
click-center 0,28; assert missions; capture $OutputDirectory/$taskCase-missions.png;
click-center 0,69; assert $(@{ locked = 'missions'; unlocked = 'briefing' }[$taskCase]); quit;
"@
        $taskCommandFile = Join-Path $taskOutput "$taskCase.txt"
        Set-Content -LiteralPath $taskCommandFile -Encoding UTF8 -Value $taskCommands
        $taskLog = Join-Path $taskOutput "$taskCase.stdout.log"
        $env:NETSTORM_SETTINGS_DIR = $taskSettings
        $taskProcess = Start-Process -FilePath $taskExe -WorkingDirectory $taskRoot `
            -ArgumentList "--language english --no-tips --ui-script-file $OutputDirectory/$taskCase.txt" `
            -WindowStyle Hidden -PassThru -Wait -RedirectStandardOutput $taskLog `
            -RedirectStandardError (Join-Path $taskOutput "$taskCase.stderr.log")
        if ($taskProcess.ExitCode -ne 0) { throw "$taskCase 캠페인 메뉴 검사 실패: " + (Get-Content -Raw (Join-Path $taskOutput "$taskCase.stderr.log")) }
        Get-Content $taskLog
    }
} finally {
    $env:NETSTORM_SETTINGS_DIR = $taskOldSettings
}
