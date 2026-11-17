// Session 사제 기도의 자산 표와 기존 Host·운반 부모 결선을 확인한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { campaignOwners } from './campaign-session-support.mjs';
import { typeNumber } from '../www/assets/type-table.js';
import { pathDirection } from '../www/core/raw-path-animation.js';

// 실제 Load 없이 같은 Session의 이동·주문과 운반 투하를 준비한다.
function scene() {
    const o = campaignOwners({ carrierRequest: () => {}, deletion: { selected: () => 0 } });
    try {
        o.connectWorldInstall({ sections: Array.from({ length: 35 }, () => new Uint8Array(0)) });
        o.connectOutcome(); o.connectAiBrains(); o.connectAiWorld(); o.connectAiProduction(); o.connectBuilding(); o.connectMotion(); o.connectDevastation();
        return o;
    } catch (error) { o.dispose(); throw error; }
}

// 사제 포획·희생이나 기도 효과를 실행하지 않고 메타데이터와 공통 부모만 검증한다.
test('Session 사제 기도: 실제 자산 표와 Host·운반 부모', () => {
    const o = scene(); let hooks;
    try {
        assert.throws(() => o.connectPriestPrayer(), /순서/); o.connectCarrierDrop(); o.connectPriestPrayer();
        const module = o.prayerModules; hooks = module.hooks;
        assert.equal(module.ready, true); assert.equal(o.priestPrayer.pool, o.core.pool); assert.equal(module.types.length, 188);
        const asset = o.assets.game.types().find((entry) => 70 + entry.block === 158);
        assert.equal(module.types[158].prayTime, Math.trunc(typeNumber(asset.definition, 'praytime') ?? 0));
        assert.equal(module.types[158].defaultFrame, asset.definition.defaultFrame);
        const parent = o.core.pool.allocate(2), raw = o.core.pool.allocated(parent); raw[10] = 82;
        assert.equal(hooks.hasEvent(parent, 605), false); assert.equal(hooks.direction(parent), pathDirection(o.core.frames[82], 0));
        assert.equal(o.priestPrayer.command(parent, 0, 0), 0);
        assert.throws(() => o.receivePriestPrayer(new Uint8Array(0)), /기도6d/);
        assert.equal(o.loaded, false); assert.throws(() => o.connectPriestPrayer(), /순서/);
    } finally { o.dispose(); }
    assert.throws(() => hooks.hasEvent(0, 605), /반납/);
});
