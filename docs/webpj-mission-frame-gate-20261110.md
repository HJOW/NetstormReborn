# webpj 미션 공통 프레임 문(RawMissionFrameGate) 이식 — WB04-E231

2026-11-10, HJOW-Athlon. 10.78 Tutorial/미션의 공통 프레임(RunFrame, PATCH 004c34c0)과 단계 예약(Advance 004c33f0)·안내 중 정지(PauseForDialog 004c2a40)·후속 안내 지연(Postpone 004c3170)을 옮겼다. 승패/사제 사건 확인 → 안내 잠금 10프레임 → tellAtList → 재개 7프레임 → 데모 종료 → Tutorial 1~6 판정기 호출의 순서와 현재 값 재조회 규칙을 한 곳에 모은 것이다. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.1절의 "순서4 이후로 넘기는 `o/` 단위"에 속하며 기준선 뒤 변경이 없고 원본 관측이 있어 E230 객체 trace 서식에 이어 옮겼다.

- cpppj 기준선 `1f00a25a`. 소스(`RawMissionFrameGate.cpp/.h`)·시험(`MissionFrameGateTests`)·fixture(`missionframe-x86.tsv`) 모두 기준선 뒤 변경이 없다(`git diff --numstat 1f00a25a HEAD` 를 저장소 루트에서 확인, 출력 없음).
- 근거 커밋: 소스·fixture `c3a294ed`(2026-10-15).
- 원본 fixture: `missionframe-x86.tsv` 328행(10.78 전용 관측; Frame·Advance·Pause·Postpone 네 몸체). 입력은 사건·게임 시각(double 비트)·시계 정지 깊이·다이얼로그/지식/안내/막는 Gump 조회 반환·재진입 변이 6종·14개 상태 필드의 원본 비트, 출력은 전체 상태 14필드의 비트·정지 깊이·외부 조회/효과 순서(`Q`·`O`·`D`·`K`·`G`·`B`·`S`·`L`·`E`·`T<번호>`·`P`·`R`)·반환값이다.
- 공유 장치: 없음(순수 상태 규칙). 웹의 `raw-tutorial-mission-release.js`(E15)는 같은 전역 이름(`stage`·`stageDirty`·`tutorialNumber`)을 갖는 상태 객체를 빌리지만 이 저장소 인스턴스와는 아직 연결하지 않았다(아래 3절).

## 1. 옮긴 것

`www/core/raw-mission-frame-gate.js`(DOM 없음, 10.78 전용)

- `createMissionFrameGateState()`: 원본 Tutorial 객체의 공통 필드와 세 단계 전역을 하나의 장기 저장소로 만든다(단계 65·stageDirty 1·tutorialNumber 0·데모 0·시작/기한/타이머 0·float 지연 0.5·잠금 10·started 0·scriptPaused 0·clearFrames 0). Tutorial1~6 판정기도 `stage`/`timerSeconds` 를 이 인스턴스에서 직접 읽어야 한다.
- `RawMissionFrameGate(state, hooks)`: 시계(`gameSeconds`, 벽시각 아님)·`paused`·`checkOutcome`·`dialogsClear`·`knowledgeOpen`·`guideOpen`·`blockingGumpOpen`·`tellStage`·`tellAt`·`endDemo`·`tutorial(number, event)`·`pause`·`resume` 13개 경계가 모두 있어야 생성된다(누락은 TypeError).
- `runFrame(event)`: started 가 0 이면 아무 조회/효과 없이 0. 아니면 stageDirty = 0 → `checkOutcome` → (다음 단계 기한 ≤ 게임 시각 **또는** 시계 정지 중) **그리고** 잠금 > 0 일 때 다이얼로그가 있으면 잠금 10, 없으면 1 감소, 잠금 < 1 이면 `tellStage` → tellAt 기한(< 게임 시각 − 시작 시각)이 있고 다이얼로그 없음·지식 창 없음이면 `tellAt` → 정지 보유 중이고 잠금 0 이면 안내/다이얼로그/막는 Gump 가 모두 없는 연속 프레임을 세어 7프레임째에 `resume`(signed DWORD 감김) → 데모이고 기한이 있고 지났으면 `endDemo` 후 기한 0 → 잠금 0·비데모·번호 1~6 이면 `tutorial` 결과 반환.
- `advance()`: 시계 정지 중이 아니고 잠금 < 1 일 때만 단계 char 를 올려(저 8비트 부호 확장) 잠금 10, 다음 기한 = float 지연 + double 게임 시각(현재 시각을 float 로 줄이지 않음), 타이머 0 을 예약하고 true. 아니면 false 와 변화 없음.
- `pauseForDialog()`/`pauseMissionFrameForDialog(state, paused, pause)`: 시계 정지가 아니거나 자기 소유 표식이 없으면 정지를 한 깊이 걸고 표식 1. 이미 자기 정지가 있으면 재진입을 생략.
- `postpone()`: 이미 비영인 다음 기한만 게임 시각 + float 지연으로 다시 예약.

## 2. 검증

`tests/mission-frame-gate.test.mjs`(3건)

- 원본 관측 재생: 328행 전부(고유 입력). 행마다 같은 14필드 원본 비트로 새 상태를 만들고 입력 비트(double/float)와 시계 정지 깊이(unsigned DWORD)·재진입 변이를 적용해 전체 상태 비트·깊이·trace·반환값을 대조한다.
- cpppj 계약 포팅 1건: 13개 경계 각각을 빼면 생성이 거부된다.
- 웹 계약 1건(원본 fixture 가 상태 비트만 보고 반환을 보지 않는 경계): 초기 상태, 승패 확인 시점의 stageDirty = 0, 시작 전 프레임의 무조회, `advance` 의 true/false 와 정지 중·잠금 중 거부, 독립 자기 정지 함수의 누락 거부와 재진입 생략, null 상태 거부.

최초 실패와 교정: 모듈은 처음부터 328행이 모두 통과했다. 시험 쪽 결함 하나를 고쳤다(시계 정지 깊이는 unsigned DWORD 라 0 에서 1 감소하면 4294967295 인데 JS 정수로 −1 이 됐다. 시험 환경의 깊이를 `>>> 0` 으로 감았다). 모듈 사본을 바꿔 돌려 비교가 작동하는지 확인했다. 확인 뒤 원본으로 되돌렸다(diff 일치). 처음 변형 47건 중 5건이 살아남아 4건은 웹 계약 1건을 더해 잡았다.

- 실패한 변형: 잠금/재개 상수, 시작 게이트·표식·확인 순서 외 모든 분기 조건(`<=`/`<`, `||`/`&&`, 단락 순서, 다이얼로그/지식/안내/막는 Gump 조회 각각, 잠금 비교, 데모 조건 셋, 튜토리얼 범위 양끝), 카운터 증가량·재개 임계·정지 표식·카운터 초기화 위치, 단계 char 부호 확장, 지연/타이머/잠금 예약, 정지 조회 제거, postpone 조건.
- 적용되지만 통과한 변형(동등 변형): `advance` 의 단계 증가에서 `& 0xff` 대신 `& 0xffff` — 이어서 `(next << 24) >> 24` 로 저 8비트만 부호 확장하므로 같은 결과다.

## 3. 경계와 남은 것

- 실제 미션 객체에 연결하지 않았다. 웹의 `raw-tutorial-mission-release.js` 가 빌리는 상태 객체를 이 저장소 인스턴스로 합치는 일, 시계·UI(지식/안내/Gump 조회)·`tellStage`/`tellAt`/`endDemo` 본체·Tutorial1~6 판정기와 호출 지점(실제 입력/화면 호출마다 한 번)은 순서4·6 몫이다(PORT_MAP session 0).

## 4. 다음 시작점

WB04-E232 는 3.1절에서 고른다. 기준선 뒤 변경이 없는 단위(`SquidSpatial`·`RawTerrainGrowth`·`RawTotalMade`)를 권한다.
