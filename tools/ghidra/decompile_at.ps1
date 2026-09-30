<#
.SYNOPSIS
    이미 만든 Ghidra 프로젝트(extracted\ghidra)에서 지정한 주소의 함수만 디컴파일한다.
    전체 디컴파일(run_decomp.ps1)이 함수로 인식하지 못한 코드를 보고 싶을 때 쓴다. 프로젝트는 읽기 전용으로 열어 변경하지 않는다.

.EXAMPLE
    powershell -ExecutionPolicy Bypass -File tools\ghidra\decompile_at.ps1 -Addresses 484ab0,4c2b20
    powershell -ExecutionPolicy Bypass -File tools\ghidra\decompile_at.ps1 -Addresses 484ab0 -Edition originalCD
#>
param(
    # 디컴파일할 함수 시작 주소 (16진수, 0x 없이)
    [Parameter(Mandatory = $true)][string[]]$Addresses,
    # 분석할 판본 (run_decomp.ps1 의 -Edition 과 같다)
    [ValidateSet('originals', 'originalCD')]
    [string]$Edition = 'originals',
    # 결과 파일 (생략하면 extracted\decomp-at\<판본>.c)
    [string]$OutFile = '',
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

# 결과 파일 기본 위치
if (-not $OutFile) {
    $OutDir = Join-Path $Root 'extracted\decomp-at'
    if (-not (Test-Path $OutDir)) { New-Item -ItemType Directory -Path $OutDir | Out-Null }
    $OutFile = Join-Path $OutDir "$Edition.c"
}

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

# 자동 분석 없이 읽기 전용으로 열어 지정 주소만 디컴파일한다
try {
    & $Headless $ProjectDir $ProjectName -process $Program -noanalysis -readOnly `
        -scriptPath (Join-Path $Root 'tools\ghidra') -postScript DecompileAt.java $OutFile @Addresses
    $ExitCode = $LASTEXITCODE
} finally {
    $env:XDG_CONFIG_HOME = $PreviousConfigHome
    $env:XDG_CACHE_HOME = $PreviousCacheHome
}
if ($ExitCode -ne 0) { throw "Ghidra 실행 실패 (종료 코드 $ExitCode)" }
Write-Host "결과: $OutFile"
