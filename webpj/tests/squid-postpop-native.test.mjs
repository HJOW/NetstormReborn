// SquidPostPop 의 기준선 뒤 장기 Client(Native) 진입을 cpppj `SquidPostPop.cpp/.h` 의 현재 계약으로 대조한다(WB04-E258, 3.6절 ② 재반영).
//  - 현재 metamap 관문(`sharedSuppressed`): 독립 suppressed 값보다 공유 getter 가 공통 후처리 진입의 억제를 정한다.
//  - 기본 장치(`bindDefaultPatchDevices`): 현재 Session 이 개별 연결한 AI 통지/로컬 owner/선택 재검사가 없을 때만 장기 기본 장치를 쓴다. 셋 중 하나라도 없으면 거부한다.
//  - 기본 자원 후처리(`bindDefaultResourcePostPop`): 기본 장치가 연결되고 현재 파생 몸체가 없는 자원 가상 표(Factory·buried·벌룬 그림자·자산 전용·Carrier/Damageable 직접 표)를 장기 자원 후처리로 보내고, 미설치면 그 위치에서 실패한다.
//  - 반납(`releaseRuntimeBindings`): 파생 몸체 열네 종·AI 통지/로컬 owner/선택 재검사/Teleport 접두만 반납하고 섬/표면 접두·기본 장치·기본 자원 후처리는 보존한다.
//  - 목록 정체성(`SquidPostPopList.matchesCurrentSource`): 같은 실제 저장소/행의 기존 getter 묶음만 재사용한다.
// C++ `runtimePin_`(호출 동안 파생 객체 수명 유지)은 JS 가 가비지 수집이라 대응이 없다. 실제 게임·브라우저 실행이 아니다. (2026-11-10 WB04-E258)
import assert from 'node:assert/strict';
import test from 'node:test';
import { RawBuriedPostPop } from '../www/core/raw-buried-post-pop.js';
import { RawCarrierSteal } from '../www/core/raw-carrier-steal.js';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { SquidPostPop } from '../www/core/squid-postpop.js';
import { SquidPostPopList, SquidPostPopState } from '../www/core/squid-postpop-state.js';
import { get, put } from './raw-support.mjs';

// 공통 객체(TYPE 74)의 가상 표와 사용하는 자원 가상 표 기록값이다(명령 실행 주소가 아니다). (2026-11-10 WB04-E258)
const COMMON_VTABLE = 0x501dd0;
const FACTORY_TABLE = 0x509148;
const BURIED_TABLE = 0x50a748;
const ASSET_TABLE = 0x50f540;
const CARRIER_TABLE = 0x503d80;
const DAMAGEABLE_TABLE = 0x50a820;

// 188칸 TYPE 입력(비용 0)이다. (2026-11-10 WB04-E258)
const makeTypes = () => Array.from({ length: 188 }, () => ({ flags1: 0, flags2: 0, footX: 1, footY: 1, cost: 0, maxHitPoints: 91 }));

// 예외가 났는지 본다. (2026-11-10 WB04-E258)
function throws(action, pattern = null) {
    try { action(); } catch (error) { return pattern ? pattern.test(error.message) : true; }
    return false;
}

// 객체 하나(SID)를 가진 작은 장면이다. 깊이는 호출자가 올려 둔 것으로 시작한다(실제 Activate 부모와 같다). (2026-11-10 WB04-E258)
function scene(vtable = COMMON_VTABLE) {
    const pool = new SidPool(24000, { server: false });
    const sid = pool.allocate(2);
    const raw = pool.physical(sid);
    put(raw, 0, vtable);
    raw[10] = 74;
    raw[34] = 1;
    const state = new SquidPostPopState();
    state.depth = 2;
    const post = new SquidPostPop(pool, makeTypes(), state);
    return { pool, sid, raw, state, post };
}

// 기본 장치 묶음(호출을 기록하는 세 장치)이다. (2026-11-10 WB04-E258)
function devices(log, owner = 1) {
    return {
        aiNotice: (current, sid, created) => log.push(['ai', current, sid, created]),
        localOwner: () => { log.push(['owner']); return owner; },
        recheckSelection: () => log.push(['recheck']),
    };
}

test('SquidPostPop(Native): 공유 metamap 관문이 독립 suppressed 값보다 공통 후처리 진입의 억제를 정한다', () => {
    const s = scene();
    s.state.depth = 0;
    s.state.suppressed = false;
    let metamap = true;
    s.state.sharedSuppressed = () => metamap;
    // 억제되면 슬롯/TYPE 을 보지 않고(범위 밖 SID 도 허용) 깊이만 32비트로 한 번 감는다.
    s.post.postPop(0xfffff, 1);
    assert.equal(s.state.depth, 0xffffffff);
    // 공유 getter 가 꺼지면 독립 값(false)이 아니라 현재 getter 를 읽어 실제 후처리로 들어가 범위 밖 SID 를 거부한다.
    metamap = false;
    s.state.suppressed = true;
    assert.ok(throws(() => s.post.postPop(0xfffff, 1)));
    // 공유 getter 가 없으면 독립 값을 쓴다.
    s.state.sharedSuppressed = null;
    s.state.depth = 0;
    s.post.postPop(0xfffff, 1);
    assert.equal(s.state.depth, 0xffffffff);
});

test('SquidPostPop(Native): 기본 장치는 셋 모두 필요하고 현재 Session 연결이 없을 때만 쓰며 반납 뒤에도 보존된다', () => {
    const s = scene();
    // 하나라도 없으면 거부한다.
    for (const missing of ['aiNotice', 'localOwner', 'recheckSelection']) {
        const given = devices([]);
        delete given[missing];
        assert.ok(throws(() => s.post.bindDefaultPatchDevices(given), /Client 장치 누락/), missing);
    }
    assert.ok(throws(() => s.post.bindDefaultPatchDevices(null)));
    const log = [];
    s.post.bindDefaultPatchDevices(devices(log, 1));
    // Session 연결이 없으면 기본 장치를 쓴다: 통지(소유자 1, 생성) → 로컬 owner → 선택 재검사.
    s.post.postPop(s.sid, 1);
    assert.deepEqual(log.map((entry) => entry[0]), ['ai', 'owner', 'recheck']);
    assert.deepEqual(log[0], ['ai', 1, s.sid, true]);
    assert.equal(s.state.depth, 1);
    // 현재 Session 이 개별 연결한 장치가 먼저이다.
    const session = [];
    s.post.bindAiNotice((current, sid, created) => session.push(['ai', current, sid, created]));
    s.post.bindLocalOwner(() => { session.push(['owner']); return 1; });
    s.post.bindSelectionRecheck(() => session.push(['recheck']));
    log.length = 0;
    s.state.depth = 2;
    s.post.postPop(s.sid, 1);
    assert.deepEqual(log, []);
    assert.deepEqual(session.map((entry) => entry[0]), ['ai', 'owner', 'recheck']);
    // 반납은 Session 연결만 지우고 기본 장치는 보존하므로 다시 기본 장치가 쓰인다.
    s.post.releaseRuntimeBindings();
    session.length = 0;
    s.state.depth = 2;
    s.post.postPop(s.sid, 1);
    assert.deepEqual(session, []);
    assert.deepEqual(log.map((entry) => entry[0]), ['ai', 'owner', 'recheck']);
});

test('SquidPostPop(Native): 기본 자원 후처리는 기본 장치가 있고 현재 파생이 없는 자원 가상 표에서만 선택되며 미설치면 실패한다', () => {
    const tables = [FACTORY_TABLE, 0x509210, BURIED_TABLE, ASSET_TABLE, CARRIER_TABLE, DAMAGEABLE_TABLE, 0x508cc8, 0x5040c0, 0x504198, 0x50e888, 0x509470];
    for (const table of tables) {
        const s = scene(table);
        const calls = [];
        // 기본 장치 없이는 선택되지 않는다(현재 파생이 없으면 미복원으로 거부한다).
        s.post.bindDefaultResourcePostPop((sid, flags) => calls.push([sid, flags]));
        assert.ok(throws(() => s.post.postPopVirtual(s.sid, 1)), `기본 장치 없음 ${table.toString(16)}`);
        assert.deepEqual(calls, []);
        s.post.bindDefaultPatchDevices(devices([]));
        s.post.postPopVirtual(s.sid, 1);
        assert.deepEqual(calls, [[s.sid, 1]], `표 ${table.toString(16)}`);
    }
    // 미설치면 공통 성공으로 바꾸지 않고 이 위치에서 실패한다.
    const missing = scene(FACTORY_TABLE);
    missing.post.bindDefaultPatchDevices(devices([]));
    assert.ok(throws(() => missing.post.postPopVirtual(missing.sid, 1), /장기 모듈 미설치/));
    // 현재 파생 몸체가 이 SID 를 처리하면 그 몸체가 먼저이고 기본 자원 후처리는 쓰지 않는다.
    const derived = scene(FACTORY_TABLE);
    const calls = [];
    derived.post.bindDefaultPatchDevices(devices([]));
    derived.post.bindDefaultResourcePostPop(() => calls.push('default'));
    derived.post.setBody('factory', { pool: derived.pool, handles: () => true, validate() {}, postPop: () => calls.push('body') });
    derived.post.postPopVirtual(derived.sid, 1);
    assert.deepEqual(calls, ['body']);
    // 자원 표가 아닌 공통 객체는 기본 자원 후처리를 거치지 않고 공통 몸체로 간다.
    const common = scene(COMMON_VTABLE);
    const commonCalls = [];
    common.post.bindDefaultPatchDevices(devices([]));
    common.post.bindDefaultResourcePostPop(() => commonCalls.push('default'));
    common.post.postPopVirtual(common.sid, 1);
    assert.deepEqual(commonCalls, []);
    assert.equal(common.state.depth, 1);
    // 함수가 아니면 거부한다.
    assert.ok(throws(() => common.post.bindDefaultResourcePostPop(null), /PATCH 장치 필요/));
});

test('SquidPostPop(Native): 반납은 파생 몸체와 Teleport 접두를 지우되 섬/표면 접두와 기본 자원 후처리는 남긴다', () => {
    const s = scene();
    const order = [];
    s.post.setIslandPrefix(() => order.push('island'));
    s.post.setSurfacePrefix(() => order.push('surface'));
    s.post.setTeleportPrefix(() => order.push('teleport'));
    s.post.setBody('asset', { pool: s.pool, handles: (sid) => s.pool.slot(sid)[0] === (ASSET_TABLE & 0xff), validate() {}, postPop: () => order.push('asset') });
    put(s.raw, 0, 0x506d98);
    s.post.postPopVirtual(s.sid, 1);
    put(s.raw, 0, 0x5070c0);
    s.state.depth = 2;
    s.post.postPopVirtual(s.sid, 1);
    put(s.raw, 0, ASSET_TABLE);
    s.post.postPopVirtual(s.sid, 1);
    assert.deepEqual(order, ['island', 'teleport', 'asset']);
    s.post.releaseRuntimeBindings();
    // 몸체는 사라졌으므로 같은 자산 표가 더 이상 분배되지 않고(미복원 거부), Teleport 접두는 다시 미연결이다.
    order.length = 0;
    assert.ok(throws(() => s.post.postPopVirtual(s.sid, 1), /몸체 미복원/));
    assert.deepEqual(order, []);
    put(s.raw, 0, 0x5070c0);
    assert.ok(throws(() => s.post.postPopVirtual(s.sid, 1), /Teleport 선택 생성 접두 미연결/));
    // 섬/표면 접두는 남아 있다.
    put(s.raw, 0, 0x506d98);
    s.state.depth = 2;
    s.post.postPopVirtual(s.sid, 1);
    put(s.raw, 0, 0x506f08);
    s.state.depth = 2;
    s.post.postPopVirtual(s.sid, 1);
    assert.deepEqual(order, ['island', 'surface']);
});

test('SquidPostPopList: 같은 실제 저장소/행 정체성의 기존 getter 묶음만 재사용 대상으로 인정한다', () => {
    const owner = {};
    const binding = (overrides = {}) => ({ storage: () => [], capacity: () => 0, readCount: () => 0, writeCount() {}, sourceOwner: owner, sourceSlot: 3, ...overrides });
    const list = new SquidPostPopList();
    assert.equal(list.matchesCurrentSource(owner, 3), false);
    list.bindCurrent(binding());
    assert.equal(list.matchesCurrentSource(owner, 3), true);
    assert.equal(list.matchesCurrentSource(owner, 4), false);
    assert.equal(list.matchesCurrentSource({}, 3), false);
    assert.equal(list.matchesCurrentSource(null, 3), false);
    // 정체성이 없는 묶음은 null 정체성끼리도 재사용하지 않는다.
    const plain = new SquidPostPopList();
    plain.bindCurrent(binding({ sourceOwner: undefined, sourceSlot: undefined }));
    assert.equal(plain.matchesCurrentSource(undefined, undefined), false);
    assert.equal(plain.matchesCurrentSource(owner, 3), false);
});

// ---- 3-A~3-C 에서 옮긴 몸체의 연결 (2026-11-10 WB04-E258) ----

test('SquidPostPop: buried 몸체를 연결하면 buried 가상 표는 QA·예약 뒤 공통 몸체를 한 번만 부른다', () => {
    const s = scene(BURIED_TABLE);
    s.raw[10] = 112;
    const trace = [];
    const body = new RawBuriedPostPop(s.pool, () => true, () => 0, {
        chooseType: (tier) => { trace.push(`choose,${tier}`); return 0x12c; },
        reserveRegular: () => { trace.push('reserve'); return true; },
        addRegular: (sid, event) => trace.push(`add,${event}`),
        validateBase: () => {},
        base: (sid, flags) => { trace.push('base'); s.post.postPopBase(sid, flags); },
        notifySurface: () => trace.push('notify'),
    });
    // 다른 풀의 몸체는 연결 때 거부한다.
    assert.ok(throws(() => s.post.setBody('buried', new RawBuriedPostPop(new SidPool(24000, { server: false }), () => true, () => 0, {})), /풀 불일치/));
    s.post.setBody('buried', body);
    // 최초 flag: 보상 추첨 → QA WORD → 두 예약 → 공통 몸체(깊이 2 → 1, 공통 몸체가 한 번만 호출됨).
    s.post.postPopVirtual(s.sid, 1);
    assert.deepEqual(trace, ['choose,5', 'reserve', 'add,0', 'reserve', 'add,1', 'base']);
    assert.equal(get(s.pool.slot(s.sid), 12, 2), 0x12c);
    assert.equal(s.state.depth, 1);
    // 최초 flag 가 없으면 추첨/예약 없이 공통 몸체만 부른다.
    trace.length = 0;
    s.state.depth = 2;
    s.post.postPopVirtual(s.sid, 0);
    assert.deepEqual(trace, ['base']);
    assert.equal(s.state.depth, 1);
    // 몸체를 해제하면 같은 가상 표는 더 이상 buried 로 분배되지 않는다(공통 성공으로 대신하지 않고 미복원으로 거부).
    s.post.setBody('buried', null);
    s.state.depth = 2;
    assert.ok(throws(() => s.post.postPopVirtual(s.sid, 1), /몸체 미복원/));
});

test('SquidPostPop: 크리스탈 크랩 포획 몸체를 연결하면 크랩 가상 표 둘은 예약 뒤 Damageable 공통 장부를 한 번 부른다', () => {
    for (const table of [0x503f20, 0x503ff0]) {
        const s = scene(table);
        const trace = [];
        const steal = new RawCarrierSteal(s.pool, new SquidHash(), makeTypes(), () => true, () => false, () => 1, new Array(81).fill(0), () => 0, () => 0, new Uint8Array(64), { value: 0 }, {
            allocateRegular: () => { trace.push('allocate'); return true; },
            regular: (sid, event, delay) => trace.push(`regular,${event},${delay}`),
            check: () => trace.push('check'),
            damageablePostPop: (sid, flags) => { trace.push(`damageable,${flags}`); s.post.postPopBase(sid, flags); },
        });
        s.post.setBody('carrierSteal', steal);
        s.post.postPopVirtual(s.sid, 1);
        assert.deepEqual(trace, ['allocate', 'regular,1170,1', 'damageable,1'], `표 ${table.toString(16)}`);
        assert.equal(s.state.depth, 1);
        // 최초 flag 가 없고 boss 가 아니면 포획 검사 뒤 공통 장부이다.
        trace.length = 0;
        s.state.depth = 2;
        s.post.postPopVirtual(s.sid, 0);
        assert.deepEqual(trace, ['damageable,0']);
    }
});
