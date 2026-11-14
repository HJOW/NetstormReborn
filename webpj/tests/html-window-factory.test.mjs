// HTML 전체생성106행과두번파서/현재serial·선택언어관문을검사한다. 원본게임을실행하지않는다. (2026-11-14 WB04-E459)
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import { HtmlWindowFactory } from '../www/ui/html-window-factory.js';
import { HtmlWindowGumpNode, HtmlWindowGumpState } from '../www/ui/html-window-gump.js';
import { HtmlWindowMetadata } from '../www/ui/html-window-metadata.js';
import { parseHtml, createParserGlobals } from '../www/ui/html-text.js';

// 명시BYTE/float/double와전체버퍼의독립관측이다.
const unpack = (value) => Buffer.from(value, 'hex').toString('latin1');
const hex = (value) => Buffer.from(value, 'latin1').toString('hex');
function fromBits(value) { const view = new DataView(new ArrayBuffer(4)); view.setUint32(0, value, true); return view.getFloat32(0, true); }
function bits(value) { const view = new DataView(new ArrayBuffer(4)); view.setFloat32(0, value, true); return view.getUint32(0, true); }
function doubleBits(value) { const view = new DataView(new ArrayBuffer(8)); view.setFloat64(0, value, true); return view.getBigUint64(0, true).toString(); }
function fnv(buffer) { let value = 14695981039346656037n; for (const byte of buffer) value = BigInt.asUintN(64, (value ^ BigInt(byte)) * 1099511628211n); return value.toString(); }
function text(buffer) { const end = buffer.indexOf(0); assert.ok(end >= 0); return Buffer.from(buffer.subarray(0, end)).toString('latin1'); }

// 원문과동일한하위생성/파서/메타장치입력만제공하며기대출력을계산하지않는다.
function replay(row) {
    const state = new HtmlWindowGumpState(); state.createdSerial = 777; state.lastCloseTime = -88.5; const node = new HtmlWindowGumpNode(); node.html.fill(0xa5);
    let constructed = false; let parses = 0; let metas = 0; let first = ''; let second = ''; const trace = []; const stop = new Error('명시공장중단');
    const geometry = row[13].split(':').map(Number); const screen = row[14].split(':').map(Number); const globals = new Map([[0x5c78f0, screen[0]], [0x5c78f4, screen[1]], [0x563e38, Number(row[15])]]);
    // 실제장치효과뒤공유상태/부모/화면변이와중단을적용한다.
    const event = (phase, args = '') => {
        trace.push(phase + (args === '' ? '' : `:${args}`));
        if (row[20] !== '-') {
            const [when, key, input] = row[20].split('.'); const value = Number(input);
            if (phase === when) { if (key === 'created') state.createdSerial = value; else if (key === 'serial') node.serial = value; else if (key === 'width') node.baseWidth = fromBits(value); else if (key === 'height') node.baseHeight = fromBits(value); else globals.set(key === 'screenW' ? 0x5c78f0 : 0x563e38, value); }
        }
        if (phase === row[21]) throw stop;
    };
    const hooks = {
        node: (token) => { assert.equal(token, 1); return node; },
        create: (width, height) => { event('allocate', '8680'); if (Number(row[9])) return 0; event('construct', `${width}:${height}`); node.serial = Number(row[12]); node.baseWidth = fromBits(geometry[0]); node.baseHeight = fromBits(geometry[1]); node.html[0] = 0; constructed = true; return 1; },
        substitute: (source, capacity) => { event('substitute', `${hex(source)}:${capacity}:0:0:0`); return unpack(row[3]); },
        parse: (parser, root, source, capacity) => {
            parser.afterHeading = 0; event('bind', `${capacity}:${root ? '2:8192' : '0:0'}`); parses += 1; event(`parse${parses}`, `${hex(source)}:${root ? 2 : 0}:${parser.shown}:${parser.hiddenDepth}:${parser.afterHeading}`);
            const output = unpack(row[parses === 1 ? 4 : 5]); if (root) node.html.set(Buffer.from(row[parses === 1 ? 6 : 7], 'hex')); if (parses === 1) first = output; else second = output; return output;
        },
        wallSeconds: () => { event('clock'); return Number(row[16]); },
        metadata: (root, buffer) => { assert.equal(root, 1); metas += 1; event('meta', hex(text(buffer))); if (metas === 1) { if (row[18] !== '-') { const value = Buffer.from(row[18], 'hex'); buffer.set(value); buffer[value.length] = 0; } first = text(buffer); } return Number(row[17]); },
        fallback: () => { event('fallback', '4f6b427574746f6e:0:0:0'); return unpack(row[19]); },
        body: (root, value) => { assert.equal(root, 1); if (parses === 2) second = value; event('body', hex(value)); }, read: (address) => { assert.ok(globals.has(address)); return globals.get(address); },
        attach: (root, x, y, parent, mode) => { assert.equal(root, 1); event('attach', `${bits(x)}:${bits(y)}:${parent}:${mode}`); }, modal: (root, enabled) => { assert.equal(root, 1); event('modal', String(Number(enabled))); }, kill: (root) => { assert.equal(root, 1); event('kill'); },
    };
    let completed = true; let result = 0;
    try { result = new HtmlWindowFactory(state, hooks).show(unpack(row[2]), Number(row[8]), Number(row[10]), Number(row[11])); } catch (error) { if (error !== stop) throw error; completed = false; }
    assert.deepEqual([String(Number(completed)), String(state.createdSerial), doubleBits(state.lastCloseTime), String(constructed ? node.serial : 0), constructed ? fnv(node.html) : '0', hex(first), hex(second), trace.join(';'), String(result)], row.slice(22, 31), row[0]);
}

test('HTML 전체공장: 원본10.78 106행의전역·8192BYTE·현재serial·파서/효과순서', () => {
    const rows = fs.readFileSync(new URL('../../cpppj/tests/fixtures/html-window-factory-x86.tsv', import.meta.url), 'utf8').trimEnd().split(/\r?\n/).slice(1); assert.equal(rows.length, 106);
    // 모든독립원문입력을새장면에서실행해최종결과만비교한다.
    for (const line of rows) { const row = line.split('\t'); assert.equal(row.length, 33); replay(row); }
});

// 선택관문과경계검사용명시하위장치다. 실제창/표시를대체하는제품코드가아니다.
function smallScene() {
    const node = new HtmlWindowGumpNode(); Object.assign(node, { serial: 7, baseWidth: 80, baseHeight: 40 }); const state = new HtmlWindowGumpState(); const calls = []; const parsers = []; let count = 0;
    const hooks = {
        node: () => node, create: () => 1, substitute: (source) => source,
        parse: (parser, root, source, capacity) => { parsers.push(parser); count += 1; calls.push(['parse', root, source, capacity]); return source; }, wallSeconds: () => 1, metadata: () => 1,
        body: (root, value) => calls.push(['body', root, value]), attach: (...args) => calls.push(['attach', ...args]), modal: (...args) => calls.push(['modal', ...args]), kill: (root) => calls.push(['kill', root]),
        read: (at) => { if (at === 0x5c78f0) return 640; if (at === 0x5c78f4) return 480; assert.equal(at, 0x563e38); return 10; },
    };
    return { node, state, hooks, calls, parsers, factory: () => new HtmlWindowFactory(state, hooks), count: () => count };
}
test('suppress/할당0은첫파서효과만실행하며미선택하위는요구하지않는다', () => {
    // 두원문root0진입모드는같은첫파싱을실행한다.
    for (const suppress of [0, 1]) {
        let effects = 0; const state = new HtmlWindowGumpState(); const factory = new HtmlWindowFactory(state, { create: () => 0, substitute: (value) => value, parse: (parser, root, source) => { assert.equal(root, 0); assert.equal(source, 'A'); effects += 1; return 'A'; } });
        assert.equal(factory.show('A\0ignored', suppress, 0, 0), 0); assert.equal(effects, 1); assert.equal(state.createdSerial, 0);
    }
});
test('같은파서의조건상태·현재메타버퍼·부모링크꼬리와modal뒤현재serial을유지한다', () => {
    const s = smallScene(); let parser; let count = 0; s.node.html.fill(0xa5);
    s.hooks.parse = (current, root, source) => { count += 1; if (count === 1) { parser = current; current.shown = 0; current.hiddenDepth = 2; s.node.html[0] = 65; return 'Original'; } assert.equal(current, parser); assert.deepEqual([current.shown, current.hiddenDepth, source], [0, 2, 'After<br>Meta']); s.node.html[0] = 66; return 'second\0ignored'; };
    s.hooks.metadata = (root, buffer) => { buffer.set(Buffer.from('After\nMeta\0', 'latin1')); return 1; }; s.hooks.modal = () => { s.node.serial = 99; };
    assert.equal(s.factory().show('A', 0, 200, 30), 99); assert.equal(s.state.createdSerial, 7); assert.equal(s.node.html[0], 66); assert.equal(s.node.html[1], 0xa5); assert.ok(s.calls.some((row) => row[0] === 'body' && row[2] === 'second\n'));
});
test('localizedContent는ASCII없는선택에서만호출하고원문기본관문과UTF16경계를구별한다', () => {
    const raw = smallScene(); assert.equal(raw.factory().show('é', 0, 0, 0), 0); assert.ok(raw.calls.some((row) => row[0] === 'body' && row[2] === 'xxx')); assert.equal(raw.count(), 1);
    const localized = smallScene(); localized.hooks.localizedContent = (value) => value === 'é'; assert.equal(localized.factory().show('é', 0, 0, 0), 7); assert.equal(localized.count(), 2);
    const ascii = smallScene(); ascii.hooks.localizedContent = () => { throw new Error('미선택'); }; assert.equal(ascii.factory().show('A', 0, 0, 0), 7);
    assert.throws(() => smallScene().factory().show('한글', 0, 0, 0), /BYTE문자열/);
});
test('첫지역8192와마지막LF/NUL8192한계를구별하고앞선할당/파서효과를보존한다', () => {
    const substituted = smallScene(); substituted.hooks.substitute = () => 'A'.repeat(8192); assert.throws(() => substituted.factory().show('A', 0, 0, 0), /치환 지역/); assert.equal(substituted.state.createdSerial, 7); assert.equal(substituted.count(), 0);
    const first = smallScene(); first.hooks.parse = () => 'A'.repeat(8192); assert.throws(() => first.factory().show('A', 0, 0, 0), /지역 NUL/);
    const second = smallScene(); let count = 0; second.hooks.parse = () => ++count === 1 ? 'A' : 'A'.repeat(8191); assert.throws(() => second.factory().show('A', 0, 0, 0), /마지막 LF/); assert.equal(second.state.lastCloseTime, 1); assert.equal(count, 2);
    const limit = smallScene(); count = 0; limit.hooks.parse = () => ++count === 1 ? 'A' : 'A'.repeat(8190); assert.equal(limit.factory().show('A', 0, 0, 0), 7); assert.equal(limit.calls.find((row) => row[0] === 'body')[2].length, 8191);
});
test('현재실제HTML파서/공유전역과metadata의기본버튼을전체공장에서연결한다', () => {
    const s = smallScene(); const globals = createParserGlobals(); const nodes = []; const metadata = new HtmlWindowMetadata({ node: () => s.node, clearButtons: () => { s.node.controlCount = 0; }, commands: { lookup: () => 0x02000089 }, addButton: (root, label, command, parameter) => { nodes.push([root, label, command, parameter]); s.node.controlCount += 1; } });
    s.hooks.parse = (parser, root, source, capacity) => parseHtml(source, parser, globals, {}, capacity); s.hooks.metadata = (root, buffer) => metadata.process(root, buffer); s.hooks.fallback = () => '$Button=OK,DoNothing,0';
    assert.equal(s.factory().show('<h1>Header</h1>text', 0, 0, 0), 7); assert.deepEqual(nodes, [[1, 'OK', 0x02000089, 0]]); assert.equal(s.node.controlCount, 1); assert.equal(globals.background, ''); assert.ok(s.calls.some((row) => row[0] === 'body' && row[2].includes('Header')));
});
