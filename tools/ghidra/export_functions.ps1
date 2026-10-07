<#
.SYNOPSIS
    tools\ghidra\<이름>-functions.json 의 함수 목록을 읽기 전용 Ghidra 로 내보낸다.
    기계어 대조 도구(tools\decomp_*_oracle.py)가 읽는 extracted\<이름>\<판본>\creation.c, functions.tsv 를 만든다.
    extracted\ 는 Git 에 없으므로 PC 마다 한 번 실행해야 한다. 목록의 순서가 곧 파일 안의 순서이며 SHA 감사가 이 순서에 의존한다.

.DESCRIPTION
    목록 파일의 "editions" 는 판본 이름 -> 주소 배열이다. 배열 대신 { "base": [...], "geometry": [...] } 같은 묶음이면
    base 는 판본 폴더에, 나머지 묶음은 그 이름의 하위 폴더에 내보낸다. "script" 가 있으면 그 Ghidra 스크립트를 쓴다(기본 ExportCreation.java).
    원본 게임·업데이터는 실행하지 않는다. Ghidra 프로젝트는 -readOnly 로 열어 변경을 저장하지 않는다.

.EXAMPLE
    powershell -ExecutionPolicy Bypass -File tools\ghidra\export_functions.ps1 -Name bridgedecay
    powershell -ExecutionPolicy Bypass -File tools\ghidra\export_functions.ps1 -Name finder,bridgeeffects -Edition originals
    powershell -ExecutionPolicy Bypass -File tools\ghidra\export_functions.ps1 -All
#>
param(
    # 내보낼 목록 이름(여러 개 가능). tools\ghidra\<이름>-functions.json 이 있어야 한다.
    [string[]]$Name = @(),
    # 목록 파일이 있는 모든 이름을 내보낸다.
    [switch]$All,
    # 지정한 판본만 내보낸다(생략하면 목록에 있는 모든 판본).
    [string[]]$Edition = @(),
    # 출력 기준 폴더(프로젝트 루트 기준). 기존 결과와 비교하려고 다른 폴더에 내보낼 때만 바꾼다.
    [string]$OutputBase = 'extracted'
)

$ErrorActionPreference = 'Stop'
# 프로젝트 루트 (이 스크립트 기준 두 단계 위). 스크립트에 넘기는 출력 경로가 루트 기준이라 루트에서 실행한다.
$Root = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
# 기본 내보내기 스크립트 이름
$DefaultScript = 'ExportCreation.java'

# -All 이면 목록 파일 이름에서 이름을 모은다
if ($All) {
    $Name = @(Get-ChildItem $PSScriptRoot -Filter '*-functions.json' | Sort-Object Name |
              ForEach-Object { $_.Name -replace '-functions\.json$', '' })
}
if (-not $Name -or $Name.Count -eq 0) { throw '내보낼 이름이 없습니다. -Name <이름> 또는 -All 을 주세요.' }

# 함수 수가 목록과 다르거나 Ghidra 가 실패한 묶음 수
$Failed = 0
Push-Location $Root
try {
    # 이름마다 목록 파일을 읽어 판본·묶음 순서대로 내보낸다
    foreach ($Item in $Name) {
        $PlanPath = Join-Path $PSScriptRoot "$Item-functions.json"
        if (-not (Test-Path $PlanPath)) { throw "함수 목록이 없습니다: $PlanPath" }
        $Plan = Get-Content -Raw -Encoding UTF8 $PlanPath | ConvertFrom-Json
        $Script = if ($Plan.script) { $Plan.script } else { $DefaultScript }
        # 판본은 목록 파일에 적힌 순서대로 처리한다
        foreach ($Property in $Plan.editions.PSObject.Properties) {
            $Target = $Property.Name
            if ($Edition.Count -gt 0 -and ($Edition -notcontains $Target)) { continue }
            # 단순 배열은 base 묶음 하나로 본다
            $Groups = [ordered]@{}
            if ($Property.Value -is [System.Array]) {
                $Groups['base'] = @($Property.Value)
            } else {
                foreach ($Group in $Property.Value.PSObject.Properties) { $Groups[$Group.Name] = @($Group.Value) }
            }
            # 묶음마다 Ghidra 를 한 번 실행한다
            foreach ($GroupName in $Groups.Keys) {
                $Addresses = $Groups[$GroupName]
                $Suffix = if ($GroupName -eq 'base') { '' } else { "/$GroupName" }
                $LogSuffix = if ($GroupName -eq 'base') { '' } else { "-$GroupName" }
                $Out = "$OutputBase/$Item/$Target$Suffix"
                $Log = "$OutputBase/$Item/$Target$LogSuffix-ghidra.log"
                New-Item -ItemType Directory -Force (Join-Path $Root "$OutputBase/$Item") | Out-Null
                & (Join-Path $PSScriptRoot 'run_script.ps1') -Edition $Target -Script $Script -ScriptArgs (@($Out) + $Addresses) *> $Log
                $Code = $LASTEXITCODE
                # 머리말 한 줄을 뺀 줄 수가 내보낸 함수 수다
                $Table = Join-Path $Root "$Out/functions.tsv"
                $Rows = if (Test-Path $Table) { (Get-Content $Table | Measure-Object -Line).Lines - 1 } else { -1 }
                if ($Code -ne 0 -or $Rows -ne $Addresses.Count) {
                    $Failed++
                    Write-Warning "$Item $Target $GroupName : 실패 (종료 코드 $Code, 함수 $Rows/$($Addresses.Count)) - $Log 확인"
                } else {
                    Write-Output "$Item $Target $GroupName : 함수 $Rows 개 -> $Out"
                }
            }
        }
    }
} finally {
    Pop-Location
}
if ($Failed -gt 0) { throw "내보내기 $Failed 건이 실패했습니다." }
