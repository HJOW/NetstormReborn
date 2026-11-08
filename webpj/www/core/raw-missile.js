// Missile.cpp 의 투사체 생성자·프레임(썬 캐논·썬더·아이스 캐논·썬 디스크 발사기·크로스보우의 투사체, 10.78 반사 포함)을 옮긴 모듈이다(cpppj `o/RawMissile.cpp/.h`, 10.78 전용).
// 원본 128바이트 객체 이미지(MissileFields)의 필드 산술과 표시 자산 생성/공간 이동/피해/삭제 순서를 보존한다. 자산 생성·Pop·소리·효과·피해·삭제는 모두 훅이다.
//  - construct(fields, request): 발사 자산의 타입으로 group0(썬 디스크 71·크로스보우 140)/group1(썬 캐논 128·썬더 131·아이스 129) 또는 반사(ricochet≠0, 아이스 반사 표시 130)를 고르고 요청 좌표·속도·거리를
//    검증한 뒤 필드(가상 표 0x50d630·발사/표적 SID·속도·반사·초기 사각형/이전 좌표 −10000·코너 여부·표적 중심·종점·원점·허용 오차·시작/종료 시각)를 쓴다.
//    원점은 포구(훅) 또는 요청 포구에 SHP/타입 hotspot 보정을 더하고(썬 디스크 71 은 보정 생략), 종점은 반사면 표적 중심, group0 이면 코너/표적 중심, group1 이면 사정선 끝점(훅)이며 모두 [1,255] clamp 한다.
//    이어 표시 자산을 만들어(훅) 방향 프레임(썬 디스크는 1)·소유자·발사 소리·Pop·비행 효과를 순서대로 수행하고 표시 자산 SID 를 반환한다.
//  - frame(fields, alive): 진단 모드면 0041be80 의 두 SID 진단을 실행하고, 비행 시각 비율로 현재 위치를 구해 Unpop/Pop 으로 옮기고 표적 사각형(코너면 코너 사각형)과 이전→현재 선분의 겹침으로 충돌을 판정한다.
//    클라우드 플로터 표적은 공유 난수로 회피할 수 있으며, 충돌 효과(코너 배리어/아이스 전용/일반)·반사 생성·썬더×스톤 타워 알림 큐를 수행하고 이전 좌표를 저장한 뒤, 충돌이거나 사거리가 끝났으면
//    발사 자산의 현재 group 에 따라 총 직접 피해/사거리 갱신 또는 반사 피해를 전달하고 종료 효과 뒤 표시 자산을 삭제한다. 진단이 현재 프로세스를 삭제하면(alive()가 거짓) 남은 필드를 읽지 않는다.
//  - MissileProcess(SquidProcess): 생성된 필드와 처리기를 묶은 Kernel 프로세스이다. runFrame 은 frame, onAttached 는 Form/Parent 기록이다.
// state(MissileState)·types[n]={group,hotspotX,hotspotY,flags2,…}·missileTypes[n]={range,frameCount,firingSound,impactSound,damagePerSecond,specialFlyerDamage,flyerDamagePerSecond} 는 호출 시점에 읽는다.
// CD 10.72 의 0x68 필드 이미지와 BYTE 프레임은 제외한다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E125: RawMissile 이식으로 추가)
import { SquidProcess } from './squid-process.js';
import { MissileFields } from './raw-missile-fields.js';

// 원본 Baseprocess 와 공통 발사/표적/코너 필드 위치, 10.78 Missile 필드 위치(속도·허용 오차·원점·위치·종점·사각형·종료/시작 시각·이전 좌표)이다. (2026-11-08 WB04-E125)
const FORM = 4;
const PARENT = 8;
const RICOCHET = 12;
const PASSED = 16;
const FIRING = 20;
const TARGET = 24;
const CORNER = 28;
const SPEED = 48;
const ALLOWANCE = 52;
const ORIGIN = 56;
const POSITION = 64;
const ENDPOINT = 72;
const BOX = 88;
const END = 104;
const START = 112;
const PREVIOUS = 120;
const TYPE_COUNT = 188;
const AIRBORNE = 0x120000;
const fround = Math.fround;
const floatView = new DataView(new ArrayBuffer(4));
const doubleView = new DataView(new ArrayBuffer(8));

// 비정렬 리틀엔디언 필드를 읽고(부호 없음) 주변 padding 을 보존하며 쓴다. (2026-11-08 WB04-E125)
function read(bytes, offset, width = 4) {
    let value = 0;
    // 현재 원본 필드 폭만 조합한다.
    for (let index = 0; index < width; index += 1) value += bytes[offset + index] * 2 ** (8 * index);
    return value;
}
function put(bytes, offset, value, width = 4) {
    const wrapped = value >>> 0;
    // 낮은 바이트부터 지정한 필드만 쓴다.
    for (let index = 0; index < width; index += 1) bytes[offset + index] = (wrapped >>> (8 * index)) & 0xff;
}

// float32 좌표와 double 시각의 비트를 그대로 읽고 쓴다. (2026-11-08 WB04-E125)
function readFloat(bytes, offset) {
    floatView.setUint32(0, read(bytes, offset), true);
    return floatView.getFloat32(0, true);
}
function putFloat(bytes, offset, value) {
    floatView.setFloat32(0, value, true);
    put(bytes, offset, floatView.getUint32(0, true));
}
function readDouble(bytes, offset) {
    doubleView.setUint32(0, read(bytes, offset), true);
    doubleView.setUint32(4, read(bytes, offset + 4), true);
    return doubleView.getFloat64(0, true);
}
function putDouble(bytes, offset, value) {
    doubleView.setFloat64(0, value, true);
    put(bytes, offset, doubleView.getUint32(0, true));
    put(bytes, offset + 4, doubleView.getUint32(4, true));
}

// 현재 raw 좌표 쌍을 조회/저장한다. (2026-11-08 WB04-E125)
function point(bytes, offset) { return [readFloat(bytes, offset), readFloat(bytes, offset + 4)]; }
function putPoint(bytes, offset, at) {
    putFloat(bytes, offset, at[0]);
    putFloat(bytes, offset + 4, at[1]);
}

// 정상 BoardCoord 의 유효 범위(NaN 거부)와 clamp 를 원본 경계로 적용한다. (2026-11-08 WB04-E125)
function valid(at) { return at[0] > 0 && at[1] > 0 && at[0] < 256 && at[1] < 256; }
function clampValue(value) { return value < 1 ? 1 : 255 < value ? 255 : value; }
function clamp(at) { return [clampValue(at[0]), clampValue(at[1])]; }

// 원본 Distance 의 정상 유한 좌표 Euclidean 길이이다. (2026-11-08 WB04-E125)
function distance(first, second) {
    const dx = first[0] - second[0];
    const dy = first[1] - second[1];
    return Math.sqrt(dx * dx + dy * dy);
}

// 실제 __ftol 의 64비트 절삭 뒤 하위 DWORD 를 보존한다. 비유한/넘침은 원본 정수 불확정값의 하위 0 이다. (2026-11-08 WB04-E125)
function damageInteger(value) {
    if (!Number.isFinite(value) || value >= 9223372036854775808 || value < -9223372036854775808) return 0;
    return Number(BigInt.asIntN(32, BigInt(Math.trunc(value))));
}

// 선택된 실제 하위만 요구한다. (2026-11-08 WB04-E125)
function need(hook, message) {
    if (typeof hook !== 'function') throw new Error(message);
    return hook;
}

// 0041be80 의 SID 범위 진단이다: Form/부모가 풀 범위 밖이면 진단을 올린 뒤(복귀하면) 계속하며 항상 true 를 반환한다. 사용: runPathBaseRobust({ form, parent, capacity, assertion }). (2026-11-08 WB04-E125)
export function runPathBaseRobust(hooks) {
    const form = need(hooks.form, '공통 도착의 선택된 실제 경계 미연결')();
    let capacity = need(hooks.capacity, '공통 도착의 선택된 실제 경계 미연결')();
    if (form && form >= capacity) {
        need(hooks.assertion, '공통 도착의 선택된 실제 경계 미연결')('formSid == 0 || form.isValid()', '\\Ns\\O\\Baseprocess.cpp', 0x69);
        capacity = hooks.capacity();
    }
    const parent = need(hooks.parent, '공통 도착의 선택된 실제 경계 미연결')();
    if (parent && parent >= capacity) need(hooks.assertion, '공통 도착의 선택된 실제 경계 미연결')('attachToSid == 0 || s.isValid()', '\\Ns\\O\\Baseprocess.cpp', 0x6c);
    return true;
}

// 같은 실제 게임 시각과 원본 전역 타입 번호이다. 호출 시점에 읽는다. 사용: new MissileState({ now: 10 }). (2026-11-08 WB04-E125)
export class MissileState {
    // 10.78 의 두 Handle 진단 관문(005e4794 같은 DWORD 의 비영 여부)이며 불리언 또는 함수이다.
    checking = false;
    // 정지 시 유지되는 현재 게임 시각이다.
    now = 0;
    // 캐논 128 · 표시 sunDisc 83 · 발사기 71 · 크로스보우 140(group0, 포구 보정은 71 과 구별) · 썬더 캐논 131 · 썬더 표시 132 · 스톤 타워 78 의 원본 타입이다.
    sunCannonType = 128;
    sunDiscType = 83;
    sunArcherType = 71;
    crossbowType = 140;
    thunderCannonType = 131;
    thunderMissileType = 132;
    stoneTowerType = 78;
    // 충돌 명령 토큰(005429c8)과 사거리 종료 여유(일반 4·썬더 10, 정수)이다.
    hitCommand = 0x020000b5;
    rangeMargin = 4;
    thunderRangeMargin = 10;
    // 실제 missileProcessType(28), incomingCorner 충돌의 공통 타입 사운드 번호 136, 현재 타일별 픽셀 배율이다.
    processType = 28;
    cornerImpactType = 136;
    scaleX = 16;
    scaleY = 11;
    // 초기 아이스 발사자 129 와 충돌/반사 표시 130, 클라우드 플로터 146, 회피 기준(부호 있는 DWORD, 난수 0..1000 과 비교)이다.
    rainCannonType = 129;
    iceMissileType = 130;
    floaterType = 146;
    avoidanceCutoff = 800;

    // 지정한 값만 덮어쓴다. (2026-11-08 WB04-E125)
    constructor(init = {}) { Object.assign(this, init); }
}

// 초기 총 발사와 10.78 아이스 반사의 실제 숫자 필드·표시 자산 수명/이동/충돌 사슬이다. 사용: new RawMissile(pool, types, missileTypes, state, hooks). (2026-11-08 WB04-E125)
export class RawMissile {
    // 실제 표시 자산/발사 건물/표적이 공유하는 저장소, group/hotspot/발자국 타입 표, 사운드 토큰/방향 수 보충 자료, 현재 게임 시각/원본 타입, 실제 월드 효과 훅이다.
    #pool;
    #types;
    #missileTypes;
    #state;
    #hooks;

    // 같은 원본 판본(10.78) 타입 표와 사운드/방향 보충 자료만 받는다. (2026-11-08 WB04-E125)
    constructor(pool, types, missileTypes, state, hooks = {}) {
        if (pool?.edition !== 'patch1078' || !types || types.length !== TYPE_COUNT || !missileTypes || missileTypes.length !== types.length) throw new Error('Missile 타입 판본/보충 자료 크기 오류');
        if (!Number.isFinite(state.scaleX) || !Number.isFinite(state.scaleY) || state.scaleX <= 0 || state.scaleY <= 0 || state.cornerImpactType >= types.length) throw new Error('Missile 픽셀 배율/충돌 타입 오류');
        this.#pool = pool;
        this.#types = types;
        this.#missileTypes = missileTypes;
        this.#state = state;
        this.#hooks = hooks;
    }

    // 동일한 월드인지 검증하는 adapter 자료이다. (2026-11-08 WB04-E125)
    get pool() { return this.#pool; }
    get processType() { return this.#state.processType; }

    // 원본 SID/타입 범위를 검증하되 free/dead 는 각 실제 body 분기가 판단한다. 슬롯 주소를 잡아 두므로 훅 뒤 바뀐 바이트를 그대로 읽는 live 뷰를 돌려준다. (2026-11-08 WB04-E125)
    #slot(sid, allowZero = false) {
        if ((!sid && !allowZero) || sid >= this.#pool.capacity) throw new RangeError('Missile SID 범위 오류');
        const raw = this.#pool.physical(sid);
        if (raw[10] >= this.#types.length) throw new RangeError('Missile 타입 범위 오류');
        return raw;
    }

    // 부모 표시 자산의 현재 frame DWORD 로 쓴다. (2026-11-08 WB04-E125)
    #frameNumber(sid, frame) { put(this.#pool.allocated(sid), 36, frame); }

    // 진단 모드가 켜져 있으면 0041be80 의 두 SID 진단을 실행한다(진단이 현재 프로세스를 삭제하면 false). (2026-11-08 WB04-E125)
    #checkBase(fields, alive) {
        if (!alive()) return false;
        const checking = this.#state.checking;
        if (!(typeof checking === 'function' ? checking() : checking)) return true;
        const capacity = this.#pool.capacity;
        runPathBaseRobust({
            form: () => (alive() ? read(fields.bytes, FORM) : 0),
            parent: () => (alive() ? read(fields.bytes, PARENT) : 0),
            capacity: () => capacity,
            assertion: this.#hooks.assertion
        });
        return alive();
    }

    // 실제 ctor 순서에 따라 숫자 준비와 표시 자산 생성/Pop/비행효과를 수행한다. 반환은 실제 Missile 표시 자산 SID 이다(이어지는 Host 부착은 factory 가 완료). 사용: construct(fields, request). (2026-11-08 WB04-E125)
    construct(fields, request) {
        const state = this.#state;
        const hooks = this.#hooks;
        const types = this.#types;
        const reflected = request.ricochet !== 0;
        const firing = this.#slot(request.firing);
        const target = this.#slot(request.target, reflected);
        const archer = firing[10] === state.sunArcherType;
        // group0 탄도와 썬 디스크 발사기 71 전용 포구 보정 생략을 독립 판정한다.
        const groupZero = archer || firing[10] === state.crossbowType;
        // 같은 group1 의 초기 아이스 129 도 원문 수치/표시 생성 순서를 사용한다.
        const cannon = firing[10] === state.sunCannonType || firing[10] === state.thunderCannonType || firing[10] === state.rainCannonType;
        if (reflected ? firing[10] !== state.iceMissileType : (request.ricochet || (!groupZero && !cannon) || valid(request.firingPosition))) throw new Error(`Missile 미복원 반사/부모/포구 생성 경로 TYPE${firing[10]}`);
        if (!reflected && types[firing[10]].group !== (groupZero ? 0 : 1)) throw new Error('Missile 실제 부모 group 계약 오류');
        if (!Number.isFinite(request.speed) || request.speed <= 0 || !valid(request.targetPosition)) throw new RangeError('Missile 생성 좌표/속도 오류');
        if ((!valid(request.firingPosition) && !hooks.muzzle) || (!reflected && !archer && !hooks.hotspot) || (!reflected && !groupZero && !hooks.rayEnd)
            || (target[10] >= 10 && !hooks.targetDimensions) || !hooks.create || !hooks.owner || !hooks.pop || (request.effectType && !hooks.flightEffect)) {
            throw new Error(`Missile 실제 생성/Pop/비행 효과 경계 누락 TYPE${firing[10]} effect${request.effectType}`);
        }
        if (request.displayType >= types.length) throw new RangeError('Missile 표시 타입 오류');
        const bytes = fields.bytes;
        put(bytes, 0, 0x50d630);
        put(bytes, FORM, 0);
        put(bytes, PARENT, 0);
        // 원본 초기 사각형과 직전 위치는 invalid 좌표 -10000 이다.
        for (let offset = BOX; offset < BOX + 16; offset += 4) putFloat(bytes, offset, -10000);
        putDouble(bytes, END, 0);
        putDouble(bytes, START, 0);
        put(bytes, FIRING, request.firing);
        put(bytes, TARGET, request.target);
        putFloat(bytes, SPEED, request.speed);
        put(bytes, RICOCHET, request.ricochet);
        put(bytes, PASSED, 0);
        putPoint(bytes, PREVIOUS, [-10000, -10000]);
        const corner = !reflected && valid(request.targetCenter);
        // 패치의 +32/+36 초기화는 group1 코너 분기에만 있으며 group0 의 기존 바이트는 보존한다.
        if (corner && !groupZero) {
            put(bytes, 32, 0);
            put(bytes, 36, 0);
        }
        put(bytes, 40, 0);
        put(bytes, 44, 0);
        putPoint(bytes, 80, request.targetCenter);
        let origin = valid(request.firingPosition) ? request.firingPosition.slice() : hooks.muzzle(request.firing).slice();
        const type = types[firing[10]];
        // 썬 디스크 71 은 원본처럼 SHP/type hotspot 보정을 생략한다. 기존 128 의 두 float 저장은 그대로다.
        if (!reflected && !archer) {
            const hot = hooks.hotspot(request.firing);
            origin[0] = fround(origin[0] + hot[0] / fround(state.scaleX));
            origin[1] = fround(origin[1] + hot[1] / fround(state.scaleY));
            origin[0] = fround(origin[0] - type.hotspotX / fround(state.scaleX));
            origin[1] = fround(origin[1] - type.hotspotY / fround(state.scaleY));
        }
        // 원문 target0 은 기본 3×3 치수로 보정한다.
        const dimensions = target[10] < 10 ? [3, 3] : hooks.targetDimensions(request.target);
        const targetCenter = [fround(request.targetPosition[0] - dimensions[0] * 0.5), fround(request.targetPosition[1] - dimensions[1] * 0.5)];
        putPoint(bytes, ENDPOINT, targetCenter);
        putPoint(bytes, POSITION, corner ? request.targetCenter : request.targetPosition);
        put(bytes, CORNER, corner ? 1 : 0);
        // group0 코너의 종점은 패치에서 코너이다.
        const endpoint = clamp(reflected ? targetCenter : (groupZero ? (corner ? request.targetCenter : targetCenter) : hooks.rayEnd(origin, corner ? request.targetCenter : targetCenter, request.firing)));
        origin = clamp(origin);
        putPoint(bytes, ENDPOINT, endpoint);
        putPoint(bytes, ORIGIN, origin);
        if (!valid(origin) || !valid(endpoint) || ((firing[11] & 1) && (target[11] & 1))) throw new Error('Missile 실제 생성 좌표/자산 원본 assert');
        putFloat(bytes, ALLOWANCE, 0);
        const length = fround(distance(origin, endpoint));
        if (length < 0 || length >= 256) throw new RangeError('Missile 생성 거리 원본 assert');
        putDouble(bytes, START, state.now);
        putDouble(bytes, END, length / request.speed + state.now);
        const asset = hooks.create(request.displayType, 2);
        this.#slot(asset);
        const display = this.#missileTypes[request.displayType];
        if (display.frameCount > 0) {
            if (request.displayType === state.sunDiscType) this.#frameNumber(asset, 1);
            else {
                need(hooks.directionFrame, 'Missile 방향 프레임 경계 누락');
                this.#frameNumber(asset, hooks.directionFrame(origin, endpoint, request.displayType) >>> 0);
            }
        }
        hooks.owner(asset, firing[34]);
        const sound = this.#missileTypes[firing[10]].firingSound ? this.#missileTypes[firing[10]].firingSound : display.firingSound;
        if (sound) need(hooks.firingSound, 'Missile 실제 발사 사운드 경계 누락')(origin[0], origin[1], sound);
        hooks.pop(asset, origin[0], origin[1], 0);
        if (request.effectType) hooks.flightEffect(request.effectType, asset);
        return asset;
    }

    // 원문 첫 진단 뒤의 저장 SID 와 두 번째 진단 뒤의 발사자 재조회를 구별한다. alive 는 호출 중 자기 삭제를 관찰하는 함수(생략하면 항상 참)이다. 사용: frame(fields, alive). (2026-11-08 WB04-E125)
    frame(fields, alive = () => true) {
        const state = this.#state;
        const hooks = this.#hooks;
        const types = this.#types;
        const missileTypes = this.#missileTypes;
        if (!this.#checkBase(fields, alive)) return;
        const reflected = read(fields.bytes, RICOCHET) !== 0;
        const assetSid = fields.parent;
        const firing = this.#slot(fields.firing);
        const target = this.#slot(fields.target, reflected);
        const asset = this.#slot(assetSid);
        const archer = firing[10] === state.sunArcherType;
        // 원본 group0 사거리/코너 종료는 크로스보우에도 같고 71 전용 포구와 무관하다.
        const groupZero = archer || firing[10] === state.crossbowType;
        const cannon = firing[10] === state.sunCannonType || firing[10] === state.thunderCannonType || firing[10] === state.rainCannonType;
        if (!reflected && ((!groupZero && !cannon) || read(fields.bytes, RICOCHET))) throw new Error('Missile 미복원 frame 파생/반사 경로');
        // 원문 빗나감 요청은 첫 프레임에 소비하고 종료 음향 표시를 유지한다.
        if (read(fields.bytes, 40)) {
            put(fields.bytes, 40, 0);
            put(fields.bytes, 44, 1);
        }
        // 반사는 일반 4 여유를 더하지 않는다.
        const margin = fround(asset[10] === state.thunderMissileType ? state.thunderRangeMargin : (reflected ? 0 : state.rangeMargin));
        if (!hooks.unpop || !hooks.pop || !hooks.destroy || (!reflected && !hooks.center)) throw new Error('Missile 실제 이동/삭제 경계 누락');
        const origin = point(fields.bytes, ORIGIN);
        const endpoint = point(fields.bytes, ENDPOINT);
        const end = readDouble(fields.bytes, END);
        const start = readDouble(fields.bytes, START);
        const remaining = end - state.now;
        const duration = end - start;
        const dx = fround(endpoint[0] - origin[0]);
        const dy = fround(endpoint[1] - origin[1]);
        const factor = 1.0 / duration;
        const current = [fround(endpoint[0] - (dx * factor) * remaining), fround(endpoint[1] - (factor * dy) * remaining)];
        if (!valid(current)) {
            if (read(fields.bytes, 44)) {
                need(hooks.impactSound, 'Missile 빗나감 종료 음향 미연결')(current[0], current[1], missileTypes[state.cornerImpactType].impactSound);
                if (!alive()) return;
            }
            hooks.destroy(assetSid, 0);
            return;
        }
        hooks.unpop(assetSid, 0);
        if (!alive()) return;
        hooks.pop(assetSid, current[0], current[1], 0);
        if (!alive()) return;
        const corner = read(fields.bytes, CORNER) !== 0;
        if (fields.target && !(target[11] & 3)) {
            if (corner) {
                const at = point(fields.bytes, ENDPOINT);
                putFloat(fields.bytes, BOX, fround(at[0] - 1.0));
                putFloat(fields.bytes, BOX + 4, fround(at[1] - 1.0));
                putFloat(fields.bytes, BOX + 8, at[0]);
                putFloat(fields.bytes, BOX + 12, at[1]);
            } else {
                const bounds = need(hooks.targetBounds, 'Missile 실제 표적 발자국 경계 누락')(fields.target, point(fields.bytes, POSITION));
                const allowance = readFloat(fields.bytes, ALLOWANCE);
                putFloat(fields.bytes, BOX, fround(bounds[0] - allowance));
                putFloat(fields.bytes, BOX + 4, fround(bounds[1] - allowance));
                putFloat(fields.bytes, BOX + 8, fround(bounds[2] + allowance));
                putFloat(fields.bytes, BOX + 12, fround(bounds[3] + allowance));
            }
        }
        const previous = point(fields.bytes, PREVIOUS);
        let hit = false;
        if (valid(previous) && !read(fields.bytes, PASSED) && !(target[40] & 0x10)) {
            const low = [Math.min(previous[0], current[0]), Math.min(previous[1], current[1])];
            const high = [Math.max(previous[0], current[0]), Math.max(previous[1], current[1])];
            const boxLow = point(fields.bytes, BOX);
            const boxHigh = point(fields.bytes, BOX + 8);
            hit = low[0] <= boxHigh[0] && boxLow[0] <= high[0] && low[1] <= boxHigh[1] && boxLow[1] <= high[1];
        }
        const currentCenter = reflected ? origin : hooks.center(fields.firing);
        const currentDistance = fround(distance(current, currentCenter));
        let exhausted;
        if (reflected) exhausted = fround(missileTypes[asset[10]].range + margin) < currentDistance;
        else if (corner) {
            const endCenter = hooks.center(fields.firing);
            exhausted = !groupZero ? distance(endpoint, endCenter) < currentDistance : fround(fround(distance(endpoint, currentCenter)) + margin) < distance(current, endCenter);
        } else {
            const range = missileTypes[firing[10]].range;
            exhausted = fround(range + margin) < currentDistance;
        }
        let last = current;
        if (hit && target[10] === state.floaterType) {
            // 원문은 현재 cutoff 를 난수 반환 뒤 FILD 로 읽으며 float 로 줄이지 않는다. 회피는 이후 충돌 검사를 막되 현재 위치/사거리 종료는 계속 처리한다.
            const roll = need(hooks.randomFloat, 'Missile 클라우드 플로터의 실제 공유 난수 미연결')(0.0, 1000.0);
            if (roll < (state.avoidanceCutoff | 0)) {
                hit = false;
                put(fields.bytes, PASSED, 1);
            }
        }
        if (hit) {
            last = [fround((current[0] + previous[0]) * 0.5), fround((current[1] + previous[1]) * 0.5)];
            if (fields.target && !(target[11] & 1)) {
                if (corner) {
                    need(hooks.impact, 'Missile 실제 배리어 충돌 효과 경계 누락')(last[0], last[1], 2, fround(0.02));
                    const sound = missileTypes[state.cornerImpactType].impactSound;
                    need(hooks.impactSound, 'Missile 실제 충돌 사운드 경계 누락')(last[0], last[1], sound);
                } else if (asset[10] === state.iceMissileType) {
                    // 아이스는 일반 HitEffect 대신 코너와 같은 실제 2번 표시를 생성한다.
                    need(hooks.impact, '아이스 투사체의 실제 충돌 표시 미연결')(last[0], last[1], 2, fround(0.02));
                    if (missileTypes[asset[10]].impactSound) {
                        if (typeof hooks.randomInteger !== 'function' || typeof hooks.impactName !== 'function') throw new Error('아이스 투사체의 실제 난수/파일명 음향 미연결');
                        const number = hooks.randomInteger(1000, 5000);
                        // 사거리 종료와 겹쳐도 전용 음향을 요청한다.
                        hooks.impactName(last[0], last[1], `iceCannonImpact${Math.trunc(number / 1000)}.wav`);
                    }
                } else {
                    need(hooks.hitEffect, 'Missile 실제 표적 충돌 효과 경계 누락')(last[0], last[1]);
                    if (missileTypes[asset[10]].impactSound && !exhausted) need(hooks.impactSound, 'Missile 실제 표적 충돌 사운드 경계 누락')(last[0], last[1], missileTypes[asset[10]].impactSound);
                }
            }
            // 최초 표적의 free 비트와 별개로 효과 뒤의 현재 표시 TYPE 를 읽는다.
            if (asset[10] === state.iceMissileType && !corner) need(hooks.ricochet, '아이스 투사체의 실제 반사 00482190 미연결')(fields);
            // 원본 10.78 은 표적의 free 비트나 코너와 별개로 충돌 확인 뒤 이 타입 조합의 사건을 보낸다(이전 좌표 기록·직접 피해·삭제보다 먼저 같은 클라이언트 입력 큐에 세 DWORD 전달).
            if (asset[10] === state.thunderMissileType && target[10] === state.stoneTowerType) {
                need(hooks.queueCommand, 'Missile 실제 충돌 입력 큐 미연결')(state.hitCommand, state.thunderMissileType, state.stoneTowerType);
            }
        }
        // 실제 효과/입력 큐 뒤이며 이전 좌표 저장과 조기 반환보다 앞선 현재 관문이다.
        if (!this.#checkBase(fields, alive)) return;
        putPoint(fields.bytes, PREVIOUS, last);
        if (!hit && !exhausted) return;
        // 진단 복귀 뒤 현재 firingSid 를 다시 해석한다.
        const finishingFiring = this.#slot(fields.firing, true);
        const liveFiring = (finishingFiring[11] & 1) === 0;
        const firingGroup = types[finishingFiring[10]].group;
        if (liveFiring && (firingGroup === 0 || firingGroup === 1)) {
            if (hit && !corner) need(hooks.hit, 'Missile 실제 직접 총 피해 경계 누락')(fields.firing, fields.target);
            else need(hooks.refresh, 'Missile 실제 총 사거리 갱신 경계 누락')(fields.firing);
        } else if (liveFiring || (hit && !corner)) {
            const damageType = missileTypes[liveFiring ? finishingFiring[10] : state.rainCannonType];
            let damage = fround(damageType.damagePerSecond * fround(0.6));
            if (missileTypes[finishingFiring[10]].specialFlyerDamage && (types[target[10]].flags2 & AIRBORNE)) damage = missileTypes[finishingFiring[10]].flyerDamagePerSecond;
            // 원문의 살아 있는 비총 발사자는 dead2 표적, 반납된 발사자는 살아 있는 표적에만 가상 98 을 호출한다.
            const eligible = fields.target && (liveFiring ? ((target[11] & 2) !== 0 && !(target[11] & 1)) : !(target[11] & 3));
            if (eligible) need(hooks.damage, '반사 Missile 실제 가상 98 피해 미연결')(fields.target, damageInteger(damage), asset[34], 0);
        }
        // 00482b00 은 비충돌 종료의 사거리 갱신 뒤에도 2번 표시를 생성한다.
        if (!hit && asset[10] === state.iceMissileType) need(hooks.impact, '아이스 투사체의 실제 사거리 종료 표시 미연결')(last[0], last[1], 2, fround(0.02));
        if (read(fields.bytes, 44)) need(hooks.impactSound, 'Missile 빗나감 종료 음향 미연결')(last[0], last[1], missileTypes[state.cornerImpactType].impactSound);
        // 원문은 마지막 진단에서 바뀐 +8 대신 처음 저장한 표시 자산 포인터를 삭제한다.
        hooks.destroy(assetSid, 0);
    }
}

// 원본 실제 Missile process 가 같은 Kernel 과 ProcessForm 수명을 소유한다. 사용: new MissileProcess(logic, fields) 를 호스트에 attach 한다. (2026-11-08 WB04-E125)
export class MissileProcess extends SquidProcess {
    // 이 객체보다 오래 살아야 하는 실제 프레임 처리기, 원본 raw 객체 이미지, 자기 삭제 뒤 raw 필드에 접근하지 않도록 하는 호스트 수명 관찰이다.
    #logic;
    #fields;
    #alive = true;

    // 생성된 raw 객체 필드와 frame 처리기의 수명을 연결한다. (2026-11-08 WB04-E125)
    constructor(logic, fields = new MissileFields()) {
        super();
        this.#logic = logic;
        this.#fields = fields;
    }

    // Kernel 프레임을 같은 raw 필드에 전달한다(호출 중 자기 삭제 뒤에는 필드를 다시 읽지 않는다). (2026-11-08 WB04-E125)
    runFrame() { this.#logic.frame(this.#fields, () => this.#alive); }

    // 호스트가 확정한 Form/Parent 를 원본 DWORD 에 쓴다. (2026-11-08 WB04-E125)
    onAttached() {
        put(this.#fields.bytes, FORM, this.form);
        put(this.#fields.bytes, PARENT, this.parent);
    }

    // 호스트 삭제 때 호출해 진행 중인 진단 caller 의 비주소 관찰에 알린다. (2026-11-08 WB04-E125)
    dispose() { this.#alive = false; }

    // 독립 대조/수명 관찰을 위한 현재 실제 필드이다. (2026-11-08 WB04-E125)
    get fields() { return this.#fields; }
}
