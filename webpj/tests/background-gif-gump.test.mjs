// 배경 GIF 창 BackgroundGifGump 의 생성/교체/종료 입력/부착/원시 복사 출력/소멸을 기준선 소스 계약과 손 계산 값으로 검증한다.
// cpppj에도 직접 시험이 없어 원본 직접 관측 수치가 아니다. 순서/경계는 기준선 소스(BackgroundGifGump.cpp)에서 옮겼다. 핵심 본체는 기록 장치와 실제 GumpCreate 로 검사하고,
// 실제 originals/d/Defeat.gif 를 실제 해독기와 GumpPixelTarget.rawBitmap 에 연결하는 한 건도 포함한다(원본 파일은 읽기만 한다, 없으면 건너뛴다).
// 실제 게임·브라우저 실행이 아니다. (2026-11-14 WB04-E444)
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { GumpCreate, GumpCreateNode, GumpCreateState } from '../www/ui/gump-create.js';
import { BackgroundGifGump, BackgroundGifGumpNode, BackgroundGifGumpState } from '../www/ui/background-gif-gump.js';
import { GumpDrawState, GumpPixelTarget } from '../www/render/gump-draw.js';
import { decodeBackgroundGif } from '../www/assets/gif.js';

// 실제 GumpCreate 위의 기록 틀이다. 파일 표는 이름 → { width, height }, 위치는 (10, 20) 이다. 적재 토큰은 0x72000000 부터 늘어난다.
function rig({ files = { 'gif/intro.gif': { width: 64, height: 48 } }, kind = 3 } = {}) {
    const log = []; const nodes = new Map(); const bitmaps = new Map(); const createState = new GumpCreateState();
    const factory = new GumpCreate(createState, { node: (token) => nodes.get(token) ?? null });
    const state = new BackgroundGifGumpState(); state.kind = kind; let nextBitmap = 0x72000000;
    const hooks = {
        node: (token) => nodes.get(token) ?? null, base: (token) => nodes.get(token) ?? null,
        constructBase: (token) => { log.push(['constructBase', token]); factory.construct(token, 0, 0, 0, 1); },
        destructBase: (token) => { log.push(['destructBase', token]); },
        pathSpec: (name) => { log.push(['pathSpec', name]); return `gif/${name}.gif`; },
        exists: (file) => { log.push(['exists', file]); return Object.hasOwn(files, file); },
        load: (file, outputs) => { log.push(['load', file]); const entry = files[file]; outputs.width = entry.width; outputs.height = entry.height; const token = nextBitmap; nextBitmap += 1; bitmaps.set(token, entry); return token; },
        releaseBitmap: (token) => { log.push(['release', token]); bitmaps.delete(token); },
        position: () => { log.push(['position']); return { x: 10, y: 20 }; },
        copy: (request) => { log.push(['copy', request.position.x, request.position.y, request.bitmap, request.stride, request.source.left, request.source.top, request.source.right, request.source.bottom]); },
        attach: (token, x, y, parent, mode) => { log.push(['attach', token, x, y, parent, mode]); },
        baseHandle: (token, event, first, second) => { log.push(['base', token, event, first, second]); return 0x1234; },
        dispatch: (event, first, second) => { log.push(['dispatch', event, first, second]); },
    };
    const node = new BackgroundGifGumpNode(); nodes.set(1, node);
    const gump = new BackgroundGifGump(state, hooks);
    return { gump, state, hooks, nodes, node, log, bitmaps, createState, factory };
}

test('BackgroundGifGumpNode: 새 노드의 종료 입력 세 DWORD 와 bitmap 은 0 이고 공통 Gump 필드를 상속한다', () => {
    const node = new BackgroundGifGumpNode(); assert.deepEqual([node.event, node.first, node.second, node.bitmap], [0, 0, 0, 0]);
    assert.ok(node instanceof GumpCreateNode); assert.equal(node.vtable, 0); assert.equal(new BackgroundGifGumpState().kind, 3);
});

test('생성자: node/base 조회가 없으면 TypeError, 나머지 장치는 도달한 분기에서만 요구한다', () => {
    assert.throws(() => new BackgroundGifGump(new BackgroundGifGumpState(), { base: () => null }), { name: 'TypeError', message: /같은 실제 노드/ });
    assert.throws(() => new BackgroundGifGump(new BackgroundGifGumpState(), { node: () => null }), { name: 'TypeError' });
    assert.throws(() => new BackgroundGifGump(new BackgroundGifGumpState(), undefined), TypeError);
    const gump = new BackgroundGifGump(new BackgroundGifGumpState(), { node: () => null, base: () => null });
    assert.ok(gump); assert.throws(() => gump.draw(1), { name: 'Error', message: /공유 노드 없음/ });
});

test('construct: 기본 생성(0,0,0,1) 뒤 가상 표/종류 3/bitmap/치수를 쓰고 종료 입력 세 필드는 적재 뒤 0 이다', () => {
    const r = rig(); r.node.event = 9; r.node.first = 8; r.node.second = 7; assert.equal(r.gump.construct(1, 'intro'), 1);
    assert.deepEqual(r.log.map((entry) => entry[0]), ['constructBase', 'pathSpec', 'exists', 'load']);
    assert.deepEqual([r.node.vtable, r.node.command, r.node.bitmap, r.node.event, r.node.first, r.node.second], [0x501b60, 3, 0x72000000, 0, 0, 0]);
    assert.deepEqual([r.node.baseWidth, r.node.width, r.node.baseHeight, r.node.height, r.node.scale], [64, 64, 48, 48, 1]);
    // 공통 생성의 등록(번호 0, 개수 1)과 플래그는 그대로 남는다.
    assert.deepEqual([r.createState.count, r.createState.entries[0], r.node.flags & 1], [1, 1, 1]);
    assert.deepEqual(r.log[1], ['pathSpec', 'intro']); assert.deepEqual(r.log[2], ['exists', 'gif/intro.gif']); assert.deepEqual(r.log[3], ['load', 'gif/intro.gif']);
});

test('construct 종류: signed BYTE 를 부호 확장한 DWORD 로 쓰고 호출 시점의 값을 읽는다', () => {
    for (const [kind, command] of [[3, 3], [0, 0], [-1, 0xffffffff], [0x80, 0xffffff80], [-128, 0xffffff80], [127, 127], [255, 0xffffffff], [256, 0]]) {
        const r = rig({ kind }); r.gump.construct(1, 'intro'); assert.equal(r.node.command, command, String(kind));
    }
    const r = rig(); r.gump.construct(1, 'intro'); r.state.kind = 21; r.gump.construct(1, 'intro'); assert.equal(r.node.command, 21);
});

test('construct 재사용 저장소: 이전 bitmap 값은 기본 생성 뒤 0 으로 지워져 반납하지 않고, 적재 토큰은 부호 없는 DWORD 로 저장한다', () => {
    const r = rig(); r.node.bitmap = 0x77; r.node.event = 5; r.gump.construct(1, 'intro'); assert.equal(r.log.some((entry) => entry[0] === 'release'), false); assert.equal(r.node.bitmap, 0x72000000);
    r.hooks.load = (file, outputs) => { outputs.width = 1; outputs.height = 1; return -1; }; r.gump.refresh(1, 'intro'); assert.equal(r.node.bitmap, 0xffffffff);
});

test('construct 파일 없음: bitmap 0·치수 0 이며 적재하지 않고 오류가 아니다', () => {
    const r = rig({ files: {} }); r.gump.construct(1, 'none');
    assert.deepEqual(r.log.map((entry) => entry[0]), ['constructBase', 'pathSpec', 'exists']);
    assert.deepEqual([r.node.bitmap, r.node.baseWidth, r.node.width, r.node.baseHeight, r.node.height, r.node.event], [0, 0, 0, 0, 0, 0]);
});

test('construct 저장소 확인: 토큰 0/저장소 없음/기본 노드와 다른 객체는 Error 이고 아무 장치도 부르지 않는다', () => {
    const r = rig(); for (const token of [0, 2]) { r.log.length = 0; assert.throws(() => r.gump.construct(token, 'intro'), { name: 'Error', message: /공유 노드 없음/ }, String(token)); assert.deepEqual(r.log, []); }
    r.hooks.base = () => new BackgroundGifGumpNode(); r.log.length = 0; assert.throws(() => r.gump.construct(1, 'intro'), /공유 노드 없음/); assert.deepEqual(r.log, []);
});

test('construct 노드 재조회: 기본 생성이 저장소 항목을 바꾸면 이후 쓰기는 새 노드에 간다', () => {
    const r = rig(); const replacement = new BackgroundGifGumpNode(); const original = r.node;
    const construct = r.hooks.constructBase; r.hooks.constructBase = (token) => { r.nodes.set(token, replacement); construct(token); };
    r.gump.construct(1, 'intro'); assert.equal(replacement.vtable, 0x501b60); assert.equal(replacement.bitmap, 0x72000000); assert.equal(original.vtable, 0);
});

test('construct 필수 장치: 도달한 위치에서만 Error 다(기본 생성/경로/존재/적재)', () => {
    for (const name of ['constructBase', 'pathSpec', 'exists', 'load']) {
        const r = rig(); delete r.hooks[name]; assert.throws(() => r.gump.construct(1, 'intro'), { name: 'Error', message: /실제 장치 미연결/ }, name);
    }
    // 파일이 없으면 적재 장치는 필요 없다.
    const r = rig({ files: {} }); delete r.hooks.load; delete r.hooks.releaseBitmap; delete r.hooks.copy; r.gump.construct(1, 'none'); assert.equal(r.node.bitmap, 0);
});

test('refresh: 이전 버퍼를 먼저 반납하고 0 으로 쓴 뒤 다시 조회/적재한다. 새 파일이 없으면 bitmap 은 0 으로 남는다', () => {
    const r = rig({ files: { 'gif/a.gif': { width: 10, height: 20 }, 'gif/b.gif': { width: 30, height: 40 } } }); r.gump.construct(1, 'a'); r.log.length = 0;
    r.gump.refresh(1, 'b'); assert.deepEqual(r.log.map((entry) => entry[0]), ['release', 'pathSpec', 'exists', 'load']); assert.deepEqual(r.log[0], ['release', 0x72000000]);
    assert.deepEqual([r.node.bitmap, r.node.baseWidth, r.node.baseHeight], [0x72000001, 30, 40]); assert.equal(r.bitmaps.has(0x72000000), false);
    r.log.length = 0; r.gump.refresh(1, 'missing'); assert.deepEqual(r.log.map((entry) => entry[0]), ['release', 'pathSpec', 'exists']); assert.deepEqual([r.node.bitmap, r.node.baseWidth, r.node.baseHeight, r.node.width], [0, 0, 0, 0]);
    // 버퍼가 없으면 반납하지 않는다.
    r.log.length = 0; r.gump.refresh(1, 'a'); assert.deepEqual(r.log.map((entry) => entry[0]), ['pathSpec', 'exists', 'load']);
});

test('refresh 반납 장치: 이전 bitmap 이 있을 때만 필수이고, 반납 장치가 던지면 bitmap 은 그대로 남는다', () => {
    const r = rig(); r.gump.construct(1, 'intro'); const held = r.node.bitmap;
    r.hooks.releaseBitmap = () => { throw new Error('반납 실패'); }; assert.throws(() => r.gump.refresh(1, 'intro'), /반납 실패/); assert.equal(r.node.bitmap, held);
    delete r.hooks.releaseBitmap; assert.throws(() => r.gump.refresh(1, 'intro'), /실제 장치 미연결/);
    r.node.bitmap = 0; r.gump.refresh(1, 'intro'); assert.equal(r.node.bitmap !== 0, true);
});

test('refresh 크기: 기본 크기는 정수 치수의 float32, 표시 크기는 정수×배율의 배정도 곱을 float32 로 저장한다', () => {
    const r = rig({ files: { 'gif/big.gif': { width: 16777217, height: 3 } } }); r.node.scale = 1.5; r.gump.refresh(1, 'big');
    assert.equal(r.node.baseWidth, 16777216); assert.equal(r.node.width, Math.fround(16777217 * 1.5)); assert.equal(r.node.width, 25165826); assert.notEqual(r.node.width, Math.fround(16777216 * 1.5));
    assert.equal(r.node.baseHeight, 3); assert.equal(r.node.height, 4.5);
    const half = rig(); half.node.scale = Math.fround(0.3); half.gump.refresh(1, 'intro'); assert.equal(half.node.width, Math.fround(64 * Math.fround(0.3))); assert.equal(half.node.height, Math.fround(48 * Math.fround(0.3)));
    assert.equal(half.node.baseWidth, 64); assert.equal(half.node.baseHeight, 48);
});

test('refresh 출력 DWORD: 적재가 쓰지 않으면 0, 음수/32비트 값은 부호 있게 읽고, bitmap 0 반환도 치수는 남는다', () => {
    const r = rig(); r.node.scale = 1; r.hooks.load = (file, outputs) => { outputs.width = -5; outputs.height = 0x80000000; return 0; }; r.gump.refresh(1, 'intro');
    assert.deepEqual([r.node.bitmap, r.node.baseWidth, r.node.baseHeight, r.node.width, r.node.height], [0, -5, -2147483648, -5, -2147483648]);
    r.hooks.load = () => 0x72ffffff; r.gump.refresh(1, 'intro'); assert.deepEqual([r.node.bitmap, r.node.baseWidth, r.node.baseHeight], [0x72ffffff, 0, 0]);
});

test('setEvent: 입력/둘째/첫째를 DWORD 로 쓰고 기존 플래그에 0x600 을 더한다', () => {
    const r = rig(); r.node.flags = 0x80000001; r.gump.setEvent(1, 5, 6, 7); assert.deepEqual([r.node.event, r.node.first, r.node.second, r.node.flags], [5, 6, 7, 0x80000601]);
    r.gump.setEvent(1, -1, -2, 0x1_0000_0003); assert.deepEqual([r.node.event, r.node.first, r.node.second], [0xffffffff, 0xfffffffe, 3]);
    // 기존 플래그가 없으면 정확히 0x600 만 켠다(0x200/0x400 둘 다).
    const fresh = rig(); fresh.node.flags = 0; fresh.gump.setEvent(1, 1, 1, 1); assert.equal(fresh.node.flags, 0x600); fresh.node.flags = 0x200; fresh.gump.setEvent(1, 1, 1, 1); assert.equal(fresh.node.flags, 0x600);
    assert.throws(() => rig().gump.setEvent(2, 1, 1, 1), /공유 노드 없음/);
});

test('reattach: 같은 현재 x/y 로 부모 0·모드 0 부착을 호출한다(장치 없으면 Error)', () => {
    const r = rig(); r.node.x = 12.5; r.node.y = -3; r.gump.reattach(1); assert.deepEqual(r.log, [['attach', 1, 12.5, -3, 0, 0]]);
    delete r.hooks.attach; assert.throws(() => r.gump.reattach(1), /실제 장치 미연결/);
});

test('handle 종료 입력: 입력이 있고 해제가 아닌 왼쪽/오른쪽 버튼/ESC 는 세 DWORD 를 큐에 넘기고 1, 기본 입력은 부르지 않는다', () => {
    for (const code of [0x10000, 0x20000, 27, 0x04010000, 0x0400001b, 0x40010000, 0x0c020000]) {
        const r = rig(); r.gump.setEvent(1, 0x80000000, 0x11, 0x22); r.log.length = 0;
        assert.equal(r.gump.handle(1, code, 5, 6), 1, code.toString(16)); assert.deepEqual(r.log, [['dispatch', 0x80000000, 0x11, 0x22]]);
    }
});

test('handle 위임: 종료 입력이 없거나 해제 비트가 있거나 다른 코드면 기본 입력으로 위임하고 그 DWORD 를 돌려준다', () => {
    const none = rig(); assert.equal(none.gump.handle(1, 0x10000, 1, 2), 0x1234); assert.deepEqual(none.log, [['base', 1, 0x10000, 1, 2]]);
    for (const event of [0x80010000, 0x80000000 | 27, 0x10001, 0x20001, 26, 28, 0x08000000 | 0x10000 | 0x10000000, 0x4000000 | 0x10000 | 0x08000000 | 0, 0, 1]) {
        const r = rig(); r.gump.setEvent(1, 3, 4, 5); r.log.length = 0;
        const expected = ((event & 0x03ffffff) === 0x10000 || (event & 0x03ffffff) === 0x20000 || (event & 0x03ffffff) === 27) && !(event & 0x80000000) ? 1 : 0x1234;
        assert.equal(r.gump.handle(1, event, 7, 8), expected, event.toString(16));
    }
    const r = rig(); r.gump.setEvent(1, 3, 4, 5); r.log.length = 0; r.hooks.baseHandle = () => -2; assert.equal(r.gump.handle(1, 0x80010000, 7, 8), 0xfffffffe);
});

test('handle 경계: 코드 마스크 위 비트(0x04000000)는 무시하고, 해제 비트는 DWORD 최상위만 보며, 큐가 노드를 지워도 다시 읽지 않는다', () => {
    const masked = rig(); masked.gump.setEvent(1, 1, 2, 3); masked.log.length = 0; assert.equal(masked.gump.handle(1, 0x04010000, 0, 0), 1); assert.deepEqual(masked.log, [['dispatch', 1, 2, 3]]);
    const high = rig(); high.gump.setEvent(1, 1, 2, 3); high.log.length = 0; assert.equal(high.gump.handle(1, 0x40010000, 0, 0), 1); assert.deepEqual(high.log, [['dispatch', 1, 2, 3]]);
    const inside = rig(); inside.gump.setEvent(1, 1, 2, 3); inside.log.length = 0; assert.equal(inside.gump.handle(1, 0x02010000, 0, 0), 0x1234); assert.deepEqual(inside.log, [['base', 1, 0x02010000, 0, 0]]);
    // 큐가 처리 도중 파생 노드를 저장소에서 지우고 값도 바꾼다: 큐에는 호출 전 값이 전달되고 handle 은 노드를 다시 조회하지 않는다.
    const gone = rig(); gone.gump.setEvent(1, 0x44, 0x55, 0x66); gone.hooks.dispatch = (event, first, second) => { gone.log.push(['dispatch', event, first, second]); gone.nodes.delete(1); };
    gone.log.length = 0; assert.equal(gone.gump.handle(1, 27, 0, 0), 1); assert.deepEqual(gone.log, [['dispatch', 0x44, 0x55, 0x66]]);
    // 입력 값은 부호 있는 수로 들어와도 DWORD 로 판정한다.
    const negative = rig(); negative.gump.setEvent(1, 1, 2, 3); negative.log.length = 0; assert.equal(negative.gump.handle(1, -2147483648 + 0x10000, 0, 0), 0x1234);
});

test('handle 필수 장치와 저장소: 종료 입력 경로는 dispatch, 위임 경로는 baseHandle 만 요구하고 저장소가 없으면 Error 다', () => {
    const a = rig(); a.gump.setEvent(1, 1, 2, 3); delete a.hooks.baseHandle; delete a.hooks.dispatch; assert.throws(() => a.gump.handle(1, 27, 0, 0), /실제 장치 미연결/);
    assert.equal(rig().gump.handle(1, 0x10000, 0, 0), 0x1234);
    const b = rig(); delete b.hooks.baseHandle; assert.throws(() => b.gump.handle(1, 0x10000, 0, 0), /실제 장치 미연결/); assert.throws(() => rig().gump.handle(3, 27, 0, 0), /공유 노드 없음/);
});

test('draw: bitmap 이 없으면 위치도 읽지 않고 반환하고, 있으면 위치 뒤 기본 크기(배율 크기 아님)의 ftol 로 폭/높이/행 간격/원본 사각형을 복사한다', () => {
    const empty = rig(); empty.gump.draw(1); assert.deepEqual(empty.log, []);
    const r = rig(); r.gump.construct(1, 'intro'); r.node.scale = 0.5; r.node.width = 32; r.node.height = 24; r.log.length = 0; r.gump.draw(1);
    assert.deepEqual(r.log, [['position'], ['copy', 10, 20, 0x72000000, 64, 0, 0, 64, 48]]);
    // 부분 값은 0 쪽으로 절삭하고(63.9→63, -1.5→-1), 큰/무효 값은 하위 DWORD 규칙(0)이다.
    const t = rig(); t.node.bitmap = 7; t.node.baseWidth = 63.9; t.node.baseHeight = -1.5; t.gump.draw(1); assert.deepEqual(t.log[1], ['copy', 10, 20, 7, 63, 0, 0, 63, -1]);
    t.log.length = 0; t.node.baseWidth = NaN; t.node.baseHeight = Infinity; t.gump.draw(1); assert.deepEqual(t.log[1], ['copy', 10, 20, 7, 0, 0, 0, 0, 0]);
});

test('draw 재조회: 위치 장치가 bitmap/기본 크기를 바꾸면 그 값으로 복사하고, 필수 장치는 도달한 곳에서만 요구한다', () => {
    const r = rig(); r.node.bitmap = 5; r.node.baseWidth = 8; r.node.baseHeight = 9;
    r.hooks.position = () => { r.node.bitmap = 6; r.node.baseWidth = 10; r.node.baseHeight = 11; return { x: 1, y: 2 }; }; r.gump.draw(1); assert.deepEqual(r.log, [['copy', 1, 2, 6, 10, 0, 0, 10, 11]]);
    const none = rig(); none.gump.draw(1);
    const noPosition = rig(); noPosition.node.bitmap = 5; delete noPosition.hooks.position; assert.throws(() => noPosition.gump.draw(1), /실제 장치 미연결/);
    const noCopy = rig(); noCopy.node.bitmap = 5; delete noCopy.hooks.copy; assert.throws(() => noCopy.gump.draw(1), /실제 장치 미연결/); assert.deepEqual(noCopy.log, [['position']]);
});

test('destruct: 가상 표를 다시 쓰고 bitmap 이 있으면 반납한 뒤 기본 소멸을 호출하며 bitmap 필드는 0 으로 쓰지 않는다', () => {
    const r = rig(); r.gump.construct(1, 'intro'); r.node.vtable = 0x1234; r.log.length = 0; r.gump.destruct(1);
    assert.deepEqual(r.log, [['release', 0x72000000], ['destructBase', 1]]); assert.equal(r.node.vtable, 0x501b60); assert.equal(r.node.bitmap, 0x72000000);
    const none = rig({ files: {} }); none.gump.construct(1, 'none'); none.log.length = 0; delete none.hooks.releaseBitmap; none.gump.destruct(1); assert.deepEqual(none.log, [['destructBase', 1]]);
    const lacking = rig(); lacking.node.bitmap = 3; delete lacking.hooks.releaseBitmap; assert.throws(() => lacking.gump.destruct(1), /실제 장치 미연결/); assert.deepEqual(lacking.log, []);
    const noBase = rig(); delete noBase.hooks.destructBase; assert.throws(() => noBase.gump.destruct(1), /실제 장치 미연결/); assert.throws(() => rig().gump.destruct(9), /공유 노드 없음/);
});

// 원본 보호 자료의 실제 GIF 를 읽기만 한다(없으면 건너뛴다).
const DEFEAT = path.join(REPO_ROOT, 'originals', 'd', 'Defeat.gif');
test('실제 Defeat.gif: 실제 해독기/치수/GumpPixelTarget.rawBitmap 에 연결해 위치·클립 안에 색 번호가 그대로 복사된다', { skip: !fs.existsSync(DEFEAT) }, () => {
    const decoded = decodeBackgroundGif(new Uint8Array(fs.readFileSync(DEFEAT)));
    const pitch = decoded.width + 40; const height = decoded.height + 30; const surface = new Uint8Array(pitch * height).fill(0xee);
    const drawState = new GumpDrawState(); Object.assign(drawState.vfxClip, { left: 0, top: 0, right: pitch - 1, bottom: height - 1 }); const target = new GumpPixelTarget(surface, pitch, height, drawState);
    const r = rig({ files: { 'gif/real.gif': { width: decoded.width, height: decoded.height } } });
    r.hooks.position = () => ({ x: 15, y: 9 });
    r.hooks.copy = (request) => target.rawBitmap(decoded.indices, request.stride, request.source.top * request.stride + request.source.left, { x: request.source.right - request.source.left, y: request.source.bottom - request.source.top }, request.position, { left: -0x80000000, top: -0x80000000, right: 0x7fffffff, bottom: 0x7fffffff });
    r.gump.construct(1, 'real'); r.gump.draw(1);
    assert.deepEqual([r.node.baseWidth, r.node.baseHeight], [decoded.width, decoded.height]);
    for (const [x, y] of [[0, 0], [1, 0], [decoded.width - 1, 0], [0, decoded.height - 1], [decoded.width - 1, decoded.height - 1], [Math.floor(decoded.width / 2), Math.floor(decoded.height / 2)]]) {
        assert.equal(surface[(y + 9) * pitch + x + 15], decoded.indices[y * decoded.width + x], `${x},${y}`);
    }
    // 위치 밖(왼쪽 15칸/위쪽 9줄/오른쪽 25칸/아래쪽 21줄)은 손대지 않는다.
    assert.equal(surface[0], 0xee); assert.equal(surface[(9 - 1) * pitch + 15], 0xee); assert.equal(surface[9 * pitch + 14], 0xee); assert.equal(surface[9 * pitch + 15 + decoded.width], 0xee); assert.equal(surface[(9 + decoded.height) * pitch + 15], 0xee);
});
