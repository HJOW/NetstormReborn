<#
.SYNOPSIS
    다리 기계어 검증의 원형·필드를 두 정밀 프로젝트에 읽기 전용으로 적용한다.
    원본 게임/파일과 Ghidra 프로젝트를 수정하지 않고 별도 C 출력만 만든다.
#>
param()
$ErrorActionPreference = 'Stop'
# 분석 결과를 저장할 저장소 루트다.
$BridgeRecoveryRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
# 두 판본을 차례로 읽고 완료 표시까지 검사한다.
foreach ($Edition in @('originals','originalCD')) {
    $Output = Join-Path $BridgeRecoveryRoot "extracted\refined\$Edition\typed-bridge.c"
    # 이전 성공 파일이 남았더라도 새 스크립트 실패를 성공으로 처리하지 않는다.
    $StartedAt = [DateTime]::UtcNow
    & (Join-Path $PSScriptRoot 'run_script.ps1') -Edition $Edition -Refined `
        -Script ApplyBridgeTypes.java -ScriptArgs @($Output)
    if ($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $Output)) {
        throw "다리 자료형 디컴파일 실패: $Edition"
    }
    if ((Get-Item -LiteralPath $Output).LastWriteTimeUtc -lt $StartedAt) {
        throw "이번 실행에서 다리 디컴파일 출력이 갱신되지 않음: $Edition"
    }
    $Expected = if ($Edition -eq 'originals') {9} else {8}
    if ((Get-Content -LiteralPath $Output -Raw -Encoding UTF8) -notmatch "(?m)^// bridge-types-complete:$Expected\s*$") {
        throw "다리 자료형 디컴파일 미완료: $Edition"
    }
}
exit 0
