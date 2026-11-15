// 전투 자원 프레임 raw-battle-resource-frame.js(RawBattleResourceFrame, 원본 00486bc0)를 cpppj 소스 계약으로 대조한다(WB04-E560).
// 원본 관측 fixture 가 없는 소단위이므로 로그인 중 반환·스펠 만료 한 번 소비·SP 주기(최초 1 쓰기 뒤 지급)·일반 간헐천 보충(전체 한 개/3×3 구역)의 경계와 호출 순서·선택 장치 요구를 직접 단언한다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-15 WB04-E560)
import test from 'node:test';
import assert from 'node:assert/strict';
import { RawFortFinishOwner } from '../www/core/raw-fort-finish-owner.js';
import { runBattleResourceFrame } from '../www/core/raw-battle-resource-frame.js';

// 완전한 전역·장치 한 벌이다. 모든 기한은 아직 지나지 않은 값이며 trace 에 호출 순서를 남긴다. (2026-11-15 WB04-E560)
function makeScene(overrides = {}) {
    const trace = [];
    const options = new Uint8Array(87);
    const players = new Uint8Array(9 * 180);
    const aux = { spellCount: 0, spellDeadline: 0, incomePrimed: 0, geyserFrameFlag: 0, geyserDeadline: 1e9, secondaryGeyserDeadline: 1e9, ordinaryGeysers: 0, resourcePool: 0 };
    const globals = {
        aux,
        loginEnd: { value: 0 },
        now: { value: 100 },
        boss: { value: true },
        battle: { value: true },
        single: { value: false },
        editor: { value: 0 },
        owner: { value: 1 },
        options,
        players: () => { trace.push('players'); return players; },
        ...overrides.globals
    };
    const rect = (cell) => ({ left: cell, top: cell, right: cell + 1, bottom: cell + 1 });
    const hooks = {
        replenish: () => trace.push('replenish'),
        income: (owner, localOwner, amount) => trace.push(`income,${owner},${localOwner},${amount}`),
        predictable: () => { trace.push('predictable'); return true; },
        wallSeconds: () => { trace.push('wall'); return 777.5; },
        budget: () => { trace.push('budget'); return 100; },
        density: () => { trace.push('density'); return 0; },
        spacing: () => { trace.push('spacing'); return 0; },
        whole: () => { trace.push('whole'); return rect(0); },
        partition: (cell) => { trace.push(`partition,${cell}`); return rect(cell); },
        count: (rectangle) => { trace.push(`count,${rectangle.left}`); return 0; },
        geyserCount: () => { trace.push('geyserCount'); return 0; },
        typeCost: () => { trace.push('typeCost'); return 5; },
        choose: (count) => { trace.push(`choose,${count}`); return 0; },
        populate: (quantity, random, rectangle, placement) => { trace.push(`populate,${quantity},${rectangle.left}`); trace.populated = { quantity, random, rectangle, placement }; },
        ...overrides.hooks
    };
    return { trace, options, players, aux, globals, hooks };
}
// 구독자(+0x2c)가 있는 Player 행을 만든다. flags 는 로그인 판정(0x22 중 하나)이다. (2026-11-15 WB04-E560)
function addSubscriber(scene, owner, flags = 2) {
    new DataView(scene.players.buffer).setUint32(owner * 180 + 0x2c, 0x1000 + owner, true);
    scene.players[owner * 180 + 0x50] = flags;
}
// 모든 구독자 Player 를 로그인한 상태로 만든다. (2026-11-15 WB04-E560)
function loginAll(scene, owners = [1, 2, 3, 4]) {
    for (const owner of owners) addSubscriber(scene, owner);
}

test('로그인 중이면 아무 효과 없이 반환하고 옵션 크기가 87바이트가 아니면 먼저 거부한다', () => {
    for (const loginEnd of [5, -1, NaN, 1e-300]) {
        const scene = makeScene({ globals: { loginEnd: { value: loginEnd } } });
        scene.aux.spellCount = 2;
        scene.aux.spellDeadline = 10;
        runBattleResourceFrame(scene.globals, scene.hooks);
        assert.deepEqual(scene.trace, []);
        assert.equal(scene.aux.spellCount, 2);
    }
    // -0 은 0 과 같아 로그인이 끝난 것으로 본다.
    const zero = makeScene({ globals: { loginEnd: { value: -0 } } });
    zero.aux.spellCount = 1;
    zero.aux.spellDeadline = 10;
    runBattleResourceFrame(zero.globals, zero.hooks);
    assert.equal(zero.aux.spellCount, 0);
    for (const size of [0, 86, 88]) {
        const scene = makeScene({ globals: { options: new Uint8Array(size), loginEnd: { value: 9 } } });
        assert.throws(() => runBattleResourceFrame(scene.globals, scene.hooks), RangeError);
    }
});

test('스펠 만료: 개수 양수·기한 있음·시각 도달일 때 한 번 소비하고 보스만 보충하며 새 기한은 보충 뒤 시각+20이다', () => {
    const scene = makeScene();
    scene.aux.spellCount = 3;
    scene.aux.spellDeadline = 100;
    runBattleResourceFrame(scene.globals, scene.hooks);
    assert.deepEqual(scene.trace, ['replenish']);
    assert.equal(scene.aux.spellCount, 2);
    assert.equal(scene.aux.spellDeadline, 120);
    // 보스가 아니면 보충 없이 소비한다.
    const guest = makeScene({ globals: { boss: { value: false } } });
    guest.aux.spellCount = 3;
    guest.aux.spellDeadline = 99.5;
    runBattleResourceFrame(guest.globals, guest.hooks);
    assert.deepEqual(guest.trace, []);
    assert.equal(guest.aux.spellCount, 2);
    // 막는 조건: 개수 0·signed 음수·기한 0·아직 이른 시각.
    for (const [count, deadline] of [[0, 50], [0x80000000, 50], [0xffffffff, 50], [3, 0], [3, 100.0001]]) {
        const blocked = makeScene();
        blocked.aux.spellCount = count;
        blocked.aux.spellDeadline = deadline;
        runBattleResourceFrame(blocked.globals, blocked.hooks);
        assert.equal(blocked.aux.spellCount, count);
        assert.equal(blocked.aux.spellDeadline, deadline);
        assert.deepEqual(blocked.trace, []);
    }
    // 보충 효과가 시각을 바꾸면 새 기한은 바뀐 현재 시각에서 계산하고, 개수는 1 에서 0 으로 내려간다.
    const moved = makeScene();
    moved.aux.spellCount = 1;
    moved.aux.spellDeadline = 90;
    moved.hooks.replenish = () => { moved.trace.push('replenish'); moved.globals.now.value = 130; };
    runBattleResourceFrame(moved.globals, moved.hooks);
    assert.equal(moved.aux.spellCount, 0);
    assert.equal(moved.aux.spellDeadline, 150);
    // 보충 장치 누락은 개수를 줄이기 전에 명시 오류이다.
    const missing = makeScene({ hooks: { replenish: undefined } });
    missing.aux.spellCount = 2;
    missing.aux.spellDeadline = 90;
    assert.throws(() => runBattleResourceFrame(missing.globals, missing.hooks), /미연결/);
    assert.equal(missing.aux.spellCount, 2);
});

test('SP 주기: 비전투면 선택 상태만 초기화하고 보스·전투·전원 로그인·주기 비영일 때 최초는 1 만 쓴다', () => {
    // 아직 기한 전이거나 간헐천 프레임 플래그가 있으면 건드리지 않는다.
    const early = makeScene();
    early.aux.incomePrimed = 1;
    runBattleResourceFrame(early.globals, early.hooks);
    assert.equal(early.aux.incomePrimed, 1);
    const flagged = makeScene({ globals: {} });
    flagged.aux.geyserDeadline = 0;
    flagged.aux.geyserFrameFlag = 1;
    flagged.aux.incomePrimed = 1;
    flagged.aux.ordinaryGeysers = 1;
    flagged.options[20] = 2;
    loginAll(flagged);
    runBattleResourceFrame(flagged.globals, flagged.hooks);
    // 간헐천 프레임 플래그가 있으면 기한이 지났어도 SP 주기 전체를 건너뛴다(지급도 기한 이동도 없다).
    assert.equal(flagged.aux.incomePrimed, 1);
    assert.equal(flagged.aux.geyserDeadline, 0);
    assert.equal(flagged.trace.some((entry) => entry.startsWith('income')), false);
    // 비전투: 기한이 지나면 최초 지급 표시를 0 으로 되돌리고 지급은 하지 않는다.
    const idle = makeScene({ globals: { battle: { value: false } } });
    idle.aux.geyserDeadline = 100;
    idle.aux.incomePrimed = 1;
    idle.aux.ordinaryGeysers = 1;
    idle.options[20] = 2;
    loginAll(idle);
    runBattleResourceFrame(idle.globals, idle.hooks);
    assert.equal(idle.aux.incomePrimed, 0);
    assert.equal(idle.aux.geyserDeadline, 100);
    assert.equal(idle.trace.some((entry) => entry.startsWith('income')), false);
    // 최초(primed 0): 기한을 주기+현재로 옮기고 1 만 쓰며 지급하지 않는다.
    const first = makeScene();
    first.aux.geyserDeadline = 100;
    first.aux.ordinaryGeysers = 1;
    first.options[20] = 2;
    loginAll(first);
    runBattleResourceFrame(first.globals, first.hooks);
    assert.equal(first.aux.incomePrimed, 1);
    assert.equal(first.aux.geyserDeadline, 160);
    assert.equal(first.trace.some((entry) => entry.startsWith('income')), false);
});

test('SP 지급: 이미 시작된 주기에서 숨김 아닌 구독자에게 지급액을 순서대로 지급한다', () => {
    const scene = makeScene({ globals: { owner: { value: 3 } } });
    scene.aux.geyserDeadline = 90;
    scene.aux.incomePrimed = 1;
    scene.aux.ordinaryGeysers = 1;
    scene.options[20] = 3;
    scene.options[21] = 2;
    loginAll(scene, [1, 2, 4, 6]);
    scene.options[62 + 2] = 1;
    runBattleResourceFrame(scene.globals, scene.hooks);
    // 지급액은 옵션 +0x15 의 선택(2 → 2500)이고 주기는 +0x14(3 → 120초)이다. 숨김(+62+owner)인 2 번은 건너뛴다.
    assert.deepEqual(scene.trace.filter((entry) => entry.startsWith('income')), ['income,1,3,2500', 'income,4,3,2500', 'income,6,3,2500']);
    assert.equal(scene.aux.geyserDeadline, 220);
    assert.equal(scene.aux.incomePrimed, 1);
    // 로그인 판정은 8행을 모두 한 번씩, 지급 판정은 숨김이 아닌 행마다 현재 Player 를 다시 빌린다.
    assert.equal(scene.trace.filter((entry) => entry === 'players').length, 8 + 7);
});

test('SP 지급 조건: 보스·전투·주기·전원 로그인 중 하나라도 아니면 기한도 지급도 바꾸지 않는다', () => {
    const base = (mutate) => {
        const scene = makeScene();
        scene.aux.geyserDeadline = 90;
        scene.aux.incomePrimed = 1;
        scene.aux.ordinaryGeysers = 1;
        scene.options[20] = 1;
        loginAll(scene);
        mutate(scene);
        runBattleResourceFrame(scene.globals, scene.hooks);
        return scene;
    };
    const ok = base(() => {});
    assert.equal(ok.trace.filter((entry) => entry.startsWith('income')).length, 4);
    for (const [label, mutate] of [
        ['보스 아님', (scene) => { scene.globals.boss = { value: false }; }],
        ['주기 0', (scene) => { scene.options[20] = 0; }],
        ['로그인 안 한 구독자', (scene) => { scene.players[3 * 180 + 0x50] = 0; }],
        ['로그인 비트 0x20', null]
    ]) {
        if (!mutate) continue;
        const scene = base(mutate);
        assert.equal(scene.trace.some((entry) => entry.startsWith('income')), false, label);
        assert.equal(scene.aux.geyserDeadline, 90, label);
    }
    // 로그인 판정은 flags 0x02 또는 0x20 한 비트면 충분하고 구독자 없는 행은 무시한다.
    const bits = base((scene) => { scene.players[2 * 180 + 0x50] = 0x20; scene.players[3 * 180 + 0x50] = 0x40; scene.players[3 * 180 + 0x2c] = 0; scene.players[3 * 180 + 0x2d] = 0; });
    assert.equal(bits.aux.geyserDeadline, 130);
    // 지급 장치가 없으면 첫 지급 시점에 명시 오류이고 기한은 이미 옮겨졌다.
    const noIncome = makeScene({ hooks: { income: undefined } });
    noIncome.aux.geyserDeadline = 90;
    noIncome.aux.incomePrimed = 1;
    noIncome.aux.ordinaryGeysers = 1;
    noIncome.options[20] = 1;
    loginAll(noIncome);
    assert.throws(() => runBattleResourceFrame(noIncome.globals, noIncome.hooks), /미연결/);
    assert.equal(noIncome.aux.geyserDeadline, 130);
});

test('일반 간헐천 조기 반환: 일반 모드·기한 전·보스 아님·편집·주기 0·미로그인이면 보충하지 않고 기한만 옮긴다', () => {
    // 일반 간헐천 모드이거나 두번째 기한 전이면 기한도 바꾸지 않는다.
    const ordinary = makeScene();
    ordinary.aux.ordinaryGeysers = 1;
    ordinary.aux.secondaryGeyserDeadline = 50;
    runBattleResourceFrame(ordinary.globals, ordinary.hooks);
    assert.equal(ordinary.aux.secondaryGeyserDeadline, 50);
    const early = makeScene();
    early.aux.secondaryGeyserDeadline = 100.001;
    runBattleResourceFrame(early.globals, early.hooks);
    assert.equal(early.aux.secondaryGeyserDeadline, 100.001);
    assert.deepEqual(early.trace, []);
    // 기한이 되면(>=) 주기 만큼 옮기고, 보충 조건이 안 되면 거기서 끝난다. 주기는 단독이면 30, 아니면 옵션 +0x13 선택이다.
    const cases = [
        ['보스 아님', (scene) => { scene.globals.boss = { value: false }; }, 60],
        ['편집', (scene) => { scene.globals.editor = { value: 1 }; }, 60],
        ['주기 0', (scene) => { scene.options[19] = 0; }, 0],
        ['미로그인', (scene) => { scene.players[2 * 180 + 0x50] = 0; }, 60]
    ];
    for (const [label, mutate, advance] of cases) {
        const scene = makeScene();
        scene.aux.secondaryGeyserDeadline = 90;
        scene.options[19] = 2;
        loginAll(scene);
        mutate(scene);
        runBattleResourceFrame(scene.globals, scene.hooks);
        // 새 기한은 이전 기한(90)이 아니라 현재 시각(100)에 주기를 더한 값이다.
        assert.equal(scene.aux.secondaryGeyserDeadline, 100 + advance, label);
        assert.equal(scene.trace.some((entry) => entry.startsWith('populate') || entry === 'predictable'), false, label);
    }
    // 단독이면 간격이 30초로 고정이다(옵션 선택과 무관, 옵션 주기 0 이어도 보충 관문은 옵션 주기를 다시 본다).
    const single = makeScene({ globals: { single: { value: true } } });
    single.aux.secondaryGeyserDeadline = 100;
    single.options[19] = 4;
    loginAll(single);
    single.hooks.typeCost = () => 1;
    single.aux.resourcePool = 10;
    runBattleResourceFrame(single.globals, single.hooks);
    assert.equal(single.aux.secondaryGeyserDeadline, 130);
});

test('전체 한 개 보충: 예측 가능이면 0xbeef 난수, 아니면 벽시각 seed 로 TYPE 비용이 자원보다 작을 때 배치한다', () => {
    const scene = makeScene();
    scene.aux.secondaryGeyserDeadline = 100;
    scene.options[19] = 1;
    scene.options[18] = 1;
    scene.aux.resourcePool = 10;
    loginAll(scene);
    runBattleResourceFrame(scene.globals, scene.hooks);
    assert.deepEqual(scene.trace.filter((entry) => !entry.startsWith('players')), ['predictable', 'typeCost', 'whole', 'populate,1,0']);
    assert.equal(scene.trace.populated.random.state, 0xbeef);
    assert.deepEqual(scene.trace.populated.placement, { excludePlayers: 0, allowLand: 1, allowVoid: 1 });
    // 예측 불가능이면 벽시각으로 seed 를 만든다.
    const live = makeScene({ hooks: {} });
    live.hooks.predictable = () => { live.trace.push('predictable'); return false; };
    live.aux.secondaryGeyserDeadline = 100;
    live.options[19] = 1;
    live.options[18] = 1;
    live.aux.resourcePool = 10;
    loginAll(live);
    runBattleResourceFrame(live.globals, live.hooks);
    assert.equal(live.trace.populated.random.state, RawFortFinishOwner.timeSeed(777.5));
    assert.equal(live.trace.includes('wall'), true);
    // 비용이 자원 이상이면(float 비교) 예산/3 목표를 넘는 개수일 때만 끝낸다.
    const costly = (cost, resource, count, budget) => {
        const run = makeScene();
        run.aux.secondaryGeyserDeadline = 100;
        run.options[19] = 1;
        run.options[18] = 1;
        run.aux.resourcePool = resource;
        run.hooks.typeCost = () => cost;
        run.hooks.geyserCount = () => count;
        run.hooks.budget = () => budget;
        loginAll(run);
        runBattleResourceFrame(run.globals, run.hooks);
        return run.trace.some((entry) => entry.startsWith('populate'));
    };
    assert.equal(costly(5, 10, 99, 3), true);
    assert.equal(costly(10, 10, 0, 90), true);
    assert.equal(costly(10, 10, 29, 90), true);
    assert.equal(costly(10, 10, 30, 90), false);
    assert.equal(costly(10, 10, 31, 90), false);
    assert.equal(costly(10, 10, 0xffffffff, 90), true);
    assert.equal(costly(16777216, 16777217, 99, 90), false);
    assert.equal(costly(-5, 0, 99, 90), true);
    // 예산 87 은 87/3=29, 89 도 29, 90 은 30 이다(0 방향 절삭).
    assert.equal(costly(10, 10, 29, 89), false);
    assert.equal(costly(10, 10, 29, 87), false);
    assert.equal(costly(10, 10, 28, 87), true);
    // 비용은 float 로 비교한다: 16777215.5 는 float 로 16777216 이 되어 자원 16777216 보다 작지 않다(예산/3 목표를 넘는 개수면 끝).
    assert.equal(costly(16777215.5, 16777216, 99, 90), false);
    // 음수 예산의 목표는 0 방향 절삭이다(-4/3 → -1): signed -2 개는 목표 미만이라 배치한다.
    assert.equal(costly(10, 10, 0xfffffffe, -4), true);
    assert.equal(costly(10, 10, 0xffffffff, -4), false);
});

test('보충 모드 선택: 단독·비전투는 옵션 18 과 무관하게 전체 한 개로 가고, 전투에서는 1 이 아니면 끝난다', () => {
    const run = (mutate) => {
        const scene = makeScene();
        scene.aux.secondaryGeyserDeadline = 100;
        scene.options[19] = 1;
        scene.aux.resourcePool = 10;
        loginAll(scene);
        mutate(scene);
        runBattleResourceFrame(scene.globals, scene.hooks);
        return scene.trace.filter((entry) => entry.startsWith('populate'));
    };
    assert.deepEqual(run((scene) => { scene.options[18] = 0; scene.globals.single = { value: true }; }), ['populate,1,0']);
    assert.deepEqual(run((scene) => { scene.options[18] = 3; scene.globals.battle = { value: false }; }), ['populate,1,0']);
    assert.deepEqual(run((scene) => { scene.options[18] = 1; }), ['populate,1,0']);
    assert.deepEqual(run((scene) => { scene.options[18] = 0; }), []);
    assert.deepEqual(run((scene) => { scene.options[18] = 3; }), []);
});

// 구역 모드(옵션 18 = 2)의 입력: 예산 100·밀도 2(+0.2)·간격 6(+0.1)이면 보정 120, 합 130, corner=16, center=17 이므로 목표는 모서리 16·중앙 17·변 8 이다. (2026-11-15 WB04-E560)
function makeCellScene(counts, overrides = {}) {
    const scene = makeScene(overrides);
    scene.aux.secondaryGeyserDeadline = 100;
    scene.options[19] = 1;
    scene.options[18] = 2;
    loginAll(scene);
    scene.hooks.budget = () => { scene.trace.push('budget'); return 100; };
    scene.hooks.density = () => { scene.trace.push('density'); return 2; };
    scene.hooks.spacing = () => { scene.trace.push('spacing'); return 6; };
    scene.hooks.count = (rectangle) => { scene.trace.push(`count,${rectangle.left}`); return counts[rectangle.left]; };
    return scene;
}
const TARGETS = [0, 16, 8, 16, 8, 17, 8, 16, 8, 16];

test('구역 보충: 목표보다 적은 구역이 하나면 난수를 쓰지 않고 부족분의 절반을 그 구역에 배치한다', () => {
    const counts = TARGETS.slice();
    counts[2] = 3;
    const scene = makeCellScene(counts);
    runBattleResourceFrame(scene.globals, scene.hooks);
    // 예산→밀도→간격 순으로 읽고 아홉 구역을 센 뒤 선택한 구역 범위를 다시 읽는다. 부족분 5 의 절반(0 방향) 2 개를 {1,1,1} 로 배치한다.
    const order = scene.trace.filter((entry) => entry !== 'players');
    assert.deepEqual(order, ['predictable', 'budget', 'density', 'spacing',
        ...Array.from({ length: 9 }, (_, index) => [`partition,${index + 1}`, `count,${index + 1}`]).flat(), 'partition,2', 'populate,2,2']);
    assert.deepEqual(scene.trace.populated.placement, { excludePlayers: 1, allowLand: 1, allowVoid: 1 });
    assert.equal(scene.trace.populated.random.state, 0xbeef);
    // 구역 모드는 배치 뒤 전체 한 개로 이어지지 않는다.
    assert.equal(scene.trace.filter((entry) => entry.startsWith('populate')).length, 1);
    assert.equal(scene.trace.includes('whole'), false);
});

test('구역 보충의 수량: 부족분 1 이하는 1 개, 그 이상은 절반이며 signed 음수 개수도 적은 구역이다', () => {
    const quantityFor = (cell, count) => {
        const counts = TARGETS.slice();
        counts[cell] = count;
        const scene = makeCellScene(counts);
        runBattleResourceFrame(scene.globals, scene.hooks);
        return scene.trace.filter((entry) => entry.startsWith('populate'));
    };
    assert.deepEqual(quantityFor(5, 16), [`populate,1,5`]);
    assert.deepEqual(quantityFor(5, 15), [`populate,1,5`]);
    assert.deepEqual(quantityFor(5, 14), [`populate,1,5`]);
    assert.deepEqual(quantityFor(5, 13), [`populate,2,5`]);
    assert.deepEqual(quantityFor(5, 12), [`populate,2,5`]);
    assert.deepEqual(quantityFor(1, 0), [`populate,8,1`]);
    assert.deepEqual(quantityFor(4, 0xffffffff), [`populate,4,4`]);
    assert.deepEqual(quantityFor(9, 15), [`populate,1,9`]);
    // 모든 구역이 목표 이상이면 아무 것도 배치하지 않고 끝난다.
    const counts = TARGETS.slice();
    const none = makeCellScene(counts);
    runBattleResourceFrame(none.globals, none.hooks);
    assert.equal(none.trace.some((entry) => entry.startsWith('populate') || entry === 'whole'), false);
});

test('구역 보충: 후보가 둘 이상이면 실제 난수 장치로 하나를 고르고 범위 밖 선택은 거부한다', () => {
    const counts = TARGETS.slice();
    counts[2] = 0;
    counts[7] = 0;
    counts[9] = 0;
    const picks = (choice) => {
        const scene = makeCellScene(counts);
        scene.hooks.choose = (count) => { scene.trace.push(`choose,${count}`); return choice; };
        runBattleResourceFrame(scene.globals, scene.hooks);
        return scene.trace;
    };
    // 후보는 원문 1..9 순서(2·7·9)이다.
    assert.equal(picks(0).includes('choose,3'), true);
    assert.deepEqual(picks(0).filter((entry) => entry.startsWith('populate')), ['populate,4,2']);
    assert.deepEqual(picks(1).filter((entry) => entry.startsWith('populate')), ['populate,8,7']);
    assert.deepEqual(picks(2).filter((entry) => entry.startsWith('populate')), ['populate,8,9']);
    const bad = makeCellScene(counts);
    bad.hooks.choose = () => 3;
    assert.throws(() => runBattleResourceFrame(bad.globals, bad.hooks), RangeError);
    assert.equal(bad.trace.some((entry) => entry.startsWith('populate')), false);
    // 후보가 하나면 난수 장치를 요구하지도 않는다. 둘 이상인데 없으면 명시 오류이다.
    const oneCounts = TARGETS.slice();
    oneCounts[3] = 0;
    const one = makeCellScene(oneCounts, { hooks: { choose: undefined } });
    one.hooks.choose = undefined;
    assert.doesNotThrow(() => runBattleResourceFrame(one.globals, one.hooks));
    const many = makeCellScene(counts);
    many.hooks.choose = undefined;
    assert.throws(() => runBattleResourceFrame(many.globals, many.hooks), /미연결/);
});

test('구역 목표 계산: 밀도·간격 보정과 두 번의 0 방향 정수화(중간 정수화가 결과를 바꾸는 입력 포함)', () => {
    // 구역 하나만 목표 미만이 되는 최소 개수를 찾는다. 목표 = 처음으로 그 구역이 후보에서 빠지는 개수이다.
    const targetsFor = (budget, density, spacing) => {
        const found = [];
        for (let cell = 1; cell <= 9; cell += 1) {
            let target = null;
            for (let probe = -30; probe <= 60 && target === null; probe += 1) {
                const counts = new Array(10).fill(1000);
                counts[cell] = probe;
                const run = makeCellScene(counts);
                run.hooks.budget = () => budget;
                run.hooks.density = () => density;
                run.hooks.spacing = () => spacing;
                runBattleResourceFrame(run.globals, run.hooks);
                if (!run.trace.some((entry) => entry.startsWith('populate'))) target = probe;
            }
            found.push(target);
        }
        return found;
    };
    // 밀도 0·간격 0: 보정 없이 100 → corner=12, center=13, 변 6.
    assert.deepEqual(targetsFor(100, 0, 0), [12, 6, 12, 6, 13, 6, 12, 6, 12]);
    // 밀도 2(+0.2)·간격 6(+0.1): 120+10=130 → corner=16, center=17, 변 8.
    assert.deepEqual(targetsFor(100, 2, 6), [16, 8, 16, 8, 17, 8, 16, 8, 16]);
    // 간격 4(-0.1)은 -10 → 90 → corner=11, center=12, 변 6. 알 수 없는 밀도(3)·간격(5)은 보정 0 이다.
    assert.deepEqual(targetsFor(100, 0, 4), [11, 6, 11, 6, 12, 6, 11, 6, 11]);
    assert.deepEqual(targetsFor(100, 3, 5), [12, 6, 12, 6, 13, 6, 12, 6, 12]);
    // 첫 정수화: 밀도 1 이면 1.1*98=107.8 을 107 로 자른 뒤 -19.6 을 더한 87.4 → 87 → corner=10(정수화를 한 번만 하면 88.2 → 11 이 된다).
    assert.deepEqual(targetsFor(98, 1, 3), [10, 5, 10, 5, 11, 5, 10, 5, 10]);
    // 예산 0 은 corner 0·center 1, 음수 예산은 0 방향 절삭이다(-100 → corner -12, center -11, 변 -5).
    assert.deepEqual(targetsFor(0, 0, 0), [0, 0, 0, 0, 1, 0, 0, 0, 0]);
    assert.deepEqual(targetsFor(-100, 0, 0), [-12, -5, -12, -5, -11, -5, -12, -5, -12]);
});

// 두 번째 변형 확인에서 드러난 경계: 읽기 전용 전역의 함수 형태, Player 물리 크기, 로그인 판정의 전체 순회, 구독자 DWORD 의 높은 BYTE, 예측 가능 설정 장치 필수. (2026-11-15 WB04-E560)
test('읽기 전용 전역의 함수·단순 값 형태는 반환값을 읽는다', () => {
    const scene = makeScene({ globals: { boss: () => false, battle: () => true, now: () => 100, loginEnd: () => 0, single: () => false, editor: () => 0, owner: () => 6 } });
    scene.aux.spellCount = 1;
    scene.aux.spellDeadline = 90;
    scene.aux.geyserDeadline = 100;
    scene.aux.incomePrimed = 1;
    scene.aux.ordinaryGeysers = 1;
    scene.options[20] = 1;
    loginAll(scene);
    runBattleResourceFrame(scene.globals, scene.hooks);
    // 보스가 아니므로 보충도 지급도 없고(함수가 false 를 돌려준다), 스펠만 소비한다.
    assert.deepEqual(scene.trace.filter((entry) => entry !== 'players'), []);
    assert.equal(scene.aux.spellCount, 0);
    assert.equal(scene.aux.geyserDeadline, 100);
    // 함수 형태의 현재 owner 와 단순 값 형태의 시각도 읽는다.
    const guest = makeScene({ globals: { boss: () => true, owner: () => 6, now: 100, battle: true } });
    guest.aux.geyserDeadline = 90;
    guest.aux.incomePrimed = 1;
    guest.aux.ordinaryGeysers = 1;
    guest.options[20] = 1;
    loginAll(guest, [2]);
    runBattleResourceFrame(guest.globals, guest.hooks);
    assert.deepEqual(guest.trace.filter((entry) => entry.startsWith('income')), ['income,2,6,500']);
});

test('Player 크기·로그인 판정 순회: 1620바이트가 아니면 거부하고 첫 미로그인 행에서 멈추지 않고 8행을 모두 읽는다', () => {
    for (const size of [0, 9 * 180 - 1, 9 * 180 + 1]) {
        const scene = makeScene();
        scene.aux.geyserDeadline = 90;
        scene.options[20] = 1;
        scene.globals.players = () => new Uint8Array(size);
        assert.throws(() => runBattleResourceFrame(scene.globals, scene.hooks), RangeError);
    }
    // 첫 구독자가 미로그인이어도 나머지 행을 계속 읽는다(현재 base 재조회 8회).
    const scene = makeScene();
    scene.aux.geyserDeadline = 90;
    scene.aux.ordinaryGeysers = 1;
    scene.options[20] = 1;
    addSubscriber(scene, 1, 0);
    addSubscriber(scene, 2, 2);
    runBattleResourceFrame(scene.globals, scene.hooks);
    assert.equal(scene.trace.filter((entry) => entry === 'players').length, 8);
    assert.equal(scene.aux.geyserDeadline, 90);
});

test('구독자 DWORD: 네 BYTE 중 어느 하나라도 0 이 아니면 구독자로 본다', () => {
    for (const byte of [0x2c, 0x2d, 0x2e, 0x2f]) {
        const scene = makeScene();
        scene.aux.geyserDeadline = 90;
        scene.aux.incomePrimed = 1;
        scene.aux.ordinaryGeysers = 1;
        scene.options[20] = 1;
        scene.players[2 * 180 + byte] = byte === 0x2f ? 0x80 : 1;
        scene.players[2 * 180 + 0x50] = 2;
        runBattleResourceFrame(scene.globals, scene.hooks);
        assert.deepEqual(scene.trace.filter((entry) => entry.startsWith('income')), ['income,2,1,500'], `BYTE ${byte}`);
    }
});

test('예측 가능 설정 장치는 일반 간헐천 보충에서 필수이고 누락은 난수를 만들기 전에 거부한다', () => {
    const scene = makeScene({ hooks: { predictable: undefined } });
    scene.aux.secondaryGeyserDeadline = 90;
    scene.options[19] = 1;
    loginAll(scene);
    assert.throws(() => runBattleResourceFrame(scene.globals, scene.hooks), /미연결/);
    assert.equal(scene.aux.secondaryGeyserDeadline, 130);
});

test('구역 목표의 32비트 경계: 큰 예산은 두 정수화에서 signed 로 감기고 개수·부족분도 signed 이다', () => {
    const populates = (budget, density, spacing, cell, count) => {
        const counts = new Array(10).fill(0x7fffffff);
        counts[cell] = count;
        const scene = makeCellScene(counts);
        scene.hooks.budget = () => budget;
        scene.hooks.density = () => density;
        scene.hooks.spacing = () => spacing;
        runBattleResourceFrame(scene.globals, scene.hooks);
        return scene.trace.filter((entry) => entry.startsWith('populate'));
    };
    // 예산 2147483647·밀도 2: 보정 2576980376 이 첫 정수화에서 -1717986920 으로 감겨 corner=-214748365 이다.
    assert.deepEqual(populates(2147483647, 2, 0, 1, -214748365), []);
    assert.deepEqual(populates(2147483647, 2, 0, 1, -214748366), ['populate,1,1']);
    // 예산 20억·간격 6: 합 22억이 두번째 정수화에서 감겨 corner=-261870912 이다.
    assert.deepEqual(populates(2000000000, 0, 6, 1, -261870912), []);
    assert.deepEqual(populates(2000000000, 0, 6, 1, -261870913), ['populate,1,1']);
    // 개수가 signed -2^31 이면 부족분 target-count 가 감겨 음수가 되므로 1 개만 배치한다.
    assert.deepEqual(populates(100, 2, 6, 5, 0x80000000), ['populate,1,5']);
    // 개수 0xffffffff 는 signed -1 이다.
    assert.deepEqual(populates(100, 2, 6, 5, 0xffffffff), ['populate,9,5']);
});
