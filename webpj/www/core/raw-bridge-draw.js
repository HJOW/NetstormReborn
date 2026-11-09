// 다리 캡처의 실제 기본 프레임과 기본 0 인 수명 숫자 표시의 가상 경계이다(cpppj `o/RawBridgeDraw.h`, 10.78 전용; 원본 4222f0 의 mov eax,[ecx+24]/ret 와 4223c0).
// 같은 raw 풀의 다리 가상 몸체(+0x78 기본 프레임 getter, +0xa4 추가 그림)를 ShapeToBuffer 에 공급한다. 현재 프레임을 캐시하지 않고 호출 때마다 풀 슬롯을 다시 읽는다.
//  - createBridgeDrawState(): 실제 5453ac 의 기본 0 에 해당하는 { lifeNumbers: 0 } 현재 표시 전역이다. 개발용 숫자 표시를 지원할 때도 같은 객체를 공유한다.
//  - RawBridgeDraw(pool, state): 풀과 현재 표시 전역은 이 처리기보다 오래 살아야 한다.
//    mainFrame(sid): 슬롯 +0 의 가상 표 기록값이 다리 클래스(0x005034c8)가 아니면 오류이고, 맞으면 프레임 DWORD(+0x24=36)를 signed int32 로 돌려준다(ShapeToBuffer 의 extra&2 기본 그림).
//    additional(sid, x, y): 실제 4223c0 은 lifeNumbers 가 0 이면 source/x/y 를 읽지 않고 정상 반환한다. 비0 이면 미복원 개발용 숫자를 빈 성공으로 넘기지 않고 오류로 멈춘다.
// 이 모듈은 DOM 을 쓰지 않는다. 실제 World/캡처 연결은 5-A 렌더러 연결에서 한다. (2026-11-10 WB04-E245: 3-B RawBridgeDraw 이식으로 추가)

// 다리 클래스의 가상 표 기록값(10.78)이며 raw 슬롯 +0 의 DWORD 와 비교만 한다. (2026-11-10 WB04-E245)
export const BRIDGE_DRAW_VTABLE = 0x005034c8;

// 슬롯의 프레임 DWORD 위치이다. (2026-11-10 WB04-E245)
const FRAME_OFFSET = 36;

// 비정렬 슬롯 DWORD 를 원본 리틀엔디언 순서로 읽는다(부호 없는 수). (2026-11-10 WB04-E245)
function dword(raw, offset) {
    let value = 0;
    // 네 바이트의 원래 자리를 보존한다.
    for (let index = 0; index < 4; index += 1) value += raw[offset + index] * 2 ** (8 * index);
    return value;
}

// 실제 5453ac 의 기본 0 에 해당하는 현재 수명 숫자 표시 전역을 만든다. 호출자가 값을 바꾸면 다음 additional 이 그 값을 읽는다. (2026-11-10 WB04-E245)
export function createBridgeDrawState() {
    return { lifeNumbers: 0 };
}

export class RawBridgeDraw {
    #pool;
    #state;

    // 현재 동결은 10.78 이다. 풀이 10.78 이 아니면 거부한다. (2026-11-10 WB04-E245)
    constructor(pool, state) {
        if (pool?.edition !== 'patch1078') throw new TypeError('다리 캡처 가상 그림 판본 오류');
        this.#pool = pool;
        this.#state = state;
    }

    // 실제 4222f0 에 해당하며 실제 다리의 현재 프레임 DWORD 를 signed int32 로 돌려준다. 다른 가상 표의 객체는 거부한다. (2026-11-10 WB04-E245)
    mainFrame(sid) {
        const raw = this.#pool.slot(sid);
        if (dword(raw, 0) !== BRIDGE_DRAW_VTABLE) throw new Error('다리 기본 그림의 다른 가상 표');
        return dword(raw, FRAME_OFFSET) | 0;
    }

    // 실제 4223c0 은 기본 0 일 때 source/x/y 를 읽지 않는다. 미복원 개발용 숫자는 빈 성공으로 넘기지 않는다. (2026-11-10 WB04-E245)
    additional() {
        if (this.#state.lifeNumbers) throw new Error('개발용 다리 수명 숫자 표시 미복원');
    }
}
