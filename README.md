Netstorm - Reborn
-------------------------------------------------------------------------

**한국어** | [English](README.en.md)

# 개요
Activision 사에서 1997년도에 개발 후 한참 전에 손 놓은    
Netstorm - Islands at war 게임을    
AI 를 이용해 되살리는 프로젝트입니다.
    
    
최우선 목표는 일단 게임이 윈도우 10, 11 에서도 버그 없이, 기존 게임과 거의 동일한 경험을 제공하도록 합니다.
    
     
게임 부활 작업은 3가지 서브프로젝트로 나뉘어 진행됩니다.
- cpppj - C++ 기반으로 개발, 기존 게임 또한 C++ 개발이 추정되는 만큼 최대한 기존 게임 참조, Windows 전용
- dotnetpj - C# Mono 기반으로 개발, cpppj 진행으로 얻은 소스코드를 참고하여 닷넷 플랫폼으로 포팅, 다양한 플랫폼 지원
- webpj - HTML/JavaScript 기반으로 개발, 웹에서 캠페인을 편리하게 경험할 수 있도록 개발, 웹 브라우저가 지원되는 모든 플랫폼을 지원하나, 일부 기능은 구현하지 않을 예정

# 현재 개발 진행상황

**2026-11-06 기준:** 주력인 `cpppj`는 사용자 요청에 따라 단위986에서 일시중지했습니다(실행 제품은 단위985, 표시 11.01). 그동안 4차(로컬 MCP·16:9 최대 1920×1080), 5차(사용자 홈 저장 경로), 7차(안티앨리어싱·FSR 1.0)를 구현했습니다. 6차(한국어)는 1차 구현까지 마쳤고, 3차(LAN 멀티플레이)는 전송 부품과 로그인 부모를 사전 구현하고 있습니다. `webpj`는 WB04-D5a(2026-11-06)까지 미션 진입에 필요한 월드 핵심 부품을 이식하고 있고, `dotnetpj`는 단위 0.154(2026-11-04)까지 Fort 적재 부품을 이식했습니다. 단위별 근거는 [cpppj·공통 인계](LEFT_JOBS.md), [dotnetpj 인계](LEFT_JOBS.dotnetpj.md), [webpj 인계](LEFT_JOBS.webpj.md)에 있습니다.

| 프로젝트 | 실행·플레이 가능 범위 | 원본과의 동일성 및 남은 범위 |
| --- | --- | --- |
| **cpppj — C++ 복원판 (주력)** | **플레이 가능.** Early Missions 6개·Priest Training 9개·캠페인 16개·데모 3개, 모두 34경로에 진입해 진행할 수 있고 메뉴·덱·미니맵·Edit·전투/AI가 연결돼 있습니다. 사용자가 캠페인 1-1·1-2 완주를 확인했고, 세 데모는 360초 주기를 두 번 지나 세 번째 월드까지 반복함을 확인했습니다(946~948). 한국어 메뉴(Options > Language), 1920×1080까지의 16:9 해상도, 선택형 안티앨리어싱·FSR 1.0, `--mcp` 로컬 MCP 서버를 쓸 수 있습니다. | 부품마다 원본 10.78 기계어와 대조해 복원하고 있으며, 비멀티 UI(929)·Edit(943)·미션 수명(956)·내부 수명/파일(980)의 구현·대조를 정리했습니다. 전체 동일성과 모든 미션의 자연 완주는 사용자 확인 대상이고, 34경로 전체를 마지막으로 다시 확인한 것은 단위645(제품644)입니다. 남은 것은 3차 멀티플레이(LAN UI·월드 동기화·일반 전투·Outpust), 6차 한국어 본문(미션·팁·도움말)과 한글 버튼 그림, 8차 Edit 시나리오 편집 UI입니다. |
| dotnetpj — C# + MonoGame 클론 | 기존 경로(`Netstorm.exe`)에서 캠페인 **1-1 전쟁의 시작!(The War Begins!)**, **1-2 훨리기그의 지배자(Master of Whirligigs)**를 근사 구현으로 플레이할 수 있습니다. 영어·한국어 UI, 도움말·우클릭 메뉴·옵션을 제공합니다. 새 경로 호스트(`Netstorm.Client.Host`)는 타이틀·메인 메뉴 등 메뉴 화면 일부까지이며 미션은 아직 없습니다. | 2026-10-28에 세운 계획(R0~R8)에 따라 cpppj 구조를 그대로 옮긴 새 `Netstorm.Client` 경로로 교체하고 있습니다. R0·R1·R2를 마쳤고, R3(Gump UI·메뉴)는 진행 중이며, R4(월드 세션)는 Template 저장소(TS0~TS6)와 Fort 적재 단계 F1~F2c-11까지 이식했습니다. 11.01 표기·Credits 3항목, 외부 접속 문서화·유효성 검사 비활성화, 실행 파일 이름 `Netstorm.exe`는 적용했습니다. 0.154의 Release 검사는 Core 1,243개·Client 858개·Assets 366개 통과입니다. 메뉴 화면 캡처 비교(D1)와 새 경로의 실제 미션 연결이 아직이라 원본 동일성은 기존 1-1·1-2 근사 수준입니다. |
| webpj — HTML + JavaScript 웹 클론 | "클릭하여 시작" 뒤 원본 자산으로 메인 메뉴를 표시합니다. Campaign·Demo·Credits(11.01 Patch Credits 포함 3항목) 대화상자와 목록, Help·Options 메뉴(설정 저장), 그림 아이콘을 포함한 F1 도움말 창, 시작 환영/팁 창, 캠페인 진행 값 저장을 구현했습니다. 브라우저 언어가 한국어면 메뉴·대화상자·F1 도움말(주제 128개 중 126개)이 D2Coding 글꼴로 한국어로 표시됩니다. **실제 미션 플레이는 아직 할 수 없으며**, 게임 규칙 없는 개발용 지형 미리보기(`?map=`)만 있습니다. | 메뉴·대화상자·도움말 창을 원본 스크린샷과 대조했습니다(도움말 창은 사실상 픽셀 일치). 미션 진입을 위해 Fort 판독·지형 계산과 월드 핵심 부품(SID 풀·공간 해시·난수·게임 시계·Kernel·입력 큐·메인 루프·Unpop/삭제·Form 층·SquidFactory·Form 호스트, WB04-A~D5a)을 원본 10.78 자료와 대조하며 이식했지만 아직 앱 수명에 연결하지 않았습니다. 자동 검사는 429개 통과입니다(WB04-D5a). 남은 것은 WB04-D5b~E, 실제 Fort Load→Start, 미션 화면·플레이 규칙, 바깥 링크 새 탭 열기, 미션 문구 한국어입니다. 웹에서는 Edit·User Made Campaigns·멀티플레이를 제공하지 않습니다. |

## C++ 복원판(cpppj)의 최신 상태

### 지금 할 수 있는 것

- **미션 34경로:** Early Missions 6개, Priest Training 9개, 캠페인 16개(1·2·3장), 데모 3개에 모두 진입해 진행합니다. 캠페인 1-1·1-2는 사용자가 직접 끝까지 플레이해 다음 미션 안내까지 정상 동작함을 확인했습니다. 세 데모는 360초가 끝나면 새 월드로 다시 시작하는 반복을 두 번 거쳐 세 번째 월드와 Exit Demo까지 확인했습니다.
- **원본 메뉴와 연출:** 메인 메뉴의 Campaign·Demo·Help·Options·Credits·Quit, 브리핑, CD 인트로 영상(SMK, 영어·독일어)을 원본 구조대로 재현합니다. Credits에는 세 번째 항목 **Netstorm 11.01 Patch Credits**가 있습니다.
- **게임 규칙:** 건설·생산·다리·채취/운반·전투, 워크샾 유닛 25종, 포획·희생·SP·미션 완료를 포함한 의식 규칙, 기술 획득, 적·아군·데모가 같은 원본 전략을 쓰는 공통 AI, 세계 간헐천 생성을 원문 대조로 복원했습니다.
- **게임 중 UI:** 덱·미니맵, 건물 단축키 QWASZX, ESC 메뉴, 채팅(Enter), 도움말 링크, 건물·운반체·특수 대상의 우클릭 메뉴와 대상별 커서, 4:3·16:9·16:10 화면비와 가장자리 스크롤을 연결했습니다.
- **Edit:** 기존 맵 불러오기·저장·다른 이름 저장, 신규 섬 추가, 시험 전투(Test Battle) 후 편집 복귀, 방향별 동맹·AI 설정 등 시나리오 값을 지원합니다. 맵 목록은 개수 제한 없이 50행씩 스크롤하며, 새 맵은 사용자 홈의 `CustomMaps`에 저장합니다.
- **저장 위치(5차):** 설정·진행·캡처·진단·편집 작업본 등 게임이 만드는 파일을 모두 `%USERPROFILE%\Saved Games\Netstorm\` 아래(`Saves`·`CustomMaps`·`Editor`·`Captures`·`Diagnostics`)에 저장합니다. 관리자 권한 없이는 쓸 수 없는 폴더에 설치해도 동작함을 확인했습니다(932~934).
- **로컬 MCP(4차):** `Netstorm.exe --mcp`로 게임을 stdio MCP 서버로 실행해 조작·상태 조회·화면 PNG·프레임 대기·소리 포함 녹화를 할 수 있습니다(935~936, [MCP 기록](docs/exe/cpp-mcp-server-20261106.md)).
- **해상도·후처리(4·7차):** 16:9 해상도 1600×900·1920×1080을 지원합니다(937). Options의 Anti-Aliasing·FSR 1.0 Upscaling은 기본으로 꺼져 있고, Direct3D 11 장치가 있으면 2560×1440·3840×2160을 고를 수 있습니다. 이 두 해상도는 1920×1080으로 그린 화면을 FSR 1.0으로 업스케일합니다(939).
- **한국어(6차 1차):** Options > Language에서 한국어를 고르면 D2Coding 글꼴과 번역표(502항목)로 메뉴·창 문구가 한국어로 바뀝니다. 한국어 미션 파일이 없으면 영어 파일로 대신 진행합니다(938).
- **Windows 10/11 대응:** 원본에서 전체화면 종료 후 재실행하면 오류가 나던 문제가 생기지 않도록 구현했고, 저장된 전체화면 상태에서 다시 실행해 메인 메뉴까지 정상 진입함을 확인했습니다(단위 563·588). 배포판은 압축을 풀고 바로 실행합니다.

### 최근 진행

- **986 (2026-11-06, 일시중지):** 진행 중이던 한국어 본문·시나리오 UI 준비본과 원격 Fort 분석을 [추적 인계 자료](docs/exe/handoffs/cpp-pause-20261106/README.md)로 보존하고, 사용자 요청으로 목표를 일시중지했습니다. 준비본은 제품에 반영하지 않았고 실행 제품은 985 그대로입니다([986 기록](docs/exe/cpp-pause-handoff-20261106.md)).
- **982~985 (3차 LAN 사전 구현):** 원문 프레이밍 기반 LAN TCP 전송 부품(982), 공유 전송 등록표와 실제 월드 콜백 수명의 접합(984), 일반 로그인 부모와 초기 SP 지정 분기(985)를 원문 근거로 사전 구현했습니다. 정책에 따라 실제 통신·멀티플레이 검사는 하지 않았으며, LAN UI·참가자 목록·일반 전투는 아직 연결하지 않았습니다. 983에서는 MCP 검사의 VS 2022 컴파일 오류 제보를 고쳤습니다([982](docs/exe/cpp-lan-tcp-transport-20261106.md)·[983](docs/exe/cpp-mcp-test-compile-20261106.md)·[984](docs/exe/cpp-shared-transport-lifetime-20261106.md)·[985](docs/exe/cpp-ordinary-login-parent-20261106.md)).
- **981:** 높이 774를 넘는 해상도에서 좌측 덱에 보이던 검은 띠를 고쳤습니다([981](docs/exe/cpp-wide-deck-background-20261106.md)).
- **957~980 (내부 수명·파일):** 공통 Pop·Factory·Form·Process·Kernel·SID 할당 회수·색표 파일·옵션 저장 실패 복구·메모리 실패 처리기와 종료 정리를 실제 Client/World에 연결해, 활성 비멀티 범위를 마감했습니다. 서비스가 끝난 CD 검증 프로토콜은 정책대로 비활성화했습니다(965). [980 정리](docs/exe/cpp-internal-lifetime-closure-20261106.md)를 봅니다.
- **944~956 (미션 수명):** Retry/Next/Abort와 연속 전환, Early1의 계속·재시작·중단·메인 연속 왕복, 세 데모의 두 번째 360초 주기와 세 번째 월드, LeaveBattle·State 전환·정리를 대조했습니다([945](docs/exe/cpp-mission-abort-replay-20261106.md)·[948](docs/exe/cpp-demo-second-alliance-20261106.md)·[956](docs/exe/cpp-mission-state-scope-20261106.md)).
- **940~943 (Edit·시나리오):** 동적 시나리오 설정·TARC, 저장 공유 거부 뒤 재저장, 객체 적재, 편집 도구 수명을 대조해 Edit 구현을 정리했습니다([943](docs/exe/cpp-editor-implementation-closure-20261106.md)).
- **932~939 (4~7차):** 사용자 홈 저장 경로·Edit 목록 무제한(932~934), 로컬 MCP 서버와 소리 포함 녹화(935~936), 16:9 해상도(937), 한국어 1차(938), 안티앨리어싱·FSR 1.0(939)을 구현하고 실제 실행으로 확인했습니다([5차](docs/exe/cpp-user-data-root-20261105.md)·[MCP](docs/exe/cpp-mcp-server-20261106.md)·[해상도](docs/exe/cpp-wide-resolution-20261106.md)·[한국어](docs/exe/cpp-korean-language-20261106.md)·[AA/FSR](docs/exe/cpp-aa-fsr-20261106.md)).
- **816~931 (UI·월드 부품):** Graph·표면·군도 생성·바람/입자/날씨·연소·마나 공급 등 월드 부품, 생성자·HP·Form 수명, 건물·운반체·특수 대상의 우클릭 메뉴와 커서(913~918), 게임 메뉴·HUD·Chat·모달 수명(920~928)을 원문과 대조해 연결했습니다. 비멀티 UI(929)와 원문 언어 처리(931)도 정리했습니다([929](docs/exe/cpp-ui-implementation-closure-20261105.md)).
- **그 전 (~815):** 11.01 표기와 세 번째 Patch Credits(685), 외부 접속 목록화와 클라이언트 유효성 검사 비활성화(684), 마지막 전체 34경로 확인(645), 간헐천 생성(607~609), ESC·선택·카메라·단축키 입력, 메인 메뉴 팝업·인트로, `Netstorm.exe` 산출물과 배포 런처 등을 연결했습니다([685](docs/exe/cpp-product-1101-20261101.md)·[684](docs/exe/cpp-external-services-20261101.md)·[645](docs/exe/cpp-solo-integrated-20261031.md)). 지난 단위의 상세 이력은 [LEFT_JOBS.old.md](LEFT_JOBS.old.md)에 있습니다.

### 남은 일

전체 목표는 미완료이며 단위986에서 사용자 요청으로 일시중지했습니다. 재개하면 3차 멀티플레이를 원격 Fort 수명(987)부터 시작해 월드 동기화·전투섬/중앙섬·일반 전투 State·참가자 목록·LAN UI·메타맵/저장/재접속·Outpust 순서로 원문 근거에 따라 사전 구현합니다(새 통신 검사는 하지 않습니다). 6차 한국어의 미션 브리핑·팁·도움말 `.korean` 본문, 한글 메인 버튼 그림, 한글 입력과 번역 검수가 남았습니다. 8차 Edit 시나리오 편집 UI, 새 맵의 기본 `.english` 자동 생성, `.english` 없이 Test Battle에 들어갈 때 종료되는 문제의 수정도 남아 있습니다. 5차의 `CustomMaps` 배치 방식, 7차의 미션·전체화면·4K 화면, 모든 미션의 자연 완주와 전체 동일성, 실제 포획·희생·특정 적 공격·승리 재현은 사용자 확인 대상입니다. 60/120프레임과 CD판 10.72 전체 복원은 별도 후순위입니다. 단위별 근거와 아직 확인하지 못한 범위는 [인수인계](LEFT_JOBS.md)에 기록합니다.

## C# + MonoGame 클론(dotnetpj)의 최신 상태

기존 실행 경로(`Netstorm.exe`)에서는 캠페인 **1-1 The War Begins!**와 **1-2 Master of Whirligigs**를 플레이할 수 있습니다. 별도로 cpppj 구조에 맞춘 새 C# 클라이언트(`Netstorm.Client`)를 구축하고 있습니다. 화면은 8비트 논리 버퍼·팔레트 기반으로 재구성하며, MonoGame은 호스트(`Netstorm.Client.Host`)와 최종 출력 역할을 맡습니다. [캠페인 1-1](docs/gameplay/campaign-one.md)·[1-2](docs/gameplay/campaign-two.md)의 기존 구현 기록과 [최신 인수인계](LEFT_JOBS.dotnetpj.md)를 참고하세요.

2026-10-28에 계획을 다시 세워(R0~R8) 기존 근사 구현을 동결하고, cpppj가 원본 기계어로 검증한 구조를 그대로 옮기는 방식으로 바꿨습니다. R0(기준선·포트 맵), R1(8비트 화면·MonoGame 출력 호스트·캡처 도구), R2(그리기 원시·팔레트·색표·일반 객체 그리기)를 마쳤습니다. R3(Gump UI·메인 메뉴 계열)은 Gump 트리·입력·글꼴·버튼·메뉴·문자 입력·이미지·배경 GIF·HTML/도움말 창과, 새 호스트의 타이틀·메인 메뉴·Help/Options 팝업·옵션 저장·Edit 맵 목록까지 옮겼습니다. 대화창과 상위 메뉴 소유 구조, 메뉴 캡처 비교(D1)가 남아 있어 아직 진행 중입니다. R4(월드 세션)는 미션 조립 부품(Fort reader·Guide·공통 Start·AI 설정·Tutorial Init), Template/DataManager 저장소(TS0~TS6, 원본 `.fort` 433개 읽기→저장 왕복 일치), Fort 적재 단계 F1~F2c-11(타입 표·Money/Meta·TypeNames/Deck·Territory·객체 적재·저장 모듈·섬 목록·군집)을 원본 x86 fixture와 대조해 이식했습니다. 11.01 표기·Credits 3항목(0.150), 외부 접속 문서화·유효성 검사 비활성화(0.151), 로컬 전송 골격(0.152)도 적용했습니다.

0.154(2026-11-04)의 Release 검사는 **Core 1,243개·Client 858개·Assets 366개** 통과(실패·건너뜀 0)입니다. 기존 경로의 검사가 포함된 값이라 새 경로의 진행도와 같지 않습니다. 다음은 F2c 나머지(`RawFortWorldLoad`, 지형 섬 Builder 계열)→F2d→F3(`RawCampaignSession` 연결)이고, 그 뒤 R5(월드 표시·HUD)→R6(플레이 입력·규칙)→R7(미션 전체·Edit·설정)→R8(한국어·4~8차 등) 순서로 진행합니다. 새 경로가 실제 미션에 연결되기 전까지 플레이 범위와 원본 동일성은 1-1·1-2 근사 경로에 한정됩니다. 순서와 남은 범위는 [인수인계](LEFT_JOBS.dotnetpj.md), 이식 근거 목록은 [cpppj → dotnetpj 이식 계획](docs/dotnet-cpp-port-plan-20261016.md)과 [Template 저장소 이식 계획](docs/dotnet-template-storage-plan-20261101.md)을 참고하세요.

## HTML + JavaScript 웹 클론(webpj)의 최신 상태

브라우저의 오디오 재생 제한에 따라 원본 자산을 적재한 뒤 "클릭하여 시작" 화면을 거쳐 메인 메뉴를 표시합니다. Campaign·Demo·Credits 대화상자와 Early Missions·Priest Training·캠페인 장 목록, Help·Options 펼침 메뉴(설정 9개 저장, 소리·음악·음량 반영), F1 General Help 창(스크롤바·링크·방문 기록, 원본과 사실상 픽셀 일치), 시작 환영/팁 창, 캠페인 진행 값(`Done*`) 저장을 원본 스크린샷과 대조해 구현했습니다. 제품 표기는 `v11.01`이며 Credits에는 세 번째 **Netstorm 11.01 Patch Credits** 항목이 있습니다(WB03-AC). 웹 런타임의 외부 접속(같은 출처의 자산 요청만 하고, 바깥 링크는 콘솔 안내)은 [외부 접속 목록](docs/webpj-external-access-20261105.md)에 정리했습니다(WB02-E). 도움말 본문의 그림 아이콘은 전체 타입 표(116개)로 해석해 표시합니다(WB03-T).

브라우저 언어가 한국어이면 D2Coding 글꼴(woff2)과 원본 방식의 언어 파일(`xlat.korean`·`tell.korean`·`offical*.korean`·`config.korean`·`help.korean`)로 메인 메뉴·펼침 메뉴·대화상자·캠페인 목록·F1 도움말을 한국어로 표시합니다. 도움말은 주제 128개 중 126개를 번역했으며, 원문 자체가 미완성인 `serverHelp`와 그림뿐인 `map`은 영어로 남습니다(WB03-N~S). 미션 문구의 한국어는 미션 구현과 함께 진행합니다.

미션 진입을 위한 기반으로 `.fort` 판독(188타입)·영역 배치·지형 마스크/타일/절벽·소유자 색표를 원본 465개 맵과 독립 대조했고(WB03-U~Y), 공통 월드 그림 출력과 개발용 지형 미리보기(`?map=`, 게임 규칙 없음), Fort 객체 로더를 만들었습니다(WB03-Z~AB). 이어 WB04에서 원본 10.78의 SID 풀·공간 해시/탐색기·난수/게임 시계/Kernel·게임 보조 전역·40칸 입력 큐/메인 루프·공통 공간 해제/Unpop/삭제기·공통 Form 층·SquidFactory·Form 호스트(Depend/Gump/Content)를 원본 fixture와 대조하며 이식했습니다(WB04-A~D5a, 2026-11-06). 이 부품들은 아직 앱 수명에 연결하지 않았으므로 **실제 미션 플레이는 아직 할 수 없습니다.** 마지막 전체 검사는 Node v24에서 **429개 통과**입니다. 다음은 WB04-D5b(Form 분배기·Content 수명)→D5c(ProcessHost·Regular)→E(Pop/소유자)→WB05 실제 Fort Load→Start→미션 화면(M2)·플레이 규칙 순서입니다.

사용자 결정에 따라 웹에서는 커스텀 맵 Edit, Campaign 목록의 **User Made Campaigns**, 멀티플레이, 해상도·전체화면 설정 등을 제공하지 않습니다. 원본 자료의 바깥 링크를 새 탭/창으로 여는 기능은 결정됐지만 아직 구현하지 않았습니다. Wind Noise·Speaker Swap L/R·Edge Scroll·Auto-Demo 설정은 값만 저장합니다. cpppj가 계속 바뀌므로 각 단위는 착수 시점의 커밋된 cpppj를 기준으로 구현하고, 이후 cpppj 변경은 별도의 재반영 과정으로 반영합니다. 자세한 상태와 다음 작업은 [최신 인수인계](LEFT_JOBS.webpj.md)를 참고하세요.

# 프로젝트 구성과 개발 목표

`cpppj`는 **① 10.78 싱글플레이 복원 → ② Windows 10/11 안정 구동(DirectDraw 오류 수정 포함) → ③ TCP/IP 로컬 네트워크 멀티플레이·아웃포스트 → ④~⑧ 추가 요구사항** 순서로 진행합니다. 추가 요구사항은 다음과 같습니다.

- **4차:** 로컬 MCP(dotnetpj 분석을 위한 모든 게임 조작·화면 캡처·소리 포함 녹화)와 16:9 최대 1920×1080 해상도 지원(그래픽 크기 스케일 조정은 불필요)
- **5차:** 설정·캠페인 진행 저장을 사용자 홈의 `Saved Games\Netstorm\Saves\`로 옮기고, 게임이 자동으로 만드는 모든 파일·폴더를 `Saved Games\Netstorm\` 아래로 옮겨 관리자 권한 없이 쓸 수 없는 위치에 설치해도 동작하게 합니다. 커스텀 맵은 기존 폴더와 `Saved Games\Netstorm\CustomMaps\` 양쪽에서 불러오고(Edit에서 새로 만든 맵은 후자에 저장), Edit 맵 목록에 스크롤을 넣고 표시 개수 제한을 없앱니다.
- **6차:** 한국어 지원
- **7차:** 메인 메뉴 Options의 안티앨리어싱과 FSR 1.0 선택(기본 꺼짐). FSR 지원 환경에서는 2560×1440·3840×2160을 고를 수 있고, 이때 FSR을 자동으로 켜서 실질 1920×1080 화면을 목표 해상도로 업스케일합니다.
- **8차:** Edit에서 맵·구성요소 배치뿐 아니라 시나리오 자체(플레이어 초기 자본·테크, 적 구성·동맹 등)를 화면에서 편집하는 전용 UI, 새 맵을 만들 때 기본 `.english` 파일 자동 생성(`.english` 없이 Test Battle에 들어가면 종료되는 문제 수정)

①·②는 자연 완주·전체 동일성 같은 사용자 확인 항목을 남긴 채 진행 중이고, 4·5·7차는 1차 구현을 마쳤으며 6차는 진행 중, 3·8차는 남아 있습니다. Windows 전용(x86-64 대상, Windows 11 ARM 고려)이며, 멀티플레이 전용 아웃포스트는 3차 목표에 포함합니다. 공식 서버 접속은 복원하지 않습니다. CD판 10.72의 비교·분석 기록도 보존합니다.

`dotnetpj`는 C# + MonoGame 기반입니다. 처음에는 이미 구현된 클론에 `cpppj`가 검증한 부분을 조각별로 옮기는 방식(2026-10-16 이식 계획)이었으나, 기존 구현이 원본과 다른 곳이 많아 2026-10-28에 계획을 다시 세웠습니다. 기존 근사 구현은 동결하고, cpppj 구조(8비트 화면·Gump UI·월드 세션)를 그대로 옮긴 새 `Netstorm.Client`로 R0~R8 순서에 따라 교체하며, cpppj를 같은 입력의 비교 기준으로 씁니다. 원본과 완전히 같은 동작을 먼저 구현하고, `cpppj`가 완성되면 전체를 다시 대조한 뒤 추가 요구사항을 반영합니다. cpppj의 4~8차 목표(MCP·와이드 해상도, 저장 위치 이동·커스텀 맵 경로·Edit 목록 스크롤, 한국어 지원, 안티앨리어싱·FSR 1.0, Edit 시나리오 UI)도 후순위 요구사항에 포함됩니다. Windows 실행 파일 이름은 `Netstorm.exe`입니다. GUI Linux 지원과 멀티플레이는 후순위입니다. 최종 요구사항에는 전체화면·16:9/16:10/4:3 화면비·전체화면 가장자리 스크롤·영어/한국어 지원이 포함됩니다. 애니메이션은 원본 수준을 먼저 복원한 뒤 60/120프레임 지원을 진행합니다.

`webpj`는 HTML + JavaScript 기반입니다. 원본과 같은 동작을 목표로 `cpppj`를 분석해 개발하며, cpppj가 아직 진행 중이므로 커밋된 현재 cpppj를 참고해 먼저 구현하고 이후 변경은 따로 재반영합니다(2026-11-05 사용자 결정). 커스텀 맵(Edit)·User Made Campaigns·멀티플레이, 웹 환경에 맞지 않는 기능(게임 종료, 해상도·전체화면 설정 등)은 제외하며, 필요한 에셋은 `webpj/www/` 안으로 복사해 사용합니다.

| 경로 | 용도 |
| --- | --- |
| `cpppj/` | C++20·Win32 기반 복원판과 검사 (주력) |
| `dotnetpj/` | C#·MonoGame DesktopGL 클론과 검사 |
| `webpj/` | HTML + JavaScript 웹 클론(`www/`), 정적 웹 서버, 검사·자산 준비 도구 |
| `assets/game-data/` | 클론용 그래픽·지도·미션·음악·효과음 데이터 |
| `fonts/` | 한국어 표시용 D2Coding 글꼴 |
| `docs/` | 분석·복원 근거, 빌드·플레이·검증 문서 |
| `tools/`, `analyzeManager/` | 정적 분석·원본 관찰·검증 도구 |
| `originals/`, `originalCD/` | 분석용 원본 10.78 및 CD판 10.72 자료 |
| `original1037/`, `originalPatches/` | 비교용 원본 10.37 자료와 10.37 → 10.62 패치(NSP1062) |
| `extracted/` | 정밀 디컴파일 결과와 빌드·검사 원시 로그. Git에 포함하지 않아 작업한 PC에만 있습니다. |
| `screenShots/`, `playingVideos/`, `originalFanGame/` | 스크린샷·녹화·팬게임 참고 자료. 대용량 영상과 팬게임 코드는 Git에 포함하지 않아 PC마다 없을 수 있습니다. |

한국어 건물·유닛 이름은 [공통 명칭 규칙](docs/korean-unit-names.md)을 따릅니다. 사제·신전·다리·발사기·간헐천·제단·거주 구역(Residence)·고갈된(Empty)은 번역할 수 있으며, 나머지는 워크샾·제네레이터처럼 영어 발음을 한글로 표기합니다. cpppj의 번역표(`xlat.korean` 502항목)와 dotnetpj의 메뉴·유닛 제목(공유 이름표 55개)이 같은 명칭을 씁니다. webpj는 메뉴·대화상자·도움말 번역을 마쳤고, 세 프로젝트 모두 미션 본문 번역은 아직입니다.

작업 전 [AGENTS.md](AGENTS.md)와 각 프로젝트의 인수인계([cpppj·공통](LEFT_JOBS.md), [dotnetpj](LEFT_JOBS.dotnetpj.md), [webpj](LEFT_JOBS.webpj.md))를 확인합니다. 원본·별도 창 실행 조건은 [실행 확인 규칙](docs/execution-confirmation.md)을 따릅니다. 분석용 원본 파일은 보호하며, 클론 데이터의 변경은 `assets/game-data/`에서 관리합니다.

# 개발 환경 준비

루트의 준비 스크립트에서 필요한 도구를 선택해 설치하거나 현재 환경을 점검할 수 있습니다. 점검만 하려면 다음 명령을 사용합니다.

```powershell
powershell -ExecutionPolicy Bypass -File .\PREPARE.ps1 -CheckOnly
```

```sh
bash ./PREPARE.sh --check-only
```

설치·선택 화면은 각각 `-CheckOnly` 또는 `--check-only`를 생략해 엽니다. REA는 기본 선택되지 않는 선택 항목이며 기존 Node.js·npm을 사용합니다. Ghidra와 REA를 함께 준비하면 `GHIDRA_INSTALL_DIR`를 연결합니다. 세 프로젝트의 빌드에 원본 게임 실행이나 REA 설치는 필요하지 않습니다.

# C++ 복원판 빌드와 실행 (cpppj, 주력)

Windows 10/11, CMake 3.21 이상, C++20 컴파일러(Visual Studio 2022 이상 또는 Build Tools의 C++ 도구)가 필요합니다. Win32 API를 직접 사용하는 Windows 전용 빌드입니다. 프로젝트 루트에서 실행합니다.

다음 스크립트를 실행하면 **게임 / 통합 테스트 / 전체** 선택 메뉴가 나옵니다. 빈 입력은 취소합니다.

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\cpppj\build-cpppj.ps1
```

| 선택 | 명령 뒤에 붙이는 인자 | 컴파일 범위 |
| --- | --- | --- |
| 게임 (권장) | `-Mode Game` | 게임 실행 파일과 필요한 공용 코드. 테스트 소스 제외 |
| 통합 테스트 | `-Mode Tests` | `netstorm_tests`와 필요한 공용 코드·도우미. 별도 집중 테스트의 중복 컴파일 제외 |
| 전체 | `-Mode All` | 기존처럼 게임·통합 테스트·모든 집중 테스트·도우미 전체 |

예를 들어 게임만 빌드하거나 집중 테스트 하나만 빌드하려면 다음처럼 실행합니다. 빌드는 증분 방식이며, 스크립트가 게임이나 테스트를 실행하지는 않습니다.

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\cpppj\build-cpppj.ps1 -Mode Game
powershell -NoProfile -ExecutionPolicy Bypass -File .\cpppj\build-cpppj.ps1 -Mode Tests -TestTarget netstorm_editor_file_ring_tests
```

기본 구성은 Release, 병렬 작업 수는 4입니다. `-Configuration Debug`, `-Jobs 2`, `-BuildDirectory cpppj/build-other`로 바꿀 수 있고, `-DryRun`은 실행할 명령만 표시합니다. 선택 메뉴 없이 통합 테스트는 `-Mode Tests`, 기존 전체 빌드는 `-Mode All`을 사용합니다.

CMake를 직접 사용할 때의 동일한 명령은 다음과 같습니다. **새 빌드 폴더의 기본값은 테스트 OFF**이며, 기존 캐시의 ON 값은 자동으로 바뀌지 않습니다.

```powershell
# 게임만
cmake -S cpppj -B cpppj/build -DCMAKE_BUILD_TYPE=Release -DNETSTORM_BUILD_TESTS=OFF
cmake --build cpppj/build --config Release --target NetstormCpp --parallel 4
# 통합 테스트만 (필요한 공용 코드도 컴파일)
cmake -S cpppj -B cpppj/build -DCMAKE_BUILD_TYPE=Release -DNETSTORM_BUILD_TESTS=ON
cmake --build cpppj/build --config Release --target netstorm_tests --parallel 4
# 기존 전체 빌드 (테스트 ON 구성 후 대상 생략)
cmake -S cpppj -B cpppj/build -DCMAKE_BUILD_TYPE=Release -DNETSTORM_BUILD_TESTS=ON
cmake --build cpppj/build --config Release --parallel 4
# 통합 테스트 실행은 컴파일과 별개로 선택
ctest --test-dir cpppj/build --build-config Release --output-on-failure
```

CTest 실행은 통합 테스트 빌드 후에 사용합니다. 각 단위의 집중 검사와 전체 검사 결과는 [인수인계](LEFT_JOBS.md)에서 구분해 확인할 수 있습니다. Visual Studio 생성기 기준 게임 실행 파일은 `cpppj/build/bin/Release/Netstorm.exe`입니다. 다음 명령은 창 모드로 C++ 복원판의 메인 메뉴를 엽니다. 메인 메뉴의 Campaign에서 캠페인 1장의 첫 미션 **The War Begins**를 선택하면 캠페인 1-1을 시작합니다.

```powershell
.\cpppj\build\bin\Release\Netstorm.exe --run originals --window
```

`--run <게임 데이터 폴더>`는 C++ 복원판을 실행합니다(원본 게임을 실행하지 않습니다). `--mission <미션 키>`를 붙이면 메뉴를 거치지 않고 해당 미션의 브리핑으로 들어갑니다(예: `--mission Tutorial1`, 캠페인 1-1은 `--mission TheWarBegins`). `--view fonts`, `--view types`, `--view TEST01`은 글꼴·타입·미션 검사 장면입니다. 인자 없이 실행하면 GUI 메인 메뉴를 표시하며, `--help`는 빌드 정보와 사용할 수 있는 옵션을 표시합니다. 한국어는 Options > Language에서 고르거나 `--set currentLanguage=korean`을 붙여 시작합니다. `--mcp`를 붙이면 게임이 로컬 MCP 서버(stdio)로 실행되며, 등록 방법과 도구 목록은 [MCP 기록](docs/exe/cpp-mcp-server-20261106.md)을 봅니다. 건설·생산·다리·채취/운반·전투와 공통 전략 AI의 자동 실행이 연결돼 있으며, 모든 미션의 자연 완주와 원본 동일성의 전체 검증은 사용자 확인 대상입니다. 빌드 절차는 [C++ 빌드](docs/cpp-build.md), 실제 최신 연결 상태는 [인수인계](LEFT_JOBS.md)를 참고하세요.

## 배포판 압축파일 만들기와 실행

배포용 압축파일(실행 파일·Visual C++ 런타임·원본 10.78 게임 데이터·한국어 글꼴과 번역표·FSR 1.0 헤더와 라이선스·실행 배치 파일)은 다음 스크립트로 만듭니다. 저장 위치를 생략하면 실행 중에 입력받고, 빈 값을 입력하면 취소합니다. 구성과 옵션은 [C++ 빌드 7절](docs/cpp-build.md)을 봅니다. 받는 PC에서는 압축을 풀고 `Netstorm.exe` 또는 `Netstorm-SavedMode.bat`(저장된 화면 모드), `Netstorm.bat`(창 모드로 강제)을 실행합니다. 설정·진행·캡처 등은 설치 폴더가 아니라 `%USERPROFILE%\Saved Games\Netstorm\`에 저장되므로 압축을 푼 폴더가 읽기 전용이어도 됩니다. 설정을 초기화하려면 `%USERPROFILE%\Saved Games\Netstorm\Saves\`의 `options.cfg`와 `fullscreenStateFile.dat`를 지웁니다. Windows PowerShell 5.1의 기본 증분 빌드→압축과 압축 해제본의 첫 실행·저장 전체화면 재실행·창 런처를 확인했고, 단위645(제품644, 1,376항목)와 단위938(한국어 글꼴·번역표 포함, 1,379항목) 배포본의 압축 해제 실행을 확인했습니다. 이후 제품의 배포본 확인과 Windows 11 배포 검증은 별도입니다.

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File cpppj\package-cpppj.ps1 D:\dist\NetstormReborn-cpppj.zip
```

# C# + MonoGame 클론 빌드와 실행 (dotnetpj, 새 경로로 교체 중)

아래 명령은 프로젝트 루트(`NetstormReborn`)에서 실행합니다. 게임 본체는 C# / MonoGame DesktopGL이며 대상 프레임워크는 `net10.0`입니다. 현재 빌드되는 실행 파일은 기존 근사 구현 경로(`Netstorm`)입니다.

## 준비

- **.NET 10 SDK**를 설치하고 `dotnet --version`으로 확인합니다. `dotnetpj/global.json`은 10.0.100 이상인 .NET 10 SDK의 최신 기능 버전을 허용합니다.
- 첫 빌드·배포 시 NuGet 패키지와 대상 OS의 런타임을 내려받을 인터넷 연결이 필요합니다.
- 저장소의 `assets/game-data/`와 한국어 글꼴 `fonts/D2Coding-Ver1.3.2-20180524-all.ttc`를 함께 체크아웃합니다. 데이터·글꼴은 빌드·배포 출력에 자동으로 복사됩니다.
- 실행 대상은 **Windows 10/11 x64** 또는 **GUI 환경의 x64 Linux(glibc 기반)**입니다. OpenGL을 지원하는 그래픽 드라이버가 필요합니다.

dotnetpj의 빌드·테스트·실행은 독립된 `assets/game-data/`를 사용하며 원본 실행 파일이 필요하지 않습니다. `originals/`는 분석용 보호 자료로 별도 보존합니다. 자산의 구성과 이관 기록은 [클론 게임 데이터](assets/README.md)를 참고합니다.

## 개발용 빌드와 실행

```sh
dotnet restore dotnetpj/Netstorm.sln
dotnet build dotnetpj/Netstorm.sln -c Release --no-restore
dotnet test dotnetpj/Netstorm.sln -c Release --no-build
dotnet run --project dotnetpj/src/Netstorm.Game -c Release --no-build -- --language korean
```

영어로 실행하려면 `--language english`를 사용합니다. 빌드 결과는 `dotnetpj/src/Netstorm.Game/bin/Release/net10.0/`에 생성됩니다. 일반 빌드는 빌드한 OS용 실행 파일을 만들며, 다른 OS용 바이너리는 아래의 `-r` 옵션으로 만듭니다.

## 런타임을 포함한 배포용 바이너리

게임 프로젝트에 `dotnet publish`를 실행합니다. 다음 명령은 Windows PowerShell과 Linux 셸에서 모두 사용할 수 있으며, Linux에서도 Windows exe를 만들 수 있습니다.

```sh
dotnet publish dotnetpj/src/Netstorm.Game/Netstorm.Game.csproj -c Release -r win-x64 --self-contained true -o dist/win-x64
dotnet publish dotnetpj/src/Netstorm.Game/Netstorm.Game.csproj -c Release -r linux-x64 --self-contained true -o dist/linux-x64
```

| 대상 | 출력 폴더 | 실행 파일 |
| --- | --- | --- |
| Windows 10/11 x64 | `dist/win-x64/` | `Netstorm.exe` |
| Linux x64 | `dist/linux-x64/` | `Netstorm` |

`--self-contained true`는 .NET 런타임까지 포함하므로 실행할 PC에 .NET SDK나 런타임을 따로 설치할 필요가 없습니다. SDL2·OpenAL 네이티브 라이브러리, `fonts/`, `game-data/`도 출력에 포함됩니다. 실행 파일은 함께 생성된 DLL·런타임 파일·글꼴·게임 데이터를 사용하므로 **출력 폴더 전체를 복사해 배포**합니다. 그래픽 드라이버와 Linux GUI 환경 등 OS의 실행 환경은 별도로 필요합니다.

실행할 PC에 .NET 10 런타임이 이미 설치되어 있다면 더 작은 배포물을 만들 수 있습니다. 위 명령의 `--self-contained true`를 `--self-contained false`로 바꾸고, 출력 폴더도 `dist/win-x64-fdd` 또는 `dist/linux-x64-fdd`처럼 구분합니다.

배포용 압축파일(zip)은 다음 스크립트로 만듭니다. 저장 위치를 생략하면 실행 중에 입력받고, 빈 값을 입력하면 취소합니다. 기본은 받는 PC에 .NET 10 런타임을 미리 설치하는 방식이며, `-SelfContained`를 붙이면 런타임까지 포함합니다. 임시 폴더에 게시하므로 저장소에 `dist/`를 남기지 않습니다.

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File dotnetpj\package-dotnetpj.ps1 D:\dist\NetstormReborn-dotnetpj.zip
```

## 포함된 게임 데이터와 바이너리 실행

빌드·배포 시 `assets/game-data/`의 내용이 출력의 **`game-data/`에 자동으로 복사**됩니다. Windows 배포 폴더는 다음과 같은 구조가 됩니다(Linux도 동일하며 실행 파일 이름은 `Netstorm`입니다).

```text
dist/win-x64/
  Netstorm.exe
  Netstorm.dll
  ... (함께 생성된 DLL·런타임·설정 파일)
  fonts/
    D2Coding-Ver1.3.2-20180524-all.ttc
  game-data/
    netstorm.tarc
    d/
    music/
    sound/
```

Windows PowerShell에서 실행:

```powershell
.\dist\win-x64\Netstorm.exe --language korean
```

Linux에서 실행:

```sh
chmod +x dist/linux-x64/Netstorm
./dist/linux-x64/Netstorm --language korean
```

일반 실행은 함께 배포된 데이터를 자동으로 찾습니다. 개발·검증용 데이터를 다른 위치에서 사용하려면 `NETSTORM_DATA`에 **`netstorm.tarc`가 들어 있는 폴더의 절대 경로**를 지정합니다. 탐색 순서는 유효한 `NETSTORM_DATA` → 실행 파일·작업 폴더 및 그 상위의 `game-data/` 또는 `assets/game-data/`입니다.

```powershell
$env:NETSTORM_DATA = "D:\Games\NetstormReborn\game-data"
.\dist\win-x64\Netstorm.exe --language korean
```

```sh
NETSTORM_DATA="/절대/경로/game-data" ./dist/linux-x64/Netstorm --language korean
```

# 웹 클론 실행 (HTML + JavaScript)

Node.js가 필요하며(v24에서 검증) 외부 npm 패키지는 쓰지 않습니다. 필요한 에셋은 `webpj/www/data/`에 커밋돼 있습니다. 프로젝트 루트에서 다음 명령으로 정적 웹 서버를 시작한 뒤 브라우저에서 `http://localhost:9803/`을 엽니다.

```sh
npm --prefix webpj start
```

자동 검사는 `npm --prefix webpj test`로 실행합니다. `assets/game-data/`의 원본 자료가 바뀌면 `npm --prefix webpj run prepare-assets`로 `webpj/www/data/`를 다시 만들고, `npm --prefix webpj run check-assets`로 복사본을 점검합니다. 개발용 주소 인자로 `?lang=ko`(한국어), `?mute`(소리 끄기), `?aspect=16:9`(화면비)를 조합할 수 있고, `?map=savetheisland`는 게임 규칙 없이 지형만 보여 주는 개발용 화면입니다.

# 주요 문서

- [cpppj·공통 인수인계](LEFT_JOBS.md) / [dotnetpj 인수인계](LEFT_JOBS.dotnetpj.md) / [webpj 인수인계](LEFT_JOBS.webpj.md): 최신 상태와 앞으로 할 작업. 지난 단위의 상세 이력은 [LEFT_JOBS.old.md](LEFT_JOBS.old.md)(cpppj·공통), [LEFT_JOBS.dotnetpj.old.md](LEFT_JOBS.dotnetpj.old.md)(dotnetpj), [LEFT_JOBS.webpj.old.md](LEFT_JOBS.webpj.old.md)(webpj)에 있습니다.
- [cpppj 일시중지 인계 자료](docs/exe/handoffs/cpp-pause-20261106/README.md): 단위986에서 보존한 한국어 본문·시나리오 UI 준비본과 원격 Fort 분석
- [TODO.md](TODO.md): cpppj·dotnetpj의 목표 프롬프트(각 인수인계의 작업을 끝까지 진행하는 지시와 건너뛸 검사)
- [C++ 복원 로드맵](docs/cpp-roadmap.md) / [실제 플레이 복원 계획](docs/cpp-playable-plan.md): 개발 단계와 복원 계획
- [cpppj → dotnetpj 이식 계획](docs/dotnet-cpp-port-plan-20261016.md): 이식 단위(W 번호)와 근거 목록. 현행 순서(R0~R8)는 [dotnetpj 인수인계](LEFT_JOBS.dotnetpj.md) 0절이 우선합니다.
- [클론 게임 데이터](assets/README.md): 자산 구성·출처·빌드 포함 방식
- [원본 실행·별도 창 확인 규칙](docs/execution-confirmation.md): AI 작업의 실행 조건

# License

MIT License
Copyright (c) 2026 HJOW
