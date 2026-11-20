# Shift-F7 화면 저장 (WB04-E1100)

미션 중 Shift-F7을 누르면 현재 화면을 256색 GIF 한 장으로 만들어 브라우저 내려받기로 저장하고, 원본처럼 `TellGifWritten` 안내 창을 띄운다. 이전에는 Shift-F7이 일반 F7(섬 소유자 표식 토글) 쪽으로 흘러가거나 소비만 됐다. cpppj `client/GifScreenshot.cpp`의 계산 부분(키 분배, 파일 이름, 고정 팔레트, GIF 인코더)을 [screenshot-gif.js](../webpj/www/core/screenshot-gif.js)로 옮겼다.

## 구현

- **키 분배.** `dispatchScreenshotKey`가 원문과 같은 조건으로 Shift-F7만 소비한다: 수정키를 지운 코드(`0x03ffffff`)가 F7(`0x760000`)이고, 뗌·Control이 없고, Shift가 있을 때뿐이다. 소비하면 저장 장치를 부르고 `TellGifWritten` 안내를 연다. 정지(Pause) 중에도 저장하며, 장치가 없으면 Tell 전에 거부한다. 앱의 미션 키 처리(`netstorm.js`)가 일반 F7 분배보다 먼저 이 분배를 호출하므로 Shift-F7은 표식 토글로 새지 않는다.
- **파일 이름.** 원문 `GifOutName`의 `pic<signed 번호>.gif`이다. 번호는 앱이 미션 사이에도 유지하는 32비트 카운터이며 저장할 때마다 1씩 오른다. 원문은 디스크에 이미 있는 이름을 건너뛰지만, 웹은 브라우저가 중복 이름을 알아서 구별(`pic0 (1).gif`)하므로 존재 확인·잠금·로그 장치는 옮기지 않았다.
- **팔레트.** `screenshotPrimaryPalette`가 고정 primary 표(256색 × 56바이트, 색마다 +24에 R·G·B double)에서 RGB를 `×255` 절삭으로 읽는다. 현재 flash·날씨 팔레트를 섞지 않으므로 화면이 붉게 번쩍이는 중에 저장해도 GIF 색은 기본 팔레트다. 범위(0..255)를 벗어난 값은 거부한다.
- **인코더.** `encodeScreenshotGif`가 팔레트 인덱스를 그대로 GIF87a 한 장(전역 색표 256, LZW 최소 코드 8)으로 쓴다. 사전이 4096 항목에 닿으면 clear 코드를 내고 다시 시작하며, 코드 폭은 해독기보다 한 항목 늦게 늘어난다. 255바이트 서브 블록으로 나누고 화면 버퍼의 행 간격(pitch) 패딩은 건너뛴다.
- **앱 연결.** `_saveScreenshot`이 현재 8비트 화면(`Screen.lock/unlock`)과 `ColorSort`의 primary 표를 인코더에 넘기고, 결과를 [downloadBytes](../webpj/www/platform/external-opener.js)로 내려받는다. `downloadBytes`는 같은 출처의 Blob 주소를 숨은 `<a download>`로 눌렀다가 곧 반납하며 외부 접속이 아니다. `options.download` 훅으로 시험에서 대체할 수 있다.

## 검증

- 새 `tests/screenshot-gif.test.mjs` 7건: 해독기(`assets/gif.js`)로 되돌려 크기·색 번호가 같음(패딩 포함 6종), 큰 잡음 화면에서 clear가 나오는 사전 재시작, **clear 위치(처음·두 번째 clear 사이 데이터 코드가 정확히 3,839개)**, 잘못된 크기·간격·색표 거부, primary 팔레트 변환과 범위 거부, 키 분배(Control·뗌·Shift 없음·다른 키는 소비하지 않음), 파일 이름의 signed 번호. 변형 확인에서 코드 폭 상한·사전 한도 경계·행 간격·절삭 대신 반올림·색표 플래그·서브 블록 길이·signed 번호·Shift 조건 제거가 모두 시험 실패로 잡힌다(색표를 0으로 채우고 덮어쓰는 변형은 모든 칸을 다시 쓰므로 동등 변형이다).
- `tests/external-access.test.mjs`의 앵커 요소 감사는 같은 파일 안의 두 곳(링크 열기, 화면 저장 내려받기)을 파일 단위로 한 번만 세도록 고쳤다. 앵커를 만드는 파일은 여전히 `platform/external-opener.js` 하나뿐이다.
- 새 `e2e/screenshot-key.spec.js`(Chromium): Tutorial1 진행 중 Shift-F7이 화면 크기와 같은 GIF 한 장(`pic0.gif`, 20,000바이트 초과)을 내려받고 `TellGifWritten` 창이 열리며 F7 표식 토글 설정이 바뀌지 않음; 창을 닫고 한 번 더 누르면 `pic1.gif`가 내려받아짐. 내려받은 GIF를 눈으로 열어 확인했다(구름 배경·섬·좌측 HUD가 화면과 같다).

## 남은 경계

- 원문 `SaveNumberedScreenshot`의 디스크 번호 건너뛰기(이미 있는 `picN.gif` 확인)·파일 잠금·로그 기록은 브라우저 내려받기로 대체돼 웹에 없다. 파일 이름 번호는 한 번 열 때마다 0부터 다시 시작한다.
- 웹 연결은 미션 키 처리 경로에만 있다. 안내 창(Tell)이 열려 있는 동안에는 키가 입력 큐로 가지 않으므로 저장하지 않는다(`TellGifWritten`을 닫아야 다음 저장이 된다). 메인 메뉴 화면에서의 저장은 연결하지 않았다.
