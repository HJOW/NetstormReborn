// 섬 런타임 raw-island-runtime.js(RawIslandRuntime·저장소 helper)를 원본 10.78 실제 PE 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: islandruntime-x86.tsv 의 `originals` 1009행 — mutable 100기록의 Reset/Count/Exists/Add/Delete/Owner get·set/Temple get·set/Match/Bounds get·set 을 입력 15개(count·섬 번호·존재·owner·원래 owner·지역·...·모드·경계 9종·callback 변화 7종)로
//    실제 PE 에서 실행해 결과 scalar, 전체 3204바이트 raw, 출력 경계 비트, 공유 전역 두 개(월드 증가·지도 갱신), Player 첫 지역 아홉 개, 효과 순서(진단 `A:B`/`A:E`·거주 구역 owner `O:`), 등록 목록 count/SID, 거주 구역 owner BYTE, 모드, 진단 횟수가 같다.
//    같은 1009행을 실제 9 Player BYTE(0xb4 간격, 첫 지역 +0x78)를 공유하는 모드로도 재생한다(첫 지역 네 바이트 밖은 모두 보존, 모드 사본은 쓰지 않음). `originalCD` 행은 다른 판본이라 사용하지 않는다.
//  - 계약: 세션 없는 cleanup 의 Reset 이 기존 Reset 행의 전체 3204바이트와 같음, 장기 배열을 실행기가 계속 읽고 쓰며 잘못된 폭은 첫 쓰기 전에 거부, 물리 100 기록 경계/필수 효과 거부,
//    월드 증가 DWORD 와 지도 DWORD 가 서로 다른 소유자에서 분리되어 보존됨(진단 복귀의 현재 값 재조회·가상 효과의 후속 변경 유지·unsigned 감김).
// 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E161: 섬 런타임 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { SquidPostPopList } from '../www/core/squid-postpop-state.js';
import { ISLAND_STORAGE_BYTES, RawIslandRuntime, createFortFinishOwnerState, createRawIslandStorage, resetRawIslandRecords } from '../www/core/raw-island-runtime.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-09 WB04-E161)
const allRows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'islandruntime-x86.tsv')) ?? [];
const rows = allRows.filter((row) => row[0] === 'originals');

// 판본 행 수, 풀 크기, 실행기에 준 네 float 비트 9종(경계)과 거주 구역 현재 좌표 4종이다. (2026-11-09 WB04-E161)
const ROWS = 1009;
const CAPACITY = 24000;
const PLAYER_STRIDE = 0xb4;
const PLAYER_FIELD = 0x78;
const RUNTIME_BOUNDS = [
    [0x41200000, 0x41a00000, 0x42700000, 0x428c0000], [0, 0, 0x42700000, 0x428c0000],
    [0xc0000000, 0x41a00000, 0x42700000, 0x428c0000], [0x42480000, 0x42540000, 0x42480000, 0x42540000],
    [0x7fc00001, 0x41a00000, 0x42700000, 0x428c0000], [0x41200000, 0x7fc00001, 0x42700000, 0x428c0000],
    [0x41200000, 0x41a00000, 0x42700000, 0x7fc00001], [0x41200000, 0x41a00000, 0x7fc00001, 0x428c0000],
    [0x41200000, 0x41a00000, 0x43800000, 0x428c0000]
];
const RUNTIME_POINTS = [[10, 20], [50.75, 52.5], [60, 70], [61, 71]].map(([x, y]) => [Math.fround(x), Math.fround(y)]);

// 예외가 나는지 본다. (2026-11-09 WB04-E161)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 바이트 배열을 소문자 16진 문자열로 바꾼다. (2026-11-09 WB04-E161)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 단일 raw pool/hash/record 와 기존 모드/Player/등록 목록의 입력 장면이다. (2026-11-09 WB04-E161)
class IslandRuntimeScene {
    constructor() {
        this.pool = new SidPool(CAPACITY, { server: false });
        this.hash = new SquidHash();
        this.records = new Uint8Array(ISLAND_STORAGE_BYTES);
        this.modes = createFortFinishOwnerState();
        this.residences = new SquidPostPopList();
        this.revision = { value: 0 };
        this.changed = { value: 0 };
        this.input = [];
        this.events = [];
        this.assertions = 0;
        this.players = new Uint8Array(0);
        this.runtime = null;
    }

    // 가상 owner 의 실제 raw 효과와 callback 직후 현재 목록/모드/Player 변화를 관찰한다. (2026-11-09 WB04-E161)
    hooks() {
        return {
            assertion: (reason) => { this.assertions += 1; this.events.push(reason === 'InvalidBounds' ? 'A:B' : 'A:E'); },
            setResidenceOwner: (sid, owner) => {
                this.pool.physical(sid)[34] = owner & 0xff;
                this.events.push(`O:${sid}:${owner}`);
                if (sid !== 60) return;
                const mutation = this.input[14];
                if (mutation === 1) this.residences.entries[1] = 63;
                if (mutation === 2) this.residences.count = 1;
                if (mutation === 3) this.hash.setCell(0, 51, 53, 72);
                if (mutation === 4) { this.modes.fort = true; this.modes.special = false; }
                if (mutation === 6) {
                    if (this.players.length === 0) this.modes.playerRegions[this.input[6]] = 5;
                    else put(this.players, this.input[6] * PLAYER_STRIDE + PLAYER_FIELD, 5);
                }
            }
        };
    }

    // 기대값 계산 없이 입력 record 의 실제 필드 폭/두 종류 owner 와 기존 목록을 구성한다. (2026-11-09 WB04-E161)
    prepare(row) {
        this.input = row[2].split(',').map(Number);
        this.events = [];
        this.players = new Uint8Array(0);
        this.assertions = 0;
        this.hash.reset();
        this.modes = createFortFinishOwnerState();
        const input = this.input;
        assert.equal(input.length, 15);
        this.records.fill(input[6] & 0xff);
        put(this.records, 0, input[0]);
        // 100개 모두에 원본 record/type/data 입력을 배치한다.
        for (let index = 0; index < 100; index += 1) {
            const record = 4 + index * 32;
            put(this.records, record, 0x11110000 + index);
            put(this.records, record + 4, index < input[0] ? 1 : 0);
            put(this.records, record + 8, input[3]);
            put(this.records, record + 12, input[5], 1);
            put(this.records, record + 13, input[4], 1);
            // 네 float 는 실행기에 전달한 비트를 그대로 저장한다.
            for (let field = 0; field < 4; field += 1) put(this.records, record + 16 + field * 4, RUNTIME_BOUNDS[input[13]][field]);
        }
        if (input[1] < 100) put(this.records, 4 + input[1] * 32 + 4, input[2]);
        this.modes.fort = input[10] !== 0;
        this.modes.special = input[8] !== 0;
        this.modes.chaffMode = input[9] !== 0;
        this.revision.value = input[11] >>> 0;
        this.changed.value = input[12] >>> 0;
        this.residences.entries = [60, 61, 62, 63];
        this.residences.count = 4;
        // 행을 재사용해도 raw 의 이전 입력이 남지 않게 명시 공간을 초기화한다.
        for (let sid = 5; sid <= 127; sid += 1) this.pool.physical(sid).fill(0);
        // 실제 pool/hash 와 기존 표면 helper 에 동일 type/float/표면 owner 입력을 전달한다.
        for (let index = 0; index < 4; index += 1) {
            const raw = this.pool.physical(60 + index);
            raw[10] = 89;
            raw[34] = 7;
            put(raw, 14, floatToBits(RUNTIME_POINTS[index][0]));
            put(raw, 18, floatToBits(RUNTIME_POINTS[index][1]));
            const support = 70 + Math.min(index, 2);
            this.pool.physical(support)[34] = [2, 3, 8][Math.min(index, 2)];
            const x = Math.trunc(RUNTIME_POINTS[index][0] + Math.fround(0.9999));
            const y = Math.trunc(RUNTIME_POINTS[index][1] + Math.fround(0.9999));
            this.hash.setCell(0, x, y, support);
        }
        this.runtime = new RawIslandRuntime(this.pool, this.hash, this.records, this.modes, this.residences, this.revision, this.changed, this.hooks());
    }
}

// 원본 scalar/전체 3204 raw/출력 bounds/공유 전역/등록 목록/가상 owner 를 전수 비교한다. (2026-11-09 WB04-E161)
function replay(sharedPlayers) {
    const scene = new IslandRuntimeScene();
    let count = 0;
    const kinds = new Set();
    // 각 독립 native 입력을 복원 몸체에 전달한다.
    for (const row of rows) {
        assert.equal(row.length, 15);
        scene.prepare(row);
        const island = scene.input[1] | 0;
        let result = 0;
        let bounds = '-';
        if (sharedPlayers) {
            scene.players = new Uint8Array(9 * PLAYER_STRIDE).fill(0xa5);
            // 기대값 대신 기존 입력 상태의 아홉 지역 필드만 준비한다.
            for (let owner = 0; owner < 9; owner += 1) put(scene.players, owner * PLAYER_STRIDE + PLAYER_FIELD, scene.modes.playerRegions[owner] >>> 0);
            scene.runtime.bindPlayerRecords(scene.players);
            scene.modes.playerRegions.fill(999);
        }
        const kind = row[1];
        kinds.add(kind);
        const runtime = scene.runtime;
        if (kind === 'Reset') runtime.reset();
        else if (kind === 'Count') result = runtime.count;
        else if (kind === 'Exists') result = runtime.exists(island);
        else if (kind === 'Add') result = runtime.add(scene.input[6], scene.input[5] & 0xff);
        else if (kind === 'Delete') runtime.delete(island);
        else if (kind === 'OwnerGet') result = runtime.owner(island);
        else if (kind === 'OwnerSet') runtime.setOwner(island, scene.input[6]);
        else if (kind === 'TempleGet') result = runtime.temple(island);
        else if (kind === 'TempleSet') runtime.setTemple(island, scene.input[6]);
        else if (kind === 'Match') result = runtime.matches(island, scene.input[6] | 0, scene.input[7] | 0) ? 1 : 0;
        else if (kind === 'GetBounds') {
            const raw = new Uint8Array(16);
            runtime.boundsBits(island).forEach((bits, field) => put(raw, field * 4, bits));
            bounds = hex(raw);
        } else if (kind === 'SetBounds') runtime.setBoundsBits(island, RUNTIME_BOUNDS[scene.input[13]]);
        const players = [];
        // 현재 Player 의 첫 지역을 읽는다.
        for (let owner = 0; owner < 9; owner += 1) players.push(sharedPlayers ? get(scene.players, owner * PLAYER_STRIDE + PLAYER_FIELD) : scene.modes.playerRegions[owner] >>> 0);
        if (sharedPlayers) {
            // 첫 지역 네 바이트 외의 Player 전부는 그대로 유지되어야 한다.
            for (let index = 0; index < scene.players.length; index += 1) {
                const offset = index % PLAYER_STRIDE;
                if (offset < PLAYER_FIELD || offset >= PLAYER_FIELD + 4) assert.equal(scene.players[index], 0xa5);
            }
            assert.ok(scene.modes.playerRegions.every((value) => value === 999));
        }
        // callback 을 포함한 실제 효과 순서를 저장한다.
        const effects = scene.events.length === 0 ? '-' : scene.events.join(';');
        // 첫 4개 현재 등록 SID 와 실제 owner BYTE 를 비교한다.
        const entries = [];
        const owners = [];
        for (let index = 0; index < 4; index += 1) {
            entries.push(scene.residences.entries[index]);
            owners.push(scene.pool.slot(60 + index)[34]);
        }
        const modes = `${scene.modes.fort ? 1 : 0},${scene.modes.special ? 1 : 0},${scene.modes.chaffMode ? 1 : 0}`;
        const where = `${row[1]} ${row[2]}`;
        assert.equal(result, Number(row[3]), where);
        assert.equal(hex(scene.records), row[4], where);
        assert.equal(bounds, row[5], where);
        assert.equal(scene.revision.value, Number(row[6]), where);
        assert.equal(scene.changed.value, Number(row[7]), where);
        assert.equal(players.join(','), row[8], where);
        assert.equal(effects, row[9], where);
        assert.equal(scene.residences.count >>> 0, Number(row[10]), where);
        assert.equal(entries.join(','), row[11], where);
        assert.equal(owners.join(','), row[12], where);
        assert.equal(modes, row[13], where);
        assert.equal(scene.assertions, Number(row[14]), where);
        count += 1;
    }
    assert.equal(count, ROWS);
    // 모든 종류가 실제로 실행되었는지 확인한다.
    assert.ok(['Reset', 'Count', 'Exists', 'Add', 'Delete', 'OwnerGet', 'OwnerSet', 'TempleGet', 'TempleSet', 'Match', 'GetBounds', 'SetBounds'].every((kind) => kinds.has(kind)), [...kinds].join(','));
}

test('섬 런타임: 원본 10.78 관측 1009행의 scalar·3204 raw·경계 비트·공유 전역·Player 첫 지역·효과 순서가 같다', { skip: rows.length === 0 }, () => { replay(false); });

test('섬 런타임: 같은 1009행을 실제 9 Player BYTE 공유 모드로 재생하면 첫 지역 네 바이트만 바뀌고 모드 사본은 쓰지 않는다', { skip: rows.length === 0 }, () => { replay(true); });

test('섬 런타임: 세션 없는 cleanup 의 Reset 은 기존 Reset 행의 전체 3204바이트와 같고 전역/목록/효과를 건드리지 않는다', { skip: rows.length === 0 }, () => {
    let count = 0;
    // 새 기대값이나 fixture 행을 만들지 않고 기존 Reset 행만 쓴다.
    for (const row of rows.filter((candidate) => candidate[1] === 'Reset')) {
        const scene = new IslandRuntimeScene();
        scene.prepare(row);
        const revision = scene.revision.value;
        const changed = scene.changed.value;
        const entries = scene.residences.entries.slice();
        resetRawIslandRecords(scene.records);
        assert.ok(hex(scene.records) === row[4] && scene.revision.value === revision && scene.changed.value === changed && scene.events.length === 0);
        assert.deepEqual(scene.residences.entries, entries);
        count += 1;
    }
    assert.equal(count, 3);
});

test('섬 런타임: 별도 3204바이트 장기 배열을 실행기가 계속 읽고 쓰며 잘못된 helper 폭은 첫 쓰기 전에 거부한다', () => {
    const scene = new IslandRuntimeScene();
    const storage = createRawIslandStorage().fill(0xa5);
    const runtime = new RawIslandRuntime(scene.pool, scene.hash, storage, scene.modes, scene.residences, scene.revision, scene.changed, scene.hooks());
    resetRawIslandRecords(storage);
    assert.ok(runtime.count === 0 && storage[18] === 0xa5 && storage[19] === 0xa5);
    assert.ok(runtime.add(3, 7) === 0 && get(storage, 0) === 1 && get(storage, 12) === 3 && storage[16] === 7 && storage[17] === 3);
    put(storage, 4, 0x12345678);
    assert.equal(runtime.temple(0), 0x12345678);
    runtime.reset();
    assert.ok(get(storage, 0) === 0 && get(storage, 4) === 0 && storage[18] === 0xa5 && storage[19] === 0xa5);
    const before = Uint8Array.from(storage);
    assert.ok(throws(() => resetRawIslandRecords(storage.subarray(0, ISLAND_STORAGE_BYTES - 1))));
    assert.deepEqual(storage, before);
});

test('섬 런타임: 물리 100기록 경계·필수 효과·Player 기록 크기를 거부한다', () => {
    const scene = new IslandRuntimeScene();
    scene.prepare(rows[0] ?? ['originals', 'Reset', '3,1,1,2,1,2,0,2,0,0,0,0,9,0,0', '0']);
    put(scene.records, 0, 100);
    assert.ok(throws(() => scene.runtime.add(1, 2)));
    assert.equal(scene.runtime.count, 100);
    assert.ok(throws(() => new RawIslandRuntime(scene.pool, scene.hash, scene.records, scene.modes, scene.residences, scene.revision, scene.changed, {})));
    assert.ok(throws(() => scene.runtime.bindPlayerRecords(new Uint8Array(10))));
    assert.ok(throws(() => scene.runtime.boundsBits(100)));
});

test('섬 런타임: 월드 증가 DWORD 와 지도 DWORD 는 서로 다른 소유자로 분리되고 진단 복귀 뒤 현재 값을 다시 읽는다', () => {
    const scene = new IslandRuntimeScene();
    const cloud = { value: 0 };
    const gameplay = { value: 0 };
    const residence = scene.pool.physical(60);
    residence.fill(0);
    residence[10] = 89;
    residence[34] = 7;
    put(residence, 14, floatToBits(50.75));
    put(residence, 18, floatToBits(52.5));
    scene.pool.physical(70)[34] = 2;
    scene.hash.setCell(0, 51, 53, 70);
    scene.residences.entries = [60];
    scene.residences.count = 1;
    let notices = 0;
    let assertions = 0;
    let expectedWorld = 18;
    let expectedOwner = 3;
    let mutateDuringNotice = false;
    const runtime = new RawIslandRuntime(scene.pool, scene.hash, scene.records, scene.modes, scene.residences, cloud, gameplay, {
        assertion: (reason) => {
            assert.equal(reason, 'MissingIsland');
            assertions += 1;
            cloud.value = 40;
            gameplay.value = 0x76543210;
            put(scene.records, 8, 1);
        },
        setResidenceOwner: (sid, owner) => {
            assert.ok(sid === 60 && owner === 2 && cloud.value === expectedWorld && gameplay.value === 1 && get(scene.records, 12) === expectedOwner);
            notices += 1;
            scene.pool.physical(sid)[34] = owner;
            if (mutateDuringNotice) { cloud.value = 700; gameplay.value = 99; }
        }
    });
    runtime.reset();
    assert.equal(runtime.add(1, 0), 0);
    runtime.setBounds(0, { left: 10, top: 20, right: 60, bottom: 70 });
    cloud.value = 17;
    gameplay.value = 0xffffffff;
    runtime.setOwner(0, 3);
    assert.ok(cloud.value === 18 && gameplay.value === 1 && runtime.owner(0) === 3 && scene.pool.slot(60)[34] === 2 && notices === 1 && assertions === 0);
    // 진단의 현재 존재/전역 변경을 재조회한 뒤 같은 1 저장을 먼저 수행하고 가상 효과의 후속 변경은 유지한다.
    put(scene.records, 8, 0);
    cloud.value = 17;
    gameplay.value = 0xffffffff;
    expectedWorld = 41;
    expectedOwner = 4;
    mutateDuringNotice = true;
    runtime.setOwner(0, 4);
    assert.ok(cloud.value === 700 && gameplay.value === 99 && runtime.owner(0) === 4 && notices === 2 && assertions === 1);
    // unsigned 감김은 증가하는 세계 DWORD 에만 적용하며 미니맵 DWORD 는 여전히 1 을 직접 저장한다.
    cloud.value = 0xffffffff;
    gameplay.value = 0xffffffff;
    expectedWorld = 0;
    expectedOwner = 5;
    mutateDuringNotice = false;
    runtime.setOwner(0, 5);
    assert.ok(cloud.value === 0 && gameplay.value === 1 && runtime.owner(0) === 5 && notices === 3 && assertions === 1);
});
