// 최초 원본2060관측과 같은 실제 Template/생산 덱의 snapshot stream 수명을 대조한다(WB04-E628).
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, get, put } from './raw-support.mjs';
import { snapshotFortTypeNames, snapshotFortDeck, FORT_SNAPSHOT_ASSERTION } from '../www/core/raw-fort-snapshot-basics.js';
import { campaignOwners } from './campaign-session-support.mjs';
import { fortSection, SECTION_NAMES } from '../www/assets/fort.js';

// 원본 진단 종류별 실제 행 번호와 signed BYTE/NUL을 포함한 이름 입력이다.
const LINES = { MissingNamesTemplate: 586, NamesNotMine: 587, MissingDeckTemplate: 1220, WriteCapacity: 36 };
const NAMES = ['', 'A', 'abcdE', '\x80\xffAz', 'Netstorm', 'Q\0ignored'];
// 원본 emu_stop만 잡고 예상 밖 물리/제품 오류는 전파한다.
class AssertionStop extends Error {}
// 관측 입력·외부 응답과 같은 가변 물리이며 기대값은 제품 실행 뒤에만 비교한다.
class Devices {
    // 고정 this와 현재 manager/owner/타입 수, 같은 State/Deck/stream/output·선택·trace다.
    slot; manager; mine; typeCount; mutation; state = new Uint8Array(32); deck = new Uint8Array(352);
    stream = new Uint8Array(16); output = new Uint8Array(4096).fill(0xcc); current = 'old'; trace = [];
    // 최초12열의 명시 입력만 사용해 물리를 준비한다.
    constructor(row) {
        this.slot = Number(row[2]); this.manager = Number(row[3]); this.mine = Number(row[4]); this.state[17] = Number(row[5]); this.typeCount = Number(row[6]); put(this.deck, 0x14c, Number(row[7]), 4);
        // count 주소 밖의 실제66개×5BYTE만 고정 입력 패턴으로 채운다.
        for (let index = 0; index < 66; index++) { for (let byte = 0; byte < 5; byte++) this.deck[index * 5 + byte] = index * 19 + byte * 37 + 3; }
        put(this.stream, 0, 77, 4); put(this.stream, 4, Number(row[8]), 4); put(this.stream, 8, Number(row[9]), 4); put(this.stream, 12, Number(row[10]), 4); this.mutation = row[11];
    }
    // 독립 PE에 공급한 외부 변경 입력이며 writer의 조건/출력을 계산하지 않는다.
    mutate(boundary) {
        const prefix = boundary + '-';
        if (this.mutation === prefix + 'manager22') this.manager = 22; else if (this.mutation === prefix + 'mine8') this.mine = 8;
        else if (this.mutation === prefix + 'version0') this.state[17] = 0; else if (this.mutation === prefix + 'version1') this.state[17] = 1;
        else if (this.mutation === prefix + 'typecount2') this.typeCount = 2; else if (this.mutation === prefix + 'typecount0') this.typeCount = 0;
        else if (this.mutation === prefix + 'deckcount2') put(this.deck, 0x14c, 2, 4); else if (this.mutation === prefix + 'deckcount0') put(this.deck, 0x14c, 0, 4);
        else if (this.mutation === prefix + 'capacity64') put(this.stream, 12, 64, 4);
    }
    // C++ const 참조와 같은 두 현재 칸이다. child가 바꾼 typeCount를 복제하지 않는다.
    globals() { const d = this; return { get templateOwner() { return d.mine; }, get typeCount() { return d.typeCount; } }; }
    // 원본에 연결한 외부 writer·물리 조회·명시 응답이며 부모의 hash/루프/인라인 쓰기는 제품이 수행한다.
    hooks() {
        return {
            manager: (slot) => { assert.equal(slot, this.slot); return this.manager; },
            assertion: (kind) => { const line = LINES[kind]; assert.ok(line); this.trace.push('assert:' + line); this.mutate('assert'); if (line === 36 && this.mutation === 'assert-stop') throw new AssertionStop(); },
            create: (manager, name) => { this.trace.push('create:' + manager + ':' + name); this.current = 'Created'; this.mutate('create'); return 1; },
            state: (slot) => { this.trace.push('state:' + slot); this.current = 'State'; this.mutate('state'); return this.state; },
            typeName: (index) => { this.trace.push('name:' + index); this.mutate('name' + index); return NAMES[index % NAMES.length]; },
            refreshDeck: (owner) => { this.trace.push('refresh:' + owner); this.mutate('refresh'); },
            getDeck: (owner) => { this.trace.push('get:' + owner); this.mutate('get'); return 2; }, deck: (token) => { assert.equal(token, 2); return this.deck; },
            write: (token, bytes) => { assert.equal(token, 1); assert.equal(bytes.length, 1); this.trace.push('write:1:' + Buffer.from(bytes).toString('hex')); const position = get(this.stream, 4, 4); assert.ok(position + 1 <= get(this.stream, 12, 4)); this.output[position] = bytes[0]; put(this.stream, 4, position + 1, 4); if (get(this.stream, 8, 4) < position + 1) put(this.stream, 8, position + 1, 4); this.mutate('write'); },
            access: (token, operation) => { assert.equal(token, 1); operation(this.stream, { buffer: (pointer) => { assert.equal(pointer, 77); return this.output; } }); },
        };
    }
}

test('Fort snapshot TypeNames/Deck: 원본2060관측의 signed hash·현재 count·고정 stream·진단/부분 중단이 같다', () => {
    const rows = readRows(path.join(REPO_ROOT, 'cpppj/tests/fixtures/fort-snapshot-basics-x86.tsv'));
    // 기대 뒤12열은 실제 부모가 실행된 뒤 비교하며 관측의 중단은 예외 unwind 증거로 확대하지 않는다.
    for (const row of rows) {
        assert.equal(row.length, 24); const d = new Devices(row); let completed = true;
        try { if (row[1] === 'names') snapshotFortTypeNames(d.slot, d.globals(), d.hooks()); else { assert.equal(row[1], 'deck'); snapshotFortDeck(d.slot, d.hooks()); } } catch (error) { if (!(error instanceof AssertionStop)) throw error; completed = false; }
        const key = row.slice(0, 12).join(','); assert.equal(completed, row[12] === '1', key);
        assert.deepEqual([d.manager, d.mine, d.state[17], d.typeCount >>> 0, get(d.deck, 0x14c, 4)], row.slice(13, 18).map(Number), key);
        assert.equal(d.current, row[18], key); assert.equal(get(d.stream, 4, 4), Number(row[19]), key); assert.equal(get(d.stream, 8, 4), Number(row[20]), key);
        assert.equal(Buffer.from(d.output.subarray(0, get(d.stream, 8, 4))).toString('hex'), row[21], key); assert.equal(d.trace.join(';'), row[22], key);
        if (completed) assert.equal(row[23], '0', key); else assert.ok(Number(row[23]) < 0, key);
    }
    assert.equal(rows.length, 2060);
});

test('Fort snapshot TypeNames: version0는 미선택 쓰기를 요구하지 않고 State 물리·선택된 누락은 접두 뒤 거부한다', () => {
    const d = new Devices(['', '', '0', '11', '0', '0', '6', '0', '0', '0', '256', 'none']); const hooks = d.hooks(); delete hooks.write; delete hooks.access;
    snapshotFortTypeNames(0, d.globals(), hooks); assert.equal(d.trace.join(';'), 'create:11:TypeNames;state:0');
    d.state[17] = 1; assert.throws(() => snapshotFortTypeNames(0, d.globals(), hooks), /실제 경계 미연결/);
    hooks.state = () => new Uint8Array(17); assert.throws(() => snapshotFortTypeNames(0, d.globals(), hooks), /State\+17/);
    assert.equal(FORT_SNAPSHOT_ASSERTION.writeCapacity, 'WriteCapacity');
});

test('Session 기본 snapshot: 실제 Template의 State 선택 뒤에도 TypeNames stream과188개 현재 TYPE를 유지한다', () => {
    const o = campaignOwners(); const fort = { sections: Array.from({ length: SECTION_NAMES.length }, () => new Uint8Array(0)) }; const state = new Uint8Array(28); state[17] = 1;
    try {
        o.templateStorage.install(1, fort, 'snapshot'); o.templateStorage.initializeState(1, state); const before = o.core.pool.bytes(); const seed = o.random.state;
        o.snapshotFortBasicSection(1, true); const names = fortSection(o.templateStorage.template(1), 'TypeNames');
        assert.equal(names[0], 188); assert.equal(names.length, 753); assert.equal(o.templateStorage.currentSection(1), 'State'); assert.equal(o.templateStorage.sectionOrder(1).at(-1), 'TypeNames');
        assert.deepEqual(o.templateState.get(1), state); assert.deepEqual(o.core.pool.bytes(), before); assert.equal(o.random.state, seed); assert.equal(o.loaded, false);
        assert.throws(() => o.snapshotFortBasicSection(9, true), /슬롯 계약/);
    } finally { o.dispose(); }
    assert.throws(() => o.snapshotFortBasicSection(1, true), /반납한 Session/);
});

test('Session 기본 snapshot: 같은 실제 생산 덱의 런타임5번째 BYTE를 제외하고 현재 raw 앞4BYTE만 쓴다', () => {
    const o = campaignOwners({ production: { allocationBytes: (bytes) => bytes.fill(0) } });
    try {
        o.templateStorage.install(1, { sections: Array.from({ length: SECTION_NAMES.length }, () => new Uint8Array(0)) }, 'deck'); o.player.create(1);
        const token = o.productionDeck.get(1); o.productionDeck.activate(token, 71, 1); const raw = o.productionDeck.bytes(token); raw[4] = 0xe7;
        o.objectState.mission = false; o.fullDeck = 1; const first = raw.slice(0, 4); o.snapshotFortBasicSection(1, false);
        assert.deepEqual([...fortSection(o.templateStorage.template(1), 'Deck')], [1, ...first]); assert.equal(o.productionDeck.bytes(token)[4], 0xe7); assert.equal(o.templateStorage.currentSection(1), 'Deck');
    } finally { o.dispose(); }
});
