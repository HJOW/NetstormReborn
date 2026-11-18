// 캠페인 Start Client 장치의 선택 순서와 현재 대상 재조회를 검사한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { makeCampaignMissionStartEffects } from '../www/core/campaign-client-services.js';

// UI·시계·World 대상은 각 효과가 호출되는 시점의 실제 공급자에서 다시 조회한다.
test('캠페인 Client Start 효과: 현재 시계·화면·World 경계', () => {
    const calls = [], clock = { game: 12.5 }, menu = { refresh: () => calls.push('menu') };
    const client = { time: () => clock, renderer: () => ({ invalidateAll: () => calls.push('renderer') }), menu: () => menu };
    const world = { session: () => ({ generateWorldGeysers: () => calls.push('geysers') }), resetCampaignCamera: () => calls.push('camera') };
    assert.throws(() => makeCampaignMissionStartEffects(client, world, null), /명령 Gump 조회 미연결/);
    const effects = makeCampaignMissionStartEffects(client, world, () => calls.push('gump')), bytes = new Uint8Array(600).fill(0xa7);
    assert.deepEqual(calls, []); effects.settingsBytes(3, bytes); assert.ok(bytes.every((value) => value === 0xa7));
    assert.equal(effects.gameSeconds(), 12.5); clock.game = 14.5; assert.equal(effects.gameSeconds(), 14.5);
    effects.refreshSp(); effects.refreshCommandGump(); effects.randomGeysers(); effects.resetCamera();
    assert.deepEqual(calls, ['renderer', 'menu', 'gump', 'geysers', 'camera']);
    client.menu = () => null; effects.refreshSp(); assert.deepEqual(calls.slice(-1), ['renderer']);
});
