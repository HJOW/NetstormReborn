// 훨리베이스(선 아비어리)의 실제 48바이트 생산 상태를 현재 Kernel 과 같은 풀에 연결하는 모듈이다(cpppj `o/RawSunAviaryProcess.cpp/.h`, 10.78 전용: 0045ec50 후처리·0045d690 생성자·inner 0045d8c0 프레임·0045d6e0/0045b070 상태 writer·0045b4a0 홈 통지).
//  - SunAviaryProcessFields: 원문 outer0/inner+20 의 48바이트 `bytes`(+8 dirty·+16 다음 프레임 시각 double·+24 AP 상태·+32 inner VT·+36 inner Form·+40 inner 부모·+44 흔들림 횟수). 생성자가 쓰지 않는 padding 과 비서버 상태도 호출자의 입력 그대로 보존한다.
//  - new RawSunAviaryProcess(pool, globals, hooks): globals = { server(), editor()(005c85a4 비영이면 Frame 생략), checking()(005e4794 개발자 프레임 진단), now(), processType()(005410f0 현재 32), flyerType()(0054117c 현재 73) } 은 모두 현재 값을 읽는 함수이고,
//    hooks = { findProcess(parent, type)→SID, allocate(size)→SunAviaryProcessFields|null, attach(fields|null, parent, type, flags), damageable(parent, flags), assertion(line), setFrame(sid, frame), sound(x, y, name),
//    checkFrame(sid, frame, line), create(type, flags)→SID, setOwner(sid, owner), pop(sid, x, y, flags), bindFlyer(flyer, home), refreshForm(fields), processFields(form)→fields|null, unpop(sid, flags), repop(sid, flags) } 이다.
//    선택하지 않은 경계는 요구하지 않으며 선택된 누락은 앞선 쓰기를 보존하고 거부한다.
//  - initialize(fields): 0045d690 — 시각 0·dirty 0xff·inner VT 0x5019ec→0x509db8·부모/Form 0·외부 VT 0x509dd8·횟수 0, 서버이면 시각 now·dirty |= 4·상태 1. 비서버는 상태를 초기화하지 않는다.
//  - postPop(parent, flags): 0045ec50 — 서버이고 flags bit0 이면 이미 부착된 타입 프로세스가 있을 때 진단 0x1e2 후 새 48바이트를 확보(NULL 이면 NULL 부착 요청)·초기화·flags 8 로 부착하고, 마지막에 Damageable 후처리이다.
//  - frame(fields): inner 0045d8c0 — 부모의 정지(+40 의 0x20)·편집 차단이면 무동작, 상태 1(홈 프레임 27·AP 8·apStart 소리)·2(25 이하까지 0.2초마다 프레임 +1)·3(대기→4)·4(내림)·6(서버: 플라이어 생성·Pop·연결→3·3초 대기)·8/9(흔들림 횟수 9 초과까지 프레임 왕복)·그 밖(개발자 진단 0x75e)을 실행한다.
//    진입 때 읽은 프레임·부모 포인터를 유지하고 효과 뒤 상태/횟수/시각/좌표는 원본 위치에서 다시 읽는다.
//  - flyerDied(home)/flyerDiedIn(...): 홈 가상 34=0045b4a0 — 서버만: 조회 → Unpop → 홈 프레임 27(직접 DWORD) → Repop → AP 8·dirty 4 → Refresh. 조회 결과 NULL 도 공간 효과 뒤에야 거부한다(원본 NULL 역참조).
//  - writeProcess(fields, packet, includeParent, force, protocol, message=0x38): 0045d6e0→0045b070 의 변경 비트(2 시각 8바이트·4 상태 DWORD)와 길이를 기록한다. 총 길이는 쓰기 전에 검사한다.
//  - SunAviaryProcess: inner Baseprocess 의 실제 Kernel 수명(`SquidProcess` 파생). runFrame·onAttached(Form/부모 반영)·notify(빈 통지)·fields·writePacket 을 가진다.
// findAttachedProcess 는 윈드 아비어리 모듈과 같은 원문 004b1a90 이다. 10.72(CD)는 범위 밖이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-13 WB04-E353: 7-P1 RawSunAviaryProcess 이식으로 추가)
import { SquidProcess } from './squid-process.js';
// 원문 004b1a90(skip/delete 0 의 첫 일치 조회)은 윈드 아비어리와 같은 함수라 한 구현을 재노출한다(2026-11-13 WB04-E353).
export { findAttachedProcess } from './raw-wind-aviary-process.js';

// 이 모듈이 쓰는 원본 필드 위치(dirty·시각·상태·inner Form·inner 부모·횟수)와 inner VT 위치이다. (2026-11-13 WB04-E353)
const DIRTY = 8, TIME = 16, STAGE = 24, INNER = 32, FORM = 36, PARENT = 40, COUNTER = 44;
// PE 005066e0/00501448 의 double 상수(프레임 간격 0.2f 를 배정도로 본 값·발사 대기 3초)와 float 상수(0.45f)이다. (2026-11-13 WB04-E353)
const FRAME_DELAY = 0.20000000298023224;
const LAUNCH_DELAY = 3;
const LAUNCH_X = Math.fround(0.45);

// 비정렬 값을 리틀엔디언 부호 없는 수로 읽는다. (2026-11-13 WB04-E353)
function read(raw, at, width = 4) {
    let value = 0;
    // 필드 폭만 읽는다.
    for (let index = 0; index < width; index += 1) value += raw[at + index] * 2 ** (8 * index);
    return value;
}

// 지정 폭만 쓴다(다른 payload 를 건드리지 않음). (2026-11-13 WB04-E353)
function put(raw, at, value, width = 4) {
    // 낮은 바이트부터 지정 폭만 쓴다.
    for (let index = 0; index < width; index += 1) raw[at + index] = Math.floor(value / 2 ** (8 * index)) & 0xff;
}

// float/double 비트 변환용 공용 보기이다. (2026-11-13 WB04-E353)
const SCRATCH = new DataView(new ArrayBuffer(8));

// 원문 좌표의 IEEE 비트를 float 로 읽는다. (2026-11-13 WB04-E353)
function position(raw, at) {
    SCRATCH.setUint32(0, read(raw, at), true);
    return SCRATCH.getFloat32(0, true);
}

// 선택된 누락만 앞선 쓰기를 보존하고 거부한다. (2026-11-13 WB04-E353)
function need(hook) {
    if (typeof hook !== 'function') throw new Error('훨리베이스 실제 하위 소비자 미연결');
    return hook;
}

// 원본 signed 값의 덧셈은 낮은 DWORD 에서 감긴다. (2026-11-13 WB04-E353)
function add(value, delta) {
    return (value + delta) | 0;
}

// 실제 double 저장을 두 DWORD 로 나누고 padding 을 보존한다. (2026-11-13 WB04-E353)
function storeTime(fields, value) {
    SCRATCH.setFloat64(0, value, true);
    put(fields.bytes, TIME, SCRATCH.getUint32(0, true));
    put(fields.bytes, TIME + 4, SCRATCH.getUint32(4, true));
}

// 단계 변경은 dirty 비트 4 와 함께 쓴다. (2026-11-13 WB04-E353)
function setStage(fields, value) {
    fields.bytes[DIRTY] |= 4;
    put(fields.bytes, STAGE, value);
}

// 이 타이머는 현재 시각을 저장한 뒤 같은 위치에서 now + delay 를 다시 저장한다(dirty 비트 2). (2026-11-13 WB04-E353)
function setTime(fields, now, delay) {
    fields.bytes[DIRTY] |= 2;
    storeTime(fields, now);
    storeTime(fields, now + delay);
}

// 생성자가 쓰지 않는 padding 과 비서버 상태도 caller 입력으로 보존하는 48바이트 이미지이다. (2026-11-13 WB04-E353)
export class SunAviaryProcessFields {
    constructor() {
        // 실제 outer0/inner+20 의 48바이트 이미지이다.
        this.bytes = new Uint8Array(0x30);
    }

    // outer+8 의 원본 변경 비트이다.
    get dirty() { return this.bytes[DIRTY]; }

    // outer+10 의 다음 프레임 시각(raw double)이다.
    get time() {
        for (let index = 0; index < 8; index += 1) SCRATCH.setUint8(index, this.bytes[TIME + index]);
        return SCRATCH.getFloat64(0, true);
    }

    // outer+18 의 AP 상태 DWORD 이다.
    get stage() { return read(this.bytes, STAGE); }

    // outer+2c 의 흔들림 횟수(signed 비교와 별개인 원문 DWORD)이다.
    get counter() { return read(this.bytes, COUNTER); }

    // inner+4(outer+24)의 현재 Form 하위 WORD 이다.
    get form() { return read(this.bytes, FORM) & 0xffff; }

    // inner+8(outer+28)의 현재 부모 하위 WORD 이다.
    get parent() { return read(this.bytes, PARENT) & 0xffff; }
}

// 부모 계산을 원본 raw 저장소에서 수행하며 서로 다른 월드의 사본을 만들지 않는 훨리베이스 로직이다. (2026-11-13 WB04-E353)
export class RawSunAviaryProcess {
    #pool;
    #globals;
    #hooks;

    // PATCH 와 현재 전역 수명을 확인한다. (2026-11-13 WB04-E353)
    constructor(pool, globals, hooks) {
        if (pool?.edition !== 'patch1078') throw new TypeError('훨리베이스 프로세스는 PATCH만 복원했습니다');
        this.#pool = pool;
        this.#globals = globals;
        this.#hooks = hooks;
    }

    // Host 가 같은 풀인지 확인할 수 있게 공개한다. (2026-11-13 WB04-E353)
    get pool() { return this.#pool; }

    // 0045d690: 비서버는 상태를 초기화하지 않는 원문과 unused padding 을 보존한다. (2026-11-13 WB04-E353)
    initialize(fields) {
        storeTime(fields, 0);
        fields.bytes[DIRTY] = 0xff;
        put(fields.bytes, INNER, 0x5019ec);
        put(fields.bytes, PARENT, 0);
        put(fields.bytes, FORM, 0);
        put(fields.bytes, 0, 0x509dd8);
        put(fields.bytes, INNER, 0x509db8);
        put(fields.bytes, COUNTER, 0);
        if (this.#globals.server()) {
            storeTime(fields, this.#globals.now());
            fields.bytes[DIRTY] |= 4;
            put(fields.bytes, STAGE, 1);
        }
    }

    // 0045ec50: 실제 할당 실패도 NULL inner 부착 요청으로 전달한다. 서버 관문은 한 번만 읽는다. (2026-11-13 WB04-E353)
    postPop(parent, flags) {
        const hooks = this.#hooks;
        if (this.#globals.server() && (flags & 1)) {
            if (need(hooks.findProcess)(parent, this.#globals.processType())) need(hooks.assertion)(0x1e2);
            const fields = need(hooks.allocate)(0x30);
            if (fields) this.initialize(fields);
            need(hooks.attach)(fields ?? null, parent, this.#globals.processType(), 8);
        }
        need(hooks.damageable)(parent, flags);
    }

    // 홈 가상 34=0045b4a0: 서버 관문은 한 번만 읽고 조회해 둔 프로세스 포인터를 공간 효과 뒤에도 유지한다. (2026-11-13 WB04-E353)
    flyerDied(home) {
        if (!this.#globals.server()) return;
        const hooks = this.#hooks;
        // 원본 4b1a90 의 현재 타입/첫 일치이다.
        const form = need(hooks.findProcess)(home, this.#globals.processType());
        // 원본은 NULL 도 공간 효과 뒤에야 역참조한다.
        const fields = form ? need(hooks.processFields)(form) : null;
        RawSunAviaryProcess.flyerDiedIn(this.#pool, fields, home, hooks.unpop, hooks.repop, hooks.refreshForm);
    }

    // 원문은 조회 결과가 NULL 이어도 Unpop/직접 27/Repop 을 끝낸 뒤에야 프로세스 필드를 소비한다. nativePhysical 이면 Native 장기 통지처럼 물리 슬롯에 쓴다. (2026-11-13 WB04-E353)
    static flyerDiedIn(pool, fields, home, unpop, repop, refresh, nativePhysical = false) {
        need(unpop)(home, 0);
        const raw = nativePhysical ? pool.physical(home) : pool.allocated(home);
        // Native 는 공간 효과 뒤 같은 물리 슬롯에 실제 DWORD 만 쓴다.
        put(raw, 36, 27);
        need(repop)(home, 0);
        if (!fields) throw new Error('훨리베이스 홈 통지의 현재 프로세스 NULL 원본 경계');
        fields.bytes[DIRTY] |= 4;
        put(fields.bytes, STAGE, 8);
        need(refresh)(fields);
    }

    // inner 0045d8c0: stage 2/4 의 frame 은 진입 시 읽으며 호출 뒤 단계/횟수/시각만 원본 위치에서 다시 읽는다. (2026-11-13 WB04-E353)
    frame(fields) {
        const pool = this.#pool;
        const globals = this.#globals;
        const hooks = this.#hooks;
        const parent = fields.parent;
        // 첫 부모 포인터의 현재 raw 슬롯(살아 있는 보기)이다. 효과가 슬롯을 바꾸면 그 뒤 읽기는 현재 값을 본다.
        const raw = pool.physical(parent);
        if ((raw[40] & 0x20) || globals.editor()) return;
        const stage = fields.stage;
        const frame = read(raw, 36) | 0;
        // 원본 ordered x87 비교이며 unordered 는 거짓이다.
        const due = () => globals.now() >= fields.time;
        // 첫 부모 포인터의 현재 좌표이다.
        const sound = () => { need(hooks.sound)(position(raw, 14), position(raw, 18), 'apStart.wav'); };
        switch (stage) {
        case 1:
            need(hooks.setFrame)(parent, 27);
            setStage(fields, 8);
            sound();
            return;
        case 2:
            if (frame > 25) { setStage(fields, 6); return; }
            if (!due()) return;
            if (globals.checking()) need(hooks.checkFrame)(parent, add(frame, 1), 0x724);
            need(hooks.setFrame)(parent, add(frame, 1));
            setTime(fields, globals.now(), FRAME_DELAY);
            return;
        case 3:
            if (due()) setStage(fields, 4);
            return;
        case 4:
            if (frame < 1) { setStage(fields, 5); return; }
            if (!due()) return;
            if (globals.checking()) need(hooks.checkFrame)(parent, add(frame, -1), 0x751);
            need(hooks.setFrame)(parent, add(frame, -1));
            setTime(fields, globals.now(), FRAME_DELAY);
            return;
        case 6: {
            if (!globals.server()) return;
            const flyer = need(hooks.create)(globals.flyerType(), 0);
            put(pool.allocated(flyer), 36, 0);
            need(hooks.setOwner)(flyer, raw[34]);
            const current = fields.parent;
            if (!read(pool.slot(current), 6, 2)) need(hooks.assertion)(0xa4);
            put(pool.allocated(current), 12, flyer, 2);
            const x = Math.fround(position(raw, 14) + LAUNCH_X);
            const y = Math.fround(position(raw, 18) - 3);
            put(pool.allocated(flyer), 12, fields.parent, 2);
            need(hooks.pop)(flyer, x, y, 0);
            need(hooks.bindFlyer)(flyer, fields.parent);
            setStage(fields, 3);
            setTime(fields, globals.now(), LAUNCH_DELAY);
            need(hooks.refreshForm)(fields);
            return;
        }
        case 8: case 9: {
            if (!due()) return;
            // 원문 진입 frame 과 별도의 계산 결과이다.
            let next = frame;
            if (!(fields.counter & 1)) {
                if (frame > 29) {
                    put(fields.bytes, COUNTER, (fields.counter + 1) >>> 0);
                    next = add(frame, -1);
                } else next = add(frame, 1);
            } else if (frame > 27) next = add(frame, -1);
            else {
                if (fields.stage === 8) sound();
                put(fields.bytes, COUNTER, (fields.counter + 1) >>> 0);
                next = add(frame, 1);
            }
            need(hooks.setFrame)(parent, next);
            if ((fields.counter | 0) > 9) {
                put(fields.bytes, COUNTER, 0);
                if (fields.stage === 8) {
                    need(hooks.setFrame)(parent, 11);
                    put(fields.bytes, STAGE, 2);
                } else put(fields.bytes, STAGE, 5);
                fields.bytes[DIRTY] |= 4;
            }
            setTime(fields, globals.now(), FRAME_DELAY);
            return;
        }
        default:
            if (globals.checking() && (stage < 1 || stage > 10)) need(hooks.assertion)(0x75e);
        }
    }

    // 0045d6e0 의 현재 inner 인자 선택과 0045b070 의 BYTE/WORD/DWORD 쓰기를 결합한다. 총 길이는 쓰기 전에 검사하고 모자라면 필드를 바꾸지 않는다. (2026-11-13 WB04-E353)
    static writeProcess(fields, packet, includeParent, force = false, protocol = 1, message = 0x38) {
        const parent = (includeParent || force) ? fields.parent : 0;
        const dirty = force ? 0xff : fields.dirty;
        const length = 9 + (parent ? 4 : 0) + ((dirty & 2) ? 8 : 0) + ((dirty & 4) ? 4 : 0);
        if (packet.length < length) throw new RangeError('훨리베이스 패킷 버퍼 크기 오류');
        if (force) fields.bytes[DIRTY] = 0xff;
        packet[0] = protocol;
        packet[1] = message;
        put(packet, 4, fields.form);
        let at = 9;
        if (parent) {
            fields.bytes[DIRTY] |= 1;
            put(packet, at, parent);
            at += 4;
        }
        if (fields.dirty & 2) {
            packet.set(fields.bytes.subarray(TIME, TIME + 8), at);
            at += 8;
        }
        if (fields.dirty & 4) {
            put(packet, at, fields.stage);
            at += 4;
        }
        put(packet, 2, at, 2);
        packet[8] = fields.dirty;
        fields.bytes[DIRTY] = 0;
        return at;
    }
}

// 원본 inner Baseprocess 를 실제 Kernel 수명과 연결한다. (2026-11-13 WB04-E353)
export class SunAviaryProcess extends SquidProcess {
    #logic;
    #fields;

    // 이미 초기화한 원문 필드 소유권을 받는다. (2026-11-13 WB04-E353)
    constructor(logic, fields) {
        super();
        this.#logic = logic;
        this.#fields = fields;
    }

    // Kernel 의 같은 프레임을 복원한 상태 기계에 전달한다. (2026-11-13 WB04-E353)
    runFrame() { this.#logic.frame(this.#fields); }

    // 원본 Host 가 기록한 Form/부모를 같은 원문 필드에 반영한다. (2026-11-13 WB04-E353)
    onAttached() {
        put(this.#fields.bytes, FORM, this.form);
        put(this.#fields.bytes, PARENT, this.parent);
    }

    // 실제 inner+1c 는 00480260 의 ret12 빈 몸체이다. (2026-11-13 WB04-E353)
    notify() {}

    // 원문 writer/검사가 같은 이미지를 조회한다. (2026-11-13 WB04-E353)
    get fields() { return this.#fields; }

    // OnAttached 전에도 현재 Host 의 Form/부모를 쓴 뒤 writer 를 부른다. (2026-11-13 WB04-E353)
    writePacket(packet, parentChanged, force = false, protocol = 1, message = 0x38) {
        this.onAttached();
        return RawSunAviaryProcess.writeProcess(this.#fields, packet, parentChanged, force, protocol, message);
    }
}
