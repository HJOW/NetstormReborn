// Dude 생성(00450a20)과 최초 Pop 후처리(00450cd0)를 원본 x86 fixture 와 cpppj 계약 검사로 대조한다(cpppj DudeSpawnTests, 10.78).
//  - dudespawn-x86.tsv originals 144행: population 용량/개수·차단·좌표 float 비트·소유자·최초 후처리 여부 입력과, 새 SID·population 전체(stale 꼬리 포함)·슬롯 50바이트·
//    해시 네 단계 머리/spot Adler·장부(비용·생산 dirty·깊이·Totalmade 세 표·소유자별 공장 수)·표시 억제·변경 사각형 출력. originalCD 144행은 범위 밖이다.
//  - 장면은 실제 보호 Dude(TYPE 70, 65클러스터/130물리프레임) 자료 위의 SidPool·SquidHash·SquidFactory·SquidOwner·SquidPop/Unpop·SquidDisplay(DirtyRegions sink)·SquidFrame·
//    SquidPostPop 과 이 단위의 RawDudeSpawn/RawDudePostPop 으로 조립한다(cpppj SpawnScene). 각 행은 입력만 설치하고 기대 칸은 비교에만 쓴다.
//  - DudeParentCurrent_* 계약 검사(현재 목록 연결·물리 슬롯·Bridge 통지·signed 관문·부분 효과 보존)는 cpppj 손검사를 1:1 로 옮겼다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E206: Dude 생성/최초 Pop 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get, adler32, bitsToFloat } from './raw-support.mjs';
import { realPlayerAssets, hex, wordAdler, concatWordAdler } from './player-scene-support.mjs';
import { realCaptureAssets } from './capture-assets-support.mjs';
import { FrameCodeTable } from '../www/assets/terrain-tiles.js';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash, SQUID_HASH_LEVELS } from '../www/core/squid-hash.js';
import { SquidUnpop } from '../www/core/squid-unpop.js';
import { SquidPostPopState, SquidPostPopList } from '../www/core/squid-postpop-state.js';
import { SquidPostPop } from '../www/core/squid-postpop.js';
import { SquidPop, RAW_POP_RESULT } from '../www/core/squid-pop.js';
import { SquidFactory } from '../www/core/squid-factory.js';
import { SquidOwner, SquidOwnerMode } from '../www/core/squid-owner.js';
import { SquidDisplay } from '../www/core/squid-display.js';
import { SquidFrame } from '../www/core/squid-frame.js';
import { makeSquidFrameHooks } from '../www/core/squid-frame-binding.js';
import { DirtyRegions } from '../www/render/dirty-regions.js';
import { RawDudeSpawn, RawDudePostPop, createDudeSpawnState, makeDudePostPopHooks } from '../www/core/raw-dude-spawn.js';

// 원본 x86 fixture 의 10.78 행이다. (2026-11-09 WB04-E206)
const rows = (readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'dudespawn-x86.tsv')) ?? []).filter((row) => row[0] === 'originals');

// 예외가 지정 종류로 나는지 본다. (2026-11-09 WB04-E206)
function throws(action, kind = Error) {
    try { action(); } catch (error) { return error instanceof kind; }
    return false;
}

// 188칸 빈 TYPE 레코드이다(cpppj RiftTypeRecord 기본값). (2026-11-09 WB04-E206)
const emptyRecord = (number) => ({ number, flags1: 0, flags2: 0, footX: 0, footY: 0, group: 0, maxHitPoints: 0, cost: 0, zOrder: 0, constructorAddress: 0 });

// 한 번만 만든 실제 Dude 장면 자료이다. (2026-11-09 WB04-E206)
let cachedAssets = null;

// 실제 보호 TYPE(최대 HP 포함)·Run 표·SHP 헤더이다. 원본 준비용 미등록 base 74 는 빈 합성 타입이다(cpppj DudeAssets). (2026-11-09 WB04-E206)
function dudeAssets() {
    if (cachedAssets) return cachedAssets;
    const player = realPlayerAssets();
    const capture = realCaptureAssets();
    const types = player.types.map((record, number) => ({ ...record, maxHitPoints: capture.types[number].maxHitPoints }));
    types[74] = emptyRecord(74);
    // 자산 번호 70 부터 실제 프레임 Run 을 채운다.
    const frames = types.map((_, number) => {
        const definition = number >= 70 ? capture.table.definitions[number - 70] : null;
        return definition ? FrameCodeTable.fromDefinition(definition) : new FrameCodeTable([]);
    });
    assert.deepEqual([types[70].footX, types[70].footY, types[70].maxHitPoints, frames[70].run('I').first, capture.shapes[70].frameCount], [0, 0, 20, 64, 65]);
    cachedAssets = { types, frames, shapes: capture.shapes };
    return cachedAssets;
}

// 원본 상태와 실제 모든 생성/공간/표시/후처리 처리기를 소유한 장면이다(cpppj SpawnScene). (2026-11-09 WB04-E206)
function spawnScene(row) {
    const assets = dudeAssets();
    const { types, frames, shapes } = assets;
    const pool = new SidPool(32768, { server: true });
    const hash = new SquidHash();
    const spots = new Uint8Array(65536);
    const book = new SquidPostPopState();
    const population = new SquidPostPopList();
    const state = createDudeSpawnState(population, { blocked: Number(row[3]) !== 0 });
    const dirty = new DirtyRegions(1024, 768);
    const sink = { suppressed: () => dirty.fullRedraw, invalidate: (rect, flags) => dirty.add({ left: rect.left, top: rect.top, right: rect.right, bottom: rect.bottom }, flags) };
    const display = new SquidDisplay(types, shapes, sink, { cameraX: 0, cameraY: 0, zoom: 65536, viewport: { left: 0, top: 0, right: 1024, bottom: 768 } });
    const unpop = new SquidUnpop(pool, hash, spots, display);
    const post = new SquidPostPop(pool, types, book);
    const pop = new SquidPop(pool, hash, spots, { display, postPop: post });
    const factory = new SquidFactory(pool, types, { unpop });
    const owner = new SquidOwner(pool, types, book, new SquidOwnerMode());
    const frame = new SquidFrame(pool, types, makeSquidFrameHooks(pool, types, display, unpop, pop));
    const dudePost = new RawDudePostPop(pool, frames, state, makeDudePostPopHooks(pool, frame, post));
    assert.equal(factory.create(74, 2), 5);
    population.entries = Array.from({ length: Number(row[1]) }, (_, index) => (0x60000000 + index) >>> 0);
    population.count = Number(row[2]);
    book.localOwner = 1;
    // 원본 seed 0 과 각 표의 서로 다른 17 오프셋을 유지한다.
    [book.localCounts(), book.localSecondaryCounts(), book.globalCounts()].forEach((table, which) => {
        // 표 전체 칸을 같은 식으로 채운다.
        for (let index = 0; index < table.length; index += 1) table[index] = (Math.imul(index, 2654435761) + which * 17) >>> 0;
    });
    post.setBody('dude', dudePost);
    return { pool, hash, spots, book, population, state, dirty, display, pop, factory, owner, dudePost, types };
}

// 장면에서 생성/최초 후처리를 실행하고 raw/공간/장부/변경 표를 원본 순서로 직렬화한다. (2026-11-09 WB04-E206)
function runSpawn(scene, row) {
    const { pool, display, pop, types } = scene;
    const hooks = {
        pop: (sid, x, y, flags) => {
            const raw = pool.slot(sid);
            const size = display.frameSize(raw, 0);
            if (pop.pop(sid, types[raw[10]], size[0], size[1], x, y, flags) !== RAW_POP_RESULT.registered) throw new Error('Dude 실제 Pop 등록 실패');
        },
    };
    const spawn = new RawDudeSpawn(pool, scene.factory, scene.owner, scene.state, hooks);
    const born = spawn.spawn(bitsToFloat(Number(row[4])), bitsToFloat(Number(row[5])), Number(row[6]));
    if (born && Number(row[7])) scene.dudePost.postPop(born, 0x50);
    const populationText = scene.population.entries.length ? scene.population.entries.join(',') : '-';
    const heads = concatWordAdler(Array.from({ length: SQUID_HASH_LEVELS }, (_, level) => scene.hash.entries(level)), 2);
    const counts = ['0', '0', ...scene.book.ownerFactories.map((list) => String(list.count))].join(',');
    const rectangles = scene.dirty.entries.map((entry) => `${entry.rect.left},${entry.rect.top},${entry.rect.right},${entry.rect.bottom},${entry.flags}`).join(';') || '-';
    const raw = hex(pool.slot(born || 6));
    return [String(born), String(scene.population.count), populationText, raw, String(heads), String(adler32(scene.spots)), String(scene.book.totalCost), String(scene.book.productionDirty),
        String(scene.book.depth), String(wordAdler(scene.book.localCounts(), 4)), String(wordAdler(scene.book.localSecondaryCounts(), 4)), String(wordAdler(scene.book.globalCounts(), 4)),
        counts, '1', raw, scene.dirty.fullRedraw ? '1' : '0', rectangles];
}

test('DudeSpawn: 원본 10.78 144행의 실제 factory/owner/Pop/Frame/후처리와 capacity/차단 결과가 같다', { skip: rows.length === 0 && 'dudespawn-x86.tsv 없음' }, () => {
    let count = 0;
    // 기대 칸은 실제 실행 뒤 비교에만 쓴다.
    for (const row of rows) {
        assert.equal(row.length, 25);
        const output = runSpawn(spawnScene(row), row);
        assert.deepEqual(output, row.slice(8), `행 ${count} 용량 ${row[1]} 개수 ${row[2]} 차단 ${row[3]} 소유자 ${row[6]} 후처리 ${row[7]}`);
        count += 1;
    }
    assert.equal(count, 144);
});

// 현재 목록/물리 슬롯/타입 Run 과 실제 Factory/Owner/공통 부모를 조립한 계약 장면이다(cpppj DudeCurrentScene). (2026-11-09 WB04-E206)
function currentScene() {
    const pool = new SidPool(24000, { server: true });
    const types = Array.from({ length: 188 }, (_, number) => emptyRecord(number));
    const population = new SquidPostPopList();
    const state = createDudeSpawnState(population);
    const frames = types.map(() => new FrameCodeTable([]));
    const book = new SquidPostPopState();
    const base = new SquidPostPop(pool, types, book);
    const factory = new SquidFactory(pool, types);
    const owner = new SquidOwner(pool, types, book, new SquidOwnerMode());
    assert.equal(pool.allocate(2), 5);
    const raw = pool.allocated(5);
    raw.fill(0);
    put(raw, 0, 0x508cc8);
    raw[10] = 74;
    raw[34] = 1;
    put(raw, 12, 0xbeef, 2);
    const code = (side, flags = 0) => ({ side: side.charCodeAt(0), variant: 0x41, flags });
    frames[74] = new FrameCodeTable([code('A'), code('I')]);
    frames[75] = new FrameCodeTable([code('A'), code('A', 1), code('I')]);
    population.entries = [0xaaaa0000, 0xbbbb0000];
    book.suppressed = true;
    book.depth = 1;
    base.bindCurrentTypes(types);
    const events = [];
    const scene = { pool, types, population, state, frames, book, base, factory, owner, raw, events };
    // Frame 장치의 인자와 공통 부모만 관찰하며 제품의 QA/목록 쓰기를 대신하지 않는다.
    scene.hooks = () => ({
        setFrame: (sid, value, flags) => {
            assert.ok(sid === 5 && flags === 0);
            events.push(`F${value}`);
            put(raw, 36, value >>> 0);
        },
        validateBase: () => { throw new Error('별도 사전 검사'); },
        base: (sid, flags) => {
            events.push('B');
            base.postPopBase(sid, flags);
        },
    });
    return scene;
}

// 원문 signed 관문/저장을 보는 현재 목록 연결을 만든다. (2026-11-09 WB04-E206)
function bindCurrent(population, { storage, capacity, readCount, writeCount }) {
    population.entries = [];
    population.count = 0;
    population.bindCurrent({ storage, capacity, readCount, writeCount });
}

test('DudeParentCurrent: 최초 비트가 없으면 물리 슬롯/목록/프레임 장치 없이 공통 부모만 실행한다', () => {
    // 최초 비트가 없는 독립 입력은 잘못된 SID 도 몸체 안에서 먼저 읽지 않는다.
    for (const flags of [0, 2, 4]) {
        const scene = currentScene();
        const hooks = scene.hooks();
        hooks.setFrame = null;
        new RawDudePostPop(scene.pool, [], scene.state, hooks).postPop(24000, flags);
        assert.deepEqual(scene.events, ['B']);
        assert.deepEqual([scene.book.depth, scene.population.count, get(scene.raw, 12, 2)], [0, 0, 0xbeef]);
    }
});

test('DudeParentCurrent: 실제 VT 로 고른 다른 TYPE·free/contained 슬롯도 QA/등록/현재 Run/공통 부모를 원문 순서로 실행한다', () => {
    const scene = currentScene();
    scene.raw[11] = 1;
    scene.raw[40] = 8;
    const body = new RawDudePostPop(scene.pool, scene.frames, scene.state, scene.hooks());
    scene.base.setBody('dude', body);
    assert.equal(body.handles(5), true);
    assert.ok(throws(() => body.validate(5, 1)));
    scene.base.postPopVirtual(5, 1);
    assert.deepEqual([get(scene.raw, 12, 2), scene.population.count, scene.population.entries[0], scene.population.entries[1], get(scene.raw, 36), scene.book.depth, scene.raw[11], scene.raw[40]],
        [0x5101, 1, 5, 0xbbbb0000, 1, 0, 1, 8]);
});

test('DudeParentCurrent: 현재 Bridge 통지 미연결이면 QA 만 남기고 목록/프레임/부모를 실행하지 않는다', () => {
    const scene = currentScene();
    const body = new RawDudePostPop(scene.pool, scene.frames, scene.state, scene.hooks(), () => 74);
    assert.ok(throws(() => body.postPop(5, 1)));
    assert.deepEqual([get(scene.raw, 12, 2), scene.population.count, scene.population.entries[0], scene.events.length], [0x5101, 0, 0xaaaa0000, 0]);
});

test('DudeParentCurrent: 통지 복귀 뒤 현재 슬롯/TYPE 의 Run 을 읽으며 없는 I 의 -1 도 프레임 장치에 전달한다', () => {
    const scene = currentScene();
    const hooks = scene.hooks();
    hooks.notifySurface = (sid) => {
        assert.ok(sid === 5 && get(scene.raw, 12, 2) === 0x5101);
        scene.events.push('N');
        scene.raw[10] = 75;
        scene.raw[11] = 1;
        scene.raw[40] = 8;
        put(scene.raw, 0, 0);
        scene.frames[75] = new FrameCodeTable([]);
    };
    new RawDudePostPop(scene.pool, scene.frames, scene.state, hooks, () => 74).postPop(5, 1);
    assert.deepEqual(scene.events, ['N', 'F-1', 'B']);
    assert.deepEqual([scene.population.count, get(scene.raw, 36), scene.raw[11]], [1, 0xffffffff, 1]);
});

test('DudeParentCurrent: Bridge 비교는 현재 DWORD 전체를 쓴다(BYTE 74 ≠ 330)', () => {
    const scene = currentScene();
    new RawDudePostPop(scene.pool, scene.frames, scene.state, scene.hooks(), () => 74 + 256).postPop(5, 1);
    assert.deepEqual([scene.population.count, scene.events.length, scene.events[0], scene.book.depth], [1, 2, 'F1', 0]);
});

test('DudeParentCurrent: 가득 찼거나 signed 음수 용량이면 배열을 읽지 않고 프레임/부모를 계속한다', () => {
    // unsigned 비교와 전수 count 검사를 구별하는 세 현재 입력이다.
    for (const [used, capacity] of [[1, 1], [2, 1], [0, 0x80000000]]) {
        const scene = currentScene();
        bindCurrent(scene.population, {
            storage: () => { throw new Error('미선택 배열'); },
            capacity: () => capacity,
            readCount: () => used,
            writeCount: () => { throw new Error('미선택 개수 쓰기'); },
        });
        new RawDudePostPop(scene.pool, scene.frames, scene.state, scene.hooks()).postPop(5, 1);
        assert.deepEqual([scene.events.length, scene.events[0], scene.book.depth, get(scene.raw, 12, 2)], [2, 'F1', 0, 0x5101]);
    }
});

test('DudeParentCurrent: 배열 조회 효과 뒤 저장 index 는 보존하고 증가는 그때의 현재 개수를 읽는다', () => {
    const scene = currentScene();
    const storage = [0xaaaa0000, 0xbbbb0000];
    let count = 0;
    bindCurrent(scene.population, {
        storage: () => { count = 7; scene.raw[10] = 75; return storage; },
        capacity: () => 2,
        readCount: () => count,
        writeCount: (value) => { assert.equal(storage[0], 5); count = value; },
    });
    new RawDudePostPop(scene.pool, scene.frames, scene.state, scene.hooks()).postPop(5, 1);
    assert.deepEqual([count, storage[0], storage[1], scene.events[0], get(scene.raw, 36), scene.book.depth], [8, 5, 0xbbbb0000, 'F2', 2, 0]);
});

test('DudeParentCurrent: 음수 개수는 signed 비교를 통과한 뒤 실제 저장 위치에서만 거부하고 QA 를 되돌리지 않는다', () => {
    const scene = currentScene();
    scene.population.count = 0xffffffff;
    assert.ok(throws(() => new RawDudePostPop(scene.pool, scene.frames, scene.state, scene.hooks()).postPop(5, 1)));
    assert.deepEqual([get(scene.raw, 12, 2), scene.population.count, scene.events.length, scene.population.entries[0]], [0x5101, 0xffffffff, 0, 0xaaaa0000]);
});

test('DudeParentCurrent: 고른 배열의 물리 범위 오류는 최초 QA 뒤에만 확인한다', () => {
    const scene = currentScene();
    bindCurrent(scene.population, {
        storage: () => [],
        capacity: () => 1,
        readCount: () => 0,
        writeCount: () => { throw new Error('도달하지 않을 개수 쓰기'); },
    });
    assert.ok(throws(() => new RawDudePostPop(scene.pool, scene.frames, scene.state, scene.hooks()).postPop(5, 1)));
    assert.deepEqual([get(scene.raw, 12, 2), scene.events.length], [0x5101, 0]);
});

test('DudeParentCurrent: 도달한 프레임 장치가 없으면 앞선 QA/목록 저장과 개수 증가를 보존한다', () => {
    const scene = currentScene();
    const hooks = scene.hooks();
    hooks.setFrame = null;
    assert.ok(throws(() => new RawDudePostPop(scene.pool, scene.frames, scene.state, hooks).postPop(5, 1)));
    assert.deepEqual([get(scene.raw, 12, 2), scene.population.count, scene.population.entries[0], scene.events.length, scene.book.depth], [0x5101, 1, 5, 0, 1]);
});

test('DudeParentCurrent: TYPE+150 물리 표 읽기가 프레임 장치 요구보다 앞선다', () => {
    const scene = currentScene();
    const hooks = scene.hooks();
    hooks.setFrame = null;
    assert.ok(throws(() => new RawDudePostPop(scene.pool, scene.frames.slice(0, 74), scene.state, hooks).postPop(5, 1), RangeError));
    assert.deepEqual([scene.population.count, scene.population.entries[0], get(scene.raw, 12, 2), scene.events.length], [1, 5, 0x5101, 0]);
});

test('DudeParentCurrent: 공통 부모 연결 실패 앞에 실제 QA/등록/프레임 장치 효과가 남는다', () => {
    const scene = currentScene();
    const hooks = scene.hooks();
    hooks.base = null;
    assert.ok(throws(() => new RawDudePostPop(scene.pool, scene.frames, scene.state, hooks).postPop(5, 1)));
    assert.deepEqual([scene.population.count, get(scene.raw, 36), scene.events.join(','), scene.book.depth], [1, 1, 'F1', 1]);
});

test('DudeParentCurrent: 명시 사전 검사는 실제 호출 앞에 끼우지 않는다', () => {
    const scene = currentScene();
    const body = new RawDudePostPop(scene.pool, scene.frames, scene.state, scene.hooks());
    assert.ok(throws(() => body.validate(5, 1)));
    assert.deepEqual([get(scene.raw, 12, 2), scene.population.count], [0xbeef, 0]);
    assert.ok(throws(() => new RawDudePostPop({ edition: 'cd1072' }, [], scene.state, {}), TypeError));
});

test('DudeParentCurrent: flags1 의 QA 저장은 풀 밖 SID 를 거부하고 그 뒤 부모를 실행하지 않는다', () => {
    const scene = currentScene();
    assert.ok(throws(() => new RawDudePostPop(scene.pool, scene.frames, scene.state, scene.hooks()).postPop(24000, 1)));
    assert.deepEqual([scene.events.length, scene.population.count], [0, 0]);
});

test('DudeParentCurrent: 생성 거부 관문은 현재 signed 개수/용량만 읽고 TYPE/좌표/배열을 미리 검사하지 않는다', () => {
    // 가득 참/초과/음수 용량은 실제 Factory 에 도달하지 않는다.
    for (const [used, capacity] of [[0, 0], [2, 1], [0, 0x80000000]]) {
        const scene = currentScene();
        scene.state.dudeType = 188;
        bindCurrent(scene.population, {
            storage: () => { throw new Error('미선택 생성 배열'); },
            capacity: () => capacity,
            readCount: () => used,
            writeCount: () => { throw new Error('미선택 생성 개수 쓰기'); },
        });
        const spawn = new RawDudeSpawn(scene.pool, scene.factory, scene.owner, scene.state, {});
        const before = scene.pool.freeCount;
        assert.equal(spawn.spawn(bitsToFloat(0x7fc12345), 0, 9999), 0);
        assert.equal(scene.pool.freeCount, before);
    }
});

test('DudeParentCurrent: 현재 용량 조회 효과가 바꾼 차단 전역을 그 뒤에 읽는다', () => {
    const scene = currentScene();
    scene.state.dudeType = 188;
    bindCurrent(scene.population, { storage: () => [], capacity: () => { scene.state.blocked = true; return 2; }, readCount: () => 0, writeCount: () => {} });
    const spawn = new RawDudeSpawn(scene.pool, scene.factory, scene.owner, scene.state, {});
    assert.equal(spawn.spawn(1, 2, 1), 0);
    assert.equal(scene.state.blocked, true);
});

test('DudeParentCurrent: 실제 Factory/Owner 와 frame 0 뒤에만 미연결 Pop 이 실패하며 NaN 을 미리 거부하지 않는다', () => {
    const scene = currentScene();
    scene.state.dudeType = 74;
    const spawn = new RawDudeSpawn(scene.pool, scene.factory, scene.owner, scene.state, {});
    assert.ok(throws(() => spawn.spawn(bitsToFloat(0x7fc12345), 12.5, 8)));
    const born = scene.pool.slot(6);
    assert.deepEqual([born[10], born[11] & 1, born[34], get(born, 36), scene.population.count], [74, 0, 8, 0, 0]);
});

test('DudeParentCurrent: 음수 개수는 생성 관문만 통과하며 배열 저장은 실제 Pop 하위에 맡긴다', () => {
    const scene = currentScene();
    scene.population.count = 0xffffffff;
    scene.state.dudeType = 74;
    let calls = 0;
    const spawn = new RawDudeSpawn(scene.pool, scene.factory, scene.owner, scene.state, {
        pop: (sid, x, y, flags) => {
            assert.ok(Number.isNaN(x));
            assert.deepEqual([sid, y, flags, scene.pool.slot(sid)[34], get(scene.pool.slot(sid), 36)], [6, 12.5, 0, 8, 0]);
            calls += 1;
        },
    });
    assert.equal(spawn.spawn(bitsToFloat(0x7fc12345), 12.5, 8), 6);
    assert.deepEqual([calls, scene.population.count], [1, 0xffffffff]);
});

test('RawDudeSpawn: 풀이 다른 factory/owner·Frame/Base 연결을 거부한다(웹 계약 검사)', () => {
    const scene = currentScene();
    const other = new SidPool(24000);
    assert.ok(throws(() => new RawDudeSpawn(other, scene.factory, scene.owner, scene.state, {}), TypeError));
    assert.ok(throws(() => makeDudePostPopHooks(other, { pool: scene.pool }, scene.base), TypeError));
    assert.ok(throws(() => new RawDudePostPop(scene.pool, scene.frames, scene.state, {}, 82), TypeError));
});
