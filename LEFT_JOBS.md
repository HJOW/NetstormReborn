# LEFT_JOBS — NetStorm 클론 프로젝트 작업 계획 및 인수인계

> 최종 갱신: 2026-09-30 (`HJOW-X3D`)
> 프로젝트 목표(AGENTS.md): 원본 NetStorm: Islands at War 를 디컴파일/분석하여 클론 코딩하고,
> **Windows 10/11** 과 **GUI 환경의 Linux** 에서 동작하며 **여러 언어를 지원**하는 게임을 만든다.
> **1차 목표 언어: 영어, 한국어** (그 외 언어는 이후 확장).
> **우선순위: Windows 10/11 > Linux** (Linux 지원은 우선순위가 낮다 — 설계상 이식성은 유지하되 검증·배포는 Windows 먼저).
> **화면 요구사항(2026-09-28 AGENTS.md 추가)**: 풀스크린 모드와 화면비 **16:9 · 16:10 · 4:3** 지원, 풀스크린에서 **마우스를 화면 끝에 대면 화면 이동**(원본도 지원) — 1.7절

---

## 2026-09-30 (`HJOW-X3D`, Windows, 수동 녹화 완료) ✅: 한 칸짜리 다리 재측정 — "단일 칸 160초"의 정체는 두 칸 막대, 붕괴 모델은 맞음

- 사용자가 한 칸짜리 조각으로 재측정을 요청해(수동 분석 예외) 새 세션 `20260930T133835395Z-693470346a00`을 시작했고, 사용자가 배치 **전부터** 직접 녹화했다. 종료는 안내 창 닫기 → 관찰 메모 → `end_session force=true`(`closed=true`)로 했고 `Netstorm.exe`는 남아 있지 않다. `originals/`는 수정하지 않았다. 영상 분석이 성공해 `screenShots/`로 옮기지 않았다(AVI 2개·WAV·입력 로그는 세션 `recording/`, Git 제외).
- **결과**(10 FPS, 배치 클릭 13:39:57.91 UTC 기준): 한 칸짜리 조각이 **+32.4초에 금, +72.4초에 낙하**, 금→낙하 **40.0초**, 스캔 위상 2.4초(금·낙하 일치). 모델(금 4번째 스캔 +30~40초, 제거 8번째 스캔 +70~80초)과 맞는다. [화면 목록 1.11절](docs/screens/README.md#111-사용자-직접-조작-녹화-한-칸짜리-다리-붕괴-시간-2026-09-30-windows-hjow-x3d).
- **이전 "단일 칸 약 160초" 관찰 해소:** 이전 증거 세션(`20260929T133717181Z-7f413cde9896`, 이 PC에 있음)의 관심 영역 이미지를 다시 보니 그때 놓은 것은 **두 칸짜리 막대**였다. 해시 전환 시각이 배치 후 +38.9~40.1(바깥 칸 금) / +77.9~79.8(바깥 칸 낙하) / +118.0~119.9(안쪽 칸 금) / +158.6~159.7초(안쪽 칸 낙하)로 칸마다 80초다. 클론의 기존 테스트 `Decay_PlankChainCollapsesFromTheTip`(40·80·90·160초)가 이미 이것을 재현하므로 **모델·코드는 수정할 것이 없다.** 문서의 "단일 칸 120초 보통 + 40초 금" 서술은 [bridge-pieces.md 4절 끝·8.3](docs/exe/bridge-pieces.md)과 [화면 목록 1.5절](docs/screens/README.md) 표에서 정정했다. 세 관찰(두 칸 막대 · T자 · 한 칸) 비교표는 8.3절.
- **남은 것**
  1. **건물형 유닛 파괴·회수 뒤 주변 다리 약화**(금 간 다리는 바로 무너지는지, 5×5 건물의 범위) — 튜토리얼 2 같은 건물 미션에서 별도 실험(아래 절 2번 그대로).
  2. (선택) 튜토리얼 안내 창을 일부러 30초 이상 열어 둔 채 다리 붕괴 스캔(게임 시계)이 멈추는지 확인. 이번까지의 세 관찰은 안내 창이 약 2초만 열려 있어 확인하지 못했다.
  3. (선택) 클론 `BridgeGrid` 테스트에 T자 조각을 추가(이전 절 3번 그대로).
- **참고(이 PC `HJOW-X3D`):** 이전 인수인계들이 "증거 세션이 이 PC에 없다"고 한 `20260929T133717181Z-…` 등은 이 PC에 있다. `extracted/analyzeManager/`의 세션 폴더(게임 복사본 포함)는 각각 약 310 MB이므로 필요 없어지면 사용자가 정리한다(자동 삭제 안 함).
- 변경 파일: `docs/exe/bridge-pieces.md`, `docs/screens/README.md`, 이 문서. 코드 변경 없음.

## 2026-09-30 (`HJOW-X3D`, Windows, 수동 녹화 완료): T자 조각 다리 붕괴 시간 재측정 — 모델과 일치

- 사용자가 수동 컨트롤 분석을 다시 요청해 새 세션 `20260930T132055915Z-667d1a82e7f4`를 시작했고(AGENTS.md 수동 분석 예외), 사용자가 `Tutorial → 1 Bridge the Gap!`에서 다리 조각 한 개를 놓는 과정을 직접 조작·녹화했다. 녹화 종료 후 안내 창을 닫고 관찰 메모를 남긴 뒤 `end_session force=true`(`closed=true`)로 종료했으며 `Netstorm.exe`는 남아 있지 않다. `originals/`는 수정하지 않았다. 영상 분석이 성공해 영상을 `screenShots/`에 옮기지 않았다(AVI 3개·WAV·입력 로그는 세션의 `recording/`, Git 제외).
- **결과**(10 FPS 영상 프레임 단위, 배치 클릭 13:24:55.75 UTC 기준): 놓은 **T자 조각**의 줄기 끝 칸에 **+36.9초 금**, **+77.1초 낙하**(금 간 뒤 40.2초). 가로 막대 동쪽 끝과 나머지 줄기는 녹화 끝(+97초)까지 그대로였다. 금은 4번째 스캔·제거는 8번째 스캔이라는 10초 스캔 수명 모델([bridge-pieces.md 8.1·8.3](docs/exe/bridge-pieces.md))과 맞고, 두 시각이 같은 스캔 위상(약 7.0초)을 준다. 어느 칸이 줄었는지도 8.1의 방문 목록 규칙을 손으로 따라간 결과와 같다(클론으로 이 모양을 돌려 보지는 않음). 상세: [화면 목록 1.10절](docs/screens/README.md#110-사용자-직접-조작-녹화-t자-다리-조각-붕괴-시간-2026-09-30-windows-hjow-x3d).
- **여전히 남은 것**
  1. ~~**단일 칸 약 160초 관찰**~~ → ✅ 위 절에서 해소(한 칸짜리 재측정 + 이전 증거 재검토: 160초는 두 칸 막대).
  2. **건물형 유닛 파괴·회수 뒤 주변 다리 약화**(금 간 다리는 바로 무너지는지, 5×5 건물의 범위) — 튜토리얼 2 같은 건물 미션에서 별도 실험.
  3. (선택) 클론의 `BridgeGrid`에 이 T자 조각을 같은 칸 배치로 넣어 줄기 끝 칸만 금 +30~40초·제거 +70~80초로 줄고 막대 동쪽 끝은 유지되는지 테스트로 고정(조각 모양은 `BridgePiece` 표에서 T자 계열을 골라 영상과 대조 필요).
- **이번에 배운 작업 요령**(자세히는 [analyze-manager.md](docs/analyze-manager.md#사용자-직접-조작-녹화-모드)): PowerShell 5.1의 `>`로 `start_session` 출력을 받으면 게임이 끝날 때까지 안 끝난다 → `cmd.exe /c` + 절대 경로; 안내 창의 단계 표시 시각은 직전 단계의 완료 시각이고 사용자가 사건마다 한 번만 누르도록 문장을 써야 하며, 시간은 영상 프레임 CSV로 잰다(이번 기록의 버튼 시각은 영상보다 최대 10초 늦음); ffmpeg 없이 MJPEG AVI를 Python+Pillow로 읽을 수 있다.
- 변경 파일: `docs/exe/bridge-pieces.md`, `docs/screens/README.md`, `docs/analyze-manager.md`, 이 문서. 코드 변경 없음. 빌드 산출물(`analyzeManager/bin`)은 이 PC에 없어서 Release 빌드를 새로 했다(경고·오류 0, Git 제외 경로).

## 2026-09-30 (`HJOW-X3D`, Windows): 수동 다리 붕괴 분석 중단·안내 창 크기 개선

- 사용자의 중단 요청으로 원본 복사본 세션 `20260930T125406012Z-d15793779d8d`를 끝냈다. 시작 화면만 캡처했고 미션 진입·게임 입력·녹화는 없었다. `end_session force=true`가 `closed=true`를 반환했으며 `Netstorm.exe`는 남아 있지 않다. 세션 보고서와 안내 단계는 `extracted/analyzeManager/20260930T125406012Z-d15793779d8d/`에 보존했다. `originals/`는 수정하지 않았다.
- **안내 창·DPI 수정**: [GuidedForm.cs](analyzeManager/GuidedForm.cs)는 96 DPI 기준 기본 클라이언트 크기 560×620, 최소 창 크기 500×540으로 바꾸고 테두리·크기 조절 손잡이를 추가했다. 이전에는 폰트가 포인트 크기라 125% 배율에서 물리 픽셀 기준 글자가 25% 커질 수 있었지만, 창·컨트롤의 자동 배율은 지정하지 않아 글자와 버튼 공간이 어긋날 수 있었다. `AutoScaleMode.Dpi`를 지정해 글꼴·창·컨트롤을 함께 조정하고, 배율 적용 후 창을 게임 옆에 다시 배치한다. 단계 이동·녹화 버튼·상태는 창 크기에 맞춰 배치하고 본문 영역이 늘어난다. 창이 화면 작업 영역을 벗어나거나 게임과 겹치면 크기를 유지하며 옆으로 다시 배치한다. [사용법](docs/analyze-manager.md)에 반영했다.
- **확인**: `analyzeManager` Release 빌드 성공(컴파일 오류 0). NuGet 취약성 피드에 연결할 수 없어 NU1900 경고 2개가 나왔다. `git diff --check` 통과. 실제 GUI 표시 확인과 테스트는 하지 않았고 원본 게임도 재실행하지 않았다.
- **미완료 분석** → ✅ 수동 녹화는 **위 절(2026-09-30 `HJOW-X3D`, 수동 녹화 완료)에서 수행**했다(사용자가 수동 분석을 다시 요청해 중단 요청은 해소됨). T자 조각 결과는 모델과 맞았고, 이어 한 칸짜리 재측정으로 단일 칸 불일치도 해소했다(맨 위 절). 건물형 유닛 파괴·회수 뒤 주변 다리 약화 실험만 남았다.

---

## 2026-09-30 (`HJOW-Athlon`, Windows): 미션 Game 메뉴·재시작·안내 창 Esc 동작 구현 ✅

- 이전 원본 자동 분석(아래 두 절)에 따라 미션 Esc는 상단 Game 메뉴를 표시·숨기고, Game → Restart Mission은 현재 미션을 다시 로드해 첫 브리핑으로 돌아간다. Game → Leave Mission은 확인 창을 띄우고 Main Menu·Replay Mission·Continue Mission 버튼을 처리한다. Replay는 같은 미션을 다시 로드한다. 현재 클론에는 메인 메뉴 화면이 없으므로 Main Menu 선택은 개발용 기본 화면으로 돌아간다.
- 안내 창에서는 원본처럼 Esc가 반응하지 않도록 수정했다. 메뉴·확인 창을 여는 동안 지도 입력과 가장자리 스크롤을 막고, 미션 시간은 계속 진행한다. 메뉴를 클릭하면 화면 전환을 게임 본체가 처리한다.
- **검증**: Windows에서 Release 빌드 경고·오류 0, Assets 175개·Core 147개 테스트 통과. 클론 `--mission tutorial1 --window 1024x768`를 실제 실행해 첫 안내 창에서 Esc가 창을 닫지 않음, 안내 버튼으로 페이지 이동, Esc → Game → Leave Mission 확인 창, Replay Mission과 Restart Mission의 첫 `NetStorm!` 브리핑 복귀, Main Menu 선택 시 개발용 기본 화면 복귀를 확인했다. 실제 원본과 메뉴 그림의 픽셀 단위 일치는 확인 대상에 포함하지 않았다.
- 변경 파일: `src/Netstorm.Game/{FortMapViewer,FortMapViewer.TutorialDialog,FortMapViewer.MissionMenu(신규),NetstormGame}.cs`, [뷰어 사용법](docs/map-viewer.md), 이 문서.

## 2026-09-30 (`HJOW-Athlon`, Windows, 원본 자동 분석): Replay·Restart Mission 첫 화면 확인 ✅

- `analyzeManager` 세션 `20260930T115011876Z-a2dcd80d0ab9`에서 `Bridge the Gap!`에 들어가 `Leave Mission → Replay Mission`을 선택했다. `Starting Mission...` 로딩 창 뒤 같은 미션의 첫 `NetStorm!` 브리핑과 **0 SP** 화면이 다시 나타났다. 증거 해시: `326a0af414b6885b7832532bcc98fd71380418f116db105354d1dbf7a5982a0e`.
- 브리핑을 닫고 `Game → Restart Mission`을 선택해도 같은 로딩 창 뒤 첫 브리핑과 **0 SP**가 다시 나타났다. 증거 해시: `fd783fb1cdfaacec8f0a7cd2cbfe919ae7260bd7d4f6a6b29b935167217d63fb`. 배치물 초기화는 이 세션에서 시험하지 않았다. 관찰 메모를 남기고 게임을 `end_session force=true`로 종료했다.
- [화면 관찰 상세](docs/screens/README.md#19-replay-missionrestart-mission-전환-2026-09-30-windows-hjow-athlon). 아래 인수인계의 미확인 항목 중 두 전환을 완료로 표시했다. 변경 파일: `docs/screens/{README,main-menu}.md`, 이 문서.

## 2026-09-30 (`HJOW-Athlon`, Windows, 원본 자동 분석): Original Credits 마지막 전환 확인 ✅

- `analyzeManager` 세션 `20260930T113718545Z-fda9e5c1745b`에서 원본 복사본을 실행하고 Original Credits를 입력 없이 관찰했다. 선택 입력은 11:39:09.541 UTC, 마지막 쪽 캡처는 11:43:34.299 UTC, 메인 메뉴가 다시 보인 캡처는 11:43:49.931 UTC다. **선택부터 복귀 관찰까지 280.39초**로, 스크립트의 14쪽 × 각 20초와 일치한다. 복귀 증거 SHA-256: `198f9dda482552b67c70baf6430e013f7972f0bb845600df645f037671ea60a9`(중앙 ROI).
- 이후 입력 없이 `The Storm Rages!` Auto-Demo로 전환한 것도 확인했다. 메인 메뉴 복귀와 데모 화면 사이 캡처 간격 때문에 데모 시작의 정확한 시각은 이번 측정으로 확정하지 않는다. 세션에 관찰 메모를 저장하고 게임을 `end_session force=true`로 종료했다. 앞서 중단된 세션 `20260930T113311036Z-0b577dc139c4`도 실행 중인 프로세스가 없음을 확인해 종료 처리했다.
- [화면 관찰 상세](docs/screens/README.md#18-original-credits-마지막-전환-2026-09-30-windows-hjow-athlon). 이 항목의 미확인 상태를 아래 인수인계 목록에서 완료로 변경했다. 변경 파일: `docs/screens/{README,main-menu}.md`, 이 문서.

## 2026-09-30 (`vm-debian-codex`, Linux, 원본 게임 실행 없음): 수동 분석 안내 창의 게임 크기 변경 대응 ✅

- `guide` 안내 창을 처음 열 때뿐 아니라 1초마다 주 게임 창의 크기·위치를 확인해, 바뀌거나 서로 겹치면 게임 바깥의 오른쪽/왼쪽 빈 영역으로 옮긴다. 녹화 시작 전에도 적용된다. 게임 안의 작은 대화상자는 배치 기준에서 제외하고, 제목 표시줄과 테두리도 피한다. 창의 최초 위치가 유지되도록 수동 시작 위치를 지정했다.
- 옆 공간이 없어지면 상태 메시지를 표시하고 진행 중인 녹화를 중단한다. 공간이 다시 생기면 안내 창을 재배치한다. [수동 조작 녹화 사용법](docs/analyze-manager.md#사용자-직접-조작-녹화-모드)에 반영했다.
- **검증**: Linux에서 분석 도구와 테스트 프로젝트 Release 빌드 오류·경고 0. Wine 단위 테스트 27개 중 26개 통과·1개 기존 심볼릭 링크 검사 건너뜀. 새 배치 테스트 4개는 창 축소·왼쪽 배치·보조 모니터·공간 부족을 검사한다. 실제 게임과 수동 안내 GUI는 실행하지 않았으므로 Windows 화면에서 이동 시점과 배치가 맞는지는 후속 확인이 필요하다.
- 변경 파일: `analyzeManager/{GuidedForm,WindowsGame,GuidePlacement}.cs`, `analyzeManager/tests/GuidePlacementTests.cs`, `docs/analyze-manager.md`, 이 문서.

## 2026-09-30 심야 분석 + Windows 부분 확인: 방화벽 경고 원인·포트와 `PREPARE.ps1` TCP 6799 항목

- **질문**: Windows PC 수동 분석 중 방화벽 경고가 떴다 — 분석 프로그램이 쓰는 포트가 있는지, 고정할 수 있는지.
- **결론** ([network-ports.md](docs/exe/network-ports.md)): 분석 도구(analyzeManager)는 포트를 쓰지 않는다. 경고의 원인은 **게임 복사본 `Netstorm.exe` 가 전투 시작 때 TCP 6799(`gameServerPort`)를 0.0.0.0 으로 리슨**하는 것이다. 메인 메뉴에서는 열리지 않고 한번 열리면 종료까지 유지된다. 세션마다 게임을 새 경로로 복사하므로 경로 기준인 방화벽 규칙에 세션마다 걸릴 수 있다. 포트는 이미 6799 로 고정이고 `setup.cfg` 의 `gameServerPort` 로 바꿀 수 있지만(실측 6899 로 바뀜) **포트를 고정해도 경고는 없어지지 않는다.**
- **Wine 실측** (`ss -ltnup` 기준 대조): 시작·메뉴 = 새 리슨 없음, 자동 데모 시작 순간 `0.0.0.0:6799`, 메뉴 복귀 후에도 유지, 종료 시 닫힘, 8998/6800/6802/80 은 어느 시점에도 없음. 복사본 setup.cfg 에서 포트를 6899 로 바꾸면 6899 만 열림(원본·저장소는 그대로, 임시 저장소는 삭제).
- **정정**: `R.exe` 는 자동 업데이트 도구가 아니라 **NETSTORM Root Server**(LAN 서버, 6800/6802/8998 관련)다. 관련 문서 세 곳 수정.
- **사용자 결정(2026-09-30)**: (1) 클라이언트 유효성 검사("Not Validated")는 구현하지 않는다. (2) 멀티플레이는 후순위로 나중에 구현한다. (3) 6799 방화벽 경고는 **임시로 6799 방화벽 예외를 미리 등록**해 해결한다. → 문서 반영: [network-ports.md 1-1·7·8절](docs/exe/network-ports.md), [main-menu.md](docs/screens/main-menu.md), 이 문서의 표(13단계)·9단계·5절.
- **`PREPARE.ps1`(Windows용) 방화벽 항목 추가**: 항목 "방화벽 예외 (TCP 6799)", 관리자 권한 검사(`Test-IsAdministrator`), 비관리자면 선택 화면에 "[사용 불가]"로 표시하고 `-All`·`-CheckOnly` 에서도 건너뛰어 결과에 "사용 불가"로 남김, 규칙 점검(`Get-FirewallRuleStatus`)·등록·재점검. 규칙은 인바운드 TCP 6799 허용, 프로필 Any, 원격 범위 `LocalSubnet`(`$FirewallRemoteScope` 로 변경). 기본 선택은 해제(보안 설정 변경). `PREPARE.sh` 는 손대지 않았다(Linux 는 경고 창이 없음).
- **사용자 Windows 확인(2026-09-30)**: TCP 6799 방화벽 허용 기능이 정상 동작했고, 관리자 권한이 없는 경우 해당 항목을 선택할 수 없는 것도 확인했다.
  - **추가 Windows 확인 필요**: `-All`·`-CheckOnly` 결과 / 규칙의 포트·프로필·원격 범위 / 반복 등록 시 중복 방지 / 되돌리기 명령 / Windows PowerShell 5.1·7 호환 / **게임 실행 시 방화벽 경고가 실제로 사라지는지**. 자세한 목록: [network-ports.md 7.2](docs/exe/network-ports.md).
  - 검증한 것: Linux 의 PowerShell 7.6 파서로 문법 오류 없음, 파일 UTF-8 BOM·LF 유지. 방화벽 cmdlet 을 모의한 비관리자 경로 한 번은 "사용 불가" 행을 냈으나 다른 항목이 Linux 에 없는 명령으로 실패해 검증으로 치지 않는다(PowerShell 도구는 스크래치패드에만 설치, 저장소 변경 없음).
- **클론 메모**: 클론의 싱글 플레이는 소켓을 열지 않는다(방화벽 경고 없음). 6799 사전 등록은 원본 분석 때만 필요한 임시 조치. 멀티플레이 구현 때만 네트워크(원본 호환 기본값 6799/6800/8998 참고).
- **부수 발견**: 시작 후 창 크기가 1600×828 → 1024×768 로 바뀌는 시점에 안내 창 클릭 좌표가 달라져 첫 클릭이 빗나갔다(이전 인수인계의 "창 크기 변경 원인 미확인"과 같은 현상). 클릭 전 `capture_state` 로 확인할 것.
- 변경 파일: `PREPARE.ps1`, `docs/exe/network-ports.md`(신규), `docs/analyze-manager.md`, `docs/screens/main-menu.md`, `docs/sources/patch-history.md`, 이 문서.

## 2026-09-30 밤 (`vm-debian-codex`, Linux, 원본 게임 실행 없음): 다리 붕괴 알고리즘을 원본 칸 단위 처리로 교체 ✅

- **한 일**: 이전의 근사("열린 끝이 있는 연결망 전체가 함께 줄어듦")를 원본 `FUN_004227e0`·`FUN_004218b0`·`FUN_004217f0`·탐색기 필터·`Graph.cpp` 그래프 검사로 교체했다. 전체 알고리즘과 근거: [다리 분석 8.1절](docs/exe/bridge-pieces.md), 클론 구현 표: 8.2절.
  - 구동자: 표면 그래프 5칸 미만이면 즉시 제거 → 단단하면 제외 → 열린 쪽 없으면 제외(B~E 2개 이상, F~K 1개 이상, L~O 항상, A·P 안 됨) → 접합 칸(A~I)으로만 재귀한 방문 목록의 0 아닌 최소 수명 − 1(없으면 7)로 맞춤.
  - 결과: 섬에 붙은 판자 사슬은 **바깥 끝부터** 한 칸씩(관찰 "두 칸짜리는 끝 칸이 먼저", "이어 붙이면 멈췄다가 끝 칸이 사라진 뒤 다시 줄어듦"을 설명), 접합 무리는 끝 판자와 함께(끝이 둘이면 스캔마다 두 번), 섬에서 떨어진 5칸 미만 조각은 즉시 붕괴.
- **검증**: 원본 맵 336개 저장 다리 90,290칸을 300초 돌리면 이전 구현은 **57%**를 없애고(공식 맵 다리가 거의 사라지는 비현실적 결과) 새 구현은 **3.2%**(주로 섬에서 떨어진 5~6칸 무리와 끝 칸)만 사라진다. 회귀 테스트 `OriginalMaps_StoredBridgesMostlySurviveFiveMinutes`. Release 빌드 오류 0, Core **147**(기존 143 + 신규: 판자 사슬·접합 무리·작은 조각·단단한 끝 칸·저장 다리 생존, 옛 연결망 수명 공유 테스트 교체)·Assets 175 통과.
- **새로 알게 된 것**
  - 방문 목록 용량 100(`0x4fc640`의 `push 0x64`), 이웃 목록 10. 금은 "이전 수명 ≥ 5, 새 수명 < 5"일 때만 가서 수명 0 인 새 칸이 4 로 바로 맞춰지면 금이 가지 않는다.
  - 스캔 주기는 `round(8001 × 프레임 시간 ÷ 10)`개씩이라 프레임률이 아주 높으면 최대 1.5배까지 늘 수 있다(클론은 10초 고정).
  - **전체 디컴파일에 없던 함수 복구법 확립**: 탐색기 필터 `FUN_004b1e80`은 가상 함수 표로만 호출되어 빠져 있었다. 새 도구 `tools/ghidra/decompile_at.sh`(Linux, 약 5초)로 디컴파일해 연결 방향 검사 `FUN_00441e40`을 찾았다 → [사용법](docs/exe/mission-header-flags.md#7-ghidra-전체-디컴파일에서-빠진-함수-재현-방법-포함). 이전 인수인계의 "Linux 에서 `DecompileAt.java` 직접 실행은 미시도"가 해소됐다.
- **근사·미확정**: 칸 처리 순서(원본은 오브젝트 번호 순, 클론은 만든 순서), 여러 칸짜리 섬 오브젝트를 칸 단위로 센 것, 접합 칸 고리(원본은 무한 재귀라 실제로 생기는지 모름 — 클론은 재귀 경로로 막음), 새 수명 ≤ 5 인 칸 위 이동체 낙하 미구현.
- **아직 설명 못 한 것**: 섬에 붙은 **단일 칸이 약 160초** 걸린 관찰(모델은 80초 안팎). 8.3절에 가능성 정리. 원본에서 캡처 간격을 좁힌 재측정(사용자 직접 조작 녹화 권장)이 필요하다. → ✅ **2026-09-30 해소**: 그 관찰은 두 칸 막대였고 한 칸은 금 +32초·낙하 +72초(맨 위 절).
- **다음 후보**: (1) 위 재측정, (2) 누락 함수 후보 528개 중 게임 로직 쪽 우선 디컴파일(`decompile_at.sh` 사용), (3) 붕괴 시 칸 위 이동체 낙하(`FUN_004202f0`~`FUN_00426120`)는 이동 구현 때, (4) 다리 연결·소유권 전파(`FUN_00421240`·`FUN_004213b0`)는 영역 소유 구현 때.
- 변경 파일: `src/Netstorm.Core/Bridges/BridgeGrid.cs`(칸 순번, `IsOpen` 제거, 붕괴 재작성), `tests/Netstorm.Core.Tests/{BridgeGridTests,BattleSessionTests}.cs`, `tools/ghidra/decompile_at.sh`(신규), `docs/exe/{bridge-pieces,mission-header-flags}.md`, `docs/{core-rules,map-viewer}.md`, `docs/formats/README.md`, 이 문서.

## 2026-09-30 저녁 (`vm-debian-codex`, Linux, 원본 게임 실행 없음): 다리 품질·주변 약화 정적 분석 + 구현

- **사용자 추가 규칙(2026-09-30)**: 다리 붕괴 조건은 시간만이 아니다. **건물형 유닛이 파괴될 때 바로 옆에 인접한 다리도 1단계 약화되고, 이로 인해 붕괴될 수 있다.** → [섬 소유권 규칙](docs/gameplay/island-ownership.md) 3번, [다리 분석 8.5절](docs/exe/bridge-pieces.md#85-건물형-유닛이-없어질-때-주변-다리-약화-2026-09-30-사용자-확인--exe-확인--구현).
- **exe 확인**
  - 주변 약화: 전투 오브젝트 공통 제거 처리 `FUN_0044b9e0`(vtable 슬롯 0x18). `maxHitPoints`가 있고(타입 플래그1 0x10) walker·balloon·flyer가 아닌 오브젝트가 없어지면 중심 칸 ±2 의 다리: 금 감 → 제거, 보통 → 금 감·수명 4(`FUN_00421db0`), 단단함 → 그대로. 제거 이유를 보지 않으므로 회수에도 적용된다고 추정.
  - 조각 품질: 생산 칸 조각은 **들어온 뒤 6초 동안 금 간 품질**(타이머 0x3c × 0.1초), 그 뒤 보통. 금 간 채로 놓으면 금 간 프레임·수명 4, 보통이면 수명 0(`FUN_00442c80` `param_10`, 로컬 배치 `FUN_004433b0`은 조각 `+0x1e`). 튜토리얼 C1 "Bridge Quality" 설명과 맞다.
  - 폭발(`Bomb.cpp`) 가장자리의 판자는 끝 칸(L~O)·수명 4 로 바뀐다(`FUN_004215d0`).
  - 단일 칸 "120초 뒤 금 감" 관찰은 exe 조건으로 설명되지 않았다(모델 30~40초, 같은 측정의 서쪽 칸 51초 관찰은 모델과 일치). 재측정 필요(→ ✅ 2026-09-30 해소: 두 칸 막대였음, 맨 위 절) — [8.3절](docs/exe/bridge-pieces.md#83-원본-관찰과의-차이-남은-확인).
- **구현(Core)**: `BridgeTray.QualityAt`·`CrackedDeciseconds`, `BridgePiece.CuredAtDeciseconds`, `BridgeGrid.Place(…, quality)`·`WeakenAround`·`WeakenedTimeLeft`·`WeakenRadius`, `BattleSession`의 다리 놓기(품질 적용)·회수 뒤 `WeakenBridgesAround`(이벤트 `BridgeCracked`·`BridgeCollapsed`), 검사합에 조각 품질 타이머 추가. 전투 파괴는 아직 없으므로 구현되면 같은 함수를 부를 것.
- **정리**: `analyzeManager/ExplorerTools.cs` `start_session` 설명에 시스템 2(`192.168.0.94`·`HJOW-Athlon`)를 반영했다(아래 2026-09-29 절의 미반영 항목 해소, `Program.cs` 도움말은 이미 일반화되어 있었음).
- **검증**: Release 솔루션 빌드 오류 0(기존 CA2014 경고 1), analyzeManager 빌드 오류·경고 0, Core **143**(기존 138 + 신규 5: 금 간 품질 배치·주변 약화·트레이 6초·세션 품질 배치·회수 약화)·Assets 175 통과. 원본·뷰어 화면은 실행하지 않았다.
- **근사·미확정**: 중심 칸은 `Footprint.CenterX/Y` 버림(짝수 크기 발자국은 원본과 한 칸 어긋날 수 있음), 들고 있는 조각의 품질 변화, 회수 시 약화(수신 쪽 경로 미확인), 늦춰진 낙하 예약(이벤트 0x2692) 미구현.
- **다음 후보**: (1) 원본에서 단일 칸 붕괴 시각·주변 약화(파괴/회수)를 사용자 직접 조작 녹화로 확인, (2) ~~`FUN_004218b0` 조건을 칸 단위로 그대로 옮겨 "열린 끝이 있는 연결망" 근사 교체~~ → 위 밤 절에서 완료, (3) 끝 칸 변환(`FUN_004215d0`)·폭발 피해는 전투 구현 때.
- 변경 파일: `src/Netstorm.Core/Bridges/{BridgeGrid,BridgePiece,BridgeTray}.cs`, `src/Netstorm.Core/Simulation/{BattleSession,BattleSession.Commands}.cs`, `tests/Netstorm.Core.Tests/{BridgeGridTests,BridgePieceTests,BattleSessionTests}.cs`, `analyzeManager/ExplorerTools.cs`, `docs/exe/bridge-pieces.md`, `docs/core-rules.md`, `docs/gameplay/island-ownership.md`, 이 문서.

## 2026-09-30 후속 분석 후보 — 공식 캠페인 1-1 The War Begins!

- **공중 공격 기지 검증**: 미션 시작 지식에 `sunAviary`가 있고 Sun Workshop 등록 목록에 Whirlibase가 나타난다. 기지를 건설해 적이 사정거리 안에 들어왔을 때 공격체(Whirligig)가 생성·발진·이동·공격하는 조건과 종료 과정을 원본 캡처·타입·exe와 대조한다. 비행형 수송 유닛의 이륙·착륙과는 별개 경로인지 확인한다.
- **적 사제 포획·Altar**: 적 High Priest의 체력 절반 기절·보호막 모습, 수송 유닛으로 **포획해 데려오는 절차**(Storm Power 환급용 `Salvage`와 다름), Altar 건설·사제 희생·지식 획득 및 중단 조건을 재현한다. 기존 `Playing 5`·`Playing 8` 캡처에 제단과 희생 중으로 보이는 장면이 있다. 상세 관찰 항목: [캠페인 1-1 분석 시나리오](docs/screens/the-war-begins-start.md#0-a-후속-분석-시나리오-공중-공격-기지와-사제-포획altar).
- **실험 조건**: `thewarbegins.english`의 `aiStartMoney = 0`은 적의 **시작 자금**이며 `aiCollectors = 1`, `aiGeyserAttachments = 1`도 설정되어 있다. 따라서 이후에도 적 자금이 0으로 유지된다고 가정하지 않는다. `myStartMoney = 3000`, `myTech = "sunWalker;rainBattery;sunAviary;sunCannon"`과 이미 배치된 적 방어 유닛도 기록한다. 이번 항목은 향후 작업 문서화이며 게임 실행·구현은 하지 않았다.

## 2026-09-30 사용자 추가 규칙 — 비행형 이동·공중 공격 (문서·주석 반영 완료, 기능 구현은 후속)

- 비행형 이동유닛은 **출발 시 이륙 → 이동 → 목적지 착륙** 과정을 거친다. 튜토리얼 1·2에는 아직 등장하지 않는다. 현재 `MovementRate`는 타입별 `speed`만 읽고 세션 이동은 사제 수집에만 적용하므로, 다음 이동 구현 시 비행 상태·시간·높이·착륙 위치/점유·이동 중 명령 변경을 별도로 분석한다.
- 일부 **건물형 유닛**은 적이 사정거리 안에 들어오면 비행형 공격 유닛을 생성해 보내 공격한다. `Air Attack Base` 타입(Whirlibase·Devil Maker·Man o'War Pool)과 `flyer` 공격체(Whirligig·Dust Devil·Man o'War)를 구분해, 사정거리 감지·생성·목표 선택·공격체 생명주기를 후속 분석한다. 이 공격체에 공중 수송 유닛의 이륙·착륙 규칙이 동일하게 적용되는지는 미확인이다.
- 반영 위치: `Simulation/MovementRate.cs`, `Rules/ObjectKind.cs` 주석, [규칙 코어](docs/core-rules.md), [이동·건설 분석](docs/exe/priest-construction.md), [섬 소유권 용어](docs/gameplay/island-ownership.md). 이번 변경은 설명만 추가했고 게임 동작은 바꾸지 않았다.

## 2026-09-30 (`vm-debian-codex`, Linux, 원본 게임 실행 없음): 수집 경제·튜토리얼 1 및 사제 이동·건설 정적 분석

- **후보 1 수집 경제 완료**: `HarvestGeyserCommand`로 소유 사제가 섬·내 다리 연결망을 따라 가이저와 완공 신전을 반복 왕복한다. 결정 하나를 전달할 때 200 SP를 더한다. 다리가 바뀌면 경로를 재탐색하고 길이나 대상이 사라지면 멈춘다. 운반량·경로·진행량을 검사합에 넣었다. 뷰어에서는 가이저 위 **H**, 스크립트에서는 `harvest x,y`로 시작한다.
- **튜토리얼 1 A~G 단계 추가**: F4 또는 첫 다리 → 다리 8·19칸 → 가이저 연결 → 200·600 SP. 원본 `FUN_004c3a20`의 경계값과 exe 상수 `0x510298 = 200.0f`, `0x514c34 = 600.0f`를 확인했다. `bridgethegap.fort`에는 가이저가 없고 원본은 `FUN_00486440`/`00486360`으로 동적 생성하므로, 현재는 동쪽에 연결 가능한 5×5 연습 받침·3×3 가이저를 결정적 위치에 만든다. 뷰어 F4와 `home` 스크립트도 연결했다.
- **후보 3 사제 이동·건설 분석 완료 범위**: `priest.type`의 `speed = 1.8`은 타입 구조체 `+0xe0`에 파싱된다. 사용자 지적대로 Golem 2.0, Balloon 1.9, Sail Skater 3.4, Crystal Crab 2.4 등 **유닛마다 속도가 다르다**. 공통 `MovementRate`가 각 타입의 값을 읽고 사제 수집 이동에 사용한다. `Construction.cpp` `00442c80` → `00442b50`에서 타입 비용을 배치 처리 중 차감함을 확인했다. `constructionRate`는 구조체 `+0x50`에 파싱되고 생략 시 기본값 **10.0**이 채워진다. 실제 소비·시간 계산식은 미확인이라 기존 관찰 기반 건설 시간(템플 16초, 워크샵 10초)을 유지했다. [상세 분석](docs/exe/priest-construction.md).
- **검증**: `dotnet test Netstorm.sln -c Release --no-restore` 성공 — Assets **175**, Core **138**, 실패 0. 연습 가이저 연결 전 수확 거부, 다리 연결 후 3회 운반 = 600 SP, 튜토리얼 1 완료, 유닛별 속도 테스트를 포함한다. 원본 게임과 뷰어 화면은 실행하지 않아 시각 배치·입력을 화면에서 확인하지 않았다.
- **남은 일**: (1) 원본 `speed` 값의 실제 칸/초 변환, 곡선 경로·충돌·다른 이동형 유닛 명령 구현; (2) `constructionRate` 소비 경로·사제의 건설 현장 이동·중단/환불 규칙을 찾아 현재 고정 시간을 교체; (3) 튜토리얼 1 가이저의 실제 생성 위치와 다리 **누적 제작 수** 판정 복원(클론은 현재 살아 있는 다리 칸 수); (4) 생성 가이저 받침과 안내 창 흐름의 실제 GUI 확인. 실행 확인 규칙은 AGENTS.md의 해당 시스템 예외를 따른다.
- 변경 파일: `src/Netstorm.Core/Simulation/{BattleSession.Harvest,MovementRate,TutorialGeysers}` 및 세션·명령·이벤트·단계·팩토리, `Bridges/BridgeGrid.cs`, `src/Netstorm.Game/FortMapViewer*`, `tests/Netstorm.Core.Tests/{HarvestEconomyTests,TutorialStagesTests}.cs`, `docs/{core-rules,map-viewer}.md`, [분석 문서](docs/exe/priest-construction.md), 이 문서.

## 0-A. 최신 인수인계 — AI용 원본 분석 도구 (2026-09-29)

### 2026-09-30 (`vm-debian-codex`, Linux, 게임 실행 없음): 튜토리얼 안내 창 ✅ (후보 2번 완료)

- `TutorialTell` 이벤트를 원본 미션 스크립트 섹션과 연결했다. `TutorialDialogScript`가 제목·본문의 HTML 부분집합·조건 평가 후 `$Button`을 읽고, 뷰어가 모달 창으로 그린다. 창이 열린 동안 세션 틱·지도 입력·가장자리 스크롤이 멈춘다.
- MORE/BACK(`Tell`), OK(`DoNothing`), Leave Tutorials(`LeaveBattle`), Next Tutorial(`MissionBegin`)을 연결했다. F8은 가장 최근 단계의 시작 안내로 돌아가며 보정 안내는 복귀 지점을 바꾸지 않는다. 버튼이 없는 F1.·보정 안내에는 닫기 버튼을 추가했다.
- [사용법](docs/map-viewer.md#튜토리얼-안내-창). 원본 그림 명령은 자리표시자이고 원본 창과 동일한 그림·정확한 배치는 아직 구현하지 않았다. 튜토리얼 1·3~6도 A. 안내는 자동으로 열리지만 이후 단계는 자동 처리되지 않는다.
- 정적 검증: Release 솔루션 빌드 성공(기존 CA2014 경고 1건), Assets 테스트 175개 통과(신규 3개에 원본 튜토리얼 2 스크립트 포함). 게임 화면은 실행하지 않았으므로 실제 화면 배치·클릭 확인은 남는다.
- 변경 파일: `src/Netstorm.Assets/TutorialDialogScript.cs`, `src/Netstorm.Game/{FortMapViewer.TutorialDialog,FortMapViewer,FortMapViewer.Session,NetstormGame}.cs`, `tests/Netstorm.Assets.Tests/TutorialDialogScriptTests.cs`, `docs/map-viewer.md`, 이 문서.

### 2026-09-30 오후 (`vm-debian-codex`, Linux, 원본 실행 없음): 튜토리얼 2 단계 처리 구현 ✅ (후보 1번 완료)

- **선행 결함 수정**: `BattleSession.CreatePlayer`가 `MissionStart.Tech` 표를 그대로 플레이어에게 넘겨, 같은 미션으로 만든 두 세션이 표를 공유했다. `TechPermissions.Clone()`을 쓰도록 고쳤다(수정을 되돌리면 새 테스트가 실패함을 확인).
  또 `Checksum()`에 기술 허용 표·회수 금지가 없었다 → 넣었다(이름 순서로 섞어 Dictionary 순서에 의존하지 않음).
- **exe 확인(정적, 2026-09-30)** → [mission-header-flags.md](docs/exe/mission-header-flags.md) 3.5절
  - 개수 표 세 개: `DAT_005c94d0` = 내 현재 개수, **`DAT_005c98d0` = 내가 지은 누적 수**(튜토리얼 단계 조건이 읽는 것, 파괴·회수는 줄이지 않고 전체 삭제 `FUN_004c27c0`만 줄임), `DAT_005c9cd0` = 전체 현재 개수. 이전 문서의 "누적인지 현재인지 미확인"을 해소했다.
  - 타이머 상수를 exe 바이트에서 읽었다: `0x506588` = **4.0초**(단계 F), `0x506590` = **2.0초**(단계 C의 `NotVortex`·단계 H). 단계 C·F가 읽는 `FUN_004d5430` = 선택한 오브젝트 번호(`DAT_005caea0`), `FUN_004d5c60` = 선택 해제.
  - 단계 넘김(`FUN_004c33f0`)은 글자 +1·타이머 지움·잠금 카운터 10, 잠금이 풀린 뒤에만 단계 함수가 호출된다(프레임 함수 `FUN_004c34c0`).
- **구현** (Core): `Simulation/TutorialStages.cs`(튜토리얼 2 단계 A~I), `SelectEntityCommand`, `PlayerState.Made/MadeWithFlags/SelectedEntityId`, `SessionEventKind.TutorialTell`(단계 섹션 이름 알림), `BattleSession.Tutorial/RunsTutorial`. 단계 A 옵션 덮어쓰기·B `sunFactory` 허용·H 회수 금지 해제를 세션이 자동으로 한다.
  건물은 **완공**, 유닛은 **배치**할 때 지은 수로 센다(출생 콜백 시점은 스크립트 문구로 추정 — 미확정).
- **뷰어**: `T` 키(커서 칸 오브젝트 선택/해제), `--script select x,y|none`, HUD에 튜토리얼 단계·선택 표시, 안내 이벤트를 알림줄에 표시. Linux에서 `--mission tutorial2`로 A→G 진행과 회수 거부, 단축 스크립트(회수 +75, 3,675 SP)를 실제 실행 확인했다. [사용법](docs/map-viewer.md)
- **검증(확실)**: Release 솔루션 빌드 오류 0(기존 CA2014 경고 1), Assets 172·Core **135**(기존 128 + 신규 7: 세션 표 독립, 검사합 범위, 단계 A~I 걷기, 결정론, 누적 수, 선택 명령, 단계 끄기/미구현 튜토리얼) 통과, Core 4회 반복 안정. 원본 게임은 실행하지 않았다.
- **근사·미확정**: 단계 넘김 뒤 잠금(10 카운트, 안내 창 표시·닫힘 조건으로 추정)은 "다음 틱부터 검사"로 대신함. 출생 콜백 시점(건설 시작/완공). `[NotVortex]` 안내는 항상 이벤트로 내보냄(원본은 그 섹션이 있을 때만). 안내 창 UI 없음.
- **다음 후보 (우선순위 순)**
  1. ~~**수집 경제**(튜토리얼 1의 D~F가 의존): 가이저 → 사제 결정 운반(결정당 200 SP), 튜토리얼 1 단계 추가.~~ → **2026-09-30 완료 범위**(위 절).
  2. ~~튜토리얼 안내 창(9단계 UI): `TutorialTell` 이벤트의 섹션 본문(HTML 부분집합·`$Button=`)을 띄우고 F8로 다시 보기.~~ → **2026-09-30 완료**(위 절).
  3. **사제 이동·건설 절차 분석** → 타입별 이동 속도·배치 시 비용 차감은 **2026-09-30 확인**. `constructionRate` 계산과 건설 현장 이동은 남음(위 절).
  4. Ghidra 누락 함수 목록(후보 528개) — `FUN_004c34c0`의 `FUN_004c8e90`·`FUN_00460de0` 의미도 여기서 확인.
- 이번 변경 파일(커밋 전): `src/Netstorm.Core/Simulation/{TutorialStages(신규),BattleSession,BattleSession.Commands,PlayerState,GameCommands,SessionEvents}.cs`, `src/Netstorm.Core/Rules/MissionStart.cs`, `src/Netstorm.Game/{FortMapViewer,FortMapViewer.Session}.cs`, `tests/Netstorm.Core.Tests/{TutorialStagesTests(신규),BattleSessionTests}.cs`, `docs/{core-rules,map-viewer}.md`, `docs/exe/mission-header-flags.md`, 이 문서. (`analyzeManager` 관련 이전 Linux/Wine 변경은 이미 커밋됨)

### 2026-09-30 오전 (`vm-debian-codex`, Linux, 원본 실행 없음): 디컴파일 준비만 하고 사용자 요청으로 중단

- **한 일 (코드·문서 변경 없음, 커밋할 것 없음)**
  - 이 PC에는 디컴파일 결과가 없어 Ghidra 12.1.4(`~/Tools/ghidra_12.1.4_PUBLIC`, JDK 25)로 `originals/Netstorm.exe`를 다시 디컴파일했다. **함수 4,506개**, `extracted/decomp/Netstorm.c` 183,825줄, 소요 약 3분. `extracted/`는 Git 제외라 커밋 대상이 아니다.
  - `run_decomp.ps1`은 PowerShell 전용이고 이 PC에는 `pwsh`가 없다. 같은 일을 하는 Linux 명령(저장소 루트에서):
    ```bash
    mkdir -p extracted/ghidra extracted/decomp extracted/ghidra-settings extracted/ghidra-cache
    export XDG_CONFIG_HOME=$PWD/extracted/ghidra-settings XDG_CACHE_HOME=$PWD/extracted/ghidra-cache
    ~/Tools/ghidra_12.1.4_PUBLIC/support/analyzeHeadless extracted/ghidra Netstorm -import originals/Netstorm.exe -overwrite \
      -scriptPath tools/ghidra -postScript ExportDecomp.java $PWD/extracted/decomp/Netstorm.c
    ```
  - `python3 tools/taff.py extract originals/netstorm.tarc extracted/tarc`로 아카이브를 풀었다(246개, 튜토리얼 스크립트는 `extracted/tarc/d/tutorial1~6.english`).
  - 기준 상태 확인: Release 빌드 오류 0(기존 CA2014 경고 1), 테스트 Core 128·Assets 172 통과.
- **아직 하지 않은 것 → 후보 1번(튜토리얼 단계 처리)은 오후에 완료함**(위 절). 2~4번은 미착수. `extracted/decomp/Netstorm.c`는 준비됐으므로 바로 분석에 들어갈 수 있다. 디컴파일에 없는 함수(`0x484ab0`·`0x4c2b20`·`0x4c3290` 등)는 `tools/ghidra/decompile_at.ps1`이 필요한데 이것도 PowerShell 전용이라 이 PC에서는 `DecompileAt.java`를 `analyzeHeadless -process`로 직접 돌려야 한다(미시도).
- **다음 후보 1번(튜토리얼 단계 처리 객체)을 위해 읽어 둔 것**
  - 튜토리얼 1은 단계 D~F(가이저 연결·사제의 결정 운반·600 SP)가 수집 경제에 의존한다. 그래서 **튜토리얼 2만 지금 구현할 수 있다**(단계 A·B·D·E·G·H는 "지은 개수" 조건). 튜토리얼 1은 후보 3번(수집 경제)이 먼저다. 튜토리얼 3~6은 전투가 필요하다.
  - 단계 C(워크샵에서 지식 등록 — 세션의 `RegisterKnowledgeCommand`로 판정 가능)·F(템플을 선택해 범위를 보면 타이머 시작)는 UI 선택 상태가 필요하다. 세션에 "선택" 개념이 없으므로 결정론을 지키려면 선택을 명령(예: `SelectEntityCommand`)으로 만들지 화면 신호로 둘지 정해야 한다.
  - "지은 개수"(`FUN_004c24c0`, `DAT_005c98d0[타입]`)가 누적인지 현재 개수인지가 미확정이다. 파일 이름이 `Totalmade.cpp`라서 **누적(지은 총수)** 일 가능성이 높다. 이 배열을 올리는 곳을 디컴파일에서 찾아 확인할 것(회수해도 줄지 않는지가 단계 G 조건에 영향).
- **발견한 문제 → ✅ 2026-09-30 오후 수정함**(위 절): `BattleSession.CreatePlayer`가 `MissionStart.Tech`를 복사하지 않고 그대로 `PlayerState.Tech`로 쓴다. `TechPermissions`가 변경 가능해졌으므로(`Set`) 같은 `MissionStart`로 세션을 두 개 만들면 한쪽의 단계 처리가 다른 쪽 표까지 바꾼다. 현재 테스트는 세션마다 미션을 새로 읽어서 드러나지 않는다. 튜토리얼 단계 처리를 넣기 전에 `TechPermissions` 복사본(예: `Clone()`)을 쓰도록 고치고 회귀 테스트를 추가할 것.
- **참고**: 이 PC(`vm-debian-codex`)는 AGENTS.md 예외 시스템 1이라 게임 실행 확인은 필요 없지만, 이번에는 실행하지 않았다.

### 2026-09-30 (`DESKTOP-HJOW`, 원본 실행 없음): 재디컴파일 · 튜토리얼 2 헤더 플래그 원인 · 게임 세션

- **재디컴파일**(사용자 요청): 두 판본을 이 PC에서 다시 디컴파일했다 — 결과는 아래 3단계 절에 기록. 패치판은 기존 결과와 SHA-256 동일, CD판은 이 PC에 없어서 새로 생성.
- **튜토리얼 2 `denySalvage = 1`·`techAllowed` 모순 원인 규명·문서화**(사용자 요청) → **[docs/exe/mission-header-flags.md](docs/exe/mission-header-flags.md)**
  - 결론: 머리 값은 **시작 상태**이고 튜토리얼 단계 처리(`FUN_004c3bb0`)가 실행 중에 바꾼다. 단계 B에서 sunFactory 허용(`FUN_004c23e0(sunFactory, 1)`), 단계 H에서 회수 금지 해제(`DAT_00595078 = 0`). 사용자 관찰(템플 뒤 Sun Workshop 건설, 마지막에 유닛 회수 75 SP)과 일치한다.
  - 기술 허용 표는 **메뉴·덱에서만** 확인한다(Construct 메뉴 항목 `FUN_00461cf0`, 생산 목록 `FUN_004752b0`, 덱 갱신, 템플의 골렘). 회수 금지는 Salvage 메뉴 명령·실행(`FUN_0044ca00`·`FUN_0044c420`)이 확인하고 스크립트 섹션 `[DenySalvage]`를 Tell하는데, 원본 스크립트에 그 섹션이 하나도 없다.
  - 이전 세션의 "`techAllowed` = 만들 수 있는 기술" 해석을 "시작 상태 + 실행 중 변경"으로 정정했다(`TechPermissions` 변경 가능화).
  - **Ghidra 전체 디컴파일에서 빠진 함수를 발견**: 이 값을 읽는 `0x484ab0`·`0x4c2b20`·`0x4c3290`이 `extracted/decomp/Netstorm.c`에 없다. INT3 패딩 뒤 함수 프롤로그 휴리스틱으로 **약 528개 후보**(실제 누락 수는 미검증). 새 도구 `tools/ghidra/decompile_at.ps1`(`DecompileAt.java`, 읽기 전용 프로젝트에서 주소를 함수로 만들어 디컴파일)를 추가했다 → [재현 방법](docs/exe/mission-header-flags.md#7-ghidra-전체-디컴파일에서-빠진-함수-재현-방법-포함), [추출 순서](docs/formats/README.md#추출-순서-처음-받은-사람용). **설정 키 문자열이 디컴파일 검색에서 안 나오면 누락 함수를 의심할 것.**
  - 튜토리얼 단계 A~I의 조건 표도 문서화했다(단계 C·F는 UI 선택·타이머 상수 의존이라 미확정).
- **게임 세션 구현**(추천 1번 "규칙 코어를 게임 루프에 연결") — [core-rules.md "게임 세션"](docs/core-rules.md#게임-세션-battlesession), [map-viewer.md](docs/map-viewer.md#게임-세션과-미션-모드)
  - Core: `Simulation/BattleSession`(24Hz 고정 틱, 명령 큐, 플레이어 상태, 엔티티, 이벤트, `Checksum()`) + 명령 7종(배치·건설·등록·회수·다리 집기/되돌리기/놓기) + `BattleSessionFactory`(맵·미션에서 조립). `BattleMap.CheckBuilding`(사제 건물)·점유 카운트·`InitialObjects`, `Bridges/BridgeReach`(빈 섬 다리 연결 판정, 이전의 `C` 키 가정을 대체), `Footprint.BorderCells`, `BridgeGrid.Version`.
  - 흐름: 템플 건설 16초 → 완공 시 섬 소유·공급원·다리 조각/골렘 공급, 워크샵 10초 → 지식 등록 → 유닛 배치(재충전 Unit Rate) → 회수 25%. 튜토리얼 2 관찰 SP(10,000 → 5,000 → 4,200 → 300씩 → 회수 +75)가 테스트와 뷰어 실행에서 그대로 나온다.
  - 뷰어: 규칙 상태를 세션으로 옮기고 `--mission 이름`(시작 SP·지식·옵션·생산 규칙), `--script "…"`(검증용 명령), Space 정지·K 생산 규칙·F 등록·Delete 회수, HUD(SP·게임 시각), 건물 건설 시험(진행 막대) 추가.
  - **검증**: Core 테스트 **128개**(기존 114 + 신규 14, 6회 반복 안정), Assets 172개 통과, 빌드 오류·경고 0. HEAD의 옛 뷰어를 임시 worktree로 빌드해 같은 명령의 캡처를 **픽셀 비교**: 다리 시험 2장·배치 시험은 지도 영역 동일. 이 비교가 저장 다리가 사라지는 버그를 잡아 고쳤고 회귀 테스트를 추가했다. Dissolved Alliance!의 다리 칸 패널만 다르다(세션은 모든 플레이어의 다리 칸을 전역 난수 하나로 채움 — 원본과 같은 구조).
  - 테스트 인프라: `OriginalData.RequireResources()`가 호출마다 새 `GameResources`를 만든다(공유 `ConfigStore`의 임시 층 Push/Pop이 병렬 테스트에서 엉뚱한 맵을 읽던 경쟁 조건).
  - 스크린샷: `extracted/screens/session-*.png`.
- **근사·미확인** (원본 분석이 필요, 문서에 명시): 건설 시간(관찰값, 사제 이동 포함)·비용 차감 시점, 사제 이동·건설 자리 도달, 다른 빈 섬을 거치는 연쇄 연결, `.fort` `Deck`·`Technology` 초기값 연결, 튜토리얼 단계 처리 코드 자체(지금은 테스트·`--script`가 단계 B·H 효과를 손으로 재현).
- **다음 후보**:
  1. 튜토리얼 단계 처리 객체 — 세션 명령·이벤트를 보고 표·`DenySalvage`를 바꾸는 별도 클래스로(단계 A·B·D·E·G·H는 개수 조건, C·F는 UI 선택과 함께)
  2. 사제 이동·건설 절차 분석(`Priest.cpp` 등) → `ConstructionTimes`·비용 차감 시점 교체
  3. 수집 경제: 가이저 → 사제 결정 운반(결정당 200 SP) (`Carrier.cpp`·`Nugget.cpp`·`Vortex.cpp`)
  4. Ghidra 누락 함수 목록 만들기(후보 528개)와 게임 로직 쪽 우선 디컴파일
  5. 세션을 게임 화면으로: 생산 창(덱) 사이드바·HUD(9단계 UI)
  6. 다리: 붕괴 대기 조건·영역 소유 이어짐(기존 후보 1)
- 이번 변경 파일(커밋 전): `src/Netstorm.Core/Simulation/*`(신규 8), `Bridges/BridgeReach.cs`(신규), `Bridges/BridgeGrid.cs`·`Rules/{BattleMap,Footprint,MissionStart}.cs`, `src/Netstorm.Game/{FortMapViewer{,.Session(신규),.Placement,.Bridges},NetstormGame}.cs`, `tests/Netstorm.Core.Tests/{BattleSessionTests,SessionData}.cs`(신규)·`{MissionStartTests,OriginalData}.cs`, `tools/ghidra/{DecompileAt.java,decompile_at.ps1}`(신규), `docs/exe/mission-header-flags.md`(신규)·`docs/{core-rules,map-viewer}.md`·`docs/exe/battle-options.md`·`docs/formats/{README,mission-script}.md`, 이 문서.

### 게임 구현: 다리 배치 판정·붕괴, 미션 시작 조건 (2026-09-30, `HJOW-Athlon`, 원본 실행 없음)

튜토리얼 1·2 사용자 직접 조작 결과(아래 두 절)를 참고해, 정적 분석으로 확인할 수 있는 부분을 구현했다.

- **다리 배치 판정·붕괴** — `src/Netstorm.Core/Bridges/BridgeGrid.cs`, [bridge-pieces.md](docs/exe/bridge-pieces.md) 8절.
  - exe 확인: 10초 주기 갱신(`Bridge.cpp` `00422bc0`, `0x52f968`), 칸 수명 0~7(`brMAX_TIME_LEFT`)·5 아래 금 감(`0x52f960`)·0 이면 제거(`00421c30`), 단단한 칸 제외, 연결망 "0이 아닌 최소 수명 − 1" 동기화(`004227e0`), 겹치는 칸이 있으면 배치 불가(`Rifttype.cpp` `0049b510`).
  - 근사: 이어짐 = "조각의 연결 방향이 섬 칸 또는 내 다리의 마주 연결된 끝에 닿음"(원본은 영역 소유 판정 `Player.cpp` `0048fdb0`), 붕괴 대상 = "열린 끝이 있는 연결망".
  - **규칙 확정(사용자 확인, 2026-09-30): 섬 가장자리 중 초목이 있는 부분에서는 다리 건설을 시작할 수 없다.** [섬 소유권 규칙](docs/gameplay/island-ownership.md) 3번, [bridge-pieces.md](docs/exe/bridge-pieces.md) 4·8절에 반영.
  - **코드 반영 완료(2026-09-30):** exe 확인 결과 초목 = `edgeFarm` 오브젝트(typeflags `dropBlocking`, 플래그2 0x10). 원본은 이 비트를 스폿 지도에 남기고(`Squid.cpp` `004b02d0`) 배치 판정(`0049b510`)이 그 칸을 부착 후보에서 뺀다.
    - 클론 구현: `TypeFlagBits.DropBlocking`, `Bridges/BridgeAnchors`(edgeFarm 칸 + dropBlocking 오브젝트 발자국 → `BridgeGrid` 부착 판정), 뷰어 다리 모드 연결
    - 테스트 `BridgeAnchorTests` 3개(원본 Bridge the Gap! 가장자리 전수 검사 포함). Core 114·Assets 172 통과
    - 스크린샷: `extracted/screens/bridge-place-{ok,vegetation,red}.png`
    - 한계: 뷰어의 edgeFarm 칸 위치는 좌표 고정 근사라 원본과 다를 수 있다([bridge-pieces.md](docs/exe/bridge-pieces.md) 8.4절)
  - 원본 관찰과의 차이: 금 간 기간 약 40초는 같지만, 모델은 첫 갱신 뒤 약 30초 만에 금이 가고 원본은 약 120초 뒤였다. 붕괴 시작 전 대기 조건(`004218b0`)을 더 풀어야 한다(8.3절).
  - 맵 뷰어 다리 모드(B)에 연결: 놓을 수 없으면 빨간 조각·이유 표시, 판정 통과 시에만 놓기, 10초 주기 붕괴·금 간 프레임·무너진 저장 다리 숨김. 확인 스크린샷 `extracted/screens/bridge-place-{ok,red}.png`([map-viewer.md](docs/map-viewer.md#다리-조각-시험-모드)).
- **미션 시작 조건** — `src/Netstorm.Core/Rules/MissionStart.cs`.
  - 머리 값 myStartMoney(없으면 전투 옵션 시작 금액)·myTech(시작 지식 → `ProductionDeck`)·techAllowed·denySalvage·denyAscend·aiOff·tutorialNumber·loadFort·title 을 읽는다.
  - techAllowed 는 원본 `Mission.cpp` `00482eb0` 순서(deny/allow 모드 전환, all = 전체, 이름 = 개별)로 해석한다(`Totalmade.cpp` `004c23c0`~`004c2400`).
  - 원본 튜토리얼 1(0 SP, 지식 없음, windVortex 만 허용)·2(10,000 SP, sunArcher)의 값이 사용자 조작 관찰과 같음을 테스트로 확인했다.
  - `myStartMoney`·`myTech` 문자열은 exe 에 있으나 읽는 코드는 간접 참조라 찾지 못했다(의미는 문서·관찰 기준).
- 테스트: Core **111개** 통과(새 `BridgeGridTests` 8개, `MissionStartTests` 3개), 빌드 오류 0.
- **다음 후보:**
  1. 붕괴 대기 조건과 영역 소유 이어짐(`004218b0`·`0048fdb0`)
  2. ~~초목 가장자리 판별 방법 찾기~~ → 2026-09-30 완료(edgeFarm dropBlocking, 위 항목). 남은 것: edgeFarm 칸 위치를 원본 전역 난수·배치 가능 여부대로 재현
  3. 끝 칸 L·M·N·O(섬 쪽 연장 그림, `004215d0`)
  4. ~~엔티티·틱~~ → 2026-09-30 `BattleSession`으로 구현(건설 시간은 관찰값 근사, 위 절). 남은 것: 사제 결정 운반(결정당 200 SP)·이동
  5. ~~미션 시작 조건을 맵 뷰어·게임 화면에 연결~~ → 2026-09-30 완료(`--mission`, 위 절)
- 이번 변경 파일(커밋 전): `src/Netstorm.Core/Bridges/BridgeGrid.cs`(신규), `src/Netstorm.Core/Rules/MissionStart.cs`(신규), `tests/Netstorm.Core.Tests/{BridgeGridTests,MissionStartTests}.cs`(신규), `src/Netstorm.Game/FortMapViewer{,.Bridges}.cs`, `docs/exe/bridge-pieces.md`·`docs/core-rules.md`·`docs/map-viewer.md`, 이 문서.

### 튜토리얼 2 사용자 직접 조작 분석 완료 (2026-09-30)

- [x] `HJOW-Athlon` 세션 `20260929T160257202Z-2e9f1861506d`에서 `2 Secret Workshop`을 사용자 직접 조작으로 완료했다. `Mission Accomplished! … Tutorial Two` 결과 화면과 3,075 SP를 PNG·세션 메모에 남겼다. 다음 튜토리얼은 열지 않았고 녹화·안내 창·게임을 종료했다. [관찰 결과](docs/screens/README.md#17-사용자-직접-조작-녹화-튜토리얼-2-완료-2026-09-30-windows-hjow-athlon).
- [x] 영상 2,751프레임(AVI 5개), 소리 WAV 3개, 입력 JSONL 1개를 저장했다. 각 AVI/WAV는 48 MB 미만이고 AVI 전부 FFmpeg 디코딩 성공, 소리도 검출됐다. 영상·오디오 길이는 약 275.1초·275.3초다.
- [x] Wind Temple·Sun Workshop 건설, Sun Disc Thrower 생산 등록·네 개 배치, 에너지 범위 확인, 하나 Salvage를 관찰했다. SP는 10,000→5,000→4,200→3,000→3,075로 변화했고, 마지막 75는 Sun Disc Thrower 원가 300의 25%다. 배치 클릭부터 완공까지 Temple 약 16초, Workshop 약 10초(이동 포함), 첫 두 유닛 배치 뒤 생산 아이콘 복귀 약 1.1~1.2초. [재충전 시간](docs/exe/production-refresh.md#실제-화면-확인-2026-09-30-튜토리얼-2).

### 튜토리얼 1 사용자 직접 조작 분석 완료 (2026-09-30)

- [x] `HJOW-Athlon`에서 세션 `20260929T154230683Z-bdf8f92d93a1`로 `Bridge the Gap!`을 사용자 직접 조작 방식으로 끝냈다. 600 SP와 `Mission Accomplished!` 결과 화면을 PNG로 저장하고 세션 메모에 연결했다. 다음 튜토리얼은 열지 않았고 녹화·안내 창·게임을 모두 종료했다. [관찰 결과](docs/screens/README.md#16-사용자-직접-조작-녹화-튜토리얼-1-완료-2026-09-30-windows-hjow-athlon).
- [x] 실제 게임에서 10 FPS 영상 2,438프레임을 AVI 4개, 소리를 WAV 2개, 마우스·키 입력을 JSONL 1개에 저장했다. 모든 AVI/WAV는 48 MB 미만이고 FFmpeg로 읽혔다. 영상·오디오 길이는 각각 약 243.8초·243.9초다. 이전 RDP 화면 가림으로 0프레임에 그친 사례와 달리, RDP 화면이 보이는 상태에서 녹화가 정상 동작했다.
- [x] 가이저 연결 뒤 사제의 결정 반납으로 SP가 0→200→400→600이 되고 튜토리얼 1 완료 창이 뜨는 것을 확인했다. 사용자가 `Bridge Quality` 설명은 빠르게 넘겼다고 알렸지만, 이후 영상에서 연결되지 않은 오른쪽 다리 끝이 입력 없이 조각나 사라지는 장면은 확인했다. 개별 금 감 시작 시각과 일반적인 붕괴 시간은 이 영상만으로 확정하지 않는다.

### 사용자 직접 조작 녹화 기능과 PNG 한도 수정 (2026-09-29~30)

- 기존 CLI/MCP AI 조작을 유지하며, 실행 중인 세션에 `guide --session ID --steps-file UTF8파일`로 붙는 Windows 안내 창을 추가했다. 단계 표시·전후 이동·중단 후 재개, 게임 화면 10 FPS MJPEG AVI, 기본 출력 장치 루프백 WAV, 마우스/키보드 JSONL 기록을 지원한다. 영상·소리는 48 MB 전에 분할하고 각 조각의 시각 파일을 남긴다. 상세 사용법은 [docs/analyze-manager.md](docs/analyze-manager.md) "사용자 직접 조작 녹화 모드" 절.
- 기존 PNG의 세션당 500장/200 MB 제한은 제거했다. 개별 파일 50 MB 미만, 이벤트 10,000개 한도는 유지한다. 아래의 500장 도달 기록과 캡처 절약 지침은 **변경 전 당시 상황**이다.
- Release 빌드 통과(오류·경고 0), 단위 테스트 23개 중 22개 통과·1개 건너뜀(심볼릭 링크 생성 권한 없음), 게임 없는 MCP 기본 검사 9개 도구 통과. Windows 기본 출력 장치 2초 녹음은 약 0.7 MB WAV 생성 확인. AVI 조각·재개와 501장 PNG 저장은 단위 테스트로 확인했다.
- 짧은 실제 세션 `20260929T145451853Z-49cc0790f162`에서 안내 창은 열렸지만, RDP 화면에서 게임 창 전면 확보가 실패하고 화면 가림 검사에 걸려 영상이 0프레임에서 중단됐다. 이 과정에서 오디오 장치 위치의 절대값을 잘못 해석해 43 MB 무음을 만든 문제를 발견해 첫 패킷 기준으로 수정했으며, 위 2초 단독 녹음으로 크기를 재확인했다. 게임은 `end_session force=true`로 종료했고 안내 창도 닫았다. **실제 영상 프레임·사용자 입력·AVI 재생은 2026-09-30 튜토리얼 1 세션에서 확인했다(위 절).**
- **테스트 산출물 정리 완료(2026-09-30):** 사용자 요청에 따라 위 테스트 세션, 단독 오디오 검사 폴더, 테스트용 JSON·안내 파일을 삭제했다. 다른 분석 세션은 보존했다.

### 원본 동적 분석 시간 계획 (2026-09-29 22:45~, `HJOW-Athlon`) — 사용자 요청으로 23:23 중단

**결과 요약** — 자세한 측정은 [docs/screens/README.md](docs/screens/README.md) 1.5절. 세션은 `20260929T133717181Z-7f413cde9896`이며, 도구의 세션 이미지 한도(500장)에 걸려 튜토리얼 1 D 단계에서 종료(`end_session force=true`)했다. 새 세션은 시작하지 않았다.

- **완료한 항목:**
  - 로딩 창 Cancel: 반응 없음(두 번 시도, 데모 그대로 시작)
  - Campaign 창 `$Timeout=120`: 실측 120.1초에 닫힘 → 45.2초 뒤 Auto-Demo
  - Auto-Demo 안내 창: 뜬 뒤 14.7초에 저절로 닫힘(`$Timeout=15`). 이전에 "안 뜬다"고 기록한 것은 캡처 간격 때문으로 보인다.
  - Restart Demo: 로딩 창을 거쳐 처음부터 다시 시작, 안내 창 다시 뜸
  - Original Credits: 이 세션에서는 20초마다 넘어가는 7번의 전환만 측정했다. 마지막 쪽 뒤 흐름은 위 2026-09-30 Windows 세션에서 확인했다.
  - Create New Map: "New Save-Game Name?" 입력 창(Cancel 함)
  - Test Battle:
    - 창 제목 `NetStorm Test Battle "<미션 제목>"`, 브리핑 창
    - Game 메뉴 맨 위에 **Return to Editing** → 편집기로 돌아감
  - 튜토리얼 1:
    - 다리 칸 약 1초마다 채움
    - 칸 그림은 시간이 지나도 불변
    - 섬 윗 가장자리(초목)는 배치 불가, 오른쪽 물가 옆은 가능
    - 가능할 때 섬 쪽 짧은 연장이 보임
- **한쪽만 붙은 다리의 붕괴:**
  - ~~단일 칸~~ **두 칸 막대**(2026-09-30 정정): 놓은 뒤 +40초 바깥 칸 금 → +80초 낙하 → +120초 안쪽 칸 금 → +160초 낙하. 기록 당시엔 "단일 칸 약 120초 보통 → 금 간 그림(K11) 약 40초 → 사라짐(총 약 160초)"로 잘못 읽었다.
  - 두 칸짜리는 끝 칸이 먼저 사라짐
- **튜토리얼 진행:** B → 다리 몇 개 → C(q·w·a·s 칸 선택 안내) → C1 → 몇 개 더 → D. **가이저는 D 단계에서야 생김**(.fort 에 없음).
- **기타:** F2 = 큰 물체 투명. ALT 스크롤은 매우 빠름.
- **다음에 이어서 할 일:**
  - [x] 새 세션에서 튜토리얼 1을 처음부터 진행(저장 불가): 가이저 연결 → 사제 결정 왕복·결정당 SP → 600 SP 결과 화면 (2026-09-30 사용자 직접 조작 녹화로 완료, 위 절)
  - [x] 튜토리얼 2 건설 시간·배치 뒤 아이콘 복귀·완료 결과 (2026-09-30 사용자 직접 조작 녹화로 완료, 위 절)
  - [x] Leave → Replay Mission 흐름 — 2026-09-30 Windows 자동 분석에서 같은 미션 첫 브리핑 재진입 확인
  - [x] Credits 마지막 쪽 뒤 흐름 — 2026-09-30 Windows 자동 분석에서 메인 메뉴 복귀 확인
  - 캡처를 아끼도록 좁은 영역·긴 간격으로 측정한다(이미지 한도 500). 배치 가능 위치 탐색은 한 번에 캡처가 많이 쌓이므로 한 세션에서 오래 하지 않는다.
  - 배치 판정 분석([bridge-pieces.md](docs/exe/bridge-pieces.md) 8절)에 위 붕괴 시간을 기준값으로 쓴다.
- 이번 변경 파일(커밋 전): `docs/screens/README.md`(1.5절), `docs/exe/bridge-pieces.md`(4절 붕괴 관찰), `LEFT_JOBS.md`

**원래 계획 (참고)**

원본은 **진행 저장 기능이 없다**(Edit 모드 맵 저장만 가능, 사용자 확인). 미션 상태가 필요한 측정은 한 번의 진행 안에서 순서대로 한다.

| 시각(대략) | 단계 | 측정·확인 |
|---|---|---|
| 22:45~22:48 | 0. 시작, Demo → 로딩 창 Cancel | Cancel 결과 |
| 22:48~22:53 | 1. Campaign 창 방치 | `$Timeout=120`(120초) → Auto-Demo 45초 → 안내 창 여부 → Restart Demo → Exit Demo |
| 22:53~22:56 | 2. Original Credits | 20초 자동 넘김, 끝 흐름 |
| 22:56~23:00 | 3. Edit | Create New Map, Test Battle (저장 없이 나감) |
| 23:00~23:25 | 4. 튜토리얼 1 한 번 진행 | 다리 칸 채움 간격, 칸 조각 금 감→단단함 시간, 다리 배치(섬 가장자리·연결 표시), 연결 안 된 다리 붕괴 시간, 사제 결정 왕복·결정당 SP, 600 SP 결과 화면 |
| 23:25~23:45 | 5. 튜토리얼 2 (Unit Rate Fast) | 템플·워크샵 건설 시간, 생산 등록, 유닛 배치 뒤 아이콘 복귀(1초 예상), Leave → Replay Mission |
| 23:45~ | 6. 문서 반영 | |

### Windows 원본 동적 확인 (2026-09-29 저녁~밤, `HJOW-Athlon`) — ✅ 메인 메뉴 흐름 조사 완료

- **실행 상태:**
  - 지정 예외 시스템 2(Windows 10 Pro)라서 확인 없이 원본 복사본을 실행했다. 세션은 네 개다.
    - `…113420484Z-3358366e5f90`: 사용자 중단(작업 전 문서 확인 지시)
    - `…113907044Z-929836edfdde`: 사용자 중단(문서 정리 지시)
    - `…114913277Z-391e6a079ff0`: 원격 데스크톱 창이 최소화되어 포커스·캡처 실패
    - `…120526716Z-327849a23cac`: 본 조사
  - 마지막 세션은 `end_session force=true`로 종료했다. 실행 중인 `Netstorm.exe`는 없고 `originals/` 변경은 없다.
  - 증거는 `extracted/analyzeManager/<세션>/`에 있다(git 제외).
  - Technical Help를 누른 결과로 Windows 도움말 앱 `HelpPane.exe`와 Edge 창이 남아 있을 수 있다(사용자에게 알림).
- **원격 데스크톱 주의:** `HJOW-Athlon`은 RDP 세션으로 쓰고 있다. **RDP 창이 최소화되면 캡처·포커스가 실패한다** → 분석 중에는 RDP 창을 띄워 두어야 한다([docs/analyze-manager.md](docs/analyze-manager.md) "RDP" 절).
- **확인한 원본 동작** → [docs/screens/README.md](docs/screens/README.md) 1.2·1.3절, [docs/screens/main-menu.md](docs/screens/main-menu.md) "메뉴 흐름도":
  - **보유 exe = 10.78** (Help → Version 창 "Version v10.78 / 10.78 Patch by Ticonderoga Entertainment") → [docs/sources/README.md](docs/sources/README.md) 2절 갱신
  - Auto-Demo는 **마지막 입력(마우스 이동 포함)과 대화상자가 닫힌 시점 중 늦은 쪽부터 45초** 뒤에 시작한다. 대화상자가 열린 동안은 시작하지 않는다.
  - 로딩 창은 "Starting Mission..." 또는 "Connecting to Game Server - Countdown N"(Cancel)이다. Auto-Demo 안내 창은 뜰 때와 안 뜰 때가 있다(조건 미확인). 선택 데모의 안내 창은 `$Timeout=15`대로 15초 뒤 자동으로 닫힌다.
  - ESC는 데모·미션 중 상단 메뉴 막대를 켜고 끈다(브리핑 창이 열려 있으면 무반응). 메뉴 막대 구성:
    - 데모 Game: Restart/Exit Demo, Quit Game
    - 미션 Game: Review Objectives F8, Restart/Leave Mission, Quit Game
    - 미션 View(F2~F9)·Options·Players·About, 편집기 Game·Edit·View·Options·About
  - Exit Demo는 확인 없이 메인 메뉴로 간다. Leave Mission은 확인 창(Main Menu / Replay / Continue)을 거쳐 메인 메뉴로 간다. 편집기에서 나갈 때는 저장 확인(Yes / No / Cancel)이 뜬다.
  - **브리핑 창이 떠 있는 동안에도 게임이 진행된다**(다리 조각이 참).
  - 도움말 드래그는 1:1이다. 38px는 스크롤 끝이다.
  - Credits 각 쪽은 20초마다 자동으로 넘어간다. Campaign 버튼은 `[UCampaign]`을 연다. Edit에서 맵을 고르면 편집기 모드로 들어간다.
  - 메인 메뉴 버튼은 커서를 먼저 올린 뒤 클릭해야 반응한다. `>` 하위 메뉴는 클릭해야 열린다.
- **남은 원본 확인 (다음 작업 후보, 지정 시스템에서 확인 없이 실행 가능):**
  1. Auto-Demo 안내 창이 뜨는 조건과 45초 상수의 exe 위치(정적 분석)
  2. Campaign 나머지 묶음 확인, Replay/Restart Mission 뒤 배치물 초기화 확인 (두 명령의 첫 브리핑 재진입·Restart Demo·로딩 창 Cancel·Original Credits 마지막 전환·Campaign `$Timeout=120`은 후속 세션에서 확인 완료)
  3. 편집기 Create New Map·Test Battle·Add Island·Set All Bridge
  4. 미션 플레이 관찰: 사이드바 배치 조작, Unit Rate 간격 측정([production-refresh.md](docs/exe/production-refresh.md) 남은 일), 결과 화면 흐름
  5. 메인 메뉴 Options의 Pause 줄 표시 조건(`DAT_00594fa4`·`DAT_005c85a4`)
- 도구 쪽 미검증: 파이프 EOF(CLI 출력을 파일로 받아 확인하지 못함). `analyzeManager/ExplorerTools.cs`·`Program.cs` 도움말의 시스템 2 미반영은 아래 절 그대로다.
- 이번 변경 파일(커밋 전): `docs/screens/README.md`, `docs/screens/main-menu.md`, `docs/analyze-manager.md`, `docs/sources/{README,game-manual,patch-history}.md`, `LEFT_JOBS.md`

### AGENTS.md 게임 구동 허용 조건 추가 반영 (2026-09-30)

- AGENTS.md에 예외가 하나 더 생겼다: **개발자(사용자)가 기존 게임 수동 컨트롤 방식으로 분석 진행을 직접 요청한 경우, 해당 작업 단계에서는 시스템과 무관하게 실제 게임 구동 확인을 받지 않아도 된다.** (기존 예외: 시스템 1·2.) 요청이 없거나 다음 작업 단계로 넘어가면 일반 규칙(목적·필요성 설명 후 명시적 확인)으로 돌아간다.
- 반영: [docs/analyze-manager.md](docs/analyze-manager.md) "실제 게임 실행 전 개발자 확인", [analyzeManager/README.md](analyzeManager/README.md), `analyzeManager/{ExplorerTools,Program}.cs`의 `start_session` 설명·CLI 도움말 문구. 문자열만 바꿨으므로 재빌드는 하지 않았다(다음 빌드에서 확인 필요).

### AGENTS.md 규칙 갱신 반영 (2026-09-29)

- **`AGENTS.md`는 절대 수정하지 않는다.** 수정이 필요하면 개발자(사용자)에게 요청한다. (이전 인수인계에 "`AGENTS.md` 갱신"으로 남은 기록은 이 규칙이 생기기 전의 작업이다.)
- **게임 구동 확인 예외 시스템이 2개로 늘었다.** 아래 시스템에서는 `analyzeManager` 등으로 실제 게임을 구동할 때 개발자 확인을 받지 않아도 된다. 식별값이 확인되지 않거나 다르면 일반 시스템 규칙(실행 전 목적·필요성 설명 후 명시적 확인)을 따른다.

  | 구분 | IP | 호스트명 | 환경 |
  |---|---|---|---|
  | 시스템 1 | `10.0.0.15` | `vm-debian-codex` | Debian 13 + Wine 10.0 |
  | 시스템 2 | `192.168.0.94` | `HJOW-Athlon` | Windows 10 Pro (이 저장소 작업 PC, 2026-09-29 호스트명·IP 확인) |

- 시스템 2가 Windows이므로 아래 "다음 작업"의 **Windows 실제 GUI 재확인**(캡처, 파이프 상속 방지 효과, CLI 실행 뒤 게임 유지 등)을 별도 확인 없이 이 PC에서 진행할 수 있다.
- 반영 문서: [docs/analyze-manager.md](docs/analyze-manager.md) "실제 게임 실행 전 개발자 확인", [analyzeManager/README.md](analyzeManager/README.md). ~~**미반영:** `analyzeManager/ExplorerTools.cs`의 `start_session` 도구 설명과 `analyzeManager/Program.cs` CLI 도움말은 아직 시스템 1만 적고 있다(코드 수정·재빌드 필요).~~ → **2026-09-30 해소**: `start_session` 설명에 시스템 2 반영, `Program.cs` 도움말은 "AGENTS.md에 지정된 시스템"으로 이미 일반화되어 있었다.

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
  - ~~팁 창이 본문 클릭·ESC에 반응하지 않는지~~ → **사용자 확인(2026-09-29): 안내 창(Demo 안내 창 포함)에서는 ESC가 동작하지 않는다.** 남은 것: 도움말 드래그·스크롤 화살표의 스크롤 양.
- **다음 작업 (사용자가 실제 게임 테스트 재개와 실행을 확인한 뒤):**
  1. 위 재확인 대상을 사용자 입력이 없는 상태에서 다시 확인한다.
  2. 메뉴 흐름 조사: Campaign·Demo·Help·Edit·Credits·Options 하위 화면의 좌표와 전환 기록 → `docs/screens/main-menu.md`, 5단계 "메뉴 흐름도", 9단계 UI 기준. Multiplayer(네트워크)·전체화면 항목은 누르지 않는다.
  3. `analyzeManager` 개선 후보(게임 실행 없이 가능): 복사본 설정에서 `autoDemo`·시작 팁을 끄는 선택 옵션, 실행 중 사용자 입력과 섞이지 않도록 안내 문구 추가.
- 이번 변경 파일(커밋 전): `analyzeManager/WindowsGame.cs`, `docs/analyze-manager.md`, `LEFT_JOBS.md`

### 이전 인수인계 (2026-09-29 오전)

**실행 확인 규칙(사용자 요청):** 일반 시스템에서 이 자동 탐험 프로그램으로 실제 게임을 구동하려면, 실행 전에 개발자에게 목적과 필요성을 알리고 명시적인 확인을 받아야 한다. CLI/MCP `start_session`, `mcp_smoke.py --live`, 검증용 직접 실행에 모두 적용한다. 일반 작업 지시나 과거 실행 이력을 새로운 실행의 확인으로 간주하지 않으며, 확인받은 범위 안에서만 실행한다. **예외:** `AGENTS.md`에 지정된 시스템(시스템 1: IP `10.0.0.15`·`vm-debian-codex`, 시스템 2: IP `192.168.0.94`·`HJOW-Athlon` — 2026-09-29 추가)에서는 개발자의 게임 구동 확인을 받지 않아도 된다. 사용자가 이번 지시에서 지정 시스템의 확인 없는 게임 실행을 명시적으로 허용했다.

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

1. 실제 GUI 테스트는 지정 시스템에서 별도 실행 확인 없이 진행할 수 있다. 그 밖의 시스템에서는 기존 중단 지시를 유지하고, 재개 지시를 받은 뒤 매 실행 전에 목적과 필요성을 설명해 명시적인 확인을 받는다. `analyzeManager`는 Windows 전용이므로 Linux 지정 시스템(시스템 1)에서는 Wine 실행 환경 준비가 별도로 필요하고, Windows 지정 시스템(시스템 2 `HJOW-Athlon`)에서는 바로 실행할 수 있다.
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
| 7 | 엔진 코어 (플랫폼 계층) | 🔶 착수 (2026-09-29: 창·전체화면·16:9/16:10/4:3 화면 계층, 가장자리 스크롤, 표시 설정 저장, 고정 틱 누적기·MSVC 난수 완료 / 팔레트 방식·입력·오디오·로깅 남음) |
| 8 | 게임 월드 / 규칙 구현 | 🔶 착수 (2026-09-29: 정적 규칙 코어 — 에너지·섬 소유권·생산 창·전투 옵션·배치 판정, 맵 뷰어 배치 시험 모드, 다리 조각 모양·추첨·회전·생산 칸 채우기와 뷰어 다리 조각 시험 모드 ; 2026-09-30: 게임 세션(고정 틱·명령·엔티티·건설·회수·재충전)·다리 배치 판정·붕괴·미션 시작 조건 연결 / 이동·전투·수집 경제·붕괴 대기 조건 남음) — [docs/core-rules.md](docs/core-rules.md) |
| 9 | UI · 미션 스크립트 · 튜토리얼 | ⬜ 대기 |
| 10 | AI | ⬜ 대기 |
| 11 | 캠페인 · 저장(fort) | ⬜ 대기 |
| 12 | 다국어 지원 | ⬜ 대기 (설계는 초기부터 반영) |
| 13 | 멀티플레이 | ⏸ 후순위 (2026-09-30 사용자 결정: 나중에 구현) |
| 14 | 패키징 · 배포 (Windows 우선, Linux 후순위) | ⬜ 대기 |
| 15 | 검증 · QA | ⬜ 상시 |

단계 번호는 대략적인 순서이며, 3~5 단계(분석)는 병행 가능하다. 6 단계 이후는 분석 결과가 나오는 대로 점진적으로 진행한다.

---

## 1. 원본 조사 결과 (1단계 조사 + 3단계 분석으로 갱신)

`originals/` 현재 파일 1,389개, 파일 크기 합계 164,486,189바이트(2026-09-29 사용자가 추가한 `help/manual.pdf` 포함).

`originalCD/`는 사용자가 추가한 더 이른 CD판의 내용이다. 사용자 제공 판본 정보는 **10.72**이고, 현재 실행·분석 기준인 `originals/Netstorm.exe`는 **10.78**이다. CD에는 설치 파일과 `MOVIE/englishintro2x.smk`·`germanintro2x.smk` 등 인트로 자료가 있다. 사용자 설명에 따르면 CD판은 Windows 98/ME에서 호환되고 XP에서는 동작하지 않았으며 Windows 10/11에서도 실행되지 않을 것으로 예상된다. 따라서 CD판은 자료·판본 비교용으로 두고 Windows 10/11 동적 분석은 `originals/` 패치판을 기준으로 한다([자료 설명](docs/sources/README.md)).

### 1.1 실행 파일 / 라이브러리

| 파일 | 내용 |
|---|---|
| `Netstorm.exe` (1.5MB) | x86 32bit PE, MSVC 빌드, **패킹되지 않음**(.text 약 1MB). 빌드 타임스탬프 2006-03 → 원본(1997) 이 아닌 **Ticonderoga Entertainment 비공식 패치 빌드(10.7x)**. 임포트: DDRAW, DSOUND, WINMM, WSOCK32, smackw32, GDI32, COMCTL32, ADVAPI32, SHELL32, VERSION, ole32, mscoree. 문자열에 `TAFF v%d.%d`, `_shapes.shp`, `*.tarc`, `IPX`, `SPX/IPX` 존재 |
| `NSENGLISHRES.DLL` | **설치·진단 프로그램용** 문자열 24개·다이얼로그 13개 (게임 UI 문자열 아님). 게임 UI 문자열은 exe 에 영어로 하드코딩되어 있고 `xlat.<언어>` 로 번역된다 → [xlat.md](docs/formats/xlat.md) |
| `Smackw32.dll` | Smacker 동영상 코덱. `originals/movie/`는 비어 있지만 별도 CD판의 `originalCD/MOVIE/`에는 영문·독문 인트로 `.smk` 파일이 있음 |
| `R.exe`, `unpack.exe`, `bzip2.exe`, `TMaker.exe` | `R.exe` = **NETSTORM Root Server**(로컬 LAN 서버, 2026-09-30 exe 문자열로 정정 — [network-ports.md](docs/exe/network-ports.md)), `unpack.exe`·`bzip2.exe` = 업데이트 압축 해제로 추정, `TMaker.exe` = 요새 생성기로 추정. 게임 본체 분석 대상은 아님 |
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

* `originals/Netstorm.exe`(10.78)는 현재 **Windows 10/11 에서 제한적으로 구동 가능**하며, 실행 시 **창 모드**로 동작한다. 이 설명은 `originalCD/NETSTORM.EXE`(10.72)에 적용되지 않는다.
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
├─ originals/                 # 현재 분석 기준 패치판 10.78 (저장소 포함)
├─ originalCD/                # 이전 CD판 10.72 및 인트로·설치 자료 (자료 참고용)
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
  - **2026-09-30 추가: "방화벽 예외 (TCP 6799)" 항목**(권장, 기본 선택 해제) — 원본 게임이 전투 시작 때 여는 6799 인바운드를 허용하는 규칙을 점검·등록한다. **관리자 권한이 필요하며 관리자 권한이 아니면 "사용 불가"로 표시**하고 건너뛴다. 사용자가 Windows에서 포트 허용 기능과 비관리자 선택 차단이 동작함을 확인했다. 규칙 세부값·반복 등록·되돌리기·실제 경고 억제 여부 등 남은 검증은 [network-ports.md 7.2](docs/exe/network-ports.md)에 기록했다.
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
- [x] 이전 CD판 `originalCD/NETSTORM.EXE` 별도 디컴파일 — 2026-09-30 완료
  - `tools/ghidra/run_decomp.ps1 -Edition originalCD` → 프로젝트 `extracted/originalCD/ghidra/`, 결과 `extracted/originalCD/decomp/NETSTORM.c` (함수 3,711개 성공, 실패 0개). 기존 `originals/` 디컴파일 결과의 SHA-256은 작업 전후 동일하다.
  - 두 판본의 Ghidra 프로젝트와 C 결과는 `extracted/` 아래라 Git에 커밋되지 않는다. 다른 PC에서는 [재생성 명령](docs/formats/README.md#추출-순서-처음-받은-사람용)을 실행해야 한다.
  - **2026-09-30 `DESKTOP-HJOW` 재디컴파일 완료** (Ghidra 12.1.4, JDK 21, 두 판본 순차 실행 — 패치판 약 6분, CD판 약 3분):
    - 패치판 `extracted/decomp/Netstorm.c`: 함수 4,506개 성공·실패 0개. 이 PC에 있던 2026-09-28 결과와 **SHA-256이 같다**(`67e3e9eb…`, 183,914줄) — 내용은 이미 최신이었고 다시 만들어도 바뀌지 않는다. 그래서 `docs/exe/`의 `Netstorm.c` 줄 번호 인용(예: energy-requirements.md 약 104288행)은 그대로 유효하다.
    - CD판 `extracted/originalCD/decomp/NETSTORM.c`: 이 PC에는 없던 결과를 새로 만들었다. 함수 3,711개 성공·실패 0개, 155,935줄, SHA-256 `f6fecc58…`. 함수 수가 `HJOW-Athlon` 기록(3,711개)과 같다.
    - 입력 exe: `originals/Netstorm.exe` `a305414c…`, `originalCD/NETSTORM.EXE` `613500a3…`. Ghidra 로그의 `Invalid GIF data` 오류 4건은 exe 안 리소스 자동 해석 경고이며 디컴파일과 무관하다.
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
    - 주요 사실: 보유 exe 는 **10.78**(2026-09-29 게임 내 Version 창으로 확정. 이전 추정: 옵션 표 근거 10.75 이상). 섬 테마·거주지 테마는 패치 10.70 V5.3~6.0 기능. 1152×864·1280×960 해상도는 패치에서 제거. 멀티는 BattleMaster 가 서버·서버 인계·포트 6800. `bridgeDrawRate`·`stuffRefreshRate` 설정 키는 패치판 exe 에 없음(확인)
    - **확인 필요**: ~~level 1 원소 유닛의 에너지~~(→ 2026-09-29 확정: 유닛마다 다르며 Bulf = Thunder 1 — 사용자 확인·PDF 대조), 파일 조회 순서(패치 문서: tarc 우선 ↔ 정적 분석: 디스크 우선), Storm Power 노랑 기준(매뉴얼은 1000 미만 빨강만), 튜토리얼 3 의 범위
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
  - [x] 2026-09-29 조각 생성: 모양 표 26개(VA 0x52f998, 가중치 합 287)·누적 가중치 추첨·회전 표(0x531590)·프레임 선택, 생산 칸 채우기(템플이 있을 때 1초마다, Bridge Slots 칸까지, 5번째 추첨마다 한 칸 조각, 템플을 잃으면 비움), AI 는 4번째마다 한 칸 조각 → [bridge-pieces.md](docs/exe/bridge-pieces.md). `Deck.cpp` 는 다리와 무관(지식·생산 덱, Gem.cpp 가중 추첨)
  - [x] 2026-09-29 원본 실행(Windows `HJOW-Athlon`, Bridge the Gap!)으로 확인. 세부는 [bridge-pieces.md](docs/exe/bridge-pieces.md) 4절·[화면 목록 1.4절](docs/screens/README.md). 클론 `RotateByPlayer`·`BridgeCursor` 에 반영, 테스트 2개 추가.
    - 오른쪽 클릭 = 시계 방향 회전
    - C = 반대 회전 켜기/끄기(켜면 반시계)
    - 조각 왼쪽 위 칸 = 커서로만 결정: (⌊(x+7)/16⌋, ⌊y/11⌋), 크기·회전 무관
    - 붙일 수 없으면 전체 빨강
    - 사이드바는 조각을 절반 크기로 그림
  - [ ] 남은 일: 배치 판정·연결·소유권 전파·붕괴(`Bridge.cpp` 00421240~004227e0 — 튜토리얼 규칙: 섬 가장자리 또는 다른 다리의 열린 끝에 붙임, 아래에 막는 것이 있으면 불가, 초목이 늘어진 가장자리는 불가), 금 간·단단해지는 시점, 미션 `aiNBridgeDrawRate` 의 쓰임
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
    - **충족 판정 (사용자 확인)**: 필요 에너지 1개 = 서로 다른 공급원(Generator·Temple, 자기 또는 동맹 소유) 1개, 그 범위가 짓는 위치에서 모두 겹쳐야 함. Sun 은 아무 원소 공급원으로. 예: Vander Tower = Thunder 2 + Sun 1 → Thunder 공급원 2 + 아무 공급원 1. 구성은 **유닛마다 다르다**(2026-09-29 사용자 확인으로 정정 — 이전 일반식 `자기 원소 × (level − 1) + Sun × 1`은 폐기): 원소 유닛 L1 = 자기 원소 1(예: **Bulf = Thunder 1**, Thunder Generator 또는 Thunder Temple 필요), L2 = 자기 원소 1 + Sun 1, L3 = 자기 원소 2 + Sun 1, Sun 유닛 = Sun × 레벨, Generator·Outpost = 아무 1. 레벨은 패치판 `.type` 기준. 문서 4절에 전 유닛 필요 공급원 표, [PDF 유닛 핸드북 대조](docs/sources/pdf-manual.md#유닛별-건설-에너지-energy-to-build)
    - **소모 개념 아님**(같은 공급원 범위에 여러 유닛 가능), 조건은 **건설·생산 순간에만** 확인(공급원이 파괴돼도 유닛 유지), **이동 유닛도 생산 지점에 에너지 필요** — 질문 A~F 모두 확정 (문서 5절)
  - 남은 일: exe 의 등록·추첨(`Deck.cpp`)·도움말의 워크샵 생산 칸(Level I 2개·II 3개·III 4개) 검증·업그레이드 비용/효과, 에너지 공급 범위·판정 함수, 등록 목록과 `myTech`·`techBit` 의 관계
- [ ] **전투**: 사거리·명중·피해 공식, 발사체 궤적, 특수 효과(`bomb*` 계열: 마비, 중력, 치유, 반역 등), 방어(차단벽·실드)
- [ ] **승패 조건**: 프리스트(priest) 사망/포획, 신전(temple) 파괴, 미션 스크립트 이벤트 발생 지점
- [ ] 미션 스크립트 인터프리터 동작(3단계 문법 명세와 교차 검증)
- [ ] AI 의사결정 루틴 (10단계 입력)
- [ ] 네트워크: 프로토콜 방식(TCP/IP, IPX), 동기화 모델(락스텝 여부), 패킷 형식 (13단계 입력)
- [ ] `PatchFixs.txt` 와 대조하여 **기준 버전** 결정 (원본 1.x 동작 vs 패치 10.7x 동작)
  - 2026-09-29 정리: [patch-history.md](docs/sources/patch-history.md) (주제별 변경), 원판 매뉴얼 수치 ↔ 현재 `.type` 대조표 [game-manual.md](docs/sources/game-manual.md) 7절. 보유 exe = **10.78**(게임 내 Version 창). 사용자 캡처·설명(섬 테마, 거주지 원소 그림, Edit 메뉴 등)이 모두 패치판 기능이므로 **패치판 동작 기준**이 자연스러움 (결정은 5절 2번)

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
  - [x] 2026-09-29 원본 실행으로 메인 메뉴 전 항목(멀티 제외)·데모·미션 진입/이탈·편집기 진입/이탈 흐름 확인 → [main-menu.md](docs/screens/main-menu.md) "메뉴 흐름도". 남은 일: 결과 화면 이후, Replay/Restart, 멀티플레이 화면
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
  - [x] 2026-09-29 기반 구현: `Netstorm.Core.Simulation.FixedTimestep`(기본 24Hz, 따라잡기 8틱 한도, 초 → 틱 올림)·`MsvcRandom`(MSVC `rand()`, srand(1) 수열 41·18467·6334… 검사). 남은 일: 게임 화면에서 시뮬레이션 갱신을 이 누적기로 돌리기(엔티티 시스템과 함께)
- [ ] 로깅, 설정 저장(사용자 데이터 경로: Windows `%APPDATA%`, Linux `$XDG_CONFIG_HOME`/`$XDG_DATA_HOME`)
  - [x] 표시 설정 저장 — 2026-09-29: `DisplaySettings`(`%APPDATA%\NetstormReborn\settings.json`, `NETSTORM_SETTINGS_DIR` 로 폴더 변경, 임시 파일 교체 저장, 손상 시 기본값)
  - [ ] 남은 일: 소리·언어·튜토리얼 팁 등 나머지 옵션 저장(원본 `options.cfg` 항목 대응), 로깅

### 8단계. 게임 월드 / 규칙 구현

4단계 명세를 기반으로 구현하며, 모든 규칙 코드는 단위 테스트를 둔다.

- [x] **정적 규칙 코어** — 2026-09-29 완료: `src/Netstorm.Core/Rules/` ([게임 규칙 코어](docs/core-rules.md)). 원소·유닛별 필요 에너지(mana 우선, 원소 L1/L2/L3·Sun·Generator 규칙), 공급원 판정(자기·동맹, 중심 거리² ≤ 반지름², 공급원 하나 = 에너지 1개), 발자국(기준점 = 오른쪽 아래 칸, exe `FUN_0049ae80` 확인), 전투 옵션·공급 반지름·튜토리얼 2 덮어쓰기, 오브젝트 분류, Storm Power 색·회수·보상, 재충전 간격, 생산 창(템플 → 다리·골렘, 워크샵 등록 규칙·칸 수·파괴 해제), 섬 소유권·위치 조건, `BattleMap`(맵 → 소유권·공급원·점유, 배치 판정·실행). Core 테스트 88개(원본 `.type` 전체 요구값·The War Begins! 맵·Generator 전력선 포함) 통과
  - [x] 맵 뷰어 **배치 시험 모드**(P) — 규칙 코어로 판정·배치·범위 표시, `--placement`·`--probe` 검증 옵션 ([실행](docs/map-viewer.md#배치-시험-모드)). Dissolved Alliance! 에서 Sail Skater 가능·Bulf 에너지 부족 확인
  - [ ] 남은 일: 공급원 판정의 대상 크기 항(exe 가상 함수 +0xA0), 다리 연결·다리 끝 판정(현재 근사), 미션 `myTech`·`.fort` Technology/Deck → 초기 지식·덱, 동맹 설정, 발자국 전체의 섬 판정
- [ ] 맵·섬·아이소메트릭 렌더링, 카메라 스크롤, 오브젝트 그리기 순서
- [ ] 엔티티 시스템 (`.type` 데이터 구동)
  - [x] 2026-09-30 골격: `GameEntity`·`BattleSession`(명령·고정 틱·이벤트·검사합). 남은 것: 이동·체력·전투 상태
- [ ] 애니메이션 시스템
- [ ] 다리 조각 생성·배치·연결·붕괴
  - [x] 2026-09-30 배치 판정(겹침·이어짐 근사)·10초 주기 붕괴(수명 7→0, 금 감, 단단한 칸 제외): `Bridges/BridgeGrid`, 뷰어 다리 모드 연결 — [bridge-pieces.md](docs/exe/bridge-pieces.md) 8절
  - [x] 2026-09-29 생성·회전: `Netstorm.Core/Bridges`(BridgeLinks·BridgePatternCatalog·BridgePiece·BridgeFrames·BridgeTray)·`Simulation/NetstormRandom`, 테스트 `BridgePieceTests` 10개(원본 exe 표와 직접 대조 포함), 맵 뷰어 다리 조각 시험 모드 B(`FortMapViewer.Bridges.cs`, `--bridges`) — [core-rules.md](docs/core-rules.md), [map-viewer.md](docs/map-viewer.md#다리-조각-시험-모드)
  - [ ] 남은 일: 배치 판정·연결·붕괴 (위 4단계 분석 후), `BattleMap` 의 다리 끝·빈 섬 연결 근사 교체
- [ ] 건물 배치·건설
  - [x] 2026-09-30 사제 건물 건설(건설 시간 뒤 완성: 템플 → 섬 소유·공급원·다리/골렘 공급, 워크샵 → 등록 가능)·회수·배치 뒤 재충전 — 건설 시간은 관찰값 근사, 사제 이동·도달은 미구현
  - [x] 유닛 배치 판정(위치·빈 자리·Storm Power·에너지)과 배치 실행 — 2026-09-29 `BattleMap` (건설 시간·Power Stream 연출·사제 건설 절차는 남음)
- [ ] 경제(가이저, 수집, 운반, Storm Power)
  - [x] Storm Power 표시 색·회수 25%·파괴 보상·결정 200 규칙 — 2026-09-29 `StormPower` (수집·운반 흐름은 남음)
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
  - **시작 흐름에서 "Not Validated" 안내 창(클라이언트 유효성 검사)은 구현하지 않는다** (사용자 결정 2026-09-30) → [main-menu.md](docs/screens/main-menu.md)
  - Multiplayer 버튼은 멀티플레이(13단계)가 후순위라 구현 전까지 생략·비활성 중 어느 쪽으로 보일지 이 단계에서 정한다
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

> **후순위 (사용자 결정 2026-09-30):** 나중에 구현한다. 그때까지 클론은 네트워크를 쓰지 않는다. 원본의 포트·서버 구조는 [network-ports.md](docs/exe/network-ports.md).

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
4. **멀티플레이 범위**: LAN 만 / 인터넷 로비 포함 / 원본 호환 — **멀티플레이 자체가 후순위(2026-09-30 사용자 결정)이므로 구현 시점에 정한다**
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

**구현 진행 (2026-09-29, 문서화된 자료로 게임 구현 — 원본 실행 없음)**: 8단계 정적 규칙 코어와 맵 뷰어 배치 시험 모드, 7단계 고정 틱·난수를 구현했다 ([게임 규칙 코어](docs/core-rules.md)). 원본 없이 진행할 수 있는 다음 구현 순서는 다음과 같다.
1. **다리** — `Bridge.cpp`·`Deck.cpp` 정적 분석 → 조각 추첨(`Deck` chance 가중)·회전·연결·금 간 상태·붕괴 → 다리 끝·빈 섬 연결 판정으로 `BattleMap`의 근사를 교체
   - **2026-09-29 진행**: 조각 생성·추첨·회전·프레임·생산 칸 채우기 완료([bridge-pieces.md](docs/exe/bridge-pieces.md)). 추첨은 `Deck` 이 아니라 `Canondecoder.cpp` 의 고정 모양 표였다.
   - **다음**: `Bridge.cpp` `FUN_00421770`(칸 방향 연결 검사)·`004217f0`·`004215d0`부터 배치 판정을 옮긴다. 그다음 `004218b0`(재귀 연결·번호)·`00421240`(소유자 전파)·`00421c30`·`00421f90`·`004227e0`(붕괴 추정)을 분석한다.
   - **2026-09-29 원본 확인 완료**: 회전 조작(오른쪽 클릭 시계·C 반대 회전)과 커서 → 왼쪽 위 칸 규칙을 확인하고 반영했다(`BridgePiece.RotateByPlayer`, `BridgeCursor`, 뷰어 R·C 키·절반 크기 칸). 세션 `20260929T130339905Z-b92c507f492a`, 게임 종료·`originals/` 변경 없음 확인.
     - 이 작업의 변경 파일(커밋 전): `src/Netstorm.Core/Bridges/{BridgePiece,BridgeCursor(신규)}.cs`, `src/Netstorm.Game/FortMapViewer.Bridges.cs`, `tests/Netstorm.Core.Tests/BridgePieceTests.cs`, `docs/exe/bridge-pieces.md`·`docs/core-rules.md`·`docs/map-viewer.md`·`docs/screens/README.md`, 이 문서.
     - 테스트: Core 100·Assets 172 통과
   - (이전) 이번 변경 파일(커밋 전): `src/Netstorm.Core/Bridges/*`·`Simulation/NetstormRandom.cs`(신규), `tests/Netstorm.Core.Tests/BridgePieceTests.cs`(신규), `src/Netstorm.Game/FortMapViewer.Bridges.cs`(신규)·`FortMapViewer.cs`·`FortMapViewer.Placement.cs`·`NetstormGame.cs`, `docs/exe/bridge-pieces.md`(신규)·`docs/core-rules.md`·`docs/map-viewer.md`, 이 문서. 빌드 오류 0(기존 CA2014 경고 1), 테스트 Core 98·Assets 172 통과.
2. **엔티티·틱** — `.type` 구동 오브젝트 목록을 `FixedTimestep`으로 갱신하고, 건설 시간(`constructionRate`)·재충전 간격(`ProductionTimers`)을 틱으로 돌린다 → **2026-09-30 완료**(`BattleSession`, 건설 시간은 관찰값 근사)
3. **초기 상태** — 미션 머리 값 `myTech`·`myStartMoney`, `.fort` `Technology`·`Deck`에서 지식·덱·Storm Power를 채운다. 튜토리얼 2 는 `BattleOptions.ApplyTutorialTwoOverrides` → **2026-09-30 부분 완료**: 머리 값·전투 옵션 적용. `.fort` `Technology`·`Deck` 연결은 남음
4. **수집·경제** — `Carrier.cpp`·`Nugget.cpp`·`Vortex.cpp` 분석 → 가이저 → 결정 → 템플 운반 흐름
5. 이후 전투(`Gunprocess`·`Damageable`·`Bomb`), 사제·희생(`Priest`·`Dais`), UI(생산 창·컨텍스트 메뉴)

이번 변경 파일(커밋 전): `src/Netstorm.Core/Rules/*`·`Simulation/*`(신규), `tests/Netstorm.Core.Tests/{EnergyRuleTests,GameRuleTests,OriginalData}.cs`(신규), `src/Netstorm.Game/FortMapViewer.Placement.cs`(신규)·`FortMapViewer.cs`·`NetstormGame.cs`, `docs/core-rules.md`(신규)·`docs/map-viewer.md`·`docs/exe/energy-requirements.md`, 이 문서.

0. **동봉 문서 참고 (2026-09-29 정리)**: 작업 전에 [docs/sources/README.md](docs/sources/README.md) 를 먼저 본다 — 규칙·조작·유닛 수치와 **현재 분석과의 불일치 12건**(3절)이 정리되어 있다. 사용자가 외부에서 추가한 [`originals/help/manual.pdf`](originals/help/manual.pdf)는 조작·생산 절차와 일부 비용을 `GAME.HLP`·보유 `.type`에 [선별 대조](docs/sources/pdf-manual.md)했다. 메인/컨텍스트 메뉴 그림은 실제 UI 형태와 부합한다(사용자 확인). 옛 메인 메뉴의 기본 항목은 7개, 보유 패치판은 `Edit` 포함 8개다. 비용·일부 메뉴 항목은 판본에 따라 다르며 전체 페이지 대조는 남아 있다. 우선 확인할 것:
   - ~~level 1 원소 유닛(Bulf·Arc Spire·Crystal Crab)의 필요 에너지 exe 분석~~ → **2026-09-29 완료**: `Rifttype.cpp` `0049b0d0`이 타입 `+0xA0`의 기본 요구값을 만들고, `Mana.cpp` `004734d0`·`00473330`이 배치 위치에서 검사한다. Bulf·Arc Spire는 Thunder 1, Crystal Crab은 Rain 1. Generator는 `.type`의 명시 `mana = "s"`로 아무 공급원 1개. 이전 후보 `0044ac30`·`00473d00`은 다른 구조체 필드로 인한 오인. [분석](docs/exe/energy-requirements.md), [유닛 표](docs/gameplay/elements-energy.md) 4절. ~~**후속**: 패치판 게임에서 교차 원소 공급원 아래 세 유닛의 배치 성공/실패를 동적으로 확인~~ → **2026-09-29 사용자 확인으로 확정**(Bulf는 Thunder 공급원 필수, 다른 원소는 소용없음, 같은 레벨이라도 유닛마다 다름). PDF 유닛 핸드북 전체와도 대조함 ([PDF 대조표](docs/sources/pdf-manual.md#유닛별-건설-에너지-energy-to-build)) — 게임 실행 검증 불필요
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
