// 원문 메타128행의 실제HtmlWindowGump/원시버퍼/명령효과와 NB08 옵션을 대조한다. 게임/브라우저 실행은 없다. (2026-11-14 WB04-E458)
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import { HtmlWindowMetadata } from '../www/ui/html-window-metadata.js';
import { HtmlWindowGump, HtmlWindowGumpNode, HtmlWindowGumpState } from '../www/ui/html-window-gump.js';
import { ButtonGumpNode } from '../www/ui/button-gump.js';
import { ScriptStringRing } from '../www/ui/script-command.js';

// 원문문자열/BYTE·DWORD 관측을 같은형식으로 직렬화한다.
const hex = (value) => Buffer.from(value, 'latin1').toString('hex');
const unpack = (value) => Buffer.from(value, 'hex').toString('latin1');
const pack = (values) => values.join(':');
// 실제첫NUL앞 문자열이며 원문물리밖은 거부한다.
function text(buffer) { const end = buffer.indexOf(0); assert.ok(end >= 0); return Buffer.from(buffer.subarray(0, end)).toString('latin1'); }
// 실제전체8192BYTE와double의 독립비트관측이다.
function fnv(buffer) { let value = 14695981039346656037n; for (const byte of buffer) value = BigInt.asUintN(64, (value ^ BigInt(byte)) * 1099511628211n); return value.toString(); }
function timeBits(value) { const view = new DataView(new ArrayBuffer(8)); view.setFloat64(0, value, true); return view.getBigUint64(0, true); }
// 원문입력만 물리버퍼에 설치하며 NUL뒤fill을 보존한다.
function bufferOf(value, capacity = 8192, fill = 0xa5) { const result = new Uint8Array(capacity).fill(fill); result.set(Buffer.from(value, 'latin1')); result[value.length] = 0; return result; }

// 같은 실제HTML부모와 버튼본문을 사용한다. 나머지는 cpppj관측과 같은명시 하위장치이며 결과를 만들지 않는다.
function scene(options = {}) {
    const node = new HtmlWindowGumpNode(); Object.assign(node, { flags: 9, controlCount: 2, menuColumns: options.columns ?? 0, timeoutParameter: 0xa5a5a5a5, onExitParameter: 0x5a5a5a5a });
    Object.assign(node.controls[0], { child: 2, command: 7, parameter: 8 }); Object.assign(node.controls[1], { child: 3, command: 7, parameter: 8 });
    const children = new Map([[2, new ButtonGumpNode()], [3, new ButtonGumpNode()]]); let next = 4; let closed = 0x020000be; const trace = []; const stores = []; const stop = new Error('명시 메타 중단');
    // 장치효과 뒤 현재열/닫힘사건 변경과 중단을 적용한다.
    const event = (name, args = '') => {
        trace.push(name + (args === '' ? '' : `:${args}`));
        if (options.mutation && options.mutation !== '-') { const [when, key, value] = options.mutation.split('.'); if (name === when) { if (key === 'columns') node.menuColumns = Number(value); else { assert.equal(key, 'closed'); closed = Number(value); } } }
        if (options.stop === name) throw stop;
    };
    const core = new HtmlWindowGump(new HtmlWindowGumpState(), {
        node: (token) => token === 1 ? node : null, base: (token) => token === 1 ? node : children.get(token), button: (token) => children.get(token), kill: (token) => event('kill', String(token)),
        allocate: (size) => { assert.equal(size, 0x170); const token = next++; children.set(token, new ButtonGumpNode()); return token; },
        constructButton: (token, label) => { children.get(token).buttonFlags = 0; event('button', `${token}:${hex(label)}`); }, attach: (token, x, y, parent, mode) => { assert.equal(x, 0); assert.equal(y, 0); event('attach', pack([token, 0, 0, parent, mode])); },
    });
    const hooks = {
        node: (token) => token === 1 ? node : null, clearButtons: (token) => core.clearButtons(token), addButton: (...args) => core.addButton(...args),
        commands: { lookup: (name) => { event('lookup', hex(name)); return options.lookup ?? 0x0200006d; }, unknown: (name) => event('diagnostic', hex(name)) },
        storeText: (value) => { stores.push(hex(value)); const token = 0x60000000 + stores.length; event('store', hex(value)); return token; },
        beginMenu: (flags) => event('begin', String(flags)), menuText: (label, command, parameter) => event('text', `${hex(label)}:${pack([command, parameter, 1, 1, 0])}`),
        checked: (checked, label, command, parameter, enabled) => event('checked', `${checked >>> 0}:${hex(label)}:${pack([command, parameter, 1, Number(enabled)])}`),
        maps: (label) => { event('mapRefresh', String(label === '*SOLOLIST*' ? 2 : 0)); event('mapRow', '0'); }, tech: (parameter) => event('tech', String(parameter)), themeTech: (low, high) => event('theme', pack([low, high >>> 0])),
        namedList: (key, command, argument) => event('named', `${hex(key)}:${command}:${hex(argument)}`), fileList: (key, command, argument) => event('files', `${hex(key)}:${command}:${hex(argument)}`),
        inlineList: (key, command, argument) => { event('inline', `${hex(key)}:${hex(command)}:${hex(argument)}`); return options.replacement ?? ''; },
        wallSeconds: () => { event('clock'); return options.wall ?? 10; }, idle: (token, enabled) => { node.flags = (enabled ? node.flags | 0x80 : node.flags & ~0x80) >>> 0; event('idle', pack([token, Number(enabled)])); },
        read: (address) => { if (address === 0x5cab30) return options.force ?? 0; if (address === 0x55a480) return options.developer ?? 0; assert.equal(address, 0x5429ec); return closed; },
        input: (limit, label, argument) => event('input', `${limit >>> 0}:${hex(label)}:${hex(argument)}`), send: (label, command, argument) => event('send', `${hex(label)}:${hex(command)}:${hex(argument)}`), queue: (...args) => event('queue', pack(args)),
    };
    return { node, children, hooks, trace, stores, stop, core, metadata: () => new HtmlWindowMetadata(hooks) };
}

test('HTML 메타: 원본10.78 128행의현재필드·20행·전체8192BYTE·고리/효과가같다', () => {
    const rows = fs.readFileSync(new URL('../../cpppj/tests/fixtures/html-window-metadata-x86.tsv', import.meta.url), 'utf8').trim().split(/\r?\n/).slice(1); assert.equal(rows.length, 128);
    // 각독립입력을 새실제부모에공급하며 기대결과를생성하지않는다.
    for (const line of rows) {
        const row = line.split('\t'); assert.equal(row.length, 22); const s = scene({ columns: Number(row[4]), force: Number(row[5]), developer: Number(row[6]), wall: Number(row[7]), replacement: unpack(row[8]), lookup: Number(row[9]), mutation: row[10], stop: row[11] });
        const buffer = bufferOf(unpack(row[2]), 8192, Number(row[3])); let completed = true; let result = 0;
        try { result = s.metadata().process(1, buffer); } catch (error) { if (error !== s.stop) throw error; completed = false; }
        const n = s.node; const time = timeBits(n.timeoutSeconds); const fields = [n.flags, n.controlCount, n.menuColumns, Number(time & 0xffffffffn), Number(time >> 32n), n.timeoutCommand, n.timeoutParameter, n.onExitCommand, n.onExitParameter];
        assert.deepEqual([String(Number(completed)), pack(fields), pack(n.controls.flatMap((entry) => [entry.child, entry.command, entry.parameter])), fnv(buffer), hex(text(buffer)), s.stores.join(';'), s.trace.join(';'), String(completed ? result : 0)], row.slice(12, 20), row[0]);
    }
});
test('NB08: 같은버퍼의본문뒤목록은원문모드에서잘리고수정모드에서실제버튼이생성된다', () => {
    const prefix = '<h1>User Made Campaigns</h1>\n'; const source = `${prefix}$Menu=%Campaigns,Start,0\nTail`;
    // 같은원문입력과 실제버튼본체에서 옵션만바꾼다.
    for (const repair of [false, true]) {
        const s = scene({ replacement: '$Button=Play,Start,1\n' }); s.hooks.repairInlineList = repair; const buffer = bufferOf(source); s.metadata().process(1, buffer);
        assert.equal(text(buffer), prefix + 'Tail'); assert.equal(s.node.controlCount, repair ? 1 : 0);
        if (repair) assert.deepEqual(s.node.controls[0], { child: 4, command: 0x0200006d, parameter: 1 });
    }
});
test('NB08: 수정목록은물리용량-1에서자르고NUL을쓰며원문모드의과대복사오류는제거효과를유지한다', () => {
    const source = 'Prefix\n$Menu=%L,Cmd,0\nTail'; const s = scene({ replacement: 'X'.repeat(500) }); s.hooks.repairInlineList = true; const buffer = bufferOf(source, 64); s.metadata().process(1, buffer);
    assert.equal(text(buffer), 'Prefix\n' + 'X'.repeat(56)); assert.equal(buffer[63], 0);
    const raw = scene({ replacement: 'X'.repeat(500) }); const unpatched = bufferOf(source, 64); assert.throws(() => raw.metadata().process(1, unpatched), RangeError); assert.equal(text(unpatched), 'Prefix\nTail');
});
test('Checked의후속캡처는lookup뒤같은물리의현재BYTE를읽고이미복사한label은보존한다', () => {
    const s = scene(); const buffer = bufferOf('$Checked=label,Cmd,0,1,1\n'); let checked;
    s.hooks.commands.lookup = () => { const current = text(buffer); buffer[current.indexOf('label')] = 0x5a; const index = current.indexOf(',1,1'); buffer[index + 1] = 48; buffer[index + 3] = 48; return 12; };
    s.hooks.checked = (...args) => { checked = args; }; s.metadata().process(1, buffer); assert.deepEqual(checked, [0, 'label', 12, 0, false]);
});
test('실제80칸고리와maps열참조는현재부모를공유하고callback표는값복사한다', () => {
    const s = scene(); const ring = new ScriptStringRing(); s.hooks.storeText = (value) => ring.store(value); s.hooks.maps = (label, columns) => { assert.equal(label, '*SOLOLIST*'); columns.value += 2; };
    const metadata = s.metadata(); s.hooks.commands.lookup = () => { throw new Error('교체됨'); }; s.hooks.storeText = s.hooks.commands.lookup;
    metadata.process(1, bufferOf('$OnExit=exit,Config,key=value\n$Menu=*SOLOLIST*,Do,0\n')); assert.equal(ring.resolve(s.node.onExitParameter), 'key=value\n'); assert.equal(s.node.onExitCommand, 0x0200006d); assert.equal(s.node.menuColumns, 3);
});
test('버퍼/지역캡처/현재부모한계는앞선버튼정리뒤명시적으로실패한다', () => {
    assert.throws(() => new HtmlWindowMetadata({}), TypeError); const s = scene(); assert.throws(() => s.metadata().process(0, bufferOf('')), /현재 부모 없음/);
    assert.throws(() => s.metadata().process(1, new Uint8Array(4).fill(1)), /NUL 범위/); assert.equal(s.node.controlCount, 0); assert.deepEqual(s.trace.slice(0, 2), ['kill:2', 'kill:3']);
    assert.throws(() => s.metadata().process(1, bufferOf(`$Button=${'L'.repeat(80)},Cmd,0\n`)), /캡처 NUL/);
    const missing = scene(); missing.hooks.commands.lookup = null; assert.throws(() => missing.metadata().process(1, bufferOf('$Button=A,Cmd,0\n')), /조회 누락/);
});
