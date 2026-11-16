// Territory 부모638행·검색20162행·seed188행과 실제 Session의 현재 지도/Template 결선을 대조한다(WB04-E632).
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, get, put } from './raw-support.mjs';
import { snapshotFortTerritory, findFortTerritoryIsland, findAnyFortTerritoryIsland, fortTerritoryIslandRecord, seedFortTerritoryRecord } from '../www/core/raw-fort-territory-save.js';
import { campaignOwners } from './campaign-session-support.mjs';
import { fortSection, SECTION_NAMES } from '../www/assets/fort.js';
import { ChunkMap } from '../www/assets/territory.js';

// 원문 emu_stop에 대응하는 외부 중단만 구별한다.
class FixtureStop extends Error {}
// 관측 입력의 raw BYTE를 복원한다. 기대 열은 호출 뒤에만 사용한다.
function bytes(hex) { return Uint8Array.from(Buffer.from(hex, 'hex')); }
// 전체 물리를 소문자 hex로 비교한다.
function hex(raw) { return Buffer.from(raw).toString('hex'); }
// 부모의 명시 외부 응답/변경과 두 실제 stream만 소유한다. 부모의 선택/쓰기는 제품 함수가 결정한다.
class Devices {
    // 고정 슬롯/현재 manager/current·현재 전역과 실제 물리다.
    slot; manager; currents; globals; streams = [new Uint8Array(16), new Uint8Array(16)]; buffers = [new Uint8Array(16384).fill(0xcc), new Uint8Array(16384).fill(0xcc)];
    // 입력 record/살아 있는 로컬 참조·명시 반환표/변경·실제 trace다.
    input; source = new Uint8Array(120); regions = new Map(); effects = []; calls = new Map(); trace = [];
    // 최초 입력15열만 원래 폭으로 배치한다.
    constructor(row) {
        this.slot = Number(row[1]); this.manager = Number(row[2]); this.currents = row.slice(3, 5).map(Number); this.globals = row.slice(5, 9).map(Number);
        // 두 stream의 data만 다르고 pos/end/capacity는 같은 입력이다.
        for (let index = 0; index < 2; index++) { put(this.streams[index], 0, index); put(this.streams[index], 4, Number(row[9])); put(this.streams[index], 8, Number(row[10])); put(this.streams[index], 12, Number(row[11])); }
        if (row[12] !== '-') for (const entry of row[12].split(';')) { const values = entry.split(':').map(Number); this.regions.set(values[0], values.slice(1)); }
        this.input = bytes(row[13]); assert.equal(this.input.length, 120); if (row[14] !== '-') this.effects = row[14].split(';').map((entry) => entry.split('/'));
    }
    // trace를 먼저 기록하고 동일 경계의 효과를 입력 순서대로 실행한다.
    boundary(kind, text) {
        this.trace.push(text); const count = (this.calls.get(kind) ?? 0) + 1; this.calls.set(kind, count); const event = kind + count;
        // 현재 값 변경과 중단만 공급하며 제품 본체의 판단은 여기에 넣지 않는다.
        for (const [at, target, field, raw] of this.effects) {
            if (at !== event) continue; const value = Number(raw) >>> 0;
            if (target === 'global') this.globals[{ special: 0, fort: 1, mine: 2, noRegion: 3 }[field]] = value;
            else if (target === 'header') { assert.equal(field, 'manager'); this.manager = value; }
            else if (target.startsWith('manager')) { assert.equal(field, 'current'); this.currents[Number(target.slice(7)) === 11 ? 0 : 1] = value; }
            else if (target.startsWith('stream')) put(this.streams[Number(target.slice(6))], { data: 0, position: 4, end: 8, capacity: 12 }[field], value);
            else if (target === 'source') this.source[Number(field.slice(4))] = value;
            else { assert.deepEqual([target, field, value], ['control', 'stop', 1]); throw new FixtureStop(); }
        }
    }
    // 명시 loader/지도/생성/진단만 연결하고 지역 선택과 인라인120BYTE 쓰기는 제품 함수가 수행한다.
    hooks() {
        return {
            load: (slot, output) => { this.source = output; output.set(this.input); this.boundary('get', 'get:' + slot); }, special: () => this.globals[0], noRegion: () => this.globals[3] | 0,
            island: (owner, region) => { const answer = this.regions.get(region)?.[0] ?? (this.globals[3] | 0); this.boundary('region', `region:${this.globals[1]}:${owner}:${region}`); return answer; },
            anyIsland: (region) => { const answer = this.regions.get(region)?.[1] ?? (this.globals[3] | 0); this.boundary('any', 'any:' + region); return answer; },
            record: (island) => { const result = Uint8Array.from({ length: 6 }, (_, index) => island * 17 + index * 29 + 7); this.boundary('record', 'record:' + island); return result; },
            seed: (record, index) => { record[0] = (record[0] & 0xc0) | (index & 0x3f); record[1] = (record[1] & 0xf7) | 3; record[3] = (index * 37 + 11) % 254 + 1; this.boundary('seed', 'seed:' + index); },
            manager: (slot) => { assert.equal(slot, this.slot); return this.manager; }, create: (selected) => this.boundary('create', `create:${selected}:Territory`),
            current: (selected) => { assert.ok(selected === 11 || selected === 22); return this.currents[selected === 11 ? 0 : 1]; },
            assertion: (kind) => this.boundary('assert', 'assert:' + (kind === 'MissingManager' ? 521 : 36)), access: (token, operation) => operation(this.streams[token], { buffer: (pointer) => this.buffers[pointer] }),
        };
    }
    // 현재 두 stream의 data/pos/end/capacity를 원문 순서로 출력한다.
    streamOutput() { return this.streams.map((stream) => [0, 4, 8, 12].map((at) => get(stream, at)).join(':')).join(';'); }
}

test('Fort Territory 부모: 원본638행의 현재 모드/region·manager/current·두 버퍼와120BYTE·중단이 같다', () => {
    const rows = readRows(path.join(REPO_ROOT, 'cpppj/tests/fixtures/fort-territory-save-x86.tsv')); let stopped = 0;
    // 입력15열에서 실행한 뒤 출력11열 전체와 비교한다.
    for (const row of rows) {
        assert.equal(row.length, 26); const d = new Devices(row); let completed = true; try { snapshotFortTerritory(d.slot, d.hooks()); } catch (error) { if (!(error instanceof FixtureStop)) throw error; completed = false; stopped++; }
        assert.equal(completed, row[15] === '1', row[0]); assert.equal(d.manager, Number(row[16]), row[0]); assert.equal(d.currents.join(':'), row[17], row[0]); assert.equal(d.globals.join(':'), row[18], row[0]); assert.equal(d.streamOutput(), row[19], row[0]);
        const end = Math.max(get(d.streams[0], 8), get(d.streams[1], 8)); assert.equal(hex(d.buffers[0].subarray(0, end)), row[20], row[0]); assert.equal(hex(d.buffers[1].subarray(0, end)), row[21], row[0]); assert.equal(hex(d.source), row[22], row[0]); assert.equal(d.trace.join(';'), row[23], row[0]); assert.ok(completed ? Number(row[24]) === 0 : Number(row[24]) < 0, row[0]); assert.ok(Number(row[25]) > 0);
    }
    assert.equal(rows.length, 638); assert.ok(stopped > 0);
});

test('Fort Territory 검색: 원본20162행의 signed BYTE/현재·원래 owner·Fort 분기와 첫 존재 일치가 같다', () => {
    const rows = readRows(path.join(REPO_ROOT, 'cpppj/tests/fixtures/fort-territory-query-x86.tsv'));
    // 각 입력의 같은3204BYTE에 지정한 기록만 배치하며 기대 결과로 반환표를 만들지 않는다.
    for (const row of rows) {
        assert.equal(row.length, 11); const records = new Uint8Array(3204); put(records, 0, Number(row[5]));
        if (row[6] !== '-') { let index = 0; for (const entry of row[6].split(';')) { const [exists, current, original, region] = entry.split(':').map(Number); const at = 4 + index++ * 32; put(records, at + 4, exists >>> 0); put(records, at + 8, current >>> 0); put(records, at + 13, original & 255, 1); put(records, at + 12, region & 255, 1); } }
        const result = row[1] === 'normal' ? findFortTerritoryIsland(records, row[2] !== '0', Number(row[3]), Number(row[4])) : findAnyFortTerritoryIsland(records, Number(row[4])); assert.equal(result >>> 0, Number(row[7]), row[0]); assert.deepEqual([row[8], row[9]], ['', '0']); assert.ok(Number(row[10]) > 0);
    }
    assert.equal(rows.length, 20162);
});

test('Fort Territory seed: 원본188행의 난수 전 두BYTE·후 byte3·원래 필드/현재 효과와 중단이 같다', () => {
    const rows = readRows(path.join(REPO_ROOT, 'cpppj/tests/fixtures/fort-territory-seed-x86.tsv'));
    // 하위 seed의 독립 관측은 부모 fixture의 명시 반환 장치와 구별한다.
    for (const row of rows) {
        assert.equal(row.length, 10); const record = bytes(row[2]); let trace = ''; let completed = true;
        // 실제 원본에 공급한 난수 반환과 callback 변경만 재생한다.
        const random = (limit) => { trace = 'random:' + limit; if (row[4] !== '-') for (const entry of row[4].split(';')) { const [event, target, field, value] = entry.split('/'); assert.equal(event, 'random1'); if (target === 'control') throw new FixtureStop(); record[Number(field.slice(4))] = Number(value); } return Number(row[3]); };
        try { seedFortTerritoryRecord(record, Number(row[1]), random); } catch (error) { if (!(error instanceof FixtureStop)) throw error; completed = false; }
        assert.equal(completed, row[5] === '1', row[0]); assert.equal(hex(record), row[6], row[0]); assert.equal(trace, row[7], row[0]); assert.ok(completed ? Number(row[8]) === 0 : Number(row[8]) < 0); assert.ok(Number(row[9]) > 0);
    }
    assert.equal(rows.length, 188);
});

test('Fort Territory 지도/물리: y/x 첫 현재 record 사본·signed 섬BYTE와 선택된 누락의 접두를 보존한다', () => {
    const map = new ChunkMap(3, 3); map.at(1, 0).island = 2; map.at(1, 0).record.set([1, 2, 3, 4, 5, 6]); map.at(0, 1).island = 2; map.at(0, 1).record.fill(9);
    const record = fortTerritoryIslandRecord(map, 2); assert.deepEqual([...record], [1, 2, 3, 4, 5, 6]); map.at(1, 0).record[2] = 77; assert.equal(record[2], 3); assert.equal(fortTerritoryIslandRecord(map, 2)[2], 77); assert.deepEqual([...fortTerritoryIslandRecord(map, 99)], [0, 0, 0, 0, 0, 0]);
    map.at(0, 0).island = 255; map.at(0, 0).record.set([9, 8, 7, 6, 5, 4]); assert.deepEqual([...fortTerritoryIslandRecord(map, -1)], [9, 8, 7, 6, 5, 4]);
    const partial = Uint8Array.from([255, 255, 1, 2, 3, 4]); assert.throws(() => seedFortTerritoryRecord(partial, 5), /실제 장치 누락/); assert.deepEqual([...partial], [0xc5, 0xf7, 1, 2, 3, 4]);
    const islands = new Uint8Array(3204); put(islands, 0, 0xffffffff); assert.equal(findAnyFortTerritoryIsland(islands, 0), 127); put(islands, 0, 101); put(islands, 8, 1); assert.equal(findAnyFortTerritoryIsland(islands, 0), 0); islands.fill(0, 4); assert.throws(() => findAnyFortTerritoryIsland(islands, 0), /물리 범위/);
});

test('Session Territory snapshot: 같은 Runtime 읽기·현재 섬/청크·Template 새 노드와 설치/반납 관문을 사용한다', () => {
    const o = campaignOwners();
    try {
        const records = new Uint8Array(120); records.set([0xa0, 1, 0x22, 0x33, 0x44, 0x55]); records.set([0xb0, 2, 0x66, 0x77, 0x88, 0x99], 6); const sections = Array.from({ length: SECTION_NAMES.length }, () => new Uint8Array(0)); sections[SECTION_NAMES.indexOf('Territory')] = records;
        assert.throws(() => o.snapshotFortTerritory(1), /설치 전/); o.connectWorldInstall({ sections }); o.chunks.xLength = 2; o.chunks.yLength = 2; const first = o.islands.add(1, 0); const second = o.islands.add(1, 1); o.chunks.at(1, 0).island = first; o.chunks.at(1, 0).record.set([6, 5, 4, 3, 2, 1]); o.chunks.at(0, 1).island = second; o.chunks.at(0, 1).record.fill(7);
        const seed = o.random.state; const pool = o.core.pool.bytes(); o.writeFortTerritory(1); const saved = fortSection(o.templateStorage.template(1), 'Territory'); assert.deepEqual([...saved.subarray(0, 6)], [6, 5, 4, 3, 2, 1]); assert.deepEqual([...saved.subarray(6, 12)], [0xb0, 2, 0x66, 0x77, 0x88, 0x99]); assert.deepEqual(saved.subarray(12), records.subarray(12)); assert.equal(o.templateStorage.currentSection(1), 'Territory'); assert.equal(o.random.state, seed); assert.deepEqual(o.core.pool.bytes(), pool); assert.equal(o.loaded, false); assert.throws(() => o.writeFortTerritory(9), /슬롯 계약/);
    } finally { o.dispose(); }
    assert.throws(() => o.writeFortTerritory(1), /반납한 Session/);
});
