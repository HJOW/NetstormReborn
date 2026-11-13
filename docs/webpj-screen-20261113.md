# webpj Screen·ScreenPresentation — 화면 장치와 호환 출력 계산 (WB04-E393, 2026-11-13, 5-A 스물한 번째 단위)

5-A 렌더러 본체 항목(`Renderer`·`ClientRendererFrame`·`ScreenPresentation`·`Screen`) 중 **ScreenPresentation**(cpppj `client/ScreenPresentation.cpp/.h`, 기준선 71줄, partial adapt)과 **Screen**(`client/Screen.cpp/.h`, 818줄, partial adapt)을 `render/screen.js` 한 모듈로 옮겨 둘 다 port 로 올렸다. 기준선 뒤 cpppj 변경이 없다(base `1f00a25a`). `Renderer` 는 E391·E392 에서 끝났고 `ClientRendererFrame` 은 다음 단위다(선행 부품 판정은 5절).

## 1. 범위

- **순수 계산(DOM 없음):** `ScreenMode`(플래그 비트 상수)·`screenModeIsLegal`(원본 004a13d0)·`clipScreenRect`(00445290, `indexed-surface.js` 의 `clipRect` 와 같은 계산)·`modernScreenMode`·`fitScreenPresentation`·`mapScreenPresentationPoint/Rect`·`scaleScreenPresentation`·`scrollScreenPixels`(보호 004a1ad0).
- **`ScreenSurface`:** 종류 `dib`(8비트 위→아래 버퍼, **줄 간격 `(폭+3)&~3`** — 1366 폭은 1368)·`window`(픽셀 없음, 줄 간격=폭). `lock()`/`unlock()` 은 DIB 만 버퍼를 돌려준다.
- **`Screen`:** 원본 전역 묶음을 객체로 둔 화면 장치. `init`(004a1270)·`initDibSection`(004a4570)·`setMode`(004a4fe0 창 경로)·`setPalette`(004a2640)·`loadPalette`/`applySavedPalette`(004a4850 후단)·`lock`/`unlock`(004a14e0/004a1550)·`setClip`/`clip`/`pane`·`blitBuffer`(004a1fe0→004a0c10)·`fillRect`(004a2060)·`clear`(004a4790)·`update`(004a1800)·`flip`·`scroll`(004a1ad0)·`showCursor`/`restoreMovieCursor`/`cursorVisible`·`presentationRect`/`toLogicalPoint`/`setClientSize`/`setActive`·`colors`/`shadeMaps`/`bindColorMaps`/`bindColorSortInput`/`logicalPalette`/`palette`/`findColor`/`weatherTints`·`paletteLoaded`(클라이언트 소유 `{ value }` 상자).
- **웹 대체(옮기지 않음):** GDI 팔레트/장치 문맥·DirectDraw·창 모양/위치(`InitialWindowPosition`)·GPU 후처리(`SetPostProcess`·`SetPresentSize`·`ReadPresented`)·팔레트 파일의 BGRX **예약 바이트**(웹 `GamePalette` 가 보존하지 않으므로 E272 결정대로 RGB 파일처럼 이전 저장 값을 유지)·`loadNativePaletteFile`(FILE 직접 기록은 `adapt-file` 경계). 표시 장치는 `options.devices` 훅으로만 받는다: `present(rect, view)`(창 모드 복사)·`presentFullScreen(output, view)`(호환 전체화면 한 번)·`paletteChanged(current)`·`waitVerticalBlank()`·`cursor(visible)`·`foreground()`. 훅이 없으면 해당 출력만 건너뛴다(원본의 널 장치 문맥과 같다).

## 2. 규칙

- **모드:** `setMode(requested)` 는 `modernScreenMode` 로 정규화한다(`|0x80`; 전체화면이면 `|DIB`·`&~flip`, 아니면 `&~(소프트웨어마우스|flip|시차)|DIB` — 예: 111→235, 11→139, 10→138, 0→130). 같은 모드면 장치를 건드리지 않고 true, 허용되지 않거나 DIB/창 표면이 없으면 false. 바뀌면 `saved ← current`(현재 팔레트를 저장 팔레트로 복사)한 뒤 표면을 연결하고 `setPalette(0,256,null,true)` 로 같은 색을 다시 적용하고 커서 깊이를 `숨김→표시→표시`(최종 −1)로 맞춘다.
- **`setPalette(start,count,colors,apply)`:** 논리 팔레트(`R|G<<8|B<<16|4<<24`)는 apply 와 무관하게 갱신하고 0번은 검정·255번은 흰색(`0x00ffffff`)으로 고정한다. 인자 배열의 0번 칸(start 0 일 때)과 **끝이 정확히 256 일 때** 마지막 칸도 같은 값으로 덮어쓴다(원본 동작). apply 는 현재 팔레트에 복사하고 `paletteChanged` 를 부르며, **전체화면이면 화면 전체 `update`**(clip 을 화면으로 되돌림)를 요청한다. count 0 + apply 는 인자 전체(1024바이트)를 0번부터 쓴다.
- **`applySavedPalette(clear)`:** E376 의 `applyScreenPaletteClear` 관문(loaded 상자=1; clear·flags 가 비영일 때만 lock→검정 채우기(현재 clip 으로 자름)→unlock→수직 동기(전체화면만)→viewport 출력 뒤 clip 복구→flip 관문)을 지난 뒤 `setPalette(0,256,null,true)`, `buildPaletteColorTable(logical)`, 명암 표(`bindColorMaps` 조회기 → 없으면 `buildShadeMaps(bindColorSortInput 조회기 → 없으면 logical)`)를 만든다. 번개 같은 일시 `setPalette` 는 색 표를 다시 만들지 않는다.
- **잠금·채우기:** `lock()` 은 깊이를 세고 처음에만 그리기 표면의 버퍼를 얻으며(표면이 없으면 `drawSurf` 오류), `unlock()` 은 0 이 되면 버퍼를 놓는다. `fillRect` 는 현재 clip 으로 자른 뒤 **모드 설정 비트(0x80)와 잠금이 없으면 아무것도 하지 않고**, 쓰는 칸이 줄 간격/버퍼 밖이면(원본은 미정의 동작) 거부한다. `clear` 는 플래그 0 이면 반환, 아니면 잠그고 채우고 푼다(예외가 나도 푼다).
- **`blitBuffer`:** 소스 사각형을 대상 크기로 바꾸고(32비트 감김 좌표) 화면/현재 clip 으로 자른 뒤 4바이트씩(마지막 0~3바이트는 1바이트씩) **먼저 읽고 쓴다**(겹친 입력도 같은 순서). 잠금이 없거나 음수 폭/줄 간격, 소스·대상 물리 범위 초과, 소스 시작이 안전 정수를 넘는 값(대상이 비어 있지 않을 때)은 거부한다.
- **`update(rect)`:** 화면으로 자르고 **clip 을 화면 전체로 되돌린 뒤** 빈 사각형이면 보내지 않는다. 호환 전체화면은 활성이고 DIB 가 있을 때만 `presentFullScreen(presentationRect())`, 창 모드는 표면이 연결돼 있으면 `present(rect, view)`. `presentationRect()` 는 창 모드면 논리 전체, 전체화면이면 `setClientSize` 로 받은 클라이언트에 비율대로 맞춘 범위(크기를 모르면 논리 전체)다.
- **그 밖:** `flip()`/`presentPaletteClear()`(clip 복구)/`scroll`(DIB 모드에서만, 이동 0 이면 잠그지 않음)/커서 깊이(`softwareMouse` 가 없고 깊이<1 일 때만 하드웨어 커서 표시 알림)/`pane()`(양 끝 포함)/`borderOffset()`(웹은 항상 0).
- **`assets/palette.js` 의 `findPaletteColor` 정밀화:** 원본 x87 몸체는 채널 차이·곱·합을 32비트로 감아 부호 있는 거리를 만들고 새 정수 거리를 반올림하기 전에 이전 float32 최솟값과 비교한다. 바이트 범위 RGB 의 결과는 그대로이고 범위 밖 정수 RGB 도 원본과 같아지도록 `Math.imul`·`| 0` 으로 감았다.

## 3. 검사 (`tests/screen.test.mjs` 34건)

- **원본 독립 관측(첫 실행 일치):** `palettecolor-x86.tsv` **786행**(세 판본×262입력, 범위 밖 RGB 420행 포함; manifest `palettecolor` — 시험 첫 실행은 `findPaletteColor` 정밀화 전이라 범위 밖 행에서 실패했고 감김 산술로 고친 뒤 전부 일치), `screen-scroll-x86.tsv` **357행**(전체 144바이트 padding 포함; manifest `screen-scroll`, 첫 실행 일치).
- **cpppj 시험 이식:** `ScreenPalette_PreparedLogicalColorsAndWeatherTable`, `ScreenMode_LegalCombinations_FollowOriginalRules`, `ScreenRect_Clip_ClampsEachCoordinate`, `ScreenScroll_AllDirectionsPreserveSourceAndPadding`(독립 좌표 대조), `ScreenScroll_SelectedSurfaceBounds`, `ScreenPresentation_AspectFitAndLetterbox`, `…_InversePointAndDirtyPhase`, `…_CompatibilityFlags`(ModernScreenMode 부분), `…_WideDibUsesActualAlignedPitch`(1366→1368), `…_IntegerNearestPixelsAndBounds`.
- **웹 계약:** 새 화면 초기 상태, 모드 전환(표면 없음/같은 모드/전체화면/활성 훅/모드 0), 잠금 깊이, 채우기·지우기(모드 비트·clip·padding·표면 밖 거부), 구름 복사(clip·워드 읽고 쓰기·겹침·범위·안전 정수), 팔레트 설정(부분 범위·256 초과·count 0·인자 덮어쓰기)과 적재(전체 색 대조·예약 바이트 유지·색/명암 표·조회기)·clear 관문 순서와 clip 복구, 모드 전환 때 팔레트 유지, 출력 분기와 입력 변환, 이동·플립·커서 깊이, 검증 경계(각 조건을 따로 거부·정확히 맞는 경계 허용·조기 반환 순서), `findPaletteColor` 최소 거리/동률.
- **옮기지 않은 시험:** GDI 객체 누수(`ScreenPalette_ReplacementsAndDestructionReleaseGdiObjects`)·창 위치(`Screen_InitialWindowPosition`)·`InputQueue` 시험(`input-queue.js` 가 이미 검사)·게임 메뉴 명령 번호 확인·`ScreenScroll_CameraUsesActualRendererDirtyAndNextDraw`(카메라 적용 `RawCameraCommit` 단위 뒤)·`RendererPresentation_ActualCameraCommit…`(같은 이유).

## 4. 변형 확인

- `screen.js`(90~560행)의 연산자·상수 변형 400회 표본: 1차 생존 **88건** 중 시험 공백 67건(검증 경계의 정확히 맞는 경우·한 조건씩 거부·초기 상태·부분 팔레트 범위와 인자 덮어쓰기·count 0 의 짧은 인자·`blitBuffer` 음수 폭/줄 간격 0/안전 정수·4바이트 겹침·`fillRect` 거꾸로 사각형/줄 간격 끝·clear 전체 화면 첫 칸·출력 원점 비영·올림 위상)를 시험 보강 11건으로 잡았고 최종 **379/400 잡음, 21건 생존 — 모두 동등/비관측**이다: ① 팔레트 적용의 `modeSet` 가드(`&`→`|`, `0`→`1` 3건)는 `modernScreenMode` 가 항상 0x80 을 켜므로 도달 불가이고, ② 타입 배열의 범위 밖 쓰기·읽기는 무시되는 루프 상한(`<`→`<=` 4건), ③ 조기 반환 조건끼리 서로 가리는 `<=`→`<`·`||`→`&&`(영역이 0 이면 `|dx| >= 폭` 도 참; 6건), ④ 부호 비트가 없는 값의 `>>>`→`>>`, ⑤ 크기만 커지는 저장 팔레트 배열 길이·`Uint8Array(4)` 워드 길이, ⑥ 전체화면 출력 요청의 사각형 시작(출력 범위만 쓰임), ⑦ `blitBuffer` 의 안전 정수 경계(`>=`→`>`·`&&`→`||`·`origin` 기본값: 어느 쪽이든 같은 오류), ⑧ 대상이 이미 화면 안으로 잘려 도달 불가인 `to` 범위 검사(2건), ⑨ 이동 0 의 잠금 생략(`!dx && !dy`: 어느 쪽이든 관측 같음).
- `palette.js findPaletteColor`(93~114행) 28회: 1차 생존 3건 중 `| 0`→`| 1`(홀짝 인접 거리의 순서)은 직접 입력 시험으로 잡고 2건(`& 255` 뒤의 `>>>`→`>>`)은 동등이다 → **26/28**.

## 5. 남은 경계와 다음

- 앱 수명에 연결하지 않았다(`PORT_MAP` session 0). 실제 `Screen` ↔ `Renderer` 출력(`Renderer.present(update)` → `Screen.update`)·`presenter.js` 연결은 `ClientRendererFrame`·`ClientMain` 단위에서 한다.
- **`ClientRendererFrame` 선행 판정:** 기준선 소스(257줄)는 Client 본체·`RawMenuControls`(L, todo)·`ClientKencloudGlobals`(todo)·`TileGroupCollection`(todo)·`RawCampaignCursorDisplay`/`RawCampaignFactoryDisplay`(todo)·`GameWorld`(todo)·`ClientGameplayGlobals`(todo)를 직접 부른다. 5-A 목록의 같은 항목이지만 빈 성공 함수로 우회하지 않으므로 선행 단위(특히 `TileGroupCollection`·`ClientKencloudGlobals`)를 먼저 이식한 뒤 이어 간다.
