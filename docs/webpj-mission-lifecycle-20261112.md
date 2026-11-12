# webpj Guide 가상 단계 분배 — MissionLifecycle (WB04-E341, 2026-11-12, 4-D B 구획)

3.1절 4-D B 구획 첫 항목의 마지막 파일인 client **MissionLifecycle**(기준선 `.h` 19 + `.cpp` 36 = 55줄, 기준선 뒤 변경 없음)을 `core/mission-lifecycle.js` 로 옮겼다. 단계 0 이 쓰는 `MissionFirst`(client `MissionGuideInput` 의 004c2a70)는 같은 파일의 나머지 입력 처리와 함께 옮기기엔 의존(입력 사건·큐·ScriptCommand)이 커서 **`core/mission-guide-input.js` 첫 부분**으로만 먼저 옮겼다(PORT_MAP `MissionGuideInput.cpp` partial). 선행으로 앞 단위 E340 이 `o/RawMissionGuide` 를 옮겼다. 이 단위로 B 구획 첫 항목(MissionFortSource·MissionFortRequest·MissionTechnology·MissionLifecycle·MissionLoadingMessage)이 모두 끝났다.

## 1. 규칙

- **runTutorialMissionStage(state, sequence, hooks):** `RawMissionGuide` 가 현재 단계 번호로 부르는 원본 가상 표의 0..6 이다(원본 순번을 별도로 복제하지 않고 `state` 를 직접 쓴다). 큰 초기화/Start/프레임은 실제 몸체(hooks)에 위임하고 단계 0/2/5/빈 6 을 직접 옮겼다.
  - 0 → `missionFirst(state, hooks.showStage)`(순번 0 이면 flags 값 1 만 요청, 아니면 현재 안내 재표시)
  - 1 → `hooks.initialize()`(004c3290 상당의 설정/월드 설치와 flags 값 1 쓰기는 그 몸체의 몫)
  - 2 → `hooks.tellGlobal('init', 1)`(004cf970) 뒤 flags |= 1
  - 3 → `hooks.start()`(실제 `RawMissionStart`; Start 가 같은 순번을 증가시킴) · 4 → `hooks.frame()`(공통 프레임)
  - 5 → `hooks.tellMission(flags 의 값 4 비트가 있으면 'Success' 아니면 'Failure')` 가 비영이면 `closeTransient()` 뒤 `pause()`(자기 정지)를 부르고, 항상 flags |= 1. **두 효과가 모두 있는지 확인한 뒤** 닫기를 부른다(정지가 없는데 팝업만 닫지 않는다).
  - 6 → 원본 004437f0 의 실제 무동작 · 그 밖의 번호는 `RangeError`.
  - 선택된 단계가 요구하는 효과가 없으면 도달한 위치에서 `Error` 로 거부하고 flags 를 바꾸지 않는다(빈 성공 금지).
- **pauseMissionWorldEntry(current, pause)(00463a40):** 현재 Guide 미션 조회를 한 번 하고, 있을 때에만 가상+28 의 Pause 를 부른다(Start 가상+10 과 구별). 조회가 없거나 현재가 있는데 pause 가 없으면 거부한다.
- **missionFirst(state, tellStage)(004c2a70, mission-guide-input.js):** 순번이 0 이면 Tell 을 부르지 않고 flags 값 1 만 켠다(다른 flags 비트와 `tellAtIndex`/`fieldC0`/`fieldC4` 는 보존). 그 밖에는 `tellStage()` 로 현재 안내를 다시 표시하며 연결이 없으면 거부한다.

## 2. 웹 범위

- 기준선 뒤 cpppj 변경이 없다(`MissionLifecycle.cpp/.h`, `MissionGuideInput.cpp/.h`). PORT_MAP: `MissionLifecycle.cpp` port(base `1f00a25a`), `MissionGuideInput.cpp` **partial**(남음: 미션 제어 명령 코드 일곱 개 상수·`missionControlScriptCode`·`queueMissionControlAction`·`dispatchMissionAbort`·`captureMissionReload`·`applyMissionDemo`·`tellMissionStage`·`dispatchMissionFirst`·`queueMissionFirstKey`). 이어지는 B 구획 `Mission`·`MissionGuideInput`·`CampaignGuideRelease` 항목에서 나머지를 옮긴다.
- cpppj 시험 `MissionLifecycle_ActualPendingAndEmptyEntry` 중 client `State`(대기열 `Pending`/`Post`/`Take`) 부분은 State 클래스(PORT_MAP `State.cpp` todo)를 옮길 때 같이 옮긴다. 월드 진입 Pause 부분만 이 단위에서 확인했다.

## 3. 검사 (`tests/mission-lifecycle.test.mjs` 9건)

- **원본 독립 관측:** `mission-lifecycle-x86.tsv` 831행(월드 진입 Pause / 실제 Guide 반복 × flags·순번·현재·pending·창·Tell 결과·정지 깊이·자기 정지·종료 요청·반복 수·기한 double 비트): 현재 활성·정지 깊이·미션 208BYTE(기한 double 8BYTE 포함)·외부 호출 순서를 전부 대조한다. 실제 기존 몸체(`RawMissionGuide`·`RawMissionFrameGate` 의 정지/재지연)와 이 단위의 단계 분배·월드 진입 Pause 를 같은 상태에 연결해 돌린다. **첫 실제 실행에서 831행이 모두 일치했다.** manifest 에 `mission-lifecycle` 항목을 추가했다.
- **cpppj 시험 이식:** `MissionLifecycle_Native831`, `MissionLifecycle_ActualPendingAndEmptyEntry`(월드 진입 Pause 부분).
- **웹 계약:** 단계 0(순번 0/그 외)·1~4 위임과 단계 2 의 init Tell·flags 쓰기, 단계 5 의 성공 비트(값 4) 선택·Tell 비영일 때 닫기→정지 순서·Tell 0 일 때 팝업/정지 불필요·두 효과 사전 확인, 단계 6 무동작과 범위 밖(7·-1) 거부, 단계별 효과 누락의 도달 위치 거부(flags 무변경), MissionFirst 의 순번 0 분기와 필드 보존, 자기 정지 도우미의 재진입 생략.
- 첫 실행의 실패 1건은 시험 쪽 산술 오류였다(`0xfffffffb` 는 이미 값 1 비트가 켜져 있어 단계 5 뒤에도 그대로인데 `0xffffffff` 를 기대했다). 모듈은 그대로이다.

## 4. 변형 확인과 남은 경계

- 한 문자열 치환 변형 22개(단계 분배·월드 진입 Pause 19 + MissionFirst 3)를 돌려 첫 실행에서 1개가 남았다: 월드 진입 Pause 의 현재 조회 존재 검사를 지워도 `current()` 호출이 같은 종류의 오류로 실패해 결과가 같아 보였다. 진단 메시지(`현재 Guide 조회 누락`·`Pause 연결 누락`)를 단언해 잡았으므로 **남은 변형은 0개**이다.
- 남은 경계: 실제 Guide 객체(현재 미션 포인터)와 `RawMissionStart` 몸체·Tell/창 조회의 연결(B 구획 `Mission`·4-D 조립), `MissionGuideInput` 나머지, client `State` 대기열.
