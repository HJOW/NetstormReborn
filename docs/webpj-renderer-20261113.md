# webpj Renderer — 장면 합성 본체 (WB04-E391, 2026-11-13, 5-A 스무 번째 단위)

5-A 렌더러 단위 중 client **Renderer**(cpppj `client/Renderer.cpp/.h`, 기준선 494줄, 상태 partial)를 `render/renderer.js` 로 옮겨 port 로 올렸다. 기준선 뒤 cpppj 변경이 없다(base `1f00a25a`). 원본 Renderer 의 변경 표 등록(`00497580`)·표시 정렬(`00497900`)·변경 영역 합성(`004994b0`)·출력(`00499fe0`)을 한 클래스로 묶는다. 변경 사각형 표 `DirtyRegions`(E59), 색 번호 그림 출력 `indexed-surface.js`, 16바이트 정렬 `draw-sort.js`, 글꼴 글자 출력 `glyph-text.js`, 셰이프 해독은 이미 있는 모듈을 그대로 쓴다. 이전 단위가 만든 앞단(`raw-renderer-prelude.js`)·구름(`raw-renderer-clouds.js`)·표시 제출(`draw-submission.js`)·진단(`raw-renderer-diagnostics.js`)을 한 프레임으로 잇는 일은 `ClientRendererFrame` 단위의 몫이다.

## 1. 규칙

- **생성:** `new Renderer(width, height, edition = 'patch1078')`. 10.78 만 받는다(다른 판본은 `RangeError`). 첫 화면은 전체 갱신이다.
- **장면 등록 `setScene(sprites, text, sort)`:** 이전 스프라이트와 새 스프라이트의 그림 범위를 모두 변경 표에 넣어 사라진 그림의 배경을 복구한다. 좌표가 유한하지 않으면 거부하고, 제출 순서의 사본(`#sourceSprites`)을 따로 보관한 뒤 16바이트 키(깊이 내림차순, y·x 오름차순)로 정렬한다. 글자는 이전 또는 새 목록이 비어 있지 않으면 전체 갱신이다. 입력 배열은 바꾸지 않는다.
- **그림 범위:** SHP 스프라이트는 본 그림과 후속 그림의 합집합(오른쪽·아래 끝은 한 칸 더해 포함하지 않는 경계, 특수 프레임은 범위 없음), 직접 그리기(`procedure`)는 선언한 `bounds` 를 클립한 값이다. 32비트 정수 범위를 넘는 좌표는 감기지 않고 거부한다.
- **`draw(pixels, pitch, background, world)`:** `draw` 뒤에는 반드시 `present` 가 와야 한다(아니면 `Present must follow Draw`). 전체 갱신이면 표를 비우고 화면 전체 한 항목(flags 0)을 만든다. 표의 각 항목은 화면에 자르고, 버려진(flags 1) 항목은 건너뛰며, 면적 없는 항목은 그리지 않지만 출력 목록에는 남는다. 변경 영역마다: (월드 없음) 불투명(4)이 아니면 배경색을 칠하고 배경 그림을 복원 → (추가 수집기가 있으면) 제출 순서 원본에 수집 결과를 합쳐 다시 정렬하고, 불투명(4)이 아닌 영역의 수집 결과만 캐시에 남김 → (월드 있음) `collect → background → submit` 순서, UI 전용(4) 영역은 월드·미리보기 표시를 제출하지 않음 → 정렬된 항목 출력 → 선택 표시(월드가 있으면 UI 전용 영역에서는 생략) → 현재 메뉴 직접 그리기(영역으로 자름) → 글자 → 소프트웨어 커서.
- **`present(update)`:** 카메라(`Renderer+5c`)·배경 시차(`+60`) 다시 그리기 요청 `{ value }` 상자가 켜져 있으면 요청을 끄고 표를 화면 전체 한 항목으로 바꾼다(첫 항목 flags 는 보존, 빌려 준 `cameraDirtyEntries()` 배열 자체는 유지). 버려지지 않은 항목을 표의 순서대로 `update` 에 넘기고 표를 비운다.
- **`sceneImage()`:** 정지한 미션 위에 대화상자를 올릴 배경을 만든다: 배경 → 마지막 표시 목록(추가 수집 캐시가 있으면 제출 순서 원본에 합쳐 다시 정렬) → 선택 표시. 변경 표·출력 상태·커서·메뉴 글자는 건드리지 않고 불투명도는 전부 255.
- **글자:** 그림자(2)·외곽선(4)·본문 패스를 `glyph-text.js` 로 현재 변경 영역 안에서 그린다(글자 자체 clip 과 교집합). 글꼴이 없으면 거부한다.
- **`setSoftwareCursor(image, at, force)`:** 같은 그림·같은 위치면(force 아님) 아무것도 하지 않고, 아니면 이전·새 사각형을 모두 변경 표에 넣는다. 그림이 `null` 이면 표시를 지운다.
- **웹 대체:** 셰이프 해독 캐시는 `ShapeDatabase.decode` 가 소유하므로 `clearDecodedImages()` 는 비울 것이 없어 아무 일도 하지 않는다. `dirty-regions.js` 의 `clear()` 는 배열을 새로 만들지 않고 길이를 0 으로 해(C++ `vector::clear`) `cameraEntries()` 로 빌려 준 살아 있는 표가 계속 같은 배열이 된다.

## 2. 웹 범위

- 앱 수명에 연결하지 않았다(`PORT_MAP` session 0). 이 모듈은 DOM 을 쓰지 않는다. 월드 장치(`world`)와 추가 수집기는 호출자가 공급한다.
- `ClientRendererFrame`(앞단·구름·표시 제출·진단과 Renderer 를 한 프레임으로 이음), `ScreenPresentation`·`Screen`(화면 모드·출력 장치)은 다음 단위이다. cpppj `RainDrawTests` 의 Renderer 합성 2건도 그 뒤에 재대조한다.
- `RendererTests.cpp` 의 `BitmapFont_GdiFallback`·`Cursor_DefaultResourceMapping` 은 GDI/리소스 장치 단위(글꼴·커서)의 몫이라 옮기지 않았다.

## 3. 검사 (`tests/renderer.test.mjs` 23건)

- **원본 독립 관측:** `renderer-x86.tsv` 의 Order **261행**(표시 비교 반환값)과 Dirty **133행**(변경 표 전체: 전체 갱신 여부, 무시된 항목의 위치·플래그 포함)이 **첫 실행에서 전부 일치**했다. manifest id `renderer`.
- **cpppj 시험 이식:** `Renderer_ColorMapsAndShadows`, `Renderer_MovementDepthAndUnchangedFrames`, `Renderer_SoftwareCursor`, `Renderer_ExtraCollectionUsesDirtyOrderAndCachedSceneImage`, `Renderer_ActualMenuOverlayCurrentClipAndCursorOrder`, `Renderer_OrderAndDirtyRegions`.
- **웹 계약:** 생성·표면·그리기 순서·배경/선택 표시 크기 검증·직접 그리기 경계, 정렬 키 거부와 제자리 정렬, world 장치 순서와 UI 전용(4), 카메라/배경 요청과 살아 있는 표, 글자 패스·clip·필드 기본값, SHP 합성(후속 그림·색 변환·그림자), 배경·선택 표시 복원, `sceneImage`(원점 배치·정렬 목록 재사용), 추가 수집 캐시(불투명 영역은 갱신 안 함), 그림 범위·32비트 좌표 경계, 면적 없는 항목, 버려진 항목, 요청 상자 초기값.

## 4. 변형 확인과 남은 경계

- 자동 연산자·상수 변형 121개를 적용했다. 1차 생존 21건 중 5건(버려짐 플래그 값, 요청 상자 초기값 둘, `sceneImage` 선택 표시의 원점 두 위치)은 시험 공백이어서 독립 입력을 보강해 모두 잡았다(재확인 5/5). `dirty-regions.js` 의 이번 변경 두 곳(`clear` 배열 유지, `cameraEntries` 살아 있는 표)은 수동 변형 2/2 를 잡았다.
- 남은 16건은 동등 변형이다: ① 비트 0 은 "버려짐" 표식이라 그려지는 항목의 flags 에 남지 않으므로 불투명 값 4→5 와 `flags & 4`→`& 5` 는 구분되지 않는다(2건), ② `invalidate` 의 기본 flags 0→1 은 `add` 가 비트 0 을 지우므로 같다, ③ 커서 초기 위치는 그림이 `null` 이면 어디서도 읽히지 않는다(2건), ④ 그림이 없는 경우의 빈 범위 `{0,0,0,0}` 한 좌표만 1 로 바꿔도 `right` 또는 `bottom` 이 0 이라 여전히 비어 있다(4건), ⑤ `glyph-text.js` 는 flags 의 비트 2·4 만 읽으므로 그림자/외곽선 값 2→3·4→5 와 꺼진 값 0→1 은 같고(4건), Renderer 가 항상 `clip` 을 넘기므로 표면 `bounds` 의 왼쪽·위 0→1 은 읽히지 않는다(2건), ⑥ 월드가 없을 때 `covered` 는 쓰이지 않으므로 0→1 이 같다.
- 다음 5-A 항목: `ClientRendererFrame`(+`ClientRendererFrameGlobals.h`), `ScreenPresentation`, `Screen`, `RawCursorPreviews`(커서 전역 K 영역 선행), cpppj Session 시험 재대조.
