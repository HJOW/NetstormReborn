// 썬 바리케이드의 연결 재생성 제어(00457460)를 옮긴 모듈이다(cpppj `o/RawFenceRebuild.cpp/.h`, 10.78 전용). 현재 동맹 이웃/사거리/재생성 호출 순서를 처리한다.
// 부모 extra 비트 0·3(9)이 있으면 아무것도 하지 않는다(진입 때만 검사). 없으면 네 방향(0·2·4·6)을 순서대로: 동맹 최근접 이웃(neighbor) 조회 → 없으면 다음 방향 → 부모와 이웃의 **현재** 두 좌표(float 4개)로 사거리 판정(withinRange)
//   → 사거리 밖이거나 이웃의 **현재** extra 가 0x21 비트를 가지면 다음 방향 → 내부 표식 생성(markers) → 표식 생성 효과 뒤의 현재 소유자들과 현재 좌표로 포스 필드 생성(shields: 좌표 4개, 부모 소유자, 이웃 소유자, 부모 SID, 이웃 SID)을 부른다.
// 형식화한 좌표 사본을 오래 보관하지 않고 각 호출 직전에 현재 raw 를 읽는다. 선택하지 않은 경계는 요구하지 않고 선택한 경계의 누락만 거부한다.
// 훅 = { neighbor(부모, 방향), withinRange(부모, 좌표4), markers(부모, 이웃), shields(좌표4, 부모 소유자, 이웃 소유자, 부모 SID, 이웃 SID) } 이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E81: 썬 바리케이드 재생성 이식으로 추가)

// 슬롯의 소유자 BYTE(+34)·extra BYTE(+40)·좌표 float(+14/+18) 위치, 부모 extra 의 진입 차단 비트(9)와 이웃 extra 의 건너뜀 비트(0x21)이다. (2026-11-08 WB04-E81)
const OWNER = 34;
const EXTRA = 40;
const X_AT = 14;
const Y_AT = 18;
const PARENT_BLOCK = 9;
const NEIGHBOR_SKIP = 0x21;

// 선택하지 않은 경계는 요구하지 않으며 선택된 누락만 거부한다. (2026-11-08 WB04-E81)
function need(callback) {
    if (typeof callback !== 'function') throw new Error('썬 바리케이드 재생성의 실제 하위 미연결');
    return callback;
}

// 네 BYTE float 의 비정렬 원본 비트를 읽는다(NaN payload 보존). (2026-11-08 WB04-E81)
const view = new DataView(new ArrayBuffer(4));
function coordinate(raw, offset) {
    for (let index = 0; index < 4; index += 1) view.setUint8(index, raw[offset + index]);
    return view.getFloat32(0, true);
}

export class RawFenceRebuild {
    // PATCH 현재 풀과 네 방향을 처리하는 실제 소비자(훅)이다.
    #pool;
    #hooks;

    // CD 의 다른 타입 배치를 PATCH 로 취급하지 않는다. (2026-11-08 WB04-E81)
    constructor(pool, hooks) {
        if (pool?.edition !== 'patch1078') throw new TypeError('썬 바리케이드 재생성 판본 오류');
        this.#pool = pool;
        this.#hooks = hooks ?? {};
    }

    // 호출 직전의 현재 두 좌표(부모 x, y, 이웃 x, y)이다. (2026-11-08 WB04-E81)
    #endpoints(parent, other) {
        const a = this.#pool.slot(parent);
        const b = this.#pool.slot(other);
        return [coordinate(a, X_AT), coordinate(a, Y_AT), coordinate(b, X_AT), coordinate(b, Y_AT)];
    }

    // 동맹 이웃→거리→대상 extra→표식→현재 포스필드 인수의 순서이다. 부모 extra 는 진입 때만 검사하고 하위 효과 뒤 대상 extra/owner/좌표는 다시 읽는다. (2026-11-08 WB04-E81)
    rebuild(parent) {
        const pool = this.#pool;
        const hooks = this.#hooks;
        if (pool.slot(parent)[EXTRA] & PARENT_BLOCK) return;
        // 원본 네 방향 0/2/4/6 의 순서이다.
        for (let direction = 0; direction < 8; direction += 2) {
            const other = need(hooks.neighbor)(parent, direction);
            if (!other) continue;
            const within = need(hooks.withinRange)(parent, this.#endpoints(parent, other));
            if (!within || (pool.slot(other)[EXTRA] & NEIGHBOR_SKIP)) continue;
            need(hooks.markers)(parent, other);
            // 표식 생성 뒤 현재 소유자들이다.
            const owners = [pool.slot(parent)[OWNER], pool.slot(other)[OWNER]];
            need(hooks.shields)(this.#endpoints(parent, other), owners[0], owners[1], parent, other);
        }
    }
}
