// 커서 앞단의 현재 상태와 Form 수명 분배를 검사한다. 유닛 생성·포획·희생은 실행하지 않는다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { campaignOwners } from './campaign-session-support.mjs';
import { SquidForms } from '../www/core/squid-forms.js';

// 실제 선행 설치를 수행하되 저장 미션과 게임은 시작하지 않는다.
function scene(extra = {}) {
    const o = campaignOwners({ carrierRequest: () => {}, deletion: { selected: () => 0 }, ...extra });
    try {
        o.connectWorldInstall({ sections: Array.from({ length: 35 }, () => new Uint8Array(0)) });
        o.connectOutcome(); o.connectAiBrains(); o.connectAiWorld(); o.connectAiProduction(); o.connectBuilding(); o.connectMotion(); o.connectDevastation(); o.connectCarrierDrop(); o.connectPriestPrayer(); o.connectBuilder();
        return o;
    } catch (error) { o.dispose(); throw error; }
}

// 현재 설정과 게임 전역을 실행 순간에 읽고 공통 Host 함수를 설치한다.
test('Session 커서 앞단: 현재 설정·게임 상태·Host 및 반납', () => {
    let option = 7; const diagnostics = [], sounds = [];
    const o = scene({ production: { effects: { configInt: (name) => { assert.equal(name, 'useProductTimers'); return option; } } }, ai: { assertion: (expression) => diagnostics.push(expression) }, sound: { play: (...args) => sounds.push(args) } });
    let game;
    try {
        assert.throws(() => o.connectCursorFront(), /순서/); o.connectGameplayPlacement(); o.connectCursorFront(); game = o.cursorModules.game;
        assert.equal(o.cursorModules.frontReady, true); assert.equal(o.cursor, null); assert.equal(o.loaded, false); assert.throws(() => o.connectCursorFront(), /중복/);
        assert.equal(game.productTimers(), 7); option = 9; assert.equal(game.productTimers(), 9); o.battleOptions[2] = 3; assert.equal(game.unitRate(), 3);
        o.playerState.battleOwner = 4; o.globals.missionPost = 1; game.prepare();
        assert.equal(o.cursorDrop.localPlayer, 4); assert.equal(o.cursorDrop.blocked, 1); assert.equal(o.state.gameplayPlacement.geometry.patternTypes, o.state.constructionState.confirm.patternTypes);
        game.lifecycle.assertion('진단', '', 5); assert.deepEqual(diagnostics, ['진단']); game.playDrop(); assert.equal(sounds.length, 1);
        for (const name of ['createMana', 'createRange', 'removeProcess', 'processEvent']) assert.equal(typeof game.lifecycle[name], 'function');
    } finally { o.dispose(); }
    assert.throws(() => game.prepare(), /반납/); assert.throws(() => game.lifecycle.createMana(1, 0), /반납/);
});

// TYPE이 바뀌어도 원본 가상표로 Form과 월드 객체의 분리·복귀를 고른다.
test('Session 커서 앞단: 현재 가상표에 따른 Form과 월드 분배', () => {
    const o = scene(), calls = [];
    try {
        o.connectGameplayPlacement(); o.connectCursorFront(); const life = o.cursorModules.game.lifecycle, c = o.core;
        const sid = c.pool.allocate(2), raw = c.pool.allocated(sid), forms = new SquidForms(c.pool); forms.constructVtableAt(6, sid); raw[10] = 75;
        const oldUnpopForm = c.factory.unpopForm, oldPopForm = c.factory.popForm, oldUnpop = o.world.unpop, oldPop = o.world.pop;
        try {
            c.factory.unpopForm = (...args) => calls.push(['form-detach', ...args]); c.factory.popForm = (...args) => calls.push(['form-pop', ...args]);
            o.world.unpop = (...args) => calls.push(['world-detach', ...args]); o.world.pop = (...args) => calls.push(['world-pop', ...args]);
            life.detach(sid, 8); life.unpop(sid, 9); new DataView(raw.buffer, raw.byteOffset, raw.byteLength).setUint32(0, 0, true);
            const view = new DataView(raw.buffer, raw.byteOffset, raw.byteLength); view.setFloat32(14, 3.5, true); view.setFloat32(18, 4.25, true);
            life.detach(sid, 10); life.unpop(sid, 11);
            assert.deepEqual(calls, [['form-detach', sid], ['form-pop', sid, 9], ['world-detach', sid, 10], ['world-pop', sid, 3.5, 4.25, 11]]);
        } finally { c.factory.unpopForm = oldUnpopForm; c.factory.popForm = oldPopForm; o.world.unpop = oldUnpop; o.world.pop = oldPop; }
    } finally { o.dispose(); }
});
