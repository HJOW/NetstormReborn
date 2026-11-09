// 섬 population raw-island-population.js(RawIslandPopulation·fortTerritoryIslandRecord)를 원본 10.78 관측 118입력과 cpppj `IslandPopulationTests` 계약으로 대조한다.
//  - 원본 관측 island-population-x86.tsv 118행: 모드(Residence·Populate·Replace)·사각형 프로필·local seed·Fort 모드·권한·진단 중 권한 변경(flip)·primary/fallback TYPE·HP 유무·성공 시도·외부 효과의 지도 변경(mutate)에서
//    호출 순서(P·D·T·A·position·destroy·create·type·hasHp·maxHp·owner·setOwner·pop·refresh)와 새 객체의 프레임 DWORD·HP·전체 청크 지도(헤더 3 DWORD + 256청크 12 BYTE)·배치 호출 수를 대조한다.
//    기존 `RawFortFinishOwner.placeRandom`(E221)/로컬 난수는 실제로 실행하고 개별 World 배치 경계만 관측한다.
//  - 웹 지도는 항목 배열을 y 행 우선으로 두므로(원본 청크 항목은 x 열 우선 12 BYTE) 원본 물리 index i 는 (x = i/16, y = i%16) 로 옮겨 쓰고 읽는다.
//  - 웹 범위 밖: 같은 fixture 의 두 번째 cpppj 시험(IslandAdd_ActualPreviewCollisionAndPlaceBeforeMissingGeneration)은 `RawIslandAdd`(미이식) 이식 때 대조한다. CD 10.72 는 범위 밖이다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-10 WB04-E223: 섬 population 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { SquidPostPopList } from '../www/core/squid-postpop-state.js';
import { ChunkMap } from '../www/assets/territory.js';
import { RawIslandRuntime, createFortFinishOwnerState } from '../www/core/raw-island-runtime.js';
import { RawFortFinishOwner } from '../www/core/raw-fort-finish-owner.js';
import { ISLAND_POPULATION_ASSERTION, RawIslandPopulation, fortTerritoryIslandRecord } from '../www/core/raw-island-population.js';

// 원본 관측 118행(머리 줄 제외)이다. 없으면 해당 검사를 건너뛴다. (2026-11-10 WB04-E223)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'island-population-x86.tsv'))?.filter((row) => row[0] !== 'entry') ?? null;

// 예외가 나는지 본다. (2026-11-10 WB04-E223)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 바이트 배열을 소문자 hex 로 만든다. (2026-11-10 WB04-E223)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 명시 입력 사각형 다섯 프로필이며 기대 결과를 계산하지 않는다. (2026-11-10 WB04-E223)
const RECTANGLES = [[16, 32, 64, 96], [15.25, 21.5, 16.1, 23.9], [-15.25, -3.5, 4.5, 9.5], [20, 21, 20, 21], [20, 21, 19, 19]]
    .map(([left, top, right, bottom]) => ({ left: Math.fround(left), top: Math.fround(top), right: Math.fround(right), bottom: Math.fround(bottom) }));

// 원본 청크 지도의 물리 index(i)를 웹 지도의 (x, y)로 바꾼다: 원본은 x 열 우선(i = x*16 + y)이다. (2026-11-10 WB04-E223)
const entryAt = (map, index) => map.at(Math.floor(index / 16), index % 16);

// 실제 256행/헤더(모드·가로·세로 DWORD 3개)를 원문 배치로 직렬화한다. (2026-11-10 WB04-E223)
function populationMap(map) {
    const bytes = new Uint8Array(3084);
    put(bytes, 0, 1);
    put(bytes, 4, map.xLength);
    put(bytes, 8, map.yLength);
    for (let index = 0; index < 256; index += 1) {
        const entry = entryAt(map, index);
        const at = 12 + index * 12;
        bytes[at] = entry.island;
        bytes[at + 1] = typeof entry.side === 'string' ? entry.side.charCodeAt(0) : entry.side & 0xff;
        bytes.set(entry.record, at + 2);
        put(bytes, at + 8, entry.ordinal >>> 0);
    }
    return bytes;
}

// fixture 입력과 실제 하위 장치의 관측만 준비하는 장면이다(cpppj PopulationScene). 입력 전역은 참조이며 부모 호출 동안 같은 주소를 유지한다. (2026-11-10 WB04-E223)
class PopulationScene {
    constructor(row) {
        const number = (at) => Number(row[at]) >>> 0;
        const at = number(9);
        const flip = number(5);
        const hp = number(8);
        const mutate = number(10);
        this.seed = number(2);
        this.fort = number(3);
        this.boss = number(4) !== 0;
        this.fortMode = this.fort !== 0;
        this.primary = number(6);
        this.fallback = number(7);
        this.residence = 89;
        this.replacement = 158;
        this.calls = 0;
        this.events = [];
        // 원문 서버/예측 free-list 영역을 모두 확보하는 유효 용량과 같은 현재 지도이다.
        this.pool = new SidPool(24000);
        this.hash = new SquidHash();
        this.map = new ChunkMap();
        this.types = Array.from({ length: 188 }, () => ({ flags1: 0, flags2: 0, footX: 0, footY: 0 }));
        this.residences = new SquidPostPopList();
        this.modes = createFortFinishOwnerState();
        // 실제 서버 첫 15000 을 거쳐 Native 장치가 선택한 15001 을 확보한다. 다음 생성은 15002 이다.
        assert.equal(this.pool.allocate(), 15000);
        assert.equal(this.pool.allocate(), 15001);
        for (let index = 0; index < 256; index += 1) {
            const entry = entryAt(this.map, index);
            entry.island = 127;
            entry.side = 46;
            entry.record.fill(0xa5);
            entry.ordinal = 0x12345678;
        }
        // Native 도구가 준비한 물리 offset 을 그대로 쓴다(실제 At 은 x*16+y 이므로 이 입력을 임의 전치하지 않는다).
        for (const [px, py] of [[1, 2], [2, 2], [1, 3], [2, 3]]) {
            const entry = entryAt(this.map, py * 16 + px);
            entry.island = 3;
            entry.record.set([3, 1, 0x21, this.seed & 0xff, 0xab, 0xcd]);
        }
        const islandBytes = new Uint8Array(3204);
        put(islandBytes, 0, 4);
        put(islandBytes, 4 + 3 * 32 + 4, 1);
        put(islandBytes, 4 + 3 * 32 + 8, 3);
        this.islands = new RawIslandRuntime(this.pool, this.hash, islandBytes, this.modes, this.residences, { value: 0 }, { value: 0 }, {
            setResidenceOwner: () => { throw new Error('미선택 Runtime 거주구역 owner'); },
            assertion: () => { throw new Error('미선택 Runtime 진단'); },
        });
        this.placement = new RawFortFinishOwner(this.pool, this.types, this.residences, this.modes, {
            regionBounds: () => { throw new Error('직접 40ded0 의 미선택 bounds'); },
            wallSeconds: () => { throw new Error('직접 40ded0 의 미선택 시각'); },
            invalidType: () => this.event('D', [29]),
            // 기존 40ded0/PRNG 는 실제로 실행하고 개별 World 배치 경계만 관측한다.
            place: (local, request) => {
                this.calls += 1;
                this.event('P', [request.type, floatToBits(request.x), floatToBits(request.y), request.region >>> 0, request.mode, request.force, local.state]);
                return this.calls >= at ? 15001 : 0;
            },
        });
        const lines = [0xc2, 0x139, 0xc3, 0xd1, 0x118];
        const kinds = Object.values(ISLAND_POPULATION_ASSERTION);
        const wall = new DataView(new ArrayBuffer(8));
        wall.setFloat64(0, 2.5, true);
        this.hooks = {
            assertion: (kind) => {
                const line = lines[kinds.indexOf(kind)];
                this.event('A', [line]);
                if (flip && (line === 0xc3 || line === 0xd1)) this.boss = true;
            },
            wallSeconds: () => { this.event('T', [wall.getUint32(0, true), wall.getUint32(4, true)]); return 2.5; },
            position: (sid) => { this.event('position', [sid]); return [55.25, 66.5]; },
            destroy: (sid, flags) => {
                this.event('destroy', [sid, flags]);
                // Native 의 +12+(2*16+1)*12+2 와 같은 물리 BYTE 만 바꾼다.
                if (mutate) entryAt(this.map, 2 * 16 + 1).record[0] = 0xff;
            },
            create: (type, flags) => {
                this.event('create', [type, flags]);
                const sid = this.pool.allocate();
                assert.equal(sid, 15002);
                const raw = this.pool.allocated(sid);
                raw.fill(0xa5);
                put(raw, 0, 0x10012000);
                raw[10] = type;
                raw[11] = 4;
                return sid;
            },
            defaultFrame: (type) => { this.event('type', [type]); return -7; },
            hasHitPoints: () => { this.event('hasHp', []); return hp !== 0; },
            typeHitPoints: (type) => { this.event('type', [type]); this.event('maxHp', []); return 87; },
            setOwner: (_sid, owner) => { this.event('owner', [3]); this.event('setOwner', [owner]); },
            pop: (_sid, x, y, flags) => this.event('pop', [floatToBits(x), floatToBits(y), flags]),
            refreshProduction: () => this.event('refresh', []),
        };
        this.population = new RawIslandPopulation(this.pool, this.map, this.map, this.islands, this.placement, {
            boss: () => this.boss, fort: () => this.fortMode, residence: () => this.residence, primary: () => this.primary, fallback: () => this.fallback, replacement: () => this.replacement,
        }, this.hooks);
    }

    // Native 와 같은 DWORD 비트 인자를 이름과 함께 기록한다. (2026-11-10 WB04-E223)
    event(name, values) { this.events.push([name, ...values].join(':')); }

    // 원문 호출 순서에 정렬이나 중복 제거를 하지 않는다. (2026-11-10 WB04-E223)
    eventText() { return this.events.length === 0 ? '-' : this.events.join(';'); }
}

test('섬 population: 원본 10.78 118입력의 현재 지도·난수·가상 순서·직접 frame/HP 쓰기가 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    assert.equal(rows.length, 118);
    let count = 0;
    // 모든 Native 행을 처음부터 실제 복원 몸체에 재생한다.
    for (const row of rows) {
        assert.equal(row.length, 18);
        const scene = new PopulationScene(row);
        if (row[0] === 'Residence') scene.population.residence(3);
        else if (row[0] === 'Populate') scene.population.populate(3);
        else scene.population.replaceInitial(RECTANGLES[Number(row[1])], 3);
        const label = `${row[0]} profile${row[1]} (#${count})`;
        assert.equal(scene.eventText(), row[12], `${label} 호출 순서`);
        assert.equal(scene.calls, Number(row[16]), `${label} 배치 호출 수`);
        assert.equal(hex(populationMap(scene.map)), row[15], `${label} 지도`);
        // 거주 구역 전용/자연 권한 반환은 생성되지 않은 새 객체를 읽지 않는다.
        if (row[12].includes('create:')) {
            const raw = scene.pool.slot(15002);
            assert.equal(get(raw, 36), Number(row[13]), `${label} frame`);
            assert.equal(get(raw, 26), Number(row[14]), `${label} hp`);
        }
        count += 1;
    }
    assert.equal(count, 118);
});

test('섬 population 계약: 지도의 첫 record 조회는 y 바깥/x 안쪽 순서이고 signed 섬 번호로 비교하며 없으면 0 이다', () => {
    const map = new ChunkMap();
    assert.deepEqual(Array.from(fortTerritoryIslandRecord(map, 3)), [0, 0, 0, 0, 0, 0]);
    // (x=5,y=1) 과 (x=2,y=2) 가 섬 3 이면 y 행이 낮은 (5,1) 이 먼저이다.
    map.at(5, 1).island = 3;
    map.at(5, 1).record.set([1, 2, 3, 4, 5, 6]);
    map.at(2, 2).island = 3;
    map.at(2, 2).record.set([9, 9, 9, 9, 9, 9]);
    assert.deepEqual(Array.from(fortTerritoryIslandRecord(map, 3)), [1, 2, 3, 4, 5, 6]);
    // 반환은 사본이라 지도를 바꾸지 않는다.
    fortTerritoryIslandRecord(map, 3)[0] = 77;
    assert.equal(map.at(5, 1).record[0], 1);
    // 섬 번호 BYTE 0xff 는 signed -1 이다.
    map.at(0, 0).island = 255;
    map.at(0, 0).record.set([7, 7, 7, 7, 7, 7]);
    assert.deepEqual(Array.from(fortTerritoryIslandRecord(map, -1)), [7, 7, 7, 7, 7, 7]);
});

test('섬 population 계약: 다른 풀의 섬 목록/배치와 선택된 효과 누락을 그 위치에서 거부한다', { skip: !rows && '원본 fixture 없음' }, () => {
    const row = rows[0];
    const scene = new PopulationScene(row);
    const other = new PopulationScene(row);
    const globals = { boss: () => true, fort: () => false, residence: () => 89, primary: () => 71, fallback: () => 133, replacement: () => 158 };
    assert.ok(throws(() => new RawIslandPopulation(scene.pool, scene.map, scene.map, other.islands, scene.placement, globals, scene.hooks)));
    assert.ok(throws(() => new RawIslandPopulation(scene.pool, scene.map, scene.map, scene.islands, other.placement, globals, scene.hooks)));
    // 진단 장치가 없으면 존재하지 않는 섬의 bounds 읽기에서 거부한다.
    const missing = new RawIslandPopulation(scene.pool, scene.map, scene.map, scene.islands, scene.placement, globals, {});
    assert.ok(throws(() => missing.residence(50)));
    // 생성 효과가 없으면 앞선 위치 snapshot 과 삭제까지만 진행한다(원본 순서 보존).
    const partial = new RawIslandPopulation(scene.pool, scene.map, scene.map, scene.islands, scene.placement, { ...globals, boss: () => true }, {
        ...scene.hooks, create: undefined,
    });
    assert.ok(throws(() => partial.replaceInitial(RECTANGLES[0], 3)));
    assert.ok(scene.events.some((item) => item.startsWith('P:')));
    assert.ok(scene.events.some((item) => item.startsWith('destroy:')));
});
