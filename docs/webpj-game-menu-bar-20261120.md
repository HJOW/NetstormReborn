# ESC 게임 메뉴막대와 우측 상단 버튼 (WB04-E1084)

미션 중 ESC를 누르면 화면 위쪽에 원본의 가로 게임 메뉴막대(Game·View·Options·Players·About)가 나타나고, 다시 누르면 닫힌다. cpppj·dotnetpj에는 없는 웹 전용 요구로 화면 우측 상단에 `☰ Menu`(한국어 `☰ 메뉴`) 버튼을 두어 같은 막대를 열고 닫는다. 버튼은 Canvas 위의 DOM 요소(`z-index` 10)라 막대의 오른쪽 끝과 겹쳐도 항상 메뉴보다 앞에 보이고 눌린다. 버튼을 누른 뒤에는 포커스를 Canvas로 돌려 ESC가 계속 동작한다.

## 구현

- **막대 조립.** 이미 이식해 둔 `RawGameMenuBar`(가로 메뉴막대 순서·폭·부착)를 실제 Client 메뉴 작성기에 연결하는 [ClientGameMenuBar](../webpj/www/ui/client-game-menu-bar.js)가 원본 `OpenGameMenu`/`ToggleGameMenu`를 옮긴다. 같은 생성순번의 살아 있는 막대가 있으면 지우고 없으면 새로 만든다(열어도 게임 시계는 멈추지 않는다). 막대는 지도 왼쪽(85)부터 화면 오른쪽 끝, 맨 위에 붙는다. `ClientMenuGump`에 `beginHorizontal()`과 `bindGameMenus()`를 추가했다.
- **하위 메뉴.** 원본 `RawGameMenus`의 Game·View·Options·Players 부모를 웹 범위로 옮긴 [RawGameMenus](../webpj/www/ui/raw-game-menus.js)가 원본 조건과 순서(체크 값·설정 반전 문자열·음량 1~5·미션/데모 구분)를 따른다. Edit·멀티플레이 이탈·Quit Game·해상도/전체화면·서버 진단은 웹 제외 범위라 만들지 않는다. About은 General Help·Version이며 Technical Help는 웹에 도움말 프로그램이 없어 숨긴다. Players 하위는 기존 `ClientPlayerMenu`가 만든다.
- **ESC.** 원본 `UberGump`와 같이 선택을 먼저 정리하고, 조각을 들고 있으면(커서 잠금 0) 취소, 아니면 막대를 전환한다. 브라우저 ESC를 항상 입력 큐에 넣고 입력 프레임이 판단하도록 바꿨다(이전에는 조각을 든 경우만 큐에 넣었다).
- **웹 버튼.** [GameMenuButton](../webpj/www/ui/game-menu-button.js)이 Canvas의 현재 CSS 배치 오른쪽 위에서 8픽셀 안쪽에 놓이며 화면 크기가 바뀌면 따라간다. 막대가 열려 있으면 눌림 상태(`aria-pressed`)로 보인다. 미션을 떠날 때 함께 제거한다.
- **명령.** 항목은 원본 명령을 입력 큐에 올린다. 앱이 실행하는 것은 목표 보기(Tutorial 첫 안내 재표시)·건물 숨김·설정 토글(소리·음악·바람·스피커·화면 가장자리 이동·자동 데모·시작 팁·조각 회전)·음량 1~5·홈·사제 보기·Tell(Version·섬 테마)·도움말이다. 연결하지 않은 명령(재시작·나가기·정지·지식 보기·플레이어 목록·배너 표시)은 메뉴에서 비활성으로 보이고, 키보드 등으로 들어와도 큐를 막지 않도록 소비하며 콘솔에 기록한다. 이 변경으로 F9 같은 키가 큐를 영구히 막던 문제도 사라졌다.

## 검증

- 원본 스크린샷(`The War Begins! - In game menu.png`)과 같은 구성으로 막대와 Game·View·Options·Players·About 하위 메뉴가 그려진다(Chromium 화면 확인). Game 하위의 Quit Game만 웹 범위라 빠진다.
- 새 `e2e/game-menu-bar.spec.js` 2건: ESC와 버튼의 열고 닫기, 막대 위치(`left 85·top 0·right 1024`)와 항목 라벨, 열린 막대 위에서 버튼이 `elementFromPoint`로 앞에 잡힘, 목표 보기 → 안내 재표시, 건물 숨김 → 전역 토글, Sound On → 설정 변경, Volume 3 → 음량 설정, Version → Tell 창이 통과했다.
- 새 `tests/raw-game-menus.test.mjs` 5건(Game·View·Options·Players 구성과 조건, 막대 전환·폭·부착)과 변형 확인(재시작 행 제거·메타맵 관문 제거·음량 채널 뒤바꿈)이 모두 시험 실패로 잡힌다.

- **회귀.** Node 26.13.0 전체 회귀 6,591건 중 3건이 실패했다: 요청 프레임 시험의 가짜 루트가 새 메뉴 버튼(DOM) 생성을 지원하지 않았고(시험 대역에 최소 요소 생성·붙이기를 추가하고 버튼 배치 단언을 더했다), 대응표 두 검사가 새 모듈 세 개의 미등록을 지적했다(`RawGameMenus.cpp`·`RawGameMenuBar.cpp` 행에 등록). 세 가지를 고친 뒤 해당 시험 파일 11건이 통과했다. Chromium 8개 파일 32건 중 31건이 통과했고, 병렬 실행에서 한 번 실패한 첫 Tutorial 진입의 생산 항목 집기 확인(`tutorial-menu-start` 첫 시험)은 단독 재실행에서 통과했다(부하에 따른 타이밍 흔들림으로 보이며 이후 같은 현상이 반복되면 조사한다).

## 남은 경계

Restart/Leave Mission은 연결하지 않았다. 가이드를 닫고 월드를 반납하는 것만으로는 메인 메뉴 화면이 다시 그려지지 않아 메인 메뉴 재그리기·음악·State 복귀가 필요하다(4D-5). Pause(F9)·지식 보기(F6)·플레이어 목록(F9)·배너 표시(F7)와 Players 하위 메뉴 항목의 실제 실행도 남아 있다.
