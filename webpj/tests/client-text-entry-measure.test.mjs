// 실제 원본 글꼴 캐시와 유니코드 글꼴을 편집 측정 경계에 연결해 검사한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import { FontSet } from '../www/assets/font-set.js';
import { UnicodeFontProvider } from '../www/assets/unicode-font.js';
import { displayTextCodec } from '../www/assets/display-text.js';
import { TextEntryGumpNode } from '../www/ui/text-entry-gump.js';
import { ClientGumpRoot } from '../www/ui/client-gump-root.js';
import { createClientTextMeasure } from '../www/ui/client-text-entry-controls.js';

// 원본 Arial 캐시의 글리프 폭과 확장 한글의 실제 글꼴 경계를 구분한다.
test('Client TextEntry 측정: 원본 글꼴과 확장 문자 및 바이트 길이', () => {
    const path = 'd/!Arial.normal.14.0.chfnt';
    const files = new Map([[path, new Uint8Array(readFileSync(new URL('../www/data/d/!Arial.normal.14.0.chfnt', import.meta.url)))]]);
    const rasterizer = { glyph: (code) => ({ advance: code === 0xd55c ? 12 : 6, width: 0, height: 0, left: 0, top: 0, alpha: new Uint8Array() }) };
    const fonts = new FontSet(files, { unicode: new UnicodeFontProvider(rasterizer) });
    const owner = new ClientGumpRoot(200, 100, () => 0, () => {}), node = new TextEntryGumpNode();
    const token = owner.allocateDerived(node, () => 0, () => {}); node.textContext.set([0, 5, 0, 0, 0, 0, 1, 1]);
    const measure = createClientTextMeasure(owner, fonts), native = fonts.get(5, 0);
    assert.deepEqual(measure(token, Uint8Array.of(65, 66, 0)), [native.glyph(65).advance + native.glyph(66).advance, 14]);
    const mixed = displayTextCodec.encode('A한B'), terminated = Uint8Array.from([...mixed, 0]);
    assert.deepEqual(measure(token, terminated), [native.glyph(65).advance + 12 + native.glyph(66).advance, 14]);
    assert.deepEqual(measure(token, terminated, 5), [native.glyph(65).advance + 12, 14]);
    assert.deepEqual(measure(token, terminated, 1), [native.glyph(65).advance, 14]);
    node.textContext[0] = 2; node.textContext[6] = 2; node.textContext[7] = 3;
    assert.deepEqual(measure(token, terminated), [native.glyph(65).advance + 12 + native.glyph(66).advance + 2, 17]);
    assert.throws(() => measure(token, mixed), /NUL/u); assert.throws(() => measure(token, mixed, 7), /길이 범위/u);
    owner.discardUnregistered(token); owner.dispose();
});
