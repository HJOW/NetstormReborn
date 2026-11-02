// 지면 타일 선택(terrain-tiles.js)을 검사한다: 이웃 비트·방향 글자·정규화, 프레임 코드 표의 글자 구간·후보 검색 5단계, 원소별 후보 범위, CRT/게임 난수 시드(BigInt 독립 계산),
// 안쪽 3 × 3 그림 프레임(첫 도장·지역 도장), 원본 `.fort` 465개 전체의 방향 글자 분포, 글자 쌍 256가지의 후보·원소 범위와 3 × 3 그림 월드 전체 해시를
// 원본 isle.type 텍스트와 마스크 재현(Python)에서 따로 계산한 기대값 표(tests/fixtures/terrain-tiles-expected.tsv)와 대조하고,
// buildTerrainFrames 가 그 기대값 안에서만 고르는지(난수로 고르는 변형은 후보 집합까지만 검증)를 확인한다.
// 기대값 표는 webpj/tools/make-terrain-tiles-expected.py 가 만들었고 www/ 구현의 출력으로 다시 만들지 않는다.
// (2026-11-02 WB03-X: 정적 맵 화면(M2) 3단계(b) 지면 타일 선택 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import crypto from 'node:crypto';
import fs from 'node:fs';
import path from 'node:path';
import { WEBPJ_ROOT, loadOriginalForts, readData } from './helpers.mjs';
import { TaffArchive } from '../www/assets/taff.js';
import { decodeOriginalText } from '../www/assets/text-encoding.js';
import { TypeTable } from '../www/assets/type-table.js';
import { createFortTypes } from '../www/assets/type-flags.js';
import { parseFort } from '../www/assets/fort.js';
import { layoutFort, pieceFrameCodes } from '../www/assets/territory.js';
import { EMPTY_TERRAIN, LocalRandom, NEIGHBOR_CELLS, WORLD_CELLS, buildIslandMask } from '../www/assets/terrain-mask.js';
import {
    CORE_SIZE, CORE_TABLE_SIZE, CORE_THEME_FRAMES, CrtRandom, FIXED_CRT_SEED, FrameCodeTable, MAX_CANDIDATES, THEME_COUNT, THEME_NAMES,
    buildTerrainFrames, chooseTerrainFrame, classifyCells, createPaintVariants, createStampVariants, neighborLetters, normalizeOrientation,
    paintCoreFrame, seedGameRandoms, stampCoreFrame, terrainFrameCandidates, themeRange,
} from '../www/assets/terrain-tiles.js';

// 공용 자료이다: 아카이브·타입 표·`.fort` 판독용 타입 정보·영역 패턴 타입의 프레임 코드·`isle` 타입의 프레임 코드 표. (2026-11-02 WB03-X)
const archive = new TaffArchive(readData('netstorm.tarc'));
const table = TypeTable.fromArchive(archive, decodeOriginalText);
const types = createFortTypes(table);
const pieceCodes = pieceFrameCodes(table);
const isleDefinition = table.definitions[table.find('isle') - 70];
const frames = FrameCodeTable.fromDefinition(isleDefinition);

// 기대값 표(Python)이다: pair(글자 쌍) / core(3 × 3 그림 월드 해시) / map(파일별 분포) 줄을 첫 칸으로 나눈다. (2026-11-02 WB03-X)
const expectedRows = fs.readFileSync(path.join(WEBPJ_ROOT, 'tests', 'fixtures', 'terrain-tiles-expected.tsv'), 'utf8')
    .split(/\r?\n/).filter((line) => line !== '' && !line.startsWith('#')).map((line) => line.split('\t'));

// 글자 쌍 → { stage, candidates[], sets[4](`!` 범위 없음, `-` 해당 없음, 아니면 번호 목록), blocks(same|differs|-: 그림 파일 이름 블록과 같은가) } 이다. (2026-11-02 WB03-X)
const pairRows = new Map(expectedRows.filter((row) => row[0] === 'pair').map((row) => [row[1], {
    stage: row[2],
    candidates: row[3] === '-' ? [] : row[3].split(',').map(Number),
    sets: row.slice(4, 8).map((text) => (text === '!' || text === '-' ? text : text === '' ? [] : text.split(',').map(Number))),
    blocks: row[8],
}]));

// 3 × 3 그림 방식 → 월드 전체 프레임 번호 해시이다. (2026-11-02 WB03-X)
const coreRows = new Map(expectedRows.filter((row) => row[0] === 'core').map((row) => [row[1], row[2]]));

// 파일별 분포 줄(map)이다. (2026-11-02 WB03-X)
const mapRows = expectedRows.filter((row) => row[0] === 'map');

// SHA-256 16진수 글을 돌려준다. (2026-11-02 WB03-X)
function sha256(bytes) {
    return crypto.createHash('sha256').update(bytes).digest('hex');
}

// 빈 마스크를 만들고, 영역 번호 owner 의 칸을 [x, y] 목록으로 채운다. (2026-11-02 WB03-X)
function drawn(owner, cells) {
    const land = new Uint8Array(WORLD_CELLS * WORLD_CELLS).fill(EMPTY_TERRAIN);
    // 영역 칸을 하나씩 채운다.
    for (const [x, y] of cells) land[y * WORLD_CELLS + x] = owner;
    return land;
}

// 사각형(왼쪽·위쪽 포함, 오른쪽·아래쪽 제외) 칸의 [x, y] 목록이다. (2026-11-02 WB03-X)
function rectangle(left, top, right, bottom) {
    const cells = [];
    // 사각형의 행과 열을 훑어 칸 목록을 만든다.
    for (let y = top; y < bottom; y += 1) for (let x = left; x < right; x += 1) cells.push([x, y]);
    return cells;
}

// 원본 `.fort` 465개의 { source, mask }이다(처음 부를 때 한 번만 만들고 이후 재사용한다). 저장소 자산이 없으면 null 이다. (2026-11-02 WB03-X)
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

// 원소 구간이 있는 글자 쌍의 후보와 원소 범위를 구현에서 같은 모양의 문자열로 만든다(기대값 표의 칸과 같은 모양). (2026-11-02 WB03-X)
function actualSets(pair) {
    const { candidates } = terrainFrameCandidates(frames, pair[0], pair[1]);
    if (pair === 'AA') return ['-', '-', '-', '-'];
    const out = [];
    // 원소마다 후보 범위를 만들고, 범위가 비면 `!` 이다.
    for (let theme = 0; theme < THEME_COUNT; theme += 1) {
        try {
            const { low, high } = themeRange(candidates.length, theme);
            out.push(candidates.slice(low, high));
        } catch {
            out.push('!');
        }
    }
    return out;
}

// ----- 이웃 비트와 방향 글자 -----

// 이웃 하나만 같은 영역일 때 켜지는 비트: 북 → 직선 1, 북동 → 대각선 2, 동 → 직선 2, 남동 → 대각선 4, 남 → 직선 4, 남서 → 대각선 8, 서 → 직선 8, 북서 → 대각선 1(원본 RawTerrainStamp 의 `1,2,2,4,4,8,8,1`).
// 글자는 비트 → 글자 표 `PNOILJFBMHKEGDCA` 이다. 다른 영역·허공·월드 밖 이웃은 켜지지 않는다. (2026-11-02 WB03-X)
test('neighborLetters: 이웃 여덟 칸의 비트와 글자', () => {
    const expected = [
        { kind: 'straight', bit: 1 }, { kind: 'diagonal', bit: 2 }, { kind: 'straight', bit: 2 }, { kind: 'diagonal', bit: 4 },
        { kind: 'straight', bit: 4 }, { kind: 'diagonal', bit: 8 }, { kind: 'straight', bit: 8 }, { kind: 'diagonal', bit: 1 },
    ];
    const letters = 'PNOILJFBMHKEGDCA';
    // 이웃 하나씩 켜 본다.
    NEIGHBOR_CELLS.forEach(([dx, dy], number) => {
        const land = drawn(4, [[100, 100], [100 + dx, 100 + dy]]);
        const result = neighborLetters(land, 100, 100);
        const straight = expected[number].kind === 'straight' ? expected[number].bit : 0;
        const diagonal = expected[number].kind === 'diagonal' ? expected[number].bit : 0;
        assert.deepEqual(result, { straight, diagonal, side: letters[straight], variant: letters[diagonal] }, `이웃 ${number}`);
    });
    // 이웃이 없으면 (P, P), 여덟 이웃이 모두 같은 영역이면 (A, A) 이다.
    assert.deepEqual(neighborLetters(drawn(4, [[100, 100]]), 100, 100), { straight: 0, diagonal: 0, side: 'P', variant: 'P' });
    assert.deepEqual(neighborLetters(drawn(4, rectangle(99, 99, 102, 102)), 100, 100), { straight: 15, diagonal: 15, side: 'A', variant: 'A' });
    // 다른 영역 이웃은 켜지지 않는다.
    const mixed = drawn(4, rectangle(99, 99, 102, 102));
    mixed[99 * WORLD_CELLS + 100] = 5;
    mixed[101 * WORLD_CELLS + 101] = 5;
    // 북(직선 1)과 남동(대각선 4)이 꺼진다: 직선 14 → 'C', 대각선 11 → 'E'.
    assert.deepEqual(neighborLetters(mixed, 100, 100), { straight: 14, diagonal: 11, side: 'C', variant: 'E' });
    // 지면이 아닌 칸은 null 이다.
    assert.equal(neighborLetters(drawn(4, [[1, 1]]), 5, 5), null);
});

// 월드 가장자리: 월드 밖은 허공이라 이웃으로 세지 않는다. 왼쪽 위 모서리 칸 (0,0)과 그 동·남·남동 이웃이면 (동 2 + 남 4 = 6 → 'F', 남동 4 → 'L'). (2026-11-02 WB03-X)
test('neighborLetters: 월드 밖은 이웃이 아니다', () => {
    const land = drawn(7, [[0, 0], [1, 0], [0, 1], [1, 1]]);
    assert.deepEqual(neighborLetters(land, 0, 0), { straight: 6, diagonal: 4, side: 'F', variant: 'L' });
    const far = drawn(7, [[255, 255], [254, 255], [255, 254], [254, 254]]);
    // 오른쪽 아래 모서리: 서 8 + 북 1 = 9 → 'H', 북서 1 → 'N'.
    assert.deepEqual(neighborLetters(far, 255, 255), { straight: 9, diagonal: 1, side: 'H', variant: 'N' });
});

// 월드의 오른쪽 끝(x = 255) 칸의 동쪽 이웃은 월드 밖이다: 한 줄 아래의 왼쪽 끝 칸(배열 인덱스로는 바로 다음 칸)을 이웃으로 세지 않는다. 왼쪽 끝(x = 0) 칸의 서쪽도 윗줄의 오른쪽 끝 칸이 아니다. (2026-11-02 WB03-X)
test('neighborLetters: 월드의 왼쪽·오른쪽 끝은 이어 붙지 않는다', () => {
    const right = drawn(6, [[255, 100], [0, 101], [0, 100], [0, 102]]);
    assert.deepEqual(neighborLetters(right, 255, 100), { straight: 0, diagonal: 0, side: 'P', variant: 'P' });
    const left = drawn(6, [[0, 100], [255, 99], [255, 100], [255, 98]]);
    assert.deepEqual(neighborLetters(left, 0, 100), { straight: 0, diagonal: 0, side: 'P', variant: 'P' });
});

// 방향 정규화(원본 0041cd20): 양쪽 직선 이웃이 받쳐 주는 대각선만 남기고 직선 연결을 다시 만든다. 손으로 계산한 예 — (B, C): 직선 북·동·남, 대각선 북동·남동·남서 → 남서는 남·서가 필요해 빠지고 (B, F) 가 된다.
// 대각선만 있고 직선이 없으면 모두 사라져 (P, P), (A, A) 는 그대로이다. 범위 밖 글자는 오류이다. (2026-11-02 WB03-X)
test('normalizeOrientation: 받쳐 주는 대각선만 남는다', () => {
    assert.deepEqual(normalizeOrientation('B', 'C'), { side: 'B', variant: 'F' });
    assert.deepEqual(normalizeOrientation('A', 'A'), { side: 'A', variant: 'A' });
    // 북·동 직선과 북동 대각선: 그대로 (I, O).
    assert.deepEqual(normalizeOrientation('I', 'O'), { side: 'I', variant: 'O' });
    // 동·서 직선(K)에는 북동 대각선을 받쳐 주는 직선이 없어 모두 사라진다.
    assert.deepEqual(normalizeOrientation('K', 'O'), { side: 'P', variant: 'P' });
    assert.deepEqual(normalizeOrientation('P', 'A'), { side: 'P', variant: 'P' });
    // 범위 밖 글자는 모두 오류이다.
    for (const bad of ['@', 'Q', '', 'AB', 1]) assert.throws(() => normalizeOrientation(bad, 'A'), /Invalid terrain side/);
    // 정규화한 글자는 다시 정규화해도 같다(고정점).
    for (const side of 'ABCDEFGHIJKLMNOP') {
        // 변형 글자 16가지도 훑는다.
        for (const variant of 'ABCDEFGHIJKLMNOP') {
            const once = normalizeOrientation(side, variant);
            assert.deepEqual(normalizeOrientation(once.side, once.variant), once, `${side}${variant}`);
        }
    }
});

// classifyCells: 지면 칸마다 방향 글자의 문자 코드가 들어가고 허공은 0 이다. neighborLetters 와 같다. (2026-11-02 WB03-X)
test('classifyCells: 지면 칸의 방향 글자', () => {
    const land = drawn(2, rectangle(40, 40, 44, 43));
    const { sides, variants } = classifyCells(land);
    assert.equal(sides.length, WORLD_CELLS * WORLD_CELLS);
    let count = 0;
    // 모든 칸을 훑는다.
    for (let i = 0; i < land.length; i += 1) {
        if (land[i] === EMPTY_TERRAIN) {
            assert.equal(sides[i] + variants[i], 0);
            continue;
        }
        count += 1;
        const letters = neighborLetters(land, i % WORLD_CELLS, Math.trunc(i / WORLD_CELLS));
        assert.equal(String.fromCharCode(sides[i]), letters.side);
        assert.equal(String.fromCharCode(variants[i]), letters.variant);
    }
    assert.equal(count, 12);
    // 4 × 3 칸에서 안쪽 (A, A) 는 가운데 2 × 1칸뿐이다.
    assert.equal([...sides].filter((value, i) => value === 0x41 && variants[i] === 0x41).length, 2);
});

// ----- 프레임 코드 표 -----

// 표의 프레임 코드를 [{ side, variant, flags }] 로 간단히 만든다: 이름 'AB' 처럼 두 글자를 주면 된다. (2026-11-02 WB03-X)
function makeTable(names, flags = 0) {
    return new FrameCodeTable(names.map((name) => ({ side: name.charCodeAt(0), variant: name.charCodeAt(1), flags })));
}

// 글자 구간: 방향 글자마다 처음 프레임 번호와 개수. 없는 글자는 first -1, count 0. 'A'~'P' 밖은 오류. 프레임 코드는 복사해 얼린다. (2026-11-02 WB03-X)
test('FrameCodeTable: 글자 구간·복사·길이', () => {
    const codes = [{ side: 0x41, variant: 0x41, flags: 0 }, { side: 0x42, variant: 0x46, flags: 5 }, { side: 0x41, variant: 0x42, flags: 4 }, { side: 0x42, variant: 0x46, flags: 5 }];
    const built = new FrameCodeTable(codes);
    assert.equal(built.length, 4);
    assert.deepEqual(built.run('A'), { first: 0, count: 2 });
    assert.deepEqual(built.run('B'), { first: 1, count: 2 });
    assert.deepEqual(built.run('P'), { first: -1, count: 0 });
    assert.deepEqual(built.run(0x42), { first: 1, count: 2 });
    // 범위 밖 글자는 모두 오류이다.
    for (const bad of ['@', 'Q', '', 0x40, 0x51]) assert.throws(() => built.run(bad), /A~P/);
    // 원본 배열을 바꿔도 표는 변하지 않고, 코드 객체는 얼려 있다.
    codes[0].side = 0x43;
    assert.equal(built.codes[0].side, 0x41);
    assert.throws(() => { built.codes[0].side = 0x43; }, TypeError);
    // 타입 정의의 클러스터 코드에서 만든 표는 isle 타입의 구조를 따른다: 317개, 'J' 구간은 JJ00 하나, 'A' 구간은 200개(AA01~24 · 지면 가장자리 AB~AE 32 · 3 × 3 그림 144).
    assert.equal(frames.length, 317);
    assert.deepEqual(frames.run('J'), { first: 172, count: 1 });
    assert.deepEqual(frames.run('A'), { first: 0, count: 200 });
    assert.equal(isleDefinition.clusters[172].name, 'JJ00');
});

// 후보 검색: 방향 글자(와 변형 글자)가 맞는 프레임을 저장 순서대로 모으고 최대 256개에서 멈춘다. 변형 null 은 방향만 본다. (2026-11-02 WB03-X)
test('FrameCodeTable.candidates: 저장 순서·최대 256개', () => {
    const names = ['BF', 'AB', 'BF', 'BK', 'AB', 'BF'];
    const built = makeTable(names);
    assert.deepEqual(built.candidates('B', 'F'), [0, 2, 5]);
    assert.deepEqual(built.candidates('B', null), [0, 2, 3, 5]);
    assert.deepEqual(built.candidates('A', 'B'), [1, 4]);
    assert.deepEqual(built.candidates('C', 'C'), []);
    // 같은 글자 300개이면 앞의 256개만 모은다.
    const many = makeTable(Array.from({ length: 300 }, () => 'BF'));
    assert.equal(MAX_CANDIDATES, 256);
    assert.deepEqual(many.candidates('B', 'F'), Array.from({ length: 256 }, (_, i) => i));
    assert.throws(() => built.candidates('R', 'F'), /A~P/);
});

// 후보 검색 5단계: 정확히 → 정규화한 글자 → (방향, 'A') → 방향만 → 없음. 정규화는 (B, C) → (B, F) 이다. 결과는 얼려 있고 같은 표·같은 글자는 같은 객체를 돌려준다(기억). (2026-11-02 WB03-X)
test('terrainFrameCandidates: 다섯 단계', () => {
    assert.deepEqual(terrainFrameCandidates(makeTable(['BC', 'BF']), 'B', 'C'), { stage: 'exact', candidates: [0] });
    assert.deepEqual(terrainFrameCandidates(makeTable(['BA', 'BF', 'BK']), 'B', 'C'), { stage: 'normalized', candidates: [1] });
    assert.deepEqual(terrainFrameCandidates(makeTable(['BK', 'BA']), 'B', 'C'), { stage: 'sideA', candidates: [1] });
    assert.deepEqual(terrainFrameCandidates(makeTable(['BK', 'CC', 'BD']), 'B', 'C'), { stage: 'side', candidates: [0, 2] });
    assert.deepEqual(terrainFrameCandidates(makeTable(['CC', 'AA']), 'B', 'C'), { stage: 'none', candidates: [] });
    // (B, C) 는 (B, F) 로 정규화되어 'BF' 만 찾고(BK 는 방향만 같아 뒤 단계의 몫이다), 같은 표의 같은 글자 쌍은 처음 찾은 결과를 다시 돌려준다.
    const built = makeTable(['BF', 'BK']);
    const first = terrainFrameCandidates(built, 'B', 'C');
    assert.equal(first.stage, 'normalized');
    assert.equal(terrainFrameCandidates(built, 'B', 'C'), first, '같은 표의 같은 글자는 기억한 결과');
    assert.ok(Object.isFrozen(first) && Object.isFrozen(first.candidates));
    // 다른 표는 서로의 기억을 쓰지 않는다.
    assert.deepEqual(terrainFrameCandidates(makeTable(['BF', 'BF']), 'B', 'C').candidates, [0, 1]);
});

// ----- 원소별 후보 범위 -----

// 원소 theme 의 후보 범위는 [q × theme + 1, q × (theme + 1)) 이고 q = 후보 수 ÷ 4(내림)이다. 후보 8개면 원소마다 한 개(3번째·…), 20개면 4개, 24개면 5개이다. 범위가 비는 후보 수(0~7)는 오류이다. (2026-11-02 WB03-X)
test('themeRange: 원소 구간', () => {
    assert.deepEqual([0, 1, 2, 3].map((theme) => themeRange(8, theme)), [{ low: 1, high: 2 }, { low: 3, high: 4 }, { low: 5, high: 6 }, { low: 7, high: 8 }]);
    assert.deepEqual([0, 1, 2, 3].map((theme) => themeRange(20, theme)), [{ low: 1, high: 5 }, { low: 6, high: 10 }, { low: 11, high: 15 }, { low: 16, high: 20 }]);
    assert.deepEqual(themeRange(24, 3), { low: 19, high: 24 });
    assert.deepEqual(themeRange(10, 0), { low: 1, high: 2 }, '4분의 1 이 2 이면 (10 ÷ 4 = 2)');
    // 후보 수 0~7 은 모두 범위 오류이다.
    for (let count = 0; count < 8; count += 1) assert.throws(() => themeRange(count, 0), RangeError, `후보 ${count}개`);
    assert.equal(THEME_COUNT, 4);
    assert.deepEqual(THEME_NAMES, ['sun', 'thunder', 'wind', 'rain']);
});

// 난수로 후보를 고른다: 원소 -1 은 전체에서 next(후보 수)를, 원소 0~3 은 next(high - low)를 한 번 호출해 low + 값을 쓴다. 후보가 없으면 난수를 쓰지 않고 -1 이다.
// 난수가 limit 이상을 돌려주면(잘못된 난수) 오류이다. (2026-11-02 WB03-X)
test('chooseTerrainFrame: 난수 소비와 후보 선택', () => {
    // 24개의 (B, F) 후보 프레임 번호는 10~33 이다.
    const names = ['AA', 'AA', 'AA', 'AA', 'AA', 'AA', 'AA', 'AA', 'AA', 'AA', ...Array.from({ length: 24 }, () => 'BF')];
    const built = makeTable(names);
    const script = (value) => {
        const calls = [];
        return { calls, next(limit) { calls.push(limit); return value; } };
    };
    let random = script(5);
    assert.equal(chooseTerrainFrame(built, 'B', 'F', -1, random), 10 + 5);
    assert.deepEqual(random.calls, [24]);
    random = script(2);
    assert.equal(chooseTerrainFrame(built, 'B', 'F', 2, random), 10 + 13 + 2, '원소 2: 범위 [13, 18)');
    assert.deepEqual(random.calls, [5]);
    random = script(0);
    assert.equal(chooseTerrainFrame(built, 'B', 'F', 0, random), 10 + 1, '원소 0: 첫 프레임은 제외');
    // 후보가 없으면 난수를 쓰지 않는다.
    random = script(0);
    assert.equal(chooseTerrainFrame(makeTable(['CC']), 'B', 'F', 0, random), -1);
    assert.deepEqual(random.calls, []);
    // 후보 수가 8 미만이면 범위 오류, 난수가 한계를 넘으면 후보 범위 오류이다.
    assert.throws(() => chooseTerrainFrame(makeTable(['BF', 'BF']), 'B', 'F', 1, script(0)), /quartile/);
    assert.throws(() => chooseTerrainFrame(built, 'B', 'F', 3, script(99)), /후보 범위/);
    // 원소 3 의 범위 [19, 24) 에서 난수가 5 를 돌려주면 인덱스가 후보 수(24)와 같아져 범위를 벗어난다.
    assert.throws(() => chooseTerrainFrame(built, 'B', 'F', 3, script(5)), /후보 범위/);
    assert.equal(chooseTerrainFrame(built, 'B', 'F', 3, script(4)), 10 + 23, '범위의 마지막 후보');
    // 실제 난수기와 함께: 같은 시드는 같은 선택을 한다.
    const a = chooseTerrainFrame(built, 'B', 'F', 1, new LocalRandom(7));
    assert.equal(a, chooseTerrainFrame(built, 'B', 'F', 1, new LocalRandom(7)));
    assert.ok(a >= 10 + 7 && a < 10 + 12);
});

// ----- 난수 시드 -----

// CRT(MSVC rand): srand(1) 의 잘 알려진 수열 41, 18467, 6334, 26500, 19169, 15724, 11478, 29358, 26962, 24464 이다. 시드는 32비트로 감긴다. (2026-11-02 WB03-X)
test('CrtRandom: MSVC rand 수열', () => {
    const crt = new CrtRandom(1);
    assert.deepEqual(Array.from({ length: 10 }, () => crt.next()), [41, 18467, 6334, 26500, 19169, 15724, 11478, 29358, 26962, 24464]);
    assert.equal(new CrtRandom(-1).state, 0xffffffff);
    // 모든 출력은 15비트이다.
    const wide = new CrtRandom(0xdeadbeef);
    // 출력이 15비트 범위 안인지 1000번 확인한다.
    for (let i = 0; i < 1000; i += 1) {
        const value = wide.next();
        assert.ok(value >= 0 && value <= 0x7fff);
    }
});

// 원본 난수 시드 함수(00455970): 고정 시드 0x38d535 로 CRT 를 시작해 100개를 버리고 세 값의 32비트 곱을 게임 난수의 시작 상태로 쓴다. BigInt 로 독립 계산한다. (2026-11-02 WB03-X)
test('seedGameRandoms: BigInt 독립 계산과 같다', () => {
    const reference = (seed) => {
        let state = BigInt(seed) & 0xffffffffn;
        const next = () => {
            state = (state * 0x343fdn + 0x269ec3n) & 0xffffffffn;
            return (state >> 16n) & 0x7fffn;
        };
        // 시드 직후 100개를 버린다.
        for (let i = 0; i < 100; i += 1) next();
        const a = next();
        const b = next();
        const c = next();
        return { gameSeed: Number((a * b * c) & 0xffffffffn), crtState: Number(state) };
    };
    // 시드마다 BigInt 계산과 대조한다.
    for (const seed of [FIXED_CRT_SEED, 0, 1, 0x7fffffff, 0xffffffff, 1761955200]) {
        const seeded = seedGameRandoms(seed);
        const expected = reference(seed);
        assert.equal(seeded.gameSeed, expected.gameSeed, `시드 ${seed}`);
        assert.equal(seeded.crt.state, expected.crtState, `시드 ${seed}: CRT 상태`);
        assert.equal(seeded.game.state, expected.gameSeed);
    }
    assert.equal(FIXED_CRT_SEED, 0x38d535);
    assert.deepEqual(seedGameRandoms().gameSeed, seedGameRandoms(FIXED_CRT_SEED).gameSeed);
    assert.notEqual(seedGameRandoms(1).gameSeed, seedGameRandoms(2).gameSeed);
    // 게임 난수는 게임 전역 난수와 같은 생성기이다: 첫 값은 상태 × 0x10003 + 3 의 상위 16비트이다.
    const seeded = seedGameRandoms();
    const state = BigInt(seeded.gameSeed) === 0n ? 0x0bad0badn : BigInt(seeded.gameSeed);
    assert.equal(seeded.game.next(1000), Number((((state * 0x10003n + 3n) & 0xffffffffn) >> 16n) % 1000n));
});

// 첫 도장의 변형 표 99개: 시드 직후 103개를 소비한 CRT 에서 99개를 이어 뽑되 앞 항목과 하위 2비트가 같으면 1 올린다(그래서 이웃 항목의 하위 2비트는 항상 다르다). crt 상태를 99번 전진시킨다. (2026-11-02 WB03-X)
test('createStampVariants: CRT 99개 표와 하위 2비트 교정', () => {
    let state = BigInt(FIXED_CRT_SEED);
    const next = () => {
        state = (state * 0x343fdn + 0x269ec3n) & 0xffffffffn;
        return Number((state >> 16n) & 0x7fffn);
    };
    // 시드 직후 103개를 소비한다.
    for (let i = 0; i < 103; i += 1) next();
    const expected = [];
    // 99개를 이어 뽑으며 하위 2비트 교정을 적용한다.
    for (let i = 0; i < 99; i += 1) {
        let value = next();
        if (i > 0 && expected[i - 1] % 4 === value % 4) value += 1;
        expected.push(value);
    }
    const seeded = seedGameRandoms();
    const variants = createStampVariants(seeded.crt);
    assert.equal(CORE_TABLE_SIZE, 99);
    assert.deepEqual(variants, expected);
    assert.equal(seeded.crt.state, Number(state), 'CRT 상태가 99번 더 전진');
    // 이웃 항목의 하위 2비트가 모두 다른지 확인한다.
    for (let i = 1; i < 99; i += 1) assert.notEqual(variants[i] % 4, variants[i - 1] % 4, `항목 ${i}`);
    // 교정이 실제로 일어나는 표가 있어야 검사가 의미 있다: 아무 시드에서나 교정 횟수를 센다.
    let corrected = 0;
    // 시드 20가지에서 교정이 실제로 일어나는지 센다.
    for (let seed = 1; seed <= 20; seed += 1) {
        const crt = new CrtRandom(seed);
        const raw = Array.from({ length: 99 }, () => crt.next());
        const table2 = createStampVariants(new CrtRandom(seed));
        corrected += table2.filter((value, i) => value !== raw[i] && i > 0).length;
    }
    assert.ok(corrected > 0);
});

// 지역 도장의 변형 표 99개: 항목마다 시드 0 의 정수 난수 Next(65536) 를 새로 뽑으므로 v, v + 1 이 번갈아 나온다(앞 항목과 하위 2비트가 같으면 1 올림). v 는 BigInt 로 독립 계산한다. (2026-11-02 WB03-X)
test('createPaintVariants: 시드 0 난수의 v, v + 1 반복', () => {
    const v = Number((((0x0bad0badn * 0x10003n + 3n) & 0xffffffffn) >> 16n) % 65536n);
    const variants = createPaintVariants();
    assert.equal(variants.length, 99);
    variants.forEach((value, i) => assert.equal(value, i % 2 === 0 ? v : v + 1, `항목 ${i}`));
    assert.deepEqual(createPaintVariants(), variants, '항상 같다');
});

// ----- 안쪽 3 × 3 그림 -----

// 합성 표(JJ00 가 6번째, 그 뒤 144개)로 3 × 3 그림 프레임의 구조를 확인한다. 같은 3 × 3 묶음(x ÷ 3, y ÷ 3)의 아홉 칸은 한 변형을 공유하고 묶음 안 위치(y % 3) × 3 + x % 3 만큼 다르다.
// 지역 도장은 원소만큼 36씩 건너뛰고, 원소 -1 은 태양이다. 같은 표를 쓰면 첫 도장과 태양 지역 도장은 같다. 'J' 구간이 없는 표는 오류이다. (2026-11-02 WB03-X)
test('stampCoreFrame·paintCoreFrame: 3 × 3 묶음의 구조', () => {
    const names = [...Array.from({ length: 5 }, () => 'AB'), 'JJ', ...Array.from({ length: 144 }, () => 'AA')];
    const built = makeTable(names);
    const start = 6;
    const variants = Array.from({ length: 99 }, (_, i) => (i * 37 + 11) % 1000);
    // 한 변형이 한 묶음 안에서 같은지 본다.
    for (const [bx, by] of [[0, 0], [5, 7], [31, 40], [98, 99], [100, 100]]) {
        const seen = new Set();
        // 묶음의 아홉 칸(행 먼저)을 확인한다.
        for (let dy = 0; dy < 3; dy += 1) {
            // 같은 행의 세 칸을 확인한다.
            for (let dx = 0; dx < 3; dx += 1) {
                const x = bx * 3 + dx;
                const y = by * 3 + dy;
                const frame = stampCoreFrame(built, variants, x, y);
                const offset = frame - start;
                assert.equal(offset % 9, dy * 3 + dx, `묶음 ${bx},${by} 칸 ${dx},${dy}`);
                seen.add(Math.trunc(offset / 9));
                assert.equal(paintCoreFrame(built, variants, 0, x, y), frame, '같은 표의 태양 지역 도장 = 첫 도장');
                assert.equal(paintCoreFrame(built, variants, -1, x, y), frame, '원소 -1 은 태양');
                // 원소 1~3 은 36씩 건너뛴다.
                for (let theme = 1; theme < 4; theme += 1) assert.equal(paintCoreFrame(built, variants, theme, x, y), frame + theme * CORE_THEME_FRAMES);
            }
        }
        assert.equal(seen.size, 1, `묶음 ${bx},${by} 의 변형`);
        assert.ok([...seen][0] >= 0 && [...seen][0] < 4);
    }
    // 변형이 실제로 네 가지 모두 나온다(표가 단조롭지 않다).
    const used = new Set();
    // 묶음 1600개에서 쓰인 변형을 모은다.
    for (let by = 0; by < 40; by += 1) for (let bx = 0; bx < 40; bx += 1) used.add(Math.trunc((stampCoreFrame(built, variants, bx * 3, by * 3) - start) / 9));
    assert.equal(used.size, 4);
    assert.equal(CORE_SIZE, 3);
    assert.equal(CORE_THEME_FRAMES, 36);
    assert.throws(() => stampCoreFrame(makeTable(['AA']), variants, 1, 1), /JJ00/);
    assert.throws(() => paintCoreFrame(makeTable(['AA']), variants, 0, 1, 1), /JJ00/);
});

// 실제 isle 타입: 3 × 3 그림 144개는 모두 AA00 이고 JJ00 다음에 원소 순서(해·천둥·바람·비)로 36개씩 이어진다. 월드 전체(256 × 256)의 프레임 번호 해시가 독립 계산(Python: 실제 CRT·isle.type 텍스트)과 같다. (2026-11-02 WB03-X)
test('실제 isle: 3 × 3 그림 월드 전체가 기대값 표와 같다', () => {
    const start = frames.run('J').first + 1;
    // 3 × 3 그림 144개가 모두 AA00 인지 확인한다.
    for (let i = start; i < start + 4 * CORE_THEME_FRAMES; i += 1) assert.equal(isleDefinition.clusters[i].name, 'AA00', `프레임 ${i}`);
    const hashOf = (pick) => {
        const grid = Buffer.alloc(WORLD_CELLS * WORLD_CELLS * 4);
        // 월드 전체를 y 바깥·x 안쪽으로 채운다.
        for (let y = 0; y < WORLD_CELLS; y += 1) for (let x = 0; x < WORLD_CELLS; x += 1) grid.writeInt32LE(pick(x, y), (y * WORLD_CELLS + x) * 4);
        return sha256(grid);
    };
    const stamp = createStampVariants(seedGameRandoms().crt);
    assert.equal(hashOf((x, y) => stampCoreFrame(frames, stamp, x, y)), coreRows.get('stamp'), '첫 도장');
    const paint = createPaintVariants();
    // 원소마다 월드 전체 해시를 확인한다.
    for (let theme = 0; theme < 4; theme += 1) assert.equal(hashOf((x, y) => paintCoreFrame(frames, paint, theme, x, y)), coreRows.get(`paint${theme}`), `지역 도장 원소 ${theme}`);
    assert.equal(coreRows.size, 5);
    // 첫 도장은 태양 묶음만 쓴다.
    for (let y = 0; y < WORLD_CELLS; y += 7) for (let x = 0; x < WORLD_CELLS; x += 5) assert.ok(stampCoreFrame(frames, stamp, x, y) < start + CORE_THEME_FRAMES);
});

// ----- 글자 쌍 256가지 -----

// 방향·변형 글자 쌍 256가지의 후보를 찾은 단계·후보 번호·원소별 유효 후보가 독립 기대값(isle.type 텍스트를 직접 읽은 Python 이 원본 산술로 계산한 것)과 같다.
// 안쪽 (A, A) 쌍의 원소 구간은 해당 없음(`-`)이고, 후보가 4분할되지 않는 쌍(후보 수 7 이하)은 `!` 이다.
// 기대값 표는 그림 파일 이름으로 나눈 원소 블록(첫 클러스터 제외)과 4분의 1 구간이 같은지도 적었다 — 같지 않은 19쌍은 JJ00 하나뿐인 'J' 묶음이거나 AA 묶음(AA01~24 와 3 × 3 그림 144개가 섞임)으로 빠지는 쌍이다. (2026-11-02 WB03-X)
test('글자 쌍 256가지: 후보 단계·후보 번호·원소별 유효 후보', () => {
    assert.equal(pairRows.size, 256);
    const stages = {};
    const blockFlags = {};
    // 방향 글자 16가지를 훑는다.
    for (const side of 'ABCDEFGHIJKLMNOP') {
        // 변형 글자 16가지를 훑는다.
        for (const variant of 'ABCDEFGHIJKLMNOP') {
            const pair = side + variant;
            const expected = pairRows.get(pair);
            const actual = terrainFrameCandidates(frames, side, variant);
            assert.equal(actual.stage, expected.stage, `${pair}: 단계`);
            assert.deepEqual([...actual.candidates], expected.candidates, `${pair}: 후보`);
            assert.deepEqual(actualSets(pair), expected.sets, `${pair}: 원소 구간`);
            stages[actual.stage] = (stages[actual.stage] ?? 0) + 1;
            blockFlags[expected.blocks] = (blockFlags[expected.blocks] ?? 0) + 1;
        }
    }
    // 단계별 쌍 수: 정확히 14(원본 클러스터 묶음), 정규화 80, (방향, A) 3, 방향만 63, 없음 96.
    assert.deepEqual(stages, { exact: 14, normalized: 80, sideA: 3, side: 63, none: 96 });
    // 그림 파일 이름 블록과 같은 쌍 140, 다른 쌍 19, 해당 없음 97(안쪽 AA 한 쌍과 후보 없음 96쌍).
    assert.deepEqual(blockFlags, { '-': 97, same: 140, differs: 19 });
    assert.deepEqual([...pairRows].filter(([, row]) => row.blocks === 'differs').map(([pair]) => pair).sort(),
        ['AJ', 'AK', 'AP', 'JA', 'JB', 'JC', 'JD', 'JE', 'JF', 'JG', 'JH', 'JI', 'JJ', 'JK', 'JL', 'JM', 'JN', 'JO', 'JP']);
});

// 원본 `.fort` 465개에 실제로 나오는 글자 쌍은 모두 후보를 정확히/정규화로 찾고(방향만·없음 단계로 빠지는 칸이 없다), 원소 구간이 그림 파일 이름의 원소 블록과 같다.
// 곧 "후보 수의 4분의 1 구간 = 원소"라는 원본 산술이 이 맵들에서는 그림의 원소와 정확히 맞는다. 'J' 방향 글자(JJ00 하나뿐)와 AA 묶음으로 빠지는 쌍은 맵에 나오지 않는다. (2026-11-02 WB03-X)
test('원본 .fort 465개에 나오는 글자 쌍은 원소 블록과 맞는다', () => {
    const used = new Map();
    // 파일마다 글자 쌍별 칸 수를 합친다.
    for (const row of mapRows) {
        if (row[6] === '-') continue;
        // 쌍마다 칸 수를 더한다.
        for (const entry of row[6].split(',')) {
            const [pair, count] = entry.split(':');
            used.set(pair, (used.get(pair) ?? 0) + Number(count));
        }
    }
    assert.equal(used.size, 51);
    assert.equal([...used.values()].reduce((sum, count) => sum + count, 0), 1909275);
    // 맵에 나오는 모든 쌍을 확인한다.
    for (const pair of used.keys()) {
        const row = pairRows.get(pair);
        assert.ok(row.stage === 'exact' || row.stage === 'normalized', `${pair}: 단계 ${row.stage}`);
        assert.equal(row.blocks, pair === 'AA' ? '-' : 'same', `${pair}: 원소 블록`);
    }
    assert.ok([...used.keys()].every((pair) => pair[0] !== 'J'));
});

// ----- 원본 .fort 465개 -----

// 원본 `.fort` 465개의 방향 글자 분포가 독립 기대값과 같다: 마스크 SHA-256, 지면 칸 수, 안쪽(여덟 이웃이 모두 같은 영역)·가장자리 칸 수, 글자 쌍별 칸 수, (x, y, 방향 글자, 변형 글자) 열의 SHA-256.
// 안쪽 칸은 (A, A) 인 칸과 같다. 지면 칸이 모두 [16, 240) 안이다(첫 도장의 범위). (2026-11-02 WB03-X)
test('원본 .fort 465개: 방향 글자 분포가 기대값 표와 같다', (context) => {
    const masks = allMasks();
    if (!masks) {
        context.skip('저장소 자산(assets/game-data)이 없다');
        return;
    }
    assert.equal(masks.length, 465);
    assert.equal(mapRows.length, masks.length);
    const mismatches = [];
    let cellsTotal = 0;
    masks.forEach(({ source, mask }, index) => {
        const { sides, variants } = classifyCells(mask);
        const histogram = {};
        const entries = [];
        let interior = 0;
        let edge = 0;
        // 월드를 y 바깥·x 안쪽으로 훑는다.
        for (let i = 0; i < mask.length; i += 1) {
            if (mask[i] === EMPTY_TERRAIN) continue;
            const x = i % WORLD_CELLS;
            const y = Math.trunc(i / WORLD_CELLS);
            assert.ok(x >= 16 && y >= 16 && x < 240 && y < 240, `${source}: 첫 도장 범위 밖 칸 ${x},${y}`);
            const key = String.fromCharCode(sides[i], variants[i]);
            histogram[key] = (histogram[key] ?? 0) + 1;
            if (key === 'AA') interior += 1;
            else edge += 1;
            entries.push(x, y, sides[i], variants[i]);
        }
        const total = interior + edge;
        cellsTotal += total;
        const row = ['map', source, sha256(mask), String(total), String(interior), String(edge),
            Object.keys(histogram).sort().map((key) => `${key}:${histogram[key]}`).join(',') || '-', sha256(Uint8Array.from(entries))];
        if (row.join('\t') !== mapRows[index].join('\t')) mismatches.push(`줄 ${index + 1}:\n실제 ${row.join('\t')}\n기대 ${mapRows[index].join('\t')}`);
    });
    assert.deepEqual(mismatches.slice(0, 3), [], `불일치 ${mismatches.length}개`);
    assert.equal(cellsTotal, 1909275, '지면 칸 합계(마스크 단위와 같다)');
});

// ----- buildTerrainFrames -----

// 첫 도장만 한 결과(원소 없음)가 기대값 안에 있다: 지면이 아닌 칸은 -1, 안쪽 칸은 첫 도장의 3 × 3 그림 프레임, 가장자리 칸은 글자 쌍의 원소 0 유효 후보 중 하나이다(난수로 고르는 변형은 후보 집합까지만 검증한다).
// 원본 465개 모두에서 확인하고 같은 입력은 같은 결과(반복성)이다. (2026-11-02 WB03-X)
test('buildTerrainFrames: 첫 도장은 안쪽 3 × 3 그림과 원소 0 후보 안에서만 고른다', (context) => {
    const masks = allMasks();
    if (!masks) {
        context.skip('저장소 자산(assets/game-data)이 없다');
        return;
    }
    const stamp = createStampVariants(seedGameRandoms().crt);
    const coreGrid = new Int32Array(WORLD_CELLS * WORLD_CELLS);
    // 월드 전체의 첫 도장 3 × 3 프레임을 채운다.
    for (let y = 0; y < WORLD_CELLS; y += 1) for (let x = 0; x < WORLD_CELLS; x += 1) coreGrid[y * WORLD_CELLS + x] = stampCoreFrame(frames, stamp, x, y);
    // 글자 쌍마다 첫 도장이 쓸 수 있는 프레임 집합(원소 0 유효 후보)이다.
    const allowed = new Map([...pairRows].filter(([pair]) => pair !== 'AA').map(([pair, row]) => [pair, new Set(Array.isArray(row.sets[0]) ? row.sets[0] : [])]));
    const used = new Set();
    let first = null;
    masks.forEach(({ source, mask }, index) => {
        const result = buildTerrainFrames(mask, frames);
        if (index === 0) first = result;
        const { sides, variants } = classifyCells(mask);
        // 모든 칸을 확인한다.
        for (let i = 0; i < mask.length; i += 1) {
            if (mask[i] === EMPTY_TERRAIN) {
                assert.equal(result[i], -1, `${source}: 허공 ${i}`);
                continue;
            }
            const pair = String.fromCharCode(sides[i], variants[i]);
            if (pair === 'AA') {
                assert.equal(result[i], coreGrid[i], `${source}: 안쪽 ${i}`);
            } else {
                assert.ok(allowed.get(pair).has(result[i]), `${source}: ${pair} 칸 ${i} 의 프레임 ${result[i]}`);
                used.add(result[i]);
            }
        }
    });
    assert.deepEqual(buildTerrainFrames(masks[0].mask, frames), first, '같은 입력은 같은 결과');
    // 가장자리 변형이 실제로 여럿 쓰였다(난수가 한 가지로 고정되지 않았다).
    assert.ok(used.size > 20, `쓰인 가장자리 프레임 ${used.size}`);
});

// 지역 도장: themeOf 가 원소를 돌려주는 영역은 다시 고른다. 첫 도장이 고른 프레임의 코드가 안쪽(A, A) 이면 그 원소의 3 × 3 그림, 아니면 같은 글자 쌍의 그 원소 유효 후보이다(글자 쌍은 첫 도장 프레임과 같다).
// 원본 465개 중 일부(29개마다 + 공식 맵 둘)에서 원소 0~3 을 모두 확인한다. (2026-11-02 WB03-X)
test('buildTerrainFrames: 지역 도장은 원소별 3 × 3 그림과 원소 후보 안에서만 고른다', (context) => {
    const masks = allMasks();
    if (!masks) {
        context.skip('저장소 자산(assets/game-data)이 없다');
        return;
    }
    const paintVariants = createPaintVariants();
    // 원소별 3 × 3 그림 프레임 월드 격자이다.
    const coreGrids = [0, 1, 2, 3].map((theme) => {
        const grid = new Int32Array(WORLD_CELLS * WORLD_CELLS);
        // 월드 전체의 지역 도장 3 × 3 프레임을 채운다.
        for (let y = 0; y < WORLD_CELLS; y += 1) for (let x = 0; x < WORLD_CELLS; x += 1) grid[y * WORLD_CELLS + x] = paintCoreFrame(frames, paintVariants, theme, x, y);
        return grid;
    });
    const picks = masks.filter(({ source }, index) => index % 29 === 0 || /savetheisland|thewarbegins/i.test(source));
    assert.ok(picks.length >= 16);
    const seen = new Map();
    // 표본 맵마다 확인한다.
    for (const { source, mask } of picks) {
        const stampResult = buildTerrainFrames(mask, frames);
        // 원소 0~3 으로 칠해 본다.
        for (let theme = 0; theme < 4; theme += 1) {
            const calls = [];
            const painted = buildTerrainFrames(mask, frames, { themeOf: (territory) => { calls.push(territory); return theme; } });
            assert.equal(new Set(calls).size, calls.length, `${source}: 영역마다 한 번만 묻는다`);
            // 모든 칸을 확인한다.
            for (let i = 0; i < mask.length; i += 1) {
                if (mask[i] === EMPTY_TERRAIN) {
                    assert.equal(painted[i], -1);
                    continue;
                }
                const code = frames.codes[stampResult[i]];
                const pair = String.fromCharCode(code.side, code.variant);
                if (pair === 'AA') {
                    assert.equal(painted[i], coreGrids[theme][i], `${source}: 원소 ${theme} 안쪽 ${i}`);
                } else {
                    const row = pairRows.get(pair);
                    assert.equal(row.stage, 'exact', `${pair}: 첫 도장 프레임의 글자는 그대로 후보 묶음이다`);
                    assert.ok(row.sets[theme].includes(painted[i]), `${source}: 원소 ${theme} ${pair} 칸 ${i} 의 프레임 ${painted[i]}`);
                    if (!seen.has(`${pair}:${theme}`)) seen.set(`${pair}:${theme}`, new Set());
                    seen.get(`${pair}:${theme}`).add(painted[i]);
                }
            }
        }
    }
    // 공식 맵과 표본에서 후보가 4개 이상인 쌍은 원소 구간의 후보가 모두 쓰였다(구간 전체를 고를 수 있다).
    for (const pair of ['BF', 'CG', 'DH', 'EI']) {
        // 원소마다 후보 전체가 쓰였는지 확인한다.
        for (let theme = 0; theme < 4; theme += 1) {
            const expected = pairRows.get(pair).sets[theme];
            assert.deepEqual([...seen.get(`${pair}:${theme}`)].sort((a, b) => a - b), expected, `${pair} 원소 ${theme}`);
        }
    }
});

// 일부 영역만 칠하기: themeOf 가 null/undefined 를 돌려주는 영역은 첫 도장 그대로이고, 칠한 영역은 첫 도장 결과와 달라진다(원소 3 의 3 × 3 그림은 태양 묶음과 다르다). 칠하지 않은 영역의 난수 소비는 칠하는 영역보다 앞서므로 영향이 없다.
// 두 영역이 있는 공식 맵 Save the Island! 으로 확인한다. (2026-11-02 WB03-X)
test('buildTerrainFrames: 원소가 없는 영역은 첫 도장 그대로', () => {
    const layout = layoutFort(parseFort(archive.read('\\d\\savetheisland.fort'), types), pieceCodes);
    const mask = buildIslandMask(layout.map, layout.islands);
    const territories = [...new Set(mask)].filter((value) => value !== EMPTY_TERRAIN).sort((a, b) => a - b);
    assert.deepEqual(territories, [0, 7]);
    const base = buildTerrainFrames(mask, frames);
    // 칠할 영역과 칠하지 않는 영역을 바꿔 가며 확인한다.
    for (const [paintedTerritory, otherTerritory] of [[7, 0], [0, 7]]) {
        const partial = buildTerrainFrames(mask, frames, { themeOf: (territory) => (territory === paintedTerritory ? 3 : undefined) });
        let changed = 0;
        // 모든 칸을 확인한다.
        for (let i = 0; i < mask.length; i += 1) {
            if (mask[i] === otherTerritory) assert.equal(partial[i], base[i], `영역 ${otherTerritory} 칸 ${i}`);
            if (mask[i] === paintedTerritory && partial[i] !== base[i]) changed += 1;
        }
        // 칠한 영역은 대부분 바뀐다(안쪽 칸은 모두 원소 3 의 묶음으로 간다).
        assert.ok(changed > 0.8 * [...mask].filter((value) => value === paintedTerritory).length, `영역 ${paintedTerritory}: 바뀐 칸 ${changed}`);
    }
    // 어떤 영역도 칠하지 않으면 themeOf 가 없는 것과 같다.
    assert.deepEqual(buildTerrainFrames(mask, frames, { themeOf: () => null }), base);
});

// 난수·CRT 주입: 항상 0 을 돌려주는 난수는 모든 가장자리 칸에서 원소 유효 후보의 첫 번째를 고르고, 같은 CRT 시드는 같은 안쪽 칸 표를 만든다. 서로 다른 CRT 시드는 안쪽 3 × 3 그림 변형이 달라진다.
// 난수만 넘기면 CRT 는 고정 시드로 채운다. (2026-11-02 WB03-X)
test('buildTerrainFrames: 난수와 CRT 를 주입할 수 있다', () => {
    const land = drawn(2, rectangle(40, 40, 52, 49));
    const zero = { next() { return 0; } };
    const result = buildTerrainFrames(land, frames, { random: zero });
    const { sides, variants } = classifyCells(land);
    // 모든 칸을 확인한다.
    for (let i = 0; i < land.length; i += 1) {
        if (land[i] === EMPTY_TERRAIN) continue;
        const pair = String.fromCharCode(sides[i], variants[i]);
        if (pair === 'AA') continue;
        const set = pairRows.get(pair).sets[0];
        assert.equal(result[i], set[0], `${pair} 칸 ${i}`);
    }
    // 난수만 넘긴 결과는 기본 CRT(고정 시드)와 같은 안쪽 칸을 쓴다.
    const reference = buildTerrainFrames(land, frames);
    const interiorIndexes = [...land.keys()].filter((i) => land[i] !== EMPTY_TERRAIN && sides[i] === 0x41 && variants[i] === 0x41);
    assert.ok(interiorIndexes.length > 0);
    // 안쪽 칸은 같은 결과여야 한다.
    for (const i of interiorIndexes) assert.equal(result[i], reference[i]);
    // CRT 시드가 다르면 안쪽 변형이 달라진다(큰 안쪽 영역).
    const big = drawn(2, rectangle(40, 40, 120, 120));
    const a = buildTerrainFrames(big, frames, { random: zero, crt: new CrtRandom(1) });
    const b = buildTerrainFrames(big, frames, { random: zero, crt: new CrtRandom(2) });
    assert.notDeepEqual(a, b);
    assert.deepEqual(buildTerrainFrames(big, frames, { random: zero, crt: new CrtRandom(1) }), a);
    // 지면이 없는 마스크는 모두 -1 이다.
    assert.ok(buildTerrainFrames(drawn(2, []), frames).every((value) => value === -1));
});

// 난수를 쓰는 순서: 첫 도장은 월드를 y 바깥·x 안쪽 순서로, 지역 도장은 그 뒤에 같은 순서로 가장자리 칸마다 난수를 한 번씩 소비한다(안쪽 칸은 난수를 쓰지 않는다).
// `next(limit)` 가 호출 번호 % limit 를 돌려주는 난수로 기대 프레임을 손으로 계산해 순서를 고정한다. 칠하기가 첫 도장 결과를 덮어쓰므로 칠하지 않은 실행과 칠한 실행을 따로 본다. (2026-11-02 WB03-X)
test('buildTerrainFrames: 난수 소비 순서는 y 바깥·x 안쪽, 첫 도장 다음 지역 도장', () => {
    const land = drawn(2, rectangle(40, 40, 46, 45));
    const { sides, variants } = classifyCells(land);
    // 호출 번호 % limit 를 돌려주는 난수와 그 호출 수 읽기이다.
    const counting = () => {
        const state = { calls: 0 };
        state.random = { next(limit) { const value = state.calls % limit; state.calls += 1; return value; } };
        return state;
    };
    // 5 × 6 사각형에서 안쪽은 4 × 3 = 12칸, 가장자리는 18칸이다. 가장자리 칸의 y·x 순서 목록이다.
    const edges = [];
    // 월드를 y 바깥·x 안쪽으로 훑어 가장자리 칸을 모은다.
    for (let y = 0; y < WORLD_CELLS; y += 1) {
        for (let x = 0; x < WORLD_CELLS; x += 1) {
            const i = y * WORLD_CELLS + x;
            if (land[i] === EMPTY_TERRAIN || (sides[i] === 0x41 && variants[i] === 0x41)) continue;
            edges.push(i);
        }
    }
    assert.equal(edges.length, 18);
    // 칠하지 않은 실행: 가장자리 칸이 y·x 순서로 첫 호출부터 번호를 받는다(원소 0).
    const stamped = counting();
    const stampResult = buildTerrainFrames(land, frames, { random: stamped.random });
    edges.forEach((i, order) => {
        const set = pairRows.get(String.fromCharCode(sides[i], variants[i])).sets[0];
        assert.equal(stampResult[i], set[order % set.length], `첫 도장 칸 ${i}`);
    });
    assert.equal(stamped.calls, 18);
    // 칠한 실행: 첫 도장이 18번 쓴 뒤 지역 도장이 같은 순서로 이어서 쓴다(원소 1).
    const painted = counting();
    const paintResult = buildTerrainFrames(land, frames, { random: painted.random, themeOf: () => 1 });
    edges.forEach((i, order) => {
        const set = pairRows.get(String.fromCharCode(sides[i], variants[i])).sets[1];
        assert.equal(paintResult[i], set[(18 + order) % set.length], `지역 도장 칸 ${i}`);
    });
    assert.equal(painted.calls, 36);
});

// 후보가 4분할되지 않는 방향 글자('J': JJ00 하나뿐)를 가진 칸은 첫 도장에서 원본처럼 범위 오류가 난다(원본 465개에는 그런 칸이 없다). 세로 한 줄 통로 칸(북·남만 지면, 'J')을 만들어 확인한다. (2026-11-02 WB03-X)
test('buildTerrainFrames: 후보가 4분할되지 않는 칸은 범위 오류', () => {
    const corridor = drawn(2, [[60, 59], [60, 60], [60, 61], [59, 59], [61, 61]]);
    assert.equal(neighborLetters(corridor, 60, 60).side, 'J');
    assert.equal(neighborLetters(corridor, 60, 60).variant, 'J');
    assert.throws(() => buildTerrainFrames(corridor, frames), /quartile/);
});

// 후보가 하나도 없는 칸은 프레임 0 으로 대신한다(원본 도장의 `frame == -1 ? 0`). 프레임 0 의 코드가 안쪽(A, A) 이면 지역 도장은 그 칸을 안쪽 3 × 3 그림으로 칠한다(원본이 칠할 때 지금 프레임의 코드를 읽기 때문이다).
// 합성 표(AA 한 개 + JJ00 + 3 × 3 그림 144개)에서 고립된 칸(P, P)은 후보가 없다. (2026-11-02 WB03-X)
test('buildTerrainFrames: 후보가 없는 칸은 프레임 0', () => {
    const built = makeTable(['AA', 'JJ', ...Array.from({ length: 144 }, () => 'AA')]);
    const land = drawn(2, [[70, 70]]);
    assert.equal(terrainFrameCandidates(built, 'P', 'P').stage, 'none');
    const stamped = buildTerrainFrames(land, built);
    assert.equal(stamped[70 * WORLD_CELLS + 70], 0);
    // 소유 원소가 있으면 프레임 0 의 코드(A, A)로 판단해 그 원소의 3 × 3 그림 프레임이 된다.
    const painted = buildTerrainFrames(land, built, { themeOf: () => 3 });
    assert.equal(painted[70 * WORLD_CELLS + 70], paintCoreFrame(built, createPaintVariants(), 3, 70, 70));
    assert.ok(painted[70 * WORLD_CELLS + 70] >= 2 + 3 * CORE_THEME_FRAMES);
});

// 원소 이름과 3 × 3 그림 묶음의 대응: THEME_NAMES 의 번호가 3 × 3 그림의 묶음 순서이며 isle.type 의 그림 파일 이름 접두사(RA*=비, TH*=천둥, WI*=바람, 그 밖=태양)와 맞는다는 것은 기대값 표를 만드는 쪽(Python)이 확인했다(원소 순서가 어긋나면 표 생성이 실패한다).
// 여기서는 클러스터의 그림 개수가 모두 1 임(층이 하나)과 AA00 의 개수를 확인한다. (2026-11-02 WB03-X)
test('isle 타입의 클러스터 구성', () => {
    assert.ok(isleDefinition.clusters.every((cluster) => cluster.imageCount === 1));
    assert.equal(isleDefinition.clusters.filter((cluster) => cluster.name === 'AA00').length, 4 * CORE_THEME_FRAMES);
    // 프레임 코드의 플래그: fringe(1)·rim(4) 조합만 있다.
    assert.deepEqual([...new Set(frames.codes.map((code) => code.flags))].sort((a, b) => a - b), [0, 4, 5]);
});
