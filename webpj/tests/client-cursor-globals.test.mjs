// Client 커서 장기 전역·원문 Cancel 꼬리·실제 카메라/Renderer를 cpppj ClientCursorGlobalsTests의 해당 경계와 대조한다.
// 실제 Client Acquire/AcquirePrepared와 RawCursorPreviews 생성은 각 소유자/생성기 단위로 이월한다. 게임/브라우저 실행이 아니다. (2026-11-14 WB04-E402)
import test from 'node:test';
import assert from 'node:assert/strict';
import { ClientCursorGlobals } from '../www/core/client-cursor-globals.js';
import { ClientCameraGlobals } from '../www/core/client-camera-globals.js';
import { ClientAiDebugGlobals } from '../www/core/client-ai-debug-globals.js';
import { CursorDropState } from '../www/core/raw-cursor-drop.js';
import { RawCursorLifecycle } from '../www/core/raw-cursor-lifecycle.js';
import { cancelCursorForState, cancelClientCursorForState, makeClientCursorPrepareHooks } from '../www/core/client-cursor-state-cleanup.js';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { Renderer } from '../www/render/renderer.js';

// 같은 실제 client 풀에서101번까지 확보한 최소 외부 입력이다. TYPE/가상 효과의 기대 출력은 만들지 않는다.
function scene() {
    const pool = new SidPool(24000, { server: false });
    // 원문이 읽을 미리보기 번호를 같은 풀에 둔다.
    for (let sid = 5; sid <= 101; sid += 1) assert.equal(pool.allocate(2), sid);
    return { pool, types: Array.from({ length: 188 }, () => ({ flags1: 0, flags2: 0, cost: 0, footX: 0, footY: 0 })), drop: new CursorDropState(), owner: new ClientCursorGlobals() };
}

test('ClientCursorGlobals_DwordWidthAndOwnerIdentity: 최초값·30DWORD·signed 개수·같은 상태/옵션 주소와 두 Client 독립성', () => {
    const first = new ClientCursorGlobals(); const second = new ClientCursorGlobals(); const life = first.lifecycle(); const placement = first.placement();
    const values = life.previews; assert.ok(values instanceof Uint32Array); assert.equal(values.byteLength, 120); assert.equal(life.costType, 92);
    assert.deepEqual([placement.labelSpacing, placement.acceptEvent, first.snapToGrid().value, first.rotationMode().value], [16, 0x0200003f, 0, 0]);
    values[0] = 0x12345678; values[29] = 0xffffffff; life.previewCount = -7; life.currentGump = 0x76543210; placement.description = 'same-native-state'; first.snapToGrid().value = 0x80000000;
    assert.equal(first.lifecycle(), life); assert.equal(first.placement(), placement); assert.equal(first.lifecycle().previews, values); assert.equal(first.snapToGrid(), first.snapToGrid());
    assert.deepEqual([life.previews[0], life.previews[29], life.previewCount, life.currentGump, first.snapToGrid().value], [0x12345678, 0xffffffff, -7, 0x76543210, 0x80000000]);
    assert.deepEqual([second.lifecycle().previews[0], second.snapToGrid().value, second.placement().description], [0, 0, '']);
    first.rotationMode().value = -1; assert.equal(first.rotationMode().value, 0xffffffff);
});

test('ClientCursorGlobals_GeneralFreshKeepsAddressesAndCameraBinding: reset은 배열/화면 점 주소와 현재 진단/카메라/회전 옵션을 보존한다', () => {
    const owner = new ClientCursorGlobals(); const life = owner.lifecycle(); const placement = owner.placement(); const ai = new ClientAiDebugGlobals();
    const arrays = [life.previews, life.warningContext, life.costContext, life.warningWidths, life.warningHeights, placement.bounds];
    const origin = placement.screenOrigin; const pixels = new Int32Array([-17, 31]); placement.sharedScreenOrigin = pixels; life.checking = ai.checking(); ai.checking().value = -1;
    life.previews.fill(0xffffffff); life.warningWidths.fill(-3); life.warningContext.fill(5); life.costContext.fill(7); life.warningHeights.fill(9); placement.bounds.fill(-11);
    Object.assign(life, { previewCount: 13, costType: 123, costImage: 999, costText: 'old' }); placement.description = 'old'; placement.labelSpacing = 22; origin.x = 8; origin.y = 9;
    owner.snapToGrid().value = 17; owner.rotationMode().value = 0x87654321; owner.resetForGeneralWorld();
    assert.equal(owner.lifecycle(), life); assert.equal(owner.placement(), placement);
    assert.deepEqual([life.previews, life.warningContext, life.costContext, life.warningWidths, life.warningHeights, placement.bounds], arrays);
    // 각 배열은 값뿐 아니라 기존 주소도 같아야 한다.
    [life.previews, life.warningContext, life.costContext, life.warningWidths, life.warningHeights, placement.bounds].forEach((value, index) => assert.equal(value, arrays[index]));
    assert.ok(life.previews.every((value) => value === 0)); assert.equal(life.previewCount, 0); assert.equal(life.costType, 92); assert.equal(life.costImage, 0); assert.equal(life.costText, '');
    assert.deepEqual(Array.from(life.warningContext), [2, 0, 0, 0, 0, 0, 1, 1]); assert.deepEqual(Array.from(life.costContext), [2, 3, 0, 0, 0, 0, 1, 1]);
    assert.ok(life.warningWidths.every((value) => value === 0)); assert.ok(life.warningHeights.every((value) => value === 0)); assert.ok(placement.bounds.every((value) => value === 0));
    assert.equal(placement.screenOrigin, origin); assert.deepEqual(origin, { x: 0, y: 0 }); assert.equal(placement.sharedScreenOrigin, pixels); assert.equal(life.checking, ai.checking()); assert.equal(life.checkingValue(), 0xffffffff);
    assert.deepEqual([placement.description, placement.labelSpacing, placement.acceptEvent, owner.snapToGrid().value, owner.rotationMode().value], ['', 16, 0x0200003f, 0, 0x87654321]);
    pixels.set([7, -9]); ai.checking().value = 0; assert.deepEqual([placement.screenOriginX(), placement.screenOriginY(), life.checkingValue()], [7, -9, 0]);
    life.previewCount = 7; owner.snapToGrid().value = 19; owner.resetForGeneralWorld();
    assert.equal(life.previews, arrays[0]); assert.equal(placement.sharedScreenOrigin, pixels); assert.equal(life.checking, ai.checking());
    assert.deepEqual([life.previewCount, owner.snapToGrid().value, owner.rotationMode().value], [0, 0, 0x87654321]);
});

test('ClientCursorGlobals_NativeCancelRequeriesCountAndProcessThenClearsAllDwords: 삭제 효과 뒤 현재 count/번호/Process를 다시 읽는다', () => {
    const { pool, types, drop, owner } = scene(); drop.lockDepth = 1; drop.cursorGump = 77; drop.legalGump = 19; owner.lifecycle().currentGump = 17; owner.lifecycle().previews[29] = 0x87654321;
    const order = []; const hooks = {
        prepareCursor: () => { order.push(1); owner.lifecycle().previewCount = 1; owner.lifecycle().previews[0] = 100; },
        destroy: (sid, flags) => { assert.equal(flags, 0); order.push(sid); if (sid === 100) { owner.lifecycle().previewCount = 2; owner.lifecycle().previews[1] = 101; drop.cursorGump = 99; } else owner.lifecycle().previewCount = 1; },
        removeProcess: (process, flags) => { assert.deepEqual([process, flags], [99, 0]); order.push(process); },
    };
    new RawCursorLifecycle(pool, types, drop, owner.lifecycle(), hooks).cancel();
    assert.deepEqual(order, [1, 100, 101, 99]); assert.deepEqual([drop.lockDepth, drop.preview, drop.cursorGump, drop.legalGump, owner.lifecycle().currentGump, owner.lifecycle().previewCount], [1, 0, 0, 19, 17, 0]);
    assert.ok(owner.lifecycle().previews.every((value) => value === 0));
});

test('ClientCursorGlobals_HighWordDoesNotAliasValidPreview·CancelRejectsAfterOriginalPrefix: 준비/강조/잠금 prefix 뒤 큰 DWORD를 보존해 거부한다', () => {
    const { pool, types, drop, owner } = scene(); const trace = []; const hooks = { prepareCursor: () => trace.push(2), destroy: () => trace.push(9), highlight: () => trace.push(1), selectCursor: () => trace.push(3) };
    owner.lifecycle().previews[0] = 0x10064; owner.lifecycle().previewCount = 1;
    const body = new RawCursorLifecycle(pool, types, drop, owner.lifecycle(), hooks); assert.throws(() => body.activate(1), /호스트SID범위/);
    assert.deepEqual(trace, [2]); assert.deepEqual([owner.lifecycle().previewCount, owner.lifecycle().previews[0]], [1, 0x10064]);
    owner.lifecycle().previewCount = 0; trace.length = 0; drop.preview = 0x10032;
    assert.throws(() => body.cancel(), /호스트SID범위/); assert.deepEqual(trace, [1, 2, 3]); assert.deepEqual([drop.preview, drop.lockDepth], [0x10032, 1]);
});

test('ClientCursorGlobals_FirstMenuCancelUsesActualProjectAndCurrentRenderer: 현재 카메라와 두 실제 Renderer의 dirty·목록/전체 DWORD 정리를 연결한다', () => {
    const { pool, types, drop, owner } = scene(); const camera = new ClientCameraGlobals(); camera.initializeNativeRenderer(1024, 768, 256, { value: 0 }, { value: 0 }); camera.snapshot().setWorld(5, 5);
    Object.assign(drop, { lockDepth: 1, preview: 0xffffffff, x: 10, y: 10, screenX: 777, screenY: 888, legalGump: 0x12345678 });
    owner.lifecycle().currentGump = 0x76543210; owner.lifecycle().previews[29] = 0xffffffff; owner.placement().hotspotX = 12; owner.placement().hotspotY = -7;
    const first = new Renderer(128, 128); const second = new Renderer(128, 128); const pixels = new Uint8Array(128 * 128); let current = first; let deliveries = 0;
    // 최초 전체 갱신을 비워 원문 첫 메뉴 Cancel이 요청한 부분 변경만 본다.
    for (const renderer of [first, second]) { renderer.draw(pixels, 128); renderer.present(() => {}); }
    const gameplay = { camera: () => camera, cursorGlobals: () => owner, cursor: drop };
    const client = { gameplayGlobals: () => gameplay, getRenderer: () => current }; const surface = { pool, types, hash: new SquidHash() };
    const provider = () => { deliveries += 1; return current; };
    cancelClientCursorForState(client, surface, {}, provider); const sharedPixels = owner.placement().sharedScreenOrigin;
    assert.equal(deliveries, 2); assert.deepEqual(first.draw(pixels, 128), [{ left: 78, top: 53, right: 86, bottom: 69 }]); first.present(() => {}); assert.deepEqual(second.draw(pixels, 128), []);
    assert.deepEqual([drop.lockDepth, drop.preview, drop.cursorGump, owner.lifecycle().previewCount], [1, 0, 0, 0]);
    current = second; camera.snapshot().setWorld(6, 6); cancelClientCursorForState(client, surface, {}, provider);
    assert.equal(deliveries, 4); assert.equal(owner.placement().sharedScreenOrigin, sharedPixels); assert.deepEqual(first.draw(pixels, 128), []); assert.deepEqual(second.draw(pixels, 128), [{ left: 62, top: 42, right: 70, bottom: 58 }]); second.present(() => {});
    assert.deepEqual([drop.screenX, drop.screenY, drop.legalGump, owner.lifecycle().currentGump, owner.placement().hotspotX, owner.placement().hotspotY], [777, 888, 0x12345678, 0x76543210, 12, -7]);
    assert.ok(owner.lifecycle().previews.every((value) => value === 0)); assert.deepEqual(Array.from(sharedPixels), [96, 66]);
    // 기본 provider도 이전 Renderer를 캡처하지 않는다.
    const prepare = makeClientCursorPrepareHooks(client); assert.deepEqual(prepare.project(10, 10), { x: 64, y: 44 });
    current = first; prepare.invalidate({ left: 1, top: 2, right: 3, bottom: 4 }, 2); assert.equal(first.cameraDirtyEntries().length, 1); assert.equal(second.cameraDirtyEntries().length, 0);
});

test('cancelCursorForState: 호출자 effects의 prepare는 보존하고 실제 원문 Prepare를 선택하며 누락된 Project를 성공으로 바꾸지 않는다', () => {
    const { pool, types, drop, owner } = scene(); drop.lockDepth = 1; const oldPrepare = () => assert.fail('호출자 game.prepare는 쓰지 않는다'); const effects = { prepareCursor: oldPrepare }; const rectangles = [];
    cancelCursorForState(pool, types, drop, owner, effects, { project: () => ({ x: 7, y: 9 }), invalidate: (rect, flags) => rectangles.push([rect, flags]) });
    assert.equal(effects.prepareCursor, oldPrepare); assert.equal(rectangles.length, 2); assert.deepEqual(rectangles[0], [{ left: 5, top: 7, right: 13, bottom: 23 }, 2]);
    assert.throws(() => cancelCursorForState(pool, types, drop, owner, {}, {}), /의존 동작 누락/);
});
