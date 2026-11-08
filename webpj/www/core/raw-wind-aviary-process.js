// 데빌메이커(윈드 아비어리)의 실제 80바이트 생산 상태를 현재 Kernel 과 같은 풀에 연결하는 모듈이다(cpppj `o/RawWindAviaryProcess.cpp/.h`, 10.78 전용: 0045edf0 후처리·45f0a0 프레임·0045b680 비행체 사망·0045c090 Form 삭제 + Kernel 프로세스 래퍼).
//  - findAttachedProcess(pool, parent, type): 부모의 종속 체인에서 첫 살아 있는(dead 비트 2 아님) 타입 Form 의 SID(없으면 0)를 찾는다. 다음 링크는 현재 후보를 검사한 뒤 읽고 손상된 무한 체인은 풀 크기로 제한한다(cpppj `RawSunAviaryProcess.cpp` 의 같은 helper 이며 이 단위에서 처음 옮김).
//  - new RawWindAviaryProcess(pool, globals, hooks): pool 은 같은 PATCH SidPool, globals = { fort(), friendlyFire()(55a490 현재 DWORD), server(), boss(), editor()(편집 차단 DWORD), checking()(개발자 진단 DWORD), now(), processType()·flyerType()·aviaryType()(59/138/139 의 현재 타입) } 는 처리 도중 바뀔 수 있어 각 원문 위치에서 다시 읽고,
//    hooks = { findProcess(parent, type)→SID, allocate()→WindAviaryProcessFields|null(실제 new 50 의 미정 바이트 또는 NULL), attach(fields|null, parent, type)(flags 8 로 같은 Kernel 부착), damageable(parent, flags)(전용 후처리 뒤 공통 Damageable), assertion(line)(Flyer.cpp 해당 행 진단), setFrame(sid, frame)(004acee0),
//    checkFrame(sid, frame, line)(checking 일 때만 현재 프레임 진단), create(type)→SID(004af530), setOwner(sid, owner)(새 더스트 데빌 가상 74), height(sid, frame)→float(SHP 헤더 +14), footY(sid)→float(TYPE+1d8 발자국 높이), pop(sid, x, y)(새 플라이어 가상 90 flags 0),
//    createFlyerProcess(sid, parent)(45e1c0 생성·부착과 전용 표 쓰기), bindFlyer(sid, home)(0045c1b0 홈 WORD 연결), circlePosition(parent, flyer, fields)→[x, y](004adea0·0041e020 과 13번 헤더), finish(fields)(상태 13 의 0041bf10), destroy(sid, flags)(45c090 임시 비행체 가상 10) } 는 도달한 위치에서만 요구한다.
//  - postPop(parent, flags): 0045edf0 — flags bit 0·(편집 fort 이면서 친군 사격이 아님이 아님)·서버이면 이미 부착된 타입 59 프로세스가 있을 때 진단 0x275 후 새 80바이트를 할당·초기화·부착하고, 그 뒤 항상 공통 Damageable 을 실행한다.
//  - frame(fields): 45f0a0 — 부모의 정지/편집 차단(+40 의 0x20, editor)이면 무동작, 상태 1(초기화)→2(생산 중: 0.3초 이내 프레임 순환·경과 4/6초 비행체 생성·성장·분리)→7→5(26초 대기)→1, 특별 부모(상태 11/12/13: 원주 위치에 비행체 생성·분리·종료) 상태 기계이며 진단은 checking 에서 상태 범위 밖을 거부한다.
//  - flyerDied(fields, refresh): 0045b680 — 서버만 상태 2 로 바꾸고 Form 갱신. formDestroy(fields): 0045c090 — boss·현재 DWORD 임시 포인터(주소 곱셈의 32비트 감김, 상위 bit31 별칭 포함)·dead 비트 2 검사 뒤 진단(비행체 타입 불일치)과 가상 10 삭제를 하며 진단 뒤에도 처음 구한 포인터를 쓴다.
//  - WindAviaryProcess: 원본 inner+38 의 실제 Kernel 소유권과 80바이트를 한 객체에 둔 `SquidProcess` 파생 래퍼이며, `runFrame`/`onAttached`(Form/부모를 inner 에 기록)/`onFormDestroy`/`notify`(빈 통지)/`fields`/`writePacket`(0x74 writer, force 면 전체 필드)을 제공한다.
// 10.72(CD)는 범위 밖이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E185: 데빌메이커 프로세스 이식으로 추가)
import { RawWindAviaryState } from './raw-wind-aviary-state.js';
import { SquidProcess } from './squid-process.js';

// 본문이 읽는 80바이트의 필드 위치(dirty·시각·단계·임시 플라이어·Form·부모·원주 개수/색인)이다. (2026-11-09 WB04-E185)
const DIRTY = 8;
const STAGE = 24;
const FLYER = 48;
const FORM = 60;
const PARENT = 64;
const CIRCLE_COUNT = 68;
const CIRCLE_INDEX = 72;
// 원본 단일 정밀 상수와 x87 중간 계산에 쓰는 double 상수이다. (2026-11-09 WB04-E185)
const FLOAT_POINT_THREE = Math.fround(0.3);
const FLOAT_POINT_ZERO_FIVE = Math.fround(0.05);

// 비정렬 DWORD 를 읽는다. (2026-11-09 WB04-E185)
function read(raw, at) {
    return (raw[at] | (raw[at + 1] << 8) | (raw[at + 2] << 16) | (raw[at + 3] << 24)) >>> 0;
}

// 지정한 폭만 쓰고 미정 padding 은 보존한다(수 또는 BigInt, BYTE/WORD/DWORD/DOUBLE 폭). (2026-11-09 WB04-E185)
function put(raw, at, value, width = 4) {
    let remaining = BigInt.asUintN(64, BigInt(value));
    for (let index = 0; index < width; index += 1) {
        raw[at + index] = Number(remaining & 0xffn);
        remaining >>= 8n;
    }
}

// raw double 을 계산 없이 읽고 쓴다(쓰기에는 dirty 를 추가하지 않음, Frame 의 예약 double 쓰기도 같은 규칙). (2026-11-09 WB04-E185)
function readTime(fields, at) {
    const view = new DataView(new ArrayBuffer(8));
    view.setUint32(0, read(fields.bytes, at), true);
    view.setUint32(4, read(fields.bytes, at + 4), true);
    return view.getFloat64(0, true);
}
function writeTime(fields, at, value) {
    const view = new DataView(new ArrayBuffer(8));
    view.setFloat64(0, value, true);
    put(fields.bytes, at, view.getBigUint64(0, true), 8);
}

// 상태/임시 플라이어의 각각 다른 변경 비트를 기록한다(상태 dirty 4, 플라이어 dirty 0x20). (2026-11-09 WB04-E185)
function setState(fields, value) {
    fields.bytes[DIRTY] |= 4;
    put(fields.bytes, STAGE, value);
}
function setFlyer(fields, value) {
    fields.bytes[DIRTY] |= 0x20;
    put(fields.bytes, FLYER, value);
}

// 현재 inner 부모/임시 플라이어의 하위 WORD 이다. (2026-11-09 WB04-E185)
const parentOf = (fields) => read(fields.bytes, PARENT) & 0xffff;
const flyerOf = (fields) => read(fields.bytes, FLYER) & 0xffff;

// 원문 float 위치를 읽는다. (2026-11-09 WB04-E185)
function readFloat(raw, at) {
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, read(raw, at), true);
    return view.getFloat32(0, true);
}

// 선택하지 않은 경계는 요구하지 않고 선택된 누락은 앞선 쓰기를 유지하며 거부한다. (2026-11-09 WB04-E185)
function need(hook) {
    if (typeof hook !== 'function') throw new Error('데빌메이커 실제 하위 효과 미연결');
    return hook;
}

// 부모의 첫 일치 순서를 유지하고 손상된 무한 체인은 풀 크기로 제한한다. (2026-11-09 WB04-E185)
export function findAttachedProcess(pool, parent, type) {
    const head = pool.slot(parent);
    let value = head[6] | (head[7] << 8);
    let visited = 0;
    // 다음 링크는 현재 후보를 검사한 뒤 읽는다.
    while (value) {
        visited += 1;
        if (value >= pool.capacity || visited >= pool.capacity) throw new Error('현재 프로세스 Form 체인 오류');
        const raw = pool.slot(value);
        if (raw[10] === type && !(raw[11] & 2)) return value;
        value = raw[4] | (raw[5] << 8);
    }
    return 0;
}

// 데빌메이커 생산/후처리 몸체이다. (2026-11-09 WB04-E185)
export class RawWindAviaryProcess {
    #pool;
    #globals;
    #hooks;

    // 같은 PATCH 풀과 현재 전역/하위 효과를 빌린다. 다른 판본 ABI 는 허용하지 않는다. (2026-11-09 WB04-E185)
    constructor(pool, globals, hooks) {
        if (pool?.edition !== 'patch1078') throw new TypeError('데빌메이커는 PATCH만 복원했습니다');
        for (const name of ['fort', 'friendlyFire', 'server', 'boss', 'editor', 'checking', 'now', 'processType', 'flyerType', 'aviaryType']) {
            if (typeof globals?.[name] !== 'function') throw new TypeError(`데빌메이커 현재 전역 조회기 누락: ${name}`);
        }
        this.#pool = pool;
        this.#globals = globals;
        this.#hooks = hooks ?? {};
    }

    // 0045edf0: 중복 조회 이후 현재 processType/server 를 생성자/부착 시점에 다시 읽는다. (2026-11-09 WB04-E185)
    postPop(parent, flags) {
        const g = this.#globals;
        const h = this.#hooks;
        if ((flags & 1) && !(g.fort() && !(g.friendlyFire() >>> 0)) && g.server()) {
            if (need(h.findProcess)(parent, g.processType() >>> 0)) need(h.assertion)(0x275);
            const fields = need(h.allocate)();
            if (fields) RawWindAviaryState.initialize(fields, g.server(), g.now());
            need(h.attach)(fields ?? null, parent, g.processType() >>> 0);
        }
        need(h.damageable)(parent, flags);
    }

    // 원문 45b680 에는 난수나 대기 시각 변경이 없다. (2026-11-09 WB04-E185)
    flyerDied(fields, refresh) {
        if (!this.#globals.server()) return;
        setState(fields, 2);
        need(refresh)(fields);
    }

    // 0045c090: 원본은 임시 DWORD 를 0 으로 지우거나 free 1 을 검사하지 않으며 진단 뒤에도 처음 구한 포인터를 사용한다. (2026-11-09 WB04-E185)
    formDestroy(fields) {
        const g = this.#globals;
        if (!g.boss()) return;
        // +48 은 WORD 가 아닌 현재 DWORD 이다.
        const stored = read(fields.bytes, FLYER);
        if (!stored) return;
        // 원본 32비트 주소 곱셈의 감김을 보존한다(상위 bit31 별칭도 같은 주소).
        const offset = Math.imul(stored, 50) >>> 0;
        if (offset < 50 || offset >= this.#pool.capacity * 50) return;
        // 손상된 비정렬 포인터의 임의 가상 호출은 지원하지 않는다.
        if (offset % 50) throw new Error('데빌메이커 임시 포인터가 슬롯 경계 밖입니다');
        const sid = offset / 50;
        const raw = this.#pool.slot(sid);
        if (raw[11] & 2) return;
        if (raw[10] !== (g.flyerType() >>> 0)) need(this.#hooks.assertion)(0x8e4);
        need(this.#hooks.destroy)(sid, 0);
    }

    // 45f0a0: 원문은 진입 부모를 보존하면서 생성/연결의 일부 인수만 현재 필드에서 다시 읽는다(부모 raw 는 살아 있는 뷰). (2026-11-09 WB04-E185)
    frame(f) {
        const g = this.#globals;
        const h = this.#hooks;
        const pool = this.#pool;
        const parent = parentOf(f);
        const raw = pool.physical(parent);
        if ((raw[40] & 0x20) || (g.editor() >>> 0)) return;
        if (read(f.bytes, STAGE) === 1 && raw[10] !== (g.aviaryType() >>> 0)) setState(f, 11);
        const stage = read(f.bytes, STAGE);
        const now = g.now();
        // 새 실제 플라이어는 직접 frame 13 을 쓰고 소유자를 먼저 지정한다.
        const create = () => {
            const sid = need(h.create)(g.flyerType() >>> 0);
            setFlyer(f, sid);
            put(pool.allocated(sid), 36, 13);
            need(h.setOwner)(sid, raw[34]);
            return sid;
        };
        switch (stage) {
        case 1:
            f.bytes[DIRTY] |= 2;
            writeTime(f, 16, now);
            writeTime(f, 16, now + 1.0);
            setState(f, 2);
            writeTime(f, 32, now);
            f.bytes[DIRTY] |= 0x30;
            writeTime(f, 40, now);
            put(f.bytes, FLYER, 0);
            return;
        case 2:
        case 7: {
            // 경과만 float 로 저장하고 x87 의 후속 곱/덧셈은 double 까지 유지한다.
            const elapsed = Math.fround(now - readTime(f, 40));
            let delay = 0;
            let current = stage;
            if (readTime(f, 32) !== 0 && readTime(f, 32) <= now) {
                const next = ((read(raw, 36) + 1) | 0) % 6;
                if (g.checking() >>> 0) need(h.checkFrame)(parent, next, 0x884);
                need(h.setFrame)(parent, next);
                current = read(f.bytes, STAGE);
                delay = FLOAT_POINT_THREE;
                if (current === 7 || elapsed < 2.0) {
                    const delta = current === 7 ? elapsed : elapsed - 2.0;
                    const scaled = delta * delta * FLOAT_POINT_ZERO_FIVE;
                    delay = scaled > 0 && scaled > FLOAT_POINT_THREE ? FLOAT_POINT_THREE : scaled <= 0 ? 0.0 : scaled;
                } else delay = 0;
                writeTime(f, 32, now + delay);
            }
            if (current === 7) {
                if (!(delay < FLOAT_POINT_THREE)) setState(f, 5);
                return;
            }
            if (read(f.bytes, FLYER) !== 0 && elapsed < 6.0) {
                // 004e49c0 은 카운터가 아닌 x87 의 0 방향 정수 절삭이다.
                const converted = Math.trunc((elapsed - 4.0) * -4.5) | 0;
                const frame = Math.min(Math.max((13 - converted) | 0, 13), 21);
                const sid = flyerOf(f);
                if (g.checking() >>> 0) need(h.checkFrame)(sid, frame, 0x8a6);
                need(h.setFrame)(sid, frame);
            }
            if ((read(f.bytes, FLYER) !== 0 || !g.boss()) && elapsed > 6.0) {
                if (g.server()) {
                    const sid = flyerOf(f);
                    need(h.createFlyerProcess)(sid, parentOf(f));
                    need(h.bindFlyer)(flyerOf(f), parentOf(f));
                    setFlyer(f, 0);
                }
                f.bytes[DIRTY] |= 0x10;
                writeTime(f, 40, now);
                setState(f, 7);
            }
            if (g.server() && elapsed > 4.0 && read(f.bytes, FLYER) === 0) {
                const sid = create();
                const actual = parentOf(f);
                const actualRaw = pool.slot(actual);
                if (!actualRaw[6] && !actualRaw[7]) need(h.assertion)(0xa4);
                put(pool.allocated(actual), 12, sid, 2);
                const x = readFloat(raw, 14);
                const y = readFloat(raw, 18);
                const height = need(h.height)(parent, read(raw, 36) | 0);
                const foot = need(h.footY)(parent);
                const py = Math.fround((height * 0.09090909361839294 - foot) + y);
                put(pool.allocated(sid), 12, parentOf(f), 2);
                need(h.pop)(sid, Math.fround(x - 1.399999976158142), py);
            }
            return;
        }
        case 5:
            if (now - readTime(f, 40) > 26.0) setState(f, 1);
            return;
        case 11:
            f.bytes[DIRTY] |= 2;
            writeTime(f, 16, now);
            setState(f, 12);
            setFlyer(f, 0);
            return;
        case 12:
            if (g.server() && read(f.bytes, FLYER) === 0) {
                const sid = create();
                const position = need(h.circlePosition)(parent, sid, f);
                put(pool.allocated(sid), 12, parentOf(f), 2);
                need(h.pop)(sid, position[0], position[1]);
            }
            if (read(f.bytes, FLYER) !== 0 || !g.boss()) {
                if (g.server()) {
                    need(h.createFlyerProcess)(flyerOf(f), parentOf(f));
                    setFlyer(f, 0);
                }
                setState(f, 13);
            }
            return;
        case 13:
            need(h.finish)(f);
            return;
        default:
            if ((g.checking() >>> 0) && (stage < 1 || stage > 13)) need(h.assertion)(0x8da);
        }
    }
}

// 원본 inner+38 의 실제 Kernel 소유권과 80바이트를 한 객체에 둔다. (2026-11-09 WB04-E185)
export class WindAviaryProcess extends SquidProcess {
    #logic;
    #fields;

    // 초기화한 80바이트를 소유한다. (2026-11-09 WB04-E185)
    constructor(logic, fields) {
        super();
        this.#logic = logic;
        this.#fields = fields;
    }

    // 같은 Kernel 프레임을 원문 상태 기계로 전달한다. (2026-11-09 WB04-E185)
    runFrame() { this.#logic.frame(this.#fields); }

    // 현재 Host 의 Form/부모를 inner 에 기록한다. (2026-11-09 WB04-E185)
    onAttached() {
        put(this.#fields.bytes, FORM, this.form);
        put(this.#fields.bytes, PARENT, this.parent);
    }

    // inner VT 509ddc+0c 이며 전달 flags 는 원본처럼 사용하지 않는다(Form 삭제 전에 임시 객체만 정리하고 프로세스 반납은 Host 에 맡김). (2026-11-09 WB04-E185)
    onFormDestroy() { this.#logic.formDestroy(this.#fields); }

    // 원본 ret12 의 빈 통지이다. (2026-11-09 WB04-E185)
    notify() {}

    // 수신/현재 상태를 사본 없이 제공한다. (2026-11-09 WB04-E185)
    get fields() { return this.#fields; }

    // 같은 Host 의 실제 0x74 writer 이다. 동기화 force 는 현재 전체 필드/부모를 기록하며 부착 전 전송도 같은 Host 필드를 사용한다. (2026-11-09 WB04-E185)
    writePacket(packet, parentChanged, force = false, protocol = 1, message = 0x74) {
        this.onAttached();
        return RawWindAviaryState.writeProcess(this.#fields, packet, parentChanged, force, protocol, message);
    }
}
