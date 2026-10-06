<#
.SYNOPSIS
    Ghidra 헤드리스 모드로 원본 실행 파일을 자동 분석하고 전체 함수를 디컴파일해 C 파일로 내보낸다.

.EXAMPLE
    powershell -ExecutionPolicy Bypass -File tools\ghidra\run_decomp.ps1
    powershell -ExecutionPolicy Bypass -File tools\ghidra\run_decomp.ps1 -Binary originals\NSENGLISHRES.DLL
    powershell -ExecutionPolicy Bypass -File tools\ghidra\run_decomp.ps1 -Edition originalCD
#>
param(
    # 분석할 바이너리 경로 (생략하면 판본별 기본 실행 파일)
    [string]$Binary = '',
    # 패치판·CD판·10.37의 프로젝트와 출력 디렉터리를 분리한다.
    [ValidateSet('originals', 'originalCD', 'original1037')]
    [string]$Edition = 'originals',
    # Ghidra 설치 폴더 (PREPARE.ps1 기본 설치 위치에서 검색)
    [string]$GhidraDir = ''
)

# 프로젝트 루트 (이 스크립트 기준 두 단계 위)
$Root = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path

# CD판은 기존 패치판 프로젝트와 C 파일을 덮어쓰지 않도록 별도 폴더를 사용한다.
if ($Edition -eq 'original1037') {
    $ProjectDir = Join-Path $Root 'extracted\original1037\ghidra'
    $OutDir = Join-Path $Root 'extracted\original1037\decomp'
    if (-not $Binary) { $Binary = 'original1037\netstorm.exe' }
} elseif ($Edition -eq 'originalCD') {
    $ProjectDir = Join-Path $Root 'extracted\originalCD\ghidra'
    $OutDir = Join-Path $Root 'extracted\originalCD\decomp'
    if (-not $Binary) { $Binary = 'originalCD\NETSTORM.EXE' }
} else {
    $ProjectDir = Join-Path $Root 'extracted\ghidra'
    $OutDir = Join-Path $Root 'extracted\decomp'
    if (-not $Binary) { $Binary = 'originals\Netstorm.exe' }
}

# Ghidra 폴더를 지정하지 않았으면 C:\Tools 에서 가장 최신 버전을 찾는다
if (-not $GhidraDir) {
    $found = Get-ChildItem 'C:\Tools' -Directory -Filter 'ghidra_*' -ErrorAction SilentlyContinue |
             Sort-Object Name -Descending | Select-Object -First 1
    if (-not $found) { throw 'Ghidra 를 찾지 못했습니다. PREPARE.ps1 로 설치하거나 -GhidraDir 를 지정하세요.' }
    $GhidraDir = $found.FullName
}
$Headless = Join-Path $GhidraDir 'support\analyzeHeadless.bat'

$BinaryPath  = (Resolve-Path (Join-Path $Root $Binary)).Path
$ProjectName = [IO.Path]::GetFileNameWithoutExtension($BinaryPath)
$OutFile     = Join-Path $OutDir ($ProjectName + '.c')

# 출력 폴더가 없으면 생성 (Ghidra 프로젝트 폴더, 디컴파일 결과 폴더)
foreach ($dir in @($ProjectDir, $OutDir)) {
    if (-not (Test-Path $dir)) { New-Item -ItemType Directory -Path $dir | Out-Null }
}

# Ghidra 설정과 캐시도 Git 제외 경로에 두어 사용자 프로필의 쓰기 권한에 의존하지 않는다.
$SettingsDir = Join-Path $Root 'extracted\ghidra-settings'
$CacheDir = Join-Path $Root 'extracted\ghidra-cache'
foreach ($dir in @($SettingsDir, $CacheDir)) {
    if (-not (Test-Path $dir)) { New-Item -ItemType Directory -Path $dir | Out-Null }
}
$PreviousConfigHome = $env:XDG_CONFIG_HOME
$PreviousCacheHome = $env:XDG_CACHE_HOME
$env:XDG_CONFIG_HOME = $SettingsDir
$env:XDG_CACHE_HOME = $CacheDir

# 자동 분석 + 디컴파일 내보내기 (기존 프로젝트가 있으면 덮어씀)
try {
    # PowerShell 5에서 Java 경고를 실패로 취급하지 않고 실제 종료 코드를 확인한다.
    $PreviousNativeErrorAction = $ErrorActionPreference
    try {
        $ErrorActionPreference = 'Continue'
        & $Headless $ProjectDir $ProjectName -import $BinaryPath -overwrite `
            -scriptPath (Join-Path $Root 'tools\ghidra') -postScript ExportDecomp.java $OutFile `
            -postScript DumpFunctions.java (Join-Path $OutDir 'functions.tsv')
        $GhidraExitCode = $LASTEXITCODE
    } finally { $ErrorActionPreference = $PreviousNativeErrorAction }
} finally {
    $env:XDG_CONFIG_HOME = $PreviousConfigHome
    $env:XDG_CACHE_HOME = $PreviousCacheHome
}

if ($GhidraExitCode -ne 0) { throw "Ghidra 디컴파일 실패 (종료 코드 $GhidraExitCode): $BinaryPath" }
if (-not (Test-Path -LiteralPath $OutFile) -or (Get-Item -LiteralPath $OutFile).Length -eq 0) {
    throw "디컴파일 결과 파일이 생성되지 않았습니다: $OutFile"
}
Write-Host "결과: $OutFile"
