// 지형 미리보기 8비트 bitmap(cpppj `o/RawTerrainBitmap`, 원본 004c1860)의 웹 대응 `core/raw-terrain-bitmap.js` 를 검사한다(4-B, WB04-E293).
//  - 고정 관측 `island-bitmap-x86.tsv` 28행(10.78; 두 x87 관측 포함): 실제 보호 texture(TYPE 102 fringe·97 지면의 SHP0 을 200 stride 40,000BYTE 에 그림)로 전체 픽셀·로컬 난수 최종 상태·분류 캐시·
//    녹색 조회 횟수·texture 조회 순서(102, 97)가 같다. 입력 문자 지도는 모두 Native 가 쓴 명시 profile 이다.
//  - 고정 관측 `island-bitmap-parent-x86.tsv` 12행: 청크 지도 → RawTerrainContext → RawTerrainGrowth.runNormal → MarkEdges('x','=') → RawTerrainBitmap 을 원문 상위(004c1cc0)와 같은 순서로
//    합성해 합성하지 않은 Native 전체 부모의 픽셀과 대조한다(합성 함수는 이 시험 안에만 있다: cpppj `client/EditorIslandBitmap` 은 Edit 범위라 이식 대상이 아니다).
//  - 계약: 작은 texture 의 반복 주기(폭-1/높이-1)·fringe 길이 난수 한계·위 칸 분류(120)·최초 분류 유지·조회 장치 누락/texture 실패 접두·비정상 texture/문자/크기 거부.
// 실제 게임·브라우저 실행이 아니다. (2026-11-11 WB04-E293)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows } from './raw-support.mjs';
import { realCaptureAssets } from './capture-assets-support.mjs';
import { drawShape, DRAW_OK } from '../www/render/vfx-draw.js';
import { ChunkMap } from '../www/assets/territory.js';
import { GameRandom } from '../www/core/random.js';
import { RawTerrainContext, TerrainContextGlobals } from '../www/core/raw-terrain-context.js';
import { RawTerrainGrowth } from '../www/core/raw-terrain-growth.js';
import { RawTerrainBitmap, TerrainBitmapState } from '../www/core/raw-terrain-bitmap.js';

// 원본 관측 파일 행(머리글 제외)이다. 파일이 없으면 null 이라 해당 검사를 건너뛴다. (2026-11-11 WB04-E293)
const rowsOf = (name) => readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', name))?.filter((row) => row[0] !== 'name') ?? null;
const BITMAP_ROWS = rowsOf('island-bitmap-x86.tsv');
const PARENT_ROWS = rowsOf('island-bitmap-parent-x86.tsv');

// 바이트의 소문자 16진 문자열이다(빈 출력은 fixture 처럼 '-'). (2026-11-11 WB04-E293)
const hexOrDash = (bytes) => (bytes.length ? Buffer.from(bytes.buffer, bytes.byteOffset, bytes.byteLength).toString('hex') : '-');

// 실제 현재 TYPE 의 SHP 0 번 프레임을 200 stride 의 255(투명)로 채운 40,000BYTE 에 그린 texture 이다(cpppj `ReadEditorIslandTexture` 와 같은 순서). (2026-11-11 WB04-E293)
function readIslandTexture(type) {
    const { table, database } = realCaptureAssets();
    if (type < 70 || type - 70 >= table.definitions.length) throw new RangeError('섬 bitmap 현재 texture TYPE 범위');
    const block = type - 70;
    const header = database.squidMetrics(block, 0);
    const pixels = new Uint8Array(40000).fill(255);
    if (drawShape(pixels, 200, 200, database, block, 0, header.hotspotX, header.hotspotY, { left: 0, top: 0, right: 199, bottom: 199 }) !== DRAW_OK) throw new Error('섬 bitmap의 실제 SHP0 출력 실패');
    return { width: header.width, height: header.height, pixels };
}

// Native 가 쓴 X 열 우선 입력이다(사용 영역 밖도 허공). profile 은 기대 출력과 독립이다. (2026-11-11 WB04-E293)
function bitmapCodes(profile) {
    const result = new Uint8Array(65536).fill(0x2e);
    // 사용 영역 밖의 원문 배열도 허공 입력을 유지한다.
    for (let x = 0; x < 64; x += 1) {
        // 해당 X 열의 Y 입력을 Native 와 같은 패턴으로 공급한다.
        for (let y = 0; y < 64; y += 1) {
            let value = 0x2e;
            if (profile === 1) value = x >= 2 && x < 13 && y >= 2 && y < 11 ? 0x78 : x >= 2 && x < 13 && y === 11 ? 0x3d : 0x2e;
            else if (profile === 2) value = 'x=..Xo'.charCodeAt((x * 7 + y) % 6);
            else if (profile === 3) value = y % 8 === 0 ? 0x78 : y % 8 === 1 || y % 8 === 2 ? 0x3d : 0x2e;
            else if (profile === 4) value = y === 0 ? 0x78 : y === 1 ? 0x3d : 0x2e;
            result[x * 256 + y] = value;
        }
    }
    return result;
}

// 실제 보호 texture/RLE 와 원본 bitmap 28행의 전체 픽셀·분류 캐시·난수를 비교한다. (2026-11-11 WB04-E293)
test('지형 bitmap 은 관측한 전체 픽셀·분류 캐시·로컬 난수를 그대로 재생한다', { skip: BITMAP_ROWS === null }, () => {
    let checked = 0;
    // 두 x87 관측도 같은 Native 입력/출력 행으로 별도 재생한다.
    for (const row of BITMAP_ROWS) {
        const width = Number(row[1]);
        const height = Number(row[2]);
        const state = new TerrainBitmapState();
        state.initialized = Number(row[5]);
        state.classified[0x78] = Number(row[6]);
        const random = new GameRandom(Number(row[3]));
        const textures = [];
        let greenCalls = 0;
        const body = new RawTerrainBitmap(state, { value: 97 }, {
            green: () => { greenCalls += 1; return Number(row[4]); },
            texture: (type) => { textures.push(type); return readIslandTexture(type); },
        });
        const image = body.render(bitmapCodes(Number(row[7])), width, height, random);
        assert.equal(image.width, width);
        assert.equal(image.height, height);
        assert.equal(image.pixels.length, width * height);
        assert.equal(random.state, Number(row[9]), row[0]);
        assert.equal(state.initialized, 1);
        assert.equal(state.classified[0x78], Number(row[10]), row[0]);
        assert.equal(Number(row[11]), 0xa5);
        assert.deepEqual(textures, [102, 97]);
        assert.equal(greenCalls, Number(row[5]) ? 0 : 1);
        const actual = hexOrDash(image.pixels);
        if (actual !== row[13]) assert.fail(`${row[0]} 행 ${checked}: 픽셀 불일치 (첫 차이 ${[...image.pixels].findIndex((value, index) => value !== Number.parseInt(row[13].slice(index * 2, index * 2 + 2), 16))})`);
        checked += 1;
    }
    assert.equal(checked, 28);
});

// 원문 상위(004c1cc0)의 합성 순서: 청크 → context(owner, noRegion, bounds) → 일반 성장 → MarkEdges('x','=') → owner × 0x10e3 로컬 난수의 Render 이다. (2026-11-11 WB04-E293)
function buildParent(owner, bounds, chunks) {
    const globals = new TerrainContextGlobals();
    const context = new RawTerrainContext('patch1078', owner, globals.noRegion, bounds, globals, () => { throw new Error('섬 bitmap 원문 context 진단'); });
    new RawTerrainGrowth(context, chunks, globals).runNormal();
    context.markEdges(0x78, 0x3d);
    const state = new TerrainBitmapState();
    state.initialized = 1;
    state.classified[0x78] = 43;
    const body = new RawTerrainBitmap(state, { value: globals.islandType }, {
        green: () => { throw new Error('Native 전체 부모의 준비된 최초 분류'); },
        texture: readIslandTexture,
    });
    return body.render(context.codes, (bounds.right - bounds.left) * 16, (bounds.bottom - bounds.top) * 16, new GameRandom((owner * 0x10e3) >>> 0));
}

// 실제 전체 부모가 생성한 지형/경계/RLE/Bitmap 결과와 합성하지 않은 픽셀을 대조한다(청크 지도·context·성장·경계·bitmap 의 결합). (2026-11-11 WB04-E293)
test('지형 bitmap 은 청크 지도에서 성장·경계까지 합성한 Native 전체 부모의 픽셀을 그대로 재생한다', { skip: PARENT_ROWS === null }, () => {
    let checked = 0;
    // offset bounds 와 owner 0/1/8 및 실제 14×14 크기도 별도 Native 행이다.
    for (const row of PARENT_ROWS) {
        const chunks = new ChunkMap(14, 14);
        const profile = Number(row[6]);
        const points = profile === 0 ? [] : profile === 1 ? [[2, 2]] : [[2, 2], [3, 2], [2, 3], [3, 3]];
        // 원본 CPU 에 공급한 같은 물리 청크/side/record seed 입력만 작성한다.
        points.forEach(([x, y], index) => {
            const cell = chunks.at(x, y);
            cell.island = 0;
            cell.side = String.fromCharCode(65 + index);
            cell.record.set([0, 1, 0x22, 37 + index, 0, 0]);
        });
        const image = buildParent(Number(row[1]), { left: Number(row[2]), top: Number(row[3]), right: Number(row[4]), bottom: Number(row[5]) }, chunks);
        assert.equal(image.width, Number(row[8]));
        assert.equal(image.height, Number(row[9]));
        assert.equal(hexOrDash(image.pixels), row[11], `${row[0]} 행 ${checked}`);
        checked += 1;
    }
    assert.equal(checked, 12);
});

// ───────────── 계약 ─────────────

// 작은 texture 를 쓰는 보조 조회 장치이다. pixels 는 200 stride 이며 칸 값은 (x + 10 × y + base) 이다. (2026-11-11 WB04-E293)
function smallTexture(width, height, base) {
    const pixels = new Uint8Array(40000);
    for (let y = 0; y < 200; y += 1) for (let x = 0; x < 200; x += 1) pixels[y * 200 + x] = (x + 10 * y + base) & 0xff;
    return { width, height, pixels };
}

// 지면 texture 의 반복 주기는 (폭 - 1, 높이 - 1) 이고 분류된 칸만 덮는다(비분류 칸은 0 을 유지). (2026-11-11 WB04-E293)
test('지형 bitmap 의 지면 texture 는 폭-1/높이-1 주기로 분류된 칸만 덮는다', () => {
    const codes = new Uint8Array(65536).fill(0x2e);
    // 6×5 영역 중 x 열 짝수·y 행 홀수가 아닌 칸을 'x' 로 둔다.
    for (let x = 0; x < 6; x += 1) for (let y = 0; y < 5; y += 1) if (!(x % 2 === 0 && y % 2 === 1)) codes[x * 256 + y] = 0x78;
    const state = new TerrainBitmapState();
    state.initialized = 1;
    state.classified[0x78] = 7;
    const body = new RawTerrainBitmap(state, { value: 97 }, { texture: (type) => (type === 97 ? smallTexture(3, 4, 5) : smallTexture(2, 2, 0)) });
    const image = body.render(codes, 6, 5, new GameRandom(1));
    for (let y = 0; y < 5; y += 1) {
        for (let x = 0; x < 6; x += 1) {
            const covered = !(x % 2 === 0 && y % 2 === 1);
            assert.equal(image.pixels[y * 6 + x], covered ? ((x % 2) + 10 * (y % 3) + 5) & 0xff : 0, `${x},${y}`);
        }
    }
});

// fringe: 위 칸이 'x' 이면 난수 `next(3) + 3` 길이를 texture 1 행부터 칠하고(출력 높이로 끊음), 위 칸이 '.' 이면 120, 위 칸이 '=' 이면 그대로 둔다. (2026-11-11 WB04-E293)
test('지형 bitmap 의 fringe 는 위 칸 분류와 난수 길이로 칠해진다', () => {
    const codes = new Uint8Array(65536).fill(0x2e);
    // 열 0: (0,3)='x' (0,4)='=' → 길이 4 로 y 4..7 을 칠한다(높이 8). 열 1: (1,2)='.' (1,3)='=' → 120. 열 2: (2,2)='=' (2,3)='=' → 둘 다 그대로.
    codes[0 * 256 + 3] = 0x78;
    codes[0 * 256 + 4] = 0x3d;
    codes[1 * 256 + 3] = 0x3d;
    codes[2 * 256 + 2] = 0x3d;
    codes[2 * 256 + 3] = 0x3d;
    const state = new TerrainBitmapState();
    state.initialized = 1;
    const limits = [];
    const random = { next: (limit) => { limits.push(limit); return 1; } };
    const body = new RawTerrainBitmap(state, { value: 97 }, { texture: (type) => (type === 102 ? smallTexture(5, 4, 100) : smallTexture(9, 9, 0)) });
    const image = body.render(codes, 3, 8, random);
    // 위 칸이 'x' 인 '=' 는 (0,4) 하나뿐이라 난수는 한 번, 제한은 3 이다.
    assert.deepEqual(limits, [3]);
    const at = (x, y) => image.pixels[y * 3 + x];
    // 열 0: y 4..7 (길이 4), 샘플 행 = (step + 1) % (높이 4 - 1) = 1, 2, 0, 1 이고 열 = x % (폭 5 - 1).
    assert.deepEqual([4, 5, 6, 7].map((y) => at(0, y)), [1, 2, 0, 1].map((row) => (0 + 10 * row + 100) & 0xff));
    assert.equal(at(0, 3), 0);
    // 열 1: 위 칸이 '.' 인 '=' 는 120.
    assert.equal(at(1, 3), 120);
    // 열 2: 위 칸이 '=' 인 (2,3) 은 그대로 0, 그 위 (2,2) 는 위 칸이 '.' 이라 120.
    assert.equal(at(2, 3), 0);
    assert.equal(at(2, 2), 120);
    // 첫 행(y 0)의 '=' 는 순회에서 제외된다.
    const top = new Uint8Array(65536).fill(0x2e);
    top[0] = 0x3d;
    const topImage = new RawTerrainBitmap(state, { value: 97 }, { texture: () => smallTexture(5, 4, 0) }).render(top, 1, 3, { next: () => { throw new Error('호출되면 안 됨'); } });
    assert.equal(topImage.pixels[0], 0);
});

// 최초 분류는 한 번만 읽고 그 뒤 팔레트/세션 교체와 무관하게 유지된다. 장치 누락과 texture 실패는 앞선 분류 접두를 되돌리지 않는다. (2026-11-11 WB04-E293)
test('지형 bitmap 의 최초 분류는 유지되고 장치 누락/texture 실패는 접두를 보존한다', () => {
    const codes = bitmapCodes(1);
    // 녹색 조회 누락: 분류 캐시는 이미 지워졌고 초기화 표식은 아직 0 이다.
    const state = new TerrainBitmapState();
    state.classified.fill(0xa5);
    assert.throws(() => new RawTerrainBitmap(state, { value: 97 }, {}).render(codes, 16, 16, new GameRandom(1)), /녹색 조회 미연결/);
    assert.equal(state.initialized, 0);
    assert.ok(state.classified.every((value) => value === 0));
    // texture 조회 누락.
    assert.throws(() => new RawTerrainBitmap(state, { value: 97 }, { green: () => 43 }).render(codes, 16, 16, new GameRandom(1)), /texture 조회 미연결/);
    assert.equal(state.initialized, 1);
    assert.equal(state.classified[0x78], 43);
    // texture 실패: 앞선 분류 완료는 되돌리지 않고 로컬 난수는 아직 소비하지 않는다.
    const failing = new TerrainBitmapState();
    failing.classified.fill(0xa5);
    const random = new GameRandom(123);
    let calls = 0;
    const body = new RawTerrainBitmap(failing, { value: 97 }, { green: () => 43, texture: (type) => { calls += 1; assert.equal(type, 102); throw new Error('selected-texture-stop'); } });
    assert.throws(() => body.render(codes, 16, 16, random), /selected-texture-stop/);
    assert.equal(calls, 1);
    assert.equal(random.state, 123);
    assert.equal(failing.initialized, 1);
    assert.equal(failing.classified[0x78], 43);
    assert.equal(failing.classified[0x2e], 0);
    assert.equal(failing.classified[0x3d], 0);
    // 최초 분류가 끝난 뒤에는 녹색 조회를 다시 하지 않는다. 현재 지면 TYPE 상자와 fringe TYPE 은 호출마다 읽는다.
    const types = [];
    const island = { value: 97 };
    const again = new RawTerrainBitmap(state, island, { green: () => { throw new Error('다시 읽으면 안 됨'); }, texture: (type) => { types.push(type); return smallTexture(4, 4, 0); } });
    again.render(codes, 4, 4, new GameRandom(1));
    island.value = 98;
    state.fringeType = 103;
    again.render(codes, 4, 4, new GameRandom(1));
    assert.deepEqual(types, [102, 97, 103, 98]);
});

// 비정상 texture·문자·크기는 정상 이미지로 가장하지 않고 거부한다. (2026-11-11 WB04-E293)
test('지형 bitmap 은 비정상 texture/문자/크기/입력 종류를 거부한다', () => {
    const state = new TerrainBitmapState();
    state.initialized = 1;
    const codes = bitmapCodes(1);
    const make = (texture) => new RawTerrainBitmap(state, { value: 97 }, { texture });
    const random = () => new GameRandom(1);
    // texture 헤더 범위: 폭/높이 1 이하, 200 초과, 픽셀 길이 오류.
    assert.throws(() => make(() => smallTexture(1, 4, 0)).render(codes, 2, 2, random()), /texture 헤더/);
    assert.throws(() => make(() => smallTexture(4, 1, 0)).render(codes, 2, 2, random()), /texture 헤더/);
    assert.throws(() => make(() => smallTexture(201, 4, 0)).render(codes, 2, 2, random()), /texture 헤더/);
    assert.throws(() => make(() => smallTexture(4, 201, 0)).render(codes, 2, 2, random()), /texture 헤더/);
    assert.throws(() => make(() => ({ width: 4, height: 4, pixels: new Uint8Array(100) })).render(codes, 2, 2, random()), /texture 헤더/);
    assert.throws(() => make(() => null).render(codes, 2, 2, random()), /texture 헤더/);
    // 원본에서 허용되는 경계(폭/높이 2·200)는 통과한다.
    make(() => smallTexture(2, 2, 0)).render(codes, 2, 2, random());
    make(() => smallTexture(200, 200, 0)).render(codes, 2, 2, random());
    // 정수가 아닌 폭/높이, 일반 배열 픽셀, fringe 만 잘못된 경우도 거부한다.
    assert.throws(() => make(() => ({ ...smallTexture(4, 4, 0), width: 4.5 })).render(codes, 2, 2, random()), /texture 헤더/);
    assert.throws(() => make(() => ({ ...smallTexture(4, 4, 0), height: 4.5 })).render(codes, 2, 2, random()), /texture 헤더/);
    assert.throws(() => make(() => ({ width: 4, height: 4, pixels: new Array(40000).fill(0) })).render(codes, 2, 2, random()), /texture 헤더/);
    assert.throws(() => make((type) => (type === 102 ? smallTexture(1, 1, 0) : smallTexture(4, 4, 0))).render(codes, 2, 2, random()), /texture 헤더/);
    // 지면 texture 만 잘못된 경우에도 거부한다(fringe 는 통과).
    assert.throws(() => make((type) => (type === 102 ? smallTexture(4, 4, 0) : smallTexture(1, 1, 0))).render(codes, 2, 2, random()), /texture 헤더/);
    // 문자 분류의 signed 범위(0x80 이상)와 크기 범위.
    const bad = new Uint8Array(65536).fill(0x2e);
    bad[0] = 0x80;
    assert.throws(() => make(() => smallTexture(4, 4, 0)).render(bad, 2, 2, random()), /signed 문자/);
    assert.throws(() => make(() => smallTexture(4, 4, 0)).render(codes, 257, 2, random()), /문자 저장소 범위/);
    assert.throws(() => make(() => smallTexture(4, 4, 0)).render(codes, 2, 257, random()), /문자 저장소 범위/);
    assert.throws(() => make(() => smallTexture(4, 4, 0)).render(codes, -1, 2, random()), /문자 저장소 범위/);
    assert.throws(() => make(() => smallTexture(4, 4, 0)).render(codes, 2, -1, random()), /문자 저장소 범위/);
    assert.throws(() => make(() => smallTexture(4, 4, 0)).render(codes, 1.5, 2, random()), /문자 저장소 범위/);
    assert.equal(make(() => smallTexture(4, 4, 0)).render(codes, 256, 1, random()).pixels.length, 256);
    assert.equal(make(() => smallTexture(4, 4, 0)).render(codes, 1, 256, random()).pixels.length, 256);
    // 입력 종류.
    assert.throws(() => make(() => smallTexture(4, 4, 0)).render(new Uint8Array(100), 2, 2, random()), TypeError);
    assert.throws(() => make(() => smallTexture(4, 4, 0)).render(new Array(65536).fill(0x2e), 2, 2, random()), TypeError);
    // 0x7f 는 signed 상한(0x80) 아래라 정상 분류 입력이다.
    const edge = new Uint8Array(65536).fill(0x2e);
    edge[0] = 0x7f;
    make(() => smallTexture(4, 4, 0)).render(edge, 2, 2, random());
    assert.throws(() => make(() => smallTexture(4, 4, 0)).render(codes, 2, 2, {}), TypeError);
    assert.throws(() => new RawTerrainBitmap({}, { value: 97 }, {}), TypeError);
    assert.throws(() => new RawTerrainBitmap(state, {}, {}), TypeError);
    assert.throws(() => new RawTerrainBitmap(state, { value: 97 }, null), TypeError);
    assert.throws(() => new RawTerrainBitmap(state, { value: 97 }, { green: 1 }), TypeError);
    assert.throws(() => new RawTerrainBitmap(state, { value: 97 }, { texture: 1 }), TypeError);
});

// 최초 녹색 분류는 하위 BYTE 만 쓴다(0 도 정상 값). (2026-11-11 WB04-E293)
test('지형 bitmap 의 최초 녹색 분류는 하위 BYTE 만 보관한다', () => {
    const state = new TerrainBitmapState();
    new RawTerrainBitmap(state, { value: 97 }, { green: () => 299, texture: () => smallTexture(4, 4, 0) }).render(bitmapCodes(0), 2, 2, new GameRandom(1));
    assert.equal(state.classified[0x78], 43);
    assert.equal(state.classified.length, 255);
    assert.equal(state.fringeType, 102);
});
