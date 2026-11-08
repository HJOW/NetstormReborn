# webpj 사제 도착(RawPriestArrival) 이식 — WB04-E162

2026-11-09, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 백예순두 번째 부분이다. 사제 실제 `+3c`(도착, 패치 `00491f70`)의 중립 회복 — 현재 섬 Temple 근처의 저 HP 중립 사제를 Temple 의 내용물로 되돌리고 자기 삭제, 절반 이상 HP 중립 사제의 귀환 — 와 일반 Carrier 도착(E160) 위임을 옮긴 `o/RawPriestArrival`(cpppj 기준선 `1f00a25a`, 기준선 뒤 변경 없음을 `git diff --numstat` 으로 확인)이다. 선행은 E160 `RawCarrierArrival`·E161 `RawIslandRuntime`·E148 `RawCanonPlacementTerrain`(지역 getter)·E62 `RawPriestState`·`SquidReward`·E39 `RawSquidNeighbors`·contained 생성이다.

## 1. 옮긴 것

`www/core/raw-priest-arrival.js`

- `createPriestArrivalState()` — `{ boss(00540bc4, 참), loading(005c89b8), suppressNeutralReturn(00594fa4) }`. 하위 효과 뒤에도 매번 다시 읽는다.
- `new RawPriestArrival(pool, priest, hitPoints, terrain, islands, footprint, create, carrier, state, hooks)` — 모든 참조는 같은 풀의 기존 모듈이며(다르면 첫 효과 전에 거부), `hooks = { destroy(sid, flags)(실제 사제 가상 +10 자기 삭제), neutralReturn(sid)(00491db0 귀환 몸체) }` 둘 다 필수이다(빈 효과로 완료 처리하지 않음).
- `arrival(sid, targetA, targetB)` — 권한이 있고 현재 HP 가 최대 HP 미만인 중립(loading/억제 아님, 소유자 0) 사제가 현재 지면의 섬 Temple 근처(발자국 거리 ≤ 1.0)이면서 Temple 소유자가 같으면 Temple 에 같은 타입/WORD 의 내용물(mask 32)을 만들고 자기 삭제한 뒤 1001 을 돌려준다. 아니고 권한이 있는 중립 사제의 현재 HP 가 최대 HP/2(0 쪽 절삭) 이상이면 귀환 몸체를 실행하고 1001. 그 밖은 `RawCarrierArrival.arrival` 로 위임(인자 그대로).
- `templeAt(x, y)` — `0046f950`: 현재 plain 좌표(0 방향 절삭, 정상 signed32 밖은 거부)의 지역(sentinel 127 이면 Temple 0)→현재 섬 Temple DWORD(풀 용량 이상이면 거부).
- 거리 helper(`0041d840`) — 축별 signed 거리(좌표가 발자국 왼쪽/위 밖이면 경계−좌표, 아니면 좌표−오른쪽/아래)의 큰 쪽과 `1.0` 의 포함 비교. x 는 단정도로 저장하고 y 는 배정도이며, 발자국 안의 음수 거리도 Euclid 길이로 바꾸지 않고 유지한다.

이 모듈은 DOM 을 쓰지 않으며 10.72(CD)의 float 거리 저장 차이는 범위 밖이다.

## 2. 검증

`tests/priest-arrival.test.mjs`(5건). 이 몸체에는 독립 원본 관측 fixture 가 없다(cpppj 시험은 실제 경로 프로세스/Host 전체 통합 한 건이며 그 사슬은 경로 프로세스 단위에서 다룬다). 그래서 같은 풀의 하위 조회(현재/최대 HP·지역·섬 Temple·발자국)와 큰 효과(내용물 생성·자기 삭제·귀환·Carrier 도착)를 기록 경계로 두고 원본 분기 순서/값을 직접 검증했다.

1. 저 HP 중립의 섬 복귀: 지역 조회 → 섬 Temple 조회 → 내용물 생성(Temple, 타입 158, WORD `0x8101`, mask 32) → 자기 삭제 → 1001 의 정확한 순서. 최대 HP 미만이면 절반 이상이어도 먼저 섬 복귀를 본다.
2. 섬 복귀 조건 불충족(Temple 없음/멀다/소유자 다름) → 절반 이상은 귀환(1001), 절반 미만은 Carrier 위임; 최대 HP 와 같으면 귀환.
3. 권한 없음·로딩·억제·소유자 있음은 항상 Carrier 위임(인자 `0x1234`/`0x5678` 그대로), 하위 효과 뒤 권한 변경을 현재 값으로 다시 읽음.
4. 거리 helper: 발자국 오른쪽 정확히 1.0 은 가깝고(포함) 1.5 는 멀며, 발자국 안(음수)과 아래쪽 경계는 가깝고 두 축이 모두 멀면 큰 쪽이 기준.
5. 지역 127 → Temple 0, Temple DWORD 가 풀 용량 이상/비유한·범위 밖 좌표/다른 풀 하위 모듈/누락 경계 거부.

최초 실패와 교정: 새 모듈은 첫 실행에 5건 모두 통과했다.

## 3. 경계와 남은 것

- 실제 경로 프로세스(`RawPathProcess`)·사제 postPop·Host 를 한 장면에 연결하는 cpppj 통합 시험(`rawPriestArrival_real_path_step_parent_halt_arrival_cleanup_same_host`)은 경로 프로세스 본체 이식 단위에서 다룬다. 이 단위의 `destroy`/`neutralReturn`(`00491db0`)은 호출자가 실제 효과를 공급하는 경계이다.
- 10.72(CD)와 10.37 관측은 범위 밖이다.

## 4. 다음 시작점

`RawPacketDispatch`→`RawConstructionCommand`, `RawBurningSources`와 `RawBurningSourceCount`, Missile Host 팩토리, `RawWindAviaryProcess`·`RawWindFlyer`, 번개 수명/프레임/화면, `RawPathProcess` 계열 등 [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 0.1절 "마일스톤" 행의 목록을 따른다.
