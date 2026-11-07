# webpj 대본 일반 입력 분배(DestroyAt·AddRandom·MakeAi) 이식 — WB04-E21

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 스물한 번째 부분이다. cpppj 기준선 `1f00a25a`의 `client/DestroyAtInput`·`client/AddRandomInput`·`client/MakeAiCommandInput`(10.78)을 한 모듈(`www/core/script-command-inputs.js`)로 옮겼다. 셋 모두 Guide/Gump 뒤의 일반 입력 하나를 받아 소비 여부를 돌려주는 분배기이며 각각 독립 원본 관측이 있다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다(Node 단위 검사만). 기준선 뒤 cpppj 변경은 없다.

## 1. 옮긴 것

수정키를 제거한(`0x03ffffff`) 코드가 자기 명령 번호가 아니면 false, 키 뗌이면 효과 없이 true다. 명령 번호는 원본 전역의 초기 토큰이다.

| 함수 | 원문 / 명령 | 내용 |
| --- | --- | --- |
| `dispatchDestroyAtInput(event, editor, mission, destroyAt)` | 004d8528..004d8551 / `0x020000e1` | Edit/미션 중 누름만 y의 32비트 전체(문자열 토큰)를 부모에 전달, 일반 모드는 소비만. 현재 선택이나 메뉴 열기 조건을 추가하지 않는다. |
| `dispatchAddRandomInput(event, editor, mission, lookup, add)` | 4d7c33..4d7c6c / `0x02000098` | 누름이고 Edit/미션 중이면 y를 타입으로 쓰되 **signed** y가 255를 넘으면 문자열 타입 조회(`lookup`의 signed 반환 −1/0도 DWORD로 배치 부모에 전달) 후 `add(타입, 1)`. |
| `dispatchMakeAiCommandInput(event, hooks)` | / `0x020000b7` | 누름이면 `editor()`가 비0이거나(단락; 이때 `missionPost`는 읽지 않음) `missionPost()`가 비0일 때 y 전체 DWORD로 `makeAi(owner)`. 자체 battle 관문과 AI 수명은 부모가 유지한다. |
| `makeAiCommandScriptCode(name)`, `isMakeAiCommand(code)` | 이름표 | 첫 NUL 앞 이름의 ASCII 대소문자 무시 비교로 명령 번호(없으면 `0xffffffff`). |

선택된 하위 장치의 누락은 그 위치에서 거부하며 도달하지 않은 장치는 요구하지 않는다.

## 2. 검증 (`tests/script-command-inputs.test.mjs`, 6건)

| 검사 | 근거 |
| --- | --- |
| DestroyAt 48행(모드·뗌·전체 y 전달) + 정확한 코드/도달한 장치만 요구 | `destroy-at-input-x86.tsv`(머리글 제외) |
| AddRandom 288행(signed 255 비교·모드·뗌·조회 결과 전달) + 정확한 코드/장치 | `add-random-input-x86.tsv`(머리글 제외) |
| MakeAi 64행(Editor OR missionPost 관문·뗌·전체 DWORD 인자) + 단락·실패 전파·이름표 | `make-ai-input-x86.tsv` |

최초 실패와 교정: 6건이 처음부터 통과했다. 기대값은 원문 열만 썼다.

## 3. 경계

- cpppj의 스크립트 큐(`QueueScriptCommand`)·`GameMenuScriptCode`·`ScriptStringRing` 이름 해석(`make-ai-names`·`create-at-names`, `DestroyAt_Native48`의 큐 연결 부분)과 실제 부모(`DestroyObjectsAt` 804행, `AddRandomObjects` 720행)는 해당 단위를 옮길 때 대조한다.
- `CreateAtInput`·`DetonateInput`은 Guide/Gump 체인 trace와 팝업 경계 관측이 함께 있어 별도 단위로 남겼다.

## 4. 다음 시작점

`RawGraph`/`RawGraphLive`, 공통 `SquidPostPop` 본체, `RawSquidPoolReset`, `RawBridgeConnect`, 그리고 Player 나머지(`RawPlayerWorld`·`RawPlayerTechnology`·`RawPlayerRegular`·`RawPlayerTemplateState` 등).
