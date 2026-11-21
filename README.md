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

**2026-11-21 기준:** 주력인 `cpppj`는 제품 1323(표시 11.01)입니다. AGENTS.md의 1~14차 목표 구현과 최종 배포 관문(M8, 제품 1310 기준)을 마친 뒤 2026-11-17 사용자 지시로 재개해 최초 실행 상태, 미션 전환 종료 제보, 한국어 갈무리11 글꼴, 원본 실행 파일 아이콘을 반영했습니다. `dotnetpj`는 새 경로의 월드 세션 조립(R4)에서 AI 런타임·정기 처리에 이어 내부 로그인·전송·Client 전역과 장기 실행 수명을 옮겨 왔고, 단위 0.732(전체 검사 5,233건 통과)에서 사용자 요청으로 일시 중지했습니다. `webpj`는 WB04-E1131까지 브라우저에서 캠페인 목록의 31개 미션과 데모·스커미시에 진입해 진행할 수 있게 됐고, 게임 플레이 중 UI의 나머지(지식·채팅 창, 객체 메뉴)와 미션 목표 수행을 구현하고 있습니다. 단위별 근거는 [cpppj·공통 인계](LEFT_JOBS.md), [dotnetpj 인계](LEFT_JOBS.dotnetpj.md), [webpj 인계](LEFT_JOBS.webpj.md)에 있습니다.

| 프로젝트 | 실행·플레이 가능 범위 | 원본과의 동일성 및 남은 범위 |
| --- | --- | --- |
| **cpppj — C++ 복원판 (주력)** | **플레이 가능.** 원본 10.78의 메뉴, Early Missions 6개·Priest Training 9개·캠페인 16개·데모 3개(34경로), Edit(시나리오 편집 포함), 스커미시를 플레이할 수 있습니다. 영어·한국어·일본어 UI, 16:9 최대 1920×1080(FSR 1.0으로 2560×1440·3840×2160), 난이도, Frame Rate 60/75/120, 로컬 MCP 서버, TCP/IP 로컬 멀티플레이·Outpost 구현을 포함하며, 배포 ZIP은 압축을 풀어 바로 실행합니다. | 부품마다 원본 10.78 기계어와 대조해 복원했고 AGENTS.md 1~14차 구현을 마감했습니다(14차는 QuickJS 엔진 탑재까지). 모든 미션의 자연 완주와 전체 동일성, 멀티플레이 실제 실행(실행 검증 0), 한국어·일본어 사람 검수, 실물 4K·다른 모니터/DPI·Windows 11/ARM 실기는 사용자 확인 대상입니다. 알려진 버그 34건은 판정을 마쳤고(수정 10건) 12건은 재현 조건이 더 필요합니다. CD판 10.72 복원은 메뉴까지 확인한 뒤 보류했습니다. 제품 1323으로 M8 배포 관문을 다시 돌리는 일이 남았습니다. |
| dotnetpj — C# + MonoGame 클론 | 기존 경로(`Netstorm.exe`)에서 캠페인 **1-1 전쟁의 시작!(The War Begins!)**, **1-2 훨리기그의 지배자(Master of Whirligigs)**를 근사 구현으로 플레이할 수 있습니다(영어·한국어 UI, 동결 상태). cpppj 구조를 그대로 옮기는 새 경로 호스트(`Netstorm.Client.Host`)는 타이틀·메인 메뉴·Help·Options·Credits·캠페인 선택·Edit 목록과, 개발용 미션 직행(`--mission`)의 세션·지형·요새 객체 적재와 AI 생성·정기 처리까지 실행됩니다. 월드 화면·입력·규칙이 없어 **새 경로로는 아직 플레이할 수 없습니다.** | 새 경로는 메뉴 12장면이 cpppj와 픽셀 단위로 같고, 부품은 원본 x86 대조 자료 363개와 맞췄습니다. 미션 시작 상태 비교는 cpppj 기준선 34경로와 dotnetpj 31미션 덤프를 확보했으나 전체 일치는 아직 0/34입니다. cpppj 소스 1,239단위(169,296줄) 가운데 499단위(86,773줄)에 대응 코드가 있고 **740단위(82,523줄)가 남았습니다**(2026-11-21 `tools/port_inventory.py` 집계, 대응 단위에도 미이식 분기가 남을 수 있음). 0.732 전체 검사는 Assets 366개·Core 3,363개·Client 1,504개 통과입니다. |
| webpj — HTML + JavaScript 웹 클론 | "클릭하여 시작" 뒤 원본 자산으로 메인 메뉴를 표시합니다. Campaign·Demo·Credits(11.01 Patch Credits 포함 3항목)·Help·Options(설정 저장, 언어·난이도·Frame Rate 선택, Reset All)·F1 도움말이 동작하고, 한국어에서는 갈무리11 글꼴로 메뉴와 모든 미션 대본을 표시합니다. **미션을 브라우저에서 진행할 수 있습니다:** 캠페인 목록의 31개 미션(Early Missions 6·Priest Training 9·캠페인 16)이 시작돼 장시간 진행에서 오류가 없고, 데모 3개의 360초 반복과 스커미시(무작위 섬·간헐천, AI 수·금지 유닛 설정) 진행도 확인했습니다. Tutorial2는 시작부터 완료 창까지 자연 진행으로 확인했고, ESC 게임 메뉴(목표 보기·Pause·Restart/Leave Mission 등)·단축키·미션 음악이 동작합니다. 원본 도움말의 조작키(화면 위치·회전·D·Pause 등)와 승패 안내 뒤 흐름(Replay·Next Mission·진행 저장)도 확인했습니다. 지식(NSInfo) HUD 패널·채팅 창 내용물, 나머지 객체 메뉴와 다른 미션의 목표 수행·완주는 아직입니다. | 메뉴 8장면이 cpppj 실행 결과와 (원인을 기록한 구름 배경을 빼고) 픽셀 차이 0입니다. 이식 대상 1,022단위 중 **887단위(86.8%)**를 원본 10.78 대조 자료와 맞추며 옮겼고 135단위가 남았습니다. Tutorial1 시작 직후 상태 덤프는 시간 의존 값을 빼고 cpppj 기준선과 구조가 같음을 확인했으며, 고정 시각 L2·L3·화면 비교와 자연 완주는 아직 검증하지 않았습니다. 마지막 전체 자동 검사는 Node v26·v22에서 6,702건 모두 통과입니다. 웹에서는 Edit·User Made Campaigns·멀티플레이를 제공하지 않습니다. |

## C++ 복원판(cpppj)의 최신 상태

### 지금 할 수 있는 것

- **미션 34경로:** Early Missions 6개, Priest Training 9개, 캠페인 16개(1·2·3장), 데모 3개에 진입해 진행합니다. 캠페인 1-1·1-2는 사용자가 직접 끝까지 플레이해 다음 미션 안내까지 확인했고, 세 데모는 360초 주기 반복을 확인했습니다. 전체 34경로를 한꺼번에 다시 확인한 마지막 기록은 제품 644입니다.
- **원본 메뉴와 연출:** 메인 메뉴의 Campaign·Demo·Help·Options·Credits·Quit, 브리핑, CD 인트로 영상(SMK)을 원본 구조대로 재현합니다. Credits에는 세 번째 항목 **Netstorm 11.01 Patch Credits**(`Copyright 2026 HJOW`)가 있습니다.
- **게임 규칙과 UI:** 건설·생산·다리·채취/운반·전투, 워크샾 유닛 25종, 포획·희생·의식, 기술 획득, 공통 AI, 간헐천 생성을 원문 대조로 복원했고, 덱·미니맵·QWASZX·ESC 메뉴·채팅·우클릭 메뉴·커서·가장자리 스크롤을 연결했습니다.
- **멀티플레이(3차):** TCP/IP 로컬 네트워크 멀티플레이와 Outpost를 구현하고 정적 대조를 마쳤습니다. 공식 서버 접속과 클라이언트 유효성 검사는 복원하지 않습니다. 실제 LAN 실행 검증은 사용자 확인으로 남아 있습니다.
- **로컬 MCP·와이드(4차):** `Netstorm.exe --mcp`로 게임을 stdio MCP 서버로 실행해 조작·상태 조회·화면 PNG·소리 포함 녹화를 할 수 있고([MCP 기록](docs/exe/cpp-mcp-server-20261106.md)), 16:9 해상도를 1920×1080까지 지원합니다.
- **저장 위치(5차):** 설정·진행·커스텀 맵·로그 등 게임이 만드는 파일을 모두 `%USERPROFILE%\Saved Games\Netstorm\` 아래에 저장합니다. 관리자 권한 없이는 쓸 수 없는 폴더에 설치해도 동작합니다. Edit 맵 목록은 개수 제한 없이 스크롤합니다.
- **한국어(6차)·일본어(13차):** Options > Language에서 한국어를 고르면 갈무리11(Galmuri11) 비트맵 글꼴로(1322) 메뉴·창·미션 본문이 한국어로 바뀝니다. 일본어는 UI(번역표 509키, Noto Sans JP)까지 지원하고 미션 본문은 영어로 대신합니다. 글꼴은 실행 파일에 내장하지 않고 `fonts\` 폴더에서 읽습니다.
- **오류 로그(7차):** 비정상 종료 직전의 상세 로그를 `%USERPROFILE%\Saved Games\Netstorm\logs\yyyyMMddHHmmss.log`에 남깁니다([오류 로그](docs/exe/cpp-error-log-20261111.md)).
- **안티앨리어싱·FSR 1.0(8차):** Options에서 선택하며 기본은 꺼져 있습니다. Direct3D 11 장치가 있으면 2560×1440·3840×2160을 고를 수 있고, 이때 1920×1080으로 그린 화면을 FSR 1.0으로 업스케일합니다.
- **시나리오 편집(9차):** Edit에서 초기 자본·테크·적 구성·동맹 등 시나리오 값을 화면에서 편집하고, 새 맵의 `.english`를 자동 생성합니다. `.english` 없이 Test Battle에 들어갈 때 종료되던 문제를 고쳤습니다.
- **난이도(11차)·스커미시(12차):** Options의 Difficulty(쉬움·보통·어려움)와 Campaign 메뉴의 Skirmish(적 AI 수·AI별 난이도/동맹·금지 유닛 최대 5개, 멀티플레이 규칙의 무작위 섬·간헐천)를 지원합니다([난이도](docs/exe/cpp-difficulty-d-20261111.md)·[스커미시](docs/exe/cpp-skirmish-sk-20261111.md)).
- **프레임·QuickJS(14차):** Options의 Frame Rate로 60/75(원본)/120을 고릅니다. QuickJS-ng v0.18.0 엔진을 정적 탑재했으며 게임 상태·파일·네트워크는 노출하지 않습니다([QuickJS](docs/exe/cpp-quickjs-engine-20261111.md)).
- **Windows 10/11 대응:** 원본에서 전체화면 종료 후 재실행하면 오류가 나던 문제가 생기지 않습니다. 배포판은 압축을 풀고 바로 실행합니다. 실행 파일 아이콘은 원본과 같습니다(1323).

### 최근 진행

- **1314~1323 (2026-11-16~19, 사용자 재개 지시):** 최초 실행 상태(음량 기본값·첫 안내 창·캠페인 잠금, 1314), 한국어 유닛 정보 오역(1315), 미션 전환 때 이전 연결 종료 제보 수정(1320), 한국어 글꼴의 갈무리11 비트맵 전환(1322), 원본 실행 파일 아이콘과 공통 ICO(1323)를 반영했습니다. 1316~1319·1321은 검사 기준선 교정입니다.
- **1312~1313 (2026-11-14):** 명령줄 전용 테스트 자동 플레이(`--test-autoplay` 등: 로컬 플레이어에 AI를 붙여 캠페인·스커미시를 자동 진행, 관전 카메라)와 일괄 실행 도구를 넣고 캠페인 1-1 승리를 재현했습니다. 일반 실행·옵션·메뉴에는 나타나지 않습니다([사용 안내](docs/testing/cpp-autoplay-test-guide.md)).
- **1311 (M8 최종 배포 관문):** Release 빌드, ZIP 1,436항목 전수 대조, 배포본 5경로 실행(영어·한국어·일본어 메뉴, 미션, 스커미시), 외부 접속 감사를 통과했습니다([최종 보고](docs/exe/cpp-m8-final-gate-1310-20261114.md)). 이후 제품 코드가 바뀌었으므로 다음 배포 전에 관문을 다시 돌려야 합니다.
- **1302~1310 (CD판 10.72):** 판본 옵션·기본 해상도·메시지·CD 자료 경로·메뉴 표시까지 복원한 뒤 사용자 결정으로 보류했습니다([보류 인계](docs/exe/cpp-cd1072-deferred-20261114.md)).
- **1285~1301:** 알려진 버그 34건 판정과 수정 10건([현황표](docs/exe/cpp-known-bugs-status-20261111.md)), 일본어 UI, 11.01 카피라이트, 난이도, 스커미시, Frame Rate 60/120, QuickJS 엔진 탑재.
- **987~1284:** 3차 LAN 멀티플레이·Outpost 구현과 정적 대조 마감(~1279), 오류 로그(1269), 한국어 K1~K5(1270~1281, [마감](docs/exe/cpp-korean-k4-k5-closure-20261111.md)), 시나리오 편집 S1~S4(1277~1284, [읽기 전용 설치 확인](docs/exe/cpp-scenario-s4-readonly-home-20261111.md)).
- **그 전 (~986):** 비멀티 UI(929)·Edit(943)·미션 수명(956)·내부 수명/파일(980) 마감, 4차 MCP·와이드, 5차 저장 위치, AA·FSR 1.0. 지난 단위의 상세 이력은 [LEFT_JOBS.old.md](LEFT_JOBS.old.md)에 있습니다.

### 남은 일

2026-11-14 일시 중지 뒤 2026-11-17 사용자 지시로 목표를 재개했습니다. 남은 것은 ① 사용자 확인 항목(모든 미션의 자연 완주와 전체 동일성, 포획·희생·특정 적 공격·승리의 실제 재현, 멀티플레이 실행, 한국어·일본어 검수, 실물 4K·다른 모니터/DPI·Windows 11/ARM), ② 사용자 결정이 필요한 선택 작업(CD판 10.72 복원 재개, 비행체 표시 보간, 일본어 본문 번역과 추가 언어, 알려진 버그 12건의 재현 조건, QuickJS 연동 범위), ③ 허용된 기존 검사 실패의 분류와 제품 1323의 새 ZIP·M8 관문 재실행(다음 AI 작업)입니다. 자세한 내용은 [인수인계](LEFT_JOBS.md)에 있습니다.

## C# + MonoGame 클론(dotnetpj)의 최신 상태

실행 경로가 둘입니다. **기존 경로**(`Netstorm.Game`, 현재의 `Netstorm.exe`)는 녹화와 관찰을 바탕으로 만든 근사 구현으로 캠페인 **1-1 The War Begins!**와 **1-2 Master of Whirligigs**를 플레이할 수 있으며, 새 기능을 넣지 않는 동결 상태입니다. **새 경로**(`Netstorm.Client` + `Netstorm.Client.Host`)는 cpppj의 파일·클래스·호출 순서·메모리 배치를 그대로 옮기는 직역이며, 화면은 8비트 논리 버퍼와 팔레트로 그리고 MonoGame은 창·입력·오디오 장치와 최종 출력만 맡습니다. 새 경로가 메뉴와 미션을 모두 맡게 되면 `Netstorm.exe` 이름을 넘겨받고 기존 경로는 삭제합니다.

지금까지 R0(기준선·대응표), R1(8비트 화면 호스트·비교 도구), R2(그리기 원시·팔레트·글꼴)를 마쳤고, R3(Gump UI·메뉴)는 메뉴 12장면이 cpppj와 픽셀까지 같습니다. **R4(월드 세션 조립)가 진행 중**으로, 미션 Fort 적재·Player·동맹·지형·섬·요새 객체·그래프와 객체 수명 공통층(공장·Pop/Unpop·삭제·피해·연소·사제·보호막)을 호스트에 연결했습니다. 0.639~0.688에서는 AI 런타임(생성·준비·전략·전체 프레임·통지·월드 등록·수집자·경제·시설 생산)과 장식·Player·스프라이트·매몰 정기 처리, 업그레이드·연구·생산 Deck, 플레어·건설 표시 장치를, 0.689~0.731에서는 내부 로그인·패킷 전송·Client 전역과 실행 객체·Player·State의 장기 수명, Client 프레임 실행 순서를 원문 순서대로 옮겨 세션에 결선했습니다. 미션 시작 상태를 cpppj와 비교할 34경로 기준선은 갖췄지만 아직 일치하는 경로가 없습니다(0/34). 남은 차이는 적재 중 후처리(농지 프레임·소유자, 그래프 번호, 장부), 정기 처리, AI 시작, 로그인·Root 전체 연결 등입니다. 2026-11-21 사용자 요청으로 0.732(전체 검사 통과, 65경로 시작 재수집은 미실행)에서 일시 중지했고, 재개하면 그 재수집과 0.733 실행 객체 반납 경계부터 진행합니다.

남은 작업은 cpppj 소스와의 대조로 단계마다 체크 목록을 만들었습니다: **R4** 세션 조립 마무리(R4-1~R4-8) → **R5** 월드 표시·카메라·HUD와 `Netstorm.exe` 전환 → **R6** 입력·규칙·AI·소리(가장 큰 단계) → **R7** 미션 전체·Edit·설정·배포 → **R8** AGENTS 4~15차 → **R9** 60/120프레임 → **R10** Linux → **R11** 로컬 멀티플레이. 마지막 전체 검사(0.732)는 **Assets 366개·Core 3,363개·Client 1,504개** 통과입니다(기존 경로 검사가 포함된 값이라 새 경로의 진행도와 같지 않습니다). 순서와 체크 목록은 [인수인계](LEFT_JOBS.dotnetpj.md), 이식 근거 목록은 [cpppj → dotnetpj 이식 계획](docs/dotnet-cpp-port-plan-20261016.md)을 참고하세요.

## HTML + JavaScript 웹 클론(webpj)의 최신 상태

브라우저의 오디오 재생 제한에 따라 원본 자산을 적재한 뒤 "클릭하여 시작" 화면을 거쳐 메인 메뉴를 표시합니다. Campaign·Demo·Credits 대화상자와 Early Missions·Priest Training·캠페인 장 목록, Help·Options 펼침 메뉴(설정 저장, 소리·음악·음량 반영, 언어·난이도·Frame Rate 선택), F1 General Help 창, 시작 환영/팁 창, 캠페인 진행 값(`Done*`) 저장을 구현했고, 메뉴 8장면은 cpppj 실행 결과와 픽셀 단위로 대조했습니다. 제품 표기는 `v11.01`이며 Credits에는 세 번째 **Netstorm 11.01 Patch Credits** 항목(`Copyright 2026 HJOW`)이 있습니다. 원본 자료의 바깥 링크는 새 탭/창으로 엽니다. 웹 런타임의 외부 접속은 [외부 접속 목록](docs/webpj-external-access-20261105.md)에 정리했습니다. 설정과 진행은 브라우저 localStorage에 저장하며, Options 맨 아래 Reset All은 확인 뒤 이 게임의 설정·진행 저장값만 지우고 다시 로딩합니다. 탭 아이콘(favicon)은 원본 실행 파일 아이콘입니다.

언어는 Options의 Language나 브라우저 언어로 정합니다. 한국어에서는 갈무리11 글꼴(일반·굵은 woff2)과 원본 방식의 언어 파일로 메인 메뉴·펼침 메뉴·대화상자·캠페인 목록·F1 도움말(주제 128개 중 126개)을 표시하고, 글꼴을 읽지 못하면 영어로 대신합니다. 미션은 튜토리얼 6개·캠페인 16개·데모·훈련 미션의 대본 전부와 게임 안 ESC 메뉴·상태 안내·진행 창, 사제·Player·워크샾 생산 메뉴와 객체 우클릭 메뉴·지식 창·Player 이름을 한국어로 옮겼고(메뉴·객체 메뉴의 번역 누락 없음), 번역 호출이 없는 생산 창·미니맵 도움말·채팅 채널 이름 등 일부 고정 영어 문구가 남았습니다.

미션을 위해 cpppj의 싱글플레이 범위를 파일 단위로 옮기고 있습니다. 기준 cpppj 판(제품 1313)의 이식 대상 1,022단위 중 **887단위(86.8%)**를 원본 10.78 대조 자료와 맞추며 옮겼고, 남은 135단위는 대부분 부품을 한 세션으로 묶는 허브(`RawCampaignSession`·`ClientMain`·`GameWorld`·`UberGump`·`RawMenuControls`)와 Native 장치·게임 메뉴·소리입니다. 옮긴 부품을 실제 미션 앱 프레임에 결선해 **브라우저에서 미션을 진행할 수 있게 됐습니다.** 캠페인 목록의 31개 미션이 연달아 시작되고 장시간 진행에서 오류가 없으며, 데모 3개는 360초마다 새 월드로 다시 시작하고, Campaign ▸ Skirmish는 무작위 섬·간헐천 월드를 만들어 AI와 진행합니다. Tutorial1·2에서는 사제 선택·이동, 가장자리 스크롤, 생산 덱의 클릭·단축키와 다리 배치, 사제 우클릭 메뉴의 신전 건설·완공, 워크샾 우클릭 메뉴의 무기 생산을 확인했고 Tutorial2는 완료 창까지 자연 진행했습니다. 좌측 사이드바·미니맵 칸은 원본 위치로 도킹했고, ESC 게임 메뉴(목표 보기·건물 숨김·Pause·Restart/Leave Mission·플레이어 목록 등)와 미션 장면 음악, Shift-F7 화면 저장과 원본 도움말의 조작키(Alt·가운데 버튼 스크롤, TAB·U·숫자 화면 위치, C 회전, D, Pause 키 등)가 동작하며, 승패 안내 뒤 Replay·Next Mission과 진행 저장도 확인했습니다. 다음은 지식(NSInfo)·채팅 창 등 HUD 나머지, 남은 객체 메뉴, 다른 미션의 목표 수행, Tutorial1 시작 상태의 고정 시각 L2·첫 화면 대조입니다(시작 직후 덤프의 구조 일치는 확인). 마지막 전체 검사는 Node v26(E1166)·v22(E1161)에서 **6,702건 모두 통과**입니다. 이후 단계는 5 화면·HUD·미션 진입(M2) → 6 입력·경제·건설(M3) → 7 전투·의식·AI → 8 미션 수명·소리(M4) → 9 표시 품질·배포(M5) → 10 후순위 요구(4~15차 중 웹 해당분)입니다.

사용자 결정에 따라 웹에서는 커스텀 맵 Edit, Campaign 목록의 **User Made Campaigns**, 멀티플레이, 해상도·전체화면 설정, 게임 종료 등을 제공하지 않습니다(12차 스커미시는 웹에서도 제공합니다). 자세한 상태와 다음 작업은 [최신 인수인계](LEFT_JOBS.webpj.md)를 참고하세요.

# 프로젝트 구성과 개발 목표

`cpppj`는 **① 10.78 싱글플레이 복원 → ② Windows 10/11 안정 구동(DirectDraw 오류 수정 포함) → ③ TCP/IP 로컬 네트워크 멀티플레이·Outpost → ④~⑮ 추가 요구사항** 순서로 진행합니다. 추가 요구사항은 다음과 같습니다.

- **4차:** 로컬 MCP(dotnetpj 분석을 위한 모든 게임 조작·화면 캡처·소리 포함 녹화)와 16:9 최대 1920×1080 해상도 지원(그래픽 크기 스케일 조정은 불필요)
- **5차:** 설정·캠페인 진행 저장을 사용자 홈의 `Saved Games\Netstorm\Saves\`로 옮기고, 게임이 자동으로 만드는 모든 파일·폴더를 `Saved Games\Netstorm\` 아래로 옮겨 관리자 권한 없이 쓸 수 없는 위치에 설치해도 동작하게 합니다. 커스텀 맵은 기존 폴더와 `Saved Games\Netstorm\CustomMaps\` 양쪽에서 불러오고(Edit에서 새로 만든 맵은 후자에 저장), Edit 맵 목록에 스크롤을 넣고 표시 개수 제한을 없앱니다.
- **6차:** 한국어 지원
- **7차:** 오류/예외 처리와 로깅 공통 구조. 비정상 종료 직전의 상세 로그를 `Saved Games\Netstorm\logs\yyyyMMddHHmmss.log`에 남깁니다.
- **8차:** 메인 메뉴 Options의 안티앨리어싱과 FSR 1.0 선택(기본 꺼짐). FSR 지원 환경에서는 2560×1440·3840×2160을 고를 수 있고, 이때 FSR을 자동으로 켜서 실질 1920×1080 화면을 목표 해상도로 업스케일합니다.
- **9차:** Edit에서 맵·구성요소 배치뿐 아니라 시나리오 자체(플레이어 초기 자본·테크, 적 구성·동맹 등)를 화면에서 편집하는 전용 UI, 새 맵을 만들 때 기본 `.english` 파일 자동 생성(`.english` 없이 Test Battle에 들어가면 종료되는 문제 수정)
- **10차:** 원본 10.78에 알려져 있던 버그의 확인과 수정
- **11차:** 난이도(쉬움·보통·어려움). 쉬움은 적 AI의 다리 건설·유닛 생성 주기가 4배로 늘고 자원채취 유닛 수가 절반이 되며, 어려움은 주기가 2배 빨라집니다.
- **12차:** 스커미시 모드. 멀티플레이처럼 무작위로 배치된 섬에서 원하는 수의 AI와 난이도·동맹·금지 유닛을 정해 플레이합니다.
- **13차:** 기타 언어 지원(일본어)
- **14차:** QuickJS 엔진 탑재(연동 범위는 추후 결정)
- **15차:** 설정 초기화. Options 맨 아래 Reset All을 다시 한번 확인받은 뒤 `options.cfg`를 비롯한 설정과 게임 진행을 모두 초기화하고 게임을 종료합니다.

`cpppj`는 1~14차 구현을 마감했습니다(14차는 엔진 탑재까지). 2026-11-18 AGENTS.md에 추가된 15차(설정 초기화)는 아직 구현하지 않았습니다. 자연 완주·전체 동일성·멀티플레이 실제 실행 같은 사용자 확인 항목이 남아 있습니다. Windows 전용(x86-64 대상, Windows 11 ARM 고려)이며, 멀티플레이 전용 Outpost는 3차 목표에 포함합니다. 공식 서버 접속은 복원하지 않습니다. CD판 10.72는 메뉴까지 복원한 뒤 보류했고 비교·분석 기록을 보존합니다.

`dotnetpj`는 C# + MonoGame 기반입니다. 처음에는 이미 구현된 클론에 `cpppj`가 검증한 부분을 조각별로 옮기는 방식(2026-10-16 이식 계획)이었으나, 기존 구현이 원본과 다른 곳이 많아 2026-10-28에 계획을 다시 세웠습니다. 기존 근사 구현은 동결하고, cpppj 구조(8비트 화면·Gump UI·월드 세션)를 그대로 옮긴 새 `Netstorm.Client`로 교체하며, cpppj 실행 파일을 같은 입력의 비교 기준으로 씁니다. 2026-11-15에 cpppj 소스 전체와 대조해 남은 작업을 R4~R11 단계의 체크 목록으로 다시 정리했습니다. 원본과 완전히 같은 동작을 먼저 구현한 뒤 cpppj의 4~15차 목표를 후순위 요구사항으로 반영합니다. Windows 실행 파일 이름은 `Netstorm.exe`입니다. GUI Linux 지원(저장 위치는 `$HOME/.netstorm/`)과 멀티플레이는 후순위입니다. 최종 요구사항에는 전체화면·16:9/16:10/4:3 화면비·전체화면 가장자리 스크롤·영어/한국어 지원이 포함됩니다. 애니메이션은 원본 수준을 먼저 복원한 뒤 60/120프레임 지원을 진행합니다.

`webpj`는 HTML + JavaScript 기반입니다. 원본과 같은 동작을 목표로 `cpppj`의 싱글플레이 범위를 파일 단위로 옮기며, 고정한 cpppj 기준 판을 옮긴 뒤 이후 변경은 따로 판정해 재반영합니다. 커스텀 맵(Edit)·User Made Campaigns·멀티플레이, 웹 환경에 맞지 않는 기능(게임 종료, 해상도·전체화면 설정 등)은 제외하며, 필요한 에셋은 `webpj/www/` 안으로 복사해 사용합니다. cpppj의 4~15차 목표 가운데 웹에 해당하는 것(4차는 WebMCP, 5차는 localStorage, 7차는 브라우저 console 출력, 15차는 게임 종료 대신 다시 로딩, 12차 스커미시 포함)이 후순위 요구사항이며 8차·9차·14차는 제외합니다.

| 경로 | 용도 |
| --- | --- |
| `cpppj/` | C++20·Win32 기반 복원판과 검사 (주력) |
| `dotnetpj/` | C#·MonoGame DesktopGL 클론과 검사 |
| `webpj/` | HTML + JavaScript 웹 클론(`www/`), 정적 웹 서버, 검사·자산 준비 도구 |
| `assets/game-data/` | 클론용 그래픽·지도·미션·음악·효과음 데이터 |
| `fonts/` | 한국어 표시용 갈무리11(Galmuri11) 글꼴(dotnetpj 기존 경로는 아직 D2Coding), 일본어·기타 언어용 Noto Sans 계열 글꼴 |
| `docs/` | 분석·복원 근거, 빌드·플레이·검증 문서 |
| `tools/`, `analyzeManager/` | 정적 분석·원본 관찰·검증 도구 |
| `originals/`, `originalCD/` | 분석용 원본 10.78 및 CD판 10.72 자료 |
| `original1037/`, `originalPatches/` | 비교용 원본 10.37 자료와 10.37 → 10.62 패치(NSP1062) |
| `extracted/` | 정밀 디컴파일 결과와 빌드·검사 원시 로그. Git에 포함하지 않아 작업한 PC에만 있습니다. |
| `screenShots/`, `playingVideos/`, `originalFanGame/` | 스크린샷·녹화·팬게임 참고 자료. 대용량 영상과 팬게임 코드는 Git에 포함하지 않아 PC마다 없을 수 있습니다. |

한국어 건물·유닛 이름은 [공통 명칭 규칙](docs/korean-unit-names.md)을 따릅니다. 사제·신전·다리·발사기·간헐천·제단·거주 구역(Residence)·고갈된(Empty)은 번역할 수 있으며, 나머지는 워크샾·제네레이터처럼 영어 발음을 한글로 표기합니다. cpppj의 번역표(`xlat.korean` 502항목)와 dotnetpj의 메뉴·유닛 제목(공유 이름표 55개)이 같은 명칭을 씁니다. cpppj와 webpj는 메뉴·도움말과 미션 대본까지 한국어로 옮겼고(전체 문장 검수는 사용자 확인 대상), dotnetpj 새 경로의 한국어 화면은 R8 6차에서 연결합니다.

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

Node.js v22 이상이 필요하며(v22·v26에서 전체 검사 통과) 외부 npm 패키지는 쓰지 않습니다. 필요한 에셋은 `webpj/www/data/`에 커밋돼 있습니다. 프로젝트 루트에서 다음 명령으로 정적 웹 서버를 시작한 뒤 브라우저에서 `http://localhost:9803/`을 엽니다.

```sh
npm --prefix webpj start
```

자동 검사는 `npm --prefix webpj test`로 실행합니다. `assets/game-data/`의 원본 자료가 바뀌면 `npm --prefix webpj run prepare-assets`로 `webpj/www/data/`를 다시 만들고, `npm --prefix webpj run check-assets`로 복사본을 점검합니다. 개발용 주소 인자로 `?lang=ko`(한국어), `?mute`(소리 끄기), `?aspect=16:9`(화면비)를 조합할 수 있고, `?map=savetheisland`는 게임 규칙 없이 지형만 보여 주는 개발용 화면입니다.

# 주요 문서

- [cpppj·공통 인수인계](LEFT_JOBS.md) / [dotnetpj 인수인계](LEFT_JOBS.dotnetpj.md) / [webpj 인수인계](LEFT_JOBS.webpj.md): 최신 상태와 앞으로 할 작업. 지난 단위의 상세 이력은 [LEFT_JOBS.old.md](LEFT_JOBS.old.md)(cpppj·공통), [LEFT_JOBS.dotnetpj.old.md](LEFT_JOBS.dotnetpj.old.md)(dotnetpj), [LEFT_JOBS.webpj.old.md](LEFT_JOBS.webpj.old.md)(webpj)에 있습니다.
- [cpppj M8 최종 배포 관문 보고](docs/exe/cpp-m8-final-gate-1310-20261114.md): 제품 1310 기준 빌드·ZIP·배포본 실행·외부 접속 감사 결과
- `tools/port_inventory.py`: cpppj 소스 대비 dotnetpj·webpj의 남은 이식량을 영역별로 집계하는 도구(`python -X utf8 tools/port_inventory.py`). 두 인수인계의 단계별 남은 양 표를 다시 만들 때 씁니다.
- [TODO.md](TODO.md): cpppj·dotnetpj의 목표 프롬프트(각 인수인계의 작업을 끝까지 진행하는 지시와 건너뛸 검사)
- [C++ 복원 로드맵](docs/cpp-roadmap.md) / [실제 플레이 복원 계획](docs/cpp-playable-plan.md): 개발 단계와 복원 계획
- [cpppj → dotnetpj 이식 계획](docs/dotnet-cpp-port-plan-20261016.md): 이식 단위(W 번호)와 근거 목록. 현행 순서(R0~R11)와 단계별 체크 목록은 [dotnetpj 인수인계](LEFT_JOBS.dotnetpj.md) 1절이 우선합니다.
- [클론 게임 데이터](assets/README.md): 자산 구성·출처·빌드 포함 방식
- [원본 실행·별도 창 확인 규칙](docs/execution-confirmation.md): AI 작업의 실행 조건

# License

MIT License
Copyright (c) 2026 HJOW
