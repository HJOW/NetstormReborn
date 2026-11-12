// 경로 프로세스(RawPathProcess)와 실제 길 찾기(RawPathSearch)를 같은 raw 저장소에서 잇는 통합을 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: pathinit-x86.tsv 의 `originals` 32행 — 실제 Init 의 표면 0/존재·void/live·Graph·flags·기존 경로(profile 0~7 × flags 0/1 × 표면 × void × legacy)가 가상 Pop 인자·현재 단계·꼭짓점 수·부모 상태/좌표·
//    꼭짓점 문자열·거리표/큐 FNV32·방향 3 선행 비트까지 같다.
//  - cpppj 시험 이식: 시작 표면이 0 이면 검색 경계보다 앞서 반환하고 표면이 있으면 실제 BFS/flags/current 에 연결, 검색 꼭짓점을 실제 Kernel 에서 한 단계씩 진행해 공간 갱신·1000 도착의 Form 삭제까지 같은 저장소를 쓴다.
//  - 웹 계약: makePathSearchHooks 는 풀이 다르면 거부하고 원래 훅 묶음을 바꾸지 않는다.
// cpppj RawPathSearchTests.cpp 의 SearchProcessEffects(부모의 도착·소리 응답만 입력 경계)를 같은 순서로 옮겼다. 10.72(CD) 행은 범위 밖이다. 실제 게임·브라우저 실행이 아니다.
// (2026-11-13 WB04-E349: 6-P RawPathProcess 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get, fnv32, floatToBits } from './raw-support.mjs';
import { SearchScene } from './path-search-scene.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidUnpop } from '../www/core/squid-unpop.js';
import { SquidPop, RAW_POP_RESULT } from '../www/core/squid-pop.js';
import { RawSquidDestroy } from '../www/core/squid-destroy.js';
import { Kernel } from '../www/core/kernel.js';
import { typeConstructorAddress } from '../www/core/squid-constructors.js';
import { SquidProcessHost, SquidProcessState } from '../www/core/squid-process.js';
import { makePathSearchHooks } from '../www/core/raw-path-search.js';
import { RawPathProcess, RawPathProcessState } from '../www/core/raw-path-process.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-13 WB04-E349)
const initRows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'pathinit-x86.tsv'));

// 검색 장면 위에 Pop/Unpop 과 부모 가상 효과(도착·소리·정리)를 연결한 묶음이다. 도착·계속·효과 요청은 입력 경계의 응답과 순서를 기록하며 게임 부모 몸체 복원으로 세지 않는다. (2026-11-13 WB04-E349)
function searchEffects(scene, pop, unpop, events) {
    const pool = scene.pool;
    return {
        unpop: (sid, flags) => {
            events.push('unpop');
            unpop.unpop(sid, scene.types[pool.slot(sid)[10]], flags);
        },
        pop: (sid, point, flags) => {
            events.push('pop');
            assert.equal(pop.pop(sid, scene.types[pool.slot(sid)[10]], 1, 1, point.x, point.y, flags), RAW_POP_RESULT.registered);
        },
        repop: (sid, flags) => {
            const raw = pool.slot(sid);
            events.push('repop');
            const view = new DataView(raw.buffer, raw.byteOffset, raw.byteLength);
            assert.equal(pop.pop(sid, scene.types[raw[10]], 1, 1, view.getFloat32(14, true), view.getFloat32(18, true), flags), RAW_POP_RESULT.registered);
        },
        halt: () => 10,
        arrival: () => { events.push('arrival'); return 1000; },
        stepSound: (sid, x, y, sound) => { assert.equal(sound, 0); events.push('sound'); },
        soundId: () => 0,
        clearBlocked: () => { throw new Error('이번 경로 입력은 겹침을 만들지 않습니다'); },
        cleanup: () => { events.push('cleanup'); },
    };
}

// 장면에 Pop/Unpop/빈 SHP 표를 연결하는 공통 준비이다: 부모(SID 5)의 가상 표를 지원 표(0x501dd0)로 둔다. (2026-11-13 WB04-E349)
function prepare(profile, owner, legacy) {
    const scene = new SearchScene();
    scene.setup(profile, owner, true, 6, legacy, 0);
    put(scene.pool.allocated(5), 0, 0x501dd0);
    const pop = new SquidPop(scene.pool, scene.hash, scene.spots);
    const unpop = new SquidUnpop(scene.pool, scene.hash, scene.spots);
    const shapes = scene.types.map(() => ({ frames: [] }));
    return { scene, pop, unpop, shapes };
}

test('RawPathProcess+Search: Init 은 시작 표면이 0 이면 검색 경계보다 앞서 반환하고, 표면이 있으면 실제 BFS/flags/current 에 연결한다', () => {
    const { scene, pop, unpop, shapes } = prepare(0, 1, false);
    const parent = 5;
    const clock = new SquidProcessState();
    const state = new RawPathProcessState(clock);
    const events = [];
    const hooks = searchEffects(scene, pop, unpop, events);
    const missing = new RawPathProcess(scene.pool, scene.hash, scene.types, scene.frames, shapes, scene.spots, state, hooks, {});
    const previous = [{ x: 9, y: 9 }];
    missing.runtime.setPath(previous, [0]);
    missing.runtime.restoreStep(1, 45);
    const surface = scene.hash.cell(0, 10, 10);
    scene.hash.setCell(0, 10, 10, 0);
    // 표면 0 이면 검색 훅이 없어도 Pop/검색/current 쓰기 모두 없이 돌아온다.
    missing.initializePath(parent, scene.destination, scene.start);
    assert.ok(events.length === 0 && missing.runtime.current === 1 && missing.runtime.count === 1);
    // 표면이 있으면 검색 경계 미연결을 명시적으로 멈춘다.
    scene.hash.setCell(0, 10, 10, surface);
    assert.throws(() => missing.initializePath(parent, scene.destination, scene.start), /실제 Path BFS 미복원 경계/);
    assert.equal(events.length, 0);
    const search = scene.searcher();
    const process = new RawPathProcess(scene.pool, scene.hash, scene.types, scene.frames, shapes, scene.spots, state, makePathSearchHooks(scene.pool, search, hooks), {});
    process.runtime.setPath(previous, [0]);
    process.runtime.restoreStep(1, 45);
    process.initializePath(parent, scene.destination, scene.start, 1);
    assert.ok(process.runtime.current === 0 && process.runtime.count > 2);
    assert.deepEqual(events, ['pop']);
    assert.equal(scene.pool.slot(parent)[11] & 4, 0);
    assert.equal(scene.hash.cell(1, 4, 4), parent);
    // 다른 풀의 검색기와는 훅 묶음을 만들지 않으며 원래 훅 묶음에는 search 를 덧씌우지 않는다.
    const foreign = new SidPool(32768, { server: false });
    assert.throws(() => makePathSearchHooks(foreign, search, hooks), TypeError);
    assert.equal(hooks.search, undefined);
});

test('RawPathProcess+Search: 검색 꼭짓점을 실제 Kernel 에서 한 단계씩 진행해 공간 갱신·1000 도착의 Form 삭제까지 같은 저장소를 쓴다', () => {
    const { scene, pop, unpop, shapes } = prepare(0, 1, false);
    const parent = 5;
    // 공통 ProcessForm 생성이 읽는 원문 내장 생성자 주소를 TYPE 입력에 명시한다(cpppj MakeFormTypeInputs).
    for (const number of [1, 2, 3, 6]) scene.types[number].constructorAddress = typeConstructorAddress(number);
    const destroy = new RawSquidDestroy(scene.pool, unpop, scene.types);
    const kernel = new Kernel(32);
    const clock = new SquidProcessState();
    const state = new RawPathProcessState(clock);
    const host = new SquidProcessHost(scene.pool, scene.types, kernel, destroy, clock);
    const events = [];
    const search = scene.searcher();
    const next = new RawPathProcess(scene.pool, scene.hash, scene.types, scene.frames, shapes, scene.spots, state, makePathSearchHooks(scene.pool, search, searchEffects(scene, pop, unpop, events)), {});
    const process = RawPathProcess.attach(host, next, parent, 0x10);
    try {
        assert.ok(process && kernel.size === 1);
        process.initializePath(parent, scene.destination, scene.start);
        process.prepare(0, 1, 90);
        const form = process.form;
        const count = process.runtime.count;
        assert.ok(count >= 2);
        const destination = process.runtime.point(count - 1);
        // 한 호출에 한 꼭짓점만 넘는 실제 Step 을 도착까지 필요한 횟수 안에서 호출한다.
        for (let step = 0; step < count + 3 && kernel.size; step += 1) {
            clock.now = 100;
            kernel.runFrame();
        }
        assert.equal(kernel.size, 0);
        assert.ok((scene.pool.slot(form)[11] & 1) !== 0 && (scene.pool.slot(parent)[11] & 4) === 0);
        assert.equal(get(scene.pool.slot(parent), 14), floatToBits(destination.x));
        assert.equal(get(scene.pool.slot(parent), 18), floatToBits(destination.y));
        assert.ok(events.includes('arrival'));
        assert.equal(events.at(-1), 'cleanup');
    } finally {
        kernel.clear();
    }
});

test('RawPathProcess+Search: 원본 10.78 pathinit 32행 — 실제 Init 의 표면 0/존재·void/live·Graph·flags·기존 경로 출력을 가상 Pop 인자까지 대조한다', { skip: !initRows && '원본 fixture 없음' }, () => {
    let count = 0;
    // 기대값은 실제 PE 의 Init→표면 조회→BFS 관찰만 사용한다.
    for (const row of initRows.filter((candidate) => candidate[0] === 'originals')) {
        const input = row[1].split(',');
        assert.ok(row.length === 12 && input.length === 5);
        const profile = Number(input[0]);
        const flags = Number(input[1]);
        const surface = Number(input[2]) !== 0;
        const voidParent = Number(input[3]) !== 0;
        const legacy = Number(input[4]) !== 0;
        const { scene, pop, unpop, shapes } = prepare(profile, 1, legacy);
        const parent = 5;
        const raw = scene.pool.allocated(parent);
        raw[11] = voidParent ? 4 : 0;
        put(raw, 14, floatToBits(scene.start.x));
        put(raw, 18, floatToBits(scene.start.y));
        if (!surface) scene.hash.setCell(0, 10, 10, 0);
        const clock = new SquidProcessState();
        const state = new RawPathProcessState(clock);
        const events = [];
        const search = scene.searcher();
        const hooks = searchEffects(scene, pop, unpop, events);
        const actualPop = hooks.pop;
        let popEvent = '-';
        // 명시 관찰 경계도 실제 공간 등록을 수행하며 원본이 넘긴 좌표/flags 를 그대로 기록한다.
        hooks.pop = (sid, point, value) => {
            popEvent = `pop,${floatToBits(point.x)},${floatToBits(point.y)},${value}`;
            actualPop(sid, point, value);
        };
        const process = new RawPathProcess(scene.pool, scene.hash, scene.types, scene.frames, shapes, scene.spots, state, makePathSearchHooks(scene.pool, search, hooks), {});
        process.runtime.bytes.set(scene.path.bytes);
        process.runtime.restoreStep(legacy ? 1 : 0, 0);
        process.initializePath(parent, scene.destination, scene.start, flags);
        scene.path = process.runtime;
        const where = `행 ${count} (${row[1]})`;
        assert.equal(popEvent, row[2], `${where} Pop`);
        assert.equal(scene.path.current, Number(row[3]), `${where} current`);
        assert.equal(scene.path.count, Number(row[4]), `${where} count`);
        assert.equal(raw[11], Number(row[5]), `${where} 부모 상태`);
        assert.equal(get(raw, 14), Number(row[6]), `${where} x`);
        assert.equal(get(raw, 18), Number(row[7]), `${where} y`);
        assert.equal(scene.points(), row[8], `${where} 꼭짓점`);
        // 검색하지 않은 작업 배열도 0 이며, 검색한 배열의 전체 바이트는 native FNV 와 같아야 한다.
        const work = search.work;
        assert.equal(fnv32(new Uint8Array(work.distance.buffer, work.distance.byteOffset, work.distance.byteLength)), Number(row[9]), `${where} 거리표`);
        assert.equal(fnv32(work.queue), Number(row[10]), `${where} 큐`);
        assert.equal(scene.state.directionPrerequisite[3], Number(row[11]), `${where} 방향 3 선행`);
        count += 1;
    }
    assert.equal(count, 32);
});
