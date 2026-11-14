// TYPE 이미지 Gump(ImageGump)의 생성/프레임/그리기 색표·그림자 순서/적중 요청과 실제 자산·표면 연결을 기준선 소스 계약과 손 계산 값으로 검증한다. (2026-11-14 WB04-E441)
// cpppj에도 직접 시험이 없어 원본 직접 관측 수치가 아니다. 순서/경계는 기준선 소스(ImageGump.cpp)에서 옮겼다. 핵심 본체는 기록 장치, 자산/픽셀 연결은 실제 10.78 자료(TypeTable·SHP)와 실제 GumpPixelTarget 이다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { GumpCreate, GumpCreateState } from '../www/ui/gump-create.js';
import { ImageGump, ImageGumpNode, ImageGumpPixelTarget, bindImageGumpAssets } from '../www/ui/image-gump.js';
import { GumpPixelTarget, GumpDrawState } from '../www/render/gump-draw.js';
import { GameAssets } from '../www/assets/game-assets.js';
import { realCaptureAssets } from './capture-assets-support.mjs';

// 실제 GumpCreate 위의 틀이다. TYPE 기준점 (3,4), 프레임 헤더 20×10·기준점 (8,6), 위치 (10,20), 그림자 오프셋 (2,3).
function rig({ owner = 0, maps = new Array(200).fill(0), scale = 1 } = {}) {
    const log = []; const nodes = new Map(); const state = new GumpCreateState(); const factory = new GumpCreate(state, { node: (token) => nodes.get(token) ?? null });
    const node = new ImageGumpNode(); nodes.set(1, node);
    const current = { localOwner: owner, ownerMaps: maps, shadowOffset: { x: 2, y: 3 } };
    const hooks = {
        node: (token) => nodes.get(token) ?? null, type: (type) => ({ shape: type - 70 + 1, hotspotX: 3, hotspotY: 4 }), header: () => ({ width: 20, height: 10, hotspotX: 8, hotspotY: 6 }),
        position: () => ({ x: 10, y: 20 }), color: (address) => log.push(['color', address]), draw: (request) => log.push(['draw', request.type, request.shape, request.frame, request.position.x, request.position.y, request.scale, request.scaled, request.mapped]),
        hit: (request) => { log.push(['hit', request.type, request.shape, request.frame, request.point.x, request.point.y, request.origin.x, request.origin.y, request.scale]); return 7; }, attach: (...args) => log.push(['attach', ...args]),
    };
    const gump = new ImageGump(factory, current, hooks); gump.construct(1, 72, 4, 0); node.scale = scale; log.length = 0; return { gump, node, nodes, current, hooks, log, factory, state };
}

test('ImageGumpNode: 새 노드의 TYPE/프레임/모드/기준점은 0 이다', () => {
    const node = new ImageGumpNode(); assert.deepEqual([node.type, node.frame, node.mode, node.hotspotX, node.hotspotY], [0, 0, 0, 0, 0]);
});

test('construct/setFrame: 공통 기본 생성(크기 0·등록) 뒤 가상 표/종류 3, 헤더 폭·높이를 float32 기본 크기로, 표시 크기는 현재 배율을 곱해, 기준점은 (헤더 − TYPE) float32 로 쓴다', () => {
    const r = rig(); assert.deepEqual([r.node.vtable, r.node.command, r.node.type, r.node.frame, r.node.mode], [0x501bb0, 3, 72, 4, 0]); assert.deepEqual([r.node.baseWidth, r.node.width, r.node.baseHeight, r.node.height, r.node.hotspotX, r.node.hotspotY], [20, 20, 10, 10, 5, 2]); assert.equal(r.state.entries[0], 1); assert.equal(r.node.flags, 9);
    r.node.scale = Math.fround(0.3); r.gump.setFrame(1, 80, 9, 6); assert.deepEqual([r.node.type, r.node.frame, r.node.mode], [80, 9, 6]); assert.equal(r.node.width, Math.fround(20 * Math.fround(0.3))); assert.equal(r.node.height, Math.fround(10 * Math.fround(0.3)));
    // 기준점은 정수 차이를 마지막에 float32 로 저장한다(큰 값이라도 정수 연산).
    r.hooks.header = () => ({ width: 1, height: 1, hotspotX: -1000, hotspotY: 32767 }); r.hooks.type = () => ({ shape: 1, hotspotX: 2147483, hotspotY: -5 }); new ImageGump(r.factory, r.current, r.hooks).setFrame(1, 70, 0, 0);
    assert.equal(r.node.hotspotX, Math.fround(-1000 - 2147483)); assert.equal(r.node.hotspotY, 32772);
    assert.throws(() => r.gump.construct(9, 72, 0, 0), { name: 'RangeError', message: /저장소 없음/ }); assert.throws(() => r.gump.construct(0, 72, 0, 0), RangeError);
});

test('draw 단위 배율: 색표 없이 위치+기준점으로 그리고 owner 색표가 있으면(모드 8 이면 단위에서는 생략) 현재 owner 열 색표로 그린다', () => {
    const r = rig(); r.gump.draw(1); assert.deepEqual(r.log, [['draw', 72, 3, 4, 15, 22, 65536, false, false]]);
    // owner 2, TYPE 72 의 색표 토큰 0x1000 → 0x1000+(2<<8)=0x1200 의 mapped 출력. 모드 8 이면 단위 배율에서는 색표 없이 그린다.
    const o = rig({ owner: 2, maps: new Array(80).fill(0).map((_, index) => (index === 72 ? 0x1000 : 0)) }); o.gump.draw(1); assert.deepEqual(o.log, [['color', 0x1200], ['draw', 72, 3, 4, 15, 22, 65536, false, true]]);
    o.log.length = 0; o.node.mode = 8; o.gump.draw(1); assert.deepEqual(o.log, [['draw', 72, 3, 4, 15, 22, 65536, false, false]]);
    // owner 가 0 이하(부호 있는 비교)이거나 표 토큰이 0 이면 색표 없음.
    o.node.mode = 0; o.log.length = 0; o.current.localOwner = 0x80000000; o.gump.draw(1); assert.deepEqual(o.log, [['draw', 72, 3, 4, 15, 22, 65536, false, false]]); o.current.localOwner = 3; o.current.ownerMaps[72] = 0; o.log.length = 0; o.gump.draw(1); assert.deepEqual(o.log, [['draw', 72, 3, 4, 15, 22, 65536, false, false]]);
});

test('draw owner 경계: owner 0 은 색표가 있어도 쓰지 않고, TYPE 번호가 배열 길이와 같으면 오류이며(길이-1 은 통과), 모드 9 의 비트 1 은 선택 색표다', () => {
    const maps = new Array(200).fill(0x3000); const zero = rig({ owner: 0, maps }); zero.gump.draw(1); assert.deepEqual(zero.log, [['draw', 72, 3, 4, 15, 22, 65536, false, false]]);
    const equal = rig({ owner: 1, maps: new Array(72).fill(1) }); assert.throws(() => equal.gump.draw(1), { name: 'RangeError', message: /현재TYPE 색표 범위/ });
    const last = rig({ owner: 1, maps: new Array(73).fill(0x3000) }); last.gump.draw(1); assert.equal(last.log[0][0], 'color');
    const nine = rig({ owner: 1, maps }); nine.node.mode = 9; nine.gump.draw(1); assert.deepEqual(nine.log, [['color', 0x556438], ['draw', 72, 3, 4, 15, 22, 65536, false, true]]);
});

test('draw 모드 비트: 1 선택/2 비활성 색표로 mapped 그리고 끝내며, 4 는 그림자 색표·그림자 오프셋(단위면 y-1)으로 먼저 그린 뒤 본체를 이어서 그린다', () => {
    const r = rig(); r.node.mode = 1; r.gump.draw(1); assert.deepEqual(r.log, [['color', 0x556438], ['draw', 72, 3, 4, 15, 22, 65536, false, true]]);
    for (const [mode, address] of [[2, 0x556038], [3, 0x556438], [10, 0x556038], [11, 0x556438]]) { r.log.length = 0; r.node.mode = mode; r.gump.draw(1); assert.deepEqual(r.log, [['color', address], ['draw', 72, 3, 4, 15, 22, 65536, false, true]], String(mode)); }
    // 그림자(4): (10+2+5, 20+3+2-1) = (17,24). 그 뒤 모드 비트가 없으면 owner 경로(색표 없음).
    r.log.length = 0; r.node.mode = 4; r.gump.draw(1); assert.deepEqual(r.log, [['color', 0x556338], ['draw', 72, 3, 4, 17, 24, 65536, false, true], ['draw', 72, 3, 4, 15, 22, 65536, false, false]]);
    r.log.length = 0; r.node.mode = 5; r.gump.draw(1); assert.deepEqual(r.log, [['color', 0x556338], ['draw', 72, 3, 4, 17, 24, 65536, false, true], ['color', 0x556438], ['draw', 72, 3, 4, 15, 22, 65536, false, true]]);
});

test('draw 비단위 배율: 16.16 배율(ftol)·배율을 곱한 기준점(절삭)·scaled 이며 그림자는 y-1 이 없고, 단위에 가까운 float32 도 단위가 아니다', () => {
    const r = rig({ scale: 0.5 }); r.gump.draw(1); assert.deepEqual(r.log, [['draw', 72, 3, 4, 12, 21, 32768, true, false]]);
    r.log.length = 0; r.node.mode = 4; r.gump.draw(1); assert.deepEqual(r.log, [['color', 0x556338], ['draw', 72, 3, 4, 14, 24, 32768, true, true], ['draw', 72, 3, 4, 12, 21, 32768, true, false]]);
    // 모드 8 은 비단위에서는 owner 색표를 막지 않는다.
    const o = rig({ scale: 0.5, owner: 1, maps: new Array(80).fill(0x2000) }); o.node.mode = 8; o.gump.draw(1); assert.deepEqual(o.log, [['color', 0x2100], ['draw', 72, 3, 4, 12, 21, 32768, true, true]]);
    const near = rig({ scale: Math.fround(1.0000001) }); near.gump.draw(1); assert.equal(near.log[0][7], true); assert.equal(near.log[0][6], 65536);
    // TYPE 번호가 owner 색표 배열 밖이면(그 분기에 도달할 때만) RangeError 다.
    const short = rig({ owner: 1, maps: [] }); assert.throws(() => short.gump.draw(1), { name: 'RangeError', message: /현재TYPE 색표 범위/ }); short.node.mode = 1; short.gump.draw(1);
});

test('hit/attach: 검사점과 기준점은 ftol 로, 배율은 ftol(배율×65536)로 실제 판정에 넘기고 결과를 그대로 돌려주며 부착은 기본 부착을 그대로 호출한다', () => {
    const r = rig({ scale: 0.5 }); r.node.x = 4; r.node.y = 6; assert.equal(r.gump.hit(1, 3.9, -2.9), 7);
    // 기준점 = ftol(5×0.5+4)=6, ftol(2×0.5+6)=7, 검사점 (3,-2), 배율 32768.
    assert.deepEqual(r.log, [['hit', 72, 3, 4, 3, -2, 6, 7, 32768]]);
    r.log.length = 0; r.gump.attach(1, 5, 6, 7, 8); assert.deepEqual(r.log, [['attach', 1, 5, 6, 7, 8]]);
    // 도달한 장치만 요구한다: 장치 하나를 없애고 그 장치에 닿는 호출은 Error, 닿지 않는 호출은 통과한다.
    const calls = { type: (gump) => gump.setFrame(1, 72, 0, 0), header: (gump) => gump.setFrame(1, 72, 0, 0), position: (gump) => gump.draw(1), draw: (gump) => gump.draw(1), hit: (gump) => gump.hit(1, 0, 0), attach: (gump) => gump.attach(1, 0, 0, 0, 0) };
    for (const [name, call] of Object.entries(calls)) { const missing = rig(); assert.throws(() => call(new ImageGump(missing.factory, missing.current, { ...missing.hooks, [name]: undefined })), { name: 'Error', message: /실제 하위 미연결/ }, name); }
    const colorless = rig(); colorless.node.mode = 1; assert.throws(() => new ImageGump(colorless.factory, colorless.current, { ...colorless.hooks, color: undefined }).draw(1), /실제 하위 미연결/);
    const plain = rig(); new ImageGump(plain.factory, plain.current, { ...plain.hooks, color: undefined, hit: undefined, attach: undefined }).draw(1);
});

// 실제 자료 위의 연결이다.
const { table, database } = realCaptureAssets(); const assets = new GameAssets(table, database);
const manType = assets.typeNumber(assets.find('man')); const bridgeType = assets.typeNumber(assets.find('bridge'));

test('bindImageGumpAssets: 실제 TYPE 번호(70부터)를 SHP 블록(번호-70)에 연결하고 TYPE 기준점/실제 Squid 헤더를 읽으며 범위 밖은 RangeError 다', () => {
    const hooks = bindImageGumpAssets({}, assets); const view = table.view(manType);
    assert.deepEqual(hooks.type(manType), { shape: manType - 70 + 1, hotspotX: view.hotspotX, hotspotY: view.hotspotY });
    const block = manType - 70; const metrics = database.squidMetrics(block, 0); assert.deepEqual(hooks.header(manType, 0), { width: metrics.width, height: metrics.height, hotspotX: metrics.hotspotX, hotspotY: metrics.hotspotY });
    assert.throws(() => hooks.type(69), { name: 'RangeError', message: /실제TYPE 자산 범위/ }); assert.throws(() => hooks.type(188), RangeError); assert.doesNotThrow(() => hooks.type(187));
    assert.throws(() => hooks.header(69, 0), { name: 'RangeError', message: /실제SHP 타입 범위/ }); assert.throws(() => hooks.header(186, 0), RangeError);
    // 프레임 수는 클러스터 수(그림자 플래그가 있으면 2배) 안만 허용한다.
    const clusters = assets.types()[block].definition.clusters.length; assert.throws(() => hooks.header(manType, clusters * 2), { name: 'RangeError', message: /frameCheck 범위/ }); assert.throws(() => hooks.header(manType, 0xffffffff), RangeError);
    // 그림자 플래그(0x440000)가 있는 타입(dude, 65 클러스터)은 2배(130)까지 허용하고, 없는 타입(banner, 22 클러스터 + 다른 플래그 비트)은 클러스터 수까지만 허용한다.
    assert.doesNotThrow(() => hooks.header(manType, clusters)); assert.doesNotThrow(() => hooks.header(manType, clusters * 2 - 1));
    const banner = assets.typeNumber(assets.find('banner')); const bannerClusters = assets.types()[banner - 70].definition.clusters.length; assert.doesNotThrow(() => hooks.header(banner, bannerClusters - 1)); assert.throws(() => hooks.header(banner, bannerClusters), { name: 'RangeError', message: /frameCheck 범위/ });
});

test('ImageGumpPixelTarget: 색표 토큰을 공급자로 해석해 공유 색표에 복사하고, 일반/배율 VFX 를 실제 표면에 그리며 특수 프레임은 그리지 않고 범위 밖 TYPE/프레임은 거부한다', () => {
    const pitch = 200; const surface = new Uint8Array(pitch * 200).fill(0xaa); const drawState = new GumpDrawState(); Object.assign(drawState.vfxClip, { left: 0, top: 0, right: 199, bottom: 199 });
    const target = new GumpPixelTarget(surface, pitch, 200, drawState); const current = new Uint8Array(256); const maps = new Map([[0x1234, Uint8Array.from({ length: 256 }, (_, index) => 255 - index)]]);
    const pixels = new ImageGumpPixelTarget(target, assets, current, (token) => { const found = maps.get(token); if (!found) throw new RangeError('색표 없음'); return found; }); const hooks = pixels.hooks({ attach: () => 'keep', node: () => null });
    assert.equal(hooks.attach(), 'keep'); hooks.color(0x1234); assert.equal(current[10], 245); assert.throws(() => hooks.color(0x9999), RangeError);
    const frame = database.blocks[manType - 70].frames[0]; const decoded = database.decode(manType - 70, 0); const opaque = decoded.opacity.findIndex((value) => value === 255); assert.ok(opaque >= 0);
    // 단위 배율: 기준점에 프레임 rect 를 더한 위치에 그림이 그려진다.
    hooks.draw({ type: manType, shape: 1, frame: 0, position: { x: 100, y: 100 }, scale: 65536, scaled: false, mapped: false });
    const column = opaque % decoded.width; const row = Math.floor(opaque / decoded.width); const at = (100 + frame.rect.left + column) + (100 + frame.rect.top + row) * pitch; assert.notEqual(surface[at], 0xaa); assert.equal(surface[at], decoded.indices[opaque]);
    // 확대/축소 출력도 표면을 바꾼다. 범위 밖 TYPE/프레임은 거부한다.
    surface.fill(0xaa); hooks.draw({ type: manType, shape: 1, frame: 0, position: { x: 100, y: 100 }, scale: 32768, scaled: true, mapped: false }); assert.equal(surface.some((value) => value !== 0xaa), true);
    assert.throws(() => hooks.draw({ type: 186, frame: 0, position: { x: 0, y: 0 }, scale: 65536, scaled: false, mapped: false }), { name: 'RangeError', message: /픽셀TYPE 오류/ });
    assert.throws(() => hooks.draw({ type: 69, frame: 0, position: { x: 0, y: 0 }, scale: 65536, scaled: false, mapped: false }), { name: 'RangeError', message: /픽셀TYPE 오류/ }); assert.throws(() => hooks.draw({ type: manType, frame: 99999, position: { x: 0, y: 0 }, scale: 65536, scaled: false, mapped: false }), { name: 'RangeError', message: /프레임 범위/ });
});

test('ImageGumpPixelTarget 적중: 불투명 픽셀 위의 검사점은 1, 투명/바깥은 0 이고 기준점이 0..199 밖이면 검사점을 그만큼 옮기며 보정 뒤 버퍼 밖이면 RangeError 다', () => {
    const pitch = 200; const target = new GumpPixelTarget(new Uint8Array(pitch * 200), pitch, 200, new GumpDrawState()); const pixels = new ImageGumpPixelTarget(target, assets, new Uint8Array(256), () => new Uint8Array(256)); const hooks = pixels.hooks({});
    const request = (type, frameIndex, x, y, ox, oy) => ({ type, shape: 1, frame: frameIndex, point: { x, y }, origin: { x: ox, y: oy }, scale: 65536 });
    const shiftOf = (origin) => (origin < 0 ? -origin : origin > 199 ? 199 - origin : 0); const clampOf = (origin) => Math.min(Math.max(origin, 0), 199);
    // 타입/프레임의 불투명 픽셀 탐침이다: 런(행)/열(세로)의 시작·끝 픽셀 중 clamp 한 기준점(cx, cy)에서 버퍼 안(경계 여유 포함)인 첫 픽셀을 찾는다.
    const probes = (type, frameIndex) => {
        const frame = database.blocks[type - 70].frames[frameIndex]; const decoded = database.decode(type - 70, frameIndex);
        const opaqueAt = (column, row) => column >= 0 && column < decoded.width && row >= 0 && row < decoded.height && decoded.opacity[row * decoded.width + column] === 255;
        const find = (cx, cy, edge) => {
            for (let row = 0; row < decoded.height; row += 1) for (let column = 0; column < decoded.width; column += 1) {
                const x = cx + frame.rect.left + column; const y = cy + frame.rect.top + row; if (!opaqueAt(column, row) || x < 0 || x > 198 || y < 0 || y > 198) continue;
                const neighbour = { runStart: opaqueAt(column - 1, row), runEnd: opaqueAt(column + 1, row), columnStart: opaqueAt(column, row - 1), columnEnd: opaqueAt(column, row + 1) }[edge];
                if (!neighbour) return { x, y };
            }
            return null;
        };
        return { find, any: (cx, cy) => find(cx, cy, 'runEnd') };
    };
    // 그림이 기준점의 왼쪽/위에 놓이는 타입(dude 프레임 0)과 오른쪽/아래에 놓이는 타입(fortGump 프레임 4).
    const man = probes(manType, 0); const wide = probes(92, 4);
    const inside = man.any(100, 100); assert.ok(inside); assert.equal(hooks.hit(request(manType, 0, inside.x, inside.y, 100, 100)), 1); assert.equal(hooks.hit(request(manType, 0, 0, 0, 100, 100)), 0); assert.equal(hooks.hit(request(manType, 0, 199, 199, 100, 100)), 0);
    // 기준점이 199 를 넘으면 그림은 199 에 놓이고(clamp) 검사점은 (199-기준점)만큼 옮겨진다. 이동량은 이웃이 투명인 픽셀로 정확히 확인한다(경계 199/200 포함).
    for (const edge of ['runStart', 'runEnd', 'columnStart', 'columnEnd']) {
        for (const ox of [199, 200, 250]) { const p = man.find(199, 100, edge); assert.ok(p, `high x ${ox} ${edge}`); assert.equal(hooks.hit(request(manType, 0, p.x - shiftOf(ox), p.y, ox, 100)), 1, `high x ${ox} ${edge}`); }
        for (const oy of [199, 200, 250]) { const p = man.find(100, 199, edge); assert.ok(p, `high y ${oy} ${edge}`); assert.equal(hooks.hit(request(manType, 0, p.x, p.y - shiftOf(oy), 100, oy)), 1, `high y ${oy} ${edge}`); }
        // 기준점이 0 이하이면 그림은 0 에 놓이고 검사점은 +|기준점| 만큼 옮겨진다(짝수/홀수 음수, 0, -1 포함).
        for (const ox of [-6, -5, -4, -1, 0]) { const p = wide.find(0, 100, edge); assert.ok(p, `low x ${ox} ${edge}`); assert.equal(hooks.hit(request(92, 4, p.x - shiftOf(ox), p.y, ox, 100)), 1, `low x ${ox} ${edge}`); }
        for (const oy of [-8, -7, -6, -1, 0]) { const p = wide.find(100, 0, edge); assert.ok(p, `low y ${oy} ${edge}`); assert.equal(hooks.hit(request(92, 4, p.x, p.y - shiftOf(oy), 100, oy)), 1, `low y ${oy} ${edge}`); }
    }
    // 보정한 검사점이 버퍼 밖이면 RangeError 다.
    assert.throws(() => hooks.hit(request(manType, 0, -1, 0, 100, 100)), { name: 'RangeError', message: /적중 버퍼 밖/ }); assert.throws(() => hooks.hit(request(manType, 0, 0, 200, 100, 100)), RangeError); assert.throws(() => hooks.hit(request(manType, 0, 0, 0, 100, 400)), RangeError);
    assert.throws(() => hooks.hit({ ...request(manType, 0, 0, 0, 100, 100), type: 69 }), { name: 'RangeError', message: /픽셀TYPE 오류/ }); assert.equal(bridgeType > 70, true);
});
