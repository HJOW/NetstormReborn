// 실제 PE의 전체160BYTE·등록표 관측과 현재 할당/공통 수집/Draw 연결을 대조한다. (2026-11-14 WB04-E410)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, bitsToFloat } from './raw-support.mjs';
import { decodeGumpNode, encodeGumpNode } from './gump-node-support.mjs';
import { GumpCreateNode, GumpCreateState, GumpCreate, gumpNodeFloatBits, GUMP_BASE_SIZE, GUMP_REGISTRY_CAPACITY } from '../www/ui/gump-create.js';
import { GumpCollection, bindGumpCollection } from '../www/render/gump-collection.js';
import { GumpDraw, GumpPixelTarget, GumpDrawState, GUMP_DRAW_CLIP, setGumpDrawClip, bindGumpDraw } from '../www/render/gump-draw.js';
import { collectDrawEntries, sortRenderEntries, dispatchDrawEntries } from '../www/render/draw-submission.js';

// 독립 원본 기록이다. CD/추가판 행은10.78 웹 이식에서 사용하지 않는다.
const rows = readRows(path.join(REPO_ROOT, 'cpppj/tests/fixtures/gumpcreate-x86.tsv'));
// 현재 카운터/화면 크기의 원문 기록 순서다.
const globals = (s) => [s.count, s.serial, s.screenWidth, s.screenHeight].join(',');
// 입력 RLE의 연속 길이/값만 풀며 빈 칸/등록 위치는 계산하지 않는다.
function table(text) { const result = new Uint32Array(GUMP_REGISTRY_CAPACITY); let at = 0; for (const run of text.split(';')) { const [count, value] = run.split(':').map(Number); assert.ok(at + count <= result.length); result.fill(value, at, at + count); at += count; } assert.equal(at, result.length); return result; }
// 현재 등록표만 RLE로 기록한다. 기대 등록 결과는 fixture에서 읽는다.
function encodedTable(entries) { const out = []; for (let at = 0; at < entries.length;) { let end = at + 1; while (end < entries.length && entries[end] === entries[at]) end += 1; out.push(`${end - at}:${entries[at]}`); at = end; } return out.join(';'); }
// 안정된 기본/파생 저장소와 현재 생성 상태다.
class Scene {
    // 같은 노드 Map/등록 상태와 조회 장치다.
    nodes = new Map(); state = new GumpCreateState(); hooks = { node: (token) => this.nodes.get(token) ?? null };
    // 원문 token=전체바이트 입력을 공급한다.
    load(text) { for (const item of text.split(';')) { const [token, bytes] = item.split('='); this.nodes.set(Number(token), decodeGumpNode(bytes)); } }
    // 모든 실제 노드를 토큰 순서로 직렬화한다.
    snapshot() { return [...this.nodes].sort(([a], [b]) => a - b).map(([token, node]) => `${token}=${encodeGumpNode(node)}`).join(';'); }
}

test('GumpCreate_PatchNative: 원본315행의 전체160BYTE/등록표·카운터·반환·할당 순서가 같다', { skip: rows === null }, () => {
    const originals = rows.filter((row) => row[0] === 'originals'); assert.equal(originals.length, 315);
    // 연속 생성은 한 행의 현재 상태를 공유하고 행 사이는 독립 저장소로 만든다.
    for (const row of originals) {
        assert.equal(row.length, 13); const s = new Scene(); [s.state.count, s.state.serial, s.state.screenWidth, s.state.screenHeight] = row[2].split(',').map(Number); s.state.entries.set(table(row[3])); s.state.collection.root = s.state.entries[0]; s.load(row[4]); assert.equal(s.snapshot(), row[4], `초기 ${row[1]}`); const allocations = []; const results = []; let action;
        s.hooks.allocate = (size) => { allocations.push(`${size},${globals(s.state)},${s.state.entries.every((value) => value === 0) ? 1 : 0}`); if (action.length === 6) [s.state.count, s.state.serial, s.state.screenWidth, s.state.screenHeight] = action.slice(2).map(Number); return Number(action[1]); }; const create = new GumpCreate(s.state, s.hooks);
        // 명시 입력 연산만 실행하며 출력 바이트/등록 결과를 생성하지 않는다.
        for (const item of row[5].split(';')) { action = item.split(','); results.push(action[0] === 'Root' ? create.initializeRoot() : create.construct(Number(action[1]), Number(action[2]), bitsToFloat(Number(action[3])), bitsToFloat(Number(action[4])), Number(action[5]), { widthBits: Number(action[3]), heightBits: Number(action[4]) })); }
        assert.equal(s.snapshot(), row[6], `노드 ${row[1]}`); assert.equal(encodedTable(s.state.entries), row[7], `표 ${row[1]}`); assert.equal(globals(s.state), row[8], `전역 ${row[1]}`); assert.equal(results.join(','), row[9]); assert.equal(allocations.join(';') || '-', row[10]); assert.equal(s.state.collection.root, s.state.entries[0]);
    }
});

test('GumpCreate 루트 실패/현재 할당 전역: 표만 비우고 실제 배열·노드/override를 보존하며 new 뒤 값을 읽는다', () => {
    const s = new Scene(); const n = new GumpCreateNode(); s.nodes.set(1, n); n.untouched = 0x81234567; s.state.entries.fill(1); s.state.count = 12; s.state.serial = 23; Object.assign(s.state.collection, { root: 1, overrideHead: 9 }); const entries = s.state.entries; const collection = s.state.collection; const before = s.snapshot();
    s.hooks.allocate = (size) => { assert.equal(size, GUMP_BASE_SIZE); assert.equal(s.state.collection.root, 0); return 0; }; const create = new GumpCreate(s.state, s.hooks); assert.equal(create.initializeRoot(), 0); assert.equal(s.snapshot(), before); assert.ok(entries.every((v) => !v)); assert.equal(s.state.entries, entries); assert.equal(s.state.collection, collection); assert.deepEqual([s.state.count, s.state.serial, collection.overrideHead], [12, 23, 9]);
    const cached = n.cached; const positioning = n.positioning; const text = n.textContext; n.derived = { alive: true }; s.hooks.allocate = () => { Object.assign(s.state, { screenWidth: 16777217, screenHeight: -16777217, count: 997, serial: 0xffffffff }); return 1; }; assert.equal(create.initializeRoot(), 1); assert.equal(s.nodes.get(1), n); assert.equal(n.cached, cached); assert.equal(n.positioning, positioning); assert.equal(n.textContext, text); assert.deepEqual(n.derived, { alive: true }); assert.deepEqual([n.width, n.height, n.id, n.flags, n.serial, n.command, n.untouched, s.state.count, s.state.serial], [16777216, -16777216, 0, 9, 0, 0, 0x81234567, 998, 0]);
});

test('GumpCreate 첫 구멍/미등록·가득 찬 표: serial은 항상 증가하며 id0 정상 반환과 signed 개수 감김을 구별한다', () => {
    const s = new Scene(); s.nodes.set(1, new GumpCreateNode()); s.nodes.set(2, new GumpCreateNode()); s.state.entries.fill(17); s.state.entries[7] = 0; s.state.entries[999] = 0; s.state.count = 12; s.state.serial = 23; const c = new GumpCreate(s.state, s.hooks);
    c.construct(1, 27, 4, 5, 1); assert.equal(s.nodes.get(1).id, 7); assert.deepEqual([s.state.entries[7], s.state.entries[999], s.state.count, s.state.serial], [1, 0, 13, 24]); c.construct(2, 28, 6, 7, 0x100001); assert.deepEqual([s.nodes.get(2).id, s.nodes.get(2).serial, s.state.count], [0, 25, 13]); s.state.entries.fill(17); c.construct(2, 29, 8, 9, 1); assert.equal(s.nodes.get(2).id, 0); assert.equal(s.state.count, 14); assert.ok(s.state.entries.every((v) => v === 17)); s.state.count = 0xffffffff; c.construct(2, 29, 8, 9, 1); assert.equal(s.state.count, 0);
    c.construct(2, 0, NaN, 1, 0x100001, { widthBits: 0x7f800001 }); assert.equal(gumpNodeFloatBits(s.nodes.get(2), 'width'), 0x7f800001); s.nodes.get(2).width = 5; assert.equal(gumpNodeFloatBits(s.nodes.get(2), 'width'), 0x40a00000);
});

test('GumpCreate 같은 생성 저장소→수집→정렬→Draw: 기본 캐시/깊이·현재 visible과 클립을 공유한다', () => {
    const s = new Scene(); s.nodes.set(1, new GumpCreateNode()); s.nodes.set(2, new GumpCreateNode()); s.state.screenWidth = 16; s.state.screenHeight = 16; s.hooks.allocate = () => 1; const create = new GumpCreate(s.state, s.hooks); create.initializeRoot(); create.construct(2, 27, 4, 4, 1 | GUMP_DRAW_CLIP); s.nodes.get(1).child = 2; Object.assign(s.nodes.get(2), { parent: 1, x: 2, y: 3 });
    const collector = new GumpCollection(s.state.collection, create.collectionHooks()); const work = new Uint8Array(64); const collection = {}; bindGumpCollection(collection, collector); const result = collectDrawEntries({ screen: { left: 0, top: 0, right: 16, bottom: 16 }, dirtyFlags: 4, groupRowLimit: 4 }, work, collection); assert.equal(result.count, 1); const record = new DataView(work.buffer); assert.deepEqual([record.getUint32(8, true), record.getInt16(12, true), work[14]], [2, -40, 2]); assert.deepEqual(s.nodes.get(2).cached, { x: 2, y: 3 });
    const state = new GumpDrawState(); setGumpDrawClip(state, { left: 0, top: 0, right: 16, bottom: 16 }); const pixels = new Uint8Array(256); const target = new GumpPixelTarget(pixels, 16, 16, state); const draw = new GumpDraw(collector, state, { draw: (token) => { assert.equal(token, 2); target.lines([{ first: { x: 0, y: 3 }, last: { x: 15, y: 3 }, color: 17 }]); } }); const dispatch = {}; bindGumpDraw(dispatch, draw); const entries = work.subarray(0, 16); sortRenderEntries(entries); dispatchDrawEntries(entries, {}, dispatch); assert.deepEqual([pixels[50], pixels[53], pixels[49], pixels[54]], [17, 17, 0, 0]); assert.equal(state.clip.right, 16); assert.equal(s.state.entries[1], 2); s.nodes.get(2).flags = 0; assert.equal(collector.collect({ left: 0, top: 0, right: 16, bottom: 16 }, work, 4), 0);
});

test('GumpCreate 도달 계약: 등록 개수/없는 토큰·저장소/할당을 변경 전 거부하며 할당 뒤 실패는 빈 표를 남긴다', () => {
    const s = new Scene(); s.nodes.set(1, new GumpCreateNode()); s.nodes.get(1).untouched = 17; s.state.count = 999; const c = new GumpCreate(s.state, s.hooks); const before = s.snapshot(); assert.throws(() => new GumpCreate(s.state), /저장소/); assert.throws(() => c.construct(0, 0, 1, 1, 1), /토큰/); assert.throws(() => c.construct(99, 0, 1, 1, 1), /토큰/); assert.throws(() => c.construct(1, 0, 1, 1, 1), /한도/); assert.equal(s.snapshot(), before); assert.equal(s.state.serial, 0); assert.throws(() => c.initializeRoot(), /할당/); c.construct(1, 0, 1, 1, 0x100001); assert.equal(s.state.count, 999); assert.equal(s.state.serial, 1); s.hooks.allocate = () => 99; assert.throws(() => c.initializeRoot(), /토큰/); assert.equal(s.state.collection.root, 0); assert.ok(s.state.entries.every((v) => v === 0));
});
