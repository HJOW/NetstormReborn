<#
.SYNOPSIS
    NetStorm 클론 프로젝트 개발 환경 점검 및 설치 스크립트 (Windows 10/11)

.DESCRIPTION
    실행하면 설치 항목 목록을 보여 주고, 사용자가 고른 항목만 점검한다.
    점검 결과 설치되어 있지 않은 항목은 자동으로 설치한 뒤 다시 점검한다.

.PARAMETER ToolsDir
    Ghidra, vcpkg 처럼 winget 으로 설치할 수 없는 도구를 설치할 폴더 (기본값: C:\Tools)

.PARAMETER CheckOnly
    설치하지 않고 점검만 한다.

.PARAMETER All
    선택 화면 없이 모든 항목을 대상으로 한다.

.EXAMPLE
    powershell -ExecutionPolicy Bypass -File .\PREPARE.ps1
    powershell -ExecutionPolicy Bypass -File .\PREPARE.ps1 -CheckOnly
#>
param(
    [string]$ToolsDir = 'C:\Tools',
    [switch]$CheckOnly,
    [switch]$All
)

# 다운로드 진행률 표시를 끄면 Invoke-WebRequest 속도가 크게 빨라진다
$ProgressPreference = 'SilentlyContinue'

# 게임 빌드에 필요한 .NET SDK 주 버전 (프로젝트 대상 프레임워크 net10.0)
$RequiredDotnetMajor = 10

# MonoGame 프로젝트 템플릿 패키지 (버전은 src/ 의 MonoGame.Framework.DesktopGL 참조 버전과 맞춘다)
$MonoGameTemplatePackage = 'MonoGame.Templates.CSharp::3.8.5.1'

# Ghidra 실행에 필요한 최소 JDK 주 버전
$MinJdkMajor = 21

# Python 도구에 필요한 최소 버전
$MinPythonVersion = [version]'3.10'

# 분석 도구에서 사용하는 Python 패키지 (pip 패키지명 → import 모듈명)
$PythonPackages = [ordered]@{
    'pillow'   = 'PIL'       # PNG 내보내기
    'pefile'   = 'pefile'    # exe·DLL 리소스 추출
    'capstone' = 'capstone'  # x86 역어셈블
}

# 설치할 VS Code 확장 ID 목록
$VSCodeExtensions = @(
    'ms-dotnettools.csdevkit',  # C# Dev Kit (솔루션·테스트 탐색기)
    'ms-dotnettools.csharp',    # C# 언어 지원
    'ms-python.python'          # Python (분석 도구)
)

# Visual Studio 설치 정보 조회 도구 경로
$VsWhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'

# MSVC C++ 빌드 도구 구성 요소 ID
$VcToolsComponent = 'Microsoft.VisualStudio.Component.VC.Tools.x86.x64'

# 프로젝트 루트 경로 (git safe.directory 형식에 맞춰 '/' 구분자 사용)
$ProjectRoot = (Resolve-Path $PSScriptRoot).Path -replace '\\', '/'


# ============================================================
# 공통 함수
# ============================================================

# 레지스트리의 시스템·사용자 PATH 를 다시 읽어 현재 세션에 반영한다 (설치 직후 명령을 찾기 위함)
function Update-SessionPath {
    $machine = [Environment]::GetEnvironmentVariable('Path', 'Machine')
    $user    = [Environment]::GetEnvironmentVariable('Path', 'User')
    $env:Path = "$machine;$user"
    # 사용자 환경 변수로 등록된 VCPKG_ROOT 도 세션에 반영
    $vcpkgRoot = [Environment]::GetEnvironmentVariable('VCPKG_ROOT', 'User')
    if ($vcpkgRoot) { $env:VCPKG_ROOT = $vcpkgRoot }
}

# 명령이 PATH 에 있는지 확인한다
function Test-Command([string]$Name) {
    return [bool](Get-Command $Name -ErrorAction SilentlyContinue)
}

# 외부 명령을 실행해 표준 출력·표준 오류를 합친 첫 줄을 돌려준다 (PowerShell 5.1 의 stderr ErrorRecord 문제 회피)
function Get-FirstLine([string]$CommandLine) {
    $out = cmd /c "$CommandLine 2>&1"
    if ($out) { return ([string]($out | Select-Object -First 1)).Trim() }
    return $null
}

# winget 에 해당 패키지가 설치되어 있는지 확인한다
function Test-WingetPackage([string]$Id) {
    winget list --id $Id -e --accept-source-agreements *> $null
    return ($LASTEXITCODE -eq 0)
}

# winget 으로 패키지를 설치한다 (추가 설치 옵션은 Override 로 전달)
function Install-WingetPackage([string]$Id, [string]$Override) {
    $wingetArgs = @('install', '--id', $Id, '-e', '--accept-package-agreements', '--accept-source-agreements')
    # 설치 프로그램 옵션을 직접 지정하는 경우 --silent 대신 --override 사용
    if ($Override) { $wingetArgs += @('--override', $Override) } else { $wingetArgs += '--silent' }
    & winget @wingetArgs
}

# 설치된 .NET SDK 중 지정한 주 버전의 최신 SDK 버전 문자열을 돌려준다 (없으면 $null)
function Get-DotnetSdk([int]$Major) {
    if (-not (Test-Command dotnet)) { return $null }
    # 'dotnet --list-sdks' 출력 줄('10.0.401 [경로]')마다 주 버전을 비교
    foreach ($line in (dotnet --list-sdks)) {
        if ($line -match '^(\d+)\.(\S+)' -and [int]$Matches[1] -eq $Major) { $found = "$($Matches[1]).$($Matches[2])" }
    }
    return $found
}

# MSVC 빌드 도구가 포함된 Visual Studio 2022 설치 경로를 찾는다
function Get-VcToolsInstance {
    if (-not (Test-Path $VsWhere)) { return $null }
    return (& $VsWhere -products * -version '[17.0,18.0)' -requires $VcToolsComponent -property installationPath | Select-Object -First 1)
}

# ToolsDir 에서 Ghidra 설치 폴더를 찾는다
function Get-GhidraDir {
    if (-not (Test-Path $ToolsDir)) { return $null }
    return (Get-ChildItem $ToolsDir -Directory -Filter 'ghidra_*' |
            Where-Object { Test-Path (Join-Path $_.FullName 'ghidraRun.bat') } |
            Sort-Object Name -Descending | Select-Object -First 1)
}

# vcpkg 설치 폴더를 찾는다 (VCPKG_ROOT 우선, 없으면 ToolsDir\vcpkg)
function Get-VcpkgDir {
    # 확인할 후보 경로를 차례로 검사
    foreach ($dir in @($env:VCPKG_ROOT, (Join-Path $ToolsDir 'vcpkg'))) {
        if ($dir -and (Test-Path (Join-Path $dir 'vcpkg.exe'))) { return $dir }
    }
    return $null
}


# ============================================================
# 설치 항목 정의
#   Check   : 설치되어 있으면 상태 문자열, 아니면 $null 을 돌려준다
#   Install : 설치를 수행한다
#   Default : 선택 화면에서 기본으로 선택할지 여부 (필수 항목은 $true)
#   목록 순서가 곧 설치 순서이므로 의존 관계(.NET SDK → MonoGame 템플릿, Python → 패키지, JDK → Ghidra, Git → vcpkg)를 지켜 배치한다
# ============================================================
$Items = @(
    [pscustomobject]@{
        Name = 'Git'; Group = '필수'; Default = $true
        Description = '소스 관리'
        Check   = { if (Test-Command git) { Get-FirstLine 'git --version' } }
        Install = { Install-WingetPackage 'Git.Git' }
    }
    [pscustomobject]@{
        Name = ".NET SDK $RequiredDotnetMajor"; Group = '필수'; Default = $true
        Description = '게임 빌드 (C# + MonoGame, 대상 net10.0)'
        Check   = { $v = Get-DotnetSdk $RequiredDotnetMajor; if ($v) { "SDK $v" } }
        Install = { Install-WingetPackage "Microsoft.DotNet.SDK.$RequiredDotnetMajor" }
    }
    [pscustomobject]@{
        Name = 'MonoGame 템플릿'; Group = '권장'; Default = $true
        Description = "dotnet new 용 MonoGame 프로젝트 템플릿 ($MonoGameTemplatePackage)"
        Check   = {
            if (Test-Command dotnet) {
                # DesktopGL 템플릿(짧은 이름 mgdesktopgl)이 목록에 있는지 확인
                $list = dotnet new list mgdesktopgl 2>$null
                if ($list -match 'mgdesktopgl') { '설치됨 (mgdesktopgl)' }
            }
        }
        Install = {
            if (-not (Test-Command dotnet)) { throw '.NET SDK 가 없습니다. .NET SDK 항목을 먼저 설치하세요.' }
            dotnet new install $MonoGameTemplatePackage
        }
    }
    [pscustomobject]@{
        Name = 'Python 3'; Group = '필수'; Default = $true
        Description = "포맷 분석·추출 도구 ($MinPythonVersion 이상)"
        Check   = {
            if (Test-Command python) {
                $line = Get-FirstLine 'python --version'
                # "Python 3.13.1" 형식에서 버전을 뽑아 최소 버전과 비교
                if ($line -match 'Python (\d+\.\d+)' -and [version]$Matches[1] -ge $MinPythonVersion) { $line }
            }
        }
        Install = { Install-WingetPackage 'Python.Python.3.13' }
    }
    [pscustomobject]@{
        Name = 'Python 패키지'; Group = '필수'; Default = $true
        Description = ($PythonPackages.Keys -join ', ')
        Check   = {
            if (Test-Command python) {
                $missing = @()
                # 각 패키지를 import 해 보고 실패한 것을 모은다
                foreach ($pkg in $PythonPackages.Keys) {
                    python -c "import $($PythonPackages[$pkg])" *> $null
                    if ($LASTEXITCODE -ne 0) { $missing += $pkg }
                }
                if ($missing.Count -eq 0) { '모두 설치됨' }
            }
        }
        Install = {
            if (-not (Test-Command python)) { throw 'Python 이 없습니다. Python 3 항목을 먼저 설치하세요.' }
            python -m pip install --user --upgrade @($PythonPackages.Keys)
        }
    }
    [pscustomobject]@{
        Name = 'VS Build Tools 2022 (C++)'; Group = '선택'; Default = $false
        Description = 'MSVC 컴파일러, Windows SDK (C# 확정으로 현재 불필요)'
        Check   = { $path = Get-VcToolsInstance; if ($path) { $path } }
        Install = {
            $workload = '--add Microsoft.VisualStudio.Workload.VCTools --includeRecommended'
            $existing = $null
            if (Test-Path $VsWhere) {
                $existing = & $VsWhere -products * -version '[17.0,18.0)' -property installationPath | Select-Object -First 1
            }
            if ($existing) {
                # 이미 VS 2022 가 있으면 C++ 워크로드만 추가 (winget 은 '이미 설치됨'으로 건너뛰기 때문)
                $setup = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\setup.exe'
                Start-Process $setup -ArgumentList "modify --installPath `"$existing`" $workload --passive --norestart" -Wait
            }
            else {
                Install-WingetPackage 'Microsoft.VisualStudio.2022.BuildTools' "--wait --passive --norestart $workload"
            }
        }
    }
    [pscustomobject]@{
        Name = 'CMake'; Group = '선택'; Default = $false
        Description = '빌드 시스템 생성기 (C# 확정으로 현재 불필요)'
        Check   = { if (Test-Command cmake) { Get-FirstLine 'cmake --version' } }
        Install = { Install-WingetPackage 'Kitware.CMake' }
    }
    [pscustomobject]@{
        Name = 'Ninja'; Group = '선택'; Default = $false
        Description = '빌드 실행기 (C# 확정으로 현재 불필요)'
        Check   = { if (Test-Command ninja) { 'ninja ' + (Get-FirstLine 'ninja --version') } }
        Install = { Install-WingetPackage 'Ninja-build.Ninja' }
    }
    [pscustomobject]@{
        Name = "JDK $MinJdkMajor+"; Group = '필수'; Default = $true
        Description = 'Ghidra 실행용 Java'
        Check   = {
            if (Test-Command java) {
                $line = Get-FirstLine 'java -version'
                # 'version "25"', 'version "21.0.12"', 'version "1.8.0_xx"' 형식에서 주 버전을 뽑는다
                if ($line -match 'version "(\d+)(?:\.(\d+))?') {
                    $major = [int]$Matches[1]
                    if ($major -eq 1 -and $Matches[2]) { $major = [int]$Matches[2] }
                    if ($major -ge $MinJdkMajor) { $line }
                }
            }
        }
        Install = { Install-WingetPackage 'EclipseAdoptium.Temurin.21.JDK' }
    }
    [pscustomobject]@{
        Name = 'Ghidra'; Group = '필수'; Default = $true
        Description = "Netstorm.exe 정적 분석 ($ToolsDir 에 설치)"
        Check   = { $dir = Get-GhidraDir; if ($dir) { $dir.FullName } }
        Install = {
            # winget 미제공 → GitHub 최신 릴리스 zip 을 받아 압축 해제
            $release = Invoke-RestMethod 'https://api.github.com/repos/NationalSecurityAgency/ghidra/releases/latest'
            $asset   = $release.assets | Where-Object { $_.name -like 'ghidra_*_PUBLIC_*.zip' } | Select-Object -First 1
            if (-not $asset) { throw 'Ghidra 릴리스 파일을 찾지 못했습니다.' }
            if (-not (Test-Path $ToolsDir)) { New-Item -ItemType Directory -Path $ToolsDir | Out-Null }
            $zipPath = Join-Path $env:TEMP $asset.name
            Write-Host "    $($release.tag_name) 다운로드 중 (수백 MB)..."
            Invoke-WebRequest -Uri $asset.browser_download_url -OutFile $zipPath -UseBasicParsing
            Write-Host '    압축 해제 중...'
            # Expand-Archive 는 큰 파일에서 매우 느리므로 .NET ZipFile 사용
            Add-Type -AssemblyName System.IO.Compression.FileSystem
            [System.IO.Compression.ZipFile]::ExtractToDirectory($zipPath, $ToolsDir)
            Remove-Item $zipPath
        }
    }
    [pscustomobject]@{
        Name = 'x64dbg'; Group = '필수'; Default = $true
        Description = '원본 실행 중 동적 분석 (디버거)'
        Check   = { if (Test-WingetPackage 'x64dbg.x64dbg') { 'winget 설치 목록에서 확인' } }
        Install = { Install-WingetPackage 'x64dbg.x64dbg' }
    }
    [pscustomobject]@{
        Name = 'Process Monitor'; Group = '필수'; Default = $true
        Description = '원본의 파일·레지스트리 접근 관찰'
        Check   = { if (Test-WingetPackage 'Microsoft.Sysinternals.ProcessMonitor') { 'winget 설치 목록에서 확인' } }
        Install = { Install-WingetPackage 'Microsoft.Sysinternals.ProcessMonitor' }
    }
    [pscustomobject]@{
        Name = 'git safe.directory 등록'; Group = '필수'; Default = $true
        Description = "'dubious ownership' 오류 해결 (git 전역 설정 변경)"
        Check   = {
            if (Test-Command git) {
                $dirs = git config --global --get-all safe.directory
                # 등록된 경로 중 프로젝트 루트(또는 전체 허용 '*')가 있는지 확인
                foreach ($d in $dirs) { if ($d -eq $ProjectRoot -or $d -eq '*') { $d; break } }
            }
        }
        Install = {
            if (-not (Test-Command git)) { throw 'Git 이 없습니다. Git 항목을 먼저 설치하세요.' }
            git config --global --add safe.directory $ProjectRoot
        }
    }
    [pscustomobject]@{
        Name = 'vcpkg'; Group = '선택'; Default = $false
        Description = "C++ 라이브러리 관리자 ($ToolsDir 에 설치, C# 확정으로 현재 불필요)"
        Check   = { $dir = Get-VcpkgDir; if ($dir) { $dir } }
        Install = {
            if (-not (Test-Command git)) { throw 'Git 이 없습니다. Git 항목을 먼저 설치하세요.' }
            $dir = Join-Path $ToolsDir 'vcpkg'
            # 이미 받아 둔 저장소가 있으면 clone 을 건너뛴다
            if (-not (Test-Path $dir)) { git clone https://github.com/microsoft/vcpkg $dir }
            & (Join-Path $dir 'bootstrap-vcpkg.bat') -disableMetrics
            # CMake 가 vcpkg 를 찾을 수 있도록 사용자 환경 변수 등록
            [Environment]::SetEnvironmentVariable('VCPKG_ROOT', $dir, 'User')
        }
    }
    [pscustomobject]@{
        Name = 'VS Code 확장'; Group = '권장'; Default = $false
        Description = ($VSCodeExtensions -join ', ')
        Check   = {
            if (Test-Command code) {
                $installed = code --list-extensions
                $missing = $VSCodeExtensions | Where-Object { $installed -notcontains $_ }
                if (-not $missing) { '모두 설치됨' }
            }
        }
        Install = {
            if (-not (Test-Command code)) { throw 'VS Code 가 없습니다.' }
            # 확장 목록을 순회하며 설치 (이미 설치된 확장은 VS Code 가 건너뜀)
            foreach ($ext in $VSCodeExtensions) { code --install-extension $ext }
        }
    }
    [pscustomobject]@{
        Name = 'yt-dlp'; Group = '선택'; Default = $false
        Description = '분석용 플레이 영상 내려받기'
        Check   = { if (Test-Command yt-dlp) { 'yt-dlp ' + (Get-FirstLine 'yt-dlp --version') } }
        Install = { Install-WingetPackage 'yt-dlp.yt-dlp' }
    }
    [pscustomobject]@{
        Name = 'FFmpeg'; Group = '선택'; Default = $false
        Description = '영상 프레임 추출'
        Check   = { if (Test-Command ffmpeg) { Get-FirstLine 'ffmpeg -version' } }
        Install = { Install-WingetPackage 'Gyan.FFmpeg' }
    }
)


# ============================================================
# 선택 화면
# ============================================================

# 항목 목록을 보여 주고 사용자가 번호로 선택을 토글하게 한다. 선택된 항목 배열을 돌려준다.
function Select-Items($List) {
    $selected = @($List | ForEach-Object { $_.Default })

    # 사용자가 진행(엔터) 또는 종료(q)를 입력할 때까지 반복
    while ($true) {
        Write-Host ''
        Write-Host '=== 설치 항목 선택 ===' -ForegroundColor Cyan
        # 각 항목을 번호·선택 상태와 함께 출력
        for ($i = 0; $i -lt $List.Count; $i++) {
            $mark = if ($selected[$i]) { '[x]' } else { '[ ]' }
            Write-Host ('{0,3}. {1} ({2}) {3}' -f ($i + 1), $mark, $List[$i].Group, $List[$i].Name) -NoNewline
            Write-Host "  - $($List[$i].Description)" -ForegroundColor DarkGray
        }
        Write-Host ''
        Write-Host '번호를 입력하면 선택/해제됩니다 (여러 개는 공백·쉼표로 구분, 범위는 1-5).'
        $answer = Read-Host 'a=전체 선택, n=전체 해제, r=필수만, 엔터=진행, q=종료'

        switch -Regex ($answer.Trim()) {
            '^$'  { return @(for ($i = 0; $i -lt $List.Count; $i++) { if ($selected[$i]) { $List[$i] } }) }
            '^q$' { return $null }
            '^a$' { for ($i = 0; $i -lt $List.Count; $i++) { $selected[$i] = $true } ; break }
            '^n$' { for ($i = 0; $i -lt $List.Count; $i++) { $selected[$i] = $false } ; break }
            '^r$' { for ($i = 0; $i -lt $List.Count; $i++) { $selected[$i] = ($List[$i].Group -eq '필수') } ; break }
            default {
                # 입력을 토큰 단위로 나눠 번호 또는 범위(예: 3-6)로 해석
                foreach ($token in ($answer -split '[\s,]+' | Where-Object { $_ })) {
                    if ($token -match '^(\d+)-(\d+)$') {
                        $range = [int]$Matches[1]..[int]$Matches[2]
                    }
                    elseif ($token -match '^\d+$') {
                        $range = @([int]$token)
                    }
                    else {
                        Write-Host "  잘못된 입력: $token" -ForegroundColor Yellow
                        continue
                    }
                    # 범위 안의 번호마다 선택 상태를 뒤집는다
                    foreach ($n in $range) {
                        if ($n -ge 1 -and $n -le $List.Count) { $selected[$n - 1] = -not $selected[$n - 1] }
                        else { Write-Host "  범위를 벗어난 번호: $n" -ForegroundColor Yellow }
                    }
                }
            }
        }
    }
}


# ============================================================
# 메인
# ============================================================

Write-Host 'NetStorm 클론 개발 환경 준비' -ForegroundColor Green
if ($CheckOnly) { Write-Host '(점검 전용 모드: 설치하지 않습니다)' -ForegroundColor Yellow }

# winget 은 대부분의 설치에 필요하므로 먼저 확인
if (-not $CheckOnly -and -not (Test-Command winget)) {
    Write-Host 'winget 이 없습니다. Microsoft Store 에서 "앱 설치 관리자(App Installer)"를 설치한 뒤 다시 실행하세요.' -ForegroundColor Red
    exit 1
}

if ($All) { $targets = $Items } else { $targets = Select-Items $Items }
if (-not $targets) {
    Write-Host '선택된 항목이 없어 종료합니다.'
    exit 0
}

$results = @()

# 선택된 항목을 순서대로 점검하고, 없으면 설치 후 다시 점검
foreach ($item in $targets) {
    Write-Host ''
    Write-Host "▶ $($item.Name)" -ForegroundColor Cyan
    $status = & $item.Check

    if ($status) {
        Write-Host "    이미 설치됨: $status" -ForegroundColor Green
        $results += [pscustomobject]@{ 항목 = $item.Name; 결과 = '이미 설치됨'; 상세 = $status }
        continue
    }

    if ($CheckOnly) {
        Write-Host '    설치되어 있지 않음' -ForegroundColor Yellow
        $results += [pscustomobject]@{ 항목 = $item.Name; 결과 = '없음'; 상세 = '' }
        continue
    }

    Write-Host '    설치되어 있지 않음 → 설치를 시작합니다.' -ForegroundColor Yellow
    try {
        & $item.Install
        Update-SessionPath
        $status = & $item.Check
        if ($status) {
            Write-Host "    설치 완료: $status" -ForegroundColor Green
            $results += [pscustomobject]@{ 항목 = $item.Name; 결과 = '설치 완료'; 상세 = $status }
        }
        else {
            Write-Host '    설치 후에도 확인되지 않습니다. 터미널을 다시 열고 -CheckOnly 로 재점검하세요.' -ForegroundColor Red
            $results += [pscustomobject]@{ 항목 = $item.Name; 결과 = '확인 실패'; 상세 = '재시작 후 재점검 필요' }
        }
    }
    catch {
        Write-Host "    설치 실패: $($_.Exception.Message)" -ForegroundColor Red
        $results += [pscustomobject]@{ 항목 = $item.Name; 결과 = '실패'; 상세 = $_.Exception.Message }
    }
}

Write-Host ''
Write-Host '=== 결과 요약 ===' -ForegroundColor Cyan
$results | Format-Table -AutoSize -Wrap

if (-not $CheckOnly) {
    Write-Host 'PATH·환경 변수 변경을 반영하려면 터미널(및 VS Code)을 다시 여세요.'
}
