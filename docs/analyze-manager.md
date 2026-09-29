# AI용 원본 게임 자동 탐험 도구

## 현재 상태와 용도

`analyzeManager/`는 AI가 원본 게임을 직접 관찰할 때 사용하는 Windows 전용 보조 프로그램이다. CLI와 로컬 stdio MCP가 같은 엔진을 호출한다. 외부 AI가 캡처를 해석하고 다음 입력을 선택하며, 도구는 입력·시각·화면·메모를 문서로 기록한다. 기존 exe/영상/파일 분석과 함께 사용할 수 있다.

**2026-09-29 사용자 요청으로 실제 게임 실행 테스트를 중단했다. 다른 작업 때문에 게임을 구동하면 안 된다. 사용자가 재개를 명시하기 전에는 아래 게임 실행 명령, `start_session`, `mcp_smoke.py --live`를 실행하지 않는다.** 최신 진행 상태는 [LEFT_JOBS.md](../LEFT_JOBS.md) 첫 인수인계 절을 따른다.

### 실제 게임 실행 전 개발자 확인

이 도구를 이용하면서 실제 게임을 구동해야 하면 **실행 전에 개발자(사용자)에게 실행 목적과 필요성을 설명하고 명시적인 확인을 받아야 한다.** 단, `AGENTS.md`에 지정된 시스템 예외가 있다. 확인 절차의 예외와 현재의 게임 실행 중단 지시는 별개다.

- CLI와 MCP의 `start_session`, `mcp_smoke.py --live`, 검증을 위한 원본/복사본 직접 실행에 모두 적용한다.
- **시스템 예외:** IP `10.0.0.15` 및 호스트명 `vm-debian-codex`에 해당하는 시스템에서는 개발자의 게임 구동 확인을 받지 않아도 된다. 시스템 식별값을 확인하지 못했거나 일치하지 않으면 일반 규칙을 따른다.
- 이 예외는 확인 절차에만 적용된다. 위의 현재 실행 중단 요청이 해제되기 전에는 지정 시스템에서도 원본이나 복사본을 실행하지 않는다. 기술적인 실행 가능 여부도 별도로 확인해야 한다.
- 일반적인 작업 진행 지시나 이전 실행 이력만으로 새로운 게임 실행이 허용되었다고 간주하지 않는다. 일반 시스템에서는 개발자가 확인한 실행 범위 안에서 진행한다. 해당 범위의 작업 단계가 완료될 때까지 확인 상태는 유지된다.
- 확인을 기다리는 동안 실제 게임을 구동하지 않는다. 게임 없는 코드 검토·문서 정리 등 독립 작업은 진행할 수 있다.
- 게임을 구동하지 않는 빌드·정적 분석·단위 테스트·기본 MCP 프로토콜 검사는 이 실행 확인 대상이 아니다.

### 검증 상태

현재 Release 빌드와 실제 게임을 띄우지 않는 단위 테스트 17개가 통과했다. 게임 없는 MCP 검사는 초기화·도구 등록·오류 응답·EOF 종료까지 통과했다. 원본 복사본을 1024×768 창 모드로 실행하여 Activision 시작 화면 PNG를 저장·육안 확인했다. 후속 캡처 때 프로세스가 사라져 있었으며 원인은 미확정이다. 실제 마우스/키보드 반응, CLI 세션 유지, MCP 이미지 전달은 아직 검증하지 못했다.

## 구성과 준비

- Windows 10/11의 잠금 해제된 대화형 데스크톱, .NET 10 SDK/Windows Desktop Runtime이 필요하다.
- 저장소에 `originals/Netstorm.exe`, `originals/d/options.cfg`, `originals/d/setup.cfg` 및 나머지 원본 자산이 있어야 한다.
- 구현은 `analyzeManager/`에 있으며 원본 설정 포맷은 기존 `Netstorm.Assets`를 참조한다. 게임 프로젝트의 UI에는 추가하지 않았다.
- MCP는 공식 C# SDK `ModelContextProtocol` 2.2.0의 stdio 전송을 사용한다. 네트워크 포트를 열지 않는다. [공식 SDK](https://github.com/modelcontextprotocol/csharp-sdk), [시작 안내](https://github.com/modelcontextprotocol/csharp-sdk/blob/main/docs/concepts/getting-started.md).

저장소 루트에서 빌드한다. 빌드와 단위 테스트 명령 자체는 원본 게임을 실행하지 않는다.

```powershell
dotnet build analyzeManager/AnalyzeManager.csproj -c Release
dotnet test analyzeManager/tests/AnalyzeManager.Tests.csproj
```

실행 파일은 `analyzeManager/bin/Release/net10.0-windows/Netstorm.AnalyzeManager.exe`다. `Netstorm.sln`에는 Windows 전용 도구를 추가하지 않았다.

## CLI 사용 계약

형식은 다음과 같다. JSON 인자 파일은 UTF-8이며 최대 64 KB다. 셸의 따옴표 처리 문제를 줄이려면 `--args-file`을 사용한다.

```text
Netstorm.AnalyzeManager [--repo 저장소] call 도구 [--args-file 요청.json | --json JSON]
Netstorm.AnalyzeManager [--repo 저장소] mcp
```

`--repo` 생략 시 현재 경로나 실행 파일의 상위 경로에서 저장소를 찾는다. 성공은 종료 코드 0, 오류는 2다. stdout에는 `{isError,data,imagePath}` JSON 하나를 출력한다. `imagePath`는 저장된 PNG의 절대 경로다. 이미지가 필요하면 AI가 해당 파일을 읽는다.

다음 명령은 **현재 실행 중단 요청이 해제된 뒤**, 일반 시스템에서는 개발자에게 목적과 필요성을 알리고 실행 확인을 받은 다음 사용한다. 위에 지정된 시스템은 확인 절차가 면제된다.

```powershell
analyzeManager/bin/Release/net10.0-windows/Netstorm.AnalyzeManager.exe call start_session --args-file analyzeManager/examples/start.json
```

응답의 `data.sessionId`를 다음 요청에 사용한다. 예를 들어 `capture.json`은 아래 형태다. `SESSION_ID`는 실제 ID로 바꾼다.

```json
{"sessionId":"SESSION_ID","region":"0,0,320,120","includeImage":true}
```

```powershell
analyzeManager/bin/Release/net10.0-windows/Netstorm.AnalyzeManager.exe call capture_state --args-file capture.json
```

세션 정보는 디스크에 남으므로 `list_sessions`로 최근 ID와 실행 여부를 찾을 수 있다. 일반 CLI 종료 시 게임에 종료 명령을 보내지 않는다. 다만 AI의 명령 실행기가 자식 프로세스를 함께 정리하는 환경에서는 게임이 유지되지 않을 수 있다. 이번 후속 캡처 실패가 이 경우인지는 미확정이며, 재개 후 지속 실행되는 MCP 서버 및 같은 상위 작업 안의 CLI 호출로 확인해야 한다.

## 로컬 MCP 연결

먼저 Release 빌드를 마친 뒤 [설정 예시](../analyzeManager/examples/mcp-settings.json)의 경로를 환경에 맞춰 MCP 호스트에 등록한다. 해당 예시는 일반적인 `mcpServers` 형식이며 실제 설정 위치·형식은 사용하는 호스트에 맞춘다. 이번 작업에서 사용자 MCP 설정은 수정하지 않았다.

```json
{
  "mcpServers": {
    "netstorm-analysis": {
      "command": "D:/Workspace/git/NetstormReborn/analyzeManager/bin/Release/net10.0-windows/Netstorm.AnalyzeManager.exe",
      "args": ["--repo", "D:/Workspace/git/NetstormReborn", "mcp"]
    }
  }
}
```

빌드된 exe를 직접 사용한다. MCP stdout은 JSON-RPC 전용이고 서버 로그는 stderr에 쓴다. 빌드 로그가 섞일 수 있는 `dotnet run`을 MCP 명령으로 등록하지 않는다. 캡처 응답은 텍스트/구조화 데이터와 PNG 이미지 콘텐츠를 반환하도록 구현했다. `includeImage=false`이면 응답의 이미지 전송만 생략하며 증거 파일은 그대로 기록한다.

## 도구 목록

| 도구 | 주요 인자 | 결과·용도 |
|---|---|---|
| `list_sessions` | 없음 | 최근 20개 세션 ID, 실행 여부, 보고서 경로 |
| `start_session` | `label`(최대 200자) | 독립 복사본 실행, 세션 ID, 첫 캡처 |
| `game_status` | `sessionId` | 프로세스·창 크기·포커스·보고서 경로, 포커스 변경 없음 |
| `capture_state` | `sessionId`, `region`, `includeImage` | 전체 또는 관심 영역 PNG와 SHA-256 |
| `game_input` | `sessionId`, `kind`, 좌표 또는 키 | 입력 전후 화면, 입력 요청/전송/결과, 변화 비율 |
| `wait_for_change` | `sessionId`, `region`, `timeoutMs`, `pollMs`, `threshold` | 기준 화면 대비 변화 감지, 시간 초과 시 `matched=false` |
| `record_observation` | `sessionId`, `note`, `evidenceHash` | AI 해석 메모와 같은 세션의 증거 연결 |
| `end_session` | `sessionId`, `force` | 종료 요청, 확인 창이 남으면 `closed=false`, 증거 보존 |

입력 예시:

```json
{"sessionId":"SESSION_ID","kind":"click","x":400,"y":240,"button":"left","settleMs":500}
```

```json
{"sessionId":"SESSION_ID","kind":"drag","x":200,"y":300,"toX":360,"toY":300,"durationMs":400}
```

```json
{"sessionId":"SESSION_ID","kind":"key","key":"ESCAPE","durationMs":80,"settleMs":300}
```

- `kind`는 `move`, `click`, `drag`, `key`; 버튼은 `left`, `right`, `middle`이다.
- 입력 좌표는 **현재 게임 창 전체 클라이언트 영역의 물리 픽셀**이다. ROI 이미지 안 좌표를 쓸 때는 ROI의 x/y를 더한다. 테두리와 제목 표시줄은 제외한다.
- `region`은 `x,y,width,height`; 생략/빈 문자열은 전체 클라이언트다. 창 밖 영역은 거부한다.
- 키는 영문자/숫자, 방향키, `ESCAPE`, `ENTER`, `SPACE`, `TAB`, `BACKSPACE`, `HOME`, `END`, `PAGEUP`, `PAGEDOWN`, 기능키, `CTRL+A` 같은 최대 4개 조합을 지원한다. `F11`, `ALT+ENTER`는 금지한다. 메뉴에서 직접 전체화면을 선택하는 동작도 분석 과정에서 피해야 한다.
- `durationMs`는 1~2000, `settleMs`는 0~5000이다. 취소/실패 시 눌렀던 키와 버튼 해제를 시도한다.
- `wait_for_change`의 `timeoutMs`는 1~30000, `pollMs`는 50~5000, `threshold`는 0 초과~1 이하다. RGB 중 하나의 차이가 24 이상인 픽셀의 비율을 기준 화면과 비교한다. 애니메이션만으로도 조건이 성립할 수 있으므로 적절한 ROI를 고른다.
- `record_observation` 메모는 최대 8000자이며 AI의 해석이라고 표시한다. 화면 변화나 OS 입력 성공만으로 게임 규칙을 확정하지 않는다.
- `force=true` 종료는 PID·시작 시각·실행 파일 경로가 모두 일치하는 해당 세션 게임만 대상으로 한다.

권장 분석 순서는 세션 확인 → 실행 → 화면 확인 → 한 번 입력 → 필요한 ROI 변화 관찰 → 근거 해시를 붙인 메모 → 종료 확인이다. 본 도구의 관찰 시간은 OS 캡처 시각이며 게임 내부 틱이나 영상의 프레임 번호와 같지 않다.

## 원본 보존과 저장 용량

세션은 `extracted/analyzeManager/<sessionId>/` 아래에 만든다.

```text
session.json           세션 ID, PID, 시작 시각, 실행 파일 해시
game/                  독립 복사한 원본 실행 파일과 자산
screens/<sha256>.png    중복 제거된 화면
events-0001.jsonl       AI가 읽는 입력·관찰·오류 이벤트
report.md              한국어 보고서 인덱스
report-0001.md          이벤트와 화면 참조
```

`originals/` 전체를 세션마다 복사한다(현재 약 310 MB). 복사본의 `options.cfg`와 `setup.cfg`에서 `InstallDir`, 창 모드, 1024×768 설정을 바꾼다. 원본 파일은 도구에서 수정하지 않는다. 이는 파일 복사에 의한 작업 분리이며 원본 실행 파일의 OS 접근 전체를 제한하는 샌드박스는 아니다.

- PNG는 정확히 동일한 바이트의 SHA-256이면 재사용한다. 이벤트와 관찰 시각은 계속 남긴다.
- 대기 중간 프레임은 저장하지 않고 시작/마지막 화면만 저장한다. 필요한 관심 영역만 캡처할 수도 있다.
- 이벤트와 Markdown은 UTF-8 바이트 크기로 약 4 MB마다 분할한다.
- 도구가 쓰는 개별 파일은 **50,000,000바이트 미만**으로 제한한다. 복사할 원본에 이 이상의 파일이 있으면 준비를 중단한다. 현재 원본 파일들은 이 한도 미만이다.
- 세션당 증거 PNG는 최대 500개/합계 200 MB, 이벤트는 최대 10,000개다. 게임 복사본 용량은 PNG 예산에 포함하지 않는다. 저장 한도를 넘는 기록은 오류를 반환하며 새 세션이 필요하다.
- 원본 게임이 자체적으로 만드는 파일 크기까지 도구가 강제하지는 않는다. Git에 옮길 증거는 별도로 크기를 확인한다.
- 실행 결과와 복사본은 기존 `extracted/` 제외 규칙으로 Git에 들어가지 않는다. 장기 보존할 분석 결과만 `docs/`로 정리한다. 자동 삭제는 하지 않으며, 게임 종료 및 필요한 근거 보존 후 세션 폴더를 정리한다.

## 제약과 남은 검증

캡처는 실제 화면의 클라이언트 영역을 읽는다. 게임이 전면에 있고 화면 안에 표시되어야 한다. 포커스 변경이나 다른 창의 가림이 감지되면 중단한다. 가림 검사는 세 지점 표본이므로 모든 겹침을 검출하는 것은 아니다. 입력 중에는 데스크톱 마우스와 포커스를 사용하므로 다른 작업과 동시에 돌리지 않는다. 잠금 화면·무인 CI·Linux 원본 조작은 대상이 아니다.

한 저장소의 CLI/MCP는 같은 파일 잠금으로 동시 조작을 거부한다. 실행 중인 관리 세션이 있으면 중복 게임 실행을 거부한다. 외부에서 수동으로 띄운 게임을 관리하거나 종료하지 않는다.

현재 확인한 범위:

1. Debug/Release 빌드: 오류 0, 경고 0.
2. 단위 테스트 17개: 원본 설정 보존·복사본 경로 변경, 세션 경로 제한, 동시 잠금, UTF-8 로그 분할/재개, 이미지 중복 제거, 타 프로세스 거부, ROI와 키 제한, 변화 비율.
3. 게임 없는 MCP 기본 검사: 프로토콜 2025-03-26 초기화, 8개 도구와 스키마, 실행 전 개발자 확인·지정 시스템 예외 문구, 세션 목록, 잘못된 ID의 오류 응답, stdin EOF 종료.
4. 실제 원본의 첫 창: 1024×768, 시작 화면 PNG 저장 및 육안 확인.
5. 이후 게임 종료/상실: 다음 캡처가 오류를 반환했고, 중단 요청 뒤 `Netstorm` 프로세스가 없음을 확인했다. 수명 문제 원인·입력 반응·MCP 이미지 전달은 미검증이다.

`analyzeManager/tests/mcp_smoke.py`의 기본 모드는 게임을 실행하지 않고 MCP 초기화·8개 도구 스키마·오류 응답·EOF 종료를 검사하며 **통과했다**. `--live`는 아직 실행하지 않았다. `--live`를 추가하면 CLI로 게임을 실행한 후 MCP 캡처/입력/기록/종료까지 검사한다. `--live`는 현재 실행 중단 요청이 해제된 뒤에만 수행한다. 일반 시스템에서는 사전에 개발자 확인이 필요하고, 위의 지정 시스템에서는 확인 절차가 면제된다.

```powershell
# 실제 게임을 실행하지 않는 MCP 검증
python analyzeManager/tests/mcp_smoke.py --exe analyzeManager/bin/Release/net10.0-windows/Netstorm.AnalyzeManager.exe
```

재개 시 실제 실행 수명 문제를 먼저 확인하고, 그 다음 입력 전후 화면·MCP 이미지·관찰 보고서를 대조한다. 도움말/캠페인 탐험, 게임 규칙 자동 확정, 시간 수치 측정 시나리오는 아직 구현 또는 검증된 결과가 아니다.
