// Session 상태 주문의 실제 TYPE·World와 펜스 진단 재조회 계약이다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { campaignOwners } from './campaign-session-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { dispatchCampaignFence } from '../www/core/raw-campaign-fence-dispatch.js';
import { FENCE_TYPE_OPERATION } from '../www/core/raw-fence-type-parent.js';

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
