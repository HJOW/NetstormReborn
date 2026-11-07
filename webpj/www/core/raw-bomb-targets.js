// 주문 효과의 반경 정수화와 대상 검색 41e4b0(41e3c0 기본 반경, 41e460 중심)을 옮긴 모듈이다(cpppj `o/RawBombTargets.cpp/.h`, 10.78 전용).
//  - defaultRadius(parent): 현재 TYPE+44 의 float 반경을 _ftol 하위 DWORD(signed)로 절삭한다. 파생의 가상 b4 분배는 호출자가 확인한다.
//  - center(parent): 부모 좌표를 +0.99999 뒤 정수 절삭(스냅)하고 0.5 를 뺀 뒤 1..255 로 제한한 캡처 중심이다(폭발과 검색이 공유).
//  - find(parent, output): 권한(boss)이 없으면 복귀 가능한 진단 0x130 을 먼저 부르고, 중심과 가상 b4 반경(signed 정수)을 순회 전에 캡처한 뒤 [중심−반경, 중심+반경] 사각형(왼쪽/위는 절삭, 오른쪽/아래는 +0.99999 스냅)을
//    실제 RawSquidFinder 네 단계 해시 탐색으로 순회한다. 예약 번호(5..14999)는 건너뛰고, 후보의 현재 TYPE 발자국·상태(extra 비트 1, 상태 비트 2)를 걸러 `(좌표 − 발자국/2)` 와 중심 거리가 `발자국/2 + 반경` 이하이면
//    파생 자격(가상 bc)을 묻고 출력 헤더의 현재 count < capacity(signed)일 때만 실제 버퍼에 후보 SID 를 저장한다. 순회 중 바뀐 후보 TYPE/좌표/상태/출력 헤더는 원본 위치에서 다시 읽는다.
//  - 반환 기하 { center, radius, area }: 캡처한 중심 2개·signed 반경·정수 finder 변환 전 float 사각형 4개이다.
// 훅 { radius(parent, 가상 표), eligible(parent, 가상 표, 후보), assertion(줄) } 는 도달한 지점에서만 요구한다(없으면 TypeError). 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-08 WB04-E54: RawBombTargets 이식으로 추가)
import { RawSquidFinder } from './raw-squid-finder.js';

// 슬롯 필드 위치와 개수, 예약 SID 범위, 진단 줄 번호이다. (2026-11-08 WB04-E54)
const TYPE_OFFSET = 10;
const STATE_OFFSET = 11;
const X_OFFSET = 14;
const Y_OFFSET = 18;
const EXTRA_OFFSET = 40;
const TYPE_COUNT = 188;
const RESERVED_FIRST = 5;
const SERVER_FIRST = 15000;
const BOSS_ASSERTION_LINE = 0x130;

// 41d7a0 이 더하는 원본 502310 의 float 값과 _ftol 의 유효 signed64 상한이다. 다른 탐색기의 0.9999 와 구별한다. (2026-11-08 WB04-E54)
const SNAP_BIAS = Math.fround(0.9999899864196777);
const SIGNED64_LIMIT = 9223372036854775808;

// 정렬되지 않은 float 4바이트를 읽는다. (2026-11-08 WB04-E54)
const floatView = new DataView(new ArrayBuffer(4));
function readFloat(raw, offset) {
    floatView.setUint32(0, (raw[offset] | (raw[offset + 1] << 8) | (raw[offset + 2] << 16) | (raw[offset + 3] << 24)) >>> 0, true);
    return floatView.getFloat32(0, true);
}

// 원본 signed64 절삭의 하위 DWORD 를 signed32 로 해석한다. 비유한/범위 밖은 0 이다. (2026-11-08 WB04-E54)
function truncate(value) {
    if (!(value >= -SIGNED64_LIMIT && value < SIGNED64_LIMIT)) return 0;
    return Math.trunc(value) | 0;
}

// 41d770/41d7a0: 절삭된 signed32 를 float 에 다시 저장한다(upper 면 +0.99999 를 먼저 더한다). (2026-11-08 WB04-E54)
function snap(value, upper) {
    return Math.fround(truncate(value + (upper ? SNAP_BIAS : 0.0)));
}

// 부모 좌표는 스냅 뒤에 0.5 를 빼고 원본 41d0d0 의 1..255 로 제한한다. (2026-11-08 WB04-E54)
function snapCenter(value) {
    return Math.min(Math.max(Math.fround(snap(value, true) - 0.5), 1.0), 255.0);
}

// 선택한 파생 몸체 누락을 무동작 성공으로 바꾸지 않는다. (2026-11-08 WB04-E54)
function need(hook) {
    if (typeof hook !== 'function') throw new TypeError('Bomb 대상 실제 하위 미연결');
    return hook;
}

// 주문 대상 검색 실행기이다. 사용: new RawBombTargets(pool, hash, types, ranges, { boss }, { radius, eligible, assertion }); targets.find(sid, { storage: new Uint32Array(100), capacity: 100, count: 0 }). (2026-11-08 WB04-E54)
export class RawBombTargets {
    #pool;
    #hash;
    #types;
    #ranges;
    #state;
    #hooks;

    // 현재 10.78 풀/188TYPE/반경 표/해시만 받는다. 훅은 생성 시점의 사본을 쓴다. (2026-11-08 WB04-E54)
    constructor(pool, hash, types, ranges, state, hooks) {
        if (pool?.edition !== 'patch1078' || types?.length !== TYPE_COUNT || ranges?.length !== types.length || typeof state !== 'object' || state === null) throw new TypeError('Bomb 대상 판본/TYPE 오류');
        this.#pool = pool;
        this.#hash = hash;
        this.#types = types;
        this.#ranges = ranges;
        this.#state = state;
        this.#hooks = { ...hooks };
    }

    // 같은 월드의 물리 저장소인지 확인하는 풀 참조이다. (2026-11-08 WB04-E54)
    get pool() {
        return this.#pool;
    }

    // 효과 뒤 같은 후보의 종류가 바뀔 수 있으므로 현재 번호를 다시 확인해 TYPE 레코드를 돌려준다. (2026-11-08 WB04-E54)
    #type(sid) {
        const kind = this.#pool.slot(sid)[TYPE_OFFSET];
        if (kind >= this.#types.length) throw new RangeError('Bomb 대상 현재 TYPE 범위');
        return this.#types[kind];
    }

    // 물리 필드의 원본 폭(바이트)만 읽고 상태 필터를 추가하지 않는다. (2026-11-08 WB04-E54)
    #read(sid, offset, width = 4) {
        const raw = this.#pool.slot(sid);
        let value = 0;
        // 낮은 바이트부터 지정한 폭을 조합한다.
        for (let index = 0; index < width; index += 1) value += raw[offset + index] * 2 ** (8 * index);
        return value;
    }

    // 41e3c0: 현재 TYPE 의 실제 반경을 정수화한다(caller 의 가상 분배를 대체하지 않는다). (2026-11-08 WB04-E54)
    defaultRadius(parent) {
        const kind = this.#pool.slot(parent)[TYPE_OFFSET];
        if (kind >= this.#ranges.length) throw new RangeError('Bomb 반경 현재 TYPE 범위');
        return truncate(this.#ranges[kind]);
    }

    // 41e460: 대상 검색과 폭발이 같은 원본 중심 계산을 재사용한다. (2026-11-08 WB04-E54)
    center(parent) {
        const raw = this.#pool.slot(parent);
        return [snapCenter(readFloat(raw, X_OFFSET)), snapCenter(readFloat(raw, Y_OFFSET))];
    }

    // 중심/반경은 순회 전에 캡처하고 후보와 출력 헤더는 원본 위치에서 현재 값을 읽는다. 출력 { storage, capacity, count } 는 signed 헤더이며 제자리 갱신한다. (2026-11-08 WB04-E54)
    find(parent, output) {
        const hooks = this.#hooks;
        if (!this.#state.boss) need(hooks.assertion)(BOSS_ASSERTION_LINE);
        const center = this.center(parent);
        const radius32 = need(hooks.radius)(parent, this.#read(parent, 0)) | 0;
        // 원본은 첫 upperX 계산 뒤부터 저장된 float 반경을 읽는다.
        const radius = Math.fround(radius32);
        const geometry = {
            center,
            radius: radius32,
            area: [Math.fround(center[0] - radius), Math.fround(center[1] - radius), Math.fround(radius32 + center[0]), Math.fround(radius + center[1])],
        };
        const area = { left: truncate(snap(geometry.area[0], false)), top: truncate(snap(geometry.area[1], false)), right: truncate(snap(geometry.area[2], true)), bottom: truncate(snap(geometry.area[3], true)) };
        const finder = new RawSquidFinder(this.#pool, this.#hash, this.#types);
        // 실시간 머리/next·발자국·buried 는 원본과 같은 네 단계 탐색기가 처리한다.
        for (let candidate = finder.begin(area); candidate; candidate = finder.next()) {
            if (candidate >= RESERVED_FIRST && candidate < SERVER_FIRST) continue;
            const type = this.#type(candidate);
            const allowance = Math.fround(type.footX * 0.5 + radius);
            if ((this.#read(candidate, EXTRA_OFFSET, 1) & 1) || (this.#read(candidate, STATE_OFFSET, 1) & 2)) continue;
            const x = Math.fround(readFloat(this.#pool.slot(candidate), X_OFFSET) - type.footX * 0.5);
            const y = Math.fround(readFloat(this.#pool.slot(candidate), Y_OFFSET) - type.footY * 0.5);
            const dx = center[0] - x;
            const dy = center[1] - y;
            if (!(Math.sqrt(dx * dx + dy * dy) <= allowance)) continue;
            if (!need(hooks.eligible)(parent, this.#read(parent, 0), candidate)) continue;
            if (output.count < output.capacity) {
                if (output.count < 0 || output.count >= output.storage.length) throw new RangeError('Bomb 대상 실제 출력 버퍼 범위');
                output.storage[output.count] = candidate;
                output.count += 1;
            }
        }
        return geometry;
    }
}
