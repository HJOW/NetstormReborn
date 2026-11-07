// 원본 일반 키 부모 중 선택 키(H/N/P/R/F4/F5/F6)·AI/간헐천 일반 명령 세 개·편집 객체 취득을 옮긴 모듈이다(cpppj `client/SelectionKeyInput.cpp`, `client/AiUtilityCommandInput.cpp`, `client/CursorObjectPickup.cpp`, 10.78 전용).
//  - dispatchSelectionKeyInput(event, hooks) [004d96b6 정지 관문 이후]: 뗌이거나 확인한 키가 아니면 false. 정지(paused)면 소비. F6(0x750000)는 Shift/Control이 있으면 소비만, 아니면 지식 명령(0x00542974)을 (명령, 0, 0)으로 예약.
//    N, 또는 Ctrl+F5(0x740000)는 원문 Shift 비트 전체를 기존 다음 유닛 몸체(next)에 전달. P/R는 사제 몸체(priest). H와 F4(0x730000)는 F4가 Shift/Control이면 소비만, 아니면 Home 명령(0x005429ac)을 (명령, 0, 0)으로 예약.
//    그 밖(F5)은 현재 localOwner 로 사제 보기 명령(0x005429d4)을 (명령, owner, owner)로 예약. 원문은 Alt 를 별도 거부 조건으로 쓰지 않는다.
//  - dispatchAiUtilityCommandInput(event, hooks): 수정키를 제거한 코드가 세 명령(CreateInitialGeysers 0x020000ad·BrainCrankMoney 0x020000c1·AiParamSet 0x02000078)일 때만 소비. 뗌은 효과 없이 소비.
//    CreateInitialGeysers → 전체 GenerateWorldGeysers(initialGeysers), BrainCrankMoney → 현재 Player1..8 brain 직접 쓰기(crankMoney), AiParamSet → y 의 전체 DWORD 를 parameter 로 전달. 모드/정지/owner 를 새 관문으로 읽지 않는다.
//    aiUtilityCommandScriptCode 는 세 이름의 ASCII 대소문자 무시 조회이다.
//  - pickupObjectForCursor(pool, source, flags, hooks) [00447340]: 10.78 풀의 물리 SID 범위를 확인하고 Unpop(source, 0) → 그 뒤 현재 슬롯의 TYPE BYTE(+10)와 argument WORD(+12)를 다시 읽어 create(sid, type, argument, 0x80, 0x10, 0)(반환 0도 그대로) → take(form, flags | 0x100).
// 선택된 장치가 없으면 그 위치에서 거부하며 앞선 효과(Unpop 등)는 되돌리지 않는다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E23: 선택 키·AI 일반 명령·편집 객체 취득 이식으로 추가)
import { InputCode } from './input-queue.js';

// 수정키 제거 마스크, 선택 키 코드, 명령 번호이다. (2026-11-08 WB04-E23)
const KEY_MASK = 0x03ffffff;
const KEY_F4 = 0x730000;
const KEY_F5 = 0x740000;
const KEY_F6 = 0x750000;
export const CREATE_INITIAL_GEYSERS_COMMAND = 0x020000ad;
export const BRAIN_CRANK_MONEY_COMMAND = 0x020000c1;
export const AI_PARAM_SET_COMMAND = 0x02000078;

// 선택된 장치가 없으면 그 위치에서 거부한다. (2026-11-08 WB04-E23)
function need(device, message) {
    if (typeof device !== 'function') throw new Error(message);
    return device;
}

// event = { code, x, y }, hooks = { paused(), command(주소) → 명령 DWORD, localOwner(), queue(command, first, second), next(shiftBits), priest() }. 소비하면 true 이다. (2026-11-08 WB04-E23)
export function dispatchSelectionKeyInput(event, hooks) {
    const message = '선택 키의 실제 장치 미연결';
    const key = (event.code & KEY_MASK) >>> 0;
    const home = key === 0x48 || key === 0x68;
    const next = key === 0x4e || key === 0x6e;
    const priest = key === 0x50 || key === 0x70 || key === 0x52 || key === 0x72;
    // 문자 하위 16비트만 비교해 일반 명령이나 마우스를 키로 오인하지 않는다.
    if ((event.code & InputCode.release) !== 0 || (!home && !next && !priest && key !== KEY_F4 && key !== KEY_F5 && key !== KEY_F6)) return false;
    if (need(hooks.paused, message)()) return true;
    if (key === KEY_F6) {
        // 원문 4d9a00 의 F6 는 공통 정지 뒤 수정키를 검사하고 일반 ShowTechnology 명령만 예약한다.
        if (event.code & (InputCode.shift | InputCode.control)) return true;
        const command = need(hooks.command, message)(0x00542974);
        need(hooks.queue, message)(command, 0, 0);
    } else if (next || (key === KEY_F5 && (event.code & InputCode.control))) need(hooks.next, message)((event.code & InputCode.shift) >>> 0);
    else if (priest) need(hooks.priest, message)();
    else if (home || key === KEY_F4) {
        if (key === KEY_F4 && (event.code & (InputCode.shift | InputCode.control))) return true;
        const command = need(hooks.command, message)(0x005429ac);
        need(hooks.queue, message)(command, 0, 0);
    } else {
        const owner = need(hooks.localOwner, message)();
        const command = need(hooks.command, message)(0x005429d4);
        need(hooks.queue, message)(command, owner, owner);
    }
    return true;
}

// 원문 이름 lookup 의 ASCII 대소문자 무시 계약으로 세 명령 번호를 구한다. 없으면 0xffffffff 이다. (2026-11-08 WB04-E23)
export function aiUtilityCommandScriptCode(name) {
    const key = name.replace(/[A-Z]/g, (letter) => letter.toLowerCase());
    if (key === 'createinitialgeysers') return CREATE_INITIAL_GEYSERS_COMMAND;
    if (key === 'braincrankmoney') return BRAIN_CRANK_MONEY_COMMAND;
    if (key === 'aiparamset') return AI_PARAM_SET_COMMAND;
    return 0xffffffff;
}

// 호출자가 수정키/outside 를 제거한 원문 명령값 세 개만 분류한다. (2026-11-08 WB04-E23)
export const isAiUtilityCommand = (code) => {
    const value = code >>> 0;
    return value === CREATE_INITIAL_GEYSERS_COMMAND || value === BRAIN_CRANK_MONEY_COMMAND || value === AI_PARAM_SET_COMMAND;
};

// hooks = { initialGeysers(), crankMoney(), parameter(DWORD) }. x 는 쓰지 않고 parameter 만 y 의 전체 32비트를 쓴다. (2026-11-08 WB04-E23)
export function dispatchAiUtilityCommandInput(event, hooks) {
    const code = (event.code & KEY_MASK) >>> 0;
    if (!isAiUtilityCommand(code)) return false;
    if ((event.code & InputCode.release) !== 0) return true;
    const message = 'AI/간헐천 일반 명령의 실제 부모 미연결';
    if (code === CREATE_INITIAL_GEYSERS_COMMAND) need(hooks.initialGeysers, message)();
    else if (code === BRAIN_CRANK_MONEY_COMMAND) need(hooks.crankMoney, message)();
    else need(hooks.parameter, message)(event.y >>> 0);
    return true;
}

// 가상 효과 뒤 TYPE/WORD 가 바뀌면 바뀐 현재값을 사용하며 앞선 Unpop 을 되돌리지 않는다. hooks = { unpop(sid, 0), create(sid, type, argument, 0x80, 0x10, 0) → form, take(form, flags) }. (2026-11-08 WB04-E23)
export function pickupObjectForCursor(pool, source, flags, hooks) {
    if (pool?.edition !== 'patch1078' || source >= pool.capacity || source > 65535) throw new RangeError('편집 객체 취득의 10.78 물리 SID 범위');
    const message = '편집 객체 취득의 실제 하위 미연결';
    need(hooks.unpop, message)(source, 0);
    const raw = pool.slot(source);
    // 원문의 MOVZX WORD 다음 MOVZX BYTE 이다.
    const argument = raw[12] | (raw[13] << 8);
    const type = raw[10];
    const form = need(hooks.create, message)(source, type, argument, 0x80, 0x10, 0);
    need(hooks.take, message)(form, (flags | 0x100) >>> 0);
}
