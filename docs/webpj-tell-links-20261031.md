# webpj Tell 대화상자의 링크와 본문 스크롤바 (WB03-K, 2026-10-31)

2026-10-31, **HJOW-X3D**(마지막 전체 정밀 디컴파일 호스트 HJOW-Athlon/2026-10-23 과 다른 PC 이지만 이번 단위는 디컴파일 소스를 읽지 않았다. 근거는 cpppj 소스·원본 기계어 기대값 파일·기존 문서다). 사용자 요청("AGENTS.md, LEFT_JOBS.webpj.md 를 읽고 webpj 다음 작업 진행")에 따라 [WB03-J 인계](../LEFT_JOBS.webpj.md)의 첫 '다음(이어서)' 항목인 **Tell 창 링크 훅(`<a href>`)** 과 **Tell 창 본문 스크롤바**를 했다. 링크를 분배하는 원본 규칙을 새 모듈로 옮겨 도움말 창과 Tell 창이 함께 쓰게 했다.

`cpppj/`·`dotnetpj/`·`originals/`·`originalCD/`·AGENTS.md·공통 `LEFT_JOBS.md` 는 수정하지 않았다(다른 세션이 같은 저장소에서 cpppj 를 `CP` 커밋으로 계속 진행 중이라 읽기만 했고 내 파일만 커밋했다). 원본 게임·analyzeManager 는 실행하지 않았고 브라우저 검사는 창이 뜨지 않는 Edge 헤드리스다.

## 1. 근거

| 항목 | 근거 |
|---|---|
| 링크 분배(명령·주제·바깥 주소) | cpppj `client/HelpLinkPolicy.cpp`(`ParseHelpLinkAddress`·`DispatchHelpLink`, 원본 `004a00c0`). 기대값 `cpppj/tests/fixtures/help-links-x86.tsv` 91행(원본 토큰 helper 까지 실제 CPU 로 실행한 결과). |
| 링크 누름의 흐름 | cpppj `client/HtmlWindowGump.cpp` `Handle`: 링크 태그 사건(`tagEvent`)이면 **창을 먼저 Kill 하고**(모달 목록에서 즉시 빠짐) 그다음 링크를 처리한다. `RawMenuControls::ActivateHtmlWindowLink` → `HelpLink` 가 도움말 창을 여는 쪽(`OpenHelp`, 방식 1·학습 0)·`cmd:` 스크립트·바깥 주소로 나눈다. 도움말 열기(`OpenQuickHelpWindow`)는 현재 모달이 없을 때만 열리는데 Kill 이 모달 목록(`RemoveDialog`)에서 즉시 빼므로 Tell 에서도 열린다. |
| 본문의 링크 판정과 끌기 | cpppj `client/ScrollingTextGumpWidgets.cpp` `Handle`(왼쪽 누름: 조각의 태그가 있으면 링크 사건, 없으면 끌기 시작·입력 붙잡기)·`Broadcast`(끌기는 스크롤 허용 비트가 있을 때만 위치를 옮김). 이미 WB03-G 에서 `ScrollingText.hitTest`·`scrollBy` 로 옮겨 도움말 창이 쓴다. |
| 본문 스크롤바 | cpppj `client/HtmlWindowFit.cpp` 37~42행: 화면에 들어가지 않으면 폭을 화면에 맞추고 높이를 폭의 절반으로 줄인 뒤 `SetSlider(1)`·`SetScrollable(1)`·`RequestScroll(0)`. 스크롤바는 본문 생성 때 만들어 두며 슬라이더·스크롤 허용 비트가 모두 있을 때만 보인다(`ScrollingTextGumpWidgets::Attach`). |
| 도움말 Back 의 이전 창 복귀 | cpppj `client/QuickHelpControl.cpp` 89~95행·`QuickHelpWindow.cpp` 38행: 아래 4절. |

## 2. 구현

| 파일 | 내용 |
|---|---|
| `www/ui/help-link.js`(신규) | `parseHelpLinkAddress`(원본 토큰 helper, 구분자 `:`, 접두 80·본문 256 미만, 첫 NUL 까지), `classifyHelpLink`(`'script'`·`'help'`·`'external'`), `dispatchHelpLink`(분류에 맞는 훅 하나만 호출). 규칙: 접두의 앞 네 글자가 `cmd:`(ASCII 대소문자 무시)면 명령, 접두가 없거나 본문이 `#` 로 시작하면 도움말 주제, 그 밖은 바깥 주소. |
| `www/ui/tell-window.js` | `createTellWindow` 가 `LinkTable` 을 만들어 본문 변환의 `link` 훅에 연결하고(`<a href>` → `~[T번호]`), `TellWindow` 가 `links`·`onLink`·`now` 를 받는다. `pointerDown`: 스크롤바 → 돌 버튼 → 목록 행 → 본문 순으로 받고, 본문의 링크 조각이면 `onLink(목적지)`, 아니면 스크롤바가 있을 때 끌기를 시작한다. `fit` 의 긴 본문 경로가 `_enableScroll`(VerticalSlider 를 본문에 연결)을 부르고 크기가 정해진 뒤 `setScroll(0)`. `place` 가 스크롤바를 본문 오른쪽 가장자리에 붙이고 `draw` 가 그린다. `pointerMove`·`pointerUp`·`cancel`·`update`·`wheel` 이 스크롤바·끌기·반복 이동·휠을 처리한다. |
| `www/ui/help-window.js` | 링크 따라가기를 `dispatchHelpLink` 로 바꿨다. `cmd:` 본문은 원본 명령 큐 넣기 규칙(`parseScriptCommand`)으로 이름·인자로 나눈다. 한도를 넘은 목적지는 따라가지 않고 알린다. |
| `www/netstorm.js` | `_openTell` 이 `now`·`onLink` 를 넘긴다. `_onDialogLink`: 대화상자를 먼저 닫고 `$OnExit` 를 받아 둔 채 `_followLink` 로 링크를 분배한 뒤 OnExit 명령을 실행한다(원본은 닫힌 창을 정리할 때 OnExit 를 큐에 넣으므로 링크 처리보다 뒤다). `_followLink`: `cmd:` 는 `_runDialogCommand`, 도움말 주제는 `_openHelp`, 바깥 주소는 콘솔에 알린다. 휠은 `wheel()` 이 있는 열린 대화상자(도움말·스크롤바가 있는 Tell)에 넘긴다. |

## 3. 동작

- **링크를 누르면** 효과음 없이 즉시 따라간다(도움말 창과 같다). Tell 창은 닫히고, 도움말 주제 링크면 도움말 창이 그 주제로 열린다(방문 기록은 그 주제 하나로 시작). `cmd:` 링크는 닫힌 뒤 명령으로 실행된다(예: `cmd:Tell,CraftWarning(a,b,c,d)` → 이름 `Tell`, 인자 `CraftWarning(a,b,c,d)`).
- **바깥 주소**(`http:`·`mailto:`·`file:` 등)는 열지 않고 콘솔에 알린 뒤 창만 닫는다. 실제 사례: 시작 팁 0(`http://www.netstormhq.com`), Technical Support(`mailto:`), `PieceDump`(`file:`).
- **실제 링크가 있는 Tell 섹션**(영어): `TechSupport`(바깥), `Tips` 팁 0·39(바깥), `BuildNoBucks`·`UpgradeNoBucks`(`stormPowerHelp`), `NoTechTheme`(`factoryHelp`·`technologyHelp`·`unitHelp`), `ChalIntro`(`multiQuickHelp`·`battleIslandType`·`challengeIslandType`), `PlayerInfo(…)`(`cmd:`), `PieceDump`, `Remvoed`. 이 중 웹 메뉴에서 지금 닿을 수 있는 것은 시작 팁 0 과 도움말 → Technical Support 뿐이고 나머지는 미션·멀티플레이가 생긴 뒤 열린다(검사는 섹션을 직접 연다).
- **스크롤바**는 본문이 화면(폭 − 40, 높이 − 65)에 들어가지 않을 때만 생긴다. 본문 폭을 화면에 맞추고 높이를 폭의 절반(요청 높이가 더 크면 그 값)으로 줄이므로 긴 본문의 창은 가로로 긴 모양이다. 스크롤바: 화살표(안쪽에서 뗄 때 5픽셀, 눌러 두면 0.4초 뒤부터 0.05초마다 반복), 손잡이 위·아래 홈(한 쪽 = 본문 높이), 손잡이 끌기, 링크가 아닌 본문 글자를 누른 채 끌기(끄는 반대 방향), 마우스 휠(웹 적응, 한 칸 45픽셀)이다. 스크롤바가 없는 짧은 본문은 끌어도 움직이지 않는다.
- **현재 `tell.english` 303개 섹션 중 1024×768·1366×768 에서 스크롤바를 쓰는 것은 없다.** 스크롤바는 검사용 긴 섹션(90줄)과 이후 긴 미션 문구·한국어 번역에서 쓰인다.

## 4. 정정: "도움말을 Tell 에서 열었을 때 Back 이 Tell 을 다시 연다"는 동작은 없다

이전 인계(WB03-G·H·I·J)는 "cpppj 의 `popup.command` 재등록으로 Back 이 이전 Tell 을 다시 여는 동작"을 후속 과제로 적었다. cpppj 를 다시 읽어 보니 이 해석이 맞지 않았다. `QuickHelpControl.cpp` 의 Back 은 방문 기록에서 이전 주제가 없으면 도움말을 닫고 **`popup.command` 가 0 이 아닐 때만** 그 명령을 큐에 넣는데, `popup.command` 는 ① 미션 스크립트가 도움말을 열었을 때(`QuickHelpWindow::Construct` 의 `scripted()` 분기, 스크립트 명령·이름 저장)와 ② 지식 창을 열 때(`RawKnowledgeMenu`, `kShowTechnologyCommand`)에만 표시된다. Tell 대화상자의 링크로 도움말을 열 때는 표시하지 않으므로 **Back 은 도움말만 닫는다**(웹 구현과 같다). 미션 스크립트가 여는 도움말의 Back 재등록은 미션 구현(WB05~)에서 연결한다. 실제 브라우저에서 `NoTechTheme` 의 `technologyHelp` 링크 → 도움말 → Back 이 도움말만 닫는 것을 확인했다.

## 5. 원본과 다르게 둔 점(웹 적응·미구현)

- 바깥 주소는 열지 않는다. 원본은 `autoOpen` 설정에 접두가 있으면 셸로 열고 아니면 접두 이름의 명령 설정을 실행하지만, 웹은 바깥 프로그램을 실행할 수 없고 주소(과거 도메인 `netstormhq.com`·`titanic.com`)도 사라졌다.
- 마우스 휠 스크롤은 원본에 없다(도움말 창과 같은 웹 적응). 화면 크기가 바뀌어 창을 다시 맞출 때는 이전 스크롤바·스크롤 위치·끌기를 버리고 처음부터 잰다(원본은 창마다 한 번만 맞춘다).
- **본문 위 커서 모양은 이 단위에 없었다.** 원본은 링크 위에서 가리키는 손(커서 번호 16, `RT_CURSOR_18.bin`), 스크롤 가능한 본문 위에서 상하 화살표(커서 번호 15, `RT_CURSOR_17.bin`)로 바꾼다(`ScrollingTextGumpWidgets::Handle` 의 cursorEvent). *(2026-10-31 WB03-L 에서 두 커서를 웹 자산 묶음에 넣고 Tell 창과 도움말 창에 연결했다 — [본문 위 커서 모양 기록](webpj-cursor-20261031.md).)*
- Tell 섹션이 가리키는 도움말 주제 7개(`stormPowerHelp`·`factoryHelp`·`technologyHelp`·`unitHelp`·`multiQuickHelp`·`battleIslandType`·`challengeIslandType`)는 모두 도움말 파일에 본문이 있어 열린다. 종류 표(`.type`)가 필요한 능력치 머리글·종류 그림이 붙는 주제(유닛·건물 종류 도움말)는 이번 링크 경로에서 닿지 않으며 기존 후속 과제 그대로다.

## 6. 검증 (HJOW-X3D, Node v24.21.0, Edge 156 헤드리스)

- `npm --prefix webpj test` **97개 통과**(신규 10: `tests/link.test.mjs`). 기존 87개는 그대로 통과했다(도움말 창의 `cmd:`·바깥 주소 검사 포함).
- **원본 x86 기대값 대조:** `help-links-x86.tsv` 91행을 읽기 전용으로 쓴다. 중간 중단 10행을 뺀 81행(명령 6·주제 10·바깥 주소 65)의 분류, 명령 본문·주제 이름·접두·바깥 주소 전체 원문(첫 NUL 까지)이 원본 호출과 같다(바깥 주소 분기는 autoOpen 조회·셸 열기·접두 조회까지의 순서를 대조). 행 수를 고정했으므로 cpppj 가 이 파일을 다시 만들면 검사가 실패한다.
- **실제 자료:** 도움말 파일 128개 주제의 모든 링크 분류가 이전 판정과 같아 도움말 창의 동작이 달라지지 않았다. Tell 섹션 8곳의 링크 목적지·누름·분류, 링크가 아닌 곳·닫힌 창 입력, 긴 본문(90줄)의 스크롤바 생성·배치·휠·화살표·반복·쪽 이동·손잡이·본문 끌기·한계·취소, 스크롤바 그리기(보이지 않게 하면 달라지는 픽셀이 모두 스크롤바 사각형 안), 크기 변경 때 다시 맞춤(큰 화면에서 스크롤바 사라짐, 다시 작아지면 처음부터), 메인 메뉴 대화상자 자리 연결을 검사한다.
- **변이 검사 23종이 모두 걸렸다**(`help-link.js` 6·`tell-window.js` 15·`help-window.js` 2). 첫 실행에서 2건을 놓쳐 검사를 보강했다: 스크롤바 그리기는 패널 질감 색 때문에 '색이 여러 가지'로는 구별되지 않아 보이게/숨기게 한 두 그림의 차이로 바꿨고, 닫힌 창 확인은 `pointerDown` 이 이미 거르는 중복이라 코드에서 뺐다.
- **실제 브라우저(마우스·키보드 이벤트, 콘솔 오류·예외 0) 1024×768·1920×1080 각 31개, 합계 62/62:** 시작 팁 0 링크 → 창 닫힘·바깥 주소 알림·팁 번호 그대로 / F1 도움말 → `cmd:Tell,TechSupport` 링크 → Technical Support Tell → `mailto:` 링크 → 닫힘·알림 / `NoTechTheme` 의 `technologyHelp` 링크 → 도움말 주제 열림(기록 하나) → Back 이 도움말만 닫음 / 긴 Tell 의 스크롤바: 휠 ±·화살표 5픽셀·홈 쪽 이동·본문 끌기·크기 변경 뒤 가운데 배치·스크롤 0·Close. 화면을 눈으로도 확인했다(스크롤바 화살표·손잡이, 링크 글자 색).
- **확인하지 않은 것:** Chrome/Firefox·터치·실제 소리 출력·긴 한국어 본문(`.korean` 자료 없음)·링크 위 커서 모양*(WB03-L 에서 확인)*.

## 7. 남은 일

1. ~~본문 위 커서 모양~~ — 2026-10-31 WB03-L 에서 완료([본문 위 커서 모양 기록](webpj-cursor-20261031.md)).
2. 미션 스크립트가 여는 도움말의 Back 재등록(`popup.command`)은 미션 구현에서 연결한다.
3. 종류 표(`.type`)가 생기면 유닛·건물 종류 도움말 주제의 능력치 머리글·그림(기존 후속 과제).
