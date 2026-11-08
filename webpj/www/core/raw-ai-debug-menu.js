// 원문 413dd0 의 선택적 AI 디버그 메뉴를 현재 Player/brain 과 controlled 메뉴 작성기에 연결하는 모듈이다(cpppj `client/RawAiDebugMenu.cpp/.h`, 10.78). 파일/창/AI 행동을 새로 소유하지 않는 현재 메뉴 부모이다.
//  - openAiDebugMenu(owner, reads, hooks): reads = { checking()(showSoftAsserts 현재 DWORD, 첫 단락에서만 읽음), player(owner)→Uint8Array(실제 현재 Player 한 기록 180바이트), body(token)→Uint8Array(실제 현재 brain 0xb08), debug(address)→DWORD(원문 로그 전역의 현재 값, 각 효과 앞에서 읽음),
//    command(address)→DWORD(명령 주소 0x542a34 AI Reporting / 0x542a38 Debug 스위치 / 0x5428d8 파라미터 공통 행의 현재 값) },
//    hooks = { format(limit, pattern, args)→문자열(4b9390 요청 80 문자열 서식 장치, 인자는 [정수 또는 문자열] 하나), header(text)(번역 없는 원문 머리 문자열을 그대로 작성), checked(on, label, command, parameter, include, enabled)(전체 DWORD 인자 여섯 개),
//    begin(flags)(477bb0(0) 같은 수직 controlled 메뉴 스택), take()→토큰(475c00 현재 메뉴를 한 번 꺼내는 부모), submenu(label, child, enabled, checked)(실제 Submenu 작성) } 는 도달한 위치에서만 요구한다.
//  - checking 이 0 이거나 owner 가 signed 비양수이면, 또는 현재 Player+0xb0 brain 이 0 이면 효과 없이 돌아간다(첫 단락만 reads.checking 을 읽음). owner 가 9 이상이면 거부한다.
//    그 뒤 원문 순서: (초기화 +0x258 이 0 이면 "NOT INITIALIZED\n" 머리) → "~kMoney=%d~.\n"(+0x2d8 signed) → "~kGoal=%s~.\n"(+0x2d4 의 None/Player/Geyser/Attack/Hookup, 표 밖은 거부) → "AI Reporting" 체크(+0xb00 전체 DWORD) → begin → 진단 7행(Debug Bridge/Geyser/Lay Piece/Build/Move/Dry Spot/Factory,
//    현재 로그 값 == 29 를 각 행 바로 앞에서 조회) → take → "AI Debug" Submenu → 파라미터 다섯 묶음(aiBridgeDrawRate/aiTimeBetweenMoves/aiGeyserAttachments/aiCollectors/aiMoneyRechargeRate)과 "AI Parameters" Submenu 를 작성한다.
//  - 원문은 처음 계산한 Player 행을 callback 동안 보관하고 효과 callback 뒤에도 같은 행의 현재 +0xb0 만 다시 읽으며(교체된 brain 을 따라감), 파라미터 단계에서는 "AI Debug" 작성 뒤 한 번 읽은 brain 을 모든 파라미터 행에서 보관한다. 부분 실패는 되돌리지 않는다.
// 10.72(CD)는 범위 밖이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E182: AI 디버그 메뉴 이식으로 추가)

// 원문 목표 0..4 의 실제 이름표와 진단 7행(문구, 로그 전역 주소)의 실제 메뉴 순서이다. (2026-11-09 WB04-E182)
const GOALS = Object.freeze(['None', 'Player', 'Geyser', 'Attack', 'Hookup']);
const DIAGNOSTICS = Object.freeze([
    ['Debug Bridge', 0x52f3cc], ['Debug Geyser', 0x52f3e4], ['Debug Lay Piece', 0x52f3d0], ['Debug Build', 0x52f3d4],
    ['Debug Move', 0x52f3d8], ['Debug Dry Spot', 0x52f3dc], ['Debug Factory', 0x52f3e0]
].map((row) => Object.freeze(row)));
// 명령 주소(AI Reporting/Debug 스위치/파라미터 공통 행)이다. (2026-11-09 WB04-E182)
const REPORTING_ADDRESS = 0x542a34;
const SWITCH_ADDRESS = 0x542a38;
const PARAMETER_ADDRESS = 0x5428d8;

// 선택된 하위 경계가 없으면 빈 메뉴 성공으로 효과를 누락하지 않고 거부한다. (2026-11-09 WB04-E182)
function need(hook) {
    if (typeof hook !== 'function') throw new Error('AI 메뉴 실제 하위 미연결');
    return hook;
}

// 원문 비정렬 DWORD 를 같은 폭으로 읽는다. (2026-11-09 WB04-E182)
function word(bytes, offset) {
    if (offset < 0 || offset > bytes.length || bytes.length - offset < 4) throw new RangeError('AI 메뉴 현재 DWORD 물리범위');
    return (bytes[offset] | (bytes[offset + 1] << 8) | (bytes[offset + 2] << 16) | (bytes[offset + 3] << 24)) >>> 0;
}

// checking 과 현재 brain 의 단락 뒤 원문 메뉴 효과를 원래 순서대로 작성한다. (2026-11-09 WB04-E182)
export function openAiDebugMenu(owner, reads, hooks) {
    if (!need(reads.checking)() || ((owner >>> 0) | 0) <= 0) return;
    if (owner >= 9) throw new RangeError('AI 메뉴 Player owner 물리범위');
    // 원문은 처음 계산한 Player 행 주소를 callback 동안 보관한다.
    const player = need(reads.player)(owner);
    // 같은 행의 현재 +0xb0 만 효과 callback 뒤 다시 읽는다.
    const brain = () => word(player, 0xb0);
    if (!brain()) return;
    // 해제/NULL 을 가짜 본문으로 대체하지 않는다.
    const field = (token, offset) => {
        if (!token) throw new RangeError('AI 메뉴 재조회 NULL brain');
        return word(need(reads.body)(token), offset);
    };
    // 첫 공통 서식 요청 폭 80 을 보존한다.
    const print = (pattern, value) => need(hooks.format)(80, pattern, [value]);
    // decimal 600 은 초기화 +0x258 이며 bridge 의 hex 600 이 아니다.
    if (!field(brain(), 0x258)) need(hooks.header)('NOT INITIALIZED\n');
    need(hooks.header)(print('~kMoney=%d~.\n', field(brain(), 0x2d8) | 0));
    const goal = field(brain(), 0x2d4);
    if (goal >= GOALS.length) throw new RangeError('AI 메뉴 목표 문자열표 범위');
    need(hooks.header)(print('~kGoal=%s~.\n', GOALS[goal]));
    need(hooks.checked)(field(brain(), 0xb00), 'AI Reporting', need(reads.command)(REPORTING_ADDRESS), owner, 1, 1);
    need(hooks.begin)(0);
    // 같은 현재 로그 값을 각 행 바로 앞에서 조회한다.
    for (const [label, address] of DIAGNOSTICS) {
        const command = need(reads.command)(SWITCH_ADDRESS);
        need(hooks.checked)(need(reads.debug)(address) === 29 ? 1 : 0, label, command, address, 1, 1);
    }
    const debug = need(hooks.take)();
    need(hooks.submenu)('AI Debug', debug, 1, 0);
    // 원문은 AI Debug 작성 뒤 한 번 읽은 본문을 모든 파라미터 행에서 보관한다.
    const captured = brain();
    need(hooks.begin)(0);
    need(hooks.begin)(0);
    // 공통 행도 호출 시점의 보관 본문 값을 다시 읽는다.
    const parameter = (label, offset, expected, packed, zeroFloat = false) => {
        const value = field(captured, offset);
        let checked;
        if (zeroFloat) {
            const view = new DataView(new ArrayBuffer(4));
            view.setUint32(0, value, true);
            checked = view.getFloat32(0, true) === 0.0;
        } else checked = value === expected;
        need(hooks.checked)(checked ? 1 : 0, label, need(reads.command)(PARAMETER_ADDRESS), (owner | packed) >>> 0, 1, 1);
    };
    // 현재 하위 메뉴를 한 번만 꺼내 바로 부착한다.
    const finish = (label) => {
        const token = need(hooks.take)();
        need(hooks.submenu)(label, token, 1, 0);
    };
    parameter('0', 0x220, 0, 0, true);
    parameter('1', 0x220, 0x3f800000, 0x10000);
    parameter('2', 0x220, 0x40000000, 0x20000);
    parameter('6', 0x220, 0x40c00000, 0x60000);
    finish('aiBridgeDrawRate');
    need(hooks.begin)(0);
    parameter('2', 0x228, 0x40000000, 0x20010);
    parameter('3', 0x228, 0x40400000, 0x30010);
    parameter('4', 0x228, 0x40800000, 0x40010);
    finish('aiTimeBetweenMoves');
    need(hooks.begin)(0);
    parameter('3', 0x230, 3, 0x30020);
    parameter('6', 0x230, 6, 0x60020);
    parameter('9', 0x230, 9, 0x90020);
    finish('aiGeyserAttachments');
    need(hooks.begin)(0);
    parameter('3', 0x234, 3, 0x30030);
    parameter('6', 0x234, 6, 0x60030);
    parameter('10', 0x234, 10, 0xa0030);
    finish('aiCollectors');
    need(hooks.begin)(0);
    parameter('0', 0x24c, 0, 0x40);
    parameter('8', 0x24c, 8, 0x80040);
    parameter('30', 0x24c, 30, 0x1e0040);
    finish('aiMoneyRechargeRate');
    finish('AI Parameters');
}
