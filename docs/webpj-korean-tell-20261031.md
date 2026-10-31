# webpj 한국어 지원 2단계: Tell 대화상자·캠페인 목록·시작 창·도움말 창 단추 (WB03-O, 2026-10-31)

2026-10-31, **HJOW-X3D**(마지막 전체 정밀 디컴파일 호스트 HJOW-Athlon/2026-10-23 과 다른 PC 이지만 이번 단위는 디컴파일 소스를 읽지 않았다. 근거는 원본 아카이브 `netstorm.tarc` 의 영어 자료, cpppj `QuickHelpWindow.cpp`(안내 문구 번역 호출 확인), [WB03-N 한국어 1단계](webpj-korean-20261031.md), dotnetpj 의 한국어 팁·캠페인 이름, [한국어 명칭 규칙](korean-unit-names.md)이다). 사용자 요청("AGENTS.md, LEFT_JOBS.webpj.md 를 읽고 webpj 다음 작업 진행")으로 [인계](../LEFT_JOBS.webpj.md)의 '다음' 첫 항목 **Tell·도움말 한국어 본문**을 했다. `cpppj/`·`dotnetpj/`·`originals/`·`originalCD/`·AGENTS.md·공통 `LEFT_JOBS.md` 는 수정하지 않았다(다른 세션이 같은 저장소에서 cpppj 를 `CP` 커밋으로 진행 중이라 읽기만 했고 내 파일만 커밋했다). 원본 게임·analyzeManager 는 실행하지 않았고, 브라우저는 우리 웹 클론만 띄운 Edge 헤드리스다.

## 1. 범위와 결과

| 대상 | 상태 |
|---|---|
| 웹 메뉴에서 닿는 **Tell 섹션 전부**(환영 `Welcome`·`Welcome2`, 시작 팁 `Tips`+`TipList` 40개, 캠페인 `UCampaign`, 크레딧 `Credits`·`CreditsNew`~`3`·`Creditsold`·`Credits1`~`13`, 데모 선택 `Demoville`·`Demo`, 버전 `About`, 기술 지원 `TechSupport`) | **한국어** (`tell.korean`, 28개 섹션 + 팁 40개) |
| 캠페인 묶음 파일 `offical1`~`6`·`offical2sep`·`offical5sep` (제목·소개·미션 이름) | **한국어** (`offical*.korean` 8개) |
| 용어표 (`{factory}` 등 이름 용어·기본 확인 단추 `OkButton`) | **한국어** (`config.korean`) |
| 대화상자의 돌 단추·본문·목록 글자 | 한국어 모드에서 **유니코드 글꼴(D2Coding)** 코드로 만들고 잰다 |
| F1 일반 도움말 창의 `Back`·`OK` 단추 | **한국어** (`xlat.korean` 번역표: `뒤로`·`확인`) |
| F1 일반 도움말 **본문**(128개 주제, `help.english` 120KB) | **영어 그대로**(한국어 글꼴 D2Coding 으로 그려짐). 한국어 도움말 파일 없음 |
| 미션 스크립트(`tutorial1.english` 등)·`<background>` 그림 | 해당 없음(미션 구현 전)·그림 자료 없음 |

한국어가 아닌 언어(영어)는 이전과 같다: 본문 조각·단추는 Windows-1252 바이트이고 원본 비트맵 글꼴로 그려진다.

## 2. 구현

| 파일 | 내용 |
|---|---|
| `www/ui/tell-window.js` | 본문 `parseStyleText(…, { encode })`·돌 단추 `new StoneButton({ …, encode })`·`fitToText(font, fonts)` 에 `FontSet.encode` 를 연결. 한국어 모드면 코드 배열이 `Uint16Array` 라 글자 측정·그리기가 유니코드 글꼴을 쓴다(장치는 WB03-N 에서 갖춤). |
| `www/ui/help-window.js` | `Back`·`OK` 단추를 `resources.translate`(xlat) 로 번역하고 `encode`·`fitToText(font, fonts)` 를 연결, 본문 `parseStyleText` 에 `encode` 연결. 명령에는 영어 키(`back`·`ok`)를 그대로 쓴다. |
| `www/assets/game-data.js` | **섹션 단위 영어 대체**: `tellSection` 이 현재 언어의 tell 파일에서 섹션을 못 찾으면 영어 tell 파일(`fallbackTellText`)에서 찾는다. `tell.korean` 은 웹에서 닿는 섹션만 번역한 부분 파일이라 번역이 없는 섹션이 열리지 않는 일이 없게 하는 웹 적응이다. 언어 파일이 통째로 없을 때의 영어 대체(`_readLanguageFile`)는 그대로. |
| `www/assets/korean-assets.js` | `buildKoreanSupport` 의 결과에 `languageFiles`(`d/*.korean` 경로 → 바이트) 추가, `KOREAN_LANGUAGE_PATHS` 내보냄. |
| `www/netstorm.js` | `_load()` 가 `languageFiles` 를 느슨한 파일로 합쳐 `GameData.build` 에 넘긴다. 한국어 묶음을 받지 못하면(`korean` 이 null) 합치지 않아 전부 영어로 대체된다. |
| `tools/prepare-assets.js`, `www/data/manifest.json` | 한국어 묶음에 `d/tell.korean`·`d/config.korean`·`d/offical*.korean` 10개 추가(원본 `webpj/translations/`). 묶음 파일 수 4 → 14. |
| `translations/tell.korean`·`config.korean`·`offical1`~`6`·`offical2sep`·`offical5sep.korean`(신규, UTF-8, LF, BOM 없음) | 번역 자료. 파일 머리 주석에 용도·규칙을 적었다. |
| `translations/xlat.korean` | `Back → 뒤로`·`OK → 확인` 추가(31 → 33개 블록). |

## 3. 번역 방식과 결정

- **원본의 언어 체계를 그대로 따랐다.** 원본은 UI 문구를 `d/xlat.<언어>`(번역표)로, 대화상자 본문을 `d/tell.<언어>`·`d/offical*.<언어>`·`d/<미션>.<언어>` 로, 이름 용어를 `d/config.<언어>` 로 나눈다(원본 아카이브에 `xlat.french`·`config.french`·`help.german` 이 남아 있어 이 체계가 실제로 쓰였음을 확인). 한국어도 같은 이름·같은 구조의 파일을 만들었다.
- **구조는 영어와 똑같이, 보이는 문구만 번역했다.** 명령·조건(`<?{키}>`)·치환 키(`{CText}`·`{param.1}`)·링크 주소·`$Button`/`$Menu`/`$Checked`/`$Timeout` 의 명령 인자·섹션 이름은 영어 원문 그대로다. 크레딧의 이름 줄(베타 테스터 수백 명 포함)은 영어 원문에서 **복사**하고 제목·역할 문구와 단추만 바꿨다.
- **용어:** AGENTS.md 의 한국어 명칭 규칙을 따랐다(Priest→사제, Temple→신전, Bridge→다리, Geyser→간헐천, Empty→고갈된, Workshop→워크샾, Generator→제네레이터, 아웃포스트·훨리기그·더스트 데빌·맨 오 워 등은 공유 명칭 표). 개념어는 `Storm Power → 스톰 파워`(스톰 간헐천과 맞춤), `Knowledge → 지식`, `Energy → 에너지`, `Furies → 퓨리`. 시작 팁 40개는 dotnetpj 의 한국어 번역을 바탕으로 하되 위 용어에 맞추고 원문의 색 표시(`{CText}…{Text}`)·링크 마크업을 되살렸다. 캠페인 이름(초기 미션·사제 훈련·자유를 위한 투쟁·국가의 탄생·완전한 승리)과 `전쟁의 시작!`·`훨리기그의 지배자`도 dotnetpj 와 같다. 나머지 미션 이름은 새로 지었다.
- **줄바꿈:** 대화상자 본문에서 원본의 줄바꿈(LF)은 그대로 줄바꿈으로 보이므로, 영어 한 줄 길이를 따라 한글 폭(13픽셀)을 고려해 한 줄을 30자 안팎으로 끊었다(긴 단락 한 줄인 팁·사제 훈련 소개는 대화상자가 폭을 맞춘다).
- **한국어 모드의 글꼴:** 한국어 모드에서는 대화상자 안의 영어 조각(이름·`F5` 같은 키 이름·영어 도움말 본문)도 D2Coding 으로 그린다(dotnetpj 결정: 한국어이면 D2Coding, 영어 글꼴에 없는 글자는 D2Coding). 영어 글꼴과 한 줄에 섞어 그리는 일은 하지 않는다.
- **`config.korean`:** 영어 용어표와 같은 키·같은 순서이며 보이는 값만 번역했다. 한국어에는 관사·복수형이 없어 `a factory`·`factory`·`factories` 가 모두 `워크샾`이다. `crash HTML`·`crash TXT`·바로 가기 주소 같은 설치·진단 문구는 번역하지 않았다. 이 용어표가 영어 용어표를 **대체**하므로(원본도 한 언어 파일만 읽음) 키를 빼먹으면 `{키}` 가 비게 된다 — 검사가 영어와 같은 키 목록임을 확인한다.

## 4. 원본과 다르게 둔 점·가정

1. **번역 문구는 이 프로젝트가 새로 쓴 것이다**(원본에는 한국어가 없다). 개발자 검수 대상이며 특히 다음은 선택이 갈리는 문구다: `Furies → 퓨리`, 크레딧의 재치 있는 직함(`Chief Nerd → 수석 괴짜`·`Father and Author → 아버지이자 작가` 등), 미션 이름(`Bridge the Gap → 간극에 다리 놓기`·`Fragile Fortune → 위태로운 행운`·`Thundering Power! → 썬더링 파워!`·`The Storm Rages → 폭풍은 몰아친다` 등), `Credits → 크레딧`(dotnetpj 는 `제작진`), `No More Tips → 그만 보기`(dotnetpj 와 같음).
2. **섹션 단위 영어 대체는 웹 적응이다.** 원본은 언어 파일에서 섹션을 못 찾으면 찾지 못했다고 알린다. 웹은 `tell.korean` 이 부분 파일이므로 영어 `tell.english` 로 대신한다. 이 대체로 열리는 섹션은 현재 웹 메뉴에서 닿지 않는다(검사가 닿는 섹션이 모두 한국어에 있음을 확인).
3. **도움말(F1) 본문은 이 단계에서는 영어 그대로였다**(같은 날 [3단계](webpj-korean-help-20261031.md)에서 일부 번역). `help.english` 는 120KB 이고 주제 하나가 파일 안의 앵커 구간이라, 번역을 부분 파일로 두려면 주제 단위 대체가 필요하다(지금 `readText` 는 파일 단위 대체). 이번에는 번역하지 않고 다음 단위로 남긴다. 용어표 키(`{factory}`·`{vortex}`·`{priest}` 등)는 영어 tell·도움말·미션 파일 어디에서도 쓰이지 않아(확인함) 한국어 용어표가 영어 본문에 한국어 이름을 섞어 넣는 일은 지금 없다.
4. **`NO_HELP_TEXT`("No help information available")는 번역하지 않았다.** cpppj `QuickHelpWindow::Show` 는 치환 결과가 비면 이 문구를 번역해 쓰지만, 웹 변환 결과는 본문 끝에 줄바꿈이 남아 비는 일이 없고, 줄바꿈이 없는 한 줄은 본문 배치(원본 `StyleTextBuffer::Layout` 과 같은 규칙: 개행으로 끝난 줄만 배치)가 그리지도 않는다. 닿지도 보이지도 않는 경로라 장치를 만들지 않았다(중간에 만들었다가 이 확인 뒤 지웠다).
5. 도움말 창의 `Back`·`OK` 는 번역표 키로 했다(cpppj 가 도움말 창 문구를 `client.Translate` 로 바꾸는 것과 같은 방식). Tell 대화상자의 단추 라벨은 번역표를 거치지 않고 Tell 파일의 문구를 그대로 쓴다(cpppj 의 Tell 단추 생성에서 번역 호출을 찾지 못했다).
6. 한국어 도움말 본문(영어 문장)이 D2Coding 으로 그려지면 한 줄이 Arial 보다 길어 스크롤바 아래로 최대 몇 픽셀 겹칠 수 있다(예: 첫 줄 "Click and drag" 끝). 도움말 창의 스크롤바가 본문 오른쪽 끝에 겹쳐 놓이는 기존 배치(원본 규칙)를 바꾸지 않았다.

## 5. 검증(HJOW-X3D, Node v24.21.0, Edge 156 헤드리스)

- `npm --prefix webpj test` **133개 통과**(신규 10: `tests/korean-tell.test.mjs`, 기존 123 그대로, 기존 한국어 검사 셋을 새 번역 키·묶음 구성에 맞게 고치고 `_load` 에 한국어 자료 합치기 단언 추가). `check-assets` 통과.
  - **구조 대조:** `tell.korean` 의 모든 섹션이 영어 원문과 지시문(`$Button`·`$Menu`·`$Checked`·`$Timeout` 의 순서·명령·인자, 목록 패턴 라벨)과 본문 안의 태그·`{치환 키}`·`~` 글자 모양 표시가 같다(팁은 번호별). 영어에 없는 섹션·중복 섹션·한글이 없는 섹션이 없다. `offical*.korean` 8개·`config.korean` 이 영어와 같은 이름·키·지시문 구조다.
  - **닿는 섹션:** 시작점(캠페인·크레딧·데모·환영·팁·버전·기술 지원)에서 `Tell` 참조를 따라 닿는 모든 섹션이 한국어에 있고(영어에서 닿는 집합과 같음) 팁이 0~39 까지 40개다.
  - **창 동작:** 닿는 섹션 전부를 한국어·영어 두 창으로 800×600·1024×768·1366×768·1280×800 에서 열어 단추 명령이 영어와 같고 본문 조각·단추 라벨이 유니코드 코드 배열이며 화면 안에 들어가고 그려짐을 확인했다. 캠페인 목록 행·미션 행 명령이 영어와 같다. 영어 창은 이전처럼 8비트 코드. 한국어 묶음이 없으면 모두 영어로 대체된다.
- **변이 검사** 한국어 자료 16종(버튼·조건·타임아웃·링크·목록 패턴·미션 명령·선행 조건·제목·용어 키·`OkButton`·가짜/중복 섹션·섹션 삭제·팁 삭제·색 키 삭제)과 소스 12종(Tell 본문·단추 `encode`·`fitToText` 의 `fonts`, 도움말 단추 번역·`encode`·`fonts`·본문 `encode`, 섹션 단위 대체·영어 대체본 경로, `languageFiles`, 앱의 자료 합치기)이 **모두 걸렸다**. 중간에 도움말 안내 문구 번역 연결이 걸리지 않아 그 경로가 닿지 않는 것을 알아내고 연결을 지웠다(4번).
- **실제 브라우저(마우스 클릭·키보드, CDP):** 1024×768·1920×1080·1366×768·1280×800 × 한국어·영어 여덟 조합 **331개 통과**, 콘솔 오류·경고 없음: 환영 → Welcome2 → 메인 메뉴·`welcomeGiven` 저장 / 팁 창 → 다음 팁 → 그만 보기(`tellTips=0`) / 캠페인 → 5개 묶음 개요 창과 Back / 데모 선택 → 취소 / 크레딧 → 패치 크레딧 4쪽·뒤로 4번 → 오리지널 크레딧 14쪽·마지막 쪽 확인 / `About` / F1 일반 도움말 → 확인. 모든 대화상자가 화면 안에 있고 단추 글자가 한국어·유니코드다. 스크린샷으로 D2Coding 글자(제목·본문·단추·목록·강조색)의 모양과 줄바꿈을 눈으로 확인했다.
- 확인하지 않은 것: Chrome/Firefox·터치·소리 출력·기기 픽셀 비율 2 이상의 한글 모양·한국어 문구의 어색함(검수 몫)·본문 끌기·스크롤바가 필요한 긴 한국어 대화상자(닿는 섹션은 모두 스크롤바 없이 들어간다).

## 6. 남은 일

1. **한국어 일반 도움말 본문**(→ 2026-10-31 WB03-P 에서 주제 단위 영어 대체와 15개 주제 번역을 했다, [3단계 기록](webpj-korean-help-20261031.md)). 나머지 113개 주제는 영어 본문이 D2Coding 으로 나온다.
2. **미션 스크립트 한국어**(`tutorial1.korean` 등 `.korean` 미션 파일, 브리핑·안내·`{@미션.키}`)는 미션 구현(WB05~) 때. Tell 조회의 '현재 미션' 단계와 함께 한다.
3. 번역 문구 개발자 검수(4절 1번), 한국어 모드의 안티앨리어싱 선택(7차 목표와 함께), `<background>` 그림(`nsisles` 자료 없음).
4. 맨 위 인계의 나머지: Auto-Demo, Options −40 규칙·와이드 화면 위치, 도움말 후속(타입 표 생기면 `<info>`·그림), 본문 끌기 중 Overlay 확인, 커서 번호 2~14(미션 구현에서). 그 뒤 정적 맵 화면(M2).
