# Reset All 확인을 게임 안 Tell 창으로 교체 (WB04-E1251)

[다음 작업 1 ⑧](../LEFT_JOBS.webpj.md#next)을 사용자 결정(2026-11-23 "바꿔 주세요")에 따라 구현했다. E863([docs/webpj-reset-all-20261119.md](webpj-reset-all-20261119.md))의 브라우저 `confirm()` 확인을 cpppj 1325 와 같은 **게임 안 Tell 확인 창**으로 바꿨다. 저장 범위(게임 소유 설정·진행 키만 지움)·앱 정리 뒤 다시 로딩·실패 처리·다른 앱 저장값 보존은 그대로이다.

## 원본(cpppj 1325) 흐름과 웹 대응

| 단계 | cpppj | 웹 |
| --- | --- | --- |
| 옵션 메뉴 최하단 행 | `UberGump.cpp:1431` `add("Reset All","Tell","Patch1101.ResetAll")` | `ui/option-menus.js` 가 `text('Reset All', 'Tell', 'Patch1101.ResetAll')` (이전: 명령 `ResetAll`) |
| 확인 창 | `ProductPresentation.cpp` 의 `[ResetAll]` 섹션: 제목 `Reset All`, 본문 `Reset all settings and campaign progress?<br>The game will close after resetting.`, 단추 `Reset All`(→`Tell,Patch1101.ResetAllConfirmed`)·`Cancel`(→`Tell,Blank`). 한국어·일본어는 `ProductTellBody` 가 고정 본문으로 바꿈 | `assets/product.js` 의 `RESET_ALL_SECTIONS`(영어·한국어)를 제품 설정 객체 `Patch1101` 에 `productCreditsText` 로 덧붙임. 영어 마지막 줄만 웹 동작에 맞게 `The game will reload after resetting.`(종료가 아니라 다시 불러옴, AGENTS.md 15차 웹 항목). 한국어 `설정 초기화`·`초기화`·`취소` |
| 확정 | `UberGump.cpp:1566~1573`: `Patch1101.ResetAllConfirmed` 는 **그 단추가 낸 제출 토큰(`resetAllSubmission_`)일 때만 한 번** `client_.ResetAllSettingsAndClose()` | `netstorm.js` `_openTell` 이 확정 대상을 받아 **확인 창이 열린 직후(`_resetAllArmed`)에만 한 번** `_resetAll()`(저장 삭제 → `destroy()` → `location.reload()`)을 실행. 확인 창 밖에서 온 같은 이름과 두 번째 확정, 그 사이에 다른 Tell(취소의 `Blank` 포함)이 온 경우는 무시하고 경고만 남김 |
| 실패 | 예외 | 저장소 접근이 막히면 `view.alert()`로 안내하고 앱을 유지(이전과 같음, `resetFailed` 문구 유지) |

- `texts.resetConfirm`(브라우저 `confirm()` 문구, 영어·한국어)은 확인 창 섹션으로 옮겨 제거했고, `_onPopupCommand` 의 직접 `ResetAll` 분기도 없앴다. 이전 흐름에서 `confirm()` 이 필요하던 곳은 없다.
- 한국어 옵션 메뉴 행 이름은 기존 번역표의 `Reset All → 모두 초기화`(웹)이고 창 제목은 cpppj 와 같은 `설정 초기화`이다(두 표의 행 이름이 다른 것은 기존 차이).
- 일본어 본문은 일본어 지원(13차) 때 같은 구성으로 더한다. 게임 안 ESC 메뉴 Options 의 Reset All 행(cpppj `RawGameMenus.cpp:160`)은 AGENTS.md 15차가 **메인 메뉴** Options 만 요구해 이번에도 더하지 않았다.

## 시험

- `tests/reset-all.test.mjs` 8건(Node): 메뉴 최하단 행이 Tell `Patch1101.ResetAll` 을 열고 원본 비교 메뉴에는 없음, `[ResetAll]` 섹션(영어·한국어 본문과 확정·취소 단추 대상), 저장 범위·부분 삭제 복구(E863 그대로), **확인 창 없이 온 확정은 무시**, 무장 뒤 확정은 삭제→정리→새로고침 순서이고 한 번만 효력, 다른 Tell 이 오면 무장이 풀림, 저장소 차단 시 안내. 가짜 view 의 `confirm()` 은 호출되면 시험이 실패한다. 확정 가드를 끄는 변형으로 세 시험이 실패함을 확인하고 복원했다.
- `e2e/reset-all.spec.js`(실제 Chromium): 영어 — Options → Reset All 이 확인 창을 열고 단추가 `['Reset All','Cancel']`, 무장 표시, Cancel 은 설정·진행·무장을 그대로 두고, 창 밖 확정은 무시, 다시 열어 `Reset All` 단추를 누르면 로드 이벤트와 함께 두 저장 항목만 지워지고(다른 앱 값 보존) 최초 실행 Welcome Tell 이 나온다. **브라우저 대화상자(confirm/alert)는 한 번도 뜨지 않음**을 확인한다. 한국어 — 옵션 → 모두 초기화가 `설정 초기화` 창을 `['초기화','취소']` 단추로 열고 취소는 아무것도 지우지 않는다(스크린샷에서 갈무리11로 그려짐).
- 관련 Node 85건(product·tell·option·skirmish·difficulty·main-menu 등)과 옵션 메뉴를 거치는 e2e 9건(first-run·language-choice·difficulty·frame-rate·korean-font) 통과.

## 남은 일

- 실제 원본 화면과의 외관 대조는 하지 않았다(확인 창은 기존 Tell 창과 같은 틀).
- 일본어 본문과 게임 안 ESC 메뉴 행은 위 이유로 남긴다.
