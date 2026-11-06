# webpj 무브라우저 실행과 cpppj 오라클 비교 기반 — WB02-H

2026-11-07, HJOW-Athlon(192.168.0.94, AGENTS.md 실행 확인 예외 PC). [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서2다. 세 가지를 만들었다. ① webpj를 브라우저 없이 Node에서 실행해 논리 화면과 관측 보고서를 얻는 도구 ② cpppj 기준선 실행 파일로 같은 단계표의 오라클을 수집하는 도구 ③ 둘을 비교하는 도구. 그다음 첫 대상인 M1 메인 메뉴 화면(영어 4:3 1024×768)을 cpppj와 대조했다.

결과: 8개 장면 모두 **기록한 원인 밖의 픽셀 차이가 0**이다. 메인 메뉴(8버튼)·Welcome·Campaign·Demo·Credits Tell 창과 Help 펼침 메뉴는 구름 배경을 빼면 픽셀까지 같다. 대조 중에 웹 결함 2건을 찾아 고쳤다. 원인을 기록한 차이 중 구름 배경은 웹 쪽 차이(후속 이식 몫)이고, 나머지 넷은 cpppj 기준선이 원본과 다른 차이다.

## 1. 파일

| 파일 | 역할 |
| --- | --- |
| `webpj/tools/headless.mjs` (신규) | 무브라우저 실행기. 실제 `Netstorm`의 `_load`·`_activate`·`_onFrame`·`_onPointerDown/Up`·`_onKeyDown`을 그대로 부르고 DOM 경계(루트 문서·Canvas·표시기·fetch)만 대체한다. 단계표 실행(`runScenario`), BMP·보고서 출력 CLI |
| `webpj/tools/cpp-oracle.mjs` (신규) | cpppj 오라클 수집기. 시나리오마다 새 프로세스와 빈 사용자 루트(`NETSTORM_USER_DATA`)를 쓴다. 캡처 BMP를 무손실 팔레트 PNG로, 보고서를 텍스트로 저장한다. 메타 `oracle.json`(커밋·EXE SHA-256·명령줄·자료 해시·수집 입력 해시)을 쓴다 |
| `webpj/tools/screen-compare.mjs` (신규) | RGB 픽셀 비교(허용 오차 없음), 구름 배경 정밀 분류, 원인 기록 판정(`judge`), 차이 그림 출력 CLI |
| `webpj/tools/image-files.mjs` (신규) | cpppj 캡처 형식(122바이트 머리 32비트 BGRA) BMP 읽기/쓰기, 결정적 팔레트 PNG 쓰기, PNG(색 형식 2·3·6) 읽기, 색 번호→RGB |
| `webpj/tests/cpp-oracle.test.mjs` (신규) | 메타·해시 검사, M1 8장면 비교 판정, 그림/분류/보고서 도구 검사(3검사). cpppj를 실행하지 않는다 |
| `webpj/tests/fixtures/cpppj/m1-scenarios.json` (신규) | 시나리오 8개의 단계표와 원인 기록(`expected`) |
| `webpj/tests/fixtures/cpppj/m1/` (신규) | 오라클: 장면별 PNG 8장(합계 약 1.9MB)·보고서 8개·`oracle.json` |
| `www/netstorm.js` (수정) | `originalComparison` 옵션(원본 비교 모드), 매 프레임 포인터 위치 폴링(결함 수정), 펼침 메뉴에 대상 이름 전달 |
| `www/ui/{main-menu,menu-list,popup-menu,tell-window,help-window,stone-button}.js` (수정) | 라벨로 조작부 좌표를 찾는 `controlPoint(label)`(cpppj `UberGump::ControlPoint`와 같은 규칙). `createTellWindow`의 `excludedLists` 인자 |
| `www/ui/option-menus.js` (수정) | `buildHelpItems`의 `technicalHelpAvailable` 인자(결함 수정) |
| `webpj/tests/fixtures/manifest.json` (수정) | 종류 `cpppj-oracle` 추가, 항목 2개(`oracle.json`·`m1-scenarios.json`) 등록. 64항목 |
| `webpj/PORT_MAP.tsv`·`tests/port-map.test.mjs` (수정) | 기준선 뒤 `9cb0547f` 판정 3행 기록, M1 대조로 확인한 사실 6행 메모. SidPool base가 판정 커밋으로 올라갈 수 있게 검사를 고쳤다 |

## 2. 무브라우저 실행기

- **방식:** 새 앱 코드를 만들지 않고 실제 `Netstorm` 객체를 쓴다. `init()`의 DOM 준비 대신 다음 대체물을 넣는다. 가짜 루트 문서(제목·`baseURI`·Canvas 생성·프레임 예약 무동작), 가짜 Canvas(1:1 좌표 사각형, 커서 그림 생성용 2D 메서드), 그리기 없는 표시기, 디스크의 `www/data`를 읽는 fetch다. 설정/진행 저장소는 `init()`과 같은 순서로 `Settings`/`Progress`(저장소 없음)를 만든다. 그다음 `_load()`→`_activate()`로 메인 메뉴에 들어간다. 그래서 메뉴·대화상자·도움말의 명령 분배와 시작 흐름(Welcome)은 브라우저와 같은 코드다.
- **단계표:** cpppj `--ui-script`와 같은 `[프레임, 동작, 인자]`다. 지원 동작은 `click`(라벨 또는 `screen:x,y`, 같은 프레임에 누름+뗌)·`down`·`up`·`esc`·`key`·`snapshot`·`report`이고, 그 밖은 오류다. 프레임 순서도 cpppj와 같다(그 프레임 입력 → `_onFrame` → 관측). 프레임 시간은 cpppj 기본 maxFPS 75(13.33ms)다.
- **라벨 조회:** `MainMenuScreen.controlPoint` 순서는 펼침 메뉴 → 대화상자 → 메인 버튼이다. 목록 행은 cpppj `RawMenuControls::Point`처럼 스타일 명령을 뺀 일반 글자 조각의 연결로 비교한다(`General Help - F1`). 가운데는 정수 사각형의 (왼+오)/2, (위+아래)/2를 절삭한다. 게임 동작에는 쓰지 않는다.
- **보고서:** cpppj와 같은 칸 `size`·`page`(Tell 창이면 현재 Tell 이름, 아니면 `main`)를 쓰고, 웹 전용으로 `web_popup`·`web_dialog`·`web_title`·`web_cursor`를 더한다.
- **원본 비교 모드:** `new Netstorm({ originalComparison: true })`는 웹이 빼는 원본 화면 요소를 원본처럼 보인다. 대상은 메인 메뉴 Multiplayer·Edit·Quit 버튼, 펼침 메뉴의 Direct Draw·Resolution·Technical Help·Pause·Pass Server Diagnostic, Campaign의 User Made Campaigns 행이다. 그 기능 자체는 여전히 구현하지 않는다. 기본값은 false이며 무브라우저 비교만 쓴다.
- **한계:** 영어만 지원한다(한국어는 Canvas 2D 글리프 래스터라이저 필요). 장면은 메인 메뉴뿐이다. 8장면 실행은 약 2초다.

## 3. cpppj 오라클 수집

- **기준선 실행 파일:** `cpppj/build`는 다른 세션이 계속 다시 빌드하므로 쓰지 않았다. 기준선 `1f00a25a`를 별도 worktree에 꺼내 `build-cpppj.ps1 -Mode Game`으로 빌드했다(약 20분, 종료 0). 결과 `Netstorm.exe`(11,521,536바이트, SHA-256 `03fa4df82768c1f345f770e447a15941a43c2b622f11b83fa0e28b8f070cbc1e`)와 `NetstormSmacker.dll`은 `extracted/webpj-oracle/1f00a25a/`(Git 제외, 이 PC 전용)에 두었고 worktree는 지웠다.
- **명령:** `--run assets/game-data --window --mute-audio --frames N --set "SCREENW=1024;SCREENH=768;…" --ui-script <임시> --ui-report <임시>`. 환경은 `NETSTORM_USER_DATA=<빈 임시 폴더>`이고 `NETSTORM_CPP_NO_AUDIO`는 지운다. 시나리오당 약 18초 걸린다.
- **최초 실패와 교정:** 첫 수집은 `--no-audio`(+`NETSTORM_CPP_NO_AUDIO=1`)였다. 이 상태는 소리 장치가 없어 Options의 소리 항목 6개가 비활성으로 그려져, 소리 장치가 있는 원본과 화면이 달랐다. 그래서 장치는 열고 소리만 끄는 `--mute-audio`로 바꿔 전체를 다시 모았다.
- **저장:** 캡처는 팔레트를 적용한 32비트 BMP(1024×768이면 약 3MB)다. 색이 256개 이하이므로 무손실 팔레트 PNG(팔레트는 색 값 오름차순, zlib 9, 같은 그림이면 같은 바이트)로 바꿔 저장하고, 원래 BMP의 SHA-256을 메타에 남긴다.
- **재현성:** 같은 실행 파일로 전체를 두 번 모았고 PNG 8장의 해시가 모두 같았다. 메인 메뉴를 프레임 5·20·21·60·200에서 캡처해도 같았다(정지 화면).
- **같은 자료:** `assets/game-data` 1351파일 중 웹이 복사한 30파일은 `webpj/www/data`와 SHA-256이 모두 같다. 웹이 복사하지 않은 1321파일은 개수만 남겼다.
- **수집 입력 해시:** 메타의 `collectionInputsSha256`은 언어·화면·공통 설정·시나리오별 id/설정/프레임/단계표만의 해시다. 원인 기록을 고쳐도 다시 모을 필요가 없고, 단계표가 바뀌면 검사가 재수집을 요구한다.

## 4. 비교 규칙

- 같은 크기의 RGB를 픽셀마다 비교한다(허용 오차 없음). 판정 조건은 세 가지다. 기록 영역 밖 차이는 0, 영역 안 차이는 기록한 픽셀 수와 정확히 같음, 구름 차이는 기록한 픽셀 수와 같음. 차이가 고쳐지거나 늘면 모두 실패하므로 기록을 갱신해야 한다.
- **구름 배경 정밀 분류:** 구름 차이는 화면 전체에 퍼져 있어 사각형으로 가리면 다른 결함을 놓친다. 그래서 다음 세 조건을 모두 만족하는 픽셀만 '구름 차이'로 센다.
  - 웹 메인 메뉴에서 구름이 그대로 보이는 위치다(웹 원본 비교 모드의 제목·8버튼은 cpppj와 픽셀까지 같으므로 cpppj에서도 구름 위치다).
  - 웹 픽셀이 웹 구름과 같다.
  - cpppj 픽셀이 cpppj 메인 메뉴 오라클과 같다.

  웹이 그림을 빠뜨리거나 cpppj에만 그림이 있으면 셋째 조건이 깨져 영역 밖 차이로 잡힌다(검사로 확인).
- 보고서는 `size`·`page`만 비교한다. cpppj의 `popup` 칸은 옛 호스트 보조 메뉴 이름이라 10.78 메인 Help/Options 펼침 메뉴가 열려도 비어 있으므로 비교하지 않는다.

## 5. M1 대조 결과

| 장면 | 전체 차이 | 구름 | 기록 영역 | 보고서 |
| --- | --- | --- | --- | --- |
| welcome(첫 실행 환영 창) | 37054 | 37054 | — | 같음 |
| main-menu(8버튼) | 37054 | 37054 | — | 같음 |
| campaign(User Made Campaigns 행 포함) | 37054 | 37054 | — | 같음 |
| demo | 37054 | 37054 | — | `page` cpp `DEMOVILLE` / 웹 `Demoville`(아래 ④) |
| credits(세 행, 11.01 Patch Credits) | 37054 | 37054 | — | 같음 |
| help-popup | 37054 | 37054 | — | 같음 |
| options-popup | 54313 | 37054 | 17259(아래 ②) | 같음 |
| general-help(F1Help 창) | 33194 | 33164 | 15+15(아래 ③) | 같음 |

### 5.1 원인을 기록한 차이

1. **구름 배경(웹 쪽, 후속 이식 몫):** cpppj는 메인 메뉴 배경을 `ClientKencloudGlobals::Draw`(원본 Kencloud 구름 생성·행 캐시 복사)로 그리고, 웹은 GIF를 `surface.tile`로 단순하게 깐다. 차이 행은 약 31행 주기로 2~3행씩 있고, 그 행 구간에서 cpppj 구름이 웹보다 가로 1픽셀 밀려 있다. 원본 스크린샷(`screenShots/mainMenu.png`, 클라이언트 원점 (2,33))과 대조하면 두 결과가 다른 37,054픽셀에서 원본은 cpppj 쪽에 가깝다. 평균 채널 오차 합은 cpppj 5.78·웹 12.47이고, 더 가까운 픽셀은 cpppj 27,889·웹 8,306이다. WB03-M1의 스크린샷 대조는 손실 압축 잡음 때문에 평균 차이만 봤으므로 이 1픽셀 어긋남을 가리지 못했다. **Kencloud 이식(3.2절 순서5 WB03-M2)에서 해소한다.**
2. **Options 펼침 메뉴 항목(cpppj 쪽):** cpppj 기준선은 원본에 없는 `Language >`(6차)·`Anti-Aliasing`·`FSR 1.0 Upscaling`(7차)을 더한다. 또 원본 스크린샷(`screenShots/mainMenu - Options.png`)에 있는 `Pause - Shift-F9` 행이 없다. 웹 원본 비교 모드는 원본 스크린샷 구성과 같다. 행 수가 달라 정지 포인터 아래 강조 행도 다르다(cpppj `Language >`, 웹 `Sound On`). 영역 `[551,294,718,600)`의 17,259픽셀이다.
3. **스크롤바 화살표 색(cpppj 쪽):** General Help 창의 위/아래 화살표 삼각형이 cpppj에서는 붉은 갈색(99,38,22)이다. 원본 스크린샷(`screenShots/help - NetStorm Instructions.png`)을 확대해 보면 검정이고 웹도 검정이다. 인계 4절 "도움말 고급 표시"의 '원본 삼각형 색 해석' 미해결 항목에 근거를 더한다. 영역 `[715,61,721,67)`·`[715,341,721,347)`의 각 15픽셀이다.
4. **Demo의 현재 Tell 이름(cpppj 쪽):** 원본 10.78 실행 파일에서 대소문자를 무시하고 찾으면 `Demoville` 하나뿐이다(`DEMOVILLE` 없음, 읽기 전용 검색). 웹은 원본과 같은 `Demoville`이다. cpppj 기준선은 `DEMOVILLE`을 넘겨 기록 이름(`global.priorTell` 등)이 다르다. 섹션 조회는 대소문자를 가리지 않아 화면은 같다.

②~④는 cpppj 수정 대상 후보이며 웹 단위에서는 고치지 않았다(cpppj 파일·공통 인계는 다른 세션 소관). 사용자에게 보고한다.

### 5.2 대조로 찾아 고친 웹 결함

1. **정지한 포인터 아래 행 강조:** 원본 UserInput은 매 프레임 마우스 위치를 폴링해 Gump에 알린다. 그래서 Help 버튼을 누른 뒤 포인터가 그대로 있으면 그 아래에 새로 열린 펼침 메뉴 첫 행이 바로 강조된다(cpppj 관측). 브라우저는 포인터가 움직일 때만 사건을 주므로 웹은 강조하지 않았다. `_onFrame`이 메인 메뉴 상태에서 마지막 포인터 위치를 매 프레임 `mainMenu.pointerMove`로 다시 알리게 고쳤다. 실제 Chromium에서도 마우스를 움직이지 않은 채 첫 행이 선택됨을 확인했다(6절).
2. **원본 비교 모드의 Technical Help 활성 여부:** 원본은 기술 도움말 프로그램이 설치돼 있을 때만 이 행을 활성으로 만든다(cpppj `ClientTechnicalHelpAvailable`). 웹에는 그 프로그램이 없으므로 원본 비교 모드에서는 비활성이 맞다. 비활성 행은 그림자 없는 스타일이라 행 높이가 1픽셀 작고, 아래 Version 행과 테두리도 1픽셀 올라간다. `buildHelpItems`에 `technicalHelpAvailable`(기본 true: 원본 스크린샷 비교용)을 더하고, 원본 비교 모드는 false를 넘긴다. 일반 웹 모드는 이 행을 만들지 않으므로 영향이 없다.

두 수정 전 help-popup의 영역 밖 차이는 1,955픽셀(강조 행 + Technical Help·Version·테두리)이었고 수정 뒤 0이다.

## 6. 검사

- `node --test tests/cpp-oracle.test.mjs`: 3/3 통과.
- 최초 실패와 교정: ① 도구 검사의 `compareImages` 기대 객체에 새 칸 `background`가 없어 실패해 기대값을 갱신하고 분류 검사를 더했다. ② port-map 검사가 SidPool base를 기준선으로 고정해 둬서 `9cb0547f` 판정 뒤 실패했다. 기준선 이후 판정 커밋도 허용하도록 고쳤다.
- 기존 UI 검사 10파일 111/111을 첫 웹 수정(조회 메서드·원본 비교 옵션) 직후 통과했다. 결함 수정 뒤에는 아래 전체 검사로 확인했다.
- 전체 `npm --prefix webpj test`(Node v24.21.0): **437/437 통과**, 실패·건너뜀 0(WB02-G 434 + 신규 3), 약 46초.
- `npm --prefix webpj run test:e2e`: Chromium 초기 페이지 스모크 1/1.
- 임시 Playwright 스크립트(Chromium, 저장소 밖)로 실제 클릭을 확인했다. 시작 → Welcome 닫기 → Help 클릭(마우스 정지) → 펼침 메뉴 `selection 0` → General Help 창 열기·OK → Credits 세 행이 정상이며 페이지 오류는 0이다.
- `node webpj/tools/port-map.mjs`: 오류 0. `node webpj/tools/fixture-manifest.mjs`: 64항목·불일치 0·cpppj 변경 보고 0.

## 7. 기준선 뒤 cpppj 변경 (이번 단위 착수 중 보고)

다른 세션 커밋 `dc6d1933`(일반 전투섬)·`9cb0547f`(중앙섬/군도) 가운데 웹 이식분이 있는 3행만 판정이 필요했다. 나머지 13단위는 모두 `todo`라 이식 때 판정하고, 새 파일 8개(`RawBattleIsland`·`RawCentralIsland`·`RawIslandPartition`·`RawPeerWorldSync`)는 재기준선 때 분류한다.

| 단위 | 판정 | 근거 |
| --- | --- | --- |
| o/RawGameAuxState | ① | 중앙섬 X/Y(00568afc/00568b00) 칸 추가. 멀티플레이 중앙섬·메타 몫(웹 제외) |
| o/SidPool | ① | 예측 커서 쓰기 접근자 `PredictableCursorValue`만 추가했고 할당 결과는 같다. 로그인/저장 부모 이식(WB05) 때 필요하면 같은 접근을 둔다 |
| client/UberGump | ③ | 로그인 중 "Creating Islands." 안내(`localLogin.creatingIslands`)는 일반 전투섬 생성 경로 소비자다. WB05 로그인 이식 때 단독 도달 여부를 확인한다 |

세 행의 base를 `9cb0547f`로 올렸다.

## 8. 사용법과 다음 단계

```powershell
# 웹을 무브라우저로 실행해 BMP·보고서를 쓴다
node webpj/tools/headless.mjs --scenarios webpj/tests/fixtures/cpppj/m1-scenarios.json --out <폴더>
# 저장된 오라클과 비교한다(차이 그림은 --diff)
node webpj/tools/screen-compare.mjs --diff <폴더>
# 오라클을 다시 모은다(창이 뜬다. AGENTS.md 실행 확인 규칙, 예외 PC만 확인 없이)
node webpj/tools/cpp-oracle.mjs --exe extracted/webpj-oracle/1f00a25a/Netstorm.exe [--id 시나리오]
```

- 새 장면은 시나리오 파일에 단계표를 더하고 오라클을 모은 뒤, 차이가 있으면 고치거나 원인을 `expected`에 적는다.
- 다른 PC에는 `extracted/webpj-oracle/1f00a25a/`가 없다. 그 PC에서 다시 모으려면 기준선을 빌드해야 한다(3절 방식).
- 다음은 3.2절 순서3 **WB04-D5b**다. 구름 Kencloud 이식은 순서5(WB03-M2)에서 하고, 그때 이 비교의 구름 차이 기록을 0으로 바꾼다.
