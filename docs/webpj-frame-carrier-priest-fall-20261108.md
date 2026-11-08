# webpj 프레임 진행·Carrier 지면 조회·사제 낙하(RawFrameAdvance·RawCarrierCheck·RawPriestFall) 이식 — WB04-E65

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 예순다섯 번째 부분이다. 사제 postPop 후반(E64)이 훅으로만 받던 세 경계 — 공통 프레임 진행(원본 004afc90), Carrier 가상 `+0xcc` 지면 조회(00426fc0), 사제 낙하 요청과 `0x25b` 이벤트(004941f0·00494580→00493e60) — 를 cpppj 기준선 `1f00a25a` 의 `o/RawFrameAdvance`·`o/RawCarrierCheck`·`o/RawPriestFall` 에서 옮겼다. 기준선 뒤 cpppj 변경이 없다. 웹은 10.78 전용이라 CD 10.72 분기(BYTE 프레임, inline 지도, J bool helper)와 10.37 행은 옮기지 않았다.

## 1. 옮긴 것

`www/core/raw-frame-advance.js` — `RawFrameAdvance(pool, frames, setter)`

- `advance(sid, delta, flags)`: 현재 프레임 DWORD 의 side 글자 구간(첫 번호 `first`, 개수 `count`)에서 `relative = current + delta - first` 를 DWORD 로 계산하고, signed 로 `count` 이상이면 `count` 를 빼고 음수이면 `count` 를 더한다(보정은 한 번뿐, modulo·반복 없음). `first + relative` 를 flags 와 함께 실제 `SquidFrame.set`(E59)에 넘기고 보정 발생 여부를 반환한다. 32비트 감김은 `| 0`/`>>> 0` 으로 보존한다. `setter` 는 같은 풀이어야 하고 `frames` 는 타입별 `FrameCodeTable` 표(참조)이다.

`www/core/raw-carrier-check.js` — `RawCarrierCheck(pool, types, spots, fallWalker)`, `makeCarrierCheckPostPopHooks`, `makeCarrierCheckSpawnHooks`

- `check(sid)`: 현재 TYPE 의 genus(flags2)에 `0x320000` 비트가 있으면 좌표·낙하를 읽지 않고 거짓(사제의 실제 genus `0x210000` 포함) → 좌표에 float32 `0.9999` 를 더해 절삭한 칸(y 먼저)의 spot & 6(지면)이 있으면 거짓 → 그 밖에는 가상 `+0xc8`(walker 낙하)의 0 아님을 반환한다. 상태·HP·free·dead·void·extra 비트는 조회 조건이 아니다. 지도 밖이거나 비유한 좌표는 변경 없는 진단으로 거부한다.
- `makeCarrierCheckPostPopHooks`/`makeCarrierCheckSpawnHooks`: Carrier postPop 의 `+0xcc` 와 사제 나선 생성의 `+0xcc` 를 같은 실제 조회로 연결하고, 반환값은 원본처럼 버린다. 다른 풀은 거부한다.

`www/core/raw-priest-fall.js` — `PriestFallState`, `RawPriestFall(pool, priest, frames, choices, surfaces, state, hooks)`, `makePriestFallHandler`, `makePriestFallWalker`, `makePriestFallHooks`, `makePriestFallFrameHooks`

- `begin(sid)`(004941f0): 보호막 확보 → 예약(`new(0x28)`)이 성공하면 공유 Regular(사건 `0x25b`, payload 0.01) 등록 → 그 시점의 현재 권한(보호막·생성 효과 뒤 다시 읽음)이면 Unpop(0)/Repop(0x800) → 현재 좌표의 소리 요청. 기존 낙하 예약 중복은 검색하지 않는다.
- `tryFall(sid)`(사제 vtable `+0xc8`, 00494e80): 현재 signed 방향이 J(9)가 아닐 때만 `begin` 을 부르고 항상 참을 반환한다.
- `handle(sid, 0x25b)`(00494580→00493e60): 사제가 이동 불가이면 보호막 확보 → 방향이 J 이면 프레임 진행(+1) 아니면 낙하 프레임 지정 → 지면(거의 올림 좌표의 표면 지도 WORD ≠ 0, 지도 밖은 없음)이 없으면 `0.1` 반환(재예약), 있으면 도착 프레임 지정 → 이동 가능해지면 보호막 해제 → 0 반환. count·payload 는 읽지 않는다.
- `makePriestFallHandler`/`makePriestFallWalker`: 실제 사제 vtable(`0x50f210`)의 `0x25b`/`+0xc8` 만 처리하고, 다른 사건·가상 표는 명시 fallback 으로 넘기며 fallback 이 없으면 사제의 미복원 사건은 거부, 사제가 아닌 Regular 는 payload 를 그대로 돌려준다.
- `makePriestFallHooks(host, shield, lookup, destroy, hooks)`: 보호막 확보를 E63 `RawPriestShield.ensure`, 공유 Regular 를 `SquidProcessHost.addSharedRegular`, 보호막 해제를 `RawPriestForcefield.find` 결과의 삭제 요청으로 연결한다. `makePriestFallFrameHooks(setter, advance, hooks)`: 낙하/도착 프레임 지정과 J 진행을 같은 실제 `SquidFrame`/`RawFrameAdvance` 에 연결한다.

## 2. 검증

- `tests/frame-advance.test.mjs`(3건): `frameadvance-x86.tsv` 10.78 7680행(코드표 종류·타입 종류·해시 단계·현재 프레임·signed 증분·flags → 보정 여부, 실제 `SquidFrame` 의 표시/Unpop/Pop 호출 기록과 슬롯 50바이트), 실제 `SquidDisplay`/`SquidPop`/`SquidUnpop` 로 한 호출 늦은 해시 이동과 반환(`FrameAdvance_ComposesRealDisplayAndSpace`), 풀/지정기 불일치·빈 표 거부와 타입 범위 오류의 무변경 거부.
- `tests/carrier-check.test.mjs`(3건): `carriercheck-x86.tsv` 10.78 676행(Check/Try 의 genus·지면·프레임 방향·권한·할당·변경 단계 → 반환·호출 기록·슬롯·현재 권한), 같은 `+0xc8` 이 실제 공유 Form/Kernel 에서 재예약되고 Carrier postPop 이 조회 반환을 버리는 합성, 현재 표 재조회·제외 genus 의 좌표 생략·같은 풀 연결 거부.
- `tests/priest-fall.test.mjs`(5건): `priestfall-x86.tsv` 10.78 456행(Begin/Handle 의 호출 기록·슬롯·반환 비트·현재 권한), 실제 공유 Form/Kernel 재예약·삭제와 실제 보호막 생성/조회/해제의 합성(`PriestFall_ComposesSharedProcessAndShieldLifetime`), 프로세스 예약 효과가 권한을 바꾸는 경우의 재조회, 낙하 방향 반복 억제와 분배기 fallback/미복원 사건 거부, 잘못된 풀·표·미연결 훅과 프레임 훅의 같은 지정기 검사.

최초 실패와 교정: 새 모듈은 위 fixture 8812행 전부를 첫 실행에 통과했다.

## 3. 경계와 남은 것

- 표면 삭제→finder→사제 가상 낙하의 합성(cpppj `CarrierCheck_ComposesSurfaceFallAndProcessScheduling` 의 `RawIslandLifecycle` 부분)과 사제 나선 생성(`RawPriestSpawn`) 쪽 조회는 해당 파일의 이식 단위에서 대조한다. 이 문서의 합성 검사는 같은 가상 `+0xc8` 을 직접 호출해 공유 Regular 재예약과 Carrier postPop 연결을 확인한다.
- 회복 분배기(`RawPriestRegen`)와의 합성은 해당 파일을 옮길 때 대조한다. 분배기의 fallback 연결 계약은 `makePriestFallHandler` 의 fallback 인자로 확인했다.
- E64 의 `RawPriestPostPopTail` 에 실제 낙하·보호막 훅을 붙인 통합 장면(회복 Form/Kernel 포함)과 앱 수명 연결은 WB05 월드 조립에서 한다.

## 4. 다음 시작점

사제 배치 계열(`RawPriestPlacement*`·`RawPriestSpawn`), `RawAssetPostPop`(Gun·제네레이터·거주 구역·바리케이드·간헐천·신전 분배), 후보/지형 권한(`RawCanonPlacementPermission`), `RawSquidTransmission` 이다.
