<#
.SYNOPSIS
    이미 만든 Ghidra 프로젝트에 대해 tools\ghidra 의 스크립트 하나를 헤드리스로 실행한다.
    기본은 읽기 전용(-readOnly)이라 프로젝트를 바꾸지 않는다. -Refined 를 주면 정밀 분석 프로젝트(extracted\refined)를 연다.

.EXAMPLE
    powershell -ExecutionPolicy Bypass -File tools\ghidra\run_script.ps1 -Script DumpFunctions.java -ScriptArgs extracted\refined\originals\functions-base.tsv
    powershell -ExecutionPolicy Bypass -File tools\ghidra\run_script.ps1 -Edition originalCD -Refined -Script DumpFunctions.java -ScriptArgs out.tsv
#>
param(
    # 실행할 스크립트 파일 이름 (tools\ghidra 안)
    [Parameter(Mandatory = $true)][string]$Script,
    # 스크립트에 넘길 인자 목록
    [string[]]$ScriptArgs = @(),
    # 분석할 판본 (run_decomp.ps1 의 -Edition 과 같다)
    [ValidateSet('originals', 'originalCD', 'original1037', 'original1062', 'original1082', 'original1082-launcher', 'original1082v12', 'original1082v12-launcher', 'patch1062')]
    [string]$Edition = 'originals',
    # 정밀 분석 프로젝트(extracted\refined\<판본>\ghidra)를 열지 여부
    [switch]$Refined,
    # 프로젝트에 변경을 저장할지 여부 (생략하면 읽기 전용)
    [switch]$Save,
    # Ghidra 설치 폴더 (생략하면 C:\Tools 의 최신 버전)
    [string]$GhidraDir = ''
)

# 프로젝트 루트 (이 스크립트 기준 두 단계 위)
$Root = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path

# 판본별 프로젝트 이름과 프로그램 이름
if ($Edition -eq 'original1082v12') {
    $ProjectName = 'netstorm'
    $Program = 'netstorm.game'
    $BaseProjectDir = Join-Path $Root 'extracted\original1082v12\ghidra'
} elseif ($Edition -eq 'original1082v12-launcher') {
    $ProjectName = 'Netstorm'
    $Program = 'Netstorm.exe'
    $BaseProjectDir = Join-Path $Root 'extracted\original1082v12\launcher\ghidra'
} elseif ($Edition -eq 'original1082') {
    $ProjectName = 'netstorm'
    $Program = 'netstorm.game'
    $BaseProjectDir = Join-Path $Root 'extracted\original1082\ghidra'
} elseif ($Edition -eq 'original1082-launcher') {
    $ProjectName = 'Netstorm'
    $Program = 'Netstorm.exe'
    $BaseProjectDir = Join-Path $Root 'extracted\original1082\launcher\ghidra'
} elseif ($Edition -eq 'patch1062') {
    $ProjectName = 'zpatch'
    $Program = 'zpatch.exe'
    $BaseProjectDir = Join-Path $Root 'extracted\patch1062\ghidra'
} elseif ($Edition -eq 'original1062') {
    $ProjectName = 'Netstorm'
    $Program = 'Netstorm.exe'
    $BaseProjectDir = Join-Path $Root 'extracted\original1062\ghidra'
} elseif ($Edition -eq 'original1037') {
    $ProjectName = 'netstorm'
    $Program = 'netstorm.exe'
    $BaseProjectDir = Join-Path $Root 'extracted\original1037\ghidra'
} elseif ($Edition -eq 'originalCD') {
    $ProjectName = 'NETSTORM'
    $Program = 'NETSTORM.EXE'
    $BaseProjectDir = Join-Path $Root 'extracted\originalCD\ghidra'
} else {
    $ProjectName = 'Netstorm'
    $Program = 'Netstorm.exe'
    $BaseProjectDir = Join-Path $Root 'extracted\ghidra'
}
# 정밀 분석 프로젝트는 기존 프로젝트와 폴더를 분리한다
if ($Refined) {
    $ProjectDir = Join-Path $Root "extracted\refined\$Edition\ghidra"
} else {
    $ProjectDir = $BaseProjectDir
}
if (-not (Test-Path (Join-Path $ProjectDir "$ProjectName.gpr"))) {
    throw "Ghidra 프로젝트가 없습니다: $ProjectDir"
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

# 자동 분석 없이 열어 스크립트만 실행한다 (-Save 가 없으면 읽기 전용)
$Mode = @('-noanalysis')
if (-not $Save) { $Mode += '-readOnly' }
try {
    # Windows PowerShell 5는 Java의 stderr 경고도 NativeCommandError로 만든다.
    # 이 호출 동안만 경고 출력을 허용하고 실제 실패 여부는 프로세스 종료 코드로 판단한다.
    $PreviousNativeErrorAction = $ErrorActionPreference
    try {
        $ErrorActionPreference = 'Continue'
        & $Headless $ProjectDir $ProjectName -process $Program @Mode `
            -scriptPath (Join-Path $Root 'tools\ghidra') -postScript $Script @ScriptArgs
        $ExitCode = $LASTEXITCODE
    } finally {
        $ErrorActionPreference = $PreviousNativeErrorAction
    }
} finally {
    $env:XDG_CONFIG_HOME = $PreviousConfigHome
    $env:XDG_CACHE_HOME = $PreviousCacheHome
}
if ($ExitCode -ne 0) { throw "Ghidra 실행 실패 (종료 코드 $ExitCode)" }
