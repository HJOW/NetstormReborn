// 내장 설명 조회/HtmlBody 부모의 원본 82행(두 x87 정밀도의 조회 순서·본문/링크 8192BYTE 전체 지문·부착/Push 호출 순서·serial)을 재생한다. 원본게임을 실행하지 않는다. (2026-11-15 WB04-E466)
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import { GumpCreate, GumpCreateNode, GumpCreateState } from '../www/ui/gump-create.js';
import { HTML_BODY_CAPACITY, HtmlBodyGump, HtmlBodyGumpNode, HtmlBodyGumpState } from '../www/ui/html-body-gump.js';
import { showEmbeddedTell } from '../www/ui/embedded-tell.js';

// 원문 좁은 바이트와 hex, 단정도 비트, FNV-1a 32 지문이다(NUL 뒤 8192BYTE 도 포함).
const unhex = (value) => Buffer.from(value, 'hex').toString('latin1'); const hex = (value) => Buffer.from(value, 'latin1').toString('hex');
const bits = (value) => { const view = new DataView(new ArrayBuffer(4)); view.setFloat32(0, value, true); return view.getUint32(0, true); };
const fingerprint = (bytes) => { let value = 2166136261; for (const byte of bytes) value = Math.imul(value ^ byte, 16777619) >>> 0; return value; };

// 한 장면을 만든다: 부모(1)·자식(2)·이전 자식(3)과 호출 기록, 명시 Native 파서 장치와 같은 물리 출력만 공급한다.
function makeScene(mask) {
    const node = new HtmlBodyGumpNode(); node.body.fill(0x5a); node.links.fill(0x5a); const child = new GumpCreateNode(); const oldChild = new GumpCreateNode(); const trace = [];
    const create = new GumpCreateState(); const state = new HtmlBodyGumpState(640, 480, 20, 7);
    const base = (token) => (token === 1 ? node : token === 2 ? child : token === 3 ? oldChild : null); const factory = new GumpCreate(create, { node: base });
    const hooks = {
        node: (token) => (token === 1 ? node : null), base, constructBase: (token, command, width, height, flags) => { factory.construct(token, command, width, height, flags); },
        allocate: (bytes) => { assert.equal(bytes, 0xd8); trace.push('allocateText'); return 2; },
        constructText: (token, width, height, font, color) => { assert.equal(token, 2); trace.push(`constructText:${width}:${height}:${font}:${color}`); },
        kill: () => { trace.push('kill'); }, push: (token) => { trace.push('push'); base(token).flags |= 8; },
        attach: (token, x, y, _parent, mode) => { trace.push(`attach:${token === 1 ? 'parent' : 'text'}:${bits(x)}:${bits(y)}:${mode}`); base(token).x = x; base(token).y = y; },
        styled: (_token, text) => { trace.push(`styled:${hex(text)}`); }, scroll: (_token, value) => { trace.push(`scroll:${value}`); }, slider: (_token, value) => { trace.push(`slider:${value}`); }, scrollable: (_token, value) => { trace.push(`scrollable:${value}`); },
    };
    const body = new HtmlBodyGump(state, hooks);
    // 명시 Native 파서 장치와 같은 물리 출력만 공급한다.
    const parse = (input, output, links) => {
        const nul = input.indexOf('\0'); input = nul < 0 ? input : input.slice(0, nul); assert.ok(output[0] === 0 && links[0] === 0); trace.push(`parse:${hex(input)}`);
        const value = `[${input}]`; for (let index = 0; index < value.length; index += 1) output[index] = value.charCodeAt(index); output[value.length] = 0; links.set([108, 105, 110, 107, 0]); if (mask & 32) node.serial = 9001;
    };
    return { node, child, trace, body, parse };
}

test('EmbeddedTell 원본 82행: 조회 순서·본문 설치·본문/링크 8192BYTE 지문·부모 필드·serial·호출 순서가 모두 일치한다', () => {
    const table = fs.readFileSync(new URL('../../cpppj/tests/fixtures/embedded-tell-x86.tsv', import.meta.url), 'utf8').split(/\r?\n/).filter(Boolean).map((line) => line.split('\t')); let checked = 0;
    for (const row of table) {
        if (row[0] === 'op') continue; assert.equal(row.length, 14, `행 ${checked}`);
        const mask = Number(row[1]); const key = unhex(row[2]); const source = unhex(row[3]); const { node, trace, body, parse } = makeScene(mask); let returned = 0;
        if (row[0] === 'SetHtml') { body.construct(1); node.x = 2.25; node.y = -3.5; if (mask & 16) node.text = 3; if (mask & 8) node.flags &= ~8; trace.length = 0; body.setHtml(1, source, parse); }
        else {
            const tell = {
                fallback: (name) => { trace.push(`fallback:${hex(name)}`); return mask & 4 ? source : null; }, missing: (name) => { trace.push(`missing:${hex(name)}`); },
                construct: () => { trace.push('new'); return body.construct(1); }, setHtml: (token, input) => { body.setHtml(token, input, parse); }, serial: (token) => { assert.equal(token, 1); return node.serial; },
            };
            if (mask & 1) tell.mission = (name) => { trace.push(`mission:${hex(name)}`); return mask & 2 ? source : null; };
            returned = showEmbeddedTell(key, tell);
        }
        const actual = [returned, node.flags, node.vtable, node.serial, node.text !== 0 ? 1 : 0, fingerprint(node.body), fingerprint(node.links), trace.join(';')];
        assert.deepEqual(actual, [Number(row[5]), Number(row[6]), Number(row[7]), Number(row[8]), Number(row[9]), Number(row[10]), Number(row[11]), row[12]], `행 ${checked} ${row[0]} mask ${mask}`); checked += 1;
    }
    assert.equal(checked, 82);
});

test('HtmlBodyGump 생성: 기본 160BYTE 뒤 가상 표·종류(부호 확장)·자식 0·두 영역 첫 바이트·화면 절반 크기를 쓰고 나머지 바이트는 보존한다', () => {
    const { node, body } = makeScene(0); node.body.fill(0x71); node.links.fill(0x72); body.construct(1);
    assert.deepEqual([node.vtable, node.command, node.text, node.body[0], node.body[1], node.links[0], node.links[1]], [0x50bd38, 20, 0, 0, 0x71, 0, 0x72]); assert.deepEqual([node.baseWidth, node.width, node.baseHeight, node.height], [320, 320 * node.scale, 240, 240 * node.scale]);
    const odd = new HtmlBodyGumpState(641, -481, -3, 0); const scene = makeScene(0); const gump = new HtmlBodyGump(odd, { node: () => scene.node, base: () => scene.node, constructBase: () => {} }); gump.construct(1);
    assert.deepEqual([scene.node.command, scene.node.baseWidth, scene.node.baseHeight], [0xfffffffd, 320, -240]); // 정수 나눗셈은 0 쪽으로 절삭한다.
});

test('HtmlBodyGump 자식 수명: Clear 는 Kill 뒤 0, SetText 는 새 생성 전까지 이전 토큰을 보존하고 부모 Push 표시가 없을 때만 다시 부착한다', () => {
    const { node, trace, body } = makeScene(0); body.construct(1); node.x = 4; node.y = 5; node.text = 3; assert.equal(node.flags & 8, 8); node.flags &= ~8; trace.length = 0; // 기본 생성은 Push 표시(8)를 켜 두므로 먼저 지운다.
    body.setText(1, 'abc\0def'); assert.deepEqual(trace, ['kill', 'allocateText', 'constructText:0:0:5:7', `attach:text:${bits(0)}:${bits(0)}:0`, `styled:${hex('abc')}`, 'scroll:0', 'slider:1', 'scrollable:1', 'push', `attach:parent:${bits(4)}:${bits(5)}:0`]); assert.equal(node.text, 2);
    trace.length = 0; body.setText(1, 'x'); assert.ok(trace.includes('kill') && !trace.includes('push')); // 부모에 Push 표시(8)가 있으면 다시 부착하지 않는다.
    trace.length = 0; body.clear(1); assert.deepEqual([trace, node.text], [['kill'], 0]); trace.length = 0; body.clear(1); assert.deepEqual([trace, node.text], [[], 0]);
});

test('HtmlBodyGump 맞춤/부착/파싱/본문 한도: 부모 폭에서 여백을 빼고 부착 뒤 여백 0 으로 다시 맞추며 NUL 없는 본문은 거부한다', () => {
    const { node, child, trace, body, parse } = makeScene(0); body.construct(1); body.setHtml(1, 'hi', parse); trace.length = 0;
    body.fit(1, 12.5); assert.deepEqual(trace, ['push', `attach:text:${bits(12.5)}:${bits(0)}:0`]); assert.deepEqual([child.baseWidth, child.baseHeight], [307.5, 240]);
    node.baseWidth = 100; trace.length = 0; body.fit(1, 12.5); assert.equal(trace.length, 2); assert.equal(child.baseWidth, 87.5);
    const hooks = { node: () => node, base: () => node, baseAttach: (...args) => { trace.push(`base:${args.join(',')}`); } }; node.text = 0; trace.length = 0; new HtmlBodyGump(new HtmlBodyGumpState(), hooks).attach(1, 1, 2, 3, 4); assert.deepEqual(trace, ['base:1,1,2,3,4']);
    // parse: 두 첫 바이트를 0 으로 쓰고 이미 출력이 있으면 설치하지 않는다.
    node.body[0] = 9; node.links[0] = 9; const calls = []; body.parse(1, { hasOutput: () => true, bind: () => calls.push('bind'), run: () => calls.push('run') }); assert.deepEqual([calls, node.body[0], node.links[0]], [['run'], 0, 0]);
    body.parse(1, { hasOutput: () => false, bind: (a, b) => { calls.push(`bind:${a.length}:${b.length}`); }, run: () => calls.push('run') }); assert.deepEqual(calls.slice(1), [`bind:${HTML_BODY_CAPACITY}:${HTML_BODY_CAPACITY}`, 'run']);
    assert.throws(() => body.setHtml(1, 'x', (_input, output) => { output.fill(1); }), /8192BYTE NUL 범위/);
    assert.throws(() => new HtmlBodyGump(new HtmlBodyGumpState(), {}).construct(1), /처리기 누락/);
});

test('HtmlBodyGump 경계: 기본 상태값·기본 생성 인자·부모 재부착 인자·첫 NUL 앞 본문·빈 본문·부착 뒤 여백 0 맞춤', () => {
    assert.deepEqual({ ...new HtmlBodyGumpState() }, { screenWidth: 0, screenHeight: 0, kind: 20, color: 0 });
    // 기본 생성은 (token, 0, 0, 0, 1) 로 부른다(이후 쓰기가 덮어쓰는 command/크기도 원문 인자 그대로).
    const scene = makeScene(0); const args = []; const gump = new HtmlBodyGump(new HtmlBodyGumpState(640, 480, 20, 7), { node: () => scene.node, base: () => scene.node, constructBase: (...values) => { args.push(values); } }); gump.construct(1); assert.deepEqual(args, [[1, 0, 0, 0, 1]]);
    // 부모 재부착은 부모 0·모드 0 이고 setText 의 NUL 앞만 StyleText 로 보낸다(맨 앞 NUL 은 빈 문자열).
    const { node, trace, body } = makeScene(0); body.construct(1); node.flags &= ~8; node.x = 6; node.y = 7; const attachArgs = []; const hooks = { node: () => node, base: () => node, allocate: () => 2, constructText: () => {}, kill: () => {}, push: () => {}, attach: (...v) => { attachArgs.push(v); }, styled: (_t, text) => { trace.push(`s:${text}`); }, scroll: () => {}, slider: () => {}, scrollable: () => {} };
    new HtmlBodyGump(new HtmlBodyGumpState(), hooks).setText(1, '\0abc'); assert.deepEqual([attachArgs.at(-1), trace.at(-1)], [[1, 6, 7, 0, 0], 's:']);
    // 파서가 아무것도 쓰지 않아 본문 첫 바이트가 0 이면 빈 본문이다. 이전 본문/링크의 첫 바이트는 파서 호출 전에 0 으로 쓰인다.
    const second = makeScene(0); second.body.construct(1); second.node.body[0] = 9; second.node.links[0] = 9; const seen = []; second.body.setHtml(1, 'x', (_input, output, links) => { seen.push([output[0], links[0]]); });
    assert.deepEqual(seen, [[0, 0]]); assert.ok(second.trace.includes(`styled:${hex('')}`));
    // 부착은 기본 부착 뒤 현재 자식을 여백 0 으로 다시 맞춘다.
    const third = makeScene(0); third.body.construct(1); third.body.setHtml(1, 'hi', third.parse); const calls = []; const wide = new HtmlBodyGump(new HtmlBodyGumpState(), { node: () => third.node, base: () => third.child, baseAttach: (...v) => { calls.push(`b:${v}`); }, push: () => { calls.push('push'); }, attach: (_t, x, y, p, m) => { calls.push(`a:${x},${y},${p},${m}`); } });
    third.node.baseWidth = 200; third.node.baseHeight = 90; wide.attach(1, 3, 4, 5, 6); assert.deepEqual(calls, ['b:1,3,4,5,6', 'push', 'a:0,0,1,0']); assert.deepEqual([third.child.baseWidth, third.child.baseHeight], [200, 90]);
});
