# webpj ShapeToBuffer 캡처와 VFX 그리기·Squid 그림 헤더 이식 — WB04-E199

2026-11-09, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 백아흔아홉 번째 부분이다. 마지막 남은 입자 표시 시스템(Burning)으로 가기 전의 client 선행 부품이다. `RawBurningFrame` 이 피해 객체 그림을 캡처할 때 쓰는 실제 `ShapeToBuffer`와, 그것이 기대는 VFX plain/mapped RLE 그리기·Squid 표시 그림 헤더 표를 옮겼다.

- cpppj 기준선 `1f00a25a`. 네 소스는 기준선 뒤 변경 없음(`git diff --numstat 1f00a25a HEAD` 0줄). 근거 커밋: `VFXDraw` `3725d58b`, `VFXMappedDraw` `7f192a4a`, `SquidRenderer` `aa506ebb`, `RawShapeToBuffer` `e6c41d17`.
- 원본 fixture(기준선 뒤 변경 없음):
  - `graphics-x86.tsv` 709행(SHA-256 `bdf284fe…`, 근거 `128f8b95`)
  - `shape-to-buffer-x86.tsv` originals 80행(`d92bc1a2…`, `ab9361f1`; originalCD 84행은 범위 밖)
  - `support-capture-x86.tsv` 162행(`507e26dc…`, `a726b49f`)
- `BurningFrameCapture`(Burning 프레임 hooks 에 캡처를 설치하는 11줄)는 `RawBurningFrame` 과 같은 시험을 쓰므로 다음 단위(E200)에서 함께 옮긴다.

## 1. 옮긴 것

### `www/render/vfx-draw.js` (`client/VFXDraw` 의 DrawShape)

- `drawShape(destination, width, height, database, block, frame, x, y, pane)`: 00401d92 의 원본 몸체.
  - pane 원점 이동, 양 끝 포함 pane/화면 클리핑, 투명 건너뛰기 유지를 보존한다. 색 0/255 도 불투명이면 쓴다.
  - 반환값 0/-1/-2/-3/-4 와, 버퍼가 작거나 블록/프레임 번호가 범위 밖일 때의 예외를 원본대로 둔다.
- `drawShapeWith(…, map)`: plain/mapped 공통 몸체다.
- `www/assets/shapes.js` 에는 `squidHeader(block, frame)` 를 추가했다. 프레임 앞 36바이트를 원문 그대로 복사하며, 추가 헤더가 없으면 `squidMetrics` 와 같은 오류를 낸다.
- 이로써 `VFXDraw` 는 port 다. 파일 map backing 은 웹의 바이트 배열로 대체했다.

### `www/render/vfx-mapped-draw.js` (`client/VFXMappedDraw`)

- `drawMappedShape(…, colors)`: 0040229f. 불투명 소스 픽셀의 색 번호만 256바이트 표로 치환하고 배경은 그대로 둔다.
- 반환값과 pane 처리는 plain 과 같다. 표 길이가 256 이 아니면 거부한다.

### `www/render/squid-renderer.js` (`client/SquidRenderer`)

- `SquidRenderer(renderer)`: Squid 표시 sink 어댑터다.
  - `suppressed()` 는 `renderer.fullRedrawPending()` 이고, `invalidate(rect, flags)` 는 rect 를 viewport 로 다시 자르지 않고 그대로 넘긴다(선택 표식 확장 보존).
  - 두 메서드가 없는 렌더러는 거부한다.
- `squidRendererShapes(table, database)`: 원본 로딩 순서의 188칸 그림 헤더 표를 만든다.
  - 내장 타입과 186·187 은 빈 칸이다.
  - 자산 타입 칸에는 클러스터 수·loaded 와, 그림자/abstract 레이어를 포함한 물리 프레임별 헤더 12값을 담는다. 헤더 12값은 출력 크기·기준점·셀 float 네 개·포구 WORD 두 개(+32/+34)·보행 간격이다.
  - 읽지 못한 정의가 있으면 빈 그림으로 대체하지 않고 거부한다.

### `www/render/raw-shape-to-buffer.js` (`client/RawShapeToBuffer`, 10.78 전용)

- `createShapeToBufferState({ zoom, addTypeHotspot, ownerTypeMaps, ownerColors, mainFrame, additional })`: cpppj `ShapeToBufferState` 의 기본값이다.
- `RawShapeToBuffer(pool, types, shapes, database, state).capture(sid)`: 004a7db0 의 원본 순서를 따른다.
  1. 현재 slot 의 TYPE 범위를 확인한다.
  2. 해제 상태 bit0, 가상 표 0/`dd…`/`cd…`, frame 범위, loaded 를 확인한다. 하나라도 어긋나면 10.78 원본 진단 예외다.
  3. TYPE maxHP 의 두 해제 표식을 확인한다.
  4. 추가 그림(extra&2)이면 `mainFrame`·`additional` 이 있어야 한다.
  5. 소유자가 있으면 색표 토큰을 확인하고, 토큰이 있으면 실제 256바이트 색표를 받는다.
  6. 현재 frame 헤더에 fixed16 배율을 적용한다(`Math.imul` low DWORD 뒤 `>> 16`). 크기를 검사하고 255 버퍼를 만든다.
  7. 기본 그림을 그린다. plain 은 가상 main 을, mapped 는 원본 비대칭대로 현재 frame 을 그리고, main −1 이면 생략한다.
  8. 파생 추가 그림을 그린다.
  9. 옵션이 켜져 있으면 TYPE 기준점 보정을 32비트 감김으로 한다.
- 상태 객체는 캡처 때마다 다시 읽는다. `pool` 접근자는 Burning/건설 캡처 어댑터가 같은 월드인지 확인하는 데 쓴다.

## 2. 검증

### `tests/vfx-draw.test.mjs` (3건)

- 원본 x86 709행: 반환값과 화면 전체(색 0/255·네 방향 클리핑·원점 이동·-1/-2/-3/-4)를 대조한다.
- mapped 웹 계약 검사(실제 보호 SHP 네 프레임): 항등표면 plain 과 같다. 역순표면 불투명 소스 픽셀만 치환되고, 배경·반환값·pane 은 같다. 표 길이·버퍼 크기·블록 범위는 거부한다.
- `squidHeader` 웹 계약 검사: `squidMetrics` 와 같은 위치를 읽고, 복사본을 돌려주며, 순수 VFX 는 거부한다.

### `tests/shape-to-buffer.test.mjs` (5건)

- Dude(TYPE 70) 80행: frame 0/1/7/32/64·배율 0.5/1/1.5/2·TYPE 기준점(−2, 3) 옵션·free/dead 상태의 메타데이터와 전체 버퍼를 대조한다.
- 섬 조각 162행: TYPE 95/157 의 기본/추가 그림(TYPE baseFrame)·extra 0/2/3·소유자 0/1/8(색표 토큰 0 → plain)·기준점 보정과, 추가 그림 소비자 호출 횟수를 대조한다.
- 거부 웹 계약 검사: 무효 객체 일곱 가지, maxHP 표식 둘, 추가/소유자 소비자 미연결, 색표 크기, TYPE 기준점 미연결, 판본 크기 오류.
- mapped/추가 그림 웹 계약 검사:
  - 호출 순서: 색표 → main → 추가 그림이며, 추가 그림은 보정 전 기준점을 받는다.
  - mapped 는 현재 frame 을 그린다.
  - plain 은 가상 main 을 그리고, main −1 이면 생략한다.
  - 상태의 현재 배율을 다시 읽는다.
- SquidRenderer 웹 계약 검사:
  - 188칸·빈 칸·클러스터 수·물리 프레임 수·포구 WORD 를 확인한다.
  - 정의 누락은 거부한다.
  - 어댑터는 무절단 전달하고, `DirtyRegions.fullRedraw` 를 반영한다.

최초 실패와 교정: 없다. 모듈과 시험 모두 첫 실행에 통과했다.

## 3. 경계와 남은 것

- `shape-to-buffer` 시험 파일의 나머지 원본 대조 행은 각 파생 그림 단위에서 이 모듈 위에 옮긴다.
  - Factory 200 → `RawFactoryDraw`
  - ThunderCannon 333 → `RawThunderCannonCapture`
  - SunAviary 384 → `RawSunAviaryCapture`
  - Geyser 459 → `RawGeyserCapture`
- `VFXMappedDraw`·`SquidRenderer` 는 cpppj 전용 시험이 없다. mapped 의 실제 소비자(소유자 색 캡처·CampaignSession 그림)와 실제 `Renderer` 연결은 WB03-M2/WB05 에서 다시 대조한다.
- TYPE 레코드의 실제 `hotspotX/Y`(타입 표 +0x1dc/+0x1e0)는 웹 레코드에 아직 없다. 시험은 원문처럼 명시한 값만 쓰고 나머지는 0 이다. 실제 Factory/타입 레코드 연결 때 채운다.
- 10.72(CD)의 8×8/4×4 투명 대체와 CD raw 오프셋은 범위 밖이다(웹은 10.78 전용).

## 4. 다음 시작점

WB04-E200 은 `o/RawBurningFrame` 과 `client/BurningFrameCapture` 다. cpppj `BurningFrameTests` 의 originals 행을 이 모듈과 기존 `raw-burning-visual.js`·`raw-burning-sources.js`·`raw-burning-victim.js`·`render/dirty-regions.js` 위에서 대조한다. 그 뒤 E201 에서 `BurningVisualSystem` 을 옮긴다.
