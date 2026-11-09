# webpj 파생 그림·캡처 4종(Factory·썬더 캐논·훨리베이스·간헐천) 이식 — WB04-E202

2026-11-09, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.1절 "그림·캡처" 묶음의 첫 부분이다. E199 실제 ShapeToBuffer(`render/raw-shape-to-buffer.js`)가 extra&2 일 때 부르는 파생 가상 그림(+78 기본 프레임·+a4 추가 그림)을 네 TYPE 에 대해 옮기고, cpppj `ShapeToBufferTests` 의 파생 캡처 TEST_CASE 를 그 위에서 원본 픽셀과 대조했다.

- cpppj 기준선 `1f00a25a`. 네 소스와 시험 파일은 기준선 뒤 변경 없음(`git diff --numstat 1f00a25a HEAD` 0줄). 근거 커밋:
  - `o/RawGeyserCapture` `ce5a3884`
  - `o/RawThunderCannonCapture` `16ca2d8b`
  - `o/RawSunAviaryCapture` `078e4c84`
  - `o/RawFactoryDraw` `16ca2d8b`
  - 시험 `cpppj/tests/ShapeToBufferTests.cpp` `ce5a3884`
- 원본 fixture(모두 10.78 단일 판본, 기준선 뒤 변경 없음):
  - `factory-capture-x86.tsv` 200행(머리 행 1줄 제외, SHA-256 `32585f45…`, 근거 `e6c41d17`)
  - `thunder-cannon-capture-x86.tsv` 333행(`a3b6b3be…`, `16ca2d8b`)
  - `sun-aviary-capture-x86.tsv` 384행(`3777792b…`, `078e4c84`)
  - `geyser-capture-x86.tsv` 459행(`ddc9e3af…`, `87c3f91f`)
- 공유 장치:
  - E199 `render/raw-shape-to-buffer.js`·`render/vfx-draw.js`(DrawShape)·`render/squid-renderer.js`(그림 헤더 표)
  - E200 `tests/capture-assets-support.mjs`(실제 TYPE/SHP 자료·client 50 풀·16진 표기)
  - WB03-X `assets/terrain-tiles.js` 의 `FrameCodeTable`(cpppj `RiftTypeFrames`: `codes[].side`·`run(글자)`)

## 1. 옮긴 것

네 모듈 모두 `www/core/`, DOM 없음, 10.78 전용이다. 슬롯은 50바이트 raw 이며 +0 가상 표·+10 TYPE·+11 상태·+36 현재 프레임·+40 extra 를 읽는다.

- `raw-geyser-capture.js`(TYPE 122 / VT `0x50da88`)
  - `isGeyserCapture(raw)`: 두 식별자가 모두 맞아야 한다.
  - `drawGeyserCapture(raw, draw, x, y)`(00485480): 현재 TYPE/프레임을 497280 소비자에 한 번 낸다. 현재 프레임이 본체와 같아도 두 번째 그림을 낸다.
  - 기본 프레임은 4acfe0 의 TYPE 고정 본체(정의의 `baseFrame`)이며 호출자가 `state.mainFrame` 에 공급한다.
- `raw-thunder-cannon-capture.js`(TYPE 131 / VT `0x50bad0`)
  - `thunderCannonCaptureMainFrame(raw, frames)`(0046ae10): 현재 프레임 코드의 방향 글자 구간 첫 번호다. 현재 프레임이 코드 표 밖이면(부호 없는 비교) `RangeError`.
  - `drawThunderCannonCapture(raw, draw, x, y)`(0046ae30): extra·출력 WORD·기본 프레임 일치로 생략하지 않고 현재 그림을 한 번 낸다.
- `raw-sun-aviary-capture.js`(TYPE 72 / VT `0x509930`)
  - `sunAviaryCaptureMainFrame(raw)`(0045b530): extra&9 면 현재 프레임, 아니면 signed 현재 20..26 → 10, 11..19 → 현재−9, 그 밖은 −1(기본 그림 생략).
  - `drawSunAviaryCapture(raw, draw, x, y)`(0045b560): extra&9 면 소비자 없이 생략, 나머지는 현재 그림을 한 번 낸다.
- `raw-factory-draw.js` `RawFactoryDraw(pool, frames)`(Factory `0x509148`·아웃포스트 `0x509210`)
  - 생성 때 10.78 풀과 188칸 프레임 표를 요구한다.
  - `handles(sid)`: 두 원본 가상 표만 받는다.
  - `mainFrame(sid)`(00454cd0): 상태 BYTE 상위 3비트 Level 의 'A'..'H' 구간 첫 번호(없으면 −1).
  - `additional(sid, x, y, draw)`(00454cf0): extra&9 또는 dead(상태 비트 2)면 생략하고, 현재 프레임(signed)이 기본 프레임과 다를 때만 `draw(x, y, type, frame)` 을 한 번 낸다. 기본 getter 뒤 현재 슬롯을 새로 읽는다(`pool.slot` 은 사본이므로 읽는 위치마다 다시 부른다).
  - 미복원 가상 표·표 밖 TYPE 은 SID·TYPE·VT 를 담은 오류로 거부한다(cpppj `logic_error`).
- 거부 종류: 다른 TYPE/가상 표와 소비자 누락은 `TypeError`(cpppj `invalid_argument`), 프레임 범위는 `RangeError`(`out_of_range`)다.

## 2. 검증

`tests/shape-to-buffer-derived.test.mjs`(8건, cpppj `ShapeToBufferTests` 의 파생 캡처 TEST_CASE 7건 + 웹 계약 1건). 각 행은 raw 슬롯·배율·기준점 옵션만 설치하고 모듈이 계산한다. 소유자 색표 토큰은 모두 0(plain 경로)이며 TYPE 기준점은 원문 입력(−2, 3)으로 덮어쓴다.

- Factory 200행: 두 가상 표·Level 0..7·extra 2/3/10·frame·기준점 옵션·배율 32768/65536/98304. 메타데이터·전체 픽셀·버퍼 크기·문맥 복원 4칸이 같고, 추가 그림은 60회(모두 배율 65536·TYPE 85)다.
- 썬더 캐논 333행·훨리베이스 384행·간헐천 459행: 프레임 × extra 0/2/3 × 소유자 0/1/8 의 메타데이터·전체 픽셀이 두 정밀도 기록 칸 모두와 같다.
- cpppj 손검사 1:1: 썬더 캐논 다른 VT(`0x50b9b8`) 거부와 범위 밖 프레임 37, 훨리베이스 생략 경로(extra 3/10 에서 현재 27, 소비자 없음 허용)와 다른 VT(`0x5099e8`), 간헐천 소비자 누락과 다른 VT(`0x5196f0`).
- 웹 계약: `RawFactoryDraw` 의 판본/표 크기 거부, Level 별 첫 번호, 같은 프레임·extra&9·dead 생략(소비자 불필요), 다른 프레임의 소비자 요구와 인자 전달, 아웃포스트 표 수용, 미복원 VT·표 밖 TYPE 거부.

최초 실패와 교정: 없다. 모듈과 시험 모두 첫 실행에 통과했다. 비교가 실제로 작동하는지 확인하려고 Factory·훨리베이스·간헐천의 추가 그림 소비자를 빈 함수로 바꾼 사본을 돌리면 세 대조 검사가 모두 실패했고, 사본은 지웠다.

## 3. 경계와 남은 것

- 네 처리기는 실제 Session 의 Squid 가상 분배(가상 +78/+a4 를 TYPE·VT 로 고르는 소비자)와 아직 연결하지 않았다(PORT_MAP session 0). 연결은 순서5 표시 연결에서 `RawCampaignParticles`·건설/피해 캡처 소비자와 함께 한다.
- 실제 TYPE 레코드 기준점(hotFootRatio)은 웹 캡처 장치 레코드에 아직 없고 시험은 원문 입력을 덮어쓴다. 타입 레코드 연결은 3.6절 `RiftTypeTable` ③ 행(WB03-M2/WB05 Draw·타입 레코드 연결) 몫이다.
- mapped(소유자 색표) 경로의 파생 그림은 fixture 에 없다(색표 토큰 0). 소유자 색 캡처 소비자 이식 때 대조한다.
- 같은 묶음의 남은 그림·캡처 단위: RawTempleDraw·RawRainVortexDraw·RawManaDraw·RawPlainAssetDraw·RawPlainUnitCapture·RawPlainWorkshopCapture·RawColorSortPrimary·RawMinimapRaster·RawColorMapCache(adapt).

## 4. 다음 시작점

WB04-E203 은 같은 "그림·캡처" 묶음의 나머지(RawPlainAssetDraw·RawPlainUnitCapture·RawPlainWorkshopCapture 등, 각 cpppj 시험과 fixture 를 먼저 확인)나 3.1절의 다른 묶음에서 고른다.
