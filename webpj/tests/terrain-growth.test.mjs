// 실제 일반 초기 지형 성장(cpppj `o/RawTerrainGrowth`, 패치 004c10a0 외)의 웹 대응 `core/raw-terrain-growth.js` 를 검사한다(4-B, WB04-E292).
//  - 고정 관측 세 파일의 originals 행을 같은 입력에서 재생해 전체 code/region 65,536BYTE 두 배열(RLE)·caller 시드·진단 기록·반환 값이 같다:
//    `terraingrowth-x86.tsv` 54행(RunNormal), `terrain-seed-x86.tsv` First 111행·Scatter 78행(첫 raw+5 시드와 흩뿌리기, 진단 복귀 효과 포함),
//    `terrain-arch-helpers-x86.tsv` Grow 72행·Cell 771행·Sparse 12행·Clear 12행(군도 root 가 공유하는 성장/제거 helper).
//  - 계약: 성장이 같은 context 배열과 stamp 필드(mapping·lastSid·makePredictable)를 유지, 살아 있는 checking 의 호출마다 재조회와 선택 진단 장치 누락 거부,
//    일반 분기는 첫 시드 진단을 만들지 않음, 입력 종류/범위 거부.
//  CD(10.72) 행은 웹 목표(10.78) 밖이라 쓰지 않는다. 실제 게임·브라우저 실행이 아니다. (2026-11-11 WB04-E292)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows } from './raw-support.mjs';
import { ChunkMap } from '../www/assets/territory.js';
import { RawTerrainContext, TerrainContextGlobals } from '../www/core/raw-terrain-context.js';
import { RawTerrainGrowth } from '../www/core/raw-terrain-growth.js';

// 원본 관측 파일(originals 행만)을 읽는다. 파일이 없으면 null 이라 해당 검사를 건너뛴다. (2026-11-11 WB04-E292)
const rowsOf = (name) => readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', name))?.filter((row) => row[0] === 'originals') ?? null;
const GROWTH_ROWS = rowsOf('terraingrowth-x86.tsv');
const SEED_ROWS = rowsOf('terrain-seed-x86.tsv');
const ARCH_ROWS = rowsOf('terrain-arch-helpers-x86.tsv');

// 원본 RLE(`16진BYTE:길이` 쉼표 목록)를 전체 65,536BYTE 배열로 풀며 기대 지형을 새로 계산하지 않는다. (2026-11-11 WB04-E292)
function rle(text) {
    const result = new Uint8Array(65536);
    let length = 0;
    // 관측된 연속 BYTE 와 길이를 저장 순서 그대로 펼친다.
    for (const run of text.split(',')) {
        const [byte, count] = run.split(':');
        assert.ok(byte !== undefined && count !== undefined, 'RLE 필드 누락');
        const n = Number(count);
        assert.ok(length + n <= 65536, 'RLE 배열 범위 초과');
        result.fill(Number.parseInt(byte, 16), length, length + n);
        length += n;
    }
    assert.equal(length, 65536, 'RLE 전체 배열 길이 오류');
    return result;
}

// 두 배열이 같은지 검사하고, 다르면 첫 불일치 위치(x, y)를 메시지로 남긴다. (2026-11-11 WB04-E292)
function assertSameBytes(actual, expected, label) {
    if (Buffer.from(actual.buffer, actual.byteOffset, actual.length).equals(Buffer.from(expected.buffer, expected.byteOffset, expected.length))) return;
    const index = actual.findIndex((value, at) => value !== expected[at]);
    assert.fail(`${label}: 첫 불일치 x=${index >> 8} y=${index & 255} 실제 ${actual[index]} 기대 ${expected[index]}`);
}

// 진단 효과 없이 context 만 만드는 보조 함수이다. (2026-11-11 WB04-E292)
const noop = () => {};

// ───────────── RunNormal (terraingrowth) ─────────────

// 기대 지형 없이 입력 청크의 원본 섬 BYTE/방향/seed 만 만든다. 각 profile 의 명시 청크 좌표는 native 입력과 같다. (2026-11-11 WB04-E292)
function growthChunks(chunks, profile, seed) {
    const profiles = [[], [[3, 3]], [[3, 3], [4, 3], [3, 4], [4, 4]], [[3, 3], [4, 3], [3, 4], [4, 4]], [[3, 3], [4, 3], [5, 3], [3, 4], [4, 4], [5, 4]], [[3, 3], [4, 4]]];
    let index = 0;
    // 원본 입력의 현재 청크 필드만 채운다.
    for (const [x, y] of profiles[profile]) {
        const chunk = chunks.at(x, y);
        chunk.island = (profile === 3 || profile === 5) ? (x - 3) & 1 : 0;
        chunk.side = String.fromCharCode(65 + ((profile * 3 + index) % 16));
        chunk.record[3] = (seed + (x - 3) * 13 + (y - 3) * 19) & 0xff;
        index += 1;
    }
}

// 정상 54개 원본 입력의 전체 code/region 배열을 대조한다(두 x87 반복은 고유 검사에 더하지 않음). (2026-11-11 WB04-E292)
test('초기 지형 성장 RunNormal 은 관측한 전체 code/region 배열을 그대로 재생한다', { skip: GROWTH_ROWS === null }, () => {
    let checked = 0;
    // 모든 기대 배열은 actual PE fixture 에서 읽고 성장에는 같은 입력만 공급한다.
    for (const row of GROWTH_ROWS) {
        assert.equal(row.length, 6);
        const input = row[1].split(',').map(Number);
        const [profile, seed, selected] = input;
        assert.equal(input.length, 3);
        const globals = new TerrainContextGlobals();
        let assertions = 0;
        const context = new RawTerrainContext('patch1078', 1, selected, { left: 3, top: 3, right: profile === 4 ? 6 : 5, bottom: 5 }, globals, () => { assertions += 1; });
        const chunks = new ChunkMap();
        growthChunks(chunks, profile, seed);
        if (profile === 5) {
            context.codes[56 * 256 + 56] = 0x78;
            context.regions[56 * 256 + 56] = 3;
            context.codes[57 * 256 + 56] = 0x3d;
            context.regions[57 * 256 + 56] = 3;
        }
        new RawTerrainGrowth(context, chunks, globals).runNormal();
        assertSameBytes(context.codes, rle(row[3]), `codes ${row[1]}`);
        assertSameBytes(context.regions, rle(row[4]), `regions ${row[1]}`);
        assert.equal(Number(row[2]), seed);
        assert.equal(assertions, Number(row[5]));
        checked += 1;
    }
    assert.equal(checked, 54);
});

// 성장은 같은 context 를 갱신하고 mapping/alias/도장 마지막 SID 는 유지한다. (2026-11-11 WB04-E292)
test('초기 지형 성장은 같은 context 배열을 갱신하고 도장 필드를 유지한다', () => {
    const globals = new TerrainContextGlobals();
    const context = new RawTerrainContext('patch1078', 1, 127, { left: 3, top: 3, right: 5, bottom: 5 }, globals, noop);
    const chunks = new ChunkMap();
    growthChunks(chunks, 2, 7);
    const codes = context.codes;
    const regions = context.regions;
    context.map.lastSid.value = 123;
    const mapping = [...context.map.typeMapping];
    new RawTerrainGrowth(context, chunks, globals).runNormal();
    assert.equal(context.codes, codes);
    assert.equal(context.map.codes, codes);
    assert.equal(context.regions, regions);
    assert.equal(context.map.regions, regions);
    assert.equal(context.map.lastSid.value, 123);
    assert.equal(context.map.makePredictable, 1);
    assert.deepEqual([...context.map.typeMapping], mapping);
    // 실제로 지형이 만들어졌다(전체가 허공이 아님).
    assert.ok(codes.some((value) => value === 0x78));
});

// ───────────── 첫 시드와 흩뿌리기 (terrain-seed) ─────────────

// 입력 profile 의 청크 경계이며 성공/실패 순서를 계산하지 않는다. (2026-11-11 WB04-E292)
function seedBounds(profile) {
    if (profile === 4) return { left: -1, top: -1, right: 1, bottom: 1 };
    return { left: 3, top: 3, right: profile === 0 ? 3 : 5, bottom: 5 };
}

// Native 입력과 같은 청크 원시 바이트만 채우고 출력 지형은 생성하지 않는다(원본 +0 섬 BYTE 와 +5 record[3] 만 변경). (2026-11-11 WB04-E292)
function seedChunks(chunks, profile, seed) {
    const add = (x, y, island, value) => {
        const chunk = chunks.at(x, y);
        chunk.island = island;
        chunk.record[3] = value & 0xff;
    };
    if (profile === 2) { add(4, 3, 0, seed); add(3, 4, 1, seed + 19); }
    if (profile === 3) { add(3, 3, 255, seed); add(4, 3, 128, seed + 19); add(3, 4, 0, seed + 37); }
    if (profile === 4) add(0, 0, 0, seed);
    if (profile >= 5 && profile <= 9) { add(3, 3, 0, seed); add(4, 3, 1, seed + 13); add(3, 4, 127, seed + 19); add(4, 4, 255, seed + 37); }
}

// 선택한 원본 몸체(First/Scatter)만 실행하고 진단 복귀의 명시 효과만 적용한다. 기대값은 getter/난수 구현으로 다시 계산하지 않는다. (2026-11-11 WB04-E292)
function replaySeed(kind, expected) {
    let checked = 0;
    // 첫 조회와 흩뿌리기 사례를 별도 검사로 분리한다.
    for (const row of SEED_ROWS) {
        assert.equal(row.length, 10);
        if (row[1] !== kind) continue;
        const fields = row[2].split(',');
        assert.equal(fields.length, 7);
        // 전체 signed DWORD 를 유지한다.
        const input = fields.map((field) => Number(field) | 0);
        const globals = new TerrainContextGlobals({ noRegion: input[2] });
        const context = new RawTerrainContext('patch1078', 1, 127, seedBounds(input[0]), globals, noop);
        const chunks = new ChunkMap();
        seedChunks(chunks, input[0], input[1] >>> 0);
        chunks.mode = input[4] >>> 0;
        if (input[0] === 8) { context.map.bounds[0] = 50; context.map.bounds[1] = 50; context.map.bounds[2] = 53; context.map.bounds[3] = 53; }
        if (input[0] === 9) { context.codes.fill(0x78); context.regions.fill(9); }
        const assertions = [];
        let checking = input[3];
        const growth = new RawTerrainGrowth(context, chunks, globals, {
            checking: () => checking,
            assertion: (expression, file, line) => {
                // 원문 진단 복귀의 명시 효과만 적용한다.
                assertions.push(`${expression}|${file}|${line}`);
                if (input[5] === 1 && line === 0xb7) { chunks.mode = 1; globals.noRegion = 0; }
                if (input[5] === 2 && line === 0xb8) { const bounds = context.chunks(); bounds.right = 0; context.setBounds(bounds); chunks.at(0, 0).island = 127; }
                if (input[5] === 3 && line === 0xb7) { chunks.mode = 1; checking = 0; chunks.at(0, 0).island = 127; }
            },
        });
        if (kind === 'First') assert.equal(growth.firstChunkSeed(), Number(row[3]), row[2]);
        else growth.scatter(input[6]);
        assert.equal(assertions.length === 0 ? '-' : assertions.join(';'), row[4], row[2]);
        assertSameBytes(context.codes, rle(row[8]), `codes ${row[2]}`);
        assertSameBytes(context.regions, rle(row[9]), `regions ${row[2]}`);
        assert.equal(Number(row[7]), kind === 'First' ? 0 : (input[6] * 2) >>> 0);
        checked += 1;
    }
    assert.equal(checked, expected);
}

// 111개 signed BYTE/전체 noRegion·순회·진단 복귀/고정 fallback 을 대조한다. (2026-11-11 WB04-E292)
test('초기 지형 첫 청크 시드는 관측한 signed BYTE 비교·순회·진단 복귀를 그대로 재생한다', { skip: SEED_ROWS === null }, () => { replaySeed('First', 111); });

// 중복/기존 x 의 지역 덮어쓰기·다른 섬 거부·빈 지도 fallback·0회 호출을 원문 배열 전체로 대조한다(78행). (2026-11-11 WB04-E292)
test('초기 지형 흩뿌리기는 관측한 전체 code/region 배열을 그대로 재생한다', { skip: SEED_ROWS === null }, () => { replaySeed('Scatter', 78); });

// 같은 장기 checking 은 호출 때 다시 읽으며 진단 없는 정상 호출에 장치를 미리 강제하지 않는다. (2026-11-11 WB04-E292)
test('초기 지형 첫 시드는 살아 있는 checking 을 호출마다 읽고 선택된 장치 누락을 숨기지 않는다', () => {
    const globals = new TerrainContextGlobals();
    const context = new RawTerrainContext('patch1078', 1, 127, { left: 3, top: 3, right: 5, bottom: 5 }, globals, noop);
    const chunks = new ChunkMap();
    let checking = 0;
    const growth = new RawTerrainGrowth(context, chunks, globals, { checking: () => checking });
    assert.equal(growth.firstChunkSeed(), 0x2333);
    checking = 0x80000000;
    assert.throws(() => growth.firstChunkSeed(), /진단 장치 누락/);
    chunks.at(4, 3).island = 0;
    chunks.at(4, 3).record[3] = 255;
    assert.equal(growth.firstChunkSeed(), 255);
    chunks.mode = 0;
    checking = 0;
    assert.throws(() => growth.firstChunkSeed(), /진단 장치 누락/);
});

// 일반 분기는 첫 시드 실패 진단을 만들지 않는다. 기존 실제 일반 성장 계약을 보존한다. (2026-11-11 WB04-E292)
test('초기 지형 일반 성장은 첫 시드를 조회하지 않는다', () => {
    const globals = new TerrainContextGlobals();
    const context = new RawTerrainContext('patch1078', 1, 127, { left: 3, top: 3, right: 5, bottom: 5 }, globals, noop);
    const chunks = new ChunkMap();
    new RawTerrainGrowth(context, chunks, globals, { checking: () => 0xffffffff }).runNormal();
    assert.ok(context.codes.every((value) => value === 0x2e));
});

// ───────────── 군도 공유 helper (terrain-arch-helpers) ─────────────

// 실제 helper 가 읽을 명시 문자/지역만 놓고 결과 성장/가중치/제거는 계산하지 않는다(입력 공식은 호출할 제거 helper 와 독립). (2026-11-11 WB04-E292)
function archInput(context, profile) {
    context.regions.fill(9);
    // Native 와 같은 넓은 입력 칸 영역이다.
    for (let x = 48; x < 112; x += 1) {
        for (let y = 48; y < 112; y += 1) {
            let code = 0x2e;
            if (profile === 0) code = (x === 70 && y === 70) ? 0x78 : 0x2e;
            else if (profile === 1) code = (x * 3 + y) % 7 === 0 ? 0x78 : (x + y) % 3 === 0 ? 0x3d : 0x2e;
            else if (profile === 2) code = 0x78;
            else if (profile === 3) code = (x === 70 || y === 70) ? 0x78 : 0x2e;
            else if (profile === 4) code = (Math.trunc(x / 3) + Math.trunc(y / 3)) % 2 === 0 ? 0x78 : 0x2e;
            else code = (x + y) % 2 === 0 ? 0x78 : 0x2e;
            context.codes[x * 256 + y] = code;
            context.regions[x * 256 + y] = (x + y) % 256;
        }
    }
}

// 선택 helper 의 모든 고유 행을 재생하며 두 배열과 caller 시드를 원문 기대값과 비교한다. (2026-11-11 WB04-E292)
function replayArch(kind, expected) {
    let checked = 0;
    const clearBounds = [
        { left: 4, top: 4, right: 6, bottom: 5 }, { left: 3, top: 3, right: 4, bottom: 5 }, { left: 6, top: 5, right: 4, bottom: 4 },
        { left: 4, top: 4, right: 4, bottom: 5 }, { left: 4, top: 5, right: 6, bottom: 4 }, { left: 0x10000004, top: 4, right: 0x10000005, bottom: 5 },
    ];
    // 선택된 helper 의 입력만 실행한다. MP/게임/전투 효과는 없다.
    for (const row of ARCH_ROWS) {
        assert.equal(row.length, 7);
        if (row[1] !== kind) continue;
        const input = row[2].split(',');
        const globals = new TerrainContextGlobals();
        const context = new RawTerrainContext('patch1078', 1, 127, { left: 4, top: 4, right: 6, bottom: 5 }, globals, noop);
        const chunks = new ChunkMap();
        const growth = new RawTerrainGrowth(context, chunks, globals);
        const seed = { value: 0 };
        let result = false;
        chunks.mode = 1;
        // Native 의 명시 청크 0 입력을 기본 noRegion 127 과 구별한다.
        for (const chunk of chunks.entries) chunk.island = 0;
        archInput(context, kind === 'Cell' ? 0 : Number(input[0]));
        if (kind === 'Grow') {
            assert.equal(input.length, 5);
            const profile = Number(input[0]);
            context.sharedArchipelagoMode().value = Number(input[1]);
            const region = Number(input[2]) | 0;
            seed.value = Number(input[3]) >>> 0;
            context.regions.fill(region & 0xff);
            growth.growRectangle(region, profile === 1 ? { left: 4, top: 4, right: 5, bottom: 6 } : { left: 4, top: 4, right: 6, bottom: 5 }, Number(input[4]) | 0, seed);
        } else if (kind === 'Cell') {
            assert.equal(input.length, 4);
            const mask = Number(input[0]) >>> 0;
            globals.noRegion = Number(input[2]) | 0;
            context.codes[70 * 256 + 70] = Number(input[3]) & 0xff;
            // 명시 mask 를 고정 방향의 이웃 입력으로만 변환한다.
            for (let direction = 0; direction < 8; direction += 1) {
                const [dx, dy] = [[0, -1], [1, -1], [1, 0], [1, 1], [0, 1], [-1, 1], [-1, 0], [-1, -1]][direction];
                context.codes[(70 + dx) * 256 + (70 + dy)] = (mask & (1 << direction)) ? 0x78 : 0x2e;
            }
            result = growth.removeSparseCell(70, 70, Number(input[1]) | 0);
        } else if (kind === 'Sparse') {
            assert.equal(input.length, 2);
            globals.noRegion = Number(input[1]) | 0;
            growth.removeSparse();
        } else {
            assert.equal(kind, 'Clear');
            assert.equal(input.length, 2);
            globals.noRegion = Number(input[1]) | 0;
            growth.clearPlacement(clearBounds[Number(input[0])]);
        }
        assertSameBytes(context.codes, rle(row[5]), `codes ${kind} ${row[2]}`);
        assertSameBytes(context.regions, rle(row[6]), `regions ${kind} ${row[2]}`);
        assert.equal(result ? 1 : 0, Number(row[3]), `${kind} ${row[2]}`);
        assert.equal(seed.value, Number(row[4]), `${kind} ${row[2]}`);
        checked += 1;
    }
    assert.equal(checked, expected);
}

// 지역 BYTE 감김/군도 비영 DWORD·목표 비양수와 비정사각형 X 폭/최종 시드를 대조한다(72행). (2026-11-11 WB04-E292)
test('초기 지형 GrowRectangle 은 관측한 지역 감김·군도 DWORD·X 폭·최종 시드를 그대로 재생한다', { skip: ARCH_ROWS === null }, () => { replayArch('Grow', 72); });

// 전체 256 이웃 mask 와 -1/4/12 임계값·비 x/현재 noRegion BYTE 를 대조한다(771행). (2026-11-11 WB04-E292)
test('초기 지형 성긴 칸 제거는 관측한 이웃 mask·임계값·noRegion 을 그대로 재생한다', { skip: ARCH_ROWS === null }, () => { replayArch('Cell', 771); });

// 같은 pass 의 즉시 제거와 재순회·내부 경계 제외를 전체 배열에서 대조한다(12행). (2026-11-11 WB04-E292)
test('초기 지형 RemoveSparse 는 관측한 즉시 제거와 재순회를 그대로 재생한다', { skip: ARCH_ROWS === null }, () => { replayArch('Sparse', 12); });

// context bounds 밖의 배치·역전/빈 영역·DWORD shift 감김을 대조한다(12행). (2026-11-11 WB04-E292)
test('초기 지형 ClearPlacement 는 관측한 범위 밖/역전/shift 감김을 그대로 재생한다', { skip: ARCH_ROWS === null }, () => { replayArch('Clear', 12); });

// ───────────── 계약 ─────────────

// 만들기 입력의 종류 오류와 진단 연결 오류를 거부한다. (2026-11-11 WB04-E292)
test('초기 지형 성장은 잘못된 연결/입력 종류를 거부한다', () => {
    const globals = new TerrainContextGlobals();
    const context = new RawTerrainContext('patch1078', 1, 127, { left: 3, top: 3, right: 5, bottom: 5 }, globals, noop);
    const chunks = new ChunkMap();
    assert.throws(() => new RawTerrainGrowth({}, chunks, globals), TypeError);
    assert.throws(() => new RawTerrainGrowth(context, {}, globals), TypeError);
    assert.throws(() => new RawTerrainGrowth(context, chunks, {}), TypeError);
    assert.throws(() => new RawTerrainGrowth(context, chunks, globals, null), TypeError);
    assert.throws(() => new RawTerrainGrowth(context, chunks, globals, { checking: 1 }), TypeError);
    assert.throws(() => new RawTerrainGrowth(context, chunks, globals, { assertion: 1 }), TypeError);
    const growth = new RawTerrainGrowth(context, chunks, globals);
    const seed = { value: 5 };
    assert.throws(() => growth.nextRange(5, 0, 3), TypeError);
    assert.throws(() => growth.nextRange(seed, 0.5, 3), TypeError);
    assert.throws(() => growth.scatter(-1), RangeError);
    assert.throws(() => growth.scatter(1.5), RangeError);
    assert.throws(() => growth.growRectangle(1, { left: 0, top: 0, right: 1 }, 1, seed), TypeError);
    assert.throws(() => growth.growRectangle(1, { left: 0, top: 0, right: 1, bottom: 1 }, 1, {}), TypeError);
    assert.throws(() => growth.removeSparseCell(70, 70, 1.5), TypeError);
    assert.throws(() => growth.clearPlacement({ left: 0, top: 0, right: 1 }), TypeError);
    assert.throws(() => growth.fixGaps(1.5), TypeError);
});

// 범위 난수: 폭이 1 미만이면 low 를 그대로 돌려주고 시드를 소비하지 않으며, 그 밖에는 시드 상자를 갱신한다. (2026-11-11 WB04-E292)
test('초기 지형 범위 난수는 비양수 폭에서 시드를 소비하지 않는다', () => {
    const globals = new TerrainContextGlobals();
    const context = new RawTerrainContext('patch1078', 1, 127, { left: 3, top: 3, right: 5, bottom: 5 }, globals, noop);
    const growth = new RawTerrainGrowth(context, new ChunkMap(), globals);
    const seed = { value: 123 };
    assert.equal(growth.nextRange(seed, 7, 7), 7);
    assert.equal(growth.nextRange(seed, 7, 3), 7);
    assert.equal(seed.value, 123);
    // 폭 1 은 limit 1 이라 결과는 low 이지만 시드는 한 번 전진한다.
    assert.equal(growth.nextRange(seed, 7, 8), 7);
    assert.equal(seed.value, ((Math.imul(123, 0x10003) + 3) >>> 0));
    // 시드 0 은 0x0bad0bad 로 바뀐 뒤 전진한다.
    const zero = { value: 0 };
    growth.nextRange(zero, 0, 10);
    assert.equal(zero.value, (Math.imul(0x0bad0bad, 0x10003) + 3) >>> 0);
    // int32 감김 폭(high - low 가 int32 를 넘는 입력)도 같은 규칙으로 처리한다: 폭이 감겨 1 이 되면 소비하고 -1 이 되면 소비하지 않는다.
    const wrap = { value: 1 };
    assert.equal(growth.nextRange(wrap, 0x7fffffff, -0x80000000), 0x7fffffff);
    assert.equal(wrap.value, (Math.imul(1, 0x10003) + 3) >>> 0);
    const negative = { value: 1 };
    assert.equal(growth.nextRange(negative, -0x80000000, 0x7fffffff), -0x80000000);
    assert.equal(negative.value, 1);
});

// 지도 밖 접근은 가장자리로 끌어당기지 않고 거부하며, 작은 위치 이상도 이웃 접근 순서를 지킨다. (2026-11-11 WB04-E292)
test('초기 지형 성장은 지도 밖 접근을 거부한다', () => {
    const globals = new TerrainContextGlobals();
    const context = new RawTerrainContext('patch1078', 1, 127, { left: 3, top: 3, right: 5, bottom: 5 }, globals, noop);
    const growth = new RawTerrainGrowth(context, new ChunkMap(), globals);
    context.codes[0] = 0x78;
    // (0, 0) 의 x 이웃 중 음수 칸이 있어 거부한다.
    assert.throws(() => growth.removeSparseCell(0, 0, 4), /지도 범위/);
    // 현재 칸이 x 가 아니면 이웃을 읽지 않고 false 이다.
    assert.equal(growth.removeSparseCell(0, 1, 4), false);
    assert.throws(() => growth.removeSparseCell(300, 1, 4), /지도 범위/);
    // 청크 지우기는 음수 시작 좌표를 거부한다.
    assert.throws(() => growth.clearPlacement({ left: -1, top: 0, right: 1, bottom: 1 }), /지도 범위/);
});

// ───────────── 변형 확인으로 보강한 계약 ─────────────

// 진단 연결이 없어도 정상 조회는 성공하고(장치를 미리 강제하지 않음), 실패 fallback 은 checking 이 꺼져 있으면 진단 없이 0x2333 이다. (2026-11-11 WB04-E292)
test('초기 지형 첫 시드는 진단 연결이 없어도 정상 입력에서 거부하지 않는다', () => {
    const globals = new TerrainContextGlobals();
    const context = new RawTerrainContext('patch1078', 1, 127, { left: 3, top: 3, right: 5, bottom: 5 }, globals, noop);
    const chunks = new ChunkMap();
    const growth = new RawTerrainGrowth(context, chunks, globals);
    assert.equal(growth.firstChunkSeed(), 0x2333);
    chunks.at(3, 3).island = 0;
    chunks.at(3, 3).record[3] = 77;
    assert.equal(growth.firstChunkSeed(), 77);
});

// 청크 getter 의 범위 진단은 현재 가로·세로와 같은 좌표(x == xLength, y == yLength)부터 보고하고, 순회는 Y 외부(top 에서 시작)/X 내부이다. (2026-11-11 WB04-E292)
test('초기 지형 첫 시드는 현재 청크 크기 경계에서 진단하고 Y 외부 순서로 첫 칸을 고른다', () => {
    const globals = new TerrainContextGlobals();
    const context = new RawTerrainContext('patch1078', 1, 127, { left: 3, top: 2, right: 5, bottom: 4 }, globals, noop);
    const chunks = new ChunkMap(4, 3);
    const lines = [];
    const growth = new RawTerrainGrowth(context, chunks, globals, { assertion: (expression, file, line) => lines.push(line) });
    // 빈 지도: (3,2) 안, (4,2) x == 4 보고, (3,3) y == 3 보고, (4,3) 둘 다 한 번 보고. 항목은 당겨진 가장자리 칸이라 모두 비어 있다.
    assert.equal(growth.firstChunkSeed(), 0x2333);
    assert.deepEqual(lines, [0xb8, 0xb8, 0xb8]);
    // 순회 순서: y 외부이면 (4,4) 가 먼저, x 외부이면 (3,5) 가 먼저이다. top 이 아닌 left 에서 y 를 시작하지 않는다((3,3) 은 범위 밖).
    const wide = new ChunkMap();
    const wideContext = new RawTerrainContext('patch1078', 1, 127, { left: 3, top: 4, right: 6, bottom: 7 }, globals, noop);
    wide.at(3, 3).island = 0;
    wide.at(3, 3).record[3] = 22;
    wide.at(4, 4).island = 0;
    wide.at(4, 4).record[3] = 33;
    wide.at(3, 5).island = 0;
    wide.at(3, 5).record[3] = 44;
    assert.equal(new RawTerrainGrowth(wideContext, wide, globals).firstChunkSeed(), 33);
});

// 범위 난수의 합은 int32 로 감긴다. (2026-11-11 WB04-E292)
test('초기 지형 범위 난수의 결과는 int32 로 감긴다', () => {
    const globals = new TerrainContextGlobals();
    const context = new RawTerrainContext('patch1078', 1, 127, { left: 3, top: 3, right: 5, bottom: 5 }, globals, noop);
    const growth = new RawTerrainGrowth(context, new ChunkMap(), globals);
    // 시드 1 의 다음 상태 0x10006 은 상위 16비트 1 → 폭 3 에서 1 이라 low + 1 이 int32 상한을 넘어 감긴다.
    const seed = { value: 1 };
    assert.equal(growth.nextRange(seed, 0x7fffffff, -0x7ffffffe), -0x80000000);
});

// 지도 저장 범위 경계(0 이상 256 미만)는 각 축에서 정확히 거부한다. (2026-11-11 WB04-E292)
test('초기 지형 성장은 지도 저장 범위의 각 축 경계를 정확히 거부한다', () => {
    const globals = new TerrainContextGlobals();
    const context = new RawTerrainContext('patch1078', 1, 127, { left: 3, top: 3, right: 5, bottom: 5 }, globals, noop);
    const growth = new RawTerrainGrowth(context, new ChunkMap(), globals);
    // 경계 안쪽 끝 칸은 허용되고 한 칸 밖은 거부된다(좌표 자체의 범위만 보이도록 칸 문자가 x 가 아닌 경우).
    assert.equal(growth.removeSparseCell(255, 255, 4), false);
    assert.equal(growth.removeSparseCell(0, 0, 4), false);
    assert.throws(() => growth.removeSparseCell(256, 1, 4), /지도 범위/);
    assert.throws(() => growth.removeSparseCell(1, 256, 4), /지도 범위/);
    assert.throws(() => growth.removeSparseCell(-1, 1, 4), /지도 범위/);
    assert.throws(() => growth.removeSparseCell(1, -1, 4), /지도 범위/);
    assert.throws(() => growth.removeSparseCell(1.5, 1, 4), /지도 범위/);
});

// right == left 이면 폭이 -1 이라 두 좌표 난수가 low 로 고정되어 성장 중심이 (80, 80) 으로 결정되므로 Put 의 양 끝 포함 bounds 와 signed 지역 비교를 직접 볼 수 있다. (2026-11-11 WB04-E292)
function deterministicGrow({ archipelago, regionByte, chunkIsland, bounds }) {
    const globals = new TerrainContextGlobals();
    const context = new RawTerrainContext('patch1078', 1, 127, { left: 4, top: 4, right: 6, bottom: 5 }, globals, noop);
    const chunks = new ChunkMap();
    for (const chunk of chunks.entries) chunk.island = chunkIsland;
    context.sharedArchipelagoMode().value = archipelago;
    for (let index = 0; index < 4; index += 1) context.map.bounds[index] = bounds[index];
    context.codes[80 * 256 + 80] = 0x78;
    context.regions[80 * 256 + 80] = regionByte;
    const growth = new RawTerrainGrowth(context, chunks, globals);
    growth.growRectangle((regionByte << 24) >> 24, { left: 5, top: 5, right: 5, bottom: 5 }, 8, { value: 1 });
    const filled = [];
    for (let x = 78; x <= 82; x += 1) for (let y = 78; y <= 82; y += 1) if (context.codes[x * 256 + y] === 0x78) filled.push(`${x},${y}`);
    return filled;
}

test('초기 지형 Put 은 현재 bounds 의 양 끝 칸을 포함하고 signed 지역을 청크 BYTE 와 비교한다', () => {
    // 군도 DWORD 가 비영이면 청크 지역 조회 없이 bounds 안의 3×3 이 모두 채워진다: 오른쪽/아래 경계 81 포함.
    const inside = deterministicGrow({ archipelago: 1, regionByte: 3, chunkIsland: 0, bounds: [79, 79, 81, 81] });
    assert.equal(inside.length, 9);
    // bounds 가 79..81 → 80..81 로 좁으면 왼쪽/위 경계(80)도 포함이고 그 밖은 제외된다.
    const narrow = deterministicGrow({ archipelago: 1, regionByte: 3, chunkIsland: 0, bounds: [80, 80, 81, 81] });
    assert.deepEqual(narrow, ['80,80', '80,81', '81,80', '81,81']);
    // 군도 0 이면 청크 지역과 같은 칸만 추가한다: 청크 BYTE 0xff 는 signed -1 이고 지역 255(= -1) 와 같다.
    const same = deterministicGrow({ archipelago: 0, regionByte: 255, chunkIsland: 0xff, bounds: [79, 79, 81, 81] });
    assert.equal(same.length, 9);
    // 청크 BYTE 가 다르면 중심 칸 말고는 추가되지 않는다.
    const other = deterministicGrow({ archipelago: 0, regionByte: 255, chunkIsland: 0x01, bounds: [79, 79, 81, 81] });
    assert.deepEqual(other, ['80,80']);
});

// 연속 실패 한계는 성공할 때마다 다시 1000 으로 돌아간다: 5×5 씨앗에서 시작해 1000 회를 넘기는 총 시도 끝에 15×15 창과 둘레 한 칸(17×17 = 289 칸)이 모두 찬다. (2026-11-11 WB04-E292)
test('초기 지형 성장의 연속 실패 한계는 성공마다 재설정된다', () => {
    const globals = new TerrainContextGlobals();
    const context = new RawTerrainContext('patch1078', 1, 127, { left: 4, top: 4, right: 6, bottom: 5 }, globals, noop);
    context.sharedArchipelagoMode().value = 1;
    context.map.bounds[0] = 0;
    context.map.bounds[1] = 0;
    context.map.bounds[2] = 255;
    context.map.bounds[3] = 255;
    // 5×5 씨앗.
    for (let x = 68; x <= 72; x += 1) for (let y = 68; y <= 72; y += 1) context.codes[x * 256 + y] = 0x78;
    const seed = { value: 20261111 };
    new RawTerrainGrowth(context, new ChunkMap(), globals).growRectangle(0, { left: 4, top: 4, right: 5, bottom: 5 }, 100000, seed);
    let count = 0;
    for (let x = 0; x < 256; x += 1) for (let y = 0; y < 256; y += 1) if (context.codes[x * 256 + y] === 0x78) count += 1;
    assert.equal(count, 289);
    // 포화한 창은 63..79 칸이다.
    assert.equal(context.codes[63 * 256 + 63], 0x78);
    assert.equal(context.codes[79 * 256 + 79], 0x78);
    assert.equal(context.codes[80 * 256 + 80], 0x2e);
});

// RemoveSparse 의 결과는 변화가 없을 때까지 반복한 고정점이다: 내부의 모든 x 칸은 가중치(직교 2·대각 1)가 임계 4 를 넘는다. (2026-11-11 WB04-E292)
test('초기 지형 RemoveSparse 의 결과에는 임계 이하 칸이 남지 않는다', () => {
    let state = 12345;
    // 검사 안에서만 쓰는 결정적 난수이다.
    const random = (limit) => { state = (Math.imul(state, 1103515245) + 12345) >>> 0; return (state >>> 8) % limit; };
    const countX = (codes) => codes.reduce((sum, value) => sum + (value === 0x78 ? 1 : 0), 0);
    let removedTotal = 0;
    // 밀도를 바꿔 여러 무늬를 검사한다.
    for (const density of [45, 55, 62, 70]) {
        const globals = new TerrainContextGlobals();
        const context = new RawTerrainContext('patch1078', 1, 127, { left: 4, top: 4, right: 6, bottom: 5 }, globals, noop);
        for (let x = 62; x <= 97; x += 1) for (let y = 62; y <= 81; y += 1) context.codes[x * 256 + y] = random(100) < density ? 0x78 : 0x2e;
        const before = countX(context.codes);
        new RawTerrainGrowth(context, new ChunkMap(), globals).removeSparse();
        removedTotal += before - countX(context.codes);
        // 내부(left+1..right-2, top+1..bottom-2)의 모든 x 칸이 임계를 넘는다.
        for (let x = 65; x <= 94; x += 1) {
            for (let y = 65; y <= 78; y += 1) {
                if (context.codes[x * 256 + y] !== 0x78) continue;
                let weight = 0;
                for (const [dx, dy, w] of [[0, -1, 2], [1, -1, 1], [1, 0, 2], [1, 1, 1], [0, 1, 2], [-1, 1, 1], [-1, 0, 2], [-1, -1, 1]]) if (context.codes[(x + dx) * 256 + (y + dy)] === 0x78) weight += w;
                assert.ok(weight > 4, `density ${density} ${x},${y} 가중치 ${weight}`);
            }
        }
    }
    assert.ok(removedTotal > 100);
});

// 8 이웃(북부터 시계방향)의 지역 목록으로 (tx, ty) 둘레를 채운다. null 은 허공이다. (2026-11-11 WB04-E292)
function setRing(context, tx, ty, regions) {
    const cells = [[0, -1], [1, -1], [1, 0], [1, 1], [0, 1], [-1, 1], [-1, 0], [-1, -1]];
    cells.forEach(([dx, dy], index) => {
        const at = (tx + dx) * 256 + (ty + dy);
        context.codes[at] = regions[index] === null ? 0x2e : 0x78;
        context.regions[at] = regions[index] === null ? 0 : regions[index];
    });
}

// 빈틈 보정을 한 번 돌린 뒤 (tx, ty) 의 문자와 지역을 돌려준다. (2026-11-11 WB04-E292)
function fixOne(tx, ty, regions) {
    const globals = new TerrainContextGlobals();
    const context = new RawTerrainContext('patch1078', 1, 127, { left: 4, top: 4, right: 6, bottom: 5 }, globals, noop);
    setRing(context, tx, ty, regions);
    new RawTerrainGrowth(context, new ChunkMap(), globals).fixGaps(1);
    return [context.codes[tx * 256 + ty], context.regions[tx * 256 + ty]];
}

test('초기 지형 빈틈 보정은 같은 지역 연속 5개가 홀수 방향에 닿을 때만 칸을 채운다', () => {
    // 지역이 바뀌면 연속 수를 0 으로 되돌린다: 3 + (다른 지역) + 이어지는 같은 지역은 5 가 되지 않는다.
    assert.deepEqual(fixOne(70, 70, [1, 1, 1, 2, 1, 1, null, null]), [0x2e, 127]);
    // 연속 4 가 홀수 방향에 닿아도 채우지 않는다(>4 가 필요).
    assert.deepEqual(fixOne(70, 70, [3, 3, 3, 3, null, null, null, null]), [0x2e, 127]);
    // 연속 5 가 짝수 방향(남쪽)에서 끝나면 채우지 않는다.
    assert.deepEqual(fixOne(70, 70, [3, 3, 3, 3, 3, null, null, null]), [0x2e, 127]);
    // 연속 5 가 홀수 방향(남서)에서 끝나면 그 지역으로 채운다.
    assert.deepEqual(fixOne(70, 70, [null, 3, 3, 3, 3, 3, null, null]), [0x78, 3]);
    // 지역 BYTE 는 signed 로 읽혀도 같은 하위 BYTE 로 되돌려 쓴다.
    assert.deepEqual(fixOne(70, 70, [255, 255, 255, 255, 255, 255, null, null]), [0x78, 255]);
    // 둘레가 모두 같은 지역이면(원본 13 이웃 순환) 채운다.
    assert.deepEqual(fixOne(70, 70, [2, 2, 2, 2, 2, 2, 2, 2]), [0x78, 2]);
});

test('초기 지형 빈틈 보정은 현재 bounds 안쪽 칸만 대상으로 삼는다', () => {
    const all = [2, 2, 2, 2, 2, 2, 2, 2];
    // map.bounds = [64, 64, 96, 80]: x 는 65..94, y 는 65..79 이다(왼쪽/위 한 칸 안쪽에서 시작, 오른쪽은 한 칸 더 제외, 아래는 bottom 직전까지).
    assert.equal(fixOne(64, 70, all)[0], 0x2e);
    assert.equal(fixOne(65, 70, all)[0], 0x78);
    assert.equal(fixOne(95, 70, all)[0], 0x2e);
    assert.equal(fixOne(94, 70, all)[0], 0x78);
    assert.equal(fixOne(70, 64, all)[0], 0x2e);
    assert.equal(fixOne(70, 65, all)[0], 0x78);
    assert.equal(fixOne(70, 79, all)[0], 0x78);
    assert.equal(fixOne(70, 80, all)[0], 0x2e);
});

// 일반 성장은 청크 섬 번호가 0..99 인 것만 받고(그 밖은 방문 배열 밖이라 거부), 현재 noRegion 은 signed 로 비교한다. (2026-11-11 WB04-E292)
test('초기 지형 일반 성장은 섬 번호 범위를 검사하고 noRegion 을 signed 로 비교한다', () => {
    const make = (noRegion, island) => {
        const globals = new TerrainContextGlobals({ noRegion });
        const context = new RawTerrainContext('patch1078', 1, noRegion, { left: 3, top: 3, right: 4, bottom: 4 }, globals, noop);
        const chunks = new ChunkMap();
        for (const chunk of chunks.entries) chunk.island = noRegion & 0xff;
        const chunk = chunks.at(3, 3);
        chunk.island = island;
        chunk.side = 'A';
        return new RawTerrainGrowth(context, chunks, globals);
    };
    assert.throws(() => make(127, 100).runNormal(), /섬 방문 배열/);
    assert.throws(() => make(127, 0x80).runNormal(), /섬 방문 배열/);
    make(127, 99).runNormal();
    make(127, 0).runNormal();
    // noRegion 이 -1 이면 청크 BYTE 0xff 와 같은 값으로 비교되어 건너뛴다.
    make(-1, 0xff).runNormal();
});
