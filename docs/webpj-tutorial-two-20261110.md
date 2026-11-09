# webpj Secret Workshop 단계 판정기(RawTutorialTwo) 이식 — WB04-E238

2026-11-10, HJOW-Athlon. 튜토리얼 2(Secret Workshop)의 A~H 단계 조건(PATCH 004c3bb0)을 옮겼다. 신전·워크샾·썬 디스크 발사기 제작 수, 선택 상태, 2초/4초 지연 타이머, 회수 완료 명령으로 공유 미션 게이트의 `Advance`/`PauseForDialog` 를 부른다. E237 의 `RawTotalMade`(허용/제작 집계)와 E231 의 `RawMissionFrameGate`(단계/타이머/정지)를 그대로 써서 원본처럼 같은 저장소 위에서 판정한다. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.1절의 "순서4 이후로 넘기는 `o/` 단위"에 속하며 기준선 뒤 변경이 없고 원본 관측이 있어 E237 에 이어 옮겼다.

- cpppj 기준선 `1f00a25a`. 소스(`RawTutorialTwo.cpp/.h`)·시험(`TutorialTwoTests`)·fixture(`tutorial-two-x86.tsv`) 모두 기준선 뒤 변경이 없다(`git diff --numstat 1f00a25a HEAD` 를 저장소 루트에서 확인, 출력 없음).
- 근거 커밋: 소스·fixture `95f879a0`(2026-10-15).
- 원본 fixture: `tutorial-two-x86.tsv` 346행(10.78 전용, `Frame` 몸체). 입력은 사건·게임 시각(double 비트)·시계 정지 깊이·재진입 변이 4종·미션 게이트 14필드·제작 집계 셋(신전 123·워크샾 85·발사기 71)·현재 선택 SID·생산 목록 포함 여부·Tell 반환·옵션 8 BYTE·회수 금지·허용 표 값, 출력은 반환값·게이트 14필드 비트·정지 깊이·외부 조회/효과 순서(`R`·`S`·`C71`·`Z`·`X`·`Tell`·`K`·`Q`·`P`)·옵션 8 BYTE·회수 금지·허용 표·현재 선택이다.
- 공유 장치: E237 `raw-total-made.js`, E231 `raw-mission-frame-gate.js`, `squid-postpop-state.js`, `sid-pool.js`.

## 1. 옮긴 것

`www/core/raw-tutorial-two.js`(DOM 없음, 10.78 전용)

- `RawTutorialTwo(pool, types, made, gate, state, hooks)`: 10.78 풀·188칸 TYPE 표·옵션 5 BYTE 이상·효과 8개(`gameSeconds`·`refreshEnergyRange`·`selected`·`productionContains`·`resetSelection`·`clearSelection`·`tell`·`closeTransientGumps`)가 있어야 생성된다(아니면 효과 전에 TypeError).
- `handle(event)`: switch 진입 때 현재 단계를 고르며 callback 이 다음 단계로 바꿔도 같은 호출에서 다른 분기로 넘어가지 않는다. 반환값은 항상 0.
  - 'A': 옵션 BYTE 4 = 0, 2 = 2, 에너지 공급 반지름 갱신, 신전 부류(0x200) 제작 수 > 0 이면 Advance.
  - 'B': 썬 워크샾 타입 허용 → 워크샾 부류(0x4000) 제작 수 > 0 이면 Advance 후 선택 해제.
  - 'C': 신전이 선택돼 있으면(선택 조회를 두 번 부름) 타이머가 0 일 때 지금 + 2초, 2초가 지났으면 `NotVortex` 섹션 Tell(열렸으면 임시 Gump 정리 뒤 이 미션의 정지) 후 선택 해제; 이어서 현재 생산 목록에 발사기(71)가 있으면 Advance.
  - 'D': 발사기 제작 수 > 0 이면 Advance. 'E': > 1 이면 Advance 후 선택 해제. 'F': 타이머가 0 이면 신전이 선택됐을 때 지금 + 4초, 아니면 시각이 지났을 때 Advance 후 선택 해제. 'G': > 3 이면 Advance, 선택 해제, 타이머 0.
  - 'H': 회수 금지를 0 으로 쓰고 타이머가 0 이면 회수 완료 명령(`event & 0x03ffffff`)이 왔을 때 지금 + 2초, 아니면 시각이 지났을 때 Advance.
- 선택 해제는 `resetSelection` 뒤 `clearSelection` 을 항상 둘 다 부르며 Advance 가 거부돼도 유지된다. 선택 SID 가 풀 밖이거나 TYPE 바이트가 표 밖이면 RangeError 로 거부한다.

## 2. 검증

`tests/tutorial-two.test.mjs`(3건)

- 원본 관측 재생: 346행 전부. 행마다 새 풀(SID 1 = 발사기 TYPE, SID 2 = 신전 TYPE)·제작 집계 셋·허용 표·게이트 14필드를 입력대로 구성하고 `RawMissionFrameGate`/`RawTotalMade` 를 실제로 써서 반환·게이트 비트·정지 깊이(unsigned DWORD)·효과 순서·옵션·회수 금지·허용 표·선택이 같은지 대조한다. 변이 4종(에너지 갱신 뒤 신전 제작 수 변경, Tell 뒤 생산 목록 변경, 선택 초기화 뒤 타이머 변경, Gump 정리 뒤 정지 깊이/표식 변경)을 포함한다.
- cpppj 계약 포팅 1건: 필수 효과 8개 각각의 누락과 다른 판본 자료(TYPE 171칸·옵션 4 BYTE)는 생성 때 거부, 알 수 없는 단계에서는 어떤 효과도 일어나지 않는다.
- 웹 계약 1건(원본 fixture 의 사건이 modifier 비트를 싣지 않는 경계): 회수 완료 명령에 상위 비트(0x04000000)가 섞여도 같은 명령으로 인정하고 다른 명령은 무시, 'F' 의 현재 선택 TYPE 바이트가 표 밖이면 RangeError.

최초 실패와 교정: 모듈은 처음부터 346행이 모두 통과했다. 모듈 사본을 바꿔 돌려 비교가 작동하는지 확인했다. 확인 뒤 원본으로 되돌렸다(diff 일치). 변형 40건 중 3건이 살아남아 2건은 웹 계약을 더해 잡았다.

- 실패한 변형: 부류·지연 상수, 생성 검사(옵션 크기·TYPE 수), 선택 조회 두 번 호출·첫 조회 게이트·부류 비트, 선택 해제의 두 호출과 순서, 'A'~'H' 의 모든 쓰기·임계값·비교·Tell/정지 순서·타이머 갱신·단계 문자와 반환값(각 1건 모두 실패).
- 적용되지만 통과한 변형(동등 변형): 선택 SID 의 풀 범위 검사 제거 — `SidPool.slot` 이 같은 RangeError 를 던진다.

## 3. 경계와 남은 것

- 실제 미션/세션에 연결하지 않았다. 옵션 표(0052f7e0)·회수 금지·타입 번호·회수 완료 명령 전역의 실제 소유자, 선택 조회(004d5430)·생산 목록(0044f970)·섹션 Tell(00482de0)·임시 Gump 정리(00466710) 등 실제 하위와 호출 지점(튜토리얼 2 프레임)은 순서4·6 몫이다(PORT_MAP session 0). `Advance`/`PauseForDialog` 의 실제 시계 정지·재개 연결도 마찬가지다.

## 4. 다음 시작점

WB04-E239 는 3.1절에서 고른다. 기준선 뒤 변경이 없는 단위(`RawTutorialLater`·`RawTerrainGrowth`·`RawColorMapCache`)를 권한다. `RawTutorialLater` 는 E234 `RawMissionStart` 와 E237 `RawTotalMade` 를 쓴다.
