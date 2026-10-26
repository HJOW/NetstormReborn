# LEFT_JOBS — cpppj 및 공통 인수인계

이 문서는 cpppj와 공통 사항의 **현재 상태, 앞으로 할 작업, 유효한 규칙**만 담는다. 2026-10-26에 정리했으며, 그 전까지의 단위별 작업 이력(단위444까지)은 [LEFT_JOBS.old.md](LEFT_JOBS.old.md)에 원문 그대로 옮겼다. dotnetpj는 [LEFT_JOBS.dotnetpj.md](LEFT_JOBS.dotnetpj.md), webpj는 [LEFT_JOBS.webpj.md](LEFT_JOBS.webpj.md)를 본다.

## 0. 재개 전 확인 사항

- 작업 전에 [AGENTS.md](AGENTS.md), 이 문서, [LEFT_JOBS.dotnetpj.md](LEFT_JOBS.dotnetpj.md)를 읽는다.
- **마지막 전체 정밀 디컴파일: HJOW-Athlon (192.168.0.94) / 2026-10-23.** 결과는 `extracted/refined/`(Git 제외)에 있다. 현재 PC가 이와 다르면 AGENTS.md 규칙에 따라 재디컴파일한 뒤 작업한다. 절차는 `tools\ghidra\refine_all.ps1`이며, 힌트 생성 단계가 콘솔 인코딩(cp949) 오류로 멈추면 `PYTHONUTF8=1`, `PYTHONIOENCODING=utf-8`을 설정하고 다시 실행한다.
- 2026-10-26의 문서 정리는 HJOW-X3D에서 인수인계 문서만 고친 것이다. 디컴파일 결과·제품 코드·검사 코드는 바꾸지 않았으므로 위 디컴파일 기록은 그대로 유효하다. 사용자는 HJOW-Athlon에서 작업을 재개할 예정이다.
- **실제 게임 실행 확인:** HJOW-Athlon, VM-W11-Codex, vm-debian-codex는 AGENTS.md의 예외 시스템이다. 그 밖의 PC에서는 게임이나 별도 창을 띄우기 전에 목적을 설명하고 사용자 확인을 받는다.
- **목표 상태:** 2026-10-26 재개 뒤466까지 완료했다. 최종 Source2 게임SHA `c8739bd5…`, build1·2 경고/오류0, 집중103고유/112관측·신규9개·전체C++3,165개·실패0이다. 같은 제품 GUI8실행(후행 경로 Edit/Cracked·Normal·Hard/종료질문 No/Credits18쪽/메인 Quit 재실행/미션 메뉴 복귀·재진입), 원본 타이머1 자연 Early2→Tutorial3를 통과했다. SaveGoMain와 자연 Native State3, 새 맵·Add Island GUI·Save as·Test Battle·시나리오·저장상태 복구·나머지 미션 완주/동일성이 남아 전체 목표는 계속 진행 중이다.
- **원시 증거 위치:** 빌드·검사·GUI 원시 로그와 통합 드라이버는 `extracted/`(Git 제외)에 있으며, 작업한 PC(주로 HJOW-Athlon)에만 있다. 커밋되는 결과 보고서는 `cpppj/*.json`, 작업 문서는 `docs/exe/cpp-*.md`다.
- **466 재개점:** 다음은 Editor의 SaveGoMain(020000cb,event.y) Yes/No→현재 전체 저장/next3→Native State3의 cleanup/prefix/메뉴 설치/공통 확정/다음 Poll4다. next3 예약만으로 성공 처리하지 않는다. Save as의 실제 텍스트 입력/확정/작업본 장치도 기존 부품을 재사용해 구현할 수 있다. 근거는 [SaveGoMain](docs/exe/cpp-editor-save-go-main-next-20261026.md), [Save as](docs/exe/cpp-editor-save-as-next-20261026.md), [로컬 AI 옵션](docs/exe/cpp-test-local-ai-integration-next-20261026.md)이다. 기존 d/!state.dat가 있으면 open 이후가 미연결이며 기본 옵션을 꺼서 우회하지 않는다. 마지막 전체 디컴파일 HJOW-Athlon/2026-10-23 유지, 디컴파일 변경 없음.
- **465 완료:** `EditorWorkingRoot`로 실제 자산 디렉터리를 먼저 정규화하여 후행 `/`·`\\`·`.`·`d/..`에서도 같은 형제 `<이름>-editor`를 사용한다. 기존 작업본 보존/원본 보호 검사는 유지한다. Release build1 종료0·EditorFiles 집중7개(신규2)·실패0. 실제 후행 구분자 Edit GUI는 다음 기능 묶음에서 검증한다. [작업 문서](docs/exe/cpp-editor-working-root-20261026.md), [보고서](cpppj/editor-working-root-20261026.json). 마지막 전체 디컴파일 HJOW-Athlon/2026-10-23 유지, 디컴파일 변경 없음. Edit 도구/상태 복구·전체 미션 완주 목표는 계속 진행 중이다.
- **466 완료:** 같은 실제 풀/게임 난수/표시의 Set All Bridge, 양판본 CanPlaceIsland, 늦은 Guide 해제와 Tutorial 데이터 수명, 실제 메인 Quit 큐/즉시 반환, Script 없는 Editor의 HTML 취소/Blank 복귀를 연결했다. Release 경고/오류0·집중103고유/112관측·신규9개·전체C++3,165개·실패0, 같은 게임 `c8739bd5…`의 GUI8·자연Early2→Tutorial3를 확인했다. 기존 검사3파일 본문·기존3156등록·보호2474파일을 보존했다. 최초 필터명2오류와 stdout/stderr 이름3개 혼합은 검사기에서만 보완하고 원시를 보존했다. [통합 문서](docs/exe/cpp-editor-runtime-integration-20261026.md), [보고서](cpppj/editor-runtime-integration-20261026.json), 원시 extracted/cpp-editor-runtime-466-*. SaveGoMain/전체State3·Editor 도구 전체·나머지 미션 자연 완주/동일성은 후속이다. 마지막 전체 디컴파일 HJOW-Athlon/2026-10-23 유지.
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

- **최신466 통합:** Source2 게임 `c8739bd5…`, 빌드2회 경고/오류0·집중103고유/112관측·신규9·전체C++3,165개·실패0. 같은 제품 GUI8과 자연Early2→Tutorial3를 통과했다. 기존3156등록/검사3본문/보호2474파일을 보존했다. 자세한 범위와 최초 검사기 오류는 [통합 문서](docs/exe/cpp-editor-runtime-integration-20261026.md), [보고서](cpppj/editor-runtime-integration-20261026.json)를 본다. SaveGoMain/전체State3·Editor 전체·전체 자연 완주 완료는 아니다.

- **최신 실제 Editor 진입(464):** build9 경고/오류0, 집중298고유/309관측·신규6개·전체C++ **3,154개·실패0**. 같은 게임SHA `661a19c1…`의 실제 Battle1/TEST01 메뉴 클릭→State4·현재 이름·공개 Runtime·실제 HUD·양끝·ESC/Game/Edit/닫기 GUI2건을 확인했다. 명시 test-state 호출은 사용하지 않았다. 기존3148등록 이름/검사3파일의 본문·보호1820/fixture654파일을 보존했다. UI scope의 선택 공장 차단을 풀었던 시도는 기존 검사2실패 후 폐기하고 기존 차단/본문을 유지했다. [464 문서](docs/exe/cpp-existing-editor-entry-20261026.md), [보고서](cpppj/existing-editor-entry-20261026.json), 원시 extracted/cpp-editor-entry-464-*. 새 맵·도구·저장·시나리오·전체 자연 완주 완료는 아니다.
- **이전 전체 정리 통합 제품(463):** build6 경고/오류0, 집중252고유/261관측·신규5개·전체C++ **3,148개·실패0**다. 같은 게임SHA`7f4517d2…`의 초기 전체 정리 두 번(실제 Player와 HUD 교환) 및 Campaign→Early1 MORE/OK 뒤 연결표2→0·World/Runtime 반납·새 HUD GUI2건을 확인했다. 이전459의3130개 실제 등록 이름도 모두 유지했다. CTest wrapper는1개이며 내부 사례 수와 구별한다. 기존 생산 검사 본문과 명시 보호1820/fixture654파일을 보존했다. 최초 컴파일/신규 검사/GUI/전체 이름 파서 실패는 원시 보존했다. 자연 Edit 설치·34경로·긴 완주 검증으로 합산하지 않는다. [463 보고서](cpppj/whole-state-runtime-20261026.json), [463 문서](docs/exe/cpp-whole-state-runtime-20261026.md).
- **이전 일반 메뉴 통합 제품(459):** Source4(2,056소스), 게임SHA`7a8e5387…`, build4 경고/오류0, 집중263개·신규2개·전체CTest **3,130개·실패0**다. 같은 제품의 실제 Early1 ESC 및 메인 메뉴 복귀·재진입 GUI2가 통과했고 현재 연결표 반납·새 토큰을 확인했다. 기존 검사 이름3128개와653자료(TSV650개), 보호1821파일을 유지했다. 이번에는 긴 자연 완주/34경로를 반복하지 않았다. [459 보고서](cpppj/mission-background-runtime-20261026.json), [459 문서](docs/exe/cpp-mission-background-runtime-20261026.md).
- **이전 통합·자연 완주 제품(454):** Source2(2,039소스), 게임SHA`8624fefb…`, build2 경고/오류0, 집중99고유/99관측·실패0(신규5개), 같은 제품 전체CTest **3,096개·실패0**·메뉴GUI2·자연Early2→Tutorial3 통과. 이전450의3,076개와451~453 신규15개·이번5개 실제 이름 집합을 모두 대조했다. [454 보고서](cpppj/state-runtime-retirement-20261026.json), [454 문서](docs/exe/cpp-state-runtime-retirement-20261026.md).
- **이전 통합 검증 제품(450):** Source1(2,038소스 동결), 게임SHA`b5966c23…`, build1 경고/오류0, 집중487고유/489관측·실패0(신규5개), 같은 제품 전체CTest **3076개·실패0**·GUI2·자연Early2→Tutorial3 통과. 447의3,052개와448/449 신규19개·이번5개 실제 이름 집합을 대조했다. [450 보고서](cpppj/squid-pool-reset-runtime-20261026.json), [450 문서](docs/exe/cpp-squid-pool-reset-runtime-20261026.md). 직전447(`97bef859…`)도 전체3,052·GUI2·자연Early2를 통과했다. 아래443·444 기록은 이전 제품이다.
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

- **메인 메뉴:** 원본10.78의8버튼을 배치했다([UberGump.cpp](cpppj/src/client/UberGump.cpp)). Campaign/Demo/Help/Options는 기존 GUI에서 확인했고,466에서 Credits의 Patch4쪽·Original14쪽 실제 버튼/Back/최종OK·메인 복귀와 메인 Quit 종료0/재실행을 확인했다. Credits20초 자동롤·원본 픽셀/음향 대조와 Native State3 전체는 후속이다. Multiplayer는 범위 밖이고 Edit 완성도 남아 있다.
- **Edit 메뉴:**
  - 메인 Edit → 편집 가능 맵 목록(원본 50칸 상한, `mayEditMissions` 옵션) → 선택 예약(State1) → LoadEditable 안내·Cancel까지 실제 GUI로 확인했다.
  - 기존 Fort의 Session/World·Deck·작업본 파일·로컬 전송과 준비 checkpoint는464에서 실제 메뉴 선택→State1→공통 확정→Poll4에 연결했다.466은 기존 Guide의 late closeGuide/모드 해제와 후행 경로를 검증했다. 전체 저장·재파싱·재로드는 기존 집중 검사 근거이며 사용자 Save as/복귀 저장 GUI 완료와 구별한다.
  -466에서 실제 Set All Bridge의 Cracked/Normal/Hard와 종료질문 No 뒤 Editor 복귀를 완료했다. Add Island 필수 하위 CanPlaceIsland도 양판본 독립관찰/새 C++2개를 통과했지만 실제 창은 미연결이다. NewFort, Add Island GUI, Save as, SaveGoMain/전체State3, Test Battle, 언어별 시나리오 적용은 남아 있다.
- **게임 중 UI:**
  - QWASZX: 문자 입력 → 생산 슬롯 예약 → 취득·회전·배치·보충을 실제 GUI로 확인했다(단위366, [문서](docs/exe/cpp-production-shortcuts-20261023.md)).
  - ESC 메뉴: 원본 가로 메뉴(Game·Edit·View·Options·Players·About)와 하위 명령·사건 큐를 연결했다(단위369, [문서](docs/exe/cpp-game-menu-runtime-20261023.md)). 원본에 없던 임시 Game 버튼은 2026-10-25에 제거했다. 459에서 원문 State7의 같은 HTML 배경 첫BYTE 초기화를 연결해 남은 배경이 ESC를 소비하던 회귀를 해결했고 실제 Early1 메뉴 복귀·재진입을 확인했다([459 문서](docs/exe/cpp-mission-background-runtime-20261026.md)).
  - 좌측 덱·미니맵: 실제 HUD 루트·도킹·구분선과 생산·미니맵·NSInfo·Chat 자식을 실제 미션에 연결했다(단위367·368·417·418, [문서](docs/exe/cpp-uber-tree-20261025.md)).
  - 남은 것: 넓은 dock 드래그 실제 검증, 전체 View 명령과 픽셀 동일성, 카메라 전체 Frame/zoom 부모, Chat 입력(F3·전송·클립보드)과 Chat GUI 드래그, 미니맵 색 억제·독립 토글.
- **공통 전략 AI:** 적·아군·데모 로컬 AI를 같은 원본 전략으로 복원하고 `RawCampaignSession::RunFrame`(현재 시각 → 정지하지 않은 AI → Kernel)에 연결했다(단위320~322, [문서](docs/exe/cpp-ai-automatic-session-20261022.md)). 정상 행동 간격에서 확장·채취·생산·공격이 원본과 같은지는 자연 플레이로 대조하지 않았다.
- **주문·의식:** Devastation의 시전 → 서버 → 효과와 사제 포획·운반·제단·다이스·룬·희생·SP·미션 완료를 연결했다. 집중 13검사는 통과했지만 일부 입력은 명시한 시전원·포로·시계 값이었다([문서](docs/exe/cpp-ritual-runtime-focused-20261021.md)). 은신·마비(단위349)와 윈드 소환 세 주문(단위350)도 복원했다. 실제 UI에서 취득 → 시전 → 포획 → 희생으로 이어지는 자연 플레이는 검증하지 않았다.
- **유닛·건물:** 워크샾 25종(썬 6·레인 7·윈드 6·썬더 6)의 생성·이동·공격·표시·소유자 색을 복원했다(단위335~345, [목록](docs/exe/cpp-unit-restoration-20261022.md)). 캠페인 2·3장에서 추가로 쓰이는 TYPE의 전체 수명은 점검하지 않았다.
- **내부 상태 수명(단위425~451):** Client와 Session이 공유하는 원본 장기 전역(카메라64BYTE, 상태 보조, 게임 보조18필드, 프로세스Host, Graph, TotalMade, 섬·청크·사제, 선택·권한·FullDeck, 연결 활동 표, 커서30DWORD·Lifecycle/Placement/snap)을 실제 소유자로 옮겼다. AI reset·Player 링크 정상 복귀의 이동 증빙과 중복 정리 방지, 사제4leaf·공통Carrier/생산·Damageable finder 및 피격·건설·삭제·채집10State의 현재 Client boss/server 조회 및 실제 Player 신전 SID에 따른 HP 회복을 통합했다. 448~450에서 실제 State 직접 전역, ClockSync·Zone·MetaZone·마커, 전체 Pool 부모·삭제 기록·공유 실패점을 추가 연결했다. 451에서 전체 Deck 명령 부모와 실제 GetFullDeck 입력도 같은 소유자에 연결했다. 계획은 [월드 장기 소유 계획](docs/exe/cpp-state-world-owner-plan-20261025.md)에 있다. 462~463에서 실제 반납 조정기와 전체 cleanup API/명시 UI 호출을 연결했다. 464에서 기존 맵 Edit의 자연 State1 호출/prefix를 연결했다. 다른 State1 분기와 State7은 남았다.
- **배포:** [cpppj/package-cpppj.ps1](cpppj/package-cpppj.ps1)을 만들었다(단위359). 자동 빌드 분기(`-NoBuild` 없이 실행)는 아직 실제로 돌려 보지 않았다.

### 2.4 알려진 미해결 이슈

- **Early1 남동 경로 실패:** 남동 간헐천 경로 입력은 D 단계·SP 0에서 Next Tutorial을 찾지 못하고 종료 1이 된다(단위417). 북서 경로는 통과한다. 원인은 확정하지 않았다.
- **options.cfg 저장·복구 실패:** 1280×720 GUI 검사의 음량 변경 구간에서 SaveOptions의 fopen이 errno 22 / doserrno 1224로 실패하거나(단위393), 검사 도구의 설정 열기·복구에서 OSError22가 간헐적으로 난다(단위369·386). 격리한 데이터 복사본 경로에서는 통과했다. 원본 폴더가 있는 H 볼륨은 exFAT다. 원인은 확정하지 않았다.
- **오래된 스모크 6개:** `cpp_menu_smoke`, `cpp_rawbuttons_smoke`, `cpp_thunder_smoke`, `cpp_weather_smoke`, `cpp_audio_smoke`, `check_dialog_controls.check_mission`은 제거된 임시 Game 버튼을 전제로 한다. 다시 쓸 때 ESC → 실제 메뉴 입력으로 바꾼다.
- **PlayerNoMake 경고 표:** 453에서 같은 ClientLocalStateGlobals의255DWORD와 실제 정리004328a0을 연결했다. 패킷 수신 부모00433c30의 실제 writer는 후속이다. CD 자동 대응00487900은 실제 spot 정리이므로 같은 함수로 취급하지 않는다.
- **README 갱신 완료:** 별도 문서 커밋 `1eddb0e8 / 20261026-07 CM`에서 국·영 README를 현재 범위와 세 프로젝트의 플레이/동일성 구분에 맞게 갱신했다.

## 3. 앞으로 할 작업 (권장 순서)

### 0. 2026-10-26 재개 — 466까지 완료

- **452 기준 제품:** Source6(2,039소스), `NetstormCpp.exe` SHA `471ec293…`, Release build6 종료0·경고/오류0, 집중444고유/444관측·실패0(기존 신규6개 전부)이다. 실제 복사본 GUI에서 QWASZX 여섯 슬롯·회전/반대 회전·합법 위치 배치(새 서버 다리15211)·보충을 확인했다. 원본/CD 등 보호1,821파일은 동일하다.
- **복구:** 두 신규 fixture 조회에 `<filesystem>`과 `LoadFixture(const char*)`의 문자열 인자를 명시했다. Source5에서 드러난 기존 `CampaignObjectMenuTests`의 Sid 접근형4진단도 같은 숫자의 DWORD 비교/실제 소비 직전 checked SID변환으로 고쳤다. 신규 바이트 채움의 C4244 두 원인도 `uint8_t{}`로 교정했다. 기존 단언·Native fixture·신규6개 검사 이름은 유지했다.
- **최초 실패 보존:** build1~4는 중단 전 실패, build5는 위 후속4오류·C4244 두 원인(컴파일러 반복 출력14행), build6은 성공이다. 동결 manifest의 검사 기준은 최초 `1ecdfa2a`이며 실제 재개 HEAD `46e65408`을 별도 기록했다.
- **검증 범위:** GUI는 `tools/cpp_production_shortcuts_smoke.py --data-root`에 실제 파일 복사본을 전달했다. `useProductTimers=0`의 기존 입력이며 원본 타이머 자연 완주·원본 픽셀/음향 동일성의 완료 증거가 아니다. 전체 CTest는 이번에 반복하지 않았다. 마지막 전체 통합은450의3,076개다.
- [452 작업 문서](docs/exe/cpp-cursor-preview-width-runtime-20261026.md), [실행 보고서](cpppj/cursor-preview-width-runtime-20261026.json), 원시 `extracted/cpp-cursor-preview-width-452-{source6,build5,build6,focused1,gui1}`를 본다. 디컴파일 산출물을 변경하지 않았으며 마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-10-23**이다.
- **최신453:** Source1(2,039소스) build1 종료0·경고/오류0, 집중104고유/108관측·실패0(신규4개·편집 설치50개 포함)이다. 같은 Client의 집계·Hash·경고255DWORD·spot65536BYTE·Island·Chunk·Graph 일곱 Effect 하위를 연결했다. [453 문서](docs/exe/cpp-state-flat-storage-runtime-20261026.md), [실행 보고서](cpppj/state-flat-storage-runtime-20261026.json). 전체 CTest·GUI는 이번에 반복하지 않았다.
- **454 완료:** 실제 AI1~8·Player 링크·Kernel 현재 처리기의 반환 증빙을 모두 소비한 경우에만 일반 Session 소멸의 게임 전역 재초기화를 생략한다. 고정뷰·표면 대여는 전체 소비자 소멸 뒤에 반납하고 실제 Regular 잔류는 장기 Host에 남긴다. Source2 build2 경고/오류0, 집중99·전체CTest3,096·실패0, GUI2·자연Early2 통과. 최초 집중의3실패는 장기 Kernel 조립2개와 Native known0 store 누락1개로, 기존 관문·단언·Native fixture를 유지해 교정했다. [454 문서](docs/exe/cpp-state-runtime-retirement-20261026.md), [실행 보고서](cpppj/state-runtime-retirement-20261026.json).
- **455 완료:** SID8목록과 선택 count의 장기 수명·원문16/50/20/30용량·등록20포인터 개수 정리 및 ReloadTheams 신전 연결을 복원했다. Source4 빌드 경고/오류0, 집중320·전체C++3108·GUI2 통과. [455 문서](docs/exe/cpp-state-sid-lists-runtime-20261026.md), [실행 보고서](cpppj/state-sid-lists-runtime-20261026.json).
- **456 완료:** SP 표시의 실제 현재 serial 재부착과 World 이전 HUD 생성/정리/모드·공통 Draw를 연결했다. Source7 build8 경고/오류0, 집중283·전체C++3114·GUI2 통과. [456 문서](docs/exe/cpp-state-hud-runtime-20261026.md), [실행 보고서](cpppj/state-hud-runtime-20261026.json).
- **최신457 완료:** 실제 연결 정리6몸체580관측과 기존 Editor의 원문 지연 종료를 연결했다. Source4 build4 경고/오류0·집중230·신규6개 통과. 장기22/22/32표의 실제 생성·재배치 writer 연결은 다음 단계다. [457 문서](docs/exe/cpp-connection-cleanup-runtime-20261026.md), [실행 보고서](cpppj/connection-cleanup-runtime-20261026.json).
- **다음:** Native State1의 작은 잔여 leaf·전체 Effect/prefix·Runtime/Player 반납 coordinator와 실제 Edit 설치를 이어서 복원한다. 3절2~7의 플레이·메뉴·동일성 목표도 계속 남는다.

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
- [x] **452 완료:** 단일 CursorDropState.preview/Content 현재 커서의 DWORD 보존과 실제 SID16 소비 경계를 복구했다. Source6 빌드 경고/오류0, 집중444개·신규6개·실패0, 복사본 실제 QWASZX·회전·Drop·보충 GUI 통과. [452 문서](docs/exe/cpp-cursor-preview-width-runtime-20261026.md), [실행 보고서](cpppj/cursor-preview-width-runtime-20261026.json). 전체 State1/Edit·전체 자연 완주 완료는 아니다.
- [x] **453 완료:** 실제 PATCH CPU16관측을 고정하고 PlayerNoMake255DWORD/spot65536BYTE의 두 작은 정리와 같은 장기 물리7개 Effect 하위를 연결했다. Source1 빌드 경고/오류0, 집중104고유/108관측·신규4개·실패0, 보호1,821파일 동일. 자연 전체 cleanup/Edit 진입은 미완료다. [453 문서](docs/exe/cpp-state-flat-storage-runtime-20261026.md).
- [x] **454 완료:** 같은 실제 자식 반환 증빙 전체를 조건으로 Session/Gameplay 대여를 보존 반납한다. 기존 일반 종료·부분 증빙은 종전 정책을 유지하며 실제 Player/표면 고정뷰는 전체 모듈 소멸 뒤에만 해제된다. 전체CTest3,096·GUI2·자연Early2까지 통합했다. 전체 Native 부모·자연 Edit 설치 완료는 아니다.
- [x] **455 완료:** 실제 정적 SID8목록을 Client 표면 owner에 연결하고 Client 선택30칸/count와 같은 등록20포인터로 정리한다. 원문 용량은 factories500/dude30/outposts100/providers256/geysers100/priests16/residences50/selection30/temples20이며 count만0으로 쓰고 포인터/데이터/용량을 보존한다. ReloadTheams의 두 번째 목록도 원문005e1898 신전으로 바로잡았다. Native66관측, Source4 build4 경고/오류0, 집중320고유/324관측·신규12개·실패0, 전체C++3108·실패0, 같은 게임`e10efe65…`의 복사본 Early1 및 QWASZX GUI2를 확인했다. 최초 집중의 신규3사례와 첫 전체의 생산2사례/89단언 실패는 동일 제품의 fixture 입력 조립으로 교정했고 Native 기대·기존 숫자 단언·원시 실패를 보존했다. 기존 fixture649개와 원본/CD 보호1821파일도 유지했다. 마지막 전체 디컴파일은 HJOW-Athlon/2026-10-23 그대로다. [455 문서](docs/exe/cpp-state-sid-lists-runtime-20261026.md), [실행 보고서](cpppj/state-sid-lists-runtime-20261026.json), 원시`extracted/cpp-state-sid-lists-455-*`를 본다. 전체 State1/Edit·전체 미션 자연 완주 완료는 아니다.
- [x] **456 완료:** 실제 SP 부모0043dad0/serial조회00464020의120관측(중단48)을 고정하고 현재 예산의 Push/현재좌표 Attach/정상 깊이 복구를 연결했다. 초기 HUD 네 공장을 World 없이 구성하고 실제 미니맵 통지는 현재 토큰의 첫 World에서만 빌린다. Native Kill/Cleanup·mode3/4도 같은 Gump 표/입력에 연결하며 실제 정리 뒤 해제된 호스트 버튼 별칭을 반납한다. Source7(2045소스) build8 경고/오류0, 집중283·신규6·전체C++3114·실패0, 같은 게임`17d6201e…`의 World 이전 HUD 생성/교환/SP/mode4 및 Early1 MORE/OK/Home GUI2를 확인했다. 일반/편집 좌측50,022픽셀 차이와 최종 화면을 직접 검토했다. build2 컴파일5오류·build4 manifest 사전 경합·GUI1 중간 기하 bitmap 범위·GUI2 삭제 버튼 별칭 실패를 보존했으며 Native 기대/기존 단언650자료·원본/CD 등 보호1821파일은 유지했다. 마지막 전체 디컴파일은 HJOW-Athlon/2026-10-23 그대로다. [456 문서](docs/exe/cpp-state-hud-runtime-20261026.md), [실행 보고서](cpppj/state-hud-runtime-20261026.json), 원시`extracted/cpp-state-hud-456-*`를 본다. 전체 State1/Edit·미션 자연 완주 완료는 아니다.
- [x] **457 완료:** 원문 연결표/pending/deferred 여섯 실제 PE580관측(중단268)을 고정하고 현재22/22/32표·동기화·개수와 같은 소멸/free 순서를 복원했다. 기존 Editor 명시 종료도 같은004df440을 쓰며 -32 이하의 원문 자연 반환을 유지한다. Source4(2049소스) build4 경고/오류0·집중230고유/관측·신규6개·실패0, 기존 fixture651개·원본/CD 등 보호1821파일을 보존했다. build1 Retire 경계 제거의 C3861 한 건과 첫 집중의 새 Client fixture 자료 경로 누락1실패를 원시 보존했고 Native/기존 단언을 유지해 교정했다. Source2 이후 제품 본문/게임`1eaab3e9…`는 같다. 장기표의 실제 생성/수락/재배치/해제 writer는 미연결이므로 빈 표를 기존 Session 연결 부재나 전체 cleanup 성공으로 판단하지 않는다. 이번 GUI/전체CTest는 반복하지 않았다. 마지막 전체 정밀 디컴파일 HJOW-Athlon/2026-10-23 그대로다. [457 문서](docs/exe/cpp-connection-cleanup-runtime-20261026.md), [실행 보고서](cpppj/connection-cleanup-runtime-20261026.json), 원시`extracted/cpp-connection-cleanup-457-*`. State1/Edit·미션 자연 완주 완료는 아니다.
- [x] **458 완료:** 연결 등록/재배치 세 실제 PE288관측(중단60)을 제품 전에 고정했다. 같은 실제 Node/close/free와 Client 장기 표를 Editor·일반 로컬 전송의 생성/수락/재배치/분배/해제 writer에 연결했다. 실제 미션 서비스도 같은 Registry를 제공하고 다른 Editor 표는 설치 전에 거부한다. Source6(2055소스) build6 경고/오류0·집중268고유/269관측·신규8·실패0, 같은 게임`20c4aca2…`의 복사본 Early1 메뉴→MORE/OK에서 현재 연결2/pending0/count0을 확인했다. 기존 검사 본문·652자료(TSV649)·원본/CD 등 보호1821파일을 보존했다. 처음 두 빌드 오류와 첫 집중의 외부 Editor 소유자 오류2건, 초기 집중1/2 단위 라벨 교정 전 JSON을 원시 보존했다. ESC 막대가 열리지 않아 복귀·재진입 계획은 실행 전 두 번 실패했으며, 네 GUI의 진입 성공을 메뉴 성공으로 확대하지 않는다. 불필요한 검사 Escape 가상 키 추가도 최종 제품에서 제거했다. 기존 리슨/수락 시점·TCP/SPX/전체 소켓 부모·별도22칸 송신버퍼 writer는 미복원이고 Native State1/Edit 설치·자연 완주도 미완료다. 전체CTest는 반복하지 않았다. 마지막 전체 정밀 디컴파일 HJOW-Athlon/2026-10-23 그대로다. [458 문서](docs/exe/cpp-connection-registration-runtime-20261026.md), [실행 보고서](cpppj/connection-registration-runtime-20261026.json), 원시`extracted/cpp-connection-registration-458-*`.
- [x] **기존 맵 Edit State1 접합:**464에서 자연 next1/cleanup/prefix/checkpoint/tail/공통 확정/Poll4를 완료했다.466에서 늦은 Guide 해제/표시 복귀/전체 품질 입력도 연결했다. 다음은 SaveGoMain/전체 Native State3와 새 맵·실제 도구·저장·Test Battle이며, 이전 하위 준비본의 완료를 전체 Editor 완료로 합산하지 않는다.
- [ ] 436의 남은 기존 SID16 변환, 438의 전체 Deck nullable+ac와 Native cleanup 접합, 443의 단일 Drop.preview SID16 경계는452에서 완료했다. 나머지 SID16/nullable 경계를 이어서 처리한다. 전체 Deck 명령 부모와 UserInput writer는451에서 완료했다.

- [x] **459 완료:** 실제 배경244가 ESC를 소비하던 원인을 Source1 임시 진단으로 확인했다. 원문 State7의565e70 첫BYTE0을 Tell/Help/HUD와 같은 ClientHtmlWindowGlobals에 연결했으며 나머지79BYTE/P 억제를 보존한다. 전체 State7 cleanup은 여전히 후속이다. Source4(2056소스) build4 경고/오류0·집중263·신규2·전체CTest3130·실패0, 같은 게임7a8e5387의 실제 Early1 ESC 및 Game→Main Menu→Early1 재진입 GUI2가 통과했다. 현재 연결2→0→2, client9 토큰1828716545→1828716547을 확인하고 세 최종 화면을 직접 검토했다. 기존 검사 본문·653자료(TSV650)·원본/CD 등 보호1821파일은 보존했다. 첫 집중의 새 반복 입력은 ClearWorld 없이 CaptureEnd를 재사용해 실패했고 독립 입력 전제조건을 교정했다. 배경/79BYTE/P 단언을 유지하고 같은 Client의 실제 정리는 GUI로 확인했다. 임시 진단은 최종 소스에서 제거했다. 마지막 정밀 디컴파일 HJOW-Athlon/2026-10-23 그대로다. [459 문서](docs/exe/cpp-mission-background-runtime-20261026.md), [실행 보고서](cpppj/mission-background-runtime-20261026.json), 원시 `extracted/cpp-escape-routing-459-*`. State1/Edit·전체 자연 완주·픽셀/음향 동일성 완료는 아니다.

- [x] **460 완료:** 실제 PE0043b540/00422490의64관측을 제품 전에 고정했다(하위 치환 없음). 같은 links.ownerColors의 앞9DWORD0..8과 같은 Storage의 다리 스캔 범위/커서/현재float 지연·double 기한을 Native Effect에 연결했다. 색 표는 원문 위치상 World 반납 앞에서도 허용하고 스캔은 반납 뒤만 허용한다. 실제 초기 first/last는 PE의-1로 교정했다. Source3(2059소스) build3 경고/오류0·집중222·신규5·실패0, 게임01324a3b이다. 기존 검사 본문·653자료(TSV650)·원본/CD 등 보호1821파일을 보존했다. 첫 집중의 새 두 Client 입력은 단일 Client 제약으로 실패했고 독립 표면 owner를 넘겨 같은 거부/미변경을 검증하도록 교정했다. Native 기대/기존 단언은 유지했다. 전체CTest/GUI는 반복하지 않았고 마지막 전체는459의3130개다. 전체 State1 조립·실제 Edit 설치·원문 Frame의 다리 스캔 호출은 아직 미완료다. 마지막 전체 정밀 디컴파일 HJOW-Athlon/2026-10-23 그대로다. [460 문서](docs/exe/cpp-state-bridge-reset-runtime-20261026.md), [실행 보고서](cpppj/state-bridge-reset-runtime-20261026.json), 원시 `extracted/cpp-state-bridge-reset-460-*`.

- [x] **461 완료:** 원문00469fc0→004558f0(0,4000)의 전투 음악도 공통00532710을 소비하므로, 기존 timeGetTime 별도 시드 대신 실제 ClientRandomGlobals::Game()을 일반/noAudio 음향에 연결했다. 독립 정수 시드 생성자는 유지하고 생성/종료/구성 실패의 공통 상태 쓰기를 추가하지 않았다. 기존 Native6시드·실제 SP 교차 소비·실제 Client 초기화/반납 등 신규4개 포함 집중41개·실패0, Release build2 종료0·경고/오류0이다. 게임SHA는dde9d6dc이며 기존 검사 본문/fixture654파일과 이번 명시 보호범위(originals/CD/AGENTS)1820파일은 검사 전후 같다. 과거1821파일 manifest와 같은 집합으로 간주하지 않는다. 전체CTest/GUI는 반복하지 않았으며 공통 난수 소비가 바뀐 다음 제품의 통합 검증은 후속이다. 마지막 전체 정밀 디컴파일 HJOW-Athlon/2026-10-23은 그대로다. [461 문서](docs/exe/cpp-scene-random-runtime-20261026.md), [실행 보고서](cpppj/scene-random-runtime-20261026.json), 원시 `extracted/cpp-scene-random-461-*`. 전체 State1/Edit·전체 미션 완주·화면/음향 동일성 완료는 아니다.

- [x] **462 완료:** `ClientStateRuntimeRetirement`가 실제 AI owner1..8·9Deck·Kernel Clear 증빙을 현재 Client/Session에 연결한다. ClearKernel 뒤에도 UI 정리/교체가 현재 처리기를 쓸 수 있게 보관하고 실제 known0/Core0 뒤 모든 증빙을 확인하여 처리기→외부 Session 소유자→고정뷰를 반납한다. 첫 메뉴도 실제 brain/Player 링크/Kernel을 조회·정리하며 비영 자식의 장치 부재는 도달 위치에서 거부한다. 실제 반납 없는 callback·다른 소유자/Kernel·이전 대여 세대를 거부하고 처리기 반납 뒤의 실제 소유자 반환은 재시도 가능하다. Release build1 경고/오류0·신규4개 포함 집중197고유/202관측·실패0, 게임3f570713이다. 기존 검사 본문과461의 보호1820/fixture654 집합은 유지했다. 전체CTest/GUI는 반복하지 않았다. 마지막 정밀 디컴파일 HJOW-Athlon/2026-10-23 그대로다. [462 문서](docs/exe/cpp-state-runtime-coordinator-20261026.md), [실행 보고서](cpppj/state-runtime-coordinator-20261026.json), 원시 `extracted/cpp-state-runtime-coordinator-462-*`. 다음 전체 부모는 원문 Core0와 두 직접 store 뒤 resetPool의 실제 Pool reset 앞에서 이 조정기를 호출해야 한다. `UberGump::ClearWorld()`는 기한/미션 정지/전역 쓰기가 섞여 있으므로 실제 반납 callback으로 그대로 호출하지 않고 참조 해제와 중복 게임 쓰기를 분리한다. 전체 State1/Edit 설치·자연 완주는 아직 미완료다.

- [x] **463 완료:** `CleanupClientStateWorld`가 기존004b2df0/전체 직접 전역/34Effect 선택을 같은 Client와 실제 기존 하위·462 반납 조정기에 연결한다. `UberGump::CleanupNativeStateWorld`와 별도 호스트 참조 반납을 제공하며 일반 ClearWorld의 전역/기한/정지 쓰기를 반복하지 않는다. 실제 초기 GUI에서 드러난 예산 Idle의 반납된 Session 참조는 원문0043e530처럼 같은 장기 SP owner를 빌려 해결했다. 마지막 실제 Resume에서는 호스트 UI 정지 표식도 반납해 다음 모달이 다시 정지한다. 기존 FortMoney 지정 본문은 현재 getter를 받는 공통 함수로 재사용했다. build6 경고/오류0·신규5개 포함 집중252고유/261관측·전체C++3148개·실패0, 같은 게임7f4517d2의 명시 GUI2건(초기 연속 정리·실제 Early1 반납)이 통과했다. 이전3130이름과 기존 생산 검사 본문·보호1820/fixture654집합도 보존했다. 전체1의3147성공과 stderr 혼입 이름 파서의 잘못된5누락 진단은 원시 보존했고, 실제 --list·무필터 실행 요약·이전 소스 등록 이름으로 대조하도록 보완했다. 첫 build1 콜백 전달 오류·새 색 표 입력의 범위 오류·첫 GUI의 SP 공백도 보존했다. 마지막 디컴파일 HJOW-Athlon/2026-10-23 그대로다. [463 문서](docs/exe/cpp-whole-state-runtime-20261026.md), [보고서](cpppj/whole-state-runtime-20261026.json), 원시 `extracted/cpp-whole-state-463-*`. **이후464에서 기존 맵의 자연 Edit next1/prefix/공통 확정을 연결했다.** 463의 test-state-world 자체는 명시 검사 호출이며 전체 미션 완주·화면/음향 동일성 완료가 아니다. 실제 metamap과 비mission phase14 저장은 선택 장치를 요구하되 Uber에서는 아직 미복원이다.

- [x] **464 완료:** 실제 기존 맵 Edit의 State1 부모를 같은 장기 Client 전역/정리/준비 Session/파일/UI/공통 확정과 연결하고, 공통 ret 뒤에만 호스트 선택·운반체 입력을 연결했다. 실제 listener/client9/accepted -1과 같은 Player/연결표를 사용하며 원문 send/receive를 동기 성공으로 대신하지 않는다. 기본 startupStateCheck는 실제 파일 부재 reader/벽시각을 사용하며 존재하는 저장 파일의 복구는 미연결이다. SID0의 원문 Finder 허용, 같은00542640 현재 마스크, Player/Bridge 모드 별칭, Editor 제목/ESC 메뉴도 연결했다. build9 경고/오류0·신규6개 포함 집중298고유/309관측·전체3154개·실패0과 같은 게임661a19c1의 자연 GUI2건이 통과했다. 첫 실패/새 검사 입력 오류/기존 차단 검사 실패/전체3의2CHECK 실패를 삭제하지 않았다. [464 문서](docs/exe/cpp-existing-editor-entry-20261026.md), [보고서](cpppj/existing-editor-entry-20261026.json). 마지막 디컴파일 HJOW-Athlon/2026-10-23 유지. **다음은 편집 도구/새 맵/저장/Test Battle/메인 메뉴·재진입과 기존 저장 상태 복구**다.

### 2. Edit 메뉴 완성


- [x] **464 완료 — 기존 맵:** 실제 목록 클릭→Native State1 cleanup/prefix→현재 이름·작업본→InstallExistingEditorCheckpoint→HUD mode1/전체 팔레트→공통 확정/loaded→호스트 입력→다음 Poll4를 연결했다. 기존 InstallHud/공통 HUD를 재사용하고 실제 listener/client9/동일 연결표를 사용한다. 실제 Battle1과 TEST01의 ESC/Game/Edit 표시·ESC 닫기까지 통과했다. TEST01 GUI 입력은 새 복사본의 loose Fort 한 개로 제한했으며 전체 원본 폴더 목록에서 TEST01 접근을 확인한 것은 아니다.
- [ ] 편집 도구(지형·섬·오브젝트·소유자)와 ESC의 Editor 명령(Save as, NewFort, 다리 수준0/1/2, Test Battle, Main Menu)을 연결한다. 464에서 메뉴 표시/열기/닫기는 완료했으며 실제 명령 동작은 별도다.
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

- [ ] 다른 State1 분기/State7의 자연 Tick 정리와 전환을 연결한다. 기존 맵 Edit의 State1 cleanup/prefix/확정은464에서 완료했다. 캠페인 이탈→Edit 전환의 mission/모드 수명, 기존 저장 상태 복구, 일반 전투 saveBattle·metamap·다른 파생 프로세스 수명은 남아 있다.
- [ ] 460의 다리 스캔 장기 저장소/reset은 완료했으나 원문 Frame의 BridgeDecayScan 순회 호출은 아직 미연결이다. 기존 계산/Native 대조 통과를 실제 프레임 실행 완료로 합산하지 않는다.
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
- [ ] **Credits:**466의 같은 제품에서 Patch4쪽/Original14쪽의 실제 More/Back/Cancel/최종OK와 메인 복귀·Quit을 확인했다. 원문 nsstart 배경 선택과20초 자동롤·픽셀/음향 동일성은 별도 대조가 남는다. [메뉴 문서](docs/exe/cpp-main-quit-credits-20261026.md)를 본다.
- [x] **메인 Quit:**466에서 RequestQuitApp→원문 모드 관문→QuitApp99→종료음→즉시 반환을 연결했다. 실제 두 실행의 종료0/재실행과 후속200프레임 사건 미실행을 확인했다. Editor 종료질문 No 복귀도 확인했지만 SaveGoMain/전체State3·Editor 저장 후 종료와 원본 음향 동일성은 별도다.
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
