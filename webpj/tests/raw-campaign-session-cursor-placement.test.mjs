// 커서의 실제 Session 배치 함수 표를 검사한다. 유닛 생성·포획·희생은 실행하지 않는다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { campaignOwners } from './campaign-session-support.mjs';
import { projectCameraCoordinates } from '../www/render/camera-view.js';

// 앞단까지 실제 설치 순서를 사용하며 저장 미션과 게임은 시작하지 않는다.
function scene() {
    const o = campaignOwners({ carrierRequest: () => {}, deletion: { selected: () => 0 } });
    try {
        o.connectWorldInstall({ sections: Array.from({ length: 35 }, () => new Uint8Array(0)) });
        o.connectOutcome(); o.connectAiBrains(); o.connectAiWorld(); o.connectAiProduction(); o.connectBuilding(); o.connectMotion(); o.connectDevastation(); o.connectCarrierDrop(); o.connectPriestPrayer(); o.connectBuilder(); o.connectGameplayPlacement(); o.connectCursorFront();
        return o;
    } catch (error) { o.dispose(); throw error; }
}

// 설명 문자열·Graph·판정 결과·투영과 Builder가 같은 현재 장치를 사용한다.
test('Session 커서 배치: 현재 에너지·Graph·판정·카메라·Builder', () => {
    const o = scene(), module = o.cursorModules, calls = [];
    const energy = o.energySupply.evaluate, graph = o.anchor.graphFor, mayPlace = o.constructionModules.mayPlace, move = o.buildingMover.move;
    let placement;
    try {
        o.connectCursorPlacement(); const game = module.game; placement = game.placement; assert.equal(module.placementReady, true); assert.throws(() => o.connectCursorPlacement(), /중복/);
        o.energySupply.evaluate = (...args) => { calls.push(['energy', ...args]); return { output: '현재 자원', value: 3 }; };
        const text = { value: '기존' }; assert.equal(placement.describe(text, 1, 2, 82, 3), 3); assert.equal(text.value, '현재 자원');
        assert.deepEqual(calls.shift(), ['energy', '기존', 1, 2, 82, 3]);
        o.anchor.graphFor = () => 254; assert.equal(placement.region(1), 0); o.anchor.graphFor = () => 7; assert.equal(placement.region(1), 8);
        const query = { type: 82 }; o.constructionModules.mayPlace = (value) => { assert.equal(value, query); o.state.gameplayPlacement.placement.blockedRelation = 23; return true; };
        assert.throws(() => placement.mayPlace(query), /본체 미설치/); o.cursor = { placement: {} }; assert.equal(placement.mayPlace(query), 1); assert.equal(o.cursor.placement.obstruction, 23);
        const p = o.gameplayGlobals.camera().snapshot().pixels, expected = projectCameraCoordinates(p.x, p.y, 4.5, 8.5);
        assert.deepEqual(placement.project(4.5, 8.5), { x: expected[0], y: expected[1] }); assert.equal(placement.processEvent, game.lifecycle.processEvent);
        o.buildingMover.move = (...args) => calls.push(['builder', ...args]); game.moveBuilder(2, 3, 4, 5); assert.deepEqual(calls.pop(), ['builder', 2, 3, 4, 5]);
        assert.equal(o.loaded, false);
    } finally { o.cursor = null; o.energySupply.evaluate = energy; o.anchor.graphFor = graph; o.constructionModules.mayPlace = mayPlace; o.buildingMover.move = move; o.dispose(); }
    assert.throws(() => placement.project(0, 0), /반납/);
});
