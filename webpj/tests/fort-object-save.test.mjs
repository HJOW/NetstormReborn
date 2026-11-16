// 원본 객체3294행/청크672행의 CRT·현재 조회·폭별 쓰기·고정 count와 실제 Session 결선을 대조한다(WB04-E630).
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, get, put, floatToBits } from './raw-support.mjs';
import { saveFortObject, saveFortChunk, fortSaveCoordinate, FORT_OBJECT_SAVE_ASSERTION } from '../www/core/raw-fort-object-save.js';
import { MEM_STREAM_ASSERTION } from '../www/core/raw-mem-stream.js';
import { makeCampaignFortObjectWriter } from '../www/core/raw-campaign-object-save.js';
import { campaignOwners } from './campaign-session-support.mjs';
import { fortSection, SECTION_NAMES } from '../www/assets/fort.js';

// 최초 관측의 전역/외부 응답 순서와 원시 버퍼·stream 토큰이다.
const GLOBALS = ['battle', 'mission', 'editor', 'scope', 'version', 'localOwner', 'bridgeType', 'daisType', 'battleMask', 'ownerMask'];
const DEVICES = ['flags1', 'flags2', 'robust', 'island', 'hasLevel', 'level']; const POINTERS = [0x01001000, 0x01006000]; const STREAM = 0x01000100;
// 진단 종류별 실제 원문 행이다.
const LINES = { XOffset: 667, YOffset: 668, Owner: 707 };
// 원문 emu_stop만 구별하며 제품/물리 오류를 삼키지 않는다.
class FixtureStop extends Error {}
// signed 문자열도 하위 DWORD를 보존해 원문 입력으로 해석한다.
function word(value) { return Number(value) >>> 0; }
// 기대가 아닌 최초 입력/외부 응답·변경만 가진 독립 장치다.
class Devices {
    // 고정 raw/전역/응답과 실제 stream·두 출력 물리다.
    raw = new Uint8Array(50); globals = new Uint32Array(10); devices = new Uint32Array(6); stream = new Uint8Array(16); buffers = [new Uint8Array(16384).fill(0xcc), new Uint8Array(16384).fill(0xcc)];
    // 외부 효과·경계 횟수·trace와 객체/검색기 입력이다.
    effects = []; calls = new Map(); trace = []; sid = 0; poolLimit = 0; current = 0; sequence = []; sequenceIndex = 0; returns = new Map(); payloads = new Map();
    // 비영 토큰이 가리키는 실제 버퍼만 반환한다.
    buffer(pointer) { const index = POINTERS.indexOf(pointer); assert.ok(index >= 0); return this.buffers[index]; }
    // 명시 stream와 효과 입력을 준비한다.
    initializeStream(row, at) { put(this.stream, 0, POINTERS[0], 4); put(this.stream, 4, word(row[at]), 4); put(this.stream, 8, word(row[at + 1]), 4); put(this.stream, 12, word(row[at + 2]), 4); if (row[at + 3] !== '-') this.effects = row[at + 3].split(';').map((text) => text.split('/')); }
    // 객체 입력25열만 구성한다.
    initializeObject(row) {
        this.sid = word(row[1]); this.raw.set(Buffer.from(row[4], 'hex')); this.devices[0] = word(row[5]); this.devices[1] = word(row[6]);
        // 원문 전역10DWORD와 네 가상 응답 입력을 고정 순서로 배치한다.
        for (let index = 0; index < 10; index++) this.globals[index] = word(row[7 + index]);
        for (let index = 2; index < 6; index++) this.devices[index] = word(row[15 + index]);
        this.initializeStream(row, 21);
    }
    // 청크 입력10열의 현재 후보/반환/명시 payload를 준비한다.
    initializeChunk(row) {
        this.poolLimit = word(row[3]); if (row[4] !== '-') this.sequence = row[4].split(',').map(word);
        // 반환표에 없는 응답1은 최초 독립 PE 외부 장치의 명시 기본값이다.
        if (row[5] !== '-') for (const text of row[5].split(';')) { const fields = text.split(':'); const sid = word(fields[0]); this.returns.set(sid, word(fields[1])); if (fields.length === 3) this.payloads.set(sid, fields[2] === '-' ? new Uint8Array(0) : new Uint8Array(Buffer.from(fields[2], 'hex'))); }
        this.initializeStream(row, 6);
    }
    // 실제 호출을 먼저 기록하고 효과를 적용할 시점은 각 외부 장치에 맡긴다.
    event(kind, text) { this.trace.push(text); const count = (this.calls.get(kind) ?? 0) + 1; this.calls.set(kind, count); return kind + count; }
    // 같은 경계의 입력 효과를 작성 순서대로 현재 물리에만 적용한다.
    mutate(event) {
        // 분기/직렬화 결과를 대신 계산하지 않는 외부 변경이다.
        for (const [at, target, field, input] of this.effects) {
            if (at !== event) continue; const value = word(input);
            if (target === 'global') { if (field === 'poollimit') this.poolLimit = value; else { const index = GLOBALS.indexOf(field); assert.ok(index >= 0); this.globals[index] = value; } }
            else if (target === 'device') { const index = DEVICES.indexOf(field); assert.ok(index >= 0); this.devices[index] = value; }
            else if (target === 'form') { const offsets = { type: [10, 1], owner: [34, 1], flags: [40, 1], qa: [12, 2], frame: [36, 4], x: [14, 4], y: [18, 4] }; assert.ok(offsets[field]); put(this.raw, offsets[field][0], value, offsets[field][1]); }
            else if (target === 'stream') { if (field === 'data') put(this.stream, 0, POINTERS[value], 4); else { const offsets = { position: 4, end: 8, capacity: 12 }; assert.ok(Object.hasOwn(offsets, field)); put(this.stream, offsets[field], value, 4); } }
            else if (target === 'finder' && field === 'current') this.current = value;
            else { assert.equal(target, 'control'); assert.equal(field, 'stop'); assert.equal(value, 1); throw new FixtureStop(); }
        }
    }
    // 진단의 실제 원문 위치와 변경/중단을 기록한다.
    assertion(line) { this.mutate(this.event('assert', 'assert:' + line)); }
    // 원문 외부 장치가 받은 bytes만 현재 data/pos에 쓰며 unsigned end를 진행한다.
    append(bytes) { const position = get(this.stream, 4, 4); const output = this.buffer(get(this.stream, 0, 4)); assert.ok(position + bytes.length <= output.length); output.set(bytes, position); const next = (position + bytes.length) >>> 0; put(this.stream, 4, next, 4); put(this.stream, 8, Math.max(get(this.stream, 8, 4), next), 4); }
    // flags getter는 효과 뒤 값, 가상 함수는 효과 전 캡처값을 반환한다.
    device(sid, index, capture) { assert.equal(sid, this.sid); const answer = this.devices[index]; const name = DEVICES[index]; this.mutate(this.event(name, name + ':' + sid + (capture ? '' : ':' + this.raw[10]))); return capture ? answer : this.devices[index]; }
    // 필터/필드 선택은 제품에 맡기는 실제 객체 외부 경계다.
    objectHooks() {
        return {
            global: (key) => { const index = GLOBALS.indexOf(key); assert.ok(index >= 0); return this.globals[index]; }, form: (sid) => { assert.equal(sid, this.sid); return this.raw; },
            flags1: (sid) => this.device(sid, 0, false), flags2: (sid) => this.device(sid, 1, false), robust: (sid) => this.device(sid, 2, true), hasLevel: (sid) => this.device(sid, 4, true), level: (sid) => this.device(sid, 5, true),
            island: (x, y) => { const answer = this.devices[3]; this.mutate(this.event('island', 'island:' + x + ':' + y)); return answer; },
            assertion: (kind) => { assert.ok(Object.hasOwn(LINES, kind)); this.assertion(LINES[kind]); },
            write: (token, source) => { assert.equal(token, STREAM); const bytes = source.slice(); const event = this.event('write', 'write:' + bytes.length + ':' + Buffer.from(bytes).toString('hex')); if ((get(this.stream, 12, 4) | 0) < ((get(this.stream, 4, 4) + bytes.length) | 0)) this.assertion(36); this.append(bytes); this.mutate(event); },
            contents: (sid, token) => { assert.equal(sid, this.sid); assert.equal(token, STREAM); const event = this.event('contents', 'contents:' + sid); this.append(new Uint8Array([0xee])); this.mutate(event); },
        };
    }
    // marker/count 쓰기는 제품이 수행하며 실제 Finder/객체 응답만 공급한다.
    chunkHooks() {
        return {
            poolLimit: () => this.poolLimit,
            begin: (area) => { this.sequenceIndex = 0; this.current = this.sequence[0] ?? 0; this.mutate(this.event('begin', 'begin:' + [area.left, area.top, area.right, area.bottom].map(floatToBits).join(':') + ':0')); return { current: () => this.current, next: () => { const before = this.current; this.sequenceIndex++; this.current = this.sequence[this.sequenceIndex] ?? 0; this.mutate(this.event('next', 'next:' + before)); } }; },
            object: (token, sid, x, y) => { assert.equal(token, STREAM); const answer = this.returns.get(sid) ?? 1; const event = this.event('object', 'object:' + sid + ':' + (x >>> 0) + ':' + (y >>> 0)); if (answer) this.append(this.payloads.has(sid) ? this.payloads.get(sid) : new Uint8Array([sid])); this.mutate(event); return answer; },
            access: (token, operation) => { assert.equal(token, STREAM); operation(this.stream, { buffer: (pointer) => this.buffer(pointer), assertion: (kind) => { assert.equal(kind, MEM_STREAM_ASSERTION.writeCapacity); this.assertion(36); } }); },
        };
    }
    // 실제 주소만 data0/1로 정규화하고 pos/end/capacity·두 버퍼는 원문 그대로 비교한다.
    checkStream(row, at) {
        const data = POINTERS.indexOf(get(this.stream, 0, 4)); assert.ok(data >= 0); assert.equal([data, get(this.stream, 4, 4), get(this.stream, 8, 4), get(this.stream, 12, 4)].join(':'), row[at], row[0]); const end = get(this.stream, 8, 4); assert.ok(end <= 16384);
        assert.equal(Buffer.from(this.buffers[0].subarray(0, end)).toString('hex'), row[at + 1], row[0]); assert.equal(Buffer.from(this.buffers[1].subarray(0, end)).toString('hex'), row[at + 2], row[0]);
    }
}

test('Fort 객체 writer: 원본3294행의 CRT 좌표·필터·현재 전역/필드·전체 물리/trace가 같다', () => {
    const rows = readRows(path.join(REPO_ROOT, 'cpppj/tests/fixtures/fort-object-save-x86.tsv'));
    // 입력25열만 구성한 뒤 실제 본문을 실행하고 뒤11열의 관측과 대조한다.
    for (const row of rows) {
        assert.equal(row.length, 36); const d = new Devices(); d.initializeObject(row); let completed = true; let result;
        try { result = saveFortObject(STREAM, d.sid, word(row[2]) | 0, word(row[3]) | 0, d.objectHooks()); } catch (error) { if (!(error instanceof FixtureStop)) throw error; completed = false; }
        assert.equal(completed, row[25] === '1', row[0]); if (completed) { assert.ok(result <= 1); assert.equal(result, word(row[26]), row[0]); }
        d.checkStream(row, 27); assert.equal(Buffer.from(d.raw).toString('hex'), row[30], row[0]); assert.equal([...d.globals].join(':'), row[31], row[0]); assert.equal([...d.devices].join(':'), row[32], row[0]); assert.equal(d.trace.join(';'), row[33], row[0]);
        if (completed) assert.equal(row[34], '0', row[0]); else assert.ok(Number(row[34]) < 0, row[0]); assert.ok(Number(row[35]) > 0);
    }
    assert.equal(rows.length, 3294);
});

test('Fort 청크 writer: 원본672행의 실제 marker·고정 countWORD·감김·부분 쓰기/trace가 같다', () => {
    const rows = readRows(path.join(REPO_ROOT, 'cpppj/tests/fixtures/fort-chunk-save-x86.tsv'));
    // marker의 내부 MemStream 진입 관측 한 항목만 C++ 시험과 같이 제외하며 BYTE/필드/진단은 전부 비교한다.
    for (const row of rows) {
        assert.equal(row.length, 19); const d = new Devices(); d.initializeChunk(row); let completed = true;
        try { saveFortChunk(STREAM, word(row[1]) | 0, word(row[2]) | 0, d.chunkHooks()); } catch (error) { if (!(error instanceof FixtureStop)) throw error; completed = false; }
        assert.equal(completed, row[10] === '1', row[0]); d.checkStream(row, 11); assert.equal(d.poolLimit, word(row[14]), row[0]); const trace = row[15].split(';'); assert.equal(trace.filter((item) => item === 'write:1:63').length, 1); assert.equal(d.trace.join(';'), trace.filter((item) => item !== 'write:1:63').join(';'), row[0]);
        if (completed) assert.equal(row[17], '0', row[0]); else assert.ok(Number(row[17]) < 0, row[0]); assert.ok(Number(row[18]) > 0);
    }
    assert.equal(rows.length, 672);
});

test('Fort 좌표 저장: float32 저장/0방향 절삭과 signed64 불확정값·DWORD 감김을 보존한다', () => {
    assert.equal(fortSaveCoordinate(15.999), 15); assert.equal(fortSaveCoordinate(-15.999), -15); assert.equal(fortSaveCoordinate(4294967296), 0); assert.equal(fortSaveCoordinate(-4294967296), 0);
    assert.equal(fortSaveCoordinate(NaN), 0); assert.equal(fortSaveCoordinate(Infinity), 0); assert.equal(fortSaveCoordinate(9223372036854775808), 0); assert.equal(fortSaveCoordinate(2147483648), -2147483648);
    assert.deepEqual(Object.keys(FORT_OBJECT_SAVE_ASSERTION), ['xOffset', 'yOffset', 'owner']);
});

test('Session 객체/청크 저장: 실제 VT/TYPE·섬 조회·공용 stream과 해시 검색기를 사용한다', () => {
    const o = campaignOwners();
    try {
        o.templateStorage.install(1, { sections: Array.from({ length: SECTION_NAMES.length }, () => new Uint8Array(0)) }, 'object'); const sid = o.world.create(85); const raw = o.core.pool.allocated(sid); put(raw, 14, floatToBits(2), 4); put(raw, 18, floatToBits(3), 4); raw[34] = 1; raw[40] &= ~9;
        o.objectState.scope = 2; o.objectState.version = 1; const stream = o.templateStorage.createSection(1, 'Chaff'); const hooks = makeCampaignFortObjectWriter(o, stream); const seed = o.random.state; const before = o.core.pool.bytes();
        assert.equal(saveFortObject(1, sid, 0, 0, hooks), 1); assert.deepEqual([...fortSection(o.templateStorage.template(1), 'Chaff').subarray(0, 2)], [0x23, 85]); assert.equal(hooks.global('daisType'), o.state.runeState.readDaisType()); assert.throws(() => hooks.global('unknown'), /분배/);
        assert.deepEqual(o.core.pool.bytes(), before); assert.equal(o.random.state, seed); assert.equal(o.loaded, false);
        const chunk = o.templateStorage.createSection(1, 'Terr00'); o.writeFortObjectChunkBody(chunk, 0, 0); assert.deepEqual([...fortSection(o.templateStorage.template(1), 'Terr00')], [99, 0, 0]); assert.throws(() => o.writeFortObjectChunk(chunk, 0, 0), /설치 전/);
        // 실제 Hash의 명시 후보 입력이다. World Pop/미션 적재 성공을 가장하지 않고 같은 finder의 비어 있지 않은 결선을 검사한다.
        assert.equal(o.core.hash.bucket(1, 2, 3), 0); o.core.hash.setBucket(1, 2, 3, sid); const occupied = o.templateStorage.createSection(1, 'Terr01'); o.writeFortObjectChunkBody(occupied, 0, 0);
        assert.deepEqual([...fortSection(o.templateStorage.template(1), 'Terr01').subarray(0, 5)], [99, 1, 0, 0x23, 85]); assert.equal(o.core.hash.bucket(1, 2, 3), sid); assert.deepEqual(o.core.pool.bytes(), before); o.core.hash.setBucket(1, 2, 3, 0);
        o.templateStorage.createSection(1, 'Terr00'); assert.throws(() => o.writeFortObjectChunkBody(chunk, 0, 0), /수명|참조|노드|유효/);
    } finally { o.core.hash.setBucket(1, 2, 3, 0); o.dispose(); }
    assert.throws(() => o.writeFortObjectChunkBody({}, 0, 0), /반납한 Session/);
});
