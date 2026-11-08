// 썬/썬더 캐논·반더 타워가 같은 467330 프레임 몸체로 실행하는 공통 발사(Fire)를 옮긴 모듈이다(cpppj `o/RawCannonFire.cpp/.h`, 10.78 전용).
// 원본 64바이트 객체 이미지(GunAnimationFields)를 그대로 쓰며 부모(캐논) 프레임을 가상 단계표로 한 칸씩 진행한다. 월드 효과(중심·공간 해제/재등록·소리·Missile 생성·즉시 빔·피해·완료 통지·삭제)는 모두 훅이다.
//  - initialize(fields, gunForm, target): 파생(썬 0x50b318/썬더 0x50b370/반더 0x50b170)의 가상 표·dirty 0xff·gun/target WORD·현재 시각·원위치 방향 0 을 쓴다(Form/Parent 는 0).
//  - step(fields, frame): 반더는 `frame | 0xc000`(발사+완료 동시), 썬은 24프레임 앞뒤 변환(<24 이면 +24|0x4000, 아니면 -24|0x8000), 썬더는 현재 부모 프레임 코드의 글자 구간을 따라 진행한다
//    (구간 끝 초과 → 처음|0x8000, 끝 → |0x4000, 끝-growlOffset/끝-roarOffset 에서 소리 thunderCannonGrowl.wav/thunderCannonRoar.wav).
//  - launch(fields): live HP 표적일 때 일반 투사체(Missile 인자 12개: 포구 기본 -10000,-10000)를 만들거나, 초기 부모 TYPE 이 반더이면 즉시 빔(create → 회피 난수 `RandomFloat(0,1000) < avoidanceCutoff`[Cloud Floater 만] → refresh 또는 hit)을 쓴다.
//    초기 부모/표적 슬롯 주소는 snapshot 이므로 훅 뒤 바뀐 바이트를 그대로 다시 읽는다(live 뷰).
//  - frame(fields) / frameWithStep(fields, step): 표적 추적(live 면 위치/중심 재기록) → 시각 도달 시 가상 step → 부모 프레임 변경(unpop 0 → 쓰기 → repop 0x40) → 발사 요청 → 완료(통지 info 6/value 7 + 삭제 0x10) 또는 다음 시각(지연 × 썬더 배율).
//  - CannonFireProcess(SquidProcess): Kernel 슬롯 프로세스이다. runFrame 은 frame, onAttached 는 Form/Parent 기록.
// state = CannonFireState{ now, kind, type, displayType, effectType, speed, delay, launchScale, scaledType, growlOffset, roarOffset, vanderType, floaterType, avoidanceCutoff, currentType? } 은 훅 뒤에도 다시 읽는다.
// 훅(gunAnimation 훅과 같은 모양) { center, targetPoint, unpop, repop, sound, createProjectile, notify, kill } 와 beam { create(source,target), refresh(source), hit(source,target), random } 은 도달한 지점에서만 요구한다.
// frames[type].frames 는 FrameCodeTable(codes[].side 글자 코드, run(글자))이다. 실제 GunProcess 연결(MakeCannonFireHostHooks/CreateCannonFire)은 RawGunProcess 이식 뒤로 둔다. 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-08 WB04-E114: RawCannonFire 이식으로 추가)
import { SquidProcess } from './squid-process.js';

// 실제 가상 단계표/생성자와 투사체 getter 를 고르는 복원 종류이다(원본 열거 순서). (2026-11-08 WB04-E114)
export const CannonFireKind = Object.freeze({ Sun: 0, Thunder: 1, Vander: 2 });

// 현재 시각과 원본 가상 getter/효과음 단계 전역이다. 호출 시점에 읽으므로 훅이 값을 바꾸면 이후 단계에 반영된다. 사용: new CannonFireState({ now: 10, kind: CannonFireKind.Sun, type: 128 }). (2026-11-08 WB04-E114)
export class CannonFireState {
    // 같은 Kernel 게임 시각, 실제 파생 종류, 부모·투사체 표시·효과의 현재 TYPE 이다.
    now = 0;
    kind = CannonFireKind.Thunder;
    type = 131;
    displayType = 132;
    effectType = 0;
    // 실제 가상 속도(float)/지연(float)과 발사 지연 배율(float), 배율을 곱하는 부모 TYPE 이다.
    speed = 20;
    delay = 0.5;
    launchScale = 0.25;
    scaledType = 131;
    // 마지막 프레임에서 소리를 내는 두 전역 차이이다.
    growlOffset = 5;
    roarOffset = 1;
    // 공통 launch 의 즉시 빔 부모 TYPE 과 Cloud Floater 비교 TYPE, 회피 기준(부호 있는 DWORD)이다.
    vanderType = 148;
    floaterType = 146;
    avoidanceCutoff = 800;
    // 실제 Sun 파생은 00541258 을 빌린다. { value } 이면 매번 다시 읽는다.
    currentType = null;

    // 지정한 값만 덮어쓴다. (2026-11-08 WB04-E114)
    constructor(init = {}) { Object.assign(this, init); }

    // 생성/부모 검증이 쓰는 현재 TYPE 전체 DWORD 이다. (2026-11-08 WB04-E114)
    get currentTypeValue() { return this.currentType ? this.currentType.value : this.type; }
}

// 슬롯/필드 위치, 타입 수, HP 플래그, 단계 코드 비트이다. (2026-11-08 WB04-E114)
const TYPE_AT = 10;
const STATE_AT = 11;
const FRAME_AT = 36;
const TYPE_COUNT = 188;
const HIT_POINTS = 0x10;
const FIRE_BIT = 0x4000;
const FINISH_BIT = 0x8000;
const FRAME_MASK = 0x3fff;
// 객체 필드 위치(Form/Parent/위치/중심/내장 표/dirty/건/표적/시각/원위치)이다. (2026-11-08 WB04-E114)
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

// float32/double 변환 버퍼이다. (2026-11-08 WB04-E114)
const floatView = new DataView(new ArrayBuffer(4));
const doubleView = new DataView(new ArrayBuffer(8));
const fround = Math.fround;

// 미정렬 리틀엔디언 필드를 읽는다(부호 없음). (2026-11-08 WB04-E114)
export function readField(bytes, offset, width = 4) {
    let value = 0;
    // 낮은 바이트부터 지정한 폭만 조합한다.
    for (let index = 0; index < width; index += 1) value += bytes[offset + index] * 2 ** (8 * index);
    return value;
}

// 주변 padding 을 보존하며 지정 폭으로 쓴다. (2026-11-08 WB04-E114)
export function writeField(bytes, offset, value, width = 4) {
    const wrapped = value >>> 0;
    // 낮은 바이트부터 지정한 폭만 쓴다.
    for (let index = 0; index < width; index += 1) bytes[offset + index] = (wrapped >>> (8 * index)) & 0xff;
}

// 필드의 단정도 좌표를 읽는다. (2026-11-08 WB04-E114)
function coordinate(bytes, offset) {
    floatView.setUint32(0, readField(bytes, offset), true);
    return floatView.getFloat32(0, true);
}

// 단정도 값을 원본 비트(uint32)로 바꾼다. (2026-11-08 WB04-E114)
function floatBits(value) {
    floatView.setFloat32(0, value, true);
    return floatView.getUint32(0, true);
}

// 현재 double 시각을 원본 두 DWORD 비트로 쓴다. (2026-11-08 WB04-E114)
function putTime(fields, value) {
    doubleView.setFloat64(0, value, true);
    writeField(fields.bytes, TIME, doubleView.getUint32(0, true));
    writeField(fields.bytes, TIME + 4, doubleView.getUint32(4, true));
}

// 필드의 double 시각을 읽는다. (2026-11-08 WB04-E114)
function readTime(fields) {
    doubleView.setUint32(0, readField(fields.bytes, TIME), true);
    doubleView.setUint32(4, readField(fields.bytes, TIME + 4), true);
    return doubleView.getFloat64(0, true);
}

// 필수 훅을 확인한다. (2026-11-08 WB04-E114)
function need(hook, message) {
    if (typeof hook !== 'function') throw new Error(message);
    return hook;
}

// 원본 Sun/Thunder/Vander Fire 의 64바이트 몸체이다. 사용: new RawCannonFire(pool, types, frames, state, hooks, beam). (2026-11-08 WB04-E114)
export class RawCannonFire {
    // 실제 자산·Form 의 단일 풀, 표적 HP 플래그 표, 타입별 코드/글자 구간 표, 현재 전역, 공간·중심·소리·Missile·완료 훅, 즉시 빔 훅이다.
    #pool;
    #types;
    #frames;
    #state;
    #hooks;
    #beam;

    // 10.78 의 실제 파생만 지원한다(CD 가상 표를 추정하지 않는다). 판본/표 크기/현재 TYPE 범위를 검사한다. (2026-11-08 WB04-E114)
    constructor(pool, types, frames, state, hooks = {}, beam = {}) {
        if (pool?.edition !== 'patch1078' || !types || types.length !== TYPE_COUNT || !frames || frames.length !== types.length || !state || state.currentTypeValue >= types.length) throw new Error('캐논 발사 실제 판본/타입 자료 오류');
        this.#pool = pool;
        this.#types = types;
        this.#frames = frames;
        this.#state = state;
        this.#hooks = hooks;
        this.#beam = beam;
    }

    // 같은 풀/TYPE 를 쓰는 factory 계약 조회이다. (2026-11-08 WB04-E114)
    get pool() { return this.#pool; }
    get type() { return this.#state.currentTypeValue; }

    // 썬 00468460, 썬더 004684d0, 반더의 공통 접두·파생 가상 표를 쓴다(Host 부착은 호출자). 사용: initialize(fields, gunForm, target). (2026-11-08 WB04-E114)
    initialize(fields, gunForm, target) {
        if (!gunForm || gunForm >= this.#pool.capacity) throw new RangeError('캐논 발사 GunForm 오류');
        const kind = this.#state.kind;
        const vander = kind === CannonFireKind.Vander;
        const sun = kind === CannonFireKind.Sun;
        const bytes = fields.bytes;
        writeField(bytes, 0, vander ? 0x50b170 : sun ? 0x50b318 : 0x50b370);
        writeField(bytes, FORM, 0);
        writeField(bytes, PARENT, 0);
        writeField(bytes, INNER, vander ? 0x50b16c : sun ? 0x50b310 : 0x50b368);
        bytes[DIRTY] = 0xff;
        writeField(bytes, GUN, gunForm, 2);
        writeField(bytes, TARGET, target, 2);
        putTime(fields, this.#state.now);
        writeField(bytes, REST, 0);
    }

    // 현재 부모 번호와 복원한 TYPE 을 검사한다. 범위 밖/미복원 TYPE 은 슬롯을 수정하기 전에 거부한다. (2026-11-08 WB04-E114)
    #parent(fields) {
        const parent = fields.parent;
        if (!parent || parent >= this.#pool.capacity || this.#pool.slot(parent)[TYPE_AT] !== this.#state.currentTypeValue) throw new Error('캐논 발사 미복원 부모 TYPE/번호');
        return parent;
    }

    // 현재 부모를 공간 해제 → DWORD 프레임 쓰기 → 재등록(0x40)한다. 훅이 부모를 바꿀 수 있어 같은 getter 를 다시 읽는다. (2026-11-08 WB04-E114)
    #setFrame(fields, frame) {
        const hooks = this.#hooks;
        if (typeof hooks.unpop !== 'function' || typeof hooks.repop !== 'function') throw new Error('캐논 발사 실제 공간 경계 누락');
        const parent = this.#parent(fields);
        hooks.unpop(parent, 0);
        writeField(this.#pool.allocated(parent), FRAME_AT, frame);
        hooks.repop(parent, 0x40);
    }

    // 완료 통지는 GunForm 을 통지 전에, 삭제할 현재 Form 은 통지 뒤에 조회하는 원문 순서이다. (2026-11-08 WB04-E114)
    #finish(fields) {
        const hooks = this.#hooks;
        if (typeof hooks.notify !== 'function' || typeof hooks.kill !== 'function') throw new Error('캐논 발사 실제 완료 경계 누락');
        const gun = fields.gunForm;
        hooks.notify(gun, 1, 6, 7);
        hooks.kill(fields.form, 0x10);
    }

    // 004684b0/00468520/00468130 의 단계표/효과음과 반더 동시 비트를 계산한다(uint32). 사용: step(fields, frame). (2026-11-08 WB04-E114)
    step(fields, frame) {
        const state = this.#state;
        if (state.kind === CannonFireKind.Vander) return ((frame | 0) | 0xc000) >>> 0;
        if (state.kind === CannonFireKind.Sun) return (frame < 24 ? (((frame + 24) | 0) | FIRE_BIT) : (((frame - 24) | 0) | FINISH_BIT)) >>> 0;
        const parent = this.#parent(fields);
        const raw = this.#pool.slot(parent);
        const current = readField(raw, FRAME_AT);
        const table = this.#frames[raw[TYPE_AT]].frames;
        const codes = table.codes;
        if (current >= codes.length) throw new RangeError('썬더 캐논 현재 프레임 코드 범위 오류');
        const run = table.run(codes[current].side);
        const last = run.first + run.count - 1;
        const next = (frame + 1) | 0;
        if (next > last) return (run.first | FINISH_BIT) >>> 0;
        if (next === last) return (next | FIRE_BIT) >>> 0;
        if (next === last - state.growlOffset) this.#sound('thunderCannonGrowl.wav', fields, '썬더 캐논 Growl 음향 경계 누락');
        if (next === last - this.#state.roarOffset) this.#sound('thunderCannonRoar.wav', fields, '썬더 캐논 Roar 음향 경계 누락');
        return next >>> 0;
    }

    // 현재 부모 위치에서 소리를 낸다(부모는 호출 시점에 다시 읽는다). (2026-11-08 WB04-E114)
    #sound(name, fields, message) {
        const sound = need(this.#hooks.sound, message);
        sound(this.#parent(fields), name);
    }

    // 00466f50: 일반 투사체와 즉시 빔을 구별한다. 초기 source/target 슬롯 주소는 snapshot 이라 훅 뒤의 현재 바이트를 다시 읽는다. 사용: launch(fields) → 생성/피해 처리 여부. (2026-11-08 WB04-E114)
    launch(fields) {
        const state = this.#state;
        const hooks = this.#hooks;
        const beam = this.#beam;
        const source = this.#parent(fields);
        const target = fields.target;
        // 슬롯 주소를 잡아 두므로 이후 훅이 바꾸는 바이트가 그대로 보이는 live 뷰이다.
        const initialSource = this.#pool.physical(source);
        if (!target || target >= this.#pool.capacity) throw new RangeError('캐논 발사 표적 번호 오류');
        const raw = this.#pool.physical(target);
        if (raw[STATE_AT] & 3) return false;
        if (raw[TYPE_AT] >= this.#types.length) throw new RangeError('캐논 발사 표적 TYPE 오류');
        if (!(this.#types[raw[TYPE_AT]].flags1 & HIT_POINTS)) return false;
        need(hooks.targetPoint, '캐논 발사 실제 표적 코너 경계 누락');
        const vander = initialSource[TYPE_AT] === state.vanderType;
        if (vander) {
            if (typeof beam.create !== 'function' || typeof beam.refresh !== 'function' || typeof beam.hit !== 'function' || !beam.random) throw new Error('반더 실제 빔/피해/난수 경계 누락');
        } else need(hooks.createProjectile, '캐논 발사 실제 Missile 경계 누락');
        const corner = hooks.targetPoint(fields.parent, fields.target);
        if (vander) {
            beam.create(fields.parent, fields.target);
            let avoided = (raw[STATE_AT] & 3) !== 0;
            if (!avoided && raw[TYPE_AT] === state.floaterType) avoided = beam.random.next(65536) * (1.0 / 65536.0) * 1000 < (state.avoidanceCutoff | 0);
            if (avoided) beam.refresh(source);
            else beam.hit(source, fields.target);
            return (raw[STATE_AT] & 3) === 0;
        }
        need(hooks.createProjectile, '현재 캐논 실제 Missile 경계 누락');
        hooks.createProjectile({
            firing: fields.parent,
            target: fields.target,
            speed: fround(state.speed),
            displayType: state.displayType,
            effectType: state.effectType,
            targetPosition: [coordinate(fields.bytes, POSITION), coordinate(fields.bytes, POSITION + 4)],
            targetCenter: [fround(corner[0]), fround(corner[1])],
            ricochet: 0,
            firingPosition: [-10000, -10000]
        });
        return true;
    }

    // 467330 프레임 몸체이다. 처음 추적한 표적의 생존 상태를 효과 이후 현재 표적과 구별한다. 사용: frame(fields). (2026-11-08 WB04-E114)
    frame(fields) {
        this.frameWithStep(fields, (current) => this.step(fields, current));
    }

    // 실제 공통 부모가 고른 가상 step 뒤에도 현재 부모/표적/시각을 같은 위치에서 다시 읽는다(80바이트 아이스 파생이 step 만 바꿔 쓴다). (2026-11-08 WB04-E114)
    frameWithStep(fields, step) {
        const target = fields.target;
        if (!target || target >= this.#pool.capacity) throw new Error('캐논 발사 표적 0 원본 assert');
        const raw = this.#pool.physical(target);
        const dead = (raw[STATE_AT] & 3) !== 0;
        if (!dead) {
            const center = need(this.#hooks.center, '캐논 발사 실제 표적 중심 경계 누락');
            writeField(fields.bytes, POSITION, readField(raw, 14));
            writeField(fields.bytes, POSITION + 4, readField(raw, 18));
            const at = center(target);
            writeField(fields.bytes, CENTER, floatBits(at[0]));
            writeField(fields.bytes, CENTER + 4, floatBits(at[1]));
        }
        const source = this.#pool.physical(this.#parent(fields));
        if (!(readTime(fields) <= this.#state.now)) return;
        const current = readField(source, FRAME_AT) | 0;
        if (typeof step !== 'function') throw new Error('캐논 발사의 실제 가상 Step 미연결');
        const code = step(current) >>> 0;
        this.#setFrame(fields, code & FRAME_MASK);
        if ((code & FIRE_BIT) && !this.launch(fields) && !dead) return;
        if (code & FINISH_BIT) {
            this.#finish(fields);
            return;
        }
        const state = this.#state;
        fields.bytes[DIRTY] |= 8;
        putTime(fields, state.now);
        const delay = fround(state.delay) * (((code & FIRE_BIT) && source[TYPE_AT] === state.scaledType) ? fround(state.launchScale) : 1.0);
        putTime(fields, state.now + delay);
    }
}

// 같은 Kernel/Host 에서 실제 파생 발사를 실행하는 TYPE 35 프로세스이다. 사용: new CannonFireProcess(logic, fields) 를 호스트에 attach 한다. (2026-11-08 WB04-E114)
export class CannonFireProcess extends SquidProcess {
    // 객체보다 오래 사는 실제 몸체와 padding 을 포함한 64바이트 객체 이미지이다.
    #logic;
    #fields;

    // 실제 원본 이미지와 장기 처리기만 보존한다. (2026-11-08 WB04-E114)
    constructor(logic, fields) {
        super();
        this.#logic = logic;
        this.#fields = fields;
    }

    // 같은 Kernel 의 실제 공통 발사 프레임을 실행한다. (2026-11-08 WB04-E114)
    runFrame() { this.#logic.frame(this.#fields); }

    // 호스트가 확정한 Form/Parent 를 원본 DWORD 에 쓴다. (2026-11-08 WB04-E114)
    onAttached() {
        writeField(this.#fields.bytes, FORM, this.form);
        writeField(this.#fields.bytes, PARENT, this.parent);
    }

    // 후속 관찰/전송이 같은 객체를 읽는다. (2026-11-08 WB04-E114)
    get fields() { return this.#fields; }
}
