// 원본 갱신1474·측정305행과 실제 FontSet/Renderer·도움말 현재 콜백을 대조한다. (2026-11-14 WB04-E414)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT, readData } from './helpers.mjs';
import { readRows, put } from './raw-support.mjs';
import { decodeGumpNode } from './gump-node-support.mjs';
import { FontSet, fontCachePath, ALL_FONT_PATHS } from '../www/assets/font-set.js';
import { UnicodeFontProvider } from '../www/assets/unicode-font.js';
import { GumpCreateState, GumpCreateNode, GumpCreate } from '../www/ui/gump-create.js';
import { GumpTree, GumpTreeState } from '../www/ui/gump-tree.js';
import { GumpCollection } from '../www/render/gump-collection.js';
import { GumpRedraw, clampOriginalScreenRect } from '../www/render/gump-redraw.js';
import { Renderer } from '../www/render/renderer.js';
import { measureGumpText } from '../www/core/gump-text-measure.js';
import { GumpText, bindStoneButtonText } from '../www/render/gump-text.js';
import { GumpHelpState, GumpHelp, bindGumpHelp } from '../www/ui/gump-help.js';
import { InputCode } from '../www/core/input-queue.js';

// 두 파일은 독립 관측이며 원본 파일에 다시 쓰지 않는다.
const redrawRows = readRows(path.join(REPO_ROOT, 'cpppj/tests/fixtures/gump-redraw-x86.tsv')); const textRows = readRows(path.join(REPO_ROOT, 'cpppj/tests/fixtures/gumptext-x86.tsv'));
// raw DWORD 사각형/정수열의 직렬화다.
const rect = (text) => { const [left, top, right, bottom] = text.split(',').map((v) => Number(v) | 0); return { left, top, right, bottom }; };
const numbers = (...values) => values.map((v) => v >>> 0).join(',');
// 입력 폭 표와 메트릭으로 실제 .chfnt 메모리 파일을 만든다. 측정 결과는 계산하지 않는다.
function fixtureFonts() {
    const files = new Map();
    // 폰트 입력은 모두 첫 폭 표이고 뒤 세 표/그림은 비어 있다.
    for (const row of textRows.filter((r) => r[0] === 'Font')) { assert.equal(row.length, 5); const data = new Uint8Array(4132); data.set(Uint8Array.from('BitmapFontData\x1a\0', (c) => c.charCodeAt(0))); const height = Number(row[3]); put(data, 16, 1); put(data, 20, height); put(data, 24, height - 5); put(data, 28, 5); put(data, 32, 256); const widths = Uint8Array.from(Buffer.from(row[4], 'hex')); assert.equal(widths.length, 1024); data.set(widths, 36); const name = fontCachePath(Number(row[1]), Number(row[2])); assert.ok(name); files.set(name, data); }
    return new FontSet(files);
}

test('GumpRedraw_Original1474: Whole728/Map728/Clamp18의 요청·전체 캐시·투영 순서가 같다', { skip: redrawRows === null }, () => {
    assert.equal(redrawRows.length, 1474); const counts = { Whole: 0, Map: 0, Clamp: 0 };
    // 각 원문 입력은 새 노드/지도/요청 저장소로 재생한다.
    for (const row of redrawRows) {
        assert.equal(row.length, 11); counts[row[0]] += 1; const bounds = rect(row[4]); let requests = '-'; let caches = '-'; let projected = '-';
        if (row[0] === 'Clamp') { const r = clampOriginalScreenRect(rect(row[10]), bounds); requests = numbers(r.left, r.top, r.right, r.bottom); }
        else {
            const nodes = new Map(); const anchors = new Map(); const projections = []; const calls = [];
            // 기존128BYTE 입력은 뒤의 미사용 글자 문맥32BYTE만0으로 채워 공통 디코더를 사용한다.
            for (const item of row[5].split(';')) { const [token, bytes] = item.split('='); assert.equal(bytes.length, 256); nodes.set(Number(token), decodeGumpNode(bytes + '00'.repeat(32))); }
            // 투영의 정수 응답만 입력이며 실제 위치/캐시는 GumpCollection이 계산한다.
            if (row[6] !== '-') for (const item of row[6].split(';')) { const [sid, x, y] = item.split(',').map(Number); anchors.set(sid, { x: x | 0, y: y | 0 }); }
            const hooks = { node: (token) => nodes.get(token), project: (sid) => { projections.push(sid); return anchors.get(sid); } }; const collection = new GumpCollection({ root: Math.min(...nodes.keys()), overrideHead: 0 }, hooks); const redraw = new GumpRedraw(collection, { node: hooks.node, map: () => bounds, invalidate: (r, flags) => calls.push(numbers(r.left, r.top, r.right, r.bottom, flags)) });
            if (row[0] === 'Whole') redraw.whole(Number(row[3]), Number(row[2])); else { assert.equal(row[0], 'Map'); redraw.map(Number(row[3]), Number(row[2])); }
            requests = calls.join(';') || '-'; caches = [...nodes].sort(([a], [b]) => a - b).map(([token, n]) => numbers(token, n.cached.x, n.cached.y)).join(';'); projected = projections.join(',') || '-';
        }
        assert.deepEqual([requests, caches, projected], row.slice(7, 10), `${row[0]} ${row[1]}`);
    }
    assert.deepEqual(counts, { Whole: 728, Map: 728, Clamp: 18 });
});

test('GumpText_Native305: 실제 캐시/FontSet의 전 슬롯·스타일/길이·감김·출력 alias가 원본과 같다', { skip: textRows === null }, () => {
    const rows = textRows.filter((r) => r[0] === 'Case' && r[1] === 'originals'); assert.equal(rows.length, 305); const fonts = fixtureFonts(); const text = new GumpText(fonts);
    // 기대 폭/높이는 저장한 PE 관측 열만 읽고 재계산하지 않는다.
    for (const row of rows) { assert.equal(row.length, 13); const bytes = Uint8Array.from(Buffer.from(row[10], 'hex')); const context = { flags: Number(row[5]), font: Number(row[3]), style: Number(row[4]), shadowX: Number(row[6]), shadowY: Number(row[7]) }; let shared = 17; const output = Number(row[9]) ? { get width() { return shared; }, set width(v) { shared = v; }, get height() { return shared; }, set height(v) { shared = v; } } : { width: 17, height: 19 }; text.measure(bytes, context, output, Number(row[8])); assert.deepEqual([output.width, output.height], [Number(row[11]), Number(row[12])], row.slice(0, 10).join(',')); }
    assert.equal(fonts.get(0, 0).height, 31);
});

test('GumpRedraw 현재 투영→크기→지도·가시성/계약: 콜백 뒤의 필드와 역전 범위를 원문 순서로 처리한다', () => {
    const nodes = new Map([[1, new GumpCreateNode()], [2, new GumpCreateNode()]]); nodes.get(1).flags = 1; const n = nodes.get(2); Object.assign(n, { flags: 0x2001, parent: 1, object: 7, width: 12.75, height: 9.5 }); let map = { left: 0, top: 0, right: 10, bottom: 10 }; const order = []; const requests = []; const collection = new GumpCollection({ root: 1, overrideHead: 0 }, { node: (token) => nodes.get(token), project: (sid) => { assert.equal(sid, 7); order.push('P'); map = { left: 100, top: 200, right: 300, bottom: 400 }; n.width = 20.75; return { x: 111, y: 211 }; } }); const redraw = new GumpRedraw(collection, { node: (token) => nodes.get(token), map: () => { order.push('M'); return map; }, invalidate: (rectangle, flags) => { assert.equal(flags, 0); order.push('I'); requests.push(rectangle); } }); redraw.map(2, 0xffffffff); assert.deepEqual(order, ['P', 'M', 'I']); assert.deepEqual(requests, [{ left: 111, top: 211, right: 131, bottom: 220 }]); order.length = requests.length = 0; nodes.get(1).flags = 0; redraw.whole(2); assert.deepEqual(order, []); nodes.get(1).flags = 1; n.flags = 1; nodes.get(1).parent = 2; assert.throws(() => redraw.whole(2), /순환/); assert.throws(() => redraw.map(99), /부재/); assert.throws(() => new GumpRedraw(collection), /장치/);
});

test('GumpRedraw 실제 생성/트리/Renderer: 현재 정수 위치의 Whole 요청이 실제 dirty 출력에 도달한다', () => {
    const state = new GumpCreateState(); state.screenWidth = state.screenHeight = 16; const nodes = new Map([[1, new GumpCreateNode()], [2, new GumpCreateNode()]]); const create = new GumpCreate(state, { node: (token) => nodes.get(token), allocate: () => 1 }); create.initializeRoot(); create.construct(2, 11, 4, 4, 1); const requests = []; const collection = new GumpCollection(state.collection, create.collectionHooks()); const renderer = new Renderer(16, 16); const redraw = new GumpRedraw(collection, { node: (token) => nodes.get(token), map: () => ({ left: 0, top: 0, right: 16, bottom: 16 }), invalidate: (r, flags) => { requests.push({ ...r }); renderer.invalidate(r, flags); } }); let tree; tree = new GumpTree(state, new GumpTreeState(), { node: (token) => nodes.get(token), redraw: (token, argument) => redraw.whole(token, argument), push: (token) => tree.push(token), attach: (...args) => tree.attach(...args), kill: () => { throw new Error('예상 밖 교체 삭제'); } }); const pixels = new Uint8Array(256); renderer.draw(pixels, 16); const presented = []; renderer.present((...args) => presented.push(args)); tree.attach(2, 2, 3, 1, 0); assert.deepEqual(requests.at(-1), { left: 2, top: 3, right: 6, bottom: 7 }); assert.deepEqual(renderer.draw(pixels, 16), [{ left: 2, top: 3, right: 6, bottom: 7 }]); assert.deepEqual(nodes.get(2).cached, { x: 2, y: 3 });
});

test('GumpText 범위/출력 순서·메트릭: NUL/명시 길이/음수·alias·불법 스타일의 폭/높이 슬롯을 구별한다', () => {
    const widths = new Int32Array(256); widths[0] = 23; const output = { width: 17, height: 19 }; const context = { flags: 2, font: 0, style: 0, shadowX: -3, shadowY: 5 }; const bytes = Uint8Array.of(65, 66); assert.throws(() => measureGumpText(bytes, widths, 14, context, output), /NUL/); assert.deepEqual(output, { width: 17, height: 19 }); assert.throws(() => measureGumpText(bytes, widths, 14, context, output, 3), /범위/); assert.deepEqual(output, { width: 17, height: 19 }); measureGumpText(new Uint8Array(), widths, 14, context, output, -2); assert.deepEqual(output, { width: -3, height: 19 }); bytes[0] = 0; measureGumpText(bytes, widths, 14, context, output, 1); assert.deepEqual(output, { width: 20, height: 19 }); const fonts = fixtureFonts(); const text = new GumpText(fonts); assert.deepEqual(text.metrics({ font: 0, style: 0 }), [31, 26, 5]); assert.deepEqual(text.metrics({ font: 0xffffffff, style: 99 }), [31, 26, 5]); assert.deepEqual(text.metrics({ font: 2, style: 0 }), [0, 0, 0]); assert.throws(() => text.measure(bytes, { ...context, font: 0xffffffff }, output, 0), /슬롯/); assert.deepEqual(output, { width: 20, height: 19 });
});

test('GumpText 실제 FontSet·기본 문맥 바인딩/Unicode: 측정과 현재 Draw가 같은 저장소/인코딩을 사용한다', () => {
    const files = new Map(ALL_FONT_PATHS.map((name) => [name, readData(name)])); const fonts = new FontSet(files); const text = new GumpText(fonts); const hooks = {}; bindStoneButtonText(hooks, text); const words = Uint32Array.of(2, 5, 7, 0, 0, 9, 1, 1); const label = Uint8Array.of(65, 66, 0); const expected = fonts.measure(label.subarray(0, 2), { flags: 2, font: 5, style: 0, shadowX: 1, shadowY: 1 }); const output = { width: 0, height: 0 }; hooks.measure(2, label, words, output, 0xffffffff); assert.deepEqual(output, expected);
    const codes = []; const provider = new UnicodeFontProvider({ glyph: (code) => { codes.push(code); return { advance: code === 0xac00 ? 13 : 7, left: 0, top: 0, width: 0, height: 0, coverage: new Uint8Array() }; } }); const unicode = new FontSet(files, { unicode: provider }); const measured = new GumpText(unicode).measure(Uint16Array.of(0xac00, 65, 0), { flags: 2, font: 5, style: 0, shadowX: 1, shadowY: 1 }); assert.deepEqual(measured, { width: 21, height: 15 }); assert.deepEqual(codes, [0xac00, 65]);
});

test('GumpHelp 현재 콜백: NUL 주제·현재 제목/본문/사건·항목 뒤 루트와 소리 순서를 보존한다', () => {
    const create = new GumpCreateState(); create.entries[0] = 91; const node = new GumpCreateNode(); node.alternateTextPointer = 5; const state = new GumpHelpState(); state.body = 11; state.event = 12; const calls = []; const help = new GumpHelp(create, state, { node: () => node, text: (token) => { assert.equal(token, 5); return 'priest\0ignored'; }, beginMenu: (value) => { assert.equal(value, 0); assert.equal(state.topic, 'priestHelp'); calls.push('begin'); node.textPointer = 7; }, title: (token) => { assert.equal(token, 7); calls.push('title'); state.body = 21; state.event = 22; state.topic = 'changedHelp'; }, entry: (...args) => { assert.deepEqual(args, [21, 22, 'changedHelp', 1, 1, 0]); calls.push('entry'); create.entries[0] = 93; }, attachMenu: (...args) => { assert.deepEqual(args, [-7, 13, 93]); calls.push('attach'); }, sound: (name) => { assert.equal(name, 'opengump.wav'); calls.push('sound'); } }); const buttons = {}; bindGumpHelp(buttons, help); assert.equal(buttons.baseEvent(1, InputCode.rightButton, 0xfffffff9, 13), 1); assert.deepEqual(calls, ['begin', 'title', 'entry', 'attach', 'sound']); assert.equal(help.handle(1, InputCode.rightButton | InputCode.release, 0, 0), 0); node.alternateTextPointer = 0; assert.equal(help.handle(1, InputCode.rightButton, 0, 0), 0); assert.equal(calls.length, 5);
});

test('GumpHelp 도달 경계/부분 실패: 미선택 사건은 장치를 요구하지 않고 앞선 주제/메뉴 효과를 보존한다', () => {
    const create = new GumpCreateState(); const state = new GumpHelpState(); const node = new GumpCreateNode(); const help = new GumpHelp(create, state, { node: (token) => token === 1 ? node : null }); assert.throws(() => new GumpHelp(create, state), /저장소/); assert.throws(() => help.handle(99, 0, 0, 0), /토큰/); assert.equal(help.handle(1, InputCode.rightButton, 0, 0), 0); node.alternateTextPointer = 5; assert.equal(help.handle(1, InputCode.leftButton, 0, 0), 0); assert.throws(() => help.handle(1, InputCode.rightButton, 0, 0), /text/); const partial = new GumpHelp(create, state, { node: () => node, text: () => 'temple', beginMenu: () => { node.textPointer = 7; } }); assert.throws(() => partial.handle(1, InputCode.rightButton, 0, 0), /title/); assert.deepEqual([state.topic, node.textPointer], ['templeHelp', 7]);
});
