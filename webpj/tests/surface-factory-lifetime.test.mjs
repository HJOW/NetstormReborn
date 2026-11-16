// Storage의 실제 장기 Factory/공유 깊이/공간·표시 부모와 World 대여를 대조한다(WB04-E593).
// 보호 자산과 실제 Pool/Form을 쓰는 225553df 소스 계약이며 새 원본 관측/미션 플레이는 아니다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { GameAssets } from '../www/assets/game-assets.js';
import { RawSurfaceStorage } from '../www/core/raw-surface-storage.js';
import { RawSurfaceWorld } from '../www/core/raw-surface-world.js';
import { CampaignSessionAssets } from '../www/core/raw-campaign-session-owners.js';
import { PriestPlacementAssets } from '../www/core/priest-placement-assets.js';
import { BridgeConnectState } from '../www/core/raw-bridge-connect.js';
import { SquidDisplay } from '../www/core/squid-display.js';
import { RawSquidContainedUnpop, commonContainedParent, commonContainedPrevious, commonContainedSetPrevious } from '../www/core/squid-contained-unpop.js';
import { realCaptureAssets } from './capture-assets-support.mjs';
import { FrameCodeTable } from '../www/assets/terrain-tiles.js';
import { get, put, floatToBits } from './raw-support.mjs';

// 선택하지 않아야 하는 외부 효과는 빈 성공 대신 실패한다.
function forbidden() { throw new Error('선택하지 않은 외부 효과'); }
// 보호 자료는 한 번 해석하며 실제 Storage는 각 검사에서 별도로 소유한다.
const data = realCaptureAssets(); const game = new GameAssets(data.table, data.database);
const assets = new CampaignSessionAssets(game, { read: forbidden });
const metadata = new PriestPlacementAssets(game.typeTable, game.shapes, assets.loadedTypes);
// 실제 물리/공유 owner를 빌리는 장기 저장소이다.
function scene() { return new RawSurfaceStorage(assets.types, assets.frames, metadata.shapes, { source: game }); }
// 현재 장기 물리를 그대로 쓰며 준비 대여 선택만 외부에서 받는다.
function world(storage, prepared = false) {
    const links = new BridgeConnectState();
    Object.assign(links, { bridgeType: 82, islandType: 94, stalagType: 95, connectorType: 155, noIslandType: 157, battle: true, mission: true });
    return new RawSurfaceWorld(storage.types, storage.frames, storage.shapes, links, 97, { storage, adoptPrepared: prepared });
}
// 값형 카메라와 실제 viewport의 독립 기대 입력이다.
function view(x = 0) { return { cameraX: x, cameraY: 0, zoom: 65536, viewport: { left: 0, top: 0, right: 640, bottom: 480 } }; }
// 실제 표시 대여를 만들며 이번에 선택하지 않는 출력은 명시 실패다.
function displayRuntime(x, changes = {}) { return { view: () => view(x), suppressed: forbidden, invalidate: forbidden, ...changes }; }
// 별도 부모에 붙은 실제 GumpForm을 만든다. 0x10은 원문 전송 억제이다.
function attached(storage) { const parent = storage.factory.create(75, 0); const form = storage.factory.createGumpForm(0); storage.factory.attachForm(form, parent, 0x10); return { parent, form }; }
// 공통 Unpop 접두 뒤 실제 분리 부모만 호출한다.
function detach(storage, sid) {
    new RawSquidContainedUnpop(storage.pool, { contentFormType: () => 6, gumpFormType: () => 3, uiEvent: () => 0 }, {
        parent: (form) => commonContainedParent(storage.pool, form), previous: (form) => commonContainedPrevious(storage.pool, form),
        setPrevious: (form, previous) => commonContainedSetPrevious(storage.pool, form, previous), parentNotice: forbidden,
    }).detach(sid);
}

test('장기 Factory: 최초 HP/Create/void Take는 World/표시/공간 대여를 요구하지 않는다', () => {
    const s = scene(); const factory = s.factory; const bytes = s.pool.bytes();
    assert.equal(factory, s.factory); assert.equal(s.borrowed, false); assert.equal(s.generation, 0);
    assert.deepEqual(s.pool.bytes(), bytes); const parent = factory.create(75, 0);
    assert.equal(s.pool.slot(parent)[10], 75); assert.equal(factory.readSelectedHitPoints(s.types[75], s.types), s.types[75].maxHitPoints);
    assert.equal(factory.take(3, 15100), 15100); assert.equal(s.borrowed, false);
    assert.throws(() => s.display, /view.*미연결/);
});

test('장기 Factory: 두 일반 World와 준비 대여가 같은 공장/공유 깊이/표시/Unpop/건물/종속 부모를 쓴다', () => {
    const s = scene(); const factory = s.factory; const book = s.postPopState; const depth = s.postPopDepth; const one = world(s); const core = one.core();
    assert.equal(core.factory, factory); assert.equal(core.bookkeeping, book); assert.equal(core.display, s.display); assert.equal(core.unpop, s.unpop); assert.equal(core.building, s.building);
    const display = s.display; const unpop = s.unpop; const building = s.building; const contained = s.containedUnpop;
    one.dispose(); book.depth = 17; const prepared = world(s, true); assert.equal(prepared.core().bookkeeping.depth, 17); prepared.dispose();
    const two = world(s); assert.equal(two.core().bookkeeping.depth, 0);
    assert.deepEqual([two.core().factory, two.core().display, two.core().unpop, two.core().building, s.containedUnpop, s.postPopState, s.postPopDepth], [factory, display, unpop, building, contained, book, depth]); two.dispose();
});

test('장기 후처리: 준비 대여/반납은 깊이·생산dirty·소유자 목록을 보존하고 일반 대여만 새 값으로 돌린다', () => {
    const s = scene(); const book = s.postPopState; const owners = book.ownerFactories; const ai = book.aiAttached;
    book.depth = 9; book.productionDirty = 0x1234; book.graphsEnabled = true; book.ownerFactories[1].entries = [15100]; book.ownerFactories[1].count = 1; book.aiAttached[1] = true;
    s.acquirePrepared().release(); assert.deepEqual([book.depth, book.productionDirty, owners[1].items(), ai[1]], [9, 0x1234, [15100], true]);
    s.acquire().release(); assert.equal(book, s.postPopState); assert.equal(owners, book.ownerFactories); assert.equal(ai, book.aiAttached);
    assert.deepEqual([book.depth, book.productionDirty, book.graphsEnabled, owners[1].items(), ai[1]], [0, 0, false, [], false]);
    s.gameAux.totalCost = -123; assert.equal(book.totalCost, -123); book.totalCost = 456; assert.equal(s.gameAux.totalCost, 456);
});

test('장기 Factory: 중복 World 대여 거부는 현재 공장 깊이·목록·실제 슬롯을 보존한다', () => {
    const s = scene(); const w = world(s); const target = w.create(75, 0); s.postPopState.depth = 7; const bytes = s.pool.bytes();
    assert.throws(() => world(s), /중복 대여/); assert.equal(s.postPopState.depth, 7); assert.deepEqual(s.pool.bytes(), bytes); assert.equal(s.factory.take(75, target), target); w.dispose();
});

test('장기 표시: 카메라를 각 원문 필드 위치에서 여섯 번 읽고 독립 값형 fallback도 보존한다', () => {
    const shape = { loaded: true, frameCount: 1, frames: [{ width: 10, height: 6, hotspotX: 2, hotspotY: 3 }] };
    const d = new SquidDisplay([{}], [shape], {}, view()); let reads = 0;
    const zooms = [65536, 131072, 65536, 65536, 65536, 32768];
    d.bindView(() => { const next = view(); next.cameraX = 10; next.cameraY = 20; next.zoom = zooms[reads++]; return next; });
    assert.deepEqual(d.bounds(0, 0, 5, 5), { left: 66, top: 32, right: 77, bottom: 36 }); assert.equal(reads, 6);
    d.bindView(null); assert.deepEqual(d.bounds(0, 0, 5, 5), { left: 78, top: 52, right: 89, bottom: 59 });
});

test('장기 표시: 장치 설치는 실행하지 않고 값 복사·대여 우선·반납 뒤 기본 장치를 적용한다', () => {
    const s = scene(); let calls = 0; const devices = displayRuntime(10, { view: () => { calls += 1; return view(10); } });
    s.bindDisplayDevices(devices); assert.equal(calls, 0); devices.view = forbidden;
    assert.equal(s.currentDisplayView().cameraX, 10); const r = displayRuntime(20); const use = s.bindDisplayRuntime(r);
    const v = s.currentDisplayView(); v.viewport.left = 999; assert.equal(s.currentDisplayView().viewport.left, 0); assert.equal(v.cameraX, 20);
    use.release(); use.release(); assert.equal(s.currentDisplayView().cameraX, 10); assert.equal(calls, 2);
});

test('장기 표시: 관문/동일 대여와 지난 release는 다른 현재 대여를 훼손하지 않는다', () => {
    const s = scene(); const r = displayRuntime(30);
    // 필수 함수 누락은 장치/카메라를 읽지 않고 거부한다.
    for (const bad of [null, {}, displayRuntime(0, { view: null }), displayRuntime(0, { suppressed: null }), displayRuntime(0, { invalidate: null })]) assert.throws(() => s.bindDisplayRuntime(bad), /같은 World/);
    const first = s.bindDisplayRuntime(r); assert.throws(() => s.bindDisplayRuntime(displayRuntime(40)), /같은 World/);
    const second = s.bindDisplayRuntime(r); first.release(); assert.equal(s.currentDisplayView().cameraX, 30); second.release(); assert.throws(() => s.currentDisplayView(), /view.*미연결/);
});

test('장기 표시: pin 중 반납도 선택한 현재 getter를 유지하고 다음 호출은 기본 장치를 쓴다', () => {
    const s = scene(); s.bindDisplayDevices(displayRuntime(10)); const calls = []; let use;
    const r = displayRuntime(20, { pin: () => { calls.push('pin'); use.release(); return { current: true }; }, view: () => { calls.push('view'); return view(20); } });
    use = s.bindDisplayRuntime(r); assert.equal(s.currentDisplayView().cameraX, 20); assert.equal(s.currentDisplayView().cameraX, 10); assert.deepEqual(calls, ['pin', 'view']);
});

test('장기 표시: pin 예외는 getter/초기 표시 생성보다 앞에서 중단한다', () => {
    const s = scene(); const error = new Error('표시 pin 중단'); let reads = 0;
    const r = displayRuntime(0, { pin: () => { throw error; }, view: () => { reads += 1; return view(); } }); const use = s.bindDisplayRuntime(r);
    assert.throws(() => s.display, (actual) => actual === error); assert.equal(reads, 0); use.release();
    s.bindDisplayDevices(displayRuntime(5)); assert.equal(s.display, s.display);
});

test('장기 표시: World 종료 뒤 과거 카메라를 보관하지 않고 새 Client/World view를 읽는다', () => {
    const s = scene(); const one = world(s); const d = s.display; one.setDisplay(view(20), {}); assert.equal(s.currentDisplayView().cameraX, 20); one.dispose();
    assert.throws(() => s.currentDisplayView(), /view.*미연결/); s.bindDisplayDevices(displayRuntime(35)); assert.equal(s.currentDisplayView().cameraX, 35);
    const two = world(s); assert.equal(two.core().display, d); assert.equal(s.currentDisplayView().cameraX, 0); two.setDisplay(view(45), {}); assert.equal(s.currentDisplayView().cameraX, 45); two.dispose(); assert.equal(s.currentDisplayView().cameraX, 35);
});

test('장기 표시: 구성 실패는 자기 Surface 대여만 반납하고 선행 표시 대여를 지우지 않는다', () => {
    const s = scene(); const r = displayRuntime(50); const use = s.bindDisplayRuntime(r);
    assert.throws(() => world(s), /같은 World/); assert.equal(s.borrowed, false); assert.equal(s.currentDisplayView().cameraX, 50); use.release(); const w = world(s); w.dispose();
});

test('장기 종속 분리: 다른 Pool을 거부하고 같은 현재 Session/기본 부모를 호출 위치에서 선택한다', () => {
    const s = scene(); const other = scene(); const w = world(s); const first = attached(s); const second = attached(s); let detaches = 0;
    assert.throws(() => s.bindContainedUnpop(other.containedUnpop), /같은 Pool/);
    const binding = { pool: s.pool, detach: (sid) => { detaches += 1; detach(s, sid); } }; const use = s.bindContainedUnpop(binding);
    s.factory.unpopForm(first.form); assert.equal(detaches, 0); // 현재 World Factory 대여는 World의 실제 종단을 쓴다.
    w.dispose(); s.factory.unpopForm(second.form); assert.equal(detaches, 1); use.release();
    assert.deepEqual([get(s.pool.slot(first.parent), 6, 2), get(s.pool.slot(second.parent), 6, 2)], [0, 0]);
});

test('장기 종속 분리: World는 현재 Session을 Storage와 공유하고 종료 뒤 기본 부모로 복귀한다', () => {
    const s = scene(); const w = world(s); const first = attached(s); let calls = 0;
    const binding = { pool: s.pool, contentFormType: 6, detach: (sid) => { calls += 1; detach(s, sid); }, unpop: forbidden };
    w.bindContainedUnpop(binding); s.factory.unpopForm(first.form); assert.equal(calls, 1); const second = attached(s); w.dispose();
    s.factory.unpopForm(second.form); assert.equal(calls, 1); assert.equal(get(s.pool.slot(second.parent), 6, 2), 0);
});

test('장기 Factory: 현재 HP와 shared 깊이는 새 World를 만들지 않고 같은 전역을 읽는다', () => {
    const s = scene(); const f = s.factory; const type = s.types[75]; type.maxHitPoints = 101; s.gameAux.priestType = 75; s.gameAux.priestQuarterHp = true;
    assert.equal(f.readSelectedHitPoints(type, s.types), 25); type.maxHitPoints = -1; assert.equal(f.readSelectedHitPoints(type, s.types), 0);
    const w = world(s); s.postPopState.depth = 0xffffffff; const { form } = attached(s); assert.equal(s.postPopState.depth, 0xffffffff); assert.equal(s.postPopDepth.value, 0xffffffff); w.dispose(); assert.equal(s.postPopDepth.value, 0xffffffff);
});


test('장기 표시: 메타 갱신만 현재 함수가 없으면 Client로 복귀하며 실제 raw 주소를 보존한다', () => {
    const s = scene(); const calls = []; const raw = new Uint8Array(76); put(raw, 0, 0x50c9f0);
    s.bindDisplayDevices(displayRuntime(10, { metaRefresh: (bytes) => { assert.equal(bytes, raw); calls.push('client'); } }));
    const r = displayRuntime(20, { pin: () => calls.push('pin') }); const use = s.bindDisplayRuntime(r); const d = s.display; calls.length = 0;
    d.update(raw); assert.deepEqual(calls, ['client']); r.metaRefresh = (bytes) => { assert.equal(bytes, raw); calls.push('world'); };
    d.update(raw, 0x2000); assert.deepEqual(calls, ['client', 'pin', 'world']); use.release(); d.update(raw); assert.deepEqual(calls, ['client', 'pin', 'world', 'client']);
});

test('장기 표시: changed는 현재 World만 pin하여 알리고 Client 대체 효과를 만들지 않는다', () => {
    const s = scene(); const calls = []; s.bindDisplayDevices(displayRuntime(0, { changed: forbidden })); s.noteChanged(); let use;
    const r = displayRuntime(1, { pin: () => { calls.push('pin'); use.release(); }, changed: () => calls.push('changed') }); use = s.bindDisplayRuntime(r);
    s.noteChanged(); s.noteChanged(); assert.deepEqual(calls, ['pin', 'changed']);
});

test('장기 표시: 본그림 dirty가 바꾼 현재 카메라를 다음 그림자 경계에서 다시 읽는다', () => {
    const types = Array.from({ length: 188 }, () => ({ flags1: 0, maxHitPoints: 0 })); types[70].flags1 = 0x40000;
    const frames = types.map(() => new FrameCodeTable([]));
    const header = { width: 10, height: 6, hotspotX: 2, hotspotY: 3 };
    const shapes = types.map(() => ({ loaded: true, frameCount: 1, frames: [{ ...header }, { ...header }] }));
    const s = new RawSurfaceStorage(types, frames, shapes); const calls = []; let current = view();
    const r = displayRuntime(0, { view: () => current, suppressed: () => false, invalidate: (rect, flags) => { calls.push([rect, flags]); current = view(20); } }); s.bindDisplayRuntime(r);
    const raw = new Uint8Array(50); put(raw, 0, 0x501dd0); raw[10] = 70; put(raw, 14, floatToBits(5)); put(raw, 18, floatToBits(5)); s.display.update(raw);
    assert.deepEqual(calls, [[{ left: 78, top: 52, right: 89, bottom: 59 }, 0], [{ left: 58, top: 52, right: 69, bottom: 59 }, 0]]);
});
