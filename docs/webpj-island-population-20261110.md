# webpj 섬 population(RawIslandPopulation) 이식 — WB04-E223

2026-11-10, HJOW-Athlon. 섬 배치(E222 Builder) 뒤의 거주 구역·초기 객체 생성과 record 후처리(0046df80 Residence·0046e000 Populate·0040e2b0 ReplaceInitial·004324e0 WriteRecord)를 옮겼다. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.1절의 "순서4 이후로 넘기는 `o/` 단위(Fort 적재·지형/섬 …)"에 속하며 기준선 뒤 변경이 없고 원본 관측이 있어 E161 섬 목록·E220 섬 검색·E221 Fort 소유자 후처리·E222 섬 Builder 에 이어 옮겼다.

- cpppj 기준선 `1f00a25a`. 소스(`RawIslandPopulation.cpp/.h`)·시험(`IslandPopulationTests`)·fixture(`island-population-x86.tsv`) 모두 기준선 뒤 변경이 없다(`git diff --numstat 1f00a25a HEAD` 루트에서 확인).
- 근거 커밋: 소스·fixture `afe4ce5b`.
- 원본 fixture: `island-population-x86.tsv` 118행(10.78 전용 관측). 입력은 모드(Residence·Populate·Replace)·사각형 프로필·local seed·Fort 모드·권한·진단 중 권한 변경·primary/fallback TYPE·HP 유무·성공 시도·외부 효과의 지도 변경, 출력은 호출 순서(`P`·`D`·`T`·`A`·`position`·`destroy`·`create`·`type`·`hasHp`·`maxHp`·`owner`·`setOwner`·`pop`·`refresh`)·새 객체 프레임 DWORD·HP·전체 청크 지도(헤더 3 DWORD + 256청크 12 BYTE)·배치 호출 수다.
- 공유 장치: E221 `raw-fort-finish-owner.js`(`placeRandom`·`timeSeed`), E222 `raw-island-builder.js`·`assets/territory.js`(`ChunkMap`), E161 `raw-island-runtime.js`, E2 `raw-squid-word.js`(`setRawSquidHitPoints`), `random.js`(`GameRandom`).

## 1. 옮긴 것

`www/core/raw-island-population.js`(DOM 없음, 10.78 전용)

- `ISLAND_POPULATION_ASSERTION`(`boundsMissing`·`recordMissing`·`residenceAuthority`·`populationAuthority`·`missingPlacement`, 원문 c2/139/c3/d1/118 보고)·`RawIslandPopulation(pool, target, world, islands, placement, globals, hooks)`: 섬 목록과 `RawFortFinishOwner` 가 같은 풀인지 확인한다. `globals` 는 현재 전역 getter(`boss`·`fort`·`residence`·`primary`·`fallback`·`replacement`), `hooks` 는 선택된 실제 장치(진단·벽시각·위치·삭제·생성·기본 프레임·HP·owner·Pop·생산 갱신).
- `residence(region)`(0046df80→0040e130): 부모가 준 record 6 BYTE 는 읽지 않고 현재 지도 사각형을 다시 얻는다(미존재 섬은 진단 뒤 실제 청크로 계속). 권한이 없으면 진단 뒤 **현재 권한을 다시 읽어** 없으면 반환. world 지도 record 의 BYTE3×103 로컬 난수로 `placeRandom` 1개를 현재 거주 구역 TYPE 으로 배치한다.
- `replaceInitial(area, region)`(0040e2b0): 중앙 10×10 사각형(원문 00500fc8=10.0, 짧은 변의 음수 대체식 `-right-left-10` 포함, 네 `_ftol` 결과를 다시 float32 로 저장)에 현재 primary TYPE 을 최대 10회, **모두 0 일 때만** fallback TYPE 을 최대 10회 배치한다(Fort 모드는 로컬 난수를 실제 벽시각 `timeSeed` 로 교체). 결과 SID 는 WORD/풀 범위만 확인한다. 결과가 없으면 진단 뒤에도 이어간다. 이어서 위치 snapshot(삭제 전) → 가상 삭제(flags 0) → 교체 TYPE 생성 → 새 객체 TYPE+118 의 기본 프레임을 프레임 DWORD(+36)에 직접 쓰기(setter 없음) → HP 보유이면 현재 타입 getter 로 비가상 HP setter → 현재 지역 owner → 가상 Pop.
- `populate(region)`(0046e000): 입구의 사각형/record snapshot → 권한 진단(현재 권한 재확인) → `residence` → `replaceInitial` → record 의 bit 0x800 → `#writeRecord`(현재 폭/높이를 y/x 순서로 읽어 같은 섬(signed BYTE)의 청크 전부에 6 BYTE 쓰기, 일치 청크가 없어도 마지막 생산 통지 실행; 섬 미존재 진단). 외부 효과가 current record 를 바꿔도 입구 snapshot 에 bit 만 켜서 쓴다.
- `fortTerritoryIslandRecord(chunks, island)`(00432430): y 바깥/x 안쪽 순서로 같은 섬의 첫 record 6 BYTE 사본(없으면 0). 원본은 `RawFortTerritorySave.cpp`(편집/저장 파일, 웹 제외 분류)에 있어 이 모듈의 지역 도우미로 두었다.

## 2. 검증

`tests/island-population.test.mjs`(3건)

- 원본 관측 재생: 118행 전부. 행마다 새 풀(서버 15000·15001 확보)·지도·섬 목록·배치 몸체를 만들고 위 출력 전부를 대조한다. 웹 청크 지도가 y 행 우선이라 원본 물리 index i 를 (x=i/16, y=i%16)으로 옮겨 쓰고 읽는다.
- 웹 계약 2건: 첫 record 조회의 순서/signed 비교/사본 반환/없으면 0, 다른 풀의 섬 목록·배치 거부와 선택 효과 누락(진단 장치 부재, 생성 부재는 위치 snapshot·삭제까지만 진행).

최초 실패와 교정: 모듈은 처음부터 118행이 모두 통과했다. 모듈 사본을 바꿔 돌려 비교가 작동하는지 확인했다. 확인 뒤 원본으로 되돌렸다(cmp 일치).

- 실패한 변형: 로컬 난수 계수 103→101, 중앙 크기 10→8, 시도 횟수 10→9, record 비트 8→4, 권한 재확인 제거, Fort 시각 seed 제거, 음수 대체식 제거(각 1건).
- 적용되지 않은 변형: record 쓰기 루프의 순서 교환(sed 패턴 불일치). 쓰기는 같은 섬의 모든 청크에 같은 6 BYTE 를 쓰므로 순회 순서는 결과에 영향이 없다.

## 3. 경계와 남은 것

- 실제 월드에 연결하지 않았다. 호출 지점(일반 전투 섬 생성의 `RawIslandAdd`·미션 로드의 Fort 부모)과 실제 삭제/생성/Pop/프레임/HP/owner 하위 연결은 순서4 몫이다(PORT_MAP session 0).
- 같은 fixture 를 쓰는 cpppj `IslandAdd_ActualPreviewCollisionAndPlaceBeforeMissingGeneration` 은 `RawIslandAdd`(미이식)를 옮길 때 대조한다. CD 10.72 는 웹 범위 밖이다.

## 4. 다음 시작점

WB04-E224 는 3.1절에서 고른다. 기준선 뒤 변경이 없는 Fort/섬 단위(`RawIslandLifecycle`·`RawFortMetaLoad`·`RawFortBadgesGlobals`·`RawFortPaletteTransition`·`RawTerrainBitmap`·`RawStateWorldCleanup`·`SquidSpatial`·`RawTerrainGrowth`)를 권한다.
