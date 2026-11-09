// 섬 받침 삭제 훅 raw-island-lifecycle.js(RawIslandLifecycle)를 원본 10.78 관측과 cpppj `IslandLifecycleTests` 계약으로 대조한다.
//  - 원본 관측 islandlifecycle-x86.tsv 의 10.78(originals) 520행(IslandPre 260·SurfacePost 260): 장면(받침 SID 50 의 이웃·다리·walker·상태·extra)과 삭제 flags 에서 지연 낙하 예약(G)·walker 낙하(W)·공통 pre/postDestroy(X/Y)의
//    호출 순서와 인자(좌표 float 비트 포함)를 대조하고 풀 전체 불변을 확인한다. 훅은 호출 사실과 인자만 기록한다.
//  - 계약: 훅 누락/다른 풀 탐색기 거부와 이웃 없는 받침도 공통 preDestroy 를 부름, 실제 Process 호스트 위에서 지연 낙하(0x2692)가 살아 있는 다리에만 예약되고 다음 Kernel 프레임에 받침 중심 칸을 포장한 payload 로 불림.
// CD 10.72·10.37 행은 웹 범위 밖이다. 실제 게임·브라우저 실행이 아니다. (2026-11-10 WB04-E224: 섬 받침 삭제 훅 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, floatToBits } from './raw-support.mjs';
import { Scene, SOURCE, framesFor } from './raw-scene-support.mjs';
import { realPlayerAssets } from './player-scene-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { SquidUnpop } from '../www/core/squid-unpop.js';
import { RawSquidDestroy } from '../www/core/squid-destroy.js';
import { Kernel } from '../www/core/kernel.js';
import { SquidProcessHost, SquidProcessState, hasScheduledBridgeFall, scheduleBridgeFall } from '../www/core/squid-process.js';
import { RawSquidNeighbors } from '../www/core/raw-squid-neighbors.js';
import { ISLAND_DESTROY_KEEPS_BRIDGES, RawIslandLifecycle } from '../www/core/raw-island-lifecycle.js';

// 저장된 독립 기계어 출력(장면 입력과 관찰 행)을 한 번만 읽는다. originals 관찰만 쓴다. (2026-11-10 WB04-E224)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'islandlifecycle-x86.tsv'));
const scenes = rows ? new Map(rows.filter((row) => row[0] === 'Scene').map((row) => [row[1], row.slice(2)])) : null;
const observations = rows ? rows.filter((row) => row[0] !== 'Scene' && row[1] === 'originals') : null;

// 판본마다 기대하는 관찰 행 수이다. (2026-11-10 WB04-E224)
const ROWS_PER_KIND = 260;
// 지연 낙하 사건 번호이다. (2026-11-10 WB04-E224)
const BRIDGE_FALL_EVENT = 0x2692;

// 예외가 나는지 본다. (2026-11-10 WB04-E224)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 원본 실행기의 대체와 같은 경계: 호출 사실과 인자만 기록한다. (2026-11-10 WB04-E224)
function hooksFor(scene) {
    return {
        scheduleFall: (bridge, x, y) => scene.events.push(`G:${bridge}:${floatToBits(x)}:${floatToBits(y)}`),
        fallWalker: (walker) => scene.events.push(`W:${walker}`),
        basePreDestroy: (sid, flags) => scene.events.push(`X:${sid}:${flags >>> 0}`),
        basePostDestroy: (sid, flags) => scene.events.push(`Y:${sid}:${flags >>> 0}`),
    };
}

test('섬 삭제 훅: 원본 10.78 IslandPre/SurfacePost 520행의 호출 순서·인자가 같고 풀은 변하지 않는다', { skip: !rows && '원본 fixture 없음' }, () => {
    const scene = new Scene();
    const counts = { IslandPre: 0, SurfacePost: 0 };
    for (const row of observations) {
        assert.equal(row.length, 6);
        const kind = row[0];
        scene.events = [];
        scene.prepare(scenes.get(row[2]));
        scene.refreshNeighbors();
        const before = scene.pool.bytes().slice();
        const lifecycle = new RawIslandLifecycle(scene.pool, scene.neighbors, hooksFor(scene));
        if (kind === 'IslandPre') lifecycle.islandPreDestroy(SOURCE, Number(row[3]));
        else if (kind === 'SurfacePost') lifecycle.surfacePostDestroy(SOURCE, Number(row[3]));
        else assert.fail(`알 수 없는 행 종류 ${kind}`);
        const label = `${kind} 행 ${counts[kind]} (장면 ${row[2]} flags ${row[3]})`;
        assert.equal(scene.eventText(), row[5], label);
        assert.deepEqual(scene.pool.bytes(), before, `${label} 풀 불변`);
        counts[kind] += 1;
    }
    assert.deepEqual(counts, { IslandPre: ROWS_PER_KIND, SurfacePost: ROWS_PER_KIND });
});

test('섬 삭제 훅 계약: 훅 누락과 다른 풀의 탐색기는 생성 때 거부하고 이웃 없는 받침도 공통 preDestroy 를 부른다', { skip: !rows && '원본 fixture 없음' }, () => {
    const scene = new Scene();
    scene.prepare(['50,91,0,0,1106247680,1106247680,3,3,671088643,16777216,8,2', '-', '0', '-', '82,1,0,0,1,2,3,4,5,6,7,8,1']);
    scene.refreshNeighbors();
    const hooks = hooksFor(scene);
    assert.ok(throws(() => new RawIslandLifecycle(scene.pool, scene.neighbors, { ...hooks, scheduleFall: undefined })));
    assert.ok(throws(() => new RawIslandLifecycle(scene.pool, scene.neighbors, { ...hooks, basePostDestroy: undefined })));
    const other = new Scene();
    assert.ok(throws(() => new RawIslandLifecycle(other.pool, scene.neighbors, hooks)));
    // 이웃이 없는 받침도 공통 preDestroy 는 항상 부른다.
    new RawIslandLifecycle(scene.pool, scene.neighbors, hooks).islandPreDestroy(SOURCE, 0x1234);
    assert.equal(scene.eventText(), 'X:50:4660');
});

test('섬 삭제 훅 계약: 표면 칸의 음수 좌표는 0 쪽으로 잘라(floor 가 아니다) 0 칸의 walker 를 낙하시키고 abstract/buried 표면은 건너뛴다', { skip: !rows && '원본 fixture 없음' }, () => {
    const scene = new Scene();
    // 표면(50)은 x = -0.5(0xBF000000), y = 31.0(0x41F80000)이고 walker(60)는 해시 2단계의 (0.5, 31.5), walker(61)은 같은 단계의 (10.5, 31.5)에 있다.
    // 0 쪽으로 자른 칸은 (0, 31)이라 60 만 걸린다. floor 로 자르면 x = -1 이 탐색기의 "끝 = -1"(월드 끝까지) 의미가 되어 61 도 걸린다.
    // 음수 좌표는 해시에 넣을 수 없으므로 양수 좌표로 장면을 만든 뒤 표면의 raw x 만 바꾼다.
    const entries = (extra) => [`50,93,0,${extra},1056964608,1106771968,1,1,134219779,16777218,2,0;60,85,0,0,1056964608,1107034112,1,1,430098,2162688,2,2;61,85,0,0,1093140480,1107034112,1,1,430098,2162688,2,2`, '-', '0', '-', '82,1,0,0,1,2,3,4,5,6,7,8,1'];
    scene.prepare(entries(0));
    put(scene.pool.physical(SOURCE), 14, 3204448256);
    scene.refreshNeighbors();
    new RawIslandLifecycle(scene.pool, scene.neighbors, hooksFor(scene)).surfacePostDestroy(SOURCE, 7);
    assert.equal(scene.eventText(), 'W:60;Y:50:7');
    // abstract(1)/buried(8) 표면은 낙하 순회 없이 공통 postDestroy 만 부른다.
    for (const extra of [1, 8]) {
        scene.prepare(entries(extra));
        put(scene.pool.physical(SOURCE), 14, 3204448256);
        scene.refreshNeighbors();
        new RawIslandLifecycle(scene.pool, scene.neighbors, hooksFor(scene)).surfacePostDestroy(SOURCE, 7);
        assert.equal(scene.eventText(), 'Y:50:7', `extra ${extra}`);
    }
});

test('섬 삭제 훅 계약: 실제 프로세스 계층에서 받침이 사라질 때 살아 있는 다리에만 지연 낙하가 예약되고 다음 Kernel 프레임에 받침 중심 칸이 payload 로 불린다', () => {
    const pool = new SidPool(32768, { server: true });
    const hash = new SquidHash();
    const spots = new Uint8Array(65536);
    const types = realPlayerAssets().types.map((record) => ({ ...record }));
    const bridgeType = 82;
    const isleType = 97;
    const islandType = 94;
    Object.assign(types[bridgeType], { flags1: 0x802, flags2: 4, footX: 1, footY: 1 });
    Object.assign(types[isleType], { flags1: 0x28000803, flags2: 2, footX: 1, footY: 1 });
    Object.assign(types[islandType], { flags1: 0x28000003, flags2: 0x01000000, footX: 3, footY: 3 });
    // 합성 프레임 표: 번호 1 이 K(동서), 2 가 A(네 방향)다.
    const frames = types.map(() => []);
    for (const number of [bridgeType, isleType, islandType]) frames[number] = framesFor(0);
    // 객체를 직접 배치한다: 가상 표 기록값, 타입, 위치, 프레임, 해시 체인. 할당 직후의 void 상태는 지운다.
    const place = (type, x, y, frame, level, state) => {
        const sid = pool.allocate(2);
        const raw = pool.allocated(sid);
        put(raw, 0, 0x11010000);
        raw[10] = type;
        raw[11] = state;
        put(raw, 14, floatToBits(x));
        put(raw, 18, floatToBits(y));
        put(raw, 0x24, frame);
        put(raw, 4, hash.bucket(level, x, y), 2);
        hash.setBucket(level, x, y, sid);
        return sid;
    };
    const island = place(islandType, 30, 30, 8, 2, 0);
    const east = place(bridgeType, 31, 29, 1, 0, 0);
    const south = place(bridgeType, 29, 31, 2, 0, 2);
    place(isleType, 27, 30, 2, 0, 0);
    const unpop = new SquidUnpop(pool, hash, spots);
    const destroy = new RawSquidDestroy(pool, unpop, types);
    const kernel = new Kernel();
    const processState = new SquidProcessState();
    processState.now = 10;
    const handled = [];
    const host = new SquidProcessHost(pool, types, kernel, destroy, processState, {
        regular: (parent, event, _count, payload) => { handled.push(`${parent}:${event}:${Math.trunc(payload)}`); return 0; },
        asset: { emit: () => {}, selected: () => 0 },
    });
    const neighbors = new RawSquidNeighbors(pool, hash, spots, types, frames);
    const order = [];
    const lifecycle = new RawIslandLifecycle(pool, neighbors, {
        scheduleFall: (bridge, x, y) => { order.push('fall'); assert.ok(scheduleBridgeFall(host, bridge, x, y)); },
        fallWalker: () => order.push('walker'),
        basePreDestroy: () => order.push('pre'),
        basePostDestroy: () => order.push('post'),
    });
    // 0x1000 이 있으면 다리를 건드리지 않고 공통 preDestroy 만 부른다.
    lifecycle.islandPreDestroy(island, ISLAND_DESTROY_KEEPS_BRIDGES);
    assert.deepEqual(order, ['pre']);
    assert.ok(!hasScheduledBridgeFall(host, east));
    // 없으면 살아 있는 다리에만 예약한 뒤 공통 preDestroy 를 부른다.
    order.length = 0;
    lifecycle.islandPreDestroy(island, 0);
    assert.deepEqual(order, ['fall', 'pre']);
    assert.ok(hasScheduledBridgeFall(host, east) && !hasScheduledBridgeFall(host, south));
    // 받침 (30,30)·3×3 의 중심은 (28.5, 28.5)이고 payload 는 (28 & 255) | (28 << 8) = 7196 이다.
    kernel.runFrame();
    assert.deepEqual(handled, [`${east}:${BRIDGE_FALL_EVENT}:7196`]);
});
