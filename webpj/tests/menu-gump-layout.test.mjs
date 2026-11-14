// 전용 PE fixture가 없는 MenuGumpLayout(항목 추가/문자열 설정/측정/전체 레이아웃)을 기준선 소스 계약과 손으로 계산한 값으로 검증한다. (2026-11-14 WB04-E428)
// cpppj에도 직접 시험이 없어 원본 직접 관측 수치가 아니다. 호출 순서/경계는 기준선 소스(MenuGumpLayout.cpp)에서 옮겼다. 메뉴 크기 처리기/기본 생성 저장소는 실제이고 측정/StyleText/제목/부착은 호출 기록 대역이다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { MenuGumpLayout, menuGumpItemBytes, bindMenuLayoutText } from '../www/ui/menu-gump-layout.js';
import { MenuGumpItem, MenuGumpNode } from '../www/ui/menu-gump.js';
import { layoutScene, only, words } from './menu-gump-support.mjs';

const TEXT = 0x10000; const COMMAND = 1; const SEPARATOR = 0x10; const CHILD = 0x20; const PARAMETER = 0x80; const DISABLED = 0x200; const STYLED = 0x400; const DETACHED = 0x800;
const VERTICAL = 0x80000; const HORIZONTAL = 0x40000; const COMPACT = 0x1000000; const FANCY_MENU = 0xa80000;
// 라벨 바이트를 문자열로 읽는다(첫 NUL 앞까지).
const labelOf = (item) => Buffer.from(item.label.subarray(0, item.label.indexOf(0) < 0 ? 40 : item.label.indexOf(0))).toString('latin1');

test('생성자/노드 검증: 두 조회 경계가 필수이고 기본/파생 주소가 같은 노드만 허용한다', () => {
    const s = layoutScene();
    for (const missing of ['node', 'baseNode']) { const hooks = { ...s.layoutHooks }; delete hooks[missing]; assert.throws(() => new MenuGumpLayout(s.create, s.menu, hooks), /레이아웃 공유 저장소 필요/, missing); }
    assert.throws(() => s.layout.reflow(99), /메뉴 레이아웃 저장소 없음/); assert.throws(() => s.layout.reflow(0), /메뉴 레이아웃 저장소 없음/);
    const split = new MenuGumpLayout(s.create, s.menu, { ...s.layoutHooks, baseNode: () => ({}) }); assert.throws(() => split.append(2, 'a', 0), /메뉴 레이아웃 저장소 없음/);
});

test('menuGumpItemBytes: 라벨 40바이트 뒤 다섯 DWORD 를 리틀 엔디언으로 직렬화하고 종료 NUL 을 더하지 않는다', () => {
    const item = new MenuGumpItem(); item.label.fill(0x41); Object.assign(item, { childId: 0x01020304, command: 0xa0b0c0d0, parameter: 0x11223344, styled: 0x55667788, flags: 0xfffefdfc });
    const bytes = menuGumpItemBytes(item); assert.equal(bytes.length, 60); assert.ok(bytes.subarray(0, 40).every((byte) => byte === 0x41));
    assert.deepEqual(Array.from(bytes.subarray(40)), [4, 3, 2, 1, 0xd0, 0xc0, 0xb0, 0xa0, 0x44, 0x33, 0x22, 0x11, 0x88, 0x77, 0x66, 0x55, 0xfc, 0xfd, 0xfe, 0xff]);
    // 반환은 사본이며 항목을 바꾸지 않는다.
    bytes[0] = 0; assert.equal(item.label[0], 0x41);
});

test('append: 문자열 비트를 더해 한 항목을 쓰고 명령/인자는 해당 비트일 때만 쓰며 항목 수와 레이아웃을 갱신한다', () => {
    const s = layoutScene({ sizes: { 0: [10, 4], 1: [30, 7] } }); Object.assign(s.menuNode, { menuFlags: VERTICAL });
    s.layout.append(2, 'Open', 0, 5, 6); const first = s.menuNode.items[0];
    // 명령/인자 비트가 없으면 command/parameter 는 쓰지 않는다.
    assert.deepEqual([first.flags, first.command, first.parameter, first.childId, first.styled], [TEXT, 0, 0, 0, 0]); assert.equal(labelOf(first), 'Open'); assert.equal(s.menuNode.itemCount, 1);
    s.layout.append(2, 'Save', COMMAND | PARAMETER, 5, 6); const second = s.menuNode.items[1];
    assert.deepEqual([second.flags, second.command, second.parameter], [COMMAND | PARAMETER | TEXT, 5, 6]);
    s.layout.append(2, 'Quit', COMMAND, 9, 8); assert.deepEqual([s.menuNode.items[2].command, s.menuNode.items[2].parameter], [9, 0]);
    s.layout.append(2, 'Help', PARAMETER, 9, 8); assert.deepEqual([s.menuNode.items[3].command, s.menuNode.items[3].parameter], [0, 8]);
    assert.equal(s.menuNode.itemCount, 4); assert.deepEqual(only(s.log, 'plain').length > 0, true);
    // 인자를 생략하면 명령/인자 비트가 있어도 0 을 쓴다.
    s.menuNode.items[4].command = 9; s.menuNode.items[4].parameter = 9; s.layout.append(2, 'Zed', COMMAND | PARAMETER); assert.deepEqual([s.menuNode.items[4].command, s.menuNode.items[4].parameter], [0, 0]);
    // 매 추가마다 전체 Reflow: 세로 메뉴 폭/목표/offset 이 현재 항목 수로 갱신된다.
    assert.equal(s.menuNode.itemWidth, 60); assert.equal(s.menuNode.targetWidth, 60);
});

test('append: 40개가 차면 조용히 생략하고 항목 수가 signed 음수이면 칸 범위로 거부한다', () => {
    const s = layoutScene(); s.menuNode.itemCount = 40; s.layout.append(2, 'x', COMMAND, 1); assert.equal(s.menuNode.itemCount, 40); assert.equal(s.menuNode.items[39].command, 0); assert.deepEqual(s.log, []);
    s.menuNode.itemCount = 41; s.layout.append(2, 'x', COMMAND, 1); assert.equal(s.menuNode.itemCount, 41); assert.deepEqual(s.log, []);
    s.menuNode.itemCount = 39; s.layout.append(2, 'x', 0); assert.equal(s.menuNode.itemCount, 40); assert.equal(labelOf(s.menuNode.items[39]), 'x');
    // 0xffffffff 는 signed -1 이라 한도를 통과하지만 칸 0xffffffff 는 범위 밖이다(명령 쓰기 전 거부).
    const t = layoutScene(); t.menuNode.itemCount = 0xffffffff; assert.throws(() => t.layout.append(2, 'x', COMMAND, 1), /메뉴 항목 범위 오류/); assert.equal(t.menuNode.items[0].command, 0);
    assert.throws(() => t.layout.append(2, 'x', 0), /메뉴 항목 범위 오류/);
});

test('append(구분선): 선두/연속은 조용히 생략하고 비어 있지 않은 문자열/자식은 거부한다', () => {
    const s = layoutScene(); s.layout.append(2, '', SEPARATOR); assert.equal(s.menuNode.itemCount, 0); assert.deepEqual(s.log, []);
    s.layout.append(2, 'A', 0); s.layout.append(2, '', SEPARATOR | COMMAND, 3); assert.equal(s.menuNode.itemCount, 2);
    assert.deepEqual([s.menuNode.items[1].flags, s.menuNode.items[1].command], [SEPARATOR | COMMAND | TEXT, 3]);
    // 직전이 구분선이면 두 번째는 생략되고 쓰기도 없다.
    s.layout.append(2, '', SEPARATOR | COMMAND, 77); assert.equal(s.menuNode.itemCount, 2); assert.equal(s.menuNode.items[2].command, 0);
    s.layout.append(2, 'B', 0); assert.equal(s.menuNode.itemCount, 3);
    assert.throws(() => s.layout.append(2, 'x', SEPARATOR), (error) => error instanceof TypeError && /구분선/.test(error.message)); assert.throws(() => s.layout.append(2, '', SEPARATOR, 0, 0, 5), /구분선/);
    // 널 문자열은 구분선 검사를 통과하지만 문자열 설정에서 거부한다(이미 쓴 flags 는 유지, 항목 수는 그대로).
    assert.throws(() => s.layout.append(2, null, SEPARATOR), /메뉴 항목 문자열 필요/); assert.equal(s.menuNode.items[3].flags, SEPARATOR | TEXT); assert.equal(s.menuNode.itemCount, 3);
    // NUL 로 시작하는 문자열/바이트 배열은 빈 문자열이다(직전이 일반 항목일 때만 구분선이 추가된다).
    s.layout.append(2, '\0abc', SEPARATOR); assert.equal(s.menuNode.itemCount, 4); s.layout.append(2, 'C', 0); s.layout.append(2, Uint8Array.of(0, 65), SEPARATOR); assert.equal(s.menuNode.itemCount, 6);
});

test('append(자식): 패치는 ID 저장→숨김→가상 Attach(자식,0,0,메뉴,0)→자식 비트이고 분리 자식은 ID 만 쓴다', () => {
    const s = layoutScene(); s.childA.flags = 0x8000000b; s.childA.id = 7;
    s.layout.append(2, 'Sub', 0, 0, 0, 5); assert.equal(s.childA.flags, 0x8000000a);
    const item = s.menuNode.items[0]; assert.equal(item.childId, 7); assert.equal(item.flags, CHILD | TEXT); assert.equal(s.childA.flags & 1, 0); assert.deepEqual(only(s.log, 'lattach'), [['lattach', 5, 0, 0, 2, 0]]);
    // Attach 시점에 이미 ID 가 저장되고 숨김 처리가 끝나 있다.
    const t = layoutScene(); t.childA.flags |= 1; t.childA.id = 8; let seen = null; t.on.lattach = () => { seen = [t.menuNode.items[0].childId, t.childA.flags & 1]; }; t.layout.append(2, 'Sub', 0, 0, 0, 5); assert.deepEqual(seen, [8, 0]);
    // 분리 자식은 ID 만 쓰고 숨김/Attach/자식 비트를 생략한다.
    const u = layoutScene(); u.childA.flags |= 1; u.childA.id = 9; u.layout.append(2, 'Det', DETACHED, 0, 0, 5);
    assert.deepEqual([u.menuNode.items[0].childId, u.menuNode.items[0].flags, u.childA.flags & 1], [9, DETACHED | TEXT, 1]); assert.deepEqual(only(u.log, 'lattach'), []);
    // 자식 저장소가 없으면 변경 없이 거부하고 Attach 경계가 없으면 ID/숨김을 쓴 뒤 거부한다.
    assert.throws(() => u.layout.append(2, 'x', 0, 0, 0, 99), /메뉴 자식\/제목 저장소 없음/);
    const v = layoutScene({ omit: ['attach'] }); v.childA.flags |= 1; v.childA.id = 6; assert.throws(() => v.layout.append(2, 'x', 0, 0, 0, 5), /메뉴 자식 Attach 경계 필요/); assert.deepEqual([v.menuNode.items[0].childId, v.childA.flags & 1, v.menuNode.itemCount], [6, 0, 0]);
});

test('append: 콜백이 바꾼 현재 항목 수를 다시 읽어 뒤의 쓰기에 사용한다', () => {
    const s = layoutScene(); s.childA.id = 4; s.on.lattach = () => { s.menuNode.itemCount = 3; };
    s.layout.append(2, 'Sub', COMMAND, 12, 0, 5); assert.equal(s.menuNode.items[0].childId, 4); assert.deepEqual([s.menuNode.items[0].command, s.menuNode.items[3].flags], [12, CHILD | COMMAND | TEXT]);
    assert.equal(labelOf(s.menuNode.items[3]), 'Sub'); assert.equal(s.menuNode.itemCount, 4);
    // 문자열 설정 콜백(StyleText parse)이 항목 수를 바꾸면 증가는 현재 값에서 한다.
    const t = layoutScene(); t.on.parse = () => { t.menuNode.itemCount = 10; }; t.layout.append(2, 'S', STYLED); assert.equal(t.menuNode.itemCount, 11);
});

test('setText(일반): 40바이트만 복사하고 NUL 뒤를 0 으로 채우며 40번째 뒤에 종료 문자를 쓰지 않는다', () => {
    const s = layoutScene(); const item = s.menuNode.items[0]; item.label.fill(0x7a);
    s.layout.setText(2, 0, 'abc', words()); assert.deepEqual(Array.from(item.label.subarray(0, 5)), [97, 98, 99, 0, 0]); assert.ok(item.label.subarray(3).every((byte) => byte === 0));
    const long = 'A'.repeat(45); item.label.fill(0x7a); s.layout.setText(2, 0, long, words()); assert.ok(item.label.every((byte) => byte === 0x41));
    // 40바이트 정확히면 NUL 이 없다. 39바이트면 마지막 칸이 0 이다.
    item.label.fill(0x7a); s.layout.setText(2, 0, 'B'.repeat(39), words()); assert.equal(item.label[39], 0); assert.equal(item.label[38], 0x42);
    // 바이트 문자열의 상위 바이트와 Uint8Array 입력, 중간 NUL 이후 무시.
    s.layout.setText(2, 0, '\xe9\xff', words()); assert.deepEqual(Array.from(item.label.subarray(0, 3)), [0xe9, 0xff, 0]);
    s.layout.setText(2, 0, Uint8Array.of(0x41, 0, 0x42, 0), words()); assert.deepEqual(Array.from(item.label.subarray(0, 3)), [0x41, 0, 0]);
    // 널 문자열은 거부하고(라벨 보존) 일반 경로는 StyleText 경계를 부르지 않는다.
    item.label.fill(0x7a); assert.throws(() => s.layout.setText(2, 0, null, words()), /메뉴 항목 문자열 필요/); assert.equal(item.label[0], 0x7a); assert.deepEqual(s.log, []);
    assert.throws(() => s.layout.setText(2, 40, 'x', words()), /메뉴 항목 범위 오류/);
});

test('setText(스타일): 문맥 font=0 사본→Clear→(비활성이면 ~*~x 접두)→Parse 순서이고 256바이트 버퍼 한도를 지킨다', () => {
    const s = layoutScene(); const item = s.menuNode.items[1]; item.flags = STYLED | TEXT; const context = words(3, 4, 5, 6, 7, 8, 9, 10);
    s.layout.setText(2, 1, 'Hi ~B', context);
    assert.deepEqual(s.log, [['clear', 2, 1], ['parse', 2, 1, 'Hi ~B', [3, 0, 5, 6, 7, 8, 9, 10]]]); assert.deepEqual(Array.from(context), [3, 4, 5, 6, 7, 8, 9, 10]);
    s.log.length = 0; item.flags = STYLED | DISABLED | TEXT; s.layout.setText(2, 1, 'Off', context); assert.equal(s.log[1][3], '~*~xOff');
    // 명령 비트(bit0)만 있는 스타일 항목은 비활성이 아니다.
    s.log.length = 0; item.flags = STYLED | COMMAND | TEXT; s.layout.setText(2, 1, 'Cmd', context); assert.equal(s.log[1][3], 'Cmd');
    // 일반 라벨은 건드리지 않는다.
    assert.ok(item.label.every((byte) => byte === 0));
    // 255자는 통과하고 256자(접두 포함)는 Clear 뒤 Parse 전에 거부한다.
    s.log.length = 0; item.flags = STYLED | TEXT; s.layout.setText(2, 1, 'a'.repeat(255), context); assert.equal(s.log.length, 2);
    s.log.length = 0; assert.throws(() => s.layout.setText(2, 1, 'a'.repeat(256), context), (error) => error instanceof RangeError && /한도/.test(error.message)); assert.deepEqual(s.log, [['clear', 2, 1]]);
    s.log.length = 0; item.flags = STYLED | DISABLED | TEXT; s.layout.setText(2, 1, 'a'.repeat(251), context); assert.equal(s.log.length, 2); s.log.length = 0; assert.throws(() => s.layout.setText(2, 1, 'a'.repeat(252), context), /한도/);
    // 첫 NUL 뒤는 처리하지 않는다.
    s.log.length = 0; item.flags = STYLED | TEXT; s.layout.setText(2, 1, 'ab\0cd', context); assert.equal(s.log[1][3], 'ab');
    for (const omit of ['clearStyle', 'parseStyle']) { const t = layoutScene({ omit: [omit] }); t.menuNode.items[0].flags = STYLED; assert.throws(() => t.layout.setText(2, 0, 'x', words()), /StyleText/, omit); }
});

test('measureItem: 읽는 시점의 항목 플래그로 styled/plain 을 고르고 결과는 signed 사본이다', () => {
    const s = layoutScene({ sizes: { 0: [7, 3], 1: [9, 4] } }); const item = s.menuNode.items[1]; item.flags = STYLED;
    const context = words(1, 2); assert.deepEqual(s.layout.measureItem(2, 1, context), [9, 4]); assert.deepEqual(only(s.log, 'styled'), [['styled', 2, 1]]); assert.deepEqual(only(s.log, 'plain'), []);
    s.log.length = 0; assert.deepEqual(s.layout.measureItem(2, 0, context), [7, 3]); assert.deepEqual(only(s.log, 'plain'), [['plain', 2, 0, [1, 2, 0, 0, 0, 0, 0, 0]]]);
    // 짝수 측정 결과도 그대로다.
    s.on.plain = () => [8, 6]; assert.deepEqual(s.layout.measureItem(2, 0, context), [8, 6]); s.on.styled = () => [12, 10]; assert.deepEqual(s.layout.measureItem(2, 1, context), [12, 10]); s.on.styled = null;
    // 측정 결과는 DWORD 하위값이다(33비트 이상 입력의 signed 변환).
    s.on.plain = () => [0xffffffff, 2 ** 31]; assert.deepEqual(s.layout.measureItem(2, 0, context), [-1, -(2 ** 31)]);
    // 돌려준 배열을 바꿔도 장치의 기본 결과는 바뀌지 않는다.
    s.on.plain = null; const first = s.layout.measureItem(2, 0, context); first[0] = 99; assert.deepEqual(s.layout.measureItem(2, 0, context), [7, 3]);
    assert.throws(() => s.layout.measureItem(2, 40, context), /메뉴 항목 범위 오류/);
    for (const [omit, flags] of [['styled', STYLED], ['plain', 0]]) { const t = layoutScene({ omit: [omit] }); t.menuNode.items[0].flags = flags; assert.throws(() => t.layout.measureItem(2, 0, words()), /측정 경계 필요/, omit); }
});

test('reflow(세로): 여백 15/15/1/1, 항목 크기=측정+여백(절삭), 자식 표식 폭 합/높이 최대, 목표 크기와 누적 offset', () => {
    const s = layoutScene({ sizes: { 0: [10, 4], 1: [30, 7], 2: [20, 9] } }); Object.assign(s.menuNode, { menuFlags: VERTICAL, itemCount: 3 }); s.menuNode.items[2].flags = CHILD | TEXT;
    s.menuNode.offsets.fill(77); s.menuNode.heights.fill(88); s.layout.reflow(2);
    assert.deepEqual(Array.from(s.menuNode.margins), [15, 15, 1, 1]); assert.deepEqual(Array.from(s.menuNode.heights.subarray(0, 4)), [6, 9, 11, 88]); assert.deepEqual(Array.from(s.menuNode.offsets.subarray(0, 4)), [0, 6, 15, 77]);
    assert.deepEqual([s.menuNode.targetWidth, s.menuNode.targetHeight, s.menuNode.itemWidth], [60, 26, 60]);
    // 메뉴 크기는 실제 MenuGump 가 목표 크기로 쓴다.
    assert.deepEqual([s.menuNode.baseWidth, s.menuNode.baseHeight, s.menuNode.width, s.menuNode.height], [60, 26, 60, 26]);
    // 자식 표식(6×5)은 폭에 더해지고 높이는 큰 쪽을 쓴다: 항목2 폭 20+6+30=56, 높이 max(9,5)+2=11.
    assert.deepEqual(only(s.log, 'marker').map((entry) => entry.slice(1)), [[2, [0, 0, 0, 0, 0, 0, 1, 1]].slice(0, 2)].map(([token]) => [token, Array.from(s.menuNode.textContext)]));
    // 표식 높이가 더 크면 표식 높이다.
    s.log.length = 0; s.measure.markerSize = [6, 20]; s.layout.reflow(2); assert.equal(s.menuNode.heights[2], 22);
    s.measure.markerSize = [6, 9]; s.layout.reflow(2); assert.equal(s.menuNode.heights[2], 11);
    s.measure.markerSize = [6, 8]; s.layout.reflow(2); assert.equal(s.menuNode.heights[2], 11); assert.equal(s.menuNode.heights[1], 9);
});

test('reflow: 분리 자식 비트도 표식을 더하지만 가로 메뉴/자식 비트 없음에는 표식 측정이 없다', () => {
    const s = layoutScene({ sizes: { 0: [10, 4] } }); Object.assign(s.menuNode, { menuFlags: VERTICAL, itemCount: 1 }); s.menuNode.items[0].flags = DETACHED | TEXT;
    s.layout.reflow(2); assert.equal(only(s.log, 'marker').length, 1); assert.equal(s.menuNode.itemWidth, 10 + 6 + 30);
    const h = layoutScene({ sizes: { 0: [10, 4] } }); Object.assign(h.menuNode, { menuFlags: HORIZONTAL, itemCount: 1 }); h.menuNode.items[0].flags = CHILD | TEXT; h.layout.reflow(2); assert.equal(only(h.log, 'marker').length, 0);
    const n = layoutScene({ sizes: { 0: [10, 4] } }); Object.assign(n.menuNode, { menuFlags: VERTICAL, itemCount: 1 }); n.layout.reflow(2); assert.equal(only(n.log, 'marker').length, 0);
    const m = layoutScene(); Object.assign(m.menuNode, { menuFlags: VERTICAL, itemCount: 1 }); m.menuNode.items[0].flags = CHILD; m.layoutHooks.marker = undefined;
    assert.throws(() => new MenuGumpLayout(m.create, m.menu, { ...m.layoutHooks, marker: undefined }).reflow(2), /메뉴 자식 표시 측정 경계 필요/);
});

test('reflow(가로): 여백 4/4/1/1, 목표 폭=항목 수×최대 폭(DWORD 감김), 모든 높이는 최대 높이, offset 은 0', () => {
    const s = layoutScene({ sizes: { 0: [10, 4], 1: [30, 7], 2: [20, 9] } }); Object.assign(s.menuNode, { menuFlags: HORIZONTAL, itemCount: 3 }); s.menuNode.offsets.fill(5);
    s.layout.reflow(2); assert.deepEqual(Array.from(s.menuNode.margins), [4, 4, 1, 1]); assert.deepEqual(Array.from(s.menuNode.heights.subarray(0, 4)), [11, 11, 11, 0]); assert.deepEqual(Array.from(s.menuNode.offsets.subarray(0, 4)), [0, 0, 0, 5]);
    assert.deepEqual([s.menuNode.itemWidth, s.menuNode.targetWidth, s.menuNode.targetHeight], [38, 114, 11]); assert.deepEqual([s.menuNode.baseWidth, s.menuNode.baseHeight], [114, 11]);
    // 항목 수×폭의 정수 곱셈은 하위 32비트만 남긴다(0x10000×0x10000 → 0).
    const t = layoutScene({ sizes: { 0: [0x10000 - 8, 2] } }); Object.assign(t.menuNode, { menuFlags: HORIZONTAL, itemCount: 1 }); t.layout.reflow(2); assert.equal(t.menuNode.targetWidth, 0x10000);
    const u = layoutScene({ sizes: { 0: [0x7ffff - 8, 2], 1: [0, 0], 2: [0, 0] } }); Object.assign(u.menuNode, { menuFlags: HORIZONTAL, itemCount: 0x20 }); u.layout.reflow(2); assert.equal(u.menuNode.targetWidth, Math.fround(Math.imul(0x20, 0x7ffff)));
});

test('reflow(compact/기타): compact 는 좌우 여백 3, 방향 비트가 없으면 목표/높이/offset 을 보존하고 항목 폭만 쓴다', () => {
    const s = layoutScene({ sizes: { 0: [10, 4] } }); Object.assign(s.menuNode, { menuFlags: VERTICAL | COMPACT, itemCount: 1 }); s.layout.reflow(2);
    assert.deepEqual(Array.from(s.menuNode.margins), [3, 3, 1, 1]); assert.equal(s.menuNode.itemWidth, 10 + 6);
    const t = layoutScene({ sizes: { 0: [10, 4] } }); Object.assign(t.menuNode, { menuFlags: COMPACT, itemCount: 1, targetWidth: 5, targetHeight: 6 }); t.menuNode.offsets[0] = 3; t.menuNode.heights[0] = 99; t.layout.reflow(2);
    assert.deepEqual(Array.from(t.menuNode.margins), [3, 3, 1, 1]); assert.deepEqual([t.menuNode.targetWidth, t.menuNode.targetHeight, t.menuNode.offsets[0], t.menuNode.itemWidth], [5, 6, 3, 10 + 6]);
    // 방향 비트가 없을 때 높이 배열은 측정으로 갱신되지만 크기 처리기는 호출되지 않는다.
    assert.equal(t.menuNode.heights[0], 6); assert.deepEqual([t.menuNode.baseWidth, t.menuNode.baseHeight], [50, 40]);
    // 비-compact 의 세로 아닌 메뉴 여백은 4/4/1/1 이다.
    const u = layoutScene({ sizes: { 0: [0, 0] } }); u.layout.reflow(2); assert.deepEqual(Array.from(u.menuNode.margins), [4, 4, 1, 1]); assert.equal(u.menuNode.itemWidth, 0);
});

test('reflow: menuFlags 의 bit0 만 있으면 세로/가로/장식/compact 로 보지 않고 목표 크기·offset 을 보존한다', () => {
    const s = layoutScene({ sizes: { 0: [10, 4] } }); Object.assign(s.menuNode, { menuFlags: 1, itemCount: 1, targetWidth: 5, targetHeight: 6 }); s.menuNode.offsets[0] = 3;
    s.layout.reflow(2); assert.deepEqual(Array.from(s.menuNode.margins), [4, 4, 1, 1]); assert.deepEqual([s.menuNode.targetWidth, s.menuNode.targetHeight, s.menuNode.offsets[0], s.menuNode.itemWidth], [5, 6, 3, 18]);
    assert.deepEqual(only(s.log, 'lpush'), []); assert.deepEqual([s.menuNode.baseWidth, s.menuNode.baseHeight], [50, 40]);
    // 가로 메뉴의 목표 높이는 음수 측정 높이로 낮아지지 않고 0 이상의 최대다.
    const h = layoutScene({ sizes: { 0: [10, -5] } }); Object.assign(h.menuNode, { menuFlags: HORIZONTAL, itemCount: 1 }); h.layout.reflow(2); assert.equal(h.menuNode.targetHeight, 0); assert.equal(h.menuNode.heights[0], 0);
});

test('reflow: 큰 측정값은 CRT 절삭/DWORD 감김, 음수 폭은 최대에 쓰이지 않으며 signed 음수 항목 수는 반복하지 않는다', () => {
    const s = layoutScene({ sizes: { 0: [0x7fffffff, 1], 1: [-100, -50] } }); Object.assign(s.menuNode, { menuFlags: VERTICAL, itemCount: 2 }); s.layout.reflow(2);
    // 0x7fffffff+30 은 signed 32비트로 감겨 음수가 되고 최대 폭 0 에 못 미친다.
    assert.equal(s.menuNode.heights[1], -48); assert.equal(s.menuNode.heights[0], 3); assert.equal(s.menuNode.itemWidth, 0); assert.equal(s.menuNode.targetHeight, -45);
    assert.equal(s.menuNode.offsets[1], 3);
    const t = layoutScene(); Object.assign(t.menuNode, { menuFlags: VERTICAL, itemCount: 0xffffffff, targetWidth: 9 }); t.layout.reflow(2); assert.deepEqual(only(t.log, 'plain'), []); assert.equal(t.menuNode.targetWidth, 0); assert.equal(t.menuNode.itemWidth, 0);
    const u = layoutScene({ sizes: { 0: [Math.fround(1e9), 0] } }); Object.assign(u.menuNode, { menuFlags: VERTICAL, itemCount: 1 }); u.layout.reflow(2);
    assert.equal(u.menuNode.heights[0], 2); assert.equal(u.menuNode.itemWidth, Math.fround(1e9 + 30));
});

test('reflow: 측정/크기 변경 콜백이 바꾼 현재 항목 수·문맥을 다음 반복에서 다시 읽는다', () => {
    const s = layoutScene({ sizes: { 0: [10, 4], 1: [10, 4], 2: [10, 4] } }); Object.assign(s.menuNode, { menuFlags: VERTICAL, itemCount: 3 }); s.on.plain = (token, index) => { if (index === 0) s.menuNode.itemCount = 2; return null; };
    s.layout.reflow(2); assert.equal(only(s.log, 'plain').length, 2); assert.equal(s.menuNode.targetHeight, 12);
    // 항목마다 현재 기본 문맥을 복사해 전달한다.
    const t = layoutScene({ sizes: { 0: [1, 1], 1: [1, 1] } }); Object.assign(t.menuNode, { menuFlags: VERTICAL, itemCount: 2 }); t.menuNode.textContext.set([0, 1, 2, 3, 4, 5, 6, 7]); t.on.plain = (token, index) => { if (index === 0) t.menuNode.textContext[3] = 99; return null; };
    t.layout.reflow(2); assert.deepEqual(only(t.log, 'plain').map((entry) => entry[3][3]), [3, 99]);
    // 크기 변경 뒤 반복 수를 다시 읽는다(장식 Push 콜백이 항목 수를 줄인다).
    const u = layoutScene({ sizes: { 0: [10, 4], 1: [10, 4], 2: [10, 4] } }); Object.assign(u.menuNode, { menuFlags: FANCY_MENU, itemCount: 3, fancy: 3, title: 4 }); u.on.push = () => { u.menuNode.itemCount = 1; }; u.menuNode.offsets.fill(7); u.layout.reflow(2);
    assert.deepEqual(Array.from(u.menuNode.offsets.subarray(0, 3)), [0, 7, 7]);
});

test('reflow(장식): 제목 Push→Fit(0)→Layout(0,0)→폭=max(항목 폭,제목 기본 폭) 절삭→제목 Attach 뒤 세로 크기 쓰기', () => {
    const s = layoutScene({ sizes: { 0: [10, 4] } }); Object.assign(s.menuNode, { menuFlags: FANCY_MENU, itemCount: 1, fancy: 3, title: 4 }); s.title.baseWidth = Math.fround(70.9); s.title.x = 7; s.title.y = 9;
    s.layout.reflow(2);
    assert.deepEqual(s.log.slice(0, 5), [['plain', 2, 0, [0, 0, 0, 0, 0, 0, 1, 1]], ['lpush', 4], ['titleFit', 4, 0], ['titleLayout', 4, 0, 0], ['lattach', 4, 7, 9, 0, 0]]);
    assert.equal(s.menuNode.itemWidth, 70); assert.equal(s.menuNode.targetWidth, 70);
    // 이어서 메뉴 크기 처리기가 장식을 갱신한다(Push→Attach).
    assert.deepEqual(s.log.slice(5).map((entry) => entry[0]), ['push', 'attach', 'push', 'attach']);
    // 항목 폭이 더 크면 항목 폭을 쓰고, 제목 폭이 NaN 이면 0 으로 절삭한다.
    const t = layoutScene({ sizes: { 0: [100, 4] } }); Object.assign(t.menuNode, { menuFlags: FANCY_MENU, itemCount: 1, fancy: 3, title: 4 }); t.layout.reflow(2); assert.equal(t.menuNode.itemWidth, 130);
    const u = layoutScene({ sizes: { 0: [100, 4] } }); Object.assign(u.menuNode, { menuFlags: FANCY_MENU, itemCount: 1, fancy: 3, title: 4 }); u.title.baseWidth = NaN; u.layout.reflow(2); assert.equal(u.menuNode.itemWidth, 0);
    const w = layoutScene({ sizes: { 0: [100, 4] } }); Object.assign(w.menuNode, { menuFlags: FANCY_MENU, itemCount: 1, fancy: 3, title: 4 }); w.title.baseWidth = 1e19; w.layout.reflow(2); assert.equal(w.menuNode.itemWidth, 0);
});

test('reflow: 장식 경계·방향 비트 오류는 도달한 지점에서 거부한다', () => {
    const s = layoutScene({ sizes: { 0: [10, 4] } }); Object.assign(s.menuNode, { menuFlags: FANCY_MENU | HORIZONTAL, itemCount: 1, fancy: 3, title: 4 });
    assert.throws(() => s.layout.reflow(2), (error) => error instanceof TypeError && /장식 메뉴의 세로 방향 필요/.test(error.message)); assert.deepEqual(only(s.log, 'lpush'), []); assert.equal(s.menuNode.itemWidth, 0);
    const t = layoutScene({ sizes: { 0: [10, 4] } }); Object.assign(t.menuNode, { menuFlags: VERTICAL | HORIZONTAL, itemCount: 1 }); assert.throws(() => t.layout.reflow(2), /일반 세로 메뉴의 가로 비트 없음 필요/); assert.equal(t.menuNode.targetWidth, 0);
    for (const [omit, pattern] of [['push', /제목 Push/], ['titleFit', /제목 Fit/], ['titleLayout', /제목 Layout/], ['attach', /제목 Attach/]]) {
        const u = layoutScene({ sizes: { 0: [10, 4] }, omit: [omit] }); Object.assign(u.menuNode, { menuFlags: FANCY_MENU, itemCount: 1, fancy: 3, title: 4 }); assert.throws(() => u.layout.reflow(2), pattern, omit);
    }
    // 제목 저장소가 없으면 Layout 뒤에 거부한다.
    const v = layoutScene({ sizes: { 0: [10, 4] } }); Object.assign(v.menuNode, { menuFlags: FANCY_MENU, itemCount: 1, fancy: 3, title: 99 }); assert.throws(() => v.layout.reflow(2), /메뉴 자식\/제목 저장소 없음/); assert.deepEqual(only(v.log, 'titleLayout'), [['titleLayout', 99, 0, 0]]);
});

test('bindMenuLayoutText: 항목 60바이트와 자식 표식(" >\\0")을 전달 문맥의 실제 글꼴 측정기로 보내고 signed 크기를 돌려준다', () => {
    const calls = []; const text = { measure: (bytes, context, output) => { calls.push([Array.from(bytes), context]); output.width = bytes.length * 2; output.height = context.font + 7; return output; } };
    const hooks = { node: (token) => (token === 2 ? Object.assign(new MenuGumpNode(), {}) : null) }; const node = hooks.node(2); void node;
    const store = new Map([[2, new MenuGumpNode()]]); store.get(2).items[3].label.set([0x41, 0x42]); store.get(2).items[3].command = 0x0201;
    const bound = { node: (token) => store.get(token) }; bindMenuLayoutText(bound, text);
    assert.deepEqual(bound.plain(2, 3, words(0x2, 5, 7, 1, 2, 3, 4, 5)), [120, 12]);
    assert.deepEqual(calls[0][0].slice(0, 3), [0x41, 0x42, 0]); assert.deepEqual(calls[0][0].slice(44, 48), [1, 2, 0, 0]);
    assert.deepEqual(calls[0][1], { flags: 2, font: 5, color: 7, style: 1, extra: 2, shadowColor: 3, shadowX: 4, shadowY: 5 });
    assert.deepEqual(bound.marker(2, words(0, 9)), [6, 16]); assert.deepEqual(calls[1][0], [0x20, 0x3e, 0]);
    assert.throws(() => bound.plain(9, 0, words()), /메뉴 글자 측정 항목 범위 오류/); assert.throws(() => bound.plain(2, 40, words()), /메뉴 글자 측정 항목 범위 오류/); assert.throws(() => bindMenuLayoutText({}, text), /메뉴 글자 측정 저장소 필요/);
    // 문맥은 길이 8 이어야 한다.
    assert.throws(() => bound.marker(2, [1, 2]), TypeError);
});
