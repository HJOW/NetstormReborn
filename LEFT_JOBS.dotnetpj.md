# LEFT_JOBS.dotnetpj — dotnetpj(C# + MonoGame) 현행 상태와 남은 작업

**기준 단위 0.468 · 정리 2026-11-12(HJOW-Athlon) · 다음 단위 0.469 · cpppj 기준 제품 1303(14차 QuickJS-ng 엔진 탑재는 1301) · dotnetpj의 마지막 cpppj 영향 검토는 제품 1247(`4d4b3a928`, 이후 1302·1303은 CD 10.72 전용이라 영향 없음) · 전체 목표 미완료 — 지금 어디까지 왔는지는 [0.1](#dotnet-overview), 남은 일은 [0.4](#now), 사용자 결정(D1~D5, 2026-11-11 확정)은 [0.5](#decisions)**

이 문서는 dotnetpj의 **현재 상태·남은 작업·계속 유효한 결정과 규칙**만 둔다. cpppj와 공통 사항은 [LEFT_JOBS.md](LEFT_JOBS.md), webpj는 [LEFT_JOBS.webpj.md](LEFT_JOBS.webpj.md)가 기준이다. 이번 정리(2026-11-12) 직전의 이 문서 전체(단위 기록 0.342~0.452 111건, 0.2~0.4의 단위별 검사·진행 나열)는 [LEFT_JOBS.dotnetpj.old.md](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261112-through-0452)에 바이트 그대로 보관했고, 그 이전 보관본 5개와 0.341 이하 단위 기록도 같은 파일에 있다. [이식 계획](docs/dotnet-cpp-port-plan-20261016.md)의 W 번호·fixture 목록은 근거 목록으로만 쓰고, 순서·구조·완료 기준은 이 문서를 따른다.

**읽는 순서:** 0.1(지금 어디까지 왔나) → 0.4(남은 작업, 특히 R4 진행판) → 0.5(확정된 결정) → 1절(cpppj 마감분 반영과 4~14차 대응표) → 필요할 때 나머지 절.

<a id="dotnet-overview"></a>

## 0. 한눈에 보기

### 0.1 진행 상황 — 실행·플레이 가능 여부와 원본과의 동일성

**한 줄 요약:** 새 경로는 **메뉴 화면이 cpppj와 픽셀까지 같다**(L4a 12장면 차이 0). 미션은 개발 진입 `--mission`으로 **세션·Player·동맹·Zone·가입/Ready·이름·지형 슬롯(1,679행)까지 조립**했지만 **월드(객체·그래프 재구성·장부 집계)는 아직 비어 있어 플레이할 수 없다.** 지금 플레이할 수 있는 것은 근사 규칙의 기존 경로뿐이다.

AGENTS.md의 진행 요약 기준(실행·플레이 가능 여부, 원본과의 유사성)으로 평가한다. dotnetpj에는 실행 경로가 둘이다([2.1](#dotnet-current-plan-20261028) 결정 5).

| 경로 | 실행·플레이 가능 여부 | 원본과의 동일성 |
| --- | --- | --- |
| **기존 경로** `Netstorm.Game`<br>(`Netstorm.exe`, 현재 배포 대상) | **실행·플레이 가능.** 메인 메뉴와 캠페인 1-1·1-2를 근사 규칙으로 플레이할 수 있다. 한국어 선택·와이드 시야 확장·테두리 없는 전체화면이 있다. 새 기능을 넣지 않는 동결 상태다(실행 오류 수정만). | **낮다.** 녹화·팬게임 기반 근사(RGBA 합성, 자체 UI 배치, 근사 세션 `BattleSession`)라 원본 동일성의 기준이 될 수 없다. 새 경로가 같은 범위를 대체하면 삭제한다. |
| **새 경로** `Netstorm.Client` + `Netstorm.Client.Host`<br>(개발 호스트, 아직 배포 안 함) | **메뉴와 미션 진입 일부만 실행.** 타이틀, 메인 메뉴 8버튼, Help·Options(해상도·음량·난이도 펼침 포함)·Credits(11.01 포함), 캠페인 선택, Edit 맵 목록·새 맵 이름까지의 메뉴, 그리고 개발 진입 `--mission <키> --handle <이름>`(세션·Player·동맹·Zone·가입/Ready 조립). **미션 월드(객체·그래프·장부)가 실제 호스트에 조립되지 않아 플레이할 수 없다.** | **부품은 높지만 전체는 미확정.** cpppj를 구조 그대로 직역했고 원본 x86 fixture 236개로 부품을 대조했다(PORT_MAP L1 338행). **화면(L4a): 1024×768 메뉴 12장면이 cpppj와 BMP 바이트 동일**(메인 메뉴·Credits Tell·Credits 항목·Help 팝업·About·Options 팝업·Resolution 펼침·Difficulty 펼침·Campaign 목록·Campaign 하위·Demo 목록·Edit 목록, 0.381~0.388). **상태(L2): cpppj 기준선은 4경로**(thewarbegins·masterofwhirligigs·savetheisland·fragilefortune)를 확보했으나 새 경로 world-1 덤프는 1,955행(슬롯 1,679·플레이어 9, 0.455)이라 cpppj의 수천 행(슬롯 1,807~2,946·객체·프로세스·장부)에 한참 못 미친다. SID 집합·개수·SetRegion·비슬롯 행은 바이트 동일하고 슬롯 내용은 Pop·공장체·프레임·소유자가 다르다(0.455·0.456 분석). 고정 입력열 비교(L3)는 0건. 부품 대조(L1) 통과를 화면·플레이 동일성으로 합산하지 않는다. |

**이식 범위(2026-11-11 집계):** PORT_MAP은 cpppj `src`의 2,303파일 중 584파일, 약 16.8만 줄 중 약 6.0만 줄(36%)을 가리킨다. 행이 있다고 파일 전체를 옮긴 것은 아니다(예: `RawCampaignSession`은 관문·소유 상태 위주로 일부만). PORT_MAP에 없는 나머지 약 10.7만 줄(1,719파일) 중 큰 묶음은 `client/Client*`(ClientMain 등) 6.6천·`RawCampaign*` 4.9천·`Native*`(LAN) 3.7천·`Uber*` 3.7천·`Sound*` 2.7천·`RawCursor*` 1.7천·`Game*` 1.7천·`Editor*` 1.1천·`Hud*` 1.0천 줄, `o/RawAi*` 4.1천·`RawConstruction*` 3.4천·`RawPriest*` 2.4천·`RawCarrier*` 2.2천·`RawGun*` 1.8천·`RawPath*` 1.7천·`RawMission*` 1.6천·`RawBomb*` 1.4천·`RawFence*` 1.1천 줄이다(R4 조립·R5 표시·R6 규칙 범위). 집계 방법은 [1.2](#resync-scope)와 같은 PORT_MAP `cpp` 열 대조다. 이후 0.394에서 `Player.cpp/.h`가 PORT_MAP에 더해졌고(`RawMissionPlayers.cs`, port 2행) 그 밖의 변화는 없다.

### 0.2 단계별 진행 (R0~R8)

| 단계 | 내용 | 이정표 | 상태 |
| --- | --- | --- | --- |
| R0 | 기준선·동기화(cpppj 오라클, PORT_MAP, `cpp_sync.py`) | — | ✅ 완료. cpppj 제품 1248~1300의 영향 재검토도 종결했다(0.379, 0.4 1-2) |
| R1 | 8비트 화면 호스트·입력 큐·오라클 도구(`--ui-script`·`snapshot`·`session-dump`·`--fixed-frame-ms`) | — | ✅ 완료 |
| R2 | 그리기 원시·팔레트·글꼴 | — | ✅ 본체 완료(실자산·세션 연결은 R4·R5) |
| R3 | Gump UI와 메인 메뉴 계열 | D1 메뉴 픽셀 일치 | 🔶 위젯·호스트 대부분 이식. **메뉴 L4a 12장면 차이 0**(0.381~0.388, 장면은 0.1). 남은 메뉴 장면 3건은 사유와 함께 뒤로 미뤘다(Language 자식 CJK는 R8, Multiplayer 섬 화면은 R5·3차, Edit 이동행은 R7). R3 잔여 위젯·소유자는 [2.5](#r3-r2-detail) |
| R4 | 월드 세션 조립 | M1 시작 상태 L2 | 🔶 월드 아래층(Template·Fort·지형·섬·Player·저장소·프로세스·월드 본체)과 F3 세션의 관문·소유 상태 대부분 완료. **실제 호스트 조립 진행 중(0.391~0.467, [R4 진행판](#r4-board)):** 미션 Fort 적재 → 세션 홀더 → Player·SP·AI·동맹 → Zone·가입·Ready → 이름·요새 순번 → 월드 적재 시작 흐름 → 타입·프레임 → 지형·난수 저장소 → 섬 부모 → 입력 결선 → 바인딩 조립·드라이런 → Generate 라이브·덤프 CRT 행·난수 단일 스트림·프리-Generate 소비 확정·프레임 선택 대조·후보 테이블 per-type 대조·SP 353 구성과 동기 순서 확정·SID 단위 per-cell 대조·reseed 시점 근본 원인 확정·호스트 시작 1회 reseed 적용·Pop 공간 등록 live 연결·요새 객체 live 적재까지. **Pop 후속(postPop·표시·그래프)·농지 배치·소유자 후속 지정·월드 본체 조립은 미착수** |
| R5 | 월드 표시·카메라·HUD | D2 진입 화면 픽셀 일치 | ⬜ 미착수 |
| R6 | 플레이 입력·규칙·소리·AI | M2·M3 | ⬜ 미착수 |
| R7 | 미션 전체·Edit·설정·배포 | M4 | ⬜ 미착수(Edit 목록·`options.cfg` 저장 일부는 R3에서 선행) |
| R8 | AGENTS **4~14차**, 60/120프레임, Linux, LAN·Outpost | — | ⬜ 미착수(검증·마감은 이 단계). 단 결정 D1에 따라 일부 요구사항 코드는 앞서 이식했다: 5차 `UserDataRoot`(0.359), 6차 확장 문자 부호화·`DisplayTextInput`(0.350·0.351), 7차 `ErrorLog`·호스트 배선(0.360·0.361), 11차 `AiDifficulty`(0.347), Options의 Difficulty·Resolution 행(0.384·0.387). 즉시 요구인 11.01 표기·Credits 3항목·외부 접속 문서화는 완료, **`Copyright 2026 HJOW` 줄은 0.342에서 반영**(0.4 1-3 완료). LAN은 `Netstorm.Net` 사전 구현 일부(검사 없음) |

### 0.3 숫자로 보는 현재

| 항목 | 값 |
| --- | --- |
| 마지막 구현 커밋 | 0.468(4경로 dn 미션 덤프 신규 생성+paired 구조 대조, 검사 2건 추가). 직전 0.467은 요새 객체 live 적재+집중 검사 5건+paired 재실행 검증 |
| 검사 | **마지막 전체 회귀 완주는 0.453:** Assets 366·Client 1,130·Core 1,945(합계 3,441, 실패 0, 새 기준선). 전체 완주 이력은 0.345(3,273) → 0.369(3,375) → 0.381(3,389) → 0.432(3,422) → 0.453(3,441)이다. 0.454는 Client HostTerrain* 10건, 0.455는 Client 전체 1,134건+Core 지형 52건, 0.456은 Client Host·Dump 69건+SessionStateDump 11건, 0.457은 Client 전체 1,133건+Core 포팅 111건, 0.459는 Client Host·Dump 24건, 0.460은 Core RiftType·FrameChoose·AmbientWind·RawTerrain 33건, 0.461은 Core SpStore·FortMoney·SetPlayerSp 12건, 0.464는 Client 전체 1,134건(신규 공유 SP 1건 포함)+paired 라이브 재실행(예측 일치) 통과(실패 0, 0.458·0.462·0.463·0.465는 조사 단위라 별도 집중 없음 — 0.462·0.464·0.465·0.466·0.467·0.468은 paired 라이브/디스크 대조). 0.465는 revert 확인용으로 깨끗한 전체 리빌드(경고 0·오류 0) 뒤 Client 전체 1,134건 통과(실패 0). 0.466은 빌드(경고 0·오류 0) 뒤 Client 전체 1,139건(신규 Pop 5건 포함, 실패 0)+paired 라이브 재실행(예측 일치) 통과. 0.467은 빌드(경고 0·오류 0) 뒤 Client 전체 1,144건(신규 FortObject 5건 포함, 실패 0)+paired 라이브 재실행(구조+결정성 일치) 통과. 0.468은 게임 코드 무변경(검사 2건 추가·측정 단위)으로 Client 전체 1,146건(신규 순서·null 프로브 2건 포함, 실패 0)+4경로 paired 구조 대조 통과. 단위별 집중 검사 내역과 변이 검출 기록은 8절과 [보관본](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261112-through-0452)에 있다 |
| PORT_MAP | `PORT_MAP.tsv` 688행(L1 338·port 350, 2026-11-12 재집계). 마지막 추가는 0.394 `Player.cpp/.h` → `RawMissionPlayers.cs`(port 2행), 그 앞은 0.381 `RawKencloud*`·`ClientKencloudGlobals` 6행이다. 행별 `base` 갱신 이력(0.344~0.371)과 0.359·0.360 신규 등록 행은 보관본에 있다 |
| fixture | 연결 236개(0.381에서 `kencloud-v2-x86.tsv` 246행·`kencloud-render-x86.tsv` 944행을 Core에 더했다). `cpp_sync.py`는 미반영분 때문에 0종료가 아니다. `reflected-commit`·PORT_MAP floor는 `f95a8db2` 그대로다(영향 재검토를 마친 행만 올린다) |
| cpppj 기준 | 현재 cpppj는 제품 1303 / 11.01([LEFT_JOBS.md](LEFT_JOBS.md) 0.1). dotnetpj의 마지막 영향 검토는 0.281(제품 1247, `4d4b3a928`)이고 재검토 범위는 0.379에서 종결했다. 그 뒤 `cpppj/src` 변경은 1302·1303(CD 10.72 전용)뿐이다([1.2](#resync-scope)). 14차 엔진 본체(`ScriptEngine`·`third_party/quickjs-ng`)는 R8에서 옮긴다 |
| 디컴파일 | 마지막 정밀 디컴파일은 HJOW-Athlon 2026-11-03(R 전용 11-07, 같은 호스트)이다. dotnetpj 이식 근거는 cpppj 소스·fixture·문서라 디컴파일이 필요 없다. 산출물을 직접 읽어야 할 때만 AGENTS.md대로 호스트를 확인한다 |

<a id="now"></a><a id="09-바로-다음-작업"></a>

### 0.4 남은 작업 — 순서대로

체크 대상은 AI 작업이다. 자연 완주·전체 동일성·Windows 11 실기는 AI 작업이 아니며 단계 전환을 막지 않는다([2.3](#stage-criteria)).

| 순서 | 작업 | 할 일과 완료 기준 | 시작 조건 |
| --- | --- | --- | --- |
| **1-1** | **전체 회귀 재완주** | ✅ 0.453에서 완주: Assets 366·Client 1,130·Core 1,945(합계 3,441, 실패 0, 새 기준선. `NETSTORM_DATA=assets/game-data/` 지정, 종료 코드 0) | 완료 |
| **1-2** | **cpppj 영향 재검토(제품 1247→HEAD)** | ✅ 게임 실행 없는 범위 종결(0.379): 31항목 전수 처분(완료 22·부분 1·미착수 8). 정적 분류 0.343, 재대조·신규 이식 0.344~0.367, 구간 대조 0.368·0.370~0.378. 그 뒤 cpppj 변경 1302·1303은 CD 10.72 전용이라 영향 없음으로 정적 확인했다(2026-11-12, [1.2](#resync-scope)). **남은 9건**은 해당 단계에서 파일을 건드릴 때 HEAD 기준으로 처리한다: 1번 `main.cpp`(부분 — 14차 QuickJS 3종 잔여, R8) · 2번 `BitmapFont.cpp`(일본어 글리프, R2·R8 13차) · 3번 `ClientMain.cpp`(호스트 루프, R1·R8) · 12번 `RawCampaignSession`(F3·R4, 난이도·스커미시는 R8) · 13번 `RawMenuControls`(R3) · 16번 `RawSurfaceStorage`·17번 `RawSurfaceWorld`(R4·R6) · 30번 `SquidPostPop`(R4) · 31번 `LocalTcp/UdpTransport`(10단계). 근거: [재검토 문서](docs/dotnet-cpp-resync-20261111.md) | 완료(병행) |
| **2** | **R4 마무리 → M1** | 🔶 **진행 중.** 아래 [R4 진행판](#r4-board)이 현재 위치와 남은 조각·순서다. 완료 기준(M1)은 34경로의 Load/Start 뒤 상태가 cpppj `session-dump`와 같은 것(L2). 세부 범위는 [2.4](#r4-detail) | 1-1과 병행 |
| **3** | **R3 마무리 → 이정표 D1** | 🔶 메뉴 L4a는 12장면 차이 0으로 사실상 끝났다(0.381~0.388, 장면은 0.1). 남은 것: 미룬 메뉴 장면 3건(Language 자식 CJK는 R8, Multiplayer 섬 화면은 R5·3차, Edit 이동행 선택은 R7)과 R3 잔여 위젯·소유자([2.5](#r3-r2-detail)). 메뉴 기본 실행 교체는 4단계에서 미션과 함께 한다(결정 D2) | 2와 병행 |
| 4 | R5 → 이정표 D2, **`Netstorm.exe` 전환** | 월드 표시·카메라·HUD. 34경로 진입 직후와 N프레임 뒤 캡처 차이 0. 이 시점에 메뉴·미션 기본 실행을 새 경로로 바꾸고 `Netstorm.exe` 이름과 배포 대상을 새 호스트로 넘긴다. 기존 경로는 `NetstormLegacy`로 내려 비교·개발용으로만 두고 배포에서 뺀다(결정 D2) | 2·3 뒤 |
| 5 | R6 → M2·M3 | 입력·선택·커서, 게임 메뉴·우클릭 메뉴, 규칙(사제·운반·건설·생산·전투·의식·미션)·AI·소리. 고정 입력열 L3 상태 해시와 대표 장면 캡처 일치. 1-1·1-2가 새 경로에서 같은 수준 이상으로 진행되면 기존 경로(`NetstormLegacy`)와 그 검사를 삭제한다(결정 D2·결정 5) | 4 뒤 |
| 6 | R7 → M4 | 34경로 전체·데모 반복·진행 저장·Retry/Next/Abort, Edit 전체, `options.cfg`(9-1 C·D·F 판정), 배포 스크립트 갱신과 압축 해제 실행 확인 | 5 뒤 |
| 7 | R8 — AGENTS 4~14차 | [1.3](#dotnet-r8-requirements) 표의 차수별 할 일. 결정 D1에 따라 요구사항 코드 상당 부분이 2~6 단계에서 함께 들어오므로 R8은 요구사항 전용 모듈(MCP·FSR 출력기·오류 로그·스커미시 화면·시나리오 편집기·외부 글꼴 출력·14차 QuickJS 엔진 탑재) 이식과 요구사항 검증이 중심이다. 14차는 cpppj가 1301에서 엔진 탑재(결정 D7 ① 범위)까지 끝냈으므로 같은 계약으로 옮기고, 연동 범위는 cpppj D7을 따른다 | 6 뒤 |
| 8 | 60/120프레임 | cpppj F1·F2(옵션 `Frame Rate` 60/75 원본/120·양보 대기). 비행체 표시 보간은 cpppj D2를 따른다 | 7 뒤 |
| 9 | Linux(DesktopGL) | 설정·진행 저장·커스텀 맵·로그는 `$HOME/.netstorm/` 아래 `Saves/`·`CustomMaps/`·`logs/`(결정 D5). Windows 전용 동작은 호스트 OS 분기에만 둔다 | 8 뒤 |
| 10 | TCP/IP 로컬 멀티플레이·Outpost | cpppj 3차(M1~M7)를 로컬 네트워크만으로 옮긴다. MP 실행 검사는 cpppj처럼 사용자 확인으로 둔다 | 최후순위 |
| — | 새 제보 처리 | 사용자가 오류·원본과의 차이를 제보하면 해당 항목을 AI 작업으로 올린다 | 제보 시 |

**완료한 준비 작업(참고):**

- **11.01 카피라이트 ✅ 0.342:** 새 경로 `Netstorm.Client/ProductPresentation.cs`의 `PatchCreditsBody`와 기존 경로 `MainMenuView`의 11.01 Credits 상세에 `Copyright 2026 HJOW`를 넣었다. 기준은 cpppj `client/ProductPresentation.cpp`(제품 1291)다.
- **오라클 갱신 ✅ 0.380:** 오라클은 `cpppj/build/bin/Release/Netstorm.exe`(2026-11-11 11:20:25 빌드, SHA-256 `09042F35637459793784D46D4489B524D8E412C52294F7640FFFFDE5B2A8E996`, 제품 1301 소스와 같은 빌드)다. 데이터 사본 `extracted/dotnet-oracle-data/`(`originals/` 1,394파일 + 클론 `cursors/`)와 `originals/` 해시 기준선 `extracted/dotnet-r0-baseline/originals-hash-before-0380.txt`는 이 PC에만 있다(Git 제외). 0.382 이후의 L4a와 R4 기준선(0.390~)은 CD 10.72 변경(`93a589636`·`b833ecb51`)만 더한 11-12 재빌드 오라클로 실행했고 메뉴·미션 경로에 영향이 없어 비교가 유효하다. cpppj `src`가 더 새로 바뀌면 PowerShell로 다시 빌드한다.

<a id="r4-board"></a>

#### R4 진행판 — 실제 호스트·월드 조립 (순서 2, 0.390~0.468)

M1(34경로의 Load/Start 뒤 상태를 cpppj `session-dump`와 대조)까지의 현재 위치다. **cpppj 기준선 4/34, 새 경로 전체 일치 0/34.** 새 경로의 world-1 덤프는 thewarbegins 기준 2,129행(슬롯 1,853, 0.467)이며, 0.468에서 baselined 4경로 중 나머지 3경로의 첫 실제 dn 미션 덤프(f090/f240)를 생성했다(masterofwhirligigs 1,743·savetheisland 2,618·fragilefortune 2,719슬롯, thunderingpower 구조 참조 2,410슬롯 별도). 월드 본체(객체·그래프 재구성·장부 집계) 행이 없어 어느 경로도 전체 일치할 수 없다. 이 구간 76단위 중 코드를 바꾼 단위는 약 절반(39개)이고, 나머지는 기록자 추적·범위 확정·기준선 확보였다.

| 묶음 | 연결한 것 (실행 호스트 world-1 덤프·집중 검사로 확인) | 단위 |
| --- | --- | --- |
| 미션 Fort | 스크립트 목록·원문 읽기·Template 풀기를 호스트 장치로 연결(thewarbegins 목록이 cpp `inspect-fort`와 일치) | 0.391 |
| 세션 홀더·덤프 | `HostSession`(Kernel·난수·풀 공유)과 world-1 덤프 행(머리·kernel·pool·slot, 저장소 7종) | 0.392·0.393 |
| 플레이어·AI | `MissionPlayers.Load`, 시작 SP, 인공지능 설정 바인딩(600B)·생성 본체(2번 brain), 참가자·동맹 81칸 | 0.394~0.400 |
| 마스크·접속 | 동맹 마스크, Zone 설치, 가입·Ready·Ready packet 원형과 라이브 연결(1번 flags 0x4b) | 0.401~0.407 |
| 요새·이름 | 요새 순번(0x5c·0x60·0x64) 기록, 전투 이름 확정(NUL)과 핸들 버퍼 공유 | 0.420·0.428·0.430 |
| 월드 적재 흐름 | 공유 저장소 묶음 → 확정 갈고리 Load 드라이런 → SetConfig 영속화 확정·연결 → `StartMission`에 `BuildWorldLoad`+`Load` 연결(**Generate·Objects는 stub**) | 0.436~0.440 |
| 타입·프레임 | `.type` 정의 116개 적재, 프레임 188 채우기, 실제 타입 표 연결 | 0.442 |
| 지형 저장소 | 지형 상태 묶음, 지형 전용 난수(`TerrainRandom`)·CRT 상자 묶음, 지역 상자의 세션 승격 | 0.444·0.447·0.448 |
| 입력 결선 | 게임 보조(`TerrainEnabled`·밀도 3종)·`ownerMode.battle`·`bridgeState.mission`·전투 옵션 5·6번(`Layout`·`Spacing`)·구독자·`AiDebug` 결선과 `WorldGeyserSpacing` 표 이식 | 0.453 |
| 바인딩 조립 | 세션 바인딩 조립(`BuildTerrainBinding`: 세션 저장소 묶음+입력 바인딩, 시작 흐름 드라이런)과 기록 효과 Generate 드라이런(일반 분기·도장·결정성) | 0.454 |
| Generate 라이브 | Territory 실바이트·`noRegion=127`·조각 107 실제값 결선, Load.Generate 갈고리→`GenerateOriginal` 라이브 연결(실제 도장+기록 Pop). 슬롯 1,679행 출현·SID 집합 일치·비슬롯 행 무변화 확인 | 0.455 |
| 난수·덤프 분석 | 고정시각 paired 기준선(`extracted/dotnet-l2-fixed/`, cpp f090/f240), world-1 CRT 행을 실제 소비 스트림(`TerrainCrt`)으로. 체인 parity 확정(CRT 296677442 일치) | 0.456 |
| 난수 단일 스트림 | 포팅 모듈(RawSpStore·지형·농지·배치·오브젝트·프레임 선택 등 13종+호스트·테스트)을 공유 `GameRandom`으로 단일화(근거 cpppj 498행). `TerrainRandom`·`_spRandom` 삭제, `HostTerrainRandomTests` 은퇴 | 0.457 |
| 프리-Generate 소비 확정 | dn 측 감사(game0·SP후=advance^353·Generate전후 무변화·덤프=advance^963)·cpp 측 정적 인벤토리·shift 상관(±5000 무)·SP offset +6=2회전·CRT 동일 절차 판정. 프레임 미수렴(78/1679) — 선택 테이블 대조·메뉴 소비량 확정은 0.459 | 0.458 |
| Pop 공간 등록 live | 세션 Pop 장치 결선(`BuildTerrainPopDevices`: 공장·해시·spots·SHP 표시, `BuildTerrainStampHooks` Create/Pop live 분기+중립 보존, `StartMission` SHP 적재·장치 연결, `TerrainDisplayShapes.Build` 블록 순서=자산 순서 정정)과 `HostTerrainPopTests` 5건(중립 보존·개수 거부·공장 vtable·섬 좌표/단계0/체인/spot·프린지 실측 3단계). paired 재실행으로 Pop 중립성 확정(Game·CRT·타입 1526·프레임 80/1679·full 78 동일, pool 바이트만 변경) | 0.466 |
| 요새 객체 live 적재 | 세션 객체 장치 결선(`BuildFortObjectDevices`: 공장·HP·내용물·건물·Pop, `MakeFortObjectLoadHooks` Create/HP/내용물, `StartMission` 객체 장치·Fort/Campaign Objects live 연결, `HostFortObjectLiveTests` 5건(장치 거부·미연결 거부·실제 요새 구조/결정성·Pop 미지원 분류·내용물 전수)). paired 재실행으로 구조 확정(슬롯 1679→1853·cpp_only 261→87, 타입 1628·프레임 123/1853·full 78, Game 2317693072 새 기준·CRT parity 유지). Pop 공간 등록은 R6 postPop·표시·그래프 장치와 함께 잇는다(실측 미지원 vtable 71·75·78·89·122 등) | 0.467 |
| 4경로 dn 미션 덤프·구조 대조 | baselined 4경로 중 나머지 3경로의 첫 실제 dn 미션 덤프(f090/f240) 생성과 SID 단위 대조(masterofwhirligigs 1743·savetheisland 2618·fragilefortune 2719슬롯, 모두 dn_only 0·공통=dn 전체, f240 dn_only [6] 게임플레이 잔여). thunderingpower 구조 참조(2410슬롯, cpp 시드 미일치로 구조만) 별도. 적재 순서 패리티 확정(순서 엄격 대조·내용물 null 전수·부착 경로 검사) | 0.468 |
| 섬 | 섬 런타임 부모 조립(`BuildIslands`: 풀·기록·모드·목록·해시 공유) | 0.452 |

**조사로 확정한 것(코드 변경 없음, 잔여 지도):** Player 9기록·이름 바이트15·SP 오프셋·`+8`/`+0x78`/`0x5c~0x64`·장부 합계·AI 부착 recipe의 기록자 추적(0.408~0.431), 월드 적재(0.435)·지형/객체 효과(0.441)·Generate 진입점(0.443)·난수 출처(0.446)·생성 입력 15종(0.450)의 범위 확정, fragilefortune 2번 SP 불일치는 cpp 쪽 AI 경제 활동이라 시작값 오류가 아니라는 결론(0.451, R6 잔여). 상세는 [보관본](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261112-through-0452)의 단위 기록이다.

**cpp L2 기준선(오라클 `session-dump`, f090·f240):** thewarbegins(0.390: 슬롯 1,941·객체 1,881), masterofwhirligigs(0.433: 3,904행·슬롯 1,807·객체 1,755), savetheisland(0.445: 6,129행·슬롯 2,926·객체 2,831), fragilefortune(0.449: 6,191행·슬롯 2,946·객체 2,848). 산출물은 `extracted/dotnet-l2-baseline/<경로>/`에 있다(Git 제외, 이 PC에만).

**검증 방법:** 개발 호스트로 `--mission <키> --handle Youpson`을 260프레임 실행(종료 코드 0)하고 world-1 덤프를 cpp 기준선과 행 단위로 대조한다. 실행 전후 `originals/` 1,394파일 해시 차이 0을 확인하고, 호스트가 바꾼 추적 파일 `assets/game-data/d/options.cfg`는 내가 바꾼 것인지 가려 되돌린다. 대표 변이를 넣어 검사의 검출력을 확인한 뒤 원복한다([6.1](#dotnet-commands)).

**남은 조각과 순서(0.441·0.450에서 정한 분할):**

1. **전체 회귀 재완주(0.4 1-1)** — 0.453에서 0.453 포함으로 완주한다.
2. **바인딩 조립 설계 → 입력 값 연결 + Generate 드라이런.** ✅ 0.454에서 끝났다(조립·바인딩·드라이런, 덤프 282행 중립).
3. **Generate 라이브 연결 + 덤프 대조** ✅ 0.455에서 1차 연결·대조(슬롯 1,679행·SID 집합·비슬롯 행 무변화), 0.456에서 메커니즘 확정: ① 생성 순서·셀 집합 동일 증명(정렬 152 flips only·Pop 좌표 집합 동일) — 농지 121은 postprocess 배치 산물(동일 셀에 dn은 97, 배치 파이프라인 R6 필요) ② 소유자는 region+1을 적재 시점에 가짐(121은 0, writer는 Objects/AI-setup 잔여) ③ CRT parity 확정(체인 동일 출력 296677442) ④ Game/SP-offset/frame divergence는 단일 공유 스트림 분리가 원인(0.457에서 단일화), SP offset +6=게임플레이 2회전·tracking 미연결은 R6 잔여. 남은 차이: Pop 공간 등록(void·좌표·레벨·체인, vtable Supports·SHP dims·postPop이 발목) · 공장 생성자·vtable · `generate:127` 갱신(의도적 결선 변경, 0.455). 위험 ①(풀 수열)은 해소 — `predictable` 플래그를 그대로 풀에 전달해 예측 영역에 할당한다.
4. **난수 단일 스트림 연결** ✅ 0.457에서 포팅 모듈 13종+호스트·테스트 20건을 공유 `GameRandom`으로 단일화(`TerrainRandom`·`_spRandom` 삭제, `HostTerrainRandomTests` 은퇴). 라이브 덤프 구조 동일(1,955행·슬롯 1,679·SID 집합 동일·비슬롯 행 무변화) 확인. 프레임은 미수렴(78/1679) — 0.458에서 dn 측 소비를 감사로 확정(reseed→SP 353회→Generate 무변화→지형 610회, 합계 advance^353+610)하고 cpp 측 정적 인벤토리를 마쳤다(AI 생성·로그인·플레이어 생성·PlaceAll 무소비, 메뉴 ambient 제외). 레거시(`BattleSession` 등)는 결정 D2 삭제 때까지 `NetstormRandom`을 둔다.
5. **프레임 선택 대조(0.459~0.465)** ✅ 0.459에서 프레임 값 분포 대조로 후보 테이블 차이를 확정: dn 최댓값 0xd0·cpp 최댓값 0x13c(동일 셀 집합·동일 순서·동일 스트림 가정 하에 설명 불가). 0.460에서 per-type 정적 대조로 필터·구조 가설을 좁혔다: `ChooseTerrainFrame` 몸체 동일(cpp `o/RawFrameChoose.cpp` 8~33행 vs dn `RawFrameChoose.cs` 20~82행, NB23 포함), `RiftTypeFrames` 구조 동일(Codes·Find*·Run·LetterKinds), `FrameTable()` 양쪽 클러스터 순서 그대로(cpp `TypeParser.cpp` 225~231행 vs dn `TypeFrameTable`+`FillWorldFrames` 70+순서, 188칸·116자산), dn 호스트 ambient 0회(`Client.Host`에 `UpdateAmbient` 없음) vs cpp 매 프레임 Wind 1~2회(`Next(100)`+10% `Next(65536)`)·Sound 기한 통과 때만 1~2회(`Next(3000)`[+`Next(20000)`]), cpp 덤프는 메뉴 경유(click-if 5/30/60→dump 90, 약 90 메뉴 프레임) vs dn 직접 진입(`--mission`, 0 메뉴 프레임). 0.461에서 reseed-Generate 동기 순서와 SP 353 구성을 확정해 ambient를 프레임 원인에서 제외했다: SP 353회=`Initialize` 33회(`Next(100)` 1+`Next(32)` 32)+활성 소유자 2명의 `Set` 잡음 320회(32비트×5회×2명), 양쪽 모두 reseed→SP→Generate가 한 시작 흐름에 동기 실행되고 프레임 루프(`Update`)와 분리돼 있어 그 사이에 ambient가 낄 수 없다(dn `ClientHostGame` 270~383행 vs `Update` 393행~, cpp `ClientMain` 349행·`RawCampaignSession` 1090행 vs 루프 601~602행). 0.462에서 SID 단위 per-cell paired 대조로 발산 위치를 확정했다: 공통 SID 1,679개(dn_only 0·cpp_only 261=게임플레이 객체 잔여), 타입 일치 1,526개(불일치 153개 중 152개는 dn97 vs cpp121 농지 postprocess·1개는 dn0 vs cpp152, 모두 R6 잔여), 프레임 일치 78개=지형 5개+프린지 72개+기타 1개이며 섬 타입 97은 1,505개 중 0개 일치·프린지 102는 168개 중 72개 일치, 불일치 표본에 +6 오프셋 다수(141→147·149→155·33→39) — 테이블 내용 가설을 검증했으나 파서·난수 알고리즘이 양쪽 동일함을 확인하고, reseed 시점 차이(dn 매 미션 reseed vs cpp 시작 1회 초기화+공유 경로 무재초기화, cpp `--mission` 메뉴 경유 vs dn 직접 진입)를 근본 원인으로 확정했다(아래 8절 0.463 기록). 0.464에서 호스트 시작 1회 reseed+SP 초기화를 적용하고 paired 재실행으로 검증했다: 덤프 Game 2583299428 == 사전 예측 advance^1764(seed) 정확 일치(33+801+320+610, 중간 소비 0 증명), CRT parity 유지, SP offset 44 동일, 프레임 80/1679(여전히 미수렴). 0.465에서 early-dump N(F) 역산으로 sim 가설을 기각하고 sim을 회수했다: 필요 gap ≥ ~3500 draws(기동 3395+f3-f4 1282+적재-전) vs K≤90 ≈ ≤180 draws, 위치도 Generate 전이 아니라 적재 후·적재 사이 인터리브라 불일치. 0.466에서 Pop live 연결 후 paired 재실행으로 Pop 중립성을 확정했다: Game 2583299428·CRT parity·타입 1526·프레임 80/1679·full 78 모두 0.464와 동일하고 pool 바이트만 Pop 기록분 변경됐다. 0.467에서 요새 객체 live 적재 후 paired 재실행으로 구조를 확정했다: 슬롯 1679→1853(+174)·cpp_only 261→87, 타입 1628·프레임 123/1853·full 78, Game 2317693072(새 기준, 객체 RandomFrame·후보 추첨 소비분)·CRT parity 유지. 프레임 수렴은 파이프라인 구현(R4-6: Pop 후속·농지·소유자+기동/메뉴 경로 복원) 뒤 재시도한다. 이후 Objects 라이브 연결(`RawFortObjectLoad` + 갈고리 약 20종, 공장 할당 parity) + 덤프 대조.
6. **장부·그래프·농지·소유자**(`RebuildGraph` 실몸체, totals·totalmade, 농지 배치 파이프라인, 소유자 후속 지정) + 덤프 대조. 선행: 프레임 수렴(5). 요새 객체 Create·소유자·HP·프레임·내용물 부착은 0.467에서 live 연결됐고, 객체 Pop 공간 등록(postPop·표시·그래프)은 R6에서 잇는다.
7. **기타 잔여:** brain 실생성자(현재 스텁 토큰 `0xA1000000+owner`), 요새 이미지 전송, 메뉴에서의 미션 시작(핸들 출처, 이름 꼬리 16~19 바이트 잔여), 전투섬 배정과 `+0x78` 첫 지역(1번 영점), Player 월드 풀·공간 Pop·예약 갈고리, 세션 `Bind*` 대리자의 실제 몸체 연결.
8. **34경로 L2 대조(M1)** — 나머지 29경로의 cpp 기준선을 만들고 새 경로 덤프와 행 단위로 대조한다. 비교 실행에는 `--fixed-unix-seconds`를 양쪽에 동일하게 준다(0.456; 고정 시각만으로는 프레임이 맞춰지지 않고 시드는 같아진다). 0.468에서 baselined 4경로의 dn 미션 덤프를 모두 갖췄고(thunderingpower 구조 참조 별도 1건) M1 판정용 4/34는 유지한다. cpp 기준선 4건의 시드가 dn 고정시각과 달라 Game·CRT 행은 직접 대조하지 않고 구조(SID·타입·소유자·프레임 분포)만 대조했다 — 동일 시드 paired cpp 재실행은 34경로 본 대조 때 묶어서 한다.

**진행 원칙:**

- **단위 크기(권고, 원하지 않으면 알려 달라):** cpppj가 1~13차를 마감해 소스가 안정됐으므로, 지금까지의 "함수 하나 + 검사 몇 건" 단위 대신 **cpppj 파일(모듈) 묶음 단위**로 옮긴다. 검증의 중심은 L2(`session-dump`)·L4a(캡처) 비교로 옮기고, 원본 fixture가 있는 모듈은 계속 L1을 연결한다. 0.330~0.341처럼 실제 조립과 무관한 관문을 하나씩 늘리는 단위는 줄인다.
- 단위마다 이 문서의 0절(0.2·0.3·0.4)과 해당 상세 절을 갱신하고, 작은 단위 기록은 [8절](#dotnet-unit-records)에 남긴 뒤 로컬 커밋한다([6.2](#dotnet-ops)).
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
| MP·LAN | 실제 LAN 접속과 MP 경로 동작(cpppj와 같은 사용자 확인 범위) |

## 1. cpppj 마감분 반영 (2026-11-11)

### 1.1 cpppj 현황과 dotnetpj에 주는 의미

cpppj는 제품 1300에서 AGENTS 1~13차 구현을 모두 마쳤고, 2026-11-11 사용자 커밋 `85551c6c2`가 더한 **14차(QuickJS 엔진 탑재)**도 제품 1301에서 엔진 탑재(cpppj 결정 D7 ① 범위)로 끝냈다. 연동 범위는 cpppj 결정 D7을 기다린다. 이후 1302·1303은 CD 10.72 전용 변경이다. cpppj에 남은 AI 작업은 CD 10.72 복원 C3~C5, M8 최종 관문 재실행, 비행체 표시 보간(F3, 선택)이다([LEFT_JOBS.md 0.1](LEFT_JOBS.md#now)). dotnetpj 범위는 4~14차다.

- **이식 기준이 안정됐다.** 그동안 cpppj가 같은 파일을 계속 고쳐 dotnetpj는 "파일을 건드릴 때 늦게 재대조"했다(0.281). 이제 한 번 재검토(0.4 1-2)한 뒤 HEAD 기준 파일 묶음 단위 이식이 가능하다.
- **요구사항(4~13차)의 해석과 계약이 cpppj 구현으로 확정됐다(14차는 엔진 탑재까지만 확정, 연동 범위는 cpppj D7 뒤).** dotnetpj는 같은 계약을 옮긴다([1.3](#dotnet-r8-requirements)). 옛 문서가 미정으로 둔 해석(MCP 범위·와이드 방식·사용자 홈 범위·Test Battle `.english` 처리)은 cpppj 문서를 따른다.
- **오라클 장치가 갖춰졌다.** `snapshot`(L4a)·`session-dump`(L2)·`--fixed-frame-ms`(L3)·MCP. dotnetpj 쪽 같은 장치는 0.283·0.284에서 만들었다(world 0 덤프까지, 세션 조립에 따라 행을 늘린다).
- **dotnetpj 비대상:** CD 10.72 복원은 cpppj 전용이다(AGENTS가 cpppj에만 둠). 단 cpppj가 C3에서 `Patch1078` 게이트(241파일 306곳)를 판본 분기로 바꾸면, dotnetpj가 이미 옮긴 판본 분기(PATCH/CD) 파일의 재대조가 필요하다. 그 커밋 이후의 영향 검토에서 확인한다. cpppj M8의 ZIP·검증 도구(`tools/verify_cpp_package.py`)는 dotnetpj 배포 검증(R7)의 참고 자료다.
- **cpppj 사용자 결정 중 dotnetpj에 닿는 것:** cpppj D2(비행체 보간) → dotnetpj 60/120프레임 범위, cpppj D3(일본어 본문 번역·다른 언어) → dotnetpj 13차 범위, cpppj D4·D5(알려진 버그 재현 조건·공식) → 수정되면 HEAD로 따라간다, cpppj D6(스커미시 후속) → dotnetpj 12차 범위.

<a id="resync-scope"></a>

### 1.2 영향 재검토 범위 (0.4 1-2의 입력)

| 항목 | 값 |
| --- | --- |
| 지난 검토 | 0.281: `f95a8db2..`(당시 HEAD, 제품 1247) — 자료 629행 중 458행 영향(L1 247·port 211), R4 우선 65행 목록. 재대조는 파일을 건드릴 때 하는 방식으로 미뤘다 |
| 이번 구간 | `4d4b3a928..HEAD`(제품 1247→1300): cpppj 47커밋, `cpppj/src` 184파일 변경(신규 39), 그중 PORT_MAP `cpp` 열에 있는 파일 47개 |
| 주요 변경 묶음 | M6 LAN·Native 등록(1258~1279), M7 5건(helpWord 캐시·메타 맵 커서·희생 안내·오류 로그, 1268~1273), K 한국어(1270·1276·1281), S 시나리오 편집(1277·1282~1284), B 알려진 버그 수정 10건(1285~1294·1300), L 일본어 UI(1288), 11.01 카피라이트(1291), D 난이도(1292), SK 스커미시(1293·1300), F 프레임(1296~1298), M8 배포(1274·1280·1295~1297) |
| 집계 방법 | `git diff --name-only 4d4b3a928..HEAD -- cpppj/src`와 `dotnetpj/PORT_MAP.tsv`의 `cpp` 열 교집합. 행별 `base` 이후 변경은 `git diff --name-only <base>..HEAD -- <cpp 파일>`로 본다(`cpp_sync.py --ports`는 0.147 이후 시간 초과라 쓰지 않는다) |
| 이후 변경(2026-11-12 정적 확인) | 제품 1302(`b833ecb51`: `CdDirectory.cpp/.h` 신규·`ClientMain.cpp` 1줄 — CD 자료 폴더 선택)와 1303(`93a589636`: `main.cpp` 8줄 — `--cd` 판본 옵션)은 모두 CD 10.72 전용이다. PORT_MAP에 있는 `ClientMain.cpp`(재검토 3번)·`main.cpp`(1번)에 닿지만 AGENTS가 CD 10.72를 cpppj에만 두므로 dotnetpj에는 영향이 없다. 두 행의 `base`는 올리지 않는다(두 파일의 잔여는 R1·R8에서 HEAD 기준으로 처리) |

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
| 후순위 | 60/120프레임 | F1·F2 1297·1298 [계획·결과](docs/exe/cpp-post-plan-fps-cd1072-20261111.md)(옵션 `Frame Rate` 60/75 원본/120, 양보 대기로 CPU 점유 감소·프레임 시간 동일) | `client/FrameRateOption` | 0.4 8단계 | 기존 경로에 근사 `FramePacing`만 |
| 후순위 | Linux(DesktopGL) | — | — | 0.4 9단계, 경로는 결정 D5 | 없음 |
| 최후순위 | TCP/IP 로컬 멀티플레이·Outpost(3차) | M1~M7 [LEFT_JOBS.md 1절](LEFT_JOBS.md#done) | `client/Native*`, `src/zacket`, `o/RawPacket*`·`RawConnection*` 등 | 로컬 네트워크만(`LocalNetworkPolicy` 루프백·리슨) | 사전 구현(검사 없음): `Netstorm.Net` 0.152·0.287·0.307·0.321·0.324·0.330~0.333, 세션 에디터 전송 0.293 |

**단계와 무관한 AGENTS 공통 요구:** `Netstorm.exe` 이름(결정 D2: R5에서 새 호스트로 이전), 11.01 표기·Credits 3항목(완료 0.150), `Copyright 2026 HJOW`(0.4 1-3), 외부 접속 문서화·클라이언트 유효성 검사 비활성화(완료 0.151, 새 경로를 옮길 때마다 [dotnet 외부 접속 문서](docs/dotnet-external-endpoints-20261103.md) 갱신), 글꼴 exe 내장 금지·exe 옆 `fonts/`(기존 경로 준수), 배포 스크립트 `package-dotnetpj.ps1`(인자 또는 입력받은 경로·빈 값이면 취소·실행 파일과 필요 에셋 전부 포함 — 현행 준수, 구성이 바뀔 때마다 압축 해제 실행 확인).

<a id="cpppj-requests"></a>

### 1.4 cpppj에 전달할 요청 (이식 계획 8절의 남은 것)

dotnetpj 작업에서는 cpppj·LEFT_JOBS.md를 수정하지 않는다. 아래 요청은 사용자에게 전달한다. 요청 1·2(`session-dump`·고정 시각 실행)는 cpppj `c91d9b84`와 dotnetpj 0.283·0.284로 해결됐다.

| 번호 | 요청 | 상태(2026-11-11) |
| --- | --- | --- |
| 3 | fixture TSV 첫 주석 줄에 열 이름 | 남음 |
| 4 | 모듈별 연결 상태표(독립 대조만 / 실제 Session 연결 / GUI 연결) | 남음. cpppj [SOURCE_MAP](cpppj/SOURCE_MAP.md)로 대신할 수 있는지 0.4 1-2에서 확인 |
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
| `Core/Original/*` | 유지. 영향 재검토(0.4 1-2) 뒤 세션 조립에 쓴다 | 대응 `o/*` | R0·R4 |
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

### 2.4 R4 진행 상세 (0.4 2단계)

현재 위치·남은 조각·순서는 [0.4 R4 진행판](#r4-board)이 기준이다. 여기는 완료 묶음과 남은 항목의 목록이다.

**완료 묶음** (단위별 상세는 보관본의 단위 기록과 [7절](#dotnet-done-index) 색인):

- Template/DataManager와 저장소 TS0~TS6(0.128~0.136), Fort 타입 표·적재·저장 F1·F2a~F2c-12(0.137~0.155).
- 지형·섬 생성 1·2차와 Fort 사제 준비(0.156~0.160), 일반지형 요청형(0.174), F2c 정상 생성·도장 본체(0.285), 월드 도장 훅(0.291), 지형 패턴 장면값 재생(0.297).
- 다리·섬 연결 본체와 판본별 전수 재생(0.161·0.162), 섬 수명주기·후처리 전수 재생(0.289·0.290, `islandlifecycle-x86.tsv` 2055행·`islandpostpop-x86.tsv` 1988행).
- F2d Player Core·초기화·Chat·참여·CoreData·Player 월드, 캠페인 월드 전역·적재, 서피스 삭제 분배기, 저장 나머지(0.163~0.171). Player 월드 CD 분기·후처리 순서 계약·잔여 순서 계약·가상 분배·월드 직접 조립 검사(0.298·0.299·0.303~0.305), Money 월드 공유(0.300), 공통 몸체 실패 순서(0.301), Subscriber 현재 읽기(0.302).
- 서피스 월드 기반·프로세스·호스트(0.172~0.225), 월드 본체 조립(0.226~0.264: `RawSurfaceWorld` 본체·`RunFrame`·수명 분배기 등).
- F3 세션: 껍데기·Fort 씨앗 적재·프레임/보고서 위임·섬 개수(0.265~0.271), Stub 관문 8종(0.272~0.279)의 본체 연결(0.286·0.288·0.292·0.294~0.296). 세션 소유 상태: Player 상태·Battle(0.286), outposts/temples 목록(0.288), AI 설정(0.306), 미션 시작 설정(0.313), 전투 옵션 87바이트·숨은 플레이어 뷰(0.319·0.320), 현재 인지(0.322), TYPE 표(0.327), 동맹 표 81칸(0.341). AI 관문: 생성·준비·초기화·다리 갱신·자금·다리 경로 전진(0.308~0.312·0.314). 동맹: 해제·협상 수신·무승부 마감·요청 입력(0.315~0.318). 객체 소유자 지정과 세션-월드 연결 검사(0.323·0.328), 가변 로컬 패킷 송신(0.326).
- 채팅 형식·추적·본문 조합·패킷·수신·이름 자동완성(0.334~0.339), Player 식별자(0.340), 아카이브 열거 검사(0.325).
- 재개 준비: 전체 회귀 재기준선(0.280), cpppj 영향 확인(0.281), cpppj 재반영 2건(Demo `Demoville`·Help 스크롤 화살표 색, 0.282), 고정 시각 실행·`session-dump` world 0(0.283·0.284), 전체 회귀(0.329).
- 사전 구현(검사 없음, 3차 범위): LAN TCP/UDP 루프백(0.287), 세션 에디터 전송(0.293), 로컬 미션 Zone(0.307), 연결 활성(0.321), 전투 방송(0.324), 패킷 등록 표(0.330), 연결 등록·정리·종료(0.331~0.333).
- **실제 호스트 조립 착수(0.390~0.459):** 미션 Fort 적재, 세션 홀더·world-1 덤프, 미션 플레이어·SP·AI·동맹, 마스크·Zone·가입·Ready, 요새 순번·이름 확정, 월드 적재 시작 흐름, 타입·프레임 채우기, 지형·난수 저장소, 섬 부모 조립, 지형 바인딩 입력 값 연결, 바인딩 조립·드라이런, Generate 라이브 연결, 난수·덤프 분석, 난수 단일 스트림 연결, 프리-Generate 소비 확정, 프레임 선택 대조(묶음별 단위는 [0.4 R4 진행판](#r4-board)). 같은 구간에 오라클 갱신·L4a 메뉴 12장면(0.380~0.389)과 cpppj 영향 재검토(0.343~0.379)가 있다. 전체 회귀 재완주(0.453: 3,441건, 실패 0).

**남은 항목:**

- [ ] **실제 호스트·월드 조립:** *(0.390~0.459에 Player·SP·AI 설정·동맹·Zone·가입/Ready·이름·요새 순번과 월드 적재 11단계 순서·타입/프레임·지형/난수 저장소·섬 부모·지형 바인딩 입력 값 연결을 호스트에 연결했다. 나머지는 [0.4 R4 진행판](#r4-board)의 2~6번.)* 미션 객체, Fort reader, Guide 교체·수명, 미션 제어 입력, 공통 Start, AI 설정·생성, Guide 단계·Lifecycle은 이식과 L1 대조를 마쳤지만 검사에서는 명시 효과로만 연결돼 있다. 실제 호스트에 분배기를 설치하고 같은 Player·제한 표·메뉴·월드 효과에 연결한다(0.66·0.68·0.69·0.75~0.79). Tutorial Init의 실제 템플릿/옵션 소유자·요새 선택·상태 실행·실제 타입 조회·호스트 단계 연결(0.105~0.110), Start의 실제 Config·Player·SP·AI·Gump 효과(0.70), `PlayerParticipation`의 실제 세션 바인딩과 동맹 수신 `ReceiveAlliance`(0.74), 동맹 표의 `ClientPlayerRecords` 공유(0.341), 세션 TYPE 표의 메뉴·월드 연결(`HostEditorSelection`의 188 상수 교체, 0.327)이 남았다. 세션 대리자(`Bind*`) 묶음은 이 조립에서 실제 몸체에 연결한다.
- [ ] **AI 나머지:** `CrankAiMoney`·`SetAiParameter`(players·aiMemory 필요), `ResetAiForState`·`ReturnAiForState`(participation 필요), 실제 AI 몸체(`RawAiCreate.Create`의 600바이트 검사, 준비 00416eb0, 258 완료 판정 등). 11차 난이도 배율 필드(`o/AiDifficulty`)는 0.347에서 이식해 `RawAiCreate`에 재대조했다(`AiDifficultyTests`·`RawAiCreateTests` 21건). 2번 brain 생성·부착은 0.399에서 연결하고 부착 recipe는 0.412에서 확정했으며 실생성자 토큰은 스텁이다([R4 진행판](#r4-board) 6번).
- [ ] **미뤄 둔 전수 재생:** `player-world-x86.tsv` 1680행(월드 스택 하네스 필요), 0.154에서 옮기지 않은 cpp 검사(`FortWorldLoadBindingTests`·`NormalTerrainWorldBindingTests`의 전체 월드 장면 등), 0.134~0.139에서 F3로 미룬 검사(`CampaignMissionFortPathTests`·`CampaignLoginObjectStreamTests`·`CampaignMissionReloadSourceTests`·`TemplateDiagnosticClientTests` 일부·`SoloMapCatalogTests`의 Tell 메타 1종·`CampaignNewFortTests`·`CampaignNewFortConfigTests`). 가상 분배 진입(`PostPopVirtual`)의 나머지 파생 분기.
- [ ] **M1 판정:** 34경로의 Load/Start 뒤 상태를 cpppj `session-dump`와 대조한다(L2). cpp 기준선은 4경로(thewarbegins 0.390·masterofwhirligigs 0.433·savetheisland 0.445·fragilefortune 0.449)를 확보했고 새 경로가 일치한 경로는 아직 0개다. 나머지 30경로의 기준선은 월드 행이 나온 뒤 만든다([R4 진행판](#r4-board)).

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
- **cpppj의 미완·경계 범위는 cpppj를 따라간다.** cpppj가 지금도 명시 경계·보류로 둔 것(LEFT_JOBS.md 4.1 NoWorld·외부 기능, 5.1 보류 분석, 5.2 기존 한계)만 dotnetpj도 경계로 둔다. 옛 목록(View/zoom, 도킹, DD 조정·확인 UI 잔여, Chat GUI 드래그, 미니맵 내용물·grid·항목별 debug, Renderer 시차 경계)은 cpppj 929 UI 마감 이전 기준이므로 0.4 1-2에서 항목별로 cpppj HEAD 구현 여부를 확인해, 구현된 것은 이식 대상으로 바꾼다. 독립 미니맵, Credits 자동 전환, Chat 입력·전달, 마나 103·배율 Draw는 이미 이식 대상이다.
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

# 영향 재검토(0.4 1-2): 지난 검토 이후 바뀐 cpppj 소스와 PORT_MAP 의 교집합
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
2. **단위 종료:** 머리말의 기준 단위·다음 단위 번호, 0.2·0.3·0.4와 해당 상세 절을 갱신한다. 문서가 없는 작은 단위만 [8절](#dotnet-unit-records)에 짧게 적고, 긴 경과는 `docs/dotnet-*.md` 단위 문서에 둔다. 끝난 상세가 길어지면 보관 파일로 옮기고 [7절](#dotnet-done-index) 색인만 갱신한다.
3. **커밋:** 인계 작성 뒤 **이 단위의 파일만** 경로로 지정해 `YYYYMMDD-NN DN 요약`으로 로컬 커밋한다(끝에 Co-Authored-By 트레일러). 커밋 번호는 다른 PC·세션과 겹칠 수 있으므로 `git log`로 당일 마지막 번호를 확인해 이어 쓴다. 같은 작업 트리에서 다른 세션이 cpppj·`LEFT_JOBS.md`를 자주 고치므로 CP/WB 파일·stage를 건드리지 않는다. **push하지 않는다.**
4. **줄바꿈:** `dotnetpj`·`docs`와 `LEFT_JOBS.dotnetpj*.md`는 저장소에 LF로 저장된다(이 문서는 작업 트리도 LF, 보관 파일은 작업 트리 CRLF). 기본 `autocrlf`로 add하고 `-c core.autocrlf=false`를 쓰지 않는다. CRLF 작업 트리 파일은 Edit 도구 대신 정확히 1회 일치하는 바이트 치환으로 고친다.
5. **빌드 환경:** Git Bash에서 `cmake --build … -- /p:…`를 실행하면 `/p:`가 경로로 바뀌어 MSBuild가 실패하므로 PowerShell로 실행한다. `cpp_sync.py` 보고가 cp949 콘솔에서 깨지면 `PYTHONUTF8=1` 또는 `python -X utf8`. 오라클에는 게임 실행 파일만 필요하며 cpppj 전체 빌드는 검사 대상까지 포함해 오래 걸린다.
6. **주석:** 새 코드의 네임스페이스·상수·함수·구조체·클래스·멤버함수·멤버변수·반복문에 한국어 주석을 단다. 주 내용은 설명과 사용법이고, 언제·왜 추가/수정했는지는 그 뒤에 적는다. C#에서는 `/// <summary>`에 무엇을 하는지와 쓰는 방법(인자·반환·호출 조건)을 쓰고 원본 주소·근거 문서·이력은 summary 뒤쪽이나 `<remarks>`에 둔다. 반복문은 바로 위 `//` 주석으로 무엇을 도는지 적는다. 원본 주소는 10.78 주소와 근거 문서(`docs/exe/cpp-*.md`)를 함께 적는다.

## 7. 참고 문서와 완료 범위 색인

<a id="dotnet-done-index"></a>

### 7.1 완료 범위 색인

단위별 상세는 [LEFT_JOBS.dotnetpj.old.md](LEFT_JOBS.dotnetpj.old.md)와 아래 문서에 있다. 마지막 열은 현행 계획에서의 처리다([2.2](#04-기존-dotnetpj-소스의-재수정-판정)). 새 경로의 진행도는 Client 검사와 2.3의 실제 완료 기준을 함께 본다. Assets·Core의 기존 경로 검사 수를 새 경로 진행도로 세지 않는다.

| 기간·단위 | 완료 범위 | 근거 문서 | 처리 |
| --- | --- | --- | --- |
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
| 0.390~0.452 (11-12) | R4 실제 호스트·월드 조립 착수 — thewarbegins 기준선(0.390)부터 섬 부모 조립(0.452)까지, 묶음별 요약은 [0.4 R4 진행판](#r4-board) | [보관 0.342~0.452](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261112-through-0452) | 새 경로(진행 중) |

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

## 8. 단위 기록 (0.453부터)

문서가 없는 작은 단위만 아래에 최신순으로 짧게 적는다. 한 기록에는 범위, 구현(새 파일·수정 파일·옮긴 cpppj 파일과 기준 커밋), C#에서 달라진 모양, 검증(집중·전체·변이·L2/L4a 비교), PORT_MAP·fixture 변화, 함께 옮긴 10차 NB 수정, 보호·실행·외부 접속 관점, 남은 것을 적는다([3.1](#verify-levels)). 0.11~0.452는 보관 파일에 있다: [0.11~0.65](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261031-through-073) · [0.66~0.126](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261102-through-0126) · [0.127~0.153](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261104-through-0154) · [0.154~0.279](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261110-through-0279) · [0.280~0.341](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261111-through-0341) · [0.342~0.452](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261112-through-0452). 묶음별 요약은 [7.1](#dotnet-done-index)과 [0.4 R4 진행판](#r4-board)에 있다.

#### 0.468 — 4경로 dn 미션 덤프 생성과 SID 단위 구조 대조·적재 순서 확정 (2026-11-12, HJOW-Athlon)

- 범위: 0.4 순서 2(R4 실제 호스트·월드 조립)의 일흔여섯 번째 단위. baselined 4경로 중 나머지 3경로의 첫 실제 dn 미션 덤프(f090/f240)를 생성하고 SID 단위로 대조하며, 요새 적재 순서 패리티를 검사로 확정한다. 제품 코드 변경 없음(검사 2건 추가·측정 단위).
- 구현: 신규 `HostFortObjectLiveTests` 2건(기존 5건 → 7건). `ProbeFortLoadVsDumpOrder`는 Load와 같은 순서·건너뜀·다리 매핑·내용물 끼워넣기로 기대 타입열(`CollectFortSection`·`CollectFortContents`: y 바깥·x 안쪽, 지역 `Matches` 건너뜀, 0xf6+→다리 타입, ContainerFlag 부착, factoryFlags→server/client/predictable 분기)을 모아 풀 번호순 실제와 엄격 대조한다. `ProbeFortContentsNulls`는 ContainerFlag 조건 내용물 null 위치를 전수 특정한다(`CollectNulls` 재귀, null이면 실패). 4경로 dn 미션 덤프 신규 생성(`*-dn-f090-0468.tsv`·`*-dn-f240-0468.tsv`, 고정시각 paired, `extracted/dotnet-l2-baseline/<경로>/dn/`, Git 제외). thunderingpower는 cpp 기준선 시드가 dn 고정시각과 달라 구조 참조용으로만 둔다.
- 검증: 빌드(경고 0·오류 0) 뒤 Client 전체 1,146건 통과(신규 2건 포함, 실패 0, NETSTORM_DATA 지정, 121초). f090 SID 대조(`compare_slots.py`, 직접 재실행 확인): masterofwhirligigs dn 1743·cpp 1807·공통 1743·타입 1591·프레임 75·full 53·Game 173420507, savetheisland 2618·2926·2618·2246·126·61·Game 4032020911, fragilefortune 2719·2946·2719·2205·195·78·Game 1491053939, thunderingpower 2410·2490·2410·2221·116·73·Game 297644596(구조만). 4경로 모두 dn_only 0·공통=dn 전체, CRT 296677442 parity 유지, dn frame max 208·cpp max 316. f240은 3경로 모두 dn_only [6] 1개(내용물 부착형, 게임플레이 잔여·R6). cpp_only SID는 클라이언트 블록 8~81+(플레이어·AI·셋업·프로세스, item 7/R6 잔여) + 타입 34 gun-process 인터리브 약 9개(postPop 생성, R6) + 5-SID 꼬리. dn 생성 순서는 fort 순서·cpp 순서와 modulo 9 인터리브를 제외하고 일치한다(순서 검사 통과). noIsland 소유자 차이(dn 0 vs cpp 2)는 미해결로 남기고 적재 후 배치(R6) 추정으로 적는다. `originals/` 미접촉, 추적 `assets/game-data/d/options.cfg`는 되돌렸다(작업 트리 `git status`에 dotnetpj 2건만 남김).
- PORT_MAP, fixture 변화 없음(대응행 추가 없음). 함께 옮긴 10차 NB 수정 없음. 외부 접속 목록 변경 없음.
- 보호: originals, originalCD, AGENTS.md, README 개요, cpppj, LEFT_JOBS.md 미수정(변경은 LEFT_JOBS.dotnetpj.md 문서와 dotnetpj 새 경로 검사만). 작업 트리에 있던 webpj 미커밋 변경(geyser-mined 2건)은 이번 단위와 무관해 손대지 않고 커밋 범위에서 제외한다. 사제 포획/희생·특정 적 공격·시나리오 승리 재현 검사는 건너뛰었다.
- 남은 것: 장부·그래프·농지·소유자(R4 진행판 6번 잔여, noIsland 소유자 후속 지정 포함), 객체 Pop 공간 등록(postPop·표시·그래프, R6) + 덤프 대조, brain 실생성자·요새 이미지·메뉴 미션 시작, 34경로 L2 대조(M1, 4/34 — 동일 시드 paired cpp 재실행은 본 대조 때 묶어서).

#### 0.467 — 요새 객체 live 적재와 paired 구조 확정 (2026-11-12, HJOW-Athlon)

- 범위: 0.4 순서 2(R4 실제 호스트·월드 조립)의 일흔다섯 번째 단위. R4 진행판 6번 중 요새 객체 조각을 live 연결한다. 제품 코드 변경 있음.
- 구현: `HostSession.BuildFortObjectDevices`(공장·HP·내용물·건물·Pop 몸체, `MakeFortObjectLoadHooks`로 Create/HP/내용물 결선, `RawFortObjectLoad(Pool, 공유 해시·spots, types, frames, Game, state, hooks)`) + `LoadFortObjects(fort, 칸)` + `StartMission` Fort/Campaign Objects live 연결(기록 병행). 상태값은 미션 적재값(Mission=true·ChaffMode=true, Fort=false·Special=false·Version=0, cpppj 1093행 대응). SetState·FinishOwner는 기록만(R6 공장 후처리·거주 배치), Destroy·VirtualCount는 미션 적재에서 도달하지 않으므로 명시 경계, 내용물 부착은 bound Form 분배+서버 전송 기록으로 live 연결한다(cpppj `RawCampaignWorldRuntime.cpp` 43~53·61·82행 대응). Pop은 기록만 둔다(R6 postPop·표시·그래프 장치와 함께 잇는다).
- 근거·판정: 실측으로 범위를 확정했다. thewarbegins 요새 객체에 Pop 지원표 밖 가상 표(71·75·78·89·122 등)가 있어 공간 등록을 수행하면 파생 등록 효과 미지원으로 멈춘다 → Pop은 R6으로 미룬다. 내용물 보유 타입(123×1·124×1·158×2)이 있어 부착은 live로 잇는다. test 순서가 live와 달라 부분집합만 돌던 문제를 `BuildLiveTerrainFirst`(Territory 120B→PlaceAll→Generate) 도입으로 해소했다(같은 적재 부분집합, 같은 GameRandom 소비 순서).
- 검증: 신규 `HostFortObjectLiveTests` 5건 통과(장치 거부·미연결 거부·실제 요새 구조/결정성·Pop 미지원 분류·내용물 전수, 실패 0, NETSTORM_DATA 지정). 빌드(경고 0·오류 0) 뒤 Client 전체 1,144건 통과(신규 5건 포함, 실패 0, 76초). paired 라이브 재실행(예외 PC, `steps-dn-f065-0467.tsv`, `--frames 90 --fixed-frame-ms 25 --fixed-unix-seconds 1760000000`, 종료 코드 0): 덤프 2,129행·슬롯 1,853(+174), `random 2317693072 296677442`(Game 새 기준·CRT parity 유지), `compare_slots.py` 대 cpp-f065 결과 공통 1853·타입 1628·프레임 123/1853·full 78·dn_only 0·cpp_only 87(261→87)·dn max 208·cpp max 316. `originals/` 미접촉, 추적 `assets/game-data/d/options.cfg`는 되돌렸다. paired 산출물은 `extracted/dotnet-l2-fixed/`에 두었다(Git 제외).
- PORT_MAP, fixture 변화 없음(대응행 추가 없음). 함께 옮긴 10차 NB 수정 없음. 외부 접속 목록 변경 없음.
- 보호: originals, originalCD, AGENTS.md, README 개요, cpppj, LEFT_JOBS.md 미수정(제품 변경은 dotnetpj 새 경로·검사만). 사제 포획/희생·특정 적 공격·시나리오 승리 재현 검사는 건너뛰었다.
- 남은 것: 객체 Pop 공간 등록(postPop·표시·그래프, R6) + 덤프 대조, 장부·그래프·농지·소유자(R4 진행판 6번 잔여), brain 실생성자·요새 이미지·메뉴 미션 시작, 34경로 L2 대조(M1, 4/34).

#### 0.466 — 지형 Pop 공간 등록 live 연결과 paired 중립성 검증 (2026-11-12, HJOW-Athlon)

- 범위: 0.4 순서 2(R4 실제 호스트·월드 조립)의 일흔세 번째 단위. R4 진행판 6번 중 Pop·공장체 조각을 live 연결한다. 제품 코드 변경 있음.
- 구현: `HostSession.BuildTerrainPopDevices`(공장·해시·spots·SHP 표시 결선, cpppj `MakeTerrainStampWorldHooks`의 core.factory·core.hash·core.shapes→FrameSize 대응) + `BuildTerrainStampHooks` Create/Pop live 분기(미연결 때 neutral 기록만이라 기존 드라이런 검사 그대로 통과) + `ClientHostGame.StartMission` SHP 적재·장치 연결(`d/_shapes.shp`→`ShapeDatabase`→`TerrainDisplayShapes.Build`→`BuildTerrainPopDevices`). 신규 `TerrainDisplayShapes.cs`(cpppj `SquidRenderer::Shapes` 이식: 70번부터 자산 순서·클러스터 수=프레임 수·SHP 물리 순서). `Build`는 정의 안의 typename이 아니라 SHP 블록 순서(=자산 로딩 순서)로 찾는다(typename 조회는 `Man` 같은 내부 이름에서 실패한다). `HostSession`에 검사·대조용 `TerrainPopHash`·`TerrainPopSpots`를 두었다. 중립 Create의 `raw.Clear()`가 VoidBit를 지우던 것을 `VoidBit` 재설정으로 고쳤다(공장 경로는 Allocate가 Void를 둔다). `ClientHostGame`의 `Netstorm.Assets` using 모호 참조(`MissionScript`·`ShapeDatabase`)는 using 제거+완전 한정으로 해소했다.
- 검증: 신규 `HostTerrainPopTests` 5건 통과(중립 보존·개수 거부·공장 vtable Supports·섬 좌표/화면좌표/단계0/섬127/void해제/체인/spot·프린지 실측 3단계, 실패 0). 기존 `HostTerrainBinding/Stores/Inputs` 9건 통과(중립 동작 보존). 빌드 경고 0·오류 0 뒤 Client 전체 1,139건 통과(신규 5건 포함, 실패 0, NETSTORM_DATA 지정, 95초). paired 라이브 재실행(예외 PC, `steps-dn-f065-0466.tsv`, `--frames 90 --fixed-frame-ms 25 --fixed-unix-seconds 1760000000`, 종료 코드 0): 덤프 `random 2583299428 296677442`(Game 예측 일치·CRT parity 유지), 구조 1,955행·슬롯 1,679 동일, `compare_slots.py` 대 cpp-f065 결과 공통 1679·타입 1526·프레임 80/1679·full 78·dn max 208·cpp max 316으로 0.464와 동일하고 pool 해시만 Pop 기록분 변경됐다(Pop은 타입·프레임·난수 중립). `originals/` 미접촉, 추적 `assets/game-data/d/options.cfg`는 되돌렸다. paired 산출물은 `extracted/dotnet-l2-fixed/`에 두었다(Git 제외).
- PORT_MAP, fixture 변화 없음(대응행 추가 없음). 함께 옮긴 10차 NB 수정 없음. 외부 접속 목록 변경 없음.
- 보호: originals, originalCD, AGENTS.md, README 개요, cpppj, LEFT_JOBS.md 미수정(제품 변경은 dotnetpj 새 경로·검사만). 사제 포획/희생·특정 적 공격·시나리오 승리 재현 검사는 건너뛰었다.
- 남은 것: Objects 라이브 연결(`RawFortObjectLoad` + 갈고리 약 20종, 공장 할당 parity) + 덤프 대조, 장부·그래프·농지·소유자(R4 진행판 6번 잔여) + 기동/메뉴 경로 복원 뒤 프레임 수렴 재시도, brain 실생성자·요새 이미지·메뉴 미션 시작, 34경로 L2 대조(M1, 4/34).

#### 0.465 — early-dump N(F) 역산과 ambient-sim 가설 기각·회수 (2026-11-12, HJOW-Athlon)

- 범위: 0.4 순서 2(R4 실제 호스트·월드 조립)의 일흔두 번째 단위. K-search 평탄(75~86/1679) 원인을 early-dump Game 역산으로 규명한다. 제품 코드 변경 없음(미커밋 `SimulateMenuAmbient` 34행을 회수해 작업 트리 clean, 조사·검증 단위).
- 증거 N(F) 역산(seed 4097495276 기준 advance 횟수, `extracted/dotnet-l2-fixed/thewarbegins/`, Git 제외): dn-f065 1764 · cpp f1 3395·f2 3397(+2)·f3 3398(+1)·f4 4680(+1282)·f6 8078(f4→f6 +3398)·f7 8079(+1)·f8 8081(+2)·f10 8084·f065 12077·f090 16168(f065→f090 +4091/25=+163.6/프레임)·f240 40719(f090→f240 +24551/150=+163.7/프레임)·dn-f065 1764. 메뉴율 1~2/프레임(Wind만)과 일치, 게임플레이율 ~163-164/프레임, AI 활성화 ~f31-f40(f29→f31 +2·f31→f40 +1155/9≈128·f40→f50 ≈164).
- Generate 구간 특정: f1-f3 world 0·슬롯 0, f4 world 1·kernel 0·슬롯 5(클릭 5 이전에 출현), f6 world 1·kernel 34·슬롯 1928. 클릭-5→덤프-6 사이 Terrain Generate+Objects 적재. CRT는 f1-f4 511160713→f6+ 296677442로 바뀌나 Game 사슬은 연속(N 단조 증가, 재시드 없음)이라 Game 무재시드(0.464 유지)·CRT만 적재 시 재설정(동일 고정 시각이라 같은 값) 또는 세션 스트림 교체로 판정. dn CRT 296677442와 f6+ parity는 유지된다.
- f4 이상치(+1282, 클릭 전 world-1·5슬롯): 비동기 적재 스캐폴딩 또는 메뉴 배경 세션 중 하나로 판정(어느 쪽이든 dn에 없는 적재-전 소비). 0.461 "양쪽 동기 실행" 서술은 정정한다: cpp 적재는 f4→f6에 걸쳐 프레임 경계를 넘나들며(메인루프 ambient가 적재 사이 약 4 draws 인터리브 가능), dn 적재는 `LoadContent` 동기라 인터리브가 없다. ambient 완전배제가 아니라 소규모 기여자로 정정한다.
- K-search 평탄 설명과 sim 기각: 지형-전 gap ≥ ~3500 draws(기동 3395+f3-f4 1282+적재-전 L_pre, dn 지형-전 1154=33+801+320 대비)인데 K≤90은 약 180 draws 이하라 범위를 벗어난다. sim 위치(`StartPlayerSp` 앞 Generate 전 일괄)도 cpp 순서(적재 후 매 프레임·적재 사이 인터리브)와 달라 magnitude·position 둘 다 불일치 → sim 회수. f4→f6 3398−Sets(320)−지형(610)−ambient ≈ 2400-2500 draws는 Objects·Pop·농지 배치·공장체·소유자 잔여(R4 진행판 6번)와 규모가 부합한다.
- 진입 비대칭 주의(M1 caveat): cpp paired 실행은 메뉴 경로(`--mission` 없음: f1-f3 world 0, 적재 f4-f6)이고 dn은 `--mission` 직접 진입(`LoadContent` 적재)이라 동일 프레임 번호가 적재-후 경과와 다르다(cpp f90≈84 vs dn f90=90 post-load). 지형 비교에는 무영향이나 Game/SP 행의 동등-프레임 cross-side 비교는 적재-후 정렬이 필요하다.
- 검증: 회수 뒤 작업 트리 clean(`git status` 빈 출력), 깨끗한 전체 리빌드(경고 0·오류 0, 69초) + Client 전체 1,134건 통과(0.464 게이트, 실패 0, NETSTORM_DATA 지정, 85초). 새 집중 검사 없음(조사 단위). 게임 실행 없음(디스크 덤프+정적 분석만). `originals/` 변경 없음, 추적 `options.cfg` 변경 없음. paired 산출물·역산 스크립트는 `extracted/dotnet-l2-fixed/`에만 둔다(Git 제외).
- PORT_MAP, fixture 변화 없음(대응행 추가 없음). 함께 옮긴 10차 NB 수정 없음. 외부 접속 목록 변경 없음.
- 보호: originals, originalCD, AGENTS.md, README 개요, cpppj, LEFT_JOBS.md 미수정(변경은 LEFT_JOBS.dotnetpj.md 문서만). 사제 포획/희생·특정 적 공격·시나리오 승리 재현 검사는 건너뛰었다.
- 남은 것: Objects 라이브 연결(`RawFortObjectLoad` + 갈고리 약 20종, 공장 할당 parity) + 덤프 대조, 장부·그래프·Pop·농지·공장체·소유자(R4 진행판 6번) + 기동/메뉴 경로 복원 뒤 프레임 수렴 재시도, brain 실생성자·요새 이미지·메뉴 미션 시작, 34경로 L2 대조(M1, 4/34).

#### 0.464 — 호스트 시작 1회 reseed+SP 초기화 적용과 paired 검증 (2026-11-12, HJOW-Athlon)

- 범위: 0.4 순서 2(R4 실제 호스트·월드 조립)의 일흔한 번째 단위. 0.463 근본 원인(reseed 시점)을 수정한다. 제품 코드 변경 있음(호스트 공유 SP·CRT 상자, 미션별 reseed 제거).
- 구현: `ClientHostGame`에 `_spModeBox`·`_spCrtBox`·`_sharedSp`를 두고 생성자에서 reseed+`Initialize` 1회 실행(cpppj `ClientMain` 349행 대응, 원본 004391ed). `HostSession`에 공유 Sp·CRT 상자 빌림 overload를 추가했다(기존 3인자 호출은 세션 전용 상자로 그대로 동작한다). `StartMission`에서 미션별 `BindSpReseed`·재초기화·`TerrainCrt=1u`를 없애고 공유 저장소로 `StartPlayerSp`(Sets만) 한다. 신규 검사 `SharedSpStoreDoesNotReseedOnStartPlayerSp`(공유 사전 초기화 뒤 Sets 잡음만 160×활성 소유자임을 수계산으로 대조, SP 값 1번 1161527296 유지).
- 검증: 깨끗한 전체 리빌드(경고 0·오류 0) 뒤 Client 전체 1,134건 통과(신규 1건 포함, 실패 0, NETSTORM_DATA 지정). paired 라이브 재실행(예외 PC, 0.462와 동일 인자+`steps-dn-f065-0464.tsv`): 덤프 Game=2583299428이 사전 예측 `advance^1764(seed)`와 정확히 일치했다(33+801+320+610, 중간 소비 0 증명), CRT 296677442 유지, SP offset 44 동일, 프레임 80/1679(여전히 미수렴 — 잔여는 메뉴 ambient, 0.465). `originals/` 변경 없음, 추적 `assets/game-data/d/options.cfg`는 되돌렸다. paired 산출물은 `extracted/dotnet-l2-fixed/`에 두었다(Git 제외).
- PORT_MAP, fixture 변화 없음(대응행 추가 없음). 함께 옮긴 10차 NB 수정 없음. 외부 접속 목록 변경 없음.
- 보호: originals, originalCD, AGENTS.md, README 개요, cpppj, LEFT_JOBS.md 미수정(제품 변경은 dotnetpj 새 경로·검사만). 사제 포획/희생·특정 적 공격·시나리오 승리 재현 검사는 건너뛰었다.
- 남은 것: 메뉴 ambient 시뮬레이션과 K 탐색·수렴 검증(0.465), Pop·농지 배치·공장체·소유자 라이브 연결, Objects·장부·그래프, brain 실생성자·요새 이미지·메뉴 미션 시작, 34경로 L2 대조(M1, 4/34).

#### 0.463 — reseed 시점 차이 근본 원인과 수정 방향 확정 (2026-11-12, HJOW-Athlon)

- 범위: 0.4 순서 2(R4 실제 호스트·월드 조립)의 일흔 번째 단위. 0.462 잔여(테이블 내용 직접 대조)를 확정한다. 제품 코드 변경 없음(조사·검증 단위).
- 테이블·난수 알고리즘 동등성 확정(정적 대조, 실행 없음): ① 프레임 표 파서 동등 — dn Core `RiftTypeDefinition`(토큰 렉서·선언 순서 보존·중복/플래그/GIF 순서 유지, `ReadFrameCode` 문자·변형·번호 분리, 7개 플래그 비트+base/gump/default/help 특수 프레임, `FrameTable()`=클러스터 순서 그대로)이 cpp `o/TypeParser.cpp`(`ReadFrameCode`·`Parse` 135~204행)와 일치한다. 양쪽 `Letter` 판정(A-Z·a-z·`_`)도 같다. ② `GameRandom::Next` 동일 — cpp `o/SpStore.cpp` 20~25행(0이면 `0x0BAD0BAD`, `state*0x10003+3`, 상위 16비트 `% limit`) vs dn `RawGameRandom.cs` 31~50행. 따라서 +6 오프셋·섬 전멸 패턴은 테이블·난수 알고리즘 탓이 아니다.
- reseed 시점 차이 확정(원본 충실도 판정 포함): dn은 미션 시작 때마다 reseed+SP 재초기화(`ClientHostGame` 272~274행, 0.395 도입)하지만 cpp는 클라이언트 시작 때 1회만 SP 초기화하고(`ClientMain` 349행, 원본 004391ed) 미션 Load에서는 공유 SP 경로를 다시 쓰지 않는다(`RawCampaignSession` 1090행은 독립 세션 전용). cpp `--mission`도 메뉴를 경유하고(`ClientMain` 556행 `UberGump::StartMission`), dn `--mission`은 메뉴를 우회하며, cpp ambient는 메뉴 포함 매 프레임 공유 난수를 소비하고(601~602행) dn 호스트에는 ambient 호출이 없다. 양쪽 미션 스트림은 reseed 정책·진입 경로·ambient 유무가 모두 달라 필연적으로 갈린다. 원본을 복원한 cpp가 기준이므로(004391ed) dn이 맞춰야 한다. CRT parity(296677442 양쪽 일치)와 모순되지 않는다: 양쪽이 같은 고정 시각으로 reseed하고 그 뒤 CRT 소비(Variants 99회 등)가 같으면 reseed 지점이 달라도 덤프 CRT가 일치한다.
- 수정 방향(0.464 구현): dn도 호스트 시작 때 1회만 reseed+SP 초기화(cpp 349행 대응)하고, 메뉴 ambient 차이(매 프레임 Wind·기한 Sound)는 메뉴 경로 연결(R3/R6) 또는 동일 메뉴 프레임 수 시뮬레이션으로 해소한 뒤 paired 재대조로 프레임 수렴을 확인한다.
- 검증: 제품 코드 무변경이라 새 집중 검사 없음(0.453 기준선 유지). 정적 대조만 수행했으므로 게임 실행 없음. `originals/` 미접촉, 추적 `options.cfg` 변경 없음.
- PORT_MAP, fixture 변화 없음. 함께 옮긴 10차 NB 수정 없음. 외부 접속 목록 변경 없음.
- 보호: originals, originalCD, AGENTS.md, README 개요, cpppj, LEFT_JOBS.md 미수정. 사제 포획/희생·특정 적 공격·시나리오 승리 재현 검사는 건너뛰었다.
- 남은 것: reseed-1회 수정과 paired 재대조(0.464), Pop·농지 배치·공장체·소유자 라이브 연결, Objects·장부·그래프, brain 실생성자·요새 이미지·메뉴 미션 시작, 34경로 L2 대조(M1, 4/34).

#### 0.462 — SID 단위 per-cell 프레임 대조와 발산 위치 확정 (2026-11-12, HJOW-Athlon)

- 범위: 0.4 순서 2(R4 실제 호스트·월드 조립)의 예순아홉 번째 단위. 0.461 잔여(SID 단위 per-cell 대조)를 확정한다. 제품 코드 변경 없음(조사·검증 단위, `compare_slots.py`는 Git 제외 `extracted/`에만 둔다).
- paired 라이브 대조(예외 PC): 깨끗한 빌드(경고 0·오류 0) 뒤 개발 호스트로 `--mission thewarbegins --handle Youpson --window --frames 90 --fixed-frame-ms 25 --fixed-unix-seconds 1760000000 --mute-audio`+`--ui-script steps-dn-f065.tsv`(65프레임 session-dump)를 실행(종료 코드 0)해 새 dn 기준선 `thewarbegins-dn-f065.tsv`(1,955행·frame 65·Game 3186620043·CRT 296677442, CRT는 cpp f065와 일치)를 얻었다. `originals/` 변경 없음, 추적 `assets/game-data/d/options.cfg`는 되돌렸다. paired 기준선(`extracted/dotnet-l2-fixed/`, Git 제외)에 dn 덤프·스텝·대조 스크립트를 추가했다.
- SID 대조 결과(`slot` 행 SID 결합, stride 50B·타입 +10·프레임 DWORD +36): dn 슬롯 1,679·cpp 슬롯 1,940·공통 1,679·dn_only 0·cpp_only 261(게임플레이 객체 잔여). 타입 일치 1,526개, 불일치 153개 중 152개는 dn97 vs cpp121(농지 postprocess, R6 잔여)·1개는 dn0 vs cpp152. 프레임 일치 78개=지형 5개+프린지 72개+기타 1개이며 섬 타입 97은 1,505개 중 0개 일치·프린지 102는 168개 중 72개 일치다. 소유자 일치는 325개(예상 잔여). 불일치 표본에 +6 오프셋이 다수(141→147·149→155·33→39)라 xAA 셀의 J-run base 차이(테이블 내용) 가설이 유력하며 0.463에서 dn `WorldFrames`와 cpp 프레임 표 내용을 직접 대조한다.
- PORT_MAP, fixture 변화 없음. 함께 옮긴 10차 NB 수정 없음. 외부 접속 목록 변경 없음.
- 보호: originals, originalCD, AGENTS.md, README 개요, cpppj, LEFT_JOBS.md 미수정. 사제 포획/희생·특정 적 공격·시나리오 승리 재현 검사는 건너뛰었다.
- 남은 것: 프레임 표 내용 직접 대조(0.463)와 프레임 반영·검증, Pop·농지 배치·공장체·소유자 라이브 연결, Objects·장부·그래프, brain 실생성자·요새 이미지·메뉴 미션 시작, 34경로 L2 대조(M1, 4/34).

#### 0.461 — SP 353 구성과 reseed-Generate 동기 순서 확정 (2026-11-12, HJOW-Athlon)

- 범위: 0.4 순서 2(R4 실제 호스트·월드 조립)의 예순여덟 번째 단위. 0.460 잔여(reseed 대비 ambient 순서)를 확정한다. 제품 코드 변경 없음(조사·검증 단위, 임시 계측 없음).
- SP 353 구성 확정(정적 계수+감사 일치): `RawSpStore.Initialize` 33회(`Next(100)` 1회+`Next(32)` 32회, `RawSpStore.cs` 149~197행) + 활성 소유자당 `Set` 160회(32비트×잡음 5회, 229~255행). `HostSession.StartPlayerSp`가 활성 소유자마다 `SetPlayerSpAmount`→`store.Set` 1회씩 호출하고 첫 `Set`이 `Initialize`를 촉발하므로 합계=33+160N. 감사 353회와 정확히 일치하는 정수해는 N=2(33+320)이며 1-1 미션(인간+적 AI 1명) 구조와 일치한다. cpp `o/SpStore.cpp` 45행 `Initialize`도 같은 순서라 양쪽 구성이 같다.
- 동기 순서 확정(정적 대조): dn `ClientHostGame.StartMission` 270~383행이 BindSpReseed→StartPlayerSp→…→`worldLoad.Load()`→`GenerateOriginal`을 한 시작 흐름에 동기 실행하고 프레임 루프(`Update` 393행~)와 분리돼 있다. cpp도 `ClientMain` 349행(시작 SP)·`RawCampaignSession` 1090행(독립 세션 SP)이 적재 흐름에서 동기 초기화하고 ambient(601~602행)는 바깥 루프에서만 돈다. 따라서 reseed→SP→Generate 사이에 ambient가 낄 수 없어 0.460의 메뉴 ambient 가설은 지형 프레임 원인에서 제외된다(ambient는 적재 후 게임플레이의 SP offset 등에만 영향, R6 잔여). 프레임 발산 원인은 성장 입력·CRT 쪽으로 좁혀졌다(0.462에서 성장 입력 대조).
- 검증: Core SpStore·FortMoney·SetPlayerSp 12건 집중 통과(실패 0, NETSTORM_DATA 지정). 제품 코드 무변경이라 전체 회귀는 0.453 기준선을 유지한다. 게임 실행 없음(정적 대조 단위). `originals/` 미접촉, 추적 `options.cfg` 변경 없음.
- PORT_MAP, fixture 변화 없음. 함께 옮긴 10차 NB 수정 없음. 외부 접속 목록 변경 없음.
- 보호: originals, originalCD, AGENTS.md, README 개요, cpppj, LEFT_JOBS.md 미수정. 사제 포획/희생·특정 적 공격·시나리오 승리 재현 검사는 건너뛰었다.
- 남은 것: 성장 입력 대조와 프레임 반영·검증(0.462), Pop·농지 배치·공장체·소유자 라이브 연결, Objects·장부·그래프, brain 실생성자·요새 이미지·메뉴 미션 시작, 34경로 L2 대조(M1, 4/34).

#### 0.460 — 후보 테이블 per-type 대조와 메뉴 소비량 확정 (2026-11-12, HJOW-Athlon)

- 범위: 0.4 순서 2(R4 실제 호스트·월드 조립)의 예순일곱 번째 단위. 0.459 잔여(per-type 대조·메뉴 소비량)를 확정한다. 제품 코드 변경 없음(조사·검증 단위, 임시 계측 없음·`git status` 제품 트리-clean 확인).
- per-type 정적 대조(실행 없음): ① `ChooseTerrainFrame` 몸체 동일 — cpp `o/RawFrameChoose.cpp` 8~33행 vs dn `RawFrameChoose.cs` 20~82행(수집→정규화→'A'→전체 폴백, theme -1·quartile, NB23 `high<=low` 전체 선택). ② `RiftTypeFrames` 구조 동일 — Codes·FindNumber·FindMasked·FindFlags·Run(A~P)·LetterKinds(cpp `o/RiftType.h` 24~46행 vs dn `RiftTypeFrames.cs` 22~156행). ③ `FrameTable()` 양쪽 클러스터 순서 그대로 — cpp `o/TypeParser.cpp` 225~231행 vs dn `TypeFrameTable`+`HostSession.FillWorldFrames`(70+순서, `WorldFrameCount`=188·`FirstAssetTypeNumber`=70·자산 116개→70..185, 0..69·186..187 빈칸, cpp `SessionAssets` 70+block·188칸과 같은 틀). 따라서 분포 차이(dn 최대 0xd0·100종 vs cpp 최대 0x13c·162종)는 필터·구조 차이가 아니라 스트림 위치 차이 쪽으로 좁혀진다.
- 메뉴 소비량 확정(정적 계수): cpp `ClientMain.cpp` 601~602행은 메뉴 여부와 무관하게 매 프레임 `UpdateAmbientSound`+`UpdateAmbientWind`를 공유 `GameRandom`으로 소비한다. Wind=`Next(100)` 1회+10% 확률 `Next(65536)` 1회(평균 약 1.1회, `o/RawAmbientWind`·dn `RawAmbientWind.cs` 48~73행 동일). Sound=기한 통과 때만 `Next(3000)` 1회[+kind 1·2 때 `Next(20000)` 1회], 기한은 벽시각+10초 후 120초 간격(`o/RawAmbientSound.cpp` 11~23행)이라 단기 메뉴(약 90 프레임)에서는 0~1회다. dn 호스트(`Client.Host`)에는 `UpdateAmbient` 호출이 없다(grep 무일치). cpp 덤프는 메뉴 경유(ui-script click-if 5/30/60→dump 90, 약 90 메뉴 프레임) vs dn 직접 진입(`--mission`, 0 메뉴 프레임)이라 적재 전 스트림이 갈린다. 단 dn 감사에서 적재 전 801회는 reseed로 무효이므로, reseed 대비 ambient 순서(적재 전후)는 0.461 paired 라이브 대조에서 확정한다.
- 검증: Core RiftType·TypeFrame·FrameChoose·AmbientWind·RawTerrain 33건 집중 통과(실패 0, NETSTORM_DATA 지정). 제품 코드 무변경이라 전체 회귀는 0.453 기준선을 유지한다. 게임 실행 없음(정적 대조 단위). `originals/` 미접촉, 추적 `options.cfg` 변경 없음.
- PORT_MAP, fixture 변화 없음(대응행 추가 없음). 함께 옮긴 10차 NB 수정 없음. 외부 접속 목록 변경 없음.
- 보호: originals, originalCD, AGENTS.md, README 개요, cpppj, LEFT_JOBS.md 미수정. 사제 포획/희생·특정 적 공격·시나리오 승리 재현 검사는 건너뛰었다.
- 남은 것: reseed 대비 ambient 순서 확정과 프레임 반영·검증(0.461 paired 라이브), Pop·농지 배치·공장체·소유자 라이브 연결, Objects·장부·그래프, brain 실생성자·요새 이미지·메뉴 미션 시작, 34경로 L2 대조(M1, 4/34).

#### 0.459 — 프레임 선택 대조와 메뉴 소비 확정 (2026-11-12, HJOW-Athlon)

- 범위: 0.4 순서 2(R4 실제 호스트·월드 조립)의 예순여섯 번째 단위. 0.458 잔여(프레임 미수렴 78/1679)의 원인을 확정한다. 제품 코드 변경 없음(조사·검증 단위, 임시 계측은 원복했고 `git status` 빈 출력으로 확인).
- dn 측 감사(임시 계측, 실행 후 제거): game0=4190983114(호스트 사전 소비 801회, reseed로 무효) → SP 후=1839122055(=advance^353(seed), 수계산 일치) → Generate 전후 무변화 → 덤프=advance^(353+610)(지형 610회 소비). reseed→SP 353회→Generate→지형 610회로 완전히 닫힌다.
- cpp 측 정적 인벤토리: 세션 조립·AI 생성·로그인·플레이어 생성·PlaceAll에 공유 난수 직접 소비 없음(모두 바인딩·지연·비난수 효과). 메뉴 ambient(`ClientMain.cpp` 601~602행, UpdateAmbientSound+UpdateAmbientWind)가 메뉴 여부와 무관하게 매 프레임 공유 `GameRandom`을 소비한다 — dn 직접 진입(`--mission`, 메뉴 없음)에는 이 구간이 없어 스트림이 갈린다.
- 프레임 값 분포 대조(paired 고정시각 덤프): dn 최댓값 0xd0·100종 vs cpp 최댓값 0x13c·162종. 동일 셀 집합·동일 생성 순서·동일 스트림 가정 하에 설명 불가이므로 `RiftTypeFrames` 내용·순서 또는 `ChooseTerrainFrame` 후보 필터가 다르다(0.460에서 per-type 대조). shift 상관 ±1~±5000 무응답, f065-vs-f090 분기가 적재 시점에 이미 존재(dn 1679행 동일·cpp 1921→1941행)함을 함께 확인했다.
- 검증: 깨끗한 빌드(경고 0·오류 0) 뒤 Client Host·Dump 24건 집중 통과(실패 0, NETSTORM_DATA 지정). 게임 실행 있음(예외 PC): cpp f065/f90 + dn f065/f90 paired 덤프(종료 코드 0). `originals/` 변경 없음, 추적 `assets/game-data/d/options.cfg`는 되돌렸다. paired 기준선(`extracted/dotnet-l2-fixed/`)에 f065 덤프를 추가했다(Git 제외).
- PORT_MAP, fixture 변화 없음(대응행 추가 없음). 함께 옮긴 10차 NB 수정 없음. 외부 접속 목록 변경 없음.
- 보호: originals, originalCD, AGENTS.md, README 개요, cpppj, LEFT_JOBS.md 미수정(조사 단위라 제품 코드 변경 없음). 사제 포획/희생·특정 적 공격·시나리오 승리 재현 검사는 건너뛰었다.
- 남은 것: 후보 테이블 per-type 대조와 메뉴 소비량 확정(0.460) 후 프레임 반영·검증, Pop·농지 배치·공장체·소유자 라이브 연결, Objects·장부·그래프, brain 실생성자·요새 이미지·메뉴 미션 시작, 34경로 L2 대조(M1, 4/34).

#### 0.458 — 프리-Generate 소비 목록 확정 (2026-11-12, HJOW-Athlon)

- 범위: 0.4 순서 2(R4 실제 호스트·월드 조립)의 예순다섯 번째 단위. 0.457 단일화 뒤에도 남은 프레임 미수렴(78/1679)의 원인을 확정한다. 제품 코드 변경 없음(조사·검증 단위이며 임시 계측은 원복했다).
- 조사(증거): ① dn 측 감사(임시 계측, 실행 후 제거): game0=4190983114(801회 사전 소비, reseed로 무효) → SP 후=1839122055(=advance^353(seed), 수계산 일치) → Generate 전후 무변화 → 덤프=advance^963 → 지형 소비 610회. ② cpp 측 절차: SP 초기화는 클라이언트 시작 시 1회(`ClientMain.cpp` 349행, 세션 독립분기는 `RawCampaignSession.cpp` 1090행) — dn과 같은 353회. ③ shift 상관 ±1~±5000 무응답 — 단순 오프셋 아님. ④ 메뉴 ambient가 매 프레임 공유 난수를 소비(`ClientMain.cpp` 601~602행 "메뉴 여부와 무관") — dn 직접 진입에는 메뉴 단계가 없어 이 소비가 빠진다. ⑤ SP offset +6 = 게임플레이 2회전(44→50·tracking 미연결은 R6 잔여)으로 seed 절차가 아님을 확정. ⑥ CRT 동일 절차·동일 출력(296677442 양쪽) — reseed 사슬 parity의 결정적 증거. ⑦ 프레임 65 vs 90 덤프 비교(cpp f065/f090, dn f065/f90): dn 1679행 동일(정적), cpp 1921/1941행(게임플레이 진행 중) — 분기가 적재 시점에 이미 갈려 있음을 확정. 결론: streams+order+tables는 동일하고, 프리-Generate 소비 목록에 cpp 특유 항목(메뉴 ambient 등)이 있어 0.459에서 수량을 확정한다.
- 검증: 깨끗한 빌드(경고 0·오류 0) 뒤 집중 검사로 확인했다(새 코드 없음). 임시 계측의 삽입·삭제로 제품 코드가 HEAD와 동일함을 `git status` 빈 출력으로 확인했다. 게임 실행 있음(예외 PC): cpp 고정시각 paired f065/f090 + dn f065/f90 4종 덤프(종료 코드 0). `originals/` 변경 없음, 추적 `assets/game-data/d/options.cfg`는 되돌렸다. paired 기준선 `extracted/dotnet-l2-fixed/`에 f065·dn f065/f90 덤프를 추가했다(Git 제외).
- PORT_MAP, fixture 변화 없음(대응행 추가 없음). 함께 옮긴 10차 NB 수정 없음. 외부 접속 목록 변경 없음.
- 보호: originals, originalCD, AGENTS.md, README 개요, cpppj, LEFT_JOBS.md 미수정(조사 단위라 제품 코드 변경 없음). 사제 포획/희생·특정 적 공격·시나리오 승리 재현 검사는 건너뛰었다.
- 남은 것: 메뉴 ambient 소비량 확정·선택 테이블 대조(0.459), Pop·농지 배치·공장체·소유자 라이브 연결, Objects·장부·그래프, brain 실생성자·요새 이미지·메뉴 미션 시작, 34경로 L2 대조(M1, 4/34).

#### 0.457 — 난수 단일 스트림 연결 (2026-11-12, HJOW-Athlon)

- 범위: 0.4 순서 2(R4 실제 호스트·월드 조립)의 예순네 번째 단위. 0.456 결론(공유 스트림 분리 탓)에 따라 포팅 모듈의 난수를 공유 `GameRandom`으로 단일화한다. 레거시 경로는 건드리지 않는다.
- 구현: `RawSpStore`·`RawTerrainStamp`·`RawNormalTerrainWorldBinding`(Generate/StampContext/GenerateOriginal)·`RawFortRandomPlacement`·`RawFarmCluster`·`RawFortObjectLoad`·`RawFrameChoose`·`RawFortFinishOwner`·`RawIslandPopulation`·`RawPriestRegen`·`RawTerrainBitmap`·`RawFortMoneySave`·`RawAddRandom`의 `NetstormRandom`을 `GameRandom`으로 교체(알고리즘 동일: 0x10003+3·상위 16비트·0이면 0x0BAD0BAD, 근거 cpppj `RawCampaignSession.cpp` 498행 `같은 실제 Client 객체`). 호출 측 `Next(int)`→`Next((uint))`+캐스트, `Next(min,max)`→동등식으로 맞췄다. `HostSession`의 `_spRandom`·`TerrainRandom`을 삭제하고 SP 저장소·Generate에 세션 공유 `Game`을 직접 연결했다. 은퇴: `HostTerrainRandomTests`(분리 단정 2건, 대체 없이 삭제 — 공유 전환이 전제 자체를 바꾸므로 은퇴 목록에 남긴다). `NetstormRandom` 클래스에 레거시 전용 주석을 달았다.
- C# 모양 차이: `GameRandom.Next(uint)`가 `uint`를 돌려주므로 인덱스·비트 연산 자리에 `unchecked((int)...)` 캐스트를 둔다. 동작·순서는 원문과 같다.
- 검증: 깨끗한 전체 리빌드(경고 0·오류 0) 뒤 Client 전체 1,133건+Core 포팅 111건 통과(실패 0, NETSTORM_DATA 지정). L1 fixture 대조는 그대로 통과한다(동일 알고리즘). 게임 실행 있음(예외 PC): `--fixed-unix-seconds 1760000000` paired 실행(종료 코드 0) — 덤프 구조 동일(1,955행·슬롯 1,679·SID 집합 동일·비슬롯 행 무변화), Game 행이 시드 고정값에서 전진값으로 바뀌어 공유 소비가 동작함을 확인했다(4097495276→3186620043). 프레임은 미수렴(78/1679) — 프리-Generate 소비 목록이 덜 채워졌거나 선택 테이블 차이이며, 단일 스트림 자체는 전제 조건이다(0456 덤프와 1452행 동일·227행 변경으로 결선이 실제로 먹혔음을 확인). `originals/` 변경 없음, 추적 `assets/game-data/d/options.cfg`는 되돌렸다.
- PORT_MAP, fixture 변화 없음(대응행 추가 없음). 함께 옮긴 10차 NB 수정 없음. 외부 접속 목록 변경 없음.
- 보호: originals, originalCD, AGENTS.md, README 개요, cpppj, LEFT_JOBS.md 미수정. 사제 포획/희생·특정 적 공격·시나리오 승리 재현 검사는 건너뛰었다.
- 남은 것: 프리-Generate 소비 목록 확정(0.458: Objects 이전 소비·AI 생성·로그인 잔여와 선택 테이블 대조), Pop·농지 배치·공장체·소유자 라이브 연결, Objects·장부·그래프, brain 실생성자·요새 이미지·메뉴 미션 시작, 34경로 L2 대조(M1, 4/34).

#### 0.456 — 고정시각 paired 대조와 덤프 CRT 행 수정 (2026-11-12, HJOW-Athlon)

- 범위: 0.4 순서 2(R4 실제 호스트·월드 조립)의 예순세 번째 단위. 0.455 잔여(농지 121·소유자·Pop·공장체·프레임 난수)의 메커니즘 확정, 덤프 CRT 행을 실제 소비 스트림으로 수정, `--fixed-unix-seconds` paired 대조 방법론 확립.
- 조사(코드 변경 없음, 증거): ① 생성 순서·셀 집합 동일 증명 — 정렬 1673행 중 121↔97 뒤집힘 152곳만 다르고 나머지 동일, Pop 좌표 집합 양쪽 1638개 바이트 동일. 농지 121은 postprocess 배치 산물(동일 셀에 dn은 97)이며 배치 파이프라인(R6 MayPlace 공간 판정)이 필요해 0.456에서 잇지 않는다. ② 소유자는 region+1을 적재 시점에 가짐(121은 0, (0,1) 853·(1,2) 500 대 (0,0) 101·(1,0) 51). writer는 Objects/AI-setup 잔여(보드 6번). f090=f240 동일이라 프레임 후반부가 아니다. ③ CRT parity 확정 — chain(1760000000)을 손으로 재계산해 dn TerrainCrt=296677442와 cpp 덤프 CRT가 일치함을 확인(수계산: seed 4097495276·offset 44·Variants 일치). ④ Game/SP-offset/frame divergence는 단일 공유 스트림 분리가 원인 — cpp는 SP·객체·지형·AI가 같은 GameRandom을 공유하고(`RawCampaignSession.cpp` 498행) dn은 Game·SP·지형 3분리라 SP 초기화 33회 소비 등이 어긋난다. 0.457 단일화 대상(NetstormRandom≡GameRandom 알고리즘 동일). ⑤ 33회 선소비 모방 실험·스트림 shift 상호상관(delta ±1·±33·±100 모두 무의미, delta 0이 79로 최대)으로 단순 오프셋 가설을 기각했다. ⑥ Pop 차단 조건 확정 — vtable Supports(0 거짓)·SHP HashLevel dims·postPop Validate가 없어 스텁 유지. ⑦ 프레임 선택 알고리즘·성장 L1은 이미 대조됐으므로(terraingrowth-x86 108행·GrowthBytes) 차이는 소비 순서 문제다.
- 구현: `HostSessionDump` world-1 난수 행을 세션 `TerrainCrt`로(미소비 호스트 Crt 상수 1 대신, 근거 cpppj `SessionStateDump.cpp` 66·116행). 신규 검사 `HostDumpWritesWorld1SessionStreams`(세션 스트림 덤프 단정).
- 검증: 깨끗한 전체 리빌드(경고 0·오류 0) 뒤 Client Host·Dump 69건+SessionStateDump 11건 통과(실패 0, NETSTORM_DATA 지정). CRT 행 원복 변이에서 신규 검사 1건 실패로 검출력을 확인한 뒤 원복하고 다시 통과했다. 게임 실행 있음(예외 PC): `--fixed-unix-seconds 1760000000` paired 실행(cpp f090/f240 + dn, 종료 코드 0). 덤프 CRT 행이 양쪽 296677442로 일치하고 SP 1번 값(1161527296)도 일치하나 SP offset(50 대 44)·Game 행·프레임(79/1679)은 갈린다. 비슬롯 행은 0.455와 동일(프레임 지터 1행 제외). `originals/` 변경 없음, 추적 `assets/game-data/d/options.cfg`는 되돌렸다. paired 기준선은 `extracted/dotnet-l2-fixed/`에 두었다(Git 제외).
- PORT_MAP, fixture 변화 없음(대응행 추가 없음). 함께 옮긴 10차 NB 수정 없음. 외부 접속 목록 변경 없음.
- 보호: originals, originalCD, AGENTS.md, README 개요, cpppj, LEFT_JOBS.md 미수정. 사제 포획/희생·특정 적 공격·시나리오 승리 재현 검사는 건너뛰었다.
- 남은 것: 난수 단일 스트림 연결(0.457), Pop·농지 배치·공장체·소유자 라이브 연결, Objects·장부·그래프, brain 실생성자·요새 이미지·메뉴 미션 시작, 34경로 L2 대조(M1, 4/34).

#### 0.455 — Generate 라이브 연결과 첫 덤프 대조 (2026-11-12, HJOW-Athlon)

- 범위: 0.4 순서 2(R4 실제 호스트·월드 조립)의 예순두 번째 단위. Load.Generate 갈고리→`GenerateOriginal` 라이브 연결과 world-1 덤프 첫 대조를 한다. 공간 Pop·군집 배치·공장 생성자는 다음 단위다.
- 구현: `RawGameAuxState.PuzzlePieceType=107` 추가(근거 cpppj `RawCampaignSession.cpp` 621~622행 `localPiece`). `HostSession`에 세션 소유 `TerrainBookkeeping`+`BuildTerrainStampHooks`(실제 Create·SetOwner·SetRegion·ChunkRegion, Pop은 공간 등록 미연결로 기록만, 근거 `RawTerrainWorldBinding.cpp` 8~22행) 추가. `BuildWorldLoad`의 조각·지역없음 stub(`() => 0u`·`() => 0`)을 세션 실제값(`GameAux.PuzzlePieceType`·`TerrainGlobals.NoRegion`, 근거 cpppj 621·628행)으로 교체. `ClientHostGame.StartMission`에 Territory 실바이트·진단 결선·Generate 라이브 어댑터 연결(농지 place는 기록만, 섬 0개라 호출 없음). dryrun 기대값 `generate:0`→`generate:127` 갱신(의도적 실제값 결선 변경, 근사 수정 아님). 신규 검사 `HostTerrainBindingTests.GenerateLiveWritesTerrainSlotsDeterministically`(실슬롯 바이트·진단 무발화·결정성).
- C# 모양 차이: 원문의 공장 Create·소유자 분배·지역 지정을 세션 소유 풀·장부·타입 표로 직접 잇는다. Pop 공간 등록은 잇지 않는다.
- 검증: 깨끗한 전체 리빌드(경고 0·오류 0) 뒤 Client 전체 1,134건+Core 지형 52건 통과(실패 0, NETSTORM_DATA 지정). 생성 타입 바이트 제거 변이에서 신규 검사 1건 실패로 검출력을 확인한 뒤 원복하고 다시 통과했다. 게임 실행 있음(예외 PC): `--mission thewarbegins --handle Youpson` 260프레임 실행(종료 코드 0)으로 덤프가 282→1,955행이 됐다(슬롯 1,679행 출현). cpp 기준선(f090, 4,174행)과 대조: ① SID 집합 완전 일치(내 것만 0개, 기준선만 262개=서버 186+저수준 76=Objects 잔여) ② 지형 개수 일치(1,673 양쪽) ③ SetRegion WORD 일치 ④ 비슬롯 행(플레이어·동맹·SP·그래프 239/255) 무변화. 남은 차이: Pop(void·좌표·레벨·체인), 공장 생성자·vtable, 프레임 난수 parity, 농지 121 152셀·소유자 후속 지정 메커니즘(0.456 조사), 그래프 16행(RebuildGraph 잔여). `originals/` 변경 없음, 추적 `assets/game-data/d/options.cfg`는 `--set` 저장분만 바뀌어 되돌렸다.
- PORT_MAP, fixture 변화 없음(대응행 추가 없음). 함께 옮긴 10차 NB 수정 없음. 외부 접속 목록 변경 없음.
- 보호: originals, originalCD, AGENTS.md, README 개요, cpppj, LEFT_JOBS.md 미수정. 사제 포획/희생·특정 적 공격·시나리오 승리 재현 검사는 건너뛰었다.
- 남은 것: Pop 공간 등록·농지 배치·공장 생성자 라이브 연결, Objects·장부·그래프 라이브 연결, brain 실생성자·요새 이미지·메뉴 미션 시작, 34경로 L2 대조(M1, 4/34).

#### 0.454 — 지형 바인딩 조립과 드라이런 (2026-11-12, HJOW-Athlon)

- 범위: 0.4 순서 2(R4 실제 호스트·월드 조립)의 예순한 번째 단위. 0.453 입력 결선에 이어 바인딩 본체 조립(`BindGenerationInputs`)과 기록 효과 Generate 드라이런까지 잇는다. Load.Generate 갈고리·도장 실제 효과 연결은 다음 단위다.
- 구현: `HostSession`에 세션 소유 `TerrainFarm`·`TerrainBinding`과 `BuildTerrainBinding(types, diagnostics, islandHooks)` 추가(세션 ChunkMap·섬 런타임·지형 전역·배치·스탬프 상태·타입 표·풀·조각 상자·프레임 188·진단 묶음, 입력 바인딩 후 보관, 근거 cpppj `RawCampaignSession.cpp` 1829~1853행). `ClientHostGame.StartMission`에 드라이런 조립 연결(기록용 섬 갈고리, 덤프 무영향). 신규 검사 `HostTerrainBindingTests` 3건(조립·입력 바인딩·장치 거부, 기록 효과 일반 분기 드라이런의 도장·결정성).
- C# 모양 차이: 원문의 바인딩 지역 변수·장기 참조를 세션 소유 상태+읽기 대리자로 둔다. 진단은 호출자가 준 묶음을 복사해 가진다(바인딩 기존 모양). 드라이런 도장 효과는 검사 전용 기록 갈고리이며 시작 흐름에는 잇지 않는다.
- 검증: 깨끗한 전체 리빌드(경고 0·오류 0) 뒤 신규 포함 집중 10건 통과(Client HostTerrain* 10, 실패 0, NETSTORM_DATA 지정). 바인딩 묶음 제거 변이에서 신규 검사 2건 실패로 검출력을 확인한 뒤 원복하고 다시 통과했다. 게임 실행 있음(예외 PC): `--mission thewarbegins --handle Youpson` 260프레임 실행(종료 코드 0)의 world-1 덤프가 282행이며 0.453 덤프와 바이트 동일(재실행 1회는 프레임 90·재실행 2회는 91으로 0.440의 기존 1프레임 지터, 내용 행 281행 동일)해 조립이 중립임을 확정했다. `originals/` 변경 없음, 추적 `assets/game-data/d/options.cfg`는 `--set` 저장분만 바뀌어 되돌렸다.
- PORT_MAP, fixture 변화 없음(대응행 추가 없음). 함께 옮긴 10차 NB 수정 없음. 외부 접속 목록 변경 없음.
- 보호: originals, originalCD, AGENTS.md, README 개요, cpppj, LEFT_JOBS.md 미수정. 사제 포획/희생·특정 적 공격·시나리오 승리 재현 검사는 건너뛰었다.
- 남은 것: Load.Generate 갈고리 연결 + 도장 실제 효과 라이브 연결(Objects·장부·그래프는 그 다음), brain 실생성자·요새 이미지·메뉴 미션 시작, 34경로 L2 대조(M1, 4/34).

#### 0.453 — 지형 바인딩 입력 값 연결 + 전체 회귀 재완주 (2026-11-12, HJOW-Athlon)

- 범위: 0.4 순서 2(R4 실제 호스트·월드 조립)의 예순 번째 단위. 0.450 대조표의 입력 15종 중 새 경로에 없던 11종 가운데 R6·성장 잔여 3종을 제외한 8종(`TerrainEnabled`·밀도 3종·`ownerMode.battle`·`bridgeState.mission`·전투 옵션 5·6번·`AiDebug` checking)을 세션 소유로 결선하고, 0.4 순서 1-1의 전체 회귀를 0.453 포함으로 재완주한다.
- 구현: `RawGameAuxState`에 `TerrainEnabled`(1)·`ArchipelagoMinimum`(15)·`ArchipelagoMaximum`(35)·`ArchipelagoPercent`(0) 추가(cpppj `o/RawGameAuxState.h` 초기값). 신규 `RawWorldGeyserSpacing.Spacing`(선택 0~3 → 3·4·5·6칸, 범위 밖 거부, 근거 `o/RawWorldGeyserBounds.cpp` 0041ca10). `HostSession`에 `GameAux`·`OwnerMode`(battle=true)·`BridgeState`(CampaignLinks 타입값·battle/mission=true)·`BattleOptions`(87바이트 `RawMissionBattleOptions.Reset` 초기화)·`AiChecking` 소유와 `BuildTerrainGenerationInputs()`(Enabled·Battle·Mission·Layout·Spacing·Span·Minimum·Maximum·Percent·Archipelago·Subscriber·Growth.Checking 결선, 근거 `RawCampaignSession.cpp` 1831~1853행) 추가. `PlayerBounds`(R6)·세션 로그 0x1d(R6)·성장 assertion은 미연결(null)로 두어 `GenerateOriginal`이 명시 거부한다. 신규 검사 `HostTerrainInputsTests` 4건(기본값 소유·간격표·공유 결선·값 추종).
- C# 모양 차이: 원문의 `uint32_t&`·`bool&` 장기 참조를 세션 소유 값+읽기 대리자(`RawCheckingFlag`·`Func`)로 빌린다. `Span` 등 상자는 `GenerationInputs` 소유 상자에 값을 동기화해 바인딩의 `BindGenerationInputs` 공유 방식과 맞춘다.
- 검증: 깨끗한 전체 리빌드(경고 0·오류 0) 뒤 신규 포함 집중 52건 통과(Client HostTerrainInputs 4·주변 7 + Core 지형 37·GameAux 4, 실패 0, NETSTORM_DATA 지정). 간격표 6→7 변이에서 신규 검사 2건 실패로 검출력을 확인한 뒤 원복하고 다시 통과했다. 전체 회귀 완주(백그라운드): Assets 366·Core 1,945·Client 1,130(합계 3,441, 실패 0, 종료 코드 0, NETSTORM_DATA 지정) — 새 기준선(직전 0.432는 3,422). 게임 실행 있음(예외 PC): `--mission thewarbegins --handle Youpson` 260프레임 실행(종료 코드 0)의 world-1 덤프가 282행으로 이전 실행분과 동일해 세션 상태 추가가 중립임을 확정했다. `originals/` 변경 없음, 추적 `assets/game-data/d/options.cfg`는 `--set` 저장분만 바뀌어 되돌렸다.
- PORT_MAP, fixture 변화 없음(대응행 추가 없음). 함께 옮긴 10차 NB 수정 없음. 외부 접속 목록 변경 없음.
- 보호: originals, originalCD, AGENTS.md, README 개요, cpppj, LEFT_JOBS.md 미수정. 사제 포획/희생·특정 적 공격·시나리오 승리 재현 검사는 건너뛰었다.
- 남은 것: 바인딩(`BindGenerationInputs`) 드라이런 + Generate 라이브 연결·덤프 대조(슬롯·객체 행 출현), Objects·장부·그래프 라이브 연결, brain 실생성자·요새 이미지·메뉴 미션 시작, 34경로 L2 대조(M1, 4/34).

## 9. AGENTS 반영 기준과 이관 기록

<a id="agents-reflection"></a>

### 9.1 AGENTS.md 반영 기준 (재확인 2026-11-12, 마지막 AGENTS 변경 `85551c6c2` 이후 변경 없음)

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
| `60226ea93` 11차 난이도·12차 스커미시·13차 기타 언어, dotnetpj 범위 **4~13차**, `Copyright 2026 HJOW` | 1.3 11~13차, 0.4 1-3 카피라이트 |
| `610670750` cpppj 플랫폼 명시 | cpppj 전용 |
| `85551c6c2` 14차 QuickJS 추가, dotnetpj 범위 **4~14차**, webpj 예외 변경(4차 WebMCP·5차 localStorage·14차 불필요·12차 대상) | 1.3 14차(cpppj 1301 탑재 계약을 같은 계약으로), 0.2 R8·0.4 7단계 범위. webpj 예외 변경은 dotnetpj 변화 없음 |

<a id="dotnet-archive-log"></a>

### 9.2 이관 기록 — LEFT_JOBS.dotnetpj.old.md

**2026-11-12 이관(HJOW-Athlon, 문서만, 사용자 요청):** 단위 기록 0.342~0.452와 0.2~0.4의 단위별 나열을 정리했다. 정리 직전(`315be9a59 / 20261112-72 DN`) 전체 원문을 [보관 파일 끝](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261112-through-0452)에 그대로 보관했다(원문 SHA-256 `654c0ee6c07e42b3059e7f644bf01fc5e4e8011a9250b3d775efe456a7d9b2ac`, 보관본을 LF로 되돌려 원문과 바이트 일치 확인). 보관 파일 머리의 보관본 목록에 새 링크를 더했다.

- **본문에서 뺀 것:** 단위 기록 0.342~0.452(111건) 전체, 0.3 '검사' 행과 0.4 1-1 행의 단위별 집중 검사 나열(0.345~0.452), 0.4 순서 2 행의 단위별 진행 이력(0.390~0.452)과 순서 3 행의 L4a 장면별 경과(0.380~0.389), 0.3 PORT_MAP 행의 행별 `base` 갱신 이력, 0.2 R4 행의 단위 나열, 0.4 1-3·1-4 행(완료 참고로 줄임), 9.2의 지난 11-11 이관 3건 상세(아래 요약).
- **새로 넣은 것:** 0.1 한 줄 요약, [0.4 R4 진행판](#r4-board)(묶음별 연결 현황·cpp 기준선·검증 방법·남은 조각 순서), 0.3의 전체 회귀 주기 도래 표시, 0.4 1-2의 남은 9건 목록, 7.1 색인 3행.
- **바로잡은 것:** cpppj 기준 제품 1301 → 1303, 14차 'cpppj 미착수' 표기(1.1·1.3·7.2·9.1) → 1301 엔진 탑재·연동 범위 D7 대기, PORT_MAP 행 수(691줄 → 688행 재집계, 0.394 `Player.cpp/.h` 반영), 0.2 R0·R8 행(재검토 종결·앞서 이식한 요구사항 코드), 1.2에 1302·1303 정적 확인 추가, 2.4·2.5의 낡은 진행 문장.
- 빌드·검사·게임 실행·디컴파일은 하지 않았다. `LEFT_JOBS.md`·cpppj·AGENTS.md는 수정하지 않았다.

**2026-11-11 이관 3건(요약, 원문은 [보관 파일 0.341까지](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261111-through-0341)):** ① 1300 반영 정리 — 단위 기록 0.280~0.341을 빼고 0.4 남은 작업 순서표·0.5 결정 표·1절 cpppj 마감분 반영·0.1 이식 범위 집계·9.1 AGENTS 반영 표를 새로 만들었다(원문 SHA-256 `698670bf211d9ec89d63ae2ffca976e9c28e87dd52c74288fb1b2ef58d785209`). ② 같은 날 결정 반영 — 사용자가 D1~D5를 정했고(D1·D3~D5 권고안, D2 "새 경로가 이름을 이어받고 작업이 끝나면 기존 경로 제거") 0.5와 관련 문장을 고쳤다. 전환 시점(R5)과 기존 경로 삭제 시점(R6)은 이 결정에 대한 AI 해석이다. ③ 같은 날 AGENTS 14차 반영 — `85551c6c2`(dotnetpj 범위 4~14차)를 머리말·0.2·0.4·1.1·1.3·3.2·7.2·9.1에 반영했다.

**이전 이관(요약, 상세는 각 보관본 머리):** [2026-11-10 — 0.279까지](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261110-through-0279)(HJOW-Athlon, AGENTS 8차 시나리오 편집·5차 확장·9차 알려진 버그·글꼴 외부 배포·평가 기준 반영), [2026-11-04 — 0.154까지](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261104-through-0154)(HJOW-Athlon), [2026-11-02 — 0.126까지](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261102-through-0126)(HJOW-Athlon), [2026-10-31 — 0.73까지](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261031-through-073)(HJOW-X3D), 2026-10-28 재수립 이관(VM-W11-Codex: 2026-10-22 이전 계획, 날짜별 완료·인계 절, 옛 0~11·14절).

**앵커:** 2026-10-28에 옮긴 절의 `<a id>`(`dotnet-current-plan-20261022`·`sid-session-plan`·`legacy-dotnet-history`·`legacy-dotnet-031`~`107` 등)는 보관 파일에 같은 이름으로 남아 있다. `LEFT_JOBS.old.md`·`LEFT_JOBS.webpj.md`·`docs/`의 기존 링크가 이 안내로 오도록 같은 이름의 빈 앵커를 바로 아래에 둔다. 그런 링크로 들어왔다면 보관 파일의 같은 앵커를 본다. 옛 절 제목 앵커(`04-…`·`05-…`·`07-…`·`08-…`·`09-…`)는 이 문서의 대응 절 앞에 남겼다.

<a id="018-단위-기록--r0-4-네-번째-묶음-사제-panic기도와-회복-허용-2026-10-28-hjow-athlon"></a><a id="dotnet-current-plan-20261022"></a><a id="legacy-dotnet-031"></a><a id="legacy-dotnet-032"></a><a id="legacy-dotnet-033"></a><a id="legacy-dotnet-038"></a><a id="legacy-dotnet-039"></a><a id="legacy-dotnet-040"></a><a id="legacy-dotnet-041"></a><a id="legacy-dotnet-042"></a><a id="legacy-dotnet-043"></a><a id="legacy-dotnet-044"></a><a id="legacy-dotnet-045"></a><a id="legacy-dotnet-046"></a><a id="legacy-dotnet-047"></a><a id="legacy-dotnet-048"></a><a id="legacy-dotnet-049"></a><a id="legacy-dotnet-050"></a><a id="legacy-dotnet-052"></a><a id="legacy-dotnet-053"></a><a id="legacy-dotnet-055"></a><a id="legacy-dotnet-056"></a><a id="legacy-dotnet-057"></a><a id="legacy-dotnet-058"></a><a id="legacy-dotnet-059"></a><a id="legacy-dotnet-064"></a><a id="legacy-dotnet-065"></a><a id="legacy-dotnet-066"></a><a id="legacy-dotnet-067"></a><a id="legacy-dotnet-068"></a><a id="legacy-dotnet-069"></a><a id="legacy-dotnet-070"></a><a id="legacy-dotnet-071"></a><a id="legacy-dotnet-072"></a><a id="legacy-dotnet-073"></a><a id="legacy-dotnet-074"></a><a id="legacy-dotnet-075"></a><a id="legacy-dotnet-077"></a><a id="legacy-dotnet-078"></a><a id="legacy-dotnet-079"></a><a id="legacy-dotnet-080"></a><a id="legacy-dotnet-082"></a><a id="legacy-dotnet-083"></a><a id="legacy-dotnet-084"></a><a id="legacy-dotnet-088"></a><a id="legacy-dotnet-092"></a><a id="legacy-dotnet-093"></a><a id="legacy-dotnet-094"></a><a id="legacy-dotnet-096"></a><a id="legacy-dotnet-097"></a><a id="legacy-dotnet-098"></a><a id="legacy-dotnet-099"></a><a id="legacy-dotnet-100"></a><a id="legacy-dotnet-101"></a><a id="legacy-dotnet-102"></a><a id="legacy-dotnet-103"></a><a id="legacy-dotnet-104"></a><a id="legacy-dotnet-105"></a><a id="legacy-dotnet-106"></a><a id="legacy-dotnet-107"></a><a id="legacy-dotnet-history"></a><a id="sid-session-plan"></a>
