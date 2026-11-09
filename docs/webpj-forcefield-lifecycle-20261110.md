# webpj 보호막 수명(RawForcefieldLifecycle) 이식 — WB04-E212

2026-11-10, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.1절 "총·피해·삭제 수명" 묶음의 일부다. 보호막(TYPE 167, 가상 표 0x00507770)의 공통 후처리 직전 예약 접두와 정기 Regular 처리(프레임 진행·사제 존재 검사·자기 삭제)를 옮겼다.

- cpppj 기준선 `1f00a25a`. 소스(`RawForcefieldLifecycle.cpp/.h`)·시험(`ForcefieldLifecycleTests`·`ForcefieldRegularCurrentTests`)·fixture(`forcefieldpostpop-x86.tsv`) 모두 기준선 뒤 변경이 없다(`git diff --numstat 1f00a25a HEAD` 0줄).
- 근거 커밋: 소스 `fbdcb4df`, fixture 마지막 변경 `6a3aafb8`.
- 원본 fixture: `cpppj/tests/fixtures/forcefieldpostpop-x86.tsv` 의 10.78(originals) 1208행. 접두(Prefix) 128행과 Regular 1080행이다. 입력은 종류·flags·예약 두 번의 성공 여부·사건·count·payload·좌표 비트·사제 TYPE·후보 배치(layout)·extra, 출력은 효과 기록·반환 float 비트·보호막 50바이트 raw 전체다. CD 10.72·10.37 행은 웹 범위(10.78) 밖이다.
- cpppj 시험: 같은 PC 의 기존 통합 시험 실행 파일(`cpppj/build/bin/Release/netstorm_tests.exe --filter Forcefield`, 게임 실행 아님)에서 17건 통과를 확인했다.
- 공유 장치(모두 기존 모듈 재사용): E41 `squid-postpop.js`(`setBody('forcefield')`·`postPopVirtual`·`postPopBase`), E1 `squid-pop.js`, WB04-B `raw-squid-finder.js`·`squid-hash.js`, E65 `raw-frame-advance.js`, D5c `squid-process.js`(`addRegular`), E63 `raw-priest-forcefield.js` 의 보호막 TYPE/사제 TYPE 상수.

## 1. 옮긴 것

`www/core/raw-forcefield-lifecycle.js`(DOM 없음, 10.78 전용)

- 상수: `FORCEFIELD_TYPE`(167)·`PATCH_FORCEFIELD_VTABLE`(0x00507770)·`FORCEFIELD_FRAME_EVENT`(1)·`FORCEFIELD_CHECK_EVENT`(2). 두 주기는 실제 PE 상수 비트(0x3da3d70a=0.08f, 0x3c23d70a=0.01f)를 `Math.fround` 로 보존했다.
- `RawForcefieldPostPop(pool, hooks)` — 후처리 직전 접두 00448d20
  - `prefix(sid, flags)`: flags 비트 1 일 때만 `reserveRegular()` 가 참이면 사건 1(0.08f), 다시 `reserveRegular()` 가 참이면 사건 2(0.01f)를 `addRegular` 로 예약한다. 첫 확보 실패는 두 번째 확보를 막지 않는다. 도달한 장치만 요구하고(선택된 장치가 없으면 그 위치에서 오류) 객체 사전 검사를 넣지 않는다. 공통 base 는 호출자가 이어 준다.
  - `validate(sid)`: 명시 사전 검사(SID<5·free/contained 비트 9·TYPE≠167·가상 표≠0x00507770 이면 오류, 읽기 전용). `SquidPostPop.validate` 가 등록 몸체의 자기 검사로 부른다.
- `ForcefieldRegularState`: `priestType`(독립 재생값)·`checking`(호출마다 현재 DWORD 를 읽는 getter, cpppj `RawCheckingFlag` 에 해당)·`sharedPriestType`(장기 전역 005412d0 의 getter, 있으면 매 검사마다 전체 DWORD 를 다시 읽음). `currentPriestType()` 은 BYTE 로 줄이지 않는다.
- `RawForcefieldRegular(pool, hash, types, state, hooks)` — Regular 처리기
  - `handle(sid, event, count, payload)`: 사건 1 은 `advance(sid, 1, 0)` 뒤 0.08f 를 돌려준다. 사건 2 는 사제 검색에 성공하면 0.01f 를, 아니면 `destroy(sid, 0)` 뒤 -1 을 돌려준다. 그 밖의 사건은 `checking()` 이 비영일 때 `assertion('0', '.\\Dais.cpp', 0x479)` 를 부르고 0 을 돌려준다. count·payload 는 읽지 않으며 PATCH 는 객체 사전 검사를 넣지 않는다(모르는 사건은 raw 를 읽지 않는다).
  - 사제 검색: `hooks.findTypeAt(x, y, 사제 TYPE)` 가 있으면 현재 사제 DWORD 와 raw float 를 그대로 넘긴다(실제 월드의 타입별 단계 선택 004b1fa0 재사용 자리). 없으면 raw 좌표를 0 방향 절삭한 한 칸으로 독립 일반 finder 를 만들어 해시 체인 순서대로 TYPE 이 현재 사제 DWORD 와 같은 첫 SID 를 돌려준다. 후보의 소유자·HP·state 는 필터가 아니고 일반 finder 의 매장 제외·발자국 조건만 적용된다.
  - 비유한/32비트 범위 밖 좌표는 오류다(임의 정수로 감기지 않음).
- `makeForcefieldPostPopProcessHooks(pool, host, reserve)`: 같은 풀의 Process 호스트의 `addRegular` 로 예약 훅을 만든다. 풀이 다르거나 `reserve` 가 함수가 아니면 오류.
- `makeForcefieldFrameHooks(pool, advance, hooks)`: 같은 풀의 프레임 진행기(`RawFrameAdvance`)로 `advance` 를 채운 훅 묶음. 나머지 효과는 그대로 둔다.
- CD 10.72 의 구성 사전 계약(예약 훅 필수·객체 사전 검사)은 웹 범위 밖이라 옮기지 않았다.

## 2. 검증

`tests/forcefield-lifecycle.test.mjs`(21건)

- 원본 관측 재생: 10.78 1208행 전부(접두는 예약 두 번·Regular 추가·공통 `postPopBase` 호출 기록, Regular 는 진행·삭제 기록과 반환 float 비트, 모든 행에서 보호막 50바이트 raw 일치). 접두 128행 재생을 cpppj `PostPrefixCurrent_ForcefieldNativePrefixes384Rows`(세 판본 합계 384) 처럼 따로 한 번 더 확인했다.
- cpppj 계약 이식
  - `PostPrefixCurrent_*` 7건: 첫 예약 없음(flags 0·2)은 장치/raw 비접근, 두 예약 실패는 생성 장치 비요구, 첫 예약 성공 뒤 생성 누락은 앞선 콜백 쓰기를 보존, 두 번째 예약의 명시 실패는 사건 1 을 보존, 첫 생성 변이 뒤에도 객체 재검사 없이 두 번째 예약, 가상 표 분배가 잘못된 TYPE/free 도 공통 부모로 직행, 필수 예약이 없으면 공통 깊이·슬롯을 바꾸지 않고 중단.
  - `ForcefieldRegularCurrent_*` 7건: 모르는 사건 raw 비접근, 진행 callback 이 처리기를 반납해도 완료, 장기 DWORD·raw float 의 타입별 finder 전달, 삭제 누락이 완료된 검색 효과를 보존, 삭제 callback 반납 뒤 -1, 진단의 현재 checking 읽기와 복귀, 진단 장치 누락 시 raw 무변경.
  - `ForcefieldLifecycle_RequiresBindingAndDiagnosesInvalidEvents`: 명시 `validate` 의 무변경 거부, 다른 풀 연결 거부, 실제 Pop 이 두 예약을 소비하고 깊이가 0 으로 돌아옴, 진단 장치 누락·해제.
- 웹 계약: `validate` 의 SID/free/contained/TYPE/가상 표 거부와 읽기 전용, 비유한·범위 밖 좌표와 타입 표 크기·판본 거부, 훅 조립의 풀 일치/불일치, **음수 소수 좌표의 0 방향 절삭**(아래 참조).

최초 실패와 교정: 모듈은 첫 실행에 21건 중 20건이 통과했다(1건은 시험 쪽 결함).

- 비교가 실제로 작동하는지 모듈 사본을 바꿔 돌려 확인했다. 확인 뒤 모두 원본으로 되돌렸다(cmp 일치).
  - 실패한 변형: 검사 주기 0.01→0.02(4건), 접두의 사건 1 번호를 2 로 교체(4건), 삭제 flags 0→1(2건), 장기 사제 TYPE 을 BYTE 로 줄임(1건).
  - **통과한 변형**: 좌표 절삭 `Math.trunc` → `Math.floor`. fixture 좌표가 모두 양수라 구별하지 못했다. 그래서 음수 소수 좌표 계약을 추가했다.
- 첫 작성한 음수 좌표 계약이 원본 모듈에서도 실패했다. 후보 SID 를 고르는 식(`60 > SHIELD ? SHIELD : 49`)이 보호막 자신(50)을 덮어써서 생긴 시험 결함이었다. 그리고 finder 는 사각형 왼쪽/위를 0으로 자르므로 `floor(-0.5) = -1` 은 왼쪽에서는 구별되지 않고 **오른쪽 경계 -1 이 "보드 끝"으로 해석**되는 점에서 달라진다. 이에 맞춰 대조점을 (보호막 x=-0.5, 같은 행 x=0.25 의 사제는 잡힘 / 같은 행 x=100.25 의 사제는 잡히지 않아 삭제)로 고쳤다. 고친 뒤 원본은 통과하고 `floor` 변형은 실패하는 것을 확인했다.

## 3. 경계와 남은 것

- 연결하지 않았다. 보호막 생성(`RawPriestShield`, E63)이 쓰는 Pop·Process 호스트와 이 접두/Regular 를 한 장면으로 묶는 cpppj `ForcefieldLifecycle_ComposesSpaceProcessesAndSelfDeletion` 은 `SquidDestroyLifecycle`(삭제 훅 공통 Pre/PostDestroy)이 있어야 하므로 **SquidDestroyLifecycle 이식(E213) 뒤 `forcefield-lifecycle-integration` 시험으로 닫는다.**
- 실제 월드의 타입별 단계 선택(004b1fa0) 연결(`findTypeAt`)과 장기 전역(사제 TYPE 005412d0·checking 005e4794) 연결, 실제 Process 호스트·프레임 진행기를 쓴 `makeForcefield*Hooks` 사용은 순서4~5 몫이다(PORT_MAP session 0).
- CD 10.72 는 웹 범위 밖이다. 이식 때 10.72 비교가 필요해지면 `forcefieldpostpop-x86.tsv` 의 originalCD 행과 cpppj `ForcefieldLifecycle_ReplaysCd` 를 근거로 한다.

## 4. 다음 시작점

WB04-E213 은 `SquidDestroyLifecycle`(공통 pre/postDestroy 의 비전투 선택·공급/작업장 목록·타입 통계·비용 장부)이다. 기준선 뒤 변경이 없고, 선행 모듈(`squid-destroy.js` RawSquidDestroy·`raw-graph.js`·`squid-reward.js`·`squid-postpop-state.js`)이 이미 있다. 끝나면 이 단위가 미룬 보호막 조합 시험(`ForcefieldLifecycle_ComposesSpaceProcessesAndSelfDeletion`)을 `tests/forcefield-lifecycle-integration.test.mjs` 로 옮긴다.
