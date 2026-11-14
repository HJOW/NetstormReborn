# 현재 C++ 정적 라이브러리의 헤드리스 모듈 관측을 C# 독립 대조 자료로 저장한다.
# 사용: PowerShell에서 이 파일을 실행하고 필요하면 -OutputPath로 TSV 저장 위치를 지정한다.
param([string]$OutputPath = '')

$ErrorActionPreference = 'Stop'
# 저장소·진단 빌드·기본 관측 자료의 절대 위치다.
$wireRepoRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$wireBuildRoot = Join-Path $wireRepoRoot 'extracted/dotnet-squid-wire-probe'
if ([string]::IsNullOrWhiteSpace($OutputPath)) {
    $OutputPath = Join-Path $wireRepoRoot 'dotnetpj/tests/Netstorm.Core.Tests/Fixtures/squid-wire-cpp.tsv'
}
$wireOutputPath = [IO.Path]::GetFullPath($OutputPath)
# 실제 원본 구현을 제공하는 Release 정적 라이브러리와 관측 프로그램 소스다.
$wireLibrary = Join-Path $wireRepoRoot 'cpppj/build/lib/Release/netstorm.lib'
$wireQuickJsLibrary = Join-Path $wireRepoRoot 'cpppj/build/lib/Release/NetstormQuickJs.lib'
$wireProbeSource = Join-Path $PSScriptRoot 'probe_squid_wire.cpp'
if (!(Test-Path -LiteralPath $wireLibrary)) { throw '먼저 cpppj의 Release netstorm 대상을 빌드해야 합니다.' }
# 설치된 C++ 도구 집합은 vswhere로 찾아 하드코딩한 PC 경로를 저장하지 않는다.
$wireVsWhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
if (!(Test-Path -LiteralPath $wireVsWhere)) { throw 'Visual Studio C++ 도구 집합을 찾을 수 없습니다.' }
$wireVsPath = & $wireVsWhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (!$wireVsPath) { throw 'x64 C++ 도구 집합이 필요합니다.' }
$wireDevCmd = Join-Path $wireVsPath 'Common7/Tools/VsDevCmd.bat'
New-Item -ItemType Directory -Path $wireBuildRoot -Force | Out-Null
# 콘솔 프로그램만 컴파일·실행하며 게임/네트워크 장치는 열지 않는다.
$wireBatch = @"
@echo off
chcp 65001 >nul
call "$wireDevCmd" -host_arch=x64 -arch=x64 >nul
if errorlevel 1 exit /b 1
cl /nologo /MD /O2 /std:c++20 /EHsc /utf-8 /I"$wireRepoRoot\cpppj\src" /Fe:probe_squid_wire.exe /Fo:probe_squid_wire.obj "$wireProbeSource" /link "$wireLibrary" "$wireQuickJsLibrary" ws2_32.lib version.lib iphlpapi.lib winmm.lib d3d11.lib dxgi.lib d3dcompiler.lib shell32.lib ole32.lib mmdevapi.lib user32.lib gdi32.lib dwmapi.lib advapi32.lib imm32.lib
if errorlevel 1 exit /b 1
probe_squid_wire.exe > native-observation.tsv
if errorlevel 1 exit /b 1
"@
[IO.File]::WriteAllText((Join-Path $wireBuildRoot 'run_probe.bat'), $wireBatch, [Text.UTF8Encoding]::new($false))
Push-Location $wireBuildRoot
try {
    & $env:ComSpec /d /c run_probe.bat
    if ($LASTEXITCODE -ne 0) { throw "C++ 관측 빌드/실행 실패: $LASTEXITCODE" }
} finally { Pop-Location }
# 계산한 기대값 대신 실제 프로그램의 관측 행만 검증·저장한다.
$wireRows = [IO.File]::ReadAllLines((Join-Path $wireBuildRoot 'native-observation.tsv'))
if ($wireRows.Count -ne 256) { throw "C++ 관측 행 수 불일치: $($wireRows.Count)" }
$wireLibraryHash = (Get-FileHash -LiteralPath $wireLibrary -Algorithm SHA256).Hash.ToLowerInvariant()
$wireProbeHash = (Get-FileHash -LiteralPath $wireProbeSource -Algorithm SHA256).Hash.ToLowerInvariant()
$wireHeader = @"
# C++ RawSquidTransmission 모듈의 독립 관측256행. 원본 PE fixture와 구별한다.
# netstorm.lib SHA256: $wireLibraryHash
# probe_squid_wire.cpp SHA256: $wireProbeHash
# index sent count bytes lastOwner deletionSid first64Hex tailUnchanged logHex type/default/hpReads
"@
$wireText = $wireHeader.TrimEnd() + "`n" + ($wireRows -join "`n") + "`n"
[IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($wireOutputPath)) | Out-Null
[IO.File]::WriteAllText($wireOutputPath, $wireText, [Text.UTF8Encoding]::new($false))
Write-Output "C++ 모듈 관측256행 저장: $wireOutputPath"
