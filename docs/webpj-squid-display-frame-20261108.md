# webpj 객체 표시·프레임 지정·프레임 연결(SquidDisplay·SquidFrame·SquidFrameBinding) 이식 — WB04-E59

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 쉰아홉 번째 부분이다. cpppj 기준선 `1f00a25a`의 `o/SquidDisplay`(원본 004ad500 표시 부모 아래의 004990d0/00498ff0 표시 갱신 계산), `o/RawRiftShapeAccess`(00419850 SHP 프레임 접근), `o/SquidFrame`(004acee0 프레임 지정·004ace40 해시 단계 저장), `o/SquidFrameBinding`(프레임 지정 훅의 Pop/Unpop/표시 연결)을 옮겼다. 표시 변경 영역을 모으는 `client/Renderer` 의 `DirtyRegions`(원본 00497580)도 같은 단위에서 `www/render/dirty-regions.js` 로 옮겼다. 웹은 10.78 전용이라 CD 10.72 분기(1바이트 프레임·표 밖 메모리 접근)는 옮기지 않았다. 네 파일 모두 기준선 뒤 cpppj 변경이 없다. 이전 단위(E58)가 미룬 `SquidFrameBinding` 이 이로써 닫힌다.

## 1. 옮긴 것

`www/core/squid-display.js` — `SquidDisplay(types, shapes, sink, view)`, `buildSquidFrameAssertionDetail`

- 생성자가 타입 표와 SHP 목록의 길이가 같은지 보고, `setView` 가 역전 viewport·양수가 아닌 확대율을 장치 호출 전에 거부한다. `validateFrameBinding(types)` 는 길이와 표시·공간 재등록이 쓰는 필드(flags1·flags2·maxHitPoints·footX·footY)를 비교한다.
- `bounds(type, frame, x, y)`: x87 의 `x*16`/`y*11` + 0.5 뒤 절삭에서 카메라를 빼고(float 곱 전 반올림 없음), 핫스폿·폭/높이는 Q16 배율(imul 하위 DWORD → 산술 우측 16)로 곱해 `[left, top, right, bottom]`(오른쪽/아래는 +1)을 만든다. SHP 가 없으면 빈 사각형, 프레임 범위 밖은 오류, 좌표 넘침은 거부한다.
- `update(bytes, flags)`: 현재 가상 표의 특수 8c 를 먼저 분배한다 — 간헐천(`0x50da88`, flags 0x2000)은 현재 TYPE 의 고정 외형 프레임으로 경계를 구하고 위/아래 50픽셀(배율 미적용)을 줄여 viewport 로 자른 뒤 flags 0 으로 보낸다. 마나/볼트(`0x50c3b0`/`0x509810`)는 오른쪽/아래 10픽셀 늘리고, 신전 rune 부분 갱신(WORD 0, 3개 표)은 표마다 네 inset 으로 줄인다. 나머지 확인된 가상 경로는 `RawSquidDisplayParent`(E58)로 보낸다(TYPE 뷰는 같은 배열의 현재 필드, `typeNumber` 는 표 인덱스, `mark` 는 아래 4990d0).
- `#mark`(4990d0): 표시 억제를 먼저 보고 본그림을 viewport 에 자른 사각형(선택 표식이면 폭 19 미만 좌우 9, 위 15, 좌우 3 확장, 확장 뒤에는 다시 자르지 않음)을 보낸 다음 그림자 레이어(flags1 0x40000, `frameCount` 뒤 물리 프레임)를 같은 마커로 보낸다.
- `validate(bytes, flags)`: 미지원 파생 표시 거부, 부분 갱신 경계 계산, 패치 `frameCheck(false)` 조건(`#patchFrameValid`: 프레임 수는 64비트 곱으로 두 배 비트를 반영, SHP·해제 표식)과 물리 프레임 범위(그림자 두 배)를 공간 쓰기 전에 확인한다. `frameSize(bytes, frame)` 는 `readRiftShapeFrame` 로 해시 단계용 칸 크기를 읽는다(004ace40 은 raw abstract 대신 고정 인자 0).
- `buildSquidFrameAssertionDetail`: 00436d40 의 `Frame assertion failed. Asking for type=N (이름) frame=F abs=A\n\nType has K frames\n` 를 만든다. TYPE 은 부호 있는 범위(`>= 10` 이고 표 안)를 먼저 판정하며 아니면 `invalidType`·0 프레임이다.
- 출력 장치(sink)는 `suppressed()`·`invalidate(rect, flags)` 가 필수이고 `checking()`(기본 0)·`frameError`·`assertion` 은 도달할 때만 요구한다(없으면 빈 성공 대신 오류).

`www/core/raw-rift-shape-access.js` — `readRiftShapeFrame(type, frame, abstract, hooks)`, `RIFT_SHAPE_ASSERTION`

- 프레임이 TYPE 프레임 수(두 배 비트면 2배, 하위 32비트 감김 후 부호 있는 비교) 밖이면 선택된 TYPE 번호 getter 와 오류 상세 writer 를 호출하고, 범위 밖·SHP 없음·해제 표식이면 `frameCheck( abstract, frame, false )`(`..\o\rifttype.h`, 0xe1) 진단을 요청한다. 진단이 복귀해도 같은 프레임으로 계속해 선택 TYPE 의 현재 SHP 에서 칸 크기를 읽는다.

`www/core/squid-frame.js` — `SquidFrame(pool, types, hooks)`, `FRAME_UPDATE_ALTERNATE`, `FRAME_REPOP_FLAGS`

- `storeLevel(sid)`: island·bridge genus 는 프레임을 읽지 않고 0, 그 밖은 현재 프레임 크기로 `SquidHash.objectLevel` 단계를 구해 단계 바이트(+0x21)에 쓴다.
- `set(sid, frame, flags)`: flags 는 0 또는 0x2000 만 허용한다. 단계를 쓰기 전의 저장값과 같으면 표시 갱신 → 프레임 DWORD 쓰기 → 표시 갱신, 다르면 새 프레임이 현재와 다를 때만 Unpop → 프레임 쓰기 → 현재 위치에 `flags | 0x50` 으로 Pop(Unpop 뒤 위치를 다시 읽음). 새 프레임의 범위는 원본도 검사하지 않는다.

`www/core/squid-frame-binding.js` — `makeSquidFrameHooks(pool, types, display, unpop, pop)`

- 구성 때 `display.validateFrameBinding(types)` 와 `pop.validateFrameBinding(pool, display, unpop)` 로 판본/풀/표시/해시/spot 불일치를 거부한 뒤 `{ frameSize, update, unpop, pop }` 을 만든다. 10.78 의 실제 표시기는 Pop 의 spot 처리 뒤에 현재 SHP 를 읽으므로 Pop 에는 칸 크기 0,0 을 넘긴다.
- `SquidUnpop.validateFrameBinding(pool, display, hash, spots)`(신규, cpppj `SquidUnpop::ValidateFrameBinding`)을 `squid-unpop.js` 에 추가하고 `SquidPop.validateFrameBinding` 이 그것을 호출하도록 했다(기존에는 풀/해시/spot 만 보고 표시 장치는 보지 않아 표시 없는 Unpop 연결을 거부하지 못했다).

`www/render/dirty-regions.js` — `DirtyRegions(width, height)`

- `add(rect, flags)`: 화면 밖은 무시, 100항목이면 전체 갱신, 버려지지 않은 항목과 순서대로 병합 시도(불투명 flags 4 면 버림 표시 1, 합집합 면적의 1/8 허용치 병합, 첫 병합 대상에서 끝). `clear` 는 첫 항목 flags 를 남긴다(`firstFlags`). 렌더러의 화면 합성은 포함하지 않는다.

## 2. 검증

- `tests/squid-display.test.mjs`(6건): `display-x86.tsv` 10.78 구간 — View/Type/Slot 입력마다 Update 200·Bounds 150 초과·Pop 240·Unpop 240 의 변경 표 전체(위치·flags·병합·넘침)와 raw 슬롯 50바이트를 비교한다. `geyser-partial-display-x86.tsv` 432행의 간헐천 부분 8c 출력과 raw 불변. 계약: `SquidDisplayLive_*` 3건(진단 복귀 뒤 현재 TYPE/좌표 재조회, 상세 writer → 진단 순서와 고정 프레임, 억제와 부모 순서 및 상세 문자열), 카메라 거부·변경 표 병합·넘침·SHP 접근 순서.
- `tests/squid-frame.test.mjs`(7건): `setframe-x86.tsv` 10.78 Set 1920행 + Thunk 4행의 사건 순서(D/E/U/P 와 호출 시점 프레임)·슬롯 50바이트. 계약: 훅 누락/잘못된 flags 거부와 상태 불변, 실제 Factory/Pop/Unpop/Display 를 조립해 단계가 한 번 늦게 옮겨지는 순서와 old/new 변경 영역 사각형(`FrameDisplayTests` 의 [308,204,320,222]·[315,218,342,229] 등), 선택 표식·그림자·alternate 경로·전체 갱신 억제, 현재 프레임 범위 오류의 쓰기 전 거부, 연결 불일치 거부(표시 없는 Unpop/Pop·다른 풀/해시/spot·다른 타입 표), 두 배 프레임 범위와 해제 표식.
- 기존 Pop/Unpop/Factory/postPop 검사(`squid-pop`·`squid-unpop*`·`squid-postpop*`)가 그대로 통과한다.

최초 실패와 교정: 새 모듈은 위 fixture 전부를 첫 실행에 통과했다. 연결 불일치 검사에서 표시 없는 `SquidUnpop` 을 연결해도 구성 단계에서 거부하지 못했는데, 웹 `SquidPop.validateFrameBinding` 이 cpppj `SquidUnpop::ValidateFrameBinding` 에 대응하는 표시 장치 비교를 빠뜨렸기 때문이다. `SquidUnpop.validateFrameBinding` 을 추가해 해결했다.

## 3. 경계와 남은 것

- 실제 Renderer 화면 합성(픽셀 확인)·`SquidRenderer` 어댑터·SHP 헤더 판독(`ShapeDatabase.SquidMetrics`)은 월드 표시 단위(WB03-M2/WB05-C)에서 한다. 이번 변경 표 검사는 `DirtyRegions` 통지까지다.
- 마나/볼트 부분 표시와 신전 rune 부분 표시의 소비자(`RawCursorMana`·`RawManaDraw` 등)는 해당 파일 이식 단위에서 `cursor-mana-display`·`mana-bolt-display`·`vortex-display` fixture 로 대조한다. `SquidDisplay` 쪽 계산(inset·10픽셀)은 이번에 옮겼다.
- 간헐천 실제 Pop/Unpop/프레임 통합(cpppj `GeyserPartialDisplay_ActualPopUnpopAndFrameUseSameFixedOutline`)은 `RawAssetPostPop` 이 필요해 자산 PostPop 단위에서 한다.
- Pop 의 서버 전송 writer 는 이 검사에서 호출만 받는 연결 객체 없는 입력이다(패킷 바이트는 검사하지 않는다). 실제 `RawSquidTransmission` 은 패킷 단위 몫이다.

## 4. 다음 시작점

종류별 PostPop 본체(Asset·Fence·Nugget·Buried·Carrier·Bomb·Factory·Priest·Rune), 후보/지형 권한(`RawCanonPlacementPermission`), `RawSquidTransmission`/패킷 writer 이다.
