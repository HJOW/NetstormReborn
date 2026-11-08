// 10.78 스톤 타워 배치가 기존 Gun(캐논/아처)의 표적 교체를 통지하는 후보 수집(00469590)과 통지(0046ae80)를 옮긴 모듈이다(cpppj `o/RawGunBlocker.cpp/.h`, 10.78 전용).
// 일반 finder 의 현재 해시/체인 순서와 아처 → 네 캐논 방향 순서를 보존하며, 새 Gun 을 만들지 않고 기존 GunProcess 의 표적 지정 경계(setTarget)만 부른다.
//  - collect(owner, point, capacity = 20): 새 타워 발자국(스톤 타워 타입)의 중심을 snap(+0.99999f 절삭)한 칸을 중심으로 아처 최대 거리(archerRange) 반경의 flags 5 finder 로 group 0(아처) 후보를 순회한다. dead/free 비트는 따로 배제하지 않고,
//    extra bit0 이거나 friendlyFire 가 꺼진 상태에서 동맹이면 건너뛰며, 중심 거리가 반경 이하이고 현재 표적이 있으며 `거리 < 기존 표적까지 점수(지상 +1000)` 이고 포구→기존 표적 선분이 타워 사각형의 두 대각선 중 하나와 교차하면 목록에 넣는다.
//    이어 0·2·4·6 캐논 방향마다 타워 사각형에서 cannonRange 만큼 뻗은 ray 영역의 finder 를 새로 시작해 group 1 후보 중 현재 방향이 그 반대 방향 `(direction-4)&7` 인 것을 같은 방식(제곱거리 < 기존 표적 점수)으로 모은다.
//    capacity 는 호출자의 signed 용량(통지 20·AI 평가 30)이며 가득 찬 목록에서도 원본 후보 순회를 끝낸다. 중복도 지우지 않는다.
//  - notify(blocker): boss 일 때만, 현재 타워의 소유자/원점으로 수집한 뒤 아직 존재하는(현재 표적이 있는) 각 Gun 에 setTarget(gun, blocker)을 같은 순서로 부른다(중복 통지도 보존).
// hooks = { currentTarget(sid)→표적 SID|undefined(총이 없으면 undefined, 표적 0 도 실제 값), mode(sid)→현재 공격 방향, muzzle(sid)→[x, y], setTarget(gun, blocker) }, state = { cannonRange, archerRange, stoneTowerType(기본 78) },
// process = { boss, friendlyFire, allies[9][9], allianceRead?(소유자, 대상) }, targeting 은 같은 풀의 RawGunTarget 이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E87: 스톤 타워 표적 통지 이식으로 추가)
import { RawSquidFinder } from './raw-squid-finder.js';
import { RawFenceTarget } from './raw-fence-target.js';

// 사각형 ceil 과 422230 중심 snap 의 서로 다른 실제 float 상수, 0/2/4/6 순서의 기본 방향표, 플레이어 수·signed 용량 상한이다. (2026-11-08 WB04-E87)
const CEIL_BIAS = Math.fround(0.9999);
const SNAP_BIAS = Math.fround(0.99999);
const DX = Object.freeze([0, 1, 0, -1]);
const DY = Object.freeze([-1, 0, 1, 0]);
const PLAYER_LAST = 8;
const PLAYER_SLOTS = 9;
const INT_MAX = 2147483647;
const fround = Math.fround;
// 슬롯의 TYPE(+10)·소유자(+34)·extra(+40)·좌표(+14/+18) 위치와 비행 flags2 비트, 아처의 지상 점수 가산이다. (2026-11-08 WB04-E87)
const TYPE = 10;
const OWNER = 34;
const EXTRA = 40;
const X_AT = 14;
const Y_AT = 18;
const FLYER = 0x100000;
const GROUND_PENALTY = 1000;

// 정상 보드 좌표와 최대 사거리의 CRT 절삭을 수행한다. (2026-11-08 WB04-E87)
function truncate(value) {
    if (!Number.isFinite(value) || value < -2147483648 || value >= 2147483648) throw new RangeError('장애물 검색 정수 좌표 범위 오류');
    return Math.trunc(value);
}

// 원본 BoardCoord 의 유효 범위이다. (2026-11-08 WB04-E87)
const valid = (point) => point[0] > 0 && point[0] < 256 && point[1] > 0 && point[1] < 256;
const clamp = (value) => (value < 1 ? 1 : 255 < value ? 255 : value);
const fmin = (a, b) => (b < a ? b : a);
const fmax = (a, b) => (a < b ? b : a);

// min/max helper 는 새 점을 먼저 보드에 clamp 하고 무효인 기존 점을 교체한다. (2026-11-08 WB04-E87)
function include(rect, point) {
    point = [clamp(point[0]), clamp(point[1])];
    if (valid([rect.left, rect.top])) { rect.left = fmin(rect.left, point[0]); rect.top = fmin(rect.top, point[1]); } else { rect.left = point[0]; rect.top = point[1]; }
    if (valid([rect.right, rect.bottom])) { rect.right = fmax(rect.right, point[0]); rect.bottom = fmax(rect.bottom, point[1]); } else { rect.right = point[0]; rect.bottom = point[1]; }
}

// 현재 float 사각형의 중심을 한 번 float 에 저장한다. (2026-11-08 WB04-E87)
const center = (rect) => [fround((rect.left + rect.right) * 0.5), fround((rect.top + rect.bottom) * 0.5)];

// 원본 41d530 의 네 방향 edge midpoint 를 동일한 float 차이/세로 동률 규칙으로 고른다. (2026-11-08 WB04-E87)
function edge(source, rect) {
    const middle = center(rect);
    const dx = fround(middle[0] - source[0]);
    const dy = fround(middle[1] - source[1]);
    const way = ((Math.abs(dx) <= Math.abs(dy) ? (dy > 0 ? 4 : 0) : (dx <= 0 ? 6 : 2)) + 4) % 8;
    if (way === 0 || way === 4) return [fround((rect.right - rect.left) * 0.5 + rect.left), way === 0 ? rect.top : rect.bottom];
    return [way === 2 ? rect.right : rect.left, fround((rect.bottom - rect.top) * 0.5 + rect.top)];
}

// 두 대각선과 실제 462800 교차를 차례로 확인한다. 사각형 경계는 포함하며 교차 helper 의 끝점 규칙은 별개이다. (2026-11-08 WB04-E87)
function crosses(rect, from, to) {
    let point = RawFenceTarget.intersection(from, to, [rect.left, rect.top], [rect.right, rect.bottom]);
    if (rect.left <= point[0] && point[0] <= rect.right && rect.top <= point[1] && point[1] <= rect.bottom) return true;
    point = RawFenceTarget.intersection(from, to, [rect.left, rect.bottom], [rect.right, rect.top]);
    return rect.left <= point[0] && point[0] <= rect.right && rect.top <= point[1] && point[1] <= rect.bottom;
}

// 비정렬 float 좌표를 읽는다(NaN payload 보존). (2026-11-08 WB04-E87)
const view = new DataView(new ArrayBuffer(4));
function coordinate(raw, offset) {
    for (let index = 0; index < 4; index += 1) view.setUint8(index, raw[offset + index]);
    return view.getFloat32(0, true);
}

export class RawGunBlocker {
    // 현재 Gun/표적/타워가 공유하는 풀, 일반 finder 의 해시, group·발자국·플래그 표, 같은 번호의 확장 높이, 중심 getter, 현재 boss/friendlyFire/방향별 동맹, TYPE 초기화의 최대 사거리, 기존 Gun 경계이다.
    #pool;
    #hash;
    #types;
    #targetTypes;
    #targeting;
    #process;
    #state;
    #hooks;

    // 10.78 의 전체 실제 TYPE 저장소를 공유하며 다른 월드와의 혼합을 거부한다. (2026-11-08 WB04-E87)
    constructor(pool, hash, types, targetTypes, targeting, process, state, hooks) {
        if (pool?.edition !== 'patch1078' || types.length !== 188 || targetTypes.length !== 188 || targeting?.pool !== pool || !(state.stoneTowerType < types.length)) throw new TypeError('장애물 통지의 판본/실제 저장소 오류');
        this.#pool = pool;
        this.#hash = hash;
        this.#types = types;
        this.#targetTypes = targetTypes;
        this.#targeting = targeting;
        this.#process = process;
        this.#state = state;
        this.#hooks = hooks ?? {};
    }

    // 현재 방향별 동맹 한 칸을 읽는다. (2026-11-08 WB04-E87)
    #isAlly(owner, target) {
        const process = this.#process;
        if (owner >= PLAYER_SLOTS || target >= PLAYER_SLOTS) throw new RangeError('Gun 현재 방향별 동맹 범위 오류');
        return process.allianceRead ? process.allianceRead(owner, target) !== 0 : Boolean(process.allies[owner][target]);
    }

    // 원본 0 표적의 점수 조회도 허용하고 풀 밖 입력만 거부한다. (2026-11-08 WB04-E87)
    #slot(sid) {
        if (!Number.isInteger(sid) || sid < 0 || sid >= this.#pool.capacity) throw new RangeError('장애물 통지 SID 오류');
        const raw = this.#pool.slot(sid);
        if (raw[TYPE] >= this.#types.length) throw new RangeError('장애물 통지 TYPE 오류');
        return raw;
    }

    // 같은 raw float 좌표를 매번 다시 읽는다(SID 0 도 원본 점수 계산에서 읽는 실제 raw 슬롯). (2026-11-08 WB04-E87)
    #position(sid) {
        const raw = this.#slot(sid);
        return [coordinate(raw, X_AT), coordinate(raw, Y_AT)];
    }

    // 확장된 표적 발자국은 SID 0 의 원본 빈 슬롯도 동일한 min/max helper 로 계산한다. (2026-11-08 WB04-E87)
    #bounds(sid) {
        const point = this.#position(sid);
        const type = this.#slot(sid)[TYPE];
        const rect = { left: point[0], top: point[1], right: point[0], bottom: point[1] };
        include(rect, [fround(point[0] - (this.#types[type].footX - 1)), fround(point[1] - (this.#types[type].footY + this.#targetTypes[type].heightExtension - 1))]);
        return rect;
    }

    // 발사기 점수는 float 저장 뒤 지상에 1000 을 더하고 캐논은 x87 제곱거리 전체를 사용한다(004693b0/00469440). (2026-11-08 WB04-E87)
    #score(source, target, archer) {
        const point = edge(source, this.#bounds(target));
        const dx = source[0] - point[0];
        const dy = source[1] - point[1];
        if (!archer) return dx * dx + dy * dy;
        const distance = fround(dx * dx + dy * dy);
        return distance + ((this.#types[this.#slot(target)[TYPE]].flags2 & FLYER) ? 0 : GROUND_PENALTY);
    }

    // 현재 source owner 방향의 동맹만 검사하며 이미 가득 찬 목록에서도 원본 후보 순회를 끝낸다. (2026-11-08 WB04-E87)
    collect(owner, point, capacity = 20) {
        const hooks = this.#hooks;
        if (owner > PLAYER_LAST || !(capacity <= INT_MAX) || !(capacity >= 0) || typeof hooks.currentTarget !== 'function' || typeof hooks.mode !== 'function' || typeof hooks.muzzle !== 'function') throw new TypeError('장애물 후보 수집 실제 경계/용량 오류');
        const state = this.#state;
        const stone = this.#types[state.stoneTowerType];
        const box = { left: point[0], top: point[1], right: point[0], bottom: point[1] };
        include(box, [fround(point[0] - (stone.footX - 1)), fround(point[1] - (stone.footY - 1))]);
        const middle = center(box);
        const result = [];
        const heights = this.#targetTypes.map((type) => type.heightExtension);
        const radius = truncate(state.archerRange);
        const x = truncate(fround(truncate(middle[0] + SNAP_BIAS)));
        const y = truncate(fround(truncate(middle[1] + SNAP_BIAS)));
        const near = new RawSquidFinder(this.#pool, this.#hash, this.#types, heights);
        // 먼저 group 0 원본 일반 finder 를 순회한다. dead/free 비트를 별도로 더해 배제하지 않는다.
        for (let sid = near.begin({ left: x - radius, top: y - radius, right: x + radius, bottom: y + radius }, 5); sid; sid = near.next()) {
            const raw = this.#slot(sid);
            if (this.#types[raw[TYPE]].group !== 0 || (raw[EXTRA] & 1) || (!this.#process.friendlyFire && this.#isAlly(owner, raw[OWNER]))) continue;
            const spot = this.#targeting.center(sid);
            const distance = fround((spot[0] - middle[0]) * (spot[0] - middle[0]) + (spot[1] - middle[1]) * (spot[1] - middle[1]));
            // 원본 최대 반경 검사는 경계를 포함하고, 아래의 기존 표적과 비교는 엄격한 작음이다.
            if (!(distance <= state.archerRange * state.archerRange)) continue;
            const current = hooks.currentTarget(sid);
            hooks.mode(sid);
            const present = current !== undefined && current !== null;
            if (present && distance < this.#score(this.#targeting.center(sid), current, true) && crosses(box, hooks.muzzle(sid), this.#position(current)) && result.length < capacity) result.push(sid);
        }
        // 캐논은 새 사각형에서 네 방향으로 뻗는 원본 ray 영역의 일반 finder 를 순서대로 사용한다.
        for (let direction = 0; direction <= 6; direction += 2) {
            const ray = { ...box };
            include(ray, [fround(DX[direction / 2] * state.cannonRange + middle[0]), fround(DY[direction / 2] * state.cannonRange + middle[1])]);
            const finder = new RawSquidFinder(this.#pool, this.#hash, this.#types, heights);
            // 최소점은 plain ftol, 최대점은 0.9999f 를 더한 뒤 절삭한다.
            for (let sid = finder.begin({ left: truncate(ray.left), top: truncate(ray.top), right: truncate(ray.right + CEIL_BIAS), bottom: truncate(ray.bottom + CEIL_BIAS) }, 5); sid; sid = finder.next()) {
                const raw = this.#slot(sid);
                if (this.#types[raw[TYPE]].group !== 1 || (raw[EXTRA] & 1) || (!this.#process.friendlyFire && this.#isAlly(owner, raw[OWNER])) || hooks.mode(sid) !== ((direction - 4) & 7)) continue;
                const current = hooks.currentTarget(sid);
                hooks.mode(sid);
                const spot = this.#targeting.center(sid);
                const dx = fround(spot[0] - middle[0]);
                const dy = fround(spot[1] - middle[1]);
                const present = current !== undefined && current !== null;
                if (present && dx * dx + dy * dy < this.#score(spot, current, false) && result.length < capacity) result.push(sid);
            }
        }
        return result;
    }

    // 현재 수집을 마친 뒤 각 번호의 기존 Gun 을 다시 조회하므로 도중 삭제된 Gun 도 자연스럽게 건너뛴다. (2026-11-08 WB04-E87)
    notify(blocker) {
        if (!this.#process.boss) return;
        if (typeof this.#hooks.setTarget !== 'function') throw new Error('장애물 통지 실제 Gun 표적 지정 누락');
        const owner = this.#slot(blocker)[OWNER];
        const candidates = this.collect(owner, this.#position(blocker));
        // 실제 20DWORD 목록 순서를 유지하며 같은 번호의 중복 통지도 보존한다.
        for (const value of candidates) {
            const current = this.#hooks.currentTarget(value);
            if (current !== undefined && current !== null) this.#hooks.setTarget(value, blocker);
        }
    }
}
