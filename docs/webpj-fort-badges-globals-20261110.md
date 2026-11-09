# webpj Badges 전역 장부(RawFortBadgesGlobals) 이식 — WB04-E225

2026-11-10, HJOW-Athlon. 미션/Fort 파일의 Badges 섹션 로더·조회·저장이 함께 빌리는 원본 전역 장부(28개 기록, 6160 BYTE)의 CRT 구성(004fcbe0)과 현재 수량 reset(0047d860), 그리고 장부를 한 번만 소유하는 `RawFortBadgesGlobals` 를 옮겼다. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.1절의 "순서4 이후로 넘기는 `o/` 단위(Fort 적재·지형/섬 …)"에 속하며 기준선 뒤 변경이 없고 원본 관측이 있어 E224 섬 받침 삭제 훅에 이어 옮겼다. 다음 Fort 적재 단위(`RawFortMetaLoad`)가 이 장부를 빌린다.

- cpppj 기준선 `1f00a25a`. 소스(`RawFortBadgesGlobals.cpp/.h`)·시험(`FortBadgesGlobalsTests`)·fixture(`fort-badges-globals-x86.tsv`) 모두 기준선 뒤 변경이 없다(`git diff --numstat 1f00a25a HEAD` 를 저장소 루트에서 확인, 출력 없음).
- 근거 커밋: 소스·fixture `fa7762f9`(2026-10-24).
- 원본 fixture: `fort-badges-globals-x86.tsv` 32행(10.78 전용 관측). 입력은 6160 BYTE 장부의 초기 프로필 0..7(0, 0xff, 0xa5, 증가, 감소, 곱셈, DWORD 패턴, 1..255 순환)과 함수 순서(`crt`·`reset`·`crt-reset`·`reset-crt` 각 8행), 출력은 두 함수를 적용한 뒤의 전체 장부(16진수)다.
- 공유 장치: 없음(순수 바이트 규칙).

## 1. 옮긴 것

`www/core/raw-fort-badges-globals.js`(DOM 없음, 10.78 전용)

- 상수 `FORT_BADGES_RECORD_COUNT`(28)·`FORT_BADGES_RECORD_BYTES`(220)·`FORT_BADGES_GLOBALS_BYTES`(6160)·`FORT_BADGES_COUNT_OFFSET`(216).
- `initializeFortBadgesRecords(records)`(004fcbe0): 기록마다 220 BYTE 를 0 으로 지우되 **첫 기록 +4..+11 의 8 BYTE 는 원본이 쓰지 않으므로 입력 그대로 둔다**. 앞 27개 기록에 PE 상수의 타입 DWORD(71·72·73·78·128·…)와 15 BYTE 칸 7행의 ASCII 도안(`s`·`w`·`t`·`r` 글자)을 쓰고 28번째 기록은 전체 0 으로 둔다.
- `resetFortBadgeCounts(records)`(0047d860): 28개 기록의 현재 수량 DWORD(+216)만 0 으로 만들고 나머지 6048 BYTE 를 보존한다.
- `RawFortBadgesGlobals`: 한 Client 수명에 BSS 0 → CRT 를 한 번 적용하는 소유자. `records()` 는 항상 같은 Uint8Array 를 돌려주고(로더/조회/저장이 복제 없이 빌림) `resetCounts()` 는 reset 만 호출한다. C++ 의 복사/이동 금지는 JS 에 대응이 없어 장부 배열을 복제하지 않는 것으로 갈음한다.
- 장부가 Uint8Array 가 아니면 TypeError, 길이가 6160 이 아니면 RangeError 로 효과 전에 거부한다(C++ `invalid_argument` 대응).

## 2. 검증

`tests/fort-badges-globals.test.mjs`(2건)

- 원본 관측 재생: 32행 전부. 행마다 프로필 장부를 만들고 관측과 같은 순서로 두 함수를 적용해 전체 6160 BYTE 가 원본 출력과 같은지 확인한다(PE 상수 27개 기록 도안, 첫 기록의 미작성 8 BYTE 보존 포함).
- cpppj 계약 포팅 1건: 소유자 두 개의 배열이 서로 다르고 한 소유자의 `records()` 는 항상 같은 배열이며, 최초 장부가 프로필 0 의 crt 행과 같고, 무늬로 채운 뒤 `resetCounts()` 는 count 4 BYTE 외 6048 BYTE 를 하나도 바꾸지 않으며, 짧은(6159)/긴(6161) 배열은 거부하고 입력 불변이다.

최초 실패와 교정: 모듈은 처음부터 32행이 모두 통과했다(PE 상수 표를 C++ 소스에서 옮긴 것을 전체 장부 비교가 확인한다). 모듈 사본을 바꿔 돌려 비교가 작동하는지 확인했다. 확인 뒤 원본으로 되돌렸다(diff 일치).

- 실패한 변형: 미작성 구간 끝 12→11·시작 4→5·모든 기록으로 확대, 행 간격 15→16, 행 시작 4→5, count 위치 216→212, 28번째 기록 건너뛰기 제거, 첫 행 문자열 변경, 마지막 기록 도안 변경, DWORD 쓰기 마스크 변경, reset 의 한 기록 누락, ASCII 비트 변경(각 1건 모두 실패).
- 적용되지 않았거나 통과한 변형: 없음.

## 3. 경계와 남은 것

- cpppj `FortBadgesGlobals_SharedMetaLoaderGetterAndWriter`(세 번째 시험)는 `RawFortMetaLoad`(로더·`SetBadge`·`LoadBadges`)와 `RawFortBadgesSave`(편집/저장 파일, 웹 제외 분류)를 쓰므로, `RawFortMetaLoad` 를 옮길 때 이 소유자의 `records()` 로 같은 시험(로더가 빌린 배열의 보존과 `ReadFortBadgeCount`)을 대조한다.
- 실제 Client 시작 흐름(원본 시작 reset 호출)과 소유자의 수명 연결은 순서4·5 몫이다. CD 10.72 분기는 웹 범위 밖이다.

## 4. 다음 시작점

WB04-E226 은 3.1절에서 고른다. 기준선 뒤 변경이 없는 Fort/섬 단위(`RawFortMetaLoad`·`RawFortPaletteTransition`·`RawTerrainBitmap`·`RawStateWorldCleanup`·`SquidSpatial`·`RawTerrainGrowth`)를 권한다.
