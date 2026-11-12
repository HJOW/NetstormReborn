// 공통 비행 프로세스 raw-flyer-process.js(FlyerProcessFields·RawFlyerProcess·FlyerProcess·RawFlyerPostPop·specializeWind)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: flyer-postpop-x86.tsv 1,421행 — 생성자 C 38행(기본 표 부착 이전의 모든 쓰기와 padding/현재 전역), 후처리 P 329행(맨 오 워 확장·그림자 생성/재배치·공통 단위 초기화의 호출 순서와 실제 객체 250바이트),
//    홈 연결 B 30행(첫 nondead Form 조회와 WORD 홈/dirty 쓰기), 상태 writer W 1,024행(모든 dirty/부모/force 의 전체 64바이트 출력과 반환 길이)이 필드 112바이트·객체·전역 16바이트·부착 직전 이미지·호출 순서까지 같다.
//    cpppj FlyerPostPopTests.cpp 의 FlyerScene(실제 풀 위에서 부착/조회/큰 효과의 인수와 명시 변경 입력만 공급)을 같은 순서로 옮겼다.
//  - cpppj 시험 이식: 선택 누락과 원문 앞선 쓰기, 짧은 패킷의 사전 거부(Flyer_RequiredBoundaries).
//  - 웹 계약: 수신(readData/initializeReceived/destructFields)·returnDelay·command·finish·홈 조회·FlyerProcess 래퍼·specialize 계열·후처리 분기(더스트 데빌/훨리기그).
// 10.72(CD)는 범위 밖이다. 실제 게임·브라우저 실행이 아니다. (2026-11-13 WB04-E352: 7-P1 RawFlyerProcess 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get, bitsToFloat, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { Kernel } from '../www/core/kernel.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { SquidUnpop } from '../www/core/squid-unpop.js';
import { RawSquidDestroy } from '../www/core/squid-destroy.js';
import { SquidProcessHost, SquidProcessState } from '../www/core/squid-process.js';
import { typeConstructorAddress } from '../www/core/squid-constructors.js';
import { findAttachedProcess } from '../www/core/raw-wind-aviary-process.js';
import { FlyerProcess, FlyerProcessFields, RawFlyerPostPop, RawFlyerProcess, specializeWind } from '../www/core/raw-flyer-process.js';

// 원본 관측 행이다(첫 행은 열 이름). 없으면 해당 검사를 건너뛴다. (2026-11-13 WB04-E352)
const rows = (() => {
    const all = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'flyer-postpop-x86.tsv'));
    return all ? all.slice(1) : null;
})();

// 부모/기존 그림자/새 그림자/종속 둘의 실제 입력 SID 이다. (2026-11-13 WB04-E352)
const IDS = [50, 60, 61, 70, 71];

// 예외가 나는지 본다. (2026-11-13 WB04-E352)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 바이트 배열의 소문자 16진 문자열이다. (2026-11-13 WB04-E352)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 원문과 같은 현재 입력을 설치하고 효과의 인수/명시 변경 입력만 기록하는 장면이다(cpppj FlyerScene). (2026-11-13 WB04-E352)
class FlyerScene {
    constructor(kind, input) {
        this.kind = kind;
        this.pool = new SidPool(24000, { server: false });
        this.types = Array.from({ length: 188 }, () => ({ cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0 }));
        this.details = Array.from({ length: 188 }, () => ({ speed: 0, frames: 0, impactSound: 0, activeSound: 0 }));
        this.fields = new FlyerProcessFields();
        this.server = true;
        this.allocated = true;
        this.now = 100.25;
        this.processType = 31;
        this.mutation = 0;
        this.popCount = 0;
        this.trace = [];
        this.attachImage = '-';
        // 관찰 입력의 71번까지 실제 확보한다.
        for (;;) {
            const sid = this.pool.allocate(2);
            if (sid === 71) break;
            if (!sid || sid > 71) throw new Error('비행 검사 SID 준비 오류');
        }
        for (const value of IDS) {
            const raw = this.pool.allocated(value);
            raw.fill(0xa5);
            put(raw, 0, 0x50a388);
            put(raw, 4, value === 70 ? 71 : 0, 2);
            put(raw, 6, value === 50 ? 70 : 0, 2);
            put(raw, 8, 0, 2);
            raw[10] = value < 70 ? 144 : 31;
            raw[11] = 0;
            put(raw, 12, 17, 2);
            put(raw, 14, floatToBits(50.25));
            put(raw, 18, floatToBits(50.5));
            put(raw, 36, 2);
            raw[40] = 0;
        }
        // 실제 해당 TYPE 의 현재 입력 필드다.
        for (const type of [31, 143, 144]) {
            this.types[type].flags1 = 0x400000;
            this.types[type].flags2 = 0x100000;
            this.details[type] = { speed: floatToBits(type === 143 ? 1.25 : 0.75), frames: 12, impactSound: 0, activeSound: 0 };
        }
        this.fields.bytes.fill(0xa5);
        this.fields.bytes[8] = 0x10;
        put(this.fields.bytes, 52, 77);
        put(this.fields.bytes, 56, 50);
        if (kind === 'C') {
            this.server = Number(input[0]) !== 0;
            this.fields.bytes.fill(Number(input[4]) & 0xff);
            this.mutation = Number(input[5]);
        } else if (kind === 'P') {
            this.server = Number(input[0]) !== 0;
            this.allocated = Number(input[2]) !== 0;
            put(this.pool.allocated(50), 36, Number(input[3]));
            this.details[144].frames = Number(input[4]) | 0;
            this.types[144].flags1 = Number(input[5]) ? 0x400000 : 0;
            put(this.pool.allocated(50), 8, Number(input[6]) ? 60 : 0, 2);
            this.mutation = Number(input[7]);
            this.fields.bytes.fill(0xa5);
        } else if (kind === 'B') {
            this.types[144].flags2 = Number(input[0]);
            const layout = Number(input[1]);
            if (!layout) put(this.pool.allocated(50), 6, 0, 2);
            if (layout === 2) this.pool.allocated(70)[11] = 2;
            if (layout === 3) this.pool.allocated(70)[11] = 1;
            if (layout === 4) {
                this.pool.allocated(70)[10] = 32;
                this.pool.allocated(71)[11] = 2;
            }
        } else if (kind === 'W') {
            this.fields.bytes[8] = Number(input[0]);
            put(this.fields.bytes, 10, 65534, 2);
            put(this.fields.bytes, 12, 65533, 2);
            put(this.fields.bytes, 16, 0x3f400000);
            put(this.fields.bytes, 20, 0x3f000000);
            put(this.fields.bytes, 24, 0xffffffff);
            this.timeAt(32, 123.25);
            this.timeAt(40, -5.75);
        }
        this.logic = new RawFlyerProcess(this.pool, this.types, { server: () => this.server, now: () => this.now, processType: () => this.processType }, this.processHooks());
        this.post = new RawFlyerPostPop(this.pool, this.types, this.details, () => this.server, this.postHooks());
    }

    // 실제 double 입력만 쓴다. (2026-11-13 WB04-E352)
    timeAt(at, value) {
        const view = new DataView(new ArrayBuffer(8));
        view.setFloat64(0, value, true);
        put(this.fields.bytes, at, view.getUint32(0, true));
        put(this.fields.bytes, at + 4, view.getUint32(4, true));
    }

    // 현재 호출을 순서대로 연결한다. (2026-11-13 WB04-E352)
    event(value) { this.trace.push(value); }

    // 결과를 만들지 않고 원문과 같은 외부 부착/조회 입력을 공급한다. (2026-11-13 WB04-E352)
    processHooks() {
        return {
            assertion: (line) => {
                this.event(`Assert,${line}`);
                if (this.mutation === 1) {
                    this.now = 200;
                    this.processType = 30;
                    this.fields.bytes[8] = 0x10;
                }
            },
            attach: (current, parent, type, flags) => {
                assert.equal(current, this.fields);
                this.attachImage = hex(current.bytes);
                this.event(`Attach,${type},${parent},${flags}`);
                if (this.mutation === 2) {
                    put(this.fields.bytes, 84, 77);
                    this.pool.allocated(50)[10] = 143;
                    put(this.pool.allocated(50), 12, 13, 2);
                    this.now = 200;
                }
            },
            findProcess: (parent, type) => findAttachedProcess(this.pool, parent, type),
            fields: (form) => { this.event(`Fields,${form}`); return this.fields; },
        };
    }

    // 시간/공간/할당 경계만 명시 관찰과 같은 인수/변경 입력으로 공급한다. (2026-11-13 WB04-E352)
    postHooks() {
        return {
            allocateRainProcess: () => {
                this.event('Allocate,112');
                if (this.mutation === 7) {
                    this.pool.allocated(50)[10] = 143;
                    put(this.pool.allocated(50), 12, 13, 2);
                }
                if (this.mutation === 8) {
                    this.server = false;
                    this.processType = 30;
                }
                return this.allocated;
            },
            createRainProcess: (parent, home, speed, parameter) => {
                this.logic.construct(this.fields, parent, home, speed, parameter);
                RawFlyerProcess.specializeRain(this.fields);
            },
            create: (type, flags) => {
                this.event(`Create,${type},${flags}`);
                if (this.mutation === 3) {
                    put(this.pool.allocated(50), 36, 7);
                    put(this.pool.allocated(50), 14, floatToBits(90.25));
                }
                return 61;
            },
            pop: (sid, x, y, flags) => {
                this.event(`Pop,${sid},${floatToBits(x)},${floatToBits(y)},${flags}`);
                this.popCount += 1;
                if (this.mutation === 4 && this.popCount === 1) {
                    put(this.pool.allocated(50), 36, 9);
                    put(this.pool.allocated(50), 18, floatToBits(70.25));
                }
            },
            unpop: (sid, flags) => {
                this.event(`Unpop,${sid},${flags}`);
                if (this.mutation === 6) {
                    put(this.pool.allocated(50), 14, floatToBits(100.25));
                    put(this.pool.allocated(50), 18, floatToBits(110.25));
                }
            },
            initializeUnit: (sid) => {
                const view = new DataView(new ArrayBuffer(8));
                view.setFloat64(0, this.now, true);
                this.event(`SetTime,${sid},${view.getUint32(0, true)},${view.getUint32(4, true)}`);
                if (this.mutation === 5) {
                    put(this.pool.allocated(50), 8, 60, 2);
                    put(this.pool.allocated(50), 36, 11);
                    put(this.pool.allocated(50), 14, floatToBits(80.25));
                }
            },
            shadowOffset: () => [0.5, 0.5],
            damageable: (sid, flags) => { this.event(`Base,${flags}`); },
        };
    }

    // 전체 객체 250바이트이다. (2026-11-13 WB04-E352)
    objects() { return IDS.map((sid) => hex(this.pool.slot(sid))).join(''); }

    // 현재 전역 16바이트이다. (2026-11-13 WB04-E352)
    globals() {
        const raw = new Uint8Array(16);
        const view = new DataView(new ArrayBuffer(8));
        view.setFloat64(0, this.now, true);
        put(raw, 0, this.server ? 1 : 0);
        put(raw, 4, view.getUint32(0, true));
        put(raw, 8, view.getUint32(4, true));
        put(raw, 12, this.processType);
        return hex(raw);
    }
}

// 실제 각 입력의 전체 필드/객체/현재 전역/패킷/부착 직전/호출 순서를 비교한다. (2026-11-13 WB04-E352)
function compareFlyer(kind, expected) {
    let compared = 0;
    // 헤더를 제외한 해당 원문 입력을 독립 재생한다.
    for (const row of rows) {
        assert.equal(row.length, 9);
        if (row[0] !== kind) continue;
        compared += 1;
        const input = row[1].split(',');
        const scene = new FlyerScene(kind, input);
        let packet = '-';
        let result = '-';
        if (kind === 'C') scene.logic.construct(scene.fields, 50, Number(input[1]), Number(input[2]), bitsToFloat(Number(input[3])));
        else if (kind === 'P') scene.post.postPop(50, Number(input[1]));
        else if (kind === 'B') scene.logic.bind(50, Number(input[2]));
        else {
            const raw = new Uint8Array(64).fill(0xa7);
            result = String(RawFlyerProcess.writeProcess(scene.fields, raw, Number(input[1]) !== 0, Number(input[2]) !== 0));
            packet = hex(raw);
        }
        const image = kind === 'P' && scene.attachImage === '-' ? '-' : hex(scene.fields.bytes);
        const trace = scene.trace.length === 0 ? '-' : scene.trace.join(';');
        const where = `${kind}/${row[1]}`;
        assert.equal(image, row[2], `${where} 필드`);
        assert.equal(scene.objects(), row[3], `${where} 객체`);
        assert.equal(scene.globals(), row[4], `${where} 전역`);
        assert.equal(packet, row[5], `${where} 패킷`);
        assert.equal(result, row[6], `${where} 길이`);
        assert.equal(scene.attachImage, row[7], `${where} 부착 직전`);
        assert.equal(trace, row[8], `${where} 호출 순서`);
    }
    assert.equal(compared, expected);
}

test('Flyer: 원본 10.78 생성자 38행 — 기본 표 부착 이전의 모든 쓰기와 padding/현재 전역이 같다', { skip: !rows && '원본 fixture 없음' }, () => { compareFlyer('C', 38); });
test('Flyer: 원본 10.78 후처리 329행 — 맨 오 워 확장과 그림자의 생성/재배치 순서가 같다', { skip: !rows && '원본 fixture 없음' }, () => { compareFlyer('P', 329); });
test('Flyer: 원본 10.78 홈 연결 30행 — 첫 nondead Form 조회와 WORD 홈/dirty 쓰기가 같다', { skip: !rows && '원본 fixture 없음' }, () => { compareFlyer('B', 30); });
test('Flyer: 원본 10.78 상태 writer 1,024행 — 모든 dirty/부모/force 의 전체 64바이트 출력이 같다', { skip: !rows && '원본 fixture 없음' }, () => { compareFlyer('W', 1024); });

test('Flyer: 선택 누락은 앞선 쓰기를 남기고 거부하며 짧은 패킷은 쓰기 전에 거부한다', () => {
    const scene = new FlyerScene('P', '1,1,1,2,12,1,1,0'.split(','));
    const hooks = scene.postHooks();
    delete hooks.create;
    const missing = new RawFlyerPostPop(scene.pool, scene.types, scene.details, () => scene.server, hooks);
    assert.ok(throws(() => missing.postPop(50, 1)));
    assert.notEqual(scene.attachImage, '-');
    assert.equal(get(scene.fields.bytes, 0), 0x509f60);
    assert.equal(scene.trace.join(';'), 'Allocate,112;Attach,31,50,8');
    // frame/notify/parentDying 훅이 없으면 조용히 생략하지 않는다.
    assert.ok(throws(() => scene.logic.frame(scene.fields)));
    assert.ok(throws(() => scene.logic.notify(scene.fields, 2, 0, 50)));
    assert.ok(throws(() => scene.logic.parentDying(scene.fields)));
    const before = Uint8Array.from(scene.fields.bytes);
    assert.ok(throws(() => RawFlyerProcess.writeProcess(scene.fields, new Uint8Array(8), true, true)));
    assert.deepEqual(scene.fields.bytes, before);
});

// 필드 이미지를 0xa5 padding 으로 채운 새 필드와 지정 가상 표를 만든다. (2026-11-13 WB04-E352)
function filled(table = 0x509cc8) {
    const fields = new FlyerProcessFields();
    fields.bytes.fill(0xa5);
    fields.sunTail.fill(0xa5);
    put(fields.bytes, 0, table);
    return fields;
}

// 로직과 전역 값 객체를 만든다(훅은 호출자가 준다). (2026-11-13 WB04-E352)
function logicOf({ server = true, now = 100.25, checking = 0, hooks = {} } = {}) {
    const pool = new SidPool(24000, { server: false });
    const types = Array.from({ length: 188 }, () => ({ flags1: 0, flags2: 0 }));
    const globals = { server: () => globals.serverValue, now: () => globals.nowValue, processType: () => 31, checking: () => globals.checkingValue, serverValue: server, nowValue: now, checkingValue: checking };
    return { pool, types, globals, logic: new RawFlyerProcess(pool, types, globals, hooks) };
}

// 필드의 8바이트 double 을 읽거나 쓴다. (2026-11-13 WB04-E352)
function doubleAt(fields, at) {
    const view = new DataView(new ArrayBuffer(8));
    for (let index = 0; index < 8; index += 1) view.setUint8(index, fields.bytes[at + index]);
    return view.getFloat64(0, true);
}
function writeDouble(fields, at, value) {
    const view = new DataView(new ArrayBuffer(8));
    view.setFloat64(0, value, true);
    for (let index = 0; index < 8; index += 1) fields.bytes[at + index] = view.getUint8(index);
}

test('Flyer: 수신용 기본 생성자는 theme 별 전용 표/타이머만 쓰고 padding·홈·위치·속도는 보존한다', () => {
    // [theme, 외부 VT, inner VT, 타이머 96 이 0, 타이머 104 가 0, 훨리기그 꼬리 앞 16바이트가 0]
    const cases = [[0, 0x509f98, 0x509f78, true, false, false], [1, 0x509f60, 0x509f40, true, true, false], [2, 0x509f24, 0x509f04, false, true, true], [7, 0x509f24, 0x509f04, false, true, true]];
    for (const [theme, outer, inner, timer96, timer104, tail] of cases) {
        const fields = filled(0);
        RawFlyerProcess.initializeReceived(fields, theme);
        assert.equal(get(fields.bytes, 0), outer, `theme ${theme}`);
        assert.equal(get(fields.bytes, 48), inner);
        assert.equal(fields.bytes[8], 0xff);
        // 기본 생성자가 지우는 칸: 시각 32/40/64/72, 부모/Form(0), 상태 12(WORD), 단계 84.
        for (const at of [32, 40, 64, 72]) assert.deepEqual(Array.from(fields.bytes.subarray(at, at + 8)), new Array(8).fill(0), `시각 ${at}`);
        assert.deepEqual([get(fields.bytes, 52), get(fields.bytes, 56), get(fields.bytes, 12, 2), get(fields.bytes, 84)], [0, 0, 0, 0]);
        // 쓰지 않는 칸은 원래 padding 을 보존한다(홈 10·속도 16·파라미터 20·상태 24).
        assert.deepEqual([get(fields.bytes, 10, 2), get(fields.bytes, 16), get(fields.bytes, 20), get(fields.bytes, 24)], [0xa5a5, 0xa5a5a5a5, 0xa5a5a5a5, 0xa5a5a5a5]);
        assert.equal(get(fields.bytes, 96) === 0 && get(fields.bytes, 100) === 0, timer96, `타이머 96 theme ${theme}`);
        assert.equal(get(fields.bytes, 104) === 0 && get(fields.bytes, 108) === 0, timer104, `타이머 104 theme ${theme}`);
        assert.deepEqual(Array.from(fields.sunTail.subarray(0, 16)), tail ? new Array(16).fill(0) : new Array(16).fill(0xa5));
        assert.deepEqual(Array.from(fields.sunTail.subarray(16)), new Array(8).fill(0xa5));
    }
    // 더스트 데빌 전용 쓰기는 독립 함수로도 같다.
    const wind = filled(0);
    specializeWind(wind);
    assert.deepEqual([get(wind.bytes, 0), get(wind.bytes, 48), get(wind.bytes, 84)], [0x509f98, 0x509f78, 0]);
});

test('Flyer: 수신 복사는 dirty 비트 순서대로 부모 DWORD·WORD·DWORD·8바이트를 복사하고 짧은 패킷에서도 앞선 쓰기를 남긴다', () => {
    const fields = filled();
    const packet = new Uint8Array(9 + 4 + 2 + 2 + 4 + 4 + 4 + 8 + 8).fill(0);
    packet[8] = 0xff;
    put(packet, 9, 0x11223344);
    put(packet, 13, 0xbeef, 2);
    put(packet, 15, 0xcafe, 2);
    for (let index = 17; index < packet.length; index += 1) packet[index] = index;
    assert.equal(RawFlyerProcess.readData(fields, packet), 0x11223344);
    assert.deepEqual([get(fields.bytes, 10, 2), get(fields.bytes, 12, 2)], [0xbeef, 0xcafe]);
    assert.deepEqual(Array.from(fields.bytes.subarray(16, 20)), [17, 18, 19, 20]);
    assert.deepEqual(Array.from(fields.bytes.subarray(40, 48)), Array.from(packet.subarray(packet.length - 8)));
    // 부모가 없고 일부 필드만 있는 패킷: dirty 0x06(홈·상태 WORD).
    const second = filled();
    const small = new Uint8Array(9 + 4);
    small[8] = 0x06;
    put(small, 9, 0x1234, 2);
    put(small, 11, 0x5678, 2);
    assert.equal(RawFlyerProcess.readData(second, small), 0);
    assert.deepEqual([get(second.bytes, 10, 2), get(second.bytes, 12, 2), get(second.bytes, 16)], [0x1234, 0x5678, 0xa5a5a5a5]);
    // 짧은 패킷: 첫 필드는 써지고 둘째에서 거부한다. 머리 9바이트 미만과 미등록 가상 표도 거부한다.
    const partial = filled();
    const cut = new Uint8Array(9 + 2 + 1);
    cut[8] = 0x06;
    put(cut, 9, 0x4242, 2);
    assert.throws(() => RawFlyerProcess.readData(partial, cut), /선택된 dirty 필드 부족/);
    assert.equal(get(partial.bytes, 10, 2), 0x4242);
    assert.throws(() => RawFlyerProcess.readData(filled(), new Uint8Array(8)), /9BYTE 머리 부족/);
    assert.throws(() => RawFlyerProcess.readData(filled(0x1234), small), /observer 미연결/);
    for (const table of [0x509cc8, 0x509f24, 0x509f60, 0x509f98]) assert.equal(RawFlyerProcess.readData(filled(table), new Uint8Array(9)), 0);
});

test('Flyer: 해제 표식은 inner VT 와 Form/부모 DWORD 전체를 −12345 로 둔다', () => {
    const fields = filled();
    RawFlyerProcess.destructFields(fields);
    assert.deepEqual([get(fields.bytes, 48), get(fields.bytes, 52), get(fields.bytes, 56)], [0x5019ec, 0xffffcfc7, 0xffffcfc7]);
    assert.equal(fields.form, 0xcfc7);
    assert.equal(fields.parent, 0xcfc7);
});

test('Flyer: returnDelay 는 현재 outer 가상 표로 1e7/60/25 초를 고르고 미등록 표는 거부한다', () => {
    for (const [table, delay] of [[0x509cc8, 10000000], [0x509f60, 10000000], [0x509f24, 60], [0x509f98, 25]]) assert.equal(RawFlyerProcess.returnDelay(filled(table)), delay);
    assert.throws(() => RawFlyerProcess.returnDelay(filled(0x1234)), /outer\+c 실제 leaf 미연결/);
});

test('Flyer: 명령 1 은 타이머 104 를 (남은 시간 + 15) 최대 60초로, 2 는 현재 leaf 지연 + now 로, 3 은 0 으로 쓰고 그 밖은 아무것도 바꾸지 않는다', () => {
    const { logic } = logicOf({ now: 100 });
    // 명령 1: 기한 130 → 남은 30 + 15 = 45 → 145. 기한 200 → 남은 100 + 15 → 60 으로 제한 → 160. 과거 기한 50 → −35 → 65(하한 없음). NaN 은 60.
    for (const [deadline, expected] of [[130, 145], [200, 160], [50, 65], [NaN, 160]]) {
        const fields = filled();
        writeDouble(fields, 104, deadline);
        logic.command(fields, 1);
        assert.equal(doubleAt(fields, 104), expected, String(deadline));
        // 다른 시각 칸은 건드리지 않는다.
        assert.deepEqual(Array.from(fields.bytes.subarray(40, 48)), new Array(8).fill(0xa5));
    }
    // 명령 2: 현재 leaf 의 지연 + now. Wind 는 25초, Sun 은 60초.
    const wind = filled(0x509f98);
    logic.command(wind, 2);
    assert.equal(doubleAt(wind, 40), 125);
    const sun = filled(0x509f24);
    logic.command(sun, 2);
    assert.equal(doubleAt(sun, 40), 160);
    assert.throws(() => logic.command(filled(0x1234), 2), /leaf 미연결/);
    // 명령 3: double 0. 그 밖의 값은 변화 없음.
    const three = filled();
    logic.command(three, 3);
    assert.equal(doubleAt(three, 40), 0);
    const other = filled();
    const before = Uint8Array.from(other.bytes);
    logic.command(other, 0);
    logic.command(other, 4);
    assert.deepEqual(other.bytes, before);
});

test('Flyer: 종료는 서버가 아니면 진단 0x41d 뒤(정상 복귀) 파편 → 부모 삭제 8 순서이고 파편 효과 뒤 바뀐 부모를 다시 읽는다', () => {
    const events = [];
    const fields = filled();
    put(fields.bytes, 56, 50);
    const { logic } = logicOf({
        server: false,
        hooks: {
            assertion: (line) => events.push(`assert:${line}`),
            pieces: (sid) => { events.push(`pieces:${sid}`); put(fields.bytes, 56, 61); },
            destroy: (sid, flags) => events.push(`destroy:${sid},${flags}`),
        },
    });
    logic.finish(fields);
    assert.deepEqual(events, ['assert:1053', 'pieces:50', 'destroy:61,8']);
    const server = logicOf({ server: true, hooks: { pieces: () => events.push('x'), destroy: () => events.push('y') } });
    events.length = 0;
    server.logic.finish(fields);
    assert.deepEqual(events, ['x', 'y']);
    assert.throws(() => logicOf({ server: true, hooks: {} }).logic.finish(fields), /하위 몸체 미연결/);
});

test('Flyer: 홈 조회/연결 — TYPE 범위·genus 진단·dead 상태·Form 없음의 checking 진단·누락 훅을 원문 순서로 처리한다', () => {
    const pool = new SidPool(24000, { server: false });
    for (let sid = 5; sid <= 60; sid += 1) pool.allocate(2);
    pool.allocated(50)[10] = 144;
    const types = Array.from({ length: 188 }, () => ({ flags1: 0, flags2: 0x100000 }));
    const state = { checking: 0, form: 0, fields: new FlyerProcessFields(), events: [] };
    state.fields.bytes.fill(0);
    put(state.fields.bytes, 10, 0x1357, 2);
    const globals = { server: () => true, now: () => 1, processType: () => 31, checking: () => state.checking };
    const hooks = {
        assertion: (line) => state.events.push(`assert:${line}`),
        findProcess: (sid, type) => { state.events.push(`find:${sid},${type}`); return state.form; },
        fields: (form) => { state.events.push(`fields:${form}`); return state.fields; },
    };
    const logic = new RawFlyerProcess(pool, types, globals, hooks);
    // Form 이 있으면 outer+a WORD 를 읽는다. genus 가 맞으면 진단이 없다.
    state.form = 77;
    assert.equal(logic.home(50), 0x1357);
    assert.deepEqual(state.events, ['find:50,31', 'fields:77']);
    // Form 이 없으면 checking 이 꺼졌을 때는 0, 켜졌을 때는 진단 0x8f5 뒤 0 이다.
    state.form = 0;
    state.events.length = 0;
    assert.equal(logic.home(50), 0);
    assert.deepEqual(state.events, ['find:50,31']);
    state.checking = 0x80000000;
    state.events.length = 0;
    assert.equal(logic.home(50), 0);
    assert.deepEqual(state.events, ['find:50,31', `assert:${0x8f5}`]);
    // dead/free 비행체는 조회 없이 0, genus 가 틀리면 진단 0x8ee 가 먼저다.
    pool.allocated(50)[11] = 2;
    state.events.length = 0;
    assert.equal(logic.home(50), 0);
    assert.deepEqual(state.events, []);
    types[144].flags2 = 0;
    assert.equal(logic.home(50), 0);
    assert.deepEqual(state.events, [`assert:${0x8ee}`]);
    pool.allocated(50)[11] = 0;
    types[144].flags2 = 0x100000;
    // 연결(bind): Form 이 없으면 자연 반환, 있으면 dirty 2 와 홈 WORD 를 쓴다. genus 가 틀리면 진단 0x904 뒤에도 계속한다.
    state.fields.bytes.fill(0);
    state.form = 0;
    state.events.length = 0;
    logic.bind(50, 0x2468);
    assert.deepEqual([state.fields.bytes[8], get(state.fields.bytes, 10, 2)], [0, 0]);
    state.form = 88;
    types[144].flags2 = 0;
    logic.bind(50, 0x2468);
    assert.ok(state.events.includes(`assert:${0x904}`));
    assert.deepEqual([state.fields.bytes[8], get(state.fields.bytes, 10, 2)], [2, 0x2468]);
    pool.allocated(50)[10] = 200;
    assert.throws(() => logic.bind(50, 1), RangeError);
    assert.throws(() => logic.home(50), RangeError);
    // 훅 누락: 선택된 위치에서 거부한다.
    const bare = new RawFlyerProcess(pool, types, globals, {});
    pool.allocated(50)[10] = 144;
    types[144].flags2 = 0x100000;
    assert.throws(() => bare.home(50), /하위 몸체 미연결/);
    assert.throws(() => bare.bind(50, 1), /하위 몸체 미연결/);
    assert.throws(() => new RawFlyerProcess(pool, types.slice(1), globals, hooks), TypeError);
    assert.throws(() => new RawFlyerProcess({ edition: 'cd1072' }, types, globals, hooks), TypeError);
});

test('FlyerProcess: Kernel 소유 래퍼가 프레임/통지/부모 사망을 로직에 전달하고 부착 결과 Form/부모를 필드에 기록한다', () => {
    const pool = new SidPool(24000, { server: true });
    const types = Array.from({ length: 188 }, (_, number) => ({ cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, ...([1, 2, 3, 6].includes(number) ? { constructorAddress: typeConstructorAddress(number) } : {}) }));
    const kernel = new Kernel(32);
    const clock = new SquidProcessState();
    const host = new SquidProcessHost(pool, types, kernel, new RawSquidDestroy(pool, new SquidUnpop(pool, new SquidHash(), new Uint8Array(65536)), types), clock, {
        asset: { selected: () => 0, emit: () => {} },
    });
    const events = [];
    const fields = new FlyerProcessFields();
    const logic = new RawFlyerProcess(pool, types, { server: () => true, now: () => 1, processType: () => 31 }, {
        frame: (given) => events.push(`frame:${given === fields}`),
        notify: (given, message, info1, info2) => events.push(`notify:${message},${info1},${info2}`),
        parentDying: () => events.push('dying'),
    });
    const process = new FlyerProcess(logic, fields);
    // 부착 전 writePacket 도 현재 Host Form/부모(0)를 기록한다.
    const packet = new Uint8Array(64);
    // force 는 부모 0 이면 부모 DWORD 를 쓰지 않고 데이터 필드 7개(2+2+4+4+4+8+8)만 쓴다: 9 + 32.
    assert.equal(process.writePacket(packet, false, true), 41);
    assert.deepEqual([get(fields.bytes, 52), get(fields.bytes, 56)], [0, 0]);
    const parent = pool.allocate(2);
    pool.allocated(parent)[10] = 144;
    try {
        const attached = host.attach(process, 31, parent, 0x10);
        assert.equal(attached, process);
        assert.deepEqual([get(fields.bytes, 52), get(fields.bytes, 56)], [process.form, parent]);
        kernel.runFrame();
        process.notify(2, 5, 6);
        process.onParentDying(parent);
        assert.deepEqual(events, ['frame:true', 'notify:2,5,6', 'dying']);
        assert.equal(fields.form, process.form);
        assert.equal(fields.parent, parent);
        assert.equal(process.fields, fields);
    } finally {
        kernel.clear();
    }
});

test('Flyer 후처리: 더스트 데빌·훨리기그 전용 분기 — Regular 중복 확인, 홈 재배치, 전용 프로세스, 소리와 서버가 아닐 때 생략', () => {
    // 전용 표 0x50a050(더스트 데빌)·0x50a1e8(훨리기그)는 서로 다른 최초 Pop 효과를 가진다.
    const make = ({ table, server = true, hasRegular = false, allocateWind = true }) => {
        const scene = new FlyerScene('B', ['0', '1', '0']);
        for (const sid of IDS) put(scene.pool.allocated(sid), 0, table);
        scene.server = server;
        const log = [];
        const hooks = {
            ...scene.postHooks(),
            hasRegular: (sid, event) => { log.push(`has:${sid},${event}`); return hasRegular; },
            allocateWindRegular: () => { log.push('allocWind'); return allocateWind; },
            createWindRegular: (sid, event, payload) => { log.push(`wind:${sid},${event},${payload}`); },
            allocateSunProcess: () => { log.push('allocSun'); return true; },
            createSunProcess: (sid, home, speed, parameter) => { log.push(`sun:${sid},${home},${speed},${floatToBits(parameter)}`); },
            repop: (sid, flags) => { log.push(`repop:${sid},${flags}`); },
            allocateSound: () => { log.push('allocSound'); return true; },
            createSound: (sid, impact, active, flags) => { log.push(`sound:${sid},${impact},${active},${flags}`); },
        };
        scene.details[144] = { speed: floatToBits(0.75), frames: 12, impactSound: 31, activeSound: 32 };
        scene.types[144].flags1 = 0;
        const post = new RawFlyerPostPop(scene.pool, scene.types, scene.details, () => scene.server, hooks);
        return { scene, post, log };
    };
    // 더스트 데빌: flags bit0 이면 event0 Regular 가 없고 확보가 되면 0.25 로 만든다. 이미 있거나 확보 실패면 만들지 않는다.
    for (const [hasRegular, allocate, created] of [[false, true, true], [true, true, false], [false, false, false]]) {
        const { scene, post, log } = make({ table: 0x50a050, hasRegular, allocateWind: allocate });
        post.postPop(50, 1);
        assert.equal(log.some((entry) => entry.startsWith('wind:')), created);
        assert.equal(log.includes('allocWind'), !hasRegular);
        assert.ok(scene.trace.includes('Base,1'));
    }
    const noBit = make({ table: 0x50a050 });
    noBit.post.postPop(50, 0);
    assert.deepEqual(noBit.log, []);
    // 훨리기그(서버): 홈(+12 WORD 17)을 Unpop → 프레임 10 → Repop 하고 전용 프로세스(0.6)와 소리를 만든다.
    const sun = make({ table: 0x50a1e8 });
    sun.post.postPop(50, 1);
    assert.deepEqual(sun.log, ['repop:17,0', 'allocSun', `sun:50,17,${floatToBits(0.75)},${floatToBits(Math.fround(0.6))}`, 'allocSound', 'sound:50,31,32,1']);
    assert.ok(sun.scene.trace.includes('Unpop,17,0'));
    assert.ok(sun.scene.trace.indexOf('Unpop,17,0') < sun.scene.trace.indexOf('Base,1'));
    assert.equal(get(sun.scene.pool.slot(17), 36), 10);
    // 서버가 아니면 홈 재배치와 전용 프로세스를 건너뛰고 소리만 요청한다.
    const client = make({ table: 0x50a1e8, server: false });
    client.post.postPop(50, 1);
    assert.deepEqual(client.log, ['allocSound', 'sound:50,31,32,1']);
    assert.ok(!client.scene.trace.includes('Unpop,17,0'));
    // 전용이 아닌 가상 표는 거부하고 handles 가 거짓이다.
    const other = make({ table: 0x1234 });
    assert.equal(other.post.handles(50), false);
    assert.throws(() => other.post.postPop(50, 1), TypeError);
    assert.throws(() => new RawFlyerPostPop(other.scene.pool, other.scene.types, other.scene.details, true, {}), TypeError);
    assert.throws(() => new RawFlyerPostPop(other.scene.pool, other.scene.types.slice(1), other.scene.details, () => true, {}), TypeError);
});

test('Flyer: free 상태(비트 1)의 비행체도 홈 조회 없이 0 이다', () => {
    const { pool, logic } = (() => {
        const pool = new SidPool(24000, { server: false });
        for (let sid = 5; sid <= 50; sid += 1) pool.allocate(2);
        pool.allocated(50)[10] = 144;
        const types = Array.from({ length: 188 }, () => ({ flags1: 0, flags2: 0x100000 }));
        const logic = new RawFlyerProcess(pool, types, { server: () => true, now: () => 1, processType: () => 31 }, { findProcess: () => assert.fail('조회하지 않는다') });
        return { pool, logic };
    })();
    for (const state of [1, 2, 3]) {
        pool.physical(50)[11] = state;
        assert.equal(logic.home(50), 0, `상태 ${state}`);
    }
});

test('Flyer: 상태 패킷은 force 가 모든 필드를 가정한 총 길이(부모 포함)를 쓰기 전에 검사하고 모자라면 필드를 바꾸지 않는다', () => {
    // force + 부모 50: 필요 길이 9 + 4 + 32 = 45. 44·41·20 바이트는 모두 쓰기 전에 거부하고 필드는 그대로여야 한다.
    for (const size of [44, 41, 20, 9]) {
        const fields = filled();
        fields.bytes[8] = 0x10;
        put(fields.bytes, 56, 50);
        const before = Uint8Array.from(fields.bytes);
        const packet = new Uint8Array(size);
        assert.throws(() => RawFlyerProcess.writeProcess(fields, packet, false, true), RangeError, `크기 ${size}`);
        assert.deepEqual(fields.bytes, before, `크기 ${size}`);
        assert.deepEqual(Array.from(packet), new Array(size).fill(0), `크기 ${size}`);
    }
    // 정확히 45바이트면 통과하고 부모 DWORD 와 force 의 전체 필드를 쓴다.
    const fields = filled();
    put(fields.bytes, 56, 50);
    const packet = new Uint8Array(45);
    assert.equal(RawFlyerProcess.writeProcess(fields, packet, false, true), 45);
    assert.equal(packet[8], 0xff);
    assert.equal(get(packet, 9), 50);
    // 부모를 포함하지 않는 일반 쓰기는 부모 DWORD 없이 dirty 필드만 요구한다(dirty 0x10 → 상태 DWORD 하나: 9 + 4).
    const plain = filled();
    plain.bytes[8] = 0x10;
    const small = new Uint8Array(13);
    assert.equal(RawFlyerProcess.writeProcess(plain, small, false), 13);
    assert.throws(() => RawFlyerProcess.writeProcess(filled(), new Uint8Array(12), true, false), RangeError);
});

test('FlyerProcess: writePacket 은 쓰기 전에 현재 Host Form/부모를 필드에 반영한다', () => {
    const pool = new SidPool(24000, { server: true });
    const types = Array.from({ length: 188 }, (_, number) => ({ cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, ...([1, 2, 3, 6].includes(number) ? { constructorAddress: typeConstructorAddress(number) } : {}) }));
    const kernel = new Kernel(32);
    const host = new SquidProcessHost(pool, types, kernel, new RawSquidDestroy(pool, new SquidUnpop(pool, new SquidHash(), new Uint8Array(65536)), types), new SquidProcessState(), { asset: { selected: () => 0, emit: () => {} } });
    const fields = new FlyerProcessFields();
    const process = new FlyerProcess(new RawFlyerProcess(pool, types, { server: () => true, now: () => 1, processType: () => 31 }, {}), fields);
    const parent = pool.allocate(2);
    pool.allocated(parent)[10] = 144;
    try {
        host.attach(process, 31, parent, 0x10);
        // 부착 뒤 필드가 다른 값으로 오염돼도 writePacket 이 다시 현재 Host 번호를 기록한다.
        put(fields.bytes, 52, 1);
        put(fields.bytes, 56, 2);
        const packet = new Uint8Array(64);
        process.writePacket(packet, false, false);
        assert.deepEqual([get(fields.bytes, 52), get(fields.bytes, 56), get(packet, 4)], [process.form, parent, process.form]);
    } finally {
        kernel.clear();
    }
});

test('Flyer 후처리: 더스트 데빌 Regular 는 event 0·페이로드 0.25 로 만들고 더스트 데빌은 맨 오 워 프로세스를 만들지 않으며, 그림자 프레임은 Unpop 전에 쓰고 그림자 좌표는 두 오프셋을 각각 더하고 입력 표의 사본을 쓴다', () => {
    const scene = new FlyerScene('B', ['0', '1', '0']);
    for (const sid of IDS) put(scene.pool.allocated(sid), 0, 0x50a050);
    const log = [];
    const hooks = { ...scene.postHooks(), hasRegular: () => false, allocateWindRegular: () => true, createWindRegular: (sid, event, payload) => log.push(`wind:${sid},${event},${payload}`) };
    new RawFlyerPostPop(scene.pool, scene.types, scene.details, () => scene.server, hooks).postPop(50, 1);
    assert.deepEqual(log, ['wind:50,0,0.25']);
    // 더스트 데빌은 Rain 프로세스 확보를 요청하지 않는다.
    assert.ok(!scene.trace.some((entry) => entry.startsWith('Allocate')));
    // 맨 오 워: 그림자(60)가 이미 있는 비행체에서 프레임을 Unpop 전에 쓰고 좌표에 비대칭 오프셋을 각각 더한다.
    const man = new FlyerScene('P', '1,0,1,2,12,0,1,0'.split(','));
    const frames = [];
    const popped = [];
    const hooks2 = {
        ...man.postHooks(),
        unpop: (sid, flags) => { frames.push(get(man.pool.slot(sid), 36)); man.event(`Unpop,${sid},${flags}`); },
        pop: (sid, x, y, flags) => { popped.push([x, y]); man.event(`Pop,${sid},${flags}`); },
        shadowOffset: () => [0.25, 0.75],
    };
    // 입력 표의 사본을 쓰므로 만든 뒤 원본을 바꿔도 영향이 없다.
    const typesCopy = man.types.map((record) => ({ ...record }));
    const detailsCopy = man.details.map((record) => ({ ...record }));
    const post = new RawFlyerPostPop(man.pool, typesCopy, detailsCopy, () => man.server, hooks2);
    typesCopy[144].flags1 = 0x400000;
    detailsCopy[144].frames = 99;
    post.postPop(50, 0);
    // 프레임 12 + 부모 프레임 2 = 14 가 Unpop 시점에 이미 그림자(60)에 쓰여 있다.
    assert.deepEqual(frames, [14]);
    assert.deepEqual(popped, [[Math.fround(50.25 + 0.25), Math.fround(50.5 + 0.75)]]);
    // 만든 뒤 입력 표를 바꿨어도 flags 1 에서의 그림자 생성 조건은 만든 시점의 표를 쓴다(원래 flags1 은 0 이라 새 그림자를 만들지 않는다).
    man.trace.length = 0;
    post.postPop(50, 1);
    assert.ok(!man.trace.some((entry) => entry.startsWith('Create')));
});
