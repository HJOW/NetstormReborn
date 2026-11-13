// 공통 UI Draw/클립을 원본476행과 실제 트리·픽셀 출력으로 대조한다. (2026-11-14 WB04-E405)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, get, bitsToFloat } from './raw-support.mjs';
import { GumpCollection, bindGumpCollection } from '../www/render/gump-collection.js';
import { collectDrawEntries, dispatchDrawEntries, sortRenderEntries } from '../www/render/draw-submission.js';
import { planButtonDraw } from '../www/render/gump-visual.js';
import { GUMP_DRAW_CLIP, GumpDrawState, GumpClipScope, setGumpDrawClip, GumpDraw, GumpPixelTarget, bindGumpDraw } from '../www/render/gump-draw.js';

// 원본 노드 주소/간격과 저장된 독립 관측이다.
const NODES = 0x15000000; const STRIDE = 128; const rows = readRows(path.join(REPO_ROOT, 'cpppj/tests/fixtures/gumpdraw-x86.tsv'));
// signed 화면 영역과 두 상태를 원문 열 순서로 읽고 쓴다.
const rect = (s) => { const [left, top, right, bottom] = s.split(',').map(Number); return { left, top, right, bottom }; };
const coordinates = (r) => [r.left, r.top, r.right, r.bottom].join(',');
const snapshot = (s) => `${coordinates(s.clip)},${coordinates(s.vfxClip)}`;
// 명시 선 입력과 IndexedImage 입력의 표기만 간단히 한다.
const line = (x1, y1, x2, y2, color) => ({ first: { x: x1, y: y1 }, last: { x: x2, y: y2 }, color });
const image = (width, height, indices, opacity) => ({ width, height, indices: Uint8Array.from(indices), opacity: Uint8Array.from(opacity) });

// 같은 노드 저장소를 실제 Collection/Draw에 공급하고 투영 효과만 명시 입력으로 둔다.
class GumpScene {
    // 노드/투영 입력·수집/클립 상태와 투영 사건이다.
    nodes = new Map(); anchors = new Map(); collection = { root: NODES, overrideHead: 0 }; draw = new GumpDrawState(); projectClip = null; projected = [];
    // 빈 노드에 입력 필드를 덮어쓴다.
    make(token, fields = {}) { const n = { flags: 0, next: 0, parent: 0, child: 0, object: 0, depth: 0, width: 0, height: 0, x: 0, y: 0, cached: { x: 0, y: 0 }, ...fields }; this.nodes.set(token, n); return n; }
    // 완성128BYTE 입력만 해석하며 기대 위치/클립은 계산하지 않는다.
    load(text) {
        // 각 토큰의 물리 필드/초기 캐시를 그대로 공급한다.
        for (const item of text.split(';')) {
            const [token, bytes] = item.split('='); const raw = Uint8Array.from(Buffer.from(bytes, 'hex'));
            this.make(Number(token), { flags: get(raw, 4), next: get(raw, 12), parent: get(raw, 20), child: get(raw, 24), object: get(raw, 36), depth: get(raw, 48),
                width: bitsToFloat(get(raw, 68)), height: bitsToFloat(get(raw, 72)), x: bitsToFloat(get(raw, 76)), y: bitsToFloat(get(raw, 80)), cached: { x: get(raw, 108) | 0, y: get(raw, 112) | 0 } });
        }
    }
    // Bounds의 실제 투영 경계가 현재 클립을 바꿀 수 있다.
    hooks() { return { node: (token) => this.nodes.get(token) ?? null, project: (sid) => { this.projected.push(sid); if (this.projectClip) setGumpDrawClip(this.draw, this.projectClip); return this.anchors.get(sid); } }; }
    // 접근 여부와 무관한 모든 노드 캐시를 주소순으로 비교한다.
    caches() { return [...this.nodes].sort(([a], [b]) => a - b).map(([token, n]) => `${token},${n.cached.x},${n.cached.y}`).join(';'); }
}

test('GumpDraw_PatchNative: 원본476행의 보호/현재 클립·중첩·캐시·투영 순서가 같다', { skip: rows === null }, () => {
    const originals = rows.filter((r) => r[0] === 'originals'); assert.equal(originals.length, 476);
    // 입력마다 새 저장소를 주어 이전 출력의 현재 상태를 섞지 않는다.
    for (const row of originals) {
        assert.equal(row.length, 17); const s = new GumpScene(); s.load(row[8]); s.draw.clip = rect(row[4]); s.draw.vfxClip = rect(row[5]); if (row[7] !== '-') s.projectClip = rect(row[7]);
        if (row[9] !== '-') {
            // 기대 투영 계산 대신 경계가 반환하는 입력 정수점만 읽는다.
            for (const item of row[9].split(';')) { const [sid, x, y] = item.split(',').map(Number); s.anchors.set(sid, { x, y }); }
        }
        const token = Number(row[2]); const mode = Number(row[3]); const observed = []; const collection = new GumpCollection(s.collection, s.hooks()); let draw;
        draw = new GumpDraw(collection, s.draw, { draw: (current) => { observed.push(`${current},${snapshot(s.draw)}`); if (current === token && mode === 1) setGumpDrawClip(s.draw, rect(row[6])); if (current === token && mode === 2) draw.draw(NODES + 2 * STRIDE); } });
        draw.draw(token); assert.equal(observed.join(';'), row[10], row[1]); assert.equal(snapshot(s.draw), row[11], row[1]); assert.equal(s.caches(), row[12], row[1]); assert.equal(s.projected.join(',') || '-', row[13], row[1]);
    }
});

// 현재 플래그/예외/투영 경계의 clip 복구는 실제 C++ 계약과 별도로 확인한다.
test('GumpDraw_CurrentFlagsAndNoUnclippedCacheWrite: 보호 없는 경로는 위치를 읽지 않고 가상 clip 변경을 남긴다', () => {
    const s = new GumpScene(); s.collection.root = 1; s.make(1); const n = s.make(2, { flags: 0x10000, parent: 999, child: 999, width: 4, height: 4, x: 2, y: 3, cached: { x: 17, y: 19 } });
    setGumpDrawClip(s.draw, rect('0,0,8,8')); let calls = 0; const c = new GumpCollection(s.collection, s.hooks()); const d = new GumpDraw(c, s.draw, { draw: (token) => { assert.equal(token, 2); calls += 1; setGumpDrawClip(s.draw, rect('1,2,5,6')); } });
    d.draw(2); assert.equal(calls, 1); assert.deepEqual(n.cached, { x: 17, y: 19 }); assert.equal(snapshot(s.draw), '1,2,5,6,1,2,4,5');
    n.parent = 1; n.flags = GUMP_DRAW_CLIP; setGumpDrawClip(s.draw, rect('0,0,8,8')); d.draw(2); assert.equal(calls, 2); assert.deepEqual(n.cached, { x: 2, y: 3 }); assert.equal(snapshot(s.draw), '0,0,8,8,0,0,7,7');
});

test('GumpDraw_ExceptionRestoresProtectedClip: 예외에서 저장한 논리 영역으로 두 전역만 복구한다', () => {
    const s = new GumpScene(); s.collection.root = 1; s.make(1); const n = s.make(2, { flags: GUMP_DRAW_CLIP, parent: 1, width: 4, height: 4, x: 2, y: 3 }); setGumpDrawClip(s.draw, rect('0,0,8,8'));
    const d = new GumpDraw(new GumpCollection(s.collection, s.hooks()), s.draw, { draw: () => { setGumpDrawClip(s.draw, rect('1,2,5,6')); throw new Error('검사 가상 Draw 실패'); } });
    assert.throws(() => d.draw(2), /검사 가상/); assert.equal(snapshot(s.draw), '0,0,8,8,0,0,7,7'); n.flags = 0; assert.throws(() => d.draw(2), /검사 가상/); assert.equal(snapshot(s.draw), '1,2,5,6,1,2,4,5');
});

test('GumpDraw_ProjectionChangesClipBeforeGuard: 실제 Bounds 투영 뒤 현재 clip을 저장하고 부착 자신 캐시는 보존한다', () => {
    const s = new GumpScene(); s.collection.root = 1; s.make(1); const n = s.make(2, { flags: GUMP_DRAW_CLIP | 0x2000, parent: 1, object: 7, width: 10, height: 10, x: 2, y: 3, cached: { x: 17, y: 19 } });
    s.anchors.set(7, { x: 4, y: 5 }); s.projectClip = rect('5,7,21,31'); setGumpDrawClip(s.draw, rect('0,0,8,8')); let observed;
    new GumpDraw(new GumpCollection(s.collection, s.hooks()), s.draw, { draw: () => { observed = snapshot(s.draw); } }).draw(2);
    assert.equal(observed, '6,8,16,18,6,8,15,17'); assert.equal(snapshot(s.draw), '5,7,21,31,5,7,20,30'); assert.deepEqual(s.projected, [7]); assert.deepEqual(n.cached, { x: 17, y: 19 });
});

test('GumpDraw_CommonTreeDispatchPixels: 실제 트리→수집/CRT/kind2→버튼 네 선/투명 그림과 보호 바이트를 지킨다', () => {
    const s = new GumpScene(); s.collection.root = 1; s.make(1, { child: 2 }); s.make(2, { flags: 1 | GUMP_DRAW_CLIP, parent: 1, child: 3, depth: 6, width: 6, height: 6, x: 2, y: 2 }); s.make(3, { flags: 1 | GUMP_DRAW_CLIP, parent: 2, depth: 5, width: 10, height: 10, x: -2, y: -2 });
    setGumpDrawClip(s.draw, rect('0,0,12,12')); const guarded = new Uint8Array(146).fill(9); const pixels = guarded.subarray(1, 145); const target = new GumpPixelTarget(pixels, 12, 12, s.draw); const picture = image(2, 2, [55, 0, 255, 44], [255, 0, 255, 255]);
    const c = new GumpCollection(s.collection, s.hooks()); const calls = []; const d = new GumpDraw(c, s.draw, { draw: (token) => {
        calls.push(token);
        if (token === 2) { const p = planButtonDraw({ rect: c.bounds(token), flags: 0, value: 0, pressed: 0, textWidth: 0, textHeight: 0 }, [], 17); target.lines(p.lines.map((l) => line(l.x1, l.y1, l.x2, l.y2, l.color))); }
        else target.image(picture, { x: 0, y: 0 });
    } });
    const collection = {}; const dispatch = {}; bindGumpCollection(collection, c); bindGumpDraw(dispatch, d); const work = new Uint8Array(64); const screen = rect('0,0,12,12'); const result = collectDrawEntries({ screen, visible: screen, unclipped: screen, dirtyFlags: 4, groupRowLimit: 8 }, work, collection);
    assert.equal(result.count, 2); const entries = work.subarray(0, 32); sortRenderEntries(entries); dispatchDrawEntries(entries, {}, dispatch); assert.deepEqual(calls, [2, 3]); assert.deepEqual([pixels[27], pixels[0], pixels[1], pixels[12], guarded[0], guarded[145]], [17, 55, 9, 255, 9, 9]); assert.equal(snapshot(s.draw), '0,0,12,12,0,0,11,11');
});

test('GumpDraw_PixelImageTransparencyCurrentInclusiveClip: 현재 VFX 양 끝 포함 영역·투명0/255·색 치환을 지킨다', () => {
    const state = new GumpDrawState(); state.clip = rect('0,0,8,4'); state.vfxClip = rect('2,1,4,2'); const pixels = new Uint8Array(32).fill(9); const target = new GumpPixelTarget(pixels, 8, 4, state); const picture = image(4, 1, [55, 0, 255, 44], [0, 255, 255, 255]); const colors = Uint8Array.from({ length: 256 }, (_, i) => i + 10);
    target.image(picture, { x: 1, y: 1 }, colors); assert.deepEqual([pixels[9], pixels[10], pixels[11], pixels[12], pixels[13]], [9, 10, 9, 54, 9]); setGumpDrawClip(state, rect('1,2,3,3')); target.image(picture, { x: 1, y: 2 }); assert.deepEqual([pixels[17], pixels[18], pixels[19]], [9, 0, 9]);
    const before = pixels.slice(); setGumpDrawClip(state, rect('4,2,4,3')); target.image(picture, { x: 1, y: 2 }); assert.deepEqual(pixels, before);
});

test('GumpDraw_PixelLinesAndExtremeClip: 직교/대각선 끝점·현재 clip·극단DWORD 감김과BYTE 색을 지킨다', () => {
    const state = new GumpDrawState(); setGumpDrawClip(state, rect('2,2,6,6')); const pixels = new Uint8Array(64).fill(9); const target = new GumpPixelTarget(pixels, 8, 8, state); const lines = [line(7, 3, 1, 3, 273), line(4, 7, 4, 1, 18)]; target.lines(lines);
    assert.deepEqual([pixels[26], pixels[29], pixels[25], pixels[30], pixels[20], pixels[44], pixels[52]], [17, 17, 9, 9, 18, 18, 9]); state.vfxClip = rect('0,0,2147483647,2147483647'); target.lines([line(-2147483648, 0, 2147483647, 0, 255)]); assert.ok(pixels.subarray(0, 8).every((v) => v === 255));
    setGumpDrawClip(state, rect('0,0,-2147483648,-2147483648')); assert.equal(state.vfxClip.right, 2147483647); setGumpDrawClip(state, rect('6,6,2,2')); const before = pixels.slice(); target.lines(lines); assert.deepEqual(pixels, before);
    pixels.fill(0); setGumpDrawClip(state, rect('0,0,8,8')); target.lines([line(2, 2, 3, 3, 1)]); assert.equal(pixels.filter((v) => v === 1).length, 2); pixels.fill(0); setGumpDrawClip(state, rect('0,0,3,3')); target.lines([line(2, 2, 3, 3, 1)]); assert.deepEqual([pixels[18], pixels[27]], [1, 0]); assert.equal(pixels.filter((v) => v === 1).length, 1);
});

test('GumpDraw_Contracts: 필수 가상/노드/부모·표면/이미지 계약과 직접 scope 복구를 검사한다', () => {
    const s = new GumpScene(); s.collection.root = 1; s.make(1); s.make(2, { flags: GUMP_DRAW_CLIP, parent: 999, width: 4, height: 4 }); setGumpDrawClip(s.draw, rect('0,0,8,8')); const c = new GumpCollection(s.collection, s.hooks()); const d = new GumpDraw(c, s.draw, { draw() {} });
    assert.throws(() => new GumpDraw(c, s.draw, {}), /가상 Draw/); assert.throws(() => d.draw(999), /저장소/); assert.throws(() => d.draw(2), /저장소/); assert.equal(snapshot(s.draw), '0,0,8,8,0,0,7,7'); const pixels = new Uint8Array(64); assert.throws(() => new GumpPixelTarget(pixels, 0, 8, s.draw), /용량/); assert.throws(() => new GumpPixelTarget(pixels.subarray(1), 8, 8, s.draw), /용량/);
    const target = new GumpPixelTarget(pixels, 8, 8, s.draw); assert.throws(() => target.image(image(2, 2, [1], [255]), { x: 0, y: 0 })); const scope = new GumpClipScope(s.draw, rect('1,1,4,4')); assert.equal(snapshot(s.draw), '1,1,4,4,1,1,3,3'); scope.dispose(); scope.dispose(); assert.equal(snapshot(s.draw), '0,0,8,8,0,0,7,7');
});

test('GumpPixelTarget rawBitmap: 보이는 평면 주소만 읽고 음수X/할당 밖 공급·signed64 넘침의 부분 출력을 지킨다', () => {
    const state = new GumpDrawState(); setGumpDrawClip(state, rect('1,0,2,1')); const pixels = new Uint8Array(4).fill(99); const target = new GumpPixelTarget(pixels, 2, 2, state); target.rawBitmap(Uint8Array.of(10, 20), 2, -1n, { x: 2, y: 1 }, { x: 0, y: 0 }, rect('0,0,2,1')); assert.deepEqual([...pixels], [99, 10, 99, 99]);
    setGumpDrawClip(state, rect('0,0,2,2')); assert.throws(() => target.rawBitmap(Uint8Array.of(10, 20), 2, -1n, { x: 2, y: 1 }, { x: 0, y: 0 }, rect('0,0,2,1')), /주소=-1/); const reads = []; target.rawBitmap(Uint8Array.of(10, 20), 2, -1n, { x: 2, y: 1 }, { x: 0, y: 0 }, rect('0,0,2,1'), (offset) => { reads.push(offset); return 55; }); assert.deepEqual(reads, [-1n]); assert.deepEqual([...pixels], [55, 10, 99, 99]);
    assert.throws(() => target.rawBitmap([], 0, 0n, { x: 0, y: 0 }, { x: 0, y: 0 }, rect('0,0,0,0')), /행 간격/);
    assert.throws(() => target.rawBitmap([], 2, 9223372036854775807n, { x: 2, y: 1 }, { x: 0, y: 0 }, rect('0,0,2,1'), () => 17), /주소 넘침/); assert.equal(pixels[0], 17); assert.equal(pixels[1], 10);
});

test('GumpPixelTarget fill/linesClipped/scaledImage: 현재 clip/절대 체크 위상과 동일 표면의 배율 출력을 유지한다', () => {
    const state = new GumpDrawState(); setGumpDrawClip(state, rect('1,1,4,4')); const pixels = new Uint8Array(25).fill(9); const target = new GumpPixelTarget(pixels, 5, 5, state); target.fill(rect('0,0,5,5'), 273, true);
    // 기존 화면 좌표 위상을 독립 픽셀 기대와 비교한다.
    for (let y = 0; y < 5; y += 1) {
        // 각 열의 실제 화면 홀짝을 직접 대조한다.
        for (let x = 0; x < 5; x += 1) assert.equal(pixels[y * 5 + x], x >= 1 && x < 4 && y >= 1 && y < 4 && ((x ^ y) & 1) ? 17 : 9);
    }
    const before = snapshot(state); target.linesClipped([line(0, 2, 4, 2, 42)], rect('2,2,3,3')); assert.equal(pixels[12], 42); assert.equal(snapshot(state), before);
    setGumpDrawClip(state, rect('0,0,5,5')); const picture = image(2, 1, [20, 30], [255, 255]); target.scaledImage(picture, { rect: rect('0,0,1,0') }, { x: 1, y: 1 }, 0x10000, 0x10000); assert.deepEqual([pixels[6], pixels[7]], [20, 30]);
    target.fill(rect('3,3,5,5'), 0x1ff); assert.deepEqual([pixels[18], pixels[19], pixels[23], pixels[24]], [255, 255, 255, 255]);
});
