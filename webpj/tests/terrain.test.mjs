// 본섬 칸 마스크(terrain-mask.js)를 검사한다: 청크 지역 난수(BigInt 독립 계산), 방향 글자 ↔ 연결 비트 표, 합성 영역의 통로·핵심 덩어리·영역 번호,
// 원본 `.fort` 465개 전체(느슨한 파일 433 + 아카이브 32)의 마스크를 원본에서 따로 옮긴 Python 재현 도구(tools/terrain_mask.py)가 만든 기대값 표와 대조,
// 두 공식 맵의 문서 값(docs/exe/terrain-and-bridges.md: 마스크 SHA-256·영역별 칸 수)을 확인한다.
// 기대값 표는 webpj/tools/make-terrain-expected.py 가 만들었고 www/ 구현의 출력으로 다시 만들지 않는다.
// (2026-11-01 WB03-W: 정적 맵 화면(M2) 3단계의 첫 부분 추가)
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
import { ChunkMap, IslandBuilder, IslandList, layoutFort, pieceFrameCodes } from '../www/assets/territory.js';
import {
    EMPTY_TERRAIN, LocalRandom, NEIGHBOR_CELLS, WORLD_CELLS, buildIslandMask, connectionMask, fillMaskGaps, orientationLetter, territoryCellCounts,
} from '../www/assets/terrain-mask.js';

// 공용 자료이다: 아카이브·타입 표·`.fort` 판독용 타입 정보·영역 패턴 타입의 프레임 코드. (2026-11-01 WB03-W)
const archive = new TaffArchive(readData('netstorm.tarc'));
const table = TypeTable.fromArchive(archive, decodeOriginalText);
const types = createFortTypes(table);
const frameCodes = pieceFrameCodes(table);

// 영역 레코드 6바이트를 만든다: 모양 번호·방향 값·플래그·위치 x(하위 니블)·위치 y(상위 니블)·시드(넷째 바이트)·남는 두 바이트. (2026-11-01 WB03-W)
function record(shape, flags, x, y, seed) {
    return Uint8Array.of(shape, flags, (y << 4) | x, seed, 0, 0);
}

// 합성 영역 레코드를 번호 위치에 놓아 청크 지도와 섬 목록을 만든다. records 는 [영역 번호, 레코드] 쌍이다. (2026-11-01 WB03-W)
function place(records) {
    const map = new ChunkMap();
    const islands = new IslandList();
    const builder = new IslandBuilder(map, islands, frameCodes, 1);
    // 번호순으로 놓는다(IslandBuilder.place 는 호출 순서가 섬 번호이다).
    for (const [territory, value] of records) builder.place(value, territory);
    return { map, islands };
}

// SHA-256 16진수 글을 돌려준다. (2026-11-01 WB03-W)
function sha256(bytes) {
    return crypto.createHash('sha256').update(bytes).digest('hex');
}

// ----- 난수와 방향 표 -----

// 청크 지역 난수: 시드 0 은 0x0bad0bad 로 바꾸고 다음 상태는 `상태 × 0x10003 + 3` 의 32비트 값, 출력은 상위 16비트를 limit 로 나눈 나머지이다. BigInt 로 독립 계산해 대조한다. (2026-11-01 WB03-W)
test('LocalRandom: BigInt 독립 계산과 같다', () => {
    const reference = (seed, limits) => {
        let state = BigInt(seed) & 0xffffffffn;
        const out = [];
        // 한 번에 하나씩 전진해 출력을 모은다.
        for (const limit of limits) {
            if (state === 0n) state = 0x0bad0badn;
            state = (state * 0x10003n + 3n) & 0xffffffffn;
            out.push(Number((state >> 16n) % BigInt(limit)));
        }
        return out;
    };
    const limits = [30, 15, 15, 1, 30, 15, 2, 7, 1000, 65536, 65535, 15, 15, 1];
    // 작은 시드와 32비트 경계 근처의 시드(부호 있는 곱셈으로 틀리기 쉬운 값)를 확인한다.
    for (const seed of [0, 1, 0x10e3, 0x25 + 0x10e3 * 7, 0x7fffffff, 0x80000000, 0xdeadbeef, 0xffffffff, 0x0bad0bad]) {
        const random = new LocalRandom(seed);
        assert.deepEqual(limits.map((limit) => random.next(limit)), reference(seed, limits), `시드 ${seed}`);
    }
    // 시드 0 은 0x0bad0bad 와 같은 수열이다. 음수 시드는 32비트로 감긴다.
    assert.equal(new LocalRandom(0).next(30), new LocalRandom(0x0bad0bad).next(30));
    assert.equal(new LocalRandom(-1).state, 0xffffffff);
    // 상태가 0 이 된 뒤에도 0x0bad0bad 로 바꾼다: 상태를 직접 0 으로 두면 시드 0 과 같은 결과이다.
    const zero = new LocalRandom(5);
    zero.state = 0;
    assert.equal(zero.next(30), new LocalRandom(0).next(30));
    // limit 1 도 상태를 전진시킨다. limit 가 1 미만이면 오류이다.
    const one = new LocalRandom(123);
    const before = one.state;
    assert.equal(one.next(1), 0);
    assert.notEqual(one.state, before);
    assert.throws(() => one.next(0), /positive/);
    assert.throws(() => one.next(-3), /positive/);
    assert.throws(() => one.next(Number.NaN), /positive/);
});

// 방향 글자 → 연결 비트 표(원본 0x52f910)는 문서의 값이고, 연결 비트 → 글자 표(0x52f8fc)와 서로 역이다(따로 추출된 두 표의 일관성). 범위 밖 글자는 오류이다. (2026-11-01 WB03-W)
test('connectionMask·orientationLetter: 문서 표와 서로 역', () => {
    const expected = [15, 7, 14, 13, 11, 6, 12, 9, 3, 5, 10, 4, 8, 1, 2, 0];
    expected.forEach((mask, i) => assert.equal(connectionMask(String.fromCharCode(0x41 + i)), mask, `글자 ${String.fromCharCode(0x41 + i)}`));
    // 글자 → 비트 → 글자가 제자리이다.
    for (let i = 0; i < 16; i += 1) {
        const letter = String.fromCharCode(0x41 + i);
        assert.equal(orientationLetter(connectionMask(letter)), letter);
    }
    assert.equal([...Array(16).keys()].map(orientationLetter).join(''), 'PNOILJFBMHKEGDCA');
    // 비트는 하위 4비트만 본다.
    assert.equal(orientationLetter(16 + 15), 'A');
    for (const bad of ['@', 'Q', 'a', '', 'AB', 1, null, undefined]) assert.throws(() => connectionMask(bad), /Invalid terrain side/, String(bad));
    assert.equal(NEIGHBOR_CELLS.length, 8);
    // 이웃은 북부터 시계 방향이고 짝수 번호는 직선이다.
    assert.deepEqual(NEIGHBOR_CELLS.map(([dx, dy]) => `${dx},${dy}`), ['0,-1', '1,-1', '1,0', '1,1', '0,1', '-1,1', '-1,0', '-1,-1']);
    NEIGHBOR_CELLS.forEach(([dx, dy], i) => assert.equal(dx === 0 || dy === 0, i % 2 === 0));
});

// ----- 합성 영역 -----

// 합성 영역(모양 7 = 2 × 2 청크 'FGIH', 영역 번호 5, 시드 0x25): 마스크의 값은 섬 번호(0)가 아니라 영역 번호(5)이다. 같은 입력은 같은 마스크이고, 다른 시드는 다른 마스크이다. (2026-11-01 WB03-W)
test('buildIslandMask: 합성 영역의 영역 번호·반복성·시드', () => {
    const { map, islands } = place([[5, record(7, 9, 3, 3, 0x25)]]);
    const mask = buildIslandMask(map, islands);
    assert.equal(mask.length, WORLD_CELLS * WORLD_CELLS);
    assert.deepEqual(buildIslandMask(map, islands), mask);
    const counts = territoryCellCounts(mask);
    assert.deepEqual(Object.keys(counts), ['5'], '섬 번호가 아니라 영역 번호가 들어간다');
    assert.ok(counts[5] > 4 * 16, `칸 수 ${counts[5]}`);
    assert.ok([...mask].every((value) => value === 5 || value === EMPTY_TERRAIN));
    // 시드 바이트가 다르면 중심·성장이 달라진다.
    const other = place([[5, record(7, 9, 3, 3, 0x26)]]);
    assert.notDeepEqual(buildIslandMask(other.map, other.islands), mask);
    // 놓은 영역이 없으면 지면이 없다.
    assert.deepEqual(territoryCellCounts(buildIslandMask(new ChunkMap(), new IslandList())), {});
});

// 각 청크의 중심 4 × 4칸(중심 (4 + (시드 & 7), 4 + ((9999 - 시드) & 7)) 둘레)과 연결 비트가 가리키는 쪽의 폭 2칸 통로, 두 번째 청크의 10 × 10칸이 채워진다. 독립 계산한 위치로 확인한다. (2026-11-01 WB03-W)
test('buildIslandMask: 중심 덩어리·통로·두 번째 청크의 10 × 10칸', () => {
    // 모양 6('OCM.N.', 3 × 2) 한 영역: 청크는 y 바깥·x 안쪽으로 (1,1)O (2,1)C (3,1)M (2,2)N 이다. 시드는 모두 같다.
    const seed = 0x0d;
    const { map, islands } = place([[2, record(6, 9, 0, 0, seed)]]);
    const mask = buildIslandMask(map, islands);
    const at = (x, y) => mask[y * WORLD_CELLS + x];
    const chunks = [[1, 1, 'O'], [2, 1, 'C'], [3, 1, 'M'], [2, 2, 'N']];
    // 중심 오프셋은 청크 시작에 (4 + (시드 & 7), 4 + ((9999 - 시드) & 7))을 더한 것이다.
    const centerX = 4 + (seed & 7);
    const centerY = 4 + ((9999 - seed) & 7);
    chunks.forEach(([cx, cy, side], i) => {
        const left = cx * 16;
        const top = cy * 16;
        // 핵심 덩어리: 중심에서 (-1..+2) 4 × 4칸.
        for (let y = top + centerY - 1; y < top + centerY + 3; y += 1) {
            for (let x = left + centerX - 1; x < left + centerX + 3; x += 1) assert.equal(at(x, y), 2, `청크 ${cx},${cy} 핵심 ${x},${y}`);
        }
        // 통로: 연결 비트(북 1·동 2·남 4·서 8)가 가리키는 쪽으로 청크 가장자리까지이다. 원본은 사각형 하나를 채운다 —
        // 왼쪽은 서쪽 비트가 있으면 청크 왼쪽 끝, 아니면 중심 x / 위쪽은 북쪽 비트가 있으면 청크 위 끝, 아니면 중심 y / 오른쪽은 동쪽 비트가 있으면 청크 오른쪽 끝, 아니면 중심 x + 2 / 아래쪽은 남쪽 비트가 있으면 청크 아래 끝, 아니면 중심 y + 2.
        // 그래서 두 방향이 켜지면 모서리 쪽 사각형이 통째로 채워진다.
        const bits = connectionMask(side);
        const x0 = left + ((bits & 8) !== 0 ? 0 : centerX);
        const x1 = left + ((bits & 2) !== 0 ? 16 : centerX + 2);
        const y0 = top + ((bits & 1) !== 0 ? 0 : centerY);
        const y1 = top + ((bits & 4) !== 0 ? 16 : centerY + 2);
        for (let y = y0; y < y1; y += 1) for (let x = x0; x < x1; x += 1) assert.equal(at(x, y), 2, `${side} 통로 ${x},${y}`);
        // 영역 안 두 번째 청크(i === 1, y·x 순서로 (2,1))는 (3..12) 10 × 10칸이 모두 채워진다.
        if (i === 1) {
            for (let y = top + 3; y < top + 13; y += 1) for (let x = left + 3; x < left + 13; x += 1) assert.equal(at(x, y), 2, `두 번째 청크 ${x},${y}`);
        }
    });
});

// ----- 틈 보정(손으로 그린 마스크) -----

// 빈 마스크를 만들고, 영역 번호 owner 의 칸을 [x, y] 목록으로 채운다. (2026-11-01 WB03-W)
function drawn(owner, cells) {
    const land = new Uint8Array(WORLD_CELLS * WORLD_CELLS).fill(EMPTY_TERRAIN);
    for (const [x, y] of cells) land[y * WORLD_CELLS + x] = owner;
    return land;
}

// 칸 (cx, cy)의 여덟 이웃 중 북부터 시계 방향 번호(0~7, NEIGHBOR_CELLS 의 번호)의 목록에 해당하는 좌표들이다. (2026-11-01 WB03-W)
function around(cx, cy, directions) {
    return directions.map((direction) => [cx + NEIGHBOR_CELLS[direction][0], cy + NEIGHBOR_CELLS[direction][1]]);
}

// 틈 보정의 채움 조건: 여덟 이웃에서 같은 영역이 5칸 넘게 이어지고 그 순간의 방향 번호가 홀수일 때만 채운다. 북~남서 6칸(방향 5, 홀수)은 채우고, 북~남 5칸(방향 4, 짝수)·북~남동 4칸은 채우지 않는다.
// 북서에서 북·북동·동·남동으로 한 바퀴 돌아 이어지는 5칸(방향 11)도 채운다. (2026-11-01 WB03-W)
test('fillMaskGaps: 5칸 넘는 연속·홀수 방향·한 바퀴 이어짐', () => {
    const filled = (cells, owner = 3) => {
        const land = drawn(owner, cells);
        fillMaskGaps(land);
        return land[100 * WORLD_CELLS + 100];
    };
    assert.equal(filled(around(100, 100, [0, 1, 2, 3, 4, 5])), 3, '북~남서 6칸');
    assert.equal(filled(around(100, 100, [0, 1, 2, 3, 4])), EMPTY_TERRAIN, '북~남 5칸(마지막 방향이 짝수)');
    assert.equal(filled(around(100, 100, [0, 1, 2, 3])), EMPTY_TERRAIN, '북~남동 4칸');
    assert.equal(filled(around(100, 100, [7, 0, 1, 2, 3])), 3, '북서~남동 5칸(한 바퀴 이어짐, 방향 11)');
    assert.equal(filled(around(100, 100, [1, 2, 3, 4, 5])), 3, '북동~남서 5칸(방향 5)');
    assert.equal(filled(around(100, 100, [0, 2, 4, 6])), EMPTY_TERRAIN, '직선 이웃 넷만');
    assert.equal(filled(around(100, 100, [0, 1, 2, 3, 4, 5, 6, 7]), 9), 9, '여덟 이웃 모두: 소유자는 이웃의 영역 번호');
});

// 소유자가 바뀌면 그 이웃은 새 연속에 세지 않고 연속이 끊긴다(원본 그대로): 북·북동·동은 영역 3, 남동~북서 다섯 칸은 영역 4 인데 남동이 세어지지 않아 채우지 않는다.
// 동쪽이 비어 있으면 남동부터 새로 시작해 다섯 칸(방향 7)이 되어 영역 4 로 채운다. (2026-11-01 WB03-W)
test('fillMaskGaps: 소유자가 바뀌는 이웃은 새 연속에 세지 않는다', () => {
    const run = (first, second) => {
        const land = drawn(3, first);
        for (const [x, y] of second) land[y * WORLD_CELLS + x] = 4;
        fillMaskGaps(land);
        return land[100 * WORLD_CELLS + 100];
    };
    assert.equal(run(around(100, 100, [0, 1, 2]), around(100, 100, [3, 4, 5, 6, 7])), EMPTY_TERRAIN);
    assert.equal(run(around(100, 100, [0, 1]), around(100, 100, [3, 4, 5, 6, 7])), 4, '북동 뒤 동이 비면 남동부터 새 연속');
});

// 월드 가장자리: 처리 범위는 (1..254)이다. 위쪽 y=1, 왼쪽 x=1, 오른쪽 x=254, 아래쪽 y=254 의 빈 칸도 채우고, 가장자리 줄(0, 255)은 채우지 않는다. (2026-11-01 WB03-W)
test('fillMaskGaps: 월드 가장자리 칸의 처리 범위', () => {
    const six = [0, 1, 2, 3, 4, 5];
    for (const [x, y] of [[100, 1], [1, 100], [254, 100], [100, 254], [1, 1], [254, 254]]) {
        const land = drawn(3, around(x, y, six));
        fillMaskGaps(land);
        assert.equal(land[y * WORLD_CELLS + x], 3, `칸 ${x},${y}`);
    }
    // 가장자리 줄은 건드리지 않는다: 위쪽 줄 y=0 의 빈 칸에 아래 이웃만 있어도 채우지 않는다.
    for (const [x, y] of [[100, 0], [0, 100], [255, 100], [100, 255]]) {
        const land = drawn(3, [0, 1, 2, 3, 4, 5, 6, 7].map((d) => [x + NEIGHBOR_CELLS[d][0], y + NEIGHBOR_CELLS[d][1]]).filter(([cx, cy]) => cx >= 0 && cy >= 0 && cx < WORLD_CELLS && cy < WORLD_CELLS));
        fillMaskGaps(land);
        assert.equal(land[y * WORLD_CELLS + x], EMPTY_TERRAIN, `가장자리 칸 ${x},${y}`);
    }
});

// 채운 칸은 곧바로 이어지는 검사에 반영되고, 첫 순회에서 못 채운 앞쪽 칸은 둘째 순회에서 채운다: A(50,50)는 남동 이웃 B(51,51)가 비어 첫 순회에서는 못 채우고,
// B 가 먼저 채워진 뒤 둘째 순회에서 채운다. 순회를 1번만 하면 A 가 비어 있다. (2026-11-01 WB03-W)
test('fillMaskGaps: 순회 횟수와 즉시 반영', () => {
    const cells = [[50, 49], [51, 49], [51, 50], [50, 51], [49, 51], [52, 50], [52, 51], [52, 52], [51, 52], [50, 52]];
    const at = (land, x, y) => land[y * WORLD_CELLS + x];
    const once = drawn(3, cells);
    fillMaskGaps(once, 1);
    assert.equal(at(once, 51, 51), 3, 'B 는 첫 순회에서 채워진다');
    assert.equal(at(once, 50, 50), EMPTY_TERRAIN, 'A 는 첫 순회에서 못 채운다');
    const twice = drawn(3, cells);
    fillMaskGaps(twice);
    assert.equal(at(twice, 51, 51), 3);
    assert.equal(at(twice, 50, 50), 3, 'A 는 둘째 순회에서 채워진다');
    // 순회 0번은 아무것도 바꾸지 않는다.
    const none = drawn(3, cells);
    fillMaskGaps(none, 0);
    assert.deepEqual(none, drawn(3, cells));
});

// ----- 성장 진단(trace) -----

// 성장은 목표량을 채우거나 연속 1000회 실패하면 끝난다: 목표를 못 채우고 끝난 청크는 마지막 성공 뒤 정확히 1000번(성공이 없었으면 1000번) 시도했다.
// 원본 `.fort` 465개에서 그렇게 끝난 청크가 실제로 있고, 목표를 채운 청크는 남은 양이 0 이하이다. (2026-11-01 WB03-W)
test('buildIslandMask: 성장은 연속 실패 1000회에서 끝난다(trace)', (context) => {
    const forts = loadOriginalForts(archive);
    if (!forts) {
        context.skip('저장소 자산(assets/game-data)이 없다');
        return;
    }
    let exhausted = 0;
    let completed = 0;
    for (const { source, bytes } of forts) {
        const layout = layoutFort(parseFort(bytes, types), frameCodes);
        const trace = {};
        buildIslandMask(layout.map, layout.islands, trace);
        const growth = trace.growth ?? [];
        assert.equal(growth.length, layout.regions.reduce((sum, region) => sum + region.chunks.length, 0), `${source}: 청크 수`);
        for (const entry of growth) {
            if (entry.remaining > 0) {
                exhausted += 1;
                assert.equal(entry.sinceSuccess, 1000, `${source} ${entry.x},${entry.y}: 마지막 성공 뒤 시도 횟수`);
            } else {
                completed += 1;
                assert.ok(entry.attempts >= 1 && entry.sinceSuccess < 1000, `${source} ${entry.x},${entry.y}`);
            }
            assert.ok(entry.target >= 51, `${source}: 목표량 최소 ${entry.target}`);
        }
    }
    assert.ok(exhausted > 0, '연속 실패로 끝난 청크가 있어야 상수가 검사된다');
    assert.ok(completed > 0, '목표를 채운 청크가 있어야 한다');
});

// ----- 원본 .fort 전체 -----

// 원본 `.fort` 465개의 마스크가 독립 재현(Python)의 기대값과 같다: 마스크의 SHA-256, 지면 칸 수, 영역별 칸 수. 놓인 영역 번호 밖의 값은 없고 놓인 영역은 모두 지면이 있다. (2026-11-01 WB03-W)
test('원본 .fort 465개: 본섬 마스크가 기대값 표와 같다', (context) => {
    const forts = loadOriginalForts(archive);
    if (!forts) {
        context.skip('저장소 자산(assets/game-data)이 없다');
        return;
    }
    const expectedFile = path.join(WEBPJ_ROOT, 'tests', 'fixtures', 'terrain-expected.tsv');
    const expected = fs.readFileSync(expectedFile, 'utf8').split(/\r?\n/).filter((line) => line !== '' && !line.startsWith('#'));
    assert.equal(forts.length, 465);
    assert.equal(expected.length, forts.length);
    const mismatches = [];
    let totalCells = 0;
    forts.forEach(({ source, bytes }, index) => {
        const layout = layoutFort(parseFort(bytes, types), frameCodes);
        const mask = buildIslandMask(layout.map, layout.islands);
        const counts = territoryCellCounts(mask);
        const total = Object.values(counts).reduce((sum, value) => sum + value, 0);
        totalCells += total;
        const line = [source, sha256(mask), String(total), Object.keys(counts).map(Number).sort((a, b) => a - b).map((key) => `${key}:${counts[key]}`).join(',') || '-'].join('\t');
        if (line !== expected[index]) mismatches.push(`줄 ${index + 1}:\n실제 ${line}\n기대 ${expected[index]}`);
        // 마스크의 값은 놓인 영역의 번호뿐이고, 놓인 영역은 모두 지면이 있다.
        const placed = layout.regions.filter((region) => region.chunks.length > 0).map((region) => region.territory);
        assert.deepEqual(Object.keys(counts).map(Number).sort((a, b) => a - b), placed, `${source}: 마스크의 영역 번호`);
    });
    assert.deepEqual(mismatches.slice(0, 3), [], `불일치 ${mismatches.length}개`);
    assert.ok(totalCells > 500000, `지면 칸 합계 ${totalCells}`);
});

// 두 공식 맵은 문서의 값과 같다(docs/exe/terrain-and-bridges.md 본섬 마스크 대조): Save the Island! Terr00 1384·Terr07 698, The War Begins! Terr00 954·Terr01 551. (2026-11-01 WB03-W)
test('공식 맵 둘: 문서의 마스크 SHA-256 과 영역별 칸 수', () => {
    const cases = [
        ['\\d\\savetheisland.fort', '68e699f55ad34041f316f4466f6c33d2edb6807833a648462a64ebbc2c505cc8', { 0: 1384, 7: 698 }],
        ['\\d\\thewarbegins.fort', 'b24af563875ef935aa1053d521a6abf8490c4ddea840ac1845b43223fd0436d1', { 0: 954, 1: 551 }],
    ];
    for (const [name, hash, counts] of cases) {
        const layout = layoutFort(parseFort(archive.read(name), types), frameCodes);
        const mask = buildIslandMask(layout.map, layout.islands);
        assert.equal(sha256(mask), hash, name);
        assert.deepEqual(territoryCellCounts(mask), counts, name);
    }
});
