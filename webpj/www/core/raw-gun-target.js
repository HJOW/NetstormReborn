// GunProcess(캐논/아처)의 실제 표적 검색과 MayShoot 계열을 같은 raw 풀·해시에 연결한 모듈이다(cpppj `o/RawGunTarget.cpp/.h`, 10.78 전용: 0046b6f0 FindTarget, 0046af80 MayTarget, 0046ac00 MayShoot, 0046b070 MayFire,
// 00469190 CannonRay, 현재 Footprint/Center). 실제 후보 finder 순서·서버 SID 필터·사거리/사정선/각도 산술을 복원하며, 모든 float 연산은 원본의 float 저장 위치(Math.fround)와 double 승격을 그대로 따른다.
//  - findTarget(x, y, type, direction, owner): 현재 타입의 발자국 중심과 공격 모드 프레임으로 문맥을 구성해 group 0(Archer)이면 center±trunc(range) 범위의 일반 finder(flags 5)로, group 1(Cannon)이면 0·2·4·6 방향 사정선마다 새 finder 로 검색한다.
//    같은 점수는 첫 후보를 유지하고 ground 후보에는 1000 을 더한다. 진단 출력(showRanges)이 켜져 있으면 최종 표적의 발자국을 drawRange(rect, true)로 내보낸다. 그 밖의 group 은 원본 assert 이다.
//  - mayTarget(query, target): PATCH 는 SID 0(TYPE 0)도 읽고 HP 보유 여부에서 거부한다. Damageable(Dais 는 extra bit0, 일반은 flags1 bit4)·서버 예약 번호 5..14999·소유자 범위·동맹 마스크(friendlyFire 가 꺼졌을 때)·extra&0x11·사제 수량(≤1)·동일 위치 뒤 mayShoot.
//  - mayShoot(query, target): group 0 사거리/윈드 아처 관측각, group 1 사정선과 확장 발자국의 접촉, group 3 직접 허용이며 그 밖의 group 은 checking 켜진 flyer 가 아니면 원본 assert 이다.
//  - mayFire(query, target): 썬 캐논이면 현재 프레임이 그 방향으로 쏠 수 있는지 먼저 검사한 뒤 mayTarget 을 부른다(투사체 생성이 아님).
//  - cannonRay(query, direction): 지정 방향의 SHP 포구(+20/+22)와 타입 핫스폿으로 정한 사거리 사각형을 float 로 반환한다(얇은 ray 는 floor/ceil 뒤 경계 확장). footprint(target, extended=true)/center(target)/directionFrame(type, direction)/cannonDirection(query, direction)도 공개한다.
// query = { x, y, centerX, centerY, type, direction, owner, frame }, 사각형 = { left, top, right, bottom }(float 값), types[n] = { footX, footY, flags1, flags2, group, hotspotX, hotspotY, maxHitPoints },
// targetTypes[n] = { range, theme, directionFrames[4], heightExtension }, shapes[n] = { loaded, frameCount, frames[{ muzzleX, muzzleY }] }, state = { friendlyFire, checking, showRanges, sunCannonType(128), sunFenceType(136),
// windArcherType(140), thunderArcherType(148), windHalfAngle(30), scaleX(16), scaleY(11), allyMasks[9], playerFlags[9], allyMaskRead?(소유자), playerFlagsRead?(소유자), angles }, hooks = { drawRange(rect, 표적 여부) } 이다.
// friendlyFire/checking 은 소비 위치마다 읽는 현재 값(참/거짓 판정)이다. GunProcess 와의 연결(MakeGunTargetHooks)은 RawGunProcess 이식 때 한다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E82: Gun 표적 이식으로 추가)
import { RawSquidFinder } from './raw-squid-finder.js';
import { normalizeBoardAngle } from './raw-board-angle.js';

// 짝수 방향의 ray 이동 표이다(원본 0052f83c/0052f85c). (2026-11-08 WB04-E82)
const DX = Object.freeze([0, 1, 1, 1, 0, -1, -1, -1]);
const DY = Object.freeze([-1, -1, 0, 1, 1, 1, 0, -1]);
// 원본 float 경계 확장·ground 후보 점수와 ceil 보정 상수(float 상수가 double 로 승격되어 쓰임)이다. (2026-11-08 WB04-E82)
const EDGE_LOW = Math.fround(0.0001);
const EDGE_HIGH = Math.fround(0.1);
const GROUND_PENALTY = 1000;
const CEIL_BIAS = Math.fround(0.9999);
// 후보 타입 flags: 1 HP 보유(flags1 0x10), flags2 의 비행(0x100000)·사제(0x200000)·Dais(0x400000), 프레임 두 배 비트(flags1 0x440000)이다. (2026-11-08 WB04-E82)
const HIT_POINTS = 0x10;
const FLYER = 0x100000;
const PRIEST = 0x200000;
const DAIS = 0x400000;
const DOUBLE_FRAMES = 0x440000;
// 썬 캐논 방향별 허용 프레임(bit 8000)이며 24..27 은 별도 방향을 검사한다. (2026-11-08 WB04-E82)
const SUN_ALLOWED_FRAME = Object.freeze([1, 3, 0, 2]);
const SUN_EXTRA_DIRECTION = Object.freeze([4, 0, 6, 2]);
// 슬롯의 TYPE(+10)·소유자(+34)·extra(+40)·수량(+26) 위치, x/y float(+14/+18), 풀의 서버 예약 끝(14999)과 최대 Player 번호, 사정선 없는 사각형이다. (2026-11-08 WB04-E82)
const TYPE = 10;
const OWNER = 34;
const EXTRA = 40;
const QUANTITY = 26;
const X_AT = 14;
const Y_AT = 18;
const RESERVED_LAST = 14999;
const PLAYER_LAST = 8;
const NO_RAY = Object.freeze({ left: -10000, top: -10000, right: -10000, bottom: -10000 });

const view = new DataView(new ArrayBuffer(4));
const fround = Math.fround;

// 비정렬 float 좌표를 읽는다(NaN payload 보존). (2026-11-08 WB04-E82)
function coordinate(raw, offset) {
    for (let index = 0; index < 4; index += 1) view.setUint8(index, raw[offset + index]);
    return view.getFloat32(0, true);
}

// 비정렬 정수를 폭만큼 읽는다. (2026-11-08 WB04-E82)
function readWord(raw, offset, width) {
    let value = 0;
    for (let index = 0; index < width; index += 1) value += raw[offset + index] * 2 ** (8 * index);
    return value >>> 0;
}

// std::min/std::max 와 같은 비교 순서(NaN 은 첫 인자)이다. (2026-11-08 WB04-E82)
const fmin = (a, b) => (b < a ? b : a);
const fmax = (a, b) => (a < b ? b : a);

// 원본 4방향 BoardCoord 비교는 Y 의 절댓값이 같으면 세로 방향을 우선한다. (2026-11-08 WB04-E82)
function direction4(x, y, targetX, targetY) {
    const dx = fround(targetX - x);
    const dy = fround(targetY - y);
    if (Math.abs(dx) <= Math.abs(dy)) return dy > 0 ? 4 : 0;
    return dx <= 0 ? 6 : 2;
}

// 원본 외곽 사각형의 OR 교차는 완전 포함도 허용하며 모든 경계가 포함된다. (2026-11-08 WB04-E82)
function intersects(a, b) {
    return (a.left <= b.left || a.left <= b.right) && (b.left <= a.right || b.right <= a.right) &&
        (a.top <= b.top || a.top <= b.bottom) && (b.top <= a.bottom || b.bottom <= a.bottom);
}

// 보드의 clamp/invalid 점 처리에서 사용되는 원본 유효 범위이다. (2026-11-08 WB04-E82)
const validPoint = (x, y) => x > 0 && y > 0 && x < 256 && y < 256;

// std::clamp(v, 1.0f, 255.0f) 이다. (2026-11-08 WB04-E82)
const clamp = (value) => (value < 1 ? 1 : 255 < value ? 255 : value);

// CRT __ftol 과 같은 정상 유한 값의 0방향 절삭이다. (2026-11-08 WB04-E82)
function truncate(value) {
    if (!Number.isFinite(value) || value < -2147483648 || value >= 2147483648) throw new RangeError('Gun 좌표 절삭 범위 오류');
    return Math.trunc(value);
}

// 원본 BoardCoord.ceil 의 정상 보드 좌표 정수 변환(0.9999f 를 더한 뒤 절삭)이다. (2026-11-08 WB04-E82)
function ceiling(value) {
    const biased = value + CEIL_BIAS;
    if (!Number.isFinite(biased) || biased < -2147483648 || biased >= 2147483648) throw new RangeError('Gun ceil 범위 오류');
    return Math.trunc(biased);
}

// 00468e70/CD 인라인 angle 정규화는 360 을 그대로 허용하며 음수에서 1-trunc(각/360)을 더한다. (2026-11-08 WB04-E82)
function normalize(angle) {
    if (!Number.isFinite(angle)) throw new TypeError('Gun 비유한 각도');
    if (angle < 0) angle = fround((1 - truncate(fround(angle / 360))) * 360 + angle);
    if (angle > 360) angle = fround(angle % 360);
    return angle;
}

// 원본의 최소 circular 구간은 끝점을 포함하며 180도보다 긴 구간은 바깥쪽을 선택한다. (2026-11-08 WB04-E82)
function containsAngle(value, first, last) {
    if (first > last) [first, last] = [last, first];
    return fround(last - first) <= 180 ? first <= value && value <= last : value <= first || last <= value;
}

export class RawGunTarget {
    // 같은 raw 풀·일반 finder 의 해시·현재 후보의 TYPE 표·실제 타입 검색 자료·SHP 포구·현재 전역·진단 효과이다.
    #pool;
    #hash;
    #types;
    #targetTypes;
    #shapes;
    #state;
    #hooks;

    // 같은 판본의 전체 타입/SHP 번호 체계(188)를 검사하며 표와 상태를 복사하지 않는다. (2026-11-08 WB04-E82)
    constructor(pool, hash, types, targetTypes, shapes, state, hooks = {}) {
        if (pool?.edition !== 'patch1078') throw new TypeError('Gun 표적은 PATCH 전용입니다');
        if (types.length !== 188 || targetTypes.length !== types.length || shapes.length !== types.length) throw new RangeError('Gun 표적 타입/SHP 판본 불일치');
        this.#pool = pool;
        this.#hash = hash;
        this.#types = types;
        this.#targetTypes = targetTypes;
        this.#shapes = shapes;
        this.#state = state;
        this.#hooks = hooks ?? {};
    }

    // 같은 raw 월드에 대한 GunProcess 어댑터인지 확인하는 연결용 getter 이다. (2026-11-08 WB04-E82)
    get pool() { return this.#pool; }

    // 실제 세션은 같은 Player+54 를 다시 읽고, 독립 입력은 기존 배열을 사용한다. (2026-11-08 WB04-E82)
    #allyMask(owner) {
        const state = this.#state;
        if (!Number.isInteger(owner) || owner < 0 || owner >= state.allyMasks.length) throw new RangeError('Gun 현재 동맹 소유자 범위 오류');
        return (state.allyMaskRead ? state.allyMaskRead(owner) : state.allyMasks[owner]) >>> 0;
    }

    // 진단 출력 분기가 도달한 시점의 현재 참여 플래그를 읽는다. (2026-11-08 WB04-E82)
    #playerFlags(owner) {
        const state = this.#state;
        if (!Number.isInteger(owner) || owner < 0 || owner >= state.playerFlags.length) throw new RangeError('Gun 현재 참여 소유자 범위 오류');
        return (state.playerFlagsRead ? state.playerFlagsRead(owner) : state.playerFlags[owner]) >>> 0;
    }

    // normal 원본 문맥만 허용한다. 잘못된 타입·소유자·비유한 좌표는 호스트 계약 밖이다. (2026-11-08 WB04-E82)
    #validate(query) {
        if (query.type >= this.#types.length || query.owner > PLAYER_LAST || !Number.isFinite(query.x) || !Number.isFinite(query.y) ||
            !Number.isFinite(query.centerX) || !Number.isFinite(query.centerY)) throw new RangeError('Gun 표적 문맥 범위 오류');
    }

    // 서로 다른 두 special 타입은 저장된 방향 표보다 먼저 처리한다. (2026-11-08 WB04-E82)
    directionFrame(type, direction) {
        const index = Math.trunc(direction / 2);
        if (index < 0 || index > 3) throw new RangeError('Gun 방향 프레임 범위 오류');
        const state = this.#state;
        if (type === state.sunCannonType) return SUN_ALLOWED_FRAME[index];
        if (type === state.sunFenceType) return index;
        const frame = this.#targetTypes[type].directionFrames[index];
        return frame === -1 ? 0 : frame;
    }

    // 홀수 방향은 자연스럽게 거부하고 짝수 방향에서는 group 1 원본 assert 를 유지한다. (2026-11-08 WB04-E82)
    cannonDirection(query, direction) {
        if (direction % 2) return false;
        if (this.#types[query.type].group !== 1) throw new Error('Gun Cannon group 원본 assert');
        if (direction < 0 || direction > 7) throw new RangeError('Gun Cannon 방향 범위 오류');
        const theme = this.#targetTypes[query.type].theme;
        return (theme !== 1 && theme !== 2) || direction === query.direction;
    }

    // base Footprint 가 사용하는 확장 높이와 by-value raw 좌표를 읽는다. 기본 extended=true 는 공격 검색이며 false 는 사정거리 표시의 원본 flags0 이다. (2026-11-08 WB04-E82)
    footprint(target, extended = true) {
        if (!target || target >= this.#pool.capacity) throw new RangeError('Gun 표적 SID 오류');
        const raw = this.#pool.slot(target);
        const number = raw[TYPE];
        if (number >= this.#types.length) throw new RangeError('Gun 표적 타입 오류');
        const type = this.#types[number];
        const x = coordinate(raw, X_AT);
        const y = coordinate(raw, Y_AT);
        let lowX = fround(x - (type.footX - 1));
        let lowY = fround(y - (type.footY + (extended ? this.#targetTypes[number].heightExtension : 0) - 1));
        // 실제 min/max helper 는 현재 원점이 아니라 들어오는 corner 를 보드에 clamp 한다.
        lowX = clamp(lowX);
        lowY = clamp(lowY);
        let highX = x;
        let highY = y;
        let left = x;
        let top = y;
        if (validPoint(left, top)) { left = fmin(left, lowX); top = fmin(top, lowY); } else { left = lowX; top = lowY; }
        if (validPoint(highX, highY)) { highX = fmax(highX, lowX); highY = fmax(highY, lowY); } else { highX = lowX; highY = lowY; }
        return { left, top, right: highX, bottom: highY };
    }

    // 확장 높이를 포함하지 않는 현재 타입 발자국의 실제 중심이다(footX/Y 의 절반을 raw 좌표에서 뺌). (2026-11-08 WB04-E82)
    center(target) {
        const raw = this.#pool.slot(target);
        const type = this.#types[raw[TYPE]];
        return [fround(coordinate(raw, X_AT) - type.footX * 0.5), fround(coordinate(raw, Y_AT) - type.footY * 0.5)];
    }

    // 실제 Cannon ray 의 별도 rectangle 과 4DWORD 반환을 보존한다. (2026-11-08 WB04-E82)
    cannonRay(query, direction) {
        this.#validate(query);
        if (!this.cannonDirection(query, direction)) return { ...NO_RAY };
        const state = this.#state;
        if (!Number.isFinite(state.scaleX) || !Number.isFinite(state.scaleY) || state.scaleX === 0 || state.scaleY === 0) throw new RangeError('Gun Cannon 픽셀 배율 오류');
        const frame = this.directionFrame(query.type, direction);
        const shape = this.#shapes[query.type];
        if (!shape.loaded || frame < 0 || frame >= shape.frames.length) throw new RangeError('Gun Cannon SHP 물리 프레임 오류');
        const type = this.#types[query.type];
        if (frame >= shape.frameCount * ((type.flags1 & DOUBLE_FRAMES) ? 2 : 1) || type.maxHitPoints === -0x22222223 || type.maxHitPoints === -0x32323233) throw new RangeError('Gun Cannon frameCheck 원본 assert');
        const header = shape.frames[frame];
        // 원본 00469190 은 화면 그림 기준점이 아닌 SHP+20/+22 포구를 읽는다.
        let x = fround(header.muzzleX / state.scaleX + query.x);
        let y = fround(header.muzzleY / state.scaleY + query.y);
        x = fround(-type.hotspotX / state.scaleX + x);
        y = fround(-type.hotspotY / state.scaleY + y);
        const range = this.#targetTypes[query.type].range;
        const endX = fround(DX[direction] * range + x);
        const endY = fround(DY[direction] * range + y);
        const rect = { left: x, top: y, right: x, bottom: y };
        // 두 판본의 min/max helper 모두 별도 incoming endpoint 를 clamp 한다.
        const minimumX = clamp(endX);
        const minimumY = clamp(endY);
        if (validPoint(rect.left, rect.top)) { rect.left = fmin(rect.left, minimumX); rect.top = fmin(rect.top, minimumY); } else { rect.left = minimumX; rect.top = minimumY; }
        if (validPoint(rect.right, rect.bottom)) { rect.right = fmax(rect.right, minimumX); rect.bottom = fmax(rect.bottom, minimumY); } else { rect.right = minimumX; rect.bottom = minimumY; }
        // 원본 width<2 또는 height<2 인 얇은 ray 는 floor/ceil 뒤 경계를 조금 확장한다.
        if (rect.right - rect.left < 2 || rect.bottom - rect.top < 2) {
            rect.left = fround(truncate(rect.left) + EDGE_LOW);
            rect.top = fround(truncate(rect.top) + EDGE_LOW);
            rect.right = fround(ceiling(rect.right) + EDGE_HIGH);
            rect.bottom = fround(ceiling(rect.bottom) + EDGE_HIGH);
            rect.left = clamp(rect.left);
            rect.top = clamp(rect.top);
            rect.right = clamp(rect.right);
            rect.bottom = clamp(rect.bottom);
        }
        if (state.showRanges && !(this.#playerFlags(query.owner) & 0x20)) {
            if (typeof this.#hooks.drawRange !== 'function') throw new Error('Gun Cannon 진단 출력 경계 누락');
            this.#hooks.drawRange({ ...rect }, false);
        }
        return rect;
    }

    // 발자국 사각형까지 최소 거리의 판본별 float 저장 위치를 유지한다(PATCH 는 거리를 float 로 저장). (2026-11-08 WB04-E82)
    #archerShot(query, target) {
        const rect = this.footprint(target);
        const dx = query.centerX < rect.left ? rect.left - query.centerX : rect.right < query.centerX ? query.centerX - rect.right : 0;
        const dy = query.centerY < rect.top ? rect.top - query.centerY : rect.bottom < query.centerY ? query.centerY - rect.bottom : 0;
        const distance = fround(Math.sqrt(dx * dx + dy * dy));
        if (!(distance < this.#targetTypes[query.type].range)) return false;
        const state = this.#state;
        if (query.type !== state.windArcherType) return true;
        const center = this.center(target);
        let angle = 0;
        if (query.centerX !== center[0] || query.centerY !== center[1]) {
            // x87 fpatan 뒤 degree 환산은 원본 double 상수 57.295827908797776 을 사용한다.
            const radians = Math.atan2(-(center[1] - query.centerY), center[0] - query.centerX);
            const raw = fround(450 - radians * 57.295827908797776);
            angle = normalize(fround(normalizeBoardAngle(raw, state.angles ?? {})));
        }
        const base = normalize(fround(query.direction * 45));
        return containsAngle(angle, normalize(fround(base - state.windHalfAngle)), normalize(fround(base + state.windHalfAngle)));
    }

    // 원본은 현재 표적 중심의 4방향을 구해 그 방향 ray 와 확장 발자국을 비교한다. (2026-11-08 WB04-E82)
    #cannonShot(query, target) {
        const center = this.center(target);
        const ray = this.cannonRay(query, direction4(query.centerX, query.centerY, center[0], center[1]));
        return intersects(this.footprint(target), ray);
    }

    // 미지원 group 은 release 에서 flyer 와 같은 직접 허용을 유지한다. (2026-11-08 WB04-E82)
    mayShoot(query, target) {
        this.#validate(query);
        const type = this.#types[query.type];
        if (type.group === 0) return this.#archerShot(query, target);
        if (type.group === 1) return this.#cannonShot(query, target);
        if (type.group !== 3 && this.#state.checking && !(type.flags2 & FLYER)) throw new Error('Gun MayShoot flyer 원본 assert');
        return true;
    }

    // 실제 Damageable·서버 번호·소유자·사제 수량만 검사하며 dead/free state 필터를 추가하지 않는다. PATCH 46af80 은 정상 빈 SID 0/TYPE 0 을 읽고 hasHP 에서 거부한다. (2026-11-08 WB04-E82)
    mayTarget(query, target) {
        this.#validate(query);
        if (target >= this.#pool.capacity) throw new RangeError('Gun 후보 SID 오류');
        const raw = this.#pool.slot(target);
        const number = raw[TYPE];
        if (number >= this.#types.length) throw new RangeError('Gun 후보 타입 오류');
        const type = this.#types[number];
        const extra = raw[EXTRA];
        if ((type.flags2 & DAIS) ? !(extra & 1) : !(type.flags1 & HIT_POINTS)) return false;
        if (target >= 5 && target <= RESERVED_LAST) return false;
        const owner = raw[OWNER];
        if (owner > PLAYER_LAST) throw new RangeError('Gun 후보 소유자 오류');
        if (!this.#state.friendlyFire && (this.#allyMask(query.owner) & (1 << owner))) return false;
        if (extra & 0x11) return false;
        if (type.flags2 & PRIEST) {
            if ((readWord(raw, QUANTITY, 4) | 0) <= 1) return false;
        }
        if (coordinate(raw, X_AT) === query.x && coordinate(raw, Y_AT) === query.y) return false;
        return this.mayShoot(query, target);
    }

    // 밴더 타워는 flyer/balloon 외에는 MayTarget 이전에 거부한다. (2026-11-08 WB04-E82)
    #archerTarget(query, target) {
        if (query.type === this.#state.thunderArcherType && !(this.#types[this.#pool.slot(target)[TYPE]].flags2 & 0x120000)) return false;
        return this.mayTarget(query, target) && this.#archerShot(query, target);
    }

    // source→rect 중심의 반대 방향 edge midpoint 를 원본 부동소수 저장 순서대로 반환한다. (2026-11-08 WB04-E82)
    #hitPoint(query, rect) {
        const midX = fround((rect.left + rect.right) * 0.5);
        const midY = fround((rect.top + rect.bottom) * 0.5);
        const direction = (direction4(query.centerX, query.centerY, midX, midY) + 4) & 7;
        if (direction === 0 || direction === 4) return [fround((rect.right - rect.left) * 0.5 + rect.left), direction === 0 ? rect.top : rect.bottom];
        return [direction === 2 ? rect.right : rect.left, fround((rect.bottom - rect.top) * 0.5 + rect.top)];
    }

    // 일반 finder 의 타입별 확장 높이 입력을 번호순으로 만든다. (2026-11-08 WB04-E82)
    #heights() {
        return this.#targetTypes.map((type) => type.heightExtension);
    }

    // 실제 Archer finder 는 center±trunc(range) 범위와 flags 5 를 쓰며 ground 에 1000 을 더한다. (2026-11-08 WB04-E82)
    #findArcher(query) {
        const range = truncate(this.#targetTypes[query.type].range);
        const x = ceiling(query.centerX);
        const y = ceiling(query.centerY);
        const finder = new RawSquidFinder(this.#pool, this.#hash, this.#types, this.#heights());
        let best = fround(1.0e13);
        let result = 0;
        // 원본은 첫 후보와 next 를 준비한 뒤 자기 동맹을 읽는다.
        const first = finder.begin({ left: x - range, top: y - range, right: x + range, bottom: y + range }, 5);
        if (!this.#state.friendlyFire && !(this.#allyMask(query.owner) & (1 << query.owner))) throw new Error('Gun 자기 동맹 원본 assert');
        // next 는 현재 후보가 반환될 때 이미 저장된다. 후보의 현재 타입/좌표는 매번 다시 조회한다.
        for (let sid = first; sid; sid = finder.next()) {
            if (!(this.#types[this.#pool.slot(sid)[TYPE]].flags1 & HIT_POINTS) || !this.#archerTarget(query, sid)) continue;
            const point = this.#hitPoint(query, this.footprint(sid));
            const dx = query.centerX - point[0];
            const dy = query.centerY - point[1];
            const distance = fround(dx * dx + dy * dy);
            const score = distance + ((this.#types[this.#pool.slot(sid)[TYPE]].flags2 & FLYER) ? 0 : GROUND_PENALTY);
            if (score < best) { best = fround(score); result = sid; }
        }
        return result;
    }

    // Cannon 은 0/2/4/6 ray 범위마다 finder 를 새로 시작한다. 같거나 먼 점수는 첫 표적을 유지한다. (2026-11-08 WB04-E82)
    #findCannon(query) {
        const range = this.#targetTypes[query.type].range;
        let best = fround(range * range * 2);
        let result = 0;
        // 원본 네 짝수 방향의 순서를 유지한다.
        for (let direction = 0; direction <= 6; direction += 2) {
            const ray = this.cannonRay(query, direction);
            const finder = new RawSquidFinder(this.#pool, this.#hash, this.#types, this.#heights());
            // minimum 은 floor, maximum 은 0.9999f 를 더한 ceil 을 사용한다.
            for (let sid = finder.begin({ left: truncate(ray.left), top: truncate(ray.top), right: ceiling(ray.right), bottom: ceiling(ray.bottom) }, 5); sid; sid = finder.next()) {
                if (!this.mayTarget(query, sid)) continue;
                const point = this.#hitPoint(query, this.footprint(sid));
                const dx = query.centerX - point[0];
                const dy = query.centerY - point[1];
                const score = fround(dx * dx + dy * dy);
                if (score < best) { best = fround(score); result = sid; }
            }
        }
        return result;
    }

    // 외부 검색은 현재 타입 발자국 중심/공격 모드 프레임으로 문맥을 구성한다. (2026-11-08 WB04-E82)
    findTarget(x, y, type, direction, owner) {
        if (type >= this.#types.length) throw new RangeError('Gun 검색 타입 오류');
        const record = this.#types[type];
        const query = { x, y, centerX: fround(x - record.footX * 0.5), centerY: fround(y - record.footY * 0.5), type, direction, owner, frame: this.directionFrame(type, direction) };
        this.#validate(query);
        const group = record.group;
        if (group !== 0 && group !== 1) throw new Error('Gun FindTarget group 원본 assert');
        const result = group === 0 ? this.#findArcher(query) : this.#findCannon(query);
        if (result && this.#state.showRanges) {
            if (typeof this.#hooks.drawRange !== 'function') throw new Error('Gun 최종 표적 진단 출력 경계 누락');
            this.#hooks.drawRange(this.footprint(result), true);
        }
        return result;
    }

    // 썬 캐논의 현재 그림 프레임이 그 방향으로 발사할 수 있는지 먼저 검사한다. (2026-11-08 WB04-E82)
    mayFire(query, target) {
        this.#validate(query);
        if (query.type === this.#state.sunCannonType) {
            const center = this.center(target);
            const direction = direction4(query.centerX, query.centerY, center[0], center[1]);
            const frame = query.frame;
            if (frame < 0 || frame > 27) throw new RangeError('Gun 썬 캐논 발사 프레임 범위 오류');
            const allowed = frame < 24 ? frame === SUN_ALLOWED_FRAME[direction / 2] : direction === SUN_EXTRA_DIRECTION[frame - 24];
            if (!allowed) return false;
        }
        return this.mayTarget(query, target);
    }
}
