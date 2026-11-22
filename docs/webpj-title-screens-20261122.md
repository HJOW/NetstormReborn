# 시작 title 그림 세 장 (WB04-E1184)

원본은 실행하면 메인 메뉴 전에 title 그림 세 장을 차례로 보인다(원문 Netstorm.exe `0x4ab3e0` `runTitleBitmaps`, 웹 이식 `core/title-bitmap-lifetime.js`). 웹에는 이 장면이 없어 '클릭하여 시작' 직후 바로 메인 메뉴였다. 이 단위가 세 장을 앱에 연결했다.

## 원본 동작과 웹 대응

| 원본(4ab3e0) | 웹 |
| --- | --- |
| 시작 전 입력이 이미 있으면 모두 건너뜀 | 클릭하여 시작을 누른 그 입력은 시작으로 쓰이고, 그 뒤 입력이 건너뛴다 |
| `title0.gif` + `title00.col` 팔레트, 3초 | 그림 1(ACTIVISION PRESENTS), 3초 |
| `title6.gif` + `title06.col`, 3초 | 그림 2, 3초 |
| `title7.gif`(팔레트 그대로), 3초 | 그림 3(Ticonderoga Entertainment), 3초 |
| 입력(키·마우스)이면 남은 그림 전체를 건너뛰고 검정으로 비운 뒤 메인 메뉴 | `click`·Enter/Space·아무 키(자동 반복 제외)가 `_finishTitleScreens` 로 메인 메뉴 진입 |
| 가운데 정렬 복사(640×480 그림을 화면 가운데) | `titleScreenOrigin` 으로 논리 화면 가운데에 그림, 나머지는 검정 |
| 화면 팔레트(0 검정·255 흰색 고정) | `buildScreenPalette(GamePalette.parse(*.col))` 로 RGBA 를 만들어 캔버스에 그림 |

- 시간표·가운데 좌표·팔레트 변환은 순수 함수(`ui/title-screens.js`: `titleScreenIndexAt`·`titleScreenOrigin`·`titleScreenRgba`)이고 앱(`netstorm.js`)의 새 상태 `AppState.TITLE`(`_enterTitleScreens`·`_drawTitleScreen`·`_finishTitleScreens`)이 그린다. 원문 첫 3초 기한은 title6/7 적재 시간을 포함하지만 웹은 자산이 이미 적재돼 있어 표시 시간만 센다.
- 자산 다섯 개(`d/title0.gif`·`d/title6.gif`·`d/TITLE7.GIF`·`d/TITLE00.COL`·`d/TITLE06.COL`)는 `tools/prepare-assets.js` 의 `menu` 묶음에 더해 `assets/game-data` 에서 복사하고 매니페스트를 갱신했다. `buildMenuResources` 가 `titleScreens` 로 해독해 둔다.

## 켜고 끄기

기본은 **켜짐**이다. 다음에서는 기본으로 건너뛴다: 원본 비교 모드(`originalComparison`, 오라클 화면 대조에 방해), 개발용 지형 미리보기(`?map=`), 자동화 브라우저(`navigator.webdriver` — 기존 e2e 가 각각 9초씩 더 기다리지 않게). 주소 `?title=1` 이면 항상 켜고 `?title=0` 이면 항상 끈다.

## 시험

- `tests/title-screens.test.mjs`(4건): 시간표 경계(0·2999·3000·5999·6000·8999·9000), 가운데 좌표, 팔레트 변환과 거부, 실제 자산 세 장의 640×480·팔레트 칠하기.
- `e2e/title-screens.spec.js`(2건, Chromium `?title=1`): 클릭 뒤 title 상태에서 640×480 그림이 검은 화면 가운데에 보이고 3초 간격으로 세 장이 유지되다 9초 뒤 메인 메뉴, 클릭·Enter/Space 입력으로 건너뜀, 자동화 기본값은 건너뛰기. 실제 화면을 캡처해 첫째(ACTIVISION PRESENTS)·셋째(Ticonderoga Entertainment) 그림이 가운데에 나옴을 확인했다.
- 원본 실행 화면과의 픽셀 대조(그림 1의 배경 회색 등 팔레트 세부)는 하지 않았다.
