// 썬 바리케이드의 내부 표식(fencemark, 00456460)과 화면의 포스 필드(00455b20 mode 1)를 생성하는 두 부모를 옮긴 모듈이다(cpppj `o/RawFenceCreate.cpp/.h`, 10.78 전용). 일반 객체 생성과 화면 효과 경계는 외부(훅)에 두고
// 원본 좌표/태그/호출 순서를 유지한다.
//  - RawFenceMarkerCreate(pool, target, type, hooks): `points(centers)` 는 두 끝점 중심(float 4개)에서 원본 방향(동률이면 세로 우선)·포함 사각형·4칸(반올림 후 /4, 63 상한) 구간 변경 뒤의 현재 float 지점이다.
//    `create(parent, other)` 는 끝점 중심을 구해 지점마다 새 fencemark 를 만든다(create → 부모 번호를 +12 WORD 에 QA 로 쓰고 → 표면 통지 → 이웃 번호의 high8 을 +26, low8 을 +30 에 QB 로 쓰고 → 부모의 현재 소유자로 지정 → flags 0x40 Pop).
//  - RawFenceShieldCreate(pool, frames, type, hooks): `points(endpoints)` 는 mode 0 과 같은 보정이나 정수 절삭 전 float 지점이며 100개 상한에 끝점을 한 번 포함한다. `create(endpoints, firstOwner, secondOwner, firstId, secondId)` 는 수직이면 J 수평이면 K
//    구간의 첫 프레임을 +36 에 직접 쓰고(생성 직후 extra bit20 이면 즉시 종료), 소유자는 두 소유자가 같거나 둘째 SID(signed)가 더 작으면 첫째, 아니면 둘째이며 flags 0x40 Pop 한다(표면 통지는 이 경로에서 부르지 않음).
// 현재 타입 전역(type)은 각 객체 생성 직전에 다시 읽는 getter 이다. 훅 = { create(type, 2)→SID, notifySurface(sid), setOwner(sid, owner), pop(sid, x, y, flags) } 이며 선택한 소비자의 누락만 거부하고 앞선 쓰기/생성은 되돌리지 않는다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E83: 썬 바리케이드 표식/포스 필드 생성 이식으로 추가)

// 0043fa50 이 현재 float 에 더한 뒤 CRT 절삭하는 반올림 편향, 표식 4칸 버킷 상한, 포스 필드의 100개 상한이다. (2026-11-08 WB04-E83)
const MARKER_ROUND_BIAS = Math.fround(0.5);
const MARKER_CELL_LAST = 63;
const SHIELD_LIMIT = 100;
// 슬롯의 소유자 BYTE(+34)·extra BYTE(+40)·TYPE(+10)·프레임 DWORD(+36)·QA WORD(+12)·QB high(+26)/low(+30) 위치와 생성 Pop 의 flags 이다. (2026-11-08 WB04-E83)
const OWNER = 34;
const EXTRA = 40;
const TYPE = 10;
const FRAME = 36;
const QA = 12;
const QB_HIGH = 26;
const QB_LOW = 30;
const SHIELD_BLOCK = 0x20;
const POP_FLAGS = 0x40;
const fround = Math.fround;

// 선택한 실제 소비자만 요구하고 앞선 쓰기/생성을 되돌리지 않는다. (2026-11-08 WB04-E83)
function need(callback) {
    if (typeof callback !== 'function') throw new Error('펜스 생성의 실제 하위 미연결');
    return callback;
}

// 원본 지정 폭의 비정렬 필드만 쓴다(낮은 바이트부터). (2026-11-08 WB04-E83)
function put(raw, offset, value, width = 4) {
    for (let index = 0; index < width; index += 1) raw[offset + index] = (value >>> (index * 8)) & 0xff;
}

// 유한 정상 보드 좌표의 원본 CRT signed 절삭이다. (2026-11-08 WB04-E83)
function toInt(value) {
    if (!Number.isFinite(value) || value < -2147483648 || value >= 2147483648) throw new RangeError('펜스 생성 CRT 좌표 범위 오류');
    return Math.trunc(value);
}

// 원본 정상 좌표의 방향 0/2/4/6 을 선택하며 동률은 세로를 우선한다(이동은 ±1 칸). (2026-11-08 WB04-E83)
function markerStep(points) {
    const dx = fround(points[2] - points[0]);
    const dy = fround(points[3] - points[1]);
    if (Math.abs(dx) <= Math.abs(dy)) return [0, dy > 0 ? 1 : -1];
    return [dx <= 0 ? -1 : 1, 0];
}

// 모든 외부 좌표는 실제 Pop 계약과 같은 유한 입력이어야 한다. (2026-11-08 WB04-E83)
function requireFinite(points) {
    for (const coordinate of points) if (!Number.isFinite(coordinate)) throw new RangeError('펜스 생성의 비유한 좌표');
}

// 보드 안쪽 [1, 255] 로 들어오는 corner 를 clamp 한다. (2026-11-08 WB04-E83)
const clamp = (value) => (value < 1 ? 1 : 255 < value ? 255 : value);

export class RawFenceMarkerCreate {
    // 같은 PATCH 풀·현재 중심 조회(RawGunTarget)·현재 fencemark 타입 getter·실제 생성/표시 소비자이다.
    #pool;
    #target;
    #type;
    #hooks;

    // 현재 프레임/중심의 다른 월드를 섞지 않는다. (2026-11-08 WB04-E83)
    constructor(pool, target, type, hooks) {
        if (pool?.edition !== 'patch1078' || target?.pool !== pool || typeof type !== 'function') throw new TypeError('fencemark 생성 판본/풀 불일치');
        this.#pool = pool;
        this.#target = target;
        this.#type = type;
        this.#hooks = hooks ?? {};
    }

    // 스냅한 구간은 이동 전 위치에서 구하고 생성 좌표는 한 걸음 이동한 뒤의 float 이다. centers = [부모 x, 부모 y, 이웃 x, 이웃 y]. (2026-11-08 WB04-E83)
    static points(centers) {
        requireFinite(centers);
        const step = markerStep(centers);
        const incomingX = clamp(centers[2]);
        const incomingY = clamp(centers[3]);
        const valid = centers[0] > 0 && centers[1] > 0 && centers[0] < 256 && centers[1] < 256;
        const bounds = [
            valid ? Math.min(centers[0], incomingX) : incomingX, valid ? Math.min(centers[1], incomingY) : incomingY,
            valid ? Math.max(centers[0], incomingX) : incomingX, valid ? Math.max(centers[1], incomingY) : incomingY
        ];
        // x87 원본은 경계 동일점도 허용한다.
        const inside = (point) => bounds[0] <= point[0] && point[0] <= bounds[2] && bounds[1] <= point[1] && point[1] <= bounds[3];
        let current = [centers[0], centers[1]];
        let previous = [-1, -1];
        const result = [];
        // 원본은 보드 사각형을 벗어날 때 종료한다.
        while (inside(current)) {
            const cell = [Math.min(Math.trunc(toInt(current[0] + MARKER_ROUND_BIAS) / 4), MARKER_CELL_LAST), Math.min(Math.trunc(toInt(current[1] + MARKER_ROUND_BIAS) / 4), MARKER_CELL_LAST)];
            current = [fround(current[0] + step[0]), fround(current[1] + step[1])];
            if (!inside(current)) break;
            if (cell[0] !== previous[0] || cell[1] !== previous[1]) {
                previous = cell;
                result.push([current[0], current[1]]);
            }
        }
        return result;
    }

    // 원본 생성자는 QA WORD 를 쓴 뒤 통지하고 끝점의 high8/low8 을 분리해서 기록한다. (2026-11-08 WB04-E83)
    create(parent, other) {
        const second = this.#target.center(other);
        const first = this.#target.center(parent);
        const hooks = this.#hooks;
        // 부모/대상 좌표의 이후 변경은 이미 정해진 로컬 이동 경로를 바꾸지 않는다.
        for (const point of RawFenceMarkerCreate.points([first[0], first[1], second[0], second[1]])) {
            const created = need(hooks.create)(this.#type(), 2);
            if (!created) throw new Error('fencemark 생성의 실제 새 SID 누락');
            put(this.#pool.allocated(created), QA, parent, 2);
            need(hooks.notifySurface)(created);
            put(this.#pool.allocated(created), QB_HIGH, (other >>> 8) & 255);
            this.#pool.allocated(created)[QB_LOW] = other & 0xff;
            need(hooks.setOwner)(created, this.#pool.slot(parent)[OWNER]);
            need(hooks.pop)(created, point[0], point[1], POP_FLAGS);
        }
    }
}

export class RawFenceShieldCreate {
    // 생성 포스 필드의 현재 extra/TYPE/프레임을 읽는 풀, 현재 TYPE 의 J/K 첫 프레임 표(188), 현재 fenceShield 타입 getter, 실제 생성/소유자/Pop 소비자이다.
    #pool;
    #frames;
    #type;
    #hooks;

    // 두 판본의 포스 필드 생성자를 동일하게 취급하지 않는다. (2026-11-08 WB04-E83)
    constructor(pool, frames, type, hooks) {
        if (pool?.edition !== 'patch1078' || frames?.length !== 188 || typeof type !== 'function') throw new TypeError('포스 필드 생성 판본/TYPE 불일치');
        this.#pool = pool;
        this.#frames = frames;
        this.#type = type;
        this.#hooks = hooks ?? {};
    }

    // 원본 mode 1 은 좌표를 정수로 바꾸지 않고 현재 float 를 그대로 가상 Pop 에 전달한다. 원본 로컬 float 의 현재 끝점/증분 보정 뒤 100회 상한에 끝점을 한 번 포함한다. (2026-11-08 WB04-E83)
    static points(endpoints) {
        requireFinite(endpoints);
        let x = fround(endpoints[0] - 1);
        let y = fround(endpoints[1] - 1);
        let endX = fround(endpoints[2] - 1);
        let endY = fround(endpoints[3] - 1);
        let dx = 0;
        let dy = 0;
        if (x === endX) {
            if (endY <= y) { dy = -1; y = fround(y - 3); if (y < endY) endY = y; } else { dy = 1; endY = fround(endY - 3); if (endY <= y) y = endY; }
            if (x <= endX) x = endX;
            if (dy > 0) { if (endY <= y) y = endY; } else if (y <= endY) y = endY;
        } else {
            if (endX <= x) { x = fround(x + 1); dx = -1; endX = fround(endX - 1); } else { x = fround(x - 1); dx = 1; endX = fround(endX + 1); }
            y = fround(y - 1);
            endY = fround(endY - 1);
            x = fround(fround(dx + dx) + x);
            endX = fround(endX - (dx + dx));
            if (dx <= 0) { if (x <= endX) x = endX; } else if (endX <= x) x = endX;
            if (y <= endY) y = endY;
        }
        const result = [];
        // 끝점은 한 번 포함하고 그 뒤 종료한다.
        for (let remaining = SHIELD_LIMIT; remaining; remaining -= 1) {
            result.push([x, y]);
            if (x === endX && y === endY) break;
            x = fround(x + dx);
            y = fround(y + dy);
        }
        return result;
    }

    // 직접 프레임 쓰기와 생성 직후 bit20 의 조기 종료를 유지한다. firstId/secondId 는 signed 정수이다. (2026-11-08 WB04-E83)
    create(endpoints, firstOwner, secondOwner, firstId, secondId) {
        const hooks = this.#hooks;
        // 수직 J / 수평 K 의 실제 TYPE 첫 프레임이다.
        const letter = endpoints[0] === endpoints[2] ? 'J' : 'K';
        for (const point of RawFenceShieldCreate.points(endpoints)) {
            // 현재 타입 전역은 각 객체 생성 직전에 다시 읽는다.
            const created = need(hooks.create)(this.#type(), 2);
            if (!created) throw new Error('포스 필드 생성의 실제 새 SID 누락');
            const raw = this.#pool.slot(created);
            if (raw[EXTRA] & SHIELD_BLOCK) return;
            if (raw[TYPE] >= this.#frames.length) throw new RangeError('포스 필드 생성의 현재 TYPE 범위 오류');
            put(this.#pool.allocated(created), FRAME, this.#frames[raw[TYPE]].run(letter).first >>> 0);
            need(hooks.setOwner)(created, firstOwner === secondOwner || (secondId | 0) < (firstId | 0) ? firstOwner : secondOwner);
            need(hooks.pop)(created, point[0], point[1], POP_FLAGS);
        }
    }
}
