# webpj 운반체 도착(RawCarrierArrival) 이식 — WB04-E160

2026-11-09, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 백예순 번째 부분이다. 실제 Carrier(운반체) 도착(`00429800`)의 현재 경로/내용물/다이스/운반 분기와 끝의 회피 검색 순서를 옮긴 `o/RawCarrierArrival`(cpppj 기준선 `1f00a25a`, 기준선 뒤 변경 없음을 `git diff --numstat` 으로 확인)이다. 사제 도착(`RawPriestArrival`)의 일반 분기가 이 본문을 직접 호출하며, E159 `RawPathCleanup`(귀환 장소·근처 신전 조회)·E56 `RawCarrierPreDestroy`(내용물 조회)·E62 `RawPriestState`·E57 Player 기준점·E5x contained 생성·E39 `RawSquidNeighbors` 위에서 동작한다.

## 1. 옮긴 것

`www/core/raw-carrier-arrival.js`

- `createCarrierArrivalState()` — 권한(`boss`)·패치 Robust 검사(`debug`)·친구 사제 pickup 허용·원래 mutable 타입 getter 값(`priestType` 158·`nuggetType` 154·`daisType` 165·`geyserType` 122·`outpostType` 126·`isleStalagType` 84)·방향을 보존하는 9×9 동맹표(`alliances[운반체 소유자×9+후보 소유자]`)·독립 재생용 Player AI 존재(`controlledOwners[9]`)와 실제 세션의 공유 getter(`sharedAllowAlliedPickup`·`sharedGeyserType`·`sharedNuggetType`). `allowAlliedPickupNow`/`geyserTypeNow`/`nuggetTypeNow(state)` 가 선행 효과 뒤의 현재 값을 읽는다(capture DWORD 는 bool 로 alias 하지 않고 현재 비0 를 판정).
- `new RawCarrierArrival(pool, hash, types, carrier, cleanup, priest, anchor, create, footprint, state, hooks)` — `hooks = { path(sid), genusAt(x, y, genus), pickup, escape(sid, delay, event), chooseReturn, chooseGeyser, goReturn, goGeyser, minedPacket, destroy, robust, (선택) currentControlled(owner), currentAlliance(first, second) }`. 도달한 큰 운반/포획/경제/프로토콜/회피 몸체는 필수 실제 효과를 공급해야 하며 같은 풀/필수 경계가 없으면 첫 효과 전에 거부한다. CD(10.72) 전용 경제 경계(`cdAddSp`·`cdOwnerPacket`·`playAt`·`cdPriestPickupPriest`)는 옮기지 않았다.
- `arrival(sid, targetA, targetB)` — 운반체 genus 가 아니면 `isCarrier` assert 로 거부 → 사제 부모의 현재 경로 helper 가 있으면 종료(1000) → 소유자 0 이면 종료 → 현재 보조(너겟 검색·지면 `0x10000000`/`0x2000` genus·귀환 장소·포획한 사제·다이스 위치 검증·pickup 후보·현재 내용물)를 읽고, 경로 깊이가 0(또는 경로 없음)이면 pickup / 다이스(권한이 있으면 포획 사제를 동맹 다이스로 옮기고 지연 5 회피, 대체 1001) / 귀환(제어 소유자가 아니면 새 이동 요청) / 간헐천 분기를 수행한 뒤, 권한이 있으면 끝의 회피 검색(다른 운반체 → 지연 1·`0xffffffff`, 섬 종유석 → 지연 2·`0x1da1a`, 현재 위치/y+2 의 위험 지면 `0x4200`/`0x200`)을 실제 finder 순서로 한다. 모든 정상 반환 앞에 패치의 Robust 검사(`debug`)를 건다.
- `pickupCandidate(sid, x, y)` — `00426900`: 거의올림→단순 절삭 점 finder 에서 현재 사제 불가/소유자/동맹 설정 조건을 원본 순서로 읽어 `{ sid, flag }`. `validGeyser`/`validReturn` — `00427580`/`004274b0` 현재 대상 검증(패치의 `canProcessStormpower` assert 포함).

이 모듈은 DOM 을 쓰지 않으며 10.72(CD) 분기는 범위 밖이다. `genusAt`(`004b2070`, `RawDamageablePostDestroy` 의 공통 getter)은 아직 그 모듈이 이식되지 않아 훅으로 받는다.

## 2. 검증

`tests/carrier-arrival.test.mjs`(4건):

1. `carrierarrival-x86.tsv` 의 `originals` 203행(전체 406행 중 10.78) — 입력 JSON(권한·제어·깊이·genus·helper·소유자·경로 유무·회피 반환·장면 6종)으로 실제 PE 에서 실행한 부모 도착 반환값, 큰 회피 경계의 요청 기록(`E:` 인자 비트·사건·반환), 부모 슬롯 전체 raw 가 모두 같다. 선택하지 않은 큰 하위 효과(포획·전달·경제·프로토콜·공간)가 실행되면 실패하도록 모두 금지 경계로 두었고, 서로 다른 반환값이 관찰되었는지 센다.
2. 값형 Player AI 표 대신 실제 현재 Player 조회 getter(`currentControlled`)를 매 판정 읽는다: 생성 뒤 반대 값의 예전 bool 표를 넣고 getter 값을 바꿔도 같은 203행 결과가 나오며 getter 가 실제로 호출된다.
3. 같은 capture DWORD 의 0/음수 비0 가 건강한 동맹 사제 pickup 을 즉시 바꾼다(0 이면 후보 없음, −7 이면 후보 60).
4. 필수 하위 경계 누락·다른 풀 연결 거부, 운반체가 아닌 부모의 `isCarrier` assert.

최초 실패와 교정: 새 모듈은 203행 fixture 와 나머지 검사를 첫 실행에 통과했다.

## 3. 경계와 남은 것

- 사제 도착(`RawPriestArrival`)은 선행인 `RawIslandRuntime` 이식 뒤 이 본문을 연결한다. 큰 운반 하위 몸체(포획 `pickup`·귀환/간헐천 후보 선택·이동 요청·mined 패킷·삭제)는 각각 별도 이식된 모듈(`raw-carrier-pickup.js`·`raw-carrier-choice.js`·`raw-carrier-move.js` 등)이 있으며 실제 연결은 사제/운반체 걷기 어댑터 단위에서 한다.
- 10.72(CD)와 10.37 관측은 범위 밖이다.

## 4. 다음 시작점

`RawIslandRuntime`→`RawPriestArrival`, `RawPacketDispatch`→`RawConstructionCommand`, `RawBurningSources`와 `RawBurningSourceCount`, Missile Host 팩토리, `RawWindAviaryProcess`·`RawWindFlyer`, 번개 수명/프레임/화면 등 [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 0.1절 "마일스톤" 행의 목록을 따른다.
