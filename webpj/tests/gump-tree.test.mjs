// 실제 PE 트리574행·부착 진단432행과 같은 생성/수집/Draw·가상 재배치를 검사한다. (2026-11-14 WB04-E411)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, bitsToFloat, floatToBits, put } from './raw-support.mjs';
import { decodeGumpNode, encodeGumpNode } from './gump-node-support.mjs';
import { GumpCreateNode, GumpCreateState, GumpCreate } from '../www/ui/gump-create.js';
import { GumpTreeState, GumpTree, recordGumpAttachDiagnostic } from '../www/ui/gump-tree.js';
import { GumpCollection } from '../www/render/gump-collection.js';
import { GumpDraw, GumpDrawState, GumpPixelTarget, GUMP_DRAW_CLIP, setGumpDrawClip, bindGumpDraw } from '../www/render/gump-draw.js';
import { sortRenderEntries, dispatchDrawEntries } from '../www/render/draw-submission.js';

// 보호10.78 독립 관측이다. CD/추가판 트리 행은 범위 밖이다.
const rows = readRows(path.join(REPO_ROOT, 'cpppj/tests/fixtures/gumptree-x86.tsv')); const diagnosticRows = readRows(path.join(REPO_ROOT, 'cpppj/tests/fixtures/gump-attach-diagnostic-x86.tsv'));
// 인자/정렬 반환의 순수 복사 float 비트는 명시 값을 우선한다.
const bits = (value, exact) => exact === undefined ? floatToBits(value) : exact >>> 0;
// ASCII 진단의 원문16진 기록이다.
const hex = (value) => Buffer.from(value).toString('hex');

// 같은 안정 노드/현재 전역과 명시 콜백 응답만 소유한다. 별도 부착/정렬 계산 모형은 없다.
class Scene {
    // 현재 생성·트리 상태와 노드/응답/투영 입력·관측 목록이다.
    state = new GumpCreateState(); tree = new GumpTreeState(); nodes = new Map(); responses = new Map(); anchors = new Map(); calls = [];
    // 실제 토큰 순서 전체160BYTE를 기록한다.
    snapshot() { return [...this.nodes].sort(([a], [b]) => a - b).map(([token, node]) => `${token}=${encodeGumpNode(node)}`).join(';'); }
    // 전역 기록 순서는 루트/화면/지도/패치 진단 모드다.
    globals() { const m = this.tree.mapView; return [this.state.collection.root, this.state.screenWidth, this.state.screenHeight, m.left, m.top, m.right, m.bottom, this.tree.pushDebugMode].join(','); }
    // 입력 DWORD를 지정 전역에만 쓴다.
    global(index, value) { if (index === 0) this.state.collection.root = value >>> 0; else if (index === 1) this.state.screenWidth = value | 0; else if (index === 2) this.state.screenHeight = value | 0; else if (index >= 3 && index <= 6) this.tree.mapView[['left', 'top', 'right', 'bottom'][index - 3]] = value | 0; else if (index === 7) this.tree.pushDebugMode = value >>> 0; else throw new Error('잘못된 트리 전역 응답'); }
    // 콜백 시점 상태를 먼저 관찰하고 원문 입력에 명시된 DWORD 응답만 적용한다.
    respond(kind, token, args = []) {
        const key = `${kind},${token}`; this.calls.push(`${[kind, token, ...args].join(',')}@${this.globals()}@${this.snapshot()}`);
        // 현재 안정 노드를 교체하지 않고 실제 raw 응답 필드만 적용한다.
        for (const [target, at, value] of this.responses.get(key) ?? []) { if (!target) { this.global(at, value); continue; } const node = this.nodes.get(target); const raw = Uint8Array.from(Buffer.from(encodeGumpNode(node), 'hex')); put(raw, at, value); Object.assign(node, decodeGumpNode(Buffer.from(raw).toString('hex'))); }
    }
    // 실제 가상 출력/통지 경계만 관찰한다. 검사 입력이 없는 응답에는 상태 쓰기를 덧붙이지 않는다.
    hooks() { return { node: (token) => this.nodes.get(token), redraw: (token, erase) => this.respond('Redraw', token, [erase ? 1 : 0]), push: (token) => this.respond('Push', token), attach: (token, x, y, parent, mode, exact = {}) => this.respond('Attach', token, [bits(x, exact.xBits), bits(y, exact.yBits), parent, mode]), kill: (token) => this.respond('Kill', token), project: (sid) => { this.respond('Project', sid); return this.anchors.get(sid); } }; }
    // 실제 생성기로 루트와 기본 세 노드를 준비한다.
    create() { for (const token of [1, 2, 3, 4]) this.nodes.set(token, new GumpCreateNode()); this.state.screenWidth = 32; this.state.screenHeight = 24; const create = new GumpCreate(this.state, { node: (token) => this.nodes.get(token), allocate: () => 1 }); create.initializeRoot(); create.construct(2, 27, 8, 6, 1 | GUMP_DRAW_CLIP); create.construct(3, 27, 3, 2, 1); create.construct(4, 27, 2, 2, 1); return create; }
}

test('GumpTree_PatchNative: 원본574행의 전체 노드·전역·정렬 비트·콜백 당시 상태/순서가 같다', { skip: rows === null }, () => {
    const originals = rows.filter((row) => row[0] === 'originals'); assert.equal(originals.length, 574);
    // 각 행의 연속 연산은 같은 현재 노드/전역을 공유한다.
    for (const row of originals) {
        assert.equal(row.length, 13); const s = new Scene(); row[2].split(',').map(Number).forEach((value, index) => s.global(index, value));
        // 완성 입력 노드의 raw 바이트를 공급한다.
        for (const item of row[3].split(';')) { const [token, raw] = item.split('='); s.nodes.set(Number(token), decodeGumpNode(raw)); } assert.equal(s.snapshot(), row[3]);
        // 응답은 지정 token/offset/value만 가지며 기대 부착 결과를 계산하지 않는다.
        if (row[5] !== '-') for (const item of row[5].split(';')) { const [key, patches] = item.split(':'); s.responses.set(key, patches.split('/').map((p) => p.split(',').map(Number))); }
        // 월드 투영은 원문에서 공급한 정수 응답이다.
        if (row[6] !== '-') for (const item of row[6].split(';')) { const [sid, x, y] = item.split(',').map(Number); s.anchors.set(sid, { x, y }); }
        const tree = new GumpTree(s.state, s.tree, s.hooks()); const results = [];
        // 명시 연산만 실제 처리기에 넘긴다. 모든 결과는 관측 열과 대조한다.
        for (const item of row[4].split(';')) {
            const [op, ...p] = item.split(','); const n = p.map(Number); const token = n[0]; let result = '-';
            if (op === 'Attach') tree.attach(token, bitsToFloat(n[1]), bitsToFloat(n[2]), n[3], n[4], { xBits: n[1], yBits: n[2] });
            else if (op === 'Push') tree.push(token);
            else if (op === 'Align') { const value = tree.align(token); const out = new Uint8Array(8); put(out, 0, bits(value[0], value.xBits)); put(out, 4, bits(value[1], value.yBits)); result = Buffer.from(out).toString('hex'); }
            else if (op === 'AddDepth') tree.addDepth(token, n[1]); else if (op === 'SetDepth') tree.setDepth(token, n[1]);
            else if (op === 'Move') tree.move(token, bitsToFloat(n[1]), bitsToFloat(n[2]), n[3], { xBits: n[1], yBits: n[2] });
            else if (op === 'Relative') tree.moveRelative(token, n[1], bitsToFloat(n[2]), bitsToFloat(n[3]), n[4], n[5], { xBits: n[2], yBits: n[3] }); else throw new Error(`알 수 없는 연산 ${op}`); results.push(result);
        }
        assert.equal(s.snapshot(), row[7], `노드 ${row[1]}`); assert.equal(s.globals(), row[8], `전역 ${row[1]}`); assert.equal(results.join('/'), row[9], `반환 ${row[1]}`); assert.equal(s.calls.join('|') || '-', row[10], `콜백 ${row[1]}`);
    }
});

test('GumpAttachDiagnostic_Native432: 현재 checking·signed GT/상한·상세와 전체 노드가 같다', { skip: diagnosticRows === null }, () => {
    assert.equal(diagnosticRows.length, 432);
    // 독립 관측의 명시 진단 입력/응답만 공급한다.
    for (const row of diagnosticRows) { const input = row[0].split(',').map(Number); const raw = Uint8Array.from({ length: 160 }, (_, i) => i * 17 + 3); put(raw, 32, input[0]); const node = decodeGumpNode(Buffer.from(raw).toString('hex')); let checking = input[1]; let detail = null; let events = '';
        recordGumpAttachDiagnostic(node, () => checking, input[2], (value) => { detail = value; events = `L:${hex(value)};`; if (input[3]) { checking = 0; node.command = 2; node.flags = 0x200009; } }); events += 'B'; assert.equal(events, row[1]); assert.equal(encodeGumpNode(node), row[2]); assert.equal(checking, Number(row[3])); assert.equal(detail === null ? '-' : hex(detail), row[4]); assert.equal(row[5], 'body'); }
});

test('GumpTree 실제 생성→부착/밀어내기→수집/Draw: 같은 캐시·깊이·클립을 유지하며 재부착 현재 위치를 읽는다', () => {
    const s = new Scene(); const create = s.create(); const tree = new GumpTree(s.state, s.tree, s.hooks()); tree.attach(2, 2, 3, 1, 0); tree.attach(3, 1, 1, 2, 0); assert.equal(s.nodes.get(1).child, 2); assert.equal(s.nodes.get(2).child, 3); assert.equal(s.nodes.get(3).depth, (s.nodes.get(2).depth - 3) >>> 0);
    const collection = new GumpCollection(s.state.collection, create.collectionHooks()); const work = new Uint8Array(128); const clip = { left: 0, top: 0, right: 32, bottom: 24 }; assert.equal(collection.collect(clip, work, 8), 2); const view = new DataView(work.buffer); assert.deepEqual([view.getUint32(8, true), view.getUint32(24, true)], [2, 3]); assert.deepEqual(s.nodes.get(3).cached, { x: 3, y: 4 });
    const state = new GumpDrawState(); setGumpDrawClip(state, clip); const pixels = new Uint8Array(768); const target = new GumpPixelTarget(pixels, 32, 24, state); const draw = new GumpDraw(collection, state, { draw: (token) => { const y = collection.position(token).y; target.lines([{ first: { x: 0, y }, last: { x: 31, y }, color: token + 10 }]); } }); const dispatch = {}; bindGumpDraw(dispatch, draw); const entries = work.subarray(0, 32); sortRenderEntries(entries); dispatchDrawEntries(entries, {}, dispatch); assert.deepEqual([pixels[97], pixels[98], pixels[105], pixels[106], pixels[128], pixels[159]], [0, 12, 12, 0, 13, 13]); tree.push(2); assert.equal(s.nodes.get(1).child, 0); assert.equal(s.nodes.get(2).parent, 1); assert.equal(collection.collect(clip, work, 8), 0); tree.attach(2, 7, 8, 0, 1); assert.equal(collection.collect(clip, work, 8), 2); assert.deepEqual(s.nodes.get(3).cached, { x: 8, y: 9 });
});

test('GumpTree 실제 가상 자식 재배치·깊이: 같은 트리의 링크를 갱신하며 unsigned 감김이 수집 signed키에 도달한다', () => {
    const s = new Scene(); const create = s.create(); const hooks = s.hooks(); let tree; hooks.push = (token) => tree.push(token); hooks.attach = (...args) => tree.attach(...args); tree = new GumpTree(s.state, s.tree, hooks); tree.attach(2, 2, 3, 1, 0); tree.attach(3, 1, 1, 2, 0); tree.attach(4, 2, 2, 2, 0); s.nodes.get(3).alignFlags = 0x10; s.nodes.get(4).alignFlags = 0x20; s.nodes.get(4).secondaryDepth = 7; tree.push(2); tree.attach(2, 4, 5, 0, 0);
    assert.deepEqual([s.nodes.get(2).child, s.nodes.get(3).next, s.nodes.get(4).previous], [3, 4, 3]); assert.deepEqual([s.nodes.get(3).x, s.nodes.get(4).y, s.nodes.get(4).depth], [5, 4, (s.nodes.get(2).depth + 7) >>> 0]); assert.equal((s.nodes.get(3).flags | s.nodes.get(4).flags) & 8, 0); tree.setDepth(2, 0xfffffff0); tree.addDepth(2, 0x80000000); assert.equal(s.nodes.get(3).depth, 0x7ffffff0); const collection = new GumpCollection(s.state.collection, create.collectionHooks()); const work = new Uint8Array(128); assert.equal(collection.collect({ left: 0, top: 0, right: 32, bottom: 24 }, work, 8), 3); const data = new DataView(work.buffer); assert.equal(data.getInt16(12, true), -16); assert.equal(data.getInt16(28, true), -16);
});

test('GumpTree 현재 진단 writer: 첫 쓰기 전에 바꾼 현재 GT/flags/부모와 checking을 실제 부착에서 읽는다', () => {
    const s = new Scene(); s.create(); let checking = 0x80000000; s.tree.checking = () => checking; const n = s.nodes.get(2); Object.assign(n, { command: 0, flags: 9, parent: 1, depth: 123 }); s.nodes.get(3).depth = 77; const events = []; const hooks = s.hooks(); hooks.diagnostic = (text) => { events.push(text); checking = 0; Object.assign(n, { command: 2, flags: n.flags | 0x200000, parent: 3, x: 999 }); }; hooks.redraw = (token, erase) => { assert.deepEqual([token, erase], [2, false]); events.push('R'); }; const tree = new GumpTree(s.state, s.tree, hooks); tree.attach(2, 3.25, -1.5, 0, 1); assert.deepEqual(events, ['gt = 0', 'R']); assert.deepEqual([checking, n.command, n.flags, n.parent, n.depth, n.x, n.y, s.nodes.get(1).child, s.nodes.get(3).child], [0, 2, 0x200001, 3, 123, 3.25, -1.5, 0, 2]);
});

test('GumpTree 계약/예외: 도달한 장치·토큰·void·지도 정렬/순환을 거부하고 이미 쓴 상태를 보존한다', () => {
    const s = new Scene(); s.create(); const hooks = s.hooks(); const tree = new GumpTree(s.state, s.tree, hooks); assert.throws(() => new GumpTree(s.state, s.tree), /저장소/); assert.throws(() => tree.attach(99, 0, 0, 1, 0), /토큰/); const before = s.snapshot(); assert.throws(() => tree.attach(2, 0, 0, 99, 0), /토큰/); assert.equal(s.snapshot(), before); assert.throws(() => tree.push(2), /void/); Object.assign(s.nodes.get(2), { parent: 1, child: 3 }); s.nodes.get(3).child = 2; assert.throws(() => tree.addDepth(2, 1), /순환/); Object.assign(s.nodes.get(2), { parent: 3, alignFlags: 0x40 }); assert.throws(() => tree.align(2), /루트/);
    recordGumpAttachDiagnostic(new GumpCreateNode(), () => 0, 67); assert.throws(() => recordGumpAttachDiagnostic(new GumpCreateNode(), () => 1, 67), /writer/); const e = new Scene(); e.create(); const failed = e.hooks(); failed.redraw = () => { throw new Error('입력 처리기 실패'); }; const writer = new GumpTree(e.state, e.tree, failed); assert.throws(() => writer.attach(2, 2, 3, 1, 0), /처리기 실패/); assert.deepEqual([e.nodes.get(1).child, e.nodes.get(2).parent, e.nodes.get(2).x, e.nodes.get(2).flags & 8], [2, 1, 2, 0]); assert.throws(() => writer.push(2), /처리기 실패/); assert.equal(e.nodes.get(1).child, 0); assert.ok(e.nodes.get(2).flags & 8);
    const world = new Scene(); world.create(); world.nodes.get(2).flags |= 0x2000; const noProjection = world.hooks(); delete noProjection.project; assert.throws(() => new GumpTree(world.state, world.tree, noProjection).attach(2, 0, 0, 1, 0), /투영 필요/); assert.equal(world.nodes.get(1).child, 2);
});
