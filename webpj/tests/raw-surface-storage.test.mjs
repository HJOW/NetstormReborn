// Client 의 실제 배열을 Session 보다 오래 보관하는 장기 저장소(cpppj `client/RawSurfaceStorage`)의 웹 대응 `core/raw-surface-storage.js` 를 검사한다(4-B, WB04-E298).
// cpppj `RawSurfaceStorageTests` 의 장기 물리 계약을 RawSurfaceWorld 없이 저장소 수준으로 옮겼다(월드 모듈이 빌리는 같은 객체의 동일성은 RawSurfaceWorld 단위에서 다시 확인한다):
//  - 구성: 같은 길이 입력 표 복사, 풀 32,768·서버 권한 alias, 초기 시작 값(표면 종류 82/94/95/155/157, 붕괴 스캔 -1/-1/0/0, 주기 10, 바람 0/1/0, 실패점 -10000).
//  - acquire(): 중복 대여를 먼저 거부하고 일반 설치의 fresh 입력만 호환(두 번째 세대부터 풀/해시/spot 재초기화, TYPE 보존, Graph/집계/섬/청크/사제 owner/State 목록/권한/GameAux/실패점)한다.
//  - acquirePrepared(): 초기화 없이 같은 물리·모드를 대여하며 중복은 같은 관문에서 거부, 반납 뒤 일반 acquire 는 다시 초기화한다. 같은 객체 동일성이 대여 사이에 유지된다.
//  - 장기 장치(표시 진단/상세·할당 진단·Graph 시계/추적·checking·주변 바람)와 지연 생성 process owner.
// 원본 관측 파일이 없는 단위 계약 검사이다. 실제 게임·브라우저 실행이 아니다. (2026-11-11 WB04-E298)
import test from 'node:test';
import assert from 'node:assert/strict';
import { FrameCodeTable } from '../www/assets/terrain-tiles.js';
import { GameAuxState } from '../www/core/game-aux-state.js';
import { GameRandom } from '../www/core/random.js';
import { RawAuthorityState } from '../www/core/raw-authority-state.js';
import { createAmbientWindState, stepAmbientWind } from '../www/core/raw-ambient-wind.js';
import { RawGraphStorage, resetRawGraphTable } from '../www/core/raw-graph.js';
import { RawProcessHostStorage } from '../www/core/raw-process-host-storage.js';
import { RawSurfaceStorage, SurfaceUse } from '../www/core/raw-surface-storage.js';
import { SidPool } from '../www/core/sid-pool.js';

// 188칸 입력 표(TYPE 이름·셰이프 프레임 수 포함)와 프레임 표이다. (2026-11-11 WB04-E298)
function inputs() {
    const types = Array.from({ length: 188 }, (_, number) => ({ cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, level: 0, name: `type${number}` }));
    const frames = Array.from({ length: 188 }, (_, number) => new FrameCodeTable(number === 0 ? [] : [{ side: 0x41, variant: 0x41, number: 0, flags: 0 }]));
    const shapes = Array.from({ length: 188 }, () => ({ frameCount: 3 }));
    return { types, frames, shapes };
}

// 새 저장소를 만든다. (2026-11-11 WB04-E298)
function make(options = {}) {
    const { types, frames, shapes } = inputs();
    return new RawSurfaceStorage(types, frames, shapes, options);
}

// 오류가 나는지 확인한다(성공을 무조건 인정하지 않음). (2026-11-11 WB04-E298)
function rejects(action) {
    try {
        action();
    } catch {
        return true;
    }
    return false;
}

// 구성: 같은 길이 입력만 받고 복사하며, 초기 시작 값과 풀/권한 정체성이 원본 값이다. (2026-11-11 WB04-E298)
test('저장소: 구성과 초기 시작 값', () => {
    const { types, frames, shapes } = inputs();
    assert.throws(() => new RawSurfaceStorage(types, frames.slice(1), shapes), /크기 불일치/);
    assert.throws(() => new RawSurfaceStorage(types, frames, shapes.slice(1)), /크기 불일치/);
    assert.throws(() => new RawSurfaceStorage(null, frames, shapes), TypeError);
    const authority = new RawAuthorityState();
    const gameAux = new GameAuxState();
    const storage = new RawSurfaceStorage(types, frames, shapes, { authority, gameAux, source: 'assets' });
    // 입력 표는 복사해 보관한다.
    types[0].level = 77;
    assert.equal(storage.types[0].level, 0);
    assert.notEqual(storage.types, types);
    assert.equal(storage.frames.length, 188);
    assert.equal(storage.codes.length, 188);
    assert.deepEqual(storage.codes[1], [{ side: 0x41, variant: 0x41, number: 0, flags: 0 }]);
    assert.equal(storage.codes[0].length, 0);
    assert.equal(storage.authority, authority);
    assert.equal(storage.gameAux, gameAux);
    assert.equal(storage.source, 'assets');
    assert.equal(make().source, null);
    assert.ok(make().authority instanceof RawAuthorityState && make().gameAux instanceof GameAuxState);
    // 풀은 32,768 이고 서버 권한은 장기 권한 owner 의 현재 값을 읽는다.
    assert.ok(storage.pool instanceof SidPool && storage.pool.capacity === 32768);
    assert.equal(storage.pool.isServer, true);
    authority.server = false;
    assert.equal(storage.pool.isServer, false);
    authority.server = true;
    assert.equal(storage.spots.length, 65536);
    assert.equal(storage.islands.length, 3204);
    assert.equal(storage.priestOwnerState.length, 9);
    assert.deepEqual([storage.disallowSquidAllocation.value, storage.borrowed, storage.generation], [0, false, 0]);
    // 표면 종류와 초기 스캔/주기/바람/실패점.
    assert.deepEqual([storage.links.bridgeType, storage.links.islandType, storage.links.stalagType, storage.links.connectorType, storage.links.noIslandType], [82, 94, 95, 155, 157]);
    assert.deepEqual(storage.bridgeDecayScan, { first: -1, last: -1, cursor: 0, next: 0 });
    assert.equal(storage.bridgeDecayPeriod, 10);
    assert.deepEqual(storage.wind, { x: 0, y: 1, angle: 0 });
    assert.deepEqual(storage.boardTargetState.invalid, { x: -10000, y: -10000 });
    assert.deepEqual([storage.ownerMode.fort, storage.ownerMode.battle, storage.ownerMode.challenge], [false, false, false]);
    assert.equal(storage.geyserType, gameAux.geyserType);
    storage.geyserType = 321;
    assert.equal(gameAux.geyserType, 321);
});

// 일반 대여: 중복은 풀 초기화보다 먼저 거부하고, 두 번째 세대부터 같은 배열을 재초기화하며 TYPE 은 보존한다. (2026-11-11 WB04-E298)
test('저장소: 일반 대여는 중복을 먼저 거부하고 같은 배열을 다시 초기화한다', () => {
    const storage = make();
    const pool = storage.pool;
    const hash = storage.hash;
    const spots = storage.spots;
    const types = storage.types;
    const freshFree = pool.freeCount;
    const first = storage.acquire();
    assert.ok(first instanceof SurfaceUse && storage.borrowed && storage.generation === 1);
    // 첫 세대는 풀을 다시 초기화하지 않는다: 만든 객체와 hash/spot/TYPE 입력을 둔다.
    const sid = pool.allocate(2);
    pool.allocated(sid)[10] = 94;
    hash.setCell(0, 1, 1, sid);
    spots[257] = 0x71;
    types[0].level = 0x13579;
    assert.ok(pool.freeCount < freshFree);
    const before = pool.bytes();
    // 중복 대여는 거부되고 현재 물리를 바꾸지 않는다.
    assert.throws(() => storage.acquire(), /중복 대여/);
    assert.throws(() => storage.acquirePrepared(), /중복 대여/);
    assert.ok(storage.borrowed && storage.generation === 1);
    assert.deepEqual(pool.bytes(), before);
    first.release();
    assert.ok(!storage.borrowed && storage.generation === 1);
    assert.equal(pool.slot(sid)[10], 94);
    // 반납은 멱등이고 다른 대여를 풀지 못한다.
    first.release();
    const second = storage.acquire();
    first.release();
    assert.ok(storage.borrowed);
    // 두 번째 세대는 같은 배열을 재초기화하고 TYPE 전역은 그대로 둔다.
    assert.equal(storage.generation, 2);
    assert.equal(storage.pool, pool);
    assert.equal(storage.hash, hash);
    assert.equal(storage.spots, spots);
    assert.equal(storage.types, types);
    assert.equal(pool.freeCount, freshFree);
    assert.equal(hash.cell(0, 1, 1), 0);
    assert.equal(spots[257], 0);
    assert.equal(types[0].level, 0x13579);
    second.release();
    // 위조한 대여는 만들 수 없다.
    assert.throws(() => new SurfaceUse(Symbol('x'), storage), TypeError);
});

// 준비 대여: Native 정리가 끝난 물리를 초기화 없이 대여해 주소/모드/바이트를 보존하고, 이후 일반 대여는 다시 초기화한다. (2026-11-11 WB04-E298)
test('저장소: 준비 대여는 물리와 모드를 초기화 없이 보존한다', () => {
    const storage = make();
    storage.acquire().release();
    // 앞선 명시 정리의 실제 하위를 호출한다.
    storage.pool.reset(0);
    storage.hash.reset();
    storage.spots.fill(0);
    storage.ownerMode.fort = true;
    storage.ownerMode.battle = false;
    storage.links.battle = false;
    storage.links.mission = false;
    storage.hash.setCell(0, 1, 1, 0x1234);
    storage.spots[257] = 0x71;
    storage.types[0].level = 0x13579;
    storage.authority.server = false;
    storage.authority.boss = false;
    storage.gameAux.lostX = 5;
    const bytes = storage.pool.bytes();
    const prepared = storage.acquirePrepared();
    assert.ok(storage.generation === 2 && storage.borrowed);
    assert.deepEqual(storage.pool.bytes(), bytes);
    assert.equal(storage.hash.cell(0, 1, 1), 0x1234);
    assert.equal(storage.spots[257], 0x71);
    assert.equal(storage.types[0].level, 0x13579);
    assert.ok(storage.ownerMode.fort && !storage.ownerMode.battle && !storage.links.battle && !storage.links.mission);
    // 권한/GameAux 의 일반 fresh 쓰기는 준비 대여에 없다.
    assert.deepEqual([storage.authority.server, storage.authority.boss, storage.gameAux.lostX], [false, false, 5]);
    assert.ok(rejects(() => storage.acquirePrepared()) && rejects(() => storage.acquire()));
    prepared.release();
    assert.ok(!storage.borrowed && storage.hash.cell(0, 1, 1) === 0x1234 && storage.spots[257] === 0x71);
    // 이어지는 일반 대여는 명시 초기화 계약을 유지한다.
    const ordinary = storage.acquire();
    assert.ok(storage.generation === 3 && storage.hash.cell(0, 1, 1) === 0 && storage.spots[257] === 0);
    assert.deepEqual([storage.authority.server, storage.authority.boss, storage.gameAux.lostX], [true, true, 0]);
    ordinary.release();
});

// Graph 기록/스택: 같은 두 객체를 대여 사이에 유지하고 준비 대여는 보존, 일반 대여는 전체 0 뒤 작은 reset 과 같다. (2026-11-11 WB04-E298)
test('저장소: Graph 기록과 스택은 같은 객체로 유지되고 일반 대여만 fresh 로 되돌린다', () => {
    const storage = make();
    const graph = storage.graph;
    const records = graph.records;
    const stack = graph.stack;
    const first = storage.acquire();
    graph.records[1] = { surfaces: 9, inUse: 1, reserved: 0x4567 };
    graph.records[252] = { surfaces: 3, inUse: 4, reserved: 0x2345 };
    graph.records[254].reserved = 0x3456;
    graph.stack[0] = 0x10203040;
    graph.stack[graph.stack.length - 1] = 0xa1b2c3d4;
    const snapshot = JSON.stringify([...graph.records]) + Array.from(graph.stack).join(',');
    first.release();
    const prepared = storage.acquirePrepared();
    assert.equal(storage.graph, graph);
    assert.equal(storage.graph.records, records);
    assert.equal(storage.graph.stack, stack);
    assert.equal(JSON.stringify([...graph.records]) + Array.from(graph.stack).join(','), snapshot);
    assert.ok(rejects(() => storage.acquire()));
    assert.equal(JSON.stringify([...graph.records]) + Array.from(graph.stack).join(','), snapshot);
    prepared.release();
    // 일반 대여: 종전 새 지역 Graph 의 전체 0 뒤 작은 reset 과 같다.
    storage.acquire().release();
    const expected = new RawGraphStorage();
    resetRawGraphTable(expected);
    assert.deepEqual(storage.graph.records, expected.records);
    assert.deepEqual(storage.graph.stack, expected.stack);
    assert.equal(storage.graph.records, records);
    assert.equal(storage.graph.stack, stack);
    // 다른 저장소의 물리는 분리된다.
    const other = make();
    assert.notEqual(other.graph.records, records);
    assert.notEqual(other.graph.stack, stack);
});

// 집계 3표·사제 owner·State SID 목록·섬 3,204BYTE·청크 지도: 준비 대여는 보존, 일반 대여는 fresh 로 초기화(같은 객체 위에서). (2026-11-11 WB04-E298)
test('저장소: 집계/사제 owner/목록/섬/청크는 준비 대여에서 보존되고 일반 대여에서 초기화된다', () => {
    const storage = make();
    const parts = [storage.totalMade.localCounts, storage.totalMade.localSecondaryCounts, storage.totalMade.globalCounts, storage.priestOwnerState, storage.islands, storage.chunks, storage.chunks.entries[5], storage.sidLists.factories.entries];
    storage.acquire().release();
    // 비영 현재 값을 모든 물리 칸에 둔다.
    for (let index = 0; index < 256; index += 1) {
        storage.totalMade.localCounts[index] = index + 1;
        storage.totalMade.localSecondaryCounts[index] = index + 2;
        storage.totalMade.globalCounts[index] = index + 3;
    }
    storage.priestOwnerState.fill(7);
    storage.islands[4] = 0x55;
    storage.islands[3203] = 0x66;
    storage.chunks.mode = 0x89abcdef;
    storage.chunks.at(3, 4).island = 7;
    storage.chunks.at(3, 4).side = 'P';
    storage.chunks.at(3, 4).record.set([1, 2, 3, 4, 5, 6]);
    storage.chunks.at(3, 4).ordinal = 71;
    storage.sidLists.factories.entries[0] = 99;
    storage.sidLists.factories.count = 3;
    const prepared = storage.acquirePrepared();
    assert.equal(storage.totalMade.localCounts[255], 256);
    assert.equal(storage.totalMade.globalCounts[0], 3);
    assert.ok(storage.priestOwnerState.every((value) => value === 7));
    assert.equal(storage.islands[4], 0x55);
    assert.equal(storage.islands[3203], 0x66);
    assert.equal(storage.chunks.mode, 0x89abcdef);
    assert.deepEqual([storage.chunks.at(3, 4).island, storage.chunks.at(3, 4).ordinal, storage.sidLists.factories.count, storage.sidLists.factories.entries[0]], [7, 71, 3, 99]);
    prepared.release();
    storage.acquire();
    assert.ok(storage.totalMade.localCounts.every((value) => value === 0) && storage.totalMade.localSecondaryCounts.every((value) => value === 0) && storage.totalMade.globalCounts.every((value) => value === 0));
    assert.ok(storage.priestOwnerState.every((value) => value === 0));
    // 섬 기록 100행은 지정 필드가 fresh 값이다: 첫 기록의 지역 BYTE(+12)는 255, 마지막 경계 float(-10000)의 상위 BYTE 는 0xc6, 임의로 쓴 BYTE 4 는 0 이다.
    assert.equal(storage.islands[4], 0);
    assert.equal(storage.islands[4 + 12], 255);
    assert.equal(storage.islands[3203], 0xc6);
    assert.deepEqual([storage.chunks.mode, storage.chunks.xLength, storage.chunks.yLength], [1, 16, 16]);
    assert.deepEqual([storage.chunks.at(3, 4).island, storage.chunks.at(3, 4).side, storage.chunks.at(3, 4).ordinal], [0x7f, '', 0]);
    assert.deepEqual([...storage.chunks.at(3, 4).record], [0, 0, 0, 0, 0, 0]);
    assert.deepEqual([storage.sidLists.factories.count, storage.sidLists.factories.entries[0]], [0, 0]);
    // 같은 객체 위에서 초기화되어 참조가 유지된다.
    assert.equal(parts[0], storage.totalMade.localCounts);
    assert.equal(parts[1], storage.totalMade.localSecondaryCounts);
    assert.equal(parts[2], storage.totalMade.globalCounts);
    assert.equal(parts[3], storage.priestOwnerState);
    assert.equal(parts[4], storage.islands);
    assert.equal(parts[5], storage.chunks);
    assert.equal(parts[6], storage.chunks.entries[5]);
    assert.equal(parts[7], storage.sidLists.factories.entries);
});

// 권한 owner: 대여 사이에 같은 객체를 유지하고 일반 대여는 서버/보스를 true 로, 실패점은 -10000 으로 되돌린다. (2026-11-11 WB04-E298)
test('저장소: 권한 owner 와 실패점은 일반 대여에서만 fresh 로 쓴다', () => {
    const authority = new RawAuthorityState();
    const gameAux = new GameAuxState();
    const storage = make({ authority, gameAux });
    authority.server = false;
    authority.boss = false;
    storage.boardTargetState.invalid.x = 5;
    storage.boardTargetState.invalid.y = 6;
    const prepared = storage.acquirePrepared();
    assert.deepEqual([authority.server, authority.boss, storage.boardTargetState.invalid.x, storage.boardTargetState.invalid.y], [false, false, 5, 6]);
    prepared.release();
    gameAux.lostX = 9;
    gameAux.geyserType = 77;
    storage.acquire();
    assert.deepEqual([authority.server, authority.boss, storage.boardTargetState.invalid.x, storage.boardTargetState.invalid.y], [true, true, -10000, -10000]);
    // 일반 월드 재설정이 GameAux 의 월드 입력을 되돌린다(같은 owner 객체).
    assert.deepEqual([gameAux.lostX, gameAux.geyserType], [0, 122]);
    assert.equal(storage.gameAux, gameAux);
    assert.equal(storage.authority, authority);
});

// 장기 장치: 표시 진단/상세·할당 진단·Graph 시계/추적·checking·주변 바람은 연결한 장치를 그대로 쓰고 미연결은 선택 시에만 실패한다. (2026-11-11 WB04-E298)
test('저장소: 장기 진단 장치와 주변 바람', () => {
    const storage = make();
    // 미연결 장치는 선택된 지점에서만 실패한다.
    assert.throws(() => storage.assertDisplay('x', 'f', 1), /진단 장치 미연결/);
    assert.throws(() => storage.recordAssertionDetail('t'), /상세 Client owner 미연결/);
    assert.throws(() => storage.recordDisplayFrameError(10, 1, 0), /상세 Client owner 미연결/);
    assert.equal(storage.readGraphClock(), 0);
    storage.traceGraph(1, 'ignored');
    assert.equal(storage.checking(), 0);
    assert.deepEqual(storage.allocationDiagnostics, { dump: null, tell: null });
    // 연결.
    const detail = { value: null };
    const calls = [];
    storage.bindDisplayDiagnostics(detail, (...args) => calls.push(args));
    storage.assertDisplay('expr', 'file', 7);
    assert.deepEqual(calls, [['expr', 'file', 7]]);
    storage.recordAssertionDetail('문구');
    assert.equal(detail.value, '문구');
    storage.recordDisplayFrameError(12, 5, 1);
    assert.equal(detail.value, 'Frame assertion failed. Asking for type=12 (type12) frame=5 abs=1\n\nType has 3 frames\n');
    // 시계/추적/checking/할당 진단.
    let now = 4.5;
    storage.bindGraphClock(() => now);
    assert.equal(storage.readGraphClock(), 4.5);
    now = 6;
    assert.equal(storage.readGraphClock(), 6);
    const trace = [];
    storage.bindGraphTrace((level, text) => trace.push([level, text]));
    storage.traceGraph(0x20, 'merge');
    assert.deepEqual(trace, [[0x20, 'merge']]);
    let word = 0;
    storage.bindChecking(() => word);
    word = 0x80000000;
    assert.equal(storage.checking(), 0x80000000);
    assert.equal(storage.angleDiagnostics().checking(), 0x80000000);
    storage.angleDiagnostics().assertion('a', 'b', 2);
    assert.deepEqual(calls[1], ['a', 'b', 2]);
    const dump = () => {};
    const tell = () => {};
    storage.bindAllocationDiagnostics(dump, tell);
    assert.deepEqual(storage.allocationDiagnostics, { dump, tell });
    assert.throws(() => storage.bindDisplayDiagnostics(null, () => {}), TypeError);
    assert.throws(() => storage.bindDisplayDiagnostics(detail, null), TypeError);
    assert.throws(() => storage.bindChecking(null), TypeError);
    assert.throws(() => storage.bindGraphClock(null), TypeError);
    assert.throws(() => storage.bindGraphTrace(null), TypeError);
    assert.throws(() => storage.bindAllocationDiagnostics(dump, null), TypeError);
    // 주변 바람: 같은 Client 난수를 직접 소비하고 상태는 대여로 재초기화되지 않는다.
    const random = new GameRandom(77);
    const oracle = new GameRandom(77);
    const expected = createAmbientWindState();
    for (let step = 0; step < 30; step += 1) {
        storage.updateAmbientWind(random);
        stepAmbientWind(expected, oracle, { checking: () => 0, assertion: () => {} });
    }
    assert.deepEqual(storage.wind, expected);
    assert.equal(random.state, oracle.state);
    storage.acquire().release();
    assert.deepEqual(storage.wind, expected);
});

// 장기 process owner: 첫 사용에 지연 생성해 같은 객체를 유지하고 같은 Pool/TYPE/권한/보조 owner 를 쓴다. 최종 소멸 뒤에는 새로 만든다. (2026-11-11 WB04-E298)
test('저장소: process owner 는 지연 생성되고 같은 물리를 빌린다', () => {
    const authority = new RawAuthorityState();
    const gameAux = new GameAuxState();
    const storage = make({ authority, gameAux });
    const processes = storage.processes;
    assert.ok(processes instanceof RawProcessHostStorage);
    assert.equal(storage.processes, processes);
    assert.equal(processes.host.pool, storage.pool);
    authority.boss = false;
    gameAux.regularProcessType = 51;
    assert.deepEqual([processes.state.boss, processes.state.regularType], [false, 51]);
    // 일반 대여/반납을 거쳐도 같은 owner 이다.
    storage.acquire().release();
    assert.equal(storage.processes, processes);
    storage.dispose();
    assert.notEqual(storage.processes, processes);
    storage.dispose();
});

// ───────────── 변형 확인으로 보강한 계약 ─────────────

// 첫 세대는 풀을 다시 초기화하지 않고, 입력 표/코드 표는 복사본이며, 낡은 대여는 새 대여를 풀지 못한다. (2026-11-11 WB04-E298)
test('저장소: 첫 세대 보존, 입력 표 복사, 낡은 대여의 영향 없음', () => {
    const { types, frames, shapes } = inputs();
    const storage = new RawSurfaceStorage(types, frames, shapes);
    assert.notEqual(storage.frames, frames);
    assert.notEqual(storage.codes[1], frames[1].codes);
    assert.notEqual(storage.shapes, shapes);
    // 첫 acquire 는 풀/해시/spot 을 다시 초기화하지 않는다: 대여 전에 둔 입력이 남는다.
    const sid = storage.pool.allocate(2);
    storage.hash.setCell(0, 2, 2, sid);
    storage.spots[9] = 0x33;
    storage.acquire().release();
    assert.equal(storage.hash.cell(0, 2, 2), sid);
    assert.equal(storage.spots[9], 0x33);
    assert.equal(storage.pool.slot(sid)[10], 0);
    assert.ok(storage.pool.freeCount < 32768 - 5);
    // 낡은 대여(최종 소멸로 이미 끝난 것)는 새 대여를 풀지 못한다.
    const old = storage.acquire();
    storage.dispose();
    const current = storage.acquire();
    old.release();
    assert.ok(storage.borrowed);
    current.release();
    assert.ok(!storage.borrowed);
});

// 일반 대여는 섬 저장소의 지정 필드 밖(padding)도 0 으로 되돌리고 청크 지도의 가로/세로 크기를 기본 16×16 으로 돌린다. (2026-11-11 WB04-E298)
test('저장소: 일반 대여는 섬 padding 과 청크 크기도 fresh 로 되돌린다', () => {
    const storage = make();
    storage.acquire().release();
    storage.islands[4 + 14] = 0xaa;
    storage.islands[4 + 15] = 0xbb;
    storage.chunks.xLength = 5;
    storage.chunks.yLength = 7;
    const prepared = storage.acquirePrepared();
    assert.equal(storage.islands[4 + 14], 0xaa);
    assert.deepEqual([storage.chunks.xLength, storage.chunks.yLength], [5, 7]);
    prepared.release();
    storage.acquire();
    assert.equal(storage.islands[4 + 14], 0);
    assert.equal(storage.islands[4 + 15], 0);
    assert.deepEqual([storage.chunks.xLength, storage.chunks.yLength], [16, 16]);
});

// 최종 소멸은 지연 생성된 process owner 의 잔류도 반납한다. (2026-11-11 WB04-E298)
test('저장소: 최종 소멸은 process owner 의 잔류를 반납한다', () => {
    const storage = make();
    let destroyed = 0;
    void storage.processes.host;
    storage.processes.kernel.add({ runFrame() {}, destroy() { destroyed += 1; } });
    assert.equal(destroyed, 0);
    storage.dispose();
    assert.equal(destroyed, 1);
});
