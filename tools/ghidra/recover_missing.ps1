<#
.SYNOPSIS
    이미 만든 Ghidra 프로젝트에서 자동 분석이 함수로 인식하지 못한 코드를 모두 찾아 함수로 만들고 디컴파일한다.
    프로젝트는 읽기 전용으로 열어 변경하지 않는다. 결과: extracted\decomp-at\<판본>-missing.c, 후보 목록 <판본>-missing.tsv, 함수 밖 코드 구간 <판본>-gaps.tsv

.EXAMPLE
    powershell -ExecutionPolicy Bypass -File tools\ghidra\recover_missing.ps1
    powershell -ExecutionPolicy Bypass -File tools\ghidra\recover_missing.ps1 -Edition originalCD
#>
param(
    # 분석할 판본 (run_decomp.ps1 의 -Edition 과 같다)
    [ValidateSet('originals', 'originalCD')]
    [string]$Edition = 'originals',
    # Ghidra 설치 폴더 (생략하면 C:\Tools 의 최신 버전)
    [string]$GhidraDir = ''
)

# 프로젝트 루트 (이 스크립트 기준 두 단계 위)
$Root = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path

# 판본별 프로젝트 폴더와 프로젝트 이름
if ($Edition -eq 'originalCD') {
    $ProjectDir = Join-Path $Root 'extracted\originalCD\ghidra'
    $ProjectName = 'NETSTORM'
    $Program = 'NETSTORM.EXE'
} else {
    $ProjectDir = Join-Path $Root 'extracted\ghidra'
    $ProjectName = 'Netstorm'
    $Program = 'Netstorm.exe'
}
if (-not (Test-Path (Join-Path $ProjectDir "$ProjectName.gpr"))) {
    throw "Ghidra 프로젝트가 없습니다. 먼저 run_decomp.ps1 을 실행하세요: $ProjectDir"
}

# Ghidra 폴더를 지정하지 않았으면 C:\Tools 에서 가장 최신 버전을 찾는다
if (-not $GhidraDir) {
    $found = Get-ChildItem 'C:\Tools' -Directory -Filter 'ghidra_*' -ErrorAction SilentlyContinue |
             Sort-Object Name -Descending | Select-Object -First 1
    if (-not $found) { throw 'Ghidra 를 찾지 못했습니다. PREPARE.ps1 로 설치하거나 -GhidraDir 를 지정하세요.' }
    $GhidraDir = $found.FullName
}
$Headless = Join-Path $GhidraDir 'support\analyzeHeadless.bat'

# 결과 파일과 후보 목록 파일 위치
$OutDir = Join-Path $Root 'extracted\decomp-at'
if (-not (Test-Path $OutDir)) { New-Item -ItemType Directory -Path $OutDir | Out-Null }
$OutFile = Join-Path $OutDir "$Edition-missing.c"
$TsvFile = Join-Path $OutDir "$Edition-missing.tsv"
$GapFile = Join-Path $OutDir "$Edition-gaps.tsv"

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

# 자동 분석 없이 읽기 전용으로 열어 누락 함수를 복구한다
try {
    & $Headless $ProjectDir $ProjectName -process $Program -noanalysis -readOnly `
        -scriptPath (Join-Path $Root 'tools\ghidra') -postScript RecoverMissing.java $OutFile $TsvFile $GapFile
    $ExitCode = $LASTEXITCODE
} finally {
    $env:XDG_CONFIG_HOME = $PreviousConfigHome
    $env:XDG_CACHE_HOME = $PreviousCacheHome
}
if ($ExitCode -ne 0) { throw "Ghidra 실행 실패 (종료 코드 $ExitCode)" }
Write-Host "결과: $OutFile"
Write-Host "후보 목록: $TsvFile"
Write-Host "함수 밖 코드 구간: $GapFile"
