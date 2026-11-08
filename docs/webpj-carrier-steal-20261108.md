# webpj 크리스탈 크랩 포획(RawCarrierSteal) 이식 — WB04-E92

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 아흔두 번째 부분이다. 크리스탈 크랩(Carrier)이 다른 운반체의 내용물을 포획하는 본문(`o/RawCarrierSteal`: 429ed0 Try·42ab80 Halt·42abb0 PostPop)과 별도 60h 수신(4267c0)을 cpppj 기준선 `1f00a25a` 에서 옮겼다. 기준선 뒤 cpppj 변경이 없다. 웹은 10.78 전용이다(10.72 의 다른 포획 본문은 같다고 추정하지 않음).

## 1. 옮긴 것

`www/core/raw-carrier-steal.js` — `RawCarrierSteal(pool, hash, types, boss, editor, localOwner, alliances, carrierType, priestType, header, haltCounter, hooks)`

- 전역 `boss`/`editor`/`localOwner`/`carrierType`/`priestType` 은 호출 위치마다 읽는 getter, `haltCounter` 는 `{ value }`, `header` 는 `{ message, protocol, length }`, `alliances` 는 81 DWORD 이다.
- `try(parent)`(429ed0): boss 가 아니거나 부모에 이미 내용물이 있거나(`find` mask 0x20) 편집 모드(조회 뒤 읽음)면 10. 아니면 부모 좌표 칸(signed64 절삭의 하위 DWORD)의 일반 finder(flags 0)를 현재 체인 순서로 순회해 기증자 flags2 가 0x210000 이고 역방향 동맹 `alliances[기증자 소유자×9 + 부모 소유자]` 가 0 이며 크랩 타입이 아닌 후보의 내용물(`find` mask 0x20)을 찾는다. 내용물이 사제 타입이고 내용물 소유자(low7)가 부모 소유자면 건너뛴다. 첫 적합 내용물은 10바이트 명령 `[프로토콜&0x7f, 메시지, 길이(2), 부모(2), 내용물(2), 기증자(2)]` 로 로컬 수신 → 송신(`broadcast`) → 기증자 이동 제거(`movementType` → `removeMovement`) → 내용물 TYPE flags2 에 0x8000 이면 귀환(`goHome`) 뒤 12 를 반환하고 성공 뒤 다음 후보를 읽지 않는다. 없으면 10.
- `halt(parent)`(42ab80): 모든 크랩이 공유하는 카운터(부모별 타이머가 아님)를 증가(DWORD 감김)시켜 signed % 4 로 만들고 0 이면 `try`, 아니면 공통 정지 검사. `postPop(parent, flags)`(42abb0): extra&9 가 없고 flags&1 이며 Regular 할당이 되면 0x492 예약, flags&1 이 없고 boss 가 아니면 운반체 검사, 마지막에 Damageable PostPop 한 번. `receive(owner, flags, packet)`(4267c0): 8바이트 미만 거부, 내용물 또는 새 부모(패킷을 효과마다 다시 읽음)가 free 면 반환, 아니면 분리 → 부착 → 미리보기 → `crabSteal.wav`. `handles(parent)`: 가상 표 0x503f20(크랩)·0x503ff0(예약 187).

## 2. 검증

- `tests/carrier-steal.test.mjs`(2건): `carrier-steal-x86.tsv` 1105행(포획/수신/정지/PostPop 네 종류, boss·편집·내용물 존재·체인 길이·기증자 flags2·동맹 방향·할당 성공, 13종 외부 변이: 편집/사제 타입/free 로 바뀜/기증자 종류 변경/boss·extra 변경/수신 패킷 부모 변경 등)의 효과 순서·결과·boss/편집/사제 타입/정지 카운터·네 슬롯 전체 raw·송신 패킷 hex 가 같다. 계약: 비권한 조기 종료와 도달한 누락 경계 구분, 공유 정지 카운터(부모 무관, 4번째 호출이 `try`), 두 가상 표 분배, 짧은 명령·CD 풀·getter 아님 거부.

최초 실패와 교정: 새 모듈은 위 fixture 1105행과 계약 검사를 모두 첫 실행에 통과했다.

## 3. 경계와 남은 것

- 공통 정지 검사(42a5f0)·Regular 생성자(496e80)·운반체 검사·내용물 분리/부착(+48/+50)·미리보기(426080)·송신(4e2e30)·이동 제거(4b1a90)·귀환(428fd0)의 실제 본체 연결은 해당 단위에서 한다.

## 4. 다음 시작점

`RawCarrierDrop`, `RawGunGroundTarget`·`RawGunProcess`(+`MakeGunTargetHooks`), Missile 계열, `RawTempleDraw`/캡처 계열(자산 시험 장치), `RawCarrierArrival`, `RawAssetPostPop`, 사제 배치 계열(선행 `RawCanonPixelShape`·`RawCanonPlacement`), 후보/지형 권한(`RawCanonPlacementPermission`), `RawSquidTransmission`, `RawDudeSpawn`·`RawDudeNotice`→`RawDudeDrop`, `RawManaBoltArrival`(시험 장면 선행), `RawIslandRuntime` 계열, 그 밖의 작은 몸체이다.
