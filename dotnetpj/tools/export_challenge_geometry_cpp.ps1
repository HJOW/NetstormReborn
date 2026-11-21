# 실제 CPP 생성자의 저장 비트105행을 추출한다. 기존 기대값이 다르면 덮어쓰지 않는다.
[CmdletBinding()]
param(
    # 비어 있으면 Core의 새 CPP Source fixture 경로를 사용한다.
    [string]$OutputPath = ''
)

# 오류 이후 잘못된 기대값을 저장하지 않는다.
$ErrorActionPreference = 'Stop'
# 도구가 속한 저장소와 임시 빌드·제품과 분리된 probe 소스다.
$repoRoot = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$probeSource = Join-Path $PSScriptRoot 'challenge_geometry_probe'
$probeBuild = Join-Path $repoRoot 'extracted/challenge-geometry-probe'
if ([string]::IsNullOrWhiteSpace($OutputPath)) {
    $OutputPath = Join-Path $repoRoot 'dotnetpj/tests/Netstorm.Core.Tests/Fixtures/challenge-geometry-cpp.tsv'
}
$OutputPath = [IO.Path]::GetFullPath($OutputPath)
& cmake -S $probeSource -B $probeBuild
if ($LASTEXITCODE -ne 0) { throw 'CS 좌표 probe의 CMake 구성 실패' }
& cmake --build $probeBuild --config Release --parallel 2
if ($LASTEXITCODE -ne 0) { throw 'CS 좌표 probe의 빌드 실패' }
# 다중 구성 Windows와 단일 구성 도구에서 실제 생성된 경로를 선택한다.
$probeExe = Join-Path $probeBuild 'Release/challenge-geometry-probe.exe'
if (-not (Test-Path -LiteralPath $probeExe)) { $probeExe = Join-Path $probeBuild 'challenge-geometry-probe' }
$rows = @(& $probeExe)
if ($LASTEXITCODE -ne 0 -or $rows.Count -ne 105) { throw 'CS 좌표 probe의 실제 생성/전체 초기화 검사 실패' }
# 소스 이력은 읽기만 하고 이 관찰이 독립 PE 자료가 아님을 파일에 명시한다.
$sourceRevision = & git -C $repoRoot log -1 --format=%H -- cpppj/src/o/RawChallengeState.cpp
if ($LASTEXITCODE -ne 0) { throw 'CS 좌표 CPP 소스 기준 조회 실패' }
$header = @('# CPP Source 관찰이며 독립 원본 PE 실행 자료가 아니다.', '# source: cpppj/src/o/RawChallengeState.cpp', "# source-revision: $sourceRevision", '# 열: field uint32-bits-or-integer')
$content = [string]::Join("`n", @($header) + @($rows)) + "`n"
if (Test-Path -LiteralPath $OutputPath) {
    if ([IO.File]::ReadAllText($OutputPath).Replace("`r`n", "`n") -ne $content) { throw '기존 CS 좌표 기대값과 다름: 원문 변경을 재검토하고 수동으로 교정해야 합니다.' }
    Write-Output "기존 CS 좌표105행 일치: $OutputPath"
    return
}
New-Item -ItemType Directory -Path (Split-Path $OutputPath -Parent) -Force | Out-Null
[IO.File]::WriteAllText($OutputPath, $content, [Text.UTF8Encoding]::new($false))
Write-Output "CPP CS 좌표105행 추출: $OutputPath"
