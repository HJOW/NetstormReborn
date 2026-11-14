// MetaImageList 의 경계(열 1 격자·홀수 경계 적중·셀 높이 25·0번 클릭·삭제 뒤 잔여 ID)를 손 계산 값으로 고정한다. (2026-11-14 WB04-E440)
// cpppj에도 직접 시험이 없어 원본 직접 관측 수치가 아니다. 순서/경계는 기준선 소스(MetaImageList.cpp)에서 옮겼다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { GumpCreate, GumpCreateState } from '../www/ui/gump-create.js';
import { MetaImageList, MetaImageListNode } from '../www/ui/meta-image-list.js';
import { MetaImageGumpNode } from '../www/ui/meta-image-gump.js';

// meta-image-list.test.mjs 와 같은 틀이며 bounds 를 바꿀 수 있다.
function build(count, columns = 0, bounds = { left: 100, top: 50, right: 300, bottom: 150 }, baseWidth = 200, baseHeight = 100) {
    const log = []; const nodes = new Map(); const state = new GumpCreateState(); const factory = new GumpCreate(state, { node: (token) => nodes.get(token) ?? null }); const list = new MetaImageListNode(); nodes.set(1, list); let next = 100;
    const hooks = {
        node: (token) => (nodes.get(token) instanceof MetaImageListNode ? nodes.get(token) : null), image: (token) => (nodes.get(token) instanceof MetaImageGumpNode ? nodes.get(token) : null),
        createImage: (image) => { const token = next; next += 1; const child = new MetaImageGumpNode(); child.scale = 1; child.image = image; nodes.set(token, child); return token; },
        attach: (...args) => log.push(['attach', ...args]), baseAttach() {}, push() {}, kill: (token) => log.push(['kill', token]), destructBase() {}, bounds: () => ({ ...bounds }),
        redraw: (...args) => log.push(['redraw', ...args]), notify: (...args) => log.push(['notify', ...args]), green: () => 0x1234, fill: (rect, color) => log.push(['fill', rect.left, rect.top, rect.right, rect.bottom, color]),
    };
    const gump = new MetaImageList(factory, hooks); gump.construct(1, columns); list.baseWidth = baseWidth; list.baseHeight = baseHeight;
    for (let index = 0; index < count; index += 1) gump.add(1, { index }, 10 * (index + 1));
    log.length = 0; return { gump, list, log };
}

test('열 수 1 은 격자다: 셀 너비=기본 너비, 셀 높이=기본 높이/개수이며 적중/그리기도 격자 계산을 쓴다', () => {
    const r = build(3, 1); r.gump.attach(1, 0, 0, 0, 0); const stepY = Math.fround(100 / 3); assert.deepEqual([r.list.cellWidth, r.list.cellHeight], [200, stepY]);
    assert.deepEqual(r.log.filter((entry) => entry[0] === 'attach').map((entry) => [entry[2], entry[3]]), [[0, 0], [0, stepY], [0, Math.fround(stepY + stepY)]]);
    // dy=40 → 40/33.333332=1.2 → 행 1 → 인덱스 1(가로 배치였다면 count*dx/기본 너비로 0).
    assert.deepEqual([r.gump.hit(1, 110, 90), r.gump.hit(1, 250, 140)], [1, 2]);
    r.log.length = 0; r.list.selection = 2; r.gump.draw(1); assert.deepEqual(r.log, [['fill', 100, 116, 300, 149, 0x1234]]);
});

test('hit/handle: 오른쪽/아래 경계가 홀수여도 마지막 칸(경계-1)은 안이고 좌표는 그대로 비교한다', () => {
    const odd = { left: 101, top: 51, right: 301, bottom: 151 }; const r = build(4, 0, odd);
    assert.deepEqual([r.gump.hit(1, 300, 60), r.gump.hit(1, 301, 60), r.gump.hit(1, 150, 150), r.gump.hit(1, 150, 151), r.gump.hit(1, 100, 60)], [3, -1, 0, -1, -1]);
    r.gump.handle(1, 0x10000, 300, 60); assert.equal(r.list.selection, 3); r.gump.handle(1, 0x10000, 150, 150); assert.equal(r.list.selection, 0);
    // 짝수 dy 가 셀 높이 25 의 경계 직전이어도 행을 올리지 않는다(dy=24 → 행 0).
    const grid = build(8, 2); grid.gump.attach(1, 0, 0, 0, 0); assert.deepEqual([grid.list.cellWidth, grid.list.cellHeight], [100, 25]);
    assert.deepEqual([grid.gump.hit(1, 110, 74), grid.gump.hit(1, 110, 75), grid.gump.hit(1, 210, 125)], [0, 2, 7]);
});

test('handle/id: 0번 칸도 선택/Redraw 하고 삭제 뒤 count 와 같은 인덱스의 ID 는 0 이다(잔여 값은 읽지 않는다)', () => {
    const r = build(4); r.gump.handle(1, 0x10000, 110, 60); assert.equal(r.list.selection, 0); assert.deepEqual(r.log, [['redraw', 1, false], ['notify', 1, 0x02000004, 0, 1]]);
    r.gump.remove(1, 3); assert.equal(r.list.ids[3], 40); assert.equal(r.gump.id(1, 3), 0); assert.equal(r.gump.id(1, 2), 30);
});
