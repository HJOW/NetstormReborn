# webpj 메인 메뉴 복원 (WB03-M1, 2026-10-23)

2026-10-23, **VM-W11-Codex**. 사용자 요청("webpj 의 메인 메뉴 화면까지만, 기존 게임과 동일 수준의 유사성, Quit·Exit 등 웹에서 쓸 수 없는 버튼 제외, 에셋은 webpj/www 내부로 복사")에 따라 웹 클론의 **로딩 → 클릭하여 시작 → 원본 메인 메뉴**를 구현했다. 근거는 원본 10.78 디컴파일 산출물(`extracted/refined/originals/`, 마지막 전체 정밀 디컴파일 **VM-W11-Codex / 2026-10-23**, 현재 PC와 일치하여 재디컴파일 없음)을 1순위, cpppj 소스를 2순위로 삼았다. dotnetpj 는 참고하지 않았다. 원본 게임·분석 프로그램은 실행하지 않았고 `originals/`·`originalCD/`·AGENTS.md 는 수정하지 않았다(`originals/d/!color.dat` 은 검사에서 읽기만 한다).

## 1. 원본 근거

| 대상 | 원본(10.78) | 웹 구현 |
|---|---|---|
| 메인 메뉴 Gump 생성 | `004d0120`: 버튼 칸 9개(Campaign·Multiplayer·Demo·Help·Edit·Intro·Credits·Options·Quit), Intro 는 CD 가 있을 때(`DAT_0054db98`)만 생성, 라벨은 번역 조회(`xlat.english` 는 원문 그대로) | `ui/main-menu.js` `MAIN_MENU_SLOTS` |
| 버튼 배치 | `004ce940`(가상 +0x20): 모든 버튼을 글자에 맞춘 뒤 최대 폭으로 통일, 윗줄 0~3번·아랫줄 4번 이후 칸을 빈 칸(null) 건너뛰며 가운데 배치. 간격 float 4.0(`00502314`), 계수 0.5(`00500ee4`), 시작 누적 0.0(`00500fc4`), y = 화면높이/3 + 0x37 / + 0x4e | `layoutMainMenuButtons`·`placeRow` (float32 누적) |
| Gump 위치 | `004d0530`: 메인 메뉴 Gump 는 화면 전체 크기(폭 = 화면 폭)로 (0,0)에 부착 | 표면 폭 = 논리 폭 |
| 돌 버튼 | 생성 `004254b0`(문맥 null → 기본 문맥 `0052f974`, 배경 자식 `004655d0(w-2,h-2,0x3e00000,1,DAT_005ca920,DAT_00542664)`), 크기 맞춤 `00424f90`(글자 폭+12·높이+6), Draw `00424ff0` | `ui/stone-button.js`, `render/gump-visual.js` |
| 버튼 질감 | 타입 `DAT_005ca920 = DAT_005411c8 = 92`(fortGump = 70 + 블록 22), 프레임 `DAT_00542664 = 1`(정적 초기값, 쓰기 참조 없음) → **fortGump 클러스터 A01 "Sandy marble"** | `STONE_BUTTON_TEXTURE_FRAME = 1` |
| 타이틀 | UberGump 생성자의 배경 이미지 Gump(`titleMenu`): x = float(화면폭/2) − 그림폭×0.5, y = float(화면높이/2) − 그림높이×0.5, 정수 절삭. `titleMenu.gif` 는 **639×480** | `titlePosition` |
| 구름 | `00470bb0`: `CloudFileSpec` GIF 가 512×512 일 때만 사용. 화면 모드 기본값(창 10·전체화면 11)에 시차 0x10·움직임 0x20 비트가 없어 정지 타일 | `surface.tile(clouds)` |
| 팔레트 | 로더 `004a4850` → `004a2640(0,256,0,1)`: 논리 팔레트·DIB 색 표의 **0번 검정·255번 흰색 고정**(GIFCLOUD.COL 255번은 청록색) | `buildScreenPalette` |
| 명암 표 | `0043c010`: 밝음 1.5배(최대 1)·어두움 0.6배, L1 거리, 최소값 float32 저장, 밝음 후보 229~245 제외 | `buildShadeMaps` (원본 캐시 `!color.dat` 와 512바이트 일치) |
| 글꼴 | 슬롯 0 = `!Arial.normal.14.700.chfnt`, 측정은 첫 표 합 + 논리 높이 14(그림자 플래그 없는 기본 문맥) | `assets/bitmap-font.js`, `measureGumpText` |
| 입력 | 누름은 정수 Bounds(도움말 플래그 0x200 의 BaseFilter), Broadcast·뗌은 float 지역 좌표 [0,폭)×[0,높이). 누름 즉시 눌린 모양 + `button.wav`, 안쪽에서 뗄 때만 click 통지, 오른쪽·가운데 버튼·키보드 무반응 | `StoneButton.pointerDown/broadcast/pointerUp` |
| 소리 | 효과음 버퍼 0xe2(GLOBALFOCUS 없음), 음량 단계 → 1/100 dB(1=-40, 2=-20, 3=-10, 4=-5, 그 밖 0), 기본 효과음 3·음악 2, 메뉴 곡 `ser22.mus` 반복(파일 끝에서 되감기) | `audio/audio-system.js` |
| 창 제목 | `config.english` `BlankWindowName = "NetStorm Main Menu"` | 메인 메뉴에서 `document.title` |
| 커서 | 기본 커서 번호 1 → 그룹 113 → `RT_CURSOR_8`(32×32 단색, 핫스폿 10,6) | `ui/cursor.js` (CSS 커서) |

1024×768 의 원본 8버튼 배치는 이 알고리즘으로 x **356/435/514/593**, y **311/334**, 75×20(그린 높이 19 + 판정 1)이 되어 기존 관측([메인 메뉴](screens/main-menu.md), [버튼 동작](videos/menu-buttons-20261003.md))과 일치한다. 원본 버튼 폭 75 는 가장 넓은 라벨 "Multiplayer"(측정 63)+12 다.

## 2. cpppj 와 다른 점 (원본 근거 우선)

- **돌 버튼 질감 프레임:** cpppj `UberGump.cpp` 는 `FrameIndex(fortGump,"A00",0)`(프레임 0)을 버튼 배경으로 쓴다. 원본 전역 `DAT_00542664` 는 1 이며, `screenShots/mainMenu.png` 버튼 내부 픽셀을 질감 위상 전체(199×145)에 대해 탐색한 결과 프레임 1·위상 (0,0)의 평균 오차가 32.5(세 채널 합)로, 프레임 0 의 최적 위상 90.8 보다 훨씬 작았다. 웹은 프레임 1 을 쓴다. cpppj 수정은 이번 범위 밖이며 [공통 인계](../LEFT_JOBS.md)에 남겼다.
- cpppj 메인 메뉴는 고정 75×19·피치 79 어댑터이며, 웹은 원본 `004ce940` 배치 알고리즘을 그대로 옮겼다.

## 3. 웹 적응

| 항목 | 원본 | 웹 |
|---|---|---|
| Quit | 프로그램 종료 | 칸을 비움(웹에서 종료 불가, 사용자 지시) |
| Multiplayer | LAN/서버 접속 | 칸을 비움(AGENTS.md: webpj 는 멀티플레이 제외, LEFT_JOBS.webpj 1절) |
| Intro | CD 탐지 시에만 | 비움(원본의 CD 없음 경로와 같음) |
| Edit | 지도 편집기 | 칸을 비움(AGENTS.md webpj 목표 변경: 커스텀 맵 Edit 제외, **2026-10-28 반영**) |
| 결과 배치 | 4+4, 폭 75 | 원본 알고리즘 그대로 **3+2**(윗줄 Campaign·Demo·Help, 아랫줄 Credits·Options), 최대 폭 = Campaign 55+12 = **67**. 1024×768 에서 윗줄 x 407/478/549, 아랫줄 x 443/514(y 311/334), 1366×768 에서 윗줄 578/649/720·아랫줄 614/685. 윗줄 첫 버튼 위치가 407.5(반 픽셀)라 원본처럼 누름(정수 407~473)과 뗌(float 408~474) 판정이 1픽셀 어긋난다. (2026-10-23 기록은 Edit 포함 3+3, 아랫줄 x 407/478/549 였다.) |
| 시작 | 실행 즉시 메뉴 | "클릭하여 시작" 입력 안에서 AudioContext(22,050Hz 요청)를 만든 뒤 메뉴 진입 |
| 포커스 상실 | 소리 들리지 않음 | 탭 숨김 시 AudioContext suspend, 복귀 시 resume |
| 화면비 | 4:3 해상도 | 논리 높이 768, 16:10 = 1228×768, 16:9 = 1366×768. 원본 배치식이 화면 폭/높이를 쓰므로 구름은 넓게 깔리고 타이틀·버튼은 가운데에 놓인다 |
| 확대 | 모니터/장치 | 8비트 표면 → 팔레트 RGB → 정수 배율 최근접 확대 후 부드러운 축소(`?scaling=sharp` 기본, `nearest`·`smooth` 선택) |
| 커서 | OS 커서 리소스 | 같은 그림을 정수 배율(화면 배율 반올림, 최대 4배)로 키운 CSS 커서, 고해상도는 image-set |

메뉴 버튼이 여는 다음 화면(캠페인·데모·크레딧 Tell 대화상자, 도움말·옵션 펼침 메뉴)은 이번 범위가 아니다. (Edit 지도 목록은 2026-10-27 목표 변경으로 제외. 2026-10-28 에 Tell 본문을 읽는 데이터층만 추가했다: [데이터층 기록](webpj-data-layer-20261028.md).) 실행 시 `onMenuCommand` 훅과 콘솔 안내만 한다(**2026-10-28 WB03-D 부터 Campaign·Demo·Credits 의 Tell 명령은 대화상자를 연다**: [Tell 대화상자 기록](webpj-tell-dialog-20261028.md). Help·Options 는 아직 안내만). 시작 시 "Did You Know?" 팁 창(tellTips=1), 45초 무입력 Auto-Demo, F1 도움말도 미구현이다.

## 4. 자산 묶음

`webpj/tools/prepare-assets.js` 가 `assets/game-data` 에서 8개 파일을 **변환 없이** `webpj/www/data/` 로 복사하고 `manifest.json`(경로·출처·묶음·크기·SHA-256·용도)을 쓴다. 복사본은 원본과 같은 Git blob 이라 저장소 크기가 늘지 않으며 `.gitattributes` 의 `webpj/www/data/** -text` 로 체크아웃마다 바이트를 보존한다.

| 묶음 | 파일 |
|---|---|
| `menu`(메뉴 표시 전 필수, 약 7.0MB) | `d/GIFCLOUD.COL`, `d/Gifcloud.gif`, `d/titleMenu.gif`, `d/_shapes.shp`, `d/!Arial.normal.14.700.chfnt`, `cursors/RT_CURSOR_8.bin`, `sound/button.wav`, (2026-10-28 추가) `netstorm.tarc`, `d/setup.cfg` |
| `menu-music`(뒤에서 받음, 약 18MB) | `music/ser22.mus`(RIFF WAV PCM 16비트 스테레오 22,050Hz) |

브라우저 적재기(`www/assets/asset-loader.js`)는 매니페스트를 읽어 묶음을 스트림으로 받으며 진행률을 로딩 화면에 표시하고 크기를 확인한다. 원본 설치 폴더·`originals/` 를 찾지 않는다.

## 5. 모듈 구조

```text
webpj/www/
  netstorm.js            앱 수명·상태 전환·입력·프레임(기존 WB01 + 자산/메뉴/소리 연결)
  package.json           {"type":"module"} — Node 검사에서 www 의 ES 모듈을 직접 import
  assets/                asset-loader · menu-assets · palette · gif · shapes · bitmap-font · text-encoding
  render/                indexed-surface(8비트 합성) · glyph-text · gump-visual(질감·외곽선 계획) · presenter(RGB 변환·확대)
  ui/                    stone-button · main-menu · cursor
  audio/                 audio-system(Web Audio)
  data/                  원본 자산 복사본 + manifest.json
webpj/tools/prepare-assets.js   자산 복사·매니페스트(--check 검사)
webpj/tests/*.test.mjs          Node 내장 검사(npm --prefix webpj test)
```

CSS 파일은 만들지 않았고 `index.html` 은 바꾸지 않았다.

## 6. 검증 (VM-W11-Codex, Node v24.19.0, Edge 헤드리스, 2026-10-23)

> **2026-10-28 갱신:** 아래 결과는 Edit 를 포함한 6버튼(3+3) 기준이다. Edit 제거 뒤 웹 배치 검사는 5버튼(3+2) 기준으로 바뀌었고(원본 8버튼·스크린샷 대조·입력 규칙 검사는 그대로) 전체 검사는 데이터층 검사를 더해 25개다. 5버튼 배치는 1024×768·1920×1080 에서 Edge 헤드리스로 다시 확인했다([데이터층 기록](webpj-data-layer-20261028.md) 3절). 원본 8버튼 스크린샷과 달라지는 칸(Multiplayer·Edit·Intro·Quit)은 '웹 제외 칸'이며, 영어 4:3 메인 메뉴의 동일성 평가는 이 칸을 뺀 부분에만 적용한다.

- **Node 검사 17개 통과**(`npm --prefix webpj test`): 매니페스트·복사본 크기/SHA-256, 팔레트 0/255 고정·버튼 색(검정 0·흰색 195·외곽선 15), **명암 표 = 원본 `!color.dat` 512바이트 일치**, GIF(타이틀 639×480·구름 512×512), 셰이프 116블록·질감 메타 200×146/그림 199×145, 글꼴 높이 14·라벨 폭(Multiplayer 63 등), Windows-1252, 커서(32×32·핫스폿 10,6·반전 0), 원본 8버튼 좌표, 웹 6버튼 좌표(1024/1366 폭), 원본 800×600 좌표, 타이틀 위치, 돌 버튼 입력 규칙(경계 x 434/435/509·y 353/354, 끌기·이웃 버튼·바깥 누름·취소), 반 픽셀 판정 차이, 배경·외곽선·글자 위치 계획, 화면 배치 계산, 음량 변환.
- **원본 스크린샷 대조:** 원본 8버튼 구성을 1024×768 로 합성해 `screenShots/mainMenu.png`(창 테두리 포함, 클라이언트 원점 (2,33))와 비교했다. 스크린샷의 손실 압축 잡음 때문에 정확 일치 대신 평균 채널 차이를 쓰며 전체 **2.7**, 타이틀 4.0, 버튼 6.3, 픽셀의 99.85%가 ±32 이내다. 프레임 0(A00) 질감보다 프레임 1 이 원본에 가깝다는 것도 검사한다.
- **실제 브라우저(CDP):** 1024×768, 1920×1080, 1280×800, 2560×1440, 1600×1000(@2배), 800×600, 900×1400(세로), 1366×768 에서 모드·버튼 좌표·클릭 시작·메뉴 진입·`NetStorm Main Menu` 제목·원본 커서 적용, Credits 누름(눌린 모양) → 누른 채 바깥 이동(평소 모양) → 바깥 뗌(불발) → 정상 클릭(실행 1회) → 우클릭(무반응), AudioContext `running`·22,050Hz·`button.wav` 등록·`ser22.mus` 재생, destroy/init 반복(Canvas 1개·제목 복원)을 확인했다. 1024×768(@1) 화면 캡처는 Node 합성 결과와 **786,432픽셀 모두 같다**. 콘솔 오류는 `favicon.ico` 404 뿐이다.
- **확인하지 않은 것:** 실제 스피커 출력(헤드리스), Chrome·Firefox·리눅스, 터치, 장시간 실행, 실제 모니터 이동의 배율 변경, 원본 실행 화면과의 와이드 비교(원본은 와이드 미지원).

## 7. 남은 일

1. 메뉴 버튼이 여는 화면: Tell 대화상자(StyleText/HTML 본문·돌 패널·목록 행) → Campaign·Demo·Credits, MenuGump 펼침 메뉴 → Help·Options(웹에서 해상도·전체화면 항목 제외). (Edit 지도 목록은 제외. 본문 조회 데이터층은 2026-10-28 완료, 그림은 남음.)
2. 시작 팁 창(`tellTips`·`tipNumber`), 45초 무입력 Auto-Demo(데모 미션 복원 뒤), F1 도움말.
3. 한국어: 버튼 라벨 번역과 D2Coding 글리프 출력 경로(원본 글꼴에는 한글이 없다). 현재 `?lang=ko` 는 웹 전용 화면 문구만 바꾼다.
4. cpppj 의 돌 버튼 질감 프레임(A00→프레임 1) 정정 여부 확인과 수정(공통 인계 참고).
5. 사용자 설정(음량·음악 끄기)의 저장과 옵션 메뉴 연결, favicon(원본 아이콘) 여부.
