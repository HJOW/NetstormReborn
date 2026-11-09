// 보호막 수명 raw-forcefield-lifecycle.js(RawForcefieldPostPop·RawForcefieldRegular·ForcefieldRegularState·makeForcefieldPostPopProcessHooks·makeForcefieldFrameHooks)를
// 원본 10.78 관측과 cpppj 계약(ForcefieldLifecycleTests·ForcefieldRegularCurrentTests)으로 대조한다.
//  - 원본 관측: forcefieldpostpop-x86.tsv 의 10.78(originals) 1208행 — 입력(종류 Prefix/Regular·flags·예약 두 번의 성공 여부·사건·count·payload·좌표 비트·사제 TYPE·후보 배치·extra)에서
//    효과 기록(예약/Regular 추가/공통 base/프레임 진행/삭제)·반환 float 비트·보호막 50바이트 raw 전체. CD 10.72·10.37 행은 웹 범위(10.78) 밖이다.
//  - 계약: PostPrefixCurrent_*(선택하지 않은 장치 비요구, 두 예약 실패 시 생성 효과 비요구, 첫 생성 변이가 두 번째 예약에 영향 없음, 가상 표 분배·공통 부모 직행, 필수 예약 없을 때 공통 전 중단),
//    ForcefieldRegularCurrent_*(모르는 사건은 raw 비접근, 처리기 반납 중 callback, 장기 DWORD 와 raw float 의 타입별 finder 전달, 삭제 누락 시 완료된 검색 효과 보존, 진단의 현재 checking 읽기와 복귀),
//    ForcefieldLifecycle_RequiresBindingAndDiagnosesInvalidEvents(다른 풀 연결 거부·진단 장치 누락·해제).
//  - CD 전용 계약(구성 사전 검사)과 실제 공간·Process·삭제 수명 한 장면 조합(ComposesSpaceProcessesAndSelfDeletion)은 SquidDestroyLifecycle 이식(E213) 뒤 `forcefield-lifecycle-integration` 으로 닫는다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-10 WB04-E212: 보호막 수명 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get, floatToBits, bitsToFloat } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { SquidPop, RAW_POP_RESULT } from '../www/core/squid-pop.js';
import { SquidPostPop } from '../www/core/squid-postpop.js';
import { SquidPostPopState } from '../www/core/squid-postpop-state.js';
import {
    FORCEFIELD_TYPE, PATCH_FORCEFIELD_VTABLE, ForcefieldRegularState, RawForcefieldPostPop, RawForcefieldRegular,
    makeForcefieldFrameHooks, makeForcefieldPostPopProcessHooks,
} from '../www/core/raw-forcefield-lifecycle.js';

// 원본 관측 10.78 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-10 WB04-E212)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'forcefieldpostpop-x86.tsv'))?.filter((row) => row[0] === 'originals') ?? null;

// 관찰 대상 보호막 SID 와 판본당 입력 수, 풀 크기이다(cpppj kShield·kCases·kCapacity). (2026-11-10 WB04-E212)
const SHIELD = 50;
const CASES = 1208;
const CAPACITY = 24000;

// 두 주기 상수의 단정도 값이다. (2026-11-10 WB04-E212)
const FRAME_INTERVAL = bitsToFloat(0x3da3d70a);
const CHECK_INTERVAL = bitsToFloat(0x3c23d70a);

// 예외가 나는지 본다. (2026-11-10 WB04-E212)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 바이트 배열을 소문자 hex 로 만든다. (2026-11-10 WB04-E212)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 빈 TYPE 표 188칸이다(footX·footY 만 쓴다). (2026-11-10 WB04-E212)
const emptyTypes = () => Array.from({ length: 188 }, () => ({ cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, maxHitPoints: 0, zOrder: 0 }));

// 후보 61 까지 실제 할당을 유지하는 합성 장면이다(cpppj ForcefieldScene). 존재 판단은 실제 finder 에 맡기고 후보 입력만 만든다. (2026-11-10 WB04-E212)
function forcefieldScene() {
    const pool = new SidPool(CAPACITY, { server: false });
    for (let sid = 5; sid <= 61; sid += 1) assert.equal(pool.allocate(2), sid);
    const scene = { pool, hash: new SquidHash(), types: emptyTypes(), events: [] };
    // 한 행의 입력으로 보호막 슬롯·후보·해시를 처음부터 다시 만든다.
    scene.prepare = (row) => {
        scene.hash.reset();
        scene.events = [];
        scene.types = emptyTypes();
        const raw = pool.allocated(SHIELD);
        raw.fill(0);
        put(raw, 0, PATCH_FORCEFIELD_VTABLE);
        raw[10] = FORCEFIELD_TYPE;
        raw[34] = 1;
        raw[40] = Number(row[12]);
        put(raw, 14, Number(row[8]));
        put(raw, 18, Number(row[9]));
        scene.types[FORCEFIELD_TYPE].footX = 1;
        scene.types[FORCEFIELD_TYPE].footY = 1;
        const layout = Number(row[11]);
        if (!layout) return;
        const x = bitsToFloat(Number(row[8]));
        const y = bitsToFloat(Number(row[9]));
        // 후보 하나: 가상 표 공통 값·TYPE·상태·extra·소유자·좌표·폭을 쓰고 해시 체인의 머리에 끼운다.
        const add = (sid, type, state, extra, ax, ay, width, owner, level) => {
            const bytes = pool.allocated(sid);
            bytes.fill(0);
            put(bytes, 0, 0x11010000);
            bytes[10] = type;
            bytes[11] = state;
            bytes[40] = extra;
            bytes[34] = owner;
            put(bytes, 14, floatToBits(ax));
            put(bytes, 18, floatToBits(ay));
            scene.types[type].footX = width;
            scene.types[type].footY = 1;
            put(bytes, 4, scene.hash.bucket(level, ax, ay), 2);
            scene.hash.setBucket(level, ax, ay, sid);
        };
        const ax = Math.fround(Math.min(255.5, x + (layout === 7 ? 1 : layout === 8 ? 2 : 0)));
        add(60, layout === 2 || layout === 9 ? 159 : 158, layout === 4 ? 2 : layout === 5 ? 4 : 0, layout === 3 ? 8 : 0, ax, y, layout === 7 ? 2 : 1, layout === 6 ? 8 : 1, 1);
        if (layout === 9) add(61, 158, 0, 0, x, y, 1, 2, 3);
    };
    // 외부 효과의 실제 순서만 문자열로 합친다.
    scene.eventText = () => (scene.events.length === 0 ? '-' : scene.events.join(';'));
    return scene;
}

// 예약 접두 한 행을 실행한다. 두 번의 예약 성공 여부는 행의 3·4 번 칸이고, 공통 base 는 PostPopBase 로 이어 준다. (2026-11-10 WB04-E212)
function runPrefix(scene, row) {
    let allocations = 0;
    const prefix = new RawForcefieldPostPop(scene.pool, {
        reserveRegular: () => { scene.events.push('A:40'); return Number(row[allocations++ ? 4 : 3]) !== 0; },
        addRegular: (sid, event, payload) => scene.events.push(`R:${sid}:${event}:${floatToBits(payload)}`),
    });
    prefix.prefix(SHIELD, Number(row[2]));
    const book = new SquidPostPopState();
    book.suppressed = true;
    book.depth = 1;
    const base = new SquidPostPop(scene.pool, scene.types, book);
    scene.events.push(`B:50:${row[2]}`);
    base.postPopBase(SHIELD, Number(row[2]));
    assert.equal(book.depth, 0);
}

// Regular 한 행을 실행해 반환 float 비트를 8자리 hex 로 돌려준다. (2026-11-10 WB04-E212)
function runRegular(scene, row) {
    const state = new ForcefieldRegularState();
    state.priestType = Number(row[10]);
    const regular = new RawForcefieldRegular(scene.pool, scene.hash, scene.types, state, {
        advance: (sid, delta, flags) => scene.events.push(`A:${sid}:${delta}:${flags}`),
        destroy: (sid, flags) => scene.events.push(`D:${sid}:${flags}`),
    });
    const returned = regular.handle(SHIELD, Number(row[5]), Number(row[6]), bitsToFloat(Number(row[7])));
    return floatToBits(returned).toString(16).padStart(8, '0');
}

test('보호막 수명: 원본 10.78 후처리 접두·Regular·타입 finder 1208행 전부', { skip: !rows && 'forcefieldpostpop fixture 없음' }, () => {
    assert.equal(rows.length, CASES);
    const scene = forcefieldScene();
    let count = 0;
    for (const row of rows) {
        assert.equal(row.length, 16);
        scene.prepare(row);
        let result = '-';
        if (row[1] === 'Prefix') runPrefix(scene, row);
        else result = runRegular(scene, row);
        const actual = [scene.eventText(), result, hex(scene.pool.slot(SHIELD))];
        assert.deepEqual(actual, [row[13], row[14], row[15]], `보호막 행 ${count + 1} (${row[1]})`);
        count += 1;
    }
    assert.equal(count, CASES);
});

// 보호막 슬롯의 쓰기 대여(할당 전 슬롯도 허용)이다. (2026-11-10 WB04-E212)
const physicalShield = (pool) => pool.physical(SHIELD);

test('후처리 접두 현재 계약: 원본 접두 128행만 재생(10.78)', { skip: !rows && 'forcefieldpostpop fixture 없음' }, () => {
    const scene = forcefieldScene();
    let count = 0;
    for (const row of rows.filter((candidate) => candidate[1] === 'Prefix')) {
        scene.prepare(row);
        runPrefix(scene, row);
        assert.equal(scene.eventText(), row[13]);
        assert.equal(hex(scene.pool.slot(SHIELD)), row[15]);
        count += 1;
    }
    assert.equal(count, 128);
});

test('후처리 접두 현재 계약: 첫 예약이 없는 flags 는 연결되지 않은 장치와 raw 슬롯에 접근하지 않는다', () => {
    const pool = new SidPool(CAPACITY, { server: false });
    const body = new RawForcefieldPostPop(pool, {});
    body.prefix(65000, 0);
    body.prefix(65000, 2);
});

test('후처리 접두 현재 계약: 두 예약이 모두 실패하면 생성 장치 누락을 요구하지 않는다', () => {
    const pool = new SidPool(CAPACITY, { server: false });
    const raw = physicalShield(pool);
    raw[10] = 255;
    raw[11] = 9;
    let calls = 0;
    const body = new RawForcefieldPostPop(pool, { reserveRegular: () => { calls += 1; return false; } });
    body.prefix(SHIELD, 1);
    assert.equal(calls, 2);
    assert.equal(raw[11], 9);
});

test('후처리 접두 현재 계약: 첫 예약 성공 뒤 생성 장치 누락은 앞선 콜백 쓰기를 보존하고 두 번째 예약에 도달하지 않는다', () => {
    const pool = new SidPool(CAPACITY, { server: false });
    const raw = physicalShield(pool);
    let calls = 0;
    const body = new RawForcefieldPostPop(pool, { reserveRegular: () => { calls += 1; raw[11] = 9; return true; } });
    assert.ok(throws(() => body.prefix(SHIELD, 1)));
    assert.equal(calls, 1);
    assert.equal(raw[11], 9);
});

test('후처리 접두 현재 계약: 두 번째 예약의 명시 실패는 이미 만든 사건 1 을 취소하지 않는다', () => {
    const pool = new SidPool(CAPACITY, { server: false });
    let calls = 0;
    const events = [];
    const body = new RawForcefieldPostPop(pool, {
        reserveRegular: () => { calls += 1; if (calls === 2) throw new Error('명시 두 번째 보호막 확보 중단'); return true; },
        addRegular: (sid, event, delay) => { assert.equal(sid, SHIELD); assert.equal(delay, FRAME_INTERVAL); events.push(event); },
    });
    assert.ok(throws(() => body.prefix(SHIELD, 1)));
    assert.equal(calls, 2);
    assert.deepEqual(events, [1]);
});

test('후처리 접두 현재 계약: 첫 생성 변이가 수명/TYPE/VT 를 바꿔도 두 번째 예약 앞에 객체 재검사를 넣지 않는다', () => {
    const pool = new SidPool(CAPACITY, { server: false });
    const raw = physicalShield(pool);
    const events = [];
    const body = new RawForcefieldPostPop(pool, {
        reserveRegular: () => true,
        addRegular: (sid, event, delay) => {
            assert.equal(sid, SHIELD);
            assert.equal(delay, event === 1 ? FRAME_INTERVAL : CHECK_INTERVAL);
            events.push(event);
            raw[10] = 255;
            raw[11] = 9;
            put(raw, 0, 0);
        },
    });
    body.prefix(SHIELD, 1);
    assert.deepEqual(events, [1, 2]);
    assert.equal(raw[11], 9);
});

test('후처리 접두 현재 계약: 10.78 가상 진입은 같은 가상 표면 잘못된 TYPE/free 도 앞단에서 거부하지 않고 공통 부모로 간다', () => {
    const pool = new SidPool(CAPACITY, { server: false });
    const types = emptyTypes();
    const book = new SquidPostPopState();
    book.suppressed = true;
    const base = new SquidPostPop(pool, types, book);
    const raw = physicalShield(pool);
    put(raw, 0, PATCH_FORCEFIELD_VTABLE);
    raw[10] = 255;
    raw[11] = 9;
    const empty = new RawForcefieldPostPop(pool, {});
    base.setBody('forcefield', empty);
    assert.ok(base.handlesForcefield(SHIELD));
    book.depth += 1;
    base.postPopVirtual(SHIELD, 0);
    assert.equal(book.depth, 0);
    assert.ok(throws(() => empty.validate(SHIELD)));
    let calls = 0;
    const ready = new RawForcefieldPostPop(pool, { reserveRegular: () => { calls += 1; return false; } });
    base.setBody('forcefield', ready);
    book.depth += 1;
    base.postPopVirtual(SHIELD, 1);
    assert.equal(calls, 2);
    assert.equal(book.depth, 0);
    assert.equal(raw[11], 9);
});

test('후처리 접두 현재 계약: 선택된 첫 예약이 없으면 공통 깊이를 줄이거나 슬롯을 바꾸지 않고 중단한다', () => {
    const pool = new SidPool(CAPACITY, { server: false });
    const types = emptyTypes();
    const book = new SquidPostPopState();
    book.suppressed = true;
    const base = new SquidPostPop(pool, types, book);
    const raw = physicalShield(pool);
    put(raw, 0, PATCH_FORCEFIELD_VTABLE);
    raw[10] = 74;
    raw[11] = 0;
    const before = hex(raw);
    base.setBody('forcefield', new RawForcefieldPostPop(pool, {}));
    book.depth += 1;
    assert.ok(throws(() => base.postPopVirtual(SHIELD, 1)));
    assert.equal(hex(raw), before);
    assert.equal(book.depth, 1);
});

// Regular 현재 계약 장면: SID 5..50 을 할당하고 보호막 VT 와 raw 좌표만 가진 슬롯이다(공간에 Pop 하지 않는다). (2026-11-10 WB04-E212)
function regularScene() {
    const pool = new SidPool(CAPACITY, { server: false });
    for (let sid = 5; sid <= SHIELD; sid += 1) assert.equal(pool.allocate(2), sid);
    const raw = pool.allocated(SHIELD);
    raw.fill(0);
    put(raw, 0, PATCH_FORCEFIELD_VTABLE);
    raw[10] = FORCEFIELD_TYPE;
    put(raw, 14, floatToBits(20.75));
    put(raw, 18, floatToBits(21.9));
    return { pool, hash: new SquidHash(), types: emptyTypes(), state: new ForcefieldRegularState() };
}

test('Regular 현재 계약: 모르는 사건은 raw 와 선택하지 않은 장치에 접근하지 않는다', () => {
    const scene = regularScene();
    const body = new RawForcefieldRegular(scene.pool, scene.hash, scene.types, scene.state, {});
    assert.equal(body.handle(65535, 99, 0xffffffff, NaN), 0);
    assert.ok(throws(() => body.handle(SHIELD, 1, 0, 0)));
});

test('Regular 현재 계약: 진행만 하는 사건 1 과 콜백이 처리기를 반납해도 마친다', () => {
    const scene = regularScene();
    let body = null;
    let calls = 0;
    body = new RawForcefieldRegular(scene.pool, scene.hash, scene.types, scene.state, {
        advance: (sid, delta, flags) => { assert.deepEqual([sid, delta, flags], [65535, 1, 0]); calls += 1; body = null; },
    });
    assert.equal(body.handle(65535, 1, 0, 0), FRAME_INTERVAL);
    assert.equal(body, null);
    assert.equal(calls, 1);
});

test('Regular 현재 계약: 장기 DWORD 와 raw float 이 타입별 finder 로 그대로 전달된다', () => {
    const scene = regularScene();
    let type = 158;
    let calls = 0;
    scene.state.sharedPriestType = () => type;
    const raw = scene.pool.allocated(SHIELD);
    raw[10] = 74;
    raw[11] = 9;
    put(raw, 0, 0xdeadbeef);
    const before = hex(raw);
    const body = new RawForcefieldRegular(scene.pool, scene.hash, scene.types, scene.state, {
        findTypeAt: (x, y, current) => {
            assert.equal(floatToBits(x), floatToBits(20.75));
            assert.equal(floatToBits(y), floatToBits(21.9));
            assert.equal(current, type);
            calls += 1;
            return 60;
        },
    });
    assert.equal(body.handle(SHIELD, 2, 0, 0), CHECK_INTERVAL);
    type = 0x1009e;
    assert.equal(body.handle(SHIELD, 2, 0, 0), CHECK_INTERVAL);
    assert.equal(calls, 2);
    assert.equal(hex(raw), before);
});

test('Regular 현재 계약: 삭제 장치 누락은 이미 끝난 검색 효과를 보존한다', () => {
    const scene = regularScene();
    let calls = 0;
    const body = new RawForcefieldRegular(scene.pool, scene.hash, scene.types, scene.state, {
        findTypeAt: () => { calls += 1; put(scene.pool.allocated(SHIELD), 26, 1234); return 0; },
    });
    assert.ok(throws(() => body.handle(SHIELD, 2, 0, 0)));
    assert.equal(calls, 1);
    assert.equal(get(scene.pool.slot(SHIELD), 26), 1234);
});

test('Regular 현재 계약: 삭제 콜백이 처리기를 반납해도 -1 을 돌려준다', () => {
    const scene = regularScene();
    let body = null;
    let calls = 0;
    body = new RawForcefieldRegular(scene.pool, scene.hash, scene.types, scene.state, {
        findTypeAt: () => 0,
        destroy: (sid, flags) => { assert.deepEqual([sid, flags], [SHIELD, 0]); calls += 1; body = null; },
    });
    assert.equal(body.handle(SHIELD, 2, 0, 0), -1);
    assert.equal(body, null);
    assert.equal(calls, 1);
});

test('Regular 현재 계약: 진단은 현재 checking 을 읽고 처리기 반납 뒤에도 0 으로 복귀한다', () => {
    const scene = regularScene();
    let checking = 0;
    let calls = 0;
    scene.state.checking = () => checking;
    let body = null;
    body = new RawForcefieldRegular(scene.pool, scene.hash, scene.types, scene.state, {
        assertion: (expression, file, line) => { assert.deepEqual([expression, file, line], ['0', '.\\Dais.cpp', 0x479]); calls += 1; body = null; },
    });
    assert.equal(body.handle(65535, 99, 0, 0), 0);
    assert.equal(calls, 0);
    checking = 0x80000000;
    assert.equal(body.handle(65535, 99, 0, 0), 0);
    assert.equal(body, null);
    assert.equal(calls, 1);
});

test('Regular 현재 계약: 선택된 진단 장치가 없으면 raw 를 바꾸지 않고 실패한다', () => {
    const scene = regularScene();
    scene.state.checking = () => 1;
    const body = new RawForcefieldRegular(scene.pool, scene.hash, scene.types, scene.state, {});
    const before = hex(scene.pool.slot(SHIELD));
    assert.ok(throws(() => body.handle(SHIELD, 99, 0, 0)));
    assert.equal(hex(scene.pool.slot(SHIELD)), before);
});

test('보호막 수명: 다른 풀 연결 거부와 해제, 진단 장치 누락, 명시 Validate 의 무변경 거부', { skip: !rows && 'forcefieldpostpop fixture 없음' }, () => {
    const scene = forcefieldScene();
    scene.prepare(rows[0]);
    const spots = new Uint8Array(65536);
    scene.pool.allocated(SHIELD)[11] = 4;
    const book = new SquidPostPopState();
    const base = new SquidPostPop(scene.pool, scene.types, book);
    const pop = new SquidPop(scene.pool, scene.hash, spots, { postPop: base });
    let calls = 0;
    const before = hex(scene.pool.slot(SHIELD));
    // 10.78 무변경 거부는 명시 validate 위치의 계약이다.
    assert.ok(throws(() => base.validate(SHIELD, 1)));
    assert.equal(hex(scene.pool.slot(SHIELD)), before);
    assert.equal(book.depth, 0);
    const prefix = new RawForcefieldPostPop(scene.pool, { reserveRegular: () => { calls += 1; return false; }, addRegular: () => assert.fail('예약 실패인데 Regular 추가') });
    const other = new SidPool(CAPACITY, { server: false });
    const otherBase = new SquidPostPop(other, scene.types, book);
    assert.ok(throws(() => otherBase.setBody('forcefield', prefix)));
    base.setBody('forcefield', prefix);
    assert.equal(pop.pop(SHIELD, scene.types[FORCEFIELD_TYPE], 1, 1, 20.75, 21.9), RAW_POP_RESULT.registered);
    assert.equal(calls, 2);
    assert.equal(book.depth, 0);
    const state = new ForcefieldRegularState();
    state.priestType = 158;
    state.checking = () => 1;
    const regular = new RawForcefieldRegular(scene.pool, scene.hash, scene.types, state, {
        advance: () => assert.fail('사건 99 는 진행하지 않는다'),
        destroy: () => assert.fail('사건 99 는 삭제하지 않는다'),
    });
    assert.ok(throws(() => regular.handle(SHIELD, 99, 0, 0)));
    state.checking = () => 0;
    assert.equal(regular.handle(SHIELD, 99, 0, 0), 0);
    base.setBody('forcefield', null);
    assert.ok(!base.handlesForcefield(SHIELD));
    assert.ok(!SquidPop.supports(PATCH_FORCEFIELD_VTABLE, 0));
});

test('보호막 후처리 validate: 잘못된 SID/free/TYPE/가상 표를 거부하고 읽기 전용이다', () => {
    const pool = new SidPool(CAPACITY, { server: false });
    for (let sid = 5; sid <= SHIELD; sid += 1) pool.allocate(2);
    const raw = pool.allocated(SHIELD);
    raw.fill(0);
    put(raw, 0, PATCH_FORCEFIELD_VTABLE);
    raw[10] = FORCEFIELD_TYPE;
    const body = new RawForcefieldPostPop(pool, {});
    body.validate(SHIELD);
    const before = hex(raw);
    assert.ok(throws(() => body.validate(4)));
    raw[11] = 1;
    assert.ok(throws(() => body.validate(SHIELD)));
    raw[11] = 8;
    assert.ok(throws(() => body.validate(SHIELD)));
    raw[11] = 0;
    raw[10] = 158;
    assert.ok(throws(() => body.validate(SHIELD)));
    raw[10] = FORCEFIELD_TYPE;
    put(raw, 0, 0x11010000);
    assert.ok(throws(() => body.validate(SHIELD)));
    put(raw, 0, PATCH_FORCEFIELD_VTABLE);
    assert.equal(hex(raw), before);
});

test('보호막 수명: 사건 2 의 기본 finder 는 비유한/범위 밖 좌표와 타입 표 크기를 거부한다', () => {
    const scene = regularScene();
    const raw = scene.pool.allocated(SHIELD);
    put(raw, 14, 0x7fc00000);
    const body = new RawForcefieldRegular(scene.pool, scene.hash, scene.types, scene.state, { destroy: () => assert.fail('좌표 거부에서는 삭제하지 않는다') });
    assert.ok(throws(() => body.handle(SHIELD, 2, 0, 0)));
    put(raw, 14, floatToBits(3e9));
    assert.ok(throws(() => body.handle(SHIELD, 2, 0, 0)));
    assert.ok(throws(() => new RawForcefieldRegular(scene.pool, scene.hash, scene.types.slice(0, 171), scene.state, {})));
    assert.ok(throws(() => new RawForcefieldPostPop({ edition: 'cd1072' }, {})));
});

test('보호막 훅 조립: 같은 풀의 Process 호스트·프레임 진행기에만 연결하고 효과를 그대로 전달한다', () => {
    const pool = new SidPool(CAPACITY, { server: false });
    const added = [];
    const host = { pool, addRegular: (sid, event, payload) => added.push([sid, event, payload]) };
    const hooks = makeForcefieldPostPopProcessHooks(pool, host, () => true);
    hooks.addRegular(SHIELD, 1, FRAME_INTERVAL);
    assert.deepEqual(added, [[SHIELD, 1, FRAME_INTERVAL]]);
    assert.ok(throws(() => makeForcefieldPostPopProcessHooks(pool, { pool: new SidPool(CAPACITY, { server: false }), addRegular() {} }, () => true)));
    assert.ok(throws(() => makeForcefieldPostPopProcessHooks(pool, host, null)));
    const advanced = [];
    const advance = { pool, advance: (sid, delta, flags) => advanced.push([sid, delta, flags]) };
    const destroy = () => {};
    const frameHooks = makeForcefieldFrameHooks(pool, advance, { destroy });
    frameHooks.advance(SHIELD, 1, 0);
    assert.deepEqual(advanced, [[SHIELD, 1, 0]]);
    assert.equal(frameHooks.destroy, destroy);
    assert.ok(throws(() => makeForcefieldFrameHooks(pool, { pool: new SidPool(CAPACITY, { server: false }), advance() {} })));
});

// 사건 2 의 기본 finder 에서 사제 후보를 (x, y) 에 두는 장면 보조이다. 후보 SID 49 는 해시 1단계 버킷의 머리에 끼운다. (2026-11-10 WB04-E212)
function plantPriest(scene, x, y) {
    const candidate = scene.pool.allocated(49);
    candidate.fill(0);
    put(candidate, 0, 0x11010000);
    candidate[10] = 158;
    put(candidate, 14, floatToBits(x));
    put(candidate, 18, floatToBits(y));
    put(candidate, 4, scene.hash.bucket(1, x, y), 2);
    scene.hash.setBucket(1, x, y, 49);
    scene.types[158].footX = 1;
    scene.types[158].footY = 1;
    scene.types[FORCEFIELD_TYPE].footX = 1;
    scene.types[FORCEFIELD_TYPE].footY = 1;
}

test('보호막 수명: 사건 2 의 기본 finder 는 음수 소수 좌표를 0 방향으로 절삭한다(-0.5 → 0칸이며 -1 로 내리면 전체 행으로 번진다)', () => {
    const scene = regularScene();
    const raw = scene.pool.allocated(SHIELD);
    put(raw, 14, floatToBits(-0.5));
    put(raw, 18, floatToBits(21.9));
    // 0칸·21칸 한 점의 사제는 잡히고, 같은 행의 먼 사제는 잡히지 않는다. 내림(-1)이면 오른쪽 경계 -1 이 보드 끝으로 해석돼 먼 사제도 잡힌다.
    plantPriest(scene, 0.25, 21.5);
    const destroyed = [];
    const body = new RawForcefieldRegular(scene.pool, scene.hash, scene.types, scene.state, { destroy: (sid, flags) => destroyed.push([sid, flags]) });
    assert.equal(body.handle(SHIELD, 2, 0, 0), CHECK_INTERVAL);
    assert.deepEqual(destroyed, []);
    const far = regularScene();
    put(far.pool.allocated(SHIELD), 14, floatToBits(-0.5));
    put(far.pool.allocated(SHIELD), 18, floatToBits(21.9));
    plantPriest(far, 100.25, 21.5);
    const farDestroyed = [];
    const farBody = new RawForcefieldRegular(far.pool, far.hash, far.types, far.state, { destroy: (sid, flags) => farDestroyed.push([sid, flags]) });
    assert.equal(farBody.handle(SHIELD, 2, 0, 0), -1);
    assert.deepEqual(farDestroyed, [[SHIELD, 0]]);
});
