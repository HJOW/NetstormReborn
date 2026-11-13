// 같은 실제 노드/위치/수명으로 이벤트257·입력232·프레임270개의 원본 관측을 재생한다. (2026-11-14 WB04-E413)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put } from './raw-support.mjs';
import { decodeGumpNode, encodeGumpNode } from './gump-node-support.mjs';
import { GumpCreateState, GumpCreateNode, GumpCreate } from '../www/ui/gump-create.js';
import { GumpTree, GumpTreeState } from '../www/ui/gump-tree.js';
import { GumpLifetimeState, GumpLifetime } from '../www/ui/gump-lifetime.js';
import { GumpEventState, GumpEvents } from '../www/ui/gump-events.js';
import { GumpInputState, GumpInput } from '../www/ui/gump-input.js';
import { GumpFrame } from '../www/ui/gump-frame.js';
import { GumpCollection } from '../www/render/gump-collection.js';

// 보호10.78 판본의 독립 관측이다. 다른 판본 행은 이식 범위 밖이다.
const eventRows = readRows(path.join(REPO_ROOT, 'cpppj/tests/fixtures/gumpevents-x86.tsv')); const inputRows = readRows(path.join(REPO_ROOT, 'cpppj/tests/fixtures/gumpinput-x86.tsv')); const frameRows = readRows(path.join(REPO_ROOT, 'cpppj/tests/fixtures/gumpframe-x86.tsv'));
// 같은 토큰 순서의 전체160BYTE만 관찰한다.
const nodeSnapshot = (nodes) => [...nodes].sort(([a], [b]) => a - b).map(([token, node]) => `${token}=${encodeGumpNode(node)}`).join(';');
// 원문 raw 응답은 같은 안정 노드의 지정 DWORD만 바꾼다.
function patchNode(nodes, token, at, value) { const node = nodes.get(token); const raw = Uint8Array.from(Buffer.from(encodeGumpNode(node), 'hex')); put(raw, at, value); Object.assign(node, decodeGumpNode(Buffer.from(raw).toString('hex'))); }
// 입력은 완성 노드 바이트이며 필터/선택 결과를 계산하지 않는다.
function loadNodes(nodes, text) { for (const item of text.split(';')) { const [token, raw] = item.split('='); nodes.set(Number(token), decodeGumpNode(raw)); } }
// 표의 연속 구간만 기록한다. 기대 프레임 대상은 관측 열에서 읽는다.
function rle(table) { const out = []; for (let at = 0; at < table.length;) { let end = at + 1; while (end < table.length && table[end] === table[at]) end += 1; out.push(`${end - at}:${table[at]}`); at = end; } return out.join(';'); }
// 입력 RLE를 정확한 길이의 표에 푼다.
function loadTable(table, text) { let at = 0; for (const item of text.split(';')) { const [count, value] = item.split(':').map(Number); assert.ok(at + count <= table.length); table.fill(value, at, at + count); at += count; } assert.equal(at, table.length); }

// 이벤트/입력의 같은 전역·노드와 명시 반환/raw 응답 장치다.
class EventScene {
    // 실제 현재 생성/수명/선택/입력 상태와 안정된 노드 Map이다.
    create = new GumpCreateState(); life = new GumpLifetimeState(); event = new GumpEventState(); input = new GumpInputState(); nodes = new Map();
    // 명시 응답/투영과 가상 호출 당시 기록이다.
    responses = new Map(); anchors = new Map(); calls = []; inputMode = false;
    // 원문 순서의 현재DWORD 전역이며 입력 장면만 뒤4개를 추가한다.
    globals() { return [this.create.entries[0], this.life.currentDialog, this.event.point.x >>> 0, this.event.point.y >>> 0, this.event.mask, this.event.bestDepth, this.event.selected, ...(this.inputMode ? [this.life.focus, this.create.collection.overrideHead, this.life.dialogCount, this.input.overlayEvent] : [])].join(','); }
    // 명시 전역 index만 같은 저장소에 적용한다.
    global(index, value) { value >>>= 0; if (index === 0) this.create.entries[0] = this.create.collection.root = value; else if (index === 1) this.life.currentDialog = value; else if (index === 2) this.event.point.x = value | 0; else if (index === 3) this.event.point.y = value | 0; else if (index === 4) this.event.mask = value; else if (index === 5) this.event.bestDepth = value; else if (index === 6) this.event.selected = value; else if (index === 7) this.life.focus = value; else if (index === 8) this.create.collection.overrideHead = value; else if (index === 9) this.life.dialogCount = value; else if (index === 10) this.input.overlayEvent = value; else throw new Error('이벤트 전역 index 오류'); }
    // 입력의 두 표는 미사용 칸까지 원문 순서로 기록한다.
    tables() { return `${[...this.life.overlay].join(',')};${[...this.life.dialogs].join(',')}`; }
    // 콜백 직전 전체 상태/인자를 기록한 뒤 명시 응답만 적용한다.
    respond(kind, token, args = []) {
        const key = `${kind},${token}`; this.calls.push(`${[kind, token, ...args].join(',')}@${this.globals()}@${nodeSnapshot(this.nodes)}${this.inputMode ? `@${this.tables()}` : ''}`); const response = this.responses.get(key); if (!response) return 0;
        // 부모/형제/후보 기대값을 계산하지 않고 입력 DWORD만 쓴다.
        for (const [target, at, value] of response.patches) { if (!target) this.global(at, value); else if (this.inputMode && target === 1) this.life.overlay[at] = value; else if (this.inputMode && target === 2) this.life.dialogs[at] = value; else patchNode(this.nodes, target, at, value); }
        return response.value;
    }
    // 같은 원본 위치/경계/캐시 계산기에 명시 정수 투영 응답만 연결한다.
    collection() { return new GumpCollection(this.create.collection, { node: (token) => this.nodes.get(token), project: (sid) => { this.respond('Project', sid); return this.anchors.get(sid); } }); }
    // 실제 기본 필터 또는 명시 가상 필터와 같은 처리기 응답을 공급한다.
    eventHooks(base = false) { return { node: (token) => this.nodes.get(token), ...(base ? {} : { filter: (token, e, x, y) => this.respond('Filter', token, [e, x, y]) !== 0 }), handle: (token, e, x, y) => this.respond('Handle', token, [e, x, y]) }; }
    // 캡처/overlay 직접 호출도 같은 가상 처리기 장치를 쓴다.
    inputHooks() { return { node: (token) => this.nodes.get(token), handle: (token, e, x, y) => this.respond('Handle', token, [e, x, y]), overflow: (code) => { this.respond('Overflow', code); } }; }
}

// 이벤트/입력의 동일한 행 구조에서 명시 입력 연산만 실제 처리기로 재생한다.
function replayEvents(rows, inputMode, count) {
    const originals = rows.filter((r) => r[0] === 'originals'); assert.equal(originals.length, count);
    // 행 사이만 독립 저장소이고 연속 연산/콜백은 같은 현재 상태다.
    for (const row of originals) {
        assert.equal(row.length, inputMode ? 16 : 14); const s = new EventScene(); s.inputMode = inputMode; row[2].split(',').map(Number).forEach((value, index) => s.global(index, value)); loadNodes(s.nodes, row[3]); const offset = inputMode ? 1 : 0;
        if (inputMode) { const parts = row[4].split(';'); s.life.overlay.set(parts[0].split(',').map(Number)); s.life.dialogs.set(parts[1].split(',').map(Number)); assert.equal(s.tables(), row[4]); }
        assert.equal(s.globals(), row[2]); assert.equal(nodeSnapshot(s.nodes), row[3]);
        // 중복 응답 키는 마지막 입력으로 덮어쓴다.
        if (row[6 + offset] !== '-') for (const item of row[6 + offset].split(';')) { const [key, text] = item.split(':'); const [kind, token, value] = key.split(','); s.responses.set(`${kind},${token}`, { value: Number(value), patches: text === '-' ? [] : text.split('/').map((p) => p.split(',').map(Number)) }); }
        // 월드 투영점은 위치 결과가 아닌 독립 정수 장치 입력이다.
        if (row[7 + offset] !== '-') for (const item of row[7 + offset].split(';')) { const [sid, x, y] = item.split(',').map(Number); s.anchors.set(sid, { x: x | 0, y: y | 0 }); }
        const collection = s.collection(); const events = new GumpEvents(s.create, s.life, s.event, collection, s.eventHooks(row[5 + offset] === '1')); const input = new GumpInput(s.create, s.life, s.input, collection, events, s.inputHooks()); const lifetime = new GumpLifetime(s.create, s.life, { node: (token) => s.nodes.get(token) }); const results = [];
        // 반환만 기록하며 기대 선택/등록/전달은 계산하지 않는다.
        for (const item of row[4 + offset].split(';')) { const [op, ...args] = item.split(','); const a = args.map(Number); const token = a[0]; let result = '-';
            if (op === 'Base') result = events.baseFilter(token, a[1], a[2], a[3]) ? 1 : 0; else if (op === 'Top') result = events.pickTopLevel({ x: a[1] | 0, y: a[2] | 0 }); else if (op === 'Pick') result = events.pick({ x: a[1] | 0, y: a[2] | 0 }, a[3]); else if (op === 'Dispatch') result = events.dispatch(token, a[1], a[2], a[3], a[4] !== 0); else if (op === 'Bubble') result = events.bubble(token, a[1], a[2], a[3]);
            else if (op === 'Capture') input.capture(token); else if (op === 'Override') input.setOverride(token, a[1] !== 0); else if (op === 'Dialog') result = input.addDialog(a[1]) ? 1 : 0; else if (op === 'Remove') result = lifetime.removeDialog(a[1]) ? 1 : 0; else if (op === 'Route') result = input.route(a[1], a[2], a[3], a[4] !== 0); else if (op === 'Overlay') result = input.overlay(a[1], { x: a[2] | 0, y: a[3] | 0 }); else throw new Error(`이벤트 연산 오류 ${op}`); results.push(result);
        }
        assert.equal(nodeSnapshot(s.nodes), row[8 + offset], `노드 ${row[1]}`); assert.equal(s.globals(), row[9 + offset], `전역 ${row[1]}`); if (inputMode) assert.equal(s.tables(), row[11], `표 ${row[1]}`); assert.equal(results.join(','), row[inputMode ? 12 : 10], `반환 ${row[1]}`); assert.equal(s.calls.join('|') || '-', row[inputMode ? 13 : 11], `콜백 ${row[1]}`);
    }
}

test('GumpEvents_Native1078: 원본257행의 선택·전달 전체 노드/전역·반환·콜백 시점 상태가 같다', { skip: eventRows === null }, () => replayEvents(eventRows, false, 257));
test('GumpInput_Native1078: 원본232행의 캡처/dialog/overlay 전체 노드/전역/표·반환·콜백 상태가 같다', { skip: inputRows === null }, () => replayEvents(inputRows, true, 232));

test('GumpFrame_OriginalPatch: 원본270행의 현재 등록/idle·전체 노드·통지/재그리기 콜백 상태가 같다', { skip: frameRows === null }, () => {
    const originals = frameRows.filter((row) => row[0] === 'originals'); assert.equal(originals.length, 270);
    // 현재 표/개수 응답만 제공하며 예상 프레임 순서를 모형으로 만들지 않는다.
    for (const [index, row] of originals.entries()) {
        assert.equal(row.length, 11); assert.equal(Number(row[1]), index); const create = new GumpCreateState(); const life = new GumpLifetimeState(); const nodes = new Map(); loadNodes(nodes, row[3]); const parts = row[4].split('#'); create.count = Number(parts[0]); loadTable(create.entries, parts[1]); loadTable(life.idle, parts[2]); create.collection.root = create.entries[0]; const state = () => `${create.count}#${rle(create.entries)}#${rle(life.idle)}`; assert.equal(nodeSnapshot(nodes), row[3]); assert.equal(state(), row[4]); const responses = new Map(); const calls = [];
        // 응답은 명시된 개수/등록/idle/노드 위치만 가진다.
        if (row[5] !== '-') for (const item of row[5].split(';')) { const [key, values] = item.split(':'); responses.set(key, values ? values.split('/').map((p) => p.split(',').map(Number)) : []); }
        const respond = (kind, token, args = []) => { const key = `${kind},${token}`; calls.push(`${[kind, token, ...args].join(',')}@${state()}@${nodeSnapshot(nodes)}`); for (const [target, at, value] of responses.get(key) ?? []) { if (!target) { assert.equal(at, 0); create.count = value; } else if (target === 1) { create.entries[at] = value; create.collection.root = create.entries[0]; } else if (target === 2) life.idle[at] = value; else patchNode(nodes, target, at, value); } };
        const frame = new GumpFrame(create, life, { node: (token) => nodes.get(token), idle: (token) => respond('Idle', token), redraw: (token, value) => respond('Redraw', token, [value ? 1 : 0]), broadcast: (token, e, x, y) => respond('Broadcast', token, [e, x, y]) });
        // 최상위 공개 연산만 실행한다. void 반환의 우연한 EAX는 대조하지 않는다.
        for (const item of row[2].split(';')) { const [op, ...args] = item.split(','); const a = args.map(Number); if (op === 'Idle') frame.idle(); else if (op === 'RedrawNode') frame.redrawNode(a[0]); else if (op === 'RedrawRoots') frame.redrawRoots(); else if (op === 'Broadcast') frame.broadcast(a[1], a[2], a[3]); else throw new Error(`프레임 연산 오류 ${op}`); }
        assert.equal(nodeSnapshot(nodes), row[6], `노드 ${row[1]}`); assert.equal(state(), row[7], `표 ${row[1]}`); assert.equal(calls.join('|') || '-', row[8], `콜백 ${row[1]}`);
    }
});

// 실제 기본 생성/트리/지연 삭제를 같은 현재 노드에 연결한다. 가상 redraw는 관측하고 release는 실제 Map에서 제거한다.
function connected(flags = 1, frameOrder = false) {
    const s = new EventScene(); s.inputMode = true; const redraw = []; const freed = [];
    // 루트/직계 두 노드/손자 네 실제 저장소다.
    for (let token = 1; token <= 4; token += 1) s.nodes.set(token, new GumpCreateNode()); s.create.screenWidth = 64; s.create.screenHeight = 48; const factory = new GumpCreate(s.create, { node: (token) => s.nodes.get(token), allocate: () => 1 }); factory.initializeRoot(); factory.construct(2, 11, 20, 16, flags); factory.construct(3, 12, 20, 16, flags); factory.construct(4, 13, 8, 6, flags); let tree; let lifetime;
    lifetime = new GumpLifetime(s.create, s.life, { node: (token) => s.nodes.get(token), push: (token) => tree.push(token), deletingDestructor: (token, bits) => lifetime.deleteScalar(token, bits), release: (token) => { freed.push(token); s.nodes.delete(token); } }); tree = new GumpTree(s.create, new GumpTreeState(), { node: (token) => s.nodes.get(token), redraw: (token, erase) => redraw.push([token, erase]), push: (token) => tree.push(token), attach: (...args) => tree.attach(...args), kill: (token) => lifetime.kill(token) });
    if (frameOrder) { tree.attach(3, 0, 0, 1, 1); tree.attach(2, 0, 0, 1, 1); tree.attach(4, 0, 0, 2, 1); s.nodes.get(2).flags |= 0x20; }
    else { tree.attach(2, 10, 10, 1, 0); tree.attach(3, 10, 10, 1, 0); tree.attach(4, 1, 1, 2, 0); }
    return { s, tree, lifetime, factory, redraw, freed, collection: s.collection() };
}

// 실제 위치/이벤트를 캡처·dialog/overlay 입력에 연결한다. 같은 +28 처리기를 양쪽에 공급한다.
function inputPair(s, collection, eventHooks = s.eventHooks(true), directHooks = null) { const events = new GumpEvents(s.create, s.life, s.event, collection, eventHooks); const input = new GumpInput(s.create, s.life, s.input, collection, events, directHooks ?? { ...s.inputHooks(), handle: eventHooks.handle }); return { events, input }; }

test('GumpEvents 실제 생성/트리/수명: 직계/전체 선택·캐시와 지연 삭제 뒤의 dialog/후속 선택이 같은 노드를 읽는다', () => {
    const { s, lifetime, collection } = connected(); const handled = []; const hooks = s.eventHooks(true); hooks.handle = (token) => { handled.push(token); return 7; }; const events = new GumpEvents(s.create, s.life, s.event, collection, hooks); Object.assign(s.nodes.get(3), { depth: 4 }); s.nodes.get(2).depth = 5; s.nodes.get(4).depth = 3;
    assert.equal(events.pickTopLevel({ x: 12, y: 12 }), 3); assert.equal(events.pick({ x: 12, y: 12 }, 0), 4); assert.deepEqual(s.nodes.get(4).cached, { x: 11, y: 11 }); s.nodes.get(3).flags |= 0x400; assert.equal(events.dispatch(1, 1, 2, 3), 3); assert.deepEqual(handled, [3]); s.life.dialogs[0] = 4; s.life.dialogCount = 1; s.life.currentDialog = 4; assert.equal(events.baseFilter(4, 0xffffffff, 999, 999), true); lifetime.kill(2); assert.equal(events.pick({ x: 12, y: 12 }, 0), 3); assert.deepEqual([s.life.currentDialog, s.life.killCount], [0, 2]);
});

test('GumpEvents 전달/전파 반환: visible만 제한하는 전달은 대상 토큰, Bubble은 처리기의 전체DWORD를 반환한다', () => {
    const { s, collection } = connected(); const hooks = s.eventHooks(); hooks.filter = () => true; hooks.handle = (token) => token === 3 ? 0x80000007 : 0; const events = new GumpEvents(s.create, s.life, s.event, collection, hooks); s.nodes.get(3).flags = 0x10009; assert.equal(events.dispatch(1, 1, 2, 3), 3); assert.notEqual(events.pick({ x: 12, y: 12 }, 0), 3); s.nodes.get(3).flags = 0; assert.equal(events.dispatch(1, 1, 2, 3), 0); assert.equal(events.dispatch(1, 1, 2, 3, true), 3); hooks.handle = (token) => token === 2 ? 0x80000007 : 0; assert.equal(new GumpEvents(s.create, s.life, s.event, collection, hooks).bubble(4, 1, 2, 3), 0x80000007);
});

test('GumpEvents 기본 필터: help의 반개방 경계·키보드/마우스·현재 dialog를 구별한다', () => {
    const { s, collection } = connected(); const events = new GumpEvents(s.create, s.life, s.event, collection, s.eventHooks(true)); s.nodes.get(2).flags |= 0xe00; assert.equal(events.baseFilter(2, 0x4010000, 10, 10), true); assert.equal(events.baseFilter(2, 0x10000, 30, 10), false); assert.equal(events.baseFilter(2, 0x20000, 10, 26), false); assert.equal(events.baseFilter(2, 0x10000, 9, 10), false); assert.equal(events.baseFilter(2, 1, 0, 0), true); assert.equal(events.baseFilter(2, 0x2000001, 999, 999), true); s.life.currentDialog = 2; assert.equal(events.baseFilter(2, 0x10000, 999, 999), true);
});

test('GumpEvents 계약: 없는 토큰/처리기와 부모·형제·자식 순환을 거부한다', () => {
    const { s, collection } = connected(); const events = new GumpEvents(s.create, s.life, s.event, collection, s.eventHooks(true)); assert.throws(() => new GumpEvents(s.create, s.life, s.event, collection), /저장소/); assert.throws(() => events.dispatch(99, 1, 2, 3), /토큰/); const hooks = s.eventHooks(); delete hooks.handle; assert.throws(() => new GumpEvents(s.create, s.life, s.event, collection, hooks).bubble(2, 1, 2, 3), /처리기/); s.nodes.get(2).parent = 2; assert.throws(() => events.bubble(2, 1, 2, 3), /순환/); s.nodes.get(2).parent = 1; s.nodes.get(3).next = 3; assert.throws(() => events.pickTopLevel({ x: 12, y: 12 }), /순환/); s.nodes.get(3).next = 2; s.nodes.get(4).child = 2; assert.throws(() => events.dispatch(2, 1, 2, 3), /순환/);
});

test('GumpEvents 예외: 가상 필터가 쓰고 중단한 현재 GT/dialog를 보존한다', () => {
    const { s, collection } = connected(); const hooks = s.eventHooks(); hooks.filter = (token) => { s.nodes.get(token).command = 999; s.life.currentDialog = token; throw new Error('필터 실패'); }; assert.throws(() => new GumpEvents(s.create, s.life, s.event, collection, hooks).dispatch(3, 1, 2, 3), /필터 실패/); assert.deepEqual([s.nodes.get(3).command, s.life.currentDialog], [999, 3]);
});

test('GumpInput 실제 캡처/dialog/삭제/overlay: 같은 수명/트리 선택을 전달하며 override의 설정/해제를 보존한다', () => {
    const { s, lifetime, collection } = connected(); s.nodes.get(2).flags |= 0x400; s.nodes.get(3).flags |= 0x400; const hooks = s.eventHooks(true); hooks.handle = () => 0x80000007; const { events, input } = inputPair(s, collection, hooks); assert.equal(input.route(1, 2, 3), 3); input.capture(4); input.capture(2); assert.equal(s.life.focus, 4); assert.equal(input.route(1, 2, 3), 4); assert.equal(s.life.focus, 0); assert.equal(input.addDialog(2), true); assert.equal(input.route(1, 2, 3), 2); assert.equal(input.addDialog(3), true); assert.equal(input.addDialog(2), false); assert.equal(s.life.currentDialog, 3); lifetime.removeDialog(3); assert.equal(s.life.currentDialog, 2); input.capture(4); lifetime.kill(2); assert.deepEqual([s.life.focus, s.life.currentDialog, s.life.killCount, s.nodes.get(3).next], [0, 0, 2, 0]); assert.equal(input.route(1, 2, 3), 3); assert.equal(events.pick({ x: 12, y: 12 }, 0), 3); lifetime.setOverlay(3, true); s.input.overlayEvent = 123; assert.equal(input.overlay(999, { x: 12, y: 12 }), 0x80000007); input.setOverride(3, true); assert.equal(s.create.collection.overrideHead, 3); input.setOverride(2, true); assert.equal(s.create.collection.overrideHead, 0); lifetime.setOverlay(3, false);
});

test('GumpInput 현재 캡처: 실패 후 재전달을 금하고 성공 후 현재 캡처를 반환/해제한다', () => {
    const { s, collection } = connected(); const handled = []; let filters = 0; let success = false; const events = s.eventHooks(); events.filter = () => { filters += 1; return true; }; const direct = s.inputHooks(); direct.handle = (token) => { handled.push(token); s.life.focus = success ? 3 : 2; return success ? 7 : 0; }; const { input } = inputPair(s, collection, events, direct); input.capture(4); assert.equal(input.route(1, 2, 3), 0); assert.deepEqual([s.life.focus, filters, handled], [2, 0, [4]]); success = true; assert.equal(input.route(1, 2, 3), 3); assert.deepEqual([s.life.focus, handled], [0, [4, 2]]);
});

test('GumpInput overlay: 현재 부모 visible·전체Pick·별도 사건/전체 반환·첫0반환 종료를 구별한다', () => {
    const { s, collection } = connected(); s.nodes.get(2).depth = 1; s.nodes.get(3).depth = 5; s.nodes.get(4).depth = 2; s.life.overlay[0] = 2; s.input.overlayEvent = 123; let calls = 0; let received = 0; let value = 0x80000007; const direct = s.inputHooks(); direct.handle = (token, event) => { calls += 1; received = event; return value; }; const { input } = inputPair(s, collection, s.eventHooks(), direct); assert.equal(input.overlay(999, { x: 12, y: 12 }), value); assert.deepEqual([calls, received], [1, 123]); s.nodes.get(1).flags = 0; s.nodes.get(2).cached.x = s.nodes.get(2).cached.y = 99; assert.equal(input.overlay(999, { x: 12, y: 12 }), 0); assert.equal(s.nodes.get(2).cached.x, 99); s.nodes.get(1).flags = 0x10001; assert.equal(input.overlay(999, { x: 12, y: 12 }), value); s.nodes.get(2).flags = 9; assert.equal(input.overlay(999, { x: 12, y: 12 }), 0); s.nodes.get(2).flags = 1; s.nodes.get(2).depth = 10; assert.equal(input.overlay(999, { x: 12, y: 12 }), 0); assert.equal(s.event.selected, 4); s.life.overlay[99] = 4; assert.equal(input.overlay(999, { x: 12, y: 12 }), value); assert.equal(input.overlay(999, { x: 19, y: 12 }), 0); s.nodes.get(2).depth = 1; s.life.overlay[99] = 2; value = 0; const before = calls; assert.equal(input.overlay(999, { x: 12, y: 12 }), 0); assert.equal(calls, before + 1);
});

test('GumpInput dialog 한도: 중복보다 먼저 로그를 부르고 미사용 뒤 칸을 중복으로 세지 않는다', () => {
    const { s, collection } = connected(); let overflows = 0; const hooks = s.inputHooks(); hooks.overflow = (code) => { assert.equal(code, 19); overflows += 1; s.life.currentDialog = 4; }; const { input } = inputPair(s, collection, s.eventHooks(), hooks); s.life.dialogs.fill(3); s.life.dialogCount = 16; assert.equal(input.addDialog(3), false); assert.deepEqual([overflows, s.life.dialogCount, s.life.currentDialog], [1, 16, 4]); s.life.dialogCount = 15; s.life.dialogs[15] = 2; assert.equal(input.addDialog(2), true); assert.deepEqual([s.life.dialogCount, s.life.currentDialog], [16, 2]);
});

test('GumpInput 계약/예외: 도달한 처리기·한도 로그·부모/토큰·음수 개수를 거부하고 예외 앞의 쓰기를 남긴다', () => {
    const { s, collection } = connected(); const { events, input } = inputPair(s, collection); assert.throws(() => new GumpInput(s.create, s.life, s.input, collection, events), /저장소/); input.capture(99); assert.throws(() => input.route(1, 2, 3), /토큰/); s.life.focus = 0; s.life.overlay[0] = 99; assert.throws(() => input.overlay(1, { x: 12, y: 12 }), /토큰/); s.life.overlay[0] = 2; s.nodes.get(2).parent = 2; assert.throws(() => input.overlay(1, { x: 12, y: 12 }), /순환/); s.nodes.get(2).parent = 1; s.life.dialogCount = 0xffffffff; assert.throws(() => input.addDialog(2), /음수/); const missing = new GumpInput(s.create, s.life, s.input, collection, events, { node: (token) => s.nodes.get(token) }); s.life.focus = 2; assert.throws(() => missing.route(1, 2, 3), /처리기/); s.life.dialogCount = 16; assert.throws(() => missing.addDialog(2), /로그/); const hooks = s.inputHooks(); hooks.handle = (token) => { s.nodes.get(token).command = 999; s.life.focus = 3; throw new Error('입력 처리기 실패'); }; const broken = new GumpInput(s.create, s.life, s.input, collection, events, hooks); s.life.focus = 0; broken.capture(2); assert.throws(() => broken.route(1, 2, 3), /입력 처리기 실패/); assert.deepEqual([s.nodes.get(2).command, s.life.focus], [999, 3]);
});

test('GumpFrame 실제 idle/지연 수명: 앞 노드가 뒤 노드를 Kill하면 idle/통지/정리 후 등록표에서 일관되게 제외된다', () => {
    const { s, lifetime } = connected(0x101, true); lifetime.setIdle(2, true); lifetime.setIdle(3, true); const visited = []; const hooks = { node: (token) => s.nodes.get(token), idle: (token) => { visited.push(token); if (token === 2 && s.nodes.has(3)) lifetime.kill(3); }, broadcast: (token) => visited.push(token) }; const frame = new GumpFrame(s.create, s.life, hooks); frame.idle(); assert.deepEqual(visited, [2]); assert.equal(s.life.killCount, 1); visited.length = 0; frame.broadcast(7, 11, 13); assert.deepEqual(visited, [2, 4]); lifetime.cleanup(); assert.equal(s.nodes.has(3), false); assert.deepEqual([s.create.count, s.life.killCount, s.nodes.get(2).next], [3, 0, 0]); visited.length = 0; frame.idle(); assert.deepEqual(visited, [2]);
});

test('GumpFrame idle/통지: 마지막500칸·무필터 idle·현재 signed count와 루트 제외를 유지한다', () => {
    const { s } = connected(0x101, true); s.nodes.get(2).flags = 0x10008; s.life.idle[79] = 2; s.life.idle[80] = 3; s.life.idle[499] = 4; const visited = []; const frame = new GumpFrame(s.create, s.life, { node: (token) => s.nodes.get(token), idle: (token) => visited.push(token), broadcast: (token, e, x, y) => { assert.deepEqual([e, x, y], [0x80000001, 11, 13]); visited.push(token); s.create.count = 1; } }); frame.idle(); assert.deepEqual(visited, [2, 3, 4]); visited.length = 0; s.nodes.get(1).flags = 0x101; s.nodes.get(2).flags = 1; s.create.count = 1; frame.broadcast(0x80000001, 11, 13); assert.deepEqual(visited, []); s.create.count = 4; frame.broadcast(0x80000001, 11, 13); assert.deepEqual(visited, [3]); visited.length = 0; s.create.count = 0xffffffff; frame.broadcast(0x80000001, 11, 13); assert.deepEqual(visited, []);
});

test('GumpFrame redraw: 최상위 next는 저장하고 하위 next는 콜백 뒤 현재 링크를 읽는다', () => {
    const { s } = connected(0x101, true); s.nodes.get(4).flags = 0x10009; const visited = []; const hooks = { node: (token) => s.nodes.get(token), redraw: (token, value) => { assert.equal(value, false); visited.push(token); if (token === 2) s.nodes.get(2).next = 0; } }; const frame = new GumpFrame(s.create, s.life, hooks); s.create.collection.overrideHead = 4; frame.redrawRoots(); assert.deepEqual(visited, [2, 4, 3]); visited.length = 0; hooks.redraw = (token) => { visited.push(token); if (token === 2) s.nodes.get(2).child = 3; }; new GumpFrame(s.create, s.life, hooks).redrawNode(2); assert.deepEqual(visited, [2, 3]);
});

test('GumpFrame 계약/예외: 조기 종료는 장치를 읽지 않으며 순환/미연결과 예외 앞의 현재 쓰기를 구별한다', () => {
    const { s } = connected(0x101, true); const node = (token) => s.nodes.get(token); assert.throws(() => new GumpFrame(s.create, s.life), /저장소/); const missing = new GumpFrame(s.create, s.life, { node }); missing.idle(); s.create.count = 0; missing.broadcast(1, 2, 3); s.nodes.get(2).flags = s.nodes.get(3).flags = 0x42001; missing.redrawRoots(); s.life.idle[0] = 999; assert.throws(() => missing.idle(), /토큰/); s.life.idle[0] = 2; assert.throws(() => missing.idle(), /idle/); s.nodes.get(2).flags = 1; assert.throws(() => missing.redrawNode(2), /재그리기/); s.nodes.get(2).flags = 0x101; s.create.count = 4; assert.throws(() => missing.broadcast(1, 2, 3), /통지/); const frame = new GumpFrame(s.create, s.life, { node, redraw: (token, erase) => s.calls.push(`${token},${erase}`) }); s.nodes.get(2).flags = 0x21; s.nodes.get(2).child = 2; assert.throws(() => frame.redrawNode(2), /순환/); s.nodes.get(2).child = 0; s.nodes.get(2).next = 2; assert.throws(() => frame.redrawRoots(), /순환/); const broken = new GumpFrame(s.create, s.life, { node, idle: (token) => { s.nodes.get(token).command = 999; s.life.idle[0] = 3; throw new Error('idle 경계 실패'); } }); assert.throws(() => broken.idle(), /idle 경계 실패/); assert.deepEqual([s.nodes.get(2).command, s.life.idle[0]], [999, 3]);
});
