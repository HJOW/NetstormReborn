// 훨리베이스(선 아비어리) raw-sun-aviary-process.js(SunAviaryProcessFields·RawSunAviaryProcess·SunAviaryProcess)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: sun-aviary-x86.tsv 1,734행 — 생성자 C 18행(미정 바이트를 포함한 모든 쓰기), 후처리 P 98행(live/dead/free 후보와 개별 할당·부착 이미지), 프레임 F 594행(현재 효과 이후 읽기와 실제 AP 상태 경계),
//    상태 writer W 1,024행(전체 dirty/부모/force 와 미사용 버퍼)이 필드 48바이트·객체 250바이트·전역 20바이트·패킷 64바이트·호출 순서까지 같고, sun-aviary-notice-x86.tsv 128행(홈 가상 34 통지의
//    서버 관문·효과 뒤 현재 값 재조회·세 변이)이 호출 순서·홈 슬롯 raw·필드·전역까지 같다. cpppj SunAviaryProcessTests.cpp 의 AviaryScene 을 같은 순서로 옮겼다.
//  - cpppj 시험 이식: 선택 누락과 부분 쓰기 보존(SunAviary_RequiredBoundaries), NULL 프로세스 홈 통지의 공간 효과 순서(SunAviary_HomeNoticeNullAfterSpatialEffects).
//  - 웹 계약: Kernel 래퍼의 프레임/Form·부모 기록/writePacket, 상태 패킷의 쓰기 전 길이 검사, 프레임 상태별 효과.
// 10.72(CD)는 범위 밖이다. 실제 게임·브라우저 실행이 아니다. (2026-11-13 WB04-E353: 7-P1 RawSunAviaryProcess 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { Kernel } from '../www/core/kernel.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { SquidUnpop } from '../www/core/squid-unpop.js';
import { RawSquidDestroy } from '../www/core/squid-destroy.js';
import { SquidProcessHost, SquidProcessState } from '../www/core/squid-process.js';
import { typeConstructorAddress } from '../www/core/squid-constructors.js';
import { RawSunAviaryProcess, SunAviaryProcess, SunAviaryProcessFields, findAttachedProcess } from '../www/core/raw-sun-aviary-process.js';

// 원본 관측 행이다(첫 행은 열 이름). 없으면 해당 검사를 건너뛴다. (2026-11-13 WB04-E353)
const fixtures = path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures');
const rows = (() => {
    const all = readRows(path.join(fixtures, 'sun-aviary-x86.tsv'));
    return all ? all.slice(1) : null;
})();
const noticeRows = readRows(path.join(fixtures, 'sun-aviary-notice-x86.tsv'));

// 두 부모/플라이어/종속 Form 입력의 실제 SID 와 native 관찰의 외부 가상 표 기록값이다. (2026-11-13 WB04-E353)
const IDS = [50, 51, 60, 70, 71];
const INPUT_VTABLE = 0x1240f000;

// 예외가 나는지 본다. (2026-11-13 WB04-E353)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 바이트 배열의 소문자 16진 문자열이다. (2026-11-13 WB04-E353)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 원문과 같은 현재 입력만 준비하고 결과는 제품 몸체가 기록하게 하는 장면이다(cpppj AviaryScene). (2026-11-13 WB04-E353)
class AviaryScene {
    constructor(kind, input) {
        this.kind = kind;
        this.pool = new SidPool(24000, { server: false });
        this.fields = new SunAviaryProcessFields();
        this.server = true;
        this.checking = false;
        this.editor = 0;
        this.processType = 32;
        this.flyerType = 73;
        this.now = 100;
        this.mutation = 0;
        this.frameCalls = 0;
        this.allocated = true;
        this.trace = [];
        this.attached = null;
        // 대상 71까지 실제 슬롯을 확보한다.
        for (;;) {
            const sid = this.pool.allocate(2);
            if (sid === 71) break;
            if (!sid || sid > 71) throw new Error('아비어리 검사 SID 준비 오류');
        }
        // 원문과 같은 다섯 50바이트 입력을 준비한다.
        for (const value of IDS) {
            const raw = this.pool.allocated(value);
            raw.fill(0xa5);
            put(raw, 0, INPUT_VTABLE);
            put(raw, 4, value === 70 ? 71 : 0, 2);
            put(raw, 6, value === 50 || value === 51 ? 70 : 0, 2);
            put(raw, 8, 127, 2);
            raw[10] = value === 50 || value === 51 ? 72 : value === 60 ? 73 : value === 70 ? 32 : 31;
            raw[11] = 0;
            put(raw, 12, value === 50 ? 150 : value === 51 ? 151 : value === 60 ? 9 : 0, 2);
            put(raw, 14, floatToBits(value + 0.25));
            put(raw, 18, floatToBits(value + 0.5));
            raw[34] = value === 50 ? 1 : value === 51 ? 2 : 0;
            put(raw, 36, value === 51 ? 7 : value === 60 ? 42 : 0);
            raw[40] = 0;
        }
        this.fields.bytes.fill(0xa5);
        put(this.fields.bytes, 0, 0x509dd8);
        this.fields.bytes[8] = 0x10;
        this.timeAt(100);
        put(this.fields.bytes, 24, 1);
        put(this.fields.bytes, 32, 0x509db8);
        put(this.fields.bytes, 36, 77);
        put(this.fields.bytes, 40, 50);
        put(this.fields.bytes, 44, 0);
        if (kind === 'C') {
            this.server = Number(input[0]) !== 0;
            this.now = [0, 100.25, -5.25][Number(input[1])];
            this.fields.bytes.fill(Number(input[2]) & 0xff);
        } else if (kind === 'P') {
            this.server = Number(input[0]) !== 0;
            this.allocated = Number(input[2]) !== 0;
            this.mutation = Number(input[4]);
            const layout = Number(input[3]);
            if (!layout) put(this.pool.allocated(50), 6, 0, 2);
            if (layout === 2) {
                this.pool.allocated(70)[11] = 2;
                this.pool.allocated(71)[10] = 32;
            }
            if (layout === 3) this.pool.allocated(70)[11] = 1;
        } else if (kind === 'F') {
            this.server = Number(input[0]) !== 0;
            this.editor = Number(input[1]);
            this.pool.allocated(50)[40] = Number(input[2]) & 0xff;
            put(this.fields.bytes, 24, Number(input[3]));
            put(this.pool.allocated(50), 36, Number.parseInt(input[4], 10) >>> 0);
            put(this.fields.bytes, 44, Number(input[5]));
            this.timeAt([99, 100, 101][Number(input[6])]);
            this.mutation = Number(input[7]);
        } else if (kind === 'W') {
            this.fields.bytes[8] = Number(input[0]) & 0xff;
            this.timeAt(123.25);
            put(this.fields.bytes, 24, 7);
            put(this.fields.bytes, 44, 5);
        }
        this.body = new RawSunAviaryProcess(this.pool, this.globals(), this.hooks());
    }

    // 같은 현재 전역을 매번 읽는 함수 묶음이다. (2026-11-13 WB04-E353)
    globals() {
        return { server: () => this.server, editor: () => this.editor, checking: () => this.checking, now: () => this.now, processType: () => this.processType, flyerType: () => this.flyerType };
    }

    // raw double 을 조립한다. (2026-11-13 WB04-E353)
    timeAt(value) {
        const view = new DataView(new ArrayBuffer(8));
        view.setFloat64(0, value, true);
        put(this.fields.bytes, 16, view.getUint32(0, true));
        put(this.fields.bytes, 20, view.getUint32(4, true));
    }

    // 실제 선택된 호출 순서이다. (2026-11-13 WB04-E353)
    event(value) { this.trace.push(value); }

    // 반환값/변경 입력만 공급하고 제품의 다음 상태나 최종 패킷을 계산하지 않는다. (2026-11-13 WB04-E353)
    hooks() {
        return {
            findProcess: (sid, type) => {
                this.event(`Find,${sid},${type}`);
                if (this.kind === 'P' && this.mutation === 1) this.server = false;
                if (this.kind === 'P' && this.mutation === 2) this.processType = 59;
                return findAttachedProcess(this.pool, sid, type);
            },
            allocate: (size) => {
                this.event(`Allocate,${size}`);
                if (!this.allocated) return null;
                const value = new SunAviaryProcessFields();
                value.bytes.fill(0xa5);
                return value;
            },
            attach: (value, sid, type, flags) => {
                this.event(`Attach,${type},${sid},${flags},${value ? '1' : '0'}`);
                this.attached = value;
            },
            damageable: (sid, flags) => { this.event(`Base,${flags}`); },
            assertion: (line) => { this.event(`Assert,${line}`); },
            setFrame: (sid, frame) => {
                this.event(`Frame,${sid},${frame}`);
                put(this.pool.allocated(sid), 36, frame >>> 0);
                this.frameCalls += 1;
                if (this.frameCalls === 1 && this.mutation === 1) {
                    put(this.fields.bytes, 40, 51);
                    this.now = 200;
                }
                if (this.frameCalls === 1 && this.mutation === 5) put(this.fields.bytes, 24, 9);
            },
            sound: (x, y, name) => {
                assert.equal(name, 'apStart.wav');
                this.event(`Sound,${floatToBits(x)},${floatToBits(y)}`);
                if (this.mutation === 2) {
                    put(this.fields.bytes, 24, 9);
                    put(this.fields.bytes, 44, 10);
                }
            },
            checkFrame: (sid, frame, line) => { this.event(`Check,${frame},${line}`); },
            create: (type, flags) => { this.event(`Create,${type},${flags}`); return 60; },
            setOwner: (sid, owner) => {
                this.event(`Owner,${sid},${owner}`);
                this.pool.allocated(sid)[34] = owner & 0xff;
                if (this.mutation === 3) {
                    put(this.fields.bytes, 40, 51);
                    this.now = 200;
                }
            },
            pop: (sid, x, y, flags) => {
                this.event(`Pop,${sid},${floatToBits(x)},${floatToBits(y)},${flags}`);
                if (this.mutation === 4) {
                    put(this.fields.bytes, 40, 51);
                    this.now = 200;
                }
            },
            bindFlyer: (flyer, sid) => { this.event(`Bind,${flyer},${sid}`); },
            refreshForm: () => {
                this.event('Refresh');
                if (this.mutation === 6) this.fields.bytes[8] = 0;
            },
        };
    }

    // 다섯 전체 기록을 원문 순서대로 읽는다. (2026-11-13 WB04-E353)
    objects() { return IDS.map((sid) => hex(this.pool.slot(sid))).join(''); }

    // 현재 전역 20바이트이다. (2026-11-13 WB04-E353)
    currentGlobals() {
        const raw = new Uint8Array(20);
        const view = new DataView(new ArrayBuffer(8));
        view.setFloat64(0, this.now, true);
        put(raw, 0, this.server ? 1 : 0);
        put(raw, 4, this.editor);
        put(raw, 8, view.getUint32(0, true));
        put(raw, 12, view.getUint32(4, true));
        put(raw, 16, this.processType);
        return hex(raw);
    }
}

// 범위를 나누어 한 실패가 다른 원문 검사 행을 감추지 않게 한다. (2026-11-13 WB04-E353)
function compareScope(kind, expected) {
    let compared = 0;
    // TSV 헤더 이외의 해당 원문 입력만 독립적으로 재생한다.
    for (const row of rows) {
        assert.equal(row.length, 8);
        if (row[0] !== kind) continue;
        compared += 1;
        const input = row[1].split(',');
        const scene = new AviaryScene(kind, input);
        let objects = '-';
        let packet = '-';
        let result = '-';
        if (kind === 'C') scene.body.initialize(scene.fields);
        else if (kind === 'P') scene.body.postPop(50, Number(input[1]));
        else if (kind === 'F') scene.body.frame(scene.fields);
        else {
            const bytes = new Uint8Array(64).fill(0xa7);
            result = String(RawSunAviaryProcess.writeProcess(scene.fields, bytes, Number(input[1]) !== 0, Number(input[2]) !== 0));
            packet = hex(bytes);
        }
        const image = kind === 'P' ? (scene.attached ? hex(scene.attached.bytes) : '-') : hex(scene.fields.bytes);
        if (kind === 'P' || kind === 'F') objects = scene.objects();
        const trace = scene.trace.length === 0 ? '-' : scene.trace.join(';');
        const where = `${kind}/${row[1]}`;
        assert.equal(image, row[2], `${where} 필드`);
        assert.equal(objects, row[3], `${where} 객체`);
        assert.equal(scene.currentGlobals(), row[4], `${where} 전역`);
        assert.equal(packet, row[5], `${where} 패킷`);
        assert.equal(result, row[6], `${where} 길이`);
        assert.equal(trace, row[7], `${where} 호출 순서`);
    }
    assert.equal(compared, expected);
}

test('SunAviary: 원본 10.78 생성자 18행 — 미정 바이트를 포함한 생성자의 모든 쓰기가 같다', { skip: !rows && '원본 fixture 없음' }, () => { compareScope('C', 18); });
test('SunAviary: 원본 10.78 후처리 98행 — live/dead/free 후보와 개별 할당이 같다', { skip: !rows && '원본 fixture 없음' }, () => { compareScope('P', 98); });
test('SunAviary: 원본 10.78 프레임 594행 — 현재 효과 이후 읽기와 실제 AP 상태 경계가 같다', { skip: !rows && '원본 fixture 없음' }, () => { compareScope('F', 594); });
test('SunAviary: 원본 10.78 상태 writer 1,024행 — 전체 dirty/부모/force 와 미사용 버퍼가 같다', { skip: !rows && '원본 fixture 없음' }, () => { compareScope('W', 1024); });

test('SunAviary: 원본 10.78 홈 통지 128행 — 서버 관문·효과 뒤 현재 값 재조회·세 변이가 호출 순서/홈 슬롯 raw/필드/전역까지 같다', { skip: !noticeRows && '원본 fixture 없음' }, () => {
    let compared = 0;
    // 서버 관문과 효과 뒤 현재 값 재조회를 각 입력에서 독립 구성한다.
    for (const row of noticeRows) {
        const input = row[0].split(',');
        const scene = new AviaryScene('N', input);
        scene.server = Number(input[0]) !== 0;
        scene.mutation = Number(input[3]);
        const raw = scene.pool.allocated(50);
        raw.fill(0xa5);
        put(raw, 0, 0x12200000);
        raw[10] = 72;
        raw[11] = 0;
        put(raw, 36, 7);
        scene.fields.bytes.fill(0xa5);
        scene.fields.bytes[8] = Number(input[1]) & 0xff;
        scene.timeAt(100.25);
        put(scene.fields.bytes, 24, Number(input[2]));
        put(scene.fields.bytes, 32, 0x509db8);
        put(scene.fields.bytes, 36, 70);
        put(scene.fields.bytes, 40, 50);
        // 큰 효과는 원문 관측과 같은 명시 입력이며 최종 상태를 계산하지 않는다.
        const hooks = {
            findProcess: (sid, type) => {
                assert.ok(sid === 50 && type === 32);
                scene.event('Find:50:32');
                if (scene.mutation === 3) {
                    scene.server = false;
                    scene.processType = 99;
                }
                return 70;
            },
            processFields: (sid) => { assert.equal(sid, 70); scene.event('Resolve:70'); return scene.fields; },
            unpop: (sid, flags) => {
                assert.ok(sid === 50 && flags === 0);
                scene.event('Unpop:50:0');
                raw[11] = 4;
                if (scene.mutation === 1) {
                    put(raw, 36, 99);
                    scene.fields.bytes[8] = 0x10;
                    put(scene.fields.bytes, 24, 5);
                    put(scene.fields.bytes, 40, 51);
                }
            },
            repop: (sid, flags) => {
                assert.ok(sid === 50 && flags === 0);
                scene.event(`Repop:50:0:${get(raw, 36)}`);
                raw[11] = 0;
                if (scene.mutation === 2) {
                    put(raw, 36, 77);
                    scene.fields.bytes[8] = 0x40;
                    put(scene.fields.bytes, 24, 9);
                }
            },
            refreshForm: (fields) => { assert.equal(fields, scene.fields); scene.event('Refresh'); },
        };
        new RawSunAviaryProcess(scene.pool, scene.globals(), hooks).flyerDied(50);
        assert.ok(row.length === 9 && row[1] === row[5] && row[2] === row[6] && row[3] === row[7] && row[4] === row[8]);
        assert.equal(scene.trace.length === 0 ? '-' : scene.trace.join(';'), row[1], `${row[0]} 호출 순서`);
        assert.equal(hex(raw), row[2], `${row[0]} 홈 슬롯`);
        assert.equal(hex(scene.fields.bytes), row[3], `${row[0]} 필드`);
        assert.equal(`${scene.server ? 1 : 0},${scene.processType}`, row[4], `${row[0]} 전역`);
        compared += 1;
    }
    assert.equal(compared, 128);
});

test('SunAviary: 원본 NULL 프로세스 역참조를 성공으로 숨기지 않으며 그 앞의 공간 효과 순서를 유지하고, 비서버는 아무것도 하지 않는다', () => {
    const scene = new AviaryScene('N', []);
    let effects = 0;
    const hooks = {
        findProcess: () => 0,
        unpop: () => { effects += 1; },
        repop: (sid) => { assert.equal(get(scene.pool.slot(sid), 36), 27); effects += 1; },
    };
    const body = new RawSunAviaryProcess(scene.pool, scene.globals(), hooks);
    assert.ok(throws(() => body.flyerDied(50)));
    assert.equal(effects, 2);
    assert.equal(get(scene.pool.slot(50), 36), 27);
    scene.server = false;
    effects = 0;
    new RawSunAviaryProcess(scene.pool, scene.globals(), {}).flyerDied(50);
    assert.equal(effects, 0);
});

test('SunAviary: 선택 누락은 앞선 쓰기를 남기고 거부하며 미선택 하위는 요구하지 않고 개발자 진단·짧은 패킷은 쓰기 전에 거부한다', () => {
    const scene = new AviaryScene('P', '1,1,1,0,0'.split(','));
    const hooks = scene.hooks();
    delete hooks.allocate;
    const missing = new RawSunAviaryProcess(scene.pool, scene.globals(), hooks);
    assert.ok(throws(() => missing.postPop(50, 1)));
    assert.ok(!scene.attached && scene.trace.join(';') === 'Find,50,32');
    scene.server = false;
    scene.trace.length = 0;
    missing.postPop(50, 1);
    assert.equal(scene.trace.join(';'), 'Base,1');
    const frame = new AviaryScene('F', '1,0,0,2,11,0,1,0'.split(','));
    frame.checking = true;
    const frameHooks = frame.hooks();
    delete frameHooks.checkFrame;
    const diagnostic = new RawSunAviaryProcess(frame.pool, frame.globals(), frameHooks);
    const before = Uint8Array.from(frame.fields.bytes);
    assert.ok(throws(() => diagnostic.frame(frame.fields)));
    assert.deepEqual(frame.fields.bytes, before);
    assert.equal(frame.trace.length, 0);
    assert.ok(throws(() => RawSunAviaryProcess.writeProcess(frame.fields, new Uint8Array(8), true)));
    assert.deepEqual(frame.fields.bytes, before);
});

test('SunAviaryProcess: Kernel 소유 래퍼가 프레임을 로직에 전달하고 부착 결과 Form/부모를 필드에 기록하며 writePacket 이 부착 전에도 현재 Host 번호를 쓴다', () => {
    const pool = new SidPool(24000, { server: true });
    const types = Array.from({ length: 188 }, (_, number) => ({ cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, ...([1, 2, 3, 6].includes(number) ? { constructorAddress: typeConstructorAddress(number) } : {}) }));
    const kernel = new Kernel(32);
    const host = new SquidProcessHost(pool, types, kernel, new RawSquidDestroy(pool, new SquidUnpop(pool, new SquidHash(), new Uint8Array(65536)), types), new SquidProcessState(), { asset: { selected: () => 0, emit: () => {} } });
    const calls = [];
    const logic = new RawSunAviaryProcess(pool, { server: () => true, editor: () => 0, checking: () => 0, now: () => 5, processType: () => 32, flyerType: () => 73 }, {
        setFrame: (sid, frame) => { calls.push(`frame:${sid},${frame}`); },
        sound: (x, y, name) => { calls.push(`sound:${name}`); },
    });
    const fields = new SunAviaryProcessFields();
    logic.initialize(fields);
    const process = new SunAviaryProcess(logic, fields);
    const packet = new Uint8Array(64);
    assert.equal(process.writePacket(packet, false, true), 9 + 8 + 4);
    assert.deepEqual([get(fields.bytes, 36), get(fields.bytes, 40)], [0, 0]);
    const parent = pool.allocate(2);
    pool.allocated(parent)[10] = 72;
    try {
        assert.equal(host.attach(process, 32, parent, 0x10), process);
        assert.deepEqual([get(fields.bytes, 36), get(fields.bytes, 40)], [process.form, parent]);
        kernel.runFrame();
        // 초기 상태 1: 홈 프레임 27 과 상태 8, apStart 소리.
        assert.deepEqual(calls, [`frame:${parent},27`, 'sound:apStart.wav']);
        assert.equal(fields.stage, 8);
        process.notify(1, 2, 3);
        assert.equal(process.fields, fields);
        assert.equal(fields.form, process.form);
        assert.equal(fields.parent, parent);
    } finally {
        kernel.clear();
    }
});

test('SunAviary 프레임: 상태별 효과 — 정지/편집 차단은 무동작, 상태 3/4/6/8 의 전환과 서버가 아닐 때 상태 6 생략', () => {
    // 정지(+40 의 0x20)·편집 차단은 모든 상태에서 무동작이다.
    for (const [guard, editor] of [[0x20, 0], [0, 1]]) {
        const scene = new AviaryScene('F', '1,0,0,1,5,0,1,0'.split(','));
        scene.pool.allocated(50)[40] = guard;
        scene.editor = editor;
        const before = Uint8Array.from(scene.fields.bytes);
        scene.body.frame(scene.fields);
        assert.deepEqual(scene.fields.bytes, before);
        assert.equal(scene.trace.length, 0);
    }
    // 상태 3 은 기한이 되면 4 로, 안 되면 유지한다. 상태 4 는 프레임 0 이면 5, 그 밖은 기한이 되면 프레임 −1.
    const wait = new AviaryScene('F', '1,0,0,3,5,0,2,0'.split(','));
    wait.body.frame(wait.fields);
    assert.equal(wait.fields.stage, 3);
    const ready = new AviaryScene('F', '1,0,0,3,5,0,0,0'.split(','));
    ready.body.frame(ready.fields);
    assert.equal(ready.fields.stage, 4);
    const lower = new AviaryScene('F', '1,0,0,4,5,0,1,0'.split(','));
    lower.body.frame(lower.fields);
    assert.deepEqual(lower.trace, ['Frame,50,4']);
    const bottom = new AviaryScene('F', '1,0,0,4,0,0,1,0'.split(','));
    bottom.body.frame(bottom.fields);
    assert.equal(bottom.fields.stage, 5);
    assert.equal(bottom.trace.length, 0);
    // 상태 6: 서버가 아니면 아무것도 하지 않고, 서버이면 플라이어를 만들어 연결하고 3 으로 간다.
    const client = new AviaryScene('F', '0,0,0,6,5,0,1,0'.split(','));
    client.body.frame(client.fields);
    assert.equal(client.trace.length, 0);
    const server = new AviaryScene('F', '1,0,0,6,5,0,1,0'.split(','));
    server.body.frame(server.fields);
    assert.deepEqual(server.trace.slice(0, 2), ['Create,73,0', 'Owner,60,1']);
    assert.equal(server.fields.stage, 3);
    assert.ok(server.trace.at(-1) === 'Refresh' && server.trace.includes('Bind,60,50'));
    // 범위 밖 상태는 개발자 진단이 켜졌을 때만 0x75e 를 낸다.
    for (const [checking, stage, diagnosed] of [[true, 0, true], [true, 11, true], [true, 7, false], [true, 10, false], [false, 11, false]]) {
        const scene = new AviaryScene('F', `1,0,0,${stage},5,0,1,0`.split(','));
        scene.checking = checking;
        scene.body.frame(scene.fields);
        assert.equal(scene.trace.includes('Assert,1886'), diagnosed, `상태 ${stage} checking ${checking}`);
    }
});

test('SunAviary: 생성자는 PATCH 풀만 받고 pool 접근자는 같은 풀을 돌려준다', () => {
    const scene = new AviaryScene('F', '1,0,0,1,5,0,1,0'.split(','));
    assert.equal(scene.body.pool, scene.pool);
    // 풀 판이 다르거나 풀이 없으면 안내 오류로 거부한다.
    assert.throws(() => new RawSunAviaryProcess({ edition: 'cd1072' }, scene.globals(), {}), TypeError);
    assert.throws(() => new RawSunAviaryProcess(null, scene.globals(), {}), TypeError);
});

test('SunAviary: 선택 누락과 NULL 프로세스 거부는 각각 원인을 알려 주는 오류 문구로 거부한다', () => {
    const scene = new AviaryScene('P', '1,1,1,0,0'.split(','));
    const hooks = scene.hooks();
    delete hooks.allocate;
    // 훅이 없다는 거부와 일반 호출 오류(TypeError)를 구별한다.
    assert.throws(() => new RawSunAviaryProcess(scene.pool, scene.globals(), hooks).postPop(50, 1), (error) => error.message === '훨리베이스 실제 하위 소비자 미연결');
    const home = new AviaryScene('N', []);
    const body = new RawSunAviaryProcess(home.pool, home.globals(), { findProcess: () => 0, unpop: () => {}, repop: () => {} });
    assert.throws(() => body.flyerDied(50), (error) => error.message === '훨리베이스 홈 통지의 현재 프로세스 NULL 원본 경계');
});

test('SunAviary: 홈 통지는 일반 연결에서 확보된 슬롯만 쓰고 Native 장기 연결은 free 물리 슬롯도 쓴다', () => {
    const pool = new SidPool(24000, { server: false });
    const fields = new SunAviaryProcessFields();
    const events = [];
    const unpop = (sid, flags) => { events.push(`unpop:${sid},${flags}`); };
    const repop = (sid, flags) => { events.push(`repop:${sid},${flags}`); };
    const refresh = (value) => { assert.equal(value, fields); events.push('refresh'); };
    // 한 번도 확보하지 않은 번호는 free 슬롯이라 일반 연결은 공간 해제 뒤 쓰기에서 거부한다.
    assert.throws(() => RawSunAviaryProcess.flyerDiedIn(pool, fields, 200, unpop, repop, refresh), /할당되지 않은 SID/);
    assert.deepEqual(events, ['unpop:200,0']);
    assert.equal(get(pool.slot(200), 36), 0);
    // Native 연결은 같은 순서로 물리 슬롯에 홈 프레임 27 을 쓰고 상태를 이어 간다.
    events.length = 0;
    RawSunAviaryProcess.flyerDiedIn(pool, fields, 200, unpop, repop, refresh, true);
    assert.deepEqual(events, ['unpop:200,0', 'repop:200,0', 'refresh']);
    assert.equal(get(pool.slot(200), 36), 27);
    assert.equal(fields.stage, 8);
    assert.equal(fields.dirty & 4, 4);
});

test('SunAviary 프레임: 첫 부모 포인터는 살아 있는 보기라 효과가 바꾼 좌표·소유자를 이후 읽기가 그대로 본다', () => {
    // 상태 6: 플라이어를 만드는 동안 부모 좌표가 (1.37, 2.5)·소유자가 3 으로 바뀌면 Pop 은 현재 값 + float 0.45 / −3 이다.
    const launch = new AviaryScene('F', '1,0,0,6,5,0,1,0'.split(','));
    const hooks = launch.hooks();
    const create = hooks.create;
    hooks.create = (type, flags) => {
        const sid = create(type, flags);
        put(launch.pool.allocated(50), 14, floatToBits(1.37));
        put(launch.pool.allocated(50), 18, floatToBits(2.5));
        launch.pool.allocated(50)[34] = 3;
        return sid;
    };
    new RawSunAviaryProcess(launch.pool, launch.globals(), hooks).frame(launch.fields);
    const x = floatToBits(Math.fround(Math.fround(1.37) + Math.fround(0.45)));
    assert.ok(launch.trace.includes('Owner,60,3'));
    assert.ok(launch.trace.includes(`Pop,60,${x},${floatToBits(-0.5)},0`));
    // 상태 1: 홈 프레임을 쓰는 동안 좌표가 바뀌면 소리는 현재 좌표에서 난다.
    const start = new AviaryScene('F', '1,0,0,1,5,0,1,0'.split(','));
    const startHooks = start.hooks();
    const setFrame = startHooks.setFrame;
    startHooks.setFrame = (sid, frame) => {
        setFrame(sid, frame);
        put(start.pool.allocated(50), 14, floatToBits(9.5));
        put(start.pool.allocated(50), 18, floatToBits(7.25));
    };
    new RawSunAviaryProcess(start.pool, start.globals(), startHooks).frame(start.fields);
    assert.ok(start.trace.includes(`Sound,${floatToBits(9.5)},${floatToBits(7.25)}`));
});

test('SunAviary 프레임: 개발자 진단이 켜지면 프레임을 바꾸기 전에 새 프레임과 줄 번호(0x724 올림·0x751 내림)를 검사한다', () => {
    const up = new AviaryScene('F', '1,0,0,2,5,0,0,0'.split(','));
    up.checking = true;
    up.body.frame(up.fields);
    assert.deepEqual(up.trace, ['Check,6,1828', 'Frame,50,6']);
    const down = new AviaryScene('F', '1,0,0,4,5,0,0,0'.split(','));
    down.checking = true;
    down.body.frame(down.fields);
    assert.deepEqual(down.trace, ['Check,4,1873', 'Frame,50,4']);
});

test('SunAviaryProcessFields: form/parent 는 DWORD 의 하위 WORD 이며 writer 도 하위 WORD 값을 쓴다', () => {
    const fields = new SunAviaryProcessFields();
    put(fields.bytes, 36, 0xabcd0005);
    put(fields.bytes, 40, 0x12340032);
    assert.equal(fields.form, 5);
    assert.equal(fields.parent, 0x32);
    const packet = new Uint8Array(64).fill(0xa7);
    assert.equal(RawSunAviaryProcess.writeProcess(fields, packet, true, false), 13);
    assert.equal(get(packet, 4), 5);
    assert.equal(get(packet, 9), 0x32);
});

test('SunAviary 프레임: 상태 6 에서 부모의 종속 WORD(+6)가 0 이면 소유자 지정 뒤 진단 0xa4 를 내고 계속 연결한다', () => {
    const zero = new AviaryScene('F', '1,0,0,6,5,0,1,0'.split(','));
    put(zero.pool.allocated(50), 6, 0, 2);
    zero.body.frame(zero.fields);
    assert.deepEqual(zero.trace.slice(0, 3), ['Create,73,0', 'Owner,60,1', 'Assert,164']);
    assert.equal(get(zero.pool.slot(50), 12, 2), 60);
    assert.equal(zero.fields.stage, 3);
    // +6 이 이미 있으면 진단을 내지 않는다.
    const linked = new AviaryScene('F', '1,0,0,6,5,0,1,0'.split(','));
    linked.body.frame(linked.fields);
    assert.ok(!linked.trace.includes('Assert,164'));
});

test('SunAviary 상태 writer: 길이(9 + 부모 4 + 시각 8 + 상태 4)에 정확히 맞는 버퍼는 쓰고 한 바이트 모자라면 쓰기 전에 거부하며 force 는 모든 필드를 가정한다', () => {
    // [dirty, 부모 포함, force, 기대 총 길이] — 부모 입력은 필드의 현재 부모(50)이다.
    for (const [dirty, includeParent, force, length] of [[0, false, false, 9], [0, true, false, 13], [2, false, false, 17], [4, false, false, 13], [6, true, false, 25], [0, true, true, 25], [0, false, true, 25]]) {
        const scene = new AviaryScene('W', [String(dirty), '0', '0']);
        scene.fields.bytes[8] = dirty;
        const before = Uint8Array.from(scene.fields.bytes);
        // 한 바이트 모자라면 아무것도 바꾸지 않고 거부한다.
        assert.throws(() => RawSunAviaryProcess.writeProcess(scene.fields, new Uint8Array(length - 1), includeParent, force), RangeError, `dirty ${dirty} 부모 ${includeParent} force ${force}`);
        assert.deepEqual(scene.fields.bytes, before, `dirty ${dirty} 부모 ${includeParent} force ${force} 거부 뒤 필드`);
        // 정확히 맞으면 쓰고 반환 길이가 총 길이이며 필드 dirty 를 비운다.
        assert.equal(RawSunAviaryProcess.writeProcess(scene.fields, new Uint8Array(length), includeParent, force), length, `dirty ${dirty} 부모 ${includeParent} force ${force}`);
        assert.equal(scene.fields.dirty, 0);
    }
});

test('SunAviary 상태 writer: protocol/message 는 패킷 머리 첫 두 바이트에 그대로 쓰이고 기본은 1/0x38 이다', () => {
    const scene = new AviaryScene('W', ['2', '0', '0']);
    scene.fields.bytes[8] = 2;
    const custom = new Uint8Array(64).fill(0xa7);
    RawSunAviaryProcess.writeProcess(scene.fields, custom, false, false, 2, 0x40);
    assert.deepEqual([custom[0], custom[1]], [2, 0x40]);
    scene.fields.bytes[8] = 2;
    const normal = new Uint8Array(64).fill(0xa7);
    RawSunAviaryProcess.writeProcess(scene.fields, normal, false, false);
    assert.deepEqual([normal[0], normal[1]], [1, 0x38]);
});

test('SunAviaryProcess: writePacket 은 부착 뒤 바뀐 필드 번호를 Host 의 현재 번호로 되돌리고 protocol/message 를 그대로 전달한다', () => {
    const pool = new SidPool(24000, { server: true });
    const types = Array.from({ length: 188 }, (_, number) => ({ cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, ...([1, 2, 3, 6].includes(number) ? { constructorAddress: typeConstructorAddress(number) } : {}) }));
    const kernel = new Kernel(32);
    const host = new SquidProcessHost(pool, types, kernel, new RawSquidDestroy(pool, new SquidUnpop(pool, new SquidHash(), new Uint8Array(65536)), types), new SquidProcessState(), { asset: { selected: () => 0, emit: () => {} } });
    const logic = new RawSunAviaryProcess(pool, { server: () => true, editor: () => 0, checking: () => 0, now: () => 5, processType: () => 32, flyerType: () => 73 }, {});
    const fields = new SunAviaryProcessFields();
    logic.initialize(fields);
    const process = new SunAviaryProcess(logic, fields);
    const parent = pool.allocate(2);
    pool.allocated(parent)[10] = 72;
    try {
        host.attach(process, 32, parent, 0x10);
        // 부착 뒤 필드 번호가 어긋나도 writer 는 Host 의 현재 Form/부모를 먼저 반영한다.
        put(fields.bytes, 36, 999);
        put(fields.bytes, 40, 998);
        const packet = new Uint8Array(64).fill(0xa7);
        process.writePacket(packet, false, false, 2, 0x40);
        assert.deepEqual([get(fields.bytes, 36), get(fields.bytes, 40)], [process.form, parent]);
        assert.deepEqual([packet[0], packet[1]], [2, 0x40]);
        assert.equal(get(packet, 4), process.form);
    } finally {
        kernel.clear();
    }
});
