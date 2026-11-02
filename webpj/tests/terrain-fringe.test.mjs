// 절벽 fringe(terrain-fringe.js)를 검사한다.
//  1) cpppj 가 원본 실행 파일(10.78)을 제한 x86 으로 실행해 얻어 둔 첫 도장 fixture(cpppj/tests/fixtures/terrainstamp-x86.tsv)를 다시 돌려, 칸마다 만든 지면·절벽의 타입/프레임/위치 비트와 게임 난수·CRT 난수의 최종 상태가 같은지 본다
//     (난수를 소비하는 순서·제외 플래그 때문에 다시 고르는 반복·부호 확장 포함).
//  2) 같은 fixture 의 프레임 선택 줄(Frame)을 다시 돌려 이웃 비트와 방향 글자, 안쪽 3 × 3 그림 표, 가장자리 후보 선택이 원본과 같은지 본다(앞 단위의 terrain-tiles.js 를 원본 실행 결과로 다시 확인).
//  3) 원본 fringe.type·isle.type 텍스트를 따로 읽은 기대값 표(tests/fixtures/terrain-fringe-expected.tsv, webpj/tools/make-terrain-fringe-expected.py)와 글자 쌍 256가지의 후보·제외 후 후보를 대조하고,
//     원본 `.fort` 465개 전체에서 buildTerrainFrames + 절벽 생성기의 결과가 그 기대 후보 안에서만 나오는지 확인한다.
// (2026-11-02 WB03-Y: 정적 맵 화면(M2) 3단계(c) 절벽 fringe 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { REPO_ROOT, WEBPJ_ROOT, loadOriginalForts, readData } from './helpers.mjs';
import { TaffArchive } from '../www/assets/taff.js';
import { decodeOriginalText } from '../www/assets/text-encoding.js';
import { TypeTable } from '../www/assets/type-table.js';
import { createFortTypes } from '../www/assets/type-flags.js';
import { parseFort } from '../www/assets/fort.js';
import { layoutFort, pieceFrameCodes } from '../www/assets/territory.js';
import { EMPTY_TERRAIN, LocalRandom, NEIGHBOR_CELLS, WORLD_CELLS, buildIslandMask } from '../www/assets/terrain-mask.js';
import {
    CrtRandom, FrameCodeTable, buildTerrainFrames, chooseTerrainFrame, createStampVariants, neighborLetters, stampCoreFrame,
} from '../www/assets/terrain-tiles.js';
import {
    EXCLUDE_LIT_AND_UNLIT, EXCLUDE_UNLIT, FRINGE_FLAG, FRINGE_Y_OFFSET, chooseFringeFrame, createFringeStamper, fringeCandidates,
} from '../www/assets/terrain-fringe.js';

// 공용 자료이다: 아카이브·타입 표·`isle`·`fringe` 타입의 프레임 코드 표. (2026-11-02 WB03-Y)
const archive = new TaffArchive(readData('netstorm.tarc'));
const table = TypeTable.fromArchive(archive, decodeOriginalText);
const types = createFortTypes(table);
const pieceCodes = pieceFrameCodes(table);
const isleFrames = FrameCodeTable.fromDefinition(table.definitions[table.find('isle') - 70]);
const fringeFrames = FrameCodeTable.fromDefinition(table.definitions[table.find('fringe') - 70]);

// cpppj 의 원본 실행 fixture 경로이다. 없으면 해당 검사를 건너뛴다. (2026-11-02 WB03-Y)
const NATIVE_FIXTURE = path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'terrainstamp-x86.tsv');

// 원본 실행 fixture 의 패치(originals) 줄이다: { kind, input[], result, gameState, crtState, cache, raw } . (2026-11-02 WB03-Y)
const nativeRows = fs.existsSync(NATIVE_FIXTURE)
    ? fs.readFileSync(NATIVE_FIXTURE, 'utf8').split(/\r?\n/).filter((line) => line !== '' && !line.startsWith('#')).map((line) => line.split('\t')).filter((row) => row[0] === 'originals')
        .map((row) => ({ kind: row[1], input: row[2].split(',').map(Number), result: Number(row[3]), gameState: Number(row[4]), crtState: Number(row[5]), cache: row[6], effects: row[7], raw: row[8] }))
    : [];

// 기대값 표(Python, fringe.type·isle.type 텍스트를 따로 읽음)이다. 없으면 해당 검사를 건너뛴다. (2026-11-02 WB03-Y)
const EXPECTED_FIXTURE = path.join(WEBPJ_ROOT, 'tests', 'fixtures', 'terrain-fringe-expected.tsv');
const expectedRows = fs.existsSync(EXPECTED_FIXTURE)
    ? fs.readFileSync(EXPECTED_FIXTURE, 'utf8').split(/\r?\n/).filter((line) => line !== '' && !line.startsWith('#')).map((line) => line.split('\t'))
    : [];

// 실수(float32)의 비트 패턴(부호 없는 32비트)이다. 원본 raw 슬롯에 쓰이는 좌표 표현이다. (2026-11-02 WB03-Y)
function floatBits(value) {
    const view = new DataView(new ArrayBuffer(4));
    view.setFloat32(0, value, true);
    return view.getUint32(0, true);
}

// fixture 의 합성 프레임 코드 표를 만든다(cpppj TerrainStampTests.cpp TerrainCodes 와 같은 구성): 0 = 16 × 16 글자 쌍 × 8개, 1 = 빈 표, 2 = 'AA' 300개, 3 = 방향 A·C·F·J·P 의 (방향, 'A') 8개씩,
// 4 = 지면은 0번 표·절벽은 빈 표. 절벽 표의 플래그는 {0, 2, 0x80, 1} 을 돌려 쓰고 지면 표의 플래그는 모두 1(절벽 만듦)이다. (2026-11-02 WB03-Y)
function syntheticTable(dataset, fringe) {
    if (dataset === 1 || (dataset === 4 && fringe)) return new FrameCodeTable([]);
    if (dataset === 4) dataset = 0;
    const codes = [];
    if (dataset === 2) {
        // 같은 코드 300개이다(최대 256 후보를 넘는다).
        for (let i = 0; i < 300; i += 1) codes.push({ side: 0x41, variant: 0x41, flags: 0 });
        return new FrameCodeTable(codes);
    }
    const flagCycle = [0, 2, 0x80, 1];
    // 한 글자 쌍에 같은 코드 8개를 더한다.
    const append = (side, variant) => {
        for (let i = 0; i < 8; i += 1) codes.push({ side, variant, flags: fringe ? flagCycle[i % 4] : 1 });
    };
    if (dataset === 0) {
        // 방향 글자 바깥·변형 글자 안쪽 순서이다.
        for (let side = 0x41; side <= 0x50; side += 1) for (let variant = 0x41; variant <= 0x50; variant += 1) append(side, variant);
    } else {
        // 방향 A·C·F·J·P 의 (방향, 'A')만 있다.
        for (const side of [0x41, 0x43, 0x46, 0x4a, 0x50]) append(side, 0x41);
    }
    return new FrameCodeTable(codes);
}

// 합성 칸 모음으로 마스크를 만든다: cells 의 [x, y, 영역 번호] 를 채운 256 × 256 마스크. (2026-11-02 WB03-Y)
function maskOf(cells) {
    const land = new Uint8Array(WORLD_CELLS * WORLD_CELLS).fill(EMPTY_TERRAIN);
    // 칸을 하나씩 채운다.
    for (const [x, y, region] of cells) land[y * WORLD_CELLS + x] = region;
    return land;
}

test('원본 실행 fixture: 첫 도장(Stamp) 780개 중 패치 180줄 가운데 재현 가능한 144줄의 지면·절벽 슬롯과 두 난수의 최종 상태가 같다', { skip: nativeRows.length === 0 }, () => {
    let replayed = 0;
    let withFringe = 0;
    let retried = 0;
    // 첫 도장 줄마다 2 × 2 칸을 도장한다.
    for (const row of nativeRows.filter((candidate) => candidate.kind === 'Stamp')) {
        const [scene, dataset, cache, crtSeed, gameSeed, excluded] = row.input;
        // 장면 2(같은 종류이지만 도장 대상이 아닌 이웃 칸)는 마스크로 표현할 수 없어 건너뛴다.
        if (scene === 2) continue;
        const stamped = [[50, 53], [51, 53], [50, 54], [51, 54]].filter(([x, y]) => !(scene === 1 && x === 51 && y === 53) && scene !== 4);
        const land = maskOf(stamped.map(([x, y]) => [x, y, scene === 3 && x === 51 && y === 54 ? 6 : 5]));
        const isle = syntheticTable(dataset, false);
        const fringeTable = syntheticTable(dataset, true);
        const random = new LocalRandom(gameSeed);
        const crt = new CrtRandom(crtSeed);
        const injected = cache ? Array.from({ length: 99 }, (_, i) => (i * 17 + 3) % 32768) : undefined;
        const stamper = createFringeStamper(isle, fringeTable, { excludedFlags: excluded });
        const frames = buildTerrainFrames(land, isle, { random, crt, stampVariants: injected, onStamp: stamper.onStamp });
        // 원본 raw 슬롯 문자열을 같은 순서(지면 → 절벽, y 바깥·x 안쪽)로 만든다: 번호:타입:프레임:소유자:지역:x 비트:y 비트.
        let sid = 40;
        const slots = [];
        for (const [x, y] of stamped.slice().sort((a, b) => a[1] - b[1] || a[0] - b[0])) {
            const index = y * WORLD_CELLS + x;
            slots.push(`${sid}:97:${frames[index]}:0:${land[index]}:${floatBits(x)}:${floatBits(y)}`);
            sid += 1;
            if (stamper.cells[index] !== -1) {
                slots.push(`${sid}:102:${stamper.cells[index]}:0:0:${floatBits(x)}:${floatBits(y + FRINGE_Y_OFFSET)}`);
                sid += 1;
                withFringe += 1;
            }
        }
        assert.equal(slots.length === 0 ? '-' : slots.join(';'), row.raw, `줄 ${JSON.stringify(row.input)}`);
        // 생성기가 센 절벽 수는 실제로 만든 절벽 칸의 수와 같다(후보가 없어 만들지 않은 칸은 세지 않는다).
        assert.equal(stamper.count(), stamper.cells.filter((value) => value !== -1).length, `절벽 수 ${JSON.stringify(row.input)}`);
        assert.equal(random.state, row.gameState, `게임 난수 ${JSON.stringify(row.input)}`);
        assert.equal(crt.state, row.crtState, `CRT 난수 ${JSON.stringify(row.input)}`);
        // 난수표(99개)는 이번에 만들어졌다면 같은 CRT 시작 상태에서 독립 계산한 값과 같다.
        const cacheText = injected ? injected.join(',') : stamped.length === 0 ? '-' : createStampVariants(new CrtRandom(crtSeed)).join(',');
        assert.equal(cacheText, row.cache, `난수표 ${JSON.stringify(row.input)}`);
        if (excluded !== 0 && fringeTable.length > 0) retried += 1;
        replayed += 1;
    }
    assert.equal(replayed, 144);
    assert.ok(withFringe > 100, `절벽이 만들어진 칸 ${withFringe}`);
    assert.ok(retried > 40, `제외 플래그가 있는 줄 ${retried}`);
});

test('원본 실행 fixture: 프레임 선택(Frame) 패치 600줄 중 재현 가능한 줄의 프레임·게임 난수·CRT 난수·난수표가 같다', { skip: nativeRows.length === 0 }, () => {
    let replayed = 0;
    let skipped = 0;
    let interior = 0;
    let edge = 0;
    // 프레임 선택 줄마다 한 칸을 고른다.
    for (const row of nativeRows.filter((candidate) => candidate.kind === 'Frame')) {
        const [code, straight, diagonal, x, y, crtSeed, gameSeed, dataset, cache, otherRegion] = row.input;
        // 중심 칸이 'x'(120)면 이웃 'h'(별칭 x)가 같은 종류이고, '='(61)이면 이웃 'h' 는 다른 종류라 이웃이 하나도 없다. 이웃 영역이 다르면 마찬가지로 이웃이 없다.
        const neighborsCount = code === 120 && !otherRegion;
        const cells = [[x, y, 5]];
        // 이웃 하나마다 입력 비트(짝수 번호는 직선 마스크, 홀수 번호는 대각선 마스크의 비트 index ÷ 2)가 켜졌을 때만 같은 영역의 지면 칸을 둔다.
        NEIGHBOR_CELLS.forEach(([dx, dy], index) => {
            const mask = index % 2 === 1 ? diagonal : straight;
            const inside = x + dx >= 0 && x + dx < WORLD_CELLS && y + dy >= 0 && y + dy < WORLD_CELLS;
            if (neighborsCount && inside && (mask & (1 << (index >> 1))) !== 0) cells.push([x + dx, y + dy, 5]);
        });
        const land = maskOf(cells);
        const letters = neighborLetters(land, x, y);
        const isle = syntheticTable(dataset, false);
        const random = new LocalRandom(gameSeed);
        const crt = new CrtRandom(crtSeed);
        // 원본은 가장자리 칸에서도 난수표를 먼저 만든다(전역 캐시가 비어 있을 때).
        const variants = cache ? Array.from({ length: 99 }, (_, i) => (i * 17 + 3) % 32768) : createStampVariants(crt);
        const isInterior = code === 120 && letters.side === 'A' && letters.variant === 'A';
        // 3 × 3 그림 자리표시자('J' 방향 글자)가 없는 표(빈 표·'AA' 300개)에서 원본은 -1 + 1 로 계산을 이어 가지만 이 구현은 자료 오류로 거부한다. 그 줄은 거부를 확인하고 건너뛴다.
        if (isInterior && isle.run('J').first === -1) {
            assert.throws(() => stampCoreFrame(isle, variants, x, y), RangeError);
            skipped += 1;
            continue;
        }
        const frame = isInterior ? stampCoreFrame(isle, variants, x, y) : chooseTerrainFrame(isle, letters.side, letters.variant, 0, random);
        assert.equal(frame === -1 ? 0 : frame, row.result, `프레임 ${JSON.stringify(row.input)}`);
        assert.equal(random.state, row.gameState, `게임 난수 ${JSON.stringify(row.input)}`);
        assert.equal(crt.state, row.crtState, `CRT 난수 ${JSON.stringify(row.input)}`);
        assert.equal(variants.join(','), row.cache, `난수표 ${JSON.stringify(row.input)}`);
        if (isInterior) interior += 1; else edge += 1;
        replayed += 1;
    }
    assert.equal(replayed + skipped, 600);
    assert.equal(skipped, 4);
    assert.ok(interior === 19 && edge === 577, `안쪽 ${interior} 가장자리 ${edge}`);
});

test('제외 플래그: 겹치는 후보는 난수를 다시 소비해 건너뛰고 부호 확장(0x80)도 원본처럼 겹친다', () => {
    // 합성 절벽 표: 'AA' 코드 4개, 플래그 {0, 0x10(unlit), 0x08(lit), 0x80}.
    const fringe = new FrameCodeTable([0, 0x10, 0x08, 0x80].map((flags) => ({ side: 0x41, variant: 0x41, flags })));
    const code = { side: 0x41, variant: 0x41, flags: 1 };
    assert.deepEqual(fringeCandidates(fringe, code, 0).allowed, [0, 1, 2, 3]);
    assert.deepEqual(fringeCandidates(fringe, code, EXCLUDE_UNLIT).allowed, [0, 2, 3]);
    assert.deepEqual(fringeCandidates(fringe, code, EXCLUDE_LIT_AND_UNLIT).allowed, [0, 3]);
    // 0x80 플래그는 부호 확장되어 상위 비트가 켜진 제외 플래그와 겹친다.
    assert.deepEqual(fringeCandidates(fringe, code, 0x80000000).allowed, [0, 1, 2]);
    // 난수가 0, 1, 2, 3, 0, ... 을 순서대로 주면 제외 플래그 0x10 이면 인덱스 1(unlit)을 버리고 다음 호출(2)을 쓴다.
    const sequence = (values) => {
        const calls = [];
        return { calls, next(limit) { calls.push(limit); return values[calls.length - 1] % limit; } };
    };
    const random = sequence([1, 2]);
    assert.equal(chooseFringeFrame(fringe, code, random, { excludedFlags: EXCLUDE_UNLIT }), 2);
    assert.deepEqual(random.calls, [4, 4]);
    // 제외되지 않는 첫 후보는 난수를 한 번만 쓴다.
    const once = sequence([3]);
    assert.equal(chooseFringeFrame(fringe, code, once, { excludedFlags: 0 }), 3);
    assert.deepEqual(once.calls, [4]);
});

test('후보가 없으면 -1(난수를 쓰지 않음), 후보가 모두 제외되면 오류, 후보 없음에 앞 바이트 플래그가 겹쳐도 오류다', () => {
    const fringe = new FrameCodeTable([{ side: 0x41, variant: 0x41, flags: 0x10 }]);
    const noRandom = { next() { throw new Error('난수를 쓰면 안 된다'); } };
    // 방향 'C' 후보는 없다.
    assert.equal(chooseFringeFrame(fringe, { side: 0x43, variant: 0x41, flags: 1 }, noRandom), -1);
    // 후보가 하나뿐인데 제외 플래그와 겹친다(원본은 끝없이 반복).
    assert.throws(() => chooseFringeFrame(fringe, { side: 0x41, variant: 0x41, flags: 1 }, noRandom, { excludedFlags: EXCLUDE_UNLIT }), RangeError);
    // 후보가 없는데 앞 바이트 플래그가 제외 플래그와 겹친다.
    assert.throws(() => chooseFringeFrame(fringe, { side: 0x43, variant: 0x41, flags: 1 }, noRandom, { excludedFlags: EXCLUDE_UNLIT, prefixFlags: 0x10 }), RangeError);
});

test('절벽 생성기: fringe 플래그가 없는 프레임은 난수도 쓰지 않고, 프레임이 표 밖이면 오류, 위치는 y + 4 칸이다', () => {
    const isle = new FrameCodeTable([{ side: 0x41, variant: 0x41, flags: 0 }, { side: 0x41, variant: 0x41, flags: FRINGE_FLAG }]);
    const fringe = new FrameCodeTable([{ side: 0x41, variant: 0x41, flags: 0 }, { side: 0x41, variant: 0x41, flags: 0 }]);
    const stamper = createFringeStamper(isle, fringe);
    const random = { calls: 0, next(limit) { this.calls += 1; return limit - 1; } };
    stamper.onStamp(10, 20, 0, random);
    assert.equal(random.calls, 0);
    assert.equal(stamper.count(), 0);
    stamper.onStamp(10, 20, 1, random);
    assert.equal(random.calls, 1);
    assert.equal(stamper.cells[20 * WORLD_CELLS + 10], 1);
    assert.equal(stamper.count(), 1);
    assert.deepEqual(stamper.positionOf(10, 20), { x: 10, y: 24 });
    assert.throws(() => stamper.onStamp(10, 20, 2, random), RangeError);
});

test('원본 fringe.type: 글자 쌍 256가지의 절벽 후보(검색 단계·제외 전/후)가 독립 기대값과 줄 단위로 같다', { skip: expectedRows.length === 0 }, () => {
    let pairs = 0;
    // pair 줄: 쌍 / 단계 / 후보 / 0x10 제외 후 / 0x18 제외 후.
    for (const row of expectedRows.filter((candidate) => candidate[0] === 'pair')) {
        const [, pair, stage, candidates, allowedBattle, allowedNone] = row;
        const code = { side: pair.charCodeAt(0), variant: pair.charCodeAt(1) };
        const actual = fringeCandidates(fringeFrames, code, EXCLUDE_UNLIT);
        const list = (text) => (text === '-' ? [] : text.split(',').map(Number));
        assert.equal(actual.stage, stage, `단계 ${pair}`);
        assert.deepEqual(actual.candidates, list(candidates), `후보 ${pair}`);
        assert.deepEqual(actual.allowed, list(allowedBattle), `전투 허용 ${pair}`);
        assert.deepEqual(fringeCandidates(fringeFrames, code, EXCLUDE_LIT_AND_UNLIT).allowed, list(allowedNone), `전체 제외 허용 ${pair}`);
        pairs += 1;
    }
    assert.equal(pairs, 256);
});

test('원본 isle.type: fringe 플래그가 있는 클러스터 목록과 각 클러스터의 절벽 후보가 독립 기대값과 같다', { skip: expectedRows.length === 0 }, () => {
    const rows = expectedRows.filter((candidate) => candidate[0] === 'isle');
    const actual = isleFrames.codes.map((code, index) => [index, code]).filter(([, code]) => (code.flags & FRINGE_FLAG) !== 0);
    assert.equal(rows.length, actual.length);
    assert.equal(rows.length, 56);
    // 줄마다 클러스터 번호와 글자 쌍, 허용 후보 수가 같다.
    rows.forEach((row, i) => {
        const [index, code] = actual[i];
        assert.equal(Number(row[1]), index);
        assert.equal(row[2], String.fromCharCode(code.side, code.variant));
        assert.equal(fringeCandidates(fringeFrames, code, EXCLUDE_UNLIT).allowed.length, Number(row[3]));
        // 두 제외 플래그 모두에서 허용 후보가 비지 않는다(비면 원본은 끝없이 반복한다).
        assert.ok(fringeCandidates(fringeFrames, code, EXCLUDE_LIT_AND_UNLIT).allowed.length > 0, `허용 후보 없음 ${row[2]}`);
    });
});

// 원본 `.fort` 465개의 { source, mask } 이다(처음 부를 때 한 번만 만들고 재사용한다). 저장소 자산이 없으면 null 이다. (2026-11-02 WB03-Y)
let maskCache;
function allMasks() {
    if (maskCache === undefined) {
        const forts = loadOriginalForts(archive);
        maskCache = forts ? forts.map(({ source, bytes }) => {
            const layout = layoutFort(parseFort(bytes, types), pieceCodes);
            return { source, mask: buildIslandMask(layout.map, layout.islands) };
        }) : null;
    }
    return maskCache;
}

test('원본 .fort 465개: 절벽은 fringe 플래그가 있는 지면 칸에만 있고 독립 기대 후보 안에서만 고른다', { skip: expectedRows.length === 0 }, () => {
    const masks = allMasks();
    if (!masks) return;
    // 독립 기대값의 pair 줄을 글자 쌍 → 허용 후보 집합으로 둔다.
    const allowed = new Map(expectedRows.filter((row) => row[0] === 'pair').map((row) => [row[1], new Set(row[4] === '-' ? [] : row[4].split(',').map(Number))]));
    let maps = 0;
    let cellsWithFringe = 0;
    let groundCells = 0;
    // 원본 맵마다 첫 도장과 절벽을 만든다.
    for (const { mask } of masks) {
        const stamper = createFringeStamper(isleFrames, fringeFrames);
        const frames = buildTerrainFrames(mask, isleFrames, { onStamp: stamper.onStamp });
        for (let index = 0; index < frames.length; index += 1) {
            if (frames[index] === -1) {
                assert.equal(stamper.cells[index], -1);
                continue;
            }
            groundCells += 1;
            const code = isleFrames.codes[frames[index]];
            const fringe = stamper.cells[index];
            if ((code.flags & FRINGE_FLAG) === 0) {
                assert.equal(fringe, -1);
                continue;
            }
            const set = allowed.get(String.fromCharCode(code.side, code.variant));
            if (set.size === 0) assert.equal(fringe, -1);
            else assert.ok(set.has(fringe), `절벽 ${fringe} 가 ${String.fromCharCode(code.side, code.variant)} 의 허용 후보 밖`);
            if (fringe !== -1) cellsWithFringe += 1;
        }
        maps += 1;
    }
    assert.equal(maps, 465);
    assert.ok(groundCells > 1_900_000);
    assert.ok(cellsWithFringe > 100_000, `절벽이 있는 칸 ${cellsWithFringe}`);
});

test('절벽 생성은 같은 난수 상태에서 항상 같다(반복성)', () => {
    const land = maskOf(Array.from({ length: 12 }, (_, i) => [60 + (i % 4), 70 + Math.trunc(i / 4), 3]));
    const run = () => {
        const stamper = createFringeStamper(isleFrames, fringeFrames);
        const frames = buildTerrainFrames(land, isleFrames, { onStamp: stamper.onStamp });
        return [Array.from(frames), Array.from(stamper.cells)];
    };
    assert.deepEqual(run(), run());
});
