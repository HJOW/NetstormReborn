// 사제 후반 후처리 raw-priest-post-pop-tail.js(RawPriestPostPopTail·makePriestPostPopTailShieldHooks)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: priestpostpoptail-x86.tsv 10.78(originals) 2048행(사제 postPop 전체 wrapper: 접두→Carrier→현재 extra→이동 불가 조회→낙하/보호막 호출 기록·슬롯 50바이트·목록·소유자 상태)과
//    priestpop-x86.tsv 10.78 1280행(실제 SquidPop/공통 postPop 분배를 거친 사제 Pop: 효과 기록·슬롯·해시 머리·목록·소유자 상태·깊이). CD 10.72·10.37 행은 웹 범위(10.78) 밖이다.
//  - 계약: PriestPostPopTail_RejectsInvalidBindings·_SkipsUnneededInvalidCoordinateAndFrameReads·PriestPostPopCurrent_PatchVirtualEntryDispatchesByVtableOnly·_PatchTailRequiresOnlyReachedEffects·
//    PriestPop_RequiresExplicitWholeHandlerBeforeMutation 의 10.78 부분. 회복 Form/Kernel·낙하·프레임 단계 이동을 통째로 잇는 통합 장면(PriestPostPopTail_ComposesProcessBooks…, PriestPop_ComposesFall…)은
//    ProcessHost 장면과 RawPriestFall 이식 단위의 몫이다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E64: 사제 후반 후처리 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { GameRandom } from '../www/core/random.js';
import { ScrambledSpStore } from '../www/core/sp-store.js';
import { SquidReward, SquidRewardState } from '../www/core/squid-reward.js';
import { SquidPostPop } from '../www/core/squid-postpop.js';
import { SquidPostPopState } from '../www/core/squid-postpop-state.js';
import { SquidPop, RAW_POP_RESULT } from '../www/core/squid-pop.js';
import { FrameCodeTable } from '../www/assets/terrain-tiles.js';
import { PRIEST_TYPE, PRIEST_VTABLE } from '../www/core/raw-priest-owner.js';
import { RawPriestState } from '../www/core/raw-priest-state.js';
import { PriestPostPopState, RawPriestPostPop } from '../www/core/raw-priest-post-pop.js';
import { RawCarrierPostPop, CarrierPostPopState, makeCarrierPostPopHooks } from '../www/core/raw-carrier-post-pop.js';
import { RawPriestPostPopTail } from '../www/core/raw-priest-post-pop-tail.js';

// 원본 관측 10.78 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E64)
const FIXTURES = path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures');
const tailRows = readRows(path.join(FIXTURES, 'priestpostpoptail-x86.tsv'))?.filter((row) => row[0] === 'originals') ?? null;
const popRows = readRows(path.join(FIXTURES, 'priestpop-x86.tsv'))?.filter((row) => row[0] === 'originals') ?? null;

// 관찰 SID, 판본당 Tail/Pop 행 수, 공통 가상 표이다. (2026-11-08 WB04-E64)
const SOURCE = 50;
const TAIL_ROWS = 2048;
const POP_ROWS = 1280;
const COMMON_VTABLE = 0x11010000;

// 예외가 나는지 본다. (2026-11-08 WB04-E64)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 바이트 배열을 소문자 hex 로 만든다. (2026-11-08 WB04-E64)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 188칸 타입 표이다. (2026-11-08 WB04-E64)
const tailTypes = () => Array.from({ length: 188 }, () => ({ cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, maxHitPoints: 0 }));

// 같은 클라이언트 풀의 입력 SID 50 까지 확보하고 고정 크기 표를 만든 장면이다. (2026-11-08 WB04-E64)
function tailScene() {
    const pool = new SidPool(24000, { server: false });
    for (let id = 5; id <= SOURCE; id += 1) assert.equal(pool.allocate(2), id);
    const types = tailTypes();
    const scene = { pool, store: new ScrambledSpStore(new GameRandom(), () => 1), book: new SquidPostPopState(), hpMode: new SquidRewardState(), priests: new PriestPostPopState(), types, hp: null, hash: new SquidHash(),
        frames: types.map(() => new FrameCodeTable([])), spots: new Uint8Array(65536), events: [], found: false, allocated: false, change: 0 };
    scene.makeHp = () => { scene.hp = new SquidReward(pool, types, scene.book, scene.hpMode, scene.store); };
    // Regular 검색/확보/부착 경계의 인자와 목록 등록 시점을 관찰한다.
    scene.prefixHooks = () => ({
        findRegular: (sid, event) => { scene.events.push(`F:${sid}:${event}:${scene.priests.priests.count}`); return scene.found; },
        reserveRegular: () => { scene.events.push('A:40'); return scene.allocated; },
        addRegular: (sid, event, payload) => scene.events.push(`R:${sid}:${event}:${floatToBits(payload)}`)
    });
    // Carrier 경계의 명시 입력 변이를 공급한다. 이후 실제 상태 조회가 현재 값을 읽는다.
    scene.carrier = (sid, flags) => {
        scene.events.push(`C:${sid}:${flags}`);
        const raw = pool.allocated(sid);
        if (scene.change === 1) raw[40] = 1;
        else if (scene.change === 2 || scene.change === 3) {
            raw[40] = scene.change === 2 ? 32 : 0;
            put(raw, 26, 150);
            put(raw, 14, floatToBits(30.75));
            put(raw, 18, floatToBits(31.9));
            scene.spots[31 * 256 + 30] = scene.change === 2 ? 0 : 6;
            scene.spots[32 * 256 + 31] = 6;
        }
    };
    // 낙하/보호막 경계 호출 순서를 기록한다. 낙하 후 extra 변경은 입력 변이이다.
    scene.tailHooks = () => ({
        fall: (sid) => { scene.events.push(`D:${sid}`); if (scene.change === 4) pool.allocated(sid)[40] = 1; },
        ensureShield: (sid) => scene.events.push(`S:${sid}`),
        clearShield: (sid) => scene.events.push(`X:${sid}`)
    });
    scene.eventText = () => (scene.events.length === 0 ? '-' : scene.events.join(';'));
    scene.list = () => `${scene.priests.priests.count}:${scene.priests.priests.entries.join(',')}`;
    scene.owners = () => scene.priests.ownerState.join(',');
    return scene;
}

test('PriestPostPopTail: 원본 10.78 전체 wrapper 2048행의 호출 기록·슬롯·목록·소유자 상태가 같다', { skip: !tailRows && '원본 fixture 없음' }, () => {
    const scene = tailScene();
    let count = 0;
    // 서로 다른 분기/효과 입력마다 실제 모듈을 새로 연결한다.
    for (const row of tailRows) {
        assert.equal(row.length, 14);
        scene.events = [];
        scene.found = Number(row[7]) !== 0;
        scene.allocated = Number(row[8]) !== 0;
        scene.change = Number(row[9]);
        scene.types[PRIEST_TYPE].maxHitPoints = 200;
        scene.types[PRIEST_TYPE].flags1 = 0x69012;
        scene.types[PRIEST_TYPE].flags2 = 0x210000;
        scene.makeHp();
        scene.frames[PRIEST_TYPE] = new FrameCodeTable([{ side: Number(row[6]), variant: 80, flags: 0 }]);
        scene.spots.fill(0);
        scene.spots[21 * 256 + 20] = Number(row[4]);
        scene.spots[22 * 256 + 21] = Number(row[5]);
        scene.priests.priests.entries = [60, 50, 70, 80];
        scene.priests.priests.count = 0;
        for (let index = 0; index < scene.priests.ownerState.length; index += 1) scene.priests.ownerState[index] = 0xabc000 + index;
        const raw = scene.pool.allocated(SOURCE);
        raw.fill(0);
        put(raw, 0, PRIEST_VTABLE);
        raw[10] = PRIEST_TYPE;
        raw[34] = 1;
        raw[40] = Number(row[2]);
        put(raw, 12, 1, 2);
        put(raw, 14, floatToBits(20.75));
        put(raw, 18, floatToBits(21.9));
        put(raw, 26, Number(row[3]));
        const prefix = new RawPriestPostPop(scene.pool, scene.hp, scene.priests, scene.prefixHooks());
        const carrier = new RawCarrierPostPop(scene.pool, scene.types, new CarrierPostPopState(), { carrierCheck: () => {}, claimGround: () => {}, validateBase: () => {}, base: (sid, flags) => scene.carrier(sid, flags) });
        const state = new RawPriestState(scene.pool, scene.hp, scene.types, scene.spots);
        new RawPriestPostPopTail(scene.pool, prefix, carrier, state, scene.frames, scene.spots, scene.tailHooks()).postPop(SOURCE, Number(row[1]));
        assert.equal(scene.eventText(), row[10], `행 ${count}`);
        assert.equal(hex(scene.pool.slot(SOURCE)), row[11], `행 ${count}`);
        assert.equal(scene.list(), row[12], `행 ${count}`);
        assert.equal(scene.owners(), row[13], `행 ${count}`);
        count += 1;
    }
    assert.equal(count, TAIL_ROWS);
    assert.ok(!scene.store.initialized);
});

// 사제 Pop 장면 입력을 준비하는 보조이다: 새로운 void 사제와 빈 공간/장부/목록에서 시작한다. (2026-11-08 WB04-E64)
function popScene() {
    const scene = tailScene();
    scene.types[PRIEST_TYPE].maxHitPoints = 200;
    scene.types[PRIEST_TYPE].cost = 123;
    scene.types[PRIEST_TYPE].flags1 = 0x69012;
    scene.types[PRIEST_TYPE].flags2 = 0x210000;
    scene.types[PRIEST_TYPE].footX = 1;
    scene.types[PRIEST_TYPE].footY = 1;
    scene.preparePop = (row) => {
        scene.events = [];
        scene.hash.reset();
        scene.spots.fill(0);
        scene.spots[21 * 256 + 20] = Number(row[4]);
        scene.spots[22 * 256 + 21] = Number(row[4]);
        scene.book = new SquidPostPopState();
        scene.book.suppressed = true;
        scene.priests.priests.entries = [60, 50, 70, 80];
        scene.priests.priests.count = 0;
        for (let index = 0; index < scene.priests.ownerState.length; index += 1) scene.priests.ownerState[index] = 0xabc000 + index;
        scene.frames[PRIEST_TYPE] = new FrameCodeTable([{ side: Number(row[5]), variant: 80, flags: 0 }]);
        const raw = scene.pool.allocated(SOURCE);
        raw.fill(0);
        put(raw, 0, PRIEST_VTABLE);
        raw[10] = PRIEST_TYPE;
        raw[11] = 4;
        raw[34] = 1;
        raw[40] = Number(row[1]);
        put(raw, 12, 1, 2);
        put(raw, 26, Number(row[3]));
    };
    return scene;
}

test('PriestPop: 원본 10.78 사제 Pop 1280행(공통 Pop/firstPop/Activate→사제 wrapper)의 기록·슬롯·해시·목록·소유자·깊이가 같다', { skip: !popRows && '원본 fixture 없음' }, () => {
    const scene = popScene();
    let count = 0;
    // 원본에서 얻은 목표 flags/슬롯/지면/목록을 계산하지 않고 각 입력을 실제 모듈에 연결한다.
    for (const row of popRows) {
        assert.equal(row.length, 13);
        scene.preparePop(row);
        scene.makeHp();
        const base = new SquidPostPop(scene.pool, scene.types, scene.book);
        const state = new RawPriestState(scene.pool, scene.hp, scene.types, scene.spots);
        // 접두가 실제 목록 삽입을 마친 시점의 검색 호출을 관찰한다. 원본 new 실패 입력은 예약/HP 조회의 분기를 보존한다.
        const prefix = new RawPriestPostPop(scene.pool, scene.hp, scene.priests, {
            findRegular: (sid, event) => { scene.events.push(`F:${sid}:${event}:${scene.priests.priests.count}`); return Number(row[6]) !== 0; },
            reserveRegular: () => { scene.events.push('A:40'); return Number(row[7]) !== 0; },
            addRegular: (sid, event, payload) => scene.events.push(`R:${sid}:${event}:${floatToBits(payload)}`)
        });
        // Carrier 의 실제 flags 를 기록하고 base 깊이 감소는 실제 공통 몸체로 실행한다.
        const effects = makeCarrierPostPopHooks(scene.pool, base, () => { throw new Error('carrier check'); }, () => { throw new Error('claim ground'); });
        effects.base = (sid, flags) => { scene.events.push(`C:${sid}:${flags}`); base.postPopBase(sid, flags); };
        const carrier = new RawCarrierPostPop(scene.pool, scene.types, new CarrierPostPopState(), effects);
        const tail = new RawPriestPostPopTail(scene.pool, prefix, carrier, state, scene.frames, scene.spots, {
            fall: (sid) => scene.events.push(`D:${sid}`),
            ensureShield: (sid) => scene.events.push(`S:${sid}`),
            clearShield: (sid) => scene.events.push(`X:${sid}`)
        });
        base.setBody('priest', tail);
        const pop = new SquidPop(scene.pool, scene.hash, scene.spots, { postPop: base });
        assert.equal(pop.pop(SOURCE, scene.types[PRIEST_TYPE], 1, 1, 20.75, 21.9, Number(row[2])), RAW_POP_RESULT.registered);
        assert.equal(scene.eventText(), row[8], `행 ${count}`);
        assert.equal(hex(scene.pool.slot(SOURCE)), row[9], `행 ${count}`);
        assert.equal(scene.hash.bucket(1, 20.75, 21.9), Number(row[10]), `행 ${count}`);
        assert.equal(scene.list(), row[11], `행 ${count}`);
        assert.equal(scene.owners(), row[12], `행 ${count}`);
        assert.equal(scene.book.depth, 0);
        count += 1;
    }
    assert.equal(count, POP_ROWS);
});

// 사제 접두/Carrier/상태/후반을 한 장면에 조립하는 보조이다. (2026-11-08 WB04-E64)
function assemble(scene, tailHooks = scene.tailHooks()) {
    scene.makeHp();
    const prefix = new RawPriestPostPop(scene.pool, scene.hp, scene.priests, scene.prefixHooks());
    const base = new SquidPostPop(scene.pool, scene.types, scene.book);
    const carrier = new RawCarrierPostPop(scene.pool, scene.types, new CarrierPostPopState(), makeCarrierPostPopHooks(scene.pool, base, () => {}, () => {}));
    const state = new RawPriestState(scene.pool, scene.hp, scene.types, scene.spots);
    return { prefix, base, carrier, state, tail: new RawPriestPostPopTail(scene.pool, prefix, carrier, state, scene.frames, scene.spots, tailHooks) };
}

// 사제 VT/TYPE158·소유자 1·원래 소유자 1·좌표(20.75, 21.9)·HP 600 의 관찰 슬롯을 만든 현재 장면이다. (2026-11-08 WB04-E64)
function currentScene() {
    const scene = tailScene();
    for (const [number, value] of [[PRIEST_TYPE, 200], [82, 600]]) {
        scene.types[number].maxHitPoints = value;
        scene.types[number].flags1 = 0x69012;
        scene.types[number].flags2 = 0x210000;
    }
    scene.allocated = true;
    for (const [index] of scene.frames.entries()) scene.frames[index] = new FrameCodeTable([{ side: 0x41, variant: 80, flags: 0 }]);
    scene.priests.priests.entries = [0, 0, 0, 0];
    for (let index = 0; index < scene.priests.ownerState.length; index += 1) scene.priests.ownerState[index] = 0xabc000 + index;
    const raw = scene.pool.physical(SOURCE);
    raw.fill(0);
    put(raw, 0, PRIEST_VTABLE);
    raw[10] = PRIEST_TYPE;
    raw[34] = 1;
    put(raw, 12, 1, 2);
    put(raw, 14, floatToBits(20.75));
    put(raw, 18, floatToBits(21.9));
    put(raw, 26, 600);
    return scene;
}

test('PriestPostPopTail: 풀/표 크기 오류는 첫 효과 전에 거부하고 PATCH 구성은 도달 전 효과를 요구하지 않는다', () => {
    const scene = currentScene();
    scene.makeHp();
    const prefix = new RawPriestPostPop(scene.pool, scene.hp, scene.priests, scene.prefixHooks());
    const base = new SquidPostPop(scene.pool, scene.types, scene.book);
    const carrier = new RawCarrierPostPop(scene.pool, scene.types, new CarrierPostPopState(), makeCarrierPostPopHooks(scene.pool, base, () => {}, () => {}));
    const state = new RawPriestState(scene.pool, scene.hp, scene.types, scene.spots);
    const deferred = new RawPriestPostPopTail(scene.pool, prefix, carrier, state, scene.frames, scene.spots, {});
    assert.equal(deferred.pool, scene.pool);
    const other = new SidPool(24000, { server: false });
    assert.ok(throws(() => new RawPriestPostPopTail(other, prefix, carrier, state, scene.frames, scene.spots, scene.tailHooks())));
    assert.ok(throws(() => new RawPriestPostPopTail(scene.pool, prefix, carrier, state, [], scene.spots, scene.tailHooks())));
    assert.ok(throws(() => new RawPriestPostPopTail(scene.pool, prefix, carrier, state, scene.frames, new Uint8Array(0), scene.tailHooks())));
    assert.ok(scene.events.length === 0 && scene.priests.priests.count === 0);
});

test('PriestPostPopTail: 낙하 flag 는 지면/프레임 조회를 생략하고 Carrier 뒤 extra 차단을 먼저 적용한다', () => {
    const scene = currentScene();
    const { tail } = assemble(scene);
    const raw = scene.pool.physical(SOURCE);
    raw[40] = 32;
    put(raw, 14, 0x7fc12345);
    put(raw, 36, 0xffffffff);
    tail.postPop(SOURCE, 0x800);
    assert.equal(scene.eventText(), 'S:50');
    scene.events = [];
    assert.ok(throws(() => tail.postPop(SOURCE, 0)));
    assert.equal(scene.events.length, 0);
    raw[40] = 8;
    tail.postPop(SOURCE, 0);
    assert.equal(scene.events.length, 0);
});

test('PriestPostPopCurrent: PATCH 가상 분배는 0x50f210 만으로 사제 wrapper 에 들어가고 명시 validate 는 TYPE158 계약을 유지한다', () => {
    const scene = currentScene();
    scene.pool.physical(SOURCE)[10] = 82;
    scene.spots[22 * 256 + 21] = 6;
    const { base, tail } = assemble(scene);
    base.setBody('priest', tail);
    assert.ok(throws(() => base.validate(SOURCE, 1)) && scene.events.length === 0);
    scene.book.localOwner = 1;
    scene.book.depth = 1;
    base.postPopVirtual(SOURCE, 1);
    assert.equal(scene.eventText(), `F:50:602:1;A:40;R:50:602:${floatToBits(2.75)};X:50`);
    assert.ok(scene.book.depth === 0 && scene.book.globalCounts()[82] === 1 && scene.priests.priests.count === 1);
});

test('PriestPostPopCurrent: 후반은 효과 훅 없이 구성 가능하고 도달한 보호막/낙하만 그 시점에 거부하며 앞선 부분 효과를 보존한다', () => {
    const scene = currentScene();
    const { tail: bare } = assemble(scene, {});
    const raw = scene.pool.physical(SOURCE);
    raw[40] = 8;
    scene.book.depth = 1;
    bare.postPop(SOURCE, 0);
    assert.ok(scene.book.depth === 0 && scene.events.length === 0 && scene.priests.ownerState[1] === 0xabc001);
    raw[40] = 0;
    scene.spots[22 * 256 + 21] = 6;
    scene.book.depth = 1;
    assert.ok(throws(() => bare.postPop(SOURCE, 1)) && scene.book.depth === 0 && scene.priests.priests.count === 1);
    assert.equal(scene.eventText(), `F:50:602:1;A:40;R:50:602:${floatToBits(8.25)}`);
    raw[40] = 0x20;
    scene.events = [];
    scene.book.depth = 1;
    assert.ok(throws(() => bare.postPop(SOURCE, 0x800)) && scene.book.depth === 0 && scene.events.length === 0);
    const noFall = { ...scene.tailHooks(), fall: undefined };
    const { tail: withoutFall } = assemble(scene, noFall);
    scene.book.depth = 1;
    withoutFall.postPop(SOURCE, 0);
    assert.ok(scene.eventText() === 'S:50' && scene.book.depth === 0);
    scene.spots[22 * 256 + 21] = 0;
    scene.events = [];
    scene.book.depth = 1;
    assert.ok(throws(() => withoutFall.postPop(SOURCE, 0)) && scene.events.length === 0 && scene.book.depth === 0);
});

test('PriestPop: 명시 사전 검사는 무변경으로 거부하고 PATCH 실제 Pop 은 signed 초과 목록의 등록만 생략하며 공간/후처리를 계속한다', { skip: !popRows && '원본 fixture 없음' }, () => {
    const scene = popScene();
    scene.preparePop(popRows[0]);
    scene.makeHp();
    const base = new SquidPostPop(scene.pool, scene.types, scene.book);
    const state = new RawPriestState(scene.pool, scene.hp, scene.types, scene.spots);
    const prefix = new RawPriestPostPop(scene.pool, scene.hp, scene.priests, { findRegular: () => true, reserveRegular: () => false, addRegular: () => { throw new Error('호출 금지'); } });
    const carrier = new RawCarrierPostPop(scene.pool, scene.types, new CarrierPostPopState(), makeCarrierPostPopHooks(scene.pool, base, () => {}, () => {}));
    const tail = new RawPriestPostPopTail(scene.pool, prefix, carrier, state, scene.frames, scene.spots, { fall: () => {}, ensureShield: () => {}, clearShield: () => {} });
    const before = hex(scene.pool.slot(SOURCE));
    assert.ok(throws(() => base.validate(SOURCE, 1)) && hex(scene.pool.slot(SOURCE)) === before);
    const otherPool = new SidPool(24000, { server: false });
    assert.ok(throws(() => new SquidPostPop(otherPool, scene.types, scene.book).setBody('priest', tail)));
    base.setBody('priest', tail);
    assert.ok(base.handlesPriest(SOURCE));
    scene.priests.priests.count = 5;
    assert.ok(throws(() => base.validate(SOURCE, 1)) && hex(scene.pool.slot(SOURCE)) === before && scene.hash.bucket(1, 20.75, 21.9) === 0);
    const pop = new SquidPop(scene.pool, scene.hash, scene.spots, { postPop: base });
    assert.equal(pop.pop(SOURCE, scene.types[PRIEST_TYPE], 1, 1, 20.75, 21.9), RAW_POP_RESULT.registered);
    assert.ok(scene.hash.bucket(1, 20.75, 21.9) === SOURCE && scene.priests.priests.count === 5 && scene.book.depth === 0);
    assert.deepEqual(scene.priests.priests.entries, [60, 50, 70, 80]);
    scene.priests.priests.count = 0;
    base.setBody('priest', null);
    assert.ok(!base.handlesPriest(SOURCE) && !SquidPop.supports(PRIEST_VTABLE, 0));
    assert.equal(get(scene.pool.slot(SOURCE), 12, 2) >= 0, true);
    assert.equal(COMMON_VTABLE, 0x11010000);
});
