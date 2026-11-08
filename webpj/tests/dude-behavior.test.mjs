// Dude 부모 행동 raw-dude-behavior.js(RawDudeBehavior)를 원본 10.78 관측 410행과 cpppj 계약으로 대조한다.
//  - 원본 관측: dude-behavior-x86.tsv — 몸체(0 Halt / 1 CheckSpace / 2 Fall / 3 Arrival / 4 PostDestroy)·상태 바이트·task WORD·지도 비트·진단 모드·population 패턴(raw+40 비트 포함)으로
//    반환값, 효과 순서(task/type/lookup/destroy/post/assert), population count 와 count 뒤 stale 꼬리까지 포함한 5개 DWORD 전체가 같다. 슬롯 raw 는 변하지 않는다. 두 x87 정밀도 열이 동일하다.
//  - 계약: 판본·지도 크기·다른 파생 자산 거부, 미연결 하위 효과 거부.
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E122: Dude 부모 행동 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidPostPopList } from '../www/core/squid-postpop-state.js';
import { RawDudeBehavior } from '../www/core/raw-dude-behavior.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E122)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'dude-behavior-x86.tsv')) ?? [];

// 예외가 나는지 본다. (2026-11-08 WB04-E122)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 실제 원본에 준 목록의 마지막 99 도 stale 보존 대상인 population 패턴이다. (2026-11-08 WB04-E122)
const PATTERNS = [[50, 51, 50, 52, 99], [51, 52, 53, 54, 99], [50, 50, 50, 50, 99], [51, 50, 52, 50, 99]];

test('DudeBehavior: 원본 10.78 410입력의 부모 반환·효과 순서·population 전체가 같다', { skip: rows.length === 0 && '원본 fixture 없음' }, () => {
    const pool = new SidPool(24000, { server: true });
    const raw = pool.physical(50);
    const spots = new Uint8Array(65536);
    let count = 0;
    // 원본 410입력과 두 정밀도의 동일 결과를 빠짐없이 검사한다.
    for (const row of rows) {
        assert.equal(row.length, 3);
        const input = row[0].split(',').map(Number);
        assert.equal(input.length, 6);
        raw.fill(0);
        put(raw, 0, 0x508cc8);
        raw[10] = 70;
        raw[11] = input[1] & 0xff;
        put(raw, 12, input[2], 2);
        put(raw, 14, floatToBits(20.75));
        put(raw, 18, floatToBits(21.25));
        raw[40] = (input[5] >> 8) & 0xff;
        spots.fill((input[3] ^ 6) & 0xff);
        spots[21 * 256 + 20] = input[3] & 0xff;
        const checking = input[4] !== 0;
        const population = new SquidPostPopList();
        population.entries = PATTERNS[input[5] & 3].slice();
        population.count = 4;
        const trace = [];
        const hooks = {
            task: (sid, event, delay) => trace.push(`task:${sid}:${event}:${floatToBits(delay)}`),
            movementType: (type) => { trace.push(`type:${type}`); return 24; },
            find: (sid, type, index, remove) => { trace.push(`lookup:${sid}:${type}:${index}:${remove ? 1 : 0}`); return 0; },
            destroy: (sid, flags) => trace.push(`destroy:${sid}:${flags}`),
            basePostDestroy: (sid, flags) => trace.push(`post:${sid}:${flags}`),
            assertion: (line) => trace.push(`assert:${line}`)
        };
        const body = new RawDudeBehavior(pool, spots, { checking, population }, hooks);
        const before = raw.slice();
        let result;
        switch (input[0]) {
        case 0: result = String(body.halt(50)); break;
        case 1: result = body.checkSpace(50) ? '1' : '0'; break;
        case 2: result = body.fall(50) ? '1' : '0'; break;
        case 3: result = String(body.arrival(50)); break;
        case 4: body.postDestroy(50, 0x12345678); result = '-'; break;
        default: assert.fail('알 수 없는 몸체');
        }
        // count 밖의 물리 꼬리를 포함해 독립 원본의 전체 5 DWORD 를 대조한다.
        result += `|${trace.length ? trace.join(';') : '-'}|${population.count}|${population.entries.join(',')}`;
        assert.equal(result, row[1], `Dude 행동 행 ${count} ${row[0]}`);
        assert.equal(row[1], row[2]);
        assert.deepEqual(raw, before);
        count += 1;
    }
    assert.equal(count, 410);
});

test('DudeBehavior: 판본·지도 크기·다른 파생 자산·미연결 하위 효과를 거부한다', () => {
    const pool = new SidPool(24000, { server: true });
    const raw = pool.physical(50);
    const spots = new Uint8Array(65536);
    const population = new SquidPostPopList();
    const state = { checking: false, population };
    assert.ok(throws(() => new RawDudeBehavior({ edition: 'cd1072' }, spots, state, {})));
    assert.ok(throws(() => new RawDudeBehavior(pool, new Uint8Array(10), state, {})));
    assert.ok(throws(() => new RawDudeBehavior(pool, spots, {}, {})));
    const body = new RawDudeBehavior(pool, spots, state, {});
    assert.equal(body.pool, pool);
    assert.equal(body.population, population);
    // 타입 70 이 아니거나 가상 표가 다른 자산은 분배하지 않고 getter 도 거부한다.
    raw.fill(0);
    assert.equal(body.handles(50), false);
    assert.ok(throws(() => body.halt(50)));
    raw[10] = 70;
    put(raw, 0, 0x508cc8);
    assert.equal(body.handles(50), true);
    // dead 면 지면 판정 없이 12, 살아 있으면 지도 비트를 읽어야 하며 task 훅이 없으면 거부한다.
    raw[11] = 2;
    assert.equal(body.halt(50), 12);
    raw[11] = 0;
    put(raw, 14, floatToBits(10.5));
    put(raw, 18, floatToBits(10.5));
    assert.ok(throws(() => body.checkSpace(50)));
    spots.fill(4);
    assert.equal(body.checkSpace(50), false);
    // 진단 모드는 호출 시점에 읽는 함수여도 된다.
    put(raw, 12, 0x5102, 2);
    assert.equal(new RawDudeBehavior(pool, spots, { checking: () => 1, population }, {}).arrival(50), 1000);
    put(raw, 12, 0x5101, 2);
    assert.ok(throws(() => new RawDudeBehavior(pool, spots, { checking: () => 1, population }, {}).arrival(50)));
    assert.ok(throws(() => body.postDestroy(50, 0)));
});
