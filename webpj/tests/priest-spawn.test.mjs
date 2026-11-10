// 사제 생성 raw-priest-spawn.js(RawPriestSpawn·PriestSpawnState·makePriestSpawnHooks)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: priestspawn-x86.tsv 10.78(originals) 408행 — 좌표·타입·WORD·spot·배치 응답 패턴·변이 단계·사후 genus → 다리 타입, 하위 호출 기록(Q 배치 검사 / N 생성 / O 소유자 / L Pop / S 표면 알림 / K Carrier 검사), 생성 슬롯 raw, 지도의 Adler-32.
//    CD 10.72·10.37 행은 웹 범위(10.78) 밖이다.
//  - 계약: priest_spawn_guards_and_failure_diagnostics(누락 경계·잘못된 지도/표 크기·비유한 좌표·타입 범위 거부, 132개 후보 소진 진단, 비 Carrier 검사 assert, 다른 풀 거부).
//    종속 해방(RawDamageableRelease)과 실제 사제 소유자까지의 합성(priest_spawn_release_uses_real_factory_and_priest_owner)은 tests/damageable-release-integration.test.mjs 가 대조한다(2026-11-10 WB04-E252).
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E68: 사제 생성 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, floatToBits, bitsToFloat, adler32 } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { RawPriestSpawn, PriestSpawnState, makePriestSpawnHooks } from '../www/core/raw-priest-spawn.js';

// 원본 관측 10.78 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E68)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'priestspawn-x86.tsv'))?.filter((row) => row[0] === 'originals') ?? null;

// 원본 독립 입력 수, 대체 생성 SID(원본 실행기 값), 합성 가상 표 기록값이다. (2026-11-08 WB04-E68)
const PATCH_ROWS = 408;
const SPAWNED = 100;
const SPAWN_VTABLE = 0x20000100;

// 예외가 나는지 본다. (2026-11-08 WB04-E68)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 바이트 배열을 소문자 hex 로 만든다. (2026-11-08 WB04-E68)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 같은 풀·상태·처리기를 조립해 입력과 결과를 대조하는 장면이다. (2026-11-08 WB04-E68)
class SpawnScene {
    constructor() {
        this.pool = new SidPool(24000, { server: false });
        this.types = Array.from({ length: 188 }, () => ({ cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, maxHitPoints: 0 }));
        this.spots = new Uint8Array(65536);
        this.state = new PriestSpawnState();
        this.events = [];
        this.pattern = 0;
        this.queries = 0;
        this.change = 0;
        this.afterGenus = 0;
        // 실행기와 같은 생성 SID 까지 확보한다.
        for (let id = 5; id <= SPAWNED; id += 1) assert.equal(this.pool.allocate(2), id);
        this.raw = this.pool.allocated(SPAWNED);
    }

    // 실제 배치 판단 대신 실행기에 공급한 반환 입력을 같은 순서로 제공한다.
    result() {
        if (this.pattern === 0xffffffff) return 0xffffffff;
        if (this.pattern === -2) return this.queries % 2;
        return this.pattern > 0 && this.queries >= this.pattern ? 1 : 0;
    }

    // 원본에서 대체한 하위 효과만 공급하며 나선/조건/WORD 쓰기는 구현에 맡긴다.
    hooks() {
        return {
            mayPlace: (query) => {
                this.queries += 1;
                const result = this.result();
                assert.ok(query.flags === 0 && query.mode === 0);
                this.events.push(`Q:${query.type}:${floatToBits(query.x)}:${floatToBits(query.y)}:${query.flags}:${query.owner}:${query.mode}:${result}`);
                if (this.change === 1 && this.queries === 1) {
                    const cell = query.y * 256 + query.x;
                    this.spots[cell] = 0;
                    this.spots[cell - 256] = 0;
                }
                if (this.change === 2) this.state.bridgeType = query.type;
                return result !== 0;
            },
            create: (type, flags) => {
                assert.equal(flags, 0);
                this.raw.fill(0xcd);
                put(this.raw, 0, SPAWN_VTABLE);
                this.raw[10] = type;
                this.raw[11] = 0;
                this.events.push(`N:${type}:${flags}:${SPAWNED}`);
                if (this.change === 2) this.state.bridgeType = 82;
                return SPAWNED;
            },
            setOwner: (sid, owner) => { assert.equal(sid, SPAWNED); this.events.push(`O:${sid}:${owner}`); },
            pop: (sid, x, y, flags) => {
                assert.ok(sid === SPAWNED && flags === 0);
                this.events.push(`L:${sid}:${floatToBits(x)}:${floatToBits(y)}:${flags}`);
                if (this.change === 3) { this.raw[10] = 82; this.types[82].flags2 = this.afterGenus; }
            },
            notifySurface: (sid) => { assert.equal(sid, SPAWNED); this.events.push(`S:${sid}`); },
            checkCarrier: (sid) => { assert.equal(sid, SPAWNED); this.events.push(`K:${sid}`); }
        };
    }

    // 입력 칸만으로 공간/반환/전역/초기 raw 를 구성한다.
    prepare(row) {
        assert.equal(row.length, 13);
        this.events = [];
        this.queries = 0;
        this.pattern = Number(row[6]);
        this.change = Number(row[7]);
        this.afterGenus = Number(row[8]);
        this.state.bridgeType = 82;
        this.state.checkingPlacement = 0;
        this.types[158].flags2 = this.types[82].flags2 = 0x210000;
        this.spots.fill(Number(row[5]));
        this.raw.fill(0xcd);
    }
}

test('PriestSpawn: 원본 10.78 408행의 나선·스냅·WORD·소유자·Pop·Carrier 검사 호출 기록과 슬롯 raw·지도 checksum 이 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    const scene = new SpawnScene();
    let count = 0;
    // 기대 위치를 구현에서 계산하지 않고 저장된 실제 관찰과 대조한다.
    for (const row of rows) {
        scene.prepare(row);
        new RawPriestSpawn(scene.pool, scene.types, scene.spots, scene.state, scene.hooks()).spawn(bitsToFloat(Number(row[1])), bitsToFloat(Number(row[2])), Number(row[3]), Number(row[4]));
        assert.equal(scene.state.bridgeType, Number(row[9]), `행 ${count}`);
        assert.equal(scene.events.join(';'), row[10], `행 ${count}`);
        assert.equal(hex(scene.pool.slot(SPAWNED)), row[11], `행 ${count}`);
        assert.equal(adler32(scene.spots), Number(row[12]), `행 ${count}`);
        count += 1;
    }
    assert.equal(count, PATCH_ROWS);
});

test('PriestSpawn: 누락 경계·잘못된 지도/표 크기·비유한 좌표·타입 범위와 132개 후보 소진 진단, 비 Carrier 검사 assert, 다른 풀 연결을 거부한다', { skip: !rows && '원본 fixture 없음' }, () => {
    const scene = new SpawnScene();
    scene.prepare(rows[0]);
    const missing = scene.hooks();
    delete missing.mayPlace;
    assert.ok(throws(() => new RawPriestSpawn(scene.pool, scene.types, scene.spots, scene.state, missing)));
    assert.ok(throws(() => new RawPriestSpawn(scene.pool, scene.types, new Uint8Array(1), scene.state, scene.hooks())));
    assert.ok(throws(() => new RawPriestSpawn(scene.pool, scene.types.slice(0, 1), scene.spots, scene.state, scene.hooks())));
    const spawn = new RawPriestSpawn(scene.pool, scene.types, scene.spots, scene.state, scene.hooks());
    assert.ok(throws(() => spawn.spawn(bitsToFloat(0x7fc00000), 21, 158, 1)));
    assert.ok(throws(() => spawn.spawn(20, Infinity, 158, 1)));
    assert.ok(throws(() => spawn.spawn(20, 21, 6, 1)));
    assert.ok(throws(() => spawn.spawn(20, 21, 255, 1)));
    assert.deepEqual(scene.events, []);
    // 모든 칸이 지면(6)이고 배치가 항상 거부되면 132개 후보를 확인한 뒤 현재 debug 전역에 따라 진단한다.
    scene.spots.fill(6);
    scene.pattern = 0;
    scene.state.checkingPlacement = 1;
    assert.ok(throws(() => spawn.spawn(20, 21, 158, 1)));
    assert.ok(scene.queries === 132 && !scene.events.some((event) => event.startsWith('N:')));
    scene.state.checkingPlacement = 0;
    scene.queries = 0;
    spawn.spawn(20, 21, 158, 1);
    assert.equal(scene.queries, 132);
    // 패치는 non-Carrier 검사가 항상 assert 이다.
    scene.events = [];
    scene.spots.fill(0);
    scene.types[82].flags2 = 0;
    assert.ok(throws(() => spawn.spawn(20, 21, 82, 1)));
    assert.ok(!scene.events.some((event) => event.startsWith('K:')) && scene.events.some((event) => event.startsWith('N:')));
    const other = new SidPool(24000, { server: false });
    assert.ok(throws(() => makePriestSpawnHooks(other, spawn, {})));
    const bound = makePriestSpawnHooks(scene.pool, spawn, { keep: 1 });
    assert.equal(bound.keep, 1);
    assert.equal(typeof bound.spawnPriest, 'function');
});
