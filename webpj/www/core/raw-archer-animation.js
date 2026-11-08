// SunArcher71(Sun Disc Thrower)의 실제 회전/각속도 발사와 sunDisc83 생성 인자를 옮긴 모듈이다(cpppj `o/RawArcherAnimation.cpp/.h`, 10.78 전용).
// 캐논의 64바이트 객체와 구별되는 원본 0x58바이트 객체 이미지(ArcherAnimationFields)를 그대로 보존하며(미초기화 padding 포함) 부모(아처) 프레임과 float 각상태를 원본 저장 순서로 갱신한다.
//  - initialize(fields, fire, gunForm, target): 공통 생성자 접두(가상 표 0x50b1c0 발사/0x50b450 회전·내장 표·dirty 0xff·gun/target WORD·현재 시각, 발사면 +38 을 0)만 쓴다. initializeFire(fields): 부착 뒤 초기 각(부모 프레임→22.5도 단위)·속도·가속도·단계 0 으로 두고
//    출력 WORD bit0 을 해제한 뒤 공간 통지를 부른다.
//  - rotate(fields): 표적 0 이면 aimDirection×90, 아니면 표적을 추적(live 이면 위치/중심 재기록)해 부모→표적 각을 구하고 가장 가까운 20칸 방향표로 한 방향 칸씩 회전한다(시각 도달 시). 같으면 완료(통지 info 3/value 4), 아니면 프레임을 바꾸고 float 0.1 뒤로 예약하며
//    소리 sunDiscThrowerRatchet0<난수/1000>.wav 를 낸다.
//  - fire(fields): 프레임 경과(delta, 0 초과 2 미만일 때만)로 각속도/각을 적분하고 임계(감김×360+표적 각-발사 편향)에서 스프링(sasprung.wav)과 발사(safired.wav, launch)를 하며, 각에 따라 프레임(FireFrame)과 소리(sarachet/whoosh)를 바꾸고
//    종료 조건(각속도≤finishSpeed 이고 정규화 각이 finishAngle±finishTolerance 안, 또는 각<0)에서 완료(통지 info 6/value 7)한다.
//  - launch(fields): live HP 표적일 때만 sunDisc Missile 요청(속도 35·표시 83·효과 0)을 훅에 넘긴다(생성 경계가 없으면 성공으로 우회하지 않고 거부).
//  - 정적 helper: nearestDirection(angle) 20칸 비균등 방향표의 가장 가까운 칸, initialAngle(frame), fireFrame(angle, sprung), degrees(from, to), directionAngle(from, to, diagnostics).
//  - ArcherAnimationProcess(SquidProcess): Kernel 슬롯 프로세스이다. runFrame 은 fire 이면 fire, 아니면 rotate. onAttached 는 Form/Parent 기록.
// state(ArcherAnimationState)와 animationTypes[type]={directions,frameCount} 는 호출 시점에 읽는다. 훅 { common{center,targetPoint,unpop,repop,randomInteger,sound,createProjectile,notify,kill}, aimDirection(sid), notifySurface(sid) } 는 도달한 지점에서만 요구한다.
// 실제 GunProcess 연결(MakeArcherAnimationHostHooks/CreateArcherAnimation 등)은 RawGunProcess/RawMissile 이식 뒤로 둔다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E115: RawArcherAnimation 이식으로 추가)
import { SquidProcess } from './squid-process.js';
import { normalizeBoardAngle } from './raw-board-angle.js';

// 필드 위치(Form/Parent/위치/중심/내장 표/dirty/건/표적/시각/원위치·각·속도·가속도·단계)와 크기이다. (2026-11-08 WB04-E115)
const FORM = 4;
const PARENT = 8;
const POSITION = 12;
const CENTER = 20;
const INNER = 32;
const DIRTY = 40;
const GUN = 42;
const TARGET = 44;
const TIME = 48;
const REST = 56;
const ANGLE = 64;
const SPEED = 68;
const ACCELERATION = 72;
const SPRUNG = 76;
const FIRED = 80;
const FIELD_SIZE = 0x58;
// 부모 슬롯의 TYPE·상태·출력 WORD 위치와 프레임 DWORD 위치, 판본 타입 수, HP 플래그이다. (2026-11-08 WB04-E115)
const TYPE_AT = 10;
const STATE_AT = 11;
const OUTPUT_AT = 12;
const FRAME_AT = 36;
const TYPE_COUNT = 188;
const HIT_POINTS = 0x10;
// 원본 20칸 비균등 회전각표, 라디안→도 계수, 회전 대기 간격(float 0.1), 표적 0 방향 BYTE 의 90도 계수(float)이다. (2026-11-08 WB04-E115)
const ANGLES = Object.freeze([0, 21, 35, 52, 71, 86, 97, 115, 121, 148, 180, 205, 237, 256, 272, 282, 291, 314, 328, 345]);
const DEGREES_PER_RADIAN = 57.295827908797776;
const ROTATE_DELAY = Math.fround(0.1);
const AIM_ANGLE_SCALE = 90;
const fround = Math.fround;

// float/double 변환 버퍼이다. (2026-11-08 WB04-E115)
const floatView = new DataView(new ArrayBuffer(4));
const doubleView = new DataView(new ArrayBuffer(8));

// 미정렬 리틀엔디언 필드를 정확한 폭으로 읽는다(부호 없음). (2026-11-08 WB04-E115)
function read(bytes, offset, width = 4) {
    let value = 0;
    // 낮은 바이트부터 지정한 폭만 조합한다.
    for (let index = 0; index < width; index += 1) value += bytes[offset + index] * 2 ** (8 * index);
    return value;
}

// 지정 필드 밖 padding 을 보존하며 쓴다(32비트 이하 정수). (2026-11-08 WB04-E115)
function put(bytes, offset, value, width = 4) {
    const wrapped = value >>> 0;
    // 낮은 바이트부터 지정한 폭만 쓴다.
    for (let index = 0; index < width; index += 1) bytes[offset + index] = (wrapped >>> (8 * index)) & 0xff;
}

// 실제 float32 각상태/좌표를 읽고 저장한다. (2026-11-08 WB04-E115)
function readFloat(bytes, offset) {
    floatView.setUint32(0, read(bytes, offset), true);
    return floatView.getFloat32(0, true);
}
function writeFloat(bytes, offset, value) {
    floatView.setFloat32(0, value, true);
    put(bytes, offset, floatView.getUint32(0, true));
}

// 현재 좌표쌍을 원본 float 폭으로 읽는다. (2026-11-08 WB04-E115)
function point(bytes, offset) { return [readFloat(bytes, offset), readFloat(bytes, offset + 4)]; }

// 원본 현재 예약 시각 쓰기만 dirty8 을 켠다. (2026-11-08 WB04-E115)
function setTime(fields, value) {
    doubleView.setFloat64(0, value, true);
    fields.bytes[DIRTY] |= 8;
    put(fields.bytes, TIME, doubleView.getUint32(0, true));
    put(fields.bytes, TIME + 4, doubleView.getUint32(4, true));
}

// 필수 훅을 확인한다. (2026-11-08 WB04-E115)
function need(hook, message) {
    if (typeof hook !== 'function') throw new Error(message);
    return hook;
}

// 원본 0x58바이트 SunArcher Fire 이미지이다. 사용: new ArcherAnimationFields() 의 bytes 를 직접 읽고 쓴다. (2026-11-08 WB04-E115)
export class ArcherAnimationFields {
    // 공통 Baseprocess 와 각 위치/각속도/가속도/발사 상태를 보존하는 직접 필드 이미지이다.
    bytes = new Uint8Array(FIELD_SIZE);

    // 실제 공통 SID 와 시각 읽기이다. (2026-11-08 WB04-E115)
    get form() { return read(this.bytes, FORM, 2); }
    get parent() { return read(this.bytes, PARENT, 2); }
    get gunForm() { return read(this.bytes, GUN, 2); }
    get target() { return read(this.bytes, TARGET, 2); }
    get time() {
        doubleView.setUint32(0, read(this.bytes, TIME), true);
        doubleView.setUint32(4, read(this.bytes, TIME + 4), true);
        return doubleView.getFloat64(0, true);
    }
}

// 같은 월드의 원본 게임 시각과 mutable SunArcher 발사 전역이다. 호출 시점에 읽는다. 사용: new ArcherAnimationState({ now: 10, delta: 0.05 }). (2026-11-08 WB04-E115)
export class ArcherAnimationState {
    // 현재 게임 시각과 원본 0055b4e0 의 실제 한 프레임 경과, 원본 SunArcher71 / 표시 자산 sunDisc83 TYPE 이다.
    now = 0;
    delta = 0;
    type = 71;
    displayType = 83;
    // 실제 00540990/94 의 float 초기 각속도/가속도, 발사각의 감김 수와 앞뒤 변형각이다.
    initialSpeed = 180;
    initialAcceleration = 180;
    winding = 4;
    fireBias = 30;
    springOffset = 180;
    resetOffset = 180;
    // 회전 종료의 각속도/위치/오차 조건이다.
    finishSpeed = 180;
    finishAngle = 270;
    finishTolerance = 30;
    // 같은 Client checking/원문 Boardcoord 진단 장치(normalizeBoardAngle 의 diagnostics)이다.
    angles = {};
    // 실제 Client 의 Sun/Wind Archer TYPE 를 빌리는 { value } 이다. 없으면 type 을 쓴다.
    currentType = null;

    // 지정한 값만 덮어쓴다. (2026-11-08 WB04-E115)
    constructor(init = {}) { Object.assign(this, init); }

    // 생성/부모 검증/가상 getter 가 같은 현재 번호를 읽는다. (2026-11-08 WB04-E115)
    get currentTypeValue() { return this.currentType ? this.currentType.value : this.type; }
}

// 회전 방향표와 발사각/속도 변화를 원본 float 저장 순서로 수행한다. 사용: new RawArcherAnimation(pool, types, animationTypes, state, hooks). (2026-11-08 WB04-E115)
export class RawArcherAnimation {
    // 같은 실제 저장소, HP 플래그 표, 타입별 방향수/물리 프레임 수, 매 프레임 읽는 전역, 실제 하위 경계 훅이다.
    #pool;
    #types;
    #animationTypes;
    #state;
    #hooks;

    // 동일 판본(10.78) 풀/타입 표/방향 자료/전역을 연결한다. (2026-11-08 WB04-E115)
    constructor(pool, types, animationTypes, state, hooks = {}) {
        if (pool?.edition !== 'patch1078' || !types || types.length !== TYPE_COUNT || !animationTypes || animationTypes.length !== types.length || !state || state.currentTypeValue >= types.length) throw new Error('Archer Animation 타입/방향자료 오류');
        this.#pool = pool;
        this.#types = types;
        this.#animationTypes = animationTypes;
        this.#state = state;
        this.#hooks = hooks;
    }

    // 동일 풀/현재 아처 TYPE 를 어댑터에 제공한다. (2026-11-08 WB04-E115)
    get pool() { return this.#pool; }
    get type() { return this.#state.currentTypeValue; }

    // 원본 공통 ctor 접두만 초기화한다(실제 Host 부착 직후 fire 면 initializeFire 를 수행). (2026-11-08 WB04-E115)
    initialize(fields, fire, gunForm, target) {
        if (!gunForm || gunForm >= this.#pool.capacity) throw new RangeError('Archer gunForm SID 오류');
        const bytes = fields.bytes;
        put(bytes, 0, fire ? 0x50b1c0 : 0x50b450);
        put(bytes, FORM, 0);
        put(bytes, PARENT, 0);
        put(bytes, INNER, fire ? 0x50b1bc : 0x50b448);
        bytes[DIRTY] = 0xff;
        put(bytes, GUN, gunForm, 2);
        put(bytes, TARGET, target, 2);
        setTime(fields, this.#state.now);
        if (fire) put(bytes, REST, 0);
    }

    // 실제 부모 현재 TYPE 을 매 조회에서 검사한다. 읽기 전용 사본을 돌려준다. (2026-11-08 WB04-E115)
    #parent(fields) {
        const sid = fields.parent;
        if (!sid || sid >= this.#pool.capacity) throw new RangeError('Archer 부모 SID 오류');
        const raw = this.#pool.slot(sid);
        if (raw[TYPE_AT] !== this.#state.currentTypeValue) throw new Error('Archer 미복원 부모 타입');
        return raw;
    }

    // 현재 물리 프레임(10.78 DWORD)이다. (2026-11-08 WB04-E115)
    #frame(parent) { return read(this.#pool.slot(parent), FRAME_AT) | 0; }

    // 기존 원본 프레임 공간 순서(Unpop → 프레임 쓰기 → Repop 0x40)이다. (2026-11-08 WB04-E115)
    #setFrame(parent, frame) {
        const hooks = this.#hooks.common ?? {};
        if (typeof hooks.unpop !== 'function' || typeof hooks.repop !== 'function') throw new Error('Archer 실제 프레임 공간 경계 누락');
        hooks.unpop(parent, 0);
        put(this.#pool.allocated(parent), FRAME_AT, frame);
        hooks.repop(parent, 0x40);
    }

    // 표적이 free/dead 이면 기존 추적 좌표를 보존한다. 중심 훅 뒤의 현재 표적 바이트를 위치로 쓴다(live 뷰). (2026-11-08 WB04-E115)
    #track(fields) {
        const sid = fields.target;
        if (!sid || sid >= this.#pool.capacity) throw new RangeError('Archer 표적 SID 오류');
        const raw = this.#pool.physical(sid);
        if (raw[STATE_AT] & 3) return false;
        const center = need((this.#hooks.common ?? {}).center, 'Archer 실제 표적 중심 경계 누락')(sid);
        put(fields.bytes, POSITION, read(raw, 14));
        put(fields.bytes, POSITION + 4, read(raw, 18));
        writeFloat(fields.bytes, CENTER, center[0]);
        writeFloat(fields.bytes, CENTER + 4, center[1]);
        return true;
    }

    // 실제 완료 통지 뒤 Kill 이 이 객체를 없앨 수 있으므로 추가 필드를 읽지 않는다. (2026-11-08 WB04-E115)
    #finish(fields, info, value) {
        const hooks = this.#hooks.common ?? {};
        if (typeof hooks.notify !== 'function' || typeof hooks.kill !== 'function') throw new Error('Archer 실제 완료 경계 누락');
        const gun = fields.gunForm;
        const form = fields.form;
        hooks.notify(gun, 1, info, value);
        hooks.kill(form, 0x10);
    }

    // 원본 회전 helper 는 가장 가까운 각도를 고르며 동률에서는 앞선 칸을 보존한다. 반환은 0..19 칸 번호이다. (2026-11-08 WB04-E115)
    static nearestDirection(angle) {
        if (!Number.isFinite(angle)) throw new RangeError('Archer 회전 비유한 각도');
        let best = -1000.0;
        let result = 0;
        // 오름차순 표의 현재 최선 차이가 양수가 되면 원본처럼 다음 칸 조회 없이 끝낸다.
        for (let index = 0; index < ANGLES.length && best < 0; index += 1) {
            const difference = ANGLES[index] - angle;
            if (Math.abs(difference) < Math.abs(best)) {
                result = index;
                best = difference;
            }
        }
        if (result === 19 && Math.abs(360.0 - angle) < Math.abs(best)) return 0;
        return result;
    }

    // 실제 Fire ctor 가 현재 0..32 프레임을 22.5도 각으로 변환한다. 반환은 float 이다. (2026-11-08 WB04-E115)
    static initialAngle(frame) {
        let local = frame | 0;
        if (local > 15) local -= 16;
        const angle = local * -22.5 + 360.0;
        if (!(angle >= 0 && angle <= 360)) throw new RangeError('Archer 초기각 원본 assert');
        return fround(angle);
    }

    // 원본 fire helper 의 fmod 결과를 float 에 저장한 뒤 실제 0.5f 가산/절삭과 sprung offset 2/frame 16 을 유지한다. (2026-11-08 WB04-E115)
    static fireFrame(angle, sprung) {
        if (!Number.isFinite(angle)) throw new RangeError('Archer 발사 비유한 각도');
        let inverse = 360.0 - (angle % 360.0);
        if (!(inverse > 0 && inverse <= 360)) throw new RangeError('Archer 발사각 원본 assert');
        if (inverse >= 360) inverse = 359;
        const value = fround(inverse * 0.044444444444444446);
        const frame = Math.trunc(fround(value + 0.5));
        return (frame + (sprung ? 2 : 0)) % 16 + (sprung ? 16 : 0);
    }

    // 좌표 동률은 0 이며 원본의 근사 pi 계수로 현재 가리키는 각도를 계산한다. (2026-11-08 WB04-E115)
    static degrees(from, to) {
        if (from[0] === to[0] && from[1] === to[1]) return 0;
        return Math.atan2(-(to[1] - from[1]), to[0] - from[0]) * DEGREES_PER_RADIAN;
    }

    // 원본 BoardCoord.Angle 은 450-atanDegrees 를 float 로 저장한 뒤 정규화한다. 반환은 float 이다. (2026-11-08 WB04-E115)
    static directionAngle(from, to, diagnostics = {}) {
        if (from[0] === to[0] && from[1] === to[1]) return 0;
        return fround(normalizeBoardAngle(fround(450.0 - RawArcherAnimation.degrees(from, to)), diagnostics));
    }

    // 부착 뒤 Fire 초기화: 초기 각/속도/가속도·단계 0, 출력 WORD bit0 해제와 공간 통지이다. (2026-11-08 WB04-E115)
    initializeFire(fields) {
        this.#parent(fields);
        const notify = need(this.#hooks.notifySurface, 'Archer 초기 출력 공간 통지 누락');
        writeFloat(fields.bytes, ANGLE, RawArcherAnimation.initialAngle(this.#frame(fields.parent)));
        writeFloat(fields.bytes, SPEED, this.#state.initialSpeed);
        writeFloat(fields.bytes, ACCELERATION, this.#state.initialAcceleration);
        put(fields.bytes, SPRUNG, 0);
        put(fields.bytes, FIRED, 0);
        this.#pool.allocated(fields.parent)[OUTPUT_AT] &= 0xfe;
        notify(fields.parent);
    }

    // 표적 재조회는 due 검사 앞에 있으며 한 틱에 방향 한 칸만 진행한다. (2026-11-08 WB04-E115)
    rotate(fields) {
        const raw = this.#parent(fields);
        const data = this.#animationTypes[raw[TYPE_AT]];
        if (data.directions <= 0 || Math.trunc(data.frameCount / data.directions) <= 0) throw new Error('Archer 실제 방향/물리 프레임 자료 미공급');
        const per = Math.trunc(data.frameCount / data.directions);
        const current = Math.trunc(this.#frame(fields.parent) / per);
        const common = this.#hooks.common ?? {};
        let angle;
        if (!fields.target) {
            angle = fround(need(this.#hooks.aimDirection, 'Archer 실제 표적 0 방향 조회 누락')(fields.parent) * AIM_ANGLE_SCALE);
        } else {
            this.#track(fields);
            const center = need(common.center, 'Archer 실제 현재 중심 경계 누락');
            angle = RawArcherAnimation.directionAngle(center(fields.parent), point(fields.bytes, CENTER), this.#state.angles);
        }
        const wanted = RawArcherAnimation.nearestDirection(angle);
        if (!(fields.time <= this.#state.now)) return;
        if (current === wanted) {
            this.#finish(fields, 3, 4);
            return;
        }
        const backward = current < wanted ? current - wanted + data.directions : current - wanted;
        const forward = wanted < current ? wanted - current + data.directions : wanted - current;
        const step = forward < backward ? 1 : -1;
        const next = current === 0 && step === -1 ? data.directions - 1 : (current + step) % data.directions;
        this.#setFrame(fields.parent, next * per);
        setTime(fields, this.#state.now + ROTATE_DELAY);
        if (typeof common.randomInteger !== 'function' || typeof common.sound !== 'function') throw new Error('Archer 실제 회전 소리 경계 누락');
        const number = Math.trunc(common.randomInteger(1000, 10000) / 1000);
        common.sound(fields.parent, `sunDiscThrowerRatchet0${number}.wav`);
    }

    // HP 가 없는 표적이나 free/dead 표적은 원본처럼 생성 없이 false 를 반환한다. 사용: launch(fields) → 생성 요청 여부. (2026-11-08 WB04-E115)
    launch(fields) {
        this.#parent(fields);
        const sid = fields.target;
        if (!sid || sid >= this.#pool.capacity) throw new RangeError('Archer launch 표적 SID 오류');
        const raw = this.#pool.slot(sid);
        if (raw[TYPE_AT] >= this.#types.length) throw new RangeError('Archer launch 표적 타입 오류');
        if ((raw[STATE_AT] & 3) || !(this.#types[raw[TYPE_AT]].flags1 & HIT_POINTS)) return false;
        const common = this.#hooks.common ?? {};
        if (typeof common.targetPoint !== 'function' || typeof common.createProjectile !== 'function') throw new Error('Archer 실제 sunDisc Missile 생성 경계 누락');
        const center = common.targetPoint(fields.parent, sid);
        common.createProjectile({
            firing: fields.parent,
            target: sid,
            speed: 35,
            displayType: this.#state.displayType,
            effectType: 0,
            targetPosition: point(fields.bytes, POSITION),
            targetCenter: [fround(center[0]), fround(center[1])],
            ricochet: 0,
            firingPosition: [0, 0]
        });
        return true;
    }

    // 실제 각상태는 매 틱 갱신되며 총 예약 시각으로 발사 호출을 대신하지 않는다. (2026-11-08 WB04-E115)
    fire(fields) {
        if (!fields.target) throw new Error('Archer Fire 표적 0 원본 assert');
        const state = this.#state;
        const common = this.#hooks.common ?? {};
        if (state.delta > 0 && state.delta < 2.0) {
            // 부모 슬롯 주소를 잡아 두므로 중심 훅 뒤 바뀐 바이트를 그대로 읽는 live 뷰이다.
            this.#parent(fields);
            const raw = this.#pool.physical(fields.parent);
            const alive = this.#track(fields);
            const delta = fround(state.delta);
            writeFloat(fields.bytes, ANGLE, delta * readFloat(fields.bytes, SPEED) + readFloat(fields.bytes, ANGLE));
            writeFloat(fields.bytes, SPEED, delta * readFloat(fields.bytes, ACCELERATION) + readFloat(fields.bytes, SPEED));
            const threshold = fround(fround(state.winding) * 360.0 + RawArcherAnimation.degrees(point(raw, 14), point(fields.bytes, CENTER)) - fround(state.fireBias));
            const position = readFloat(fields.bytes, ANGLE);
            if (!read(fields.bytes, FIRED)) {
                if (!read(fields.bytes, SPRUNG) && fround(threshold - fround(state.springOffset)) <= position) {
                    need(common.sound, 'Archer 실제 sasprung.wav 경계 누락');
                    put(fields.bytes, SPRUNG, 1);
                    writeFloat(fields.bytes, ACCELERATION, -readFloat(fields.bytes, ACCELERATION));
                    common.sound(fields.parent, 'sasprung.wav');
                }
                if (!read(fields.bytes, FIRED) && threshold <= position) {
                    if (typeof this.#hooks.notifySurface !== 'function' || typeof common.sound !== 'function') throw new Error('Archer 실제 발사 출력/사운드 경계 누락');
                    put(fields.bytes, SPRUNG, 1);
                    put(fields.bytes, FIRED, 1);
                    this.#pool.allocated(fields.parent)[OUTPUT_AT] |= 1;
                    this.#hooks.notifySurface(fields.parent);
                    if (!this.launch(fields) && alive) return;
                    common.sound(fields.parent, 'safired.wav');
                }
            }
            if (read(fields.bytes, FIRED) && read(fields.bytes, SPRUNG) && fround(threshold + fround(state.resetOffset)) <= position) put(fields.bytes, SPRUNG, 0);
            if (position > 0) {
                const next = RawArcherAnimation.fireFrame(position, read(fields.bytes, SPRUNG) !== 0);
                if (next !== this.#frame(fields.parent)) {
                    this.#setFrame(fields.parent, next);
                    if (readFloat(fields.bytes, SPEED) < 0) {
                        need(common.sound, 'Archer 실제 sarachet.wav 경계 누락');
                        common.sound(fields.parent, 'sarachet.wav');
                    } else if (this.#frame(fields.parent) === 2) {
                        need(common.sound, 'Archer 실제 whoosh 경계 누락');
                        common.sound(fields.parent, 'sunDiscThrowerWhoosh.wav');
                    }
                }
            }
        }
        const speed = readFloat(fields.bytes, SPEED);
        let completed = false;
        if (read(fields.bytes, FIRED) && speed <= fround(state.finishSpeed)) {
            // 진단이 복귀한 뒤 현재 finishAngle/tolerance 를 읽는다.
            const angle = normalizeBoardAngle(readFloat(fields.bytes, ANGLE), state.angles);
            completed = Math.abs(angle - fround(state.finishAngle)) < fround(state.finishTolerance);
        }
        if (completed || readFloat(fields.bytes, ANGLE) < 0) this.#finish(fields, 6, 7);
    }
}

// 실제 Host/Kernel 이 독립 SunArcher 회전/발사 객체를 소유한다. 사용: new ArcherAnimationProcess(logic, fields, fire) 를 호스트에 attach 한다. (2026-11-08 WB04-E115)
export class ArcherAnimationProcess extends SquidProcess {
    // 이 객체보다 오래 사는 처리기, 소유 이미지, 파생 종류(true 면 발사)이다.
    #logic;
    #fields;
    #fire;

    // 현재 원본 이미지와 파생 종류를 소유한다. (2026-11-08 WB04-E115)
    constructor(logic, fields, fire) {
        super();
        this.#logic = logic;
        this.#fields = fields;
        this.#fire = fire;
    }

    // 실제 Kernel 틱에서 원본 회전 또는 발사 몸체를 선택한다. (2026-11-08 WB04-E115)
    runFrame() {
        if (this.#fire) this.#logic.fire(this.#fields);
        else this.#logic.rotate(this.#fields);
    }

    // 현재 Form/부모 번호를 원본 비정렬 이미지와 동기화한다. (2026-11-08 WB04-E115)
    onAttached() {
        put(this.#fields.bytes, FORM, this.form);
        put(this.#fields.bytes, PARENT, this.parent);
    }

    // 원본 생성 후 각 초기화와 독립 관찰에 사용한다. (2026-11-08 WB04-E115)
    get fields() { return this.#fields; }
}
