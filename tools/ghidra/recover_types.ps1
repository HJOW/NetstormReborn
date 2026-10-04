<#
.SYNOPSIS
    두 판본에서 검토된 함수만 자료형을 적용해 별도 디컴파일을 만든다.
    정밀 프로젝트는 읽기 전용으로 열어 변경을 저장하지 않는다. 원본 게임은 실행하지 않는다.
#>
param()
$ErrorActionPreference = 'Stop'
# 분석 결과는 Git 제외 경로에 둔다.
$RecoveryRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
# 두 판본을 순서대로 열어 타입 복원 결과를 생성한다.
foreach ($Edition in @('originals', 'originalCD')) {
    $Output = Join-Path $RecoveryRoot "extracted\refined\$Edition\typed-core.c"
    & (Join-Path $PSScriptRoot 'run_script.ps1') -Edition $Edition -Refined `
        -Script ApplyRecoveredTypes.java -ScriptArgs @($Output)
    if ($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $Output)) {
        throw "자료형 적용 디컴파일 실패: $Edition"
    }
    # Ghidra가 스크립트 예외에도 종료 코드 0을 줄 수 있어 출력 완료 표시도 확인한다.
    if ((Get-Content -LiteralPath $Output -Raw -Encoding UTF8) -notmatch '(?m)^// recovered-complete:5\s*$') {
        throw "자료형 적용 디컴파일 미완료: $Edition"
    }
}
# Java의 stderr 경고와 작업 성공의 종료 코드를 구분한다.
exit 0
