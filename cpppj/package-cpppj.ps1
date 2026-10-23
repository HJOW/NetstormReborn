<#
.SYNOPSIS
    cpppj(C++ 클론) 배포판 압축파일(zip)을 만든다.

.DESCRIPTION
    클론 실행 파일(NetstormCpp.exe), Visual C++ 런타임 DLL, 게임 데이터(원본 10.78),
    실행용 배치 파일(Netstorm.bat), 안내문을 하나의 zip 으로 묶는다.
    받은 사람이 압축을 풀고 Netstorm.bat 를 실행하면 별도 설치 없이 클론이 시작되도록 구성한다.
    (2026-10-23 신규. AGENTS.md 의 "배포판 압축파일 생성용 스크립트" 요구사항을 따른다.)

    - 저장 위치(파일명 포함)를 첫 번째 매개변수로 주면 그 위치를 쓴다.
    - 매개변수를 생략하면 실행 중에 입력을 받는다. 빈 값을 입력하면 취소로 보고 종료한다(종료 코드 0).
    - 폴더만 입력하면 그 폴더 안에 기본 파일명으로 만든다. 확장자를 생략하면 .zip 을 붙인다.
    - 실행 파일이 없거나 소스보다 오래됐으면 NetstormCpp 대상만 Release 로 빌드한다(-NoBuild 로 끈다).
    - 게임 데이터는 기본적으로 저장소의 originals 폴더에서 읽기만 한다. 원본 파일은 수정하지 않는다.
    - 사용자별 상태 파일(d\options.cfg, fullscreenStateFile.dat)은 넣지 않는다. 받은 사람의 첫 실행에서 새로 만들어진다.

.PARAMETER OutputPath
    배포판 압축파일을 저장할 위치(파일명 포함). 생략하면 실행 중에 입력받는다.

.PARAMETER GameSource
    게임 데이터를 가져올 폴더. 기본값은 저장소의 originals 이다.
    netstorm.tarc, Netstorm.exe, d, sound, music 이 들어 있어야 한다.

.PARAMETER ExePath
    묶을 클론 실행 파일. 기본값은 cpppj\build\bin\Release\NetstormCpp.exe 이다.
    직접 지정하면 빌드와 오래됨 검사를 하지 않는다.

.PARAMETER NoBuild
    실행 파일이 오래됐어도 빌드하지 않고 그대로 묶는다(경고만 표시). 파일이 없으면 오류로 멈춘다.

.PARAMETER NoRuntime
    Visual C++ 런타임 DLL 을 넣지 않는다. 받는 쪽에 런타임이 설치돼 있을 때만 쓴다.

.PARAMETER DryRun
    압축하지 않고 들어갈 항목과 제외 항목, 빌드 필요 여부만 보여 준다.

.EXAMPLE
    powershell -NoProfile -ExecutionPolicy Bypass -File cpppj\package-cpppj.ps1 D:\dist\NetstormReborn-cpppj.zip

.EXAMPLE
    powershell -NoProfile -ExecutionPolicy Bypass -File cpppj\package-cpppj.ps1 -DryRun D:\dist
#>
[CmdletBinding()]
param(
    # 배포판 압축파일 저장 위치(파일명 포함)다. 비어 있으면 실행 중에 입력받는다. (2026-10-23 AGENTS.md 요구사항으로 추가)
    [Parameter(Position = 0)]
    [string] $OutputPath,

    # 게임 데이터 원천 폴더다. 비어 있으면 저장소의 originals 를 쓴다. (2026-10-23 다른 위치의 데이터로 묶을 수 있게 추가)
    [string] $GameSource,

    # 묶을 실행 파일 경로다. 비어 있으면 Release 빌드 산출물을 쓴다. (2026-10-23 미리 만든 실행 파일을 쓰게 하려고 추가)
    [string] $ExePath,

    # 켜면 오래된 실행 파일도 다시 빌드하지 않는다. (2026-10-23 긴 빌드를 건너뛰는 용도로 추가)
    [switch] $NoBuild,

    # 켜면 Visual C++ 런타임 DLL 을 동봉하지 않는다. (2026-10-23 런타임이 이미 있는 환경용으로 추가)
    [switch] $NoRuntime,

    # 켜면 압축 없이 계획만 출력한다. (2026-10-23 실행 전 점검용으로 추가)
    [switch] $DryRun
)

# 정의되지 않은 변수 사용을 오류로 잡는다. (2026-10-23 오타로 인한 조용한 실패를 막으려고 추가)
Set-StrictMode -Version 2
# 모든 오류에서 즉시 멈춘다. 불완전한 배포판이 만들어지지 않게 한다. (2026-10-23 신규)
$ErrorActionPreference = 'Stop'

# ----------------------------------------------------------------------------------------------
# 상수
# ----------------------------------------------------------------------------------------------

# 이 스크립트가 있는 cpppj 폴더다. 빌드 폴더와 소스 폴더를 여기서 찾는다. (2026-10-23 신규)
$CppRoot = $PSScriptRoot

# 저장소 최상위 폴더다. originals 와 LICENSE 를 여기서 찾는다. (2026-10-23 신규)
$RepoRoot = Split-Path -Parent $CppRoot

# 압축파일 안에서 모든 항목을 감싸는 최상위 폴더 이름이다. 풀 때 현재 폴더가 어지러워지지 않게 한다. (2026-10-23 신규)
$PackageRootName = 'NetstormReborn-cpppj'

# 클론 실행 파일 이름이다. 압축파일 최상위에 이 이름으로 넣는다. (2026-10-23 신규)
$ExeName = 'NetstormCpp.exe'

# 압축파일 안의 게임 데이터 폴더 이름이다. 실행 배치 파일이 `--run game` 으로 이 폴더를 가리킨다. (2026-10-23 신규)
$GameFolderName = 'game'

# 기본 실행 파일 위치다. `cmake --build cpppj/build --config Release` 의 산출물 경로이다. (2026-10-23 신규)
$DefaultExePath = Join-Path $CppRoot 'build\bin\Release\NetstormCpp.exe'

# 폴더만 입력했을 때 쓰는 기본 압축파일 이름이다. 날짜를 붙여 이전 배포판을 덮어쓰지 않게 한다. (2026-10-23 신규)
$DefaultZipName = 'NetstormReborn-cpppj-{0}.zip' -f (Get-Date -Format 'yyyyMMdd')

# 게임 데이터 원천에서 그대로 복사할 파일이다.
# Netstorm.exe 는 실행하지 않고 아이콘·커서·로딩 그림 리소스를 읽는 자료로만 쓴다(ClientMain.cpp 의 LOAD_LIBRARY_AS_DATAFILE).
# netstorm.tarc 는 타입·지도·미션 아카이브이다. (2026-10-23 클라이언트가 읽는 파일을 소스에서 확인해 정함)
$GameFiles = @('netstorm.tarc', 'Netstorm.exe')

# 게임 데이터 원천에서 통째로 복사할 폴더다. d 는 셰이프·팔레트·지도·미션·글꼴, sound 는 효과음, music 은 음악이다.
# (2026-10-23 클라이언트가 읽는 폴더를 소스에서 확인해 정함. help, movie, w 등은 클론이 읽지 않아 제외)
$GameDirs = @('d', 'sound', 'music')

# 게임 데이터 폴더 기준으로 제외할 상대 경로다(대소문자 무시, 슬래시 구분).
# d/options.cfg 는 실행하며 만들어지는 사용자별 설정이므로 배포판에 섞지 않는다. 없으면 setup.cfg 기본값으로 시작한다. (2026-10-23 신규)
$ExcludedRelativePaths = @('d/options.cfg')

# 폴더와 상관없이 제외할 파일 이름 패턴이다.
# *.php 는 내려받은 지도 묶음(zip)의 잔재이고, fullscreenStateFile.dat 는 전체화면 시도 표시 파일이며 남아 있으면 창 모드로 강제된다.
# Thumbs.db, desktop.ini 는 탐색기가 만든 파일이다. (2026-10-23 신규)
$ExcludedNamePatterns = @('*.php', 'fullscreenStateFile.dat', 'Thumbs.db', 'desktop.ini')

# 게임 데이터 원천에 반드시 있어야 하는 파일(원천 기준 상대 경로)이다. 하나라도 없으면 불완전한 데이터로 보고 멈춘다. (2026-10-23 신규)
$RequiredGameEntries = @('netstorm.tarc', 'Netstorm.exe', 'd/setup.cfg', 'd/_shapes.shp', 'd/gifcloud.col')

# 실행 파일이 가져올 수 있는 Visual C++ 런타임 DLL 이름이다.
# 실행 파일 안에서 이름이 발견된 것만 동봉한다. UCRT(api-ms-win-crt-*)는 Windows 10/11 에 기본 포함되어 넣지 않는다. (2026-10-23 신규)
$RuntimeDllCandidates = @(
    'msvcp140.dll', 'msvcp140_1.dll', 'msvcp140_2.dll', 'msvcp140_atomic_wait.dll', 'msvcp140_codecvt_ids.dll',
    'vcruntime140.dll', 'vcruntime140_1.dll', 'vcruntime140_threads.dll', 'concrt140.dll')

# 실행 파일이 소스보다 오래됐는지 비교할 때 보는 파일 확장자(소문자)다. (2026-10-23 신규)
$SourceExtensions = @('.cpp', '.h', '.hpp', '.cmake', '.txt')

# ----------------------------------------------------------------------------------------------
# 함수
# ----------------------------------------------------------------------------------------------

# 바이트 수를 읽기 쉬운 문자열(예: 12.3 MB)로 바꾼다. 요약 출력에 쓴다. (2026-10-23 신규)
function Format-Size {
    param([long] $Bytes)
    return ('{0:N1} MB' -f ($Bytes / 1MB))
}

# 사용자가 입력한 저장 위치를 완전한 .zip 파일 경로로 바꾼다.
# 따옴표·환경 변수·상대 경로를 처리하고, 폴더만 주어지면 기본 파일명을 붙이며, 확장자가 없으면 .zip 을 붙인다.
# .zip 이 아닌 확장자는 오류로 처리한다. (2026-10-23 신규. 붙여넣은 경로의 따옴표와 폴더 입력을 모두 받으려고 추가)
function Resolve-ZipPath {
    param([string] $Raw)
    # 양끝 공백과 따옴표를 벗긴다. 탐색기에서 "경로 복사"한 값이 따옴표를 포함하기 때문이다.
    $text = $Raw.Trim().Trim('"', "'").Trim()
    $text = [System.Environment]::ExpandEnvironmentVariables($text)
    # 입력이 구분자로 끝나면 폴더를 뜻한다.
    $endsWithSeparator = $text.EndsWith('\') -or $text.EndsWith('/')
    # 상대 경로는 PowerShell 의 현재 위치를 기준으로 한다. .NET 의 현재 폴더와 다를 수 있다.
    if (-not [System.IO.Path]::IsPathRooted($text)) {
        $text = Join-Path (Get-Location).ProviderPath $text
    }
    $text = [System.IO.Path]::GetFullPath($text)
    if ($endsWithSeparator -or (Test-Path -LiteralPath $text -PathType Container)) {
        $text = Join-Path $text $DefaultZipName
    }
    $extension = [System.IO.Path]::GetExtension($text).ToLowerInvariant()
    if ([string]::IsNullOrEmpty($extension)) {
        $text = $text + '.zip'
    } elseif ($extension -ne '.zip') {
        throw "저장 위치의 확장자는 .zip 이어야 합니다: ${text}"
    }
    return $text
}

# 저장 위치를 사용자에게 입력받아 그대로 돌려준다. 빈 문자열이면 호출한 쪽이 취소로 처리한다.
# (2026-10-23 AGENTS.md: 매개변수가 없으면 저장 위치를 입력받고, 빈 값은 취소)
function Read-ZipPathFromUser {
    Write-Host '배포판 압축파일을 저장할 위치를 파일명까지 입력하세요.'
    Write-Host '  예: D:\dist\NetstormReborn-cpppj.zip   (폴더만 입력하면 기본 파일명을 사용합니다)'
    Write-Host '  아무것도 입력하지 않고 Enter 를 누르면 취소합니다.'
    return (Read-Host '저장 위치')
}

# 설치된 Visual Studio 의 설치 폴더 목록을 최신순으로 돌려준다. vswhere 가 없으면 빈 목록이다.
# cmake 와 런타임 DLL 을 VS 설치 안에서 찾는 데 쓴다. (2026-10-23 신규)
function Get-VisualStudioRoots {
    $roots = New-Object System.Collections.Generic.List[string]
    $programFilesX86 = ${env:ProgramFiles(x86)}
    if ($programFilesX86) {
        $vswhere = Join-Path $programFilesX86 'Microsoft Visual Studio\Installer\vswhere.exe'
        if (Test-Path -LiteralPath $vswhere) {
            # vswhere 는 줄마다 설치 폴더 하나를 출력한다. -sort 로 최신 버전이 먼저 온다.
            foreach ($line in (& $vswhere -products * -sort -property installationPath)) {
                if ($line) { $roots.Add([string]$line) }
            }
        }
    }
    return $roots.ToArray()
}

# cmake 실행 파일 경로를 찾는다. 빌드 폴더를 구성한 cmake(CMakeCache.txt 의 CMAKE_COMMAND)를 가장 먼저 쓰고,
# 없으면 PATH 의 cmake, 그다음 Visual Studio 에 포함된 cmake 를 쓴다. 못 찾으면 $null 이다.
# (2026-10-23 신규. 개발자 명령 프롬프트가 아니어도 빌드되게 하고, 빌드 폴더를 만든 것과 다른 버전의 cmake 로 이어 빌드해
#  생성기 상태가 어긋나는 일을 피하려고 추가)
function Find-CMakeTool {
    $cache = Join-Path $CppRoot 'build\CMakeCache.txt'
    if (Test-Path -LiteralPath $cache) {
        # 캐시에서 이 빌드 폴더를 구성한 cmake 의 경로를 한 줄씩 찾는다.
        foreach ($line in [System.IO.File]::ReadLines($cache)) {
            if ($line.StartsWith('CMAKE_COMMAND:INTERNAL=')) {
                $recorded = $line.Substring('CMAKE_COMMAND:INTERNAL='.Length)
                if (Test-Path -LiteralPath $recorded) { return $recorded }
                break
            }
        }
    }
    $command = Get-Command cmake -ErrorAction SilentlyContinue
    if ($command) { return $command.Source }
    # PATH 에 없으면 설치된 Visual Studio 마다 번들 cmake 위치를 확인한다.
    foreach ($root in @(Get-VisualStudioRoots)) {
        $candidate = Join-Path $root 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
        if (Test-Path -LiteralPath $candidate) { return $candidate }
    }
    return $null
}

# cpppj 소스(src, cmake 폴더와 최상위 CMakeLists.txt) 중 가장 최근에 바뀐 파일의 시각(UTC)을 돌려준다.
# 실행 파일이 이 시각보다 오래되면 소스 변경이 반영되지 않은 실행 파일이다. (2026-10-23 신규. 오래된 exe 를 배포하는 실수를 막으려고 추가)
function Get-NewestSourceTime {
    $newest = [datetime]::MinValue
    $files = New-Object System.Collections.Generic.List[string]
    # 소스가 있는 두 폴더를 재귀적으로 훑는다. hist 같은 과거 복사본 폴더는 보지 않는다.
    foreach ($folder in @((Join-Path $CppRoot 'src'), (Join-Path $CppRoot 'cmake'))) {
        if (Test-Path -LiteralPath $folder -PathType Container) {
            # 폴더 안의 모든 파일 경로를 모은다.
            foreach ($file in [System.IO.Directory]::EnumerateFiles($folder, '*', [System.IO.SearchOption]::AllDirectories)) {
                $files.Add($file)
            }
        }
    }
    $files.Add((Join-Path $CppRoot 'CMakeLists.txt'))
    # 모은 파일 중 소스 확장자인 것만 최종 수정 시각을 비교한다.
    foreach ($file in $files) {
        if (-not (Test-Path -LiteralPath $file)) { continue }
        $extension = [System.IO.Path]::GetExtension($file).ToLowerInvariant()
        if ($SourceExtensions -contains $extension) {
            $time = [System.IO.File]::GetLastWriteTimeUtc($file)
            if ($time -gt $newest) { $newest = $time }
        }
    }
    return $newest
}

# NetstormCpp 대상만 Release 로 빌드한다. 빌드 폴더가 아직 구성되지 않았으면 먼저 구성한다.
# 시험 프로그램까지 빌드하면 오래 걸려 실행 파일 대상만 지정한다. (2026-10-23 신규)
function Invoke-ExeBuild {
    $cmake = Find-CMakeTool
    if (-not $cmake) {
        throw 'cmake 를 찾을 수 없습니다. cmake 를 설치하거나, 미리 빌드한 실행 파일을 -ExePath 로 지정하세요.'
    }
    $buildDir = Join-Path $CppRoot 'build'
    if (-not (Test-Path -LiteralPath (Join-Path $buildDir 'CMakeCache.txt'))) {
        Write-Host "빌드 폴더가 구성되지 않아 먼저 구성합니다: ${buildDir}"
        & $cmake -S $CppRoot -B $buildDir
        if ($LASTEXITCODE -ne 0) { throw "cmake 구성에 실패했습니다(종료 코드 ${LASTEXITCODE})." }
    }
    Write-Host 'NetstormCpp 를 Release 로 빌드합니다(시간이 걸릴 수 있습니다)...'
    & $cmake --build $buildDir --config Release --target NetstormCpp
    if ($LASTEXITCODE -ne 0) { throw "빌드에 실패했습니다(종료 코드 ${LASTEXITCODE})." }
}

# 실행 파일이 필요로 하는 Visual C++ 런타임 DLL 의 위치 계획을 돌려준다.
# 실행 파일 바이트에서 DLL 이름을 찾아 필요한 것만 고르고, 개발자 환경 변수 -> Visual Studio 재배포 폴더 -> 시스템 폴더 순으로 파일을 찾는다.
# 필요한 DLL 을 하나라도 못 찾으면 오류로 멈춘다(런타임이 없는 PC 에서 실행되지 않는 배포판을 막기 위해).
# (2026-10-23 신규. 실행 파일이 MSVCP140/VCRUNTIME140 에 동적 링크되어 있어 받는 PC 에 설치가 없어도 실행되게 하려고 추가)
function Get-RuntimeDllPlan {
    param([string] $ExeFile)
    $bytes = [System.IO.File]::ReadAllBytes($ExeFile)
    $text = [System.Text.Encoding]::ASCII.GetString($bytes)
    $needed = New-Object System.Collections.Generic.List[string]
    # 후보 이름마다 실행 파일의 임포트 이름에 들어 있는지 확인한다.
    foreach ($name in $RuntimeDllCandidates) {
        if ($text.IndexOf($name, [System.StringComparison]::OrdinalIgnoreCase) -ge 0) { $needed.Add($name) }
    }

    # 파일을 찾을 폴더 후보다. 앞에 있을수록 우선한다. 각 항목은 폴더와 출처 이름의 쌍이다.
    $searchDirs = New-Object System.Collections.Generic.List[object]
    if ($env:VCToolsRedistDir) {
        $redistX64 = Join-Path $env:VCToolsRedistDir 'x64'
        if (Test-Path -LiteralPath $redistX64) {
            # 개발자 명령 프롬프트가 지정한 재배포 폴더의 CRT 하위 폴더를 후보에 넣는다.
            foreach ($crt in @(Get-ChildItem -LiteralPath $redistX64 -Directory -Filter 'Microsoft.VC*.CRT')) {
                $searchDirs.Add([pscustomobject]@{ Path = $crt.FullName; Origin = 'VCToolsRedistDir' })
            }
        }
    }
    # 설치된 Visual Studio 마다 가장 높은 버전의 재배포 CRT 폴더를 후보에 넣는다.
    foreach ($root in @(Get-VisualStudioRoots)) {
        $redistRoot = Join-Path $root 'VC\Redist\MSVC'
        if (-not (Test-Path -LiteralPath $redistRoot)) { continue }
        # 버전 이름(14.51.36231 등)인 폴더만 골라 높은 버전부터 본다. v145 같은 이름은 제외한다.
        $versions = @(Get-ChildItem -LiteralPath $redistRoot -Directory |
            Where-Object { $_.Name -match '^\d+(\.\d+)+$' } |
            Sort-Object { [version]$_.Name } -Descending)
        # 각 버전의 x64 CRT 폴더를 후보에 추가한다.
        foreach ($version in $versions) {
            $x64 = Join-Path $version.FullName 'x64'
            if (-not (Test-Path -LiteralPath $x64)) { continue }
            # x64 아래의 Microsoft.VC*.CRT 폴더가 실제 DLL 이 있는 곳이다.
            foreach ($crt in @(Get-ChildItem -LiteralPath $x64 -Directory -Filter 'Microsoft.VC*.CRT')) {
                $searchDirs.Add([pscustomobject]@{ Path = $crt.FullName; Origin = 'Visual Studio ' + $version.Name })
            }
        }
    }
    # 마지막 수단으로 시스템 폴더를 넣는다. 설치된 재배포 패키지의 사본이다.
    $searchDirs.Add([pscustomobject]@{ Path = (Join-Path $env:SystemRoot 'System32'); Origin = 'System32' })

    $plan = New-Object System.Collections.Generic.List[object]
    # 필요한 DLL 이름마다 후보 폴더를 차례로 뒤져 처음 발견한 파일을 채택한다.
    foreach ($name in $needed) {
        $found = $null
        # 후보 폴더를 앞에서부터 확인한다.
        foreach ($dir in $searchDirs) {
            $candidate = Join-Path $dir.Path $name
            if (Test-Path -LiteralPath $candidate -PathType Leaf) {
                $found = [pscustomobject]@{
                    Name    = $name
                    Path    = $candidate
                    Origin  = $dir.Origin
                    Version = (Get-Item -LiteralPath $candidate).VersionInfo.FileVersion
                }
                break
            }
        }
        if (-not $found) {
            throw "필요한 런타임 DLL 을 찾지 못했습니다: ${name}. Visual Studio 의 C++ 구성 요소를 설치하거나 -NoRuntime 으로 생략하세요."
        }
        $plan.Add($found)
    }
    return $plan.ToArray()
}

# 저장소의 Git 정보를 한 줄로 돌려준다(커밋 해시, cpppj 의 미커밋 변경 여부). git 이 없으면 그 사실을 적는다.
# 배포판이 어느 소스에서 나왔는지 BUILD_INFO.txt 에 남기려는 용도이다. (2026-10-23 신규)
function Get-GitSummary {
    $git = Get-Command git -ErrorAction SilentlyContinue
    if (-not $git) { return '(git 을 찾을 수 없음)' }
    # git 이 stderr 로 경고를 내도 스크립트가 멈추지 않게 이 블록에서만 오류 처리를 완화한다.
    $previous = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    try {
        $head = (& git -C $RepoRoot rev-parse --short HEAD 2>$null)
        $dirty = (& git -C $RepoRoot status --porcelain -- cpppj 2>$null)
        if (-not $head) { return '(git 저장소 정보를 읽지 못함)' }
        if ($dirty) { return ('{0} (cpppj 에 커밋되지 않은 변경 있음)' -f $head) }
        return [string]$head
    } finally {
        $ErrorActionPreference = $previous
    }
}

# 실행용 배치 파일(.bat)의 본문을 돌려준다. WindowMode 가 참이면 `--window` 로 창 모드를 강제한다.
# cmd.exe 코드 페이지 문제를 피하려고 ASCII 문자만 쓴다. 게임 폴더(game)는 이 파일 기준 상대 경로로 지정한다.
# (2026-10-23 신규. 실행 파일은 인자 없이 실행하면 빌드 정보만 출력하므로 `--run game` 을 대신 호출하는 런처가 필요하다)
function New-LauncherText {
    param([bool] $WindowMode)
    $runLine = if ($WindowMode) { '"NetstormCpp.exe" --run game --window %*' } else { '"NetstormCpp.exe" --run game %*' }
    $title = if ($WindowMode) { 'starts the clone in window mode' } else { 'starts the clone with the saved screen mode settings' }
    $lines = @(
        '@echo off',
        ('rem NetStorm cpppj launcher - ' + $title + '.'),
        'setlocal',
        'pushd "%~dp0"',
        'if not exist "NetstormCpp.exe" goto :missing',
        'if not exist "game\netstorm.tarc" goto :missing',
        $runLine,
        'set "RC=%ERRORLEVEL%"',
        'popd',
        'if "%RC%"=="0" exit /b 0',
        'echo.',
        'echo NetstormCpp.exe exited with code %RC%.',
        'pause',
        'exit /b %RC%',
        ':missing',
        'popd',
        'echo NetstormCpp.exe or game\netstorm.tarc was not found. Extract the whole archive first.',
        'pause',
        'exit /b 1')
    return (($lines -join "`r`n") + "`r`n")
}

# 압축파일 최상위에 넣는 안내문(README.txt) 본문을 돌려준다. 한국어와 영어를 함께 적는다.
# 받는 사람이 풀고 실행하는 방법, 폴더 구성, 설정 초기화 방법을 알려 준다. (2026-10-23 신규)
function New-ReadmeText {
    $lines = @(
        'NetStorm Reborn - cpppj (C++ clone) / NetStorm Reborn - cpppj (C++ 클론)',
        '========================================================================',
        '',
        '[실행 방법]',
        '1. 이 압축파일을 쓰기 가능한 폴더에 풉니다. Program Files 같은 보호 폴더는 피하세요.',
        '   설정이 game\d\options.cfg 에 저장되므로 쓰기 권한이 필요합니다.',
        '2. Netstorm.bat 를 실행합니다. 창 모드로 시작합니다.',
        '   Netstorm-SavedMode.bat 는 저장된 화면 모드 설정(전체화면 포함)을 따릅니다.',
        '',
        '[필요 환경]',
        '- Windows 10 / 11 (64비트)',
        '- Visual C++ 런타임(msvcp140.dll 등)은 이 폴더에 들어 있어 따로 설치하지 않아도 됩니다.',
        '',
        '[폴더 구성]',
        '  NetstormCpp.exe     클론 실행 파일',
        '  *.dll               Visual C++ 런타임',
        '  Netstorm.bat        창 모드 실행',
        '  Netstorm-SavedMode.bat  저장된 화면 모드로 실행',
        '  game\               게임 데이터 (원본 Netstorm 10.78)',
        '    netstorm.tarc, d\, sound\, music\',
        '    Netstorm.exe      원본 실행 파일. 클론이 아이콘, 커서, 로딩 그림을 읽는 자료로만 쓰며',
        '                      직접 실행하지 않습니다.',
        '  BUILD_INFO.txt      이 배포판을 만든 소스와 실행 파일 정보',
        '  LICENSE.txt         라이선스',
        '',
        '[설정 초기화]',
        'game\d\options.cfg 와 game\fullscreenStateFile.dat 를 지우면 기본 설정으로 돌아갑니다.',
        '',
        '[알림]',
        '- 개발 중인 클론이라 원본의 모든 기능이 구현된 것은 아닙니다. 멀티플레이는 포함되지 않습니다.',
        '',
        '[How to run]',
        '1. Extract the archive to a writable folder (not Program Files). Settings are saved to game\d\options.cfg.',
        '2. Run Netstorm.bat (window mode). Netstorm-SavedMode.bat follows the saved screen mode settings.',
        '3. Requires Windows 10/11 x64. The Visual C++ runtime DLLs are bundled in this folder.',
        '4. game\Netstorm.exe is the original executable. The clone only reads its icon/cursor resources; do not run it directly.',
        '5. Delete game\d\options.cfg and game\fullscreenStateFile.dat to reset settings.',
        '6. This is a work-in-progress clone; multiplayer is not included.')
    return (($lines -join "`r`n") + "`r`n")
}

# 목록에 파일 항목을 추가한다. 압축할 때 원본 파일을 스트림으로 읽는다.
# Entry 는 압축파일 안의 경로(슬래시 구분), Source 는 디스크 상의 원본 파일 경로이다. (2026-10-23 신규)
function Add-FileItem {
    param($Items, [string] $Entry, [string] $Source)
    $info = New-Object System.IO.FileInfo($Source)
    $Items.Add([pscustomobject]@{
        Entry  = $Entry
        Source = $Source
        Bytes  = $null
        Length = $info.Length
        Time   = $info.LastWriteTime
    })
}

# 목록에 스크립트가 직접 만든 텍스트 항목을 추가한다.
# UseBom 이 참이면 UTF-8 BOM 을 붙여 메모장 등에서 한글이 깨지지 않게 하고, 거짓이면 ASCII 로 쓴다(.bat 용).
# (2026-10-23 신규. 배치 파일은 BOM 이 있으면 첫 줄이 깨져서 ASCII 로 분리했다)
function Add-TextItem {
    param($Items, [string] $Entry, [string] $Text, [bool] $UseBom)
    $encoding = if ($UseBom) { New-Object System.Text.UTF8Encoding($true) } else { [System.Text.Encoding]::ASCII }
    $preamble = $encoding.GetPreamble()
    $body = $encoding.GetBytes($Text)
    $bytes = New-Object byte[] ($preamble.Length + $body.Length)
    [System.Array]::Copy($preamble, 0, $bytes, 0, $preamble.Length)
    [System.Array]::Copy($body, 0, $bytes, $preamble.Length, $body.Length)
    $Items.Add([pscustomobject]@{
        Entry  = $Entry
        Source = $null
        Bytes  = $bytes
        Length = [long]$bytes.Length
        Time   = Get-Date
    })
}

# 게임 데이터 폴더 기준 상대 경로(슬래시 구분)가 제외 대상인지 판정한다.
# 정확히 일치하는 경로 목록과 파일 이름 패턴 목록을 차례로 본다. (2026-10-23 신규)
function Test-GameExcluded {
    param([string] $Relative)
    # 경로 전체가 제외 목록과 같은지 확인한다.
    foreach ($path in $ExcludedRelativePaths) {
        if ($Relative -ieq $path) { return $true }
    }
    $leaf = $Relative.Substring($Relative.LastIndexOf('/') + 1)
    # 파일 이름이 제외 패턴에 맞는지 확인한다.
    foreach ($pattern in $ExcludedNamePatterns) {
        if ($leaf -like $pattern) { return $true }
    }
    return $false
}

# 게임 데이터 원천 폴더를 검사하고 압축할 항목 목록을 만든다. 결과는 Items(항목)와 Skipped(제외한 상대 경로)를 가진 객체이다.
# 필수 파일이 없으면 오류로 멈춘다. 항목은 경로 순(서수 비교)으로 정렬해 같은 입력이면 같은 순서로 압축한다.
# (2026-10-23 신규)
function Get-GameItems {
    param([string] $Source)
    # 필수 파일이 모두 있는지 먼저 확인한다.
    foreach ($required in $RequiredGameEntries) {
        $path = Join-Path $Source ($required -replace '/', '\')
        if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
            throw "게임 데이터에 필수 파일이 없습니다: ${required} (원천: ${Source})"
        }
    }
    $items = New-Object System.Collections.Generic.List[object]
    $skipped = New-Object System.Collections.Generic.List[string]
    $prefix = $PackageRootName + '/' + $GameFolderName

    # 최상위에서 그대로 복사할 파일을 추가한다.
    foreach ($name in $GameFiles) {
        Add-FileItem -Items $items -Entry ($prefix + '/' + $name) -Source (Join-Path $Source $name)
    }
    # 통째로 복사할 폴더마다 하위 파일을 모아 제외 규칙을 적용한다.
    foreach ($dir in $GameDirs) {
        $base = Join-Path $Source $dir
        if (-not (Test-Path -LiteralPath $base -PathType Container)) {
            throw "게임 데이터에 필수 폴더가 없습니다: ${dir} (원천: ${Source})"
        }
        $files = New-Object System.Collections.Generic.List[string]
        # 폴더 아래 모든 파일의 전체 경로를 모은다.
        foreach ($file in [System.IO.Directory]::EnumerateFiles($base, '*', [System.IO.SearchOption]::AllDirectories)) {
            $files.Add($file)
        }
        $files.Sort([System.StringComparer]::Ordinal)
        # 파일마다 상대 경로를 만들고 제외 대상이면 건너뛴다.
        foreach ($file in $files) {
            $relative = $dir + '/' + $file.Substring($base.Length).TrimStart('\', '/').Replace('\', '/')
            if (Test-GameExcluded -Relative $relative) {
                $skipped.Add($relative)
                continue
            }
            Add-FileItem -Items $items -Entry ($prefix + '/' + $relative) -Source $file
        }
    }
    return [pscustomobject]@{ Items = $items; Skipped = $skipped }
}

# 항목 목록을 zip 으로 쓴다. 항목 이름은 슬래시 구분 UTF-8 이며 압축 수준은 Optimal 이다.
# 원본 파일을 읽는 중에 다른 프로그램이 열고 있어도 읽을 수 있게 공유 읽기로 연다.
# 진행 상황은 PowerShell 진행 표시줄로 보여 준다. (2026-10-23 신규. Compress-Archive 는 5.1 에서 역슬래시 이름을 만들어 직접 구현)
function Write-ZipPackage {
    param($Items, [string] $PartialPath)
    $totalBytes = [long]0
    # 진행률 계산을 위해 전체 바이트 수를 합산한다.
    foreach ($item in $Items) { $totalBytes += $item.Length }
    $doneBytes = [long]0
    $doneCount = 0
    $minimumTime = [datetime]'1980-01-01'

    $stream = [System.IO.File]::Open($PartialPath, [System.IO.FileMode]::Create, [System.IO.FileAccess]::ReadWrite, [System.IO.FileShare]::None)
    try {
        $zip = New-Object System.IO.Compression.ZipArchive($stream, [System.IO.Compression.ZipArchiveMode]::Create, $false, (New-Object System.Text.UTF8Encoding($false)))
        try {
            # 항목을 순서대로 압축파일에 쓴다.
            foreach ($item in $Items) {
                $entry = $zip.CreateEntry($item.Entry, [System.IO.Compression.CompressionLevel]::Optimal)
                # zip 의 시각은 1980년 이후만 표현할 수 있어 그 이전 시각은 보정한다.
                $time = $item.Time
                if ($time -lt $minimumTime) { $time = $minimumTime }
                $entry.LastWriteTime = [System.DateTimeOffset]$time
                $output = $entry.Open()
                try {
                    if ($null -ne $item.Bytes) {
                        $output.Write($item.Bytes, 0, $item.Bytes.Length)
                    } else {
                        $reader = [System.IO.File]::Open($item.Source, [System.IO.FileMode]::Open, [System.IO.FileAccess]::Read, [System.IO.FileShare]::ReadWrite)
                        try { $reader.CopyTo($output, 1048576) } finally { $reader.Dispose() }
                    }
                } finally {
                    $output.Dispose()
                }
                $doneBytes += $item.Length
                $doneCount++
                # 항목마다 갱신하면 느려지므로 64개마다 한 번 진행 표시줄을 갱신한다.
                if (($doneCount % 64) -eq 0) {
                    $percent = if ($totalBytes -gt 0) { [int](100 * $doneBytes / $totalBytes) } else { 100 }
                    Write-Progress -Activity '배포판 압축 중' -Status ('{0} / {1} 항목' -f $doneCount, $Items.Count) -PercentComplete $percent
                }
            }
        } finally {
            $zip.Dispose()
        }
    } finally {
        $stream.Dispose()
        Write-Progress -Activity '배포판 압축 중' -Completed
    }
}

# 만든 zip 을 다시 열어 항목 수·이름·크기가 계획과 같은지, 들어가면 안 되는 파일이 없는지 확인한다.
# 문제 설명 문자열의 배열을 돌려주며 비어 있으면 통과이다. (2026-10-23 신규. 불완전한 압축파일이 배포되는 것을 막으려고 추가)
function Test-ZipPackage {
    param($Items, [string] $ZipPath)
    $problems = New-Object System.Collections.Generic.List[string]
    $stream = [System.IO.File]::Open($ZipPath, [System.IO.FileMode]::Open, [System.IO.FileAccess]::Read, [System.IO.FileShare]::Read)
    try {
        $zip = New-Object System.IO.Compression.ZipArchive($stream, [System.IO.Compression.ZipArchiveMode]::Read, $false, (New-Object System.Text.UTF8Encoding($false)))
        try {
            $actual = @{}
            # 압축파일 안의 항목을 이름 -> 크기 표로 만든다.
            foreach ($entry in $zip.Entries) { $actual[$entry.FullName] = $entry.Length }
            if ($zip.Entries.Count -ne $Items.Count) {
                $problems.Add(('항목 수가 다릅니다: 계획 {0}개, 실제 {1}개' -f $Items.Count, $zip.Entries.Count))
            }
            # 계획한 항목이 모두 같은 크기로 들어 있는지 확인한다.
            foreach ($item in $Items) {
                if (-not $actual.ContainsKey($item.Entry)) {
                    $problems.Add('누락: ' + $item.Entry)
                } elseif ([long]$actual[$item.Entry] -ne [long]$item.Length) {
                    $problems.Add(('크기 불일치: {0} (계획 {1}, 실제 {2})' -f $item.Entry, $item.Length, $actual[$item.Entry]))
                }
            }
            # 사용자별 상태 파일이나 서버 잔재가 섞였는지 이름으로 확인한다.
            foreach ($name in $actual.Keys) {
                $leaf = $name.Substring($name.LastIndexOf('/') + 1)
                if ($name -like '*/d/options.cfg' -or $leaf -like 'fullscreenStateFile.dat' -or $leaf -like '*.php') {
                    $problems.Add('들어가면 안 되는 파일: ' + $name)
                }
            }
        } finally {
            $zip.Dispose()
        }
    } finally {
        $stream.Dispose()
    }
    return $problems.ToArray()
}

# ----------------------------------------------------------------------------------------------
# 본문
# ----------------------------------------------------------------------------------------------

# 최종 종료 코드다. 정상 완료와 사용자 취소는 0, 실패는 1이다. (2026-10-23 신규)
$exitCode = 0
# 쓰는 도중의 임시 압축파일 경로다. 성공하면 최종 이름으로 옮기고, 실패하면 지운다. (2026-10-23 신규)
$partialPath = $null

try {
    $startedAt = Get-Date
    Add-Type -AssemblyName System.IO.Compression

    # 1) 저장 위치를 정한다. 매개변수가 없으면 입력받고, 빈 값이면 취소한다.
    $interactive = $false
    if ([string]::IsNullOrWhiteSpace($OutputPath)) {
        $interactive = $true
        $answer = $null
        try {
            $answer = Read-ZipPathFromUser
        } catch {
            throw '저장 위치를 입력받을 수 없습니다(비대화형 실행). 저장 위치를 첫 번째 매개변수로 지정하세요.'
        }
        if ([string]::IsNullOrWhiteSpace($answer)) {
            Write-Host '입력이 비어 있어 취소했습니다. 압축파일을 만들지 않았습니다.'
            exit 0
        }
        $OutputPath = $answer
    }
    $zipPath = Resolve-ZipPath -Raw $OutputPath
    if ($interactive -and (Test-Path -LiteralPath $zipPath)) {
        $overwrite = Read-Host "이미 파일이 있습니다: ${zipPath}`n덮어쓸까요? (y/N)"
        if ($overwrite -notmatch '^[yY]') {
            Write-Host '취소했습니다. 압축파일을 만들지 않았습니다.'
            exit 0
        }
    }

    # 2) 게임 데이터 원천을 정하고 항목을 모은다.
    if ([string]::IsNullOrWhiteSpace($GameSource)) { $GameSource = Join-Path $RepoRoot 'originals' }
    $GameSource = [System.IO.Path]::GetFullPath($GameSource)
    $game = Get-GameItems -Source $GameSource

    # 3) 실행 파일을 정한다. 직접 지정했으면 그대로 쓰고, 아니면 없음/오래됨을 검사해 필요하면 빌드한다.
    $exeExplicit = -not [string]::IsNullOrWhiteSpace($ExePath)
    if ($exeExplicit) {
        $ExePath = [System.IO.Path]::GetFullPath($ExePath)
    } else {
        $ExePath = $DefaultExePath
    }
    $exeExists = Test-Path -LiteralPath $ExePath -PathType Leaf
    $buildNeeded = $false
    $buildReason = ''
    if (-not $exeExplicit) {
        if (-not $exeExists) {
            $buildNeeded = $true
            $buildReason = '실행 파일이 없습니다'
        } else {
            $newestSource = Get-NewestSourceTime
            $exeTime = (Get-Item -LiteralPath $ExePath).LastWriteTimeUtc
            if ($exeTime -lt $newestSource) {
                $buildNeeded = $true
                $buildReason = ('실행 파일({0:yyyy-MM-dd HH:mm})이 가장 최근 소스({1:yyyy-MM-dd HH:mm})보다 오래됐습니다' -f $exeTime.ToLocalTime(), $newestSource.ToLocalTime())
            }
        }
    }
    if ($buildNeeded) {
        if ($NoBuild) {
            if (-not $exeExists) { throw "${buildReason}. -NoBuild 를 빼고 다시 실행하거나 -ExePath 로 지정하세요." }
            Write-Warning "${buildReason}. -NoBuild 라서 빌드하지 않고 그대로 묶습니다."
        } elseif ($DryRun) {
            Write-Host "[DryRun] ${buildReason}. 실제 실행에서는 NetstormCpp 를 Release 로 빌드합니다."
        } else {
            Write-Host "${buildReason}."
            Invoke-ExeBuild
            $exeExists = Test-Path -LiteralPath $ExePath -PathType Leaf
        }
    }
    if (-not $exeExists -and -not ($DryRun -and $buildNeeded)) {
        throw "실행 파일을 찾을 수 없습니다: ${ExePath}"
    }

    # 4) 전체 항목 목록을 조립한다. 순서는 실행 파일, 런타임 DLL, 배치 파일, 안내문, 라이선스, 게임 데이터이다.
    $items = New-Object System.Collections.Generic.List[object]
    $runtimePlan = @()
    $exeHash = '(빌드 후 계산)'
    if ($exeExists) {
        Add-FileItem -Items $items -Entry ($PackageRootName + '/' + $ExeName) -Source $ExePath
        $exeHash = (Get-FileHash -LiteralPath $ExePath -Algorithm SHA256).Hash
        if (-not $NoRuntime) {
            $runtimePlan = @(Get-RuntimeDllPlan -ExeFile $ExePath)
            # 필요한 런타임 DLL 을 실행 파일 옆에 넣는다. 같은 폴더가 DLL 검색 순서의 첫 번째이다.
            foreach ($dll in $runtimePlan) {
                Add-FileItem -Items $items -Entry ($PackageRootName + '/' + $dll.Name) -Source $dll.Path
            }
        }
    }
    Add-TextItem -Items $items -Entry ($PackageRootName + '/Netstorm.bat') -Text (New-LauncherText -WindowMode $true) -UseBom $false
    Add-TextItem -Items $items -Entry ($PackageRootName + '/Netstorm-SavedMode.bat') -Text (New-LauncherText -WindowMode $false) -UseBom $false
    Add-TextItem -Items $items -Entry ($PackageRootName + '/README.txt') -Text (New-ReadmeText) -UseBom $true
    $licensePath = Join-Path $RepoRoot 'LICENSE'
    if (Test-Path -LiteralPath $licensePath -PathType Leaf) {
        Add-FileItem -Items $items -Entry ($PackageRootName + '/LICENSE.txt') -Source $licensePath
    }

    # BUILD_INFO.txt 는 게임 항목을 포함한 전체 수를 알아야 해서 게임 항목을 더한 뒤 마지막에 만든다.
    $gameBytes = [long]0
    # 게임 데이터 항목을 전체 목록에 옮기면서 크기를 합산한다.
    foreach ($item in $game.Items) {
        $items.Add($item)
        $gameBytes += $item.Length
    }
    $sourceLabel = $GameSource
    if ($GameSource.StartsWith($RepoRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
        $sourceLabel = $GameSource.Substring($RepoRoot.Length).TrimStart('\', '/')
    }
    $infoLines = New-Object System.Collections.Generic.List[string]
    $infoLines.Add('NetStorm Reborn - cpppj distribution')
    $infoLines.Add(('Created      : {0:yyyy-MM-dd HH:mm:ss}' -f $startedAt))
    $infoLines.Add('Source commit: ' + (Get-GitSummary))
    if ($exeExists) {
        $exeInfo = Get-Item -LiteralPath $ExePath
        $infoLines.Add(('Executable   : {0}  {1:N0} bytes  built {2:yyyy-MM-dd HH:mm:ss}' -f $ExeName, $exeInfo.Length, $exeInfo.LastWriteTime))
        $infoLines.Add('Executable SHA256: ' + $exeHash)
    }
    $infoLines.Add(('Game data    : original 10.78 files from "{0}"  ({1} files, {2:N0} bytes, {3} excluded)' -f $sourceLabel, $game.Items.Count, $gameBytes, $game.Skipped.Count))
    # 동봉한 런타임 DLL 마다 버전과 출처를 적는다.
    foreach ($dll in $runtimePlan) {
        $infoLines.Add(('Runtime      : {0}  {1}  ({2})' -f $dll.Name, $dll.Version, $dll.Origin))
    }
    if ($NoRuntime) { $infoLines.Add('Runtime      : not bundled (-NoRuntime)') }
    Add-TextItem -Items $items -Entry ($PackageRootName + '/BUILD_INFO.txt') -Text ((($infoLines -join "`r`n")) + "`r`n") -UseBom $true

    $totalBytes = [long]0
    # 압축 전 전체 크기를 합산한다.
    foreach ($item in $items) { $totalBytes += $item.Length }

    # 5) DryRun 이면 계획만 출력하고 끝낸다.
    Write-Host ''
    Write-Host ('저장 위치      : ' + $zipPath)
    Write-Host ('실행 파일      : ' + $ExePath)
    Write-Host ('게임 데이터 원천: ' + $GameSource)
    # 동봉할 런타임 DLL 을 출처와 함께 보여 준다.
    foreach ($dll in $runtimePlan) {
        Write-Host ('런타임 DLL     : {0}  {1}  ({2})' -f $dll.Name, $dll.Version, $dll.Origin)
    }
    Write-Host ('항목 {0}개, 압축 전 {1}' -f $items.Count, (Format-Size $totalBytes))
    Write-Host ('제외한 파일 {0}개' -f $game.Skipped.Count)
    # 제외한 파일을 모두 보여 주면 길어서 앞 10개만 보여 준다.
    foreach ($skippedName in ($game.Skipped | Select-Object -First 10)) {
        Write-Host ('  - ' + $skippedName)
    }
    if ($DryRun) {
        Write-Host ''
        Write-Host '[DryRun] 압축파일을 만들지 않았습니다.'
        exit 0
    }

    # 6) 임시 파일로 압축하고 검증한 뒤 최종 이름으로 옮긴다. 검증 실패 시 기존 파일을 건드리지 않는다.
    $zipDir = [System.IO.Path]::GetDirectoryName($zipPath)
    if (-not (Test-Path -LiteralPath $zipDir -PathType Container)) {
        New-Item -ItemType Directory -Path $zipDir -Force | Out-Null
    }
    $partialPath = $zipPath + '.partial'
    Write-Host ''
    Write-Host '압축 중입니다...'
    Write-ZipPackage -Items $items -PartialPath $partialPath
    $problems = @(Test-ZipPackage -Items $items -ZipPath $partialPath)
    if ($problems.Count -gt 0) {
        # 검증에서 나온 문제를 모두 보여 준 뒤 실패로 처리한다.
        foreach ($problem in $problems) { Write-Host ('  ! ' + $problem) -ForegroundColor Red }
        throw '만든 압축파일의 검증에 실패했습니다.'
    }
    Move-Item -LiteralPath $partialPath -Destination $zipPath -Force
    $partialPath = $null

    $zipSize = (Get-Item -LiteralPath $zipPath).Length
    $elapsed = (Get-Date) - $startedAt
    Write-Host ''
    Write-Host ('완료: {0}' -f $zipPath) -ForegroundColor Green
    Write-Host ('  압축 파일 {0} (압축 전 {1}), 항목 {2}개, 소요 {3:N0}초' -f (Format-Size $zipSize), (Format-Size $totalBytes), $items.Count, $elapsed.TotalSeconds)
} catch {
    Write-Host ('오류: ' + $_.Exception.Message) -ForegroundColor Red
    $exitCode = 1
} finally {
    # 실패했거나 중단됐을 때 남은 임시 파일을 지운다.
    if ($partialPath -and (Test-Path -LiteralPath $partialPath)) {
        Remove-Item -LiteralPath $partialPath -Force -ErrorAction SilentlyContinue
    }
}
exit $exitCode
