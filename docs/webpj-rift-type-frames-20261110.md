# webpj 프레임 코드 표 검색 — WB04-E266 (2026-11-10, 4-A 두 번째 항목 RiftType)

3.1절 4-A 의 **RiftType** 을 옮겼다. cpppj `RiftType.cpp` 는 `RiftTypeFrames`(프레임 코드 배열)의 검색 세 가지와 글자 구간·종류 수를 가진다. 웹 `FrameCodeTable`(`www/assets/terrain-tiles.js`)은 `run`/`candidates` 만 있었고 코드에 번호를 보관하지 않았다. 기준선 판(`git show 1f00a25a:cpppj/src/o/RiftType.cpp`)을 그대로 옮겼다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다.

## 1. 옮긴 것

| cpppj | 웹 | 원문 |
| --- | --- | --- |
| `FrameCode` 4바이트(방향·변형·번호·플래그) | `FrameCodeTable.codes[i] = { side, variant, number, flags }` (번호 미지정 입력은 0) | 원본 코드 +0..+3 |
| `FindNumber(side, variant, number)` | `findNumber` | 0049a9a0(B)/CD 00444350: 방향·변형·번호가 모두 같은 첫 코드, 없으면 -1 |
| `FindMasked(side, variant, number, mask)` | `findMasked` | 0049a9e0(B)/CD 00444410: 위 조건에 더해 플래그를 signed 8비트 → 32비트로 부호 확장해 DWORD 마스크와 AND 한 값이 비영 |
| `FindFlags(side, variant, flags)` | `findFlags` | 0049aa30(A)/CD 00444460(A): 방향·변형과 부호 확장한 플래그가 정확히 같은 첫 코드(번호는 조건이 아님) |
| `LetterKinds()` | `letterKinds` | 글자 'A'~'P' 중 구간이 하나라도 있는 글자 수(원본 +0x12c) |
| `Run(letter)`·`Codes()` | 기존 `run`·`codes` | 변경 없음 |

인자는 BYTE(`& 0xff`)로 줄여 비교하고 플래그 마스크는 DWORD(`>>> 0`)로, 정확 비교 플래그는 int32 로 읽는다.

## 2. 검사 (`tests/rift-type-frames.test.mjs` 5건)

- `original-x86.tsv` 의 **FrameFindNumber·FrameFindMasked·FrameFindFlags 각 512행(1536행)**: 원본 기계어가 낸 반환 위치와 같다. 같은 파일의 Config 270행은 4-A Config 항목이 읽는다. 첫 실행에 통과했다.
- 정의에서 만든 표가 번호를 보관하고 중복 코드는 먼저 선언된 위치를 돌려준다(AssetTests 대응).
- 플래그 부호 확장(0x81 = -127, 상위 비트가 마스크 0x100 에도 걸림)·마스크 비영·정확 비교·BYTE 축소, `letterKinds`.
- 변형 확인(9개 중 8 탐지, 살아남은 1개 — `findNumber` 의 변형 비교 제거 — 는 원본 512행이 방향이 같고 변형만 다른 중복을 담지 않아 구별하지 못했다): 방향이 같고 변형만 다른 코드를 구별하는 시험을 더해 닫았다. 처음 쓴 부호 확장 시험은 `0x81` 이 `0x100` 마스크에 걸리는 점(부호 확장이 상위 비트를 세움)을 놓쳐 기대값이 틀렸고, 시험 쪽을 고쳤다(모듈 결함이 아님).

## 3. 대응표·남은 경계

- `PORT_MAP.tsv` `RiftType.cpp`: partial → **port**(웹 `type-table.js`·`terrain-tiles.js`). 소비자(RawBridgeEvents 훅·IslandBuilder·InventoryItemGump)가 이 검색을 부르는 연결은 해당 단위가 옮길 때 한다.
- manifest 에 `original-x86.tsv` 항목(`originalx86`)을 더했다(FrameFind 1536행 사용, Config 270행은 다음 4-A Config 항목).
- 다음은 4-A **TypeParser**(`.type` 정의 해석의 나머지)이다.
