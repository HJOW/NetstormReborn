// 섬 Builder raw-island-builder.js(RawIslandBuilder)를 원본 10.78 관측과 cpppj `IslandBuilderTests` 계약으로 대조한다.
//  - 원본 관측 islandbuilder-x86.tsv 의 10.78(originals) 행(Place·PlaceAll): 입력 14칸(월드/임시 지도·owner·territory·기존 count·builder 경계 4필드·ignorePosition·프레임 프로필·청크 16×16·start/end)과 6 BYTE 레코드(또는 20레코드)에서
//    반환 지역·대상 지도 256청크 12 BYTE 전체(섬 번호/방향 글자/레코드 6 BYTE/순번)·임시 지도일 때 월드 지도 불변·3204 BYTE 섬 목록 전체(count 밖 padding 포함)·진단 순서(B·E·F·R·M)와 진단 횟수를 대조한다.
//  - 계약: 형식/필수 진단/물리 Terr 범위를 효과 전에 거부하고 정상 empty 범위는 원본 진단 뒤 반환하며, 실제 Builder 가 패턴을 쓰기 전에 현재 Canon 진단을 읽는다.
//  - 웹 범위 밖: 실제 월드 후속 성장 소비자(IslandBuilder_LiveWorldChunkRuntimeAndGrowthConsumer, RawTerrainGrowth 미이식)는 그 모듈을 옮길 때 대조한다. CD 10.72 행(OddDirection·BoundsNotFound)도 범위 밖이다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-10 WB04-E222: 섬 Builder 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { SquidPostPopList } from '../www/core/squid-postpop-state.js';
import { ChunkMap } from '../www/assets/territory.js';
import { RawIslandRuntime, createFortFinishOwnerState, createRawIslandStorage } from '../www/core/raw-island-runtime.js';
import { ISLAND_BUILDER_ASSERTION, RawIslandBuilder } from '../www/core/raw-island-builder.js';

// 원본 관측 10.78(originals) 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-10 WB04-E222)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'islandbuilder-x86.tsv'))?.filter((row) => row[0] === 'originals') ?? null;

// 실제 puzzlePiece 타입과 fixture 의 원본 물리 record 수이다. (2026-11-10 WB04-E222)
const PIECE = 107;
const CHUNKS = 256;
const ISLANDS = 100;

// 예외가 나는지 본다. (2026-11-10 WB04-E222)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 16방향·번호 0..10 의 합성 프레임 입력은 native 입력과 같고 출력 frame 을 미리 계산하지 않는다. profile 1 은 역순, 2 는 빈 표, 3 은 G 방향만 제거한다. (2026-11-10 WB04-E222)
function builderFrames(profile) {
    let codes = [];
    // 빈 profile 2 외에는 모든 방향의 원본 패턴 variation 번호를 공급한다.
    for (let side = 65; side <= 80; side += 1) {
        for (let number = 0; number < 11; number += 1) if (profile !== 2) codes.push({ side, variant: 80, number, flags: 0 });
    }
    if (profile === 1) codes.reverse();
    if (profile !== 2) codes.push({ ...codes[0] });
    // 부분 누락 profile 3 은 G 방향만 제거하여 진단/실제 쓰기의 교차 순서를 관찰한다.
    if (profile === 3) codes = codes.filter((code) => code.side !== 71);
    return codes;
}

// fixture 의 연속 hex 바이트를 원본 값으로 읽는다. (2026-11-10 WB04-E222)
function hexBytes(text) {
    assert.equal(text.length % 2, 0);
    return Uint8Array.from(text.match(/../g) ?? [], (pair) => parseInt(pair, 16));
}

// 같은 길이 block RLE 를 풀며(존재/owner/bounds 를 계산해 대신하지 않음) 전체 물리 길이로 복원한다. (2026-11-10 WB04-E222)
function blocks(text, width, count) {
    const bytes = [];
    for (const run of text.split(';')) {
        const fields = run.split(':');
        assert.equal(fields.length, 2);
        const block = hexBytes(fields[0]);
        assert.equal(block.length, width);
        const repeat = Number(fields[1]);
        assert.ok(repeat <= (count * width - bytes.length) / width, 'RLE 초과');
        for (let index = 0; index < repeat; index += 1) bytes.push(...block);
    }
    assert.equal(bytes.length, count * width);
    return Uint8Array.from(bytes);
}

// 현재 ChunkMap 의 물리 256개 X 열 우선 record 를 원본 12 BYTE 로 직렬화한다. (2026-11-10 WB04-E222)
function chunkBytes(map) {
    const bytes = new Uint8Array(CHUNKS * 12);
    for (let x = 0; x < 16; x += 1) {
        for (let y = 0; y < 16; y += 1) {
            const entry = map.at(x, y);
            const offset = (x * 16 + y) * 12;
            bytes[offset] = entry.island;
            bytes[offset + 1] = typeof entry.side === 'string' ? entry.side.charCodeAt(0) : entry.side & 0xff;
            bytes.set(entry.record, offset + 2);
            put(bytes, offset + 8, entry.ordinal >>> 0);
        }
    }
    return bytes;
}

// fixture 의 builder 전역/현재 저장소를 준비하며 종료 출력은 native 에서만 읽는다(cpppj BuilderReplayScene). (2026-11-10 WB04-E222)
class BuilderScene {
    constructor() {
        this.pool = new SidPool(32768, { server: false });
        this.hash = new SquidHash();
        this.world = new ChunkMap();
        this.custom = new ChunkMap();
        this.records = createRawIslandStorage();
        this.modes = createFortFinishOwnerState();
        this.residences = new SquidPostPopList();
        this.revision = { value: 0 };
        this.changed = { value: 0 };
        this.piece = PIECE;
        this.frames = Array.from({ length: 188 }, () => []);
        this.events = [];
        this.assertions = 0;
        this.islands = new RawIslandRuntime(this.pool, this.hash, this.records, this.modes, this.residences, this.revision, this.changed, {
            setResidenceOwner: () => { throw new Error('Add/Bounds 는 Residence owner 를 요청하지 않는다'); },
            assertion: () => { throw new Error('Builder fixture 의 Runtime 진단은 원본 분류와 불일치한다'); },
        });
    }

    // 각 행에서 native 와 같은 full raw/padding/기존 count 밖 영역을 초기화한다. (2026-11-10 WB04-E222)
    prepare(count, profile) {
        this.records.fill(0xa5);
        put(this.records, 0, count);
        this.frames[this.piece] = builderFrames(profile);
        this.events = [];
        this.assertions = 0;
        // 네이티브 초기 chunk 12 BYTE 의 모든 필드를 current getter 에 준비한다.
        for (let x = 0; x < 16; x += 1) {
            for (let y = 0; y < 16; y += 1) {
                for (const map of [this.world, this.custom]) {
                    const entry = map.at(x, y);
                    entry.island = 127;
                    entry.side = 46;
                    entry.record.fill(0xa5);
                    entry.ordinal = 0x12345678;
                }
            }
        }
    }

    // 각 원본 진단의 순서를 native 기록의 한 문자와 같게 관찰한다. (2026-11-10 WB04-E222)
    diagnostic(kind) {
        this.assertions += 1;
        const letters = {
            [ISLAND_BUILDER_ASSERTION.invalidBounds]: 'B', [ISLAND_BUILDER_ASSERTION.equalRange]: 'E', [ISLAND_BUILDER_ASSERTION.missingFrame]: 'F',
            [ISLAND_BUILDER_ASSERTION.chunkRange]: 'R', [ISLAND_BUILDER_ASSERTION.boundsMissingIsland]: 'M',
        };
        this.events.push(letters[kind] ?? assert.fail(`모르는 진단 ${kind}`));
    }

    // 진단이 없으면 '-' 이며 중복 진단도 발생 순서로 유지한다. (2026-11-10 WB04-E222)
    eventText() { return this.events.length === 0 ? '-' : this.events.join(';'); }
}

test('섬 Builder: 원본 10.78 Place/PlaceAll 의 전체 청크·3204 BYTE·진단 순서가 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    assert.ok(rows.length > 0);
    let checked = 0;
    // 고유 row 만 재생하며 두 x87 관찰은 여기서 고유 검사로 중복 계수하지 않는다.
    for (const row of rows) {
        assert.equal(row.length, 10);
        const value = row[2].split(',').map(Number);
        assert.equal(value.length, 14);
        assert.ok(value[10] === 16 && value[11] === 16);
        const scene = new BuilderScene();
        scene.prepare(value[3], value[9]);
        const state = { bounds: { left: value[4] | 0, top: value[5] | 0, right: value[6] | 0, bottom: value[7] | 0 }, owner: value[1], decoderDiagnostics: null };
        const target = value[0] ? scene.world : scene.custom;
        const builder = new RawIslandBuilder(target, scene.world, scene.islands, scene.frames, () => scene.piece, state, (kind) => scene.diagnostic(kind));
        const record = hexBytes(row[3]);
        let result = 0;
        if (row[1] === 'Place') {
            assert.equal(record.length, 6);
            result = builder.place(record, value[2], value[8] !== 0);
        } else builder.placeAll(record, value[12] | 0, value[13] | 0);
        const expectedChunks = blocks(row[5], 12, CHUNKS);
        const split = row[7].indexOf(',');
        assert.ok(split >= 0);
        const islandBlocks = blocks(row[7].slice(split + 1), 32, ISLANDS);
        const expectedIslands = new Uint8Array(4 + islandBlocks.length);
        expectedIslands.set(islandBlocks, 4);
        put(expectedIslands, 0, Number(row[7].slice(0, split)));
        const label = `${row[1]} #${checked} ${row[2]}`;
        assert.equal(result, Number(row[4]), `${label} 반환`);
        assert.deepEqual(chunkBytes(target), expectedChunks, `${label} 대상 지도`);
        assert.deepEqual(scene.records, expectedIslands, `${label} 3204 BYTE`);
        assert.equal(scene.eventText(), row[8], `${label} 진단 순서`);
        assert.equal(scene.assertions, Number(row[9]), `${label} 진단 횟수`);
        if (!value[0]) assert.deepEqual(chunkBytes(scene.world), blocks(row[6], 12, CHUNKS), `${label} 월드 지도 불변`);
        assert.equal(builder.inWorld(), value[0] !== 0);
        assert.equal(builder.pool, scene.pool);
        checked += 1;
    }
    assert.equal(checked, rows.length);
});

test('섬 Builder 계약: 형식/필수 진단/물리 Terr 범위를 효과 전에 거부하고 정상 empty 범위는 원본 진단 뒤 반환한다', () => {
    const scene = new BuilderScene();
    scene.prepare(2, 0);
    const state = { bounds: { left: 1, top: 1, right: 15, bottom: 15 }, owner: 1, decoderDiagnostics: null };
    assert.ok(throws(() => new RawIslandBuilder(scene.world, scene.world, scene.islands, scene.frames, () => scene.piece, state, null)));
    assert.ok(throws(() => new RawIslandBuilder(scene.world, scene.world, scene.islands, scene.frames.slice(0, 171), () => scene.piece, state, () => {})));
    const builder = new RawIslandBuilder(scene.world, scene.world, scene.islands, scene.frames, () => scene.piece, state, (kind) => scene.diagnostic(kind));
    const before = scene.records.slice();
    const records = new Uint8Array(120);
    assert.ok(throws(() => builder.placeAll(records.subarray(0, 119))));
    assert.ok(throws(() => builder.placeAll(records, -1, 20)));
    assert.ok(throws(() => builder.placeAll(records, 0, 21)));
    builder.placeAll(records, 4, 4);
    assert.equal(scene.eventText(), 'E');
    assert.deepEqual(scene.records, before);
});

test('섬 Builder 계약: 실제 Builder 가 패턴을 쓰기 전에 현재 Canon 진단을 읽으며 진단이 중단하면 청크를 쓰지 않는다', () => {
    const scene = new BuilderScene();
    scene.prepare(0, 0);
    // 기존 정상 패턴 번호/순서는 유지하고 금 간 비트(0x20)만 입력한다.
    scene.frames[scene.piece] = scene.frames[scene.piece].map((code) => ({ ...code, flags: code.flags | 0x20 }));
    let reported = 0;
    const diagnostics = { checking: () => 1, assertion: (_expression, _file, line) => { assert.equal(line, 0x325); reported += 1; throw new Error('섬 Canon 진단 중단'); } };
    const state = { bounds: { left: 1, top: 1, right: 15, bottom: 15 }, owner: 1, decoderDiagnostics: diagnostics };
    const before = chunkBytes(scene.world);
    const builder = new RawIslandBuilder(scene.world, scene.world, scene.islands, scene.frames, () => scene.piece, state, () => assert.fail('별도 Builder 진단 미선택'));
    const record = Uint8Array.from([3, 1, 0, 7, 0xaa, 0xbb]);
    assert.ok(throws(() => builder.place(record, 0)));
    assert.equal(reported, 1);
    assert.deepEqual(chunkBytes(scene.world), before);
});

test('섬 Builder 계약: 현재 puzzlePiece 타입이 범위 밖이면 거부하고 같은 크기 사본 지도는 월드로 보지 않는다', () => {
    const scene = new BuilderScene();
    scene.prepare(0, 0);
    const state = { bounds: { left: 1, top: 1, right: 15, bottom: 15 }, owner: 1, decoderDiagnostics: null };
    const make = (target, piece = () => scene.piece) => new RawIslandBuilder(target, scene.world, scene.islands, scene.frames, piece, state, (kind) => scene.diagnostic(kind));
    assert.ok(throws(() => make(scene.world, () => 188).place(Uint8Array.from([3, 1, 0, 0, 0, 0]), 0)));
    // 임시 지도는 전달한 지역을 유지하고 섬 목록에 등록하지 않는다.
    const count = scene.islands.count;
    assert.equal(make(scene.custom).place(Uint8Array.from([3, 1, 0, 0, 0, 0]), 9), 9);
    assert.equal(scene.islands.count, count);
    assert.ok(!make(scene.custom).inWorld() && make(scene.world).inWorld());
});
