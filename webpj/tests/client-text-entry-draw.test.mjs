// 실제 TextEntry와 원본 글꼴·Gump 배경·픽셀 장치의 결합을 검사한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import { FontSet } from '../www/assets/font-set.js';
import { UnicodeFontProvider } from '../www/assets/unicode-font.js';
import { displayTextCodec } from '../www/assets/display-text.js';
import { ClientGumpRoot } from '../www/ui/client-gump-root.js';
import { ClientTextEntryControls } from '../www/ui/client-text-entry-controls.js';
import { createClientTextOutput } from '../www/ui/client-text-entry-draw.js';
import { TextEntryGumpState } from '../www/ui/text-entry-gump.js';
import { ButtonGumpState } from '../www/ui/button-gump.js';
import { StoneButtonGumpState } from '../www/ui/stone-button-gump.js';
import { ScrollGumpState } from '../www/ui/scroll-gump.js';
import { ScrollGumpWidgetState } from '../www/ui/scroll-gump-widgets.js';
import { GumpDraw, GumpDrawState, GumpPixelTarget, setGumpDrawClip } from '../www/render/gump-draw.js';
import { GumpBackgroundDraw, GumpBackgroundDrawState } from '../www/render/gump-background-draw.js';
import { GumpGlyphColorState } from '../www/render/gump-text-draw.js';

// 같은 트리에서 배경과 영문·한글·영문을 순서대로 실제 표면에 그린다.
test('Client TextEntry 출력: 원본·유니코드 글리프와 공통 클립', () => {
    const path = 'd/!Arial.normal.14.0.chfnt';
    const files = new Map([[path, new Uint8Array(readFileSync(new URL('../www/data/d/!Arial.normal.14.0.chfnt', import.meta.url)))]]);
    const rasterizer = { glyph: () => ({ advance: 12, width: 1, height: 1, left: 0, top: 0, coverage: Uint8Array.of(255) }) };
    const fonts = new FontSet(files, { unicode: new UnicodeFontProvider(rasterizer) });
    const owner = new ClientGumpRoot(240, 100, () => 0, () => {}), state = new GumpDrawState();
    setGumpDrawClip(state, { left: 0, top: 0, right: 240, bottom: 100 });
    const pixels = new Uint8Array(240 * 100), surface = new GumpPixelTarget(pixels, 240, 100, state), images = [];
    const target = { fill: (...args) => surface.fill(...args), image: (image, point, colors) => { images.push({ image, point: { ...point }, color: colors[100] }); surface.image(image, point, colors); } };
    const background = new GumpBackgroundDraw(owner.collection(), state, new GumpBackgroundDrawState(), { node: (token) => owner.node(token), fill: (...args) => target.fill(...args) });
    const output = createClientTextOutput(owner, fonts, background, target, new GumpGlyphColorState());
    const buttonState = new ButtonGumpState(), stoneState = new StoneButtonGumpState(), scrollState = new ScrollGumpState();
    const controls = new ClientTextEntryControls(owner, { entryState: new TextEntryGumpState(), buttonState, stoneState, scrollState, widgetState: new ScrollGumpWidgetState(),
        backgroundColor: () => 35, now: () => 1, poll: () => ({ code: 0 }), sound: () => {}, baseEvent: () => 0, fonts, ...output });
    const token = controls.allocate(), initial = displayTextCodec.encode('A한B');
    controls.construct(token, 150, 60, initial, 5, 0x04000005, 42); owner.attach(token, 10, 10);
    const node = owner.node(token), buffer = controls.storage().find(node.buffer); buffer.select(initial.length, initial.length);
    const draw = new GumpDraw(owner.collection(), state, { draw: (current) => controls.backend().draw(current) }); draw.draw(token);
    const font = fonts.get(5, 0), a = font.glyph(65), b = font.glyph(66);
    assert.equal(images.length, 3); assert.deepEqual(images.map((item) => item.color), [42, 42, 42]);
    assert.equal(images[1].point.x, 10 + a.drawAdvance); assert.equal(images[1].point.y, 10);
    assert.equal(images[2].point.x, 10 + a.drawAdvance + 12 + (b.image?.left ?? 0));
    assert.equal(pixels[10 * 240 + 10 + a.drawAdvance], 42); assert.equal(pixels[65 * 240 + 20], 5);
    assert.deepEqual(state.clip, { left: 0, top: 0, right: 240, bottom: 100 });
    // 선택 영역은 실제 배경색 글리프와 전경색 사각형으로 출력한다.
    images.length = 0; buffer.select(1, 5); draw.draw(token);
    const selectionX = 10 + a.advance;
    assert.deepEqual(images.map((item) => item.color), [42, 5, 42]);
    assert.equal(pixels[10 * 240 + selectionX], 5); assert.equal(pixels[10 * 240 + selectionX + 11], 42);
    // 혼합 줄에서도 모든 그림자 글리프 다음에 모든 본문 글리프를 그린다.
    images.length = 0; node.textContext[0] = 2; node.textContext[5] = 9;
    output.drawText(token, Uint8Array.from([...initial, 0]), { x: 0, y: 0 }, null);
    assert.deepEqual(images.map((item) => item.color), [9, 9, 9, 42, 42, 42]);
    controls.close(); owner.dispose();
});
