// 4D-1a의 공용 저장 자식 네 계열을 cpppj 시험과 동결된 x86 관측으로 대조한다(WB04-E633).
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, get, put, floatToBits } from './raw-support.mjs';
import { GameRandom } from '../www/core/random.js';
import { ScrambledSpStore } from '../www/core/sp-store.js';
import { writeMemStream } from '../www/core/raw-mem-stream.js';
import { saveFortMoney, snapshotFortMoney } from '../www/core/raw-fort-money-save.js';
import { saveFortMission, saveFortMissionName, saveFortMissionState } from '../www/core/raw-fort-mission-save.js';
import { saveFortBadges, readFortBadgeCount } from '../www/core/raw-fort-badges-save.js';
import { writeFortCoreData } from '../www/core/raw-fort-core-write.js';

// 원문 emu_stop에 대응하는 지정 외부 중단만 제품 오류와 구별한다.
class FixtureStop extends Error {}
// 관측의 실제 BYTE 전체를 소문자 hex로 표현한다.
function hex(bytes) { return Buffer.from(bytes).toString('hex'); }
// 기존 fixture를 직접 읽으며 기대 열은 제품 호출 뒤에만 비교한다.
function rows(filename) { const result = readRows(path.join(REPO_ROOT, 'cpppj/tests/fixtures', filename)); assert.ok(result); return result; }
// 실제 전체 물리의 FNV-1a64이며 원문 출력 형식만 공유한다.
function hash(bytes) { let value = 0xcbf29ce484222325n; /* 전체 BYTE의 uint64 감김을 보존한다. */ for (const byte of bytes) value = BigInt.asUintN(64, (value ^ BigInt(byte)) * 0x100000001b3n); return value.toString(16).padStart(16, '0'); }
// 장부 전체를 무손실 run length로 나타내 padding도 비교한다.
function rle(bytes) { const result = []; /* 같은 BYTE가 연속되는 최대 구간만 묶는다. */ for (let start = 0; start < bytes.length;) { let end = start + 1; /* 다음 값의 경계를 찾는다. */ while (end < bytes.length && bytes[end] === bytes[start]) end++; result.push(`${end - start}:${hex(bytes.subarray(start, start + 1))}`); start = end; } return result.join(','); }

// 원본 Money 입력의 두 stream/버퍼와 외부 callback 변경만 공급한다.
class MoneyDevices {
    // 같은 난수·고정 slot·현재 manager와 stream 선택이다.
    random; slot; manager; selected;
    // 실제 물리 두 쌍과 명시 외부 효과/호출 순서다.
    fields = [new Uint8Array(16), new Uint8Array(16)]; buffers = [Uint8Array.from({ length: 64 }, (_, i) => i), Uint8Array.from({ length: 64 }, (_, i) => i)]; effects; calls = {}; trace = [];
    // 앞 입력11열만 읽어 원문 물리를 준비한다.
    constructor(row) {
        this.random = new GameRandom(Number(row[2])); this.slot = Number(row[3]); this.manager = Number(row[5]); this.selected = Number(row[6]); this.effects = row[10] === '-' ? [] : row[10].split(';').map((effect) => effect.split('/'));
        // 두 stream에 서로 다른 data와 같은 최초 pos/end/capacity를 쓴다.
        this.fields.forEach((fields, index) => { put(fields, 0, 0x14001000 + index * 0x100); put(fields, 4, Number(row[7])); put(fields, 8, Number(row[8])); put(fields, 12, Number(row[9])); });
    }
    // 지정 경계에서만 입력의 변경을 순서대로 적용한다.
    boundary(event, text) {
        this.trace.push(text); const key = event + (this.calls[event] = (this.calls[event] ?? 0) + 1);
        // 같은 경계에 준비한 여러 변경은 원래 순서를 유지한다.
        for (const [when, field, input] of this.effects) { if (when !== key) continue; const value = Number(input); if (field === 'stop') throw new FixtureStop(); if (field === 'random') this.random.setState(value); else if (field === 'manager') this.manager = value; else { const [target, member] = field.split('.'); put(this.fields[target === 'a' ? 0 : 1], { data: 0, pos: 4, end: 8, alloc: 12, cap: 12 }[member], value); } }
    }
    // 실제 일반 MemStream writer를 같은 반환 stream에 연결한다.
    hooks() { return {
        manager: (slot) => { assert.equal(slot, this.slot); const captured = this.manager; this.boundary('manager', `manager:${slot}:${captured}`); return captured; },
        create: (manager, name) => { assert.equal(name, 'Money'); const stream = 0x14000100 + this.selected * 0x20; this.boundary('create', `create:${manager}:Money:${stream}:${this.random.state}`); return stream; },
        write: (stream, bytes) => { assert.equal(bytes.length, 4); this.boundary('write', `write:${stream}:${get(bytes, 0)}:${this.random.state}`); const index = (stream - 0x14000100) / 0x20; assert.ok(index === 0 || index === 1); writeMemStream(this.fields[index], bytes, 4, {
            buffer: (token) => { const selected = (token - 0x14001000) / 0x100; assert.ok(selected === 0 || selected === 1); return this.buffers[selected]; },
            assertion: (reason) => { assert.equal(reason, 'WriteCapacity'); this.boundary('assert', `assert:capacity:${this.random.state}`); },
        }); },
    }; }
}
// 난수의 모든1010결과·부모·현재 stream·부분 중단1105행을 비교한다.
test('FortMoneySave_IndependentNativeFloatAndStream', () => {
    const source = rows('fort-money-save-x86.tsv'); assert.equal(source.length, 1105);
    // 기대값으로 장치 응답을 만들지 않고 실행 뒤 결과열 전체를 비교한다.
    for (const row of source) { assert.equal(row.length, 21); const d = new MoneyDevices(row), hooks = d.hooks(); let completed = true; try { if (row[1] === 'money') saveFortMoney(0x14000100 + d.selected * 0x20, d.random, hooks.write); else { assert.equal(row[1], 'parent'); snapshotFortMoney(d.slot, d.random, hooks); } } catch (error) { if (!(error instanceof FixtureStop)) throw error; completed = false; } assert.deepEqual([Number(completed), d.random.state, d.manager, ...d.fields.map(hex), ...d.buffers.map(hex), d.trace.join(';') || '-'], [Number(row[11]), Number(row[12]), Number(row[13]), ...row.slice(14, 19)], row[0]); assert.ok(completed ? Number(row[19]) === 0 : Number(row[19]) < 0); assert.ok(Number(row[20]) > 0); }
});
// 선택 경계 누락 앞의 Create/난수와 clamp 없는 slot 계약이다.
test('FortMoneySave_LazyEffectsAndUnclampedSlot', () => {
    const random = new GameRandom(1); assert.throws(() => saveFortMoney(77, random, null)); assert.equal(random.state, 0x10006); random.setState(1);
    const calls = []; const hooks = { manager: (slot) => { calls.push(slot); return 0; } }; assert.throws(() => snapshotFortMoney(-2, random, hooks)); assert.equal(random.state, 1);
    hooks.create = (manager, name) => { assert.equal(manager, 0); assert.equal(name, 'Money'); random.setState(1); return 77; }; assert.throws(() => snapshotFortMoney(-2, random, hooks)); assert.equal(random.state, 0x10006);
    hooks.write = (stream, bytes) => { assert.equal(stream, 77); assert.equal(get(bytes, 0), 0x43a70000); }; snapshotFortMoney(-2, random, hooks); assert.deepEqual(calls, [-2, -2, -2]);
});
// 같은 난수만 전진하고 분산 SP 초기화/재시드/배열 쓰기는 하지 않는다.
test('FortMoneySave_SharedRandomWithoutSpStoreWrites', () => { const random = new GameRandom(1); let reseeds = 0; const store = new ScrambledSpStore(random, () => { reseeds++; return 0; }, 0x87654321), before = store.pools().slice(); saveFortMoney(77, random, (stream, bytes) => { assert.equal(stream, 77); assert.equal(get(bytes, 0), 0x43a70000); }); assert.equal(random.state, 0x10006); assert.equal(reseeds, 0); assert.equal(store.initialized, false); assert.deepEqual(store.pools(), before); });

// Guide/Mission의 입력과 실제 일반 writer/두 물리 버퍼만 소유한다.
class MissionDevices {
    // 고정 입력과 현재 Guide/Mission 필드·조회 횟수다.
    mode; current; nameCase; manager; mutation; sequence; flags; name; writes = 0; currentReads = 0; nameReads = 0;
    // 실제 stream16BYTE와 살아 있는 출력 두 할당·시간순 기록이다.
    stream = new Uint8Array(16); buffers = [new Uint8Array(0x11000).fill(0xcc), new Uint8Array(0x11000).fill(0xcc)]; trace = [];
    // 입력12열과 원본에 공급한 이름 패턴만 준비한다.
    constructor(input) {
        const f = input.split(','); this.mode = f[0]; this.current = Number(f[1]); this.nameCase = Number(f[2]); this.manager = Number(f[10]); this.mutation = f[11]; this.sequence = [0, Number(f[6]), Number(f[8])]; this.flags = [0, Number(f[7]), Number(f[9])]; put(this.stream, 0, 0x12040000); [4, 8, 12].forEach((at, index) => put(this.stream, at, Number(f[index + 3])));
        this.name = this.nameCase === 2 ? new Uint8Array([65, 0, 66, 67, 0]) : new Uint8Array([0, 3, 0, 31, 65535, 65536, 65537, 0][this.nameCase] + 1);
        if (this.nameCase !== 2) { /* 최초 NUL 앞만 같은 입력 패턴으로 채운다. */ for (let index = 0; index + 1 < this.name.length; index++) this.name[index] = 65 + index % 23; }
    }
    // 지정된 외부 callback의 변경만 적용한다.
    mutate(boundary) {
        const mutation = this.mutation;
        if (mutation === `${boundary}-current2`) this.current = 2; else if (mutation === `${boundary}-current0`) this.current = 0;
        else if (mutation === `${boundary}-name`) this.name.set([90, 0, 89]);
        else if (mutation === 'write1-sequence' && boundary === 'write1') this.sequence[1] = 0x13579bdf;
        else if (mutation === 'write3-flags' && boundary === 'write3') this.flags[1] = 0x2468ace0;
        else if (mutation === 'write4-flags' && boundary === 'write4') this.flags[1] = 0x01020304;
        else if (mutation === 'capacity-current' && boundary === 'capacity') { put(this.stream, 0, 0x12060000); put(this.stream, 4, 5); put(this.stream, 8, 2); put(this.stream, 12, 0x11000); this.sequence[1] = 0x11223344; }
        else if (mutation === 'write1-physical' && boundary === 'write1') put(this.stream, 0, 0);
        else if (mutation === `${boundary}-stop`) throw new FixtureStop(boundary);
    }
    // 순수 현재 getter와 실제 일반 writer를 공급한다.
    hooks() { return {
        current: () => { this.currentReads++; return this.current; }, name: () => { this.nameReads++; return this.nameCase === 7 ? null : this.name; },
        field: (selected, offset) => { if (!selected) throw new FixtureStop('mission0'); assert.ok(selected <= 2); return offset === 0x58 ? this.sequence[selected] : this.flags[selected]; },
        assertion: (reason) => { assert.equal(reason, 'NullName'); this.trace.push('A:155'); this.mutate('null'); },
        write: (selected, bytes) => { assert.equal(selected, 1); this.writes++; this.trace.push(`W:${this.writes}:${bytes.length}:${hash(bytes)}`); this.mutate(`write${this.writes}`); try { writeMemStream(this.stream, bytes, bytes.length, {
            buffer: (token) => { if (!token) throw new FixtureStop('buffer0'); const index = (token - 0x12040000) / 0x20000; assert.ok(index === 0 || index === 1); return this.buffers[index]; },
            assertion: () => { this.trace.push('A:36'); this.mutate('capacity'); },
        }); } catch (error) { if (!get(this.stream, 0) && bytes.length) throw new FixtureStop('buffer0'); throw error; } },
    }; }
    // 입력이 선택한 공개 API만 실행한다.
    run() { const hooks = this.hooks(); if (this.mode === 'guide') saveFortMission(1, hooks); else if (this.mode === 'name') saveFortMissionName(1, this.nameCase === 7 ? null : this.name, hooks); else { assert.equal(this.mode, 'state'); saveFortMissionState(1, this.current, hooks); } }
    // 현재 전체 상태를 원문 열 순서로 모은다.
    state() { return [this.manager, this.current, this.sequence[1], this.flags[1], this.sequence[2], this.flags[2], this.writes].join(','); }
    // 미사용 할당과 앞뒤 가드를 포함한 전체 물리를 비교한다.
    raw() { return hex(this.stream) + this.buffers.map((bytes) => `|${hash(bytes)}:${hex(bytes.subarray(0, 64))}:${hex(bytes.subarray(-16))}`).join(''); }
}
// 최초115입력의 두 정밀도·이름 WORD 경계·전체 출력/중단12건을 비교한다.
test('FortMissionSave_IndependentNative230', () => { const source = rows('fort-mission-save-x86.tsv'); let stops = 0; /* 모든 원문 관측을 한 번씩 재생한다. */ for (const row of source) { const d = new MissionDevices(row[1]); let status = '1'; try { d.run(); } catch (error) { if (!(error instanceof FixtureStop)) throw error; status = error.message; stops++; } assert.deepEqual([status, d.state(), d.raw(), d.trace.join(';')], row.slice(2), row.slice(0, 2).join(':')); } assert.equal(source.length, 230); assert.equal(stops, 12); });
// 현재 Mission 재선택과 최초 선택 유지·null/비NUL 물리·길이0 쓰기 계약이다.
test('FortMissionSave_CurrentAndPhysicalBoundaries', () => {
    let queries = 0; saveFortMission(99, { current: () => { queries++; return 0; } }); assert.equal(queries, 1);
    const d = new MissionDevices('guide,1,1,0,0,64,1,2,3,4,11,write3-current2'); d.run(); assert.equal(d.currentReads, 2); assert.equal(d.nameReads, 1); assert.equal(get(d.buffers[0], 5), 1); assert.equal(get(d.buffers[0], 9), 2);
    const lengths = [], hooks = { write: (stream, bytes) => { assert.equal(stream, 77); lengths.push(bytes.length); } }; assert.throws(() => saveFortMissionName(77, new Uint8Array([65, 66]), hooks), RangeError); assert.throws(() => saveFortMissionName(77, null, hooks)); saveFortMissionName(77, new Uint8Array(1), hooks); assert.deepEqual(lengths, [2, 0]);
});

// Badges 원본 입력의 두 stream/두 살아 있는 할당과 외부 callback 변경만 공급한다.
class BadgesDevices {
    // 최초 입력·실제 Create 반환/현재 stream·getter 결과와 용량 진단 횟수다.
    kind; profile; query; count; manager; mutation; created = 0; token = 0; result = 0; capacityCalls = 0; active = 1; counterToken = 0; counterPosition = 0;
    // 실제 Template20BYTE·stream16BYTE·출력과 Client 장부다.
    header = Uint8Array.from({ length: 20 }, (_, index) => index); nodes = [new Uint8Array(16), new Uint8Array(16)]; buffers = [new Uint8Array(256).fill(0xcc), new Uint8Array(256).fill(0xdd)]; badges = new Uint8Array(6160).fill(0xa5);
    // 원문 외부 manager current와 callback 횟수·전체 순서다.
    currents = { 0: 90, 11: 91, 22: 92 }; calls = {}; trace = [];
    // 최초9입력만 사용해 장부와 두 물리를 구성한다.
    constructor(input) {
        const f = input.split(','); [this.kind, this.profile, this.query, this.count, this.manager] = f.slice(0, 5).map(Number); this.mutation = f[8]; put(this.header, 0, this.pointer(this.manager));
        this.nodes.forEach((node, index) => { put(node, 0, 0x12008000 + index * 0x1000); [4, 8, 12].forEach((at, field) => put(node, at, Number(f[field + 5]))); });
        // 각 profile은 원본 도구가 제공한 입력 패턴이며 저장 기대값을 계산하지 않는다.
        for (let index = 0; index < 28; index++) {
            let type = index, quantity = 0;
            if (this.profile === 0) { type = index === 0 ? this.query : 70 + index; quantity = index === 0 ? this.count : 0; }
            else if (this.profile === 1) { const duplicate = index === 0 || index === 13 || index === 27; type = duplicate ? this.query : 100 + index; quantity = duplicate ? this.count + (index === 0 ? 0 : index === 13 ? 1 : 2) : 0; }
            else if (this.profile === 2) { type = index < 3 ? [0, 254, 255][index] : index === 27 ? 0 : 100 + index; quantity = index < 3 ? this.count : index === 27 ? this.count + 7 : 0; }
            else if (this.profile === 3) { type = index < 27 ? index : 254; quantity = this.count + index; }
            else if (this.profile === 4) { type = 1000 + index; quantity = this.count; }
            put(this.badges, index * 220, type); put(this.badges, index * 220 + 216, quantity);
        }
    }
    // manager 장치 ID를 원본 raw 포인터로만 변환한다.
    pointer(id) { assert.ok([0, 11, 22].includes(id)); return id === 0 ? 0 : id === 11 ? 0x12000100 : 0x12000200; }
    // 현재 Template의 실제 raw 포인터를 장치 ID로 읽는다.
    currentManager() { const pointer = get(this.header, 0); assert.ok([0, 0x12000100, 0x12000200].includes(pointer)); return pointer === 0 ? 0 : pointer === 0x12000100 ? 11 : 22; }
    // 최초 count 할당의 BYTE이며 현재 data 변경을 따라가지 않는다.
    counter() { return this.counterToken ? this.buffers[this.counterToken - 1][this.counterPosition] : -1; }
    // 외부 callback 호출 순서를 기록하고 실제 횟수를 반환한다.
    event(name, text) { this.trace.push(text); return this.calls[name] = (this.calls[name] ?? 0) + 1; }
    // 지정된 callback의 외부 변경만 적용한다.
    mutate(boundary) {
        const split = this.mutation.lastIndexOf('-'); if (split < 0 || this.mutation.slice(0, split) !== boundary) return; const action = this.mutation.slice(split + 1), node = this.nodes[this.active - 1];
        if (action.startsWith('manager')) put(this.header, 0, this.pointer(Number(action.slice(7))));
        else if (action === 'dataA' || action === 'dataB') put(node, 0, action === 'dataA' ? 0x12008000 : 0x12009000);
        else if (action.startsWith('pos')) put(node, 4, Number(action.slice(3))); else if (action.startsWith('end')) put(node, 8, Number(action.slice(3))); else if (action.startsWith('cap')) put(node, 12, Number(action.slice(3)));
        else if (action.startsWith('counter')) this.buffers[this.counterToken - 1][this.counterPosition] = Number(action.slice(7));
        else if (action === 'table999') put(this.badges, 216, 999);
        else if (action === 'current2') { /* 세 manager의 current만 바꾸며 반환 stream은 바꾸지 않는다. */ for (const key of Object.keys(this.currents)) this.currents[key] = 2; }
        else if (action === 'stop') throw new FixtureStop();
    }
    // manager/Create/getter와 동기 MemStream 빌림을 연결한다.
    hooks() { return {
        manager: (slot) => { assert.equal(slot, 1); return this.currentManager(); },
        create: (selected, name) => { assert.equal(name, 'Badges'); this.created++; this.token = this.mutation === 'create-return2' ? 2 : 1; this.active = this.token; this.event('create', `create:${selected}:Badges`); this.currents[selected] = this.token; this.mutate('create'); return this.token; },
        assertion: (line) => { const node = this.nodes[this.active - 1], index = this.event('assert', `assert:${line}:${get(node, 0)}:${get(node, 4)}:${get(node, 8)}:${get(node, 12)}:${this.counter()}`); this.mutate(line === 36 ? `capacity${++this.capacityCalls}` : `assert${index}`); },
        count: (type) => { const value = readFortBadgeCount(this.badges, type); this.event('count', `count:${type}:${value}:${this.counter()}`); this.mutate(`count${type}`); return value; },
        access: (selected, operation) => { assert.equal(selected, this.token); const node = this.nodes[selected - 1]; if (!this.counterToken) { this.counterToken = get(node, 0) === 0x12008000 ? 1 : 2; this.counterPosition = get(node, 4); } operation(node, { buffer: (pointer) => { const index = (pointer - 0x12008000) / 0x1000; assert.ok(index === 0 || index === 1); return this.buffers[index]; } }); },
    }; }
    // 실제 전체 외부 상태를 원문 순서로 관측한다.
    state() { return [this.currentManager(), this.created, this.token, this.result, this.currents[0], this.currents[11], this.currents[22]].join(','); }
    // 미사용 stream/버퍼와 Template 부가 필드도 함께 비교한다.
    raw() { return [this.header, ...this.nodes, ...this.buffers].map(hex).join(''); }
}
// 원문676입력×두 정밀도의 전체 장부/출력과 중단80건·경계 횟수를 비교한다.
test('FortBadgesSave_IndependentNative1352', () => {
    const source = rows('fort-badges-save-x86.tsv'); let stops = 0, counts = 0, assertions = 0, creates = 0;
    // getter 단독도 저장 부모와 같은 실제 장부를 사용한다.
    for (const row of source) { const d = new BadgesDevices(row[1]); let completed = true; try { if (!d.kind) saveFortBadges(1, d.hooks()); else d.result = readFortBadgeCount(d.badges, d.query); } catch (error) { if (!(error instanceof FixtureStop)) throw error; completed = false; stops++; } assert.deepEqual([Number(completed), d.state(), d.raw(), rle(d.badges), d.trace.join(';')], [Number(row[2]), ...row.slice(3)], row.slice(0, 2).join(':')); counts += d.calls.count ?? 0; assertions += d.calls.assert ?? 0; creates += d.calls.create ?? 0; }
    assert.deepEqual([source.length, stops, counts, assertions, creates], [1352, 80, 167834, 3788, 710]);
});
// 누락된 count getter도 Create/초기 BYTE 쓰기가 끝난 위치에서만 실패한다.
test('FortBadgesSave_MissingDevicesAndPhysicalLedger', () => { const d = new BadgesDevices('0,0,0,1,11,0,0,128,none'); assert.throws(() => saveFortBadges(1, { ...d.hooks(), count: null })); assert.equal(d.created, 1); assert.equal(get(d.nodes[0], 4), 1); assert.equal(d.buffers[0][0], 0); const getter = new BadgesDevices('1,1,255,0,11,0,0,128,none'); assert.equal(readFortBadgeCount(getter.badges, 255), 0); assert.equal(readFortBadgeCount(getter.badges, 0xffffffff), 0); assert.throws(() => readFortBadgeCount(getter.badges.subarray(0, 6159), 0), RangeError); });

// CoreData의 두 Player 할당·현재 전역/seed와 명시 callback 응답만 제공한다.
class CoreDevices {
    // 최초 전역과 실제 seed DWORD 참조·난수 입력·현재 Player base다.
    slot; manager; metamap; post; editor; owner; seed; sequence; mutation; active = 1;
    // 살아 있는 두9x180BYTE 할당과 실제 경계 횟수·부분 출력이다.
    players = [new Uint8Array(1620), new Uint8Array(1620)]; calls = {}; trace = []; output = [];
    // 최초9입력과 원본 Player 패턴만 구성한다.
    constructor(input) { const f = input.split(','); [this.slot, this.manager, this.metamap, this.post, this.editor, this.owner] = f.slice(0, 6).map(Number); this.seed = { value: Number(f[6]) }; this.sequence = Number(f[7]); this.mutation = f[8]; this.players.forEach((block, group) => { /* 각9기록의 실제10BYTE만 초기화한다. */ for (let index = 0; index < 9; index++) { /* 기록 내 입력 BYTE 패턴이다. */ for (let byte = 0; byte < 10; byte++) block[index * 180 + 108 + byte] = group * 79 + index * 17 + byte * 13; } }); }
    // 시간순 trace와 해당 경계의 횟수를 관측한다.
    event(kind, text) { this.trace.push(text); return this.calls[kind] = (this.calls[kind] ?? 0) + 1; }
    // 지정 callback에서만 최초 입력의 외부 변경을 적용한다.
    mutate(boundary) {
        const split = this.mutation.indexOf('-'); if (split < 0 || this.mutation.slice(0, split) !== boundary) return; const action = this.mutation.slice(split + 1);
        if (action === 'stop') throw new FixtureStop();
        if (action.startsWith('manager')) this.manager = Number(action.slice(7)); else if (action.startsWith('post')) this.post = Number(action.slice(4)); else if (action.startsWith('meta')) this.metamap = Number(action.slice(4)); else if (action.startsWith('editor')) this.editor = Number(action.slice(6)); else if (action.startsWith('owner')) this.owner = Number(action.slice(5)); else if (action === 'playerB') this.active = 2;
        else if (action === 'core') { const kind = boundary.replace(/\d+$/, ''); /* 옛 할당도 같은 외부 변경을 받는다. */ for (const block of this.players) { /* Player마다 다른10BYTE다. */ for (let index = 0; index < 9; index++) { /* 각 child 뒤 재조회 여부를 구분하는 입력이다. */ for (let byte = 0; byte < 10; byte++) block[index * 180 + 108 + byte] = this.calls[kind] * 23 + index * 17 + byte * 11; } } }
        else if (action.startsWith('seed')) this.seed.value = parseInt(action.slice(4), ['seed12340000', 'seed80000000'].includes(action) ? 16 : 10); else throw new Error('CoreWrite 명시 변경 미해석');
    }
    // 현재 전역 getter와 같은 seed 참조를 빌린다.
    globals() { return {
        metamap: () => { this.event('metamap', `metamap:4:${this.metamap}`); return this.metamap; }, missionPost: () => { this.event('post', `post:4:${this.post}`); return this.post; }, editor: () => { this.event('editor', `editor:4:${this.editor}`); return this.editor; },
        players: () => { this.event('players', `players:4:${this.active}`); return this.players[this.active - 1]; }, playerOwner: () => { this.event('owner', `owner:4:${this.owner}`); return this.owner; }, seed: () => this.seed,
    }; }
    // 독립 PE에 공급했던 외부 응답만 제공하며 제품 저장 구현을 대신하지 않는다.
    hooks() { return {
        manager: (slot) => { assert.equal(slot, this.slot); this.event('manager', `manager:${this.manager}`); return this.manager; }, assertion: (reason) => { const index = this.event('assert', `A:${reason}`); this.mutate(`assert${index}`); },
        create: (manager, name) => { const index = this.event('create', `C:${manager}:${name}`); this.mutate(`create${index}`); return 101; }, refresh: () => { const index = this.event('refresh', 'F'); this.mutate(`refresh${index}`); },
        write: (stream, bytes) => { this.output.push(...bytes); const index = this.event('write', `W:${stream}:${bytes.length}:${hex(bytes)}`); this.mutate(`write${index}`); }, prepareRandom: () => { const index = this.event('prepare', 'P'); this.mutate(`prepare${index}`); },
        nextRandom: (maximum) => { const result = this.sequence === 0 ? [0, 65536, 0x12348001][Math.min(this.calls.random ?? 0, 2)] : this.sequence === 1 ? 0xffffffff : 1; const index = this.event('random', `R:${maximum}:${result}`); this.mutate(`random${index}`); return result; },
        sp: (stream, value) => { const index = this.event('sp', `S:${stream}:${floatToBits(value)}`); this.mutate(`sp${index}`); },
    }; }
    // 두 Player 할당의 실제10BYTE를 원시 순서로 비교한다.
    cores() { const result = []; /* 미사용 Player도 빠짐없이 포함한다. */ for (const block of this.players) { /* 같은9기록의10BYTE만 관측한다. */ for (let index = 0; index < 9; index++) result.push(hex(block.subarray(index * 180 + 108, index * 180 + 118))); } return result.join(''); }
    // 현재 전체 전역을 원문 DWORD 열 순서로 나타낸다.
    state() { return [this.manager, this.metamap, this.post, this.editor, this.owner, this.seed.value, this.active].join(','); }
}
// 원문 fixture 하나의 정상/중단·전체 상태·순서와 부분 출력을 비교한다.
function compareCore(name, expected, expectedStops) {
    const source = rows(name); let stops = 0;
    // 직접 seed 참조 읽기에 인위 callback을 추가하지 않고 원문의 읽기 관측만 분리한다.
    for (const row of source) { const d = new CoreDevices(row[1]); let completed = true; try { writeFortCoreData(d.slot, d.globals(), d.hooks()); } catch (error) { if (!(error instanceof FixtureStop)) throw error; completed = false; stops++; } assert.deepEqual([completed, d.state(), d.cores(), d.output.length ? hex(d.output) : '-', d.trace.join(';')], [row[2] === 'normal', row[3], row[4], row[5], row[6].split(';').filter((event) => !event.startsWith('seed:')).join(';')], row.slice(0, 2).join(':')); }
    assert.equal(source.length, expected); assert.equal(stops, expectedStops);
}
// 최초169입력의 두 정밀도 전체338관측이다.
test('FortCoreWrite_IndependentNative338', () => compareCore('fort-core-write-x86.tsv', 338, 16));
// 별도 세 번째 진단과 상위 비영 DWORD22관측이다.
test('FortCoreWrite_IndependentNativeExtra22', () => compareCore('fort-core-write-extra-x86.tsv', 22, 2));
// Create/refresh/prepare의 별도 callback36관측이다.
test('FortCoreWrite_IndependentNativeCallbacks36', () => compareCore('fort-core-write-callbacks-x86.tsv', 36, 6));
// 자연 반환은 미선택 장치가 필요 없고 선택 경계 누락은 선행 부분 효과를 보존한다.
test('FortCoreWrite_SelectedMissingDeviceBoundaries', () => {
    const d = new CoreDevices('1,0,1,1,0,1,0,0,none'); writeFortCoreData(1, { ...d.globals(), editor: null, players: null, playerOwner: null, seed: null }, { ...d.hooks(), create: null, refresh: null, write: null, prepareRandom: null, nextRandom: null, sp: null }); assert.equal(d.calls.assert, 2); assert.equal(d.calls.create, undefined);
    // 실제 도달한 각 하위를 한 번씩 제거한다.
    for (let stage = 0; stage < 7; stage++) { const selected = new CoreDevices('1,11,0,0,0,1,0,0,none'), globals = selected.globals(), hooks = selected.hooks(); if (stage === 3) globals.seed = null; else hooks[['create', 'refresh', 'write', '', 'prepareRandom', 'nextRandom', 'sp'][stage]] = null; assert.throws(() => writeFortCoreData(1, globals, hooks)); assert.equal(selected.calls.create ?? 0, stage ? 1 : 0); assert.equal(selected.calls.refresh ?? 0, stage >= 2 ? 1 : 0); assert.equal(selected.output.length, stage <= 2 ? 0 : stage < 6 ? 10 : 12); assert.equal(selected.seed.value, stage === 6 ? 32769 : 0); }
});
// Player 물리 범위는 자연 반환 뒤·선행 refresh 효과가 끝난 실제 읽기에서만 검사한다.
test('FortCoreWrite_PhysicalReadAfterSelectedRefresh', () => { const d = new CoreDevices('1,11,0,0,1,1,0,0,none'), globals = { ...d.globals(), players: () => new Uint8Array(0) }; writeFortCoreData(1, globals, d.hooks()); assert.equal(d.calls.create, undefined); d.editor = 0; assert.throws(() => writeFortCoreData(1, globals, d.hooks()), RangeError); assert.equal(d.calls.create, 1); assert.equal(d.calls.refresh, 1); assert.equal(d.output.length, 0); assert.equal(d.seed.value, 0); });
