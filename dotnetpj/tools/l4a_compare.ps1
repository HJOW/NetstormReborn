<#
.SYNOPSIS
  같은 ui-script 로 cpppj 오라클과 dotnetpj 새 경로 호스트를 각각 실행해 논리 캡처(snapshot)를 만들고 픽셀 단위로 비교한다 (L4a).

.DESCRIPTION
  사용법 (저장소 루트에서 실행):
    pwsh/powershell dotnetpj/tools/l4a_compare.ps1 -Steps steps.tsv -Name menu-main [-Frames 130] [-Build] [-SkipCpp]

  steps.tsv 는 cpppj --ui-script 와 같은 문법(프레임<TAB>동작<TAB>인자)이다. 캡처 경로 자리에는 {OUT} 을 쓴다.
    예: "30<TAB>snapshot<TAB>{OUT}\main.bmp"
  {OUT} 은 양쪽 실행에서 각각 extracted/dotnet-oracle-captures/<Name>/cpp, .../dn 으로 바뀐다.
  두 폴더에 같은 이름의 BMP 가 생기므로 cpp_frame_compare.py --dir 로 전부 비교한다.

  AGENTS.md 의 실행 확인 규칙: 이 도구는 게임 창을 연다. 예외 시스템(HJOW-Athlon·VM-W11-Codex·vm-debian-codex)이 아니면
  실행 전에 사용자의 명시적 확인을 받아야 한다.

  보호: originals/ 는 읽기만 한다. 데이터 사본(extracted/dotnet-oracle-data)을 만들 때와 실행 전후에 originals/ 해시 목록을 비교해
  바뀌었으면 실패로 끝낸다. 사용자 루트(NETSTORM_USER_DATA)는 실행마다 빈 임시 폴더를 쓴다.

  종료 코드: 0 모든 캡처 일치, 1 차이 있음, 2 입력·실행 오류.
  (2026-11-11 0.380 에서 추가: 첫 L4a 비교를 반복 가능하게 만들기 위한 도구)

.PARAMETER Steps
  ui-script 템플릿 경로. {OUT} 자리표시자를 쓴다.
.PARAMETER Name
  캡처 폴더 이름(extracted/dotnet-oracle-captures/<Name>).
.PARAMETER Frames
  양쪽 실행의 --frames 값. 스크립트의 마지막 프레임 이상이어야 한다.
.PARAMETER Set
  양쪽 실행의 --set 값. 기본은 영어·4:3·환영 창 없음·자동 데모/팁 끔.
.PARAMETER FixedFrameMs
  가상 시계 간격(ms). 0 이면 쓰지 않는다. 결정론 비교에는 25 를 쓴다.
.PARAMETER Build
  dotnetpj 호스트를 Release 로 먼저 빌드한다. 없으면 이미 빌드된 결과를 쓴다.
.PARAMETER SkipCpp
  cpppj 오라클 실행을 건너뛰고 이미 있는 cpp 캡처를 재사용한다(오라클이 바뀌지 않았을 때만).
#>
param(
    [Parameter(Mandatory = $true)][string]$Steps,
    [Parameter(Mandatory = $true)][string]$Name,
    [int]$Frames = 130,
    [string]$Set = 'SCREENW=1024;SCREENH=768;WelcomeGiven=1;autoDemo=0;tellTips=0',
    [int]$FixedFrameMs = 25,
    [switch]$Build,
    [switch]$SkipCpp
)

$ErrorActionPreference = 'Stop'
# 저장소 루트(이 스크립트는 dotnetpj/tools 에 있다)
$root = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
Set-Location $root

# 오류 메시지를 표준 오류로 쓰고 종료 코드 2로 끝낸다(Write-Error 는 -Stop 설정에서 종료 코드를 1로 만든다).
function Fail {
    param([string]$Message)
    [Console]::Error.WriteLine($Message)
    exit 2
}

# originals/ 의 모든 파일 SHA-256 목록을 한 문자열로 만든다. 실행 전후 비교로 원본 불변을 확인한다.
function Get-OriginalsHash {
    $lines = Get-ChildItem -Path (Join-Path $root 'originals') -Recurse -File | ForEach-Object {
        '{0}  {1}' -f (Get-FileHash -Algorithm SHA256 -LiteralPath $_.FullName).Hash, $_.FullName.Substring($root.Length + 1)
    } | Sort-Object
    return ($lines -join "`n")
}

# ui-script 템플릿의 {OUT} 을 면별 캡처 폴더로 바꿔 실제 스크립트 파일을 만든다.
function New-StepsFile {
    param([string]$Template, [string]$OutDir, [string]$Target)
    $text = [IO.File]::ReadAllText($Template, [Text.Encoding]::UTF8).Replace('{OUT}', $OutDir)
    [IO.File]::WriteAllText($Target, $text, (New-Object Text.UTF8Encoding($false)))
}

# 실행 한 번에 쓰는 빈 사용자 루트를 만든다. 이전 내용이 비교에 섞이지 않게 항상 새로 만든다.
function New-EmptyUserRoot {
    param([string]$Path)
    if (Test-Path $Path) { Remove-Item -Recurse -Force $Path }
    New-Item -ItemType Directory -Force $Path | Out-Null
    return $Path
}

if (-not (Test-Path $Steps)) { Fail "ui-script 템플릿이 없다: $Steps" }
$stepsFull = (Resolve-Path $Steps).Path
$capRoot = Join-Path $root "extracted\dotnet-oracle-captures\$Name"
$cppOut = Join-Path $capRoot 'cpp'
$dnOut = Join-Path $capRoot 'dn'
$work = Join-Path $env:TEMP "l4a-$Name-$PID"
New-Item -ItemType Directory -Force $capRoot, $work | Out-Null

# 데이터 사본이 없으면 만든다(originals 사본 + 클론 커서 자료. dotnetpj 호스트는 cursors/ 를 읽는다).
$dataDir = Join-Path $root 'extracted\dotnet-oracle-data'
if (-not (Test-Path $dataDir)) {
    robocopy (Join-Path $root 'originals') $dataDir /E /NFL /NDL /NJH /NJS /NP | Out-Null
}
if (-not (Test-Path (Join-Path $dataDir 'cursors'))) {
    robocopy (Join-Path $root 'assets\game-data\cursors') (Join-Path $dataDir 'cursors') /E /NFL /NDL /NJH /NJS /NP | Out-Null
}

$before = Get-OriginalsHash
$cppExe = Join-Path $root 'cpppj\build\bin\Release\Netstorm.exe'
$dnProject = Join-Path $root 'dotnetpj\src\Netstorm.Client.Host\Netstorm.Client.Host.csproj'
$dnExe = Join-Path $root 'dotnetpj\src\Netstorm.Client.Host\bin\Release\net10.0\Netstorm.Client.Host.exe'

if ($Build) {
    dotnet build $dnProject -c Release --nologo -v q
    if ($LASTEXITCODE -ne 0) { Fail 'dotnetpj 호스트 빌드 실패' }
}
if (-not (Test-Path $dnExe)) { Fail "dotnetpj 호스트 실행 파일이 없다(-Build 로 빌드): $dnExe" }
if (-not $SkipCpp -and -not (Test-Path $cppExe)) { Fail "cpppj 오라클 실행 파일이 없다: $cppExe" }

$exitCode = 0
try {
    # --- cpppj 오라클 실행 ---
    if (-not $SkipCpp) {
        if (Test-Path $cppOut) { Remove-Item -Recurse -Force $cppOut }
        New-Item -ItemType Directory -Force $cppOut | Out-Null
        $cppSteps = Join-Path $work 'steps-cpp.tsv'
        New-StepsFile -Template $stepsFull -OutDir $cppOut -Target $cppSteps
        $env:NETSTORM_USER_DATA = New-EmptyUserRoot (Join-Path $work 'user-cpp')
        $cppArgs = @('--run', $dataDir, '--window', '--mute-audio', '--frames', $Frames, '--set', $Set, '--ui-script', $cppSteps, '--ui-report', (Join-Path $cppOut 'report.tsv'))
        if ($FixedFrameMs -gt 0) { $cppArgs += @('--fixed-frame-ms', $FixedFrameMs) }
        & $cppExe @cppArgs | Out-Null
        if ($LASTEXITCODE -ne 0) { Fail "cpppj 오라클 종료 코드 $LASTEXITCODE" }
    }

    # --- dotnetpj 새 경로 실행 ---
    if (Test-Path $dnOut) { Remove-Item -Recurse -Force $dnOut }
    New-Item -ItemType Directory -Force $dnOut | Out-Null
    $dnSteps = Join-Path $work 'steps-dn.tsv'
    New-StepsFile -Template $stepsFull -OutDir $dnOut -Target $dnSteps
    $env:NETSTORM_USER_DATA = New-EmptyUserRoot (Join-Path $work 'user-dn')
    $env:NETSTORM_DATA = $dataDir
    $dnArgs = @('--window', '--frames', $Frames, '--set', $Set, '--ui-script', $dnSteps, '--ui-report', (Join-Path $dnOut 'report.tsv'))
    if ($FixedFrameMs -gt 0) { $dnArgs += @('--fixed-frame-ms', $FixedFrameMs) }
    & $dnExe @dnArgs | Out-Null
    if ($LASTEXITCODE -ne 0) { Fail "dotnetpj 호스트 종료 코드 $LASTEXITCODE" }
}
finally {
    Remove-Item Env:NETSTORM_USER_DATA, Env:NETSTORM_DATA -ErrorAction SilentlyContinue
}

# originals/ 가 그대로인지 확인한다.
$after = Get-OriginalsHash
if ($before -ne $after) { Fail 'originals/ 해시 목록이 실행 전후에 달라졌다. 즉시 확인한다.' }

# 캡처 전부를 비교한다(보고서 report.tsv 는 BMP 가 아니라 비교 대상이 아니다).
$env:PYTHONUTF8 = '1'
python (Join-Path $root 'dotnetpj\tools\cpp_frame_compare.py') --dir $cppOut $dnOut --list 5
$exitCode = $LASTEXITCODE
Remove-Item -Recurse -Force $work -ErrorAction SilentlyContinue
exit $exitCode
