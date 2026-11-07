// 삭제/해체 보상(SquidReward)과 분산 SP 저장소(ScrambledSpStore)를 원본 10.78 관측과 cpppj 계약 검사로 대조한다(WB04-D5b).
//  - 원본 관측(cpppj/tests/fixtures, 10.78 행만): reward-x86 2400행(cpppj RewardTests)·sp-add-bits-x86 504행(RewardSpBitsTests)·
//    rifttype-hit-points-x86 579행(RewardHitPointDiagnosticsTests, 초기 음수 번호 행 제외). 입력·명시 장치는 cpppj 검사와 같고 기대값은 fixture 에서 읽는다.
//  - 계약: cpppj RewardTests(저장소 왕복/거부·GameAux 비율/자원·사제 최대 HP)·RewardHitPointDiagnosticsTests·RewardTypeNumberTests(1~6)·RewardCurrentTypesTests(보상 부분).
//    삭제 장부(SquidDestroyLifecycle, WB04-E)·사제 상태(RawPriestState)·일반 피해 후처리 연결 검사는 해당 모듈 이식 때 옮긴다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-07 WB04-D5b: 보상 경제 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get, adler32, bitsToFloat, floatToBits, dwordBytes } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { GameRandom } from '../www/core/random.js';
import { GameAuxState } from '../www/core/game-aux-state.js';
import { ScrambledSpStore } from '../www/core/sp-store.js';
import { SquidReward, SquidRewardState } from '../www/core/squid-reward.js';

// 원본 관측 파일 폴더와 행 읽기이다. 파일이 없으면 해당 검사를 건너뛴다. (2026-11-07 WB04-D5b)
const FIXTURES = path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures');
const rewardRows = readRows(path.join(FIXTURES, 'reward-x86.tsv'))?.filter((row) => row[0] === 'originals') ?? null;
const spBitsRows = readRows(path.join(FIXTURES, 'sp-add-bits-x86.tsv'))?.filter((row) => row[0] !== 'entry') ?? null;
const hpRows = readRows(path.join(FIXTURES, 'rifttype-hit-points-x86.tsv'))?.filter((row) => row[0] === 'originals') ?? null;

// 합성 대상 SID 와 일반 장면의 대상 SID(cpppj kSid·kSource)이다. (2026-11-07 WB04-D5b)
const SID = 6, SOURCE = 50;

// 콤마 항목을 정수 목록으로 읽는다(부호 있는 10진수). (2026-11-07 WB04-D5b)
const numbers = (text) => text.split(',').map(Number);
// 예외가 나는지 본다. (2026-11-07 WB04-D5b)
const throws = (action) => {
    try {
        action();
        return false;
    } catch {
        return true;
    }
};
// 빈 188칸 TYPE 레코드 표와 장부이다(cpppj RiftTypeRecord 기본값·SquidPostPopState). (2026-11-07 WB04-D5b)
const emptyTypes = (count = 188) => Array.from({ length: count }, () => ({ cost: 0, maxHitPoints: 0, flags1: 0, flags2: 0 }));
const books = (localOwner = 0) => ({ localOwner, aiAttached: new Array(9).fill(false) });
// 분산 배열의 물리 순서 Adler-32 이다(cpppj PoolsAdler). (2026-11-07 WB04-D5b)
const poolsAdler = (store) => adler32(dwordBytes(Array.from(store.pools())));

// 원본 2400행: 객체/보조 타입·HP·장부·전역·지갑·초기 SP 저장(같은 재시드/채움)·가상 개수 대본으로 refund 한 뒤 샘 풀·지갑·SP·난수·배열·offset·표시/가상 호출 수.
test('SquidReward: 원본10.78 보상 2400행과 같다', { skip: !rewardRows && '원본 fixture 없음' }, () => {
    // 행마다 독립 풀·저장소·보상으로 실행한다.
    for (const row of rewardRows) {
        assert.equal(row.length, 30);
        const pool = new SidPool(32768, { server: true });
        // 5·6·7 을 차례로 할당한다.
        for (const sid of [5, 6, 7]) assert.equal(pool.allocate(2), sid);
        const types = emptyTypes();
        const number = Number(row[1]);
        // 객체 타입과 두 보조 타입 기록은 생성기와 같은 순서로 덮어쓴다.
        for (const [index, text] of [[number, row[2]], [147, row[3]], [164, row[4]]]) {
            const [cost, hp, flags1, flags2] = numbers(text);
            Object.assign(types[index], { cost: bitsToFloat(cost >>> 0), maxHitPoints: hp | 0, flags1: flags1 >>> 0, flags2: flags2 >>> 0, group: 10 });
        }
        const raw = pool.allocated(SID);
        raw.fill(0);
        raw[10] = number;
        raw[11] = Number(row[5]);
        raw[40] = Number(row[6]);
        put(raw, 26, Number(row[7]) >>> 0, 4);
        const book = books(Number(row[9]));
        const aiMask = Number(row[14]);
        // AI 부착 마스크를 소유자별로 푼다.
        for (let owner = 0; owner < 9; owner += 1) book.aiAttached[owner] = ((aiMask >>> owner) & 1) !== 0;
        const state = new SquidRewardState();
        const flags = Number(row[13]);
        state.percent = Number(row[11]) >>> 0;
        state.geyserPool = Number(row[12]) | 0;
        state.fixedSpA = (flags & 1) !== 0;
        state.fixedSpB = (flags & 2) !== 0;
        state.halfHealthMode = (flags & 4) !== 0;
        state.aiMirrorDisabled = (flags & 8) !== 0;
        state.walletClamp = (flags & 0x10) !== 0;
        state.priestQuarterHp = (flags & 0x20) !== 0;
        state.debugAsserts = (flags & 0x40) ? 1 : 0;
        numbers(row[15]).forEach((value, index) => { state.aiWallet[index] = value | 0; });
        const seed = Number(row[17]) >>> 0;
        const rng = new GameRandom(0);
        const store = new ScrambledSpStore(rng, () => seed, Number(row[18]) >>> 0);
        const sp = numbers(row[16]);
        // 같은 호출 순서로 초기 SP 를 쓴다.
        for (let owner = 0; owner < 9; owner += 1) store.set(owner, sp[owner] >>> 0);
        const label = row.slice(1, 20).join(' ');
        assert.equal(rng.state, Number(row[23]), label);
        assert.equal(poolsAdler(store), Number(row[24]), label);
        const script = row[19] === 'base' ? null : numbers(row[19]);
        let cursor = 0;
        let uiCalls = 0;
        const hooks = { refreshSp: () => { uiCalls += 1; } };
        if (script) {
            hooks.virtualCount = () => {
                assert.ok(cursor < script.length, label);
                return script[cursor++] | 0;
            };
        }
        const reward = new SquidReward(pool, types, book, state, store, hooks);
        reward.refund(SID, Number(row[8]), row[10] === '1');
        assert.equal(state.geyserPool >>> 0, Number(row[20]), label);
        assert.deepEqual(Array.from(state.aiWallet, (value) => value >>> 0), numbers(row[21]).map((value) => value >>> 0), label);
        assert.deepEqual(Array.from({ length: 9 }, (_, owner) => store.get(owner)), numbers(row[22]).map((value) => value >>> 0), label);
        assert.equal(rng.state, Number(row[25]), label);
        assert.equal(poolsAdler(store), Number(row[26]), label);
        assert.equal(store.offset, Number(row[27]), label);
        assert.equal(uiCalls, Number(row[28]), label);
        assert.equal(reward.virtualCountCalls, Number(row[29]), label);
    }
    assert.equal(rewardRows.length, 2400);
});

// 원본 504행: 소유자 0/1/3·고정 두 모드의 큰 유한/무한대/NaN/서브노멀 SP 와 delta 에 대한 저장 float 비트·지갑 절삭·장치 순서.
test('SquidReward.addSpBits: 원본10.78 SP 가산 비트 504행과 같다', { skip: !spBitsRows && '원본 fixture 없음' }, () => {
    // 행마다 같은 시드의 저장소로 실행한다.
    for (const row of spBitsRows) {
        assert.equal(row.length, 9);
        const owner = Number(row[1]);
        const fixed = Number(row[2]);
        const pool = new SidPool(24000);
        const state = new SquidRewardState();
        state.fixedSpA = (fixed & 1) !== 0;
        state.fixedSpB = (fixed & 2) !== 0;
        const store = new ScrambledSpStore(new GameRandom(1), () => 77);
        store.set(owner, Number(row[3]) >>> 0);
        const trace = [];
        let mirror = 0;
        const reward = new SquidReward(pool, emptyTypes(), books(), state, store, { refreshSp: () => trace.push('refresh') });
        reward.bindPatchWallet((selected, amount) => {
            assert.equal(selected, owner);
            mirror = amount >>> 0;
            trace.push('wallet');
        });
        reward.addSpBits(owner, Number(row[4]) >>> 0);
        const label = row.join(' ');
        assert.equal(store.get(owner), Number(row[6]), label);
        assert.equal(mirror, Number(row[7]), label);
        assert.deepEqual(trace, ['wallet', 'refresh'], label);
        assert.equal(row[8], 'get;set;wallet;refresh');
    }
    assert.equal(spBitsRows.length, 504);
});

// 원본 579행(10.78, 초기 음수 번호 제외): 선택 TYPE 의 HP·플래그와 checking/1/4/사제/다이스 현재값, 진단 복귀 뒤 바뀐 표 기준(±1 이동)·전역으로 다시 읽는다.
test('SquidReward.typeHitPoints: 원본10.78 TYPE HP 진단 579행과 같다', { skip: !hpRows && '원본 fixture 없음' }, () => {
    let checked = 0;
    // 행마다 190칸 물리 표의 188칸 보기를 만든다.
    for (const row of hpRows) {
        assert.equal(row.length, 6);
        const input = row[1].split(',');
        assert.equal(input.length, 9);
        if (Number(input[7]) !== 0) continue;
        const physical = emptyTypes(190);
        const number = Number(input[0]);
        const selected = physical[1 + number];
        selected.maxHitPoints = Number(input[1]) | 0;
        selected.flags1 = Number(input[2]) >>> 0;
        const origin = physical.slice(1, 189);
        const pool = new SidPool(32768);
        const random = new GameRandom(0);
        const mode = new SquidRewardState();
        const reward = new SquidReward(pool, origin, books(), mode, new ScrambledSpStore(random, () => 1));
        const owner = { checking: Number(input[3]) >>> 0, quarter: Number(input[4]) !== 0, priest: Number(input[5]) >>> 0, dais: Number(input[6]) >>> 0 };
        const profile = Number(input[8]);
        mode.debugAsserts = () => owner.checking;
        mode.share('priestQuarterHp', owner, 'quarter');
        mode.share('priestType', owner, 'priest');
        mode.share('daisType', owner, 'dais');
        reward.bindCurrentTypes(origin);
        const diagnostics = [];
        reward.bindHitPointDiagnostics((expression, file, line) => {
            diagnostics.push(`${expression}|${file}|${line}`);
            // 관측기의 명시 복귀 입력: checking 끔·1/4 켬·선택 HP 변경·표 기준 이동과 사제 번호.
            if (profile) {
                owner.checking = 0;
                owner.quarter = true;
                selected.maxHitPoints = [-13, -37, 41][profile - 1];
                const shift = [0, 1, -1][profile - 1];
                owner.priest = (74 - shift) >>> 0;
                reward.bindCurrentTypes(physical.slice(1 + shift, 1 + shift + 188));
            }
        });
        const seed = random.state;
        const free = pool.freeCount;
        const label = row.join(' ');
        assert.equal(reward.typeHitPoints(number), Number(row[2]), label);
        assert.equal(diagnostics.length ? diagnostics.join(';') : '-', row[3], label);
        assert.deepEqual([random.state, pool.freeCount, reward.virtualCountCalls], [seed, free, 0], label);
        checked += 1;
    }
    assert.equal(checked, 579);
});

// 저장소는 소유자 범위·재시드 누락을 변경 전에 거부하고, 모든 소유자의 값 비트가 서로 간섭 없이 되읽힌다(cpppj reward_sp_store_roundtrip_and_guards).
test('ScrambledSpStore: 왕복과 입력 거부', () => {
    const random = new GameRandom(123);
    const missing = new ScrambledSpStore(random, null, 0);
    assert.ok(throws(() => missing.get(0)));
    assert.ok(!missing.initialized);
    const store = new ScrambledSpStore(random, () => 0x12345678, 0xcdcdcdcd);
    assert.ok(throws(() => store.get(9)));
    assert.ok(throws(() => store.set(9, 1)));
    assert.ok(!store.initialized);
    // 세 값과 모든 소유자가 서로 간섭하지 않는다.
    for (let owner = 0; owner < 9; owner += 1) store.set(owner, (0x80000001 + owner * 0x01010101) >>> 0);
    for (let owner = 0; owner < 9; owner += 1) assert.equal(store.get(owner), (0x80000001 + owner * 0x01010101) >>> 0);
    store.set(4, 0xffffffff);
    assert.equal(store.get(4), 0xffffffff);
    assert.equal(store.get(3), (0x80000001 + 3 * 0x01010101) >>> 0);
});

// 회전은 offset 을 3 늘리고 모든 DWORD 를 돌려도 값이 유지되며, 추적 offset 소유자는 초기화 전에만 연결된다(cpppj SpStore 계약).
test('ScrambledSpStore: 회전·추적 offset·할당 입력', () => {
    const tracking = { value: -1 };
    const store = new ScrambledSpStore(new GameRandom(9), () => 0x55aa, (bytes) => bytes.fill(0x11));
    store.bindTrackingOffset(tracking);
    store.set(2, 0xdeadbeef);
    assert.equal(tracking.value, store.offset);
    assert.ok(store.firstPoolReady);
    const offset = store.offset;
    const before = store.pools();
    store.rotate();
    assert.equal(store.offset, offset + 3);
    assert.equal(store.get(2), 0xdeadbeef);
    assert.equal(store.pools()[0], (((before[0] << 3) | (before[0] >>> 29)) >>> 0));
    assert.ok(throws(() => store.bindTrackingOffset({ value: 0 })));
    assert.ok(throws(() => new ScrambledSpStore(new GameRandom(1), () => 1).trackingOffset));
});

// 보상 입력 오류는 상태를 바꾸기 전에 거부한다(cpppj reward_rejects_bad_inputs_before_writing).
test('SquidReward: 잘못된 입력을 쓰기 전에 거부한다', () => {
    const pool = new SidPool(32768);
    pool.allocate(2);
    pool.allocated(5)[10] = 82;
    const types = emptyTypes();
    types[82].cost = 100;
    const state = new SquidRewardState();
    const store = new ScrambledSpStore(new GameRandom(0), () => 1);
    assert.ok(throws(() => new SquidReward(pool, types, books(), state, null)));
    const reward = new SquidReward(pool, types, books(), state, store);
    assert.ok(throws(() => reward.refund(5, 9, false)));
    assert.ok(throws(() => reward.refund(0, 1, false)));
    assert.ok(!store.initialized);
    assert.equal(state.geyserPool, 0);
});

// 실제 SP 갱신 효과가 자원을 바꾼 뒤 refund 는 같은 소유자의 현재 595088 을 다시 읽고, 비율도 현재값을 쓴다(cpppj GameAux_RewardUsesCurrentPercentAndResourceAfterSpEffect).
test('SquidReward: SP 효과 뒤 현재 비율·자원을 다시 읽는다', () => {
    const owner = new GameAuxState();
    const pool = new SidPool(32768);
    assert.equal(pool.allocate(2), 5);
    pool.allocated(5).fill(0);
    pool.allocated(5)[10] = 74;
    const types = emptyTypes();
    types[74].cost = 100;
    const store = new ScrambledSpStore(new GameRandom(0), () => 1);
    store.set(1, floatToBits(1000));
    const state = new SquidRewardState();
    state.percent = 99;
    state.geyserPool = -777;
    state.share('percent', owner, 'refundPercent');
    state.share('geyserPool', owner, 'resourcePool');
    owner.resourcePool = 10;
    let effects = 0;
    const reward = new SquidReward(pool, types, books(), state, store, { refreshSp: () => { effects += 1; owner.resourcePool = 1000; } });
    const first = reward.refund(5, 1, false);
    assert.deepEqual([first.applied, first.cost, first.refund, owner.resourcePool, effects, state.geyserPool, state.percent], [true, 100, 25, 1075, 1, -777, 99]);
    owner.refundPercent = 50;
    const second = reward.refund(5, 1, false);
    assert.deepEqual([second.cost, second.refund, owner.resourcePool, effects, state.geyserPool, state.percent], [100, 50, 1050, 2, -777, 99]);
});

// 최대 HP 조회가 실제 005ca8f0 의 현재값을 읽고 독립 값은 보존한다(cpppj GameAux_RewardPriestMaximumUsesCurrentQuarterHp).
test('SquidReward: 사제 1/4 HP 는 빌린 현재값을 읽는다', () => {
    const owner = new GameAuxState();
    const types = emptyTypes();
    types[158].maxHitPoints = 200;
    types[89].maxHitPoints = 120;
    const state = new SquidRewardState();
    state.priestQuarterHp = true;
    state.share('priestQuarterHp', owner);
    const reward = new SquidReward(new SidPool(32768), types, books(), state, new ScrambledSpStore(new GameRandom(0), () => 1));
    assert.deepEqual([reward.typeHitPoints(158), reward.typeHitPoints(89)], [200, 120]);
    owner.priestQuarterHp = true;
    assert.deepEqual([reward.typeHitPoints(158), reward.typeHitPoints(89)], [50, 120]);
    owner.priestQuarterHp = false;
    assert.equal(reward.typeHitPoints(158), 200);
    assert.ok(state.priestQuarterHp);
});

// 다이스 최대 HP 도 선택한 제단 TYPE 기록을 유지하고 진단 뒤 현재 약화/사제 번호를 읽는다(cpppj RewardHitPointDiagnostics_MaximumKeepsSelectedAltarAfterDiagnosis).
test('SquidReward.maxHitPoints: 진단 뒤에도 선택한 제단 기록을 유지한다', () => {
    const physical = emptyTypes(189);
    physical[74].flags2 = 0x400000;
    physical[75].maxHitPoints = 29;
    physical[76].maxHitPoints = 999;
    const pool = new SidPool(32768);
    const state = new SquidRewardState();
    const reward = new SquidReward(pool, physical.slice(0, 188), books(), state, new ScrambledSpStore(new GameRandom(0), () => 1));
    reward.bindCurrentTypes(physical.slice(0, 188));
    const owner = new GameAuxState();
    owner.altarType = 75;
    owner.daisType = 77;
    owner.priestType = 158;
    state.bindGameAux(owner);
    let checking = 0x80000000;
    let calls = 0;
    state.debugAsserts = () => checking;
    const sid = pool.allocate(2);
    const raw = pool.allocated(sid);
    raw[10] = 74;
    raw[40] = 1;
    put(raw, 26, 123);
    reward.bindHitPointDiagnostics((expression, file, line) => {
        assert.deepEqual([expression, file, line], ['(typeFlags & TYPE_FLAG_HAS_HIT_POINTS) != 0 || getNum() == daisType', '\\Ns\\O\\Rifttype.cpp', 0x1e3]);
        calls += 1;
        owner.altarType = 76;
        owner.priestType = 74;
        owner.priestQuarterHp = true;
        physical[75].maxHitPoints = -37;
        raw[10] = 76;
        reward.bindCurrentTypes(physical.slice(1, 189));
    });
    // 원문 진단이 바꾼 raw TYPE 이나 새 제단의 999 로 다시 고르지 않는다.
    assert.equal(reward.maxHitPoints(sid), -9);
    assert.deepEqual([calls, get(raw, 26), raw[10]], [1, 123, 76]);
    checking = 0;
});

// 진단 장치의 교체/누락과 checking/다이스에 따른 미선택을 구별한다(cpppj RewardHitPointDiagnostics_MissingAndReplacedCallbackStayAtSelectedBoundary).
test('SquidReward.typeHitPoints: 진단 장치 누락·교체 경계', () => {
    const types = emptyTypes();
    types[74].maxHitPoints = -7;
    const state = new SquidRewardState();
    let checking = 1;
    state.debugAsserts = () => checking;
    state.types.dais = 75;
    const reward = new SquidReward(new SidPool(32768), types, books(), state, new ScrambledSpStore(new GameRandom(0), () => 1));
    assert.ok(throws(() => reward.typeHitPoints(74)));
    reward.bindHitPointDiagnostics(() => {
        reward.bindHitPointDiagnostics(null);
        checking = 0;
        state.priestQuarterHp = true;
        state.types.priest = 74;
    });
    assert.equal(reward.typeHitPoints(74), -1);
    assert.equal(reward.typeHitPoints(74), -1);
    checking = 1;
    state.types.dais = 74;
    assert.equal(reward.typeHitPoints(74), -1);
    state.types.dais = 75;
    assert.ok(throws(() => reward.typeHitPoints(74)));
});

// 보상 번호 장면이다: 5..60 할당, 비용/HP 입력, 대상 50 은 TYPE 70(cpppj RewardTypeNumberTests 의 RewardNumberScene). (2026-11-07 WB04-D5b)
function numberScene(hooks = {}) {
    const pool = new SidPool(24000, { server: false });
    // 합성 대상의 raw 상태를 할당 뒤 0 으로 준비한다.
    for (let sid = 5; sid <= 60; sid += 1) {
        assert.equal(pool.allocate(2), sid);
        pool.allocated(sid).fill(0);
    }
    const types = emptyTypes();
    [[70, 40], [71, 80], [72, 120], [73, 200], [147, 300], [150, 500], [154, 600], [164, 1000], [165, 1200]].forEach(([number, cost]) => { types[number].cost = cost; });
    types[70].maxHitPoints = 100;
    types[71].maxHitPoints = 200;
    types[73].maxHitPoints = 700;
    pool.allocated(SOURCE)[10] = 70;
    const mode = new SquidRewardState();
    const owner = new GameAuxState();
    const scene = { pool, types, mode, owner, reward: null };
    scene.bind = () => {
        mode.share('altarType', owner);
        mode.share('daisType', owner);
        mode.share('silentType', owner);
        mode.share('aliasType', owner);
        mode.share('nuggetType', owner);
    };
    scene.make = (selectedHooks = hooks) => {
        scene.reward = new SquidReward(pool, types, books(), mode, new ScrambledSpStore(new GameRandom(12345), () => 1), selectedHooks);
        scene.reward.bindCurrentTypes(types);
    };
    return scene;
}

// 초기 PE 값과 일반 월드 준비의 보존, 명시 빌림 없는 독립 입력(cpppj RewardTypeNumber_OwnerDefaultsResetAndUnboundCompatibility).
test('SquidReward: 특별 TYPE 번호 기본값과 빌림 전후', () => {
    const scene = numberScene();
    const { owner } = scene;
    assert.deepEqual([owner.altarType, owner.daisType, owner.rainBlockerType, owner.growingRainBlockerType, owner.nuggetType], [164, 165, 147, 150, 154]);
    Object.assign(owner, { altarType: 70, daisType: 71, rainBlockerType: 72, growingRainBlockerType: 73, nuggetType: 74 });
    owner.resetForGeneralWorld();
    assert.deepEqual([owner.altarType, owner.daisType, owner.rainBlockerType, owner.growingRainBlockerType, owner.nuggetType], [70, 71, 72, 73, 74]);
    scene.make();
    assert.equal(scene.reward.plan(SOURCE, 1, false).cost, 40);
    assert.equal(scene.reward.plan(SOURCE, 1, true).refund, 10);
    scene.bind();
    assert.equal(scene.reward.salvageRefund(SOURCE, 70, 0), 0);
    assert.ok(!scene.reward.plan(SOURCE, 1, true).applied);
});

// 같은 현재 번호가 개수−1·HP 예외·다이스 최대 HP 원천을 고른다(cpppj RewardTypeNumber_CurrentAltarDaisCountAndHp).
test('SquidReward: 현재 제단/다이스 번호의 개수·HP', () => {
    const scene = numberScene();
    scene.bind();
    scene.owner.altarType = 70;
    scene.owner.daisType = 71;
    scene.types[70].flags1 = 0x10;
    scene.types[73].flags1 = 0x10;
    scene.types[72].flags2 = 0x400000;
    scene.make();
    const raw = scene.pool.allocated(SOURCE);
    raw[10] = 74;
    assert.deepEqual([0, 1].flatMap((count) => [70, 71].map((type) => scene.reward.salvageRefund(SOURCE, type, count))), [0, 0, 10, 20]);
    raw[10] = 72;
    raw[40] = 1;
    assert.equal(scene.reward.maxHitPoints(SOURCE), 100);
    scene.owner.altarType = 73;
    assert.equal(scene.reward.maxHitPoints(SOURCE), 700);
    raw[10] = 74;
    assert.equal(scene.reward.salvageRefund(SOURCE, 70, 0), 10);
    scene.mode.debugAsserts = 1;
    assert.equal(scene.reward.typeHitPoints(71), 200);
    scene.owner.daisType = 74;
    assert.ok(throws(() => scene.reward.typeHitPoints(71)));
});

// 삭제 제외·성장 비용 별칭·성장 환불 0·너겟 전액을 독립해서 바꾼다(cpppj RewardTypeNumber_CurrentRainAliasesAndFullNuggetRefund).
test('SquidReward: 현재 레인 바리케이드 별칭과 너겟 전액 환불', () => {
    const scene = numberScene();
    scene.bind();
    scene.make();
    scene.owner.rainBlockerType = 70;
    assert.ok(!scene.reward.plan(SOURCE, 1, false).applied);
    assert.equal(scene.reward.plan(SOURCE, 1, true).refund, 10);
    scene.owner.nuggetType = 70;
    assert.equal(scene.reward.plan(SOURCE, 1, true).refund, 40);
    scene.owner.growingRainBlockerType = 70;
    assert.equal(scene.reward.plan(SOURCE, 1, true).refund, 0);
    scene.owner.rainBlockerType = 71;
    assert.deepEqual(scene.reward.plan(SOURCE, 1, false), { applied: true, cost: 80, refund: 20 });
    scene.owner.growingRainBlockerType = 72;
    assert.equal(scene.reward.plan(SOURCE, 1, false).cost, 40);
});

// 가상 개수 → 현재 레인 바리케이드 TYPE 로드 순서(cpppj RewardTypeNumber_AliasReadsCurrentRainAfterSecondVirtualCount, 10.78).
test('SquidReward: 별칭은 두 번째 가상 개수 뒤 현재 레인 바리케이드를 읽는다', () => {
    let calls = 0;
    const scene = numberScene();
    scene.bind();
    scene.make({
        virtualCount: (sid) => {
            assert.equal(sid, SOURCE);
            calls += 1;
            if (calls === 1) scene.owner.growingRainBlockerType = 70;
            else scene.owner.rainBlockerType = 71;
            return 0;
        },
    });
    const result = scene.reward.plan(SOURCE, 1, false);
    assert.deepEqual([calls, result.cost, result.refund, scene.reward.virtualCountCalls], [2, 80, 20, 2]);
});

// 별칭 판정은 첫 가상 개수 뒤의 현재 슬롯 TYPE BYTE 를 읽는다(cpppj Plan 의 raw span 재조회 계약을 웹에서 확인하는 검사, 음성 대조에서 추가). (2026-11-07 WB04-D5b)
test('SquidReward: 별칭 판정은 가상 개수 뒤 현재 슬롯 TYPE 을 읽는다', () => {
    let calls = 0;
    const scene = numberScene();
    scene.bind();
    scene.make({
        virtualCount: () => {
            calls += 1;
            if (calls === 1) scene.pool.allocated(SOURCE)[10] = 150;
            return 0;
        },
    });
    const result = scene.reward.plan(SOURCE, 1, false);
    // 첫 TYPE 70 의 곡선 뒤 현재 TYPE 150(성장형)이라 두 번째 개수와 레인 바리케이드 147 의 비용 300 을 쓴다.
    assert.deepEqual([calls, result.cost, result.refund], [2, 300, 75]);
});

// PATCH 0044c37f 는 가상 개수 전에 환불 TYPE 을 저장한다(cpppj RewardTypeNumber_PatchSavesRefundTypeBeforeCountAndCdReadsAfter, 10.78).
test('SquidReward: 환불 TYPE 은 가상 개수 전에 저장한다', () => {
    let calls = 0;
    const scene = numberScene();
    scene.bind();
    scene.owner.nuggetType = 71;
    scene.make({
        virtualCount: () => {
            calls += 1;
            if (calls === 2) scene.pool.allocated(SOURCE)[10] = 71;
            return 0;
        },
    });
    const result = scene.reward.plan(SOURCE, 1, true);
    assert.deepEqual([calls, result.cost, result.refund, scene.pool.slot(SOURCE)[10]], [2, 40, 10, 71]);
});

// 번호 변경과 TYPE float 비용 변경은 별도이며 최초 정수 비용 캐시를 다시 만들지 않는다(cpppj RewardTypeNumber_CurrentNumberBindingPreservesLoadedCostSnapshot·RewardCurrentTypes).
test('SquidReward: 현재 TYPE 표 빌림과 로딩 비용 사본', () => {
    const types = emptyTypes();
    Object.assign(types[158], { maxHitPoints: 200, flags1: 0x10, flags2: 0x200000, cost: 80 });
    Object.assign(types[164], { maxHitPoints: 400, flags1: 0x10 });
    const pool = new SidPool(24000, { server: false });
    // 대상까지 실제 할당한다.
    for (let sid = 5; sid <= SOURCE; sid += 1) assert.equal(pool.allocate(2), sid);
    const raw = pool.allocated(SOURCE);
    raw.fill(0);
    raw[10] = 158;
    put(raw, 26, 150);
    const mode = new SquidRewardState();
    const random = new GameRandom(0);
    const reward = new SquidReward(pool, types, books(), mode, new ScrambledSpStore(random, () => 1));
    // 독립 구성은 생성 시 사본을 쓴다.
    types[158].maxHitPoints = 400;
    types[158].cost = 120;
    assert.deepEqual([reward.maxHitPoints(SOURCE), reward.typeCostUnits(158)], [200, 80]);
    reward.bindCurrentTypes(types);
    assert.deepEqual([reward.maxHitPoints(SOURCE), reward.typeCostUnits(158)], [400, 80]);
    types[158].maxHitPoints = 600;
    assert.equal(reward.maxHitPoints(SOURCE), 600);
    mode.priestQuarterHp = true;
    assert.equal(reward.maxHitPoints(SOURCE), 150);
    mode.priestQuarterHp = false;
    assert.ok(reward.hasHitPoints(SOURCE));
    types[158].flags1 = 0;
    assert.ok(!reward.hasHitPoints(SOURCE));
    types[158].flags2 = 0x400000;
    assert.ok(!reward.hasHitPoints(SOURCE));
    pool.allocated(SOURCE)[40] = 1;
    assert.deepEqual([reward.hasHitPoints(SOURCE), reward.maxHitPoints(SOURCE)], [true, 400]);
    types[164].maxHitPoints = 900;
    assert.equal(reward.maxHitPoints(SOURCE), 900);
    // 잘못된 빌림은 기존 연결을 바꾸지 않는다.
    assert.ok(throws(() => reward.bindCurrentTypes(types.slice(0, 187))));
    assert.equal(reward.maxHitPoints(SOURCE), 900);
    assert.ok(throws(() => reward.typeHitPoints(188)));
    assert.ok(throws(() => reward.typeCostUnits(188)));
    // 로딩 비용표를 빌리면 그 최초 기록을, 풀면 생성 시 사본을 쓴다.
    const loaded = emptyTypes();
    loaded[158].cost = 33;
    reward.bindLoadedCosts(loaded);
    assert.equal(reward.typeCostUnits(158), 33);
    reward.bindLoadedCosts(null);
    assert.equal(reward.typeCostUnits(158), 80);
    reward.bindCurrentTypes(null);
    assert.equal(reward.maxHitPoints(SOURCE), 200);
    assert.equal(random.state, 0);
    assert.equal(reward.virtualCountCalls, 0);
});
