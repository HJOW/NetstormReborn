# LEFT_JOBS.dotnetpj — 현행 상태와 남은 작업

**2026-11-26 · 목표 진행 중 · 마지막 완료 1.060(SL-7 잠금·전체 회귀) · 다음 SL-8 파일 계열 소비자 → SL-9 → R5 월드 화면(입력·음향·포획·회피·건설 도착·0x6c backend·live Path 틱은 R5/R6/R11) · 현재 단계 R4(R4-2·R4-4·R4-5·R4-6, 2/8) · 마지막 전체 회귀 1.060(7,307통과·M1 0/34)·다음 1.064 · cpppj 제품1339 / 11.01 · 전체 목표 미완료 · 1.060 뒤 정리**

현행 인계는 이 문서가 기준이다. 단위별 근거는 `docs/dotnet-canon-0NNN-*.md`·`docs/dotnet-canon-1NNN-*.md`이다. 정리 직전 원문 전체는 보관 파일 끝에 있다: 최신 [1.030 보관본](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261125-through-1030)(0.984~1.030의 요약표 단위별 행·R4-5 하위 행동 단위별 완료 나열·RU와 전송·수신 완료 항목 세부·R4-7 하위 체크·6절 0.969~0.998 단위별 나열)과 [0.983 보관본](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261124-through-0983)이다. 그 이전 보관본(0.950·0.900·0.837·0.797·0.732·0.635·0.613·0.545)과 각 보관본에 옮긴 내용은 [6절](#dotnet-archive-log) 이관 기록에 있다. cpppj와 공통 사항은 [LEFT_JOBS.md](LEFT_JOBS.md), 웹은 [LEFT_JOBS.webpj.md](LEFT_JOBS.webpj.md)를 따른다.

<a id="dotnet-overview"></a>

## 0. 현재 상태와 다음 작업

| 경로 | 실행·플레이 | 원본과의 동일성 |
| --- | --- | --- |
| 유일한 실행 경로 `Netstorm.Game` (`Netstorm.exe`) + 내부 `Netstorm.Client` | 0.807에서 구 근사 실행기를 제거하고 직역 호스트가 이름을 승계했다. 타이틀·메뉴·도움말·Options·Credits·캠페인 선택·Edit 목록과 이름 창이 동작한다. 개발용 `--mission <키> --handle <이름>`으로 일반31미션의 시작 상태 일부를 조립한다. 월드 표시·입력·규칙의 실제 경로가 남아 플레이 불가다. | 1024×768 메뉴 **12장면**의 논리 BMP가 cpppj와 바이트 일치했다(0.381~0.388). L2는 **0/34**(데모 3경로 DN 진입은 0.998에서 연결해 34경로 모두 대조)이며 L3·L4b·L5는 미착수다. |

- **검사 기준선.** 마지막 정기 전체 1.060은 Assets383·Core4088·Client2836 합계7,307통과/실패0이다. 정기 회귀 뒤에는 신규 검사가 없다. 0.880 이후 기존 이름·중복을 보존한다. NETSTORM_DATA 없이 Release 오류0·신규경고0이며 검사 사본은 실행본과 같고 원본1392파일은 전후 불변이다. 그 이전 단위별 집중 수는 각 단위 문서와 보관본에 있다. [1.040](docs/dotnet-canon-1040-20261126.md)
- **L2 기준선.** 마지막 수집은 extracted/dotnet-l2-0998이다. 전체34경로의68실행 모두 종료0·오류/누락0이며 데모3경로의 DN 덤프가 생겨 34경로 모두 대조에 들어간다. M1 **0/34**(31미션 동등 0·cpp_only997·dn_only21), 고정 수신26/26이다. CPP 차이는 기존 미초기화SP행3열 범위뿐이다. 세 설치 값(`_nativeLoaded`·`_editorUiInstalling`·`_editorPrefixUiInstalling`)은 false다. 0.999 이후 단위는 제품 변경이 없거나 검사·미결선 본체·세션 결선(프레임 미연결)만 더해 덤프에 영향이 없고, 1.001·1.008·1.020·1.024에서 대표 3경로, 1.031에서 대표 1경로(thewarbegins), 1.032에서 대표 3경로 DN 덤프가 0998과 바이트 일치함을 확인해 기준선을 유지한다(M1 0/34·cpp 1행 비결정 SP행 제외). [0.998](docs/dotnet-canon-0998-20261125.md) · [격차 분석](docs/dotnet-l2-0606-20261117.md)

<a id="now"></a><a id="09-바로-다음-작업"></a><a id="r4-board"></a>

### 0.1 재개 순서와 최근 완료 요약

1. **다음 — SL-8 파일 계열 소비자 → SL-9 → R5 월드 화면.** SL-7 루프·재시작은 1.060으로 마쳤다(영화 갱신 잠금과 2건 대조, 정기 전체 회귀 7,307, 대표 3경로 덤프 불변). R4-5e2 L2 대조 측정은 1.033으로 마쳤다(34/34 판정·258·틱 난수 AI 몫·live 틱은 R6). R4-5c 신전 삭제 결선은 1.032로 마쳤다. R4-5e1 세션 결선은 1.031로 마쳤다. 하위 행동 본체 20조각은 1.004~1.025로, 세션 호출자 중 수집자·부족 좌표·운반·사제는 1.027~1.030으로 결선했다. 남은 런타임 갈고리 `Drops`(배치 행동)·`Bridges`(다리 전략)와 호출자 안의 미연결 하위 갈고리는 R6에서 잇는다(2026-11-25 사용자 결정, 그때까지 명시 실패, [1.1 R4-5](#r4-detail)). R4-8 M1은 같은 시드·고정 프레임·빈 사용자 루트로 34/34 상태 일치를 판정하며 1.032 재측정까지 0/34다. 미룬 세션 로그인 본체는 C안 확정(2026-11-25 사용자 결정)으로 R6 SL-9·R11 수송에 이관했다(R4-7 마감).
   - 그 뒤 순서: ① SL-8 파일 계열 소비자 ② SL-9 ③ R5 월드 화면. PORT_MAP 행 보충([4절](#dotnet-inventory))도 남았다. 다음 정기 전체 회귀는 1.064이다.
   - R4-6 전송·수신은 R4 범위 수신기 설치가 0.979로 끝났다. 남은 것은 Root 생성 부모·Root Source 공개/제출·수송 세션 갈래이고, 그중 Root Source 공개/제출과 수송 갈래는 수송 backend(R11)·세션 수명(R6)이 필요해 그 단계에서 잇는다([1.1](#r4-detail) 전송·수신).
   - 선행 완료(중복 금지): 아래 요약표의 단위 범위와 1.1 완료 항목을 먼저 확인한다. 특히 Player 고유 Pre/Residence/Dude(0.813)·첫 장치 설치(0.814)·현재 지갑/Host 메모리 대여(0.815)·HUD 도킹/전체 Uber 부모·현재 this DockView/공통 HUD(0.817~0.819)·미니맵 기하·공유 FullRefresh·`RawMinimapControls`(MapRevision 대여·GumpRedraw 등록, 0.821~0.822)·NSInfo 생성/부착(0.823)·생산 패널/`OwnerColorTableState`·`ProductionControlContext`/`ClientProductionSync`(0.825~0.837)·`RawBaseFileIO`·CRT/TAFF(0.831~0.834)·Root 전송/같은 지도 getter(0.838)·Native Player 정기/Template(0.839)·Native 기본 Host·Process writer·패킷 분배 부모(0.841~0.899)·RU-1~RU-6(0.901~0.997)·분배 기본표·메타 수신기·Build 연결(0.933~0.982)·제단 정기 분기(0.939)·파일 계열(0.983·0.985)·R4-7 데모·보류 검사(0.998~1.003)·AI 하위 행동 본체(1.004~1.025)·직접 명령 입력(1.026)·세션 호출자 4종(1.027~1.030)·세션 AI 프레임 결선(1.031)·신전 삭제 결선(1.032)·L2 대조 측정(1.033)은 다시 만들지 않는다. WP-1 세션 Root 정리 상위 설치(1.034)·WP-2 제품 Root Path/Start/Motion 설치(1.035)·WP-3 전체 Native World 가상 삭제 분배 설치(1.036)·WP-4a 파생 수신·원격 삭제 결선(1.037)·WP-4b Dude 전용 PostPop(1.038)·WP-4c 사제 삭제·지면 소유자 결선(1.039)·WP-4d 월드·건설 효과 pin 전달(1.040)·WP-5a 사용자 옵션 경로(1.041)·WP-5b 커스텀 맵 읽기·저장(1.042)·WP-5c Edit 목록 스크롤 소비(1.043)·WP-5d 읽기 전용 설치 확인(1.044)·DI-1 Native Frame/Root UI 수집·그리기(1.045)·DI-3 커서 호출자(1.046)·DI-4a 카메라 Commit(1.047)·DI-4b 카메라 Present(1.048)·DI-4c 저장 카메라(1.049)·DI-4d Pan·결선(1.050)·SL-1 로그인·Root·Clock(1.051)·SL-2a intro 감지(1.052)·SL-2b PlayIntro 분배(1.053)·SL-3a EnergySupply(1.054)·SL-3b 에너지 조립(1.055)·SL-3c 주변음(1.056)·SL-4 저장 부모(1.057)·SL-5 메뉴 부모·Source(1.058)도 다시 만들지 않는다.
   - 주의: 기존83VT 실제 분배/고유 파생·Runtimepin을 잇고 미설치를 일반 Pre/Post/no-op으로 통과시키지 않는다. prepared 상태나 동일 깊이 검사를 완화하지 않는다. 실제 지도 영역을 전체viewport로 대체하지 않는다. Root의 세 번째 NSInfo와 F6 RawKnowledgeMenu를 혼동하지 않는다. 후속 UI/World 소비자도 같은 현재 색표/OwnerTables를 빌려야 하며 별도 표를 만들지 않는다. 실제 색표 연결을 NativeRoot/World 전체 설치 완료로 세지 않는다. 설치 후/Editor 성공 범위를 0.837의 거부 검사로 완료 처리하지 않는다. 초기 Source 미니맵 공장은 Open이 아닌 Construct와 기존 bitmap 준비/전체 도킹의 Push→Attach를 사용한다.
   - 소견: 최신 `extracted/dotnet-0837/next-0838-static.md`. 이전 Root·미니맵·생산 소견은 extracted/dotnet-0816·0820·0821·0822·0824·0826의 `next-08NN-static.md`다.

2. **R4 마감.** 위 순서 뒤 [1.1절](#r4-detail)의 R4-6 월드·경로(WP)·표시·입력(DI)·시작·수명(SL) 나머지와 R4-2~R4-5의 일치 대조를 닫고 R4-8(M1)로 넘어간다(R4-7은 1.003 뒤 마감). 이어서 R5~R11과 후순위 요구사항(3절)을 진행한다. R4-6 전송·수신의 R11·R6 의존 갈래는 해당 단계에서 잇는다.
3. **사용자 검사 범위(2026-11-22).** 캠페인1-1~1-5·스커미시에 한해 아군 AI 활성화로 포획/희생·특정 적 공격·시나리오 승리를 재현할 수 있다. 그 밖의 스테이지 해당 검사는 사용자 수동 확인으로 남긴다. 단순 진입/대기/생산/건설 검사는 진행한다. DN 자동 플레이의 실제 연결은 아직 없다. 원본1392파일은 마지막 정기 전체 회귀(1.024) 전후에 불변이다.
4. **사용자 추가 요구(2026-11-23, 기록만).** 세 프로젝트 공통 규칙은 cpppj가 1328~1330에서 정했다([공통](LEFT_JOBS.md#user-requests-20261123)). DN은 해당 소비자를 연결할 때 그 규칙을 따른다(2026-11-25 재확인: 셋 다 DN 코드에 아직 없다).
   - [ ] 방향키 카메라 이동 — 게임 중 ↑↓←→로 카메라를 옮긴다. 원본 ←/→ 조각 회전(`RawCampaignCursor.cpp:179~180`)과 TextEntry 커서 이동을 먼저 처리한다. 완료 기준: R5 카메라·R6 입력 연결 때 cpppj 규칙대로 구현하고 네 방향 이동·회전·입력 보존을 확인한다. 규칙: [cpppj1328](docs/exe/cpp-arrow-camera-1328-20261123.md)(`UserInput.cpp`·`UberGump.cpp`, 사건당 32픽셀, 입력 UI·조각 회전 우선).
   - [ ] ESC 게임 메뉴 한국어 — webpj에서 번역 누락 행과 "플레이어"(Players) 메뉴 글자 깨짐이 재현됐다. 완료 기준: DN 게임 메뉴 연결 때 Game·View·Options·Players·About 하위의 한국어 라벨과 Player 이름 표시를 화면으로 확인한다. 규칙: [cpppj1329](docs/exe/cpp-esc-menu-korean-1329-20261123.md)(공통 `assets/lang/korean/xlat.korean` 23키 보완·`RawGameMenus.cpp`). cpppj에서는 Players 글자 깨짐이 재현되지 않았다. DN 배포(`assets/game-data`)에는 아직 이 공통 번역 자산이 없다.
   - [ ] 정보(About) ▸ 버전 창 Copyright — 원본 Tell `[About]`에 11.01 패치와 `Copyright 2026 HJOW`(AGENTS.md)가 없다. 완료 기준: cpppj에서 정한 문구로 영어·한국어 창에 원본 저작권 줄·10.78 패치 줄·11.01 패치(HJOW)·`Copyright 2026 HJOW`를 표시한다. 문구: [cpppj1330](docs/exe/cpp-about-copyright-1330-20261123.md)(`ProductPresentation.cpp`의 About 확장, 원본 Tell 파일 불변). DN `ProductPresentation.cs`는 Credits 3항목·HJOW 카피라이트(0.342)만 반영했다.
   - 14차 모딩(QuickJS + webpj 이름 체계 공통 모드 API, 2026-11-23 사용자 결정)은 [3.1](#dotnet-modding)의 MOD-1~5다. 착수는 사용자 지시를 따른다.

**최근 완료 요약(0.639~1.035).** 단위별 완료 문장과 집중 검사 수는 보관본과 각 단위 문서에 있다. 완료 범위와 남은 일의 기준은 1.1·1.2절 체크 항목이다.

| 주제 | 단위 | 핵심 | 남은 일 |
| --- | --- | --- | --- |
| AI 런타임 | 0.639~0.641(0.641 부분)·0.645~0.650·0.653~0.658·0.661~0.663·0.665·0.815 | AI 생성부터 준비·전략·전체 프레임·통지·월드 등록·경제·시설 생산 본체와 세션 결선, 현재 지갑 전체/실제 Host 메모리 대여·장기 객체 통지 진입점(독립2520관측). 1.004 이후는 아래 AI 두 행 | [R4-5](#r4-detail) |
| 정기 처리 | 0.642·0.643·0.651·0.659·0.839·0.996 | 장식·Player·스프라이트·매몰 정기 본체와 세션 분배, Native Player 현재 상태/Template 후단·실제 Host 예약, Dais 로컬 SP 가산·0x307 공유 부착의 실제 장치(0.996) | R4-4 |
| 업그레이드·연구·생산 Deck | 0.666·0.667·0.682·0.683·0.685~0.687 | 업그레이드 요청/완료, 연구·생산 Deck·기술 표시의 실제 세션 연결, 58h 분배·연구 운반 | R4-5, [1.2](#r6-preports) |
| 플레어·건설 | 0.669~0.671·0.673~0.675·0.677~0.679·0.681 | 플레어·건설 진행/표시 본체와 제품 장치·음향 | 1.2 |
| 파편·35h | 0.809~0.811 | 전체 본체/수명·독립1196관측·건물 캡처2577/기존 작성336관측·실제 Host/Game 파편/섬 정리와 같은 삭제 분배 설치 API | 일반 Root 전체 Native 삭제·캡처/월드 뷰와 나머지 고정 수신, 1.1 |
| 로그인·전송·Client 전역 | 0.689~0.691·0.693~0.695·0.697~0.699·0.701~0.703·0.705·0.707·0.715·0.965·0.997 | 패킷·연결표·Poll·로그인/재접속·옵션·상태 전이·Clock과 장기 Client 전역, PacketDelays 선택 분포·장치·지연FIFO(수송 팩토리 hooks.delay 자리), 세 설치 값 공개 부모(0.997) | R4-6 |
| Runtime·Player 장기 수명 | 0.706·0.709~0.711·0.713·0.714·0.717~0.719 | 실제 Runtime 대여/반납, 장기 Player·Core·옵션 전역, 현재 Runtime pin | R4-6 |
| Client 프레임·장기 Host | 0.721~0.723·0.725~0.727·0.838 | Clock/Frame 공유, 프레임 실행 순서, 장기 Surface·Host Runtime | R4-6 |
| Native State 정리·저장소·반납 | 0.729~0.731·0.733~0.735·0.813~0.814 | State 정리 부모(원문294행), 실제 전역·저장소·장기SID, 실제 자식 증빙/Runtime 반납·제품 Surface 대여, MetaZone 본체/실제Client표·Runtime삭제경계와 Player 고유Pre/현재 records owner(독립360관측), 첫 장치 설치/동일Surface 장부·Graph·로딩·Native삭제깊이 공유 | R4-6(전체 가상 삭제 분배) |
| Root 자원·장기 자산표·선택 | 0.737~0.739·0.741~0.743·0.745~0.747 | NSInfo/서버 목록·장면 음악·Template/State 공유·State 효과분배/Root경계·첫메뉴 자산표·선택/강조/실제Clear/Root커서 | 나머지 기본 장치와 월드 표시 설치, R4-6 |
| 커서·카메라·Renderer·Host 표시 | 0.749~0.751·0.753~0.755 | 커서 고정 데이터·수명·위치와 현재 checking 공유, 카메라 16DWORD·실제 snapshot, Renderer 장면/캐시/dirty·Native submit·288원문 출력, Renderer 공개·Camera 초기화·구름 token·현재 Surface·Root 실제 Cancel | Native Frame/Root UI, 같은 지도 owner의 실제 HUD 작성자, 전체 커서 효과/입력, CameraCommit/Present |
| Chat 계열 | 0.757~0.759·0.761~0.763·0.765~0.767·0.769~0.771·0.773~0.775·0.777·0.995 | 실제 Subscriber·Chat 4800BYTE, manager·명령표·Chat 시작, 점 명령 분배, 제출·수신자·형식/제출 부모, Identity·일반 메시지/패킷, ChatWindow 상태·기하·전체 부모·공장/가상 분배, 공통 UI 소유권, 스타일 그림 출력, 중계·고정 등록, 장기 Client41h 수신 바인딩, 점 명령 효과의 실제 Root 연결(0.995) | NativeRoot/HUD 창 생성·Enter, 제출/송신 부모의 전송 backend(R11) |
| Root 수신·전송 관찰기 | 0.778~0.779·0.781~0.783·0.785~0.787 | UI 수신 원문·실제 Root UI 수신(뉴스·알림), 장기 패킷 기록/Root 소유자, 기본 18행·송신 관찰기, 장기 설정/버전 응답, 3f/30/7a/5f/7d 관찰기, 69h 장기 목록, 장기 CS/7bh | InstallClientPacketDefaults 나머지([1.1](#r4-detail) R4-6 전송·수신), Root Source/Record/Sent, NativeMapView 공급자 |
| 객체 패킷 수신·Host entry | 0.789~0.791·0.802~0.803·0.806·0.811 | 수신/변경/검증/압축/WORD 하위 본체, 실제 이름 83표·부착, Host 2c/7a/4a/4b·6a/7e/33/34/70의 실제 본문9개와변경/삭제5개·채집5bh·전체 개발자31h·섬 정리35h(나머지8본문과 26/26은 아래 Process 행) | 같은 Host 파생 수신/변경/전체 가상 삭제(Root 공급 RU-2는 0.902~0.903 완료) |
| 경로 Runtime·Data·본체·탐색 | 0.793~0.795·0.797~0.799·0.801 | 경로 Runtime·독립 관측, Data typed/Native 버퍼, Process 8종·전송 분배, BFS 탐색·Start/Cleanup 본체·Host 검색 공유, 건설 환불·Cleanup 하위·실제 SP 공유와 Halt/Arrival 본체·Host 부모 조립과전체Path/Start/Motion·30h | 제품Root전체장치설치와가상삭제 |
| HUD 도킹·전체 부모/공통 조립 | 0.817~0.819·0.821~0.822·0.991·0.993·0.994 | 도킹/같은 지도 owner(2150), 전체 Uber 부모/현재 this(160), 입력/방송(13398)·공통 Frame2186행과 실제 전체 UI 소유/가상 분배/수명·재배치, 미니맵 기하/셀 그림·공유 갱신 지원(4722/1028/120), 실제 공통 미니맵/기본 영역·출력/통지 수명(1474), 실제 Root 미니맵 설치와 MapRevision 공유(0.991), 같은 지도 owner HUD 작성자의 지도 저장소 공유(0.993), 살아 있는 선택 객체의 약한 연결 수명(0.994) | 실제 게임 Root의 나머지 패널/메시지 공장·바깥 UI 장치, R4-6 |
| 실행 경로 전환·Host 진단 | 0.805·0.807 | Host 부착 예외 소유권·독립 공장 진단, 구 근사 실행기 제거와 `Netstorm.Game`/`Netstorm.exe` 이름 승계·공통 ICO·갈무리11 배포 | R5 월드 화면, [0.3](#korean-font-change)·[0.4](#exe-icon) |
| NSInfo·생산 패널·색표 | 0.823·0.825~0.827·0.829~0.830·0.835·0.837 | NSInfo 전체 생성/일반 부착·공통 UI 소유, 생산 격자/항목/안내·정보/단축키/동기화/유지·실제 생산 부모/예산/출력, 현재 owner 색표·생산 문맥·장기 Finder, 최초 고정 RGB/primary/9표·캐시 부모, Game 최초 팔레트·TYPE/사용자 캐시·소유자 옵션, ClientProductionSync 장기/대여 분기·생산 본체/초기 거부·색표7 소비자 | 실제 Load/Editor 설치 부모(RU-6)·Root/생산 공장(RU-3b), NSInfo 외부 장치 실제 공급(RU-3a) |
| 파일 IO·검색·CRT | 0.831·0.833·0.834·0.983·0.985 | RawBaseFileIO 현재 FILE/키·부분 입출력 전체 본체(독립557관측), 같은 경로 Device/검색 수명·실제 Windows 검색·전체 열기 정책(독립1396관측), 등록 TAFF/현재 FILE·실제 CRT/캐시 reader·writer 입구, RawFileName 나머지 조회(Name·Directory·Full, 0.983), RawStaticStrings 외곽·ClientStaticStrings 고리 공유와 정수·캡처 서식·SavedStateFiles 경로와 Host 결선(0.985) | SavedState Read·Write 세션 결선(R6·[1.1](#r4-detail) SL-8 잔여) |
| Native 기본 Host·수명·writer | 0.841~0.844 | 기본 Host 지연 설치/Native 공장·동일 자원 예약, 기본 수명·현재 VT35종/피해·건설 사건, 기본 Dais/Path writer·실제 PID/FormPacket, 현재 Runtime의 알려진 ProcessForm 삭제(비 ProcessForm 자산은 명시 거부) | 전체 Native 가상 자산 삭제 분배(R4-6 월드·경로) |
| 건설·총·비행 Process | 0.845~0.847·0.849~0.851·0.853~0.855·0.857~0.859·0.861 | RawConstructionData codec/0x37, GunProcess raw/codec·3ah, FlyerProcess codec·FlyerPostPop·39h/86h, 훨리베이스(Sun) 38h·맨 오 워 풀(Rain) 73h·데빌메이커(Wind·RawWindCircle) 80BYTE/74h의 상태 기계·실제 Host/Native writer | Root 비행 생산/음향/큰 Frame, CDHost 미복원 경계 |
| 총 애니메이션·투사체·표적 | 0.862~0.863·0.865~0.867·0.869~0.871·0.873~0.874·0.878~0.879·0.881~0.882 | RawGunAnimation 공통 codec과 썬 캐논·아처·캐논/반더·크로스보우·아이스 캐논 발사, 미사일·포구/사정선·아이스 반사점·피해/DPS·반사탄/플로터, RawGunTarget 표적 Finder, 같은 World 전투 애니메이션 공장·3b/3c(RawCampaignArcherBindings)·고정 수신26/26(0.882) | Root의 Gun 큰 장치, RawGunTarget 실제 보호 자산 `cannon-asset-ray48` 검사 |
| Dais·Dude·Disc 정기·의식 | 0.875·0.877·0.883·0.885~0.887·0.889~0.891·0.893~0.895 | 디스크 정기·현재 TYPE 애니메이션 필드, RawDudeRegular, Rune 후처리/RawRunePostPop, 사제 의식(RawPriestRitualActions), TempleRitual, RawDaisRegular·RawRitualRegular, Dude·Disc·Dais 세션 typed 정기 분배, Kernel/Process 초기 등록 계약 | [R4-4 남음](#r4-detail) |
| 패킷 분배 부모·Build 연결 | 0.897~0.899·0.934·0.935·0.964·0.966·0.967·0.969~0.971·0.973~0.975·0.977~0.979·0.981·0.982 | `ClientPacketDispatch.Build`/`InstallDefaults` 기본표에 41h·로그인·설정/버전/기술/변수·Native gameplay 6종·기본 18행, Carrier 15행·건설/연구 8행 고정 F(원문 gameplay 직후·임대 복구), 장기 CS 0x7b·0x7c(호스트 챌린지 소유), 수송 팩토리 생성·재결합·Publish와 같은 표 공유, 메타 수신기 2번·1a/1b·0x66·0x26·0x1d·0x0f·0x71·0x21·0x88·0x67·0x20·0x22·0x23·0x24·0x25 연결(0.979로 R4 범위 설치 완료), 0x0f 서버 목록의 실제 NSInfo·Saves 결선(0.981). 항목별은 [1.1](#r4-detail) 전송·수신 | Root 생성 부모, Root Source/Record/Sent·제출과 수송 세션 갈래·종료/닫기/반납(R11·R6 의존) |
| Root 첫 장치·공급값·Bind(RU-1·RU-2) | 0.901~0.903 | PostPop 선택 재검사·Resource·Reward(실제 지갑 공급)/현재 데모·Deletion·PlayerRecords의 실제 owner 조립, 간헐천 AI 등록 순회, Editor 별칭 상자, 언어 번호·화면 크기·설정 치환 공급, `StartMission`의 `BindSquidPacketDevices` 생산 결선(고정 26F 중 Root 공급 항목) | RU-3~RU-6, [1.1](#r4-detail) |
| NSInfo 메타 UI(RU-3) | 0.905~0.907·0.909~0.911·0.913~0.915·0.917~0.919·0.921~0.923·0.925~0.927·0.929~0.931·0.986·0.987·0.989·0.990 | 메시지 공장 팔레트 프로브(`savedMessage`·`equalText`·`constructMessage`·`formatMessage`·`testMessage`·`visitMessage`·`messageEvent`), 등록 명부 39명 정렬, 메타 본문·플레이어/배틀 패널, BattleOptions 22레코드 표·변이, 루트/PlayerInfo/BattleInfo 원문 입력(87BYTE 송신·기본값 1..21), 지식 행 CRT 정렬·배틀 정렬/전체/개별/제외/제거 메뉴, 세 파생 입력 토큰 분배와 그 생산 결선, 메타맵 관문·특수 그림·입력 후크 분배 검증 | RU-3 나머지(RU-3b 잔여), [1.1](#r4-detail) |
| 분배 기본표·메타 수신 본체 | 0.933~0.935·0.937·0.938·0.941~0.943·0.945~0.947·0.949~0.951·0.953~0.963 | 435220 패킷 기록 요청 재설정(FILE/enabled 유지), 기본 18행·송신 관찰/버전 응답 검증, 메타 수신 본체·설치(004b4160 버전 확인·0042cad0 전투 설명·004b2c60 master 조회·0x7c·0x1d·0x66·1a/1b·0x0f·2번·0x26·0x88·0x25·0x21·0x23·0x24·0x67·0x20·0x22), Carrier/Construction 고정 F 설치·대여와 4종 송신 옵저버. 항목별은 [1.1](#r4-detail) 전송·수신 | 위 행과 같음 |
| 제단 정기 분기 | 0.939 | 44a5e0 제단(RunAltarRegular) 정기 분기 본체·후크·저항 상태 | R4-4 남음 2, [1.1](#r4-detail) |
| 정기 전체 회귀·L2·M1 재측정 | 0.644부터 4단위마다(1.024까지, 1.016·1.028 차례는 건너뜀) | 마지막 1.024 전체 7,167통과/실패0·기존 이름/중복 보존·원본1392 불변, 1.008·1.020·1.024 대표 3경로 DN 0998 일치·M1 0/34 유지 | 다음 1.032 · [1.024](docs/dotnet-canon-1024-20261125.md) |
| 데모·보류 검사(R4-7) | 0.998~1.003 | `StartDemo` 3경로 DN 진입(`--demo`, 0.998)·Player 월드 표 originals 840행(0.999)·패치/CD 장면 대조와 CD 조립 연결(1.000·1.001)·Template 진단 Client 연결(1.002)·미션 Fort 경로 읽기(1.003) | R4-7 마감, 미룬 세션 로그인 본체는 SL-9·R11, [1.1](#r4-detail) |
| AI 하위 행동 본체(R4-5b) | 1.004~1.007·1.009~1.019·1.021~1.025 | Core 20조각: 부족 좌표(`RawAiDrySpots`)·피해 반응(`RawAiDamage`, 149행 전수)·건설 선택/배치 탐색(`RawAiBuildChoice`·`RawAiBuildSearch`)·배치 표적/평가/스캔/선택/행동(`RawAiDrop*`)·가짜 좌표 탐색/가짜 부족 좌표(`RawAiDryGeometry`·`RawAiFakeDrySpots`)·사제/운반 행동(`RawAiPriestActions`·`RawAiCarrierActions`)·자원 선택(`RawAiResourceChoice`)·다리 경로/후보/계획/검색/전략(`RawAiBridge*`)·이동(`RawAiMovement`·`RawCarrierMoveState`) 본체와 fixture 대조(단위별 4~20건) | 세션 호출자 R4-5b·R4-5e, [1.1](#r4-detail) |
| AI 직접 명령·세션 호출자·프레임 결선·신전 삭제·L2 대조 측정(R4-5d·R4-5b·R4-5e1·R4-5c·R4-5e2) | 1.026~1.033 | 유틸리티 3명령 분기(`AiUtilityCommandInput`, `crankMoney`·`parameter`의 세션 연결), 수집자·부족 좌표·운반·사제 호출자의 세션 장치 결선(`HostSession.Ai*`, 호출자마다 미연결 하위 갈고리는 R6 명시 거부), 세션 AI 프레임 결선(`RunAiFrame(bool paused)`, 정지 단락·설치 검사·준비 완료 brain의 배치 명시 실패, live 틱 결선은 R6), 신전 삭제 결선(`BuildAiTempleDestroy()`, 범위 검사·실제 파괴, 시작 흐름 배선), L2 대조 측정(`ai_frame_gap.py`, brain parity 29/34·SP owner 1 drift·Player 947셀·난수 34/34, 판정·258·AI 몫은 R6) | R4-6 잔여(WP·DI·SL)·R4-5d 입력 꼬리·`GenerateWorldGeysers`, [1.1](#r4-detail) |

<a id="first-run"></a>

### 0.2 최초 실행 상태 — DN 적용 대기

원본 관찰과 cpppj 1314 구현이 완료됐다. DN은 빈 사용자 루트·무옵션 데이터 사본에서 아래 네 항목을 메뉴/설정 결선과 함께 확인한다. 2026-11-25 재확인: DN 코드에 `welcomeGiven` 처리가 없고 `HostStartupScene.SaveOptions`는 설치 데이터의 `d/options.cfg`에 쓴다. [원본 관찰](docs/exe/original-first-run-20261116.md) · [cpppj 1314](docs/exe/cpp-first-run-1314-20261116.md)

- [ ] Welcome(OK) → Welcome2(Play Early Missions / Main Menu) → 메뉴 버튼. 완료 기준: Welcome2가 열릴 때 `welcomeGiven=1`이 사용자 루트에 저장되고 재실행하면 Tips 분기로 간다.
- [ ] `HostStartupScene.SaveOptions`의 설치 데이터 쓰기를 사용자 루트로 옮긴다. 완료 기준: 설치 경로가 읽기 전용이어도 옵션·진행 저장이 사용자 루트에만 생긴다.
- [ ] Struggle For Freedom 최초 상태는 1번만 선택 가능하고 1~6번 모두 체크가 없다. 자기 `Done*`는 체크, 직전 `Done*`는 해금에 쓰고 저장·재실행을 확인한다. 실제 미션 결과 연결은 R6/R7에서 검증한다.
- [ ] 최초 Options는 소리·음악·바람·가장자리 스크롤·Auto-Demo·팁 ON, 스피커 교환 OFF, 효과음·음악 각각 5, 800×600·전체화면 요청이다. 기존 사용자 값과 진행을 보존하고 전체화면 재실행을 확인한다. 메뉴 일치 시험의 1024×768 창 모드는 시험 조건이다.

<a id="korean-font-change"></a>

### 0.3 한국어 글꼴 — DN 적용 대기

AGENTS.md의 현행 지정은 갈무리11이다.0.807에서 새 Game 빌드/게시/ZIP에 일반·굵은 TTF와 라이선스를 외부 fonts로 배치했고 D2Coding 배포 의존을 제거했다. 한국어 화면 연결은 R8 6차다. 2026-11-25 재확인: `KoreanFontFamily` 상수와 Game 화면의 갈무리11 글리프 출력은 DN 코드에 아직 없다(배포 복사만 있음). [공통 기준](LEFT_JOBS.md#korean-font-change) · [cpppj 1322](docs/exe/cpp-korean-galmuri-1322-20261117.md)

- [ ] `KoreanFontFamily` 공통 상수와 글꼴 파일·라이선스 설정을 마련해 로딩·측정·그리기·한글 대체·개발 표시가 같이 사용하도록 한다.
- [ ] 유일한 Game 경로의8비트 출력에 갈무리11의 비트맵 글리프를 적용한다. 구 실행기는 제거했으며 TTC 판독기 검사는 실제 TTC로 유지한다.
- [x] 0.807에서 Game.csproj/배포 스크립트의 복사·필수 파일·고지·안내를 맞추고 두 TTF/라이선스의 ZIP 전체 바이트를 검증했다.
- [ ] 작은 메뉴·도움말·브리핑과 영한 혼합 제목에서 일반/굵은 획·폭·기준선·줄 높이·잘림을 확인한다. 영어 `.chfnt`와 일본어 Noto Sans JP를 함께 대조한다.
- [ ] **WB E1288 점검에서 얻은 적용 조건(2026-11-23 기록).** 사용자 지시로 문서에만 반영하고 지금 작업하지 않는다. 위 둘째·넷째 항목을 구현할 때 지킨다. 근거: [WB E1288](docs/webpj-korean-glyph-grid-20261123.md)
  - 글리프: cpppj처럼 비트맵 TTF strike를 픽셀 그대로 정수배 복제하는 것을 우선한다(`EmbeddedBitmapFont`, 한글 칸 전진 폭 12·기준선 위 12줄). 외곽선을 래스터화한다면 설계 격자 1em = 12px로 그린다(11px이면 가로획이 빠져 `가`→`기`). 안티앨리어싱 결과를 문턱으로 자르면 ㅔ의 1px 간격이 메워지므로 4배로 그려 칸 평균을 쓰거나 비트맵을 쓴다.
  - 원본 글리프 혼합: Windows-1252로 나타낼 수 있는 글자(숫자·영문·공백·문장 부호)는 원본 `.chfnt`로, 확장 글자만 갈무리11로 그린다(cpppj `EncodeBytes`·`GumpTextDraw`). 모두 갈무리로 그리면 사이드바 SP 숫자(슬롯 3)가 커지고 공백 폭(5px 대 원본 3px) 차이로 Tutorial3 D1의 제네레이터 그림 세 개가 두 줄로 갈린다(webpj에서 실제로 난 결함).
  - 기준선·큰 슬롯: 확장 글자 기준선을 `max(원본 기준선, 배율×11)`(cpppj는 칸 기준 12) 이상으로 두어 슬롯 1·6(원본 기준선 9)에서 한글 윗줄이 잘리지 않게 한다. 큰 슬롯 배율·굵기는 cpppj(제목 1배·굵게)와 webpj(2배·보통체)가 다르며 cpppj D8 결정을 따른다.
  - 확인 기준: SP 표시가 영어 화면과 같다. Tutorial3 D1 그림 세 개가 한 줄에 놓인다. `가`의 잉크가 11줄이고 `에`의 ㅔ가 떨어져 있다. 슬롯 1·6 한글 윗줄이 잘리지 않는다.
  - 표시 확대: 현행 `ClientHostGame.Draw`는 `HostPresentation.IntegerFitRect`·`PointClamp`의 정수 배율이라 cpppj 전체화면 같은 획 불균일은 없다. 1080p에서 1024×768이 1배로 작게 보이는 문제의 고해상도 글자 층 제안은 [공통 0.7](LEFT_JOBS.md#korean-font-change)·cpppj D8 결정을 따른다.

<a id="exe-icon"></a>

### 0.4 실행 파일 아이콘 — EXE/ZIP 적용 완료, 창 표시 확인 남음

AGENTS.md 지정에 따라0.807에서 공통 [assets/Netstorm.ico](assets/Netstorm.ico)(1,078바이트, SHA-256 `bd4c18c5…f01e4f068`)를 Game.csproj ApplicationIcon으로 지정했다. 원본32×32/16×16의 두 원시 이미지가 개발/게시/ZIP EXE와 전수 일치한다. 빌드는 원본 EXE를 읽지 않는다. [0.807](docs/dotnet-canon-0807-20261122.md) · [공통 기준](LEFT_JOBS.md#exe-icon)

- [x] 0.807에서 공통 ICO를 재사용하고 Game.csproj ApplicationIcon과 실제 EXE의 두 원시 이미지/그룹을 전수 대조했다. 해시는0.807 보고서에 기록했다.
- [x] 0.807에서 개발/게시 EXE와 최종 ZIP의 EXE가 전체 바이트 같고 동일 아이콘/11.01 버전임을 검증했다.
- [ ] DesktopGL 창·작업 표시줄의 추가 아이콘 설정 필요 여부를 확인한다. 탐색기 사람 확인은 사용자에게 남긴다. 2026-11-25 재확인: Game은 csproj `ApplicationIcon`만 지정하고 창 아이콘을 코드에서 따로 설정하지 않는다.

<a id="dotnet-stage-board"></a><a id="stage-criteria"></a>

## 1. 단계별 작업과 완료 기준

| 단계 | 현행 상태와 완료 기준 |
| --- | --- |
| R0 기준선·동기화 | 완료. cpppj 기준선·PORT_MAP·`cpp_sync.py`. |
| R1 화면 호스트·입력 큐 | 완료. 8비트 호스트·입력 큐·비교 도구. 실제 월드 입력 소비는 R6. |
| R2 그리기·팔레트·글꼴 | 원시 본체 완료. 실자산 어댑터·수집/정렬·종류별 분배·`SpriteDraw`/그림자 공유와 `owner-capture` C행은 R4/R5에서 연결. |
| R3 Gump UI·메뉴 → D1 | 메뉴 L4a 12장면 일치. 메뉴 상위 소유자·모달·등록·대화창·텍스트 입력과 하위 화면 연결 잔여. 미룬 장면은 Language 자식 CJK(R8), Multiplayer 섬(R5/R11), Edit 이동행(R7). [UI 잔여](#r3-r2-detail) |
| **R4 월드 세션 조립 → M1** | **R4-2·R4-4·R4-5·R4-6 병행 진행 중**(0.689~0.997은 주로 R4-6, 1.004 이후는 R4-5). 34경로(Early 6·Training 9·캠페인 16·데모 3)의 같은 시드 Load/Start `session-dump`가 cpppj와 일치해야 한다(L2). [R4 체크](#r4-detail) |
| R5 월드 화면·카메라·HUD → D2 | 렌더러 수집·정렬·dirty/팔레트, 타입별 Draw와 효과, 카메라·전체화면 가장자리 스크롤, HUD·덱·미니맵·인벤토리. 진입 직후와 N프레임 뒤34경로 L4a 차이0 및 압축 해제 실행을 대조한다. |
| R6 플레이·규칙·AI·음향 → M2/M3 | Tutorial1 → Early Missions → 1-1·1-2 순서로 입력·커서·선택·메뉴, 사제/경로/운반/수확, 건설/생산/경제/다리, 전투, 의식/승패, AI, 소리/음악/동영상, 채팅/경고를 연결한다. M2는 고정 입력열 L3와 대표 캡처 일치, M3는 1-1·1-2가 기존 경로 수준 이상으로 진행되는 것. [선행 완료와 잔여](#r6-preports) |
| R7 미션 전체·Edit·설정·배포 → M4 | 34경로 진입·진행·저장, Retry/Next/Abort, 데모 2주기, Edit 왕복 저장·Save as·Test Battle/복귀, `options.cfg` 시점·전체화면 재실행, A~F 중 C·D·F 판정. ZIP 전수 대조와 압축 해제 실행, `Netstorm.exe` 외 실행 파일 사용법 문서화. 배포본 `Netstorm.exe`의 아이콘이 원본 실행 파일 아이콘과 같은지 확인한다(AGENTS 2026-11-19 추가, [0.4](#exe-icon)). |
| R8 AGENTS 4~15차 | 아래 차수별 요구에 도달할 때마다 기능을 검증한다. 공통 파일을 옮기는 과정에서 이미 들어온 부분도 실제 경로에서 확인한다. [요구사항](#dotnet-r8-requirements) |
| R9 60/120 FPS | Frame Rate 60/75(원본)/120과 양보 대기의 프레임 시간 대조. 비행체 표시 보간은 cpppj D2 결정에 따른다. |
| R10 Linux | DesktopGL와 파일 장치의 경로·권한·원자 저장을 확인한다. `$HOME/.netstorm/` 아래 `Saves/`·`CustomMaps/`·`logs/`를 사용한다. |
| R11 로컬 멀티플레이·Outpost | TCP/IP 로컬만 지원한다. `Netstorm.Net` 일부 사전 구현. 루트 서버 독립 모듈은 로컬 실행에 필요한 범위를 먼저 판정하고 실동작은 사용자 확인으로 둔다. |

<a id="r4-detail"></a>

### 1.1 R4 세션 조립 체크 (2/8 완료)

단위별 완료 문장은 [1.030 보관본](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261125-through-1030)·[0.983 보관본](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261124-through-0983)·[0.950 보관본](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261124-through-0950)·[0.900 보관본](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261123-through-0900)·[0.797 보관본](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261121-through-0797)·[0.732 보관본](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261121-through-0732)·[0.635 보관본](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261119-through-0635)과 각 단위 문서에 있다. 아래에는 하위 단계별 완료 범위와 남은 일만 둔다. 통합 조각을 마칠 때 같은 조건의 일괄 덤프로 M1 진척을 확인한다.

- [x] **R4-1 객체 수명 공통층(0.546~0.600).** 미션에서 `HostSession` 별도 풀을 제거했다. 풀·공장·공간·건물 저장소 공유는 live 슬롯 바이트 대조로 확인했다. Dais·Player·Aviary 개별 몸체, 장기 Unpop·Pop·Display·Graph·Host 실행기, 실제 전송·파생 Runtime pin은 R6 장치 결선에서 잇는다. [마감 감사](docs/dotnet-r4-1-close-20261117.md)
- [ ] **R4-2 적재 Pop 전체 상태 일치.** cpp 근거는 `RawNormalTerrainWorldBinding`·`RawRegionPaint`·`RawRegionOwnership`·`Islandbuilder`·`SquidPostPop`이다.
  - 완료(0.601~0.642): 섬·지형의 3×3 스탬프·섬 `Prefix` 발화·적재 깊이·그래프 per-Pop(0.601~0.605), 지원 타입 요새 일반 Pop과 농지 어댑터·Generate 실결선(0.607·0.612~0.614, 대표3 농지 선택 집합·타입·소유자 100% 일치), 지역 칠하기·소유권과 신전 가상 Owner·사원 Pop 파생(0.618~0.622), 공장 후처리·정기 트리거/본체/분배·Fort 경로 L2 대조·초기 레벨 적재(0.624~0.630), 장식 정기 본체와 세션 분배(0.642, 가상98 전체 몸체·world.Destroy 송신은 R6). [섬 후처리](docs/dotnet-session-island-postpop-20261117.md) · [그래프](docs/dotnet-session-terrain-graph-20261117.md) · [요새 Pop](docs/dotnet-fort-general-pop-20261118.md) · [Generate](docs/dotnet-canon-0614-20261118.md) · [0.622](docs/dotnet-canon-0622-20261119.md) · [0.630](docs/dotnet-canon-0630-20261119.md) · [0.642](docs/dotnet-canon-0642-20261119.md)
  - 남음(일치 대조): 농지 프레임/소유자를 맞춘다(0.728 프레임 일치 thewarbegins 107/152·tutorial1 56/69·surrounded 334/500, 소유자 일치는 합계 68/721). 그래프는 surrounded 255/255지만 thewarbegins 253/255 등 일반 31경로 중 26경로에 차이가 남는다. 표시 등록·스탬프·장부 `totals`/`totalmade`도 덤프와 대조한다.
  - 남음(요새·공장): 비용이 있는 전용 요새 8종(71·75·78·89·122·123·124·158)은 R6 파생 본체에 의존하고, 지원표 밖 처리도 남았다. 공장 뒤 4건의 sid 재사용 순서와 업그레이드 그림 의존 고리도 맞춘다.
- [ ] **R4-3 신전·소용돌이 → Player 첫 지역.**
  - 완료: 신전 Pop → 소용돌이 → 섬 소유자 → Player `+0x78` 경로값이 일치했다(0.622, `thewarbegins`·`tutorial1`). `surrounded` 혼합 경로에서도 첫 지역 9행이 일치했다(0.623).
  - 남음: AI 뇌 경로(`bc1menu76` 사원 없는 6행)는 R4-5 뒤에 닫는다. 농지 가장자리 432칸은 R6·R11 투표 입력이다. 거주 구역 5칸 전파는 0.623 기준이며, 0.635 거주 구역 연결 뒤 `surrounded`는 다시 재지 않았다.
- [ ] **R4-4 정기 처리·프로세스 초기 등록.** 입력은 `RawAssetRegular`과 Player·Dais·Dude·Disc·장식 정기 처리다. 정기 분배와 Kernel/Process 초기 등록은 0.894~0.895로 마감했고 아래 남음 두 단위가 닫히면 체크한다.
  - 완료(틱·정기 본체): 미션 틱 `AdvanceMissionFrame`(0.629), 고정 시각 1프레임 1틱과 덤프 시계(0.631), 매 프레임 `UpdateAmbientWind`(0.632)·주변음(0.633), 간헐천(0.634)·거주 구역(0.635)·펜스(0.636) Pop과 정기, 장식(0.642)·Player(0.643)·스프라이트(0.651)·매몰(0.659) 정기 본체와 세션 분배. [0.631 격차 분해](docs/dotnet-canon-0631-20261119.md)
  - 완료(펜스 생성·삭제): 표식·포스 필드 생성 본체 연결과 연결선·표식 삭제의 전체 파괴 결선(0.644). [0.644](docs/dotnet-canon-0644-20261119.md)
  - 완료(Dais·Dude·Disc·의식, 0.875~0.895): 디스크 정기(0.875·0.877), Dude 정기 본체(0.883), Rune 후처리·PostPop 전수대조(0.885·0.887), 사제 의식(0.886), TempleRitual(0.889), DaisRegular 본체와 공유 분기 RawRitualRegular(0.890), Dude·Disc·Dais 세션 typed 정기 분배(0.891·0.893·0.895), Kernel/Process 초기 등록 계약(0.894). [0.890](docs/dotnet-canon-0890-20261123.md) · [0.894](docs/dotnet-canon-0894-20261123.md)
  - [ ] **남음 1 — 정기 부착 시점과 나머지 틱 난수원.** 0.631에서 분해한 RNG 25,611 격차의 틱 구간, AI 전체 행동의 실제 매 프레임 호출, 장식 재생·연소 등 남은 난수원을 잇는다. 완료 기준: 대표 경로의 틱별 난수 소비가 cpp 덤프와 같아 0.631 분해의 격차가 0이 된다. [0.631](docs/dotnet-canon-0631-20261119.md)
     - 진행(0.996): 주변 바람(0.632)은 매 프레임 소비한다. 0.996에서 틱 소비를 바꾸지 않았음을 전체 34경로 DN 덤프 불변으로 확인했다. 남음: AI 매 프레임 호출(R4-5e)·장식·재생·연소(R6)·공장 사건 부착 시점과 미션 시작 Pop 정렬(R4-6 시작 흐름).
  - [ ] **남음 2 — Dais 실제 장치.** MakeDaisRegularHooks(실제 Frame/Host/Damageable/로컬 SP 결선)·실제0x307 Host 부착. 제단 분기 RunAltarRegular 본문·후크·저항 상태는 0.939에서 이식했다(후크 합성은 세션 설치에서 잇는다). 완료 기준: 세션 Dais 정기가 실제 hooks로 돌고 제단·0x307 분기에 미연결 진단이 남지 않는다.
     - 진행(0.996): 세션 가용분 완료 — `AddLocalSp` 실제 보상 가산(`reward.AddSp`)·신전 0x307 공유 예약·부착의 실제 Host 연결(`processes.AddSharedRegular`). 남음(R6): `Damageable` 전체 몸체(`RawAssetRegular` 이식)·음향·경로·의식 Runtime·제단 전체 후크 합성. [0.996](docs/dotnet-canon-0996-20261125.md)
  - 다른 단계로 넘긴 것: 거주 구역 사건 2(주민 생성), 펜스 이웃 새로고침(이벤트 1), 장식 가상98 전체 몸체·삭제 송신, Player 화면 로그·DataManager 통지는 R6이다. 펜스 표식·연결선 삭제의 송신·통지 일치는 R4-6에서 대조한다.
- [ ] **R4-5 AI 초기화.**
  - 완료(생성·본체): 생성·초기 갱신·해제·동맹 난이도(0.639~0.641), 준비·전략·전체 프레임·통지·월드 등록 본체와 장치 조립·세션 결선(0.645~0.650), 지역 경계·분산 SP·자금·지도 상한(0.653~0.654), 그래프 조회·경제 본체·신전 생산 TYPE(0.657~0.658), 경제 세션 연결(0.662). [최근 경제 결선](docs/dotnet-canon-0662-20261120.md)
  - 완료(하위 행동 본체): 수집자(0.655)·배치 선행(0.661)·시설 생산(0.663, 세션 0.665) 본체, 업그레이드 요청·수신·사건3 완료(0.666~0.667), 실제 업그레이드/연구·생산 Deck·내용물 통지의 세션 연결(0.682~0.686), 공유 분배기·58h 등록·연구 운반(0.687), 1.004~1.025의 20조각(부족 좌표·피해 반응·건설 선택·배치 탐색/표적/평가/스캔/선택/행동·가짜 좌표 탐색/가짜 부족 좌표·사제/운반 행동·자원 선택·다리 경로/후보/계획/검색/전략·이동). 20조각의 파일·대조 수는 [0.1 요약표](#now)의 AI 하위 행동 본체 행에 있다.
  - 남음 — 아래 다섯 세부가 닫히면 체크한다.
    - [ ] **R4-5a 제품 요청 양끝 전송.** AI 업그레이드·연구·생산 요청의 제품 요청/응답 양끝. 완료 기준: AI 요청이 실제 전송·Poll을 거쳐 기존 수신 본체(58h 등, 0.666~0.687)에 닿는다.
    - [x] **R4-5b 하위 행동 세션 호출자.** 수집자·시설과 나머지 하위 행동의 세션 호출자. 완료 기준: 각 본체(0.655·0.661·0.663 등)가 세션 AI 프레임의 실제 호출자에서 원문 순서로 불린다.
      - 완료(1.027~1.030, 호출자 b1~b4): `BuildAiRuntimeDevices`의 `AiRuntimeHooks`에 수집자(`Collectors`)·부족 좌표(`DryScan`·`DryNotice`)·운반(`Carriers`)·사제(`Priest`, 원문 SID 범위 검사 포함) 호출자를 실제 장치로 결선했다(`HostSession.AiCollectors`·`AiDrySpots`·`AiCarriers`·`AiPriests`). 호출자 안의 미연결 하위 갈고리(구매·참여·배치·생산·음향, 에너지 승인, 이동·자원 선택·하역, 탐색·건설 선택·제단·발자국·회수 명령 등)는 원문 순서 도달 시 명시 거부하며 단위 문서가 R6으로 적었다.
      - R6로 넘긴 것(2026-11-25 사용자 결정, 옛 R4-5b5·R4-5b6): 배치 호출자 `AiRuntimeHooks.Drops`(cpp `frameHooks.drops` → `RawAiDropActions.Run`(1.015), 하위 갈고리의 제품 조회·`RawAiDropSelection` 선택 등)와 다리 호출자 `AiRuntimeHooks.Bridges`(cpp `frameHooks.bridges` → `RawAiBridgeStrategies.Update`(1.024)), b1~b4 호출자 안의 미연결 하위 갈고리. 코드의 명시 실패 문구도 "(R6)"이다(`HostSession.cs`). R6 완료 기준: 장치 결선 단언과 미연결 하위의 명시 거부를 집중 검사하고 대표 3경로 L2 덤프가 불변이다.
    - [x] **R4-5c 신전 삭제.** 완료 기준: AI 신전 삭제가 세션에서 원문 순서·통지로 처리된다.
      - 완료(1.032): `BuildAiTempleDestroy()`가 원문 범위 검사(SID 범위, cpp `out_of_range`와 같은 순서) 뒤 전략 `setup.destroy`와 같은 실제 세션 파괴에 전달하고(`HostSession.AiBrains.cs`), 시작한 미션 흐름(`ClientHostGame`)에서 넘긴다. 원문 NoTemple 분기(첫 지역→섬 신전 조회→신전 제거→brain 없이 0 반환)의 순서·통지는 세션 파괴 분배를 그대로 쓴다. 기존 NoTemple 기록 관례(`AiBrains` 0 기록, `MissionPlayersLoadTests.CreatesAiBrainsNoTemplePathWithoutConstruct` 고정)는 유지한다. 근거: [1.032](docs/dotnet-canon-1032-20261125.md).
    - [ ] **R4-5d `SetAiParameter`·`CrankAiMoney` 실제 호출자.** cpp는 `UberGump.cpp`의 aiCommands(`crankMoney`·`parameter`)가 `RawCampaignSession`의 두 메서드를 부른다. 완료 기준: 두 명령이 세션 AI 메모리에 닿는다.
      - 진행(1.026): 명령 분기(`AiUtilityCommandInput` 상수·분류·분배)와 세션 조립(`DispatchAiUtilityCommand`→`CrankAiMoney`·`SetAiParameter`)을 이었다. 남음: 입력 꼬리 결선(UberGump 상당)·초기 간헐천 생성(`GenerateWorldGeysers`).
    - [ ] **R4-5e 매 프레임 실행과 L2 대조.** 완료 기준: AI 매 프레임 실행이 세션에 붙고 258 완료·brain·SP·Player 덤프 행이 cpp와 같다.
      - [x] **R4-5e1 프레임 부착(세션 결선 완료, live 틱 결선은 R6).** cpp `RawCampaignSession::PrepareFrame`은 일시정지가 아니면 `aiRuntime->Frame()`을 부른다. DN 세션에는 `RunAiFrame(bool paused)`을 두어 같은 조건·순서·설치 검사를 세션 API로 수행한다(1.031, `HostSession.PlayerWorld.cs`). 준비 완료 brain은 준비·목표·수집자·시설·부족·사제를 지나 배치에서 원문 순서 명시 실패(`AI 배치 행동 미연결(R6)`)한다. live 틱 결선(매 프레임 호출)은 Drops·Bridges와 함께 R6에서 잇는다 — `RunMissionFrame`에 넣으면 `thewarbegins` 90프레임 덤프가 종료 1로 끝나 L2 수집이 깨짐을 1.031에서 실측했다. DN `RawAiRuntime.Actions`는 사제 다음에 `Drops`·`Bridges`를 조건 없이 부르므로(`RawAiRuntime.cs:140~141`, cpp도 같은 순서) 그때까지 명시 실패한다. 근거: [1.031](docs/dotnet-canon-1031-20261125.md).
      - [ ] **R4-5e2 L2 대조.** 완료 기준: 대표 3경로 뒤 34경로 덤프에서 258 완료·brain·SP·Player 행이 cpp와 같다. R4-4 남음 1의 틱 난수 격차 중 AI 몫도 함께 확인한다.
      - 진행(1.033): 새 도구 `ai_frame_gap.py`로 0998 수집 34경로를 측정한 결과 brain 생성 parity 29/34(나머지 11 owner는 데모 진입 경로·tutorial5/6 적재 사제 결손으로 귀속 규명), SP는 owner 1만 34/34 drift(AI owner 일치), Player 947셀은 로그인·SID 체계, 난수는 34/34 divergence다. 258은 덤프에 행이 없어 양쪽 대조 불가다. 34/34 판정·258·틱 난수 AI 몫 분리·live 틱 결선은 R6에서 잇는다. 근거: [1.033](docs/dotnet-canon-1033-20261125.md).
- [ ] **R4-6 내부 로그인·전송·Client 전역.** 메뉴 핸들 출처와 이름 꼬리 16~19바이트, 로컬 Login→Ready→Start 실제 패킷, 요새 이미지·`Bind*`·State 수명을 연결한다. 실제 handshake 검사와 메뉴/`--mission`의 진입 덤프를 대조한다.
  - 완료(0.689~0.997): 묶음별 범위·핵심은 [0.1 요약표](#now)에, 단위별 완료 문장은 [0.797](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261121-through-0797)·[0.837](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261123-through-0837)·[0.900](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261123-through-0900)·[0.950](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261124-through-0950)·[0.983](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261124-through-0983)·[1.030](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261125-through-1030) 보관본과 단위 문서에 있다.
  - 남음(Root 장치·UI): RU-1·RU-2·RU-3c·RU-3d·RU-4·RU-5·RU-6은 끝났고 RU-3a·RU-3b가 남았다. 둘 다 미션 진입 Root 결선은 0.987에서 이었고 남은 장치는 R5 표시·메뉴·카메라와 R11 수송에 달려 있다. 각 단위는 집중 검사와 대표3 덤프/지표 불변으로 닫는다.
    - 완료(0.901~0.997, 항목별 원문 `[x]` 7개는 [1.030 보관본](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261125-through-1030)): RU-1 Root 첫 장치(PostPop 선택 재검사·Resource·Reward·Deletion·PlayerRecords·현재 Menu 데모 getter, `CreateAiWallet` 실제 공급, 0.901)·RU-2 공급값·State와 `StartMission`의 `BindSquidPacketDevices` 생산 결선(0.902·0.903)·RU-3c NativeRoot/HUD 창 생성·Enter(0.989)·RU-3d HUD 작성자(`GumpButtonTree.RegisterChatAlert`의 같은 `ChatAlertId` 칸)·Root reply Tell(`HostMenuInput.TryShowTell`, 0.990)·RU-4 미니맵 설치·HUD 도킹 작성자·선택 수명(같은 `MapRevision`, 전체 viewport 대체 없음, 0.991·0.993·0.994)·RU-5 Chat 점 명령 효과(0.995)·RU-6 Load/Editor 세 설치 값 공개(0.997). 넘긴 것: Flare/Mana/AttackSound/DaisFinish·Gun/Flyer/Aviary 큰 본체·장면음악·색표7·zone 완전 결선(R5/R6), 인게임 HUD 등록·버튼 점멸 확인과 Run(R5·세션 State 부모), Chat 번쩍임(R6)·일반 송신(R11), Load 하위 본체·최종 공개(R5/R6/R9).
    - [ ] **RU-3 Root 공장·창.** 실제 게임 Root의 네 자식·Root/생산 공장·채팅/메시지 공장과 바깥 UI 장치, NativeRoot/HUD 창 생성·Enter, 알림 등록 ID의 실제 HUD 작성자, NSInfo HUD/메타 UI/Root reply 저장. 완료 기준: 미션 진입 Root가 이 공장·창을 소유하고 세 번째 NSInfo와 F6 RawKnowledgeMenu를 구별한다.
      - 완료(0.905~0.931, 묶음 핵심은 [0.1 요약표](#now)의 NSInfo 메타 UI 행): 메시지 공장 팔레트 프로브와 NSInfo 메타 UI 본체(명부·본문·패널·옵션 표·원문 입력·메뉴·입력 분배)의 `HostNsInfoWindows` 생산 결선, 메타맵 관문·특수 그림·입력 후크 분배 검증. 단위 문서는 `docs/dotnet-canon-0905`~`0931`이다.
      - [ ] **RU-3a NSInfo 외부 장치 실제 공급.** 송신·음향·메뉴·카메라·입력·배치 15종 묶음 공장(`HostNsInfoRootDevices`)은 0.986에서, 미션 진입 Root 결선(`ClientHostGame.EnsureRootFactories`)은 0.987에서 이었다. 남음: 참가자·전투 옵션/제외/선택값/제거 메뉴와 중앙 카메라(R5, 지금 명시 실패), 12BYTE 상태·87BYTE 옵션 송신 outbox 비우기(R11 수송). 완료 기준: 미션 진입 경로의 NSInfo가 장치 미연결 명시 실패 없이 동작한다.
      - [ ] **RU-3b 네 공장·바깥 UI 설치.** Root·메시지 공장 집합과 진입 결선은 0.987에서 이식했다(생산은 세션 장치, 채팅은 메뉴 경로 소유를 런타임 장치로 묶음). `HostNsInfoWindows`는 이제 미션 진입 Root도 소유한다(2026-11-25 grep). 남음: 생산·채팅 공장 객체 생성과 메시지 공장 팔레트·배경 생성·사건(R5 표시, 지금 명시 실패), 메뉴 왕복 정리(SL-7). 완료 기준: 위 RU-3 완료 기준을 미션 진입 Root에서 집중 검사로 확인한다.
    - 이 묶음 밖 후속: 실제 F7의 상위 일반 입력, Dais의 실제 current records 하위. NSInfo의 메타 배치/입력/특수 그림 조립은 RU-3(0.907~0.931)에서 끝났다.
  - 남음(전송·수신, 근거 `cpppj/src/client/ClientCampaignTransport.cpp`). `InstallClientPacketDefaults`(270행)의 R4 범위 수신기 설치는 0.979로 끝났다. 남은 것은 Root 생성 부모·Root Source 공개/제출과 수송 세션 갈래이며, 대부분 수송 backend(R11)·세션 수명(R6)이 필요하다. 단위별 완료 문장은 [0.983](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261124-through-0983)·[1.030](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261125-through-1030) 보관본과 단위 문서에 있다. 종료·닫기·반납과 연결 활성 정리는 R11 수송에서 잇는다.
    - [ ] 전체 InstallClientPacketDefaults(270행)와 MakeClientCampaignTransport 부모. 아래 완료분은 재사용하고, 남은 설치는 2026-11-24 DN 코드 grep 기준이다.
      - [x] **고정 26F·메타 수신 본체·Build 연결(0.882~0.982).** `NativeGameplay26F` 26항목의 `BindSquidPacketDevices`·`ReceiveFixedGameplayPacket`, `ClientPacketDispatch.Build`/`InstallDefaults`의 41h(`Root.ReceiveNativePacketChat`·`ClientChatPacketReceiver`)·`NativeLoginPacketReceivers`·설정/버전/기술/변수·`NativeGameplayPacketReceivers` 6종·기본 18행, 메타 수신 본체·설치(0.937~0.963, 1a/1b `InstallClientStateBattleStartReceivers`·0x0f `InstallClientMetaGlobalReceivers`·2번 `InstallClientConnectionEndedReceiver`와 본체 `Netstorm.Net/ConnectionEnded.cs` 포함)와 Build 연결(0.964~0.979: Carrier 15행·건설/연구 8행 고정 F `NativeCarrierPacketReceivers`·`NativeConstructionPacketReceivers`, `PacketDelayGlobals` 지연FIFO·`PacketDispatchHooks.Delay` 자리, 0x7b `ClientChallengePacketDefaults`·0x7c `ClientMetaGameLogin`, 2번·1a/1b·0x66·0x26·0x1d·0x0f·0x71·0x21·0x88·0x67·0x20·0x22·0x23·0x24·0x25), 0x0f 서버 값·목록·저장의 NSInfo·Saves 결선(0.981)·0x20 검증(0.982). 후속 갈래(표시·NSInfo·Chat·음향·메뉴 Run 등)는 명시 실패로 남기고 Carrier/Construction 수신 본체는 RU-5/R5 후속에서 잇는다. 수송 팩토리(`ClientCampaignTransport.Make`)는 같은 표를 공유한다. 메타 참가자(0x20·0x21·0x22·0x88)는 남은 설치가 없다. 원래 세 `[x]` 항목의 수신기별 세부는 [1.030 보관본](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261125-through-1030)에 있다.
      - [ ] **MakeClientCampaignTransport 수송 팩토리.** 완료 기준: 호출자의 BindSession보다 먼저 같은 기본 표를 채우는 원문 순서로 전체 설치를 한 부모에서 호출하고, 임대 반납 뒤 C3/관찰기/metadata가 복구된다. 진행: 0.964~0.967에서 Carrier/Construction 설치 순서·복구, 지연FIFO, 0x7b·0x7c, 수송 생성·재결합·Publish와 `MissionNetwork`·`TransportGlobals`·`SharesNetwork`·`RebindWorld`·팩토리 후크 조립·기본표 설치 공유를 이었다. 남음: 세션 `RecordConnectionPacket` 갈래·`BindConnectionTransport` 부모, R11의 `BindMetaSource`·`MetaConnectionRuntime`·종료/닫기/반납과 연결 활성 정리.
    - [ ] Root의 실제 Source/Record/Sent·송신/제출과 HostSession 실제 송신 부모(구 `Mission.RawCampaignSession`의 sendChatMutable 입구로 대신하지 않음). Root Source 공개·제출 갈래는 수송 backend(R11)·세션 수명(R6)이 필요하다(0.979~0.983 단위 문서의 다음).
    - [x] 전체 435220 옵션 부모의 Requested만 0으로 재설정하는 호출(0.933, FILE/enabled를 닫지 않음). 재설정 호출을 받는 옵션 재해석 부모(전체 435220 재실행)는 R7 옵션 저장 시점에서 잇는다.
    - [ ] 나머지 순수 observer/장기 수신, 장기 CS/7bh의 나머지 Meta/UI/투영/배열 소비자와 Root 생성 부모. 26F 검토 행·누락 하위 모듈·독립 Path writer 자료는 extracted/dotnet-0788/next-0789.md다.
  - 남음(월드·경로). 순수 observer나 구 Mission 세션·일반 삭제 Pre/Post로 결선을 대신하지 않는다.
    - [x] **WP-1 Cleanup 상위 설치.** Cleanup의 실제 상위 설치·예약/의식/6ch·전체 가상 삭제 장치. 완료 기준: 세션 Root의 Cleanup이 예약·의식·6ch 정리와 가상 삭제를 원문 순서로 실제 장치에서 수행한다.
      - 완료(1.034): 세션 신전 의식 본체 보관·공개(`TempleRitual`)와 Root급 실제 갈고리(`BuildPathCleanupRootHooks`: 예약 항상 성공·의식 실제 본체·가상 삭제 실제 분배)를 두고 Motion 조립의 빈 예약·의식을 기본 연결했다. 0x6c 통지는 건설 환불 체인(기존 결선)으로 수행한다. AfterSpatial 네 분기(helper 예약·너겟 삭제·의식·남은 helper 환불+0x6c+삭제)를 원문 순서로 실제 장치와 함께 대조했다(신규 6건). 의식 Rune 하위·0x6c 송신 backend은 R6/R11에서 잇는다. 근거: [1.034](docs/dotnet-canon-1034-20261125.md).
    - [x] **WP-2 제품 Root의 Path/Start/Motion 설치.** 본체/API/Host·30h는 0.801 완료.
      - 완료(1.035): 세션 Root급 효과 묶음(`BuildMotionRootEffects`: Metadata·벌프 난수·의식 목표점·정리 예약/의식·가상 삭제는 실제 장치, Cd*·PlayAt·Robust는 FillMissing 상당 명시 거부, 입력·음향·포획·회피는 R6 명시 거부)과 한 번 설치(`BuildMotionRootDevices`: 섬·앵커·프레임·상태 조립)를 제품 진입 경로(`ClientHostGame`)에 연결했다. 근거: [1.035](docs/dotnet-canon-1035-20261125.md). 완료 기준: 제품 Root가 전체 Path/Start/Motion을 설치하고 미션 진입 경로가 그 장치를 쓴다.
    - [x] **WP-3 전체 Native World 자산 가상 삭제 분배.** MetaZone 원문 본체/Client표/pin 경계는 0.734~0.735, 현재 Runtime의 알려진 ProcessForm 삭제는 0.844 완료. 완료 기준: MetaZone의 전체 Native World 자산 가상 삭제 분배가 설치되고 일반 삭제 Pre/Post로 대신하지 않는다.
      - 완료(1.036): 세션 Root급 실제 전체 분배(`BuildWorldDeletionRootDevices`: 실제 프로세스 Runtime·장기 저장소·감시 쌍 조립, 원문 83개 가상표 전체를 실제 공통 본체로, 미복원 표 명시 거부)와 제품 진입 경로 연결(`ClientHostGame`)을 Native 전송·선택 계약으로 설치하고 Form 삭제·미복원 거부 3건과 대표 3경로 덤프 불변으로 대조했다. Gump·내용물 Form은 UI·내용물 본체(R3)에서 잇고 자산 삭제의 Damageable·사제 전처리·공통 삭제 장치는 WP-4·R5/R6에서 잇는다. 근거: [1.036](docs/dotnet-canon-1036-20261126.md).
    - [x] **WP-4 같은 Host 파생 수신·큰 경계·pin.** 같은 Host의 파생 수신/변경/전체 가상 삭제를 현재 Runtime·저장소 전송 관문과 함께 잇고, Dude 전용PostPop·사제 보호막/낙하 음향 등 기존 큰 경계와 World/건설 효과 pin 전달을 실제 설치한다. 완료 기준: 해당 경계의 미연결 진단이 미션 진입 경로에 남지 않는다. 검증 가능한 네 조각으로 나누어 이었다.
      - [x] **WP-4a 파생 수신·원격 삭제 결선(1.037).** 제품 진입 경로 `BindSquidPacketDevices`에 `VirtualDestroy` 지연 배선을 두고 2d/2e/65·53h·62h·개발자·채집 삭제가 같은 전체 분배로 동작함을 4건(53h 단일·62h 다중·2eh 변경 삭제와 미연결 명시 거부)으로 대조했다. 근거: [1.037](docs/dotnet-canon-1037-20261126.md).
      - [x] **WP-4b Dude 전용 PostPop(1.038).** Dude 후처리 본체(`RawDudePostPop`: idle·등록·I 프레임·공통 몸체)를 이식하고 분배 등록·세션 체인·요새 적재 결선을 이었다. 최초·재Pop·미설치·설치 거부·몸체 단위 5건과 대표 3경로 덤프 불변으로 대조했다. 근거: [1.038](docs/dotnet-canon-1038-20261126.md).
      - [x] **WP-4c 사제 삭제·지면 소유자 결선(1.039).** 사제 월드 기본값의 가상 삭제·지면 소유자 갱신을 같은 Host 장치(전체 분배·섬 받침)에 이었다. 적재 Pop 도달·깊이 복원, 받침 미설치 명시 실패, 601 삭제의 분배 관문 도달, 외부 Walker 훅 미연결 명시 실패 4건과 대표 3경로 덤프 불변으로 대조했다. 보호막·낙하 음향은 제품 음향이 이미 연결하므로 기본값 유지, 외부 Walker·안내·행동·측정·보충은 R6에서 잇는다. 근거: [1.039](docs/dotnet-canon-1039-20261126.md).
      - [x] **WP-4d World/건설 효과 pin 전달(1.040).** 미션 프레임의 월드·건설 효과를 현재 Runtime pin 안에서 실행하도록 `RunPinnedWorldFrame` API와 제품 루프 결선을 설치했다. pin 보유 실행·미공개 폴백·예외 반납 3건, 정기 전체 회귀 7,229, 대표 3경로 덤프 불변으로 대조했다. 근거: [1.040](docs/dotnet-canon-1040-20261126.md).
    - [ ] **WP-5 RawGunTarget 보호 자산 검사.** 실제 보호 자산 `cannon-asset-ray48` 검사(0.878 후속). 완료 기준: 실제 자산으로 표적 Finder의 보호 분기를 집중 검사한다.
  - 남음(표시·입력). 0.838의 같은 현재 지도 getter를 재사용하고 실제 HUD 도킹이 그 저장소를 작성한다.
    - [x] **DI-1 Native Frame/Root UI 수집·그리기.** 완료 기준: Root UI가 Native Frame에서 수집·그려지고 미연결 진단이 없다.
      - 완료(1.045): 미션 중 Root UI(생산·채팅·HUD)를 같은 공통 트리의 수집·그리기 경로로 매 프레임 그린다(메뉴 경로 불변). Root 수집 포함과 그리기 미연결 진단 없음 3건, 관련 906건, 대표 3경로 덤프 불변으로 대조했다. 전체 화면 대조는 R5 D2에서 한다. 근거: [1.045](docs/dotnet-canon-1045-20261126.md).
    - [x] **DI-2 같은 지도 owner의 실제 HUD 작성자(0.993·0.994).** RU-4와 같은 대상이며 RU-4 완료와 함께 닫았다.
    - [x] **DI-3 커서 호출자.** 전체 커서 효과/Drop/Previews/Position 호출자. 완료 기준: 네 호출자가 실제 입력 경로에서 불린다.
      - 완료(1.046): `BuildCursorDevices`가 같은 지형 Pop TYPE 인스턴스에서 수명·배치 본체를 조립하고(`TakeCursor`·`CancelCursor`·`QueryCursor`·`PrepareCursor`·`TryCancelCursor`), 제품 진입 경로에서 실제 포인터 이동을 현재 커서 위치에 반영하며 Esc 누름은 잡은 커서가 있을 때만 놓는다. UI·음향·프로세스 하위와 카메라 투영은 R6·DI-4 명시 거부다. 근거: [1.046](docs/dotnet-canon-1046-20261126.md).
    - [x] **DI-4 카메라 저장·Commit.** SavedCameraKey/무효점·CameraCommit/Present/Pan. 완료 기준: 실제 지도 영역으로 저장·Commit·Present·Pan이 돈다(전체 viewport 대체 없음).
      - [x] **DI-4a 카메라 Commit(1.047).** `RawCameraCommit`의 상태·전역·갈고기와 `CommitCameraTarget`(497960)을 이식했다(배율·이동·정지·IDIV 분기 4건 대조). Present·SavedKey·Pan과 세션·제품 결선은 뒤 조각에서 잇는다. 근거: [1.047](docs/dotnet-canon-1047-20261126.md).
      - [x] **DI-4b 카메라 Present(1.048).** `RawCameraPresent`의 전역·갈고기와 `PresentCameraTarget`(49a081)을 이식했다(진단 관문·두 제한 분기 4건 대조, 정기 전체 회귀 7,260). 전체 자체·세션·제품 결선은 뒤 조각에서 잇는다. 근거: [1.048](docs/dotnet-canon-1048-20261126.md).
      - [x] **DI-4c 저장 카메라(1.049).** 저장 카메라 갈고기와 초기화·포착·판정·숫자 분배 본체를 이식했다(초기화·포착·범위·저장·불러오기·무시 분기 5건 대조). 입력 경로 결선은 뒤 조각에서 잇는다. 근거: [1.049](docs/dotnet-canon-1049-20261126.md).
      - [x] **DI-4d Pan·결선(1.050).** Pan 본체·목표 쓰기를 이식하고 세션 카메라 조립과 메뉴 진입 결선을 이었다(Pan·목표·조립·실행·진입 9건 대조, DI-4 마감). 화면 이동·뒤집기·실제 지도 영역·소리·중심은 R5·R6에서 잇는다. 근거: [1.050](docs/dotnet-canon-1050-20261126.md).
  - 남음(시작·수명). 각 세부는 실제 제품 Root·세션 경로에서 원문 순서로 돌고 미연결 진단이 남지 않으면 닫는다.
    - [x] **SL-1 로그인·Root 등록·Clock(1.051).** 전체 로그인·제품 Root 등록·제품 Clock 프레임의 제품 순서를 세션 수준에서 고정했다(로그인 순서·Root 등록·Clock 전진 3건 대조, 제품 변경 없음). 근거: [1.051](docs/dotnet-canon-1051-20261126.md).
    - [x] **SL-2 Native 시작 초기화.** 전체 Native 시작의 intro/장면음악/나머지 지도 초기화와 Guide Init.
      - [x] **SL-2a intro 감지(1.052).** `DetectIntroMovie`를 이식하고 제품 시작 경로에 이었다(빈 경로·부재·존재·장치 누락 4건 대조, 정기 전체 회귀 7,281). PlayIntro 입력 분배는 State39·영상(R6)에서 잇는다. 근거: [1.052](docs/dotnet-canon-1052-20261126.md).
      - [x] **SL-2b PlayIntro 분배(1.053).** `DispatchPlayIntroInput`과 PlayIntro 명령 상수를 이식했다(미소비·뗌·누름 3건 대조, SL-2 마감). 장면음악 구동은 SL-3(나머지 음향)에서, Guide 진입은 R6(튜토리얼 모드·브리핑)에서 잇는다. 지도 초기화(카메라·월드 셀)는 제품에 연결되어 있다. 근거: [1.053](docs/dotnet-canon-1053-20261126.md).
    - [x] **SL-3 음향·저장·옵션·EnergySupply.** 나머지 음향/저장/옵션/전체 EnergySupply.
      - [x] **SL-3a 전체 EnergySupply(1.054).** `EnergySupplyState`와 `RawEnergySupply` 본체(수집·대응·평가·허용·소비·정렬)를 이식했다(분배·수집·대응·평가·허용·정렬 6건 대조). 세션 결선은 SL-3b에서 잇는다. 근거: [1.054](docs/dotnet-canon-1054-20261126.md).
      - [x] **SL-3b 에너지 세션 조립(1.055).** `BuildEnergyDevices`가 같은 세션 공급 목록·동맹·반경·치트 값으로 본체를 조립했다(조립·실행·우회 3건 대조). 요구 문자열 원천과 배치·AI 호출자 결선은 R6에서 잇는다. 근거: [1.055](docs/dotnet-canon-1055-20261126.md).
      - [x] **SL-3c 주변음 실재생(1.056).** 주변음 Sound 갈고리를 미션 음향이 있을 때만 실제 재생기에 전달했다(음소거·종류별 2건 대조, 정기 전체 회귀 7,295, SL-3 마감). 장면음악 구동·메뉴 주변음·화면 번쩍임은 R6(R5 화면 효과)에서 잇는다. 근거: [1.056](docs/dotnet-canon-1056-20261126.md).
    - [x] **SL-4 non-mine Notify.** non-mine Notify의 실제 DataManager/추출 부모.
      - [x] **SL-4 저장 부모 결선(1.057).** `BuildTemplateSaveDevices`가 같은 현재 manager·이름·숨김 전역과 실제 파일 쓰기로 저장 부모를 연결했다(미연결 실패·자연 복귀·실제 저장·억제 3건 대조). 편집기 저장은 R9에서 잇는다. 근거: [1.057](docs/dotnet-canon-1057-20261126.md).
    - [x] **SL-5 재인계·큰 메뉴 부모.** Source의 Editor/LAN/Meta 재인계와 원본 큰 메뉴 부모 3행(0.707).
      - [x] **SL-5a 큰 메뉴 부모·Source 재인계(1.058).** `InstallNativeMainMenuState`와 출처 재인계·Editor 진입·Battle14 통지를 이식하고 parent 3행을 실제 부모로 실행했다(3행 실행·intro·출처·Editor·갈래 5건 대조). 제품 호출 연결은 R6 상태 분배에서 잇는다. 근거: [1.058](docs/dotnet-canon-1058-20261126.md).
    - [x] **SL-6 Native 초기화 세대·WorldRuntime.** 앞선 Native 초기화 주소 세대와 전체 bridge(0.714), Native WorldRuntime 장치 일원화.
      - [x] **SL-6a 장치 일원화 고정(1.059).** 제품 순서의 공유 소유자 행렬(풀·SP·Gameplay·템플릿·프로세스 시각·로컬·커널)을 고정하고 엇갈린 소유자를 조립 단계에서 거부한다(2건 대조). 앞선 Native 초기화 주소 세대는 상수 해킹 없이 R4-8·M1에서 잇는다. 근거: [1.059](docs/dotnet-canon-1059-20261126.md) · [0.714](docs/dotnet-canon-0714-20261120.md).
    - [x] **SL-7 루프·재시작.** 미션 없는 장기 Surface/주변음·영화 갱신 잠금과 나머지 루프 단계(0.723), 메뉴 Kernel/효과·전체 State/자산 재시작(0.725). 완료 기준: 메뉴↔미션 왕복에서 루프 단계와 재시작 순서가 cpp와 같다.
      - [x] **SL-7a 영화 갱신 잠금(1.060).** `PauseMovieUpdates` 카운터와 제품 루프 관문을 이었다(가감·판정 2건 대조, 정기 전체 회귀 7,307). 나머지 루프 단계(연결 감시·SceneFrame·미니맵·전투 후처리)와 전체 State·자산 재시작·왕복은 R5/R6/R7에서 잇는다. 근거: [1.060](docs/dotnet-canon-1060-20261126.md).
    - [ ] **SL-8 파일 계열 소비자.** RawFileName 나머지 조회(Name·Directory·Full)는 0.983에서, RawStaticStrings 외곽·ClientStaticStrings 고리 공유와 정수·캡처 서식·SavedStateFiles 경로와 Host 결선은 0.985에서 이식했다. 남음: SavedState Read·Write의 세션 결선(R6). 완료 기준: 두 장치가 실제 소비자 경로에서 쓰인다.
    - [ ] **SL-9 미룬 세션 로그인 본체(R4-7c3c에서 이관, C안 확정).** 미션 Fort 경로 서버 설치·로그인 객체 전송·미션 재적재·미션 객체 stream·새 Fort 검사와 세션 `LoadLocalConnection`·`Load`·새 Fort 설치 본체. 수송 연결 갈래는 R11에서 함께 잇는다. 근거: [분석](docs/dotnet-session-login-gap-20261125.md). 완료 기준: 13검사가 실제 세션 경로에서 통과한다.
- [x] **R4-7 데모와 보류 검사(0.998~1.003).** `StartDemo` 3경로 DN 진입(R4-7a, 0.998), `player-world-x86.tsv` 중 originals 840행을 DN `RawPlayerWorld` 실제 장면으로 대조(R4-7b, 0.999, CD 840행은 cpppj 몫), F3 보류 검사의 패치 장면(`FortWorldLoadBindingTests`·`NormalTerrainWorldBindingTests`, R4-7c1, 1.000)·CD 장면과 CD 조립(R4-7c2, 1.001)·Template 진단 전체 Client(R4-7c3a, 1.002)·미션 Fort 경로 읽기(`CampaignMissionFortPath`, R4-7c3b, 1.003)를 마쳤다. 미룬 세션 로그인 본체(R4-7c3c)는 C안 확정(2026-11-25 사용자 결정)으로 SL-9·R11 수송에 이관했고 Tell 메타 1종은 대화 본체(R3k-11)로 넘겼다. 하위 체크 6개(`[x]` 5·이관한 `[ ]` R4-7c3c)의 원문은 [1.030 보관본](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261125-through-1030)에 있다.
- [ ] **R4-8 M1 판정.** 같은 시드·고정 프레임·빈 사용자 루트로 34/34 상태 일치. 로드 뒤 프레임 진행에서만 생기는 차이는 cpp 후속 프레임 덤프로 입증한 뒤 R6으로 넘긴다.
  - 진행(2026-11-25): DN 프레임 1 덤프 3경로 + cpp 프레임 61·150 덤프(thewarbegins)로 분해했다.
    DN은 프레임 1→90에 슬롯을 하나도 만들거나 지우지 않으며, M1 슬롯 수 차이는 DN 프레임 1 시점에 이미
    전부 존재한다. 지형 bounds 34/34·정적 타입 개수는 DN-1과 cpp-90이 일치해 로드 구조물 패리티가 있다.
    cpp에만 있는 슬롯은 전부 생산·건설·채집·플레이어 계열이고 cpp 90→150에 type-6이 1개 더 생긴다.
    owner 불일치(농지 owner 2 대 0 등)는 프레임 61→90에 변하지 않아 로드 매핑·초기 AI 배치 문제다.
    근거: [분해](docs/dotnet-m1-load-vs-frame-20261125.md). 남은 일은 R4-5 AI 호출자·매 프레임 실행과
    R4-4 틱 RNG·장식·공장 사건 부착, R6 런타임·세션 수명이며 M1 재측정은 1.008·1.020·1.024에서 닫았다(DN 3경로 0998 일치·M1 0/34 유지·기준선 0998 유지).

<a id="r6-preports"></a>

### 1.2 후속 단계에서 이미 이식한 범위

- [x] **R6-3 배치 본체(0.609~0.611).** 권한·관계, 지형 메이커 2종, Player 앵커·서라운딩·파이프라인을 이식했고 MayPlace 3건 전수 일치를 확인했다. 실제 농지 호출은 R4-2(0.614)에서 결선했다. 사용자 건설 입력 통합은 R6-3에서 검증한다. [0.611](docs/dotnet-canon-0611-20261118.md)
- [x] **펜스 본체 8종(0.587~0.592).** `RawFenceLifetime`·`Neighbors`·`Line`·`MarkerClear`·`Rebuild`·`Create`·`PostPop`·`Regular`과 해당 저장소 갈고리를 이식했다. 세션 Pop·정기 결선은 0.636에서 마감했다. 표식·포스 필드 생성은 0.644에서 연결했다([0.644](docs/dotnet-canon-0644-20261119.md)). R6의 타깃·타입 부모·Damageable 정기 라우팅과 연결선 생성·이웃 새로고침은 열려 있다. [펜스 후처리와 정기 처리](docs/dotnet-surface-fence-postpop-20261117.md)
- [x] **시설 업그레이드 본체(0.666~0.667).** 요청·수신·공통58h writer와 사건3 그림/완료·실제 Host 정리·현재 AI 통지 본체를 이식했다. 가상/건설/표시/음향과 요청 API는0.682~0.683, 연구 내용물/Deck·부모 통지는0.686에서 실제 세션에 연결했다. 공유 분배기·58h 등록·연구 운반은 0.687에서 연결했다([0.687](docs/dotnet-canon-0687-20261120.md)). 제품 요청 전송과 UI, 세 비행장 홈 통지·게임 내 HUD 메시지(0.682)는 후속이다. [요청](docs/dotnet-canon-0666-20261120.md) · [그림/완료](docs/dotnet-canon-0667-20261120.md)
- [x] **플레어·건설 본체와 실제 장치(0.669~0.681).** 플레어 생성/정기, 건설 진행/완료/실제Host 수명, 밀어내기/실패 섬 정리(0.669~0.671), 건설 표시 생성/동기화/입자 선택1,776행·프레임/Draw1,317행과 실제10.78 Kernel/Form/공통 목록 수명12건(0.673~0.675), 공장 가상 그림·실제 SHP 캡처(0.677), 같은 WorldRendering/Host/Kernel·COL 팔레트·현재 카메라/dirty/8비트 출력의 제품 건설 표시 장치(0.678), 플레어의 실제 장치·정기·음향(0.679), 건설 진행/완료 장치(0.681)를 연결했다. private 효과의 DropIsland 낙하/전송, 실제 건설 요청, Carrier 플레이 효과, 나머지 캡처·전용 색표·R5 월드 뷰는 후속이다. [플레어](docs/dotnet-canon-0669-20261120.md) · [건설 진행](docs/dotnet-canon-0670-20261120.md) · [표시 프레임](docs/dotnet-canon-0674-20261120.md) · [0.678](docs/dotnet-canon-0678-20261120.md) · [0.681](docs/dotnet-canon-0681-20261120.md)
- [x] **간헐천 수명과 벌룬 본체(0.583~0.584).** 세션 Pop·사건 0 정기는 0.634에서 결선했다. 채굴 `RawGeyserMined`은0.803에서독립2312관측/Host5bh를연결했다. Root의전체음향/가상삭제와운반상위는남았다. `RawBalloonFrame`은0.795의Path본체에포함되며제품Root설치가남았다. AI 등록은 R4-5에서 잇는다. [간헐천](docs/dotnet-surface-geyser-20261117.md) · [벌룬](docs/dotnet-surface-balloon-20261117.md)
- [x] **Outpost 전처리 본체(0.582).** 장기 삭제 분배·목록 압축·부모 호출을 집중 검사했다. 전체 Outpost 수명과 LAN 통합은 R11에서 이어간다. [Outpost](docs/dotnet-surface-outpost-20261117.md)
- [ ] **공통 장치의 실제 미션 연결.** `RawSurfaceAssetDeletion/Notice`·`Content/Reward`·`Lifetime/Deletion`·`PriestDeletion`·`Damageable/Fence/Balloon`은 대응 본체 또는 분배 장치가 있다. 큰 파일의 미이식 분기와 호출자 장치는 R4-2~R6에서 확인한다. `RawIceTower`의 성장 정기·완성형 파괴·재생성/삭제와 Native 설치도 남아 있다. [R4-1 감사](docs/dotnet-r4-1-close-20261117.md)

<a id="r3-r2-detail"></a><a id="carried-limits"></a>

### 1.3 앞 단계에서 이월한 작업

- [ ] **R3 UI.** `ParseDialogPage`·`ConfigOverwriteAndQueueLifetime` 잔여와 MessageGump, 맵/파일/기술 목록·Guide·모달·명령 소유자, QuickHelp TYPE/AST, Console/클립보드/Save as/숫자 창, ChatWindow의 NativeRoot 창 생성·Enter(상태·기하·전체 부모·공장은 0.767~0.774 완료), 배경 GIF 노드·창, 하위 화면 라벨과 실제 명령 소비. HTML/스타일 그림의 실제 자산/본문·메뉴 출력은0.773 완료이며 Native 색표7/옵션 변경 부모는 후속이다.
- [ ] **R2 화면 통합.** 실자산·그리기 결선과 구름 카메라·시간 이동·dirty 갱신·구름 파일 fallback(R5). 빠른 물리 입력 이력과 옵션 변경 중 소리 수명은 R6.
- [ ] **파일·호스트 경계.** `--set` 중복 처리 차이, `MatchDisk` 절대/긴 경로, Linux `WriteAtomic`·`ReadOnly`, 이름 정규화·옛 인코딩, Fort 이름/Template 소유자·Save as는 해당 소비자를 옮길 때 대조한다. CD 슬롯 폭·경계의 하네스와 기존 단위의 예외·할당·복사 경계는 보관본 2.6과 단위 문서를 참조한다.

<a id="dotnet-current-plan-20261028"></a><a id="05-실행-순서-r0r8"></a><a id="04-기존-dotnetpj-소스의-재수정-판정"></a><a id="decisions"></a><a id="08-사용자-확인이-필요한-것-확인-전까지-에이전트-기본값으로-진행"></a><a id="dotnet-user-decisions"></a>

## 2. 계속 유효한 결정

- **원본 10.78 경계 보존.** 화면은 8비트 인덱스 버퍼·256색 팔레트, MonoGame은 창·입력·오디오 장치·최종 출력이다. cpppj의 파일/클래스·호출 순서·부작용·정수 폭·float32 저장·목록·난수·PATCH 50바이트 슬롯 배치를 보존한다. 플랫폼 장치는 .NET/MonoGame으로 대체하고 OS 전용 동작은 호스트에 둔다. cpppj 미복원·보류 경계는 명시 진단으로 드러낸다. CD 10.72 제품 복원은 cpppj 범위다.
- **D1(사용자 결정).** cpppj HEAD의 요구사항 코드가 공통 파일에 섞여 있으면 함께 옮긴다. 기본 영어·보통·75 FPS·4:3 창 모드에서 원본 동작을 먼저 대조하고 후순위 기능은 R8에서 검증한다. 파일 장치는 사용자 루트 정책을 초기 이식부터 적용한다.
- **D2(사용자 결정,0.807 적용 완료).** 2026-11-22 사용자가 즉시 전환을 지시해 구 실행기를 제거하고 직역 호스트가 Game/Netstorm.exe를 승계했다. 기존 settings.json은 가져오지 않으며 홈의 기존 저장값을 삭제하지 않았다. 전용 자동 실행 검사16개는 은퇴시켰고 공통 부품/L1 검사는 유지한다. R5 전환/R6 삭제라는 이전 에이전트 해석은 폐기한다.
- **D3~D5(사용자 결정).** R8에서 로컬 MCP와 한국어·일본어 화면을 검증한다. Linux는 `$HOME/.netstorm/` 아래에 저장한다. Template 판독·433 Fort 왕복 검사의 기존 결정은 [Template 계획](docs/dotnet-template-storage-plan-20261101.md)에 있다.
- **사용자 지시.** 정기 전체 회귀는 4단위당 1회만 수행한다(2026-11-19). 포획/희생·특정 적 공격·승리 재현은2026-11-22 지시로캠페인1-1~1-5·스커미시에서만아군AI 활성화로허용하며나머지는사용자수동확인으로둔다.

<a id="original-parity-plan"></a>

### 2.1 원본과의 의도적 차이(9-1 결정)

같은 입력의 원본 근거를 확보해 동작 차이를 판정한다. 현 구조로 원본 경험을 재현할 수 있으면 유지하고 필요하면 구조를 바꾼다. [2026-10-10 결정과 A~F 상세](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261118-through-0613)

| 항목 | 완료 범위와 다음 작업 |
| --- | --- |
| A 시계·고정 틱 | 프레임 명령·붕괴 스캔·정지 시각 첫 단계 완료. 나머지 정수 틱·보간·대본/브리핑 지연·틱 기반 검사 이전은 R4/R6. [첫 단계](docs/dotnet-frame-parity-20261011.md) |
| B 입력 큐 / E 부분 다시 그리기 | 40칸 입력 큐 본체는 R1 완료, 실제 사건 소비는 R6. dirty 영역 표시 경로는 R5. |
| C 설정 / D 프레임 대기 / F 실행 환경 | 사용자 루트 `options.cfg` 저장 시점과 전체화면 재실행, 양보 대기 프레임 시간, 중복 실행·창 크기 등 관측 동작은 R7에서 판정한다. |

<a id="dotnet-r8-requirements"></a><a id="agents-reflection"></a>

## 3. AGENTS 4~15차 — R8 반영 범위

현행 AGENTS 변경은 갈무리11 지정 `2c8ca3bf8`, 15차 추가 `faecb9cbd`, 실행 파일 아이콘 `d047bf9a3`([0.4](#exe-icon)), webpj favicon `11b16b778`(DN 해당 없음), 인계 문서 정리 규칙 `1fc63cb1a`(문서 관리만, 2026-11-23~25 정리에 적용)까지 확인했다(2026-11-25 재확인). 아래는 새 경로 기준이다.

| 차수 | 처리 및 검증 |
| --- | --- |
| 4 로컬 MCP·와이드 | stdio JSON-RPC로 모든 조작·상태 조회·화면 캡처·소리 포함 녹화. 4:3·16:9·16:10, 16:9 최대 1920×1080의 논리 해상도와 그래픽 크기 유지. [MCP](docs/exe/cpp-mcp-server-20261106.md) · [와이드](docs/exe/cpp-wide-resolution-20261106.md) |
| 5 사용자 데이터 | Windows `%USERPROFILE%\Saved Games\Netstorm\` 아래 모든 런타임 생성물. 설정·진행은 `Saves/`, 맵은 설치 경로와 `CustomMaps/` 양쪽 읽기, 새 Edit 맵은 `CustomMaps/` 저장. 목록 무제한 스크롤과 읽기 전용 설치 확인. [cpppj 사용자 루트](docs/exe/cpp-user-data-root-20261105.md) · DN 분할·완료: 5a 옵션 경로(1.041)·5b 읽기·저장(1.042)·5c 목록 스크롤(1.043)·5d 읽기 전용 확인(1.044) 모두 `[x]` |
| 6 한국어 | 외부 갈무리11과 한국어 명칭 규칙. 표시용 부호화·`DisplayTextInput`의 선행 이식을 화면·번역과 연결한다. [전환 체크](#korean-font-change) |
| 7 오류·로그 | 공통 오류/예외 처리와 상세 로그(`logs/yyyyMMddHHmmss.log`), 시작 실패부터 추적 가능해야 한다. [cpppj 로그](docs/exe/cpp-error-log-20261111.md) |
| 8 AA·FSR 1.0 | Options에서 선택, 둘 다 기본 OFF. 지원 환경의 2560×1440·3840×2160은 내부 1920×1080에서 자동 FSR. [cpppj 출력 계약](docs/exe/cpp-aa-fsr-20261106.md) |
| 9 시나리오 Edit | 새 맵 기본 `.english` 자동 생성, 누락 시 Test Battle 종료 오류 교정, 편집 UI에서 초기 SP·테크·플레이어·동맹 수정. [cpppj 편집](docs/exe/cpp-scenario-s3-ui-20261111.md) |
| 10 알려진 버그 | cpppj의 34건 판정과 수정 10건을 해당 파일의 이식·검증에 반영하고 NB 번호를 기록한다. [판정표](docs/exe/cpp-known-bugs-status-20261111.md) |
| 11 난이도 | Options 행과 AI 배율 부품 선행. 기본은 보통(원본), 쉬움은 적 AI의 다리·생산 주기 ×4와 채취 유닛 절반(최소 1), 어려움은 적 AI의 주기 ×½. 동맹 판정 포함. [cpppj 난이도](docs/exe/cpp-difficulty-d-20261111.md) |
| 12 스커미시 | Campaign에서 시작/취소, 적 수·AI별 난이도/동맹, 금지 유닛 최대 5개(Outpost 포함). 멀티플레이 규칙의 임의 섬·간헐천. [cpppj 스커미시](docs/exe/cpp-skirmish-sk-20261111.md) |
| 13 일본어 | 외부 Noto Sans JP, 일본어 UI와 본문 영어 대체. 번역 범위는 cpppj D3. [cpppj 일본어](docs/exe/cpp-japanese-l1-l3-20261111.md) |
| 14 QuickJS | quickjs-ng 엔진 탑재·자체 검사·라이선스 동봉. **2026-11-23 사용자 결정: 모딩 지원** — webpj 이름 체계의 공통 모드 API를 QuickJS로 제공하고 게임 로직은 C#을 유지한다. 결정·세부 단위는 [3.1](#dotnet-modding). [cpppj 엔진 계약](docs/exe/cpp-quickjs-engine-20261111.md) |
| **15 Reset All** | **미착수.** Options 맨 아래에 Reset All을 제공한다. 취소 시 현재 설정·진행을 보존하고 확인 시 `options.cfg`를 포함한 설정·게임 진행을 초기화한 뒤 종료한다. 다시 실행해 최초 상태를 검증한다. cpppj1325에서 확인/취소·초기화·종료와 최초 상태 복원을 구현했다(2026-11-22). DN은 해당 Source를 옮길 때 대조한다. [AGENTS.md](AGENTS.md) |

**공통 완료 사항.** 기존 배포의 `Netstorm.exe` 이름과 압축 스크립트, 11.01·Credits 3항목·HJOW/AI 크레딧 및 카피라이트(0.150·0.342), 폐기 클라이언트 유효성 검사 비활성·외부 접속 문서화(0.151). 0.807에서 직역 호스트가 `Netstorm.exe` 이름을 승계하고 원본 아이콘을 EXE/ZIP에 적용했다(창 표시 확인은 [0.4](#exe-icon)). 새 소비자가 생기면 외부 접속 목록을 갱신한다. [배포 스크립트](dotnetpj/package-dotnetpj.ps1) · [외부 접속](docs/dotnet-external-endpoints-20261103.md)

<a id="dotnet-modding"></a>

### 3.1 14차 모딩 — QuickJS + webpj 이름 체계의 공통 모드 API (2026-11-23 사용자 결정)

**결정.**
- dotnetpj에 QuickJS(quickjs-ng, cpppj와 같은 v0.18.0 소스·provenance `cpppj/third_party/quickjs-ng`)를 탑재해 다른 사용자가 기능을 더하는 모딩을 지원한다.
- 모드 API는 webpj의 함수·클래스 이름 체계를 따른다. webpj 내부 모듈(`Raw*`·원문 주소식 이름 등)을 그대로 노출하지 않고, 이름·형태를 webpj에 맞춘 얇은 공개 계약을 정한다. webpj가 기준 구현이고(브라우저 JS라 엔진 불필요) dotnetpj는 같은 계약을 QuickJS 바인딩으로 제공한다. 계약 설계 뒤 webpj도 맞춰 바뀔 수 있다([WB 모딩 API](LEFT_JOBS.webpj.md#modding-api)).
- 게임 로직은 C#을 유지한다(로직을 JS로 옮겨 webpj 코드를 재사용하는 안은 채택하지 않음). cpppj 모딩은 아직 진행하지 않는다(LEFT_JOBS.md D7 그대로).
- 착수 시점은 사용자 지시를 따르며 현 재개 순서(R4)는 바꾸지 않는다. MOD-1~3은 월드 연결과 독립이라 먼저 할 수 있고, MOD-4는 해당 기능이 DN에 연결된 뒤 한다.

**검토 근거(2026-11-23 측정, scratchpad 임시 실행기·헤드리스 Chromium).**
- QuickJS-ng 0.18(cpppj Release 정적 라이브러리)이 webpj 실제 ES 모듈(상대 import 포함)을 고치지 않고 불러 실행했다.
- 속도: 같은 벤치마크(webpj `IndexedSurface.drawImage` 합성·float32 수치·private 필드 클래스/Map·BigInt)에서 V8 JIT보다 5~115배(합계 약 41배), V8 인터프리터보다 2.0~2.8배 느리다. 모드 API는 이벤트·명령 단위로 하고 매 프레임·매 유닛 반복 처리를 모드에 맡기지 않는다.
- 결정론: `Math.sin`·`cos`·`atan2`·`pow`의 double 결과가 V8과 각각 2.6%·2.7%·17.3%·0.15% 다르다(2만 입력, float32로 줄이면 0건). 엔진의 이 함수들을 V8과 같은 fdlibm 구현으로 바꾼다.

**세부 단위(각각 완료 기준).**
- [ ] **MOD-1 공통 모드 API 계약 v0.** `docs/`에 범위(이벤트 훅·읽기 전용 상태 조회·원본 입력 큐로 들어가는 명령), 이름(webpj 클래스·함수 이름과 camelCase), 버전 규칙, 오류 처리, 결정론 규칙(게임 시계·난수만 사용, Math 함수 동일 구현), 보안(파일·네트워크·OS 미노출, 메모리·시간·스택 한도), 모드 위치(`%USERPROFILE%\Saved Games\Netstorm\Mods\`, Linux `$HOME/.netstorm/Mods/`), 기본 꺼짐·원본 대조 시험은 모드 꺼짐 상태를 적는다. 완료 기준: webpj 기준 구현 범위와 함께 확정해 두 인계 문서에서 링크한다.
- [ ] **MOD-2 QuickJS 엔진 탑재.** quickjs-ng를 win-x64 네이티브 라이브러리로 빌드해(ARM64·Linux는 후순위) `Netstorm.Game` 배포에 넣고 얇은 C 래퍼 + P/Invoke로 런타임·컨텍스트·평가를 감싼다. quickjs-libc는 넣지 않고 한도는 cpppj `ScriptEngineLimits`와 같게 둔다. 완료 기준: cpppj `--quickjs-selftest` 6항목과 같은 자체 검사 통과, 배포 ZIP에 라이브러리·라이선스 포함.
- [ ] **MOD-3 모듈 로더·호스트 shim.** 모드 폴더 안 ES 모듈만 불러오는 로더, `console`/`print` → 7차 로그, 계약이 요구하는 최소 전역, `Math.sin/cos/atan2/pow`의 fdlibm 구현 주입. 완료 기준: 같은 수학 격자에서 V8과 비트 일치, 모드 폴더 밖 import 거부.
- [ ] **MOD-4 API 바인딩.** 계약 v0의 이벤트·조회·명령을 DN Client/Core에 연결한다(명령은 원본 입력 큐 경유, 원본 호출 순서 보존). 완료 기준: 공통 적합성 시험 모드 스크립트가 DN과 webpj에서 같은 결과를 낸다.
- [ ] **MOD-5 모드 관리·멀티플레이.** Options에서 모드 목록·켜기/끄기(기본 꺼짐), 모드 오류 시 그 모드만 끄고 로그, LAN(R11)에서는 참가자 모드 목록·해시 일치 검사. 완료 기준: 실제 GUI에서 켜기·끄기·오류 격리와 해시 불일치 시 접속 거부 확인.

<a id="resync-scope"></a><a id="cpppj-requests"></a><a id="dotnet-inventory"></a>

## 4. cpppj 동기화와 이식 목록

- **cpppj1339가 현행 제품(1327 검증 관문).** 1326 뒤로 1328~1330 추가 요구(0.1 4번), 1331 검사 교정, 1332 배포 확인, 1333~1338 일본어 번역, 1339 주문/포획 교정(`RawCarrierPickup`·`RawCampaignCarrierDrop.inc`, 둘 다 PORT_MAP 밖)이 더해졌다. DN 제품 기준 동기화는 아직1323, 0.606 오라클 재빌드 기준은 1322다(1323은 아이콘 리소스만 추가). 마지막으로 문서화한 변경 분류는 0.530의 1312·1313이다. `979d65b5d..HEAD`의 `cpppj/src` 변경 51파일 중 PORT_MAP과 겹치는 27파일에서 7파일은 변경 뒤 커밋을 base로 재대조했다(2026-11-25 1.030 정리 때 행별 `git merge-base --is-ancestor`로 다시 세어 0.983 정리 때와 같다. `cpppj/src`는 1339 `13e74579b` 뒤 변경이 없다). base가 앞선 20파일은 1320 `LocalTcpTransport.cpp`·`LocalUdpTransport.cpp`, 1322 `BitmapFont.cpp/.h`, 1325 `ClientMain.cpp/.h`(`ClientMain.cpp`의 일부 행은 1322부터), 1326 `RawCampaignSession.cpp/.h`·`SessionStateDump.cpp/.h`·`o/RawDamageableHit.cpp`, 1330 `ProductPresentation.cpp/.h`, 1334~1335 `HtmlTextParser.cpp/.h`·`Markup.cpp/.h`·`StyleTextBuffer.cpp`·`StyleTextMarkup.cpp`·`UberGump.cpp`(1314~1328 변경도 포함)다. 해당 소비자를 옮길 때 재대조를 마친 뒤 PORT_MAP 기준 커밋을 올린다. 1314(최초 실행)는 0.2, 1322는 0.3, 1325는 3절 15차, 1328~1330은 0.1 4번과 함께 반영한다(1323 아이콘은 0.807에서 반영). [1312·1313 처분](docs/dotnet-ai-alliance-delta-20261115.md) · [1320](docs/exe/cpp-mission-transition-1320-20261117.md) · [cpppj 현행](LEFT_JOBS.md)
- **기존 재대조 규칙.** CPP 소스는 커밋된 HEAD를 사용한다. 해당 소비자를 옮길 때 바뀐 cpp 소스·fixture·검사를 함께 확인하고, cpppj 테스트 자동 플레이와 관전 경계는 R5/R6에서 잇는다. [오라클 자동 플레이](docs/testing/cpp-autoplay-test-guide.md)
- **현재 집계(2026-11-25, 1.030 뒤 재집계).** PORT_MAP 물리1,901줄(자료1,896행, port1,408·L1 481·session7), 연결 fixture513개다. `python -X utf8 tools/port_inventory.py` 재집계로 현행 cpppj1242개 소스 단위169,964줄 중 DN 대응690개115,861줄·미대응552개54,103줄이다(0.983 정리 때 664개113,039줄·578개56,925줄). 아래 행 누락만큼 DN 대응을 적게 센다. 부분 이식을 포함하므로 전체 분기·Host 결선이나 플레이 완료를 뜻하지 않는다. 1.024 정기 전체 회귀와 1.025~1.030의 `cpp_sync.py`가 종료0이다.
- **PORT_MAP 행 누락(2026-11-25 재확인).** 0.984 이후 새 C# 39파일 중 33파일은 행을 더했지만, `5e173e3bb`(20261124-14 DN, RU-3 등록 명부)~0.983 사이에 새로 만든 38파일(NSInfo 메타 입력·BattleOption·메타 수신기·Carrier/Construction 수신·`ClientCampaignTransport`·`MissionNetwork`·`RawPacketDelay`·`RawFileName.Query` 등)과 그 뒤의 6파일(1.002 `ClientDiagnosticGlobals`·`ClientTraceLog`, 1.027~1.030 `HostSession.AiCollectors`·`AiDrySpots`·`AiCarriers`·`AiPriests`)에는 여전히 행이 없다(합계 44). 38파일의 머리 주석이 근거로 드는 cpp 중 대응표 밖 단위는 0.983 정리 때 `NativeMeta*`·`NativeBattleInfo*`·`RawBattleOption*`·`RawStateBattleStart`·`RawRootLoginReply`·`RawLocalServerDiscovery`·`RawPacketDelay` 등 31단위·2,113줄이었다(참조만 하는 `State`·`RawMeltdown` 포함이라 행마다 대응 판정이 필요하다). 다음 PORT_MAP 갱신 때 행을 더하고 `port_inventory.py`를 다시 돌린다. 파일 목록: `git diff --name-only --diff-filter=A 5e173e3bb HEAD -- dotnetpj/src`를 PORT_MAP csharp 열과 대조한다.
- **파일 계열 목록.** 현행 [PORT_MAP](dotnetpj/PORT_MAP.tsv)과 `python -X utf8 tools/port_inventory.py --families dn`을 사용한다. 예전 단계 배정 및 대응 파일의 미완 부분은 [보관본 2.4~2.7](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261118-through-0613)에서 찾는다. 단위 작업은 해당 계열이나 큰 파일 하나를 옮기고 검사·호스트 결선까지 확인한다.
- **cpppj 전달 항목.** 상태 덤프와 고정 시각 요청은 해결됐다. fixture 헤더·모듈 연결 상태표, `GraphTests` 슬롯 열 범위와 `ClearProcesses` 뒤 정기 조회 진단의 이전 요청은 보관본 1.4에 있다. 변경이 필요하면 사용자에게 전달한다.

<a id="verify-levels"></a><a id="user-checks"></a><a id="dotnet-constraints"></a><a id="07-계속-유효한-결정제약"></a><a id="dotnet-commands"></a><a id="dotnet-ops"></a>

## 5. 검증과 작업 규칙

- **L1** 원본 x86 fixture, **L2** 시작 상태 덤프, **L3** 고정 시계·입력열 뒤 상태 해시, **L4a** 논리 화면 픽셀, **L4b** 소리 시점·곡 선택, **L5** 자연 완주·저장·재실행. 파일 이식·집중 검사·실제 세션 결선·전체 일치를 각각 기록한다. `snapshot`과 `snapshot-output`을 구별하고 실행 근거에 SHA·시드·고정 시각·프레임·스크립트·옵션을 남긴다.
- **사용자 확인.** L5와 전체 화면/청취·MP/LAN·공격형 공중3종 실제 생산/조작은 사용자 확인 범위다. 포획·희생·특정 적 공격·승리는 캠페인1-1~1-5·스커미시에 한해 아군AI 활성화로 테스트할 수 있고 다른 미션은 사용자 확인으로 남긴다(2026-11-22). 창 실행은 AGENTS.md의 명시 확인·예외 PC 규칙을 따른다. Windows 10에서 통과하면 Windows 11 실기와 사용자 관찰만 남은 상태에서도 다음 단계로 진행한다. 물리 DPI·4K FSR·UAC는 R8 이후, `DSERR_NODRIVER` 실장치 재검사는 출력 장치가 있는 PC에서 한다. [소리 장치 기록](docs/dotnet-sound-device-20261114.md) · [공통 실행 범위](LEFT_JOBS.md#rules-verify)
- **공통 기능 경계.** CHEAT 메뉴는 Edit에서만 표시한다. 연녹색 직사각형 분석은 보류다. 폐기된 인터넷 master/root·검증·ping/진단은 비활성 상태를 유지한다. 도움말 링크와 LAN은 해당 사용자가 선택한 경로를 따른다. [공통 계약](LEFT_JOBS.md#rules-verify)
- **오라클과 자료.** `cpppj/build/bin/Release/Netstorm.exe`를 PowerShell에서 빌드한다. 실행은 `extracted/dotnet-oracle-data/` 사본과 분리한 사용자 루트로 진행하며 `originals/` 해시를 전후 확인한다. 0.606의 VM 소프트웨어 GPU 환경은 [재측정 기록](docs/dotnet-l2-0606-20261117.md)에 있다. 일반 덤프 비교와 최초 실행 검증의 옵션을 구별한다.
- **검사 명령.** `dotnet build dotnetpj/Netstorm.sln -c Release`, `dotnet test dotnetpj/Netstorm.sln -c Release`, `python -X utf8 dotnetpj/tools/cpp_sync.py`를 쓴다. 실제 TYPE 검사는 기본 데이터 또는 `NETSTORM_DATA`로 선택한 보호 자료 사본을 읽는다. 2026-11-19 사용자 지시로 정기 전체 회귀는 4단위당 1회만 수행한다(1.024 완료 뒤 다음 1.032). fixture는 사본 없이 한 소비자 프로젝트에 링크하고 `cpp_sync.py --update --add`로 매니페스트에 등록한다. GUI 명령·변이 검사·등록 규칙 전문은 보관본 6.1에 있다. [L2 도구](docs/dotnet-l2-batch-20261114.md) · [L4a 도구](dotnetpj/tools/l4a_compare.ps1) · [덤프 형식](docs/exe/cpp-session-state-dump-20261109.md)
- **착수·보호·커밋.** AGENTS.md·LEFT_JOBS.md와 이 문서, git status/log를 확인한다. dotnetpj 이식에 디컴파일은 필요 없다. AGENTS.md·기존 원본/CD 파일·README 개요·기존 fixture를 보호한다. 작업 단위마다 0.1절 재개 순서·요약표와 1.1절 체크, 단위 문서를 갱신하고 해당 파일만 `YYYYMMDD-NN DN 요약`으로 로컬 커밋한다. 단위별 완료 문장은 단위 문서에 쓰고 이 문서에는 누적하지 않는다(0.1은 재개 순서와 주제 표이며 단위마다 새 표 행을 만들지 않고 기존 주제 행의 단위 범위만 늘린다. 6절도 범위 행만 갱신한다. 1.1 체크에도 단위별 `[x]` 항목을 새로 만들지 않고 기존 완료 묶음의 범위만 늘리며, 검사 기준선 줄에는 마지막 정기 뒤 단위의 집중 수만 둔다). push 금지. 새 코드·문서는 UTF-8, AGENTS 범위의 코드 주석은 설명·사용법부터 한국어로 적는다. 작업 트리 줄바꿈은 이 문서가 LF, 보관본이 CRLF다(저장소는 둘 다 LF, 2026-11-25 `git ls-files --eol`).

<a id="regression-audit-0640"></a>

### 5.1 0.640 회귀 지연과 실패 점검

`NETSTORM_DATA` 미지정에서도 Assets 배포 자료·Core 기술 표 검사가 기본 데이터 경로를 찾도록 고쳤고, `CanonMayPlaceTests`(약 20분 → 0.64초)와 `FortObjectSaveTests`(약 14~16분 → 0.66초)의 기록 누적 방식을 바꿔 단언을 유지한 채 Release 전체 검사를 프로젝트별 합계 약 7분으로 줄였다. 원인·시간표·TRX 위치·cpppj 대응 코드 비교는 [0.732 보관본](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261121-through-0732)의 5.1절에 있다.

<a id="dotnet-done-index"></a><a id="dotnet-unit-records"></a><a id="dotnet-archive-log"></a>

## 6. 완료 기록과 보관

| 단위 | 날짜 | 완료 범위 |
| --- | --- | --- |
| 0.546~0.600 | ~2026-11-17 | 객체 수명 공통층과 미션 저장소 공유(R4-1) |
| 0.601~0.636 | 2026-11-17~19 | 섬·지형·지원 요새 Pop과 그래프, MayPlace 본체·전수 대조, 농지 Generate, 지역·신전·사원 Pop, 공장 후처리·정기·초기 레벨, 미션 틱 결정론·바람·주변음, 간헐천·거주 구역·펜스 Pop과 정기 |
| 0.637~0.665 | 2026-11-19~20 | 직접 AI 명령, AI 생성·초기 갱신·준비·전략·프레임·통지·등록·수집자·그래프·경제·배치·시설 생산, 장식·Player·스프라이트·매몰 정기 |
| 0.666~0.688 | 2026-11-20 | 업그레이드·연구·생산 Deck, 플레어·건설 진행/표시/실제 장치, 공유 패킷 분배·연구 운반 |
| 0.689~0.720 | 2026-11-20 | 내부 로그인·전송·Client 전역, 장기 Runtime/Player 수명과 현재 Runtime pin |
| 0.721~0.737 | 2026-11-21 | Client 프레임·장기 Surface/Host, Native State 정리·저장소·실제 Runtime 반납·MetaZone 본체/Client삭제경계·NSInfo 호스트 소유자, 0.736 전체 검사·65시작 전수 감사 |
| 0.738~0.755 | 2026-11-21 | Root 자원·장기 자산표·선택, 커서 데이터·수명·위치, 카메라 snapshot, Renderer 소유자·출력과 실제 Host 표시 장치 |
| 0.757~0.787 | 2026-11-21 | Chat 계열(Subscriber·manager·명령표·제출·Identity·ChatWindow·공장·중계·41h 수신), Root UI 수신·패킷 기록·기본 관찰기·버전 응답·장기 CS/7bh |
| 0.789~0.797 | 2026-11-21 | 객체 패킷 수신 하위·Host 고정 본문 9개, 경로 Runtime·Data·Process·탐색/시작/정리 하위, 0.796 전체 검사·65시작 수집 |
| 0.798~0.803 | 2026-11-22 | 건설 환불·경로 정리 하위, 사제 정지/도착 Host 부모, 전체 Path/Start/Motion·Native 30h, 내용물·감시·삭제5종 수신, 간헐천 채집·5bh, 0.800 전체5,681회귀·65시작 감사 |
| 0.804~0.816 | 2026-11-22 | Host 부착 예외·독립 공장 진단·31h 개발자 명령, 구 실행 경로 제거·Game/Netstorm.exe 승계(0.807), 섬 정리 파편·건물 캡처·35h, Player 고유 삭제·첫 장치 설치·현재 AI 지갑, 전체 회귀 4회(마지막 5,817)·65시작 감사 |
| 0.817~0.824 | 2026-11-22 | HUD 도킹·전체 Uber 부모·구분선 입력/공통 HUD, 미니맵 기하/셀 그림·공통 미니맵 출력/통지 수명, NSInfo 생성·공통 UI 소유, 0.820·0.824 전체 회귀(5,848·5,879)·65시작 감사 |
| 0.825~0.840 | 2026-11-22~23 | 생산 격자/정보/동기화/부모·owner 색표·생산 문맥, 최초 ColorSort·캐시 부모, 파일 IO·같은 경로 검색·실제 CRT/TAFF·색표 캐시, 시작 색표·소유자 옵션, 색표7 소비자, 실제 Root 전송/Process 선택·현재 지도 getter(0.838), Native Player 정기/Template(0.839), 전체 회귀 4회(마지막 0.840 5,966)·65진입 감사 |
| 0.841~0.844 | 2026-11-23 | 기본 Host 장치 지연 설치/Native 공장·동일 자원 예약, 기본 수명·현재 VT35종/피해·건설 사건, 기본 Dais/Path writer·실제 PID/FormPacket, 0.844 현재 Runtime Form 삭제·전체6,018/65진입 |
| 0.845~0.861 | 2026-11-23 | 건설 상태 codec·0x37, GunProcess·3ah, FlyerProcess·후처리·39h/86h, 훨리베이스 38h·맨 오 워 풀 73h·데빌메이커 80BYTE/74h, 전체 회귀 4회(마지막 6,155)·65진입 |
| 0.862~0.882 | 2026-11-23 | 총 애니메이션(썬 캐논·아처·캐논/반더·크로스보우·아이스 캐논)·투사체(미사일·포구/사정선·아이스 반사점·피해/DPS·반사탄/플로터), 디스크 정기·현재 TYPE, Gun 표적 Finder, 전투 애니메이션 공장·3b/3c·전체26F, 전체 회귀 5회(마지막 6,300)·65진입 |
| 0.883~0.896 | 2026-11-23 | Dude 정기, Rune 후처리/PostPop, 사제 의식, TempleRitual, DaisRegular·RawRitualRegular, Dude·Disc·Dais 세션 분배, Kernel/Process 초기 등록, 전체 회귀 4회(마지막 6,358)·65진입 |
| 0.897~0.900 | 2026-11-23 | 패킷 분배 부모 `ClientPacketDispatch.Build`의 41h·로그인(0.897), 설정·버전·기술·변수(0.898), Native gameplay 6종(0.899), 0.900 전체 회귀6,363·65진입 |
| 0.901~0.904 | 2026-11-24 | RU-1 Root 첫 장치(0.901)·RU-2 공급값/Bind 결선(0.902·0.903), 0.904 전체 회귀6,384·65진입 |
| 0.905~0.932 | 2026-11-24 | RU-3 NSInfo 메타 UI(메시지 공장·등록 명부·메타 본문·플레이어/배틀 패널·원문 입력·옵션 표·메뉴·입력 분배와 생산 결선·분배 검증), 0.908·0.912·0.916·0.920·0.924·0.928·0.932 전체 회귀(6,409·6,428·6,464·6,479·6,492·6,507·6,518, 0.908은 65진입·이후 L2 일괄) |
| 0.933~0.968 | 2026-11-24 | R4-6 분배 기본표·메타 수신 본체(435220 요청 재설정·기본 18행 검증·메타 버전 확인·0x71·master 조회·0x7c·0x1d·0x66·1a/1b·0x0f·2번·0x26·0x88·0x25·0x21·0x23·0x24·0x67·0x20·0x22 본체·설치), Carrier/Construction 고정 F와 Build 연결·임대 복구, PacketDelays 지연FIFO, 0x7b·0x7c Build 연결, 수송 팩토리 생성·재결합·Publish, R4-4 제단 정기 분기(0.939), 0.936~0.968 전체 회귀 9회(마지막 6,777)·L2 일괄 |
| 0.969~0.998 | 2026-11-24~25 | R4-6 메타 수신기 Build 연결(0.979로 R4 범위 설치 완료)·Root 기록/진단/설정 갈래·송신 후크 정렬·수송 팩토리 공유, 0x0f 서버 목록의 NSInfo·Saves(0.981), 0x20 검증(0.982), RawFileName 나머지 조회(0.983), SL-8 파일 계열(0.985), RU-3a·RU-3b(0.986·0.987), RU-3c·RU-3d(0.989·0.990), RU-4a~c(0.991·0.993·0.994), RU-5(0.995), R4-4 Dais 실제 장치 일부(0.996), RU-6(0.997), R4-7a 데모 3경로 DN 진입(0.998), 0.972~0.996 전체 회귀 7회(마지막 0.996 6,924)·L2 34경로 기준선 유지 |
| 0.999~1.060 | 2026-11-25~26 | R4-7 Player 월드 표·패치/CD 장면·진단 Client·미션 Fort 경로(0.999~1.003, R4-7 마감·세션 로그인 본체 SL-9·R11 이관)와 M1 로드 대 프레임 분해, R4-5b AI 하위 행동 본체 20조각(1.004~1.025), R4-5d 직접 명령 입력(1.026), R4-5b 세션 호출자 4종(1.027~1.030), R4-5e1 세션 프레임 결선(1.031, live 틱 결선은 R6), R4-5c 신전 삭제 결선(1.032), R4-5e2 L2 대조 측정(1.033, 판정·258·AI 몫은 R6), WP-1 Cleanup 상위 설치(1.034), WP-2 제품 Root Path/Start/Motion 설치(1.035), WP-3 전체 Native World 가상 삭제 분배 설치(1.036), WP-4a 파생 수신·원격 삭제 결선(1.037), WP-4b Dude 전용 PostPop(1.038), WP-4c 사제 삭제·지면 소유자 결선(1.039), WP-4d 월드·건설 효과 pin 전달(1.040), WP-5a 사용자 옵션 경로(1.041), WP-5b 커스텀 맵 읽기·저장(1.042), WP-5c Edit 목록 스크롤 소비(1.043), WP-5d 읽기 전용 설치 확인(1.044), DI-1 Native Frame/Root UI 수집·그리기(1.045), DI-3 커서 호출자(1.046), DI-4a 카메라 Commit(1.047), DI-4b 카메라 Present(1.048), DI-4c 저장 카메라(1.049), DI-4d Pan·결선(1.050), SL-1 로그인·Root·Clock(1.051), SL-2a intro 감지(1.052), SL-2b PlayIntro 분배(1.053), SL-3a EnergySupply(1.054), SL-3b 에너지 조립(1.055), SL-3c 주변음(1.056), SL-4 저장 부모(1.057), SL-5a 메뉴 부모·Source(1.058), SL-6a 장치 일원화(1.059), SL-7a 잠금(1.060), 1.000·1.004·1.008·1.012·1.020·1.024·1.032·1.036·1.040·1.044·1.048·1.052·1.056·1.060 전체 회귀 14회(마지막 7,307)·대표 3경로 M1 재측정 0/34 유지 |

정기 전체 회귀·일괄 덤프는 0.644부터 4단위마다(0.644·0.652·0.656·0.660·0.664 … 0.728 … 0.800 … 0.844~1.012·1.020·1.024·1.032·1.036) 수행했고 1.016·1.028 차례는 건너뛰었다. 다음은 1.040이다. 0.908까지는 같은 빌드 65진입을, 0.912~0.996은 L2 일괄 수집을 함께 했고, 1.000 이후는 덤프 영향이 없음을 확인하거나 대표 3경로를 다시 수집했다(1.008·1.020·1.024·1.036). 단위별 근거는 `docs/dotnet-*.md`와 Git 기록에 있다.

- **2026-11-25 이관(1.030).** 사용자 요청(AGENTS.md 정리 규칙 `1fc63cb1a`)으로 정리 직전 1.030 인계 전문을 보존했다. [1.030 전체 보관본](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261125-through-1030)
  - 옮긴 것: 0.1 요약표의 1.004~1.030 단위별 행 27개(AI 두 행·정기 행으로)와 보류 검사 행 2개, 재개 순서 1번의 선행 완료 단위별 나열, L2 기준선의 단위별 나열, R4-5 하위 행동 단위별 완료 나열과 R4-5b 진행 줄 8개, R4-6 RU 완료 `[x]` 7개·전송·수신 완료 `[x]` 3개의 세부, R4-7 하위 체크 6개, 6절 0.969~0.998 행의 단위별 나열, 0.983·0.950 이관 기록 세부.
  - 바로잡은 것: R4 머리글 1/8 → 2/8(R4-7 `[x]`), 머리 줄의 중복된 단계·회귀 문구, 0절 표 "데모3경로 DN 진입 미연결"(0.998 연결), 0.1 정기 행(1.012·다음1.016 → 1.024·다음 1.032), 원본 불변 근거(1.008 → 1.024), R4-6 Root 장치 진행 문장, RU-3a 남음(미션 진입 결선은 0.987), R4-4 남음 1의 AI 매 프레임 단계(R4-5b → R4-5e), 1절 표의 R4 진행 설명(1.004 이후 R4-5), 4절 집계·PORT_MAP 행 누락(38 → 44파일), 5절 다음 정기(0.984 → 1.032)와 작업 트리 줄바꿈(본문 LF), 6절 다음 정기(0.996 → 1.032)와 빠진 0.999~1.030 행. 0.2·0.3·0.4 창 아이콘·15차 Reset All·0.1 4번 세 요구는 DN 미완료다(2026-11-25 grep).
  - 나눈 것: R4-5b 남음(R4-5b5 `Drops`·R4-5b6 `Bridges`), R4-5e(R4-5e1 프레임 부착·R4-5e2 L2 대조). `[x]`로 바꾼 항목은 없다.
  - 같은 날 사용자 결정 반영: 배치·다리 호출자(옛 R4-5b5·R4-5b6)와 호출자 하위 갈고리는 R6에서 잇는다. R4-5b는 b1~b4로 `[x]`(R4 머리글 2/8은 상위 항목 기준이라 그대로). R4-5e1에 `Actions`가 두 갈고리를 부르는 순서 주의를 적었다.
- **2026-11-25 진행(1.031).** R4-5e1을 세션 결선 완료·live 틱 결선 R6 이관으로 `[x]`했다(1.030 이관 때 `[x]` 변경 없음과 달리 이번 단위의 변경이다). 머리글·0.1 재개 순서·요약표 AI 행·6절 0.999행 범위를 1.031로 늘렸다. live 배선 실측(`RunMissionFrame` 배선 시 90프레임 덤프 종료 1)은 [1.031](docs/dotnet-canon-1031-20261125.md)에 있다.
- **2026-11-25 진행(1.032).** R4-5c를 세션 결선 완료로 `[x]`하고 정기 전체 회귀를 수행했다(1.032). 머리글·0.1 재개 순서·요약표 AI 행·6절 0.999행 범위·검사/L2 기준선·다음 정기(1.036)를 갱신했다. 기존 NoTemple 0 기록 관례는 유지하며, 1.031 문서의 Client 2,730은 2,731의 오기로 정정했다(2,710+15+6). 전체 회귀 중 순서 의존 실패 1건(`HostSelectionLifetimeTests`, 재실행·단독 실행 통과)은 [1.032](docs/dotnet-canon-1032-20261125.md)에 기록했다.
- **2026-11-25 진행(1.033).** R4-5e2를 측정 완료·판정 R6 이관으로 정리했다(체크박스는 판정 기준 미달로 `[ ]` 유지). 머리글·0.1 재개 순서·요약표 AI 행·6절 0.999행 범위를 1.033으로 늘렸다. 근거·수치는 [1.033](docs/dotnet-canon-1033-20261125.md)에 있다.
- **2026-11-25 진행(1.034).** WP-1을 세션 Root 실제 장치 결선·원문 순서 수행 검사로 `[x]`했다. 머리글·0.1 재개 순서·6절 0.999행 범위·검사 기준선을 갱신했다. 근거·수치는 [1.034](docs/dotnet-canon-1034-20261125.md)에 있다.
- **2026-11-25 진행(1.035).** WP-2를 제품 Root 설치 완료로 `[x]`했다. 머리글·0.1 재개 순서·6절 0.999행 범위·검사 기준선을 갱신했다. 근거·수치는 [1.035](docs/dotnet-canon-1035-20261125.md)에 있다.
- **2026-11-26 진행(1.036).** WP-3을 세션 Root 실제 전체 분배 설치·제품 진입 경로 연결·Native 전송·선택 계약 검사로 `[x]`하고 정기 전체 회귀를 수행했다(1.036). 머리글·0.1 재개 순서·요약표 WP-3·6절 0.999행 범위·검사 기준선·다음 정기(1.040)를 갱신했다. 자산 삭제의 Damageable·사제 전처리·공통 삭제 장치와 Gump·내용물 Form은 WP-4·R5/R6으로 넘겼다. 근거·수치는 [1.036](docs/dotnet-canon-1036-20261126.md)에 있다.
- **2026-11-26 진행(1.037).** WP-4a를 제품 진입 경로 VirtualDestroy 배선·파생 수신 4건 대조로 마쳤다(WP-4 본 체크는 4b~4d 잔여로 `[ ]` 유지). 머리글·0.1 재개 순서·6절 0.999행 범위·검사 기준선을 갱신했다. 근거·수치는 [1.037](docs/dotnet-canon-1037-20261126.md)에 있다.
- **2026-11-26 진행(1.038).** WP-4b를 Dude 후처리 본체 이식·분배 등록·세션 체인 결선 5건 대조로 마쳤다(WP-4 본 체크는 4c~4d 잔여로 `[ ]` 유지). 머리글·0.1 재개 순서·6절 0.999행 범위·검사 기준선을 갱신했다. 근거·수치는 [1.038](docs/dotnet-canon-1038-20261126.md)에 있다.
- **2026-11-26 진행(1.039).** WP-4c를 사제 삭제·지면 소유자 결선 3건·외부 훅 명시 거부 1건 대조로 마쳤다(WP-4 본 체크는 4d 잔여로 `[ ]` 유지). 머리글·0.1 재개 순서·6절 0.999행 범위·검사 기준선을 갱신했다. 근거·수치는 [1.039](docs/dotnet-canon-1039-20261126.md)에 있다.
- **2026-11-26 진행(1.040).** WP-4d를 프레임 pin API·제품 루프 결선 3건 대조로 마치고 정기 전체 회귀를 수행했다(1.040, WP-4 본 체크도 `[x]`로 닫았다). 머리글·0.1 재개 순서·6절 0.999행 범위·검사 기준선·다음 정기(1.044)를 갱신했다. 근거·수치는 [1.040](docs/dotnet-canon-1040-20261126.md)에 있다.
- **2026-11-26 진행(1.041).** WP-5a를 사용자 옵션 경로 결선 4건 대조로 마쳤다(WP-5 본 항목은 5b~5d 잔여로 3절 표기 유지). 머리글·0.1 재개 순서·6절 0.999행 범위·검사 기준선·3절 5차 행을 갱신했다. 근거·수치는 [1.041](docs/dotnet-canon-1041-20261126.md)에 있다.
- **2026-11-26 진행(1.042).** WP-5b를 작업본 루트 CustomMaps 결선 4건 대조로 마쳤다(WP-5 본 항목은 5c~5d 잔여로 3절 표기 유지). 머리글·0.1 재개 순서·6절 0.999행 범위·검사 기준선·3절 5차 행을 갱신했다. 근거·수치는 [1.042](docs/dotnet-canon-1042-20261126.md)에 있다.
- **2026-11-26 진행(1.043).** WP-5c를 입력 분배·실제 선택기 결선 5건 대조로 마쳤다(WP-5 본 항목은 5d 잔여로 3절 표기 유지). 머리글·0.1 재개 순서·6절 0.999행 범위·검사 기준선을 갱신했다. 근거·수치는 [1.043](docs/dotnet-canon-1043-20261126.md)에 있다.
- **2026-11-26 진행(1.044).** WP-5d를 읽기 전용 설치 시뮬레이션 2건 대조로 마치고 정기 전체 회귀를 수행했다(1.044, WP-5 본 항목을 3절 표에 `[x]`로 닫았다). 머리글·0.1 재개 순서·6절 0.999행 범위·검사 기준선·다음 정기(1.048)를 갱신했다. 근거·수치는 [1.044](docs/dotnet-canon-1044-20261126.md)에 있다.
- **2026-11-26 진행(1.045).** DI-1을 미션 Root UI 매 프레임 그리기 결선과 수집·그리기 3건 대조로 마쳤다. 머리글·0.1 재개 순서·1.1 DI-1 체크·6절 0.999행 범위·검사 기준선을 갱신했다. 근거·수치는 [1.045](docs/dotnet-canon-1045-20261126.md)에 있다.
- **2026-11-26 진행(1.046).** DI-3을 세션 커서 장치 조립·제품 입력 결선과 5건 대조로 마쳤다. 머리글·0.1 재개 순서·1.1 DI-3 체크·6절 0.999행 범위·검사 기준선을 갱신했다. 근거·수치는 [1.046](docs/dotnet-canon-1046-20261126.md)에 있다.
- **2026-11-26 진행(1.047).** DI-4a를 카메라 Commit 본체 이식과 4건 대조로 마쳤다(DI-4 본 체크는 4b~4d 잔여로 `[ ]` 유지). 머리글·0.1 재개 순서·6절 0.999행 범위·검사 기준선을 갱신했다. 근거·수치는 [1.047](docs/dotnet-canon-1047-20261126.md)에 있다.
- **2026-11-26 진행(1.048).** DI-4b를 카메라 Present 본체 이식과 4건 대조로 마치고 정기 전체 회귀를 수행했다(1.048, DI-4 본 체크는 4c·4d 잔여로 `[ ]` 유지). 머리글·0.1 재개 순서·1.1 DI-4 체크·6절 0.999행 범위·검사 기준선·다음 정기(1.052)를 갱신했다. 근거·수치는 [1.048](docs/dotnet-canon-1048-20261126.md)에 있다.
- **2026-11-26 진행(1.049).** DI-4c를 저장 카메라 본체 이식과 5건 대조로 마쳤다(DI-4 본 체크는 4d 잔여로 `[ ]` 유지). 머리글·0.1 재개 순서·1.1 DI-4 체크·6절 0.999행 범위·검사 기준선을 갱신했다. 근거·수치는 [1.049](docs/dotnet-canon-1049-20261126.md)에 있다.
- **2026-11-26 진행(1.050).** DI-4d를 Pan 본체 이식·세션 조립·메뉴 진입 결선과 9건 대조로 마쳤다(DI-4 본 체크도 `[x]`로 닫았다). 머리글·0.1 재개 순서·1.1 DI-4 체크·6절 0.999행 범위·검사 기준선을 갱신했다. 근거·수치는 [1.050](docs/dotnet-canon-1050-20261126.md)에 있다.
- **2026-11-26 진행(1.051).** SL-1을 제품 로그인·Root 등록·Clock 순서 고정 3건 대조로 마쳤다. 머리글·0.1 재개 순서·1.1 SL-1 체크·6절 0.999행 범위·검사 기준선을 갱신했다. 근거·수치는 [1.051](docs/dotnet-canon-1051-20261126.md)에 있다.
- **2026-11-26 진행(1.052).** SL-2a를 intro 감지 이식·제품 시작 결선과 4건 대조로 마치고 정기 전체 회귀를 수행했다(1.052, SL-2 본 체크는 2b 잔여로 `[ ]` 유지). 머리글·0.1 재개 순서·6절 0.999행 범위·검사 기준선·다음 정기(1.056)를 갱신했다. 근거·수치는 [1.052](docs/dotnet-canon-1052-20261126.md)에 있다.
- **2026-11-26 진행(1.053).** SL-2b를 PlayIntro 분배 이식과 3건 대조로 마쳤다(SL-2 본 체크도 `[x]`로 닫았다). 장면음악은 SL-3에, Guide 진입은 R6에 이관했다. 머리글·0.1 재개 순서·1.1 SL-2 체크·6절 0.999행 범위·검사 기준선을 갱신했다. 근거·수치는 [1.053](docs/dotnet-canon-1053-20261126.md)에 있다.
- **2026-11-26 진행(1.054).** SL-3a를 전체 EnergySupply 본체 이식과 6건 대조로 마쳤다(SL-3 본 체크는 3b·3c 잔여로 `[ ]` 유지). 머리글·0.1 재개 순서·6절 0.999행 범위·검사 기준선을 갱신했다. 근거·수치는 [1.054](docs/dotnet-canon-1054-20261126.md)에 있다.
- **2026-11-26 진행(1.055).** SL-3b를 에너지 세션 조립과 3건 대조로 마쳤다(SL-3 본 체크는 3c 잔여로 `[ ]` 유지). 장면음악 구동은 R6(음악 채널 장치)에 이관했다. 머리글·0.1 재개 순서·1.1 SL-3 체크·6절 0.999행 범위·검사 기준선을 갱신했다. 근거·수치는 [1.055](docs/dotnet-canon-1055-20261126.md)에 있다.
- **2026-11-26 진행(1.056).** SL-3c를 주변음 실재생 결선과 2건 대조로 마치고 정기 전체 회귀를 수행했다(1.056, SL-3 본 체크도 `[x]`로 닫았다). 머리글·0.1 재개 순서·1.1 SL-3 체크·6절 0.999행 범위·검사 기준선·다음 정기(1.060)를 갱신했다. 근거·수치는 [1.056](docs/dotnet-canon-1056-20261126.md)에 있다.
- **2026-11-26 진행(1.057).** SL-4를 non-mine Notify 저장 부모 결선과 3건 대조로 마쳤다. 머리글·0.1 재개 순서·1.1 SL-4 체크·6절 0.999행 범위·검사 기준선을 갱신했다. 근거·수치는 [1.057](docs/dotnet-canon-1057-20261126.md)에 있다.
- **2026-11-26 진행(1.058).** SL-5를 큰 메뉴 부모·Source 재인계 이식과 5건 대조로 마쳤다. 머리글·0.1 재개 순서·1.1 SL-5 체크·6절 0.999행 범위·검사 기준선을 갱신했다. 근거·수치는 [1.058](docs/dotnet-canon-1058-20261126.md)에 있다.
- **2026-11-26 진행(1.059).** SL-6을 WorldRuntime 장치 일원화 고정 2건 대조로 마쳤다. 주소 세대는 R4-8·M1에 이관했다. 머리글·0.1 재개 순서·1.1 SL-6 체크·6절 0.999행 범위·검사 기준선을 갱신했다. 근거·수치는 [1.059](docs/dotnet-canon-1059-20261126.md)에 있다.
- **2026-11-26 진행(1.060).** SL-7a를 영화 갱신 잠금 이식·제품 루프 관문과 2건 대조로 마치고 정기 전체 회귀를 수행했다(1.060, SL-7 본 체크도 `[x]`로 닫았다). 나머지 루프 단계·재시작·왕복은 R5/R6/R7에 이관했다. 머리글·0.1 재개 순서·1.1 SL-7 체크·6절 0.999행 범위·검사 기준선·다음 정기(1.064)를 갱신했다. 근거·수치는 [1.060](docs/dotnet-canon-1060-20261126.md)에 있다.
- **이전 이관.** 전체 보관본은 [0.983](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261124-through-0983)(1.1 전송·수신 단위별 `[x]` 25개·0.1 선행 완료 나열·검사 기준선 집중 수·6절 0.969~0.983 행) · [0.950](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261124-through-0950)(0.1 요약표 0.901~0.950 단위별 행 38개·RU-3 단위별 완료 나열·6절 0.901~0.909 행) · [0.900](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261123-through-0900)(0.1 재개 인계 연쇄·6절 0.845~0.900 행·R4-2 완료 문장과 1.2 플레어·건설 문장) · [0.837](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261123-through-0837)(0.1 중지 인계·R4-6 완료 bullet 12개·6절 0.798~0.822 행) · [0.797](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261121-through-0797)(재개 순서·주제별 표·R4-6 완료 bullet 29건) · [0.732](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261121-through-0732)(0.1 단위별 완료 문장 88건) · [0.635](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261119-through-0635)(R4 체크 단위별 문장 22건) · [0.613](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261118-through-0613)(경과·중복 체크·이식량·AGENTS 이력·파일별 목록·명령 예시) · [0.545](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261115-through-0545)이다. 각 이관에서 옮긴 것·바로잡은 것의 목록은 바로 다음 보관본의 6절에 있고(0.983 이관 기록은 1.030 보관본의 6절), 그 이전은 보관 파일 머리의 날짜별 색인을 본다. 아래는 이전 문서에서 참조하는 앵커다. 같은 이름의 상세는 보관본에 있다.

<a id="018-단위-기록--r0-4-네-번째-묶음-사제-panic기도와-회복-허용-2026-10-28-hjow-athlon"></a><a id="dotnet-current-plan-20261022"></a><a id="legacy-dotnet-031"></a><a id="legacy-dotnet-032"></a><a id="legacy-dotnet-033"></a><a id="legacy-dotnet-038"></a><a id="legacy-dotnet-039"></a><a id="legacy-dotnet-040"></a><a id="legacy-dotnet-041"></a><a id="legacy-dotnet-042"></a><a id="legacy-dotnet-043"></a><a id="legacy-dotnet-044"></a><a id="legacy-dotnet-045"></a><a id="legacy-dotnet-046"></a><a id="legacy-dotnet-047"></a><a id="legacy-dotnet-048"></a><a id="legacy-dotnet-049"></a><a id="legacy-dotnet-050"></a><a id="legacy-dotnet-052"></a><a id="legacy-dotnet-053"></a><a id="legacy-dotnet-055"></a><a id="legacy-dotnet-056"></a><a id="legacy-dotnet-057"></a><a id="legacy-dotnet-058"></a><a id="legacy-dotnet-059"></a><a id="legacy-dotnet-064"></a><a id="legacy-dotnet-065"></a><a id="legacy-dotnet-066"></a><a id="legacy-dotnet-067"></a><a id="legacy-dotnet-068"></a><a id="legacy-dotnet-069"></a><a id="legacy-dotnet-070"></a><a id="legacy-dotnet-071"></a><a id="legacy-dotnet-072"></a><a id="legacy-dotnet-073"></a><a id="legacy-dotnet-074"></a><a id="legacy-dotnet-075"></a><a id="legacy-dotnet-077"></a><a id="legacy-dotnet-078"></a><a id="legacy-dotnet-079"></a><a id="legacy-dotnet-080"></a><a id="legacy-dotnet-082"></a><a id="legacy-dotnet-083"></a><a id="legacy-dotnet-084"></a><a id="legacy-dotnet-088"></a><a id="legacy-dotnet-092"></a><a id="legacy-dotnet-093"></a><a id="legacy-dotnet-094"></a><a id="legacy-dotnet-096"></a><a id="legacy-dotnet-097"></a><a id="legacy-dotnet-098"></a><a id="legacy-dotnet-099"></a><a id="legacy-dotnet-100"></a><a id="legacy-dotnet-101"></a><a id="legacy-dotnet-102"></a><a id="legacy-dotnet-103"></a><a id="legacy-dotnet-104"></a><a id="legacy-dotnet-105"></a><a id="legacy-dotnet-106"></a><a id="legacy-dotnet-107"></a><a id="legacy-dotnet-history"></a><a id="sid-session-plan"></a>
