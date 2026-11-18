// 실제 Client 장기 Pool과 전역 권한을 써서 월드 없는 다리 스캔과 프레임 접합을 검사한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { runSurfaceBridgeDecay, runClientBridgeDecay, reportClientBridgeDecay } from '../www/core/client-bridge-decay.js';
import { ClientGameplayGlobals } from '../www/core/client-gameplay-globals.js';
import { RawSurfaceStorage } from '../www/core/raw-surface-storage.js';
import { resetStateBridgeDecayScan } from '../www/core/raw-state-bridge-reset.js';
import { CampaignSessionAssets } from '../www/core/raw-campaign-session-owners.js';
import { PriestPlacementAssets } from '../www/core/priest-placement-assets.js';
import { GameAssets } from '../www/assets/game-assets.js';
import { GamePalette } from '../www/assets/palette.js';
import { readData } from './helpers.mjs';
import { realCaptureAssets } from './capture-assets-support.mjs';

// 보호된 실제 10.78 자산은 읽기만 한다.
const data = realCaptureAssets(), game = new GameAssets(data.table, data.database, GamePalette.parse(readData('d/GIFCLOUD.COL')));
// 같은 권한·보조 전역을 빌리는 실제 Client 표면 저장소를 만든다.
function scene() {
    const globals = new ClientGameplayGlobals(), assets = new CampaignSessionAssets(game, { read: () => { throw new Error('선택하지 않은 파일'); } });
    const metadata = new PriestPlacementAssets(game.typeTable, game.shapes, assets.loadedTypes);
    const storage = new RawSurfaceStorage(assets.types, assets.frames, metadata.shapes, { source: game, authority: globals.authority(), gameAux: globals.gameAux() });
    return { storage, globals };
}

// 빈 실제 풀에서 커서·기한과 현재 정지/권한/편집 상태를 원본과 같이 지킨다.
test('Client 다리 붕괴: 월드 없는 장기 스캔과 현재 관문', () => {
    const { storage, globals } = scene(), scan = storage.bridgeDecayScan, calls = []; resetStateBridgeDecayScan(scan, storage.bridgeDecayPeriod, 0);
    const cell = (sid) => calls.push(sid); runSurfaceBridgeDecay(storage, globals, { game: 1, wall: 1, delta: 0.5, number: 1 }, false, cell);
    assert.equal(scan.cursor, 15400); assert.equal(scan.next, 10); assert.deepEqual(calls, []); assert.equal(storage.borrowed, false);
    globals.authority().boss = false; runSurfaceBridgeDecay(storage, globals, { game: 2, delta: 1 }, false, cell); assert.equal(scan.cursor, 15400);
    globals.authority().boss = true; globals.world.editor = 1; runSurfaceBridgeDecay(storage, globals, { game: 3, delta: 1 }, false, cell); assert.equal(scan.cursor, 15400);
    globals.world.editor = 0; runSurfaceBridgeDecay(storage, globals, { game: 4, delta: 1 }, true, cell); assert.equal(scan.cursor, 15400); assert.equal(scan.next, 10);
    assert.throws(() => runSurfaceBridgeDecay(storage, new ClientGameplayGlobals(), { game: 4, delta: 1 }, false, cell), /권한 소유자 불일치/); assert.equal(scan.cursor, 15400);
});

// Client 프레임은 선택된 칸을 현재 Runtime에 보내고 같은 현재 시각/주기를 재조회한다.
test('Client 다리 붕괴: 현재 Runtime 칸 처리와 읽기 전용 보고', () => {
    const { storage, globals } = scene(), clock = { game: 1, wall: 1, delta: 1, number: 1 }, calls = [];
    const empty = { surfaceStorageIfInitialized: () => null }; assert.equal(reportClientBridgeDecay(empty), ''); runClientBridgeDecay(empty);
    const client = { surfaceStorageIfInitialized: () => storage, gameplayGlobals: () => globals, time: () => clock, paused: () => false };
    resetStateBridgeDecayScan(storage.bridgeDecayScan, 10, 0); const before = reportClientBridgeDecay(client); assert.match(before, /^bridge_scan_global\t15000\t23001\t15000\t10\t10\t0\t1\t1\t0\t\d+\n$/);
    const sid = storage.pool.allocate(2), raw = storage.pool.allocated(sid); raw[10] = 82; raw[11] = 0; assert.ok(storage.types[82].flags2 & 4);
    Object.assign(storage.bridgeDecayScan, { first: sid, last: sid + 1, cursor: sid, next: 0 });
    const use = globals.acquirePrepared(); use.publish({ world: () => ({ decayBridgeCell: (current, selected) => { calls.push([current, selected]); current.game = 20; storage.bridgeDecayPeriod = 4; } }) });
    try {
        runClientBridgeDecay(client); assert.equal(calls.length, 1); assert.equal(calls[0][0], clock); assert.equal(calls[0][1], sid);
        assert.equal(storage.bridgeDecayScan.cursor, sid); assert.equal(storage.bridgeDecayScan.next, 24);
        const snapshot = storage.pool.bytes(), report = reportClientBridgeDecay(client); assert.equal(report, `bridge_scan_global\t${sid}\t${sid + 1}\t${sid}\t24\t4\t0\t1\t1\t1\t1\n`); assert.deepEqual(storage.pool.bytes(), snapshot);
    } finally { use.release(); }
});
