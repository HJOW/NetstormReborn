// Dude 정기 처리 raw-dude-regular.js(RawDudeRegular)를 원본 10.78 관측 1600행과 cpppj 계약으로 대조한다.
//  - 원본 관측: dude-regular-x86.tsv — 몸체(0 Handle / 1 Wander / 2 Return / 3 Drop / 4 Fall / 5 MoveTo / 6 정적 초기화)와 17개 입력(상태·사건·count·payload·공급자 수·순환 값·난수·프레임·지면/경로/거주/이동 결과·재진입 변경)으로
//    반환 float 비트, 큰 효과의 인자 순서(space/lookup/task/nearest/point/move/frame/destroy/unpop/pop/assert), 슬롯 50 바이트 전체, 난수 상태, 순환 값이 같다(두 x87 정밀도 열 동일). 정적 초기화 3행은 30칸/count 0 이다.
//  - 계약: 판본·프레임 표 크기 거부, 다른 파생 자산 거부, 미연결 하위 효과 거부, 중복 초기화 거부.
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E122: Dude 정기 처리 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get, bitsToFloat, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { GameRandom } from '../www/core/random.js';
import { SquidPostPopList } from '../www/core/squid-postpop-state.js';
import { FrameCodeTable } from '../www/assets/terrain-tiles.js';
import { RawDudeRegular, initializeDudePopulation } from '../www/core/raw-dude-regular.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E122)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'dude-regular-x86.tsv')) ?? [];
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 예외가 나는지 본다. (2026-11-08 WB04-E122)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// NaN 은 단정도 비트 payload 를 JS 수로 표현할 수 없으므로 같은 NaN 부류끼리 같은 것으로 본다. (2026-11-08 WB04-E122)
function sameBits(actual, expected) {
    const isNan = (bits) => ((bits >>> 23) & 0xff) === 0xff && (bits & 0x7fffff) !== 0;
    return actual === expected || (isNan(actual) && isNan(expected));
}

test('DudeRegular: 원본 10.78 1600입력의 정기/하위 분배 반환·효과 순서·슬롯·난수·순환 값이 같다', { skip: rows.length === 0 && '원본 fixture 없음' }, () => {
    const pool = new SidPool(24000, { server: false });
    // 원문에 쓰인 현재 부모/목표 번호를 실제 client 목록에서 확보한다.
    for (let number = 5; number <= 53; number += 1) assert.equal(pool.allocate(2), number);
    const frames = Array.from({ length: 188 }, () => new FrameCodeTable([]));
    frames[70] = new FrameCodeTable([{ side: 65, variant: 80, flags: 0 }, { side: 65, variant: 80, flags: 0 }, { side: 65, variant: 80, flags: 0 },
        { side: 66, variant: 80, flags: 0 }, { side: 66, variant: 80, flags: 0 }, { side: 73, variant: 80, flags: 0 }]);
    const raw = pool.physical(50);
    let count = 0;
    let initializers = 0;
    // 최초 1600행과 53/64비트 결과를 빠짐없이 비교한다.
    for (const row of rows) {
        assert.equal(row.length, 3);
        assert.equal(row[1], row[2]);
        const fields = row[0].split(',');
        assert.equal(fields.length, 17);
        const input = fields.map((text) => Number(text) >>> 0);
        const expected = row[1].split('|');
        assert.equal(expected.length, 6);
        if (input[0] === 6) {
            const native = expected[5].split(',').map(Number);
            assert.equal(native.length, 4);
            const population = new SquidPostPopList();
            initializeDudePopulation(population);
            assert.ok(native[0] === 0x508c04 && native[1] === 1);
            assert.ok(population.entries.length === native[2] && population.count === native[3]);
            assert.ok(throws(() => initializeDudePopulation(population)));
            initializers += 1;
            count += 1;
            continue;
        }
        raw.fill(0xa5);
        put(raw, 0, 0x508cc8);
        raw[10] = 70;
        raw[11] = input[4] & 0xff;
        put(raw, 12, 0x5102, 2);
        put(raw, 14, floatToBits(20.75));
        put(raw, 18, input[16]);
        raw[34] = 2;
        put(raw, 36, input[13]);
        // 목표 진단은 일반 목표 이동 wrapper 의 원본 입력이며 free/dead 에서도 raw 읽기를 허용한다.
        for (let number = 51; number <= 53; number += 1) {
            const target = pool.physical(number);
            target.fill(0);
            target[10] = 71;
            target[11] = input[15] & 0xff;
        }
        const random = new GameRandom(input[12]);
        const providers = new SquidPostPopList();
        providers.entries = [51, 52, 53];
        providers.count = input[10];
        const state = { checking: input[5] !== 0, providers, providerIndex: input[11] | 0, pollEvent: 0x1a2b, pathType: 24 };
        const trace = [];
        const event = (name, values) => trace.push([name, ...values].join(':'));
        const hooks = {
            checkSpace: (sid) => { event('space', [sid]); return input[7] !== 0; },
            findPath: (sid, type) => { event('lookup', [sid, type, 0, 0]); return input[6] ? 52 : 0; },
            task: (sid, task, delay) => { event('task', [sid, task, floatToBits(delay)]); put(raw, 12, task, 2); },
            nearestResidence: (sid) => {
                const current = pool.slot(sid);
                event('nearest', [get(current, 14), get(current, 18), 0x59a9d8, current[34]]);
                return input[8] ? 51 : 0;
            },
            entrance: (sid) => { event('point', [sid]); return { x: 51.25, y: 52.75 }; },
            move: (sid, point, quickly) => { event('move', [sid, floatToBits(point.x), floatToBits(point.y), quickly ? 1 : 0]); return input[9] !== 0; },
            setFrame: (sid, frame, flags) => { event('frame', [sid, frame >>> 0, flags]); put(raw, 36, frame >>> 0); },
            destroy: (sid, flags) => { event('destroy', [sid, flags]); raw[11] |= 2; },
            unpop: (sid, flags) => {
                event('unpop', [sid, flags]);
                raw[11] |= 4;
                if (input[14]) {
                    put(raw, 14, floatToBits(42.5));
                    put(raw, 18, floatToBits(43.5));
                    put(raw, 36, 3);
                }
            },
            pop: (sid, point, flags) => {
                event('pop', [sid, floatToBits(point.x), floatToBits(point.y), flags]);
                put(raw, 14, floatToBits(point.x));
                put(raw, 18, floatToBits(point.y));
                raw[11] &= ~4 & 0xff;
            },
            assertion: (line) => event('assert', [line])
        };
        const body = new RawDudeRegular(pool, frames, random, state, hooks);
        const signedCount = input[2] | 0;
        let result;
        switch (input[0]) {
        case 0: result = floatToBits(body.handle(50, input[1], input[2], bitsToFloat(input[3]))); break;
        case 1: result = floatToBits(body.wander(50, signedCount, input[1] !== 0)); break;
        case 2: result = floatToBits(body.return(50, input[6] ? 52 : 0, signedCount)); break;
        case 3: result = floatToBits(body.drop(50, input[6] ? 52 : 0, signedCount)); break;
        case 4: result = floatToBits(body.fall(50, signedCount)); break;
        case 5: result = body.moveTo(50, input[8] ? 51 : 0, input[1] !== 0) ? 1 : 0; break;
        default: assert.fail('알 수 없는 몸체');
        }
        const observed = `${result}|${trace.length ? trace.join(';') : '-'}|${hex(raw)}|${random.state}|${state.providerIndex >>> 0}|-`;
        const parts = observed.split('|');
        // 반환 float 비트는 NaN payload 만 같은 부류로 보고 나머지 필드는 문자열 그대로 비교한다.
        assert.ok(sameBits(Number(parts[0]), Number(expected[0])), `Dude 행 ${count} 반환 ${row[0]}`);
        assert.deepEqual(parts.slice(1), expected.slice(1), `Dude 행 ${count} ${row[0]}`);
        count += 1;
    }
    assert.equal(count, 1600);
    assert.equal(initializers, 3);
});

test('DudeRegular: 판본·프레임 표 크기·다른 파생 자산·미연결 하위 효과를 거부한다', () => {
    const pool = new SidPool(24000, { server: false });
    for (let number = 5; number <= 53; number += 1) assert.equal(pool.allocate(2), number);
    const frames = Array.from({ length: 188 }, () => new FrameCodeTable([]));
    const providers = new SquidPostPopList();
    const state = { checking: false, providers, providerIndex: 0, pollEvent: 0x1a2b, pathType: 24 };
    const random = new GameRandom(1);
    assert.ok(throws(() => new RawDudeRegular({ edition: 'cd1072' }, frames, random, state, {})));
    assert.ok(throws(() => new RawDudeRegular(pool, frames.slice(0, 10), random, state, {})));
    const body = new RawDudeRegular(pool, frames, random, state, {});
    assert.equal(body.pool, pool);
    const raw = pool.physical(50);
    raw.fill(0);
    assert.equal(body.handles(50), false);
    assert.ok(throws(() => body.handle(50, 0x5102, 0, 1)));
    raw[10] = 70;
    put(raw, 0, 0x508cc8);
    assert.equal(body.handles(50), true);
    // poll 사건은 훅 없이 0, 그 밖에는 선택된 하위 효과가 필요하다.
    assert.equal(body.handle(50, 0x1a2b, 0, 5), 0);
    assert.ok(throws(() => body.handle(50, 0x5102, 0, 5)));
    assert.equal(body.moveTo(50, 0, false), false);
    assert.ok(throws(() => body.fall(50, 101)));
    assert.equal(body.handle(50, 0x1234, 0, 5), 5);
});
