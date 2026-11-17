// Session 운반 투하의 공통 명령·현재 진단 뷰와 실제 위치 앞단을 확인한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { campaignOwners } from './campaign-session-support.mjs';

// 같은 실제 기반 owner에서 게임 Load 없이 이동과 파괴 주문까지 설치한다.
function scene() {
    const o = campaignOwners({ carrierRequest: () => {}, deletion: { selected: () => 0 } });
    try {
        o.connectWorldInstall({ sections: Array.from({ length: 35 }, () => new Uint8Array(0)) });
        o.connectOutcome(); o.connectAiBrains(); o.connectAiWorld(); o.connectAiProduction(); o.connectBuilding(); o.connectMotion();
        return o;
    } catch (error) { o.dispose(); throw error; }
}

// 사제 포획이나 투하를 실행하지 않고 현재 상태와 선행 배치 경계를 검증한다.
test('Session 운반 투하 앞단: 공통 명령과 현재 지도·진단 뷰', () => {
    const o = scene(); let module;
    try {
        assert.throws(() => o.connectCarrierDropFront(), /순서/);
        o.connectDevastation(); o.connectCarrierDropFront(); module = o.carrierDropModules;
        assert.equal(module.ready, true); assert.equal(o.carrierDropPoint.pool, o.core.pool);
        assert.equal(o.objectCommand.command(0, 0), 0);
        o.playerState.battleOwner = 3; o.cursorManaGlobals.mapLimit = 96;
        const checking = o.gameplayGlobals.aiDebug().checking(); checking.value = 11;
        o.prepareCarrierDrop(); assert.equal(o.state.carrierDropState.localOwner, 3); assert.equal(module.pointState.mapLimit, 96);
        checking.value = 22; assert.equal(o.state.carrierDropState.checking, 22);
        o.state.gunState.checking = 0; checking.value = 33; assert.equal(o.state.carrierDropState.checking, 33);
        const sid = o.world.create(82, 2); assert.deepEqual(module.pointHooks.center(sid), o.gunTarget.center(sid));
        assert.throws(() => module.pointHooks.mayPlace({}), /MayPlace 미설치/);
        assert.equal(o.loaded, false); assert.throws(() => o.connectCarrierDropFront(), /순서/);
    } finally { o.dispose(); }
    assert.throws(() => module.prepare(), /반납/);
});
