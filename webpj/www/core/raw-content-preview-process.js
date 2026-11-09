// Carrier 가 만든 일시적 내용물 그림(미리보기)을 만료 시각에 부모 삭제로 정리하는 10.78 프로세스를 옮긴 모듈이다(cpppj `o/RawContentPreviewProcess.cpp/.h`, 10.78 전용: 428700 생성자·428770 프레임·480260 통지·468230→41bd00 소멸).
//  - ContentPreviewFields: 원문 24바이트(+0 VT·+4 Form·+8 부모·+c 미정 패딩·+10 double 만료 시각)이다. `bytes` 를 그대로 보관해 생성자가 건드리지 않는 바이트(+c 패딩 등)를 보존한다.
//    deadline() 은 +10 의 현재 double 이고, parent() 는 +8 의 현재 부모 번호이다(SID 범위 0xffff 를 넘으면 호스트가 RangeError 로 거부한다).
//  - new RawContentPreviewProcess(pool, globals, hooks): pool 은 같은 PATCH SidPool 이어야 한다(10.72 풀은 TypeError). globals = { now()(DAT55b4d0 현재 게임 시각 double), processType()(DAT54112c 현재 값, SoundProcess 와 같은 전역) } 는 소비 위치마다 다시 읽는다.
//    hooks = { attach(fields, parent, type, flags)(41bd20 부착: 같은 필드·부모·현재 타입·flags 0x10), destroy(parent, flags)(만료한 부모의 현재 가상+10 에 flags 0x400000 전달),
//    assertion()(41bfb0 의 `parent->getHead() != 0` 진단이며 반환하면 캡처한 부모를 계속 쓴다), release(fields)(scalar destructor flags 하위 bit0 이 선택하는 원본 할당 해제 경계) } 는 도달한 위치에서만 요구한다(없으면 그 지점에서 TypeError).
//  - construct(fields, parent, duration): +8/+4/+0 을 0/0/0x503c1c 로 쓰고 +10 을 double 0 으로 초기화한 뒤(500460), float 로 줄인 duration 에 현재 시각을 더한 double 을 +10 에 쓰고 attach 를 호출한다.
//  - run(fields): `now >= deadline` 일 때만 동작한다(NaN 비교는 건너뜀). 부모를 먼저 고정하고 head(+6 WORD)가 0 이면 진단을 보고한 뒤 destroy(parent, 0x400000) 를 호출한다.
//  - RawContentPreviewProcess.notify: 실제 480260 은 세 인자를 읽거나 쓰지 않는 빈 몸체이다. destruct(fields, flags): VT/Form/부모를 0x5019ec/0xffffcfc7/0xffffcfc7 로 쓰고 flags bit0 이면 release 를 요구한다.
//  - ContentPreviewProcess: 같은 Kernel 과 ProcessForm 수명에 24바이트를 연결하는 `SquidProcess` 파생 래퍼이다. runFrame(Host 에 부착된 때만 run)·onAttached(Host 의 풀이 같은지 검사한 뒤 Form/부모를 +4/+8 에 기록)·notify(빈 통지)·destroy(Kernel 소멸 시 flags 0 소멸 쓰기)·fields 를 제공한다.
// 10.72(CD)는 범위 밖이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-10 WB04-E241: 내용물 미리보기 프로세스 이식으로 추가)
import { SquidProcess } from './squid-process.js';

// 24바이트 안의 필드 위치(VT·Form·부모·미정 패딩 다음의 double 만료 시각)와 원문 상수(만료 부모 삭제 flags·부착 flags·생성/소멸 VT·소멸 뒤 표식)이다. (2026-11-10 WB04-E241)
const VT_AT = 0;
const FORM_AT = 4;
const PARENT_AT = 8;
const DEADLINE_AT = 16;
const EXPIRY_DESTROY_FLAGS = 0x400000;
const ATTACH_FLAGS = 0x10;
const CONSTRUCT_VT = 0x503c1c;
const DESTRUCT_VT = 0x5019ec;
const DESTRUCT_MARK = 0xffffcfc7;
const FIELD_BYTES = 24;
const MAX_SID = 0xffff;

// 비정렬 DWORD 를 읽는다(낮은 바이트부터 정확히 네 바이트만 합친다). (2026-11-10 WB04-E241)
function read(raw, at) {
    return (raw[at] | (raw[at + 1] << 8) | (raw[at + 2] << 16) | (raw[at + 3] << 24)) >>> 0;
}

// 지정한 DWORD 만 써 +c 패딩과 만료 시각을 보존한다. (2026-11-10 WB04-E241)
function put(raw, at, value) {
    for (let index = 0; index < 4; index += 1) raw[at + index] = (value >>> (8 * index)) & 0xff;
}

// 선택된 경계만 검사한다. 미만료/NaN/빈 통지는 효과를 요구하지 않는다. (2026-11-10 WB04-E241)
function need(hook) {
    if (typeof hook !== 'function') throw new TypeError('내용물 미리보기 프로세스의 실제 효과 미연결');
    return hook;
}

// 원문 24바이트이다. `bytes` 를 직접 읽고 쓰므로 생성자가 건드리지 않는 +c 패딩도 호출 사이에 보존된다. (2026-11-10 WB04-E241)
export class ContentPreviewFields {
    // 24바이트를 0 으로 만든다(호출자가 원문 입력으로 채울 수 있다).
    constructor() {
        this.bytes = new Uint8Array(FIELD_BYTES);
    }

    // +10 의 현재 double 만료 시각이다(비트를 그대로 해석한다).
    deadline() {
        const view = new DataView(new ArrayBuffer(8));
        view.setUint32(0, read(this.bytes, DEADLINE_AT), true);
        view.setUint32(4, read(this.bytes, DEADLINE_AT + 4), true);
        return view.getFloat64(0, true);
    }

    // +8 의 현재 부모 번호이다. SID 범위(0xffff)를 넘는 DWORD 는 호스트가 RangeError 로 거부한다.
    parent() {
        const parent = read(this.bytes, PARENT_AT);
        if (parent > MAX_SID) throw new RangeError('미리보기 부모 DWORD가 SID 범위를 벗어났습니다');
        return parent;
    }
}

// 원본 x87 가 피연산자에 NaN 이 없는데 무효 연산(∞ + −∞)을 할 때 쓰는 "실수 불확정" 비트이다(부호 비트가 켜진 조용한 NaN). (2026-11-10 WB04-E241)
const INDEFINITE_NAN_BITS = 0xfff8000000000000n;

// float 로 줄인 duration 에 현재 시각(double)을 더한 값의 64비트 비트열이다. 피연산자가 모두 NaN 이 아닌데 합이 NaN 이면(∞ + −∞) 호스트와 무관하게 x87 의 실수 불확정 비트를 쓰고,
// 피연산자가 NaN 이면 호스트의 NaN 전파(첫 NaN 피연산자의 조용한 형태)를 따른다. (2026-11-10 WB04-E241)
function sumBits(duration, now) {
    const narrowed = Math.fround(duration);
    const sum = narrowed + now;
    const view = new DataView(new ArrayBuffer(8));
    if (Number.isNaN(sum) && !Number.isNaN(narrowed) && !Number.isNaN(now)) return INDEFINITE_NAN_BITS;
    view.setFloat64(0, sum, true);
    return view.getBigUint64(0, true);
}

// 내용물 미리보기 프로세스의 생성·실행·통지·소멸 몸체이다. (2026-11-10 WB04-E241)
export class RawContentPreviewProcess {
    #pool;
    #globals;
    #hooks;

    // 시계/타입/풀은 로직보다 오래 살아야 한다. 10.78 풀이 아니면 거부한다.
    constructor(pool, globals, hooks) {
        if (pool?.edition !== 'patch1078') throw new TypeError('내용물 미리보기 프로세스는 10.78 몸체입니다');
        this.#pool = pool;
        this.#globals = globals;
        this.#hooks = hooks ?? {};
    }

    // 다른 Host 의 부모/프로세스를 혼합하지 않게 검사하는 실제 풀이다.
    get pool() { return this.#pool; }

    // 428700: duration 을 float 로 줄여 현재 시각을 더한 double 을 만료 시각으로 쓰고 같은 필드/부모/현재 타입/flags 0x10 을 부착한다. 부착 효과가 없으면 앞선 쓰기를 남긴 채 거부한다.
    construct(fields, parent, duration) {
        put(fields.bytes, PARENT_AT, 0);
        put(fields.bytes, FORM_AT, 0);
        put(fields.bytes, VT_AT, CONSTRUCT_VT);
        put(fields.bytes, DEADLINE_AT, 0);
        put(fields.bytes, DEADLINE_AT + 4, 0); // 500460 의 원문 double 0 초기화 뒤 실제 만료 시각을 덮어쓴다.
        const deadline = sumBits(duration, this.#globals.now());
        put(fields.bytes, DEADLINE_AT, Number(deadline & 0xffffffffn));
        put(fields.bytes, DEADLINE_AT + 4, Number(deadline >> 32n));
        need(this.#hooks.attach)(fields, parent, this.#globals.processType() >>> 0, ATTACH_FLAGS);
    }

    // 428770: now >= deadline 일 때만 부모를 지운다(NaN 비교는 건너뜀). 부모 포인터는 진단 전에 고정하고 진단이 반환해도 그 부모를 계속 쓴다.
    run(fields) {
        if (!(this.#globals.now() >= fields.deadline())) return;
        const parent = fields.parent();
        const raw = this.#pool.slot(parent); // 41bfb0 은 진단 전에 부모 포인터를 고정한다.
        if (!(raw[6] | raw[7])) need(this.#hooks.assertion)();
        need(this.#hooks.destroy)(parent, EXPIRY_DESTROY_FLAGS);
    }

    // 실제 480260 은 세 인자를 정리하는 빈 몸체이다(상태를 읽거나 쓰지 않는다).
    static notify(fields, event, info, value) {}

    // 468230→41bd00: 소멸 표식을 쓰고 flags 하위 bit0 이면 원본 할당 해제 경계를 요구한다.
    destruct(fields, flags) {
        put(fields.bytes, VT_AT, DESTRUCT_VT);
        put(fields.bytes, FORM_AT, DESTRUCT_MARK);
        put(fields.bytes, PARENT_AT, DESTRUCT_MARK);
        if (flags & 1) need(this.#hooks.release)(fields);
    }
}

// 같은 Kernel 과 ProcessForm 수명에 원문 24바이트를 연결하는 프로세스 래퍼이다. 실제 생성은 construct 의 attach 경계가 수행한다. (2026-11-10 WB04-E241)
export class ContentPreviewProcess extends SquidProcess {
    #logic;
    #fields;

    // 생성자가 이미 계산한 필드를 보관한다.
    constructor(logic, fields) {
        super();
        this.#logic = logic;
        this.#fields = fields;
    }

    // 같은 Host 에 부착한 프로세스만 실행한다(삭제 뒤 멤버를 다시 읽지 않는다).
    runFrame() { if (this.host) this.#logic.run(this.#fields); }

    // 실제 Host 가 확정한 Form/부모를 두 DWORD 에 쓴다. Host 의 풀이 로직의 풀과 다르면 거부한다.
    onAttached() {
        if (!this.host || this.host.pool !== this.#logic.pool) throw new Error('미리보기 프로세스와 Host의 풀 불일치');
        put(this.#fields.bytes, FORM_AT, this.form);
        put(this.#fields.bytes, PARENT_AT, this.parent);
    }

    // 가상 1c 의 실제 빈 통지이다.
    notify(event, info, value) { RawContentPreviewProcess.notify(this.#fields, event, info, value); }

    // C++ 소유권 해제 직전의 원본 base 소멸 쓰기(flags 0)이다. Kernel 이 소멸할 때 부른다.
    destroy() { this.#logic.destruct(this.#fields, 0); }

    // 현재 단일 원문 필드를 사본 없이 돌려준다.
    get fields() { return this.#fields; }
}
