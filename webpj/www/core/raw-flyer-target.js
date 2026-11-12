// 비행체의 표적 검색·의존 경쟁 수·포위 위치를 같은 현재 월드에서 계산하는 모듈이다(cpppj `o/RawFlyerTarget.cpp/.h`, 10.78 전용: 0045c4b0 표적 검색·0045b840 가까운 비행체 수·0045b9c0 자리 수·0045ba10 자리 번호·0045bb80 포위 위치).
//  - new RawFlyerTarget(pool, types, ranges, globals, hooks): pool 은 같은 PATCH SidPool, types = 188칸 TYPE({ footX, footY, flags2 })의 **살아 있는 참조**(원문도 span 이며 호출 사이에 바뀐 값을 읽는다), ranges = 188칸 TYPE+44 사거리(float, 만들 때 사본),
//    globals = { server(), checking(), now(), sunType(), processEnd(), dependType() } 는 모두 현재 값을 읽는 함수이고,
//    hooks = { assertion(line), checkForms(sid), offset(sid)→[x, y], begin(area, flags)→SID, next()→SID, mayTarget(parent, target)→bool, watcher(form)→SID, processParent(watcher)→SID, returnDelay(fields)→초, command(sid, command) } 이다.
//    area = { left, top, right, bottom } 정수 사각형이다. 선택하지 않은 경계는 요구하지 않으며 선택된 누락은 앞선 쓰기를 보존하고 거부한다.
//  - find(fields): 0045c4b0 — 서버가 아니면 진단 0x3af, 부모 종속 검사 0x3b7, 부모 위치에서 이동 보정을 뺀 출발점(처음이면 정수로 절삭해 부모 +42/+46 에 저장, 아니면 저장된 정수), 표적이 이미 있으면 0x3d5 뒤 해제,
//    출발점 ± 사거리의 사각형(1..255 로 자른 뒤 확장; 오른쪽/아래에 float 0.99999 보정 후 정수 절삭)으로 일반 finder 를 돌리며 필터(발사 가능·flags2 0x120000·홈·훨리기그의 0x210000)·자리(가까운 비행체 수 < 자리 수)·사거리·엄격한 최소 거리(첫 동률 유지)를 통과한
//    마지막 후보를 표적으로 쓰고 dirty |= 4, 훨리기그이고 귀환 기한이 0 이면 복귀 지연과 명령 2 를 낸다. 마지막에 종속 검사 0x413.
//  - closerCount(target, flyer): 0045b840 — 표적이 비행체(자유/사망 아님)일 때 의존 Form 의 상대 프로세스 부모 중 이 비행체가 아닌 비행체로서 표적 중심에 더 가까운 수. capacity(target): 0045b9c0 — 죽지 않은 1×1 발자국 표적 1칸, 그 밖 3칸.
//    slotIndex(target, flyer): 0045ba10 — 자기 의존 Form 을 찾고(없으면 checking 일 때 0x374, 0) 다시 돌며 더 작은 Form 번호의 비행체 수. scatter(target, flyer): 0045bb80 — 자리 1 이면 (0, 0), 아니면 [(0, −1), (−1, 1), (1, 1)] 중 자리 번호.
// 10.72(CD)는 범위 밖이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-13 WB04-E356: 7-P1 RawFlyerTarget 이식으로 추가)

// 이 모듈이 쓰는 raw 객체와 outer 필드의 BYTE 위치(종속 head·TYPE·상태·좌표·dirty·표적·귀환 기한)이다. (2026-11-13 WB04-E356)
const HEAD = 6, TYPE = 10, STATE = 11, X = 14, Y = 18, DIRTY = 8, TARGET = 12, RETURN = 40;
// 원본 00502310 의 오른쪽/아래 절삭 전 보정(float 0.99999)이며 ceil 로 대신하지 않는다. (2026-11-13 WB04-E356)
const RIGHT_BIAS = Math.fround(0.99999);
// PE 00532794 의 실제 포위 순서이다. (2026-11-13 WB04-E356)
const SCATTER = [[0, -1], [-1, 1], [1, 1]];
// TYPE 표 크기(PATCH)이다. (2026-11-13 WB04-E356)
const TYPE_COUNT = 188;

// 비정렬 값을 리틀엔디언 부호 없는 수로 읽는다. (2026-11-13 WB04-E356)
function read(raw, at, width = 4) {
    let value = 0;
    // 필드 폭 밖의 padding 을 읽지 않는다.
    for (let index = 0; index < width; index += 1) value += raw[at + index] * 2 ** (8 * index);
    return value;
}

// 지정 폭만 쓴다(다른 payload 를 건드리지 않음). (2026-11-13 WB04-E356)
function put(raw, at, value, width = 4) {
    // 낮은 바이트부터 원문 순서로 기록한다.
    for (let index = 0; index < width; index += 1) raw[at + index] = Math.floor(value / 2 ** (8 * index)) & 0xff;
}

// float/double 비트 변환용 공용 보기이다. (2026-11-13 WB04-E356)
const SCRATCH = new DataView(new ArrayBuffer(8));

// 원문 좌표 float 의 IEEE 비트를 읽는다. (2026-11-13 WB04-E356)
function float(raw, at) {
    SCRATCH.setUint32(0, read(raw, at), true);
    return SCRATCH.getFloat32(0, true);
}

// 실제 ftol 의 signed64 절삭 뒤 하위 DWORD(int32)를 보존한다. 비유한·범위 밖 CRT 입력은 명시 거부한다. (2026-11-13 WB04-E356)
function truncate(value) {
    if (!Number.isFinite(value) || value < -9223372036854775808 || value >= 9223372036854775808) throw new RangeError('비행 표적 좌표 ftol 범위 오류');
    return Number(BigInt.asIntN(32, BigInt(Math.trunc(value))));
}

// 필드의 raw double 을 비트 그대로 읽는다. (2026-11-13 WB04-E356)
function time(fields, at) {
    SCRATCH.setUint32(0, read(fields.bytes, at), true);
    SCRATCH.setUint32(4, read(fields.bytes, at + 4), true);
    return SCRATCH.getFloat64(0, true);
}

// double 의 두 DWORD 만 대입한다. (2026-11-13 WB04-E356)
function putTime(fields, at, value) {
    SCRATCH.setFloat64(0, value, true);
    put(fields.bytes, at, SCRATCH.getUint32(0, true));
    put(fields.bytes, at + 4, SCRATCH.getUint32(4, true));
}

// Point.distance: float 입력 사이의 차이는 중간 float 저장 없이 double 로 계산한다. (2026-11-13 WB04-E356)
function distance(first, second) {
    const x = first[0] - second[0];
    const y = first[1] - second[1];
    return Math.sqrt(x * x + y * y);
}

// 원본 Point.isValid 는 0/256 경계와 비유한 좌표를 제외한다. (2026-11-13 WB04-E356)
function valid(point) {
    return point[0] > 0 && point[1] > 0 && point[0] < 256 && point[1] < 256;
}

// C++ std::min/std::max 의 비교 방향이다(NaN 이면 앞 인자를 유지). (2026-11-13 WB04-E356)
const stdMin = (first, second) => (second < first ? second : first);
const stdMax = (first, second) => (first < second ? second : first);

// Rect 확장은 추가 점을 먼저 1..255 로 자른 뒤 기존 꼭짓점의 유효성에 따라 교체한다. (2026-11-13 WB04-E356)
function include(rect, point) {
    const clamped = [point[0], point[1]];
    // 두 축의 원문 clamp 순서를 보존한다.
    for (let axis = 0; axis < 2; axis += 1) {
        clamped[axis] = clamped[axis] >= 1 ? clamped[axis] : 1;
        if (clamped[axis] > 255) clamped[axis] = 255;
    }
    if (!valid([rect[0], rect[1]])) {
        rect[0] = clamped[0];
        rect[1] = clamped[1];
    } else {
        rect[0] = stdMin(rect[0], clamped[0]);
        rect[1] = stdMin(rect[1], clamped[1]);
    }
    if (!valid([rect[2], rect[3]])) {
        rect[2] = clamped[0];
        rect[3] = clamped[1];
    } else {
        rect[2] = stdMax(rect[2], clamped[0]);
        rect[3] = stdMax(rect[3], clamped[1]);
    }
}

// 선택된 미복원 하위를 빈 성공으로 바꾸지 않고 앞선 원문 쓰기를 보존한다. (2026-11-13 WB04-E356)
function need(hook) {
    if (typeof hook !== 'function') throw new Error('비행 표적 실제 하위 몸체 미연결');
    return hook;
}

// 의존 finder 가 후보를 반환하기 전에 저장하는 next 와 타입 필터를 같은 풀에서 실행하는 커서이다. (2026-11-13 WB04-E356)
class DependCursor {
    #pool;
    #depend;
    #next;
    #steps = 0;

    // 현재 부모 head 부터 시작하고, 의존 TYPE 은 각 후보에서 다시 읽는다. (2026-11-13 WB04-E356)
    constructor(pool, parent, depend) {
        this.#pool = pool;
        this.#depend = depend;
        this.#next = read(pool.slot(parent), HEAD, 2);
    }

    // 다음 의존 Form 을 돌려준다(없으면 0). 원본은 free/dead 상태를 추가로 배제하지 않고, 손상된 정상 범위 밖 무한 체인만 거부한다. (2026-11-13 WB04-E356)
    next() {
        const pool = this.#pool;
        while (this.#next) {
            this.#steps += 1;
            if (this.#next >= pool.capacity || this.#steps > pool.capacity) throw new Error('비행 의존 finder 범위/순환 오류');
            const current = this.#next;
            const raw = pool.slot(current);
            this.#next = read(raw, 4, 2);
            if (raw[TYPE] <= 69 && raw[TYPE] === this.#depend()) return current;
        }
        return 0;
    }
}

// 비행체 표적 검색/경쟁/포위 로직이다. (2026-11-13 WB04-E356)
export class RawFlyerTarget {
    #pool;
    #types;
    #ranges;
    #globals;
    #hooks;

    // PATCH 와 188칸 TYPE/사거리 표·현재 전역 조회기를 확인한다. TYPE 표는 살아 있는 참조, 사거리 표는 만들 때 사본이다. (2026-11-13 WB04-E356)
    constructor(pool, types, ranges, globals, hooks) {
        if (pool?.edition !== 'patch1078' || types?.length !== TYPE_COUNT || ranges?.length !== TYPE_COUNT) throw new TypeError('비행 표적은 같은 PATCH188TYPE 자료가 필요합니다');
        // 모든 전역은 처리 도중 바뀔 수 있어 값이 아니라 현재 값 조회 함수여야 한다.
        for (const name of ['server', 'checking', 'now', 'sunType', 'processEnd', 'dependType']) {
            if (typeof globals?.[name] !== 'function') throw new TypeError(`비행 표적 현재 전역 조회기 누락: ${name}`);
        }
        this.#pool = pool;
        this.#types = types;
        this.#ranges = Array.from(ranges);
        this.#globals = globals;
        this.#hooks = hooks ?? {};
    }

    // 연결된 검색기와 같은 풀인지 확인할 수 있게 공개한다. (2026-11-13 WB04-E356)
    get pool() { return this.#pool; }

    // 번호에 해당하는 현재 TYPE 기록이다(표 밖은 거부). (2026-11-13 WB04-E356)
    #type(number) {
        const type = this.#types[number];
        if (!type) throw new RangeError('비행 표적 TYPE 번호가 표 밖입니다');
        return type;
    }

    // 원문의 Parent getter 는 부모 head 가 비어 있으면 진단한 뒤 같은 포인터를 돌려준다. (2026-11-13 WB04-E356)
    #parent(fields) {
        const sid = fields.parent;
        if (!read(this.#pool.slot(sid), HEAD, 2)) need(this.#hooks.assertion)(0xa4);
        return sid;
    }

    // 현재 raw 좌표에서 실제 TYPE 발자국 절반을 뺀 중심이다. (2026-11-13 WB04-E356)
    #center(sid) {
        const raw = this.#pool.slot(sid);
        const type = this.#type(raw[TYPE]);
        return [Math.fround(float(raw, X) - type.footX * 0.5), Math.fround(float(raw, Y) - type.footY * 0.5)];
    }

    // capacity 는 free 비트를 추가 검사하지 않으며 dead 와 실제 1×1 발자국만 구별한다. (2026-11-13 WB04-E356)
    capacity(target) {
        if (!target) return 3;
        const raw = this.#pool.slot(target);
        const type = this.#type(raw[TYPE]);
        return !(raw[STATE] & 2) && type.footX === 1 && type.footY === 1 ? 1 : 3;
    }

    // 비교 기준 거리는 float 에 먼저 저장하고 다른 비행체의 거리는 double 상태로 비교한다. (2026-11-13 WB04-E356)
    closerCount(target, flyer) {
        const pool = this.#pool;
        const globals = this.#globals;
        const hooks = this.#hooks;
        if (!target || !flyer) return 0;
        if ((pool.slot(target)[STATE] & 3) || (pool.slot(flyer)[STATE] & 3)) return 0;
        const center = this.#center(target);
        const mine = Math.fround(distance(this.#center(flyer), center));
        let count = 0;
        const cursor = new DependCursor(pool, target, globals.dependType);
        // 반환 전 저장한 next 로 같은 의존 목록을 계속한다.
        for (let form = cursor.next(); form; form = cursor.next()) {
            const watcher = need(hooks.watcher)(form);
            if (!watcher) continue;
            const type = pool.slot(watcher)[TYPE];
            if (type < 10 || type >= globals.processEnd()) continue;
            const parent = need(hooks.processParent)(watcher);
            if (parent !== flyer && (this.#type(pool.slot(parent)[TYPE]).flags2 & 0x100000) && distance(this.#center(parent), center) < mine) count += 1;
        }
        return count;
    }

    // 자기 의존 Form 의 번호를 먼저 찾고 다시 시작해 더 작은 Form 번호의 비행체만 센다. (2026-11-13 WB04-E356)
    slotIndex(target, flyer) {
        const pool = this.#pool;
        const globals = this.#globals;
        const hooks = this.#hooks;
        let mine = 0;
        const first = new DependCursor(pool, target, globals.dependType);
        // 첫 검색은 상대 Form 의 가상 부모를 직접 비교한다.
        for (let form = first.next(); form; form = first.next()) {
            const watcher = need(hooks.watcher)(form);
            if (watcher && read(pool.slot(watcher), 16, 2) === flyer) {
                mine = form;
                break;
            }
        }
        if (!mine) {
            if (globals.checking() >>> 0) need(hooks.assertion)(0x374);
            return 0;
        }
        let count = 0;
        const second = new DependCursor(pool, target, globals.dependType);
        // 순회 순서 대신 실제 Form 번호의 크기로 자리를 결정한다.
        for (let form = second.next(); form; form = second.next()) {
            const watcher = need(hooks.watcher)(form);
            if (!watcher) continue;
            const type = pool.slot(watcher)[TYPE];
            if (type < 10 || type >= globals.processEnd()) continue;
            const parent = need(hooks.processParent)(watcher);
            if (!read(pool.slot(parent), HEAD, 2)) need(hooks.assertion)(0xa4);
            if ((this.#type(pool.slot(parent)[TYPE]).flags2 & 0x100000) && form < mine) count += 1;
        }
        return count;
    }

    // 자리가 하나뿐이면 (0, 0), 아니면 자리 번호의 실제 포위 오프셋이다. (2026-11-13 WB04-E356)
    scatter(target, flyer) {
        if (this.capacity(target) === 1) return [0, 0];
        const index = this.slotIndex(target, flyer);
        if (index >= 0 && index < this.capacity(target)) return SCATTER[index];
        return [0, 0];
    }

    // 현재 원문 pointer·전역 읽기 순서를 유지하며 거리 동률은 먼저 반환된 후보를 남긴다. (2026-11-13 WB04-E356)
    find(fields) {
        const pool = this.#pool;
        const globals = this.#globals;
        const hooks = this.#hooks;
        if (!globals.server()) need(hooks.assertion)(0x3af);
        const parent = this.#parent(fields);
        if (!need(hooks.checkForms)(parent)) need(hooks.assertion)(0x3b7);
        const home = fields.home;
        // 원문 raw 는 살아 있는 읽기 전용 보기라 이동 보정 효과 뒤의 +42/+46 을 본다.
        const raw = pool.physical(parent);
        const range = this.#ranges[raw[TYPE]];
        let origin = this.#center(parent);
        const offset = need(hooks.offset)(parent);
        origin = [Math.fround(origin[0] - offset[0]), Math.fround(origin[1] - offset[1])];
        if (!read(raw, 42) && !read(raw, 46)) {
            put(pool.allocated(parent), 42, truncate(origin[0]));
            put(pool.allocated(parent), 46, truncate(origin[1]));
        } else origin = [Math.fround(read(raw, 42) | 0), Math.fround(read(raw, 46) | 0)];
        if (read(fields.bytes, TARGET, 2)) need(hooks.assertion)(0x3d5);
        fields.bytes[DIRTY] |= 4;
        put(fields.bytes, TARGET, 0, 2);
        // 원본은 현재 출발점 하나로 시작한다.
        const rect = [origin[0], origin[1], origin[0], origin[1]];
        include(rect, [Math.fround(origin[0] + range), Math.fround(origin[1] + range)]);
        include(rect, [Math.fround(origin[0] - range), Math.fround(origin[1] - range)]);
        let best = Math.fround(range + range);
        const area = { left: truncate(rect[0]), top: truncate(rect[1]), right: truncate(rect[2] + RIGHT_BIAS), bottom: truncate(rect[3] + RIGHT_BIAS) };
        // 원본 일반 finder 의 같은 순서와 중복 후보를 유지한다.
        for (let target = need(hooks.begin)(area, 1); target; target = need(hooks.next)()) {
            const flags = this.#type(pool.slot(target)[TYPE]).flags2;
            if (!need(hooks.mayTarget)(fields.parent, target) || (flags & 0x120000) || target === home || ((flags & 0x210000) && pool.slot(parent)[TYPE] === globals.sunType())) continue;
            const capacity = this.capacity(target);
            if (this.closerCount(target, parent) >= capacity) continue;
            const first = this.#center(target);
            const second = this.#center(target);
            const delta = [Math.fround(first[0] - origin[0]), Math.fround(second[1] - origin[1])];
            if (!(Math.sqrt(delta[0] * delta[0] + delta[1] * delta[1]) < range)) continue;
            const measured = distance(this.#center(target), origin);
            if (!(measured < best && measured < range)) continue;
            best = Math.fround(measured);
            fields.bytes[DIRTY] |= 4;
            put(fields.bytes, TARGET, target, 2);
            if (pool.slot(this.#parent(fields))[TYPE] === globals.sunType() && time(fields, RETURN) === 0) {
                const delay = need(hooks.returnDelay)(fields);
                putTime(fields, RETURN, delay + globals.now());
                need(hooks.command)(this.#parent(fields), 2);
            }
        }
        if (!need(hooks.checkForms)(parent)) need(hooks.assertion)(0x413);
    }
}
