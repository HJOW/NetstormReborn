Netstorm - Reborn
-------------------------------------------------------------------------

**한국어** | [English](README.en.md)

# 개요 
Activision 사에서 1997년도에 개발 후 한참 전에 손 놓은
Netstorm - Islands at war 게임을
AI 를 이용해 되살리는 프로젝트입니다.

세 가지 버전을 병행하며, 현재 **실제로 플레이할 수 있는 주력은 `cpppj`(C++ 복원판)** 입니다. 원본 10.78을 기계어 수준으로 대조해 복원하고 있으며, 모든 미션(34경로)에 진입할 수 있고 캠페인 1-1은 원본과 거의 동일한 수준으로 끝까지 진행할 수 있습니다. `dotnetpj`(C# + MonoGame)와 `webpj`(HTML + JavaScript)는 `cpppj`의 검증 결과를 바탕으로 뒤따라 개발합니다.

# 현재 개발 진행상황

**2026-10-30 기준:** cpppj는 단위 611까지 진행했고 전체 목표를 계속 수행 중입니다. dotnetpj는 `20261029-58` / R3k-8(인계 0.61)에서 사용자 요청으로 일시 정지한 상태이고, webpj는 WB03-I까지 마쳤습니다. 각 프로젝트의 **실행·플레이 범위**와 **원본 동일성**을 구분해 적습니다.

| 프로젝트 | 실행·플레이 가능 범위 | 원본과의 동일성 및 남은 범위 |
| --- | --- | --- |
| **cpppj — C++ 복원판 (주력)** | **현재 플레이 가능합니다.** `Netstorm.exe`로 원본 메인 메뉴부터 Early Missions 6개·Priest Training 9개·캠페인 16개·데모 3개, 총 34경로에 들어가 진행할 수 있습니다. 캠페인 1-1은 사용자가 직접 완주와 다음 스테이지 안내까지 확인했고(2026-10-28) 원본과 거의 동일한 수준으로 진행됩니다. 건설·생산·다리·채취/운반·전투, 공통 전략 AI, 덱·미니맵·QWASZX·ESC 메뉴, 의식·기술 획득, 채팅, 인트로 영상, 세계 간헐천 생성, Edit(기존 맵 편집·저장·시험 전투)가 연결돼 있습니다. | 원본 10.78 기계어 대조에 근거한 복원입니다. 최신 611은 클라우드 플로터의 투사체 회피를 원문 481사례 차이 0으로 복원했고, 집중 검사 46개 실패 0(등록 3,516개), Release 경고·오류 0입니다. 새 배포본의 기본 진입·비정지 진행·실제 창 출력도 확인했습니다. 같은609 제품의 전체34경로는610에서34/34 통과했습니다. **미완료:** 모든 미션의 자연 완주·승패, 남은 UI·Edit 도구·시나리오, 전체 외관·음향의 원본 대조, Windows 11 실기·PCM 음향·사용자 활성 전체화면, 멀티플레이·Outpust(디컴파일 기반 사전 구현 예정). |
| dotnetpj — C# + MonoGame 클론 | 기존 경로에서 캠페인 **1-1 전쟁의 시작!(The War Begins!)**, **1-2 훨리기그의 지배자(Master of Whirligigs)**를 플레이할 수 있습니다. 영어·한국어 UI, 도움말·우클릭 메뉴·옵션을 제공합니다. | 2026-10-28에 계획을 다시 세워, 기존 근사 구현을 동결하고 원본 구조에 맞춘 새 `Netstorm.Client` 경로로 교체하는 중입니다(일시 정지). 8비트 화면·MonoGame 출력(R1)과 그리기 원시·팔레트·색표·일반 객체 그리기(R2)를 마쳤고, Gump UI(R3)는 트리·입력·글꼴·버튼·메뉴·문자 입력창·이미지·배경 GIF·HTML/도움말 창의 생성과 링크·모달 연결(R3k-8)까지 진행했습니다. 2026-10-29 최신 Release 빌드는 경고·오류 0, 전체 검사 2,002개 통과입니다. R3 전체와 메뉴 화면 캡처 비교는 아직이고, 새 화면 경로와 월드 코어는 기본 게임 세션에 연결되지 않아 플레이 가능 범위와 원본 동일성은 기존 1-1·1-2 근사 구현 수준입니다. 실행 파일 이름을 `Netstorm.exe`로 바꾸는 작업도 아직 적용 전입니다. |
| webpj — HTML + JavaScript 웹 클론 | 원본 자산을 적재하고 "클릭하여 시작"을 거쳐 메인 메뉴를 표시합니다. Campaign·Demo·Credits 대화상자와 Early Missions 등 하위 목록 5종, Help·Options 펼침 메뉴(설정 저장, 소리·음악·음량 반영)와 F1 일반 도움말 창(스크롤바·링크·방문 기록)을 구현했고, 캠페인 진행 값(`Done*`)을 브라우저에 저장합니다. 메인 메뉴 Edit 버튼과 Campaign 목록의 User Made Campaigns 행은 제거했습니다. | 2026-10-30 WB03-I 기준 자동 검사 77개가 통과했고 실제 브라우저(Edge 헤드리스) 확인도 통과했습니다. 대화상자·목록·도움말 창은 원본 스크린샷과 대조했습니다(도움말 창은 사실상 픽셀 일치). 타입 정보가 있는 도움말 주제의 능력치 머리글·타입 그림, 시작 팁, 한국어 표시, 정적 맵 화면과 실제 게임 플레이는 아직 없고, Wind Noise·Speaker Swap L/R·Edge Scroll·Auto-Demo·Tell Tips 설정은 값만 저장합니다. 웹에서는 커스텀 맵 Edit·User Made Campaigns·멀티플레이를 제공하지 않습니다. |

## C++ 복원판(cpppj)의 최신 상태

### 지금 할 수 있는 것

- **미션 34경로:** Early Missions 6개, Priest Training 9개, 캠페인 16개(1·2·3장), 데모 3개에 모두 진입해 진행합니다. 캠페인 1-1은 사용자가 직접 끝까지 플레이해 다음 스테이지 안내까지 정상 동작함을 확인했습니다.
- **원본 메뉴와 연출:** 메인 메뉴의 Campaign·Demo·Help·Options·Credits·Quit, 브리핑, CD 인트로 영상(SMK, 영어·독일어)을 원본 구조대로 재현합니다.
- **게임 규칙:** 건설·생산·다리·채취/운반·전투, 워크샾 유닛 25종, 포획·희생·SP·미션 완료를 포함한 의식 규칙, 기술 획득, 적·아군·데모가 같은 원본 전략을 쓰는 공통 AI, 세계 간헐천 생성을 원문 대조로 복원했습니다.
- **게임 중 UI:** 덱·미니맵, 건물 단축키 QWASZX, ESC 메뉴, 채팅(Enter), 도움말 링크, 4:3·16:9·16:10 화면비와 가장자리 스크롤을 연결했습니다.
- **Edit:** 기존 맵 불러오기·저장·다른 이름 저장, 신규 섬 추가, 시험 전투(Test Battle) 후 편집 복귀, 방향별 동맹·AI 설정 등 시나리오 값을 지원합니다.
- **Windows 10/11 대응:** 원본에서 전체화면 종료 후 재실행하면 오류가 나던 문제가 생기지 않도록 구현했고, 저장된 전체화면 상태에서 다시 실행해 메인 메뉴까지 정상 진입함을 확인했습니다(단위 563·588). 배포판은 `Netstorm.bat`로 바로 실행됩니다.

### 최근 진행

- **611(클라우드 플로터 회피):** 투사체 충돌에서 발생하던 미복원 예외를 원본 회피와 같은 게임 난수로 복원했습니다. 원문481사례/962관측 차이0, 집중46개 실패0이며 새배포본의 기본진입·진행·창RGB도 확인했습니다. 실제 특정적공격은 사용자 확인 대상으로 남깁니다([611 기록](docs/exe/cpp-missile-floater-20261030.md)).
- **607~609 (간헐천):** 일반 간헐천 배치(607), 범위·분할·Player 예산(608), 실제 세계 부모와 미션 Start 연결(609)을 원본 기계어와 대조해 복원했습니다. 609에서는 새 배포본으로 시나리오 옵션 `randGeysers`를 0과 1로 바꿔 2회 실행했고, 둘 다 정상 종료했으며 간헐천이 4개에서 43개로 늘었습니다([609 기록](docs/exe/cpp-world-geyser-20261030.md)).
- **606 (전체 34경로):** 605 제품의 새 배포본에서 Early Missions 6개·Priest Training 9개·캠페인 16개·데모 3개가 모두 진입·짧은 진행·정상 종료했고, 실제 Session 시작과 게임 시각 증가, 실제 창 화면 RGB가 34개 모두 일치했습니다. 1024×768 창 모드의 확인이며 원본 픽셀 비교나 자연 완주의 증거는 아닙니다([606 기록](docs/exe/cpp-solo-current-20261030.md)).
- **605:** 이탈 확인 요청에서 즉시 메인으로 이동하던 차이를 원본 ConfirmRetreat로 고쳤습니다([605 기록](docs/exe/cpp-retreat-confirmation-20261030.md)).
- **그 전:** `Netstorm.exe` 산출물과 배포 런처, 실제 Chat Enter, 이탈의 메인 복귀, 인트로 SMK와 CD 영상 동봉, Edit 시험 전투의 시나리오·동맹 수명, Help 링크 등을 연결했습니다.

### 남은 일

전체 목표는 계속 진행 중입니다. 모든 미션의 자연 완주와 승패, 남은 UI·Edit 도구·시나리오·파일 수명, 외관·음향의 전체 원본 대조, Windows 11 실기와 PCM 음향·사용자 활성 전체화면 검증이 남았습니다. 포획·희생·특정 적 공격·승리 재현은 사용자 확인으로 남깁니다. 이후 멀티플레이·Outpust는 디컴파일만 참고해 무시험 사전 구현하고, 4차 MCP, 5차 저장 위치·커스텀 맵 경로·목록, 6차 한국어 순으로 진행합니다. 7차 안티앨리어싱·FSR 1.0은 별도 후순위 요구입니다. 단위별 근거와 경계는 [인수인계](LEFT_JOBS.md)에 기록합니다.

## C# + MonoGame 클론(dotnetpj)의 최신 상태

기존 실행 경로에서는 캠페인 **1-1 The War Begins!**와 **1-2 Master of Whirligigs**를 플레이할 수 있습니다. 별도로 cpppj 구조에 맞춘 새 C# 클라이언트(`Netstorm.Client`)를 구축하고 있습니다. 화면은 8비트 논리 버퍼·팔레트 기반으로 재구성하며, MonoGame은 호스트(`Netstorm.Client.Host`)와 최종 출력 역할을 맡습니다. [캠페인 1-1](docs/gameplay/campaign-one.md)·[1-2](docs/gameplay/campaign-two.md)의 기존 구현 기록과 [최신 인수인계](LEFT_JOBS.dotnetpj.md)를 참고하세요.

2026-10-28에 계획을 다시 세워(R0~R8) 기존 근사 구현을 동결하고, cpppj가 원본 기계어로 검증한 구조를 그대로 옮기는 방식으로 바꿨습니다. R0(기준선·포트 맵), R1(8비트 화면·MonoGame 출력 호스트·캡처 도구), R2(그리기 원시·팔레트·색표·일반 객체 그리기)를 마쳤고, R3(Gump UI·메인 메뉴 계열)을 진행하다 2026-10-29에 사용자 요청으로 일시 정지했습니다. R3k-8(인계 0.61)까지 Gump 트리·입력, 글꼴·버튼·메뉴, 문자 입력창, 이미지·배경 GIF, HTML 창·전용 도움말 창의 생성과 링크·모달 연결을 옮겼습니다. Release 전체 검사는 경고·오류 0, **2,002개 통과**(Assets 363·Core 1,030·Client 609)입니다. 이는 기존 코드 및 새 모듈 단위 검증을 포함한 결과이며, 새 화면/월드 경로가 실제 게임 세션에 연결됐거나 메인 메뉴 화면이 원본과 픽셀 단위로 일치한다는 뜻이 아닙니다. 재개하면 스크립트 명령·전체 설정 인터페이스·목록 소유 구조, 메시지/대화창, 상위 메뉴 조립을 잇고, 이후 월드 세션(R4)·월드 표시/HUD(R5)·플레이 입력/규칙(R6)·미션 전체/Edit/설정(R7) 순으로 진행합니다. 기존 플레이 범위와 원본 동일성은 아직 1-1·1-2의 근사 경로에 한정됩니다. 순서와 남은 범위는 [인수인계](LEFT_JOBS.dotnetpj.md), 최근 단위는 [HTML 창](docs/dotnet-html-window-20261029.md)·[HTML 창 생성기](docs/dotnet-html-factory-20261029.md) 기록, 이식 근거 목록은 [cpppj → dotnetpj 이식 계획](docs/dotnet-cpp-port-plan-20261016.md)을 참고하세요.

## HTML + JavaScript 웹 클론(webpj)의 최신 상태

브라우저의 오디오 재생 제한에 따라 원본 자산을 적재한 뒤 "클릭하여 시작" 화면을 거쳐 메인 메뉴를 표시합니다. 메인 메뉴의 Edit 버튼은 제거했고, Campaign·Demo·Credits 대화상자와 Early Missions·Priest Training·캠페인 장 목록(5종)을 원본 스크린샷과 대조해 연결했습니다. 2026-10-29 WB03-F에서는 Help·Options 펼침 메뉴(하위 메뉴·켜짐 표시·Esc/바깥 누름 닫기)를 원본 위치에 맞춰 복원하고, 설정 9개를 브라우저 저장소에 저장·복원하며 Sound/Music 켜기·끄기와 음량을 실제 소리에 반영했습니다. Wind Noise·Speaker Swap L/R·Edge Scroll·Auto-Demo·Tell Tips는 값만 저장하며 해당 기능은 아직 없습니다. 2026-10-30 WB03-G에서는 F1·Help 메뉴의 General Help 창(450×350 패널, 스크롤바·끌기·키/휠 스크롤, 링크 이동·Back·OK, 128개 주제)을 복원해 원본 스크린샷과 사실상 픽셀 단위로 일치함을 확인했습니다. 같은 날 WB03-H에서는 캠페인 진행 값(`Done*`)의 쓰기와 즉시 저장, 설정 쓰기 층을 추가해 새로고침 후에도 유지됨을 확인했고, 브라우저 언어가 한국어일 때 Campaign 목록이 비던 문제를 영어 자료로 대체해 고쳤습니다. WB03-I에서는 사용자 결정에 따라 Campaign 목록에서 User Made Campaigns 행과 그 앞 구분선을 뺀 6행(패널 238×268)으로 바꿨고, 나머지 부분은 원본 8행 배치와 글자 모양이 같음을 확인했습니다. 자동 검사 77개가 통과했고 실제 브라우저(Edge 헤드리스) 확인도 통과했습니다.

사용자 결정에 따라 웹에서는 커스텀 맵 Edit와 Campaign 목록의 **User Made Campaigns**를 제공하지 않으며, 두 항목 모두 화면에 나타나지 않습니다. 타입 정보가 있는 도움말 주제의 능력치 머리글·타입 그림, 시작 팁, 한국어 표시, 정적 맵 화면과 실제 게임 플레이는 아직 구현되지 않았습니다. 멀티플레이와 해상도·전체화면 설정 등도 제외합니다. 자세한 상태와 다음 작업은 [최신 인수인계](LEFT_JOBS.webpj.md)를 참고하세요.

# 프로젝트 구성과 개발 목표

`cpppj`는 **① 10.78 싱글플레이 복원 → ② Windows 10/11 안정 구동(DirectDraw 오류 수정 포함) → ③ TCP/IP 로컬 네트워크 멀티플레이·아웃포스트 → ④~⑦ 추가 요구사항** 순서로 진행합니다. 추가 요구사항은 4차 MCP(dotnetpj 개발을 위한 게임 분석 용도), 5차 설정·캠페인 진행 저장을 사용자 홈의 `Saved Games\Netstorm\Saves\`로 이동하고 커스텀 맵을 기존 폴더와 `Saved Games\Netstorm\CustomMaps\` 양쪽에서 불러오며(Edit에서 새로 만든 맵은 후자에 저장) Edit 맵 목록에 스크롤을 넣고 표시 개수 제한을 없애는 것, 6차 한국어 지원, 7차 메인 메뉴 Options에서 선택하는 안티앨리어싱과 FSR 1.0 업스케일(기본값 꺼짐)입니다. 5차 전까지 저장 위치는 원본과 같은 게임 자료 폴더입니다. Windows 전용이며, 멀티플레이 전용 아웃포스트는 3차 목표에 포함합니다. 공식 서버 접속은 복원하지 않습니다. CD판 10.72의 비교·분석 기록도 보존합니다.

`dotnetpj`는 C# + MonoGame 기반입니다. 처음에는 이미 구현된 클론에 `cpppj`가 검증한 부분을 조각별로 옮기는 방식(2026-10-16 이식 계획)이었으나, 기존 구현이 원본과 다른 곳이 많아 2026-10-28에 계획을 다시 세웠습니다. 기존 근사 구현은 동결하고, cpppj 구조(8비트 화면·Gump UI·월드 세션)를 그대로 옮긴 새 `Netstorm.Client`로 R0~R8 순서에 따라 교체하며, cpppj를 같은 입력의 비교 기준으로 씁니다. 원본과 완전히 같은 동작을 먼저 구현하고, `cpppj`가 완성되면 전체를 다시 대조한 뒤 추가 요구사항을 반영합니다. cpppj의 4~7차 목표(MCP, 저장 위치 이동·커스텀 맵 경로·Edit 목록 스크롤, 한국어 지원, 안티앨리어싱·FSR 1.0)도 후순위 요구사항에 포함됩니다. Windows 실행 파일 이름은 `Netstorm.exe`로 맞춰야 합니다. GUI Linux 지원과 멀티플레이는 후순위입니다. 최종 요구사항에는 전체화면·16:9/16:10/4:3 화면비·전체화면 가장자리 스크롤·영어/한국어 지원이 포함됩니다. 애니메이션은 원본 수준을 먼저 복원한 뒤 60/120프레임 지원을 진행합니다.

`webpj`는 HTML + JavaScript 기반입니다. `cpppj`가 완성되면 이를 분석해 원본과 같은 동작을 목표로 개발합니다. 커스텀 맵(Edit)·User Made Campaigns·멀티플레이, 웹 환경에 맞지 않는 기능(게임 종료, 해상도·전체화면 설정 등)은 제외하며, 필요한 에셋은 `webpj/www/` 안으로 복사해 사용합니다.

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

한국어 건물·유닛 이름은 [공통 명칭 규칙](docs/korean-unit-names.md)을 따릅니다. 사제·신전·다리·발사기·간헐천·제단·거주 구역(Residence)·고갈된(Empty)은 번역할 수 있으며, 나머지는 워크샾·제네레이터처럼 영어 발음을 한글로 표기합니다. dotnetpj의 메뉴·유닛 제목에는 공유 이름표 55개를 연결했습니다. 도움말·미션 본문의 전체 번역은 아직 완료하지 않았습니다.

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

```sh
cmake -S cpppj -B cpppj/build -DCMAKE_BUILD_TYPE=Release
cmake --build cpppj/build --config Release
ctest --test-dir cpppj/build --build-config Release --output-on-failure
```

CTest는 래퍼 검사 하나로 등록되어 있으며, 현재 등록된 C++ 검사는 3,514개입니다. 전체 실행은 수 분이 걸리며, 각 단위의 집중 검사와 전체 검사 결과는 [인수인계](LEFT_JOBS.md)에서 구분해 확인할 수 있습니다. Visual Studio 생성기 기준 실행 파일은 `cpppj/build/bin/Release/Netstorm.exe`입니다. 다음 명령은 창 모드로 C++ 복원판의 메인 메뉴를 엽니다. 메인 메뉴의 Campaign에서 캠페인 1장의 첫 미션 **The War Begins**를 선택하면 캠페인 1-1을 시작합니다.

```powershell
.\cpppj\build\bin\Release\Netstorm.exe --run originals --window
```

`--run <게임 데이터 폴더>`는 C++ 복원판을 실행합니다(원본 게임을 실행하지 않습니다). `--mission <미션 키>`를 붙이면 메뉴를 거치지 않고 해당 미션의 브리핑으로 들어갑니다(예: `--mission Tutorial1`, 캠페인 1-1은 `--mission TheWarBegins`). `--view fonts`, `--view types`, `--view TEST01`은 글꼴·타입·미션 검사 장면입니다. 인자 없이(또는 `--help`로) 실행하면 빌드 정보와 사용할 수 있는 옵션을 표시합니다. 실제 건설·생산·다리·채취/운반·전투와 공통 전략 AI의 자동 실행이 연결돼 있으나, 목표 미션 전체의 자연 완주, Edit의 나머지 편집 도구·시나리오/수명과 원본 동일성 검증은 미완료입니다. 빌드 절차는 [C++ 빌드](docs/cpp-build.md), 실제 최신 연결 상태는 [인수인계](LEFT_JOBS.md)를 참고하세요.

## 배포판 압축파일 만들기와 실행

배포용 압축파일(실행 파일·Visual C++ 런타임·원본 10.78 게임 데이터·실행 배치 파일)은 다음 스크립트로 만듭니다. 저장 위치를 생략하면 실행 중에 입력받고, 빈 값을 입력하면 취소합니다. 구성과 옵션은 [C++ 빌드 7절](docs/cpp-build.md)을 봅니다. 받는 PC에서는 압축을 풀고 `Netstorm.bat`(창 모드) 또는 `Netstorm-SavedMode.bat`(저장된 화면 모드)을 실행하면 되며, 설정 파일을 쓰므로 압축을 푸는 폴더는 쓰기 가능해야 합니다. 사용자별 설정·진행 저장 파일(`d/options.cfg`, `fullscreenStateFile.dat`)은 압축에 넣지 않아 받은 PC의 첫 실행에서 새로 만들어집니다. Windows PowerShell 5.1의 기본 증분 빌드→압축, 새 압축 해제본의 첫 실행·저장 전체화면 재실행·창 런처, 최신 609 배포본(1,376항목)의 실제 실행을 확인했습니다. 전체 미션/동일성과 Windows 11 배포 검증은 별도입니다.

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File cpppj\package-cpppj.ps1 D:\dist\NetstormReborn-cpppj.zip
```

# C# + MonoGame 클론 빌드와 실행 (dotnetpj, 개발 일시 정지)

아래 명령은 프로젝트 루트(`NetstormReborn`)에서 실행합니다. 게임 본체는 C# / MonoGame DesktopGL이며 대상 프레임워크는 `net10.0`입니다. 현재 빌드되는 실행 파일은 기존 근사 구현 경로(`NetstormClone`)입니다.

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
| Windows 10/11 x64 | `dist/win-x64/` | `NetstormClone.exe` |
| Linux x64 | `dist/linux-x64/` | `NetstormClone` |

`--self-contained true`는 .NET 런타임까지 포함하므로 실행할 PC에 .NET SDK나 런타임을 따로 설치할 필요가 없습니다. SDL2·OpenAL 네이티브 라이브러리, `fonts/`, `game-data/`도 출력에 포함됩니다. 실행 파일은 함께 생성된 DLL·런타임 파일·글꼴·게임 데이터를 사용하므로 **출력 폴더 전체를 복사해 배포**합니다. 그래픽 드라이버와 Linux GUI 환경 등 OS의 실행 환경은 별도로 필요합니다.

실행할 PC에 .NET 10 런타임이 이미 설치되어 있다면 더 작은 배포물을 만들 수 있습니다. 위 명령의 `--self-contained true`를 `--self-contained false`로 바꾸고, 출력 폴더도 `dist/win-x64-fdd` 또는 `dist/linux-x64-fdd`처럼 구분합니다.

배포용 압축파일(zip)은 다음 스크립트로 만듭니다. 저장 위치를 생략하면 실행 중에 입력받고, 빈 값을 입력하면 취소합니다. 기본은 받는 PC에 .NET 10 런타임을 미리 설치하는 방식이며, `-SelfContained`를 붙이면 런타임까지 포함합니다. 임시 폴더에 게시하므로 저장소에 `dist/`를 남기지 않습니다.

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File dotnetpj\package-dotnetpj.ps1 D:\dist\NetstormReborn-dotnetpj.zip
```

## 포함된 게임 데이터와 바이너리 실행

빌드·배포 시 `assets/game-data/`의 내용이 출력의 **`game-data/`에 자동으로 복사**됩니다. Windows 배포 폴더는 다음과 같은 구조가 됩니다(Linux도 동일하며 실행 파일 이름은 `NetstormClone`입니다).

```text
dist/win-x64/
  NetstormClone.exe
  NetstormClone.dll
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
.\dist\win-x64\NetstormClone.exe --language korean
```

Linux에서 실행:

```sh
chmod +x dist/linux-x64/NetstormClone
./dist/linux-x64/NetstormClone --language korean
```

일반 실행은 함께 배포된 데이터를 자동으로 찾습니다. 개발·검증용 데이터를 다른 위치에서 사용하려면 `NETSTORM_DATA`에 **`netstorm.tarc`가 들어 있는 폴더의 절대 경로**를 지정합니다. 탐색 순서는 유효한 `NETSTORM_DATA` → 실행 파일·작업 폴더 및 그 상위의 `game-data/` 또는 `assets/game-data/`입니다.

```powershell
$env:NETSTORM_DATA = "D:\Games\NetstormReborn\game-data"
.\dist\win-x64\NetstormClone.exe --language korean
```

```sh
NETSTORM_DATA="/절대/경로/game-data" ./dist/linux-x64/NetstormClone --language korean
```

# 웹 클론 실행 (HTML + JavaScript)

Node.js가 필요하며(v24에서 검증) 외부 npm 패키지는 쓰지 않습니다. 필요한 에셋은 `webpj/www/data/`에 커밋돼 있습니다. 프로젝트 루트에서 다음 명령으로 정적 웹 서버를 시작한 뒤 브라우저에서 `http://localhost:9803/`을 엽니다.

```sh
npm --prefix webpj start
```

자동 검사는 `npm --prefix webpj test`로 실행합니다. `assets/game-data/`의 원본 자료가 바뀌면 `npm --prefix webpj run prepare-assets`로 `webpj/www/data/`를 다시 만들고, `npm --prefix webpj run check-assets`로 복사본을 점검합니다.

# 주요 문서

- [cpppj·공통 인수인계](LEFT_JOBS.md) / [dotnetpj 인수인계](LEFT_JOBS.dotnetpj.md) / [webpj 인수인계](LEFT_JOBS.webpj.md): 최신 상태와 앞으로 할 작업. 지난 단위의 상세 이력은 [LEFT_JOBS.old.md](LEFT_JOBS.old.md)(cpppj·공통)와 [LEFT_JOBS.dotnetpj.old.md](LEFT_JOBS.dotnetpj.old.md)(dotnetpj)에 있습니다.
- [TODO.md](TODO.md): cpppj의 현행 목표 프롬프트(Early Missions·Priest Training·캠페인·데모·Edit·메뉴·게임 중 UI)
- [C++ 복원 로드맵](docs/cpp-roadmap.md) / [실제 플레이 복원 계획](docs/cpp-playable-plan.md): 개발 단계와 복원 계획
- [cpppj → dotnetpj 이식 계획](docs/dotnet-cpp-port-plan-20261016.md): 이식 단위(W 번호)와 근거 목록. 현행 순서(R0~R8)는 [dotnetpj 인수인계](LEFT_JOBS.dotnetpj.md) 0절이 우선합니다.
- [클론 게임 데이터](assets/README.md): 자산 구성·출처·빌드 포함 방식
- [원본 실행·별도 창 확인 규칙](docs/execution-confirmation.md): AI 작업의 실행 조건

# License

MIT License
Copyright (c) 2026 HJOW
