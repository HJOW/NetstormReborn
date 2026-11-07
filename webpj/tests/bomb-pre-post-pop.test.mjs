// Point Blast 계열 Bomb 의 가상 전처리 raw-bomb-pre-destroy.js(RawBombPreDestroy)와 세 주문 효과 접두 raw-bomb-post-pop.js(RawBombPostPop)를 독립 원본 관측으로 대조한다.
//  - bomb-predestroy-x86.tsv 96행: 상태 비트 9·flags 0x100000 관문과 폭발/소리 뒤 현재 좌표·가상 표·상태 변경(mutation 1~3)이 만드는 효과 문자열과 50바이트 슬롯.
//  - bomb-postpop-x86.tsv 432행: 세 가상 표 × flags × 조회 결과 × 할당 결과 × 난수 시드의 호출 순서(`find|alloc|regular|base`)와 최종 난수 상태. 실제 SquidPostPop 분배(억제된 공통 장부)를 거쳐 공통 깊이가 한 번만 줄어드는 것도 확인한다.
// 입력 슬롯은 원본과 같은 raw 50바이트이며 기대값은 계산하지 않고 원본 문자열과 비교한다. (2026-11-08 WB04-E51: RawBombPreDestroy·RawBombPostPop 이식과 함께 추가)
import assert from 'node:assert/strict';
import path from 'node:path';
import test from 'node:test';
import { GameRandom } from '../www/core/random.js';
import { RawBombPostPop } from '../www/core/raw-bomb-post-pop.js';
import { RawBombPreDestroy } from '../www/core/raw-bomb-pre-destroy.js';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidPostPop } from '../www/core/squid-postpop.js';
import { SquidPostPopState } from '../www/core/squid-postpop-state.js';
import { REPO_ROOT } from './helpers.mjs';
import { floatToBits, get, put, readRows } from './raw-support.mjs';

// 바이트 배열을 소문자 hex 로 만든다. (2026-11-08 WB04-E51)
const hex = (bytes) => Array.from(bytes, (byte) => byte.toString(16).padStart(2, '0')).join('');

// 입력의 signed/unsigned 십진수를 원본 하위 32비트 수로 읽는다. (2026-11-08 WB04-E51)
const unsigned = (text) => Number(BigInt.asUintN(32, BigInt(text)));

// 슬롯 50 까지 확보한 풀과 그 쓰기 가능한 50바이트 뷰를 만든다. (2026-11-08 WB04-E51)
function scene(capacity) {
    const pool = new SidPool(capacity, { server: false });
    // 부모 슬롯 50 까지 순서대로 확보한다.
    for (let value = 5; value <= 50; value += 1) assert.equal(pool.allocate(2), value);
    return { pool, raw: pool.physical(50).subarray(0, 50) };
}

test('bomb-predestroy: 폭발 관문·효과 뒤 현재 좌표·상태 변경과 공통 전처리 호출을 원본 96행과 대조한다', () => {
    const { pool, raw } = scene(24000);
    const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'bomb-predestroy-x86.tsv'));
    let count = 0;
    // 원본 96행을 재생한다.
    for (const row of rows) {
        assert.equal(row.length, 5);
        const [extra, flags, mutation] = row[0].split(',').map(unsigned);
        raw.fill(0xa5);
        put(raw, 0, 0x502370, 4);
        raw[10] = 113;
        raw[11] = 2;
        raw[40] = extra & 0xff;
        put(raw, 14, floatToBits(98.0), 4);
        put(raw, 18, floatToBits(122.0), 4);
        let events = '';
        // 효과는 원본과 같은 입력 변경만 한다.
        const event = (text) => {
            if (events) events += ';';
            events += text;
        };
        const bomb = new RawBombPreDestroy(pool, {
            explosion: (sid) => {
                event(`E:${sid}`);
                if (mutation === 1) {
                    put(raw, 14, 0x42cb0000, 4);
                    put(raw, 18, 0xc0880000, 4);
                }
                if (mutation === 2) {
                    raw[40] = 9;
                    put(raw, 0, 0x501dd0, 4);
                }
            },
            sound: (x, y) => {
                event(`S:${floatToBits(x)}:${floatToBits(y)}:0`);
                if (mutation === 3) {
                    raw[40] = 0;
                    put(raw, 12, 0xbeef, 2);
                }
            },
            base: (sid, value) => event(`B:${sid}:${value}`),
        });
        assert.ok(bomb.handles(50));
        bomb.preDestroy(50, flags);
        assert.equal(events, row[1], `전처리 행 ${count} 효과`);
        assert.equal(hex(raw), row[2], `전처리 행 ${count} 슬롯`);
        assert.equal(row[1], row[3]);
        assert.equal(row[2], row[4]);
        count += 1;
    }
    assert.equal(count, 96);
});

test('bomb-predestroy: 판본 불일치와 다른 가상 표·누락 훅은 도달한 지점에서만 거부한다', () => {
    assert.throws(() => new RawBombPreDestroy({ edition: 'original1037' }, {}), TypeError);
    const { pool, raw } = scene(24000);
    put(raw, 0, 0x501dd0, 4);
    const bomb = new RawBombPreDestroy(pool, { base() {} });
    assert.equal(bomb.handles(50), false);
    assert.throws(() => bomb.preDestroy(50, 0), TypeError);
    put(raw, 0, 0x502370, 4);
    raw[40] = 0;
    // 폭발 삭제 플래그가 없으면 폭발/소리 훅 없이 공통 전처리만 부른다.
    bomb.preDestroy(50, 0);
    // 플래그가 있으면 폭발 훅이 필요하다.
    assert.throws(() => bomb.preDestroy(50, 0x100000), TypeError);
    // 공통 전처리 훅은 항상 필요하다.
    assert.throws(() => new RawBombPreDestroy(pool, {}).preDestroy(50, 0), TypeError);
});

test('bomb-postpop: 세 가상 표의 최초 예약 접두와 공통 깊이를 실제 분배로 원본 432행과 대조한다', () => {
    const { pool, raw } = scene(24000);
    const types = Array.from({ length: 188 }, () => ({ flags1: 0, flags2: 0, footX: 1, footY: 1, cost: 1.25, maxHitPoints: 91 }));
    const state = new SquidPostPopState();
    // 억제된 공통 장부는 깊이만 줄이며 다른 효과를 합성하지 않는다.
    state.suppressed = true;
    const base = new SquidPostPop(pool, types, state);
    const random = new GameRandom();
    const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'bomb-postpop-x86.tsv'));
    let count = 0;
    // 원본 432행을 재생한다.
    for (const row of rows) {
        assert.equal(row.length, 7);
        const kind = unsigned(row[0]);
        const flags = unsigned(row[1]);
        const found = unsigned(row[2]) !== 0;
        const allocated = unsigned(row[3]) !== 0;
        random.setState(unsigned(row[4]));
        const events = [];
        const post = new RawBombPostPop(pool, random, {
            hasRegular: (parent, event) => {
                events.push(`find:${parent}:${event}`);
                return found;
            },
            reserveRegular: () => {
                events.push('alloc:40');
                return allocated;
            },
            addRegular: (parent, event, delay) => events.push(`regular:${event}:${parent}:${floatToBits(delay)}`),
        });
        base.setBody('bomb', post);
        put(raw, 0, [0x502c38, 0x502cf0, 0x502db0][kind], 4);
        raw[10] = 0;
        raw[11] = 9;
        state.depth = 1;
        const saved = Uint8Array.from(raw);
        assert.ok(base.handlesDerived(50));
        base.postPopVirtual(50, flags);
        assert.equal(state.depth, 0);
        assert.deepEqual(Uint8Array.from(raw), saved);
        events.push(`base:50:${flags}`);
        assert.equal(events.join('|'), row[5], `후처리 행 ${count} 호출열`);
        assert.equal(random.state, unsigned(row[6]), `후처리 행 ${count} 난수`);
        base.setBody('bomb', null);
        count += 1;
    }
    assert.equal(count, 432);
});

test('bomb-postpop: 선택 장치는 도달한 지점에서만 요구하고 이미 고른 몸체는 조회 중 표가 바뀌어도 유지한다', () => {
    const { pool, raw } = scene(24000);
    put(raw, 0, 0x502c38, 4);
    const random = new GameRandom(55);
    const empty = new RawBombPostPop(pool, random, {});
    empty.prefix(50, 0);
    assert.equal(random.state, 55);
    assert.throws(() => empty.prefix(50, 1), TypeError);
    const hooks = { hasRegular: () => false, reserveRegular: () => false };
    // 할당 실패에서 난수/추가 장치는 요구하지 않는다.
    new RawBombPostPop(pool, random, hooks).prefix(50, 1);
    assert.equal(random.state, 55);
    put(raw, 0, 0x502cf0, 4);
    // 뒤 장치가 없어도 원본 앞선 난수 소비는 남는다.
    assert.throws(() => new RawBombPostPop(pool, random, { ...hooks, reserveRegular: () => true }).prefix(50, 1), TypeError);
    assert.notEqual(random.state, 55);
    let payload = 0;
    put(raw, 0, 0x502c38, 4);
    random.setState(55);
    new RawBombPostPop(pool, random, {
        hasRegular: () => {
            put(raw, 0, 0x502db0, 4);
            return false;
        },
        reserveRegular: () => true,
        addRegular: (parent, event, delay) => { payload = floatToBits(delay); },
    }).prefix(50, 1);
    // 이미 고른 그라비테이션은 조회 중 표가 미티어로 바뀌어도 난수를 쓰지 않는다.
    assert.equal(payload, 0x3d4ccccd);
    assert.equal(random.state, 55);
    assert.equal(get(raw, 0, 4), 0x502db0);
    // 지원하지 않는 표와 다른 판본은 거부한다.
    put(raw, 0, 0x501dd0, 4);
    assert.throws(() => new RawBombPostPop(pool, random, hooks).prefix(50, 1), Error);
    assert.equal(new RawBombPostPop(pool, random, hooks).handles(50), false);
    assert.throws(() => new RawBombPostPop({ edition: 'original1037' }, random, hooks), TypeError);
    assert.equal(new RawBombPostPop(pool, random, hooks).pool, pool);
});
