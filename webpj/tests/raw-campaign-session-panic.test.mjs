// 같은 실제 Session의 공황 연결과 지형·난수·수명 계약을 검사한다(WB04-E644).
import test from 'node:test';
import assert from 'node:assert/strict';
import { campaignOwners } from './campaign-session-support.mjs';
import { RawCampaignSessionPanic } from '../www/core/raw-campaign-session-panic.js';
import { GameRandom } from '../www/core/random.js';
import { put } from './raw-support.mjs';

// 실제 이동 설치 순서를 사용하며 게임 Load는 수행하지 않는다.
function scene() {
    const o = campaignOwners();
    try {
        o.connectWorldInstall({ sections: Array.from({ length: 35 }, () => new Uint8Array(0)) });
        o.connectOutcome(); o.connectAiBrains(); o.connectAiWorld(); o.connectAiProduction(); o.connectBuilding();
        o.connectMotionArrivals(); o.connectMotionInputs(); o.connectCarrierRouting(); o.connectMotionRuntime(); o.connectPriestPanic();
        return o;
    } catch (error) { o.dispose(); throw error; }
}

test('사제 공황: 설치 순서·반복·같은 Pool과 반납 관문', () => {
    const early = campaignOwners(), pending = new RawCampaignSessionPanic(early);
    try { assert.throws(() => pending.bind(), /순서/); } finally { early.dispose(); pending.dispose(); }
    const o = scene(), module = o.panicModules, hook = module.hooks.land;
    try {
        assert.equal(o.priestPanic.pool, o.core.pool); assert.equal(o.loaded, false);
        assert.throws(() => o.connectPriestPanic(), /순서/);
    } finally { o.dispose(); }
    assert.throws(() => hook(0, 0), /반납/);
});

test('사제 공황: 같은 지형 해시·좌표 절삭·실제 공유 난수', () => {
    const o = scene();
    try {
        const h = o.panicModules.hooks, surface = o.world.create(o.terrainType);
        o.core.hash.setCell(0, 12, 23, surface);
        assert.equal(h.land(12.9, 23.8), true); assert.equal(h.land(-1.1, 23), false); assert.equal(h.land(256, 23), false);
        assert.throws(() => h.land(Infinity, 0), /CRT 범위/);
        const reference = new GameRandom(o.random.state), expected = reference.next(65536) / 65536 * 40 - 20;
        assert.equal(h.random(-20, 20), expected); assert.equal(o.random.state, reference.state);
        assert.equal(h.region(12.2, 23.2), o.motion.terrain.regionAt(13, 24));
    } finally { o.dispose(); }
});

test('사제 공황: 부상 신전 복귀는 실제 본체와 현재 속도표를 공유한다', () => {
    const o = scene();
    try {
        const priest = o.world.create(158), temple = o.world.create(80), raw = o.core.pool.allocated(priest);
        put(raw, 26, 1); raw[34] = 1; o.core.pool.allocated(temple)[34] = 1;
        const speed = o.dudeSpeeds[158].speed, seen = [];
        o.motion.priestArrival.templeAt = () => temple;
        o.carrierTarget.startTarget = (...args) => { seen.push([args, o.dudeSpeeds[158].speed]); return 1; };
        o.motionModules.effects.priest.neutralReturn(priest);
        assert.deepEqual(seen, [[[priest, temple, 0, 0], Math.fround(speed + speed)]]);
        assert.equal(o.dudeSpeeds[158].speed, Math.trunc(speed));
    } finally { o.dispose(); }
});
