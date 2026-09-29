# LEFT_JOBS — NetStorm 클론 프로젝트 작업 계획 및 인수인계

> 최종 갱신: 2026-09-29
> 프로젝트 목표(AGENTS.md): 원본 NetStorm: Islands at War 를 디컴파일/분석하여 클론 코딩하고,
> **Windows 10/11** 과 **GUI 환경의 Linux** 에서 동작하며 **여러 언어를 지원**하는 게임을 만든다.
> **1차 목표 언어: 영어, 한국어** (그 외 언어는 이후 확장).
> **우선순위: Windows 10/11 > Linux** (Linux 지원은 우선순위가 낮다 — 설계상 이식성은 유지하되 검증·배포는 Windows 먼저).
> **화면 요구사항(2026-09-28 AGENTS.md 추가)**: 풀스크린 모드와 화면비 **16:9 · 16:10 · 4:3** 지원, 풀스크린에서 **마우스를 화면 끝에 대면 화면 이동**(원본도 지원) — 1.7절

---

## 0-A. 최신 인수인계 — AI용 원본 분석 도구 (2026-09-29)

### Linux/Wine 실제 실행 테스트 (2026-09-29 16시, `vm-debian-codex`) — 입력 전달 확인, 캡처는 X11 필요 → 아래 절에서 캡처 해결

- **실행 방식:** 사용자가 Claude Code 권한 방식을 Manual로 바꾸고 실행 테스트를 요청했다. 권한 창 조작 때문에 포커스가 VS Code로 가므로, 한 번 허용한 스크립트 안에서 게임 시작 → X11 활성 창 대기 → 입력 → 종료까지 진행했다. 사용자에게는 포커스용 클릭을 게임 그림 밖(오른쪽 검은 영역)에만 하도록 요청했다. 실행 2회(1회는 스크립트 실수로 X11 캡처 실패 — 제목 검색이 다른 X 창 id를 고름, 활성 창 id를 쓰도록 고쳐 재실행). 두 세션 모두 `end_session`으로 종료, 실행 중인 `Netstorm.exe` 없음, `git status` 깨끗(`originals/` 변경 없음).
  - 세션: `20260929T070136750Z-c11bc8b4f280`(캡처 비교), `20260929T070536517Z-6fa3657cf3de`(입력). X11 캡처 증거는 각 세션 폴더 `x11/*.png`. 사용한 스크립트는 세션 임시 폴더에 있었고 저장소에 넣지 않았다.
- **확인한 것(확실):**
  - `renderer=gdi`로도 **도구 캡처(Wine GDI 화면 복사)는 전부 (0,0,0)** → 이 대책은 효과 없음. 도구의 `changedRatio`·`wait_for_change`는 Wine에서 쓸 수 없다.
  - **X11 창 직접 캡처(`import -window <활성 창 id>`)는 정상.**
  - `linux-wine.sh call start_session`(임시 파일 방식)은 곧바로 반환하고 이후 CLI 호출로 세션을 이어 쓸 수 있다.
  - 게임 창은 뜨자마자 X11 활성 창이 되며 도구 포커스 검사 통과. 창은 1600×828 최대화 상태로 남고 게임은 왼쪽 위 1024×768에만 그린다.
  - **도구 입력이 게임에 전달된다:** 팁 창 OK (661,436) 클릭 → "Not Validated" 창 → OK (511,464) → 메인 메뉴. 메인 메뉴 버튼 위치는 Windows 측정값과 같다.
  - Windows 절의 재확인 대상 중: **메인 메뉴에서 F1 → 도움말 창 "NetStorm Instructions"가 열린다**(입력 1.5초 뒤 캡처). **도움말 창에서 ESC(80ms 누름)는 창을 닫지 않았다**(1초 뒤 화면 동일).
  - 일반 `end_session`(WM_CLOSE)은 도움말 창이 열린 상태에서도 확인 창 없이 종료했다.
- **다음 작업:** → 1·2는 아래 "Wine 캡처 경로 구현" 절에서 진행함.

### Wine 캡처 경로 구현 + 재확인 대상 조사 (2026-09-29 16시 10분, `vm-debian-codex`) — ✅ 캡처 해결

- **도구 수정(`analyzeManager/WindowsGame.cs`, `SessionStore.cs`, 테스트):** `ntdll!wine_get_version`으로 Wine을 감지해, Wine에서는 게임 **창 자체 DC를 BitBlt로 복사**(`wine-window-dc`)하고, 전부 검으면 `PrintWindow(PW_CLIENTONLY)`(`wine-printwindow`), 둘 다 검으면 `wine-black`으로 표시한다. Windows는 기존 화면 복사(`screen`)를 함수로 분리만 했다. 증거 항목에 `method` 필드를 추가했다(기본값 `screen`, 기존 JSON과 호환). 단위 테스트에 `method` 전달 확인을 추가했다.
  - Linux에서 Wine 쪽 X11 명령 실행(`cmd /c Z:\usr\bin\import`)은 Wine 10에서 되지 않아(ELF 실행 불가) 이 방식은 버렸다.
- **검증(확실):** `linux-wine.sh setup` 빌드 오류/경고 0, Wine 단위 테스트 20개 중 19개 통과·1개 건너뜀(기존과 같은 링크 테스트). 실제 실행 1회(세션 `20260929T071149864Z-507f034c3833`): 캡처 33건 전부 `wine-window-dc` 성공, 같은 순간 X11 캡처 9쌍과 **픽셀 단위로 동일**. `changedRatio`·`wait_for_change` 정상. 정상 종료, 실행 중인 `Netstorm.exe` 없음, `originals/` 변경 없음.
- **당시 미검증:** Wine 캡처 변경 직후에는 Windows 빌드·테스트·실제 캡처를 다시 하지 않았다. Wine 기본(OpenGL) 렌더러에서의 창 DC 캡처도 미확인(`renderer=gdi` 유지). Windows 빌드·테스트는 아래 후속 검증에서 처리했다.
- **2026-09-29 Windows 게임 없는 후속 검증:** `dotnet build analyzeManager/AnalyzeManager.csproj -c Release --no-restore` 오류·경고 0, `dotnet test analyzeManager/tests/AnalyzeManager.Tests.csproj -c Release --no-restore` 20개 중 19개 통과·1개 건너뜀(심볼릭 링크 생성 권한 없음), `mcp_smoke.py` 기본 모드 통과(프로토콜 2025-03-26, 도구 8개, EOF 종료). 이 결과로 위 **Windows 빌드·단위 테스트 미검증은 해소**했다. 샌드박스의 `obj` 및 `desktop.lock` 접근 거부 때문에 각 검사는 승인된 샌드박스 밖 권한으로 재실행했다. **Windows 실제 캡처는 이번에 검증하지 않았다.** 원본과 복사본 게임은 실행하지 않았다.
- **파이프 상속 정적 수정(게임 실행 없음):** `AnalysisEngine.StartAsync`의 게임 시작을 `UseShellExecute=false`와 표준 입출력 리디렉션으로 바꾸고, 입력을 닫으며 출력·오류를 비우도록 했다. Wine에서 관찰된 원본 게임의 호출자 stdout 파이프 상속을 끊으려는 변경이다. 변경 후 Windows Release 단위 테스트 20개 중 19개 통과·1개 건너뜀, MCP 기본 검사 통과. 이 검사는 실제 `start_session`을 호출하지 않으므로 **Windows·Wine에서 게임 실행 수명과 파이프 EOF 개선 여부는 아직 검증하지 않았다.**
- **원본 동작 확인(확실, [docs/screens/README.md](docs/screens/README.md) 1.1절):** 팁 창은 본문 클릭·ESC에 반응 없음. F1 → 도움말. 도움말 ESC로 안 닫힘. 도움말 100px 드래그 → 38px 스크롤, 화살표 1회 → 5px. Auto-Demo는 메뉴에서 마지막 입력 후 약 45초에 "The Storm Rages!" 자동 시작, 시작 직후 "NetStorm Demo" 안내 창(OK 약 (554,457))은 ESC로 안 닫힘.
- **관찰했지만 원인 미확인:** 데모 중 두 번째 ESC 호출 시점에 창이 1600×828(최대화)에서 1024×768로 바뀌었다. 첫 ESC 결과 기록(07:13:18.8)까지는 1600×828, 두 번째 입력 요청(07:13:20.3)에서 1024×768이었고 두 시점 모두 `foreground=true`라 도구 `FocusAsync`의 `ShowWindow(SW_RESTORE)`는 호출되지 않았을 것으로 본다. 게임 자체(데모 진입 후 창 크기 재설정)나 창 관리자가 원인일 수 있다.
- **다음 작업:**
  1. ~~Windows에서 빌드·단위 테스트·`mcp_smoke.py` 기본 모드로 회귀 확인~~ → 위 게임 없는 후속 검증에서 완료. Windows 실제 캡처 확인은 사용자 확인 후.
  2. 남은 재확인 대상: 데모 안내 창 OK 후 ESC 메뉴 막대·Game 드롭다운, Auto-Demo 대기 시간 기준(마지막 입력/메뉴 표시), 도움말 38px가 드래그 비율인지 스크롤 끝인지(작은 드래그로 확인), 창 크기 변경 원인.
  3. 메뉴 흐름 조사(Campaign·Demo·Help·Edit·Credits·Options 하위 화면; Multiplayer·전체화면 제외)를 Linux에서 도구로 진행.
  4. 파이프 상속 방지 코드를 적용했다. 실제 게임 실행이 허용되는 단계에서 Wine의 파이프 연결 CLI 호출과 MCP 서버 종료 후 EOF를 재확인한다.
  5. 게임 실행 없이 `screenShots/mainMenu*.png`와 추출 `tell.english`·`offical1~6.english`를 대조해 [메뉴 좌표·정적 이동 경로](docs/screens/main-menu.md)를 작성했다. 개별 항목의 클릭 결과는 위 2·3번 동적 확인에 남긴다. 크레딧 제목의 10.72를 보유 exe 버전으로 보던 기존 화면 문서의 오류도 정정했다.

### Linux/Wine 분석 도구 실행 (2026-09-29 저녁, `vm-debian-codex`) — 권한 거부로 중단 → 위 절에서 이어서 진행함

- **실행 상태:** 지정 예외 시스템이라 확인 없이 게임을 한 번 실행했다(세션 `20260929T061418633Z-0ab98c5681a7`, `end_session force=true`로 종료, 실행 중인 `Netstorm.exe` 없음, `originals/` 변경 없음). 이후 **두 번째 게임 실행과 X11 창 직접 캡처는 Claude Code 자동 모드 권한 분류기가 거부**했다. 우회하지 않고 멈췄다. 다음 실제 실행에는 사용자가 Claude Code 권한 규칙(예: `bash analyzeManager/linux-wine.sh call …`, `wine …` 허용)을 추가하거나 직접 허용해야 한다.
- **도구 수정:**
  - `SessionStore.RejectReparse`: 드라이브 루트는 검사에서 뺐다. Wine이 `Z:\`(리눅스 루트)를 링크로 보고해 실제 저장소 경로가 전부 거부되던 문제다. 중간 경로의 링크는 계속 거부한다. Wine은 마운트 지점(tmpfs `/tmp` 등)도 링크로 보고하므로 그 아래 저장소는 여전히 거부된다.
  - 단위 테스트 2개 추가(`AcceptsDriveRootAndOrdinaryPaths`, `RejectsLinkedParentDirectory` — 링크를 못 만드는 환경에서는 건너뜀).
  - `tests/mcp_smoke.py`에 `--wine` 선택 추가(Linux에서 Windows 전용 `CREATE_NO_WINDOW`를 쓰지 않음).
  - 신규 `analyzeManager/linux-wine.sh`: `setup`(win-x86 배포 → `extracted/wine/`, 32비트 접두 경로, 렌더러 gdi) · `test` · `smoke` · `call`.
- **게임 없는 검증(확실):** Linux Release 빌드 오류/경고 0. Wine에서 단위 테스트 20개 중 19개 통과·1개 건너뜀(Wine은 심볼릭 링크 생성을 오류 없이 무시). `mcp_smoke.py --wine` 기본 모드 통과. `linux-wine.sh`의 게임 없는 하위 명령 전부 동작. **Windows에서의 빌드·테스트는 이번 변경 후 다시 돌리지 않았다.**
- **실제 실행 1회에서 확인한 것(확실):** Wine에서 복사본 준비(약 2초), 게임 시작, 창 찾기·포커스·캡처 호출 성공, 다른 CLI 호출로 같은 세션 이어서 캡처, 강제 종료. 창 제목이 `Activision and Titanic Entertainment Present: NetStorm` → `NetStorm Main Menu`로 바뀌었다.
- **문제 1 — 캡처가 검은 화면:** 시작 직후(1600×828, 최대화 상태) 캡처와 7분 뒤(1024×768) 캡처가 모두 검은색이었다. 원인은 Wine의 기본 DirectDraw(wined3d/OpenGL) 출력이 GDI 화면 복사에 잡히지 않기 때문으로 **추정**한다. 대책으로 전용 접두 경로에 `HKCU\Software\Wine\Direct3D` `renderer=gdi`를 설정했으나 **그 뒤 실행이 거부되어 효과 미확인.**
- **문제 2 — 파이프 상속:** `start_session` 출력을 파이프로 받으면 셸이 게임 종료 때까지 기다렸다(게임이 도구의 표준 출력을 물려받음). `linux-wine.sh call`은 임시 파일로 받는다(이 방식의 `start_session`은 미검증). MCP 모드에서도 같은 문제로 호스트가 EOF를 늦게 받을 수 있다.
- **다음 작업 (실제 실행이 허용된 뒤):**
  1. `bash analyzeManager/linux-wine.sh call start_session '{"label":"…"}'` → 몇 초 뒤 `capture_state`로 gdi 렌더러에서 화면이 보이는지 확인. 여전히 검으면 Wine 가상 데스크톱(`explorer /desktop=…,1024x768`)이나 X11 창 캡처(XGetImage) 백엔드를 검토한다.
  2. 캡처가 되면 입력(클릭·ESC) 전달과 화면 변화를 확인하고, 아래 Windows 절의 "재확인 대상"을 Linux에서 진행한다.
  3. 파이프 상속: 게임 시작 시 표준 입출력을 넘기지 않는 방법(예: `UseShellExecute=false` + 표준 핸들 리디렉션 후 닫기)을 Windows·Wine 양쪽에서 확인 후 적용.
- 이번 변경 파일(커밋 전): `analyzeManager/SessionStore.cs`, `analyzeManager/tests/SessionStoreTests.cs`, `analyzeManager/tests/mcp_smoke.py`, `analyzeManager/linux-wine.sh`(신규), `analyzeManager/README.md`, `docs/analyze-manager.md`, `LEFT_JOBS.md`

### Windows 실제 실행 검증 (2026-09-29 오후, 사용자 요청으로 중단)

- **실행 상태:** 사용자가 이번 단계의 실제 게임 실행을 허용했다(시스템 `DESKTOP-HJOW`, 지정 예외 시스템 아님). 이후 사용자 요청으로 테스트를 중단했고, 곧바로 `end_session force=true`로 세션 게임을 종료했다. 실행 중인 `Netstorm.exe`가 없고 `originals/`가 변경되지 않았음을 확인했다.
- **실제 게임 테스트는 나중에 한다(사용자 결정).** 실행 중에 사용자가 동시에 마우스를 조작한 경우가 있었다. 그래서 실행 중 화면 변화 가운데 **도구 입력의 결과라고 단정할 수 없는 관찰은 결과에서 뺐다**(아래 "재확인 대상"). 다음에 실행할 때는 사용자 확인을 다시 받고, 실행 중 사용자 입력이 없는 상태에서 진행한다.
- **게임 없는 검증(확실):** Windows Release 빌드 오류/경고 0, 단위 테스트 **18개 통과**(Linux에서 추가한 손상 PNG 테스트 포함), `mcp_smoke.py` 기본 모드 통과. 코드 수정 후에도 다시 통과했다.
- **실제 실행으로 확인된 도구 동작(확실)** — 세션 `20260929T060009857Z-a78447df7657`·`…060326099Z-6e78f68fa740`·`…060406149Z-ac08dc951f43`, 증거는 `extracted/analyzeManager/<세션>/`:
  - Claude Code Bash에서 CLI `start_session`을 호출하고 CLI 프로세스가 끝난 뒤에도 게임 프로세스가 유지되었다. `game_status`·`capture_state`로 이어서 조작할 수 있었다.
  - `mcp_smoke.py --live` **통과**: CLI로 실행 → MCP 재접속·상태 확인, PNG 이미지 콘텐츠 응답, 입력 전달, `wait_for_change` 시간 초과 응답, 관찰 기록, 강제 종료.
  - 일반 `end_session`(WM_CLOSE)으로 메인 메뉴 상태의 게임이 확인 창 없이 종료되었다. `end_session force=true`도 해당 세션 게임만 종료했다.
  - 포커스를 잃었을 때 도구가 입력을 보내지 않고 오류를 반환했다(안전장치 동작).
- **도구 수정(`analyzeManager/WindowsGame.cs`):**
  - 첫 `start_session`이 "게임 창의 포커스를 얻지 못했습니다"로 실패했다. `FocusAsync`에 `AttachThreadInput` 방식과 Alt 키 신호 방식을 차례로 쓰는 전면 전환 재시도를 추가한 뒤 이후 캡처·입력의 포커스 획득은 성공했다. 실패 원인은 Windows의 백그라운드 전면 전환 제한으로 추정한다.
  - `CheckForeground`에 200ms 재확인을 추가했다. 끝내 실패하면 당시 전면 창의 handle·pid·제목을 오류에 남긴다. 이 정보로 마지막 실패 때 전면 창이 VS Code였음을 확인했다.
- **주의(사용자 확인):** 도움말 본문의 파란 글자는 외부 링크다. 도움말에서 (500,300)부터 드래그한 입력이 "Netstorm:HQ's Web Site" 링크 줄 위였고, 사용자가 NetstormHQ 홈페이지가 다른 브라우저로 열렸다고 확인했다. 도움말 창에서는 링크 글자가 없는 빈 영역에서만 클릭·드래그한다.
- **원본 화면에서 확인한 사실(정지 캡처 기준, 확실):**
  - 도구가 준비한 복사본의 시작 화면은 세 세션 모두 같았다. Activision 시작 화면 → 메인 메뉴 위의 "Did You Know?" 팁 창(Prior Tip / Next Tip / No More Tips / OK) → 그 뒤의 "Not Validated" 창(자동 업데이터 검증 실패 안내: 멀티플레이 불가, 싱글·LAN 가능, OK 버튼).
  - **메인 메뉴 버튼 좌표** (1024×768 클라이언트, 정지 캡처에서 버튼 테두리 선(밝기 52)으로 측정): 버튼 75×19px, 가로 간격 79, 세로 간격 23. 윗줄 y 311~329는 Campaign x 356~430, Multiplayer 435~509, Demo 514~588, Help 593~667. 아랫줄 y 334~352는 Edit·Credits·Options·Quit(같은 x). 묶음 가로 중심 ≈ 512. 증거: 세션 `…060406149Z-ac08dc951f43`의 `screens/0fba1e0f784c316746bc38c72b7a6823fb57b6fd1e54ee5cb2e1742fa6e26fc7.png`
  - 도움말 창 "NetStorm Instructions"의 모양: 창 약 (288,40)~(738,390), 본문의 파란 링크 목록(GAME HELP / GAME SUPPORT / GAME INFORMATION), 오른쪽 스크롤바, Back·OK 버튼. 증거: 세션 `…060009857Z-a78447df7657`의 `screens/235ec5d3….png`
- **재확인 대상 (사용자 동시 조작 가능성 때문에 결과로 쓰지 않음):**
  - 메인 메뉴에서 F1이 도움말 창을 여는지. 원본 도움말 메뉴 표기 "General Help - F1"과는 맞지만 이번 실행만으로는 단정하지 않는다.
  - 메인 메뉴에서 데모 전투 화면으로 바뀐 것이 Auto-Demo 자동 시작인지 다른 입력 때문인지.
  - 데모 중 ESC 뒤 화면 맨 위 메뉴 막대와 Game 드롭다운(Restart Demo / Exit Demo / Quit Game)이 보인 것이 ESC 때문인지. 드롭다운 항목 자체는 캡처에 보였다.
  - 팁 창이 본문 클릭·ESC에 반응하지 않는지, 도움말 드래그·스크롤 화살표의 스크롤 양.
- **다음 작업 (사용자가 실제 게임 테스트 재개와 실행을 확인한 뒤):**
  1. 위 재확인 대상을 사용자 입력이 없는 상태에서 다시 확인한다.
  2. 메뉴 흐름 조사: Campaign·Demo·Help·Edit·Credits·Options 하위 화면의 좌표와 전환 기록 → `docs/screens/main-menu.md`, 5단계 "메뉴 흐름도", 9단계 UI 기준. Multiplayer(네트워크)·전체화면 항목은 누르지 않는다.
  3. `analyzeManager` 개선 후보(게임 실행 없이 가능): 복사본 설정에서 `autoDemo`·시작 팁을 끄는 선택 옵션, 실행 중 사용자 입력과 섞이지 않도록 안내 문구 추가.
- 이번 변경 파일(커밋 전): `analyzeManager/WindowsGame.cs`, `docs/analyze-manager.md`, `LEFT_JOBS.md`

### 이전 인수인계 (2026-09-29 오전)

**실행 확인 규칙(사용자 요청):** 일반 시스템에서 이 자동 탐험 프로그램으로 실제 게임을 구동하려면, 실행 전에 개발자에게 목적과 필요성을 알리고 명시적인 확인을 받아야 한다. CLI/MCP `start_session`, `mcp_smoke.py --live`, 검증용 직접 실행에 모두 적용한다. 일반 작업 지시나 과거 실행 이력을 새로운 실행의 확인으로 간주하지 않으며, 확인받은 범위 안에서만 실행한다. **예외:** IP `10.0.0.15`, 호스트명 `vm-debian-codex`로 지정된 시스템에서는 개발자의 게임 구동 확인을 받지 않아도 된다(`AGENTS.md`). 사용자가 이번 지시에서 지정 시스템의 확인 없는 게임 실행을 명시적으로 허용했다.

**실행 상태:** 앞서 사용자가 실제 게임 실행 테스트 중단을 지시했고, 중단 요청 뒤 실행 중인 `Netstorm`이 없음을 확인했다. 이후 사용자가 지정 시스템의 확인 없는 게임 실행을 명시적으로 허용했으므로, **지정 시스템에서는 이 중단 지시를 해제한 것으로 적용한다.** 그 밖의 시스템에서는 기존 중단 지시가 유지되며, 사용자의 별도 재개 지시가 필요하다. 이번 문서 수정에서는 원본/복사본을 실행하지 않았다.

### Linux/Wine 실행 가능성 확인 (2026-09-29, 사용자 요청으로 이 단계에서 중단)

- 확인 시스템: `vm-debian-codex`(Debian 13, Linux x86_64), GUI 세션 Wayland/X11 `DISPLAY=:1`, X11 화면 1600×900. Wine 10.0과 32비트 Wine 구성 요소가 설치되어 있다. Wine 실행과 X11 조회는 명령 샌드박스 밖에서 성공했다.
- Windows용 .NET 10 런타임과 기존 분석 도구 빌드는 없었다. NuGet 연결을 확인한 뒤 `dotnet restore analyzeManager/AnalyzeManager.csproj -r win-x86 -p:NuGetAudit=false`와 `dotnet publish analyzeManager/AnalyzeManager.csproj -c Release -r win-x86 --self-contained true --no-restore -o /tmp/netstorm-analyze-winx86 -p:NuGetAudit=false`가 성공했다. 배포 출력은 `/tmp/netstorm-analyze-winx86/`(약 114 MB)에 있으며 Git에 포함되지 않는다.
- 사용자 소유의 임시 Wine 접두 경로 `/tmp/netstorm-wine-check/`(win32)에서 `wine cmd /c ver`와 분석 도구 `--help`가 성공했다. 저장소를 Wine의 `Z:\home\hjow\Workspaces\git\NetstormReborn` 경로로 전달한 `list_sessions`는 `RejectReparse`가 `Z:\` 드라이브 루트를 링크로 판정해 거부했다. 접두 경로의 실제 `C:\netstorm-probe` 디렉토리에 `Netstorm.exe`만 복사한 가짜 저장소에서는 `list_sessions`가 `sessions: []`로 성공했다.
- **검증 범위:** Linux에서 Wine을 통한 도구 프로세스 시작과 게임 없는 세션 목록 조회까지 가능하다. 원본 게임, `start_session`, 실제 화면 캡처·입력, MCP 통신은 실행/검증하지 않았다. 현재 원본 저장소 경로를 그대로 넘기면 `Z:\` 링크 검사에 막히며, `C:\netstorm-probe`는 설정·자산이 없는 가짜 저장소다. 따라서 Linux에서 원본 게임 분석 도구의 전체 사용 가능 여부는 아직 확정되지 않았다.
- **→ 2026-09-29 저녁 이어서 진행함** (위 "Linux/Wine 분석 도구 실행" 절: 링크 검사 수정, 실제 실행 1회, 캡처 검은 화면 문제). 당시 기록: 사용자가 재개를 요청하면 Wine 경로의 링크 검사 정책을 검토하거나 Wine C 드라이브의 독립 원본 복사본을 준비하고, 지속 실행되는 Wine 프로세스에서 세션 수명·창 캡처·입력·MCP 이미지를 순서대로 확인한다. `/tmp`의 접두 경로와 배포물은 임시 산출물이므로 다음 환경에서 다시 만들어야 할 수 있다. 이번 요청에 따라 여기서 멈추며 게임을 실행하지 않는다.

### 구현한 내용

- `TODO.md`의 AI용 원본 게임 탐험 도구를 `analyzeManager/`에 구현했다. 아직 작업 전체 완료로 보지 않는다. 사용자 작성 `TODO.md`는 그대로 유지했다.
- C#/.NET 10 Windows 독립 프로젝트. CLI와 공식 C# SDK 2.2.0 기반 stdio MCP가 같은 엔진을 호출한다. 게임 솔루션에는 추가하지 않았다.
- 도구 8개: `list_sessions`, `start_session`, `game_status`, `capture_state`, `game_input`, `wait_for_change`, `record_observation`, `end_session`.
- 원본 전체를 `extracted/analyzeManager/<sessionId>/game/`에 복사한 뒤 복사본의 options/setup 설정만 수정: 창 모드 1024×768, `InstallDir` 변경. 원본 파일을 수정하는 경로는 만들지 않았다.
- PID·시작 시각·exe 경로로 관리 대상 확인, 파일 잠금으로 동시 조작 제한, 클라이언트 물리 픽셀 입력, 관심 영역 캡처·변화 대기, 입력 전후 기록, 취소 시 키/버튼 해제 구현.
- PNG SHA-256 중복 재사용, JSONL/Markdown 약 4 MB 분할, 파일당 50,000,000바이트 미만, 세션당 PNG 500개/200 MB 및 이벤트 10,000개 한도. 원본 게임이 자체 생성하는 파일까지 크기를 강제하지는 않는다.
- [사용법·구조·제약](docs/analyze-manager.md), [도구 안내](analyzeManager/README.md), [MCP 설정 예시](analyzeManager/examples/mcp-settings.json)를 작성했다. 실제 사용자 MCP 설정은 변경하지 않았다.

### 검증된 것 / 검증하지 못한 것

- `dotnet build analyzeManager/AnalyzeManager.csproj --no-restore`: Debug 빌드 성공, 오류 0·경고 0.
- `dotnet test analyzeManager/tests/AnalyzeManager.Tests.csproj`: **17개 통과**. 가짜 원본을 사용하는 파일 보존·로그/이미지·입력 경계 검사이며 게임은 실행하지 않는 테스트다.
- **2026-09-29 게임 없는 후속 검증:** Release 빌드 오류/경고 0, Release 단위 테스트 17개 통과. `mcp_smoke.py` 기본 모드로 MCP `initialize`(프로토콜 2025-03-26), 도구 8개 스키마, `list_sessions`, 잘못된 세션의 오류 응답, EOF 종료를 확인했다. 서버와 테스트는 게임을 실행하지 않았다. `start_session` 도구 설명과 CLI 도움말에 일반 시스템의 개발자 확인 조건과 지정 시스템 예외를 반영했고, Release 재빌드 및 게임 없는 MCP 기본 검사를 다시 통과했다.
- **2026-09-29 Linux 정적 후속 작업:** `SessionStore.StoreFrame`이 같은 해시 이름의 손상된 PNG를 재사용하던 문제를 수정했다. 새 PNG는 임시 파일로 완성해 최종 이름으로 옮기며, 재사용 시 크기와 SHA-256을 검사한다. 손상 파일 회귀 테스트 1개를 추가했다. 이 Linux 시스템에서는 NuGet 패키지 다운로드가 완료되지 않아 Windows 대상 프로젝트의 변경 후 빌드·테스트를 실행하지 못했다. 대신 `SessionStore.cs`를 패키지 의존성 없는 .NET 10 임시 검증 프로젝트에 직접 연결해 정상 중복, 동일 길이 손상, 잘린 파일 검출을 통과했다. Windows 대상 전체 테스트와 MCP 검사는 이 변경 이후 재검증 필요하다.
- 실제 복사본을 한 번 실행하여 **1024×768 창과 Activision 시작 화면 PNG**를 저장·육안 확인했다. 세션 ID: `20260929T020751113Z-1ca6ff22ee2e`.
  - 증거: `extracted/analyzeManager/20260929T020751113Z-1ca6ff22ee2e/screens/3eb1594375aa2b0498cb594a3d3fdb4d3e955305bc1d634faa5b2031bf68294b.png`
  - 보고서: 같은 세션의 `report.md`. 실행 파일 SHA-256과 PID/시작 시각은 `session.json`에 있다. 결과는 Git 제외 경로에 보존했다.
- 후속 CLI `capture_state`는 “이 세션의 게임이 종료되었거나 프로세스 식별 정보가 다릅니다.” 오류를 반환했고 실제 프로세스도 없었다. **종료 원인은 미확정**이다. 명령 실행기의 자식 프로세스 정리 여부도 재개 후 확인할 후보이며 현재 확정해서 쓰면 안 된다. manifest의 `phase=running`은 마지막 기록 상태이고 실제 상태는 `game_status.running`으로 판단한다.
- 실제 마우스·키보드 반응, CLI 호출 간 게임 유지, **MCP 이미지 전송**은 미검증이다. `analyzeManager/tests/mcp_smoke.py --live`는 실행하지 않았다. 실제 MCP 호스트 등록도 미실시다.
- 중단 요청 이후 실제 게임은 재실행하지 않았다. 2026-09-29 후속 작업에서는 코드의 실행 확인 안내, Release 빌드·단위 테스트, **게임 없는** MCP 프로토콜 검사와 문서를 진행했다. 실제 입력과 MCP 이미지 전달 검증이 끝났다고 보고하면 안 된다.

### 다음 작업

1. 실제 GUI 테스트는 지정 시스템에서 별도 실행 확인 없이 진행할 수 있다. 그 밖의 시스템에서는 기존 중단 지시를 유지하고, 재개 지시를 받은 뒤 매 실행 전에 목적과 필요성을 설명해 명시적인 확인을 받는다. `analyzeManager`는 Windows 전용이므로 지정 시스템이 Linux인 경우 실행 환경의 기술적 준비는 별도로 필요하다.
2. `mcp_smoke.py` 기본 모드의 프로토콜 검사는 완료했다. `--live`는 CLI 실행·MCP 캡처·입력·메모·강제 종료까지 수행한다. 지정 시스템에서는 실행 가능하며, 그 밖의 시스템에서는 위 규칙을 따른다.
3. Windows GUI 실행 환경이 준비되면 지속 실행되는 MCP 서버/상위 프로세스 안에서 CLI 실행 후 게임 생존을 확인하고, 이번 후속 캡처 실패 원인을 해결한다. 단일 `start_session` 성공만으로 장기 사용 가능하다고 결론 내리지 않는다.
4. 실제 입력 전후 캡처, ROI 변화 대기, MCP PNG 응답, 한국어 보고서 링크, 정상 종료 및 종료 확인 창 처리를 검증한다. 성공하면 [도구 문서](docs/analyze-manager.md)의 검증 범위를 갱신한다.
5. 원본 메뉴/도움말/캠페인 분석은 그 뒤 도구를 사용해 수행한다. 기존 6절의 게임 규칙 동적 확인 항목들은 이번에 완료하지 않았다.

이번 작업 파일: `analyzeManager/` 신규 소스·테스트·예시·README, `docs/analyze-manager.md` 신규, `LEFT_JOBS.md`·`AGENTS.md` 갱신. 시작 시 이미 사용자 수정이 있던 `TODO.md`는 보존했다. 커밋은 하지 않았다.

## 0. 진행 현황 요약

| 단계 | 내용 | 상태 |
|---|---|---|
| 1 | 설계 (본 문서 작성: 원본 조사, 단계·작업 정의) | ✅ 완료 (2026-09-27) |
| 2 | 개발 환경 구축 | ✅ 완료 (2026-09-27) — C# + MonoGame(net10.0), 솔루션·테스트·CI |
| 3 | 원본 분석 — 데이터 포맷 | 🔶 거의 완료 (TAFF·셰이프·팔레트·.type·설정(조회·치환 규칙 포함)·번역 체계·파일 조회 순서·HLP 본문·`.fort` 컨테이너/오브젝트 완료 / `.fort` 일부 섹션 남음) — [docs/formats/](docs/formats/README.md) |
| 4 | 원본 분석 — 실행 파일(게임 로직) | 🔶 착수 (전체 디컴파일·모듈 맵 완료) |
| 5 | 원본 분석 — 플레이 영상 | 🔶 착수 (스크린샷 42장 목록·관찰 정리, 로컬 영상 4개 형식·화면 영역 확인, 프레임 추출·애니메이션 간격 측정 도구, Dissolved Alliance! 맵 대조·시작 카메라 규칙, 애니메이션 속도 측정 완료 / 미션별 관찰 노트 미착수) — [docs/videos/](docs/videos/README.md) |
| 6 | 자산 로더 / 개발용 뷰어 | 🔶 진행 중 (TAFF·팔레트·셰이프·.type·.cfg·TTC·.fort·가상 파일 시스템·설정 치환·번역표·미션 스크립트 로더·스프라이트 탐색(동작 재생·팔레트·속성) 완료) |
| 7 | 엔진 코어 (플랫폼 계층) | 🔶 착수 (2026-09-29: 창·전체화면·16:9/16:10/4:3 화면 계층, 가장자리 스크롤, 표시 설정 저장 완료 / 팔레트 방식·입력·오디오·고정 틱·로깅 남음) |
| 8 | 게임 월드 / 규칙 구현 | ⬜ 대기 |
| 9 | UI · 미션 스크립트 · 튜토리얼 | ⬜ 대기 |
| 10 | AI | ⬜ 대기 |
| 11 | 캠페인 · 저장(fort) | ⬜ 대기 |
| 12 | 다국어 지원 | ⬜ 대기 (설계는 초기부터 반영) |
| 13 | 멀티플레이 | ⬜ 대기 |
| 14 | 패키징 · 배포 (Windows 우선, Linux 후순위) | ⬜ 대기 |
| 15 | 검증 · QA | ⬜ 상시 |

단계 번호는 대략적인 순서이며, 3~5 단계(분석)는 병행 가능하다. 6 단계 이후는 분석 결과가 나오는 대로 점진적으로 진행한다.

---

## 1. 원본 조사 결과 (1단계 조사 + 3단계 분석으로 갱신)

`originals/` 현재 파일 1,389개, 파일 크기 합계 164,486,189바이트(2026-09-29 사용자가 추가한 `help/manual.pdf` 포함).

### 1.1 실행 파일 / 라이브러리

| 파일 | 내용 |
|---|---|
| `Netstorm.exe` (1.5MB) | x86 32bit PE, MSVC 빌드, **패킹되지 않음**(.text 약 1MB). 빌드 타임스탬프 2006-03 → 원본(1997) 이 아닌 **Ticonderoga Entertainment 비공식 패치 빌드(10.7x)**. 임포트: DDRAW, DSOUND, WINMM, WSOCK32, smackw32, GDI32, COMCTL32, ADVAPI32, SHELL32, VERSION, ole32, mscoree. 문자열에 `TAFF v%d.%d`, `_shapes.shp`, `*.tarc`, `IPX`, `SPX/IPX` 존재 |
| `NSENGLISHRES.DLL` | **설치·진단 프로그램용** 문자열 24개·다이얼로그 13개 (게임 UI 문자열 아님). 게임 UI 문자열은 exe 에 영어로 하드코딩되어 있고 `xlat.<언어>` 로 번역된다 → [xlat.md](docs/formats/xlat.md) |
| `Smackw32.dll` | Smacker 동영상 코덱. 단, `movie/` 폴더는 비어 있음 |
| `R.exe`, `unpack.exe`, `bzip2.exe`, `TMaker.exe` | 보조 도구(업데이터/압축 해제/요새 생성기로 추정). 게임 본체 분석 대상은 아님 |
| `PatchFixs.txt` | 패치 변경 이력(892줄). 원본 대비 **변경된 규칙·버그 수정 목록** → 어느 버전 동작을 기준으로 할지 결정 시 참고 |

### 1.2 데이터 파일

| 파일/패턴 | 확인된 내용 | 비고 |
|---|---|---|
| `netstorm.tarc` (1.1MB) | TAFF 아카이브, 엔트리 246개. 데이터는 XOR 키 `mydoghasfleas` 로 인코딩 → **해독 완료** ([taff.md](docs/formats/taff.md)) | `.type`(오브젝트 정의), 공식 미션, 번역 테이블(`xlat.*`), 용어표(`config.*`) 포함 |
| `d/_shapes.shp` (5.6MB) | Miles VFX 셰이프 블록 116개(타입당 1개), 프레임 3,783개, 행 단위 RLE → **해독 완료** ([shp.md](docs/formats/shp.md)) | 모든 스프라이트. 블록 순서 = exe 의 타입 로딩 순서 |
| `d/*.COL` | Autodesk Animator Pro 팔레트 (8바이트 헤더 + 256×RGB) | **`GIFCLOUD.COL` = 게임 팔레트** (`setup.cfg` 지정). `TITLE*.COL` = 타이틀 화면. 나머지 유닛 이름 팔레트는 그래픽 제작용으로 추정 |
| `d/!color.dat` | 256바이트 색상 변환 테이블 9개 (항등·검정·단색·어둡게 등) | 개별 용도는 4단계에서 확인 ([config.md](docs/formats/config.md)) |
| `d/!*.chfnt` (18개) | Windows GDI 글꼴(Arial 등)로 게임이 **실행 중에 만든 캐시** (없으면 재생성) | 라틴 문자 전용 → 한국어는 `fonts/` 의 D2Coding 사용 (1.5절, [chfnt.md](docs/formats/chfnt.md)) |
| `d/*.english`, `*.german` 등 (약 580개) | 미션 스크립트. INI 유사 섹션(`[Header]`, `[A.]`, `[Succeeded]`, `[Failed]`, `[aiNPriestDead]`, `[aiNTempleHalfDead]`, `[@1]` …) + `key = value` + HTML 부분집합(`<h2>`, `<p>`, `<i>`) + 명령(`$Button=`, `$Timeout=`, `$Menu=`, `$Checked=`, `$OnExit=`, `<$Config,…>`) | 팬 제작 미션 다수 포함. 오타(`$Buton=`, `$button=`)가 섞여 있어 **관대한 파서** 필요 |
| `d/*.fort` (431개) | `'F'` + 플래그 + 길이 접두 섹션 35개(이름 고정). 월드 16×16 청크의 오브젝트 레코드 → **해독 완료** (일부 섹션 내부 제외) | 섬/요새·미션 맵 ([fort.md](docs/formats/fort.md)) |
| `d/*.gif` | 팬 제작 이미지로 추정 | 우선순위 낮음 |
| `d/options.cfg`, `setup.cfg`, `!rootservers.dat` | 설정 / 온라인 서버 목록. `.cfg` 는 TAFF 와 같은 키로 XOR 인코딩 → **해독 완료** | `tools/nscfg.py` 로 읽기·수정 가능. **화면 해상도 설정 `SCREENW/SCREENH` = 1024×768** (640×480 고정 아님) |
| `music/*.mus` (9개) | 확장자만 다른 **RIFF WAV** (PCM 16bit, 스테레오, 22,050Hz) | 그대로 재생 가능 |
| `sound/*.wav` (218개) | 표준 PCM WAV 효과음 (대부분 22,050Hz 8bit 모노) | 그대로 재생 가능 |
| `help/*.HLP` | WinHelp 도움말 (GAME, HELP, VOCAB, VENDOR, README) | 유닛·규칙·용어 설명 → 규칙 분석의 1차 자료 |
| `help/manual.pdf` | 사용자가 외부 사이트에서 확보해 추가한 구버전 공식 매뉴얼 PDF | [자료 목록](docs/sources/README.md), [조작·화면·비용 선별 대조](docs/sources/pdf-manual.md). `GAME.HLP` 전체와의 대조는 남음 |

### 1.3 주의 사항

* **원본 자산 재활용 가능**: 개발사 지원이 오래전에 끊겨 저작권 행사가 사실상 없으므로, 원본 그래픽·사운드·미션·도움말 등 **기존 파일을 필요 시 그대로 재활용**한다. 대체 자산을 새로 만들 필요는 없다.
  * `originals/`(약 310MB, 1,388개)는 **2026-09-28 사용자 커밋("오리지널 빌드")으로 git 저장소에 포함**되었다 (`.gitignore` 의 `/originals/` 제외 규칙을 주석 처리). 이전의 "커밋하지 않는다" 방침은 폐기.
    * 결과: 저장소를 받은 환경(CI 포함)에서도 원본 검증 테스트가 건너뛰지 않고 실행된다. `extracted/` 는 여전히 커밋하지 않는다.
  * 원본 플레이 영상 `playingVideos/` 는 용량이 커서 **git 에 커밋하지 않는다** (AGENTS.md, `.gitignore` 의 `/playingVideos/**`). 새 환경에는 따로 복사해 두어야 한다 (1.6절).
    * 주의: 원본을 실행하면 게임이 `d/options.cfg` 를 저장하고 `d/!*.chfnt` 캐시를 다시 만들 수 있으므로, 분석용 실행 뒤 의도하지 않은 원본 파일 변경이 커밋되지 않게 `git status` 로 확인한다.
  * 배포 패키지에는 필요한 자산을 동봉할 수 있다(14단계).
  * 게임 엔진은 원본 포맷을 직접 읽는 구조로 만든다(자산 변환 없이 원본 폴더 또는 동봉 폴더에서 로드).
* (Linux 대응 시) Linux 는 파일 시스템이 **대소문자를 구분**한다. 원본 데이터 파일명은 대소문자가 뒤섞여 있고(`Battle1.fort` / `battle1.english`), 아카이브 내부 경로는 `\` 구분자를 쓴다 → 파일 탐색은 반드시 대소문자 무시 + 경로 구분자 정규화 계층을 거친다.
* 원본 텍스트 파일 인코딩은 Windows-1252(영어/독일어)로 추정 → 로드 시 UTF-8 로 변환한다. 새로 만드는 문서·코드는 UTF-8.

### 1.4 원본 실행 환경 (AGENTS.md 기준)

* 원본 `Netstorm.exe` 는 현재 **Windows 10/11 에서 제한적으로 구동 가능**하며, 실행 시 **창 모드**로 동작한다.
* 게임 옵션에서 전체화면으로 전환하면 전환 자체는 되지만, **게임 종료 후 재실행 시 오류가 발생**한다. 오류 후 한 번 더 실행하면 다시 창 모드로 실행된다.
  * 원본을 분석용으로 실행할 때는 **전체화면 전환을 하지 않는다**.
  * 전체화면 플레이 자체는 가능하다: 로컬 플레이 영상(`playingVideos/`)은 1024×768 전체화면으로 플레이하며 녹화한 것이다 (문제는 전체화면 설정이 저장된 뒤의 **재실행**).
  * **분석 결과(정적)**: 설정은 `d/options.cfg` 의 `startInFullScreen` 에 저장된다. 시작 시 이 값이 참이면 `workingFullScreenFlags` 모드로 화면을 초기화하는데, 실패하면 초기화 함수가 그대로 반환되어 게임이 뜨지 않는다 → DirectDraw 독점 전체화면 초기화 실패로 추정. 자세한 내용은 [config.md](docs/formats/config.md) "전체화면 재실행 오류".
  * **복구 방법(추정)**: `python tools/nscfg.py set originals/d/options.cfg startInFullScreen 0` (원본은 `.bak` 으로 백업됨). **현재 `originals/d/options.cfg` 는 `startInFullScreen = "1"` 상태**라 다음 실행 때 오류가 날 수 있다.
  * 오류 후 창 모드로 돌아오는 과정은 동적 분석으로 확인한다. 클론에서는 이 버그를 재현하지 않는다.
* 원본을 직접 실행할 수 있으므로 다음이 가능하다.
  * **동적 분석**: x64dbg 등으로 실행 중인 원본의 메모리·함수 호출·파일 접근 관찰 (정적 분석 결과 검증)
  * **직접 대조**: 영상뿐 아니라 원본 화면 캡처·수치 측정과 클론 결과를 나란히 비교

### 1.5 한국어 글꼴 (AGENTS.md 기준)

* 한국어 표시가 필요한 경우 **`fonts/D2Coding-Ver1.3.2-20180524-all.ttc`** 를 사용한다.
* TrueType Collection(TTC)이며 글꼴 4종을 담고 있다. 로드할 때 **face 인덱스**를 지정해야 한다.

  | 인덱스 | 글꼴 |
  |---|---|
  | 0 | D2Coding (일반) |
  | 1 | D2Coding Bold |
  | 2 | D2Coding ligature |
  | 3 | D2Coding ligature Bold |

* 라이선스: SIL Open Font License 1.1 → 게임과 함께 배포 가능 (배포 시 OFL 고지 동봉).
* **monospace(고정폭)** 글꼴이다. 원본 UI 는 Arial(가변폭) 기준으로 배치되어 있으므로, 같은 문장이라도 폭이 달라진다 → 텍스트 영역은 글꼴 메트릭으로 측정해 줄바꿈·크기를 정해야 한다(좌표 하드코딩 금지).
* 영어 텍스트는 원본 느낌을 살리기 위해 원본 `.chfnt` 비트맵 폰트 사용을 우선 검토하고, 한국어(및 원본 폰트에 없는 문자)는 D2Coding 으로 대체하는 **폰트 폴백 체인**을 둔다. 영어도 D2Coding 으로 통일할지는 구현 시 화면을 보고 결정.

### 1.6 외부 참고 자료 (AGENTS.md 기준)

* **플레이 영상 — 로컬** (`playingVideos/`, 2026-09-28 AGENTS.md 추가, git 제외): 원본 게임을 최대 해상도 **1024×768 풀스크린**으로 두고 플레이하며 녹화한 영상 4개. 파일·미션 대응은 5단계 표 참고
  * 모니터가 16:9 여서 4:3 게임 화면 **좌우에 같은 크기의 검은 여백(레터박스)** 이 들어가 있다 → 좌표를 잴 때는 가운데 4:3 영역을 먼저 잘라 1024×768 기준으로 환산한다.
  * 형식 (2026-09-28, 4개 모두 `ffprobe` 확인): 영상 **AV1 1920×1080 60fps**, 소리 AAC. 길이·크기와 추출 방법은 [docs/videos/README.md](docs/videos/README.md). 이 PC 의 FFmpeg 에 AV1 디코더 `libdav1d` 가 있어 `ffprobe`·`ffmpeg` 로 그대로 처리 가능 (Python OpenCV 등은 AV1 미지원일 수 있으므로 프레임 추출은 `ffmpeg` 로 한다)
  * 계산상 게임 화면은 **가운데 1440×1080** (좌우 여백 각 240px), 배율 1080/768 = **1.40625배**(정수 아님 → 픽셀 경계가 흐려짐). 영상 좌표 → 게임 좌표: `x = (영상x − 240) / 1.40625`, `y = 영상y / 1.40625`. 실제 여백 경계는 첫 프레임에서 한 번 측정해 확인한다
  * 방송 중 녹화라 **사람 음성이 함께 녹음**되어 있다 → 효과음·음악 재생 시점을 소리로 판단할 때는 음성과 섞인 점에 주의한다 (원본 소리 자체는 `sound/`·`music/` 파일이 기준).
  * **평소보다 밝게 촬영**되었다 (AGENTS.md, HDR 설정 문제로 추정) → 영상의 색은 원본 팔레트와 다르므로 색 비교에는 쓰지 않는다 (스크린샷 사용).
* **플레이 영상 — YouTube**: 튜토리얼, 캠페인 1-1~4, 1-5, 1-6, 2-1, 2-2, 2-3, **3-2**(2026-09-29 AGENTS.md 추가) — 주소는 5단계 표 참고. 캠페인 2-x·3-2 는 로컬 영상이 없고 YouTube 에만 있다 (필요 시 `yt-dlp` 로 `extracted/videos/` 등에 받는다)
* **게임 플레이 방법 소개 홈페이지 (한국어)**: https://hjow.duckdns.org/netstorm/learnmain.htm (2026-09-28 AGENTS.md 에 추가)
  * 하위 페이지 (2026-09-28 목차 확인):

    | 페이지 | 주제 | 주로 참고할 단계 |
    |---|---|---|
    | [learninstall.htm](https://hjow.duckdns.org/netstorm/learninstall.htm) | 설치 및 시작, 메뉴, 옵션 | 9단계(메뉴·옵션 화면) |
    | [learninterface.htm](https://hjow.duckdns.org/netstorm/learninterface.htm) | 화면 구성·UI 설명 | 5단계(화면 관찰), 9단계(HUD) |
    | [learnmineral.htm](https://hjow.duckdns.org/netstorm/learnmineral.htm) | 광물 자원 | 4·8단계(경제) |
    | [learnresource.htm](https://hjow.duckdns.org/netstorm/learnresource.htm) | 자원 수집·관리 (Storm Power 등) | 4·8단계(경제) |
    | [learntemple.htm](https://hjow.duckdns.org/netstorm/learntemple.htm) | 신전 시스템 | 4·8단계(신전·기술) |
    | [learnpriest.htm](https://hjow.duckdns.org/netstorm/learnpriest.htm) | 사제(프리스트)·고위 사제 | 4·8단계(승패 조건·프리스트) |
    | [learnmovie.htm](https://hjow.duckdns.org/netstorm/learnmovie.htm) | 플레이 영상 | 5단계 |
    | [unitmain.htm](https://hjow.duckdns.org/netstorm/unitmain.htm) | 전체 유닛 목록·설명 | 4·8단계(유닛·건물 규칙), [types.md](docs/gameplay/types.md) 수치와 대조 |

  * 활용 방침: 게임 규칙의 **1차 근거는 원본 데이터·실행 파일 분석**이고, 이 홈페이지는 동작 이해와 교차 검증용으로 쓴다. 내용이 분석 결과와 다르면 원본 동작을 우선하고 차이를 문서에 남긴다.
  * **한국어 용어 참고 자료**로도 쓴다 (12단계 한국어 번역 시 유닛·건물·자원 이름의 기존 한국어 표기 확인).
* **원본 스크린샷** (AGENTS.md, `screenShots/`, 2026-09-28 기준 42장): 외부 캡처 도구로 찍어 작업표시줄·창 테두리·바깥 영역이 일부 포함된다. 메인 메뉴 계열은 `mainMenu - …`, 미션 화면은 `미션 이름 - 상황` 형식의 파일 이름이다. 모두 원본 1024×768 창 모드.
  * **전체 목록과 캡처별 관찰: [docs/screens/README.md](docs/screens/README.md)** — 메인 메뉴·Campaign(하위 6개)·Multiplayer·Demo·Help·Edit·Credits·Options(하위 4개), The War Begins!(브리핑·진행 9장·정보 창 3장·게임 메뉴·승리·패배 2장), Dissolved Alliance!(시작 + 섬별 2장), 기존 튜토리얼·캠페인 시작 화면
  * 클라이언트 영역(1024×768)의 캡처 내 시작 위치는 파일마다 조금씩 다르므로 제목 표시줄·테두리 경계를 측정해 잘라 쓴다.
  * 원본 카메라는 **위치 이동만 되고 높이·확대 배율은 바꿀 수 없다** (사용자 확인). 그래서 섬 3개가 있는 Dissolved Alliance! 는 한 화면에 담을 수 없어 시점을 옮겨 섬마다 찍었다.

### 1.7 결과물 화면 요구사항 (2026-09-28 AGENTS.md 추가)

* **풀스크린 모드**를 지원해야 한다 (원본처럼 재실행 오류가 나서는 안 된다 — 1.4절).
* **화면비 16:9, 16:10, 4:3** 을 지원해야 한다. 원본 기본 해상도 1024×768 과 메인 메뉴 타이틀 그림(640×480)은 4:3 이다.
  * **원본은 4:3 해상도만 지원했다** (AGENTS.md·사용자 확인): Options → Resolution 하위 메뉴가 `640 by 480`, `800 by 600`, `1024 by 768` 세 가지뿐 (`mainMenu - Options - Resolution.png`). 따라서 16:9·16:10 은 원본 참고 자료가 없는 **새 기능**이며 동작은 클론에서 설계한다.
  * 원본은 **1024×768 이 최대 해상도**이며, 16:9 모니터에서 풀스크린으로 실행하면 4:3 화면 좌우에 같은 크기의 검은 여백이 생긴다 (로컬 플레이 영상에서 확인). 즉 원본의 와이드 모니터 대응은 "4:3 가운데 + 좌우 여백" 이다.
  * 원본 카메라는 확대·축소가 없고 해상도가 곧 보이는 맵 범위다 (640×480 보다 1024×768 이 더 넓게 보임). 와이드 화면에서 맵을 옆으로 더 보여 주는 방식이 원본 해상도 선택의 동작과 가장 가깝다.
  * **결정됨(2026-09-29, 사용자): 와이드 화면(16:9·16:10)에서는 "시야 확장"으로 동작한다.** 원본과 같은 픽셀 배율을 유지한 채 맵을 옆으로 더 보여 준다 (논리 높이 = 고른 원본 해상도의 높이, 논리 폭이 화면비에 맞춰 늘어남). **4:3 레터박스(좌우 검은 여백) 화면은 게임 동작으로 쓰지 않는다.** 구현은 [map-viewer.md](docs/map-viewer.md) "화면 설정". 멀티플레이에서 해상도에 따른 시야 차이를 허용할지는 13단계에서 정한다.
  * 메뉴·대화상자는 원본처럼 가운데 배치하고 남는 영역은 배경(구름)으로 채우는 방식이 원본 캡처와 맞는다.
  * 참고: 코드에는 개발용 `WideScreenMode.Letterbox`(F10, `--wide letterbox`)가 남아 있다. 원본 캡처와 1024×768 로 겹쳐 볼 때만 쓰며, 게임 옵션으로 노출하지 않는다 (제거할지는 미정).
  * 사이드바·미니맵 등 HUD 는 화면 가장자리 기준으로 배치하고, 좌표를 4:3 기준으로 하드코딩하지 않는다.
* **풀스크린에서 마우스 커서를 화면 끝에 대면 화면(카메라)이 이동**해야 한다 (원본도 지원). 원본은 Options 메뉴의 **`Edge Scroll in Fullscreen`** 선택 항목으로 켜고 끈다 (캡처에서는 켜짐). 이동 속도·가장자리 폭은 원본 동작을 측정해 맞춘다 (4·5단계) → **2026-09-29 exe 분석·구현 완료** ([edge-scroll.md](docs/exe/edge-scroll.md)): 창 테두리가 없을 때(전체화면 또는 바탕화면과 같은 크기의 창)만 동작하므로 일반 창 모드에서는 동작하지 않는다.
* 원본 Options 메뉴 전체(설정 화면 구현 기준): Direct Draw / Full Screen, Resolution >, Sound On, Play Music, Wind Noise, Speaker Swap L/R, Sound Effect Volume >(1~5), Music Volume >(1~5), Edge Scroll in Fullscreen, Auto-Demo, Tell Tips at Startup, Pause - Shift-F9, Pass Server Diagnostic

---

## 2. 기술 스택 (**확정: C# + MonoGame**, 2026-09-27 사용자 결정)

| 영역 | 선택 | 비고 |
|---|---|---|
| 언어 / 런타임 | **C#** / .NET (MonoGame 공식 템플릿의 대상 프레임워크를 따름, 현재 net8.0 LTS) | 이 PC 에는 .NET SDK 9.0.318 설치됨 (net8.0 대상 빌드 가능) |
| 게임 프레임워크 | **MonoGame 3.8.x — DesktopGL 플랫폼** | OpenGL 기반이라 **같은 코드로 Windows 10/11 과 Linux 모두** 지원 (WindowsDX 는 Windows 전용이라 사용하지 않음) |
| 빌드 | `dotnet` CLI + 솔루션(`.sln`) / SDK 스타일 `.csproj` | Visual Studio 2022, VS Code(C# Dev Kit), Rider 모두 사용 가능 |
| 렌더링 | 원본 자산은 Content Pipeline 을 거치지 않고 **런타임에 직접 로드**. 8bit 인덱스 프레임 → 팔레트 적용 방식은 (a) CPU 에서 RGBA 변환 후 `Texture2D.SetData` 또는 (b) 인덱스 텍스처 + 팔레트 텍스처 + 픽셀 셰이더 중 7단계에서 결정 | 해상도 가변(원본 설정 `SCREENW/SCREENH`, 기본 1024×768) + `RenderTarget2D` 비율 스케일링. 원본은 640×480 고정이 아니므로 해상도별 UI 배치 확인 필요. **화면비 16:9·16:10·4:3 지원 필수**(1.7절) |
| 폰트 | **FontStashSharp** (런타임 TTF 래스터화) + **D2Coding**(`fonts/`) | MonoGame 기본 `SpriteFont` 는 빌드 시 글자를 미리 구워야 해서 한글(1만 자 이상)에 부적합. D2Coding 은 TTC 이므로 face 인덱스 지정 가능 여부를 확인하고, 불가하면 필요한 face 만 TTF 로 분리해 사용 |
| 오디오 | MonoGame `SoundEffect`(효과음, PCM WAV 직접 로드) + `DynamicSoundEffectInstance`(음악 스트리밍) | 원본 오디오는 모두 표준 PCM WAV. 음악 파일이 18~20MB 이므로 스트리밍 필요 |
| 네트워크 | **LiteNetLib** (UDP) 또는 `System.Net.Sockets` | 결정론적 락스텝 구현. 13단계에서 확정 |
| 설정 / 번역 파일 | `System.Text.Json` (설정) + 원본 `xlat` 형식 호환 번역 파일(UTF-8) | 원본 번역 체계 재활용 ([xlat.md](docs/formats/xlat.md)) |
| 테스트 | **xUnit** | 포맷 로더·규칙·결정론 테스트 |
| 배포 | `dotnet publish` self-contained (win-x64 / linux-x64), 단일 파일 옵션 검토 | 사용자 PC 에 .NET 설치 불필요 |
| 분석/추출 도구 | Python 3.13 (기존 `tools/` 유지) | 포맷 리버싱·검증용. 게임 본체는 C# 로더를 새로 작성하고 Python 결과와 비교 테스트 |
| 디컴파일 | Ghidra + x64dbg (변경 없음) | 디컴파일 결과(C 의사코드)를 C# 로 옮길 때 정수 오버플로·부호·구조체 레이아웃 차이에 주의 |

### 결정론(멀티플레이·리플레이) 관련 주의

* 게임 시뮬레이션 코드에서는 `float`/`double` 연산 결과가 플랫폼·JIT 에 따라 달라질 수 있으므로 **고정소수점(정수) 연산**을 우선 검토한다 (원본 동작 분석 후 결정).
* `System.Random` 대신 원본 난수 생성기를 그대로 옮긴 자체 구현을 쓴다.
* `Dictionary`/`HashSet` 순회 순서에 의존하는 로직을 만들지 않는다.

### C++ 전제로 이미 준비된 항목의 처리

* `PREPARE.ps1` 의 VS Build Tools(C++)·CMake·Ninja·vcpkg 항목은 게임 빌드에 더 이상 필수가 아니다 → 2단계에서 **.NET SDK·MonoGame 템플릿 항목을 추가**하고 C++ 항목은 선택으로 내린다.
* `src/` 아래 C++ 기준으로 만든 빈 폴더(`platform/`, `engine/` 등)는 3절의 C# 프로젝트 구조로 재구성한다.

---

## 3. 디렉터리 구조 (C# + MonoGame 기준)

```
Netstorm/
├─ originals/                 # 원본 (읽기 전용, 재활용 가능, 용량 문제로 저장소에 커밋하지 않음)
├─ docs/
│  ├─ formats/                # 파일 포맷 명세 (taff.md, shp.md, type.md, fort.md, mission-script.md …)
│  ├─ gameplay/               # 게임 규칙·수치·공식 명세
│  ├─ exe/                    # 실행 파일 분석 노트 (모듈 맵, 구조체)
│  ├─ videos/                 # 영상 분석 노트
│  └─ screens/                # 원본 화면 캡처(screenShots/) 관찰 노트
├─ tools/                     # Python 추출·검증 도구, Ghidra 스크립트
├─ extracted/                 # 도구가 원본에서 추출한 결과물 (커밋하지 않음, 필요 시 생성)
├─ fonts/                     # 한국어 글꼴 D2Coding (TTC, SIL OFL)
├─ locale/                    # 번역 파일 (en, ko 1차 / de 등 이후)
├─ screenShots/               # 사용자가 찍은 원본 게임 캡처 (외부 캡처 도구 — 작업표시줄·창 테두리 일부 포함, 1.6절 표)
├─ Netstorm.sln
├─ src/
│  ├─ Netstorm.Assets/        # 원본 포맷 로더 (TAFF, .type, _shapes.shp, 팔레트, .fort, 설정, 번역) — MonoGame 비의존
│  ├─ Netstorm.Core/          # 게임 규칙·엔티티·전투·경제·다리·AI·미션 스크립트 — MonoGame 비의존 (결정론·테스트 용이)
│  ├─ Netstorm.Net/           # 멀티플레이 (락스텝)
│  └─ Netstorm.Game/          # MonoGame DesktopGL 실행 프로젝트: 렌더링·입력·오디오·UI·화면 전환
└─ tests/
   ├─ Netstorm.Assets.Tests/  # xUnit: 원본 전체 파일 파싱, Python 도구 결과와 비교
   └─ Netstorm.Core.Tests/    # xUnit: 규칙·결정론 테스트
```

* 게임 로직(`Core`)과 자산 로더(`Assets`)는 MonoGame 에 의존하지 않게 분리한다 → 헤드리스 테스트·리플레이 검증·서버 재사용이 쉽다.

---

## 4. 단계별 작업 상세

각 작업은 `[ ]` 체크박스로 관리하고, 완료 시 `[x]` 로 바꾸고 산출물 경로를 적는다.

### 2단계. 개발 환경 구축 — ✅ 완료 (2026-09-27)

- [x] 기술 스택 확정: **C# + MonoGame (DesktopGL)** — 2026-09-27 사용자 결정 (2절)
- [x] `git init`, `.gitignore` — 2026-09-27 완료. .NET 산출물(`bin/`, `obj/`, `publish/`, `TestResults/`)도 제외
- [x] 개발 도구 점검·설치 스크립트 `PREPARE.ps1` — 2026-09-27 완료 (C# 기준으로 갱신)
  - 사용법: `powershell -ExecutionPolicy Bypass -File .\PREPARE.ps1` (항목 선택 후 점검·설치), `-CheckOnly`(점검만), `-All`(선택 화면 생략), `-ToolsDir`(Ghidra·vcpkg 설치 폴더, 기본 `C:\Tools`)
  - 필수: Git, **.NET SDK 10**, Python 3 + 패키지(pillow·pefile·capstone), JDK 21+, Ghidra, x64dbg, Process Monitor, git safe.directory / 권장: MonoGame 템플릿, VS Code 확장(C# Dev Kit·C#·Python) / 선택: VS Build Tools(C++)·CMake·Ninja·vcpkg(C# 확정으로 불필요), yt-dlp, FFmpeg
  - 2026-09-27 점검 결과: 필수 전부 설치됨 (.NET SDK 8.0.425 / 9.0.318 / 10.0.401 확인). MonoGame 템플릿은 미설치(프로젝트는 템플릿 없이 구성했으므로 필수 아님)
- [x] Linux 용 개발 도구 점검·설치 스크립트 `PREPARE.sh` — 2026-09-28 작성 (PREPARE.ps1 과 같은 항목 선택 → 점검 → 설치 → 재점검 흐름)
  - 사용법: `bash ./PREPARE.sh` (일반 사용자로 실행, 시스템 패키지는 스크립트가 sudo 호출), `--check-only`, `--all`, `--tools-dir DIR`(기본 `~/Tools`)
  - 패키지 관리자 apt / dnf / pacman / zypper 지원. .NET SDK 10 은 배포판 저장소 대신 `dotnet-install.sh` 로 `~/.dotnet` 에 설치하고 `~/.profile` 등에 PATH·DOTNET_ROOT 등록
  - 필수: 기본 도구(curl·unzip·tar), Git, 빌드·실행 라이브러리(ICU·OpenSSL·OpenGL), .NET SDK 10, Python 3 + 패키지(배포판 패키지 우선, 실패 시 pip --user), JDK 21+(javac 로 확인), Ghidra, git safe.directory / 권장: MonoGame 템플릿, **Wine**(x64dbg·Process Monitor 대체: 원본 32bit exe 실행, winedbg, `WINEDEBUG=+file`), VS Code 확장 / 선택: C++ 빌드 도구·CMake·Ninja·vcpkg, yt-dlp(`~/.local/bin` 에 공식 최신 실행 파일), FFmpeg
  - `.gitattributes` 에 `*.sh text eol=lf` 추가 (Windows 체크아웃에서도 LF 유지)
  - 검증: Windows Git Bash 에서 문법 검사·`--check-only --all`·선택 화면 입력 처리만 확인. **실제 Linux 배포판에서의 설치 동작은 미검증** (아래 "Linux 에서 빌드·실행 확인" 때 함께 확인)
  - Ghidra 헤드리스 디컴파일 스크립트 `tools/ghidra/run_decomp.ps1` 은 아직 Windows 전용 (Linux 용 필요 시 `analyzeHeadless` 로 옮길 것)
- [x] `.editorconfig` — 2026-09-27 완료. UTF-8·LF 기본, `.ps1` 은 UTF-8 BOM·CRLF, `.sln` CRLF, C# 스타일 규칙(파일 범위 네임스페이스, Allman 중괄호, `_camelCase` private 필드)
- [x] 솔루션 골격 — 2026-09-27 완료. `src/` 의 C++ 기준 빈 폴더 삭제 후 3절 구조로 생성
  - `Netstorm.sln`, `global.json`(SDK 10.0.x), `Directory.Build.props`(공통: **net10.0**, Nullable, ImplicitUsings), `Directory.Packages.props`(패키지 버전 중앙 관리)
  - 대상 프레임워크는 **net10.0 (LTS, 2028-11 지원 종료)**. .NET 8 은 2026-11 지원 종료라 제외. MonoGame 3.8.5.1 패키지는 net8.0 대상이지만 net10.0 에서 그대로 동작함을 확인
  - 패키지: MonoGame.Framework.DesktopGL 3.8.5.1, FontStashSharp.MonoGame 1.6.1, xunit.v3 3.2.2 (+ xunit.runner.visualstudio 3.1.5, Microsoft.NET.Test.Sdk 17.14.1)
  - xUnit v3 를 쓴 이유: 원본 데이터가 없는 환경(CI)에서 원본이 필요한 테스트를 `Assert.SkipWhen` 으로 건너뛰기 위함. xunit.v3 4.x 는 새 테스트 플랫폼(MTP v2) 설정이 필요해 3.2.2 사용
- [x] MonoGame DesktopGL 창 + **D2Coding 한글 출력** + 원본 스프라이트 애니메이션 표시 — 2026-09-27 완료 (Windows 에서 확인)
  - FontStashSharp 는 TTC face 선택을 지원하지 않으므로 `TrueTypeCollection.ExtractFace` 로 face 0 을 TTF 로 분리해 로드
  - 검증용 `--screenshot <경로>` 옵션: 30프레임 후 화면을 PNG 로 저장하고 종료
- [x] CI — 2026-09-27 완료. `.github/workflows/ci.yml` (windows-latest + ubuntu-latest, `dotnet build` + `dotnet test`)
  - 원본이 없는 환경을 흉내 내 실행: 21개 중 원본 필요 테스트는 건너뛰고 나머지 통과 확인. **원격 저장소가 아직 없어 실제 GitHub Actions 실행은 미확인**
  - 2026-09-28 `originals/` 가 저장소에 포함되어 CI 에서도 원본 검증 테스트가 실행될 것으로 예상된다 (체크아웃 용량 약 310MB 증가). 실제 실행 시 소요 시간·Linux 대소문자 파일 찾기 결과를 확인할 것
- [x] Ghidra 프로젝트 생성, `Netstorm.exe` 임포트·자동 분석·전체 디컴파일 — 2026-09-27 완료
  - `tools/ghidra/run_decomp.ps1` (헤드리스, 약 10~20분) → 프로젝트 `extracted/ghidra/`, 결과 `extracted/decomp/Netstorm.c` (2026-09-28 재추출: 함수 4,506개 성공, 실패 0개)
- [ ] (후순위) Linux 에서 빌드·실행 확인

#### 빌드·실행 방법

```powershell
dotnet build Netstorm.sln                     # 전체 빌드
dotnet test Netstorm.sln                      # 테스트 (원본 데이터가 있으면 원본 검증 테스트도 실행)
dotnet run --project src/Netstorm.Game        # 게임 실행 (Esc 종료)
dotnet run --project src/Netstorm.Game -- --screenshot extracted/screens/shot.png
```

원본 데이터 폴더는 `NETSTORM_DATA` 환경 변수 → 실행 파일/현재 폴더에서 상위로 올라가며 `originals/` 탐색 순으로 찾는다 (`GameDataLocator`).

### 3단계. 원본 분석 — 데이터 포맷

각 포맷마다 (1) `docs/formats/*.md` 명세, (2) `tools/` 의 Python 파서·추출기, (3) 원본 전체 파일에 대한 파싱 성공 검증을 산출물로 한다.
exe 내부의 파일 로딩 함수를 Ghidra 로 함께 추적하면 빠르다(`TAFF v%d.%d`, `_shapes.shp` 문자열 참조 지점부터 시작).

- [x] **TAFF 아카이브 (`netstorm.tarc`)** — 2026-09-27 완료: [taff.md](docs/formats/taff.md), `tools/taff.py`. XOR 키 `mydoghasfleas`, 246개 전부 추출
- [x] **`.type`** 문법·속성·플래그 — 2026-09-27 완료: [type.md](docs/formats/type.md), `tools/typefile.py`, 수치표 [docs/gameplay/types.md](docs/gameplay/types.md) (70종)
  - [x] 2026-09-28: 클러스터 이름 → 원본 4바이트 프레임 코드(측면·변형·번호·플래그 비트), 기본/도움말/gump/base 프레임 규칙, 프레임 검색 함수 5종 확인 ([type.md](docs/formats/type.md) "원본 프레임 코드 표"), C# `TypeFrameTable`
  - [ ] 남은 일: 속성·플래그의 정확한 게임 내 의미 확정 (4단계와 연계), 측면 글자의 방향 대응(예: sunCannon L~P), 동작별 재생 속도
- [x] **`_shapes.shp`** — 2026-09-27 완료: [shp.md](docs/formats/shp.md), `tools/shp.py`. 116블록·3,783프레임 전부 디코딩, PNG 추출 검증
  - [ ] 남은 일: 헤더 bounds/origin 의 정확한 의미, 특수 레코드 91개의 용도
- [x] **팔레트** — 2026-09-27 완료: 게임 팔레트 = `d/GIFCLOUD.COL` ([shp.md](docs/formats/shp.md) "색상")
  - [ ] 남은 일: `!color.dat` 테이블 9개의 개별 용도, 플레이어 색 구현 방식 (4단계)
- [x] **`.chfnt`** 용도 확인 — 2026-09-27 완료: GDI 글꼴의 런타임 캐시 ([chfnt.md](docs/formats/chfnt.md))
  - [ ] 남은 일(낮은 우선순위): 글리프 데이터 디코딩 (영어 원본 글꼴 재현이 필요할 때)
- [x] **`.fort`** 컨테이너·오브젝트 레코드 — 2026-09-27 완료: [fort.md](docs/formats/fort.md), `tools/fort.py` (`dump`, `verify`)
  - 원본 463개(느슨한 파일 431 + 아카이브 32) 전부 섹션 끝까지 정확히 해석
  - 섹션 35개(Subscriber … Deck, Reserved2~4, Terr00~19), 타입 번호 변환(TypeNames 해시), 청크 레코드, 타입 플래그(typeflags 단어 → 비트, 파생 규칙) 해독
  - [x] 2026-09-28: 위치 바이트 x/y, `Territory` 모양·생성 플래그·청크 위치 및 `TerrNN` 배치 해석 — [영역 배치 분석](docs/exe/territory-layout.md), C# `FortMap`. 원본 463개 청크 수와 캡처 좌표 대조 완료
  - [x] 2026-09-28: `Deck` 섹션의 개수·4바이트 항목(타입 번호, chance, 부호 있는 power, numRemaining) 및 TypeNames 변환 해석. 공식 맵 둘과 원본 463개 파일 검증, 전체 170개 검사 통과 ([형식](docs/formats/fort.md))
    - 게임 의미(추정): 워크샵에서 등록하는 사이드바 "덱" — [workshop-deck.md](docs/gameplay/workshop-deck.md) 3절
  - [x] 2026-09-29: `Technology` 섹션의 타입별 목록 플래그·선택적 QA/QB·container 길이 해석, `FortFile.Technology`와 `tools/fort.py`에 추가. Python 도구로 원본 463개 전부 파싱, 공식 맵 두 개를 대조해 전체 172개 검사 통과. QA/QB와 비어 있지 않은 container는 원본 파일에 없어 동적 검증 필요 ([형식과 제한](docs/formats/fort.md))
  - [ ] 남은 일: `State`·`Badges`·`CoreData`·`Mission` 섹션 내부 구조, `Territory` 외관·나머지 플래그, 회전된 영역 동적 검증, 내부 class 값에 따른 파생 플래그
- [ ] **미션 스크립트** (`.english` 등): 문법 명세 — 섹션 종류와 발생 조건(이벤트), `[Header]` 키 전체 목록(`missionType`, `myTech`, `aiNName/Tech/StartMoney/Collectors/GeyserAttachments/color/BridgeDrawRate/Ability` …), `$` 명령과 인자, 인라인 HTML 태그, `{mission.filename}` 등 치환 변수. 전 파일 대상 키·명령 빈도 통계
  - [x] 문법 개요·빈도 통계·공식 캠페인 구성 — 2026-09-27 완료: [mission-script.md](docs/formats/mission-script.md)
  - [x] 머리 값·섹션 조회 규칙 — 2026-09-28 완료: 원본은 미션 파일을 설정 파일로 읽음(머리 값 = 파일 전체 첫 일치), 섹션 찾기·본문 범위 규칙 확인 ([mission-script.md](docs/formats/mission-script.md) "원본 해석 규칙")
  - [x] 조건 태그 평가 규칙 — 2026-09-28 후속: Htmlgump `0046c090`·`0046c810`, 토큰 `00487620`·atol의 어셈블리 대조. 숫자 참/거짓·문자열 일치·g/ge/l/le 비교·부정·원본 중첩 표시 규칙 확인, 본문 준비 구현 ([명세](docs/formats/mission-script.md))
  - [ ] 남은 일: 각 명령·버튼 동작·이벤트 섹션의 정확한 의미, 인라인 명령 실행·HTML 스타일 변환·동적 UI 대조 (4·9단계)
- [x] **설정 파일** — 2026-09-27 완료: [config.md](docs/formats/config.md), `tools/nscfg.py` (복호화·값 변경, 바이트 단위 라운드트립 검증)
  - [x] 2026-09-28: 조회 규칙(**첫 일치 우선** — 이전의 "마지막 값 우선" 기록 정정)·불러오는 순서(options.cfg 가 setup.cfg 보다 먼저)·설정 객체 층·`{키|기본값}` 치환·`` ` `` 이스케이프·`{Not Found:키}` 확인 ([config.md](docs/formats/config.md) "설정 조회 규칙")
- [x] **파일 조회 순서** (느슨한 파일 vs 아카이브) — 2026-09-28 완료: 데이터 폴더 디스크 → `*.tarc` → 보조(CD) 폴더 ([vfs.md](docs/formats/vfs.md))
- [x] **리소스** — 2026-09-27 완료: `tools/peres.py` (비트맵 3·커서 18·다이얼로그·문자열). DLL 문자열은 설치 프로그램용
- [x] **도움말** — 2026-09-29: 형제 저장소의 helpdeco 소스를 VS 2022 Build Tools Win32 Release로 빌드하고 `help/*.HLP` 5개에서 실제 본문 150개·BMP 77개를 추출. `tools/hlp.py`로 UTF-8 토픽별 텍스트 생성·목록 대조, [HLP 형식·재현](docs/formats/hlp.md), [게임 규칙 요약](docs/gameplay/help-manual.md). 게임 내 도움말 텍스트는 아카이브의 `help.english` 에도 있음
  - [x] **동봉 문서 전체 정독·정리** — 2026-09-29: `GAME.HLP`(규칙·화면·조작·튜토리얼·멀티·유닛/주문 핸드북), `readme.hlp`·`HELP.HLP`·`VENDOR`·`VOCAB`, `README.DOC`(Word, 1997-10 변경점), `*.CNT`·`HELP.EXE`, `PatchFixs.txt`(10.70~10.78), `Readme.txt`·`TMaker.txt`·`disclaimer.txt`·`steam_appid.txt` → **[docs/sources/](docs/sources/README.md)** (목록·불일치 12건, [게임 매뉴얼](docs/sources/game-manual.md), [패치 이력](docs/sources/patch-history.md), [설치·지원 문서](docs/sources/support-docs.md))
    - 주요 사실: 보유 exe 는 **10.75 이상**(옵션 표 근거, 10.77/10.78 추정). 섬 테마·거주지 테마는 패치 10.70 V5.3~6.0 기능. 1152×864·1280×960 해상도는 패치에서 제거. 멀티는 BattleMaster 가 서버·서버 인계·포트 6800. `bridgeDrawRate`·`stuffRefreshRate` 설정 키는 패치판 exe 에 없음(확인)
    - **확인 필요**: level 1 원소 유닛의 에너지(매뉴얼: Bulf = Thunder 1 ↔ 사용자 규칙: 아무 1), 파일 조회 순서(패치 문서: tarc 우선 ↔ 정적 분석: 디스크 우선), Storm Power 노랑 기준(매뉴얼은 1000 미만 빨강만), 튜토리얼 3 의 범위
  - [ ] 남은 일: 원본 도움말의 링크·토픽 간 탐색 정보 복원과 패치 실행 파일에서 수치·규칙 검증. helpdeco는 모든 파일에서 browse 재구성 경고 출력
- [x] 오디오 전수 확인 — 2026-09-27 완료: 229개 모두 표준 PCM WAV
  - [ ] 남은 일: 사운드 파일명 ↔ 게임 이벤트 매핑 표 (`.type` 의 `*Sound` 속성 + exe 문자열)
- [x] **다국어 체계** (계획에 없던 발견) — 2026-09-27 완료: [xlat.md](docs/formats/xlat.md). 원본에 `xlat.<언어>` 번역 테이블(독일어 871블록, 고유 원문 774개)과 `config.<언어>` 용어표가 있어 12단계 설계 기반으로 사용
  - [x] 2026-09-28: `Xlat.cpp` 해석 규칙(줄 첫 글자 `*`/`-`/`=`, 대소문자 구분 완전 일치, 뒤 번역 우선)과 언어 표 확인

**완료 기준**: 게임에 필요한 모든 원본 자산을 Python 도구로 읽어 사람이 볼 수 있는 형태(PNG/JSON/텍스트)로 내보낼 수 있다.

### 4단계. 원본 분석 — 실행 파일 (게임 로직)

목표는 "코드를 그대로 옮기는 것"이 아니라 **동작 명세를 뽑아내는 것**이다. 결과는 `docs/exe/`(함수 맵, 구조체), `docs/gameplay/`(규칙·수치) 에 정리한다.
규칙 이해·교차 검증에는 플레이 방법 소개 홈페이지(1.6절: 자원·신전·사제·유닛 페이지)를 함께 참고한다.

- [ ] 라이브러리 함수(CRT, DirectX, WinSock 래퍼) 식별·명명 → 게임 고유 코드 범위 축소
- [ ] 동적 분석 환경: 원본을 창 모드로 실행 + x64dbg 연결, 파일 접근(Process Monitor)·메모리 관찰로 정적 분석 결과 검증
- [x] 전체화면 설정 저장 위치·시작 흐름 확인 (정적 분석, 1.4절) — 2026-09-27
  - [ ] 남은 일: 동적 분석으로 실패 지점·플래그 초기화 과정 확인
  - 클론 쪽 대응(2026-09-29): 디스플레이 모드를 바꾸지 않는 테두리 없는 전체화면 창 + 시작 실패 표식으로 같은 문제를 피한다 ([map-viewer.md](docs/map-viewer.md) "화면 설정")
- [x] 가장자리 스크롤 판정·속도 (정적 분석) — 2026-09-29: [edge-scroll.md](docs/exe/edge-scroll.md). 남은 일: 왼쪽 버튼·Shift 상태 극성, 카메라 목표 위치의 보간·맵 경계 제한을 원본 실행으로 확인
- [ ] 서브시스템 경계 식별: 메인 루프/틱, 렌더, 입력, 사운드, 파일 로딩, 네트워크, 스크립트, AI
  - 출발점: assert 메시지로 복원한 원본 소스 파일 114개 모듈 맵 [docs/exe/modules.md](docs/exe/modules.md) (`tools/ghidra/module_map.py`)
- [ ] 핵심 구조체 복원: 게임 오브젝트, 플레이어, 섬, 다리, 타일/좌표계
- [ ] **게임 틱**: 고정 프레임 여부, 틱 레이트, 난수 생성기(결정론 확보에 필수)
  - [x] 2026-09-28 부분 확인 ([animation-timing.md](docs/videos/animation-timing.md)): 화면 루프 상한 `maxFPS = 75`(`sleepPerLoop = 0`), 게임 시각 = `timeGetTime` 기반 초(`00460cd0`), 애니메이션은 고정 틱이 아니라 오브젝트별 "다음 시각 = 현재 시각 + 간격(0.04초·0.08초 등)" 타이머 → 루프 주기로 올림되어 실제 **24Hz·12Hz** (영상 측정과 일치)
  - [ ] 남은 일: 게임 로직(이동·전투·경제)의 갱신 방식(가변 시간 간격인지), 가이저·신전 등 타입별 간격 상수 위치, 난수 생성기
- [ ] **좌표계·맵**: 아이소메트릭 투영, 타일 크기, 섬 형태, 높이/레이어, 그리기 순서(정렬 규칙)
- [ ] **경제**: 가이저(geyser) → Storm Power(게임 내 재화) 수집 흐름, 수집 유닛(collector) 이동, 자원 운반, 제단(altar)·희생, 비용
  - [x] 2026-09-29 도움말 근거 확보: 수송 유닛의 가이저→신전·전초기지 운반, 가이저 초기 2000, 적 파괴·건강한 유닛 회수 보상 25%, 제단 업그레이드·희생 절차. 실행 파일 판정은 남음 — [도움말의 게임 규칙](docs/gameplay/help-manual.md)
- [ ] **섬 소유권** — 규칙은 사용자 확인으로 정리됨 (2026-09-28): [docs/gameplay/island-ownership.md](docs/gameplay/island-ownership.md)
  - 섬 테두리 색 = 소유 플레이어 색. **템플이 건설된 섬이 그 플레이어 소유**. 템플은 **플레이어당 동시에 1기(파괴되면 사제가 재건 가능), 빈 섬에만**, 원소 무관 **5000** Storm Power
  - **템플이 있어야 다리 건설** 가능. 섬 상태는 내 섬 / 빈 섬(무소유) / 남의 섬
  - **남의 섬**: 건물·유닛 일체 건설 불가, 이동형 유닛은 이동 가능
  - **빈 섬**: 워크샵·알타는 사제가 도달하면 건설 가능(다리 불필요, 내 섬도 가능), 건물형 유닛은 내 섬과 다리로 연결돼야 건설 가능. 연결은 건설 시점에만 필요하고, 다리가 끊겨도 지은 유닛은 계속 동작
  - 용어: 템플·워크샵·알타는 "유닛"이 아니다 (사제 `Construct` 로 건설). 그 밖의 작은 건물·이동체가 유닛
  - 캡처 근거: The War Begins! 에서 적 신전이 없어지자 적 섬 테두리가 빨강 → 주황(소유자 없음), 지면도 초록으로 바뀜
  - 남은 일: exe 판정 함수, 소유자 변경 시점, 템플이 파괴된 섬에 남은 건물·유닛·다리의 처리, Outpost(중립 섬 소유), 받침 섬·가이저 바위의 소유권
- [ ] **다리 건설**: 다리 조각 생성 규칙(모양 풀, 순서, `BridgeDrawRate`), 배치 판정, 연결·붕괴 조건 (다리는 소유한 섬에서만 시작 — 위 소유권 규칙)
- [ ] **건물/유닛**: 배치 규칙(소유한 섬에만 배치 — 위 소유권 규칙), 건설 시간, 원소(Sun/Rain/Wind/Thunder) 별 기술 트리, 연구(기술 획득) 방식
  - **생산 규칙 (사용자 확인, 2026-09-28)**: 유닛·건물을 생산·건설하려면 **해당 타입의 워크샵을 우클릭**해 `Put Knowledge into Production >` 로 그 유닛을 **왼쪽 사이드바 "덱"에 등록**해야 한다. 등록된 것만 사이드바에서 골라 배치할 수 있다 — [workshop-deck.md](docs/gameplay/workshop-deck.md)
  - **덱의 출처 (사용자 확인, 2026-09-29)**: 화면 왼쪽 패널 = "덱" = 매뉴얼의 Production window. **워크샵이 파괴되면 그 워크샵으로 등록한 유닛이 덱에서 사라지고**, **템플은 다리 조각과 골렘을 덱에 넣으며 템플이 파괴되면 둘 다 사라진다**. **한 유닛은 한 워크샵에만 등록**(이미 등록된 유닛은 다른 워크샵 목록에 안 나옴), **파괴된 워크샵을 재건하면 우클릭으로 다시 등록해야 복구** — [workshop-deck.md](docs/gameplay/workshop-deck.md) "덱의 출처와 사라지는 조건"
  - 캡처 근거: Sun Workshop 목록(Rain Generator·Sun Cannon·Whirlibase) ↔ 이후 캡처 사이드바의 아이콘 3개. `.fort` `Deck` 섹션(타입·chance 가중 추첨)이 이 덱을 저장하는 것으로 추정
  - **원소·에너지 규칙 (사용자 확인, 2026-09-28)** — [elements-energy.md](docs/gameplay/elements-energy.md): 원소는 Rain·Wind·Thunder 3종 + 공용 Sun. 워크샵은 자기 원소 유닛만 등록(Sun Workshop 은 예외로 다른 원소 Generator 도 등록). 템플·워크샵 외 작은 건물도 "유닛". 건물형 유닛은 소유 섬·소유 섬과 다리로 연결된 무인도·본인 다리 끝에 건설. 건설·생산에는 Storm Power + 필요 원소 에너지(예: Ice Cannon = Rain 1 + Sun 1)가 그 위치에 공급돼야 함. 공급원은 Temple·Generator(넓은 범위에 자기 원소 1개분), Sun 은 어느 원소로든 대체
    - 데이터 대조: `.type` 의 `level` = 필요 에너지 개수 (사용자 확인), 발전기 `minUsage = maxUsage = -100`
    - 신전 공급(B) 해결: Temple 도 Generator 와 같이 자기 원소 1개분·같은 범위. 우클릭 시 노란 별이 돌며 범위 표시 (사용자 확인). **튜토리얼에서는 신전 범위가 대폭 축소된 경우가 있었음** (사용자 확인)
    - **공급 범위 측정 완료 (2026-09-28, 문서 6절)**: 공급 범위는 선택 시 **원소 모양 아이콘**(물방울·조개껍데기 등)으로 표시되고, 노란 별은 건물형 유닛의 **공격 범위** 표시 (사용자 확인). 아이콘이 칸 좌표 원 위에 놓임 → **일반 미션: 템플·Generator 모두 반지름 30칸(거리² ≤ 900)** (스크린샷 2장), **튜토리얼 2 Secret Workshop: 약 14칸**으로 축소 (튜토리얼 일부만). Dissolved Alliance 영상의 17.32칸 별 원은 공격 범위
    - **exe 확인 완료 (2026-09-29)** — [battle-options.md](docs/exe/battle-options.md): 반지름 = 전투 옵션 **Generator Range**. 표 `{14, 22, 30, 38}`(Short·Normal·Long·Very Long)에서 골라 **14~30칸으로 제한**(`004b4860`), 기본 Very Long → 30칸. **튜토리얼 2 처리 함수(`Totalmade.cpp` `004c3bb0`) 첫 단계만 Short = 14칸**으로 바꿈. 전투 옵션 21종 표(`0x52f5d0`)·저장 위치(`0x52f7e0 + 번호`)도 정리
    - 다음: 공격 범위(별)와 `.type` `range` 의 관계, 옵션 기본값이 적용되는 위치, `rangeDisplayProcessType`
    - **충족 판정 (사용자 확인)**: 필요 에너지 1개 = 서로 다른 공급원(Generator·Temple, 자기 또는 동맹 소유) 1개, 그 범위가 짓는 위치에서 모두 겹쳐야 함. Sun 은 아무 원소 공급원으로. 예: Vander Tower = Thunder 2 + Sun 1 → Thunder 공급원 2 + 아무 공급원 1. 구성 **`자기 원소 × (level − 1) + Sun × 1`** (사용자 확인: Generator = 아무 1, Sun level 2 = 아무 2 포함). 문서 4절에 전 유닛 필요 공급원 표
    - **소모 개념 아님**(같은 공급원 범위에 여러 유닛 가능), 조건은 **건설·생산 순간에만** 확인(공급원이 파괴돼도 유닛 유지), **이동 유닛도 생산 지점에 에너지 필요** — 질문 A~F 모두 확정 (문서 5절)
  - 남은 일: exe 의 등록·추첨(`Deck.cpp`)·도움말의 워크샵 생산 칸(Level I 2개·II 3개·III 4개) 검증·업그레이드 비용/효과, 에너지 공급 범위·판정 함수, 등록 목록과 `myTech`·`techBit` 의 관계
- [ ] **전투**: 사거리·명중·피해 공식, 발사체 궤적, 특수 효과(`bomb*` 계열: 마비, 중력, 치유, 반역 등), 방어(차단벽·실드)
- [ ] **승패 조건**: 프리스트(priest) 사망/포획, 신전(temple) 파괴, 미션 스크립트 이벤트 발생 지점
- [ ] 미션 스크립트 인터프리터 동작(3단계 문법 명세와 교차 검증)
- [ ] AI 의사결정 루틴 (10단계 입력)
- [ ] 네트워크: 프로토콜 방식(TCP/IP, IPX), 동기화 모델(락스텝 여부), 패킷 형식 (13단계 입력)
- [ ] `PatchFixs.txt` 와 대조하여 **기준 버전** 결정 (원본 1.x 동작 vs 패치 10.7x 동작)
  - 2026-09-29 정리: [patch-history.md](docs/sources/patch-history.md) (주제별 변경), 원판 매뉴얼 수치 ↔ 현재 `.type` 대조표 [game-manual.md](docs/sources/game-manual.md) 7절. 보유 exe = 10.75 이상. 사용자 캡처·설명(섬 테마, 거주지 원소 그림, Edit 메뉴 등)이 모두 패치판 기능이므로 **패치판 동작 기준**이 자연스러움 (결정은 5절 2번)

**완료 기준**: `docs/gameplay/` 만 보고도 게임 규칙을 재구현할 수 있다.

### 5단계. 원본 분석 — 플레이 영상

영상별로 `docs/videos/<이름>.md` 에 타임스탬프 기반 관찰 노트를 작성한다.
화면 구성은 홈페이지의 [게임 인터페이스](https://hjow.duckdns.org/netstorm/learninterface.htm) 설명과 대조해 UI 요소 이름을 정한다 (1.6절).
원본을 직접 실행할 수 있으므로(1.4절), 영상으로 확인하기 어려운 부분은 원본을 실행해 캡처·측정한다.
**로컬 영상(`playingVideos/`)을 우선 사용**한다 (원본 화질, 내려받기 불필요). 로컬에 없는 캠페인 2-x 만 YouTube 를 쓴다.

| 영상 | 로컬 파일 (`playingVideos/`) | YouTube 주소 | 주요 관찰 항목 |
|---|---|---|---|
| 튜토리얼 (Early Missions 1~6) | `Netstorm Islands at war - Early Missions.mp4` | https://www.youtube.com/watch?v=CI3dCrUt4tY | 기본 조작, UI 레이아웃, 튜토리얼 메시지 흐름 |
| 캠페인 1-1~4 (The War Begins! ~ Fragile Fortune) | `Netstorm Islands at war - The War Begins! ~ Fragile Fortune.mp4` | https://www.youtube.com/watch?v=AMEzorbjQYQ | 초반 미션 흐름, 메뉴·브리핑 화면 |
| 캠페인 1-5 (Thundering Power!) | `Netstorm Islands at war - Thundering Power!.mp4` | https://www.youtube.com/watch?v=adR1Kap60hw | |
| 캠페인 1-6 (Dissolved Alliance) | `Netstorm Islands at war - Dissolved Alliance.mp4` | https://www.youtube.com/watch?v=PUeIPg1BEzo | 섬 테마 3종·소유권 변화 (스크린샷과 대조) |
| 캠페인 2-1 (The Noose) | 없음 | https://www.youtube.com/watch?v=zpZsx4dRac8 | |
| 캠페인 2-2 (Rain Vs. Rain) | 없음 | https://www.youtube.com/watch?v=2NxTN314RnE | |
| 캠페인 2-3 (Run For It!) | 없음 | https://www.youtube.com/watch?v=LHkgSp0J73E | |
| 캠페인 3-2 (To The Rescue!) — 2026-09-29 추가 | 없음 | https://www.youtube.com/watch?v=WDQSrGqZAH0 | Complete Victory 캠페인 유일한 영상 (미션 이름은 캠페인 순서로 추정, 영상 확인 시 검증) |

* 로컬 영상은 1024×768 풀스크린을 16:9 모니터에서 녹화해 **좌우 검은 여백**이 있다. 프레임을 뽑을 때(`ffmpeg`, 결과는 `extracted/videos/`) 가운데 4:3 영역을 잘라 1024×768 로 맞춘 뒤 스크린샷 분석과 같은 좌표계로 잰다.
  * 형식: **AV1 1920×1080 60fps** + AAC (4개 모두 확인). 게임 화면 = x 240~1679 의 1440×1080 (프레임 4장에서 측정, 계산값과 같음), 1.40625배 확대. 창 모드 스크린샷보다 밝고 채도가 높아(AGENTS.md: HDR 설정 문제로 밝게 촬영) 색 비교에는 쓰지 않는다
  * [x] 프레임 추출 도구 `tools/videoframes.py` (`probe`·`frame`·`range`, 게임 영역 잘라 1024×768 로 축소 또는 `--native`) — 2026-09-28, [사용법](docs/videos/README.md)
  * 추출 예: `ffmpeg -ss 00:01:00 -i "<영상>" -frames:v 1 -vf "crop=1440:1080:240:0,scale=1024:768:flags=area" out.png` — 확대가 정수배가 아니라 축소해도 원본 픽셀과 정확히 같지는 않으므로, 정밀 측정(스프라이트 템플릿 매칭 등)은 스크린샷을 우선하고 영상은 **시간 측정**(애니메이션·건설·다리·이동 속도, 60fps → 약 16.7ms 단위)에 주로 쓴다
  * AV1 소프트웨어 디코딩은 느리므로 긴 구간 전체를 풀지 말고 `-ss`/`-t` 로 필요한 구간만 뽑는다
* 방송 음성이 섞여 있으므로 소리 관련 관찰(효과음·음악 전환 시점)은 참고용으로만 쓴다.
* 미션 이름 대응은 영상 파일 이름과 공식 캠페인 구성([mission-script.md](docs/formats/mission-script.md)) 기준. YouTube 2-x 의 미션 이름은 A Nation Rises 순서로 추정한 것이므로 영상 확인 시 검증한다.

- [ ] 화면 구성(HUD, 사이드바, 미니맵, 버튼 배치, 커서 종류) 캡처 및 레이아웃 좌표 기록
  - [x] 원본 캡처 1: 튜토리얼 "Bridge the Gap!" 시작 직후 — 2026-09-28 완료: [docs/screens/bridge-the-gap-start.md](docs/screens/bridge-the-gap-start.md)
    (클라이언트 1024×768 위치, 사이드바·미니맵 배치, 맵 데이터 대조. 처음의 칸 크기 가설 32×22px 은 캡처 2 로 정정됨)
  - [x] 원본 캡처 2: 캠페인 1-1 "The War Begins!" 시작 직후 — 2026-09-28 완료: [docs/screens/the-war-begins-start.md](docs/screens/the-war-begins-start.md)
    (**좌표계 확정**: 위치 바이트 상위=x·하위=y, 청크 i → (i%16, i/16), 미니맵 1px = 2칸, 칸 = 16 × 약 11.25px(→ 캡처 3 에서 16 × 11 로 정정). noIsland = 가이저 받침 바위, 섬 지형 테마 = 신전 원소 추정)
  - [x] 원본 캡처 3: 캠페인 1-3 "Save the Island!" 시작 직후 (유닛·다리·바리케이드 배치) — 2026-09-28 완료: [docs/screens/save-the-island-start.md](docs/screens/save-the-island-start.md)
    (**템플릿 매칭으로 정밀 확정**: 칸 = 정확히 16 × 11px, 모든 타입이 칸 좌표→화면 점에 스프라이트 기준점(0,0)을 그림, TerrNN 청크 = 영역 소속 청크를 y·x 순으로 훑은 것. Sun Barricade = 두 기둥 사이 광선, 미니맵은 다리도 소유자 색으로 표시)
  - [x] 원본 캡처 4~42 목록·캡처별 관찰 정리 — 2026-09-28: [docs/screens/README.md](docs/screens/README.md)
    (메인 메뉴 계열 19장, The War Begins! 19장, Dissolved Alliance! 3장, 도움말 1장. 메인 메뉴 타이틀 그림 640×480 가운데 (192, 144), 도움말 = `help.english` `F1Help` 절(메뉴에서는 `<?{global.inMission}>` F8 줄 숨김 — 조건 평가 구현과 일치), Credits의 10.72는 제작진 명단 제목이며 exe 버전 증거가 아님, Storm Power 색 흰/노랑/빨강 실례, 정보 창·게임 메뉴·결과 창 구성)
  - [ ] 정밀 노트 작성(좌표·글꼴 측정): 메인 메뉴·대화상자(`docs/screens/main-menu.md`), 도움말 창, 미션 화면 정보 창·게임 메뉴 막대, 결과 창
    - 할 일: 타이틀·버튼·돌 테두리·스크롤바 그림의 원본 자산 찾기(exe 리소스 비트맵, `TITLE*.COL` 짝 그림), 줄 간격·글꼴(`!Arial.*.chfnt` 중 어느 것인지), 메뉴 정의가 exe 하드코딩인지 스크립트(`offical*.english`·`tell.english` 의 `$Button=`)인지 확인, 도움말 링크(`#앵커`, `cmd:Tell,…`, `http…`) 동작
  - [x] Dissolved Alliance! 시작 캡처와 `--map dissolvedalliance` 대조 — 2026-09-28: [dissolved-alliance-start.md](docs/screens/dissolved-alliance-start.md)
    - 섬 모양·다리·가이저·건물 위치 일치. **플레이어 1 = 가운데 바람 섬**, 왼쪽 눈 섬 = Duke of Rain(소유자 3), 오른쪽 돌 섬 = Prince of Thunder(소유자 2) (목록 문서의 "눈 섬 = 플레이어" 정정)
    - **원본 시작 카메라 규칙**: 플레이어 1 사제 칸 기준점이 클라이언트 약 (525, 393) (캡처 3장 공통 ±4px) → 맵 뷰어 시작 카메라에 반영, 원본 캡처와 ±6px 이내로 겹침
    - **거주지(Residence) 외관 = 섬 원소별 그림** 확인 → 뷰어가 영역 원소의 lit 그림을 고르도록 수정 (`MapSpriteFrames.BodyFrame(FortMapObject, theme)`, `FortTerrainPreview.TerritoryTheme`), 검사 9개 추가·전체 147개 통과
    - 남은 일: `Playing 1`·`Playing 2`(시점 이동 캡처)의 카메라 위치를 템플릿 매칭으로 구해 섬별 대조, 소유자 3 이상의 플레이어 색, 원본의 거주지 원소 재선택 경로(exe), 섬 가장자리 풀 장식
  - [ ] The War Begins! 진행 캡처로 **신전 원소 → 지면 테마**(비 → 눈·얼음, 바람 → 갈색 풀밭, 번개 → 어두운 돌, 신전 없음 → 초록 풀밭)와 소유권 변화 확인 (4단계 exe 분석으로 확정)
- [ ] 메뉴 흐름도(타이틀 → 캠페인/멀티 → 브리핑 → 게임 → 결과)
- [ ] 애니메이션 속도·연출(건설, 다리 설치, 폭발, 승리/패배), 사운드·음악 재생 타이밍
  - [x] 반복 애니메이션 속도 — 2026-09-28: 가이저 증기 약 24Hz(41.7ms), 신전 회오리·피해 연기 12Hz(83.3ms). `tools/videoframes.py cadence` 로 영상 4개 중 3개에서 재현 측정 ([animation-timing.md](docs/videos/animation-timing.md))
  - [ ] 남은 일: 건설·다리 설치·폭발·유닛 동작별 속도, 승리/패배 연출
- [ ] 수치 검증용 관찰(건설 시간, 공격 간격, 자원 증가 속도) → 4단계 결과와 대조
- [ ] 각 미션의 시작 상태·목표·스크립트 이벤트 발생 순서

**완료 기준**: 구현 결과를 영상과 나란히 비교할 체크리스트가 준비된다.

### 6단계. 자산 로더 / 개발용 뷰어 (C#, `Netstorm.Assets` + `Netstorm.Game`) — 🔶 진행 중

`src/Netstorm.Assets/` (MonoGame 비의존), 테스트 `tests/Netstorm.Assets.Tests/` (147개, 전부 통과 — 2026-09-28)

- [x] `XorCipher` — TAFF·설정 파일 공용 XOR — 2026-09-27 완료
- [x] `TaffArchive` — 아카이브 읽기, 대소문자·구분자 무시 이름 검색 — 2026-09-27 완료
- [x] `Palette` — .COL(0x308)·RGBX(0x400) 팔레트 — 2026-09-27 완료
- [x] `ShapeDatabase` — 블록·프레임 헤더·RLE 디코딩(8bit 인덱스 + 투명 마스크) — 2026-09-27 완료. 3,692개 이미지 프레임 전부 디코딩 테스트
- [x] `TypeLoadOrder` — 타입 로딩 순서 116개 (셰이프 블록 대응) — 2026-09-27 완료
- [x] `TypeDefinition` — `.type` 파서(머리·플래그·속성·클러스터) — 2026-09-27 완료. "클러스터 × 레이어 = 프레임" 규칙 115/116 블록 검증 (예외 manabolt)
- [x] `ConfigFile` — `.cfg` 복호화·값 읽기/쓰기·재인코딩(원본과 바이트 동일) — 2026-09-27 완료
- [x] `TrueTypeCollection` — TTC 에서 face 분리 (D2Coding) — 2026-09-27 완료
- [x] `GameDataLocator` — 데이터 폴더 탐색 + 대소문자 무시 파일 찾기 (Linux 대비) — 2026-09-27 완료
- [x] `GameFileSystem` 가상 파일 시스템 — 2026-09-28 완료: 원본 `Basefile.cpp` 와 같은 우선순위(데이터 폴더 디스크 → `*.tarc` 이름순 → 보조 폴더), `.\`·`\D\` 원본 경로 표기 수용, 와일드카드 `Find`. `OriginalText`(Windows-1252/UTF-8 자동 판별) ([vfs.md](docs/formats/vfs.md))
- [ ] 자산 경로 결정: 동봉 자산 폴더 우선 → 없으면 원본 설치 경로 탐지·선택 (필수 파일 검증 — Steam 설치본 포함)
- [x] `TypeCatalog` (타입 목록 + 플래그 계산, 이름 해시) · `FortFile` (.fort 섹션·청크·오브젝트·내용물) — 2026-09-27 완료. 원본 463개 전부 해석 테스트
- [x] `FortMap` (영역 패턴·저장된 오브젝트의 월드 좌표) — 2026-09-28 완료. 실행 파일에서 패턴 64개 추출·내장, 원본 463개 영역 청크 수와 Save the Island! 캡처 좌표 검증
- [x] 개발용 정적 맵 뷰어 (`--map 이름/경로`) — 2026-09-28 완료. 이동·확대·사제 중심·오브젝트 정보·PNG 캡처. [실행 방법과 제한](docs/map-viewer.md)
- [x] 저장된 다리 렌더링 — 2026-09-28 완료. bridge 값은 각 칸의 클러스터 번호임을 확인, `MapSpriteFrames` 구현, 원본 463개 파일의 다리 프레임 범위 전수 검사
- [x] 지면 미리보기 — 2026-09-28 완료. `FortTerrainPreview`: 청크 연결 통로·시드 성장·빈 틈 보정·원소별 원본 타일·작은 받침 표시. G로 청크 윤곽 전환. 두 공식 맵의 반복 생성·타일 유효성 검증
  - [x] 본섬 밀도 시드 수정·마스크 대조 — 2026-09-28 완료: 원본 어셈블리에서 청크별 밀도 시드 초기화 확인, 월드 y·x 처리 순서 반영. `IslandCells`·`tools/terrain_mask.py` 추가. 두 공식 맵의 전체 마스크가 독립 Python 재현 결과와 일치하며, 두 원본 캡처에 경계를 겹쳐 형태를 확인. 회귀 검사 2개 추가, 전체 39개 통과 ([수치·재현 명령](docs/exe/terrain-and-bridges.md))
  - [x] 별도 절벽 `fringe` 표시 — 2026-09-28 완료: 원본 `004bfe50`의 생성 플래그·방향 폴백·y+4칸 기준점·전투 조명 조건을 적용한 `FortTerrainFringe`와 뷰어 표시 추가. 두 공식 맵 PNG 대조, 회귀 검사 6개 추가. 변형·깊이 정렬의 완전 재현은 미완료 ([분석](docs/exe/terrain-and-bridges.md))
  - [x] 다중 레이어 본체 프레임 오류 수정 — 2026-09-28 완료: 잘못된 `클러스터 × 레이어 수` 계산을 `본체 프레임 = 클러스터 번호`로 정정. 원본 가이저 기본 B00(49번)이 그림자 영역을 선택하지 않는 검사 추가, 전체 46개 통과
  - [x] 저장된 작은 받침 전용 스프라이트 복원 — 2026-09-28 완료: 같은 소유자의 noIsland 3×3 묶음에서 island·islandStalag를 같은 오른쪽 아래 기준점에 표시. 두 공식 맵의 26개·13개 받침이 저장된 모든 논리 칸을 중복 없이 포함하는지 검증. 개발용 청록·빨강 색상표 적용, 중립은 P09. 전체 54개 검사 통과, 두 맵 PNG 생성 ([분석과 제한](docs/exe/terrain-and-bridges.md))
  - [x] 본섬 `isle` 소유자색 변환 — 2026-09-28 완료: 원본 `0043b750`·`0043bc40`의 256색 변환표와 `00498220`의 소유자색 적용 경로를 미리보기에 반영. 원본 캡처와 수정 전후 PNG 육안 대조, Game 빌드 성공 ([분석과 제한](docs/exe/terrain-and-bridges.md))
  - [x] 흰 가장자리 장식 `edgeFarm` — 2026-09-28 완료: 원본 `0046da70`·`0040e560`의 청크당 20개 배치 목표와 `0040db90`의 `matchframe` 대체 규칙을 확인. 뷰어에 재현 가능한 위치 미리보기와 같은 번호 프레임·소유자색 표시 추가. 두 공식 맵 PNG를 원본 캡처와 육안 대조, 전체 검사 150개 통과. 전역 난수에 따른 개별 위치는 미확정 ([분석](docs/exe/terrain-and-bridges.md))
  - [x] 안쪽 `AA` 타일의 3×3 그림 — 2026-09-28 완료: 원본 `004c04b0`의 `JJ00` 다음 원소별 36프레임 배치와 칸 좌표식을 적용. 변형 묶음은 원본 전역 난수표 대신 3×3 영역 좌표로 고정. 네 원소의 아홉 칸 프레임 순서 검사, 원본 캡처와 뷰어 PNG 육안 대조, 전체 154개 검사 통과 ([분석](docs/exe/terrain-and-bridges.md))
  - [x] 안쪽 `AA` 변형의 99항목 난수표 — 2026-09-28 완료: 원본 MSVC `_rand`, 시드 직후 103회 호출, 연속 항목 하위 2비트 보정, 좌표 선택식을 고정 시드 미리보기에 적용. 다섯 좌표의 기대 프레임 검사, 전체 159개 검사 통과. 실제 게임의 시간 시드와 추가 난수 소비 순서는 미확정 ([분석](docs/exe/terrain-and-bridges.md))
  - [x] 가장자리 타일의 원소별 변형 범위 — 2026-09-28 완료: 원본 `0049aa90`의 후보 4분할과 원소별 첫 프레임 제외를 반영. 작은 받침 일반 폴백에서 본섬 전용 `AA00` 조각을 제외. `AB`·`BF` 후보 검사와 두 공식 맵 PNG 육안 확인, 전체 168개 검사 통과. 개별 난수 선택은 좌표 기반 미리보기 ([분석](docs/exe/terrain-and-bridges.md))
  - [ ] 남은 일: 가장자리 타일의 개별 난수 선택·실행 당시 전역 난수 시드/소비 순서·원소 선택·절벽 변형/깊이 정렬·받침 동적 생성/소유자 전파·그림자·일반 플레이어색·edgeFarm 개별 배치 위치. 원본 실행 중 마스크 메모리 대조·픽셀 단위 외관 검증은 아직 미완료 ([분석과 제한](docs/exe/terrain-and-bridges.md))
- [x] 미션 스크립트 로더, 번역(`xlat`·`config.<언어>`) 로더 — 2026-09-28 완료
  - `ConfigText`(원본 설정 조회 규칙), `ConfigStore`(층·이름 접두어 층·`{키|기본값}`·`{@미션.키}`·이스케이프 치환, `ExpandSpec("missionSpec", 이름)`), `ConfigFile.Get` 첫 일치로 정정
  - `XlatTable`(원본 해석 규칙, 블록 수·고유 원문 수), `GameLanguage`(원본 언어 + korean, OS 언어 대응, 언어 파일 없으면 영어 대체)
  - `MissionScript`(머리 값·섹션·복수 이름 머리·`$명령` 어휘 분석). 원본 스크립트 668개 전수 해석, xlat 4개 언어 블록 수, setup.cfg + config.english 치환 검사
  - [x] 게임 실행 프로젝트 설정·VFS 연결 — 2026-09-28 `GameResources`: options → setup을 한 텍스트로 합쳐 첫 일치 우선 유지, 영어/선택 언어 용어표·xlat·미션 머리 값 공급자 연결. 설정 지정 팔레트·맵 경로 적용, 셰이프·타입도 VFS 조회. `--language` 명시 선택, 설정/OS 언어, 파일별 영어 대체. 신규 검사 10개·전체 84개 통과, 빌드·독일어 확인 화면·한국어 선택 맵 PNG 검증 ([실행과 제한](docs/runtime-resources.md))
  - [x] 조건 태그 본문 전처리 — 2026-09-28 `MissionConditions`·`MissionScript.PrepareSection`: 변수 치환 후 숫자/문자열 비교·부정·표시 상태를 평가하고 활성 줄 명령 추출. 원본 CraftWarning 경계값·668개 스크립트 전체 섹션 준비 검사, 신규 49개·전체 133개 통과 ([규칙과 제한](docs/formats/mission-script.md))
  - [ ] 남은 일: 인라인 `<$명령,…>`·HTML 부분집합 해석·명령 실행/UI 연결(9단계 인터프리터), identity/user/dev/guild 설정 연결·설정 적용과 저장
- [ ] 셰이프 헤더 bounds/origin 의 의미 확정 후 기준점 처리 정리 (현재는 xmin/ymin 오프셋만 사용)
- [ ] 개발용 뷰어: 스프라이트·애니메이션·팔레트·`.type` 속성 탐색
  - [x] 정적 스프라이트 탐색 (`--sprites 타입`) — 2026-09-28 완료. 20프레임 격자·선택 프레임 확대·타입/프레임 이동·클러스터 메타데이터 표시, 페이지 이동 시 텍스처 해제. [실행 방법](docs/sprite-browser.md). 게임 프로젝트 빌드 경고 0개, `isle` PNG 캡처 확인
  - [x] 동작 재생·팔레트 교체·`.type` 속성 전체 탐색 — 2026-09-28 완료. `TypeFrameTable`(원본 프레임 코드 표) 기반으로 측면·변형이 같은 클러스터를 한 동작으로 재생(Space, ±속도), P로 `.COL` 32개 순환, Tab으로 속성 목록. 명령줄 `--frame`·`--palette`·`--play`·`--props`. 오른쪽 패널에 프레임 코드·특수 프레임·레이어 표시. `MapSpriteFrames` 기본 프레임을 원본 규칙(마지막 default)으로 정정(로딩 목록 116개에서는 결과 동일). 신규 검사 5개·전체 138개 통과, 빌드 오류 0, PNG 4장 육안 확인 ([실행 안내](docs/sprite-browser.md))
  - [ ] 남은 일: 원본 틱 기준 재생 속도·동작 전환 재현(4단계 게임 틱 분석 후), 기본 확인 화면 샘플 애니메이션을 동작 단위로 교체

### 7단계. 엔진 코어 (플랫폼 계층)

- [x] 창·전체화면·해상도 스케일링(MonoGame `GraphicsDeviceManager` + `RenderTarget2D`, 해상도 가변·레터박스) — 2026-09-29 완료: `Netstorm.Core.Display`(`ScreenLayoutCalculator`·`WideScreenMode`·`DisplaySettings`, 테스트 50개) + `Netstorm.Game/DisplayManager`. 논리 해상도(원본 480/600/768 높이) 렌더 타깃 → 뷰포트로 늘려 표시, 마우스 좌표 논리 변환, F11 전체화면(테두리 없는 전체 화면 창)·F10 와이드 처리·F9 해상도 높이·F7 가장자리 스크롤, `--window/--fullscreen/--wide/--view-height` 명령줄. 1024×768·1920×1080·1920×1200 창과 1920×1080 전체화면 실행 확인, 전체화면 저장 뒤 연속 재실행 두 번 정상, 시작 실패 표식(`StartupInProgress`)이 남으면 창 모드로 시작하는 안전장치 확인 ([map-viewer.md](docs/map-viewer.md) "화면 설정")
  - [ ] 남은 일: HiDPI(창 좌표와 백버퍼 크기가 다른 환경) 검증, 글꼴이 비정수 배율에서 흐려지는 문제(게임 UI 를 만들 때 원본 해상도 기준 글꼴/정수 배율 옵션 검토), 다중 모니터·전체화면 커서 가두기(SDL 그랩), Linux 에서 전체화면 동작 확인, 옵션 화면(9단계)과 설정 연결
  - 전체화면 ↔ 창 모드 전환과 설정 저장 후 재실행이 안정적으로 동작해야 한다 (원본의 재실행 오류 재현 금지, 1.4절)
  - **화면비 16:9 · 16:10 · 4:3 지원** (AGENTS.md, 1.7절): 와이드 화면은 **맵 시야 확장으로 확정**(레터박스는 쓰지 않음, 2026-09-29 사용자 결정), 메뉴·대화상자 가운데 배치, 해상도별 스크린샷 검증(예: 1920×1080, 1920×1200, 1024×768)
- [x] **가장자리 스크롤**: 풀스크린에서 마우스 커서가 화면 끝에 닿으면 카메라 이동 (AGENTS.md, 원본도 지원) — 2026-09-29 완료: 원본 `004d65de`~`004d67ad` 분석([edge-scroll.md](docs/exe/edge-scroll.md): 1픽셀 가장자리, 시작 속도 2에서 초당 +30, 상한 `edgeScrollSpeed` 35 프레임당 픽셀, 전체화면·왼쪽 버튼 안 누름·팝업 없음 조건, 위쪽은 메뉴 막대 예외, Shift 한 축)를 `EdgeScrollController`(초당 = 프레임당 × 75)로 구현하고 맵 뷰어에 연결. 실제 전체화면 스크린샷으로 이동량 확인
  - [ ] 남은 일: 메뉴 막대가 생기면 `TopEdgeBlocked` 연결, 왼쪽 버튼·Shift 상태 극성과 카메라 보간·맵 경계 제한의 동적 확인(원본 실행), 게임 카메라(8단계)에 연결
  - 원본 설정값: `options.cfg` 의 **`edgeScrollSpeed = 35`** (2026-09-28 확인, 단위·적용 코드는 미확인), 켜기/끄기는 Options 메뉴 `Edge Scroll in Fullscreen`
- [ ] 8bit 인덱스 → 팔레트 적용 방식 결정 및 구현 (CPU 변환 vs 팔레트 셰이더, 2절) — 그림자·색상 변환 테이블(`!color.dat`) 효과 대응
- [ ] 입력(마우스·키보드·단축키), 커서
- [ ] 오디오: `SoundEffect` 효과음 다중 재생, `DynamicSoundEffectInstance` 음악 스트리밍, 볼륨
- [ ] 고정 틱 게임 루프(시뮬레이션과 렌더 분리), 결정론적 난수
- [ ] 로깅, 설정 저장(사용자 데이터 경로: Windows `%APPDATA%`, Linux `$XDG_CONFIG_HOME`/`$XDG_DATA_HOME`)
  - [x] 표시 설정 저장 — 2026-09-29: `DisplaySettings`(`%APPDATA%\NetstormReborn\settings.json`, `NETSTORM_SETTINGS_DIR` 로 폴더 변경, 임시 파일 교체 저장, 손상 시 기본값)
  - [ ] 남은 일: 소리·언어·튜토리얼 팁 등 나머지 옵션 저장(원본 `options.cfg` 항목 대응), 로깅

### 8단계. 게임 월드 / 규칙 구현

4단계 명세를 기반으로 구현하며, 모든 규칙 코드는 단위 테스트를 둔다.

- [ ] 맵·섬·아이소메트릭 렌더링, 카메라 스크롤, 오브젝트 그리기 순서
- [ ] 엔티티 시스템 (`.type` 데이터 구동)
- [ ] 애니메이션 시스템
- [ ] 다리 조각 생성·배치·연결·붕괴
- [ ] 건물 배치·건설
- [ ] 경제(가이저, 수집, 운반, Storm Power)
- [ ] 이동 유닛(수집 유닛, 프리스트 등) 경로 탐색
- [ ] 전투·발사체·특수 효과
- [ ] 기술 획득·원소별 기술 트리
- [ ] 승패 판정

**완료 기준**: 스크립트 없이 1:1 스커미시(인간 vs 더미)가 끝까지 진행된다.

### 9단계. UI · 미션 스크립트 · 튜토리얼

- [ ] UI 위젯 프레임워크(버튼, 체크박스, 메뉴, 스크롤 텍스트, 대화상자)
- [ ] 인게임 HUD (5단계 레이아웃 기준)
  - UI 요소 이름·역할은 홈페이지 [게임 인터페이스](https://hjow.duckdns.org/netstorm/learninterface.htm)·[설치 및 시작](https://hjow.duckdns.org/netstorm/learninstall.htm)(메뉴·옵션) 참고
  - Storm Power(게임 내 재화) 표시: 충분하면 흰색, 부족해지기 시작하면 노란색, 더 부족하면 빨간색 (사용자 확인) — **기준값 확인(2026-09-29, `Combatgump.cpp` `0043da10`): SP ≤ 1000 빨강, 1001~2000 노랑, 2001 이상 흰색**. 형식 `~3~E~%c%d~[I%d.3]` (크기 3·엠보스·색·숫자·SP 아이콘). 캡처 값 전부 일치 ([sources/README.md](docs/sources/README.md) 3절 #5)
- [ ] 미션 스크립트 파서(관대한 파싱: 대소문자 무시, 알려진 오타 허용, 경고 로그) 및 인터프리터
  - [x] 본문 변수 치환·조건 평가·활성 줄 명령 추출 — 2026-09-28 완료. 명령 실행·게임 상태/이벤트·UI 연결은 미완료
- [ ] HTML 부분집합 렌더러(`<h2>`, `<p>`, `<i>`, `<br>` …) — 한국어 줄바꿈(어절 단위) 지원
  - 도움말(`help.english`)에서 쓰는 요소도 포함: `<b>`, `<a name>`·`<a href="#앵커">`·`cmd:`·`http` 링크, `~색이름~.` 색 코드, `<c>` 강조, 조건 태그. 원본 모습은 도움말 캡처(`help - NetStorm Instructions.png`)와 대조
- [ ] 이벤트 섹션 트리거 연결 (`[Succeeded]`, `[Failed]`, `[aiNPriestDead]` 등)
- [ ] 메인 메뉴 / 설정 / 브리핑 / 결과 화면
  - 메인 메뉴 버튼 8개(Campaign, Multiplayer, Demo, Help, Edit, Credits, Options, Quit), 640×480 타이틀 그림 가운데 + 구름 배경 — `mainMenu.png` 기준
  - 하위 화면 구성은 [docs/screens/README.md](docs/screens/README.md) 1절: Campaign(6묶음·완료 점·잠긴 흐린 글자), Multiplayer(요새 섬 + Multiplayer Options 창), Demo(3개), Help 드롭다운(General Help - F1, Technical Help, Version), Edit(Load Battle Map 2열 목록), Credits(10.72 패치·원본), Options 드롭다운
  - 미션 흐름: 브리핑(`[A.]`, Review Knowledge / Play Mission) → 게임 → Success!(`[Succeeded]`, Leave Missions / Next Mission) 또는 Failure!(`[Failed]`, Continue) → 재도전 확인(Replay Mission / Leave Missions)
- [ ] 게임 화면 오브젝트 정보 창(컨텍스트 메뉴) — **마우스 오른쪽 버튼 클릭으로 연다** (사용자 확인, [workshop-deck.md](docs/gameplay/workshop-deck.md)). 워크샵 메뉴는 사이드바 덱 등록 경로라 생산에 필수: `<이름> Level I`, Owner·Alignment·Class, 명령(Construct >, View Netstorm Knowledge, Put Knowledge into Production >, Upgrade costs N, Salvage gains N, About, Player >), 하위 창은 오른쪽에 열림 — `The War Begins! - * Context Menu.png`
- [ ] 게임 메뉴 막대(화면 맨 위): Game · View · Options · Players · About. Game = Review Mission Objectives - F8 / Restart Mission, Leave Mission / Quit Game
- [ ] 튜토리얼 재현 → 튜토리얼 영상과 대조

### 10단계. AI

- [ ] 미션 헤더의 AI 파라미터 해석(`Tech`, `StartMoney`, `Collectors`, `GeyserAttachments`, `BridgeDrawRate`, `Ability` 플래그)
- [ ] 4단계에서 분석한 AI 루틴 재구현 (건설 순서, 다리 확장, 공격 대상 선택)
- [ ] 난이도·행동을 원본 미션에서 비교 검증

### 11단계. 캠페인 · 저장

- [ ] 캠페인 진행 상태 저장(`$Config` 명령 대응 — 완료 미션 기록 등)
- [ ] `.fort` 읽기/쓰기 (플레이어 요새 저장)
- [ ] 튜토리얼(Early Missions: Bridge the Gap! 등 6개)과 캠페인(Struggle For Freedom 이후)을 영상과 대조하며 순서대로 플레이 가능하게 만들기 — Early Missions 는 캠페인이 아니라 튜토리얼임 (2026-09-28 사용자 확인)

### 12단계. 다국어 지원 (설계는 2단계부터 반영)

- [ ] 모든 UI 문자열을 코드에서 분리 → 문자열 ID + 번역 파일(`locale/<언어>/`)
- [ ] 원본 문자열 소스 통합: `NSENGLISHRES.DLL`/exe 리소스, 도움말, 미션 스크립트
- [x] 미션 스크립트 언어 선택 규칙: `<미션>.<언어>` 파일 우선 → 없으면 영어 폴백 (원본의 `.english`/`.german`/`.french` 규칙 확장, 예: `.korean`) — 2026-09-28 `GameLanguage.ResolveFile`
- [x] 원본 텍스트 Windows-1252 → UTF-8 변환 로딩 — 2026-09-28 `OriginalText` (UTF-8 파일은 그대로)
- [ ] 한국어 글꼴 렌더링: `fonts/` 의 D2Coding TTC 로드(face 인덱스 0 일반 / 1 Bold), 폰트 폴백 체인(원본 `.chfnt` → D2Coding), 고정폭 글꼴에 맞춘 텍스트 박스 자동 크기 조정 (1.5절)
- [ ] 게임 내 언어 선택 메뉴
  - [x] 실행 시 `--language` → 설정 → OS UI 언어 선택 — 2026-09-28 `GameResources`·실행 프로젝트 연결 (개발용 안내 문구 전체 번역은 미완료)
- [ ] **1차 지원 언어: 영어, 한국어** (AGENTS.md). 독일어 등 원본 제공 언어는 원본 파일이 있으므로 이후 확장 시 우선 추가
- [ ] 한국어 번역: UI 문자열, 도움말, 튜토리얼·캠페인 미션 스크립트(`.korean` 파일 신설)
  - 용어 표기는 한국어 플레이 방법 소개 홈페이지(1.6절)의 기존 표기를 참고해 정하고, 용어집(`locale/ko/` 등)으로 관리

### 13단계. 멀티플레이

- [ ] 결정론적 락스텝 모델 (명령만 전송, 동기화 검사용 체크섬)
- [ ] LAN 게임 (직접 IP 접속 / 로컬 브로드캐스트 탐색)
- [ ] 인터넷 게임: 로비 서버(별도 프로그램) — 후순위
- [ ] 원본 클라이언트와의 프로토콜 호환 여부 결정 (기본 방침: 호환 안 함, 분석 결과에 따라 재검토)
- [ ] 리플레이 저장(락스텝 명령 기록 재사용) — 디버깅·QA 에도 활용

### 14단계. 패키징 · 배포

- [ ] Windows 10/11 x64: `dotnet publish -r win-x64 --self-contained` 결과를 zip 또는 설치형으로 배포
- [ ] 원본 자산 동봉 배포 (재활용 가능, 1.3절) — 필요한 파일만 선별해 패키지 크기 최소화
- [ ] D2Coding 글꼴 동봉 + SIL OFL 1.1 라이선스 고지 포함
- [ ] (후순위) Linux: `dotnet publish -r linux-x64 --self-contained` + AppImage 우선(필요 시 Flatpak), X11·Wayland 양쪽 동작 확인
- [ ] 라이선스·면책 고지 문서

### 15단계. 검증 · QA (상시)

- [ ] 포맷 로더 회귀 테스트 (원본 전체 파일 파싱)
- [ ] 규칙 단위 테스트 (4단계 수치 기반)
- [ ] 영상 대조 체크리스트 통과 여부 기록
- [ ] 원본 직접 실행 결과(화면·수치)와 클론 비교
- [ ] 결정론 테스트: 동일 입력 리플레이 → 동일 체크섬 (Windows 먼저, Linux 지원 시 교차 검증)

---

## 5. 결정이 필요한 사항

1. ~~**기술 스택**~~ → **결정됨** (2026-09-27): C# + MonoGame (DesktopGL)
2. **기준 버전**: 원본 1997 동작 vs Ticonderoga 패치(10.7x, 보유 exe) 동작 — 보유 exe 는 패치판이므로 기본적으로 패치판 동작을 따르되, 차이점은 `PatchFixs.txt` 로 문서화
3. ~~**자산 정책**~~ → **결정됨** (AGENTS.md): 원본 파일 재활용 가능. 저장소에는 커밋하지 않고, 배포 시 필요한 자산을 동봉한다
4. **멀티플레이 범위**: LAN 만 / 인터넷 로비 포함 / 원본 호환
5. ~~**와이드 화면(16:9·16:10) 처리**~~ → **결정됨(2026-09-29, 사용자): 맵 시야 확장.** 4:3 + 좌우 여백(레터박스) 화면은 필요 없다. 근거: 원본은 4:3 만 지원하고(최대 1024×768) 카메라 확대가 없으며 해상도를 높이면 더 넓은 맵이 보였다 — 와이드에서 맵을 옆으로 더 보여 주는 것이 그 동작과 가깝다 (1.7절). 남은 문제: 멀티플레이에서 해상도(화면비)에 따른 시야 차이를 허용할지 (13단계)

## 5-0. 현재 진행 상황 (2026-09-29 세션 종료 시점)

**이 세션에서 완료 (모두 문서 작업·정적 분석, 코드 변경 없음, 커밋 전)**
* AGENTS.md 추가 사항 반영: 캠페인 3-2 YouTube 영상 (5단계 영상 표)
* 동봉 문서 전체 정리 → [docs/sources/](docs/sources/README.md) (게임 매뉴얼·패치 이력·설치/지원 문서, 불일치 12건)
* 에너지 공급 범위 exe 확인 → [docs/exe/battle-options.md](docs/exe/battle-options.md) (Generator Range 14/22/30/38, 30 상한, 튜토리얼 2 만 14칸, 전투 옵션 21종 표)
* 불일치 해소 3건: 파일 조회 순서 = **디스크 우선 유지**([vfs.md](docs/formats/vfs.md)), Storm Power 색 = **≤1000 빨강 / ≤2000 노랑 / 그 이상 흰색**(`0043da10`), `bridgeDrawRate`·`stuffRefreshRate` 설정 키는 패치판 exe 에 없음

**이후 이어서 진행 (2026-09-29 후속 세션) — 코드 변경 포함, 커밋 전**
* 가장자리 스크롤 exe 분석 → [docs/exe/edge-scroll.md](docs/exe/edge-scroll.md) (1픽셀 가장자리, 속도 2 → +30/초 → 상한 `edgeScrollSpeed` 35 프레임당 픽셀, 창 테두리 없을 때만, 위쪽은 메뉴 막대 예외, 왼쪽 버튼·Shift 조건)
* 7단계 착수: `src/Netstorm.Core/Display/`(`ScreenLayoutCalculator`, `WideScreenMode`, `EdgeScrollController`, `DisplaySettings`) + `src/Netstorm.Game/DisplayManager.cs` + 뷰어 연결(`FortMapViewer`·`SpriteBrowser`·`NetstormGame`), 새 테스트 프로젝트 `tests/Netstorm.Core.Tests`(50개, 솔루션 추가). 빌드 오류 0, 전체 테스트 222개(Assets 172 + Core 50) 통과
* 실제 실행 확인: 창 1024×768·1920×1080·1920×1200(시야 확장), 1920×1080 4:3 레터박스(영상과 같은 좌우 240px), 1920×1080 전체화면, 전체화면 저장 뒤 연속 재실행 2회, 시작 실패 표식 안전장치, 가장자리 스크롤 이동량(스크린샷 측정 ≈ 곡선 계산값)
* 이 세션의 변경 파일(커밋 전): `LEFT_JOBS.md`, `Netstorm.sln`, `docs/map-viewer.md`, `docs/exe/edge-scroll.md`(신규), `docs/sources/README.md`, `src/Netstorm.Core/Display/*`(신규), `src/Netstorm.Game/{DisplayManager(신규),NetstormGame,FortMapViewer,SpriteBrowser}.cs`, `tests/Netstorm.Core.Tests/*`(신규)
* 와이드 화면 방식은 사용자가 **시야 확장으로 확정**했다 (레터박스는 게임에서 쓰지 않음, 5절 5번). 사용자 확인 필요: 뷰어 안내 영역을 4줄(128px)로 늘리고 "다리·지면 미리보기 검증 전" 안내 줄을 화면에서 제거한 것(내용은 map-viewer.md 에 그대로 있음)

**당시 중단된 작업 → 2026-09-29 후속 분석으로 처리**
* ~~level 1 원소 유닛의 필요 에너지 판정 함수 확인~~ → 타입 구조체 `+0xA0` 요구 문자열, `Rifttype.cpp` `0049b0d0` 생성 및 `Mana.cpp` `004734d0`·`00473330` 검사 확인. 이전 후보 `0044ac30`·`00473d00`은 다른 구조체의 필드였다. 결과는 [energy-requirements.md](docs/exe/energy-requirements.md), 게임 내 재현은 6절 후속 항목에 기록

**커밋되지 않은 변경 (이번 세션 마지막 작업분)**: `LEFT_JOBS.md`, `docs/formats/vfs.md`, `docs/gameplay/elements-energy.md`, `docs/screens/README.md`, `docs/sources/README.md` (그 이전 작업분은 0929 01·02 커밋에 포함됨)

## 5-1. 현재 진행 상황 (2026-09-28 세션 중단 시점 인수인계 → 같은 날 재개·처리)

작업 도중 셸 명령(Bash·PowerShell)이 자동 모드 안전 확인 단계의 무응답으로 계속 거부되어 세션을 멈췄다. 파일 읽기·수정만 가능했던 구간의 결과이므로 아래 "미검증" 항목을 다음 세션 처음에 확인한다.

**이 세션에서 완료한 것**
* 코드 (빌드 오류 0·테스트 138개 통과 확인됨):
  * `TypeFrameTable` — 원본 `Rifttype.cpp` 프레임 코드 표(측면·변형·번호·플래그), 기본/도움말/gump/base 프레임, 검색 함수 ([type.md](docs/formats/type.md) "원본 프레임 코드 표")
  * 스프라이트 뷰어 동작 재생·팔레트 순환·속성 목록, `--frame`·`--palette`·`--play`·`--props` ([sprite-browser.md](docs/sprite-browser.md))
  * `GameResources.FindPaletteNames`·`LoadNamedPalette`·`PaletteName`, `MapSpriteFrames` 기본 프레임을 원본 규칙(마지막 default)으로 정정
* 문서 (셸 없이 작성, 코드 변경 없음):
  * [docs/screens/README.md](docs/screens/README.md) — 스크린샷 42장 목록·캡처별 관찰, 원본 해상도 4:3 세 가지뿐·카메라 확대 없음(사용자 확인), 신전 원소 → 지면 테마 추정
  * [docs/gameplay/island-ownership.md](docs/gameplay/island-ownership.md) — 섬 소유권 규칙(사용자 확인): 테두리 색 = 소유자, 신전이 있어야 소유, 소유 섬에서만 배치·다리 시작, 비소유 섬은 통과만
  * 본 문서: AGENTS.md 변경(화면비·풀스크린·가장자리 스크롤·스크린샷·로컬 영상) 반영, `originals/` 커밋 방침 변경, 로컬 영상 형식(AV1 1920×1080 60fps, 사용자 확인) 반영
* 코드 확인: `FortTerrainPreview` 는 이미 영역 신전의 소유자·원소로 테두리 색·지면 테마를 정하고 신전이 없으면 중립·`sun`(초록)으로 그린다 → 소유권 규칙과 일치 (수정 불필요)

**위 중단 시점의 미확인 항목 → 2026-09-28 재개 후 처리 결과**
1. ~~커밋 여부 확인~~ → 사용자가 모두 커밋함 (재개 시점에 미반영 파일 없음)
2. ~~나머지 영상 형식·여백 측정~~ → 4개 모두 AV1 1920×1080 60fps, 게임 영역 x 240~1679 측정 ([docs/videos/README.md](docs/videos/README.md))
3. ~~Dissolved Alliance! 맵 뷰어 대조~~ → 완료 ([dissolved-alliance-start.md](docs/screens/dissolved-alliance-start.md)). 원본 시작 카메라 규칙 발견·뷰어 반영, 거주지 원소별 그림 반영 (검사 147개 통과, 빌드 오류 0, 경고는 기존 CA2014 뿐)
4. 영상 프레임 추출 도구 `tools/videoframes.py` 작성 완료. `docs/videos/` 미션별 관찰 노트는 **미착수** → 6절 5번

**재개 후 변경 파일 (커밋 전)**: `tools/videoframes.py`(신규), `src/Netstorm.Assets/MapSpriteFrames.cs`, `src/Netstorm.Assets/FortTerrainPreview.cs`, `src/Netstorm.Game/FortMapViewer.cs`, `tests/Netstorm.Assets.Tests/MapRenderingTests.cs`, `docs/videos/README.md`(신규), `docs/screens/dissolved-alliance-start.md`(신규), `docs/screens/README.md`, `docs/gameplay/island-ownership.md`, `docs/map-viewer.md`, `LEFT_JOBS.md`

## 6. 바로 다음 작업

0. **동봉 문서 참고 (2026-09-29 정리)**: 작업 전에 [docs/sources/README.md](docs/sources/README.md) 를 먼저 본다 — 규칙·조작·유닛 수치와 **현재 분석과의 불일치 12건**(3절)이 정리되어 있다. 사용자가 외부에서 추가한 [`originals/help/manual.pdf`](originals/help/manual.pdf)는 조작·생산 절차와 일부 비용을 `GAME.HLP`·보유 `.type`에 [선별 대조](docs/sources/pdf-manual.md)했다. 메인/컨텍스트 메뉴 그림은 실제 UI 형태와 부합한다(사용자 확인). 옛 메인 메뉴의 기본 항목은 7개, 보유 패치판은 `Edit` 포함 8개다. 비용·일부 메뉴 항목은 판본에 따라 다르며 전체 페이지 대조는 남아 있다. 우선 확인할 것:
   - ~~level 1 원소 유닛(Bulf·Arc Spire·Crystal Crab)의 필요 에너지 exe 분석~~ → **2026-09-29 완료**: `Rifttype.cpp` `0049b0d0`이 타입 `+0xA0`의 기본 요구값을 만들고, `Mana.cpp` `004734d0`·`00473330`이 배치 위치에서 검사한다. Bulf·Arc Spire는 Thunder 1, Crystal Crab은 Rain 1. Generator는 `.type`의 명시 `mana = "s"`로 아무 공급원 1개. 이전 후보 `0044ac30`·`00473d00`은 다른 구조체 필드로 인한 오인. [분석](docs/exe/energy-requirements.md), [유닛 표](docs/gameplay/elements-energy.md) 4절. **후속**: 패치판 게임에서 교차 원소 공급원 아래 세 유닛의 배치 성공/실패를 동적으로 확인
   - ~~파일 조회 순서~~ → 2026-09-29 재확인: 보유 exe 는 **디스크 우선** (열기 22곳·이름 해석 12곳 모두 디스크 먼저, 아카이브 우선 존재 검사 18곳은 결과를 있음/없음으로만 사용). 패치 문서 문장은 이 exe 의 열기 동작과 맞지 않음 — [vfs.md](docs/formats/vfs.md)
   - ~~Storm Power 숫자 색 기준값~~ → 2026-09-29 확인: ≤1000 빨강, 1001~2000 노랑, 그 이상 흰색 (`0043da10`)
1. 4·6단계: 지면 미리보기를 원본 캡처와 대조하여 정확도를 높이기
   - 출발점: `Chunkmap.cpp`, `Islandbuilder.cpp`, `Terrainbuilder.cpp`, `Renderer.cpp`, 청크 크기 16×16 (Template.cpp `FUN_004be020`)
   - **작업 시작 전 참고**: [save-the-island-start.md](docs/screens/save-the-island-start.md) 4절(칸 16×11px·기준점 규칙·영역 청크 배치 — 템플릿 매칭으로 확정), [the-war-begins-start.md](docs/screens/the-war-begins-start.md) 4절(미니맵 대조), [bridge-the-gap-start.md](docs/screens/bridge-the-gap-start.md)
   - **완료(2026-09-28)**: 영역 청크 결정 규칙·맵 뷰어 기본 구현, `FortMap` 및 [영역 배치 분석](docs/exe/territory-layout.md), [맵 뷰어 실행](docs/map-viewer.md)
   - **추가 완료(2026-09-28)**: 저장된 다리 표시와 지면 미리보기 구현. [다리·지면 분석](docs/exe/terrain-and-bridges.md). 이전 추정 정정: 저장된 bridge 값은 각 칸의 프레임이며 VA 0x52f998은 새 조각 배치용 복합 패턴
   - **후속 완료(2026-09-28)**: 본섬 밀도 시드 오류 수정, 독립 재현 도구와 두 공식 맵의 전체 마스크 일치 검증, 원본 캡처의 섬 형태 대조. `extracted/terrain/*-overlay.png`와 수정 뷰어 PNG 생성, 테스트 39개 통과·솔루션 빌드 성공 (NuGet 취약성 조회 연결 실패 경고 NU1900 1개)
   - **절벽 후속 완료(2026-09-28)**: fringe 생성·방향 폴백·y+4칸·전투 조명 조건 적용, 뷰어 표시 및 두 공식 맵 PNG 대조. 다중 레이어 본체 프레임 오류도 수정하여 가이저 본체 표시 복구. 테스트 46개 통과·빌드 성공 (NU1900 조회 경고 1개)
   - **받침 후속 완료(2026-09-28)**: 저장된 noIsland 3×3 묶음을 전용 island·islandStalag로 복원, 공통 기준점·소유자색 클러스터 선택 적용. Save the Island! 26개·The War Begins! 13개 받침의 모든 저장 칸 대조, 테스트 54개 통과·빌드 성공 (NU1900 조회 경고 1개). `extracted/screens/fort-map-supports-*.png` 생성
   - **지면 색상 후속 완료(2026-09-28)**: 원본 isle 변환표와 소유자별 색상 적용 경로 구현. Save the Island! 수정 전후·원본 캡처 육안 대조에서 갈색 윗면 테두리가 청록색으로 바뀜을 확인. Game 빌드 성공, `extracted/terrain/player-color-*.png` 두 맵 생성
   - **가장자리 장식 후속 완료(2026-09-28)**: 흰 돌출 장식이 원본 `edgeFarm` 프레임임을 확인. `Islandbuilder`의 청크당 20개 배치 목표와 `matchframe`의 `isle` 대체 규칙을 뷰어에 반영. Save the Island!·The War Begins! 원본 캡처와 새 PNG 육안 대조, 전체 검사 150개 통과. 위치는 원본 전역 난수 대신 재현 가능한 근사
   - **안쪽 지면 후속 완료(2026-09-28)**: `isle.type`의 `JJ00` 뒤 원소별 36프레임과 원본 `004c04b0`의 3×3 좌표식을 적용. 네 원소 3×3 프레임 검사, Save the Island! PNG 대조, 전체 154개 검사 통과. 묶음별 변형 번호는 전역 난수 대신 고정 해시
   - **안쪽 변형 난수표 후속 완료(2026-09-28)**: 원본 MSVC `_rand`의 고정 시드와 초기 103회 호출, 99항목 표 보정, 좌표 선택식을 적용. Save the Island! PNG 생성, 전체 159개 검사 통과. 실제 게임의 시간 시드와 추가 난수 소비 순서는 미확정
   - **가장자리 변형 범위 후속 완료(2026-09-28)**: `0049aa90`의 원소별 후보 범위와 첫 프레임 제외를 적용. 두 공식 맵 PNG 육안 확인, 전체 168개 검사 통과. 개별 변형은 전역 난수 대신 좌표로 고정
   - 남은 핵심: 가장자리 타일의 개별 난수 선택·실행 당시 전역 난수 시드/소비 순서·원소 선택·절벽 변형과 깊이 정렬·그림자·받침 동적 생성/소유자 전파·일반 플레이어색·edgeFarm 개별 배치 위치, 일반 전투 섬 재배치, 카메라 원점. 원본 실행 중 마스크 메모리·픽셀 외관 비교는 미완료. 뷰어는 현재 미션 저장 위치 원점 (1,1)을 사용
   - 방향 주의: 원본 활성 영역은 모두 방향 0. 섬 생성과 크기 계산 함수의 방향 전달 방식 차이를 분석 문서에 기록했으며 회전된 파일의 동적 확인 필요
2. `.fort` 남은 섹션(`Territory` 외관·잔여 플래그, `State` 등) 해석 — `Template.cpp` 의 해당 읽기 함수(디컴파일 126,100~127,840행)
   - **Deck 완료(2026-09-28)**: `Deck.cpp` 항목 구조와 `Template.cpp` 저장·읽기 순서를 확인해 `FortFile.Deck` 구현. 공식 맵 첫 항목과 원본 463개 섹션 길이 대조, 전체 170개 검사 통과 — [형식](docs/formats/fort.md)
   - **Technology 완료(2026-09-29)**: 타입별 저장 상태와 선택적 필드를 `FortFile.Technology`와 `tools/fort.py`로 읽음. Python 도구로 463개 `.fort`를 파싱하고 공식 맵 두 개 검증, 전체 172개 검사 통과. 특수 타입·문맥 경로의 목록 플래그 저장 여부는 미검증 — [형식](docs/formats/fort.md)
3. 6단계 나머지: 애니메이션·팔레트·`.type` 속성 탐색, 자산 경로 결정(동봉 폴더 → 원본 설치 경로 탐지)
   - **완료(2026-09-28)**: 가상 파일 시스템(디스크 우선 확인), 설정 조회·치환, 번역표, 미션 스크립트 로더 — [vfs.md](docs/formats/vfs.md), [config.md](docs/formats/config.md) "설정 조회 규칙", 테스트 74개 통과
   - **실행 연결 완료(2026-09-28)**: `GameResources`로 설정·언어 용어표·번역표·미션 머리 값 조회 구성. 팔레트·셰이프·타입·맵은 공통 VFS 조회, `--language`·OS 언어·영어 파일 대체 적용. 전체 84개 검사 통과·빌드 성공, 독일어 확인 화면/한국어 선택 맵 PNG 생성 — [실행 자산·설정·언어](docs/runtime-resources.md)
   - **조건 평가 완료(2026-09-28)**: 원본 비교·부정·atol·중첩 표시 규칙 확인, `MissionConditions`·`PrepareSection` 구현. 원본 CraftWarning 경계값과 668개 스크립트 전체 섹션 전처리 검사, 전체 133개 통과·빌드 성공 — [조건 명세](docs/formats/mission-script.md)
   - **정적 스프라이트 탐색 완료(2026-09-28)**: `--sprites 타입`, 20프레임 격자·클러스터 정보·PNG 캡처. [실행 안내](docs/sprite-browser.md)
   - **스프라이트 동작·팔레트·속성 탐색 완료(2026-09-28)**: 원본 프레임 코드 표(`Rifttype.cpp`) 해석과 `TypeFrameTable`, 뷰어 재생·팔레트 순환·속성 목록. [type.md](docs/formats/type.md) "원본 프레임 코드 표", [실행 안내](docs/sprite-browser.md)
   - 다음 후보: 인라인/줄 명령의 인자 파싱·실행 및 HTML UI, 창·오디오 설정 적용과 사용자 설정 저장, 게임 틱 분석 후 동작별 재생 속도
4. 4단계: 메인 루프/틱, 다리 생성(`Deck.cpp`, `Bridge.cpp`), 경제 분석
5. 5단계(영상 분석)와 병행: 공식 캠페인 구성([mission-script.md](docs/formats/mission-script.md))과 영상 대응 확인
   - 2026-09-28 로컬 영상 4개 형식·화면 영역 확인과 추출 도구 `tools/videoframes.py` 완료 ([docs/videos/README.md](docs/videos/README.md)). **다음: 미션별 관찰 노트(`docs/videos/<이름>.md`) 시작** — 애니메이션·건설·다리 조각 생성 간격·이동 속도처럼 스크린샷으로 잴 수 없는 시간 수치를 우선 측정 (60fps 프레임 번호 기준)
   - **애니메이션 속도 완료(2026-09-28)**: 가이저 24Hz·신전/연기 12Hz, exe 의 "현재 시각 + 간격" 타이머(`Flyer.cpp` 0.04초, `Lightning.cpp` 0.08초)와 `maxFPS = 75` 루프 양자화로 설명 — [animation-timing.md](docs/videos/animation-timing.md), 측정 명령 `videoframes.py cadence`. 다음은 타입별 간격 상수 위치(exe)와 건설·다리 조각 생성 간격 측정
   - **에너지 공급 범위 완료(2026-09-29)**: 스크린샷 측정(일반 30칸·튜토리얼 2 약 14칸)과 exe 전투 옵션 Generator Range(표 14/22/30/38, 14~30 제한, 튜토리얼 2만 Short) 일치 — [elements-energy.md](docs/gameplay/elements-energy.md) 6절, [battle-options.md](docs/exe/battle-options.md). 도움말 `GAME.HLP`(다른 세션 추출)의 "BattleMasters"·튜토리얼 2 안내가 단서
   - Dissolved Alliance! `Playing 1`·`Playing 2`(시점 이동 캡처)도 템플릿 매칭으로 카메라 위치를 구해 섬별로 뷰어와 대조
   - **유닛 재충전 간격(Unit Rate) 정적 분석 완료(2026-09-29)**: 화면 왼쪽 "덱" = 매뉴얼의 Production window. 보유 `setup.cfg`의 `useProductTimers = 0`에서는 Unit Rate 순서대로 10/5/1초이지만, **요새 모드에서는 0.0001초**로 대체된다. 이전에 네트워크 역할 플래그로 추정한 `DAT_00594fb8`은 exe 검사 문자열로 `inFortMode`임을 확인했다. `useProductTimers`가 켜진 별도 경로는 30/15/8초·수량 상한 1이며 요새 모드 예외가 없다. [근거와 구현 기준](docs/exe/production-refresh.md). **남은 일**: 원본에서 배치 직후 아이콘 복귀와 일반 전투의 Unit Rate별 간격을 프레임으로 측정한다.
   - 2026-09-28 추가 캡처 목록·관찰 정리 완료 ([docs/screens/README.md](docs/screens/README.md)). 다음은 Dissolved Alliance! 섬별 캡처로 지면 테마·소유자 테두리 대조, 메뉴 화면 정밀 좌표 노트
   - 섬 소유권 규칙([island-ownership.md](docs/gameplay/island-ownership.md))과 지면 미리보기 대조 — 2026-09-28 확인: `FortTerrainPreview` 는 이미 영역 신전의 소유자·원소로 테두리 색·테마를 정하고 신전이 없으면 중립·`sun`(초록) 으로 그린다 (규칙과 일치). 남은 것은 작은 받침·`createsisland` 발판의 소유자 결정(현재 저장된 오브젝트 소유자 사용)
6. (원격 저장소가 생기면) CI 실제 실행 확인 — `originals/` 포함 후 원본 검증 테스트까지 실행되는지
7. ~~7단계 착수 시 화면 요구사항(풀스크린·16:9/16:10/4:3·가장자리 스크롤, 1.7절)을 먼저 설계에 반영~~ → **2026-09-29 완료** (7단계 첫 두 항목). 7단계 다음 후보: 8bit 인덱스 → 팔레트 적용 방식(CPU 변환 vs 셰이더), 입력·커서, 오디오, 고정 틱 루프·결정론 난수, 로깅

### 참고: 3단계에서 만든 도구 사용 순서

[docs/formats/README.md](docs/formats/README.md) "추출 순서" 참고. 추출 결과(`extracted/`)는 git 에 포함되지 않으므로 새 환경에서는 다시 실행해야 한다.
