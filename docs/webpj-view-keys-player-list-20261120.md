# 표시·안내 단축키와 플레이어 목록 (WB04-E1087)

미션 화면에서 F2(건물 숨김)·Shift-F3(섬 테마 켬/끔)·F8(목표 보기)·F9(플레이어 목록) 키가 메뉴막대 항목과 같은 명령으로 동작한다. 이전에는 이 키들이 입력 큐에 들어가지 않았고, 이식해 둔 표시 키 분배기(`dispatchViewKeyInput`)와 F9 명령 예약기(`queuePlayerListKey`)가 앱에 연결되지 않았다.

## 구현

- `_onKeyDown`이 미션 화면에서 F2·F3·F8·F9를 가상 키로 입력 큐에 올리고(브라우저 기본 동작은 막는다), 입력 프레임이 큐 소비 허용 목록에 이 키들을 더해 뒤의 입력이 막히지 않게 한다. F7(배너)은 소유 색표 재구성이 아직 없어 큐에 넣지 않는다.
- F2·F3·F8은 `dispatchViewKeyInput`에 현재 표시 전역(건물 숨김·월드 다시 그리기)과 장치를 연결해 처리한다. F8은 원본처럼 미션 플래그(`bridgeState.mission`)가 있으면 첫 명령(`0x02000091`)을 예약해 Tutorial 첫 안내를 다시 보인다. Shift 조합의 섬 테마 전환은 메뉴와 같은 `SetIslands(0|1)` 경로(`_tellOrSetIslands`)를 쓴다.
- F9는 `queuePlayerListKey`가 플레이어 목록 명령(`0x020000f0`, Shift면 정지 `0x020000cf`)을 예약하고, 앱이 `ClientGameMenuBar.openPlayerList()`로 원본 `RawGameMenus.players(true)`(장식 메뉴 "Player List", 화면 중앙 부착)를 연다. 메뉴막대의 `View Player List - F9`도 같은 명령이다. 각 행의 하위 메뉴는 기존 `ClientPlayerMenu`가 만든다.

## 검증

새 `e2e/game-menu-bar.spec.js`의 단축키 검사가 F2를 두 번 눌러 `hideBuildings`가 0→1→0으로 바뀌는지, F8이 목표 안내를 다시 열고 버튼으로 닫을 수 있는지, F9가 "Player List" 장식 메뉴를 화면 중앙에 여는지 확인한다(Chromium).

Node 22.23.3 전체 회귀 6,593건이 전부 통과했고(로그 `extracted/webpj-e1087-node22.log`) Chromium 7개 파일 33건이 모두 통과했다.

## 남은 경계

Shift-F9(정지)는 상태 표시줄(`PauseStatus` 경고 표시)이 없어 연결하지 않았다. F6(지식 보기), F7(배너 표시), F3(채팅 전환)과 플레이어 목록 각 행 하위 메뉴의 실제 명령 실행은 남아 있다.
