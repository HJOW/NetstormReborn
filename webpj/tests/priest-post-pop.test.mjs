// 사제 후처리 접두 raw-priest-post-pop.js(RawPriestPostPop·PriestPostPopState·resetPriestOwnerState·makePriestPostPopProcessHooks)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: priestpostpop-x86.tsv 10.78(originals) Prefix 2392행(상태·extra·소유자·원래 단어·HP·1/4 모드·찾음/확보 결과·목록 용량/개수/항목 → 목록 결과·소유자 상태 9칸·검색/확보/생성 호출 기록·슬롯 50바이트).
//    CD 10.72·10.37 행은 웹 범위(10.78) 밖이다. 최대 HP 는 실제 SquidReward 를 쓴다.
//  - 계약: priest_postpop_allocation_failure_skips_hp_lookup_and_creation·PriestPostPopCurrent_*(물리 슬롯과 사전 검사 분리, signed 목록 관문과 현재 개수 재조회, 현재 사제 TYPE DWORD, 도달 장치만 요구)·
//    PriestOwnerState_*(데이터 전용 정리의 진단 복귀 규칙, 예약 효과 뒤 현재 공유 표 선택). 사제 후반(Tail)과의 가상 분배·ProcessHost 통합은 다음 단위의 몫이다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E63: 사제 후처리 접두 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { GameRandom } from '../www/core/random.js';
import { ScrambledSpStore } from '../www/core/sp-store.js';
import { SquidReward, SquidRewardState } from '../www/core/squid-reward.js';
import { SquidPostPopState, SquidPostPopList } from '../www/core/squid-postpop-state.js';
import { PRIEST_TYPE, PRIEST_VTABLE } from '../www/core/raw-priest-owner.js';
import { PRIEST_REGEN_EVENT, PriestPostPopState, RawPriestPostPop, resetPriestOwnerState, makePriestPostPopProcessHooks } from '../www/core/raw-priest-post-pop.js';

// 원본 관측 10.78 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E63)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'priestpostpop-x86.tsv'))?.filter((row) => row[1] === 'originals') ?? null;

// 관찰 SID, 판본당 관찰 행 수, 공통 가상 표이다. (2026-11-08 WB04-E63)
const SOURCE = 50;
const ROWS = 2392;
const COMMON_VTABLE = 0x11010000;

// 예외가 나는지 본다. (2026-11-08 WB04-E63)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 바이트 배열을 소문자 hex 로 만든다. (2026-11-08 WB04-E63)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// payload float 의 비트 문자열이다. (2026-11-08 WB04-E63)
const bits = (value) => String(floatToBits(value));

// 188칸 타입 표이다. (2026-11-08 WB04-E63)
const priestTypes = () => Array.from({ length: 188 }, () => ({ cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, maxHitPoints: 0 }));

// 콘솔 전용 입력 SID 와 읽기만 하는 HP 모듈을 준비한 장면이다. (2026-11-08 WB04-E63)
function prefixScene() {
    const pool = new SidPool(24000, { server: false });
    // 실제 client 영역에서 관찰 SID 까지 할당한다.
    for (let id = 5; id <= SOURCE; id += 1) assert.equal(pool.allocate(2), id);
    const scene = { pool, store: new ScrambledSpStore(new GameRandom(), () => 1), book: new SquidPostPopState(), hpState: new SquidRewardState(), state: new PriestPostPopState(), types: priestTypes(), hp: null, events: [], storage: [], found: false, allocated: false, priestHolder: { priestType: PRIEST_TYPE } };
    scene.hpState.share('priestType', scene.priestHolder, 'priestType');
    scene.makeHp = () => { scene.hp = new SquidReward(pool, scene.types, scene.book, scene.hpState, scene.store); };
    // 검색 시점의 목록 개수와 확보/생성 인자만 기록한다. 최대 HP 는 실제 복원 함수를 사용한다.
    scene.hooks = () => ({
        findRegular: (sid, event) => { scene.events.push(`F:${sid}:${event}:${scene.state.priests.count}`); return scene.found; },
        reserveRegular: () => { scene.events.push('A:40'); return scene.allocated; },
        addRegular: (sid, event, payload) => scene.events.push(`R:${sid}:${event}:${floatToBits(payload)}`)
    });
    // 입력만 공급하고 기대 출력이나 원본 분기는 계산하지 않는다.
    scene.prepare = (row) => {
        assert.equal(row.length, 19);
        scene.events = [];
        scene.found = Number(row[10]) !== 0;
        scene.allocated = Number(row[11]) !== 0;
        scene.hpState.priestQuarterHp = Number(row[9]) !== 0;
        scene.types[PRIEST_TYPE].maxHitPoints = Number(row[8]) | 0;
        scene.types[PRIEST_TYPE].flags1 = 0x00069012;
        scene.types[PRIEST_TYPE].flags2 = 0x00210000;
        scene.makeHp();
        scene.storage = row[14].split(',').map(Number);
        const capacity = Number(row[12]);
        assert.ok(capacity <= scene.storage.length);
        scene.state.priests.entries = scene.storage.slice(0, capacity);
        scene.state.priests.count = Number(row[13]);
        // 원본 실행기에 공급한 9칸의 비영 상태이다.
        for (let index = 0; index < scene.state.ownerState.length; index += 1) scene.state.ownerState[index] = 0xabc000 + index;
        const raw = pool.allocated(SOURCE);
        raw.fill(0);
        put(raw, 0, PRIEST_VTABLE);
        raw[10] = PRIEST_TYPE;
        raw[11] = Number(row[4]);
        raw[34] = Number(row[6]);
        raw[40] = Number(row[5]);
        put(raw, 12, Number(row[7]), 2);
        put(raw, 14, floatToBits(20.75));
        put(raw, 18, floatToBits(21.9));
        put(raw, 26, 42);
    };
    // 용량 뒤의 입력 저장소는 접근하지 않으므로 원래 값 그대로 비교한다.
    scene.list = () => {
        scene.state.priests.entries.forEach((value, index) => { scene.storage[index] = value; });
        return `${scene.state.priests.count}:${scene.storage.join(',')}`;
    };
    scene.owners = () => scene.state.ownerState.join(',');
    scene.eventText = () => (scene.events.length === 0 ? '-' : scene.events.join(';'));
    return scene;
}

test('PriestPostPop: 원본 10.78 접두 2392행의 목록·소유자 칸·호출 기록·슬롯 50바이트가 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    const scene = prefixScene();
    let count = 0;
    // 입력마다 새 HP 타입 표를 연결하되 실제 prefix 몸체는 동일하게 사용한다.
    for (const row of rows) {
        scene.prepare(row);
        new RawPriestPostPop(scene.pool, scene.hp, scene.state, scene.hooks()).prefix(SOURCE, Number(row[3]));
        assert.equal(scene.list(), row[15], `행 ${count}`);
        assert.equal(scene.owners(), row[16], `행 ${count}`);
        assert.equal(scene.eventText(), row[17], `행 ${count}`);
        assert.equal(hex(scene.pool.slot(SOURCE)), row[18], `행 ${count}`);
        count += 1;
    }
    assert.equal(count, ROWS);
    assert.ok(!scene.store.initialized);
});

test('PriestPostPop: 사전 검사는 명시 validate 로만 거부하고 풀 혼합은 구성 때 거부하며 PATCH 구성은 효과 훅을 요구하지 않는다', { skip: !rows && '원본 fixture 없음' }, () => {
    const scene = prefixScene();
    scene.prepare(rows[0]);
    const hooks = scene.hooks();
    hooks.reserveRegular = undefined;
    const deferred = new RawPriestPostPop(scene.pool, scene.hp, scene.state, {});
    const partial = new RawPriestPostPop(scene.pool, scene.hp, scene.state, hooks);
    assert.ok(deferred.pool === scene.pool && partial.pool === scene.pool);
    assert.ok(throws(() => new RawPriestPostPop(new SidPool(24000, { server: false }), scene.hp, scene.state, scene.hooks())));
    const prefix = new RawPriestPostPop(scene.pool, scene.hp, scene.state, scene.hooks());
    // 실패 입력은 raw 슬롯·목록·소유자 칸과 호출 기록을 바꾸지 않아야 한다.
    const reject = (sid) => {
        const before = hex(scene.pool.slot(SOURCE));
        const list = scene.list();
        const owners = scene.owners();
        assert.ok(throws(() => prefix.validate(sid)));
        assert.ok(hex(scene.pool.slot(SOURCE)) === before && scene.list() === list && scene.owners() === owners && scene.events.length === 0);
    };
    reject(0);
    reject(51);
    scene.pool.allocated(SOURCE)[10] = 82;
    reject(SOURCE);
    scene.pool.allocated(SOURCE)[10] = PRIEST_TYPE;
    put(scene.pool.allocated(SOURCE), 0, COMMON_VTABLE);
    reject(SOURCE);
    put(scene.pool.allocated(SOURCE), 0, PRIEST_VTABLE);
    scene.state.priests.count = 5;
    reject(SOURCE);
});

test('PriestPostPop: 확보 실패는 HP 조회와 생성을 생략하지만 목록과 소유자 상태 쓰기는 계속 진행한다', { skip: !rows && '원본 fixture 없음' }, () => {
    const scene = prefixScene();
    const row = [...rows[0]];
    row[3] = '1';
    row[5] = '0';
    row[6] = '1';
    row[7] = '43905';
    row[10] = '0';
    row[11] = '0';
    row[12] = '4';
    row[13] = '0';
    scene.prepare(row);
    scene.hpState.debugAsserts = 1;
    scene.types[PRIEST_TYPE].flags1 = 0;
    scene.makeHp();
    assert.ok(throws(() => scene.hp.typeHitPoints(PRIEST_TYPE)));
    const prefix = new RawPriestPostPop(scene.pool, scene.hp, scene.state, scene.hooks());
    prefix.prefix(SOURCE, 1);
    assert.ok(scene.state.priests.count === 1 && scene.state.priests.entries[0] === SOURCE && scene.state.ownerState[1] === 0);
    assert.equal(scene.eventText(), 'F:50:602:1;A:40');
    // 이미 있으면 확보/HP 조회도 하지 않는다.
    scene.events = [];
    scene.found = true;
    prefix.prefix(SOURCE, 1);
    assert.ok(scene.eventText() === 'F:50:602:1' && scene.state.priests.count === 1);
});

// 사제 VT/TYPE158·소유자 1·원래 소유자 1·좌표(20.75, 21.9)·HP 600 의 관찰 슬롯을 만든 현재 장면이다. TYPE 158/82 의 최대 HP 입력을 담는다. (2026-11-08 WB04-E63)
function currentScene() {
    const scene = prefixScene();
    scene.found = false;
    scene.allocated = true;
    for (const [number, value] of [[PRIEST_TYPE, 200], [82, 600]]) {
        scene.types[number].maxHitPoints = value;
        scene.types[number].flags1 = 0x69012;
        scene.types[number].flags2 = 0x210000;
    }
    scene.makeHp();
    scene.state.priests.entries = [0, 0, 0, 0];
    for (let index = 0; index < scene.state.ownerState.length; index += 1) scene.state.ownerState[index] = 0xabc000 + index;
    const raw = scene.pool.physical(SOURCE);
    raw.fill(0);
    put(raw, 0, PRIEST_VTABLE);
    raw[10] = PRIEST_TYPE;
    raw[34] = 1;
    put(raw, 12, 1, 2);
    put(raw, 14, floatToBits(20.75));
    put(raw, 18, floatToBits(21.9));
    put(raw, 26, 600);
    // 현재 장면의 검색 기록은 호출 시점의 현재 목록(공유 목록 포함) 개수를 읽는다.
    scene.hooks = () => ({
        findRegular: (sid, event) => { scene.events.push(`F:${sid}:${event}:${scene.state.currentPriests().readCount()}`); return scene.found; },
        reserveRegular: () => { scene.events.push("A:40"); return scene.allocated; },
        addRegular: (sid, event, payload) => scene.events.push(`R:${sid}:${event}:${floatToBits(payload)}`)
    });
    scene.body = (hooks = scene.hooks()) => new RawPriestPostPop(scene.pool, scene.hp, scene.state, hooks, () => scene.priestHolder.priestType);
    return scene;
}

test('PriestPostPopCurrent: 접두는 사전 검사 없이 현재 물리 슬롯으로 목록·회복·소유자 칸을 쓴다', () => {
    const s = currentScene();
    const raw = s.pool.physical(SOURCE);
    raw[10] = 82;
    raw[11] = 9;
    const body = s.body();
    assert.ok(throws(() => body.validate(SOURCE)) && s.events.length === 0);
    // 분자 TYPE158 200/6*50=1650, 객체 TYPE82 최대HP600 이다.
    body.prefix(SOURCE, 1);
    assert.equal(s.eventText(), `F:50:602:1;A:40;R:50:602:${bits(2.75)}`);
    assert.ok(s.state.priests.count === 1 && s.state.priests.entries[0] === 50 && s.state.ownerState[1] === 0 && s.state.ownerState[2] === 0xabc002 && raw[11] === 9);
});

test('PriestPostPopCurrent: 00414450 의 signed 관문·현재 개수 재조회·처음 개수 저장 위치와 음수 저장 경계를 보존한다', () => {
    const s = currentScene();
    const body = s.body();
    s.found = true;
    s.state.priests.count = 5;
    body.prefix(SOURCE, 1);
    assert.ok(s.eventText() === 'F:50:602:5' && s.state.priests.count === 5 && s.state.ownerState[1] === 0);
    s.events = [];
    s.state.ownerState[1] = 7;
    s.state.priests.count = 0xffffffff;
    assert.ok(throws(() => body.prefix(SOURCE, 1)) && s.events.length === 0 && s.state.priests.count === 0xffffffff && s.state.ownerState[1] === 7);
    s.state.priests.count = 1;
    s.state.priests.entries[0] = 50;
    s.state.ownerState[1] = 7;
    body.prefix(SOURCE, 1);
    assert.ok(s.eventText() === 'F:50:602:1' && s.state.priests.count === 1 && s.state.ownerState[1] === 0);
    const bound = new SquidPostPopList();
    const storage = [7, 50, 9, 0];
    const reads = [2, 1, 1, 2];
    let read = 0;
    let written = 0;
    bound.bindCurrent({ storage: () => storage, capacity: () => 4, readCount: () => reads[read++], writeCount: (value) => { written = value; } });
    s.state.sharedPriests = bound;
    s.events = [];
    body.prefix(SOURCE, 1);
    assert.deepEqual(storage, [7, 50, 50, 0]);
    assert.ok(written === 2 && read === 4 && s.eventText() === 'F:50:602:2');
});

test('PriestPostPopCurrent: 회복 분자는 호출 시점의 현재 사제 TYPE DWORD 를 읽고 최대 HP 1/4 비교도 같은 현재 값을 쓴다', () => {
    const s = currentScene();
    const body = s.body();
    // 분자 TYPE82 600/6*50=5000, 객체 TYPE158 최대HP 200 이다.
    s.priestHolder.priestType = 82;
    body.prefix(SOURCE, 1);
    // 같은 구성의 다음 호출은 바뀐 현재 DWORD 를 읽는다: 1650/200.
    s.priestHolder.priestType = PRIEST_TYPE;
    body.prefix(SOURCE, 1);
    // TYPE82 만 1/4: 150/6*50=1250, 객체 TYPE158 은 현재 사제 TYPE 가 아니라 200 이다.
    s.priestHolder.priestType = 82;
    s.hpState.priestQuarterHp = true;
    body.prefix(SOURCE, 1);
    assert.equal(s.eventText(), `F:50:602:1;A:40;R:50:602:${bits(25)};F:50:602:1;A:40;R:50:602:${bits(8.25)};F:50:602:1;A:40;R:50:602:${bits(6.25)}`);
    assert.ok(s.state.priests.count === 1 && s.hp.typeHitPoints(PRIEST_TYPE) === 200 && s.hp.typeHitPoints(82) === 150);
});

test('PriestPostPopCurrent: PATCH 접두는 도달한 검색/확보/생성 장치만 그 시점에 요구한다', () => {
    const s = currentScene();
    const bare = s.body({});
    const raw = s.pool.physical(SOURCE);
    raw[40] = 8;
    bare.prefix(SOURCE, 1);
    assert.ok(s.state.priests.count === 0 && s.state.ownerState[1] === 0xabc001);
    raw[40] = 0;
    bare.prefix(SOURCE, 0);
    assert.ok(s.state.priests.count === 0 && s.state.ownerState[1] === 0);
    s.state.ownerState[1] = 7;
    assert.ok(throws(() => bare.prefix(SOURCE, 1)) && s.state.priests.count === 1 && s.state.ownerState[1] === 7);
    const foundHooks = s.hooks();
    foundHooks.reserveRegular = undefined;
    foundHooks.addRegular = undefined;
    s.found = true;
    const found = s.body(foundHooks);
    found.prefix(SOURCE, 1);
    assert.ok(s.eventText() === 'F:50:602:1' && s.state.ownerState[1] === 0);
    s.events = [];
    s.found = false;
    s.state.ownerState[1] = 7;
    assert.ok(throws(() => found.prefix(SOURCE, 1)) && s.eventText() === 'F:50:602:1' && s.state.ownerState[1] === 7);
    const reserveHooks = s.hooks();
    reserveHooks.addRegular = undefined;
    const reserve = s.body(reserveHooks);
    s.events = [];
    s.allocated = false;
    reserve.prefix(SOURCE, 1);
    assert.ok(s.eventText() === 'F:50:602:1;A:40' && s.state.ownerState[1] === 0);
    s.events = [];
    s.allocated = true;
    s.state.ownerState[1] = 7;
    assert.ok(throws(() => reserve.prefix(SOURCE, 1)) && s.eventText() === 'F:50:602:1;A:40' && s.state.ownerState[1] === 7);
});

test('PriestOwnerState: 데이터 전용 정리는 지정한 1..8 DWORD 만 쓰고 목록/0번을 보존하며 범위 밖은 진단 뒤 쓰지 않는다', () => {
    const state = new PriestPostPopState();
    state.priests.entries = [50, 50, 60, 99];
    state.priests.count = 3;
    state.ownerState.fill(0x77777777);
    const shared = new Array(9).fill(0).map((_, index) => 0x1000 + index);
    state.sharedOwnerState = shared;
    const fallback = [...state.ownerState];
    const entries = [...state.priests.entries];
    const expected = [...shared];
    assert.equal(state.currentOwnerState(), shared);
    resetPriestOwnerState(state.currentOwnerState(), 1);
    expected[1] = 0;
    resetPriestOwnerState(state.currentOwnerState(), 8);
    expected[8] = 0;
    assert.ok(shared.every((value, index) => value === expected[index]) && state.ownerState.every((value, index) => value === fallback[index]));
    assert.ok(state.priests.entries.every((value, index) => value === entries[index]) && state.priests.count === 3 && shared[0] === 0x1000);
    const diagnosed = [];
    for (const player of [0, 9, 0xffffffff]) {
        resetPriestOwnerState(state.currentOwnerState(), player, (invalid) => diagnosed.push(invalid));
        assert.ok(shared.every((value, index) => value === expected[index]), '원문은 진단이 복귀해도 무효 칸을 쓰지 않는다');
    }
    assert.deepEqual(diagnosed, [0, 9, 0xffffffff]);
    assert.ok(throws(() => resetPriestOwnerState(state.currentOwnerState(), 0)));
});

test('PriestOwnerState: 후처리는 예약 효과 뒤의 현재 공유 표를 선택하고 독립 fallback 을 사후 동기화하지 않는다', { skip: !rows && '원본 fixture 없음' }, () => {
    const scene = prefixScene();
    const row = [...rows[0]];
    row[3] = '1';
    row[5] = '0';
    row[6] = '1';
    row[7] = '43905';
    row[10] = '1';
    row[12] = '4';
    row[13] = '0';
    scene.prepare(row);
    const first = new Array(9).fill(31);
    const second = new Array(9).fill(41);
    scene.state.sharedOwnerState = first;
    const fallback = [...scene.state.ownerState];
    const hooks = scene.hooks();
    hooks.findRegular = (sid, event) => {
        assert.ok(sid === SOURCE && event === PRIEST_REGEN_EVENT && scene.state.priests.count === 1);
        scene.state.sharedOwnerState = second;
        return true;
    };
    new RawPriestPostPop(scene.pool, scene.hp, scene.state, hooks).prefix(SOURCE, 1);
    assert.ok(first.every((value) => value === 31) && second[1] === 0 && second[0] === 41 && scene.state.ownerState.every((value, index) => value === fallback[index]));
    assert.ok(scene.state.priests.count === 1 && scene.state.priests.entries[0] === SOURCE && scene.state.currentOwnerState() === second);
    assert.equal(typeof makePriestPostPopProcessHooks, 'function');
});
