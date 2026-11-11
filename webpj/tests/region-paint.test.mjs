// 지역 투표 뒤의 실제 지면 flood 와 패치판 경계 프레임 도장(cpppj `o/RawRegionPaint`, 00470900/0046fe40/004703b0)의 웹 대응 `core/raw-region-paint.js` 를 검사한다(4-B, WB04-E296).
//  - 고정 관측 `regionpaint-x86.tsv` originals 386행(scene 6 × 변형 3 × 소유자/원소/시작 좌표): 같은 raw 슬롯·섬 지도·지형 지역 getter(`RawCanonPlacementTerrain.regionAt`)·게임 난수에서
//    소유자 가상 호출/edgeFarm finder 호출 순서·슬롯의 소유자와 프레임 DWORD·경계 후보 삽입 순서·영향 지역 기록의 float 비트·drawer 네 float/요청·월드 다시 그리기 DWORD·게임 난수 상태가 같다.
//    CD 98행은 웹 목표(10.78) 밖이다. 모든 기대 값은 원본 PE 관측에서 읽고 이 시험은 입력 슬롯/지도만 만든다.
//  - 계약: 공유 월드 다시 그리기 DWORD(진입 시점 값을 캐시하지 않고 호출 시점의 현재 값에 더함, 시작 칸이 비어도 증가, 감김), 두 독립 변형 캐시, 정수 좌표 비트 재해석(음수 좌표의 xBits),
//    투표 wrapper 의 풀 일치 확인과 요청 종류/연결 거부.
// 실제 게임·브라우저 실행이 아니다. (2026-11-11 WB04-E296)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get, floatToBits } from './raw-support.mjs';
import { FrameCodeTable, paintCoreFrame } from '../www/assets/terrain-tiles.js';
import { SidPool } from '../www/core/sid-pool.js';
import { GameRandom } from '../www/core/random.js';
import { RawCanonPlacementTerrain, createCanonPlacementTerrainState } from '../www/core/raw-canon-placement-terrain.js';
import { RegionOwnershipState } from '../www/core/raw-region-ownership.js';
import { RawRegionPaint, RegionPaintState, makeRegionPaintOwnershipHooks } from '../www/core/raw-region-paint.js';

// 원본 관측 행(originals 만)이다. 파일이 없으면 null 이라 해당 검사를 건너뛴다. (2026-11-11 WB04-E296)
const ROWS = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'regionpaint-x86.tsv'))?.filter((row) => row[0] === 'originals') ?? null;

// 원본 실행기에 제공한 실제 타입 번호와 좌표 입력이다. (2026-11-11 WB04-E296)
const TERRAIN = 83;
const EDGE = 84;
const OTHER = 85;
const POINTS = [{ x: 10, y: 10 }, { x: 11, y: 10 }, { x: 10, y: 11 }, { x: 11, y: 11 }, { x: 12, y: 10 }, { x: 12, y: 11 }];

// 지도 포함 항목만 지정하며 기대 결과는 독립 fixture 에서 읽는다. (2026-11-11 WB04-E296)
const SCENES = [[], [0], [0, 1, 4], [0, 1, 2, 3], [0, 1, 2, 3, 4, 5], [0, 1, 2, 3, 4, 5]];

// 테스트 입력 프레임은 AA 와 144개 JA 구간 및 비연결 PP 다. (2026-11-11 WB04-E296)
function codes() {
    const result = [{ side: 0x41, variant: 0x41, number: 0, flags: 0 }];
    // 실제 J 시작 번호 1 과 원소별 36프레임 구간을 입력으로 제공한다.
    for (let index = 0; index < 144; index += 1) result.push({ side: 0x4a, variant: 0x41, number: index, flags: 0 });
    // 비연결 PP 도 유효한 네 quartile 의 12개 후보를 공급한다.
    for (let index = 0; index < 12; index += 1) result.push({ side: 0x50, variant: 0x50, number: index, flags: 0 });
    return result;
}

// 지형 지역 getter 가 읽는 188칸 타입 표(지면 타입만 섬 genus)와 동일 프레임 표 188개이다. (2026-11-11 WB04-E296)
function tables() {
    const types = Array.from({ length: 188 }, () => ({ flags2: 0 }));
    types[TERRAIN].flags2 = 2;
    const table = codes();
    return { types, frames: Array.from({ length: 188 }, () => new FrameCodeTable(table)) };
}

// float 좌표(정수 + 소수)를 raw 슬롯에 쓴다. (2026-11-11 WB04-E296)
function putPoint(raw, x, y) {
    put(raw, 14, floatToBits(x));
    put(raw, 18, floatToBits(y));
}

// 한 행의 입력으로 장면을 만들고 도장을 한 번 실행해 관측 문자열을 돌려준다. (2026-11-11 WB04-E296)
function replay(row) {
    const scene = Number(row[1]);
    const variant = Number(row[2]);
    const pool = new SidPool(32768);
    const { types, frames } = tables();
    const islands = new Uint16Array(65536);
    const initialFrames = [0, 145, 3];
    // 출력에서 읽을 미사용 슬롯도 원본 합성 pool 처럼 0 으로 초기화한다(free 슬롯이라 물리 접근).
    for (const number of [50, 51, 52, 53, 54, 55, 70, 71]) pool.physical(number).fill(0);
    // 선행 소유자·장벽·잘못된 시작 타입을 입력으로만 작성한다.
    for (const index of SCENES[scene]) {
        const point = POINTS[index];
        const sid = 50 + index;
        const raw = pool.physical(sid);
        raw[10] = scene === 5 && index === 0 ? OTHER : TERRAIN;
        raw[34] = (index % 3 + variant) % 3;
        put(raw, 8, point.x < 12 ? 7 : 9, 2);
        putPoint(raw, point.x + 0.125, point.y + 0.25);
        put(raw, 36, initialFrames[variant], 4);
        islands[point.y * 256 + point.x] = sid;
    }
    const fallback = new Map([['9,10', 70], ['12,12', 71]]);
    // edgeFarm 은 표면 지도 밖 일반 finder 후보로만 공급한다.
    for (const [key, sid] of fallback) {
        const [x, y] = key.split(',').map(Number);
        const raw = pool.physical(sid);
        raw[10] = EDGE;
        raw[34] = 2;
        putPoint(raw, x + 0.125, y + 0.25);
    }
    const reject = () => { throw new Error('도장 재생에 배치 호출'); };
    const terrain = new RawCanonPlacementTerrain(pool, types, Array.from({ length: 188 }, () => ({})), islands, createCanonPlacementTerrainState(),
        { candidatePermission: reject, shapePermission: reject, finishRelations: reject });
    const random = new GameRandom(0x23456789);
    const state = new RegionPaintState();
    state.owner = 6;
    state.theme = 3;
    state.revision = 0xffffffff;
    const ownership = new RegionOwnershipState({ affected: [{ x: 1.0, y: 2.0, region: 7 }] });
    const events = [];
    const hooks = {
        setOwner: (sid, owner) => { events.push(`O:${sid}:${owner}`); pool.allocated(sid)[34] = owner & 0xff; },
        boundaryAt: (x, y, type) => {
            assert.equal(type, EDGE);
            const sid = fallback.get(`${Math.trunc(x)},${Math.trunc(y)}`) ?? 0;
            events.push(`F:${Math.trunc(x)}:${Math.trunc(y)}:${sid}`);
            return sid;
        },
        regionAt: (x, y) => terrain.regionAt(Math.trunc(x), Math.trunc(y)),
    };
    const paint = new RawRegionPaint(pool, frames, islands, TERRAIN, EDGE, random, state, ownership, hooks);
    paint.paint({ x: Number.parseInt(row[5], 10), y: Number.parseInt(row[6], 10), owner: Number(row[3]), theme: Number(row[4]) });
    const objects = [50, 51, 52, 53, 54, 55, 70, 71].map((number) => {
        const raw = pool.slot(number);
        return `${number}:${raw[34]}:${get(raw, 36, 4)}`;
    }).join(',');
    const boundary = state.boundary.map((point) => `${point.x}:${point.y}`).join(',');
    // 도장이 만든 기록은 float 값과 원본 비트가 일치한다(NaN 비트가 아닌 경우).
    for (const value of ownership.affected) if (value.xBits !== undefined && !Number.isNaN(value.x)) assert.equal(floatToBits(value.x), value.xBits);
    const affected = ownership.affected.map((value) => `${value.xBits ?? floatToBits(value.x)}:${value.yBits ?? floatToBits(value.y)}:${value.region}`).join(',');
    const drawer = `${state.bounds.map(floatToBits).join(':')}:${state.owner}:${state.theme >>> 0}`;
    return { events: events.join(';'), objects, boundary, affected, drawer, revision: state.revision, rng: random.state };
}

// 패치의 실제 frame/경계 확장/고유 지역 기록과 두 난수 경로를 대조한다(386행). (2026-11-11 WB04-E296)
test('지역 도장은 관측한 flood·경계 프레임·영향 지역·난수를 그대로 재생한다', { skip: ROWS === null }, () => {
    let count = 0;
    // 각 행은 서로 다른 새 drawer/cache/map 으로 실행한다.
    for (const row of ROWS) {
        assert.equal(row.length, 14);
        const result = replay(row);
        const label = `행 ${count} scene${row[1]}/variant${row[2]} owner${row[3]} theme${row[4]} (${row[5]},${row[6]})`;
        assert.equal(result.events || '-', row[7], `${label} 효과`);
        assert.equal(result.objects, row[8], `${label} 슬롯`);
        assert.equal(result.boundary || '-', row[9], `${label} 후보`);
        assert.equal(result.affected || '-', row[10], `${label} 영향 지역`);
        assert.equal(result.drawer, row[11], `${label} drawer`);
        assert.equal(result.revision, Number(row[12]), `${label} revision`);
        assert.equal(result.rng, Number(row[13]), `${label} 난수`);
        count += 1;
    }
    assert.equal(count, 386);
});

// ───────────── 계약 ─────────────

// 단일 표면 슬롯과 공유 월드 다시 그리기 상자로 장면을 만든다. (2026-11-11 WB04-E296)
function singleSurface({ mutate = () => {} } = {}) {
    const pool = new SidPool(32768);
    const surface = pool.allocate(2);
    const raw = pool.allocated(surface);
    raw.fill(0);
    raw[10] = TERRAIN;
    raw[34] = 2;
    putPoint(raw, 10.125, 10.25);
    const { frames } = tables();
    const islands = new Uint16Array(65536);
    islands[10 * 256 + 10] = surface;
    const box = { value: 17 };
    const state = new RegionPaintState();
    state.revision = 0x76543210;
    state.sharedWorldRedraw = box;
    const ownership = new RegionOwnershipState();
    const log = { assignments: 0 };
    const hooks = {
        setOwner: (sid, owner) => { log.assignments += 1; pool.allocated(sid)[34] = owner & 0xff; mutate(box); },
        boundaryAt: () => 0,
        regionAt: () => 7,
    };
    return { pool, surface, box, state, log, paint: new RawRegionPaint(pool, frames, islands, TERRAIN, EDGE, new GameRandom(0x23456789), state, ownership, hooks) };
}

// 실제 지면 flood 효과 뒤 월드 DWORD 만 증가시키며 진입 값을 캐시하지 않는다. 빈 시작 칸도 원문 증가를 수행하고 감김을 보존한다. (2026-11-11 WB04-E296)
test('지역 도장은 공유 월드 다시 그리기 DWORD 의 현재 값에 호출마다 하나를 더한다', () => {
    let mutating = false;
    const scene = singleSurface({ mutate: (box) => { if (mutating) box.value = 37; } });
    assert.equal(scene.state.worldRedraw, 17);
    scene.paint.paint({ x: 10, y: 10, owner: 3, theme: 0 });
    assert.equal(scene.log.assignments, 1);
    assert.equal(scene.pool.slot(scene.surface)[34], 3);
    assert.equal(scene.box.value, 18);
    assert.equal(scene.state.revision, 0x76543210);
    // setOwner 뒤 현재 37 을 읽으며 Paint 진입의 18 을 캐시하지 않는다.
    mutating = true;
    scene.paint.paint({ x: 10, y: 10, owner: 4, theme: 0 });
    assert.equal(scene.log.assignments, 2);
    assert.equal(scene.pool.slot(scene.surface)[34], 4);
    assert.equal(scene.box.value, 38);
    assert.equal(scene.state.revision, 0x76543210);
    // 빈 시작 칸도 증가하고 unsigned 감김을 보존한다.
    scene.box.value = 0xffffffff;
    scene.paint.paint({ x: 0, y: 0, owner: 1, theme: 0 });
    assert.equal(scene.log.assignments, 2);
    assert.equal(scene.box.value, 0);
    assert.equal(scene.state.revision, 0x76543210);
    // 독립 입력은 revision 을 쓴다.
    const own = new RegionPaintState();
    own.revision = 0xffffffff;
    own.incrementWorldRedraw();
    assert.equal(own.revision, 0);
    assert.equal(own.worldRedraw, 0);
});

// 본섬 안쪽 프레임과 경계 프레임은 서로 독립인 99 캐시를 첫 사용에 만들고 재사용한다. 시작 칸이 지면이 아니거나 월드 밖이면 소유자/원소를 바꾸지 않는다. (2026-11-11 WB04-E296)
test('지역 도장의 두 변형 캐시는 독립이고 재사용되며 무효 시작 칸은 요청을 기록하지 않는다', () => {
    const scene = singleSurface();
    assert.equal(scene.state.coreVariants, null);
    assert.equal(scene.state.boundaryVariants, null);
    scene.state.owner = 9;
    scene.state.theme = 5;
    scene.paint.paint({ x: 20, y: 20, owner: 4, theme: 1 });
    assert.equal(scene.state.owner, 9);
    assert.equal(scene.state.theme, 5);
    assert.deepEqual(scene.state.bounds, [257, 257, -1, -1]);
    assert.equal(scene.state.coreVariants, null);
    scene.paint.paint({ x: -5, y: 400, owner: 4, theme: 1 });
    assert.equal(scene.state.owner, 9);
    scene.paint.paint({ x: 10, y: 10, owner: 4, theme: 1 });
    assert.equal(scene.state.owner, 4);
    assert.equal(scene.state.theme, 1);
    assert.equal(scene.state.coreVariants.length, 99);
    const cache = scene.state.coreVariants;
    // 경계 도장은 별도 캐시를 만든다(같은 요청에서 AA 경계가 있으면): 직접 만든 경계 캐시도 재사용한다.
    scene.state.boundaryVariants = Array.from({ length: 99 }, (_, index) => index);
    scene.paint.paint({ x: 10, y: 10, owner: 5, theme: 2 });
    assert.equal(scene.state.coreVariants, cache);
    assert.equal(scene.state.boundaryVariants.length, 99);
    assert.equal(scene.state.theme, 2);
    // theme 는 int32 로 보관한다(요청 DWORD 의 상위 비트).
    scene.paint.paint({ x: 10, y: 10, owner: 6, theme: 0xffffffff });
    assert.equal(scene.state.theme, -1);
});

// 영향 지역 기록은 정수 좌표의 비트를 float 로 재해석한다(음수는 NaN 비트라 원본 비트를 따로 보관). (2026-11-11 WB04-E296)
test('지역 도장의 영향 지역 기록은 정수 좌표 비트를 그대로 보관한다', () => {
    const pool = new SidPool(32768);
    const surface = pool.allocate(2);
    const edge = pool.allocate(2);
    const surfaceRaw = pool.allocated(surface);
    surfaceRaw.fill(0);
    surfaceRaw[10] = TERRAIN;
    surfaceRaw[34] = 1;
    putPoint(surfaceRaw, 0.125, 5.25);
    const edgeRaw = pool.allocated(edge);
    edgeRaw.fill(0);
    edgeRaw[10] = EDGE;
    edgeRaw[34] = 1;
    putPoint(edgeRaw, -0.875, 5.25);
    const { frames } = tables();
    const islands = new Uint16Array(65536);
    islands[5 * 256 + 0] = surface;
    const ownership = new RegionOwnershipState();
    const regions = [];
    const hooks = {
        setOwner: (sid, owner) => { pool.allocated(sid)[34] = owner & 0xff; },
        boundaryAt: (x, y) => (x === -1 && y === 5 ? edge : 0),
        regionAt: (x, y) => { regions.push([x, y]); return x === -1 ? 3 : 7; },
    };
    new RawRegionPaint(pool, frames, islands, TERRAIN, EDGE, new GameRandom(1), new RegionPaintState(), ownership, hooks).paint({ x: 0, y: 5, owner: 4, theme: 0 });
    // 시작 표면의 서쪽(-1, 5)이 빈 표면이라 후보에 들어가 edgeFarm finder 를 거친 영향 지역 기록이 남는다.
    const record = ownership.affected.find((value) => value.region === 3);
    assert.ok(record);
    assert.equal(record.xBits, 0xffffffff);
    assert.equal(record.yBits, 5);
    assert.ok(regions.some(([x, y]) => x === -1 && y === 5));
    // 같은 지역 번호는 한 번만 기록한다.
    assert.equal(ownership.affected.filter((value) => value.region === 3).length, 1);
});

// 구성과 요청의 종류를 확인하고 투표 wrapper 가 같은 풀의 도장만 연결한다. (2026-11-11 WB04-E296)
test('지역 도장은 잘못된 연결/요청을 거부하고 투표 wrapper 는 같은 풀만 받는다', () => {
    const scene = singleSurface();
    const { frames } = tables();
    const pool = scene.pool;
    const islands = new Uint16Array(65536);
    const ownership = new RegionOwnershipState();
    const hooks = { setOwner() {}, boundaryAt: () => 0, regionAt: () => 7 };
    const make = (changes = {}) => new RawRegionPaint(changes.pool ?? pool, changes.frames ?? frames, changes.islands ?? islands, changes.terrain ?? TERRAIN, changes.edge ?? EDGE,
        changes.random ?? new GameRandom(1), changes.state ?? new RegionPaintState(), changes.ownership ?? ownership, changes.hooks ?? hooks);
    make();
    assert.throws(() => make({ pool: {} }), TypeError);
    assert.throws(() => make({ frames: frames.slice(1) }), TypeError);
    assert.throws(() => make({ islands: new Uint16Array(10) }), TypeError);
    assert.throws(() => make({ islands: new Uint8Array(65536) }), TypeError);
    assert.throws(() => make({ terrain: 188 }), TypeError);
    assert.throws(() => make({ terrain: -1 }), TypeError);
    assert.throws(() => make({ edge: 188 }), TypeError);
    assert.throws(() => make({ random: {} }), TypeError);
    assert.throws(() => make({ state: {} }), TypeError);
    assert.throws(() => make({ ownership: {} }), TypeError);
    for (const name of ['setOwner', 'boundaryAt', 'regionAt']) assert.throws(() => make({ hooks: { ...hooks, [name]: undefined } }), TypeError, name);
    const paint = make();
    assert.throws(() => paint.paint(null), TypeError);
    assert.throws(() => paint.paint({ x: 1.5, y: 1, owner: 1, theme: 0 }), TypeError);
    assert.throws(() => paint.paint({ x: 1, y: 1, owner: -1, theme: 0 }), TypeError);
    assert.throws(() => paint.paint({ x: 1, y: 1, owner: 0x1_0000_0000, theme: 0 }), TypeError);
    assert.throws(() => paint.paint({ x: 1, y: 1, owner: 1, theme: 1.5 }), TypeError);
    assert.equal(paint.pool, pool);
    // 투표 wrapper: 같은 풀이면 paint 훅을 채운 사본, 다른 풀/객체면 거부한다.
    const base = { regionAt: () => 1 };
    const wired = makeRegionPaintOwnershipHooks(pool, paint, base);
    assert.equal(typeof wired.paint, 'function');
    assert.equal(wired.regionAt, base.regionAt);
    assert.equal(base.paint, undefined);
    assert.throws(() => makeRegionPaintOwnershipHooks(new SidPool(32768), paint, base), TypeError);
    assert.throws(() => makeRegionPaintOwnershipHooks(pool, null, base), TypeError);
});

// ───────────── 변형 확인으로 보강한 계약 ─────────────

// 표면 하나(AA 코드, 프레임 0)와 고정 좌표 float 로 장면을 만든다. (2026-11-11 WB04-E296)
function surfaceScene({ floatX = 10, floatY = 10, frames = tables().frames, owner = 2 } = {}) {
    const pool = new SidPool(32768);
    const surface = pool.allocate(2);
    const raw = pool.allocated(surface);
    raw.fill(0);
    raw[10] = TERRAIN;
    raw[34] = owner;
    putPoint(raw, floatX, floatY);
    const islands = new Uint16Array(65536);
    islands[10 * 256 + 10] = surface;
    const state = new RegionPaintState();
    const events = [];
    const hooks = {
        setOwner: (sid, who) => { events.push(`O:${sid}:${who}`); pool.allocated(sid)[34] = who & 0xff; },
        boundaryAt: () => 0,
        regionAt: () => 7,
    };
    const paint = new RawRegionPaint(pool, frames, islands, TERRAIN, EDGE, new GameRandom(1), state, new RegionOwnershipState(), hooks);
    return { pool, surface, islands, state, events, paint, hooks, frames };
}

// 초기 상태, 요청 DWORD 보존(0xff 를 넘는 소유자), 무효 시작 칸이 후보 목록을 비운다. (2026-11-11 WB04-E296)
test('지역 도장의 초기 상태와 요청 값은 원본 폭 그대로 보관된다', () => {
    const fresh = new RegionPaintState();
    assert.deepEqual(fresh.bounds, [-10000, -10000, -10000, -10000]);
    assert.deepEqual([fresh.owner, fresh.theme, fresh.revision, fresh.boundary.length, fresh.coreVariants, fresh.boundaryVariants, fresh.sharedWorldRedraw], [0, 0, 0, 0, null, null, null]);
    const scene = surfaceScene();
    scene.paint.paint({ x: 10, y: 10, owner: 0x103, theme: 0 });
    // 전체 DWORD 소유자는 raw BYTE(3)와 다르므로 소유자 호출이 일어나고 그 값 그대로 전달된다.
    assert.equal(scene.state.owner, 0x103);
    assert.deepEqual(scene.events, ['O:5:259']);
    // 무효 시작 칸은 후보 목록을 비운다(이전 호출의 후보가 남지 않음).
    assert.ok(scene.state.boundary.length > 0);
    scene.paint.paint({ x: 0, y: 0, owner: 1, theme: 0 });
    assert.equal(scene.state.boundary.length, 0);
    // 요청 원소는 DWORD 범위(음수 int32 포함)만 받는다.
    assert.throws(() => scene.paint.paint({ x: 10, y: 10, owner: 1, theme: 0x1_0000_0000 }), TypeError);
    assert.throws(() => scene.paint.paint({ x: 10, y: 10, owner: 1, theme: -0x80000001 }), TypeError);
});

// 지도 범위는 정확히 0..255 이며 256 은 다음 행의 칸을 읽지 않고 월드 밖(SID 0)이다. (2026-11-11 WB04-E296)
test('지역 도장의 표면 조회는 지도 끝을 넘으면 월드 밖이다', () => {
    const scene = surfaceScene();
    const other = scene.pool.allocate(2);
    const raw = scene.pool.allocated(other);
    raw.fill(0);
    raw[10] = TERRAIN;
    raw[34] = 7;
    scene.islands[11 * 256 + 0] = other;
    scene.paint.paint({ x: 256, y: 10, owner: 4, theme: 0 });
    assert.deepEqual(scene.events, []);
    assert.equal(scene.pool.slot(other)[34], 7);
    // 마지막 열(255)은 정상 지도 칸이다.
    const edge = scene.pool.allocate(2);
    const edgeRaw = scene.pool.allocated(edge);
    edgeRaw.fill(0);
    edgeRaw[10] = TERRAIN;
    edgeRaw[34] = 7;
    scene.islands[10 * 256 + 255] = edge;
    scene.paint.paint({ x: 255, y: 10, owner: 4, theme: 0 });
    assert.equal(scene.events[0], `O:${edge}:4`);
});

// 현재 프레임 코드 조회는 DWORD 전체를 읽고(256 이상은 코드 표 밖) 코드 표 길이와 같은 번호도 거부한다. (2026-11-11 WB04-E296)
test('지역 도장은 표 밖 현재 프레임 번호를 거부한다', () => {
    const length = codes().length;
    for (const frame of [length, 256, 0x10000]) {
        const scene = surfaceScene();
        put(scene.pool.allocated(scene.surface), 36, frame, 4);
        assert.throws(() => scene.paint.paint({ x: 10, y: 10, owner: 4, theme: 0 }), /프레임 코드 범위/, `프레임 ${frame}`);
    }
    // 마지막 정상 번호는 통과한다.
    const ok = surfaceScene();
    put(ok.pool.allocated(ok.surface), 36, length - 1, 4);
    ok.paint.paint({ x: 10, y: 10, owner: 4, theme: 0 });
});

// 안쪽 프레임의 픽셀 residue 는 raw 좌표의 0 방향 절삭이다(반올림 아님). 유한하지 않거나 int 범위 밖 좌표는 거부한다. (2026-11-11 WB04-E296)
test('지역 도장의 안쪽 프레임은 raw 좌표를 절삭해 쓰고 범위 밖 좌표는 거부한다', () => {
    const frameOf = (floatX, floatY) => {
        const scene = surfaceScene({ floatX, floatY });
        scene.paint.paint({ x: 10, y: 10, owner: 4, theme: 0 });
        return get(scene.pool.slot(scene.surface), 36, 4);
    };
    // 10.125/10.25 와 10.75/10.875 는 모두 절삭하면 (10, 10)이라 같은 프레임이다.
    assert.equal(frameOf(10.75, 10.875), frameOf(10.125, 10.25));
    // 정수 좌표 (11, 10)과는 residue 가 달라 다른 프레임이다(x % 3 이 달라짐).
    assert.notEqual(frameOf(11.5, 10.5), frameOf(10.5, 10.5));
    // 비유한(NaN/Infinity)과 int32 범위 밖(양/음)은 거부한다.
    for (const bits of [0x7f800000, 0xff800000, 0x7fc00000, 0x4f800000, 0xcf800001]) {
        const scene = surfaceScene();
        put(scene.pool.allocated(scene.surface), 14, bits);
        assert.throws(() => scene.paint.paint({ x: 10, y: 10, owner: 4, theme: 0 }), /좌표 절삭/, `bits ${bits.toString(16)}`);
    }
});

// 16비트를 넘는 프레임 번호도 DWORD 로 쓴다(첫 J 구간이 뒤쪽에 있는 큰 표). 기대값은 타입 모델 paintCoreFrame 의 독립 계산이다. (2026-11-11 WB04-E296)
test('지역 도장은 프레임 번호를 DWORD 전체로 쓴다', () => {
    const big = [];
    for (let index = 0; index < 65600; index += 1) big.push({ side: 0x41, variant: 0x41, number: index & 0xff, flags: 0 });
    big.push({ side: 0x4a, variant: 0x41, number: 0, flags: 0 });
    for (let index = 0; index < 144; index += 1) big.push({ side: 0x4a, variant: 0x41, number: index, flags: 0 });
    const frames = tables().frames;
    frames[TERRAIN] = new FrameCodeTable(big);
    for (const theme of [0, 1, 2, 3]) {
        const scene = surfaceScene({ frames });
        scene.paint.paint({ x: 10, y: 10, owner: 4, theme });
        const frame = get(scene.pool.slot(scene.surface), 36, 4);
        assert.equal(frame, paintCoreFrame(frames[TERRAIN], scene.state.coreVariants, theme, 10, 10), `theme ${theme}`);
        assert.ok(frame > 0xffff);
    }
});

// 경계 후보: 빈 표면의 edgeFarm 를 거쳐 이웃 후보가 늘고, 이미 다른 소유자의 지면 후보도 소유자를 바꾸며, 경계 캐시는 안쪽 캐시와 별개로 만든다. (2026-11-11 WB04-E296)
function boundaryScene(frames = tables().frames) {
    const pool = new SidPool(32768);
    const make = (type, owner, frame, x, y) => {
        const sid = pool.allocate(2);
        const raw = pool.allocated(sid);
        raw.fill(0);
        raw[10] = type;
        raw[34] = owner;
        put(raw, 36, frame, 4);
        putPoint(raw, x, y);
        return sid;
    };
    const start = make(TERRAIN, 2, 0, 10, 10);
    const farther = make(TERRAIN, 9, 145, 11, 9);
    const edge = make(EDGE, 5, 0, 10, 9);
    const islands = new Uint16Array(65536);
    islands[10 * 256 + 10] = start;
    islands[9 * 256 + 11] = farther;
    const state = new RegionPaintState();
    const events = [];
    const ownership = new RegionOwnershipState();
    const hooks = {
        setOwner: (sid, owner) => { events.push(`O:${sid}:${owner}`); pool.allocated(sid)[34] = owner & 0xff; },
        boundaryAt: (x, y, type) => { assert.equal(type, EDGE); return x === 10 && y === 9 ? edge : 0; },
        regionAt: () => 7,
    };
    const paint = new RawRegionPaint(pool, frames, islands, TERRAIN, EDGE, new GameRandom(1), state, ownership, hooks);
    return { pool, start, farther, edge, state, events, ownership, paint };
}

test('지역 도장의 경계 후보는 edgeFarm 이웃과 다른 소유자의 지면까지 소유자를 바꾼다', () => {
    const scene = boundaryScene();
    scene.paint.paint({ x: 10, y: 10, owner: 4, theme: 0 });
    // 시작 지면 → 경계 edgeFarm → edgeFarm 의 이웃인 다른 소유자 지면 순서이다.
    assert.deepEqual(scene.events, [`O:${scene.start}:4`, `O:${scene.edge}:4`, `O:${scene.farther}:4`]);
    assert.equal(scene.pool.slot(scene.farther)[34], 4);
    // 경계 캐시는 별개로 만들어진다.
    assert.equal(scene.state.coreVariants.length, 99);
    assert.equal(scene.state.boundaryVariants.length, 99);
    assert.notEqual(scene.state.boundaryVariants, scene.state.coreVariants);
    // 영향 지역은 같은 지역 번호를 한 번만 기록한다(시작 후보의 지역 7).
    assert.equal(scene.ownership.affected.length, 1);
    const record = scene.ownership.affected[0];
    assert.equal(floatToBits(record.x), record.xBits);
    assert.equal(floatToBits(record.y), record.yBits);
    assert.deepEqual([record.xBits, record.yBits, record.region], [10, 9, 7]);
    // 이웃 후보는 북·동·남·서 순서로 중복 없이 이어진다.
    const points = scene.state.boundary.map((point) => `${point.x},${point.y}`);
    assert.deepEqual(points.slice(0, 4), ['10,9', '11,10', '10,11', '9,10']);
    assert.deepEqual(points.slice(4), ['10,8', '11,9', '10,10', '9,9']);
});

// 현재 프레임 코드는 지면 타입의 표에서 읽고 edgeFarm 타입의 표는 읽지 않는다(edgeFarm 표가 비어도 경계 도장은 정상). (2026-11-11 WB04-E296)
test('지역 도장은 경계 edgeFarm 의 프레임 코드도 지면 타입의 표에서 읽는다', () => {
    const frames = tables().frames;
    frames[EDGE] = new FrameCodeTable([]);
    const scene = boundaryScene(frames);
    scene.paint.paint({ x: 10, y: 10, owner: 4, theme: 0 });
    assert.deepEqual(scene.events, [`O:${scene.start}:4`, `O:${scene.edge}:4`, `O:${scene.farther}:4`]);
});

// 월드 왼쪽 밖으로 이어지는 edgeFarm 경계 후보에서 안쪽 프레임의 캐시 좌표(x ÷ 3)가 음수가 되면 거부한다(x = -3 에서 -1). (2026-11-11 WB04-E296)
test('지역 도장은 음수 캐시 좌표를 거부한다', () => {
    const pool = new SidPool(32768);
    const { frames } = tables();
    const make = (type, owner, x, y) => {
        const sid = pool.allocate(2);
        const raw = pool.allocated(sid);
        raw.fill(0);
        raw[10] = type;
        raw[34] = owner;
        putPoint(raw, x, y);
        return sid;
    };
    const start = make(TERRAIN, 2, 0, 10);
    const edges = new Map([[-1, make(EDGE, 5, -1, 10)], [-2, make(EDGE, 5, -2, 10)], [-3, make(EDGE, 5, -3, 10)]]);
    const islands = new Uint16Array(65536);
    islands[10 * 256 + 0] = start;
    const hooks = {
        setOwner: (sid, owner) => { pool.allocated(sid)[34] = owner & 0xff; },
        boundaryAt: (x, y) => (y === 10 ? (edges.get(x) ?? 0) : 0),
        regionAt: () => 7,
    };
    const paint = new RawRegionPaint(pool, frames, islands, TERRAIN, EDGE, new GameRandom(1), new RegionPaintState(), new RegionOwnershipState(), hooks);
    assert.throws(() => paint.paint({ x: 0, y: 10, owner: 4, theme: 0 }), /변형 캐시 좌표/);
});
