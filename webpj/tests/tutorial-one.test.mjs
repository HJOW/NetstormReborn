// Totalmade(허용 표·집계 표 조작 raw-total-made.js, 004c23c0~004c2560)와 Early Mission 1 단계 판정기(raw-tutorial-one.js, 004c3a20)를 원본 10.78 관측과 cpppj `TutorialOneTests` 계약으로 대조한다.
//  - 원본 관측 tutorial-one-x86.tsv 의 10.78(originals): `Total` 52행(여덟 연산 × 입력의 반환값·진단 횟수·네 표 1024 BYTE 전체)과 `Stage` 266행(단락 평가·반환·다중 Graph·SP 포함 경계·NaN 의 호출 순서 `B`·`G`·`M`·`A`·`S`).
//  - 계약: 실제 reset 행을 독립 helper 와 공유 장부 두 경로로 재생, 공유 상태/가변 목록이 callback 뒤 현재 값을 읽음.
// CD 10.72(originalCD) Total 52행은 웹 범위 밖이다. 실제 게임·브라우저 실행이 아니다. (2026-11-10 WB04-E237: Totalmade 와 Early1 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put } from './raw-support.mjs';
import { RawTotalMadeStorage, SquidPostPopList, SquidPostPopState } from '../www/core/squid-postpop-state.js';
import { RawTotalMade, TotalMadeAssertion, resetTotalMadeCounts, setTotalMadeAllow, setTotalMadeAllowAll } from '../www/core/raw-total-made.js';
import { RawTutorialOne } from '../www/core/raw-tutorial-one.js';

// 저장된 독립 기계어 출력을 한 번만 읽는다. originals 관찰만 쓴다. (2026-11-10 WB04-E237)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'tutorial-one-x86.tsv'));
const totals = rows ? rows.filter((row) => row[0] === 'Total' && row[1] === 'originals') : null;
const stages = rows ? rows.filter((row) => row[0] === 'Stage') : null;

// 원본 실행기에 전달한 독립 카운터 입력을 같은 DWORD 로 준비한다(기대 결과는 계산하지 않는다). (2026-11-10 WB04-E237)
const inputCount = (index, kind) => Math.imul(index + 1, 0x1020304 + kind * 17) >>> 0;
// 같은 실제 표의 전체 1024 BYTE 를 원본 fixture 와 비교할 문자열로 만든다. (2026-11-10 WB04-E237)
function tableHex(table) {
    const bytes = new Uint8Array(table.length * 4);
    for (let index = 0; index < table.length; index += 1) put(bytes, index * 4, table[index], 4);
    return Array.from(bytes, (value) => value.toString(16).padStart(2, '0')).join('');
}
// fixture 의 원본 float32 바이트(8 hex)를 반올림하지 않고 읽는다. (2026-11-10 WB04-E237)
function inputFloat(text) {
    const view = new DataView(new ArrayBuffer(4));
    for (let index = 0; index < 4; index += 1) view.setUint8(index, parseInt(text.slice(index * 2, index * 2 + 2), 16));
    return view.getFloat32(0, true);
}
// 10 진 문자열을 부호 있는 DWORD 로 줄인다(원본 stoll → int32). (2026-11-10 WB04-E237)
const int32 = (text) => Number(BigInt.asIntN(32, BigInt(text)));
// 188칸 TYPE 표에 원본 입력과 같은 부류(1 << (i % 6))를 준다. (2026-11-10 WB04-E237)
const typeTable = () => Array.from({ length: 188 }, (_, index) => ({ flags2: (1 << (index % 6)) >>> 0 }));
// 장부의 세 표와 허용 표를 독립 입력값으로 채운다. (2026-11-10 WB04-E237)
function fillTables(storage, restrictions) {
    for (let index = 0; index < 256; index += 1) {
        storage.localCounts[index] = inputCount(index, 0);
        storage.localSecondaryCounts[index] = inputCount(index, 1);
        storage.globalCounts[index] = inputCount(index, 2);
        restrictions[index] = inputCount(index, 3);
    }
}

test('Totalmade: 원본 10.78 52행(허용 표·집계 표 여덟 연산)의 반환값·진단 횟수·네 표 전체가 모두 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    let count = 0;
    for (const row of totals) {
        assert.equal(row.length, 11);
        const state = new SquidPostPopState();
        const storage = new RawTotalMadeStorage();
        state.bindTotalMade(storage);
        const restrictions = new Uint32Array(256);
        fillTables(storage, restrictions);
        let assertions = 0;
        const totalMade = new RawTotalMade(typeTable(), state, restrictions, () => { assertions += 1; });
        const operation = Number(row[2]);
        const first = int32(row[3]);
        const second = int32(row[4]);
        let result = 0;
        switch (operation) {
        case 0: totalMade.allowAll(first); break;
        case 1: totalMade.allow(first, second); break;
        case 2: result = Number(totalMade.allowed(first)); break;
        case 3: totalMade.resetCounts(); break;
        case 4: result = totalMade.local(first); break;
        case 5: result = totalMade.built(first); break;
        case 6: result = totalMade.global(first); break;
        case 7: result = totalMade.builtForGenus(first >>> 0) >>> 0; break;
        default: assert.fail(`알 수 없는 연산 ${operation}`);
        }
        count += 1;
        const label = `행 ${count} 연산 ${operation} 인자 ${row[3]},${row[4]}`;
        assert.equal(result, Number(row[5]), `${label} 반환`);
        assert.equal(assertions, Number(row[6]), `${label} 진단`);
        assert.equal(tableHex(storage.localCounts), row[7], `${label} 로컬`);
        assert.equal(tableHex(storage.localSecondaryCounts), row[8], `${label} 누적`);
        assert.equal(tableHex(storage.globalCounts), row[9], `${label} 전역`);
        assert.equal(tableHex(restrictions), row[10], `${label} 허용`);
    }
    assert.equal(count, 52);
});

test('Totalmade: 실제 reset 행을 세션 없는 helper 와 공유 장부 두 경로에 각각 재생해도 같고 제어/목록/미사용 독립 배열은 건드리지 않는다', { skip: !rows && '원본 fixture 없음' }, () => {
    let resets = 0;
    for (const row of totals) {
        if (Number(row[2]) !== 3) continue;
        const input = new RawTotalMadeStorage();
        const restrictions = new Uint32Array(256); // 256번째 물리 칸에도 다른 비트를 주어 보존 범위를 확인한다.
        fillTables(input, restrictions);
        const copyOf = (source) => { const target = new RawTotalMadeStorage(); target.localCounts.set(source.localCounts); target.localSecondaryCounts.set(source.localSecondaryCounts); target.globalCounts.set(source.globalCounts); return target; };
        // 독립 helper: 세 표만 255칸씩 지우고 같은 물리 배열이며 허용 표는 불변이다.
        let storage = copyOf(input);
        const addresses = [storage.localCounts, storage.localSecondaryCounts, storage.globalCounts];
        resetTotalMadeCounts(storage);
        assert.ok(tableHex(storage.localCounts) === row[7] && tableHex(storage.localSecondaryCounts) === row[8] && tableHex(storage.globalCounts) === row[9]);
        assert.ok(storage.localCounts === addresses[0] && storage.localSecondaryCounts === addresses[1] && storage.globalCounts === addresses[2] && tableHex(restrictions) === row[10]);
        // 공유 장부: 같은 장기 저장소만 지우고 독립 값 표·제어 필드·목록은 그대로이다.
        storage = copyOf(input);
        const counts = new SquidPostPopState();
        counts.bindTotalMade(storage);
        counts.graphsEnabled = true;
        counts.localOwner = 3;
        counts.depth = 7;
        counts.totalCost = 91;
        counts.productionDirty = 13;
        counts.providers.entries = [9, 10];
        counts.providers.count = 1;
        counts.ownerFactories[1].entries = [11, 12];
        counts.ownerFactories[1].count = 1;
        let assertions = 0;
        const totalMade = new RawTotalMade(typeTable(), counts, restrictions, () => { assertions += 1; });
        assert.ok(counts.localCounts() === storage.localCounts && counts.localSecondaryCounts() === storage.localSecondaryCounts && counts.globalCounts() === storage.globalCounts);
        totalMade.resetCounts();
        assert.ok(tableHex(storage.localCounts) === row[7] && tableHex(storage.localSecondaryCounts) === row[8] && tableHex(storage.globalCounts) === row[9]);
        assert.equal(assertions, Number(row[6]));
        assert.equal(tableHex(restrictions), row[10]);
        assert.ok(counts.graphsEnabled && counts.localOwner === 3 && counts.depth === 7 && counts.totalCost === 91 && counts.productionDirty === 13);
        assert.deepEqual([counts.providers.count, counts.providers.entries, counts.ownerFactories[1].count, counts.ownerFactories[1].entries], [1, [9, 10], 1, [11, 12]]);
        resets += 1;
    }
    assert.ok(resets > 0);
});

test('Early1 단계 판정기: 원본 10.78 266행의 단락 평가·반환·다중 Graph·SP 포함 경계·NaN 과 호출 순서가 모두 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    let count = 0;
    for (const row of stages) {
        assert.equal(row.length, 9);
        const state = { stage: Number(BigInt.asIntN(32, BigInt(row[1]))), localPlayer: 1, battleShown: Number(row[4]), homeCommand: 0x730000, reclaimCommand: 0x123456 };
        const players = new Uint8Array(9 * 0xb4);
        put(players, 0xb4 + 0x7c, 55);
        const graphs = new Map();
        const geysers = new SquidPostPopList();
        // Native 의 같은 현재 Graph 와 실제 목록 순서를 입력한다.
        for (const item of row[6].split(',')) {
            const [sid, graph] = item.split(':').map(Number);
            graphs.set(sid, graph);
            if (sid !== 55) geysers.entries.push(sid);
        }
        geysers.count = geysers.entries.length;
        const trace = []; // 실제 호출한 경계 순서를 보존한다.
        const hooks = {
            builtForGenus: (genus) => { assert.equal(genus, 4); trace.push('B'); return int32(row[3]); },
            graphFor: (sid) => { trace.push(`G${sid}`); if (!graphs.has(sid)) throw new RangeError('입력에 없는 Graph'); return graphs.get(sid); },
            money: (owner) => { trace.push(`M${owner}`); return inputFloat(row[5]); },
            advance: () => { trace.push('A'); },
            spawnGeysers: () => { trace.push('S'); },
        };
        const tutorial = new RawTutorialOne(state, players, geysers, hooks);
        const result = tutorial.handle(Number(row[2]));
        count += 1;
        assert.equal(result, Number(row[7]), `행 ${count} 반환`);
        assert.equal(trace.length ? trace.join(',') : '-', row[8], `행 ${count} 호출 순서`);
    }
    assert.equal(count, 266);
});

test('Early1 단계 판정기 계약: 상태·목록을 복제하지 않고 callback 뒤 현재 count·허용 표·장부를 읽는다', () => {
    const types = Array.from({ length: 188 }, () => ({ flags2: 0 }));
    types[82].flags2 = 4;
    const counts = new SquidPostPopState();
    const restrictions = new Uint32Array(256);
    const totalMade = new RawTotalMade(types, counts, restrictions);
    totalMade.allow(82, 0);
    assert.ok(!totalMade.allowed(82));
    totalMade.allow(82, 1);
    assert.ok(totalMade.allowed(82));
    counts.localSecondaryCounts()[82] = 19;
    assert.equal(totalMade.builtForGenus(4), 19);
    const state = { stage: 0x44, localPlayer: 1, battleShown: 1, homeCommand: 123, reclaimCommand: 456 }; // 'D'
    const players = new Uint8Array(9 * 0xb4);
    put(players, 0xb4 + 0x7c, 55);
    const geysers = new SquidPostPopList();
    geysers.entries = [80, 81, 82];
    geysers.count = 3;
    let advances = 0;
    let lookups = 0;
    const tutorial = new RawTutorialOne(state, players, geysers, {
        builtForGenus: (genus) => totalMade.builtForGenus(genus),
        graphFor: () => { lookups += 1; return 2; },
        money: () => 600,
        advance: () => { advances += 1; state.stage = 0x45; geysers.count = 1; }, // 'E', 목록이 줄어든다.
        spawnGeysers: () => assert.fail('생성은 C 단계에서만'),
    });
    assert.equal(tutorial.handle(0), 0);
    assert.ok(state.stage === 0x45 && advances === 1 && lookups === 2);
    assert.equal(tutorial.handle(0), 0);
    assert.equal(advances, 2);
});

test('Totalmade·Early1 계약: 허용 표 쓰기 범위·잘못된 타입 진단·빈 genus·판본/크기/장치 거부', () => {
    const types = typeTable();
    const state = new SquidPostPopState();
    const restrictions = new Uint32Array(256).fill(7);
    const seen = [];
    const totalMade = new RawTotalMade(types, state, restrictions, (kind) => { seen.push(kind); });
    // allowAll 은 첫 255칸만 쓰고 256번째 칸은 보존한다.
    totalMade.allowAll(5);
    assert.ok(restrictions.subarray(0, 255).every((value) => value === 0) && restrictions[255] === 7);
    totalMade.allowAll(0);
    assert.ok(restrictions.subarray(0, 255).every((value) => value === 1) && restrictions[255] === 7);
    // 허용 표 쓰기는 물리 범위 밖만 거부하고(타입 유효성은 보지 않음) 음수·표 밖은 쓰기 전에 거부한다.
    totalMade.allow(0, 1);
    assert.equal(restrictions[0], 0);
    assert.throws(() => totalMade.allow(256, 1), RangeError);
    assert.throws(() => totalMade.allow(-1, 1), RangeError);
    // 잘못된 타입(0·188)은 진단 뒤 거짓/0, 빈 genus 는 진단 뒤 0.
    assert.ok(!totalMade.allowed(0) && !totalMade.allowed(188));
    assert.ok(totalMade.local(0) === 0 && totalMade.built(188) === 0 && totalMade.global(-1) === 0);
    assert.equal(totalMade.builtForGenus(0), 0);
    assert.deepEqual(seen, [TotalMadeAssertion.InvalidType, TotalMadeAssertion.InvalidType, TotalMadeAssertion.InvalidType, TotalMadeAssertion.InvalidType, TotalMadeAssertion.InvalidType, TotalMadeAssertion.ZeroGenus]);
    // 진단 장치가 없으면 그 위치에서 거부한다. 판본(타입 수)/허용 표 크기도 거부한다.
    const silent = new RawTotalMade(types, state, restrictions);
    assert.throws(() => silent.allowed(0), TypeError);
    assert.throws(() => silent.builtForGenus(0), TypeError);
    assert.throws(() => new RawTotalMade(types.slice(0, 171), state, restrictions), TypeError);
    assert.throws(() => new RawTotalMade(types, state, new Uint32Array(254)), TypeError);
    assert.throws(() => setTotalMadeAllowAll(new Uint32Array(254), 1), RangeError);
    assert.throws(() => setTotalMadeAllow(restrictions, 1.5, 1), RangeError);
    // genus 합산은 DWORD 로 감기고(부호 있는 결과), 70 미만 타입과 겹치지 않는 부류는 더하지 않는다.
    const wrap = new SquidPostPopState();
    wrap.localSecondaryCounts()[70] = 0xffffffff;
    wrap.localSecondaryCounts()[71] = 2;
    wrap.localSecondaryCounts()[5] = 9;
    const wrapTypes = Array.from({ length: 188 }, () => ({ flags2: 0 }));
    wrapTypes[5].flags2 = 8;
    wrapTypes[70].flags2 = 8;
    wrapTypes[71].flags2 = 8 | 16;
    wrapTypes[72].flags2 = 16;
    assert.equal(new RawTotalMade(wrapTypes, wrap, new Uint32Array(256)).builtForGenus(8), 1);
    // 합이 2^31 이상이면 부호 있는 결과(음수)로 돌려준다.
    const big = new SquidPostPopState();
    big.localSecondaryCounts()[70] = 0x80000000;
    const bigTypes = Array.from({ length: 188 }, () => ({ flags2: 0 }));
    bigTypes[70].flags2 = 1;
    assert.equal(new RawTotalMade(bigTypes, big, new Uint32Array(256)).builtForGenus(1), -2147483648);
    // Early1 'D': 음수(부호 있는) count 는 순회하지 않고, 목록 물리 길이보다 큰 count 는 범위 오류이다.
    const listed = new SquidPostPopList();
    listed.entries = [80, 81, 82];
    const dState = { stage: 0x44, localPlayer: 1, battleShown: 0, homeCommand: 0, reclaimCommand: 0 };
    let dAdvances = 0;
    const dHooks = { builtForGenus: () => 0, graphFor: () => 5, money: () => 0, advance: () => { dAdvances += 1; }, spawnGeysers: () => {} };
    const dTutorial = new RawTutorialOne(dState, new Uint8Array(9 * 0xb4), listed, dHooks);
    listed.count = 0xffffffff;
    assert.equal(dTutorial.handle(0), 0);
    assert.equal(dAdvances, 0);
    listed.count = 4;
    assert.throws(() => dTutorial.handle(0), RangeError);
    assert.equal(dAdvances, 3);
    // Early1: Player 표 크기·필수 효과 누락·현재 Player 범위 거부.
    const geysers = new SquidPostPopList();
    const hooks = { builtForGenus: () => 0, graphFor: () => 0, money: () => 0, advance: () => {}, spawnGeysers: () => {} };
    const stateOne = { stage: 0x44, localPlayer: 9, battleShown: 0, homeCommand: 0, reclaimCommand: 0 };
    assert.throws(() => new RawTutorialOne(stateOne, new Uint8Array(10), geysers, hooks), TypeError);
    for (const name of Object.keys(hooks)) assert.throws(() => new RawTutorialOne(stateOne, new Uint8Array(9 * 0xb4), geysers, { ...hooks, [name]: undefined }), TypeError, name);
    assert.throws(() => new RawTutorialOne(stateOne, new Uint8Array(9 * 0xb4), geysers, hooks).handle(0), RangeError);
});
