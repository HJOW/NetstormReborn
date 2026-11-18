// 생산 유지 43ef10의 원본 86행에 대해 전체 내용물·188개 타이머·패널·효과 순서를 대조한다.
import './nan-exact.mjs';
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, bitsToFloat } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { ContainedFinderState } from '../www/core/raw-contained-finder.js';
import { selectBridgePattern } from '../www/core/raw-canon-type.js';
import { ProductionMaintenanceGlobals, ProductionMaintenancePanel, RawProductionMaintenance } from '../www/core/raw-production-maintenance.js';

// 부동소수 원본 비트를 정확하게 읽고 little endian 출력으로 변환한다.
function doubleBits(bits) { const view = new DataView(new ArrayBuffer(8)); view.setBigUint64(0, BigInt(bits), true); return view.getFloat64(0, true); }
// 현재 double 저장 바이트를 원본 fixture와 같은 16진수로 만든다.
function doubleHex(value) { const view = new DataView(new ArrayBuffer(8)); view.setFloat64(0, value, true); return Buffer.from(view.buffer).toString('hex'); }
// 원본 슬롯을 순서대로 16진수로 표현한다.
function hex(bytes) { return Buffer.from(bytes).toString('hex'); }

// 고정된 보호 원본 관측을 누락 없이 읽고 별개 풀에서 매 행을 실행한다.
test('생산 유지: 원본 86행의 내용물·타이머·패널·효과 순서', () => {
    const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'production-idle-x86.tsv')); assert.ok(rows); assert.equal(rows.length, 86);
    // 행별 현재 슬롯과 독립 전역을 만들고 기대값은 관측 파일에서만 읽는다.
    for (const row of rows) {
        assert.equal(row.length, 6); const values = row[0].split(',').map(BigInt); assert.equal(values.length, 27);
        const number = (index) => Number(BigInt.asUintN(32, values[index])); const real = (index) => doubleBits(values[index]);
        const pool = new SidPool(65535); const finder = new ContainedFinderState();
        // 관측 입력과 동일한 클라이언트 SID 5~8을 확보한다.
        for (let sid = 5; sid <= 8; sid += 1) { assert.equal(pool.allocate(2), sid); pool.allocated(sid).fill(0); }
        const parent = pool.allocated(5); put(parent, 6, 6, 2); parent[10] = 152; parent[34] = 1;
        // 다리와 두 생산 종류의 원문 내용물 체인을 복원한다.
        for (let index = 0; index < 3; index += 1) {
            const raw = pool.allocated(6 + index); put(raw, 0, 0x1240f000); put(raw, 4, index === 2 ? 0 : 7 + index, 2); raw[10] = 6;
            put(raw, 18, [82, 128, 133][index], 2); put(raw, 20, index ? 8 : number(20), 2); put(raw, 12, index ? 0 : number(22), 2); raw[30] = number(index ? 23 + index : 19);
        }
        put(pool.allocated(6), 31, number(21)); const globals = new ProductionMaintenanceGlobals(); globals.stockUntil = new Array(188).fill(0); globals.stockUntil[128] = globals.stockUntil[133] = real(26);
        Object.assign(globals, { stockEnabled: number(11), stockLimit: number(12) | 0, quickBridges: number(14), forcedPattern: number(15), sequence: number(16) });
        const panel = new ProductionMaintenancePanel(); Object.assign(panel, { bridgeUntil: real(10), oldMoney: number(17) | 0, slots: number(5) | 0, temple: number(4) });
        const reads = { active: () => number(0) === 1 || number(0) === 2, technology: () => number(2), templateTechnology: () => number(2), temple: () => number(3), editor: () => number(1), generatorTimer: () => number(7), stockRate: () => number(13), artificial: () => number(6) !== 0 };
        const events = [], hooks = { wallSeconds: () => real(8), gameSeconds: () => real(9), money: () => bitsToFloat(number(18)), touchDeck: () => {}, pattern: () => { events.push('R:10000'); return selectBridgePattern(1234); },
            create: (...args) => events.push(`C:${args.join(',')}`), destroy: (sid) => events.push(`D:${sid}`), allowed: (type) => Boolean((number(23) >>> Number(type === 133)) & 1), cost: (type) => type === 128 ? 400 : 200, refresh: (mask) => events.push(`U:${mask}`) };
        new RawProductionMaintenance(pool, 188, finder, globals, panel, reads, hooks).run();
        assert.equal(`${doubleHex(panel.bridgeUntil)},${panel.oldMoney >>> 0},${panel.temple}`, row[1]);
        assert.equal([6, 7, 8].map((sid) => hex(pool.slot(sid))).join(''), row[2]); assert.equal(globals.stockUntil.map(doubleHex).join(''), row[3]);
        assert.equal(String(globals.sequence), row[4]); assert.equal(events.join(';') || '-', row[5]);
    }
});

// 테스트 전용 순서의 0번, 순환, 해제와 잘못된 입력의 원자성을 확인한다.
test('생산 유지: 테스트 다리 순서 검증과 보존', () => {
    const globals = new ProductionMaintenanceGlobals(); assert.equal(globals.nextTestBridgePattern(), null); globals.configureTestBridgePatterns('3, 0,25');
    assert.deepEqual(Array.from({ length: 4 }, () => globals.nextTestBridgePattern()), [3, 0, 25, 3]);
    // 잘못된 필드는 현재 순서와 소비 횟수를 변경하지 않는다.
    for (const value of ['26', '-1', '1,', 'x', '1,,2']) { assert.throws(() => globals.configureTestBridgePatterns(value)); assert.equal(globals.testBridgeDraws, 4); assert.deepEqual(globals.testBridgePatterns, [3, 0, 25]); }
    globals.configureTestBridgePatterns('off'); assert.equal(globals.nextTestBridgePattern(), null); assert.equal(globals.testBridgeDraws, 0);
});

// 실제 난수 소비와 기본 한 칸 판정 뒤에만 테스트 지정 순서를 덮어쓴다.
test('생산 유지: 원래 추첨 뒤 테스트 조각을 적용하고 빈자리가 없으면 소비하지 않는다', () => {
    const pool = new SidPool(32768); assert.equal(pool.allocate(2), 5); pool.allocated(5).fill(0); pool.allocated(5)[10] = 152; pool.allocated(5)[34] = 1;
    const finder = new ContainedFinderState(), globals = new ProductionMaintenanceGlobals(), panel = new ProductionMaintenancePanel(), supplied = [];
    globals.forcedPattern = 12; globals.configureTestBridgePatterns('3,0'); panel.slots = 1; let randomCalls = 0;
    const reads = { active: () => true, technology: () => 5, templateTechnology: () => 5, temple: () => 1, editor: () => 0, generatorTimer: () => 0, stockRate: () => 0, artificial: () => false };
    const hooks = { wallSeconds: () => 100, gameSeconds: () => 100, money: () => 100, touchDeck: () => {}, pattern: () => { randomCalls += 1; return 7; },
        create: (_parent, _type, pattern) => supplied.push(pattern), destroy: () => {}, allowed: () => true, cost: () => 0, refresh: () => {} };
    const maintenance = new RawProductionMaintenance(pool, 188, finder, globals, panel, reads, hooks);
    maintenance.run(); maintenance.run(); assert.deepEqual(supplied, [3, 0]); assert.equal(randomCalls, 2); assert.equal(globals.sequence, 2);
    panel.slots = 0; maintenance.run(); assert.equal(globals.testBridgeDraws, 2); assert.equal(randomCalls, 2);
    globals.configureTestBridgePatterns('off'); panel.slots = 1; maintenance.run(); assert.equal(supplied.at(-1), 12); assert.equal(randomCalls, 3); assert.equal(globals.sequence, 3);
    assert.throws(() => new RawProductionMaintenance(pool, 188, finder, globals, panel, reads, { ...hooks, create: null }), /연결 누락/);
});
