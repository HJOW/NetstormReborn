# webpj 선택 키 부모·AI/간헐천 일반 명령·편집 객체 취득 이식 — WB04-E23

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 스물세 번째 부분이다. cpppj 기준선 `1f00a25a`의 `client/SelectionKeyInput`·`client/AiUtilityCommandInput`·`client/CursorObjectPickup`(10.78)을 한 모듈(`www/core/client-input-commands.js`)로 옮겼다. 세 단위 모두 일반 입력(또는 키 한 번)이 하위 몸체에 미치는 순서를 다루며 각각 독립 원본 관측이 있다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다(Node 단위 검사만). 기준선 뒤 cpppj 변경은 없다.

## 1. 옮긴 것

- **`dispatchSelectionKeyInput(event, hooks)`** (004d96b6 정지 관문 이후): 뗌이거나 확인한 키(H/N/P/R/F4/F5/F6)가 아니면 false. 정지면 소비. F6(`0x750000`)는 Shift/Control이 있으면 소비만, 아니면 지식 명령(`0x00542974`)을 `(명령,0,0)`으로 예약. N 또는 Ctrl+F5(`0x740000`)는 원문 Shift 비트 전체를 기존 다음 유닛 몸체에 전달. P/R는 사제 몸체. H와 F4(`0x730000`)는 F4가 Shift/Control이면 소비만, 아니면 Home 명령(`0x005429ac`)을 `(명령,0,0)`으로 예약. 그 밖(F5)은 현재 localOwner로 사제 보기 명령(`0x005429d4`)을 `(명령,owner,owner)`로 예약. Alt는 별도 거부 조건이 아니다.
- **`dispatchAiUtilityCommandInput` / `aiUtilityCommandScriptCode` / `isAiUtilityCommand`**: 수정키를 제거한 코드가 CreateInitialGeysers(`0x020000ad`)·BrainCrankMoney(`0x020000c1`)·AiParamSet(`0x02000078`)일 때만 소비, 뗌은 효과 없이 소비. 각각 전체 GenerateWorldGeysers, 현재 Player1..8 brain 직접 쓰기, y 전체 DWORD를 `parameter`에 전달한다. 모드/정지/owner를 새 관문으로 읽지 않는다. 이름은 ASCII 대소문자를 무시한다.
- **`pickupObjectForCursor(pool, source, flags, hooks)`** (00447340): 10.78 물리 SID 범위 확인 → `unpop(source,0)` → 그 뒤 현재 슬롯의 TYPE BYTE(+10)와 argument WORD(+12)를 다시 읽어 `create(sid,type,argument,0x80,0x10,0)`(반환 0도 그대로) → `take(form, flags|0x100)`. Unpop이 슬롯을 바꾸면 바뀐 값을 쓰고 앞선 효과는 되돌리지 않는다.

## 2. 검증 (`tests/client-input-commands.test.mjs`, 6건)

| 검사 | 근거 |
| --- | --- |
| 선택 키 480행(정지·Shift/Control/Alt·대소문자·F4/F5/F6 큐 예약 인자) + 뗌/명령/마우스 오인 금지, 정지 소비 | `selection-key-input-x86.tsv` |
| AI 일반 명령 72행(선택된 부모·param2 전체 DWORD·수정키/outside/뗌) + 누락/writer 실패/이름표 | `ai-utility-commands-x86.tsv` |
| 편집 객체 취득 96행(TYPE/WORD/flags/Unpop 변경/생성 0) + 도달한 하위 누락 시 선행 Unpop 보존 | `cursor-object-pickup-x86.tsv` |

선택 키 시험 장면의 메뉴 명령 표는 cpppj `GameMenuCommands`의 세 항목(`0x00542974→0x020000a0`, `0x005429ac→0x020000ae`, `0x005429d4→0x020000b8`)만 둔다. 최초 실패와 교정: 6건이 처음부터 통과했다.

## 3. 경계

- 이름→명령 해석(`ai-utility-names` 30행·`QueueScriptCommand`)과 전체 `GameMenuCommands` 표, 실제 `RawUserSelection`의 `Select`/`ToggleMultiple`, 세계 간헐천 생성·brain 쓰기·`4143b0` 등 하위 몸체는 해당 단위를 옮길 때 대조한다.
- `RitualChoiceInput`·`CreateAtInput`·`DetonateInput`·`ProductionShortcuts`는 별도 관측/장면(대상 진단, Guide/Gump 체인 trace, 팝업 경계, 입력 고리 8040행)이 있어 남겼다.

## 4. 다음 시작점

`RawGraph`/`RawGraphLive`, 공통 `SquidPostPop` 본체, `RawSquidPoolReset`, `RawBridgeConnect`, 그리고 Player 나머지(`RawPlayerWorld`·`RawPlayerTechnology`·`RawPlayerRegular`·`RawPlayerTemplateState` 등).
