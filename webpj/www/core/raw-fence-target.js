// 썬 바리케이드(fencemark)가 가로막는 투사체의 도착 좌표 계산(004571b0 Point·004568b0 Across·004819b0/004818d0 포구/시야·00462800 선분 교차)을 옮긴 모듈이다(cpppj `o/RawFenceTarget.cpp/.h`, 10.78 전용).
// 원본 검색/픽셀 좌표/교차 배리어의 계산을 빈 표적 좌표 콜백으로 대체하지 않으며, 원본 x87 나눗셈/덧셈 뒤 float 저장 위치(Math.fround)를 보존한다.
//  - intersection(a, b, c, d): 두 선분의 교차점이다. 수직/평행/사선별로 첫 절편과 결과 x/y 만 float 로 저장하며, 교차점이 양 선분의 포함 사각형 안(각 사각형의 min/min·max/max 대각 끝점은 제외)이 아니면 invalid(-10000,-10000)를 돌려준다.
//  - across(mark, firing, target): 한 fencemark 의 압축 끝점(QA WORD 와 QB 의 high24/low8)·동맹(같은 소유자이거나 방향별 동맹이면 invalid)·발자국·배리어 축 위 여부·Cannon/대각/픽셀 배율 보정·양쪽 분류를 거쳐 선분 교차와 최종 픽셀 보정 좌표를 계산한다.
//  - point(firing, target): 발사/표적 범위를 합친 뒤 현재 해시 단계의 칸 경계로 넓혀 일반 finder(flags 1)로 순회하며 fencemark 후보마다 across 를 구하고, 포구에 엄격히 가까운 유효 좌표(거리 float 저장)를 고른다(없으면 invalid).
//  - muzzle(firing)/sightBounds(firing): 현재 프레임의 SHP 포구 픽셀을 더하고 TYPE 핫스폿을 뺀 위치/발자국 두 corner. 모든 좌표는 현재 raw 와 현재 픽셀 배율에서 매 호출 다시 읽는다.
// types[n] = { footX, footY, group, hotspotX, hotspotY }, geometry[n] = { shotOffsets[[x,y]] }, target 은 같은 풀의 RawGunTarget, targeting = { scaleX, scaleY, angles }, process = { allies[9][9], allianceRead?(소유자, 대상) },
// fenceType 은 fencemark TYPE 번호(기본 90)이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E86: 썬 바리케이드 표적 이식으로 추가)
import { RawSquidFinder } from './raw-squid-finder.js';
import { SquidHash, SQUID_HASH_SCALES } from './squid-hash.js';
import { normalizeBoardAngle } from './raw-board-angle.js';

// 원본 BoardCoord 의 invalid 와 현재 hash 최댓값 경계·ceil 보정의 float 상수이다. (2026-11-08 WB04-E86)
const INVALID = Object.freeze([-10000, -10000]);
const HASH_EPSILON = Math.fround(0.001);
const CEIL_BIAS = Math.fround(0.9999);
const fround = Math.fround;
// 슬롯의 TYPE(+10)·소유자(+34)·프레임(+36)·QA WORD(+12)·QB 상위 DWORD(+26)/하위 BYTE(+30)·좌표(+14/+18) 위치이다. (2026-11-08 WB04-E86)
const TYPE = 10;
const OWNER = 34;
const FRAME = 36;
const QA = 12;
const QB_HIGH = 26;
const QB_LOW = 30;
const X_AT = 14;
const Y_AT = 18;
const PLAYER_SLOTS = 9;

// 비정렬 little endian 정수를 지정 폭대로 조립한다. (2026-11-08 WB04-E86)
function read(raw, offset, width = 4) {
    let value = 0;
    for (let index = 0; index < width; index += 1) value += raw[offset + index] * 2 ** (8 * index);
    return value >>> 0;
}

// 비정렬 float 를 읽는다(NaN payload 보존). (2026-11-08 WB04-E86)
const view = new DataView(new ArrayBuffer(4));
function coordinate(raw, offset) {
    for (let index = 0; index < 4; index += 1) view.setUint8(index, raw[offset + index]);
    return view.getFloat32(0, true);
}

// 실제 유효 좌표는 각 성분이 0 보다 크고 256 보다 작다. (2026-11-08 WB04-E86)
const valid = (point) => point[0] > 0 && point[0] < 256 && point[1] > 0 && point[1] < 256;

// std::clamp(v, 1, 255) 와 std::min/std::max 의 비교 순서이다. (2026-11-08 WB04-E86)
const clamp = (value) => (value < 1 ? 1 : 255 < value ? 255 : value);
const fmin = (a, b) => (b < a ? b : a);
const fmax = (a, b) => (a < b ? b : a);

// 원본 Rect min/max 는 들어오는 점을 1..255 로 자르고 기존 각 corner 의 invalid 를 별도로 처리한다. (2026-11-08 WB04-E86)
function include(rect, point) {
    point = [clamp(point[0]), clamp(point[1])];
    if (valid([rect.left, rect.top])) { rect.left = fmin(rect.left, point[0]); rect.top = fmin(rect.top, point[1]); } else { rect.left = point[0]; rect.top = point[1]; }
    if (valid([rect.right, rect.bottom])) { rect.right = fmax(rect.right, point[0]); rect.bottom = fmax(rect.bottom, point[1]); } else { rect.right = point[0]; rect.bottom = point[1]; }
}

// 원본 Geometry 의 포함은 경계 접촉을 허용하되 min/min·max/max 대각 끝점은 제외한다. (2026-11-08 WB04-E86)
function interior(rect, point) {
    return rect.left <= point[0] && point[0] <= rect.right && rect.top <= point[1] && point[1] <= rect.bottom &&
        (point[0] !== rect.left || point[1] !== rect.top) && (point[0] !== rect.right || point[1] !== rect.bottom);
}

// 0041ce90 의 좌표 차는 먼저 float 에 저장한다. cardinal 이면 Y 동률을 우선한다. (2026-11-08 WB04-E86)
function direction(from, to, cardinal) {
    const x = fround(to[0] - from[0]);
    const y = fround(to[1] - from[1]);
    if (cardinal) return Math.abs(x) <= Math.abs(y) ? (y > 0 ? 4 : 0) : (x <= 0 ? 6 : 2);
    if (x < 0) return y < 0 ? 7 : y > 0 ? 5 : 6;
    if (x > 0) return y < 0 ? 1 : y > 0 ? 3 : 2;
    return y < 0 ? 0 : y > 0 ? 4 : -1;
}

// 실제 BoardCoord.angle 의 float 입력 저장·double fpatan 환산·정규화 뒤 CRT 절삭이다. (2026-11-08 WB04-E86)
function angle(from, to, diagnostics) {
    if (from[0] === to[0] && from[1] === to[1]) return 0;
    const radians = Math.atan2(-(to[1] - from[1]), to[0] - from[0]);
    const degrees = fround(450 - radians * 57.295827908797776);
    return Math.trunc(normalizeBoardAngle(degrees, diagnostics ?? {})) | 0;
}

export class RawFenceTarget {
    // 압축 배리어 끝점과 현재 발사자/표적의 풀, 후보 순서/경계 도장용 해시, 발자국·핫스폿·group 표, 포구 WORD 자료, 중심 조회, 픽셀 배율/각도 진단, 동맹 표, fencemark 타입이다.
    #pool;
    #hash;
    #types;
    #geometry;
    #target;
    #targeting;
    #process;
    #fenceType;

    // 같은 Pool/TYPE 계약을 확인하며 CD 의 다른 fence 몸체를 PATCH 규칙으로 실행하지 않는다. (2026-11-08 WB04-E86)
    constructor(pool, hash, types, geometry, target, targeting, process, fenceType = 90) {
        if (pool?.edition !== 'patch1078' || !(hash instanceof SquidHash) || target?.pool !== pool || types.length !== 188 || geometry.length !== types.length) throw new TypeError('fencemark 표적의 판본/같은 Pool/TYPE 계약 오류');
        this.#pool = pool;
        this.#hash = hash;
        this.#types = types;
        this.#geometry = geometry;
        this.#target = target;
        this.#targeting = targeting;
        this.#process = process;
        this.#fenceType = fenceType;
    }

    // 현재 방향별 동맹 한 칸을 읽는다. 범위 밖은 거부한다. (2026-11-08 WB04-E86)
    #isAlly(owner, target) {
        const process = this.#process;
        if (owner >= PLAYER_SLOTS || target >= PLAYER_SLOTS) throw new RangeError('Gun 현재 방향별 동맹 범위 오류');
        return process.allianceRead ? process.allianceRead(owner, target) !== 0 : Boolean(process.allies[owner][target]);
    }

    // 원본 정상 자산 범위를 벗어나는 포인터 산술은 호스트에서 거부한다. (2026-11-08 WB04-E86)
    #slot(sid) {
        if (!Number.isInteger(sid) || sid < 5 || sid >= this.#pool.capacity) throw new RangeError('fencemark 표적 SID 오류');
        const raw = this.#pool.slot(sid);
        if (raw[TYPE] < 70 || raw[TYPE] >= this.#types.length) throw new RangeError('fencemark 표적 TYPE 오류');
        return raw;
    }

    // 현재 슬롯의 binary32 위치를 읽는다. (2026-11-08 WB04-E86)
    #position(sid) {
        const raw = this.#slot(sid);
        return [coordinate(raw, X_AT), coordinate(raw, Y_AT)];
    }

    // 원본 x87 나눗셈/덧셈 후 각 성분을 float 에 저장한다(현재 픽셀 배율을 매 호출 다시 읽음). (2026-11-08 WB04-E86)
    #add(point, x, y) {
        const targeting = this.#targeting;
        if (!Number.isFinite(targeting.scaleX) || !Number.isFinite(targeting.scaleY) || targeting.scaleX <= 0 || targeting.scaleY <= 0) throw new RangeError('fencemark 현재 픽셀 배율 오류');
        return [fround(x / targeting.scaleX + point[0]), fround(y / targeting.scaleY + point[1])];
    }

    // 발자국의 실제 마지막 칸까지 포함하는 flags 0 사각형이다. (2026-11-08 WB04-E86)
    #bounds(sid) {
        const position = this.#position(sid);
        const type = this.#types[this.#slot(sid)[TYPE]];
        const rect = { left: position[0], top: position[1], right: position[0], bottom: position[1] };
        include(rect, [fround(position[0] - (type.footX - 1)), fround(position[1] - (type.footY - 1))]);
        return rect;
    }

    // 현재 프레임의 SHP 포구 픽셀(signed short 쌍)을 읽는다. 범위 밖은 거부한다. (2026-11-08 WB04-E86)
    #shot(firing, message) {
        const raw = this.#slot(firing);
        const number = raw[TYPE];
        const frame = read(raw, FRAME) | 0;
        const offsets = this.#geometry[number].shotOffsets;
        if (frame < 0 || frame >= offsets.length) throw new RangeError(message);
        return [number, offsets[frame]];
    }

    // 일반 포구는 현재 SHP 좌표를 더한 뒤 TYPE 핫스폿을 빼며 아처 전용 기본 프레임 포구와 구별한다. (2026-11-08 WB04-E86)
    muzzle(firing) {
        const [number, offset] = this.#shot(firing, 'fencemark 현재 포구 SHP 프레임 오류');
        return this.#add(this.#add(this.#position(firing), offset[0], offset[1]), -this.#types[number].hotspotX, -this.#types[number].hotspotY);
    }

    // 실제 기본 발자국의 두 corner 에 현재 포구/TYPE 픽셀 보정을 각각 적용한다. (2026-11-08 WB04-E86)
    sightBounds(firing) {
        const [number, offset] = this.#shot(firing, 'fencemark 시야 SHP 프레임 오류');
        const rect = this.#bounds(firing);
        const type = this.#types[number];
        const low = this.#add(this.#add([rect.left, rect.top], offset[0], offset[1]), -type.hotspotX, -type.hotspotY);
        const high = this.#add(this.#add([rect.right, rect.bottom], offset[0], offset[1]), -type.hotspotX, -type.hotspotY);
        return { left: low[0], top: low[1], right: high[0], bottom: high[1] };
    }

    // 두 비수직 선의 첫 절편과 결과 x/y 만 float 로 저장하는 실제 x87 순서를 유지한다. (2026-11-08 WB04-E86)
    static intersection(a, b, c, d) {
        let point = [0, 0];
        if (a[0] === b[0] || c[0] === d[0]) {
            if (a[0] === b[0] && c[0] === d[0]) point = [b[0], b[1]];
            else {
                const vertical = a[0] === b[0] ? a : c;
                const first = a[0] === b[0] ? c : a;
                const last = a[0] === b[0] ? d : b;
                const slope = (last[1] - first[1]) / (last[0] - first[0]);
                point = [vertical[0], fround(slope * vertical[0] + (first[1] - first[0] * slope))];
            }
        } else {
            const first = (b[1] - a[1]) / (b[0] - a[0]);
            const second = (d[1] - c[1]) / (d[0] - c[0]);
            const intercept = fround(a[1] - a[0] * first);
            point[0] = fround(((c[1] - c[0] * second) - intercept) / (first - second));
            point[1] = fround(point[0] * first + intercept);
        }
        const firstRect = { left: a[0], top: a[1], right: a[0], bottom: a[1] };
        const secondRect = { left: c[0], top: c[1], right: c[0], bottom: c[1] };
        include(firstRect, b);
        include(secondRect, d);
        return interior(firstRect, point) && interior(secondRect, point) ? point : [INVALID[0], INVALID[1]];
    }

    // 실제 fencemark 의 WORD 끝점과 분리된 high24/low8 끝점을 원본 순서로 읽는다. (2026-11-08 WB04-E86)
    across(mark, firing, target) {
        const raw = this.#slot(mark);
        const firingRaw = this.#slot(firing);
        const firstSid = read(raw, QA, 2);
        const firstRaw = this.#slot(firstSid);
        const owner = firstRaw[OWNER];
        const firingOwner = firingRaw[OWNER];
        // 동맹 조회 전에 원본 압축 끝점 번호를 보존한다.
        const secondId = ((read(raw, QB_HIGH) << 8) | raw[QB_LOW]) >>> 0;
        if (owner >= PLAYER_SLOTS || firingOwner >= PLAYER_SLOTS) throw new RangeError('fencemark 동맹 Player 번호 오류');
        if (owner === firingOwner || this.#isAlly(firingOwner, owner)) return [INVALID[0], INVALID[1]];
        if (secondId > 65535) throw new RangeError('fencemark 압축 끝점 SID 오류');
        const originalA = this.#position(firstSid);
        const originalB = this.#position(secondId);
        const firingPosition = this.#position(firing);
        const targetPosition = this.#position(target);
        let a = originalA;
        let b = originalB;
        const way = direction(a, b, true);
        const axis = (way === 2 || way === 6) ? 1 : 0;
        const targetType = this.#types[this.#slot(target)[TYPE]];
        const firingType = this.#types[firingRaw[TYPE]];
        let width = targetType.footX;
        let height = targetType.footY;
        const extent = axis ? height : width;
        if (firingPosition[axis] === a[axis]) return [INVALID[0], INVALID[1]];
        if (extent === 3 && targetPosition[axis] === a[axis]) return [INVALID[0], INVALID[1]];
        // 폭/높이 3보다 큰 표적이 배리어 축에 걸친 원본 내부 칸을 검사한다.
        for (let index = 0; extent > 3 && index < extent - 2; index += 1) {
            if (a[axis] === targetPosition[axis] - index) return [INVALID[0], INVALID[1]];
        }
        const cannon = firingType.group === 1;
        if (cannon && (axis ? (a[0] === firingPosition[0] || b[0] === firingPosition[0]) : (a[1] === firingPosition[1] || b[1] === firingPosition[1]))) return [INVALID[0], INVALID[1]];
        if (axis) {
            if (a[0] <= b[0]) { b = this.#add(b, -32, -11); a = this.#add(a, 0, -11); } else { a = this.#add(a, -32, -11); b = this.#add(b, 0, -11); }
        } else if (a[1] <= b[1]) { b = this.#add(b, -16, -22); a = this.#add(a, -16, 0); } else { a = this.#add(a, -16, -22); b = this.#add(b, -16, 0); }
        if (width === 1 && height === 1) { width = 2; height = 2; }
        // 004567f0 의 배리어 띠/발자국 겹침은 반환 AL 의 참/거짓만 사용한다.
        const overlaps = (position, footX, footY) => {
            const edge = this.#add(originalA, axis ? 0 : -32, axis ? -22 : 0);
            const low = this.#add(position, axis ? 0 : footX * -16 + 16, axis ? footY * -11 + 11 : 0);
            return position[axis] >= edge[axis] && low[axis] <= originalA[axis];
        };
        const firingDirection = direction(originalA, firingPosition, false);
        const targetDirection = direction(originalA, targetPosition, false);
        // 원본 8방향의 배리어 양쪽 분류이며 축 위는 -1 이다.
        const side = (way8) => {
            if (axis) return way8 === 0 || way8 === 1 || way8 === 7 ? 1 : way8 === 3 || way8 === 4 || way8 === 5 ? 0 : -1;
            return way8 === 1 || way8 === 2 || way8 === 3 ? 0 : way8 === 5 || way8 === 6 || way8 === 7 ? 1 : -1;
        };
        const firstSide = side(targetDirection);
        const secondSide = side(firingDirection);
        const firingSide = secondSide === 1;
        let ambiguous = firstSide < 0 || secondSide < 0;
        const targetSide = firstSide === 1 ? 1 : 0;
        if (overlaps(targetPosition, width, height) || overlaps(firingPosition, firingType.footX, firingType.footY)) {
            let nearTarget = targetPosition;
            if (!targetSide) nearTarget = this.#add(nearTarget, axis ? 0 : width * -16 + 16, axis ? height * -11 + 11 : 0);
            if (a[axis] === nearTarget[axis]) ambiguous = true;
            let nearFiring = firingPosition;
            if (!firingSide) nearFiring = this.#add(nearFiring, axis ? 0 : firingType.footX * -16 + 16, axis ? firingType.footY * -11 + 11 : 0);
            if (a[axis] === nearFiring[axis]) return [INVALID[0], INVALID[1]];
            if (targetSide === (firingSide ? 1 : 0) && !ambiguous) return [INVALID[0], INVALID[1]];
        }
        const targetCenter = this.#target.center(target);
        const firingCenter = this.#target.center(firing);
        const point = RawFenceTarget.intersection(a, b, firingCenter, targetCenter);
        if (!valid(point)) return point;
        const degrees = angle(point, targetCenter, this.#targeting.angles);
        if (axis) return this.#add(point, 0, firingSide && !cannon && degrees < 192 ? -11 : -22);
        if (cannon) { point[1] = firingCenter[1]; return this.#add(point, 0, -15); }
        if (!firingSide) return this.#add(point, 0, -11);
        return degrees <= 60 ? this.#add(point, -16, 0) : point;
    }

    // 실제 발사/표적 범위를 합친 뒤 현재 해시 단계 경계에 도장하고 일반 finder 1 로 순회한다. (2026-11-08 WB04-E86)
    point(firing, target) {
        const rect = this.sightBounds(firing);
        const muzzle = this.muzzle(firing);
        const bounds = this.#bounds(target);
        include(rect, [bounds.left, bounds.top]);
        include(rect, [bounds.right, bounds.bottom]);
        const level = this.#hash.selected;
        const scale = SQUID_HASH_SCALES[level];
        const last = SquidHash.side(level) - 1;
        // 00432580 의 절삭/정수 나눗셈이다.
        const lower = (value) => scale * Math.min(Math.trunc(Math.trunc(value) / scale), last);
        const low = [fround(lower(rect.left)), fround(lower(rect.top))];
        const high = [fround(lower(rect.right) + scale - HASH_EPSILON), fround(lower(rect.bottom) + scale - HASH_EPSILON)];
        include(rect, low);
        include(rect, high);
        // 원본 오른쪽/아래 int 사각형 변환이다.
        const ceil = (value) => Math.trunc(value + CEIL_BIAS);
        const finder = new RawSquidFinder(this.#pool, this.#hash, this.#types);
        let result = [INVALID[0], INVALID[1]];
        let best = 10000;
        // 실제 해시/체인 순서에서 현재 float 최솟값과 거리를 엄격히 비교하고 선택 후 float 에 저장한다.
        for (let sid = finder.begin({ left: Math.trunc(rect.left), top: Math.trunc(rect.top), right: ceil(rect.right), bottom: ceil(rect.bottom) }, 1); sid; sid = finder.next()) {
            if (this.#pool.slot(sid)[TYPE] !== this.#fenceType) continue;
            const candidate = this.across(sid, firing, target);
            if (!valid(candidate)) continue;
            const x = candidate[0] - muzzle[0];
            const y = candidate[1] - muzzle[1];
            const distance = Math.sqrt(x * x + y * y);
            if (distance < best) { best = fround(distance); result = candidate; }
        }
        return result;
    }
}
