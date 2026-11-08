# webpj 건설 도착(RawConstructionArrival) 이식 — WB04-E133

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 백서른세 번째 부분이다. 건설 운반 객체(teleportEffect)가 목적지에 도착했을 때의 판정 — 서버가 확정한 건설 대상에서 실제 건설을 시작하고, 예측 대상만 보이면 대기하며, 그 밖에는 자기 자신을 지우는 몸체(원본 `00444080`) — 를 옮긴 `o/RawConstructionArrival`(cpppj 기준선 `1f00a25a`, 기준선 뒤 변경 없음)이다. 선행은 앞 단위(E132)의 `RawConstructionProcess.start`, 기존 `RawSquidFinder`·`SidPool`·`SquidHash`다.

## 1. 옮긴 것

`www/core/raw-construction-arrival.js`

- `ConstructionArrivalState` — 도착 판정이 호출 뒤에도 다시 읽는 원본 전역: `checking`(005e4794, 현재 DWORD 비영 뷰: 숫자·bool 또는 현재 값을 돌려주는 함수), `streakProcessType`(005410a0, 기본 12), `duration`(005325c0, float32 기본 0.6), `sharedStreakProcessType`(실제 Session 이 표시 생성/부착과 같은 전역을 빌려주는 함수). `readChecking()`·`currentStreakProcessType()` 은 사용 위치마다 값을 다시 읽는다.
- `RawConstructionArrival(pool, hash, types, state, hooks)` — 10.78 `SidPool`·타입 표 188칸·필수 하위 효과 세 개(`start`·`removeContained`·`destroy`)를 생성 때 확인한다. `types` 는 사본이 아니라 현재 표를 읽는다.
  - `arrive(builder)` (00444080): 도착 객체의 x/y 를 `ftol → float32 저장 → ftol`(비유한·64비트 범위 밖은 하위 0)해 정수 칸으로 만들고, 그 칸 하나짜리 사각형으로 실제 `RawSquidFinder` 를 시작해 끝까지 방문한다. 후보마다 현재 raw 를 다시 읽는다: extra(+40) 비트 0(abstract)이 없으면 건너뛰고, TYPE 이 타입 표 밖이면 거부하며, 타입 flags1 에 needsBuilder(0x8000)가 없으면 건너뛴다. 남은 후보는 "찾음"이며, 후보 번호가 5 미만이거나 서버 영역(15000 이상)이면 `start(후보, duration)` 으로 건설을 시작한다("시작함"). 시작 callback 이 바꾼 현재 raw 와 해시는 finder 가 미리 저장한 next 이후의 후보에 반영된다(사본·별도 후보 목록으로 대신하지 않음).
    순회 뒤: 시작하지 못했는데 후보만 찾았다면(예측 대상만 보임) `checking` 이 꺼져 있을 때 첫 streak contained 를 지우고(`removeContained(builder, 현재 streak TYPE)`, TYPE 은 삭제가 선택된 위치에서만 읽음) 1002(대기)를 반환한다. 그 밖에(서버 대상에 시작했거나 대상이 없음) 자기 삭제(`destroy(builder, 0)`) 뒤 1001(종료)을 반환하며, 삭제 뒤에는 객체를 다시 읽지 않는다.
- `makeConstructionArrivalHooks(pool, process, hooks)` — `start` 를 같은 풀의 실제 `RawConstructionProcess.start` 로 잇는다. 풀이 다르면 거부하고 나머지 경계는 그대로 둔다.

## 2. 검증

`tests/construction-arrival.test.mjs`(6건):

1. `constructionarrival-x86.tsv` 의 `originals` 153행(`originalCD` 153행은 10.72 라 제외) — 도착 객체 좌표·`checking`(0/1)·streak 타입(12/47)·지속 시간 비트·후보 0개(1행)·1개(128행)·3개(24행)(SID 60/14999/15000/15001/15002, TYPE·상태·abstract·좌표·발자국·flags1·해시 단계)·현재 자료 변형 6가지(`Mutate` 0 없음, 1 다른 서버 후보의 abstract 해제, 2 타입 flags1 해제, 3 시작한 후보의 next 변경, 4 다른 후보의 좌표 변경, 5 지속 시간 변경)에서 반환 코드(1001 131·1002 22)와 하위 효과 순서(`S` 시작[SID·지속 시간 비트]/`R` contained 삭제/`K` 자기 삭제)가 모두 같다.
2. 계약: 필수 하위 효과 세 개 각각의 누락·타입 표 171칸·전역 없음·다른 판본 거부, `pool` 노출.
3. 예측 대상만 있을 때 삭제가 도달한 위치에서 공유 streak 타입을 현재 값(56→57)으로 읽고(`R:50:56:0:1`→`R:50:57:0:1`), `checking`(함수 뷰)이 비영이면 삭제 없이 대기하며, 빌린 전역이 없으면 독립 입력의 12 를 쓴다.
4. 타입 표 밖 TYPE 의 abstract 후보는 효과 없이 거부한다.
5. 좌표 `ftol → float32 → ftol`: 음수·큰 값(1e12)·NaN·±무한대·float 가수 경계(16777217)에서 후보 없음 종료(`K:50:0`)로 끝나고 예외가 없다.
6. start 어댑터: 같은 풀의 프로세스에는 `start(15001, 0.6f)` 가 전달되고 다른 풀·없는 프로세스는 거부하며, 실제 `RawConstructionProcess`(attach 경계만 기록)에 이으면 도착 판정이 표시 효과 → 부착(flags 0x10) → 자기 삭제 순서로 실행된다.

최초 실패와 교정: 새 모듈은 153행 fixture 와 나머지 검사를 첫 실행에 통과했다.

## 3. 경계와 남은 것

- 마나볼트 도착(`RawManaBoltArrival`, 같은 cpppj 시험의 독립 101행)은 별도 단위다.
- 이 몸체를 부르는 PathProcess 의 도착 호출(parent 가상 +0x3c)과 경로 탐색·이동·재시도 분배는 경로 프로세스 단위에서 잇는다. 10.72(CD) 경로(extra +35, 타입 171)는 범위 밖이다.
- 건설 계열의 나머지(`Notice`·`Effects`·`Local`·`Confirm`·`Request`·`Clear`·`Data`·`Builder`·`Place`·`Command`·`Packet`·`Selection`)는 이 단위 뒤에 이어 옮긴다.

## 4. 다음 시작점

`RawConstructionNotice`·`RawConstructionEffects`(유닛 밀어내기 `displace` 는 E132 `makeConstructionProcessHooks` 의 `effects` 로 이어진다) 등 건설 계열, `RawManaBoltArrival`, Missile Host 팩토리, `RawWindAviaryProcess`·`RawWindFlyer`, 번개 수명/프레임/화면, `RawPathProcess` 계열 등 [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 0.1절 "마일스톤" 행의 목록을 따른다.
