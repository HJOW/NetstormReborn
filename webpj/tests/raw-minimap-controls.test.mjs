// 미니맵 조립의 bitmap·입력·월드 대여 수명을 실제 dirty/raster와 픽셀 대상으로 확인한다. 게임을 실행하지 않는다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { createMetadisplayDirtyState } from '../www/core/metadisplay-dirty.js';
import { GumpDrawState, GumpPixelTarget } from '../www/render/gump-draw.js';
import { createRawMinimapGlobals, RawMinimapControls } from '../www/ui/raw-minimap-controls.js';

// 공통 Gump 허브가 아직 이식되지 않아 실제 처리기에 필요한 호출 경계만 기록한다.
function scene() {
    const state = createMetadisplayDirtyState(); const globals = createRawMinimapGlobals(); const trace = [];
    const storage = { pool: new SidPool(24000, { server: false }), hash: new SquidHash(), types: Array.from({ length: 188 }, () => ({ flags2: 0 })), ownerMode: { challenge: 0 }, spots: new Uint8Array(65536) };
    let control; let mode = 0; let registration;
    const tree = {
        token: () => 100, create: { collection: { root: 1 } }, changed: () => trace.push('changed'), requestRedrawBase: () => trace.push('redraw'),
        factory: { construct: (token, command, width, height, flags) => Object.assign(control.node(token), { command, baseWidth: width, width, baseHeight: height, height, flags, serial: 42 }) },
        tree: { attach: (token, x, y) => Object.assign(control.node(token), { x, y }), push: () => trace.push('push') },
        lifetime: { setIdle: (token, value) => trace.push(`idle:${value}`), kill: () => trace.push('kill'), cleanup: () => { control.release(100); trace.push('cleanup'); } },
        input: { capture: () => trace.push('capture') },
        collection: { position: () => ({ x: 0, y: 0 }), bounds: () => ({ left: 0, top: 0, right: 128, bottom: 128 }), screenToLocal: (token, p) => [p.x, p.y] },
    };
    const menus = { registerExternalGumps: (handlers) => { registration = handlers; return 9; }, unregisterExternalGumps: (id) => trace.push(`unregister:${id}`), setHelpText: () => {}, setHelpWord: () => {}, baseHandle: () => 7 };
    const hooks = { view: () => ({ camera: [0, 0], viewport: { left: 0, top: 0, right: 64, bottom: 44 }, pixelsX: 16, pixelsY: 11 }), center: (x, y) => trace.push(`center:${x},${y}`), move: (x, y) => trace.push(`move:${x},${y}`), inputMode: () => mode, poll: () => ({ x: -1, y: 20 }) };
    control = new RawMinimapControls(tree, menus, storage, state, globals, hooks);
    return { control, state, globals, storage, trace, registration, setMode: (value) => { mode = value; } };
}

test('미니맵 bitmap과 표시 등록은 같은 상태를 사용하고 현재 픽셀과 입력에 연결된다', () => {
    const s = scene(); s.control.open(); assert.equal(s.state.bitmap.length, 16384); assert.equal(s.globals.current, 100); assert.equal(s.globals.serial, 42); assert.equal(s.registration.owns(100), true);
    s.globals.colors.owners[2] = 9; s.state.bitmap.fill(3);
    const pixels = new Uint8Array(128 * 128); const draw = new GumpDrawState(); Object.assign(draw.clip, { left: 0, top: 0, right: 128, bottom: 128 }); Object.assign(draw.vfxClip, draw.clip);
    s.control.draw(100, new GumpPixelTarget(pixels, 128, 128, draw)); assert.equal(pixels[128 * 20 + 20], 3); assert.equal(pixels[0], 9);
    assert.equal(s.control.handle(100, 0x10000, 10, 12), 7); assert.deepEqual(s.trace.slice(0, 4), ['center:20,24', 'redraw', 'capture', 'idle:true']);
    s.control.idle(100); assert.equal(s.trace.at(-2), 'center:-2,40'); assert.equal(s.control.handle(100, 0x80010000, 0, 0), 1);
    s.setMode(2); assert.equal(s.control.handle(100, 0x10000, 3, 4), 1); assert.equal(s.trace.at(-1), 'move:6,8');
    s.control.dispose(); assert.equal(s.state.displayPresent, false); assert.equal(s.trace.at(-1), 'unregister:9');
});

test('미니맵 World는 같은 물리만 대여하고 반납하며 다른 물리는 거부한다', () => {
    const s = scene(); const trace = [];
    // 현재 대여한 처리기와 반납된 처리기의 객체 동일성을 확인하는 통지 장치다.
    const device = () => ({ pool: s.storage.pool, setMetadisplayDirty: (dirty) => trace.push(dirty), releaseMetadisplayDirty: (dirty) => trace.push(dirty) });
    const core = { ...s.storage, pop: device(), unpop: device() }; s.control.bindWorld(core); assert.equal(s.state.fullRefresh, 1); assert.equal(trace[0], trace[1]);
    assert.throws(() => s.control.bindWorld(core), TypeError); s.control.unbindWorld(); assert.equal(trace[0], trace[2]); assert.equal(trace[1], trace[3]);
    assert.throws(() => s.control.bindWorld({ ...core, spots: new Uint8Array(1) }), TypeError); s.control.dispose();
});
