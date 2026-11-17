// 게임 배치의 실제 Session 상태와 파이프라인 결선을 확인한다. 유닛 생성·포획·희생은 실행하지 않는다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { campaignOwners } from './campaign-session-support.mjs';

// 실제 선행 설치를 수행하되 저장 미션과 게임을 시작하지 않는다.
function scene() {
    const o = campaignOwners({ carrierRequest: () => {}, deletion: { selected: () => 0 } });
    try {
        o.connectWorldInstall({ sections: Array.from({ length: 35 }, () => new Uint8Array(0)) });
        o.connectOutcome(); o.connectAiBrains(); o.connectAiWorld(); o.connectAiProduction(); o.connectBuilding(); o.connectMotion(); o.connectDevastation(); o.connectCarrierDrop(); o.connectPriestPrayer(); o.connectBuilder();
        return o;
    } catch (error) { o.dispose(); throw error; }
}

// 판정은 실제 파이프라인을 통과하고 호출할 때 현재 소유자·동맹·Graph·섬을 반영한다.
test('Session 게임 배치: 실제 파이프라인과 현재 상태를 매 호출 연결한다', () => {
    const o = scene(), state = o.state.gameplayPlacement, anchor = state.anchor;
    try {
        assert.throws(() => o.constructionModules.mayPlace({}), /미설치/);
        o.connectGameplayPlacement(); assert.equal(o.gameplayPipeline.pool, o.core.pool); assert.notEqual(o.gameplayPipeline, o.pipeline);
        assert.equal(state.permission.useAlliances, 1); assert.throws(() => o.connectGameplayPlacement(), /중복/);
        o.gameplayGlobals.gameAux().missionSettings()[1] = 1; o.playerState.battleOwner = 2; o.alliances[3] = 7;
        o.state.anchorState.graphReady = 1; o.state.anchorState.invalidGraph = 253; o.fullDeck = 19;
        const raw = new DataView(o.islandBytes.buffer, o.islandBytes.byteOffset, o.islandBytes.byteLength); raw.setUint32(4, 13, true); raw.setUint32(8, 29, true);
        const query = { type: 82, argument: 82, x: 20, y: 20, flags: 0, owner: 2, mode: 0 };
        assert.equal(o.constructionModules.mayPlace(query), true);
        assert.equal(state.placement.localPlayer, 2); assert.equal(state.permission.alliances, o.alliances); assert.equal(state.preview.alliances[3], 7);
        assert.equal(state.anchor, anchor); assert.notEqual(state.anchor, o.state.anchorState); assert.equal(anchor.graphReady, 1); assert.equal(anchor.invalidGraph, 253); assert.equal(anchor.currentIgnoreRestrictions, 19);
        assert.deepEqual(o.gameplayRegions[0], { restricted: 13, exists: 29 });
        o.state.anchorState.invalidGraph = 252; o.fullDeck = 20; raw.setUint32(4, 31, true); o.playerState.battleOwner = 3;
        assert.equal(o.constructionModules.mayPlace(query), true); assert.equal(anchor.invalidGraph, 252); assert.equal(anchor.currentIgnoreRestrictions, 20);
        assert.equal(o.gameplayRegions[0].restricted, 31); assert.equal(state.placement.localPlayer, 3); assert.equal(o.loaded, false);
    } finally { o.dispose(); }
    assert.throws(() => o.constructionModules.mayPlace({}), /반납/); assert.throws(() => o.connectGameplayPlacement(), /반납/);
});
