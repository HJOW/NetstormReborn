# 게임 메뉴막대 이식 (WB04-E539)

6단계의 "게임 메뉴" 중 `client/RawGameMenuBar`(원문 10.78 화면 위 가로 Game 메뉴막대 004cc210)를 `webpj/www/core/raw-game-menu-bar.js`로 옮겼다.
기준선 `225553df`와 현재 cpppj 소스가 같음을 확인했다. 하위 부모(`Game`·`Edit`·`View`·`Options`·`Players`·`Cheat`)는 `RawGameMenus` 단위(다음 후보)의 몫이며 이 모듈은 `child` 훅으로만 부른다.

## 옮긴 범위

- `RawGameMenuBar.open()`: 현재 Player/검산 준비 → 가로 메뉴 생성 → `Game` → (Editor일 때) `Edit` → `View` → `Options` → 검산 DWORD +3(감김, signed 9600 초과 시 9600 빼기) →
  (Battle일 때) `Players` → About 세로 메뉴(`General Help - ~iF1~.`·`Technical Help`(include는 기술 도움말 가용성)·구분선·`Version`) → controlled에서 About을 꺼내 현재 제목으로 부착 →
  (Cheat와 Mission이 모두 참일 때) `Cheat` 부착 → 남은 하나 꺼내기 → 폭(Width−Left의 signed 차이, 단정도) → 폭 설정 뒤 다시 읽은 Left로 x 부착(y 0, 현재 루트, mode 1) → 생성순번 기록.
  각 하위는 번역 → 하위 부모 몸체 → 자식 부착 순서이며, 메뉴 생성 뒤 바뀔 수 있는 모드·폭·생성순번은 원문 접근 순서대로 다시 읽는다.
- `createGameMenuBarState(tracking)`: 0054d4e0 검산 DWORD의 실제 소유 `{ value }` 상자를 빌린다(별도 사본 없음). `menuSerial`(005cad24, 토큰/등록 칸 id와 구별하는 Gump+2c의 생성순번)과
  세 명령 전역(F1 도움말 0x02000017·Technical 0x02000049·Tell 0x02000095)을 함께 보관한다.
- `bindGameMenuBarControls(hooks, widgets, controls, menus, node, storeText)`: 이미 옮긴 `MenuGumpWidgets`(가로/세로/부착)·`MenuGumpControls`(항목/하위/구분선)·`MenuGump`(꺼내기/폭)로 위임하고,
  문자열 인자는 문자열 고리로, 빈 문자열은 숫자 0으로 구별하며, 사라진 노드는 성공으로 바꾸지 않는다.
- 선택한 분기에서 도달한 경계만 요구하며 누락은 `Error`(`연결 누락`)로 거부한다(없는 하위를 빈 성공으로 만들지 않는다).

## 시험

`webpj/tests/raw-game-menu-bar.test.mjs` 4건:

1. 원본 관측 `game-menu-bar-x86.tsv` **510행**: 입력 9 DWORD(editor/battle/cheat/mission/검산/기술 도움말/폭/Left/변형)에서 최종 검산 DWORD·생성순번·하위/번역/항목/폭/부착의 전체 호출 순서.
   **첫 실행에서 510행 전부 일치**했다.
2. 계약: 없는 하위 거부(앞선 준비/가로/번역 효과는 남음)·선택 경계 누락 거부·검산 상자 필수.
3. 계약: 현재 전역 읽기 순서(Cheat 단락·Left 먼저 Width·Left 재조회)와 폭의 단정도 반올림·음수 폭. 원본 fixture가 읽기 순서와 2^24 초과 정수를 관측하지 않아 변형 확인 뒤 보강했다.
4. 연결: 컨트롤/위젯/메뉴 위임, 숫자 0과 문자열 고리 구별, 사라진 노드 거부.

**변형 확인:** 검산 상수·임계·뺄셈·Editor/Battle/Cheat 분기·Cheat 단락 순서·마지막 꺼내기·폭 계산·Left 재조회·부착 mode·생성순번·기술 도움말 include·명령 교환·About 재번역·번역/하위 순서·준비 생략·fround·Cheat 하위 번호
20가지를 넣어 시험이 모두 실패함을 확인했다(처음에 살아남은 Cheat 단락 순서와 fround는 3번 시험으로 잡았다).

`webpj/tests/fixtures/manifest.json`에 `game-menu-bar`(510행, 원본 10.78) 항목을 추가했다.

## 남은 경계

- 하위 부모 `RawGameMenus`(Game/Edit/View/Options/Players)와 `RawPlayerMenu`, 실제 Gump 번역·Player 준비·기술 도움말 가용성 조회는 아직 연결하지 않았다(PORT_MAP `port`).
