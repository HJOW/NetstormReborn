# webpj 언어 변경 요청·창 제목·게임 시간 표시 이식 — WB04-E34

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 서른네 번째 부분이다. cpppj 기준선 `1f00a25a`의 `client/LanguageRequest`(+`LanguageOptions`의 이름표)·`client/WindowTitle`·`client/GameTimeDisplay`(10.78)를 옮겼다. 모두 훅 스타일(도달한 장치만 요구, 앞선 부분 효과는 되돌리지 않음)이다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다(Node 단위 검사만). 기준선 뒤 cpppj 변경은 없다.

## 1. 옮긴 것

| 모듈 | 원문 | 내용 |
| --- | --- | --- |
| `www/core/language-request.js` | 004dea70, 542988/54298c | `requestOriginalLanguage(requested, current, refresh, hooks)`: 요청이 현재 번호(DWORD 비트)와 같으면 아무것도 읽거나 바꾸지 않음. 다르면 `set('currentLanguage', 언어 이름)`(동기 Config 변경이 현재 번호/용어표를 갱신) → 복귀 뒤 **현재** `LanguageChanged` 명령을 `changed()`로 읽어 `queue(명령,0,0)` → `refresh.value`를 1 올림(DWORD 감김). 예외는 앞선 효과를 되돌리지 않고 뒤 카운터를 올리지 않음. `dispatchSetLanguage`(뗌이 아니면 두 번째 DWORD 전달, 첫 DWORD는 원문 부모가 쓰지 않음), `dispatchLanguageChanged`(이미 원문 Gump 경로로 전달했으면 중복 없이 소비), `originalLanguageName`(원문 순서 `english,english,french,german,spanish,japanese,portuguese` + 클론 추가 `korean`) |
| `www/core/window-title.js` | 00460730 | `refreshWindowTitle(state, hooks)`: 요새 이름이 null/빈 문자열이면 `blankWindowName`, 편집 `EditorWindowName`, 일반 요새는 Template +0/+8이 모두 비영일 때만 `readonly` 접두 + `windowName`, 미션은 demo > testBattle > 일반 키로 설정 치환보다 **먼저** 미션 title을 읽어 조합. null과 빈 문자열 인자를 구분. 256바이트 이상/NUL 포함은 `RangeError`. 최종 제목은 `setTitle`로 한 번 적용 |
| `www/core/game-time-display.js` | 004d9862, 004a68b0 | `dispatchGameTimeKey`(T/t 뗌 제외, 정지가 아니면 표시 DWORD의 비영 전체를 0, 0을 1로; 수정키 허용; 정지 조회 누락 거부), `readGameTimeSeconds`(clock 호출 **뒤에** 원점을 읽고 `stamp`를 쓴 다음 감긴 signed 차이를 1000으로 0 쪽 절삭, `-0` 방지) |

## 2. 검증 (`tests/language-title-time.test.mjs`, 6건)

원본 10.78 관측(`cpppj/tests/fixtures`를 직접 읽음) 1081행:

- `language-request-x86.tsv` 337행: 설정 콜백 효과(`mutation` 1: 현재 번호/카운터/명령이 콜백에서 바뀜, 2: 큐 안에서 카운터가 바뀜) 뒤의 최종 전역·호출 순서·실제 큐 writer 12바이트.
- `window-title-x86.tsv` 중 10.78(`originals`) 576행(원본 `originalCD` 576행은 범위 밖): 편집/요새/미션 분기와 readonly·NULL/빈 인자·설정 조회 순서·미션 title 조회 시점·최종 문자열.
- `game-time-key-x86.tsv` 168행(뗌·모달·정지·수정키·DWORD 토글, 선행 모달은 호출 거부로 구별).

계약 검사: 같은 번호 자연 반환, 설정/큐 중단 뒤 앞선 효과 보존, 입력 분배의 수정키 허용·뗌 소비·알림 중복 방지, 이름표 표 밖 거부, 창 제목 소비자 누락·256바이트·NUL 거부, clock 예외 뒤 stamp/origin 보존과 `-0` 없는 0 쪽 절삭.

최초 실패와 교정: 6건이 처음부터 통과했다.

## 3. 경계

- 전투 시간 메시지 90행(`game-time-message-x86.tsv`: MessageGump 192바이트·서식/기한/Idle/측정 순서·음수 IDIV)은 `MessageGump` 단위에서 대조한다. 여기서는 키와 초 계산만 옮겼다.
- 실제 Config 변경 알림(`currentLanguage` 용어표 적재)·Chat 재부착·설정 치환(`pathSpec`)·Win32/HTML 창 적용(`setTitle`)은 훅으로만 주입했다. `WindowTitle`은 웹에서 `document.title` 같은 소비자로 대체되므로 분류 접두가 `[adapt-screen]`이다(`PORT_MAP`). CD 10.72는 범위 밖이다.
- `ChangePal`류 설정 변경 알림과 `LanguageOptions`의 `DetectOriginalLanguage`/`FindOriginalLanguage`/`ApplyOriginalLanguageOption`은 언어 옵션 단위에서 옮긴다(이 단위는 이름표만 사용).

## 4. 다음 시작점

`HtmlInputDialog`(109행, 대화창 gump 배치)·`DialogCommandLifetime`(258행, `DialogExitCommand` 상태)·`LanguageOptions`, `ProductionShortcuts`·`RitualChoiceInput`·`ThemeTechnologyCommand`·`RawSalvageCommand`. 큰 묶음은 `RawGraph`/`RawGraphLive`, `SquidPostPop` 처리기 본체, `RawBridgeConnect`, Player 나머지이다.
