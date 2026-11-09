// 지역 소유 투표 raw-region-ownership.js(RawRegionOwnership)를 원본 10.78 관측 regionownership-x86.tsv 와 cpppj `RegionOwnershipTests.cpp` 계약으로 대조한다(WB04-E249).
//  - 관측 440행(originals): 12개 목록 장면 × 여섯 좌표(소수 경계 포함) × 세 소유자 변형 × mode(0/10)·옵션·지역 경계에서 모든 지역 getter 호출(R)·옵션 조회(C)·도장(P) 사건 순서와 인자, 누적 영향 지역, 월드 다시 그리기/미니맵 DWORD 와
//    입력 목록 배열 불변을 대조한다. 지역 getter 는 실제 RawCanonPlacementTerrain 과 snap 어댑터(makeTerrainRegionOwnershipHooks)를 거친다. updateNative 도 같은 440행을 한 번 더 재생한다(건강한 목록에서는 typed 와 같아야 한다).
//  - 계약: 필수 경계 누락·같은 목록 두 개·다른 풀 거부, NaN 좌표는 투표·도장·상태 변경 전에 거부, 소유자/목록 count 손상 거부, 공유 월드 다시 그리기/미니맵 요청 DWORD 는 서로 다른 소유자의 현재값을 읽고 쓴다.
//  - Native: 음수 signed count 는 배열을 읽지 않고, 현재 저장소 물리 밖은 항목 선택 때 거부하며, 현재 TYPE 장치(typeTheme)가 있으면 독립 테마 표를 쓰지 않는다.
// CD 10.72·10.37 행과 outpost 수명 통합 시험(RawOutpostLifecycle, 멀티플레이 전용)은 웹 범위 밖이다. 실제 게임·브라우저 실행이 아니다. (2026-11-10 WB04-E249)
import assert from 'node:assert/strict';
import path from 'node:path';
import test from 'node:test';
import { RawCanonPlacementTerrain, createCanonPlacementTerrainState } from '../www/core/raw-canon-placement-terrain.js';
import { RawRegionOwnership, RegionOwnershipState, makeTerrainRegionOwnershipHooks } from '../www/core/raw-region-ownership.js';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidPostPopList } from '../www/core/squid-postpop-state.js';
import { REPO_ROOT } from './helpers.mjs';
import { bitsToFloat, floatToBits, put, readRows } from './raw-support.mjs';

// 분석 도구의 풀 크기·wrapper 초기 DWORD 와 각 PE 의 관측 행 수이다. (2026-11-10 WB04-E249)
const CAPACITY = 24000;
const ROWS = 440;
const INITIAL_REVISION = 0xffffffff;
const INITIAL_DIRTY = 0x12345678;

// 기대값을 포함하지 않는 목록(추가, 작업장)·raw 좌표/소유자·타입 테마 입력이다. (2026-11-10 WB04-E249)
const SCENES = [
    [[], []], [[50], []], [[50, 53], [60]], [[50], [60, 63]], [[50, 51, 53], [60, 61]],
    [[50, 51, 52, 53, 54, 55], [60, 61, 62, 63, 64, 65]], [[50, 50, 50], [61, 64]], [[51, 54], [60, 63, 64]],
    [[], [60, 60, 63, 61]], [[52, 55], [62, 65]], [[50, 51], [60, 61, 63, 64, 65]], [[55, 50, 53, 50], [65, 60, 60, 63]],
];
const COORDS = [[10.2, 20.3], [30.000001, 21.00002], [40.5, 22.5], [10.00002, 20.000001], [30.5, 21.3], [40.000001, 22.000001]].map(([x, y]) => [Math.fround(x), Math.fround(y)]);
const OWNERS = [[1, 2, 0, 1, 2, 8], [3, 3, 3, 3, 3, 3], [0, 0, 8, 2, 2, 8]];
const WORKSHOP_OWNERS = [[1, 2, 8, 1, 2, 8], [3, 4, 3, 3, 3, 3], [2, 0, 8, 2, 0, 8]];
const THEMES = [0, 1, 2, 3, 0xffffffff, 0];

// 10.78(originals) 관측 행만 쓴다. 파일이 없으면 검사를 건너뛴다. (2026-11-10 WB04-E249)
const allRows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'regionownership-x86.tsv'));
const rows = allRows?.filter((row) => row[0] === 'originals') ?? null;

// 10진 문자열 DWORD 를 부호 없는 수로 읽는다. (2026-11-10 WB04-E249)
const number = (text) => Number(BigInt(text) & 0xffffffffn);

// 누적 입력 기록을 만들며 중복 지역을 지우지 않는다. 원본이 투표에 쓰지 않는 좌표도 같은 기록으로 공급한다. (2026-11-10 WB04-E249)
function affectedOf(variant) {
    const regions = variant === 0 ? [7, 9] : variant === 1 ? [7, 7, 9] : [9, 9, 7];
    return regions.map((region, index) => ({ x: Math.fround(90.5 + index), y: Math.fround(91.25 + index), region }));
}

// 목록 하나이다: 항목과 개수, 뒤에 지워지면 안 되는 표식(0xabababab)을 8칸까지 채운다. (2026-11-10 WB04-E249)
function listOf(items, padded = true) {
    const list = new SquidPostPopList();
    list.entries = [...items];
    list.count = items.length;
    if (padded) while (list.entries.length < 8) list.entries.push(0xabababab);
    return list;
}

// 합성 고정 지도(두 섬·다리·빈 칸)와 실제 지형 어댑터 장면이다. (2026-11-10 WB04-E249)
function terrainScene() {
    const pool = new SidPool(CAPACITY, { server: false });
    const types = Array.from({ length: 188 }, () => ({ flags1: 0, flags2: 0, footX: 0, footY: 0 }));
    const frames = types.map(() => []);
    const islands = new Uint16Array(65536);
    // 원본 실행기와 같은 x 구간을 공급하며 결과는 계산하지 않는다.
    for (let y = 0; y < 256; y += 1) {
        for (let x = 0; x < 256; x += 1) islands[y * 256 + x] = x < 20 ? 100 : x < 40 ? 101 : x < 60 ? 102 : 0;
    }
    const state = createCanonPlacementTerrainState();
    const terrain = new RawCanonPlacementTerrain(pool, types, frames, islands, state, {
        candidatePermission: () => false, shapePermission: () => false, finishRelations: () => false,
    });
    return { pool, types, islands, state, terrain };
}

// 모든 getter·옵션·도장 사건과 원본 wrapper 상태를 같은 실제 지역 지도에 재생한다. native 이면 updateNative 를 부른다. (2026-11-10 WB04-E249)
function replay(native) {
    const { pool, types, state: terrainState, terrain } = terrainScene();
    const themes = new Array(188).fill(0);
    let count = 0;
    // 입력별 새 목록/전역을 준비하며 이전 행의 테마나 누적 지역을 재사용하지 않는다.
    for (const row of rows) {
        assert.equal(row.length, 13);
        const scene = number(row[1]);
        const variant = number(row[2]);
        // 실제 raw 목록 항목을 공급하며 원본 owner/type getter 의 오프셋을 유지한다.
        for (let i = 0; i < COORDS.length; i += 1) {
            themes[83 + i] = THEMES[(i + variant) % THEMES.length];
            // 추가 목록(50..)과 작업장 목록(60..)의 소유자 입력은 독립적이다.
            for (const second of [false, true]) {
                const raw = pool.physical((second ? 60 : 50) + i);
                raw.fill(0);
                raw[10] = 83 + i;
                raw[34] = (second ? WORKSHOP_OWNERS : OWNERS)[variant][i];
                put(raw, 14, floatToBits(COORDS[i][0]));
                put(raw, 18, floatToBits(COORDS[i][1]));
            }
        }
        // 지역 getter 는 raw +8 과 현재 genus 를 읽으며 bridge 는 sentinel 을 반환한다.
        for (const sid of [100, 101, 102]) {
            const type = sid - 17;
            const raw = pool.physical(sid);
            raw.fill(0);
            raw[10] = type;
            put(raw, 8, sid === 100 ? 7 : sid === 101 ? 9 : 42, 2);
            types[type].flags2 = sid === 100 ? 2 : sid === 101 ? 0x1000002 : 4;
        }
        terrainState.emptyRegion = number(row[8]);
        const additional = listOf(SCENES[scene][0]);
        const workshops = listOf(SCENES[scene][1]);
        const beforeAdditional = [...additional.entries];
        const beforeWorkshops = [...workshops.entries];
        const state = new RegionOwnershipState({ affected: affectedOf(variant), revision: INITIAL_REVISION, dirty: INITIAL_DIRTY });
        const events = [];
        let paints = 0;
        // 호출 순서를 저장하며 외부 도장 뒤의 누적 기록만 입력으로 제공한다.
        const hooks = makeTerrainRegionOwnershipHooks(pool, terrain, {
            paint: (paint) => {
                events.push(`P:${paint.x}:${paint.y}:${paint.owner}:${paint.theme}`);
                paints += 1;
                if (number(row[5]) && paints === 2) state.affected = affectedOf(variant);
            },
            themeMode: () => { events.push(`C:${row[4]}`); return number(row[4]); },
        });
        const actualRegion = hooks.regionAt;
        // 실제 terrain 어댑터 진입 전에 같은 raw float 인자 비트를 관찰한다.
        hooks.regionAt = (x, y) => {
            events.push(`R:${floatToBits(x)}:${floatToBits(y)}`);
            return actualRegion(x, y);
        };
        const ownership = new RawRegionOwnership(pool, additional, workshops, themes, state, hooks);
        if (native) ownership.updateNative(bitsToFloat(number(row[6])), bitsToFloat(number(row[7])), number(row[3]));
        else ownership.update(bitsToFloat(number(row[6])), bitsToFloat(number(row[7])), number(row[3]));
        const regions = state.affected.length ? state.affected.map((value) => value.region).join(',') : '-';
        assert.equal(events.join(';'), row[9], `행 ${count} 사건`);
        assert.equal(regions, row[10], `행 ${count} 영향 지역`);
        assert.deepEqual([state.revision, state.dirty], [number(row[11]), number(row[12])], `행 ${count} DWORD`);
        assert.deepEqual([additional.entries, workshops.entries], [beforeAdditional, beforeWorkshops]);
        count += 1;
    }
    assert.equal(count, ROWS);
}

test('region-ownership: 원본 10.78 지역 소유 투표 440행(두 도장·테마·재귀·DWORD 감김)이 typed update 와 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    assert.equal(allRows.length, 3 * ROWS);
    replay(false);
});

test('region-ownership: updateNative 가 같은 440행에서 typed update 와 같은 결과를 낸다', { skip: !rows && '원본 fixture 없음' }, () => replay(true));

// 부정확한 연결/좌표를 묵시적인 기본 판정으로 바꾸지 않도록 검사한다.
test('region-ownership: 필수 경계 누락·같은 목록·NaN 좌표·소유자와 count 손상을 거부한다', () => {
    const pool = new SidPool(CAPACITY, { server: false });
    const additional = listOf([50], false);
    const workshops = listOf([], false);
    const themes = new Array(188).fill(0);
    const state = new RegionOwnershipState({ affected: [{ x: 1, y: 2, region: 7 }], revision: 5, dirty: 9 });
    let paints = 0;
    const hooks = { regionAt: () => 7, paint: () => { paints += 1; }, themeMode: () => 1 };
    assert.throws(() => new RawRegionOwnership(pool, additional, workshops, themes, state, { ...hooks, paint: null }), TypeError);
    assert.throws(() => new RawRegionOwnership(pool, additional, workshops, themes, state, { ...hooks, themeMode: null }), TypeError);
    assert.throws(() => new RawRegionOwnership(pool, additional, additional, themes, state, hooks), TypeError);
    assert.throws(() => new RawRegionOwnership(pool, additional, workshops, new Array(187).fill(0), state, hooks), TypeError);
    const ownership = new RawRegionOwnership(pool, additional, workshops, themes, state, hooks);
    // NaN 좌표: typed update 는 투표·도장·상태 변경 전에 거부한다. Native 몸체는 좌표 절삭을 실제 도장 위치에서 하므로
    // 영향 지역 기록을 비운 뒤(원문 순서) 첫 도장에서 거부하며, 도장과 월드/미니맵 DWORD 는 바뀌지 않는다.
    assert.throws(() => ownership.update(Number.NaN, 1), RangeError);
    assert.deepEqual([paints, state.affected.length, state.revision, state.dirty], [0, 1, 5, 9]);
    assert.throws(() => ownership.updateNative(Number.NaN, 1), RangeError);
    assert.deepEqual([paints, state.affected.length, state.revision, state.dirty], [0, 0, 5, 9]);
    // int 범위 밖 절삭 값(2^31 정확히, -2^31 보다 작은 값)도 같은 방식으로 거부한다.
    for (const bad of [2147483648, -2147483904]) assert.throws(() => ownership.update(bad, 1), RangeError, String(bad));
    assert.equal(paints, 0);
    // 다른 풀의 지형은 지역 어댑터 구성 때 거부한다.
    const { terrain } = terrainScene();
    assert.throws(() => makeTerrainRegionOwnershipHooks(pool, terrain, hooks), TypeError);
    // 소유자 9 이상과 count 손상(count > 용량)은 도장 전에 거부한다.
    const raw = pool.physical(50);
    raw.fill(0);
    raw[34] = 9;
    assert.throws(() => ownership.update(1, 2), RangeError);
    assert.equal(paints, 0);
    additional.count = 2;
    assert.throws(() => ownership.update(1, 2), Error);
    assert.equal(paints, 0);
});

// 월드 다시 그리기 DWORD 의 증가와 미니맵 요청 DWORD 의 1 쓰기를 서로 다른 현재 소유자에 적용한다.
test('region-ownership: 공유 월드 다시 그리기 증가와 미니맵 요청 1 쓰기는 서로 다른 소유자의 현재값을 쓴다', () => {
    const redraw = { value: 17 };
    const map = { value: 0xffffffff };
    const pool = new SidPool(CAPACITY, { server: false });
    const outpost = pool.allocate(2);
    const raw = pool.allocated(outpost);
    raw.fill(0);
    raw[10] = 83;
    raw[34] = 2;
    put(raw, 14, floatToBits(10.2));
    put(raw, 18, floatToBits(20.3));
    const additional = listOf([outpost], false);
    const workshops = listOf([], false);
    const state = new RegionOwnershipState({ affected: [], revision: 0x12345678, dirty: 0x87654321, sharedWorldRedraw: redraw, sharedMapRevision: map });
    const paints = [];
    let mutate = false;
    // 원문 두 도장 호출의 인자와 마지막 효과 뒤 현재값 재조회를 관측한다.
    const hooks = {
        regionAt: () => 7,
        themeMode: () => 1,
        paint: (paint) => {
            paints.push(paint);
            if (mutate && paints.length === 2) { redraw.value = 37; map.value = 0xffffffff; }
        },
    };
    const ownership = new RawRegionOwnership(pool, additional, workshops, new Array(188).fill(0), state, hooks);
    assert.equal(state.worldRedraw, 17);
    assert.equal(state.mapRevision, 0xffffffff);
    ownership.update(Math.fround(10.2), Math.fround(20.3));
    assert.equal(paints.length, 2);
    assert.deepEqual([paints[0].owner, paints[1].owner, paints[1].x, paints[1].y], [0, 2, 10, 20]);
    // 미니맵은 0xffffffff 를 증가해 0 으로 만드는 연산이 아니라 1 을 쓴다. 독립 필드는 건드리지 않는다.
    assert.deepEqual([redraw.value, map.value, state.revision, state.dirty], [18, 1, 0x12345678, 0x87654321]);
    paints.length = 0;
    mutate = true;
    map.value = 19;
    ownership.update(Math.fround(10.2), Math.fround(20.3));
    // paint 앞의 18 을 캐시한 증가나 사후 사본 동기화가 아니라 마지막 효과 뒤 현재값(37)을 증가한다.
    assert.deepEqual([paints.length, redraw.value, map.value, state.revision, state.dirty], [2, 38, 1, 0x12345678, 0x87654321]);
});

// Native 몸체의 목록 계약: 음수 signed count 는 배열을 읽지 않고, 물리 밖 항목은 선택할 때 거부하며, 현재 TYPE 장치가 있으면 독립 테마 표를 쓰지 않는다.
test('region-ownership: updateNative 의 signed count·현재 저장소 물리·현재 TYPE 테마 장치', () => {
    const pool = new SidPool(CAPACITY, { server: false });
    const first = pool.allocate(2);
    const raw = pool.allocated(first);
    raw.fill(0);
    raw[10] = 83;
    raw[34] = 2;
    const events = [];
    const hooks = { regionAt: () => 7, themeMode: () => 1, paint: (paint) => events.push(`${paint.owner}:${paint.theme}`) };
    // 음수 count(0xffffffff)는 두 목록 모두 배열을 읽지 않아 표가 비고, 중립 도장 둘(1, 0)만 남는다. typed update 는 count 가 용량을 넘어 거부한다.
    const negative = listOf([first], false);
    negative.count = 0xffffffff;
    const state = new RegionOwnershipState({ affected: [{ x: 0, y: 0, region: 7 }, { x: 0, y: 0, region: 7 }] });
    const ownership = new RawRegionOwnership(pool, negative, listOf([], false), new Array(188).fill(0), state, hooks);
    assert.throws(() => ownership.update(1, 2), Error);
    ownership.updateNative(1, 2);
    assert.deepEqual(events, ['1:0', '0:0']);
    assert.deepEqual([state.affected.length, state.revision, state.dirty], [0, 1, 1]);
    // 선택한 항목이 현재 저장소 물리 밖이면 그 항목을 고를 때 거부한다.
    const short = listOf([first], false);
    short.count = 2;
    const shortOwnership = new RawRegionOwnership(pool, short, listOf([], false), new Array(188).fill(0), new RegionOwnershipState(), hooks);
    assert.throws(() => shortOwnership.updateNative(1, 2), RangeError);
    // 현재 TYPE 테마 장치가 있으면 독립 테마 표(길이 0)를 요구하지 않고 선택한 타입의 현재 DWORD 를 읽는다.
    events.length = 0;
    const types = [];
    const temple = pool.allocate(2);
    const templeRaw = pool.allocated(temple);
    templeRaw.fill(0);
    templeRaw[10] = 90;
    templeRaw[34] = 3;
    const withTemple = new RawRegionOwnership(pool, listOf([], false), listOf([temple], false), [], new RegionOwnershipState(), {
        ...hooks,
        typeTheme: (type) => { types.push(type); return 1; },
    });
    withTemple.updateNative(1, 2);
    assert.deepEqual(types, [90]);
    // 같은 소유자(3)의 신전 테마: 종류 1 → 3.
    assert.deepEqual(events, ['0:0', '3:3']);
});

// fixture 가 구별하지 못하는 경계(변형 시험에서 살아남은 입력): snap 편향은 float 비트 0x3f7fff58 이며 1 ulp 큰 값과 칸이 갈리는 좌표가 있고,
// Native 의 신전 테마 순회도 signed count 이다(음수는 배열을 읽지 않음). 표 DWORD 감김은 Int32Array 저장 자체가 하므로 `| 0` 유무는 동등 변형이다. (2026-11-10 WB04-E249)
test('region-ownership: snap 편향 칸 경계와 Native 신전 테마 순회의 signed count', () => {
    // 지도 x<1 은 섬 100(지역 7), 그 밖은 섬 101(지역 9)이다. 0x3727ffff 는 편향을 더해도 1 에 못 미쳐 0 칸, 0x37280000 은 정확히 1 이 되어 1 칸이다.
    const scene = terrainScene();
    for (let y = 0; y < 256; y += 1) for (let x = 0; x < 256; x += 1) scene.islands[y * 256 + x] = x < 1 ? 100 : 101;
    for (const [sid, region, flags2] of [[100, 7, 2], [101, 9, 2]]) {
        const raw = scene.pool.physical(sid);
        raw.fill(0);
        raw[10] = 83 + (sid - 100);
        put(raw, 8, region, 2);
        scene.types[raw[10]].flags2 = flags2;
    }
    const hooks = makeTerrainRegionOwnershipHooks(scene.pool, scene.terrain, { paint: () => {}, themeMode: () => 0 });
    assert.equal(hooks.regionAt(bitsToFloat(0x3727ffff), 0), 7);
    assert.equal(hooks.regionAt(bitsToFloat(0x37280000), 0), 9);
    // 추가 목록의 소유자 2 가 이기고 신전 목록 count 가 음수이면 신전 테마를 읽지 않아 테마 0 이다.
    const pool = new SidPool(CAPACITY, { server: false });
    const voter = pool.allocate(2);
    const raw = pool.allocated(voter);
    raw.fill(0);
    raw[10] = 83;
    raw[34] = 2;
    const events = [];
    const temples = listOf([voter], false);
    temples.count = 0xffffffff;
    const ownership = new RawRegionOwnership(pool, listOf([voter], false), temples, new Array(188).fill(1), new RegionOwnershipState(), {
        regionAt: () => 7, themeMode: () => 1, paint: (paint) => events.push(`${paint.owner}:${paint.theme}`),
    });
    ownership.updateNative(1, 2);
    assert.deepEqual(events, ['0:0', '2:0']);
});
