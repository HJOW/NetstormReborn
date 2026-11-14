// 실제 보호 GIF 자산·설정과 공통 Gump 등록표에 원문 GIF 공장·원시 픽셀·입력 큐·수명을 접합하는 RawBackgroundGifControls 를 검사한다.
// 원본 GIF 34개와 변형 4개는 cpppj 가 원본 LZW 본체로 미리 관측한 독립 색 번호 지문과 대조한다. 원본게임을 실행하지 않는다. (2026-11-14 WB04-E462)
import test from 'node:test';
import assert from 'node:assert/strict';
import crypto from 'node:crypto';
import fs from 'node:fs';
import path from 'node:path';
import { REPO_ROOT, readDataMap } from './helpers.mjs';
import { ARCHIVE_PATH, GameData, SETUP_PATH } from '../www/assets/game-data.js';
import { decodeBackgroundGif, decodeGif } from '../www/assets/gif.js';
import { GumpCreate, GumpCreateNode, GumpCreateState } from '../www/ui/gump-create.js';
import { GumpCollection } from '../www/render/gump-collection.js';
import { GumpLifetime, GumpLifetimeState } from '../www/ui/gump-lifetime.js';
import { GumpTree, GumpTreeState } from '../www/ui/gump-tree.js';
import { GumpEvents, GumpEventState } from '../www/ui/gump-events.js';
import { GumpInput, GumpInputState } from '../www/ui/gump-input.js';
import { GumpDrawState, GumpPixelTarget, setGumpDrawClip } from '../www/render/gump-draw.js';
import { createRawMenuTree } from '../www/ui/raw-menu-tree.js';
import { RawBackgroundGifControls } from '../www/ui/raw-background-gif-controls.js';

// 원문 입력 코드 비트(cpppj InputCode)다.
const CODE = { release: 0x80000000, shift: 0x20000000, control: 0x10000000, command: 0x02000000, left: 0x10000, right: 0x20000 };
// 원본 FNV-1a 64비트 지문이다(독립 관측과 같은 계산).
function fnv64(bytes) { let value = 14695981039346656037n; for (const byte of bytes) value = BigInt.asUintN(64, (value ^ BigInt(byte)) * 1099511628211n); return value.toString(16).padStart(16, '0'); }
const sha256 = (bytes) => crypto.createHash('sha256').update(bytes).digest('hex');
// 탭 구분 fixture 를 머리글 한 줄 뒤부터 열 목록으로 읽는다.
function rows(name) { return fs.readFileSync(new URL(`../../cpppj/tests/fixtures/${name}`, import.meta.url), 'utf8').split(/\r?\n/).filter(Boolean).slice(1).map((line) => line.split('\t')); }

// 원본 설치 폴더의 d/*.gif 전체(34개)를 이름 그대로 읽는다. 공개 웹 자료에는 없는 파일이라 원본 폴더에서만 읽는다.
const ORIGINAL_D = path.join(REPO_ROOT, 'originals', 'd');
function originalGifs() { return new Map(fs.readdirSync(ORIGINAL_D).filter((name) => /\.gif$/i.test(name)).map((name) => [`d/${name}`, new Uint8Array(fs.readFileSync(path.join(ORIGINAL_D, name)))])); }

// 실제 GameData 를 설정/파일 인터페이스로 열고 추가 파일을 느슨한 파일로 등록한다.
function openData(extra = new Map()) {
    const files = readDataMap([ARCHIVE_PATH, SETUP_PATH, 'd/titleMenu.gif']);
    for (const [name, bytes] of extra) files.set(name, bytes);
    return GameData.build({ files, language: 'english' });
}

// 같은 실제 공통 등록표/트리/수명/수집기에 GIF 창 한 소유자를 접합한다. 다른 월드/게임 상태는 만들지 않는다.
function makeScene(data) {
    const registry = new GumpCreateState(); registry.screenWidth = 640; registry.screenHeight = 480; const nodes = new Map([[1, new GumpCreateNode()]]); const released = []; const queued = [];
    let gifs = null; let nextToken = 100; const scene = { registry, nodes, released, queued, destroyOnQueue: 0, changed: 0 };
    // 모든 파생 종류의 같은 기본 노드 조회다(루트 + GIF 노드).
    const lookup = (token) => nodes.get(token) ?? gifs?.node(token) ?? null;
    const create = new GumpCreate(registry, { node: lookup, allocate: () => 1 }); create.initializeRoot();
    const collection = new GumpCollection(registry.collection, create.collectionHooks());
    const life = new GumpLifetimeState(); const lifeHooks = { node: lookup, release: (token) => { released.push(token); if (gifs.node(token)) gifs.release(token); else nodes.delete(token); } };
    const lifetime = new GumpLifetime(registry, life, lifeHooks);
    const treeHooks = { node: lookup, redraw: () => {}, push: (token) => tree.push(token), attach: (...args) => tree.attach(...args), kill: (token) => lifetime.kill(token) };
    const tree = new GumpTree(registry, new GumpTreeState(), treeHooks); lifeHooks.push = treeHooks.push;
    // 원문 scalar deleting destructor 는 GIF 노드에서 이미지 반납→기본 소멸→객체 반납 순서다.
    lifeHooks.deletingDestructor = (token, flags) => { if (gifs?.node(token)) gifs.deleteScalar(token, flags); else lifetime.deleteScalar(token, flags); };
    const eventState = new GumpEventState(); const events = new GumpEvents(registry, life, eventState, collection, { node: lookup }); const inputState = new GumpInputState(); const input = new GumpInput(registry, life, inputState, collection, events, { node: lookup });
    const common = createRawMenuTree({
        create: registry, life, factory: create, collection, tree, lifetime, events, input, stones: {},
        token: () => nextToken++, node: lookup, attach: (token, x, y, parent, mode) => tree.attach(token, x, y, parent, mode), changed: () => { scene.changed += 1; },
    });
    const context = { files: data.files, config: data, dispatch: (event, first, second) => { queued.push([event, first, second]); if (scene.destroyOnQueue) { const token = scene.destroyOnQueue; scene.destroyOnQueue = 0; lifetime.kill(token); lifetime.cleanup(); } } };
    // 같은 실제 기본 Help 입력에 위임하는 기본 처리(검사에서는 입력 DWORD 를 그대로 기록한다).
    const base = []; gifs = new RawBackgroundGifControls(common, context, (...args) => { base.push(args); return 0x12345678; });
    return Object.assign(scene, { gifs, common, tree, lifetime, base, context, count: () => registry.count });
}

test('GIF 창 실제 기본 경로: 팩토리·고정 파생 저장소·동시 창의 독립 이미지·교체/반납', () => {
    const data = openData(originalGifs()); const scene = makeScene(data); const { gifs } = scene; const before = scene.count();
    const first = gifs.create('titleMenu'); const second = gifs.create('defeat'); const a = gifs.node(first); const b = gifs.node(second);
    assert.equal(scene.common.node(first), a); assert.deepEqual([a.vtable, a.command, a.flags, a.event, a.first, a.second], [0x501b60, 3, 9, 0, 0, 0]); assert.ok(a.bitmap && b.bitmap && a.bitmap !== b.bitmap);
    assert.deepEqual([gifs.imageCount, scene.count(), a.baseWidth, a.baseHeight, b.baseWidth, b.baseHeight], [2, before + 2, 639, 480, 640, 480]);
    // 호스트 원시 이미지 주소 별칭은 입력/Help/Chat 문자열과 구별되는 0x72000000 대역에서 단조 증가한다.
    assert.deepEqual([a.bitmap, b.bitmap], [0x72000000, 0x72000001]);
    const old = b.bitmap; gifs.backend.refresh(second, 'victory'); assert.notEqual(b.bitmap, old); assert.equal(gifs.imageCount, 2); assert.throws(() => gifs.bitmap(old), RangeError);
    scene.tree.attach(first, 0, 0, 1, 0); scene.tree.attach(second, 0, 0, 1, 0); scene.lifetime.kill(first); scene.lifetime.cleanup();
    assert.equal(scene.common.node(first), null); assert.equal(scene.common.node(second), b); assert.equal(gifs.imageCount, 1); assert.equal(scene.count(), before + 1);
    // 기본 생성 전에 실패한 미등록 노드는 등록표 정리와 별도로 직접 반납한다.
    const raw = gifs.allocate(); assert.ok(gifs.node(raw)); assert.equal(scene.count(), before + 1);
    gifs.close(); assert.equal(scene.count(), before); assert.equal(gifs.imageCount, 0); assert.equal(gifs.node(raw), null); assert.ok(scene.changed > 0);
});

test('GIF 창 보호 자산 34개: 해독 색 번호가 원본 LZW 독립 관측의 지문·해시·치수와 같다', () => {
    const table = rows('background-gif-assets-x86.tsv'); assert.equal(table.length, 34);
    const data = openData(originalGifs()); const scene = makeScene(data); const { gifs } = scene;
    for (const row of table) {
        const [name, width, height, fnv, pixelSha] = row; data.set('BackgroundGifSpec', name);
        const token = gifs.create('native-observation'); const node = gifs.node(token); const bitmap = gifs.bitmap(node.bitmap);
        assert.deepEqual([node.baseWidth, node.baseHeight, bitmap.width, bitmap.height, bitmap.indices.length], [Number(width), Number(height), Number(width), Number(height), Number(width) * Number(height)], name);
        assert.equal(fnv64(bitmap.indices), fnv, name); assert.equal(sha256(bitmap.indices), pixelSha, name);
        gifs.close(); assert.equal(gifs.imageCount, 0);
    }
});

// 독립 Native 와 같은 입력 헤더만 바꾸는 GIF 변형이다(기대 픽셀은 계산하지 않는다).
function gifVariant(original, name) {
    const descriptor = 13 + (3 << ((original[10] & 7) + 1)); let data = Uint8Array.from(original);
    const word = (offset, value) => { data[offset] = value & 0xff; data[offset + 1] = (value >> 8) & 0xff; };
    if (name === 'localPalette') {
        // 전역 팔레트를 떼어 같은 팔레트를 첫 이미지의 지역 팔레트로 옮긴다.
        const head = Uint8Array.from(data.subarray(0, 13)); head[10] &= 0x7f; const image = Uint8Array.from(original.subarray(descriptor, descriptor + 10)); image[9] = (image[9] & 0x78) | 0x80 | (original[10] & 7);
        return Uint8Array.from([...head, ...image, ...original.subarray(13, descriptor), ...original.subarray(descriptor + 10)]);
    }
    assert.ok(['logicalOffsets', 'smallCanvas', 'interlaced'].includes(name)); const small = name === 'smallCanvas';
    word(6, small ? 1 : 683); word(8, small ? 1 : 521); word(descriptor + 1, small ? 40000 : 17); word(descriptor + 3, small ? 40000 : 19); if (name === 'interlaced') data[descriptor + 9] |= 0x40; return data;
}

test('GIF 해독 변형 4개: 논리 화면/오프셋은 무시하고 첫 descriptor·지역 팔레트·인터레이스 네 패스를 쓴다', () => {
    const source = readDataMap(['d/titleMenu.gif']).get('d/titleMenu.gif'); const table = rows('background-gif-variants-x86.tsv'); assert.equal(table.length, 4);
    for (const row of table) {
        const [name, width, height, fnv, pixelSha] = row; const data = gifVariant(source, name); const image = decodeBackgroundGif(data);
        assert.deepEqual([image.width, image.height, image.indices.length], [Number(width), Number(height), Number(width) * Number(height)], name); assert.equal(fnv64(image.indices), fnv, name); assert.equal(sha256(image.indices), pixelSha, name);
        if (name === 'logicalOffsets') assert.equal(decodeGif(data).width, 683);
    }
});

// 640x480 표면(행 간격 647)과 클립으로 실제 공통 위치/원시 복사를 그린다. 반환은 표면 바이트다.
function paint(scene, token, clip) {
    const pixels = new Uint8Array(647 * 480).fill(0x5a); const state = new GumpDrawState(); setGumpDrawClip(state, clip ?? { left: 0, top: 0, right: 640, bottom: 480 });
    scene.gifs.draw(token, new GumpPixelTarget(pixels, 647, 480, state)); return pixels;
}

test('GIF 창 공통 픽셀: 부착 위치·원시 base 치수 복사·클립·행 끝 패딩', () => {
    const data = openData(); const scene = makeScene(data); const { gifs } = scene; const token = gifs.create('titleMenu'); scene.tree.attach(token, 11, 13, 1, 1);
    const node = gifs.node(token); node.width = 320; node.height = 240; const bitmap = gifs.bitmap(node.bitmap); const pixels = paint(scene, token); const clip = { left: 29, top: 31, right: 603, bottom: 459 }; const clipped = paint(scene, token, clip);
    let changed = 0; let full = 0; let clipDifferences = 0;
    for (let y = 0; y < 480; y += 1) for (let x = 0; x < 647; x += 1) {
        const inside = x >= 11 && x < 640 && y >= 13; const expected = inside ? bitmap.indices[(y - 13) * bitmap.width + x - 11] : 0x5a; full += pixels[y * 647 + x] !== expected;
        const inClip = x >= clip.left && x < clip.right && y >= clip.top && y < clip.bottom; clipDifferences += clipped[y * 647 + x] !== (inClip ? expected : 0x5a); changed += clipped[y * 647 + x] !== 0x5a;
    }
    assert.deepEqual([full, clipDifferences], [0, 0]); assert.ok(changed > 0); assert.equal(pixels[400 * 647 + 500], bitmap.indices[(400 - 13) * bitmap.width + 500 - 11]); gifs.close();
});

test('GIF 창 원시 큐 사건: 수정자/해제/키를 구별하고 사건 세 DWORD 를 정규화 없이 넘기며 콜백 뒤 노드를 다시 읽지 않는다', () => {
    const data = openData(originalGifs()); const scene = makeScene(data); const { gifs, queued } = scene; const before = scene.count(); const token = gifs.create('defeat'); scene.tree.attach(token, 0, 0, 1, 0);
    gifs.backend.setEvent(token, 0xfedcba98, 0x87654321, 0xabcdef01); assert.equal(gifs.node(token).flags & 0x600, 0x600);
    assert.equal(gifs.handle(token, CODE.left, 123, 456), 1); assert.equal(gifs.handle(token, (CODE.right | CODE.shift) >>> 0, 123, 456), 1); assert.equal(gifs.handle(token, (27 | CODE.control) >>> 0, 123, 456), 1); assert.equal(queued.length, 3);
    for (const value of queued) assert.deepEqual(value, [0xfedcba98, 0x87654321, 0xabcdef01]); // 사건을 Command 로 정규화하지 않는다.
    // 해제/명령/일반 키는 기본 입력에 위임하고 큐를 늘리지 않는다.
    assert.equal(gifs.handle(token, (27 | CODE.release) >>> 0, 1, 2), 0x12345678); gifs.handle(token, (27 | CODE.command) >>> 0, 1, 2); gifs.handle(token, 65, 1, 2); assert.equal(queued.length, 3); assert.equal(scene.base.length, 3);
    assert.deepEqual(scene.base[0], [token, (27 | CODE.release) >>> 0, 1, 2]);
    // 큐 콜백이 창을 반납해도 반환은 1 이고 노드를 다시 읽지 않는다.
    scene.destroyOnQueue = token; assert.equal(gifs.handle(token, 27, 5, 7), 1); assert.equal(scene.common.node(token), null); assert.equal(scene.count(), before); assert.equal(gifs.imageCount, 0); assert.equal(queued.length, 4);
});

test('GIF 창 없는 파일/잘못된 파일/페이지 전환: 부분 생성도 등록표와 실제 이미지를 회수하고 이전 그림 반납 뒤의 해독 예외를 없던 파일로 바꾸지 않는다', () => {
    const data = openData(new Map([['d/bad.gif', Uint8Array.of(98, 97, 100)]])); const scene = makeScene(data); const { gifs } = scene; const before = scene.count();
    const token = gifs.create('titleMenu'); gifs.backend.refresh(token, 'nonexistent-background-402');
    assert.deepEqual([gifs.node(token).bitmap, gifs.node(token).baseWidth, gifs.node(token).height, gifs.imageCount], [0, 0, 0, 0]); gifs.close(); assert.equal(scene.count(), before);
    // 해독할 수 없는 파일은 던지지만 생성된 기본 노드는 등록돼 있다가 정리에서 반납된다.
    data.set('BackgroundGifSpec', 'd/setup.cfg'); assert.throws(() => gifs.create('invalid-gif-input')); assert.equal(scene.count(), before + 1); assert.equal(gifs.imageCount, 0); gifs.close(); assert.equal(scene.count(), before);
    // 페이지 전환(전체 정리)은 모든 창과 이미지를 반납한다.
    data.set('BackgroundGifSpec', 'd/titleMenu.gif'); gifs.create('first'); gifs.create('second'); assert.equal(gifs.imageCount, 2); gifs.close(); assert.equal(scene.count(), before); assert.equal(gifs.imageCount, 0);
    // 이전 그림이 있는 노드에서 짧은 파일로 교체하면 이전 그림은 먼저 반납되고 예외 뒤 bitmap 은 0 이다.
    const again = gifs.create('titleMenu'); data.set('BackgroundGifSpec', 'd/bad.gif'); assert.throws(() => gifs.backend.refresh(again, 'truncated'), RangeError);
    assert.equal(gifs.node(again).bitmap, 0); assert.equal(gifs.imageCount, 0); gifs.close(); assert.equal(scene.count(), before);
});

test('GIF 창 조립 경계: 생성 인자/Draw 재진입/원시 할당 밖 BYTE 공급/미연결 큐/내부 불일치', () => {
    const data = openData(); const scene = makeScene(data); const { gifs, common, context } = scene;
    assert.throws(() => new RawBackgroundGifControls(common, { ...context, files: null }, () => 0), TypeError); assert.throws(() => new RawBackgroundGifControls(common, { ...context, config: null }, () => 0), TypeError); assert.throws(() => new RawBackgroundGifControls(common, context, null), TypeError);
    assert.throws(() => gifs.bitmap(0x72000000), /이미지 없음/); const token = gifs.create('titleMenu'); scene.tree.attach(token, 0, 0, 1, 1);
    // 보이는 원시 복사가 실제 할당 밖이면 물리 공급이 필요하고, 공급이 있으면 그 BYTE 를 쓴다.
    gifs.node(token).baseWidth = 700; assert.throws(() => paint(scene, token), /할당 밖/);
    const supplied = []; const withOutside = makeScene(data); withOutside.context.outside = (bitmap, offset) => { supplied.push(offset); return 0x33; };
    const second = withOutside.gifs; const other = second.create('titleMenu'); withOutside.tree.attach(other, 0, 0, 1, 1); second.node(other).baseWidth = 700; const pixels = paint(withOutside, other); assert.ok(supplied.length > 0); assert.ok(pixels.includes(0x33));
    // Draw 는 재진입할 수 없고 정상/예외 반환 뒤 표면을 반납한다.
    const target = new GumpPixelTarget(new Uint8Array(647 * 480), 647, 480, new GumpDrawState()); const hostile = { rawBitmap: () => { assert.throws(() => gifs.draw(token, target), /재진입/); throw new Error('명시 복사 실패'); } };
    gifs.node(token).baseWidth = 639; assert.throws(() => gifs.draw(token, hostile), /명시 복사 실패/); assert.doesNotThrow(() => paint(scene, token));
    // 미연결 원시 입력 큐/없는 이미지 반납은 명시 오류다.
    const noQueue = makeScene(data); noQueue.context.dispatch = null; const solo = noQueue.gifs; const id = solo.create('titleMenu'); solo.backend.setEvent(id, 1, 2, 3);
    assert.throws(() => solo.handle(id, CODE.left, 0, 0), /원시 입력 큐 미연결/); solo.deleteScalar(id, 0); assert.equal(solo.imageCount, 0); assert.throws(() => solo.deleteScalar(id, 0), /반납 불일치/);
});

test('RawMenuTree: 필수 멤버 검증과 장치 없는 독립 표시의 변경 통지 대체', () => {
    const data = openData(); const { common } = makeScene(data); const parts = { ...common };
    for (const name of ['create', 'life', 'factory', 'collection', 'tree', 'lifetime', 'events', 'input', 'stones', 'token', 'node', 'attach', 'changed']) { const broken = { ...parts }; delete broken[name]; assert.throws(() => createRawMenuTree(broken), new RegExp(`필수 멤버 없음: ${name}`)); }
    assert.throws(() => createRawMenuTree(null), /묶음 필요/); assert.throws(() => createRawMenuTree(5), /묶음 필요/); assert.throws(() => createRawMenuTree({ ...parts, create: 3 }), /필수 멤버 없음: create/); assert.throws(() => createRawMenuTree({ ...parts, node: {} }), /필수 멤버 없음: node/); assert.throws(() => createRawMenuTree({ ...parts, invalidate: 3 }), /선택 멤버는 함수여야 함: invalidate/);
    let changed = 0; const plain = createRawMenuTree({ ...parts, changed: () => { changed += 1; } }); plain.requestInvalidate({ left: 0, top: 0, right: 1, bottom: 1 }, 7); plain.requestRedrawBase(5, true); assert.equal(changed, 2);
    const calls = []; const full = createRawMenuTree({ ...parts, changed: () => { changed += 1; }, invalidate: (rect, flags) => calls.push(['invalidate', rect.right, flags]), baseRedraw: (token, erase) => calls.push(['redraw', token, erase]) });
    full.requestInvalidate({ left: 0, top: 0, right: 9, bottom: 1 }, 7); full.requestRedrawBase(5, false); assert.deepEqual(calls, [['invalidate', 9, 7], ['redraw', 5, false]]); assert.equal(changed, 2);
});
