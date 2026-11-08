# webpj 내용물 검색기·Carrier 삭제 준비·Player 기술 목록(RawContainedFinder·RawCarrierPreDestroy·RawPlayerTechnology) 이식 — WB04-E56

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 쉰여섯 번째 부분이다. cpppj 기준선 `1f00a25a`의 `o/RawPlayerTechnology`(원본 00490330 기술 조회, 004903b0 전체 삭제, 00490420 보장 생성, 004914e0 목록 한 반복, 004915c0 목록 추가)를 옮기면서, 그 필수 공유 장치인 `o/RawContainedFinder`(004b2270 Begin, 004b1b20 Next, 004acc60 CountContained)와 `o/RawCarrierPreDestroy`(00426890 Carrier 삭제 준비, 004ac9f0 FindContained)를 먼저 옮겼다(0.2절 4: 필요한 공유 장치가 없으면 먼저 이식). 세 파일 모두 기준선 뒤 cpppj 변경이 없다. 웹은 10.78 전용이라 CD 10.72 분기(인라인 finder·dead 진단 없음)는 옮기지 않았다.

## 1. 옮긴 것

`www/core/raw-contained-finder.js` — `ContainedFinderState`, `RawContainedFinder(pool, state)`, `countContained(pool, state, parent, mask, kind)`

- `ContainedFinderState`: boss·containedType(DWORD)·checkingDead·assertion 과 실제 Client 의 공유 boss(`bindBoss({value})`)·checking getter(`bindChecking`). 현재 값은 `currentBoss`/`currentChecking` 으로 소비 위치마다 다시 읽는다(cpppj `RawCheckingFlag`·`sharedBoss` 대응).
- `begin(parent, allowDead)`: checking 비영·부모 dead·boss 아님·dead 허용 아님이면 진단(Squidfinder.cpp 0x13c, 장치 없으면 명시 오류) → 복귀 뒤 부모 head WORD 를 처음 읽는다. `next()`: 후보 next WORD 를 반환 전에 저장하고 매 후보 현재 contained 타입과 TYPE BYTE 를 DWORD 그대로 비교한다. free/dead/void/contained 는 거르지 않고, 범위 밖·순환은 방문 시점에 오류다.
- `countContained`: kind(+18, DWORD 비교라 65536 을 0 으로 줄이지 않음)·mask(+20) 조건의 전체 개수(signed).

`www/core/raw-carrier-pre-destroy.js` — `RawCarrierPreDestroy(pool, state, hooks)`, `makeCarrierPreDestroyHooks`

- `validate`(할당 슬롯·free/contained 아님·자산 TYPE 70~187·하위 Damageable 사전 검사), `preDestroy`(검사 → Damageable 삭제 준비 → 그 효과 뒤 현재 boss 재조회 → `findContained(1,0,0)` 이면 전역 후처리), `findContained(parent, mask, kind, index, allowDead)`(공용 커서로 kind/mask WORD 조건·index 건너뛰기). 세 하위 효과는 생성 때 요구한다.
- `makeCarrierPreDestroyHooks`: 사제 삭제 준비 훅의 `validateCarrier`/`carrierPre` 만 같은 풀의 실제 몸체로 바꾼다(사제 삭제 준비 이식 때 소비).

`www/core/raw-player-technology.js` — `RawPlayerTechnology(player, finder, create, types, hooks)`, `PlayerTechnologyList`, `bindPlayerTechnologyTypes`

- `find`: 타입 범위 진단(0x363, 복귀 뒤 계속) → `RawPlayerWorld.technology` 의 PlayerKnown 진단·부모 SID 범위 → `findContained(4, type, 0)`. `clear`: 부모 한 번 캡처 → flags 4 내용물을 매회 처음부터 다시 찾아 가상 삭제(flags 0). `ensure`: 진단(0x376) → 중첩 find → 없으면 부모 재조회 → `RawContainedCreate.create(부모, type, 0, 4, 0x10, 0)`.
- `parseStep`: 빈 항목도 이름 조회를 한 번 하고, `all`(ASCII 대소문자 무시)이면 0번부터 group 10 이 아닌 TYPE 을, 조회 번호가 양수면 그것을 용량 40·중복 제거로 넣는다. 미등록 비빈 항목은 index 를 올리지 않아 Stalled. `addList`: 목록을 끝까지 만든 뒤 ensure 하며 무진행은 생성 전에 명시 오류.
- `bindPlayerTechnologyTypes(records)`: 원본 `RiftTypeTable::Find` 처럼 번호순 TYPE 레코드(프로세스·내장·자산 이름)의 첫 ASCII 대소문자 무시 일치.

## 2. 검증

- `tests/carrier-pre-destroy.test.mjs`(4건): `carrierpredestroy-x86.tsv` 10.78 960행(직접 조회 Q 480·Carrier 전체 P 480: 반환·사건·boss·다섯 슬롯 전체 바이트). 계약: 하위 경계 누락·잘못된 SID/상태/TYPE·Damageable 사전 검사 실패의 무효과 거부, SID 0 head, 순환/범위 밖 체인, 10.78 dead 진단, 사제 훅 연결, 공유 boss 재조회(cpppj 두 계약의 10.78 부분). 웹 보강: 진단 복귀 뒤 head 재조회·dead 허용·countContained 의 DWORD kind.
- `tests/player-technology.test.mjs`(3건): `player-technology-x86.tsv` 158행(find 56·ensure 56·clear 16·add 18·parse 9·stalled 3). 실제 `RawPlayerWorld`·`RawCarrierPreDestroy`·`RawContainedCreate`(SquidFactory 내장 contained 생성자)로 반환·외부 경계 순서·목록·관찰 다섯 슬롯·생성 슬롯·기술 부모를 비교한다. 계약: 실제 TYPE 이름 바인딩(`player-scene-support.mjs` 의 실제 레코드, `sunWalker;windBattery;sunCannon` 3개)·무진행 생성 전 거부. 웹 보강: 같은 풀·이름 조회·TYPE 표 계약과 `all` 의 40개 상한.

최초 실패와 교정: 세 모듈 모두 원본 1118행 전부를 첫 실행에 통과했다.

## 3. 경계

- 실제 Damageable 삭제 준비(0044b4b0)·전역 후처리(00485e40)·가상 삭제(가상 +10)·사제 삭제 준비 연결은 각 이식 단위와 월드 조립에서 공급한다.
- `ContainedFinderState` 의 공유 boss/checking 은 WB05 에서 실제 Client 권한(`RawAuthorityState`)·checking 소유자에 연결한다.

## 4. 다음 시작점

WB04-E57: `RawPlayerGeyserBounds`(0048f5c0, fixture `player-geyser-bounds` 439행; CanonDecoder 의 발자국 범위 00425b90 를 `territory.js` 에 보강)와 `RawPlayerPlacementAnchor`(0048fdb0, `playeranchor` 10.78 1598행; 이번 `RawContainedFinder` 사용). 그 뒤 종류별 PostPop 본체·`SquidDisplay`/`SquidFrame` 이다.
