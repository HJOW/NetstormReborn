<#
.SYNOPSIS
    원본 로컬/Root 서버 R.exe를 게임 클라이언트와 분리된 Ghidra 프로젝트로 정밀 디컴파일한다.
.DESCRIPTION
    실행 파일은 읽기만 하며 서버를 실행하지 않는다. 클라이언트 양판본 프로젝트와 결과는 변경하지 않는다.
    이미 있는 서버 프로젝트는 재사용하므로 명시적으로 -Reimport를 지정해야 다시 임포트한다.
#>
param(
    [string]$GhidraDir = '', # 생략하면 C:\Tools의 최신 Ghidra 설치 폴더를 사용한다.
    [switch]$Reimport # 기존 서버 분석을 폐기하고 같은 전용 프로젝트에 원본을 다시 임포트한다.
)

$ErrorActionPreference = 'Stop'
$Root = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path # 게임 저장소의 절대 기준 경로다.
$Binary = Join-Path $Root 'originals\R.exe' # 수정하거나 실행하지 않는 서버 원본이다.
$OutDir = Join-Path $Root 'extracted\refined\root-server' # 클라이언트 결과와 구분하는 서버 전용 산출물이다.
$ProjectDir = Join-Path $OutDir 'ghidra' # R 전용 프로젝트의 저장 폴더다.
$ProjectName = 'NetstormRootServer' # Netstorm/NETSTORM 클라이언트 프로젝트와 다른 이름이다.
New-Item -ItemType Directory -Force -Path $ProjectDir | Out-Null
if (-not $GhidraDir) {
    $Found = Get-ChildItem 'C:\Tools' -Directory -Filter 'ghidra_*' | Sort-Object Name -Descending | Select-Object -First 1 # 기존 설치 중 최신 이름이다.
    if (-not $Found) { throw 'Ghidra 설치를 찾지 못했습니다. -GhidraDir를 지정하세요.' }
    $GhidraDir = $Found.FullName
}
$Headless = Join-Path $GhidraDir 'support\analyzeHeadless.bat' # 별도 창을 열지 않는 분석 진입점이다.
$ProjectArgs = @($ProjectDir, $ProjectName) # 전용 프로젝트의 위치와 이름이다.
if ($Reimport -or -not (Test-Path -LiteralPath (Join-Path $ProjectDir "$ProjectName.gpr"))) {
    $ProjectArgs += @('-import', $Binary, '-overwrite')
} else {
    $ProjectArgs += @('-process', 'R.exe', '-noanalysis')
}
$PostArgs = @(
    '-scriptPath', $PSScriptRoot,
    '-postScript', 'FixSwitches.java', (Join-Path $OutDir 'switches.tsv'),
    '-postScript', 'RecoverMissing.java', '-', (Join-Path $OutDir 'missing.tsv'), (Join-Path $OutDir 'gaps.tsv'), 'commit',
    '-postScript', 'FixSwitches.java', (Join-Path $OutDir 'switches.tsv'),
    '-postScript', 'ApplyConventions.java', (Join-Path $OutDir 'conventions.tsv'), '-', 'follow-thunks', 'clear-analysis-parameters',
    '-postScript', 'ExportDecomp.java', (Join-Path $OutDir 'raw.c'),
    '-postScript', 'DumpFunctions.java', (Join-Path $OutDir 'functions.tsv')
) # 클라이언트 정밀 분석과 같은 근거 기반 복구 순서이며 판본 간 힌트는 적용하지 않는다.
$Switches = Join-Path $OutDir 'switches.tsv' # 두 FixSwitches가 이어 쓰는 이번 실행의 전용 결과다.
if (Test-Path -LiteralPath $Switches) { Remove-Item -LiteralPath $Switches } # 재사용 실행에서 옛 switch 행을 중복 누적하지 않는다.
$PreviousConfig = $env:XDG_CONFIG_HOME # 호출자의 설정 위치를 복구하기 위한 원래 값이다.
$PreviousCache = $env:XDG_CACHE_HOME # 호출자의 캐시 위치를 복구하기 위한 원래 값이다.
$env:XDG_CONFIG_HOME = Join-Path $Root 'extracted\ghidra-settings'
$env:XDG_CACHE_HOME = Join-Path $Root 'extracted\ghidra-cache'
try {
    & $Headless @ProjectArgs @PostArgs
    if ($LASTEXITCODE -ne 0) { throw "R.exe 정밀 디컴파일 실패: $LASTEXITCODE" }
} finally {
    $env:XDG_CONFIG_HOME = $PreviousConfig
    $env:XDG_CACHE_HOME = $PreviousCache
}
foreach ($FileName in @('raw.c', 'functions.tsv', 'conventions.tsv')) { # 필수 결과가 실제 생성됐는지 확인한다.
    $Result = Get-Item -LiteralPath (Join-Path $OutDir $FileName) # 이번 분석의 필수 산출물이다.
    if ($Result.Length -eq 0) { throw "빈 분석 결과: $($Result.FullName)" }
}
Write-Host "서버 정밀 디컴파일 결과: $OutDir"
