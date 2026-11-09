// 내용물 미리보기 프로세스(raw-content-preview-process.js: RawContentPreviewProcess·ContentPreviewProcess)를 원본 10.78 관측과 대조한다(WB04-E241).
//  - content-preview-process-x86.tsv 957행: 생성(428700)·프레임(428770)·통지(480260)·소멸(468230→41bd00)의 24바이트와 효과 순서. 열 = 입력 9개(동작, 현재 시각 비트, 인자 float 비트, 저장된 만료 시각 비트, 부모, 현재 타입, 부모 head,
//    효과 변형, 동작 인자), 53비트 기본 CRT 정밀도의 24바이트·효과, 64비트 정밀도의 24바이트·효과. 웹은 JS double(53비트)이라 앞의 두 열(raw53·events53)만 재생하고 64비트 열은 별도 관측으로 보존한다(cpppj 와 같은 규칙).
//  - cpppj ContentPreviewProcessTests 를 옮긴다: 실제 Host 부착·자연 만료/외부 부모 삭제의 수명, 선택된 효과만 요구하는 계약과 판본 거부.
//  - 입력만 설치하고 결과는 모듈이 계산한다. 이전 행의 메모리가 섞이지 않도록 매 행 필드를 새로 만든다. 실제 게임·브라우저 실행이 아니다. (2026-11-10 WB04-E241: 내용물 미리보기 프로세스 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { SquidUnpop } from '../www/core/squid-unpop.js';
import { RawSquidDestroy, SQUID_DESTROY_EFFECT } from '../www/core/squid-destroy.js';
import { typeConstructorAddress } from '../www/core/squid-constructors.js';
import { Kernel } from '../www/core/kernel.js';
import { SquidProcessHost, SquidProcessState } from '../www/core/squid-process.js';
import { ContentPreviewFields, ContentPreviewProcess, RawContentPreviewProcess } from '../www/core/raw-content-preview-process.js';

// 원본 관측 파일 행이다(10.78 전용). 파일이 없으면 해당 검사를 건너뛴다. (2026-11-10 WB04-E241)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'content-preview-process-x86.tsv'));

// 바이트 배열을 소문자 16진 문자열로 바꾼다. (2026-11-10 WB04-E241)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 64비트 비트열을 double 로 읽는다. (2026-11-10 WB04-E241)
function doubleFromBits(bits) {
    const view = new DataView(new ArrayBuffer(8));
    view.setBigUint64(0, BigInt(bits), true);
    return view.getFloat64(0, true);
}

// 32비트 비트열을 float 로 읽는다(NaN 비트도 호스트가 읽는 값 그대로). (2026-11-10 WB04-E241)
function floatFromBits(bits) {
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, Number(bits) >>> 0, true);
    return view.getFloat32(0, true);
}

// 만료 시각 +10 의 double 비트를 기록한다. (2026-11-10 WB04-E241)
function setDeadline(fields, bits) {
    put(fields.bytes, 16, Number(BigInt(bits) & 0xffffffffn));
    put(fields.bytes, 20, Number(BigInt(bits) >> 32n));
}

// 순서 있는 효과를 원문과 같은 형식으로 기록한다. (2026-11-10 WB04-E241)
function event(trace, name, values = []) {
    const head = trace.length ? `${trace};` : '';
    return `${head}${name}${values.map((value) => `:${value}`).join('')}`;
}

// 내장 Form 네 생성자 주소만 넣은 188칸 TYPE 입력이다(cpppj MakeFormTypeInputs). (2026-11-10 WB04-E241)
const formTypeInputs = () => Array.from({ length: 188 }, (_, number) => ({ cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, ...([1, 2, 3, 6].includes(number) ? { constructorAddress: typeConstructorAddress(number) } : {}) }));

// 957행: 입력 열 9개 → 24바이트와 효과 순서(53비트 정밀도)를 재생한다.
test('RawContentPreviewProcess: 원본10.78 생성·프레임·통지·소멸 957행', { skip: !rows && '원본 fixture 없음' }, () => {
    const pool = new SidPool(24000, { server: true });
    let count = 0;
    for (const columns of rows) {
        assert.equal(columns.length, 5);
        const row = columns[0].split(',').map((value) => BigInt(value));
        assert.equal(row.length, 9);
        const fields = new ContentPreviewFields();
        fields.bytes.fill(0xa5);
        put(fields.bytes, 0, 0x503c1c);
        put(fields.bytes, 4, 90);
        put(fields.bytes, 8, Number(row[4]));
        setDeadline(fields, row[3]);
        const parent = Number(row[4]);
        put(pool.physical(parent), 6, Number(row[6]), 2); // 독립 입력마다 물리 슬롯을 다시 채운다.
        let now = doubleFromBits(row[1]);
        const kind = Number(row[5]);
        let trace = '';
        const logic = new RawContentPreviewProcess(pool, { now: () => now, processType: () => kind }, {
            attach: (attached, sid, type, flags) => {
                trace = `${event(trace, 'A', [type, sid, 0, flags])}:${hex(attached.bytes)}`;
                put(attached.bytes, 4, 90);
                put(attached.bytes, 8, sid);
                if (row[7] === 1n) now = 17.25;
                if (row[7] === 2n) setDeadline(attached, 0x401d000000000000n); // 7.25
            },
            destroy: (sid, flags) => {
                trace = event(trace, 'D', [sid, flags]);
                if (row[7] === 4n) fields.bytes.fill(0xcc);
            },
            assertion: () => {
                trace = `${event(trace, '!')}:parent->getHead() != 0:164`;
                if (row[7] === 3n) put(fields.bytes, 8, 51);
            },
            release: (released) => { trace = `${event(trace, 'F')}:${hex(released.bytes)}`; },
        });
        switch (Number(row[0])) {
        case 0: logic.construct(fields, parent, floatFromBits(row[2])); break;
        case 1: logic.run(fields); break;
        case 2: RawContentPreviewProcess.notify(fields, Number(row[8]), 0x80000000, 0xffffffff); break;
        case 3: logic.destruct(fields, Number(row[8])); break;
        default: assert.fail('알 수 없는 동작');
        }
        assert.equal(hex(fields.bytes), columns[1], `행 ${count + 1} 24바이트 ${columns[0]}`);
        assert.equal(trace.length ? trace : '-', columns[2], `행 ${count + 1} 효과 ${columns[0]}`);
        count += 1;
    }
    assert.equal(count, 957);
});

// 같은 풀의 삭제기·Kernel·호스트 묶음이다. 자산 그래픽은 void 입력으로 제한한다(cpppj PreviewHostScene). (2026-11-10 WB04-E241)
class PreviewHostScene {
    constructor() {
        this.pool = new SidPool(24000, { server: true });
        this.hash = new SquidHash();
        this.types = formTypeInputs();
        this.unpop = new SquidUnpop(this.pool, this.hash, new Uint8Array(65536));
        this.destroy = new RawSquidDestroy(this.pool, this.unpop, this.types);
        this.kernel = new Kernel();
        this.state = new SquidProcessState();
        this.sharedType = 47; // SoundProcess 와 공유하는 현재 전역의 명시 입력이다.
        this.deletes = 0;
        this.lastFlags = 0;
        this.latest = null;
        this.host = new SquidProcessHost(this.pool, this.types, this.kernel, this.destroy, this.state, {
            asset: {
                emit: (event) => {
                    if (event.effect === SQUID_DESTROY_EFFECT.preDestroy) this.destroy.completePreDestroy();
                    else if (event.effect === SQUID_DESTROY_EFFECT.postDestroy) this.destroy.completePostDestroy();
                    else throw new Error('void 미리보기 장면에 없는 자산 효과');
                },
                selected: () => 0,
            },
        });
        this.logic = new RawContentPreviewProcess(this.pool, { now: () => this.state.now, processType: () => this.sharedType }, {
            attach: (fields, parent, kind, flags) => {
                this.latest = this.host.attach(new ContentPreviewProcess(this.logic, fields), kind, parent, flags);
            },
            destroy: (parent, flags) => {
                this.deletes += 1;
                this.lastFlags = flags;
                this.destroy.destroy(parent, flags, this.host.hooks());
            },
            assertion: () => { throw new Error('부착된 부모에 실제 종속 머리가 없습니다'); },
        });
    }

    // 원래 내용물 미리보기처럼 클라이언트 자산을 만들고 공간 미등록 void 상태를 명시한다.
    parent() {
        const sid = this.pool.allocate(2);
        const raw = this.pool.allocated(sid);
        put(raw, 0, 0x501dd0);
        raw[10] = 82;
        raw[11] = 4;
        raw[40] = 8;
        return sid;
    }

    // 실제 생성 몸체에서 같은 Host 부착 경계까지 실행한다.
    add(parent, duration) {
        this.logic.construct(new ContentPreviewFields(), parent, duration);
        return this.latest;
    }
}

// 부모 삭제가 실행 중인 자신과 다른 만료 타이머의 Form/Kernel 을 한 번씩 제거해야 한다(자연 만료와 외부 부모 삭제 모두).
test('ContentPreviewProcess: 실제 Host 만료와 외부 부모 삭제의 수명', () => {
    for (const external of [false, true]) {
        const scene = new PreviewHostScene();
        scene.state.now = 16777217.25;
        const parent = scene.parent();
        const first = scene.add(parent, 0.2);
        assert.ok(first);
        const firstForm = first.form;
        const deadline = first.fields.deadline();
        const firstPid = scene.kernel.find(first);
        const second = scene.add(parent, 0.8);
        assert.ok(second);
        const secondForm = second.form;
        const secondPid = scene.kernel.find(second);
        assert.equal(scene.kernel.size, 2);
        assert.equal(scene.pool.slot(firstForm)[10], 47);
        assert.equal(scene.pool.slot(secondForm)[10], 47);
        assert.equal(get(first.fields.bytes, 4), firstForm);
        assert.equal(get(first.fields.bytes, 8), parent);
        const before = Buffer.from(first.fields.bytes);
        first.notify(3, 0, 0);
        assert.deepEqual(Buffer.from(first.fields.bytes), before);
        scene.state.now = nextBelow(deadline);
        scene.kernel.runFrame();
        assert.equal(scene.kernel.size, 2);
        assert.equal(scene.deletes, 0);
        if (external) scene.destroy.destroy(parent, 0, scene.host.hooks());
        else {
            scene.state.now = deadline;
            scene.kernel.runFrame();
        }
        assert.equal(scene.kernel.size, 0);
        assert.equal(scene.kernel.get(firstPid), null);
        assert.equal(scene.kernel.get(secondPid), null);
        assert.ok(scene.pool.slot(parent)[11] & 1);
        assert.ok(scene.pool.slot(firstForm)[11] & 1);
        assert.ok(scene.pool.slot(secondForm)[11] & 1);
        assert.equal(scene.destroy.preDepth, 0);
        assert.equal(scene.destroy.postDepth, 0);
        assert.equal(scene.deletes, external ? 0 : 1);
        if (!external) assert.equal(scene.lastFlags, 0x400000);
    }
});

// 바로 아래 double 이다(cpppj std::nextafter(deadline, -INFINITY)). (2026-11-10 WB04-E241)
function nextBelow(value) {
    const view = new DataView(new ArrayBuffer(8));
    view.setFloat64(0, value, true);
    view.setBigUint64(0, view.getBigUint64(0, true) - 1n, true);
    return view.getFloat64(0, true);
}

// 없는 효과는 선택될 때만 거부하며 실제 빈 통지는 연결 없이 허용한다. 10.72 풀은 만들 수 없다.
test('RawContentPreviewProcess: 선택된 효과만 요구하고 판본을 거부한다', () => {
    const pool = new SidPool(24000, { server: true });
    const logic = new RawContentPreviewProcess(pool, { now: () => 1.0, processType: () => 47 }, {});
    const fields = new ContentPreviewFields();
    setDeadline(fields, 0x4000000000000000n); // 2.0
    logic.run(fields); // 미만료는 부모도 효과도 읽지 않는다.
    RawContentPreviewProcess.notify(fields, 1, 2, 3);
    setDeadline(fields, 0x7ff8123456789abcn);
    logic.run(fields); // 비교 불가도 부모와 삭제 효과를 요구하지 않는다.
    assert.throws(() => logic.construct(fields, 50, 0.2), TypeError);
    logic.destruct(fields, 0);
    assert.equal(get(fields.bytes, 4), 0xffffcfc7);
    assert.equal(get(fields.bytes, 8), 0xffffcfc7);
    assert.throws(() => logic.destruct(fields, 1), TypeError);
    // 웹 SidPool 은 항상 10.78 이라 CD 판본은 edition 만 가진 대역으로 거부 경로를 확인한다.
    assert.throws(() => new RawContentPreviewProcess({ edition: 'cd1072' }, { now: () => 1, processType: () => 47 }, {}), TypeError);
    assert.throws(() => new RawContentPreviewProcess(null, { now: () => 1, processType: () => 47 }, {}), TypeError);
});

// 생성 계약: float 로 줄인 duration 에 현재 시각을 더하고, ∞ + −∞ 는 호스트와 무관하게 x87 의 실수 불확정 비트가 되며, 부착 효과가 받는 인자(현재 타입의 DWORD·flags 0x10)와 앞선 +0/+4/+8 쓰기를 확인한다.
test('RawContentPreviewProcess: 생성 인자와 만료 시각 계약', () => {
    const pool = new SidPool(24000, { server: true });
    let now = 1.0;
    let type = 47;
    const seen = [];
    const logic = new RawContentPreviewProcess(pool, { now: () => now, processType: () => type }, { attach: (fields, parent, kind, flags) => seen.push([parent, kind, flags, hex(fields.bytes)]) });
    const fields = new ContentPreviewFields();
    fields.bytes.fill(0xa5);
    logic.construct(fields, 50, 0.2); // 0.2 는 float32 로 줄어든 뒤 더한다(1.2 가 아니다).
    assert.equal(fields.deadline(), Math.fround(0.2) + 1.0);
    assert.notEqual(fields.deadline(), 1.2);
    assert.equal(hex(fields.bytes.subarray(0, 16)), '1c3c50000000000000000000a5a5a5a5'); // +0 VT·+4 Form·+8 부모는 0, +c 패딩은 보존.
    assert.deepEqual(seen[0].slice(0, 3), [50, 47, 0x10]);
    type = -1; // 현재 타입은 DWORD 로 전달한다.
    now = Infinity;
    logic.construct(fields, 50, -Infinity);
    assert.equal(hex(fields.bytes.subarray(16)), '000000000000f8ff');
    assert.equal(seen[1][1], 0xffffffff);
    now = -Infinity;
    logic.construct(fields, 50, Infinity);
    assert.equal(hex(fields.bytes.subarray(16)), '000000000000f8ff');
    now = Infinity;
    logic.construct(fields, 50, Infinity); // NaN 이 아닌 무한대끼리의 합은 그대로 +∞ 이다.
    assert.equal(hex(fields.bytes.subarray(16)), '000000000000f07f');
});

// 프레임 계약: 부모 DWORD 가 SID 범위를 넘으면 거부하고, head 가 있으면 진단 없이 삭제만 하며, head 가 없으면 진단이 반환해도 처음 고정한 부모에 삭제한다.
test('RawContentPreviewProcess: 프레임 부모 고정과 진단 순서', () => {
    const pool = new SidPool(24000, { server: true });
    const trace = [];
    const logic = new RawContentPreviewProcess(pool, { now: () => 5.0, processType: () => 47 }, {
        assertion: () => trace.push('!'),
        destroy: (parent, flags) => trace.push(`D${parent}:${flags}`),
    });
    const fields = new ContentPreviewFields();
    setDeadline(fields, 0x4014000000000000n); // 5.0: now >= deadline 이므로 만료이다.
    put(fields.bytes, 8, 0x10000);
    assert.throws(() => logic.run(fields), RangeError);
    assert.deepEqual(trace, []);
    put(fields.bytes, 8, 50);
    put(pool.physical(50), 6, 0, 2);
    put(pool.physical(50), 6, 0x0100, 2); // 상위 바이트만 있어도 head 가 있다.
    logic.run(fields);
    assert.deepEqual(trace, ['D50:4194304']);
    put(pool.physical(50), 6, 0, 2);
    logic.run(fields);
    assert.deepEqual(trace.slice(1), ['!', 'D50:4194304']);
    setDeadline(fields, 0x4014000000000001n); // 바로 위 double: 아직 만료 전이다.
    logic.run(fields);
    assert.equal(trace.length, 3);
});

// 프로세스 래퍼 계약: Host 에 부착되지 않은 프레임은 무동작, 부착 전 onAttached 와 다른 풀의 Host 는 거부, Kernel 소멸(destroy)은 flags 0 소멸 쓰기이다.
test('ContentPreviewProcess: 부착 전후와 풀 불일치 계약', () => {
    const scene = new PreviewHostScene();
    const logic = new RawContentPreviewProcess(scene.pool, { now: () => 100, processType: () => 47 }, { destroy: () => assert.fail('부착 전 프레임은 실행하지 않는다') });
    const fields = new ContentPreviewFields();
    setDeadline(fields, 0n);
    const loose = new ContentPreviewProcess(logic, fields);
    loose.runFrame(); // Host 가 없으면 무동작이다.
    assert.throws(() => loose.onAttached(), /풀 불일치/);
    const other = new SidPool(24000, { server: true });
    const mismatched = new ContentPreviewProcess(new RawContentPreviewProcess(other, { now: () => 0, processType: () => 47 }, {}), new ContentPreviewFields());
    assert.throws(() => scene.host.attach(mismatched, 47, scene.parent(), 0x10), /풀 불일치/);
    loose.destroy();
    assert.equal(get(fields.bytes, 0), 0x5019ec);
    assert.equal(get(fields.bytes, 4), 0xffffcfc7);
    assert.equal(get(fields.bytes, 8), 0xffffcfc7);
    assert.equal(loose.fields, fields);
    loose.notify(1, 2, 3); // 빈 통지는 필드를 바꾸지 않는다.
    assert.equal(get(fields.bytes, 0), 0x5019ec);
});

// 부모 DWORD 경계: 0xffff 까지는 SID 이고 0x10000 이상(부호 비트가 켜진 값 포함)은 풀을 읽기 전에 SID 범위 오류로 거부한다.
test('ContentPreviewFields: 부모 DWORD 의 SID 범위 경계', () => {
    const fields = new ContentPreviewFields();
    put(fields.bytes, 8, 0xffff);
    assert.equal(fields.parent(), 0xffff);
    put(fields.bytes, 8, 0);
    assert.equal(fields.parent(), 0);
    for (const value of [0x10000, 0x80000000, 0xffffffff]) {
        put(fields.bytes, 8, value);
        assert.throws(() => fields.parent(), /SID 범위/, `부모 ${value}`);
    }
    assert.equal(new ContentPreviewFields().deadline(), 0); // 초기 24바이트는 모두 0 이다.
});

// 선택된 효과가 함수가 아니면 undefined 뿐 아니라 null·숫자도 그 위치에서 "효과 미연결" 로 거부한다.
test('RawContentPreviewProcess: 함수가 아닌 효과는 모두 미연결로 거부한다', () => {
    const pool = new SidPool(24000, { server: true });
    for (const bad of [undefined, null, 5, 'x', {}]) {
        const logic = new RawContentPreviewProcess(pool, { now: () => 9, processType: () => 47 }, { attach: bad, destroy: bad, assertion: bad, release: bad });
        const fields = new ContentPreviewFields();
        assert.throws(() => logic.construct(fields, 50, 0.2), /실제 효과 미연결/);
        put(fields.bytes, 8, 50);
        put(pool.physical(50), 6, 1, 2);
        setDeadline(fields, 0n);
        assert.throws(() => logic.run(fields), /실제 효과 미연결/); // head 가 있으므로 진단 없이 삭제 효과에서 거부한다.
        put(pool.physical(50), 6, 0, 2);
        assert.throws(() => logic.run(fields), /실제 효과 미연결/); // head 가 없으므로 진단 효과에서 거부한다.
        assert.throws(() => logic.destruct(fields, 3), /실제 효과 미연결/);
    }
});
