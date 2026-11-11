# webpj Fort 저장 전역 묶음과 로더 상태 대여(FortSerializationGlobals · RawFortObjectLoad) — WB04-E279 (2026-11-11, 4-B)

3.1절 4-B 의 **FortSerializationGlobals**(11줄)를 `core/fort-serialization-globals.js` 로 옮기고, 이를 빌리는 **RawFortObjectLoad** 의 남은 부분(공유 상태 대여 생성자)을 `core/fort-object-load.js` 에 더해 두 항목을 함께 닫았다. 앞의 값형·stream 경로는 WB03-AB·E276 에서 옮겼고 이번 단위가 `RawFortObjectLoad` 의 마지막 남은 부분이다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다.

## 1. 기준선 뒤 변경 판정 (3.6절 ③ 4-B)

`git diff 1f00a25a HEAD -- cpppj/src/o/FortSerializationGlobals.h cpppj/src/o/RawFortObjectLoad.h` 는 두 헤더의 변경(커밋 `0d276d0d 20261108-235 CP Fort 저장 전역과 생산 타입의 장기 소유권 접합`)이다. `RawFortObjectLoad.cpp` 는 기준선 뒤 변경이 없다(3-E 판정 E259 의 "③ 4-B" 그대로).

- **`FortSerializationGlobals`:** 기준선은 `version` 한 칸이었고, 뒤에 `loadingDepth`(005c89b8)·`scope`(005c89b0)·`ownerSavedMask`(00542644, 초기 0x5d77cf00)·`battleSavedMask`(0054263c, 초기 0x40480002)가 붙어 다섯 DWORD 가 됐다. Session 교체/반납이 PE 초기값으로 덮지 않는다.
- **`FortObjectLoadState`:** 세 가지 생성자가 생겼다. ① `(version)` — 판본만 빌리고 깊이·scope·두 마스크는 로컬(기준선과 같은 독립 입력, 웹의 `createFortObjectLoadState(versionOwner)` 가 이미 같음), ② `(version, depth)` — 판본과 깊이만 명시 대여, ③ `(FortSerializationGlobals&)` — 다섯 DWORD 전부를 장기 owner 에서 빌린다. 복사·이동은 삭제된다(빌린 참조를 다른 수명에 남기지 않음). `battleSavedMask`(0054263c)는 기준선에도 값 필드로 있었고 웹 상태에는 없던 칸이다(소비자는 client 저장 경로).

## 2. 구현

- `fort-serialization-globals.js`: `FortSerializationGlobals` 클래스. 다섯 칸을 32비트 접근자로 두고(쓰기 때 `>>> 0`) 초기값 상수 `FORT_OWNER_SAVED_MASK_INITIAL`·`FORT_BATTLE_SAVED_MASK_INITIAL` 을 내보낸다. 읽기/쓰기는 항상 현재 값을 지나며 만들거나 빌릴 때 값을 되돌리지 않는다.
- `fort-object-load.js`:
  - `createFortObjectLoadState(versionOwner, depthOwner = null)` — 둘째 인자 `{ value }` 상자를 주면 `loadingDepth` 가 그 상자를 빌린다(접근자). 주지 않으면 이전과 같은 로컬 값이다. `battleSavedMask` 로컬 칸(초기 0x40480002)을 더했다. 상자 모양이 아니면 `TypeError`.
  - `createFortObjectLoadStateFromGlobals(globals)` — 판본은 묶음의 `version` 을 `{ value }` 처럼 보이는 접근자 상자로 넘겨 기존 판본 대여 경로를 그대로 쓰고, 나머지 네 칸은 묶음 칸에 접근자로 연결한다. 묶음이 `FortSerializationGlobals` 가 아니면 `TypeError`. 나머지 칸(다리/섬 타입·`ownerClearedMask`·`fortKeepMask`·특수 관문·청크 크기)은 독립 입력과 같은 로컬 값이다.
- 로더 본체(`FortObjectLoad`)는 바꾸지 않았다. 상태 객체의 접근자 속성만 지나므로 같은 코드가 독립/대여 상태 모두에서 동작한다.

## 3. 검사 (`tests/fort-serialization-globals.test.mjs` 6건)

- 묶음 초기값(0·0·0·0x5d77cf00·0x40480002)과 32비트 감김, 두 묶음의 독립.
- 전체 대여: 다섯 칸이 양방향으로 같은 칸이며(상태에 쓰면 묶음이, 묶음에 쓰면 상태가 바뀜) 만들 때 값을 되돌리지 않고, 두 상태가 한 묶음을 공유하며, 빌리지 않는 칸은 상태마다 로컬이다.
- 깊이 상자 대여: 깊이만 공유하고 scope/두 마스크는 로컬, 상자에 직접 쓴 음수도 DWORD 로 읽는다.
- 생성 계약: 묶음 종류·상자 모양 오류 거부.
- **원본 `fortobjectload-x86.tsv` 609행 재생** (기존 harness 에 `options.globals`/`options.depthOwner` 를 더함): 전체 대여/깊이 대여 상태로도 독립 상태와 같은 관측(효과·슬롯·난수·판본/깊이/scope/차단)이며 묶음/상자의 칸이 상태와 같은 값이다. 소유자 저장 마스크를 0 과 모든 비트로 바꿔 독립 상태에서 같은 값을 쓴 결과와 같은지 보았다(마스크가 결과를 바꾸는 입력이 있음을 단언해 연결을 구별).
- 변형 확인 20개 중 처음에 1개가 살아남았다: 깊이 getter 의 `>>> 0` 제거(상자에 직접 쓴 음수를 읽는 경계, 계약 검사를 보강해 잡음). 나머지 19개(대여 제거·칸 빠짐 4종·판본 접근자 고정/무시·setter 무시·전투 마스크 기본값·instanceof/모양 검사 제거·초기값 2종·5칸 감김 제거)는 모두 죽었다.

## 4. 남은 경계

- cpppj 의 `TemplateSerializationGlobalsTests`·`ClientRegionOwnerTests` 등은 `CampaignTemplateStorage`·`RawSurfaceWorld` 같은 client 소비자 시험이다. 웹은 해당 client 단위(4-B `RawSurfaceWorld`, 4-D `CampaignTemplateStorage`)를 이식할 때 이 묶음을 빌려 같은 단언으로 닫는다.
- 판본 상자(`versionOwner`)는 여전히 `{ value }` 이며, 장기 owner 가 `FortSerializationGlobals` 이면 `createFortObjectLoadStateFromGlobals` 를 쓴다. `fortKeepMask`(00542640)는 이 묶음에 없는 별도 전역이다.
- 3.1절 4-B 는 9/30 이다. 다음은 **Template**(partial 389줄)의 남은 `RefreshObjectRecords` 이다.
