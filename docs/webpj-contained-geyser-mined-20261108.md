# webpj 종속 조회/삭제·간헐천 채집 통지(RawContainedLookup·RawGeyserMined) 이식 — WB04-E108

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 백여덟 번째 부분이다. 타입과 순번으로 종속(contained) 객체를 찾거나 가상 삭제를 요청하는 조회 몸체(004b1a90)와, 간헐천 채집 통지 패킷(0x5b)의 처리기(004266c0: 소리·수량/cost 차감·고갈 삭제·emptyGeyser 생성)를 cpppj 기준선 `1f00a25a` 의 `o/RawContainedLookup`, `o/RawGeyserMined` 에서 옮겼다. 선행은 이미 이식된 `SquidHash`·`GameRandom`·`SidPool` 이다. 기준선 뒤 cpppj 변경이 없다. 웹은 10.78 전용이다.

## 1. 옮긴 것

`www/core/raw-contained-lookup.js` — `RawContainedLookup(pool, destroy)`, `makeContainedConstructionHooks(pool, lookup, hooks)`

- `find(parent, type, ordinal = 0, remove = false)`: parent 의 head WORD(+6)부터 next WORD(+4) 체인을 따라 TYPE BYTE(+10)가 type(DWORD 전체와 비교)과 같고 dead 비트(상태 +11 의 2)가 없는 항목 중 ordinal 번째(0부터)를 찾는다. free/void/contained 비트는 후보를 제외하지 않는다. `remove=false` 는 해당 SID(없으면 0)를 반환한다. `remove=true` 는 순번이 0 인 항목에서 서버 권한이거나 client 번호(5 이상 serverFirst 미만)일 때만 가상 삭제(`destroy(sid, 0)`)를 하고 **계속 진행**하여 0 을 반환한다(권한이 없어도 순번은 감소). 순번은 DWORD 로 감소하며 0 에서 감긴다.
- 범용 contained finder(`raw-contained-finder.js`)와 달리 **다음 링크는 삭제 효과가 끝난 뒤** 현재 슬롯에서 읽는다(삭제가 슬롯을 반납하거나 next 를 바꿀 수 있음). 범위 밖/순환 체인은 방문 시점에 거부하고 이미 실행한 삭제 효과는 되돌리지 않는다. 삭제 경계가 없으면 첫 조회 전에 거부한다.
- `makeContainedConstructionHooks`: 건설 완료의 `removeContained` 경계를 `find(parent, type, 0, true)` 로 연결한다(다른 풀의 lookup 은 거부, 나머지 hooks 유지).

`www/core/raw-geyser-mined.js` — `RawGeyserMined(pool, hash, types, random, state, hooks)`

- `handle(geyser)`: free 객체(상태 bit0)는 모든 효과를 건너뛴다. 아니면 게임 난수로 소리 번호 `(난수(6000)+1000)/1000`(1~6)을 정해 `geyserMined{번호}.wav` 를 현재 좌표에서 flags 0 으로 요청한다(소리마다 난수 정확히 한 번 전진). 소리 뒤 **현재** 수량 WORD(+12, unsigned)에서 현재 nugget 타입의 cost(float)를 뺀 값을 0 방향으로 절삭(signed64 하위 WORD, 불확정값은 0)해 수량으로 기록한다. 기록 뒤 다리(bridge) 수량 num assert(검사 전역이 켜졌고 abstract 비트가 없고 수량의 bit3..6 이 7 초과)가 위반이면 오류이다. 이어 현재 boss 이고 수량이 0 이면 현재 좌표를 보관해 간헐천을 삭제(flags 0x1000)하고, 삭제 뒤 그 좌표의 현재 표면(정수 칸의 hash0 머리 TYPE 이 isle 타입)이 아니면 emptyGeyser 를 생성(`create(type, 0)`)해 보관한 좌표로 배치(`pop(sid, x, y, 0)`)한다. 좌표 정수화는 ftol 하위 DWORD 이며 NaN/무한/범위 밖은 0, 256 밖 칸은 표면 없음이다.
- `handlePacket(packet)`: 패킷 +4 WORD 를 SID 로 읽어 `handle` 한다(6바이트 미만은 거부). 필수 효과(`sound·destroy·create·pop`)·188 TYPE 표 누락과 CD 판본은 raw 수량이나 난수를 바꾸기 전에 거부한다.

## 2. 검증

- `tests/contained-lookup.test.mjs`(2건): `contained-x86.tsv` 의 `originals` 805행(`originalCD` 805행·`original1037` 805행은 다른 판본이라 제외). 입력 6 값(서버 권한·체인 패턴 0~4·타입·순번·삭제 여부·삭제 콜백 변이 0~5)으로 반환 SID·가상 삭제 요청 순서(`D:SID:flags`)·부모와 일곱 종속 슬롯 전체 raw 가 같다. 계약: 범위 밖/순환 링크와 삭제 뒤 손상된 next 거부, 삭제 경계 누락·다른 월드의 건설 어댑터 연결 거부.
- `tests/geyser-mined.test.mjs`(2건): `geysermined-x86.tsv` 의 `originals` 1156행(`originalCD` 1156행은 제외). 입력 7 값(수량 WORD·cost 비트·난수 seed·free·boss·표면 타입·소리 콜백 변이 0~12[수량/좌표/boss/타입 변경, 좌표 NaN·무한·±2³², 다리 assert])으로 게임 난수 상태·효과 순서(`S` 소리 좌표 비트·번호, `D` 삭제, `C` 생성, `P` 배치, `A` assert)·간헐천 슬롯(6)과 생성물 슬롯(7) 50바이트(가상 표 DWORD 제외 전체)가 같다. 계약: 필수 효과/타입 표 누락·CD 판본·짧은 패킷 거부와, 소리 콜백이 boss 를 켜면 같은 호출에서 고갈 삭제·생성·배치가 일어남.

최초 실패와 교정: 새 모듈은 위 fixture 1961행과 계약 검사를 모두 첫 실행에 통과했다.

## 3. 경계와 남은 것

- `RawContainedLookup` 의 건설 완료 연결 시험(`ContainedLookup_ConstructionStopsActualSoundAndKeepsOtherProcess`)은 건설 프로세스/소리 프로세스/Kernel 이 이식 전이라 하지 않았다. `RawGeyserMined` 의 패킷 분배기 등록(`RegisterGeyserMined`)과 실제 factory/삭제/Pop/공간 연결 시험(`real_packet_factory_destroy_and_space`)은 `RawPacketDispatch` 이식 뒤에 한다(PORT_MAP 은 partial). 소리·가상 삭제·생성·Pop 은 훅 경계이다.

## 4. 다음 시작점

`RawLocalGeyser`(로컬 간헐천 생성), `RawPathProcess`(RunFrame·도착·Halt; 커널/Form 장면 선행)·`RawPathStart`·`RawPathData`·`RawPathCleanup`, `RawBalloonFrame`, `RawGunGroundTarget`·`RawGunProcess`(+`MakeGunTargetHooks`), Missile 계열, `RawPacketDispatch`(패킷 분배기), `RawTempleDraw`/캡처 계열(자산 시험 장치), `RawCarrierArrival`, `RawAssetPostPop`, 사제 배치 계열(선행 `RawCanonPixelShape`·`RawCanonPlacement`), 후보/지형 권한(`RawCanonPlacementPermission`), `RawSquidTransmission`, `RawDudeSpawn`·`RawDudeNotice`→`RawDudeDrop`, `RawManaBoltArrival`(시험 장면 선행), `RawIslandRuntime` 계열, 분산 SP 정책(`ReadPlayerSpAmount`/`SetPlayerSpAmount`), 그 밖의 작은 몸체이다.
