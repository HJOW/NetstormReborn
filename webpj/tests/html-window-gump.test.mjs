// HTML 부모의 실제24필드·20행·8192BYTE/자식/전역과 외부효과를 독립 원본PE250+88행에 대조한다. 게임 실행은 없다. (2026-11-14 WB04-E456)
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import { HtmlWindowGump, HtmlWindowGumpNode, HtmlWindowGumpState } from '../www/ui/html-window-gump.js';
import { ButtonGumpNode } from '../www/ui/button-gump.js';
import { GumpCreate, GumpCreateNode, GumpCreateState } from '../www/ui/gump-create.js';
import { GumpBackgroundCreate } from '../www/ui/gump-background-create.js';
import { GumpDecoration, GumpDecorationState } from '../www/ui/gump-decoration.js';
import { GumpCollection } from '../www/render/gump-collection.js';
import { GumpLifetime, GumpLifetimeState } from '../www/ui/gump-lifetime.js';
import { GumpTree, GumpTreeState } from '../www/ui/gump-tree.js';

// 원문24필드의 순서와 float32필드 범위다. 다른파생필드/배경은 별도 저장소다.
const FIELD_NAMES = ['vtable', 'flags', 'command', 'secondaryDepth', 'scale', 'baseWidth', 'baseHeight', 'width', 'height', 'x', 'y', 'controlCount', 'body', 'updateDepth', 'requestedWidth', 'requestedHeight', 'menuColumns', 'auxiliarySerial', 'timeLo', 'timeHi', 'timeoutCommand', 'timeoutParameter', 'onExitCommand', 'onExitParameter'];
// fixture 변이 이름은 command/count 등 호스트이름과 다르다.
const MUTATION_NAMES = ['vtable', 'flags', 'kind', 'depth', 'scale', 'bw', 'bh', 'width', 'height', 'x', 'y', 'count', 'body', 'update', 'rw', 'rh', 'columns', 'aux', 'timeLo', 'timeHi', 'timeout', 'timeoutArg', 'exit', 'exitArg'];
// 동일 비트의 float32/float64를 산술 변환 없이 읽고 쓰는 검사 도구다.
function fromFloatBits(value) { const view = new DataView(new ArrayBuffer(4)); view.setUint32(0, value, true); return view.getFloat32(0, true); }
function floatBits(value) { const view = new DataView(new ArrayBuffer(4)); view.setFloat32(0, value, true); return view.getUint32(0, true); }
function fromDoubleBits(value) { const view = new DataView(new ArrayBuffer(8)); view.setBigUint64(0, BigInt(value), true); return view.getFloat64(0, true); }
function doubleBits(value) { const view = new DataView(new ArrayBuffer(8)); view.setFloat64(0, value, true); return view.getBigUint64(0, true); }
// 명시DWORD입력을 같은 노드에 적용하며 출력 기대값은 읽지 않는다.
function setFields(node, values) {
    // 원문의24필드 중 double을 제외한 필드만 직접 설정한다.
    for (let index = 0; index < FIELD_NAMES.length; index += 1) if (index !== 18 && index !== 19) node[FIELD_NAMES[index]] = index >= 4 && index <= 10 ? fromFloatBits(values[index]) : values[index];
    node.timeoutSeconds = fromDoubleBits(BigInt(values[18]) | (BigInt(values[19]) << 32n));
}
// 실제 현재값을 같은24DWORD로 직렬화한다.
function fields(node) {
    const time = doubleBits(node.timeoutSeconds);
    return FIELD_NAMES.map((key, index) => index === 18 ? Number(time & 0xffffffffn) : index === 19 ? Number(time >> 32n) : index >= 4 && index <= 10 ? floatBits(node[key]) : node[key] >>> 0);
}
// 실제8192BYTE의 독립 FNV64 관찰값이다.
function fnv(bytes) { let hash = 14695981039346656037n; for (const byte of bytes) hash = BigInt.asUintN(64, (hash ^ BigInt(byte)) * 1099511628211n); return hash.toString(); }
// 실제DWORD목록과 바이트문자열의 직렬화다.
const numbers = (value) => value.split(':').map(Number);
const pack = (values) => values.join(':');
const hex = (value) => Buffer.from(value, 'latin1').toString('hex');

// cpppj HtmlWindowGumpTests와 같은 명시 하위 장치/중단/재진입 변이를 제공한다.
function replay(row) {
    const node = new HtmlWindowGumpNode(); setFields(node, numbers(row[3])); node.html.fill(Number(row[5])); const slots = numbers(row[4]);
    //20행의 입력은 원본 관측과 같은 순서로 작성한다.
    node.controls.forEach((entry, index) => Object.assign(entry, { child: slots[index * 3], command: slots[index * 3 + 1], parameter: slots[index * 3 + 2] }));
    const children = Array.from({ length: 24 }, (_, index) => Object.assign(new ButtonGumpNode(), { serial: 1000 + index, secondaryDepth: 0x12345678, buttonFlags: 0x2468 }));
    const state = new HtmlWindowGumpState(); state.instances = Number(row[6]); state.kind = (Number(row[7]) << 24) >> 24; state.lastCloseTime = -7.5; const trace = [];
    // 생성된 실제자식 토큰2..25의 고정 주소다.
    const child = (token) => { if (token < 2 || token > 25) throw new Error('검사 자식 주소 없음'); return children[token - 2]; };
    // 실제 호출 뒤에만 명시 변이/중단을 적용해 원문 재조회 위치를 검증한다.
    const event = (name, argument = '') => {
        trace.push(name + (argument === '' ? '' : `:${argument}`));
        if (row[12] !== '-') { const [when, key, value] = row[12].split('.'); if (when === name) { const current = fields(node); const index = MUTATION_NAMES.indexOf(key); assert.notEqual(index, -1); current[index] = Number(value); setFields(node, current); } }
        if (row[13] === name) throw stop;
    };
    const stop = new Error('명시 원문 중단');
    const hooks = {
        node: (token) => token === 1 ? node : null, base: (token) => token === 1 ? node : token >= 2 && token <= 25 ? child(token) : null, button: child,
        constructDecoration: () => { Object.assign(node, { vtable: 0x50ac70, flags: 9, command: 57, secondaryDepth: 0, scale: 1, baseWidth: 0, baseHeight: 0, width: 0, height: 0, x: 0, y: 0 }); event('base'); },
        destructBase: () => event('destruct'), push: (token) => event('push', String(token)), kill: (token) => event('kill', String(token)),
        allocate: (size) => { event('alloc', String(size)); assert.ok(size === 0x170 || size === 0xd8); return size === 0x170 ? 24 : 25; },
        constructButton: (token, label) => { child(token).buttonFlags = 0; event('button', `${token}:${hex(label)}`); }, constructBody: (token) => { child(token).secondaryDepth = 0; event('body', String(token)); },
        parseBody: (token, source) => event('parse', `${token}:${hex(source)}`), scrollable: (token, value) => event('scroll', `${token}:${value >>> 0}`),
        attach: (token, x, y, parent, mode) => event('attach', pack([token, floatBits(x), floatBits(y), parent, mode])),
        reference: (serial) => { event('reference', String(serial)); return Number(row[10]); }, wallSeconds: () => { event('wall'); return fromDoubleBits(row[11]); },
        queue: (...args) => event('queue', pack(args)), link: (token) => event('link', String(token)), baseHandle: (token, ...args) => { event('baseHandle', pack(args)); return 0xfedcba98; },
    };
    const gump = new HtmlWindowGump(state, hooks); const args = numbers(row[8]); const source = Buffer.from(row[9], 'hex').toString('latin1'); let result = 0; let completed = true;
    try {
        if (row[1] === 'construct') result = gump.construct(1, args[0], args[1]);
        else if (row[1] === 'destroy') gump.destruct(1);
        else if (row[1] === 'clear') gump.clearButtons(1);
        else if (row[1] === 'add') gump.addButton(1, source, args[1], args[2], args[3]);
        else if (row[1] === 'body') gump.setBody(1, source);
        else if (row[1] === 'idle') gump.idle(1);
        else { assert.equal(row[1], 'handle'); result = gump.handle(1, args[0], args[1], args[2]); }
    } catch (error) { if (error !== stop) throw error; completed = false; }
    assert.deepEqual([String(Number(completed)), pack(fields(node)), pack(node.controls.flatMap((entry) => [entry.child, entry.command, entry.parameter])), fnv(node.html), pack(children.flatMap((entry) => [entry.serial, entry.secondaryDepth, entry.buttonFlags])), String(state.instances), doubleBits(state.lastCloseTime).toString(), trace.join(';'), String(completed ? result : 0)], row.slice(14, 23), row[0]);
}

// 최초 원본수명/입력과 후속 실제Idle 관측을 구별해 각각 전행 비교한다.
for (const [file, count] of [['html-window-gump-x86.tsv', 250], ['html-window-idle-v2-x86.tsv', 88]]) {
    test(`HTML 부모: ${file} 원본10.78 ${count}행 전체상태·8192BYTE·효과`, () => {
        const rows = fs.readFileSync(new URL(`../../cpppj/tests/fixtures/${file}`, import.meta.url), 'utf8').trim().split(/\r?\n/).slice(1); assert.equal(rows.length, count);
        // 기대출력을 만들지 않고 각 독립 원문입력에 제품을 실행한다.
        for (const line of rows) { const row = line.split('\t'); assert.equal(row.length, 25); replay(row); }
    });
}

test('HTML 부모: 기본/파생 주소를 공유하고 선택하지 않은 장치는 선행 요구하지 않는다', () => {
    assert.throws(() => new HtmlWindowGump(new HtmlWindowGumpState(), {}), TypeError); const node = new HtmlWindowGumpNode(); const state = new HtmlWindowGumpState();
    const gump = new HtmlWindowGump(state, { node: () => node, base: () => node }); gump.idle(1); node.controlCount = 20; gump.addButton(1, '', 0, 0, 0);
    assert.throws(() => gump.construct(1, 0, 0), /미연결/); assert.throws(() => new HtmlWindowGump(state, { node: () => node, base: () => new GumpCreateNode() }).idle(1), /같은 실제 주소/);
    node.controlCount = 0xffffffff; assert.throws(() => gump.addButton(1, '', 0, 0, 0), RangeError); gump.clearButtons(1); assert.equal(node.controlCount, 0);
});
test('HTML 본문: LF없는1006BYTE·지역버퍼 한계와LF있는긴본문을 구별한다', () => {
    const node = new HtmlWindowGumpNode(); node.flags = 8; node.body = 2; const child = new GumpCreateNode(); const trace = [];
    const gump = new HtmlWindowGump(new HtmlWindowGumpState(), { node: () => node, base: (token) => token === 1 ? node : child, kill: (token) => trace.push(['kill', token]), allocate: () => 2, constructBody: () => {}, scrollable: () => {}, parseBody: (token, value) => trace.push(['parse', value]), attach: () => {}, reference: () => 0 });
    assert.throws(() => gump.setBody(1, 'x'.repeat(1007)), RangeError); assert.equal(trace.length, 0); gump.setBody(1, 'x'.repeat(1006)); assert.equal(trace.at(-1)[1].length, 1007);
    gump.setBody(1, 'x'.repeat(2000) + '\n'); assert.equal(trace.at(-1)[1].length, 2001); gump.setBody(1, 'A\0B'); assert.equal(trace.at(-1)[1], 'A\n');
});
test('HTML 본문/버튼: 할당0에서도 앞선 쓰기/부착과 실제 선택 오류를 보존한다', () => {
    const node = new HtmlWindowGumpNode(); node.flags = 8; node.body = 2; const trace = [];
    const hooks = { node: () => node, base: (token) => token === 1 ? node : null, kill: (token) => trace.push(['kill', token]), allocate: () => 0, attach: (...args) => trace.push(['attach', ...args]) };
    const gump = new HtmlWindowGump(new HtmlWindowGumpState(), hooks); assert.throws(() => gump.setBody(1, 'A'), /실제 자식 없음/); assert.equal(node.body, 0); assert.deepEqual(trace, [['kill', 2]]);
    gump.addButton(1, '', 8, 9, 0); assert.equal(node.controlCount, 1); assert.deepEqual(node.controls[0], { child: 0, command: 8, parameter: 9 }); assert.deepEqual(trace.at(-1), ['attach', 0, 0, 0, 1, 0]);
});
test('HTML 기본입력 위임은 사건/두인자의 전체DWORD 비트를 보존한다', () => {
    const node = new HtmlWindowGumpNode(); const state = new HtmlWindowGumpState(); let forwarded;
    const gump = new HtmlWindowGump(state, { node: () => node, base: () => node, baseHandle: (...args) => { forwarded = args; return 0x81234567; } });
    assert.equal(gump.handle(1, -1, -2, -3), 0x81234567); assert.deepEqual(forwarded, [1, 0xffffffff, 0xfffffffe, 0xfffffffd]);
});
test('HTML 공통장치 조립: 실제 장식/등록·트리·수명과 같은 장기count/OnExit/반납', () => {
    const registry = new GumpCreateState(); const nodes = new Map([[1, new GumpCreateNode()], [2, new HtmlWindowGumpNode()]]); const lookup = (token) => nodes.get(token); const events = [];
    const create = new GumpCreate(registry, { node: lookup, allocate: () => 1 }); create.initializeRoot(); const collection = new GumpCollection(registry.collection, { node: lookup });
    const life = new GumpLifetimeState(); const lifeHooks = { node: lookup, release: (token) => { events.push(['free', token]); nodes.delete(token); } }; const lifetime = new GumpLifetime(registry, life, lifeHooks);
    const treeHooks = { node: lookup, redraw: () => {}, push: (token) => tree.push(token), attach: (...args) => tree.attach(...args), kill: (token) => lifetime.kill(token) }; const tree = new GumpTree(registry, new GumpTreeState(), treeHooks); lifeHooks.push = treeHooks.push;
    const background = new GumpBackgroundCreate(registry, create, () => 0, { node: lookup }); const decoration = new GumpDecoration(registry, new GumpDecorationState(), background, null, collection, lifetime, { node: lookup, base: lookup, type: () => ({ shape: 0, firstFrame: 0 }) });
    const state = new HtmlWindowGumpState(); const gump = new HtmlWindowGump(state, { node: (token) => token === 2 ? lookup(token) : null, base: lookup, constructDecoration: (token) => decoration.construct(token, 0, 0), destructBase: (token) => lifetime.destructBase(token), reference: () => 0, wallSeconds: () => 12.5, queue: (...args) => events.push(['queue', ...args]), kill: (token) => lifetime.kill(token) });
    lifeHooks.deletingDestructor = (token, flags) => { if (token === 2) { gump.destruct(token); if (flags & 1) lifeHooks.release(token); } else lifetime.deleteScalar(token, flags); };
    gump.construct(2, 400, 30); assert.equal(registry.count, 2); assert.equal(state.instances, 1); assert.equal(nodes.get(2).requestedWidth, 400); tree.attach(2, 10, 20, 1, 0); nodes.get(2).onExitCommand = 8; nodes.get(2).onExitParameter = 9;
    lifetime.kill(2); assert.equal(state.instances, 1); lifetime.cleanup(); assert.equal(state.instances, 0); assert.equal(state.lastCloseTime, 12.5); assert.equal(registry.count, 1); assert.equal(nodes.get(1).child, 0); assert.deepEqual(events, [['queue', 8, 9, 9], ['free', 2]]);
});
