# webpj 영역 배치: `Territory` 해석과 `TerrNN` 청크의 월드 배치 (WB03-V, 2026-11-01)

2026-11-01, **HJOW-Athlon**(마지막 전체 정밀 디컴파일 호스트 HJOW-Athlon/2026-10-23 과 같은 PC 이지만 이번 단위는 디컴파일 소스를 읽지 않았다. 근거는 cpppj 의 복원 소스 `o/CanonDecoder.cpp`·`o/CanonDecoderTables.inc`·`o/ChunkMap.cpp`·`o/Islandbuilder.cpp`·`o/RiftType.cpp`와 그 근거 문서 [영역 청크 배치 분석](exe/territory-layout.md), 그리고 cpppj 의 테스트 `tests/TerritoryTests.cpp`·검사 도구 `tools/cpp_window_smoke.py` 이다. 모두 읽기만 했다). 사용자 요청("AGENTS.md, LEFT_JOBS.webpj.md 를 읽고 webpj 다음 작업 진행")으로 [인계](../LEFT_JOBS.webpj.md)의 '다음' 첫 항목인 **정적 맵 화면(M2) 2단계 — 영역 배치**를 했다. 1단계([`.fort` 판독기](webpj-fort-reader-20261101.md))가 `Territory` 120바이트를 원시 값으로만 보존하고 `TerrNN` 의 청크 레코드가 월드의 어느 청크인지는 정하지 않았는데, 지형 생성과 오브젝트 그리기는 월드 좌표가 있어야 하므로 이 배치가 먼저다. `cpppj/`·`dotnetpj/`·`originals/`·`originalCD/`·AGENTS.md·공통 `LEFT_JOBS.md`·`README.md`(이전 단위에서 받은 '공통 파일은 나중에 몰아서 갱신' 지시를 이어 지킴)·`.gitattributes`·공유 명칭 JSON·`docs/formats/` 는 수정하지 않았다(읽기만). 원본 게임·analyzeManager·브라우저는 실행하지 않았다(화면이 바뀌는 변경이 아니다).

## 1. 범위와 결과

| 대상 | 결과 |
|---|---|
| 영역 패턴 표 | 원본 68개(모양 번호는 6비트라 앞의 64개만 선택됨)를 칸 단위(방향 글자·변형 글자·번호 글자)로 `territory-patterns.js` 에 담음. 실행 파일에서 따로 뽑은 dotnetpj 자료와 칸마다 같음 |
| 패턴 풀기 | 회전 0~3(방향 값 ÷ 2)을 반영해 칸 좌표·프레임·방향 글자를 내는 반복자. `puzzlePiece`(프레임 32개)에서 68개 패턴 × 회전 4가지 모두 빠지는 칸 없이 풀림(374칸) |
| 청크 지도·섬 목록·놓기 | `ChunkMap`(16 × 16)·`IslandList`·`IslandBuilder`(`place`/`placeAll`)·`territoryChunks`(y 바깥·x 안쪽 순서) |
| `.fort` 배치 | `layoutFort`: 영역 20개의 놓인 청크 + `TerrNN` 청크 레코드 짝짓기 + `Chaff` 청크 i → 월드 청크 (i % 16, i ÷ 16) |
| 원본 맵 | `.fort` **465개 모두**(느슨한 433 + 아카이브 32) 독립 구현의 기대값과 줄 단위로 같음. 놓인 영역 **2,060개**(cpppj 문서의 수와 같음), 놓인 청크 수 = 저장 청크 레코드 수 **전부**, 지도 밖·프레임 없음 칸 **0**, 월드 좌표를 붙인 오브젝트 **313,712개**(1단계의 레코드 합계와 같음) |

화면에 보이는 변화는 없다(배치 계산만). 웹 공개 데이터(`www/data`)도 늘리지 않았다(패턴 표는 JS 모듈에 담겨 배포 자산 목록·매니페스트와 무관).

## 2. 원본 규칙(cpppj 근거)

* **레코드**(6바이트 × 20): `[0]` 하위 6비트 모양 번호·상위 2비트 방향 값, `[1]` 플래그(bit 0 이 켜지고 bit 1 이 꺼져야 놓음), `[2]` 청크 위치(**하위 니블 x, 상위 니블 y** — 오브젝트 위치 바이트와 반대), `[3~5]` 보존만 함(역할 미확정).
* **놓는 순서와 원점:** 영역을 번호순으로 놓고 나중에 놓은 영역이 겹친 청크를 차지한다(섬 번호 = 놓은 순서). 미션·요새를 읽는 경로(`00432050`)는 기준 사각형 (1,1)~(15,15)의 왼쪽 위에서 위치 바이트를 센다 — 청크 (x,y) = (1 + 위치 x + 패턴 안 열, 1 + 위치 y + 패턴 안 줄). 요새 편집 경로의 시작점 (6,6)과 일반 전투의 플레이어별 재배치는 다른 경로라 옮기지 않았다.
* **패턴 풀기**(`00425c20`·`00425860`·`00425700`·`00425c00`): 회전 = 방향 값 ÷ 2(0~3). 패턴 칸은 줄 우선이고 회전표(`0x531530~0x531580`)가 시작 칸·바깥(줄)/안쪽(칸) 걸음을 정한다. 칸의 방향 글자 `'.'` 은 빈 칸이고 `'A'`~`'P'` 이면 회전표(`0x531590`)로 글자를 바꾼 뒤 `puzzlePiece` 의 프레임 코드에서 (바뀐 글자, 변형 `'P'`, 변형 숫자)가 같은 첫 프레임을 찾는다. 못 찾은 칸은 원본이 assert 보고 후 건너뛴다.
* **저장 청크 짝짓기**(`004be2c0`): 월드를 y 바깥·x 안쪽으로 훑을 때 그 영역에 속한 청크의 순서가 `TerrNN` 섹션의 청크 레코드 순서이다. `Chaff` 는 256청크 전부를 같은 순서로 담는다.
* **방향 처리의 주의점**(`territory-layout.md`): 생성 경로는 저장 방향 값을 ÷ 2 하는 반면 크기 계산 경로(`004bfd70`·`004bfde0`)는 × 2 해서 쓴다. 이 단위는 생성 경로만 옮겼다. **원본 파일의 활성 영역은 모두 방향 0** 이라 회전된 저장 파일의 실제 플레이 결과는 원본으로 검증하지 못했다.

## 3. 구현

| 파일 | 내용 |
|---|---|
| `www/assets/territory-patterns.js`(신규, 생성물) | `TERRITORY_PATTERNS`(68개: 폭·높이·`sides`·`variations`·`labels` 글자열)·`TERRITORY_PATTERN_COUNT` |
| `tools/make-territory-patterns.py`(신규) | cpppj `CanonDecoderTables.inc` 의 영역 패턴 68개를 위 모듈로 쓰는 생성 도구. `--check` 는 파일을 쓰지 않고 같은지만 확인 |
| `www/assets/territory.js`(신규) | `findFrameNumber`·`pieceFrameCodes`·`decodeTerritoryPattern`(제너레이터)·`ChunkMap`·`IslandList`·`IslandBuilder`·`territoryChunks`·`readTerritoryRecord`·`worldCell`·`layoutFort`·회전표 `ROTATED_SIDES` |
| `tools/make-territory-expected.py`(신규) | 독립 기대값 표 생성 도구. JS 구현과 겹치지 않는 `tools/fort.py`(Python 판독기)와 실행 파일에서 따로 뽑은 dotnetpj 패턴 JSON 만으로 계산 |
| `tests/fixtures/territory-expected.tsv`(신규) | 위 도구의 결과(2,990줄, 약 200KB). `www/` 구현의 출력으로 다시 만들지 않는다 |
| `tests/territory.test.mjs`(신규) | 19개 검사(아래 5절) |

사용 예: `layoutFort(parseFort(bytes, types), pieceFrameCodes(typeTable))` → `layout.regions[7].chunks`(= Save the Island! 의 플레이어 영역 `10,6 11,6 10,7 11,7`), 오브젝트의 월드 칸은 `worldCell(chunk.x, chunk.y, object)`. 청크 지도의 항목은 (x, y) → `entries[y × 16 + x]` 이다(원본은 `x × 16 + y` 이지만 밖으로 드러나지 않는다).

옮기지 않은 것: 요새 편집(시작점 (6,6))·일반 전투 배치, 다른 청크 지도(`CustomMap`)로 놓는 경로(원본 조건 "전역 청크 지도일 때만 섬 목록에 등록"은 항상 참으로 둠), 섬 목록의 나머지 필드(섬 범위 기록 `0046f190`), `ChunkMap::Reset`·모드 필드, 다리 패턴·`CanonTypeDecoder`(타입별 패턴), 지형 생성.

## 4. 원본과 다르게 둔 점·가정

1. **지도 밖 칸:** 원본은 `ChunkMap::At` 에서 assert 한 뒤 가장자리로 당겨 같은 항목에 쓴다. 이 구현도 당기되 `IslandBuilder.clampedCells` 로 세어 둔다(원본 `.fort` 465개에서는 0).
2. **프레임 없는 칸:** 원본처럼 건너뛰되 `missingFrames` 로 센다(원본 `.fort` 에서는 0).
3. **저장 청크가 모자라거나 남을 때:** 원본은 모자라면 읽기 끝을 넘어 assert 하고 남으면 무시한다. 이 구현은 오류를 던지지 않고 `region.matches` 를 거짓으로 둔다(모자란 청크의 `stored` 는 null). 원본 `.fort` 465개에서는 모두 일치한다.
4. **소유자:** 미션 읽기 경로처럼 섬 소유자를 1 로 둔다(`layoutFort` 의 `owner` 인자로 바꿀 수 있음). 실제 소유자 결정은 월드에 오브젝트를 놓는 단계의 일이다.
5. **회전된 영역은 원본으로 검증하지 못했다.** 회전 1·2·3 의 기하는 시계 방향 90도 단위의 독립 계산과, 글자 변환은 회전표의 성질(순열·`A`/`P` 고정·회전 1 의 거듭제곱)과 맞췄을 뿐이다. 회전표 자체는 cpppj 가 옮긴 값이며 원본 화면과 대조한 것이 아니다.
6. **패턴 표 출처:** cpppj 표(68개)에서 옮겼고, 검사는 dotnetpj 의 패턴 JSON(실행 파일에서 직접 추출)과 칸마다 대조한다. 두 추출이 서로 일치한다.
7. 이 단위의 검사는 **독립 구현(Python)과의 일치**와 cpppj 문서·테스트의 수치·좌표가 근거이다. 원본 실행 중 메모리나 원본 화면과 대조한 것이 아니다.

## 5. 검증(HJOW-Athlon, Node v24.21.0)

* `npm --prefix webpj test`: **187개 중 185개 통과**(신규 19). 실패 2개는 이 PC 의 `fonts/License.txt` 줄바꿈(CRLF) 때문에 자산 비교 2개가 실패하는 기존 현상이다(이전 단위 기록과 같음, 내용 차이 아님). `make-territory-patterns.py --check` 통과.
* 신규 19개: 패턴 표의 칸 구성(68개)·dotnetpj 자료와 칸마다 일치·프레임 찾기(첫 일치/없음/중복)·`puzzlePiece` 프레임 코드(32개)·**회전 0 의 줄 우선 순서와 글자**(68개 패턴)·방향 값 ÷ 2 와 범위 오류·**회전 1~3 의 기하**(68개 패턴, 독립 계산)·회전표의 성질·건너뜀과 원본 assert 경계(번호 `a`/`z`/0x60/0x7b, 방향 `A`/`P`/`@`/`Q`/`Z`, 변형 `/`·`0`·`:`·`;`)·**실제 프레임에서 68개 × 회전 4가지가 빠지는 칸 없이 풀림**·섬 목록·청크 지도(가장자리 당김·섬 사각형·크기 범위)·**TEST01 의 알려진 배치**(cpppj 테스트와 같은 좌표·글자 문자열)·놓기 옵션(위치 무시·기준 사각형·겹침·지도 밖·회전된 영역·모양 번호 40)·놓는 조건(플래그 값 7가지·짧은 입력·19번 레코드·범위)·레코드 읽기와 월드 칸·`layoutFort` 합성 입력(짝짓기·수 불일치·영역 없음)·**원본 `.fort` 465개 전체**(영역 줄과 월드 좌표를 붙인 오브젝트 해시가 기대값 표와 줄 단위로 같음, 놓인 청크 수 = 저장 청크 수, 지도 밖·프레임 없음 0)·**Save the Island! 의 알려진 월드 좌표**(플레이어 영역 Terr07 = 월드 청크 (10,6)(11,6)(10,7)(11,7), `rainVortex` (187,108)·`priest` (185,118)·`Chaff` 의 `rainBattery` (193,106) — [영역 배치 분석](exe/territory-layout.md)의 캡처 일치 좌표).
* **변이 검사:** `territory.js`·`territory-patterns.js` 71곳(상수·비트·회전표·걸음·시작 칸·경계값·분기 조건·좌표 계산·청크 순서·짝짓기·패턴 글자/변형/번호)을 하나씩 바꿔 테스트가 실패하는지 확인하고 매번 복원했다. 첫 실행은 70/71 — 놓친 것은 **모양 마스크 `0x3f` → `0x1f`**(원본 `.fort` 의 모양 번호가 모두 32 미만이라 검사에 안 걸림)였고, 모양 번호 40 을 놓는 검사를 더해 걸리게 했다(최종 71/71). 동등 변이로 건너뛴 것: 항목 저장 순서(`y × 16 + x` ↔ `x × 16 + y`, 밖으로 드러나지 않음), 모양 번호 표 범위 검사(6비트라 도달 불가).
* 확인하지 않은 것: 브라우저에서의 동작(이 모듈은 아직 화면에 연결되지 않음), 회전된(방향 값 2·3) 저장 파일의 원본 결과, 원본 10.72(CD판)의 영역 배치, 원본 실행 중 값과의 대조.

## 6. 다음

M2 의 이어지는 단계: (1) **지형 생성**(`Terrainbuilder`: 청크별 시드·성장·타일 선택, [지면 생성 문서](exe/terrain-and-bridges.md), cpppj `TerrainGrowth`·`TerrainContext`·`CanonTerrain`) — 이번에 만든 `ChunkMap`(섬 번호·영역 안 순서·방향 글자)이 입력이다. 두 공식 맵의 본섬 칸 마스크 SHA-256(Save the Island! `68e699f5…`, The War Begins! `b24af563…`)과 영역별 칸 수가 문서에 있어 기대값으로 쓸 수 있다. (2) 오브젝트·지형 그리기(셰이프 프레임·소유자 색표·그림자·깊이 정렬, [그리기 정렬](exe/cpp-draw-sort-reconstruction.md))와 카메라 — 오브젝트의 월드 칸은 `worldCell` 로 얻는다. (3) 게임 화면 UI(좌측 덱·미니맵).
