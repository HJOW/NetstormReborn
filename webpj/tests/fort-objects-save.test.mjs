// 원본 영역930행·전체 Objects714행과 같은 실제 Template/청크/Temp 결선을 대조한다(WB04-E631).
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, get, put } from './raw-support.mjs';
import { saveFortTerritoryChunks, saveFortChaffChunks } from '../www/core/raw-fort-region-save.js';
import { saveFortObjects } from '../www/core/raw-fort-objects-save.js';
import { campaignOwners } from './campaign-session-support.mjs';
import { fortSection, SECTION_NAMES } from '../www/assets/fort.js';

// 원문 emu_stop만 구별하고 다른 장치/물리 오류는 검사 실패로 전파한다.
class FixtureStop extends Error {}
// 독립 영역 관측의 현재 크기/scope와 고정 인자·명시 외부 응답이다.
class RegionDevices {
    width; height; scope; owner; region; regions; decisions; effects; calls = new Map(); trace = [];
    // 앞10열 입력만 해석한다.
    constructor(row) { this.width = Number(row[2]); this.height = Number(row[3]); this.scope = Number(row[4]); this.owner = Number(row[5]); this.region = Number(row[6]); this.regions = Buffer.from(row[7], 'hex'); this.decisions = row[8].split(',').map(Number); this.effects = row[9] === '-' ? [] : row[9].split(';').map((text) => text.split('/')); }
    // 캡처한 반환값은 바꾸지 않고 같은 외부 CALL의 변경만 적용한다.
    boundary(kind, text) {
        this.trace.push(text); const count = (this.calls.get(kind) ?? 0) + 1; this.calls.set(kind, count); const event = kind + count;
        // 순회/판정 자체를 대신 구현하지 않는 명시 전역 변경이다.
        for (const [at, field, input] of this.effects) { if (at !== event) continue; const value = Number(input); if (field === 'width') this.width = value | 0; else if (field === 'height') this.height = value | 0; else if (field === 'scope') this.scope = value >>> 0; else { assert.equal(field, 'stop'); assert.equal(value, 1); throw new FixtureStop(); } }
    }
    // C++ DWORD 참조와 같은 현재 scope다.
    scopeRef() { const d = this; return { get value() { return d.scope; }, set value(value) { d.scope = value; } }; }
    // getter는 효과 전 반환값을 캡처하며 각 위치의 실제 trace를 기록한다.
    hooks() {
        return {
            chunksX: () => { const value = this.width; this.boundary('x', 'X:' + value + ':' + this.scope); return value; }, chunksY: () => { const value = this.height; this.boundary('y', 'Y:' + value + ':' + this.scope); return value; },
            chunkRegion: (x, y) => { assert.ok(x >= 0 && x < 8 && y >= 0 && y < 8); const byte = this.regions[(y * 8 + x) % this.regions.length]; this.boundary('cell', 'cell:' + x + ':' + y + ':' + byte + ':' + this.scope); return byte; },
            allowed: (byte, owner, region) => { const answer = this.decisions[(this.calls.get('allow') ?? 0) % this.decisions.length]; this.boundary('allow', 'allow:' + byte + ':' + owner + ':' + region + ':' + answer + ':' + this.scope); return answer; },
            chunk: (stream, x, y) => this.boundary('chunk', 'chunk:' + stream + ':' + x + ':' + y + ':' + this.scope),
        };
    }
}

// 최초 Objects 관측의 불투명 버퍼 기준과 진단 행이다.
const BUFFER_BASE = 0x12008000; const LINES = { MissingTemplate: 957, NotMine: 958, WriteCapacity: 36 };
// 부모의 동작을 계산하지 않는 현재 manager/21개 원시 stream·하위 응답이다.
class ObjectsDevices {
    slot; manager; mine; fort; pattern; position; end; capacity; mutation; region = -1; version = 0xaabbccdd; active = 0; temps = 0;
    nodes = Array.from({ length: 21 }, () => new Uint8Array(16)); output = Array.from({ length: 21 }, () => new Uint8Array(64).fill(0xcc)); currents = new Map([[0, 90], [11, 91], [22, 92]]); calls = new Map(); trace = [];
    // 미사용 노드/주변 CC 바이트도 최초 입력처럼 준비한다.
    constructor(input) {
        const f = input.split(','); assert.equal(f.length, 9); [this.slot, this.manager, this.mine, this.fort, this.pattern, this.position, this.end, this.capacity] = f.slice(0, 8).map(Number); this.mutation = f[8];
        // 같은21개 buffer 토큰과 초기 용량만 정의하며 제품 쓰기를 선행하지 않는다.
        for (let token = 1; token <= 21; token++) { put(this.nodes[token - 1], 0, BUFFER_BASE + token * 64, 4); put(this.nodes[token - 1], 12, 64, 4); }
    }
    // const owner/mutable version 참조와 마지막 현재 fort 조회다.
    globals() { const d = this; return { get templateOwner() { return d.mine; }, get version() { return d.version; }, set version(value) { d.version = value; }, fort: () => d.fort }; }
    // 호출 종류별1기준 횟수와 실제 순서만 기록한다.
    record(kind, text) { this.trace.push(text); const count = (this.calls.get(kind) ?? 0) + 1; this.calls.set(kind, count); return count; }
    // 독립 관측에 공급한 명시 callback 변경/중단만 실행한다.
    mutate(boundary) {
        if (this.mutation === 'assert-manager22' && boundary === 'assert') this.manager = 22;
        else if (this.mutation === boundary + '-manager22') this.manager = 22;
        else if (this.mutation === 'temp1-version171' && boundary === 'temp1') this.version = 0x123456ab;
        else if (this.mutation === 'territory1-version127' && boundary === 'territory1') this.version = 0xff00007f;
        else if (this.mutation === 'territory20-fort1' && boundary === 'territory20') this.fort = 1;
        else if (this.mutation === 'rename1-fort0' && boundary === 'rename1') this.fort = 0;
        else if (this.mutation === 'chaff-version255' && boundary === 'chaff') this.version = 0x765432ff;
        else if (this.mutation === 'capacity-current' && boundary === 'capacity') { const node = this.nodes[this.active - 1]; put(node, 4, 5, 4); put(node, 8, 2, 4); put(node, 12, 64, 4); this.version = 0x987654ee; }
        else if (this.mutation === boundary + '-stop') throw new FixtureStop();
    }
    // 영역 응답은 명시1BYTE만 쓴다. 부모 version 쓰기는 제품 본문에 맡긴다.
    append(token, value) { const node = this.nodes[token - 1]; const pointer = get(node, 0, 4); const position = get(node, 4, 4); const target = (pointer - BUFFER_BASE) / 64; assert.ok(pointer >= BUFFER_BASE + 64 && target <= 21 && position < 64); this.output[target - 1][position] = value; put(node, 4, position + 1, 4); if ((get(node, 8, 4) | 0) < position + 1) put(node, 8, position + 1, 4); }
    // DataManager 알고리즘을 복제하지 않고 원문 부모가 받은 하위 응답/부작용만 제공한다.
    hooks() {
        return {
            manager: (slot) => { assert.equal(slot, this.slot); return this.manager; }, assertion: (kind) => { const line = LINES[kind]; assert.ok(line); this.record('assert', 'assert:' + line); this.mutate(line === 36 ? 'capacity' : 'assert'); },
            find: (manager, name) => { const current = Number(name.slice(4)); const found = this.pattern === 1 || (this.pattern === 2 && current % 2 === 0); const index = this.record('find', 'find:' + manager + ':' + name); this.currents.set(manager, found ? current + 1 : 0); this.mutate('find' + index); return found ? 1 : 0; },
            create: (manager, name) => { const index = this.record('create', 'create:' + manager + ':' + name); let token; if (name === 'Temp') { this.region++; token = this.region + 1; this.temps++; } else token = name === 'Chaff' ? 21 : Number(name.slice(4)) + 1; this.currents.set(manager, token); if (name === 'Temp' || name === 'Chaff') { const node = this.nodes[token - 1]; put(node, 0, BUFFER_BASE + token * 64, 4); put(node, 4, this.position, 4); put(node, 8, this.end, 4); put(node, 12, this.capacity, 4); } this.mutate('create' + index); if (name === 'Temp') this.mutate('temp' + this.temps); else if (name === 'Chaff') this.mutate('chaff'); return token; },
            select: (manager, name, writable) => { assert.equal(name, 'Temp'); assert.equal(writable, 1); const token = this.region + 1; const index = this.record('select', 'select:' + manager + ':Temp:1'); this.currents.set(manager, token); if (this.pattern === 2) { if (this.region % 3 === 0) put(this.nodes[token - 1], 0, 0, 4); else if (this.region % 3 === 1) put(this.nodes[token - 1], 8, 0, 4); } this.mutate('select' + index); return token; },
            rename: (manager, destination, source) => { assert.equal(source, 'Temp'); this.mutate('rename' + this.record('rename', 'rename:' + manager + ':' + destination + ':' + source)); },
            territory: (stream, owner, region) => { const index = this.record('territory', 'territory:' + stream + ':' + owner + ':' + region); this.append(stream, 0x40 + region); this.mutate('territory' + index); }, chaff: (stream) => { this.record('chaffbody', 'chaff:' + stream); this.append(stream, 0x7f); },
            access: (token, operation) => { this.active = token; operation(this.nodes[token - 1], { buffer: (pointer) => { assert.ok(pointer >= BUFFER_BASE + 64 && (pointer - BUFFER_BASE) % 64 === 0); return this.output[(pointer - BUFFER_BASE) / 64 - 1]; } }); },
        };
    }
    // 현재7전역과manager 선택을 최초 원문 순서로 표현한다.
    state() { return [this.manager, this.mine, this.version, this.fort, this.currents.get(0), this.currents.get(11), this.currents.get(22)].join(','); }
    // 미사용 노드/주변64BYTE까지 전부 비교한다.
    raw() { return this.nodes.map((node, index) => Buffer.from(node).toString('hex') + Buffer.from(this.output[index]).toString('hex')).join(':'); }
}

test('Fort 영역 저장: 원본930행의 height/width 재조회·signed BYTE·scope·현재 효과/중단이 같다', () => {
    const rows = readRows(path.join(REPO_ROOT, 'cpppj/tests/fixtures/fort-region-save-x86.tsv'));
    // 기대 뒤8열은 실제 두 순회 중 해당 몸체 실행이 끝난 뒤에만 읽는다.
    for (const row of rows) { assert.equal(row.length, 18); const d = new RegionDevices(row); let completed = true; try { if (row[1] === 'territory') saveFortTerritoryChunks(0x12345678, d.owner, d.region, d.scopeRef(), d.hooks()); else { assert.equal(row[1], 'chaff'); saveFortChaffChunks(0x12345678, d.scopeRef(), d.hooks()); } } catch (error) { if (!(error instanceof FixtureStop)) throw error; completed = false; } assert.equal(completed, row[10] === '1', row[0]); assert.deepEqual([d.width, d.height, d.scope], row.slice(11, 14).map(Number), row[0]); assert.equal(d.trace.join(';') || '-', row[14], row[0]); if (completed) assert.equal(row[15], '0'); else assert.ok(Number(row[15]) < 0); assert.ok(Number(row[16]) > 0); assert.ok(Number(row[17]) <= 1); if (row[1] === 'chaff') assert.equal(d.calls.get('allow') ?? 0, 0); }
    assert.equal(rows.length, 930);
});

test('Fort 영역 저장: 빈 지도/누락의 최초 scope와 Chaff의 실제 셀 조회 변경을 보존한다', () => {
    const scope = { value: 77 }; let rows = 0; const hooks = { chunksY: () => { assert.equal(scope.value, 77); return 0; } }; saveFortTerritoryChunks(1, 1, 2, scope, hooks); assert.equal(scope.value, 1); scope.value = 77; saveFortChaffChunks(1, scope, hooks); assert.equal(scope.value, 2);
    scope.value = 77; assert.throws(() => saveFortChaffChunks(1, scope, {}), /실제 경계 누락/); assert.equal(scope.value, 77); assert.throws(() => saveFortTerritoryChunks(1, 1, 2, scope, { chunksY: () => 1 }), /실제 경계 누락/); assert.equal(scope.value, 1);
    saveFortChaffChunks(1, scope, { chunksX: () => 0, chunksY: () => { rows++; return 2; } }); assert.equal(rows, 3);
    let width = 2; let queries = 0; let writes = 0; saveFortChaffChunks(1, scope, { chunksY: () => 1, chunksX: () => width, chunkRegion: (x, y) => { assert.deepEqual([x, y, scope.value], [0, 0, 2]); scope.value = 9; width = 0; queries++; return 255; }, allowed: () => { throw new Error('미선택 판정'); }, chunk: (stream, x, y) => { assert.deepEqual([stream, x, y, scope.value], [1, 0, 0, 9]); writes++; } }); assert.deepEqual([queries, writes, scope.value], [1, 1, 9]);
});

test('Fort 전체 Objects: 원본714행의 현재 manager/version·21개 stream/물리·Temp/이름 교체/부분 중단이 같다', () => {
    const rows = readRows(path.join(REPO_ROOT, 'cpppj/tests/fixtures/fort-objects-parent-x86.tsv')); const unique = new Set(); let stopped = 0;
    // 최초357개 입력의 두 정밀도를 중복 없이 재생하며 기대는 실행 뒤 비교한다.
    for (const row of rows) { assert.equal(row.length, 6); const key = row[0] + ':' + row[1]; assert.equal(unique.has(key), false); unique.add(key); const d = new ObjectsDevices(row[1]); let completed = true; try { saveFortObjects(d.slot, d.globals(), d.hooks()); } catch (error) { if (!(error instanceof FixtureStop)) throw error; completed = false; stopped++; } assert.equal(completed, row[2] === '1', key); assert.equal(d.state(), row[3], key); assert.equal(d.raw(), row[4], key); assert.equal(d.trace.join(';'), row[5], key); }
    assert.equal(rows.length, 714); assert.equal(stopped, 48);
});

test('Fort 전체 Objects: 미선택 Chaff는 요구하지 않고 find 누락은 version2 쓰기 뒤 실패한다', () => {
    const d = new ObjectsDevices('1,11,1,0,0,0,0,64,none'); const hooks = d.hooks(); delete hooks.chaff; saveFortObjects(1, d.globals(), hooks); assert.equal(d.temps, 20); assert.equal(d.calls.get('chaffbody') ?? 0, 0);
    const missing = new ObjectsDevices('1,11,1,0,0,0,0,64,none'); const absent = missing.hooks(); delete absent.find; assert.throws(() => saveFortObjects(1, missing.globals(), absent), /실제 경계 미연결/); assert.equal(missing.version, 2); assert.equal(missing.trace.join(';'), '');
});

test('Session 전체 Objects: 실제 Template의20영역·Temp 이름 교체·현재 scope/version과 조건부 Chaff를 연결한다', () => {
    const o = campaignOwners();
    try {
        o.templateStorage.install(1, { sections: Array.from({ length: SECTION_NAMES.length }, () => new Uint8Array(0)) }, 'objects'); o.objectState.chunksX = 2; o.objectState.chunksY = 1;
        const island = o.islands.add(1, 0); o.chunks.at(0, 0).island = island; const before = o.core.pool.bytes(); const seed = o.random.state;
        assert.throws(() => o.snapshotFortObjects(1), /설치 전/); o.writeFortObjects(1); assert.deepEqual([...fortSection(o.templateStorage.template(1), 'Terr00')], [2, 99, 0, 0]);
        // 표준20개 영역 중 첫 영역만 실제 섬의 선택 청크를 포함한다.
        for (let region = 1; region < 20; region++) assert.deepEqual([...fortSection(o.templateStorage.template(1), 'Terr' + String(region).padStart(2, '0'))], [2]);
        assert.deepEqual([o.objectState.scope, o.objectState.version], [1, 2]); assert.equal(o.templateStorage.sectionOrder(1).includes('Temp'), false); assert.equal(fortSection(o.templateStorage.template(1), 'Chaff').length, 0);
        o.core.ownerMode.fort = true; o.writeFortObjects(1); assert.deepEqual([...fortSection(o.templateStorage.template(1), 'Chaff')], [2, 99, 0, 0, 99, 0, 0]); assert.deepEqual([o.objectState.scope, o.objectState.version], [2, 2]); assert.deepEqual(o.core.pool.bytes(), before); assert.equal(o.random.state, seed); assert.equal(o.loaded, false); assert.throws(() => o.writeFortObjects(9), /슬롯 계약/);
    } finally { o.dispose(); }
    assert.throws(() => o.writeFortObjects(1), /반납한 Session/);
});
