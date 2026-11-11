# LEFT_JOBS — cpppj 현행 상태와 남은 작업

**확정 제품 1301 / 11.01 · 정리 2026-11-11(HJOW-Athlon) · 다음 단위 번호 1302 · 마지막 디컴파일 HJOW-Athlon(변경 없음, 재디컴파일 없음) · 전체 목표 미완료 — 남은 일은 [0.1](#now), 사용자 결정은 [0.2](#decisions), 사용자 확인은 [0.3](#user-checks)**

이 문서는 cpppj와 공통 사항의 **현재 상태·남은 작업·유효한 규칙**만 둔다. 끝난 작업 묶음은 [1절](#done)에 한두 줄로 요약하고 상세는 근거 문서에 있다. 이번 정리 직전의 이 문서 전체(제품 1298 시점, 완료 묶음의 상세 표·B 판정 상세·D/SK 하위 단계표)는 [LEFT_JOBS.old.md](LEFT_JOBS.old.md#archive-20261111-through-1298)에 바이트 그대로 보관했고, 1254~1288 경과는 [제품1288 시점 보관본](LEFT_JOBS.old.md#archive-20261111-through-1288)에 있다. dotnetpj·webpj의 현황과 계획은 [LEFT_JOBS.dotnetpj.md](LEFT_JOBS.dotnetpj.md)·[LEFT_JOBS.webpj.md](LEFT_JOBS.webpj.md)가 기준이다. 원격 저장소에 push하지 않는다.

**읽는 순서:** 0.1(남은 작업) → 0.2(결정할 것) → 0.3(확인할 것) → 필요할 때 나머지 절.

<a id="now"></a>

## 0. 한눈에 보기

### 0.1 남은 작업 — AI가 진행할 수 있는 것

AGENTS 1~14차 구현은 모두 끝났다([0.4](#stages)). 14차(사용자 커밋 `85551c6c2`)는 **1301에서 QuickJS-ng 엔진 탑재**(D7 ① 범위)로 끝냈고 연동 범위는 결정 D7을 기다린다. 남은 AI 작업은 아래 셋이다.

| 순서 | 작업 | 현재 상태 | 시작 조건 / 다음 행동 |
| --- | --- | --- | --- |
| **1** | **CD 10.72 복원 C3~C5** (실제 복원·CD 확인 설정 대체·CD 자료 실행 검증) | C1(Windows 10 구동 원인·해결 방안)·C2(판본 게이트 목록) **문서화만 완료**: [근거](docs/exe/cpp-cd1072-startup-and-gates-20261111.md). 일반 클라이언트(메뉴·미션·월드)는 `Patch1078` 전용이며 게이트는 **241개 파일 306곳**, `Netstorm.exe --run <CD 자료> --cd`는 `Unknown --run option: --cd`로 거부된다. AGENTS는 10.72 복원을 "차후 작업"으로 두고 지금은 문서화를 요구한다 | **착수 가능.** 복원 범위·설치 CD 확인 정책은 결정 **D1**이 정한다. 결정이 없으면 문서의 복원 순서 ①자산/타입/폰트 경로 → ②시작/메뉴 → ③튜토리얼 진입부터 최소 범위로 시작하고 CD 확인은 설정(`CDDir`)으로 대체한다 |
| **2** | **M8 최종 관문 재실행** ([3절](#final-gate)) | 마지막 통과는 제품 1297 기준이다. 이후 코드가 바뀌었다(1298 프레임 대기 방식·1299 CD 문서·1300 스커미시 MP 규칙 월드와 NB23 교정·**1301 QuickJS 엔진 정적 링크와 배포 `third-party\quickjs-ng\` 추가**) | **코드 변경이 모두 끝난 뒤 1회.** 14차 탑재(1301)는 끝났으므로 CD 10.72 C3~C5를 하지 않으면 바로 해도 된다. 작은 단위마다 반복하지 않는다 |
| 3 | 비행체 표시 보간(F3, 선택) | 미착수. 지상 이동은 게임 시계 기반이라 상한만 올려도 갱신이 더 잦아지지만, 비행체는 원문이 0.02초(50Hz) 간격으로 갱신을 예약해 50FPS를 넘으면 부드러워지지 않는다 | 결정 **D2**가 "한다"일 때만 |
| — | 새 제보 처리 | 재현 조건·영상·로그가 오면 해당 NB 행([1.1](#bugs))을 다시 연다 | 제보 시 |

**진행 원칙:** 단위마다 이 문서를 갱신하고 로컬 커밋한다(커밋이 실패해도 작업은 계속). 사제 포획/희생·특정 적 공격·시나리오 승리 재현, 사용자 직접 확인으로 명시된 검사, MP 실행 검사는 건너뛴다. 단순 미션 진입/대기/생산/건설로 끝나는 비MP 검사는 수행한다([5절](#rules-verify)).

<a id="decisions"></a>

### 0.2 사용자 결정이 필요한 것

AI가 근거 자료만으로는 정할 수 없는 항목이다. **권고**대로면 "그대로 진행"이라고만 답하면 된다.

| ID | 결정할 내용 | 배경 | AI 권고 | 결정되면 |
| --- | --- | --- | --- | --- |
| **D1** | **CD 10.72 복원의 범위와 설치 CD 확인 정책** | 10.72 전체 복원은 10.78 복원에 맞먹는 규모다. 10.72는 Windows 10에서 아예 구동되지 않으며 원인 후보(전체화면 DirectDraw·CD 확인·구형 3DCARD)는 [문서](docs/exe/cpp-cd1072-startup-and-gates-20261111.md) §2. 사용자 환경의 **실제 구동 실패 문구/로그**가 있으면 원인을 좁힐 수 있다 | **메인 메뉴 + 튜토리얼 진입**까지(C3 ①~③, C5)만 먼저. CD 확인은 설정(`CDDir` + 표시 파일 존재만 확인), CD 삽입 대기는 하지 않음, 온라인 검증은 복구하지 않음 | 범위대로 `Patch1078` 게이트를 판본별 분기로 바꾸고 `--run <CD 자료> --cd`를 일반 클라이언트에 연다 |
| **D2** | **60/120프레임을 상한 옵션까지만 할지, 비행체 표시 보간까지 할지** | 옵션 `Frame Rate`(60/75 원본/120)는 완료(1297·1298). 비행체만 50Hz 갱신이라 보간이 있어야 부드럽다 | **지금은 상한 옵션까지만.** 120Hz에서 비행체가 끊겨 보이면 알려 달라 | "보간까지"면 표시 전용 보간(논리·MP 동기 불변)을 만든다 |
| **D3** | **일본어 본문 번역(13차)의 범위·방식, 다른 언어 노출** | AGENTS는 "일본어 — 추후 추가 예정"이다. 미션/튜토리얼/캠페인 본문 43개·도움말 128주제·Tell 창 본문·맵/캠페인 이름·유닛·건물 설명은 영어로 표시된다. 원문에 독일어 자료(`d\*.german` 85개·xlat 774키)가 있으나 메뉴에 노출하지 않는다 | **지금은 하지 않음**(AGENTS가 후속으로 정함). 시작하면 범위(전체/일부)·AI 번역 사용 여부·검수 주체를 정할 것 | 정한 범위로 `.japanese` 본문을 만든다 |
| **D4** | **알려진 버그 12건의 재현 조건과 올바른 값** (NB28 + 분석 후 미확정 11건) | 원문 코드 근거는 정리했으나 재현 조건/고쳐진 동작 자료가 원문에 없다. 건별 필요한 정보는 [1.1](#bugs)의 표 | 아는 건만 알려 주면 된다. 모르면 그대로 둔다 | 받은 조건으로 해당 행을 다시 열고 수정·검증 |
| D5 | NB04·NB29의 공식(제품 쪽 설계 기준) | Even 구역 순서는 시간 시드로 섞고, 간헐천 예산은 6명 이후 인당 6개로 이어 붙였다(원문 공식은 6명에서 최대인 비단조 오류) | **유지** | 다른 목표 공식이 있으면 `WorldGeyserBudget`(`o/RawWorldGeyserBounds.cpp`)·`RawWorldGeyser` 조정 |
| D6 | 스커미시 후속(선택) | 시작 섬 **모양**은 원본 fort에서 고른 후보 22개 중 무작위이고(위치·중앙섬·군도·간헐천은 MP 규칙), 사람은 항상 남쪽(소유자 1)이다([근거](docs/exe/cpp-skirmish-sk-20261111.md) §3) | **유지** | 후보를 넓히거나 사람 시작 위치를 무작위로 바꾼다 |
| **D7** | **QuickJS(14차)를 어디까지 연동할지** | AGENTS가 "어느 정도까지 연동할지는 추후 고민"으로 남겼다. 후보: ① 탑재만(엔진 생성·자체 검사) ② 개발용 진단(MCP·CLI에서 JS 평가) ③ 게임 상태 읽기·명령을 노출하는 스크립트/모드 ④ 미션 대본·Tell 명령 확장. **①은 1301에서 완료**([근거](docs/exe/cpp-quickjs-engine-20261111.md)) | **지금은 ①만.** ② 이상으로 넓힐 때 노출 API, 파일·네트워크 차단, 스크립트 위치(사용자 홈 아래)를 함께 정한다 | 정한 범위로 바인딩·검사를 만들고 dotnetpj에도 같은 계약을 넘긴다 |

<a id="user-checks"></a>

### 0.3 사용자 확인 대기 (AI는 실행하지 않음)

| 영역 | 확인할 것 |
| --- | --- |
| 플레이 동일성 | 모든 시나리오 자연 완주, 사제 포획/희생·특정 적 공격·승리, 전체 화면/음향 동일성 |
| MP·LAN(3차) | 실제 LAN 접속·Join/Test Battle의 MP 경로·로그인 debugBurst·Outpost, 조기/원격 희생 안내 UI/패킷(M7.3), 메타 맵 커서 깊이 0 반례(M7.2, 나오면 `00447000` 효과 구현), 로비 섬 사제·거주 구역 스프라이트 표시 |
| 한국어(6차) | 물리 IME 조합/후보창·OS 클립보드 붙여넣기, `.korean` 본문 전체 문장 교정. 영문+한글 혼합 H2 제목의 줄바꿈 근본 원인은 미확정이라 좁은 창에 새 혼합 제목을 만들면 같은 잘림이 나올 수 있다 |
| 일본어(13차) | 물리 IME·OS 클립보드, 번역 품질, `NotoSansJP-License.txt` 저작권 줄, 화면 크기별 글꼴 굵기(Noto Sans JP 가변 글꼴의 기본 인스턴스가 Thin wght 100) |
| 알려진 버그(10차) | NB05 존/MP 채팅의 긴 입력, NB09 존 오른쪽 클릭 메뉴, NB10 실제 미션 종료 제보, NB11 `Dissolved Alliance` 데모·더 긴 시간·AI 워크샾 대파괴, NB22 동시 눌림(chord)·다른 화면, 사제 포획이 얽힌 NB18/NB19, NB03 다른 워크샾·여러 칸, 실제 대국 중 섬 생성 종료 재발(NB23 교정 후) |
| 난이도(11차) | 쉬움/어려움에서 적 AI가 실제로 느려지고 빨라지는 체감, 게임 중 난이도를 바꾼 뒤 다음 AI 생성의 반영 |
| 스커미시(12차) | 금지 유닛이 AI 건설에도 적용되는지, 동맹 AI와 적 AI의 실제 전투·승패, 한국어·일본어 화면의 유닛 이름, 실제 대국에서 섬 배치가 MP와 같다고 느껴지는지 |
| 프레임(F1/F2) | 60/120FPS에서 소리 동기(청취 필요), 실물 4K FSR 조합 부하 |
| 설치·환경 | 실제 Program Files/UAC 설치, 다른 모니터/DPI·앱 전환·청취·활성 전체화면 물리 출력, 실물 4K FSR, 실제 CD, Windows 11/ARM 실기 |

<a id="stages"></a>

### 0.4 AGENTS 단계별 현황

구현 마감과 전체 플레이·물리 환경 검증 완료를 구별한다.

| 단계 | 상태 | 남은 것 |
| --- | --- | --- |
| 1차 10.78 복원 | 비MP 활성 범위 마감(UI 929·Edit 943·미션 956·수명/FILE 980) | 모든 시나리오 자연 완주·전체 화면/음향 동일성은 사용자 확인 |
| 2차 Windows 10/11 호환 | 구현 마감(전체화면 재실행·화면비·가장자리 스크롤·저장/덱 결함 981) | 다른 모니터/DPI·Windows 11/ARM 실기 |
| 3차 LAN·Outpost | **구현·정적 대조 마감:** M1~M5, M6-a~n(1279 정적 종결), M7 5건(1268~1273) | **MP 실행 검증 0**(사용자 확인) |
| 4차 로컬 MCP·와이드 | 구현 마감(935~937, 최대 1920×1080) | 새 구체적 누락이 나올 때만 다시 연다 |
| 5차 사용자 홈 저장·맵 목록 | 구현 마감(932~934), 읽기 전용 설치·실제 사용자 홈 확인(1284) | 실제 Program Files/UAC 설치는 사용자 확인 |
| 6차 한국어 | **완료**(K1~K5, 1270~1281) | 물리 IME/OS 클립보드, 본문 전체 문장 교정(사람 검수) |
| 7차 오류/예외·상세 로그 | **완료**(E1~E3·M7.5, 1269) | 한계: 던진 위치의 스택 없음, ARM64 레지스터 덤프 미구현 |
| 8차 AA·FSR 1.0 | 구현 마감(939, 기본 OFF, 1440p/4K 자동 FSR·논리 1080p) | 실물 4K/활성 전체화면 출력·성능 |
| 9차 시나리오 편집 | **완료**(S1~S4, 1277~1284) | 없음 |
| 10차 알려진 버그 | **판정 34/34 마감**(1285~1294, NB23 실제 교정 1300) | 사용자 결정 D4·D5, 확인 [0.3](#user-checks) |
| 11차 난이도 조절 | **완료**(1292) | 실제 전투 체감은 사용자 확인 |
| 12차 스커미시 | **완료**(UI·시작 1293, **MP 규칙 월드 1300**) | D6(선택), 금지 유닛·실제 전투는 사용자 확인 |
| 13차 기타 언어 | 일본어 UI 완료(1288), 11.01 Credits·Difficulty·Skirmish 일본어 포함 | D3(본문 번역·다른 언어) |
| 14차 QuickJS | **탑재 완료**(1301, D7 ① 범위): QuickJS-ng v0.18.0 정적 링크, 엔진 생성·해제와 시작 자체 검사 6항목, `--quickjs-selftest`, 자원 한도(16MiB·스택 256KiB·1초), 게임 상태·파일·네트워크 비노출. x64 빌드·실행, ARM64 교차 빌드 | 연동 범위 D7, ARM64 실기 실행 |
| 후순위 | 60/120프레임 F1·F2 완료(1297·1298), CD 10.72 C1·C2 문서화(1299) | [0.1](#now)의 CD 10.72 C3~C5, D2 |

### 0.5 최신 기준값

| 항목 | 현행 값 |
| --- | --- |
| 확정 제품 | **1301 / 11.01.** 최근: Frame Rate 옵션 1297·프레임 대기 방식 1298·CD 10.72 C1·C2 문서 1299·스커미시 MP 규칙 월드와 NB23 교정 1300·**14차 QuickJS-ng 엔진 탑재 1301**. 직전 정리 시점은 1298(커밋 `1f5b4ead1`까지) |
| 실행·플레이 | 무인자 GUI·메뉴·미션 플레이가 가능하다. 최근 확인: 스커미시 시작(AI 1·3·7명, 약 90초 동안 AI가 다리 건설), 일반 미션 `thewarbegins` 진입·대기, 메인 `Multiplayer` 로비 화면 표시(접속은 미검증), 한국어·일본어 UI, 1301 메인 메뉴 시작과 `--quickjs-selftest` PASS 6/6 |
| 원본 동일성 | 부품별 디컴파일/기계어·fixture 대조와 제한된 플레이 확인을 누적했다. 모든 시나리오 자연 완주·전체 화면/음향 동일성은 미확정이다 |
| 최근 배포 ZIP | M8 재실행(제품 1297 기준): 1433항목 전수 일치, 누락/금지/exe 글꼴 0. **1298 이후 코드가 바뀌어 새 ZIP·관문이 필요하다**(1301부터 ZIP에 `third-party\quickjs-ng\` 포함, [3절](#final-gate)) |
| 전체 회귀 | 전체 34경로 진입은 제품 644(645에서 검증)가 마지막이다. 최신 전체 솔루션 검사 통과 기록은 없고 단위마다 관련 집중 검사만 했다(기존 실패는 [4.2절](#baselines)) |
| 마지막 디컴파일 | Client 양판본 2026-11-03, R 전용 2026-11-07, 모두 **HJOW-Athlon(192.168.0.94)**. [기록](docs/exe/cpp-decompilation-20261103.md). 이후 새 디컴파일·원문 변경은 없다 |
| 보호 파일 | AGENTS.md·originals·originalCD·fixture는 수정하지 않았다(마지막 해시 집계는 1262~1266의 2,808파일 불변) |

<a id="done"></a>

## 1. 완료한 작업 묶음 (요약 — 다시 만들지 않는다)

| 묶음 | 단위 | 결과 | 근거 |
| --- | --- | --- | --- |
| M6 LAN·Native 등록(3차) | ~1173, 1258~1267, 1272, 1278, 1279 | M6-a~n 구현·정적 종결. 비MP 도달 가능 경로는 모두 장기 소유자에 연결, 도달 불가 행은 명시 오류 유지 | [M6 감사](docs/exe/cpp-lan-remaining-audit-20261108.md)·[1262 감사](docs/exe/cpp-native-registry-owner-audit-20261110.md)·[1279 종결](docs/exe/cpp-m6n-reachability-closure-20261111.md)·[1278 Fort 섬](docs/exe/cpp-fort-island-populate-20261111.md)·[1272](docs/exe/cpp-m6n-fort-callers-noworld-audit-20261111.md)·[1267](docs/exe/cpp-asset-postpop-noworld-20261111.md)·[1266](docs/exe/cpp-native-fort-snapshot-20261110.md)·[1265](docs/exe/cpp-login-debug-burst-20261110.md)·[1264](docs/exe/cpp-initial-postpop-owner-20261110.md)·[1263](docs/exe/cpp-native-join-test-battle-20261110.md) |
| M7 후속 5건 | 1268~1273 | helpWord 캐시·메타 맵 커서(정적 판정)·희생 안내/이동 marker·오류/예외 로그 | [1268](docs/exe/cpp-help-word-cache-20261111.md)·[1275](docs/exe/cpp-help-regression-korean-screens-20261111.md)·[1273](docs/exe/cpp-meta-cursor-reachability-20261111.md)·[1271](docs/exe/cpp-early-sacrifice-marker-20261111.md)·[1269](docs/exe/cpp-error-log-20261111.md) |
| M8 배포 | 1274, 1280, 1295~1297 | 글꼴 exe 내장 금지·외부 `fonts\`·ZIP 전수 대조·외부 접속 재감사 | [선행 점검](docs/exe/cpp-m8-font-zip-precheck-20261111.md)·[1차](docs/exe/cpp-m8-final-gate-20261111.md)·[재실행](docs/exe/cpp-m8-final-gate-1294-20261111.md) |
| K 한국어(6차) | 1270, 1276, 1281 | K1~K5 | [K1](docs/exe/cpp-korean-k1-merge-20261111.md)·[화면·입력](docs/exe/cpp-korean-screens-input-verification-20261111.md)·[K4/K5](docs/exe/cpp-korean-k4-k5-closure-20261111.md) |
| S 시나리오 편집(9차) | 1277, 1282~1284 | S1~S4 | [S1](docs/exe/cpp-scenario-s1-merge-20261111.md)·[S2](docs/exe/cpp-scenario-s2-test-battle-20261111.md)·[S3](docs/exe/cpp-scenario-s3-ui-20261111.md)·[S4](docs/exe/cpp-scenario-s4-readonly-home-20261111.md) |
| B 알려진 버그(10차) | 1285~1294, NB23 1300 | 34/34 판정, 수정 10건 | [판정 현황표](docs/exe/cpp-known-bugs-status-20261111.md)·[NB08/NB24 보고서](cpppj/known-bugs-nb08-nb24-20261111.json). 요약 [1.1](#bugs) |
| L 일본어 UI(13차 1차) | 1288 | 번역표 509키·외부 `NotoSansJP-*`·`.japanese` 없으면 영어 본문 대체·배포 동봉 | [근거](docs/exe/cpp-japanese-l1-l3-20261111.md)·[보고서](cpppj/japanese-l1-l3-20261111.json) |
| 11.01 카피라이트 | 1291 | Patch Credits 영·한·일 본문에 `Copyright 2026 HJOW` | [근거](docs/exe/cpp-product-1101-20261101.md) |
| D 난이도(11차) | 1292 | Options Difficulty(영·한·일, 기본 보통). AI 설정 +0x220/224/228 주기 배율(쉬움 ×4·어려움 ×½), 쉬움의 채취 유닛 절반(최소 1). 난이도는 AI 단위 값(스커미시가 AI별로 쓴다) | [근거](docs/exe/cpp-difficulty-d-20261111.md) |
| SK 스커미시(12차) | 1293, **1300** | Campaign의 Skirmish 항목·자체 설정 화면 4장(AI 수·AI별 난이도/동맹·금지 유닛 최대 5(Outpost 포함)·시작/취소, 영·한·일). **1300: 월드를 MP 시작 규칙으로 생성**(중앙섬 먼저·참가자별 전투섬을 방향표 링에·소유자=참가자·MP 군도 밀도·MP 간헐천 생성기) | [근거](docs/exe/cpp-skirmish-sk-20261111.md) |
| F 프레임(후순위) | 1296~1298 | 계획 문서·옵션 `Frame Rate`(60/75/120)·양보 대기(CPU 점유 감소) | [계획·결과](docs/exe/cpp-post-plan-fps-cd1072-20261111.md) |
| C 10.72 문서화(후순위) | 1299 | C1 구동 관찰·해결 방안, C2 판본 게이트 목록 | [근거](docs/exe/cpp-cd1072-startup-and-gates-20261111.md) |
| Q QuickJS 엔진 탑재(14차) | 1301 | `cpppj/third_party/quickjs-ng/`(v0.18.0 상류 23파일 바이트 그대로·`provenance.json`·MIT)·정적 라이브러리 `NetstormQuickJs`, `platform/ScriptEngine`(생성/해제·평가·메모리/스택/시간 한도·외부 장치 없음), 시작 자체 검사(7차 활동 기록)·`--quickjs-selftest`, 배포 ZIP·검증 도구에 라이선스/출처 필수화, 가져오기 DLL 불변. 검사 14/14(x64 Release·Debug), ARM64 교차 빌드 | [근거](docs/exe/cpp-quickjs-engine-20261111.md)·[엔진 설명](cpppj/third_party/quickjs-ng/README.ko.md) |

[986 준비본](docs/exe/handoffs/cpp-pause-20261106/README.md)의 한국어·시나리오 후보는 K1(1270)·S1(1277)에서 필요한 부분을 병합했고 S2~S4로 기능 검증을 마쳤다. 원격 Fort 준비분은 987에서 구현했다. 남은 후보 파일은 보관 자료로만 쓰며 현행 소스에 그대로 덮어쓰지 않는다.

<a id="bugs"></a>

### 1.1 B — 알려진 버그 34건 판정 요약 (마감)

행별 판정·근거·검증은 [판정 현황표](docs/exe/cpp-known-bugs-status-20261111.md)가 기준이고, 원문 순서의 [NB01~NB34 등록 목록](docs/exe/cpp-known-bugs-plan-20261109.md)이 있다. **원문 10.78의 알려진 버그를 다시 만드는 클론이라 고쳐진 뒤의 동작 자료가 없는 항목이 많고,** 이미 해결된 듯한 건도 원문·활성 호출자 근거 없이 닫지 않았다.

| 판정 | 건수 | ID |
| --- | --- | --- |
| 수정 완료 | 10 | NB02(엠보스 오프셋 두 좌표 검사), NB04(Even 간헐천 구역 순서), NB08(User Made Campaigns 목록), NB09(긴 길드 태그), NB10(강제 적 범위 밖 정리), NB18(내용물 든 운반체의 사제 줍기 거부), NB20(P/R 키가 든 건설 조각 취소), **NB23(섬 생성 종료: 프레임 분위 범위가 비는 타일 코드, 1300)**, NB24(긴 캠페인 제목), NB29(간헐천 예산) |
| 현행에 이미 반영 | 1 | NB07 |
| 현행 미재현 | 4 | NB03, NB05, NB11, NB22 |
| 해당 없음 | 1 | NB31 |
| **설계 기준 필요** | 1 | **NB28** |
| **분석 후 미확정** | 11 | **NB01, NB06, NB13, NB14, NB19, NB21, NB25, NB26, NB27, NB32, NB33** |
| MP 전용(미수정, 실행 제외) | 6 | NB12, NB15, NB16, NB17, NB30, NB34 |

**D4에 필요한 정보(건별):**

| 건 | 알려 줄 것 |
| --- | --- |
| NB28 | 연속 게임에서 간헐천이 과다해지는 재현 순서(몇 판째, 어떤 모드)와 올바른 개수 |
| NB01 | 보충 간헐천 맵에서 약 1시간 뒤 종료되는 상황의 로그(AI 간헐천 연결 목록 매개변수 불일치는 정적으로 확인하지 못했다) |
| NB06 | AI가 찾은 신전을 보지 못하고 신전을 반복 건설하는 재현 시의 지역/신전 로그 |
| NB13 | "Cid"가 가리키는 유닛/건물(원본 자료에 정의 없음, 산 계열로 추정)과 부동소수 반올림 경계가 나타나는 상황 |
| NB14 | 싱글 플레이 Edit 목록에서 숨길 "다인용" fort를 가르는 기준(fort 종류 값의 의미; 원본 스크린샷에도 영어 대본이 없는 `Beginning`이 보인다) |
| NB19·NB21 | 크리스털을 내려놓은 뒤 처리장에 갔을 때 200SP 미지급 / 건설 메뉴 조각을 든 뒤 컬렉터 선택이 어긋나는 조작 순서(사제 포획이 얽히면 실제 시험도 제외 대상) |
| NB25·NB26 | 썬 캐논 사거리 표시와 발사가 어긋나는 조건, 썬더 캐논이 정면 표적에 계속 쏘는 조건(교전 재현 영상/로그) |
| NB27·NB32 | 공중 표적 변경 중 건설 종료, 워크샾/신전 인접 유닛을 공격하는 아이스 캐논 종료의 재현 조건(공중 3종·특정 공격 재현은 AI 실행 제외) |
| NB33 | 에디터에서 아이스 타워 섬을 삭제하는 조작(삭제 비트가 전달되는지 실행 확인 필요) |

새 제보가 오면 해당 행을 판정 현황표에서 다시 연다.

<a id="lang"></a>

## 2. L — 13차 기타 언어 (남은 것)

1288에 일본어 UI(509키)·11.01 Credits·Difficulty·Skirmish 일본어가 끝났다. 남은 일은 **결정 D3** 뒤의 본문 번역(미션/튜토리얼/캠페인 본문 43개·도움말 128주제·Tell 창 본문·맵/캠페인 이름·유닛·건물 설명)과 다른 언어 노출(독일어 자료는 있으나 미노출, 프랑스어·스페인어·포르투갈어는 이름만 예약)이다. 글꼴 굵기(Noto Sans JP 가변 글꼴의 기본 인스턴스가 Thin)는 다음 L 단위에서 화면 크기별로 확인한다.

<a id="final-gate"></a>

## 3. M8 최종 관문 재실행 (출시 직전, 코드 변경이 모두 끝난 뒤 1회)

작은 단위마다 전체 검사·ZIP을 반복하지 않는다. 코드를 고친 뒤에는 ZIP을 새로 만들어야 한다(1284에서 교정 전 ZIP의 exe로 실패한 사례가 있다).

1. Release 빌드([6절](#rules-ops) 명령) 뒤 `powershell -File cpppj\package-cpppj.ps1 -NoBuild <zip>`으로 ZIP을 만든다.
2. `python tools/verify_cpp_package.py <zip> --extract <빈 폴더> --json <보고서>`로 모든 항목을 SHA-256 대조한다(누락/금지/exe 글꼴 시그니처 0, BUILD_INFO 해시 일치, `third-party/quickjs-ng/LICENSE`·`provenance.json`은 1301부터 필수 항목이다).
3. 푼 배포본의 `Netstorm.exe`로 영어·한국어·일본어 메인 메뉴와 단순 비MP 진입(스커미시 시작 포함)을 확인한다(종료 코드 0, 오류 로그 없음, 설치 폴더 불변).
4. 외부 접속을 재감사한다: WinINet/WinHTTP/URLMon import 0, `master.`/`security.dat`/`Not Validated`/`PIRATE`/`bugs@` 0, checking 목록 0, 새 활성 호출자([외부 정책](docs/exe/cpp-external-services-20261101.md)). 1301 QuickJS가 더한 가져오기는 DLL 28개 불변·함수 +39(CRT 수학/힙·시각/시간대·동기화)뿐이었다.
5. 필요하면 전체 34경로 진입 회귀와 전체 솔루션 검사 기준선을 다시 집계한다(마지막 전체 회귀는 제품 644).

**알려진 한계:** 설치 경로가 약 200자를 넘으면 원문 256BYTE 경로 작성기 한계로 시작 직후 종료한다(원문 보존, 오류 로그가 원인을 지목). 이미 끝난 항목(1274·1280·1284·1288·1295~1297)은 [1절](#done)이다.

<a id="contracts"></a>

## 4. 유지할 계약과 기준선

### 4.1 LAN·M6 코드가 유지할 계약

| 범위 | 유지 조건 |
| --- | --- |
| 장기 저장소 | `ClientConstructionGlobals`의 SP/HP/Canon/Chunks·최근/전체 목록·현재 Player/Bounds를 공유한다. Native Session은 같은 본체·캐시·영향 목록을 빌린다. 임시 World/Player나 TYPE·난수 사본을 만들지 않는다. |
| 자산·Form 삭제 | 자산 Pre/Post·통지 각 83/83VT, 장기 Regular 87/87VT, 기본 Form(Process·Depend·Gump·Content) 삭제 4/4를 연결했다. 큰 `449b30`과 파생 통지는 실제 Runtime을 pin한다. 정확한 잔여 행은 [1249 보고서](cpppj/vortex-regular-owner-20261110.json)의 `staticAudit.remainingRows`를 따른다. |
| NoWorld | Session 없이 Pop되는 객체는 메타 TYPE 108/109/161(VT `0050c9f0`/`0050cc08`)뿐이다(1272). 나머지 Session 소유 VT 행과 큰 AI·일반 Descend·배치 호출자·Fort 상위 Finish는 월드 안에서만 도달하므로 명시 오류를 유지한다(1279). 반례가 나오면 그 경로만 장기 소유자로 옮긴다. |
| 실제 현재값 | 선택된 효과 위치에서 현재 TYPE/역할·Player·게임 시각·단일 게임 난수를 읽는다. 거주 구역 Fort 생성은 실제 Dude count 대 Provider count를 먼저 비교한다. Flare 억제 `005c85f0`/로딩 `005c89b8`/효과 TYPE `00541200`은 같은 장기 전역을 쓰며 Session 생성/반납이 초기화하지 않는다. |
| 호출·해제 수명 | weak 대여 → 실제 Runtime pin → 함수 사본 → 호출을 유지한다. Client 장치는 첫 메뉴/Session 전에 결합한다. body를 State/backing보다 먼저 반납하고 실제 Menu/Session/Host 정리 뒤 장치를 끊는다. 강한 Session 역참조·빈 callback·가짜 ACK·성공 상수를 넣지 않는다. |
| LAN·R | M1~M5·M6·Pop/저장 복구/78h/기록·기존 checking 본체를 재작성하지 않는다. 새 R/TCP·UDP/등록표/시계/Config/주소/기록 owner를 유지한다. R10.71 3BYTE 존/Client10.78 47BYTE 존은 1093 compact 호환 범위다. 원본 R 실행/복사로 대체하지 않는다. |
| 외부 기능 | 요청 DWORD `0054dbac`은 reset 0/1ah·1bh read 근거뿐이며 새 writer/CLI를 발명하지 않는다. 폐기 CD 7f/80·온라인 유효성 검증·PIRATE quota를 복구하지 않는다. IPX/SPX는 명시 미지원, hostname OS 조회/첫 LAN IPv4·숫자 TCP/UDP를 유지하며 원문에 없는 관리 포트를 추가하지 않는다. |
| 스커미시 월드 | 미션 시작 전 `prepareWorld`가 MP 시작 순서(군도 밀도→중앙섬→참가자별 전투섬)를 실행한다. 객체 소유자/배치/Chaff/영역 일치 모드 네 곳은 생성 동안만 MP 규칙으로 바꾸고 끝나면(예외 포함) 되돌린다. 섬 기록은 100개뿐이라 5명 이상은 군도 밀도를 5~8%로 낮춘다. 일반 미션은 월드 계획이 없어 이 경로를 타지 않는다 |
| 주석 함정 | 같은 줄 `//` 주석이 뒤 코드를 삼킨 결함이 1275·1278·1281에서 반복됐다. 코드 줄 중간에 `//` 주석을 넣지 않는다. |
| QuickJS 엔진(14차) | `cpppj/third_party/quickjs-ng/`의 상류 파일은 바꾸지 않는다(`provenance.json` SHA-256, `.gitattributes`의 `-text`). 판본을 올릴 때는 클론에서 새 태그를 꺼내 provenance를 다시 만들고 자체 검사·`netstorm_script_engine_tests`를 다시 돌린다. 엔진 포함 경로는 `ScriptEngine.cpp` 원천 속성으로만 둔다(`netstorm` 대상 전체 포함 경로를 바꾸면 840여 소스가 다시 컴파일된다). quickjs-libc는 넣지 않으며 D7 결정 전에는 스크립트에 게임 상태·파일·네트워크를 노출하지 않는다. 메모리 한도는 상류 `JS_SetMemoryLimit`이 아니라 할당자 콜백 예산이다(상류 한도는 다 차면 예외 값이 `null`이 된다) |

<a id="baselines"></a>

### 4.2 집중 검사의 알려진 기존 실패

변경 전 기준선과 같은 실패는 새 결함으로 세지 않는다. 비교 방법은 기준선 워크트리 또는 자기 파일만 stash한 빌드다.

| 검사 대상 | 기존 실패(기준선 동일 확인 단위) |
| --- | --- |
| `netstorm_pop_context_tests` | 25건(1267) |
| `netstorm_guide_fort_request_tests` | 4건(1278) |
| `netstorm_file_directory_tests` | 3건(1285) |
| `netstorm_menu_session_tests`·`netstorm_main_popup_tests`·`netstorm_file_tests` | 기준선과 동일(1277). menu_session 6건 실패는 1268에서 직접 비교 없이 helpWord와 무관한 메시지로 판단했다 |
| 실제 미션 검사 하네스(`run-dtest.py` 등) | 보호 파일 해시(AGENTS.md 갱신 이후 기록과 다름)로 `passed:false`가 나오지만 미션 진입·종료 코드 0은 정상이다. 1300에서도 같은 값 |
| 과거 938 집중 실패·이월 건수 | [보관본의 5.2절](LEFT_JOBS.old.md#archive-20261109-through-1206) |

<a id="rules-verify"></a>

## 5. 검증 제외와 미해결 한계

### 5.1 계속 유효한 사용자 결정

| 구분 | 적용할 규칙 |
| --- | --- |
| 사용자 직접 확인 | 전체 동일성·모든 시나리오 자연 완주, 사제 포획/희생·특정 적 공격·시나리오 승리를 재현하는 실제 플레이 검사는 건너뛴다. 이들만 남으면 다음 단계로 가며 새 제보는 해당 건을 다시 연다. |
| MP | 디컴파일 참고 구현·컴파일·정적 대조는 수행한다. **새 MP 단위/패킷/게임/통신/기록파일 실행 검사는 하지 않는다.** 실행 제외가 구현 생략이나 동작 검증 완료를 뜻하지 않는다. |
| 공중 3종 | 훨리기그·맨오워·더스트 데빌 전용 생성/생산/행동 관측은 실행하지 않는다. 정적 분석/코드는 가능하다. |
| 보류 분석 | 스펠 효과 도움말 그림 추가 분석(530), 연녹색 직사각형 추가 분석. |
| 환경 | 물리 입력/캡처는 762의 새 부팅/RDP에서도 2회 timeout이었다. 환경 변화 전 재시도하지 않는다. 다른 모니터/DPI·앱 전환·청취·활성 전체화면 물리 출력·실제 CD·Windows 11/ARM 실기는 별도 확인이다. |
| 허용 검증 | 변경과 직접 관련된 **단순 미션 진입/대기/생산/건설로 끝나는 비MP 검사**는 수행한다. 원본/복사본/클론·analyzeManager·별도 창은 AGENTS의 호스트 예외/명시 허용 범위를 따르며 현재 HJOW-Athlon은 예외 호스트다. |

### 5.2 완료로 바꾸지 않을 기존 한계

- **전체 검사 기준:** 최신 전체 솔루션 통과 기록은 없다. 과거 실패 수를 최신 재집계로 표시하지 않으며, 관련 변경이나 최종 관문에서 실제로 다시 검증한다.
- **일반 Battle/자연 완주:** `CampaignSaveGoMain_ActualManagerGateAndSelectedBattleStop`은 일반 Battle/MP 이월로 새 MP 검사를 제외한다. `CampaignForwardBattle_NaturalTempleAssault` 549 실패는 사용자 자연 완주/별도 지원 범위다. 전략/승리 단언을 약화하지 않고, TEST02 로컬 AI/SP·자동 공략 지원은 별도 요청 전 하지 않는다. `owner!=myPlayerId || demo!=0`를 `gate.demo=1`로 우회하지 않는다.
- **부호화:** 원문 8비트/설정·경로의 `EncodeOriginalText`와 화면용 `EncodeDisplayText`/`DecodeDisplayText`를 구별한다(한글·일본어 확장은 표시 전용). MakeAi owner0 이전/삭제·전략 차이, 설치 글꼴/다른 ACP·구형 무BOM 설정 자동 이관은 [글꼴 경계](docs/exe/cpp-font-face-encoding-20261103.md)를 따른다.
- **정밀도:** 32비트 FILE/FileSpec/포인터·SEH/스택·미작성값 자체 일치, 임의 외부 heap→SID, 실제 OS 고갈/물리 디스크 장애·UNC/드라이브 CWD·CD 전체 dump/trace/Retype·x87 전체 입력/픽셀은 호스트/후순위 경계다. 부분 I/O/버퍼 실패/복구 966·new_handler/종료 979를 미착수로 되돌리지 않는다. 활성 호출자 미연결을 이 경계로 숨기지 않는다.
- **음향/그림:** PCM 936은 HJOW-Athlon 프로세스 WASAPI 101653프레임/비무음 99726과 영상/소리 합성을 확인했다. 청취/다른 장치/실제 CD/활성 전체화면까지 검증한 것은 아니다. 덱 검은 띠 981과 사용자 보류 연녹색 사각형을 혼동하지 않는다.
- **설치 경로 길이:** 약 200자를 넘는 설치 경로는 원문 256BYTE 경로 작성기 한계로 시작 직후 종료한다(원문 보존).

<a id="rules-ops"></a>

## 6. 착수·검증·커밋 운영 규칙

1. **착수:** AGENTS.md·이 문서·LEFT_JOBS.dotnetpj.md를 다시 읽고 git status/log로 다른 세션 변경을 확인한다. 현재 호스트가 0.5의 마지막 디컴파일 호스트와 다르면 CPP 작업/원문 분석 전에 `tools/ghidra/refine_all.ps1`을 다시 실행한다. R 원문은 필요 시 `tools/ghidra/refine_root_server.ps1`도 실행하고 기록에 호스트를 갱신한다. HJOW-X3D의 2026-10-27 중단 산출물은 완성본으로 쓰지 않는다.
2. **단위 종료:** 머리말의 확정 제품·다음 단위 번호, 0.1~0.5의 값을 갱신하고 완료 단위를 1절 표에 반영한다. 긴 경과는 단위 문서/보고서에 두고 이 문서에 단락으로 쌓지 않는다. 준비본/정적 대조/컴파일/단위 실행/GUI/실물 확인을 구별해 적는다.
3. **커밋:** 인계 작성 뒤 **자기 파일만** `YYYYMMDD-NN CP 요약`으로 로컬 커밋한다(끝에 Co-Authored-By 트레일러). 당일 번호와 다른 세션의 stage를 확인하고 DN/WB 파일·stage를 수정/정리/커밋하지 않는다. 실패해도 작업을 계속하며 **push하지 않는다**. 저장소/전역 설정과 최초 TSV의 `-text`를 바꾸지 않는다.
4. **빌드:** HJOW-Athlon은 VS18(2026)/MSBuild 18.x, HJOW-X3D는 VS17(2022)/v143이다. 아래 **4병렬** 명령을 PowerShell에서 사용한다(Bash는 `/p:`를 경로로 바꾼다). 단일 실행 핸들의 실제 종료 0을 확인하며 **빌드 중 제품 소스를 변경하지 않는다**. 큰 헤더(`RawCampaignSession.h`·`ClientMain.h` 등)를 바꾸면 수십 분이 걸리므로 설명 주석은 가능하면 .cpp에 둔다. [빌드 안내](docs/cpp-build.md).
5. **검증:** 최초 실패/교정 원시와 기대값을 보존한다. 기대값을 제품에 맞춰 약화하지 않는다. 관련 집중 확인 뒤 다음 작업으로 가며, 전체 34경로/CTest/ZIP 반복은 해당 관문에서만 한다. [5절](#rules-verify)의 실행 제외를 적용한다.
6. **보호/실행:** AGENTS.md·원본/CD 기존 파일·루트 README.md의 **개요**를 수정하지 않는다. 원본 실행 전후 해시/허용 설정 복원, 시작한 프로세스만 종료, CHEAT/검사 옵션과 기본 AI/프로필 분리를 유지한다.
7. **제품:** Windows `Netstorm.exe`, 대상 NetstormCpp/OUTPUT_NAME Netstorm/ARCHIVE_OUTPUT_NAME NetstormCpp, 버전 11.01·세 번째 Patch Credits(HJOW·AI), 무인자 GUI/--help/--run/분석 CLI·`--quickjs-selftest`(14차)를 유지한다. 전체 34경로(Early6·Training9·캠페인16·데모3), EXE 옆 `game/netstorm.tarc` 우선을 유지하고 cwd의 다른 게임/보호 원본을 자동 탐색하지 않는다. 배포 스크립트의 무인자 경로 입력·빈 값 취소·필수 에셋/글꼴/라이선스 동봉 계약도 유지한다.
8. **외부 접속:** 로컬 TCP/IP 외 외부 도메인/IP 기능은 [외부 정책](docs/exe/cpp-external-services-20261101.md)·[주소 목록](cpppj/external-endpoints-20261101.json)에 문서화하고 폐기 온라인 유효성 검증은 비활성화한다. 새 활성 호출자도 감사한다.
9. **언어/인코딩:** 새 문서/코드는 UTF-8, 응답·새 코드의 네임스페이스/상수/함수/클래스/구조체/멤버/반복문 설명은 한국어다. AGENTS의 D2Coding·번역 명칭을 따르고 글꼴은 exe에 내장하지 않고 exe 옆 `fonts\`에서 적재한다. 이 문서는 CRLF로 저장한다. 원시 로그/fixture 인코딩과 ClientMain.cpp 등 `-text` 소스의 혼합 줄바꿈은 바이트 국소 수정으로 보존하고 Python read_text/write_text 전면 재작성은 피한다. Bash 명령 텍스트의 `\\`는 `\`로 줄어들므로 역슬래시가 든 편집은 파일 도구/스크립트 파일로 한다. PowerShell 5.1 stdin은 `$OutputEncoding = [System.Text.UTF8Encoding]::new($false)`, 필요 시 Python UTF-8 모드를 쓴다.
10. **현행 기준:** 옛 로드맵·단위 보고서의 "다음"보다 이 문서의 현재 계획을 우선한다.

```powershell
cmake --build cpppj/build --config Release --target NetstormCpp NetstormRoot --parallel 4 -- /p:UseMultiToolTask=true /p:MultiProcMaxCount=4 /p:EnforceProcessCountAcrossBuilds=true
```

<a id="refs"></a>

## 7. 근거·보관·AGENTS 반영 기준

| 범위 | 기준 자료 |
| --- | --- |
| 이력 보관 | [제품1298 시점 전체(이번 정리 직전)](LEFT_JOBS.old.md#archive-20261111-through-1298), [제품1288 시점 원문](LEFT_JOBS.old.md#archive-20261111-through-1288)(1254~1288 단위 경과·만료된 순서/지시·완료된 M/K/E/S 상세), [제품1240 시점](LEFT_JOBS.old.md#archive-20261110-through-1240), [제품1206 시점](LEFT_JOBS.old.md#archive-20261109-through-1206) 등. 기존 보관본은 수정하지 않는다. |
| 1254~1261 정적 감사·고정 수신 | [1254 직접 소유 경계](docs/exe/cpp-direct-runtime-owner-audit-20261110.md)·[1256 줄바꿈](docs/exe/cpp-client-main-line-endings-20261110.md)·[1258 건설](docs/exe/cpp-native-construction-completion-20261110.md)·[1259 Carrier](docs/exe/cpp-native-carrier-fixed-receivers-20261110.md)·[1260 gameplay](docs/exe/cpp-native-gameplay-fixed-receivers-20261110.md)·[1261 Login](docs/exe/cpp-native-login-fixed-receivers-20261110.md) |
| M6 최초 조사 | [Regular 1217](docs/exe/cpp-no-world-regular-audit-20261109.md)·[Bomb 1224](docs/exe/cpp-bomb-regular-audit-20261109.md)·[16VT/19TYPE 행렬](cpppj/bomb-regular-audit-20261109.json)·[1246 연결](docs/exe/cpp-arc-acid-regular-owner-20261110.md)·[1247 피해율 교정](docs/exe/cpp-type-damage-rate-correction-20261110.md) |
| 비MP 마감 근거 | [UI 929](docs/exe/cpp-ui-implementation-closure-20261105.md)·[Edit 943](docs/exe/cpp-editor-implementation-closure-20261106.md)·[미션 956](docs/exe/cpp-mission-state-scope-20261106.md)·[수명 980](docs/exe/cpp-internal-lifetime-closure-20261106.md) |
| 디컴파일·소스 | [마지막 디컴파일](docs/exe/cpp-decompilation-20261103.md)·[신뢰도](docs/exe/decompile-reliability.md)·[SOURCE_MAP](cpppj/SOURCE_MAP.md)·[모듈](docs/exe/modules.md) |
| 제품·서비스 계약 | [11.01](docs/exe/cpp-product-1101-20261101.md)·[외부 서비스](docs/exe/cpp-external-services-20261101.md)·[TODO](TODO.md)·[미션 대응](docs/gameplay/demo-campaign-map.md) |
| 타 프로젝트 요청 처리 | [webpj 요청(제품 1227 포함 완료)](docs/exe/cpp-webpj-requests-20261109.md)·[session-dump/고정 시각](docs/exe/cpp-session-state-dump-20261109.md). DN의 DEMOVILLE/Help Slider 옛 값 재반영 요청은 유지하며 DN 자체 변경은 CP 작업에서 하지 않는다. |

**AGENTS.md 반영 기준(재확인 2026-11-11):** AGENTS.md는 수정하지 않는다. 사용자 변경은 모두 현재 계획에 반영했다.

| 사용자 변경 | 반영 상태 |
| --- | --- |
| `75e4eed9e` 글꼴 외부 배포 | 한국어 D2Coding·일본어 Noto Sans JP를 exe 옆 `fonts\`에서 적재하고 ZIP에 라이선스와 함께 동봉(1274·1280·1288 확인). 최종 관문에서 다시 확인 |
| `8661e1060`·`8136df49e`·`b5117f524` 7차 오류 로그·기타 언어 | 7차 완료(1269), 기타 언어 일본어 1차(1288) |
| `b5197d48` 10차 알려진 버그 34건 | [1.1](#bugs) 판정 마감 |
| `551d27ae` 세 프로젝트 평가 기준 | 진행 요약 요청 시 cpppj·dotnetpj·webpj를 각각 실행·플레이 가능 여부와 원본 동일성으로 평가한다. DN/WB 값은 각 인계를 따른다 |
| `3051892a` 루트 README 개요 보호 | README.md 개요 수정 금지, README.en.md Overview는 번역으로 갱신 가능 |
| `60226ea93` 11~13차·카피라이트 | 11차 난이도(1292)·12차 스커미시(1293·1300)·13차 일본어(1288)·`Copyright 2026 HJOW`(1291) 반영 |
| `85551c6c2` 14차 QuickJS 추가 | **엔진 탑재 완료(1301)**, D7(연동 범위) 대기, [0.4](#stages) 14차 행, [3절](#final-gate) 2번 라이선스·출처 필수 항목 |
| `22a0877c3` + `60226ea93` + `85551c6c2` DN/WB 후순위 범위 | DN은 CPP **4~14차**와 Linux `$HOME/.netstorm` 경로, WB는 CPP **4~14차**(7차는 브라우저 console, 4차는 WebMCP, 5차는 localStorage, 14차는 불필요, **8·9차 제외**. 12차 스커미시는 웹 대상이 됐고 사용자 결정으로 Outpost도 함께 구현)를 후순위 요구로 가진다. 구현·진행률은 각 인계가 기준이며 CP 작업에서 DN/WB 인계 파일은 수정하지 않는다 |
