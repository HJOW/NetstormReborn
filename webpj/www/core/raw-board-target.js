// 운반 명령의 일반 접근점(41d530)과 사각형 변 중간점(41d400)을 옮긴 모듈이다(cpppj `o/RawBoardTarget.cpp/.h`, 10.78 전용).
// 건설 사제의 Graph 순회(41db70)와 구별되는 원본 기하 계산이며, 좌표를 지도 범위로 보정하지 않는다. 사각형 오른쪽의 확장만 x87(53비트 배정도)에 남고 다른 세 변은 float 저장을 거친다.
//  - side(rectangle, side, flags): 0/4(북/남)이면 가로 중간점과 위/아래 변, 2/6(동/서)이면 오른쪽/왼쪽 변과 세로 중간점을 돌려준다. flags 의 bit2 이면 사각형을 복사해 왼쪽·위·아래를 1 만큼 float 로 넓히고 오른쪽은 배정도로 넓힌다.
//    그 밖의 방향은 원본 진단(0x1f9)을 부른 뒤 flags 를 float 로 읽은 값(signaling NaN 은 quiet 로)을 두 좌표로 돌려준다.
//  - nearest(rectangle, source, flags): 사각형 중심(float 저장)을 향한 출발점의 방향을 4 칸 돌려 가장 가까운 변을 고른다. flags bit1 이 있고 방향이 0(북)이면 출발점이 중심 왼쪽이면 서(6), 아니면 동(2)이다(순서 없는 값도 동쪽).
//    flags bit0 이 있으면 side 결과이고, 없으면 진단(0x262) 뒤 현재 실패점(state.invalid)을 그대로 돌려준다.
// state = BoardTargetState(invalid: { x, y }, 월드 초기화 4af170 이 기록하는 -10000 쌍; 진단 뒤 현재 값을 다시 읽음), assertion(line)은 해당 분기에 도달할 때만 필요하다. 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-08 WB04-E100: 운반 대상 접근점 이식으로 추가)
import { RawSquidNeighbors } from './raw-squid-neighbors.js';

// 중간점 비율과 변 확장 폭(실제 500ee4/5012d0)이다. (2026-11-08 WB04-E100)
const HALF = 0.5;
const EXPAND = 1;
const fround = Math.fround;

// 단정도 float 비트(uint32)를 수로 바꾼다. (2026-11-08 WB04-E100)
function bitsToFloat(bits) {
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, bits >>> 0, true);
    return view.getFloat32(0, true);
}

// 잘못된 방향의 원본 FLD 는 flags 를 float 로 읽으며 signaling NaN 을 quiet 로 바꾼다. (2026-11-08 WB04-E100)
function flagFloat(flags) {
    let bits = flags >>> 0;
    if ((bits & 0x7f800000) === 0x7f800000 && (bits & 0x7fffff)) bits = (bits | 0x400000) >>> 0;
    return bitsToFloat(bits);
}

// 월드 초기화 4af170 이 기록한 실패점이며 진단 뒤 현재 값을 다시 읽는다. (2026-11-08 WB04-E100)
export class BoardTargetState {
    // 545370/545374 의 실제 초기화 후 두 float 이다.
    invalid = { x: -10000, y: -10000 };
}

// 일반 접근점/변 중간점 계산기이다. (2026-11-08 WB04-E100)
export class RawBoardTarget {
    // 실패 시 진단 뒤 현재 두 float 를 사용하는 상태이다. (2026-11-08 WB04-E100)
    #state;
    // Boardcoord.cpp 의 1f9/262 진단 호출이다(해당 분기 도달 때만 필요). (2026-11-08 WB04-E100)
    #assertion;

    // 좌표를 별도 보정하거나 Graph getter 를 요구하지 않는다. (2026-11-08 WB04-E100)
    constructor(state, assertion = null) {
        this.#state = state;
        this.#assertion = assertion;
    }

    // 41d400: 원본은 사각형을 복사해 확장하며 결과를 지도 범위로 clamp 하지 않는다. (2026-11-08 WB04-E100)
    side(rectangle, side, flags) {
        let { left, top, right, bottom } = rectangle;
        if (flags & 4) {
            left = fround(left - EXPAND);
            top = fround(top - EXPAND);
            right += EXPAND;
            bottom = fround(bottom + EXPAND);
        }
        if (side === 0 || side === 4) return { x: fround((right - left) * HALF + left), y: side === 0 ? top : bottom };
        if (side === 2 || side === 6) return { x: side === 2 ? fround(right) : left, y: fround((bottom - top) * HALF + top) };
        if (typeof this.#assertion !== 'function') throw new Error('사각형 변 중간점 원본 진단 미연결');
        this.#assertion(0x1f9);
        const value = flagFloat(flags);
        return { x: value, y: value };
    }

    // 41d530: flags2 비교는 순서 없는 값도 동쪽으로 가므로 <= 조건으로 바꾸지 않는다. (2026-11-08 WB04-E100)
    nearest(rectangle, source, flags) {
        const { left, top, right, bottom } = rectangle;
        const x = fround((left + right) * HALF);
        const y = fround((bottom + top) * HALF);
        let side = ((RawSquidNeighbors.direction(source.x, source.y, x, y) - 4) >>> 0) & 7;
        if ((flags & 2) && side === 0) side = source.x < (left + right) * HALF ? 6 : 2;
        if (flags & 1) return this.side(rectangle, side, flags);
        if (typeof this.#assertion !== 'function') throw new Error('일반 접근점 원본 진단 미연결');
        this.#assertion(0x262);
        return { x: this.#state.invalid.x, y: this.#state.invalid.y };
    }
}
