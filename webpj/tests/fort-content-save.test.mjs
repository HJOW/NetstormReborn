// 최초 원본706관측의 중첩 내용물/현재 후보·원래 개수 주소와 실제 Session Form/stream을 대조한다(WB04-E629).
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, get, put } from './raw-support.mjs';
import { saveFortContents, FORT_CONTENT_SAVE_ASSERTION } from '../www/core/raw-fort-content-save.js';
import { campaignOwners } from './campaign-session-support.mjs';
import { fortSection, SECTION_NAMES } from '../www/assets/fort.js';

// 원본 emu_stop에 해당하는 명시 외부 중단만 구별한다.
class FixtureStop extends Error {}
// 독립 관측의 입력/외부 응답만 소유한다. 출력/부모의 직렬화 분기는 제품 함수가 결정한다.
class Devices {
    // 실제 stream·두 원시 버퍼·16개의50BYTE Form과 metadata/후보열·현재 finder다.
    parent = 1; limit = 8; stream = new Uint8Array(16); buffers = [new Uint8Array(16384).fill(0xcc), new Uint8Array(16384).fill(0xcc)];
    forms = Array.from({ length: 16 }, () => new Uint8Array(50)); robust = new Map(); types = new Map(); children = new Map(); current = null;
    // 입력 순서·명시 변경/현재 경계 횟수·실제 trace다.
    formOrder = []; typeOrder = []; mutations = []; calls = new Map(); trace = [];
    // 앞10열만 해석해 원래 폭으로 물리를 준비한다. 관측 기대열은 읽지 않는다.
    constructor(row = null) {
        if (!row) return;
        this.parent = Number(row[1]); this.limit = Number(row[2]); put(this.stream, 4, Number(row[3]), 4); put(this.stream, 8, Number(row[4]), 4); put(this.stream, 12, Number(row[5]), 4);
        // 지정된 Form만 실제 kind/QA/listflags WORD와 가상 Robust 응답을 갖는다.
        if (row[6] !== '-') for (const record of row[6].split(';')) { const [sid, kind, quantity, flags, robust] = record.split(':').map(Number); this.formOrder.push(sid); put(this.forms[sid], 18, kind, 2); put(this.forms[sid], 12, quantity, 2); put(this.forms[sid], 20, flags, 2); this.robust.set(sid, robust); }
        // 세 DWORD metadata도 입력 순서와 현재 변경을 별도로 유지한다.
        if (row[7] !== '-') for (const record of row[7].split(';')) { const [kind, flags1, contentListFlags, containerListFlags] = record.split(':').map(Number); this.typeOrder.push(kind); this.types.set(kind, { flags1, contentListFlags, containerListFlags }); }
        // 반복 후보/경계 밖 SID도 원문 입력에서 삭제하지 않는다.
        if (row[8] !== '-') for (const record of row[8].split(';')) { const [parent, sequence] = record.split(':'); this.children.set(Number(parent), sequence === '-' ? [] : sequence.split(',').map(Number)); }
        if (row[9] !== '-') this.mutations = row[9].split(';').map((record) => record.split('/'));
    }
    // 같은 경계의 여러 변경을 작성 순서로 적용하며 부모의 후속 조건을 대신 계산하지 않는다.
    boundary(name, text) {
        this.trace.push(text); const count = (this.calls.get(name) ?? 0) + 1; this.calls.set(name, count); const event = name + count;
        // 현재 Form/metadata/stream/global/finder의 명시 외부 효과다.
        for (const [at, target, field, input] of this.mutations) {
            if (at !== event) continue; const value = Number(input) >>> 0;
            if (target.startsWith('form')) { const sid = Number(target.slice(4)); if (field === 'robust') this.robust.set(sid, value); else put(this.forms[sid], field === 'kind' ? 18 : field === 'quantity' ? 12 : 20, value, 2); }
            else if (target.startsWith('type')) { const type = this.types.get(Number(target.slice(4))); type[field === 'flags1' ? 'flags1' : field === 'content' ? 'contentListFlags' : 'containerListFlags'] = value; }
            else if (target === 'stream') put(this.stream, field === 'data' ? 0 : field === 'position' ? 4 : field === 'end' ? 8 : 12, value, 4);
            else if (target === 'global') { assert.equal(field, 'poollimit'); this.limit = value; }
            else if (target === 'finder') { assert.equal(field, 'current'); this.current.current = value; }
            else { assert.equal(target, 'control'); assert.equal(field, 'stop'); assert.equal(value, 1); throw new FixtureStop(); }
        }
    }
    // 각 Begin은 실제 독립 cursor를 반환하며 중첩 자식이 끝나도 부모의 sequence/index는 보존된다.
    hooks() {
        return {
            begin: (parent) => {
                const sequence = this.children.get(parent) ?? []; const cursor = { parent, sequence, current: sequence[0] ?? 0, index: 0 }; this.current = cursor; this.boundary('begin', 'begin:' + parent);
                return { current: () => { this.current = cursor; return cursor.current; }, next: () => { this.current = cursor; const before = cursor.current; cursor.index++; cursor.current = cursor.sequence[cursor.index] ?? 0; this.boundary('next', 'next:' + cursor.parent + ':' + before); } };
            },
            poolLimit: () => this.limit, form: (sid) => { assert.ok(sid < this.forms.length); return this.forms[sid]; },
            robust: (sid) => { const answer = this.robust.get(sid) ?? 0; this.boundary('robust', 'robust:' + sid); return answer !== 0; },
            type: (kind) => { this.boundary('type', 'type:' + kind); assert.ok(this.types.has(kind)); return this.types.get(kind); },
            parentType: (parent) => { this.boundary('parent', 'parent:' + parent); const kind = get(this.forms[parent], 18, 2); assert.ok(this.types.has(kind)); return this.types.get(kind); },
            assertion: (kind) => this.boundary('assert', 'assert:' + (kind === FORT_CONTENT_SAVE_ASSERTION.writeCapacity ? 36 : 278)),
            access: (token, operation) => { assert.equal(token, 1); operation(this.stream, { buffer: (data) => { assert.ok(data >= 0 && data < this.buffers.length); return this.buffers[data]; } }); },
        };
    }
    // 관측 입력의 같은 순서로 현재 raw Form/Robust 값을 표현한다.
    formOutput() { return this.formOrder.map((sid) => [sid, get(this.forms[sid], 18, 2), get(this.forms[sid], 12, 2), get(this.forms[sid], 20, 2), this.robust.get(sid)].join(':')).join(';'); }
    // 같은 순서의 현재 metadata 세 DWORD다.
    typeOutput() { return this.typeOrder.map((kind) => { const type = this.types.get(kind); return [kind, type.flags1, type.contentListFlags, type.containerListFlags].join(':'); }).join(';'); }
}

test('Fort 내용물 저장: 최초 원본706관측의 전체 버퍼·현재 후보/metadata·trace·정상/부분 중단이 같다', () => {
    const rows = readRows(path.join(REPO_ROOT, 'cpppj/tests/fixtures/fort-content-save-x86.tsv'));
    // 앞10열을 실제 본문에 공급한 뒤 뒤12열의 원본 기대와 비교한다.
    for (const row of rows) {
        assert.equal(row.length, 22); const d = new Devices(row); let completed = true;
        try { saveFortContents(d.parent, 1, d.hooks()); } catch (error) { if (!(error instanceof FixtureStop)) throw error; completed = false; }
        assert.equal(completed, row[10] === '1', row[0]); assert.deepEqual([get(d.stream, 4, 4), get(d.stream, 8, 4), get(d.stream, 0, 4)], row.slice(11, 14).map(Number), row[0]);
        assert.equal(Buffer.from(d.buffers[0].subarray(0, get(d.stream, 8, 4))).toString('hex'), row[14], row[0]); assert.equal(Buffer.from(d.buffers[1].subarray(0, get(d.stream, 8, 4))).toString('hex'), row[15], row[0]); assert.equal(d.trace.join(';'), row[16], row[0]);
        if (completed) assert.equal(row[17], '0', row[0]); else assert.ok(Number(row[17]) < 0, row[0]);
        assert.equal(d.limit, Number(row[18]), row[0]); assert.equal(d.formOutput(), row[19], row[0]); assert.equal(d.typeOutput(), row[20], row[0]); assert.ok(Number(row[21]) > 0, row[0]);
    }
    assert.equal(rows.length, 706);
});

test('Fort 내용물 저장: 빈 목록의 무진단 헤더·선택된 누락/순환과 개수 BYTE 감김을 보존한다', () => {
    const empty = new Devices(); const hooks = empty.hooks(); for (const name of ['robust', 'type', 'form', 'parentType', 'assertion', 'poolLimit']) delete hooks[name]; saveFortContents(1, 1, hooks); assert.equal(get(empty.stream, 4, 4), 1); assert.equal(empty.buffers[0][0], 0); assert.equal(empty.trace.join(';'), 'begin:1');
    const cycle = new Devices(); put(cycle.stream, 12, 4096, 4); cycle.children.set(1, [2]); cycle.children.set(2, [1]); cycle.robust.set(1, 1); cycle.robust.set(2, 1); put(cycle.forms[1], 18, 71, 2); put(cycle.forms[2], 18, 71, 2); cycle.types.set(71, { flags1: 0x20000, contentListFlags: 0, containerListFlags: 0 }); assert.throws(() => saveFortContents(1, 1, cycle.hooks()), /순환 부모/); assert.ok(get(cycle.stream, 4, 4) > 1);
    const missing = new Devices(); missing.children.set(1, [2]); const absent = missing.hooks(); delete absent.robust; assert.throws(() => saveFortContents(1, 1, absent), /실제 경계 미연결/); assert.equal(missing.buffers[0][0], 0); assert.equal(get(missing.stream, 4, 4), 1);
    const repeated = new Devices(); repeated.children.set(1, new Array(256).fill(2)); repeated.robust.set(2, 1); put(repeated.forms[1], 18, 71, 2); put(repeated.forms[2], 18, 71, 2); repeated.types.set(71, { flags1: 0, contentListFlags: 0, containerListFlags: 0 }); put(repeated.stream, 12, 4096, 4); saveFortContents(1, 1, repeated.hooks()); assert.equal(repeated.buffers[0][0], 0); assert.equal(get(repeated.stream, 4, 4), 513);
});

test('Session 기술 내용물 저장: 실제 같은 Form6/Robust·finder와 보관한 Technology 노드를 사용한다', () => {
    const o = campaignOwners();
    try {
        o.templateStorage.install(1, { sections: Array.from({ length: SECTION_NAMES.length }, () => new Uint8Array(0)) }, 'contents'); const parent = o.player.create(1); o.contained.create(parent, 71, 0, 4, 0x10, 0);
        const stream = o.templateStorage.createSection(1, 'Technology'); o.templateStorage.selectSection(1, 'State', 1);
        const bytes = o.core.pool.bytes(); const seed = o.random.state; o.writeFortTechnology(parent, stream);
        assert.deepEqual([...fortSection(o.templateStorage.template(1), 'Technology').subarray(0, 2)], [1, 71]); assert.equal(o.templateStorage.currentSection(1), 'State'); assert.deepEqual(o.core.pool.bytes(), bytes); assert.equal(o.random.state, seed); assert.equal(o.loaded, false);
        assert.throws(() => o.snapshotFortTechnology(1), /설치\/슬롯 계약/);
        o.templateStorage.createSection(1, 'Technology'); assert.throws(() => o.writeFortTechnology(parent, stream), /수명|참조|노드|유효/);
    } finally { o.dispose(); }
    assert.throws(() => o.writeFortTechnology(0, {}), /반납한 Session/);
});
