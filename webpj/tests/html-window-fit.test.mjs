// HTML 전체Fit의26객체·부모6필드·20child·현재메뉴/전역·측정횟수·효과를 원본160행과 대조한다. 실제 게임 실행은 없다. (2026-11-14 WB04-E457)
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import { HtmlWindowFit } from '../www/ui/html-window-fit.js';
import { HtmlWindowGump, HtmlWindowGumpNode, HtmlWindowGumpState } from '../www/ui/html-window-gump.js';
import { GumpCreate, GumpCreateNode, GumpCreateState } from '../www/ui/gump-create.js';
import { ButtonGumpNode } from '../www/ui/button-gump.js';
import { GumpTree, GumpTreeState } from '../www/ui/gump-tree.js';

// 동일 비트의 float32와 원문DWORD 직렬화다.
function fromBits(value) { const view = new DataView(new ArrayBuffer(4)); view.setUint32(0, value, true); return view.getFloat32(0, true); }
function bits(value) { const view = new DataView(new ArrayBuffer(4)); view.setFloat32(0, value, true); return view.getUint32(0, true); }
const numbers = (value) => value === '' ? [] : value.split(':').map(Number);
const pack = (values) => values.join(':');
// 원문Gump의 같은9필드와 HTML부모의 같은6필드다.
const KEYS = ['flags', 'depth', 'scale', 'baseWidth', 'baseHeight', 'width', 'height', 'x', 'y'];
const ROOT_KEYS = ['controlCount', 'body', 'requestedWidth', 'requestedHeight', 'menuColumns', 'auxiliarySerial'];
const ROOT_NAMES = ['count', 'body', 'rw', 'rh', 'columns', 'aux'];
const ADDRESSES = [0x5ca9b4, 0x531860, 0x531864, 0x5c78f0, 0x5c78f4, 0x5ca9cc, 0x5ca9d4, 0x5ca9d8, 0x5ca9d0];
// 기대출력을 보지 않고 명시 원문 입력만 적용한다.
function setFields(node, values) { KEYS.forEach((key, index) => { node[key] = index < 2 ? values[index] : fromBits(values[index]); }); }
// 현재 실제 객체9필드를 원문DWORD로 읽는다.
function fields(node) { return KEYS.map((key, index) => index < 2 ? node[key] >>> 0 : bits(node[key])); }
// 현재 부모6필드를 같은 순서로 읽고 쓴다.
function rootFields(node) { return ROOT_KEYS.map((key) => node[key] >>> 0); }
function setRoot(node, values) { ROOT_KEYS.forEach((key, index) => { node[key] = values[index]; }); }

// 각 입력행은 새 실제 노드와 독립 측정장치/현재메뉴를 사용한다. 기대배치는 오직 마지막 비교에서만 읽는다.
function replay(row) {
    const node = new HtmlWindowGumpNode(); const children = Array.from({ length: 25 }, () => new GumpCreateNode());
    const object = (token) => { if (token === 1) return node; if (token < 2 || token > 26) throw new Error('Fit 검사 실제주소 없음'); return children[token - 2]; };
    const values = numbers(row[2]);
    // 실제26객체의9필드를 같은 순서로 구성한다.
    for (let token = 1; token <= 26; token += 1) setFields(object(token), values.slice((token - 1) * 9, token * 9));
    setRoot(node, numbers(row[3])); const slots = numbers(row[4]); node.controls.forEach((entry, index) => { entry.child = slots[index]; });
    const globals = numbers(row[5]); const natural = numbers(row[6]); const widths = numbers(row[7]); const heights = numbers(row[8]); const menus = numbers(row[9]);
    let heightCalls = 0; const trace = []; const stop = new Error('명시 중단');
    // 하위장치의 실제효과 뒤 명시 변이/중단만 적용한다.
    const event = (name, args) => {
        trace.push(name + (args.length ? `:${pack(args)}` : ''));
        if (row[12] !== '-') {
            const [when, key, value] = row[12].split('.');
            if (when === name) { if (key === 'view') globals[0] = Number(value); else { const current = rootFields(node); const index = ROOT_NAMES.indexOf(key); assert.notEqual(index, -1); current[index] = Number(value); setRoot(node, current); } }
        }
        if (row[13] === name) throw stop;
    };
    // 전달된 원시 좌표만 실제 객체에 저장하며 배치 예상값을 계산하지 않는다.
    const attach = (name, token, x, y, parent, mode) => { const target = object(token); target.x = x; target.y = y; target.flags = (target.flags & ~8) >>> 0; event(name, [token, bits(x), bits(y), parent, mode]); };
    const hooks = {
        node: (token) => token === 1 ? node : null, base: object,
        fitButton: (token) => { assert.ok(token >= 4 && token <= 23); object(token).baseWidth = fromBits(widths[token - 4]); event('fitButton', [token]); },
        push: (token) => { object(token).flags = (object(token).flags | 8) >>> 0; event('push', [token]); },
        fitContent: (token, x, y) => { assert.deepEqual([x, y], [0, 0]); object(token).baseWidth = fromBits(natural[0]); object(token).baseHeight = fromBits(natural[1]); event('fitContent', [token]); },
        fitHeight: (token) => { assert.ok(heightCalls < heights.length); object(token).baseHeight = fromBits(heights[heightCalls++]); event('fitHeight', [token]); },
        slider: (token, value) => event('slider', [token, value]), scrollable: (token, value) => event('scroll', [token, value]), requestScroll: (token, value) => event('request', [token, value >>> 0]),
        attach: (...args) => attach('attach', ...args), attachBase: (...args) => attach('attachBase', ...args),
        currentMenu: () => { assert.ok(menus.length); const token = menus.at(-1); event('menuCurrent', [token]); return token; },
        attachMenu: (x, y, parent) => { assert.ok(menus.length); const token = menus.at(-1); const target = object(token); target.x = Math.fround(x); target.y = Math.fround(y); target.flags = (target.flags & ~8) >>> 0; event('menuAttach', [token, x >>> 0, y >>> 0, parent]); menus.pop(); },
        reference: (serial) => { event('reference', [serial]); return Number(row[11]); },
        read: (address) => { const index = ADDRESSES.indexOf(address); assert.notEqual(index, -1); return globals[index]; },
        addDepth: (token, delta) => { object(token).depth = (object(token).depth + delta) >>> 0; event('depth', [token, delta]); },
    };
    const fit = new HtmlWindowFit(hooks); const args = numbers(row[10]); let completed = true;
    try { fit.attach(1, fromBits(args[0]), fromBits(args[1]), args[2], args[3]); } catch (error) { if (error !== stop) throw error; completed = false; }
    const output = [node, ...children].flatMap(fields);
    assert.deepEqual([String(Number(completed)), pack(output), pack(rootFields(node)), pack(node.controls.map((entry) => entry.child)), pack(globals), pack(menus), String(heightCalls), trace.join(';')], row.slice(14, 22), row[0]);
}

test('HTML 전체Fit: 원본10.78 160행의 모든객체·현재필드·효과순서가같다', () => {
    const rows = fs.readFileSync(new URL('../../cpppj/tests/fixtures/html-window-fit-x86.tsv', import.meta.url), 'utf8').trim().split(/\r?\n/).slice(1); assert.equal(rows.length, 160);
    // 같은160입력을 C++와같은명시하위로 실행하되 기대결과를 생성하지 않는다.
    for (const line of rows) { const row = line.split('\t'); assert.equal(row.length, 24); replay(row); }
});
test('HTML Fit: 같은주소/필수하위를 검사하고 signedcount와 실제20행 범위를 구별한다', () => {
    assert.throws(() => new HtmlWindowFit({}), TypeError); const node = new HtmlWindowGumpNode();
    assert.throws(() => new HtmlWindowFit({ node: () => node, base: () => new GumpCreateNode() }).attach(1, 0, 0, 0, 0), /실제 부모 없음/);
    let fits = 0; node.controlCount = 21; node.controls.forEach((entry) => { entry.child = 2; });
    const fit = new HtmlWindowFit({ node: () => node, base: () => node, fitButton: () => { fits += 1; } }); assert.throws(() => fit.attach(1, 0, 0, 0, 0), RangeError); assert.equal(fits, 20);
    node.controlCount = 0xffffffff; assert.throws(() => fit.attach(1, 0, 0, 0, 0), /미연결/); assert.equal(fits, 20);
});
test('HTML 생성/본문/버튼과Fit은 같은실제Gump등록·트리에서손계산한배치를만든다', () => {
    const state = new GumpCreateState(); state.screenWidth = 640; state.screenHeight = 480; const nodes = new Map([[1, new GumpCreateNode()], [2, new HtmlWindowGumpNode()]]); const lookup = (token) => nodes.get(token);
    const create = new GumpCreate(state, { node: lookup, allocate: () => 1 }); create.initializeRoot();
    const treeHooks = { node: lookup, redraw: () => {}, push: (token) => tree.push(token), attach: (...args) => tree.attach(...args), kill: () => { throw new Error('미선택Kill'); } }; const tree = new GumpTree(state, new GumpTreeState(), treeHooks); let next = 3; const parsed = [];
    const fit = new HtmlWindowFit({
        node: (token) => token === 2 ? lookup(token) : null, base: lookup, fitButton: () => {}, push: (token) => tree.push(token), fitContent: (token) => { lookup(token).baseWidth = 80; lookup(token).baseHeight = 12; }, fitHeight: (token) => { lookup(token).baseHeight = lookup(token).height = 12; },
        attach: (...args) => tree.attach(...args), attachBase: (...args) => tree.attach(...args), reference: () => 0,
        read: (at) => { if (at === 0x5ca9cc || at === 0x5ca9d0) return 0; if (at === 0x5ca9d4) return 640; if (at === 0x5ca9d8) return 480; throw new Error('미선택화면'); }, addDepth: (token, delta) => tree.addDepth(token, delta),
    });
    const html = new HtmlWindowGump(new HtmlWindowGumpState(), {
        node: (token) => token === 2 ? lookup(token) : null, base: lookup, button: lookup, constructDecoration: (token) => create.construct(token, 57, 0, 0, 1),
        allocate: (size) => { const token = next++; nodes.set(token, size === 0x170 ? new ButtonGumpNode() : new GumpCreateNode()); return token; }, constructButton: (token, label) => create.construct(token, 0, label === 'one' ? 30 : 50, 18, 1), constructBody: (token) => create.construct(token, 0, 0, 0, 1),
        scrollable: () => {}, parseBody: (token, source) => parsed.push([token, source]), attach: (...args) => args[0] === 2 ? fit.attach(...args) : tree.attach(...args), reference: () => 0,
    });
    html.construct(2, 200, 30); html.addButton(2, 'one', 8, 9, 0); html.addButton(2, 'two', 10, 11, 1); html.setBody(2, 'body'); const root = nodes.get(2); fit.attach(2, 999, 999, 1, 0);
    assert.deepEqual([root.baseWidth, root.baseHeight, root.x, root.y, root.depth, root.parent], [200, 77, 220, 201.5, 0xffffffb0, 1]);
    assert.deepEqual(root.controls.slice(0, 2).map((entry) => { const node = lookup(entry.child); return [node.baseWidth, node.width, node.x, node.y, node.parent]; }), [[50, 50, 42, 42, 2], [50, 50, 107, 42, 2]]);
    assert.deepEqual([lookup(root.body).x, lookup(root.body).y, lookup(root.body).parent], [60, 20, 2]); assert.deepEqual(parsed, [[root.body, 'body\n']]); assert.equal(state.count, 5);
});
