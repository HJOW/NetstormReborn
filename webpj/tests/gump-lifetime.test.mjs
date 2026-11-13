// 실제 삭제/overlay 진단/reset 관측과 생성·트리·해제·등록 재사용을 검사한다. (2026-11-14 WB04-E412)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put } from './raw-support.mjs';
import { decodeGumpNode, encodeGumpNode } from './gump-node-support.mjs';
import { GumpCreateNode, GumpCreateState, GumpCreate } from '../www/ui/gump-create.js';
import { GumpTree, GumpTreeState } from '../www/ui/gump-tree.js';
import { GumpLifetimeState, GumpLifetime, runGumpOverlayParent } from '../www/ui/gump-lifetime.js';
import { GumpCollection } from '../www/render/gump-collection.js';

// 10.78만 읽는 독립 PE 관측이다.
const rows = readRows(path.join(REPO_ROOT, 'cpppj/tests/fixtures/gumplifetime-x86.tsv')); const resets = readRows(path.join(REPO_ROOT, 'cpppj/tests/fixtures/uireset-x86.tsv')); const checkingRows = readRows(path.join(REPO_ROOT, 'cpppj/tests/fixtures/gump-checking-parents-x86.tsv'));
// 현재 표의 연속 구간 길이/값만 직렬화한다. 기대 필터/삭제 결과는 계산하지 않는다.
function rle(table) { const out = []; for (let at = 0; at < table.length;) { let end = at + 1; while (end < table.length && table[end] === table[at]) end += 1; out.push(`${end - at}:${table[at]}`); at = end; } return out.join(';'); }
// 입력에 명시된 RLE를 고정 표에 그대로 푼다.
function loadTable(table, text) { let at = 0; for (const item of text.split(';')) { const [count, value] = item.split(':').map(Number); assert.ok(at + count <= table.length); table.fill(value, at, at + count); at += count; } assert.equal(at, table.length); }
// 진단 인자의 순수 ASCII 바이트다.
const hex = (text) => Buffer.from(text).toString('hex');

// 명시 raw 응답·관측 경계만 가진 같은 생성/수명 저장소다.
class Scene {
    // 현재 노드/전역·다섯 표와 raw 콜백 응답이다.
    nodes = new Map(); state = new GumpCreateState(); life = new GumpLifetimeState(); responses = new Map(); calls = []; excluded = 11; reset = false;
    // 전체 고정 표의 현재 참조다.
    table(index) { return [null, this.state.entries, this.life.idle, this.life.overlay, this.life.kill, this.life.dialogs][index]; }
    // 관측 순서의 현재 scalar다. reset만 serial/제외BYTE를 추가한다.
    values() { return [this.state.count, this.life.killCount, this.life.dialogCount, this.life.focus, this.state.collection.overrideHead, this.life.currentDialog, this.life.diagnostics, ...(this.reset ? [this.state.serial, this.excluded & 255] : [])]; }
    // 입력 DWORD를 지정 전역에만 공급한다.
    global(index, value) { if (index === 0) this.state.count = value >>> 0; else if (index === 1) this.life.killCount = value >>> 0; else if (index === 2) this.life.dialogCount = value >>> 0; else if (index === 3) this.life.focus = value >>> 0; else if (index === 4) this.state.collection.overrideHead = value >>> 0; else if (index === 5) this.life.currentDialog = value >>> 0; else if (index === 6) this.life.diagnostics = value >>> 0; else if (index === 7) this.state.serial = value >>> 0; else if (index === 8) this.excluded = value << 24 >> 24; else throw new Error('수명 전역 index 오류'); }
    // 전체 상태/모든 raw 노드를 원문 순서로 직렬화한다.
    snapshot() { return `${this.values().join(',')}#${[1, 2, 3, 4, 5].map((i) => rle(this.table(i))).join('#')}`; }
    nodesSnapshot() { return [...this.nodes].sort(([a], [b]) => a - b).map(([token, node]) => `${token}=${encodeGumpNode(node)}`).join(';'); }
    // 콜백 당시 상태를 먼저 기록하고 fixture 입력 DWORD 응답만 같은 안정 저장소에 쓴다.
    respond(kind, token, args = []) {
        const key = `${kind},${token}`; this.calls.push(`${[kind, token, ...args].join(',')}@${this.snapshot()}@${this.nodesSnapshot()}`);
        // 전역/표/노드 응답은 실제 지정 위치만 바꾸며 삭제 결과를 모형으로 계산하지 않는다.
        for (const [target, at, value] of this.responses.get(key) ?? []) { if (!target) this.global(at, value); else if (target <= 5) { this.table(target)[at] = value; this.state.collection.root = this.state.entries[0]; } else { const node = this.nodes.get(target); const raw = Uint8Array.from(Buffer.from(encodeGumpNode(node), 'hex')); put(raw, at, value); Object.assign(node, decodeGumpNode(Buffer.from(raw).toString('hex'))); } }
    }
    // 가상/해제 경계는 관측만 한다. 실제 해제/트리 조립 검사에서는 해당 실제 처리기로 바꾼다.
    hooks() { return { node: (token) => this.nodes.get(token), push: (token) => this.respond('Push', token), deleteForm: (form, flags) => this.respond('Form', form, [flags]), deletingDestructor: (token, flags) => this.respond('Destructor', token, [flags]), release: (pointer) => this.respond('Free', pointer) }; }
    // 원문 초기 상태/전체 노드를 기대값 계산 없이 읽는다.
    load(row) { const parts = row[2].split('#'); parts[0].split(',').map(Number).forEach((v, i) => this.global(i, v)); for (let i = 1; i <= 5; i += 1) loadTable(this.table(i), parts[i]); this.state.collection.root = this.state.entries[0]; for (const item of row[3].split(';')) { const [token, raw] = item.split('='); this.nodes.set(Number(token), decodeGumpNode(raw)); } if (row[5] !== '-') for (const item of row[5].split(';')) { const [key, patch] = item.split(':'); this.responses.set(key, patch.split('/').map((p) => p.split(',').map(Number))); } }
}

// 같은 실제 생성/트리/수명 장치를 함께 조립한다. redraw는 관측하고 release는 실제 저장소를 제거한다.
function connected() {
    const s = new Scene(); for (let token = 1; token <= 6; token += 1) s.nodes.set(token, new GumpCreateNode()); s.state.screenWidth = 64; s.state.screenHeight = 48; const create = new GumpCreate(s.state, { node: (token) => s.nodes.get(token), allocate: () => 1 }); create.initializeRoot(); create.construct(2, 27, 8, 6, 1); create.construct(3, 27, 3, 2, 1); create.construct(4, 27, 2, 2, 1); const freed = []; const redraw = []; let tree; let lifetime;
    const hooks = s.hooks(); hooks.push = (token) => tree.push(token); hooks.deletingDestructor = (token, flags) => lifetime.deleteScalar(token, flags); hooks.release = (token) => { freed.push(token); s.nodes.delete(token); }; lifetime = new GumpLifetime(s.state, s.life, hooks);
    tree = new GumpTree(s.state, new GumpTreeState(), { node: (token) => s.nodes.get(token), redraw: (token, erase) => redraw.push([token, erase]), push: (token) => tree.push(token), attach: (...args) => tree.attach(...args), kill: (token) => lifetime.kill(token) }); return { s, create, tree, lifetime, freed, redraw };
}

// 지정된 연속 연산만 실행하며 모든 기대 출력은 독립 관측 열에서 읽는다.
function replay(input, reset, expectedCount) {
    const originals = input.filter((r) => r[0] === 'originals'); assert.equal(originals.length, expectedCount);
    // 행 사이는 독립이고 같은 행의 콜백/연속 연산은 현재 상태를 공유한다.
    for (const row of originals) { assert.equal(row.length, 13); const s = new Scene(); s.reset = reset; s.load(row); assert.equal(s.snapshot(), row[2]); assert.equal(s.nodesSnapshot(), row[3]); const hooks = s.hooks(); let lifetime; if (row[6] === '1') hooks.deletingDestructor = (token, flags) => lifetime.deleteScalar(token, flags); lifetime = new GumpLifetime(s.state, s.life, hooks); const results = [];
        // 명시 연산 입력만 원문 API에 넘기고 반환값만 기록한다.
        for (const item of row[4].split(';')) { const [op, ...args] = item.split(','); const n = args.map(Number); const token = n[0]; let result = '-'; if (op === 'Kill') lifetime.kill(token); else if (op === 'Cleanup') lifetime.cleanup(); else if (op === 'Destroy') lifetime.destroy(token); else if (op === 'Base') lifetime.destructBase(token); else if (op === 'Scalar') result = String(lifetime.deleteScalar(token, n[1])); else if (op === 'Idle') lifetime.setIdle(token, n[1] !== 0); else if (op === 'Overlay') lifetime.setOverlay(token, n[1] !== 0); else if (op === 'Dialog') result = lifetime.removeDialog(token) ? '1' : '0'; else if (op === 'KillExceptType') lifetime.killExceptType(() => s.excluded); else if (op === 'ReplaceRoot') lifetime.replaceRoot(token); else throw new Error(`수명 연산 오류 ${op}`); results.push(result); }
        assert.equal(s.nodesSnapshot(), row[7], `노드 ${row[1]}`); assert.equal(s.snapshot(), row[8], `표/전역 ${row[1]}`); assert.equal(results.join('/'), row[9]); assert.equal(s.calls.join('|') || '-', row[10], `콜백 ${row[1]}`);
    }
}

test('GumpLifetime_PatchNative: 원본173행 전체 노드/다섯 표·전역·반환·콜백 당시 상태가 같다', { skip: rows === null }, () => replay(rows, false, 173));
test('GumpReset_PatchNative: 원본121행의 제외BYTE 현재값·전체999칸·루트/큐·콜백 상태가 같다', { skip: resets === null }, () => replay(resets, true, 121));

test('GumpCheckingParents_Native368: overlay320·기본 소멸48의 진단 복귀/중단·현재 포인터·전체 표가 같다', { skip: checkingRows === null }, () => {
    assert.equal(checkingRows.length, 368); const TOKEN = 0x12001000; const MARKER = 0x12345678;
    // 각 입력은 원문 표 모양/진단 응답만 공급하며 기대 결과를 계산하지 않는다.
    for (const row of checkingRows) {
        assert.equal(row.length, 12); const [op, pattern, enabled, flags, check, resume, mutate] = row[0].split(','); const create = new GumpCreateState(); const life = new GumpLifetimeState(); const node = new GumpCreateNode(); let checking = Number(check); let before = 0xa5a5a5a5; const events = []; Object.assign(node, { vtable: MARKER, flags: Number(flags), textPointer: 0x1111, alternateTextPointer: 0x2222 }); life.checking = () => checking;
        if (Number(pattern) >= 3) life.overlay.fill(MARKER); if (Number(pattern) === 1) for (let i = 0; i < 100; i += 1) life.overlay[i] = i % 2 ? 0 : MARKER; if ([2, 4].includes(Number(pattern))) life.overlay[3] = life.overlay[99] = TOKEN;
        life.idle[0] = life.idle[499] = life.kill[0] = life.kill[999] = TOKEN; life.idle[1] = life.kill[1] = MARKER;
        // 콜백은 독립 입력에 지정된 같은 노드/전역/표만 바꾼다.
        const change = (which) => { checking = 0; if (which === 1) { node.flags = 0x13579bd; node.alternateTextPointer = 0x3333; life.overlay[99] = life.overlay[98] = MARKER; } else { node.flags = 0xcafebabe; node.alternateTextPointer = 0x4444; life.overlay[50] = TOKEN; life.overlay[99] = MARKER; } };
        const assertion = (condition, source, line) => { events.push(`D:${hex(condition)}:${hex(source)}:${line}`); if (Number(mutate) === 1) change(1); if (!Number(resume)) throw new Error('assertion'); }; let outcome = 'ok';
        try { if (op === 'O') runGumpOverlayParent(TOKEN, Number(enabled) !== 0, life.overlay, () => life.currentChecking(), { write: (index, value) => { if (index === -1) before = value; else life.overlay[index] = value; }, flags: () => node.flags, setFlags: (value) => { node.flags = value; }, assertion }); else new GumpLifetime(create, life, { node: (token) => { assert.equal(token, TOKEN); return node; }, assertion, release: (pointer) => { events.push(`F:${pointer}`); if (Number(mutate) === 2 && pointer === 0x1111) change(2); } }).destructBase(TOKEN); } catch (error) { outcome = error.message; }
        assert.deepEqual([outcome, events.join(';') || '-', String(checking), String(node.flags), String(node.vtable), String(node.textPointer), String(node.alternateTextPointer), String(before), rle(life.overlay), rle(life.idle), rle(life.kill)], row.slice(1), row[0]);
    }
});

test('GumpLifetime 실제 생성/트리 Kill→Cleanup: 중복 요청·자식 해제·빈 등록 칸/serial 재사용을 연결한다', () => {
    const { s, create, tree, lifetime, freed } = connected(); tree.attach(2, 2, 3, 1, 0); tree.attach(3, 1, 1, 2, 0); tree.attach(4, 2, 2, 2, 0); lifetime.setIdle(3, true); lifetime.setOverlay(4, true); s.life.focus = 3; s.state.collection.overrideHead = 4; s.life.dialogs.set([3, 4]); s.life.dialogCount = 2; s.life.currentDialog = 4; lifetime.kill(2);
    assert.deepEqual([...s.life.kill.subarray(0, 3)], [4, 3, 2]); assert.deepEqual([s.life.killCount, s.nodes.get(1).child, s.nodes.get(2).child, s.life.focus, s.state.collection.overrideHead, s.life.dialogCount, s.state.count], [3, 0, 0, 0, 0, 0, 4]); lifetime.kill(3); assert.equal(s.life.killCount, 4); lifetime.cleanup(); assert.deepEqual(freed, [4, 3, 2]); assert.deepEqual([s.state.count, s.state.entries[0], s.state.collection.root, s.life.killCount], [1, 1, 1, 0]); s.nodes.set(5, new GumpCreateNode()); create.construct(5, 27, 1, 1, 1); assert.deepEqual([s.nodes.get(5).id, s.nodes.get(5).serial, s.state.count], [1, 5, 2]); tree.attach(5, 1, 2, 1, 0); const collection = new GumpCollection(s.state.collection, create.collectionHooks()); const work = new Uint8Array(64); assert.equal(collection.collect({ left: 0, top: 0, right: 64, bottom: 48 }, work, 4), 1); assert.equal(new DataView(work.buffer).getUint32(8, true), 5);
    const replacement = connected(); replacement.s.nodes.get(2).flags |= 0x4000; replacement.s.nodes.get(3).flags |= 0x4000; replacement.tree.attach(2, 0, 0, 1, 0); replacement.tree.attach(3, 0, 0, 1, 0); assert.deepEqual([replacement.s.nodes.get(1).child, replacement.s.life.killCount, replacement.s.life.kill[0], replacement.s.state.count, replacement.s.nodes.get(2).flags & 0x10000], [3, 1, 2, 4, 0x10000]); assert.deepEqual(replacement.freed, []);
});

test('GumpLifetime 실제 제외/루트 교체: 자식 이동·현재 Kill 필터·새 루트 id와 이전 칸 재사용을 연결한다', () => {
    const { s, create, tree, lifetime, freed } = connected(); s.nodes.get(2).command = 21; s.nodes.get(3).command = s.nodes.get(4).command = 11; tree.attach(3, 2, 3, 1, 0); tree.attach(2, 4, 5, 1, 0); tree.attach(4, 1, 1, 2, 0); lifetime.killExceptType(() => s.excluded); lifetime.cleanup(); assert.deepEqual(freed, [4, 2]); assert.deepEqual([s.state.count, s.state.serial, s.nodes.get(1).child], [2, 4, 3]); create.construct(5, 54, 64, 48, 1); const temporary = s.nodes.get(5).id; s.state.entries[temporary] = 0; s.nodes.get(5).id = 0; const { x, y } = s.nodes.get(3); tree.push(3); tree.attach(3, x, y, 5, 0); lifetime.replaceRoot(5); assert.deepEqual(freed, [4, 2, 1]); assert.deepEqual([s.state.entries[0], s.state.collection.root, s.state.count, s.state.serial, s.nodes.get(5).id, s.nodes.get(5).child, s.nodes.get(3).parent, s.life.killCount, s.state.entries[temporary]], [5, 5, 2, 5, 0, 3, 5, 0, 0]); create.construct(6, 21, 4, 3, 1); assert.deepEqual([s.nodes.get(6).id, s.nodes.get(6).serial, s.state.count], [temporary, 6, 3]);
});

test('GumpLifetime 현재 overlay: 미선택 노드 생략·진단 전 마지막 빈 칸 고정·물리 -1 거부와 checking 대여를 구별한다', () => {
    const state = new GumpCreateState(); const life = new GumpLifetimeState(); const node = new GumpCreateNode(); let checking = 0; let reads = 0; let assertions = 0; life.checking = () => checking; const lifetime = new GumpLifetime(state, life, { node: (token) => { assert.equal(token, 7); reads += 1; return node; }, assertion: () => { assertions += 1; checking = 0; } }); lifetime.setOverlay(7, false); assert.deepEqual([reads, assertions], [0, 0]); checking = 0x80000000; lifetime.setOverlay(7, false); assert.deepEqual([reads, assertions, checking], [0, 1, 0]); life.overlay[99] = 7; lifetime.setOverlay(7, true); assert.equal(assertions, 2); assert.equal(life.overlay[98], 7); assert.ok(node.flags & 0x80000); life.overlay.fill(9); node.flags = 0x123; assert.throws(() => lifetime.setOverlay(7, true), /실제 배열 범위/); assert.equal(assertions, 3); assert.equal(node.flags, 0x123); assert.ok(life.overlay.every((v) => v === 9)); life.checking = null; life.diagnostics = 0xffffffff; assert.equal(life.currentChecking(), 0xffffffff); life.diagnostics = 0; assert.equal(life.currentChecking(), 0);
});

test('GumpLifetime 기본 소멸/계약: 모든 중복만 지우고 등록/dialog/flags를 보존하며 예외 앞의 쓰기를 남긴다', () => {
    const { s, lifetime } = connected(); const flags = s.nodes.get(2).flags; s.life.idle[1] = s.life.idle[499] = s.life.overlay[0] = s.life.overlay[99] = s.life.kill[0] = s.life.kill[999] = 2; s.life.killCount = 1; s.life.dialogs[0] = 2; s.life.dialogCount = 1; lifetime.destructBase(2); assert.deepEqual([s.life.idle[1], s.life.idle[499], s.life.overlay[99], s.life.kill[999]], [0, 0, 0, 0]); assert.deepEqual([s.state.count, s.state.entries[1], s.life.killCount, s.life.dialogCount, s.nodes.get(2).flags], [4, 2, 1, 1, flags]); assert.throws(() => new GumpLifetime(s.state, s.life), /저장소/); assert.throws(() => lifetime.kill(99), /토큰/); s.nodes.get(2).flags |= 0x80; assert.throws(() => lifetime.setIdle(2, false), /표\/플래그/); s.nodes.get(2).flags &= ~0x80; s.life.killCount = 999; assert.throws(() => lifetime.kill(2), /큐 원본 한도/); s.life.killCount = 0; s.life.dialogCount = 17; assert.throws(() => lifetime.removeDialog(2), /dialog/); s.life.dialogCount = 0; s.nodes.get(2).child = 3; s.nodes.get(3).child = 2; assert.throws(() => lifetime.kill(2), /자식 순환/); s.nodes.get(2).child = s.nodes.get(3).child = 0; s.nodes.get(2).id = 1000; assert.throws(() => lifetime.destroy(2), /id 범위/); s.nodes.get(2).id = 1;
    s.life.killCount = 1; s.life.kill[0] = 2; assert.throws(() => lifetime.cleanup(), /dead 필요/); s.life.killCount = 0; s.life.kill[0] = 0; lifetime.setOverlay(2, true); assert.throws(() => lifetime.setOverlay(2, true), /선택 장치/);
    const base = new GumpLifetime(s.state, s.life, { node: (token) => s.nodes.get(token), deletingDestructor: () => { throw new Error('가상 소멸자 실패'); }, release: () => { throw new Error('텍스트 해제 실패'); } }); assert.throws(() => base.destroy(2), /가상 소멸자 실패/); assert.deepEqual([s.state.count, s.state.entries[1], s.nodes.get(2).flags & 4, s.nodes.get(2).flags & 0x10000], [3, 0, 4, 0]); Object.assign(s.nodes.get(3), { vtable: 123, textPointer: 456 }); assert.throws(() => base.destructBase(3), /텍스트 해제 실패/); assert.deepEqual([s.nodes.get(3).vtable, s.nodes.get(3).textPointer], [0x50ac70, 456]);
});
