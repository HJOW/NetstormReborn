<#
.SYNOPSIS
    dotnetpj(C# + MonoGame 클론) 배포판 압축파일(zip)을 만든다.

.DESCRIPTION
    게임 프로젝트(Netstorm.Game)를 Release / win-x64 로 `dotnet publish` 한 결과에 안내문, 빌드 정보,
    라이선스 고지를 더해 하나의 zip 으로 묶는다. 받은 사람이 압축을 풀고 NetstormClone.exe 를 실행하면
    되도록 구성한다. 게임 데이터(game-data)와 한국어 글꼴(fonts)은 게시 결과에 이미 포함되어 있다.
    (2026-10-23 신규. AGENTS.md 의 "배포판 압축파일 생성용 스크립트" 요구사항을 따른다.)

    - 저장 위치(파일명 포함)를 첫 번째 매개변수로 주면 그 위치를 쓴다.
    - 매개변수를 생략하면 실행 중에 입력을 받는다. 빈 값을 입력하면 취소로 보고 종료한다(종료 코드 0).
    - 폴더만 입력하면 그 폴더 안에 기본 파일명으로 만든다. 확장자를 생략하면 .zip 을 붙인다.
    - 기본은 프레임워크 종속 배포다. AGENTS.md 의 가정대로 받는 사람이 .NET 10 런타임을 미리 설치한다.
      -SelfContained 를 주면 .NET 런타임까지 포함한다(용량이 늘어난다).
    - 게시는 임시 폴더에서 하고 끝나면 지운다. 저장소 안에 dist 같은 산출물을 남기지 않는다.
    - 디버그 기호(*.pdb)는 넣지 않는다.

.PARAMETER OutputPath
    배포판 압축파일을 저장할 위치(파일명 포함). 생략하면 실행 중에 입력받는다.

.PARAMETER PublishDir
    이미 `dotnet publish` 한 폴더를 그대로 묶는다. 지정하면 게시를 건너뛴다.
    이 폴더는 지우거나 바꾸지 않는다.

.PARAMETER SelfContained
    .NET 런타임을 포함해 게시한다. 받는 PC 에 .NET 을 설치하지 않아도 실행되지만 용량이 커진다.
    -PublishDir 와 함께 쓰면 안내문과 검사 기준만 바뀐다(게시 결과는 그대로).

.PARAMETER DryRun
    게시도 압축도 하지 않고 계획만 보여 준다. -PublishDir 를 함께 주면 실제 포함 목록 요약까지 보여 준다.

.EXAMPLE
    powershell -NoProfile -ExecutionPolicy Bypass -File dotnetpj\package-dotnetpj.ps1 D:\dist\NetstormReborn-dotnetpj.zip

.EXAMPLE
    powershell -NoProfile -ExecutionPolicy Bypass -File dotnetpj\package-dotnetpj.ps1 -SelfContained D:\dist
#>
[CmdletBinding()]
param(
    # 배포판 압축파일 저장 위치(파일명 포함)다. 비어 있으면 실행 중에 입력받는다. (2026-10-23 AGENTS.md 요구사항으로 추가)
    [Parameter(Position = 0)]
    [string] $OutputPath,

    # 이미 게시한 폴더다. 비어 있지 않으면 dotnet publish 를 건너뛰고 이 폴더를 묶는다. (2026-10-23 미리 만든 게시 결과를 쓰게 하려고 추가)
    [string] $PublishDir,

    # 켜면 .NET 런타임을 포함해 게시한다. 기본은 프레임워크 종속이다. (2026-10-23 런타임 설치가 어려운 환경용으로 추가)
    [switch] $SelfContained,

    # 켜면 게시와 압축 없이 계획만 출력한다. (2026-10-23 실행 전 점검용으로 추가)
    [switch] $DryRun
)

# 정의되지 않은 변수 사용을 오류로 잡는다. (2026-10-23 오타로 인한 조용한 실패를 막으려고 추가)
Set-StrictMode -Version 2
# 모든 오류에서 즉시 멈춘다. 불완전한 배포판이 만들어지지 않게 한다. (2026-10-23 신규)
$ErrorActionPreference = 'Stop'

# ----------------------------------------------------------------------------------------------
# 상수
# ----------------------------------------------------------------------------------------------

# 이 스크립트가 있는 dotnetpj 폴더다. 게임 프로젝트를 여기서 상대적으로 찾는다. (2026-10-23 신규)
$DotnetRoot = $PSScriptRoot

# 저장소 최상위 폴더다. LICENSE 와 git 정보를 여기서 찾는다. (2026-10-23 신규)
$RepoRoot = Split-Path -Parent $DotnetRoot

# 게시할 게임 프로젝트 파일이다. 어셈블리 이름은 NetstormClone, 출력은 WinExe 이다. (2026-10-23 신규)
$GameProject = Join-Path $DotnetRoot 'src\Netstorm.Game\Netstorm.Game.csproj'

# 게시 대상 런타임 식별자다. Windows 10/11 x64 만 지원한다(리눅스는 후순위, AGENTS.md). (2026-10-23 신규)
$RuntimeId = 'win-x64'

# 압축파일 안에서 모든 항목을 감싸는 최상위 폴더 이름이다. 풀 때 현재 폴더가 어지러워지지 않게 한다. (2026-10-23 신규)
$PackageRootName = 'NetstormReborn-dotnetpj'

# 게임 실행 파일 이름이다. 게시 결과 최상위에 있다. (2026-10-23 신규)
$ExeName = 'NetstormClone.exe'

# 폴더만 입력했을 때 쓰는 기본 압축파일 이름이다. 날짜와 배포 방식을 붙여 이전 배포판을 덮어쓰지 않게 한다. (2026-10-23 신규)
$DefaultZipName = 'NetstormReborn-dotnetpj-{0}.zip' -f (Get-Date -Format 'yyyyMMdd')

# 임시 게시 폴더 이름의 접두어다. 이 접두어로 시작하는 폴더만 스크립트가 지운다. (2026-10-23 사용자 폴더를 잘못 지우지 않게 하려고 추가)
$StagingPrefix = 'NetstormReborn-dotnetpj-'

# 압축파일에서 제외할 파일 이름 패턴이다. 디버그 기호는 배포판에 필요하지 않고 용량만 늘린다.
# Thumbs.db, desktop.ini 는 탐색기가 만든 파일이다. (2026-10-23 신규)
$ExcludedNamePatterns = @('*.pdb', 'Thumbs.db', 'desktop.ini')

# 게시 결과에 반드시 있어야 하는 파일(게시 폴더 기준 상대 경로, 슬래시 구분)이다. 하나라도 없으면 불완전한 게시로 보고 멈춘다.
# game-data 세 파일은 Netstorm.Game.csproj 의 ValidateGameData 대상과 같고, SDL2/openal 은 MonoGame 이 쓰는 네이티브 라이브러리이다.
# (2026-10-23 신규)
$RequiredPublishEntries = @(
    'NetstormClone.exe', 'NetstormClone.dll', 'NetstormClone.deps.json', 'NetstormClone.runtimeconfig.json',
    'SDL2.dll', 'openal.dll',
    'game-data/netstorm.tarc', 'game-data/d/_shapes.shp', 'game-data/d/GIFCLOUD.COL',
    'fonts/D2Coding-Ver1.3.2-20180524-all.ttc')

# 자체 포함 게시일 때 추가로 필요한 파일이다. .NET 런타임의 핵심 라이브러리가 들어 있는지 본다. (2026-10-23 신규)
$RequiredSelfContainedEntries = @('coreclr.dll', 'hostfxr.dll')

# 글꼴 폴더에서 찾아 함께 넣을 라이선스 전문 파일 후보 이름이다. 저장소에 전문이 추가되면 자동으로 포함된다.
# (2026-10-23 SIL OFL 은 글꼴을 재배포할 때 전문을 함께 주어야 하는데 현재 저장소에 전문 파일이 없어 후보만 열어 둠)
$FontLicenseCandidates = @('OFL.txt', 'OFL-1.1.txt', 'D2Coding-LICENSE.txt', 'LICENSE.txt')

# D2Coding 글꼴의 고지문이다. 글꼴 파일 내부 name 표(저작권 0번, 라이선스 설명 13번, 라이선스 주소 14번)에서 그대로 옮겼다.
# (2026-10-23 글꼴 파일 D2Coding-Ver1.3.2-20180524-all.ttc 를 직접 읽어 확인한 문구)
$FontNoticeLines = @(
    'D2Coding  (fonts/D2Coding-Ver1.3.2-20180524-all.ttc, Version 1.3.2; Build 20180524)',
    '  Copyright (c) 2015-2016 NHN Corporation. All rights reserved. Font designed by FONTRIX Inc.',
    '  This Font Software is licensed under the SIL Open Font License, Version 1.1.',
    '  License URL embedded in the font: http://dev.naver.com/wiki/nanumfont/index.php/OpenFontLicense',
    '  SIL OFL 1.1 official site: https://openfontlicense.org')

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
    Write-Host '  예: D:\dist\NetstormReborn-dotnetpj.zip   (폴더만 입력하면 기본 파일명을 사용합니다)'
    Write-Host '  아무것도 입력하지 않고 Enter 를 누르면 취소합니다.'
    return (Read-Host '저장 위치')
}

# dotnet 실행 파일 경로를 찾는다. PATH 를 먼저 보고, 없으면 기본 설치 위치를 본다. 못 찾으면 $null 이다.
# (2026-10-23 신규. 빌드 도구가 PATH 에 없는 환경에서도 안내 메시지를 분명히 하려고 추가)
function Find-DotnetTool {
    $command = Get-Command dotnet -ErrorAction SilentlyContinue
    if ($command) { return $command.Source }
    $default = Join-Path $env:ProgramFiles 'dotnet\dotnet.exe'
    if (Test-Path -LiteralPath $default) { return $default }
    return $null
}

# 게임 프로젝트를 Release / win-x64 로 게시한다. Destination 은 출력 폴더이다.
# SelfContained 가 참이면 .NET 런타임까지 포함한다. 복원에 NuGet 연결이 필요할 수 있어 실패하면 그 가능성을 안내한다.
# dotnet 이 RID 별 중간 폴더(bin\Release\net10.0\win-x64)를 쓰므로 개발용 빌드 출력과 섞이지 않는다.
# (2026-10-23 신규. README 의 배포 명령을 스크립트로 옮김)
function Invoke-GamePublish {
    param([string] $Destination, [bool] $EmbedRuntime)
    $dotnet = Find-DotnetTool
    if (-not $dotnet) {
        throw 'dotnet 을 찾을 수 없습니다. .NET 10 SDK 를 설치하거나, 이미 게시한 폴더를 -PublishDir 로 지정하세요.'
    }
    $selfContainedText = if ($EmbedRuntime) { 'true' } else { 'false' }
    Write-Host ("dotnet publish: Release / ${RuntimeId} / self-contained=${selfContainedText}")
    & $dotnet publish $GameProject -c Release -r $RuntimeId --self-contained $selfContainedText -o $Destination --nologo -v minimal
    if ($LASTEXITCODE -ne 0) {
        throw "dotnet publish 에 실패했습니다(종료 코드 ${LASTEXITCODE}). NuGet 패키지를 처음 받는 경우 인터넷 연결이 필요합니다."
    }
}

# 게시 폴더 기준 상대 경로(슬래시 구분)의 파일 이름이 제외 패턴에 맞는지 판정한다. (2026-10-23 신규)
function Test-PublishExcluded {
    param([string] $Relative)
    $leaf = $Relative.Substring($Relative.LastIndexOf('/') + 1)
    # 파일 이름이 제외 패턴 중 하나에 맞는지 확인한다.
    foreach ($pattern in $ExcludedNamePatterns) {
        if ($leaf -like $pattern) { return $true }
    }
    return $false
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

# 목록에 스크립트가 직접 만든 텍스트 항목을 추가한다. UTF-8 BOM 을 붙여 메모장 등에서 한글이 깨지지 않게 한다.
# (2026-10-23 신규)
function Add-TextItem {
    param($Items, [string] $Entry, [string] $Text)
    $encoding = New-Object System.Text.UTF8Encoding($true)
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

# 게시 폴더를 검사하고 압축할 항목 목록을 만든다. 결과는 Items(항목)와 Skipped(제외한 상대 경로)를 가진 객체이다.
# 필수 파일이 없으면 오류로 멈춘다. 항목은 경로 순(서수 비교)으로 정렬해 같은 입력이면 같은 순서로 압축한다.
# (2026-10-23 신규)
function Get-PublishItems {
    param([string] $Folder, [bool] $EmbedRuntime)
    # 필수 파일이 모두 있는지 먼저 확인한다. 자체 포함이면 런타임 핵심 파일도 요구한다.
    $required = New-Object System.Collections.Generic.List[string]
    $required.AddRange([string[]]$RequiredPublishEntries)
    if ($EmbedRuntime) { $required.AddRange([string[]]$RequiredSelfContainedEntries) }
    # 필수 항목마다 게시 폴더에 실제 파일이 있는지 확인한다.
    foreach ($entry in $required) {
        $path = Join-Path $Folder ($entry -replace '/', '\')
        if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
            throw "게시 결과에 필수 파일이 없습니다: ${entry} (폴더: ${Folder})"
        }
    }
    $items = New-Object System.Collections.Generic.List[object]
    $skipped = New-Object System.Collections.Generic.List[string]
    $files = New-Object System.Collections.Generic.List[string]
    # 게시 폴더 아래 모든 파일의 전체 경로를 모은다.
    foreach ($file in [System.IO.Directory]::EnumerateFiles($Folder, '*', [System.IO.SearchOption]::AllDirectories)) {
        $files.Add($file)
    }
    $files.Sort([System.StringComparer]::Ordinal)
    $prefix = $Folder.TrimEnd('\', '/')
    # 파일마다 상대 경로를 만들고 제외 대상이면 건너뛴다.
    foreach ($file in $files) {
        $relative = $file.Substring($prefix.Length).TrimStart('\', '/').Replace('\', '/')
        if (Test-PublishExcluded -Relative $relative) {
            $skipped.Add($relative)
            continue
        }
        Add-FileItem -Items $items -Entry ($PackageRootName + '/' + $relative) -Source $file
    }
    return [pscustomobject]@{ Items = $items; Skipped = $skipped }
}

# NuGet 전역 패키지 폴더 경로를 돌려준다. NUGET_PACKAGES 환경 변수를 먼저 보고, 없으면 사용자 폴더의 기본 위치를 쓴다.
# (2026-10-23 신규. 라이선스 정보를 패키지 메타데이터에서 읽기 위해 추가)
function Get-NuGetPackagesRoot {
    if ($env:NUGET_PACKAGES) { return $env:NUGET_PACKAGES }
    return (Join-Path $env:USERPROFILE '.nuget\packages')
}

# 게시 결과의 deps.json 에 적힌 NuGet 패키지마다 버전과 라이선스를 NuGet 캐시의 nuspec 에서 읽어 고지문을 만든다.
# 라이선스를 파일로 제공하는 패키지는 그 파일을 licenses 폴더에 함께 넣도록 FileItems 에 모은다.
# 메타데이터를 찾지 못하면 그 사실을 적고 계속한다(배포판 생성을 막지 않는다). 결과는 Text(문자열)와 FileItems(Entry/Source 쌍)를 가진 객체이다.
# (2026-10-23 신규. 라이선스 이름을 기억에 의존하지 않고 실제 패키지 정보에서 가져오려고 추가)
function Get-ThirdPartyNotices {
    param([string] $DepsJsonPath)
    $lines = New-Object System.Collections.Generic.List[string]
    $fileItems = New-Object System.Collections.Generic.List[object]
    $lines.Add('Third-party components bundled with NetStorm Reborn (dotnetpj)')
    $lines.Add('================================================================')
    $lines.Add('')
    $lines.Add('NetStorm Reborn itself is distributed under the MIT License (see LICENSE.txt).')
    $lines.Add('')
    $lines.Add('[Font]')
    # 글꼴 고지문을 그대로 옮긴다.
    foreach ($line in $FontNoticeLines) { $lines.Add($line) }
    $lines.Add('')
    $lines.Add('[NuGet packages]')

    $packageRoot = Get-NuGetPackagesRoot
    $deps = Get-Content -LiteralPath $DepsJsonPath -Raw -Encoding UTF8 | ConvertFrom-Json
    # deps.json 의 라이브러리 항목 중 패키지(또는 자체 포함 런타임 묶음)만 골라 정보를 찾는다.
    foreach ($library in $deps.libraries.PSObject.Properties) {
        $kind = $library.Value.type
        if ($kind -ne 'package' -and $kind -ne 'runtimepack') { continue }
        $slash = $library.Name.LastIndexOf('/')
        $name = $library.Name.Substring(0, $slash) -replace '^runtimepack\.', ''
        $version = $library.Name.Substring($slash + 1)
        $lines.Add('')
        $lines.Add("${name} ${version}")
        $folder = Join-Path $packageRoot (Join-Path $name.ToLowerInvariant() $version.ToLowerInvariant())
        $nuspecFile = $null
        if (Test-Path -LiteralPath $folder -PathType Container) {
            $nuspecFile = Get-ChildItem -LiteralPath $folder -Filter '*.nuspec' -File | Select-Object -First 1
        }
        if (-not $nuspecFile) {
            $lines.Add('  License: (package metadata not found in the NuGet cache; see the package page on nuget.org)')
            continue
        }
        $xml = New-Object System.Xml.XmlDocument
        $xml.Load($nuspecFile.FullName)
        $licenseNode = $xml.GetElementsByTagName('license') | Select-Object -First 1
        $copyrightNode = $xml.GetElementsByTagName('copyright') | Select-Object -First 1
        if ($copyrightNode -and $copyrightNode.InnerText) { $lines.Add('  Copyright: ' + $copyrightNode.InnerText.Trim()) }
        if (-not $licenseNode) {
            $lines.Add('  License: (not declared in the package metadata)')
        } elseif ($licenseNode.GetAttribute('type') -eq 'file') {
            # 라이선스를 파일로 제공하는 패키지는 그 파일을 licenses 폴더에 복사한다.
            $source = Join-Path $folder ($licenseNode.InnerText.Trim() -replace '/', '\')
            if (Test-Path -LiteralPath $source -PathType Leaf) {
                $leaf = [System.IO.Path]::GetFileName($source)
                if (-not [System.IO.Path]::GetExtension($leaf)) { $leaf = $leaf + '.txt' }
                $entry = $PackageRootName + '/licenses/' + $name + '-' + $leaf
                $fileItems.Add([pscustomobject]@{ Entry = $entry; Source = $source })
                $lines.Add('  License: see licenses/' + $name + '-' + $leaf)
            } else {
                $lines.Add('  License: file declared but not found in the NuGet cache: ' + $licenseNode.InnerText.Trim())
            }
        } else {
            $expression = $licenseNode.InnerText.Trim()
            # 공백이나 괄호가 있는 복합 식은 주소 형식을 확인하지 못했으므로 식만 적고, 단순한 식에만 주소를 붙인다.
            if ($expression -match '^[A-Za-z0-9.+-]+$') {
                $lines.Add('  License: ' + $expression + '  (SPDX expression; https://licenses.nuget.org/' + $expression + ')')
            } else {
                $lines.Add('  License: ' + $expression + '  (SPDX expression)')
            }
        }
    }
    return [pscustomobject]@{ Text = ((($lines -join "`r`n")) + "`r`n"); FileItems = $fileItems }
}

# 저장소의 Git 정보를 한 줄로 돌려준다(커밋 해시, 배포에 영향을 주는 폴더의 미커밋 변경 여부). git 이 없으면 그 사실을 적는다.
# 배포판이 어느 소스에서 나왔는지 BUILD_INFO.txt 에 남기려는 용도이다. (2026-10-23 신규)
function Get-GitSummary {
    $git = Get-Command git -ErrorAction SilentlyContinue
    if (-not $git) { return '(git 을 찾을 수 없음)' }
    # git 이 stderr 로 경고를 내도 스크립트가 멈추지 않게 이 블록에서만 오류 처리를 완화한다.
    $previous = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    try {
        $head = (& git -C $RepoRoot rev-parse --short HEAD 2>$null)
        $dirty = (& git -C $RepoRoot status --porcelain -- dotnetpj assets fonts 2>$null)
        if (-not $head) { return '(git 저장소 정보를 읽지 못함)' }
        if ($dirty) { return ('{0} (dotnetpj/assets/fonts 에 커밋되지 않은 변경 있음)' -f $head) }
        return [string]$head
    } finally {
        $ErrorActionPreference = $previous
    }
}

# 압축파일 최상위에 넣는 안내문(README.txt) 본문을 돌려준다. 한국어와 영어를 함께 적는다.
# EmbedRuntime 에 따라 .NET 런타임 설치 안내가 달라진다. (2026-10-23 신규)
function New-ReadmeText {
    param([bool] $EmbedRuntime)
    $runtimeKo = if ($EmbedRuntime) {
        '- .NET 런타임이 포함되어 있어 따로 설치하지 않아도 됩니다.'
    } else {
        '- .NET 10 런타임(x64)을 미리 설치해야 합니다. 없으면 실행할 때 설치 안내가 표시됩니다.  https://dotnet.microsoft.com/download/dotnet/10.0'
    }
    $runtimeEn = if ($EmbedRuntime) {
        '3. The .NET runtime is bundled; nothing else needs to be installed.'
    } else {
        '3. Install the .NET 10 Runtime (x64) beforehand: https://dotnet.microsoft.com/download/dotnet/10.0'
    }
    $lines = @(
        'NetStorm Reborn - dotnetpj (C# + MonoGame clone) / NetStorm Reborn - dotnetpj (C# + MonoGame 클론)',
        '==================================================================================================',
        '',
        '[실행 방법]',
        '1. 압축을 풀고 NetstormClone.exe 를 실행합니다.',
        '2. 시작 옵션은 명령 프롬프트나 바로가기의 대상 뒤에 붙입니다.',
        '     NetstormClone.exe --language korean     한국어 (english 로 바꾸면 영어)',
        '     NetstormClone.exe --windowed            이번 실행만 창 모드',
        '     NetstormClone.exe --fullscreen          이번 실행만 전체화면',
        '     NetstormClone.exe --window 1280x960     창 크기 지정',
        '',
        '[필요 환경]',
        '- Windows 10 / 11 (64비트)',
        $runtimeKo,
        '',
        '[설정]',
        '- 화면·소리 설정은 %APPDATA%\NetstormReborn\settings.json 에 저장되며, 설정을 이 폴더에 쓰지 않습니다.',
        '  설정을 초기화하려면 그 파일을 지우세요.',
        '',
        '[폴더 구성]',
        '  NetstormClone.exe     게임 실행 파일',
        '  *.dll, *.json         게임 라이브러리와 실행 설정',
        '  SDL2.dll, openal.dll  MonoGame 이 쓰는 창/소리 라이브러리',
        '  game-data\            게임 데이터 (원본 Netstorm 10.78에서 옮긴 그래픽, 지도, 미션, 소리)',
        '  fonts\                한국어 글꼴 D2Coding',
        '  licenses\             제3자 구성 요소 라이선스',
        '  BUILD_INFO.txt        이 배포판을 만든 소스와 실행 파일 정보',
        '  LICENSE.txt           라이선스',
        '',
        '[알림]',
        '- 개발 중인 클론이라 원본의 모든 기능이 구현된 것은 아닙니다. 멀티플레이는 포함되지 않습니다.',
        '',
        '[How to run]',
        '1. Extract the archive and run NetstormClone.exe.',
        '2. Options (append to the command line): --language korean|english, --windowed, --fullscreen, --window 1280x960.',
        $runtimeEn,
        '4. Screen/sound settings are stored in %APPDATA%\NetstormReborn\settings.json, not in this folder. Delete that file to reset.',
        '5. Requires Windows 10/11 x64. This is a work-in-progress clone; multiplayer is not included.')
    return (($lines -join "`r`n") + "`r`n")
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
            # 디버그 기호나 최상위 폴더 밖의 항목이 섞였는지 이름으로 확인한다.
            foreach ($name in $actual.Keys) {
                if ($name -like '*.pdb') { $problems.Add('들어가면 안 되는 파일: ' + $name) }
                if (-not $name.StartsWith($PackageRootName + '/')) { $problems.Add('최상위 폴더 밖의 항목: ' + $name) }
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
# 스크립트가 직접 만든 임시 게시 폴더다. 끝나면 지운다. -PublishDir 로 받은 폴더는 여기에 넣지 않는다. (2026-10-23 신규)
$stagingDir = $null

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

    # 2) 게시 폴더를 정한다. 지정한 폴더가 있으면 그대로 쓰고, 아니면 임시 폴더에 게시한다. DryRun 은 게시하지 않는다.
    $publishFolder = $null
    $publishedByScript = $false
    if (-not [string]::IsNullOrWhiteSpace($PublishDir)) {
        $publishFolder = [System.IO.Path]::GetFullPath($PublishDir)
        if (-not (Test-Path -LiteralPath $publishFolder -PathType Container)) {
            throw "게시 폴더를 찾을 수 없습니다: ${publishFolder}"
        }
    } elseif (-not $DryRun) {
        $stagingDir = Join-Path ([System.IO.Path]::GetTempPath()) ($StagingPrefix + [System.Guid]::NewGuid().ToString('N'))
        Invoke-GamePublish -Destination $stagingDir -EmbedRuntime ([bool]$SelfContained)
        $publishFolder = $stagingDir
        $publishedByScript = $true
    }

    # 3) DryRun 이고 게시 폴더가 없으면 여기서 계획만 보여 주고 끝낸다.
    Write-Host ''
    Write-Host ('저장 위치      : ' + $zipPath)
    Write-Host ('배포 방식      : ' + $(if ($SelfContained) { '자체 포함(.NET 런타임 포함)' } else { '프레임워크 종속(.NET 10 런타임 필요)' }))
    Write-Host ('대상           : Release / ' + $RuntimeId)
    if ($null -eq $publishFolder) {
        Write-Host ('게시 프로젝트  : ' + $GameProject)
        Write-Host '[DryRun] 게시와 압축을 하지 않았습니다. 실제 실행에서는 임시 폴더에 dotnet publish 한 뒤 묶습니다.'
        exit 0
    }

    # 4) 게시 결과를 검사하고 항목 목록을 조립한다.
    $publish = Get-PublishItems -Folder $publishFolder -EmbedRuntime ([bool]$SelfContained)
    $items = New-Object System.Collections.Generic.List[object]
    $publishBytes = [long]0
    # 게시 결과 항목을 전체 목록에 옮기면서 크기를 합산한다.
    foreach ($item in $publish.Items) {
        $items.Add($item)
        $publishBytes += $item.Length
    }

    # 제3자 고지문과 라이선스 파일을 만든다. 글꼴 라이선스 전문이 저장소에 있으면 함께 넣는다.
    $notices = Get-ThirdPartyNotices -DepsJsonPath (Join-Path $publishFolder 'NetstormClone.deps.json')
    Add-TextItem -Items $items -Entry ($PackageRootName + '/licenses/THIRD_PARTY_NOTICES.txt') -Text $notices.Text
    # NuGet 패키지가 파일로 제공한 라이선스를 licenses 폴더에 넣는다.
    foreach ($licenseFile in $notices.FileItems) {
        Add-FileItem -Items $items -Entry $licenseFile.Entry -Source $licenseFile.Source
    }
    $fontLicenseFound = $false
    # 글꼴 폴더에 라이선스 전문 후보 파일이 있으면 찾아서 넣는다.
    foreach ($candidate in $FontLicenseCandidates) {
        $candidatePath = Join-Path $RepoRoot (Join-Path 'fonts' $candidate)
        if (Test-Path -LiteralPath $candidatePath -PathType Leaf) {
            Add-FileItem -Items $items -Entry ($PackageRootName + '/licenses/D2Coding-' + $candidate) -Source $candidatePath
            $fontLicenseFound = $true
            break
        }
    }
    if (-not $fontLicenseFound) {
        Write-Warning 'fonts 폴더에 SIL OFL 라이선스 전문 파일(OFL.txt 등)이 없어 고지문(THIRD_PARTY_NOTICES.txt)만 넣습니다.'
    }

    Add-TextItem -Items $items -Entry ($PackageRootName + '/README.txt') -Text (New-ReadmeText -EmbedRuntime ([bool]$SelfContained))
    $licensePath = Join-Path $RepoRoot 'LICENSE'
    if (Test-Path -LiteralPath $licensePath -PathType Leaf) {
        Add-FileItem -Items $items -Entry ($PackageRootName + '/LICENSE.txt') -Source $licensePath
    }

    # BUILD_INFO.txt 는 게시 결과의 파일 정보를 알아야 해서 항목을 모은 뒤 마지막에 만든다.
    $exePath = Join-Path $publishFolder $ExeName
    $dllPath = Join-Path $publishFolder 'NetstormClone.dll'
    $exeInfo = Get-Item -LiteralPath $exePath
    $dllInfo = Get-Item -LiteralPath $dllPath
    $sdkText = '(알 수 없음)'
    $dotnetTool = Find-DotnetTool
    if ($dotnetTool) { $sdkText = ((& $dotnetTool --version) | Select-Object -First 1) }
    $gameDataCount = 0
    # 게시 결과 중 game-data 아래 파일 수를 센다.
    foreach ($item in $publish.Items) {
        if ($item.Entry.StartsWith($PackageRootName + '/game-data/')) { $gameDataCount++ }
    }
    $infoLines = New-Object System.Collections.Generic.List[string]
    $infoLines.Add('NetStorm Reborn - dotnetpj distribution')
    $infoLines.Add(('Created      : {0:yyyy-MM-dd HH:mm:ss}' -f $startedAt))
    $infoLines.Add('Source commit: ' + (Get-GitSummary))
    $infoLines.Add('Target       : Release / ' + $RuntimeId + $(if ($SelfContained) { ' / self-contained' } else { ' / framework-dependent (.NET 10 runtime required)' }))
    $infoLines.Add('.NET SDK     : ' + $sdkText)
    $infoLines.Add(('Executable   : {0}  {1:N0} bytes  built {2:yyyy-MM-dd HH:mm:ss}' -f $ExeName, $exeInfo.Length, $exeInfo.LastWriteTime))
    $infoLines.Add('Executable SHA256: ' + (Get-FileHash -LiteralPath $exePath -Algorithm SHA256).Hash)
    $infoLines.Add(('Game library : NetstormClone.dll  version {0}  SHA256 {1}' -f $dllInfo.VersionInfo.FileVersion, (Get-FileHash -LiteralPath $dllPath -Algorithm SHA256).Hash))
    $infoLines.Add(('Game data    : {0} files from assets/game-data (clone data source)' -f $gameDataCount))
    $infoLines.Add(('Published    : {0} files, {1:N0} bytes, {2} excluded (debug symbols)' -f $publish.Items.Count, $publishBytes, $publish.Skipped.Count))
    $infoLines.Add($(if ($publishedByScript) { 'Publish dir  : generated by this script (temporary)' } else { 'Publish dir  : supplied by -PublishDir' }))
    Add-TextItem -Items $items -Entry ($PackageRootName + '/BUILD_INFO.txt') -Text (($infoLines -join "`r`n") + "`r`n")

    $totalBytes = [long]0
    # 압축 전 전체 크기를 합산한다.
    foreach ($item in $items) { $totalBytes += $item.Length }

    # 5) 계획을 출력한다. DryRun(-PublishDir 지정) 이면 여기서 끝낸다.
    Write-Host ('게시 폴더      : ' + $publishFolder + $(if ($publishedByScript) { ' (임시)' } else { '' }))
    Write-Host ('항목 {0}개, 압축 전 {1}' -f $items.Count, (Format-Size $totalBytes))
    Write-Host ('제외한 파일 {0}개 (디버그 기호)' -f $publish.Skipped.Count)
    # 제외한 파일을 모두 보여 주면 길어서 앞 10개만 보여 준다.
    foreach ($skippedName in ($publish.Skipped | Select-Object -First 10)) {
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
    # 실패했거나 중단됐을 때 남은 임시 압축파일을 지운다.
    if ($partialPath -and (Test-Path -LiteralPath $partialPath)) {
        Remove-Item -LiteralPath $partialPath -Force -ErrorAction SilentlyContinue
    }
    # 스크립트가 만든 임시 게시 폴더만 지운다. 접두어로 시작하는 폴더인지 한 번 더 확인한다.
    if ($stagingDir -and (Test-Path -LiteralPath $stagingDir) -and (Split-Path -Leaf $stagingDir).StartsWith($StagingPrefix)) {
        Remove-Item -LiteralPath $stagingDir -Recurse -Force -ErrorAction SilentlyContinue
    }
}
exit $exitCode
