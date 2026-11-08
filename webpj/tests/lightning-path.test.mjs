// 빔 경로 raw-lightning-path.js(RawLightningPath)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: lightning-path-x86.tsv 216행 — 종류 0(생성자 72행: source/target 의 다른 높이 보정·배율·퇴화 점·미사용 바이트)과 종류 1(생성+Seed+Step 144행: 재귀 가지·난수 두 번 소비·완료 뒤 재호출)에서
//    모든 중간 64바이트 경로 이미지·입자 꼬리·실제 난수 상태와 120입자 전체(24바이트 × 120)가 같다.
//  - 계약: 소유자 불일치/용량 초과/배율 오류/미복원 판본 거부, 직접 좌표 생성자.
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E117: 빔 경로 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get, bitsToFloat, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { GameRandom } from '../www/core/random.js';
import { LightningParticles, LightningPathFields, RawLightningPath } from '../www/core/raw-lightning-path.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E117)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'lightning-path-x86.tsv')) ?? [];

// 예외가 나는지 본다. (2026-11-08 WB04-E117)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 16진 문자열을 바이트로 복원한다. (2026-11-08 WB04-E117)
function decode(text) {
    return Uint8Array.from(Buffer.from(text, 'hex'));
}

// 같은 시점의 전체 경로 64바이트·현재 total·실제 난수 상태를 붙인다. (2026-11-08 WB04-E117)
function snapshot(trace, fields, particles, random) {
    const tail = new Uint8Array(8);
    put(tail, 0, particles.total);
    put(tail, 4, random.state);
    trace.push(...fields.bytes, ...tail);
}

// 용량 전체의 실제 24바이트 의미 필드를 원본 순서로 직렬화한다. (2026-11-08 WB04-E117)
function serialize(particles) {
    const bytes = new Uint8Array(particles.values.length * 24);
    // 미사용 꼬리/NULL 이전 포인터도 비교 범위에 포함한다.
    particles.values.forEach((particle, index) => {
        const at = index * 24;
        put(bytes, at, floatToBits(particle.x));
        put(bytes, at + 4, floatToBits(particle.y));
        put(bytes, at + 8, floatToBits(particle.strength));
        put(bytes, at + 12, floatToBits(particle.jitterWeight));
        put(bytes, at + 16, floatToBits(particle.life));
        put(bytes, at + 20, particle.previous);
    });
    return bytes;
}

// 발자국 중심을 타입별로 계산하는 원문 중심 조회이다(반더/키 큰 유닛 3칸, 맨 오브 워 2칸, 그 밖 1칸). (2026-11-08 WB04-E117)
function makeCenter(pool) {
    return (sid) => {
        const raw = pool.slot(sid);
        const foot = raw[10] === 148 || raw[10] === 149 ? 3 : raw[10] === 144 ? 2 : 1;
        return [Math.fround(bitsToFloat(get(raw, 14)) - foot * 0.5), Math.fround(bitsToFloat(get(raw, 18)) - foot * 0.5)];
    };
}

// 현재 종류(0 생성자 / 1 생성+Seed+Step)의 행을 모두 재생한다. (2026-11-08 WB04-E117)
function replay(kind) {
    const pool = new SidPool(24000, { server: false });
    // 원문이 읽는 두 SID 50/51 을 포함하는 실제 저장소를 준비한다.
    for (let sid = 5; sid <= 51; sid += 1) assert.equal(pool.allocate(2), sid);
    let count = 0;
    // 현재 종류의 기본 53비트 원문을 사용하며 기대값은 재생성하지 않는다.
    rows.forEach((row, index) => {
        assert.equal(row.length, 13);
        if (Number(row[0]) !== kind) return;
        const state = { scaleX: bitsToFloat(Number(row[7])), scaleY: bitsToFloat(Number(row[8])), vanderType: 148, tallType: 149, manOWarType: 144 };
        const random = new GameRandom(Number(row[9]));
        // 실제 두 raw 슬롯의 타입/좌표만 구성한다.
        for (const [sid, type, x, y] of [[50, Number(row[1]), Number(row[3]), Number(row[4])], [51, Number(row[2]), Number(row[5]), Number(row[6])]]) {
            const raw = pool.physical(sid);
            raw.fill(0);
            raw[10] = type;
            put(raw, 14, x);
            put(raw, 18, y);
        }
        const logic = new RawLightningPath(pool, random, state, makeCenter(pool));
        const fields = new LightningPathFields();
        fields.bytes.fill(0xa5);
        const particles = new LightningParticles(120, 1);
        const trace = [];
        logic.initialize(fields, 50, 51, 1);
        snapshot(trace, fields, particles, random);
        if (kind) {
            logic.seed(fields, particles);
            snapshot(trace, fields, particles, random);
            // 원본의 진행/후퇴/완료 후 호출 순서를 보존하도록 각 입력 쌍을 그대로 전달한다.
            for (const step of row[10].split(',')) {
                const pair = step.split(':');
                assert.equal(pair.length, 2);
                logic.step(fields, particles, bitsToFloat(Number(pair[0])), bitsToFloat(Number(pair[1])));
                snapshot(trace, fields, particles, random);
            }
        }
        assert.deepEqual(Buffer.from(trace), Buffer.from(decode(row[11])), `빔 경로 행 ${index} 중간 상태`);
        assert.deepEqual(Buffer.from(serialize(particles)), Buffer.from(decode(row[12])), `빔 경로 행 ${index} 120입자`);
        count += 1;
    });
    return count;
}

test('LightningPath: 원본 10.78 생성자의 높이 보정·배율·퇴화 점·미사용 바이트가 같다', { skip: rows.length === 0 && '원본 fixture 없음' }, () => {
    assert.equal(replay(0), 72);
});

test('LightningPath: 원본 10.78 재귀 가지·난수 두 번 소비·완료 후 재호출·120입자 전체가 같다', { skip: rows.length === 0 && '원본 fixture 없음' }, () => {
    assert.equal(replay(1), 144);
});

test('LightningPath: 소유자/용량/배율/판본 오류를 거부하고 직접 좌표 생성자를 확인한다', () => {
    const pool = new SidPool(24000, { server: false });
    for (let sid = 5; sid <= 51; sid += 1) assert.equal(pool.allocate(2), sid);
    const state = { scaleX: 16, scaleY: 11, vanderType: 148, tallType: 149, manOWarType: 144 };
    const center = () => [10, 10];
    assert.ok(throws(() => new RawLightningPath({ edition: 'cd1072' }, new GameRandom(1), state, center)));
    assert.ok(throws(() => new RawLightningPath(pool, new GameRandom(1), { ...state, scaleX: 0 }, center)));
    assert.ok(throws(() => new RawLightningPath(pool, new GameRandom(1), { ...state, scaleY: Number.NaN }, center)));
    assert.ok(throws(() => new RawLightningPath(pool, new GameRandom(1), state, null)));
    const logic = new RawLightningPath(pool, new GameRandom(7), state, center);
    const fields = new LightningPathFields();
    // 소유자 0 / 범위 밖 SID 는 상태를 바꾸기 전에 거부한다.
    assert.ok(throws(() => logic.initialize(fields, 50, 51, 0)));
    assert.ok(throws(() => logic.initialize(fields, 24000, 51, 1)));
    assert.ok(throws(() => logic.initializePoints(fields, [0, 0], [1, 1], 0)));
    // 직접 좌표 생성자는 타입/중심/배율 보정을 읽지 않고 끝점/거리/각을 쓴다.
    fields.bytes.fill(0xa5);
    logic.initializePoints(fields, [10, 20], [10, 60], 1);
    assert.equal(get(fields.bytes, 0), floatToBits(10));
    assert.equal(get(fields.bytes, 12), floatToBits(60));
    assert.equal(get(fields.bytes, 24), floatToBits(40));
    assert.equal(get(fields.bytes, 32), floatToBits(2));
    assert.equal(get(fields.bytes, 60), 1);
    assert.equal(fields.bytes[40], 0xa5);
    // 소유자가 다른 입자 저장소나 용량 초과는 거부한다.
    assert.ok(throws(() => logic.seed(fields, new LightningParticles(120, 2))));
    assert.ok(throws(() => logic.seed(fields, new LightningParticles(0, 1))));
    const particles = new LightningParticles(2, 1);
    logic.seed(fields, particles);
    assert.equal(particles.total, 1);
    assert.equal(particles.values[0].strength, 1);
    // 용량 2 에서 진행하면 입자가 모자라 원본 assert 로 거부한다.
    assert.ok(throws(() => logic.step(fields, particles, 1, 0)));
    assert.ok(throws(() => logic.step(fields, new LightningParticles(120, 1), Number.NaN, 0)));
});
