// 영역(Territory) 배치를 검사한다: 영역 패턴 표(실행 파일에서 따로 뽑은 dotnetpj 자료와 대조), 패턴 풀기(회전·좌표·글자), 청크 지도/섬 목록/영역 놓기의 규칙,
// 원본 `.fort` 465개 전체(느슨한 파일 433 + 아카이브 32)의 배치와 저장 청크 짝짓기를 독립 구현(tools/fort.py + 패턴 JSON)이 만든 기대값 표와 줄 단위로 대조,
// Save the Island! 의 알려진 월드 좌표(캡처와 일치하는 신전·사제·포대)를 확인한다.
// 기대값 표는 webpj/tools/make-territory-expected.py 가 만들었고 www/ 구현의 출력으로 다시 만들지 않는다.
// (2026-11-01 WB03-V: 정적 맵 화면(M2) 2단계 영역 배치 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import crypto from 'node:crypto';
import fs from 'node:fs';
import path from 'node:path';
import { REPO_ROOT, WEBPJ_ROOT, loadOriginalForts, readData } from './helpers.mjs';
import { TaffArchive } from '../www/assets/taff.js';
import { decodeOriginalText } from '../www/assets/text-encoding.js';
import { FIRST_ASSET_TYPE_NUMBER, TYPE_LOAD_ORDER, TypeTable } from '../www/assets/type-table.js';
import { createFortTypes } from '../www/assets/type-flags.js';
import { CHUNK_CELLS, TERRITORY_COUNT, WORLD_CHUNKS_PER_SIDE, parseFort } from '../www/assets/fort.js';
import { TERRITORY_PATTERNS, TERRITORY_PATTERN_COUNT } from '../www/assets/territory-patterns.js';
import {
    ChunkMap, DEFAULT_CHUNK_RECT, IslandBuilder, IslandList, NO_ISLAND, ROTATED_SIDES, TERRITORY_RECORD_BYTES, decodeTerritoryPattern, findFrameNumber, layoutFort,
    pieceFrameCodes, readTerritoryRecord, territoryChunks, worldCell,
} from '../www/assets/territory.js';

// 공용 자료이다: 아카이브·타입 표·`.fort` 판독용 타입 정보·영역 패턴 타입의 프레임 코드. (2026-11-01 WB03-V)
const archive = new TaffArchive(readData('netstorm.tarc'));
const table = TypeTable.fromArchive(archive, decodeOriginalText);
const types = createFortTypes(table);
const frameCodes = pieceFrameCodes(table);

// 영역 레코드 6바이트를 만든다: 모양 번호·방향 값(0~3)·플래그·위치 x(하위 니블)·위치 y(상위 니블)·남는 두 바이트. (2026-11-01 WB03-V)
function record(shape, direction, flags, x, y) {
    return Uint8Array.of((direction << 6) | shape, flags, (y << 4) | x, 0, 0, 0);
}

// 놓인 청크 목록을 `x,y방향글자;` 한 글로 만든다(cpppj 테스트와 같은 서식). (2026-11-01 WB03-V)
function chunkText(map, islands, owner, territory) {
    return territoryChunks(map, islands, owner, territory).map(({ x, y }) => `${x},${y}${map.at(x, y).side};`).join('');
}

// 영역 레코드 20개(120바이트)를 만든다. records 는 [번호, 6바이트] 쌍이고 나머지는 0 이다. (2026-11-01 WB03-V)
function territoryBytes(records) {
    const bytes = new Uint8Array(TERRITORY_COUNT * TERRITORY_RECORD_BYTES);
    for (const [index, value] of records) bytes.set(value, index * TERRITORY_RECORD_BYTES);
    return bytes;
}

// ----- 영역 패턴 표 -----

// 표는 원본 68개이고 모양마다 폭 × 높이(≤ 15)칸의 세 글자열을 가진다. 방향 글자는 'A'~'P' 또는 빈 칸 '.'이며 변형 글자는 빈 칸이 아닌 칸에서 숫자이다. (2026-11-01 WB03-V)
test('영역 패턴 표: 68개와 칸 구성', () => {
    assert.equal(TERRITORY_PATTERNS.length, 68);
    assert.equal(TERRITORY_PATTERN_COUNT, 68);
    TERRITORY_PATTERNS.forEach((pattern, number) => {
        const cells = pattern.width * pattern.height;
        assert.ok(cells >= 1 && cells <= 15, `패턴 ${number} 크기`);
        assert.equal(pattern.sides.length, cells, `패턴 ${number} sides`);
        assert.equal(pattern.variations.length, cells, `패턴 ${number} variations`);
        assert.equal(pattern.labels.length, cells, `패턴 ${number} labels`);
        for (let i = 0; i < cells; i += 1) {
            const side = pattern.sides[i];
            assert.ok(side === '.' || (side >= 'A' && side <= 'P'), `패턴 ${number} 칸 ${i} 방향 글자`);
            if (side !== '.') assert.match(pattern.variations[i], /[0-9]/, `패턴 ${number} 칸 ${i} 변형`);
            assert.match(pattern.labels[i], /[-a-z]/, `패턴 ${number} 칸 ${i} 번호 글자`);
        }
    });
    assert.ok(Object.isFrozen(TERRITORY_PATTERNS));
});

// 표는 실행 파일에서 따로 뽑은 dotnetpj 의 패턴 JSON(폭·높이·방향 글자·변형·번호)과 칸마다 같다. cpppj 표를 옮긴 이쪽과 실행 파일을 직접 읽은 저쪽이 일치해야 한다. (2026-11-01 WB03-V)
test('영역 패턴 표: 실행 파일에서 따로 뽑은 dotnetpj 자료와 같다', (context) => {
    const file = path.join(REPO_ROOT, 'dotnetpj', 'src', 'Netstorm.Assets', 'TerritoryPatterns.json');
    if (!fs.existsSync(file)) {
        context.skip('dotnetpj 패턴 자료가 없다');
        return;
    }
    const reference = JSON.parse(fs.readFileSync(file, 'utf8'));
    assert.equal(reference.length, TERRITORY_PATTERNS.length);
    reference.forEach((expected, number) => {
        const pattern = TERRITORY_PATTERNS[number];
        assert.equal(pattern.width, expected.Width, `패턴 ${number} 폭`);
        assert.equal(pattern.height, expected.Height, `패턴 ${number} 높이`);
        assert.equal(pattern.sides, expected.Cells, `패턴 ${number} 방향 글자`);
        // 번호 글자: 없으면 JSON 은 -97 이다('a' - 0x61 = 0 부터).
        const labels = [...pattern.labels].map((char) => (char === '-' ? -97 : char.charCodeAt(0) - 0x61));
        assert.deepEqual(labels, expected.Labels, `패턴 ${number} 번호`);
        // 변형 숫자는 빈 칸이 아닌 칸에서만 비교한다(빈 칸은 JSON 이 0 으로 적는다).
        for (let i = 0; i < pattern.sides.length; i += 1) {
            if (pattern.sides[i] !== '.') assert.equal(pattern.variations.charCodeAt(i) - 0x30, expected.Variations[i], `패턴 ${number} 칸 ${i} 변형`);
        }
    });
});

// ----- 프레임 찾기와 패턴 풀기 -----

// 프레임 찾기는 (방향 글자, 변형 글자, 번호)가 모두 같은 첫 프레임이다. 없으면 -1, 중복이면 앞의 것이다. (2026-11-01 WB03-V)
test('findFrameNumber: 첫 일치와 없음', () => {
    const codes = [{ side: 65, variant: 80, number: 1 }, { side: 66, variant: 80, number: 1 }, { side: 66, variant: 80, number: 1 }, { side: 66, variant: 81, number: 2 }];
    assert.equal(findFrameNumber(codes, 66, 80, 1), 1);
    assert.equal(findFrameNumber(codes, 66, 81, 2), 3);
    assert.equal(findFrameNumber(codes, 66, 80, 2), -1);
    assert.equal(findFrameNumber(codes, 67, 80, 1), -1);
    assert.equal(findFrameNumber([], 65, 80, 1), -1);
});

// `puzzlePiece` 타입은 읽혔고 프레임 코드는 클러스터 수와 같으며, 정의가 없으면 오류이다. (2026-11-01 WB03-V)
test('pieceFrameCodes: puzzlePiece 프레임 코드', () => {
    assert.equal(frameCodes.length, table.definitions[TYPE_LOAD_ORDER.indexOf('puzzlePiece')].clusters.length);
    assert.ok(frameCodes.length > 0);
    frameCodes.forEach((code) => assert.ok(code.side >= 0x41 && code.side <= 0x5a));
    assert.throws(() => pieceFrameCodes({ definitions: TYPE_LOAD_ORDER.map(() => null) }), /puzzlePiece/);
});

// 프레임 코드를 가진 가짜 패턴 풀이다: 모든 방향 글자 A~P 가 변형 'P' 와 변형 숫자 1 로 하나씩 있다(프레임 번호 = 글자 순서). (2026-11-01 WB03-V)
function lettersAsFrames(variation = 1) {
    return Array.from({ length: 16 }, (_, i) => ({ side: 0x41 + i, variant: 0x50, number: variation }));
}

// 한 패턴을 풀어 칸 목록을 배열로 만든다. (2026-11-01 WB03-V)
function decode(pattern, direction, x, y, codes = frameCodes) {
    return [...decodeTerritoryPattern(pattern, direction, x, y, codes)];
}

// 회전 0: 줄 우선 순서로 빈 칸을 건너뛰고 좌표는 (원점 + 칸 위치), 방향 글자는 패턴 글자 그대로이다. 모든 패턴·원점에서 확인한다. (2026-11-01 WB03-V)
test('decodeTerritoryPattern: 회전 0 은 줄 우선 순서와 패턴 글자 그대로', () => {
    TERRITORY_PATTERNS.forEach((pattern, number) => {
        const expected = [];
        // 독립 계산: 줄 우선으로 훑어 빈 칸이 아닌 칸의 (x, y, 글자)를 모은다.
        for (let i = 0; i < pattern.sides.length; i += 1) {
            if (pattern.sides[i] !== '.') expected.push(`${7 + (i % pattern.width)},${3 + Math.trunc(i / pattern.width)}${pattern.sides[i]}`);
        }
        const actual = decode(pattern, 0, 7, 3).map((cell) => `${cell.x},${cell.y}${cell.side}`);
        assert.deepEqual(actual, expected, `패턴 ${number}`);
    });
});

// 저장 방향 값은 0~3 이고 회전은 ÷ 2 라 0·1 은 회전 0, 2·3 은 회전 1(시계 방향 90도)이다. (2026-11-01 WB03-V)
test('decodeTerritoryPattern: 방향 값 ÷ 2 로 회전을 고른다', () => {
    const pattern = TERRITORY_PATTERNS[5];
    assert.deepEqual(decode(pattern, 1, 4, 4), decode(pattern, 0, 4, 4));
    assert.deepEqual(decode(pattern, 3, 4, 4), decode(pattern, 2, 4, 4));
    assert.notDeepEqual(decode(pattern, 2, 4, 4), decode(pattern, 0, 4, 4));
    // 회전 2·3 은 저장 값으로는 나오지 않지만 풀기는 된다(방향 값 4~7). 범위 밖은 오류이다.
    assert.ok(decode(pattern, 4, 4, 4).length > 0);
    assert.ok(decode(pattern, 7, 4, 4).length > 0);
    assert.throws(() => decode(pattern, 8, 4, 4), /rotation/);
    assert.throws(() => decode(pattern, -2, 4, 4), /rotation/);
});

// 회전 r(방향 값 2r)은 패턴을 시계 방향 90도 × r 돌린 것이다. 칸 (열 c, 줄 r)이 가는 새 위치(폭 w, 높이 h)는 회전 1 이 (h-1-r, c), 회전 2 가 (w-1-c, h-1-r), 회전 3 이 (r, w-1-c)이다.
// 방향 글자는 회전표에 따라 바뀐다. 기하는 독립 계산으로, 글자 변환은 회전표의 성질(순열·4번 돌리면 제자리)로 확인한다. (2026-11-01 WB03-V)
test('decodeTerritoryPattern: 회전 1~3 은 시계 방향 90도 단위의 기하와 같다', () => {
    const newPosition = [
        null,
        (c, r, w, h) => [h - 1 - r, c],
        (c, r, w, h) => [w - 1 - c, h - 1 - r],
        (c, r, w, h) => [r, w - 1 - c],
    ];
    for (const rotation of [1, 2, 3]) {
        TERRITORY_PATTERNS.forEach((pattern, number) => {
            const expected = new Map();
            // 독립 계산: 빈 칸이 아닌 칸마다 새 위치와 원래 글자를 적는다.
            for (let i = 0; i < pattern.sides.length; i += 1) {
                if (pattern.sides[i] === '.') continue;
                const [nc, nr] = newPosition[rotation](i % pattern.width, Math.trunc(i / pattern.width), pattern.width, pattern.height);
                expected.set(`${10 + nc},${2 + nr}`, pattern.sides[i]);
            }
            const actual = decode(pattern, rotation * 2, 10, 2);
            assert.equal(actual.length, expected.size, `회전 ${rotation} 패턴 ${number} 칸 수`);
            for (const cell of actual) {
                const original = expected.get(`${cell.x},${cell.y}`);
                assert.ok(original, `회전 ${rotation} 패턴 ${number} 의 칸 ${cell.x},${cell.y} 는 회전된 위치여야 한다`);
                assert.equal(cell.side, ROTATED_SIDES[rotation][original.charCodeAt(0) - 0x41], `회전 ${rotation} 패턴 ${number} 의 칸 ${cell.x},${cell.y} 글자`);
            }
        });
    }
});

// 회전표: 네 줄 모두 'A'~'P' 16글자의 순열이고, 'A'·'P' 는 고정이며, 회전 1 을 두 번·세 번·네 번 적용하면 회전 2·3·0 이다. (2026-11-01 WB03-V)
test('회전표: 순열이며 회전 1 의 거듭제곱이다', () => {
    const alphabet = 'ABCDEFGHIJKLMNOP';
    ROTATED_SIDES.forEach((row, rotation) => {
        assert.equal(row.length, 16);
        assert.equal([...row].sort().join(''), alphabet, `회전 ${rotation} 순열`);
        assert.equal(row[0], 'A');
        assert.equal(row[15], 'P');
    });
    assert.equal(ROTATED_SIDES[0], alphabet);
    // 글자 변환을 거듭 적용한다.
    const apply = (row, text) => [...text].map((char) => row[char.charCodeAt(0) - 0x41]).join('');
    const once = ROTATED_SIDES[1];
    assert.equal(apply(once, once), ROTATED_SIDES[2]);
    assert.equal(apply(once, ROTATED_SIDES[2]), ROTATED_SIDES[3]);
    assert.equal(apply(once, ROTATED_SIDES[3]), alphabet);
});

// 프레임 코드에 없는 글자 칸은 건너뛰고 알린다(원본 assert 후 계속). 번호 글자·변형 범위 오류는 오류이다. (2026-11-01 WB03-V)
test('decodeTerritoryPattern: 프레임 없는 칸 건너뜀과 원본 assert', () => {
    const pattern = { width: 3, height: 1, sides: 'FAG', variations: '111', labels: '---' };
    // 'A' 프레임이 없는 코드 목록: 가운데 칸이 건너뛰어진다.
    const codes = lettersAsFrames().filter((code) => code.side !== 0x41);
    let missing = 0;
    const cells = [...decodeTerritoryPattern(pattern, 0, 0, 0, codes, () => { missing += 1; })];
    assert.deepEqual(cells.map((cell) => cell.side), ['F', 'G']);
    assert.deepEqual(cells.map((cell) => cell.x), [0, 2]);
    assert.equal(missing, 1);
    // 변형 숫자가 다르면 프레임이 없다.
    assert.equal(decode(pattern, 0, 0, 0, lettersAsFrames(2)).length, 0);
    // 번호 글자는 'a'~'z' 이고 값은 'a' 를 뺀 수이다(없으면 -0x61).
    const labeled = decode({ width: 2, height: 1, sides: 'FG', variations: '11', labels: 'c-' }, 0, 0, 0, lettersAsFrames());
    assert.deepEqual(labeled.map((cell) => cell.label), [2, -0x61]);
    assert.throws(() => decode({ width: 1, height: 1, sides: 'F', variations: '1', labels: 'A' }, 0, 0, 0, lettersAsFrames()), /num==0/);
    // 경계: 번호 글자 'a'(0x61)·'z'(0x7a)는 허용, 0x60·0x7b 는 오류. 방향 글자 'A'·'P' 는 허용, '@'(0x40)·'Q'(0x51)·'Z' 는 오류. 변형 글자 '/'(-1)는 오류.
    for (const label of ['a', 'z']) assert.equal(decode({ width: 1, height: 1, sides: 'F', variations: '1', labels: label }, 0, 0, 0, lettersAsFrames()).length, 1, `번호 ${label}`);
    for (const label of ['`', '{']) assert.throws(() => decode({ width: 1, height: 1, sides: 'F', variations: '1', labels: label }, 0, 0, 0, lettersAsFrames()), /num==0/, `번호 ${label}`);
    for (const side of ['@', 'Q', 'Z']) assert.throws(() => decode({ width: 1, height: 1, sides: side, variations: '1', labels: '-' }, 0, 0, 0, lettersAsFrames()), /orientation/, `방향 ${side}`);
    for (const side of ['A', 'P']) assert.equal(decode({ width: 1, height: 1, sides: side, variations: '1', labels: '-' }, 0, 0, 0, lettersAsFrames()).length, 1, `방향 ${side}`);
    assert.throws(() => decode({ width: 1, height: 1, sides: 'F', variations: '/', labels: '-' }, 0, 0, 0, lettersAsFrames()), /variation/);
    assert.equal(decode({ width: 1, height: 1, sides: 'F', variations: '0', labels: '-' }, 0, 0, 0, lettersAsFrames(0)).length, 1, '변형 0 은 허용 범위 처음');
    assert.throws(() => decode({ width: 1, height: 1, sides: 'F', variations: '=', labels: '-' }, 0, 0, 0, lettersAsFrames()), /variation/);
    assert.throws(() => decode({ width: 1, height: 1, sides: 'F', variations: ';', labels: '-' }, 0, 0, 0, lettersAsFrames()), /variation/);
    // 변형 숫자 10('0x3a' ':')은 허용 범위 끝이다.
    assert.equal(decode({ width: 1, height: 1, sides: 'F', variations: ':', labels: '-' }, 0, 0, 0, lettersAsFrames(10)).length, 1);
});

// 영역 패턴의 모든 비어 있지 않은 칸은 실제 `puzzlePiece` 프레임(32개)을 찾는다: 표의 68개 패턴 모두에서 회전 0~3(방향 값 0·2·4·6) 어느 것이든 건너뛰는 칸이 없다
// (저장 방향 값 0~3 이 닿는 것은 회전 0·1 이지만 회전표 전체를 확인한다). (2026-11-01 WB03-V)
test('decodeTerritoryPattern: 실제 puzzlePiece 프레임에서 모든 패턴·회전이 빠지는 칸 없이 풀린다', () => {
    assert.equal(frameCodes.length, 32);
    for (const direction of [0, 2, 4, 6]) {
        TERRITORY_PATTERNS.forEach((pattern, number) => {
            let missing = 0;
            const cells = [...decodeTerritoryPattern(pattern, direction, 0, 0, frameCodes, () => { missing += 1; })];
            assert.equal(missing, 0, `패턴 ${number} 방향 ${direction}`);
            assert.equal(cells.length, [...pattern.sides].filter((side) => side !== '.').length, `패턴 ${number} 방향 ${direction} 칸 수`);
            // 프레임 번호는 프레임 코드 목록 안이고 그 코드의 글자가 칸의 글자이다.
            cells.forEach((cell) => {
                assert.ok(cell.frame >= 0 && cell.frame < frameCodes.length);
                assert.equal(String.fromCharCode(frameCodes[cell.frame].side), cell.side);
            });
        });
    }
});

// ----- 섬 목록·청크 지도·영역 놓기 -----

// 섬 목록: 번호는 등록 순서이고 소유자와 영역 번호가 모두 맞아야 일치한다. (2026-11-01 WB03-V)
test('IslandList: 등록 순서와 일치 조건', () => {
    const islands = new IslandList();
    assert.ok(!islands.exists(0) && !islands.matches(0, 1, 0));
    assert.equal(islands.add(1, 3), 0);
    assert.equal(islands.add(2, 3), 1);
    assert.ok(islands.exists(1) && !islands.exists(2) && !islands.exists(-1));
    assert.ok(islands.matches(0, 1, 3) && !islands.matches(0, 2, 3) && !islands.matches(0, 1, 4));
    assert.ok(!islands.matches(NO_ISLAND, 1, 3));
    assert.equal(islands.islands.length, 2);
});

// 청크 지도: 처음엔 섬이 없고, 범위 밖 좌표는 가장자리로 당겨지며, 섬의 사각형은 차지한 청크를 모두 감싼다(찾지 못하면 16,16,1,1). 크기 범위는 1~16 이다. (2026-11-01 WB03-V)
test('ChunkMap: 초기 상태·가장자리 당김·섬 사각형·크기 범위', () => {
    const map = new ChunkMap();
    assert.equal(map.xLength, 16);
    assert.equal(map.entries.length, 256);
    assert.ok(!map.hasIsland(3, 3));
    assert.equal(map.at(3, 3).island, NO_ISLAND);
    assert.equal(NO_ISLAND, 0x7f);
    assert.deepEqual(map.bounds(0), { left: 16, top: 16, right: 1, bottom: 1 });
    map.at(3, 8).island = 0;
    map.at(5, 9).island = 0;
    assert.ok(map.hasIsland(3, 8));
    assert.deepEqual(map.bounds(0), { left: 3, top: 8, right: 6, bottom: 10 });
    // 범위 밖 좌표는 가장자리의 항목이다.
    assert.equal(map.at(-5, 8), map.at(0, 8));
    assert.equal(map.at(99, 99), map.at(15, 15));
    assert.ok(map.contains(15, 15) && !map.contains(16, 0) && !map.contains(0, -1));
    // 더 작은 지도는 그 크기까지만 당긴다.
    const small = new ChunkMap(4, 3);
    assert.equal(small.at(9, 9), small.at(3, 2));
    assert.throws(() => new ChunkMap(0, 4), /Chunk map size/);
    assert.throws(() => new ChunkMap(4, 17), /Chunk map size/);
    // 항목은 서로 다른 객체이다(레코드 배열을 공유하지 않는다).
    assert.notEqual(map.at(1, 1).record, map.at(2, 1).record);
});

// 원본 0046dba0·0046dd70: TEST01 의 영역 0~3(알려진 배치는 cpppj 테스트 IslandBuilder_PlacesTerritoriesAtStoredPositions 와 같다). (2026-11-01 WB03-V)
test('IslandBuilder: 저장 위치에 영역을 놓는다(TEST01 의 알려진 배치)', () => {
    const map = new ChunkMap();
    const islands = new IslandList();
    const builder = new IslandBuilder(map, islands, frameCodes, 1);
    // TEST01 의 영역 0~3 과 놓지 않는 레코드 둘(플래그 0, 플래그 bit 1).
    const territories = territoryBytes([
        [0, record(0, 0, 9, 1, 6)], [1, record(5, 0, 9, 10, 2)], [2, record(2, 0, 9, 9, 10)], [3, record(0, 0, 9, 6, 3)], [4, record(7, 0, 0, 0, 0)], [5, record(7, 0, 3, 0, 0)],
    ]);
    builder.placeAll(territories);
    assert.equal(islands.islands.length, 4);
    assert.equal(chunkText(map, islands, 1, 0), '3,7L;2,8F;3,8A;4,8G;2,9I;3,9N;4,9H;');
    assert.equal(chunkText(map, islands, 1, 1), '11,3O;12,3K;13,3G;13,4N;');
    assert.equal(chunkText(map, islands, 1, 2), '10,11F;11,11C;12,11G;10,12B;11,12A;12,12D;10,13I;11,13N;12,13H;');
    assert.equal(chunkText(map, islands, 1, 3), '8,4L;7,5F;8,5A;9,5G;7,6I;8,6N;9,6H;');
    assert.equal(chunkText(map, islands, 1, 4), '');
    assert.equal(chunkText(map, islands, 1, 5), '');
    // 다른 소유자의 영역으로는 찾지 않는다.
    assert.equal(chunkText(map, islands, 2, 0), '');
    // 청크 항목: 섬 번호, 레코드 6바이트, 영역 안의 순서, 방향 글자.
    const entry = map.at(3, 8);
    assert.equal(entry.island, 0);
    assert.equal(entry.side, 'A');
    assert.equal(entry.ordinal, 2);
    assert.deepEqual([...entry.record], [0, 9, 0x61, 0, 0, 0]);
    assert.deepEqual(map.bounds(1), { left: 11, top: 3, right: 14, bottom: 5 });
    assert.ok(!map.hasIsland(2, 7) && map.hasIsland(3, 7));
    assert.equal(builder.clampedCells, 0);
    assert.equal(builder.missingFrames, 0);
});

// 한 레코드 놓기: 위치 바이트 무시, 기준 사각형의 원점, 겹침(나중에 놓은 영역이 차지), 지도 밖 칸 세기, 잘못된 입력. (2026-11-01 WB03-V)
test('IslandBuilder: 놓기 옵션·겹침·오류', () => {
    const map = new ChunkMap();
    const islands = new IslandList();
    const builder = new IslandBuilder(map, islands, frameCodes, 3);
    // Save the Island 의 영역 7: 2 × 2, 위치 바이트 0x59 → (10,6)부터.
    const sample = record(7, 0, 9, 9, 5);
    assert.equal(sample[2], 0x59);
    assert.equal(builder.place(sample, 7), 0);
    assert.equal(chunkText(map, islands, 3, 7), '10,6F;11,6G;10,7I;11,7H;');
    // 위치를 무시하면 기준 사각형의 왼쪽 위(1,1)에 놓는다.
    assert.equal(builder.place(sample, 8, true), 1);
    assert.equal(chunkText(map, islands, 3, 8), '1,1F;2,1G;1,2I;2,2H;');
    // 기준 사각형을 달리 주면 그 원점에서 센다.
    const other = new ChunkMap();
    const otherIslands = new IslandList();
    const shifted = new IslandBuilder(other, otherIslands, frameCodes, 1, { left: 4, top: 5, right: 15, bottom: 15 });
    shifted.place(record(4, 0, 9, 1, 2), 0);
    assert.equal(chunkText(other, otherIslands, 1, 0), '5,7O;6,7K;7,7M;');
    // 나중에 놓은 영역이 겹친 청크를 차지한다.
    shifted.place(record(4, 0, 9, 3, 2), 1);
    assert.equal(chunkText(other, otherIslands, 1, 0), '5,7O;6,7K;');
    assert.equal(chunkText(other, otherIslands, 1, 1), '7,7O;8,7K;9,7M;');
    // 모양 번호는 하위 6비트(최대 63)라 32 이상도 고르고 표를 벗어나지 않는다: 패턴 40(5 × 2, 'PPPPP..P..')은 6칸, 패턴 8('OG..IM')은 4칸이다. 레코드가 짧으면 오류이다.
    const wideMap = new ChunkMap();
    const wideIslands = new IslandList();
    new IslandBuilder(wideMap, wideIslands, frameCodes, 1).place(record(40, 0, 9, 0, 0), 0);
    assert.equal(chunkText(wideMap, wideIslands, 1, 0), '1,1P;2,1P;3,1P;4,1P;5,1P;3,2P;');
    builder.place(record(0x3f, 0, 9, 0, 0), 0);
    assert.throws(() => builder.place(Uint8Array.of(0, 9, 0), 0), /Territory record/);
    assert.throws(() => builder.placeAll(new Uint8Array(120), 3, 3), /iStart != iEnd/);
    // 지도 밖(위치 15,15 + 기준 1 → 16)으로 나간 칸은 가장자리로 당겨지고 세어진다.
    const edge = new ChunkMap();
    const edgeIslands = new IslandList();
    const edgeBuilder = new IslandBuilder(edge, edgeIslands, frameCodes, 1);
    edgeBuilder.place(record(7, 0, 9, 15, 15), 0);
    assert.equal(edgeBuilder.clampedCells, 4);
    assert.ok(edge.hasIsland(15, 15));
    assert.equal(edge.at(15, 15).side, 'H');
    // 회전된 영역(방향 값 2)도 놓인다: 3 × 2 모양 5(OKG..N)가 2 × 3 이 된다.
    const rotatedMap = new ChunkMap();
    const rotatedIslands = new IslandList();
    new IslandBuilder(rotatedMap, rotatedIslands, frameCodes, 1).place(record(5, 2, 9, 0, 0), 0);
    assert.equal(territoryChunks(rotatedMap, rotatedIslands, 1, 0).length, 4);
    assert.deepEqual(rotatedMap.bounds(0), { left: 1, top: 1, right: 3, bottom: 4 });
});

// 놓는 조건: 플래그 bit 0 이 켜지고 bit 1 이 꺼져야 한다(값 0·2·3 은 놓지 않는다). 레코드가 120바이트보다 짧으면 있는 만큼만 본다. (2026-11-01 WB03-V)
test('IslandBuilder.placeAll: 활성·건너뜀 플래그', () => {
    for (const [flags, placed] of [[0, 0], [1, 1], [2, 0], [3, 0], [9, 1], [8, 0], [0xfd, 1]]) {
        const map = new ChunkMap();
        const islands = new IslandList();
        new IslandBuilder(map, islands, frameCodes, 1).placeAll(territoryBytes([[2, record(7, 0, flags, 3, 3)]]));
        assert.equal(islands.islands.length, placed, `플래그 ${flags}`);
    }
    const map = new ChunkMap();
    const islands = new IslandList();
    // 3번째 레코드까지만 있는 짧은 입력은 있는 만큼만 놓는다.
    const short = new Uint8Array(3 * TERRITORY_RECORD_BYTES);
    short.set(record(7, 0, 9, 1, 1), 2 * TERRITORY_RECORD_BYTES);
    new IslandBuilder(map, islands, frameCodes, 1).placeAll(short);
    assert.equal(islands.islands.length, 1);
    assert.equal(islands.islands[0].territory, 2);
    // 마지막 영역(19번)의 레코드도 놓인다(레코드 끝이 입력 끝과 정확히 같은 경우).
    const lastMap = new ChunkMap();
    const lastIslands = new IslandList();
    new IslandBuilder(lastMap, lastIslands, frameCodes, 1).placeAll(territoryBytes([[19, record(7, 0, 9, 4, 4)]]));
    assert.deepEqual(lastIslands.islands, [{ owner: 1, territory: 19 }]);
    // 범위를 지정하면 그 번호만 본다.
    const ranged = territoryBytes([[1, record(7, 0, 9, 1, 1)], [2, record(7, 0, 9, 5, 5)], [3, record(7, 0, 9, 8, 8)]]);
    const rangedIslands = new IslandList();
    new IslandBuilder(new ChunkMap(), rangedIslands, frameCodes, 1).placeAll(ranged, 2, 4);
    assert.deepEqual(rangedIslands.islands.map((island) => island.territory), [2, 3]);
});

// 영역 레코드 읽기와 월드 칸 좌표. (2026-11-01 WB03-V)
test('readTerritoryRecord·worldCell', () => {
    const bytes = territoryBytes([[3, Uint8Array.of(0xc7, 9, 0x59, 1, 2, 3)]]);
    assert.deepEqual({ ...readTerritoryRecord(bytes, 3), raw: undefined }, { shape: 7, direction: 3, flags: 9, active: true, positionX: 9, positionY: 5, raw: undefined });
    assert.deepEqual([...readTerritoryRecord(bytes, 3).raw], [0xc7, 9, 0x59, 1, 2, 3]);
    assert.equal(readTerritoryRecord(bytes, 0).active, false);
    assert.equal(readTerritoryRecord(territoryBytes([[0, Uint8Array.of(0, 3, 0, 0, 0, 0)]]), 0).active, false, '건너뜀 플래그');
    assert.deepEqual(worldCell(11, 6, { cellX: 11, cellY: 12 }), { x: 187, y: 108 });
    assert.deepEqual(worldCell(0, 0, { cellX: 0, cellY: 0 }), { x: 0, y: 0 });
    assert.equal(CHUNK_CELLS, 16);
});

// layoutFort 합성 입력: 영역이 없으면 놓인 청크가 없고, Chaff 는 월드 청크 순서이며, 저장 청크 수가 다르면 matches 가 거짓이다. (2026-11-01 WB03-V)
test('layoutFort: 합성 입력의 짝짓기와 수 불일치', () => {
    const chunk = (name) => ({ objects: [name] });
    const fort = {
        territory: territoryBytes([[0, record(7, 0, 9, 9, 5)], [1, record(4, 0, 9, 1, 2)]]),
        chaff: { chunks: Array.from({ length: 40 }, (_, i) => chunk(`c${i}`)) },
        territories: Array.from({ length: TERRITORY_COUNT }, () => ({ chunks: [] })),
    };
    fort.territories[0].chunks = [chunk('a'), chunk('b'), chunk('c'), chunk('d')];
    fort.territories[1].chunks = [chunk('e'), chunk('f')];
    const layout = layoutFort(fort, frameCodes);
    assert.equal(layout.regions.length, TERRITORY_COUNT);
    // Chaff 의 청크 i 는 월드 청크 (i % 16, i ÷ 16).
    assert.deepEqual([layout.chaff[17].x, layout.chaff[17].y, layout.chaff[17].chunk.objects[0]], [1, 1, 'c17']);
    // 영역 0: 모양 7(2 × 2)이 (10,6)부터, 저장 청크 4개가 y 바깥·x 안쪽 순서로 짝지어진다.
    assert.deepEqual(layout.regions[0].chunks.map((c) => `${c.x},${c.y}${c.side}:${c.stored.objects[0]}`), ['10,6F:a', '11,6G:b', '10,7I:c', '11,7H:d']);
    assert.ok(layout.regions[0].matches);
    assert.deepEqual(layout.regions[0].record, { ...layout.regions[0].record, shape: 7, direction: 0, flags: 9, positionX: 9, positionY: 5 });
    // 영역 1: 놓인 청크는 3개인데 저장은 2개 → 마지막 청크는 짝이 없고 matches 는 거짓이다.
    assert.equal(layout.regions[1].chunks.length, 3);
    assert.equal(layout.regions[1].storedCount, 2);
    assert.ok(!layout.regions[1].matches);
    assert.equal(layout.regions[1].chunks[2].stored, null);
    // 놓이지 않은 영역은 비어 있고 저장도 없으니 일치로 본다.
    assert.deepEqual(layout.regions[2].chunks, []);
    assert.ok(layout.regions[2].matches);
    // 영역 레코드가 없는 파일(territory null)은 아무것도 놓지 않는다.
    const none = layoutFort({ ...fort, territory: null }, frameCodes);
    assert.ok(none.regions.every((region) => region.chunks.length === 0 && region.record === null));
    assert.equal(none.islands.islands.length, 0);
});

// ----- 원본 .fort 전체 -----

// 파일 하나의 영역 배치를 기대값 표와 같은 서식의 줄 목록으로 만든다(tools/make-territory-expected.py 와 같은 서식). (2026-11-01 WB03-V)
function describeLayout(source, fort, layout) {
    const lines = [`FORT ${source}`];
    const typeLabel = (type) => (type >= FIRST_ASSET_TYPE_NUMBER && type < FIRST_ASSET_TYPE_NUMBER + TYPE_LOAD_ORDER.length ? types.records[type].assetName : '-');
    const objects = [];
    // 월드 칸 좌표와 타입 이름으로 청크 안 오브젝트를 모은다(빈 레코드는 건너뛴다).
    const collect = (cx, cy, chunk) => {
        for (const object of chunk.objects) {
            if (object.type === 0) continue;
            const cell = worldCell(cx, cy, object);
            objects.push(`${cell.x},${cell.y}|${typeLabel(object.type)}`);
        }
    };
    for (const entry of layout.chaff) collect(entry.x, entry.y, entry.chunk);
    // 영역마다 레코드 값, 저장 청크 수, 놓인 청크를 적는다.
    for (const region of layout.regions) {
        if (region.chunks.length === 0 && region.storedCount === 0) continue;
        const r = region.record ?? { shape: 0, direction: 0, flags: 0, positionX: 0, positionY: 0 };
        lines.push(`T ${region.territory} shape=${r.shape} dir=${r.direction} flags=${r.flags} pos=${r.positionX},${r.positionY} stored=${region.storedCount} chunks=${region.chunks.map((c) => `${c.x},${c.y}${c.side};`).join('')}`);
        for (const placed of region.chunks) if (placed.stored) collect(placed.x, placed.y, placed.stored);
    }
    lines.push(`O ${objects.length} ${crypto.createHash('sha256').update(`${objects.join('\n')}\n`, 'ascii').digest('hex')}`);
    return lines;
}

// 원본 `.fort` 465개 전체의 영역 배치가 독립 구현의 기대값과 줄 단위로 같고, 놓인 청크 수가 저장 청크 레코드 수와 같으며 지도 밖·프레임 없음 칸이 없다. (2026-11-01 WB03-V)
test('원본 .fort 465개: 영역 배치와 저장 청크 짝짓기가 기대값 표와 같다', (context) => {
    const forts = loadOriginalForts(archive);
    if (!forts) {
        context.skip('저장소 자산(assets/game-data)이 없다');
        return;
    }
    const expectedFile = path.join(WEBPJ_ROOT, 'tests', 'fixtures', 'territory-expected.tsv');
    // 줄바꿈이 CRLF 로 바뀐 체크아웃(core.autocrlf=true)에서도 읽도록 CR 을 허용한다.
    const expected = fs.readFileSync(expectedFile, 'utf8').split(/\r?\n/).filter((line) => line !== '' && !line.startsWith('#'));
    const actual = [];
    let placedRegions = 0;
    let mismatched = 0;
    for (const { source, bytes } of forts) {
        const fort = parseFort(bytes, types);
        const layout = layoutFort(fort, frameCodes);
        actual.push(...describeLayout(source, fort, layout));
        assert.equal(layout.chaff.length, WORLD_CHUNKS_PER_SIDE * WORLD_CHUNKS_PER_SIDE, `${source}: Chaff 는 월드 청크 256개를 모두 담는다`);
        assert.equal(layout.builder.clampedCells, 0, `${source}: 지도 밖으로 나간 칸`);
        assert.equal(layout.builder.missingFrames, 0, `${source}: 프레임을 찾지 못한 칸`);
        for (const region of layout.regions) {
            if (region.chunks.length > 0) placedRegions += 1;
            if (region.chunks.length > 0 && !region.matches) mismatched += 1;
        }
    }
    assert.equal(forts.length, 465);
    assert.equal(mismatched, 0, '놓인 청크 수와 저장 청크 레코드 수가 다른 영역');
    assert.ok(placedRegions > 2000, `놓인 영역 ${placedRegions}개`);
    assert.equal(actual.length, expected.length, '줄 수');
    const firstDifference = actual.findIndex((line, index) => line !== expected[index]);
    assert.equal(firstDifference, -1, firstDifference < 0 ? '' : `줄 ${firstDifference + 1}:\n실제 ${actual[firstDifference]}\n기대 ${expected[firstDifference]}`);
});

// Save the Island! 의 알려진 월드 좌표(docs/exe/territory-layout.md, 원본 캡처의 신전·사제·포대와 일치): rainVortex (187,108), priest (185,118), Chaff 의 rainBattery (193,106).
// 플레이어 영역 Terr07 은 모양 7, 위치 0x59 → 월드 청크 (10,6)(11,6)(10,7)(11,7). (2026-11-01 WB03-V)
test('Save the Island!: 알려진 월드 좌표와 플레이어 영역 청크', () => {
    const fort = parseFort(archive.read('\\d\\savetheisland.fort'), types);
    const layout = layoutFort(fort, frameCodes);
    const player = layout.regions[7];
    assert.deepEqual(player.chunks.map((chunk) => `${chunk.x},${chunk.y}`), ['10,6', '11,6', '10,7', '11,7']);
    assert.equal(player.record.shape, 7);
    assert.equal(player.record.positionX, 9);
    assert.equal(player.record.positionY, 5);
    assert.ok(player.matches);
    // 영역 오브젝트의 월드 좌표를 타입 이름으로 모은다.
    const territoryObjects = [];
    for (const region of layout.regions) {
        for (const placed of region.chunks) {
            for (const object of placed.stored?.objects ?? []) {
                if (object.type !== 0) territoryObjects.push({ name: types.records[object.type].assetName, ...worldCell(placed.x, placed.y, object) });
            }
        }
    }
    assert.ok(territoryObjects.some((o) => o.name === 'rainVortex' && o.x === 187 && o.y === 108), 'rainVortex (187,108)');
    assert.ok(territoryObjects.some((o) => o.name === 'priest' && o.x === 185 && o.y === 118), 'priest (185,118)');
    // Chaff 의 청크 (12,6)에 있는 rainBattery 는 월드 (193,106) 이다.
    const chaffBattery = layout.chaff.flatMap((entry) => entry.chunk.objects.filter((o) => o.type !== 0).map((o) => ({ name: types.records[o.type].assetName, ...worldCell(entry.x, entry.y, o) })));
    assert.ok(chaffBattery.some((o) => o.name === 'rainBattery' && o.x === 193 && o.y === 106), 'Chaff rainBattery (193,106)');
    assert.equal(layout.chaff.length, WORLD_CHUNKS_PER_SIDE * WORLD_CHUNKS_PER_SIDE);
    assert.equal(DEFAULT_CHUNK_RECT.left, 1);
    assert.equal(DEFAULT_CHUNK_RECT.top, 1);
});
