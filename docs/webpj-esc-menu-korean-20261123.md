# webpj ESC 게임 메뉴 한국어 점검 (WB04-E1290, 2026-11-23)

사용자 제보(2026-11-23, [LEFT_JOBS.md 공통](../LEFT_JOBS.md#user-requests-20261123))를 cpppj 1329(`840d92a87`, [근거](exe/cpp-esc-menu-korean-1329-20261123.md))와 맞춰 웹에서 다시 점검했다. 제보 내용은 "번역되지 않은 행이 남아 있고 '플레이어' 메뉴를 고르면 글자가 깨진다"이다. cpppj는 두 번째를 재현하지 못했지만 웹은 재현됐다.

## 점검 방법

- 새 e2e `e2e/korean-esc-menu.spec.js`가 한국어 The War Begins!에서 ESC 메뉴막대의 다섯 메뉴(게임·보기·옵션·플레이어·정보)를 열고, 하위 메뉴가 있는 행을 모두 눌러 내려가며 모든 행의 라벨을 모은다(명령 행은 누르지 않는다). 단축키 표기(`~iF4~.`)와 서식 자리표시자를 뗀 뒤 영문 낱말 3자 이상이 남은 행을 실패로 본다(허용: `FPS`·`English`·`Deutsch`·`NetStorm`). 제어 문자가 있는 행도 실패이다. 행 40개 이상을 확인한다.
- Node 시험 `tests/korean-ingame.test.mjs`는 실제 메뉴 코드가 요청하는 번역 키가 번역표에 모두 있는지 보며, 프레임 속도 선택지를 가짜 값 대신 실제 `frameRateChoices`로 바꿨다.

## 찾은 것과 고친 것

1. **번역되지 않은 행 — 프레임 속도 `75 FPS (Original)`.** 전수 e2e가 찾은 영어 행은 이 하나뿐이다. `RawGameMenus.options()`가 Frame Rate 라벨을 번역 함수에 거치지 않고 그대로 썼다(cpppj 1329 이전과 같은 결함). `www/ui/raw-game-menus.js`가 `#text(rate.label)`로 번역하고 `translations/xlat.korean`에 `75 FPS (원본)`을 더했다(`prepare-assets`로 `www/data` 복사본·manifest 갱신). `60 FPS`·`120 FPS`는 cpppj처럼 고유 기술 이름이라 번역표에 없다. cpppj가 더한 23키 가운데 웹 메뉴에 나오는 것은 이것뿐이다: 나머지 키 중 `Hide/Show Island Themes`·`View Netstorm Knowledge`·`Counterclockwise Piece Rotation`·`Send ...`·`Whisper To`·`Refresh Zones`는 웹 번역표에 이미 있고, 해상도 열(`1024 by 768` 등 10개)은 웹이 해상도 메뉴를 만들지 않으며(AGENTS 웹 제외), `OBSERVER`(멀티플레이 관전자 Player 행)와 `Scenario...`(Edit 호환 메뉴)는 웹 범위 밖이다.
2. **'플레이어' 메뉴 글자 깨짐 — Player 정보 창 제목.** 메뉴 행 라벨은 E1166에서 이미 해독했지만, 행을 눌러 열리는 Player 정보 창(장식 메뉴)의 제목 `Player %s`는 이름 칸의 표시 부호화 바이트(한국어 이름은 선두 `0x1f` 4바이트 시퀀스)를 해독하지 않고 이진 문자열 그대로 그려 제목 뒤에 `| ㅁ , ·` 같은 깨진 글자가 나왔다. 같은 원인으로 동맹 줄(`Allied with ~i%s~.`)의 다른 Player 이름도 깨질 수 있었다. 고친 곳:
   - `www/core/raw-player-menu.js`: 이름 칸을 읽는 `displayName(owner)`을 더해 제목·동맹 줄이 선택 장치 `decodeName`으로 해독한 문자열을 쓰게 했다(장치가 없으면 이전과 같다 — 원본 292입력 fixture 대조 시험은 장치 없이 그대로 통과).
   - `www/ui/client-player-menu.js`: 장치가 있으면 `decodeName`을 Player 메뉴 hooks에 넘긴다.
   - `www/netstorm.js`: Player 메뉴 devices에 `decodeName`(표시 부호화 `displayTextCodec.decode`)을 연결한다.

## 검증

- e2e `korean-esc-menu.spec.js` 통과(고치기 전에는 `옵션 > 프레임 속도: "75 FPS (Original)"` 하나로 실패). 화면 캡처로 Player 정보 창 제목이 고치기 전 `플레이어 | ㅁ , ·`에서 `플레이어 간수`로 바뀌었음을 확인했다.
- `tests/raw-player-menu.test.mjs`에 이름 해독 시험을 더했다: `decodeName`이 있으면 제목 `Player 알파\n`과 동맹 줄 `Allied with ~i간수~.(One-Way)\n`이 나오고, 없으면 같은 바이트가 `\u001f`가 든 이진 문자열로 남아 달라진다(변형 확인을 겸함).
- `tests/korean-ingame.test.mjs`: 프레임 속도 라벨이 번역 경로를 지나는지, `75 FPS (Original)` → `75 FPS (원본)`인지 단언한다.
- 관련 Node 시험 410건(player·menu·korean 이름 시험 묶음) 가운데 1건이 실패했으나 이 단위의 변경이 아니다(아래 '기존 실패').
- 관련 e2e 12건(`player-menu-actions`·`player-menu-items`·`korean-players-menu`·`korean-ingame` 4건·`send-money-dialog` 2건·`frame-rate` 2건·`korean-esc-menu`) 통과.

## 기존 실패(E1288) 교정

`tests/korean.test.mjs`의 `createCanvasGlyphRasterizer` 시험이 E1288(`a18c42805`, 래스터라이저를 4배 그리기·칸 평균으로 바꿈) 뒤부터 실패하고 있었다(내 변경을 치운 기준 상태에서도 같은 실패를 재현). 가짜 canvas가 설계 크기 그대로 3×4 상자를 그려 4배 그리기에서는 칸 평균이 1×1이 됐다. 가짜 `fillText`가 글자 크기 비율(`emSize×4/13`)만큼 상자를 키우게 하고 글꼴 문자열 기대값을 `52px`(13×4)로 고쳤다. 래스터라이저 본체의 결함이 아니라 시험의 가정이 낡은 것이다. E1288은 집중 시험만 돌려 이 실패를 놓쳤다.

## 남은 확인

- 실제 원본 화면과의 대조는 해당 없음(원본은 영어 메뉴). 한국어 문구의 사람 검수는 3.5절 항목이다.
