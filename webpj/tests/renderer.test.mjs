// 장면 합성기 renderer.js(Renderer·sortRenderSprites)를 검사한다(cpppj `RendererTests.cpp` 의 Renderer_* 여섯 건 이식 + 웹 계약).
//  - 이식: OrderAndDirtyRegions(원본 x86 관측 `renderer-x86.tsv` 의 Order 261행·Dirty 133행: 표시 비교 반환값과 변경 표 전체), ColorMapsAndShadows(0/255 불투명·소스 변환·그림자 대상 변환·범위 보호), MovementDepthAndUnchangedFrames(깊이 순서·이동 흔적 제거·변화 없는 프레임의 출력 0회),
//    SoftwareCursor(움직임 흔적 없음·사라짐), ExtraCollectionUsesDirtyOrderAndCachedSceneImage(dirty 마다 한 번 수집·원본 깊이 순서·정지 장면 캐시), ActualMenuOverlayCurrentClipAndCursorOrder(선택 표시 뒤·커서 앞·현재 clip).
//    BitmapFont_GdiFallback·Cursor_DefaultResourceMapping 은 GDI/리소스 장치 단위의 몫이라 옮기지 않았고, 글자 출력은 대역 글꼴로 따로 확인한다.
//  - 웹 계약: 생성/표면/그리기 순서 오류, 직접 그리기 경계, world 장치 순서와 UI 전용 4, 카메라/배경 다시 그리기 요청, 정렬 키 거부, 글자 패스와 clip, sceneImage 의 구성.
// 실제 게임·브라우저 실행이 아니다. (2026-11-13 WB04-E391: 5-A Renderer 본체 이식으로 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows } from './raw-support.mjs';
import { ShapeDatabase } from '../www/assets/shapes.js';
import { compareDrawOrder } from '../www/render/draw-sort.js';
import { DirtyRegions } from '../www/render/dirty-regions.js';
import { Renderer, drawIndexedImage, sortRenderSprites } from '../www/render/renderer.js';

const INT_MAX = 2147483647;

// 한 픽셀 VFX 프레임 데이터베이스이다. 불투명 색 0 과 255 도 검사할 수 있다(cpppj PixelShape). (2026-11-13 WB04-E391)
function pixelShape(color) {
    const bytes = new Uint8Array(43);
    const view = new DataView(bytes.buffer);
    view.setUint32(0, 0x30312e31, true);
    view.setUint32(4, 1, true);
    view.setUint32(8, 16, true);
    bytes.set([2, color, 0], 40);
    return new ShapeDatabase(bytes);
}

// SHP 스프라이트 한 개이다(필드 이름은 cpppj RenderSprite 와 같다). (2026-11-13 WB04-E391)
const sprite = (database, x, y, depth, extra = {}) => ({
    database, block: 0, frame: 0, x, y, clip: { left: 0, top: 0, right: 8, bottom: 8 }, order: { x: 2, y: 2, depth }, colors: null, shadow: false, followingFrames: [], procedure: null, sourceSid: 0, ...extra,
});

// 직접 그리기 스프라이트이다: 호출 순서와 clip 을 기록하고 표시된 한 칸에만 라벨을 쓴다. (2026-11-13 WB04-E391)
function procedureSprite(log, label, depth, extra = {}) {
    return {
        database: null, block: 0, frame: 0, x: 0, y: 0, clip: { left: 0, top: 0, right: 20, bottom: 10 }, order: { x: 1, y: 1, depth }, colors: null, shadow: false, followingFrames: [], sourceSid: 0,
        procedure: {
            bounds: { left: 0, top: 0, right: 20, bottom: 10 },
            draw: (target, pitch, height, cut) => {
                log.push(label);
                if (cut.left <= 1 && cut.right > 1 && cut.top <= 1 && cut.bottom > 1) target[21] = label;
            },
        },
        ...extra,
    };
}

// 예외가 나는지 본다. (2026-11-13 WB04-E391)
function throwsOf(action) {
    try { action(); } catch { return true; }
    return false;
}

test('Renderer_ColorMapsAndShadows: 0/255 불투명·소스 변환·그림자의 대상 변환과 범위 보호', () => {
    const image = { width: 3, height: 1, indices: Uint8Array.of(0, 255, 100), opacity: Uint8Array.of(255, 0, 255) };
    const pixels = new Uint8Array(8).fill(7);
    const map = new Uint8Array(256);
    for (let color = 0; color < 256; color += 1) map[color] = 255 - color;
    drawIndexedImage(pixels, 4, 2, image, 1, 0, { left: 0, top: 0, right: 4, bottom: 2 }, map);
    assert.deepEqual(Array.from(pixels), [7, 255, 7, 155, 7, 7, 7, 7]);
    drawIndexedImage(pixels, 4, 2, image, -1, 1, { left: 0, top: 0, right: 2, bottom: 2 }, map, true);
    assert.deepEqual([pixels[4], pixels[5], pixels[6]], [7, 248, 7]);
    assert.ok(throwsOf(() => drawIndexedImage(pixels, 4, 2, image, 0, 0, { left: 0, top: 0, right: 4, bottom: 2 }, null, true)));
});

test('Renderer_MovementDepthAndUnchangedFrames: 깊이 순서·이동 흔적 제거·변화 없는 프레임의 출력 0회', () => {
    const back = pixelShape(0);
    const front = pixelShape(255);
    const renderer = new Renderer(8, 8);
    const pixels = new Uint8Array(64).fill(9);
    const a = sprite(back, 2, 2, 10);
    let b = sprite(front, 2, 2, -10);
    // 잘못된 블록, 좌표 감김(x = INT_MAX)은 거부한다.
    assert.throws(() => renderer.setScene([{ ...a, block: back.blocks.length }]), /Invalid sprite block/);
    assert.throws(() => renderer.setScene([{ ...a, x: INT_MAX }]), /coordinate overflow/);
    renderer.setScene([b, a]);
    assert.equal(renderer.draw(pixels, 8).length, 1);
    // 깊이가 큰 a(0) 를 먼저, 작은 b(255) 를 위에 그린다.
    assert.equal(pixels[18], 255);
    let updates = 0;
    renderer.present(() => { updates += 1; });
    assert.equal(updates, 1);
    // 변화가 없으면 그릴 영역이 없고 출력도 늘지 않는다.
    assert.equal(renderer.draw(pixels, 8).length, 0);
    renderer.present(() => { updates += 1; });
    assert.equal(updates, 1);
    // 그림을 옮기면 이전 자리를 배경으로 복구하고 새 자리에 그린다.
    b = { ...b, x: 5, y: 5 };
    renderer.setScene([b]);
    const changed = renderer.draw(pixels, 8);
    assert.ok(changed.length > 0 && changed[0].right - changed[0].left < 8);
    assert.equal(pixels[18], 0);
    assert.equal(pixels[45], 255);
    renderer.present(() => {});
    assert.equal(renderer.drawCount, 2);
    renderer.setScene([]);
    renderer.draw(pixels, 8);
    renderer.present(() => {});
    assert.equal(pixels[45], 0);
});

test('Renderer_SoftwareCursor: 소프트웨어 커서가 움직일 때 이전 그림이 남지 않고 사라지며 좌표 감김은 거부한다', () => {
    const renderer = new Renderer(6, 2);
    const pixels = new Uint8Array(12);
    const image = { width: 2, height: 1, indices: Uint8Array.of(100, 200), opacity: Uint8Array.of(255, 0) };
    renderer.setSoftwareCursor(image, { x: 1, y: 0 });
    renderer.draw(pixels, 6);
    renderer.present(() => {});
    assert.deepEqual([pixels[1], pixels[2]], [100, 0]);
    renderer.setSoftwareCursor(image, { x: 3, y: 0 });
    renderer.draw(pixels, 6);
    renderer.present(() => {});
    assert.deepEqual([pixels[1], pixels[3]], [0, 100]);
    renderer.setSoftwareCursor(null, { x: 0, y: 0 });
    renderer.draw(pixels, 6);
    renderer.present(() => {});
    assert.equal(pixels[3], 0);
    assert.throws(() => renderer.setSoftwareCursor(image, { x: INT_MAX, y: 0 }), /coordinate overflow/);
    // 같은 그림·같은 위치는 새 변경 영역을 만들지 않지만 force 는 만든다.
    renderer.setSoftwareCursor(image, { x: 1, y: 0 });
    renderer.draw(pixels, 6);
    renderer.present(() => {});
    renderer.setSoftwareCursor(image, { x: 1, y: 0 });
    assert.equal(renderer.draw(pixels, 6).length, 0);
    renderer.present(() => {});
    renderer.setSoftwareCursor(image, { x: 1, y: 0 }, true);
    assert.ok(renderer.draw(pixels, 6).length > 0);
});

test('Renderer_ExtraCollectionUsesDirtyOrderAndCachedSceneImage: 추가 수집은 dirty 마다 한 번 부르고 원본 깊이 순서로 합성하며 정지 장면은 캐시를 쓴다', () => {
    const renderer = new Renderer(20, 10);
    let collections = 0;
    const draws = [];
    const pixels = new Uint8Array(200);
    renderer.setScene([procedureSprite(draws, 3, 3), procedureSprite(draws, 1, 9)]);
    renderer.setExtraCollection((clip, flags) => {
        collections += 1;
        assert.equal(flags, 0);
        return [procedureSprite(draws, 2, 5)];
    });
    assert.ok(renderer.hasExtraCollection());
    renderer.draw(pixels, 20);
    renderer.present(() => {});
    assert.equal(collections, 1);
    assert.deepEqual(draws, [1, 2, 3]);
    assert.equal(pixels[21], 3);
    draws.length = 0;
    // 서로 떨어진 두 변경 영역은 각각 한 번씩 수집하고 각각 같은 순서로 합성한다.
    renderer.invalidate({ left: 0, top: 0, right: 3, bottom: 3 }, 0);
    renderer.invalidate({ left: 12, top: 5, right: 15, bottom: 8 }, 0);
    renderer.draw(pixels, 20);
    renderer.present(() => {});
    assert.equal(collections, 3);
    assert.deepEqual(draws, [1, 2, 3, 1, 2, 3]);
    draws.length = 0;
    // 정지 장면 이미지는 새 수집 없이 마지막 표시 목록을 같은 정렬로 합성한다.
    const image = renderer.sceneImage();
    assert.equal(collections, 3);
    assert.deepEqual(draws, [1, 2, 3]);
    assert.equal(image.indices[21], 3);
    // 수집기를 떼면 정적 월드만 남는다.
    renderer.setExtraCollection(null);
    assert.equal(renderer.hasExtraCollection(), false);
    draws.length = 0;
    renderer.draw(pixels, 20);
    renderer.present(() => {});
    assert.equal(collections, 3);
    assert.deepEqual(draws, [1, 3]);
});

test('Renderer_ActualMenuOverlayCurrentClipAndCursorOrder: 메뉴 출력은 선택 표시 뒤·커서 앞이며 변경 clip 밖의 이전 픽셀을 다시 쓰지 않는다', () => {
    const renderer = new Renderer(8, 8);
    const pixels = new Uint8Array(64);
    const clips = [];
    let color = 7;
    renderer.setOverlay({ width: 8, height: 8, indices: new Uint8Array(64).fill(5), opacity: new Uint8Array(64).fill(255) });
    renderer.setSoftwareCursor({ width: 1, height: 1, indices: Uint8Array.of(9), opacity: Uint8Array.of(255) }, { x: 3, y: 3 });
    renderer.setOverlayProcedure({
        bounds: { left: 2, top: 2, right: 6, bottom: 6 },
        draw: (target, pitch, height, clip) => {
            clips.push({ ...clip });
            for (let y = clip.top; y < clip.bottom; y += 1) target.fill(color, y * pitch + clip.left, y * pitch + clip.right);
        },
    });
    renderer.draw(pixels, 8);
    renderer.present(() => {});
    assert.ok(clips.length === 1 && clips[0].left === 2 && clips[0].right === 6);
    assert.deepEqual([pixels[2 * 8 + 2], pixels[3 * 8 + 3], pixels[1 * 8 + 1]], [7, 9, 5]);
    clips.length = 0;
    color = 8;
    renderer.invalidate({ left: 2, top: 2, right: 3, bottom: 3 });
    renderer.draw(pixels, 8);
    renderer.present(() => {});
    assert.deepEqual(clips, [{ left: 2, top: 2, right: 3, bottom: 3 }]);
    assert.deepEqual([pixels[2 * 8 + 2], pixels[5 * 8 + 5], pixels[3 * 8 + 3]], [8, 7, 9]);
    // 정지 월드의 별도 배경에는 메뉴/커서를 굳히지 않고 선택 표시만 합성한다.
    const image = renderer.sceneImage();
    assert.deepEqual([image.indices[2 * 8 + 2], image.indices[3 * 8 + 3]], [5, 5]);
    renderer.setOverlayProcedure(null);
    renderer.draw(pixels, 8);
    renderer.present(() => {});
    assert.deepEqual([pixels[2 * 8 + 2], pixels[3 * 8 + 3]], [5, 9]);
});

test('Renderer 계약: 생성·표면·그리기 순서·배경/선택 표시 크기·직접 그리기 경계 오류와 정렬 키 거부', () => {
    assert.throws(() => new Renderer(8, 8, 'cd1072'), /10\.78/);
    assert.throws(() => new Renderer(0, 8), RangeError);
    const renderer = new Renderer(8, 8);
    const pixels = new Uint8Array(64);
    // 표면은 폭 이상의 pitch 와 높이 이상의 용량이어야 한다.
    assert.throws(() => renderer.draw(pixels, 7), /Invalid renderer surface/);
    assert.throws(() => renderer.draw(new Uint8Array(63), 8), /Invalid renderer surface/);
    // draw 뒤에는 present 가 먼저다.
    renderer.draw(pixels, 8);
    assert.throws(() => renderer.draw(pixels, 8), /Present must follow Draw/);
    renderer.present(() => {});
    // 배경/선택 표시는 화면 크기의 올바른 그림이어야 한다.
    const bad = { width: 7, height: 8, indices: new Uint8Array(56), opacity: new Uint8Array(56) };
    assert.throws(() => renderer.setBackground(bad), /Invalid renderer background/);
    assert.throws(() => renderer.setOverlay(bad), /Invalid renderer overlay/);
    assert.throws(() => renderer.setBackground({ width: 8, height: 8, indices: new Uint8Array(63), opacity: new Uint8Array(64) }), /Invalid renderer background/);
    assert.throws(() => renderer.setOverlay({ width: 8, height: 8, indices: new Uint8Array(64), opacity: new Uint8Array(63) }), /Invalid renderer overlay/);
    renderer.setBackground(null);
    renderer.setOverlay(null);
    // 메뉴 출력은 draw 함수를 가져야 한다. 비우기는 이미 비었으면 변경을 만들지 않는다.
    assert.throws(() => renderer.setOverlayProcedure({ bounds: { left: 0, top: 0, right: 1, bottom: 1 } }), /UI 표면 출력 경계/);
    renderer.setOverlayProcedure(null);
    renderer.draw(pixels, 8);
    renderer.present(() => {});
    renderer.setOverlayProcedure(null);
    assert.equal(renderer.fullRedrawPending(), false);
    // 직접 그리기는 database 와 함께 쓸 수 없고 draw 가 있어야 하며, SHP 항목은 database 가 있어야 한다.
    const log = [];
    assert.throws(() => renderer.setScene([procedureSprite(log, 1, 0, { database: pixelShape(0) })]), /직접 표시 항목/);
    assert.throws(() => renderer.setScene([procedureSprite(log, 1, 0, { procedure: { bounds: { left: 0, top: 0, right: 1, bottom: 1 } } })]), /직접 표시 항목/);
    assert.throws(() => renderer.setScene([sprite(null, 0, 0, 0)]), /Null sprite database/);
    // 비유한 정렬 키는 정렬 전에 거부하고 이전 장면은 그대로이다.
    const good = procedureSprite(log, 1, 0);
    renderer.setScene([good]);
    assert.throws(() => renderer.setScene([procedureSprite(log, 2, 0, { order: { x: NaN, y: 0, depth: 0 } })]), /Nonfinite draw order/);
    assert.throws(() => renderer.setScene([procedureSprite(log, 2, 0, { order: { x: 0, y: Infinity, depth: 0 } })]), /Nonfinite draw order/);
    renderer.draw(pixels, 8);
    renderer.present(() => {});
    assert.deepEqual(log, [1]);
    // 장면 입력 배열은 바꾸지 않는다(정렬은 사본에 한다).
    const input = [procedureSprite([], 1, 1), procedureSprite([], 2, 9)];
    const copy = input.slice();
    renderer.setScene(input);
    assert.deepEqual(input, copy);
});

test('sortRenderSprites: 깊이 내림차순, y·x 오름차순으로 제자리 정렬하고 같은 키는 원본 비교(항상 1)를 따르며 잘못된 키는 변경 전에 거부한다', () => {
    const labelled = (label, x, y, depth) => ({ label, order: { x, y, depth } });
    const list = [labelled('a', 5, 1, 0), labelled('b', 1, 1, 7), labelled('c', 1, 0, 0), labelled('d', 0, 1, 0), labelled('e', 3, 1, -4), labelled('f', 9, 9, 7)];
    const result = sortRenderSprites(list);
    assert.equal(result, list);
    // 깊이 7 이 먼저(y 1 → 9), 이어서 깊이 0 은 y 0 → 1(x 0 → 5), 마지막 깊이 −4.
    assert.deepEqual(list.map((item) => item.label), ['b', 'f', 'c', 'd', 'a', 'e']);
    // 모든 항목 수(8 이하는 선택 정렬, 초과는 퀵정렬 경로)에서 오름차순 불변식을 만족한다.
    for (const count of [0, 1, 2, 8, 9, 40, 200]) {
        const items = Array.from({ length: count }, (_, index) => labelled(index, (index * 7) % 11, (index * 5) % 13, ((index * 3) % 9) - 4));
        sortRenderSprites(items);
        for (let index = 1; index < items.length; index += 1) {
            const previous = items[index - 1].order;
            const current = items[index].order;
            const ordered = previous.depth > current.depth || (previous.depth === current.depth && (previous.y < current.y || (previous.y === current.y && previous.x <= current.x)));
            assert.ok(ordered, `${count}개 중 ${index}번째`);
        }
        assert.deepEqual(items.map((item) => item.label).sort((a, b) => a - b), Array.from({ length: count }, (_, index) => index));
    }
    // 잘못된 키(float32 가 아닌 x, 범위 밖 깊이)는 아무 항목도 옮기기 전에 거부한다.
    const before = [labelled('x', 1, 1, 0), labelled('y', 0.1, 1, 0)];
    const snapshot = before.slice();
    assert.throws(() => sortRenderSprites(before), RangeError);
    assert.deepEqual(before, snapshot);
    assert.throws(() => sortRenderSprites([labelled('x', 1, 1, 40000)]), RangeError);
});

test('Renderer.draw 의 world 장치: 지형 수집 → 배경 → 월드 제출 → 커서 순서이고 UI 전용(4) 영역은 월드 표시를 제출하지 않으며 장치 누락은 거부한다', () => {
    const renderer = new Renderer(8, 8);
    const pixels = new Uint8Array(64).fill(1);
    const calls = [];
    const world = {
        collect: (clip, flags) => { calls.push(['collect', clip.left, clip.right, flags]); return 3; },
        background: (clip, flags, covered) => { calls.push(['background', flags, covered]); },
        submit: (target, pitch, height, clip) => { calls.push(['submit', pitch, height, clip.right]); target[0] = 77; },
    };
    renderer.setSoftwareCursor({ width: 1, height: 1, indices: Uint8Array.of(9), opacity: Uint8Array.of(255) }, { x: 1, y: 0 });
    renderer.draw(pixels, 8, 0, world);
    renderer.present(() => {});
    assert.deepEqual(calls, [['collect', 0, 8, 0], ['background', 0, 3], ['submit', 8, 8, 8]]);
    // 독립 경로의 배경 지우기는 world 가 있으면 하지 않는다: 제출이 쓰지 않은 칸은 그대로이다.
    assert.deepEqual([pixels[0], pixels[1], pixels[2]], [77, 9, 1]);
    // world 의 지형/배경 장치가 빠지면 그리기 전에 거부한다.
    // 거부된 draw 는 그릴 영역(painted)을 이미 남겼으므로 다음 draw 전에 present 로 정리해야 한다.
    renderer.invalidateAll();
    assert.throws(() => renderer.draw(pixels, 8, 0, { background: () => {} }), /지형\/배경 장치 누락/);
    assert.throws(() => renderer.draw(pixels, 8, 0, { collect: () => 0 }), /Present must follow Draw/);
    renderer.present(() => {});
    renderer.invalidateAll();
    assert.throws(() => renderer.draw(pixels, 8, 0, { collect: () => 0 }), /지형\/배경 장치 누락/);
    renderer.present(() => {});
    // UI 전용(4) 변경 영역: world.submit 이 없으면 월드/미리보기 목록을 비우고 배경도 부르되 선택 표시는 그리지 않는다.
    const log = [];
    const ui = new Renderer(8, 8);
    ui.setScene([procedureSprite(log, 1, 0, { clip: { left: 0, top: 0, right: 8, bottom: 8 }, procedure: { bounds: { left: 0, top: 0, right: 8, bottom: 8 }, draw: () => log.push('world') } })]);
    ui.setOverlay({ width: 8, height: 8, indices: new Uint8Array(64).fill(6), opacity: new Uint8Array(64).fill(255) });
    ui.present(() => {});
    const surface = new Uint8Array(64);
    ui.draw(surface, 8);
    ui.present(() => {});
    log.length = 0;
    surface.fill(0);
    ui.invalidate({ left: 0, top: 0, right: 2, bottom: 2 }, 4);
    const opaque = [];
    ui.draw(surface, 8, 0, { collect: (clip, flags) => { opaque.push(flags); return 0; }, background: () => opaque.push('background') });
    ui.present(() => {});
    assert.deepEqual(opaque, [4, 'background']);
    assert.deepEqual(log, []);
    // 선택 표시도 UI 전용 영역에는 그리지 않는다.
    assert.equal(surface[0], 0);
});

test('Renderer: 카메라/배경 다시 그리기 요청은 present 에서 전체 한 항목으로 바뀌고 부분 draw 의 첫 flags 는 보존한다', () => {
    const renderer = new Renderer(8, 8);
    const pixels = new Uint8Array(64);
    renderer.draw(pixels, 8);
    renderer.present(() => {});
    // 부분 변경 하나를 그린 뒤 카메라 요청이 있으면 present 는 전체 사각형 한 건(첫 flags 유지)을 내보낸다.
    renderer.invalidate({ left: 1, top: 1, right: 3, bottom: 3 }, 4);
    renderer.draw(pixels, 8);
    renderer.cameraRedraw.value = 1;
    const sent = [];
    renderer.present((rect) => sent.push(rect));
    assert.deepEqual(sent, [{ left: 0, top: 0, right: 8, bottom: 8 }]);
    assert.equal(renderer.cameraRedraw.value, 0);
    assert.equal(renderer.presentCount, 2);
    // 배경 시차 요청도 따로 같은 효과이고 요청 상자는 매번 같은 객체이다.
    assert.equal(renderer.backgroundRedraw, renderer.backgroundRedraw);
    renderer.invalidate({ left: 4, top: 4, right: 6, bottom: 6 }, 0);
    renderer.draw(pixels, 8);
    renderer.backgroundRedraw.value = 1;
    const second = [];
    renderer.present((rect) => second.push(rect));
    assert.deepEqual(second, [{ left: 0, top: 0, right: 8, bottom: 8 }]);
    assert.equal(renderer.backgroundRedraw.value, 0);
    // 커서가 추가하는 살아 있는 표: present 로 비워도 같은 배열이다.
    const entries = renderer.cameraDirtyEntries();
    renderer.invalidate({ left: 0, top: 0, right: 1, bottom: 1 }, 0);
    assert.equal(entries.length, 1);
    renderer.draw(pixels, 8);
    renderer.present(() => {});
    assert.equal(entries.length, 0);
    assert.equal(renderer.cameraDirtyEntries(), entries);
});

test('Renderer 글자: 그림자·외곽선·본문 패스를 현재 clip 안에 대역 글꼴로 그리고 글자 clip 과 글꼴 누락을 따른다', () => {
    // 한 글자 7×1 픽셀 대역 글꼴이다: 글자 픽셀은 색 번호 100 이고 전진 폭은 8이다.
    const glyph = { present: true, drawAdvance: 8, image: { width: 1, height: 1, left: 0, top: 0, indices: Uint8Array.of(100), opacity: Uint8Array.of(255) } };
    const font = { glyph: () => glyph };
    const text = (extra = {}) => ({ font, bytes: Uint8Array.of(65), x: 4, y: 4, color: 200, shadowColor: 50, shadowOffset: { x: 1, y: 1 }, shadow: false, outline: false, clip: null, ...extra });
    const draw = (items, clip) => {
        const renderer = new Renderer(16, 16);
        const pixels = new Uint8Array(256);
        renderer.setScene([], items);
        renderer.draw(pixels, 16);
        renderer.present(() => {});
        return pixels;
    };
    // 본문만: (4,4) 한 픽셀.
    const plain = draw([text()]);
    assert.equal(plain[4 * 16 + 4], 200);
    assert.equal(plain.filter((pixel) => pixel !== 0).length, 1);
    // 그림자: (5,5) 에 그림자 색, 본문이 위에 온다.
    const shadow = draw([text({ shadow: true })]);
    assert.deepEqual([shadow[4 * 16 + 4], shadow[5 * 16 + 5]], [200, 50]);
    // 외곽선: 위·오른쪽·아래·왼쪽 네 칸이 그림자 색이다.
    const outline = draw([text({ outline: true })]);
    assert.deepEqual([outline[3 * 16 + 4], outline[4 * 16 + 5], outline[5 * 16 + 4], outline[4 * 16 + 3], outline[4 * 16 + 4]], [50, 50, 50, 50, 200]);
    // 글자 자체 clip 이 본문 칸을 벗어나면 그리지 않는다.
    const clipped = draw([text({ clip: { left: 0, top: 0, right: 4, bottom: 16 } })]);
    assert.equal(clipped.filter((pixel) => pixel !== 0).length, 0);
    // 글꼴이 없으면 거부한다.
    assert.throws(() => draw([text({ font: null })]), /Null bitmap font/);
    // 글자가 있는 장면에서 글자를 바꾸면 전체를 다시 그린다(이전 글자 흔적 없음).
    const renderer = new Renderer(16, 16);
    const pixels = new Uint8Array(256);
    renderer.setScene([], [text()]);
    renderer.draw(pixels, 16);
    renderer.present(() => {});
    renderer.setScene([], [text({ x: 8, y: 8 })]);
    assert.equal(renderer.fullRedrawPending(), true);
    renderer.draw(pixels, 16);
    renderer.present(() => {});
    assert.deepEqual([pixels[4 * 16 + 4], pixels[8 * 16 + 8]], [0, 200]);
});

test('Renderer 합성: SHP 스프라이트의 후속 그림·색 변환·그림자와 clip 은 한 정렬 항목 안에서 같은 위치로 그린다', () => {
    const back = pixelShape(10);
    const front = pixelShape(20);
    // 한 블록의 프레임 0 만 있는 데이터베이스라 followingFrames 는 같은 프레임을 다시 그려 순서를 확인한다.
    const renderer = new Renderer(8, 8);
    const pixels = new Uint8Array(64).fill(9);
    const map = new Uint8Array(256);
    for (let color = 0; color < 256; color += 1) map[color] = (color + 100) & 255;
    // 색 변환: 그림 색 10 → 110.
    renderer.setScene([sprite(back, 2, 2, 0, { colors: map })]);
    renderer.draw(pixels, 8, 9);
    renderer.present(() => {});
    assert.equal(pixels[2 * 8 + 2], 110);
    // 그림자: 대상 화면 색(9 → 109)을 변환하며 그림 색은 쓰지 않는다.
    renderer.setScene([sprite(front, 4, 4, 0, { colors: map, shadow: true })]);
    renderer.draw(pixels, 8, 9);
    renderer.present(() => {});
    assert.equal(pixels[4 * 8 + 4], 109);
    assert.equal(pixels[2 * 8 + 2], 9);
    // 후속 그림은 본 그림 뒤에 같은 위치로 덮는다(같은 프레임 두 번 = 같은 결과, 색 변환 한 번씩).
    renderer.setScene([sprite(back, 6, 6, 0, { followingFrames: [0] })]);
    renderer.draw(pixels, 8, 9);
    renderer.present(() => {});
    assert.equal(pixels[6 * 8 + 6], 10);
    // 스프라이트 clip 이 그림 위치를 포함하지 않으면 그리지 않고, 그림자 없는 색 변환표 누락은 거부한다.
    renderer.setScene([sprite(back, 1, 1, 0, { clip: { left: 4, top: 4, right: 8, bottom: 8 } })]);
    renderer.draw(pixels, 8, 9);
    renderer.present(() => {});
    assert.equal(pixels[1 * 8 + 1], 9);
    renderer.setScene([sprite(back, 3, 3, 0, { shadow: true })]);
    assert.throws(() => renderer.draw(pixels, 8), TypeError);
});

// ---- 변형 확인에서 드러난 공백을 독립 입력으로 보강한 시험들 (2026-11-13 WB04-E391) ----

// 번호가 1부터 늘어나는 비균일 그림이다(원점 배치 오차를 드러낸다). opaque 가 주어지면 그 인덱스 판정으로 불투명을 정한다. (2026-11-13 WB04-E391)
const gradient = (opaque = () => true) => ({ width: 8, height: 8, indices: Uint8Array.from({ length: 64 }, (_, index) => index + 1), opacity: Uint8Array.from({ length: 64 }, (_, index) => (opaque(index) ? 255 : 0)) });

// 특수 레코드(좌표 대신 메타데이터)만 가진 한 프레임 데이터베이스이다: 그림 범위가 없다. (2026-11-13 WB04-E391)
function specialShape() {
    const bytes = new Uint8Array(40);
    const view = new DataView(bytes.buffer);
    view.setUint32(0, 0x30312e31, true);
    view.setUint32(4, 1, true);
    view.setUint32(8, 16, true);
    view.setInt32(24, 0x7fff0000, true);
    return new ShapeDatabase(bytes);
}

test('Renderer 배경·선택 표시: 원점에 놓이고 변경 영역만 복원하며 불투명 영역(4)은 배경색을 다시 칠하지 않는다', () => {
    const renderer = new Renderer(8, 8);
    const pixels = new Uint8Array(64).fill(77);
    renderer.setBackground(gradient());
    renderer.draw(pixels, 8, 99);
    renderer.present(() => {});
    // 첫 프레임은 전체 갱신이고 배경 그림이 원점에 그대로 놓인다.
    assert.deepEqual(Array.from(pixels), Array.from({ length: 64 }, (_, index) => index + 1));
    // 부분 변경: 변경 영역 안만 다시 합성하고(배경 그림이 덮음) 밖은 이전 값 그대로이다.
    pixels.fill(77);
    renderer.invalidate({ left: 2, top: 2, right: 5, bottom: 4 });
    renderer.draw(pixels, 8, 99);
    renderer.present(() => {});
    for (let y = 0; y < 8; y += 1) {
        for (let x = 0; x < 8; x += 1) assert.equal(pixels[y * 8 + x], x >= 2 && x < 5 && y >= 2 && y < 4 ? y * 8 + x + 1 : 77, `${x},${y}`);
    }
    // 배경이 없으면 배경색으로 지우되 불투명(4) 영역은 지우지 않는다.
    const plain = new Renderer(8, 8);
    plain.draw(pixels, 8, 5);
    plain.present(() => {});
    pixels.fill(77);
    plain.invalidate({ left: 1, top: 1, right: 3, bottom: 3 }, 0);
    plain.draw(pixels, 8, 5);
    plain.present(() => {});
    assert.deepEqual([pixels[1 * 8 + 1], pixels[2 * 8 + 2], pixels[3 * 8 + 3], pixels[1 * 8 + 3], pixels[3 * 8 + 1]], [5, 5, 77, 77, 77]);
    pixels.fill(77);
    plain.invalidate({ left: 1, top: 1, right: 3, bottom: 3 }, 4);
    plain.draw(pixels, 8, 5);
    plain.present(() => {});
    assert.deepEqual([pixels[1 * 8 + 1], pixels[2 * 8 + 2]], [77, 77]);
    // 선택 표시는 스프라이트 위 원점에 놓이고 투명 칸은 아래를 보존한다.
    const over = new Renderer(8, 8);
    over.setOverlay(gradient((index) => index % 2 === 0));
    over.draw(pixels, 8, 3);
    over.present(() => {});
    for (let index = 0; index < 64; index += 1) assert.equal(pixels[index], index % 2 === 0 ? index + 1 : 3, `선택 표시 ${index}`);
});

test('Renderer.sceneImage: 배경·표시 목록·선택 표시를 원점 기준으로 합성하고 불투명도는 전부 255 이며 표시 목록은 정렬된 장면을 쓴다', () => {
    const log = [];
    const renderer = new Renderer(8, 8);
    renderer.setBackground(gradient());
    renderer.setOverlay(gradient((index) => index === 63));
    const image = renderer.sceneImage();
    assert.deepEqual([image.width, image.height], [8, 8]);
    assert.ok(image.opacity.every((value) => value === 255));
    // 배경 번호가 원점에 놓이고 선택 표시의 불투명 한 칸(63)만 덮는다.
    for (let index = 0; index < 63; index += 1) assert.equal(image.indices[index], index + 1);
    assert.equal(image.indices[63], 64);
    // 제출 순서와 깊이 순서가 반대인 두 항목: 정렬된 장면(깊이 큰 것 먼저)이므로 마지막은 깊이가 작은 쪽이다.
    const scene = new Renderer(20, 10);
    const first = procedureSprite(log, 1, 9);
    const second = procedureSprite(log, 3, 3);
    scene.setScene([second, first]);
    assert.equal(scene.sceneImage().indices[21], 3);
    assert.deepEqual(log, [1, 3]);
    // 같은 키의 항목이 아홉 개 이상이면 다시 정렬할 때 순서가 섞일 수 있으므로, 장면 이미지는 이미 정렬된 목록을 다시 정렬하지 않고 그대로 그린다.
    const same = new Renderer(20, 10);
    const sameLog = [];
    same.setScene(Array.from({ length: 12 }, (_, index) => procedureSprite(sameLog, index + 1, 0)));
    same.draw(new Uint8Array(200), 20);
    same.present(() => {});
    const drawOrder = sameLog.slice();
    sameLog.length = 0;
    same.sceneImage();
    assert.deepEqual(sameLog, drawOrder);
});

test('Renderer 추가 수집 캐시: 불투명(4) 영역 수집은 캐시를 갱신하지 않고 정지 장면은 제출 순서의 원본에 캐시를 합쳐 정렬한다', () => {
    const renderer = new Renderer(20, 10);
    const log = [];
    const pixels = new Uint8Array(200);
    renderer.setScene([procedureSprite(log, 3, 3), procedureSprite(log, 1, 9)]);
    renderer.setExtraCollection((clip, flags) => [procedureSprite(log, flags & 4 ? 4 : 2, flags & 4 ? 0 : 5)]);
    renderer.draw(pixels, 20);
    renderer.present(() => {});
    // 첫 수집(flags 0)의 라벨 2 가 캐시에 남는다. UI 전용(4) 영역의 수집(라벨 4)은 캐시를 갱신하지 않는다.
    renderer.invalidate({ left: 0, top: 0, right: 3, bottom: 3 }, 4);
    renderer.draw(pixels, 20);
    renderer.present(() => {});
    log.length = 0;
    const image = renderer.sceneImage();
    assert.deepEqual(log, [1, 2, 3]);
    assert.equal(image.indices[21], 3);
    // 수집기를 바꾸면 캐시도 비워진다: 캐시가 없으면 정렬된 정적 장면만 쓴다.
    renderer.setExtraCollection(() => []);
    log.length = 0;
    renderer.sceneImage();
    assert.deepEqual(log, [1, 3]);
    // 수집기 인자는 함수나 비움(null/undefined)만 받는다.
    assert.throws(() => renderer.setExtraCollection('collect'), /추가 수집기는 함수/);
    assert.throws(() => renderer.setExtraCollection(5), TypeError);
    renderer.setExtraCollection(undefined);
    assert.equal(renderer.hasExtraCollection(), false);
});

test('Renderer 그림 범위: 오른쪽/아래 끝은 한 칸 더해 포함하지 않는 경계로 만들고 특수 프레임은 변경 영역을 만들지 않는다', () => {
    const back = pixelShape(10);
    const renderer = new Renderer(8, 8);
    const pixels = new Uint8Array(64);
    renderer.draw(pixels, 8);
    renderer.present(() => {});
    // 한 픽셀 그림은 [2,3)×[2,3) 한 칸의 변경 영역이다.
    renderer.setScene([sprite(back, 2, 2, 0)]);
    assert.deepEqual(renderer.draw(pixels, 8).map((rect) => ({ ...rect })), [{ left: 2, top: 2, right: 3, bottom: 3 }]);
    renderer.present(() => {});
    // 특수 레코드만 가진 스프라이트는 그림 범위가 없어 변경 영역도 없고 출력도 없다(먼저 이전 장면을 비워 이전 범위를 지운다).
    renderer.setScene([]);
    renderer.draw(pixels, 8);
    renderer.present(() => {});
    let updates = 0;
    renderer.setScene([sprite(specialShape(), 2, 2, 0)]);
    assert.equal(renderer.draw(pixels, 8).length, 0);
    renderer.present(() => { updates += 1; });
    assert.equal(updates, 0);
});

test('Renderer 좌표 경계: 32비트 끝값은 허용하고 한 칸 넘으면 거부한다', () => {
    const back = pixelShape(10);
    const renderer = new Renderer(8, 8);
    const image = { width: 2, height: 1, indices: Uint8Array.of(1, 2), opacity: Uint8Array.of(255, 255) };
    // 커서: 끝 좌표(x + 폭)가 정확히 INT_MAX 이면 허용, 하나 넘으면 거부.
    renderer.setSoftwareCursor(image, { x: INT_MAX - 2, y: 0 });
    assert.throws(() => renderer.setSoftwareCursor(image, { x: INT_MAX - 1, y: 0 }), /coordinate overflow/);
    renderer.setSoftwareCursor(null, { x: 0, y: 0 });
    // 스프라이트: 오른쪽 끝(+1)이 정확히 INT_MAX 이면 허용, 하나 넘으면 거부. 왼쪽은 INT_MIN 까지 허용한다.
    renderer.setScene([sprite(back, INT_MAX - 1, 0, 0)]);
    assert.throws(() => renderer.setScene([sprite(back, INT_MAX, 0, 0)]), /coordinate overflow/);
    renderer.setScene([sprite(back, -2147483648, 0, 0)]);
    assert.throws(() => renderer.setScene([sprite(back, -2147483649, 0, 0)]), /coordinate overflow/);
});

test('Renderer 변경 영역: 면적 없는 항목은 그리지 않지만 출력은 하고, 화면 밖으로 걸친 항목은 화면 경계로 잘라 둘 다에 쓴다', () => {
    const renderer = new Renderer(8, 8);
    const pixels = new Uint8Array(64);
    renderer.draw(pixels, 8);
    renderer.present(() => {});
    renderer.invalidate({ left: 2, top: 0, right: 2, bottom: 8 });
    assert.equal(renderer.draw(pixels, 8).length, 0);
    const sent = [];
    renderer.present((rect) => sent.push(rect));
    assert.deepEqual(sent, [{ left: 2, top: 0, right: 2, bottom: 8 }]);
    renderer.invalidate({ left: 0, top: 3, right: 8, bottom: 3 });
    assert.equal(renderer.draw(pixels, 8).length, 0);
    renderer.present(() => {});
    // 화면 밖에 걸친 항목: 잘라서 그리고 잘린 사각형을 내보낸다.
    renderer.invalidate({ left: -5, top: -5, right: 3, bottom: 3 });
    assert.deepEqual(renderer.draw(pixels, 8).map((rect) => ({ ...rect })), [{ left: 0, top: 0, right: 3, bottom: 3 }]);
    const cut = [];
    renderer.present((rect) => cut.push(rect));
    assert.deepEqual(cut, [{ left: 0, top: 0, right: 3, bottom: 3 }]);
    // 요청이 없으면 부분 변경은 그 사각형 그대로 나간다(카메라/배경 요청 상자는 처음부터 0).
    assert.deepEqual([renderer.cameraRedraw.value, renderer.backgroundRedraw.value], [0, 0]);
    renderer.invalidate({ left: 4, top: 4, right: 6, bottom: 6 });
    renderer.draw(pixels, 8);
    const only = [];
    renderer.present((rect) => only.push(rect));
    assert.deepEqual(only, [{ left: 4, top: 4, right: 6, bottom: 6 }]);
});

test('Renderer 배경·선택 표시 검증: 폭·높이·색 번호 길이·불투명 길이를 각각 따로 거부한다', () => {
    const renderer = new Renderer(8, 8);
    const make = (override) => ({ width: 8, height: 8, indices: new Uint8Array(64), opacity: new Uint8Array(64), ...override });
    const cases = [
        make({ width: 9, indices: new Uint8Array(72), opacity: new Uint8Array(72) }),
        make({ height: 9, indices: new Uint8Array(72), opacity: new Uint8Array(72) }),
        make({ indices: new Uint8Array(63) }),
        make({ opacity: new Uint8Array(65) }),
        make({ width: 8, height: 8, indices: new Uint8Array(64), opacity: new Uint8Array(0) }),
    ];
    for (const bad of cases) {
        assert.throws(() => renderer.setBackground(bad), /Invalid renderer background/);
        assert.throws(() => renderer.setOverlay(bad), /Invalid renderer overlay/);
    }
    renderer.setBackground(make({}));
    renderer.setOverlay(make({}));
});

test('Renderer 글자와 장면 갱신: 글자 이동·삭제는 전체 갱신이고 필드 기본값(색 255·그림자 색 0·오프셋 0)을 쓴다', () => {
    const glyph = { present: true, drawAdvance: 8, image: { width: 1, height: 1, left: 0, top: 0, indices: Uint8Array.of(100), opacity: Uint8Array.of(255) } };
    const font = { glyph: () => glyph };
    const renderer = new Renderer(16, 16);
    const pixels = new Uint8Array(256);
    renderer.draw(pixels, 16);
    renderer.present(() => {});
    // 글자도 이전 글자도 없으면 전체 갱신이 아니다.
    renderer.setScene([], []);
    assert.equal(renderer.fullRedrawPending(), false);
    // 새 글자가 생기면, 있던 글자가 사라지면 전체 갱신이다.
    renderer.setScene([], [{ font, bytes: Uint8Array.of(65), x: 4, y: 4 }]);
    assert.equal(renderer.fullRedrawPending(), true);
    pixels.fill(9);
    renderer.draw(pixels, 16, 9);
    renderer.present(() => {});
    // 기본값: 본문 색 255, 그림자 켜면 그림자 색 0 을 같은 위치(오프셋 0)에 먼저 쓰고 본문이 덮는다.
    assert.equal(pixels[4 * 16 + 4], 255);
    renderer.setScene([], []);
    assert.equal(renderer.fullRedrawPending(), true);
    renderer.draw(pixels, 16, 9);
    renderer.present(() => {});
    assert.equal(pixels[4 * 16 + 4], 9);
    renderer.setScene([], [{ font, bytes: Uint8Array.of(65), x: 4, y: 4, shadow: true }]);
    pixels.fill(9);
    renderer.draw(pixels, 16, 9);
    renderer.present(() => {});
    assert.equal(pixels[4 * 16 + 4], 255);
    assert.equal(pixels.filter((pixel) => pixel !== 9).length, 1);
    // 외곽선은 네 방향 칸에 그림자 색(기본 0)을 쓴다.
    renderer.setScene([], [{ font, bytes: Uint8Array.of(65), x: 4, y: 4, outline: true }]);
    pixels.fill(9);
    renderer.draw(pixels, 16, 9);
    renderer.present(() => {});
    assert.deepEqual([pixels[3 * 16 + 4], pixels[4 * 16 + 5], pixels[5 * 16 + 4], pixels[4 * 16 + 3], pixels[4 * 16 + 4]], [0, 0, 0, 0, 255]);
});

test('Renderer.draw 의 선택 표시와 world: 월드 장치가 있어도 UI 전용(4) 영역이 아니면 선택 표시를 그리고 UI 전용 영역이면 건너뛴다', () => {
    const make = () => {
        const renderer = new Renderer(8, 8);
        renderer.setOverlay(gradient((index) => index % 2 === 0));
        const pixels = new Uint8Array(64).fill(200);
        renderer.draw(pixels, 8);
        renderer.present(() => {});
        pixels.fill(200);
        return { renderer, pixels };
    };
    const world = { collect: () => 0, background: () => {} };
    const covered = (pixels) => pixels.filter((pixel, index) => index % 2 === 0 && pixel === index + 1).length;
    // 월드 없음: 불투명 영역(4)이어도 선택 표시를 그린다.
    const plain = make();
    plain.renderer.invalidate({ left: 0, top: 0, right: 8, bottom: 8 }, 4);
    plain.renderer.draw(plain.pixels, 8);
    plain.renderer.present(() => {});
    assert.equal(covered(plain.pixels), 32);
    // 월드 있음(submit 없음): 일반 영역은 그리고 UI 전용 영역은 건너뛴다.
    const normal = make();
    normal.renderer.invalidate({ left: 0, top: 0, right: 8, bottom: 8 }, 0);
    normal.renderer.draw(normal.pixels, 8, 0, world);
    normal.renderer.present(() => {});
    assert.equal(covered(normal.pixels), 32);
    const ui = make();
    ui.renderer.invalidate({ left: 0, top: 0, right: 8, bottom: 8 }, 4);
    ui.renderer.draw(ui.pixels, 8, 0, world);
    ui.renderer.present(() => {});
    assert.equal(covered(ui.pixels), 0);
});

// 원본 x86 관측 행이다(Order 표시 비교 261행·Dirty 변경 표 133행). 파일이 없으면 해당 검사를 건너뛴다. (2026-11-13 WB04-E391)
const fixtureRows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'renderer-x86.tsv'));

// 정수 다섯 개(left, top, right, bottom, flags)씩 묶인 항목 문자열을 읽는다. '-' 는 빈 표이다. (2026-11-13 WB04-E391)
function parseEntries(text) {
    if (text === '-') return [];
    const numbers = text.replace(/[,;]/gu, ' ').split(/\s+/u).filter((part) => part.length > 0).map(Number);
    const entries = [];
    // 다섯 값씩 한 항목이다.
    for (let index = 0; index + 4 < numbers.length; index += 5) entries.push(numbers.slice(index, index + 5));
    return entries;
}

test('Renderer_OrderAndDirtyRegions: 원본 x86 관측 Order 261행의 표시 비교 반환값과 Dirty 133행의 변경 표(무시된 항목의 위치·플래그 포함)가 모두 일치한다', { skip: fixtureRows === null }, () => {
    let orders = 0;
    let sequences = 0;
    // 각 고정 기대값을 이식 구현에 입력한다.
    for (const row of fixtureRows) {
        if (row[0] === 'Order') {
            const [ax, ay, ad, bx, by, bd, expected] = row.slice(1).map(Number);
            const a = { x: Math.fround(ax), y: Math.fround(ay), depth: ad };
            const b = { x: Math.fround(bx), y: Math.fround(by), depth: bd };
            assert.equal(compareDrawOrder(a, b), expected, row.join(' '));
            orders += 1;
        } else {
            assert.equal(row[0], 'Dirty');
            const [width, height, operations, full, expectedText] = [Number(row[1]), Number(row[2]), row[3], Number(row[4]), row[5]];
            const regions = new DirtyRegions(width, height);
            // 원본 기계어에 입력한 같은 시퀀스를 등록한다.
            for (const op of parseEntries(operations)) regions.add({ left: op[0], top: op[1], right: op[2], bottom: op[3] }, op[4]);
            const expected = parseEntries(expectedText);
            assert.equal(regions.fullRedraw, full !== 0, row.join(' '));
            assert.equal(regions.entries.length, expected.length, row.join(' '));
            // 무시한 항목의 위치·플래그도 포함해 대조한다.
            expected.forEach((entry, index) => {
                const actual = regions.entries[index];
                assert.deepEqual([actual.rect.left, actual.rect.top, actual.rect.right, actual.rect.bottom, actual.flags], entry, row.join(' '));
            });
            sequences += 1;
        }
    }
    assert.equal(orders, 261);
    assert.equal(sequences, 133);
});

test('Renderer.sceneImage 선택 표시 위치: 배경과 번호가 다른 선택 표시는 불투명한 칸만 원점 기준 같은 칸에 덮는다', () => {
    const renderer = new Renderer(8, 8);
    renderer.setBackground(gradient());
    // 배경(index+1)과 구별되는 번호(index+100)를 가진 선택 표시: 왼쪽 위 모서리·내부·오른쪽 아래 모서리만 불투명하다.
    const opaque = new Set([0, 9, 63]);
    renderer.setOverlay({ ...gradient((index) => opaque.has(index)), indices: Uint8Array.from({ length: 64 }, (_, index) => index + 100) });
    const image = renderer.sceneImage();
    for (let index = 0; index < 64; index += 1) assert.equal(image.indices[index], opaque.has(index) ? index + 100 : index + 1, `칸 ${index}`);
});

test('Renderer 초기 상태와 버려진 항목: 요청 상자는 0 에서 시작하고 병합으로 버려진 항목은 그리지도 내보내지도 않는다', () => {
    const renderer = new Renderer(8, 8);
    // 만들자마자 카메라/배경 다시 그리기 요청은 없다.
    assert.equal(renderer.cameraRedraw.value, 0);
    assert.equal(renderer.backgroundRedraw.value, 0);
    const pixels = new Uint8Array(64);
    renderer.draw(pixels, 8);
    renderer.present(() => {});
    // 작은 항목 뒤에 그것을 완전히 덮는 불투명(4) 항목이 오면 앞 항목은 버려진다(원본 관측 Dirty: 1,1,3,3,0;0,0,5,5,4 → 1,1,3,3,1;0,0,5,5,4).
    renderer.invalidate({ left: 1, top: 1, right: 3, bottom: 3 }, 0);
    renderer.invalidate({ left: 0, top: 0, right: 5, bottom: 5 }, 4);
    assert.deepEqual(renderer.cameraDirtyEntries().map((entry) => entry.flags), [1, 4]);
    const painted = renderer.draw(pixels, 8);
    assert.deepEqual(painted, [{ left: 0, top: 0, right: 5, bottom: 5 }]);
    const sent = [];
    renderer.present((rect) => sent.push(rect));
    assert.deepEqual(sent, [{ left: 0, top: 0, right: 5, bottom: 5 }]);
    assert.equal(renderer.presentCount, 2);
});

// ---- cpppj RendererPresentationTests 이식 (2026-11-13 WB04-E392: Renderer 시험 보강) ----

// 원본 출력 요청 관측 288행이다(보호 PE 의 카메라/배경 요청·빈 표·첫 flags 입력과 최종 9 DWORD·실제 호출). 파일이 없으면 해당 검사를 건너뛴다. (2026-11-13 WB04-E392)
const presentRows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'renderer-present-dirty-x86.tsv'));

// 호출 문자열(`;` 구분)에서 화면 출력(U:…) 호출만 남긴다(cpppj Updates). (2026-11-13 WB04-E392)
function updatesOf(text) {
    return text.split(';').filter((event) => event.startsWith('U:')).join(';');
}

// 화면 사각형의 출력 호출 문자열이다(cpppj Update). (2026-11-13 WB04-E392)
const updateText = (rect) => `U:${rect.left}:${rect.top}:${rect.right}:${rect.bottom}`;

test('RendererPresentation_NativeDirtyRequestsAndFirstFlags: 원본 PE 의 요청·첫 flags·빈 표·세 화면비 288 입력과 그리기·출력 소비가 일치한다', { skip: presentRows === null }, () => {
    let rows = 0;
    // 기대값은 이식 구현으로 만들지 않고 보호 PE 의 고정 출력만 읽는다.
    for (const row of presentRows) {
        assert.equal(row.length, 18, row.join(' '));
        const words = row.slice(0, 17).map(Number);
        const events = row[17];
        const [width, height] = words;
        const renderer = new Renderer(width, height);
        const pixels = new Uint8Array(width * height);
        renderer.draw(pixels, width);
        renderer.present(() => {});
        // 개수 0 도 첫 flags 물리 바이트를 지우지 않는 원문 입력이다: 표를 한 항목으로 바꾸고 비운 뒤 개수만큼 다시 넣는다.
        const entries = renderer.cameraDirtyEntries();
        entries.length = 0;
        entries.push({ rect: { left: 2, top: 3, right: 7, bottom: 8 }, flags: words[6] });
        renderer.clearDirty();
        if (words[5]) entries.push({ rect: { left: 2, top: 3, right: 7, bottom: 8 }, flags: words[6] });
        if (words[5] === 2) entries.push({ rect: { left: 20, top: 30, right: 40, bottom: 50 }, flags: words[7] });
        renderer.cameraRedraw.value = words[2];
        renderer.backgroundRedraw.value = words[3];
        const drawn = renderer.draw(pixels, width);
        const expectedDraws = (words[5] > 0 && !(words[6] & 1) ? 1 : 0) + (words[5] > 1 && !(words[7] & 1) ? 1 : 0);
        // 출력 전체화가 월드의 재수집/재합성을 늘리지 않아야 한다.
        assert.equal(drawn.length, expectedDraws, row.join(' '));
        assert.equal(renderer.cameraRedraw.value, words[2], row.join(' '));
        assert.equal(renderer.backgroundRedraw.value, words[3], row.join(' '));
        const actual = [];
        // 원문은 두 요청을 지운 뒤 같은 표를 내보낸다.
        renderer.present((rect) => {
            assert.equal(renderer.cameraRedraw.value, 0, row.join(' '));
            assert.equal(renderer.backgroundRedraw.value, 0, row.join(' '));
            assert.equal(renderer.cameraDirtyEntries().length, (words[2] || words[3]) ? 1 : words[5], row.join(' '));
            actual.push(updateText(rect));
        });
        assert.equal(actual.join(';'), updatesOf(events), row.join(' '));
        assert.equal(renderer.cameraRedraw.value, words[8], row.join(' '));
        assert.equal(renderer.backgroundRedraw.value, words[9], row.join(' '));
        assert.equal(renderer.cameraDirtyEntries().length, 0, row.join(' '));
        assert.equal(renderer.draw(pixels, width).length, 0, row.join(' '));
        renderer.present(() => {});
        rows += 1;
    }
    assert.equal(rows, 288);
});

test('RendererPresentation_ScrollKeepsInteriorAndPresentsFull: 이동 뒤 노출 띠만 월드로 다시 합성하고 안쪽 픽셀은 보존하며 전체 화면을 한 번 출력한다', () => {
    const renderer = new Renderer(20, 10);
    const pixels = new Uint8Array(200).fill(7);
    const collected = [];
    renderer.draw(pixels, 20);
    renderer.present(() => {});
    pixels.fill(7);
    const world = {
        collect: (rect) => { collected.push({ ...rect }); return 0; },
        background: () => {},
        // 실제 노출 띠 바깥의 스크롤 결과는 그대로 둔다.
        submit: (surface, pitch, height, rect) => {
            for (let y = rect.top; y < rect.bottom; y += 1) surface.fill(9, y * pitch + rect.left, y * pitch + rect.right);
        },
    };
    renderer.invalidate({ left: 18, top: 0, right: 20, bottom: 10 });
    renderer.cameraRedraw.value = 1;
    renderer.draw(pixels, 20, 0, world);
    assert.deepEqual(collected, [{ left: 18, top: 0, right: 20, bottom: 10 }]);
    assert.equal(pixels[10], 7);
    assert.equal(pixels[19], 9);
    const updated = [];
    renderer.present((rect) => updated.push(updateText(rect)));
    assert.deepEqual(updated, ['U:0:0:20:10']);
    assert.equal(renderer.draw(pixels, 20, 0, world).length, 0);
    renderer.present(() => {});
});

test('RendererPresentation_ClipsActualTableBeforePresent: 그리기에서 화면 경계로 제한된 항목은 빈 영역이어도 출력 표에 남는다', () => {
    const renderer = new Renderer(20, 10);
    const pixels = new Uint8Array(200);
    renderer.draw(pixels, 20);
    renderer.present(() => {});
    const entries = renderer.cameraDirtyEntries();
    entries.length = 0;
    entries.push({ rect: { left: -2, top: -3, right: 7, bottom: 8 }, flags: 0 }, { rect: { left: 21, top: 11, right: 30, bottom: 20 }, flags: 0 });
    const drawn = renderer.draw(pixels, 20);
    assert.equal(drawn.length, 1);
    assert.equal(updateText(drawn[0]), 'U:0:0:7:8');
    const updates = [];
    renderer.present((rect) => updates.push(updateText(rect)));
    assert.equal(updates.length, 2);
    assert.equal(updates[1], 'U:20:10:20:10');
});

test('Renderer.draw 의 world(submit 없음/있음): submit 이 없으면 비불투명 영역은 월드와 추가 수집을 깊이 순으로 그리고 불투명(4) 영역은 수집만 하며, submit 이 있으면 추가 수집기를 부르지 않는다', () => {
    const log = [];
    const renderer = new Renderer(20, 10);
    let collections = 0;
    renderer.setScene([procedureSprite(log, 1, 5)]);
    renderer.setExtraCollection(() => { collections += 1; return [procedureSprite(log, 2, 3)]; });
    const pixels = new Uint8Array(200);
    const bare = { collect: () => 0, background: () => {} };
    // submit 이 없는 world: 비불투명 영역은 정렬된 월드 항목(깊이 5)과 추가 수집(깊이 3)을 이 순서로 그린다.
    renderer.draw(pixels, 20, 0, bare);
    renderer.present(() => {});
    assert.equal(collections, 1);
    assert.deepEqual(log, [1, 2]);
    // 불투명(4) 영역: 추가 수집기는 부르지만 월드 표시는 그리지 않는다.
    log.length = 0;
    renderer.invalidate({ left: 0, top: 0, right: 3, bottom: 3 }, 4);
    renderer.draw(pixels, 20, 0, bare);
    renderer.present(() => {});
    assert.equal(collections, 2);
    assert.deepEqual(log, []);
    // submit 이 있는 world: 같은 5,000칸 출력기가 모든 종류를 그리므로 추가 수집기는 부르지 않는다.
    renderer.invalidate({ left: 0, top: 0, right: 3, bottom: 3 }, 0);
    renderer.draw(pixels, 20, 0, { ...bare, submit: () => log.push('submit') });
    renderer.present(() => {});
    assert.equal(collections, 2);
    assert.deepEqual(log, ['submit']);
});
