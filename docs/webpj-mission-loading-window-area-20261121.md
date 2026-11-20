# 미션 진행 창을 원본 지도 영역에 배치하고 대화상자의 영역 모서리 유지 (WB04-E1131)

E1130 에서 미션 안내 창을 HUD 를 뺀 지도 영역 가운데로 옮긴 데 이어, 미션이 시작되기 전의 진행 창("Starting Mission..." 등)도 cpppj(10.78 복원)와 같은 영역에 놓고, 대화상자가 열릴 때마다 영역이 화면 전체로 되돌아가던 결함을 고쳤다.

## cpppj 근거

- `cpppj/src/client/UberGump.cpp` 의 네이티브 로그인 장치 `startingProgress` 는 `ShowMissionLoading(LoadingHtmlContext({85,0,화면폭,화면높이}), "Starting Mission...")` 이다. `ScreenRect` 는 `{left, top, right, bottom}` 이므로 왼쪽 85px 를 뺀 영역이다(곧 도킹될 HUD 자리).
- 로그인 진행 문구 `ShowLoginProgress`·`ShowLoginWaiting` 은 `LoadingHtmlContext(CurrentMapView())` 이다. 현재 Gump 의 지도 영역이라 HUD 도킹 전에는 화면 전체이고 도킹 뒤에는 `{85,0,...}` 이다.

## 변경

| 위치 | 내용 |
| --- | --- |
| `www/netstorm.js` `_showMissionLoading(message, view)` | 보기 영역 `{ left, top, right, bottom }` 을 받아 `TellWindow` 의 `screen` 을 `{ left, top, width, height }` 로 만든다. 없으면 화면 전체. |
| `www/netstorm.js` 로그인 장치 | `startingProgress` 는 `{ left: 85, top: 0, right: 메뉴 폭, bottom: 메뉴 높이 }`, `progress` 는 `this._clientGump.mapView()` 를 넘긴다. |
| `www/ui/main-menu.js` `_fitDialog()` | 대화상자가 `screen.left`·`screen.top` 을 이미 갖고 있으면 그 모서리를 지키고 오른쪽·아래 끝만 현재 논리 화면 끝에 맞춘다. 이전에는 `setDialog` 와 창 크기 변경마다 `screen` 을 화면 전체로 덮어써서, E1130 의 `_openTell` 영역 지정이 한 프레임 뒤 `_syncDialogToMapView` 가 고칠 때까지 무효였다. 이제 열리는 순간부터 맞다. |

## 시험

- `tests/mission-loading-window.test.mjs`: 시작 안내가 `{left 85, top 0, width 939, height 768}` 이고 `x = 85 + (939 − 폭) / 2` 이며, 메뉴를 1280×720 으로 바꿔도 모서리 85 를 지키고 폭만 1195 가 된다.
- `e2e/l2-tutorial1.spec.js`: 실제 Chromium 에서 Tutorial1 을 시작할 때 `_showMissionLoading` 호출을 기록해, 시작 안내(`left 85`)가 한 번 이상 있고 모든 진행 창이 자기 영역 가운데에 있음을 확인한다. 첫 실행에서 `progress` 경로가 "Starting Mission..." 문구를 HUD 도킹 전 전체 화면 영역으로 여는 것이 보여(cpppj `CurrentMapView()` 와 같다) 단언을 영역별 가운데로 바꿨다.

## 남은 것

- 도움말 창(Help) 가운데 배치는 아직 원본 영역과 대조하지 않았다.
- 첫 프레임 논리 화면 대조(`load.png`)는 L2 고정 시각 모드와 함께 남아 있다([E1129](webpj-l2-tutorial1-20261121.md)·[E1130](webpj-tell-window-map-view-20261121.md)).
