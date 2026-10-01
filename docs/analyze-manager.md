# AI용 원본 게임 자동 탐험 도구

## 현재 상태와 용도

`analyzeManager/`에는 AI가 원본 게임을 관찰·조작하는 Windows 전용 도구와, YouTube 영상을 읽는 교차 플랫폼 portable 도구가 있다. 두 실행 파일 모두 CLI와 로컬 stdio MCP를 제공하지만 portable 빌드는 YouTube 도구만 등록한다. 외부 AI가 화면을 해석하고 다음 입력을 선택하며, 도구는 입력·시각·화면·메모를 문서로 기록한다.

**2026-10-01 추가:** 원본 게임 대신 **YouTube 플레이 영상**을 읽는 `youtube_*` 도구(CLI·MCP 공통)가 있다. 게임을 실행하지 않으므로 아래 개발자 확인 규칙의 대상이 아니다. → [YouTube 영상 분석](#youtube-영상-분석-원본-게임-실행-없음)

**실행 상태:** 2026-09-29 사용자가 실제 게임 실행 테스트를 중단하도록 요청했으나, 이후 `AGENTS.md`에 지정된 시스템에서는 확인 없이 게임을 실행해도 된다고 명시했다. 따라서 지정 시스템에서는 게임 실행을 진행할 수 있다. 그 밖의 시스템에는 기존 중단 지시가 유지되며 별도 재개 지시가 필요하다. 최신 진행 상태는 [LEFT_JOBS.md](../LEFT_JOBS.md) 첫 인수인계 절을 따른다.

### 실제 게임 실행 전 개발자 확인

일반 시스템에서 이 도구로 실제 게임을 구동해야 하면 **실행 전에 개발자(사용자)에게 실행 목적과 필요성을 설명하고 명시적인 확인을 받아야 한다.** `AGENTS.md`에 지정된 시스템, 또는 개발자가 수동 컨트롤 분석을 직접 요청한 작업 단계에서는 이 확인이 필요하지 않으며, 이번 사용자 지시로 게임 실행도 허용되었다.

- CLI와 MCP의 `start_session`, `mcp_smoke.py --live`, 검증을 위한 원본/복사본 직접 실행에 모두 적용한다.
- **시스템 예외:** `AGENTS.md`에 지정된 아래 시스템에서는 개발자의 게임 구동 확인을 받지 않고 실행할 수 있다. 시스템 식별값(IP·호스트명)을 확인하지 못했거나 일치하지 않으면 일반 시스템 규칙을 따른다.

  | 구분 | IP | 호스트명 | 환경 |
  |---|---|---|---|
  | 시스템 1 | `10.0.0.15` | `vm-debian-codex` | Debian 13 + Wine 10.0 (Linux/Wine 절 참고) |
  | 시스템 2 | `192.168.0.94` | `HJOW-Athlon` | Windows 10 Pro (2026-09-29 추가, 이 저장소 작업 PC) |

- **수동 조작 분석 예외(2026-09-30 추가):** 개발자(사용자)가 **기존 게임 수동 컨트롤 방식으로 분석을 진행하라고 직접 요청**한 경우, 해당 작업 단계에서는 시스템과 관계없이 실제 게임 구동 확인을 다시 받지 않아도 된다. 요청이 없었거나 다른 작업 단계로 넘어갔다면 일반 규칙으로 돌아간다.
- 지정 시스템의 실행 허용과 기술적인 실행 가능 여부는 별개다. 원본 게임 제어 도구는 Windows 전용이므로 Linux(시스템 1)에서는 Wine 실행 환경이 필요하다. YouTube 영상 분석은 Linux portable 빌드를 사용할 수 있다. 시스템 2는 Windows이므로 별도 준비 없이 게임 제어 도구를 실행할 수 있다.
- 지정 시스템 목록은 `AGENTS.md`가 기준이다. **`AGENTS.md`는 AI가 직접 수정하지 않으며**, 목록 변경이 필요하면 개발자(사용자)에게 수정을 요청한다. 이 문서·`LEFT_JOBS.md`·`analyzeManager/README.md`의 목록은 그 사본이다.
- 일반적인 작업 진행 지시나 이전 실행 이력만으로 새로운 게임 실행이 허용되었다고 간주하지 않는다. 일반 시스템에서는 개발자가 확인한 실행 범위 안에서 진행한다. 해당 범위의 작업 단계가 완료될 때까지 확인 상태는 유지된다.
- 확인을 기다리는 동안 실제 게임을 구동하지 않는다. 게임 없는 코드 검토·문서 정리 등 독립 작업은 진행할 수 있다.
- 게임을 구동하지 않는 빌드·정적 분석·단위 테스트·기본 MCP 프로토콜 검사는 이 실행 확인 대상이 아니다.

### 검증 상태

**2026-09-29 밤 Windows 실제 실행(`HJOW-Athlon`, 세션 `20260929T120526716Z-327849a23cac`):**
- 약 30분 동안 CLI 호출을 이어 가는 동안(도구 이벤트 약 460건) 게임이 유지되었다. 캡처·입력·`wait_for_change`가 정상으로 동작했고, 마지막에 `end_session force=true`로 종료했다.
- 원본 관찰 결과는 [화면 목록 1.3절](screens/README.md)에 적었다.
- 사용 중 알게 된 점은 아래 "원본 조작 요령"에 정리했다.

### 원본 조작 요령 (2026-09-29 Windows 실행에서 확인)

- **메인 메뉴 버튼은 `move`로 커서를 먼저 올린 뒤 `click`한다.** 이동 없이 바로 보낸 80ms 클릭에는 Help 버튼이 반응하지 않았다. 올린 뒤 150ms 클릭은 모든 메뉴에서 동작했다.
- **메뉴의 Technical Help는 누르지 않는다.** 외부 `help\help.exe`가 실행되어 Windows 도움말 앱(`HelpPane.exe`)·Edge 창이 앞으로 나오고, 도구는 포커스 변경으로 입력을 중단한다.
- `wait_for_change`는 호출이 시작된 순간의 화면을 기준으로 삼는다. 연속 호출 사이(CLI 시작 약 2~4초)에 일어난 전환은 놓칠 수 있다.
  - 전환 시각을 재려면 한 번의 호출 제한 시간(최대 30초) 안에 전환이 들어오도록 시작 시각을 맞춘다.
  - 또는 짧은 간격의 `capture_state`와 이벤트 기록의 UTC 시각을 함께 쓴다.
- 대기 시간을 잴 때는 도우미 스크립트의 셸 시각이 아니라 `events-0001.jsonl`의 `input_sent` UTC 시각을 기준으로 삼는다.

**2026-09-29 저녁 Windows 실제 실행(`HJOW-Athlon`, 지정 예외 시스템 2):** Release 빌드 오류·경고 0. 세션 `20260929T113907044Z-929836edfdde`에서 다음을 확인했다.
- Windows 화면 복사 캡처(`method=screen`)가 정상이다. Wine 캡처 변경 뒤 남아 있던 **Windows 실제 캡처 재확인은 해소**했다.
- Claude Code Bash에서 CLI를 여러 번 호출하는 동안 게임이 유지되었다(`start_session` → `capture_state`·`game_input`·`wait_for_change` → `end_session force=true`).
- `wait_for_change`로 Auto-Demo 전환을 감지했다(변화율 0.375).
- 두 번째 ESC 전송 직후 전면 창이 VS Code로 바뀌자, 도구가 전면 창 정보(handle·pid·제목)를 담은 오류를 남기고 작업을 중단했다(안전장치 동작).
- CLI 출력은 임시 파일로 받았으므로 **파이프 상속 방지 효과(EOF)는 이번에도 검증하지 않았다.**
- 원본 관찰 결과는 [화면 목록 1.2절](screens/README.md)에 적었다. 같은 날의 앞선 세션 `20260929T113420484Z-3358366e5f90`은 사용자 중단으로 메인 메뉴 진입까지만 진행했다.

**2026-09-29 Windows 게임 없는 후속 검증:** Wine 캡처 경로 추가 뒤 Release 빌드 오류·경고 0, 단위 테스트 20개 중 19개 통과·1개 건너뜀(심볼릭 링크 생성 권한 없음), `mcp_smoke.py` 기본 모드 통과(도구 8개·EOF 종료). 파이프 상속 방지 코드 적용 후에도 Release 단위 테스트와 MCP 기본 검사를 다시 통과했다. 원본 및 복사본 게임은 실행하지 않았다. Windows 실제 캡처와 파이프 상속 방지 효과는 재확인하지 않았다.

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
- **캡처가 검은 화면이었다.** 시작 직후 1600×828(최대화된 창)과 7분 뒤 1024×768 캡처 모두 검은색이다. Wine의 기본 DirectDraw 구현(wined3d, OpenGL)으로 그린 내용이 GDI 화면 복사(`CopyFromScreen`)에 잡히지 않는 것으로 **추정**한다.
- **CLI 출력을 파이프로 받으면 게임이 끝날 때까지 셸이 기다린다.** Wine에서 시작한 게임이 도구의 표준 출력을 물려받기 때문이다. `linux-wine.sh call`은 결과를 임시 파일로 받아 이를 피한다. MCP로 쓰면 서버가 끝난 뒤에도 게임이 MCP 파이프를 잡고 있어 호스트가 EOF를 늦게 받을 수 있다.

이 파이프 문제를 줄이기 위해 `AnalysisEngine.StartAsync`에서 게임 프로세스를 별도 표준 입출력 파이프로 시작하고, 입력을 닫으며 출력을 비우도록 바꿨다. 게임 없이 빌드·단위 테스트·MCP 기본 검사를 통과했지만, 원본 게임을 시작하지 않았으므로 Wine CLI 파이프와 MCP EOF가 실제로 개선됐는지는 미확인이다.

**2026-09-29 오후 실제 실행 2회 (같은 시스템, 세션 `20260929T070136750Z-c11bc8b4f280`·`20260929T070536517Z-6fa3657cf3de`):**
- `renderer=gdi` 설정 뒤에도 **도구 캡처는 모든 픽셀이 (0,0,0)** 이었다(SHA-256 `a26aa2d7…`, 이전 세션과 동일). 따라서 이 설정은 검은 캡처의 해결책이 아니다. 도구의 `changedRatio`·`wait_for_change`도 Wine에서는 항상 변화 0이 되어 쓸 수 없다.
- 같은 순간 **X11 창 직접 캡처(ImageMagick `import -window <X 창 id>`)는 정상 화면**이었다. 증거는 각 세션 폴더의 `x11/*.png`(git 제외 경로).
- `linux-wine.sh call start_session`(임시 파일 방식)은 곧바로 반환했고, 이어지는 CLI 호출로 같은 세션을 조작할 수 있었다.
- 게임 창은 X11에 뜨자마자 활성 창(`_NET_ACTIVE_WINDOW`)이 되었고 도구의 포커스 검사도 통과했다. 창은 1600×828로 최대화된 채 남고 게임은 클라이언트 왼쪽 위 1024×768에만 그린다(오른쪽·아래는 검은색). 입력 좌표는 Windows와 같은 클라이언트 좌표를 쓴다.
- **입력 전달 확인(X11 캡처 기준):** 팁 창 OK (661,436) 클릭 → 팁 창이 닫히고 "Not Validated" 창 표시 → 그 OK (511,464) 클릭 → 메인 메뉴. 메인 메뉴 버튼 위치는 Windows 측정값과 같았다(윗줄 y≈311~329).
- 일반 `end_session`(WM_CLOSE)으로 도움말 창이 열린 상태의 게임도 확인 창 없이 종료되었다.

**Wine 캡처 경로 (2026-09-29 추가, 해결됨):** 도구는 `ntdll`의 `wine_get_version` 내보내기로 Wine 실행을 감지한다. Wine에서는 화면 전체 DC 대신 **게임 창 자체 DC를 BitBlt로 복사**하고(`wine-window-dc`), 결과가 전부 검으면 `PrintWindow(PW_CLIENTONLY)`로 다시 시도한다(`wine-printwindow`). 둘 다 검으면 그대로 기록하되 `wine-black`으로 표시한다. Windows에서는 기존 화면 복사(`screen`)를 그대로 쓴다. 쓴 방식은 증거 항목의 `method`에 남는다.
- 실제 실행 확인(세션 `20260929T071149864Z-507f034c3833`, `renderer=gdi` 접두 경로): 캡처 33건이 모두 `wine-window-dc`로 성공했다. 같은 순간의 X11 창 직접 캡처 9쌍과 **픽셀 단위로 완전히 같았다**. `changedRatio`와 `wait_for_change`도 정상 동작한다(Auto-Demo 시작을 변화율 0.375로 감지).
- Wine 기본 렌더러(OpenGL)에서도 창 DC 캡처가 되는지는 확인하지 않았다. `setup`이 설정하는 `renderer=gdi`를 유지한다.
- 이 변경 뒤 Windows에서의 빌드·단위 테스트·게임 없는 MCP 기본 검사는 위 후속 검증에서 통과했다. Windows 실제 캡처는 아직 다시 확인하지 않았다(Windows 경로의 코드는 함수로 분리만 했다).

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
| `set_guide_steps` | `sessionId`, `steps` | 줄 단위 안내를 세션에 저장해 사용자 조작 창에서 표시 |
| `end_session` | `sessionId`, `force` | 종료 요청, 확인 창이 남으면 `closed=false`, 증거 보존 |
| `youtube_probe` 외 5개 | `url`·`videoId` 등 | 게임 실행 없이 YouTube 영상을 읽는다 — [YouTube 영상 분석](#youtube-영상-분석-원본-게임-실행-없음) |

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

## 사용자 직접 조작 녹화 모드

기존 CLI/MCP 자동 조작과 별도로 `guide` 명령을 실행하면 게임 옆에 단계 안내 창이 열린다. 이 명령은 **이미 실행 중인 관리 세션**에 붙으며 게임을 새로 시작하지 않는다. 실제 게임을 시작하는 `start_session`에는 위 실행 확인 규칙이 그대로 적용된다. Windows 10/11용 기능이며 Wine에서의 안내 창·오디오 녹화는 검증하지 않았다.

먼저 조작 지시를 UTF-8 텍스트 파일에 한 줄당 한 단계씩 적는다. 빈 줄은 무시한다. 예를 들어 `extracted/analyzeManager/steps.txt`에 아래처럼 적는다.

```text
튜토리얼 1을 열고 첫 안내 화면을 확인하세요.
다리 조각 하나를 섬 가장자리에 놓으세요.
가이저까지 다리를 연결하고 사제의 이동을 관찰하세요.
```

기존 `start_session`의 응답에서 받은 ID로 안내 창을 연다. `--steps-file`의 내용은 세션 안의 `guide-steps.txt`에 복사되어 창을 닫고 다시 열 때도 남는다. 이후에는 `--steps-file`을 생략해 같은 안내를 다시 사용할 수 있다. AI가 MCP/CLI의 `set_guide_steps`에 줄 단위 `steps`를 전달해 미리 저장할 수도 있다. 안내 창을 연 뒤 파일을 바꿨으면 `안내 다시 읽기`를 누른다.

```powershell
analyzeManager/bin/Release/net10.0-windows/Netstorm.AnalyzeManager.exe guide --session SESSION_ID --steps-file extracted/analyzeManager/steps.txt
```

안내 창은 96 DPI 기준 기본 클라이언트 크기 560×620으로 열리며, Windows 디스플레이 배율에 따라 글꼴·창·컨트롤 크기가 함께 조정된다. 테두리나 오른쪽 아래 크기 조절 손잡이를 끌어 사용자가 크기를 바꿀 수 있다. 창 크기에 맞춰 안내 본문이 늘어나고 조작 버튼과 상태 줄은 계속 보인다. 최소 창 크기는 96 DPI 기준 500×540이다. DPI 배율을 반영한 실제 크기로 게임 창의 제목 표시줄·테두리를 포함한 오른쪽 또는 왼쪽 빈 화면에 안내 창을 놓는다. 게임 창의 크기·위치가 바뀌거나 안내 창을 키워 게임과 겹치거나 화면 밖으로 옮기면 약 1초 안에 같은 크기로 새 위치를 찾는다(녹화 대기 중에도 적용). 게임 내 대화상자가 열려도 배치 기준은 주 게임 창이다. 놓을 공간이 없으면 안내 모드를 시작하지 않으며, 사용 중 공간이 없어지면 상태를 표시하고 진행 중인 녹화를 중단한다. 공간이 다시 생기면 안내 창을 재배치한다. `녹화 시작 / 이어서`를 누른 다음 사용자가 게임을 직접 조작한다. `다음 단계`와 `이전`을 누르면 안내와 해당 시각이 기록되고 게임 포커스로 돌아간다. `녹화 중단`은 게임을 종료하지 않고 파일을 닫는다. 다시 시작하거나 안내 창을 다시 열면 기존 녹화 파일 뒤에 새 번호의 조각을 만든다. 원본 게임 자체에 진행 저장 기능이 없으므로 게임을 종료한 뒤 같은 미션 상태로 돌아가는 기능은 아니다.

세션의 `recording/`에는 다음 파일이 생긴다.

| 파일 | 내용 |
|---|---|
| `video-0001.avi` 등 | 게임 창만 10 FPS, JPEG 품질 75의 MJPEG 영상으로 기록. 화면 커서는 영상에 합성하지 않으며 좌표는 입력 기록에 남김 |
| `video-0001.frames.csv` 등 | 각 영상 프레임의 UTC 시각과 세션 경과 밀리초 |
| `audio-0001.wav` 등 | Windows 기본 재생 장치의 출력 소리를 WAV로 기록. 다른 앱 소리도 포함될 수 있음 |
| `audio-0001.start.txt` 등 | 해당 WAV의 시작 UTC 시각 |
| `input-0001.jsonl` 등 | 게임이 전면일 때의 마우스 이동·버튼·휠과 키 누름·해제, 게임 클라이언트 좌표와 시각 |

AVI와 WAV는 각각 **48,000,000바이트 전에** 새 조각으로 분할한다. 입력 로그는 약 4 MB마다 나눈다. 화면 프레임·소리·입력의 시각을 함께 써서 후속 분석에서 대응시킨다. 안내 창이나 다른 앱의 입력은 게임 입력으로 기록하지 않는다. 안내 창이 열린 동안에는 같은 데스크톱 잠금을 사용하므로 기존 AI 조작 명령을 동시에 보낼 수 없다. 게임 창이 가려지거나 최소화되면 녹화를 중단하고 오류를 표시한다. 녹화 파일은 `extracted/`에 남아 Git에는 포함되지 않는다.

**실제 게임 검증(2026-09-30):** `HJOW-Athlon`의 RDP 화면이 보이는 상태에서 사용자가 튜토리얼 1을 직접 조작했다. 세션 `20260929T154230683Z-bdf8f92d93a1`에 10 FPS 영상 2,438프레임(AVI 4개), 기본 출력 소리(WAV 2개), 마우스·키 입력(JSONL 1개)을 기록하고 오류 없이 중단했다. 각 AVI/WAV는 48 MB 미만이며 `ffprobe`로 모두 읽히고, AVI 4개는 FFmpeg 오류 없이 끝까지 디코딩됐다. 영상 약 243.8초와 오디오 약 243.9초가 맞으며 두 WAV에서 실제 소리도 검출됐다. 안내 단계 8개와 최종 600 SP 결과 화면을 기록했다. [게임 관찰 결과](screens/README.md#16-사용자-직접-조작-녹화-튜토리얼-1-완료-2026-09-30-windows-hjow-athlon)를 참고한다. 이전의 0프레임 세션은 RDP 화면 가림 조건에서 발생한 사례다. 녹화 중단 후 다시 시작하는 흐름은 게임 없는 검사로만 확인했다.

같은 날 튜토리얼 2도 세션 `20260929T160257202Z-2e9f1861506d`에서 끝까지 녹화했다. 게임 화면 2,751프레임(AVI 5개), WAV 3개, 입력 JSONL 1개를 오류 없이 남겼다. 각 파일은 48 MB 미만이고 AVI 5개 모두 끝까지 디코딩됐으며, 영상 약 275.1초와 오디오 약 275.3초가 맞는다. [튜토리얼 2 관찰 결과](screens/README.md#17-사용자-직접-조작-녹화-튜토리얼-2-완료-2026-09-30-windows-hjow-athlon)에 건설·생산·회수 장면을 기록했다.

**사용 요령 (2026-09-30 `HJOW-X3D` 실행에서 확인):**
- **Windows PowerShell 5.1에서 `start_session`의 출력을 `>`로 파일에 받으면 명령이 끝나지 않는다.** 게임 프로세스가 도구의 표준 출력을 물려받아 PowerShell이 게임 종료까지 기다린다(게임은 정상 실행되고 출력 파일에는 세션 ID가 들어 있다). `cmd.exe /c "\"exe\" call start_session --args-file … > \"파일\" 2>&1"`처럼 cmd의 리다이렉트를 쓰고, 경로는 **절대 경로**로 준다(cmd는 `/` 상대 경로를 해석하지 못함). 대기가 남은 호출은 게임이 끝나면 저절로 끝나지만, 도구 호출의 제한 시간(백그라운드 기본 30분)에 걸리면 게임이 같이 정리될 수 있다.
- **`guide`는 창을 닫을 때까지 돌아오지 않으므로 백그라운드로 실행**하고, 끝나면 안내 창을 닫은(`CloseMainWindow`) 뒤 `record_observation`, `end_session force=true` 순으로 정리한다.
- **안내 문장은 "이 동작을 한 뒤 「다음 단계」를 누르세요" 형식으로 한 사건에 한 번만 누르게 쓴다.** 단계 N이 표시된 시각(`guided_step`의 `index=N`)은 **단계 N−1의 완료 시각**이다. 같은 사건을 두 문장으로 나누면 사용자가 두 번 눌러 시각이 한 단계씩 밀린다(2026-09-30 기록에서 최대 10초 차이). 시간 측정은 안내 버튼이 아니라 영상 프레임 CSV와 입력 JSONL로 한다.
- **영상 분석:** AVI는 MJPEG이라 ffmpeg 없이 RIFF `movi` 목록의 `00dc` 청크를 차례로 꺼내 JPEG으로 열 수 있다(Python + Pillow). 프레임 순번은 `video-000N.frames.csv`의 `frame`·`utc`와 같다.

## 기존 게임 플레이 녹화 분석 모드

`record-play`는 이미 실행 중인 관리 세션에 지침 없는 안내 창을 붙인다. 게임을 새로 시작하지 않는다. 게임 실행이 필요한 `start_session`에는 앞의 개발자 확인 규칙이 적용된다. 안내 창에서 **녹화 시작 / 다시 시작**을 누른 뒤 사용자가 게임을 자유롭게 플레이하고, 끝나면 **녹화 중단**을 누른다. 창은 계속 열려 있으므로 중단 뒤에도 같은 게임 세션에서 다시 녹화를 시작할 수 있다.

```powershell
analyzeManager/bin/Release/net10.0-windows/Netstorm.AnalyzeManager.exe record-play --session SESSION_ID
```

안내 창 아래 상태 영역은 **녹화 대기**, 초록색 **녹화 중**(경과 시간·영상 프레임 수), **녹화 중단됨 · 다시 시작 가능**, **게임 종료**를 구분한다. 화면 캡처·음성·입력 기록 오류, 게임 창 가림·크기 변경, 창을 게임 옆에 둘 공간 부족으로 녹화가 멈추면 빨간 상태 영역에 이유가 표시되고 녹화 시작 버튼이 다시 켜진다. 0프레임으로 끝난 녹화도 오류로 표시한다. 문제를 해결한 뒤 버튼을 누르면 앞선 파일을 덮지 않고 다음 번호의 조각에 기록한다. 게임 자체가 종료된 경우에는 새 게임 세션이 필요하다. 안내 창을 닫아도 녹화 파일은 안전하게 닫고 게임은 종료하지 않는다.

파일은 저장소의 `playingVideos/<SESSION_ID>/`에 저장된다. `video-0001.avi` 등은 게임 창의 10 FPS MJPEG 영상이며 **커서·소리가 영상 안에 들어 있지 않다**. 같은 번호의 `.frames.csv`에 프레임 시각이 있고, `audio-0001.wav` 등의 Windows 기본 출력 소리와 시작 시각 `.start.txt`, `input-0001.jsonl` 등의 게임 입력·좌표가 함께 남는다. AVI와 WAV는 각각 48,000,000바이트 전에 자동으로 다음 파일을 시작해 **파일 하나가 50 MB에 이르지 않도록** 한다. 총 녹화 용량에는 별도 한도가 없으므로 긴 플레이 전에는 저장 공간을 확인한다. `recording-index.json`은 녹화가 중단될 때 완성된 조각의 이름·바이트 크기·시각 파일을 갱신한다. `playingVideos/`의 내용은 Git에서 제외된다.

사용자가 프롬프트로 녹화 완료를 알리면 AI는 해당 세션의 `recording-index.json`에서 이번 영상 목록을 찾고, `.frames.csv`와 입력 JSONL의 시각을 맞춰 필요한 장면의 JPEG 프레임을 추출·관찰한 뒤 근거 시각을 적어 문서화한다. WAV는 소리가 필요한 관찰에 사용한다. 기존 `tools/videoframes.py`는 FFmpeg가 있는 환경의 영상 추출 도구이며, 이 모드의 MJPEG AVI는 위 수동 녹화 절의 RIFF `00dc` 청크 추출 방법으로 FFmpeg 없이도 읽을 수 있다. 짧은 사건의 정확한 시각이나 화면에 드러나지 않는 규칙은 별도 관찰이 필요하다.

## YouTube 영상 분석 (원본 게임 실행 없음)

YouTube에는 원본 게임 플레이 영상이 많다(AGENTS.md의 튜토리얼·캠페인 영상, 전용 채널 `@netstormcampaigns2591`). 직접 플레이·녹화하는 것보다 빠르게 화면·흐름·시간을 확인할 수 있도록, 영상 주소를 받아 **필요한 시각의 프레임만** 원격 스트림에서 읽는 도구를 두었다(2026-10-01). CLI `call`과 MCP가 같은 엔진(`YouTubeAnalyzer`)을 쓰며, AI는 주로 MCP로 사용한다. MCP 응답은 구조화 데이터와 함께 프레임(여러 장이면 관찰표) PNG 이미지를 돌려준다.

* **게임을 실행하지 않는다.** `start_session`의 개발자 확인 규칙 대상이 아니며 데스크톱 잠금도 쓰지 않는다(게임 분석과 동시에 사용 가능).
* **인터넷을 쓴다.** 외부 프로그램 `yt-dlp`(메타데이터·스트림 주소)와 `ffmpeg`/`ffprobe`(원격 탐색·디코딩)가 필요하다. Windows는 `PREPARE.ps1`, Linux는 `PREPARE.sh`의 해당 항목으로 설치하거나 환경 변수 `NETSTORM_YTDLP`·`NETSTORM_FFMPEG`·`NETSTORM_FFPROBE`로 경로를 지정한다. 받는 대상은 YouTube 도메인 주소뿐이다(다른 사이트 주소는 거부).
* 기록은 Git 제외 경로 `extracted/youtube/<영상 ID>/`에 남는다.

### Linux에서 YouTube 도구 사용

Windows 게임 분석기는 `net10.0-windows` GUI와 Win32 창 제어가 필요하므로 Linux에서 게임을 시작·조작하지 않는다. 별도 `analyzeManager/portable/AnalyzeManager.Portable.csproj`는 공용 분석 엔진을 사용해 YouTube 전용 CLI·stdio MCP를 제공한다. 프레임 PNG 합성은 GDI+ 대신 FFmpeg RGB 디코딩과 내장 픽셀 글꼴을 쓴다.

Linux에서는 .NET 10 SDK, `yt-dlp`, Deno, `ffmpeg`, `ffprobe`가 필요하다. 저장소 루트에서 `bash PREPARE.sh`를 실행해 yt-dlp·Deno·FFmpeg·`ytanalyzer` 항목을 선택해 준비한다. 패키지 설치를 할 수 없으면 FFmpeg는 BtbN 정적 빌드를 사용자 `~/.local/bin`에 설치한다. 전체 항목의 설치 상태만 확인할 때는 `bash PREPARE.sh --check-only --all`을 쓴다.

```bash
dotnet build analyzeManager/portable/AnalyzeManager.Portable.csproj -c Release
YT=analyzeManager/portable/bin/Release/net10.0/Netstorm.AnalyzeManager
$YT call youtube_probe --json '{"url":"adR1Kap60hw"}'
$YT call youtube_frames --json '{"videoId":"adR1Kap60hw","times":"13:31, 15:30.5"}'
$YT mcp
```

MCP 설정에서도 실행 파일을 `analyzeManager/portable/bin/Release/net10.0/Netstorm.AnalyzeManager`로, 인자를 `mcp`로 지정한다. 이 서버는 `youtube_*` 도구 6개만 공개하며 `start_session` 등 원본 게임 제어 도구는 등록하지 않는다. `PREPARE.sh`의 `ytanalyzer` 항목은 이 프로젝트를 Release 빌드한다. Linux 기본 실행·이미지 생성은 2026-10-01 Debian GUI 환경에서 오류 없이 빌드했고, YouTube 프레임 추출과 해시 메모 기록을 확인했다.

### 도구

| 도구 | 주요 인자 | 결과·용도 |
|---|---|---|
| `youtube_probe` | `url`(영상 주소 또는 11자 ID) | 제목·채널·길이·원본 화질·**업로더 챕터**·**SponsorBlock 구간**·주의 사항. `info.json` 저장. **다른 영상 도구보다 먼저 호출**한다 |
| `youtube_list` | `url`(채널 `@핸들`·`/channel/ID`·재생목록), `limit`(기본 100, 최대 500) | 영상 ID·제목·길이 목록. 분석할 영상을 고를 때 쓴다 |
| `youtube_frames` | `url`/`videoId`, `times`(쉼표 목록) 또는 `start`·`step`·`count`(최대 60장), `region`, `maxHeight`(기본 1080), `columns`, `cellWidth`, `allowDurationMismatch`, `includeImage` | 지정 시각 프레임 PNG(`frames/<sha256>.png`)와, 여러 장이면 시각을 적은 관찰표(`sheets/<sha256>.png`). 한 장당 약 3~5초. 같은 시각·화질·잘라내기는 다시 받지 않는다 |
| `youtube_clip` | `url`/`videoId`, `start`·`end`(초, 최대 300초), `maxHeight`(기본 720), `includeAudio` | 원본 프레임률 그대로의 mp4(`clips/`). 시작 프레임을 다시 인코딩해 **clip 시각 t = 영상 시각 start + t**. 50 MB 미만이어야 하며 넘으면 실패를 알린다. 애니메이션·이동·건설 시간처럼 프레임 단위 측정용 |
| `youtube_note` | `url`/`videoId`, `note`(최대 8000자), `time`, `evidenceHash` | AI 해석 메모를 `notes.jsonl`·`report.md`에 남긴다. 증거는 이 영상에서 뽑은 프레임 sha256 만 받는다 |
| `youtube_videos` | 없음 | 조회해 둔 영상 목록(재접속 뒤 이어서 분석) |

시각 표기: `612.5`, `10:12.5`, `1:02:03`, `1h2m3s`, `75s`. 주소의 `t=`·`start=`는 `youtube_probe` 응답의 `urlStartSeconds`로 돌려준다. `region`은 **원본 스트림 해상도** 기준 `x,y,width,height`다(1080p 방송 녹화라면 게임 영역 `240,0,1440,1080`).

```powershell
$exe = "analyzeManager/bin/Release/net10.0-windows/Netstorm.AnalyzeManager.exe"
& $exe call youtube_probe  --json '{"url":"https://www.youtube.com/watch?v=0p7VvzSxTAY"}'
& $exe call youtube_frames --json '{"videoId":"0p7VvzSxTAY","times":"8:00, 8:02.5, 1:20:00"}'
& $exe call youtube_frames --json '{"videoId":"0p7VvzSxTAY","start":0,"step":120,"count":24,"maxHeight":360,"columns":6,"cellWidth":240}'
& $exe call youtube_clip   --json '{"videoId":"0p7VvzSxTAY","start":480,"end":490}'
& $exe call youtube_list   --json '{"url":"https://www.youtube.com/@netstormcampaigns2591","limit":50}'
```

권장 순서: `youtube_list`로 영상 찾기 → `youtube_probe`(챕터로 미션 경계 파악, 광고 성격 구간 확인) → 넓은 간격의 `youtube_frames` 관찰표로 장면 위치 찾기 → 좁은 간격 프레임 또는 `youtube_clip`으로 정밀 관찰 → `youtube_note`로 근거 해시와 함께 기록 → 확정된 결과는 `docs/videos/`에 정리.

### 광고 처리

YouTube 시청 중 나오는 광고가 분석을 오염시키지 않도록 다음과 같이 다룬다.

1. **재생기 광고(앞·중간·끝 광고)는 들어오지 않는다.** 이 광고는 YouTube 재생기가 본 영상과 별개로 트는 다른 영상이다. 도구는 브라우저로 재생하거나 화면을 녹화하지 않고 yt-dlp가 알려 준 **원본 미디어 스트림**을 ffmpeg로 직접 읽으므로, 프레임에 광고가 섞이지 않고 **영상 시각 = 업로드 원본의 시각**이다. 광고 길이만큼 시각이 밀리는 일도 없다. (같은 이유로 브라우저 재생 화면 캡처로 분석하지 않는다.)
2. **서버 삽입 광고(SSAI) 대비:** YouTube는 광고를 영상 스트림에 직접 이어 붙이는 방식을 시험한 적이 있다. 그런 스트림은 원래 길이보다 길어지므로, 스트림 주소를 처음 받을 때 `ffprobe`로 실제 길이를 재서 메타데이터 길이와 비교한다. `max(2초, 길이의 0.5%)`보다 길면 시각이 어긋날 수 있으므로 **`youtube_frames`·`youtube_clip`이 거부**한다. 잠시 뒤 다시 시도하거나(`stream-h*.json` 삭제) 위험을 알고 `allowDurationMismatch=true`로 진행한다. 실측: `0p7VvzSxTAY` 스트림 6093.5초 / 메타데이터 6094초(정상).
3. **업로더가 영상 안에 넣은 광고**(협찬·자기 홍보·구독 요청·인트로·아웃트로 등)는 스트림의 일부라 막을 수 없다. `youtube_probe`가 **SponsorBlock** 공개 구간을 함께 받아 `nonContentSegments`로 알려 주고, 그 안의 프레임은 `nonContentSegment`(관찰표에서는 주황 글씨)로, 겹치는 구간 저장은 `warnings`로 표시한다. SponsorBlock 등록이 없는 영상도 많으므로(지금까지 조회한 영상은 모두 0개) 프레임이 게임 화면인지는 AI가 눈으로도 확인한다. 업로더 챕터(`chapters`)는 광고가 아닌 미션 경계 등으로 쓴다.
4. 스트림 주소는 몇 시간 뒤 만료된다(주소의 `expire`). 만료 10분 전이면 새로 받고, 추출 중 거부되면 한 번 새로 받아 다시 시도한다.

### 저장 형식

```text
extracted/youtube/<영상 ID>/
  info.json            youtube_probe 결과 (제목·길이·챕터·SponsorBlock 구간·주의 사항)
  stream-h<높이>.json   스트림 주소 캐시 (형식·해상도·fps·스트림 길이·만료 시각)
  frames/<sha256>.png  추출 프레임 (같은 그림은 파일 하나)
  frames.jsonl         시각·화질·잘라내기 → 프레임 해시 색인 (재사용)
  sheets/<sha256>.png  관찰표
  clips/<시작>-<끝>-h<높이>[-a].mp4   구간 저장
  notes.jsonl          AI 메모
  report.md            사람이 읽는 기록 (조회·프레임·구간·메모)
```

### 제약

* 연령 제한·회원 전용·비공개 영상은 로그인 쿠키가 없으면 받지 못한다(현재 쿠키 옵션 없음). 생방송 중인 영상은 길이가 확정되지 않아 주의가 붙는다.
* 원격 탐색이라 네트워크 상태에 따라 한 장 3~5초가 걸린다. 많은 장면은 낮은 `maxHeight`(예: 360)로 훑은 뒤 필요한 곳만 1080으로 다시 뽑는다.
* YouTube 재인코딩 영상은 원본 팔레트 색과 다를 수 있고, 업로더의 녹화 해상도·자막·편집이 섞일 수 있다. 색 비교는 스크린샷·원본 자산을, 시간 측정은 `youtube_clip` 프레임을 쓴다. 영상 fps(대개 30·60)보다 짧은 간격은 잴 수 없다.
* yt-dlp가 YouTube 변경으로 실패하면 Windows에서는 `winget upgrade yt-dlp.yt-dlp`, Linux에서는 `python3 -m pip install --user --upgrade yt-dlp`로 갱신한다.

### 검증 (2026-10-01, `DESKTOP-HJOW`)

* Release 빌드 오류·경고 0. 단위 테스트 `YouTubeVideoTests`(주소·목록 주소·시각 해석, 광고 성격 구간, 서버 삽입 광고 판정, 만료 시각, 경로 제한) 포함 전체 52개 중 51개 통과·1개 기존 건너뜀.
* MCP: `python analyzeManager/tests/mcp_smoke.py --exe … --youtube https://youtu.be/CI3dCrUt4tY` 통과(도구 15개, 관찰표가 PNG 이미지 콘텐츠로 옴). 기본 모드도 YouTube 도구 목록·잘못된 주소 거부를 검사한다.
* CLI 실측: `0p7VvzSxTAY`(1:41:34, 1280×720 30fps, 챕터 5개) 조회, 4장 관찰표 약 25초, 같은 프레임 재사용 0.5초, 10초 구간 저장 약 16초(773 KB, 10.000초 30fps, 첫 프레임이 8:00 프레임과 일치), 채널 목록 5개, 다른 사이트 주소·영상 길이 초과 시각 거부.

## Windows 원격 데스크톱(RDP)으로 접속한 PC에서 사용할 때 (2026-09-29 확인)

- `HJOW-Athlon`은 원격 데스크톱 세션(`rdp-tcp#…`)으로 사용 중이며, 콘솔 세션은 잠금 화면(`LogonUI.exe`) 상태였다. 도구와 게임은 RDP 세션 안에서 실행된다.
- **RDP 클라이언트 창이 최소화되거나 가려진 동안에는 그 세션의 화면이 그려지지 않는다.** 이때 다음 증상이 나타났다(세션 `20260929T114913277Z-391e6a079ff0`).
  - `game_status`: `foreground=false`
  - `game_input`·`capture_state`: "게임 창의 포커스를 얻지 못했습니다" 오류
  - .NET `CopyFromScreen`: "The handle is invalid"
- 같은 PC에서 RDP 화면이 보이던 앞선 세션은 정상으로 동작했다. 따라서 **분석 중에는 RDP 창을 최소화하지 않고 화면에 띄워 두어야 한다.** 사용자의 마우스·키보드 입력도 없어야 한다.
- 확인 방법: `qwinsta`로 현재 세션이 RDP인지 본다. 도구 오류가 반복되면 사용자에게 RDP 창을 띄워 달라고 요청한다. 가려진 창에 클릭을 억지로 보내는 우회는 다른 프로그램(예: VS Code)을 조작할 위험이 있으므로 쓰지 않는다.

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
- PNG의 세션당 개수·총량 제한은 없다. 개별 PNG는 50 MB 미만이어야 하며, 이벤트는 세션당 최대 10,000개다. 기존 세션에서도 500장 이후 캡처를 이어갈 수 있다. 디스크 잔여 공간은 사용자가 확인해야 한다.
- 원본 게임이 자체적으로 만드는 파일 크기까지 도구가 강제하지는 않는다. Git에 옮길 증거는 별도로 크기를 확인한다.
- 실행 결과와 복사본은 기존 `extracted/` 제외 규칙으로 Git에 들어가지 않는다. 장기 보존할 분석 결과만 `docs/`로 정리한다. 자동 삭제는 하지 않으며, 게임 종료 및 필요한 근거 보존 후 세션 폴더를 정리한다.

## Windows 방화벽 경고와 포트 (2026-09-30)

* **분석 도구 자체는 포트를 열지 않는다**(소켓 코드 없음, MCP 는 stdio). Windows 에서 뜨는 방화벽 경고는 **도구가 실행한 게임 복사본 `Netstorm.exe`** 때문이다.
* 게임은 **전투가 시작되면**(데모·튜토리얼·미션) TCP **6799**(`gameServerPort`)를 모든 인터페이스에서 리슨한다. 메인 메뉴에서는 열리지 않고, 한번 열리면 게임 종료까지 유지된다. Wine 에서 `ss` 로 확인했다.
* 세션마다 게임을 새 경로(`extracted\analyzeManager\<세션>\game\Netstorm.exe`)로 복사하므로, 방화벽 규칙이 경로 기준이라 **세션마다 경고가 다시 뜰 수 있다.** 포트를 고정해도(이미 6799 고정) 이 반복은 막지 못한다.
* **해결 방침(사용자 결정 2026-09-30):** 임시로 **TCP 6799 를 방화벽 예외로 미리 등록**한다. 사용자가 Windows에서 `PREPARE.ps1`의 포트 허용 기능과 비관리자 선택 차단이 정상 동작함을 확인했다. 규칙 세부 값·반복 등록·되돌리기·게임 실행 시 경고 억제 여부는 추가 확인이 필요하다. 상태와 절차는 [network-ports.md](exe/network-ports.md) 7절.
* 클라이언트 유효성 검사("Not Validated" 창)와 멀티플레이는 클론에서 각각 구현하지 않음·후순위다(같은 문서 1-1절).

## 제약과 남은 검증

캡처는 실제 화면의 클라이언트 영역을 읽는다. 게임이 전면에 있고 화면 안에 표시되어야 한다. 포커스 변경이나 다른 창의 가림이 감지되면 중단한다. 가림 검사는 세 지점 표본이므로 모든 겹침을 검출하는 것은 아니다. 입력 중에는 데스크톱 마우스와 포커스를 사용하므로 다른 작업과 동시에 돌리지 않는다. 잠금 화면·무인 CI·Linux 원본 조작은 대상이 아니다.

한 저장소의 CLI/MCP는 같은 파일 잠금으로 동시 조작을 거부한다. 실행 중인 관리 세션이 있으면 중복 게임 실행을 거부한다. 외부에서 수동으로 띄운 게임을 관리하거나 종료하지 않는다.

현재 확인한 범위:

1. 최신 Windows Release 빌드: 오류 0, 경고 0. Wine 캡처 경로 변경 이후의 게임 없는 빌드다.
2. 최신 Windows 단위 테스트 20개 중 19개 통과·1개 건너뜀(심볼릭 링크 생성 권한 없음): 원본 설정 보존·복사본 경로 변경, 세션 경로 제한, 동시 잠금, UTF-8 로그 분할/재개, 이미지 중복 제거·손상 검출, 타 프로세스 거부, ROI와 키 제한, 변화 비율, 캡처 방식 기록.
3. 게임 없는 MCP 기본 검사: 프로토콜 2025-03-26 초기화, 9개 도구와 스키마, 실행 전 개발자 확인·지정 시스템 예외 문구, 세션 목록, 잘못된 ID의 오류 응답, stdin EOF 종료.
4. 실제 원본의 첫 창: 1024×768, 시작 화면 PNG 저장 및 육안 확인.
5. 이후 게임 종료/상실: 다음 캡처가 오류를 반환했고, 중단 요청 뒤 `Netstorm` 프로세스가 없음을 확인했다.
6. 2026-09-29 Windows 재검증에서는 위 게임 상실이 재현되지 않았다. CLI 종료 뒤 게임 유지, `--live`(MCP PNG 전달 포함), 일반·강제 종료를 확인했다. 개별 입력에 대한 게임 반응은 사용자 동시 조작 가능성 때문에 다음 테스트에서 다시 확인한다.

`analyzeManager/tests/mcp_smoke.py`의 기본 모드는 게임을 실행하지 않고 MCP 초기화·9개 도구 스키마·오류 응답·EOF 종료를 검사하며 **통과했다**. `--live`는 2026-09-29 Windows에서 실행해 **통과했다**. `--live`를 추가하면 CLI로 게임을 실행한 후 MCP 캡처/입력/기록/종료까지 검사한다. 지정 시스템에서는 별도 확인 없이 실행할 수 있다. 그 밖의 시스템에서는 기존 중단 지시의 재개와 실행 전 개발자 확인이 필요하다.

```powershell
# 실제 게임을 실행하지 않는 MCP 검증
python analyzeManager/tests/mcp_smoke.py --exe analyzeManager/bin/Release/net10.0-windows/Netstorm.AnalyzeManager.exe
```

재개 시 실제 실행 수명 문제를 먼저 확인하고, 그 다음 입력 전후 화면·MCP 이미지·관찰 보고서를 대조한다. 도움말/캠페인 탐험, 게임 규칙 자동 확정, 시간 수치 측정 시나리오는 아직 구현 또는 검증된 결과가 아니다.
