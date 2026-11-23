// 커서 Gump 연결의 원본 명령·부모 공유와 팝업 종료 관문을 확인한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { bindCampaignCursorGumps, makeCampaignCursorClientEffects } from '../www/core/campaign-cursor-client-effects.js';
import { FontSet, ALL_FONT_PATHS } from '../www/assets/font-set.js';
import { GumpText } from '../www/render/gump-text.js';
import { decodeGumpTextContext } from '../www/render/gump-text-draw.js';
import { InputCode } from '../www/core/input-queue.js';
import { UnicodeFontProvider } from '../www/assets/unicode-font.js';
import { readData } from './helpers.mjs';

// 실제 컨트롤 호출의 인자와 반환을 보존하고 기존 팝업 훅은 덮어쓰지 않는다.
test('캠페인 커서 Gump: 화면 조회·분배·조각과 팝업 연결', () => {
    const calls = [], effects = { placement: {}, drop: {}, lifecycle: {} };
    const controls = { pickGump: (point) => { calls.push(['pick', point]); return 7; }, dispatchGump: (...args) => { calls.push(['dispatch', ...args]); return 19; },
        gumpParentSid: (token) => { calls.push(['parent', token]); return 23; }, handleGump: (...args) => { calls.push(['handle', ...args]); return 41; }, requestClosePopups: () => calls.push('close') };
    bindCampaignCursorGumps(effects, controls); assert.equal(effects.placement.gumpAt(-2, 8), 7);
    assert.equal(effects.placement.accept(7, 8, 9, 10, 2), 19); assert.equal(effects.placement.accept(7, 8, 9, 10, 0), 19);
    assert.equal(effects.placement.parent, effects.drop.guiParent); assert.equal(effects.drop.guiParent(7), 23); assert.equal(effects.drop.guiFlags(7), 41);
    assert.equal(effects.drop.guiReceive(7, 123), undefined); effects.lifecycle.closePopups();
    assert.deepEqual(calls, [['pick', { x: -2, y: 8 }], ['dispatch', 7, 8, 9, 10, true], ['dispatch', 7, 8, 9, 10, false], ['parent', 7],
        ['handle', 7, 0x0200005f, 0, 0], ['handle', 7, 0x0200005e, 123, 0], 'close']);
    const existing = () => calls.push('existing'), other = { placement: {}, drop: {}, lifecycle: { closePopups: existing } };
    bindCampaignCursorGumps(other, controls); assert.equal(other.lifecycle.closePopups, existing); other.lifecycle.closePopups(); assert.equal(calls.at(-1), 'existing');
});

// 실제 FontSet 측정과 현재 Client 장치 교체를 사용하며 입력 효과는 값형으로 보존한다.
test('캠페인 커서 Client: 현재 글꼴·번역·화면·입력 장치', () => {
    const fonts = new FontSet(new Map(ALL_FONT_PATHS.map((name) => [name, readData(name)]))), calls = [], words = Uint32Array.of(2, 0, 17, 0, 0, 9, 1, 1);
    let renderer = { invalidate: (...args) => calls.push(['first', ...args]) }, menu = null, translation = 'first';
    const client = { fonts: () => fonts, translate: (text) => `${translation}:${text}`, getRenderer: () => renderer,
        getCursor: () => ({ set: (value) => calls.push(['cursor', value]) }), poll: (code) => { calls.push(['poll', code]); return { x: 12, y: 14 }; },
        recordDeveloperBuildPlacement: (...args) => calls.push(['record', ...args]) };
    const controls = { menus: () => menu, pickGump: () => 0, dispatchGump: () => 0, gumpParentSid: () => 0, handleGump: () => 0, requestClosePopups: () => {} };
    const original = { placement: { previous: 7 }, lifecycle: { highlight: () => {} }, drop: {} }, effects = makeCampaignCursorClientEffects(client, controls, original);
    assert.equal(original.placement.invalidate, undefined); assert.equal(effects.placement.previous, 7); assert.equal(effects.lifecycle.highlight, original.lifecycle.highlight);
    assert.equal(effects.lifecycle.translate('A'), 'first:A'); translation = 'next'; assert.equal(effects.lifecycle.translate('A'), 'next:A');
    const expected = new GumpText(fonts).measure(Uint8Array.of(65, 66, 0), decodeGumpTextContext(words));
    assert.deepEqual(effects.lifecycle.measure('AB\0ignored', words), [expected.width, expected.height]);
    assert.throws(() => effects.drop.showSp(1, 2, 3, 4), /Gump 트리/); menu = { showSp: (...args) => calls.push(['sp', ...args]) };
    effects.drop.showSp(1, 2, 3, 4); effects.lifecycle.selectCursor(0xffffffff); assert.deepEqual(effects.lifecycle.mouse(), { x: 12, y: 14 });
    effects.drop.record(7, 'A', 1, 2, 3, 4); effects.placement.invalidate({ left: 1 }, 2); renderer = { invalidate: (...args) => calls.push(['second', ...args]) }; effects.placement.invalidate({ left: 3 }, 4);
    assert.deepEqual(calls, [['sp', 1, 2, 3, 4], ['cursor', -1], ['poll', InputCode.leftButton], ['record', 7, 'A', 1, 2, 3, 4], ['first', { left: 1 }, 2], ['second', { left: 3 }, 4]]);
    delete client.recordDeveloperBuildPlacement; assert.throws(() => effects.drop.record(1), /건설 기록 장치/);
});

// 한국어 공급자가 있으면 원본 BYTE가 아닌 UTF-16 글자와 NUL을 같은 측정기에 전달한다.
test('캠페인 커서 Client: 한국어 글꼴 측정', () => {
    const glyphs = [], unicode = new UnicodeFontProvider({ glyph: (code) => { glyphs.push(code); return { advance: 13, left: 0, top: 0, width: 0, height: 0, coverage: new Uint8Array() }; } });
    const fonts = new FontSet(new Map(ALL_FONT_PATHS.map((name) => [name, readData(name)])), { unicode });
    const effects = makeCampaignCursorClientEffects({ fonts: () => fonts }, {}, {}), words = Uint32Array.of(2, 0, 17, 0, 0, 9, 1, 1);
    const measured = effects.lifecycle.measure('가나', words); assert.deepEqual(glyphs, [0xac00, 0xb098]);
    const expected = new GumpText(fonts).measure(Uint16Array.of(0xac00, 0xb098, 0), decodeGumpTextContext(words));
    assert.deepEqual(measured, [expected.width, expected.height]);
});

// 측정(lifecycle.measure)이 쓰는 글자 배열과 그리기(raw-campaign-cursor-display 의 `fonts.encode(`${text}\0`)`)가 쓰는 글자 배열이 실제 한국어 경고 문구에서 같아, 경고·비용 문구의 측정 폭과 그려지는 폭이 어긋나지 않는다. (2026-11-23 WB04-E1256)
// 웹은 한국어를 유니코드 글꼴 경로(UTF-16 코드 단위)로 측정·출력하며 두 경로가 같은 `FontSet.encode` 를 쓴다(원본의 Windows-1252 바이트 대 표시 코덱 문제는 웹에 없다).
test('캠페인 커서 Client: 한국어 경고 문구의 측정 폭과 그리기 폭이 같다', async () => {
    const { readFileSync } = await import('node:fs'), { XlatTable } = await import('../www/assets/xlat.js'), { REPO_ROOT } = await import('./helpers.mjs');
    const korean = new XlatTable(readFileSync(`${REPO_ROOT}/webpj/translations/xlat.korean`, 'utf8'));
    const unicode = new UnicodeFontProvider({ glyph: (code) => ({ advance: code > 0x7f ? 13 : 6, left: 0, top: 0, width: 0, height: 0, coverage: new Uint8Array() }) });
    const fonts = new FontSet(new Map(ALL_FONT_PATHS.map((name) => [name, readData(name)])), { unicode });
    const effects = makeCampaignCursorClientEffects({ fonts: () => fonts }, {}, {}), words = Uint32Array.of(2, 0, 17, 0, 0, 9, 1, 1), text = new GumpText(fonts);
    const warnings = ["Priest can't get here.", 'Must be placed over constructing artifact', 'Cannot place here', 'Energy not satisfied', 'Only on Controlled Island'];
    for (const key of warnings) {
        const translated = korean.translate(key); assert.notEqual(translated, key, `${key} 한국어 번역`);
        const drawn = text.measure(fonts.encode(`${translated}\0`), decodeGumpTextContext(words)), measured = effects.lifecycle.measure(translated, words);
        assert.deepEqual(measured, [drawn.width, drawn.height], key);
        assert.ok(measured[0] > 13 * 5, `${key} 폭`);
    }
});
