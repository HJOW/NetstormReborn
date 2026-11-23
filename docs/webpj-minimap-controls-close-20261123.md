# RawMinimapControls partial → port 판정 (WB04-E1248)

대응표 `cpppj/src/client/RawMinimapControls.cpp` 행의 남은 항목 세 가지를 도달성과 기존 증거로 하나씩 판정해 행을 `partial` 에서 `port` 로 닫았다. 코드 변경은 없다(partial 29→28단위, 20,275→20,094줄, port 979).

## 남은 항목과 판정

| 남았다고 적힌 것 | 판정 | 근거 |
| --- | --- | --- |
| 클릭·드래그 실사용 확인 | **이미 확인됨** | `e2e/minimap.spec.js`(E1128): 실제 Chromium 마우스로 미니맵을 눌러 카메라가 그 위치로 가고 눌러서 끌면 따라 움직이며 F5 로 사제 시점으로 돌아옴. 우클릭 팝업은 `e2e/mission-object-commands.spec.js`(E1236)가 실제 우클릭으로 확인 |
| 원본 fixture 전체 대조 | **이미 확인됨** | geometry 는 `tests/minimap-geometry-native-fixtures.test.mjs`, blit/raster 는 `tests/minimap-blit-native-fixtures.test.mjs`·`minimap-raster.test.mjs` 가 원본 10.78 x86 실행 결과와 대조. 조립(수명·입력·dirty 대여)은 `tests/raw-minimap-controls.test.mjs`. 미션 화면의 미니맵 칸은 원본 스크린샷과 픽셀 차이 0(E1082) |
| `ToggleMinimap`(원문 `00480810`, 명령 `0x02000037`) 대응 없음 | **도달하지 않음 → 범위 밖** | 아래 |
| `SetPalette` 를 설치 때 한 번만 호출 | 무해(E1224 판정 유지) | battlePal 전환이 HUD 생성 전에 끝난다 |

## ToggleMinimap 도달성

원본 `ToggleMinimap` 은 현재 미니맵이 있으면 지연 Kill 로 닫고 없으면 같은 공장으로 100,100 위치에 다시 만든다. 이 명령(`0x02000037`)에 닿는 길을 모두 찾았다.

- cpppj 에서 코드 `0x02000037` 을 받는 곳은 `UberGump.cpp` 의 일반 입력 전환(`transitionCode==0x02000037`)과 `GameMenuCommands.cpp` 의 이름표(`"toggleminimap"`) 두 곳뿐이다. 이름표는 대본·대화상자 단추·개발자 명령이 이름으로 명령을 부를 때 쓰는 표이다.
- 웹에는 같은 이름표 항목(`core/game-menu-commands.js`)만 있고 이 코드를 받는 처리기와 키 연결이 없다. 기본 키 입력(`core/view-key-input.js`: F2·F3·F4·F5·F6·F7·F9 등)에 이 명령을 보내는 키가 없다.
- 추출한 전체 영어 대본(`extracted/cpp-korean-preparation`), 한국어 번역(`webpj/translations`, `assets/lang`), 원본·웹 자산 어디에도 `toggleminimap` 이 없다(대소문자 무시 검색).

따라서 단독 미션(캠페인·데모·스커미시)에서 이 명령이 불리는 경로가 없고, 구현해도 화면으로 도달 불가이다(E1238·E1242·E1243 과 같은 방식의 도달성 판정). 나중에 키 설정 UI 나 대본이 이 이름을 쓰게 되면 그때 `RawButtonControls::ToggleMinimap` 과 함께 이식한다.

## 시험

코드 변경이 없어 새 시험은 없다. 대응표·manifest 시험(`tests/port-map.test.mjs`·`tests/fixture-manifest.test.mjs`)으로 행 형식과 상태 규칙을 확인했다.

## 남은 일

- 원본을 실제로 실행해 `ToggleMinimap` 키 연결이 정말 없는지는 확인하지 않았다(소스·자산 검색 판정).
- 5-B 의 나머지 행(RawProductionControls·UberHud·CampaignCursorClientEffects)은 그대로 남아 있다.
