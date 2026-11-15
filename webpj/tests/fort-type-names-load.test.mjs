// 원본TypeNames664본문/38cleanup과 현재 경계·부분 쓰기를 실제 이식 본문에 대조한다. 기대열을 입력 구성에 쓰지 않는다. (WB04-E573)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, get, put } from './raw-support.mjs';
import { loadFortTypeNames, unwindFortTypeNamesLoad } from '../www/core/raw-fort-type-names-load.js';

// 원문 실행기의 명시 주소 토큰·물리 폭·이름 입력이다. 호스트 주소를 역참조하지 않는다.
const DATA = [0x12001000, 0x12002000], BLOCKS = [0, 0x12004000, 0x12004200, 0x12004400], BUFFER_BYTES = 1088;
const NAMES = ['Alpha', 'Beta', '', '\xffA\x80\x7f', 'Alpha', 'LongNameWithMoreThanTwentyCharacters123', 'Case', 'case'];
// 저장한 원본 관측은 두 x87 제어에서 같은 입력을 각각 보존한다.
const ROWS = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'fort-type-names-load-x86.tsv'));
// 물리 바이트 전체를 NUL 뒤까지 관측한다.
const hex = (bytes) => Buffer.from(bytes).toString('hex');
// 원본에 공급했던 독립 저장hash 입력이다. 제품 hash로 기대값을 만들지 않는다.
function hashes(mode) { return mode === 0 ? [0x68706ca2, 0x61746542, 0, 0x7e8040ff] : mode === 1 ? [0x68706ca2, 0x68706ca2, 0x61746542, 0] : [0xffffffff, 0x12345678, 0x80000000, 0x7fffffff]; }
// 실제 이름 입력 바이트와 첫NUL만 쓰고 뒤 패딩은 보존한다.
function inputName(bytes, text) { assert.ok(text.length < bytes.length); for (let at = 0; at < text.length; at += 1) bytes[at] = text.charCodeAt(at); bytes[text.length] = 0; }
// 명시 외부 중단을 제품의 연결/물리 오류와 구별한다.
class StageStop extends Error { constructor(stage) { super(stage); this.stage = stage; } }

// 원본 입력/경계만 공급하고 zero/hash/비교/변환 결과는 제품 본문이 계산한다.
class TypesDevices {
    // 현재 전역과 경계 횟수·trace·물리 읽기 단계다.
    slot; selected; depth; conversion; allocated; count; mutation; trace = []; physical = ''; selects = 0; asserts = 0; allocations = 0; records = 0; buffersRead = 0; conversionAccesses = 0;
    // 같은 물리의 header·stream·data·세 변환표와 전체 TYPE 이름 입력이다.
    headers = new Uint8Array(180); streams = [new Uint8Array(16), new Uint8Array(16)]; data = [new Uint8Array(BUFFER_BYTES), new Uint8Array(BUFFER_BYTES)]; blocks = Array.from({ length: 3 }, () => new Uint8Array(255)); names = Array.from({ length: 262 }, () => new Uint8Array(128));
    // 입력13칸만 사용한다. 원본 완료/기대 raw/trace는 읽지 않는다.
    constructor(input) {
        const f = input.split(','); assert.equal(f.length, 13); const [slot, manager, selected, present, end, position, stored, mode, count, conversion, allocated, depth] = f.slice(0, 12).map(Number);
        Object.assign(this, { slot, selected, count, depth, conversion: BLOCKS[conversion / 11], allocated: BLOCKS[allocated / 11], mutation: f[12] });
        // 대상 외 header 바이트도 같은 원문 패턴을 설치한다.
        for (let at = 0; at < this.headers.length; at += 1) this.headers[at] = 0x70 + at; put(this.headers, slot * 20, manager);
        // 서로 다른 두 stream/data를 명시 입력과 같은 배치로 준비한다.
        for (let index = 0; index < 2; index += 1) {
            const fields = this.streams[index], bytes = this.data[index]; put(fields, 0, present ? DATA[index] : 0); put(fields, 4, position); put(fields, 8, end); put(fields, 12, BUFFER_BYTES);
            // count/hash 바깥 주변 패딩도 원본과 같다.
            for (let at = 0; at < bytes.length; at += 1) bytes[at] = 0xa0 + at + index * 7;
            if (position < BUFFER_BYTES) { bytes[position] = stored; for (let number = 0; number < stored; number += 1) { const target = position + 1 + number * 4; if (target + 4 <= BUFFER_BYTES) put(bytes, target, hashes(mode)[(number + index) % 4]); } }
        }
        // 초기 변환표와 TYPE 이름의 미작성 주변 바이트를 비영으로 구별한다.
        for (const [index, block] of this.blocks.entries()) for (let at = 0; at < block.length; at += 1) block[at] = 0x60 + (index + 1) * 11 + at;
        for (const [index, name] of this.names.entries()) { name.fill(0xa7); inputName(name, NAMES[index % NAMES.length]); }
    }
    // 같은 현재 전역의 getter/setter를 매번 빌린다.
    globals() { const owner = this; return { get loadingDepth() { return owner.depth; }, set loadingDepth(value) { owner.depth = value >>> 0; }, get conversion() { return owner.conversion; }, set conversion(value) { owner.conversion = value >>> 0; } }; }
    // 실제 하위가 선택한 시점의 명시 변경만 수행한다.
    mutate(boundary) {
        const stream = this.streams[this.selected], mutation = this.mutation;
        if (mutation === 'select-stream1' && boundary === 'select') this.selected = 1;
        else if (mutation === 'select-manager0' && boundary === 'select') put(this.headers, this.slot * 20, 0);
        else if (mutation === `${boundary}-depth7`) this.depth = 7;
        else if (mutation === `${boundary}-depthffffffff`) this.depth = 0xffffffff;
        else if (mutation === `${boundary}-conversion0`) this.conversion = 0;
        else if (mutation === `${boundary}-conversion11`) this.conversion = BLOCKS[1];
        else if (mutation === `${boundary}-conversion22`) this.conversion = BLOCKS[2];
        else if (mutation === `${boundary}-data1`) put(stream, 0, DATA[1]);
        else if (mutation === `${boundary}-pos9`) put(stream, 4, 9);
        else if (mutation === `${boundary}-endffffffff`) put(stream, 8, 0xffffffff);
        else if (mutation === `${boundary}-count0`) this.count = 0;
        else if (mutation === `${boundary}-count260`) this.count = 260;
        else if (mutation === 'record1-name' && boundary === 'record1') inputName(this.names[0], 'Beta');
        else if (mutation === 'record1-bufferhash' && boundary === 'record1') put(this.data[0], 1, hashes(0)[1]);
        else if (mutation === `${boundary}-stop`) throw new StageStop(boundary);
    }
    // 원문 Select/할당/TYPE/진단의 선택 순서를 기록한다.
    hooks() { return {
        manager: (slot) => get(this.headers, slot * 20), select: (manager, name, writable) => { assert.equal(name, 'TypeNames'); assert.equal(writable, 1); this.selects += 1; this.trace.push(`S:${manager}:TypeNames:1`); this.mutate('select'); return this.selected; }, stream: (token) => this.streams[token],
        buffer: (token) => { this.buffersRead += 1; this.physical = this.buffersRead === 1 ? 'count-read' : 'hash-read'; const index = DATA.indexOf(token); if (index === -1) throw new RangeError('현재 data 주소'); return this.data[index]; },
        allocate: (bytes) => { assert.equal(bytes, 255); this.allocations += 1; this.trace.push('N:255'); this.mutate('allocate'); return this.allocated; },
        conversionBuffer: (token) => { this.conversionAccesses += 1; if (!token) throw new StageStop(this.conversionAccesses === 1 ? 'allocation0' : 'conversion-write'); const index = BLOCKS.indexOf(token); if (index < 1) throw new RangeError('현재 conversion 주소'); return this.blocks[index - 1]; },
        typeCount: () => { this.trace.push(`C:${this.count}`); return this.count; },
        typeName: (index) => { const name = this.names[index], end = name.indexOf(0); assert.notEqual(end, -1); this.records += 1; this.trace.push(`R:${index}:${hex(name.subarray(0, end))}`); this.mutate(`record${this.records}`); return this.names[index]; },
        assertion: (line) => { assert.equal(line, 608); this.asserts += 1; this.trace.push('A:608'); this.mutate('assert'); }, loadingDepthChanged: (value) => this.trace.push(`L:${value}`),
    }; }
    // 현재 전역/경계 개수만 관측한다.
    state() { const index = BLOCKS.indexOf(this.conversion); assert.notEqual(index, -1); return [this.depth, index * 11, this.count, this.selected, this.selects, this.asserts, this.allocations, this.records].join(','); }
    // CPU/하위가 바꿀 수 있는 전체 물리를 원본 순서대로 관측한다.
    raw() { return [this.headers, ...this.streams, ...this.data, ...this.blocks, this.names[0], this.names[1]].map(hex).join(':'); }
}

// 원본의 getter 경로와 같은 물리 access 동기 경로를 각각 대조한다.
for (const access of [false, true]) test(`FortTypeNames: ${access ? '동기 access' : 'getter'} 원본664본문·38별도cleanup의 현재 전역·전체 raw·경계 순서가 같다`, () => {
    assert.equal(ROWS.length, 664); const unique = new Set(); let stopped = 0;
    // 각 저장 입력만 재생하고 고정된 원본 기대열을 마지막에 비교한다.
    for (const row of ROWS) {
        assert.equal(row.length, 7); const key = row[0] + '/' + row[1]; assert.equal(unique.has(key), false); unique.add(key);
        const scene = new TypesDevices(row[1]), hooks = scene.hooks(); let completion = '1';
        if (access) { const stream = hooks.stream, buffer = hooks.buffer; hooks.access = (token, operation) => operation(stream(token), { buffer }); hooks.stream = hooks.buffer = () => assert.fail('access 경로가 legacy getter를 부르면 안 된다'); }
        try { loadFortTypeNames(scene.slot, scene.globals(), hooks); } catch (error) { if (error instanceof StageStop) completion = error.stage; else if (error instanceof RangeError && scene.physical) completion = scene.physical; else throw error; }
        const state = scene.state(), raw = scene.raw(), trace = scene.trace.join(';'); let cleanup = '-';
        if (completion !== '1') { stopped += 1; scene.trace.length = 0; unwindFortTypeNamesLoad(scene.globals(), hooks); cleanup = scene.state() + '/' + scene.trace.join(';'); assert.equal(scene.raw(), raw); }
        assert.deepEqual([completion, state, raw, trace, cleanup], row.slice(2), row[1]);
    }
    assert.equal(stopped, 38);
});
test('FortTypeNames: 빈stream은 미도달 효과를 요구하지 않고 기존 변환은 해제 없이0으로 덮는다', () => {
    const scene = new TypesDevices('1,11,0,0,1,0,4,0,7,11,22,1,none'), hooks = scene.hooks(), before = scene.blocks.map(hex);
    // 빈 경로가 선택하지 않는 효과는 전부 제거한다.
    for (const name of ['allocate', 'typeName', 'typeCount', 'assertion', 'buffer', 'conversionBuffer']) hooks[name] = null;
    loadFortTypeNames(1, scene.globals(), hooks); assert.equal(scene.conversion, 0); assert.equal(scene.depth, 1); assert.equal(scene.allocations, 0); assert.deepEqual(scene.blocks.map(hex), before);
});
test('FortTypeNames: 누락 진단·부분zero·NUL 없는 이름의 접두를 보존하고 별도 unwind만 깊이를 반납한다', () => {
    const missing = new TypesDevices('1,11,0,1,1,0,4,0,7,11,22,1,none'), hooks = missing.hooks(); hooks.assertion = null;
    assert.throws(() => loadFortTypeNames(1, missing.globals(), hooks), /미연결/); assert.deepEqual([missing.depth, get(missing.streams[0], 4), missing.allocations, missing.conversion], [2, 1, 0, BLOCKS[1]]); unwindFortTypeNamesLoad(missing.globals(), hooks); assert.equal(missing.depth, 1);
    // 4바이트 쓰기·뒤WORD·마지막BYTE의 서로 다른 실패 지점을 확인한다.
    for (const size of [1, 251, 252, 253, 254]) {
        const scene = new TypesDevices('1,11,0,1,1,0,4,0,7,0,22,1,none'), effects = scene.hooks(), before = scene.blocks[1].slice(); effects.conversionBuffer = () => scene.blocks[1].subarray(0, size);
        assert.throws(() => loadFortTypeNames(1, scene.globals(), effects), RangeError); const written = size >= 254 ? 254 : Math.floor(Math.min(size, 252) / 4) * 4;
        assert.ok(scene.blocks[1].subarray(0, written).every((x) => x === 0)); assert.deepEqual(scene.blocks[1].subarray(written), before.subarray(written)); assert.equal(scene.depth, 2); unwindFortTypeNamesLoad(scene.globals(), effects); assert.equal(scene.depth, 1);
    }
    const noNul = new TypesDevices('1,11,0,1,1,0,4,0,1,0,22,1,none'), effects = noNul.hooks(); effects.typeName = () => Uint8Array.of(65, 66);
    assert.throws(() => loadFortTypeNames(1, noNul.globals(), effects), /NUL/); assert.equal(noNul.depth, 2); assert.equal(noNul.conversion, BLOCKS[2]); unwindFortTypeNamesLoad(noNul.globals(), effects); assert.equal(noNul.depth, 1);
});
