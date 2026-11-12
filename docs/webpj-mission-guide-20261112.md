# webpj Guide 단계 반복 — RawMissionGuide (WB04-E340, 2026-11-12, 4-D B 구획 선행)

3.1절 4-D B 구획의 **MissionLifecycle** 시험(`MissionLifecycleTests.cpp`)이 실제 `RawMissionGuide` 반복 몸체를 쓰므로, `o/` 단위 **RawMissionGuide**(기준선 `.cpp` 36 + `.h` 30 = 66줄, 기준선 뒤 변경 없음)를 `core/raw-mission-guide.js` 로 앞당겨 옮겼다(3.0절 "필요한 공유 장치가 없으면 먼저 이식"). 목록 밖 항목이라 B 구획 목록에 이유와 함께 한 줄을 더했다.

## 1. 규칙

- **run()(원본 00463bb0):** 현재 미션(`hooks.current()`)이 없으면 아무것도 하지 않는다. 있으면 단계 변경 전 `sequence` 를 기억하고
  1. flags 값 8 이 서 있으면 현재 미션이 **아직 있을 때만**(다시 조회) `release()` 를 부르고 끝낸다.
  2. `pending() == 0` 이고 `dialogsClear()` 일 때만(짧은 평가 — pending 이 비영이면 창 관문을 읽지 않는다) 현재 미션을 다시 얻어 flags 값 1 이 있으면 `advance` 하고, 현재 단계를 다시 읽어 0..6 이면 `stage(현재 미션, 단계)` 를 부르고 단계 6 이면 이어서 `release()`, 범위 밖이면 `invalidStage()`.
  3. 막혔으면 `postpone(현재 미션)`.
  4. 마지막으로 현재 미션을 다시 얻어 없거나 단계가 6 이거나 단계가 바뀌지 않았으면 끝내고, 바뀌었으면 같은 호출 안에서 처음부터 반복한다(do 루프). 단계 5 의 advance 는 6 까지만 만들고 같은 Run 에서 단계 6/해제를 부르지 않는다(다음 Run 몫).
  - 원본이 현재 포인터를 역참조하는 위치(전환 확인·단계 선택·가상 호출 대상)마다 현재 값을 다시 읽으며, 그 위치에서 NULL 이면 손상 범위를 호스트 `Error('…현재 미션 NULL')` 로 거부한다. 그 밖의 조회는 NULL 을 정상 종료로 본다.
- **RawMissionGuide.advance(state)(원본 00482c80, 정적):** `sequence` 를 1 올리고(32비트 감김) flags 값 1 을 지운다.
- **생성:** 일곱 경계(`current`·`pending`·`dialogsClear`·`stage`·`postpone`·`release`·`invalidStage`)가 모두 함수여야 하며 하나라도 없으면 `TypeError`(아직 없는 UI/가상 몸체를 빈 성공으로 바꾸지 않는다). 웹은 10.78 만 옮겨 판본 인자가 없다.

## 2. 웹 범위

- 기준선 뒤 cpppj 변경이 없다. PORT_MAP: `RawMissionGuide.cpp` port(base `1f00a25a`). 실제 Guide 객체(현재 미션 포인터·Tell/창 조회)와 가상 단계 몸체(MissionLifecycle)·pending(client State)과의 연결은 B 구획 `Mission`·`CampaignGuideRelease`·4-D 조립 때 한다(PORT_MAP session 0).

## 3. 검사 (`tests/raw-mission-guide.test.mjs` 12건)

- **원본 독립 관측:** `mission-guide-x86.tsv` 1550행(Run `R` · Advance `A` × 두 객체의 flags/sequence × pending/창 관문 × 호출 도중 외부 변경 7가지(action 1~6·8)(Advance·flags 설정·해제 비트·단계 변경·현재 소멸·현재 교체·flags+단계) × 현재 포인터 조회 횟수): 최종 현재 식별자·두 객체의 전체 208BYTE(다섯 필드 밖 stale 바이트 보존 포함)·외부 호출 순서를 전부 대조한다. **첫 실제 실행에서 1550행이 모두 일치했다.** manifest 에 `mission-guide` 항목을 추가했다.
- **cpppj 시험 이식:** `MissionGuide_Native1550`·`StageFiveDefersReleaseUntilNextRun`·`CurrentGetterSeparatesStageSelectionAndTarget`·`RequiresEveryBoundary`(판본 거부는 제외).
- **웹 계약:** advance 의 32비트 감김과 flags 값 1 만 해제(상위 비트 보존), 현재 없음 무동작, 해제 비트 8(두 번째 조회가 이미 비면 release 안 함), pending 비영의 짧은 평가, 창이 막히면 advance 없이 postpone, 범위 밖 단계 진단과 단계 6 의 같은 Run 해제, 단계 변화의 같은 호출 반복, NULL 진단(단계 선택 위치·가상 호출 대상 위치 두 곳).

## 4. 변형 확인과 남은 경계

- 한 문자열 치환 변형 29개를 돌려 첫 실행에서 2개가 남았다. ① `sequence === 6` → `>= 6` 은 앞의 `sequence <= 6` 조건 안이라 같은 결과인 **동등 변형**이다. ② `requireCurrent()` 의 NULL 거부 제거는 그 뒤 필드 읽기가 `TypeError` 로 어차피 실패해 결과가 같아 보였으나 진단 메시지가 달라지므로, NULL 시험에서 `현재 미션 NULL` 메시지를 단언해 잡았다. **남은 동등 변형은 1개.**
- 남은 경계: 실제 Guide 객체·가상 단계 몸체 연결(MissionLifecycle 다음 단위들), pending 의 client `State` 연결.
