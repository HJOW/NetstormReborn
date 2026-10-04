<#
.SYNOPSIS
    신뢰도를 높인 "정밀 디컴파일"을 만든다. 기본 전체 디컴파일(run_decomp.ps1)과 별개의 프로젝트·결과 폴더를 쓴다.
    1단계(기본): 임포트·자동 분석 → switch 몸체 보정 → 누락 함수 복구(확정) → switch 재보정
                 → 근거 기반 호출 규약 지정 → 전체 디컴파일 내보내기 → 함수별 사실 덤프.
    2단계(-ApplyHints): 이미 만든 정밀 프로젝트에 상대 판본의 근거로 만든 힌트(hints.tsv)를 적용하고 다시 내보낸다.
    결과: extracted\refined\<판본>\ 아래 raw.c, functions.tsv, conventions.tsv, missing.tsv, gaps.tsv, switches.tsv 와
          Ghidra 프로젝트. 판본 간 함수 대응·힌트·신뢰도 등급은 tools\decomp_refine.py 가 만든다.
    두 판본을 한 번에 처리하려면 tools\ghidra\refine_all.ps1 을 쓴다.

.EXAMPLE
    powershell -ExecutionPolicy Bypass -File tools\ghidra\refine_decomp.ps1
    powershell -ExecutionPolicy Bypass -File tools\ghidra\refine_decomp.ps1 -Edition originalCD
    powershell -ExecutionPolicy Bypass -File tools\ghidra\refine_decomp.ps1 -ApplyHints
#>
param(
    # 분석할 판본 (run_decomp.ps1 의 -Edition 과 같다)
    [ValidateSet('originals', 'originalCD')]
    [string]$Edition = 'originals',
    # 2단계: 기존 정밀 프로젝트에 hints.tsv 의 호출 규약 힌트를 적용하고 다시 내보낸다 (임포트·분석·복구는 하지 않음)
    [switch]$ApplyHints,
    # Ghidra 설치 폴더 (생략하면 C:\Tools 의 최신 버전)
    [string]$GhidraDir = ''
)

# 프로젝트 루트 (이 스크립트 기준 두 단계 위)
$Root = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path

# 판본별 원본 실행 파일과 프로젝트 이름
if ($Edition -eq 'originalCD') {
    $Binary = Join-Path $Root 'originalCD\NETSTORM.EXE'
    $ProjectName = 'NETSTORM'
} else {
    $Binary = Join-Path $Root 'originals\Netstorm.exe'
    $ProjectName = 'Netstorm'
}

# 결과 폴더와 프로젝트 폴더 (Git 제외 경로)
$OutDir = Join-Path $Root "extracted\refined\$Edition"
$ProjectDir = Join-Path $OutDir 'ghidra'
foreach ($dir in @($OutDir, $ProjectDir)) {
    if (-not (Test-Path $dir)) { New-Item -ItemType Directory -Path $dir | Out-Null }
}

# 단계별 결과 파일
$RawC = Join-Path $OutDir 'raw.c'
$FunctionsTsv = Join-Path $OutDir 'functions.tsv'
$ConventionsTsv = Join-Path $OutDir 'conventions.tsv'
$HintsTsv = Join-Path $OutDir 'hints.tsv'
$MissingTsv = Join-Path $OutDir 'missing.tsv'
$GapsTsv = Join-Path $OutDir 'gaps.tsv'
$SwitchesTsv = Join-Path $OutDir 'switches.tsv'

# 이번 실행이 새로 만드는 파일은 먼저 지운다 (switches.tsv 는 이어 쓰기라 남아 있으면 중복된다)
if ($ApplyHints) {
    if (-not (Test-Path -LiteralPath $HintsTsv)) {
        throw "힌트 파일이 없습니다. 먼저 python tools/decomp_refine.py --hints 를 실행하세요: $HintsTsv"
    }
    $Stale = @($RawC, $FunctionsTsv, $ConventionsTsv)
} else {
    $Stale = @($RawC, $FunctionsTsv, $ConventionsTsv, $HintsTsv, $MissingTsv, $GapsTsv, $SwitchesTsv)
}
foreach ($file in $Stale) {
    if (Test-Path -LiteralPath $file) { Remove-Item -LiteralPath $file -Force }
}

# Ghidra 폴더를 지정하지 않았으면 C:\Tools 에서 가장 최신 버전을 찾는다
if (-not $GhidraDir) {
    $found = Get-ChildItem 'C:\Tools' -Directory -Filter 'ghidra_*' -ErrorAction SilentlyContinue |
             Sort-Object Name -Descending | Select-Object -First 1
    if (-not $found) { throw 'Ghidra 를 찾지 못했습니다. PREPARE.ps1 로 설치하거나 -GhidraDir 를 지정하세요.' }
    $GhidraDir = $found.FullName
}
$Headless = Join-Path $GhidraDir 'support\analyzeHeadless.bat'

# Ghidra 설정과 캐시는 Git 제외 경로에 둔다 (run_decomp.ps1 과 같음)
$SettingsDir = Join-Path $Root 'extracted\ghidra-settings'
$CacheDir = Join-Path $Root 'extracted\ghidra-cache'
foreach ($dir in @($SettingsDir, $CacheDir)) {
    if (-not (Test-Path $dir)) { New-Item -ItemType Directory -Path $dir | Out-Null }
}
$PreviousConfigHome = $env:XDG_CONFIG_HOME
$PreviousCacheHome = $env:XDG_CACHE_HOME
$env:XDG_CONFIG_HOME = $SettingsDir
$env:XDG_CACHE_HOME = $CacheDir

# 후처리 스크립트 순서 (각 항목은 -postScript 뒤에 붙는 인자 목록)
if ($ApplyHints) {
    $Steps = @(
        , @('ApplyConventions.java', $ConventionsTsv, $HintsTsv)
    )
} else {
    $Steps = @(
        @('FixSwitches.java', $SwitchesTsv),
        @('RecoverMissing.java', '-', $MissingTsv, $GapsTsv, 'commit'),
        @('FixSwitches.java', $SwitchesTsv),
        @('ApplyConventions.java', $ConventionsTsv)
    )
}
$Steps += , @('ExportDecomp.java', $RawC)
$Steps += , @('DumpFunctions.java', $FunctionsTsv)

# 단계 목록을 analyzeHeadless 인자로 펼친다
$PostArgs = @()
foreach ($step in $Steps) {
    $PostArgs += '-postScript'
    $PostArgs += $step
}

# 1단계: 임포트 + 자동 분석 + 후처리 (기존 정밀 프로젝트가 있으면 덮어씀)
# 2단계: 기존 정밀 프로젝트를 자동 분석 없이 열어 후처리만 하고 저장
try {
    if ($ApplyHints) {
        & $Headless $ProjectDir $ProjectName -process ([IO.Path]::GetFileName($Binary)) -noanalysis `
            -scriptPath (Join-Path $Root 'tools\ghidra') @PostArgs
    } else {
        & $Headless $ProjectDir $ProjectName -import $Binary -overwrite `
            -scriptPath (Join-Path $Root 'tools\ghidra') @PostArgs
    }
    $ExitCode = $LASTEXITCODE
} finally {
    $env:XDG_CONFIG_HOME = $PreviousConfigHome
    $env:XDG_CACHE_HOME = $PreviousCacheHome
}
if ($ExitCode -ne 0) { throw "Ghidra 정밀 디컴파일 실패 (종료 코드 $ExitCode): $Binary" }
foreach ($file in @($RawC, $FunctionsTsv, $ConventionsTsv)) {
    if (-not (Test-Path -LiteralPath $file) -or (Get-Item -LiteralPath $file).Length -eq 0) {
        throw "결과 파일이 생성되지 않았습니다: $file"
    }
}
Write-Host "결과 폴더: $OutDir"
