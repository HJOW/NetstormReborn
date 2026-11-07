// 생산 조각 문자 키(Q/W/A/S/Z·Y/X/E/D, 원문 Userinput 일반 키 구간)를 실제 명령 큐에 예약하는 분배를 옮긴 모듈이다(cpppj `client/ProductionShortcuts.cpp/.h`, 10.78 전용).
// queueProductionShortcut(event, paused, hooks): Guide/Gump 뒤의 일반 키 구간이다. 키 뗌이거나 정지(깊이>0)면 예약하지 않고 false 이며 modifier 와 활성 커서를 추가로 거부하지 않는다. 수정키를 지운 코드를 문자로 해석한다.
//   q/Q → 슬롯 0, w/W → 1, a/A → 2, s/S → 3, z/Z/y/Y → 4, x/X → 5 를 예약 직전에 현재 명령 전역(hooks.command(), 00542ac4: 현재 기술의 slot BYTE 취득 명령)을 읽어 hooks.queue(명령, 슬롯, 0)으로 예약한다.
//   e/E 는 현재 슬롯 수와 무관하게 여섯 명령을 FIFO 로 예약하며 매번 현재 명령을 다시 읽는다. d/D 는 별도 최근 항목 명령(hooks.repeatCommand(), 00542b10: 현재 lastType 과 일치하는 생산 항목 재취득)을 소비 직전에 읽어 (명령, 0, 0)으로 예약한다.
//   그 밖의 문자는 소비하지 않는다(false). 선택된 문자 키만 명령 조회/큐 예약 장치를 요구하고, 중간에 큐가 실패해도 앞선 예약은 되돌리지 않는다. 큐의 포화 정책은 해당 큐(InputQueue)가 수행한다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E36: 생산 단축키 이식으로 추가)
import { InputCode } from './input-queue.js';

// 명령 번호(00542ac4 다리 슬롯 취득, 00542b10 최근 항목 재취득)와 수정키 제거 마스크, 여섯 다리 슬롯 수이다. (2026-11-08 WB04-E36)
export const PRODUCTION_BRIDGE_COMMAND = 0x020000f4;
export const PRODUCTION_REPEAT_COMMAND = 0x02000107;
const KEY_MASK = 0x03ffffff;
const BRIDGE_SLOTS = 6;

// event = { code, x, y }, hooks = { command(), repeatCommand(), queue(명령, 슬롯, 0) } 이다. 소비하면 true 이다. (2026-11-08 WB04-E36)
export function queueProductionShortcut(event, paused, hooks) {
    if ((event.code & InputCode.release) !== 0 || paused) return false;
    // getter 효과 뒤 해당 명령 인자값을 고정하고 같은 큐에 예약한다. 최근 항목과 여섯 다리 슬롯의 현재 명령을 구별한다.
    const enqueue = (slot, repeat = false) => {
        const lookup = repeat ? hooks.repeatCommand : hooks.command;
        if (typeof lookup !== 'function') throw new TypeError('생산 단축키의 현재 명령 조회 누락');
        const command = lookup();
        if (typeof hooks.queue !== 'function') throw new TypeError('생산 단축키의 실제 큐 예약 누락');
        hooks.queue(command, slot, 0);
    };
    const key = (event.code & KEY_MASK) >>> 0;
    switch (key) {
    case 0x64: case 0x44: enqueue(0, true); break; // d/D: 004d96fa 의 getlastitem 은 별도 최근 TYPE 소비자에 0/0 만 보낸다.
    case 0x71: case 0x51: enqueue(0); break; // q/Q
    case 0x77: case 0x57: enqueue(1); break; // w/W
    case 0x61: case 0x41: enqueue(2); break; // a/A
    case 0x73: case 0x53: enqueue(3); break; // s/S
    case 0x7a: case 0x5a: case 0x79: case 0x59: enqueue(4); break; // z/Z/y/Y
    case 0x78: case 0x58: enqueue(5); break; // x/X
    case 0x65: case 0x45:
        // 원본은 현재 슬롯 수와 무관하게 여섯 명령을 FIFO 로 예약한다.
        for (let slot = 0; slot < BRIDGE_SLOTS; slot += 1) enqueue(slot);
        break;
    default: return false;
    }
    return true;
}
