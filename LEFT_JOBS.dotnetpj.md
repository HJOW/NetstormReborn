# LEFT_JOBS.dotnetpj — dotnetpj(C# + MonoGame) 현행 상태와 남은 작업

**기준 단위 0.562 · 갱신 2026-11-15(HJOW-Athlon) · 현재 단계 R4(세션·월드 조립) · 다음 단위 0.563 = [R4-1](#r4-detail) · cpppj 기준 제품 1313 · 전체 목표 미완료 · 사용자 재개 지시로 작업 진행 중 — 지금 어디까지 왔는지는 [0.1](#dotnet-overview), 단계별 남은 양은 [0.2](#dotnet-stage-board), 다음에 할 일은 [0.4](#now), 단계별 체크 목록은 [2.4](#r4-detail)**

이 문서는 dotnetpj의 **현재 상태·남은 작업·계속 유효한 결정과 규칙**만 둔다. cpppj와 공통 사항은 [LEFT_JOBS.md](LEFT_JOBS.md), webpj는 [LEFT_JOBS.webpj.md](LEFT_JOBS.webpj.md)가 기준이다. 이번 정리(2026-11-15) 직전의 이 문서 전체(단위 기록 0.453~0.545 93건, R4 진행판의 묶음별 경과, 0.3의 단위별 검사·fixture 나열)는 [LEFT_JOBS.dotnetpj.old.md](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261115-through-0545)에 바이트 그대로 보관했고, 그 이전 보관본 6개와 0.452 이하 단위 기록도 같은 파일에 있다. [이식 계획](docs/dotnet-cpp-port-plan-20261016.md)의 W 번호·fixture 목록은 근거 목록으로만 쓰고, 순서·구조·완료 기준은 이 문서를 따른다.

**읽는 순서:** 0.1(지금 어디까지 왔나) → 0.2(단계별 남은 양) → 0.4(다음에 할 일) → 2.4(단계별 체크 목록) → 0.5(확정된 결정) → 필요할 때 나머지 절.

<a id="dotnet-overview"></a>

## 0. 한눈에 보기

### 0.1 진행 상황 — 실행·플레이 가능 여부와 원본과의 동일성

**한 줄 요약:** 새 경로는 **메뉴 12장면이 cpppj와 픽셀까지 같고**, 개발 진입 `--mission`으로 일반 31미션의 세션·Player·동맹·Zone·지형·요새 객체 슬롯·그래프 초기화까지 조립한다. **객체 Pop 후처리·정기 처리·AI 시작·월드 표시·입력·규칙이 연결되지 않아 새 경로로는 아직 플레이할 수 없다.** 지금 플레이할 수 있는 것은 동결된 근사 기존 경로다.

AGENTS.md의 진행 요약 기준(실행·플레이 가능 여부, 원본과의 유사성)으로 평가한다. dotnetpj에는 실행 경로가 둘이다([2.1](#dotnet-current-plan-20261028) 결정 5).

| 경로 | 실행·플레이 가능 여부 | 원본과의 동일성 |
| --- | --- | --- |
| **기존 경로** `Netstorm.Game`<br>(`Netstorm.exe`, 현재 배포 대상) | **실행·플레이 가능.** 메인 메뉴와 캠페인 1-1·1-2를 근사 규칙으로 플레이할 수 있다. 한국어 선택·와이드 시야 확장·테두리 없는 전체화면이 있다. 새 기능을 넣지 않는 동결 상태다(실행 오류 수정만). | **낮다.** 녹화·팬게임 기반 근사(RGBA 합성, 자체 UI 배치, 근사 세션 `BattleSession`)라 원본 동일성의 기준이 될 수 없다. 새 경로가 같은 범위를 대체하면 삭제한다. |
| **새 경로** `Netstorm.Client` + `Netstorm.Client.Host`<br>(개발 호스트, 아직 배포 안 함) | **메뉴와 미션 진입 일부만 실행.** 타이틀, 메인 메뉴 8버튼, Help·Options(해상도·음량·난이도 펼침 포함)·Credits(11.01 포함), 캠페인 선택, Edit 맵 목록·새 맵 이름까지의 메뉴, 그리고 개발 진입 `--mission <키> --handle <이름>`(세션·Player·동맹·Zone·가입/Ready·지형·요새 객체 슬롯·그래프 초기화, Player 화신·사제 Pop). **월드 화면·입력·규칙이 없어 플레이할 수 없다.** | **부품은 높지만 전체는 미확정.** cpppj를 구조 그대로 직역했고 원본 x86 fixture 313개로 부품을 대조했다(PORT_MAP L1 433행). **화면(L4a): 1024×768 메뉴 12장면이 cpppj와 BMP 바이트 동일**(0.381~0.388). **상태(L2): cpp 기준선 34/34경로·dn 일반 31미션 덤프를 같은 시드로 확보해 대조했고 전체 일치는 0/34**다(데모 3경로는 dn 진입 미연결, [일괄 수집 결과](docs/dotnet-l2-batch-20261114.md)). 덤프에서 SID 집합·타입·난수 흐름은 맞고, 소유자 스탬프·그래프 번호·장부·정기 처리·AI·첫 지역(+0x78)이 다르다([2.4](#r4-detail) R4-2~R4-5의 대상). L3·L4b·L5는 미착수다. |

**이식 범위(0.545 시점, 2026-11-15 집계, `tools/port_inventory.py`):** cpppj `src`는 소스 단위(같은 이름의 `.cpp/.h/.inc` 묶음) 1,237개·169,017줄이다. dotnetpj `PORT_MAP.tsv` 886행이 가리키는 것은 그중 **406단위(75,329줄, 45%)**이고, **대응이 없는 것이 831단위(93,688줄, 55%)**다. 대응이 있다는 것은 행이 있다는 뜻일 뿐 그 파일을 끝까지 옮겼다는 뜻이 아니다(예: `RawCampaignSession`·`ClientMain`·`Renderer`는 일부만). 대응 없는 831단위의 단계별 배분은 0.2, 영역별 표는 [2.7](#dotnet-inventory)이다.

<a id="dotnet-stage-board"></a>

### 0.2 단계 진행판 (R0~R11)

**남은 양** 칸은 그 단계가 맡는 영역에서 PORT_MAP 대응이 아직 없는 cpppj 단위 수 / 줄 수다([2.7](#dotnet-inventory) 영역 기호). 이미 대응이 있는 파일의 미완 부분과 호스트 결선·덤프 대조는 이 숫자에 드러나지 않는다. 단계별 체크 목록은 [2.4](#r4-detail)다.

| 단계 | 내용 | 이정표·완료 기준 | 상태 | 남은 양(대응 없는 cpppj) |
| --- | --- | --- | --- | --- |
| R0 | 기준선·동기화(cpppj 오라클, PORT_MAP, `cpp_sync.py`) | — | ✅ 완료 | — |
| R1 | 8비트 화면 호스트·입력 큐·오라클 도구(`--ui-script`·`snapshot`·`session-dump`·`--fixed-frame-ms`) | — | ✅ 완료 | — |
| R2 | 그리기 원시·팔레트·글꼴 | — | ✅ 본체 완료(실자산·세션 연결은 R5) | — |
| R3 | Gump UI와 메인 메뉴 계열 | D1 메뉴 픽셀 일치 | 🔶 **메뉴 L4a 12장면 차이 0.** 미룬 3장면(Language 자식 CJK는 R8, Multiplayer 섬 화면은 R5·R11, Edit 이동행은 R7)과 잔여 위젯·소유자([2.5](#r3-r2-detail))는 R5~R7에서 해당 화면과 함께 닫는다 | R5의 G 영역에 포함 |
| **R4** | **월드 세션 조립** | **M1: 34경로 Load/Start 상태가 cpppj `session-dump`와 같다(L2)** | 🔶 **진행 중(0.390~0.562).** 세션·Player·지형·요새 객체 적재와 객체 수명 공통층(공장·Pop/Unpop·삭제·피해·연소·사제·보호막 부품)까지. **M1 0/34.** 남은 조각 R4-1~R4-8 | A·B·C·D **137단위 / 13,873줄** + K·L·M 중 적재 경로 |
| R5 | 월드 표시·카메라·HUD, **`Netstorm.exe` 전환** | D2: 34경로 진입 직후·N프레임 뒤 캡처 차이 0(L4a) | ⬜ 미착수(연소·공통 표시 등록 부품만 R4에서 선행) | E·G **102단위 / 13,764줄** |
| R6 | 플레이 입력·규칙·AI·소리 | M2: 고정 입력열 L3 상태 해시 일치 · M3: 1-1·1-2를 새 경로에서 진행 → 기존 경로 삭제 | ⬜ 미착수(사제·Carrier·보호막·소리 장치 일부 부품만 R4에서 선행) | F·H·I·J·K·L·N·O **382단위 / 45,090줄** |
| R7 | 미션 전체·Edit·설정·배포 | M4: 34경로 진입·진행·저장 수명, Edit, 배포 압축 해제 실행 | ⬜ 미착수(Edit 목록·`options.cfg` 저장 일부는 R3에서 선행) | M·Y **45단위 / 5,657줄** |
| R8 | AGENTS 4~14차 요구사항 전용 모듈과 검증 | 차수별([1.3](#dotnet-r8-requirements)) | ⬜ 미착수. 결정 D1에 따라 앞서 옮긴 것: 5차 `UserDataRoot`, 6차 표시 부호화·`DisplayTextInput`, 7차 `ErrorLog`, 11차 `AiDifficulty`, Options의 Difficulty·Resolution 행 | X·W **29단위 / 4,553줄** |
| R9 | 60/120프레임 | cpppj F1·F2와 같은 옵션·프레임 시간 | ⬜ | (R8 X 영역의 `FrameRateOption` 포함) |
| R10 | Linux(DesktopGL) | `$HOME/.netstorm/` 저장(결정 D5) | ⬜ | — |
| R11 | TCP/IP 로컬 멀티플레이·Outpost | cpppj 3차를 로컬 네트워크만으로 | ⬜ 사전 구현 일부(`Netstorm.Net`, 검사 없음) | Z·V **131단위 / 10,322줄**(V 57단위 3,719줄은 루트 서버 독립 모듈이라 착수 때 필요 여부부터 판정) |

즉시 요구(단계와 무관): 11.01 표기·Credits 3항목·`Copyright 2026 HJOW`·외부 접속 문서화·유효성 검사 비활성화는 완료했다([9.1](#agents-reflection)).

### 0.3 숫자로 보는 현재

| 항목 | 값 |
| --- | --- |
| 마지막 구현 단위 | **0.562(2026-11-15, HJOW-Athlon):** [현재 자산 값 사본·최초 비용 owner](docs/dotnet-storage-assets-20261115.md). Core421/421·Client120/120, 신규7검사. TYPE/usage·프레임 코드·셰이프/헤더 독립 소유와 명시 Source의 최초 비용 지연 연결·재대여 보존. 실제 Client 자산뷰/Native 장치·파생과 호스트 전환은 잔여 |
| 검사 기준선 | 0.546 재개 전체 회귀 완주: **Assets366/366·Client일반1,232/1,232·Core2,393/2,394**(17분39초, 대형 독립 저장 포함). Core 실패1은 `NETSTORM_DATA` 누락이며 환경을 바로잡은 **관련155/155**에서 동일 검사와 신규11검사 통과. 최초 전체 종료1과 후속 통과를 구분한다. 실장치 오디오Explicit1 미실행. 0.562 Core421/421·Client120/120(신규7, 기존 관련408/408도 통과)·호스트 Release 0경고·0오류. TRX `extracted/dotnet-regression-0546/`·`extracted/dotnet-form-parent-0546-final/`, [근거](docs/dotnet-form-parent-20261115.md) |
| PORT_MAP | `PORT_MAP.tsv` **945행(L1 433·port 512)**, cpppj 416단위에 대응. 행은 파일 전체 완료를 뜻하지 않는다 |
| fixture | 연결 **313개**(원본 x86 대조 자료, `cpp-fixture-manifest.tsv`) |
| L2 기준선 | 0.562: cpp34는0.546 기준선 재사용(SHA 확인), dn일반31/31 신규 실행 종료0·수집 오류0·원본1,394파일 전후 동일. **일치0/34**, 데모dn3 미연결. dn31 상태는0.561과 동일(24경로 파일 전체 동일,7경로는 frame행만 차이). 자료 `extracted/dotnet-l2-0562/`, [이번 결과](docs/dotnet-storage-assets-20261115.md)·[일괄 도구](docs/dotnet-l2-batch-20261114.md) |
| cpppj 기준 | 제품 **1313 / 11.01**(M8 최종 관문 1311 완료, CD 10.72 복원은 사용자 보류, cpppj 목표 일시 중지). 영향 분류는 0.379(1247→1300 전수)·0.499(1304~1310)·0.530(1312·1313)까지 마쳤다([1.2](#resync-scope)). `reflected-commit`/floor는 올리지 않았다 |
| 오라클 | `cpppj/build/bin/Release/Netstorm.exe`. cpppj `src`가 바뀌면 PowerShell로 다시 빌드한다. 데이터 사본 `extracted/dotnet-oracle-data/`·캡처 `extracted/dotnet-oracle-captures/`·기준선 `extracted/dotnet-r0-baseline/`은 이 PC에만 있다(Git 제외) |
| 디컴파일 | dotnetpj는 디컴파일 소스 분석이 필요하지 않다(AGENTS.md `c17aeede1`). 이식 근거는 cpppj 소스·fixture·문서다 |

<a id="now"></a><a id="09-바로-다음-작업"></a>

### 0.4 남은 작업 — 순서대로

**작업 상태:** 사용자 재개 지시로 0.546부터 R4-1을 진행 중이다(2026-11-15). **0.562에서 최초 입력 TYPE/usage·프레임·셰이프/헤더의 독립 값 소유와 명시 자산 Source의 최초 로딩 비용 연결을 마쳤다. 다음은 실제 Client 자산뷰/Native 기본 장치·전용 파생·Runtime 조립이다.** 실제 Client Registry 전송 장치 설치, Native 파생 내용물·현재 finder TYPE와 나머지 파생 설치, 현재 HostSession의 별도 풀 전환 및 피해/해방·삭제/배치 상위 결선이 남아 있다. 사제 포획/희생·특정 적 공격·시나리오 승리 재현과 MP 실행 검사는 계속 사용자 확인 대상이다.

체크 대상은 AI 작업이다. 자연 완주·전체 동일성·Windows 11 실기는 AI 작업이 아니며 단계 전환을 막지 않는다([2.3](#stage-criteria)).

| 순서 | 작업 | 할 일과 완료 기준 | 시작 조건 |
| --- | --- | --- | --- |
| **1** | **R4 마무리 → M1** | 🔶 **진행 중.** [2.4](#r4-detail)의 R4-1 → R4-8 순서. 완료 기준(M1)은 34경로의 Load/Start 뒤 상태가 cpppj `session-dump`와 같은 것(L2). 한 조각이 끝날 때마다 34경로 일괄 덤프를 다시 떠서 일치 행이 늘었는지 본다 | 재개 즉시 |
| 2 | R5 → 이정표 D2, **`Netstorm.exe` 전환** | 월드 표시·카메라·HUD([2.4](#r4-detail) R5-1~R5-5). 34경로 진입 직후와 N프레임 뒤 캡처 차이 0. 이 시점에 메뉴·미션 기본 실행을 새 경로로 바꾸고 `Netstorm.exe` 이름과 배포 대상을 새 호스트로 넘긴다. 기존 경로는 `NetstormLegacy`로 내려 비교·개발용으로만 두고 배포에서 뺀다(결정 D2). R3 잔여 위젯·미룬 메뉴 장면 중 Multiplayer 섬 화면도 여기서 닫는다 | 1 뒤(R5-1의 표시 부품 이식은 R4와 병행 가능) |
| 3 | R6 → M2·M3 | 입력·선택·커서, 게임 메뉴·우클릭 메뉴, 규칙(사제·운반·건설·생산·전투·의식·미션)·AI·소리([2.4](#r4-detail) R6-1~R6-8). 고정 입력열 L3 상태 해시와 대표 장면 캡처 일치. 1-1·1-2가 새 경로에서 같은 수준 이상으로 진행되면 기존 경로(`NetstormLegacy`)와 그 검사를 삭제한다(결정 D2·결정 5) | 2 뒤 |
| 4 | R7 → M4 | 34경로 전체·데모 반복·진행 저장·Retry/Next/Abort, Edit 전체, `options.cfg`(9-1 C·D·F 판정), 배포 스크립트 갱신과 압축 해제 실행 확인 | 3 뒤 |
| 5 | R8 — AGENTS 4~14차 | [1.3](#dotnet-r8-requirements) 표의 차수별 할 일. 결정 D1에 따라 요구사항 코드 상당 부분이 1~4 단계에서 함께 들어오므로 R8은 요구사항 전용 모듈(MCP·FSR 출력기·스커미시 화면·시나리오 편집기·외부 글꼴 출력·14차 QuickJS 엔진 탑재) 이식과 요구사항 검증이 중심이다 | 4 뒤 |
| 6 | R9 60/120프레임 | cpppj F1·F2(옵션 `Frame Rate` 60/75 원본/120·양보 대기). 비행체 표시 보간은 cpppj D2를 따른다 | 5 뒤 |
| 7 | R10 Linux(DesktopGL) | 설정·진행 저장·커스텀 맵·로그는 `$HOME/.netstorm/` 아래 `Saves/`·`CustomMaps/`·`logs/`(결정 D5). Windows 전용 동작은 호스트 OS 분기에만 둔다 | 6 뒤 |
| 8 | R11 TCP/IP 로컬 멀티플레이·Outpost | cpppj 3차(M1~M7)를 로컬 네트워크만으로 옮긴다. MP 실행 검사는 cpppj처럼 사용자 확인으로 둔다 | 최후순위 |
| — | cpppj 변경 따라가기 | cpppj가 재개돼 `src`가 바뀌면 [1.2](#resync-scope) 방식으로 PORT_MAP 교집합만 다시 분류한다 | cpppj 커밋 발생 시 |
| — | 새 제보 처리 | 사용자가 오류·원본과의 차이를 제보하면 해당 항목을 AI 작업으로 올린다 | 제보 시 |

<a id="r4-board"></a>

**진행 원칙(2026-11-15 재정리):**

- **단위 = [2.4](#r4-detail) 체크 목록의 한 줄(cpppj 이름 계열) 또는 그 계열의 큰 파일 하나.** 0.500~0.545처럼 한 함수의 현재 부모·진단 경계를 하나씩 닫는 단위는 그 계열의 fixture 대조에 꼭 필요할 때만 쓰고, 기본은 "계열을 옮긴다 → cpppj 검사를 같이 옮긴다 → 호스트에 결선한다 → 34경로 덤프(R4)·캡처(R5)·상태 해시(R6)가 cpp 쪽으로 움직였는지 본다"다. 검증의 중심은 L2·L4a·L3 비교이고, 원본 fixture가 있는 모듈은 계속 L1을 연결한다.
- **진행 표시:** 단위를 마치면 2.4의 해당 줄을 `- [x]`로 바꾸고 끝에 단위 번호를 적는다. PORT_MAP에 행을 더하고 `python -X utf8 tools/port_inventory.py`로 0.2의 남은 양을 다시 센다(20단위마다 또는 단계가 바뀔 때).
- 단위마다 이 문서의 0.3·0.4와 2.4 해당 줄을 갱신하고, 경과는 `docs/dotnet-*.md` 단위 문서에 둔 뒤 로컬 커밋한다([6.2](#dotnet-ops)). 이 문서에 단위별 경과를 쌓지 않는다([8절](#dotnet-unit-records)).
- 실제 실행 검사에서 사제 포획/희생, 특정 적 공격, 시나리오 승리를 재현해야 하는 검사는 건너뛴다. MP 실행 검사는 하지 않는다. 단순 미션 진입·대기·생산·건설로 끝나는 비MP 검사는 수행한다(LEFT_JOBS.md 5.1과 같음).

<a id="decisions"></a><a id="08-사용자-확인이-필요한-것-확인-전까지-에이전트-기본값으로-진행"></a>

### 0.5 사용자 결정 (2026-11-11 확정)

이번 정리에서 사용자 확인을 기다리던 항목을 사용자가 2026-11-11에 모두 정했다. **현재 결정을 기다리는 항목은 없다.** 새 결정이 필요해지면 이 절에 표로 더한다. 아래 결정 D1~D5는 이정표 D1(메뉴 픽셀 일치)·D2(진입 화면 픽셀 일치)와 다르다.

| ID | 결정 | 내용과 적용 |
| --- | --- | --- |
| **D1** | **cpppj HEAD를 요구사항 코드까지 그대로 옮긴다**(권고안) | cpppj가 원본 복원 코드 위에 같은 파일로 얹은 요구사항 코드(Options의 Difficulty·Frame Rate·Language, Campaign의 Skirmish, Edit의 Scenario 패널, 모든 파일 경로의 사용자 홈 루트, 한글·일본어 표시 부호화, Credits 카피라이트 등)를 결정 2대로 함께 옮긴다. 각 단계의 완료 판정은 먼저 **기본 설정(영어·보통 난이도·Frame Rate 75 원본·4:3·창 모드)에서 cpppj와 같음**으로 하고, 요구사항 기능 자체의 검증·마감은 R8에서 한다. AGENTS의 "일단은 기존 게임과 완전히 동일하게 동작하는 것을 목표로 개발 후 요구사항을 반영"은 이 검증 순서로 지킨다. 저장 위치는 옛 계획의 "R7까지 게임 폴더 `d/`, R8 5차에서 사용자 홈"이 아니라 처음부터 cpppj와 같은 사용자 루트다(시험은 `NETSTORM_USER_DATA` 빈 루트) |
| **D2** | **새 경로가 `Netstorm.exe` 이름을 이어받는다. 작업이 끝나면 기존 경로는 제거한다** | 적용(AI 해석): 이름과 배포 대상은 새 경로가 메뉴와 미션을 모두 맡을 수 있게 되는 R5(이정표 D2)에서 한 번에 넘긴다. 그 전에 넘기면 배포판에서 미션을 할 수 없거나, 새 메뉴에서 기존 경로 미션으로 넘기는 임시 연결이 필요하다. 넘길 때 `Netstorm.Client.Host`의 어셈블리 이름을 `Netstorm`으로, 기존 `Netstorm.Game`은 `NetstormLegacy`로 바꾸고 배포에서 뺀다. 기존 경로와 그 근사 검사는 새 경로가 1-1·1-2를 같은 수준 이상으로 진행하면(R6, 결정 5) 삭제한다. 그때까지 기존 경로는 실행 오류 수정만 한다 |
| D3 | **dotnetpj 클론에도 4차 로컬 MCP를 붙인다**(권고안) | R8 4차에서 cpppj MCP(935~937)와 같은 도구 표면(stdio JSON-RPC로 게임 내 모든 조작·상태 조회·PNG 화면 캡처·소리 포함 녹화)을 만든다. 그 전까지 dotnetpj 개발의 분석은 cpppj MCP와 `--ui-script`·`session-dump`로 한다 |
| D4 | **한국어(6차)·일본어(13차) 화면을 R8보다 앞당기지 않는다**(권고안) | 결정 D1에 따라 언어 메뉴·표시 부호화 경계는 R3 이식에 함께 들어오지만 검증은 영어로만 한다. 외부 `fonts/`의 D2Coding·Noto Sans JP 글리프 출력과 번역 본문 적재·검증은 R8에서 한다 |
| D5 | **Linux 저장 경로는 `$HOME/.netstorm/` 아래 `Saves/`·`CustomMaps/`·`logs/`**(권고안) | Windows의 `%USERPROFILE%\Saved Games\Netstorm\`(`Saves\`·`CustomMaps\`·`logs\`)에 대응한다. 글자대로 바꾼 `$HOME/.netstorm/Saved Games/Netstorm/...`는 쓰지 않는다 |

**이번 정리로 닫힌 옛 확인 항목:**

- **옛 0.8 7번(알려진 버그 34건의 dotnetpj 반영) — AGENTS 변경으로 해소.** `8661e1060`·`60226ea93`이 dotnetpj 후순위 범위를 4~13차로 넓혀 10차(알려진 버그)가 dotnetpj 범위에 들어왔다. dotnetpj는 34건을 따로 조사하지 않고, cpppj HEAD의 수정 10건(NB02·NB04·NB08·NB09·NB10·NB18·NB20·NB23·NB24·NB29)을 해당 파일을 옮길 때 함께 옮긴 뒤 단위 기록에 "10차 NBxx 포함"으로 적는다. cpppj가 새 수정을 넣으면(LEFT_JOBS.md D4·D5) 같은 방식으로 따라간다.
- 옛 0.8 1번(기존 경로 처리)·3번(cpppj 미복원 경로)·4번(프로젝트 배치)은 결정 5·6·3으로 확정돼 [2.1](#dotnet-current-plan-20261028)에 남긴다.

<a id="user-checks"></a>

### 0.6 사용자 확인 대기 (AI는 실행하지 않음)

| 영역 | 확인할 것 |
| --- | --- |
| 플레이 동일성(L5) | 모든 시나리오(Early Missions·Priest Training·캠페인·데모)의 자연 완주와 전체 화면·음향 동일성. 단계에 이것만 남으면 다음 단계로 넘어간다(2026-10-31 사용자 결정) |
| 기존 경로 | 1-1·1-2 근사 플레이 중 실행 오류 제보(실행 오류 수정만 한다) |
| 환경 | Windows 11 실기는 단계 전환 조건이 아니다(Windows 10에서 오류 없이 구동되면 넘어간다). 다른 모니터/DPI·실물 4K FSR·Program Files/UAC 설치는 R8 이후 |
| 소리 출력 환경 | 0.516 HJOW-Athlon 실장치 검사가 DSERR_NODRIVER로 초기화 실패(cpp도 동일). 활성 출력 장치가 있는 Windows PC에서 Explicit 음소거 재생/복제/정지/재초기화 검사를 재수행한다. 독립 구현/단계 진행은 계속한다([검사 명령](docs/dotnet-sound-device-20261114.md)) |
| MP·LAN | 실제 LAN 접속과 MP 경로 동작(cpppj와 같은 사용자 확인 범위) |

## 1. cpppj 마감분 반영 (2026-11-11, 2026-11-15 갱신)

### 1.1 cpppj 현황과 dotnetpj에 주는 의미

cpppj는 제품 1313이며 AGENTS 1~14차 구현(14차는 엔진 탑재 범위)과 M8 최종 배포 관문(1311)을 마쳤고, CD 10.72 복원은 사용자 보류, 목표는 일시 중지 상태다([LEFT_JOBS.md 0.1](LEFT_JOBS.md#now), 2026-11-15 확인). 1312·1313은 명령줄 전용 테스트 자동 플레이와 11차 동맹 AI 판정 보정이다. dotnetpj 범위는 4~14차다.

- **이식 기준이 안정됐다.** cpppj가 멈춰 있으므로 dotnetpj는 HEAD 기준 파일 묶음 단위로 옮길 수 있다. cpppj가 다시 움직이면 [1.2](#resync-scope) 방식으로 교집합만 분류한다.
- **요구사항(4~13차)의 해석과 계약이 cpppj 구현으로 확정됐다(14차는 엔진 탑재까지만 확정, 연동 범위는 cpppj D7 뒤).** dotnetpj는 같은 계약을 옮긴다([1.3](#dotnet-r8-requirements)).
- **오라클 장치가 갖춰졌다.** `snapshot`(L4a)·`session-dump`(L2)·`--fixed-frame-ms`(L3)·MCP, 그리고 1312의 `--test-autoplay` 계열(아군 AI 자동 플레이·관전 카메라, [사용 안내](docs/testing/cpp-autoplay-test-guide.md))이다. 자동 플레이는 R6의 L3 비교에서 사람 입력 없이 전투·승리 장면을 만드는 데 쓸 수 있다(명령줄 전용, 일반 실행에는 없음).
- **dotnetpj 비대상:** CD 10.72 복원은 cpppj 전용이다. 단 cpppj가 `Patch1078` 게이트를 판본 분기로 바꾸면 dotnetpj가 이미 옮긴 판본 분기(PATCH/CD) 파일의 재대조가 필요하다. cpppj M8의 ZIP·검증 도구(`tools/verify_cpp_package.py`)는 dotnetpj 배포 검증(R7)의 참고 자료다.
- **cpppj 사용자 결정 중 dotnetpj에 닿는 것:** cpppj D2(비행체 보간) → R9 범위, D3(일본어 본문 번역·다른 언어) → 13차 범위, D4·D5(알려진 버그 재현 조건·공식) → 수정되면 HEAD로 따라간다, D6(스커미시 후속) → 12차 범위, D7(QuickJS 연동) → 14차 범위.

<a id="resync-scope"></a>

### 1.2 영향 재검토 범위 (0.4 'cpppj 변경 따라가기'의 입력)

| 항목 | 값 |
| --- | --- |
| 지난 검토 | 0.281: `f95a8db2..`(당시 HEAD, 제품 1247) — 자료 629행 중 458행 영향(L1 247·port 211), R4 우선 65행 목록. 재대조는 파일을 건드릴 때 하는 방식으로 미뤘다 |
| 이번 구간 | `4d4b3a928..HEAD`(제품 1247→1300): cpppj 47커밋, `cpppj/src` 184파일 변경(신규 39), 그중 PORT_MAP `cpp` 열에 있는 파일 47개 |
| 주요 변경 묶음 | M6 LAN·Native 등록(1258~1279), M7 5건(helpWord 캐시·메타 맵 커서·희생 안내·오류 로그, 1268~1273), K 한국어(1270·1276·1281), S 시나리오 편집(1277·1282~1284), B 알려진 버그 수정 10건(1285~1294·1300), L 일본어 UI(1288), 11.01 카피라이트(1291), D 난이도(1292), SK 스커미시(1293·1300), F 프레임(1296~1298), M8 배포(1274·1280·1295~1297) |
| 집계 방법 | `git diff --name-only 4d4b3a928..HEAD -- cpppj/src`와 `dotnetpj/PORT_MAP.tsv`의 `cpp` 열 교집합. 행별 `base` 이후 변경은 `git diff --name-only <base>..HEAD -- <cpp 파일>`로 본다(`cpp_sync.py --ports`는 0.147 이후 시간 초과라 쓰지 않는다) |
| 이후 변경(2026-11-12 정적 확인) | 제품 1302(`b833ecb51`: `CdDirectory.cpp/.h` 신규·`ClientMain.cpp` 1줄 — CD 자료 폴더 선택)와 1303(`93a589636`: `main.cpp` 8줄 — `--cd` 판본 옵션)은 모두 CD 10.72 전용이다. PORT_MAP에 있는 `ClientMain.cpp`(재검토 3번)·`main.cpp`(1번)에 닿지만 AGENTS가 CD 10.72를 cpppj에만 두므로 dotnetpj에는 영향이 없다. 두 행의 `base`는 올리지 않는다(두 파일의 잔여는 R1·R8에서 HEAD 기준으로 처리) |
| 추가 구간(0.530, 2026-11-15) | 1312(`382ec33fe`)·1313(`979d65b5d`)의15src파일은3이식(AiAllianceSide·RawAiCreate2),2C++빌드 분류,10상위모듈 이월이다. [전체 처분표](docs/dotnet-ai-alliance-delta-20261115.md). AI Host 설정 판정·실제 피격·테스트CLI/관전 결선은 R4/R5/R6 잔여이며 전체 반영으로 세지 않는다 |

<a id="dotnet-r8-requirements"></a>

### 1.3 R8 — AGENTS 4~14차 차수별 대응

AGENTS(`8661e1060`·`60226ea93`·`85551c6c2`)는 cpppj의 **4~14차**를 dotnetpj 후순위 요구로 둔다(webpj와 달리 제외 차수가 없다). 옛 문서의 4~8차 번호는 7차(오류 로그) 신설로 한 칸씩 밀렸다(옛 7차 AA·FSR → 8차, 옛 8차 시나리오 편집 → 9차, 옛 9차 알려진 버그 → 10차). 각 차수의 해석·세부 계약은 cpppj 구현을 기준으로 옮긴다(결정 2). 기존 경로에 이미 있는 요구사항 기능(한국어 선택·와이드·전체화면)은 버리지 않고 새 경로에서 cpppj 방식으로 다시 만든다.

| 차수 | AGENTS 요구 | cpppj 구현·근거 | cpppj 주요 소스 | dotnetpj에서 할 일 | dotnetpj 현황 |
| --- | --- | --- | --- | --- | --- |
| **4차** | 로컬 MCP(게임 내 모든 조작·화면 캡처·소리 포함 녹화), 16:9 와이드 최대 1920×1080(해상도 지원만, 그래픽 크기 스케일 조정 없음) | 935~937 [MCP](docs/exe/cpp-mcp-server-20261106.md), [와이드](docs/exe/cpp-wide-resolution-20261106.md)·[와이드 덱 배경](docs/exe/cpp-wide-deck-background-20261106.md) | `app/McpServer`, `platform/Json`·`Png`·`AudioLoopback`·`FrameRecorder`, `client/ScreenPresentation`·`ScreenResolutions.h`, 도구 `tools/mcp_cpp_smoke.py`·`mcp_cpp_record_smoke.py` | MCP는 결정 D3대로 dotnetpj에도 붙인다. 와이드는 cpppj 논리 해상도 방식으로 만들고 기존 경로의 시야 확장(`WideScreenMode`)은 교체한다 | 기존 경로에 근사 와이드만 |
| **5차** | 설정·캠페인 진행을 `%USERPROFILE%\Saved Games\Netstorm\Saves\`에, 커스텀 맵은 기존 폴더와 `...\CustomMaps\` 양쪽에서 읽고 Edit 신규 맵은 `CustomMaps\`에 저장, Edit 목록 스크롤·개수 무제한, 구동 중 자동 생성하는 **모든** 파일·디렉터리를 사용자 홈 아래로(읽기 전용 설치에서도 동작) | 932~934 [사용자 루트](docs/exe/cpp-user-data-root-20261105.md)(`CustomMaps\d\NAME.fort`·최초 Edit의 원본 맵 복사·한 화면 50행 스크롤), 1284 [읽기 전용 설치 확인](docs/exe/cpp-scenario-s4-readonly-home-20261111.md) | `platform/UserDataRoot`·`PortableStartup` | 결정 D1에 따라 파일 장치 이식 때 함께 옮긴다(R7 이전부터 사용자 루트). 기존 경로는 R5에서 배포에서 빠지고 R6 뒤 삭제되므로 `settings.json`은 옮기지 않는다(결정 D2). Linux는 `$HOME/.netstorm/`(결정 D5) | 새 경로 파일 장치(0.126·0.127)는 게임 폴더 기준 |
| **6차** | 한국어(D2Coding, 한국어 명칭 규칙) | K1~K5 1270~1281 [K1](docs/exe/cpp-korean-k1-merge-20261111.md)·[화면·입력](docs/exe/cpp-korean-screens-input-verification-20261111.md)·[K4/K5](docs/exe/cpp-korean-k4-k5-closure-20261111.md)·[글꼴 경계](docs/exe/cpp-font-face-encoding-20261103.md) | `client/DisplayTextInput`, 표시 전용 `EncodeDisplayText`/`DecodeDisplayText`, `lang\korean` 번역표, 외부 `fonts\` 적재 | 8비트 화면에 D2Coding 글리프를 그린다. 원문 8비트 문자열과 화면 표시 부호화를 구분한다. cpppj 배포는 저장소의 최신 D2Coding(1.4.0)을 넣으므로, 이때 [5절](#dotnet-user-decisions) 7번의 1.4.0 교체 점검을 함께 한다 | 기존 경로만(D2Coding 1.3.2) |
| **7차** | 오류/예외 처리·상세 로그 공통 구조(`...\Netstorm\logs\yyyyMMddHHmmss.log`, 원인 추적에 충분한 정보) | 1269 [오류 로그](docs/exe/cpp-error-log-20261111.md) | `platform/ErrorLog` | .NET 처리되지 않은 예외(`AppDomain.UnhandledException` 등)와 명시 오류 경로를 같은 위치·이름 규칙으로 기록한다. Linux는 `$HOME/.netstorm/logs/`(결정 D5) | 없음 |
| **8차** | 안티앨리어싱·FSR 1.0 선택(기본 꺼짐), FSR 지원 환경이면 2560×1440·3840×2160 추가(자동 FSR, 실질 1920×1080 업스케일) | 939 [AA·FSR](docs/exe/cpp-aa-fsr-20261106.md) | `client/GpuPresenter`·`FsrConstants`(Direct3D 11 후처리 출력기) | MonoGame 최종 출력 단계의 셰이더로 같은 상수·조건을 옮긴다. FSR 3.1 안은 쓰지 않는다 | 없음 |
| **9차** | Edit에서 시나리오 자체 편집(편집 모드 전용 UI: 초기 자본·테크·적 구성·동맹 등), 새 맵의 `.english` 기본 내용 자동 생성, `.english`가 없을 때 Test Battle 종료 수정 | S1~S4 1277·1282~1284 [S1](docs/exe/cpp-scenario-s1-merge-20261111.md)·[S2](docs/exe/cpp-scenario-s2-test-battle-20261111.md)·[S3](docs/exe/cpp-scenario-s3-ui-20261111.md)·[S4](docs/exe/cpp-scenario-s4-readonly-home-20261111.md) | `client/EditorScenarioController`·`EditorScenarioFiles`·`EditorScenarioModel`·`ScenarioDocument`·`UberScenario.inc` | R7 Edit 이식 뒤 | 없음(Edit 목록·이름 모달까지만) |
| **10차** | 10.78 알려진 버그 34건 확인·수정 | 판정 34/34, 수정 10건 [판정 현황표](docs/exe/cpp-known-bugs-status-20261111.md)·[NB01~NB34 목록](docs/exe/cpp-known-bugs-plan-20261109.md) | 건별(판정표 참조) | 해당 파일을 옮길 때 함께 옮기고 단위 기록에 NB 번호를 적는다. 따로 조사하지 않는다(0.5 아래) | — |
| **11차** | Options 난이도(쉬움·보통·어려움, 기본 보통). 쉬움은 적 AI 다리 건설·유닛 생성 주기 ×4와 채취 유닛 절반(최소 1), 어려움은 주기 ×½ | 1292 [난이도](docs/exe/cpp-difficulty-d-20261111.md)(AI 설정 +0x220/224/228 배율, AI 단위 값) | `o/AiDifficulty` | 세션 AI 설정 소유자(0.306)에 배율 필드를 HEAD대로 두고, 보통이 원본과 같은지 먼저 검증한다 | 없음 |
| **12차** | 스커미시(Campaign 메뉴 항목, 자체 UI: 적 AI 수·AI별 난이도/동맹·금지 유닛 최대 5개(Outpost 포함)·시작/취소, 섬 배치·간헐천은 MP처럼 랜덤) | 1293·1300 [스커미시](docs/exe/cpp-skirmish-sk-20261111.md)(미션 시작 전 `prepareWorld`가 군도 밀도→중앙섬→참가자별 전투섬 순서로 생성) | `client/SkirmishSetup`·`SkirmishMapList.inc`·`SkirmishStartIslands.inc` | 이미 옮긴 섬 생성(중앙·전투·군도 분리, 0.156)과 간헐천 생성을 MP 시작 순서로 묶는 경로를 옮긴다. 계약은 LEFT_JOBS.md 4.1 '스커미시 월드' | 없음 |
| **13차** | 기타 언어(일본어 — 추후 추가 예정) | 일본어 UI 1288 [일본어](docs/exe/cpp-japanese-l1-l3-20261111.md)(번역표 509키·외부 Noto Sans JP·`.japanese`가 없으면 영어 본문) | 번역표·표시 부호화·외부 `fonts\NotoSansJP-*` | 6차 다음. 본문 번역 범위는 cpppj D3을 따른다 | 없음 |
| **14차** | QuickJS 지원([quickjs-ng](https://github.com/quickjs-ng/quickjs)). 일단 엔진 탑재만 하고 어느 정도까지 연동할지는 추후 결정(AGENTS `85551c6c2`, 2026-11-11 추가) | **cpppj 1301 탑재 완료**([근거](docs/exe/cpp-quickjs-engine-20261111.md): QuickJS-ng v0.18.0 정적 링크, 엔진 생성·해제, 시작 자체 검사 6항목, `--quickjs-selftest`, 메모리 16MiB·스택 256KiB·1초 한도, 게임 상태·파일·네트워크 비노출). 연동 범위는 cpppj 결정 D7 대기 | `third_party/quickjs-ng/`, `platform/ScriptEngine` | cpppj의 탑재 계약(D7 ① 범위)을 같은 계약으로 옮긴다(결정 D1). AI 기본안: quickjs-ng 네이티브 라이브러리를 대상 플랫폼별(Windows x64 먼저, Linux는 9단계)로 빌드해 P/Invoke로 적재하고, 글꼴처럼 exe 밖 파일로 배포 ZIP에 라이선스(MIT)와 함께 넣는다. AGENTS가 QuickJS를 지정했으므로 Jint 같은 관리형 JS 엔진으로 대체하지 않는다. 엔진이 외부 접속을 하지 않음을 [dotnet 외부 접속 문서](docs/dotnet-external-endpoints-20261103.md)에 적는다 | 없음 |
| 후순위 | 60/120프레임 | F1·F2 1297·1298 [계획·결과](docs/exe/cpp-post-plan-fps-cd1072-20261111.md)(옵션 `Frame Rate` 60/75 원본/120, 양보 대기로 CPU 점유 감소·프레임 시간 동일) | `client/FrameRateOption` | R9([0.2](#dotnet-stage-board)) | 기존 경로에 근사 `FramePacing`만 |
| 후순위 | Linux(DesktopGL) | — | — | R10, 경로는 결정 D5 | 없음 |
| 최후순위 | TCP/IP 로컬 멀티플레이·Outpost(3차) | M1~M7 [LEFT_JOBS.md 1절](LEFT_JOBS.md#done) | `client/Native*`, `src/zacket`, `o/RawPacket*`·`RawConnection*` 등 | 로컬 네트워크만(`LocalNetworkPolicy` 루프백·리슨) | 사전 구현(검사 없음): `Netstorm.Net` 0.152·0.287·0.307·0.321·0.324·0.330~0.333, 세션 에디터 전송 0.293 |

**단계와 무관한 AGENTS 공통 요구:** `Netstorm.exe` 이름(결정 D2: R5에서 새 호스트로 이전), 11.01 표기·Credits 3항목(완료 0.150), `Copyright 2026 HJOW`(완료 0.342), 외부 접속 문서화·클라이언트 유효성 검사 비활성화(완료 0.151, 새 경로를 옮길 때마다 [dotnet 외부 접속 문서](docs/dotnet-external-endpoints-20261103.md) 갱신), 글꼴 exe 내장 금지·exe 옆 `fonts/`(기존 경로 준수), 배포 스크립트 `package-dotnetpj.ps1`(인자 또는 입력받은 경로·빈 값이면 취소·실행 파일과 필요 에셋 전부 포함 — 현행 준수, 구성이 바뀔 때마다 압축 해제 실행 확인).

<a id="cpppj-requests"></a>

### 1.4 cpppj에 전달할 요청 (이식 계획 8절의 남은 것)

dotnetpj 작업에서는 cpppj·LEFT_JOBS.md를 수정하지 않는다. 아래 요청은 사용자에게 전달한다. 요청 1·2(`session-dump`·고정 시각 실행)는 cpppj `c91d9b84`와 dotnetpj 0.283·0.284로 해결됐다.

| 번호 | 요청 | 상태(2026-11-11) |
| --- | --- | --- |
| 3 | fixture TSV 첫 주석 줄에 열 이름 | 남음 |
| 4 | 모듈별 연결 상태표(독립 대조만 / 실제 Session 연결 / GUI 연결) | 남음. cpppj [SOURCE_MAP](cpppj/SOURCE_MAP.md)로 대신할 수 있는지는 다음 cpppj 영향 재검토 때 확인 |
| 5 | `cpppj/tests/GraphTests.cpp:105`가 `graph-x86.tsv` 객체 열 `v[11]`(범위 밖, 객체는 정수 11개)을 읽음 | 남음(HEAD에서도 같은 줄) |
| 6 | `world.ClearProcesses()` 뒤 `FindEvent(sid,1)`가 null 대신 미등록 진단(`PlayerWorld_SameSurfaceCoreRecordsAndInitialRegular` 꼬리, 0.305) | 남음. dotnetpj는 원본 동작 그대로 둔다 |

<a id="dotnet-current-plan-20261028"></a><a id="05-실행-순서-r0r8"></a>

## 2. 현행 계획 — cpppj를 오라클로 한 구조 보존 직역 (2026-10-28 수립, 2026-11-11 갱신)

**진단(2026-10-28):** 기존 구현은 화면이 RGBA 합성이고, UI가 원본 Gump 트리가 아닌 자체 배치이며, 규칙이 근사 세션(`BattleSession`)에 있었다. 옮겨 둔 `Original` 모듈은 세션에 연결되지 않아 플레이를 바꾸지 못했고 같은 입력에서 cpppj와 비교할 수단도 없었다. 그래서 아래 구조로 다시 세웠다.

### 2.1 구조 결정 (계속 유효)

- **결정 1 — 화면은 8비트 논리 화면으로 옮긴다.** cpppj의 `Screen`·`Renderer`·팔레트·셰이프·색표·Gump Draw를 C#으로 옮긴다. 그리기 대상은 팔레트 인덱스 버퍼(`byte[]`, 행 간격 포함)와 256색 팔레트다. MonoGame은 창·입력 장치·오디오 장치·**최종 출력**만 맡는다. 최종 출력은 인덱스를 RGBA로 바꿔 텍스처를 갱신하고, cpppj `ScaleScreenPresentation`과 같은 정수 픽셀 중심 최근접 확대와 검정 여백을 쓴다. 게임 화면 경로에서는 SpriteBatch 합성·실수 알파·`RenderTarget2D` 확대를 없앤다. 9-1 E(부분 다시 그리기)는 cpppj Renderer의 dirty 경로로 해소한다.
- **결정 2 — cpppj 구조를 보존해 직역한다.** cpppj의 파일·클래스·함수 경계와 이름, 부작용과 호출 순서, 정수 폭(BYTE/WORD 감김), float32 저장 지점, 목록 순서, 난수 소비를 그대로 옮긴다. cpppj가 원본 메모리 배치(슬롯·전역 DWORD)를 쓰는 곳은 C#도 같은 배치를 쓴다. Win32·DirectDraw·GDI·WINMM·파일 장치처럼 cpppj에서도 대체 경계인 부분만 .NET/MonoGame으로 바꾸고 차이와 이유를 한국어 주석에 적는다. Windows 전용 동작(커서 제한 등)은 호스트의 OS 분기에만 두어 Linux 작업을 막지 않는다. 요구사항 코드가 섞인 파일도 그대로 옮긴다(결정 D1).
- **결정 3 — 프로젝트를 cpppj 디렉터리와 짝지어 배치한다.**

  | 프로젝트 | 대응하는 cpppj | 내용 |
  | --- | --- | --- |
  | `Netstorm.Core`(`Original/` 등) | `src/o` | 원본 월드 코어·규칙 모듈. 기존 경로용 근사 코드(`Simulation`·`Rules`·`Bridges` 등)도 아직 같은 프로젝트에 있다 |
  | `Netstorm.Client` | `src/client` 중 플랫폼 독립 부분 | 화면 버퍼·Gump·Renderer·세션·HUD·소리 논리·Chat·Mission. MonoGame에 의존하지 않는다 |
  | `Netstorm.Client.Host` | `src/app`·`src/platform`의 대체 부분 | 새 경로 MonoGame 호스트(개발용) |
  | `Netstorm.Game` | — | 기존 경로 MonoGame 게임(동결, 결정 5) |
  | `Netstorm.Assets` | `GameAssets`·`RiftTypeTable`·`Template`·`Config*` 등 | 자료 판독. cpppj HEAD와 재대조한다 |
  | `Netstorm.Net` | `src/zacket`, 연결 계열 | 멀티플레이. 최후순위(현재 사전 구현만) |

  대응표는 [PORT_MAP.md](dotnetpj/PORT_MAP.md)·`PORT_MAP.tsv`(cpppj 파일 → C# 파일 → 이식 기준 cpppj 커밋 → 상태)이며 [cpppj/SOURCE_MAP.md](cpppj/SOURCE_MAP.md)와 짝을 이룬다.
- **결정 4 — cpppj 실행 파일을 판정 기준(오라클)으로 쓴다.** cpppj는 원본 기계어와 대조된 결과이므로 dotnetpj는 같은 입력에서 cpppj와 같아야 한다. 화면은 cpppj `--run … --ui-script <steps.tsv>`의 `snapshot` 논리 캡처(팔레트 변환 BMP)를, 상태는 헤드리스 출력(`--dump-world`·`--dump-types`·`--inspect-*`)과 ui-script `session-dump`(L2)·`--fixed-frame-ms`(L3)를 기준으로 쓴다([형식](docs/exe/cpp-session-state-dump-20261109.md)). dotnetpj 호스트도 같은 문법의 실행기와 같은 형식의 캡처·덤프를 낸다. 비교 도구는 `dotnetpj/tools/`에 둔다.
- **결정 5 — 기존 경로는 동결한 뒤 교체·삭제한다.** 기존 `BattleSession`·`FortMapViewer`·`MainMenuView` 등에는 새 기능 추가와 부분 수선을 멈춘다(실행 오류 수정만 예외). 새 경로가 메뉴와 미션을 모두 맡게 되는 R5에서 기본 실행과 `Netstorm.exe` 이름·배포 대상을 새 경로로 넘기고, 기존 경로는 `NetstormLegacy`로 내려 비교·개발용으로만 둔다(결정 D2). 1-1·1-2가 새 경로에서 같은 수준 이상으로 진행되면 기존 경로와 그 검사를 삭제한다. 근사를 기대하는 기존 검사는 기대값을 고치지 않고 대체되는 코드와 함께 은퇴시키며 은퇴 목록을 단위 기록에 남긴다. 원본 fixture를 쓰는 검사는 대상 모듈을 옮긴 뒤에도 유지한다.
- **결정 6 — cpppj가 미완·보류로 둔 범위는 추측으로 채우지 않는다.** cpppj가 명시 경계·미복원·보류로 둔 동작은 dotnetpj 새 경로에서도 같은 경계로 둔다. cpppj가 명시 오류로 멈추는 미연결 경로는 같은 지점의 명시 예외·진단으로 옮긴다(예: LEFT_JOBS.md 4.1의 NoWorld 경계). 옛 "미복원 동작은 근사를 유지한다"는 동결된 기존 경로에만 남는다.

<a id="04-기존-dotnetpj-소스의-재수정-판정"></a>

### 2.2 기존 dotnetpj 소스의 재수정 판정

"교체"는 cpppj 대응 모듈을 새로 옮긴 뒤 기존 파일을 지운다는 뜻이다. "재대조"는 cpppj HEAD와 같은 입력에서 결과·오류 처리를 비교해, 같으면 유지하고 다르면 cpppj 구조로 바꾼다는 뜻이다. 아직 은퇴시킨 기존 코드·검사는 0건이다.

| 기존 dotnetpj | 판정 | 기준이 되는 cpppj | 단계 |
| --- | --- | --- | --- |
| `Game/NetstormGame`·`DisplayManager`·`PerfMeter`·`FrameWallClock`, `Core/Display/ScreenLayout`·`WideScreenMode`·`FramePacing`·`DisplaySettings` | 교체. 호스트에는 창·입력·오디오 장치·출력만 남긴다. 와이드 시야 확장은 cpppj 논리 해상도 방식으로 바꾼다. 새 경로는 cpppj와 같은 사용자 루트의 `options.cfg`를 쓴다(결정 D1). 기존 경로의 `settings.json`은 기존 경로와 함께 삭제한다 | `ClientMain`, `Screen`, `ScreenPresentation`, `ScreenResolutions.h`, `ScreenPaletteApply`, `FrameRateOption` | R1(설정은 R7) |
| `Game/UiAutomation` | 교체 | `app/main.cpp`의 `--ui-script`·`snapshot` | R1 |
| `Core/Display/OriginalGameClock`·`FrameLoopClock`, `Simulation/BattleSessionFrameClock` | 유지 후 재대조(L1 검증 있음). `ClientMain` 루프에 맞춰 새 호스트에 다시 연결 | `ClientMain`, `o/GameClock`, [main-loop.md](docs/exe/main-loop.md) | R1 |
| `Game/SpriteAnimation`·`FortMapViewer.Sprites`, `Assets/ObjectColorRemap`·`IsleColorRemap`·`ProductionTintRemap`·`MapSpriteFrames`·`StructureAnimation`·`BitmapFontAtlas`, `Core/Display/DrawOrder`·`DisplayBounds` | 교체. `DrawOrder`·`DisplayBounds`의 L1 대조는 새 모듈의 검사로 흡수 | `VFXDraw`·`VFXMappedDraw`·`VFXLine`·`VFXTransform`, `RawShapeToBuffer`, `SpriteDraw`, `*ColorMaps`, `Palette*`, `DrawSort`, `o/SquidDisplay` | R2·R5 |
| `Game/MainMenuView`·`OriginalUiSkin`·`HelpWindow`·`OriginalFonts`·`OriginalCursor`, `Core/Rules/ButtonGump`, `Assets/CampaignMenu`·`StartupTips`·`HelpDocument`·`HelpTopics`·`InlinePicture` | 교체 | `Gump*`, `ButtonGump`·`StoneButtonGump`, `MenuGump*`, `ScrollGump*`, `TextGump`, `HtmlWindow*`·`HtmlBodyGump`·`Markup`·`StyleText*`, `QuickHelp*`, `TellTip`, `UberGump`·`UberRootGump`, `NativeMainMenuState`, `BitmapFont`, `Cursor` | R3 |
| `Assets/BitmapFont`(`.chfnt`)·`Palette`·`GifImage`·`ShapeDatabase`·`CursorBitmap`·`TaffArchive`·`XlatTable`·`XorCipher`·`WaveFile`·`PatternDecoder`·`Config*`·`Type*`·`OriginalText`·`GameFileSystem`·`GameDataLocator`·`GameResources`·`GameLanguage`·`GameUnitNames`·`TrueTypeCollection` | 재대조. L1/정적 대조 기록은 있으나 기준 시점이 10-07~10-10이다 | `GameAssets`, `RiftTypeTable`, `Template`, `Config*`, `Xlat`, `GifImage`, `BitmapFont`, `RawImage` | R0·R2 |
| `Assets/FortFile`·`FortMap` / `FortTerrainPreview`·`FortTerrainFringe`·`FortEdgeFarmPreview`·`FortIslandSupports` | Fort 판독은 재대조(`FortFile`은 연결하지 않은 옛 판독기라 R4 Fort 적재 연결 때 은퇴), 지형 미리보기 4종은 교체 | `RawFort*Load`, `RawCampaignWorldLoad`, `TerrainBuilder`, `RawTerrain*`, `Islandbuilder`·`RawIsland*`, `RawFarmCluster` | R4 |
| `Assets/MissionScript`·`MissionConditions`·`TutorialDialogScript` | 교체 | `TellSection`·`TellParameters`·`DialogScript`·`Html*`·`ScriptCommand`·`RawMissionGuide` | R3·R6 |
| `Core/Simulation/BattleSession.*`(15파일)·`BattleSessionFactory`·`GameEntity`·`GameCommands`·`SessionEvents`·`PlayerState`·`UnitProduction`·`*Animation`·`CombatImpact`·`FlyerFlight`·`MovementRate`·`UnitHeading`·`ConstructionTimes`·`TutorialStages`·`TutorialGeysers`·`FixedTimestep` | 교체 후 삭제 | `RawCampaignSession*`, `RawCampaign*Bindings`, `RawSurfaceWorld`, `State`, `Mission*`, 대응 `Raw*` | R4·R6 |
| `Core/Rules/*`(25파일: `BattleMap`·`SpotMap`·`Footprint`·`IslandOwnership`·`Energy*`·`StormPower`·`ProductionDeck`·`KnowledgeCatalog`·`PlacementCursor`·`RangeIndicator`·`SelectionMarks`·`GameCursor`·`MiniMapLayout`·`PlayerColors`·`MissionStart` 등) | 교체 후 삭제. `SpotRules`의 L1 대조는 새 해시 모듈 검사로 흡수 | `SquidHash`·`SquidSpatial`, `CanonDecoder`·`RawCanonPlacement*`, `RawEnergySupply`, `SpStore`, `RawProductionDeck`, `RawPlayerTechnology`, `RawCursor*`, `MinimapGeometry`, `RawPlayerColor`, `RawMissionStart` 등 | R4~R6 |
| `Core/Bridges/*`(13파일) | L1 대조된 `BridgeDecayRules`·`SurfaceFinder`·`BridgeSurfaceRules`·`SurfaceGraph`는 `Original` 쪽 대응 모듈로 합친다. 나머지는 교체 후 삭제 | `Bridge`, `Graph`·`RawGraph`, `SquidFinder`, `RawBridge*`, `CanonDecoder` | R4 |
| `Simulation/MissionFrameGate`·`PlayerParticipation`·`MissionParticipation`·`TotalMade`·`TutorialOne`·`NetstormRandom`·`MsvcRandom`·`SidPool` | 유지 후 재대조(L1 검증 있음). 세션 조립 때 cpppj와 같은 소유 위치로 재배선. `SidPool`은 `RawSidPool`과 하나로 정리 | `RawMissionFrameGate`, `RawPlayerParticipation`, `RawTotalMade`, `RawTutorialOne`, `CrtRandom`, `SidPool` | R0·R4 |
| `Core/Original/*` | 유지. 영향 재검토(0.379 종결) 뒤 세션 조립에 쓴다 | 대응 `o/*` | R0·R4 |
| `Core/Audio/*`, `Game/AudioPlayer`, `FortMapViewer.Audio` | 교체. 장치 계층만 MonoGame | `Sound`·`SoundProcess`·`SoundMusic*`·`SoundSceneMusic`·`SoundDevice`·`ClientAudio` | R6 |
| `Game/FortMapViewer*.cs`(26파일) | 교체 후 삭제 | `Renderer`·HUD·커서·선택·메뉴, `RawCampaignSession`의 입력 | R5·R6 |
| `Game/SpriteBrowser` | 개발 도구로 유지하되 게임 경로와 분리 | — | 필요 시 |
| `package-dotnetpj.ps1` | 유지. 프로젝트·자산 구성이 바뀔 때마다 갱신하고 압축 해제 실행을 확인한다. 글꼴은 exe 옆 `fonts/`에 별도 파일로 넣는다. R5 전환 때 배포 대상을 새 호스트(`Netstorm.exe`)로 바꾸고 `NetstormLegacy`는 넣지 않는다(결정 D2) | `cpppj/package-cpppj.ps1`(참고) | 매 전환 |

<a id="stage-criteria"></a>

### 2.3 단계별 범위와 완료 기준 (R3~R7)

각 단계 괄호의 W 번호는 [이식 계획](docs/dotnet-cpp-port-plan-20261016.md) 5절의 근거 목록이다. R0~R2는 완료했다([7절](#dotnet-done-index)).

**단계 전환 기준(2026-10-31 사용자 결정, cpppj와 같음):** 모든 시나리오의 자연 완주와 전체 동일성은 사용자가 직접 플레이하며 확인하고, 단계에 남은 일이 이것뿐이면 다음 단계로 넘어간다. 각 단계의 L1~L4 대조는 AI가 한다. Windows 10에서 오류 없이 구동되면 Windows 11 실기 검증 없이 넘어간다. 사용자 제보는 해당 항목을 AI 작업으로 올린다.

- **R3 — Gump UI와 메인 메뉴 계열(W15) → 이정표 D1: 메뉴 화면 픽셀 일치.** 옮길 것: Gump 프레임워크 전체, 버튼·돌 버튼·메뉴·스크롤·텍스트·텍스트 입력·HTML 창·메시지·이미지·배경 GIF 위젯, `UberGump`·`UberRootGump`·`RawUberRootControls`·`NativeMainMenuState`, 로딩 화면(`MissionLoadingMessage`·`RawMenuLoading`), Tell·Dialog·StyleText·Markup·QuickHelp, 기본 커서. 범위: 메인 메뉴 8버튼, Campaign/Demo/Help/Options(해상도·음량·Window Mode)/Credits/Tips/Quit, 캠페인 목록, 브리핑 창(결정 D1에 따라 cpppj HEAD의 Difficulty·Frame Rate·Language·Skirmish 항목도 함께 옮기되 기본 설정 화면으로 비교). **완료 기준:** 같은 ui-script로 각 화면·하위 메뉴·Help 쪽·Credits 쪽을 캡처해 cpppj와 차이 0. 메뉴 기본 실행 교체는 R5에서 미션과 함께 한다(결정 D2).
- **R4 — 월드 세션 조립(W02·W03 잔여, W04~W07) → M1 시작 상태 L2.** 옮길 것: 시간·Kernel·Process·Regular, SP 저장소·보상·난수의 실제 연결, Fort 적재(객체·월드·메타·돈·소유자 마감·TypeNames·Deck), 지형·섬·농지·영역, 다리 연결, Player·기술·색, 미션 Start·로그인·타이머·Guide, AI 생성과 초기 갱신, 생산 Deck. `RawSurfaceWorld`/`RawWorldCore`와 `RawCampaignSession`의 소유 구조를 그대로 옮기고 기존 `Original` 모듈을 그 안에 끼운다. **완료 기준:** 34경로(Early6·Training9·캠페인16·데모3) 각각의 Load/Start 뒤 상태가 cpppj와 같다. cpppj 쪽은 ui-script `session-dump`(판번호 1)로 얻고 dotnetpj 호스트도 같은 형식을 낸다. 양쪽 모두 같은 `--fixed-frame-ms`와 빈 사용자 루트를 쓴다([6.1](#dotnet-commands)).
- **R5 — 월드 표시·카메라·HUD(W14 나머지, W15의 HUD) → 이정표 D2: 진입 화면 픽셀 일치.** 월드 표시(`Renderer`·`DrawSort`·`DrawSubmission`·`RawRenderCollection`·`ObjectCollection`·`TileGroupCollection`·`PreviewCollection`·`PreviewDraw`·`SquidRenderer`, 지형 바인딩, 타입별 Draw, 시각 시스템 Burning·Construction·Dissolve·FieryTail·Lightning·Shrapnel·Smoke·RenderProcess, KenCloud, 파티클, 번개 화면, Fort 팔레트 전환), 카메라(`RawCameraView`·`RawCameraCommit`·`RawCameraPresent`·`CameraHome`·`UberGumpCamera`·`RawEdgeScroll`), HUD(`UberHud`·`HudDock`·`HudNsInfo`·채팅 표시, 미니맵, `FloatingSpGump`·`ProductionHintGump`, 덱 `InventoryGump`). **완료 기준:** 34경로의 진입 직후와 N프레임 뒤 캡처가 cpppj와 차이 0. 애니메이션 장면은 같은 `--set`(maxFPS·sleepPerLoop 등)과 `--fixed-frame-ms` 조건에서 비교하고, 맞출 수 없는 장면은 비교 범위를 기록한다. 이 시점에 메뉴·미션의 기본 실행과 `Netstorm.exe` 이름·배포 대상을 새 경로로 넘긴다(결정 D2).
- **R6 — 플레이 입력·규칙·소리(W08~W13, W16, W19의 AI) → M2·M3.** 입력·선택·커서(`UserInput`·`GameWorldInput`·`RawUserPick`·`RawObjectHit`·`RawUserSelection`·`RawUserHighlight`·`RawCampaignCursor*`·`RawCursor*`), 게임 메뉴(QWASZX `RawProductionControls`·`ProductionShortcuts`, ESC 가로·하위 메뉴, Player List·F9, 우클릭 메뉴 전체, Warning·MessageBoard), 규칙(사제·경로·운반·수확, 배치·건설·생산, 전투·방어·파생 건물, 의식·신전·제단, 미션 진행·안내·승패, 공통 전략 AI `RawAi*`), 소리(소리 Process·음악 선택·흐름·장면 음악, 장치는 MonoGame). `arc-spire`·`acid-fence`·`ice-tower` fixture는 해당 몸체 클래스를 옮길 때 연결한다. **완료 기준:** 고정 입력열의 L3 상태 해시(프레임별 `session-dump`)와 대표 장면 캡처가 일치하고, 1-1의 건설 파이프라인이 원본 경로로 동작한다(M2). 1-1·1-2가 새 경로에서 진행되면 기존 경로와 그 검사를 삭제한다.
- **R7 — 미션 전체·Edit·설정·배포(W17·W18, 9-1 C·D·F) → M4.** 34경로 전체, 데모 3개의 반복, 진행 저장과 다음 미션 해제, Retry/Next/Abort. Edit(편집 메뉴·섬 추가·저장·Test Battle·SaveGoMain, `.english` 등 언어 파일; Edit 제외는 webpj에만 해당). `options.cfg`를 원본의 시점·형식으로 읽고 쓰되 전체화면 전환 뒤 재실행 오류는 허용하지 않는다. 9-1 D·F를 판정하고 Windows 10에서 확인한다. 저장 위치는 cpppj와 같은 사용자 루트다(결정 D1). 배포 스크립트를 갱신하고(`Netstorm.exe`, 글꼴은 exe 옆 `fonts/`) 압축 해제한 사본에서 실행되는지 확인한다. **완료 기준:** 34경로 짧은 진입과 핵심 장면이 cpppj와 일치한다. 자연 완주(L5)는 완료 기준에 넣지 않는다.
- **R8** — [1.3](#dotnet-r8-requirements) 표.

<a id="r4-detail"></a>

### 2.4 단계별 남은 작업 상세 (R4~R11, 2026-11-15 cpppj 소스 대조로 재수립)

현재 cpppj 소스(제품 1313)의 모든 단위를 `PORT_MAP.tsv`와 대조해, 대응이 없는 831단위를 단계에 배정했다. 아래 체크 줄 하나는 "같은 머리 이름을 쓰는 cpppj 단위 묶음(계열)"이며 `디렉터리/계열*` 단위 수·줄 수와 단위별 줄 수를 적었다. 목록은 `python -X utf8 tools/port_inventory.py --families dn`으로 다시 만들 수 있다([2.7](#dotnet-inventory)). 영역은 이름으로 나눈 근사 분류이므로, 착수할 때 그 계열이 실제로 그 단계 몫인지 cpppj 호출 관계로 한 번 확인하고 아니면 맞는 단계의 목록으로 옮긴다. **이미 대응이 있는 큰 파일의 미완 부분**(`RawCampaignSession`·`ClientMain`·`RawSurfaceWorld`·`RawSurfaceStorage`·`Renderer`·`SquidPostPop`·`RawMenuControls` 등)은 목록에 나오지 않으므로 각 조각의 "할 일"에 따로 적었다.

#### R4 — 월드 세션 조립 → M1 (현재 단계)

**지금까지(0.128~0.545, 묶음별 경과는 [보관본](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261115-through-0545)의 R4 진행판과 [7.1](#dotnet-done-index)):** Template·Fort 저장소와 적재, 지형·섬 생성, 다리·섬 연결, Player·캠페인 월드, 서피스 월드 기반·프로세스·호스트, 월드 본체, F3 세션 관문과 소유 상태, 그리고 실제 호스트 조립(미션 Fort 적재 → 세션 홀더 → Player·SP·AI 설정·동맹 → Zone·가입·Ready → 이름·요새 순번 → 월드 적재 흐름 → 타입·프레임 → 지형·난수 저장소 → 섬 부모 → Generate 라이브 → 난수 단일 스트림과 시작 1회 reseed → Pop 공간 등록 → 요새 객체 적재 → 그래프 재구성 → Player 화신·사제 Pop). 0.500~0.545에서는 객체 수명 공통층(AI 본체·기술·에너지 부품, 사제 조회·Pop·낙하·회복·보호막·소리, Carrier 전처리, 배치 기하·후보 충돌, 피해 전처리·피격·삭제 후처리·종속 해방, 연소 생성·프레임·팔레트, 공통 표시 등록, 공통 삭제·풀 반납, ProcessHost, Form 해제, Take·Create·Unpop 현재 부모)을 원본 관측과 대조해 호스트에 연결했다.

**덤프가 아직 다른 이유(0.462~0.477 조사로 확정):** 난수·SID 집합·지형 타입은 맞는다. 다른 것은 ① 적재 중 postPop이 찍는 지면·받침 소유자 스탬프(예: thewarbegins isle-97 1,353건), ② 농지 후처리 타입(dn 97 대 cpp 121), ③ 그래프 per-Pop 번호(일치 120/255), ④ 장부 totals·totalmade(dn 0), ⑤ 정기 처리·프로세스 행, ⑥ AI brain, ⑦ Player 첫 지역(+0x78, dn은 전 경로 127), ⑧ 데모 3경로 진입이다. 이전 문서는 ①~④를 "R6 postPop 파이프라인 소관"으로 적었으나, M1(Load/Start 상태 일치)에 필요하므로 이번 재정리에서 **R4-2로 당긴다.**

| 조각 | 할 일 | 주 cpppj 근거 | 끝났다는 증거 |
| --- | --- | --- | --- |
| **R4-1** 객체 수명 공통층 마감 | 🔶 0.546~0.562에서 현재 Form·Content·Pool·Graph/PostPop 공통부·일반/준비 대여·World 기본 반납과 공유 상태/전역 주소·장기 Graph/이웃/Owner/표시와 typed 공장/건물/Unpop·공통 종속·장기 전송/공간과 현재 자산 소유자 분배·입력 자산 값 사본/최초 비용 연결 완료. `NativeStorage` 파생 내용물·실제 Client 전송 장치 설치·실제 Client 자산뷰와 Native 장치/실제 Runtime pin, 나머지 파생·현재 HostSession 별도 풀 전환·상위 결선은 잔여. 이 층이 닫혀야 R4-2 이후가 같은 저장소를 쓴다 | `client/RawSurfaceStorage`·`RawSurfaceWorld`(대응 있음, 재검토 16·17번 잔여), `client/RawSurface*` 11단위, `o/RawSquid*`·`RawContent*`·`RawDepend*` | 대표 미션 덤프의 slot 행이 변하지 않고 집중 검사 통과 |
| **R4-2** 적재 중 Pop 후처리 | 지면·받침 소유자 스탬프(`IslandPostPop.SetSupportOwner` 3×3·`Prefix`), 농지 배치 파이프라인(타입 121), 표시 등록, 그래프 per-Pop 번호 부여, 장부 totals·totalmade 집계(`PostPopBase`) | `o/SquidPostPop`(대응 있음, 재검토 30번 잔여)·`SquidPostPopPatch`, `o/RawRegionPaint`·`RawRegionOwnership`, `o/RawGraphLive`, `o/Islandbuilder`, `o/RawFarmCluster`(대응 있음) | 덤프의 owner·type·graph·totals 행이 cpp와 같다 |
| **R4-3** 신전·소용돌이 사슬 | 신전 Pop → 소용돌이 → 섬 소유자 → Player 첫 지역(+0x78). 단일 미션의 기록자는 전투섬 배정 갈고리가 아니라 이 사슬이다(0.477) | `o/RawVortex*`·`RawTemple*`·`RawRune*`·`RawDais*` 중 적재 경로(L 영역, R6-5 목록에서 당겨 온다) | 경로별 +0x78이 cpp와 같다(surrounded의 무신전 127 포함) |
| **R4-4** 정기 처리·프로세스 초기 등록 | 적재 직후 등록되는 Regular와 프로세스, `RawSurfaceStorage.UpdateAmbientWind` 연결 | `o/RawAssetRegular`·`RawDaisRegular`·`RawDudeRegular`·`RawPlayerRegular`·`RawDiscRegular`·`RawBuriedRegular`·`RawDecorationRegular`·`RawProcess*`, `client/RawSurface*Regular` | 덤프 kernel·regular·process 행이 cpp와 같다 |
| **R4-5** AI 생성·초기 갱신 | 호스트의 AI 호출 자체가 아직 없다(0.503). brain 실생성자, `CrankAiMoney`·`SetAiParameter`·`ResetAiForState`·`ReturnAiForState`, 600바이트 검사·준비 00416eb0·258 완료 판정, 설정+23c 동맹 판정(0.530) | `o/RawAiCreate`(대응 있음)·`RawMakeAi`·`RawAiStrategySetup`·`RawAiPreparation`·`RawAiWallet` 등 K 영역 중 생성·초기 갱신 경로(R6-6 목록에서 당겨 온다) | AI가 있는 미션의 덤프 brain·SP·Player 행이 cpp와 같다 |
| **R4-6** 로그인·내부 전송·Client 전역 수명 | 메뉴에서의 미션 시작(핸들 출처, 이름 꼬리 16~19바이트), 로컬 접속 → 로그인 → Ready → Start의 실제 패킷 경로, 요새 이미지 전송, 세션 `Bind*` 대리자의 실제 몸체, State 전환·정리 | `client/RawCampaignSessionLogin`·`ClientCampaignTransport`·`CompositeMissionTransport`·`CampaignClientServices`·`CampaignTransportServices`·`RawCampaignWorldRuntime`·`ClientState*`·`ClientPlayerRecords`·`ClientLocalStateGlobals`, `o/RawPacket*`·`RawConnection*`·`RawMission*`·`RawLocalMissionState`, `client/RawCampaignSession`(대응 있음, 재검토 12번 잔여) | cpppj `CampaignSession_LocalLoginActualHandshakeAndWorldLoad` 대응 검사 통과, 메뉴 경유 진입 덤프가 `--mission` 직행과 같다 |
| **R4-7** 데모 3경로·미뤄 둔 전수 재생 | `StartDemo` 진입 연결, `player-world-x86.tsv` 1,680행(월드 스택 하네스), 0.134~0.154에서 F3로 미룬 cpp 검사(`FortWorldLoadBindingTests`·`NormalTerrainWorldBindingTests`·`CampaignMissionFortPathTests`·`CampaignLoginObjectStreamTests`·`CampaignMissionReloadSourceTests` 등) | `o/RawMissionStart`(대응 있음), `client/RawCampaignMissionStartBindings`·`RawCampaignTutorialOneBindings` | 데모 3경로 dn 덤프 생성, 미룬 검사 이식 수 기록 |
| **R4-8** M1 판정 | 34경로 같은 시드 L2 일괄 대조(`--fixed-unix-seconds` 양쪽 동일). 남은 차이는 행 단위로 원인을 적는다 | [L2 일괄 도구](docs/dotnet-l2-batch-20261114.md) | **34/34 일치.** 일치하지 않는 행이 "Load/Start 뒤 프레임 진행으로만 생기는 값"임을 cpp f6 덤프로 입증한 경우에만 R6으로 넘긴다 |

**R4 체크 목록 — 대응 없는 cpppj 단위(A·B·C·D 영역 137단위 / 13,873줄).** MP·Editor 전용으로 확인되는 것은 R11·R7 목록으로 옮긴다.

기존 대응이 있던 큰 파일의 미완 부분(아래 누락 단위 수에는 포함하지 않음):

- [x] `o/ContentFormHost` 현재 정렬·필드·진단·가상 재부착 몸체 — 0.549. NativeStorage의 장기 소유자 연결은 R4-1 잔여.
- [x] `o/ContentFormLifetime` 현재 가상표·DWORD 커서·동적 owner/역할/비용·삭제/보상 몸체 — 0.550. 장기 실제 Client 장치·상위 분배 연결은 R4-1 잔여.
- [x] `o/SidPool` PATCH 현재 할당·진단·소진 회수와 Storage/World의 실제 Host 삭제 대여 — 0.551. 현재 개발 호스트의 NativeStorage 전환은 R4-1 잔여.
- [x] `o/RawSquidNeighbors` 현재 TYPE/코드 사본·값 원천/지연 코드·현재 flag8 지도·원본 raw 좌표 spot — 0.552.
- [x] `o/RawGraph` 공유 물리/현재 TYPE와 `RawGraphLive` 전체 실행 — 0.553. 실제 World 배열·진단·로그·시계 연결 포함. 실행기 자체의 NativeStorage 장기 보유와 R4-2 적재 결선은 잔여.
- [x] `o/RawSquidPostPopParent`·`SquidPostPopPatch`·`RawManaRegistration` 현재 공통 몸체 — 0.554. 외곽 가상 분배·NativeStorage 장기 보유는 잔여.
- [x] `o/SquidPostPop` 현재 이식된 파생/접두의 가상 분배·장기 자원 선택·외곽 호출 pin — 0.555. 미이식 파생 설치와 실제 Runtime/NativeStorage 대여는 잔여.
- [x] `client/RawSurfaceStorage` 일반/준비 물리 대여와 `RawSurfaceWorld` 준비 진입·생성 실패/일반 종료 반납 — 0.556. 장기 장부 본체/실행기·실제 Runtime pin은 잔여.
- [x] `client/RawSurfaceStorage` 공유 장부/삭제/로딩 상태·전역 주소·장기 Graph/이웃/Owner와 실제 World 재사용 — 0.557. 공장·표시·공간 등 장기 조립은 잔여.
- [x] `o/SquidDisplay` 현재 뷰와 `client/RawSurfaceStorage` 장기 표시·실제 기본/World 장치·호출 pin·재사용/반납 — 0.558. Graph의 종료 World 보유도 교정. 실제 Native Client/Runtime 전체 연결은 잔여.
- [x] `o/RawBuildingSpace` 현재 TYPE/전체 checking·원문 진단 복귀와 CD Notify 무효과, `client/RawSurfaceStorage` typed 장기 공장/건물/Unpop·기본 공통 종속/약한 현재 종단·World 재사용 — 0.559. Native 파생 내용물/전송·finder TYPE와 나머지 공간/호스트 조립은 잔여.
- [x] `client/RawSurfaceStorage` 명시 Native/typed 장치·장기 전송 writer·현재 Pop/외부 Process 대여/호출 pin·World 재사용/반납과 기본 공장 Form 전송 — 0.560. 실제 Client Registry/Native 파생 설치와 나머지 공간 조립은 잔여.
- [x] `client/RawSurfaceStorage` typed 장기 PostPop/Pop·프레임·연결/표면·World 재사용과 `o/RawAssetOwner` 현재 생산/중립 VT·생산 DWORD 분배 — 0.561. 최초 자산/비용 소유·Native 기본 장치/파생 설치와 실제 호스트 조립은 잔여.
- [x] `client/RawSurfaceStorage` 입력 TYPE/usage·프레임·셰이프/헤더의 값 사본과 명시 최초 Source 비용 연결·준비/일반 재대여 보존 — 0.562. 상위 GameAssets/실제 Client 자산뷰·Native 장치/파생 조립은 잔여.

**A 자료·파일·설정 — 30단위 / 2,572줄**

- [ ] `o/Base*` 1단위 595줄: BaseFile 595
- [ ] `o/RawBase*` 4단위 345줄: RawBaseFileIO 139, RawBaseFileReadPolicy 93, RawBaseFileOpenPolicy 71, RawBaseFileWholeRead 42
- [ ] `o/RawFile*` 1단위 149줄: RawFileName 149
- [ ] `client/Xlat*` 1단위 123줄: XlatFileRead 123
- [ ] `client/Gif*` 1단위 110줄: GifFileRead 110
- [ ] `o/RawXlat*` 1단위 101줄: RawXlat 101
- [ ] `client/RawBuild*` 1단위 100줄: RawBuildReplay 100
- [ ] `o/File*` 1단위 88줄: FileSpecQuery 88
- [ ] `client/Root*` 2단위 87줄: RootServerEndpoint 51, RootServerListDisk 36
- [ ] `client/Asset*` 1단위 86줄: AssetDeletionVtables 86
- [ ] `client/Data*` 1단위 78줄: DataManagerFileRead 78
- [ ] `client/Config*` 1단위 72줄: ConfigFileRead 72
- [ ] `client/RawTrace*` 1단위 71줄: RawTraceLog 71
- [ ] `o/Xlat*` 1단위 68줄: Xlat 68
- [ ] `client/RawIndexed*` 1단위 63줄: RawIndexedBitmap 63
- [ ] `client/Startup*` 1단위 56줄: StartupFiles 56
- [ ] `client/Indexed*` 1단위 52줄: IndexedBitmapFile 52
- [ ] `o/RawStatic*` 1단위 50줄: RawStaticStrings 50
- [ ] `client/RawIssue*` 1단위 49줄: RawIssueCommands 49
- [ ] `client/Read*` 1단위 40줄: ReadOnlyFileMap 40
- [ ] `client/Registered*` 1단위 38줄: RegisteredMessages 38
- [ ] `client/RawNs*` 1단위 33줄: RawNsInfoContext 33
- [ ] `o/Startup*` 1단위 31줄: StartupArchiveFiles 31
- [ ] `o/Win32*` 1단위 30줄: Win32FileError 30
- [ ] `client/Lost*` 1단위 29줄: LostPositionKey 29
- [ ] `o/RawConfig*` 1단위 28줄: RawConfigFileLoad 28

**B 월드 코어(SID·슬롯·Pop·해시) — 28단위 / 2,055줄**

- [ ] `o/RawSquid*` 8단위 524줄: RawSquidMutationReceive 106, RawSquidReceive 88, RawSquidName 73, RawSquidLevel 69, RawSquidCompressedReceive 67, RawSquidVerification 62, RawSquidDefense 46, RawSquidWord 13
- [x] `o/RawSquidPostPopParent` 1단위60줄 — 0.554(현재 조회·저장 genus·효과 순서, R4-2 적재 결선은 잔여)
- [ ] `o/RawObject*` 4단위 380줄: RawObjectTraceMain 130, RawObjectTraceFields 99, RawObjectTrace 82, RawObjectCommand 69
- [ ] `client/Object*` 2단위 178줄: ObjectDumpDisk 110, ObjectOwnerCommands 68
- [x] `o/Content*` 1단위 158줄: ContentNoticeVtables 158 — 0.546
- [ ] `client/RawObject*` 1단위 129줄: RawObjectDump 129
- [ ] `o/SquidName*` 1단위 86줄: SquidNameVtables 86
- [ ] `o/RawBuried*` 1단위 79줄: RawBuriedPostPop 79
- [ ] `client/Type*` 1단위 72줄: TypeFileRead 72
- [ ] `o/RawDestroy*` 1단위 69줄: RawDestroyAt 69
- [ ] `o/RawCreate*` 1단위 62줄: RawCreateAt 62
- [x] `o/RawContent*` 1단위 61줄: RawContentParentNotice 61 — 0.546
- [ ] `o/RawAllocation*` 1단위 52줄: RawAllocationFailure 52
- [x] `o/RawDepend*` 1단위 50줄: RawDependCreate 50 — 0.547(DependFormHost 현재 몸체·World 공장 연결 포함)
- [ ] `o/Pop*` 1단위 49줄: PopVtables 49
- [ ] `o/RawChecking*` 1단위 48줄: RawCheckingDiagnostics 48
- [ ] `client/Frame*` 1단위 41줄: FrameRateOption 41
- [x] `o/RawAssetOwner` 1단위40줄 — 0.561(R4-1 공간 조립의 선행 의존, 두 원본128행·World/Storage 생산 장부 결선)
- [x] `o/SquidPost*` 1단위28줄: SquidPostPopPatch 28 — 0.554(현재 공통 몸체, 외곽 가상 파생 분배는 잔여)
- [ ] `client/Current*` 1단위 17줄: CurrentAllocationBytes 17

**C Fort·지형·섬 적재 — 5단위 / 733줄**

- [ ] `o/RawRegion*` 3단위 521줄: RawRegionPaint 229, RawRegionOwnership 229, RawRegionOwner 63
- [x] `o/RawGraph*` 1단위 250줄: RawGraphLive 250 — 0.553(R4-1 선행으로 현재 몸체·공유 배열 이식, R4-2 적재 per-Pop 결선은 잔여)
- [ ] `o/Islandbuilder*` 1단위 115줄: Islandbuilder 115
- [ ] `client/Fort*` 1단위 97줄: FortCoreDataLoad 97

**D 세션·월드 조립·로그인·패킷·Client 전역 — 74단위 / 8,513줄**

- [ ] `client/GameWorld*` 1단위 1,157줄: GameWorld 1157
- [ ] `client/RawCampaign*` 5단위 1,120줄: RawCampaignSessionLogin 389, RawCampaignAssetBindings 282, RawCampaignWorldRuntime 197, RawCampaignOwnership 137, RawCampaignLifetimeBindings 115
- [ ] `client/RawSurface*` 11단위 578줄: RawSurfaceAttack 100, RawSurfaceAssetDeletion 95, RawSurfaceContent 77, RawSurfaceLifetime 55, RawSurfaceAssetNotice 52, RawSurfaceDeletion 46, RawSurfaceDepend 42, RawSurfacePlayerRegular 38, RawSurfaceRegistryDeletion 29, RawSurfaceDecoration 24, RawSurfaceFrameRegular 20
- [ ] `o/RawPacket*` 4단위 405줄: RawPacketDispatch 191, RawPacketDelay 108, RawPacketCopy 62, RawPacketFrame 44
- [ ] `o/RawAsset*` 1단위 404줄: RawAssetRegular 404
- [ ] `client/ClientState*` 4단위 384줄: ClientStateWorldGlobals 112, ClientStateWorldCleanup 102, ClientStateRuntimeRetirement 99, ClientStateStorageCleanup 71
- [ ] `client/ClientCampaign*` 1단위 378줄: ClientCampaignTransport 378
- [ ] `client/CampaignClient*` 1단위 373줄: CampaignClientServices 373
- [ ] `o/RawPlayer*` 4단위 321줄: RawPlayerContentNotice 115, RawPlayerRegular 85, RawPlayerNotification 72, RawPlayerLinksCleanup 49
- [ ] `o/RawConnection*` 2단위 292줄: RawConnectionRegistry 246, RawConnectionHealth 46
- [ ] `client/RawGame*` 1단위 273줄: RawGameMenus 273
- [ ] `client/ClientPlayer*` 2단위 226줄: ClientPlayerRecords 189, ClientPlayerRuntime 37
- [ ] `o/RawState*` 2단위 184줄: RawStateWorldCleanup 162, RawStateZoneReset 22
- [ ] `client/ClientLocal*` 1단위 180줄: ClientLocalStateGlobals 180
- [ ] `client/ClientDiagnostics*` 1단위 157줄: ClientDiagnostics 157
- [ ] `client/ClientPacket*` 2단위 137줄: ClientPacketSources 89, ClientPacketRecord 48
- [ ] `client/Saved*` 2단위 130줄: SavedStateFileRead 79, SavedStateFileWrite 51
- [ ] `client/CampaignTransport*` 1단위 129줄: CampaignTransportServices 129
- [ ] `o/RawClock*` 2단위 124줄: RawClockSynchronization 99, RawClockSyncReset 25
- [ ] `client/GameAssets*` 1단위 122줄: GameAssets 122
- [ ] `o/RawProcess*` 2단위 112줄: RawProcessSummary 77, RawProcessRemoval 35
- [ ] `client/ClientBuild*` 1단위 99줄: ClientBuildReplay 99
- [ ] `client/RawBattle*` 1단위 96줄: RawBattleLeave 96
- [ ] `o/RawRoot*` 1단위 90줄: RawRootLoginReply 90
- [ ] `o/RawDisc*` 1단위 89줄: RawDiscRegular 89
- [ ] `client/ClientNs*` 1단위 87줄: ClientNsInfoGlobals 87
- [ ] `client/Fort*` 1단위 80줄: FortSubscriberClient 80
- [ ] `client/ClientStatic*` 1단위 78줄: ClientStaticStrings 78
- [ ] `client/ClientConnection*` 2단위 77줄: ClientConnectionGlobals 45, ClientConnectionActivityGlobals 32
- [ ] `o/RawBuried*` 1단위 75줄: RawBuriedRegular 75
- [ ] `client/RawMonitor*` 1단위 74줄: RawMonitorTime 74
- [ ] `o/RawPeer*` 1단위 68줄: RawPeerWorldSync 68
- [ ] `client/System*` 1단위 54줄: SystemNewsTime 54
- [ ] `client/State*` 1단위 53줄: State 53
- [ ] `client/CampaignLogin*` 1단위 47줄: CampaignLoginServices 47
- [ ] `client/LocalServer*` 1단위 41줄: LocalServerProcess 41
- [x] `o/RawForm*` 1단위 40줄: RawFormLinked 40 — 0.546(실제 건설 소비자는 R6)
- [x] `o/RawBridgeFallParent` 1단위 46줄 — 0.548(R6에서 R4-1 의존 단계로 이동, World 낙하 예약 연결)
- [ ] `client/ClientRandom*` 1단위 36줄: ClientRandomGlobals 36
- [ ] `o/RawDeveloper*` 1단위 35줄: RawDeveloperCommandPacket 35
- [ ] `o/RawDecoration*` 1단위 33줄: RawDecorationRegular 33
- [ ] `client/ClientAllocation*` 1단위 31줄: ClientAllocationFailure 31
- [ ] `client/ClientSubscriber*` 1단위 26줄: ClientSubscriberGlobals 26
- [ ] `o/RawTransport*` 1단위 18줄: RawTransportPolling 18

#### R5 — 월드 표시·카메라·HUD → D2, `Netstorm.exe` 전환

| 조각 | 할 일 | 주 cpppj 근거 | 끝났다는 증거 |
| --- | --- | --- | --- |
| **R5-1** 렌더러 프레임·수집·정렬 | 프레임 본체, 객체·타일·미리보기 수집, 정렬·제출, dirty 사각형(9-1 E), 실자산 어댑터·`SpriteDraw` 클래스·그림자 alias 공유(R2 잔여), Fort 팔레트 전환 | `client/ClientRendererFrame`·`RawRenderer*`·`Renderer`(대응 있음)·`DrawSort`·`DrawSubmission`·`ObjectCollection`·`TileGroupCollection`·`Preview*`·`SquidRenderer` | 진입 직후 정지 화면의 지형·객체 층이 cpp `snapshot`과 같다 |
| **R5-2** 타입별 Draw·시각 시스템 | 건설·번개·파편·용해·불꼬리·연기 시각 시스템, 입자, 공장 표시, 구름(Kencloud는 메뉴에서 이식 완료), 번개 화면. 연소·공통 표시 등록은 R4에서 옮겼다(0.533~0.538) | `o/*VisualSystem`, `client/RawCampaignParticles`·`RawCampaignFactoryDisplay`·`ConstructionVisualRenderer`·`ThunderProcess` | N프레임 뒤 캡처 차이 0(효과가 있는 미션 포함) |
| **R5-3** 카메라·가장자리 스크롤 | 초기 카메라·홈·중심 요청·이동·가장자리 스크롤(전체화면에서 동작, AGENTS), 초기 구름 카메라 원점(2.6의 0.80) | `client/RawCamera*`·`CameraHome`·`ClientCameraGlobals`·`RawEdgeScroll` | 카메라 이동 입력열 뒤 캡처 차이 0 |
| **R5-4** HUD·덱·미니맵·인벤토리 | 게임 화면의 Uber 허브와 HUD 조립, 덱·생산 칸, 미니맵, SP 표시, NsInfo. R3 잔여 중 게임 화면 소유자(2.5)도 여기서 닫는다 | `client/UberGump`(2,683줄, 대응 없음)·`UberHud`·`HudDock`·`HudNs*`·`Inventory*`·`RawMinimapControls`·`RawButtonControls`·`FloatingSp*`·`UberRootGump`·`RawUberRootControls` | HUD가 있는 진입 화면 전체가 cpp와 같다 |
| **R5-5** D2 판정과 `Netstorm.exe` 전환 | 34경로 진입 직후·N프레임 뒤 L4a. 통과하면 `Netstorm.Client.Host` 어셈블리 이름을 `Netstorm`으로, `Netstorm.Game`을 `NetstormLegacy`로 바꾸고 `package-dotnetpj.ps1`의 배포 대상을 새 호스트로 넘긴다(결정 D2) | `l4a_compare.ps1` | 34경로 캡처 차이 0, 배포 ZIP 압축 해제 실행 확인 |

**R5 체크 목록 — 대응 없는 cpppj 단위(E·G 영역 102단위 / 13,764줄).**

**E 표시·렌더러·카메라·HUD·시각 효과 — 52단위 / 6,166줄**

- [ ] `client/Preview*` 2단위 577줄: PreviewDraw 363, PreviewCollection 214
- [ ] `client/Inventory*` 3단위 463줄: InventoryGump 205, InventoryItemGump 186, InventoryAboutMenu 72
- [ ] `client/ClientRenderer*` 2단위 420줄: ClientRendererFrame 394, ClientRendererFrameGlobals 26
- [ ] `client/HudDock*` 1단위 278줄: HudDock 278
- [ ] `client/HudNs*` 2단위 243줄: HudNsInfoCreate 168, HudNsInfo 75
- [ ] `client/Draw*` 2단위 242줄: DrawSubmission 143, DrawWater 99
- [ ] `o/Construction*` 1단위 237줄: ConstructionVisualSystem 237
- [ ] `client/RawCamera*` 3단위 204줄: RawCameraCommit 109, RawCameraPan 50, RawCameraPresent 45
- [ ] `client/RawRenderer*` 2단위 187줄: RawRendererPrelude 111, RawRendererClouds 76
- [ ] `client/RawMinimap*` 1단위 181줄: RawMinimapControls 181
- [ ] `o/Lightning*` 1단위 172줄: LightningVisualSystem 172
- [ ] `o/Shrapnel*` 1단위 158줄: ShrapnelVisualSystem 158
- [ ] `client/Object*` 1단위 157줄: ObjectCollection 157
- [ ] `o/Dissolve*` 1단위 157줄: DissolveVisualSystem 157
- [ ] `o/Fiery*` 1단위 157줄: FieryTailVisualSystem 157
- [ ] `client/UberHud*` 1단위 156줄: UberHud 156
- [ ] `client/Camera*` 1단위 151줄: CameraHome 151
- [ ] `o/Smoke*` 1단위 145줄: SmokeVisualSystem 145
- [ ] `client/Tile*` 1단위 136줄: TileGroupCollection 136
- [ ] `client/Owner*` 1단위 122줄: OwnerColorTableState 122
- [ ] `o/RawMinimap*` 1단위 121줄: RawMinimapRaster 121
- [ ] `o/RawColor*` 1단위 118줄: RawColorMapCache 118
- [ ] `client/Gif*` 1단위 114줄: GifScreenshot 114
- [ ] `o/RawContent*` 1단위 113줄: RawContentPreviewProcess 113
- [ ] `client/Renderer*` 1단위 108줄: RendererDiagnostics 108
- [ ] `o/Evil*` 1단위 101줄: EvilRayVisualSystem 101
- [ ] `client/Minimap*` 1단위 98줄: MinimapGeometry 98
- [ ] `client/ClientCamera*` 1단위 93줄: ClientCameraGlobals 93
- [ ] `client/Palette*` 1단위 89줄: PaletteFileRead 89
- [ ] `client/UberGump*` 1단위 83줄: UberGumpCamera 83
- [ ] `client/NativeShape*` 1단위 72줄: NativeShapeFile 72
- [ ] `client/Saved*` 1단위 66줄: SavedCameraKey 66
- [ ] `client/Screen*` 2단위 64줄: ScreenModeTransition 43, ScreenMouseKey 21
- [ ] `client/RawEdge*` 1단위 63줄: RawEdgeScroll 63
- [ ] `client/Construction*` 1단위 57줄: ConstructionVisualRenderer 57
- [ ] `client/ClientColor*` 1단위 55줄: ClientColorMapFile 55
- [ ] `o/RawAuxiliary*` 1단위 46줄: RawAuxiliaryDraw 46
- [ ] `client/VFX*` 1단위 40줄: VFXEllipse 40
- [ ] `client/HudPlayer*` 1단위 36줄: HudPlayerButtonDraw 36
- [ ] `o/RawPlain*` 1단위 33줄: RawPlainAssetDraw 33
- [ ] `client/GameTime*` 1단위 29줄: GameTimeDisplay 29
- [ ] `o/RawSprite*` 1단위 24줄: RawSpriteRegular 24

**G UI 프레임워크·메뉴(Gump·HTML·Tell) — 50단위 / 7,598줄**

- [ ] `client/UberGump*` 1단위 2,683줄: UberGump 2683
- [ ] `client/RawButton*` 1단위 811줄: RawButtonControls 811
- [ ] `client/RawObject*` 5단위 619줄: RawObjectMenu 179, RawObjectMenuInfo 161, RawObjectMenuVariants 131, RawObjectMenuOpen 122, RawObjectMenuSlots 26
- [ ] `client/GameMenu*` 2단위 331줄: GameMenuCommands 190, GameMenuActions 141
- [ ] `client/CampaignObject*` 1단위 229줄: CampaignObjectMenuClient 229
- [ ] `client/RawPlayer*` 1단위 223줄: RawPlayerMenu 223
- [ ] `client/UberRoot*` 1단위 187줄: UberRootGump 187
- [ ] `client/ClientOptions*` 2단위 179줄: ClientOptionsDisk 165, ClientOptionsSaveIO 14
- [ ] `client/RawUber*` 1단위 161줄: RawUberRootControls 161
- [ ] `client/Tell*` 3단위 157줄: TellSection 78, TellParameters 41, TellTip 38
- [ ] `client/QuickHelp*` 2단위 156줄: QuickHelpList 87, QuickHelpFileRead 69
- [ ] `client/Text*` 1단위 146줄: TextClipboard 146
- [ ] `client/Help*` 2단위 144줄: HelpLinkPolicy 73, HelpListDisk 71
- [ ] `client/Font*` 2단위 134줄: FontCacheFile 84, FontFaceEncoding 50
- [ ] `client/RawGame*` 1단위 123줄: RawGameMenuBar 123
- [ ] `client/NativeFont*` 1단위 110줄: NativeFontCacheFile 110
- [ ] `client/Technical*` 2단위 94줄: TechnicalHelp 62, TechnicalHelpClient 32
- [ ] `client/NativeMain*` 1단위 90줄: NativeMainMenuState 90
- [ ] `client/Object*` 1단위 88줄: ObjectMenuSlots 88
- [ ] `client/Gump*` 1단위 86줄: Gump 86
- [ ] `client/ClientTitle*` 1단위 85줄: ClientTitleBitmaps 85
- [ ] `client/RawMenu*` 2단위 84줄: RawMenuMultiplayer 73, RawMenuLoading 11
- [ ] `client/GumpRedraw*` 1단위 79줄: GumpRedraw 79
- [ ] `client/Title*` 1단위 78줄: TitleBitmapLifetime 78
- [ ] `client/GumpPointer*` 1단위 77줄: GumpPointerBoundary 77
- [ ] `client/NativeIntro*` 1단위 64줄: NativeIntroState 64
- [ ] `client/ClientHelp*` 1단위 63줄: ClientHelpWord 63
- [ ] `client/External*` 1단위 47줄: ExternalHelpClient 47
- [ ] `client/Window*` 1단위 46줄: WindowTitle 46
- [ ] `client/Main*` 1단위 44줄: MainMenuActivity 44
- [ ] `o/RawRemote*` 1단위 43줄: RawRemoteTellPacket 43
- [ ] `client/RawHtml*` 1단위 32줄: RawHtmlWindowContext 32
- [ ] `client/RawSurface*` 1단위 31줄: RawSurfaceGump 31
- [ ] `client/Intro*` 1단위 29줄: IntroMenu 29
- [ ] `client/Multiplayer*` 1단위 24줄: MultiplayerOptionsWindow 24
- [ ] `client/RawText*` 1단위 13줄: RawTextEntryContext 13
- [ ] `client/ClientHtml*` 1단위 8줄: ClientHtmlWindowGlobals 8

#### R6 — 플레이 입력·규칙·AI·소리 → M2·M3

가장 큰 단계다(382단위 / 45,090줄). 조각마다 "o/ 규칙 부품 → client 바인딩 → 호스트 결선 → 고정 입력열 L3 해시" 순서로 닫는다. Tutorial1 → Early Missions → 캠페인 1-1·1-2 순으로 필요한 것부터 옮기고(수직 진행), 공격형 공중 3종은 코드만 옮긴다([3.2](#dotnet-constraints)).

| 조각 | 할 일 | 영역 | 끝났다는 증거 |
| --- | --- | --- | --- |
| **R6-1** 입력·커서·선택·게임 메뉴 | 사건 큐 전체 처리(9-1 B), 선택·강조·후보, 커서 상태·범위·배치, QWASZX·ESC 가로/하위 메뉴·Player List·F9, 객체 우클릭 메뉴 전체 | F 50단위 / 4,483줄 | 선택·메뉴 입력열의 L3 해시와 캡처 일치 |
| **R6-2** 사제·경로·운반·수확 | 경로 탐색·이동 프로세스, 운반체 선택·도착·투하, 간헐천·덩어리 채집, 사제 명령·기도·공황(사제 Pop·회복·보호막 부품은 R4에서 선행) | H 59단위 / 7,270줄 | Tutorial1의 이동·수확 구간 L3 일치 |
| **R6-3** 건설·생산·경제·다리 | 배치·건설 프로세스·빌더, 생산 덱·공장 연구/업그레이드, 에너지·마나 공급, 다리 배치·붕괴, 기술·지식 | I 72단위 / 9,034줄 | 건설·생산 입력열 L3 일치(1-1 초반) |
| **R6-4** 전투·비행·투사체·방어 | 총·표적·애니메이션, 폭탄·주문, 울타리, 비행체·새장, 투사체·미사일, 피격·파괴 효과(피해 전처리·피격·연소 부품은 R4에서 선행) | J 97단위 / 12,026줄 | cpppj `--test-autoplay` 고정 시드 교전 구간의 L3 일치 |
| **R6-5** 신전·제단·의식·승패 | 의식·희생·포획 규칙, 소용돌이, 미션 결과 판정(R4-3에서 당긴 적재 경로 제외). 실제 포획/희생·승리 재현 실행은 사용자 확인 | L 35단위 / 3,614줄 | 부품 fixture와 결과 판정 검사 통과 |
| **R6-6** AI 전체 | 전략 설정·준비·다리 탐색·건설 선택·경제·배치·운반 행동(R4-5에서 당긴 생성 경로 제외), 11차 난이도 배율이 보통에서 원본과 같은지 | K 38단위 / 4,243줄 | AI가 있는 미션의 N프레임 L3 일치 |
| **R6-7** 소리·음악·동영상 | 위치 음향·음악 선택·장면 음악·옵션 변경 중 재생 관리, 인트로 동영상. 장치 계층만 MonoGame(실장치 검사는 0.6) | N 13단위 / 2,231줄 | L4b: 소리 시점·곡 선택이 cpp MCP 녹화와 같다 |
| **R6-8** 채팅·메시지·경고 | 채팅 창·런타임·점 명령, MessageGump·Warning·MessageBoard(채팅 형식·패킷 부품은 0.334~0.339에서 선행) | O 19단위 / 2,240줄 | 채팅·경고 입력열 캡처 일치 |

- **M2:** 고정 입력열(Tutorial1·1-1 초반·자동 플레이 교전)의 L3 상태 해시와 대표 장면 캡처가 cpp와 같다.
- **M3:** 1-1·1-2가 새 경로에서 기존 경로와 같은 수준 이상으로 진행된다 → `NetstormLegacy`와 그 근사 검사를 삭제한다(결정 D2·5).

**R6 체크 목록 — 대응 없는 cpppj 단위(F·H·I·J·K·L·N·O 영역).**

**F 입력·커서·선택·게임 메뉴 — 50단위 / 4,483줄**

- [ ] `client/RawCursor*` 9단위 1,703줄: RawCursorMana 290, RawCursorRange 283, RawCursorLifecycle 235, RawCursorPlacement 228, RawCursorDrop 199, RawCursorPreviews 183, RawCursorReturnPiece 111, RawCursorPosition 99, RawCursorActions 75
- [ ] `client/RawCampaign*` 3단위 487줄: RawCampaignCursor 284, RawCampaignCursorDisplay 161, RawCampaignCarrierInput 42
- [ ] `client/RawUser*` 3단위 368줄: RawUserSelection 221, RawUserPick 81, RawUserHighlight 66
- [ ] `client/RawCarrier*` 1단위 175줄: RawCarrierInput 175
- [ ] `client/User*` 1단위 122줄: UserInput 122
- [ ] `client/ClientUser*` 2단위 120줄: ClientUserSelectionGlobals 77, ClientUserSelectionCleanup 43
- [ ] `client/ClientColor*` 1단위 114줄: ClientColorSortInput 114
- [ ] `client/HudDivider*` 1단위 100줄: HudDividerInput 100
- [ ] `client/Send*` 1단위 97줄: SendMoneyInput 97
- [ ] `o/Selection*` 1단위 86줄: SelectionVtables 86
- [ ] `client/Configuration*` 1단위 78줄: ConfigurationCommandInput 78
- [ ] `client/CampaignCursor*` 1단위 74줄: CampaignCursorClientEffects 74
- [ ] `o/RawSquid*` 1단위 72줄: RawSquidSelection 72
- [ ] `o/RawFactory*` 1단위 71줄: RawFactoryInput 71
- [ ] `client/Presentation*` 1단위 63줄: PresentationCommandInput 63
- [ ] `client/Utility*` 1단위 59줄: UtilityCommandInput 59
- [ ] `client/View*` 1단위 58줄: ViewKeyInput 58
- [ ] `client/ClientCursor*` 2단위 52줄: ClientCursorStateCleanup 27, ClientCursorGlobals 25
- [ ] `client/Cursor*` 2단위 49줄: CursorObjectPickup 29, CursorPreviewSource 20
- [ ] `client/Vortex*` 1단위 47줄: VortexCommandInput 47
- [ ] `client/Ritual*` 1단위 43줄: RitualChoiceInput 43
- [ ] `client/Selection*` 1단위 42줄: SelectionKeyInput 42
- [ ] `client/GameWorld*` 1단위 41줄: GameWorldInput 41
- [ ] `client/Priest*` 1단위 38줄: PriestSelectionShortcut 38
- [ ] `client/Next*` 1단위 37줄: NextUnitSelection 37
- [ ] `client/CampaignUser*` 1단위 36줄: CampaignUserInputServices 36
- [ ] `client/HtmlInput*` 1단위 36줄: HtmlInputDialog 36
- [ ] `o/RawConstruction*` 1단위 34줄: RawConstructionSelection 34
- [ ] `client/Rotation*` 1단위 30줄: RotationKeyInput 30
- [ ] `client/Capture*` 1단위 27줄: CaptureInput 27
- [ ] `client/Detonate*` 1단위 26줄: DetonateInput 26
- [ ] `client/Url*` 1단위 26줄: UrlInput 26
- [ ] `client/Pause*` 1단위 25줄: PauseKeyInput 25
- [ ] `client/Create*` 1단위 24줄: CreateAtInput 24
- [ ] `client/Destroy*` 1단위 23줄: DestroyAtInput 23

**H 사제·운반·수확·경로 — 59단위 / 7,270줄**

- [ ] `o/RawCarrier*` 16단위 1,971줄: RawCarrierArrival 228, RawCarrierChoice 187, RawCarrierDrop 175, RawCarrierRegular 142, RawCarrierSteal 138, RawCarrierCandidateSort 130, RawCarrierTransfer 129, RawCarrierPickup 119, RawCarrierContentNotice 119, RawCarrierEscape 118, RawCarrierMove 116, RawCarrierFalling 109, RawCarrierTarget 79, RawCarrierFallStart 76, RawCarrierTask 62, RawCarrierMining 44
- [ ] `o/RawPath*` 8단위 1,513줄: RawPathProcess 362, RawPathRuntime 306, RawPathData 259, RawPathStart 197, RawPathCleanup 181, RawPathArrivalParent 95, RawPathRetarget 66, RawPathHaltParent 47
- [ ] `o/RawDude*` 6단위 831줄: RawDudeNotice 188, RawDudeSpawn 170, RawDudeRegular 166, RawDudeBehavior 124, RawDudeTask 96, RawDudeDrop 87
- [ ] `o/RawPriest*` 4단위 432줄: RawPriestParticipation 132, RawPriestHalt 122, RawPriestArrival 108, RawPriestTakenNotice 70
- [ ] `client/RawCampaign*` 4단위 370줄: RawCampaignPathBindings 281, RawCampaignCarrierDrop 53, RawCampaignPrayer 24, RawCampaignPanic 12
- [ ] `client/RawCarrier*` 2단위 315줄: RawCarrierMenu 177, RawCarrierDropPoint 138
- [ ] `o/RawLocal*` 1단위 311줄: RawLocalGeyser 311
- [ ] `o/RawWorld*` 2단위 269줄: RawWorldGeyserBounds 150, RawWorldGeyser 119
- [ ] `o/RawGeyser*` 2단위 226줄: RawGeyserMined 142, RawGeyserLifetime 84
- [ ] `o/RawNugget*` 3단위 208줄: RawNuggetRegular 100, RawNuggetReplenish 65, RawNuggetPostPop 43
- [ ] `client/RawSurface*` 4단위 191줄: RawSurfaceCarrier 83, RawSurfaceDudeRegular 62, RawSurfacePriestDeletion 24, RawSurfacePriest 22
- [ ] `o/RawPlayer*` 1단위 178줄: RawPlayerGeyserBounds 178
- [ ] `o/RawBulf*` 1단위 113줄: RawBulfFrame 113
- [ ] `o/RawSalvage*` 1단위 94줄: RawSalvageCommand 94
- [ ] `o/RawGem*` 1단위 90줄: RawGemReward 90
- [ ] `o/RawWalker*` 1단위 85줄: RawWalkerFrame 85
- [ ] `client/Priest*` 1단위 39줄: PriestPlacementAssets 39
- [ ] `client/ClientConstruction*` 1단위 34줄: ClientConstructionGem 34

**I 건설·생산·경제·다리 — 72단위 / 9,034줄**

- [ ] `o/RawConstruction*` 16단위 3,056줄: RawConstructionPlace 449, RawConstructionProcess 386, RawConstructionBuilder 270, RawConstructionNotice 253, RawConstructionEffects 240, RawConstructionFrame 229, RawConstructionRequest 199, RawConstructionConfirm 198, RawConstructionClear 169, RawConstructionLocal 168, RawConstructionData 147, RawConstructionArrival 109, RawConstructionCommand 101, RawConstructionPacket 91, RawConstructionFailurePackets 33, RawConstructionHistory 14
- [ ] `client/RawCampaign*` 3단위 889줄: RawCampaignConstructionBindings 401, RawCampaignFactoryDisplay 335, RawCampaignProductionBindings 153
- [ ] `o/RawFactory*` 6단위 666줄: RawFactoryPostPop 203, RawFactoryResearch 154, RawFactoryUpgrade 127, RawFactoryUpgradeCommand 84, RawFactoryDraw 57, RawFactoryRegular 41
- [ ] `client/RawProduction*` 2단위 566줄: RawProductionControls 393, RawProductionMaintenance 173
- [ ] `client/ClientConstruction*` 5단위 523줄: ClientConstructionGlobals 253, ClientConstructionRegions 123, ClientConstructionRandomPlacement 57, ClientConstructionPlacement 54, ClientConstructionDevices 36
- [ ] `o/RawCanon*` 3단위 368줄: RawCanonPlacementRelations 148, RawCanonPlacementPipeline 117, RawCanonPlacementPermission 103
- [ ] `o/RawProduction*` 2단위 368줄: RawProductionDeck 260, RawProductionSync 108
- [ ] `o/RawBridge*` 3단위 274줄: RawBridgeDecay 138, RawBridgeCarriers 99, RawBridgeDraw 37 (RawBridgeFallParent는 R4-1에서 0.548 완료)
- [ ] `o/RawMana*` 3단위 206줄: RawManaBoltRegular 89, RawManaBoltArrival 73, RawManaDraw 44
- [x] `o/RawManaRegistration` 1단위51줄 — 0.554(R4-1 선행, 양 판본 CPU1,084행 대조)
- [ ] `o/RawEnergy*` 1단위 206줄: RawEnergySupply 206
- [ ] `o/RawPlayer*` 1단위 199줄: RawPlayerPlacementAnchor 199
- [ ] `client/RawSurface*` 4단위 177줄: RawSurfaceMana 54, RawSurfaceFactoryRegular 53, RawSurfaceResidenceRegular 37, RawSurfaceReward 33
- [ ] `client/RawFactory*` 1단위 168줄: RawFactoryMenu 168
- [ ] `client/Production*` 3단위 149줄: ProductionHintGump 85, ProductionShortcuts 46, ProductionControlContext 18
- [ ] `client/RawKnowledge*` 1단위 121줄: RawKnowledgeMenu 121
- [ ] `client/Technology*` 1단위 110줄: TechnologyListMenu 110
- [ ] `client/Knowledge*` 1단위 105줄: KnowledgeSort 105
- [ ] `o/RawDeck*` 1단위 102줄: RawDeckDraw 102
- [ ] `client/Floating*` 1단위 87줄: FloatingSpGump 87
- [ ] `o/Energy*` 1단위 86줄: EnergyProviderVtables 86
- [ ] `o/RawResidence*` 1단위 76줄: RawResidenceRegular 76
- [ ] `o/RawTechnology*` 1단위 60줄: RawTechnologyTypesPacket 60
- [ ] `o/RawNew*` 1단위 50줄: RawNewTechnology 50
- [ ] `o/RawTheme*` 1단위 49줄: RawThemeTechnology 49
- [ ] `client/Theme*` 1단위 48줄: ThemeTechnologyCommand 48
- [ ] `client/ClientProduction*` 1단위 46줄: ClientProductionSync 46
- [ ] `client/ClientBridge*` 1단위 42줄: ClientBridgeDecay 42
- [ ] `client/RawStorm*` 1단위 42줄: RawStormPowerRefresh 42
- [ ] `client/Construction*` 1단위 41줄: ConstructionHistoryKey 41
- [ ] `o/RawSp*` 1단위 33줄: RawSpStoreNoticeBindings 33
- [ ] `o/RawState*` 1단위 24줄: RawStateBridgeReset 24

**J 전투·비행·투사체·방어 — 97단위 / 12,026줄**

- [ ] `o/RawGun*` 9단위 1,785줄: RawGunProcess 442, RawGunTarget 399, RawGunAnimation 349, RawGunBlocker 157, RawGunImpact 140, RawGunHit 122, RawGunGroundTarget 93, RawGunMode 50, RawGunAngle 33
- [ ] `o/RawBomb*` 13단위 1,398줄: RawBombStrike 157, RawBombEffect 153, RawBombTargets 141, RawBombRegular 126, RawBombBlast 126, RawBombTreason 119, RawBombHeal 114, RawBombEffectTimer 114, RawBombTimerPacket 88, RawBombStatus 82, RawBombHardener 62, RawBombActivation 59, RawBombPostPop 57
- [ ] `o/RawFence*` 10단위 1,078줄: RawFenceTarget 225, RawFenceNeighbors 144, RawFenceCreate 138, RawFenceRegular 128, RawFencePostPop 100, RawFenceLine 87, RawFenceLifetime 84, RawFenceMarkerClear 76, RawFenceRebuild 62, RawFenceTypeParent 34
- [ ] `client/RawCampaign*` 6단위 825줄: RawCampaignHitBindings 223, RawCampaignArcherBindings 223, RawCampaignDevastation 163, RawCampaignMotionBindings 137, RawCampaignGunQueries 59, RawCampaignMeltdown 20
- [ ] `o/RawFlyer*` 3단위 728줄: RawFlyerProcess 279, RawFlyerFrame 257, RawFlyerTarget 192
- [ ] `o/RawWind*` 5단위 517줄: RawWindAviaryProcess 168, RawWindFlyer 130, RawWindAviaryState 92, RawWindSpell 70, RawWindCircle 57
- [ ] `o/RawLightning*` 3단위 482줄: RawLightningPath 173, RawLightningFrame 169, RawLightningLifetime 140
- [ ] `o/RawMissile*` 1단위 402줄: RawMissile 402
- [ ] `o/RawSun*` 2단위 402줄: RawSunAviaryProcess 220, RawSunFlyerFrame 182
- [ ] `client/RawSurface*` 6단위 398줄: RawSurfaceBomb 112, RawSurfaceDamageable 97, RawSurfaceFence 85, RawSurfaceIce 41, RawSurfaceFlare 32, RawSurfaceBalloon 31
- [ ] `o/RawIce*` 3단위 362줄: RawIceTower 132, RawIceRicochet 132, RawIceCannonFire 98
- [ ] `o/RawFiery*` 2단위 302줄: RawFieryTailFrame 213, RawFieryTailLifetime 89
- [ ] `o/RawArcher*` 2단위 300줄: RawArcherAnimation 268, RawArcherTypeData 32
- [ ] `o/RawShrapnel*` 2단위 274줄: RawShrapnelFrame 192, RawShrapnelLifetime 82
- [ ] `o/RawEvil*` 2단위 224줄: RawEvilRayFrame 118, RawEvilRayLifetime 106
- [ ] `o/RawSmoke*` 1단위 217줄: RawSmokeEffect 217
- [ ] `o/RawDissolve*` 2단위 214줄: RawDissolveFrame 145, RawDissolveLifetime 69
- [ ] `o/RawCannon*` 1단위 193줄: RawCannonFire 193
- [ ] `o/RawBalloon*` 2단위 192줄: RawBalloonBody 123, RawBalloonFrame 69
- [ ] `o/RawRain*` 1단위 187줄: RawRainAviaryProcess 187
- [ ] `o/CampaignDamage*` 1단위 174줄: CampaignDamageVtables 174
- [ ] `o/RawProjectile*` 1단위 174줄: RawProjectileGeometry 174
- [ ] `o/RawFlare*` 1단위 164줄: RawFlare 164
- [ ] `o/RawCrossbow*` 1단위 151줄: RawCrossbowAnimation 151
- [ ] `o/RawMeltdown*` 1단위 137줄: RawMeltdown 137
- [ ] `client/Object*` 1단위 86줄: ObjectHitVtables 86
- [ ] `o/RawAcid*` 1단위 76줄: RawAcidZap 76
- [ ] `o/RawMano*` 1단위 76줄: RawManoSpell 76
- [ ] `o/RawSpecial*` 1단위 76줄: RawSpecialDamage 76
- [ ] `o/RawBoard*` 1단위 70줄: RawBoardApproach 70
- [ ] `client/RawObject*` 1단위 69줄: RawObjectHit 69
- [ ] `client/Sprite*` 1단위 48줄: SpriteHitPointBar 48
- [ ] `client/RawBomb*` 1단위 45줄: RawBombBlastBindings 45
- [ ] `o/RawPlayer*` 1단위 45줄: RawPlayerDamageNotice 45
- [ ] `client/Thunder*` 1단위 32줄: ThunderProcess 32
- [ ] `o/RawBulwark*` 1단위 30줄: RawBulwarkHit 30
- [ ] `client/Special*` 1단위 28줄: SpecialPointerTrace 28
- [ ] `o/RawAviary*` 1단위 25줄: RawAviaryProcessData 25
- [ ] `client/Current*` 1단위 22줄: CurrentTypeHitPoints 22
- [ ] `client/Spell*` 1단위 11줄: SpellFrameVisibility 11
- [ ] `client/CampaignDamageable*` 1단위 7줄: CampaignDamageableRegularTables 7

**K AI — 38단위 / 4,243줄**

- [ ] `o/RawAi*` 31단위 3,830줄: RawAiStrategySetup 197, RawAiPreparation 170, RawAiBridgePath 168, RawAiCarrierActions 164, RawAiBridgeStrategies 163, RawAiObjectNotice 162, RawAiPriestActions 153, RawAiFactoryProduction 148, RawAiBridgeSearch 144, RawAiDropActions 143, RawAiCollectors 143, RawAiBridgeCandidates 139, RawAiDamage 132, RawAiDrySpots 121, RawAiDeployment 119, RawAiWorldRegistration 118, RawAiFakeDrySpots 113, RawAiResourceChoice 112, RawAiDropScan 110, RawAiBuildSearch 108, RawAiDropTargets 104, RawAiDropEvaluation 104, RawAiMovement 101, RawAiWallet 100, RawAiDropSelection 100, RawAiDryGeometry 96, RawAiBuildChoice 94, RawAiRuntime 85, RawAiEconomy 82, RawAiBridgePlanning 81, RawAiCommandState 56
- [ ] `o/RawMake*` 1단위 104줄: RawMakeAi 104
- [ ] `client/Ai*` 2단위 97줄: AiDebugCommandInput 58, AiUtilityCommandInput 39
- [ ] `client/RawAi*` 1단위 75줄: RawAiDebugMenu 75
- [ ] `o/RawGeyser*` 1단위 70줄: RawGeyserAiRegistration 70
- [ ] `client/ClientAi*` 1단위 34줄: ClientAiDebugGlobals 34
- [ ] `client/Make*` 1단위 33줄: MakeAiCommandInput 33

**L 신전·제단·의식·승패 — 35단위 / 3,614줄**

- [ ] `o/RawDais*` 6단위 938줄: RawDaisRegular 380, RawDaisLifecycle 191, RawDaisDeletion 131, RawDaisProcessData 107, RawDaisContentNotice 93, RawDaisDraw 36
- [ ] `o/RawVortex*` 4단위 533줄: RawVortexAscend 181, RawVortexDeparture 165, RawVortexOwner 136, RawVortexOwnerDiagnostic 51
- [ ] `o/RawTemple*` 2단위 318줄: RawTempleRitual 277, RawTempleDraw 41
- [ ] `o/RawAltar*` 2단위 272줄: RawAltarSacrifice 237, RawAltarReleaseParent 35
- [ ] `o/RawMission*` 1단위 248줄: RawMissionOutcome 248
- [ ] `client/RawCampaign*` 2단위 236줄: RawCampaignRitual 161, RawCampaignVortex 75
- [ ] `o/RawPriest*` 3단위 173줄: RawPriestRitualActions 77, RawPriestRitualFrame 50, RawPriestSacrificeRequest 46
- [ ] `o/RawRitual*` 3단위 150줄: RawRitualRegular 92, RawRitualRefresh 33, RawRitualChoice 25
- [ ] `o/RawRune*` 1단위 148줄: RawRunePostPop 148
- [ ] `client/RawSurface*` 3단위 142줄: RawSurfaceRitual 63, RawSurfaceDaisDeletion 44, RawSurfaceVortex 35
- [ ] `client/ClientCapture*` 1단위 103줄: ClientCaptureSequence 103
- [ ] `client/RawCapture*` 1단위 75줄: RawCaptureSequence 75
- [ ] `o/RawForeign*` 1단위 55줄: RawForeignRitualReward 55
- [ ] `o/RawRain*` 1단위 54줄: RawRainVortexDraw 54
- [ ] `o/RawSun*` 1단위 53줄: RawSunAviaryCapture 53
- [ ] `o/RawThunder*` 1단위 49줄: RawThunderCannonCapture 49
- [ ] `o/RawGeyser*` 1단위 39줄: RawGeyserCapture 39
- [ ] `o/RawPlain*` 1단위 28줄: RawPlainWorkshopCapture 28

**N 소리·음악·동영상 — 13단위 / 2,231줄**

- [ ] `client/SoundMusic*` 4단위 891줄: SoundMusic 455, SoundMusicRuntime 193, SoundMusicFile 148, SoundMusicSelection 95
- [ ] `client/ClientAudio*` 1단위 467줄: ClientAudio 467
- [ ] `client/SoundScene*` 1단위 295줄: SoundSceneMusic 295
- [ ] `client/Movie*` 4단위 273줄: MoviePcmOutput 83, MovieLifecycle 80, MoviePlayback 76, MovieTimer 34
- [ ] `client/RawMovie*` 1단위 142줄: RawMovieControls 142
- [ ] `client/Smacker*` 1단위 119줄: SmackerDecoder 119
- [ ] `o/RawAmbient*` 1단위 44줄: RawAmbientSound 44

**O 채팅·메시지 — 19단위 / 2,240줄**

- [ ] `client/ClientChat*` 4단위 436줄: ClientChatRuntime 243, ClientChatManagerGlobals 123, ClientChatPacketReceiver 42, ClientChatWindowGlobals 28
- [ ] `client/Message*` 2단위 360줄: MessageGump 266, MessageBoard 94
- [ ] `client/Chat*` 2단위 340줄: ChatWindow 272, ChatRecipients 68
- [ ] `client/RawChat*` 3단위 251줄: RawChatSubmit 142, RawChatRelay 86, RawChatWindowContext 23
- [ ] `client/RawDot*` 2단위 201줄: RawDotCommandDispatch 125, RawDotCommandRegister 76
- [ ] `client/Warning*` 1단위 184줄: WarningMessages 184
- [ ] `o/RawChat*` 1단위 150줄: RawChatManagerPrefix 150
- [ ] `client/HudChat*` 1단위 135줄: HudChatGeometry 135
- [ ] `client/MissionLoading*` 1단위 79줄: MissionLoadingMessage 79
- [ ] `client/ClientDot*` 1단위 56줄: ClientDotCommandGlobals 56
- [ ] `client/RawMenu*` 1단위 48줄: RawMenuChat 48

#### R7 — 미션 전체·Edit·설정·배포 → M4

| 조각 | 할 일 | 끝났다는 증거 |
| --- | --- | --- |
| **R7-1** 미션 수명·안내·대본 | 튜토리얼 게이트·대본 명령·Dialog, Retry/Next/Abort·메뉴 복귀, 진행 저장과 다음 미션 해제, 데모 360초 반복·Auto-Demo | 34경로 진입·진행·저장 수명 검사, 데모 반복 2주기 |
| **R7-2** Edit 전체 | 편집 메뉴·섬 추가·저장·Save as·Test Battle·SaveGoMain, 언어 파일, Edit 이동행 메뉴 장면(R3에서 미룸). 대응 있는 35단위의 세션 연결과 대응 없는 26단위 | 기존 맵 왕복 저장 일치, Test Battle 진입·복귀 |
| **R7-3** 설정·9-1 판정 | `options.cfg`를 원본 시점·형식으로 읽고 쓰기, 전체화면 전환 뒤 재실행 오류 없음(AGENTS), 9-1 C·D·F 판정([4절](#original-parity-plan)) | 판정 근거 기록, Windows 10 확인 |
| **R7-4** 배포 | `package-dotnetpj.ps1` 갱신(인자 없으면 저장 위치 입력, 빈 값은 취소), 압축 해제 실행, exe 옆 `fonts/`·라이선스, `Netstorm.exe` 외 실행 파일 사용법 문서 | ZIP 전수 대조와 압축 해제 실행 확인 |

**R7 체크 목록 — 대응 없는 cpppj 단위(M·Y 영역 45단위 / 5,657줄).**

**M 미션 수명·안내·대본·튜토리얼 — 19단위 / 2,941줄**

- [ ] `o/RawMission*` 6단위 1,297줄: RawMissionClientPackets 333, RawMissionLogin 302, RawMissionLoginTimer 277, RawMissionFrameGate 138, RawMissionServerPackets 137, RawMissionSettingsPacket 110
- [ ] `o/RawTutorial*` 4단위 460줄: RawTutorialLater 191, RawTutorialTwo 127, RawTutorialOne 97, RawTutorialMissionRelease 45
- [ ] `client/RawCampaign*` 2단위 309줄: RawCampaignMissionStartBindings 173, RawCampaignTutorialOneBindings 136
- [ ] `client/Composite*` 1단위 297줄: CompositeMissionTransport 297
- [ ] `client/Dialog*` 2단위 224줄: DialogBodyLayout 188, DialogCommandLifetime 36
- [ ] `o/RawLocal*` 1단위 222줄: RawLocalMissionState 222
- [ ] `client/CampaignGuide*` 1단위 82줄: CampaignGuideRelease 82
- [ ] `client/Move*` 1단위 33줄: MoveDialog 33
- [ ] `client/MissionNetwork*` 1단위 17줄: MissionNetwork 17

**Y Edit·시나리오 편집 — 26단위 / 2,716줄**

- [ ] `client/EditorScenario*` 3단위 469줄: EditorScenarioModel 241, EditorScenarioFiles 127, EditorScenarioController 101
- [ ] `client/Scenario*` 3단위 321줄: ScenarioDocument 209, ScenarioEffectInput 58, ScenarioObjectInput 54
- [ ] `client/UberEditor*` 1단위 314줄: UberEditor 314
- [ ] `client/EditorIsland*` 5단위 288줄: EditorIslandLayout 108, EditorIslandDrag 96, EditorIslandBitmap 33, EditorIslandWindow 29, EditorIslandDialog 22
- [ ] `client/EditorLocal*` 1단위 261줄: EditorLocalTransport 261
- [ ] `o/RawEditor*` 2단위 201줄: RawEditorWorldFinish 124, RawEditorPlayers 77
- [ ] `client/RawCampaign*` 2단위 172줄: RawCampaignEditorIsland 116, RawCampaignNewFort 56
- [ ] `o/RawExisting*` 1단위 142줄: RawExistingEditorInstallOuter 142
- [ ] `o/RawBridge*` 1단위 112줄: RawBridgeEditor 112
- [ ] `o/RawDeck*` 1단위 105줄: RawDeckRestore 105
- [ ] `client/RawMenu*` 1단위 78줄: RawMenuEditorIsland 78
- [ ] `client/UberScenario*` 1단위 60줄: UberScenario 60
- [ ] `o/RawSubscriber*` 1단위 60줄: RawSubscriberIdGenerate 60
- [ ] `client/Existing*` 1단위 48줄: ExistingEditorAfterCheckpoint 48
- [ ] `client/EditorSave*` 1단위 43줄: EditorSaveGoMain 43
- [ ] `client/EditorTest*` 1단위 42줄: EditorTestBattle 42

#### R8 — AGENTS 4~14차 요구사항 전용 모듈

차수별 할 일은 [1.3](#dotnet-r8-requirements) 표가 기준이다. 권장 순서: 7차 오류 로그 마감(이미 선행) → 5차 사용자 루트 검증 → 4차 와이드·MCP(결정 D3) → 6차 한국어 → 13차 일본어 → 11차 난이도 → 12차 스커미시(Outpost 금지 유닛 포함) → 9차 시나리오 편집 → 10차 알려진 버그 수정 10건 대조(해당 파일을 옮길 때 이미 따라온 것 확인) → 8차 AA·FSR 1.0 → 14차 QuickJS 엔진 탑재.

**R8 체크 목록 — 대응 없는 cpppj 단위(X·W 영역 29단위 / 4,553줄).** W에는 cpppj 개발용 검사 도구(`app/FrameBindingInspect` 등)가 섞여 있으며 dotnetpj에 필요한 것만 옮긴다.

**X 요구사항 전용(4~14차) — 13단위 / 1,875줄**

- [ ] `client/Gpu*` 1단위 512줄: GpuPresenter 512
- [ ] `client/Skirmish*` 2단위 440줄: SkirmishSetup 416, SkirmishStartIslands 24
- [ ] `client/CampaignTest*` 2단위 324줄: CampaignTestPlay 176, CampaignTestSpectator 148
- [ ] `client/Korean*` 1단위 212줄: KoreanResources 212
- [ ] `client/Cd*` 2단위 183줄: CdDirectory 117, CdPresence 66
- [ ] `client/Language*` 2단위 94줄: LanguageOptions 52, LanguageRequest 42
- [ ] `client/Fsr*` 1단위 55줄: FsrConstants 55
- [ ] `client/Recheck*` 1단위 34줄: RecheckCdInput 34
- [ ] `client/ClientCd*` 1단위 21줄: ClientCdGlobals 21

**W 호스트·플랫폼·개발 도구(app·platform) — 16단위 / 2,678줄**

- [ ] `app/Mcp*` 1단위 483줄: McpServer 483
- [ ] `platform/Script*` 1단위 470줄: ScriptEngine 470
- [ ] `platform/Json*` 1단위 280줄: Json 280
- [ ] `platform/Audio*` 1단위 249줄: AudioLoopback 249
- [ ] `app/Frame*` 1단위 226줄: FrameBindingInspect 226
- [ ] `app/Inspect*` 1단위 200줄: InspectView 200
- [ ] `platform/Frame*` 1단위 196줄: FrameRecorder 196
- [ ] `platform/Png*` 1단위 194줄: Png 194
- [ ] `platform/Portable*` 1단위 84줄: PortableStartup 84
- [ ] `app/Canon*` 1단위 82줄: CanonMayPlaceInspect 82
- [ ] `platform/Legacy*` 1단위 75줄: LegacyRegistryValue 75
- [ ] `platform/Lan*` 1단위 59줄: LanIpv4Name 59
- [ ] `platform/File*` 1단위 25줄: FileOutput 25
- [ ] `platform/Console*` 1단위 22줄: Console 22
- [ ] `platform/Utf8*` 1단위 18줄: Utf8Path 18
- [ ] `platform/Scoped*` 1단위 15줄: ScopedNewHandler 15

#### R9~R11 — 60/120프레임·Linux·로컬 멀티플레이

- **R9 60/120프레임:** `client/FrameRateOption`(R8 목록)과 호스트 표시 루프. cpppj F1·F2와 같은 옵션(60/75 원본/120)과 양보 대기, 프레임 시간이 같음을 같은 방식으로 측정한다.
- **R10 Linux:** DesktopGL 호스트의 경로·파일 장치 분기(결정 D5), `WriteAtomic`·`ReadOnly`의 Windows 외 동작(2.6의 0.127).
- **R11 로컬 멀티플레이·Outpost:** 아래 목록. LAN TCP/UDP는 `LocalNetworkPolicy`의 루프백·로컬 네트워크만 허용하고, 공식 서버·클라이언트 유효성 검사·ping/진단은 옮겨도 연결하지 않는다. V(루트 서버 독립 모듈)는 로컬 서버 프로세스가 dotnetpj에 필요한지부터 판정한다.

**R11 체크 목록 — 대응 없는 cpppj 단위(Z·V 영역 131단위 / 10,322줄).**

**Z 멀티플레이·LAN·Outpost — 74단위 / 6,603줄**

- [ ] `client/NativeMeta*` 20단위 1,577줄: NativeMetaDisplay 216, NativeMetaScene 157, NativeMetaConnectedState 112, NativeMetaProjection 111, NativeMetaObjectInput 87, NativeMetaSail 78, NativeMetaParticipantCreate 76, NativeMetaSpriteDraw 73, NativeMetaBattleDescription 71, NativeMetaState 70, NativeMetaParticipantName 69, NativeMetaParticipantMove 69, NativeMetaSpriteRefresh 65, NativeMetaParticipantRemove 56, NativeMetaBattleCreate 53, NativeMetaPlayerMenu 51, NativeMetaSailError 44, NativeMetaBattleOptions 44, NativeMetaGeometry 42, NativeMetaBattleRemove 33
- [ ] `client/NativeFort*` 3단위 739줄: NativeFortSnapshot 536, NativeFortPreparation 104, NativeFortSaveVirtual 99
- [ ] `client/ClientMeta*` 8단위 608줄: ClientMetaPacketReceivers 205, ClientMetaPointerInput 130, ClientMetaSpriteRefresh 56, ClientMetaPlayerMenu 53, ClientMetaObjectInput 52, ClientMetaSpriteDraw 48, ClientMetaBattleDescription 36, ClientMetaBattleObjects 28
- [ ] `o/RawBattle*` 8단위 516줄: RawBattleCompletion 143, RawBattleOptionText 93, RawBattleResourceFrame 69, RawBattleOptionMutation 62, RawBattleReconnect 60, RawBattleOptionSlots 37, RawBattleNuggetPlacement 31, RawBattleResourceOptions 21
- [ ] `client/Meta*` 1단위 371줄: MetaConnectionRuntime 371
- [ ] `client/NativeBattle*` 3단위 226줄: NativeBattleInfoMenus 98, NativeBattleInfoInput 72, NativeBattleReturnState 56
- [ ] `client/HudNs*` 1단위 224줄: HudNsInfoMeta 224
- [ ] `client/Lan*` 1단위 216줄: LanBattleLobby 216
- [ ] `client/ClientChallenge*` 2단위 214줄: ClientChallengeGlobals 180, ClientChallengeImages 34
- [ ] `o/RawOutpost*` 1단위 148줄: RawOutpostLifecycle 148
- [ ] `o/RawState*` 3단위 130줄: RawStateBattleStart 57, RawStateBattleOptions 44, RawStateMetaBattleDescription 29
- [ ] `client/UberMeta*` 1단위 118줄: UberMeta 118
- [ ] `o/RawChallenge*` 2단위 108줄: RawChallengeState 74, RawChallengeLoginReply 34
- [ ] `o/RawLocal*` 1단위 108줄: RawLocalServerDiscovery 108
- [ ] `client/NativeIsland*` 1단위 102줄: NativeIslandRig 102
- [ ] `client/UberLan*` 1단위 101줄: UberLan 101
- [ ] `client/NativePlayer*` 1단위 98줄: NativePlayerInfoInput 98
- [ ] `o/RawOrdinary*` 1단위 96줄: RawOrdinaryBattleStart 96
- [ ] `client/NativeLogin*` 1단위 92줄: NativeLoginPacketReceivers 92
- [ ] `client/NativeSpring3d*` 1단위 91줄: NativeSpring3d 91
- [ ] `client/NativeGameplay*` 1단위 89줄: NativeGameplayPacketReceivers 89
- [ ] `client/NativeCamera*` 1단위 87줄: NativeCameraRig 87
- [ ] `o/RawMeta*` 1단위 73줄: RawMetaZoneCleanup 73
- [ ] `client/NativeConstruction*` 1단위 63줄: NativeConstructionPacketReceivers 63
- [ ] `client/NativeCarrier*` 1단위 62줄: NativeCarrierPacketReceivers 62
- [ ] `client/NativeNs*` 1단위 49줄: NativeNsInfoInput 49
- [ ] `client/HtmlZsend*` 1단위 48줄: HtmlZsend 48
- [ ] `o/RawCd*` 1단위 48줄: RawCdBattleCompletion 48
- [ ] `o/RawStartup*` 1단위 48줄: RawStartupStateCheck 48
- [ ] `client/NativeJoin*` 1단위 43줄: NativeJoinTestBattle 43
- [ ] `client/Saved*` 1단위 40줄: SavedStateFiles 40
- [ ] `client/NativeLocal*` 1단위 37줄: NativeLocalServerLaunch 37
- [ ] `client/UberReconnect*` 1단위 33줄: UberReconnect 33

**V root(루트 서버 독립 모듈) — 57단위 / 3,719줄**

- [ ] `root/Challenge*` 16단위 877줄: ChallengeRuntime 134, ChallengeStore 113, ChallengeState 69, ChallengePeriodic 66, ChallengeMove 62, ChallengeBattleControl 57, ChallengeChat 53, ChallengeLogin 52, ChallengeParticipants 40, ChallengeCommands 40, ChallengeReady 39, ChallengeBattleLaunch 35, ChallengeDetail 33, ChallengeAdmin 32, ChallengeCommandAdmin 28, ChallengeSend 24
- [ ] `root/Transceiver*` 4단위 334줄: TransceiverTransport 182, TransceiverSend 59, TransceiverReceive 52, TransceiverInternals 41
- [ ] `root/Shared*` 5단위 274줄: SharedFiles 151, SharedFileStorage 42, SharedFilesDetail 31, SharedGuildFiles 28, SharedGuildStorage 22
- [ ] `root/Root*` 4단위 262줄: RootDirectory 133, RootControl 68, RootDiagnostic 34, RootBroadcast 27
- [ ] `root/Packet*` 3단위 254줄: PacketRegistration 150, PacketObservers 64, PacketRecord 40
- [ ] `root/Server*` 3단위 212줄: Server 90, ServerList 86, ServerListFile 36
- [ ] `root/Socket*` 1단위 167줄: SocketDevice 167
- [ ] `root/Remote*` 3단위 158줄: RemoteCommand 61, RemoteCommandSession 57, RemoteCommandServer 40
- [ ] `root/Network*` 1단위 119줄: NetworkPump 119
- [ ] `root/Configuration*` 1단위 115줄: Configuration 115
- [ ] `root/Connection*` 1단위 114줄: ConnectionState 114
- [ ] `root/Main*` 2단위 107줄: MainLoop 88, MainSchedule 19
- [ ] `root/Lifecycle*` 1단위 99줄: Lifecycle 99
- [ ] `root/Process*` 1단위 90줄: ProcessTrace 90
- [ ] `root/Command*` 2단위 87줄: CommandPages 73, CommandDetail 14
- [ ] `root/Login*` 1단위 82줄: LoginRequest 82
- [ ] `root/Clock*` 1단위 81줄: Clock 81
- [ ] `root/Address*` 1단위 79줄: AddressCodec 79
- [ ] `root/Routing*` 1단위 71줄: Routing 71
- [ ] `root/Guild*` 1단위 38줄: GuildManagement 38
- [ ] `root/LocalAddress*` 1단위 38줄: LocalAddressDevice 38
- [ ] `root/Participant*` 1단위 32줄: ParticipantState 32
- [ ] `root/Random*` 1단위 16줄: Random 16
- [ ] `root/main*` 1단위 13줄: main 13

<a id="r3-r2-detail"></a>

### 2.5 R3·R2 잔여와 9-1 A

- [ ] **R3 잔여 → 이정표 D1:** 위젯 본체와 실제 호스트의 표시·입력 경로는 대부분 옮겼으나 R3 완료로 세지 않는다.
  - 대화창 분리(`ParseDialogPage`)와 `ConfigOverwriteAndQueueLifetime`의 남은 부분, MessageGump·대화창.
  - 맵/파일/기술 목록 owner, 실제 상위 메뉴 소유·분배, `RawMenuControls`의 전체 모달·등록 소유자와 링크·명령 소비자, Guide 우선 연결 등 기타 상위 명령·실제 맵/기술 목록 조립, HTML 그림 자산과 다른 파생·월드 통합 출력.
  - 전체 자산 조립, `ReadAssetQuickHelpType`의 TYPE/AST·요구 에너지 후처리, 음향.
  - `RawTextEntryControls`의 Console·부분 생성 정리·시스템 클립보드, Save as·숫자 창·ChatWindow, `RawBackgroundGifControls`의 노드·창 소유자.
  - 호스트 프레임·입력 큐, report·kernel-tell: 메인 메뉴 라벨 좌표는 실제 버튼에 연결했고(0.85) 하위 화면 라벨과 실제 호스트 명령 소비가 남는다.
  - 이정표 D1 메뉴 캡처 비교: ✅ 12장면 차이 0(0.381~0.388, 장면은 0.1). 남은 3장면(Language 자식 CJK·Multiplayer 섬 화면·Edit 이동행)은 R8·R5·R7로 미뤘다.
- [ ] **R2 잔여 세션 연결:** 실자산 어댑터, 수집·정렬·kind 분배, `SpriteDraw` 클래스·그림자 alias 공유, `owner-capture` C행 480개는 R4·R5에서 잇는다.
- [ ] **9-1 A 나머지([4절](#original-parity-plan)):** 정수 틱을 실제 시각·원본 타이머로 옮기는 일을 세션 교체와 함께 처리한다.

<a id="carried-limits"></a>

### 2.6 이월한 이식 한계 (이후 단위에서 해소됐다는 기록이 없는 것만)

- 0.80: 초기 구름 카메라 원점은 임시 범위이고 실제 카메라(R5)·시간별 이동·부분 dirty는 미연결이다. 512×512 구름 파일이 없거나 크기가 다를 때의 원본 생성 fallback은 이식하지 않아 명시 오류다.
- 0.81: 반복 `--set`은 C# 호스트가 병합하고 같은 이름은 마지막 값을 쓰지만 cpppj는 마지막 묶음만 보관한다. 오라클 연결 때 계약 차이를 정리한다.
- 0.86·0.87: 프레임 사이의 빠른 물리 입력 이력, 공통 음향·음악(R6 범위)과 옵션 변경 중 재생 관리가 남았다.
- 0.93: 부분 생성 중 돌 버튼 내부 배경 할당 실패의 모든 사례를 검증하지 않았다.
- 0.126: cpppj와의 이관 장부 교차 읽기, `MatchDisk`의 Win32 특수 경계(절대 경로·긴 경로)는 검증하지 않았다. 실제 Editor State1 월드·저장·Save as의 Template/현재 이름 세션 소유자는 미이식이다(F3 범위).
- 0.127: Windows 외의 `WriteAtomic`은 write-through·재시도가 없는 준비용 `File.Move`이며 durable 검증이 아니다. `ReadOnly`의 Windows 외 동작은 관측하지 않았다. 새 파일 장치의 이름 정규화(ASCII만 접음)와 기존 `GameFileSystem.NormalizePath`(비ASCII도 접음)의 차이는 고치지 않았다. cpppj `RelativePath`는 옮기지 않았다.
- 0.128: 기존 `Netstorm.Assets.FortFile`은 연결하지 않은 이전 독립 판독기로 남아 있으며 은퇴는 R4 Fort 적재 단위에서 한다.
- 0.132: `WriteStream`은 입력을 배열로 복사하므로 입력이 같은 섹션 버퍼의 구간이면 원본의 DWORD 순방향 복사와 다를 수 있다(현재 호출처에는 없음, 소비자를 옮길 때 확인).
- 0.134: `AllocateEmptyManager`의 할당 실패(NULL) 경로는 이식하지 않았다.
- 0.135: 짝이 맞지 않는 UTF-8(옛 ANSI) 이름은 manager에서 꺼낼 때 U+FFFD로 바뀌어 왕복하지 않는다. `Dump`의 파일명은 바이트 문자열 그대로 출력된다.
- 0.136: `HostFortNameConfirmation`의 `header`·`setName` 실제 연결(현재 Template 소유자·전체 snapshot)은 F3이다. `FortCurrentNameDevices`의 두 dirty DWORD 주소 비교는 생략했다(호출자가 `ClientFortGlobals.Catalog`에 연결해야 함). cpppj는 비ASCII 이름을 CP1252→UTF-8로 이중 인코딩하는 차이가 있다.
- 0.137·0.138: `ReadSections` 검사의 `Parse` 이식은 F3(세션 표 소유)이다. `from_chars` 지수 언더플로 거부는 MSVC 동작을 확인하지 못해 옮기지 않았다. `TypeRequiredEnergyTests`·`ArcherTypeDataTests`·`CanonTypeDecoder` 계열과 세션 통합 검사는 F2·F3에서 잇는다.
- 0.152·0.208·0.220·0.230·0.268: `LocalMissionTransport`·로컬 미션 패킷·Host 전송(`WriteFormPacket`)·월드 전송(`SynchronizePeer`·`TransmitDeletion`)·세션 연결 Stub은 검사 없는 사전 구현이거나 명시 오류다. Zone·로그인·분배 검사는 F3 세션 연결 때, TCP/IP 소켓과 송수신은 3차 로컬 구현 때 `LocalNetworkPolicy`의 루프백 전용으로 더한다.
- 0.153: `AddRandom_Native288`의 대본 GameMenu 후반부(`QueueScriptCommand`·`GameMenuScriptCode`)는 R6에서 옮긴다.
- 0.154: 판본 분기 잔여(원문 근거는 있으나 fixture에서 관측되지 않음) — `RawSquidFactory.Take`·`PostCreate`의 나머지 CD 분기, `RawPop`의 무효 void CD throw·겹침 CD 항상 쓰기, `RawUnpop`의 추가 비트 CD 위치, `RawPostPop`·`RawUnpop`의 표시 지원 판본 overload 연결. 탐색기 발자국 조건은 원문(`footX<0`) 대신 기존 C# 조건(`FootX<1`)을 유지했다(기존 직접 검사 `RawFinderTests.FootprintValidationRejectsBadFeet`와 충돌, 차이로 기록). `RawPathSearch` 전체·`RawCanonPlacementPipeline`·`MakeFortRandomPlacementHooks`는 R6이다.
- 0.159: 배치 지형의 파이프라인 어댑터·통합 검사는 R6 범위로 뺐다.
- 0.176: `RawSurfaceStorage.UpdateAmbientWind`의 F3 연결이 남았다.
- 0.182: `Kernel` 진단 5종은 Client·`SquidProcess`가 필요해 옮기지 않았다.
- 0.221~0.224: 공장 HP의 소비(`TypeHitPoints`)는 R6, Create/Take 부모 진단 전달은 월드·세션 조립 때 잇는다.
- 0.242: 월드 보고서의 효과·삭제 요청 계수기 증가 연결(수명주기 쪽)은 미연결이다.
- 0.305: 정리 뒤 정기 조회 꼬리는 오라클도 실패하므로 원본 동작 그대로 두었다([1.4](#cpppj-requests) 요청 6).
- 0.330~0.341의 C# 모양 차이: 문자열은 Latin1로 풀어 string으로 전달(ASCII 범위에서 원문과 동등), C++ 약한 참조·공유 포인터는 `WeakReference`·`IDisposable` pin으로, 세션의 cpp 공유 참조는 내부 소유 배열·대리자(`Bind*`) 주입으로 옮겼다. 실제 공유 바인딩은 실제 호스트 조립에서 잇는다.

<a id="dotnet-inventory"></a>

### 2.7 영역별 집계 (2026-11-15, cpppj 제품 1313 · dotnetpj 0.545 · webpj WB04-E495)

`python -X utf8 tools/port_inventory.py`의 출력이다. 단위는 같은 디렉터리의 같은 이름 `.cpp/.h/.inc` 묶음, 줄 수는 현재 cpppj 소스 줄(주석 포함), `dn 대응 있음/없음`은 `dotnetpj/PORT_MAP.tsv`에 그 단위의 파일이 하나라도 있는지다. 오른쪽 다섯 칸은 같은 단위의 webpj 상태로, 두 프로젝트의 진행을 나란히 보려고 함께 적었다. 영역은 이름의 낱말로 정한 근사 분류다.

| 영역 | cpppj 단위 / 줄 | dn 대응 있음 | dn 대응 없음 | wb port | wb partial | wb todo | wb 제외 | wb 대응표 밖 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| A 자료·파일·설정 | 45 / 4,652 | 15 / 2,080 | 30 / 2,572 | 32 / 3,561 | 2 / 497 | 1 / 33 | 5 / 250 | 5 / 311 |
| B 월드 코어(SID·슬롯·Pop·해시) | 76 / 9,489 | 45 / 7,306 | 31 / 2,183 | 65 / 7,758 | 1 / 709 | 1 / 68 | 4 / 590 | 5 / 364 |
| C Fort·지형·섬 적재 | 32 / 5,328 | 26 / 4,345 | 6 / 983 | 29 / 5,100 | 0 / 0 | 0 / 0 | 0 / 0 | 3 / 228 |
| D 세션·월드 조립·로그인·패킷·Client 전역 | 127 / 26,874 | 53 / 18,361 | 74 / 8,513 | 72 / 10,996 | 4 / 2,858 | 17 / 10,794 | 2 / 108 | 32 / 2,118 |
| E 표시·렌더러·카메라·HUD·시각 효과 | 94 / 13,047 | 42 / 6,881 | 52 / 6,166 | 82 / 11,717 | 2 / 299 | 5 / 871 | 3 / 100 | 2 / 60 |
| F 입력·커서·선택·게임 메뉴 | 58 / 5,535 | 8 / 1,052 | 50 / 4,483 | 41 / 3,759 | 4 / 266 | 10 / 1,315 | 1 / 27 | 2 / 168 |
| G UI 프레임워크·메뉴(Gump·HTML·Tell) | 104 / 19,666 | 54 / 12,068 | 50 / 7,598 | 62 / 10,098 | 12 / 4,342 | 20 / 4,690 | 5 / 302 | 5 / 234 |
| H 사제·운반·수확·경로 | 86 / 11,047 | 27 / 3,777 | 59 / 7,270 | 73 / 9,631 | 1 / 511 | 6 / 658 | 0 / 0 | 6 / 247 |
| I 건설·생산·경제·다리 | 104 / 14,605 | 32 / 5,571 | 72 / 9,034 | 81 / 12,132 | 1 / 48 | 9 / 1,515 | 0 / 0 | 13 / 910 |
| J 전투·비행·투사체·방어 | 121 / 14,635 | 24 / 2,609 | 97 / 12,026 | 102 / 12,964 | 0 / 0 | 8 / 877 | 0 / 0 | 11 / 794 |
| K AI | 44 / 4,807 | 6 / 564 | 38 / 4,243 | 42 / 4,752 | 1 / 28 | 0 / 0 | 0 / 0 | 1 / 27 |
| L 신전·제단·의식·승패 | 38 / 3,764 | 3 / 150 | 35 / 3,614 | 27 / 2,713 | 0 / 0 | 1 / 161 | 2 / 178 | 8 / 712 |
| M 미션 수명·안내·대본·튜토리얼 | 32 / 4,622 | 13 / 1,681 | 19 / 2,941 | 25 / 3,653 | 0 / 0 | 6 / 952 | 0 / 0 | 1 / 17 |
| N 소리·음악·동영상 | 17 / 3,822 | 4 / 1,591 | 13 / 2,231 | 3 / 112 | 4 / 2,041 | 10 / 1,669 | 0 / 0 | 0 / 0 |
| O 채팅·메시지 | 26 / 2,933 | 7 / 693 | 19 / 2,240 | 24 / 2,843 | 0 / 0 | 1 / 48 | 0 / 0 | 1 / 42 |
| X 요구사항 전용(4~14차) | 14 / 1,974 | 1 / 99 | 13 / 1,875 | 2 / 94 | 1 / 212 | 0 / 0 | 6 / 805 | 5 / 863 |
| Y Edit·시나리오 편집 | 61 / 6,150 | 35 / 3,434 | 26 / 2,716 | 2 / 112 | 0 / 0 | 0 / 0 | 54 / 5,300 | 5 / 738 |
| Z 멀티플레이·LAN·Outpost | 81 / 7,879 | 7 / 1,276 | 74 / 6,603 | 4 / 514 | 0 / 0 | 0 / 0 | 6 / 869 | 71 / 6,496 |
| V root(루트 서버 독립 모듈) | 57 / 3,719 | 0 / 0 | 57 / 3,719 | 0 / 0 | 0 / 0 | 0 / 0 | 0 / 0 | 57 / 3,719 |
| W 호스트·플랫폼·개발 도구(app·platform) | 20 / 4,469 | 4 / 1,791 | 16 / 2,678 | 0 / 0 | 0 / 0 | 0 / 0 | 0 / 0 | 20 / 4,469 |
| **합계** | 1237 / 169,017 | 406 / 75,329 | 831 / 93,688 | 768 / 102,509 | 33 / 11,811 | 95 / 23,651 | 88 / 8,529 | 253 / 22,517 |

- **읽는 법:** dotnetpj는 왼쪽(대응 없음 831단위 / 93,688줄), webpj는 오른쪽(todo 95 + partial 33 = 128단위)이 남은 이식량이다. webpj는 부품 이식이 앞서 있고 세션 조립이 남았으며, dotnetpj는 세션 조립이 앞서 있고 부품 이식이 남았다. 같은 cpppj 단위를 옮길 때 다른 쪽의 단위 문서(`docs/webpj-*.md`·`docs/dotnet-*.md`)에 적힌 최초 실패·경계 기록을 먼저 읽으면 같은 함정을 피할 수 있다.
- **다시 만들기:** 요약 표는 인자 없이, 단계 체크 목록의 원자료는 `--families dn`, 단위별 원자료는 `--tsv <경로>`다. 영역 규칙은 도구의 `TOKEN_RULES`에 있다.

## 3. 검증 단계와 계속 유효한 결정·제약

<a id="verify-levels"></a>

### 3.1 검증 단계 L1~L5와 기록 규칙

- **L1:** 원본 x86 fixture. **L2:** 같은 미션 Load/Start 상태가 cpppj와 같다(cpppj `session-dump`). **L3:** 같은 고정 시계·입력열로 N프레임 진행한 상태 해시가 같다(`--fixed-frame-ms` + 프레임별 `session-dump`). **L4a:** 같은 ui-script의 논리 캡처가 픽셀 차이 0이다. **L4b:** 소리 시점과 곡 선택이 같다. **L5:** 자연 완주·저장·재실행 — 사용자 확인 대상이라 AI 작업과 단계 완료 기준에 넣지 않는다.
- cpppj 캡처·덤프는 오라클 SHA·스크립트·`--set` 값·`--fixed-frame-ms` 값·프레임과 함께 기록한다. 논리 캡처(`snapshot`)와 실제 출력 캡처(`snapshot-output`)를 구별한다. 실행에는 `originals/`가 아니라 `extracted/` 아래의 데이터 사본과 빈 사용자 루트를 쓴다.
- 한 단위를 마칠 때 기록할 것: 옮긴 cpppj 파일과 기준 커밋, 연결 fixture와 행 수, 이식한 cpppj 검사 수, 은퇴시킨 기존 코드와 검사, 비교한 캡처·덤프 수와 차이, 함께 옮긴 10차 NB 수정, 남은 경계. L1 통과를 L2 이상으로 합산하지 않는다. 기존 근사 검사의 은퇴로 실패를 감추지 않으며, 원본 fixture 기대값을 C# 결과로 바꾸지 않는다.
- 클론·cpppj 창 실행은 AGENTS.md의 확인·예외 규칙을 따른다(VM-W11-Codex·HJOW-Athlon·vm-debian-codex는 예외 PC, 그 밖의 PC는 실행 전 목적 설명과 명시적 확인). 이 계획은 원본 게임을 실행할 필요가 없다.

<a id="dotnet-constraints"></a><a id="07-계속-유효한-결정제약"></a>

### 3.2 계속 유효한 결정·제약

- **목표 순서:** ① 원본 10.78과 동일한 동작 → ② 요구사항(AGENTS 4~14차, 화면비·60/120프레임·다국어·재실행 오류 방지) → ③ Linux. 이미 들어간 요구사항 기능(한국어 선택·와이드·전체화면)은 버리지 않고 새 경로에서 cpppj 방식(논리 해상도·호환 전체화면)으로 다시 만든다. cpppj HEAD에 섞인 요구사항 코드도 함께 옮기고 기본 설정에서 먼저 검증한다(결정 D1).
- **AGENTS.md 요구의 반영 상태** — [9.1](#agents-reflection) 표.
- **글꼴·명칭·주석:** 영어 화면은 원본 `.chfnt`, 한국어는 D2Coding, 일본어·기타 언어는 Noto Sans·Noto Sans JP다([5절](#dotnet-user-decisions) 2·7번). 글꼴은 exe(어셈블리 리소스 포함)에 내장하지 않고 배포판 `Netstorm.exe` 옆 `fonts/`에서 별도 파일로 읽으며 라이선스 파일도 함께 배포한다. 현재 기존 경로는 D2Coding 1.3.2를 출력 폴더 `fonts/`로 복사한다(`CopyToOutputDirectory`). AGENTS.md의 한국어 명칭 규칙을 따른다. 기존 주석은 대상을 고칠 때 정비한다(5절 8번).
- **cpppj 공통 제약을 준용한다.** CHEAT 항목은 Edit 편집에서만 나타난다. 공격형 공중 3종(훨리기그·맨오워·더스트 데빌)은 코드를 옮기되 실제 생산·생성·우클릭 재현과 추가 실행 관측은 하지 않는다. 연녹색 직사각형 분석은 보류한다(원본에서도 관찰됨). 대응 `.english`가 없을 때 Test Battle이 끝나는 원본 동작과 새 맵 `.english` 자동 생성은 9차(cpppj S2) 계약을 따른다. 실제 실행 검사에서 사제 포획/희생·특정 적 공격·시나리오 승리를 재현해야 하는 검사는 건너뛴다([TODO.md](TODO.md)). MP 실행 검사는 하지 않는다.
- **cpppj의 미완·경계 범위는 cpppj를 따라간다.** cpppj가 지금도 명시 경계·보류로 둔 것(LEFT_JOBS.md 4.1 NoWorld·외부 기능, 5.1 보류 분석, 5.2 기존 한계)만 dotnetpj도 경계로 둔다. 옛 목록(View/zoom, 도킹, DD 조정·확인 UI 잔여, Chat GUI 드래그, 미니맵 내용물·grid·항목별 debug, Renderer 시차 경계)은 cpppj 929 UI 마감 이전 기준이므로 해당 화면을 옮길 때 항목별로 cpppj HEAD 구현 여부를 확인해, 구현된 것은 이식 대상으로 바꾼다. 독립 미니맵, Credits 자동 전환, Chat 입력·전달, 마나 103·배율 Draw는 이미 이식 대상이다.
- **근거 우선순위:** 원본 fixture → cpppj 보고서의 계약·경계 → cpppj 소스 → 녹화·스크린샷. cpppj는 HEAD만 쓰고 미커밋 변경은 쓰지 않는다. `cpppj/`·`LEFT_JOBS.md`는 수정하지 않고, 필요한 요청은 [1.4](#cpppj-requests)와 이식 계획 8절에 적어 사용자에게 전달한다(사용자가 두 문서를 함께 정리하라고 지시한 경우는 예외).
- **외부 접속:** 새 경로의 Help 링크(R3)·시작 화면의 검증 안내·`Netstorm.Net`(3차)을 옮길 때마다 [dotnet 외부 접속 문서](docs/dotnet-external-endpoints-20261103.md)의 소비자 목록을 갱신한다. 구 클라이언트 검증·master/root 인터넷 서버·ping/진단은 이식해도 연결하지 않고 성공을 합성하지 않는다. 사용자가 선택하는 도움말 링크 실행과 로컬 서버/LAN 접속(3차)은 별도 경계다(0.151).
- **보호·커밋:** 원본/CD 디렉터리·AGENTS.md·루트 README.md의 "개요"는 수정하지 않는다(README.en.md "Overview"는 번역으로만). push하지 않는다. 새 문서·코드는 UTF-8과 한국어 주석 규칙을 따른다.

<a id="original-parity-plan"></a>

## 4. 9-1 결정 — 의도적 차이는 원본처럼 동작하도록 바꾼다 (2026-10-10 사용자 결정)

**결정:** dotnetpj가 일부러 다르게 두었던 차이(아래 A~F)는 원본처럼 동작하도록 바꾼다. **지금 구조를 둔 채 원본 게임의 경험을 거의 동일하게 만들 수 있으면 구조는 유지해도 되지만, 그렇게 할 수 없으면 구조 자체를 원본처럼 바꾼다.** 요구사항·편의는 더 이상 차이를 그대로 두는 근거가 아니다. 목표 순서 ①에 속한다.

**항목마다 밟는 순서:** ① 그 차이 때문에 원본과 달라지는 동작을 구체적으로 적는다 → ② 원본 근거(cpppj의 x86 기대값·`--dump-*` 출력·분석 문서·녹화)와 같은 입력으로 비교한다 → ③ 차이가 없거나 플레이에서 구별할 수 없으면 구조를 유지하고 근거를 이 문서에 남긴다 → ④ 구별되면 먼저 지금 구조 안에서 고치고, 안 되면 구조를 원본 방식으로 바꾼다. **비교 근거 없이 "같다"고 적지 않는다.** 2026-10-28부터 MonoGame은 창·입력·오디오 장치·최종 출력 계층으로만 쓰고 게임 화면은 8비트 논리 화면에 그린다(결정 1). cpppj가 원본 배치를 쓰는 곳은 C#도 같은 배치를 쓴다(결정 2). CD 10.72·10.82 값을 쓰지 않는 것은 기준 판본(10.78)을 따르는 것이라 원본과의 차이가 아니다.

| # | 판정 시작 시 차이(원본) | 원본과 달라질 수 있는 동작 | 처리 시점·현황 |
| --- | --- | --- | --- |
| A | 24Hz 고정 틱 + 화면 보간(원본은 고정 틱 없이 프레임마다 한 번 돌고 "지금 + 간격" 타이머를 쓴다) | 사건이 틱 경계(41.7ms)에 맞춰 일어나고, 명령이 다음 틱 처음에 실행되며, 붕괴 스캔이 틱당 33개다(원본은 프레임당 11개) | **첫 단계 완료(2026-10-11):** 명령·프레임 붕괴 스캔·정지 시각 보존, 화면 루프·숫자 시계·세션 시계를 프레임 처리로 바꾸고 원본 10.78 Scan 1,948행·Init 4행과 일치시켰다([근거](docs/dotnet-frame-parity-20261011.md)). **나머지(R4 Kernel/Regular와 함께):** 이동·전투·건설·생산·수확·의식·AI·튜토리얼·애니메이션의 정수 틱을 실제 시각/원본 타이머로 이행, 과도 단계의 `FrameSeconds`·`Seconds`·8틱 상한·정지 중 보간 잔여 정리, 안내창 재개/브리핑 지연·FPS 표본/Capture·초 경계 초기화, 이벤트 Tick·RunTicks 기반 검사 이전. A 전체 완료로 처리하지 않는다 |
| B | 입력을 MonoGame으로 읽는다(원본은 사건 큐 40칸, 넘치면 새 사건을 버리고 한 바퀴에 한 번 처리) | 빠른 연속 클릭·끌기·키 반복의 처리 순서와 누락 | R1에서 입력 큐를 만들었고 전체 입력 처리(`client/UserInput`·`InputEvent`)는 R6에서 옮긴다 |
| C | 자체 `settings.json`을 쓰고 `options.cfg`는 읽기만(원본은 `d/options.cfg`를 시작·변경 시 저장하고 `fullscreenStateFile.dat`를 만들고 지운다) | 옵션 저장 시점과 위치 | R7. 새 경로는 0.114에서 변경 옵션의 `options.cfg` 저장을 선행했다. **전체화면 전환 뒤 재실행 오류는 클론에서 발생하지 않아야 한다**(AGENTS). 위치는 cpppj와 같은 사용자 루트(결정 D1) |
| D | 그리기 직전 제한의 대기를 Sleep/Yield로 양보(원본은 바쁜 대기) | 실제 간격·흔들림·초과 대기 | R7에서 판정. cpppj F2(1298)가 양보 대기로 바꾸고 **프레임 시간이 같음**을 확인했으므로([근거](docs/exe/cpp-post-plan-fps-cd1072-20261111.md)) 판정 때 이 근거와 같은 방식의 측정을 쓴다. 근거 없이 같다고 적지 않는다 |
| E | 매 프레임 전체를 다시 그린다(원본은 변경 사각형 100개 표로 부분만) | 화면 결과가 같아야 한다 | R1·R5에서 cpppj Renderer의 dirty 경로로 처리 |
| F | 뮤텍스·레지스트리·CD 찾기·창 크기 고정이 없다 | 창 크기 조절·중복 실행처럼 사용자가 보는 동작 | R7에서 판정. 사용자가 보는 동작만 원본대로 맞춘다([cpp-screen-reconstruction.md](docs/exe/cpp-screen-reconstruction.md)) |

판정 근거 문서: A는 [main-loop.md](docs/exe/main-loop.md) 7절·[cpp-process-reconstruction.md](docs/exe/cpp-process-reconstruction.md)·`process-x86.tsv`·`bridgedecay-x86.tsv`의 Scan 행, B는 [cpp-screen-reconstruction.md](docs/exe/cpp-screen-reconstruction.md) 6절, C는 [cpp-options-reconstruction.md](docs/exe/cpp-options-reconstruction.md)와 options x86 기대값 16개, D는 main-loop.md 4·7절, E는 [cpp-renderer-reconstruction.md](docs/exe/cpp-renderer-reconstruction.md). 옛 9절 표·옛 순서 제안은 [보관 파일](LEFT_JOBS.dotnetpj.old.md)에 있다.

<a id="dotnet-user-decisions"></a>

## 5. 사용자 결정·확인 기록 (옛 10절, 계속 유효한 것)

번호는 옛 10절 번호다. 해결·구현이 끝난 1·3·4번은 보관 파일에 있다. 2026-11-11에 확정한 D1~D5는 [0.5](#decisions)다.

2. **영어 UI 글꼴(2026-10-10 사용자 결정):** 판단 기준은 선택한 언어다. 영어이면 UI 글자를 원본 비트맵 글꼴(`.chfnt`)로 그리고 폭도 그 글꼴로 잰다. 한국어이면 D2Coding을 쓴다. 영어 화면에 원본 글꼴에 없는 글자(한글 등)가 섞이면 D2Coding으로 대신 그린다(에이전트 해석). 새 경로는 R2·R3의 `BitmapFont`·Gump 글리프로 영어를 그리고, 한국어는 R8(6차)에서 8비트 화면용으로 다시 만든다(결정 D4). 글꼴 파일은 exe 옆 `fonts/`에서 읽는다(AGENTS `75e4eed9`).
5. **"원본과 완전히 동일한 동작"과 의도적 차이(2026-10-10 사용자 결정):** 원본처럼 동작하도록 바꾼다. 구조를 둔 채 경험을 거의 동일하게 만들 수 있으면 구조는 유지해도 되지만, 그렇게 할 수 없으면 구조 자체를 원본처럼 바꾼다. 항목별 판정은 [4절](#original-parity-plan).
6. **이미 들어간 요구사항 기능(2026-10-10, 에이전트 해석):** 한국어 선택·와이드 화면 시야 확장·테두리 없는 전체화면 창은 되돌리지 않는다. 기본 실행(영어·4:3)에서 원본과 같은 동작을 먼저 맞추고, 새 경로로 옮길 때 cpppj 방식으로 다시 만든다.
7. **D2Coding 1.4.0 교체(2026-10-10 사용자 결정): 중요도가 낮다.** 당분간 1.3.2를 쓰고, 글꼴 코드를 고칠 일이 있거나 여유가 있을 때 한다. 교체 때 확인할 것: 코드 3곳의 파일명(`Netstorm.Game.csproj`·`NetstormGame.cs`·`TrueTypeCollectionTests.cs`), 배포 스크립트의 글꼴 경로와 고지문, TTC face 순서·글자 폭·줄 높이, `TrueTypeCollectionTests` 기대값, 한국어 화면 캡처, README 두 개의 파일명. 새 경로의 6차 작업은 글꼴 코드를 새로 만드는 일이라 이때 cpppj와 같은 최신 D2Coding으로 맞추는 것을 기본으로 한다([1.3](#dotnet-r8-requirements) 6차).
8. **기존 코드 주석 정비(2026-10-10 사용자 결정):** 주석이 설명하는 대상이 수정될 때 새 규칙(설명·사용법 먼저, 추가/수정 이력은 뒤)에 맞춘다. 고치지 않는 대상의 주석을 일괄 정리하지 않는다.
9. **cpppj 미복원 동작(2026-10-16, 에이전트 기본값):** 동결된 기존 경로에서만 근사를 유지하고 [core-rules.md](docs/core-rules.md)의 근사 목록에 "원본 근거 없음"으로 등록한다. 새 경로는 결정 6.
10. **원본 월드 코어 도입(2026-10-16, 에이전트 판단):** 5번 결정의 "구조 자체를 원본처럼 바꾼다"에 해당한다([이식 계획 4절](docs/dotnet-cpp-port-plan-20261016.md#4-구조-결정-c-원본-월드-코어-도입)). 원본 월드 코어는 R4에서 세션에 조립한다.
11. **원본 월드 코어의 저장 방식(2026-10-16, 에이전트 판단):** 코어 저장소는 슬롯 50바이트를 그대로 둔다(W02~W03 fixture가 슬롯 전체 바이트를 대조하고 파생 생성자 쓰기가 필드 이름 없는 오프셋이라 의미 모델로는 검증할 수 없다). 가상 표 주소는 클래스 식별 값으로만 쓴다. 결정 2로 범위를 넓혀, cpppj가 원본 배치를 쓰는 곳은 세션·화면 쪽도 같은 배치를 쓴다.
12. **Template 저장소 이식 방식(2026-11-01 사용자 결정):** 계획서 기본값 네 가지(`RawSectionBuffer`, `RiftTypeTable` 직역, `FortTemplate` 위치는 F1에서 확정, 저장소 cpppj 대조 대신 fort 433개 왕복 검사)로 진행했다([계획](docs/dotnet-template-storage-plan-20261101.md) 7절).

## 6. 검증 명령과 운영 규칙

<a id="dotnet-commands"></a>

### 6.1 검증 명령

```powershell
# 저장소 루트에서 기준선과 단계별 검증 (헤드리스)
# 실제 TYPE 기술 소비자 검사에는 이 PC의 보호 원본 사본을 명시한다.
$env:NETSTORM_DATA = (Resolve-Path "extracted/dotnet-oracle-data").Path
dotnet build dotnetpj/Netstorm.sln -c Release
dotnet test dotnetpj/Netstorm.sln -c Release

# cpppj 콘솔 출력 대조 (헤드리스 명령만 사용. --run 등 게임/창 실행은 해당 PC의 규칙을 별도로 확인)
cpppj/build/bin/Release/Netstorm.exe --dump-types originals
cpppj/build/bin/Release/Netstorm.exe --dump-world originals thewarbegins
cpppj/build/bin/Release/Netstorm.exe --inspect-bridges originals

# cpppj 반영 범위 확인. 저장소 루트에서 실행. 마지막 반영 커밋은 cpp-fixture-manifest.tsv 의 reflected-commit
python dotnetpj/tools/cpp_sync.py                                    # 보고 (읽기 전용, 연결 fixture 가 어긋나면 종료 코드 1)
python dotnetpj/tools/cpp_sync.py --update --reflect HEAD            # HEAD까지 영향 검토·반영을 모두 마친 경우에만 갱신
python dotnetpj/tools/cpp_sync.py --update --add 이름:Core           # 새 fixture 를 연결할 때 목록에 등록 (Assets 도 가능)

# 영향 재검토(1.2): 지난 검토 이후 바뀐 cpppj 소스와 PORT_MAP 의 교집합
git diff --name-only 4d4b3a928..HEAD -- cpppj/src
```

```powershell
# cpppj 오라클 캡처. 창이 뜨므로 AGENTS.md 확인·예외 규칙을 따른다.
# 데이터는 originals/ 대신 extracted/ 아래 사본을 쓴다. steps.tsv 한 줄은 "프레임<TAB>동작<TAB>인자"다
# (click·click-if·right·key·esc·pointer·snapshot·session-dump 등. snapshot·session-dump 인자는 저장할 파일 경로).
cpppj/build/bin/Release/Netstorm.exe --run <데이터 사본> --window --no-audio --frames 120 `
  --set "SCREENW=1024;SCREENH=768;maxFPS=40;sleepPerLoop=25;autoDemo=0;tellTips=0" `
  --ui-script steps.tsv --ui-report states.tsv            # 미션 직행은 --mission thewarbegins 등을 더한다
# L4a 일괄 도구(0.380): 같은 ui-script 로 cpppj 와 dotnetpj 호스트를 각각 돌려 캡처를 만들고 바로 비교한다. 종료 코드 0 일치·1 차이·2 오류.
#   steps 템플릿의 캡처 경로 자리에 {OUT} 을 쓴다(예: dotnetpj/tools/l4a/menu-main.tsv). 캡처는 extracted/dotnet-oracle-captures/<Name>/cpp, dn 에 남는다.
#   powershell -NoProfile -ExecutionPolicy Bypass -File dotnetpj/tools/l4a_compare.ps1 -Steps dotnetpj/tools/l4a/menu-main.tsv -Name menu-main -Frames 130 [-Build] [-SkipCpp]
#   창이 뜨므로 AGENTS.md 확인·예외 규칙을 따른다. 실행 전후 originals/ 해시를 비교해 달라지면 실패로 끝낸다.
# 캡처 비교 도구: python dotnetpj/tools/cpp_frame_compare.py A.bmp B.bmp [--diff diff.png]  /  --dir 폴더A 폴더B (종료 코드 0 일치·1 차이·2 입력 오류)
# dotnetpj 쪽의 같은 실행 인자(--frames/--ui-script/--ui-report/--set/--fixed-frame-ms)는 Netstorm.Client.Host 가 받는다.
# PowerShell 5.1의 Start-Process -PassThru 는 종료 코드를 비우므로 `& exe 인자; $LASTEXITCODE` 로 읽는다.

# cpppj 실제 Session 상태 덤프(L2)·고정 시각 실행(L3).
# 결정론 비교에는 가상 시계(--fixed-frame-ms)와 빈 사용자 루트를 쓴다. 기본 OS 시계 두 실행은 게임 시각부터 달라 비교할 수 없다.
# WelcomeGiven=1 이 없으면 환영 창 때문에 메뉴 클릭이 실패하고, --mission 뒤 'Play Mission' 은 늦게 나타나므로 click-if 를 여러 프레임에 둔다.
$env:NETSTORM_USER_DATA = "<빈 폴더>"
cpppj/build/bin/Release/Netstorm.exe --run <데이터 사본> --mission thewarbegins --fixed-frame-ms 25 --window --mute-audio --frames 250 `
  --set "SCREENW=1024;SCREENH=768;WelcomeGiven=1;autoDemo=0;tellTips=0" --ui-script steps.tsv --ui-report report.tsv
# steps.tsv 예: "5<TAB>click-if<TAB>Play Mission", "90<TAB>session-dump<TAB><출력 경로>.tsv"
# 덤프 형식(판번호 1, ASCII·LF·탭, float/double 은 비트 정수): docs/exe/cpp-session-state-dump-20261109.md
```

- 클론 창을 띄우는 검사(`tools/clone_*_smoke.ps1`, `dotnet run --project dotnetpj/src/Netstorm.Game`)는 AGENTS.md의 확인·예외 규칙을 따른다.
- 원본 x86 기대값 대조만 돌릴 때: `dotnet test -c Release --filter "FullyQualifiedName~X86"`. 새 기대값 파일은 `tests/Netstorm.Core.Tests/Netstorm.Core.Tests.csproj`에 링크로 추가하고(사본 금지) `cpp_sync.py --update --add`로 SHA-256 목록에 등록한다(빠뜨리면 `EveryLinkedFixtureIsRegistered`가 실패). 한 fixture는 한 소비자 프로젝트에서 검사한다(`config-x86.tsv`는 Assets.Tests). 포트 맵의 `port` 행은 fixture 대조가 없는 이식이며 L1이 아니다.
- **변이 검사:** 대표 변이를 수동으로 넣고 제품·검사 빌드 뒤 `--no-build` 집중 실행으로 검출을 확인한 다음 원복하고 집중 검사를 다시 통과시킨다. 원복 뒤 증분 빌드가 옛 결과를 쓰지 않게 필요하면 `--no-incremental`로 다시 빌드한다. 사본(`extracted/dn-mutation-ts0`)을 쓸 때는 다중 행 패턴 들여쓰기가 틀리면 일치 0개로 조용히 건너뛰므로 개수 assert로 잡고 복사한 파일은 반드시 `touch`한다. 무한 루프로 걸리는 변이도 검출로 기록한다.
- **전체 검사 완주:** 단위마다 집중 검사로 넘어가더라도 20단위 또는 한 작업일마다 솔루션 전체 검사를 끝까지 돌려 0.3의 기준선을 새로 적는다.
- **오라클 자료:** 데이터 사본 `extracted/dotnet-oracle-data/`, 캡처 `extracted/dotnet-oracle-captures/`, 기준선 `extracted/dotnet-r0-baseline/`은 Git 제외이며 작업 PC에만 있다. 실행 전후 `originals/` 해시를 비교한다.
- **options.cfg 변경 판별:** 호스트나 원본 게임을 실행한 뒤 추적 파일 `assets/game-data/d/options.cfg`가 바뀌어 있으면 누구의 실행 때문인지 먼저 가린다. 내가 만든 변경일 때만 되돌린다.

<a id="dotnet-ops"></a>

### 6.2 착수·커밋 운영 규칙

1. **착수:** AGENTS.md·이 문서·LEFT_JOBS.md를 다시 읽고 `git status`/`git log`로 다른 세션 변경을 확인한다. dotnetpj는 디컴파일 산출물을 쓰지 않으므로 호스트 확인은 산출물을 직접 읽을 때만 한다.
2. **단위 종료:** 머리말의 기준 단위·다음 단위 번호, 0.2·0.3·0.4와 해당 상세 절을 갱신한다. [2.4](#r4-detail)의 해당 체크 줄을 `- [x]`로 바꿔 단위 번호를 적고, 경과는 `docs/dotnet-*.md` 단위 문서에 둔다(8절에는 쌓지 않는다). 끝난 상세가 길어지면 보관 파일로 옮기고 [7절](#dotnet-done-index) 색인만 갱신한다.
3. **커밋:** 인계 작성 뒤 **이 단위의 파일만** 경로로 지정해 `YYYYMMDD-NN DN 요약`으로 로컬 커밋한다(끝에 Co-Authored-By 트레일러). 커밋 번호는 다른 PC·세션과 겹칠 수 있으므로 `git log`로 당일 마지막 번호를 확인해 이어 쓴다. 같은 작업 트리에서 다른 세션이 cpppj·`LEFT_JOBS.md`를 자주 고치므로 CP/WB 파일·stage를 건드리지 않는다. **push하지 않는다.**
4. **줄바꿈:** `dotnetpj`·`docs`와 `LEFT_JOBS.dotnetpj*.md`는 저장소에 LF로 저장된다(이 문서는 작업 트리도 LF, 보관 파일은 작업 트리 CRLF). 기본 `autocrlf`로 add하고 `-c core.autocrlf=false`를 쓰지 않는다. CRLF 작업 트리 파일은 Edit 도구 대신 정확히 1회 일치하는 바이트 치환으로 고친다.
5. **빌드 환경:** Git Bash에서 `cmake --build … -- /p:…`를 실행하면 `/p:`가 경로로 바뀌어 MSBuild가 실패하므로 PowerShell로 실행한다. `cpp_sync.py` 보고가 cp949 콘솔에서 깨지면 `PYTHONUTF8=1` 또는 `python -X utf8`. 오라클에는 게임 실행 파일만 필요하며 cpppj 전체 빌드는 검사 대상까지 포함해 오래 걸린다.
6. **주석:** 새 코드의 네임스페이스·상수·함수·구조체·클래스·멤버함수·멤버변수·반복문에 한국어 주석을 단다. 주 내용은 설명과 사용법이고, 언제·왜 추가/수정했는지는 그 뒤에 적는다. C#에서는 `/// <summary>`에 무엇을 하는지와 쓰는 방법(인자·반환·호출 조건)을 쓰고 원본 주소·근거 문서·이력은 summary 뒤쪽이나 `<remarks>`에 둔다. 반복문은 바로 위 `//` 주석으로 무엇을 도는지 적는다. 원본 주소는 10.78 주소와 근거 문서(`docs/exe/cpp-*.md`)를 함께 적는다.

## 7. 참고 문서와 완료 범위 색인

<a id="dotnet-done-index"></a>

### 7.1 완료 범위 색인

단위별 상세는 [LEFT_JOBS.dotnetpj.old.md](LEFT_JOBS.dotnetpj.old.md)와 아래 문서에 있다. 마지막 열은 현행 계획에서의 처리다([2.2](#04-기존-dotnetpj-소스의-재수정-판정)). 새 경로의 진행도는 Client 검사와 2.3의 실제 완료 기준을 함께 본다. 0.453 이후 단위는 단위마다 `docs/dotnet-*-2026111[2-5].md` 근거 문서가 있다(47개).

단위별 상세는 [LEFT_JOBS.dotnetpj.old.md](LEFT_JOBS.dotnetpj.old.md)와 아래 문서에 있다. 마지막 열은 현행 계획에서의 처리다([2.2](#04-기존-dotnetpj-소스의-재수정-판정)). 새 경로의 진행도는 Client 검사와 2.3의 실제 완료 기준을 함께 본다. Assets·Core의 기존 경로 검사 수를 새 경로 진행도로 세지 않는다.

| 0.500~0.545 (11-14~15) | 객체 수명 공통층 — AI 본체·기술·에너지 부품(0.500~0.503), Player 화신·공유 장부(0.504), 사제 조회·등록·Pop·낙하·회복·보호막·소리(0.505~0.518·0.522~0.529), 배치 접두·기하·후보 충돌(0.519~0.521), Carrier·피해 전처리(0.523·0.524), cpp 1312/1313 영향 분류(0.530), 종속 해방·삭제 후처리·피격(0.531~0.533), 공통 표시 등록·연소(0.534~0.538), 공통 삭제·풀 반납(0.539·0.540), ProcessHost·Form 해제(0.541·0.542), Take·Create·Unpop 현재 부모(0.543~0.545) | [보관 0.453~0.545](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261115-through-0545)의 8절·R4 진행판, 단위 문서 `docs/dotnet-*-2026111[45].md` | 새 경로 R4(상위 결선은 R4-1~R4-5) |
| 0.546 (11-15) | 현재 Form 부모 체인·내용물 통지 가상표와 World 기본 해제 연결 | [단위 문서](docs/dotnet-form-parent-20261115.md) | 새 경로 R4-1(전체 수명 연결은 잔여) |
| 0.547 (11-15) | 감시 Form의 현재 타입·공장·두 생성·가상 삭제·실제 World 해제 연결 | [단위 문서](docs/dotnet-depend-current-20261115.md) | 새 경로 R4-1(장기 NativeStorage는 잔여) |
| 0.548 (11-15) | 현재 connector·표면 콜백 사본·다리 수명·낙하 진단 부모, 낙하 좌표의 수치 변환 교정 | [단위 문서](docs/dotnet-bridge-current-20261115.md) | 새 경로 R4-1(장기 NativeStorage는 잔여) |
| 0.549 (11-15) | Content Form의 현재 정렬·TYPE 대여·DWORD 뷰·필드·진단·재부착과 contained 생성 마지막 부착 | [단위 문서](docs/dotnet-content-host-current-20261115.md) | 새 경로 R4-1(수명/보상·NativeStorage는 잔여) |
| 0.550 (11-15) | Content 현재 가상표·커서 DWORD·동적 owner/역할/비용과 실제 삭제·보상 수명 | [단위 문서](docs/dotnet-content-lifetime-current-20261115.md) | 새 경로 R4-1(NativeStorage·상위 결선은 잔여) |
| 0.551 (11-15) | Pool 현재 할당·진단·최대50 소진 회수와 실제 World/Host 가상 삭제 대여 | [단위 문서](docs/dotnet-sid-allocation-current-20261115.md) | 새 경로 R4-1(NativeStorage·현재 호스트 전환은 잔여) |
| 0.552 (11-15) | 현재 이웃 TYPE·값 원천·표면 지도 flag8 순회·signed short 캐시·raw 좌표 spot | [단위 문서](docs/dotnet-neighbor-current-20261115.md) | 새 경로 R4-1(Graph·NativeStorage는 잔여) |
| 0.553 (11-15) | Graph 공유 기록/stack·현재 실행·콜백 재진입과 실제 World 물리/로그/시계 | [단위 문서](docs/dotnet-graph-live-20261115.md) | 새 경로 R4-1(NativeStorage 장기 보유·R4-2 결선은 잔여) |
| 0.554 (11-15) | 현재 PostPop 공통 부모·PATCH 몸체·마나 목록 등록과 TYPE/최초 비용/진단 | [단위 문서](docs/dotnet-postpop-current-20261115.md) | 새 경로 R4-1(가상 파생 분배·NativeStorage 장기 보유는 잔여) |
| 0.555 (11-15) | 현재 PostPop 가상 분배·다리/섬/표면/Teleport 접두·Carrier/자산·외곽 pin·실제 World 접두 | [단위 문서](docs/dotnet-postpop-dispatch-20261115.md) | 새 경로 R4-1(미이식 파생·NativeStorage·현재 호스트 전환은 잔여) |
| 0.556 (11-15) | Storage 일반 fresh의 물리 정체성·준비 World 진입·생성 실패/명시 일반 종료 반납 | [단위 문서](docs/dotnet-storage-lifetime-20261115.md) | 새 경로 R4-1(공유 장부 본체·장기 실행기·실제 Runtime pin은 잔여) |
| 0.557 (11-15) | Storage 공유 상태/비용/좌표/깊이·Template 로딩 깊이·장기 Graph/이웃/Owner | [단위 문서](docs/dotnet-storage-state-20261115.md) | 새 경로 R4-1(장기 공장/표시/공간·Native 장치/실제 Runtime pin은 잔여) |
| 0.558 (11-15) | 현재 뷰 원문별 조회·Storage 장기 표시/짧은 World 장치·호출 pin·실제 재사용/반납 | [단위 문서](docs/dotnet-storage-display-20261115.md) | 새 경로 R4-1(공장/공간·Native 장치/실제 호스트는 잔여) |
| 0.559 (11-15) | 현재 건물 공간/진단·Storage 장기 공장/건물/Unpop·기본 종속/현재 종단과 World 재사용 | [단위 문서](docs/dotnet-storage-creation-20261115.md) | 새 경로 R4-1(Native 파생 내용물/전송·나머지 공간·호스트는 잔여) |
| 0.560 (11-15) | Storage 장기 전송·명시 Native 장치/현재 typed·Pop/Process 대여·호출 pin·World 반납 뒤 Form 전송 | [단위 문서](docs/dotnet-storage-transmission-20261115.md) | 새 경로 R4-1(실제 Client 장치/나머지 공간·호스트는 잔여) |
| 0.561 (11-15) | 장기 공간 PostPop/Pop·프레임·연결/표면·현재 생산/중립 소유자·실제 World 재사용 | [단위 문서](docs/dotnet-storage-space-20261115.md) | 새 경로 R4-1(최초 자산/비용·Native 장치/파생·호스트는 잔여) |
| 0.562 (11-15) | 현재 입력 자산의 독립 값 사본·명시 최초 Source/로딩 비용·World 재대여 보존 | [단위 문서](docs/dotnet-storage-assets-20261115.md) | 새 경로 R4-1(실제 Client 자산뷰·Native 장치/파생·호스트는 잔여) |
| 0.478~0.499 (11-12~14) | L2 기준선 확장 — 6번째~26번째 경로의 cpp 기준선·dn 덤프와 구조 대조, 담긴 섬 판정(0.484), 34경로 같은 시드 일괄 수집·도구(0.499) | [L2 일괄](docs/dotnet-l2-batch-20261114.md), 같은 보관본 | 새 경로 R4(M1 판정 자료) |
| 0.453~0.477 (11-12) | 지형 바인딩 조립·Generate 라이브·난수 단일 스트림·시작 1회 reseed(0.453~0.465), Pop 공간 등록·요새 객체 적재·그래프 재구성(0.466~0.470), 장부·전투섬·첫 지역 귀속 확정과 Player 월드 갈고리(0.471~0.477) | 같은 보관본 | 새 경로 R4 |
| 기간·단위 | 완료 범위 | 근거 문서 | 처리 |
| --- | --- | --- | --- |
| 0.536 (11-15) | 연소 실제보호캡처/프레임·출력/삭제·실제프로세스·공통dirty | [근거](docs/dotnet-burning-frame-system-20261115.md) | 새 경로 R4·제품 연결 잔여 |
| 0.535 (11-15) | 연소 생성/입자·전체연소점·현재대상/SID상세진단 | [근거](docs/dotnet-burning-constructor-sources-20261115.md) | 새 경로 R4·프레임/제품 연결 잔여 |
| 0.534 (11-15) | 공통 표시 등록·수집/건설 버퍼소멸·실제Host/Form/Kernel 수명 | [근거](docs/dotnet-render-lifetime-20261115.md) | 새 경로 R4·파생/제품 연결 잔여 |
| 0.533 (11-15) | burning·일반 피격의 실제 HP/현재 진단·양판본 폭·VT 분배 | [근거](docs/dotnet-damageable-hit-current-20261115.md) | 새 경로 R4·상위 결선 잔여 |
| 2026-09-27~10-05 | MonoGame 기반 초기 C# 구현(메인 메뉴·캠페인 1-1/1-2·다리·사제·전투·희생·덱 등 녹화·팬게임 기반), `dotnetpj/` 이동 | [보관 파일 14절](LEFT_JOBS.dotnetpj.old.md#legacy-dotnet-history) | 기존 경로(대부분 교체) |
| 2026-10-07~10-13 | 자료 계층 정정, 다리·표면·그래프·SID·점유·표시 경계 계산, `.chfnt`·커서 표·배경 GIF 색·미션 시작 값, 9-1 A 첫 단계·화면 루프·시계, 도움말·안내창·단계 프레임, 한국어 명칭 | [10-07](docs/dotnet-reconstruction-20261007.md)·[10-09](docs/dotnet-reconstruction-20261009.md)·[10-10](docs/dotnet-reconstruction-20261010.md)·[frame-parity](docs/dotnet-frame-parity-20261011.md)·[loop-parity](docs/dotnet-loop-parity-20261012.md)·[clock-kernel](docs/dotnet-clock-kernel-20261012.md)·[session-clock](docs/dotnet-session-clock-20261012.md)·[dialog-frames](docs/dotnet-dialog-frames-20261013.md)·[stage-frames](docs/dotnet-stage-frames-20261013.md) | 판독기·시계는 재대조, 계산은 `Original`로 흡수, 나머지 교체 |
| 2026-10-15~10-23 | 감사 D01~D16, W01 동기화 도구, W02·W03 원본 월드 코어(슬롯 풀·생성자·Pop/Unpop/PostPop·그래프·삭제·Damageable·Form·내용물 등), W05 선행(SP 저장소·보상), 배포 스크립트 | [감사](docs/dotnet-cpp-audit-20261015.md)·[이식 계획](docs/dotnet-cpp-port-plan-20261016.md)·[W02 첫 조각](docs/dotnet-original-core-20261016.md) 외 `docs/dotnet-original-core-*.md`·[SP 저장소·보상](docs/dotnet-original-core-spstore-reward-20261023.md) | 유지. R4 세션 조립 |
| R0~R2 (10-28~29, 0.13~0.28) | cpppj 오라클·재대조 영향표·PORT_MAP, `Netstorm.Client` 8비트 Screen·입력 큐·캡처·스크립트 도구·`cpp_frame_compare.py`·MonoGame 출력 호스트, R0-4 코어 보강, 팔레트·VFX·색 표·SpriteDraw | [영향표](docs/dotnet-cpp-resync-20261028.md)·[PORT_MAP.md](dotnetpj/PORT_MAP.md)·[R1 문서](docs/dotnet-client-screen-20261028.md)·`docs/dotnet-r04-*.md`·[SpriteDraw](docs/dotnet-spritedraw-20261029.md) | 새 경로 |
| R3a~R3k (10-29~31, 0.29~0.65) | Gump 기반·트리·수명·사건/입력·프레임·클립·배경·글꼴·버튼, StyleText·Slider·메뉴, 문자 입력·Console·Image·MetaImage, 배경 GIF·HTML·도움말·스크립트 명령·Config·MissionScript | `docs/dotnet-gump-*.md`·`docs/dotnet-style*.md`·`docs/dotnet-menu-*.md`·`docs/dotnet-text-entry-*.md`·`docs/dotnet-html-*.md`·`docs/dotnet-quick-help-*.md`·`docs/dotnet-config-*.md` 등 | 새 경로 |
| 0.66~0.127 (10-31~11-01) | 미션 조립 부품, 호스트 입력·타이틀·메인 메뉴, 공통 트리·HTML 창, 기존 경로 `Netstorm.exe` 이름·배포(0.96), 통합 표시·메뉴 입력, Tutorial Init·초기화, 호스트 Help·Options·옵션 저장, Edit 맵 목록·이름·작업본 | [보관 0.66~0.126](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261102-through-0126)·[보관 0.127~0.154](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261104-through-0154) | 새 경로(L1 일부). 실제 조립은 R4 |
| 0.128~0.155 (11-01~07) | DataManager·Template 저장소 TS0~TS6, Fort 타입 표·적재·저장 F1~F2c-12, 11.01 표기·Credits(0.150), 외부 접속 문서화(0.151), 멀티플레이 사전 구현(0.152) | [보관 0.127~0.154](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261104-through-0154)·[Template 저장소 계획](docs/dotnet-template-storage-plan-20261101.md)·[외부 접속](docs/dotnet-external-endpoints-20261103.md) | 대부분 L1, 세션 연결은 F3 |
| 0.155~0.279 (11-07~10) | 지형·섬 생성, 다리 연결, F2d Player·캠페인 월드, 서피스 월드 기반·프로세스·호스트, 월드 본체 조립, F3 세션 껍데기·Stub | [보관 0.154~0.279](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261110-through-0279) | 새 경로(L1·port) |
| 0.280~0.341 (11-10~11) | 재개 준비(재기준선·영향 확인·재반영 2건·`session-dump`·고정 시각), F2c 정상 생성, 섬 전수 재생, F3 본체·세션 소유 상태·AI 관문·동맹·채팅·Player 식별자, LAN 사전 구현, 전체 회귀(0.329) | [보관 0.280~0.341](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261111-through-0341), 요약 [2.4](#r4-detail) | 새 경로(port 위주) |
| 0.342~0.379 (11-11) | 11.01 `Copyright 2026 HJOW` 반영(0.342)과 cpppj 영향 재검토 31항목 전수 처분 — 정적 분류(0.343), 10차 NB 수정 재대조(0.344~0.348 등), 결정 D1에 따라 앞당긴 요구사항 코드의 신규 이식(11차 `AiDifficulty` 0.347, 6차 확장 문자 부호화·`DisplayTextInput` 0.350·0.351, 5차 `UserDataRoot` 0.359, 7차 `ErrorLog`·호스트 배선 0.360·0.361), 구간 대조(0.368·0.370~0.378), 전체 회귀(0.369), 범위 종결(0.379) | [보관 0.342~0.452](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261112-through-0452), [재검토 문서](docs/dotnet-cpp-resync-20261111.md) | 새 경로(port 위주) |
| 0.380~0.389 (11-11~12) | 오라클 갱신, L4a 일괄 도구 `l4a_compare.ps1`, 메뉴 12장면 차이 0 — 돌 버튼 높이 교정(0.380), 구름 Kencloud 이식(0.381), 모달 게이트(0.382), Help·Options 실제 행·앵커(0.383·0.384), Edit 목록 무제한(0.386), 해상도 10행(0.387), 남은 장면 확정(0.389) | [L4a 문서](docs/dotnet-l4a-menu-cloud-20261111.md), [보관 0.342~0.452](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261112-through-0452) | 새 경로 |
| 0.390~0.452 (11-12) | R4 실제 호스트·월드 조립 착수 — thewarbegins 기준선(0.390)부터 섬 부모 조립(0.452)까지, 묶음별 요약은 보관본의 R4 진행판 | [보관 0.342~0.452](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261112-through-0452) | 새 경로(진행 중) |

### 7.2 참고 문서

| 주제 | 문서 | cpppj 기준 구현 |
| --- | --- | --- |
| **cpppj 현행 상태·계약·사용자 결정** | [LEFT_JOBS.md](LEFT_JOBS.md)(0.4 단계별 현황·1절 완료 묶음·4.1 계약·5절 실행 제외) | — |
| cpppj 실제 Session 덤프·고정 시각 실행(L2·L3 오라클) | [cpp-session-state-dump-20261109.md](docs/exe/cpp-session-state-dump-20261109.md) | `client/SessionStateDump`, `client/ClientTimeSource`, `app/main.cpp` |
| cpppj의 다른 프로젝트 요청 처리·재반영 대상 | [cpp-webpj-requests-20261109.md](docs/exe/cpp-webpj-requests-20261109.md) | `client/UberGump.cpp`, `client/RawMenuControls` |
| **AGENTS 4~14차의 cpppj 구현**(14차는 1301 엔진 탑재까지, 연동 범위 D7 대기) | [1.3](#dotnet-r8-requirements) 표의 근거 열 | 1.3 표의 소스 열 |
| cpppj 11.01 표기·Credits·카피라이트 / 외부 접속 목록 | [cpp-product-1101-20261101.md](docs/exe/cpp-product-1101-20261101.md), [cpp-external-services-20261101.md](docs/exe/cpp-external-services-20261101.md) | `client/ProductPresentation.cpp` |
| cpppj CD 10.72(dotnetpj 비대상, 게이트 영향만) | [cpp-cd1072-startup-and-gates-20261111.md](docs/exe/cpp-cd1072-startup-and-gates-20261111.md) | `Patch1078` 게이트 |
| cpppj 영향 재검토(제품 1247→1300) 31항목 처분 | [dotnet-cpp-resync-20261111.md](docs/dotnet-cpp-resync-20261111.md) | — |
| **L4a 첫 비교(메인 메뉴)·구름 Kencloud 이식** | [dotnet-l4a-menu-cloud-20261111.md](docs/dotnet-l4a-menu-cloud-20261111.md) | `o/RawKencloud`·`RawKencloudRender`, `client/ClientKencloudGlobals`·`ClientMain.cpp` 팔레트 전환 |
| 8비트 화면 출력·전체화면 호환 | [cpp-modern-fullscreen-20261027.md](docs/exe/cpp-modern-fullscreen-20261027.md), [cpp-screen-resolution-menu-20261027.md](docs/exe/cpp-screen-resolution-menu-20261027.md) | `client/Screen.cpp`, `client/ScreenPresentation.cpp`, `client/ClientMain.cpp` |
| cpppj → dotnetpj 이식 계획(W01~W20, 8절 요청) | [dotnet-cpp-port-plan-20261016.md](docs/dotnet-cpp-port-plan-20261016.md) | `client/RawCampaignSession`, `client/RawSurfaceWorld` |
| 원본 월드 코어 W02 첫 조각 | [dotnet-original-core-20261016.md](docs/dotnet-original-core-20261016.md) | `o/SidPool.cpp`, `o/SquidFactory.cpp`, `o/TypeConstructors.inc`, `o/DerivedConstructors.inc` |
| Template 저장소 이식 계획(TS0~TS6, F1~F3) | [dotnet-template-storage-plan-20261101.md](docs/dotnet-template-storage-plan-20261101.md) | `client/CampaignTemplateStorage` |
| dotnetpj 외부 접속 문서 | [dotnet-external-endpoints-20261103.md](docs/dotnet-external-endpoints-20261103.md) | — |
| 반영 점검 D01~D16 | [dotnet-cpp-audit-20261015.md](docs/dotnet-cpp-audit-20261015.md) | — |
| 표시 이름·내부 타입명 대응 | [unit-source-name-map.md](docs/unit-source-name-map.md) | `o/RiftTypeTable.cpp` |
| 데모·캠페인 대응 | [demo-campaign-map.md](docs/gameplay/demo-campaign-map.md) | `o/RawMissionStart` |
| 설정 조회·치환·저장 | [cpp-config-reconstruction.md](docs/exe/cpp-config-reconstruction.md), [cpp-options-reconstruction.md](docs/exe/cpp-options-reconstruction.md) | `o/Config.cpp`, `o/ConfigInterface.cpp` |
| 타입 표·요새 파일 | [cpp-fort-reconstruction.md](docs/exe/cpp-fort-reconstruction.md), [cpp-assets-reconstruction.md](docs/exe/cpp-assets-reconstruction.md) | `o/RiftTypeTable.cpp`, `o/Template.cpp` |
| 다리 표·추첨·열린 방향·수명 | [cpp-bridge-reconstruction.md](docs/exe/cpp-bridge-reconstruction.md), [bridge-pieces.md](docs/exe/bridge-pieces.md) | `o/Bridge.cpp`, `o/CanonDecoder.cpp` |
| 표면 이웃·연결·붕괴 방문 / 그래프 | [cpp-surface-reconstruction.md](docs/exe/cpp-surface-reconstruction.md), [cpp-graph-reconstruction.md](docs/exe/cpp-graph-reconstruction.md) | `o/SquidFinder.cpp`, `o/Graph.cpp`, `o/RawGraph.cpp` |
| 점유 비트·해시 / SID / 첫 등록 | [cpp-hash-reconstruction.md](docs/exe/cpp-hash-reconstruction.md), [cpp-spatial-reconstruction.md](docs/exe/cpp-spatial-reconstruction.md), [cpp-sid-reconstruction.md](docs/exe/cpp-sid-reconstruction.md), [cpp-postpop-reconstruction.md](docs/exe/cpp-postpop-reconstruction.md) | `o/SquidHash.cpp`, `o/SquidSpatial.cpp`, `o/SidPool.cpp`, `o/SquidPostPop.cpp` |
| 그리기 순서·글꼴·커서 / 메뉴·브리핑 | [cpp-renderer-reconstruction.md](docs/exe/cpp-renderer-reconstruction.md), [cpp-display-reconstruction.md](docs/exe/cpp-display-reconstruction.md), [cpp-menu-reconstruction.md](docs/exe/cpp-menu-reconstruction.md) | `client/Renderer.cpp`, `o/SquidDisplay.cpp`, `client/UberGump.cpp`, `client/Gump.cpp` |
| 지형·시작 값·조작 / 메인 루프 | [cpp-world-reconstruction.md](docs/exe/cpp-world-reconstruction.md), [main-loop.md](docs/exe/main-loop.md) | `o/TerrainBuilder.cpp`, `client/ClientMain.cpp` |
| 판본 비교(10.37·10.62·10.82·V12) | [cpp-reference-versions.md](docs/exe/cpp-reference-versions.md) | — |
| C# 규칙 코어의 현재 범위와 근사 목록(기존 경로) | [core-rules.md](docs/core-rules.md) | — |
| cpppj 소스 맵·디컴파일 신뢰도 | [SOURCE_MAP](cpppj/SOURCE_MAP.md), [decompile-reliability.md](docs/exe/decompile-reliability.md) | — |

<a id="dotnet-unit-records"></a>

## 8. 단위 기록

이 절에는 단위 기록을 쌓지 않는다(2026-11-15 정리). 단위의 범위·구현·검증·남은 경계는 `docs/dotnet-<주제>-<날짜>.md` 단위 문서와 커밋 메시지에 남기고, 이 문서에서는 [2.4](#r4-detail)의 체크 줄에 단위 번호만 적는다. 0.453~0.545의 기록 93건은 [보관본](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261115-through-0545), 그 이전은 같은 파일의 앞선 보관본에 있다.

## 9. AGENTS 반영 기준과 이관 기록

<a id="agents-reflection"></a>

### 9.1 AGENTS.md 반영 기준 (재확인 2026-11-15, 마지막 AGENTS 변경 `c17aeede1`)

AGENTS.md는 수정하지 않는다. dotnetpj에 닿는 사용자 변경은 모두 현재 계획에 반영했다.

| 사용자 변경 | dotnetpj 반영 |
| --- | --- |
| `ef0c6b91` 실행 파일 이름 `Netstorm.exe` | 기존 경로 0.96 적용. 결정 D2: R5에서 새 호스트로 넘기고 기존 경로는 `NetstormLegacy`로 내린 뒤 삭제 |
| `462c9a64` 11.01 표기·Credits 3항목 | 0.150 완료(새 경로·기존 경로) |
| `0190ba95` 외부 접속 문서화·유효성 검사 비활성화 | 0.151 완료, 이식 때마다 갱신(3.2) |
| `5867f35e`·`990a3541` 4차 로컬 MCP(전체 조작·캡처·소리 포함 녹화)·와이드 최대 1920×1080 | [1.3](#dotnet-r8-requirements) 4차, 결정 D3(dotnetpj에도 MCP) |
| `98b2f20c`·`aaafbbc4` 시나리오 편집 추가·5차 확장(자동 생성 파일 전부 사용자 홈, 읽기 전용 설치) | 1.3 5차·9차 |
| `851054a8`·`3051892a` 기존 파일 재활용·README 개요 보호 | 3.2 |
| `b5197d48` 알려진 버그 34건(현 10차) | 1.3 10차. 옛 0.8 7번은 아래 범위 확대로 해소(0.5 아래) |
| `551d27ae` 세 프로젝트 평가 기준 | 0.1(실행·플레이 가능 여부와 원본 동일성) |
| `75e4eed9` 글꼴 exe 내장 금지·exe 옆 `fonts/`·일본어 등 다른 글꼴 | 3.2·5절 2·7번 |
| `8136df49e`·`b5117f524`·`8661e1060` 7차 오류/예외 상세 로그 신설(로그 경로·`yyyyMMddHHmmss.log`·원인 추적 정보)과 차수 재번호, Linux `$HOME/.netstorm` | 1.3 7차(옛 번호 대응은 1.3 머리), Linux 경로는 결정 D5 |
| `22a0877c3` webpj 예외(8·9차 등) | webpj 전용이라 dotnetpj 변화 없음 |
| `60226ea93` 11차 난이도·12차 스커미시·13차 기타 언어, dotnetpj 범위 **4~13차**, `Copyright 2026 HJOW` | 1.3 11~13차, 카피라이트는 0.342 완료 |
| `610670750` cpppj 플랫폼 명시 | cpppj 전용 |
| `85551c6c2` 14차 QuickJS 추가, dotnetpj 범위 **4~14차**, webpj 예외 변경(4차 WebMCP·5차 localStorage·14차 불필요·12차 대상) | 1.3 14차(cpppj 1301 탑재 계약을 같은 계약으로), 0.2 R8·0.4 순서 5 범위. webpj 예외 변경은 dotnetpj 변화 없음 |
| `c17aeede1` dotnetpj·webpj는 디컴파일 소스 분석이 필요하지 않음(2026-11-15) | 0.3 디컴파일 행, [6.2](#dotnet-ops) 1번. 착수 때 디컴파일 호스트를 확인하지 않는다 |

<a id="dotnet-archive-log"></a>

### 9.2 이관 기록 — LEFT_JOBS.dotnetpj.old.md

**2026-11-15 이관(HJOW-Athlon, 문서만, 사용자 요청 — 인계 문서 정리와 cpppj 소스 기준 계획 재수립):** 정리 직전(`0b54c5d96 / 20261115-50 DN`) 전체 원문을 [보관 파일 끝](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261115-through-0545)에 그대로 보관했다(원문 SHA-256 `233ac2c58a351e9802088a5689c1016b48672df5b2ff82367052d475c04bd1b8`, 보관본을 LF로 되돌려 원문과 바이트 일치 확인). 보관 파일 머리의 보관본 목록에 링크를 더했다.

- **본문에서 뺀 것:** 단위 기록 0.453~0.545(93건) 전체, 0.4 R4 진행판의 묶음별 경과 표·조사 확정 목록·cpp 기준선 나열·남은 조각 1~8의 단위별 이력, 0.3의 단위별 검사·fixture·PORT_MAP 갱신 나열, 0.2 R4 행의 단위 나열, 0.4의 끝난 순서 1-1·1-2 행과 완료한 준비 작업, 2.4의 완료 묶음 목록(7.1 색인으로 대체), 7.1 머리의 0.527~0.532 낱줄.
- **새로 넣은 것:** 0.2 단계 진행판(R0~R11, 단계별 남은 cpppj 단위·줄), 2.4 단계별 남은 작업 상세(R4-1~R4-8·R5-1~R5-5·R6-1~R6-8·R7-1~R7-4와 단계별 체크 목록), 2.7 영역별 집계, 집계 도구 `tools/port_inventory.py`, 0.4 진행 원칙의 단위 크기·진행 표시 규칙, 7.1 색인 3행(0.453~0.545), 9.1의 `c17aeede1` 행.
- **바로잡은 것:** 이식 범위 집계(2026-11-11의 "584파일·약 6.0만 줄 36%" → 단위 기준 406/1,237단위·75,329/169,017줄), 적재 중 postPop(소유자 스탬프·그래프 번호·장부·농지)의 담당 단계를 R6에서 R4-2로(M1 판정에 필요), 단계 번호(60/120프레임·Linux·LAN을 R9·R10·R11로 명시), 1.1의 cpppj 현황(1313·M8 완료·목표 일시 중지·자동 플레이 오라클), 0.3·6.2의 디컴파일 호스트 확인 문구(AGENTS `c17aeede1`).
- 빌드·검사·게임 실행·디컴파일은 하지 않았다. `LEFT_JOBS.md`·cpppj·AGENTS.md는 수정하지 않았다.

**2026-11-12 이관(HJOW-Athlon, 문서만, 사용자 요청):** 단위 기록 0.342~0.452와 0.2~0.4의 단위별 나열을 정리했다. 정리 직전(`315be9a59 / 20261112-72 DN`) 전체 원문을 [보관 파일 끝](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261112-through-0452)에 그대로 보관했다(원문 SHA-256 `654c0ee6c07e42b3059e7f644bf01fc5e4e8011a9250b3d775efe456a7d9b2ac`, 보관본을 LF로 되돌려 원문과 바이트 일치 확인). 보관 파일 머리의 보관본 목록에 새 링크를 더했다.

- **본문에서 뺀 것:** 단위 기록 0.342~0.452(111건) 전체, 0.3 '검사' 행과 0.4 1-1 행의 단위별 집중 검사 나열(0.345~0.452), 0.4 순서 2 행의 단위별 진행 이력(0.390~0.452)과 순서 3 행의 L4a 장면별 경과(0.380~0.389), 0.3 PORT_MAP 행의 행별 `base` 갱신 이력, 0.2 R4 행의 단위 나열, 0.4 1-3·1-4 행(완료 참고로 줄임), 9.2의 지난 11-11 이관 3건 상세(아래 요약).
- **새로 넣은 것:** 0.1 한 줄 요약, [0.4 R4 진행판](#r4-board)(묶음별 연결 현황·cpp 기준선·검증 방법·남은 조각 순서), 0.3의 전체 회귀 주기 도래 표시, 0.4 1-2의 남은 9건 목록, 7.1 색인 3행.
- **바로잡은 것:** cpppj 기준 제품 1301 → 1303, 14차 'cpppj 미착수' 표기(1.1·1.3·7.2·9.1) → 1301 엔진 탑재·연동 범위 D7 대기, PORT_MAP 행 수(691줄 → 688행 재집계, 0.394 `Player.cpp/.h` 반영), 0.2 R0·R8 행(재검토 종결·앞서 이식한 요구사항 코드), 1.2에 1302·1303 정적 확인 추가, 2.4·2.5의 낡은 진행 문장.
- 빌드·검사·게임 실행·디컴파일은 하지 않았다. `LEFT_JOBS.md`·cpppj·AGENTS.md는 수정하지 않았다.

**2026-11-11 이관 3건(요약, 원문은 [보관 파일 0.341까지](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261111-through-0341)):** ① 1300 반영 정리 — 단위 기록 0.280~0.341을 빼고 0.4 남은 작업 순서표·0.5 결정 표·1절 cpppj 마감분 반영·0.1 이식 범위 집계·9.1 AGENTS 반영 표를 새로 만들었다(원문 SHA-256 `698670bf211d9ec89d63ae2ffca976e9c28e87dd52c74288fb1b2ef58d785209`). ② 같은 날 결정 반영 — 사용자가 D1~D5를 정했고(D1·D3~D5 권고안, D2 "새 경로가 이름을 이어받고 작업이 끝나면 기존 경로 제거") 0.5와 관련 문장을 고쳤다. 전환 시점(R5)과 기존 경로 삭제 시점(R6)은 이 결정에 대한 AI 해석이다. ③ 같은 날 AGENTS 14차 반영 — `85551c6c2`(dotnetpj 범위 4~14차)를 머리말·0.2·0.4·1.1·1.3·3.2·7.2·9.1에 반영했다.

**이전 이관(요약, 상세는 각 보관본 머리):** [2026-11-10 — 0.279까지](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261110-through-0279)(HJOW-Athlon, AGENTS 8차 시나리오 편집·5차 확장·9차 알려진 버그·글꼴 외부 배포·평가 기준 반영), [2026-11-04 — 0.154까지](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261104-through-0154)(HJOW-Athlon), [2026-11-02 — 0.126까지](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261102-through-0126)(HJOW-Athlon), [2026-10-31 — 0.73까지](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261031-through-073)(HJOW-X3D), 2026-10-28 재수립 이관(VM-W11-Codex: 2026-10-22 이전 계획, 날짜별 완료·인계 절, 옛 0~11·14절).

**앵커:** 2026-10-28에 옮긴 절의 `<a id>`(`dotnet-current-plan-20261022`·`sid-session-plan`·`legacy-dotnet-history`·`legacy-dotnet-031`~`107` 등)는 보관 파일에 같은 이름으로 남아 있다. `LEFT_JOBS.old.md`·`LEFT_JOBS.webpj.md`·`docs/`의 기존 링크가 이 안내로 오도록 같은 이름의 빈 앵커를 바로 아래에 둔다. 그런 링크로 들어왔다면 보관 파일의 같은 앵커를 본다. 옛 절 제목 앵커(`04-…`·`05-…`·`07-…`·`08-…`·`09-…`)는 이 문서의 대응 절 앞에 남겼다.

<a id="018-단위-기록--r0-4-네-번째-묶음-사제-panic기도와-회복-허용-2026-10-28-hjow-athlon"></a><a id="dotnet-current-plan-20261022"></a><a id="legacy-dotnet-031"></a><a id="legacy-dotnet-032"></a><a id="legacy-dotnet-033"></a><a id="legacy-dotnet-038"></a><a id="legacy-dotnet-039"></a><a id="legacy-dotnet-040"></a><a id="legacy-dotnet-041"></a><a id="legacy-dotnet-042"></a><a id="legacy-dotnet-043"></a><a id="legacy-dotnet-044"></a><a id="legacy-dotnet-045"></a><a id="legacy-dotnet-046"></a><a id="legacy-dotnet-047"></a><a id="legacy-dotnet-048"></a><a id="legacy-dotnet-049"></a><a id="legacy-dotnet-050"></a><a id="legacy-dotnet-052"></a><a id="legacy-dotnet-053"></a><a id="legacy-dotnet-055"></a><a id="legacy-dotnet-056"></a><a id="legacy-dotnet-057"></a><a id="legacy-dotnet-058"></a><a id="legacy-dotnet-059"></a><a id="legacy-dotnet-064"></a><a id="legacy-dotnet-065"></a><a id="legacy-dotnet-066"></a><a id="legacy-dotnet-067"></a><a id="legacy-dotnet-068"></a><a id="legacy-dotnet-069"></a><a id="legacy-dotnet-070"></a><a id="legacy-dotnet-071"></a><a id="legacy-dotnet-072"></a><a id="legacy-dotnet-073"></a><a id="legacy-dotnet-074"></a><a id="legacy-dotnet-075"></a><a id="legacy-dotnet-077"></a><a id="legacy-dotnet-078"></a><a id="legacy-dotnet-079"></a><a id="legacy-dotnet-080"></a><a id="legacy-dotnet-082"></a><a id="legacy-dotnet-083"></a><a id="legacy-dotnet-084"></a><a id="legacy-dotnet-088"></a><a id="legacy-dotnet-092"></a><a id="legacy-dotnet-093"></a><a id="legacy-dotnet-094"></a><a id="legacy-dotnet-096"></a><a id="legacy-dotnet-097"></a><a id="legacy-dotnet-098"></a><a id="legacy-dotnet-099"></a><a id="legacy-dotnet-100"></a><a id="legacy-dotnet-101"></a><a id="legacy-dotnet-102"></a><a id="legacy-dotnet-103"></a><a id="legacy-dotnet-104"></a><a id="legacy-dotnet-105"></a><a id="legacy-dotnet-106"></a><a id="legacy-dotnet-107"></a><a id="legacy-dotnet-history"></a><a id="sid-session-plan"></a>
