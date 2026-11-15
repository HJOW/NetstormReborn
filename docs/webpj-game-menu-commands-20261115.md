# 게임 메뉴 명령 표 이식 (WB04-E538)

6단계의 "게임 메뉴" 중 `client/GameMenuCommands`(원문 메뉴가 읽는 명령 전역의 초기 ID, HTML/대본 이름→명령 조회, 일반 메뉴 명령 판정, ESC·F9 키 판정)를
`webpj/www/core/game-menu-commands.js`로 옮겼다. 기준선 `225553df`와 현재 cpppj 소스가 같음을 확인했다.
E536(`VortexCommandInput`)·E537(`RawKnowledgeMenu`)이 이 표의 선행 부품이었다.

## 옮긴 범위

- `gameMenuCommandValue(address)`: 확인한 원본 전역 주소(00542xxx, 호스트 포인터로 역참조하지 않는 식별자)의 명령 ID 또는 `null`. 함수 앞부분의 특수 주소 4개
  (개발자 NextStage 0x00542a40→0x020000d3, 설정 세 명령 0x0054279c/0x0054291c/0x00542a10)를 먼저 보고 표 64쌍(원문 순서)에서 첫 쌍을 돌려준다.
  표는 cpppj 소스에서 기계적으로 생성했다(손으로 옮기지 않음).
- `gameMenuScriptCode(name)`: 각 입력 모듈의 이름표를 원문 순서(MakeAi → AiUtility → CreateAt → DrawWater → AiDebug → CreateHelpList → Presentation → Utility →
  Configuration → ScenarioEffect → ScenarioObject → Vortex)로 먼저 조회한 뒤 이름 표 51개를 ASCII 대소문자 무시로 비교한다. 미등록은 0xffffffff.
  `CreateHelpList`만 입력 문자열의 첫 NUL까지 비교하고 나머지 이름 표는 NUL을 자르지 않는다.
- `isGameMenuCommand(code)`: 설정·유틸리티 명령, 주소 표의 모든 값, TechnicalHelp(0x02000049)·ScreenModeChange(0x02000002)·AcceptScreenModeChange(0x020000e4)이면 참.
- `isEscapeKeyPress(event)`: 뗌이 아니고 `code & 03ffffff`가 정확히 0x1b일 때만(수정키 무시, 명령/마우스/가상 키와 구별).
- `queuePlayerListKey(event, queue)`: F9(0x780000)를 Player List(0x020000f0), Shift+F9를 Pause(0x020000cf)로 두 인자 0과 함께 예약하며 뗌·Ctrl은 거부, Alt는 거부하지 않는다.
- exclude-host 단위(`QuickHelpList`·`CaptureInput`·`RecheckCdInput`)는 효과를 옮기지 않지만 명령 ID(0x0200009c/0x0200009d/0x020000fd)와 이름은 같은 표에 남겨 cpppj와 같은 조회 결과를 낸다.
- **옮기지 않은 것:** `HtmlScriptCommandValue`(UberGump의 `DialogScriptCode`가 Tell·Config·Warning·ViewSpot·MoveDialog·SetOwner·Connect·Salvage·SendMoney·FortSelection·mission 이름과
  기술 창 이름, 이 표를 묶는 해석기)는 UberGump 단위 몫이다.

## 시험

`webpj/tests/game-menu-commands.test.mjs` 7건:

1. 원본 PE 이름표 `html-solo-command-names-pe.tsv` **43개**: 전역 주소 조회는 `null`이거나 같은 명령이고, 이름(원문·소문자·대문자·교대 표기)은 같은 명령이거나 미등록이며
   네 표기가 모두 일관되어야 한다. 이 표가 직접 소유하는 주소 23개·이름 29개, 소유하지 않는 14개(Config·DoNothing·MissionAbort/Begin/Restart·MoveDialog·Salvage·SendAllBucks·
   SendBucks·SetOwner·StartDemo·Tell·ViewSpot·Warning)는 UberGump의 다른 해석기 몫임을 이름까지 고정했다. **첫 실행에서 일치**했다.
2. `knowledge-world-names-x86.tsv` **21행**: 이름 조회 → 문자열 고리 → 40칸 큐의 사건·토큰·고리 위치(E537에서 미룬 시험). 첫 실행 전부 일치.
3. **소스 교차 확인**: cpppj `GameMenuCommands.cpp`의 `kCommands` 64쌍과 이름 if-사슬 51개를 시험이 소스 텍스트에서 읽고 client 헤더의 `kXxxCommand` 기호 상수를 풀어
   `gameMenuCommandValue`·`gameMenuScriptCode`·`isGameMenuCommand`와 전부 대조한다(원본 PE 이름표가 닿지 않는 주소 항목까지 검증).
4. 계약: 특수 주소 4개, 기호 상수 28개 주소, exclude-host ID, 미등록 주소, 음수 주소는 하위 32비트, 입력 모듈 우선·대소문자·오자 거부·NUL 처리.
5. 허용표 계약, 6. ESC/F9 판정(cpppj `GameMenusTests`·`UserSelectionTests`와 같은 사건·예약 순서).

**변형 확인:** 특수 주소·이름 대소문자·HelpList NUL·허용표 세 리터럴·설정/유틸리티 분기·ESC 뗌/마스크·F9 Ctrl/뗌/Alt/명령 교환·입력 모듈 이름표·표 값 변경 등 21가지를 넣어
시험이 20가지에서 실패함을 확인했다. 유일하게 살아남은 "주소 첫 쌍 → 마지막 쌍"은 표에 중복 주소가 없어(64쌍 모두 서로 다른 주소) 관측 불가능한 동등 변형이다.

`webpj/tests/fixtures/manifest.json`에 `html-solo-command-names-pe`(43개)·`knowledge-world-names`(21행)·`src-game-menu-commands`(소스 교차 확인) 항목을 추가했다.
E537의 `raw-knowledge-menu.test.mjs`에는 cpppj `KnowledgeMenu_ActualScriptCommandQueue`에 해당하는 이름→큐 계약 시험 1건을 더했다.

## 남은 경계

- `HtmlScriptCommandValue`(UberGump `DialogScriptCode`)와 `GameMenuActions`·`PauseKeyInput` 같은 일반 입력 부모의 결합은 5-C의 Uber 허브 단위에서 연결한다.
- 이 표는 PORT_MAP `port`이며 앱 수명에는 아직 연결하지 않았다.
