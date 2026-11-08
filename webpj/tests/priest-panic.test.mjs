// 사제 공황 이동 raw-priest-panic.js(RawPriestPanic)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: priest-panic-x86.tsv(10.78 전용 단일 판본) 600행 — 아홉 입력(신전 유무·현재 HP·소유자 일치·임시 속도 비트·경로 결과·땅/영역 변형·난수 seed·변이 단계) → 호출 순서(T/H/V/L/G/R/P),
//    사제와 신전 슬롯 두 개의 raw, 두 TYPE 속도 비트, 난수 상태. 큰 하위(신전/최대 HP/경로/땅/영역/난수/점 경로)는 명시 경계로 값과 호출만 기록한다.
//  - 계약: 풀·속도 표 크기 거부, 도달한 하위 누락 진단.
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E66: 사제 공황 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { GameRandom } from '../www/core/random.js';
import { RawPriestPanic } from '../www/core/raw-priest-panic.js';

// 원본 관측 행이다(단일 판본 파일). 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E66)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'priest-panic-x86.tsv'));

// 원본 관찰 사제/신전 SID, 독립 행 수이다. (2026-11-08 WB04-E66)
const ACTOR = 50;
const TEMPLE = 60;
const ROWS = 600;

// 예외가 나는지 본다. (2026-11-08 WB04-E66)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 바이트 배열을 소문자 hex 로 만든다. (2026-11-08 WB04-E66)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 단정도 비트를 수로 바꾼다. (2026-11-08 WB04-E66)
const bitsToFloat = (bits) => {
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, bits >>> 0, true);
    return view.getFloat32(0, true);
};

test('PriestPanic: 원본 10.78 600행의 호출 순서·사제/신전 raw·두 TYPE 속도 비트·난수 상태가 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    let count = 0;
    // 결과를 쓰지 않고 명시 경계의 입력·현재 자료 변화만 공급한다.
    for (const row of rows) {
        assert.equal(row.length, 5);
        const c = row[0].split(',').map(Number);
        assert.equal(c.length, 9);
        const pool = new SidPool(24000, { server: true });
        const actor = pool.physical(ACTOR);
        const temple = pool.physical(TEMPLE);
        actor.fill(0);
        temple.fill(0);
        actor[10] = 158;
        put(actor, 26, c[1]);
        actor[34] = 1;
        temple[34] = c[2] ? 1 : 2;
        put(actor, 14, floatToBits(50.25));
        put(actor, 18, floatToBits(50.75));
        const speeds = Array.from({ length: 188 }, () => ({ speed: 0 }));
        speeds[158].speed = bitsToFloat(c[3]);
        speeds[159].speed = 2.5;
        const random = new GameRandom(c[7]);
        let lands = 0;
        let regions = 0;
        let randoms = 0;
        const effects = [];
        // 원문 호출 순서의 전체 DWORD 비트를 기록한다.
        const event = (tag, ...values) => effects.push([tag, ...values].join(':'));
        const hooks = {
            temple: (x, y) => {
                event('T', floatToBits(x), floatToBits(y));
                if (c[8] === 1) { put(actor, 14, floatToBits(51.25)); put(actor, 26, 20); actor[34] = 2; temple[34] = 2; }
                return c[0] ? TEMPLE : 0;
            },
            maximum: (sid) => {
                event('H', sid);
                if (c[8] === 2) { actor[10] = 159; actor[34] = 2; temple[34] = 2; }
                return 100;
            },
            target: (sid, target) => {
                event('V', sid, target, 0, 0, floatToBits(speeds[actor[10]].speed));
                if (c[8] === 3) { actor[10] = 159; put(actor, 18, floatToBits(52.5)); speeds[158].speed = 9.25; }
                return c[4];
            },
            land: (x, y) => {
                event('L', floatToBits(x), floatToBits(y));
                lands += 1;
                if (lands === 1 && c[8] === 4) { put(actor, 14, floatToBits(61.25)); put(actor, 18, floatToBits(62.5)); }
                return !(c[5] === 0 || (c[5] === 1 && (lands === 2 || lands === 3)));
            },
            region: (x, y) => {
                event('G', floatToBits(x), floatToBits(y));
                regions += 1;
                if (c[8] === 6 && regions === 1) { put(actor, 14, floatToBits(81.25)); put(actor, 18, floatToBits(82.5)); }
                return c[5] === 3 && (regions === 1 || regions === 3 || regions === 5) ? 2 : 1;
            },
            random: (low, high) => {
                event('R', floatToBits(low), floatToBits(high));
                randoms += 1;
                if (c[8] === 5 && randoms === 1) { put(actor, 14, floatToBits(71.25)); put(actor, 18, floatToBits(72.5)); }
                return (random.next(65536) / 65536.0) * Math.abs(high - low) + low;
            },
            point: (sid, x, y) => {
                event('P', sid, floatToBits(x), floatToBits(y), 0, 0, 0, 0, 0);
                if (c[8] === 7) actor[34] = 3;
                return c[6];
            }
        };
        new RawPriestPanic(pool, speeds, hooks).run(ACTOR);
        const speedBits = `${floatToBits(speeds[158].speed)},${floatToBits(speeds[159].speed)}`;
        assert.equal(effects.join(';'), row[1], `행 ${count}: ${row[0]}`);
        assert.equal(hex(actor) + hex(temple), row[2], `행 ${count}: ${row[0]}`);
        assert.equal(speedBits, row[3], `행 ${count}: ${row[0]}`);
        assert.equal(String(random.state), row[4], `행 ${count}: ${row[0]}`);
        count += 1;
    }
    assert.equal(count, ROWS);
});

test('PriestPanic: 풀·속도 표 크기를 구성 때 거부하고 도달한 하위 누락만 그 시점에 거부한다', () => {
    const pool = new SidPool(24000, { server: true });
    const speeds = Array.from({ length: 188 }, () => ({ speed: 1 }));
    assert.ok(throws(() => new RawPriestPanic(pool, speeds.slice(1), {})));
    assert.ok(throws(() => new RawPriestPanic({ edition: 'cd1072' }, speeds, {})));
    // 신전이 없고 땅이 아니면 임의 이동은 첫 후보에서 점 경로를 요구한다. 누락은 앞선 효과 뒤에 진단한다.
    const seen = [];
    const panic = new RawPriestPanic(pool, speeds, { temple: () => 0, land: () => false, random: () => { seen.push('R'); return 0; } });
    assert.ok(throws(() => panic.run(ACTOR)));
    assert.deepEqual(seen, ['R', 'R']);
    // 속도가 비유한이면 임시 속도 쓰기 전에 거부하고 원래 속도를 보존한다.
    const actor = pool.physical(ACTOR);
    actor.fill(0);
    actor[10] = 158;
    put(actor, 26, 1);
    actor[34] = 1;
    pool.physical(TEMPLE).fill(0);
    pool.physical(TEMPLE)[34] = 1;
    speeds[158].speed = Infinity;
    assert.ok(throws(() => new RawPriestPanic(pool, speeds, { temple: () => TEMPLE, maximum: () => 100 }).run(ACTOR)));
    assert.equal(speeds[158].speed, Infinity);
});
