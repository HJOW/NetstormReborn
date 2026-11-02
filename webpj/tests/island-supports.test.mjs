// 작은 받침 찾기(island-supports.js)를 검사한다: 서쪽·북쪽 칸의 글자로 자기 글자를 정하는 규칙(원본 noIsland postPop), 읽는 순서에 따른 결과, 3 × 3 완전한 받침 판정,
// 타입 번호·발자국 상수가 실제 타입 표와 같은지, 그리고 원본 `.fort` 465개 전체에서 칸 수·받침 수·불완전한 받침·겹친 오브젝트·받침 목록 해시가
// 독립 구현(Python 판독기 + 같은 규칙을 따로 구현, webpj/tools/make-island-supports-expected.py)의 기대값과 줄 단위로 같은지 본다.
// (2026-11-02 WB03-Y: 정적 맵 화면(M2) 3단계(c) 작은 받침 추가)
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
import {
    ISLAND_TYPE, NO_ISLAND_TYPE, STALAG_TYPE, SUPPORT_ROWS, SUPPORT_SIZE, collectNoIslandObjects, findSupports, supportLetter,
} from '../www/assets/island-supports.js';

// 공용 자료이다: 아카이브·타입 표·`.fort` 판독용 타입 정보. (2026-11-02 WB03-Y)
const archive = new TaffArchive(readData('netstorm.tarc'));
const table = TypeTable.fromArchive(archive, decodeOriginalText);
const types = createFortTypes(table);
const pieceCodes = pieceFrameCodes(table);

// 기대값 표(Python)이다: 출처 → { cells, supports, incomplete, duplicates, digest, owners }. 없으면 해당 검사를 건너뛴다. (2026-11-02 WB03-Y)
const EXPECTED_FIXTURE = path.join(WEBPJ_ROOT, 'tests', 'fixtures', 'island-supports-expected.tsv');
const expected = fs.existsSync(EXPECTED_FIXTURE)
    ? new Map(fs.readFileSync(EXPECTED_FIXTURE, 'utf8').split(/\r?\n/).filter((line) => line !== '' && !line.startsWith('#')).map((line) => line.split('\t'))
        .map((row) => [row[0], { cells: Number(row[1]), supports: Number(row[2]), incomplete: Number(row[3]), duplicates: Number(row[4]), digest: row[5], owners: row[6] }]))
    : new Map();

// 합성 받침 칸 모음이다: 왼쪽 위 (x, y) 에서 3 × 3 을 행 우선으로 만든 noIsland 칸 목록(소유자 owner). (2026-11-02 WB03-Y)
function block(x, y, owner = 1, territory = -1) {
    const cells = [];
    // 행 우선으로 9칸을 만든다.
    for (let dy = 0; dy < 3; dy += 1) for (let dx = 0; dx < 3; dx += 1) cells.push({ x: x + dx, y: y + dy, storedOwner: owner, storedFrame: null, territory });
    return cells;
}

test('타입 번호와 발자국 상수가 실제 타입 표와 같다', () => {
    assert.equal(table.find('noIsland'), NO_ISLAND_TYPE);
    assert.equal(table.find('island'), ISLAND_TYPE);
    assert.equal(table.find('islandStalag'), STALAG_TYPE);
    const island = table.definitions[ISLAND_TYPE - 70];
    const property = (name) => island.properties.find((item) => item.name.toLowerCase() === name)?.value;
    assert.equal(property('foot_x'), SUPPORT_SIZE);
    assert.equal(property('foot_y'), SUPPORT_SIZE);
    // noIsland 의 클러스터에 받침 글자 9개(F C G B A D I E H)가 `P` 변형 번호 1 로 모두 있다.
    const noIsland = table.definitions[NO_ISLAND_TYPE - 70];
    for (const letter of SUPPORT_ROWS.join('')) {
        assert.ok(noIsland.clusters.some((cluster) => cluster.code.side === letter.charCodeAt(0) && cluster.code.variant === 0x50 && cluster.code.number === 1), `noIsland ${letter}01`);
    }
});

test('칸 글자 규칙: 서쪽이 우선이고 그다음 북쪽이며 이웃이 없으면 F 이다', () => {
    const west = { C: 'G', F: 'C', A: 'D', B: 'A', E: 'H', I: 'E' };
    // 서쪽 글자 6가지는 북쪽과 상관없이 정해진다.
    for (const [from, to] of Object.entries(west)) for (const north of [null, 'F', 'B', 'A']) assert.equal(supportLetter(from, north), to);
    // 서쪽이 없거나 규칙에 없는 글자(G, D, H)면 북쪽을 본다: F → B, B → I, 그 밖은 F.
    for (const from of [null, 'G', 'D', 'H']) {
        assert.equal(supportLetter(from, 'F'), 'B');
        assert.equal(supportLetter(from, 'B'), 'I');
        assert.equal(supportLetter(from, 'A'), 'F');
        assert.equal(supportLetter(from, null), 'F');
    }
});

test('행 우선으로 만든 3 × 3 은 F C G / B A D / I E H 가 되고 윗면 기준은 오른쪽 아래 칸이다', () => {
    const result = findSupports(block(10, 20, 3));
    assert.deepEqual(result.cells.map((cell) => cell.letter).join(''), SUPPORT_ROWS.join(''));
    assert.equal(result.supports.length, 1);
    assert.deepEqual(result.supports[0], { x: 10, y: 20, anchorX: 12, anchorY: 22, owner: 3, territory: -1, complete: true, missing: 0 });
    assert.equal(result.orphanCells, 0);
});

test('받침 둘이 붙어 있어도 읽는 순서대로 경계가 나뉜다', () => {
    // 가로로 붙은 두 받침 (10,20) 과 (13,20): 두 번째 받침의 첫 칸 (13,20) 의 서쪽은 G(12,20) 이다 — 규칙에 없는 글자라 북쪽(없음)을 보아 F 가 된다.
    const together = findSupports([...block(10, 20, 1), ...block(13, 20, 2)]);
    assert.equal(together.supports.length, 2);
    assert.ok(together.supports.every((support) => support.complete));
    assert.deepEqual(together.supports.map((support) => support.owner), [1, 2]);
    // 세로로 붙은 두 받침 (10,20) 과 (10,23): 두 번째 블록의 첫 칸 북쪽은 I 라 F 가 된다.
    const stacked = findSupports([...block(10, 20), ...block(10, 23)]);
    assert.equal(stacked.supports.length, 2);
    assert.ok(stacked.supports.every((support) => support.complete));
});

test('읽는 순서가 바뀌면 글자가 달라진다(이웃이 아직 없으면 F)', () => {
    // 오른쪽 칸 (11,20) 을 왼쪽 칸 (10,20) 보다 먼저 만들면 (11,20) 은 서쪽이 없어 F 가 되고, (10,20) 은 서쪽이 없어 F 이다.
    const result = findSupports([{ x: 11, y: 20, storedOwner: 1, storedFrame: null, territory: -1 }, { x: 10, y: 20, storedOwner: 1, storedFrame: null, territory: -1 }]);
    assert.deepEqual(result.cells.map((cell) => cell.letter), ['F', 'F']);
    assert.equal(result.supports.length, 2);
    assert.ok(result.supports.every((support) => !support.complete && support.missing > 0));
});

test('불완전한 받침·월드 가장자리·같은 칸의 겹침', () => {
    // 가운데 칸 하나가 빠진 받침.
    const partial = block(10, 20).filter((cell) => !(cell.x === 11 && cell.y === 21));
    const incomplete = findSupports(partial);
    // 가운데 칸이 없으면 (12,21) 의 서쪽이 비고 북쪽이 G 라 규칙에 없어 이 칸도 F 가 되어 받침이 둘로 읽힌다(둘 다 불완전). 첫 받침은 2칸이, 둘째는 8칸이 모자란다.
    assert.deepEqual(incomplete.supports.map((support) => [support.x, support.y, support.complete, support.missing]), [[10, 20, false, 2], [12, 21, false, 8]]);
    // 왼쪽 가장자리 x = 0 의 받침: 이전 줄 오른쪽 끝 칸을 서쪽으로 잘못 읽지 않는다.
    const edge = findSupports([{ x: 255, y: 19, storedOwner: 1, storedFrame: null, territory: -1 }, ...block(0, 20)]);
    assert.equal(edge.cells[1].letter, 'F');
    assert.ok(edge.supports.some((support) => support.x === 0 && support.complete));
    // 같은 칸에 둘째 오브젝트가 겹치면 받침을 하나 더 만들지 않는다.
    const duplicate = findSupports([...block(10, 20), { x: 10, y: 20, storedOwner: 9, storedFrame: null, territory: -1 }]);
    assert.equal(duplicate.supports.length, 1);
    assert.equal(duplicate.cells.length, 10);
    assert.equal(duplicate.letters.size, 9);
    // 둘째 오브젝트는 표면 지도에 남지 않으므로 첫 오브젝트가 정한 글자가 유지된다: (11,20) 을 서쪽 (10,20) 보다 먼저 만들면 F 이고, 뒤늦게 겹친 오브젝트의 글자(C)로 덮어쓰지 않는다.
    const object = (x, y) => ({ x, y, storedOwner: 1, storedFrame: null, territory: -1 });
    const late = findSupports([object(11, 20), object(10, 20), object(11, 20)]);
    assert.equal(late.letters.get(20 * 256 + 11), 'F');
    assert.equal(late.cells[2].letter, 'C');
    assert.equal(late.supports.length, 2);
    // 마지막 칸 하나만 빠진 받침은 완전하지 않다.
    const lacking = findSupports(block(10, 20).slice(0, 8));
    assert.deepEqual(lacking.supports.map((support) => [support.x, support.y, support.complete, support.missing]), [[10, 20, false, 1]]);
});

test('collectNoIslandObjects: Chaff 의 청크 다음 영역 순서로 월드 칸 좌표를 붙여 모은다', () => {
    const object = (type, cellX, cellY, owner) => ({ type, cellX, cellY, owner, frame: null });
    const layout = {
        chaff: [{ x: 1, y: 2, chunk: { objects: [object(NO_ISLAND_TYPE, 3, 4, 2), object(NO_ISLAND_TYPE + 1, 0, 0, 1)] } }, { x: 2, y: 2, chunk: null }],
        regions: [
            { territory: 0, chunks: [{ x: 5, y: 6, stored: { objects: [object(NO_ISLAND_TYPE, 15, 15, 4)] } }, { x: 6, y: 6, stored: null }] },
            { territory: 3, chunks: [{ x: 0, y: 0, stored: { objects: [object(NO_ISLAND_TYPE, 0, 1, null)] } }] },
        ],
    };
    assert.deepEqual(collectNoIslandObjects(layout), [
        { x: 19, y: 36, storedOwner: 2, storedFrame: null, territory: -1 },
        { x: 95, y: 111, storedOwner: 4, storedFrame: null, territory: 0 },
        { x: 0, y: 1, storedOwner: null, storedFrame: null, territory: 3 },
    ]);
});

// 원본 `.fort` 465개의 { source, 결과 } 이다(처음 부를 때 한 번만 만들고 재사용한다). 저장소 자산이 없으면 null 이다. (2026-11-02 WB03-Y)
let resultCache;
function allSupports() {
    if (resultCache === undefined) {
        const forts = loadOriginalForts(archive);
        resultCache = forts ? forts.map(({ source, bytes }) => ({ source, result: findSupports(collectNoIslandObjects(layoutFort(parseFort(bytes, types), pieceCodes))) })) : null;
    }
    return resultCache;
}

test('원본 .fort 465개: 칸 수·받침 수·불완전한 받침·겹친 오브젝트·받침 목록 해시·소유자별 수가 독립 구현과 줄 단위로 같다', { skip: expected.size === 0 }, () => {
    const all = allSupports();
    if (!all) return;
    let maps = 0;
    let cells = 0;
    let supports = 0;
    let incomplete = 0;
    // 파일마다 기대 줄과 대조한다.
    for (const { source, result } of all) {
        const row = expected.get(source);
        assert.ok(row, `기대값 줄이 없다: ${source}`);
        const lines = result.supports.map((support) => `${support.x},${support.y},${support.owner === null ? 'none' : support.owner},${support.missing}`);
        const owners = new Map();
        for (const support of result.supports) {
            const key = support.owner === null ? 'none' : String(support.owner);
            owners.set(key, (owners.get(key) ?? 0) + 1);
        }
        assert.equal(result.cells.length, row.cells, `칸 ${source}`);
        assert.equal(result.supports.length, row.supports, `받침 ${source}`);
        assert.equal(result.supports.filter((support) => !support.complete).length, row.incomplete, `불완전 ${source}`);
        assert.equal(result.cells.length - result.letters.size, row.duplicates, `겹침 ${source}`);
        assert.equal(crypto.createHash('sha256').update(`${lines.join('\n')}\n`).digest('hex'), row.digest, `해시 ${source}`);
        assert.equal([...owners.keys()].sort().map((key) => `${key}:${owners.get(key)}`).join(','), row.owners, `소유자 ${source}`);
        maps += 1;
        cells += result.cells.length;
        supports += result.supports.length;
        incomplete += result.supports.filter((support) => !support.complete).length;
    }
    assert.equal(maps, 465);
    assert.equal(cells, 176093);
    assert.equal(supports, 19568);
    assert.equal(incomplete, 10);
});

test('공식 맵 32개와 느슨한 맵 대부분은 모든 noIsland 칸이 정확히 한 완전한 받침에 속한다', () => {
    const all = allSupports();
    if (!all) return;
    const official = all.filter(({ source }) => source.startsWith('tarc:'));
    assert.equal(official.length, 32);
    // 공식 맵은 전부 완전한 받침이고 칸 수 = 9 × 받침 수이며 겹침이 없다.
    for (const { source, result } of official) {
        assert.equal(result.cells.length, result.supports.length * 9, source);
        assert.ok(result.supports.every((support) => support.complete), source);
        assert.equal(result.letters.size, result.cells.length, source);
        assert.equal(result.orphanCells, 0, source);
    }
    // 문서의 두 공식 맵 값: Save the Island! 26개(234칸), The War Begins! 13개(117칸).
    const byName = (needle) => official.find(({ source }) => source.toLowerCase().includes(needle));
    assert.equal(byName('savetheisland').result.supports.length, 26);
    assert.equal(byName('savetheisland').result.cells.length, 234);
    assert.equal(byName('thewarbegins').result.supports.length, 13);
    assert.equal(byName('thewarbegins').result.cells.length, 117);
    // 불완전한 받침은 오브젝트가 겹친 사용자 제작 맵 5개에만 있다.
    const broken = all.filter(({ result }) => result.supports.some((support) => !support.complete)).map(({ source }) => source).sort();
    assert.deepEqual(broken, ['loose:MasterMind.fort', 'loose:Tester.fort', 'loose:WOF1.fort', 'loose:rebelhq.fort', 'loose:sw_m1.fort']);
});
