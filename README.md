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

**2026-11-29 기준:** 주력 `cpppj`는 제품1386/표시11.01이며 최신 배포 ZIP은1378(제품1377)입니다.1381 뒤 일시 중지 상태에서 사용자 요청만 처리해 정지한 건설 커서 재판정(1382), Tutorial6 크로스보우 가격 변수·미션 중 난이도 변경 차단(1383), AGENTS 변경에 따른 화면비별1280 폭 상한과 FSR 출력(1384), 생산 격자 부모 소실 뒤 Form 번호 재삭제 방지(1386)를 반영했습니다. `dotnetpj`는1.229 뒤 사용자 요청으로 일시 중지했고 `webpj`는E1488까지 진행했습니다.

| 프로젝트 | 실행·플레이 | 원본과의 동일성·남은 범위 |
| --- | --- | --- |
| **cpppj — C++ 복원판** | **플레이 가능.** 일반31미션·데모3개, Edit/시나리오 편집·스커미시·TCP/IP 로컬 멀티플레이·Outpost, 영어/한국어/일본어, 와이드(화면비별 최대1280 폭)·AA/FSR·난이도·Reset All·로컬 MCP를 구현했습니다. 초기6미션은 다음 단계의 실제 조건을 우측 하단에 배경 없는 글자로 표시합니다. | 최신 전체34경로 관문은1327(제품1326), 일본어 관문은1362, 최신 ZIP은1378(제품1377)입니다. 제품1382~1386은 집중 검사·Release 빌드로 검증했고 새 ZIP은 아직 만들지 않았습니다. 실제 TCP의 생산/건설·호스트 인수·기존 ID 복구는 확인했고 실물 LAN·자연 완주·전체 화면/음향 동일성과 물리 환경은 사용자 확인입니다. CD10.72·선택 추가 작업은 보류합니다. |
| **dotnetpj — C# + MonoGame** | 실행 경로는 `Netstorm.Game`/`Netstorm.exe` 하나이며 메뉴·세션 적재·월드 수집/타일·일반 객체 그림·HUD/미니맵/생산 덱 일부를 연결했습니다. 마지막 완료는1.229(옵션 부모/창·1280 정책 일부)이며 2026-11-29 사용자 요청으로 일시 중지했습니다. State 전환·나머지 표시·입력·규칙 연결이 남아 **아직 플레이할 수 없습니다.** | 메뉴12장면은 cpppj와 일치했고 시작 상태 일치는0/34입니다(1.175, 34경로 진입/대기 모두 정상 종료). 마지막 정기 전체 회귀는1.228의7,764통과·실패0입니다. 미니맵 차이는1픽셀까지 줄였습니다. 튜토리얼 안내 DN-TUT-1~4·해상도 DN-RES-1~4·생산 격자 정렬 DN-GRID-1~2가 남았습니다. |
| **webpj — HTML + JavaScript** | **브라우저에서 플레이 가능.**31미션·데모·스커미시(Outpost 포함), 영어/한국어/일본어, 게임 메뉴/채팅·생산/건설·전투/AI·진행 저장·설정/진행 JSON 복사·WebMCP·모드 관리를 제공하고 공식 튜토리얼6개에 다음 단계 조건 안내를 표시합니다. 마지막 완료 단위는WB04-E1488(화면비별 논리 최대1280 폭)입니다. | 기준 판의 이식1,007단위는 완료됐습니다. Tutorial1 시작 상태3화면비·Tutorial1~3 완주 L3, 나머지 Tutorial의 허용 구간·캠페인16개 진입/대기를 대조했고 실제 아군 AI로1-1·1-2·1-5·스커미시 승리를 확인했습니다. 최신 전체는 Node26 E1486 7,033건·Node22 E1485 7,032건 모두 통과입니다. CP1384·1386 변경 판정, 1-3·1-4·1-6 승리 등 사용자 확인(M4)과 물리60/120Hz·청취(M5)가 남았습니다. |

원본에 없던 **튜토리얼 진행 조건 안내는 세 프로젝트 공통 요구**입니다. cpppj(1371~1372)와 webpj(E1386~E1389)는 구현·허용 범위 검증을 마쳤고, dotnetpj는 조건/문구·표시·진행 수명·검증/배포의 네 작업(DN-TUT-1~4)이 미완료입니다. [공통 적용 계약](docs/tutorial-objectives-porting-20261127.md)·[cpppj 인계](LEFT_JOBS.md)·[dotnetpj 인계](LEFT_JOBS.dotnetpj.md)·[webpj 인계](LEFT_JOBS.webpj.md).

2026-11-28 이후 공통 후속도 같은 방식으로 나눠 기록합니다. Tutorial6 크로스보우 가격 변수·미션 중 난이도 변경 차단·정지한 건설 커서 재판정은 세 프로젝트 모두 반영했고(CP1382~1383·DN1.215·WB E1486~E1487), dotnetpj·webpj는 F3 채팅 닫기·작성 문장 보존도 맞췄습니다. AGENTS 4·8차 해상도 변경(화면비별 최대1280 폭, FSR은1920·2560·3840 폭 출력)은 cpppj1384·webpj E1488(웹은 FSR 제외)에 반영했고, dotnetpj는 정책 일부(1.229)만 들어가 DN-RES-1~4가 남았습니다. CP1386 생산 격자 보완은 DN-GRID-1~2·WB-GRID-1~2로 대기 중입니다. [해상도 정책](docs/resolution-policy-1280-20261128.md)·[사용자 후속](docs/game-user-followups-20261128.md)·[CP1386](docs/exe/cpp-production-grid-parent-loss-1386-20261129.md).

## C++ 복원판(cpppj)의 최신 상태

- **미션·조작:** Early Missions6·Priest Training9·캠페인16·데모3, Edit·시나리오 편집과 스커미시를 지원합니다. 원본 방식의 건설·생산·채취/운반·전투/의식·AI, HUD·메뉴·채팅·우클릭·가장자리 스크롤에 방향키 카메라 이동을 더했습니다.
- **튜토리얼 안내1371~1372:** 공식6미션/40단계/39키를 영어·한국어·일본어로 제공하고 실제 gate를 읽기만 합니다. 배경 없는 우측 하단 글자는 창/메뉴에서 가려졌다 복원되고 재시작·전환·이탈 때 정리됩니다. 집중22/22·실제 GUI19경로 종료0입니다. [조건표](docs/exe/cpp-tutorial-objectives-1371-20261127.md)·[구현/화면](docs/exe/cpp-tutorial-objective-display-1372-20261127.md).
- **언어:** 한국어는 외부 갈무리11, 일본어는 외부 Noto Sans JP를 사용합니다. 일본어 UI·공식43본문·도움말128주제 번역과 최종 관문1362를 완료했고, 튜토리얼 안내 뒤 한/일 번역표는각575키입니다. 사람 문장·물리 IME/클립보드 검수는 남습니다.
- **LAN1364~1380:** 실제 두 프로세스의 로비·전투 시작·시계·서버 인수·신전/골렘·워크샾/다리·기존 ID 재시작 복구를 검증했습니다. 최신1380은 공유 객체7,305/로컬 파생 표면51·SP 일치·세 프로세스 종료0입니다. **사용자 결정대로 전투 중 새 ID 참가 기능은 추가하지 않습니다.** 공식 서버·폐기 클라이언트 검증은 비활성 상태입니다. [봇 사용법](docs/testing/cpp-lan-opponent-guide.md)·[최신 검증](docs/exe/cpp-original-lan-scope-1380-20261127.md).
- **사용자 데이터·설정:** 설정·진행·커스텀 맵·로그는 `%USERPROFILE%\Saved Games\Netstorm\` 아래에 저장합니다. Edit 목록 스크롤·새 맵의 `.english` 생성·시나리오 편집, 확인 후 설정/진행만 초기화하는 Reset All을 제공합니다.
- **화면·장치:**1384부터 기본 최대 해상도는 화면비별1280×720·800·960·1024이며, FSR1.0 사용 가능 시1920·2560·3840 폭 네 화면비 출력을 같은 화면비의1280 폭 입력에서 확대합니다(옛1366/1600·미지원FSR 저장값은 자동 복구). 선택 AA·Frame Rate60/75/120, MCP 조작/PNG/소리 포함 녹화를 제공합니다. 난이도·스커미시·QuickJS-ng 엔진 탑재도 완료했습니다. QuickJS 추가 API와 비행체 보간은 보류입니다.
- **제품·로그:**11.01 Patch Credits/HJOW·AI 사용·Copyright2026 HJOW를 표시합니다. 사용자 로그의 표시 상세와 Regular SID/TYPE/frame/event 문맥을1377에서 보완했고18/18검사를 통과했습니다. [로그 근거](docs/exe/cpp-error-context-1377-20261127.md).
- **사용자 요청1382~1386(일시 중지 중 처리):** 정지한 건설 커서의 에너지·공간 재판정(1382), Tutorial6의 `global.crossbowCost`에 실제 windArcher 비용(현 자료550) 공개와 미션 중 Difficulty 변경 차단(1383, 메인 Options는 유지), 1280 해상도 정책(1384), 생산 격자 부모가 삭제된 뒤 패널을 닫을 때 이미 반납된 Form 번호를 다시 지우던 결함 보완(1386, 원본과의 의도적 차이)을 반영했습니다. [1382](docs/exe/cpp-stationary-placement-1382-20261128.md)·[1383](docs/game-user-followups-20261128.md)·[1384](docs/resolution-policy-1280-20261128.md)·[1386](docs/exe/cpp-production-grid-parent-loss-1386-20261129.md).
- **최신 배포1378:** 제품1377·1,480항목, CRC/원천 해시·글꼴/라이선스·중복0·압축 해제본 한국어 전체화면/일본어 창 GUI2회 종료0을 확인했습니다.1380에서 같은 EXE/ZIP 해시와 보호2,810파일 불변을 재확인했습니다. 제품1382~1386을 담은 새 ZIP은 재개 지시가 있을 때 만듭니다. [배포 검증](docs/exe/cpp-final-package-1378-20261127.md).

cpppj 목표는1381 뒤 일시 중지 상태이며 재개 지시 전에는 사용자 개별 요청만 처리합니다. 남은 것은 명시 사용자 확인·미결 재현 자료와 보류/선택 사항입니다. 알려진 버그34건의 판정과 사용자 확인 범위, CD10.72 메뉴 이후 보류, D8 한국어 선명도·QuickJS 추가 연동·보간은 [cpppj 인계](LEFT_JOBS.md)를 따릅니다. 기존 관문·집중 검사·실제 실행·사람 확인을 구분하며 전체 원본 동일성을 완료로 선언하지 않습니다.

## C# + MonoGame 클론(dotnetpj)의 최신 상태

직역 호스트 `Netstorm.Game`이8비트 논리 버퍼·Gump UI·월드 세션을 제공하는 `Netstorm.Client`를 사용하고 MonoGame이 창·입력·오디오·출력을 맡습니다. R4 세션 조립은2/8, R5 월드 화면은2/5입니다.1.080~1.229에서 미니맵(차이6,000→1픽셀)·GumpForm/생산 HUD·일반 객체/효과 그림(kind 0·3)·객체 정보 메뉴·State UI/Uber·음향/음악/옵션·Scene 후속 A–G를 연결했고, 사용자 후속(가격/난이도/F3/정지 커서,1.215)과1280 해상도 정책 일부(1.229)를 반영했습니다. 2026-11-29 사용자 요청으로1.229 뒤 일시 중지했습니다.

재개 순서는 H(State7/정리/결과의 실제 호출자, H-1~3)→R5-4c3c 미니맵 잔여1픽셀·전환 수명→R5-4c4 인벤토리·공통 HUD→R5-4d UberRoot 패널→R5-3 타입별 그림→R5-5 가장자리 스크롤/34경로 L4a→R6 규칙/입력입니다. 튜토리얼 안내 DN-TUT-1~4, 해상도 DN-RES-1~4, CP1386 정렬 DN-GRID-1~2, CP LAN 교정 DN-MP-02~08, 최초 실행·갈무리11 화면 적용·창 아이콘 확인이 남았습니다. 미션 시작 상태0/34와 플레이 불가 상태는 그대로이며 CP 검증을 DN 완료로 세지 않습니다. [DN 인계](LEFT_JOBS.dotnetpj.md).

## HTML + JavaScript 웹 클론(webpj)의 최신 상태

‘클릭하여 시작’ 뒤31미션·데모·스커미시를 진행할 수 있고 메뉴·채팅·영어/한국어/일본어 대본·갈무리11·Reset All·localStorage 진행 저장과 설정/진행 JSON 복사·불러오기가 동작합니다. 논리 화면은 화면비별 최대1280×720·800·960·1024이고 더 큰 창은 CSS/DPR 표시만 확대합니다(E1488). WebMCP는 지원 Chromium에서 실제 입력·PNG·소리 포함 WebM/다운로드를 확인했고 미지원 브라우저의 일반 실행도 확인했습니다. 웹에서는 Edit·User Made Campaigns·멀티플레이·CD 인트로·해상도/전체화면 설정·FSR·게임 종료를 제공하지 않습니다.

E1381~E1488에서 앱 HUD 생성·State7 수명과 전투7화면0픽셀(E1385), 튜토리얼 조건 안내(E1386~E1389), 1-1·1-2·1-5·스커미시 실제 아군 AI 승리, 메모리·SID·그래프 회복, 표시 성능·고DPI, 일본어(13차, E1410~E1412), Outpost(12차), 14차 공통 모드 API v0.1.0 기준 구현과 Options 모드 관리(E1470~E1484), 2026-11-28 사용자 후속(E1486~E1488)을 마쳤습니다. 다음 자동 작업은 CP1384 `RawGameMenus.cpp` 1행 판정과 CP1386 생산 격자 반영(WB-GRID-1~2), 정기 전체 회귀입니다. 1-3·1-4·1-6 승리와 이후 플레이·한국어 문장 검수(M4), 물리60/120Hz·모니터 이동·청취(M5), dotnetpj와의 모드 적합성 대조, 자료가 필요한 알려진 버그14건은 사용자·외부 조건을 기다립니다. [WB 인계](LEFT_JOBS.webpj.md)·[WebMCP](docs/webpj-webmcp-20261126.md)·[공통 모드 API](docs/netstorm-common-mod-api-v0-20261128.md).

# 프로젝트 구성과 개발 목표

`cpppj`는 **① 10.78 싱글플레이 복원 → ② Windows 10/11 안정 구동(DirectDraw 오류 수정 포함) → ③ TCP/IP 로컬 네트워크 멀티플레이·Outpost → ④~⑮ 추가 요구사항** 순서로 진행합니다. 추가 요구사항은 다음과 같습니다.

- **4차:** 로컬 MCP(dotnetpj 분석을 위한 모든 게임 조작·화면 캡처·소리 포함 녹화)와 와이드(16:9) 해상도 지원. 최대 1280×720이며 세로는 화면비에 따라 800·960·1024까지(그래픽 크기 스케일 조정은 불필요)
- **5차:** 설정·캠페인 진행 저장을 사용자 홈의 `Saved Games\Netstorm\Saves\`로 옮기고, 게임이 자동으로 만드는 모든 파일·폴더를 `Saved Games\Netstorm\` 아래로 옮겨 관리자 권한 없이 쓸 수 없는 위치에 설치해도 동작하게 합니다. 커스텀 맵은 기존 폴더와 `Saved Games\Netstorm\CustomMaps\` 양쪽에서 불러오고(Edit에서 새로 만든 맵은 후자에 저장), Edit 맵 목록에 스크롤을 넣고 표시 개수 제한을 없앱니다.
- **6차:** 한국어 지원
- **7차:** 오류/예외 처리와 로깅 공통 구조. 비정상 종료 직전의 상세 로그를 `Saved Games\Netstorm\logs\yyyyMMddHHmmss.log`에 남깁니다.
- **8차:** 메인 메뉴 Options의 안티앨리어싱과 FSR 1.0 선택(기본 꺼짐). FSR 지원 환경에서는 1920×1080·2560×1440·3840×2160을 고를 수 있고, 이때 FSR을 자동으로 켜서 실질 1280×720 화면을 목표 해상도로 업스케일합니다.
- **9차:** Edit에서 맵·구성요소 배치뿐 아니라 시나리오 자체(플레이어 초기 자본·테크, 적 구성·동맹 등)를 화면에서 편집하는 전용 UI, 새 맵을 만들 때 기본 `.english` 파일 자동 생성(`.english` 없이 Test Battle에 들어가면 종료되는 문제 수정)
- **10차:** 원본 10.78에 알려져 있던 버그의 확인과 수정
- **11차:** 난이도(쉬움·보통·어려움). 쉬움은 적 AI의 다리 건설·유닛 생성 주기가 4배로 늘고 자원채취 유닛 수가 절반이 되며, 어려움은 주기가 2배 빨라집니다.
- **12차:** 스커미시 모드. 멀티플레이처럼 무작위로 배치된 섬에서 원하는 수의 AI와 난이도·동맹·금지 유닛을 정해 플레이합니다.
- **13차:** 기타 언어 지원(일본어)
- **14차:** QuickJS 엔진 탑재(연동 범위는 추후 결정)
- **15차:** 설정 초기화. Options 맨 아래 Reset All을 다시 한번 확인받은 뒤 `options.cfg`를 비롯한 설정과 게임 진행을 모두 초기화하고 게임을 종료합니다.

`cpppj`는10.78 대상1~15차 구현을 마감했습니다(14차는 엔진 탑재까지).13차 일본어 UI·공식43본문·도움말128주제는1362까지 완료했고 이후 튜토리얼 안내·실제 TCP 검증·로그 보완을 반영했으며, 2026-11-28 AGENTS 해상도 변경에 맞춰4·8차를1384에서 다시 반영했습니다. 자연 완주·전체 동일성·실물 LAN/다른 환경은 사용자 확인입니다. Windows 전용(x86-64 대상, Windows11 ARM 고려)이며 공식 서버 접속은 복원하지 않습니다. CD10.72는 메뉴 이후 보류를 유지합니다.

`dotnetpj`는 C# + MonoGame 기반입니다. 처음에는 이미 구현된 클론에 `cpppj`가 검증한 부분을 조각별로 옮기는 방식(2026-10-16 이식 계획)이었으나, 기존 구현이 원본과 다른 곳이 많아 2026-10-28에 계획을 다시 세웠습니다. 기존 근사 구현은 2026-11-22에 제거했고, 유일한 실행 경로 `Netstorm.Game`이 cpppj 구조(8비트 화면·Gump UI·월드 세션)를 그대로 옮긴 `Netstorm.Client`를 사용하며, cpppj 실행 파일을 같은 입력의 비교 기준으로 씁니다. 2026-11-15에 cpppj 소스 전체와 대조해 남은 작업을 R4~R11 단계의 체크 목록으로 다시 정리했습니다. 원본과 완전히 같은 동작을 먼저 구현한 뒤 cpppj의 4~15차 목표를 후순위 요구사항으로 반영합니다. 14차는 사용자 결정(2026-11-23)에 따라 QuickJS 위에 webpj와 같은 공통 모드 API를 제공하며, 현재는 계약 확정(MOD-1)까지입니다. Windows 실행 파일 이름은 `Netstorm.exe`입니다. GUI Linux 지원(저장 위치는 `$HOME/.netstorm/`)과 멀티플레이는 후순위입니다. 최종 요구사항에는 전체화면·16:9/16:10/4:3 화면비·전체화면 가장자리 스크롤·영어/한국어 지원이 포함됩니다. 애니메이션은 원본 수준을 먼저 복원한 뒤 60/120프레임 지원을 진행합니다.

`webpj`는 HTML + JavaScript 기반입니다. 원본과 같은 동작을 목표로 `cpppj`의 싱글플레이 범위를 파일 단위로 옮기며, 고정한 cpppj 기준 판을 옮긴 뒤 이후 변경은 따로 판정해 재반영합니다. 커스텀 맵(Edit)·User Made Campaigns·멀티플레이, 웹 환경에 맞지 않는 기능(게임 종료, 해상도·전체화면 설정 등)은 제외하며, 필요한 에셋은 `webpj/www/` 안으로 복사해 사용합니다. cpppj의 4~15차 목표 가운데 웹에 해당하는 것(4차는 WebMCP, 5차는 localStorage, 7차는 브라우저 console 출력, 15차는 게임 종료 대신 다시 로딩, 12차 스커미시 포함)이 후순위 요구사항이며 8차·9차는 제외합니다. 14차는 브라우저가 JS를 지원하므로 QuickJS를 탑재하지 않고, 사용자 결정(2026-11-23)에 따라 dotnetpj와 공유할 공통 모드 API의 기준 구현을 맡습니다.

| 경로 | 용도 |
| --- | --- |
| `cpppj/` | C++20·Win32 기반 복원판과 검사 (주력) |
| `dotnetpj/` | C#·MonoGame DesktopGL 클론과 검사 |
| `webpj/` | HTML + JavaScript 웹 클론(`www/`), 정적 웹 서버, 검사·자산 준비 도구 |
| `assets/game-data/` | 클론용 그래픽·지도·미션·음악·효과음 데이터 |
| `fonts/` | 한국어 표시용 갈무리11(Galmuri11), 일본어·기타 언어용 Noto Sans 계열 글꼴. dotnetpj도 갈무리11을 외부 배포하며 화면 결선은 후속 작업이다. |
| `docs/` | 분석·복원 근거, 빌드·플레이·검증 문서 |
| `tools/`, `analyzeManager/` | 정적 분석·원본 관찰·검증 도구 |
| `originals/`, `originalCD/` | 분석용 원본 10.78 및 CD판 10.72 자료 |
| `original1037/`, `originalPatches/` | 비교용 원본 10.37 자료와 10.37 → 10.62 패치(NSP1062) |
| `extracted/` | 정밀 디컴파일 결과와 빌드·검사 원시 로그. Git에 포함하지 않아 작업한 PC에만 있습니다. |
| `screenShots/`, `playingVideos/`, `originalFanGame/` | 스크린샷·녹화·팬게임 참고 자료. 대용량 영상과 팬게임 코드는 Git에 포함하지 않아 PC마다 없을 수 있습니다. |

한국어 건물·유닛 이름은 [공통 명칭 규칙](docs/korean-unit-names.md)을 따릅니다. 사제·신전·다리·발사기·간헐천·제단·거주 구역(Residence)·고갈된(Empty)은 번역할 수 있으며, 나머지는 워크샾·제네레이터처럼 영어 발음을 한글로 표기합니다. cpppj의 번역표(`xlat.korean` 575항목)와 dotnetpj의 메뉴·유닛 제목(공유 이름표 55개)이 같은 명칭을 씁니다. cpppj와 webpj는 메뉴·도움말과 미션 대본까지 한국어로 옮겼고(전체 문장 검수는 사용자 확인 대상), dotnetpj 새 경로의 한국어 화면은 R8 6차에서 연결합니다.

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

배포용 압축파일은 아래 스크립트로 만듭니다. 실행 파일·Visual C++ 런타임·게임 데이터·한국어/일본어 번역·외부 글꼴/라이선스·QuickJS/FSR 라이선스·실행 배치 파일을 포함합니다. 저장 위치를 생략하면 입력받고 빈 값이면 취소합니다. 구성과 옵션은 [C++ 빌드7절](docs/cpp-build.md)을 따릅니다.

받는 PC에서는 압축을 풀고 `Netstorm.exe` 또는 `Netstorm-SavedMode.bat`(저장된 화면 모드), `Netstorm.bat`(창 모드 강제)을 실행합니다. 설정·진행·캡처 등은 `%USERPROFILE%\Saved Games\Netstorm\`에 저장되므로 설치 폴더가 읽기 전용이어도 됩니다. 설정/진행 초기화는 Options의 Reset All을 사용합니다.

최신 ZIP1378은 제품1377/1,480항목입니다. CRC·원천 해시·언어/글꼴/라이선스·압축 해제본 실제 GUI2회와 실행 뒤 배포 파일 불변을 확인했습니다.1380에서 현재 EXE/ZIP 해시를 다시 대조했습니다. 그 뒤 제품1382~1386은 아직 ZIP에 포함하지 않았습니다. 전체34경로 관문1327은 당시 제품1326의 기록으로 유지하며 Windows11/다른 물리 환경은 사용자 확인입니다. [최신 배포 근거](docs/exe/cpp-final-package-1378-20261127.md).

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File cpppj\package-cpppj.ps1 D:\dist\NetstormReborn-cpppj.zip
```

# C# + MonoGame 클론 빌드와 실행 (dotnetpj, 개발 중)

아래 명령은 프로젝트 루트(`NetstormReborn`)에서 실행합니다. 게임 본체는 C# / MonoGame DesktopGL이며 대상 프레임워크는 `net10.0`입니다. 실행 프로젝트는 `Netstorm.Game`(출력 `Netstorm.exe`)이며 현재는 메뉴·개발용 미션 적재까지 동작하고 월드 플레이는 아직입니다.

## 준비

- **.NET 10 SDK**를 설치하고 `dotnet --version`으로 확인합니다. `dotnetpj/global.json`은 10.0.100 이상인 .NET 10 SDK의 최신 기능 버전을 허용합니다.
- 첫 빌드·배포 시 NuGet 패키지와 대상 OS의 런타임을 내려받을 인터넷 연결이 필요합니다.
- 저장소의 `assets/game-data/`와 외부 갈무리11의 일반/굵은 TTF와 `fonts/Galmuri11-LICENSE.txt`를 함께 체크아웃합니다. 데이터·글꼴은 빌드·배포 출력에 자동으로 복사됩니다.
- 현재 실행/검증 대상은 **Windows 10/11 x64**이며 Linux GUI 지원은 후순위입니다. OpenGL을 지원하는 그래픽 드라이버가 필요합니다.

dotnetpj의 빌드·테스트·실행은 독립된 `assets/game-data/`를 사용하며 원본 실행 파일이 필요하지 않습니다. `originals/`는 분석용 보호 자료로 별도 보존합니다. 자산의 구성과 이관 기록은 [클론 게임 데이터](assets/README.md)를 참고합니다.

## 개발용 빌드와 실행

```sh
dotnet restore dotnetpj/Netstorm.sln
dotnet build dotnetpj/Netstorm.sln -c Release --no-restore
dotnet test dotnetpj/Netstorm.sln -c Release --no-build
dotnet run --project dotnetpj/src/Netstorm.Game -c Release --no-build -- --window --set "currentLanguage=english"
```

현재 영어 경로는 `--set "currentLanguage=english"`를 사용합니다. 1.228부터 설정 전에 OS 언어를 감지하지만 한국어 화면 결선은 후속 작업이며 갈무리11 파일은 외부에 배치합니다. 설정·진행은1.041부터 사용자 루트의 `Saves/options.cfg`(Windows `%USERPROFILE%\Saved Games\Netstorm\Saves\`, Linux `$HOME/.netstorm/Saves/`)에 쓰며 설치 경로는 읽기만 합니다. 빌드 결과는 `dotnetpj/src/Netstorm.Game/bin/Release/net10.0/`에 생성됩니다. 일반 빌드는 빌드한 OS용 실행 파일을 만들며, 다른 OS용 바이너리는 아래의 `-r` 옵션으로 만듭니다.

## 런타임을 포함한 배포용 바이너리

게임 프로젝트에 `dotnet publish`를 실행합니다. 다음 명령은 Windows PowerShell과 Linux 셸에서 모두 사용할 수 있으며, Linux에서도 Windows exe를 만들 수 있습니다.

```sh
dotnet publish dotnetpj/src/Netstorm.Game/Netstorm.Game.csproj -c Release -r win-x64 --self-contained true -o dist/win-x64
```

| 대상 | 출력 폴더 | 실행 파일 |
| --- | --- | --- |
| Windows 10/11 x64 | `dist/win-x64/` | `Netstorm.exe` |

`--self-contained true`는 .NET 런타임까지 포함하므로 실행할 PC에 .NET SDK나 런타임을 따로 설치할 필요가 없습니다. SDL2·OpenAL 네이티브 라이브러리, `fonts/`, `game-data/`도 출력에 포함됩니다. 실행 파일은 함께 생성된 DLL·런타임 파일·글꼴·게임 데이터를 사용하므로 **출력 폴더 전체를 복사해 배포**합니다. 그래픽 드라이버와 Linux GUI 환경 등 OS의 실행 환경은 별도로 필요합니다.

실행할 PC에 .NET 10 런타임이 이미 설치되어 있다면 더 작은 배포물을 만들 수 있습니다. 위 명령의 `--self-contained true`를 `--self-contained false`로 바꾸고, 출력 폴더도 `dist/win-x64-fdd` 또는 `dist/linux-x64-fdd`처럼 구분합니다.

배포용 압축파일(zip)은 다음 스크립트로 만듭니다. 저장 위치를 생략하면 실행 중에 입력받고, 빈 값을 입력하면 취소합니다. 기본은 받는 PC에 .NET 10 런타임을 미리 설치하는 방식이며, `-SelfContained`를 붙이면 런타임까지 포함합니다. 임시 폴더에 게시하므로 저장소에 `dist/`를 남기지 않습니다.

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File dotnetpj\package-dotnetpj.ps1 D:\dist\NetstormReborn-dotnetpj.zip
```

## 포함된 게임 데이터와 바이너리 실행

빌드·배포 시 `assets/game-data/`의 내용이 출력의 **`game-data/`에 자동으로 복사**됩니다. 현재 Windows 배포 폴더의 구조는 다음과 같습니다.

```text
dist/win-x64/
  Netstorm.exe
  Netstorm.dll
  ... (함께 생성된 DLL·런타임·설정 파일)
  fonts/
    Galmuri11Bitmap-Regular-2.40.4.ttf
    Galmuri11Bitmap-Bold-2.40.4.ttf
    Galmuri11-LICENSE.txt
  game-data/
    netstorm.tarc
    d/
    music/
    sound/
```

Windows PowerShell에서 실행:

```powershell
.\dist\win-x64\Netstorm.exe --window --set "currentLanguage=english"
```

Linux에서 실행(R10 Linux 단계 전이라 검증하지 않았습니다):

```sh
chmod +x dist/linux-x64/Netstorm
./dist/linux-x64/Netstorm --window --set "currentLanguage=english"
```

일반 실행은 함께 배포된 데이터를 자동으로 찾습니다. 개발·검증용 데이터를 다른 위치에서 사용하려면 `NETSTORM_DATA`에 **`netstorm.tarc`가 들어 있는 폴더의 절대 경로**를 지정합니다. 탐색 순서는 유효한 `NETSTORM_DATA` → 실행 파일·작업 폴더 및 그 상위의 `game-data/` 또는 `assets/game-data/`입니다.

```powershell
$env:NETSTORM_DATA = "D:\Games\NetstormReborn\game-data"
.\dist\win-x64\Netstorm.exe --window --set "currentLanguage=english"
```

```sh
NETSTORM_DATA="/절대/경로/game-data" ./dist/linux-x64/Netstorm --window --set "currentLanguage=english"
```

# 웹 클론 실행 (HTML + JavaScript)

Node.js 22.13 이상(22.x 계열) 또는 24 이상이 필요합니다. 빌드·검사 도구는 `npm --prefix webpj ci`로 설치합니다. 필요한 에셋은 `webpj/www/data/`에 커밋돼 있습니다. 프로젝트 루트에서 다음 명령으로 정적 웹 서버를 시작한 뒤 브라우저에서 `http://localhost:9803/`을 엽니다.

```sh
npm --prefix webpj start
```

ESLint 검사는 `npm --prefix webpj run lint`, 운영 번들은 `npm --prefix webpj run build`, 개발 번들은 `npm --prefix webpj run build:dev`로 만듭니다. webpack은 생성 전에 전체 ESLint 검사를 기다리며 오류나 경고가 있으면 기존 번들을 보존하고 실패합니다. 자동 갱신과 직접 webpack 호출에도 같은 검사가 적용됩니다. [설정·기존 코드 예외 관리](docs/webpj-eslint-20261127.md)를 참조합니다.

웹 언어는 Options ▸ Language에서 English/Korean/Japanese를 고르고 저장할 수 있습니다. 일본어 글꼴은 Google Fonts CDN을 사용하며 적재에 실패하면 영어로 계속합니다. [일본어 지원/검증](docs/webpj-japanese-browser-support-20261127.md)·[외부 접속 경계](docs/webpj-external-access-20261105.md#9-일본어-google-fonts-경계-2026-11-27-wb04-e1412). Options에는 모드 관리(공통 모드 API v0.1.0, 기본 꺼짐)와 설정/진행 JSON 복사·불러오기 행이 있습니다([모드 사용법](docs/webpj-mod-options-20261128.md)·[JSON](docs/game-user-followups-20261128.md)).

자동 검사는 `npm --prefix webpj test`로 실행합니다. `assets/game-data/`의 원본 자료가 바뀌면 `npm --prefix webpj run prepare-assets`로 `webpj/www/data/`를 다시 만들고, `npm --prefix webpj run check-assets`로 복사본을 점검합니다. 개발용 주소 인자로 `?lang=ko`(한국어)·`?lang=ja`(일본어), `?mute`(소리 끄기), `?aspect=16:9`(화면비)를 조합할 수 있고, `?map=savetheisland`는 게임 규칙 없이 지형만 보여 주는 개발용 화면입니다.

# 주요 문서

- [cpppj·공통 인수인계](LEFT_JOBS.md) / [dotnetpj 인수인계](LEFT_JOBS.dotnetpj.md) / [webpj 인수인계](LEFT_JOBS.webpj.md): 최신 상태와 앞으로 할 작업. 지난 단위의 상세 이력은 [LEFT_JOBS.old.md](LEFT_JOBS.old.md)(cpppj·공통), [LEFT_JOBS.dotnetpj.old.md](LEFT_JOBS.dotnetpj.old.md)(dotnetpj), [LEFT_JOBS.webpj.old.md](LEFT_JOBS.webpj.old.md)(webpj)에 있습니다.
- [cpppj M8 최종 배포 관문 보고](docs/exe/cpp-m8-final-gate-1327-20261122.md): 제품 1326 기준 빌드·ZIP·배포본 실행·외부 접속 감사 결과(이전 제품 1310 기준은 [1311 보고](docs/exe/cpp-m8-final-gate-1310-20261114.md))
- [해상도 정책](docs/resolution-policy-1280-20261128.md): AGENTS 4·8차 변경(화면비별 1280 폭 상한·FSR 출력)의 세 프로젝트 공통 기준과 cpppj 1384 근거
- [공통 모드 API v0](docs/netstorm-common-mod-api-v0-20261128.md): webpj가 기준 구현하고 dotnetpj가 QuickJS로 따를 14차 모딩 계약
- `tools/port_inventory.py`: cpppj 소스 대비 dotnetpj·webpj의 남은 이식량을 영역별로 집계하는 도구(`python -X utf8 tools/port_inventory.py`). 두 인수인계의 단계별 남은 양 표를 다시 만들 때 씁니다.
- [TODO.md](TODO.md): cpppj·dotnetpj의 목표 프롬프트(각 인수인계의 작업을 끝까지 진행하는 지시와 건너뛸 검사)
- [C++ 복원 로드맵](docs/cpp-roadmap.md) / [실제 플레이 복원 계획](docs/cpp-playable-plan.md): 개발 단계와 복원 계획
- [cpppj → dotnetpj 이식 계획](docs/dotnet-cpp-port-plan-20261016.md): 이식 단위(W 번호)와 근거 목록. 현행 순서(R0~R11)와 단계별 체크 목록은 [dotnetpj 인수인계](LEFT_JOBS.dotnetpj.md) 1절이 우선합니다.
- [클론 게임 데이터](assets/README.md): 자산 구성·출처·빌드 포함 방식
- [원본 실행·별도 창 확인 규칙](docs/execution-confirmation.md): AI 작업의 실행 조건

# License

MIT License
Copyright (c) 2026 HJOW
