// 공통 비행 프로세스(맨 오 워·훨리기그·더스트 데빌 등이 공유하는 원문 112바이트 객체)의 생성/상태 전송/홈 연결과, 두 전용 비행체(맨 오 워 144·훨리기그 73)·더스트 데빌의 최초 후처리를 옮긴 모듈이다
// (cpppj `o/RawFlyerProcess.cpp/.h`, 10.78 전용: 0045c340 생성·0045c1b0 홈 연결·0045c100 홈 조회·45aed0/45c310 상태 writer·45afb0 수신·45ae40 해제·45bc30 종료·45e6b0 공통 후처리·45fac0/45ef30 전용 후처리).
//  - FlyerProcessFields: 원문 outer/inner 를 합친 `bytes`(112바이트: +8 dirty·+10 홈 WORD·+12/16/20/24 상태·+32/40 시각 double·+48 inner VT·+52 Form·+56 부모·+64/72 시각·+84 단계·+96/104 전용 타이머)와
//    훨리기그 전용 꼬리 `sunTail`(+70..87 의 24바이트, 112바이트 관측과 따로 보존). 생성자가 건드리지 않는 padding 도 호출자의 입력이다.
//  - RawFlyerProcess: construct(기본 표 쓰기와 inner 부착 순서)·bind/home(홈 WORD 쓰기/조회)·command(1·2·3 기한)·finish(서버 진단→파편→부모 삭제)·frame/notify/parentDying(큰 하위 몸체는 훅으로, 없으면 거부) 와
//    정적 함수 specializeRain/specializeSun/specializeWind(전용 표·타이머 쓰기)·initializeReceived(수신용 기본 생성자)·readData(수신 dirty 필드 복사)·destructFields(−12345 해제 표식)·returnDelay·writeProcess(상태 패킷 writer).
//  - FlyerProcess: 원본 Kernel 소유 프로세스(`SquidProcess` 파생)가 필드를 소유하고 runFrame/onAttached/notify/onParentDying 을 로직에 전달한다(부착 전 writePacket 도 현재 Host 의 Form/부모를 기록).
//  - RawFlyerPostPop: 전용 가상 표 0x50a388(맨 오 워)·0x50a1e8(훨리기그)·0x50a050(더스트 데빌)만 처리하는 후처리(45e6b0 공통 + 전용 부분): 더스트 데빌 최초 Pop 의 event0 중복 확인과 Regular 부착, 훨리기그의 홈 재배치·전용 프로세스·소리,
//    맨 오 워의 프로세스 생성, 그림자 생성/재배치, 공통 단위 초기화와 Damageable 후처리.
// 훅과 전역은 현재 값을 매번 읽는 getter/함수이다: globals = { server(), now(), processType()(005410ec 비행 타입 31), checking()(홈 조회의 finder 뒤 processSid 진단 관문, 생략하면 0) },
//  hooks = { assertion(line), attach(fields, parent, type, flags), findProcess(sid, type), fields(form)→FlyerProcessFields, frame(fields), notify(fields, message, info1, info2), parentDying(fields), pieces(sid), destroy(sid, flags) }.
// 10.72(CD)는 범위 밖이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-13 WB04-E352: 7-P1 비행 유닛 부품 RawFlyerProcess 이식으로 추가)
import { SquidProcess } from './squid-process.js';

// 원본 outer/inner 필드 위치(dirty·홈·inner VT·inner Form·inner 부모)이다. (2026-11-13 WB04-E352)
const DIRTY = 8;
const HOME = 10;
const INNER = 48;
const FORM = 52;
const PARENT = 56;
// dirty 2..80 이 켜는 전송 필드의 실제 오프셋/폭(두 WORD·세 DWORD·두 8바이트)이다. (2026-11-13 WB04-E352)
const DATA_OFFSETS = Object.freeze([10, 12, 16, 20, 24, 32, 40]);
const DATA_WIDTHS = Object.freeze([2, 2, 4, 4, 4, 8, 8]);
// 비행 파생 가상 표 주소(기본·Rain·Sun·Wind)와 inner VT 이다. (2026-11-13 WB04-E352)
const BASE_TABLE = 0x509cc8, RAIN_TABLE = 0x509f60, SUN_TABLE = 0x509f24, WIND_TABLE = 0x509f98;
// 부모 genus 비트(비행 종류 0x100000)와 홈/후처리가 읽는 필드, 전용 후처리 가상 표이다. (2026-11-13 WB04-E352)
const FLYER_GENUS = 0x100000;
const MAN_TABLE = 0x50a388, SUN_POST_TABLE = 0x50a1e8, WIND_POST_TABLE = 0x50a050;
// 그림자가 있는 비행체 TYPE flags1 비트와 프레임 필드(+36)·그림자 WORD(+8)이다. (2026-11-13 WB04-E352)
const SHADOW_FLAGS1 = 0x400000;

// 비정렬 값을 리틀엔디언 부호 없는 수로 읽는다. (2026-11-13 WB04-E352)
function read(raw, at, width = 4) {
    let value = 0;
    // 필드 폭만 읽는다.
    for (let index = 0; index < width; index += 1) value += raw[at + index] * 2 ** (8 * index);
    return value;
}

// 폭만큼 하위 바이트를 써 padding 과 인접 필드를 보존한다. (2026-11-13 WB04-E352)
function put(raw, at, value, width = 4) {
    // 낮은 바이트부터 지정 폭만 쓴다.
    for (let index = 0; index < width; index += 1) raw[at + index] = Math.floor(value / 2 ** (8 * index)) & 0xff;
}

// float/double 비트 변환용 공용 보기이다. (2026-11-13 WB04-E352)
const SCRATCH = new DataView(new ArrayBuffer(8));

// float 값의 비트 DWORD 이다. (2026-11-13 WB04-E352)
function floatBits(value) {
    SCRATCH.setFloat32(0, value, true);
    return SCRATCH.getUint32(0, true);
}

// DWORD 비트를 float 로 읽는다. (2026-11-13 WB04-E352)
function floatOf(bits) {
    SCRATCH.setUint32(0, bits >>> 0, true);
    return SCRATCH.getFloat32(0, true);
}

// 실제 double 저장의 낮은/높은 DWORD 를 쓴다. (2026-11-13 WB04-E352)
function time(fields, at, value) {
    SCRATCH.setFloat64(0, value, true);
    put(fields.bytes, at, SCRATCH.getUint32(0, true));
    put(fields.bytes, at + 4, SCRATCH.getUint32(4, true));
}

// 선택된 누락만 앞선 쓰기를 보존하고 거부한다. (2026-11-13 WB04-E352)
function need(hook) {
    if (typeof hook !== 'function') throw new Error('비행체 실제 하위 몸체 미연결');
    return hook;
}

// 프레임 덧셈의 하위 DWORD 감김을 보존한다. (2026-11-13 WB04-E352)
function frameSum(frames, raw) {
    return ((frames >>> 0) + read(raw, 36)) >>> 0;
}

// 원문 outer 의 112바이트 + 훨리기그 꼬리 24바이트 이미지이다. 생성 전 호출자의 padding 을 그대로 소유한다. (2026-11-13 WB04-E352)
export class FlyerProcessFields {
    constructor() {
        // 기본 outer0/inner+30 과 맨 오 워 추가 필드까지 같은 112바이트이다.
        this.bytes = new Uint8Array(0x70);
        // 훨리기그 0x88 객체의 +70..87 이다. 기존 112바이트 관측과 별도로 보존한다.
        this.sunTail = new Uint8Array(0x18);
    }

    // outer+34 의 현재 Form(inner 하위 WORD)이다.
    get form() { return read(this.bytes, FORM) & 0xffff; }

    // outer+38 의 현재 부모(inner 하위 WORD)이다.
    get parent() { return read(this.bytes, PARENT) & 0xffff; }

    // outer+a 의 홈 WORD 이다.
    get home() { return read(this.bytes, HOME, 2); }
}

// 더스트 데빌의 전용 표 쓰기(45e1c0, 기본 생성자의 부착 뒤): 전용 가상 표·inner VT·타이머 96·단계 84. 수신용 초기화와 후속 RawWindFlyer 가 같은 쓰기를 재사용한다. (2026-11-13 WB04-E352)
export function specializeWind(fields) {
    put(fields.bytes, 0, WIND_TABLE);
    put(fields.bytes, INNER, 0x509f78);
    time(fields, 96, 0);
    put(fields.bytes, 84, 0);
}

// 같은 풀의 실제 공통 프로세스 생성/전송과 홈 갱신만 담당한다. (2026-11-13 WB04-E352)
export class RawFlyerProcess {
    #pool;
    #types;
    #globals;
    #hooks;

    // PATCH 자료의 수명/크기를 확인한다. types 는 같은 현재 TYPE 표(flags2)이며 이 처리기보다 오래 살아야 한다. (2026-11-13 WB04-E352)
    constructor(pool, types, globals, hooks) {
        if (pool?.edition !== 'patch1078' || !Array.isArray(types) || types.length !== 188) throw new TypeError('비행 프로세스는 PATCH188 타입만 복원했습니다');
        this.#pool = pool;
        this.#types = types;
        this.#globals = globals;
        this.#hooks = hooks;
    }

    // 다른 Host 의 필드와 혼합하지 않도록 같은 풀을 공개한다. (2026-11-13 WB04-E352)
    get pool() { return this.#pool; }

    // 현재 checking 진단 DWORD(숫자 또는 getter, 생략하면 0)의 비영 여부이다. (2026-11-13 WB04-E352)
    #checking() {
        const value = this.#globals.checking;
        return ((typeof value === 'function' ? value() : value ?? 0) >>> 0) !== 0;
    }

    // 0045c340: 기본 생성자의 실제 부착은 맨 오 워 전용 표 쓰기보다 먼저다. 서버가 아니면 진단 0x2e3(정상 복귀 시 계속)을 낸다. (2026-11-13 WB04-E352)
    construct(fields, parent, home, speed, parameter) {
        time(fields, 32, 0);
        time(fields, 40, 0);
        fields.bytes[DIRTY] = 0xff;
        put(fields.bytes, INNER, 0x5019ec);
        put(fields.bytes, PARENT, 0);
        put(fields.bytes, FORM, 0);
        put(fields.bytes, 0, BASE_TABLE);
        put(fields.bytes, INNER, 0x509ca8);
        time(fields, 64, 0);
        time(fields, 72, 0);
        put(fields.bytes, 84, 0);
        if (!this.#globals.server()) need(this.#hooks.assertion)(0x2e3);
        put(fields.bytes, HOME, home, 2);
        fields.bytes[DIRTY] |= 0x1e;
        put(fields.bytes, 16, speed);
        put(fields.bytes, 12, 0, 2);
        put(fields.bytes, 20, floatBits(parameter));
        fields.bytes[DIRTY] |= 0x60;
        time(fields, 32, this.#globals.now());
        put(fields.bytes, 24, 0);
        need(this.#hooks.attach)(fields, parent, this.#globals.processType(), 8);
    }

    // 부착이 끝난 뒤 0045fac0 의 맨 오 워 추가 쓰기다. (2026-11-13 WB04-E352)
    static specializeRain(fields) {
        put(fields.bytes, 0, RAIN_TABLE);
        put(fields.bytes, INNER, 0x509f40);
        time(fields, 96, 0);
        time(fields, 104, 0);
        put(fields.bytes, 84, 0);
    }

    // 부착 뒤 45ef30 의 전용 표/타이머 104 와 꼬리 앞 16바이트·단계 84 만 쓴다. 원본은 +80/+84 를 생성자가 초기화하지 않으며 전용 첫 Frame 에서 쓴다. (2026-11-13 WB04-E352)
    static specializeSun(fields) {
        put(fields.bytes, 0, SUN_TABLE);
        put(fields.bytes, INNER, 0x509f04);
        time(fields, 104, 0);
        fields.sunTail.fill(0, 0, 16);
        put(fields.bytes, 84, 0);
    }

    // 45c280 뒤 theme 0/1/그 외의 45e280/45e180/45e130 기본 생성자만 실행한다. 위치/부착/첫 Frame 은 추가하지 않으며 미작성 home/위치/속도와 padding 을 보존한다. (2026-11-13 WB04-E352)
    static initializeReceived(fields, theme) {
        time(fields, 32, 0);
        time(fields, 40, 0);
        fields.bytes[DIRTY] = 0xff;
        put(fields.bytes, INNER, 0x5019ec);
        put(fields.bytes, PARENT, 0);
        put(fields.bytes, FORM, 0);
        put(fields.bytes, 0, BASE_TABLE);
        put(fields.bytes, INNER, 0x509ca8);
        time(fields, 64, 0);
        time(fields, 72, 0);
        put(fields.bytes, 12, 0, 2);
        put(fields.bytes, 84, 0);
        if (theme === 1) RawFlyerProcess.specializeRain(fields);
        else if (!theme) specializeWind(fields);
        else RawFlyerProcess.specializeSun(fields);
    }

    // 45afb0 수신: 같은 두 빈 observer 와 부모 DWORD/WORD·DWORD·8바이트 선택 복사이다. 반환은 패킷의 별도 부모 DWORD(없으면 0)이다. 앞선 부분 쓰기는 예외에서도 유지한다. (2026-11-13 WB04-E352)
    static readData(fields, packet) {
        if (packet.length < 9) throw new RangeError('비행 수신의 실제9BYTE 머리 부족');
        const dirty = packet[8];
        let offset = 9;
        let parent = 0;
        // 네 실제 outer VT0 은 모두 480260 의 ret12 이다. 미등록 가상을 같은 빈 몸체로 대체하지 않는다.
        const observe = () => {
            const table = read(fields.bytes, 0);
            if (table !== BASE_TABLE && table !== SUN_TABLE && table !== RAIN_TABLE && table !== WIND_TABLE) throw new Error('비행 데이터의 현재 가상0 observer 미연결');
        };
        const require = (width) => {
            if (offset > packet.length || width > packet.length - offset) throw new RangeError('비행 수신의 선택된 dirty 필드 부족');
        };
        observe();
        if (dirty & 1) {
            require(4);
            parent = read(packet, offset);
            offset += 4;
        }
        // 원문 WORD/DWORD/8바이트의 순서/비트만 복원한다.
        for (let index = 0; index < DATA_OFFSETS.length; index += 1) {
            if (dirty & (2 << index)) {
                require(DATA_WIDTHS[index]);
                fields.bytes.set(packet.subarray(offset, offset + DATA_WIDTHS[index]), DATA_OFFSETS[index]);
                offset += DATA_WIDTHS[index];
            }
        }
        observe();
        return parent;
    }

    // 45ae40→41bd00: inner VT 와 Form/부모를 −12345(DWORD 전체) 표식으로 둔다. 임시 수신 객체를 해제하기 직전에 사용한다. (2026-11-13 WB04-E352)
    static destructFields(fields) {
        put(fields.bytes, INNER, 0x5019ec);
        put(fields.bytes, FORM, 0xffffcfc7);
        put(fields.bytes, PARENT, 0xffffcfc7);
    }

    // 0045c1b0: genus 검사(진단 0x904, 정상 복귀 시 계속)→첫 nondead 비행 Form 조회→dirty 2·홈 WORD 쓰기이다. Form 이 없으면 자연 반환한다. (2026-11-13 WB04-E352)
    bind(flyer, home) {
        const type = this.#pool.slot(flyer)[10];
        if (type >= this.#types.length) throw new RangeError('비행 홈 연결 TYPE 범위 오류');
        if ((this.#types[type].flags2 & FLYER_GENUS) === 0) need(this.#hooks.assertion)(0x904);
        const form = need(this.#hooks.findProcess)(flyer, this.#globals.processType());
        if (!form) return;
        const fields = need(this.#hooks.fields)(form);
        fields.bytes[DIRTY] |= 2;
        put(fields.bytes, HOME, home, 2);
    }

    // 0045c100: genus 진단(0x8ee)은 무조건 선택되며 dead/free 면 0, Form 이 없으면 현재 checking 이 켜졌을 때 진단 0x8f5 를 낸 뒤 0, 있으면 같은 outer+a WORD 이다. (2026-11-13 WB04-E352)
    home(flyer) {
        const type = this.#pool.slot(flyer)[10];
        if (type >= this.#types.length) throw new RangeError('비행 홈 조회 TYPE 범위 오류');
        if ((this.#types[type].flags2 & FLYER_GENUS) === 0) need(this.#hooks.assertion)(0x8ee);
        if (this.#pool.slot(flyer)[11] & 3) return 0;
        // 실제 첫 타입 조회 결과는 진단 중 새 Form 이 생겨도 다시 찾지 않는다.
        const form = need(this.#hooks.findProcess)(flyer, this.#globals.processType());
        if (!form) {
            if (this.#checking()) need(this.#hooks.assertion)(0x8f5);
            return 0;
        }
        return need(this.#hooks.fields)(form).home;
    }

    // 아직 미복원인 큰 Frame 을 조용히 생략하지 않는다. (2026-11-13 WB04-E352)
    frame(fields) { need(this.#hooks.frame)(fields); }

    // 실제 선택 통지(0045b760 서버 표적 해제)를 유지한다. (2026-11-13 WB04-E352)
    notify(fields, message, info1, info2) { need(this.#hooks.notify)(fields, message, info1, info2); }

    // 홈의 부모 사망 통지(0045b7a0)를 유지한다. (2026-11-13 WB04-E352)
    parentDying(fields) { need(this.#hooks.parentDying)(fields); }

    // 현재 outer+c 의 세 FLD32 leaf 만 선택한다: 기본/Rain 1e7 초, Sun 60초, Wind 25초. 테마/자산 TYPE 로 다른 가상을 추정하지 않는다. (2026-11-13 WB04-E352)
    static returnDelay(fields) {
        switch (read(fields.bytes, 0)) {
        case BASE_TABLE: case RAIN_TABLE: return 10000000;
        case SUN_TABLE: return 60;
        case WIND_TABLE: return 25;
        default: throw new Error('비행 명령의 현재 outer+c 실제 leaf 미연결');
        }
    }

    // 45b320/3b0/430: 같은 scheduler 가 다음 자연 Frame 에서 읽을 현재 8바이트 기한만 쓴다. dirty/예약/즉시 Frame 은 추가하지 않는다. (2026-11-13 WB04-E352)
    command(fields, command) {
        const now = this.#globals.now();
        if (command === 1) {
            for (let index = 0; index < 8; index += 1) SCRATCH.setUint8(index, fields.bytes[104 + index]);
            const remaining = (SCRATCH.getFloat64(0, true) - now) + 15;
            // PE 의 비순서 비교도 60 을 선택하며 음수 하한은 없다.
            const limited = remaining < 60 ? remaining : 60;
            time(fields, 104, limited + now);
            return;
        }
        if (command === 2) {
            time(fields, 40, RawFlyerProcess.returnDelay(fields) + now);
            return;
        }
        // 3 은 원문의 00500460 double 0 이며 현재 시각/새 Regular 로 바꾸지 않는다.
        if (command === 3) time(fields, 40, 0);
    }

    // 45bc30: 서버가 아니면 진단 0x41d(정상 복귀 시 계속)→현재 파편→현재 부모의 가상 삭제 8 이다. 파편 효과 뒤 부모가 바뀌면 그 현재 부모를 다시 읽어 삭제한다. (2026-11-13 WB04-E352)
    finish(fields) {
        if (!this.#globals.server()) need(this.#hooks.assertion)(0x41d);
        need(this.#hooks.pieces)(fields.parent);
        need(this.#hooks.destroy)(fields.parent, 8);
    }

    // 0045c310→0045aed0: 기본 직렬화의 WORD/DWORD/8바이트 writer 이다. 추가 96/104 타이머는 이 패킷에 포함되지 않는다. packet 은 쓰기 전에 선택 필드 총 길이를 검사한다. 반환은 패킷 길이이다. (2026-11-13 WB04-E352)
    static writeProcess(fields, packet, includeParent, force = false, protocol = 1, message = 0x39) {
        const parent = (includeParent || force) ? read(fields.bytes, PARENT) : 0;
        const dirty = force ? 0xff : fields.bytes[DIRTY];
        let required = 9 + (parent ? 4 : 0);
        // 실제 선택 필드들의 총 길이를 쓰기 전에 검사한다(원문의 WORD 2·DWORD 3·8바이트 2 폭).
        for (let index = 0; index < DATA_WIDTHS.length; index += 1) if (dirty & (2 << index)) required += DATA_WIDTHS[index];
        if (packet.length < required) throw new RangeError('비행 프로세스 상태 패킷 크기 오류');
        if (force) fields.bytes[DIRTY] = 0xff;
        packet[0] = protocol;
        packet[1] = message;
        put(packet, 4, read(fields.bytes, FORM));
        let at = 9;
        if (parent) {
            fields.bytes[DIRTY] |= 1;
            put(packet, at, parent);
            at += 4;
        }
        // 같은 메타데이터의 바이트 복사로 NaN/미정 8바이트도 보존한다.
        for (let index = 0; index < DATA_OFFSETS.length; index += 1) {
            if (fields.bytes[DIRTY] & (2 << index)) {
                packet.set(fields.bytes.subarray(DATA_OFFSETS[index], DATA_OFFSETS[index] + DATA_WIDTHS[index]), at);
                at += DATA_WIDTHS[index];
            }
        }
        put(packet, 2, at, 2);
        packet[8] = fields.bytes[DIRTY];
        fields.bytes[DIRTY] = 0;
        return at;
    }
}

// 현재 필드의 단일 소유권을 실제 Kernel/Host 와 연결한다. (2026-11-13 WB04-E352)
export class FlyerProcess extends SquidProcess {
    #logic;
    #fields;

    // 생성 이전 호출자 바이트를 소유한다. (2026-11-13 WB04-E352)
    constructor(logic, fields) {
        super();
        this.#logic = logic;
        this.#fields = fields;
    }

    // 같은 Kernel 의 프레임을 실제 몸체로 전달한다. (2026-11-13 WB04-E352)
    runFrame() { this.#logic.frame(this.#fields); }

    // Host 의 현재 Form/부모를 원문 필드에 반영한다. (2026-11-13 WB04-E352)
    onAttached() {
        put(this.#fields.bytes, FORM, this.form);
        put(this.#fields.bytes, PARENT, this.parent);
    }

    // 표적 의존 통지의 실제 명시 경계이다(ProcessForm 가상 34: event, info, value). (2026-11-13 WB04-E352)
    notify(message, info1, info2) { this.#logic.notify(this.#fields, message, info1, info2); }

    // 현재 프로세스의 홈 통지를 전달한다. 원문은 전달된 부모 인자를 사용하지 않는다. (2026-11-13 WB04-E352)
    onParentDying() { this.#logic.parentDying(this.#fields); }

    // 현재 바이트를 사본 없이 공개한다. (2026-11-13 WB04-E352)
    get fields() { return this.#fields; }

    // 부착 이전 전송도 실제 Host Form/부모를 기록한 뒤 writer 를 부른다. (2026-11-13 WB04-E352)
    writePacket(packet, parentChanged, force = false, protocol = 1, message = 0x39) {
        this.onAttached();
        return RawFlyerProcess.writeProcess(this.#fields, packet, parentChanged, force, protocol, message);
    }
}

// 실제 맨 오 워 144/훨리기그 73 의 두 전용 PostPop 과 더스트 데빌, 공통 45e6b0 을 등록한다. (2026-11-13 WB04-E352)
export class RawFlyerPostPop {
    #pool;
    #types;
    #details;
    #server;
    #hooks;

    // pool 은 같은 PATCH 풀, types 는 188칸 TYPE(flags1), details 는 같은 번호의 { speed(float 비트), frames, impactSound, activeSound }, server 는 처리 중 바뀔 수 있는 현재 서버를 읽는 함수이다. (2026-11-13 WB04-E352)
    constructor(pool, types, details, server, hooks) {
        if (pool?.edition !== 'patch1078' || types.length !== 188 || details.length !== 188 || typeof server !== 'function') throw new TypeError('비행 후처리는 PATCH188 타입만 복원했습니다');
        this.#pool = pool;
        this.#types = types.map((record) => ({ ...record }));
        this.#details = details.map((record) => ({ ...record }));
        this.#server = server;
        this.#hooks = hooks;
    }

    // 공통 PostPop 과 같은 풀인지 확인할 수 있게 공개한다. (2026-11-13 WB04-E352)
    get pool() { return this.#pool; }

    // 실제 세 전용 가상 표에 한정하며 다른 비행체는 별도 복원이 필요하다. (2026-11-13 WB04-E352)
    handles(sid) {
        const table = read(this.#pool.slot(sid), 0);
        return table === MAN_TABLE || table === SUN_POST_TABLE || table === WIND_POST_TABLE;
    }

    // 같은 번호의 보충 자료를 꺼낸다(표 밖은 호스트 오류). (2026-11-13 WB04-E352)
    #detail(type) {
        const detail = this.#details[type];
        if (!detail) throw new RangeError('비행 후처리 보충 자료 TYPE 범위 오류');
        return detail;
    }

    // 전용 45fac0/45ef30 → 45e6b0 → Damageable 의 실제 순서이다. 0045e6b0 은 TYPE 포인터를 처음 snapshot 하고 효과 뒤에도 그 TYPE 의 현재 프레임 수를 쓴다. (2026-11-13 WB04-E352)
    postPop(sid, flags) {
        if (!this.handles(sid)) throw new TypeError('다른 비행 파생 PostPop 미복원');
        const pool = this.#pool;
        const hooks = this.#hooks;
        // 효과가 가상 표를 바꿔도 진입한 전용 몸체를 유지한다.
        const sun = read(pool.slot(sid), 0) === SUN_POST_TABLE;
        // 더스트 데빌은 생산기가 비행 프로세스를 따로 부착한다.
        const wind = read(pool.slot(sid), 0) === WIND_POST_TABLE;
        if (wind && (flags & 1) && !need(hooks.hasRegular)(sid, 0) && need(hooks.allocateWindRegular)()) need(hooks.createWindRegular)(sid, 0, 0.25);
        if (sun && (flags & 1)) {
            // 홈/확보/부착 전에 읽은 TYPE 보충 자료이다.
            const original = this.#detail(pool.slot(sid)[10]);
            if (this.#server()) {
                // 두 가상 공간 효과가 공유하는 최초 홈이다.
                const home = read(pool.slot(sid), 12, 2);
                need(hooks.unpop)(home, 0);
                put(pool.allocated(home), 36, 10);
                need(hooks.repop)(home, 0);
                if (need(hooks.allocateSunProcess)()) need(hooks.createSunProcess)(sid, read(pool.slot(sid), 12, 2), original.speed, Math.fround(0.6));
            }
            if (need(hooks.allocateSound)()) need(hooks.createSound)(sid, original.impactSound, original.activeSound, 1);
        }
        if (!sun && !wind && (flags & 1) && this.#server() && need(hooks.allocateRainProcess)()) {
            const home = read(pool.slot(sid), 12, 2);
            const speed = this.#detail(pool.slot(sid)[10]).speed;
            need(hooks.createRainProcess)(sid, home, speed, Math.fround(0.5));
        }
        // 프로세스 생성 효과가 끝난 현재 TYPE 이다.
        const type = pool.slot(sid)[10];
        const details = this.#detail(type);
        if (flags & 1) {
            if ((read(pool.slot(sid), 36) | 0) < details.frames && (this.#types[type].flags1 & SHADOW_FLAGS1) !== 0) {
                const shadow = need(hooks.create)(pool.slot(sid)[10], 2);
                const raw = pool.allocated(shadow);
                put(raw, 36, frameSum(details.frames, pool.slot(sid)));
                raw[40] |= 8;
                put(pool.allocated(sid), 8, shadow, 2);
                const parent = pool.slot(sid);
                need(hooks.pop)(shadow, floatOf(read(parent, 14)), floatOf(read(parent, 18)), 0);
            }
            need(hooks.initializeUnit)(sid);
        }
        const shadow = read(pool.slot(sid), 8, 2);
        if (shadow) {
            put(pool.allocated(shadow), 36, frameSum(details.frames, pool.slot(sid)));
            need(hooks.unpop)(shadow, 0);
            // 원문은 +c8 전에 현재 좌표 두 개를 snapshot 한다.
            const x = floatOf(read(pool.slot(sid), 14));
            const y = floatOf(read(pool.slot(sid), 18));
            const offset = need(hooks.shadowOffset)(sid);
            need(hooks.pop)(shadow, Math.fround(x + offset[0]), Math.fround(y + offset[1]), 0);
        }
        need(hooks.damageable)(sid, flags);
    }
}
