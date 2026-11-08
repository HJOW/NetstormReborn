# webpj 마나 볼트 도착(RawManaBoltArrival) 이식 — WB04-E143

2026-11-09, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 백마흔세 번째 부분이다. 마나 볼트(타입 105)가 건설 대상 옆에 도착했을 때의 대상 검색(원본 `0045aab0`)과 도착 분기(`0045ab30`)를 옮긴 `o/RawManaBoltArrival`(cpppj 기준선 `1f00a25a`, 기준선 뒤 변경 없음을 `git diff --numstat` 으로 확인)이다. 사제 도착이나 teleport(타입 134) 도착(E133 `RawConstructionArrival`)과 다른 몸체이며 기존 `RawSquidFinder`·`SidPool`·`SquidHash` 위에서 돈다.

## 1. 옮긴 것

`www/core/raw-mana-bolt-arrival.js`

- `ManaBoltArrivalState` — `regularType`(00541128, 46)·`sharedRegularType`(00541164, 61)·`delay`(00532750, 0.05f)와 실제 Session 이 같은 장기 전역을 빌려주는 `currentRegularType`/`currentSharedRegularType` 함수. `regularTypeNow()`/`sharedRegularTypeNow()` 가 사용 위치마다 현재 값을 읽는다.
- `RawManaBoltArrival(pool, hash, types, state, hooks)` — 10.78 `SidPool`·타입 표 188칸을 확인한다. 선택된 위치의 효과 경계는 도달할 때 확인한다(없으면 생략하지 않고 거부).
  - `target(bolt)`: 볼트 현재 좌표를 `ftol` 한 칸 하나에서 일반 finder(표면 단계 생략 플래그 1)를 돌려 abstract 비트(+40 bit0)가 있고 preview 비트(bit3)가 없고 소유자(+34)가 볼트와 같은 첫 현재 후보를 돌려준다(없으면 0). dead/void/HP 필터는 없다.
  - `arrive(bolt)`: 볼트 좌표에서 `sunCannonRaiseLower5.wav` 를 한 번 재생하고, 대상이 있으면 일반 Regular(46)·공유 Regular(61)의 현재 번호로 프로세스를 차례로 제거하고 새 Regular 확보에 성공하면 event 1·payload 0.05f 로 부착한 뒤 1000, 없으면 부모를 플래그 0 으로 삭제하고 1001 을 반환한다.

## 2. 검증

`tests/mana-bolt-arrival.test.mjs`(3건):

1. `mana-bolt-arrival-x86.tsv` 101행(독립 PE 관측, 10.78) — 후보의 extra·상태·소유자와 새 Regular 확보 여부·대상 존재에서 `target` 결과·`arrive` 반환(1000/1001)과 효과 순서(S 소리 위치 비트/R 프로세스 제거/N 확보 시도/A Regular 부착/K 삭제)가 모두 같다.
2. 효과 경계가 없으면 거부(소리 → 삭제만 연결하면 대상 없는 도착은 성공), 판본(10.72)·타입 표 171칸 거부, `pool` 노출.
3. 첫 제거의 외부 효과 뒤 공유 번호를 다시 읽는다(46 제거 뒤 공유 번호가 99 로 바뀌면 둘째 제거는 99).

최초 실패와 교정: 새 모듈은 101행 fixture 를 첫 실행에 통과했다.

## 3. 경계와 남은 것

- 같은 cpppj 파일군의 `RawManaBoltRegular`(마나 볼트 주기 프레임·화면 위치, `0045a8e0`)는 시험이 클라이언트 투영(`SoundToScreen`·`RawCursorPosition`)을 요구해 이식하지 않았다. 해당 클라이언트 모듈 이식 뒤 옮긴다.
- 10.72(CD)의 다른 raw 폭은 범위 밖이다.

## 4. 다음 시작점

`RawPacketDispatch`→`RawConstructionCommand`, Canon/사제 배치 계열(`RawCanonPixelShape`·`RawCanonPlacement*`·`RawPriestPlacement*`), Missile Host 팩토리, `RawWindAviaryProcess`·`RawWindFlyer`, 번개 수명/프레임/화면, `RawPathProcess` 계열 등 [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 0.1절 "마일스톤" 행의 목록을 따른다.
