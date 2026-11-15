// 아웃포스트 목록 수명 raw-outpost-lifecycle.js(RawOutpostLifecycle·makeOutpostDamageablePreDestroyHooks)를 원본 10.78 관측 768행과 cpppj `OutpostLifecycleTests.cpp` 계약으로 대조한다(WB04-E556).
//  - outpostlifecycle-x86.tsv 의 10.78(originals) 768행: P(postPop)/D(preDestroy) 접두의 두 목록 전체(비활성 꼬리 포함)·개수·지역 통지(R)/작업장 base(B) 순서·raw 슬롯 쓰기.
//  - 계약: 같은 실제 Player 기준점/Damageable 에 연결한 등록·제거가 기준점을 즉시 바꾸는 흐름, 손상 연결·누락 효과를 목록 변경 전에 거부.
//  - 범위 밖: originalCD·original1037 행은 웹 범위(10.78) 밖이다. 실제 게임·브라우저 실행이 아니다. (2026-11-15 WB04-E556)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidPostPopList, SquidPostPopState } from '../www/core/squid-postpop-state.js';
import { ContainedFinderState } from '../www/core/raw-contained-finder.js';
import { PlayerPlacementAnchorState, RawPlayerPlacementAnchor } from '../www/core/raw-player-placement-anchor.js';
import { DamageablePreDestroyState, RawDamageablePreDestroy, DAMAGEABLE_SOUND } from '../www/core/raw-damageable-pre-destroy.js';
import { SquidFactory } from '../www/core/squid-factory.js';
import { typeConstructorAddress } from '../www/core/squid-constructors.js';
import { RawOutpostLifecycle, makeOutpostDamageablePreDestroyHooks } from '../www/core/raw-outpost-lifecycle.js';

// 원본 관측 10.78 행이다. 파일이 없으면 해당 검사를 건너뛴다. (2026-11-15 WB04-E556)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'outpostlifecycle-x86.tsv'))?.filter((row) => row[0] === 'originals') ?? null;
// 판본당 관찰 행 수, 목록 용량, 입력 SID, 채움 값, 풀 크기, 아웃포스트 TYPE 이다(cpppj 와 같음). (2026-11-15 WB04-E556)
const ROWS = 768;
const LIST_SIZE = 8;
const SOURCE = 50;
const OUTPOST = 126;
const FILL = 0xabababab;
const CAPACITY = 24000;
// 원본 행의 쉼표 DWORD 목록을 원래 순서와 비활성 꼬리까지 읽는다(계산하지 않음). (2026-11-15 WB04-E556)
const numbers = (text) => text.split(',').map((part) => Number(part) >>> 0);
// 바이트 배열을 소문자 16진 문자열로 만든다(원문 Hex). (2026-11-15 WB04-E556)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 독립 fixture 의 초기 목록이다: 장면 번호(0..7)로 고르며 두 번째 목록은 번호를 3 만큼 돌려 쓴다. 비활성 꼬리는 채움 값이다. (2026-11-15 WB04-E556)
function input(scene, second) {
    const lists = [[], [60], [50], [60, 50, 61, 50], [60, 61, 62, 63, 64, 65, 66, 67], [50, 60, 50, 61, 50, 62, 50, 63], [50, 50, 50, 50, 50, 50, 50], [61, 60, 50]];
    const list = new SquidPostPopList();
    const entries = lists[second ? (scene + 3) % lists.length : scene];
    list.entries = [...entries, ...new Array(LIST_SIZE - entries.length).fill(FILL)];
    list.count = entries.length;
    return list;
}

test('RawOutpostLifecycle: 원본 10.78 목록 접두 768행의 두 목록·개수·지역/base 순서·raw 쓰기가 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    assert.equal(rows.length, ROWS);
    const pool = new SidPool(CAPACITY, { server: false });
    let count = 0;
    // 같은 입력 순서로 wrapper 를 호출하며 기대 분기는 구현하지 않는다.
    for (const row of rows) {
        assert.equal(row.length, 14);
        const raw = pool.physical(SOURCE);
        raw.fill(0xab);
        raw[10] = OUTPOST;
        raw[11] = 2;
        raw[40] = Number(row[2]);
        put(raw, 14, Number(row[6]));
        put(raw, 18, Number(row[7]));
        const additional = input(Number(row[4]), false);
        const nearest = input(Number(row[4]), true);
        let events = '';
        const base = (sid, flags) => {
            assert.ok(sid === SOURCE && flags === Number(row[3]));
            if (events) events += ';';
            events += `B:${flags}:${additional.count}:${nearest.count}`;
        };
        const lifecycle = new RawOutpostLifecycle(pool, additional, nearest, {
            regionChanged: (x, y) => {
                events = `R:${floatToBits(x)}:${floatToBits(y)}:${additional.count}:${nearest.count}`;
                if (Number(row[5])) { raw[40] = 9; put(raw, 14, 0x42c80000); }
            },
            validatePostPop: () => {},
            postPop: base,
            validatePreDestroy: () => {},
            preDestroy: base
        });
        if (row[1] === 'P') lifecycle.postPop(SOURCE, Number(row[3])); else lifecycle.preDestroy(SOURCE, Number(row[3]));
        const label = `행 ${count}`;
        assert.equal(additional.count, Number(row[8]), `${label} 추가 개수`);
        assert.deepEqual(additional.entries, numbers(row[9]), `${label} 추가 목록`);
        assert.equal(nearest.count, Number(row[10]), `${label} 거리 개수`);
        assert.deepEqual(nearest.entries, numbers(row[11]), `${label} 거리 목록`);
        assert.equal(events, row[12], `${label} 사건`);
        assert.equal(hex(raw), row[13], `${label} raw`);
        count += 1;
    }
    assert.equal(count, ROWS);
});

test('RawOutpostLifecycle: 같은 실제 Player 기준점/Damageable 에 연결한 등록·제거가 기준점을 즉시 바꾼다', () => {
    const pool = new SidPool(CAPACITY, { server: false });
    const types = Array.from({ length: 188 }, () => ({ flags1: 0, flags2: 0, footX: 0, footY: 0, group: 0 }));
    types[83].flags1 = 0x800;
    types[84].flags1 = 0x800;
    types[83].flags2 = 2;
    types[84].flags2 = 6;
    const surfaces = new Uint16Array(65536);
    surfaces[21 * 256 + 20] = 60;
    const books = new SquidPostPopState();
    books.ownerFactories[1].entries = [51];
    books.ownerFactories[1].count = 1;
    const additional = new SquidPostPopList();
    additional.entries = new Array(4).fill(0);
    const nearest = new SquidPostPopList();
    nearest.entries = new Array(4).fill(0);
    const contained = new ContainedFinderState();
    const state = new PlayerPlacementAnchorState();
    state.graphReady = 1;
    // 작업장의 종속은 linked 를 켜지만 작업장 자체는 다른 그래프이므로 추가 목록만 기준점이 된다.
    for (const sid of [50, 51, 60, 61]) {
        const raw = pool.physical(sid);
        raw.fill(0);
        raw[10] = sid === 50 ? OUTPOST : sid === 51 ? 84 : sid === 60 ? 83 : 6;
        raw[34] = 1;
        raw[30] = sid === 51 ? 8 : 7;
        put(raw, 14, floatToBits(20));
        put(raw, 18, floatToBits(21));
    }
    // 실제 outpost 생성자 쓰기를 적용하되 전체 Pop/작업장 프로세스는 공급하지 않는다.
    types[OUTPOST].constructorAddress = typeConstructorAddress(OUTPOST);
    new SquidFactory(pool, types).construct(OUTPOST, SOURCE);
    put(pool.physical(51), 6, 61, 2);
    put(pool.physical(61), 18, 80, 2);
    put(pool.physical(61), 20, 0x10, 2);
    const anchor = new RawPlayerPlacementAnchor(pool, types, surfaces, books, additional, contained, state);
    const regionAnchors = [];
    let basePosts = 0;
    let basePres = 0;
    let collapses = 0;
    let sounds = 0;
    const damageable = new RawDamageablePreDestroy(pool, new Uint8Array(65536), contained, new DamageablePreDestroyState(), {
        collapse: (sid) => { assert.equal(sid, SOURCE); collapses += 1; },
        explosion: () => {},
        sound: (event) => { assert.equal(event.sound, DAMAGEABLE_SOUND.collapse); sounds += 1; },
        releaseContained: () => {},
        validateBase: () => {},
        base: (sid, flags) => { assert.ok(sid === SOURCE && flags === 0x200000); basePres += 1; }
    });
    const lifecycle = new RawOutpostLifecycle(pool, additional, nearest, makeOutpostDamageablePreDestroyHooks(pool, damageable, {
        regionChanged: (x, y) => regionAnchors.push(anchor.locate(1, 80, x, y)),
        validatePostPop: () => {},
        postPop: () => { basePosts += 1; }
    }));
    assert.equal(anchor.locate(1, 80, 20, 21), 0);
    lifecycle.postPop(SOURCE, 0);
    assert.equal(anchor.locate(1, 80, 20, 21), 0);
    lifecycle.postPop(SOURCE, 1);
    assert.equal(anchor.locate(1, 80, 20, 21), 50);
    lifecycle.postPop(SOURCE, 1);
    assert.deepEqual([additional.count, nearest.count, regionAnchors], [1, 1, [50, 50]]);
    lifecycle.preDestroy(SOURCE, 0x200000);
    assert.deepEqual([anchor.locate(1, 80, 20, 21), additional.count, nearest.count, regionAnchors], [0, 0, 0, [50, 50, 0]]);
    assert.deepEqual([basePosts, basePres, collapses, sounds], [3, 1, 1, 1]);
    lifecycle.postPop(SOURCE, 1);
    assert.deepEqual([anchor.locate(1, 80, 20, 21), additional.count, nearest.count], [50, 1, 1]);
});

test('RawOutpostLifecycle: 손상 연결·누락 효과를 목록 변경 전에 거부하고 원본이 읽지 않는 손상 목록은 막지 않는다', () => {
    const pool = new SidPool(CAPACITY, { server: false });
    const raw = pool.physical(SOURCE);
    raw.fill(0);
    raw[10] = OUTPOST;
    const list = (entries, count) => { const result = new SquidPostPopList(); result.entries = entries; result.count = count; return result; };
    const additional = list([60, FILL], 1);
    const nearest = list([61, FILL], 3);
    let regions = 0;
    let bases = 0;
    const hooks = { regionChanged: () => { regions += 1; }, validatePostPop: () => {}, postPop: () => { bases += 1; }, validatePreDestroy: () => {}, preDestroy: () => { bases += 1; } };
    assert.throws(() => new RawOutpostLifecycle(pool, additional, additional, hooks), TypeError);
    for (const name of ['regionChanged', 'validatePostPop', 'postPop', 'validatePreDestroy', 'preDestroy']) assert.throws(() => new RawOutpostLifecycle(pool, additional, nearest, { ...hooks, [name]: undefined }), TypeError);
    const lifecycle = new RawOutpostLifecycle(pool, additional, nearest, hooks);
    const before = [...additional.entries];
    // 두 번째 목록의 count 가 용량을 넘으면 두 번째를 읽는 순간 거부하며 첫 목록도 바꾸지 않는다.
    assert.throws(() => lifecycle.postPop(SOURCE, 1));
    assert.deepEqual([additional.entries, additional.count, regions, bases], [before, 1, 0, 0]);
    assert.throws(() => lifecycle.preDestroy(SOURCE, 1));
    assert.deepEqual([additional.entries, additional.count, regions, bases], [before, 1, 0, 0]);
    // 원본 분기에서 읽지 않는 손상 목록은 abstract 객체/두 번째 Pop 의 base 호출을 막지 않는다.
    lifecycle.postPop(SOURCE, 0);
    raw[40] = 9;
    lifecycle.postPop(SOURCE, 1);
    lifecycle.preDestroy(SOURCE, 1);
    assert.deepEqual([regions, bases], [0, 3]);
    // free 슬롯(자산이 아님)은 효과 전에 거부한다.
    raw[11] = 1;
    assert.throws(() => lifecycle.preDestroy(SOURCE, 1));
    raw[11] = 0;
    raw[40] = 0;
    // 선검사가 거부하면 목록은 그대로이다.
    nearest.count = 1;
    const guarded = new RawOutpostLifecycle(pool, additional, nearest, { ...hooks, validatePostPop: () => { throw new Error('원래 부모 경계의 사전 거부'); } });
    assert.throws(() => guarded.postPop(SOURCE, 1));
    assert.deepEqual([additional.entries, additional.count, regions], [before, 1, 0]);
    // 다른 풀의 Damageable 연결은 거부한다.
    const other = new SidPool(CAPACITY, { server: false });
    const otherDamageable = new RawDamageablePreDestroy(other, new Uint8Array(65536), new ContainedFinderState(), new DamageablePreDestroyState(), {
        collapse: () => {}, explosion: () => {}, sound: () => {}, releaseContained: () => {}, validateBase: () => {}, base: () => {}
    });
    assert.throws(() => makeOutpostDamageablePreDestroyHooks(pool, otherDamageable, hooks), TypeError);
});

test('RawOutpostLifecycle.preDestroyNative: 실제 455840 순서(extra → 두 현재 목록 압축 → 현재 지역 → Damageable 부모)와 음수 count·비활성 꼬리 보존', () => {
    const pool = new SidPool(CAPACITY, { server: false });
    const raw = pool.physical(SOURCE);
    raw.fill(0);
    put(raw, 14, floatToBits(12.5));
    put(raw, 18, floatToBits(34.25));
    const list = (entries, count) => { const result = new SquidPostPopList(); result.entries = entries; result.count = count; return result; };
    const events = [];
    const hooks = { regionChanged: (x, y) => events.push(`R:${x}:${y}`), damageablePre: (sid, flags) => events.push(`D:${sid}:${flags}`) };
    // 일반 객체: 두 목록에서 모든 중복을 앞으로 압축하고 현재 count 만 줄이며 꼬리는 지우지 않는다.
    let additional = list([60, 50, 61, 50, FILL, FILL, FILL, FILL], 4);
    let nearest = list([50, 60, FILL, FILL], 2);
    RawOutpostLifecycle.preDestroyNative(pool, additional, nearest, SOURCE, 0x200000, hooks);
    assert.deepEqual([additional.count, additional.entries.slice(0, 4), nearest.count, nearest.entries.slice(0, 2)], [2, [60, 61, 61, 50], 1, [60, 60]]);
    assert.deepEqual(events, ['R:12.5:34.25', `D:${SOURCE}:${0x200000}`]);
    // abstract/buried(extra 9)는 목록/지역을 건드리지 않고 부모만 호출한다.
    events.length = 0;
    raw[40] = 9;
    additional = list([50, 60], 2);
    RawOutpostLifecycle.preDestroyNative(pool, additional, list([50], 1), SOURCE, 7, hooks);
    assert.deepEqual([additional.count, events], [2, [`D:${SOURCE}:7`]]);
    // 음수 count 는 배열 포인터도 읽지 않고 그대로 보존한다(signed 비교).
    events.length = 0;
    raw[40] = 0;
    additional = list([50, 60], 0xfffffffe);
    nearest = list([50], 0xffffffff);
    RawOutpostLifecycle.preDestroyNative(pool, additional, nearest, SOURCE, 0, hooks);
    assert.deepEqual([additional.count, nearest.count, additional.entries], [0xfffffffe, 0xffffffff, [50, 60]]);
    // 선택한 장치가 없으면 앞선 목록 압축을 되돌리지 않고 명시 오류이다.
    additional = list([50, 60], 2);
    assert.throws(() => RawOutpostLifecycle.preDestroyNative(pool, additional, list([], 0), SOURCE, 0, { damageablePre: () => {} }), /미연결/);
    assert.equal(additional.count, 1);
});

test('RawOutpostLifecycle: 자산 TYPE 범위(70..187)·두 목록 선검사 순서·선검사 위치·실제 Damageable 선검사 연결', () => {
    const pool = new SidPool(CAPACITY, { server: false });
    const raw = pool.physical(SOURCE);
    raw.fill(0);
    const list = (entries, count) => { const result = new SquidPostPopList(); result.entries = entries; result.count = count; return result; };
    const quiet = { regionChanged: () => {}, validatePostPop: () => {}, postPop: () => {}, validatePreDestroy: () => {}, preDestroy: () => {} };
    const lifecycle = new RawOutpostLifecycle(pool, list([FILL, FILL], 0), list([FILL, FILL], 0), quiet);
    // 자산 TYPE 은 70 이상 187 이하만 허용한다(69 와 188 은 효과 전에 거부).
    for (const [type, accepted] of [[69, false], [70, true], [187, true], [188, false]]) {
        raw[10] = type;
        if (accepted) lifecycle.postPop(SOURCE, 0); else assert.throws(() => lifecycle.postPop(SOURCE, 0), RangeError, `TYPE ${type}`);
    }
    raw[10] = OUTPOST;
    // 첫 목록이 손상(count 가 용량 초과)이고 둘째가 정상이면 둘째를 바꾸기 전에 거부한다(두 목록을 먼저 검사).
    let additional = list([60, FILL], 3);
    let nearest = list([61, FILL], 1);
    let guarded = new RawOutpostLifecycle(pool, additional, nearest, quiet);
    assert.throws(() => guarded.postPop(SOURCE, 1));
    assert.deepEqual([nearest.entries, nearest.count], [[61, FILL], 1]);
    // 삭제도 둘째가 손상이면 첫 목록(SOURCE 포함)을 바꾸기 전에 거부한다.
    additional = list([SOURCE, 60, FILL, FILL], 2);
    nearest = list([61, FILL], 3);
    guarded = new RawOutpostLifecycle(pool, additional, nearest, quiet);
    assert.throws(() => guarded.preDestroy(SOURCE, 1));
    assert.deepEqual([additional.entries, additional.count], [[SOURCE, 60, FILL, FILL], 2]);
    // 삭제 선검사가 거부하면 어떤 목록/지역도 바꾸지 않는다.
    let regions = 0;
    additional = list([SOURCE, 60], 2);
    nearest = list([SOURCE, 61], 2);
    guarded = new RawOutpostLifecycle(pool, additional, nearest, { ...quiet, regionChanged: () => { regions += 1; }, validatePreDestroy: () => { throw new Error('선검사 거부'); } });
    assert.throws(() => guarded.preDestroy(SOURCE, 1), /선검사 거부/);
    assert.deepEqual([additional.count, nearest.count, regions], [2, 2, 0]);
    // 실제 Damageable 선검사 연결: 좌표가 보드 밖이면 같은 풀의 Damageable 선검사가 거부해 두 목록이 그대로이다.
    const damageable = new RawDamageablePreDestroy(pool, new Uint8Array(65536), new ContainedFinderState(), new DamageablePreDestroyState(), {
        collapse: () => {}, explosion: () => {}, sound: () => {}, releaseContained: () => {}, validateBase: () => {}, base: () => {}
    });
    additional = list([SOURCE, 60], 2);
    nearest = list([SOURCE, 61], 2);
    const real = new RawOutpostLifecycle(pool, additional, nearest, makeOutpostDamageablePreDestroyHooks(pool, damageable, quiet));
    raw[11] = 0;
    put(raw, 14, floatToBits(-5));
    put(raw, 18, floatToBits(3));
    assert.throws(() => real.preDestroy(SOURCE, 1), RangeError);
    assert.deepEqual([additional.count, nearest.count], [2, 2]);
});

test('RawOutpostLifecycle.preDestroyNative: extra 의 buried 단독 비트·첫 목록 우선 압축·현재 count 가 용량을 넘으면 물리 부족', () => {
    const pool = new SidPool(CAPACITY, { server: false });
    const raw = pool.physical(SOURCE);
    raw.fill(0);
    const list = (entries, count) => { const result = new SquidPostPopList(); result.entries = entries; result.count = count; return result; };
    const events = [];
    const hooks = { regionChanged: () => events.push('R'), damageablePre: () => events.push('D') };
    // extra 의 buried(1) 단독 비트도 목록/지역을 건너뛴다.
    raw[40] = 1;
    let additional = list([SOURCE, 60], 2);
    RawOutpostLifecycle.preDestroyNative(pool, additional, list([SOURCE], 1), SOURCE, 0, hooks);
    assert.deepEqual([additional.count, events], [2, ['D']]);
    // 첫 목록이 물리 부족(count 가 길이 초과)이면 둘째 목록은 읽기 전에 중단한다.
    raw[40] = 0;
    events.length = 0;
    additional = list([SOURCE, 60], 5);
    const nearest = list([SOURCE, 61], 2);
    assert.throws(() => RawOutpostLifecycle.preDestroyNative(pool, additional, nearest, SOURCE, 0, hooks), RangeError);
    // 첫 목록은 물리 끝에 닿기 전까지만 압축하고(제거 하나 뒤 한 칸 당김) 둘째 목록과 지역/부모 효과에는 닿지 않는다.
    assert.deepEqual([additional.entries, nearest.entries, nearest.count, events], [[60, 60], [SOURCE, 61], 2, []]);
});
