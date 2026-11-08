// 플레어 raw-flare.js(RawFlare)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: flare-x86.tsv 139행(Spawn 30 + Regular 109 — 억제/로딩/Kernel 개수/부모 genus·좌표·깊이/SHP 핫스팟/난수 시드/예약 성공 비트, 또는 프레임 단계·Kernel·좌표·시드·예약·payload)
//    → 반환 float 비트, 게임 난수 상태, 부모(50)와 열 개 후보(100..109) 슬롯 전체 raw, 호출 순서(K Kernel 개수/C 생성/H 핫스팟/O 소유자/P 배치/R 예약/A Regular 부착/F 프레임/U 갱신/D 삭제/S 소리).
//    bomb-status-flare-x86.tsv 60행 — 같은 Spawn 입력 30개를 C/D 방향으로 각각 실행한 난수·열한 슬롯 raw·호출 순서.
//  - 계약: 가상 표가 다른 부모의 Regular 는 기존 처리기로 전달하고, 처리기가 없으면 거부하며, 판본/표 크기를 거부한다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E110: 플레어 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, bitsToFloat, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { GameRandom } from '../www/core/random.js';
import { FrameCodeTable } from '../www/assets/terrain-tiles.js';
import { RawFlare } from '../www/core/raw-flare.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E110)
const fixtures = path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures');
const flareRows = readRows(path.join(fixtures, 'flare-x86.tsv'));
const bombRows = readRows(path.join(fixtures, 'bomb-status-flare-x86.tsv'));
const hex = (bytes) => Buffer.from(bytes).toString('hex');
const bits = floatToBits;

// 예외가 나는지 본다. (2026-11-08 WB04-E110)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 원본과 같은 112개 프레임 코드/16구간의 명시 입력이다(각 글자에 7개의 같은 변형 프레임). (2026-11-08 WB04-E110)
const flareFrames = () => new FrameCodeTable(Array.from({ length: 112 }, (_, index) => ({ side: 0x41 + Math.floor(index / 7), variant: 0x41, flags: 0 })));

// 음수와 64비트 double 억제 입력을 원본 비트로 읽는다(Spawn 의 세 번째 값은 두 DWORD 로 나눈다). (2026-11-08 WB04-E110)
function flareInputs(text, spawn) {
    const result = [];
    // Spawn 의 double 은 두 DWORD 로 나누어 테스트 입력에만 보관한다.
    text.split(',').forEach((word, index) => {
        const value = BigInt.asUintN(64, BigInt(word));
        result.push(Number(BigInt.asUintN(32, value)));
        if (spawn && index === 2) result.push(Number(BigInt.asUintN(32, value >> 32n)));
    });
    return result;
}

// 원본의 독립 하위 입력을 공급하고 새 JS 의 효과 인자만 기록하는 장면이다. (2026-11-08 WB04-E110)
function makeScene(input, spawn) {
    const scene = {
        pool: new SidPool(24000),
        types: Array.from({ length: 188 }, () => ({ flags2: 0 })),
        frames: Array.from({ length: 188 }, flareFrames),
        random: new GameRandom(),
        state: { suppressedTime: 0, loadingDepth: 0, type: 106 },
        kernel: 0,
        hotspot: [0, 0],
        allocations: 0,
        reserved: 0,
        created: 0,
        trace: []
    };
    // 본체가 직접 쓰는 대상은 실제 할당한 슬롯이다.
    for (let sid = 5; sid < 110; sid += 1) assert.equal(scene.pool.allocate(2), sid);
    // 원본 관측의 부모와 10개 생성 후보에 같은 보존 패턴을 넣는다.
    for (let sid = 50; sid < 110; sid += 1) {
        if (sid !== 50 && sid < 100) continue;
        const raw = scene.pool.physical(sid);
        raw.fill(0xa5);
        put(raw, 0, 0x5096a0);
        raw[10] = 106;
        raw[11] = 0;
        raw[34] = 8;
        raw[35] = 99;
        raw[40] = 32;
        put(raw, 36, 77);
    }
    const parent = scene.pool.physical(50);
    if (spawn) {
        const view = new DataView(new ArrayBuffer(8));
        view.setUint32(0, input[2], true);
        view.setUint32(4, input[3], true);
        scene.state.suppressedTime = view.getFloat64(0, true);
        scene.state.loadingDepth = input[4];
        scene.kernel = input[5] | 0;
        scene.types[85].flags2 = input[6];
        parent[10] = 85;
        put(parent, 14, input[7]);
        put(parent, 18, input[8]);
        parent[35] = input[9] & 0xff;
        put(parent, 36, 0);
        scene.hotspot = [(input[10] << 16) >> 16, (input[11] << 16) >> 16];
        scene.random.setState(input[12]);
        scene.allocations = input[13];
    } else {
        scene.kernel = input[3] | 0;
        put(parent, 14, input[4]);
        put(parent, 18, input[5]);
        put(parent, 36, input[0] * 7);
        scene.random.setState(input[6]);
        scene.allocations = input[7];
    }
    // 한 효과의 인자는 DWORD 비트를 부호 없이 출력한다.
    const add = (label, ...values) => scene.trace.push(values.reduce((text, value) => `${text},${value >>> 0}`, label));
    scene.hooks = {
        processCount: () => { scene.trace.push(`K,${scene.kernel}`); return scene.kernel; },
        create: (kind, flags) => { scene.created += 1; const sid = 99 + scene.created; add('C', kind, flags, sid); return sid; },
        sourceHotspot: (parentSid) => { assert.equal(parentSid, 50); scene.trace.push('H'); return scene.hotspot; },
        owner: (sid, owner) => { add('O', sid, owner); scene.pool.physical(sid)[34] = owner; },
        pop: (sid, x, y, flags) => { add('P', sid, bits(x), bits(y), flags); put(scene.pool.physical(sid), 14, bits(x)); put(scene.pool.physical(sid), 18, bits(y)); },
        reserveRegular: () => { const success = scene.reserved < 32 ? (scene.allocations >>> scene.reserved) & 1 : 0; scene.reserved += 1; add('R', success); return success !== 0; },
        addRegular: (sid, event, payload) => add('A', sid, event, bits(payload)),
        frame: (sid, frame, flags) => { add('F', sid, frame, flags); put(scene.pool.physical(sid), 36, frame >>> 0); },
        update: (sid) => add('U', sid),
        destroy: (sid, flags) => add('D', sid, flags),
        sound: (x, y, name, flags) => { assert.equal(name, 'powerup.wav'); add('S', bits(x), bits(y), flags); }
    };
    scene.make = () => new RawFlare(scene.pool, scene.types, scene.frames, scene.random, scene.state, scene.hooks);
    // 부모(50)와 열 개 후보(100..109) 슬롯 전체의 원문 출력이다.
    scene.raws = (sids) => sids.map((sid) => hex(scene.pool.slot(sid))).join('');
    return scene;
}

test('Flare: 원본 10.78 139입력(Spawn 30+Regular 109)의 전체 슬롯·난수 상태·호출 순서·반환 float 이 같다', { skip: !flareRows && '원본 fixture 없음' }, () => {
    let spawnCount = 0;
    let regularCount = 0;
    // 기대값은 원본 TSV 에만 있으며 새 몸체로 생성하지 않는다.
    for (const row of flareRows) {
        const spawn = row[0] === 'Spawn';
        const input = flareInputs(row[1], spawn);
        const scene = makeScene(input, spawn);
        const flare = scene.make();
        let result = 0;
        if (spawn) { flare.spawn(50, input[0] | 0, input[1]); spawnCount += 1; }
        else { result = bits(flare.handle(50, input[1], input[2], bitsToFloat(input[8]))); regularCount += 1; }
        const sids = [50, ...Array.from({ length: 10 }, (_, index) => 100 + index)];
        const label = `플레어 ${row[0]} ${row[1]}`;
        assert.equal(result, Number(row[2]), label);
        assert.equal(String(scene.random.state), row[3], label);
        assert.equal(scene.raws(sids), row[4], label);
        assert.equal(scene.trace.length === 0 ? '-' : scene.trace.join(';'), row[5], label);
    }
    assert.equal(spawnCount, 30);
    assert.equal(regularCount, 109);
});

test('Flare: C/D 방향 보호 원문 60입력의 난수·열한 슬롯 raw·예약 인자 호출 순서가 같다', { skip: !bombRows && '원본 fixture 없음' }, () => {
    let count = 0;
    // 원본 입력 30개를 두 방향에서 각각 실행한다.
    for (const row of bombRows) {
        const input = flareInputs(row[1], true);
        const scene = makeScene(input, true);
        scene.make().spawnDirection(50, input[0] | 0, input[1], 106, Number(row[0]));
        // 방문하지 않은 주변 슬롯도 모두 비교한다.
        const label = `방향 ${row[0]}: ${row[1]}`;
        assert.equal(Number(row[2]), 0);
        assert.equal(String(scene.random.state), row[3], label);
        assert.equal(scene.raws([50, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109]), row[4], label);
        assert.equal(scene.trace.length === 0 ? '-' : scene.trace.join(';'), row[5], label);
        count += 1;
    }
    assert.equal(count, 60);
});

test('Flare: 가상 표가 다른 부모의 Regular 는 기존 처리기로 전달하고 처리기가 없으면 거부하며 판본/표 크기를 거부한다', () => {
    const scene = makeScene([0, 0, 0, 0, 0, 0, 0, 0, 0], false);
    const flare = scene.make();
    assert.ok(flare.handles(50));
    put(scene.pool.physical(50), 0, 0x502370);
    assert.ok(!flare.handles(50));
    const passed = [];
    const handler = flare.handler((sid, event, count, payload) => { passed.push([sid, event, count, payload]); return 2.5; });
    assert.equal(handler(50, 3, 4, 1.5), 2.5);
    assert.deepEqual(passed, [[50, 3, 4, 1.5]]);
    assert.ok(throws(() => flare.handler()(50, 3, 4, 1.5)));
    assert.ok(throws(() => flare.handle(50, 3, 4, 1.5)));
    assert.ok(throws(() => new RawFlare({ edition: 'cd1072' }, scene.types, scene.frames, scene.random, scene.state, {})));
    assert.ok(throws(() => new RawFlare(scene.pool, scene.types.slice(0, 171), scene.frames, scene.random, scene.state, {})));
    assert.ok(throws(() => new RawFlare(scene.pool, scene.types, scene.frames.slice(0, 10), scene.random, scene.state, {})));
    assert.equal(flare.pool, scene.pool);
    // 억제/로딩이면 어떤 훅도 요구하지 않고 끝난다.
    scene.state.suppressedTime = 1;
    scene.make().spawn(50, 3, 9);
    scene.state.suppressedTime = 0;
    scene.state.loadingDepth = 1;
    scene.make().spawn(50, 3, 9);
    assert.ok(throws(() => scene.make().spawnDirection(50, 1, 9, 106, 16)) === false);
});
