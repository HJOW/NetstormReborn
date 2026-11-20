# 미션 안내 창을 지도 영역 가운데에 배치 (WB04-E1130)

미션 중에 뜨는 안내 창(Tutorial 대본의 `NetStorm!` 첫 창·브리핑·승리/패배 안내 등 `TellWindow`)이 화면 전체 가운데가 아니라 **왼쪽 HUD(85px)를 뺀 지도 영역의 가운데**에 놓이도록 맞췄다.

## 근거

- cpppj 기준선 `load.png`(Tutorial1 시작 직후 첫 화면)와 원본 스크린샷의 Briefing·Success 창은 모두 HUD 오른쪽 영역(x 85..1024)의 가운데에 놓여 있다. 원본은 대화상자 `Fit` 끝에서 "보기 영역 가운데"에 놓는데, 미션에서 보기 영역은 HUD 도킹 뒤의 지도 영역(`world.mapView()` = left 85, top 0, right 1024, bottom 768)이다.
- 웹은 이전까지 `screen` 을 항상 메인 메뉴 크기(1024×768 논리 화면 전체)로 넘겨 창이 85px/2 만큼 왼쪽으로 치우쳐 있었다.

## 변경

| 위치 | 내용 |
| --- | --- |
| `www/ui/tell-window.js` `place()` | 가운데 계산에 `screen.left`·`screen.top` 을 더한다. 값이 없으면 0이라 메인 메뉴의 기존 배치(화면 전체 가운데)는 그대로이다. 생성자에 `followsMapView = true` 를 더했다. |
| `www/netstorm.js` `_openTell` | 미션 화면이 활성이면 `screen` 을 지도 영역 `{ left, top, width, height }` 로 넘긴다. 아니면 기존대로 메인 메뉴 크기이다. |
| `www/netstorm.js` `_syncDialogToMapView()` | 튜토리얼의 첫 안내 창은 HUD 도킹보다 **먼저** 열려서 화면 전체 가운데에 놓였다가 어긋났다. 매 프레임 `_render` 직전에 열린 창(`followsMapView`)의 `screen` 이 현재 지도 영역과 다르면 영역을 바꾸고 `place()` 를 다시 불러 화면 표시를 더럽힌다(같으면 아무것도 하지 않는다). 도움말·게임 메뉴 창 등 자체 위치 규칙을 쓰는 창은 `followsMapView` 가 없어 대상이 아니다. |

## 시험

- `tests/dialog.test.mjs` 에 `place()` 의 영역 배치 시험을 더했다: 기존 화면 전체 가운데, `{ left 85, top 0, width 939, height 768 }` 일 때 `x = 85 + (939 − 폭) / 2`, 자식 본문 위치의 동행, `top` 가산.
- `e2e/l2-tutorial1.spec.js` 에 실제 Chromium 에서 Tutorial1 첫 안내 창의 `x` 가 `85 + (939 − 폭) / 2` 와 1px 안에서 같다는 단언을 더했다. 이 파일은 같은 단위에서 두 번 연속 통과했다.
- 영향 범위 Chromium 묶음 `tutorial-menu-start, english-tutorial2, korean-mission, pause-menu, demo-start` 가 33건 모두 통과했다(8.1분).

## 남은 것

- 미션 로딩 창의 위치(cpppj 는 `{85, 0, W, H}`)와 도움말 창의 가운데 배치는 아직 검증하지 않았다.
- 첫 프레임 논리 화면 대조(`load.png`)는 L2 고정 시각 모드와 함께 남아 있다([E1129](webpj-l2-tutorial1-20261121.md)).
