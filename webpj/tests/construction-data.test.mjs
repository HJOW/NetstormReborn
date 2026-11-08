// 건설 상태 메시지 raw-construction-data.js(writeConstructionData·writeConstructionProcessData·readConstructionData·RawConstructionData)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: constructiondata-x86.tsv 의 `originals` 1780행 — Write 1034(dirty 마스크·form·부모·force·헤더 바이트)·Read 512·ReadNull 32·Receive 162(free/live form·dirty·할당/부착/조회 경계·변동 1/2)·
//    Wrapper 40(실제 host 에 붙인 건설 프로세스의 전송)에서 사건 순서(A 할당/J 부착/K Kernel 조회)·필드 7항목(dirty·시작/끝 시각 비트·소리·추가)·부모 출력·패킷 전/후 전체·주변 form raw 가 같다.
//    `originalCD`·`original1037` 행은 다른 판본이라 사용하지 않는다.
//  - 계약: 부족한 버퍼 길이는 필드/패킷 변경 전에 거부(선언 길이는 읽기 판단에 쓰지 않음), 잘못된 새/기존 수신·할당 실패·필수 경계 누락은 부착 전에 거부, 실제 host 의 전체/부분 전송→수신→Kernel 진행→지연 완료,
//    다른 프로세스(Regular)·상위 비트 Kernel 번호가 건설 필드를 덮어쓰지 못함, 실제 새 소유 수신(logic+host)의 부착과 잘못된 부모/풀 거부.
// 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E141: 건설 상태 메시지 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { SquidUnpop } from '../www/core/squid-unpop.js';
import { RawSquidDestroy } from '../www/core/squid-destroy.js';
import { Kernel } from '../www/core/kernel.js';
import { SquidProcessHost, SquidProcessState } from '../www/core/squid-process.js';
import { typeConstructorAddress } from '../www/core/squid-constructors.js';
import { ConstructionProcess, ConstructionProcessState, RawConstructionProcess } from '../www/core/raw-construction-process.js';
import { RawConstructionData, createConstructionDataHeader, readConstructionData, writeConstructionData, writeConstructionProcessData } from '../www/core/raw-construction-data.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-09 WB04-E141)
const allRows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'constructiondata-x86.tsv')) ?? [];
const rows = allRows.filter((row) => row[0] === 'originals');

// 원본 관찰의 입력 DWORD 순서, 판본별 행 수, 저장소의 소리 표식, 풀 크기이다. (2026-11-09 WB04-E141)
const [DIRTY, FORM, PARENT, FORCE, INCLUDE, START_LOW, START_HIGH, END_LOW, END_HIGH, ADDITIONAL, PROTOCOL, MESSAGE, FREE, PID, PRESENT, MUTATE, TYPE] = Array.from({ length: 17 }, (_, index) => index);
const ROWS = 1780;
const SOUND = 0x15000040;
const CAPACITY = 24000;

// 예외가 나는지 본다. (2026-11-09 WB04-E141)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 두 DWORD 를 산술 변환 없이 double 로 묶는다. (2026-11-09 WB04-E141)
const view = new DataView(new ArrayBuffer(8));
function time(low, high) {
    view.setUint32(0, low, true);
    view.setUint32(4, high, true);
    return view.getFloat64(0, true);
}

// 필드 전체를 실제 기계어 관찰의 일곱 항목으로 만든다(시각은 비트로). (2026-11-09 WB04-E141)
function fieldsText(fields) {
    view.setFloat64(0, fields.start, true);
    const startLow = view.getUint32(0, true);
    const startHigh = view.getUint32(4, true);
    view.setFloat64(0, fields.end, true);
    return [fields.dirty, startLow, startHigh, view.getUint32(0, true), view.getUint32(4, true), fields.sound, fields.additional].join(':');
}

// 16진수 문자열을 바이트로 바꾼다. (2026-11-09 WB04-E141)
const bytesOf = (text) => Uint8Array.from(text.match(/../g) ?? [], (pair) => parseInt(pair, 16));
// 바이트를 소문자 16진수로 만든다. (2026-11-09 WB04-E141)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 공통 ProcessForm 생성이 읽는 원문 내장 Form 생성자 주소를 입력에 명시한 타입 표이다. (2026-11-09 WB04-E141)
const formTypeInputs = () => Array.from({ length: 188 }, (_, number) => ({ cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, maxHitPoints: 0, ...([1, 2, 3, 6].includes(number) ? { constructorAddress: typeConstructorAddress(number) } : {}) }));

// 실제 form/Kernel 을 소유한 검사 월드이다. hooks 수신과 새 객체 직접 부착을 함께 검사한다. (2026-11-09 WB04-E141)
class DataScene {
    constructor(server = false) {
        this.pool = new SidPool(CAPACITY, { server });
        this.types = formTypeInputs();
        this.hash = new SquidHash();
        this.spots = new Uint8Array(65536);
        this.unpop = new SquidUnpop(this.pool, this.hash, this.spots);
        this.destroy = new RawSquidDestroy(this.pool, this.unpop, this.types);
        this.kernel = new Kernel();
        this.clock = new SquidProcessState();
        this.host = new SquidProcessHost(this.pool, this.types, this.kernel, this.destroy, this.clock);
        this.state = new ConstructionProcessState();
        this.logic = new RawConstructionProcess(this.pool, this.types, this.state, this.hooks(), this.host);
        const raw = this.pool.physical(20000);
        raw.fill(0);
        raw[10] = 100;
        raw[40] = 1;
    }

    // HP 와 abstract 완료만 실제 raw 상태에 연결하고 음향/표시 호출은 비워 둔다. (2026-11-09 WB04-E141)
    hooks() {
        return {
            reserve: () => true, effect: () => {}, loadSound: () => SOUND, reserveSound: () => false, attachSound: () => {}, refresh: () => {}, updateDisplay: () => {},
            removeContained: () => {}, playAt: () => {}, readySound: () => 0, play: () => {},
            unpop: (sid) => { this.pool.allocated(sid)[11] |= 4; },
            repop: (sid) => { this.pool.allocated(sid)[11] &= 0xfb; },
            push: () => {}, maxHitPoints: () => 100,
        };
    }

    // 사용하지 않는 새 객체 경계를 갖춘 기존 form 수신 옵션이다. lookup 은 실제 host 가 담당한다. (2026-11-09 WB04-E141)
    dataOptions(extra = {}) {
        return { hooks: { allocate: () => null, attach: () => {} }, host: this.host, ...extra };
    }
}

test('상태 메시지: 원본 10.78 관측 1780행(Write·Read·ReadNull·Receive·Wrapper)의 필드·부모·패킷·form raw 가 같다', { skip: rows.length === 0 }, () => {
    assert.equal(rows.length, ROWS);
    const pool = new SidPool(CAPACITY, { server: false });
    const state = new ConstructionProcessState();
    const counts = {};
    // 행별 입력 필드와 경계 응답만 준비하며 출력은 fixture 에서 읽는다.
    for (const row of rows) {
        assert.equal(row.length, 9);
        const input = row[2].split(',').map(Number);
        assert.equal(input.length, 18);
        counts[row[1]] = (counts[row[1]] ?? 0) + 1;
        let fields = { dirty: input[DIRTY], start: time(input[START_LOW], input[START_HIGH]), end: time(input[END_LOW], input[END_HIGH]), sound: SOUND, additional: input[ADDITIONAL] };
        const packet = bytesOf(row[6]);
        const events = [];
        let output = '-';
        state.processType = input[TYPE];
        const form = row[1] === 'Receive' ? input[FORM] : 90;
        const raw = pool.physical(form);
        raw.fill(0xa7);
        raw[11] = input[FREE] & 0xff;
        put(raw, 18, input[PID]);
        if (row[1] === 'Write') {
            writeConstructionData(fields, packet, input[FORM], input[PARENT], input[FORCE] !== 0, { protocol: input[PROTOCOL] & 0xff, message: input[MESSAGE] & 0xff });
        } else if (row[1] === 'Wrapper') {
            const scene = new DataScene();
            if (input[FORM] === 6) assert.ok(scene.host.addRegular(20000, 1, 0));
            const process = scene.host.attach(new ConstructionProcess(scene.logic, fields), 30, 20000, 0x10);
            assert.ok(process);
            assert.equal(process.form, input[FORM]);
            writeConstructionProcessData(process, packet, input[INCLUDE] !== 0, input[FORCE] !== 0);
            fields = process.fields;
        } else if (row[1] === 'Read' || row[1] === 'ReadNull') output = String(readConstructionData(row[1] === 'ReadNull' ? null : fields, packet));
        else {
            const hooks = {
                // 할당은 현재 타입/패킷을 바꿀 수 있다. 수신 몸체가 캡처한 form 과 재조회한 값을 대조한다.
                allocate: () => {
                    events.push('A:56');
                    if (input[MUTATE] === 1) state.processType = 44;
                    if (input[MUTATE] === 2) { put(packet, 4, 91); put(packet, 9, 0xffffffff); }
                    return fields;
                },
                // 명시 form 부착의 원본 인자만 기록하고 raw form 은 바꾸지 않는다.
                attach: (_, type, parent, captured, flags) => events.push(`J:${type}:${parent}:${captured}:${flags}`),
                // Kernel 번호의 DWORD 전체를 기록하며 지정된 슬롯 유무만 공급한다.
                lookup: (pid) => { events.push(`K:${pid}`); return input[PRESENT] ? fields : null; },
            };
            new RawConstructionData(pool, state, { hooks }).receive(packet);
        }
        const where = `${row[1]} ${row[2]}`;
        assert.equal(events.length === 0 ? '-' : events.join(';'), row[3], where);
        assert.equal(fieldsText(fields), row[4], where);
        assert.equal(output, row[5], where);
        assert.equal(hex(packet), row[7], where);
        assert.equal(hex(raw), row[8], where);
    }
    assert.deepEqual(counts, { Write: 1034, Read: 512, ReadNull: 32, Receive: 162, Wrapper: 40 });
});

test('상태 메시지: 시작의 전체 전송과 끝 시각의 부분 전송을 실제 수신/Kernel 진행/완료에 연결한다', () => {
    const scene = new DataScene();
    scene.logic.start(20000, 8);
    const process = scene.host.fromForm(5);
    assert.ok(process instanceof ConstructionProcess);
    const packet = new Uint8Array(30);
    assert.equal(writeConstructionProcessData(process, packet, true, false), 30);
    assert.equal(process.fields.dirty, 0);
    assert.equal(packet[8], 255);
    const copy = { dirty: 0, start: 1, end: 2, sound: 0, additional: 9 };
    assert.equal(readConstructionData(copy, packet), 20000);
    assert.equal(copy.start, 0);
    assert.equal(copy.end, 8);
    assert.equal(copy.additional, 0);
    scene.state.now = 5;
    scene.logic.resetEndFromForm(5, 1);
    assert.equal(process.fields.end, 15);
    assert.equal(process.fields.dirty, 4);
    assert.equal(writeConstructionProcessData(process, packet, false, false), 17);
    assert.equal(packet[8], 4);
    assert.equal(process.fields.dirty, 0);
    process.fields.end = 8;
    process.fields.additional = 0x7b;
    new RawConstructionData(scene.pool, scene.state, scene.dataOptions()).receive(packet.subarray(0, 17));
    assert.equal(process.fields.end, 15);
    assert.equal(process.fields.additional, 0x7b);
    assert.equal(process.fields.dirty, 0);
    scene.state.now = 8;
    scene.kernel.runFrame();
    assert.equal(scene.kernel.size, 1);
    assert.equal(get(scene.pool.slot(20000), 26, 4), 53);
    scene.state.now = 15;
    scene.kernel.runFrame();
    assert.equal(scene.kernel.size, 0);
    assert.equal(get(scene.pool.slot(20000), 26, 4), 100);
    assert.equal(scene.pool.slot(20000)[40] & 1, 0);
});

test('상태 메시지: 모든 부족 버퍼 길이는 필드/패킷 변경 전에 거부하고 선언 길이는 읽기 판단에 쓰지 않는다', () => {
    const fresh = () => ({ dirty: 255, start: 1, end: 2, sound: SOUND, additional: 0x5a });
    const fields = fresh();
    const before = fieldsText(fields);
    const packet = new Uint8Array(30);
    // 최대 쓰기의 모든 부족 길이와 최대 읽기의 모든 부족 길이를 검사한다.
    for (let size = 0; size < packet.length; size += 1) {
        packet.fill(0xcc);
        assert.ok(throws(() => writeConstructionData(fields, packet.subarray(0, size), 90, 20000, true)), `write ${size}`);
        assert.equal(fieldsText(fields), before);
        assert.equal(hex(packet), 'cc'.repeat(30));
        packet.fill(0);
        packet[8] = 255;
        const partial = fresh();
        assert.ok(throws(() => readConstructionData(partial, packet.subarray(0, size))), `read ${size}`);
        // 읽기는 선택된 실제 저장 직전만 검사하므로 이미 들어간 앞선 필드는 보존한다: 시작 시각은 21바이트, 끝 시각은 29바이트부터 쓰이고 추가 바이트는 30바이트 전체에서만 읽힌다.
        assert.equal(partial.start, size >= 21 ? 0 : 1, `start ${size}`);
        assert.equal(partial.end, size >= 29 ? 0 : 2, `end ${size}`);
        assert.equal(partial.additional, 0x5a);
        assert.equal(partial.dirty, 255);
        assert.equal(partial.sound, SOUND);
    }
    packet.fill(0);
    packet[8] = 255;
    put(packet, 2, 0xffff, 2);
    assert.equal(readConstructionData(fields, packet), 0);
    assert.equal(fields.dirty, 255);
    assert.equal(fields.sound, SOUND);
    assert.ok(throws(() => readConstructionData(null, packet)));
});

test('상태 메시지: 부정한 새/기존 상태·할당 실패·필수 경계 누락은 부착 경계를 실행하지 않는다', () => {
    const scene = new DataScene();
    const fields = { dirty: 0, start: 0, end: 0, sound: 0, additional: 0 };
    const events = [];
    const hooks = { allocate: () => fields, lookup: () => fields, attach: () => events.push('attach') };
    const receiver = new RawConstructionData(scene.pool, scene.state, { hooks });
    const packet = new Uint8Array(30);
    put(packet, 4, 90);
    scene.pool.physical(90)[11] = 1;
    // 진단 장치가 없으면 free form 의 부분 패킷(0x42)·live form 의 부모 비트(0x4a)를 성공으로 합성하지 않는다.
    packet[8] = 15;
    assert.ok(throws(() => receiver.receive(packet)));
    packet[8] = 255;
    // 전체 상태(0xff)의 free form 은 진단 없이 새 객체를 만들어 부착 경계를 한 번 부른다.
    receiver.receive(packet);
    assert.deepEqual(events, ['attach']);
    events.length = 0;
    scene.pool.physical(90)[11] = 0;
    assert.ok(throws(() => receiver.receive(packet)));
    put(packet, 4, 0x1005a);
    assert.ok(throws(() => receiver.receive(packet)));
    assert.deepEqual(events, []);
    assert.ok(throws(() => new RawConstructionData(scene.pool, scene.state, { hooks: { ...hooks, allocate: undefined } })));
    assert.ok(throws(() => new RawConstructionData(scene.pool, scene.state, { hooks: { allocate: () => fields, attach: () => {} } })));
    const other = new DataScene();
    assert.ok(throws(() => new RawConstructionData(scene.pool, scene.state, { hooks, host: other.host })));
    assert.ok(throws(() => new RawConstructionData(scene.pool, scene.state, { logic: other.logic, host: scene.host })));
});

test('상태 메시지: 다른 프로세스(Regular)·상위 비트 Kernel 번호가 건설 필드를 덮어쓰지 못한다', () => {
    const scene = new DataScene();
    scene.logic.start(20000, 8);
    const process = scene.host.fromForm(5);
    assert.ok(process instanceof ConstructionProcess);
    const before = fieldsText(process.fields);
    const packet = new Uint8Array(30);
    packet[8] = 4;
    put(packet, 4, 5);
    const receiver = new RawConstructionData(scene.pool, scene.state, scene.dataOptions());
    put(scene.pool.allocated(5), 18, 0x10001);
    assert.ok(throws(() => receiver.receive(packet)));
    assert.equal(fieldsText(process.fields), before);
    packet[8] = 0x80;
    assert.ok(!throws(() => receiver.receive(packet)));
    put(scene.pool.allocated(5), 18, 1);
    packet[8] = 4;
    const regular = scene.host.addRegular(20000, 1, 0);
    assert.ok(regular);
    put(packet, 4, regular.form);
    assert.ok(throws(() => receiver.receive(packet)));
});

test('상태 메시지: 실제 새 소유 수신(logic+host)은 서버 form 에 건설 프로세스를 부착하고 잘못된 부모/풀을 거부한다', () => {
    const source = new DataScene(true);
    const target = new DataScene(false);
    target.clock.boss = false;
    target.state.boss = false;
    const form = source.pool.layout.serverFirst + 50;
    const sent = source.host.attach(new ConstructionProcess(source.logic, { dirty: 255, start: 0, end: 8, sound: SOUND, additional: 0x5a }), 30, 20000, 0x10, form);
    assert.ok(sent);
    const packet = new Uint8Array(30);
    assert.equal(writeConstructionProcessData(sent, packet, true, false, { protocol: 0x82, message: 55 }), 30);
    const diagnostics = [];
    const receiver = new RawConstructionData(target.pool, target.state, { logic: target.logic, host: target.host, assertion: (expression, file, line) => diagnostics.push([expression, file, line]) });
    receiver.receive(packet);
    const received = target.host.fromForm(form);
    assert.ok(received instanceof ConstructionProcess);
    assert.equal(received.form, form);
    assert.equal(received.parent, 20000);
    assert.equal(target.kernel.size, 1);
    assert.equal(received.fields.dirty, 255);
    assert.equal(received.fields.start, 0);
    assert.equal(received.fields.end, 8);
    assert.equal(received.fields.additional, 0x5a);
    assert.equal(received.fields.sound, 0);
    assert.deepEqual(diagnostics, []);
    // 부분 상태로 오는 새 form 은 원문 진단 0x42 를 낸 뒤 계속한다.
    const partial = new Uint8Array(packet);
    partial[8] = 0x0f;
    put(partial, 4, form + 1);
    receiver.receive(partial);
    assert.deepEqual(diagnostics.map((item) => item[2]), [0x42]);
    // 범위를 넘는 부모 DWORD 는 Kernel 객체를 남기지 않고 거부한다.
    const before = target.kernel.size;
    const bad = new Uint8Array(30);
    bad[8] = 255;
    put(bad, 4, form + 7);
    put(bad, 9, 0x10000);
    assert.ok(throws(() => receiver.receive(bad)));
    assert.equal(target.kernel.size, before);
});
