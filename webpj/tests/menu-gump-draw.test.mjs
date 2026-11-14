// 전용 PE fixture가 없는 MenuGumpDraw(메뉴 배경/강조/구분선/체크 그림/항목 글자/하위 메뉴 표시)를 기준선 소스 계약과 손으로 계산한 좌표로 검증한다. (2026-11-14 WB04-E430)
// cpppj에도 직접 시험이 없어 원본 직접 관측 수치가 아니다. 호출 순서/경계는 기준선 소스(MenuGumpDraw.cpp)에서 옮겼다. 위치/경계 수집기·클립 보호·메뉴 레이아웃·기본 생성 저장소는 실제이고 배경/글자/StyleText/체크 그림은 호출 기록 대역이다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { MenuGumpDraw, MenuGumpDrawState, bindMenuGumpDraw, bindMenuGumpDrawAssets } from '../www/ui/menu-gump-draw.js';
import { GumpCollection } from '../www/render/gump-collection.js';
import { GumpDrawState, setGumpDrawClip } from '../www/render/gump-draw.js';
import { layoutScene, only } from './menu-gump-support.mjs';

const TEXT = 0x10000; const CHECKED = 4; const SELECTED = 8; const SEPARATOR = 0x10; const CHILD = 0x20; const BOLD = 0x40; const DISABLED = 0x200; const STYLED = 0x400; const DYNAMIC = 0x800;
const VERTICAL = 0x80000; const COMPACT = 0x1000000; const HIGHLIGHT = 0x2100000; const SEPARATOR_SECOND = 0x2080000;
// 화면 전체 클립(640×480)이다. 메뉴 경계와 교차해 임시 클립이 된다.
const SCREEN = { left: 0, top: 0, right: 640, bottom: 480 };

// 세로 메뉴 장면이다. 메뉴(토큰 2)는 루트 아래 (100,50) 80×60 이고 목표 크기는 60×26(현재와 달라 확장 중 보정이 생김), 항목 3개(offsets 0/6/15, heights 6/9/11)다. 여백은 서로 다른 15/8/2/5 로 둔다.
function drawScene(options = {}) {
    const s = layoutScene({ sizes: options.sizes ?? { 0: [10, 4], 1: [10, 4], 2: [10, 4] } }); const { menuNode } = s;
    s.nodes.get(1).x = 0; s.nodes.get(1).y = 0;
    Object.assign(menuNode, { parent: 1, x: 100, y: 50, width: 80, height: 60, menuFlags: VERTICAL, itemCount: 3, targetWidth: 60, targetHeight: 26, itemWidth: 60, type: 3, frame: 4 });
    menuNode.offsets.set([0, 6, 15]); menuNode.heights.set([6, 9, 11]); menuNode.margins.set([15, 8, 2, 5]); menuNode.textContext.set([5, 6, 7, 8, 9, 10, 11, 12]);
    for (let i = 0; i < 3; i += 1) { menuNode.items[i].flags = TEXT; menuNode.items[i].label.set(Buffer.from(`item${i}`, 'latin1')); }
    const collection = options.collection ?? new GumpCollection(s.create.collection, { node: s.hooks.node }); const clip = new GumpDrawState(); setGumpDrawClip(clip, SCREEN);
    const state = new MenuGumpDrawState(); Object.assign(state, { foreground: 21, disabled: 22, backgroundType: 31, backgroundFrame: 32, markerType: 41 });
    const snapshot = () => ({ ...clip.clip });
    const background = { draw: (token) => s.log.push(['bgDraw', token, snapshot()]), background: (rect, flags, type, frame) => { s.log.push(['background', { ...rect }, flags, type, frame, snapshot()]); return s.on.background?.(rect, flags, type, frame); } };
    const text = { draw: (bytes, point, context) => { s.log.push(['text', Buffer.from(bytes.subarray(0, bytes.indexOf(0) < 0 ? 40 : bytes.indexOf(0))).toString('latin1'), { ...point }, context]); return s.on.text?.(); },
        drawGump: (target, token, bytes, offset, defaultContext, context) => s.log.push(['glyphs', token, Array.from(bytes), { ...offset }, Array.from(defaultContext), context]) };
    const style = { draw: (head, point, override) => s.log.push(['style', head, { ...point }, override]) };
    const hooks = { node: s.hooks.node, base: s.hooks.baseNode, marker: (type, point) => s.log.push(['marker', type, { ...point }]) };
    for (const key of options.omit ?? []) delete hooks[key];
    const draw = new MenuGumpDraw(s.create, clip, state, collection, background, text, style, s.layout, hooks);
    return { ...s, draw, clip, state, drawHooks: hooks, collection, background, text, style, snapshot };
}
// 기록에서 이름의 항목만, 이름 제외한 값으로 돌려준다.
const rows = (log, name) => only(log, name).map((entry) => entry.slice(1));
// 메뉴 경계(100,50)-(180,110) 와 화면의 교차 클립이다.
const MENU_CLIP = { left: 100, top: 50, right: 180, bottom: 110 };

test('생성자/노드 검증: 두 조회 경계가 필수이고 기본/파생 주소가 같은 노드만 허용한다', () => {
    const s = drawScene();
    for (const missing of ['node', 'base']) { const hooks = { ...s.drawHooks }; delete hooks[missing]; assert.throws(() => new MenuGumpDraw(s.create, s.clip, s.state, s.collection, s.background, s.text, s.style, s.layout, hooks), /메뉴 출력 공유 저장소 필요/, missing); }
    const split = new MenuGumpDraw(s.create, s.clip, s.state, s.collection, s.background, s.text, s.style, s.layout, { ...s.drawHooks, base: () => ({}) });
    assert.throws(() => split.draw(2), /메뉴 출력 공유 노드 없음/); assert.deepEqual(s.clip.clip, SCREEN);
    assert.throws(() => s.draw.draw(99), /메뉴 출력 공유 노드 없음|토큰/);
});

test('draw: 경계 클립→기본 배경 Draw→항목 글자 순서이고 클립은 마지막에 화면 전체로 복구된다', () => {
    const s = drawScene(); s.draw.draw(2);
    assert.deepEqual(s.log.slice(0, 1), [['bgDraw', 2, MENU_CLIP]]); assert.deepEqual(s.clip.clip, SCREEN); assert.deepEqual(s.clip.vfxClip, { left: 0, top: 0, right: 639, bottom: 479 });
    // 세로 일반 항목: x = 100+15-(60-80)=135, y = 50+offset+2-(26-60)=86+offset. 문맥은 font=0·색=현재 흰색이다.
    assert.deepEqual(rows(s.log, 'text').map((row) => [row[0], row[1]]), [['item0', { x: 135, y: 86 }], ['item1', { x: 135, y: 92 }], ['item2', { x: 135, y: 101 }]]);
    assert.deepEqual(only(s.log, 'text')[0][3], { flags: 5, font: 0, color: 21, style: 8, extra: 9, shadowColor: 10, shadowX: 11, shadowY: 12 });
    // 문맥은 항목마다 새 복사본이라 노드의 기본 문맥은 바뀌지 않는다.
    assert.deepEqual(Array.from(s.menuNode.textContext), [5, 6, 7, 8, 9, 10, 11, 12]);
});

test('draw: 굵게는 style=1, 비활성은 비활성 색+flags 0, 스타일 항목은 저장 조각을 override 없이 출력한다', () => {
    const s = drawScene(); s.menuNode.items[0].flags = TEXT | BOLD; s.menuNode.items[1].flags = TEXT | DISABLED | BOLD; s.menuNode.items[2].flags = TEXT | STYLED; s.menuNode.items[2].styled = 77; s.draw.draw(2);
    const texts = only(s.log, 'text'); assert.deepEqual(texts[0][3], { flags: 5, font: 0, color: 21, style: 1, extra: 9, shadowColor: 10, shadowX: 11, shadowY: 12 });
    assert.deepEqual(texts[1][3], { flags: 0, font: 0, color: 22, style: 1, extra: 9, shadowColor: 10, shadowX: 11, shadowY: 12 });
    assert.deepEqual(rows(s.log, 'style'), [[77, { x: 135, y: 101 }, null]]);
    // 일반 글자는 항목 60바이트(라벨+필드)를 넘긴다.
    const bytes = []; s.text.draw = (value) => bytes.push(Array.from(value)); s.menuNode.items[0].command = 0x04030201; s.draw.draw(2); assert.equal(bytes[0].length, 60); assert.deepEqual(bytes[0].slice(44, 48), [1, 2, 3, 4]);
});

test('draw(선택 강조): 세로는 로컬 [2, offset+2, 폭-2, 높이+offset-2] 를 화면 rect 로 바꿔 클립 후 0x2100000 배경을 현재 메뉴 type/frame 으로 그린다', () => {
    const s = drawScene(); s.menuNode.items[1].flags = TEXT | SELECTED; s.draw.draw(2);
    // 항목1: offset 6, 높이 9 → 로컬 [2,8,78,13] → 화면 [102,58,178,63]. 이 rect 가 메뉴 경계 안이라 클립도 같다.
    const rect = { left: 102, top: 58, right: 178, bottom: 63 };
    assert.deepEqual(rows(s.log, 'background'), [[rect, HIGHLIGHT, 3, 4, rect]]);
    // 강조는 글자보다 먼저, 그 항목의 글자 앞에서 그려진다.
    const names = s.log.map((entry) => entry[0] + (entry[1] === 'item1' ? ':1' : '')); assert.ok(names.indexOf('background') < names.indexOf('text:1'));
    // 구분선 비트가 있으면 강조하지 않는다.
    const t = drawScene(); t.menuNode.items[1].flags = TEXT | SELECTED | SEPARATOR; t.draw.draw(2); assert.deepEqual(rows(t.log, 'background').map((row) => row[1]), [0x2100000, 0x2080000]);
});

test('draw(강조 가로/compact): 가로는 [index×항목 폭+2, 2, (index+1)×항목 폭-2, 높이-2], compact 는 [2,2,폭-2,높이-2] 이며 글자는 항목 폭×index 위치다', () => {
    const s = drawScene(); Object.assign(s.menuNode, { menuFlags: 0x40000, itemWidth: 25 }); s.menuNode.items[1].flags = TEXT | SELECTED; s.draw.draw(2);
    assert.deepEqual(rows(s.log, 'background'), [[{ left: 127, top: 52, right: 148, bottom: 108 }, HIGHLIGHT, 3, 4, { left: 127, top: 52, right: 148, bottom: 108 }]]);
    // 글자: x = ftol(index×25+15+100), y = ftol(50+margins[2]) = 52.
    assert.deepEqual(rows(s.log, 'text').map((row) => row[1]), [{ x: 115, y: 52 }, { x: 140, y: 52 }, { x: 165, y: 52 }]);
    const t = drawScene(); Object.assign(t.menuNode, { menuFlags: VERTICAL | COMPACT }); t.menuNode.items[0].flags = TEXT | SELECTED; t.draw.draw(2);
    assert.deepEqual(rows(t.log, 'background')[0][0], { left: 102, top: 52, right: 178, bottom: 108 });
    // 세 항목 모두 세로 배치 글자 위치를 쓴다(compact 도 세로 비트가 있다).
    assert.deepEqual(rows(t.log, 'text').map((row) => row[1].y), [86, 92, 101]);
});

test('draw(구분선): 세로에서만 두 개의 1픽셀 명암 배경을 그리고 두 번째 pass 는 top/bottom 을 1 증가시키며 전역 type/frame 을 쓴다', () => {
    const s = drawScene(); s.menuNode.items[1].flags = TEXT | SEPARATOR; s.draw.draw(2);
    // y = ftol((6+2)-(26-60)+9*0.5-1) = ftol(8+34+4.5-1) = 45, right = ftol(80+2-4) = 78. 로컬 [2,45,78,46] → 화면 [102,95,178,96].
    const first = { left: 102, top: 95, right: 178, bottom: 96 }; const second = { left: 102, top: 96, right: 178, bottom: 97 };
    assert.deepEqual(rows(s.log, 'background'), [[first, HIGHLIGHT, 31, 32, first], [second, SEPARATOR_SECOND, 31, 32, second]]);
    // 구분선 항목은 글자를 그리지 않는다. 클립은 매 pass 뒤 복구된다.
    assert.deepEqual(rows(s.log, 'text').map((row) => row[0]), ['item0', 'item2']); assert.deepEqual(s.clip.clip, SCREEN);
    // 가로 메뉴에서는 구분선 비트가 있어도 글자로 그린다.
    const t = drawScene(); Object.assign(t.menuNode, { menuFlags: 0x40000, itemWidth: 25 }); t.menuNode.items[1].flags = TEXT | SEPARATOR; t.draw.draw(2); assert.deepEqual(rows(t.log, 'background'), []); assert.equal(only(t.log, 'text').length, 3);
});

test('draw(체크): 세로 체크 항목은 글자 앞에 현재 체크 그림 타입으로 (x+23+5, y+7) 위치에 표식을 출력한다', () => {
    const s = drawScene(); s.menuNode.items[2].flags = TEXT | CHECKED; s.draw.draw(2);
    // x = ftol(15×0.25-(60-80)) = 23, y = ftol(15+2-(26-60)) = 51 → 화면 (100+23+5, 50+51+7).
    assert.deepEqual(rows(s.log, 'marker'), [[41, { x: 128, y: 108 }]]);
    const names = s.log.map((entry) => entry[0] + (entry[1] === 'item2' ? ':2' : '')); assert.ok(names.indexOf('marker') < names.indexOf('text:2'));
    // 가로 메뉴는 체크 비트를 쓰지 않는다. 표식 경계가 없으면 글자 전에 거부한다.
    const t = drawScene({ omit: ['marker'] }); t.menuNode.items[2].flags = TEXT | CHECKED; assert.throws(() => t.draw.draw(2), /메뉴 체크 SHP 출력 경계 필요/); assert.equal(only(t.log, 'text').length, 2); assert.deepEqual(t.clip.clip, SCREEN);
    const u = drawScene(); Object.assign(u.menuNode, { menuFlags: 0x40000, itemWidth: 25 }); u.menuNode.items[2].flags = TEXT | CHECKED; u.draw.draw(2); assert.deepEqual(rows(u.log, 'marker'), []);
});

test('draw(하위 표시): 자식/동적 자식 항목은 글자 뒤에 " >" 를 측정 폭만큼 오른쪽에 같은 글꼴 문맥으로 그린다', () => {
    const s = drawScene({ sizes: { 0: [10, 4], 1: [30, 4], 2: [10, 4] } }); s.menuNode.items[1].flags = TEXT | CHILD; s.menuNode.items[2].flags = TEXT | DYNAMIC | BOLD; s.draw.draw(2);
    // markerX = ftol((15-(60-80))+측정 폭) = 35+폭, markerY = ftol(offset+2-(26-60)) = 36+offset.
    const glyphs = only(s.log, 'glyphs'); assert.deepEqual(glyphs.map((entry) => [entry[1], entry[2], entry[3]]), [[2, [0x20, 0x3e, 0], { x: 65, y: 42 }], [2, [0x20, 0x3e, 0], { x: 45, y: 51 }]]);
    // 기본 문맥(노드)과 해석된 현재 문맥(항목별 굵게/색)을 함께 넘긴다.
    assert.deepEqual(glyphs[0][4], [5, 6, 7, 8, 9, 10, 11, 12]); assert.equal(glyphs[0][5].color, 21); assert.equal(glyphs[1][5].style, 1);
    // 글자 뒤에 표식이 이어진다.
    const names = s.log.map((entry) => entry[0] + (entry[1] === 'item1' ? ':1' : '')); assert.ok(names.indexOf('text:1') < names.indexOf('glyphs'));
    // 가로 메뉴는 하위 표식을 그리지 않는다.
    const t = drawScene(); Object.assign(t.menuNode, { menuFlags: 0x40000, itemWidth: 25 }); t.menuNode.items[1].flags = TEXT | CHILD; t.draw.draw(2); assert.deepEqual(only(t.log, 'glyphs'), []);
});

test('draw: Y 좌표 변환은 패치의 bottom float32 저장·DWORD 감김·절삭을 지킨다', () => {
    // top 은 정수 Y 를 그대로, bottom 은 float32 로 한 번 저장한 Y 를 써서 16777217 에서 한 칸 차이가 난다.
    const collection = { position: () => ({ x: 3, y: 16777217 }), bounds: () => ({ left: 0, top: 0, right: 640, bottom: 480 }) };
    const s = drawScene({ collection }); s.menuNode.items[1].flags = TEXT | SELECTED; s.draw.draw(2);
    // 항목1: 로컬 y [8,13] → top = ftol(16777217+8)=16777225, bottom = ftol(fround(16777217)+13)=16777229.
    assert.deepEqual(rows(s.log, 'background')[0][0], { left: 5, top: 16777225, right: 81, bottom: 16777229 });
    // 위치 덧셈은 DWORD 로 감긴다: 체크 표식 좌표.
    const wrap = { position: () => ({ x: 0x7fffffff, y: 0 }), bounds: () => ({ left: 0, top: 0, right: 640, bottom: 480 }) };
    const t = drawScene({ collection: wrap }); t.menuNode.items[0].flags = TEXT | CHECKED; t.draw.draw(2); assert.equal(rows(t.log, 'marker')[0][1].x, (0x7fffffff + 23 + 5) | 0);
});

test('draw: 출력 콜백이 바꾼 항목 수/플래그를 다음 반복에서 다시 읽고 예외에서도 클립을 복구한다', () => {
    const s = drawScene(); s.on.text = () => { s.menuNode.itemCount = 1; }; s.draw.draw(2); assert.equal(only(s.log, 'text').length, 1);
    // 같은 항목의 플래그를 선택 강조 콜백이 바꿔도 이후 분기는 현재 flags 를 읽는다.
    const t = drawScene(); t.menuNode.items[1].flags = TEXT | SELECTED; t.on.background = () => { t.menuNode.items[1].flags = TEXT | SEPARATOR; }; t.draw.draw(2);
    assert.deepEqual(rows(t.log, 'background').map((row) => row[1]), [HIGHLIGHT, HIGHLIGHT, SEPARATOR_SECOND]); assert.deepEqual(rows(t.log, 'text').map((row) => row[0]), ['item0', 'item2']);
    // 항목 수가 40 을 넘거나 signed 음수이면: 음수는 반복하지 않고 큰 수는 칸 범위로 거부하며 클립을 복구한다.
    const u = drawScene(); u.menuNode.itemCount = 0xffffffff; u.draw.draw(2); assert.equal(only(u.log, 'text').length, 0);
    const v = drawScene(); v.menuNode.itemCount = 41; assert.throws(() => v.draw.draw(2), /메뉴 출력 항목 범위 오류/); assert.deepEqual(v.clip.clip, SCREEN); assert.equal(only(v.log, 'text').length, 40);
});

test('기본값/짝수 항목 수: 전역 기본값은 모두 0 이고 항목 수 2 는 두 항목만 그린다', () => {
    const state = new MenuGumpDrawState(); assert.deepEqual([state.foreground, state.disabled, state.backgroundType, state.backgroundFrame, state.markerType], [0, 0, 0, 0, 0]);
    const s = drawScene(); s.menuNode.itemCount = 2; s.draw.draw(2); assert.deepEqual(rows(s.log, 'text').map((row) => row[0]), ['item0', 'item1']);
});

test('draw: 명령 비트(bit0)만 있는 항목은 체크/선택/구분선/굵게/비활성/스타일/자식이 아니고, 선택+bit0 는 강조한다', () => {
    const s = drawScene(); for (const item of s.menuNode.items.slice(0, 3)) item.flags = TEXT | 1; s.menuNode.items[1].flags = TEXT | 1 | SELECTED; s.menuNode.menuFlags = VERTICAL | 1; s.draw.draw(2);
    assert.deepEqual(rows(s.log, 'marker'), []); assert.deepEqual(rows(s.log, 'style'), []); assert.deepEqual(rows(s.log, 'glyphs'), []);
    assert.deepEqual(rows(s.log, 'background').map((row) => row[1]), [HIGHLIGHT]); assert.deepEqual(rows(s.log, 'background')[0][0], { left: 102, top: 58, right: 178, bottom: 63 });
    assert.deepEqual(only(s.log, 'text').map((row) => [row[1], row[3].color, row[3].style, row[3].flags]), [['item0', 21, 8, 5], ['item1', 21, 8, 5], ['item2', 21, 8, 5]]);
    // 방향 bit0 만 있으면 가로 배치다.
    const t = drawScene(); t.menuNode.menuFlags = 1; t.menuNode.itemWidth = 25; t.draw.draw(2); assert.deepEqual(rows(t.log, 'text').map((row) => row[1]), [{ x: 115, y: 52 }, { x: 140, y: 52 }, { x: 165, y: 52 }]);
});

test('draw: 위치 계산은 CRT 절삭의 무효 입력(NaN/무한대/범위 밖)을 0 으로, 32비트를 넘는 값은 하위 DWORD 로 쓴다', () => {
    const s = drawScene(); Object.assign(s.menuNode, { menuFlags: 0x40000, itemWidth: Infinity }); s.draw.draw(2);
    // index 0 은 0×∞=NaN, 나머지는 ∞ 라 모두 0 이다.
    assert.deepEqual(rows(s.log, 'text').map((row) => row[1].x), [0, 0, 0]);
    const t = drawScene(); Object.assign(t.menuNode, { menuFlags: 0x40000, itemWidth: 1e19 }); t.draw.draw(2); assert.deepEqual(rows(t.log, 'text').map((row) => row[1].x), [115, 0, 0]);
    // 3e9×index+115 의 절삭은 하위 32비트 signed 다(33비트로 읽으면 3000000115).
    const u = drawScene(); Object.assign(u.menuNode, { menuFlags: 0x40000, itemWidth: 3e9 }); u.menuNode.itemCount = 2; u.draw.draw(2); assert.deepEqual(rows(u.log, 'text').map((row) => row[1].x), [115, (3000000115 - 4294967296)]);
});

test('bindMenuGumpDraw: 메뉴만 실제 Draw 로 보내고 다른 타입의 기존 출력은 보존한다', () => {
    const s = drawScene(); const calls = []; const hooks = { draw: (token) => calls.push(token) };
    bindMenuGumpDraw(hooks, (token) => (token === 2 ? s.draw : null)); hooks.draw(7); assert.deepEqual(calls, [7]); assert.equal(only(s.log, 'bgDraw').length, 0);
    hooks.draw(2); assert.equal(only(s.log, 'bgDraw').length, 1); assert.deepEqual(calls, [7]);
    assert.throws(() => bindMenuGumpDraw({}, null), /메뉴 출력 분배 조회 필요/); const bare = {}; bindMenuGumpDraw(bare, () => null); assert.throws(() => bare.draw(1), /메뉴 외 Draw 경계 필요/);
});

test('bindMenuGumpDrawAssets: 체크 표식은 타입의 J 구간 첫 번호+2 물리 프레임을 기존 장식 픽셀 대상에 출력하고 타입 범위를 검사한다', () => {
    const shapes = []; const pixels = { hooks: () => ({ shape: (...args) => shapes.push(args) }) };
    const views = new Map([[72, { first: [0, 0, 0, 0, 0, 0, 0, 0, 5, 11, 0, 0, 0, 0, 0, 0] }], [73, { first: new Array(16).fill(-1) }]]);
    const assets = { types: () => new Array(10).fill(null), typeTable: { view: (type) => views.get(type) } }; const hooks = {}; bindMenuGumpDrawAssets(hooks, assets, pixels);
    hooks.marker(72, { x: 3, y: 4 }); assert.deepEqual(shapes, [[72, 13, { x: 3, y: 4 }]]);
    // J 구간이 없으면 -1+2 = 1 이다.
    hooks.marker(73, { x: 0, y: 0 }); assert.equal(shapes[1][1], 1);
    // 내장 타입(<70)과 표 밖 타입은 거부한다. 경계: 70 은 정상 범위, 79 는 마지막, 80 은 밖이다.
    assert.throws(() => hooks.marker(69, { x: 0, y: 0 }), (error) => error instanceof RangeError && /메뉴 체크 실제 타입 범위 초과/.test(error.message)); assert.throws(() => hooks.marker(80, { x: 0, y: 0 }), /타입 범위 초과/); assert.equal(shapes.length, 2);
    views.set(70, { first: new Array(16).fill(0) }); views.set(79, { first: new Array(16).fill(0) }); hooks.marker(70, { x: 0, y: 0 }); hooks.marker(79, { x: 0, y: 0 }); assert.deepEqual(shapes.slice(2).map((row) => [row[0], row[1]]), [[70, 2], [79, 2]]);
});
