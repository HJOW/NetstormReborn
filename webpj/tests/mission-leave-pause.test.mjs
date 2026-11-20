// 미션을 떠날 때 미션 관문이 안내 창 때문에 건 게임 시계 정지를 되돌리는지 검사한다. (2026-11-21 WB04-E1132)
// 열린 안내 창(시계 정지)에서 미션을 떠나면 정지 깊이가 다음 미션으로 남아 새 미션의 시계가 멈춘 채 시작하던 결함을 막는다.
// 실제 앱 동작은 e2e/mission-leave-pause.spec.js 가 Chromium 에서 확인한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { Netstorm } from '../www/netstorm.js';

// `_leaveCampaignMission` 이 부르는 장치만 기록하는 가짜 앱을 만든다. gate 는 미션 관문 상태(scriptPaused)이다.
function makeApp(gate, { guide = true } = {}) {
    const calls = [];
    const app = {
        _campaignWorld: {},
        _clientGuide: guide ? { current: () => ({ runtime: { gate } }), close: () => calls.push('guide.close') } : null,
        _gameClock: { popGamePause: () => calls.push('pop') },
        _closeCampaignPopups: () => calls.push('popups'), _releaseCampaignWorld: () => calls.push('release'),
        _mainMenu: { setDialog: () => calls.push('setDialog'), dirty: false }, _ensureSurface: () => calls.push('surface'), _displayDirty: false, _applyCursor: () => calls.push('cursor'),
    };
    return { app, calls };
}

// 관문이 정지를 건 상태로 떠나면 한 단계만 풀고 플래그를 0으로 돌린 뒤 평소 정리를 한다.
test('미션 떠나기: 관문 정지가 남아 있으면 한 단계 풀고 플래그를 지운다', () => {
    const gate = { scriptPaused: 1 }, { app, calls } = makeApp(gate);
    assert.equal(Netstorm.prototype._leaveCampaignMission.call(app), true);
    assert.equal(gate.scriptPaused, 0);
    assert.deepEqual(calls.slice(0, 2), ['pop', 'popups']); assert.equal(calls.filter((name) => name === 'pop').length, 1);
    assert.ok(calls.includes('guide.close') && calls.includes('release'));
});

// 관문이 정지를 걸지 않았거나 Guide 가 없으면 시계를 건드리지 않는다.
test('미션 떠나기: 관문 정지가 없으면 시계를 건드리지 않는다', () => {
    const gate = { scriptPaused: 0 }, { app, calls } = makeApp(gate);
    assert.equal(Netstorm.prototype._leaveCampaignMission.call(app), true); assert.equal(calls.includes('pop'), false);
    const none = makeApp(null, { guide: false });
    assert.equal(Netstorm.prototype._leaveCampaignMission.call(none.app), true); assert.equal(none.calls.includes('pop'), false);
});

// 미션도 Guide 도 없으면 아무것도 하지 않는다.
test('미션 떠나기: 떠날 미션이 없으면 false 이고 아무 정리도 하지 않는다', () => {
    const gate = { scriptPaused: 1 }, { app, calls } = makeApp(gate);
    app._campaignWorld = null; app._clientGuide = { current: () => null, close: () => calls.push('guide.close') };
    assert.equal(Netstorm.prototype._leaveCampaignMission.call(app), false); assert.deepEqual(calls, []); assert.equal(gate.scriptPaused, 1);
});
