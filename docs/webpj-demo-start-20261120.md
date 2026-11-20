# 데모 시작(StartDemo)과 메인 메뉴 Auto-Demo (WB04-E1092)

메인 메뉴 Demo 목록(Demoville)의 세 데모(폭풍은 몰아친다·내 등을 지켜라·해체된 동맹)가 시작되고, 메인 메뉴를 45초 동안 건드리지 않으면 Auto-Demo 옵션에 따라 DemoStormRages가 저절로 시작된다. 이전에는 `StartDemo` 명령이 "아직 구현되지 않았습니다" 로그만 남기고 아무 일도 하지 않았다.

## 구현

- **StartDemo.** `_runDialogCommand`가 `StartDemo`를 받으면 `_beginCampaignDemo(이름)`가 연결된 세 데모 이름만 받아(원본 `UberGump`의 `BeginMission(이름, 1)`) 현재 미션이 있으면 먼저 떠난 뒤 `Guide.begin(이름, 데모=1)`로 시작한다. 마지막 미션 기록에도 데모 표시를 남겨 Restart가 같은 데모를 다시 시작한다.
- **무활동 감시.** 이식해 둔 `idleMainMenuActivity`(원본 `004cee80`)를 메인 메뉴 프레임에 연결했다(`_idleMainMenuActivity`). 대화창이나 펼침 메뉴가 열려 있거나 포인터가 움직이면 기한(벽시각 +45초)을 다시 잡고, Auto-Demo 옵션(`autoDemo`, 기본 켜짐)이 켜져 있고 기한이 지나면 DemoStormRages를 시작한다. 미션 중에는 감시를 해제해 메인 메뉴로 돌아올 때 45초를 새로 센다.

## 검증

새 `e2e/demo-start.spec.js` 5건(Chromium): Demo 목록에서 세 데모를 각각 시작하면 데모 표시(`gate.demo=1`)로 월드가 8초 이상 흐르고 오류가 없다(DemoStormRages는 소유자 섬과 얼음 섬의 전투 장면이 그려진다); 데모 중 ESC 메뉴의 Game 하위가 `Restart Demo`·`Exit Demo`이며 Exit Demo가 메인 메뉴로 돌려보낸다; 무활동 기한을 앞당기면 Auto-Demo가 켜진 때만 데모가 자동 시작되고 꺼져 있으면 시작되지 않는다.

## 남은 경계

데모는 360초(`demoEndSeconds`)가 지나면 같은 데모를 다시 시작한다(원본 `endDemo` → `MissionReload`). 이 반복은 코드 경로(`Guide` 프레임의 `endDemo`)가 이미 연결돼 있으나 360초 대기 때문에 Chromium에서 확인하지 않았다. 데모 도중 사용자 입력으로 즉시 끝내는 원본 동작이 있는지는 원본 자료로 확인하지 못했다(현재는 ESC 메뉴의 Exit Demo로 나간다).
