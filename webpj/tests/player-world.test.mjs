// Player 월드 연결 raw-player-world.js(RawPlayerWorld: 생성·파생 postPop·Technology getter)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: player-world-x86.tsv 의 10.78(originals) 840행(Create 192·PostPop 576·두 getter 72). cpppj PlayerWorld_PatchNativeAndActualHost 와 같은 입력으로
//    실제 Factory(보호 Player152 생성자)·Pop·Owner·한 칸 검색(RawBridgeConnect)·ProcessForm/Kernel 호스트를 연결하고 실행 뒤 전체 9기록·슬롯·해시·spot·장부·Regular·Form·Kernel 을 비교한다.
//  - 계약: cpppj PlayerWorldTests 의 PlayerParentCurrent_* (물리 VT·현재 getter·예약 실패/누락·special 재조회·owner DWORD 비교·늦은 공통 실패·생성 순서).
//  - 범위 밖: PlayerWorld_SameSurfaceCoreRecordsAndInitialRegular 는 RawSurfaceWorld(WB05 월드 조립) 몫이고, CD 행/계약은 웹 범위(10.78) 밖이다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E55: Player 월드 연결 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get, adler32 } from './raw-support.mjs';
import { realPlayerAssets, hex, wordAdler, concatWordAdler, doubleBits, bitsDouble } from './player-scene-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash, SQUID_HASH_LEVELS } from '../www/core/squid-hash.js';
import { SquidUnpop } from '../www/core/squid-unpop.js';
import { SquidPostPopState } from '../www/core/squid-postpop-state.js';
import { SquidPostPop } from '../www/core/squid-postpop.js';
import { SquidPop } from '../www/core/squid-pop.js';
import { RAW_POP_RESULT } from '../www/core/raw-squid-pop-parent.js';
import { SquidFactory } from '../www/core/squid-factory.js';
import { SquidOwner, SquidOwnerMode } from '../www/core/squid-owner.js';
import { RawSquidNeighbors } from '../www/core/raw-squid-neighbors.js';
import { BridgeConnectState, RawBridgeConnect } from '../www/core/raw-bridge-connect.js';
import { RawSquidDestroy } from '../www/core/squid-destroy.js';
import { Kernel, PROCESS_CAPACITY } from '../www/core/kernel.js';
import { SquidProcessHost, SquidProcessState } from '../www/core/squid-process.js';
import { typeConstructorAddress } from '../www/core/squid-constructors.js';
import { PLAYER_RECORD_STRIDE, PLAYER_TECHNOLOGY_OFFSET, PLAYER_VTABLE, PLAYER_WORLD_ASSERTION, PlayerWorldState, RawPlayerWorld } from '../www/core/raw-player-world.js';

// 원본 관측 10.78 행이다. 파일이 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E55)
const FIXTURES = path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures');
const worldRows = readRows(path.join(FIXTURES, 'player-world-x86.tsv'))?.filter((row) => row[0] === 'originals') ?? null;

// Player 기록 하나의 Technology DWORD 위치이다. (2026-11-08 WB04-E55)
const technologyAt = (owner) => owner * PLAYER_RECORD_STRIDE + PLAYER_TECHNOLOGY_OFFSET;

// 예외가 나는지 본다. (2026-11-08 WB04-E55)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 선택 범위 밖 효과를 실제 완료로 대체하지 않는 거부 장치이다. (2026-11-08 WB04-E55)
const missing = () => { throw new Error('Player 선택 범위 밖의 실제 후속 효과 미복원'); };

// 원본 초기 9기록의 포인터 비트·Technology DWORD 입력이다(호스트 포인터로 역참조하지 않는다). (2026-11-08 WB04-E55)
function playerRecords(records) {
    // 각 기록의 빈 목록 포인터와 서로 다른 Technology 저장 비트를 채운다.
    for (let owner = 0; owner < 9; owner += 1) {
        put(records, owner * PLAYER_RECORD_STRIDE, (0x12003000 + owner * 0x100) >>> 0);
        put(records, technologyAt(owner), (0xfedc0000 + owner) >>> 0);
    }
}

// 행 하나의 실제 장면이다(cpppj PlayerReplayScene). 입력 10칸만 쓰고 기대 출력은 실행 뒤에만 비교한다. (2026-11-08 WB04-E55)
function replayWorldRow(row) {
    const assets = realPlayerAssets();
    const { types } = assets;
    const input = row[1].split(',');
    assert.equal(input.length, 10);
    const number = (index) => Number(input[index]) >>> 0;
    const pool = new SidPool(32768, { server: true });
    const hash = new SquidHash();
    const spots = new Uint8Array(65536);
    const book = new SquidPostPopState();
    const unpop = new SquidUnpop(pool, hash, spots);
    const post = new SquidPostPop(pool, types, book);
    const pop = new SquidPop(pool, hash, spots, { postPop: post });
    pop.bindDiagnostics({ types });
    const factory = new SquidFactory(pool, types, { unpop });
    const mode = new SquidOwnerMode();
    const owner = new SquidOwner(pool, types, book, mode);
    const neighbors = new RawSquidNeighbors(pool, hash, spots, types, assets.frames);
    const finder = new RawBridgeConnect(pool, neighbors, new BridgeConnectState(), { create: missing, setOwner: missing, pop: missing, setFrame: missing, notifySurface: missing });
    const destroy = new RawSquidDestroy(pool, unpop, types);
    const kernel = new Kernel();
    const clock = new SquidProcessState();
    const host = new SquidProcessHost(pool, types, kernel, destroy, clock, { regular: missing });
    const records = new Uint8Array(9 * PLAYER_RECORD_STRIDE);
    const state = new PlayerWorldState();
    state.playerKnown = number(4);
    state.battleOwner = number(2);
    state.templateOwner = number(3);
    state.fortMode = number(5) !== 0;
    state.special = number(6) !== 0 ? 1 : 0;
    mode.fort = state.fortMode;
    mode.battle = !state.fortMode;
    assert.ok(types[152].footX === 1 && types[152].footY === 1 && types[152].maxHitPoints === 0 && assets.view(152).frameCount === 2);
    book.localOwner = state.battleOwner & 0xff;
    clock.now = bitsDouble(input[9]);
    playerRecords(records);
    // 현재 세 표 전체를 독립 입력으로 채운다(원본 seed 0/표별 17 오프셋).
    [book.localCounts(), book.localSecondaryCounts(), book.globalCounts()].forEach((table, which) => {
        for (let index = 0; index < table.length; index += 1) table[index] = (Math.imul(index, 2654435761) + which * 17) >>> 0;
    });
    const events = [];
    let regular = null;
    const hooks = {
        assertion: (kind) => {
            const line = kind === PLAYER_WORLD_ASSERTION.playerAlreadyExists ? 0x354 : number(0) === 3 ? 0x29d : 0x351;
            events.push(`A:${line}`);
        },
        find: (x, y, type) => finder.findTypeAt(x, y, type),
        create: (type, flags) => factory.create(type, flags),
        setOwner: (sid, value) => owner.set(sid, value),
        pop: (sid, x, y, flags) => {
            const [width, height] = assets.frameSize(pool.slot(sid)[10], 0);
            if (pop.pop(sid, types[152], width, height, x, y, flags) !== RAW_POP_RESULT.registered) throw new Error('Player 실제 첫 공간 Pop 실패');
        },
        reserveRegular: () => { events.push('N:40'); return number(7) !== 0; },
        addRegular: (sid, event, payload) => { regular = host.addRegular(sid, event, payload); events.push(`B:46:${sid}:0:80`); },
    };
    const player = new RawPlayerWorld(pool, post, records, state, hooks);
    post.setBody('player', player);
    // 네 몸체 중 선택된 하나를 실행한다.
    const kind = number(0);
    let sid = 5;
    let result = 0;
    if (kind === 0) {
        sid = player.create(number(1) | 0);
        result = sid;
    } else if (kind === 1) {
        sid = factory.create(152, 2);
        pool.allocated(sid)[34] = number(1) & 0xff;
        book.depth = 1;
        player.postPop(sid, number(8));
    } else result = kind === 2 ? player.currentTechnology() : player.technology(number(1) | 0);
    const assertions = events.filter((event) => event.startsWith('A:')).length;
    const counts = ['0', '0', ...book.ownerFactories.map((list) => String(list.count))].join(',');
    const active = [];
    // Kernel 슬롯 순서대로 실제 PID 만 기록한다.
    if (kernel.size) for (let pid = 1; pid <= PROCESS_CAPACITY; pid += 1) if (kernel.get(pid)) active.push(pid);
    const fields = regular ? `${doubleBits(regular.time)},${regular.event},${new Uint32Array(new Float32Array([regular.payload]).buffer)[0]},${regular.count}` : '-';
    const form = regular ? regular.form : 0;
    // 현재 해시를 원본 네 단계 배열 순서로 이어 붙인다.
    const heads = Array.from({ length: SQUID_HASH_LEVELS }, (_, level) => hash.entries(level));
    const output = [String(result >>> 0), events.length ? events.join(';') : '-', hex(records), hex(pool.slot(sid)), String(concatWordAdler(heads, 2)), String(adler32(spots)),
        String(book.totalCost), String(book.productionDirty), String(book.depth), String(wordAdler(book.localCounts(), 4)), String(wordAdler(book.localSecondaryCounts(), 4)), String(wordAdler(book.globalCounts(), 4)),
        counts, '1', fields, String(form), form ? hex(pool.slot(form)) : '-', active.length ? active.join(',') : '-', String(clock.typeRotation), String(assertions)];
    kernel.clear();
    return output;
}

test('PlayerWorld: 원본 10.78 Player 네 몸체 840행이 실제 Factory/Pop/Owner/ProcessForm/Kernel 과 같다', { skip: !worldRows && '원본 fixture 없음' }, () => {
    let count = 0;
    // 행마다 독립 장면에서 실행해 22열 기대값과 비교한다.
    for (const row of worldRows) {
        assert.equal(row.length, 22);
        assert.deepEqual(replayWorldRow(row), row.slice(2), `Player 행 ${count} 입력 ${row[1]}`);
        count += 1;
    }
    assert.equal(count, 840);
});

// cpppj PlayerCurrentScene: PATCH 물리 슬롯 5 와 공통 부모(억제·깊이 1), 9기록과 상태를 만든다. (2026-11-08 WB04-E55)
function currentScene() {
    const pool = new SidPool(24000, { server: true });
    const types = Array.from({ length: 188 }, (_, number) => ({ cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, ...([1, 2, 3, 6].includes(number) ? { constructorAddress: typeConstructorAddress(number) } : {}) }));
    const book = new SquidPostPopState();
    const base = new SquidPostPop(pool, types, book);
    const records = new Uint8Array(9 * PLAYER_RECORD_STRIDE);
    const state = new PlayerWorldState();
    assert.equal(pool.allocate(2), 5);
    const raw = pool.allocated(5);
    raw.fill(0);
    put(raw, 0, PLAYER_VTABLE);
    raw[10] = 74;
    raw[34] = 1;
    put(raw, 36, 0xdeadbeef);
    book.depth = 1;
    book.suppressed = true;
    base.bindCurrentTypes(types);
    const events = [];
    // 예약 인자/순서만 관찰하고 기록/프레임 쓰기는 실제 몸체에 맡긴다.
    const hooks = () => ({
        reserveRegular: () => { events.push('N'); return true; },
        addRegular: (sid, event, payload) => { assert.ok(sid === 5 && event === 1 && payload === 5); events.push('R'); },
    });
    return { pool, types, book, base, records, state, raw: pool.physical(5), events, hooks };
}

test('PlayerParentCurrent: 실제 PATCH VT 는 다른 TYPE/free 에서도 기록을 먼저 쓰고 별도 Validate 는 기존 계약이다', () => {
    const scene = currentScene();
    scene.raw[11] = 1;
    scene.raw[40] = 8;
    const body = new RawPlayerWorld(scene.pool, scene.base, scene.records, scene.state, {});
    scene.base.setBody('player', body);
    assert.ok(body.handles(5));
    assert.ok(throws(() => body.validate(5, 0)));
    scene.base.postPopVirtual(5, 0);
    assert.deepEqual([get(scene.records, technologyAt(1)), scene.book.depth, get(scene.raw, 36)], [5, 0, 0xdeadbeef]);
});

test('PlayerParentCurrent: PlayerKnown 이 없으면 현재 owner 를 읽지 않고 0 을 돌려준다', () => {
    const scene = currentScene();
    scene.state.battleOwner = 65535;
    const body = new RawPlayerWorld(scene.pool, scene.base, scene.records, scene.state, {});
    assert.equal(body.currentTechnology(), 0);
    scene.state.playerKnown = 1;
    assert.ok(throws(() => body.currentTechnology()));
});

test('PlayerParentCurrent: 예약 장치 누락은 먼저 쓴 Technology SID 를 보존하고 special 프레임에 도달하지 않는다', () => {
    const scene = currentScene();
    scene.state.special = 1;
    const body = new RawPlayerWorld(scene.pool, scene.base, scene.records, scene.state, {});
    assert.ok(throws(() => body.postPop(5, 1)));
    assert.deepEqual([get(scene.records, technologyAt(1)), get(scene.raw, 36), scene.book.depth], [5, 0xdeadbeef, 1]);
});

test('PlayerParentCurrent: 예약 실패 뒤 바뀐 special 을 읽고 현재 free/TYPE/VT 와 무관하게 직접 frame 1 을 쓴다', () => {
    const scene = currentScene();
    const hooks = scene.hooks();
    delete hooks.addRegular;
    hooks.reserveRegular = () => {
        scene.events.push('N');
        assert.equal(get(scene.records, technologyAt(1)), 5);
        scene.state.special = 256;
        scene.state.battleOwner = 8;
        scene.raw[34] = 8;
        scene.raw[11] = 1;
        scene.raw[40] = 8;
        scene.raw[10] = 255;
        put(scene.raw, 0, 0);
        return false;
    };
    const body = new RawPlayerWorld(scene.pool, scene.base, scene.records, scene.state, hooks);
    body.postPop(5, 1);
    assert.deepEqual(scene.events, ['N']);
    assert.deepEqual([get(scene.records, technologyAt(1)), get(scene.records, technologyAt(8)), get(scene.raw, 36), scene.book.depth, scene.raw[11]], [5, 0, 1, 0, 1]);
});

test('PlayerParentCurrent: 성공한 예약 뒤 생성 장치 누락은 기록/예약을 남기고 special/공통 부모를 실행하지 않는다', () => {
    const scene = currentScene();
    const hooks = scene.hooks();
    delete hooks.addRegular;
    const body = new RawPlayerWorld(scene.pool, scene.base, scene.records, scene.state, hooks);
    assert.ok(throws(() => body.postPop(5, 1)));
    assert.deepEqual(scene.events, ['N']);
    assert.deepEqual([get(scene.records, technologyAt(1)), scene.book.depth], [5, 1]);
});

test('PlayerParentCurrent: Regular 생성 효과 뒤 special 을 다시 읽으며 처음 고른 local/fort 관문은 재검사하지 않는다', () => {
    const scene = currentScene();
    scene.state.special = 1;
    const hooks = scene.hooks();
    hooks.addRegular = () => {
        scene.events.push('R');
        scene.state.special = 0;
        scene.state.fortMode = true;
        scene.raw[34] = 0;
        scene.raw[11] = 1;
    };
    const body = new RawPlayerWorld(scene.pool, scene.base, scene.records, scene.state, hooks);
    body.postPop(5, 1);
    assert.deepEqual(scene.events, ['N', 'R']);
    assert.deepEqual([get(scene.raw, 36), get(scene.records, technologyAt(1)), get(scene.records, technologyAt(0)), scene.book.depth], [0xdeadbeef, 5, 0, 0]);
});

test('PlayerParentCurrent: owner BYTE 1 과 현재 DWORD 257 은 달라 예약하지 않으며 special 첫 프레임은 따로 실행한다', () => {
    const scene = currentScene();
    scene.state.battleOwner = 257;
    scene.state.special = 1;
    const body = new RawPlayerWorld(scene.pool, scene.base, scene.records, scene.state, {});
    body.postPop(5, 1);
    assert.deepEqual([get(scene.raw, 36), get(scene.records, technologyAt(1)), scene.book.depth], [1, 5, 0]);
});

test('PlayerParentCurrent: fort 또는 최초 비트가 없는 입력은 예약 장치 없이 기록과 공통 부모를 갱신한다', () => {
    // 서로 다른 두 미선택 관문을 독립 장면에서 확인한다.
    for (const flags of [1, 2]) {
        const scene = currentScene();
        scene.state.fortMode = flags === 1;
        scene.state.special = flags === 2 ? 1 : 0;
        const body = new RawPlayerWorld(scene.pool, scene.base, scene.records, scene.state, {});
        body.postPop(5, flags);
        assert.deepEqual([get(scene.records, technologyAt(1)), get(scene.raw, 36), scene.book.depth], [5, 0xdeadbeef, 0]);
    }
});

test('PlayerParentCurrent: 공통 부모의 선택 재검사 실패 앞에 Player 기록·예약·special 쓰기와 공통 통계가 남는다', () => {
    const scene = currentScene();
    scene.book.suppressed = false;
    scene.book.pendingPlacement = true;
    scene.book.localOwner = 1;
    scene.state.special = 1;
    const hooks = scene.hooks();
    hooks.reserveRegular = () => { scene.events.push('N'); return false; };
    const body = new RawPlayerWorld(scene.pool, scene.base, scene.records, scene.state, hooks);
    assert.ok(throws(() => body.postPop(5, 0x2000001)));
    assert.deepEqual([get(scene.records, technologyAt(1)), get(scene.raw, 36), scene.book.globalCounts()[74], scene.book.depth, scene.events.length], [5, 1, 1, 1, 1]);
});

test('PlayerParentCurrent: 9기록 밖 owner 는 진단으로 꾸미지 않고 물리 저장 직전에 거부한다', () => {
    const scene = currentScene();
    scene.raw[34] = 9;
    const before = scene.records.slice();
    const body = new RawPlayerWorld(scene.pool, scene.base, scene.records, scene.state, scene.hooks());
    assert.ok(throws(() => body.postPop(5, 1)));
    assert.deepEqual([scene.records, scene.events, scene.book.depth], [before, [], 1]);
});

test('PlayerParentCurrent: 진단 복귀 뒤 현재 TYPE 를 다시 읽고 Owner 뒤 물리 frame 0 을 쓴 다음 저장한 좌표로 Pop 한다', () => {
    const scene = currentScene();
    const hooks = scene.hooks();
    hooks.assertion = (kind) => {
        scene.events.push(kind === PLAYER_WORLD_ASSERTION.playerAlreadyExists ? 'A2' : 'A1');
        if (kind === PLAYER_WORLD_ASSERTION.playerAlreadyExists) scene.state.playerType = 76;
        else {
            scene.state.playerType = 74;
            scene.state.special = 1;
        }
    };
    hooks.find = (x, y, type) => {
        assert.ok(x === 18 && y === 10 && type === 74);
        scene.events.push('F');
        scene.state.playerType = 75;
        scene.state.special = 0;
        return 6;
    };
    hooks.create = (type, flags) => { assert.ok(type === 76 && flags === 2); scene.events.push('C'); return 5; };
    hooks.setOwner = (sid, value) => {
        assert.ok(sid === 5 && value === 8);
        scene.events.push('O');
        scene.raw[11] = 1;
        scene.raw[40] = 8;
    };
    hooks.pop = (sid, x, y, flags) => {
        assert.ok(sid === 5 && x === 18 && y === 10 && flags === 0 && get(scene.raw, 36) === 0 && scene.raw[11] === 1);
        scene.events.push('P');
    };
    const body = new RawPlayerWorld(scene.pool, scene.base, scene.records, scene.state, hooks);
    assert.equal(body.create(8), 5);
    assert.deepEqual(scene.events, ['A1', 'F', 'A2', 'C', 'O', 'P']);
});

test('PlayerParentCurrent: 생성/Owner 뒤 미연결 Pop 에 도달하기 전에 직접 frame 0 이 남는다', () => {
    const scene = currentScene();
    scene.state.playerKnown = 1;
    const hooks = {
        find: () => 0,
        create: () => 5,
        setOwner: (sid, value) => { scene.raw[34] = value & 0xff; scene.raw[11] = 1; },
    };
    const body = new RawPlayerWorld(scene.pool, scene.base, scene.records, scene.state, hooks);
    assert.ok(throws(() => body.create(8)));
    assert.deepEqual([scene.raw[34], scene.raw[11], get(scene.raw, 36)], [8, 1, 0]);
});

test('PlayerParentCurrent: 진단 효과가 갱신한 명시 owner 기록을 읽으며 현재 local owner 로 바꾸지 않는다', () => {
    const scene = currentScene();
    const hooks = {
        assertion: (kind) => {
            assert.equal(kind, PLAYER_WORLD_ASSERTION.playerIdKnown);
            scene.events.push('A');
            put(scene.records, technologyAt(8), 0x12345678);
            scene.state.battleOwner = 1;
        },
    };
    const body = new RawPlayerWorld(scene.pool, scene.base, scene.records, scene.state, hooks);
    assert.equal(body.technology(8), 0x12345678);
    assert.deepEqual([scene.events.length, body.currentTechnology()], [1, 0]);
});

test('PlayerParentCurrent: 실제 9기록 크기와 같은 풀 계약을 유지한다(웹 전용: 다른 풀·상태 거부 포함)', () => {
    const scene = currentScene();
    assert.ok(throws(() => new RawPlayerWorld(scene.pool, scene.base, scene.records.subarray(0, 8 * PLAYER_RECORD_STRIDE), scene.state, {})));
    const other = currentScene();
    assert.ok(throws(() => new RawPlayerWorld(scene.pool, other.base, scene.records, scene.state, {})));
    assert.ok(throws(() => new RawPlayerWorld(scene.pool, scene.base, scene.records, {}, {})));
    // 같은 9기록 저장소와 상태를 사본 없이 제공한다.
    const body = new RawPlayerWorld(scene.pool, scene.base, scene.records, scene.state, {});
    assert.ok(body.records === scene.records && body.state === scene.state && body.pool === scene.pool);
});
