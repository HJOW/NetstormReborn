// Gunanim.cpp 의 썬 캐논 회전·발사·원위치 애니메이션 프레임 몸체와 공통 직렬화(466cc0/466ac0/466c20/467f40/467f80)를 옮긴 모듈이다(cpppj `o/RawGunAnimation.cpp/.h`, 10.78 전용).
// 원본 0x40바이트 객체 이미지를 그대로 보존하며(미초기화 padding 포함) 부모(썬 캐논) 프레임을 한 단계씩 진행한다. 월드 효과(중심·공간 해제/재등록·난수·소리·Missile 생성·완료 통지·삭제)는 모두 훅이다.
//  - GunAnimationFields: bytes(64) 필드 이미지와 form(+4)/parent(+8)/gunForm(+2a)/target(+2c)/time(+30 double)/restDirection(+38) 읽기이다.
//  - RawGunAnimation(pool, types, state, hooks): initialize(fields, kind, gunForm, target)는 가상 표·dirty 0xff·gun/target WORD·현재 시각(·원위치 방향 0)을 쓴다.
//    frame(fields, kind)는 SunFire 면 표적 추적 뒤 시각이 되면 프레임 ±24 와 발사/완료, SunRotate/SunRest 면 시각이 되면 방향 표(28칸 5행)로 한 프레임씩 회전·원위치를 진행한다(회전 소리 파일 번호는 난수/1000).
//    launch(fields)는 live HP 표적일 때만 Missile 생성 인자 12개를 훅에 넘긴다(createProjectile 이 없으면 성공으로 우회하지 않고 예외).
//  - initializeReceivedGunAnimation / readGunAnimationProcessData / gunAnimationUsesFirePacket / writeGunAnimationProcessData: 수신 전용 생성자 네 종류, 선택 필드 복원, 가상 writer 선택, dirty 선택 직렬화이다(CD 가상 표는 10.72 라 제외).
//  - GunAnimationProcess(SquidProcess): Kernel 슬롯 프로세스이다. runFrame 은 frame, onAttached 는 Form/Parent 기록, writePacket 은 공통 직렬화이다.
// 훅 { center(sid)→[x,y], targetPoint(firing, target)→[x,y], unpop(sid,flags), repop(sid,flags), randomInteger(low,high), sound(sid,name), createProjectile(request), notify(form,event,info,value), kill(form,flags) } 는 도달한 지점에서만 요구한다.
// 실제 GunProcess 생성 경계(factory/MakeGunAnimationHostHooks)는 RawGunProcess 이식 전이라 포함하지 않는다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E113: RawGunAnimation 이식으로 추가)
import { SquidProcess } from './squid-process.js';

// 썬 캐논 애니메이션 파생 종류이다(값은 원본 열거 순서). (2026-11-08 WB04-E113)
export const GunAnimationKind = Object.freeze({ SunRotate: 0, SunFire: 1, SunRest: 2 });
// 수신 전용 생성자 종류이다(00468830/00468900 네 기본 생성자). (2026-11-08 WB04-E113)
export const GunAnimationReceiveKind = Object.freeze({ SunFire: 0, SunRotate: 1, ArcherFire: 2, ArcherRotate: 3 });

// 객체 필드 위치(Baseprocess Form/Parent, 위치, 중심, 내장 가상 표, dirty, 건/표적 WORD, 시각, 원위치 방향)와 크기이다. (2026-11-08 WB04-E113)
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
const FIELD_SIZE = 0x40;
// 부모 프레임 위치(10.78 DWORD)와 부모/표적 슬롯 필드(TYPE·상태), 타입 수, HP 플래그이다. (2026-11-08 WB04-E113)
const FRAME_AT = 36;
const TYPE_AT = 10;
const STATE_AT = 11;
const TYPE_COUNT = 188;
const HIT_POINTS = 0x10;
// 회전/발사 대기 간격(float 상수를 double 로 확장한 값)과 발사 프레임 수/회전 프레임 수이다. (2026-11-08 WB04-E113)
const ROTATE_DELAY = Math.fround(0.2);
const FIRE_DELAY = Math.fround(0.07);
const FIRE_FRAMES = 48;
const ROTATE_FRAMES = 28;
// 원위치 stage8 이 8번 방향까지 읽으므로 원본 다섯 행 전체를 보존하는 회전/원위치 프레임 표이다(32768 = 완료 표식). (2026-11-08 WB04-E113)
const SUN_ROTATE = Object.freeze([
    [4, 32768, 12, 19, 5, 6, 7, 8, 9, 10, 11, 1, 13, 14, 15, 16, 20, 16, 17, 18, 21, 8, 8, 22, 32768, 32768, 32768, 32768],
    [4, 11, 12, 32768, 5, 6, 7, 8, 22, 8, 9, 10, 13, 14, 15, 16, 17, 18, 19, 3, 16, 20, 23, 16, 32768, 32768, 32768, 32768],
    [32768, 11, 12, 19, 0, 4, 5, 6, 7, 8, 9, 10, 13, 14, 15, 16, 20, 16, 17, 18, 21, 8, 8, 22, 32768, 32768, 32768, 32768],
    [4, 11, 32768, 19, 5, 6, 7, 8, 22, 8, 9, 10, 2, 12, 13, 14, 15, 16, 17, 18, 16, 20, 23, 16, 32768, 32768, 32768, 32768],
    [4, 11, 12, 19, 5, 6, 7, 8, 32768, 8, 9, 10, 13, 14, 15, 16, 32768, 16, 17, 18, 32768, 20, 23, 16, 32768, 32768, 32768, 32768]
]);
// 10.78 의 발사/회전 가상 표 주소이다(patch 전용; CD 표는 제외). (2026-11-08 WB04-E113)
const FIRE_TABLES = new Set([0x50b0d0, 0x50b120, 0x50b170, 0x50b1c0, 0x50b318, 0x50b370, 0x50b3f0]);
const ROTATE_TABLES = new Set([0x50b210, 0x50b260, 0x50b2c8, 0x50b450]);
// 파생별 바깥/내장 가상 표 주소이다([회전, 발사, 원위치]). (2026-11-08 WB04-E113)
const OUTER_TABLE = Object.freeze([0x50b260, 0x50b318, 0x50b2c8]);
const INNER_TABLE = Object.freeze([0x50b258, 0x50b310, 0x50b2c4]);

// float32 와 그 비트 사이의 변환 버퍼이다. (2026-11-08 WB04-E113)
const floatView = new DataView(new ArrayBuffer(4));
const doubleView = new DataView(new ArrayBuffer(8));
const fround = Math.fround;

// 미정렬 리틀엔디언 필드를 정확한 폭으로 읽는다(부호 없음). (2026-11-08 WB04-E113)
function read(bytes, offset, width = 4) {
    let value = 0;
    // 낮은 바이트부터 지정한 폭만 조합한다.
    for (let index = 0; index < width; index += 1) value += bytes[offset + index] * 2 ** (8 * index);
    return value;
}

// 주변 padding 을 보존하며 지정 폭으로 쓴다(32비트 이하 정수). (2026-11-08 WB04-E113)
function put(bytes, offset, value, width = 4) {
    const wrapped = value >>> 0;
    // 낮은 바이트부터 지정한 폭만 쓴다.
    for (let index = 0; index < width; index += 1) bytes[offset + index] = (wrapped >>> (8 * index)) & 0xff;
}

// 단정도 float 값을 원본 비트(uint32)로 바꾼다. (2026-11-08 WB04-E113)
function floatBits(value) {
    floatView.setFloat32(0, value, true);
    return floatView.getUint32(0, true);
}

// 필드의 단정도 좌표를 읽는다. (2026-11-08 WB04-E113)
function coordinate(bytes, offset) {
    floatView.setUint32(0, read(bytes, offset), true);
    return floatView.getFloat32(0, true);
}

// 필수 훅이 함수인지 확인한다. (2026-11-08 WB04-E113)
function need(hook, message) {
    if (typeof hook !== 'function') throw new Error(message);
    return hook;
}

// 원본 BoardCoord 의 4방향 판정이다(Y 절댓값 동률에서는 세로 방향이 앞선다). (2026-11-08 WB04-E113)
function direction(from, to) {
    const dx = fround(to[0] - from[0]);
    const dy = fround(to[1] - from[1]);
    if (Math.abs(dx) <= Math.abs(dy)) return dy > 0 ? 4 : 0;
    return dx <= 0 ? 6 : 2;
}

// 원본 0x40바이트 객체 필드 이미지이다. 사용: new GunAnimationFields() 의 bytes 를 직접 읽고 쓴다. (2026-11-08 WB04-E113)
export class GunAnimationFields {
    // 미초기화 padding 을 포함한 직접 필드 이미지이다. (2026-11-08 WB04-E113)
    bytes = new Uint8Array(FIELD_SIZE);

    // 실제 Baseprocess Form/Parent 와 내장 gunForm/target/시각/원위치 방향 읽기이다. (2026-11-08 WB04-E113)
    get form() { return read(this.bytes, FORM, 2); }
    get parent() { return read(this.bytes, PARENT, 2); }
    get gunForm() { return read(this.bytes, GUN, 2); }
    get target() { return read(this.bytes, TARGET, 2); }
    get time() {
        doubleView.setUint32(0, read(this.bytes, TIME), true);
        doubleView.setUint32(4, read(this.bytes, TIME + 4), true);
        return doubleView.getFloat64(0, true);
    }
    get restDirection() { return read(this.bytes, REST) | 0; }
}

// 현재 시각을 raw 비트 그대로 필드에 쓴다. (2026-11-08 WB04-E113)
function putTime(fields, value) {
    doubleView.setFloat64(0, value, true);
    put(fields.bytes, TIME, doubleView.getUint32(0, true));
    put(fields.bytes, TIME + 4, doubleView.getUint32(4, true));
}

// dirty8 과 다음 실행 시각을 같은 순서로 기록한다. (2026-11-08 WB04-E113)
function setTime(fields, value) {
    fields.bytes[DIRTY] |= 8;
    putTime(fields, value);
}

// 수신 전용 네 생성자 00468830/00468900 의 지정 필드만 초기화한다(미작성 좌표/표적/방향과 padding 은 보존). 사용: initializeReceivedGunAnimation(outerBytes, GunAnimationReceiveKind.SunFire). (2026-11-08 WB04-E113)
export function initializeReceivedGunAnimation(outer, kind) {
    const archerFire = kind === GunAnimationReceiveKind.ArcherFire;
    if (outer.length < (archerFire ? 88 : 56)) throw new RangeError('애니메이션 수신 생성자의 실제 객체 크기 부족');
    const rotate = kind === GunAnimationReceiveKind.SunRotate || kind === GunAnimationReceiveKind.ArcherRotate;
    put(outer, PARENT, 0);
    put(outer, FORM, 0);
    put(outer, INNER, rotate ? 0x50b0c0 : 0x50b0c4);
    put(outer, TIME, 0);
    put(outer, TIME + 4, 0);
    outer[DIRTY] = 0xff;
    if (archerFire) {
        put(outer, 64, 0);
        put(outer, 76, 0);
        put(outer, 80, 0);
        put(outer, 0, 0x50b1c0);
        put(outer, INNER, 0x50b1bc);
        put(outer, 68, 0x43340000);
        put(outer, 72, 0x43340000);
        return;
    }
    if (kind === GunAnimationReceiveKind.SunFire) {
        put(outer, 0, 0x50b318);
        put(outer, INNER, 0x50b310);
        return;
    }
    const sunRotate = kind === GunAnimationReceiveKind.SunRotate;
    put(outer, 0, sunRotate ? 0x50b260 : 0x50b450);
    put(outer, INNER, sunRotate ? 0x50b258 : 0x50b448);
}

// 공통 decoder 00466cc0: 부모 DWORD 는 별도 반환하고 선택된 WORD/8바이트만 복사한다(앞선 부분 쓰기는 뒤 실패에도 유지). 사용: const parent = readGunAnimationProcessData(outer, packet). (2026-11-08 WB04-E113)
export function readGunAnimationProcessData(outer, packet) {
    if (packet.length < 9) throw new RangeError('애니메이션 수신의 실제 9바이트 머리 부족');
    const dirty = packet[8];
    let offset = 9;
    let parent = 0;
    const require = (width) => { if (offset > packet.length || width > packet.length - offset) throw new RangeError('애니메이션 수신의 선택 필드 부족'); };
    if (dirty & 1) {
        require(4);
        parent = read(packet, offset);
        offset += 4;
    }
    // decoder 에는 observer/dirty 소비/추가 상태 초기화가 없다. 선택 필드를 원래 폭 그대로 복사한다.
    for (const [bit, target, width] of [[2, GUN, 2], [4, TARGET, 2], [8, TIME, 8]]) {
        if (!(dirty & bit)) continue;
        require(width);
        if (target > outer.length || width > outer.length - target) throw new Error('애니메이션 수신의 현재 PID/선택 필드 미연결');
        outer.set(packet.subarray(offset, offset + width), target);
        offset += width;
    }
    return parent;
}

// 보호 PE 의 실제 outer+0 가상 표가 두 wrapper 중 어느 것인지 고른다(true = 발사 0x3b, false = 회전/원위치 0x3c). 미연결 표는 임의 패킷으로 보내지 않는다. (2026-11-08 WB04-E113)
export function gunAnimationUsesFirePacket(outer) {
    if (outer.length < 4) throw new RangeError('애니메이션 writer 의 실제 outer VT 부족');
    const table = read(outer, 0);
    if (FIRE_TABLES.has(table)) return true;
    if (ROTATE_TABLES.has(table)) return false;
    throw new Error('애니메이션의 현재 가상 writer 미연결');
}

// 공통 직렬화 466ac0/466c20: force/부모 선택/원래 폭의 쓰기 뒤 dirty 를 비운다. 반환은 기록한 길이이다. 사용: writeGunAnimationProcessData(outer, packet, includeParent, force, message, protocol = 1). (2026-11-08 WB04-E113)
export function writeGunAnimationProcessData(outer, packet, includeParent, force, message, protocol = 1) {
    if (outer.length < 56) throw new RangeError('애니메이션 writer 의 실제 공통 접두 부족');
    const parent = includeParent || force ? read(outer, PARENT) : 0;
    const dirty = force ? 0xff : outer[DIRTY];
    const length = 9 + (parent ? 4 : 0) + ((dirty & 2) ? 2 : 0) + ((dirty & 4) ? 2 : 0) + ((dirty & 8) ? 8 : 0);
    if (packet.length < length) throw new RangeError('애니메이션 writer 의 실제 버퍼 부족');
    if (force) outer[DIRTY] = 0xff;
    packet[0] = protocol;
    packet[1] = message;
    put(packet, 4, read(outer, FORM));
    let offset = 9;
    if (parent) {
        // 비영 부모 인자만 기록하며 기존 dirty1 을 임의로 지우지 않는다.
        outer[DIRTY] |= 1;
        put(packet, offset, parent);
        offset += 4;
    }
    // 각 선택 필드를 계산 없이 원래 WORD/8바이트 비트로 복사한다.
    for (const [bit, source, width] of [[2, GUN, 2], [4, TARGET, 2], [8, TIME, 8]]) {
        if (!(outer[DIRTY] & bit)) continue;
        packet.set(outer.subarray(source, source + width), offset);
        offset += width;
    }
    // 원문은 전역 선언 길이를 읽거나 새로 쓰지 않는다.
    put(packet, 2, offset, 2);
    packet[8] = outer[DIRTY];
    outer[DIRTY] = 0;
    return offset;
}

// 썬 캐논 애니메이션 프레임 처리기이다. 사용: new RawGunAnimation(pool, types, state, hooks). state = { now, sunCannonType = 128, displayType = 80, effectType = 11, currentSunCannonType?: {value} }. (2026-11-08 WB04-E113)
export class RawGunAnimation {
    // 현재 자산과 애니메이션 Form 이 공유하는 SID 풀, HP 플래그를 읽는 타입 표, 현재 시각/타입 번호, 월드 효과 훅이다. (2026-11-08 WB04-E113)
    #pool;
    #types;
    #state;
    #hooks;

    // 판본(10.78)과 타입 표 크기를 검사한다. (2026-11-08 WB04-E113)
    constructor(pool, types, state, hooks = {}) {
        if (pool?.edition !== 'patch1078' || !types || types.length !== TYPE_COUNT || !state) throw new Error('GunAnimation 타입 판본/크기 오류');
        this.#pool = pool;
        this.#types = types;
        this.#state = state;
        this.#hooks = hooks;
    }

    // 같은 풀을 사용하는 호스트/어댑터만 연결한다. (2026-11-08 WB04-E113)
    get pool() { return this.#pool; }

    // 현재 썬 캐논 TYPE 번호이다(실제 Client 전역을 빌린 값이 있으면 매번 다시 읽는다). (2026-11-08 WB04-E113)
    sunCannonType() {
        const borrowed = this.#state.currentSunCannonType;
        return borrowed ? borrowed.value : (this.#state.sunCannonType ?? 128);
    }

    // 원본 파생 ctor 의 초기화와 gunForm/target setter 및 현재 시각을 준비한다(좌표·padding 은 초기화하지 않는다). 사용: initialize(fields, GunAnimationKind.SunRotate, gunForm, target). (2026-11-08 WB04-E113)
    initialize(fields, kind, gunForm, target) {
        if (!gunForm || gunForm >= this.#pool.capacity) throw new RangeError('GunAnimation gunForm 오류');
        const bytes = fields.bytes;
        put(bytes, 0, OUTER_TABLE[kind]);
        put(bytes, FORM, 0);
        put(bytes, PARENT, 0);
        put(bytes, INNER, INNER_TABLE[kind]);
        bytes[DIRTY] = 0xff;
        put(bytes, GUN, gunForm, 2);
        put(bytes, TARGET, kind === GunAnimationKind.SunRest ? 0 : target, 2);
        putTime(fields, this.#state.now);
        if (kind !== GunAnimationKind.SunRotate) put(bytes, REST, 0);
    }

    // 부모 프레임(10.78 DWORD)을 읽는다. 부모 범위와 썬 캐논 TYPE 을 검사한다. (2026-11-08 WB04-E113)
    #currentFrame(parent) {
        if (!parent || parent >= this.#pool.capacity) throw new RangeError('GunAnimation 부모 범위 오류');
        const raw = this.#pool.slot(parent);
        if (raw[TYPE_AT] !== this.sunCannonType()) throw new Error('GunAnimation 미복원 부모 파생 타입');
        return read(raw, FRAME_AT) | 0;
    }

    // 원본 Unpop → 프레임 쓰기 → Repop(0x40) 순서로 부모 프레임을 바꾼다. 공간 효과 훅이 없으면 거부한다. (2026-11-08 WB04-E113)
    #setFrame(parent, frame) {
        const hooks = this.#hooks;
        if (typeof hooks.unpop !== 'function' || typeof hooks.repop !== 'function') throw new Error('GunAnimation 프레임 공간 효과 누락');
        hooks.unpop(parent, 0);
        put(this.#pool.allocated(parent), FRAME_AT, frame);
        hooks.repop(parent, 0x40);
    }

    // 살아 있는 표적의 위치/중심을 매 프레임 다시 기록한다. 표적이 free/dead 이면 기존 추적 좌표를 유지하고 false 를 돌려준다. (2026-11-08 WB04-E113)
    #trackTarget(fields) {
        const target = fields.target;
        if (!target || target >= this.#pool.capacity) throw new RangeError('GunAnimation 표적 범위 오류');
        const raw = this.#pool.slot(target);
        if (raw[STATE_AT] & 3) return false;
        const center = need(this.#hooks.center, 'GunAnimation 실제 중심 경계 누락');
        put(fields.bytes, POSITION, read(raw, 14));
        put(fields.bytes, POSITION + 4, read(raw, 18));
        const at = center(target);
        put(fields.bytes, CENTER, floatBits(at[0]));
        put(fields.bytes, CENTER + 4, floatBits(at[1]));
        return true;
    }

    // 00466f50: 유효 HP 표적에서만 Missile 생성 인자 12개를 훅에 넘긴다. createProjectile 이 구현되지 않았다면 이 호출의 완료는 투사체 복원 완료가 아니다. (2026-11-08 WB04-E113)
    launch(fields) {
        const target = fields.target;
        if (!target || target >= this.#pool.capacity) throw new RangeError('GunAnimation launch 표적 오류');
        const raw = this.#pool.slot(target);
        if (raw[TYPE_AT] >= this.#types.length) throw new RangeError('GunAnimation launch 표적 타입 오류');
        if ((raw[STATE_AT] & 3) || !(this.#types[raw[TYPE_AT]].flags1 & HIT_POINTS)) return false;
        const hooks = this.#hooks;
        if (typeof hooks.targetPoint !== 'function' || typeof hooks.createProjectile !== 'function') throw new Error('GunAnimation 실제 Missile 생성 경계 누락');
        const firing = fields.parent;
        const center = hooks.targetPoint(firing, target);
        hooks.createProjectile({
            firing,
            target,
            speed: 20,
            displayType: this.#state.displayType ?? 80,
            effectType: this.#state.effectType ?? 11,
            targetPosition: [coordinate(fields.bytes, POSITION), coordinate(fields.bytes, POSITION + 4)],
            targetCenter: [fround(center[0]), fround(center[1])],
            ricochet: 0,
            firingPosition: [0, 0]
        });
        return true;
    }

    // 완료 통지(event 1)를 현재 GunProcess 에 보낸 뒤 애니메이션 Form 을 flags 0x10 으로 없앤다. Kill 뒤 필드는 다시 읽지 않는다. (2026-11-08 WB04-E113)
    #finish(fields, info, value) {
        const hooks = this.#hooks;
        if (typeof hooks.notify !== 'function' || typeof hooks.kill !== 'function') throw new Error('GunAnimation 완료 효과 누락');
        const gun = fields.gunForm;
        const form = fields.form;
        hooks.notify(gun, 1, info, value);
        hooks.kill(form, 0x10);
    }

    // 00467130/00467330/00467240: 각 프레임은 한 물리 단계만 진행하고 현재 시각에서 다음 간격을 더한다. 사용: frame(fields, GunAnimationKind.SunFire). (2026-11-08 WB04-E113)
    frame(fields, kind) {
        const parent = fields.parent;
        const current = this.#currentFrame(parent);
        if (current < 0 || current >= (kind === GunAnimationKind.SunFire ? FIRE_FRAMES : ROTATE_FRAMES)) throw new RangeError('GunAnimation 썬 캐논 물리 프레임 오류');
        const now = this.#state.now;
        if (kind === GunAnimationKind.SunFire) {
            const alive = this.#trackTarget(fields);
            if (!(fields.time <= this.#state.now)) return;
            const code = (current < 24 ? ((current + 24) | 0x4000) : ((current - 24) | 0x8000)) >>> 0;
            this.#setFrame(parent, code & 0x3fff);
            if ((code & 0x4000) && !this.launch(fields) && alive) return;
            if (code & 0x8000) {
                this.#finish(fields, 6, 7);
                return;
            }
            setTime(fields, this.#state.now + FIRE_DELAY);
            return;
        }
        if (!(fields.time <= now)) return;
        let heading = Math.trunc(fields.restDirection / 2);
        if (kind === GunAnimationKind.SunRotate) {
            const hooks = this.#hooks;
            if (!fields.target || typeof hooks.center !== 'function') throw new Error('GunAnimation 회전 표적/중심 경계 누락');
            const to = hooks.center(fields.target);
            const from = hooks.center(parent);
            heading = direction(from, to) / 2;
        }
        if (heading < 0 || heading > 4) throw new RangeError('GunAnimation 원위치 방향 오류');
        const code = SUN_ROTATE[heading][current];
        if (code & 0x8000) {
            if (kind === GunAnimationKind.SunRotate) {
                this.#finish(fields, 3, 4);
                return;
            }
            if (fields.restDirection === 8) {
                this.#finish(fields, 9, 10);
                return;
            }
            put(fields.bytes, REST, fields.restDirection + 2);
        } else {
            if (kind === GunAnimationKind.SunRotate) {
                const hooks = this.#hooks;
                if (typeof hooks.randomInteger !== 'function' || typeof hooks.sound !== 'function') throw new Error('GunAnimation 회전 소리 경계 누락');
                const number = Math.trunc(hooks.randomInteger(1000, 10000) / 1000);
                hooks.sound(parent, `sunCannonRaiseLower${number}.wav`);
            }
            this.#setFrame(parent, code & 0x3fff);
        }
        setTime(fields, this.#state.now + ROTATE_DELAY);
    }

    // 원본 dirty 접두 packet 을 쓴다(회전/원위치 0x3c, 발사 0x3b). 사용: RawGunAnimation.writePacket(fields, packet, includeParent, force, protocol). (2026-11-08 WB04-E113)
    static writePacket(fields, packet, includeParent, force = false, protocol = 1) {
        return writeGunAnimationProcessData(fields.bytes, packet, includeParent, force, gunAnimationUsesFirePacket(fields.bytes) ? 0x3b : 0x3c, protocol);
    }
}

// 같은 Kernel 과 ProcessForm 수명에서 프레임 몸체를 소비하는 애니메이션 프로세스이다. 사용: new GunAnimationProcess(logic, fields, kind) 를 호스트에 attach 한다. (2026-11-08 WB04-E113)
export class GunAnimationProcess extends SquidProcess {
    // 객체보다 오래 사는 실제 프레임 처리기, 현재 raw 필드와 padding, 생성자에서 고른 파생 종류이다. (2026-11-08 WB04-E113)
    #logic;
    #fields;
    #kind;

    // 초기화된 필드와 파생 종류를 소유한다. (2026-11-08 WB04-E113)
    constructor(logic, fields, kind) {
        super();
        this.#logic = logic;
        this.#fields = fields;
        this.#kind = kind;
    }

    // Kernel 의 현재 틱을 같은 프레임 몸체로 전달한다. (2026-11-08 WB04-E113)
    runFrame() { this.#logic.frame(this.#fields, this.#kind); }

    // 호스트가 확정한 Form/Parent DWORD 를 필드에 기록한다. (2026-11-08 WB04-E113)
    onAttached() {
        put(this.#fields.bytes, FORM, this.form);
        put(this.#fields.bytes, PARENT, this.parent);
    }

    // 월드 전송기가 같은 raw 상태로 packet 을 작성한다(Form/Parent 를 다시 기록한 뒤 공통 직렬화). (2026-11-08 WB04-E113)
    writePacket(packet, parentChanged, force = false) {
        put(this.#fields.bytes, FORM, this.form);
        put(this.#fields.bytes, PARENT, this.parent);
        return RawGunAnimation.writePacket(this.#fields, packet, parentChanged, force);
    }

    // 검사/월드 사건이 같은 필드를 조회한다. (2026-11-08 WB04-E113)
    get fields() { return this.#fields; }
}
