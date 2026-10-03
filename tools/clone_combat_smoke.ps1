# 원본 실행·OS 입력 없이 조준·탄·폭발 그림과 일시정지를 클론에서 검사한다.
param([string]$OutputDirectory = 'extracted/fangame-combat-20261003/smoke')
$ErrorActionPreference = 'Stop'
# 검사 데이터·사용자 설정은 출력 폴더에만 두고 환경 변수를 종료 시 복구한다.
$taskRoot = Split-Path -Parent $PSScriptRoot
$taskExe = Join-Path $taskRoot 'src/Netstorm.Game/bin/Release/net10.0/NetstormClone.exe'
$taskOutput = Join-Path $taskRoot $OutputDirectory
$taskData = Join-Path $taskOutput 'data'
$taskOldData = $env:NETSTORM_DATA
$taskOldSettings = $env:NETSTORM_SETTINGS_DIR
python (Join-Path $PSScriptRoot 'clone_combat_fixture.py') --source (Join-Path $taskRoot 'assets/game-data') --output $taskData
if ($LASTEXITCODE -ne 0) { throw '전투 검사 맵 생성 실패' }
# 논리 틱으로 정확한 조준·탄 이동·파괴 장면을 만든다. 화면 검사 중에는 세션 시간이 멈춘다.
$taskCases = @(
    @{ Name = 'archers'; Seconds = '0.1666667'; Pose = '17'; Crossbow = '3' },
    @{ Name = 'crossbow-loading'; Seconds = '0.2916667'; Pose = '18'; Crossbow = '2' },
    @{ Name = 'turning'; Seconds = '0.5'; Pose = '19'; Crossbow = '1' },
    @{ Name = 'crossbow-release'; Seconds = '0.9166667'; Pose = '27'; Crossbow = '4' },
    @{ Name = 'projectiles'; Seconds = '1'; Pose = '3' },
    @{ Name = 'explosion'; Seconds = '2'; Pose = '18' }
)
try {
    $env:NETSTORM_DATA = $taskData
    # 조준·장전·발사·폭발 장면을 영어 4:3과 한국어 16:9에서 확인한다.
    foreach ($taskFormat in @(@{ Name = 'english'; Width = 1024; Height = 768 }, @{ Name = 'korean'; Width = 1280; Height = 720 })) {
        foreach ($taskCase in $taskCases) {
            $taskRelative = "$OutputDirectory/$($taskFormat.Name)-$($taskCase.Name)"
            $taskFolder = Join-Path $taskRoot $taskRelative
            $taskSettings = Join-Path $taskFolder 'settings'
            New-Item -ItemType Directory -Force -Path $taskSettings | Out-Null
            @{ WindowWidth = $taskFormat.Width; WindowHeight = $taskFormat.Height; ViewHeight = 768;
                PlayMusic = $false; WindNoise = $false } | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $taskSettings 'settings.json') -Encoding UTF8
            $taskSoundChecks = if ($taskCase.Name -eq 'archers') { 'assert-detail audio-last=fireCrossbow.wav; assert-detail audio-result=played;' } else { '' }
            $taskCrossbowChecks = if ($taskCase.ContainsKey('Crossbow')) { "assert-detail crossbowposes=$($taskCase.Crossbow);" } else { '' }
            $taskScript = "wait 30; assert battle; assert-detail sunposes=$($taskCase.Pose); $taskCrossbowChecks $taskSoundChecks capture $taskRelative/scene.png; wait 60; assert-detail sunposes=$($taskCase.Pose); $taskCrossbowChecks capture $taskRelative/paused.png; quit;"
            Set-Content -LiteralPath (Join-Path $taskFolder 'commands.txt') -Encoding UTF8 -Value $taskScript
            $env:NETSTORM_SETTINGS_DIR = $taskSettings
            $taskSoundFlag = if ($taskCase.Name -eq 'archers') { '' } else { '--no-sound' }
            $taskArguments = "--map fancombat --language $($taskFormat.Name) $taskSoundFlag --no-music --script `"combat 1; wait $($taskCase.Seconds); combat 0`" --ui-script-file $taskRelative/commands.txt"
            $taskProcess = Start-Process -FilePath $taskExe -WorkingDirectory $taskRoot -ArgumentList $taskArguments -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $taskFolder 'stdout.log') -RedirectStandardError (Join-Path $taskFolder 'stderr.log')
            $null = $taskProcess.Handle
            if (!$taskProcess.WaitForExit(45000)) { $taskProcess.Kill(); throw "전투 $taskRelative 제한 시간 초과" }
            if ($taskProcess.ExitCode -ne 0) { throw (Get-Content -LiteralPath (Join-Path $taskFolder 'stderr.log') -Raw) }
            $taskChecks = @(Select-String -LiteralPath (Join-Path $taskFolder 'stdout.log') -Pattern 'UI PASS').Count
            Write-Output "COMBAT PASS: $taskRelative ($taskChecks checks)"
        }
    }
} finally { $env:NETSTORM_DATA = $taskOldData; $env:NETSTORM_SETTINGS_DIR = $taskOldSettings }
