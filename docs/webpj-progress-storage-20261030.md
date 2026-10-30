# webpj 캠페인 진행 값(`Done*`) 쓰기·저장과 설정 쓰기 층 (WB03-H, 2026-10-30)

2026-10-30, **HJOW-Athlon**. 사용자 요청("AGENTS.md·LEFT_JOBS.webpj.md 를 읽고 webpj 다음 작업 진행, 다른 세션에서 cpppj 진행 중이니 cpppj 파일 수정 자제")에 따라 [직전 단위(WB03-G)](webpj-help-window-20261030.md)의 '다음' 첫 항목인 **진행 값(`Done*`) 쓰기·저장과 `GameData` 설정 층의 쓰기(`Set`)** 를 했다. 지금까지 웹 판본은 진행 값을 읽기만 했고(캠페인 목록의 체크 표시·잠금), 값을 쓰는 곳도 저장하는 곳도 없었다. 미션 플레이(WB05 이후)가 생기면 미션 스크립트의 `<$Config,Done{mission.fileName}=1>` 효과가 바로 이 경로로 들어온다.

`cpppj/`·`dotnetpj/`·`originals/`·`originalCD/`·AGENTS.md·공통 `LEFT_JOBS.md` 는 수정하지 않았다(cpppj 소스·fixture·문서는 읽기만 함, 같은 저장소에서 다른 세션이 cpppj 를 작업 중이라 내 파일만 커밋). 원본 게임·analyzeManager 는 실행하지 않았고 브라우저 검사는 Edge 헤드리스(창 없음)다. 마지막 전체 정밀 디컴파일 호스트(HJOW-Athlon / 2026-10-23)와 현재 PC 가 같아 디컴파일 소스(`extracted/refined/originals/Netstorm.c`)를 읽었다(저장 호출 지점 `00441d10`·`00441de0`).

## 1. 근거: 원본이 하는 일

| 항목 | 원본 동작 | 근거 |
|---|---|---|
| 값 쓰기 `Config::Set` | 키에 `.` 이 있으면 마지막에 등록한 객체부터 접두어가 맞는 첫 객체를 대상으로 삼고 키를 첫 `.` 뒤로 바꾼다(이름 없는 객체는 길이 0 비교라 항상 맞는다). 대상에서 **같은 키의 줄을 모두 지운 뒤** `키 = "값"` 줄을 버퍼 끝에 덧붙이고 대상의 changed 표시를 켠다. 줄을 지웠으면(이미 있던 키) 참을 돌려준다. | `004404e0`, cpppj `Config::Set`, [설정 복원 문서](exe/cpp-config-reconstruction.md) |
| 줄 삭제 `RemoveKey` | 찾을 때마다 그 줄(키 위치부터 다음 줄 첫 글자 앞까지)을 지운다. 다음 줄 첫 글자는 빈 줄·들여쓰기를 건너뛴 위치다. | `00440240` |
| 저장 시점 | 메인 루프가 changed 표시를 보고 저장한다(`00441de0` → `00440f30`). 시작·옵션 변경 뒤 저장 경로 `00441d10` 도 있다. 저장 내용은 `[options.cfg]` 섹션 본문 + `[END]` 섹션 본문이다. | 디컴파일 `00441de0`·`00441d10`, [캠페인 진행 문서](exe/cpp-campaign-progress-20261030.md) |
| 완료 기록 | 미션 스크립트의 `<$Config,Done{mission.fileName}=1>`. 목록의 활성 조건은 `{Done<앞 미션>}` 이다. | [캠페인 진행 문서](exe/cpp-campaign-progress-20261030.md), [미션 스크립트](formats/mission-script.md) |
| 키 모양 | 메뉴 자료가 참조하는 키: `DoneTutorial1..6`, `DoneTheWarBegins` 같은 영문 이름, `Donebc3acid_bar76` 같은 숫자·밑줄 포함 이름. 대소문자는 무시된다(저장 파일 `DoneThewarbegins`, 목록 `{DoneTheWarBegins}`). | 웹 메뉴 자료(`offical*.english`) 전수 조사 |

## 2. 구현

| 파일 | 내용 |
|---|---|
| `www/assets/config.js` | `Config.removeKey`·`Config.set`(위 원본 규칙)과 `changed` 표시를 추가했다. 읽기 전용이던 설정 계층이 쓰기까지 갖는다. |
| `www/assets/game-data.js` | `GameData.set`(전역 설정에 쓰고 `onChange(키, 문자열 값)` 훅 호출, `silent` 이면 훅 생략)·`setInt`(정수만), 생성 옵션 `onChange`. `findFiles`(아래 §5). |
| `www/platform/progress.js` (새 파일) | `Progress`: 진행 값을 버전이 있는 JSON(`netstorm.progress`)으로 로컬 저장소에 보관한다. 키는 `Done`+영숫자·밑줄(최대 80자), 값은 십진 정수 문자열이며 원본 `ConfigParseLong` 과 같은 32비트 정수로 정리한다. 키는 대소문자 무시, 적은 대로의 키를 보존한다. 같은 값은 쓰지 않는다. 저장은 **다른 탭이 먼저 저장한 값을 다시 읽어 합쳐서** 쓴다(진행 값은 늘기만 하므로 안전). 손상·다른 버전·잘못된 항목·저장소 오류는 진행 값 없이(또는 그 항목만 버리고) 계속한다. |
| `www/platform/settings.js` | `findSettingKey`(설정 키를 대소문자 무시로 찾기) 추가. |
| `www/netstorm.js` | `Progress` 를 설정과 같은 저장소로 만들고, 게임 데이터 생성 때 저장된 진행 값을 `[options.cfg]` 자리에 넣고 `onChange` 를 `_onConfigWrite` 에 연결한다. `_onConfigWrite`: 진행 키 → 진행 저장소, 설정 키 → 설정 저장소, 그 밖의 값(`global.` 실행 중 값 등)은 저장하지 않는다. `_onSettingChange`: 설정 변경을 게임 데이터에도 반영(되돌아오는 통지 없음). `_syncGameData`: 재시작(init)에서 저장소를 새로 읽을 때 기존 게임 데이터에 맞춤. `get progress`. |

흐름: `gameData.set('DoneTutorial1', 1)` → 전역 설정 버퍼에 반영(즉시 조회 가능) → `_onConfigWrite` → `Progress.set` → 로컬 저장소에 즉시 저장. 다음 실행에서는 저장된 값이 첫 적재 때 `[options.cfg]` 자리에 들어간다.

## 3. 원본·cpppj 와 다른 점(웹 적응)

- **저장 형식·시점:** 원본은 options.cfg 텍스트 파일을 메인 루프의 changed 표시로 저장한다. 웹은 진행 값을 별도 JSON 으로, **값이 바뀔 때마다 즉시** 저장한다(탭이 언제 닫힐지 알 수 없어 종료 시점 저장에 기대지 않는다). 원본에서 `[END]` 뒤에 붙는 실행 중 값 전부를 저장하는 것도 아니고, 진행 값(`Done*`)과 설정 9개만 저장한다.
- **설정과 진행의 저장 항목 분리:** 설정은 고정 9개 키를 엄격히 검증하고, 진행 값은 미션 파일 이름에서 오는 열린 키 집합이라 규칙이 다르다. 한쪽이 깨져도 다른 쪽이 남도록 `netstorm.settings`·`netstorm.progress` 로 나눴다.
- **값 검증:** 원본 `Set` 은 값을 검증하지 않는다. 게임 데이터 층(`GameData.set`)은 원본처럼 검증하지 않고, 저장소로 넘어가는 시점(`Progress`·`Settings`)에서만 검증한다. 따라서 `gameData.set('DoneX', 'abc')` 는 메모리 설정 층에는 쓰이지만 저장되지 않는다.
- **`changed` 표시:** `Config` 에는 원본처럼 두었지만 웹은 이 표시로 저장하지 않고 훅으로 저장한다.
- **초기화 방법 없음:** 진행을 지우는 화면은 없다(원본도 options.cfg 를 지워야 한다). 개발용으로 `window.netstorm.progress.clear()` 가 저장 항목까지 지운다.

## 4. 검증 (HJOW-Athlon, Node v24.21.0, Edge 156.0.4314.50 헤드리스)

- `npm --prefix webpj test` **74개 통과**(기존 62 + 신규 12). 
  - **원본 x86 대조:** `config.test.mjs` 가 cpppj fixture `config-x86.tsv` 의 `Remove` 272행·`Set` 143행(대상 객체·접두어 떼기·줄 삭제 뒤 버퍼·changed 대상)을 추가로 대조한다. 합계 2,440행이 원본 기계어 결과와 같다. 행 수를 고정해 두어 cpppj 쪽이 fixture 를 다시 만들면 이 검사가 실패한다(그때 의도된 변경인지 확인하고 `config.test.mjs` 의 `EXPECTED_ROWS` 를 갱신).
  - **신규 `progress.test.mjs`:** 키·값 검증, 대소문자·저장 형식·같은 값 무시·다시 읽기, 저장소 없음/읽기·쓰기 오류/손상/버전/잘못된 항목, 다른 탭 값 합치기, `clear`, `GameData.set`(줄 하나만 남음·버퍼 끝 덧붙임·통지·`silent`·훅 예외·정수 검사·`global.` 접두어), **진행 값 쓰기 → 캠페인 목록 체크 표시/잠금 해제 → 저장 → 새로 읽어도 같은 상태**, 한국어 Campaign 목록, 앱 연결(진행→진행 저장소, 설정 키→설정 저장소, 설정 변경→게임 데이터, 실행 중 값 미저장, 새로고침·재시작 동기화).
- **변이 검사 20종:** 줄 삭제 첫 줄만·기존 줄 미삭제·접두어 객체 미선택·접두어 미제거·changed 미설정(5), 다른 탭 합치기 제거·대소문자 구분·같은 값 재저장·설정 키 허용·값 정리 제거·버전 확인 제거·잘못된 항목 미폐기(7), `GameData.set` 통지 제거·`silent` 무시(2), 앱의 진행 저장 연결 제거·설정 저장 연결 제거·설정→게임 데이터 반영 제거·재시작 동기화에서 진행 값 누락(4), 한국어 파일 목록 대체 제거(1), 첫 적재가 저장된 진행 값을 안 넣음(1, 브라우저 검사가 잡음: D3·E1 실패)이 **모두 검사에 걸렸다.** 첫 시험에서 놓친 변이는 없었다(바꿀 문자열이 맞지 않아 건너뛴 1건은 다시 시험해 걸림을 확인).
- **실제 브라우저(마우스 클릭, CDP):** 1024×768·1920×1080 각 19/19 + 저장소 접근 차단 5/5 = **43/43**, 영어 로캘(`?lang=en`) 19/19. 확인한 것: 새 프로필에서 진행 없음·첫 행만 활성 → `gameData.set('DoneTutorial1', 1)` 가 **즉시** 로컬 저장소에 저장 → Campaign→Early Missions 를 실제 클릭해 1번 체크·2번 해제 → 설정↔게임 데이터 동기화(양방향) → 실행 중 값 미저장 → **새로고침 후 진행 값·설정·체크 표시 유지** → 여러 미션 완료와 Struggle For Freedom 해제 → `destroy`/`init` 재시작에서 다른 탭이 저장한 값 반영 → 콘솔 오류 없음(favicon 404 제외). 저장소 접근이 예외를 던지는 환경에서 진행 값 없이 시작해 메모리에서 쓰기·목록 반영이 되고 예외가 없다.
- **확인하지 않은 것:** Chrome/Firefox·터치·실제 소리 출력, 실제 `storage` 이벤트로 열린 다른 탭의 화면 갱신(다른 탭의 값은 다음 시작/재시작에 반영되며 저장 시 합쳐질 뿐이다), 저장소 용량 초과 같은 실제 환경 오류(가짜 저장소로만 시험). 실제 미션 승리로 `Done*` 이 쓰이는 경로는 미션 플레이(WB05~)가 생긴 뒤 확인한다 — 지금은 개발 콘솔의 `gameData.set` 이 같은 경로다.

## 5. 함께 고친 결함: 한국어 브라우저에서 Campaign 목록이 비어 있었다

브라우저 검사 중 발견했다. 이 PC 의 Edge 언어가 `ko` 라 앱이 언어를 `korean` 으로 정했고, 캠페인 목록의 패턴 `\D\offical*.korean` 에 맞는 파일이 없어 **Campaign 을 누르면 Back 버튼만 있는 빈 대화상자**가 열렸다. 그동안 검사는 영어 기준이었다. 다른 언어 파일은 이미 영어로 대체했지만(`tell.korean`·`config.korean`) 파일 목록 패턴에는 대체가 없었다.

`GameData.findFiles(pattern)` 를 추가해 현재 언어의 파일이 하나도 없으면 패턴 끝의 언어 이름을 영어로 바꿔 한 번 더 찾게 했고, `tell-lists.js` 의 `expandFileList` 가 이를 쓴다. 영어 데이터는 그대로다. 원본은 대체하지 않는 웹 적응이며(`.korean` 자료가 갖춰지면 자연히 한국어 파일이 먼저 쓰인다), `data.test.mjs`·`progress.test.mjs` 가 검사한다(대체를 빼면 `progress.test.mjs` 의 한국어 시험이 실패함을 확인).

## 6. 남은 일

- 미션 플레이(WB05~)가 `<$Config,Done…=1>` 을 실제로 쓰게 되면 이 경로를 그대로 쓴다. 승리 시점에 쓰는지는 그때 자연 플레이로 확인한다.
- Tell 지역 인자 `Name(arg1,arg2)`·현재 미션 조회, 시작 팁 창·Auto-Demo, 한국어(D2Coding·`.korean`)·`<background>` 그림, `User Made Campaigns` 항목 처리, Options −40 규칙·와이드 화면 위치(원본 근거 필요), 도움말 후속 항목은 [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 맨 위 '다음'에 이어진다.
- 재시작(`destroy`→`init`) 중 저장소를 쓸 수 없는 환경에서는 새 `Progress` 가 비어 시작하지만 기존 게임 데이터는 이전 수명의 진행 값을 그대로 가진다(저장소를 쓸 수 없고 같은 페이지에서 재시작하는 드문 경우만 해당, 호스트가 새로 불러오면 사라진다).
