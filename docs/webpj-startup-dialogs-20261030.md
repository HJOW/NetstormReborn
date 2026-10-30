# webpj 시작 창(환영·팁)과 Tell 지역 인자·스크립트 명령 (WB03-J, 2026-10-30)

2026-10-30, **HJOW-Athlon**(마지막 전체 정밀 디컴파일 호스트와 같아 재디컴파일은 필요 없었으나 디컴파일 소스는 읽지 않았다. 근거는 cpppj 소스·원본 기계어 기대값 파일·기존 문서다). 사용자 요청("AGENTS.md·LEFT_JOBS.webpj.md 를 읽고 webpj 다음 작업 진행")에 따라 `LEFT_JOBS.webpj.md` 의 '다음(이어서)' 첫 항목인 **Tell 지역 인자 `Name(arg1,arg2)`** 와 두 번째 항목인 **시작 팁 창**을 한 단위로 했다. 팁 창이 쓰는 `Config` 버튼 명령·`TellTip` 명령·`{@tell.tipN}` 조회·조건 태그·본문 인라인 명령이 모두 이 단위에 필요해 함께 옮겼다.

`cpppj/`·`dotnetpj/`·`originals/`·`originalCD/`·AGENTS.md·공통 `LEFT_JOBS.md` 는 수정하지 않았다(다른 세션이 cpppj 를 작업 중이라 읽기만 했고 커밋에도 넣지 않았다). 원본 게임·analyzeManager 는 실행하지 않았다. 브라우저는 Edge 헤드리스(창 없음)다.

## 1. 근거

| 항목 | 근거 |
|---|---|
| 메인 메뉴 진입 때의 시작 창 순서 | 원본 State3(`004b6fa9` 이후), cpppj `client/NativeMainMenuState.cpp` 35행과 [문서](exe/cpp-native-main-menu-state-20261026.md): `WelcomeGiven=0` → Tell `Welcome` 만. 아니면 Tell `Blank` → (`startup()` 이 0 이고 첫 메뉴일 때) `UserNotUpdated` → `tellTips` 가 켜져 있으면 `Tips`. 첫 메뉴 표시는 `WelcomeGiven` 이 켜진 갈래의 끝에서 꺼진다. |
| Tell 이름과 지역 인자 | 원본 `004c8ea0`, cpppj `client/TellParameters.cpp` 와 [문서](exe/cpp-tell-parameters-20261023.md). 기대값 `cpppj/tests/fixtures/tell-parameters-x86.tsv` 1,070행. |
| Tell 조회·이름 기록·BLANK | 원본 `004cf730`, cpppj `client/TellSection.cpp`·`UberGump::TryTell` 와 [문서](exe/cpp-tell-section-client-20261025.md): BLANK 는 지역 인자 해석 앞의 조기 분기, 이름이 (대소문자 무시) 달라지면 이전 이름을 `global.priorTell` 에 쓰고, 조회가 실패해도 현재 이름은 남는다. |
| 팁 명령 | 원본 `004d6fc9..004d7044`, cpppj `client/TellTip.cpp`. 기대값 `tell-tip-x86.tsv` 388행. |
| 스크립트 명령 | 원본 `004c9090`(명령 큐)·`00441580`(Config 명령), cpppj `client/ScriptCommand.cpp` 와 [문서](exe/cpp-script-command-20261015.md). 기대값 `script-command-x86.tsv`(토큰 128·Config 18·명령 31행). |
| 팁 문구 | `tell.english` 의 `[TipList]`(`tip0=…`~`tip39=…`), `[Tips]` 섹션(`{@tell.tip{tipNumber}\|You're out of Tips!}`, `<?{tipNumber}>` 로 Prior Tip 단추 조건, `TellTip,1\|2\|3`, `Config,tellTips=0`). |
| 프로젝트 결정 | "Not Validated"(`UserNotUpdated`) 창은 구현하지 않는다(2026-09-30, [메뉴 흐름도](screens/main-menu.md), [네트워크 문서](exe/network-ports.md) 1-1절). |

## 2. 구현

| 파일 | 내용 |
|---|---|
| `www/ui/tell-parameters.js`(신규) | `parseTellParameters`: 이름은 구분자 `(`, 인자는 `,)` 로 원본 토큰 helper 가 읽고, 첫 빈 토큰에서 끝나며, 비어 있지 않은 각 인자를 설정 치환한 값을 `번호="값"` 줄로 `param` 객체에 덧붙인다. 1024바이트 이상은 RangeError. |
| `www/ui/script-command.js`(신규) | `parseScriptCommand`(이름 구분자 `#,;>`, 인자 구분자 `;>`, 숫자로 시작하면 32비트 정수), `applyScriptConfig`(`키=값`, 값은 LF 까지). |
| `www/ui/tell-tip.js`(신규) | `dispatchTellTip`: y=1 이면 번호 −1, y≥2 면 +1, 부호 있는 32비트로 0 이하면 0, `tipNumber` 를 `%d` 로 설정에 쓰고 y≠3 이면 `Tips` 를 다시 연다. |
| `www/ui/html-text.js` | 토큰 helper `readRawTextToken` 을 내보냈다(동작 변경 없음). |
| `www/ui/tell-window.js` | `createTellWindow` 가 BLANK 조기 분기·`param` 객체 등록과 반납·이름 기록·**조건 태그 평가**(이전에는 Tell 창에서 조건이 평가되지 않았다)·본문 인라인 명령 수집을 한다. 반환에 `blank`·`inlineCommands` 가 늘었다. |
| `www/assets/game-data.js` | `findTellSection`(이름 기록 + 조회, `global.priorTell`), `currentTell`, **`{@미션.키}` 적재 훅**(`missionSpec` 파일을 임시 객체에 읽음; 이전에는 훅이 없어 `{@tell.tipN}` 이 항상 비었다). |
| `www/platform/settings.js` | 설정 키 `welcomeGiven`(토글, 기본 0)·`tipNumber`(부호 있는 32비트 정수, 기본 0)와 `integer` 종류 추가. 두 값 모두 원본 `setup.cfg` 기본값 0 과 같다. |
| `www/netstorm.js` | `_runMainMenuStartup`(메인 메뉴 진입 때 환영/팁), `_openTell` 의 BLANK·인라인 명령, `_runScriptCommand`(Config·Warning), `_runTellTip`, 버튼 명령 `Config`·`TellTip` 연결. |

## 3. 시작 흐름과 원본과 다른 점

처음 실행(`welcomeGiven=0`): 메인 메뉴 위에 **Welcome**("NetStorm", OK) → OK → **Welcome2**(열리자마자 `<$config,welcomeGiven=1>` 이 실행되어 저장됨; Play Early Missions·Main Menu). 이후 실행(`welcomeGiven=1`)이고 `tellTips` 가 켜져 있으면 **Tips**("Did You Know?")가 열린다. OK 는 다음 팁 번호를 저장만 하고 닫고, Next/Prior Tip 은 같은 창을 새 번호로 다시 연다. No More Tips 는 `tellTips=0` 을 저장하고 닫는다. 팁은 Options 의 "Tell Tips at Startup" 으로 다시 켠다. 한 수명에서 메인 메뉴에 다시 들어와도 첫 메뉴 표시가 꺼져 있어 팁이 반복되지 않는다(미션에서 돌아올 때를 위한 원본 동작).

| 원본 | 웹 판본 |
|---|---|
| 팁 창 아래에 "Not Validated" 창이 겹쳐 열린다(OK 두 번). | **열지 않는다.** 프로젝트 결정(2026-09-30)이며 웹에는 업데이터가 없다. |
| `startup()`(저장 상태 `!state.dat` 질문)이 참이면 팁을 건너뛴다. | 웹에는 저장 상태 파일이 없어 질문 자체가 없다. |
| `Welcome` 의 `<background="nsisles">` 로 배경 그림이 바뀐다(추정). | 이름만 저장한다(기존 정책). **`nsisles` 그림은 원본 폴더·CD·TAFF 어디에서도 찾지 못했다**(`*isles*` 검색 0건). 자료가 없어 `titleMenu` 배경이 그대로 보인다. |
| 팁 0 의 NetstormHQ 링크(`<a href=…>`)는 눌리고 외부 사이트가 열린다. | 링크 글자 모양(청록색)만 나오고 눌러도 아무 일이 없다. Tell 창에는 링크 훅이 아직 없고(도움말 창에는 있음), 원본 주소(`netstormhq.com`)는 이미 사라졌다. 처음 설치 때 첫 팁이 0 번이라 화면에 보인다. |
| 본문 인라인 명령(`<$config,…>`)은 명령 큐에 넣어 다음 입력에서 처리한다. | 창을 올린 직후 순서대로 실행한다. Config·Warning 만 연결했고 그 밖의 명령은 안내만 한다(cpppj 도 이 둘만 연결). |
| 현재 Tell 이름은 256바이트 배열에 복사한다. | 256글자 이상은 거부한다(RangeError). |

## 4. 검증

### Node (`npm --prefix webpj test`: 77 → **87개 통과**, 신규 10)

- **원본 기계어 기대값 대조:** 스크립트 명령 토큰 128행·Config 18행·명령 인자 24행(큐에 들어간 행), Tell 지역 인자 **1,070행 전부**(이름·치환 호출·Append 줄의 순서까지), 팁 명령 **388행 전부**(번호·호출 순서·부호 있는 하한). 행 수를 고정했다(cpppj 가 이 파일을 다시 만들면 실패하며, 의도한 변경인지 확인하고 갱신한다). 기존 토큰 helper `readRawTextToken` 도 이 128행으로 처음 원본과 직접 대조했다(기존에는 조건 태그에서만 쓰였다).
- 기대값 파일이 직접 다루지 않는 경계는 따로 확인했다: 명령 이름 구분자 `#`·`;`, 인자 안의 쉼표 보존·`;` 에서 끝남(cpppj 코드·문서 근거), 입력 1024바이트 경계·NUL 뒤 무시·긴 이름이 78글자씩 이름과 인자로 나뉘는 원본 동작(고치지 않음).
- Tell 창 공장: BLANK(대소문자 무시)·`param` 객체의 수명(본문 처리 뒤 내려가고 바깥 같은 이름 객체가 다시 보임)·세 인자(공백·따옴표·설정 치환)·`global.priorTell` 의 변경 조건·실패한 이름도 현재 이름이 됨·빈 섹션·긴 이름 거부.
- 실제 자료: `{@tell.tip3}`·`{@tell.tip999|…}` 조회, Tips 본문의 번호별 문구와 Prior Tip 단추 조건(0 이면 없음, 0 이 아니면 있음, 범위 밖이면 "You're out of Tips!"), Welcome→Welcome2 의 인라인 Config 와 쓰기 통지.
- 기존 검사 두 곳을 의도에 맞게 고쳤다: `dialog.test.mjs`(`Blank` 는 이제 "없는 섹션"이 아니라 창을 열지 않는 BLANK), `popup.test.mjs`(설정 키 9 → 11개와 기본값 스냅샷, 정수 설정 검증 추가).
- **변이 검사 33종(Node 27 + 브라우저 6)이 모두 걸렸다.** 첫 Node 실행 26종 중 22종이 걸렸고 4건을 놓쳤다: 이전 팁 하한의 `<=`→`<` 는 값 0 에서 결과가 같은 동등 변이라 하한 값을 바꾸는 변이로 교체했고, 명령 이름의 `#` 구분자와 인자의 쉼표 보존은 기대값 파일이 쓰지 않아 직접 확인을 추가했으며, 한 변이는 CRLF 파일의 여러 줄 패턴이라 찾지 못해 한 줄 패턴으로 바꿨다(교체·추가 5종은 모두 걸림). 브라우저 변이는 첫 시험에서 한 건이 앞선 Edge 종료 직후의 CDP 연결 실패였고, 그 변이만 다시 돌려 실제 실패(`tellTips=0` 저장 불발)로 걸림을 확인했다. 변이는 모두 원래 내용으로 복원했다.

### 실제 브라우저 (Edge 156 헤드리스, 마우스 클릭)

1024×768 en · 1920×1080 ko · 1366×768 en · 1280×800 ko 각 **20/20**(합계 80/80, 콘솔 오류 0): 새 저장소에서 Welcome 열림 → OK → Welcome2(`welcomeGiven=1` 즉시 저장) → Main Menu 로 닫고 메인 메뉴 버튼이 눌림 → 새로고침 → Tips(번호 0, Prior Tip 없음) → Next Tip(번호 1, `tipNumber=1` 저장, Prior Tip 생김) → Prior Tip(0 으로 복귀) → Next 두 번(2) → OK(3 저장, 닫힘) → 새로고침 → 번호 3 팁 → No More Tips(`tellTips=0` 저장) → 새로고침(창 없음) → Options 의 "Tell Tips at Startup" 이 꺼져 있음 → 눌러 켬(`tellTips=1`) → 새로고침(팁 다시 열림, 번호 유지) → 같은 수명에서 메인 메뉴 재진입(팁 반복 없음). 4:3·16:9·16:10 과 한국어 로캘 모두 같은 결과이며 화면 스크린샷으로 창 모양·가운데 배치를 직접 확인했다.

### 원본 관측과의 대조

문서 기록 원본 관측 [화면 목록](screens/README.md): 1024×768 에서 팁 창 **OK 단추 클릭 지점 (661,436)**. 웹 복원의 Tips 창 배치를 팁 번호 0~40 에 대해 계산하면 OK 중심이 (661.5, 437)(번호 1·5·11·12·14·15·27·29·40, 창 425×157)·(661.5, 444)·(661.5, 451)·(661.5, 458)(창 폭 445)로 **x 는 모든 번호에서 661 이고 y 는 가장 가까운 437 이 관측 436 과 1픽셀 차**다. 그 관측 때의 팁 번호는 기록되어 있지 않아(같은 문서군의 UI 녹화 기록에는 팁 15 가 나오며 15 의 OK 중심이 437 이다) 번호별 일치는 주장하지 않는다. 팁 창의 원본 스크린샷은 없어 픽셀 대조는 못 했다.

### 확인하지 않은 것

Chrome/Firefox·터치·소리 출력·팁 창이 열린 채 Esc(원본은 안내 창에서 Esc 무반응 — 웹 Tell 창도 Esc 를 받지 않지만 이번에 따로 시험하지 않음)·`storage` 이벤트로 다른 탭 갱신·`.korean` 자료(없음).

## 5. 새로 확인한 사실

- `setup.cfg` 의 `tipNumber` 기본값이 **0** 이라 처음 설치한 상태의 첫 팁은 tip0(NetstormHQ 방문 안내)이다. 팁은 `tip0`~`tip39` 40개이며 OK 를 누를 때마다 다음 번호가 저장된다.
- 본문 끝 섹션이 헤더만 있고 뒤에 아무것도 없으면(`[Empty]` 로 파일이 끝남) 원본 규칙대로 섹션이 없는 것으로 조회된다(뒤에 줄이 하나 있어야 존재).
- `$Button=라벨,명령,인자` 의 인자는 공백·쉼표에서 끝난다(원본 토큰 규칙). 인자에 공백이 필요하면 따옴표를 쓴다.

## 6. 남은 일

[LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 맨 위 '다음(이어서)': Tell 창 링크 훅·스크롤바, Auto-Demo, 한국어(D2Coding·`.korean`)·`<background>` 그림, Options −40 규칙, 도움말 후속, 그 뒤 정적 맵 화면(M2). 환영 창의 "Play Early Missions"(`MissionBegin,Tutorial1`)는 미션이 구현될 때 연결한다(지금은 창을 닫고 안내만 남는다).
