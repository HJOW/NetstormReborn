// 크로스보우(windArcher)의 내림 발사(004674c0)와 생성자(00468050), 00466f50 의 일반 투사체(arrow81) 분기를 옮긴 모듈이다(cpppj `o/RawCrossbowAnimation.cpp/.h`, 10.78 전용).
// 캐논과 같은 64바이트 객체 이미지(GunAnimationFields)를 쓰며, 회전은 아처의 `RawArcherAnimation.rotate` 를 부모 TYPE 140 으로 공유한다(이 모듈은 발사만 소유한다).
//  - initialize(fields, gunForm, target): 공통 ctor 접두(가상 표 0x50b120·내장 표 0x50b11c·dirty 0xff·gun/target WORD·현재 시각·원위치 방향 0)만 쓰고 Form/Parent 는 0 으로 둔다(실제 Host 부착이 채움).
//  - frame(fields): 시각이 도달하기 전에는 아무것도 하지 않는다(추적도 하지 않음). 도달하면 표적이 live 일 때 위치/중심을 재기록하고, 현재 부모 프레임이 한 방향의 첫 칸(`프레임 % 방향당 프레임 == 0`)이면
//    프레임을 `프레임 + 방향당 프레임 − 1` 로 바꾸고 launch 한 뒤(실패해도 처음 추적 당시 죽어 있던 표적이면 계속) 완료(통지 info 6/value 7 + 삭제 0x10)하며, 아니면 프레임을 1 줄이고 float 지연 뒤로 예약한다.
//  - launch(fields): live HP 표적일 때만 arrow Missile 요청(속도 35·표시 81·효과 0·포구 -10000,-10000)을 훅에 넘긴다(생성 경계가 없으면 성공으로 우회하지 않고 거부). 요청의 현재 필드/전역은 표적점 훅 뒤에 다시 읽는다.
//  - CrossbowAnimationProcess(SquidProcess): Kernel 슬롯 프로세스이다. runFrame 은 frame, onAttached 는 Form/Parent 기록.
// state(CrossbowAnimationState)와 animationTypes[type]={directions,frameCount} 는 호출 시점에 읽는다. 훅 { center, targetPoint, unpop, repop, createProjectile, notify, kill } 는 도달한 지점에서만 요구한다.
// 실제 GunProcess 연결(MakeCrossbowAnimationHostHooks/CreateCrossbowAnimation)은 RawGunProcess 이식 뒤로 둔다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E116: RawCrossbowAnimation 이식으로 추가)
import { SquidProcess } from './squid-process.js';

// 필드 위치(Form/Parent/위치/중심/내장 표/dirty/건/표적/시각/원위치)이다. (2026-11-08 WB04-E116)
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
// 슬롯 위치(TYPE·상태·프레임 DWORD), 판본 타입 수, HP 플래그이다. (2026-11-08 WB04-E116)
const TYPE_AT = 10;
const STATE_AT = 11;
const FRAME_AT = 36;
const TYPE_COUNT = 188;
const HIT_POINTS = 0x10;
const fround = Math.fround;

// float/double 변환 버퍼이다. (2026-11-08 WB04-E116)
const floatView = new DataView(new ArrayBuffer(4));
const doubleView = new DataView(new ArrayBuffer(8));

// 미정렬 리틀엔디언 필드를 지정한 폭으로 읽는다. (2026-11-08 WB04-E116)
function read(bytes, offset, width = 4) {
    let value = 0;
    // 낮은 바이트부터 지정한 폭만 조합한다.
    for (let index = 0; index < width; index += 1) value += bytes[offset + index] * 2 ** (8 * index);
    return value;
}

// 원본 저장 폭 밖의 padding 을 변경하지 않고 쓴다. (2026-11-08 WB04-E116)
function put(bytes, offset, value, width = 4) {
    const wrapped = value >>> 0;
    // 낮은 바이트부터 지정한 폭만 쓴다.
    for (let index = 0; index < width; index += 1) bytes[offset + index] = (wrapped >>> (8 * index)) & 0xff;
}

// 현재 추적 좌표의 단정도 값을 읽는다. (2026-11-08 WB04-E116)
function coordinate(bytes, offset) {
    floatView.setUint32(0, read(bytes, offset), true);
    return floatView.getFloat32(0, true);
}

// 단정도 값의 원본 비트이다. (2026-11-08 WB04-E116)
function floatBits(value) {
    floatView.setFloat32(0, value, true);
    return floatView.getUint32(0, true);
}

// 실제 double 시각을 정렬에 의존하지 않고 저장한다. (2026-11-08 WB04-E116)
function putTime(fields, value) {
    doubleView.setFloat64(0, value, true);
    put(fields.bytes, TIME, doubleView.getUint32(0, true));
    put(fields.bytes, TIME + 4, doubleView.getUint32(4, true));
}

// 10.78 크로스보우 가상 getter 의 현재 게임 시각과 원본 전역이다. 호출 시점에 읽는다. 사용: new CrossbowAnimationState({ now: 10 }). (2026-11-08 WB04-E116)
export class CrossbowAnimationState {
    // 같은 Kernel 이 공급하는 현재 게임 시각, 실제 windArcher 와 arrow 의 원본 TYPE 이다.
    now = 0;
    type = 140;
    displayType = 81;
    // 0050b168/00508d98 에서 읽은 발사 속도(float)와 지연(float)이다.
    speed = 35;
    delay = 0.2;
    // 실제 00541288 을 수신 공장과 공유하는 { value } 이다. 없으면 type 을 쓴다.
    currentType = null;

    // 지정한 값만 덮어쓴다. (2026-11-08 WB04-E116)
    constructor(init = {}) { Object.assign(this, init); }

    // 생성/부모 검증이 쓰는 현재 TYPE 전체 DWORD 이다. (2026-11-08 WB04-E116)
    get currentTypeValue() { return this.currentType ? this.currentType.value : this.type; }
}

// 크로스보우 발사 논리이다. 사용: new RawCrossbowAnimation(pool, types, animationTypes, state, hooks). (2026-11-08 WB04-E116)
export class RawCrossbowAnimation {
    // 실제 자산과 Form 이 공유하는 풀, 표적 HP 플래그 표, 방향 수/물리 프레임 수 표, 현재 시각/가상 getter 전역, 공간·중심·투사체·완료 훅이다.
    #pool;
    #types;
    #animationTypes;
    #state;
    #hooks;

    // 크로스보우는 우선 10.78 에 한정하며 다른 판본의 주소를 추정하지 않는다. (2026-11-08 WB04-E116)
    constructor(pool, types, animationTypes, state, hooks = {}) {
        if (pool?.edition !== 'patch1078' || !types || types.length !== TYPE_COUNT || !animationTypes || animationTypes.length !== types.length || !state || state.currentTypeValue >= types.length) throw new Error('크로스보우 실제 판본/TYPE 자료 오류');
        this.#pool = pool;
        this.#types = types;
        this.#animationTypes = animationTypes;
        this.#state = state;
        this.#hooks = hooks;
    }

    // factory 가 같은 풀과 복원 TYPE 만 검사한다. (2026-11-08 WB04-E116)
    get pool() { return this.#pool; }
    get type() { return this.#state.currentTypeValue; }

    // 00468050 은 64바이트 중 지정한 필드만 초기화하며 공통 Host 부착이 Form/Parent 를 채운다. 사용: initialize(fields, gunForm, target). (2026-11-08 WB04-E116)
    initialize(fields, gunForm, target) {
        if (!gunForm || gunForm >= this.#pool.capacity) throw new RangeError('크로스보우 GunForm 오류');
        const bytes = fields.bytes;
        put(bytes, 0, 0x50b120);
        put(bytes, FORM, 0);
        put(bytes, PARENT, 0);
        put(bytes, INNER, 0x50b11c);
        bytes[DIRTY] = 0xff;
        put(bytes, GUN, gunForm, 2);
        put(bytes, TARGET, target, 2);
        putTime(fields, this.#state.now);
        put(bytes, REST, 0);
    }

    // 실제 부모 번호/TYPE 가 이 몸체의 현재 입력인지 확인한다. (2026-11-08 WB04-E116)
    #parent(fields) {
        const parent = fields.parent;
        if (!parent || parent >= this.#pool.capacity || this.#pool.slot(parent)[TYPE_AT] !== this.#state.currentTypeValue) throw new Error('크로스보우 미복원 부모 TYPE/번호');
        return parent;
    }

    // 실제 함수는 최초 부모 snapshot 과 별개로 프레임 변경 시 부모를 다시 조회한다(Unpop → 쓰기 → Repop 0x40). (2026-11-08 WB04-E116)
    #setFrame(fields, frame) {
        const hooks = this.#hooks;
        if (typeof hooks.unpop !== 'function' || typeof hooks.repop !== 'function') throw new Error('크로스보우 실제 프레임 공간 경계 누락');
        const parent = this.#parent(fields);
        hooks.unpop(parent, 0);
        put(this.#pool.allocated(parent), FRAME_AT, frame);
        hooks.repop(parent, 0x40);
    }

    // 완료 통지가 현재 animation 을 없앨 수 있으므로 필요한 번호를 미리 보존한다. (2026-11-08 WB04-E116)
    #finish(fields) {
        const hooks = this.#hooks;
        if (typeof hooks.notify !== 'function' || typeof hooks.kill !== 'function') throw new Error('크로스보우 실제 완료 경계 누락');
        const gun = fields.gunForm;
        const form = fields.form;
        hooks.notify(gun, 1, 6, 7);
        hooks.kill(form, 0x10);
    }

    // 00466f50 의 일반 투사체 분기를 실행하며 실제 ctor 가 없으면 성공으로 간주하지 않는다. 사용: launch(fields) → 생성 요청 여부. (2026-11-08 WB04-E116)
    launch(fields) {
        this.#parent(fields);
        const target = fields.target;
        if (!target || target >= this.#pool.capacity) throw new RangeError('크로스보우 발사 표적 오류');
        // 표적 슬롯 주소를 잡아 두므로 훅 뒤 바뀐 바이트를 그대로 읽는 live 뷰이다.
        const raw = this.#pool.physical(target);
        if (raw[STATE_AT] & 3) return false;
        if (raw[TYPE_AT] >= this.#types.length) throw new RangeError('크로스보우 표적 TYPE 오류');
        if (!(this.#types[raw[TYPE_AT]].flags1 & HIT_POINTS)) return false;
        const hooks = this.#hooks;
        if (typeof hooks.targetPoint !== 'function' || typeof hooks.createProjectile !== 'function') throw new Error('크로스보우 실제 Missile 생성 경계 누락');
        const corner = hooks.targetPoint(fields.parent, fields.target);
        const state = this.#state;
        hooks.createProjectile({
            firing: fields.parent,
            target: fields.target,
            speed: fround(state.speed),
            displayType: state.displayType,
            effectType: 0,
            targetPosition: [coordinate(fields.bytes, POSITION), coordinate(fields.bytes, POSITION + 4)],
            targetCenter: [fround(corner[0]), fround(corner[1])],
            ricochet: 0,
            firingPosition: [-10000, -10000]
        });
        return true;
    }

    // 004674c0 은 due 전 추적하지 않으며 발사 실패 당시 살아 있던 표적이면 완료를 보류한다. 사용: frame(fields). (2026-11-08 WB04-E116)
    frame(fields) {
        const state = this.#state;
        if (!(fields.time <= state.now)) return;
        const parent = this.#parent(fields);
        // 부모/표적 슬롯 주소를 잡아 두므로 중심 훅 뒤 바뀐 바이트를 그대로 읽는 live 뷰이다.
        const source = this.#pool.physical(parent);
        const target = fields.target;
        if (!target || target >= this.#pool.capacity) throw new RangeError('크로스보우 현재 표적 오류');
        const raw = this.#pool.physical(target);
        const dead = (raw[STATE_AT] & 3) !== 0;
        if (!dead) {
            if (typeof this.#hooks.center !== 'function') throw new Error('크로스보우 실제 표적 중심 경계 누락');
            put(fields.bytes, POSITION, read(raw, 14));
            put(fields.bytes, POSITION + 4, read(raw, 18));
            const center = this.#hooks.center(target);
            put(fields.bytes, CENTER, floatBits(center[0]));
            put(fields.bytes, CENTER + 4, floatBits(center[1]));
        }
        const data = this.#animationTypes[source[TYPE_AT]];
        if (data.directions <= 0 || Math.trunc(data.frameCount / data.directions) <= 0) throw new Error('크로스보우 실제 방향/프레임 자료 미공급');
        const per = Math.trunc(data.frameCount / data.directions);
        const frame = read(source, FRAME_AT) | 0;
        if (frame % per === 0) {
            this.#setFrame(fields, frame + per - 1);
            if (!this.launch(fields) && !dead) return;
            this.#finish(fields);
            return;
        }
        this.#setFrame(fields, frame - 1);
        fields.bytes[DIRTY] |= 8;
        putTime(fields, state.now);
        putTime(fields, state.now + fround(state.delay));
    }
}

// 같은 Kernel 에서 실제 크로스보우 발사 몸체를 실행하는 35 프로세스이다. 사용: new CrossbowAnimationProcess(logic, fields) 를 호스트에 attach 한다. (2026-11-08 WB04-E116)
export class CrossbowAnimationProcess extends SquidProcess {
    // 객체보다 오래 사는 실제 발사 처리기와 padding 을 포함한 64바이트 원본 이미지이다.
    #logic;
    #fields;

    // 실제 원본 필드 이미지와 장기 처리기만 소유/참조한다. (2026-11-08 WB04-E116)
    constructor(logic, fields) {
        super();
        this.#logic = logic;
        this.#fields = fields;
    }

    // Kernel 의 실제 발사 프레임을 한 번 처리한다. (2026-11-08 WB04-E116)
    runFrame() { this.#logic.frame(this.#fields); }

    // Host 가 확정한 Form/Parent 를 원본 DWORD 에 쓴다. (2026-11-08 WB04-E116)
    onAttached() {
        put(this.#fields.bytes, FORM, this.form);
        put(this.#fields.bytes, PARENT, this.parent);
    }

    // 후속 관찰과 전송은 이 원본 이미지에 접근한다. (2026-11-08 WB04-E116)
    get fields() { return this.#fields; }
}
