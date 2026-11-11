// Fort 단일 랜덤 배치(cpppj `o/RawFortRandomPlacement`, 원본 0040db90)와 edgeFarm 군집 배치(`o/RawFarmCluster`, 원본 0040e560)의 웹 대응 `core/raw-fort-random-placement.js` · `core/raw-farm-cluster.js` 를 검사한다(4-B, WB04-E285).
//  - 고정 관측 `farmcluster-x86.tsv` originals 104행: 실제 단일 풀(32768)·hash0·spot 지도·타입(isle 97 · edgeFarm 121)·청크 지역·두 난수에서 군집을 실행해 배치 시도 횟수 · 단일 진입/반환 DWORD trace 의 Adler · 원시 효과(공간 L · 지역 owner I · 생성 C · 생성자 K ·
//    owner O · Unpop U · Repop R · Pop P) 순서 Adler · 변경된 지면과 새 객체의 전체 raw · 전역/local 난수 상태가 모두 같다. 이 재생이 단일 배치의 모든 분기(spot·edgeFarm 존재·공간·지역·지면 교체·임의 프레임·owner)를 함께 검증한다.
//    같은 104행을 Login.bb4 와 다른 bb0 DWORD 를 직접 빌린 archipelago 로도 다시 재생한다(cpppj GameAux_FarmClusterUsesSharedArchipelagoNative104). CD 104행은 웹 목표(10.78) 밖이다.
//  - 계약: 구성 거부(판본·표 크기·필수 효과·다른 월드)와 단일 배치의 경계(반올림 spot 범위·프레임 −1 assert·owner 덮어쓰기·force)를 직접 검사한다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-11 WB04-E285)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, adler32, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash, SQUID_HASH_SCALES } from '../www/core/squid-hash.js';
import { SquidFactory } from '../www/core/squid-factory.js';
import { GameRandom } from '../www/core/random.js';
import { RawIslandRuntime, createFortFinishOwnerState, ISLAND_STORAGE_BYTES } from '../www/core/raw-island-runtime.js';
import { SquidPostPopList } from '../www/core/squid-postpop-state.js';
import { typeConstructorAddress } from '../www/core/squid-constructors.js';
import { FortRandomPlacementState, RawFortRandomPlacement, makeFortSinglePlacementHook } from '../www/core/raw-fort-random-placement.js';
import { FarmClusterState, RawFarmCluster, makeFarmClusterPlacementHook } from '../www/core/raw-farm-cluster.js';

// 원본 관측 파일(originals 행만)이다. 파일이 없으면 해당 검사를 건너뛴다. (2026-11-11 WB04-E285)
const ROWS = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'farmcluster-x86.tsv'))?.filter((row) => row[0] === 'originals') ?? null;

// 독립 native 입력의 지면 범위/시작 SID 와 생성 SID·원본 vtable 입력이다. (2026-11-11 WB04-E285)
const FIRST = 60;
const LAST = 132;
const SIDE = LAST - FIRST + 1;
const GROUND = 200;
const BORN = 10000;
const VTABLE = 0x13000000;

// 비정렬 DWORD 읽기/쓰기와 16진 표기이다. (2026-11-11 WB04-E285)
const get = (bytes, at, width = 4) => {
    let value = 0;
    for (let byte = 0; byte < width; byte += 1) value += bytes[at + byte] * 2 ** (8 * byte);
    return value;
};
function put(bytes, at, value, width = 4) {
    for (let byte = 0; byte < width; byte += 1) bytes[at + byte] = Math.floor(value / 2 ** (8 * byte)) & 255;
}
const hex = (bytes) => Array.from(bytes, (byte) => byte.toString(16).padStart(2, '0')).join('');

// 예외가 나는지 본다. (2026-11-11 WB04-E285)
function throws(action) {
    try {
        action();
        return false;
    } catch {
        return true;
    }
}

// 빈 타입 레코드이다(factory/finder 가 읽는 칸). (2026-11-11 WB04-E285)
const blankType = () => ({ name: '', constructorAddress: 0, flags1: 0, flags2: 0, maxHitPoints: 0, zOrder: 0, footX: 0, footY: 0 });

// 실제 단일 풀/hash/spot/type/지역/두 난수에서 군집과 단일 배치를 조립하는 장면이다. (2026-11-11 WB04-E285)
class FarmScene {
    constructor() {
        this.pool = new SidPool(32768, { server: false });
        this.hash = new SquidHash();
        this.spots = new Uint8Array(65536);
        this.types = Array.from({ length: 188 }, blankType);
        this.frames = Array.from({ length: 188 }, () => ({ codes: [] }));
        this.random = new GameRandom();
        this.placementState = new FortRandomPlacementState();
        this.state = new FarmClusterState();
        this.entries = Array.from({ length: 256 }, () => ({ island: 0, record: new Uint8Array(6) }));
        this.chunks = { xLength: 16, yLength: 16, at: (x, y) => this.entries[y * 16 + x] };
        this.records = new Uint8Array(ISLAND_STORAGE_BYTES);
        this.modes = createFortFinishOwnerState();
        this.residences = new SquidPostPopList();
        const missing = () => { throw new Error('군집 예상 밖 IslandRuntime 효과'); };
        this.islands = new RawIslandRuntime(this.pool, this.hash, this.records, this.modes, this.residences, { value: 0 }, { value: 0 }, { setResidenceOwner: missing, assertion: missing });
        this.events = [];
        this.trace = [];
        this.input = [];
        this.born = 0;
        this.attempts = 0;
        this.lastLocal = 0;
    }

    // 경계 효과의 원본 순서/인자를 같은 콜론 형식으로 저장한다(모든 값은 unsigned DWORD). (2026-11-11 WB04-E285)
    event(kind, ...values) {
        this.events.push([kind, ...values.map((value) => value >>> 0)].join(':'));
    }

    // 현재 type 이 표면이면 level 0, 일반 1칸 edgeFarm 이면 level 1 에 등록하는 명시 공간 경계이다. (2026-11-11 WB04-E285)
    level(sid) {
        return this.types[this.pool.slot(sid)[10]].flags2 & 6 ? 0 : 1;
    }

    // 실제 Unpop 경계는 현재 WORD 체인에서 같은 SID 를 제거한다. (2026-11-11 WB04-E285)
    unhash(sid) {
        const raw = this.pool.slot(sid);
        const x = Math.trunc(new DataView(raw.buffer, raw.byteOffset).getFloat32(14, true));
        const y = Math.trunc(new DataView(raw.buffer, raw.byteOffset).getFloat32(18, true));
        const level = this.level(sid);
        const scale = SQUID_HASH_SCALES[level];
        const cx = Math.trunc(x / scale);
        const cy = Math.trunc(y / scale);
        let current = this.hash.cell(level, cx, cy);
        let previous = 0;
        // 실제 next 를 따라가며 해당 SID 이외의 후행 객체를 유지한다.
        while (current) {
            const after = get(this.pool.slot(current), 4, 2);
            if (current === sid) {
                if (previous) put(this.pool.physical(previous), 4, after, 2);
                else this.hash.setCell(level, cx, cy, after);
                put(this.pool.physical(sid), 4, 0, 2);
                return;
            }
            previous = current;
            current = after;
        }
    }

    // 현재 type/원래 좌표를 다시 읽어 같은 hash 에 등록한다. (2026-11-11 WB04-E285)
    rehash(sid) {
        const raw = this.pool.slot(sid);
        const view = new DataView(raw.buffer, raw.byteOffset);
        const level = this.level(sid);
        const scale = SQUID_HASH_SCALES[level];
        const cx = Math.trunc(Math.trunc(view.getFloat32(14, true)) / scale);
        const cy = Math.trunc(Math.trunc(view.getFloat32(18, true)) / scale);
        put(this.pool.physical(sid), 4, this.hash.cell(level, cx, cy), 2);
        this.hash.setCell(level, cx, cy, sid);
    }

    // 실제 Place 의 legal/factory/가상 공간·owner 경계를 기계어 실행기와 동일한 명시 입력으로 연결한다. (2026-11-11 WB04-E285)
    placementHooks() {
        return {
            mayPlace: (query) => {
                this.event('L', query.type, query.argument, floatToBits(query.x), floatToBits(query.y), query.flags, query.owner, query.mode);
                return this.input[6] !== 0;
            },
            chunkRegion: (x, y) => (this.chunks.at(x, y).island << 24) >> 24,
            regionOwner: (region) => {
                this.event('I', region);
                return this.islands.owner(region);
            },
            create: (type, flags) => {
                const sid = BORN + this.born;
                this.born += 1;
                const raw = this.pool.physical(sid);
                // 명시 factory 입력은 원본과 같은 잔여 payload 를 제공한다.
                for (let index = 0; index < raw.length; index += 1) raw[index] = (index * 37 + 11) & 255;
                put(raw, 0, VTABLE);
                raw[10] = type;
                raw[11] = 4;
                put(raw, 6, 0, 2);
                this.event('C', type, flags, sid);
                return sid;
            },
            construct: (sid, type) => {
                this.event('K', sid, type);
                this.factory.construct(type, sid);
            },
            hasHitPoints: (sid) => (this.types[this.pool.slot(sid)[10]].flags1 & 0x10) !== 0,
            typeHitPoints: () => 803,
            setOwner: (sid, owner) => {
                this.event('O', sid, owner);
                put(this.pool.physical(sid), 34, owner, 1);
            },
            unpop: (sid, flags) => {
                this.event('U', sid, flags);
                this.unhash(sid);
                this.pool.physical(sid)[11] |= 4;
            },
            repop: (sid, flags) => {
                this.event('R', sid, flags);
                this.pool.physical(sid)[11] &= ~4 & 255;
                this.rehash(sid);
            },
            pop: (sid, x, y, flags) => {
                this.event('P', sid, floatToBits(x), floatToBits(y), flags);
                const raw = this.pool.physical(sid);
                put(raw, 14, floatToBits(x));
                put(raw, 18, floatToBits(y));
                raw[11] &= ~4 & 255;
                this.rehash(sid);
            },
        };
    }

    // 원본 배치 entry/return 을 실제 place 의 앞뒤에서 기록한다. (2026-11-11 WB04-E285)
    clusterHooks() {
        const hooks = makeFarmClusterPlacementHook(this.pool, this.placement, {});
        const actual = hooks.place;
        hooks.place = (local, request) => {
            this.attempts += 1;
            this.lastLocal = local.state;
            const values = [this.lastLocal, request.type, floatToBits(request.x), floatToBits(request.y), request.region >>> 0, request.mode, request.force];
            // 단일 진입의 원본 DWORD 일곱 개를 순서대로 기록한다.
            for (const value of values) this.trace.push(...[0, 8, 16, 24].map((shift) => (value >>> shift) & 255));
            const result = actual(local, request);
            this.trace.push(...[0, 8, 16, 24].map((shift) => (result >>> shift) & 255));
            return result;
        };
        return hooks;
    }

    // 원본 입력만 구성하며 군집 결과는 실제 두 복원 몸체가 만든다. row 의 두 번째 열이 12개 DWORD 입력이다. (2026-11-11 WB04-E285)
    prepare(row, shared = null) {
        this.input = row[1].split(',').map(Number);
        assert.equal(this.input.length, 12);
        this.events = [];
        this.trace = [];
        this.born = 0;
        this.attempts = 0;
        this.lastLocal = 0;
        this.hash.reset();
        this.records.fill(0);
        this.types = Array.from({ length: 188 }, blankType);
        this.frames = Array.from({ length: 188 }, () => ({ codes: [] }));
        this.random.setState(this.input[8]);
        this.state = new FarmClusterState();
        this.state.archipelago = this.input[3] !== 0;
        this.state.archipelagoSpan = this.input[4] | 0;
        if (shared) {
            this.state.sharedArchipelago = () => this.input[3];
            this.state.archipelago = this.input[3] === 0; // 반대 bool 사본은 무시되어야 한다.
        }
        this.spots.fill(this.input[6] !== 0 ? 1 : 0);
        Object.assign(this.types[97], { flags1: 0x10, flags2: 2, footX: 1, footY: 1 });
        Object.assign(this.types[121], { flags1: this.input[5] ? 0x100010 : 0x200010, flags2: 0x80000, footX: 1, footY: 1, constructorAddress: this.input[11] ? typeConstructorAddress(121) : 0 });
        this.frames[121] = { codes: [{ side: 80, variant: 80, number: 0, flags: 0 }, { side: 80, variant: 80, number: 1, flags: 255 }, { side: 65, variant: 65, number: 0, flags: 0 }] };
        // 같은 정수 좌표 ground 를 실제 raw/header/hash0 에 배치한다.
        for (let x = FIRST; x <= LAST; x += 1) {
            for (let y = FIRST; y <= LAST; y += 1) {
                const sid = GROUND + (x - FIRST) * SIDE + y - FIRST;
                const raw = this.pool.physical(sid);
                raw.fill(0);
                put(raw, 0, VTABLE);
                raw[10] = 97;
                put(raw, 14, floatToBits(x));
                put(raw, 18, floatToBits(y));
                put(raw, 36, 7);
                put(raw, 34, this.input[7], 1);
                this.hash.setCell(0, x, y, sid);
            }
        }
        // 현재 지역 record 의 seed BYTE 는 별도 입력이다.
        for (const entry of this.entries) {
            entry.island = (this.input[9] ? this.input[1] : 127) & 255;
            entry.record = Uint8Array.of(0, 0, 0, this.input[2] & 255, 0, 0);
        }
        put(this.records, 0, 100);
        // 기존 Owner/Exists 가 읽는 실제 100기록이다.
        for (let region = 0; region < 100; region += 1) {
            put(this.records, 4 + region * 32 + 4, this.input[10]);
            put(this.records, 4 + region * 32 + 8, this.input[7]);
        }
        this.factory = new SquidFactory(this.pool, this.types);
        this.placement = new RawFortRandomPlacement(this.pool, this.hash, this.spots, this.types, this.frames, this.random, this.placementState, this.placementHooks());
        this.cluster = new RawFarmCluster(this.pool, this.chunks, this.islands, this.placementState, this.state, this.clusterHooks());
    }

    // 실제 변경된 ground/생성 객체의 전체 raw 를 원본과 같은 형식으로 읽는다. (2026-11-11 WB04-E285)
    changedRaw() {
        const parts = [];
        // 군집 후 타입이 달라진 실제 ground 만 읽는다.
        for (let number = GROUND; number < GROUND + SIDE * SIDE; number += 1) {
            const raw = this.pool.slot(number);
            if (raw[10] !== 97) parts.push(`${number}:${hex(raw)}`);
        }
        // 실제 factory 경계가 제공한 새 슬롯도 포함한다.
        for (let number = BORN; number < BORN + this.born; number += 1) parts.push(`${number}:${hex(this.pool.slot(number))}`);
        return parts.length ? parts.join(';') : '-';
    }
}

// 모든 군집 입력의 배치 시도/trace/primitive 효과/raw/두 난수를 비교한다. (2026-11-11 WB04-E285)
function replay(shared) {
    const scene = new FarmScene();
    let count = 0;
    // 실제 PE 에서 관찰한 모든 고유 입력을 순회한다.
    for (const row of ROWS) {
        assert.equal(row.length, 9);
        scene.prepare(row, shared);
        scene.cluster.place({ left: 80.25, top: 80.75, right: 96.625, bottom: 96.125 }, scene.input[1] | 0, scene.input[0]);
        const effects = Uint8Array.from(Buffer.from(scene.events.join(';'), 'latin1'));
        const where = `행 ${count + 1}: ${row[1]}`;
        assert.equal(scene.attempts, Number(row[2]), `시도 ${where}`);
        assert.equal(adler32(Uint8Array.from(scene.trace)), Number(row[3]), `trace ${where}`);
        assert.equal(adler32(effects), Number(row[4]), `효과 ${where}`);
        assert.equal(scene.changedRaw(), row[5], `raw ${where}`);
        assert.equal(scene.random.state, Number(row[6]), `전역 난수 ${where}`);
        assert.equal(scene.lastLocal, Number(row[7]), `local 난수 ${where}`);
        count += 1;
    }
    assert.equal(count, 104);
}

// 패치 군집 104개를 기존 실제 단일 Place/efarmCtor 와 같은 풀에서 대조한다. (2026-11-11 WB04-E285)
test('FarmCluster: 원본 10.78 군집 104입력의 시도·진입/반환 trace·효과 순서·raw·두 난수가 같다', { skip: ROWS ? false : 'farmcluster-x86.tsv 없음' }, () => {
    replay(false);
});

// Login.bb4 와 다른 bb0 DWORD 를 직접 빌린 archipelago 로 같은 104행을 재생한다(반대 bool 사본은 무시). (2026-11-11 WB04-E285)
test('FarmCluster: 공유 archipelago(bb0)를 직접 빌려도 원본 104입력이 같다', { skip: ROWS ? false : 'farmcluster-x86.tsv 없음' }, () => {
    replay(true);
});

// 필수 단일 배치/같은 월드 runtime 연결이 빠지면 거부한다(cpppj FarmCluster_RejectsMissingEffectsAndOtherWorld). (2026-11-11 WB04-E285)
test('FarmCluster 계약: 필수 단일 배치 효과 누락과 다른 월드는 거부한다', () => {
    const scene = new FarmScene();
    assert.ok(throws(() => new RawFarmCluster(scene.pool, scene.chunks, scene.islands, scene.placementState, scene.state, {})));
    const other = new SidPool(32768, { server: false });
    assert.ok(throws(() => new RawFarmCluster(other, scene.chunks, scene.islands, scene.placementState, scene.state, { place: () => 0 })));
    assert.ok(throws(() => makeFarmClusterPlacementHook(other, { pool: scene.pool }, {})));
    assert.ok(throws(() => makeFortSinglePlacementHook(other, { pool: scene.pool }, {})));
});

// 계약: 단일 배치의 구성 거부와 경계(반올림 spot 범위·프레임 −1 assert·owner 덮어쓰기·force·모서리 거부 1). (2026-11-11 WB04-E285)
test('Fort 단일 배치 계약: 구성 거부·spot 범위·프레임 −1·owner 덮어쓰기·force·지역 불일치', () => {
    const scene = new FarmScene();
    const row = ROWS ? ROWS[0] : ['originals', '1,0,0,0,1,0,1,2,305419896,1,1,0'];
    scene.prepare(row);
    const hooks = scene.placementHooks();
    const build = (overrides = {}) => new RawFortRandomPlacement(scene.pool, scene.hash, scene.spots, scene.types, scene.frames, scene.random, scene.placementState, { ...hooks, ...overrides });
    // 구성 거부: 필수 효과·표 크기·spot 크기·판본 없는 풀.
    assert.ok(throws(() => build({ pop: null })));
    assert.ok(throws(() => new RawFortRandomPlacement(scene.pool, scene.hash, scene.spots.subarray(0, 65535), scene.types, scene.frames, scene.random, scene.placementState, hooks)));
    assert.ok(throws(() => new RawFortRandomPlacement(scene.pool, scene.hash, scene.spots, scene.types.slice(0, 187), scene.frames, scene.random, scene.placementState, hooks)));
    assert.ok(throws(() => new RawFortRandomPlacement(scene.pool, scene.hash, scene.spots, scene.types, scene.frames.slice(0, 187), scene.random, scene.placementState, hooks)));
    // 반올림 spot 이 보드 밖이면 거부한다(x 255.6 → 256, x −1.5 → −1).
    const placement = build();
    assert.ok(throws(() => placement.place(scene.random, { type: 121, x: 255.6, y: 80, region: 0, mode: 0, force: 1 })));
    assert.ok(throws(() => placement.place(scene.random, { type: 121, x: -1.5, y: 80, region: 0, mode: 0, force: 1 })));
    // 임의 프레임 후보가 없으면(프레임 코드 없음) −1 이 되어 assert 로 거부한다. 거부 전 생성은 일어나지 않는다.
    scene.events.length = 0;
    scene.types[121].flags1 = 0x100010;
    scene.frames[121] = { codes: [] };
    scene.spots.fill(1);
    scene.input[6] = 1;
    assert.ok(throws(() => build().place(scene.random, { type: 121, x: 90, y: 90, region: 0, mode: 0, force: 1 })));
    assert.ok(!scene.events.some((event) => event.startsWith('C:')));
    // owner 덮어쓰기와 force: 일반 프레임 타입을 force 로 놓으면 owner 0 이고, 덮어쓰기 전역이 있으면 그 값이다.
    scene.types[121].flags1 = 0x10;
    scene.hash.reset();
    scene.events.length = 0;
    let sid = build().place(scene.random, { type: 121, x: 90, y: 90, region: 0, mode: 0, force: 1 });
    assert.ok(sid >= BORN);
    assert.ok(scene.events.includes(`O:${sid}:0`));
    scene.hash.reset();
    scene.events.length = 0;
    scene.placementState.ownerOverride = 5;
    sid = build().place(scene.random, { type: 121, x: 91, y: 91, region: 0, mode: 0, force: 1 });
    assert.ok(scene.events.includes(`O:${sid}:5`));
    // force 가 아니면 청크 지역이 요청 지역과 다를 때 0 이다(공간 검사는 이미 통과).
    scene.placementState.ownerOverride = 0;
    scene.entries.forEach((entry) => { entry.island = 3; });
    scene.events.length = 0;
    assert.equal(build().place(scene.random, { type: 121, x: 92, y: 92, region: 4, mode: 0, force: 0 }), 0);
    assert.ok(scene.events.some((event) => event.startsWith('L:')));
    assert.ok(!scene.events.some((event) => event.startsWith('C:')));
    // 모서리 규칙 경계: 보드 밖 좌표는 거부하고 지면이 없는 좌표는 교체할 수 없는 쪽(모서리 조건 없음 = true)이다.
    assert.ok(throws(() => placement.mayReplace(256, 0)));
    assert.ok(throws(() => placement.mayReplace(0, -1)));
    scene.hash.reset();
    assert.equal(placement.mayReplace(100, 100), true);
});

// 계약 2: 단일 배치의 경계 입력 — 반올림 spot 위치·spot 하위 6비트 마스크·청크 좌표 순서·지면 없음 assert·지면 프레임 읽기·후보 256 상한·프레임 코드 패스 순서. 원본 104행은 균일한 spot 지도와 모서리 모드만 써서 구별하지 못한다. (2026-11-11 WB04-E285)
test('Fort 단일 배치 계약 2: 반올림 spot·spot 마스크·청크 좌표·지면 assert·지면/임의 프레임 선택 경계', () => {
    const rowInput = ROWS ? ROWS[0] : ['originals', '1,0,0,0,1,0,1,2,305419896,1,1,0'];
    // 매 경우 새 장면을 만든다(지면 5329개 등록). 요청 TYPE 121 은 기본이 지면 프레임(0x200000)이다.
    const fresh = (adjust = null) => {
        const scene = new FarmScene();
        scene.prepare(rowInput);
        scene.spots.fill(0);
        const hooks = scene.placementHooks();
        if (adjust) adjust(scene, hooks);
        const placement = new RawFortRandomPlacement(scene.pool, scene.hash, scene.spots, scene.types, scene.frames, scene.random, scene.placementState, hooks);
        return { scene, placement };
    };
    const frameOf = (scene, sid) => get(scene.pool.slot(sid), 36);
    // 반올림 spot: (90.6, 90.4) 는 (91, 90) 칸이다. 그 칸만 0x20 이면 통과하고(+0.5 없이는 (90,90) 이라 0), 하위 6비트 밖(0x40)이면 0 이다.
    let { scene, placement } = fresh();
    scene.spots[90 * 256 + 91] = 0x20;
    assert.ok(placement.place(scene.random, { type: 121, x: 90.6, y: 90.4, region: 0, mode: 0, force: 1 }) >= BORN);
    ({ scene, placement } = fresh());
    scene.spots[90 * 256 + 91] = 0x40;
    assert.equal(placement.place(scene.random, { type: 121, x: 90.6, y: 90.4, region: 0, mode: 0, force: 1 }), 0);
    // 청크 좌표: force 가 아니면 chunkRegion(ftol(x)/16, ftol(y)/16) 를 부른다(x=90 → 5, y=70 → 4).
    const calls = [];
    ({ scene, placement } = fresh((s, hooks) => {
        s.spots.fill(1);
        hooks.chunkRegion = (cx, cy) => { calls.push([cx, cy]); return 0; };
    }));
    assert.ok(placement.place(scene.random, { type: 121, x: 90, y: 70, region: 0, mode: 0, force: 0 }) >= BORN);
    assert.deepEqual(calls, [[5, 4]]);
    // 지면 프레임 타입인데 지면이 없으면 assert 로 거부한다.
    ({ scene, placement } = fresh());
    scene.spots.fill(1);
    scene.hash.reset();
    assert.ok(throws(() => placement.place(scene.random, { type: 121, x: 90, y: 90, region: 0, mode: 0, force: 1 })));
    // mode 0 의 지면 프레임: 지면(프레임 7)의 현재 프레임을 새 객체가 가진다.
    ({ scene, placement } = fresh());
    scene.spots.fill(1);
    const sid = placement.place(scene.random, { type: 121, x: 90, y: 90, region: 0, mode: 0, force: 1 });
    assert.equal(frameOf(scene, sid), 7);
    // 임의 프레임: 후보는 최대 256 개이고 전역 난수 한 번으로 고른다(300개 모두 일치 → 난수 limit 256).
    ({ scene, placement } = fresh());
    scene.spots.fill(1);
    scene.types[121].flags1 = 0x100010;
    scene.frames[121] = { codes: Array.from({ length: 300 }, (_, index) => ({ side: 80, variant: 80, number: index & 255, flags: 0 })) };
    scene.random.setState(12345);
    const expected = new GameRandom(12345).next(256);
    assert.equal(frameOf(scene, placement.place(scene.random, { type: 121, x: 90, y: 90, region: 0, mode: 0, force: 1 })), expected);
    // 프레임 코드 패스: P/P 가 있으면 그것만(pass 0), 없고 P/A 가 있으면 그것만(pass 2), 둘 다 없으면 P 방향 아무 변형(pass 3).
    const pick = (codes) => {
        ({ scene, placement } = fresh());
        scene.spots.fill(1);
        scene.types[121].flags1 = 0x100010;
        scene.frames[121] = { codes };
        return () => placement.place(scene.random, { type: 121, x: 90, y: 90, region: 0, mode: 0, force: 1 });
    };
    let run = pick([{ side: 80, variant: 80, number: 0, flags: 0 }, { side: 80, variant: 65, number: 1, flags: 0 }, { side: 80, variant: 66, number: 2, flags: 0 }]);
    assert.equal(frameOf(scene, run()), 0);
    run = pick([{ side: 80, variant: 66, number: 0, flags: 0 }, { side: 80, variant: 65, number: 1, flags: 0 }]);
    assert.equal(frameOf(scene, run()), 1);
    run = pick([{ side: 80, variant: 66, number: 0, flags: 0 }, { side: 65, variant: 80, number: 1, flags: 0 }]);
    assert.equal(frameOf(scene, run()), 0);
    // 후보가 하나도 없으면(P 방향 코드 없음) 프레임 −1 assert 로 거부한다.
    run = pick([{ side: 65, variant: 65, number: 0, flags: 0 }]);
    assert.ok(throws(run));
});

// 계약 3: 모서리 규칙의 네 방향·보드 끝 경계·Retype 의 void/foot assert·finder 의 지면 건너뛰기 플래그·CRT 좌표 범위. (2026-11-11 WB04-E285)
test('Fort 단일 배치 계약 3: 모서리 네 방향·보드 끝·Retype assert·표면 건너뛰기·좌표 범위', () => {
    const rowInput = ROWS ? ROWS[0] : ['originals', '1,0,0,0,1,0,1,2,305419896,1,1,0'];
    const build = () => {
        const scene = new FarmScene();
        scene.prepare(rowInput);
        scene.hash.reset(); // 지면을 모두 지우고 필요한 칸만 다시 등록한다.
        const hooks = scene.placementHooks();
        const placement = new RawFortRandomPlacement(scene.pool, scene.hash, scene.spots, scene.types, scene.frames, scene.random, scene.placementState, hooks);
        let next = GROUND;
        // (x, y) 칸에 지면(isle 97) 객체를 놓는다.
        const ground = (x, y) => {
            const sid = next;
            next += 1;
            const raw = scene.pool.physical(sid);
            raw.fill(0);
            put(raw, 0, VTABLE);
            raw[10] = 97;
            put(raw, 14, floatToBits(x));
            put(raw, 18, floatToBits(y));
            scene.hash.setCell(0, x, y, sid);
            return sid;
        };
        return { scene, placement, ground };
    };
    // 모서리 네 방향: 대각 칸이 비고 인접 두 칸이 있으면 그 방향이 교체를 막는다(다른 방향 조건은 일부러 성립하지 않게 한다).
    const cases = [
        { name: 'NW', cells: [[0, -1], [-1, 0]] },
        { name: 'NE', cells: [[0, -1], [1, 0]] },
        { name: 'SE', cells: [[0, 1], [1, 0]] },
        { name: 'SW', cells: [[0, 1], [-1, 0]] },
    ];
    for (const { name, cells } of cases) {
        const { placement, ground } = build();
        for (const [dx, dy] of cells) ground(100 + dx, 100 + dy);
        assert.equal(placement.mayReplace(100, 100), false, name);
    }
    // 대각 칸까지 모두 있으면 교체할 수 있다.
    {
        const { placement, ground } = build();
        for (let dx = -1; dx <= 1; dx += 1) for (let dy = -1; dy <= 1; dy += 1) ground(100 + dx, 100 + dy);
        assert.equal(placement.mayReplace(100, 100), true);
    }
    // 보드 끝: x=255 의 오른쪽(256) 칸은 보드 밖이라 없는 칸이다(다음 줄 첫 칸이 있어도 읽지 않는다).
    {
        const { placement, ground } = build();
        ground(255, 101);
        ground(0, 101);
        assert.equal(placement.mayReplace(255, 100), true);
    }
    // Retype: void 상태가 아니거나 foot 이 다르면 거부하고, 통과하면 raw 타입을 바꾸고 현재 생성자를 한 번 재실행한다.
    {
        const { scene, placement } = build();
        scene.spots.fill(1);
        const sid = scene.placementHooks().create(121, 0); // 새 객체는 void 상태(4)이다.
        scene.events.length = 0;
        placement.retype(sid, 121);
        assert.deepEqual(scene.events, [`K:${sid}:121`]);
        scene.pool.physical(sid)[11] = 0;
        assert.ok(throws(() => placement.retype(sid, 121)));
        scene.pool.physical(sid)[11] = 4;
        scene.types[97].footX = 2;
        scene.pool.physical(sid)[10] = 97;
        assert.ok(throws(() => placement.retype(sid, 121)));
    }
    // 교체한 자리: 지면이 edgeFarm 으로 바뀌어 일반 공간(level 1)에 다시 등록되므로 같은 자리에 다시 놓으면 edgeFarm 존재 검사에서 0 이다(표면 level 0 은 건너뛴다).
    {
        const { scene, placement, ground } = build();
        scene.spots.fill(1);
        for (let dx = -1; dx <= 1; dx += 1) for (let dy = -1; dy <= 1; dy += 1) ground(90 + dx, 90 + dy);
        const first = placement.place(scene.random, { type: 121, x: 90, y: 90, region: 0, mode: 1, force: 1 });
        assert.ok(first >= GROUND && first < GROUND + 9);
        assert.equal(placement.place(scene.random, { type: 121, x: 90, y: 90, region: 0, mode: 1, force: 1 }), 0);
    }
    // CRT 좌표 범위: 비유한/±9.2e18 이상은 거부한다.
    {
        const { scene, placement } = build();
        scene.spots.fill(1);
        scene.types[121].flags1 = 0x10; // 지면 프레임이 아닌 타입이라 범위 검사가 없으면 배치가 끝까지 진행된다.
        for (const x of [Infinity, -Infinity, NaN, 1e19]) assert.throws(() => placement.place(scene.random, { type: 121, x, y: 90, region: 0, mode: 0, force: 1 }), /CRT 좌표 범위/, String(x));
    }
});

// 계약 2(군집): 지역 시드의 signed 비교와 청크 순회 순서, limit < 1 의 난수 비소비, 실패 한도 재설정, 현재 edgeFarm TYPE 재조회, float 저장. 원본 104행은 균일한 청크 지도와 단일 성공 패턴만 써서 구별하지 못한다. (2026-11-11 WB04-E285)
test('FarmCluster 계약 2: 지역 시드 signed·청크 순회 순서·limit<1·실패 한도 재설정·현재 TYPE·float 저장', () => {
    const scene = new FarmScene();
    // 청크 지도 stand-in 이다: 기본은 아무 지역도 아닌 island 0(요청 지역이 −1 이면 255 가 일치).
    const makeChunks = (fill = null) => {
        const entries = Array.from({ length: 256 }, () => ({ island: 0, record: new Uint8Array(6) }));
        if (fill) fill(entries);
        return { xLength: 16, yLength: 16, at: (x, y) => entries[y * 16 + x] };
    };
    const run = (chunks, area, region, quantity, place) => new RawFarmCluster(scene.pool, chunks, scene.islands, scene.placementState, scene.state, { place }).place(area, region, quantity);
    const RECT = { left: 80.25, top: 80.75, right: 96.625, bottom: 96.125 };
    // 첫 place 호출 시점의 local 난수 상태를 기록하고 항상 실패하는 훅이다.
    const capture = () => {
        const seen = { first: null, requests: [] };
        seen.place = (random, request) => {
            if (seen.first === null) seen.first = random.state;
            seen.requests.push(request);
            return 0;
        };
        return seen;
    };
    // 지역 −1(= island BYTE 255)과 청크 순회 순서: y 바깥/x 안쪽이므로 (x=1,y=0)이 (x=0,y=1)보다 먼저 일치한다. 시드 = record BYTE+3 × 77.
    let seen = capture();
    run(makeChunks((entries) => {
        entries[1].island = 255;
        entries[1].record = Uint8Array.of(0, 0, 0, 2, 0, 0);
        entries[16].island = 255;
        entries[16].record = Uint8Array.of(0, 0, 0, 3, 0, 0);
    }), RECT, -1, 0, seen.place);
    let reference = new GameRandom(2 * 77);
    reference.next(15);
    reference.next(16);
    assert.equal(seen.first, reference.state);
    // 시드가 없는 지역은 시드 0 이다.
    seen = capture();
    run(makeChunks(), RECT, 5, 0, seen.place);
    reference = new GameRandom(0);
    reference.next(15);
    reference.next(16);
    assert.equal(seen.first, reference.state);
    // 높이가 0 이면(limit < 1) 높이 난수를 소비하지 않고 폭만 소비한다.
    seen = capture();
    run(makeChunks((entries) => { entries[0].record = Uint8Array.of(0, 0, 0, 5, 0, 0); }), { left: 80.25, top: 80.75, right: 96.625, bottom: 80.75 }, 0, 0, seen.place);
    reference = new GameRandom(5 * 77);
    reference.next(16);
    assert.equal(seen.first, reference.state);
    // 실패 한도 재설정: 중심 10번 실패 뒤 한 번 성공하면 한도가 다시 30000 이 된다 → 총 시도 = 10 + 1 + 주변 9 + 29999.
    let calls = 0;
    run(makeChunks(), RECT, 0, 1, () => {
        calls += 1;
        return calls === 11 ? 1 : 0;
    });
    assert.equal(calls, 10 + 1 + 9 + 29999);
    // 현재 edgeFarm TYPE 은 요청마다 다시 읽는다(첫 요청 130, 이후 131).
    let reads = 0;
    scene.placementState.sharedEdgeFarmType = () => {
        reads += 1;
        return reads === 1 ? 130 : 131;
    };
    seen = capture();
    run(makeChunks(), RECT, 0, 1, seen.place);
    assert.deepEqual(seen.requests.slice(0, 3).map((request) => request.type), [130, 131, 131]);
    scene.placementState.sharedEdgeFarmType = null;
    // 좌표는 float32 로 저장한다: 0.1f 기준 영역의 중심/주변 좌표가 모두 float32 로 정확하다.
    seen = capture();
    run(makeChunks(), { left: Math.fround(0.1), top: Math.fround(0.1), right: Math.fround(0.1) + 16, bottom: Math.fround(0.1) + 16 }, 0, 1, seen.place);
    assert.ok(seen.requests.length > 100);
    assert.ok(seen.requests.every((request) => Math.fround(request.x) === request.x && Math.fround(request.y) === request.y));
});
