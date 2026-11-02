# LEFT_JOBS — cpppj 및 공통 인수인계

이 문서는 cpppj와 공통 사항의 **현재 상태, 남은 작업, 유효한 사용자 결정**만 담는다. 단위별 상세와 증거는 `docs/exe/cpp-*.md`·`cpppj/*.json`, 과거 기록은 [LEFT_JOBS.old.md](LEFT_JOBS.old.md), dotnetpj는 [LEFT_JOBS.dotnetpj.md](LEFT_JOBS.dotnetpj.md), webpj는 [LEFT_JOBS.webpj.md](LEFT_JOBS.webpj.md)에 있다.

단위를 마치면 0.1절의 값과 2.1절의 해당 영역 행을 덮어써 갱신한다. 단위별 진행 기록을 이 문서 위쪽에 쌓지 않고 단위 문서에 남긴다.

**2026-11-02 정리:** 현행 값·남은 작업·유효한 결정 중심으로 정리했다(5절 마지막 항목). AGENTS.md 최근 변경의 반영 현황은 [1.3절](#agents-reflection-20261102), 완료 항목의 근거는 2.1절에 있다. 정리 직전 전체 원문은 [LEFT_JOBS.old.md](LEFT_JOBS.old.md#archive-20261102-before-cleanup)에 그대로 보관했다.

## 0. 재개 전 확인 사항

### 0.1 현재 값 (2026-11-03, 단위777 완료·목표 진행 중)

| 항목 | 값 |
| --- | --- |
| 진행 상태 | **777 완료·전체 목표 진행 중.** 776은 `7f903eba`로 커밋했다. 다음은779 checking 공유 소비자와783 별도 부모다. [777 기록](docs/exe/cpp-type-live-read-20261103.md)과 [재개 준비 인계](docs/exe/cpp-pause-handover-20261103.md)의 필수 교정을 따른다. |
| 현재 제품 | **실행 제품777 / 표시11.01 유지.** TYPE를 같은 고리·현재 FILE/키로 읽고 기존 Decode/Parse·CLI/CD/snapshot은 보존했다. 원문47입력/94회 재관측이 일치했고 집중13개(새5개) 실패0이다. 새 게임/CLI/MP0·마지막GUI746·전체34/ZIP645·644는 별도다. |
| EXE SHA-256 | `94e79d5aabbfdb8a98fb6e889b8ad3d8d5e5da4c8bdf10dc0223692947ea192d` (777 / 11.01) |
| ZIP SHA-256 | 645에서 만든644 제품의 `6485febc36080dc90e9eec43965a842f85d39e81c82a003a96b83b34b3694b73` (1376항목/게임 자산1358개). 실제 한글😀 경로에 압축 해제해 최종34를 실행했다. |
| 등록 C++ 검사 수 | 최신777 집중13개(새5개)/실패0. 실행대상66개/전체71대상·새대상0 유지이며 전체 C++ 재실행은 아니다. |
| 최신 전체34 | 단위645, 644 제품 기준 최종34/34. 최소 비정지 시각 증가20.843초·창DC RGB·Native Frame 일치. 최초 입력 누락34실패/재검증34통과와 준비2회를 포함한 총70프로세스를 구별한다. |
| 전체 C++ 마지막 실행 | 단위549. 3389개 중 7개 검사가 실패했다(580단언). 이후 입력 정정 뒤 집중 검사만 통과했고 전체 재실행 기록은 없다. 마지막 전체 통과는 단위510(508 제품, 3337개, 실패 0)이다. |
| 보호 기준 | 기존 **2712개** 모두 불변·새fixture2 포함2714개/fixture894. 다음 기준은777의protected-after.json이다. |
| OS/플랫폼 요구 | 사용자61067075: cpppj Windows10/11 x86-64(AMD64) 대상, Windows11 ARM 지원도 고려. 현재 EXE는PE0x8664/AMD64이며 ARM 빌드/실기 미검증이다. |
| **마지막 전체 정밀 디컴파일** | **HJOW-Athlon (192.168.0.94) / 2026-10-23** |

### 0.2 재개 절차

1. AGENTS.md, 이 문서, LEFT_JOBS.dotnetpj.md를 다시 읽는다. webpj 작업이면 LEFT_JOBS.webpj.md도 읽는다. `git fetch`로 다른 PC의 커밋을 확인한다. push는 하지 않는다.
2. `hostname`이 위 디컴파일 호스트와 다르면, cpppj 작업이나 디컴파일 분석 전에 `tools/ghidra/refine_all.ps1`로 다시 디컴파일하고 완료 호스트를 이 문서와 작업 내역 문서에 갱신한다. cp949 오류가 나면 `PYTHONUTF8=1`, `PYTHONIOENCODING=utf-8`을 설정한다. HJOW-X3D의 `extracted/refined/originals/`는 2026-10-27에 중단된 불완전 자료(`raw.c`·`functions.tsv`·`conventions.tsv` 없음)이므로 완료 산출물로 쓰지 않는다.
3. 실제 게임 구동과 별도 창이 뜨는 작업은 AGENTS.md를 따른다. 확인 없이 진행할 수 있는 시스템은 vm-debian-codex(10.0.0.15), VM-W11-Codex(10.0.0.17), HJOW-Athlon(192.168.0.94)뿐이다. **HJOW-X3D를 포함한 그 밖의 PC에서는 실행 목적과 필요성을 설명하고 명시적 확인을 받는다.** 허용받으면 해당 작업 단계가 끝날 때까지 유지된다. 사용자가 원본 수동 컨트롤이나 녹화 분석을 직접 요청한 작업 단계도 확인이 필요 없다.
4. 빌드·검사·GUI 원시 자료는 작업 PC의 `extracted/`(Git 제외)에만 있다. 커밋되는 보고서는 `cpppj/*.json`, 작업 문서는 `docs/exe/cpp-*.md`다. 다른 PC에 원시 자료가 있다고 가정하지 않는다.
5. 다른 세션의 dotnetpj·webpj 변경, staged 파일, 실행 출력은 이 작업에 포함하지 않는다. 단위마다 인계를 갱신하고 로컬 커밋한다.
6. 645 뒤의 실제 실행은 각 단위의 변경과 직접 관련된 기능별 확인이다. 646~671·673~675·679·681·683·686~694·698·701~703·705~713·715는 새 게임 실행0이고, 실제 실행이 있었던 672·676·677·678·680·682·684·685·695·696·697·699·700·704·714·716·717·718·719의 횟수와 최초 도구 입력 실패의 구별은 각 단위 문서를 따른다. 전체34 재검사나 자연 완주로 합산하지 않으며 소수 단위 뒤 반복하지 않는 사용자 지시(4절)를 유지한다.
7. 마지막 제품/집중 빌드는777의 `extracted/cpp-resume-777/`에 있다. Release 경고/오류0·집중13개 실패0, 원문47입력/94회 재관측이 일치한다. 최초 두 실행의 각4단언 실패는 반환0 물리 span의 검사 입력 대응을 교정했다. 기존47행 기대와 제품 코어는 불변이며 추가4행/8관측은 마지막94회 안의 Read 직후 물리다([777 기록](docs/exe/cpp-type-live-read-20261103.md)). 새 게임/CLI/MP0·기존2712불변/현재2714이다. 다음은779 checking 공유 소비자/두 필수 교정과783 부모·781/782 GIF/초기화면·784 입력꼬리·785 보조음향 및 남은 비멀티/호환성이다.
8. 720은 분석기만 Release 빌드/집중4개·MCP16도구를 확인했다. 게임 제품719 SHA는 유지한다. 원본 실기4수명과 cpppj1수명의 범위·오디오 초기 실패·WGC 실패·사용자 직접 종료 정정·CLI timeout을 [720](docs/exe/cpp-editor-graph-native-20261102.md)에서 구별한다. 큰 원시Pool/화면은extracted에만 있고 최초 원본 관측4개를 fixture로 고정했다.

### 0.3 유지되는 제한과 보류

- **AGENTS.md 요구의 반영 현황:** [1.3절](#agents-reflection-20261102)의 표를 따른다. 4차(로컬 MCP·16:9 최대1920×1080)와 7차(고해상도 FSR)는 후순위 미착수이며 AGENTS 자체는 수정하지 않았다. 11.01과 외부 접속 문서화는 아래 두 항목이 현행 지시다.
- **추가 AGENTS 지시(11.01,685 cpppj 적용):** 해당 변경 당시 SHA13880803…의 버전 표기11.01·Credits3항목/Netstorm 11.01 Patch Credits·제목NetStorm 11.01 Patch Credits·HJOW 개발/AI 사용 설명을 구현했다. 기존 창과 같은 Back/Cancel을 제공한다. 원문 내부 판본/기존10.72 Credits는 역사적 기준으로 유지하며 DN/WB는 각 작업에서 같은 지침을 적용한다.
- **갱신 AGENTS 지시(684 적용):** 로컬 멀티플레이 서버를 제외한 외부 도메인/IP 접속 기능은 [별도 문서](docs/exe/cpp-external-services-20261101.md)·[원문 목록](cpppj/external-endpoints-20261101.json)을 따른다. 종료된 서버의 클라이언트 유효성 검사/security.dat writer를 연결하지 않는다. 제품의 검증 getter·Not Validated 안내도 제외하며 성공1을 합성하지 않는다. 원문 참조 테스트만 명시true다. 사용자 선택 외부 도움말 링크와 로컬 자료/패킷 형식 검사는 별개로 유지한다.
- **재현하지 않는 검사:** 사제 포획/희생, 특정 적 공격, 시나리오 승리를 재현해야 하는 테스트는 건너뛴다. 사용자가 나중에 직접 플레이해 확인한다([TODO.md](TODO.md)).
- **사용자 확인 대상:** 모든 시나리오(Early Missions, Priest Training, 캠페인, 데모)의 자연 완주와 전체 동일성은 사용자가 직접 실행·플레이하며 테스트한다. AI 작업 대상에서 제외한다. 메뉴·진입·수 초 진행의 짧은 검증과 기능별 원문 대조는 계속 AI가 한다. 단계 전환은 1.1절의 기준을 따른다.
- **공중 3종:** 훨리기그·맨오워·더스트 데빌은 정적 분석과 코드 작업만 한다. 새 전용 생산·생성·우클릭·행동 관측/검사는 하지 않는다.
- **멀티플레이:** 싱글플레이·UI·호환성 뒤 디컴파일만 참고해 사전 구현한다. 신규 단위 검사, 게임 실행, 실제 통신 테스트는 하지 않으며 사전 구현과 검증 완료를 구별한다.
- **사용자 보류 항목:** 스펠 효과 도움말 그림의 추가 분석(그림 임시 숨김 유지, 단위530), 연녹색 직사각형 분석(원본에서도 보였다는 사용자 확인, 원격 데스크톱 원인 여부 미확정), 대응 `.english` 부재 개선은 사용자98b2f20c의8차로 이월한다.
- **아직 근거가 없는 범위:** 다른 모니터/DPI, 실제 앱 포커스 전환, 물리 키 입력, 실제 PCM 출력, 사용자 활성 전체화면의 물리 출력, 실제 CD 삽입. Windows 11 실기도 미검증이지만 단계 전환 조건이 아니다(1.1절).
- **복원 한계:** 단위612의 반사 Generate 입력은 원본 32비트와 현재판의 스택 배치·미정값 자체 일치를 보장하지 않는다. 기존 VFX 커널의 signed WORD/정상 표본 범위 계약도 유지하며, 양수·각도0·원색 입력 근거를 음수/회전/전체 픽셀로 확대하지 않는다.

## 1. 현행 목표

### 1.1 cpppj 단계 (AGENTS.md 기준)

| 단계 | 내용 | 현재 상태 |
| --- | --- | --- |
| 1차 | 원본을 디컴파일해 C++로 최대한 똑같이 복원한다. 기준은 10.78이며 멀티플레이·Outpust는 제외한다. 문서화할 때 10.72 관련 내용도 기록한다. | **진행 중.** 34경로 진입·진행은 가능하다. AI 작업으로 남은 것은 UI/Edit/시나리오/수명이다. 자연 완주와 전체 동일성은 사용자 확인 대상이다. |
| 2차 | Windows 10/11에서 오류 없이 실행한다. 전체화면 DirectDraw 오류와 전체화면 종료 후 재실행 오류를 수정한다. | **진행 중.** Windows 10 호환 전체화면과 저장 재시작은 확인했다. 남은 Windows 10 확인을 마치면 Windows 11 실기 검증 없이 다음 단계로 넘어간다. 10.72의 Windows 10 구동 불가 해결책은 10.72 작업 때 문서화한다. |
| 3차 | 멀티플레이·Outpust. 공식 서버는 없어졌으므로 TCP/IP 로컬 네트워크만 구현한다. | 전용 단계는 미착수. 채팅 MP 분기·707의 공유 의식 선택/비미션 UI·709의Zsend/송신장치처럼 일부만 정적으로 작성했고 신규 MP 검사는 하지 않았다. |
| 4차 | **로컬MCP**로 dotnetpj 분석을 위한 모든 게임 조작·화면 캡처·소리 포함 녹화. **16:9 최대1920×1080 지원**이며 이 단계에서 그래픽 크기 스케일 조정은 불필요하다. | 미착수. 기존 화면비 처리/원문BMP 수집과 별도의 완료 조건이다. |
| 5차 | **일반 권한·읽기 전용 설치 정상 동작.** 설정/진행은`%USERPROFILE%\Saved Games\Netstorm\Saves\`, 신규 맵은`CustomMaps\`다. 모든 자동 생성 파일/디렉터리(임시/백업/잠금/장부/캡처/진단 포함)와 대응 reader도 같은Netstorm하위로 옮긴다. 원래/사용자 맵 병행 읽기와 Edit 스크롤/무제한도 포함한다. | **구현 미착수.** 사용자aaafbbc4를733에서 반영하고12파일군/50정적후보를 고정했다. 실제 권한/전체 경로 검증은 남는다. |
| 6차 | 한국어 지원 | 미착수 |
| 7차 | Options의AA/FSR1.0 선택은 기본off. FSR 지원 환경에는2560×1440/3840×2160을 추가하고 해당 해상도 선택 시FSR 자동켜기·실질1920×1080에서 목표로 업스케일한다. [FidelityFX-FSR](https://github.com/GPUOpen-Effects/FidelityFX-FSR)를 참고한다. | 미착수 |
| 8차 | Edit 전용 시나리오 구성 UI(초기 자본/테크·적·동맹 등), 새 맵의 기본 english 자동 생성, english 부재 Test Battle 종료 개선 | **미착수.** 사용자98b2f20c의 AGENTS/TODO 갱신을728에서 반영했다.1~7차 뒤 진행한다. |

**단계 전환 기준 (2026-10-31 사용자 결정):**

- **자연 완주·전체 동일성:** 모든 시나리오(Early Missions, Priest Training, 캠페인, 데모)의 자연 완주와 전체 동일성은 사용자가 직접 실행·플레이하며 테스트한다. AI 작업 대상이 아니며, 단계에 남은 작업이 이것뿐이면 다음 단계로 넘어간다.
- **Windows 11:** Windows 10에서 오류 없이 구동되면 Windows 11에서도 문제가 없을 가능성이 높다고 본다. Windows 10 확인을 마치면 Windows 11 실기 검증 없이 다음 단계로 넘어간다.
- **제보 처리:** 사용자가 플레이 중 오류나 원본과의 차이를 제보하면 해당 항목을 AI 작업으로 올린다.

60·120프레임 지원과 CD 10.72 전체 복원은 별도 후순위다. cpppj는 Windows용만 개발한다. dotnetpj 후순위 요구에도 4~8차가 포함되며, webpj는 Edit·멀티플레이를 제외한다.

### 1.2 현재 목표 범위 ([TODO.md](TODO.md))

- Early Missions 6개, Priest Training 9개, 캠페인 전체 16개, 데모 3개를 사용자가 플레이할 수 있고 원본 10.78과 완전히 같게 동작하도록 한다. 자연 완주와 전체 동일성의 확인은 사용자가 한다(1.1절).
- Edit 메뉴의 커스텀 맵/시나리오, 메인 메뉴의 비멀티 항목, 좌측 덱·미니맵·다리 단축키 QWASZX·ESC 메뉴를 구현한다.
- Windows 10/11 호환성을 해결한다. 원본과 다른 호환 처리와 이유는 한국어 코드 주석에 명시한다. 전체화면, 4:3/16:9/16:10, 가장자리 스크롤 요구를 유지한다.
- 그 뒤 멀티플레이를 무시험으로 사전 구현하고 4~8차를 진행한다(98b2f20c의 AGENTS/TODO 갱신).
- 버전 표기는11.01이며 Credits 목록에 세 번째 Netstorm 11.01 Patch Credits와 동일한 형식의 메시지 창(제목NetStorm 11.01 Patch Credits, HJOW 개발/AI 사용 설명)을 추가한다(추가 AGENTS.md).
- 외부 도메인/IP 접속 기능을 로컬 서버와 구별해 문서화하고, 클라이언트 유효성 검사는 비활성화한다(갱신 AGENTS.md,684 적용).
- 개발 작업 단위마다 인수인계를 갱신하고 커밋한다. 커밋에 실패해도 작업은 이어 간다.

대상은 원본 `d/offical1~5.english`의 MissionBegin 순서와 `tell.DEMOVILLE`의 데모 순서다.

| 구분 | 미션 키 (원본 표시 이름) |
| --- | --- |
| Early Missions (offical1) | Tutorial1 (Bridge the Gap), Tutorial2 (Secret Workshop), Tutorial3 (Capture The Priest), Tutorial4 (Tactical Combat), Tutorial5 (Subtle Defense), Tutorial6 (Raw Power) |
| Priest Training (offical2) | bc1menu76, bc2bridging76, bc3acid_bar76, bc4bow_rush76, bc5walkers76, bc6mows76, bc7sun_bars76, bc8fury76, bc9ssun_bars76 |
| 캠페인 1장 (offical3) | TheWarBegins, MasterOfWhirligigs, SaveTheIsland, FragileFortune, ThunderingPower, DissolvedAlliance |
| 캠페인 2장 (offical4) | TheNoose, RainVsRain, RunForIt, GuardMyBack, Surrounded |
| 캠페인 3장 (offical5) | BreakingThrough, ToTheRescue, Vicious, EnemyTerritory, FinalConfrontation |
| 데모 | DemoStormRages (The Storm Rages), GuardMyBack (Demo of "Guard My Back"), DissolvedAlliance (Demo of "Dissolved Alliance") |

일반 31경로(Early 6 + Training 9 + 캠페인 16)와 데모 3개를 합쳐 34경로다. 같은 자원을 쓰는 GuardMyBack·DissolvedAlliance도 일반/데모를 따로 검증한다([대응 근거](docs/gameplay/demo-campaign-map.md)).

<a id="agents-reflection-20261102"></a>

### 1.3 AGENTS.md 최근 변경의 반영 현황 (2026-11-02, `98b2f20c`까지 반영)

AGENTS.md 자체는 수정하지 않는다. 아래 표는 최근 변경이 프로젝트별 인수인계에서 어떻게 다뤄지는지다. 과거 변경의 번복(FSR 3.1 → FSR 제거 → FSR 1.0)과 커밋별 대응 이력은 [정리 직전 원문](LEFT_JOBS.old.md#archive-20261102-before-cleanup)의 1.3절에 있고, 현재 기준은 가장 최근 커밋이다.

| 요구 (AGENTS.md 커밋) | cpppj | dotnetpj | webpj |
| --- | --- | --- | --- |
| **5차 전체 자동 파일·일반 권한 설치** (`aaafbbc4`) | 733에서 모든 생성 디렉터리/임시·복구·잠금·캡처·진단과 reader를 UserRoot 아래로 전환할 요구/정적 목록을 반영. 구현/실제 권한 검증은 미착수 | 4~8차 공통 후순위 요구. DN 인계 반영은 해당 프로젝트에서 확인 | 기존 webpj 범위를 유지 |
| **cpppj OS/플랫폼** (`61067075`) | Windows10/11 x86-64(AMD64) 대상·Windows11 ARM 고려. 현재 EXE AMD64 확인, ARM 빌드/실기 미검증 | 해당 프로젝트 기존 지침 유지 | 웹 대상 유지 |
| **8차 시나리오 편집 UI**·기본english 자동 생성/부재 종료 개선 (`98b2f20c`) | 후순위 미착수.728에서1.1/3절에 반영. 기존 보류는8차에서 해제 | AGENTS.md가4~8차를 동일 후순위로 지정. DN 인계 반영은 해당 프로젝트에서 확인 | Edit 제외 범위를 유지 |
| **4차 로컬 MCP** — dotnetpj 분석용으로 게임 내 모든 조작·화면 캡처·소리 포함 녹화 (`990a3541`, `5867f35e`) | 후순위 미착수(1.1절, 3절 8번) | R8 후순위 미착수. "로컬"의 범위와 dotnetpj 클론에 붙이는지는 [0.8 6번](LEFT_JOBS.dotnetpj.md#08-사용자-확인이-필요한-것-확인-전까지-에이전트-기본값으로-진행) | AGENTS.md가 지정하지 않음 |
| **4차 와이드 해상도** — 16:9, 최대 1920×1080. 이 단계는 해상도 지원만이며 그래픽 크기 스케일 조정은 불필요 (`5867f35e`) | 후순위 미착수. 기존 화면비 처리와 별도 완료 조건 | R8 후순위 미착수 | 지정 없음 |
| **7차 AA·FSR 1.0** — Options에서 선택(기본 off). FSR 지원 환경은 2560×1440·3840×2160을 목록에 추가하고 선택 시 FSR 자동 켜기, 실질 1920×1080에서 업스케일 (`7dd1ab4e`, `5867f35e`) | 후순위 미착수. FSR 3.1/FidelityFX-SDK 안은 쓰지 않는다 | R8 후순위 미착수 | 지정 없음 |
| **제품 버전 11.01 · Credits 세 번째 항목** "Netstorm 11.01 Patch Credits"(창 제목 "NetStorm 11.01 Patch Credits", 내용은 HJOW 개발·AI 사용) (`462c9a64`) | **적용 완료(685)** — [문서](docs/exe/cpp-product-1101-20261101.md) | 미착수. 기존 경로와 새 경로 양쪽 적용 필요(0.9) | LEFT_JOBS.webpj.md에 반영 기록 없음. webpj 작업 때 확인 |
| **외부 도메인/IP 접속 기능 문서화, 클라이언트 유효성 검사 비활성화** (로컬 멀티플레이 서버 제외) (`0190ba95`) | **적용 완료(684)** — [문서](docs/exe/cpp-external-services-20261101.md)·[목록](cpppj/external-endpoints-20261101.json) | 지침을 0.7·0.9에 반영(2026-11-02). C# 소스(.cs) 정적 검색에서 소켓·HTTP 접속은 없고 외부 실행은 기존 경로 도움말 링크 1곳뿐 | 반영 기록 없음. 외부 링크가 있으면 확인 |
| **실행 파일 `Netstorm.exe`** (`ef0c6b91`) | 완료(580·581) | 기존 경로 완료(0.96). 새 호스트로 기본 실행을 넘길 때의 이름 소유는 0.8 5번 | 해당 없음(웹) |
| **4~6차 재편** — 5차 저장 위치·커스텀 맵·Edit 목록(스크롤·무제한), 6차 한국어 (`bfccadf5`, `24959e75`) | 후순위 미착수 | R8 후순위 미착수 | 한국어는 webpj가 별도로 진행(LEFT_JOBS.webpj.md) |

AGENTS.md의 나머지 현행 규칙(커밋 메시지 형식, push 금지, 한국어 주석·UTF-8, 실행 확인·예외 PC, 배포 스크립트, 보호 대상)은 0.2절과 4절에 반영돼 있다.

## 2. 현재 상태 (2026-11-03, 단위777 기준)

- **게임 실행·플레이 가능 여부:** 인자 없는 `Netstorm.exe`로 GUI가 시작되며 마지막 전체34 진입·비정지 진행·정상 종료는645의644제품에서 확인했다.672에서는 메뉴/음향/UI 관련 경로를 별도 실행했다. 자연 완주 근거는 과거 제품의 Early1/2 자동 완주, 사용자가 다른 PC에서 직접 완주한 캠페인 1-1, 2026-11-01 사용자가 cpppj를 직접 실행해 완주한 캠페인 1-2(적 사제 포획·희생, Next Missions로 1-3 진입 확인. PC·제품·SHA는 미제공)다. 이 1-2 플레이에서 원본과 다른 점 2건이 제보됐다(2.4절).
- **기존 게임과의 동일성:** 원문 관측과 대조를 마친 부품이 많다. 자연 완주와 전체 동일성은 사용자가 플레이하며 확인한다.

단위/집중 검사, 짧은 진입, 자연 완주, 전체 동일성은 각각 별도로 판정한다. 서로 다른 제품의 성공을 합산하지 않는다.

### 2.1 영역별 마지막 근거

| 영역 | 마지막 근거 | 확인한 범위 | 남은 한계 |
| --- | --- | --- | --- |
| TYPE 실제 파일·키 읽기 | [777](docs/exe/cpp-type-live-read-20261103.md) | TYPE를 같은 고리·현재 FILE/키로 읽고 기존 Decode/Parse·CLI/CD/snapshot은 보존했다. 원문47입력/94회 재관측이 일치했고 집중13개(새5개) 실패0이다. | 새게임/MP0·기존2712불변/현재2714. 실제 Client는 소스/빌드 근거·정확한 scanner EOF/32비트 ABI/전체 GUI는 별도 |
| 네 Renderer 진단·BFS 배열 수명 | [776](docs/exe/cpp-renderer-diagnostics-20261103.md) | Hash/Fire/Spot/Board 진단과 현재 플래그·같은 BFS 작업 배열의 장기 소유를 연결했다. 집중21개(새4개) 실패0이며 표시252입력/504회·일반키576입력/1152회의 재관측이 최초 fixture와 바이트 일치했다. | 새게임/MP0·기존2710불변/현재2712. 기본UI 미추가·실제 OS/전체 프레임·Lock 실패/32비트 ABI는 별도 |
| 조건부 AI 메뉴·공유진단·Toggle | [775](docs/exe/cpp-ai-debug-menu-20261103.md) | 원문메뉴60/120·입력200/400·미등록이름21·실제Toggle추가63과각반복604관측/4fixture가일치했다. 현재brain/공유진단과이전MakeAi를확인했다. 집중12개 실패0 | 새게임/MP0·기존보호2706불변/현재2710. 기본UI미추가·known주소만·checking98/776표시별도·메뉴전체픽셀/SEHABI별도 |
| 글꼴 ACP·UTF8 설정·디스크 | [774](docs/exe/cpp-font-face-encoding-20261103.md) | Windows NLS24입력/24관측·root추가24(거부9)가일치했다. raw31BYTE/설정왕복·TAFF/긴949경로·기존글꼴/읽기정책을확인했다. 집중26개 실패0 | 새게임/MP0·기존보호2705불변/현재2706. raw31/공유256BYTE/TAFF보존·NLS참고24·실제설치글꼴/다른ACP/구형무BOM이관별도 |
| MakeAi 일반 설정·owner0 수명 | [773](docs/exe/cpp-make-ai-parent-20261103.md) | 최종원문부모770·객체2592·입력64의6852관측과이름12 및독립반복6864관측/4fixture가일치했다. 실제battle0/기존brain과이전AI직접writer를확인했다. 집중12개 실패0 | 새게임/MP0·기존보호2701불변/현재2705. 실제battle0/기존brain만·owner0양성이전/삭제·전략/미정stack/ABI별도 |
| 일반 간헐천·AI 직접 명령 | [772](docs/exe/cpp-ai-direct-commands-20261103.md) | 원문writer1940/3880·입력72/144·이름30과 독립 반복4054관측/3fixture가 일치했다. 실제AI 본체·지갑·반납/재생성과 기존간헐천 부모를 확인했다. 집중10개 실패0 | 새게임/MP0·기존보호2698불변/현재2701. 실제AI메모리/지갑 수명·전략Tick/MakeAi/조건메뉴/전체tail GUI는별도 |
| Config 실제 읽기·일반미션·Tell | [771](docs/exe/cpp-config-live-read-20261103.md) | 원문68입력/136관측과 준비·root 재관측136회가 일치했다. 현재TAFF/동적미션·언어·명시Editor source 분리와 기존설정/경로/시작파일 회귀를 확인했다. 집중38개 실패0 | 새게임/MP0·기존보호2697불변/현재2698. 기본Reader/CD/명시Editor 분리·미정padding/부분I/O·ConfigAppend Native본문/SEH ABI 별도 |
| CreateAt 실제 생성·후처리·입력 | [770](docs/exe/cpp-create-at-parent-20261103.md) | 원문 부모580/1160·입력408/816·이름15와 독립 반복1991관측이 일치했다. 비전투TYPE76/받침의 실제 생성·owner·HP·장부·배치와 기존DestroyAt 회귀를 확인했다. 집중14개 실패0 | 새게임/MP0·기존보호2694불변/현재2697. 비전투TYPE76 실제 접합·TYPE 이웃 포인터/전체 입력 GUI·가상객체 ABI 별도 |
| DataManager 실제 읽기·Template 할당 수명 | [769](docs/exe/cpp-datamanager-live-read-20261103.md) | 원문60/120과 재관측, 별도본문4/8·재관측8이 일치했다. 미발견/빈 파일·현재TAFF/부분암호·서명/부분파서·기존Template 소유 수명을 확인했다. 집중15개 실패0 | 새게임/MP0·기존보호2692불변/현재2694. generic 빈 이름표 Native와 실제Fort35개 구별·Editor/snapshot 보존·FILE/SEH ABI 별도 |
| DrawWater 일반 명령·후단 표시 | [768](docs/exe/cpp-draw-water-frame-20261103.md) | 원문 표시76/152·입력384/768·이름9·프레임5/10과 추가 관측이 일치했다. 현재 finder·픽셀/clip·색 재조회·장기 flag와 dirty0 전체 출력을 확인했다. 집중5개 실패0 | 새게임/MP0·기존보호2689불변/현재2692. 메모리 화면 경계·실제 GUI/전체 프레임/DirectDraw ABI·다른 진단은 별도 |
| QuickHelp 최초 실시간 파일·캐시 | [767](docs/exe/cpp-quick-help-live-read-20261103.md) | 원문62입력/124관측과 준비재관측124·추가124회 일치. 현재TAFF/Unicode·부분암호실패반납·필수missing·LF/NUL/단일성공cache와snapshot분리를확인했다. 집중12개 실패0 | 새게임/MP0·기존보호2688불변/현재2689. 부분I/O미정tail/32비트FILE·외부창SEH는host경계·기존snapshot계약보존 |
| Help목록·HTML stream·지역파일 수명 | [766](docs/exe/cpp-quick-help-list-stream-20261103.md) | 원문299입력/580관측과 추가580회·별도LF경계7관측 일치. 현재TYPE재조회·실제HTML/CRLF·r+b tail·공유고리·stream반납을확인했다. 집중8개 실패0 | 새게임/MP0·기존보호2684불변/현재2688. 원문실패는SEH전prefix·실제브라우저/전체Help외관·32비트FILE ABI는별도 |
| 존재조회·세 실제 파일 소비자 | [765](docs/exe/cpp-file-probe-consumers-20261103.md) | 원문640입력/1280관측과 추가1280회 일치. 출처5/2/1/3/4·Unicode/metadata·가상CWD/검색/현재root·실제세소비자의실패수명을확인했다. 집중37개 실패0 | 새게임/MP0·기존보호2683불변/현재2684. payload/decoder계약·snapshot/OS직접reader보존·전체FILE/SEH ABI는별도 |
| 글꼴·음질 명령·표시 캐시 수명 | [764](docs/exe/cpp-presentation-font-sound-20261103.md) | 원문 글꼴410/820·음질/옵션224/448·이름21과 추가820/448/21회 일치. 같은 주소를 재사용한 실제 글리프 픽셀과 음향 실패/음소거·동기 설정을 확인했다. 집중27개 실패0 | 새게임/MP0·기존보호2677불변/현재2683. OS/GDI 글꼴·ACP↔UTF8·실제 장치/PCM·전체 픽셀은 미검증 |
| 명시41baa0·객체덤프 수명 | [763](docs/exe/cpp-basefile-whole-read-20261103.md) | 원문133입력/266관측과 추가266회 일치. 생성완료 전후 반납·최소4/추가폭·현재키/부분읽기와 실제Unicode TAFF/덤프 회귀를 확인했다. 집중19개 실패0 | 새게임/MP0·기존보호2676불변/현재2677. 원문SEH/24BYTE ABI·미정할당밖쓰기는 host경계·다른live reader는 후속 |
| 설정명령·raw priority 메인루프 | [762](docs/exe/cpp-configuration-command-input-20261103.md) | 원문 명령238/714·이름30·메인루프96/288과 별도 재관측이 일치했다. 현재 Config/State 저장·목록kind/팔레트·범위밖 raw priority를 확인했다. 집중6개 실패0 | 새게임/MP0·기존보호2673불변/현재2676. MP chalPal/항해 신규검사0·실제OSpriority/GUI/PCM 미검증·새환경캡처2회실패 |
| outside·공통정지/현재모달 | [761](docs/exe/cpp-outside-input-routing-20261103.md) | 원문684행/1356회와 별도 재관측이 바이트 일치했다. C32/Tab16/숫자32행·실제outside 키큐/커서·정지8조합·현재폴링 분리를 확인했다. 집중15개 실패0 | 새게임/MP0·기존보호2672불변/현재2673. 실제Uber 모달은 컴파일/정적접합·OS/물리/전체GUI와32비트ABI는 별도 |
| 일반명령4종·항해 정적 분기 | [760](docs/exe/cpp-utility-command-input-20261103.md) | 최종 비MP 원문283입력/849관측·이름39입력은 최초fixture의 해당부분과 바이트 일치했다. 현재Config/저장실패순서·전체DWORD SID와 첫메뉴 예약SID1의 카메라를 확인했다. 집중5개 실패0 | 새게임0·MP분류전기록/이후제외·기존보호2670불변/현재2672. 초기MP명령관측/15실패 보존·후속MP검사제외·World없는Carrier preview/GUI는 별도 |
| kind17 공통 생성자·NULL/실패 수명 | [759](docs/exe/cpp-warning-message-constructor-20261103.md) | 원문50입력/150관측과 추가150회 일치. 실제 공장의 현재 입력·NULL/빈/NUL·6개 생성 실패/부분Style 반납·기존11/16과 억제를 확인했다. 집중27개 실패0 | 새게임/MP0·기존보호2669불변/현재2670. 등록 해제는 복구 가능한 호스트 예외 보완·원문SEH/다른kind/GUI/PCM은 별도 |
| World/Storm 선택 지식 메뉴 | [758](docs/exe/cpp-knowledge-world-menu-20261103.md) | 원문 메뉴50·이름/큐21입력과 각각 별도 재관측이 일치했다. 실제 HTML 이름조회/명령허용표·FIFO와 기존 Storm25장면도 확인했다. 집중4개 실패0 | 새게임/MP0·기존보호2667불변/현재2669. 기본UI caller/전체픽셀·x87다른정밀도·실제GUI/물리/PCM은 별도 |
| BaseFile 전체 열기/현재FILE | [757](docs/exe/cpp-basefile-open-policy-20261103.md) | 원문1364입력/2728관측과 추가2728회 일치. 실제Unicode metadata/current payload·mode2폴백·키/예외반납과 덤프 재읽기/쓰기 부모를 확인했다. 집중44개 실패0 | 새게임/MP0·기존보호2666불변/현재2667. 32비트FILE/FileSpec/예외SEH ABI·부분I/O·다른실시간reader와MP는별도 |
| D/d 최근 생산 취득 | [756](docs/exe/cpp-repeat-item-key-20261103.md) | 원문452입력/904관측과 별도904회 일치. 실제FIFO·현재lastType/content/serial·기존슬롯키/Inventory 소비를 확인했다. 집중65개 실패0 | 새게임/MP0·기존보호2665불변/현재2666. 실제UserInput outside 공통거부는761후속·OS/GUI/물리/PCM은 별도 |
| 객체덤프 공유 파일명 고리 | [755](docs/exe/cpp-object-dump-file-ring-20261103.md) | 원문30입력/60관측·실제Unicode Client의 정상5칸/첫열기실패1칸/읽기예외3칸/직접쓰기실패5칸을 확인했다. 집중11개 실패0 | 새게임/MP0·기존보호2665불변/현재2665. 상대쓰기실패의TAFF/보조전체폴백·전체FILE/SEH ABI는757후속 |
| T/t 시간·현재 메시지 억제 | [754](docs/exe/cpp-game-time-display-20261103.md) | 원문 키168/메시지90입력·774회와 추가774회 일치. 0.1초 기한/공통Style·Idle·현재missionPost getter를 확인했다. 집중57개 실패0 | 새게임/MP0·기존보호2663불변/현재2665. 실제Uber/OS GUI·물리/PCM·전체kind/FILE ABI는 별도 |
| Pause 키 일반명령 예약 | [753](docs/exe/cpp-pause-key-input-20261103.md) | 정지 앞 현재542a30·0/0 FIFO예약/수정키/뗌·실제큐포화/기존Pause 소비. 원문246입력·재관측일치·집중37개 실패0 | 새게임/MP0·기존보호2662불변/현재2663. 실제Uber/OS 키·물리/PCM·전체UI는 별도 |
| MatchDisk/FileSpec·가상CWD | [752](docs/exe/cpp-file-spec-disk-20261103.md) | 비패턴 접두·UNC/루트/첫NUL·원래 패턴의 backslash 접두/검색핸들/OS순서·가상CWD와 실제Edit/목록 연결. 집중100개 실패0·기존164원문 재관측 일치 | 새게임/MP0·보호2662불변. 전체FILE/264BYTE ABI·실제UNC서버/다른drive상대CWD·부분I/O/GUI/물리는 별도 |
| U/u 최근 손실 위치 | [751](docs/exe/cpp-lost-position-key-20261102.md) | 현재 공유 좌표/세계 크기·정지/모달·strict 유효점과 실제 Session 삭제 writer→카메라. 원문1022입력·재관측일치·집중21개 실패0 | 창 없는 검사이며 실제 Uber/OS 키·PCM·CD 전체는 별도. 초기 검사장면 Gun 전제 실패3회 보존·새 게임/MP0·보호2661불변/현재2662 |
| 객체메뉴반환/음향·직접지점진단 | [750](docs/exe/cpp-object-menu-open-parent-20261102.md) | 제품객체메뉴의실제가상ac반환0/1과부착/성공음향부모·Ctrl special 직접Main로그/현재깊이/들여쓰기/폴링. 집중71개실패0·원문8+36행 | 가상ac/70·장치/투영/finder는원문관측의명시경계. 새게임/CLI/MP검사0·전체타입/OS GUI/PCM아님. 기존2659불변/현재2661 |
| 편집취득/우클릭취소·폴링 | [749](docs/exe/cpp-pointer-modes-20261102.md) | 모드1의실제워크샾픽셀Pick/Unpop/Form/Take/Cancel·빈후보강조0·우클릭1~5/정지/disabled·폴링별도유지. 전기본부분1개23단언실패→후집중67개실패0·원문96행 | 모드6ChalSailPos정적연결만/새MP검사0. 새게임/CLI0·전체타입/실제OS GUI/PCM은아님. 기존2658불변/현재2659 |
| Edit좌클릭/Ctrl다리순환 | [748](docs/exe/cpp-editor-primary-selection-20261102.md) | 실제Client/Input/World/Session의owner추적·Ctrl가상74·422640/같은난수/프레임. 전plain1개2단언실패→후집중64개실패0·원문256+120행/fixture2 | 맵밖후보0/disabled보존의입력연결검사·새게임/GUI/CLI0. 활성Select/픽셀Pick·모드1/6·우클릭관문·special영역/missionPost선행은남음. 기존2656불변/현재2658 |
| Edit소유자/실제월드갱신 | [747](docs/exe/cpp-editor-owner-redraw-20261102.md) | Session의같은Pool/Core/0059a8b0·우클릭/SetOwner접합. 전실제World1개/4단언실패→후집중55개실패0·원문256재관측일치. 미니맵005949d4와감김/중립/Editor0분리 | 새게임/CLI/fixture0·일반SetOwner연결은컴파일이며이번새GUI아님. 좌클릭Ctrl/품질순환·전체상태/물리/PCM은남음. 보호2656불변. |
| 화면 변경 부모/실패 복구·DD조정 | [746](docs/exe/cpp-screen-mode-transition-20261102.md) | 원문192입력/384실행·실제 초기화DWORD/공통메타 갱신·집중33개 실패0. 최종제품 거부요청17 복구/동일235·조정취소/확인/15초만료·창138/같은사본 저장재시작 통과 | 전체4게임 중 중간제품 파서실패·최종제품Tips 입력실패를 보존. 앱 내 메뉴 입력이며 물리/PCM/실제장치할당실패/전체UI·CD 전체 검증은 별도다. 보호2655불변·새fixture1. |
| 객체 부착 SP 비용 표시/월드 투영 | [745](docs/exe/cpp-floating-sp-reference-20261102.md) | 실제 Session Form/등록표·Reference 뒤2000비트와 GumpTree/Collection 공통 현재 객체/카메라 투영 연결. 전2개 미연결 예외→후52개 실패0·실제 수집/보호 글꼴 픽셀 출력·Kill/부모 삭제/지연 Cleanup·Form/등록 반납 | 새 게임/CLI/Native 관측/fixture/실행 대상0·보호2655불변. 전체UI/동일성·물리 키/PCM/자연 완주·10.72는 별도다. |
| 전체화면 F12 마우스 전환 | [744](docs/exe/cpp-screen-mouse-key-20261102.md) | 원문 Guide/모달/정지 앞 현재flags&1/XOR40·즉시 장치 호출. 원문552입력/1,104실행·집중23개 실패0. 실제 조정 모달235→171→235·뗌/수정키·창 모드138 보존 | 최초 검사 F12 이름 미지원 종료1과 최종 종료0을 구별한다. 앱 내 메인 메뉴 입력이며 물리 키/전체Console/모달·픽셀/PCM/10.72는 별도. 기존보호2654불변·새fixture1. |
| 현재 미션 demo 선택 관문 | [743](docs/exe/cpp-mission-demo-selection-20261102.md) | 실제 Session factory가 현재 missionOutcome.demo를 조회하며 World의 임시 consoleFocus 제거. 원문10입력·수정 전2개20단언 실패→메뉴·Session28개/선택23개 실패0. Select/ESC 초기화 차단과 독립 Clear 구별 | 새 GUI/CLI/fixture/실행 대상0·보호2654불변. Console/모달 전체/물리 입력·원본 전체 동일성·자연 완주는 별도다. |
| ESC 선택 정리/커서 취소 | [742](docs/exe/cpp-escape-selection-20261102.md) | 원문ESC/Select0의154입력·두정밀도/각308실행 일치·집중23개 실패0. 실제 전741제품 선택15000 잔존→후742제품 일반/정지 선택0·메뉴 왕복·Q 조각 취소/다음ESC 메뉴. 정확한 사건 종류·뗌 판정 | 앱 내 입력이고 전체Console/모달·커서/메뉴 모든 상태·물리 키/전체 픽셀·PCM은 아니다. 보호2653불변/새fixture1. |
| F6 지식 키의 정지 경계 | [741](docs/exe/cpp-knowledge-key-20261102.md) | 실제UserInput의 공통 정지 뒤 F6/수정키/일반 큐 연결·새원문16/기존480행·수정 전4개41단언 실패→집중20개 실패0. 실제 캠페인1-1 한 프로세스의 정지/재개·수정키/뗌·일반/AltF6 카드29개·ESC 왕복 | 앱 내 입력이며 물리 키/전체 픽셀·UI/PCM/자연 완주 검증은 아니다. 기존 일반 ShowTechnology/F9 경로는 유지했다. 보호2652불변/새fixture1. |
| 게임/테스트/전체 선택 빌드 | [739](docs/exe/cpp-build-modes-20261102.md) | 새 기본 테스트OFF·대화형/명령행 Game/Tests/All·집중 대상 지정·기존 CI/프리셋 ON·양쪽 README. 선택/인자15개, 실제 게임/집중 대상 증분 빌드, 통합 테스트 의존5대상과 전체71대상/테스트 실행66개 구성 확인 | 전체 재컴파일/통합 테스트 실행/성능 전후 비교는 수행하지 않았다. 실행 제품738 유지, 공용 엔진은 테스트 빌드에도 필요하다. |
| Edit 저장·삭제의 경로 고리 | [740](docs/exe/cpp-editor-mutation-ring-20261102.md) | 원본PE6함수 정적 대조·실제 Template writer/삭제 각1칸·chmod0칸·7개/18단언 실패 뒤 집중58개 실패0/기존1902행. Unicode/tail/생성/공유거부/readonly/누락/보호/장부/실제35섹션SaveFile/10칸뷰수명·CLI11.01 | 새Native/fixture/게임0. 호스트 준비/일반 잘라쓰기에 원문 예약을 합성하지 않았다. 전체 FILE ABI/부분I/O/다른 소비자/실패와 전체UI는 남는다. |
| Edit 원본 Graph·분석기 재실행 | [720](docs/exe/cpp-editor-graph-native-20261102.md) | 원본 생성/배치/저장/재적재 전체Pool120000·표면674개, 같은Fort719제품의Graph1/670·전체표면 타입/상태/소유자/좌표 일치. 같은복사본 재실행·녹화 명시 생략, 분석기 집중4개/MCP16도구 통과 | 배치 직후 장부678 대조·다중섬/변경별 개별Graph 소속과 frame468개 차이. 게임제품/전체34/ZIP은 그대로다. |
| 덱 재활성 현재 옵션 | [677](docs/exe/cpp-deck-reactivation-20261101.md) | 실제 반환의 설정 두 번 조회·할당 뒤 모드/Unit Rate. 집중8개 실패0. 같은1-1 UI 배치에서676 약10초→677 약1초 | 원문 Fast1초 복원이다. 최초 브리핑 입력 실패1회와 최종2회를 구별한다. 자연 완주/전체34/새 ZIP/물리 입력은 아니다. |
| 간헐천·고정 표 대상 변환 | [678](docs/exe/cpp-translated-shadow-20261101.md) | 004dab90/0040351d 원문296전체표면/888관측, 실제 간헐천8입력. 집중31개 실패0·실제1-2초기 GUI2회. 원문 정렬/색표 유지 | 과거 소스 치환 설명/픽셀 기대를 정정했다. 10.78 회전0·일반 범위다. 실제 새 다리와 원본 나란히 픽셀/전체34/자연완주는 아니다. |
| 지정 집중 변이 | [679](docs/exe/cpp-focused-mutation-20261101.md) | 기준선5/31개와실제결함5종 검출. 프로세스고장경계Python3개 통과. 사본대상소스원래바이트복구 | 최초x구분자 미검출은동치로분류하고 숫자0 오류로교정했다. 전체C++/게임/MP검사가아니며사본변이EXE는제품이아니다. |
| MoveDialog 현재 모달 | [680](docs/exe/cpp-move-dialog-20261101.md) | 원문73입력/219관측·집중9개 실패0. HTML명령→일반 입력→같은 모달의 공통 Push/Attach 연결. 임시 본문의 실제 창 상단 y=0 관측 | 최초 미등록 문서 접두어 입력 실패와 최종 GUI 통과를 구별한다. 원본 시나리오/전투/MP/물리 입력 검사가 아니며 CD 대응은 정적 후보뿐이다. |
| 전체34 진입 | [645](docs/exe/cpp-solo-integrated-20261031.md) (644 제품) | 새 ZIP의 일반31+실제Demo3 최종34/34, 종료0·비정지 시각 증가·창DC 전체RGB·Native Frame·수집/분배. 최소 증가20.843초. 주3566등록/집중37통과 | 짧은 진입이다. 최초 입력 누락34실패/재검증34통과/첫 실행 준비2회/총70프로세스를 구별한다. 자연 완주·전체 동일성·물리 입력은 아니다. |
| 반복 미션 수명 | [638](docs/exe/cpp-state-repeat-20261031.md) (637 제품) | TEST02 4:3 창, 캠페인 1-1 16:9 전체화면. Restart·Leave 취소·메인 복귀·Quit, 창 DC RGB 4쌍 | 앱 내 검사 입력이다. 장시간 누수, 모든 전환, 자연 완주의 증거가 아니다. |
| Guide 소멸 | [671](docs/exe/cpp-guide-release-20261101.md) | 명시 End/자동 Guide의 실제 Tutorial6DWORD·AI·기본 Config/예외 정리, 장기 deny 공유, 같은 World/UserInput 보존. 새10·기존25/실패0 | 원본32비트 SEH 전체 ABI·GUI·자연 완료 검증이 아니다. 기존 검사 장면의 최초3실패/교정과 최종35를 구별한다. |
| 위치 음향 현재 조회 | [672](docs/exe/cpp-sound-current-view-20261101.md) | 현재 snapshot/실제 Gump 지도·장치 뒤 pan→volume 재조회, 실제 Client 연결. 집중30/실패0와 GUI view10프로세스의 pan/Home·메뉴0·소리 off/on·세 해상도 재시작 | 실제 PCM/청취는 미검증이다. 곡 만료 현재 battle은673에서 별도 연결했다. 저장1224는674의 별도 실제 파일 검사로 수정했으며 전체 음향 묶음을 재실행한 것은 아니다. |
| 곡 만료의 현재 battle | [673](docs/exe/cpp-scene-current-battle-20261101.md) | 실제 Client의 현재 owner 연결·시각/NaN 관문·효과 뒤 재조회·cleanup순서 보존·Shutdown뒤 현재 조회. 집중24개 실패0/기존9339행 | 실제 Run 연결은 빌드/정적 확인이며 GUI cleanup 전체를 재실행하지 않았다. 새9개는 명시 입력이다. |
| 매핑 중인 설정 저장 | [674](docs/exe/cpp-options-mapped-save-20261101.md) | 실제1224·C/H·Unicode 재적재·이전view·지연 정리·공유거부32/changed/복구 파일·일반 절삭. 집중7개 실패0 | 실제 GUI/672 전체audio 재검사는 아니다. 원인 프로세스·Python 복구 writer·실제 부분I/O/1176/1177는 미검증이다. 외부 매핑이 종료까지 남으면 성공 백업도 남을 수 있다. |
| Shift 다중 선택/그룹 이동 | [701](docs/exe/cpp-multiple-selection-20261102.md) | 원문004da6b0/004d5500·고정30 보조 목록·현재 가상 통지/승격·enabled0/1/정리·실제 Shift/이동점 큐 접합. 원문601·정적85명령·집중15개 실패0 | 초기 잘못된 승격 표시 대상/SID0 예외를 고치고 기대값을 유지했다. 새 게임0이며 실제 화면 Shift·물리 입력/모든 커서·다음 유닛 선택004da880은 별도다. |
| N/Shift+N 다음 유닛 선택 | [702](docs/exe/cpp-next-unit-selection-20261102.md) | 실제 순환/free·전체owner/TYPE 검색과 선택/가운데/정지/현재값 후속 선택, 실제 입력 연결. 원문410·집중18개 실패0 | 선택/카메라 경계는 기존 실제 몸체에 접합했지만 새 GUI/물리 키 검사는0이다. 전체 동일성은 별도다. |
| P/R 실제 사제 선택/반복 View | [703](docs/exe/cpp-priest-selection-shortcut-20261102.md) | 화면 재클릭 대신 실제 참여 SID→세 인자 Select·반복 View 큐, 장기 누적 DWORD/일반 reset 보존. 원문 부분 분기220·집중21개 실패0 | 조회/선택은 명시 Native 경계이며 제품 실제 접합은 컴파일했다. 새 GUI/실제 포획·희생 검사는0이다. |
| C/c 회전 방향·같은 현재 옵션 | [717](docs/exe/cpp-rotation-key-20261102.md) | 원문264입력/792관측·Client rotmode/Config/우클릭 공유·신규3개 실패0·실제 다리0→2→0/정지·모달·메뉴 체크/복귀 | 최초 CLI 수정키 문자 실패와 최종24.6초 통과를 구별한다. 앱 내 입력이며 물리 키/전체 동일성은 별도다. |
| Tab 최근 로컬 배치5칸 | [718](docs/exe/cpp-construction-history-key-20261102.md) | 원문511/1533관측·Client 장부/기존 통지 writer·즉시 중심·신규4/집중10개 실패0·실제 덱 배치→미니맵→Tab·정지/모달·재시작 뒤 보존 | 최초 워크숍group10 입력 오인과 최종62.2초 통과를 구별한다. 별개10칸 저장 카메라는 남고 물리 키/전체 동일성은 별도다. |
| 숫자/Shift 저장 카메라10칸 | [719](docs/exe/cpp-saved-camera-key-20261102.md) | 원문1750/5250관측·언어 표/현재 Renderer 지도 중심·Client80BYTE·신규3개 실패0·실제 두 위치 저장/호출·정지/모달·재시작 보존 | Tab5칸과 별개다. 기본배율/1024×768의 앱 내 입력이며 물리 키/PCM·전체 동일성은 별도다. 최초 검사 헤더 누락을 기록했다. |
| 선택 키 부모/View 큐·직접 진입 | [704](docs/exe/cpp-selection-key-input-20261102.md) | H/F4/F5/ControlF5·정지·수정키/기존 N/P/R, Guide 원시 F4 합성 제거와 --mission의 메인 모달 정리. 원문480·집중16개 실패0 | 실제GUI3회 중 처음 두 실패와 최종 캠페인1-1 왕복 통과를 구별한다. 앱 내 입력이며 물리 키/전체 픽셀 동일성은 별도다. |
| 5차 전체 파일/플랫폼 요구 | [733](docs/exe/cpp-phase5-file-inventory-20261102.md) | aaafbbc4/61067075 반영·1709소스/50정적후보·12파일군/reader·해시 재검증 일치. 제품732 유지/현재AMD64 확인 | 제품 코드/빌드/게임0.5차 이관·실제 일반권한/읽기전용설치·ARM 지원 검증은 미구현이다. 보호파일 변경1은 사용자 AGENTS이며 작업 수정0. |
| Edit Template 파일고리 | [738](docs/exe/cpp-editor-file-ring-20261102.md) | 수정전공유번호3단언실패뒤실제Exists/readonly/read연결·Unicode/누락/진단/10칸수명/장부분리·실제Template Load두칸/제목·집중51개실패0/기존1902행·CLI종료0 | 새Native/fixture/GUI0. 저장/삭제의 공유 고리와 직접chmod 경계는740에서 이어 확인했다. 다른 소비자/전체ABI/실패는 별도. 보호2652불변. |
| 생성자선행TAFF/실제시작순서 | [737](docs/exe/cpp-startup-io-20261102.md) | 736제품에서생성자1단언실패검출뒤제거. 실제같은Client의부모선택/하위손상미접근·선택한손상파일의CD뒤오류·7조회종류/snapshot/const복사/직접조회·집중31개실패0/기존414행·CLI종료0 | 새Native/fixture/GUI0. 독립조회준비는cpppj호환이며실제Run은Native등록을사용한다. 전체FILE/리소스I/O·다른소비자/실패·10.72는별도. 보호2652불변. |
| 시작아카이브/검색수명 | [736](docs/exe/cpp-startup-archives-20261102.md) | 실제441965~441a44/Full/재대입/Next/별도dtor의140관측·54+16행/전체고리·집중25개실패0/414행. 실제Startup설정Load/두Set뒤핸들반납·등록예외/기존Unicode회귀·CLI종료0 | 새GUI0. OS/등록장치는Native명시경계이며전체부모ABI/전체264BYTE FileSpec/생성자선행I/O·다른소비자/실패는남는다. 보호2650불변. |
| setup탐색/실제파일루트 | [735](docs/exe/cpp-startup-location-20261102.md) | 실제441790~4418bd/Probe의168관측·84행/전체10칸·집중17개실패0/332행. 실제Unicode6후보/가상CWD/기본·CD목록/기존reader·하위폴더메인1회종료0 | 최초통합검사의입력/기대표기2단언실패보존. Full/FindNext/TAFF wildcard고리·최초I/O/파일실패·10.72/분석CLI시작은별도. 보호2649불변. |
| 디렉터리/무인자 모듈 경로 고리 | [734](docs/exe/cpp-file-directory-20261102.md) | 실제459990/435cd0의396관측·192+6행/무인자3칸·전체버퍼/10칸·집중18개실패0/777행. 실제Unicode 패키지와main→Client/VFS/설정의공유소유자 인계·CLI종료0 | 새GUI0. 드라이브루트/긴Unicode 경로선택의명시호환차이·원문NULL읽기30행 구별. 전체Startup선행6후보/CWD/TARC·Full/FindNext/전체파일수명은남는다. 보호2647불변. |
| Startup위치확정뒤파일명작성 | [732](docs/exe/cpp-startup-file-names-20261102.md) | 원문441a44~441c5e/실제Load48관측·24행/전체10칸·작성6/7+이름5/6=11/13칸·집중25개실패0. 실제Unicode Startup/저장/재적재·CLI종료0 | 새게임0·연결전3개/262단언실패보존. 선행탐색/CWD/TARC/전체Startup ABI·Template/파일실패는별도다. 보호2646불변. |
| 설정Load의이름/검색수명 | [731](docs/exe/cpp-config-path-load-20261102.md) | 실제440e00/FileSpec/이름getter24관측·12행/section·Find→Read→Close·같은Client/VFS/CLI·집중22개실패0. 실제Unicode읽기9칸/검색·CLI1024/종료0 | 새게임0·첫이름충돌빌드실패정정보존. Startup별도경로작성·VFS FileSpec/FindNext·Template·전체파일실패는별도다. 보호2645불변. |
| 옵션 저장 이름/파일 열기·UI 경로 | [730](docs/exe/cpp-options-name-ring-20261102.md)·[729](docs/exe/cpp-file-path-ui-20261102.md) | 전체459b20/선택459550의64행·실제옵션부모4root/136관측·일반저장4칸/같은Client 읽기/CD·집중49개실패0.729의지역복사/Capture/CD/Help 유지 | 새게임0·연결전2개/20단언실패보존. Config Startup/Load·Template·일반쓰기 실패/FILE ABI는 남는다. 보호2644불변. |
| VFS/저장 상태의 경로 고리 | [728](docs/exe/cpp-file-path-clients-20261102.md) | 실제VFS/Client/State 공유·459900 장치 getter32행 전체버퍼/71관측·집중22개실패0. 같은제품한글GUI의Edit/메인1회종료0 | 연결전20단언실패와원문최초소모량명명정정 보존. AGENTS외부변경98b2를별도기록. 다른경로소비자/전체FileSpec·FILE ABI/전체UI는별도다. |
| 전체 경로 작성기/Client 수명 | [727](docs/exe/cpp-file-name-formatter-20261102.md) | 전체459c60/실제strchr352입력/704관측·재관측, 현재256BYTE/throw·10칸/별칭/Client·집중8개실패0. HUD probe의수동연결/후행slash거부제거 | 새게임0. CD는정적대조만이며배열밖미정쓰기는host거부다. 다른파일명소비자/전체예외ABI·전체GUI는별도다. |
| Edit 소유자/저장 재적재 | [726](docs/exe/cpp-editor-owner-save-20261102.md) | 실제 같은 워크샾1→2→0→8/취소·현재Core·HTML명령·Yes저장/메인/재적재의owner8 보존. 최종675프레임 종료0 | 제품725유지·빌드0. 전체5프로세스 중 최초 이름 입력/혼합 대상 검증 실패를 보존했다. 창1024×768의 한 객체이며 전체파일/타입·물리 입력은 별도다. |
| 실제 CHEAT 편집 전용 노출 | [725](docs/exe/cpp-editor-cheat-menu-20261102.md) | 제품Session의공통special getter/파생DigUp include에Edit 제한. 실제8조합·원문2458행·기존Detonate를포함집중7개실패0 | 새게임0. 수정전7단언실패보존. 원문작성기자체/다른special 의미는변경하지않았다. 나머지객체상태/전체UI는별도다. |
| 다중 섬/다리 편집 Graph | [724](docs/exe/cpp-editor-graph-world-20261102.md) | 두 섬/다리 연결·owner 변경·절단의즉시성분2→1→2, 집중1개 실패0. 원본5시점×두정밀도10호출의전체풀/표/스택·해시/점유 바이트일치 | 새게임0·제품722유지. 현재생성월드를원본Graph입력으로사용했으며 전체지형생성/GUI/전체동일성 증명은아니다. 최초private접근컴파일실패를보존했다. |
| 같은 후보 배치 직후 Graph | [723](docs/exe/cpp-editor-graph-placement-20261102.md) | 원본 후보8을 실제 새 Fort/편집 Session에 배치. 전체674개 표면·성분670/장부678·noGraph4개 위치 일치. C++1개/도구6개 실패0 | 새 게임0·제품722 유지. 차이8은 원본에도 존재한다. 다중 섬/편집 후 분할·병합은 별도다. |
| 초기 지면99cache Client 수명 | [722](docs/exe/cpp-terrain-cache-client-20261102.md) | Session 값형을 장기 Client 참조로 바꿈. 원문1560행·새Client/대여·두 실제 Session의 캐시/내부 그림·CRT 보존, 집중10개 최종 실패0 | 새 게임0. 최초 검사 Subscriber owner 누락 실패를 보존. 원본 실행별 seed/cache·전체 그림 동일성을 확인한 것은 아니다. |
| 개별 Graph 소속/변형 코드 | [721](docs/exe/cpp-editor-graph-membership-20261102.md) | 실제674개 표면·성분670/장부670·noGraph4개 소속 일치. 방향/flags674개 일치·도구5개 실패0 | 실제 Edit 재적재1회/종료0. 난수/cache·그림 픽셀·배치 직후/다중 섬은 별도다. 원본 전역99cache의 Session 수명 차이는722에서 수정했다. |
| 언어 변경 요청/기존 Chat 알림 | [712](docs/exe/cpp-language-request-20261102.md) | SetLanguage/LanguageChanged·동기 Config→실제 큐→현재 카운터·중복 없는 Chat Push/Attach. 원문337/집중11개 실패0 | 새 게임0. 전체 UI 재생성/언어 UI·폰트·MP 카운터 소비자는 별도다. |
| 언어 옵션/용어표·최초 다중 Xlat | [710](docs/exe/cpp-language-options-20261102.md), [711](docs/exe/cpp-xlat-runtime-20261102.md) | 정확 이름/영어0·1/두 LoadLanguage와 최초 세 Xlat/언어별 칸·파일 바이트 수명. 실제97526BYTE/830키 대조·집중8개 실패0 | 새 게임0. 언어 요청/Chat 알림은712에서 연결했다. 전체 번역 UI/폰트·6차 한국어·파일 오류 ABI는 별도다. |
| HTML Zsend/현재 송신 장치 | [709](docs/exe/cpp-html-zsend-20261102.md) | 원문87h/버전1/274BYTE·세80BYTE필드/채널11, 같은 연결표·실제 writer/수명 정적 연결 | 새검사/GUI/통신0. 원문 미정 스택 prefix/꼬리 동일성·root서버 접속/수신·MP 동작은 검증 완료가 아니다. |
| HTML 입력 대화창 | [708](docs/exe/cpp-html-input-dialog-20261102.md) | 실제 $Input→공통 TextEntry/두 버튼/모달·후속 Tell. 원문109·새3/기존6 집중9개 최종 실패0 | 새 게임0. 기존 메타 인자 캡처 문법을 바꾸지 않았으며 Zsend는709에서정적연결했으며TARC전체실제표시·후속MPTell은미검증이다. |
| HTML 전체/원소 기술 목록·공유 필터 | [705](docs/exe/cpp-technology-list-menu-20261102.md), [706](docs/exe/cpp-technology-all-menu-20261102.md), [707](docs/exe/cpp-ritual-choice-20261102.md) | 실제 fixed40/HTML/제단Form·원소/본섬·보상 행. 707은 선택 이름/6b 요청·수신/비미션 Tell을 정적 연결 | 707 신규 검사/GUI0. 전체 보유 랭크 효과·조기 희생 안내/후반 MP UI는 남고 비미션 선택 동작은 검증 완료가 아니다. Input은708에서연결했고Zsend는709에서정적연결했다.새MP검사는없다. |
| 실제 메인 부모/45초 autoDemo | [700](docs/exe/cpp-main-menu-parent-20261102.md) | 타입62·고정9슬롯·모달/idle·HTML 보존·일반 선택 Kill·현재 옵션/HTML/루트 팝업/포인터·45초 큐/Kill. 원문1944·집중16 실패0·메뉴 왕복/등록187·자동 데모 초기 안내 진입 | 총3게임 exit0 중 최종 제품2회다. 최초 Help serial 누락 수정과 auto 도구 phase4 기대 오류의 같은 원시 재분석을 구별한다. 물리 입력/PCM·안내 OK 뒤 진행·CD/전체 예외/할당 실패는 아니다. |
| Intro 감지/버튼/일반 입력 | [699](docs/exe/cpp-intro-menu-20261102.md) | 실제 존재 출처0..5·시작 캐시·조건부9버튼·PlayIntro 이름/큐/next39/35·기존 Native39/40/41. 원문84·집중11개 실패0·실제8/9버튼·영상2왕복/정리 | 총3게임 exit0, 최초 live 도구의 retired 필드 오독을 고쳐 최종 통과했다. 메인 부모/autoDemo는700에서 연결했다. 전체 영상/PCM·물리 입력·CD는 별도다. |
| 메인 버튼 해상도별 배치 | [698](docs/exe/cpp-main-menu-layout-20261102.md)·[699](docs/exe/cpp-intro-menu-20261102.md) | 원본 글꼴 폭·실제 FitToText/공통 폭·SCREENH/3+55/+78·입력 경계. 698의6해상도/48버튼,699의9버튼과 실제 메인8/9버튼을 확인했다 | 최초698 순환 종료와 교정을 보존한다. 실제 부모/autoDemo는700 완료이며 CD 배치와 전체 픽셀은 별도다. |
| 메인 Help·Options 팝업 | [697](docs/exe/cpp-main-menu-popup-20261102.md)·[700](docs/exe/cpp-main-menu-parent-20261102.md) | 실제 Help3행·공유 Options·현재 버튼/깊이/모달·명령 큐. 700은 실제 부모의005cad28 serial 허용과 General Help/Version/Options 왕복·같은 부모/등록187 복귀를 확인했다 | 최초700의 실제 Help 연결 누락을 고치고 최종 통과했다. 물리 입력/PCM/DD 장치·전체34/전체C++와 전체 예외/할당 실패는 아니다. |
| RecheckCD 현재 serial | [696](docs/exe/cpp-recheck-cd-20261102.md) | 이름/메타/큐 0x020000fd·Client 장기 serial·조회/공통 Kill/0쓰기. 원문332/996·집중4개·변이7/7·실제 메인 메뉴(serial 0 무효과, 모달 serial Kill) | 10.78의 005c85ec는 직접 쓰는 곳이 없어 실제 게임에서는 무효과다. Kill 경로는 검사 전용 입력으로 확인했다. 10.72의 시간 기반 00433bf0은 미대조. 최초 두 실행의 검사 기대 오류와 최종 통과를 구별한다. |
| 연속 화면 저장/BMP | [695](docs/exe/cpp-capture-client-20261102.md)·[694](docs/exe/cpp-indexed-bitmap-20261101.md)·[693](docs/exe/cpp-capture-sequence-20261101.md) | 실제Client owner/Frame·Capture 입력·8비트BMP·공유고리/완료창. 새원문192/576·집중14개·최종GUI20프레임/RGB/반납/메인복귀 | 최초Unicode본문실패와최종통과를구별한다. 메뉴한주기이며수집중모드변경/1000프레임GUI/반복주기·물리키·소리녹화는미검증이다. |
| URL 버튼/기존 Help 접합 | [692](docs/exe/cpp-url-input-20261101.md) | 이름/일반 입력 원문96행·실제 고리/큐·공통 Help 창/cmd/파일 링크 전달. 최종 선택5개 실패0 | 실제 F7→View/외부 브라우저·Unicode 이 버튼 왕복·새CD·전체메타 검증은 아니다. 외부 프로그램 실행0이다. |
| 실제 Pop 겹침/조기 반환 | [691](docs/exe/cpp-pop89-context-20261101.md) | 실제89의전체raw/spot 원문3관측 일치·상위 예외 변환 수정·범위/nonvoid/contained 앞단. 기존4+실제Session2개 통과 | 특정겹침1입력이며 원문void와C++진단enum2를 구별한다. 전체공간/자연생성·GUI·새CD게임 검증이 아니다. |
| 대본 DestroyAt 점 검색/삭제 | [690](docs/exe/cpp-destroy-at-20261101.md) | parser16/입력48/부모804·현재 finder/가상 삭제 연결. 최종 선택7개 실패0. 범위 값은 원문처럼 무시 | 최초 span 빌드/89 Pop 입력 실패를 구별한다. 89의 원문 겹침/상위 차이는691에서 처리했다. GUI/모든 타입·CD/손상 ABI/전체34·전체C++ 검증은 아니다. |
| 대본 AddRandom 실제 배치 | [689](docs/exe/cpp-add-random-20261101.md) | 일반입력288/실제부모720·현재Session/Factory직결·최종선택13개실패0. 실제134생성/잘못된TYPE로그와Pool무변경 | 최초89생성보장기대실패/입력정정을구별한다. GUI/모든타입·CD부모실행·전체34/전체C++검증은아니다. |
| 대본 일반 연기/화면 번개/명시 무동작 | [688](docs/exe/cpp-scenario-effect-input-20261101.md) | 3이름·원문576/1728·현재표시연결·연기8/5 대냉각10/3·부모0독립소유. 최종18개실패0 | 생성부모는명시원문경계. 실제GUI/픽셀·CD·모든SID/SEH ABI·전체34/전체C++는아니다. |
| HTML 사용 명령/실제 이름 조회·DumpSquids | [715](docs/exe/cpp-dump-squids-20261102.md) | 영문549문서/Menu 둘째 인자·실제43이름/172표기·원문 무동작48/144관측·Menu/큐, 집중6개 최종 통과 | 새 게임0·동적 Config/가림/전체 GUI·모든 명령 몸체는 별도다. MP 관련14개를 새 이름 대조에서 제외했다. 최초 stale schema 실행기5단언 실패를 구별한다. |
| 대본 송금 이름/명시 태그 목록 | [687](docs/exe/cpp-scenario-money-names-20261101.md) | 원본 송금6이름·실제 parser/큐와기존8개 실패0. 보관499대본의명시22이름/원문표288항목 정적 조사 | 기존 송금 부모는 변경하지 않았다. 동적/TARC/Config 확장·메뉴둘째인자·실제 게임 검증은 아니다. |
| 메뉴 없는 시나리오 객체 명령 | [686](docs/exe/cpp-scenario-object-input-20261101.md) | 원문5이름·480입력, 키 뗌/SID0/반환 무시·현재 Session 직결. 집중14개 실패0·83VT 정적 점검 갱신 | 객체6c는 명시 경계. 새 실제 효과/게임·CD·전체34/전체C++ 검증이 아니다. |
| 제품11.01/추가 Credits | [685](docs/exe/cpp-product-1101-20261101.md) | CLI/Windows버전/실제GUI11.01, 메뉴정확히3행·HJOW/AI·같은창 Back/Cancel. 기존24개 실패0·최종두해상도 GUI | 내부원문판본/옛Credits유지. 전체34/전체C++·CD GUI·MP·물리입력/PCM은 아니다. |
| 외부 기능/구 검증 비활성화 | [684](docs/exe/cpp-external-services-20261101.md) | 설정/URL/원문 두 판본·R.exe·현재 소비자 목록, 제품 검증 getter/안내 제외·성공1 합성 없음. 집중12개 실패0·전후 메뉴GUI | 원문82관측은 참조모드. 실제 외부 통신/MP·CD실행은0. URL후보59/18은 중복·템플릿을 포함한다. |
| 대본 기술 지급·SP 이름 | [683](docs/exe/cpp-get-technology-input-20261101.md) | GetTechnology/ChangeMoney 이름2개·기술 지급 일반 입력→기존 Session 부모, 새 원문48/144·집중10개 실패0 | 새 게임 실행0. 기존 부모45사례이며 실제 지급/안내 GUI·CD·전체34/전체 C++는 아니다. |
| 대본 음향·보기 명령 이름 | [682](docs/exe/cpp-script-bindings-20261101.md) | PlaySound/ViewPriest/SetAllBridge/DigUp 이름4개, 같은 현재 고리/음향 연결. 원문48/144·집중12개 실패0·수정 전/후 GUI2회 | 이름표 끝49BYTE 증가까지 확인. PCM/청취·다른3명령 새GUI행동·전체34·CD본문은 아니다. |
| 객체 메뉴 연결 정적 대조 | [681](docs/exe/cpp-object-menu-wiring-20261101.md) | 실제 PE83VT/현재 생성표, 작성13·비멀티 열기5·특수 명령9 연결·메타맵2 이월 | 제품 변경/본문·게임 실행0. 소스 연결 대조이며 전체 상태·효과·CD·선택기/픽셀 검증이 아니다. |
| 카메라 writer·ViewSpot | [676](docs/exe/cpp-viewspot-20261101.md) | zoom/flags/시차/+60/목표 writer 전수 분류. 대본 ViewSpot 연결, 기계어 대조34행, 집중5개/실패0, 실제 RunForIt 입장 카메라 (880,980)→(1712,584) | 패킷/디버그 분류는 정적 판정이다. 원본 픽셀 나란히 비교·다른 ViewSpot 맵 진입·전체34는 아니다. CD 처리기 몸체 미대조 |
| 설정 복구 도구·매핑 원인 | [675](docs/exe/cpp-tool-settings-writer-20261101.md) | 954바이트 추적 파일에서 `git diff` 계열이 errno22/doserrno1224를 재현(C/H 대조군0). 도구의 1224 판별·마지막 시도 교체·지연 정리. 실제 `git diff` 대조 수정 전 600회 중 실패17 → 수정 후 1007회 실패0. 검사6개/변이8/8 | 과거 개별 실패의 프로세스는 사후 특정 불가. 954바이트 `git status`는0건이고 재현은32KB 초과 표본뿐이다. Defender 제외 경로 미확인. 1176/1177 미주입. 게임 실행0 |
| 옛 메뉴/음향 도구5개 | [672](docs/exe/cpp-sound-current-view-20261101.md) | ESC→실제Game/ABORT·해상도No/Yes/재시작·실제HTML 캡처/스택·32반복·번개4곡·합성COL. menu/dialog/thunder/weather 최종 통과, audio4그룹과 view 별도 통과 | 최초 실패9묶음·전체GUI64/CLI12/GUI종료1 두 개를 구별한다. 전체audio 실패·원본 동적픽셀/자연완주/물리 입력은 미검증이다. |
| 시작·휴대 배포 | [637](docs/exe/cpp-portable-startup-20261031.md) | 무인자 GUI 시작, 실제 Unicode EXE 경로 기준 `game/` 우선, cwd 독립, 자기 콘솔 분리, GUI 오류 표시. 집중 19/실패 0, 빌드 경고·오류 0 | 물리 입력 0건. 2.4절 참조 |
| 카메라 | [615](docs/exe/cpp-camera-pan-20261030.md), [617](docs/exe/cpp-camera-pan-presentation-20261030.md), [632](docs/exe/cpp-camera-center-20261031.md), [633](docs/exe/cpp-camera-immediate-20261031.md), [635](docs/exe/cpp-camera-viewport-20261031.md) | 원문 Alt/가운데 pan, 확대 전체화면 논리 단위, 직접 중심 목표, Home 계열 즉시 적용, viewport 변경 시 현재 값 보존 | 물리 키, 다른 DPI, 실제 포커스 전환 |
| Renderer·Native Draw | [618](docs/exe/cpp-renderer-prelude-20261030.md), [619](docs/exe/cpp-renderer-clouds-20261030.md), [620](docs/exe/cpp-renderer-frame-20261030.md), [622](docs/exe/cpp-native-scene-20261031.md), [628](docs/exe/cpp-mana-additional-20261031.md), [629](docs/exe/cpp-auxiliary-draw-20261031.md), [648](docs/exe/cpp-renderer-presentation-20261031.md), [655](docs/exe/cpp-mission-entry-20261031.md) | 앞단, 구름 4칸·배경 선택, 실제 Frame, 단일 표시 목록, 마나 103 추가 Draw, 배율·고정색, 부분 그리기/전체 출력 요청·첫 flags/count0 보존, 실제 Zone 뒤 초기화 | 전체 픽셀 동일성, 기존 VFX 계약·전체 비동기State7 |
| UI 갱신 영역 | [624](docs/exe/cpp-gump-redraw-20261031.md), [625](docs/exe/cpp-ui-dirty-20261031.md), [627](docs/exe/cpp-gump-derived-dirty-20261031.md), [630](docs/exe/cpp-inventory-blink-20261031.md), [631](docs/exe/cpp-minimap-dirty-20261031.md), [639](docs/exe/cpp-minimap-content-20261031.md) | Gump 기본 갱신, 지도 clamp, 기본 UI, 장식·메뉴 행, 생산 점멸, 미니맵 기본 영역·지면/소유자/삭제 픽셀. grid/debug 직접 writer와 생성 초기값 대조 | 일반 경로 활성 writer를 찾지 못한 grid/debug는 노출하지 않는다. 간접 alias 부재 전체 증명은 아니다. |
| State1→7 최초 파일/미설치 커서 실기 | [714](docs/exe/cpp-guide-fort-state-20261102.md) | 장기 파일 장치 지연 공유·State 전용 Cancel/실행기 존재 분배. 집중3개·최종 Early1/1024×768의1→7→8→14·시각/ESC/H 통과 | 실제5회 중 초기2제품 실패·3검사 전제 교정, v3 우발 승리 안내1회는 추가 검증하지 않았다. 물리/PCM/전체 동일성은 별도다. |
| State1 현재 Guide의 Fort 요청 | [713](docs/exe/cpp-guide-fort-request-20261102.md) | Champion/현재 handle·실제 Subscriber writer/공유 My·한 슬롯 Zone/State7과 편집 checkpoint 없는 조기 반환. 원문198/집중51개 실패0 | 713은 새 게임0/컴파일·정적 접합이며 최초 실제 전환은714에서 보완했다. State3 특이 분기·전체 동일성은 별도다. |
| Edit·시나리오 | [590](docs/exe/cpp-editor-scenario-20261030.md), [597](docs/exe/cpp-editor-rules-20261030.md), [609](docs/exe/cpp-world-geyser-20261030.md), [634](docs/exe/cpp-editor-catalog-current-20261031.md), [650](docs/exe/cpp-world-generation-session-20261031.md), [653](docs/exe/cpp-editor-default-name-20261031.md), [661](docs/exe/cpp-editor-entry-prelude-20261031.md), [662](docs/exe/cpp-editor-deferred-file-20261031.md), [663](docs/exe/cpp-fort-object-stream-20261031.md), [664](docs/exe/cpp-campaign-object-stream-20261031.md), [665](docs/exe/cpp-editor-invalid-file-20261101.md) | 시작값·작업본 왕복·방향별 동맹·세계 생성·Fort433개 이관/필터와 기본 이름 선택·늦은 파일 준비/Session checkpoint·State1 공통 앞단/52h 통지·선행 파싱 제거/실제 Campaign·Editor Objects 스트림·예외 수명/손상 파일 정리·기본 이름 재선택 반복 | 3절 2번.665의10개 통과는 전체 ABI/GUI 검증이 아니다. |
| Edit Unicode 파일 | [646](docs/exe/cpp-editor-unicode-20261031.md) | UTF-8/native 경로·wide CRT, 실제 Unicode 파일 왕복·권한·목록 이관/장부 재열기·삭제 보존·source TAFF 격리. 집중24통과 | GUI/전체34/한국어 UI 검증이 아니다. 최신 전체34/ZIP은644제품이며672의 기능별GUI와 구별한다. |
| Edit Template 쓰기 | [669](docs/exe/cpp-editor-file-writer-20261101.md) | 원문 r+b/실패 시 생성과 실제BaseFile 쓰기/반납. 짧은 이미지 뒤 tail 보존·실제35섹션 재파싱·현재 이름/서명·Unicode/보호/공유 거부. 관련28개/기존322행 통과 | 실제 부분쓰기/닫기 반환 실패는 새 관측이 아니다. 일반 Write/준비/복사와 전체 FILE ABI·GUI는 별도다. |
| 객체 메뉴·건설 | [524](docs/exe/cpp-object-hit-20261028.md), [528](docs/exe/cpp-factory-upgrade-runtime-20261028.md), [531](docs/exe/cpp-altar-salvage-runtime-20261028.md), [537](docs/exe/cpp-mission-cost-guard-20261028.md) | 픽셀 선택, 시험한 메뉴·업그레이드·회수, 미션 비용 관문 | 다른 타입·레벨·상태 |
| Factory 중복 경고 | [647](docs/exe/cpp-factory-duplicate-warning-20261031.md) | 실제 제공된 경고 callback 보존·Client 채널4 로그·명시 중복 입력의 경고 뒤 기존 객체 삭제. 최종1개 통과 | 일반 Pop 겹침 거부와 분리한 +20 장치 접합이다. 합법 GUI 배치/전체20/전체34 검증이 아니다. |
| HUD·단축키 | [509](docs/exe/cpp-hud-input-runtime-20261027.md), [511](docs/exe/cpp-player-list-runtime-20261028.md), [540](docs/exe/cpp-minimap-toggle-runtime-20261028.md), [547](docs/exe/cpp-send-money-runtime-20261028.md), [548](docs/exe/cpp-screenshot-runtime-20261028.md) | 미니맵, QWASZX, ESC 일부, Player List, 독립 미니맵, Send Money, F7·Shift F7 | 스크립트 입력이다. 3절 1번 |
| Chat | [640](docs/exe/cpp-ui-lifetime-current-20261031.md), [574](docs/exe/cpp-chat-completion-20261029.md), [586](docs/exe/cpp-chat-live-message-20261030.md), [614](docs/exe/cpp-chat-settings-20261030.md), [649](docs/exe/cpp-issue-commands-20261031.md) | 입력줄, 자동완성, 일반 Enter·중계·로컬 큐·공유 로그, 설정 재실행, 초기 점 명령/밝기·실제 오류 출력 | 3절 4번 |
| 미션 진입 UI·팔레트 | [656](docs/exe/cpp-mission-starting-tail-20261031.md), [657](docs/exe/cpp-mission-async-20261031.md), [658](docs/exe/cpp-mission-starting-commit-20261031.md), [659](docs/exe/cpp-mission-starting-prefix-20261031.md), [660](docs/exe/cpp-mission-starting-parent-20261031.md), [667](docs/exe/cpp-login-template-lifetime-20261101.md), [668](docs/exe/cpp-datamanager-dump-20261101.md) | UI/색 수명·Tick/큐/완료 공개·안내·7/8 확정·부모의 복사/로그/World 교체. 최초 파일 수명667, Stripped Image 덤프/같은 Client 진단668·관련20개 통과 | 재접속/Zone0·메타 Poll·시계 패킷·실제 GUI 전체 교체는 별도다. |
| 영상·CD | [594](docs/exe/cpp-movie-runtime-20261030.md), [595](docs/exe/cpp-cd-movie-package-20261030.md), [654](docs/exe/cpp-cd-presence-20261031.md), [655](docs/exe/cpp-mission-entry-20261031.md), [670](docs/exe/cpp-movie-override-20261101.md) | MovieGump/스킵·자연 끝/완료 저장·CD 경로/캐시와 시작/State7 결과 저장·영문/독문 동봉을 유지한다.670은 단일 override의 수집/Renderer·End/Kill/재진입 Close, 관련10개/기존810행을 확인했다. | 실제 CD 삽입/열거, PCM, 활성 전체화면 물리 출력.670 새2개는 최초 준비뿐이며 전체 영상 관측이 아니다. |
| 데모 | [567](docs/exe/cpp-demo-repeat-20261029.md), [569](docs/exe/cpp-demo-repeat-two-20261029.md) | 세 데모의 첫 360초 만료 → 재시작 → Exit Demo → Quit | 두 번째 전체 360초 |
| 저장·진행 | [568](docs/exe/cpp-saved-state-file-20261029.md), [578](docs/exe/cpp-campaign-progress-20261030.md), [588](docs/exe/cpp-battle-leave-20261030.md), [716](docs/exe/cpp-mission-editor-lifetime-20261102.md) | 저장상태 파일 정리·진행 키/다음 미션 해제·일반 중단State3, Early1 중단→Edit→시험 전투2회/재시작1회→복귀2회/이탈Cancel·No | 716은 승패 없는 검사 사본의 한 연속 경로다. 실제 승리부터 자동 저장까지는 사용자 확인, 일반 전투 저장/복구는8번 후순위MP다. |
| 메뉴·Help | [566](docs/exe/cpp-credits-roll-20261029.md), [600](docs/exe/cpp-cheat-menu-20261030.md), [603](docs/exe/cpp-help-menus-20261030.md) | Credits 자동 전환, ESC Cheat 직접 메뉴, Help·Technical Help·외부 링크 로컬 도우미 | 원본 픽셀·음향 |
| 의식 보상 | [598](docs/exe/cpp-new-technology-20261030.md), [599](docs/exe/cpp-ritual-refresh-20261030.md), [604](docs/exe/cpp-player-notices-20261030.md), [642](docs/exe/cpp-foreign-ritual-reward-20261031.md) | 로컬 기술·Core/snapshot·프레임말 결과와 비로컬 SP5000/별도144BYTE 기술 장부 실제 연결 | 707의 선택 UI/6b 요청은 정적 연결만 했다. 조기 희생 안내·비미션 실제 랭크 상승·MP 상태/사망 안내는 남는다. |
| 파일/TAFF | [643](docs/exe/cpp-taff-secondary-20261031.md), [644](docs/exe/cpp-basefile-read-policy-20261031.md), [651](docs/exe/cpp-basefile-io-20261031.md), [652](docs/exe/cpp-basefile-read-key-20261031.md) | 두 TAFF/열거·Help·UTF-8/W·경로 정책과 BaseFile11몸체·실제 디스크/TAFF/객체 덤프 장치.652 원문235행·집중15개 통과 | 전체41b4c0 FILE/예외 ABI·다른 쓰기/키/스트림 소비자·실제 손상/실패 수명 |
| 투사체·수명 | [552](docs/exe/cpp-bridge-runtime-20261029.md), [611](docs/exe/cpp-missile-floater-20261030.md), [612](docs/exe/cpp-ice-ricochet-scratch-20261030.md), [613](docs/exe/cpp-fence-event3-20261030.md) | 다리 붕괴·대피, 플로터 회피, 반사 변위 0, 펜스 사건 3 | 실제 특정 적 공격은 생략했다. 0.3절의 복원 한계 |
| 전체화면·해상도 | [호환 전체화면](docs/exe/cpp-modern-fullscreen-20261027.md), [가장자리](docs/exe/cpp-edge-scroll-runtime-20261027.md), [해상도 메뉴](docs/exe/cpp-screen-resolution-menu-20261027.md), [635](docs/exe/cpp-camera-viewport-20261031.md) | Windows 10 DIB 호환 출력, 가장자리 이동, 세 화면비 메뉴, 저장·종료·새 프로세스 적용 | 3절 7번 |

기본 채팅 구분선/숨김/복원과 실제 브리핑·중단 확인의 캡처 취소/미션 재진입은 [640](docs/exe/cpp-ui-lifetime-current-20261031.md)에서 같은637 제품으로 다시 확인했다. 전체 Chat·물리 입력의 완료 근거는 아니다.

### 2.2 미션별 상태

아래 장기 플레이 증거는 과거 각 제품에서 얻은 것이며 현재 제품의 자연 완주 회귀 결과가 아니다. 남은 확인 열의 자연 완주·승패·화면·음향은 사용자 확인 대상이다.

| 대상 | 확보한 증거 | 남은 확인 |
| --- | --- | --- |
| Early1 | 북서 간헐천 경로 A~G, 채취/귀환, SP 0→600, Tutorial2 진입 | 남동 경로 실패 원인, 실패/재시작/F8, 화면·음향 |
| Early2 | A~I, 신전·워크샾·지식·생산·발사기·회수(환불 75), Tutorial3 진입. 마지막 자연 완주 회귀는 467 제품 | 재시도·취소, 화면·음향 |
| Early3~6 | 진입/Start, 단계 판정, 짧은 진행. Early3은 468 Source1에서 채취/귀환, SP 2000→2200→2400, A→B→C. Early5는 과거 F(오벨리스크 획득)까지 | 각 자연 완주와 다음 미션. Early3은 최종 468 제품에서 완주한 기록이 아니며 attempt4의 종료 1도 보존한다. |
| 캠페인 1-1 | 자동 AI 세션의 캐논 적 신전 HP 5000→0(322). 2026-10-28 사용자가 다른 PC에서 직접 완주하고 다음 스테이지 안내 정상을 확인. 638에서 Restart·Leave 반복 확인 | 사용자 실행의 PC/커밋/SHA는 미제공이며 현재 제품의 자연 완주 검증이 아니다. 보호막 중심·제단 포획 사제색(541)의 실제 확인은 사용자 담당. 1-2 진입과 패배/Retry는 별도 |
| 캠페인 1-2 (MasterOfWhirligigs) | 진입/Start, 자동 AI/Kernel 400프레임(321), 645 전체34 재확인. **2026-11-01 사용자가 cpppj를 직접 실행해 완주: 적 사제 포획·희생, 승리 뒤 Next Missions 버튼으로 캠페인 1-3 진입 확인.** 사용자 실행의 PC/제품/SHA는 미제공이며 AI 검사 근거와 합산하지 않는다 | 플레이 중 제보된 원본과의 차이 2건(덱 재활성 시간·간헐천 그림자, 2.4절)의 점검. 이 제보는 1-1에서도 있었다는 인상이다. 패배/Retry는 별도 |
| 캠페인 1-3~1-6 | 진입/Start, 자동 AI/Kernel 400프레임(321), 645 전체34 재확인. 1-6 무입력 전투의 신전 파괴 뒤 섬 중립·기본 테마 복귀(575). 1-3은 사용자가 1-2의 Next Missions로 진입하는 것까지만 확인 | 자연 플레이, 승패, 후속 명령 |
| 캠페인 2·3장 | 645 전체34에서 진입/Start, 비정지 시각, 창 출력, 종료 0 확인 | 추가 TYPE 전체 수명, 자연 플레이 |
| Priest Training 9개 | 645 전체34에서 9/9 진입, 비정지 진행, 창 출력, 종료 0 확인 | 각 자연 완주, 완료 안내/메뉴 복귀, bc8 전체 원소 선택지 |
| 데모 3개 | 645 전체34에서 실제 메뉴 진입, 종료 0 확인. 첫 360초 만료 → 재시작 → Exit Demo → Quit 확인(567, 569) | 재시작 뒤 두 번째 전체 360초, 연속 반복/정리, 승패 |

Early3 후속 입력과 미실행 계획은 [부분 진행 보관](docs/exe/prepared/tutorial3-partial-468-20261027/ATTEMPT4-NEXT-C-H.md)을 본다. 다음 워크샾 메뉴의 71/75 생산·공급·사제 무력화/포획은 미완료다.

### 2.3 기능 영역별 상태

- **시작·메인 메뉴:** 무인자 `Netstorm.exe`가 GUI로 시작한다(637). 기본8/조건부Intro9버튼, Campaign/Demo/Help/Options·Credits·Quit·Tips, 실제 인트로 영상/Native39 복귀를 연결했다. 697~700은 실제 팝업/배치·Intro 감지·부모/모달/HTML 보존/45초 autoDemo까지 이었다. 승패 전환, 일반 전투 저장상태 복구(661에서 후순위MP로 분류), 미션 이탈 전체 선택, 전체 픽셀/음향은 별도다. Multiplayer는 앞 목표 뒤에 사전 구현한다.
- **Edit:** 기존 맵 자연 State1 설치, 전체 저장/Save as, 새 이름 목록/재시작, Set All Bridge, SaveGoMain, 신규 Fort/자동 섬 창, 기존·새 맵 섬 추가를 연결했다. Test Battle → 짧은 전투 → Return to Editing 왕복(492), 시나리오 시작값(590), 방향별 동맹(597), 세계 간헐천(609)을 확인했다. 남은 것은 3절 2번이다.
- **게임 중 UI:** QWASZX 취득/회전/배치/보충, 원본 ESC 가로/하위 메뉴, 실제 HUD 루트·도킹/덱·미니맵/NSInfo/Chat, 독립 미니맵, Player List, Send Money, F2·F3·F7·F8·Shift F7·Ctrl Shift F7, 가장자리 이동과 원문 pan, 실제 Chat 입력·전달을 연결했다. 기본 Chat 구분선 드래그/숨김/복원은543과640에서 확인했다. 나머지 View/zoom·도킹 조합·모드별 표시·픽셀 대조는 남는다.
- **공통 전략 AI:** 적·아군·데모가 같은 원본 전략을 쓰고 `RawCampaignSession::RunFrame`의 현재 시각 → 비정지 AI → Kernel 순서에 연결된다(320~322). 정상 간격의 확장/채취/생산/공격은 자연 플레이로 대조하지 않았다.
- **유닛·주문·의식:** 워크샾 25종의 생성/이동/공격/표시·소유자색(335~345), Devastation, 포획/운반/희생, SP/미션 완료, 은신/마비, 윈드 소환을 복원했다. 의식 집중 검사에는 명시 시전원/포로/시계 입력이 포함되며, 실제 UI 취득 → 시전 → 포획 → 희생의 자연 플레이는 미검증이다.
- **내부 수명:** Client/Session의 장기 전역, Pool/Graph/Player, Deck, 권한, 커서, 반납 조정기와 cleanup, 기존 맵 Edit 설치, Guide/World 분리, 작업본 reader, Login 재적재를 연결했다(425~486). 정상 State7은 `CleanupNativeStateWorld(1)`, 정상 MainMenu는 전체 cleanup(0)에 이미 연결돼 있다(638). 남은 것은 3절 3·4번이다.
- **배포:** [cpppj/package-cpppj.ps1](cpppj/package-cpppj.ps1)의 기본 증분 빌드 → ZIP, BUILD_INFO, 전체 자료/런타임, 인자 경계를 확인했다(562). 637부터 최상위 EXE 직접 실행이 기본이고, 창 모드 강제와 추가 인자용 배치 2개를 유지한다. 전체 게임 배포 검증과 구별한다.

### 2.4 알려진 미해결 이슈

- 2026-11-01 사용자 제보 2건(덱 재활성 지연 677, 간헐천 그림자 678)은 수정을 마쳤다. 범위와 한계는 2.1절 행을 따르며 자연 플레이 확인은 사용자에게 남긴다.
- **물리 입력 미검증(637,640,669,762):** 762에서 새부팅(11-02 20:24)/RDP세션1(20:25 로그인)·전경비NULL·원격오디오1개를 확인해 환경변화 뒤 기존VSCode 캡처를 fresh선택/재선택 총2회 재확인했다. 최초 FrameArrived timed out, 재선택 window capture timed out으로 모두 실패했다. activation/입력/새창/게임/PCM열기0이며 기존669의세션2 실패와 구별한다. 다음 실제환경변화전 반복하지 않는다. [762 기록](docs/exe/cpp-configuration-command-input-20261103.md)을 따른다.
- **비ASCII OS 글꼴 이름(774 완료):** ACP31BYTE와 UTF-8 Config/VFS 사이의 직접 Set·부팅 reader·CHFNT disk 변환은774에서 연결했다([기록](docs/exe/cpp-font-face-encoding-20261103.md)). 실제 설치 글꼴/다른 ACP/구형 무BOM 설정 이관의 확인은 별도다.
- **Early1 남동 경로:** D·SP 0에서 Next Tutorial을 찾지 못해 종료 1(417). 북서 경로는 통과하며 실패 원인은 미확정이다.
- **options.cfg 저장/복구(674 제품·675 도구):** 활성 매핑 1224 대응과 도구 복구는 완료했다([674](docs/exe/cpp-options-mapped-save-20261101.md)·[675](docs/exe/cpp-tool-settings-writer-20261101.md)). 수정된 추적 파일을 `git diff` 계열이 매핑하는 동안 실제 954바이트에서 같은 신호가 재현되며 볼륨 형식과는 무관하다. 남은 한계: 과거 개별 실패의 프로세스는 사후 특정할 수 없다. 실제 GUI/전체audio는 재실행하지 않았고 실패한 전체audio 원시는 672 live/audio-v2에 유지한다. 공유 삭제도 거부되면 이전 설정/changed/복구 파일을 보존하고 실패를 알린다. 프로세스 종료까지 매핑이 남으면 성공 백업/교체 폴더도 남을 수 있다. 부분 I/O·1176/1177 오류 주입은 미실행이다.
- **PlayerNoMake:** 경고 255DWORD/정리 `004328a0`의 장기 연결은 453에서 완료했으나 패킷 부모 `00433c30`의 실제 writer는 남았다. CD `00487900`은 spot 정리이므로 같은 함수로 취급하지 않는다.
- **섬 추가 후 Graph(720~723):** 원본 실제 New Map n720의 배치 직후 장부1/678·실제 소속670+noGraph4 → 재적재1/670을720에서 관측했다.721은 재적재 개별 소속을,723은 같은 후보8의 현재 새 맵 배치 직후674개 표면·성분670/장부678·noGraph4개 위치까지 대조했다([723](docs/exe/cpp-editor-graph-placement-20261102.md)). 차이8은 원본/현재에 모두 있어 정규화하지 않는다. 724에서 대표 두 섬/다리 연결·owner 변경·중간 다리 삭제와 두 재구성 분기의 전체 원본출력도 일치했다. 모든 레이아웃/전체 동일성으로 확대하지 않는다. 그림 변형/실행별 난수 동일성은 별도이며 원본 전역99cache의 Session 수명 차이는722에서 수정했다.

## 3. 앞으로 할 작업 (권장 순서)

아래 순서로 진행한다. 체크 항목(`[ ]`)은 AI 작업 대상이고, 5·6번은 사용자 확인 목록이라 단계 전환을 막지 않는다. 완료한 항목은 이 절에서 뺐다. 근거는 2.1절과 보관본([2026-11-02 정리 직전 원문](LEFT_JOBS.old.md#archive-20261102-before-cleanup), [2026-10-31 정리 직전 원문](LEFT_JOBS.old.md#archive-20261031-through-638))을 따른다.

1. 화면 캡처가 정상화되면 물리 입력 QWASZX·ESC·Home·포커스를 검증한다(7번).
2. 남은 비멀티 UI·Edit·시나리오·파일·수명·호환성을 1~4번과 7번으로 이어 간다.
3. 645의 같은644제품/새ZIP/전체34 뒤646~777을 진행했다. **현재 제품777/11.01은 TYPE의 실제 FILE/키 부모를 연결했다.** 다음은779/783 checking·781/782 GIF/초기화면·784 입력꼬리·785 보조음향이다. 이전 완료분과 사용자 직접 확인/MP 무시험 범위를 유지한다.
4. 체크 항목을 마치면 5·6번과 Windows 11 실기를 기다리지 않고 멀티플레이 무시험 사전 구현과 4~8차로 넘어간다(8번).

### 1. 대상별 커서·오브젝트 우클릭·게임 UI

- **완료분을 되돌리지 않는 원칙(2026-11-02 정리 때 완료 항목에서 옮김):** 681의 정적 대조로 원본83VT의 작성13개·비멀티 열기5개·특수 명령9개는 현재 소스에 연결돼 있고, 총7열기 중 메타맵2개(4789b0/479390)만 3차MP 이월이다. Dig Up/전환/다리/Meltdown/Detonate의 입력 연결을 다시 미구현으로 세지 않는다. 미니맵 grid/항목별 debug는 일반 경로 활성 writer를 찾지 못했으므로(639) 노출하지 않으며 간접 alias 전체 부재의 증거로 확대하지 않는다. 695의 개발자키55a484는 켜지 않는다.
- **메인 부모/팝업/버튼(697~700):** Help/Options 공통 생성기·현재 Client 조건·실제 버튼 기하/모달/큐,698 글자맞춤/두 행 배치,699 최초 Intro 감지/조건부9버튼/영상 왕복,700 실제 부모/모달·HTML 보존/일반 선택 Kill·Help serial과45초 autoDemo를 연결했다. [700](docs/exe/cpp-main-menu-parent-20261102.md)을 따른다. 메인 전체 예외/할당 실패·CD 대응·전체 픽셀/음향은 완료 판정하지 않는다.
- **메인 이후 다음 지점(700 뒤):** 남은 ESC/View·도킹·DD 조정/확인 UI와 Edit 지형/객체·시나리오 writer/파일 수명을 대조한다. 완료한Intro·실제 부모/HTML 보존·현재 모달·45초 autoDemo를 다시 정적 확인만 하는 작업으로 되돌리지 않는다. 자동 데모 실기는 초기 Demo 안내창까지이며 OK 뒤 진행/자연 완주는 사용자 확인 범위와 구별한다.
- **다음 유닛 선택(702 완료):** 원문004da880의 N/Shift+N 순환 검색·free 건너뛰기·소유자/TYPE 관문·첫 선택/가운데/정지/후속 선택과 실제 입력을 연결했다. 원문410·집중18개 실패0이며 GUI는 추가하지 않았다([702](docs/exe/cpp-next-unit-selection-20261102.md)). 기본 Shift 그룹 선택/이동701과 함께 미구현으로 되돌리지 않는다. 남은 커서/메뉴/Edit/수명은 아래를 따른다.
- **선택/회전/카메라 키701~704·717~719 완료:** Shift 그룹·N/ShiftN·P/R·H/F4/F5/ControlF5·C/c·Tab·숫자/Shift 저장 위치를 연결했다.718의 최근 로컬 배치5칸과719의 별개 저장10칸이 같은 Client 수명을 보존하며 실제 미니맵 이동/재시작 뒤 호출했다([719](docs/exe/cpp-saved-camera-key-20261102.md)). 미구현으로 되돌리지 않으며 물리 입력/전체 UI 동일성은 별도다. **U/u의 최근 손실 위치 이동은751에서 같은 공통 삭제 owner와 즉시 카메라에 연결했다.** 숫자 저장10칸/Tab 최근5칸과 별개이며 새 GUI/물리 키 검사는0이다.
- **RequestMenu 전송기:** 4d712e..4d717c의 서버 전송은 3차 정적 사전 구현에 남긴다. RecheckCD의 현재 serial 정리는 696에서 완료했다.
- [ ] 크리스탈의 나머지 상태/명령, 비석의 미검증 특수 명령, 제단/일반 건물형의 나머지 타입·소유자·레벨·손상·건설/생산·포획/희생 상태별 메뉴·명령 효과·반납과 선택기 잔여를 점검한다. 이미 확인한 비석 기본 메뉴/전환과 소유자 일부를 미구현으로 되돌리지 않는다.
- [ ] 운반체 전체 종류와 나머지 커서 조합을 점검한다(536,576). **기본 Shift 다중 선택·승격/제외·30칸 보조 목록·그룹 이동은701 완료**이며 다시 미구현으로 세지 않는다. 실제 화면/물리 입력·모든 종류/조합은 별도다. 공격형 공중3종은 정적 분석만 한다.
- [ ] 남은 ESC 하위/View·도킹 조합·DD 조정 표시·Chat 입력과 모드별 표시/픽셀을 확인한다. 741~758의 F6/ESC/현재 demo/전체화면 F12/부착SP/화면변경·DD조정/Pause/T/t/D/d·파일/지식 메뉴 완료 근거는2.1절과 각 단위 문서를 따른다. F9는 정지 앞, F6는 정지 뒤 예약을 유지하며 완료된 단축키·독립 미니맵·Send Money를 새 근거 없이 반복하지 않는다. 글꼴/음질·glyph cache는764에서연결했다. 존재조회/세소비자는765에서연결했다. Help stream writer는766에서연결했다. QuickHelp 최초reader는767에서연결했다. TYPE reader는777에서 연결했다. 다음은 GIF/초기화면 reader·checking 소비자와 남은 일반명령이다. 등록 snapshot/무진단 optional과 OS 직접 ReadFileBytes 계약은 보존한다. 원문 등록/도달성을 대조하며 활성 caller가 없는 관리명령을 기본 UI에 임의 노출하지 않는다. 퇴역명령 AdvanceRank/SpawnScreenChange/Stall/BridgeLikeCrazy/1/CreateRandomGuns/DumpProcessList/DumpSquids/DestoryAllPost는 비교 뒤 무효과이므로 기능을 합성하지 않는다. 전체 Console/모달/픽셀·32비트 FILE/FileSpec/SEH ABI 동일성은 좁은 검사로 완료 처리하지 않는다.
- 메타맵 번개 `00479650`은 MP 전용 실제 기록 분기이므로 8번의 무시험 사전 구현에 포함한다. 일반 번개/진단(559~565)과 구별한다.
- **647 메뉴 정적 점검:** c2(Test Battle/편집 복귀)와c5(Leave)는 이미 처리기에 연결돼 있다. c6(Refresh Zones)/db(Declare a Draw)는 메타맵/일반 Battle 전용이므로 후순위MP로 잇는다. 명령 표에 있는 값만 보고 비멀티 미연결로 되돌리지 않는다.
- **오류 복원 원칙:** 실제 오류의 원문 몸체/호출 순서를 대조한다. guard 제거, 배열 임의 확장, 손상 후보 무조건 무시, 진단 6 강제, 임시 속도로 숨기지 않는다.

### 2. Edit 메뉴·커스텀 맵/시나리오

- **Edit 현행 경계(716):** Early1 중단 뒤 실제 Edit·Test Battle2회/Restart1회·편집 복귀2회·LeaveEditMode Cancel/No의 연속 수명을 같은715 제품에서 확인했다([716](docs/exe/cpp-mission-editor-lifetime-20261102.md)). 이를 전체 시나리오/파일 실패로 확대하지 않는다.715의549문서/43이름과714의State1/7 보완도 미구현으로 되돌리지 않는다. 남은 지형/객체·동적/Config writer·파일 실패·Graph를 이어 간다. State3 특이 분기는 도달 미입증/미연결이다.

- **편집/우클릭 입력(748~750 완료)/다음:** owner/Ctrl다리순환·모드1취득/취소·선택정리/폴링분리·객체가상ac반환/성공음향·Ctrl+special직접가상70지점진단을연결했다. special0055a480의현재값만읽고개발자0055a484를켜지않는다. challenge선행클릭/빈점항해메뉴·모드6ChalSailPos소비자는MP후속무시험범위로구별한다. 남은비멀티명령/UI·파일/수명/시나리오/호환성을점검하며이번단위를일반입력/전체목표완료로확대하지않는다.
- [ ] 다른 시나리오 조합·실제 설정/UI writer와 TARC/동적/Config 확장·남은 명령 부모를 대조한다.715는 명시549문서/Menu 둘째 인자 조사·실제43이름/172표기·한 메타/큐 접합까지 확인했다.687/715의 명시 목록이나 이름 일치를 전체 동적 실행 완료로 판정하지 않는다.
- [ ] 남은 지형·오브젝트·소유자 도구와 LeaveEditMode/State1·비동기 State7의 전체 분기를 점검한다. 726의 같은 워크샾 owner0/2/8·취소·Yes 저장/재적재는 완료이며 전체 소유자 도구/타입으로 확대하지 않는다([726](docs/exe/cpp-editor-owner-save-20261102.md)).
- **대표 Graph 경로 완료(724):** 두 실제 섬/다리94개 연결·owner 변경·중간 다리 삭제의 재구성 전후 성분2→1→2와 원본10호출의 전체풀/표/스택·해시/점유가 일치했다.723의같은후보배치 직후678/실제670·noGraph4개와721재적재670도 완료다. 전체레이아웃/그림동일성은사용자확인대상이고, 작업본 저장/재파싱·이름/목록·미션 전환의다른수명은남는다.
- [ ] 나머지 owner 조합/시나리오 조건·지형/오브젝트 도구와 전체 언어 UI/폰트·파일 실패 수명을 대조한다. 원문 언어 번호/용어표710·최초 다중 Xlat711과 언어 변경 요청/기존 Chat 알림712는 연결했다. 590의 명시 `.french`는 프랑스어 UI나 한국어 완료가 아니며, 6차의 메뉴 전체 경로/문자열/글꼴은 남는다. TEST01/TEST02 원본은 읽기 참조만 하고 저장은 작업본에 한다.
- **목록 상한:** 원문의 첫 50개 상한은 5차 전까지 유지한다. 실제 Fort 433개는 종류2가 432개, 종류0이 1개여서 옵션0만으로 TEST02를 앞 50개에 넣지 못한다(634). 실제 Edit 검사 방법은 590·633·634 문서를 따른다.
- **판본 차이:** 10.78 기본 설명의 원문 `{Text}` 리터럴은 유지한다(494). 10.72 자료와 혼동하지 않는다.
- **영문 파일 부재 개선은8차로 이월(98b2f20c):** 현재 원본 복원 단계는 `.english` 부재 시 Test Battle 종료를 유지한다.8차에서 새 맵의 기본 english 자동 생성과 시나리오 UI/부재 종료를 개선한다. 지금 임의 fallback을 넣어 완료 처리하지 않는다.

### 3. 미션 수명·진행 저장·데모 반복

- [ ] Retry·Next Tutorial·Next Mission·Abort의 나머지 연속 전환과 참조/타이머·다리/AI 재고를 확인한다. Restart와 Leave → 메인은638, Early1 중단→Edit→시험 전투/재시작·복귀·Cancel/No는716에서 확인했다. 자연 승패가 필요한 부분은 사용자 확인 대상이다.
- [ ] 세 데모의 재시작 뒤 두 번째 전체 360초(다시 만료)와 추가 연속 주기·나머지 안내를 확인한다. GuardMyBack의 15초 안내와 DissolvedAlliance의 Demo 섹션 부재를 유지하며 Script demo 키만으로 모드를 추정하지 않는다.
- [ ] 승패를 포함한 나머지 Native3 선택과 State1/단독 State7·전투 정리004b2df0의 자연 Tick/연속 전환을 확인한다. 존재하는 일반 전투 상태 복구/StateReconnect·서버 이전은661의 정적 분류에 따라8번의 후순위MP로 잇는다. 질문/삭제 UI는 유지하며 reader/open만으로 재접속 완료로 세지 않는다. LeaveBattle의 나머지 이유/CD/원격 페널티와 State6도 별도 잔여다(588).
- **661 State1/7 경계:** 정상 단독 State7 부모의 최초 복사/로그·World 교체·7/8 확정은660에서 연결했다.661은 State1의 공통 출처/첫BYTE/이전 Battle14 통지를 State3과 공유했다([661](docs/exe/cpp-editor-entry-prelude-20261031.md)). 확인한 current+84 직접 작성은 일반 전투 서버 이전이며 !missionPost/PlayerKnown 저장 writer와 연결된다. 해당 재접속·Zone0/메타 Poll/시계 패킷·외부 동기화는8번의 후순위MP로 구분한다. State1 현재 Guide의 요청/조기 반환은713에서 연결했다. 714는 Early1의 실제State1→7→14/시각/H/ESC를 보완했다. State3 특이 분기·State1 나머지 조건/도구·다른 전환 전체는 남는다. 간접 alias 전체 부재나 모든 State7 분기 완료로 확대하지 않는다.

### 4. 내부 수명·실제 연결

- [ ] 일반 전투 saveBattle·metamap/비mission phase14 저장과 나머지 파생 프로세스 수명을 복원한다. Early1 중단→Edit의 mission/모드와 시험 전투·재시작/복귀·이탈 수명은716에서 확인했다. saveBattle 미연결 선택은 638의 Guide 경로 통과로 완료 처리하지 않는다.
- [ ] 남은Chat모드별표시·전체팔레트전환·경로작성기잔여소비자를확인한다. 전체459c60/10칸/HUD727·장치/VFS/저장상태728·옵션/Capture/CD/Help729~730·Config Load/FileSpec/CLI731·Startup파일명732·디렉터리/모듈경로734·6후보setup/가상CWD/파일루트735·Full/같은FileSpec/Next/설정Load뒤반납736·생성자선행TAFF제거/실제Run등록순서737·실제Edit/목록/Template파일조회공유고리738·저장/삭제공유고리와직접chmod경계740은완료다. 전체최초FILE/리소스I/O·다른FileSpec/전체264BYTE ABI/검색실패·Template·일반FILE/TAFF/보조실패는남는다. 원격·메타맵·시스템뉴스실제UI·기타dot명령도잔여다. 입력573/574·설정/재실행614를전체Chat GUI완료로확대하지않는다. **752 완료:** MatchDisk는 같은 FileSpecQuery/가상CWD 검색 장치를 사용한다.현대 VFS/EditorFiles/장부/Catalog의 '/' 확장은 해당 호출 경계에서 native로 바꾸며 원문 비패턴 접두·UNC/루트·패턴의 backslash 접두를 보존한다.전체ABI/다른실패는 남는다. 현재 등록 dot 명령은 desc만 MP 후속이며 나머지와 실제 도킹은 이미 연결돼 있다.
- **651 정적 경계:** 번개 색 범위228..245/현재228·18, 합법 현대 모드의 비flip과 색 소비자는 기존 연결을 확인했다. 전체 GUI/픽셀 완료는 아니다. PlayerNoMake writer `00433c30`은 객체 수신 생성/검증 부모라 후순위MP 무시험 사전 구현으로 잇는다.
- [ ] BaseFile의 다른 쓰기/키/스트림 소비자·실제 손상/할당·부분 I/O 실패 수명, 보조 음향과 비미션 전체 랭크 상승을 연결한다. **757 완료:**41b4c0전체출처부모·일반/부분FILE·키·실제TAFF metadata/current FILE과객체덤프재읽기/재쓰기를연결했다.32비트FILE/FileSpec/예외SEH ABI는호스트소유경계와별도로판단하며전체동일성을주장하지않는다.명시41baa0와ObjectDump는763에서연결했다. 나머지실시간reader는각caller계약대로767/769/771등에서잇는다.0044fa00은004b54e0 재접속 저장까지 복원해야 한다. CD 일반 열기는4ba5d0/부분 파일은4ba630이며 자동 정밀 후보의 반대 대응을 그대로 따르지 않는다(652).
- [ ] 조기 희생 안내를 확인한다. 실제 missionPost 0의 일반 전투 경로라 후순위로 남겼다(615).
- [ ] 원본 32비트와 현재판의 스택 배치·미정값 자체 일치 여부는 별도로 판단한다(612).
- [ ] 연결 활동 frame22/주기64·timeout/UI·endpoint/server/boss와 실제 리슨/수락·22칸 송신 writer는 멀티플레이 후순위의 무시험 사전 구현 범위로 잇는다.
- 근거는 [월드 장기 소유](docs/exe/cpp-state-world-owner-plan-20261025.md)·[State Client 공백](docs/exe/cpp-state-cleanup-client-gap-20261025.md)·[직접 전역 소유/반납](docs/exe/cpp-state-cleanup-global-owners-20261025.md)을 따른다. 오래된 PLAY 목록의 완료 여부는 현행 코드/후속 단위와 대조한다. ClearWorld의 오래된 ‘후속’ 주석과 과거 428 문서만으로 미연결 버그를 주장하지 않는다.

### 5. 원본 동일성 대조 — 사용자 확인 대상

전체 동일성은 사용자가 실행·플레이하며 확인한다. AI 작업 대상이 아니며 아래 항목이 남아 있어도 다음 단계로 넘어간다. 기능을 구현할 때 해당 부품을 원문과 대조하는 일은 계속 AI가 한다. 단, 사용자가 플레이 중 원본과의 차이로 제보한 항목은 AI 작업으로 올린다(1.1절). 간헐천 그림자와 덱 재활성 시간(2026-11-01)이 그 예이며 3절 1번에 있다.

- 목표 TYPE의 대기/건설/발사/피격/파괴/포획/희생 프레임·방향·색표·그림자·발 기준점·가림/파편/지형 효과와 캠페인 2·3장 추가 TYPE의 전체 수명
- 메뉴·브리핑·생산/지식·선택/사거리/비용·커서/회전/회수/업그레이드·우클릭/단축키·Pause/Help·카메라/도킹/View/Chat/PlayerList의 남은 조합/픽셀. 좁은 기능 완료를 전체 동일성으로 세지 않는다.
- 건설/발사/적중/출격/귀환/연소/붕괴/포획/의식/승패/음악의 파일·시점·반복·위치, 정지 중 시간/난수와 정상 간격 공통 AI
- 장시간 플레이·메뉴/데모 반복의 자원 누적·정지/크래시
- Credits의 원본 픽셀/음향 전체 동일성. 566은 no-audio/일부 현재 캡처이며 원본 실행 비교가 아니다.
- 녹화의 HDR 밝기·방송 음성을 게임 효과로 오인하지 않는다. 표시 변환 픽셀 일치와 전체 외관 동일성은 별도다.

### 6. 자연 완주·테스트 지원 — 사용자 확인 대상

모든 시나리오(Early Missions, Priest Training, 캠페인, 데모)의 자연 완주는 사용자가 실행·플레이하며 확인한다. AI 작업 대상이 아니며 아래 항목이 남아 있어도 다음 단계로 넘어간다. 자동 공략과 테스트 지원은 별도 요청이 없으면 진행하지 않는다.

- Early3→6, 캠페인 1-1의 포획/운반/희생/승패 → 1-2, 나머지 1장 → 2/3장, Training 9개와 선택지, 데모의 자연 완주. 현재 제품의 자연 완주가 완료됐다고 쓰지 않는다. 캠페인 1-2는 2026-11-01 사용자가 직접 완주(적 사제 포획·희생, Next Missions로 1-3 진입)를 확인했다. 사용자 실행의 제품/SHA는 미제공이므로 현재 제품의 자연 완주 회귀 결과로 쓰지 않는다.
- 실제 승리 뒤 완료값 0→1 자동 저장과 재실행 후 다음 미션 신규 해제. 명시 키의 저장/재실행 해제는 578에서 확인했다. 기존 완료값 1 보존은 신규 해제의 증거가 아니다.
- **549 자동공략 잔여:** `CampaignForwardBattle_NaturalTempleAssault`는 합법 사격 배치 지점 부재로 실패했다. 제품/전술 입력 원인은 미확정이고 준비 전략은 미채택이다. 원래 함수/전략/승리 단언은 유지하며 별도 요청이 있을 때 원문과 대조한다. 전체 C++ 통과로 기록하지 않는다.
- TEST02의 명시 로컬 owner AI 옵션은 별도 테스트 지원 제안이며 미구현이다. **원문 기능의 누락과 구별한다:** 2026-11-01 정적 재확인에서 원문004c2b20도 `owner!=myPlayerId || demo!=0`일 때만 사제가 있는 owner의 AI를 만든다. 현행 RawMissionStart는 이 조건을 유지한다. 별도 지원을 구현할 때 일반 demo0의 공통 AI를 로컬 owner에도 쓰되 `gate.demo=1` 우회로 튜토리얼/Demo 사건/타이머를 바꾸지 않는다.
- **TEST02 포획/희생 보류:** 사용자 수정판 `3c12b4e7`/Fort 5282바이트의 현재 SHA/배치를 다시 확인하고 옛 좌표를 재사용하지 않는다. 기술은 크로스보우·썬 디스크 발사기·맨오워풀·데빌메이커 우선이며 훨리베이스는 제외한다.
- 로컬 AI의 SP/Tech/이름/Off/NoTemple/동맹/능력은 적 AI와 따로 준비하고 공유 난수 차이를 기록한다. 기본 게임/프로필에 섞지 않고 검사 조작임을 주석/기록에 표시한다. 우리 편 AI·빈도/SP 추가는 [검토](docs/exe/cpp-test-local-ai-and-sp-feasibility-20261021.md)만 했으며 구현 완료가 아니다.

### 7. Windows 10/11 호환성과 전체화면

- [ ] 화면 캡처 정상화 뒤 물리 입력 QWASZX·ESC·Home·포커스를 검증한다.762의 새부팅/RDP1에서도 VSCode fresh선택/재선택2회가 timeout이므로 다음 실제환경변화전 반복하지 않는다. 관측이 안 되면 입력을 보내지 않고 앱 내 검사 입력을 물리 입력으로 합산하지 않는다.
- [ ] Windows 10에서 다른 모니터/DPI, 실제 다른 앱 포커스 전환, DD 조정의 다른 비트/실제 장치 실패 경계를 검증한다.746의 단일 마우스 조정/취소/확인/15초만료·저장재시작과 명시 불법 모드 복구는 완료했다. 메시지 주입을 실제 앱 전환으로 합산하지 않는다.
- [ ] 실제 PCM 출력과 사용자 활성 전체화면의 물리 출력을 확인한다. 594는 waveOut0/전경NULL이었다.762 읽기조사는 새RDP의 원격오디오1/전경비NULL을 확인했으나 PCM열기/출력0이며 실제청취성공이아니다. 실제 CD 삽입 W 경로도 미검증이다(595).
- [ ] 전체화면 종료 후 반복 재실행과 화면비/스크롤 회귀를 유지한다. 원본과 다른 호환 처리는 이유를 한국어 주석에 남긴다.
- Windows 11 실기 검증은 단계 전환 조건이 아니다. 위 항목을 Windows 10에서 마치면 다음 단계로 넘어간다(1.1절).

### 8. 배포·문서·후속 단계

- [ ] 2.4절의 다른 미해결 이슈를 처리한다.
- [ ] 싱글플레이/UI/호환성 뒤 TCP/IP 로컬 멀티플레이·Outpust/전용 분기를 디컴파일 근거로만 사전 구현한다. 신규 단위/게임/실제 통신 검사는 하지 않고 동작·동일성·안정성 미검증을 기록한다. 공식 서버 복구는 가정하지 않는다. 메타맵 번개 `00479650`의 실제 기록 owner/getter와 MP Challenge 참가자/전투방 이미지·텍스트 `478d10`도 이 범위에 포함한다. 일반 전투 !state.dat 저장/복구·startup StateReconnect/서버 이전·Zone0/메타 Poll/4c1e70 시계 패킷도661의 정적 분류를 따라 포함한다. 기존 질문/삭제 UI는 유지하며 신규 검사는 하지 않는다.
- [ ] **4~8차 후순위:** 3차 멀티플레이 뒤에 1.1절 순서대로 진행한다. 모두 미착수다.
  - **4차 로컬MCP/해상도:** 모든 게임 조작·캡처·소리 포함 녹화와16:9 최대1920×1080을 제공한다. 이 단계의 그래픽 크기 스케일 조정은 불필요하다. 5867f35e 기준이며 기존 BMP 수집/외부 검사/화면비 처리만으로 완료 처리하지 않는다.
  - **5차(aaafbbc4,733 반영):** 일반 권한·읽기 전용 설치에서도 정상 실행되도록 **모든 자동 생성 파일/디렉터리와 reader**를`%USERPROFILE%\Saved Games\Netstorm\` 아래로 전환한다. 설정/진행은Saves, 신규맵은CustomMaps이며 원래/사용자 맵 병행 읽기·목록 스크롤/무제한도 포함한다. [12파일군/50정적후보](docs/exe/cpp-phase5-file-inventory-20261102.md)와766에서연결한NetStormPieceList.html writer/PieceDump file링크·안내의 옵션 교체 임시/복구·!state·전체화면 표식 reader/delete·Edit 작업본/장부/잠금/pending·GIF/BMP·squids/procSumm CWD 경로·추후 MCP 생성물까지 다룬다. 이관/우선순위·두 설치 충돌/장부 identity와 명시 export 정책을 결정하고 원본과 다른 이유를 한국어 주석에 남긴다. 새 설치 사본의 실제 쓰기 거부 권한/일반 권한·다른CWD/Unicode·저장/재적재/중단/교체/전체화면 재실행으로 확인하며 아직 구현하지 않았다.
  - **7차:** AA/FSR1.0은 각각 Options에서 선택하며 기본off다. FSR지원환경에서2560×1440/3840×2160을 목록에 추가하고 이 해상도 선택 시FSR을 자동으로 켠다. 실질1920×1080을 목표 해상도로 업스케일한다. 참고 저장소는1.1절의FidelityFX-FSR이다.
  - **8차(98b2f20c):** Edit 안의 별도 UI로 시나리오 초기 자본/테크·적·동맹 등을 편집한다. 새 맵의 기본 english를 자동 생성하고 english 부재 Test Battle 종료를 개선한다.1~7차 뒤 구현하며 기본파일/저장/재적재·기존수동스크립트 보존을 함께 확인한다.
- 60·120프레임과 CD 10.72 전체 복원은 별도 후순위다. CD의 Windows 10 구동 불가 해결책도 해당 작업 때 문서화한다.
- README는 제품이 바뀌면 해당 근거로 다시 갱신한다. DN/WB의 별도 상태와 cpppj 과거 기록은 보존한다.

## 4. 운영 규칙과 유효한 사용자 결정

- **재개(2026-11-03 사용자 /goal):**775 완료 뒤의 일시 정지를 해제하고776부터 계속한다. 지정 플레이/사용자 직접 확인/멀티플레이 검사는 건너뛰며 단위마다 인계·로컬 커밋을 수행한다.776~785의 준비·필수 교정·로컬 백업은 [재개 준비 인계](docs/exe/cpp-pause-handover-20261103.md)에 있다.

- **진행(2026-11-02 사용자 /goal):** 모든 AI 작업을 끝낼 때까지 진행한다. 단위별 구현·집중 검사·인계를 갱신하고 로컬 커밋을 시도한다. 현재 완료 제품과 근거는0.1절을 따른다. 전체 목표는 미완료다. 지정 플레이/사용자 확인/MP 검사는 건너뛰고 후순위MP/3~8차 순서는 유지한다.
- **스테이지 실행 검사 빈도(2026-10-31 최신 사용자 지시):** 현재 진행 중인645의 검사는 마저 진행한다. 다음 주기부터는 게임을 실행해 각 스테이지에 진입하고 기다리는 검사를 훨씬 드물게 하며, 더 많은 기능 묶음을 구현한 뒤 한꺼번에 수행한다. 소수의 작업 단위가 끝날 때마다 전체34 또는 동일한 진입 검사를 반복하지 않는다. 중간 확인은 빌드·정적 원문 대조·필요한 집중 검사 위주로 한다. 새 실제 실행은 변경과 직접 관련된 확인이 필요한지 먼저 판단하고, 이미 통과한 진입을 관례로 반복하지 않는다. 같은 취지로 매 단위 34경로·긴 Early 완주·전체 CTest를 반복하지 않으며, 옛 3~5단위 기준을 자동 실행 조건으로 쓰지 않는다. 자연 완주는 사용자 확인 대상이므로 통합 검사에 넣지 않는다.
- **CHEAT 표시(2026-10-28 사용자 확인):** CHEAT 문구가 있는 항목은 게임 진행 중에 나타나지 않고 Edit 모드 편집에서만 나타날 수 있다. Edit 안에서도 각 항목의 원본 추가 조건을 적용한다.725에서 실제 공통/오벨리스크 메뉴의editor0/special1 노출도 차단했고8조합으로 확인했다. 원문 독립작성기의special 분기는 참조용으로 유지한다.
- **기록:** 정적 대조·명시 입력·컴파일·C++ 검사·GUI 관측을 구별한다. 최초 실패/교정 원시는 지우지 않고 기대값을 제품에 맞춰 약화하지 않는다. 서로 다른 제품의 성공을 합산하거나 준비본을 제품 통과 수에 넣지 않는다.
- **실제 실행(2026-10-27):** 메뉴/스테이지 진입·수 초 진행의 짧은 검증은 가능하다. 스테이지 클리어는 사용자 확인 대상이다. 실행 전 확인 여부는 0.2절과 AGENTS.md를 따른다.
- **Test Battle 언어파일(2026-11-02 갱신):** 원본 복원 단계의 `.english` 부재 종료는 유지하되, 사용자98b2f20c의8차에서 기본파일 자동 생성/시나리오 UI와 함께 개선한다.1~7차 중 임의 fallback으로 먼저 바꾸지 않는다.
- **테스트 조작:** `--test-bridge-patterns` 등은 원본 분석 결과와 구별해 주석/기록에 표시하고 기본 게임·적 AI·저장 프로필에 섞지 않는다. 로컬 AI/TEST02 방침은 3절 6번을 따른다.
- **실행 파일 이름:** Windows 게임 실행 파일은 `Netstorm.exe`다. CMake 대상은 NetstormCpp를 유지하고 OUTPUT_NAME으로 산출물 이름을 정하며, import library는 정적 `netstorm.lib`와 겹치지 않게 ARCHIVE_OUTPUT_NAME NetstormCpp로 분리한다(580, 581). 이전 단위의 보고서·원시 로그에 남은 `NetstormCpp.exe`는 당시 이름으로 보존한다. 원본 `Netstorm.exe`와 구별할 때는 프로세스 이름이 아니라 전체 경로를 쓴다.
- **실행 방식:** 인자 없는 실행은 GUI이고 `--help`와 명시적 `--run`·분석 명령은 유지한다(637). 자료는 실제 EXE 옆 `game/netstorm.tarc`를 우선하며 cwd의 다른 게임이나 저장소의 보호 원본을 자동 탐색하지 않는다.
- **보호:** AGENTS.md와 `originals/`·`originalCD/` 기존 파일은 수정/삭제하지 않는다. AGENTS.md가 허용한 `options.cfg`·`fullScreenStateFile.dat`는 예외다. 게임 실행 전후 원본 전체 해시를 비교하고 허용 설정을 복원한다.
- **빌드:** HJOW-Athlon은 Visual Studio 18(2026)/MSBuild 18.x를 쓴다. 최근 명령은 `cmake --build cpppj/build --config Release --parallel 4 -- /p:UseMultiToolTask=true /p:MultiProcMaxCount=4 /p:EnforceProcessCountAcrossBuilds=true`이며 앞선 컴파일러/CMake 자식이 모두 끝난 뒤 시작한다([빌드 안내](docs/cpp-build.md)). HJOW-X3D는 Visual Studio 17(2022)/v143이고 cmake가 PATH에 없어 BuildTools 내장 cmake를 전체 경로로 부른다.
- **UTF-8:** 새 문서/코드와 한국어 주석은 UTF-8이다. PowerShell 5.1의 Python stdin은 `$env:PYTHONUTF8=1`만으로 부족하므로 한국어를 넘길 때 `$OutputEncoding = [System.Text.UTF8Encoding]::new($false)`도 설정하거나 UTF-8 파일을 사용한다. 원시 로그/Native fixture는 인코딩 정리로 덮어쓰지 않는다.
- **Git:** CRLF 보존을 위해 확인/stage에 `git -c core.autocrlf=false`와 필요 시 `-c core.whitespace=cr-at-eol`을 쓴다. 저장소/전역 설정을 바꾸지 않고 해당 작업 파일만 stage한다. 최초 관측 바이트가 중요한 원시 TSV는 `-text` 속성으로 고정한다(589). 커밋은 `YYYYMMDD-NN PJ 요약`이며 당일 마지막 번호를 `git log`로 확인한다(cpppj CP, dotnetpj DN, webpj WB, 공통 CM). 원격 push는 금지다.
- **도구:** `tools/cpp_solo_entry_smoke.py`(34경로, 데모는 `--demos`), `tools/cpp_demo_start_smoke.py`, 자연 Early1/2용 `tools/cpp_tutorial_harvest_smoke.py`·`tools/cpp_tutorial_two_smoke.py`, ESC용 `tools/cpp_game_menu_smoke.py`·`tools/cpp_game_menu_pause_smoke.py`. 실제 호출 인자/최근 batch는 작업 PC의 `extracted/` 드라이버를 따른다.

## 5. 주요 참고 문서

- [C++ 빌드](docs/cpp-build.md), [cpppj 로드맵](docs/cpp-roadmap.md), [플레이 가능 계획](docs/cpp-playable-plan.md), [소스 맵](cpppj/SOURCE_MAP.md)
- [모듈 맵](docs/exe/modules.md), [정밀 디컴파일 신뢰도](docs/exe/decompile-reliability.md), [2026-10-23 양판본 정밀 디컴파일](docs/exe/cpp-decompilation-20261023.md)
- [원본 자동 탐험 analyzeManager](docs/analyze-manager.md), [유닛 25종 점검](docs/exe/cpp-unit-restoration-20261022.md), [의식 집중 검증](docs/exe/cpp-ritual-runtime-focused-20261021.md)
- [준비본 보관](docs/exe/prepared/README.md): 443/444는 통합됐고 469 준비본의 Subscriber/공유표/NewFort/섬 추가는 후속 단위에서 채택됐다. 개별 보관본의 미적용/미빌드 문구는 보관 당시 상태다. 보관 base/patch/manifest/142파일 원시는 유지한다.
- [Edit 왕복/냉각](docs/exe/cpp-ice-cooling-20261027.md), [새 맵 실제 저장](docs/exe/cpp-editor-new-map-live-20261027.md), [기존 맵 섬 추가](docs/exe/cpp-editor-existing-islands-20261027.md)
- 단위별 상세/최초 실패/증거 해시: [과거 이력](LEFT_JOBS.old.md), [단위504까지](LEFT_JOBS.old.md#archive-20261027-through-504), [단위549까지의 0절](LEFT_JOBS.old.md#archive-20261029-section0-through-549), [단위561까지의 3절](LEFT_JOBS.old.md#archive-20261029-section3-through-561), [단위638까지의 전체 원문](LEFT_JOBS.old.md#archive-20261031-through-638), [단위695까지의 전체 원문(2026-11-02 정리 직전)](LEFT_JOBS.old.md#archive-20261102-before-cleanup)
- **2026-11-02 문서 정리(HJOW-Athlon, 사용자 요청):** LEFT_JOBS.md와 LEFT_JOBS.dotnetpj.md를 함께 정리했다. 빌드·검사·게임 실행·디컴파일은 하지 않았고 제품(695/11.01)과 마지막 디컴파일 호스트(HJOW-Athlon/2026-10-23)는 그대로다. 이 문서에서 뺀 것은 0.1의 반복 수명 행, 1.3의 AGENTS 변경 커밋별 이력, 2.4의 완료된 이슈와 설정 저장 설명의 중복 부분, 3절의 완료(`[x]`) 항목, 4절의 대체된 재개·주기 지시다. 완료 항목의 보호 원칙은 3절 1번 첫 항목으로 옮겼다. 보관본의 “다음”이나 “중지”는 당시 이력이며 현재 작업 순서로 재사용하지 않는다.

## 인수인계 문서 임시 분할

dotnetpj 와 병행 작업하기 위해,
이 LEFT_JOBS.md 에는 당분간 cpppj 및 디컴파일 관련 진행 상황과 그 인수인계 내용을 적는다.
dotnetpj 진행 상황과 인수인계 내용은 LEFT_JOBS.dotnetpj.md 에 적는다
cpppj 1차 목표 달성 및 윈도우 10, 11 호환성 작업, MCP 추가 작업까지 완료되어 dotnetpj 구현에 cpppj 를 활용할 수 있게 되면, 인수인계 문서를 다시 LEFT_JOBS.md 로 통합한다.

webpj 진행 상황과 인수인계 내용은 AGENTS.md에 따라 LEFT_JOBS.webpj.md에 적는다.
