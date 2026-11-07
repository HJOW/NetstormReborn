// 애시드 울타리 raw-acid-fence.js(RawAcidFence·postPopAcidFence)의 네 진입점을 독립 원본 관측 acid-fence-x86.tsv 1125행과 대조한다.
// 행 종류: P 50(최초 0.3 예약 후처리) · R 84(Regular: count 감김·마지막 프레임·중지 비트) · A/B 43+43(Attack: 두 축 첫 양방향 동맹, B 는 정수 좌표 경계) · S 903+C 2(Strike: 13영역·40개 중복·송신·수신·효과 뒤 재조회).
// 입력은 원본과 같은 47개 raw 슬롯(부모 50·이웃 60·61·후보 70~113)이며 모든 하위 효과(검색 I/F·예약 N/R·피해 H·송신 S·진단 !·프레임 B·소리 W·분해 X·삭제 K)는 원본 기록과 같은 문자열로 남긴다.
// 콜백이 바꾸는 현재 슬롯/전역/패킷만 수행하고 기대값은 계산하지 않는다. (2026-11-08 WB04-E50: RawAcidFence 이식과 함께 추가)
import assert from 'node:assert/strict';
import path from 'node:path';
import test from 'node:test';
import { postPopAcidFence, RawAcidFence } from '../www/core/raw-acid-fence.js';
import { RawAcidZap } from '../www/core/raw-acid-zap.js';
import { SidPool } from '../www/core/sid-pool.js';
import { REPO_ROOT } from './helpers.mjs';
import { bitsToFloat, floatToBits, get, put, readRows } from './raw-support.mjs';

// 바이트 배열을 소문자 hex 로 만든다. (2026-11-08 WB04-E50)
const hex = (bytes) => Array.from(bytes, (byte) => byte.toString(16).padStart(2, '0')).join('');

// 입력의 signed/unsigned 십진수를 원본 하위 32비트로 읽는다(float 비트를 해석하지 않는다). (2026-11-08 WB04-E50)
const numbers = (text) => text.split(',').map((value) => Number(BigInt.asUintN(32, BigInt(value))));

// 지역별 후보의 독립 타입이다(13개 영역을 각각 통과시킨다). (2026-11-08 WB04-E50)
const REGION_KINDS = [133, 144, 144, 73, 73, 138, 138, 144, 138, 73, 146, 141, 111];

// 원본 1125행을 재생한다. 몸체마다 콜백이 쓰는 입력만 구성하고 효과 문자열·47개 슬롯·반환 비트를 비교한다. 종류별 비교 행 수를 돌려준다. (2026-11-08 WB04-E50)
function replay() {
    const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'acid-fence-x86.tsv'));
    const kinds = new Map();
    // 원본 1125행을 순서대로 재생한다.
    rows.forEach((row, line) => {
        assert.equal(row.length, 5);
        const a = numbers(row[1]);
        const kind = row[0];
        const pool = new SidPool(32768, { server: false });
        // 부모(50)·이웃(60·61)·후보(70..113)가 쓰는 raw 슬롯을 확보한다.
        for (let value = 5; value <= 113; value += 1) assert.equal(pool.allocate(2), value);
        const parent = 50;
        const writable = (sid) => pool.physical(sid).subarray(0, 50);
        const state = { boss: true, editor: 0, localOwner: 1 };
        let friendly = 0;
        let x = 10;
        let y = 10;
        let bx = 14;
        let by = 15;
        let radius = 6;
        let mask = 3;
        let mode = 0;
        let mutation = 0;
        let allocation = 1;
        let fx = 3;
        let fy = 4;
        let total = 7;
        if (kind === 'P') allocation = a[2];
        if (kind === 'A' || kind === 'B') {
            radius = bitsToFloat(a[0]);
            x = bitsToFloat(a[1]);
            y = bitsToFloat(a[2]);
            mask = a[3];
            mode = a[4];
            state.boss = a[5] !== 0;
            state.editor = a[6];
            friendly = a[7];
            mutation = a[8];
            fx = a[9];
            fy = a[10];
        }
        if (kind === 'S' || kind === 'C') {
            mode = a[2];
            state.boss = a[3] !== 0;
            state.editor = a[4];
            friendly = a[5];
            mutation = a[6];
            x = bitsToFloat(a[7]);
            y = bitsToFloat(a[8]);
            bx = bitsToFloat(a[9]);
            by = bitsToFloat(a[10]);
            fx = a[11];
            fy = a[12];
            allocation = a[18];
        }
        if (kind === 'R') total = a[3];
        const types = Array.from({ length: 188 }, () => ({ flags1: 0, flags2: 0, footX: 1, footY: 1 }));
        const frames = Array.from({ length: 188 }, () => []);
        const spots = new Uint8Array(65536).fill(1);
        const alliances = new Uint32Array(81);
        // 순수 메타 입력이다.
        for (const [type, genus] of [[133, 0x50100], [158, 0x210000], [73, 0x100000], [144, 0x100000], [138, 0x100000], [146, 0x20000], [141, 0x20000], [111, 0x20000]]) types[type].flags2 = genus;
        const ids = [50, 60, 61];
        // 현재 변경 가능한 47개 슬롯이다.
        for (let sid = 70; sid < 114; sid += 1) ids.push(sid);
        // 원문의 50바이트 시작값을 독립 설치한다.
        for (const sid of ids) {
            const raw = writable(sid);
            raw.fill(0);
            put(raw, 0, 0x12200000, 4);
            raw[10] = sid < 70 ? 145 : 133;
            raw[34] = sid < 70 ? 1 : 2;
            put(raw, 14, floatToBits(sid === 60 ? bx : x), 4);
            put(raw, 18, floatToBits(sid === 60 ? by : sid === 61 ? Math.fround(y + 5) : y), 4);
        }
        if (kind === 'B') {
            put(writable(60), 14, floatToBits(Math.trunc(x) + 4), 4);
            put(writable(60), 18, floatToBits(Math.trunc(y)), 4);
            put(writable(61), 14, floatToBits(Math.trunc(x)), 4);
            put(writable(61), 18, floatToBits(Math.trunc(y) + 5), 4);
        }
        if (mode === 2 || mode === 5) {
            writable(71)[10] = 158;
            writable(72)[10] = 138;
            writable(73)[34] = 1;
            writable(74)[40] = 1;
            writable(75)[10] = 90;
            types[90].flags2 = 0x200;
        }
        if (mode === 4) {
            // 13개 각각을 통과시킨다.
            for (let index = 0; index < REGION_KINDS.length; index += 1) writable(70 + index)[10] = REGION_KINDS[index];
        }
        if (kind === 'S' && mode === 6) {
            types[a[14]].flags2 = a[13];
            const raw = writable(70);
            raw[10] = a[14];
            raw[11] = a[15];
            raw[40] = a[16];
            raw[34] = a[17];
        }
        // 독립 방향 현재값이다.
        for (const [first, second] of [[1, 1], [2, 2], [3, 3], [1, 3], [3, 1]]) alliances[first * 9 + second] = 1;
        types[145].footX = fx | 0;
        types[145].footY = fy | 0;
        // numFrames 관측 입력이다.
        frames[145] = new Array(total).fill({ side: 65 });
        let effects = '';
        let scans = 0;
        const special = { rainFlyer: 144, sunFlyer: 73, windFlyer: 138, rainBalloon: 146, windBalloon: 141, sunBalloon: 111 };
        // 원본이 받은 인수의 전체 DWORD 와 순서를 남긴다.
        const event = (name, values = []) => {
            if (effects) effects += ';';
            effects += name + values.map((value) => `:${value >>> 0}`).join('');
        };
        const hooks = {
            range: (sid) => {
                assert.equal(sid, parent);
                return radius;
            },
            find: (px, py, type) => {
                event('F', [floatToBits(px), floatToBits(py), type]);
                if ((mask & 1) && px === Math.trunc(x) + 4 && py === Math.trunc(y)) return 60;
                if ((mask & 2) && px === Math.trunc(x) && py === Math.trunc(y) + 5) return 61;
                return 0;
            },
            reserveRegular: () => {
                event('N', [40]);
                if (mutation === 8) state.boss = false;
                return allocation !== 0;
            },
            regular: (sid, ev, delay) => event('R', [ev, sid, floatToBits(delay)]),
            iterate: (area, consume) => {
                const phase = scans % 13;
                scans += 1;
                event('I', [area.left, area.top, area.right, area.bottom]);
                let victims = [];
                if (mode === 1 || mode === 6) victims = [70];
                else if (mode === 2) victims = [50, 60, 61, 70, 71, 72, 73, 74, 75];
                else if (mode === 3) for (let sid = 70; sid < 114; sid += 1) victims.push(sid);
                else if (mode === 4) victims = [70 + phase];
                else if (mode === 5) victims = [70, 70, 70, 71, 72];
                if (mutation === 4 && scans === 1) {
                    put(writable(parent), 14, floatToBits(11.25), 4);
                    put(writable(parent), 18, floatToBits(12.5), 4);
                }
                if (mutation === 5 && scans === 13) writable(70)[10] = 158;
                if (mutation === 9 && scans === 1) special.rainFlyer = 0;
                // 정확한 후보 순서만 주며 필터를 대체하지 않는다.
                for (const sid of victims) consume(sid);
            },
            damage: (target, damage, owner, source) => {
                event('H', [target, damage, owner, source]);
                if (mutation === 6) {
                    writable(target)[11] = 2;
                    writable(parent)[34] = 3;
                }
            },
            broadcast: (packet) => {
                if (effects) effects += ';';
                effects += `S:${hex(packet)}`;
                const victim = get(packet, 4, 2);
                if (mutation === 1) writable(victim)[11] = 1;
                if (mutation === 2) writable(parent)[34] = 3;
                if (mutation === 7) put(packet, 4, 71, 2);
            },
            assertion: (line) => event('!', [line]),
            setFrame: (sid, frame, flags) => {
                event('B', [frame, flags]);
                put(writable(sid), 36, frame >>> 0, 4);
            },
        };
        const zap = new RawAcidZap(pool, types, { altar: 164, growingIceTower: 150 }, { message: 0x61, protocol: 1, length: 11 }, {
            sound: (px, py, name, loop) => {
                event('W', [floatToBits(px), floatToBits(py)]);
                effects += `:${name}:${loop}`;
            },
            dissolve: (target, first, second, orientation) => event('X', [target, first, second, orientation]),
            destroy: (target, flags) => {
                event('K', [target, flags]);
                writable(target)[11] |= 2;
                if (mutation === 3) {
                    put(writable(parent), 14, floatToBits(11.5), 4);
                    put(writable(parent), 18, floatToBits(12.25), 4);
                }
            },
        });
        const acid = new RawAcidFence(pool, types, frames, spots, alliances, state, () => friendly, special, zap, hooks);
        let result = 0;
        if (kind === 'P') {
            writable(parent)[40] = a[1] & 0xff;
            postPopAcidFence(pool, parent, a[0], () => {
                event('N', [40]);
                return allocation !== 0;
            }, (sid, ev, delay) => event('R', [ev, sid, floatToBits(delay)]), (sid, flags) => event('D', [flags]));
        } else if (kind === 'R') {
            writable(parent)[40] = a[2] & 0xff;
            result = floatToBits(acid.handle(parent, a[0], a[1], bitsToFloat(a[4])));
        } else if (kind === 'A' || kind === 'B') {
            acid.attack(parent);
        } else {
            acid.strike(parent, [60, 61].slice(0, a[0]), a[1] | 0);
        }
        // 원문 부수효과 뒤 47슬롯 전체를 비교한다.
        const raws = ids.map((sid) => hex(pool.slot(sid))).join('');
        assert.equal(effects || '-', row[2], `애시드 원문 행 ${line}/${kind}:${row[1]} 효과`);
        assert.equal(raws, row[3], `애시드 원문 행 ${line}/${kind} 슬롯`);
        assert.equal(result, Number(row[4]), `애시드 원문 행 ${line}/${kind} 반환`);
        kinds.set(kind, (kinds.get(kind) ?? 0) + 1);
    });
    return kinds;
}

test('acid-fence: 최초 0.3 예약·Regular·Attack·13영역 Strike·송신/수신·효과 뒤 재조회를 원본 1125행과 대조한다', () => {
    const kinds = replay();
    assert.deepEqual(Object.fromEntries([...kinds].sort()), { A: 43, B: 43, C: 2, P: 50, R: 84, S: 903 });
});

// 계약 시험용 최소 구성이다. 부모(50)와 이웃(60) 슬롯을 만들고 호출 기록 훅을 연결한다. (2026-11-08 WB04-E50)
function contract(overrides = {}) {
    const pool = new SidPool(32768, { server: false });
    // 부모·이웃 raw 슬롯을 확보한다.
    for (let value = 5; value <= 63; value += 1) assert.equal(pool.allocate(2), value);
    const types = Array.from({ length: 188 }, () => ({ flags1: 0, flags2: 0, footX: 1, footY: 1 }));
    const frames = Array.from({ length: 188 }, () => []);
    const alliances = new Uint32Array(81);
    const calls = [];
    const hooks = {
        range: () => 6,
        find: () => 0,
        iterate: () => {},
        reserveRegular: () => true,
        regular: (...args) => calls.push(['regular', ...args]),
        damage: (...args) => calls.push(['damage', ...args]),
        broadcast: () => {},
        assertion: (line) => calls.push(['assert', line]),
        setFrame: (...args) => calls.push(['frame', ...args]),
        ...overrides.hooks,
    };
    for (const sid of [50, 60]) {
        const raw = pool.physical(sid);
        raw.fill(0);
        raw[10] = 145;
        raw[34] = 1;
        put(raw, 14, floatToBits(10), 4);
        put(raw, 18, floatToBits(10), 4);
    }
    const zap = new RawAcidZap(pool, types, { altar: 164, growingIceTower: 150 }, { message: 1, protocol: 1, length: 11 }, { sound() {}, dissolve() {}, destroy() {} });
    const state = { boss: true, editor: 0, localOwner: 1 };
    const spots = new Uint8Array(65536).fill(1);
    const special = { rainFlyer: 144, sunFlyer: 73, windFlyer: 138, rainBalloon: 146, windBalloon: 141, sunBalloon: 111 };
    return { pool, types, frames, spots, alliances, state, special, zap, hooks, calls, make: () => new RawAcidFence(pool, types, frames, spots, alliances, state, () => 0, special, zap, hooks) };
}

test('acid-fence: 구성 오류와 판본 불일치는 효과 전에 거부한다', () => {
    const c = contract();
    const build = (changes = {}) => new RawAcidFence(changes.pool ?? c.pool, changes.types ?? c.types, c.frames, c.spots, c.alliances, c.state, changes.friendlyFire ?? (() => 0), c.special, changes.zap ?? c.zap, c.hooks);
    assert.ok(build());
    assert.throws(() => build({ types: c.types.slice(1) }), TypeError);
    assert.throws(() => build({ friendlyFire: 1 }), TypeError);
    assert.throws(() => build({ zap: {} }), TypeError);
    assert.throws(() => build({ pool: { edition: 'original1037' } }), TypeError);
    assert.throws(() => postPopAcidFence({ edition: 'original1037' }, 50, 1, () => true, () => {}, () => {}), TypeError);
});

test('acid-fence: 후처리는 비트 9가 있으면 예약을 읽지 않고 승인되면 0.3 지연 Regular 를 건다', () => {
    const c = contract();
    c.pool.physical(50)[40] = 1;
    let reserved = 0;
    postPopAcidFence(c.pool, 50, 1, () => { reserved += 1; return true; }, () => assert.fail('호출되면 안 된다'), () => {});
    assert.equal(reserved, 0);
    c.pool.physical(50)[40] = 0;
    const seen = [];
    postPopAcidFence(c.pool, 50, 1, () => true, (sid, ev, delay) => seen.push([sid, ev, delay]), (sid, flags) => seen.push(['damageable', sid, flags]));
    assert.deepEqual(seen, [[50, 0, Math.fround(0.3)], ['damageable', 50, 1]]);
    // 예약이 승인되면 regular 훅이 필요하고 공통 damageable 은 항상 필요하다.
    assert.throws(() => postPopAcidFence(c.pool, 50, 1, () => true, undefined, () => {}), TypeError);
    assert.throws(() => postPopAcidFence(c.pool, 50, 0, () => true, () => {}, undefined), TypeError);
});

test('acid-fence: Regular 는 마지막 프레임 초과 시 0, 중지 비트에서는 period 만 돌려준다', () => {
    const c = contract();
    c.frames[145] = new Array(3).fill({ side: 65 });
    const fence = c.make();
    // count 1 → 2 는 마지막 프레임(2)과 같아 0 프레임으로 되돌린다.
    assert.equal(fence.handle(50, 1, 1, 0.75), 0.75);
    assert.deepEqual(c.calls, [['frame', 50, 0, 0]]);
    c.calls.length = 0;
    // count 2 → 3 은 마지막 프레임 2보다 크므로 0 이다.
    assert.equal(fence.handle(50, 1, 2, 0.75), 0);
    assert.deepEqual(c.calls, []);
    c.pool.physical(50)[40] = 0x20;
    assert.equal(fence.handle(50, 1, 1, 0.5), 0.5);
    assert.deepEqual(c.calls, []);
});

test('acid-fence: 사제는 피해 1000, 그 밖은 11바이트 명령을 송신한 뒤 로컬 수신하고 불변식 위반만 진단한다', () => {
    const c = contract({
        hooks: {
            iterate: (area, consume) => consume(61),
            broadcast: (packet) => { put(packet, 6, 62, 2); },
        },
    });
    c.types[145].flags2 = 0;
    c.types[133].flags2 = 0x50100;
    for (const sid of [61, 62]) {
        const raw = c.pool.physical(sid);
        raw.fill(0);
        raw[10] = 133;
        raw[34] = 2;
    }
    const fence = c.make();
    // 부모 소유자 1·후보 61 소유자 2(동맹 아님)는 첫 영역에서 수집되고 포획 명령이 송신된다. 송신 훅이 첫 폴을 62 로 바꾸면 진단 0x2fb 만 남는다.
    fence.strike(50, [60], 1);
    assert.ok(c.calls.some((call) => call[0] === 'assert' && call[1] === 0x2fb));
    assert.ok(!c.calls.some((call) => call[0] === 'assert' && call[1] !== 0x2fb));
});
