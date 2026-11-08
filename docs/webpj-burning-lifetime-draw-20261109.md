# webpj Burning 출력·소멸·프레임 판정(RawBurningLifetimeDraw) 이식 — WB04-E184

2026-11-09, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 백여든네 번째 부분이다. 실제 Burning 입자 출력(`423ec0`)과 소멸(`423cd0`/`4240f0`), RenderProcess 의 프레임 진입 판정(`487940`), 71(발사기)/128(캐논)의 `+a4` 캡처 그림 제출을 맡는 `o/RawBurningLifetimeDraw`를 옮겼다(cpppj 기준선 `1f00a25a`, 기준선 뒤 `RawBurningLifetimeDraw.cpp/.h`·`BurningLifetimeDrawTests.cpp`·관련 fixture 의 변경 없음을 `git diff --numstat` 으로 확인). E183 의 `RawBurningVisual`·`RawRenderRegistry` 위에 얹어 세 fixture 합 871행을 실제 PE 와 대조한다.

## 1. 옮긴 것

`www/core/raw-burning-lifetime-draw.js`(DOM 없음)

- `drawBurningVisual(fields, state, palette, pixel)` — 별도 source 입자는 그리지 않고 양수 수명(ordered `life > 0`)의 이동 입자만 사용 수(`total`, capacity 가 아님)까지 순회한다. 각 입자는 좌표를 먼저 0 방향으로 정수화 → 카메라를 빼 float 에 저장 → 색을 조회(수명을 9 로 제한한 값 × 색수 × float(1/9)의 x87 곱, 0 이상으로 제한, 수명 9 에서는 인접 DWORD 를 읽을 수 있음)한 뒤 두 번째 정수화를 하고, 현재 dirty clip 의 5점 (x, y)·(x+1, y)·(x+1, y−1)·(x, y−1)·(x, y−2)이 모두 clip 안일 때만 그 순서로 `pixel(x, y, color)` 를 호출한다. `state = { cameraX, cameraY, viewport }` 는 전체 viewport 로 확대하지 않는 현재 clip 이고, `palette = { count: 8, readable }` 의 `readable` 은 같은 실제 할당의 유효 snapshot 이며 임의 끝색을 추가하지 않는다.
- `destroyBurningVisual(fields, registry, flags, release, stopSound)` — 캡처 이미지 → 행 표식 → 소리 핸들 → 입자 버퍼 순서로 존재할 때만 `release(BurningRelease.*)` 하고 비운 뒤 표시 등록 제거(base 소멸) → flags bit 0 이면 객체 반납이다. 공유 팔레트는 월드 소유라 반납하지 않는다. `renderProcessFrameDue(slot, frame)` — 슬롯이 7 미만(signed)이거나 (slot XOR frame)의 하위 2비트가 0 일 때만 파생 프레임으로 간다.
- `isBurningArcherCapture(raw)`·`drawBurningArcherCapture(raw, base, muzzleX, muzzleY, draw, x, y)` — 복원한 71/128 의 실제 두 가상 표(0x50b888/0x50b9b8)와 TYPE 를 함께 확인하고, 캐논은 그림을 제출하지 않고 발사기는 extra(+40 의 0x9 비트)·현재 프레임(+36)과 기본 프레임의 비교·출력 WORD bit 0 에 따라 `draw(71, 현재 프레임, x, y)` 와 조건부 포구 디스크 `draw(83, 0, x + muzzleX, y + muzzleY)` 를 순서대로 제출한다.

## 2. 검증

`tests/burning-lifetime-draw.test.mjs`(6건). 입력만 설치하고 모듈이 계산하며 기대 열은 비교에만 쓴다.

- `burning-lifetime-draw-x86.tsv` D 579행(5픽셀 출력의 수명/좌표/clip 과 실제 쓰기 순서, 64×64 표면의 Adler-32·합), X 64행(선택 버퍼/flags 별 반납·소리·등록/base 순서), C 12행(최초 색 표 생성자의 의미 27 DWORD·입자 zero·부착/확보·첫 gradient 인수 6 double 비트).
- `render-frame-gate-x86.tsv` 128행(signed slot 과 현재 DWORD frame 의 `487940` 반환), `burning-archer-capture-x86.tsv` 88행(71/128 의 +78/+a4 그림 요청, 포구 (−7, 11) 고정).
- 계약: 입자/출력 연결 오류 거부, capacity 0 의 무동작, 다른 발사기/미연결 draw 경계 거부, 미연결 반납/소리 경계 거부.

최초 실패와 교정: 새 모듈과 시험 6건은 모든 fixture(871행)를 첫 실행에 통과했다.

## 3. 경계와 남은 것

- `RawBurningFrame`(캡처·단계·visible·timer·source·이동 입자·dirty 영역)은 보호 SHP 의 실제 캡처(`ShapeToBuffer`)와 실제 dirty 표 소비자가 필요해 후속 단위이다(fixture `burningframe-x86.tsv` 213행). `BurningVisualSystem`·`RenderProcessSystem` 의 Host 조립과 `DrawBurningVisual` 의 실제 팔레트/Surface 연결도 후속이다.
- 10.72(CD)와 10.37 관측은 범위 밖이다.

## 4. 다음 시작점

`RawBurningFrame`(캡처 자산 필요)·`BurningVisualSystem`·`RenderProcessSystem`, `RawPacketDispatch`→`RawConstructionCommand`, Missile Host 팩토리, `RawWindAviaryProcess`·`RawWindFlyer`, 번개 수명/프레임/화면, `RawPathProcess` 계열 등 [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 0.1절 "마일스톤" 행의 목록을 따른다.
