// 원본 UserInput 의 OS 폴링 결과를 입력 큐와 연결하는 포인터 안팎 전환 앞단을 이식한다(cpppj client/GumpPointerBoundary.cpp/.h, 기준선 1f00a25a, 10.78 004d6379..004d65a0).
// 창 안→밖 전환 때 좌/우 버튼 상태를 기록하고, 밖→안 전환 때 기록상 눌려 있었지만 지금은 떼어진 버튼에 합성 release 를 입력 큐에 예약한다.
// 사건은 { code, x, y } 이고 code 는 DWORD, 좌표는 signed 다. 폴링은 요청 code 마다 현재 OS 상태를 새로 읽는다. 큐 예약은 원본 52f90 의 raw code/x/y 를 그대로 쓴다(PushCommand 변환 없음).
// 직후 Guide 4d65a7→카메라/Overlay/Broadcast/일반 사건 드레인은 호출자가 같은 현재 포인터로 이어 간다.
// 전용 PE fixture 는 없다(cpppj 에도 직접 시험이 없다). 기준선 뒤 변경도 없다.
// (2026-11-14 WB04-E433: RawMenuControls 선행 포인터 경계 이식)
import { InputCode } from '../core/input-queue.js';

// 원본은 현재 포인터의 좌표를 얻기 위해 Alt 를 폴링하며 language 3 에서만 Ctrl 상태를 함께 읽는다(요청 code 가 폴링 종류를 정한다).
const POINTER_POLL = 0x00120000; const CONTROL_POLL = 0x00110000; const COMBINED_LANGUAGE = 3;
// 005cafe8 의 두 일회 초기화 비트다. edge/tooltip 의 뒤쪽 초기화 비트를 지우지 않는다.
const LEFT_INITIALIZED = 1; const RIGHT_INITIALIZED = 2;
// code 의 최상위 비트만 검사한다. 다른 modifier/outside/pressedSince 비트는 바꾸지 않는다.
const pressed = (event) => (((event.code >>> 0) & InputCode.release) === 0);
// 사건 복사본을 만든다(필드를 그대로 전달하며 장치가 돌려준 객체를 변형하지 않는다).
const copy = (event) => ({ code: event.code >>> 0, x: event.x | 0, y: event.y | 0 });
// 실제 도달한 예약 분기에서만 필수 콜백을 검사한다.
function enqueue(hooks, event) {
    if (typeof hooks.queue !== 'function') throw new Error('포인터 재진입 release 의 실제 입력 큐 미연결');
    hooks.queue(event);
}

// Client 수명으로 유지하는 원본 정적 기록이다. 페이지·월드 교체 때 임의 초기화하지 않는다.
export class GumpPointerBoundaryState {
    // 005cafe8 의 전체 DWORD 이며 이 앞단은 비트 1/2 만 초기화한다. 나머지 비트는 보존한다.
    initializationFlags = 0;
    // 0054354c 의 PE 초기값 1 이다. 현재 포인터를 기록한 뒤에는 0/1 만 쓴다.
    wasInside = 1;
    // 005cafdc/e0/e4 와 005cafd0/d4/d8 의 마지막 inside→outside 좌/우 버튼 12바이트 기록이다.
    leftOutside = { code: 0, x: 0, y: 0 }; rightOutside = { code: 0, x: 0, y: 0 };
}

// 첫 포인터 폴링 뒤 정적 초기화→상태 전환 분기→현재 inside 저장 순서를 실행하고 정규화한 포인터를 반환한다.
// hooks = { poll(code), queue(event) }. 콜백은 같은 state 를 바꿀 수 있고 마지막에는 현재 포인터 값으로 wasInside 를 덮어쓴다. 사용: prepareGumpPointerBoundary(state, language, { poll, queue })
export function prepareGumpPointerBoundary(state, currentLanguage, hooks) {
    if (typeof hooks?.poll !== 'function') throw new TypeError('포인터 앞단의 실제 현재 폴링 필요');
    const pointer = copy(hooks.poll(POINTER_POLL)); // 원본 4d6398/4d63fd 의 첫 12바이트를 그대로 복사한다.
    if (currentLanguage >>> 0 === COMBINED_LANGUAGE) {
        const control = copy(hooks.poll(CONTROL_POLL)); // 원본은 두 사건이 모두 pressed 일 때 첫 code 에만 release 를 더한다.
        if (pressed(pointer) && pressed(control)) pointer.code = (pointer.code | InputCode.release) >>> 0;
    }
    if (!(state.initializationFlags & LEFT_INITIALIZED)) { state.initializationFlags = (state.initializationFlags | LEFT_INITIALIZED) >>> 0; state.leftOutside = { code: 0, x: 0, y: 0 }; }
    if (!(state.initializationFlags & RIGHT_INITIALIZED)) { state.initializationFlags = (state.initializationFlags | RIGHT_INITIALIZED) >>> 0; state.rightOutside = { code: 0, x: 0, y: 0 }; }
    const inside = ((pointer.code & InputCode.outside) === 0); // Guide 가 변경하기 전 폴링 사건의 현재 비트 26 이다.
    if (!state.wasInside) {
        if (inside) {
            const left = copy(hooks.poll(InputCode.leftButton)); // 좌·우를 먼저 모두 폴링한 다음 기록 비교를 시작한다.
            const right = copy(hooks.poll(InputCode.rightButton));
            if (pressed(state.leftOutside) && !pressed(left)) enqueue(hooks, left);
            if (pressed(state.rightOutside) && !pressed(right)) enqueue(hooks, right); // 좌 queue 콜백 뒤 현재 우 기록을 다시 읽는다.
        }
    } else if (!inside) {
        state.leftOutside = copy(hooks.poll(InputCode.leftButton)); // 우 poll 보다 먼저 좌 결과를 기록한다.
        state.rightOutside = copy(hooks.poll(InputCode.rightButton));
    }
    state.wasInside = inside ? 1 : 0; // 중간 poll/queue 콜백이 이 필드를 바꿔도 마지막에는 현재 포인터 값으로 덮어쓴다.
    return pointer;
}

// 실제 40칸 InputQueue.push 에 합성 release 를 연결한다. 큐 포화·기존 사건 순서는 기존 큐가 소유하며 code 의 release/수식 비트 및 signed 좌표를 손실하지 않는다.
// 사용: prepareGumpPointerBoundaryWithQueue(state, language, poll, inputQueue)
export function prepareGumpPointerBoundaryWithQueue(state, currentLanguage, poll, queue) {
    return prepareGumpPointerBoundary(state, currentLanguage, { poll, queue: (event) => queue.push(event.code, event.x, event.y) });
}
