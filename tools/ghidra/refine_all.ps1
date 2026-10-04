<#
.SYNOPSIS
    두 판본(패치판 originals, CD판 originalCD)의 정밀 디컴파일을 처음부터 끝까지 만든다.
    순서: 판본별 1단계(refine_decomp.ps1) → 판본 간 대응으로 호출 규약 힌트 계산(decomp_refine.py --hints)
          → 판본별 2단계(refine_decomp.ps1 -ApplyHints) → 대응표·등급·최종 C 파일(decomp_refine.py).
    소요: 약 30~40분. 결과는 extracted\refined 아래에 생기며 Git 에 커밋되지 않는다. 원본 게임은 실행하지 않는다.

.EXAMPLE
    powershell -ExecutionPolicy Bypass -File tools\ghidra\refine_all.ps1
    powershell -ExecutionPolicy Bypass -File tools\ghidra\refine_all.ps1 -SkipStage1
#>
param(
    # 1단계(임포트·분석·복구)를 건너뛰고 기존 정밀 프로젝트에서 힌트 단계부터 다시 한다
    [switch]$SkipStage1,
    # Python 실행 파일 (생략하면 PATH 의 python)
    [string]$Python = 'python'
)

# 프로젝트 루트 (이 스크립트 기준 두 단계 위)
$Root = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
# 판본 목록 (패치판, CD판)
$Editions = @('originals', 'originalCD')
# 판본별 단계 스크립트와 대응·등급 도구
$Refine = Join-Path $PSScriptRoot 'refine_decomp.ps1'
$Tool = Join-Path $Root 'tools\decomp_refine.py'

# 외부 명령이 실패하면 멈춘다
function Assert-Success([string]$What) {
    if ($LASTEXITCODE -ne 0) { throw "$What 실패 (종료 코드 $LASTEXITCODE)" }
}

# 1단계: 판본마다 임포트·분석·복구·호출 규약 지정·내보내기
if (-not $SkipStage1) {
    foreach ($edition in $Editions) {
        Write-Host "== 1단계: $edition"
        & powershell -ExecutionPolicy Bypass -File $Refine -Edition $edition
        Assert-Success "1단계 $edition"
    }
}

# 판본 간 대응으로 호출 규약 힌트를 만든다
Write-Host '== 판본 간 대응과 호출 규약 힌트'
& $Python $Tool --hints
Assert-Success '힌트 계산'

# 2단계: 판본마다 힌트를 적용하고 다시 내보낸다
foreach ($edition in $Editions) {
    Write-Host "== 2단계: $edition"
    & powershell -ExecutionPolicy Bypass -File $Refine -Edition $edition -ApplyHints
    Assert-Success "2단계 $edition"
}

# 대응표·등급·함수 포인터 표·최종 C 파일
Write-Host '== 대응표·등급·최종 C 파일'
& $Python $Tool
Assert-Success '최종 결과 작성'
Write-Host "결과 폴더: $(Join-Path $Root 'extracted\refined')"
