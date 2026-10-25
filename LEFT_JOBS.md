# LEFT_JOBS — cpppj 및 공통 인수인계

이 문서는 cpppj와 공통 사항의 **현재 상태, 앞으로 할 작업, 유효한 규칙**만 담는다. 2026-10-26에 정리했으며, 그 전까지의 단위별 작업 이력(단위444까지)은 [LEFT_JOBS.old.md](LEFT_JOBS.old.md)에 원문 그대로 옮겼다. dotnetpj는 [LEFT_JOBS.dotnetpj.md](LEFT_JOBS.dotnetpj.md), webpj는 [LEFT_JOBS.webpj.md](LEFT_JOBS.webpj.md)를 본다.

## 0. 재개 전 확인 사항

- 작업 전에 [AGENTS.md](AGENTS.md), 이 문서, [LEFT_JOBS.dotnetpj.md](LEFT_JOBS.dotnetpj.md)를 읽는다.
- **마지막 전체 정밀 디컴파일: HJOW-Athlon (192.168.0.94) / 2026-10-23.** 결과는 `extracted/refined/`(Git 제외)에 있다. 현재 PC가 이와 다르면 AGENTS.md 규칙에 따라 재디컴파일한 뒤 작업한다. 절차는 `tools\ghidra\refine_all.ps1`이며, 힌트 생성 단계가 콘솔 인코딩(cp949) 오류로 멈추면 `PYTHONUTF8=1`, `PYTHONIOENCODING=utf-8`을 설정하고 다시 실행한다.
- 2026-10-26의 문서 정리는 HJOW-X3D에서 인수인계 문서만 고친 것이다. 디컴파일 결과·제품 코드·검사 코드는 바꾸지 않았으므로 위 디컴파일 기록은 그대로 유효하다. 사용자는 HJOW-Athlon에서 작업을 재개할 예정이다.
- **실제 게임 실행 확인:** HJOW-Athlon, VM-W11-Codex, vm-debian-codex는 AGENTS.md의 예외 시스템이다. 그 밖의 PC에서는 게임이나 별도 창을 띄우기 전에 목적을 설명하고 사용자 확인을 받는다.
- **목표 상태:** 2026-10-26 사용자 요청으로 작업을 중단하고 인수인계를 작성했다. 451까지 완료했고, 452는 Source4/build4 컴파일 실패 상태로 보존한다. 452 집중·GUI 검사는 실행하지 않았다. 문서와 미완료 변경을 커밋한 뒤 목표를 일시 중지한다. 재개 시 3절 0번부터 확인한다.
- **원시 증거 위치:** 빌드·검사·GUI 원시 로그와 통합 드라이버는 `extracted/`(Git 제외)에 있으며, 작업한 PC(주로 HJOW-Athlon)에만 있다. 커밋되는 결과 보고서는 `cpppj/*.json`, 작업 문서는 `docs/exe/cpp-*.md`다.
- 원격 저장소에 push하지 않는다. 커밋 메시지는 `YYYYMMDD-NN CP 요약` 형식이며, NN은 `git log`로 그날의 마지막 번호를 확인해 정한다.

## 1. 현행 목표 ([TODO.md](TODO.md))

- Early Missions 6개, 캠페인 전체 16개, Priest Training 9개, 데모 3개를 사용자가 플레이할 수 있고, 원본 10.78과 완전히 같게 동작하도록 한다. Priest Training은 2026-10-26 사용자 지시로 목표에 추가했으며, TODO.md에도 반영됐다.
- Edit 메뉴(커스텀 맵·시나리오 포함)를 구현한다.
- 메인 메뉴 중 Multiplayer를 제외한 나머지 메뉴를 구현한다.
- 게임 진행 중 UI(좌측 덱, 좌측 하단 미니맵, 다리 건설 단축키 QWASZX, ESC 메뉴)를 구현한다.
- 작업 단위마다 이 문서를 갱신하고 커밋한다. 커밋에 실패해도 작업은 이어 간다.

대상 미션 (원본 `d/offical1~5.english`의 MissionBegin 순서와 `tell.DEMOVILLE`의 데모 순서):

| 구분 | 미션 키 (원본 표시 이름) |
| --- | --- |
| Early Missions (offical1) | Tutorial1 (Bridge the Gap), Tutorial2 (Secret Workshop), Tutorial3 (Capture The Priest), Tutorial4 (Tactical Combat), Tutorial5 (Subtle Defense), Tutorial6 (Raw Power) |
| 캠페인 1장 (offical3) | TheWarBegins, MasterOfWhirligigs, SaveTheIsland, FragileFortune, ThunderingPower, DissolvedAlliance |
| 캠페인 2장 (offical4) | TheNoose, RainVsRain, RunForIt, GuardMyBack, Surrounded |
| 캠페인 3장 (offical5) | BreakingThrough, ToTheRescue, Vicious, EnemyTerritory, FinalConfrontation |
| 데모 | DemoStormRages (The Storm Rages), GuardMyBack (Demo of "Guard My Back"), DissolvedAlliance (Demo of "Dissolved Alliance") |
| Priest Training (offical2) | bc1menu76, bc2bridging76, bc3acid_bar76, bc4bow_rush76, bc5walkers76, bc6mows76, bc7sun_bars76, bc8fury76, bc9ssun_bars76 |

- 데모 2·3은 캠페인 2-4·1-6과 같은 원본 자원을 쓰지만, 일반 플레이와 데모 모드는 따로 검증한다([데모 대응 근거](docs/gameplay/demo-campaign-map.md)).
- 메뉴 진입 검사(34경로)는 위 일반 미션 31개(Early 6 + 캠페인 16 + Priest Training 9)와 데모 3개를 모두 포함한다.

## 2. 현재 상태 (2026-10-26 기준)

### 2.1 마지막 통합 검증 기준

- **최신 통합 검증 제품(450):** Source1(2,038소스 동결), 게임SHA`b5966c23…`, build1 경고/오류0, 집중487고유/489관측·실패0(신규5개), 같은 제품 전체CTest **3076개·실패0**·GUI2·자연Early2→Tutorial3 통과. 447의3,052개와448/449 신규19개·이번5개 실제 이름 집합을 대조했다. [450 보고서](cpppj/squid-pool-reset-runtime-20261026.json), [450 문서](docs/exe/cpp-squid-pool-reset-runtime-20261026.md). 직전447(`97bef859…`)도 전체3,052·GUI2·자연Early2를 통과했다. 아래443·444 기록은 이전 제품이다.
- 제품: 443·444 준비본 통합 Source4(2,027소스 동결), 게임 실행 파일 SHA `423d0911…`. 시작 HEAD는 `9f5768a4`이고 통합 도중의 `1eddb0e8 / 20261026-07 CM`은 국·영 README만 변경했다.
- build4 종료0·경고/오류0, 집중 검사422고유/430관측·실패0(신규20개 전부), 전체 CTest **3,027개·실패0**, 메뉴 GUI2실행, 자연 Early2 → Tutorial3 진입 통과. 근거는 [실행 보고서](cpppj/cursor-retirement-runtime-20261026.json)와 [작업 문서](docs/exe/cpp-cursor-retirement-runtime-20261026.md)다. 이전3,007개 실제 검사 이름도 모두 보존됐다. 최초 빌드 두 실패와 집중의 신규 fixture3실패는 원시 로그에 보존했고 제품 관문·기존 기대값을 완화하지 않았다.
- 34경로(일반 31 + 데모 3) 초기 진입과 자연 Early1·Early2 완주를 함께 통과한 마지막 기록은 단위423 제품(`a1bfa97d / 20261025-17 CP`)이다. 그 뒤의 제품에서는 34경로와 Early1을 다시 돌리지 않았다.
- 보호 파일 기준은 1,821개다. 게임 실행 전후 `originals/` 전체 해시가 같아야 하고, 허용 설정 파일(options.cfg, fullscreenStateFile.dat)은 복원한다.

### 2.2 미션별 상태

| 대상 | 현재 확보한 증거 | 남은 확인 |
| --- | --- | --- |
| Early1 | 북서 간헐천 경로로 A~G 안내, 채취·귀환, SP 0→600, 완료 후 Tutorial2 진입 | 남동 간헐천 경로 실패 원인(2.4), 실패·재시작·F8, 표시·음향 대조 |
| Early2 | A~I 안내, 신전·워크샾·지식·생산·발사기·회수(환불 75), 완료 후 Tutorial3 진입. 통합 검증마다 재확인 | 재시도·취소, 표시·음향 대조 |
| Early3~6 | 진입, Start, 전용 단계 판정, 짧은 진행. Early5는 과거에 F 단계(오벨리스크 획득)까지 부분 진행 | 각 미션 자연 완주와 다음 미션 진입 |
| 캠페인 1-1 | 자동 AI가 활동하는 세션에서 실제 캐논으로 적 신전 HP 5000→0(단위322) | 적 사제 포획·운반·희생, 자연 승리, 1-2 진입, 패배·Retry |
| 캠페인 1-2~1-6 | 진입, Start, 자동 AI·Kernel 400프레임 진행(단위321) | 자연 플레이, 승패, 후속 명령 |
| 캠페인 2장·3장 | 진입과 Start 통과 | 등장 TYPE의 미연결 경계 점검 후 자연 플레이 |
| 데모 3개 | 실제 메뉴 진입, 15초 안내 시간 초과, 로컬 AI를 포함한 자동 진행 진입 | `demoRestartTimer=360` 만료 뒤 재시작 반복과 정리, 데모 분기 안내 |
| Priest Training 9개 | 진입 통과(bc8fury76의 기술 선택 정지는 단위376에서 해결) | 각 스테이지의 자연 완주와 완료 안내·메뉴 복귀 |

### 2.3 기능 영역별 상태

- **메인 메뉴:** 원본 10.78의 8버튼(Campaign, Multiplayer, Demo, Help, Edit, Credits, Options, Quit)을 배치했다([UberGump.cpp](cpppj/src/client/UberGump.cpp)). Campaign(UCampaign 목록), Demo, Help, Options(해상도·음량 등)는 실제 GUI 검사에서 확인했다. Credits(`Tell Credits`)와 Quit도 연결돼 있지만 cpppj에서 따로 검증한 기록은 없다. Multiplayer는 이번 범위 밖이라 `NotImplemented` 안내를 띄운다. 남은 메뉴 작업은 Edit 완성과 Credits·Quit 확인(3절 7번)이다.
- **Edit 메뉴:**
  - 메인 Edit → 편집 가능 맵 목록(원본 50칸 상한, `mayEditMissions` 옵션) → 선택 예약(State1) → LoadEditable 안내·Cancel까지 실제 GUI로 확인했다.
  - Editor Session 조립(기존 Fort의 최초 World·Deck, 작업본 파일 장치, 준비 대여·UI 설치, 로컬 전송·요청 큐)과 전체 Fort 저장·재파싱·재로드는 단위 검사에서만 동작한다. 제품에서 편집 화면을 설치하는 `RawCampaignSession::InstallExistingEditorCheckpoint`([RawCampaignSession.cpp](cpppj/src/client/RawCampaignSession.cpp))는 아직 테스트에서만 호출된다.
  - Create New Map(NewFort), 편집 도구, Save as, Test Battle, 편집 화면 복귀, 언어별 시나리오 적용은 미구현이다.
- **게임 중 UI:**
  - QWASZX: 문자 입력 → 생산 슬롯 예약 → 취득·회전·배치·보충을 실제 GUI로 확인했다(단위366, [문서](docs/exe/cpp-production-shortcuts-20261023.md)).
  - ESC 메뉴: 원본 가로 메뉴(Game·Edit·View·Options·Players·About)와 하위 명령·사건 큐를 연결했다(단위369, [문서](docs/exe/cpp-game-menu-runtime-20261023.md)). 원본에 없던 임시 Game 버튼은 2026-10-25에 제거했다.
  - 좌측 덱·미니맵: 실제 HUD 루트·도킹·구분선과 생산·미니맵·NSInfo·Chat 자식을 실제 미션에 연결했다(단위367·368·417·418, [문서](docs/exe/cpp-uber-tree-20261025.md)).
  - 남은 것: 넓은 dock 드래그 실제 검증, 전체 View 명령과 픽셀 동일성, 카메라 전체 Frame/zoom 부모, Chat 입력(F3·전송·클립보드)과 Chat GUI 드래그, 미니맵 색 억제·독립 토글.
- **공통 전략 AI:** 적·아군·데모 로컬 AI를 같은 원본 전략으로 복원하고 `RawCampaignSession::RunFrame`(현재 시각 → 정지하지 않은 AI → Kernel)에 연결했다(단위320~322, [문서](docs/exe/cpp-ai-automatic-session-20261022.md)). 정상 행동 간격에서 확장·채취·생산·공격이 원본과 같은지는 자연 플레이로 대조하지 않았다.
- **주문·의식:** Devastation의 시전 → 서버 → 효과와 사제 포획·운반·제단·다이스·룬·희생·SP·미션 완료를 연결했다. 집중 13검사는 통과했지만 일부 입력은 명시한 시전원·포로·시계 값이었다([문서](docs/exe/cpp-ritual-runtime-focused-20261021.md)). 은신·마비(단위349)와 윈드 소환 세 주문(단위350)도 복원했다. 실제 UI에서 취득 → 시전 → 포획 → 희생으로 이어지는 자연 플레이는 검증하지 않았다.
- **유닛·건물:** 워크샾 25종(썬 6·레인 7·윈드 6·썬더 6)의 생성·이동·공격·표시·소유자 색을 복원했다(단위335~345, [목록](docs/exe/cpp-unit-restoration-20261022.md)). 캠페인 2·3장에서 추가로 쓰이는 TYPE의 전체 수명은 점검하지 않았다.
- **내부 상태 수명(단위425~451):** Client와 Session이 공유하는 원본 장기 전역(카메라64BYTE, 상태 보조, 게임 보조18필드, 프로세스Host, Graph, TotalMade, 섬·청크·사제, 선택·권한·FullDeck, 연결 활동 표, 커서30DWORD·Lifecycle/Placement/snap)을 실제 소유자로 옮겼다. AI reset·Player 링크 정상 복귀의 이동 증빙과 중복 정리 방지, 사제4leaf·공통Carrier/생산·Damageable finder 및 피격·건설·삭제·채집10State의 현재 Client boss/server 조회 및 실제 Player 신전 SID에 따른 HP 회복을 통합했다. 448~450에서 실제 State 직접 전역, ClockSync·Zone·MetaZone·마커, 전체 Pool 부모·삭제 기록·공유 실패점을 추가 연결했다. 451에서 전체 Deck 명령 부모와 실제 GetFullDeck 입력도 같은 소유자에 연결했다. 계획은 [월드 장기 소유 계획](docs/exe/cpp-state-world-owner-plan-20261025.md)에 있다. Native State1 전체 cleanup/prefix와 반납 coordinator는 남았다.
- **배포:** [cpppj/package-cpppj.ps1](cpppj/package-cpppj.ps1)을 만들었다(단위359). 자동 빌드 분기(`-NoBuild` 없이 실행)는 아직 실제로 돌려 보지 않았다.

### 2.4 알려진 미해결 이슈

- **Early1 남동 경로 실패:** 남동 간헐천 경로 입력은 D 단계·SP 0에서 Next Tutorial을 찾지 못하고 종료 1이 된다(단위417). 북서 경로는 통과한다. 원인은 확정하지 않았다.
- **options.cfg 저장·복구 실패:** 1280×720 GUI 검사의 음량 변경 구간에서 SaveOptions의 fopen이 errno 22 / doserrno 1224로 실패하거나(단위393), 검사 도구의 설정 열기·복구에서 OSError22가 간헐적으로 난다(단위369·386). 격리한 데이터 복사본 경로에서는 통과했다. 원본 폴더가 있는 H 볼륨은 exFAT다. 원인은 확정하지 않았다.
- **오래된 스모크 6개:** `cpp_menu_smoke`, `cpp_rawbuttons_smoke`, `cpp_thunder_smoke`, `cpp_weather_smoke`, `cpp_audio_smoke`, `check_dialog_controls.check_mission`은 제거된 임시 Game 버튼을 전제로 한다. 다시 쓸 때 ESC → 실제 메뉴 입력으로 바꾼다.
- **PlayerNoMake 경고 표:** `0054d4e8`(255 DWORD)의 실제 제품 소유자를 확인하지 않았다(단위389).
- **README 갱신 완료:** 별도 문서 커밋 `1eddb0e8 / 20261026-07 CM`에서 국·영 README를 현재 범위와 세 프로젝트의 플레이/동일성 구분에 맞게 갱신했다.

## 3. 앞으로 할 작업 (권장 순서)

### 0. 2026-10-26 사용자 요청 중단 — 452 미완료 인수인계

- **중단 지시:** 사용자가 작업 중단 → 인수인계 작성 → git 커밋 → 목표 일시 중지를 요청했다. 요청 뒤 제품 코드·검사를 고치거나 새 빌드·테스트를 실행하지 않았다. 문서와 중단 기록만 작성했다.
- **마지막 완료:** `1ecdfa2a / 20261026-15 CP`의 451이다. 집중 459고유/461관측·신규 5개·GetFullDeck 실제 입력 GUI를 통과했다. 마지막 전체 통합은 다른 제품인 450(`74bc5941`)의 3,076개·GUI 2실행·자연 Early2다. 현재 452 코드를 빌드·플레이 가능 상태로 간주하지 않는다.
- **452 변경:** 단일 `CursorDropState::preview`를 `uint32_t`로 보존하고, Drop의 캡처·준비 prefix와 실제 SID16 소비를 분리했다. Lifecycle·배치·미리보기 품질·보고와 ContentLifetime의 값형·현재 getter도 같은 DWORD를 쓴다. 기존 검사 8파일의 입력형·접근형을 대응했고 신규 6개를 추가했다. 원문 280관측(정상 240/준비 중단 40)과 추가 커서 비교 leaf 56관측은 제품 변경 전에 고정했다. Native 가상·GUI·Cancel 경계의 범위는 각 보고서와 작업 문서에 구분했다.

| 시도 | 실제 결과 |
| --- | --- |
| Source1 / build1 | VS18 설치 미완료로 컴파일 전에 종료 1, 0.130초. 캐시를 강제 변경하지 않았다. |
| Source2 / build2 | 설치 완료 뒤 Content 현재 커서 getter가 기존 Sid를 반환하여 오류 1, 종료 1, 63.739초. 논리 비교를 DWORD로 옮겼다. |
| Source3 / build3 | 제품 컴파일 뒤 기존 Campaign 검사의 Sid 접근형 누락으로 10진단, 종료 1, 309.911초. 동일 숫자의 checked 변환·정수 비교로 대응했다. |
| **Source4 / build4** | **종료 1, 컴파일 진단 14개·경고 0, 99.755초. 신규 ContentLifetimeTests.cpp:191의 `std::filesystem::path` 정의 누락이 첫 오류다. 아직 수정하지 않았다.** |

- **재개 첫 수정:** [ContentLifetimeTests.cpp](cpppj/tests/ContentLifetimeTests.cpp):191과 [CursorDropTests.cpp](cpppj/tests/CursorDropTests.cpp):180의 신규 fixture 조회부터 점검한다. `<filesystem>`의 명시 포함이 필요하며, 기존 [RawSceneSupport.h](cpppj/tests/RawSceneSupport.h):88의 `LoadFixture`는 `const char*`만 받는다. 새 코드는 `filesystem::path`를 직접 넘겼으므로 문자열 인자 또는 정확한 파일 입력으로 고쳐야 한다. 첫 include 오류에 가린 후속 형식 오류가 있을 수 있으니 기존 단언·Native 행을 약화하지 말고 빌드로 확인한다.
- **실행 여부:** 452의 집중·GUI·전체 CTest는 전부 미실행이다. `focused1/` 성공 자료와 452 runtime 결과 JSON도 아직 없다. 남아 있는 실행 파일을 452의 성공 산출물로 쓰지 않는다. 게임 실행 파일은 중간 제품일 수 있고 테스트 실행 파일은 이전 빌드일 수 있다.
- **현재 프로세스:** 중단 확인 시점의 build4와 자동 집중 대기기는 이미 위 실패로 종료했다. 세션 `2828`(build4), `26873`(after-build), `27684`(설치 대기 pipeline1), `59287`(build3)은 모두 종료 상태를 확인했다. `extracted/cpp-cursor-preview-width-452-user-stop.json`의 stopped/remaining은 빈 배열이다. 실제 빌드·cl/link·게임·테스트 프로세스도 없었다. 자동 재시작하지 않는다.
- **환경 변경:** 작업 중 기존 VS2026 설치 프로세스 24908/5060이 실행되며 VS18 인스턴스가 일시 미완료였다. 이후 `vswhere -all -products * -utf8 -format json`으로 `18.11.12324.101`·isComplete/isLaunchable true를 확인했다. 새 MSBuild는 `18.11.1-1.26513.3`이다. 사용자 설치 프로세스는 종료시키지 않았다. 현재 PC는 HJOW-Athlon이고 마지막 전체 정밀 디컴파일은 같은 호스트의 2026-10-23 결과다.
- **동결·보호:** Source4는 2,039소스이고 신규 검사는 6개다. 중단 후 동결 소스와 보호 1,821파일의 SHA, 원본·CD 1,819파일 집합 유지를 다시 확인했다. `AGENTS.md`, 원본·CD, 기존 Native fixture는 바꾸지 않았다.
- **원시 재개 자료(HJOW-Athlon에만 있음):** `extracted/cpp-cursor-preview-width-452-` 접두어 아래 `source1~4/hashes.json`, `build1~4.log`와 각 `buildN-result.json`, `pipeline1.json`·`pipeline2.json`(둘 다 실패), `native1.log`, `content-native1.log`(가상표 입력 누락 실패)·`content-native2.log`(성공), `test-migration.json`, `resume.json`, `user-stop.json`을 본다. Native 보고서 2개·fixture 2개·관측 도구 2개와 작업 문서는 Git에 보존한다.
- **재개 순서:** AGENTS.md·본 인계·닷넷 인계를 읽고 호스트·VS·작업 트리를 확인한다 → 위 테스트 컴파일 오류 수정 → 새 Source5 동결 → `python extracted/cpp-cursor-preview-width-452-build.py 5 5` → 종료 0·경고/오류 0 확인 → 새 집중 시도(현재 focused1이 없으면 `python extracted/cpp-cursor-preview-width-452-focused.py 1 5`) → 실제 QWASZX·회전·Drop GUI → 보고서·인계 갱신과 완료 커밋. 기존 pipeline1과 after-build.py는 실패한 build2/build4에 고정돼 있으므로 재사용하지 않는다.
- **커밋 뒤 동결 드라이버 주의:** 현재 `freeze.py`는 현재 HEAD와 비교해 신규 6개를 요구한다. 이번 미완료 커밋에는 그 6개가 이미 들어가므로 그대로 실행하면 단언이 실패한다. 재개 시 EOL 보존용 현재 HEAD와 검사 집합 비교 기준을 구분하고, 후자는 원래 기준 `1ecdfa2a9ffedd927a30a9271ce7bd39043da73d`로 유지한다. Source5 manifest의 `head`와 `newTestCases`도 이 기준·기존 신규 6개를 기록해야 `focused.py`가 같은 신규 집합을 검증한다. 성공 조건을 없애거나 기대 개수를 임의로 낮추지 않는다.
- **GUI 계획만 있음:** `tools/cpp_production_shortcuts_smoke.py`의 기존 실제 입력을 사용하려 했다. 원본을 별도 폴더에 복사한 환경에서 실행하는 wrapper는 아직 작성하지 않았다. 이 도구는 `--output`만 받고 ROOT/originals를 고정하므로 코드를 먼저 읽는다. 기존 프로필은 `useProductTimers=0`이며, 원본 제품 타이머로 자연 완주한 검증과 구분해 기록한다.
- 452 완료 뒤에도 다른 SID16·전체 Deck nullable `+ac`·작은 leaf·Native State1 전체 Effect/prefix·Runtime/Player 고정뷰 반납과 실제 Edit 설치, 전체 Early·훈련·캠페인·데모 자연 완주와 동일성 목표가 남는다. 사용자가 재개하기 전에는 목표 작업을 진행하지 않는다.

### 1. 443·444 준비본 병합과 검증 (재개 시 첫 작업)

- **2026-10-26 완료:** 시작 HEAD `9f5768a4`에 443·444를 hunk/삼방향 병합했다. 443 입력28→결과32, 444 결과10의 보관 SHA 재현을 확인했다. CMake 두 곳과 편집 checkpoint 검사 끝의 추가 충돌은 기존 441·442와 새 항목을 함께 보존했다. Source4(2,027파일) Release 빌드 종료0·경고/오류0, 집중422고유/430관측·실패0, 신규20개 전부 실행, 같은 최종 게임`423d0911…`의 메뉴 GUI2실행·자연Early2→Tutorial3·전체CTest3,027개 통과를 확인했다. 최초 빌드 두 차례와 최초 집중의 신규 Session fixture3실패는 지우지 않았으며 입력형/include/외부 fixture만 교정했다. [작업 문서](docs/exe/cpp-cursor-retirement-runtime-20261026.md), [실행 JSON](cpppj/cursor-retirement-runtime-20261026.json), 원시`extracted/cpp-cursor-retirement-443444-*`를 본다.
- [x] 보관 위치와 재현 방법은 [준비본 보관 안내](docs/exe/prepared/README.md)를 본다. 두 준비본을 재현·검토하고 현재 main에 병합해 빌드·집중 검사를 통과했다. 아래 준비본 설명은 보관 당시 기준이다.
  - **443 커서 장기 상태**([README](docs/exe/prepared/cursor-owner-443-20261025/README.md)): 입력 28 → 결과 32, 실제 patch 30파일, 신규 8검사. 미리보기 30 DWORD, Lifecycle/Placement/snap 데이터와 실제 Cancel 연결을 준비했다. patch SHA `6aa4a0ab…`, manifest SHA `53ec6d6d…`. 기준은 `815fbd67` + 439·440 준비본 + 439 Load setup 교정본의 명시 혼합이며, 442와 현재 Source6 95파일은 반영되지 않았다.
  - **444 AI·Player 링크 반납**([README](docs/exe/prepared/state-runtime-retirement-444-20261025/README.md)): 제품 7·검사 3파일, 신규 12검사. 실제 자식 정상 복귀의 이동 전용 증빙이다. Session.h/.cpp 기준은 442 준비본이고 나머지 8파일은 `815fbd67`이다. patch SHA `7536a350…`, manifest SHA `529b43bf…`. **2026-10-26 root가 원문 순서·실패 prefix·호출 대여·이동 증빙·고정뷰 반납 경계를 검토했다.**
- [x] 보관 base를 덮어쓰지 않고 별도 디렉터리 재현 뒤 현재 코드와 hunk 단위로 병합했다. 보관 SHA에 맞는 EOL과 실제 현재 코드의 기존 줄바꿈을 구별해 보존하고 Source1~4를 각각 동결했다.
- [x] 빌드 → 신규20개 포함 집중 → 동일 제품의 통합batch(전체CTest3,027개, 메뉴GUI, 자연Early2)를 완료했다. 이번 드라이버는 `extracted/cpp-cursor-retirement-443444-{build,focused,batch,gui,finalize}.py`이고 최종 원시는`batch1/`이다.
- [x] **445 완료:** 사제 HP·회복·낙하·Carrier·내용물과 공통Carrier/생산·Damageable finder를 같은 Client boss 주소에 연결했다. Source3 빌드 경고/오류0, 집중515고유/684관측·실패0(신규6개 전부)이다. 첫 집중의 신규 실제 Session1실패는 합성 최대HP200의 경계100을 원본 타입에도 적용한 검사 오류였으며, 독립 원본`priest.type` 최대HP100·원문004942b0의 절반50을 대조해 수정하고 초기값 단언을 추가했다. 합성/기존 Native 기대값과 제품은 그대로다. PowerShell 인코딩으로 물음표가 된 새 주석·이전 보고서 설명은 UTF-8 한국어로 복원했으며 원시 로그·증거 SHA는 유지했다. [445 문서](docs/exe/cpp-priest-authority-runtime-20261026.md), [실행 보고서](cpppj/priest-authority-runtime-20261026.json), 원시`extracted/cpp-priest-authority-445-*`를 본다.
- [x] **446 완료:** 피격·삭제·건설·Content·빌더·간헐천10개 State의 boss/server를 실제 Client 주소로 연결하고 Prepare의 복사를 제거했다. Source3 빌드 경고/오류0, 집중480고유/480관측·실패0(신규12개 전부). 원문22함수의 조회 위치와 CD 차이를 대조하고 기존269사례 바이트·fixture10개 내용을 보존했다. [446 문서](docs/exe/cpp-runtime-authority-20261026.md), [실행 보고서](cpppj/runtime-authority-20261026.json), 원시`extracted/cpp-runtime-authority-446-*`를 본다.
- [x] **447 완료:** 실제 Player9 고정 기록의 신전 SID DWORD를 HP·Regular 회복이 직접 읽도록 연결했다(PATCH stride0xb4/+7c, CD stride0xac/+74). Native setter3,936행·새 실제 PE63입력/126회·기존231사례를 보존해 신규7개와 현재 DWORD·조회 생략·Unpop 시점·실제 보호막 정리·Client Session을 확인했다. Source1 빌드 경고/오류0, 집중479고유/480관측·실패0, 같은 게임 전체CTest3,052·GUI2·자연Early2→Tutorial3 통과. [447 문서](docs/exe/cpp-priest-recovery-runtime-20261026.md), [실행 보고서](cpppj/priest-recovery-runtime-20261026.json), 원시`extracted/cpp-priest-recovery-447-*`를 본다.
- [x] **448 완료:** Native cleanup 직접 대상(DWORD 읽기69·쓰기63/프레임22포함, WORD1, double9, 현재 Player/TYPE/Template)을 실제 Client owner에 연결했다. 사제 HP1/4와 건물 숨김도 생성·HP·연소·Uber 메뉴가 같은 장기 필드를 읽는다. 기존230사례 본문·Native fixture4개와 음향 초기화 본문2231BYTE를 보존했다. Source1 명시 중단·Source2 신규7장면의 초기화 전 checked getter 실패를 원시 보존하고, 기존 API를 유지한 순수 초기화 조회로 수정했다. 최종 Source3 빌드 경고/오류0, 집중489고유/491관측·실패0(신규11개), 같은 제품 메뉴/초기 플레이 GUI2 통과다. [448 문서](docs/exe/cpp-state-client-globals-20261026.md), [실행 보고서](cpppj/state-client-globals-20261026.json), 원시`extracted/cpp-state-client-globals-448-*`를 본다. 34개 Effect/반납 coordinator·자연 Edit 진입은 아직 미완료다.
- [x] **449 완료:** ClockSync3 DWORD와 Zone6BYTE·MetaZone51기록/flag를 같은 ClientLocalStateGlobals에 연결하고 실제 로그인도 같은 Zone/ticks를 빌린다. 마커 signed 상태·DWORD SID·현재 월드의 실제 가상 삭제→SID0, Zone의4+2BYTE 정리를 복원했다. 제품 전에 원본PE 마커86·Zone10관측을 고정했고 기존229사례 본문·Native fixture3개를 보존했다. Source1 빌드 경고/오류0, 집중480고유/480관측·실패0(신규8개), 같은 제품 Early1 실제 로그인/화면 GUI1 통과다. [449 문서](docs/exe/cpp-state-storage-runtime-20261026.md), [실행 보고서](cpppj/state-storage-runtime-20261026.json), [Native 보고서](cpppj/state-storage-leaves-native-20261026.json), 원시`extracted/cpp-state-storage-449-*`를 본다. 전체 State1·시각 동기화·MetaMap·자연 Edit 진입 완료는 아니다.
- [x] **450 완료:** 기존 SidPool 삭제 기록 두20쌍(80DWORD)을 쓰는 실제 Pool 부모를 복원하고 실패점 BoardTargetState를 Storage 수명으로 옮겨 Board/AI가 직접 빌린다. 제품 전 두 PE에서 부모·하위 Reset을 치환 없이12관측(각82store)했다. 하위 Reset·Record·Release 본문은 바이트 보존, 기존46사례/fixture3개 내용도 유지했다. Source1 빌드 경고/오류0, 집중487고유/489관측·실패0(신규5), 같은 제품 전체CTest3076·GUI2·자연Early2→Tutorial3 통과. [450 문서](docs/exe/cpp-squid-pool-reset-runtime-20261026.md), [실행 보고서](cpppj/squid-pool-reset-runtime-20261026.json), [Native 보고서](cpppj/squid-pool-reset-native-20261026.json), 원시`extracted/cpp-squid-pool-reset-450-*`를 본다. 전체 State1·자연 Edit 완료는 아니다.
- [x] **451 완료:** 전체 Deck 부모0044f9d0와 GetFullDeck(00542a28/020000cd)을 같은 Client FullDeck·현재 Player Deck·공유 난수·생산 dirty에 연결했다. 제품 전 실제 부모230관측(실제FullDeck/Append/Entry/난수 포함)·Guide/Gump/입력384관측을 고정했다. Source1 span at 컴파일 실패와 Source2 검사 optional 비교 경고2를 보존했고 Source3(2,039소스) 빌드 경고/오류0, 집중459고유/461관측·실패0(신규5개)을 확인했다. 별도 자료 복사본의 검사 Tell 버튼을 실제 클릭해 같은 Client FullDeck0→1을 관측했다. 기존222사례 본문·fixture6개를 유지했고 원본/CD도 그대로다. [451 문서](docs/exe/cpp-full-deck-command-runtime-20261026.md), [실행 보고서](cpppj/full-deck-command-runtime-20261026.json), [Native 보고서](cpppj/full-deck-command-native-20261026.json), 원시`extracted/cpp-full-deck-command-451-*`를 본다. 전체 State1/Edit 완료는 아니다.
- [ ] **452 사용자 요청 중단·미완료:** 단일 CursorDropState.preview와 Content 현재 커서 값을 DWORD로 옮겼다. 두 PE Drop 280관측·내용물 커서 leaf 56관측은 고정했으나 신규 6개 C++ 검사는 아직 실행하지 않았다. Source4/build4는 ContentLifetimeTests.cpp:191의 filesystem 정의 누락에서 시작한 14진단으로 종료 1이었다. 빌드·자동 집중·게임 프로세스는 종료 상태다. 3절 0번의 오류와 재개 순서를 따른다. [작업 문서](docs/exe/cpp-cursor-preview-width-runtime-20261026.md), 원시 `extracted/cpp-cursor-preview-width-452-*`를 본다. 완료로 합산하지 않는다.
- [ ] **다음:** 아래 SID16/전체 Deck 잔여 경계를 대조하고, 남은 작은 leaf(255DWORD/20 pool header 등)와 Native State1 전체 cleanup/prefix·GameplayUse/고정뷰 반납 coordinator를 실제 Client에 접합한다. 그 뒤 실제 Edit 예약을 소비해 기존 checkpoint·HUD·팔레트·전송을 설치한다. 기존 단위 모듈/준비 checkpoint 통과를 전체 Edit 진입 완료로 합산하지 않는다.
- [ ] 436의 남은 기존 SID16 변환, 438의 전체 Deck nullable+ac와 Native cleanup 접합, 443의 단일 Drop.preview SID16 경계를 이어서 처리한다. 전체 Deck 명령 부모와 UserInput writer는451에서 완료했다.

### 2. Edit 메뉴 완성

- [ ] 맵 선택 뒤 예약된 Edit next1(State1)을 소비하도록 연결한다: Native State1 cleanup/prefix → 현재 Fort 이름·작업본 파일 → `InstallExistingEditorCheckpoint` → HUD mode4·팔레트 → 공통 State 확정·다음 Poll4 → loaded 공개. 기존 `InstallHud()`(`UberHud.inc`)를 재사용하고 중복 HUD 공장을 만들지 않는다. 원문 `004e22c0`은 Editor에서도 로컬 listener/client9를 만들므로, 로그인 없는 Editor에 동기 직접 수신이나 빈 전송을 붙이지 않는다.
- [ ] 편집 도구(지형·섬·오브젝트·소유자)와 ESC 메뉴의 Editor 행(Save as, NewFort, 다리 수준 0/1/2, Test Battle, Main Menu)을 연결한다.
- [ ] Create New Map(`LOADLOADFORT`, NewFort alternate1), Save as·저장·다시 불러오기, Test Battle과 편집 화면 복귀(`020000c2`), 종료(`LeaveEditMode`/`SaveGoMain`)를 구현한다.
- [ ] 언어별 시나리오 파일(`.english`)의 사용자·AI 구성, 시작 SP, 브리핑 적용을 연결한다.
- [ ] `originals/d/TEST01.fort`·`TEST01.english`와 TEST02는 읽기 참조로만 쓰고, 저장은 실제 작업본 경로에 한다.

### 3. 자연 완주 범위 확대

- [ ] Early3 → Early4 → Early5 → Early6 순서로 메뉴 → 브리핑·안내 → 실제 진행 → 완료 → 다음 미션 진입을 확인한다.
- [ ] 캠페인 1-1: 적 사제 포획·운반·희생 → 자연 승리 → 1-2 진입을 확인하고, 패배와 Retry도 확인한다.
- [ ] 캠페인 1-2 → 1-6 → 2장 → 3장 순서로 넓힌다. 각 장에 앞서 등장 유닛·건물·주문 TYPE을 목록화하고, 이동 분배(38/41/42), 총 피해·예약 피해 분배, 파생 삭제·내용물 통지, 파편 색표처럼 아직 fallback인 경계에 도달하는지 확인해 필요한 원본 몸체를 연결한다. Devastation 외 주문도 캠페인에서 쓰이면 범위에 넣는다.
- [ ] 2026-10-21 총정리의 PLAY-1(이동 정리 환불), PLAY-2(중립 회복·사제 중립 복귀), PLAY-5(Start의 세계 랜덤 간헐천 `00486440` 등) 경계는 이후 단위에서 일부 해결됐을 수 있다. 착수 전에 현재 소스의 미공급·`FillMissing` 경계를 다시 조사한다. 옛 목록은 [LEFT_JOBS.old.md](LEFT_JOBS.old.md)의 "현행 목표 잔여 작업 총정리" 절에 있다.
- [ ] Priest Training 9개도 각 스테이지를 브리핑 → 실제 진행 → 완료 안내 → 메뉴 복귀까지 확인한다. bc8fury76은 원소 선택(Sun 등) 분기가 있으므로 선택지마다 확인한다.
- [ ] **테스트용 로컬 owner AI 옵션 구현:** 일반 미션(demo0)을 유지한 채 로컬 owner에도 데모와 같은 원본 공통 AI를 생성·실행하는 명시 테스트 옵션을 만든다(예: [가능성 검토](docs/exe/cpp-test-local-ai-and-sp-feasibility-20261021.md)의 `--test-local-ai original`). 공통 AI의 자동 실행은 단위321에서 연결됐으므로 이제 구현할 수 있다.
  - 전체 데모 모드 전환(`gate.demo=1`)은 쓰지 않는다. Early 튜토리얼 처리 생략, 사건 이름의 `Demo` 접두어, 데모 종료 타이머가 함께 바뀌기 때문이다.
  - 로컬 AI 설정(StartMoney, Tech, 이름/Off/NoTemple/동맹/능력)은 공통·적 `ai...` 설정을 고치지 않고 별도로 준비한다. 적의 설정·동작 경로는 그대로 두고, 공유 난수 소비 차이만 보고한다.
  - 기본 게임·적 AI·저장 프로필에 섞지 않고, 원본 분석 결과가 아닌 테스트 조작임을 주석·기록에 표시한다.
  - 로컬 AI로 얻은 결과는 원본 규칙·연결 검증 자료다. 사람이 직접 입력한 자연 플레이 검증과 구분해 기록한다.
- [ ] **TEST02 포획·희생 테스트(보류 중):** 작업 AI가 직접 조작하면 진행이 너무 느리므로, 위 로컬 owner AI 옵션으로 데모의 아군 AI가 플레이하게 해서 테스트한다(사용자 결정, 2026-10-26 확인). 사용자 수정판(`3c12b4e7`, Fort 5,282BYTE)과 워크샾 25종 복원은 끝났다. 옵션 구현 뒤 진행하며, 시작 전에 Fort·언어 파일의 SHA와 배치를 다시 확인하고 옛 좌표는 재사용하지 않는다. 로컬 AI의 기술 구성을 정할 때는 보행 유닛 공격용으로 크로스보우·썬 디스크 발사기·맨오워풀·데빌메이커를 우선하고 훨리베이스는 제외한다는 기존 지시를 참고한다.

### 4. 미션 수명·진행 저장·데모 반복

- [ ] 완료 기록 0 → 1의 디스크 저장과 재실행 후 다음 미션 해제를 확인한다. 지금까지의 회귀는 기존 완료값 1을 보존한 상태에서만 통과했다.
- [ ] 자연 성공·실패 → 안내 → Retry/Restart, Next Tutorial/Next Mission, Abort/메뉴 복귀를 실제 버튼으로 확인한다. 연속 전환과 같은 미션 재시작 뒤에 참조·타이머·다리·AI 재고가 남지 않아야 한다.
- [ ] 데모 3개의 `demoRestartTimer=360` 만료 → 재시작 반복과 정리를 확인한다. GuardMyBack의 Demo 안내 15초와 DissolvedAlliance에 Demo 섹션이 없다는 차이를 유지하며, Script의 `demo` 키만으로 실행 모드를 추정하지 않는다.
- [ ] 전체 비동기 State7/전투 정리(`004b2df0`)의 남은 경계를 연결한다.

### 5. 남은 내부 수명·연결 경계

- [ ] Native State1 전체 cleanup/prefix, AI/Links/UI/Player 고정 뷰, GameplayUse 반납 coordinator, 다른 파생 프로세스 수명.
- [ ] 카메라 전체 Frame/zoom 부모, Renderer `004994b0`의 flags/override/dirty/시차 자연 Frame 연결.
- [ ] 연결 활동: frame22 clear, 주기 64 송신/timeout/UI 전체 부모, 실제 endpoint 삭제, 서버/boss 전체 owner.
- [ ] Chat manager·recipient·clipboard·전송, 전체 팔레트 전환과 디렉터리 이름 formatter.
- 참고: [State 정리의 Client 공백](docs/exe/cpp-state-cleanup-client-gap-20261025.md), [직접 전역 소유자와 반납 순서](docs/exe/cpp-state-cleanup-global-owners-20261025.md).

### 6. 원본 동일성 대조

- [ ] 표시: 목표 TYPE의 대기·건설·발사·피격·파괴·포획·희생 상태별 프레임, 방향, 색표, 그림자, 발 기준점, 가림 순서, 파편·지형 효과.
- [ ] 입력·UI: 메뉴·브리핑·생산·지식·선택·사거리·비용·커서/회전·회수·업그레이드·우클릭·전체 단축키·카메라/스크롤·Pause·Help, Chat/PlayerList 토글.
- [ ] 음향·시간: 건설·발사·적중·출격·귀환·연소·붕괴·포획·의식·승패·음악 전환의 파일·시점·반복·위치, Pause와 안내 정지 중의 시간 진행, 난수 소비 순서.
- [ ] 안정성: 창 모드 장시간 플레이의 크래시·무한 정지·자원 누적, 메뉴·데모 반복.
- 녹화 자료의 HDR 밝기와 방송 음성을 게임 효과로 오인하지 않는다.

### 7. 기타 정리

- [ ] 2.4의 미해결 이슈를 처리한다.
- [ ] **Credits:** 메인 메뉴 Credits 버튼은 창 하나를 띄운다. 원본 tell `[Credits]` 창(배경 nsstart)의 제목, 두 항목("Netstorm 10.72 Patch Credits" → `CreditsNew`, "Netstorm Original Credits" → `Creditsold`), Cancel 버튼의 표시·동작을 원본과 대조한다. 과거 원본 자동 분석(2026-09-30)에서 Original Credits는 14쪽이 20초씩 자동으로 넘어간 뒤 메인 메뉴로 돌아왔으므로, 하위 항목도 함께 확인한다.
- [ ] **Quit:** 클릭하면 게임이 즉시 종료되므로 화면 대조는 어렵다. 디컴파일한 원본의 종료 경로(종료 전 처리)를 정적으로 비교하고, 클론에서는 정상 종료(종료 코드 0, 프로세스·창이 남지 않음)와 재실행 시 오류가 없는지만 확인한다.
- [ ] 최신 소스 PC에서 `cpppj/package-cpppj.ps1`을 `-NoBuild` 없이 실행해 빌드 → 묶기가 이어지는지, `BUILD_INFO.txt`의 실행 파일 시각·SHA가 새 산출물인지 확인한다.
- [x] 국·영 README의 cpppj 진행 상태 갱신: `1eddb0e8 / 20261026-07 CM`에서 완료됐다.

### 이번 목표 밖 (AGENTS.md의 후순위 단계)

- 2차: Windows 10/11 안정 구동, 전체화면 DirectDraw 오류 수정, 4:3·16:9·16:10, 전체화면 가장자리 스크롤. 창 모드 완주만으로 이 의무를 완료 처리하지 않는다.
- 3차: TCP/IP 로컬 멀티플레이, Outpust, 멀티플레이 전용 분기.
- 4차 이후: 한국어, MCP, 60/120프레임, CD 10.72 전체 복원(10.72의 Windows 10 구동 불가 해결책도 그때 문서화한다).

## 4. 운영 규칙과 유효한 사용자 결정

- **긴 검사 묶음(2026-10-25 사용자 결정):** 작업 단위마다 34경로 진입, 긴 Early 완주, 전체 CTest를 반복하지 않는다. 단위별로는 원문 근거와 변경 부위 집중 검사, 필요한 증분 빌드, 인계, 커밋을 한다. 전체 CTest와 긴 실제 플레이는 기능 묶음 완성 또는 3~5개 단위 뒤 같은 최종 제품으로 통합 실행한다. 실제 입력·타이머·저장처럼 즉시 확인이 필요한 변경은 해당 장면만 짧게 먼저 실행한다.
- **기록 원칙:** 컴파일만 한 검사를 실행 통과로 적지 않는다. 명시 입력·정적 대조·컴파일·C++ 검사·GUI 관측을 구별한다. 최초 실패와 교정 과정의 원시 로그를 지우지 않으며, 검사 기대값을 제품에 맞춰 약화하지 않는다. 단위 검사 통과나 미션 진입만으로 완주·동일성을 선언하지 않는다.
- **테스트 전용 조작:** `--test-bridge-patterns` 같은 임시 조작은 원본 분석 결과가 아님을 주석·기록에 표시하고 기본 게임·적 AI·저장 프로필에 섞지 않는다. 우리 편만 AI 활성화·행동 빈도 증가·시작 SP 추가는 [가능성 검토](docs/exe/cpp-test-local-ai-and-sp-feasibility-20261021.md)만 했고 구현하지 않았다. 이 중 로컬 owner AI 옵션은 TEST02 포획·희생 테스트에 쓰기로 했다(3절 3번).
- **보호 파일:** `originals/`, `originalCD/`의 파일을 수정·삭제하지 않는다(options.cfg, fullscreenStateFile.dat 제외). 게임 실행 전후 `originals/` 전체 해시를 비교하고 허용 설정 파일을 복원한다. AGENTS.md도 수정하지 않는다.
- **빌드:** HJOW-Athlon은 Visual Studio 18(2026) 생성기·MSBuild 18.x를 쓴다. 최근 사용한 명령은 `cmake --build cpppj/build --config Release --parallel 4 -- /p:UseMultiToolTask=true /p:MultiProcMaxCount=4 /p:EnforceProcessCountAcrossBuilds=true`이며, 이전 컴파일러·CMake 자식 프로세스가 모두 끝난 뒤 시작한다. 상세는 [C++ 빌드](docs/cpp-build.md)를 본다.
- **PowerShell UTF-8:** 5.1에서 here-string을 Python stdin으로 넘길 때 `$env:PYTHONUTF8=1`만으로는 PowerShell의 기본 파이프 인코딩이 바뀌지 않는다. 한국어를 포함하면 `$OutputEncoding = [System.Text.UTF8Encoding]::new($false)`도 해당 셸에서 설정하거나 UTF-8 파일을 `apply_patch`로 작성해 실행한다. 445에서 새 주석/보고서 설명의 물음표를 발견해 복원했다. 기존 원시 로그와 Native fixture는 인코딩 정리 대상으로 덮어쓰지 않는다.
- **Git:** 작업 트리에 CRLF 파일이 있으므로 확인·스테이징에는 `git -c core.autocrlf=false`와 필요하면 `-c core.whitespace=cr-at-eol`을 쓴다. 저장소·전역 설정은 바꾸지 않는다. 다른 PC·세션의 변경이 섞일 수 있으므로 커밋할 파일만 지정해 stage한다.
- **검증 도구:** `tools/cpp_solo_entry_smoke.py`(34경로 진입, `--demos`로 데모 세 항목), `tools/cpp_demo_start_smoke.py`, `tools/cpp_tutorial_harvest_smoke.py`(자연 Early1), `tools/cpp_tutorial_two_smoke.py`(자연 Early2), `tools/cpp_game_menu_smoke.py`·`tools/cpp_game_menu_pause_smoke.py`(ESC 메뉴). 최근 통합 batch가 실제로 부른 도구와 인자는 `extracted/`의 batch·gui 드라이버에서 확인한다.

## 5. 주요 참고 문서

- [C++ 빌드](docs/cpp-build.md), [cpppj 로드맵](docs/cpp-roadmap.md), [플레이 가능 계획](docs/cpp-playable-plan.md), [소스 맵](cpppj/SOURCE_MAP.md)
- [모듈 맵](docs/exe/modules.md), [정밀 디컴파일 신뢰도](docs/exe/decompile-reliability.md), [2026-10-23 양 판본(10.78·10.72) 정밀 디컴파일 기록](docs/exe/cpp-decompilation-20261023.md)
- [원본 자동 탐험 도구 analyzeManager](docs/analyze-manager.md)
- [준비본 보관 안내](docs/exe/prepared/README.md)
- 단위별 상세 이력과 과거 근거: [LEFT_JOBS.old.md](LEFT_JOBS.old.md)

## 인수인계 문서 임시 분할

dotnetpj 와 병행 작업하기 위해, 
이 LEFT_JOBS.md 에는 당분간 cpppj 및 디컴파일 관련 진행 상황과 그 인수인계 내용을 적는다.
dotnetpj 진행 상황과 인수인계 내용은 LEFT_JOBS.dotnetpj.md 에 적는다
cpppj 1차 목표 달성 및 윈도우 10, 11 호환성 작업, MCP 추가 작업까지 완료되어 dotnetpj 구현에 cpppj 를 활용할 수 있게 되면, 인수인계 문서를 다시 LEFT_JOBS.md 로 통합한다.

---

**이전 내용은 [LEFT_JOBS.old.md](LEFT_JOBS.old.md)로 옮겼다.** 2026-10-26 정리 전 LEFT_JOBS.md의 전체 내용(2026-09-27 ~ 2026-10-25의 단위별 작업 이력과 초기 조사·계획 절)이 원문 그대로 들어 있다.
