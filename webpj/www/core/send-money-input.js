// 일반 송금 명령 여섯 개(Send300Bucks·Send700Bucks·SendBucks·SendXBucks·SendAllBucks2·SendAllBucks)의 금액·AI 지갑·로컬 메시지 전송 순서를 연결하는 입력 분배 모듈이다(cpppj `client/SendMoneyInput.cpp/.h`, 10.78 전용, 원문 4c9350 등).
// 메뉴와 HTML 확인 버튼이 같은 송금 부모를 쓴다. 수정키를 지운 코드가 여섯 명령이 아니면 false, 키 뗌(0x80000000)이면 효과 없이 true 이다. 대상 y 는 전체 DWORD 이다.
//  - 0x02000100(X 입력)은 numberDialog(대상), 0x0200012b(All 확인)는 askAll(대상)을 호출한다.
//  - 0x02000118(All 실행)은 canAfford(money())이면 현재 SP 의 float → 64비트 절삭 하위 32비트 정수 금액으로 이체한다. 0x02000120/0x02000108/0x020000ec 는 300/700/1000 이고 canAfford(금액)이면 이체한다(진단 줄 0x913/0x8e9/0x8c2).
//  - 이체(transfer): 대상의 플래그가 0x20(AI)이면 [고정 금액만 checking 이고 미션이 아닐 때 inSingle 진단] → addLocal(−금액) → wallet(대상, 금액), 아니고 0x1(연결 Player)이면 addOwner(로컬, −금액) → 11바이트 패킷 {1,0x6c,11,0,대상,로컬,0,금액 LE 4바이트}를 send. 그 밖의 플래그는 아무 효과도 없다.
//  - confirmSendMoney(대상, 문자열, hooks) [004c9350]: 첫 LF 앞을 CRT atol 형으로 읽어 양수이고 (PlayerKnown 이면 현재 SP, 아니면 0) 이하이며 canAfford 이면 Ui.cpp 0x61c 진단과 함께 이체한다. 잘못된 금액도 항상 1 로 닫는다.
// 선택된 하위의 누락은 그 위치에서 거부하고 이미 일어난 효과는 되돌리지 않는다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E27: 송금 일반 명령 이식으로 추가)
import { asciiLower, configParseLong } from '../assets/config.js';

// 여섯 명령의 원문 번호이다. (2026-11-08 WB04-E27)
export const SEND_300_COMMAND = 0x02000120;
export const SEND_700_COMMAND = 0x02000108;
export const SEND_1000_COMMAND = 0x020000ec;
export const SEND_X_COMMAND = 0x02000100;
export const SEND_ALL_ASK_COMMAND = 0x0200012b;
export const SEND_ALL_COMMAND = 0x02000118;
// 수정키 제거 마스크와 뗌 비트이다. (2026-11-08 WB04-E27)
const KEY_MASK = 0x03ffffff;
const RELEASE = 0x80000000;
// 이름표(소문자)이다. 고정 300/700/1000·X 입력·All 확인/실행의 서로 다른 명령이다. (2026-11-08 WB04-E27)
const NAMES = new Map([
    ['send300bucks', SEND_300_COMMAND],
    ['send700bucks', SEND_700_COMMAND],
    ['sendbucks', SEND_1000_COMMAND],
    ['sendxbucks', SEND_X_COMMAND],
    ['sendallbucks2', SEND_ALL_ASK_COMMAND],
    ['sendallbucks', SEND_ALL_COMMAND],
]);

// 선택된 경계가 없으면 그 자리에서 실패하며 이미 발생한 효과를 되돌리지 않는다. (2026-11-08 WB04-E27)
function need(device) {
    if (typeof device !== 'function') throw new Error('Send Money 실제 장치 미연결');
    return device;
}

// 원본 _ftol 의 64비트 절삭 뒤 하위 DWORD(signed)이다. 유한하지 않거나 범위 밖이면 0 이다. (2026-11-08 WB04-E27)
function amountOf(value) {
    if (!Number.isFinite(value) || value >= 9223372036854775808 || value < -9223372036854775808) return 0;
    return Number(BigInt.asIntN(32, BigInt(Math.trunc(value))));
}

// 같은 고정/All/X 의 플래그 분기를 공유하되 All 에는 원문에 없는 inSingle 진단을 넣지 않는다. (2026-11-08 WB04-E27)
function transfer(target, amount, diagnose, hooks, source, line) {
    const flags = need(hooks.flags)(target) >>> 0;
    // 원본 DWORD neg 뒤 signed fild 이다.
    const negative = Math.fround((0 - amount) | 0);
    if ((flags & 0x20) !== 0) {
        if (diagnose && need(hooks.checking)() && !need(hooks.mission)()) {
            if (hooks.assertionAt) hooks.assertionAt('inSingle', source, line);
            else need(hooks.assertion)();
        }
        need(hooks.addLocal)(negative);
        need(hooks.wallet)(target, amount);
    } else if ((flags & 1) !== 0) {
        need(hooks.addOwner)(need(hooks.local)(), negative);
        // 0040af90 초기 헤더와 실제 송금 kind 0 이다.
        const packet = Uint8Array.of(1, 0x6c, 11, 0, target & 0xff, need(hooks.local)() & 0xff, 0, 0, 0, 0, 0);
        const value = amount >>> 0;
        // 원본 비정렬 signed 금액을 낮은 BYTE 부터 쓴다.
        for (let byte = 0; byte < 4; byte += 1) packet[7 + byte] = (value >>> (8 * byte)) & 0xff;
        need(hooks.send)(packet);
    }
}

// 여섯 명령 중 하나인지이다(상위 26비트만 비교). (2026-11-08 WB04-E27)
export function isSendMoneyCommand(code) {
    const value = (code & KEY_MASK) >>> 0;
    for (const command of NAMES.values()) if (value === command) return true;
    return false;
}

// 대소문자 무시 이름 조회이며 모르는 이름은 0xffffffff 이다. (2026-11-08 WB04-E27)
export function sendMoneyScriptCode(name) {
    return NAMES.get(asciiLower(name)) ?? 0xffffffff;
}

// event = { code, x, y }, hooks = { local, known, editor, mission, checking, money, canAfford, flags, addLocal, addOwner, wallet, send, numberDialog, askAll, assertion, assertionAt } 이다. 소비하면 true 이다. (2026-11-08 WB04-E27)
export function dispatchSendMoneyInput(event, hooks) {
    const code = (event.code & KEY_MASK) >>> 0;
    if (!isSendMoneyCommand(code)) return false;
    if ((event.code & RELEASE) !== 0) return true;
    const target = event.y >>> 0;
    if (code === SEND_X_COMMAND) { need(hooks.numberDialog)(target); return true; }
    if (code === SEND_ALL_ASK_COMMAND) { need(hooks.askAll)(target); return true; }
    if (code === SEND_ALL_COMMAND) {
        // 확인 순간의 SP 를 읽고 정수 부분만 송금한다.
        if (need(hooks.canAfford)(need(hooks.money)())) transfer(target, amountOf(need(hooks.money)()), false, hooks, '', 0);
        return true;
    }
    const amount = code === SEND_300_COMMAND ? 300 : code === SEND_700_COMMAND ? 700 : 1000;
    // 원문 고정 300/700/1000 의 서로 다른 Userinput.cpp 진단 줄이다.
    const line = code === SEND_300_COMMAND ? 0x913 : code === SEND_700_COMMAND ? 0x8e9 : 0x8c2;
    if (need(hooks.canAfford)(amount)) transfer(target, amount, true, hooks, '.\\Userinput.cpp', line);
    return true;
}

// 004c9350 의 확인 버튼 몸체이다. 자금 판정 뒤 현재 대상을 사용하며 잘못된 금액도 1 로 닫는다. (2026-11-08 WB04-E27)
export function confirmSendMoney(target, text, hooks) {
    const lf = text.indexOf('\n');
    const amount = configParseLong(lf < 0 ? text : text.slice(0, lf));
    if (amount > 0) {
        const visible = need(hooks.known)() ? need(hooks.money)() : 0;
        if (Math.fround(amount) <= visible && need(hooks.canAfford)(Math.fround(amount))) transfer(target, amount, true, hooks, '.\\Ui.cpp', 0x61c);
    }
    return 1;
}
