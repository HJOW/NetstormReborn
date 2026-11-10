# LEFT_JOBS — cpppj 현행 상태와 남은 작업

**확정 제품 1291 / 11.01 · 인계 정리 2026-11-11(HJOW-Athlon) · 다음 단위 번호 1292 · AGENTS 11~13차 변경(`60226ea93`) 반영 · 전체 목표 미완료**

이 문서는 cpppj와 공통 사항의 **현재 상태·남은 작업·유효한 규칙**만 둔다. 1254~1288 단위별 경과와 만료된 지시·순서는 [제품1288 시점 보관본](LEFT_JOBS.old.md#archive-20261111-through-1288)에 원문 그대로 있고, 각 단위의 상세는 [5.1절](#done) 색인과 [8절](#refs)의 근거 문서에 있다. dotnetpj·webpj의 현황과 계획은 [LEFT_JOBS.dotnetpj.md](LEFT_JOBS.dotnetpj.md)·[LEFT_JOBS.webpj.md](LEFT_JOBS.webpj.md)가 기준이다. 원격 저장소에 push하지 않는다.

**읽는 순서:** [0. 한눈에 보기](#now) → [1. 알려진 버그(진행 중)](#bugs) → 필요할 때 나머지 절.

<a id="now"></a>

## 0. 한눈에 보기

### 0.1 남은 작업과 순서

사용자 순서 지시(2026-11-11)는 **M6-n → M7 → M8 → K → S → B → L**이었다. 그 뒤 AGENTS 커밋 `60226ea93`이 **11차(난이도)·12차(스커미시)를 새로 정하고 기타 언어를 13차로 옮겼으므로**, 차수 순서에 따라 **B(10차) → 카피라이트 → D(11차) → SK(12차) → L(13차 후속) → M8 최종 관문**으로 진행한다. M6·M7·K(6차)·E(7차)·S(9차)는 끝났고([5.1절](#done)), 지금은 **B(10차)** 차례다. 일본어 L 1차(1288)는 B 진행 중에 먼저 끝났다.

| 순서 | 작업 | 현재 상태 | 다음 행동 |
| --- | --- | --- | --- |
| **1** | **B — 10차 알려진 버그 NB01~NB34** ([1절](#bugs)) | **진행 중:** 34건 중 11건 판정(수정 6·이미 반영 1·현행 미재현 3·재현 의심 0 — NB20은 1289, NB18은 1290에서 수정), **미조사 23건** | 미조사 23건을 [1.2절](#bugs-next) 순서로(다음: NB03·NB10·NB13·NB14·NB21·NB23·NB25·NB26·NB33) |
| 추가 | ~~11.01 카피라이트 표기~~ (AGENTS `60226ea93` 확정 요구, [8절](#refs)) | **완료(1291):** 11.01 Patch Credits 영어·한국어·일본어 본문에 `Copyright 2026 HJOW`, 세 언어 실제 화면 확인 | [근거](docs/exe/cpp-product-1101-20261101.md) |
| **2** | **D — 11차 난이도 조절** ([1A절](#difficulty)) | 미착수. Options에 쉬움/보통/어려움(기본 보통), 적 AI 다리 건설·유닛 생성 주기와 쉬움의 채취 유닛 수 반감 | D1: AI의 다리 건설·유닛 생성 주기와 채취 유닛 수 결정 위치를 디컴파일 원문에서 찾는다 |
| **3** | **SK — 12차 스커미시 모드** ([1B절](#skirmish)) | 미착수. Campaign 메뉴의 Skirmish, 별도 설정 UI, 랜덤 섬·간헐천의 AI 대전. D의 AI별 난이도 구조에 의존 | D 완료 뒤. SK1: 원문 멀티플레이어 게임 시작·금지 유닛·동맹 경로와 cpppj 재사용 지점 조사 |
| **4** | **L — 13차 기타 언어 후속** ([2절](#lang)) | 일본어 UI 1차 완료(1288), 본문은 영어 대체. AGENTS는 "일본어 - 추후 추가 예정" | 본문 번역 범위·다른 언어 노출 여부를 사용자에게 확인한 뒤 진행 |
| **5** | **M8 최종 관문 재실행** ([3절](#final-gate)) | 1차 통과(1280), 1288 ZIP 전수 일치 | B·D·SK·L의 코드 변경이 끝난 뒤 출시 직전 1회 |
| — | 사용자 확인 대기 ([4절](#user-checks)) | MP 실행·IME·자연 완주 등 | AI는 실행하지 않는다. 새 제보가 오면 해당 건을 다시 연다 |
| — | 별도 후순위 | 미착수 | 1~13차 뒤 60/120프레임, CD 10.72 전체 복원·호환성(Windows 10 구동 해결 문서화 포함)의 계획을 정한다. DN/WB/Linux 구현은 각 인계 범위다 |

**진행 원칙:** 단위마다 이 문서를 갱신하고 로컬 커밋한다(커밋이 실패해도 작업은 계속). 사제 포획/희생·특정 적 공격·시나리오 승리 재현, 사용자 직접 확인으로 명시된 검사, MP 실행 검사는 건너뛴다. 단순 미션 진입/대기/생산/건설로 끝나는 비MP 검사는 수행한다. 제외된 검사만 남으면 다음 작업으로 간다([6절](#rules-verify)).

### 0.2 AGENTS 단계별 현황

구현 마감과 전체 플레이·물리 환경 검증 완료를 구별한다.

| 단계 | 상태 | 남은 것 |
| --- | --- | --- |
| 1차 10.78 복원 | 비MP 활성 범위 마감(UI 929·Edit 943·미션 956·수명/FILE 980) | 모든 시나리오 자연 완주·전체 화면/음향 동일성은 사용자 확인 |
| 2차 Windows 10/11 호환 | 구현 마감(전체화면 재실행·화면비·가장자리 스크롤·저장/덱 결함 981) | 다른 모니터/DPI·Windows 11/ARM 실기 |
| 3차 LAN·Outpost | **구현·정적 대조 마감:** M1~M5, M6-a~n(1279 정적 종결), M7 5건(1268~1273) | **MP 실행 검증 0**(사용자 확인). 로비 섬의 사제·거주 구역 스프라이트가 원본 스크린샷처럼 보이지 않는 문제 미확정 |
| 4차 로컬 MCP·와이드 | 구현 마감(935~937, 최대 1920×1080) | 새 구체적 누락이 나올 때만 다시 연다 |
| 5차 사용자 홈 저장·맵 목록 | 구현 마감(932~934), 읽기 전용 설치·실제 사용자 홈 확인(1284) | 실제 Program Files/UAC 설치는 사용자 확인 |
| 6차 한국어 | **완료**(K1~K5, 1270~1281) | 물리 IME/OS 클립보드, `.korean` 본문 전체 문장 교정(사람 검수) |
| 7차 오류/예외·상세 로그 | **완료**(E1~E3·M7.5, 1269) | 한계: 던진 위치의 스택 없음, ARM64 레지스터 덤프 미구현 |
| 8차 AA·FSR 1.0 | 구현 마감(939, 기본 OFF, 1440p/4K 자동 FSR·논리 1080p) | 실물 4K/활성 전체화면 출력·입력 역변환/성능 |
| 9차 시나리오 편집 | **완료**(S1~S4, 1277~1284) | 없음 |
| **10차 알려진 버그** | **진행 중**(1285~) | [1절](#bugs) |
| 11차 난이도 조절 | 미착수(`60226ea93`로 확정) | [1A절](#difficulty) |
| 12차 스커미시 모드 | 미착수(`60226ea93`로 확정) | [1B절](#skirmish) |
| 13차 기타 언어 | 일본어 UI 1차 완료(1288). 기타 언어가 11차→13차로 옮겨졌고 AGENTS에 "일본어 - 추후 추가 예정"으로 적혀 있다 | [2절](#lang) |

### 0.3 최신 기준값

| 항목 | 현행 값 |
| --- | --- |
| 확정 제품 | **1291 / 11.01**(NB20 수정 1289·NB18 방어 1290·카피라이트 1291). 직전 1288은 커밋 `0444b4b6c`. 사용자 커밋 `4ec722eaf`(NB11 판정 문서)·`60226ea93`(AGENTS 11~13차)는 코드와 무관하다. |
| 실행·플레이 | 무인자 GUI·메뉴·미션 플레이가 가능하다. 최근 확인: Tutorial4 생산/건설 요청/600프레임 대기, Edit→Scenario 편집·저장→Test Battle 진입·복귀, 메인 `Multiplayer`의 로비 화면 표시(접속은 미검증), 한국어·일본어 UI. |
| 원본 동일성 | 부품별 디컴파일/기계어·fixture 대조와 제한된 플레이 확인을 누적했다. 모든 시나리오 자연 완주·전체 화면/음향 동일성은 미확정이다. |
| 최근 배포 ZIP | 1288: 1433항목, `tools/verify_cpp_package.py` 전수 일치(누락/금지/exe 글꼴 시그니처 0), 푼 exe로 일본어 메뉴 확인. 1280: 최종 관문 1차(1430항목, 영어·한국어 실행, 외부 접속 재감사). |
| 전체 회귀 | 전체 34경로 진입은 제품 644(645에서 검증)가 마지막이다. 최신 전체 솔루션 검사 통과 기록은 없고 단위마다 관련 집중 검사만 했다(기존 실패는 [5.3절](#baselines)). |
| 마지막 디컴파일 | Client 양판본 2026-11-03, R 전용 2026-11-07, 모두 **HJOW-Athlon(192.168.0.94)**. [기록](docs/exe/cpp-decompilation-20261103.md). 이후 새 디컴파일·원문 변경은 없다. |
| 보호 파일 | AGENTS.md·originals·originalCD·fixture는 수정하지 않았다(마지막 해시 집계는 1262~1266의 2,808파일 불변). |

<a id="bugs"></a>

## 1. B — 10차 알려진 버그 (진행 중)

AGENTS의 34건은 [NB01~NB34 등록 목록](docs/exe/cpp-known-bugs-plan-20261109.md)에 원문 순서대로 있고, 행별 판정·근거·검증은 [판정 현황표](docs/exe/cpp-known-bugs-status-20261111.md)가 기준이다. 판정은 `수정 완료 / 현행에 이미 반영 / 해당 없음 / 미완료`로 적고 허용 검증 결과와 사용자 확인 잔여를 따로 적는다. 이미 해결된 듯한 건도 원문·활성 호출자 근거 없이 닫지 않는다.

| 하위 단계 | 의미 | 상태 |
| --- | --- | --- |
| B1 | 34행마다 원본 주소/근거·현행 활성 호출자·필요 수정·검증 범위를 연결 | 11/34 판정. 미조사 23건은 현황표에 조사 시작점만 있다 |
| B2 | 비MP 원인(수명/버퍼/입력/에디터/AI/전투/간헐천 등) 수정과 허용 집중 검증 | NB02·NB08·NB09·NB18·NB20·NB24 수정. 새 집중 대상 `netstorm_known_bug_tests` |
| B3 | MP/클라이언트 동기화 원인을 디컴파일 대조로 수정·컴파일·정적 대조(MP 실행 없음) | 미착수 |
| B4 | 34행 최종 판정, 관련 빌드·비MP 회귀·필요한 배포 정합성 | 미착수 |

### 1.1 판정 완료 11건

| 판정 | ID | 요점 |
| --- | --- | --- |
| 수정 완료 | NB02, NB08, NB09, NB18, NB20, NB24 | 엠보스 오프셋을 두 좌표 모두 범위 안일 때만 적용 / User Made Campaigns 목록의 3중 원인 교정 / 긴 길드 태그를 줄여 맞춤(MP 실행 제외) / 사제 선택 키(P/R)가 커서에 든 건설 조각을 먼저 취소(1289) / 내용물을 든 운반체가 사제를 줍는 요청·수신 거부(1290, 정적·단위 검증) / 80BYTE를 넘는 캠페인 제목을 줄여 표시 |
| 현행에 이미 반영 | NB07 | 기존 이름으로 새 맵을 만들면 `Unique Name Required` 안내 |
| 현행 미재현 | NB05, NB11, NB22 | 긴 채팅 입력 / 데모의 AI 워크샾 대피해 / P·R·F 조합 키. 사용자 확인 잔여는 [4절](#user-checks) |

<a id="bugs-next"></a>

### 1.2 남은 23건 — 권장 순서

각 건의 조사 시작점은 판정 현황표의 "미조사(원인 위치만 정리)" 표에 있다. 판정이 끝나면 현황표 위쪽 표로 옮긴다.

| 순서 | ID | 할 일 | 검증 범위 |
| --- | --- | --- | --- |
| 1 | ~~NB20~~ | **1289 완료**(수정 완료). 원문 10.78의 사제 선택 키 P/R(`004d9884..004d98e1`)은 취소 없이 선택만 했다 → `SelectPriestByKey`에서 조각을 들고 있으면 `00446e70` 취소를 먼저 실행 | 단위 `KnownBug_NB20_PriestKeyCancelsHeldBuildPiece`·Tutorial4 실제 P 키(`extracted/cpp-resume-1278/nb20fix2/`) |
| 2 | NB03, NB10, NB13, NB14, NB21, NB23, NB25, NB26, NB33 | 비MP 원인 조사·수정: 워크샾 생산 슬롯 표시, 싱글 미션의 잘못된 player side, Cid 부동소수점 반올림(edge suck), 싱글 편집기의 MP fort 로드, 컬렉터 선택 뒤 건설 메뉴 배치, 드문 섬 생성 종료, 썬 캐논 사거리 표시, 썬더 캐논 정면 표적, 에디터 아이스 타워 섬 삭제 | 단순 진입/생산/건설·편집 실행으로 확인 가능한 것만 실행 |
| 3 | NB27, NB32 | 공중 표적 변경 뒤 건설 중 종료, 워크샾/신전 인접 유닛을 공격하는 아이스 캐논의 종료 | 코드·정적 대조. 공중 3종·특정 공격 재현은 사용자 확인 |
| 4 | NB01, NB06 | 보충 간헐천 맵에서 AI 연결 매개변수 누락(약 1시간 뒤 종료), AI의 신전 반복 생성 | 비영 brain 경로. 긴 시간 실행이 필요하면 정적 대조를 우선 |
| 5 | ~~NB18~~(1290 완료), NB19 | (NB18 완료) 스톰 크리스털을 내려놓은 뒤 200SP 미지급 | 코드·정적 대조. 사제 포획이 얽힌 실제 시험은 사용자 확인 |
| 6 | NB12, NB15, NB16, NB17, NB30, NB31, NB34 (B3) | MP 전용: 입력 중 Battlemaster 시작, 클라이언트 애시드 바리케이드 회전·벌프 줍기/내리기, 호스트 종료 시 fort 유실, 해상도 재시작 뒤 존 로그인 잔류, 클라이언트 체력 표시 | 디컴파일 대조·컴파일만. MP 실행 제외 |
| 7 | NB04, NB28, NB29 | 시작/재생 간헐천 분배식(Even Geyser 순서, 연속 게임의 간헐천 과다, 시작 간헐천 공식) | **올바른 공식이 원문 자료에 없어 사용자 설계 결정이 필요하다.** 착수 전에 기준을 질문한다 |
| 8 | 34행 전체(B4) | 판정표 마감·관련 빌드·비MP 회귀 | 끝나면 [2절](#lang)·[3절](#final-gate)로 |

<a id="difficulty"></a>

## 1A. D — 11차 난이도 조절 (미착수)

**AGENTS 원문 요구:** 메인 메뉴 Options 안에 난이도 설정을 추가한다. 쉬움·보통·어려움 3가지, 기본값은 보통이며 보통은 현행과 동일하다.

| 난이도 | 적 인공지능 변화 |
| --- | --- |
| 보통(기본) | 현행과 동일. 기존 동작이 한 비트도 달라지면 안 된다 |
| 쉬움 | 다리 건설·유닛 생성 컨트롤 **주기를 4배로 늘린다**(느려짐). 스톰파워가 충분할 때 생산하는 자원채취 유닛 수를 **절반으로 줄인다(최소 1 보장)** |
| 어려움 | 다리 건설·유닛 생성 컨트롤 **주기를 2배 빠르게** 한다(절반) |

| 하위 단계 | 할 일 |
| --- | --- |
| D1 | 조사: AI brain의 다리 건설·유닛 생성 컨트롤 주기(타이머·프레임 간격) 상수와 채취 유닛(컬렉터) 생산 수 결정 위치를 디컴파일 원문과 cpppj `RawAi*`에서 찾아 주소·소스 위치·단위를 문서에 기록한다. "적 인공지능"의 범위(동맹 AI 포함 여부)는 원문 구분을 따라 정하고 문서화한다 |
| D2 | 설정: Options 화면 항목(원본 배치 모방), 저장 키(기본 보통, 키 없는 기존 설정 파일은 보통), 영어·한국어·일본어 문구. 저장 위치는 5차 사용자 홈 규칙을 따른다 |
| D3 | 적용: **난이도를 AI 단위 값으로 만든다**(12차가 AI마다 다른 난이도를 쓰므로). 전역 옵션은 캠페인·Test Battle의 기본값 출처일 뿐이다. 보통의 기존 값 불변을 단위 검사로 고정하고 쉬움/어려움 배율·최소 1 보장을 검사한다. LAN 호스트 규칙은 건드리지 않는다 |
| D4 | 검증: 단위 검사 + 옵션 화면 실행·저장/복원 + 단순 미션 진입·대기·생산·건설. 실제 교전·승리 재현은 건너뛰고 사용자 확인으로 남긴다 |

<a id="skirmish"></a>

## 1B. SK — 12차 스커미시 모드 (미착수)

**AGENTS 원문 요구:** 임의의 섬 배치(멀티플레이어 게임처럼) 환경에서 적 인공지능과 하는 게임. 사용자가 원하는 수의 적 AI와 동맹을 설정한다. 메인 메뉴 Campaign 메뉴 안에 Skirmish를 추가한다. Skirmish 선택 시 **별도 UI(자체 구현)**에서 ① 적 AI 수 ② 금지 유닛(선택 사항, 최대 5개, Outpost도 선택 가능) ③ AI별 난이도(옵션 난이도 대신 이 게임에 적용)와 동맹을 받는다. **시작**과 **취소** 버튼이 있고 취소는 Campaign 메뉴로 돌아간다. 시작은 로딩 뒤 바로 게임을 시작하며 섬 배치와 간헐천은 멀티플레이어 게임처럼 무작위다. (WB는 12차 제외, DN은 포함)

| 하위 단계 | 할 일 |
| --- | --- |
| SK1 | 조사: 원문 멀티플레이어 게임 시작 경로(무작위 섬·간헐천 배치, 금지 유닛 구조, 팀·동맹, AI 슬롯 최대 수)와 cpppj의 `RawMissionStart`·Test Battle·LAN 호스트 경로에서 **재사용할 부분**을 정한다 |
| SK2 | UI: Campaign 메뉴의 Skirmish 항목과 설정 화면(AI 수·금지 유닛 5칸·AI별 난이도/동맹·시작/취소), 영어·한국어·일본어 문구 |
| SK3 | 게임 생성: 무작위 섬·간헐천, AI N명, 동맹, 금지 유닛(건설 메뉴에서 제외), AI별 난이도(D 구조), 로딩 연출 |
| SK4 | 검증: 설정 화면·취소 복귀·시작→로딩→게임 진입→일정 시간 대기·생산·건설. 교전·승리 재현은 건너뛴다. 외부 접속을 만들지 않는다 |

<a id="lang"></a>

## 2. L — 기타 언어 후속 (AGENTS 13차)

**1288 완료 범위:** 일본어 UI 번역표 509키(`assets/lang/japanese/xlat.japanese`), 히라가나/가타카나 정적·한자 동적(최대 3,946자) 표시 부호화, Options→Language의 `日本語`, 외부 `fonts\NotoSansJP-*`, `.japanese`가 없을 때 영어 본문 대체, 11.01 크레딧 일본어, 배포 동봉. [근거](docs/exe/cpp-japanese-l1-l3-20261111.md)·[보고서](cpppj/japanese-l1-l3-20261111.json).

| 남은 일 | 내용 | 진행 조건 |
| --- | --- | --- |
| 일본어 본문 번역 | 미션/튜토리얼/캠페인 본문 43개, 도움말 128주제, Tell 창·`config.*` 본문, 맵/캠페인 이름, 유닛·건물 설명 문단이 영어로 표시된다 | 범위와 번역 방식(AI 번역 사용 여부·검수 주체)을 사용자에게 확인한 뒤 진행 |
| 다른 언어 | 원문에 독일어 자료(`d\*.german` 85개·xlat 774키)가 있으나 메뉴에 노출하지 않는다. 프랑스어·스페인어·포르투갈어는 이름만 예약되어 있고 자료가 없다 | 추가·노출 여부는 사용자 결정 |
| 글꼴 굵기 | Noto Sans JP 가변 글꼴의 기본 인스턴스가 Thin(wght 100)이라 화면 크기별 GDI 렌더링 굵기 확인이 더 필요하다 | 다음 L 단위에서 화면 확인 |
| 사용자 확인 | 일본어 IME 조합·OS 클립보드, 번역 품질, `NotoSansJP-License.txt` 저작권 줄 대조 | [4절](#user-checks) |

<a id="final-gate"></a>

## 3. M8 최종 관문 재실행 (출시 직전)

B·L이 코드를 바꾸므로 **두 작업의 코드 변경이 끝난 뒤 1회** 다시 실행한다. 작은 단위마다 전체 검사·ZIP을 반복하지 않는다. 코드를 고친 뒤에는 ZIP을 새로 만들어야 한다(1284에서 교정 전 ZIP의 exe로 실패한 사례가 있다).

1. Release 빌드([7절](#rules-ops) 명령) 뒤 `powershell -File cpppj\package-cpppj.ps1 -NoBuild <zip>`으로 ZIP을 만든다.
2. `python tools/verify_cpp_package.py <zip> --extract <빈 폴더> --json <보고서>`로 모든 항목을 SHA-256 대조한다(누락/금지/exe 글꼴 시그니처 0, BUILD_INFO 해시 일치).
3. 푼 배포본의 `Netstorm.exe`로 영어·한국어·일본어 메인 메뉴와 단순 비MP 진입을 확인한다(종료 코드 0, 오류 로그 없음, 설치 폴더 불변).
4. 외부 접속을 재감사한다: WinINet/WinHTTP/URLMon import 0, `master.`/`security.dat`/`Not Validated`/`PIRATE`/`bugs@` 0, checking 목록 0, 새 활성 호출자([외부 정책](docs/exe/cpp-external-services-20261101.md)).
5. 필요하면 전체 34경로 진입 회귀와 전체 솔루션 검사 기준선을 다시 집계한다(마지막 전체 회귀는 제품 644).

**이미 끝난 M8 항목:** 글꼴 exe 내장 금지·외부 `fonts\` 적재·D2Coding 라이선스 동봉 교정(1274), 최종 관문 1차(1280), 일본어 글꼴 동봉(1288), 읽기 전용 설치·실제 사용자 홈(1284). **알려진 한계:** 설치 경로가 약 200자를 넘으면 원문 256BYTE 경로 작성기 한계로 시작 직후 종료한다(원문 보존, 오류 로그가 원인을 지목).

<a id="user-checks"></a>

## 4. 사용자 확인 대기 (AI는 실행하지 않음)

| 영역 | 확인할 것 |
| --- | --- |
| 플레이 동일성 | 모든 시나리오 자연 완주, 사제 포획/희생·특정 적 공격·승리, 전체 화면/음향 동일성 |
| MP·LAN(3차) | 실제 LAN 접속·Join/Test Battle의 MP 경로·로그인 debugBurst·Outpost, 조기/원격 희생 안내 UI/패킷(M7.3), 메타 맵 커서 깊이 0 반례(M7.2, 나오면 `00447000` 효과 구현), 로비 섬 사제·거주 구역 스프라이트 표시 |
| 한국어(6차) | 물리 IME 조합/후보창·OS 클립보드 붙여넣기, `.korean` 본문 전체 문장 교정. 영문+한글 혼합 H2 제목의 줄바꿈 근본 원인은 미확정이라 좁은 창에 새 혼합 제목을 만들면 같은 잘림이 나올 수 있다 |
| 일본어(13차) | 물리 IME·OS 클립보드, 번역 품질, `NotoSansJP-License.txt` 저작권 줄 |
| 알려진 버그(10차) | NB05 존/MP 채팅의 긴 입력, NB09 존 오른쪽 클릭 메뉴, NB11 `Dissolved Alliance` 데모·더 긴 시간·AI 워크샾 대파괴, NB22 동시 눌림(chord)·다른 화면, 사제 포획이 얽힌 NB18/NB19 |
| 설치·환경 | 실제 Program Files/UAC 설치, 다른 모니터/DPI·앱 전환·청취·활성 전체화면 물리 출력, 실물 4K FSR, 실제 CD, Windows 11/ARM 실기 |

<a id="done"></a>

## 5. 완료 범위와 유지할 계약

### 5.1 완료 묶음 색인 (다시 만들지 않는다)

| 묶음 | 단위 | 근거 |
| --- | --- | --- |
| M6-a~k | ~1173 | [M6 감사](docs/exe/cpp-lan-remaining-audit-20261108.md). 감사 문서의 초기 "없음" 판정은 당시 조사이며 현행 완료 범위보다 우선하지 않는다 |
| M6-l Native 등록표·고정 수신 | 1258~1262 | [1262 전체표/연결 owner 감사](docs/exe/cpp-native-registry-owner-audit-20261110.md)·[보고서](cpppj/native-registry-owner-audit-20261110.json) |
| M6-m JoinTestBattle | 1263 | [근거](docs/exe/cpp-native-join-test-battle-20261110.md) |
| M6-n NoWorld·호출자·owner | 1264~1267, 1272, 1278, 1279 | [1279 도달성 종결](docs/exe/cpp-m6n-reachability-closure-20261111.md)·[1278 Fort 섬 채우기](docs/exe/cpp-fort-island-populate-20261111.md)·[1272 Fort 호출자/수명](docs/exe/cpp-m6n-fort-callers-noworld-audit-20261111.md)·[1267 자산 후처리](docs/exe/cpp-asset-postpop-noworld-20261111.md)·[1266 Fort Snapshot](docs/exe/cpp-native-fort-snapshot-20261110.md)·[1265 debugBurst](docs/exe/cpp-login-debug-burst-20261110.md)·[1264 최초 후처리](docs/exe/cpp-initial-postpop-owner-20261110.md) |
| M7.1 helpWord 캐시 | 1268, 1275(F1 회귀 교정) | [1268](docs/exe/cpp-help-word-cache-20261111.md)·[1275](docs/exe/cpp-help-regression-korean-screens-20261111.md) |
| M7.2 메타 맵 커서 | 1273(정적 판정) | [근거](docs/exe/cpp-meta-cursor-reachability-20261111.md) |
| M7.3·M7.4 희생 안내·이동 marker | 1271 | [근거](docs/exe/cpp-early-sacrifice-marker-20261111.md) |
| M7.5·E1~E3 오류/예외 로그 | 1269 | [근거](docs/exe/cpp-error-log-20261111.md) |
| M8 1차 | 1274, 1280 | [글꼴·ZIP 선행 점검](docs/exe/cpp-m8-font-zip-precheck-20261111.md)·[최종 관문 1차](docs/exe/cpp-m8-final-gate-20261111.md) |
| K1~K5 한국어 | 1270, 1276, 1281 | [K1 병합](docs/exe/cpp-korean-k1-merge-20261111.md)·[화면·입력 검증](docs/exe/cpp-korean-screens-input-verification-20261111.md)·[K4/K5 마감](docs/exe/cpp-korean-k4-k5-closure-20261111.md) |
| S1~S4 시나리오 편집 | 1277, 1282~1284 | [S1](docs/exe/cpp-scenario-s1-merge-20261111.md)·[S2](docs/exe/cpp-scenario-s2-test-battle-20261111.md)·[S3](docs/exe/cpp-scenario-s3-ui-20261111.md)·[S4](docs/exe/cpp-scenario-s4-readonly-home-20261111.md) |
| B 착수·수정 | 1285~1290 | [판정 현황표](docs/exe/cpp-known-bugs-status-20261111.md)·[NB08/NB24 보고서](cpppj/known-bugs-nb08-nb24-20261111.json) |
| L 일본어 1차 | 1288 | [근거](docs/exe/cpp-japanese-l1-l3-20261111.md) |

[986 준비본](docs/exe/handoffs/cpp-pause-20261106/README.md)의 한국어·시나리오 후보는 K1(1270)·S1(1277)에서 필요한 부분을 병합했고 S2~S4로 기능 검증을 마쳤다. 원격 Fort 준비분은 987에서 구현했다. 남은 후보 파일은 보관 자료로만 쓰며 현행 소스에 그대로 덮어쓰지 않는다.

### 5.2 LAN·M6 코드가 유지할 계약

| 범위 | 유지 조건 |
| --- | --- |
| 장기 저장소 | `ClientConstructionGlobals`의 SP/HP/Canon/Chunks·최근/전체 목록·현재 Player/Bounds를 공유한다. Native Session은 같은 본체·캐시·영향 목록을 빌린다. 임시 World/Player나 TYPE·난수 사본을 만들지 않는다. |
| 자산·Form 삭제 | 자산 Pre/Post·통지 각 83/83VT, 장기 Regular 87/87VT, 기본 Form(Process·Depend·Gump·Content) 삭제 4/4를 연결했다. 큰 `449b30`과 파생 통지는 실제 Runtime을 pin한다. 정확한 잔여 행은 [1249 보고서](cpppj/vortex-regular-owner-20261110.json)의 `staticAudit.remainingRows`를 따른다. |
| NoWorld | Session 없이 Pop되는 객체는 메타 TYPE 108/109/161(VT `0050c9f0`/`0050cc08`)뿐이다(1272). 나머지 Session 소유 VT 행(Crab·Teleport·Rune·Altar·Dais·Aviary·Flyer·Gun 4·Blocker·Player·Priest·Residence·Temple)과 큰 AI·일반 Descend·배치 호출자·Fort 상위 Finish는 월드 안에서만 도달하므로 명시 오류를 유지한다(1279). 반례가 나오면 그 경로만 장기 소유자로 옮긴다. |
| 실제 현재값 | 선택된 효과 위치에서 현재 TYPE/역할·Player·게임 시각·단일 게임 난수를 읽는다. 거주 구역 Fort 생성은 실제 Dude count 대 Provider count를 먼저 비교한다. Flare 억제 `005c85f0`/로딩 `005c89b8`/효과 TYPE `00541200`은 같은 장기 전역을 쓰며 Session 생성/반납이 초기화하지 않는다. |
| 호출·해제 수명 | weak 대여 → 실제 Runtime pin → 함수 사본 → 호출을 유지한다. Client 장치는 첫 메뉴/Session 전에 결합한다. body를 State/backing보다 먼저 반납하고 실제 Menu/Session/Host 정리 뒤 장치를 끊는다. 장기 상태가 Session 소유 기록을 빌리면 Session 반납 때 함께 놓는다(1272 사제 Player9 span 사례). 강한 Session 역참조·빈 callback·가짜 ACK·성공 상수를 넣지 않는다. |
| LAN·R | M1~M5·M6·Pop/저장 복구/78h/기록·기존 checking 본체를 재작성하지 않는다. 새 R/TCP·UDP/등록표/시계/Config/주소/기록 owner를 유지한다. R10.71 3BYTE 존/Client10.78 47BYTE 존은 1093 compact 호환 범위다. 원본 R 실행/복사로 대체하지 않는다. |
| 외부 기능 | 요청 DWORD `0054dbac`은 reset 0/1ah·1bh read 근거뿐이며 새 writer/CLI를 발명하지 않는다. 폐기 CD 7f/80·온라인 유효성 검증·PIRATE quota를 복구하지 않는다. IPX/SPX는 명시 미지원, hostname OS 조회/첫 LAN IPv4·숫자 TCP/UDP를 유지하며 원문에 없는 관리 포트를 추가하지 않는다. |
| 주석 함정 | 같은 줄 `//` 주석이 뒤 코드를 삼킨 결함이 1275(Help 토큰 분기)·1278(`hooks.setOwner`)·1281에서 반복됐다. 코드 줄 중간에 `//` 주석을 넣지 않는다. |

<a id="baselines"></a>

### 5.3 집중 검사의 알려진 기존 실패

변경 전 기준선과 같은 실패는 새 결함으로 세지 않는다. 비교 방법은 기준선 워크트리 또는 자기 파일만 stash한 빌드다.

| 검사 대상 | 기존 실패(기준선 동일 확인 단위) |
| --- | --- |
| `netstorm_pop_context_tests` | 25건(1267) |
| `netstorm_guide_fort_request_tests` | 4건(1278) |
| `netstorm_file_directory_tests` | 3건(1285) |
| `netstorm_menu_session_tests`·`netstorm_main_popup_tests`·`netstorm_file_tests` | 기준선과 동일(1277). menu_session 6건 실패는 1268에서 직접 비교 없이 helpWord와 무관한 메시지로 판단했다 |
| 과거 938 집중 실패·이월 건수 | [보관본의 5.2절](LEFT_JOBS.old.md#archive-20261109-through-1206) |

<a id="rules-verify"></a>

## 6. 검증 제외와 미해결 한계

### 6.1 계속 유효한 사용자 결정

| 구분 | 적용할 규칙 |
| --- | --- |
| 사용자 직접 확인 | 전체 동일성·모든 시나리오 자연 완주, 사제 포획/희생·특정 적 공격·시나리오 승리를 재현하는 실제 플레이 검사는 건너뛴다. 이들만 남으면 다음 단계로 가며 새 제보는 해당 건을 다시 연다. |
| MP | 디컴파일 참고 구현·컴파일·정적 대조는 수행한다. **새 MP 단위/패킷/게임/통신/기록파일 실행 검사는 하지 않는다.** 실행 제외가 구현 생략이나 동작 검증 완료를 뜻하지 않는다. |
| 공중 3종 | 훨리기그·맨오워·더스트 데빌 전용 생성/생산/행동 관측은 실행하지 않는다. 정적 분석/코드는 가능하다. |
| 보류 분석 | 스펠 효과 도움말 그림 추가 분석(530), 연녹색 직사각형 추가 분석. |
| 환경 | 물리 입력/캡처는 762의 새 부팅/RDP에서도 2회 timeout이었다. 환경 변화 전 재시도하지 않는다. 다른 모니터/DPI·앱 전환·청취·활성 전체화면 물리 출력·실제 CD·Windows 11/ARM 실기는 별도 확인이다. |
| 허용 검증 | 변경과 직접 관련된 **단순 미션 진입/대기/생산/건설로 끝나는 비MP 검사**는 수행한다. 원본/복사본/클론·analyzeManager·별도 창은 AGENTS의 호스트 예외/명시 허용 범위를 따르며 현재 HJOW-Athlon은 예외 호스트다. |

### 6.2 완료로 바꾸지 않을 기존 한계

- **전체 검사 기준:** 최신 전체 솔루션 통과 기록은 없다. 과거 실패 수를 최신 재집계로 표시하지 않으며, 관련 변경이나 최종 관문에서 실제로 다시 검증한다.
- **일반 Battle/자연 완주:** `CampaignSaveGoMain_ActualManagerGateAndSelectedBattleStop`은 일반 Battle/MP 이월로 새 MP 검사를 제외한다. `CampaignForwardBattle_NaturalTempleAssault` 549 실패는 사용자 자연 완주/별도 지원 범위다. 전략/승리 단언을 약화하지 않고, TEST02 로컬 AI/SP·자동 공략 지원은 별도 요청 전 하지 않는다. `owner!=myPlayerId || demo!=0`를 `gate.demo=1`로 우회하지 않는다.
- **부호화:** 원문 8비트/설정·경로의 `EncodeOriginalText`와 화면용 `EncodeDisplayText`/`DecodeDisplayText`를 구별한다(한글·일본어 확장은 표시 전용). MakeAi owner0 이전/삭제·전략 차이, 설치 글꼴/다른 ACP·구형 무BOM 설정 자동 이관은 [글꼴 경계](docs/exe/cpp-font-face-encoding-20261103.md)를 따른다.
- **정밀도:** 32비트 FILE/FileSpec/포인터·SEH/스택·미작성값 자체 일치, 임의 외부 heap→SID, 실제 OS 고갈/물리 디스크 장애·UNC/드라이브 CWD·CD 전체 dump/trace/Retype·x87 전체 입력/픽셀은 호스트/후순위 경계다. 부분 I/O/버퍼 실패/복구 966·new_handler/종료 979를 미착수로 되돌리지 않는다. 활성 호출자 미연결을 이 경계로 숨기지 않는다.
- **음향/그림:** PCM 936은 HJOW-Athlon 프로세스 WASAPI 101653프레임/비무음 99726과 영상/소리 합성을 확인했다. 청취/다른 장치/실제 CD/활성 전체화면까지 검증한 것은 아니다. 덱 검은 띠 981과 사용자 보류 연녹색 사각형을 혼동하지 않는다.
- **설치 경로 길이:** 약 200자를 넘는 설치 경로는 원문 256BYTE 경로 작성기 한계로 시작 직후 종료한다(원문 보존).

<a id="rules-ops"></a>

## 7. 착수·검증·커밋 운영 규칙

1. **착수:** AGENTS.md·이 문서·LEFT_JOBS.dotnetpj.md를 다시 읽고 git status/log로 다른 세션 변경을 확인한다. 현재 호스트가 0.3의 마지막 디컴파일 호스트와 다르면 CPP 작업/원문 분석 전에 `tools/ghidra/refine_all.ps1`을 다시 실행한다. R 원문은 필요 시 `tools/ghidra/refine_root_server.ps1`도 실행하고 기록에 호스트를 갱신한다. HJOW-X3D의 2026-10-27 중단 산출물은 완성본으로 쓰지 않는다.
2. **단위 종료:** 머리말의 확정 제품·다음 단위 번호, 0.1~0.3의 값, 해당 절(1~3절)의 표를 갱신하고 완료 단위를 5.1 색인에 한 줄로 더한다. 긴 경과는 단위 문서/보고서에 두고 이 문서에 단락으로 쌓지 않는다. 준비본/정적 대조/컴파일/단위 실행/GUI/실물 확인을 구별해 적는다.
3. **커밋:** 인계 작성 뒤 **자기 파일만** `YYYYMMDD-NN CP 요약`으로 로컬 커밋한다. 당일 번호와 다른 세션의 stage를 확인하고 DN/WB 파일·stage를 수정/정리/커밋하지 않는다(1266 때 동시 webpj 세션이 공유 인덱스를 커밋하며 CP 파일이 WB 커밋 `93d891ac8`에 섞인 사례가 있다). 실패해도 작업을 계속하며 **push하지 않는다**. 저장소/전역 설정과 최초 TSV의 `-text`를 바꾸지 않는다.
4. **빌드:** HJOW-Athlon은 VS18(2026)/MSBuild 18.x, HJOW-X3D는 VS17(2022)/v143이다. 아래 **4병렬** 명령을 PowerShell에서 사용한다. 단일 실행 핸들의 실제 종료 0을 확인하며 **빌드 중 제품 소스를 변경하지 않는다**. [빌드 안내](docs/cpp-build.md).
5. **검증:** 최초 실패/교정 원시와 기대값을 보존한다. 기대값을 제품에 맞춰 약화하지 않는다. 관련 집중 확인 뒤 다음 작업으로 가며, 전체 34경로/CTest/ZIP 반복은 해당 관문에서만 한다. [6절](#rules-verify)의 실행 제외를 적용한다.
6. **보호/실행:** AGENTS.md·원본/CD 기존 파일·루트 README.md의 **개요**를 수정하지 않는다. 원본 실행 전후 해시/허용 설정 복원, 시작한 프로세스만 종료, CHEAT/검사 옵션과 기본 AI/프로필 분리를 유지한다.
7. **제품:** Windows `Netstorm.exe`, 대상 NetstormCpp/OUTPUT_NAME Netstorm/ARCHIVE_OUTPUT_NAME NetstormCpp, 버전 11.01·세 번째 Patch Credits(HJOW·AI), 무인자 GUI/--help/--run/분석 CLI를 유지한다. 전체 34경로(Early6·Training9·캠페인16·데모3), EXE 옆 `game/netstorm.tarc` 우선을 유지하고 cwd의 다른 게임/보호 원본을 자동 탐색하지 않는다. 배포 스크립트의 무인자 경로 입력·빈 값 취소·필수 에셋/글꼴/라이선스 동봉 계약도 유지한다.
8. **외부 접속:** 로컬 TCP/IP 외 외부 도메인/IP 기능은 [외부 정책](docs/exe/cpp-external-services-20261101.md)·[주소 목록](cpppj/external-endpoints-20261101.json)에 문서화하고 폐기 온라인 유효성 검증은 비활성화한다. 새 활성 호출자도 감사한다.
9. **언어/인코딩:** 새 문서/코드는 UTF-8, 응답·새 코드의 네임스페이스/상수/함수/클래스/구조체/멤버/반복문 설명은 한국어다. AGENTS의 D2Coding·번역 명칭을 따르고 글꼴은 exe에 내장하지 않고 exe 옆 `fonts\`에서 적재한다. 이 문서는 CRLF로 저장한다. 원시 로그/fixture 인코딩과 ClientMain.cpp 등 `-text` 소스의 혼합 줄바꿈은 바이트 국소 수정으로 보존하고 Python read_text/write_text 전면 재작성은 피한다. PowerShell 5.1 stdin은 `$OutputEncoding = [System.Text.UTF8Encoding]::new($false)`, 필요 시 Python UTF-8 모드를 쓴다.
10. **현행 기준:** 옛 로드맵·단위 보고서의 "다음"보다 이 문서의 현재 계획을 우선한다.

```powershell
cmake --build cpppj/build --config Release --target NetstormCpp NetstormRoot --parallel 4 -- /p:UseMultiToolTask=true /p:MultiProcMaxCount=4 /p:EnforceProcessCountAcrossBuilds=true
```

<a id="refs"></a>

## 8. 근거·보관·AGENTS 반영 기준

| 범위 | 기준 자료 |
| --- | --- |
| 이력 보관 | [제품1288 시점 원문](LEFT_JOBS.old.md#archive-20261111-through-1288)(1254~1288 단위 경과·만료된 순서/지시·완료된 M/K/E/S 상세), [제품1240 시점](LEFT_JOBS.old.md#archive-20261110-through-1240), [제품1206 시점](LEFT_JOBS.old.md#archive-20261109-through-1206) 등. 기존 보관본은 수정하지 않는다. |
| 1254~1261 정적 감사·고정 수신 | [1254 직접 소유 경계](docs/exe/cpp-direct-runtime-owner-audit-20261110.md)·[1256 줄바꿈](docs/exe/cpp-client-main-line-endings-20261110.md)·[1258 건설](docs/exe/cpp-native-construction-completion-20261110.md)·[1259 Carrier](docs/exe/cpp-native-carrier-fixed-receivers-20261110.md)·[1260 gameplay](docs/exe/cpp-native-gameplay-fixed-receivers-20261110.md)·[1261 Login](docs/exe/cpp-native-login-fixed-receivers-20261110.md) |
| M6 전체 감사·최초 조사 | [M6 감사](docs/exe/cpp-lan-remaining-audit-20261108.md)·[Regular 1217](docs/exe/cpp-no-world-regular-audit-20261109.md)·[Bomb 1224](docs/exe/cpp-bomb-regular-audit-20261109.md)·[16VT/19TYPE 행렬](cpppj/bomb-regular-audit-20261109.json)·[1246 연결](docs/exe/cpp-arc-acid-regular-owner-20261110.md)·[1247 피해율 교정](docs/exe/cpp-type-damage-rate-correction-20261110.md) |
| 비MP 마감 근거 | [UI 929](docs/exe/cpp-ui-implementation-closure-20261105.md)·[Edit 943](docs/exe/cpp-editor-implementation-closure-20261106.md)·[미션 956](docs/exe/cpp-mission-state-scope-20261106.md)·[수명 980](docs/exe/cpp-internal-lifetime-closure-20261106.md) |
| 디컴파일·소스 | [마지막 디컴파일](docs/exe/cpp-decompilation-20261103.md)·[신뢰도](docs/exe/decompile-reliability.md)·[SOURCE_MAP](cpppj/SOURCE_MAP.md)·[모듈](docs/exe/modules.md) |
| 제품·서비스 계약 | [11.01](docs/exe/cpp-product-1101-20261101.md)·[외부 서비스](docs/exe/cpp-external-services-20261101.md)·[TODO](TODO.md)·[미션 대응](docs/gameplay/demo-campaign-map.md) |
| 타 프로젝트 요청 처리 | [webpj 요청(제품 1227 포함 완료)](docs/exe/cpp-webpj-requests-20261109.md)·[session-dump/고정 시각](docs/exe/cpp-session-state-dump-20261109.md). DN의 DEMOVILLE/Help Slider 옛 값 재반영 요청은 유지하며 DN 자체 변경은 CP 작업에서 하지 않는다. |

**AGENTS.md 반영 기준(재확인 2026-11-11):** AGENTS.md는 수정하지 않는다. 아래 사용자 변경을 현재 계획에 반영했다.

| 사용자 변경 | 반영 상태 |
| --- | --- |
| `75e4eed9e` 글꼴 외부 배포 | 한국어 D2Coding·일본어 Noto Sans JP를 exe 옆 `fonts\`에서 적재하고 ZIP에 라이선스와 함께 동봉(1274·1280·1288 확인). 최종 관문에서 다시 확인 |
| `8661e1060`·`8136df49e`·`b5117f524` 7차 오류 로그·기타 언어(당시 11차) | 7차 완료(1269), 기타 언어 일본어 1차(1288) |
| `b5197d48` 10차 알려진 버그 34건 | [1절](#bugs) 진행 중 |
| `551d27ae` 세 프로젝트 평가 기준 | 진행 요약 요청 시 cpppj·dotnetpj·webpj를 각각 실행·플레이 가능 여부와 원본 동일성으로 평가한다. DN/WB 값은 각 인계를 따른다 |
| `3051892a` 루트 README 개요 보호 | README.md 개요 수정 금지, README.en.md Overview는 번역으로 갱신 가능 |
| **`60226ea93` 목표 일부 수정(2026-11-11 커밋)** | **11차 난이도 조절(쉬움/보통/어려움)·12차 스커미시 모드가 확정**되고 기타 언어는 **13차**로 이동("일본어 - 추후 추가 예정"), **11.01 카피라이트는 `Copyright 2026 HJOW`로 표기**. 11차는 [1A절](#difficulty), 12차는 [1B절](#skirmish), 13차는 [2절](#lang), 카피라이트는 0.1의 "추가" 작업으로 계획에 넣었다 |
| `22a0877c3` + `60226ea93` DN/WB 후순위 범위 | DN은 CPP **4~13차**와 Linux `$HOME/.netstorm` 경로, WB는 CPP **6~13차**(7차는 브라우저 console, **8·9·12차 제외**)를 후순위 요구로 가진다. 구현·진행률은 각 인계가 기준이며 CP 작업에서 DN/WB 인계 파일은 수정하지 않는다(11·12·13차 반영은 각 세션이 한다) |
