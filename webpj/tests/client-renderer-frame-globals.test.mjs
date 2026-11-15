// cpppj RendererCloud_FrameGlobalsShareActualPreludeQueue의 실제 구름 큐 공유와 Client 수명을 검사한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { ClientRendererFrameGlobals } from '../www/core/client-renderer-frame-globals.js';
import { appendRendererCloudDirty } from '../www/render/raw-renderer-clouds.js';
import { prepareRendererFrame } from '../www/render/raw-renderer-prelude.js';
import { markKencloudDirty } from '../www/core/raw-kencloud-render.js';
import { Renderer } from '../www/render/renderer.js';

// 원문 참조 DWORD를 테스트 장치에서 빌릴 수 있도록 만든다.
const box = (value) => ({ value });

test('Renderer 프레임 전역은 앞단과 구름 네 칸을 공유하고 다른 Client와 독립적이다', () => {
    const first = new ClientRendererFrameGlobals();
    const second = new ClientRendererFrameGlobals();
    assert.deepEqual([...first.scan()], Array(19).fill(0));
    assert.equal(first.cloudDirtyCount().value, 0);
    assert.equal(first.hideGroups().value, 0);
    assert.notEqual(first.scan(), second.scan());
    assert.notEqual(first.cloudDirtyCount(), second.cloudDirtyCount());
    first.scan()[0] = 3;
    appendRendererCloudDirty(first.cloudQueue(), { left: 1, top: 2, right: 3, bottom: 4 }, 0);
    assert.equal(first.cloudDirtyCount().value, 1);
    assert.equal(second.cloudDirtyCount().value, 0);
    const cloud = new Uint32Array(2054);
    const background = box(7); const dirtyCount = box(9); const added = [];
    const globals = Object.fromEntries(Object.entries({ mode: 139, force: 1, width: 1024, height: 768, initialHeight: 768, cameraY: 0, left: 0, top: 0, right: 1024, bottom: 768 }).map(([key, value]) => [key, box(value)]));
    prepareRendererFrame({ scan: first.scan(), backgroundRedraw: background, dirtyCount, drawCounter: first.cloudDirtyCount() }, globals, {
        markCloudDirty: () => markKencloudDirty(cloud),
        queueCloudDirty: (rect) => appendRendererCloudDirty(first.cloudQueue(), rect, 0),
        overrideActive: () => 0,
        invalidate: (rect, flags) => { assert.equal(flags, 0); added.push(rect); },
    });
    assert.equal(first.cloudDirtyCount().value, 0);
    assert.equal(dirtyCount.value, 0);
    assert.equal(first.scan()[0], 3);
    assert.equal(first.scan()[18], 139);
    assert.equal(background.value, 7);
    assert.equal(first.cloudRectangles()[0].left, 1);
    assert.equal(first.cloudRectangles()[1].right, 1024);
    assert.equal(first.cloudRectangles()[2].bottom, 768);
    assert.equal(added.length, 1);
    assert.equal(cloud[2051], 1);
    assert.equal(cloud[2052], 0);
    assert.equal(second.cloudRectangles()[1].right, 0);
});

test('Renderer 프레임 전역의 대여 참조와 DWORD 저장 폭은 유지된다', () => {
    const globals = new ClientRendererFrameGlobals();
    const queue = globals.cloudQueue();
    assert.equal(queue, globals.cloudQueue());
    assert.equal(queue.rectangles, globals.cloudRectangles());
    assert.equal(queue.count, globals.cloudDirtyCount());
    globals.hideGroups().value = -1;
    assert.equal(globals.hideGroups().value, 0xffffffff);
    queue.count.value = 0x100000001;
    assert.equal(globals.cloudDirtyCount().value, 1);
});

test('구름 칸의 대여 참조는 기록 뒤에도 유지되고 실제 Renderer의 변경 영역을 갱신한다', () => {
    const owner = new ClientRendererFrameGlobals();
    const slot = owner.cloudRectangles()[0];
    const renderer = new Renderer(1024, 768);
    const pixels = new Uint8Array(1024 * 768);
    renderer.draw(pixels, 1024);
    renderer.present(() => {});
    renderer.invalidate({ left: 1, top: 2, right: 3, bottom: 4 });
    const dirtyCount = box(renderer.cameraDirtyEntries().length);
    const values = { mode: 138, force: 1, width: 1024, height: 768, initialHeight: 800, cameraY: 0, left: 84, top: 0, right: 1024, bottom: 768 };
    const globals = Object.fromEntries(Object.entries(values).map(([key, value]) => [key, box(value)]));
    const clouds = new Uint32Array(2054);
    prepareRendererFrame({ scan: owner.scan(), backgroundRedraw: renderer.backgroundRedraw, dirtyCount, drawCounter: owner.cloudDirtyCount() }, globals, {
        markCloudDirty: () => markKencloudDirty(clouds),
        queueCloudDirty: (rect) => appendRendererCloudDirty(owner.cloudQueue(), rect, 0),
        overrideActive: () => 0,
        clearDirty: () => renderer.clearDirty(),
        invalidate: (rect, flags) => { assert.equal(dirtyCount.value, 0); renderer.invalidate(rect, flags); },
    });
    assert.equal(owner.cloudRectangles()[0], slot);
    assert.deepEqual(slot, { left: 0, top: 0, right: 1024, bottom: 768 });
    assert.equal(owner.cloudDirtyCount().value, 0);
    assert.equal(owner.cloudRectangles()[1].bottom, 768);
    assert.equal(renderer.fullRedrawPending(), false);
    assert.deepEqual(renderer.cameraDirtyEntries(), [{ rect: { left: 0, top: 0, right: 1024, bottom: 768 }, flags: 0 }]);
});
