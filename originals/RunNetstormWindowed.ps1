# 이 스크립트는 저장된 전체화면 시작값을 창 모드로 바꿔 원본 게임을 실행합니다.
$ErrorActionPreference = 'Stop'

# 스크립트 위치를 기준으로 원본 게임 파일 경로를 정합니다.
$gameDirectory = Split-Path -Parent $MyInvocation.MyCommand.Path
$configPath = Join-Path $gameDirectory 'd\options.cfg'
$gamePath = Join-Path $gameDirectory 'Netstorm.exe'
$backupPath = Join-Path $gameDirectory 'd\options.cfg.before-windowed-launch.bak'

# 설정 파일이나 실행 파일이 없으면 원본을 실행하지 않고 오류를 표시합니다.
if (-not (Test-Path -LiteralPath $configPath -PathType Leaf)) {
    throw "설정 파일을 찾을 수 없습니다: $configPath"
}
if (-not (Test-Path -LiteralPath $gamePath -PathType Leaf)) {
    throw "게임 실행 파일을 찾을 수 없습니다: $gamePath"
}

# 원본 설정의 XOR 키와 서명을 지정합니다.
$xorKey = [Text.Encoding]::ASCII.GetBytes('mydoghasfleas')
$signature = 'mQdsT'
$legacyEncoding = [Text.Encoding]::GetEncoding(28591)

# XOR 암호화된 설정 바이트를 읽어 평문으로 복호화합니다.
$encodedBytes = [IO.File]::ReadAllBytes($configPath)
$decodedBytes = New-Object byte[] $encodedBytes.Length
# 모든 바이트를 순회하며 원본 형식의 반복 XOR 복호화를 수행합니다.
for ($i = 0; $i -lt $encodedBytes.Length; $i++) {
    $decodedBytes[$i] = [byte]($encodedBytes[$i] -bxor $xorKey[$i % $xorKey.Length])
}

# 서명과 텍스트 인코딩이 예상 형식인지 확인합니다.
if ($decodedBytes.Length -lt $signature.Length -or $legacyEncoding.GetString($decodedBytes, 0, $signature.Length) -ne $signature) {
    throw 'options.cfg 형식이 예상과 다릅니다. 설정 파일은 변경하지 않았습니다.'
}
$configText = $legacyEncoding.GetString($decodedBytes, $signature.Length, $decodedBytes.Length - $signature.Length)

# 첫 startInFullScreen 값을 찾아 전체화면으로 시작하지 않도록 바꿉니다.
$startModePattern = '(?im)^([ \t]*startInFullScreen[ \t]*=[ \t]*)"([^"]*)"'
$startModeMatch = [regex]::Match($configText, $startModePattern)
if (-not $startModeMatch.Success) {
    throw 'options.cfg에서 startInFullScreen 항목을 찾지 못했습니다. 설정 파일은 변경하지 않았습니다.'
}

if ($startModeMatch.Groups[2].Value -ne '0') {
    # 최초 실행 전 설정 파일 원본을 한 번만 보관합니다.
    if (-not (Test-Path -LiteralPath $backupPath -PathType Leaf)) {
        Copy-Item -LiteralPath $configPath -Destination $backupPath
    }

    # 첫 일치 항목만 바꿔 나머지 게임 옵션과 줄바꿈을 보존합니다.
    $startModeRegex = [regex]::new($startModePattern)
    $updatedText = $startModeRegex.Replace($configText, '$1"0"', 1)

    # 수정한 평문에 서명을 붙이고 바이트 값을 그대로 보존하는 인코딩으로 변환합니다.
    $plainBytes = $legacyEncoding.GetBytes($signature + $updatedText)
    $newEncodedBytes = New-Object byte[] $plainBytes.Length
    # 모든 바이트를 순회하며 원본 형식의 반복 XOR 암호화를 수행합니다.
    for ($i = 0; $i -lt $plainBytes.Length; $i++) {
        $newEncodedBytes[$i] = [byte]($plainBytes[$i] -bxor $xorKey[$i % $xorKey.Length])
    }

    # 임시 파일을 완성한 뒤 설정 파일을 교체해 부분 기록을 방지합니다.
    $temporaryPath = $configPath + '.safe-launch.tmp'
    [IO.File]::WriteAllBytes($temporaryPath, $newEncodedBytes)
    try {
        [IO.File]::Replace($temporaryPath, $configPath, $null)
    }
    finally {
        # 교체 실패로 남은 임시 파일만 정리합니다.
        if (Test-Path -LiteralPath $temporaryPath -PathType Leaf) {
            Remove-Item -LiteralPath $temporaryPath
        }
    }
    Write-Host '전체화면 시작 설정을 창 모드로 변경했습니다.'
}
else {
    Write-Host '이미 창 모드 시작으로 설정되어 있습니다.'
}

# 원본 실행 파일을 설치 폴더를 작업 폴더로 지정해 동기 실행합니다.
$gameProcess = Start-Process -FilePath $gamePath -WorkingDirectory $gameDirectory -PassThru -Wait
exit $gameProcess.ExitCode
