# webpj 한국어 지원 1단계: D2Coding 글자 출력·번역표·메인 메뉴/펼침 메뉴 라벨 (WB03-N, 2026-10-31)

2026-10-31, **HJOW-X3D**(마지막 전체 정밀 디컴파일 호스트 HJOW-Athlon/2026-10-23 과 다른 PC 이지만 이번 단위는 디컴파일 소스를 읽지 않았다. 근거는 cpppj `o/Xlat.cpp`·`client/ClientMain.cpp`, [원본 다국어 체계 문서](formats/xlat.md), dotnetpj 인계의 영어/한국어 글꼴 결정, 기존 webpj 코드다). 사용자 요청("AGENTS.md, LEFT_JOBS.webpj.md 를 읽고 webpj 다음 작업 진행")으로 [인계](../LEFT_JOBS.webpj.md)의 '다음' 첫 항목인 **한국어(D2Coding 글꼴·언어 자료·한국어 라벨)** 를 했다. 작업 도중 사용자가 **"fonts 안에 구한 woff2 를 webpj 안으로 복사해 사용하라"** 고 지시해, 처음 계획했던 TTC 에서 글꼴을 꺼내는 방식을 버리고 `fonts/D2Coding.woff2`·`fonts/D2CodingBold.woff2` 를 그대로 복사해 쓴다.

`cpppj/`·`dotnetpj/`·`originals/`·`originalCD/`·AGENTS.md·공통 `LEFT_JOBS.md` 는 수정하지 않았다(다른 세션이 같은 저장소에서 cpppj 를 `CP` 커밋으로 계속 진행 중이라 읽기만 했고 내 파일만 커밋했다). 원본 게임·analyzeManager 는 실행하지 않았고, 브라우저는 우리 웹 클론만 띄운 Edge 헤드리스다.

## 1. 근거

| 항목 | 근거 |
|---|---|
| 번역표 방식 | 원본은 영어 문자열을 코드에 두고 실행 중 `d/xlat.<언어>` 번역표로 바꾼다. 형식·읽기 규칙은 [xlat.md](formats/xlat.md)와 cpppj `o/Xlat.cpp`(`*` 다음 줄부터 원문, `-` 앞까지 원문·뒤부터 번역문, `=` 앞까지 번역문, CR 제거, 줄바꿈 하나만 뗌, 덮어쓰기, 미완성 블록 버림, 대소문자 구분 완전 일치, 번역이 없으면 원문). cpppj `ClientMain.cpp` 550행이 `d/xlat.<language>` 를 읽고 `Translate` 로 창 제목·Loading 문구·캠페인 문구를 바꾼다. 메인 메뉴 라벨도 번역 조회를 거친다([메인 메뉴 복원 기록](webpj-main-menu-20261023.md): "xlat.english 는 원문 그대로"). |
| 언어별 글꼴 | dotnetpj 인계 3절 2번(2026-10-10 사용자 결정): 언어가 영어이면 원본 비트맵 글꼴(`.chfnt`), 한국어이면 D2Coding. 영어 글꼴에 없는 글자는 D2Coding. dotnetpj 계획 0.8 2번: 한국어 D2Coding 글리프를 **8비트 화면에 단색으로** 그린다. |
| 글꼴 파일 | 프로젝트 `fonts/` 의 사용자 제공 `D2Coding.woff2`(1,486,740바이트)·`D2CodingBold.woff2`(1,580,004바이트)·`License.txt`(SIL OFL 1.1). 둘 다 `wOF2` 서명이며 sfnt 크기 4,185,576·4,359,732바이트(TrueType 윤곽). 버전은 파일에서 확인하지 않았다. |
| 기준선·줄 높이 | 원본 글꼴 캐시의 슬롯별 값: 높이 14 → 기준선 11·아래 3, 높이 20 → 19·15·4 등. D2Coding 은 hhea 850+230(unitsPerEm 1000)이다. |

## 2. 구현

| 파일 | 내용 |
|---|---|
| `www/assets/xlat.js`(신규) | `XlatTable`: 번역표 읽기·`translate`·`has`·`size`. cpppj `Xlat.cpp` 와 같은 상태 기계. |
| `translations/xlat.korean`(신규, UTF-8) | 한국어 번역표 31개 블록: 메인 메뉴 버튼 9개(웹에 보이는 5개 + 원본 비교용 4개), Options·Help 펼침 메뉴 라벨(`Volume 1~5`·`General Help - ~iF1~.` 포함), 창 제목. 글자 모양 표시(`~i`)는 번역문에도 둔다. |
| `www/assets/unicode-font.js`(신규) | `UnicodeGlyphFont`(BitmapFont 와 같은 인터페이스: `height`·`ascent`·`descent`·`glyph(code)`·`measure(units)`), `UnicodeFontProvider`(슬롯·스타일별 글꼴). 래스터라이저의 알파를 문턱(110)으로 잘라 **글자 색 번호 100 한 가지**로 만들어, 기존 글자 그리기가 그림자·외곽선·본문 패스를 영어 글리프와 똑같이 칠한다. 밑줄·취소선은 글자 칸 전체에 선을 긋는다. |
| `www/platform/glyph-rasterizer.js`(신규) | 브라우저 래스터라이저. `FontFace` 로 보통체(400)·굵은체(700) woff2 를 `D2Coding` 이름으로 등록하고(CSS 파일 없음) 글자 하나를 Canvas 2D 로 그려 정수 전진 폭과 잘라낸 알파 그림을 돌려준다. |
| `www/assets/korean-assets.js`(신규) | `buildKoreanSupport(files, { createRasterizer })` → `{ translate, unicode, translations }`. 한국어 묶음(`korean`)의 경로 상수. |
| `www/assets/text-encoding.js` | `encodeUtf16Units`(UTF-16 코드 단위 `Uint16Array`). |
| `www/assets/font-set.js` | `FontSet(files, { unicode })`: `encode(text)`(유니코드 글꼴이 있으면 `Uint16Array`, 없으면 Windows-1252 `Uint8Array`), `unicodeFont(context)`(줄 높이·기준선은 같은 슬롯의 원본 비트맵 글꼴), 글자 코드 배열 종류로 글꼴 고르기(`measureFont(context, bytes)`·`drawFont(context, bytes)`). |
| `www/assets/bitmap-font.js`·`www/render/glyph-text.js` | `BitmapFont.glyph(code)` 와 `drawGlyphText` 가 `font.glyph(code)` 로 글리프를 얻는다(두 글꼴 공통). `FONT_INK_INDEX` 내보냄. |
| `www/ui/stone-button.js`·`main-menu.js` | 돌 버튼이 `label`(영어 키)과 `text`(번역된 표시 문구)를 가지고, 한국어 라벨은 유니코드 글꼴로 재고(`labelFont`) 그린다. 메인 메뉴가 `resources.translate`·`resources.fonts.encode` 로 버튼을 만든다. 명령 통지에는 영어 라벨을 쓴다. |
| `www/ui/style-text.js`·`menu-list.js`·`option-menus.js` | `parseStyleText` 의 `options.encode`, 메뉴 목록이 `fonts.encode` 사용, `buildPopupItems(…, { translate })` 가 Options·Help 라벨(하위 목록 포함)을 번역한다. 명령·구분선·체크 상태는 그대로. |
| `www/assets/menu-assets.js` | `buildMenuResources(files, korean)`: `fonts` 에 유니코드 공급자를, `translate` 에 번역 함수(영어는 원문 그대로)를 싣는다. |
| `www/netstorm.js` | `_load()` 가 언어가 한국어일 때만 메뉴 묶음 뒤에 `korean` 묶음을 받는다(진행률은 두 묶음 합계). `_loadKorean` 은 실패하면 경고만 남기고 영어로 계속한다. 메인 메뉴 창 제목과 펼침 메뉴 라벨을 번역한다. `MAIN_MENU_WINDOW_TITLE` 내보냄. |
| `tools/prepare-assets.js`, `www/data/manifest.json` | 자산마다 `source`(저장소 루트 기준 원본 경로)를 따로 적을 수 있게 했다. 새 묶음 `korean`: `fonts/D2Coding.woff2`·`fonts/D2CodingBold.woff2`·`fonts/License.txt`(`fonts/` 원본)와 `d/xlat.korean`(`translations/xlat.korean` 원본)을 변환 없이 복사해 `www/data/` 에 둔다. 기존 항목의 manifest 내용은 그대로이고 머리의 설명 문구와 묶음 표만 바뀌었다. |
| `.gitattributes` | `webpj/translations/**`·`fonts/License.txt` 를 `-text` 로 둬서 원본과 `www/data` 복사본의 SHA-256 이 체크아웃마다 같게 했다(`www/data/**` 는 이미 `-text`). |

## 3. 동작

- 브라우저 언어(또는 `?lang=ko`)가 한국어면 메인 메뉴 버튼이 **캠페인·데모·도움말·크레딧·옵션** 으로 나오고, Options 는 소리 켜기·음악 재생·바람 소리·스피커 좌우 바꾸기·효과음/음악 음량(음량 1~5)·전체 화면 가장자리 스크롤·자동 데모·시작 시 팁 표시, Help 는 일반 도움말 - F1·버전이다. 창 제목은 `NetStorm 메인 메뉴`, 클릭하여 시작 화면도 같은 D2Coding 으로 나온다.
- 글자 크기(em)는 글꼴 슬롯 높이 × 13/14 다(슬롯 0 은 13: 한글 13폭·영문 7폭). 굵은체는 **굵게 스타일(2)일 때만** 쓴다. 슬롯 0·3 의 굵기 700 은 영어 Arial 획을 위한 값이라 한글 단색 글리프에서는 획이 뭉쳐 읽기 어려워 보통체를 쓴다. 크기·굵기는 1배·3배 확대(최근접) 화면을 12px 굵은체·12px 보통체·13px 보통체로 비교해 정했다.
- 줄 높이·기준선은 같은 슬롯의 원본 비트맵 글꼴을 따라, 영어 조각과 한 줄에 섞여도 기준선이 맞는다(예: 슬롯 3 은 높이 19·기준선 15).
- 영어(`?lang=en`)는 한국어 묶음을 받지 않고 이전과 같다(`resources.fonts.unicode` 는 null, 번역 함수는 항등).
- **(이 단계 당시)** Tell 대화상자·도움말 창의 본문과 대화상자 안의 버튼 글은 영어 원문 그대로 원본 글꼴이었다. 그 문구는 `tell.<언어>`·`.korean` 자료에서 오는데 한국어 자료가 없었기 때문이다. **같은 날 [2단계(WB03-O)](webpj-korean-tell-20261031.md)에서 Tell 대화상자·캠페인 목록·시작 창이 한국어가 되었고 도움말 창은 단추만 한국어다(도움말 본문은 아직 영어).**

## 4. 원본과 다르게 둔 점·가정

1. **번역 문구는 이 프로젝트가 새로 쓴 것이다**(원본에는 한국어가 없다). 용어는 AGENTS.md 의 명칭 규칙을 따르되 메뉴 라벨에는 건물·유닛 이름이 없다. `Credits → 크레딧`, `Tell Tips at Startup → 시작 시 팁 표시`, `Pass Server Diagnostic → 서버 진단 통과` 같은 선택은 **개발자 검수 대상**이다.
2. **글리프는 1비트 단색이다**(문턱 110). 원본 비트맵 글꼴(GDI 로 만든 안티앨리어싱 없는 글리프)과 같은 8비트 표면 규칙이지만, 같은 모양은 아니다. 안티앨리어싱 한글은 7차 목표(안티앨리어싱·FSR 선택 옵션)와 함께 검토한다.
3. **한국어 글꼴 묶음을 받지 못하면(파일 누락·글꼴 등록 실패) 경고를 남기고 영어 메뉴로 계속한다.** 한국어는 선택 기능이고 영어 자료로 대체할 수 있기 때문이다(웹 적응).
4. 원본 글꼴에 없는 글자(한글)는 **UI 문구를 부호화하는 곳에서** 유니코드 글꼴이 있을 때만 쓴다. 영어 자원에서 한글이 들어오면 `?` 로 나온다(이전과 같음). Tell·도움말 본문은 아직 `encodeWindows1252` 경로라 영어 본문만 다룬다.
5. 줄 높이는 원본 비트맵 글꼴 값을 따르므로 슬롯 높이 12·13 글꼴에서 한글(em 11·12)이 줄 안에 들어가는지는 메인 메뉴·펼침 메뉴(슬롯 0)에서만 확인했다.
6. `Pass Server Diagnostic ` 번역 키 끝의 공백은 코드의 라벨과 같게 유지해야 한다. 편집기가 줄 끝 공백을 지우면 번역이 빠진다(검사가 잡는다).

## 5. 검증(HJOW-X3D, Node v24.21.0, Edge 156 헤드리스)

- `npm --prefix webpj test` **123개 통과**(신규 18: `tests/korean.test.mjs`, 기존 105 그대로). `check-assets` 통과.
  - 번역표 읽기 규칙(CR·여러 줄·덮어쓰기·미완성 블록)과 `xlat.korean`(UTF-8·BOM 없음·중복 없음·**코드가 만들 수 있는 모든 라벨이 번역돼 있고 번역표에 쓰이지 않는 원문이 없음**).
  - woff2 가 `fonts/` 원본과 같은 바이트이고 매니페스트 `korean` 묶음 구성이 맞음.
  - `UnicodeGlyphFont`(문턱 109/110 경계·위치·밑줄·취소선·공백·캐시·측정), `UnicodeFontProvider`(슬롯·스타일별 사양, 기준 글꼴의 줄 높이·기준선), `FontSet` 의 부호화·글꼴 고르기·그림자 오프셋, 글자 그리기 패스·위치, 돌 버튼·한국어 메인 메뉴 배치·그리기(버튼 밖 픽셀 변화 없음), 펼침 메뉴 번역·크기·명령 실행, 가짜 DOM 으로 Canvas 래스터라이저(글꼴 등록·크롭·전진 폭 반올림·굵게/기울임 글꼴 문자열·정리)와 `Netstorm._load`(언어별 묶음 선택·진행률 합계·한국어 묶음 실패 시 영어로 계속)·`_openPopup`.
- **변이 검사** 52종 중 51종이 걸렸다(첫 실행에서 놓친 5종 중 4종은 검사를 보강했다: 기준 글꼴 줄 높이·기준선 참조, 한국어 묶음 크기 계산, `_load` 의 한국어 분기). 남은 1종은 `_enterMainMenu` 의 창 제목 번역 한 줄로, 실제 브라우저에서 `NetStorm 메인 메뉴` 로 바뀌는 것을 확인했다.
- 실제 브라우저(마우스 클릭·CDP) 1024×768·1920×1080·1366×768·1280×800 한국어: 클릭하여 시작 → 메인 메뉴(버튼 5개 모두 유니코드 라벨, 폭 51) → Options 열기(구분선을 뺀 항목 9개 모두 유니코드, 첫 행 `소리 켜기` 를 눌러 `sound` 1→0 으로 바뀌고 팝업이 닫힘, 다시 열면 파란 점이 빠져 있음) → Help 열기(`일반 도움말 - F1`·`버전`) → 일반 도움말 창(`HelpWindow`) → Campaign 대화상자(`TellWindow`, 영어 원본 글꼴) → 콘솔 오류·경고 없음. 영어(`?lang=en`) 1024×768 은 라벨이 영어이고 같은 흐름이 통과했다. `D2Coding.woff2` 요청을 막은 한국어 실행은 경고 한 줄을 남기고 영어 메뉴로 클릭 대기까지 갔다.
- 확인하지 않은 것: Chrome/Firefox·터치·소리 출력·기기 픽셀 비율 2 이상의 한글 모양·실제 Windows 환경에서 D2Coding 이 설치된 PC 와의 차이(FontFace 가 우선하도록 같은 이름으로 등록함)·긴 한국어 문장의 줄바꿈(본문 줄바꿈은 공백 기준이라 어절 단위로 동작할 것으로 보이나 Tell/도움말 한국어 자료가 없어 확인하지 못함).

## 6. 남은 일

1. **(완료: 2026-10-31 WB03-O, [2단계 기록](webpj-korean-tell-20261031.md))** Tell 대화상자 본문·단추·캠페인 목록·시작 창을 한국어로 했다. 남은 것은 도움말(F1) 본문 `help.korean` 과 미션 `.korean` 파일이다.
2. 번역 문구 검수(개발자), 한국어 모드의 안티앨리어싱 선택(7차 목표와 함께).
3. Options −40 규칙·와이드 화면 위치(원본 근거 필요), Auto-Demo, 도움말 후속(타입 표가 생기면 `<info>`·그림), Tell 조회의 '현재 미션', 본문 끌기 중 Overlay 확인, 커서 번호 2~14(미션 구현에서) — 맨 위 인계의 '다음'을 따른다.
