// HUD divider 입력(handle)과 프레임 방송(broadcast)을 원본 10.78 독립 관측 13,398행(독립 6,699입력의 두 정밀도)의 같은 raw 입력·최종 상태(flags/기준점/focus)·장치 호출·최종 Push/Attach 인자에 대조한다. 원본게임을 실행하지 않는다.
// 앞 14열만 입력으로 쓰고 뒤 6열의 관측은 비교에만 쓴다(cpppj `HudDividerInputTests` 와 같은 방식). (2026-11-15 WB04-E471)
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import { GumpCreateNode, GumpCreateState } from '../www/ui/gump-create.js';
import { GumpCollection } from '../www/render/gump-collection.js';
import { GumpLifetimeState } from '../www/ui/gump-lifetime.js';
import { GumpEventState, GumpEvents } from '../www/ui/gump-events.js';
import { GumpInputState, GumpInput } from '../www/ui/gump-input.js';
import { GumpTree, GumpTreeState } from '../www/ui/gump-tree.js';
import { HudDividerNode } from '../www/ui/hud-dock.js';
import { HudDividerInput } from '../www/ui/hud-divider-input.js';

// 열 수와 원본 행 수, native 포인터와 구별하는 안정된 검사 토큰이다.
const COLUMNS = 20; const NATIVE_ROWS = 13398; const DIVIDER = 1; const PARENT = 2; const GRANDPARENT = 3; const OTHER = 4;
// 음수도 하위 DWORD 비트로 읽는 입력 숫자와 signed 입력, float32 비트 변환이다.
const unsigned = (text) => Number(BigInt.asUintN(32, BigInt(text)));
const signed = (text) => Number(BigInt.asIntN(32, BigInt(text)));
const floatBits = (value) => { const view = new DataView(new ArrayBuffer(4)); view.setFloat32(0, value, true); return view.getUint32(0, true); };
const floatValue = (bits) => { const view = new DataView(new ArrayBuffer(4)); view.setUint32(0, bits >>> 0, true); return view.getFloat32(0, true); };
// 실제 콜백 인자만 구분자 목록으로 기록한다.
const packed = (values) => values.map((value) => value >>> 0).join(':');

// 제품 공개 경계에서 직접 관측할 수 없는 native 내부 함수 진입 표식만 제외한다.
function observableTrace(text) {
    if (text === '-') return '';
    return text.split(';').filter((item) => !(item === 'local' || item.startsWith('localout:') || item.startsWith('anchor:') || item.startsWith('capture:') || item.startsWith('move:'))).join(';');
}

// 기대 수식을 계산하지 않고 같은 공통 노드/위치/입력/이동에 입력을 재생한다.
class DividerScene {
    // 제품과 공통 Gump 가 동시에 읽는 같은 파생 노드, native 일반 부모 체인과 기존 capture 에 대응하는 노드, 공통 상태와 호출 순서 기록이다.
    divider = new HudDividerNode(); parent = new GumpCreateNode(); grandparent = new GumpCreateNode(); other = new GumpCreateNode();
    create = new GumpCreateState(); lifetime = new GumpLifetimeState(); eventsState = new GumpEventState(); inputState = new GumpInputState(); treeState = new GumpTreeState();
    menuCount = 0; pollCode = 0; baseResult = 0; trace = '';

    // 앞 14열만 입력으로 설치한다. 뒤 6열의 관측을 상태 준비에 사용하지 않는다.
    constructor(row) {
        this.divider.axis = unsigned(row[5]); this.divider.flags = unsigned(row[6]);
        const limits = row[7].split(':'); const geometry = row[8].split(':'); const parents = row[9].split(':');
        if (limits.length !== 4 || geometry.length !== 4 || parents.length !== 3) throw new Error('Divider fixture 입력 열 개수');
        this.divider.minX = signed(limits[0]); this.divider.maxX = signed(limits[1]); this.divider.minY = signed(limits[2]); this.divider.maxY = signed(limits[3]);
        this.divider.x = floatValue(unsigned(geometry[0])); this.divider.y = floatValue(unsigned(geometry[1])); this.divider.positioning[0] = unsigned(geometry[2]); this.divider.positioning[1] = unsigned(geometry[3]);
        const depth = unsigned(parents[0]);
        this.divider.parent = depth ? PARENT : 0; this.parent.parent = depth === 2 ? GRANDPARENT : 0; this.parent.x = floatValue(unsigned(parents[1])); this.parent.y = floatValue(unsigned(parents[2]));
        this.lifetime.focus = unsigned(row[10]) === 1 ? DIVIDER : unsigned(row[10]) === 2 ? OTHER : 0;
        this.pollCode = unsigned(row[11]); this.menuCount = unsigned(row[12]); this.baseResult = unsigned(row[13]);
        const node = (token) => this.base(token);
        this.collection = new GumpCollection(this.create.collection, { node });
        this.events = new GumpEvents(this.create, this.lifetime, this.eventsState, this.collection, { node });
        this.input = new GumpInput(this.create, this.lifetime, this.inputState, this.collection, this.events, { node });
        this.tree = new GumpTree(this.create, this.treeState, this.treeHooks());
        this.handler = new HudDividerInput(this.collection, this.input, this.tree, this.inputHooks());
    }

    // 같은 네 노드의 기본 주소를 공급한다.
    base(token) { return token === DIVIDER ? this.divider : token === PARENT ? this.parent : token === GRANDPARENT ? this.grandparent : token === OTHER ? this.other : undefined; }

    // 호출 순서를 기록한다.
    append(item) { this.trace += (this.trace ? ';' : '') + item; }

    // 원문의 stub 과 동일하게 최종 링크/좌표를 바꾸지 않고 실제 콜백 인자만 읽는다.
    treeHooks() {
        return { node: (token) => this.base(token), redraw: () => {}, push: () => { this.append('detach'); }, kill: () => {},
            attach: (token, x, y, destination, mode) => { this.append(`attach:${packed([floatBits(x), floatBits(y), destination, mode])}`); } };
    }

    // 실제 폴링/기본 Help/커서/현재 메뉴 수의 경계만 기록한다.
    inputHooks() {
        return { divider: (token) => (token === DIVIDER ? this.divider : null), base: (token) => this.base(token),
            baseHandle: (token, event, first, second) => { this.append(`base:${packed([event, first, second])}`); return this.baseResult; },
            cursor: (cursor) => { this.append(`cursor:${cursor}`); }, menuInstances: () => { this.append(`modal:${this.menuCount}`); return this.menuCount; },
            poll: (code) => { this.append(`poll:${code}`); return { code: this.pollCode }; }, cursorEvent: 0x0200003b };
    }

    // 제품의 실제 flags/anchor/focus 를 native 의 0/self/other 표현으로 읽는다.
    state() {
        const focus = this.lifetime.focus === DIVIDER ? 1 : this.lifetime.focus === OTHER ? 2 : 0;
        return packed([this.divider.flags, this.divider.positioning[0], this.divider.positioning[1], focus]);
    }
}

// 탭 구분 fixture 의 머리글·주석을 건너뛴 행 목록이다.
function rows(name) {
    return fs.readFileSync(new URL(`../../cpppj/tests/fixtures/${name}`, import.meta.url), 'utf8').split(/\r?\n/).filter((line) => line && !line.startsWith('#') && !line.startsWith('name\t')).map((line) => line.split('\t'));
}

test('HudDividerInput 원본 10.78 13,398행: 입력/방송의 최종 flags·기준점·focus 상태와 장치 호출·최종 Push/Attach 인자가 모두 일치한다', () => {
    const table = rows('hud-divider-input-x86.tsv'); const failures = [];
    assert.equal(table.length, NATIVE_ROWS);
    for (const row of table) {
        assert.equal(row.length, COLUMNS, row[0]);
        const scene = new DividerScene(row); const event = unsigned(row[2]); const first = signed(row[3]) >>> 0; const second = signed(row[4]) >>> 0; const problems = [];
        if (row[14] !== '1' || row[18] !== '0' || !(unsigned(row[19]) > 0)) problems.push('range');
        if (row[1] === 'handle') { if (scene.handler.handle(DIVIDER, event, first, second) !== unsigned(row[15])) problems.push('result'); }
        else if (row[1] === 'broadcast') scene.handler.broadcast(DIVIDER, event, first, second);
        else problems.push('operation');
        if (scene.state() !== row[16]) problems.push('state');
        if (scene.trace !== observableTrace(row[17])) problems.push('trace');
        if (problems.length) failures.push(`${row[0]}/${row[1]}: ${problems.join(',')}`);
    }
    assert.deepEqual(failures.slice(0, 20), []);
});

// 원본 관측이 닿지 않는 독립 시험용 입력 열이다. 부모가 없는 divider(노드 위치 0)와 모든 제한 0 에서 시작하며 필요한 값은 장면에서 직접 바꾼다.
function plainRow(operation, event, first, second, axis, flags, pollCode = 0, menuCount = 0) {
    const zero = floatBits(0);
    return ['plain', operation, String(event), String(first), String(second), String(axis), String(flags), '0:0:0:0', `${zero}:${zero}:0:0`, '0:0:0', '0', String(pollCode), String(menuCount), '73', '1', '0', '', '-', '0', '1'];
}
// 장치 호출 trace 에서 외부 호출 이름만 순서대로 뽑는다.
const names = (trace) => (trace ? trace.split(';').map((item) => item.split(':')[0]) : []);

test('HudDividerInput 생성/공유 노드 검사: 공유 조회 둘이 없으면 TypeError 이고 토큰 0/조회 실패/기본 주소 불일치는 Error 이다', () => {
    const scene = new DividerScene(plainRow('handle', 0x10000, 5, 6, 1, 0)); const hooks = scene.inputHooks();
    assert.throws(() => new HudDividerInput(scene.collection, scene.input, scene.tree, { ...hooks, divider: undefined }), TypeError);
    assert.throws(() => new HudDividerInput(scene.collection, scene.input, scene.tree, { ...hooks, base: undefined }), TypeError);
    assert.throws(() => scene.handler.handle(0, 0x10000, 5, 6), /공유 노드 없음/); assert.throws(() => scene.handler.broadcast(0, 0, 5, 6), /공유 노드 없음/);
    assert.throws(() => scene.handler.handle(PARENT, 0x10000, 5, 6), /공유 노드 없음/);
    // 파생 노드와 공통 기본 노드가 다른 객체이면 거부한다.
    const split = new HudDividerInput(scene.collection, scene.input, scene.tree, { ...hooks, base: () => new GumpCreateNode() });
    assert.throws(() => split.handle(DIVIDER, 0x10000, 5, 6), /공유 노드 없음/);
});

test('HudDividerInput 입력: 좌클릭 누름은 flags 에서 기준점/방송 비트만 더해 로컬 위치를 저장하고 첫 캡처만 유지하며 release 는 방송 비트만 지운다', () => {
    const press = new DividerScene(plainRow('handle', 0x10000, 40, 50, 1, 0));
    assert.equal(press.handler.handle(DIVIDER, 0x10000, 40, 50), 1); assert.equal(press.divider.flags, 0x110); assert.equal(press.lifetime.focus, DIVIDER);
    assert.deepEqual([press.divider.positioning[0], press.divider.positioning[1]], [floatBits(40), floatBits(50)]);
    // 이미 다른 노드가 캡처 중이면 focus 를 바꾸지 않는다(위치는 새로 저장).
    const held = new DividerScene(plainRow('handle', 0x10000, 40, 50, 2, 0)); held.lifetime.focus = OTHER; held.handler.handle(DIVIDER, 0x10000, 7, 8);
    assert.equal(held.lifetime.focus, OTHER); assert.deepEqual([held.divider.positioning[0], held.divider.positioning[1]], [floatBits(7), floatBits(8)]);
    // 축 비트(1|2)가 없으면 누름은 기본 Help 로 간다. release 는 축과 무관하게 방송 비트만 지운다.
    const none = new DividerScene(plainRow('handle', 0x10000, 1, 2, 0, 0x1ff)); assert.equal(none.handler.handle(DIVIDER, 0x10000, 1, 2), 73); assert.equal(none.divider.flags, 0x1ff);
    assert.equal(none.handler.handle(DIVIDER, 0x80010000, 1, 2), 1); assert.equal(none.divider.flags, 0xff);
    // 다른 클릭 코드나 release 비트 단독 사건은 기본 Help 로 넘기며 상태를 바꾸지 않는다. 사건 코드는 상위 비트를 제외하고 비교한다.
    const other = new DividerScene(plainRow('handle', 0x20000, 1, 2, 3, 0x100)); assert.equal(other.handler.handle(DIVIDER, 0x20000, 1, 2), 73); assert.equal(other.divider.flags, 0x100);
    assert.equal(other.handler.handle(DIVIDER, 0x04010000, 1, 2), 1); assert.equal(other.divider.flags, 0x110);
});

test('HudDividerInput hover 커서: 기본 hover 사건에서 세로(2)가 가로(1)보다 우선해 커서 5/4 를 요청하고 축이 없으면 기본 Help, 장치가 없으면 도달한 분기에서 TypeError', () => {
    const hover = 0x0200003b;
    for (const [axis, cursor] of [[1, 4], [2, 5], [3, 5]]) { const scene = new DividerScene(plainRow('handle', hover, 1, 2, axis, 0)); assert.equal(scene.handler.handle(DIVIDER, hover, 1, 2), 1); assert.equal(scene.trace, `cursor:${cursor}`); }
    const none = new DividerScene(plainRow('handle', hover, 1, 2, 0, 0)); assert.equal(none.handler.handle(DIVIDER, hover, 1, 2), 73); assert.equal(none.trace, `base:${packed([hover, 1, 2])}`);
    // cursorEvent 를 주지 않으면 기본값(0200003b)을 쓴다.
    const vertical = new DividerScene(plainRow('handle', hover, 1, 2, 2, 0)); const hooks = { ...vertical.inputHooks() }; delete hooks.cursorEvent;
    const defaulted = new HudDividerInput(vertical.collection, vertical.input, vertical.tree, hooks); assert.equal(defaulted.handle(DIVIDER, hover, 1, 2), 1); assert.equal(vertical.trace, 'cursor:5');
    for (const [axis, message] of [[2, /세로 커서/], [1, /가로 커서/]]) { const bare = new DividerScene(plainRow('handle', hover, 1, 2, axis, 0)); const missing = new HudDividerInput(bare.collection, bare.input, bare.tree, { ...bare.inputHooks(), cursor: undefined }); assert.throws(() => missing.handle(DIVIDER, hover, 1, 2), message); }
    const noHelp = new DividerScene(plainRow('handle', 0x20000, 1, 2, 0, 0)); assert.throws(() => new HudDividerInput(noHelp.collection, noHelp.input, noHelp.tree, { ...noHelp.inputHooks(), baseHandle: undefined }).handle(DIVIDER, 0x20000, 1, 2), /기본 Help/);
});

test('HudDividerInput 방송: 메뉴가 열려 있으면 폴링 없이 반환하고 폴링 release 비트(0x80000000)만 방송 비트를 지우며 축이 없으면 이동 없이 끝난다', () => {
    const open = new DividerScene(plainRow('broadcast', 0, 100, 100, 3, 0x100, 0x80000000, 2)); open.handler.broadcast(DIVIDER, 0, 100, 100); assert.deepEqual(names(open.trace), ['modal']); assert.equal(open.divider.flags, 0x100);
    // release 비트 하나(0x80000000)만 지우며 하위 비트만 있는 폴링 코드(1)는 지우지 않는다.
    const low = new DividerScene(plainRow('broadcast', 0, 100, 100, 2, 0x100, 1, 0)); low.handler.broadcast(DIVIDER, 0, 100, 100); assert.equal(low.divider.flags, 0x100);
    const high = new DividerScene(plainRow('broadcast', 0, 100, 100, 2, 0x100, 0x80000000, 0)); high.handler.broadcast(DIVIDER, 0, 100, 100); assert.equal(high.divider.flags, 0);
    // 축 1|2 가 모두 없으면 폴링 뒤 Move 없이 반환한다.
    const idle = new DividerScene(plainRow('broadcast', 0, 100, 100, 0, 0, 0, 0)); idle.handler.broadcast(DIVIDER, 0, 100, 100); assert.deepEqual(names(idle.trace), ['modal', 'poll']);
    // 장치가 없으면 도달한 위치에서 TypeError 이다.
    const bare = new DividerScene(plainRow('broadcast', 0, 100, 100, 2, 0, 0, 0)); const hooks = bare.inputHooks();
    assert.throws(() => new HudDividerInput(bare.collection, bare.input, bare.tree, { ...hooks, menuInstances: undefined }).broadcast(DIVIDER, 0, 1, 2), /메뉴 인스턴스/);
    assert.throws(() => new HudDividerInput(bare.collection, bare.input, bare.tree, { ...hooks, poll: undefined }).broadcast(DIVIDER, 0, 1, 2), /폴링/);
});

test('HudDividerInput 방송 이동: 마우스 좌표에서 2 를 빼고 제한 비트가 있을 때만 정수 제한에 맞추며 제한이 선택되면 float32 로 줄인 제한값을 쓴다', () => {
    // 세로 축(2): y = 마우스 y − 2, x = 저장된 로컬 x + 노드 x. 제한은 축 8 이 있을 때만 적용한다.
    const run = (axis, first, second, limits) => {
        const scene = new DividerScene(plainRow('broadcast', 0, first, second, axis, 0)); Object.assign(scene.divider, limits); scene.divider.x = 3; scene.divider.y = 5; scene.divider.positioning[0] = floatBits(10); scene.divider.positioning[1] = floatBits(20);
        scene.handler.broadcast(DIVIDER, 0, first >>> 0, second >>> 0); return scene.trace.split(';').find((item) => item.startsWith('attach:')) ?? '';
    };
    const bits = (x, y) => `attach:${packed([floatBits(x), floatBits(y), 0, 0])}`;
    assert.equal(run(2, 0, 100, { minY: 0, maxY: 50 }), bits(13, 98)); assert.equal(run(10, 0, 100, { minY: 0, maxY: 50 }), bits(13, 50)); assert.equal(run(10, 0, -20, { minY: 0, maxY: 50 }), bits(13, 0));
    // 가로 축(1): x = 마우스 x − 2, y = 저장된 로컬 y + 노드 y. 제한은 축 4 가 있을 때만 적용한다.
    assert.equal(run(1, 100, 0, { minX: 0, maxX: 50 }), bits(98, 25)); assert.equal(run(5, 100, 0, { minX: 0, maxX: 50 }), bits(50, 25)); assert.equal(run(5, -20, 0, { minX: 0, maxX: 50 }), bits(0, 25));
    // 제한 정수가 float32 로 정확하지 않으면 제한이 선택될 때 줄인 값을 쓴다(16777217 → 16777216).
    assert.equal(run(5, 20000000, 0, { minX: 0, maxX: 16777217 }), bits(16777216, 25)); assert.equal(run(5, -20000000, 0, { minX: 16777217, maxX: 20000000 }), bits(16777216, 25));
    // 두 축이 모두 있으면 세로 축이 우선한다.
    assert.equal(run(3, 100, 100, {}), bits(13, 98));
});
