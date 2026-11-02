// 표시 순서/카메라는 원본 x86 fixture 바이트와 대조하고 합성은 의도적으로 겹친 픽셀로 검증한다.
// 게임·브라우저·원본 PE는 실행하지 않는다. fixture는 읽기만 한다. (2026-11-02 WB03-Z)
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { REPO_ROOT, readData } from './helpers.mjs';
import { CameraView, cameraFtolLow } from '../www/render/camera-view.js';
import { compareDrawOrder, sortDrawEntries } from '../www/render/draw-sort.js';
import { WorldSpriteRenderer, physicalShapeFrame } from '../www/render/world-sprites.js';
import { IndexedSurface } from '../www/render/indexed-surface.js';
import { TaffArchive } from '../www/assets/taff.js';
import { TypeTable } from '../www/assets/type-table.js';
import { decodeOriginalText } from '../www/assets/text-encoding.js';
import { ShapeDatabase } from '../www/assets/shapes.js';
import { createOwnerColorTables } from '../www/assets/owner-colors.js';
import { GamePalette, buildScreenPalette } from '../www/assets/palette.js';

// 원본 fixture의 비주석 TSV 줄을 읽는다. 자료가 없는 환경은 대조를 건너뛴다.
function fixture(t, name) {
    const file = path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', name);
    if (!fs.existsSync(file)) { t.skip(`원본 fixture 없음: ${name}`); return null; }
    return fs.readFileSync(file, 'utf8').split(/\r?\n/).filter((line) => line && !line.startsWith('#')).map((line) => line.split('\t'));
}

// 16바이트 표시 항목을 키와 원시 바이트로 읽는다. token/kind/tail/float 비트는 재생성하지 않는다.
function entries(hex) {
    if (hex === '-') return [];
    const bytes = Buffer.from(hex, 'hex');
    const result = [];
    // 항목 전체의 원래 바이트 참조를 보존한다.
    for (let offset = 0; offset < bytes.length; offset += 16) {
        result.push({ x: bytes.readFloatLE(offset), y: bytes.readFloatLE(offset + 4), depth: bytes.readInt16LE(offset + 12), bytes: bytes.subarray(offset, offset + 16) });
    }
    return result;
}

// 원본 실행 결과의 전체 16바이트 항목 순서와 비교한다. JS 정렬로 기대값을 만들지 않는다.
test('10.78 CRT 정렬: 원본 325줄의 전체 항목 바이트 일치', (t) => {
    const rows = fixture(t, 'drawsort-x86.tsv');
    if (!rows) return;
    let compared = 0;
    // CD/추가 판본은 이 모듈의 범위 밖이다.
    for (const row of rows) {
        if (row[0] !== 'originals') continue;
        const work = entries(row[3]);
        sortDrawEntries(work);
        assert.equal(work.length ? Buffer.concat(work.map((entry) => entry.bytes)).toString('hex') : '-', row[4], `${row[1]} ${row[2]}`);
        compared += 1;
    }
    assert.equal(compared, 325);
});

// 키 검증이 뒤쪽에서 실패해도 입력의 참조 순서와 상태를 먼저 바꾸지 않는다.
test('정렬: signed 깊이·완전 동률·float32 계약과 변경 전 거부', () => {
    assert.equal(compareDrawOrder({ x: 0, y: 0, depth: 0 }, { x: 0, y: 0, depth: 0 }), 1);
    const tied = Array.from({ length: 9 }, (_, i) => ({ x: 0, y: 0, depth: 0, token: i + 1 }));
    sortDrawEntries(tied);
    assert.deepEqual(tied.map((entry) => entry.token), [9, 5, 3, 4, 2, 6, 7, 8, 1]);
    const depths = [{ x: 0, y: 0, depth: -32768 }, { x: 0, y: 0, depth: 32767 }];
    sortDrawEntries(depths);
    assert.deepEqual(depths.map((entry) => entry.depth), [32767, -32768]);
    // 부호 없는 깊이·비유한/미저장 float 좌표를 조용히 정규화하지 않는다.
    for (const invalid of [{ x: NaN }, { y: Infinity }, { x: 0.1 }, { depth: 65535 }, { depth: 0.5 }]) {
        const bad = [...tied, { x: 0, y: 0, depth: 0, ...invalid }];
        const before = [...bad];
        assert.throws(() => sortDrawEntries(bad), RangeError);
        assert.deepEqual(bad, before);
    }
});

// fixture의 argX/Y는 float 숫자가 아닌 DWORD 비트 문자열이다.
function floatBits(hex) {
    const bytes = Buffer.alloc(4);
    bytes.writeUInt32LE(Number.parseInt(hex, 16));
    return bytes.readFloatLE();
}

// 16개 입력의 전체64바이트를 원본 관측과 대조한다(무관 DWORD·NaN payload·음수0 포함).
test('카메라: 원본 16줄의 초기화·world/pixels setter·투영 바이트 일치', (t) => {
    const rows = fixture(t, 'camera-view-x86.tsv');
    if (!rows) return;
    let compared = 0;
    // 헤더만 제외하고 모든 원본 동작을 재생한다.
    for (const row of rows) {
        if (row[0] === 'operation') continue;
        const view = new CameraView(new Uint8Array(Buffer.from(row[1], 'hex')));
        const diagnostics = [];
        const assertion = (message, line) => diagnostics.push([message, line]);
        if (row[0] === 'init') view.initialize();
        else if (row[0] === 'world') view.setWorld(floatBits(row[2]), floatBits(row[3]), assertion);
        else if (row[0] === 'pixels') view.setPixels(Number.parseInt(row[2], 16) | 0, Number.parseInt(row[3], 16) | 0, assertion);
        else if (row[0] === 'project') {
            const point = view.project(floatBits(row[2]), floatBits(row[3]));
            // 원본 관측기는 반환 두 좌표를 앞 두 쌍에 기록했다.
            for (const offset of [0, 8]) {
                view.data.setInt32(offset, point.x, true);
                view.data.setInt32(offset + 4, point.y, true);
            }
        } else assert.fail(`알 수 없는 동작: ${row[0]}`);
        assert.equal(Buffer.from(view.bytes).toString('hex'), row[4], `${row[0]} ${row[2]} ${row[3]}`);
        // Python csv가 quote한 JSON 필드도 같은 메시지·행 번호로 해독한다.
        const text = row[9].startsWith('"') ? row[9].slice(1, -1).replaceAll('""', '"') : row[9];
        assert.deepEqual(diagnostics, JSON.parse(text));
        compared += 1;
    }
    assert.equal(compared, 16);
});

// 카메라의 월드 분수·독립 snapshot·음수 ftol·DWORD 감김·커서 한 칸 보정을 구별한다.
test('카메라: 분수 원점·독립 사본·음수 투영·역변환·진단 후 재조회', () => {
    const view = new CameraView();
    assert.ok(view.bytes.every((byte) => byte === 0));
    view.initialize();
    view.setWorld(1.25, 2.5);
    const snapshot = view.copy();
    // Buffer의 부분 보기도 constructor에서 독립64바이트로 복사한다(byteOffset을 잃지 않는다).
    const backing = Buffer.alloc(80, 0xa5);
    Buffer.from(view.bytes).copy(backing, 8);
    const copied = new CameraView(backing.subarray(8, 72));
    assert.deepEqual(copied.bytes, view.bytes);
    copied.bytes[0] ^= 255;
    assert.notEqual(copied.bytes[0], backing[8]);
    assert.deepEqual(view.pixels, { x: 20, y: 27 });
    assert.deepEqual(view.world, { x: 1.25, y: 2.5 });
    view.setPixels(17, 29);
    assert.deepEqual(snapshot.pixels, { x: 20, y: 27 });
    assert.deepEqual(snapshot.project(-1, -1), { x: -35, y: -37 });
    assert.deepEqual(snapshot.screenToWorld(-16, -11), snapshot.world);
    assert.deepEqual(snapshot.screenToWorld(0, 0), { x: 2.25, y: 3.5 });
    assert.equal(view.world.y, Math.fround(29 * Math.fround(1 / 11)));
    // _ftol indefinite의 하위 DWORD와 signed32 감김을 확인한다.
    for (const value of [NaN, Infinity, -Infinity, 2 ** 63, -(2 ** 63) - 2048]) assert.equal(cameraFtolLow(value), 0);
    assert.equal(cameraFtolLow(0x100000001), 1);
    assert.equal(cameraFtolLow(-0x100000001), -1);
    const messages = [];
    view.setWorld(floatBits('4d000001'), floatBits('4d800001'), (message) => {
        messages.push(message);
        view.data.setFloat32(4, -1, true);
    });
    assert.deepEqual(messages, ['scrollPosPixX >= 0']);
    assert.ok(view.pixels.y < 0);
});

// 한 픽셀짜리 서로 다른 프레임의 간단한 자산 장치다. 원본 상태 계산을 대신하지 않는다.
function pixelAssets() {
    const frames = [10, 20, 30, 40].map((color) => ({
        special: false, rect: { left: 0, top: 0, right: 0, bottom: 0 },
        image: { width: 1, height: 1, indices: Uint8Array.of(color), opacity: Uint8Array.of(255) },
    }));
    return {
        table: { view: (type) => type === 70 ? { block: 0, frameCount: 2, hotspotX: 1, hotspotY: 2 } : null },
        shapes: {
            frame: (block, frame) => { if (block !== 0 || !frames[frame]) throw new RangeError('frame'); return frames[frame]; },
            decode: (block, frame) => frames[frame].image,
        },
    };
}

// 실제 같은 위치에서 겹치는 픽셀로 정렬 결과와 색표/프레임 이동을 확인한다.
test('월드 그림: 타입 기준점·9개 동률 마지막 색·정렬 끔·후속 레이어 연속 합성', () => {
    const renderer = new WorldSpriteRenderer(pixelAssets());
    const camera = new CameraView();
    camera.initialize();
    const surface = new IndexedSurface(32, 32);
    const sprites = Array.from({ length: 9 }, (_, index) => ({ type: 70, frame: 0, x: 1, y: 1, depth: 0,
        colorMap: Uint8Array.from({ length: 256 }, () => index + 1) }));
    const original = [...sprites];
    renderer.draw(surface, camera, sprites);
    assert.equal(surface.pixels[9 * 32 + 15], 1);
    assert.deepEqual(sprites, original);
    renderer.draw(surface, camera, sprites, { sort: false });
    assert.equal(surface.pixels[9 * 32 + 15], 9);
    // 깊이10의 본체·후속 layer1 다음에 깊이0의 본체가 그려져야 한다.
    renderer.draw(surface, camera, [
        { type: 70, frame: 0, x: 1, y: 1, depth: 10, following: [{ frame: 0, layer: 1 }] },
        { type: 70, frame: 1, x: 1, y: 1, depth: 0 },
    ]);
    assert.equal(surface.pixels[9 * 32 + 15], 20);
    renderer.draw(surface, camera, [{ type: 70, frame: 0, x: 1, y: 1, depth: 0, following: [{ frame: 1, layer: 1 }] }]);
    assert.equal(surface.pixels[9 * 32 + 15], 40);
    assert.throws(() => physicalShapeFrame(renderer.table, renderer.shapes, 70, 2), RangeError);
    assert.throws(() => physicalShapeFrame(renderer.table, renderer.shapes, 70, 0, 2), RangeError);
});

// 그림자 마스크의 원색은 무시하고 이미 있는 배경을 치환한다. 투명/clip은 배경을 보존한다.
test('8비트 합성: 그림자 대상 색 변환·투명·음수 위치·right/bottom 제외', () => {
    const surface = new IndexedSurface(4, 3);
    surface.fill(7);
    const image = { width: 3, height: 2, indices: Uint8Array.of(2, 3, 4, 5, 6, 7), opacity: Uint8Array.of(255, 0, 255, 255, 255, 255) };
    const map = Uint8Array.from({ length: 256 }, (_, color) => (color + 10) & 255);
    surface.drawImage(image, -1, 0, { left: 0, top: 0, right: 1, bottom: 2 }, map, true);
    assert.deepEqual([...surface.pixels], [7, 7, 7, 7, 17, 7, 7, 7, 7, 7, 7, 7]);
    surface.drawImage(image, 0, 0, { left: 0, top: 0, right: 3, bottom: 1 }, map, true);
    assert.deepEqual([...surface.pixels.slice(0, 4)], [17, 7, 17, 7]);
    assert.throws(() => surface.drawImage(image, 0, 0, surface.bounds, null, true), TypeError);
    const renderer = new WorldSpriteRenderer(pixelAssets());
    const camera = new CameraView();
    camera.initialize();
    assert.throws(() => renderer.draw(surface, camera, [{ type: 70, frame: 0, x: 0, y: 0, depth: 0, shadow: true }]), TypeError);
});

// 렌더러를 통한 그림자 경로도 소유자 표를 배경 표로 잘못 사용하지 않는다.
test('월드 그림: 그림자 레이어·화면 밖 해독 생략·특수 레코드·clip 경계', () => {
    const assets = pixelAssets();
    const colors = Uint8Array.from({ length: 256 }, (_, color) => color + 1);
    const ownerColors = new Map([[70, Array.from({ length: 9 }, () => Uint8Array.from({ length: 256 }, () => 99))]]);
    const renderer = new WorldSpriteRenderer({ ...assets, ownerColors });
    const camera = new CameraView();
    camera.initialize();
    const surface = new IndexedSurface(32, 32);
    surface.fill(7);
    const sprite = { type: 70, frame: 0, layer: 1, x: 1, y: 1, depth: 0, ownerColor: 1, shadow: true, colorMap: colors };
    renderer.draw(surface, camera, [sprite]);
    assert.equal(surface.pixels[9 * 32 + 15], 8);
    renderer.draw(surface, camera, [sprite], { clip: { left: 0, top: 0, right: 15, bottom: 32 } });
    renderer.draw(surface, camera, [sprite], { clip: { left: 0, top: 0, right: 32, bottom: 9 } });
    assert.equal(surface.pixels[9 * 32 + 15], 8);
    // 오류를 내는 해독기로 화면 밖/그림 없는 레코드가 실제로 해독되지 않음을 확인한다.
    assets.shapes.decode = () => assert.fail('보이지 않는 프레임을 해독함');
    renderer.draw(surface, camera, [{ ...sprite, x: 10 }]);
    assets.shapes.frame(0, 2).special = true;
    renderer.draw(surface, camera, [sprite]);
    assert.equal(surface.pixels[9 * 32 + 15], 8);
    // 등록되지 않은 색 번호와 없는 타입은 합성 전에 거부한다.
    assert.throws(() => renderer.plan(camera, [{ ...sprite, ownerColor: 9 }]), RangeError);
    assert.throws(() => renderer.plan(camera, [{ ...sprite, type: 69 }]), RangeError);
});

// 공개 자산의116타입 전체에서 물리 레이어 계산을 검증하고 실제 지면의 소유자 색 치환을 확인한다.
test('원본 자산: 116타입의 모든 레이어와 지면 SHP 기준점/소유자 색 합성', () => {
    const archive = new TaffArchive(readData('netstorm.tarc'));
    const table = TypeTable.fromArchive(archive, decodeOriginalText);
    const shapes = new ShapeDatabase(readData('d/_shapes.shp'));
    // 타입마다 실제 물리 프레임 수를 독립 원본 헤더에서 읽는다.
    for (let type = 70; type < 186; type += 1) {
        const view = table.view(type);
        const count = shapes.blocks[view.block].frames.length;
        for (let index = 0; index < count; index += 1) {
            assert.equal(physicalShapeFrame(table, shapes, type, index % view.frameCount, Math.floor(index / view.frameCount)), index);
        }
    }
    const palette = buildScreenPalette(GamePalette.parse(readData('d/GIFCLOUD.COL')));
    const ownerColors = createOwnerColorTables(palette.logical, { useBanners: 0 });
    const renderer = new WorldSpriteRenderer({ table, shapes, ownerColors });
    const camera = new CameraView();
    camera.initialize();
    camera.setPixels(1600, 1100);
    const type = table.find('isle');
    const view = table.view(type);
    // isle의100번은 소유자 색 소스가 있는 실제 가장자리 클러스터이다(0번은 소스 색이 없다).
    const image = shapes.decode(view.block, 100);
    const surface = new IndexedSurface(160, 110);
    surface.fill(7);
    renderer.draw(surface, camera, [{ type, frame: 100, x: 104, y: 104, depth: 30, ownerColor: 3 }]);
    // 알려진 월드→픽셀 기준(1664-1600, 1144-1100)에서 모든 출력 픽셀을 검사한다.
    const expected = new Uint8Array(surface.pixels.length).fill(7);
    let covered = 0;
    let remapped = 0;
    for (let y = 0; y < image.height; y += 1) {
        for (let x = 0; x < image.width; x += 1) {
            const input = y * image.width + x;
            if (!image.opacity[input]) continue;
            const outX = 64 - view.hotspotX + image.left + x;
            const outY = 44 - view.hotspotY + image.top + y;
            if (outX < 0 || outX >= 160 || outY < 0 || outY >= 110) continue;
            const color = image.indices[input];
            const output = ownerColors.get(type)[3][color];
            expected[outY * 160 + outX] = output;
            covered += 1;
            if (color !== output) remapped += 1;
        }
    }
    assert.ok(covered > 0);
    assert.ok(remapped > 0);
    assert.deepEqual(surface.pixels, expected);
});
