// 대본(스크립트) 명령이 예약한 일반 입력 중 DestroyAt·AddRandom·MakeAi 의 분배기를 옮긴 모듈이다(cpppj `client/DestroyAtInput.cpp`, `client/AddRandomInput.cpp`, `client/MakeAiCommandInput.cpp`, 10.78 전용).
// 모두 Guide/Gump 뒤의 일반 입력 하나(event = { code, x, y })를 받아 소비 여부를 돌려준다. 수정키를 제거한(0x03ffffff) 코드가 자기 명령 번호가 아니면 false, 키 뗌이면 효과 없이 true 이다. 명령 번호는 원본 전역의 초기 토큰이다.
//  - dispatchDestroyAtInput(event, editor, mission, destroyAt) [004d8528..004d8551, 0x020000e1]: Edit/미션 중 누름만 y 의 32비트 전체(문자열 토큰)를 부모에 전달하고 일반 모드는 소비만 한다. 현재 선택이나 메뉴 열기 조건을 추가하지 않는다.
//  - dispatchAddRandomInput(event, editor, mission, lookup, add) [4d7c33..4d7c6c, 0x02000098]: 누름이고 Edit/미션 중이면 y 를 타입으로 쓰되 signed y 가 255 를 넘으면 문자열 타입 조회(lookup(y)의 signed 반환을 DWORD 로 보존; -1/0 도 배치 부모로 전달)를 한 뒤 add(타입, 1).
//  - dispatchMakeAiCommandInput(event, hooks) [0x020000b7]: 누름이면 Editor 가 비0 이거나(단락; 그 경우 missionPost 는 읽지 않음) missionPost 가 비0 일 때 y 전체 DWORD 로 makeAi(owner) 호출(자체 battle 관문과 AI 수명은 부모가 유지). makeAiCommandScriptCode 는 첫 NUL 앞 이름의 ASCII 대소문자 무시 비교이다.
// 선택된 하위 장치의 누락은 그 위치에서 거부하며 도달하지 않은 장치는 요구하지 않는다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E21: 대본 일반 입력 분배 이식으로 추가)
import { InputCode } from './input-queue.js';

// 각 명령의 원본 전역 초기 토큰과 수정키 제거 마스크이다. (2026-11-08 WB04-E21)
export const DESTROY_AT_COMMAND = 0x020000e1;
export const ADD_RANDOM_COMMAND = 0x02000098;
export const MAKE_AI_COMMAND = 0x020000b7;
const KEY_MASK = 0x03ffffff;

// 선택된 하위가 없으면 그 위치에서 거부한다. (2026-11-08 WB04-E21)
function need(device, name) {
    if (typeof device !== 'function') throw new Error(`${name} 의 실제 현재 부모 미연결`);
    return device;
}

// 뗌 비트 판정이다. (2026-11-08 WB04-E21)
const released = (event) => (event.code & InputCode.release) !== 0;

// Edit/미션 중 누름만 y 의 문자열 토큰을 부모에 전달하고 다른 명령은 false 이다. destroyAt(token) 은 문자열 삭제 부모이다. (2026-11-08 WB04-E21)
export function dispatchDestroyAtInput(event, editor, mission, destroyAt) {
    if (((event.code & KEY_MASK) >>> 0) !== DESTROY_AT_COMMAND) return false;
    if (released(event) || (!editor && !mission)) return true;
    need(destroyAt, 'DestroyAt')(event.y >>> 0);
    return true;
}

// 키 뗌과 일반 모드는 소비만 하고, signed y 가 255 보다 클 때만 문자열 타입 조회를 한다. lookup(token) 은 signed 반환, add(type, quantity) 는 배치 부모이다. (2026-11-08 WB04-E21)
export function dispatchAddRandomInput(event, editor, mission, lookup, add) {
    if (((event.code & KEY_MASK) >>> 0) !== ADD_RANDOM_COMMAND) return false;
    if (released(event) || (!editor && !mission)) return true;
    let type = event.y >>> 0;
    // 원문처럼 lookup 의 -1/0 도 배치 부모로 전달하고 잘못된 타입 진단은 그 하위에 맡긴다.
    if (event.y > 255) type = need(lookup, 'AddRandom 타입 이름 조회')(type) >>> 0;
    need(add, 'AddRandom 배치')(type, 1);
    return true;
}

// 원문 이름표(ASCII 대소문자 무시, 첫 NUL 앞만)로 MakeAi 명령 번호를 구한다. 없으면 0xffffffff 이다. (2026-11-08 WB04-E21)
export function makeAiCommandScriptCode(name) {
    const nul = name.indexOf('\0');
    const text = nul < 0 ? name : name.slice(0, nul);
    return text.replace(/[A-Z]/g, (letter) => letter.toLowerCase()) === 'makeai' ? MAKE_AI_COMMAND : 0xffffffff;
}

// 분류 전에 수식 비트를 제거한 정확한 명령값만 받는다. (2026-11-08 WB04-E21)
export const isMakeAiCommand = (code) => (code >>> 0) === MAKE_AI_COMMAND;

// hooks = { editor() → DWORD, missionPost() → DWORD, makeAi(owner DWORD) }. 뗌은 효과 없이 소비하고 다른 명령은 false 이다. (2026-11-08 WB04-E21)
export function dispatchMakeAiCommandInput(event, hooks) {
    if (!isMakeAiCommand((event.code & KEY_MASK) >>> 0)) return false;
    if (released(event)) return true;
    if (need(hooks.editor, 'MakeAi 입력')() || need(hooks.missionPost, 'MakeAi 입력')()) need(hooks.makeAi, 'MakeAi 입력')(event.y >>> 0);
    return true;
}
