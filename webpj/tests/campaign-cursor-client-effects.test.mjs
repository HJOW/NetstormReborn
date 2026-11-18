// 커서 Gump 연결의 원본 명령·부모 공유와 팝업 종료 관문을 확인한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { bindCampaignCursorGumps } from '../www/core/campaign-cursor-client-effects.js';

// 실제 컨트롤 호출의 인자와 반환을 보존하고 기존 팝업 훅은 덮어쓰지 않는다.
test('캠페인 커서 Gump: 화면 조회·분배·조각과 팝업 연결', () => {
    const calls = [], effects = { placement: {}, drop: {}, lifecycle: {} };
    const controls = { pickGump: (point) => { calls.push(['pick', point]); return 7; }, dispatchGump: (...args) => { calls.push(['dispatch', ...args]); return 19; },
        gumpParentSid: (token) => { calls.push(['parent', token]); return 23; }, handleGump: (...args) => { calls.push(['handle', ...args]); return 41; }, requestClosePopups: () => calls.push('close') };
    bindCampaignCursorGumps(effects, controls); assert.equal(effects.placement.gumpAt(-2, 8), 7);
    assert.equal(effects.placement.accept(7, 8, 9, 10, 2), 19); assert.equal(effects.placement.accept(7, 8, 9, 10, 0), 19);
    assert.equal(effects.placement.parent, effects.drop.guiParent); assert.equal(effects.drop.guiParent(7), 23); assert.equal(effects.drop.guiFlags(7), 41);
    assert.equal(effects.drop.guiReceive(7, 123), undefined); effects.lifecycle.closePopups();
    assert.deepEqual(calls, [['pick', { x: -2, y: 8 }], ['dispatch', 7, 8, 9, 10, true], ['dispatch', 7, 8, 9, 10, false], ['parent', 7],
        ['handle', 7, 0x0200005f, 0, 0], ['handle', 7, 0x0200005e, 123, 0], 'close']);
    const existing = () => calls.push('existing'), other = { placement: {}, drop: {}, lifecycle: { closePopups: existing } };
    bindCampaignCursorGumps(other, controls); assert.equal(other.lifecycle.closePopups, existing); other.lifecycle.closePopups(); assert.equal(calls.at(-1), 'existing');
});
