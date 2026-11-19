// 실제 TYPE·SHP·Canon 측정과 생산 항목 자산 경계를 같은 World에서 검증한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { CampaignCursorScene } from './campaign-cursor-support.mjs';
import { RawCanonPixelShape } from '../www/core/raw-canon-pixel-shape.js';
import { bindInventoryItemAssets } from '../www/ui/inventory-item-gump.js';
import { decodeCanonType } from '../www/core/raw-canon-type.js';

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
