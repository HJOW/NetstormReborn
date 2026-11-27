// 일본어 글꼴의 외부 적재 수명·실패와 실제 줄바꿈/제품 문구/번호 계약을 검사한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { DATA_ROOT, readDataMap } from './helpers.mjs';
import { buildJapaneseSupport } from '../www/assets/japanese-assets.js';
import { buildMenuResources } from '../www/assets/menu-assets.js';
import { createJapaneseGlyphRasterizer } from '../www/platform/japanese-font.js';
import { JAPANESE_FONT_STYLESHEET } from '../www/assets/japanese-asset-config.js';
import { productCreditsText, productCreditsLabel } from '../www/assets/product.js';
import { originalLanguageNumber, savedLanguage, parseLanguageTarget } from '../www/platform/language-choice.js';
import { Netstorm, normalizeLanguage } from '../www/netstorm.js';
import { ScrollingText } from '../www/ui/scroll-text.js';
import { createTextContext } from '../www/render/glyph-text.js';

// 같은FontFaceSet/링크수명 계약을 가진 최소문서다. 네트워크의 성공/실패/시간초과를 입력으로 제어한다.
function fontDocument(mode = 'success', pending = null) {
    // 이 적재가 소유한 링크와 폰트요청 기록이다.
    const links = new Set(), calls = [], loadedFaces = [];
    const document = {
        defaultView: { setTimeout: mode === 'timeout' ? (callback) => { queueMicrotask(callback); return 1; } : setTimeout, clearTimeout },
        createElement(name) {
            if (name === 'canvas') return { getContext: () => ({}) };
            assert.equal(name, 'link'); return { remove() { links.delete(this); } };
        },
        head: { append(link) { links.add(link); queueMicrotask(() => mode === 'error' ? link.onerror?.() : link.onload?.()); } },
        fonts: { [Symbol.iterator]() { return [{ family: '"Noto Sans JP"', load: () => { loadedFaces.push('common'); return pending ?? Promise.resolve({ status: 'loaded' }); } }, { family: "'Noto Sans JP'", load: () => { loadedFaces.push('rare'); return pending ?? Promise.resolve({ status: 'loaded' }); } }, { family: 'Other Font', load: () => { throw new Error('선택하지않은글꼴'); } }][Symbol.iterator](); }, load(spec, characters) { calls.push({ spec, characters }); return pending ?? Promise.resolve(mode === 'empty' ? [] : [{ status: 'loaded' }]); } },
    };
    return { document, links, calls, loadedFaces };
}

// 실제 문자선적재는문서폰트API에만전달되고 두굵기준비/자기링크반납을 수행한다.
test('일본어CDN: 두굵기를기다리고 등록링크를 반납한다', async () => {
    const { document, links, calls, loadedFaces } = fontDocument();
    const rasterizer = await createJapaneseGlyphRasterizer({ document, text: '日本語日本語' });
    assert.equal(links.size, 1); assert.equal([...links][0].href, JAPANESE_FONT_STYLESHEET);
    assert.equal(new URL([...links][0].href).searchParams.has('text'), false);
    assert.deepEqual(calls.map((call) => call.spec), ['400 12px "Noto Sans JP"', '700 12px "Noto Sans JP"']);
    assert.ok(calls.every((call) => call.characters === '日本語'));
    assert.deepEqual(loadedFaces, ['common', 'rare'], '처음쓰는글자용부분글꼴도미리적재하며다른글꼴은건드리지않는다.');
    rasterizer.dispose(); rasterizer.dispose(); assert.equal(links.size, 0);
});

// 대체글꼴로조용히성공하지않고 외부 실패/빈등록/시간초과를앱에전달하며링크를정리한다.
test('일본어CDN: 오류·빈등록·시간초과를전달하고링크를남기지않는다', async () => {
    // 세실패원인은별도로실행하며각경계의정리를확인한다.
    for (const mode of ['error', 'empty', 'timeout']) {
        const { document, links } = fontDocument(mode);
        await assert.rejects(createJapaneseGlyphRasterizer({ document, text: '日本語' })); assert.equal(links.size, 0, mode);
    }
});

// 화면을반납한세대의늦은글꼴결과가현재앱에스타일시트를남기지않는다.
test('일본어적재: 취소된세대의글꼴결과를반납한다', async () => {
    let finish;
    const pending = new Promise((resolve) => { finish = resolve; }), { document, links, calls } = fontDocument('success', pending);
    const app = new Netstorm({ language: 'japanese' }); app._root = { ownerDocument: document }; app._loadGeneration = 1;
    const files = new Map([['d/xlat.japanese', new TextEncoder().encode('***\nCampaign\n---\nキャンペーン\n===\n')]]);
    const loading = app._loadJapanese({ loadBundle: async () => files }, 0, 0, 1);
    // 글꼴대기가시작될때까지유한한마이크로태스크만양보한다.
    for (let index = 0; index < 10 && calls.length < 2; index += 1) await new Promise((resolve) => setImmediate(resolve));
    assert.equal(calls.length, 2); app._loadGeneration = 2; finish([{ status: 'loaded' }]);
    assert.equal(await loading, null); assert.equal(links.size, 0);
});

// 일본어의웹선택번호3과원본엔진번호5를섞지않고브라우저별칭/제품추가문구를지킨다.
test('일본어선택: 별칭·저장3·원본번호5와크레딧/초기화를연결한다', () => {
    assert.deepEqual(['ja','ja-JP','japanese','JA-jp'].map(normalizeLanguage), Array(4).fill('japanese'));
    assert.equal(normalizeLanguage('javascript'), 'english');
    assert.deepEqual(['english','korean','japanese','unknown'].map(originalLanguageNumber), [0,7,5,0]);
    assert.equal(savedLanguage({ getItem: () => '{"values":{"language":3}}' }), 'japanese');
    assert.equal(parseLanguageTarget('Patch1101.SetLanguage(3)'), 3);
    assert.match(productCreditsLabel('japanese'), /パッチ クレジット/u);
    const body = productCreditsText('japanese');
    assert.match(body, /HJOW/u); assert.equal((body.match(/Copyright 2026 HJOW/gu) ?? []).length, 1);
    assert.match(body, /Anthropic Claude Code/u); assert.match(body, /再読み込み/u); assert.match(body, /Patch1101.ResetAllConfirmed/u);
});

// 실제FontSet/ScrollingText로긴무공백일본어와링크·결합문자·영문낱말을같은입력으로검증한다.
test('일본어줄바꿈: 문장부호/문자·스타일·링크를보존하고폭안에배치한다', async () => {
    const manifest = JSON.parse(fs.readFileSync(path.join(DATA_ROOT, 'manifest.json'), 'utf8'));
    const menu = readDataMap(manifest.files.filter((entry) => entry.bundle === 'menu').map((entry) => entry.path));
    const japanese = readDataMap(manifest.files.filter((entry) => entry.bundle === 'japanese').map((entry) => entry.path));
    // 이검사는줄배치계약이며실제Noto글리프모양은브라우저실측으로별도확인한다.
    const support = await buildJapaneseSupport(japanese, { createRasterizer: () => ({ glyph: () => ({ advance: 12, left: 0, top: 0, width: 1, height: 1, coverage: Uint8Array.of(255) }) }) });
    const { fonts } = buildMenuResources(menu, support), context = createTextContext({ font: 0 });
    const body = new ScrollingText(fonts, context), source = '「日本語を読む。」Storm Power を確認する。か\u3099日本語。';
    body.append(fonts.encode(source + '\n'), context, 42); body.setWidth(72); body.fitHeight();
    const lines = body.softLines.map((line) => String.fromCharCode(...line.fragments.flatMap((fragment) => Array.from(fragment.bytes))));
    assert.equal(lines.join(''), source); assert.ok(body.maximumWidth() <= 72);
    assert.ok(lines.every((line) => !/^(?:[、。）」』]|\u3099)/u.test(line) && !/[「『（]$/u.test(line)));
    assert.ok(lines.some((line) => line.includes('Storm'))); assert.ok(lines.some((line) => line.includes('Power')));
    assert.ok(body.softLines.flatMap((line) => line.fragments).every((fragment) => fragment.tag === 42));
});

// CDN실패시읽을수있는영어안내를제공하고저장된일본어를다시선택해실제재적재할수있다.
test('일본어대체: 영어시작안내와같은저장선택재시도를허용한다', () => {
    const app = new Netstorm({ language: 'japanese' }), writes = []; let reloads = 0;
    app._gameData = { language: 'english' }; app._settings = { set: (...args) => writes.push(args) };
    app._view = () => ({ location: { search: '?lang=japanese', pathname: '/', hash: '' }, history: { replaceState() {} } });
    app.options.reload = () => { reloads += 1; };
    assert.equal(app.texts.clickToStart, 'Click to start'); assert.equal(app._chooseLanguage(3), true);
    assert.equal(reloads, 1); assert.deepEqual(writes, [['language', 3]]);
    app._gameData = { language: 'japanese' }; assert.equal(app.texts.clickToStart, 'クリックして開始'); assert.equal(app._chooseLanguage(3), false);
});
