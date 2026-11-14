// 21칸 Image 목록(MetaImageList)의 생성/등록/배치/적중/선택/삭제를 기준선 소스 계약과 손 계산 값으로 검증한다. (2026-11-14 WB04-E440)
// cpppj에도 직접 시험이 없어 원본 직접 관측 수치가 아니다. 순서/경계는 기준선 소스(MetaImageList.cpp)에서 옮겼다. 공통 factory 는 실제 GumpCreate 이고 목록 경계는 (100,50)-(300,150)이다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { GumpCreate, GumpCreateState } from '../www/ui/gump-create.js';
import { MetaImageList, MetaImageListNode, META_IMAGE_LIST_CAPACITY } from '../www/ui/meta-image-list.js';
import { MetaImageGumpNode } from '../www/ui/meta-image-gump.js';

// 실제 GumpCreate 위의 틀이다. createImage 는 100+번호 토큰의 자식 노드(배율 1)를 만들고 log 는 훅 호출을 기록한다.
function rig(columns = 0, baseWidth = 200, baseHeight = 100) {
    const log = []; const nodes = new Map(); const state = new GumpCreateState(); const factory = new GumpCreate(state, { node: (token) => nodes.get(token) ?? null });
    const list = new MetaImageListNode(); nodes.set(1, list); let next = 100; const images = [];
    const hooks = {
        node: (token) => (nodes.get(token) instanceof MetaImageListNode ? nodes.get(token) : null), image: (token) => (nodes.get(token) instanceof MetaImageGumpNode ? nodes.get(token) : null),
        createImage: (image) => { const token = next; next += 1; const child = new MetaImageGumpNode(); child.scale = 1; child.image = image; nodes.set(token, child); images.push(image); log.push(['createImage', token]); return token; },
        attach: (...args) => log.push(['attach', ...args]), baseAttach: (...args) => log.push(['baseAttach', ...args]), push: (token) => log.push(['push', token]), kill: (token) => log.push(['kill', token]), destructBase: (token) => log.push(['destructBase', token]),
        bounds: () => ({ left: 100, top: 50, right: 300, bottom: 150 }), redraw: (token, erase) => log.push(['redraw', token, erase]), notify: (...args) => log.push(['notify', ...args]), green: () => 0x1234, fill: (rect, color) => log.push(['fill', rect.left, rect.top, rect.right, rect.bottom, color]),
    };
    const gump = new MetaImageList(factory, hooks); gump.construct(1, columns); list.baseWidth = baseWidth; list.baseHeight = baseHeight; log.length = 0;
    return { gump, list, nodes, state, hooks, log, images, factory };
}
// n 개의 Image(구별용 객체)를 id 10*번호 로 등록한다.
function filled(count, columns = 0, baseWidth = 200, baseHeight = 100) { const r = rig(columns, baseWidth, baseHeight); for (let index = 0; index < count; index += 1) r.gump.add(1, { index }, 10 * (index + 1)); r.log.length = 0; return r; }
const names = (log) => log.map((entry) => entry[0]);

test('MetaImageListNode: 새 노드는 개수 0·배열 21칸 0·선택 -1·열 0·셀 크기 0 이다', () => {
    const node = new MetaImageListNode(); assert.deepEqual([node.count, node.selection, node.columns, node.cellHeight, node.cellWidth, node.images.length, node.ids.length], [0, -1, 0, 0, 0, 21, 21]); assert.equal(node.images.every((value) => value === 0) && node.ids.every((value) => value === 0), true);
});

test('construct: 공통 기본 생성 뒤 가상 표/종류 22/count 0/이미지 배열 0/플래그 0x200/선택 -1/열 수를 쓰고 ID 배열과 셀 크기는 보존한다', () => {
    const r = rig(); r.list.ids[3] = 9; r.list.cellWidth = 7; r.list.cellHeight = 8; r.list.images[2] = 55; r.list.count = 4; r.list.selection = 3;
    assert.equal(r.gump.construct(1, 4), 1); assert.deepEqual([r.list.vtable, r.list.command, r.list.count, r.list.selection, r.list.columns, r.list.flags & 0x200], [0x50d0d4, 22, 0, -1, 4, 0x200]); assert.equal(r.list.images.every((value) => value === 0), true);
    assert.deepEqual([r.list.ids[3], r.list.cellWidth, r.list.cellHeight], [9, 7, 8]); assert.deepEqual([r.list.flags, r.list.baseWidth, r.list.baseHeight, r.list.width, r.list.height], [0x209, 0, 0, 0, 0]); assert.equal(r.state.entries[0], 1); assert.equal(META_IMAGE_LIST_CAPACITY, 21);
    assert.throws(() => r.gump.construct(9, 0), RangeError); assert.throws(() => r.gump.construct(0, 0), RangeError);
});

test('add: ID 저장 → 자식 생성 → 배열/개수 → 자식 부착 → 목록 Push → 현재 x/y 로 재부착 순서이고 21칸이 차면 false 다', () => {
    const r = rig(); r.list.x = 3; r.list.y = 4; const image = { name: 'a' }; assert.equal(r.gump.add(1, image, 77), true);
    assert.deepEqual(r.log, [['createImage', 100], ['attach', 100, 0, 0, 1, 0], ['push', 1], ['attach', 1, 3, 4, 0, 0]]); assert.deepEqual([r.list.count, r.list.ids[0], r.list.images[0]], [1, 77, 100]);
    for (let index = 1; index < 21; index += 1) assert.equal(r.gump.add(1, { index }, index), true); assert.equal(r.list.count, 21); r.log.length = 0; assert.equal(r.gump.add(1, {}, 0), false); assert.deepEqual(r.log, []);
    // 생성 중 count 가 바뀌어도 ID 는 호출 전 count 칸에, 자식은 생성 뒤 count 칸에 쓴다.
    const w = rig(); w.hooks.createImage = (() => { const original = w.hooks.createImage; return (value) => { w.list.count += 1; return original(value); }; })(); const gump = new MetaImageList(w.factory, w.hooks);
    gump.add(1, {}, 5); assert.deepEqual([w.list.ids[0], w.list.images[1], w.list.count], [5, 100, 2]);
    assert.throws(() => rig().gump.add(9, {}, 0), RangeError); const big = rig(); big.list.count = 22; assert.throws(() => big.gump.add(1, {}, 0), { name: 'RangeError', message: /count 오류/ });
});

test('attach(가로 배치): 셀 너비=기본 너비/개수, 셀 높이=min(기본 높이, 셀 너비)이며 자식을 Push 한 뒤 크기를 float32 로 쓰고 x 만 누적해 부착한다', () => {
    const r = filled(4); r.gump.attach(1, 5, 6, 7, 8); assert.deepEqual([r.list.cellWidth, r.list.cellHeight], [50, 50]);
    assert.deepEqual(r.log, [['baseAttach', 1, 5, 6, 7, 8], ['push', 100], ['attach', 100, 0, 0, 1, 0], ['push', 101], ['attach', 101, 50, 0, 1, 0], ['push', 102], ['attach', 102, 100, 0, 1, 0], ['push', 103], ['attach', 103, 150, 0, 1, 0]]);
    for (const token of [100, 101, 102, 103]) { const child = r.nodes.get(token); assert.deepEqual([child.baseWidth, child.width, child.baseHeight, child.height], [50, 50, 50, 50]); }
    // 하나뿐이면 셀 너비 200 이 기본 높이 100 보다 크므로 셀 높이는 기본 높이다. 자식 배율은 크기에 곱해진다.
    const one = filled(1); one.nodes.get(100).scale = 0.5; one.gump.attach(1, 0, 0, 0, 0); assert.deepEqual([one.list.cellWidth, one.list.cellHeight, one.nodes.get(100).width, one.nodes.get(100).height], [200, 100, 100, 50]);
});

test('attach(격자): 셀 너비=기본 너비/열 수, 셀 높이=기본 높이/행 수(개수/열+나머지)이며 float32 누적 좌표로 열마다 부착하고 한 줄이 차면 다음 행으로 내린다', () => {
    const r = filled(7, 3); r.gump.attach(1, 0, 0, 0, 0); const stepX = Math.fround(200 / 3); const stepY = Math.fround(100 / 3); assert.deepEqual([r.list.cellWidth, r.list.cellHeight], [stepX, stepY]);
    const x1 = Math.fround(stepX); const x2 = Math.fround(x1 + stepX); const y1 = Math.fround(stepY); const y2 = Math.fround(y1 + stepY); const expected = [[0, 0], [x1, 0], [x2, 0], [0, y1], [x1, y1], [x2, y1], [0, y2]];
    const attaches = r.log.filter((entry) => entry[0] === 'attach'); assert.deepEqual(attaches.map((entry) => [entry[2], entry[3]]), expected);
    // 개수가 열 수로 나누어떨어지면 행 수는 몫이다(6개/3열 → 2행).
    const even = filled(6, 3); even.gump.attach(1, 0, 0, 0, 0); assert.equal(even.list.cellHeight, 50);
    // 개수 0 이하는 기본 부착만 하고 돌아온다.
    const none = rig(); none.list.cellWidth = 9; none.list.cellHeight = 8; none.gump.attach(1, 1, 2, 3, 4); assert.deepEqual([none.list.cellWidth, none.list.cellHeight], [9, 8]); none.log.length = 0; none.gump.attach(1, 1, 2, 3, 4); assert.deepEqual(none.log, [['baseAttach', 1, 1, 2, 3, 4]]);
    assert.throws(() => new MetaImageList(none.factory, { ...none.hooks, baseAttach: undefined }).attach(1, 0, 0, 0, 0), { name: 'Error', message: /실제 하위 미연결/ });
});

test('attach: 자식 부착 중 count 가 줄면 현재 count 를 종료 조건으로 읽어 나머지 자식을 건너뛴다', () => {
    const r = filled(4); r.hooks.attach = (token) => { r.log.push(['attach', token]); if (token === 101) r.list.count = 2; }; const gump = new MetaImageList(r.factory, r.hooks); gump.attach(1, 0, 0, 0, 0);
    assert.deepEqual(r.log.filter((entry) => entry[0] === 'attach').map((entry) => entry[1]), [100, 101]);
});

test('hit: 경계 밖(오른쪽/아래 제외)은 -1, 가로는 count×dx 를 정수로 먼저 곱해 기본 너비로 나눈 ftol, 격자는 행×열 수+열이며 개수를 넘으면 -1(같으면 그 값)이다', () => {
    const h = filled(4); assert.deepEqual([h.gump.hit(1, 100, 50), h.gump.hit(1, 299, 149), h.gump.hit(1, 149, 60), h.gump.hit(1, 150, 60), h.gump.hit(1, 300, 60), h.gump.hit(1, 99, 60), h.gump.hit(1, 120, 150), h.gump.hit(1, 120, 49)], [0, 3, 0, 1, -1, -1, -1, -1]);
    const g = filled(4, 3); g.gump.attach(1, 0, 0, 0, 0);
    // 셀 66.666664×50(행 2: 4개/3열 → 2행, 100/2=50). (170,90) → dx=70 → 열 1, dy=40 → 행 0 → 인덱스 1. (190,140) → 열 2, 행 1 → 5 > 4 라서 -1. 인덱스 4 는 개수와 같아 허용된다.
    assert.deepEqual([g.gump.hit(1, 170, 90), g.gump.hit(1, 290, 140), g.gump.hit(1, 110, 110), g.gump.hit(1, 100, 50)], [1, -1, 3, 0]); assert.deepEqual([g.gump.hit(1, 166, 100), g.gump.hit(1, 167, 100)], [3, 4]);
    // 정수 곱 감김: count=21, dx 가 큰 경우도 32비트 곱을 쓴다(기본 너비를 0 으로 두면 무한대/NaN → ftol 0).
    const zero = filled(2); zero.list.baseWidth = 0; assert.equal(zero.gump.hit(1, 150, 60), 0);
    assert.throws(() => rig().gump.hit(9, 0, 0), RangeError);
});

test('handle: 뗌 없는 좌누름(0x10000)만 처리해 선택을 토글하고 Redraw/부모 통지하며 화면 밖 클릭도 현재 비교 결과로 통지한다', () => {
    const r = filled(4); assert.equal(r.gump.handle(1, 0x10000, 160, 60), 1); assert.equal(r.list.selection, 1); assert.deepEqual(r.log, [['redraw', 1, false], ['notify', 1, 0x02000004, 1, 1]]);
    r.log.length = 0; r.gump.handle(1, 0x10000, 160, 60); assert.equal(r.list.selection, -1); assert.deepEqual(r.log, [['redraw', 1, false], ['notify', 1, 0x02000004, 1, 0]]);
    // 다른 칸을 누르면 선택이 옮겨진다.
    r.log.length = 0; r.gump.handle(1, 0x10000, 110, 60); r.gump.handle(1, 0x10000, 260, 60); assert.equal(r.list.selection, 2 + 1);
    // 화면 밖(-1): Redraw 없이 통지(값1=0xffffffff, 값2 는 선택 -1 일 때만 1).
    r.log.length = 0; r.gump.handle(1, 0x10000, 10, 10); assert.deepEqual(r.log, [['notify', 1, 0x02000004, 0xffffffff, 0]]); r.list.selection = -1; r.log.length = 0; r.gump.handle(1, 0x10000, 10, 10); assert.deepEqual(r.log, [['notify', 1, 0x02000004, 0xffffffff, 1]]);
    // 다른 사건/뗌 비트/명령 비트가 섞인 사건은 처리하지 않는다.
    r.log.length = 0; for (const event of [0x20000, 0x1, 0x80010000, 0x02010000]) assert.equal(r.gump.handle(1, event >>> 0, 160, 60), 0, event.toString(16)); assert.deepEqual(r.log, []);
    assert.equal(r.gump.handle(1, 0x10000 | 0x40000000, 160, 60), 1);
});

test('draw: 선택이 없으면 무효과이고 가로는 선택×기본 너비/개수 절삭 좌표, 격자는 (선택%열, 선택/열)×셀 크기 절삭 좌표를 체크무늬로 채운다', () => {
    const r = filled(4); r.gump.draw(1); assert.deepEqual(r.log, []);
    r.list.selection = 1; r.gump.draw(1); assert.deepEqual(r.log, [['fill', 150, 50, 200, 150, 0x1234]]);
    const g = filled(7, 3); g.gump.attach(1, 0, 0, 0, 0); g.log.length = 0; g.list.selection = 4; g.gump.draw(1);
    // 열 1×66.666664+100=166.66666 → 166, 오른쪽 166+66.666664 → 232, 행 1×33.333332+50 → 83, 아래 83+33.333332 → 116.
    assert.deepEqual(g.log, [['fill', 166, 83, 232, 116, 0x1234]]); g.log.length = 0; g.list.selection = 0; g.gump.draw(1); assert.deepEqual(g.log, [['fill', 100, 50, 166, 83, 0x1234]]);
    assert.throws(() => new MetaImageList(r.factory, { ...r.hooks, green: undefined }).draw(1), { name: 'Error', message: /실제 하위 미연결/ });
});

test('id/image/remove: 인덱스는 0..20 만 허용(RangeError)하고 유효 접두부 밖은 0/null/false 이며 삭제는 Kill 예약·접두부 압축·count 만 줄인다', () => {
    const r = filled(4);
    assert.deepEqual([r.gump.id(1, 0), r.gump.id(1, 3), r.gump.id(1, 4), r.gump.id(1, 20)], [10, 40, 0, 0]); for (const index of [-1, 21]) { assert.throws(() => r.gump.id(1, index), { name: 'RangeError', message: /ID 원문 index/ }); assert.throws(() => r.gump.remove(1, index), { name: 'RangeError', message: /삭제 원문 index/ }); }
    assert.deepEqual([r.gump.image(1, 0), r.gump.image(1, 3).index, r.gump.image(1, 4), r.gump.image(1, -1)], [r.images[0], 3, null, null]); assert.equal(r.gump.image(1, 1), r.images[1]);
    r.list.selection = 2; assert.equal(r.gump.remove(1, 1), true); assert.deepEqual(r.log, [['kill', 101]]); assert.deepEqual([r.list.count, r.list.selection], [3, 2]);
    assert.deepEqual(Array.from(r.list.images.subarray(0, 4)), [100, 102, 103, 103]); assert.deepEqual(Array.from(r.list.ids.subarray(0, 4)), [10, 30, 40, 40]);
    assert.deepEqual([r.gump.remove(1, 3), r.gump.remove(1, 5), rig().gump.remove(1, 0)], [false, false, false]); r.log.length = 0; assert.equal(r.gump.remove(1, 2), true); assert.deepEqual(r.log, [['kill', 103]]); assert.equal(r.list.count, 2);
});

test('destruct: 가상 표를 복원한 뒤 실제 기본 소멸자를 부르고 저장소/장치가 없으면 거부한다', () => {
    const r = filled(2); r.list.vtable = 0; r.gump.destruct(1); assert.equal(r.list.vtable, 0x50d0d4); assert.deepEqual(r.log, [['destructBase', 1]]);
    assert.throws(() => r.gump.destruct(9), RangeError); assert.throws(() => new MetaImageList(r.factory, { ...r.hooks, destructBase: undefined }).destruct(1), { name: 'Error', message: /실제 하위 미연결/ });
});
