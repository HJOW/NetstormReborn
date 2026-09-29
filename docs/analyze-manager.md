# AI용 원본 게임 자동 탐험 도구

## 현재 상태와 용도

`analyzeManager/`는 AI가 원본 게임을 직접 관찰할 때 사용하는 Windows 전용 보조 프로그램이다. CLI와 로컬 stdio MCP가 같은 엔진을 호출한다. 외부 AI가 캡처를 해석하고 다음 입력을 선택하며, 도구는 입력·시각·화면·메모를 문서로 기록한다. 기존 exe/영상/파일 분석과 함께 사용할 수 있다.

**실행 상태:** 2026-09-29 사용자가 실제 게임 실행 테스트를 중단하도록 요청했으나, 이후 `AGENTS.md`에 지정된 시스템에서는 확인 없이 게임을 실행해도 된다고 명시했다. 따라서 지정 시스템에서는 게임 실행을 진행할 수 있다. 그 밖의 시스템에는 기존 중단 지시가 유지되며 별도 재개 지시가 필요하다. 최신 진행 상태는 [LEFT_JOBS.md](../LEFT_JOBS.md) 첫 인수인계 절을 따른다.

### 실제 게임 실행 전 개발자 확인

일반 시스템에서 이 도구로 실제 게임을 구동해야 하면 **실행 전에 개발자(사용자)에게 실행 목적과 필요성을 설명하고 명시적인 확인을 받아야 한다.** `AGENTS.md`에 지정된 시스템에서는 이 확인이 필요하지 않으며, 이번 사용자 지시로 게임 실행도 허용되었다.

- CLI와 MCP의 `start_session`, `mcp_smoke.py --live`, 검증을 위한 원본/복사본 직접 실행에 모두 적용한다.
- **시스템 예외:** IP `10.0.0.15`, 호스트명 `vm-debian-codex`로 지정된 시스템에서는 개발자의 게임 구동 확인을 받지 않고 실행할 수 있다. 시스템 식별값을 확인하지 못했거나 일치하지 않으면 일반 시스템 규칙을 따른다.
- 지정 시스템의 실행 허용과 기술적인 실행 가능 여부는 별개다. `analyzeManager`는 Windows 전용이므로 Linux에서 사용하려면 실행 환경을 별도로 준비해야 한다.
- 일반적인 작업 진행 지시나 이전 실행 이력만으로 새로운 게임 실행이 허용되었다고 간주하지 않는다. 일반 시스템에서는 개발자가 확인한 실행 범위 안에서 진행한다. 해당 범위의 작업 단계가 완료될 때까지 확인 상태는 유지된다.
- 확인을 기다리는 동안 실제 게임을 구동하지 않는다. 게임 없는 코드 검토·문서 정리 등 독립 작업은 진행할 수 있다.
- 게임을 구동하지 않는 빌드·정적 분석·단위 테스트·기본 MCP 프로토콜 검사는 이 실행 확인 대상이 아니다.

### 검증 상태

**2026-09-29 Windows 실제 실행 검증(사용자 허용, 이후 사용자 요청으로 중단):**
- 게임 없는 검증: Release 빌드 오류/경고 0, 단위 테스트 18개 통과, `mcp_smoke.py` 기본 모드 통과.
- 실제 실행: `mcp_smoke.py --live` 통과. MCP 캡처가 PNG 이미지 콘텐츠로 전달되었다.
- CLI 호출이 끝난 뒤에도 게임이 유지되었다. 일반·강제 `end_session`이 해당 세션 게임을 종료했다.
- 포커스를 잃으면 입력을 보내지 않고 오류를 반환했다.

실행 중 사용자가 동시에 마우스를 조작한 경우가 있었다. 그래서 개별 입력에 대한 게임 반응은 이번 결과로 확정하지 않는다. 실제 게임 테스트는 나중에 사용자 입력이 없는 상태에서 다시 한다.

이번 검증에서 고친 점은 두 가지다.
- 전면 전환 실패: 첫 시작에서 창 포커스를 얻지 못했다. `AttachThreadInput` 방식과 Alt 키 신호 방식으로 재시도하게 고쳤고, 이후 포커스 획득은 성공했다.
- 전면 창 검사: 200ms 재확인을 추가하고, 실패하면 당시 전면 창 정보를 오류에 남긴다.

**주의:**
- 도움말 창의 파란 글자는 외부 링크다. 누르면 브라우저가 열려 포커스를 잃는다. 링크 줄 위에서 드래그를 시작해 실제로 브라우저가 열렸다(사용자 확인). 도움말에서는 링크 없는 빈 영역만 조작한다.
- 게임 실행 중에는 사용자가 마우스·키보드를 쓰지 않아야 증거가 섞이지 않는다.

자세한 결과는 [LEFT_JOBS.md](../LEFT_JOBS.md) 0-A절 "Windows 실제 실행 검증"에 있다.

아래는 그 이전의 기록이다.

이전 Windows 환경에서 Release 빌드와 실제 게임을 띄우지 않는 단위 테스트 17개가 통과했다. 게임 없는 MCP 검사는 초기화·도구 등록·오류 응답·EOF 종료까지 통과했다. 이후 Linux에서 PNG 증거 저장의 손상 검출을 수정하고 회귀 테스트 1개를 추가했다. 변경된 `SessionStore.cs`는 패키지 의존성 없는 .NET 10 임시 프로젝트로 정상 중복·손상 검출을 확인했지만, NuGet 다운로드가 완료되지 않아 Windows 대상 전체 빌드·테스트는 아직 다시 실행하지 못했다. 원본 복사본을 1024×768 창 모드로 실행하여 Activision 시작 화면 PNG를 저장·육안 확인했다. 후속 캡처 때 프로세스가 사라져 있었으며 원인은 미확정이다. 실제 마우스/키보드 반응, CLI 세션 유지, MCP 이미지 전달은 아직 검증하지 못했다.

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

## Linux(Wine)에서 사용

도구는 Windows용이지만 Linux에서는 win-x86 자체 포함 배포물을 Wine으로 실행한다. 준비와 게임 없는 검사는 [linux-wine.sh](../analyzeManager/linux-wine.sh)로 한다. 산출물(배포물·Wine 접두 경로)은 모두 git 제외 경로 `extracted/wine/`에 만든다.

```bash
bash analyzeManager/linux-wine.sh setup   # 도구·테스트 win-x86 배포, 32비트 접두 경로 생성, 렌더러 gdi 설정
bash analyzeManager/linux-wine.sh test    # 단위 테스트 (가짜 원본, 게임 실행 없음)
bash analyzeManager/linux-wine.sh smoke   # MCP 기본 검사 (게임 실행 없음)
bash analyzeManager/linux-wine.sh call list_sessions
bash analyzeManager/linux-wine.sh call capture_state '{"sessionId":"SESSION_ID"}'
```

- Wine에서는 저장소 경로를 `Z:\home\...` 형식으로 넘긴다(스크립트가 변환). Wine은 리눅스 루트에 연결된 `Z:\` 루트와 **마운트 지점**(예: tmpfs `/tmp`)을 링크로 보고한다. 링크 검사는 드라이브 루트만 예외로 두므로, 저장소가 별도 마운트 지점 아래에 있으면 거부된다.
- Wine의 심볼릭 링크 생성은 오류 없이 무시된다. 그래서 링크 거부 단위 테스트는 Wine에서 건너뛴다(Windows에서 확인).

**2026-09-29 확인 결과 (`vm-debian-codex`, Debian 13, Wine 10.0, Xwayland `DISPLAY=:2` 1600×900):**
- 게임 없는 검사: Linux에서 Release 빌드 오류/경고 0. Wine에서 단위 테스트 20개 중 19개 통과, 1개(링크 생성 불가) 건너뜀. `mcp_smoke.py --wine` 기본 모드 통과(프로토콜 2025-03-26, 도구 8개, EOF 종료).
- 실제 실행 1회(세션 `20260929T061418633Z-0ab98c5681a7`): 복사본 준비 약 2초, 게임 시작, 창 찾기·포커스·캡처 호출, CLI 재호출로 같은 세션 이어서 캡처, `end_session force=true` 종료까지 동작했다. 창 제목은 시작 화면 `Activision and Titanic Entertainment Present: NetStorm` → `NetStorm Main Menu`로 바뀌었다. 원본 폴더는 바뀌지 않았다.
- **캡처가 검은 화면이었다.** 시작 직후 1600×828(최대화된 창)과 7분 뒤 1024×768 캡처 모두 검은색이다. Wine의 기본 DirectDraw 구현(wined3d, OpenGL)으로 그린 내용이 GDI 화면 복사(`CopyFromScreen`)에 잡히지 않는 것으로 **추정**한다. `setup`은 접두 경로의 `HKCU\Software\Wine\Direct3D\renderer`를 `gdi`로 바꾸지만, 이 설정으로 캡처되는지는 **아직 실행해 보지 않았다.**
- **CLI 출력을 파이프로 받으면 게임이 끝날 때까지 셸이 기다린다.** Wine에서 시작한 게임이 도구의 표준 출력을 물려받기 때문이다. `linux-wine.sh call`은 결과를 임시 파일로 받아 이를 피한다(임시 파일 방식으로 `start_session`을 호출하는 것은 미검증). MCP로 쓰면 서버가 끝난 뒤에도 게임이 MCP 파이프를 잡고 있어 호스트가 EOF를 늦게 받을 수 있다.
- 입력 전달(SendInput)과 입력 뒤 화면 변화는 확인하지 못했다.

## CLI 사용 계약

형식은 다음과 같다. JSON 인자 파일은 UTF-8이며 최대 64 KB다. 셸의 따옴표 처리 문제를 줄이려면 `--args-file`을 사용한다.

```text
Netstorm.AnalyzeManager [--repo 저장소] call 도구 [--args-file 요청.json | --json JSON]
Netstorm.AnalyzeManager [--repo 저장소] mcp
```

`--repo` 생략 시 현재 경로나 실행 파일의 상위 경로에서 저장소를 찾는다. 성공은 종료 코드 0, 오류는 2다. stdout에는 `{isError,data,imagePath}` JSON 하나를 출력한다. `imagePath`는 저장된 PNG의 절대 경로다. 이미지가 필요하면 AI가 해당 파일을 읽는다.

다음 명령은 지정 시스템에서 별도 확인 없이 사용할 수 있다. 그 밖의 시스템에서는 기존 중단 지시의 재개와 실행 전 개발자 확인이 필요하다.

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

- PNG는 임시 파일로 완성한 뒤 해시 이름으로 옮긴다. 같은 이름의 기존 파일은 크기와 SHA-256을 확인한 뒤 재사용한다. 이벤트와 관찰 시각은 계속 남긴다.
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

1. 이전 Windows 환경의 Debug/Release 빌드: 오류 0, 경고 0. Linux에서 PNG 저장 변경 후 전체 빌드는 NuGet 다운로드 실패로 미실시.
2. 이전 단위 테스트 17개 통과: 원본 설정 보존·복사본 경로 변경, 세션 경로 제한, 동시 잠금, UTF-8 로그 분할/재개, 이미지 중복 제거, 타 프로세스 거부, ROI와 키 제한, 변화 비율. 손상 PNG 회귀 테스트 1개를 추가했으며 Windows 대상 테스트 실행은 남았다.
3. 게임 없는 MCP 기본 검사: 프로토콜 2025-03-26 초기화, 8개 도구와 스키마, 실행 전 개발자 확인·지정 시스템 예외 문구, 세션 목록, 잘못된 ID의 오류 응답, stdin EOF 종료.
4. 실제 원본의 첫 창: 1024×768, 시작 화면 PNG 저장 및 육안 확인.
5. 이후 게임 종료/상실: 다음 캡처가 오류를 반환했고, 중단 요청 뒤 `Netstorm` 프로세스가 없음을 확인했다.
6. 2026-09-29 Windows 재검증에서는 위 게임 상실이 재현되지 않았다. CLI 종료 뒤 게임 유지, `--live`(MCP PNG 전달 포함), 일반·강제 종료를 확인했다. 개별 입력에 대한 게임 반응은 사용자 동시 조작 가능성 때문에 다음 테스트에서 다시 확인한다.

`analyzeManager/tests/mcp_smoke.py`의 기본 모드는 게임을 실행하지 않고 MCP 초기화·8개 도구 스키마·오류 응답·EOF 종료를 검사하며 **통과했다**. `--live`는 2026-09-29 Windows에서 실행해 **통과했다**. `--live`를 추가하면 CLI로 게임을 실행한 후 MCP 캡처/입력/기록/종료까지 검사한다. 지정 시스템에서는 별도 확인 없이 실행할 수 있다. 그 밖의 시스템에서는 기존 중단 지시의 재개와 실행 전 개발자 확인이 필요하다.

```powershell
# 실제 게임을 실행하지 않는 MCP 검증
python analyzeManager/tests/mcp_smoke.py --exe analyzeManager/bin/Release/net10.0-windows/Netstorm.AnalyzeManager.exe
```

재개 시 실제 실행 수명 문제를 먼저 확인하고, 그 다음 입력 전후 화면·MCP 이미지·관찰 보고서를 대조한다. 도움말/캠페인 탐험, 게임 규칙 자동 확정, 시간 수치 측정 시나리오는 아직 구현 또는 검증된 결과가 아니다.
