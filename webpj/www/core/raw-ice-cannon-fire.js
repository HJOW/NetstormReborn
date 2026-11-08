// 아이스 캐논의 80바이트 파생 발사(004685e0/00468660)를 공통 발사(raw-cannon-fire.js)와 같은 467330 프레임 몸체에 연결한 모듈이다(cpppj `o/RawIceCannonFire.cpp/.h`, 10.78 전용).
// 공통 64바이트와 +40..+4f 의 미초기화 padding 을 각각 보존한다(+40 active DWORD / +44 padding / +48 double 전환 시각).
//  - initialize(fields, gunForm, target): 공통 초기화 뒤 두 가상 표(0x50b3f0/0x50b3ec)·active 1·전환 시각 `현재+4.0` 만 쓴다.
//  - step(fields, frame): 전환 시각이 지났으면(NaN 이면 건너뜀) active 가 켜져 있을 때 active 를 끄고 냉각 표시(cooling 훅)를 생성한 뒤 `현재(훅 뒤)+6.0` 으로 미루며 입력 프레임을 그대로 돌려주고,
//    꺼져 있으면 active 를 켜고 `현재+4.0` 으로 둔다. 그다음 현재 부모(현재 TYPE 이어야 함)의 프레임 코드 글자 구간에서 다음 칸을 구해, 구간 끝을 넘으면 처음 칸, 아니면 문맥(context 훅)을 만들어 mayTarget 이 거짓이면 `|0x8000`,
//    참이면 `|0x4000`(다음 칸이 끝이고 active 일 때만)을 붙여 돌려준다.
//  - frame(fields): 같은 467330 공통 추적/공간/발사/완료를 쓰되 가상 step 만 위 step 이다. launch(fields): 공통 발사의 같은 투사체 생성(아이스 표시/속도 getter 는 state 가 담는다).
//  - IceCannonFireProcess(SquidProcess): Kernel 슬롯 프로세스이다. runFrame 은 frame, onAttached 는 Form/Parent 기록.
// 훅 iceHooks = { cooling(sid), context(sid)→8 DWORD 배열, mayTarget(context, target)→bool } 는 도달한 지점에서만 요구한다. 실제 GunProcess 연결(MakeIceCannonFireHostHooks/CreateIceCannonFire)은 RawGunProcess 이식 뒤로 둔다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E114: RawIceCannonFire 이식으로 추가)
import { SquidProcess } from './squid-process.js';
import { GunAnimationFields } from './raw-gun-animation.js';
import { RawCannonFire, readField, writeField } from './raw-cannon-fire.js';

// 슬롯·필드 위치와 단계 코드 비트이다. (2026-11-08 WB04-E114)
const TYPE_AT = 10;
const FRAME_AT = 36;
const FORM = 4;
const PARENT = 8;
const EXTRA_SIZE = 16;
const ACTIVE = 0;
const DEADLINE = 8;
const FIRE_BIT = 0x4000;
const FINISH_BIT = 0x8000;
const ACTIVE_SECONDS = 4.0;
const COOLING_SECONDS = 6.0;

// double 비트 변환 버퍼이다. (2026-11-08 WB04-E114)
const doubleView = new DataView(new ArrayBuffer(8));

// 아이스 파생 80바이트 필드 이미지이다(공통 64바이트 + 전용 16바이트). 사용: new IceCannonFireFields(). (2026-11-08 WB04-E114)
export class IceCannonFireFields {
    // 기존 467330 이 읽고 쓰는 같은 접두이다.
    common = new GunAnimationFields();
    // +40 active DWORD / +44 padding / +48 double 전환 시각이다.
    extra = new Uint8Array(EXTRA_SIZE);
}

// +48 전환 시각 double 을 원본 비트로 읽는다. (2026-11-08 WB04-E114)
function deadline(fields) {
    doubleView.setUint32(0, readField(fields.extra, DEADLINE), true);
    doubleView.setUint32(4, readField(fields.extra, DEADLINE + 4), true);
    return doubleView.getFloat64(0, true);
}

// +48 전환 시각 double 을 원본 비트로 쓴다(+44 padding 과 공통 dirty 는 건드리지 않는다). (2026-11-08 WB04-E114)
function setDeadline(fields, value) {
    doubleView.setFloat64(0, value, true);
    writeField(fields.extra, DEADLINE, doubleView.getUint32(0, true));
    writeField(fields.extra, DEADLINE + 4, doubleView.getUint32(4, true));
}

// 아이스 캐논 발사 논리이다. 사용: new RawIceCannonFire(pool, types, frames, state, hooks, iceHooks, beam). (2026-11-08 WB04-E114)
export class RawIceCannonFire {
    // 현재 부모 프레임을 읽는 같은 풀, 글자 구간 표, 현재 전역(훅 뒤에도 다시 읽음), 아이스 훅, 공통 발사 몸체이다.
    #pool;
    #frames;
    #state;
    #hooks;
    #common;

    // 판본/범위 검사는 공통 생성자가 수행한다. (2026-11-08 WB04-E114)
    constructor(pool, types, frames, state, common = {}, hooks = {}, beam = {}) {
        this.#pool = pool;
        this.#frames = frames;
        this.#state = state;
        this.#hooks = hooks;
        this.#common = new RawCannonFire(pool, types, frames, state, common, beam);
    }

    // factory 가 같은 저장소/TYPE 를 검증한다. (2026-11-08 WB04-E114)
    get pool() { return this.#pool; }
    get type() { return this.#state.currentTypeValue; }

    // 004685e0: 공통 생성자가 끝난 뒤 파생 쓰기만 수행한다(padding 보존). 사용: initialize(fields, gunForm, target). (2026-11-08 WB04-E114)
    initialize(fields, gunForm, target) {
        this.#common.initialize(fields.common, gunForm, target);
        writeField(fields.common.bytes, 0, 0x50b3f0);
        writeField(fields.common.bytes, 32, 0x50b3ec);
        writeField(fields.extra, ACTIVE, 1);
        setDeadline(fields, this.#state.now + ACTIVE_SECONDS);
    }

    // 00468660: 냉각/문자 구간/표적 판정이다. fcomp/test C0 는 unordered(NaN) 때 전환을 건너뛴다. 반환은 uint32 단계 코드이다. 사용: step(fields, frame). (2026-11-08 WB04-E114)
    step(fields, frame) {
        const state = this.#state;
        if (state.now >= deadline(fields)) {
            if (readField(fields.extra, ACTIVE)) {
                writeField(fields.extra, ACTIVE, 0);
                if (typeof this.#hooks.cooling !== 'function') throw new Error('아이스 캐논의 실제 냉각 표시 004a8770 미연결');
                this.#hooks.cooling(fields.common.parent);
                setDeadline(fields, state.now + COOLING_SECONDS);
                return (frame | 0) >>> 0;
            }
            writeField(fields.extra, ACTIVE, 1);
            setDeadline(fields, state.now + ACTIVE_SECONDS);
        }
        const parent = fields.common.parent;
        if (!parent || parent >= this.#pool.capacity) throw new RangeError('아이스 캐논 현재 부모 범위');
        const raw = this.#pool.slot(parent);
        if (raw[TYPE_AT] !== state.currentTypeValue) throw new Error('아이스 캐논 현재 부모 TYPE');
        const current = readField(raw, FRAME_AT);
        const table = this.#frames[raw[TYPE_AT]].frames;
        const codes = table.codes;
        if (current >= codes.length) throw new RangeError('아이스 캐논 현재 프레임 코드');
        const run = table.run(codes[current].side);
        const last = ((run.first >>> 0) + (run.count >>> 0) - 1) | 0;
        const next = ((frame >>> 0) + 1) | 0;
        if (next > last) return run.first >>> 0;
        if (typeof this.#hooks.context !== 'function') throw new Error('아이스 캐논 실제 8DWORD 문맥 미연결');
        // context 효과 뒤의 현재 target 을 읽는다.
        const context = this.#hooks.context(fields.common.parent);
        if (typeof this.#hooks.mayTarget !== 'function') throw new Error('아이스 캐논 실제 MayTarget 미연결');
        if (!this.#hooks.mayTarget(context, fields.common.target)) return (next | FINISH_BIT) >>> 0;
        return (next | ((next === last && readField(fields.extra, ACTIVE)) ? FIRE_BIT : 0)) >>> 0;
    }

    // 같은 467330 의 실제 가상 step 으로 진행한다. (2026-11-08 WB04-E114)
    frame(fields) {
        this.#common.frameWithStep(fields.common, (frame) => this.step(fields, frame));
    }

    // 동일한 실제 투사체 생성의 가상 getter 만 아이스 상태이다. (2026-11-08 WB04-E114)
    launch(fields) { return this.#common.launch(fields.common); }
}

// Kernel 35 의 아이스 발사 파생 프로세스이다. 사용: new IceCannonFireProcess(logic, fields) 를 호스트에 attach 한다. (2026-11-08 WB04-E114)
export class IceCannonFireProcess extends SquidProcess {
    // 프로세스보다 오래 사는 월드 논리와 padding 을 포함한 80바이트 객체 상태이다.
    #logic;
    #fields;

    // 같은 장기 논리와 현재 80바이트를 보유한다. (2026-11-08 WB04-E114)
    constructor(logic, fields) {
        super();
        this.#logic = logic;
        this.#fields = fields;
    }

    // Kernel 의 현재 틱을 같은 원문 몸체로 전달한다. (2026-11-08 WB04-E114)
    runFrame() { this.#logic.frame(this.#fields); }

    // 실제 호스트가 확정한 Form/Parent DWORD 만 기록한다. (2026-11-08 WB04-E114)
    onAttached() {
        writeField(this.#fields.common.bytes, FORM, this.form);
        writeField(this.#fields.common.bytes, PARENT, this.parent);
    }

    // 관찰/전송은 같은 실제 객체 상태를 사용한다. (2026-11-08 WB04-E114)
    get fields() { return this.#fields; }
}
