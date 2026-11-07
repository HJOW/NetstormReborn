// 아크 스파이어 raw-arc-spire.js(RawArcSpire·postPopArcSpire)의 다섯 원본 몸체를 독립 원본 관측 arc-spire-x86.tsv 665행과 대조한다.
// 행 종류: P 50(최초 Regular 예약·공통 후처리) · R 84(Regular: count 감김·반전 프레임·공격) · N 192(Collect: 양방향 동맹·좌표·가득 찬 버퍼) · A 243(Attack: 두 축 스캔·범위·편집 모드)
//        · S 90(Strike: 40개 적 후보·살아남은 체인) · T 6(정수 근처 float 경계). 각 행은 x87 두 정밀도 결과가 같은 것만 모았다.
// 입력은 원본과 같은 12개 raw 슬롯(부모 50·이웃 60·61·후보 70~113)이며 하위 효과는 원본 기록과 같은 문자열로 남긴다. 콜백이 바꾸는 현재 슬롯만 수행하고 기대값은 계산하지 않는다.
// (2026-11-08 WB04-E49: RawArcSpire 이식과 함께 추가)
import assert from 'node:assert/strict';
import path from 'node:path';
import test from 'node:test';
import { postPopArcSpire, RawArcSpire } from '../www/core/raw-arc-spire.js';
import { SidPool } from '../www/core/sid-pool.js';
import { REPO_ROOT } from './helpers.mjs';
import { bitsToFloat, floatToBits, put, readRows } from './raw-support.mjs';

// 바이트 배열을 소문자 hex 로 만든다. (2026-11-08 WB04-E49)
const hex = (bytes) => Array.from(bytes, (byte) => byte.toString(16).padStart(2, '0')).join('');

// 입력의 signed/unsigned 십진수를 원본 하위 32비트로 읽는다. (2026-11-08 WB04-E49)
const numbers = (text) => text.split(',').map((word) => Number(BigInt.asUintN(32, BigInt(word))));

// 큰 finder 가 원본과 이 시험 양쪽에 공급하는 독립 순서 입력이다(0 없음 · 1 부모/이웃 포함 5개 · 2 세 후보 · 4 여섯 후보 · 그 밖 44개). (2026-11-08 WB04-E49)
function candidates(mode) {
    if (!mode) return [];
    if (mode === 1) return [50, 60, 70, 71, 72];
    if (mode === 2) return [70, 71, 72];
    if (mode === 4) return [70, 71, 72, 73, 74, 75];
    const result = [];
    // 40개 포화 뒤의 추가 후보도 공급한다.
    for (let sid = 70; sid < 114; sid += 1) result.push(sid);
    return result;
}

// 원본 관측 665행을 재생한다. 몸체마다 콜백이 쓰는 입력만 구성하고 모든 효과 문자열·슬롯·반환·개수·버퍼를 비교한다. (2026-11-08 WB04-E49)
function replay() {
    const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'arc-spire-x86.tsv'));
    const kinds = new Map();
    // 원본 665행을 순서대로 재생한다.
    rows.forEach((row, line) => {
        assert.equal(row.length, 12);
        const a = numbers(row[1]);
        // 두 x87 정밀도 관측이 모두 같은 행만 들어 있다.
        for (let index = 2; index < 7; index += 1) assert.equal(row[index], row[index + 5]);
        const pool = new SidPool(32768, { server: false });
        // 부모(50)·이웃(60·61)·후보(70..113)가 쓰는 raw 슬롯을 확보한다.
        for (let value = 5; value <= 113; value += 1) assert.equal(pool.allocate(2), value);
        const parent = 50;
        const writable = (sid) => pool.physical(sid).subarray(0, 50);
        const types = Array.from({ length: 188 }, () => ({ flags1: 0x10, flags2: 0, footX: 1, footY: 1 }));
        const frames = Array.from({ length: 188 }, () => []);
        const spots = new Uint8Array(65536).fill(row[0] === 'A' ? 1 : 0);
        const alliances = new Uint32Array(81);
        const state = { editor: 0 };
        let mask = 0;
        let mode = 0;
        let mutation = 0;
        let x = 10;
        let y = 10;
        let radius = 6;
        let rate = 41.5;
        let period = 0.75;
        if (row[0] === 'A') {
            radius = bitsToFloat(a[0]);
            x = bitsToFloat(a[1]);
            y = bitsToFloat(a[2]);
            mask = a[3];
            mode = a[4];
            state.editor = a[5];
            mutation = a[6];
            period = bitsToFloat(a[7]);
            rate = bitsToFloat(a[8]);
        }
        if (row[0] === 'S' || row[0] === 'T') {
            mode = a[1];
            state.editor = a[2];
            mutation = a[3];
        }
        if (row[0] === 'R') period = bitsToFloat(a[4]);
        // 실제 4adc60 의 기본 damageable 메타이다.
        for (const type of types) type.flags1 = 0x10;
        const setup = (sid) => {
            const raw = writable(sid);
            raw.fill(0);
            put(raw, 0, 0x12200000, 4);
            raw[10] = sid < 70 ? 149 : 78;
            raw[34] = sid < 70 ? 1 : 2;
            put(raw, 14, floatToBits(Math.fround(x + (sid === 60 ? 4 : 0))), 4);
            put(raw, 18, floatToBits(Math.fround(y + (sid === 61 ? 5 : 0))), 4);
        };
        // 부모와 두 축의 이웃이다.
        for (const sid of [50, 60, 61]) setup(sid);
        // 큰 finder 가 읽을 모든 후보를 준비한다.
        for (let sid = 70; sid < 114; sid += 1) setup(sid);
        if (mode === 1) {
            writable(71)[10] = 79;
            types[79].flags2 = 0x120000;
            writable(72)[40] = 1;
        }
        if (mode === 4) {
            types[78].flags1 = 0;
            writable(71)[40] = 1;
            writable(72)[34] = 1;
            writable(73)[10] = 91;
            types[91].flags2 = 0x400000;
            writable(74)[10] = 92;
            types[92].flags2 = 0x120000;
            writable(75)[10] = 93;
        }
        if (row[0] === 'T') {
            // 실제 정수 근처 float 입력만 경계에 공급한다.
            const offset = bitsToFloat(a[5]);
            put(writable(60), 14, floatToBits(Math.fround(14 + offset)), 4);
            put(writable(60), 18, floatToBits(Math.fround(10 + offset)), 4);
        }
        // 반대 방향도 별도 메타이다.
        for (const [first, second] of [[1, 1], [2, 2], [3, 3], [1, 3], [3, 1]]) alliances[first * 9 + second] = 1;
        // 현재 TYPE.numFrames 의 입력이다.
        frames[149] = new Array(row[0] === 'R' ? a[3] : 6).fill({ side: 65, variant: 0, number: 0, flags: 0 });
        let effects = '';
        // 원본 효과 인자를 DWORD 십진수로 기록한다.
        const event = (name, values = []) => {
            if (effects) effects += ';';
            effects += name + values.map((value) => `:${value >>> 0}`).join('');
        };
        const hooks = {
            range: (sid) => {
                assert.equal(sid, parent);
                return radius;
            },
            damageRate: (sid) => {
                assert.equal(sid, parent);
                event('A', [floatToBits(period)]);
                return rate;
            },
            find: (px, py, type) => {
                event('F', [floatToBits(px), floatToBits(py), type]);
                if (row[0] === 'N') return a[6] ? 60 : 0;
                if ((mask & 1) && px === Math.trunc(x) + 4 && py === Math.trunc(y)) return 60;
                if ((mask & 2) && px === Math.trunc(x) && py === Math.trunc(y) + 5) return 61;
                return 0;
            },
            iterate: (area, consume) => {
                event('I', [area.left, area.top, area.right, area.bottom]);
                // 실제 finder 경계의 독립 후보 순서만 공급한다.
                for (const sid of candidates(mode)) consume(sid);
            },
            beam: (source, target) => {
                event('L', [source, target]);
                if (mutation === 1 && target === 70) writable(target)[11] = 1;
            },
            damage: (target, value, owner, source) => {
                event('H', [target, value, owner, source]);
                if (mutation === 2 && target === 70) {
                    writable(target)[11] = 2;
                    writable(parent)[34] = 3;
                }
            },
            setFrame: (sid, frame, flags) => {
                assert.equal(sid, parent);
                event('B', [frame, flags]);
                put(writable(sid), 36, frame >>> 0, 4);
            },
            damageable: (sid, ev, count, value) => {
                assert.equal(sid, parent);
                event('C', [ev, count, floatToBits(value)]);
                return 0.75;
            },
        };
        const arc = new RawArcSpire(pool, types, frames, spots, alliances, state, hooks);
        let result = 0;
        let count = 0;
        let buffer = '-';
        if (row[0] === 'P') {
            writable(parent)[40] = a[1] & 0xff;
            postPopArcSpire(pool, parent, a[0], () => {
                event('N', [40]);
                return a[2] !== 0;
            }, (sid, ev, value) => event('R', [ev, sid, floatToBits(value)]), (sid, flags) => {
                assert.equal(sid, parent);
                event('D', [flags]);
            });
        } else if (row[0] === 'R') {
            writable(parent)[40] = a[2] & 0xff;
            result = floatToBits(arc.handle(parent, a[0], a[1], period));
        } else if (row[0] === 'N') {
            const raw = writable(60);
            raw[40] = a[0] & 0xff;
            raw[34] = 3;
            put(raw, 14, floatToBits(a[3] ? 14.25 : 14.0), 4);
            alliances[10] = a[1];
            alliances[12] = a[1];
            alliances[28] = a[2];
            const values = new Array(a[4]).fill(0xa5a5);
            const collected = arc.collect(parent, values, a[5], 14, 10, 149);
            result = collected.found ? 1 : 0;
            count = collected.count;
            const original = new Uint8Array(80).fill(0xa5);
            // 호스트 WORD SID 를 원본 DWORD 항목으로 정규화한다.
            for (let index = 0; index < values.length; index += 1) if (values[index] !== 0xa5a5) put(original, index * 4, values[index], 4);
            buffer = hex(original);
        } else if (row[0] === 'A') {
            arc.attack(parent, period);
        } else {
            arc.strike(parent, [60, 61].slice(0, a[0]), a[4] | 0);
        }
        // 표시 변경과 효과 뒤 현재 owner/상태도 대조한다.
        const raws = [50, 60, 61, 70, 71, 72].map((sid) => hex(pool.slot(sid))).join('');
        assert.equal(effects || '-', row[2], `아크 원문 행 ${line}/${row[0]} 효과`);
        assert.equal(raws, row[3], `아크 원문 행 ${line}/${row[0]} 슬롯`);
        assert.equal(result, Number(row[4]), `아크 원문 행 ${line}/${row[0]} 반환`);
        assert.equal(count, Number(row[5]), `아크 원문 행 ${line}/${row[0]} 개수`);
        assert.equal(buffer, row[6], `아크 원문 행 ${line}/${row[0]} 버퍼`);
        kinds.set(row[0], (kinds.get(row[0]) ?? 0) + 1);
    });
    return kinds;
}

test('arc-spire: 최초 예약·Regular·Collect·Attack·Strike·경계 float를 원본 665행과 대조한다', () => {
    const kinds = replay();
    assert.deepEqual(Object.fromEntries([...kinds].sort()), { A: 243, N: 192, P: 50, R: 84, S: 90, T: 6 });
});

// 계약 시험용 최소 구성이다. 슬롯 50(부모 149)·60(이웃)과 모든 훅을 호출 기록으로 만든다. (2026-11-08 WB04-E49)
function contract(overrides = {}) {
    const pool = new SidPool(32768, { server: false });
    // 부모·이웃 raw 슬롯을 확보한다.
    for (let value = 5; value <= 61; value += 1) assert.equal(pool.allocate(2), value);
    const types = Array.from({ length: 188 }, () => ({ flags1: 0x10, flags2: 0, footX: 1, footY: 1 }));
    const frames = Array.from({ length: 188 }, () => []);
    const alliances = new Uint32Array(81);
    alliances[1 * 9 + 1] = 1;
    const calls = [];
    const hooks = {
        range: () => 6,
        damageRate: () => 40,
        find: () => 0,
        iterate: () => {},
        beam: (source, target) => calls.push(['beam', source, target]),
        damage: (...args) => calls.push(['damage', ...args]),
        setFrame: (...args) => calls.push(['frame', ...args]),
        damageable: (...args) => {
            calls.push(['damageable', ...args]);
            return 0.5;
        },
        ...overrides.hooks,
    };
    for (const sid of [50, 60]) {
        const raw = pool.physical(sid);
        raw.fill(0);
        raw[10] = 149;
        raw[34] = 1;
        put(raw, 14, floatToBits(overrides.x ?? 10), 4);
        put(raw, 18, floatToBits(10), 4);
    }
    const state = { editor: 0 };
    const spots = new Uint8Array(65536).fill(1);
    return { pool, types, frames, spots, alliances, state, hooks, calls, arc: new RawArcSpire(pool, types, frames, spots, alliances, state, hooks) };
}

test('arc-spire: 구성 오류와 판본 불일치는 효과 전에 거부한다', () => {
    const { pool, types, frames, spots, alliances, state, hooks } = contract();
    assert.throws(() => new RawArcSpire(pool, types.slice(1), frames, spots, alliances, state, hooks), TypeError);
    assert.throws(() => new RawArcSpire(pool, types, frames.slice(1), spots, alliances, state, hooks), TypeError);
    assert.throws(() => new RawArcSpire(pool, types, frames, spots.slice(1), alliances, state, hooks), TypeError);
    assert.throws(() => new RawArcSpire(pool, types, frames, spots, alliances.slice(1), state, hooks), TypeError);
    assert.throws(() => new RawArcSpire(pool, types, frames, spots, alliances, null, hooks), TypeError);
    assert.throws(() => new RawArcSpire({ edition: 'original1037' }, types, frames, spots, alliances, state, hooks), TypeError);
    assert.throws(() => postPopArcSpire({ edition: 'original1037' }, 50, 1, () => true, () => {}, () => {}), TypeError);
});

test('arc-spire: 후처리는 상태 비트 9가 있으면 예약을 읽지 않고 필요한 훅은 도달할 때만 요구한다', () => {
    const { pool } = contract();
    pool.physical(50)[40] = 8;
    let reserved = 0;
    const seen = [];
    postPopArcSpire(pool, 50, 1, () => { reserved += 1; return true; }, () => seen.push('regular'), (sid, flags) => seen.push(['damageable', sid, flags]));
    assert.equal(reserved, 0);
    assert.deepEqual(seen, [['damageable', 50, 1]]);
    pool.physical(50)[40] = 0;
    // 예약이 거절되면 regular 는 없어도 된다.
    postPopArcSpire(pool, 50, 1, () => false, undefined, () => {});
    // 예약이 승인되면 regular 훅이 필요하다.
    assert.throws(() => postPopArcSpire(pool, 50, 1, () => true, undefined, () => {}), TypeError);
    // 공통 damageable 은 항상 필요하다.
    assert.throws(() => postPopArcSpire(pool, 50, 0, () => true, () => {}, undefined), TypeError);
});

test('arc-spire: Regular 는 프레임 수 초과 시 0을 돌려주고 중지 비트에서는 공통 후처리만 한다', () => {
    const { pool, frames, arc, calls } = contract();
    frames[149] = new Array(3).fill({ side: 65 });
    // event1 의 count 2 → 3 이면 (3−3)=0 프레임으로 되돌린다.
    assert.equal(arc.handle(50, 1, 2, 0.75), 0.5);
    assert.deepEqual(calls, [['frame', 50, 0, 0], ['damageable', 50, 1, 3, 0.75]]);
    calls.length = 0;
    // count 3 → 4 는 총 프레임 3보다 크므로 공통 후처리 없이 0 이다.
    assert.equal(arc.handle(50, 1, 3, 0.75), 0);
    assert.deepEqual(calls, []);
    // 0xffffffff 는 DWORD 로 감겨 0 이 되어 (3−0)=3 프레임이다.
    assert.equal(arc.handle(50, 1, 0xffffffff, 0.75), 0.5);
    assert.deepEqual(calls.at(0), ['frame', 50, 3, 0]);
    calls.length = 0;
    pool.physical(50)[40] = 0x20;
    assert.equal(arc.handle(50, 1, 1, 0.75), 0.5);
    assert.deepEqual(calls, [['damageable', 50, 1, 1, 0.75]]);
});

test('arc-spire: 지도 밖 축 스캔은 효과 전에 거부하고 hook 누락은 도달한 지점에서만 요구한다', () => {
    const { arc } = contract({ x: -5 });
    assert.throws(() => arc.attack(50, 0.75), RangeError);
    const fine = contract({ hooks: { iterate: undefined } });
    // 이웃이 없으면 iterate 도 필요하지 않다.
    fine.arc.attack(50, 0.75);
    // 이웃이 있으면 iterate 가 필요하다.
    assert.throws(() => fine.arc.strike(50, [60], 1), TypeError);
    // 동맹 owner 가 9 이상이면 범위 오류이다.
    const c = contract({ hooks: { find: () => 60 } });
    c.pool.physical(60)[34] = 9;
    assert.throws(() => c.arc.collect(50, [], 0, 10, 10, 149), RangeError);
});
