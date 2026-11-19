// 실제 TYPE·SHP·Canon 측정과 생산 항목 자산 경계를 같은 World에서 검증한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { CampaignCursorScene } from './campaign-cursor-support.mjs';
import { RawCanonPixelShape } from '../www/core/raw-canon-pixel-shape.js';
import { bindInventoryItemAssets } from '../www/ui/inventory-item-gump.js';
import { decodeCanonType } from '../www/core/raw-canon-type.js';
import { ClientImageGump } from '../www/ui/client-image-gump.js';
import { ClientGumpBackground } from '../www/ui/client-gump-background.js';
import { ClientHudDivider } from '../www/ui/client-hud-divider.js';
import { ClientGumpRoot } from '../www/ui/client-gump-root.js';
import { GumpDrawState, GumpPixelTarget, setGumpDrawClip } from '../www/render/gump-draw.js';

// 원본 다리 패턴의 같은 TYPE·프레임 순서와 실제 픽셀 범위를 항목에 전달한다.
test('생산 항목 자산: 실제 다리 TYPE·SHP와 Canon 패턴', () => {
    const scene = new CampaignCursorScene();
    try {
        const geometry = scene.metadata, shape = new RawCanonPixelShape(scene.core.pool, scene.core.types, geometry.frames, geometry.shapes, geometry.hotspots, geometry.geometry, { scaleX: 16, scaleY: 11 });
        const hooks = {}; bindInventoryItemAssets(hooks, scene.assets, geometry, shape);
        const type = scene.core.bridgeState.bridgeType, query = { type, argument: 0, direction: 0, x: 0, y: 0, explicitFrame: false };
        assert.equal(hooks.type(type).gumpFrame, scene.assets.types()[type - 70].definition.gumpFrame);
        assert.deepEqual(hooks.type(type).frames.codes, geometry.frames[type].frames.map((code) => ({ side: code.side, variant: code.variant, number: code.number ?? 0, flags: code.flags })));
        assert.deepEqual(hooks.measure(type, 0, false), shape.measure(type, 0, 0, false));
        const actual = []; hooks.walk(query, (cell) => actual.push(cell)); const expected = [];
        const meta = geometry.frames[type], decoder = decodeCanonType(meta.frames, meta.defaultFrame, geometry.geometry.patternTypes, query, geometry.geometry.decoderDiagnostics);
        // 독립 디코더의 실제 모든 유효 칸과 순서를 비교한다.
        while (decoder.valid) { expected.push({ frame: decoder.frame, x: decoder.x, y: decoder.y }); decoder.advance(); }
        assert.ok(actual.length > 0); assert.deepEqual(actual, expected); assert.throws(() => hooks.type(188), /범위/u);
        assert.throws(() => hooks.walk({ ...query, type: 188 }, () => {}), /타입 오류/u);
    } finally { scene.world.dispose(); }
});

// 실제 배경에서 파생된 구분선도 같은 자산 생성과 클립 출력에 참여한다.
test('Client HUD 구분선: 실제 패널 배경 생성·픽셀·수명', () => {
    const scene = new CampaignCursorScene(), root = new ClientGumpRoot(200, 200, () => 0, () => {});
    try {
        const identity = Uint8Array.from({ length: 256 }, (_value, index) => index);
        const background = new ClientGumpBackground(root, scene.assets, () => 0, () => ({ bright: identity, dark: identity }), () => ({ basic: new Uint8Array(9) }), () => 0);
        const divider = new ClientHudDivider(root, { background, backgroundColor: () => 0, dividerColor: () => 55, closePopups: () => {} });
        const type = scene.assets.find('fortGump').block + 70, token = divider.allocate();
        divider.construct(token, 0, 80, 5, 0x02a00000, type, 2); root.attach(token, 20, 20);
        const draw = new GumpDrawState(); setGumpDrawClip(draw, { left: 30, top: 20, right: 60, bottom: 25 });
        const pixels = new Uint8Array(40000).fill(255); root.draw(token, draw, new GumpPixelTarget(pixels, 200, 200, draw));
        assert.ok(pixels.some((value) => value !== 255)); assert.equal(pixels[20 * 200 + 20], 255);
        root.remove(token); assert.equal(root.node(token), null);
    } finally { root.dispose(); scene.world.dispose(); }
});

// 실제 원본 그림을 같은 Gump 클립에 출력하고 픽셀 기반 적중과 삭제를 검사한다.
test('Client 이미지 Gump: 실제 다리 SHP 출력·적중·삭제', () => {
    const scene = new CampaignCursorScene(), root = new ClientGumpRoot(200, 200, () => 0, () => {});
    try {
        const state = { localOwner: 0, ownerMaps: new Uint32Array(188), shadowOffset: { x: 5, y: 5 } };
        const image = new ClientImageGump(root, scene.assets, state, () => { throw new Error('선택하지 않은 색표'); }, () => 0);
        const token = image.allocate(); image.construct(token, scene.core.bridgeState.bridgeType, 0, 0); root.attach(token, 40, 40);
        const draw = new GumpDrawState(); setGumpDrawClip(draw, { left: 0, top: 0, right: 200, bottom: 200 });
        const pixels = new Uint8Array(40000).fill(255); root.draw(token, draw, new GumpPixelTarget(pixels, 200, 200, draw));
        assert.ok(pixels.some((value) => value !== 255));
        let found = false;
        // 실제 출력된 픽셀의 부모 로컬 좌표를 같은 적중 장치로 확인한다.
        for (let y = 0; y < 200 && !found; y += 1) for (let x = 0; x < 200; x += 1) if (pixels[y * 200 + x] !== 255 && image.hit(token, x, y)) { found = true; break; }
        assert.equal(found, true); root.remove(token); assert.throws(() => image.hit(token, 0, 0), /저장소/u);
    } finally { root.dispose(); scene.world.dispose(); }
});

// 실제 원본 패널 배경을 Client Gump 클립과 같은 픽셀에 출력한다.
test('Client 배경 Gump: 실제 패널 SHP·클립·삭제', () => {
    const scene = new CampaignCursorScene(), root = new ClientGumpRoot(200, 200, () => 0, () => {});
    try {
        const identity = Uint8Array.from({ length: 256 }, (_value, index) => index);
        const background = new ClientGumpBackground(root, scene.assets, () => 0, () => ({ bright: identity, dark: identity }), () => ({ basic: new Uint8Array(9) }), () => 0);
        const type = scene.assets.find('fortGump').block + 70, token = background.create(50, 50, 0x02000000, type, 2); root.attach(token, 20, 20);
        const draw = new GumpDrawState(); setGumpDrawClip(draw, { left: 30, top: 30, right: 60, bottom: 60 });
        const pixels = new Uint8Array(40000).fill(255); root.draw(token, draw, new GumpPixelTarget(pixels, 200, 200, draw));
        assert.ok(pixels.some((value) => value !== 255)); assert.equal(pixels[10 * 200 + 10], 255); assert.equal(pixels[20 * 200 + 20], 255);
        root.remove(token); assert.throws(() => root.draw(token, draw, new GumpPixelTarget(pixels, 200, 200, draw)), /토큰/u);
    } finally { root.dispose(); scene.world.dispose(); }
});
