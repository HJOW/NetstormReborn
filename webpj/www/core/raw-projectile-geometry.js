// 썬 디스크 발사기 포구와 Missile 표적 치수/사각형의 원본 조회(발사기 포구 0046a780·가상 +b4 포구·캐논 사정선 끝 00481750·방향 프레임 0041d6e0·표적 상자 0049af10)를 옮긴 모듈이다(cpppj `o/RawProjectileGeometry.cpp/.h`, 10.78 전용).
// 표시 hotspot 과 포구 픽셀을 구분하며 현재 raw 프레임을 매번 읽고, 원본 x87 나눗셈/덧셈 뒤 float 저장 위치(Math.fround)와 현재 raw 프레임 폭을 보존한다.
//  - archerMuzzle(firing): TYPE 71(썬 디스크 발사기)만. TYPE 의 기본 프레임과 현재 프레임(+36 signed)의 SHP 포구 픽셀(signed short 쌍)을 먼저 정수로 더해 현재 배율로 나누고 현재 좌표에 더한다(TYPE hotspot 은 빼지 않음).
//  - muzzle(firing): 실제 가상 +b4 분배이다. 71 은 archerMuzzle, 10.78 의 128·129·131·140 은 현재 raw 좌표를 그대로 돌려준다. 그 밖의 TYPE 은 미복원이라 거부한다.
//  - cannonRayEnd(from, to, firing): 128/129/131 만. 목표 방향(세로 동률 우선 4방향)과 현재 프레임의 cardinal 방향(썬 캐논은 프레임 0..3·24..27, 아이스/썬더는 같은 현재 방향 getter)이 다르면 난수 없이 TYPE 사정거리의 정수 절삭(imul 하위 DWORD)으로 끝점을 [1, 255] clamp 하고,
//    세로(0/4)면 (출발 x, 끝 y), 가로면 (끝 x, 출발 y) 를 돌려준다.
//  - hotspot(firing): 현재 물리 SHP 포구 픽셀(기본 프레임과 합산하지 않음). directionFrame(from, to, type): 표시 타입의 균등 방향 반올림 뒤 방향당 물리 프레임 수를 곱한다. dimensions(target)/targetBounds(target, position): 실제 footX/footY 와 확장 높이가 있는 표적 상자이다.
// types[n] = { footX, footY }, geometry[n] = { defaultFrame, frameCount, directions, shotOffsets[[x,y]], heightExtension, range(float) }, scale 은 현재 BoardCoord 픽셀 배율(기본 16, 11), gunMode 는 { pool, get(sid) } 이다. 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-08 WB04-E85: 투사체 포구/치수 기하 이식으로 추가)

// 실제 10.78 방향 helper 의 근사 원주/직각/역원주 double 비트이다. (2026-11-08 WB04-E85)
const bits = new DataView(new ArrayBuffer(8));
const fromBits = (value) => { bits.setBigUint64(0, value, true); return bits.getFloat64(0, true); };
const TWO_PI = fromBits(0x401921f9f01b866en);
const HALF_PI = fromBits(0x3ff921f9f01b866en);
const INVERSE_TWO_PI = fromBits(0x3fc45f318e7adaf5n);
// 발사 가능 프레임 0..3 과 24..27 의 실제 cardinal 방향이다. (2026-11-08 WB04-E85)
const SUN_CANNON_FRAME_DIRECTIONS = Object.freeze([4, 0, 6, 2]);
const fround = Math.fround;

// 슬롯의 TYPE(+10)·프레임 DWORD(+36)·extra(+40)·좌표(+14/+18) 위치이다. (2026-11-08 WB04-E85)
const TYPE = 10;
const FRAME = 36;
const EXTRA = 40;
const X_AT = 14;
const Y_AT = 18;

// 현재 원본 little endian DWORD 를 정렬 가정 없이 읽는다. (2026-11-08 WB04-E85)
function read(raw, offset) {
    return (raw[offset] | (raw[offset + 1] << 8) | (raw[offset + 2] << 16) | (raw[offset + 3] << 24)) >>> 0;
}

// 비정렬 float 좌표를 읽는다(NaN payload 보존). (2026-11-08 WB04-E85)
const view = new DataView(new ArrayBuffer(4));
function coordinate(raw, offset) {
    for (let index = 0; index < 4; index += 1) view.setUint8(index, raw[offset + index]);
    return view.getFloat32(0, true);
}

// 원본 Rect min/max 는 들어오는 corner 만 1..255 에 clamp 한다. (2026-11-08 WB04-E85)
const clamp = (value) => (value < 1 ? 1 : 255 < value ? 255 : value);

export class RawProjectileGeometry {
    // 같은 풀·TYPE 발자국 표·SHP/기본 프레임 입력·현재 픽셀 배율·(선택) 현재 방향 getter 이다.
    #pool;
    #types;
    #geometry;
    #scaleX;
    #scaleY;
    #gunMode;

    // 실제 판본 전체 표(188)/유한 양수 픽셀 배율을 확인한다. 배율은 float32 로 저장한다. (2026-11-08 WB04-E85)
    constructor(pool, types, geometry, scaleX = 16, scaleY = 11, gunMode = null) {
        const x = fround(scaleX);
        const y = fround(scaleY);
        if (pool?.edition !== 'patch1078' || types.length !== 188 || geometry.length !== types.length || !Number.isFinite(x) || !Number.isFinite(y) || x <= 0 || y <= 0) throw new TypeError('Projectile geometry 판본/배율 오류');
        if (gunMode && gunMode.pool !== pool) throw new TypeError('Projectile geometry/방향 getter 풀 불일치');
        this.#pool = pool;
        this.#types = types;
        this.#geometry = geometry;
        this.#scaleX = x;
        this.#scaleY = y;
        this.#gunMode = gunMode;
    }

    // 같은 Pool 계약의 읽기 전용 확인이다. (2026-11-08 WB04-E85)
    get pool() { return this.#pool; }

    // 유효 실제 자산만 허용하며 free/dead 여부는 호출자 몸체가 판단한다. (2026-11-08 WB04-E85)
    #slot(sid) {
        if (!Number.isInteger(sid) || sid < 5 || sid >= this.#pool.capacity) throw new RangeError('Projectile geometry SID 오류');
        const raw = this.#pool.slot(sid);
        if (raw[TYPE] < 70 || raw[TYPE] >= this.#types.length) throw new RangeError('Projectile geometry 현재 TYPE 오류');
        return raw;
    }

    // 실제 포구 getter 의 abstract 인자는 진단에만 쓰며 반환 물리 번호에 레이어를 더하지 않는다. (2026-11-08 WB04-E85)
    #offset(type, frame) {
        const offsets = this.#geometry[type].shotOffsets;
        if (frame < 0 || frame >= offsets.length) throw new RangeError('Projectile 포구 SHP 물리 프레임 오류');
        return offsets[frame];
    }

    // 두 SHP 헤더의 signed short 를 먼저 정수로 더하며 타입 hotspot 을 추가로 빼지 않는다. (2026-11-08 WB04-E85)
    archerMuzzle(firing) {
        const raw = this.#slot(firing);
        if (raw[TYPE] !== 71) throw new Error('Projectile 포구의 썬 디스크 발사기 외 파생 미복원');
        const first = this.#offset(raw[TYPE], this.#geometry[raw[TYPE]].defaultFrame);
        const current = this.#offset(raw[TYPE], read(raw, FRAME) | 0);
        const x = coordinate(raw, X_AT);
        const y = coordinate(raw, Y_AT);
        return [fround((first[0] + current[0]) / this.#scaleX + x), fround((first[1] + current[1]) / this.#scaleY + y)];
    }

    // 실제 VT 50b888/50b6e8/50b9b8 의 +b4 는 각각 46a780/41c110/41c110 으로 포구를 반환한다. (2026-11-08 WB04-E85)
    muzzle(firing) {
        const raw = this.#slot(firing);
        if (raw[TYPE] === 71) return this.archerMuzzle(firing);
        // PATCH TYPE 128/129/131/140 의 +0xb4 getter(0041c110)는 원시 x/y 좌표를 그대로 복사한다.
        if (raw[TYPE] === 128 || raw[TYPE] === 129 || raw[TYPE] === 131 || raw[TYPE] === 140) return [coordinate(raw, X_AT), coordinate(raw, Y_AT)];
        throw new Error(`Projectile 실제 가상 포구 미복원 TYPE${raw[TYPE]}`);
    }

    // 실제 00481750 의 썬/썬더 분기이다. 방향 불일치는 난수 없이 TYPE 사정거리의 정수 변환으로 보정한다. (2026-11-08 WB04-E85)
    cannonRayEnd(from, to, firing) {
        const raw = this.#slot(firing);
        if (raw[TYPE] !== 128 && raw[TYPE] !== 129 && raw[TYPE] !== 131) throw new Error('Cannon rayEnd 미복원 판본/TYPE');
        if (![from[0], from[1], to[0], to[1]].every(Number.isFinite)) throw new TypeError('Cannon rayEnd 비유한 좌표');
        const dx = fround(to[0] - from[0]);
        const dy = fround(to[1] - from[1]);
        const direction = Math.abs(dx) <= Math.abs(dy) ? (dy > 0 ? 4 : 0) : (dx <= 0 ? 6 : 2);
        const frame = read(raw, FRAME) | 0;
        let frameDirection = -1;
        if (raw[TYPE] === 129 || raw[TYPE] === 131) {
            if (!this.#gunMode) throw new Error('아이스/썬더 사정선의 실제 방향 getter 미연결');
            frameDirection = this.#gunMode.get(firing) | 0;
        } else if (frame >= 0 && frame < 4) frameDirection = SUN_CANNON_FRAME_DIRECTIONS[frame];
        else if (frame >= 24 && frame < 28) frameDirection = SUN_CANNON_FRAME_DIRECTIONS[frame - 24];
        else throw new Error(`Cannon rayEnd 미복원 물리 프레임${frame}`);
        if (frameDirection !== 0 && frameDirection !== 2 && frameDirection !== 4 && frameDirection !== 6) throw new Error('캐논 사정선의 원본 cardinal 방향 진단');
        if (frameDirection !== direction) {
            const range = this.#geometry[raw[TYPE]].range;
            if (!Number.isFinite(range) || range < -2147483648 || range >= 2147483648) throw new RangeError('Cannon rayEnd 사정거리 정수 변환 범위 오류');
            const distance = Math.trunc(range);
            // 원본 imul 의 하위 DWORD 를 보존해 INT_MIN 의 서/북 방향 부호 반전도 정의된 연산으로 처리한다.
            const backwards = (0 - distance) | 0;
            const offsetX = frameDirection === 2 ? distance : frameDirection === 6 ? backwards : 0;
            const offsetY = frameDirection === 4 ? distance : frameDirection === 0 ? backwards : 0;
            to = [clamp(fround(offsetX + from[0])), clamp(fround(offsetY + from[1]))];
        }
        if (frameDirection === 0 || frameDirection === 4) return [from[0], to[1]];
        return [to[0], from[1]];
    }

    // 가상 포구 계산 뒤 Missile ctor 가 별도로 요구하는 현재 SHP 기준점이다(현재 물리 SHP header+20/+22, 기본 프레임과 합산하지 않음). (2026-11-08 WB04-E85)
    hotspot(firing) {
        const raw = this.#slot(firing);
        const offset = this.#offset(raw[TYPE], read(raw, FRAME) | 0);
        return [offset[0], offset[1]];
    }

    // 20칸 비균등 발사기 회전표와 달리 이 helper 는 표시 타입의 균등 방향을 사용한다(PATCH 0041d6e0). (2026-11-08 WB04-E85)
    directionFrame(from, to, type) {
        if (!Number.isInteger(type) || type < 0 || type >= this.#geometry.length) throw new RangeError('Projectile 방향 helper TYPE 오류');
        const data = this.#geometry[type];
        if (data.directions <= 0 || Math.trunc(data.frameCount / data.directions) <= 0) throw new Error('Projectile 방향/물리 프레임 자료 누락');
        if (![from[0], from[1], to[0], to[1]].every(Number.isFinite)) throw new TypeError('Projectile 비유한 방향 좌표');
        let angle = Math.atan2(-(to[1] - from[1]), to[0] - from[0]) - HALF_PI;
        if (angle < 0) angle += TWO_PI;
        const scaled = fround((TWO_PI - angle) * data.directions * INVERSE_TWO_PI);
        const direction = Math.trunc(scaled + 0.5) | 0;
        return ((direction % data.directions) * Math.trunc(data.frameCount / data.directions)) | 0;
    }

    // TYPE 실제 footX/footY 조회이다. 높이 확장은 ctor 의 치수 요청에서 사용하지 않는다(확장 플래그 0 의 현재 값). (2026-11-08 WB04-E85)
    dimensions(target) {
        const type = this.#types[this.#slot(target)[TYPE]];
        return [type.footX, type.footY];
    }

    // PATCH 0049af10: 기록된 표적 위치의 full foot/확장 높이 사각형이다. 원본 full 크기를 빼고 입력 위치가 유효하지 않으면 들어온 corner 로 바꾸는 Rect helper 를 보존한다. (2026-11-08 WB04-E85)
    targetBounds(target, position) {
        const number = this.#slot(target)[TYPE];
        const type = this.#types[number];
        const low = [clamp(fround(position[0] - type.footX)), clamp(fround(position[1] - (type.footY + this.#geometry[number].heightExtension)))];
        const valid = position[0] > 0 && position[1] > 0 && position[0] < 256 && position[1] < 256;
        return valid ? [Math.min(position[0], low[0]), Math.min(position[1], low[1]), Math.max(position[0], low[0]), Math.max(position[1], low[1])] : [low[0], low[1], low[0], low[1]];
    }
}
