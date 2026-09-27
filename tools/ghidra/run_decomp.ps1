<#
.SYNOPSIS
    Ghidra 헤드리스 모드로 원본 실행 파일을 자동 분석하고 전체 함수를 디컴파일해 C 파일로 내보낸다.

.EXAMPLE
    powershell -ExecutionPolicy Bypass -File tools\ghidra\run_decomp.ps1
    powershell -ExecutionPolicy Bypass -File tools\ghidra\run_decomp.ps1 -Binary originals\NSENGLISHRES.DLL
#>
param(
    # 분석할 바이너리 경로
    [string]$Binary = 'originals\Netstorm.exe',
    # Ghidra 설치 폴더 (PREPARE.ps1 기본 설치 위치에서 검색)
    [string]$GhidraDir = ''
)

# 프로젝트 루트 (이 스크립트 기준 두 단계 위)
$Root = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path

# Ghidra 프로젝트와 디컴파일 결과를 둘 폴더 (extracted/ 는 git 에서 제외됨)
$ProjectDir = Join-Path $Root 'extracted\ghidra'
$OutDir     = Join-Path $Root 'extracted\decomp'

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

# 자동 분석 + 디컴파일 내보내기 (기존 프로젝트가 있으면 덮어씀)
& $Headless $ProjectDir $ProjectName -import $BinaryPath -overwrite `
    -scriptPath (Join-Path $Root 'tools\ghidra') -postScript ExportDecomp.java $OutFile

Write-Host "결과: $OutFile"
