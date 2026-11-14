# webpj 점 명령·HTML 조건/입력 원본 관측 연결 (WB04-E478, 2026-11-15)

3.8절 이월 목록의 "점 명령/HTML 나머지"를 원본 10.78 독립 관측 fixture 다섯 개 **1,352행**에 대조했다. 네 묶음은 첫 실행에서 전체 일치했고, `htmlcondition`이 **이식 결함 하나**를 찾아냈다(숨김 깊이의 uint32 감김). 결함을 고친 뒤 전체 일치했다.

| fixture | 행 | 대상 | 비교한 것 |
| --- | --- | --- | --- |
| `dot-command-dispatch` | 512 | `dispatchDotCommand` | 독립 1,024관측(두 정밀도)의 명령표 2568바이트·입력·기본 슬롯/구분자 위치·외부 호출 순서(중복 이름·31바이트 이름·호출 중 표/입력/상태 변경) |
| `dot-command-register` | 250 | `registerDotCommand` | count 63 끝쓰기와 복사 뒤 count 변경·복사 중단의 인접 전체 표·반환·복사 인자 |
| `dot-command-register-v2` | 12 | 같은 함수 | count 64 정상과 count 65 종단 실패 전 전체 표(물리 밖 store 거부), 음수 count 의 CRT 인자 경계 4행 |
| `htmlcondition` | 489(10.78) | `applyHtmlCondition` + `parseHtml` | 조건 태그/전체 순회의 출력·입력 위치·표시/숨김 깊이·상태 비트·분리된 Config 치환 호출 순서. 같은 fixture 의 originalCD(10.72) 265행은 웹 범위 밖이라 제외 |
| `html-input-dialog` | 89(확인) | `confirmHtmlInput` | 복사·강제 NUL·첫 LF·종료 NUL 부터의 공백 검사·atol·서식/Tell 호출 순서와 반환. 생성/공장 20행은 `SaveFortNameDialog` 이식 때 |

## 이식 결함 교정

- `applyHtmlCondition`(`www/ui/html-text.js`): 거짓 조건이 숨김 깊이를 올릴 때 `state.hiddenDepth += 1`로 JS 수 연산을 써서 깊이가 `0xffffffff`일 때 `4294967296`이 되었다. 원문은 DWORD 증가이므로 0으로 감겨야 하며 원본 관측 10행(깊이 `0xffffffff`에서의 거짓 조건)이 정확히 이 차이로 어긋났다. `(state.hiddenDepth + 1) >>> 0`으로 고쳤다. 이 값은 실제 게임에서는 거의 도달하지 않지만 원문 의미를 지킨다. 같은 입력의 독립 회귀 시험(`tests/command-native-fixtures.test.mjs`)도 추가했다.

## 한 일

- `tests/command-native-fixtures.test.mjs` 신설: cpppj `DotCommandDispatchTests`·`DotCommandRegisterTests`(보충 v2 포함)·`HtmlConditionEffectTests`·`HtmlInputDialogTests`의 입력 구성(앞 열만 읽음)과 직렬화를 그대로 옮겼다.
- manifest에 다섯 항목(`unit WB04-E478`)을 연결했고 PORT_MAP 네 행(`RawDotCommandDispatch`·`RawDotCommandRegister`·`HtmlConditionEffect`(partial 유지)·`HtmlInputDialog`) 메모에 연결 사실과 결함 교정을 덧붙였으며 해당 모듈 세 곳과 `html-input-dialog` 시험의 "cpppj에도 직접 시험이 없다"는 오기를 정정했다.

## 남은 것

`dot-command-static-image.hex`(`ClientDotCommandGlobals`)와 3.8절 나머지 항목은 후속 단위다. `HtmlConditionEffect`는 PORT_MAP에서 partial(조건 효과 본체는 이식, 일부 binder 미이식)로 남아 있다.
