<#
.SYNOPSIS
    cpppj의 게임, 통합 테스트 또는 기존 전체 빌드를 선택한다.
.DESCRIPTION
    모드를 생략하면 선택 메뉴를 표시한다. 빈 입력은 취소한다.
    Game은 테스트 구성을 끄고 NetstormCpp만 빌드한다.
    Tests는 netstorm_tests와 필요한 공용 코드/도우미만 빌드한다.
    All은 테스트 구성을 켜고 게임과 모든 집중 테스트를 기존처럼 전체 빌드한다.
    컴파일만 수행하며 게임이나 테스트를 실행하지 않는다.
.EXAMPLE
    powershell -NoProfile -ExecutionPolicy Bypass -File cpppj\build-cpppj.ps1 -Mode Game
.EXAMPLE
    .\cpppj\build-cpppj.ps1 -Mode Tests -TestTarget netstorm_editor_file_ring_tests
#>
[CmdletBinding()]
param(
    # 빌드 범위다. 생략하면 대화형으로 선택한다.
    [ValidateSet('Game', 'Tests', 'All')][string]$Mode,
    # 테스트 전용 모드의 대상이다. 기본값은 통합 실행 파일 하나다.
    [ValidatePattern('^netstorm_[A-Za-z0-9_]+$')][string[]]$TestTarget = @('netstorm_tests'),
    # CMake 빌드 구성이다. Visual Studio와 단일 구성 생성기 모두에 전달한다.
    [ValidateSet('Release', 'Debug', 'RelWithDebInfo', 'MinSizeRel')][string]$Configuration = 'Release',
    # 상대 경로는 현재 셸 위치가 아니라 저장소 루트를 기준으로 해석한다.
    [string]$BuildDirectory = 'cpppj/build',
    # 동시에 실행할 컴파일 작업 수다. 메모리 사용을 제한하도록 기본 네 개다.
    [ValidateRange(1, 256)][int]$Jobs = 4,
    # 새 빌드 폴더의 생성기를 지정한다. 기존 캐시와 다르면 CMake가 오류를 보고한다.
    [string]$Generator,
    # 특정 CMake 실행 파일을 사용할 때 지정한다.
    [string]$CMakePath,
    # 구성만 수행하고 컴파일은 생략한다.
    [switch]$ConfigureOnly,
    # 파일을 만들거나 CMake를 실행하지 않고 명령만 표시한다.
    [switch]$DryRun
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

# 생략된 모드를 사용자에게 입력받는다. 잘못된 입력은 다시 선택하도록 안내한다.
if (-not $Mode) {
    Write-Host 'cpppj 빌드 선택: 1 게임 (권장) / 2 통합 테스트 / 3 전체'
    do {
        $answer = Read-Host '번호 또는 Game/Tests/All 입력 (빈 값: 취소)'
        if ([string]::IsNullOrWhiteSpace($answer)) { Write-Host '빌드를 취소했습니다.'; exit 0 }
        switch ($answer.Trim()) {
            { $_ -in '1', 'Game' } { $Mode = 'Game' }
            { $_ -in '2', 'Tests' } { $Mode = 'Tests' }
            { $_ -in '3', 'All' } { $Mode = 'All' }
            default { Write-Host '1, 2, 3 또는 Game, Tests, All을 입력하세요.' }
        }
    } while (-not $Mode)
}
if ($Mode -ne 'Tests' -and $PSBoundParameters.ContainsKey('TestTarget')) {
    throw '-TestTarget은 -Mode Tests에서만 사용할 수 있습니다.'
}
$repositoryRoot = Split-Path -Parent $PSScriptRoot
$buildRoot = if ([IO.Path]::IsPathRooted($BuildDirectory)) {
    [IO.Path]::GetFullPath($BuildDirectory)
} else {
    [IO.Path]::GetFullPath((Join-Path $repositoryRoot $BuildDirectory))
}
$cachePath = Join-Path $buildRoot 'CMakeCache.txt'

# 지정한 빌드 캐시의 단일 값을 읽는다. 캐시가 없으면 빈 값을 반환한다.
function Get-CachedValue([string]$Name) {
    if (-not (Test-Path -LiteralPath $cachePath -PathType Leaf)) { return $null }
    # 캐시 항목의 타입과 무관하게 이름이 일치하는 한 줄만 찾는다.
    foreach ($line in [IO.File]::ReadLines($cachePath)) {
        if ($line -match ('^' + [regex]::Escape($Name) + ':[^=]+=(.*)$')) { return $Matches[1] }
    }
    return $null
}

# 기존 캐시, PATH, Visual Studio 번들 순으로 CMake를 찾는다. 명시 경로가 최우선이다.
function Find-CMakeTool {
    if ($CMakePath) { return (Get-Command $CMakePath -CommandType Application -ErrorAction Stop).Source }
    $recorded = Get-CachedValue 'CMAKE_COMMAND'
    if ($recorded -and (Test-Path -LiteralPath $recorded -PathType Leaf)) { return $recorded }
    $command = Get-Command cmake -CommandType Application -ErrorAction SilentlyContinue
    if ($command) { return $command.Source }
    $locator = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (Test-Path -LiteralPath $locator -PathType Leaf) {
        $roots = & $locator -all -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
        # 설치된 C++ 도구의 번들 CMake 중 첫 실행 파일을 사용한다.
        foreach ($root in $roots) {
            $candidate = Join-Path $root 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
            if (Test-Path -LiteralPath $candidate -PathType Leaf) { return $candidate }
        }
    }
    throw 'CMake를 찾지 못했습니다. CMake를 설치하거나 -CMakePath를 지정하세요.'
}

$cmakeTool = Find-CMakeTool
$selectedGenerator = if ($Generator) { $Generator } else { Get-CachedValue 'CMAKE_GENERATOR' }
if (-not $selectedGenerator) { $selectedGenerator = $env:CMAKE_GENERATOR }
$testsEnabled = if ($Mode -eq 'Game') { 'OFF' } else { 'ON' }
$configureArguments = @('-S', $PSScriptRoot, '-B', $buildRoot,
    "-DCMAKE_BUILD_TYPE:STRING=$Configuration", "-DNETSTORM_BUILD_TESTS:BOOL=$testsEnabled")
if ($Generator) { $configureArguments += @('-G', $Generator) }
# 새 Visual Studio 빌드는 AMD64를 선택한다. 기존 캐시의 플랫폼은 그대로 사용한다.
if (-not (Test-Path -LiteralPath $cachePath) -and
    (-not $selectedGenerator -or $selectedGenerator -like 'Visual Studio*') -and
    -not $env:CMAKE_GENERATOR_PLATFORM) { $configureArguments += @('-A', 'x64') }

# 명령을 표시한 뒤 문자열 재해석 없이 인자 배열로 실행한다. 실패 코드는 셸에 돌려준다.
function Invoke-CMakeStep([string[]]$Arguments) {
    $displayArguments = @(
        # 표시용 따옴표는 공백/한글 경로를 읽기 쉽게 하며 실행에는 사용하지 않는다.
        foreach ($argument in $Arguments) { "'" + $argument.Replace("'", "''") + "'" }
    )
    Write-Host ("CMake [$Mode]: & '$cmakeTool' " + ($displayArguments -join ' '))
    if (-not $DryRun) {
        & $cmakeTool @Arguments
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    }
}

Invoke-CMakeStep $configureArguments
if ($ConfigureOnly) { exit 0 }
if (-not $DryRun) { $selectedGenerator = Get-CachedValue 'CMAKE_GENERATOR' }
$buildArguments = @('--build', $buildRoot, '--config', $Configuration, '--parallel', "$Jobs")
if ($Mode -eq 'Game') { $buildArguments += @('--target', 'NetstormCpp') }
if ($Mode -eq 'Tests') { $buildArguments += @('--target') + $TestTarget }
# All은 대상을 생략해 CMake의 기존 기본 전체 빌드 동작을 유지한다.
if ($selectedGenerator -like 'Visual Studio*') {
    $buildArguments += @('--', '/p:UseMultiToolTask=true', "/p:MultiProcMaxCount=$Jobs", '/p:EnforceProcessCountAcrossBuilds=true')
}
Invoke-CMakeStep $buildArguments
