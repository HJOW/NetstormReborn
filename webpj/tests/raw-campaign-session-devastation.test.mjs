// Session 상태 주문의 실제 TYPE·World와 펜스 진단 재조회 계약이다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { campaignOwners } from './campaign-session-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { dispatchCampaignFence } from '../www/core/raw-campaign-fence-dispatch.js';
import { FENCE_TYPE_OPERATION } from '../www/core/raw-fence-type-parent.js';
import { BOMB_TREASON_ROLE } from '../www/core/raw-bomb-treason.js';

// 같은 실제 기반 owner에서 저장 미션을 Load하지 않고 이동까지 설치한다.
function scene() {
    const o = campaignOwners({ carrierRequest: () => {}, deletion: { selected: () => 0 } });
    try {
        o.connectWorldInstall({ sections: Array.from({ length: 35 }, () => new Uint8Array(0)) });
        o.connectOutcome(); o.connectAiBrains(); o.connectAiWorld(); o.connectAiProduction(); o.connectBuilding(); o.connectMotion();
        return o;
    } catch (error) { o.dispose(); throw error; }
}

// 실제 자산 번호와 상태 주문을 같은 Session에 설치한다.
test('Session 파괴 주문 앞단: 실제 TYPE 표와 상태 자격', () => {
    const early = campaignOwners();
    try { assert.throws(() => early.connectDevastationStatus(), /순서/); } finally { early.dispose(); }
    const o = scene(); let hooks;
    try {
        o.connectDevastationStatus(); hooks = o.devastationModules.statusHooks;
        assert.equal(o.devastationRegularTypes.length, 188); assert.equal(o.devastationEffectTypes.length, 188);
        assert.equal(o.devastationRegularTypes[82].firstFrame, o.core.frames[82].run('A').first);
        assert.equal(o.devastationRegularTypes[82].castTime, o.carrierRegularModules.types[82].castTime);
        assert.equal(o.devastationEffectTypes[82].cost, o.core.types[82].cost); assert.equal(o.bombStatusState.alliances, o.alliances);
        const parent = o.world.create(117), altar = o.world.create(164);
        assert.equal(o.bombStatus.mayTarget(parent, altar), true); assert.equal(o.loaded, false);
        assert.throws(() => o.connectDevastationStatus(), /순서/);
    } finally { o.dispose(); }
    assert.throws(() => hooks.refresh(0), /반납/);
});

// 진단 callback이 바꾼 물리 슬롯을 재조회해 두 펜스 작업에 동일하게 전달한다.
test('Session 펜스 분배: 진단 뒤 현재 슬롯 재조회', () => {
    const pool = new SidPool(32768, { server: true }), sid = pool.allocate(2), raw = pool.allocated(sid), calls = [];
    const owner = { core: { pool }, gameplayGlobals: { gameAux: () => ({ sunFenceType: 136 }) }, state: { gunState: { checking: 1 } },
        services: { ai: { assertion: () => { calls.push('assert'); raw[10] = 136; } } },
        fenceRebuild: { rebuild: (value) => calls.push(['rebuild', value]) }, fenceLifetime: { refresh: (value) => calls.push(['refresh', value]) } };
    raw[10] = 2; dispatchCampaignFence(owner, sid, FENCE_TYPE_OPERATION.rebuild);
    raw[10] = 2; dispatchCampaignFence(owner, sid, FENCE_TYPE_OPERATION.refresh);
    assert.deepEqual(calls, ['assert', ['rebuild', sid], 'assert', ['refresh', sid]]);
});

// 일반 다리 대상에서 세 주문의 같은 풀과 현재 역할·효과 입력을 검증한다.
test('Session 파괴 효과: 트리즌·힐·하드너의 현재 World 연결', () => {
    const o = scene(); let treason, heal;
    try {
        o.connectDevastationStatus();
        assert.throws(() => o.connectDevastationEffects(), /순서/);
        o.connectDevastationSummons(); o.connectDevastationEffects();
        assert.equal(o.devastationModules.effectsReady, true);
        treason = o.devastationModules.treasonHooks; heal = o.devastationModules.healHooks;
        const pool = o.core.pool, target = pool.allocate(2), raw = pool.allocated(target);
        raw[10] = 82;
        assert.equal(o.bombTreason.pool, pool); assert.equal(o.bombHeal.pool, pool); assert.equal(o.bombHardener.pool, pool);
        assert.equal(o.bombHeal.mayTarget(target), (o.core.types[82].flags2 & 0x81574204) >>> 0);
        assert.equal(o.bombHardener.mayTarget(target), (o.core.types[82].flags2 & 4) >>> 0);
        assert.equal(heal.hasHitPoints(target), o.hp.hasHitPoints(target));
        const aux = o.gameplayGlobals.gameAux(); aux.emptyGeyserType = 85;
        assert.equal(treason.role(BOMB_TREASON_ROLE.EmptyGeyser), 85);
        o.healBurstType = 177; o.state.gunTargetState.sunFenceType = 137;
        assert.equal(heal.burstType(), 177); assert.equal(heal.fenceType(), 137);
        assert.equal(o.loaded, false); assert.throws(() => o.connectDevastationEffects(), /순서/);
    } finally { o.dispose(); }
    assert.throws(() => treason.role(BOMB_TREASON_ROLE.Flare), /반납/);
    assert.throws(() => heal.burstType(), /반납/);
});

// 주문을 발동하지 않고 실제 장기 효과 TYPE와 카메라의 현재 값을 확인한다.
test('Session 직접 피해 주문: 현재 효과 TYPE와 카메라 연결', () => {
    const o = scene(); let state, hooks;
    try {
        o.connectDevastationStatus(); o.connectDevastationSummons();
        assert.throws(() => o.connectDevastationStrike(), /순서/);
        o.connectDevastationEffects(); o.connectDevastationStrike();
        state = o.devastationModules.strikeState; hooks = o.devastationModules.strikeHooks;
        assert.equal(o.devastationModules.strikeReady, true); assert.equal(o.bombStrike.pool, o.core.pool);
        const aux = o.gameplayGlobals.gameAux(); aux.gravityEffectType = 175; aux.meteorEffectType = 178;
        assert.equal(state.firstEffectType, 175); assert.equal(state.hailEffectType, 178);
        o.gameplayGlobals.camera().snapshot().setPixels(16, 11);
        assert.deepEqual(hooks.project(2, 3), [16, 22]);
        assert.equal(o.loaded, false); assert.throws(() => o.connectDevastationStrike(), /순서/);
    } finally { o.dispose(); }
    assert.throws(() => hooks.project(2, 3), /반납/);
    assert.throws(() => state.firstEffectType, /반납/);
});

// 상태 주문의 자격만 확인하며 현재 TYPE 사거리와 같은 폭발 좌표 장치를 사용한다.
test('Session 주문 표적: 현재 사거리와 상태 자격 분배', () => {
    const o = scene(); let hooks;
    try {
        o.connectDevastationStatus(); o.connectDevastationSummons(); o.connectDevastationEffects();
        assert.throws(() => o.connectDevastationTargets(), /순서/);
        o.connectDevastationStrike(); o.connectDevastationTargets(); hooks = o.devastationModules.targetHooks;
        assert.equal(o.devastationModules.targetsReady, true);
        assert.equal(o.devastationTargets.pool, o.core.pool); assert.equal(o.devastationBlast.pool, o.core.pool);
        const parent = o.world.create(117), altar = o.world.create(164);
        o.core.types[117].range = 5.75; assert.equal(hooks.radius(parent, 0x502510), 5);
        o.core.types[117].range = 8.25; assert.equal(hooks.radius(parent, 0x502510), 8);
        assert.equal(hooks.eligible(parent, 0x502510, altar), true);
        assert.throws(() => hooks.radius(parent, 0), /표 오류/);
        assert.throws(() => hooks.eligible(parent, 0, altar), /표 오류/);
        assert.equal(o.loaded, false); assert.throws(() => o.connectDevastationTargets(), /순서/);
    } finally { o.dispose(); }
    assert.throws(() => hooks.radius(0, 0x502510), /반납/);
});
