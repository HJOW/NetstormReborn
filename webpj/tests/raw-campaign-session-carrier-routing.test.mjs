// 실제 Session의 현재 Board/Player/목록/TYPE/난수 결선 검사다(WB04-E642).
import test from 'node:test';
import assert from 'node:assert/strict';
import { campaignOwners } from './campaign-session-support.mjs';
import { RawCampaignSessionCarrierRouting } from '../www/core/raw-campaign-session-carrier-routing.js';
import { GameRandom } from '../www/core/random.js';
import { RawPathRuntime } from '../www/core/raw-path-runtime.js';
import { put } from './raw-support.mjs';

// 외부 음향/화면만 제공하고 기존 실제 생성자/건설/도착/이동 입력 순서로 조립한다.
function scene(bind = true) {
    const o = campaignOwners({ deletion: { selected: () => 0 }, construction: { refresh: () => {} }, sound: { play: (sound) => sound, playOnceAt: (_x, _y, sound) => sound, playLoopAt: (_x, _y, current, sound) => current || sound, stop: () => {}, isPlaying: () => 0 } });
    let routing = null;
    try {
        o.connectWorldInstall({ sections: Array.from({ length: 35 }, () => new Uint8Array(0)) }); o.connectOutcome(); o.connectAiBrains(); o.connectAiWorld(); o.connectAiProduction(); o.connectBuilding(); o.connectMotionArrivals(); o.connectMotionInputs();
        if (bind) { o.connectCarrierRouting(); routing = o.carrierRoutingModules; } else { routing = new RawCampaignSessionCarrierRouting(o); } return { o, routing };
    } catch (error) { o.dispose(); routing?.dispose(); throw error; }
}
// 부모의 실제 Host를 먼저 비우고 지역 함수 표의 대여를 닫는다.
function close(o, routing) { try { o.dispose(); } finally { routing.dispose(); } }

test('Session 운반 경로: 선행/반복/반납 관문과 같은 Pool·Storage 실패점/난수 불변', () => {
    const unready = campaignOwners(), early = new RawCampaignSessionCarrierRouting(unready);
    try { assert.throws(() => early.bind(), /초기화 전/); } finally { close(unready, early); }
    const { o, routing } = scene(false);
    try {
        const invalid = o.world.storage.boardTargetState.invalid, seed = o.random.state, bytes = o.core.pool.bytes(); invalid.x = -17.5; invalid.y = -23.25;
        routing.bind(); assert.equal(routing.ready, true); assert.deepEqual(invalid, { x: -17.5, y: -23.25 });
        for (const body of [o.carrierChoice, o.carrierTarget, o.carrierEscape, o.pathRetarget]) assert.equal(body.pool, o.core.pool); // 4개 실제 모듈은 같은 기존 Pool을 빌린다.
        assert.equal(o.random.state, seed); assert.deepEqual(o.core.pool.bytes(), bytes); assert.equal(o.loaded, false); assert.equal(o.motion, null);
        assert.throws(() => routing.bind(), /반복/);
    } finally { close(o, routing); }
    assert.throws(() => routing.choiceHooks.movement(0), /반납/); assert.throws(() => routing.startMotion({ parent: 0 }), /반납/);
});

test('Session 운반 후보: 현재 Player brain/귀환점·공유 목록과 같은 TYPE 이동 속도', () => {
    const { o, routing } = scene();
    try {
        const parent = o.world.create(133); o.core.owner.set(parent, 1); o.world.pop(parent, 32, 32);
        const e = o.motionModules.effects; assert.equal(e.carrier.chooseReturn(parent), 0); assert.equal(e.carrier.chooseGeyser(parent), 0);
        assert.deepEqual(routing.choiceHooks.movement(0), [0, 0]); const speed = routing.choiceHooks.movement(parent); assert.ok(speed[0] > 0 && speed[1] > 0); assert.equal(routing.escapeHooks.movement, routing.choiceHooks.movement);
        assert.equal(e.carrier.currentControlled(1), false); put(o.players, 180 + 0xb0, 0x12345678); assert.equal(e.carrier.currentControlled(1), true);
        assert.equal(o.carrierTarget.goReturn(parent, parent), 0); put(o.players, 180 + 0xb0, 0); assert.equal(e.carrier.currentControlled(1), false);
        assert.throws(() => routing.choiceHooks.validReturn(parent, parent), /현재 이동 설치 전/);
    } finally { close(o, routing); }
});

test('Session 운반 회피: 공유 난수·실제 현재 낙하 좌표 제한·미설치 경로 조회 거부', () => {
    const { o, routing } = scene();
    try {
        const h = routing.escapeHooks, expected = new GameRandom(o.random.state), seed = o.random.state;
        assert.throws(() => h.random(2, 2), /난수 구간/); assert.equal(o.random.state, seed); assert.equal(h.random(-20, 30), expected.next(50) - 20); assert.equal(o.random.state, expected.state);
        assert.equal(h.valid({ x: 0, y: 32 }), false); assert.equal(h.valid({ x: 32, y: 32 }), true); o.cursorManaGlobals.mapLimit = 31; assert.equal(h.valid({ x: 32, y: 32 }), false);
        assert.throws(() => h.path(0), /현재 이동 설치 전/); assert.equal(o.motionModules.effects.carrier.escape, o.services.motion.carrier.escape);
        assert.throws(() => routing.targetHooks.start({ parent: 0 }), /현재 이동 설치 전/);
    } finally { close(o, routing); }
});

test('Session 재탐색: 같은 실제 내용물과 현재 너겟 TYPE·죽은 귀환점의 대상 갱신', () => {
    const { o, routing } = scene();
    try {
        const parent = o.world.create(133), dead = o.world.create(76); o.core.owner.set(parent, 1); o.core.pool.allocated(dead)[11] |= 2;
        const content = o.contained.create(parent, 154, 0, 0x20, 0, 0); assert.ok(content);
        const runtime = new RawPathRuntime(); put(runtime.bytes, 8, parent); runtime.setTargets(0, dead);
        o.gameplayGlobals.gameAux().nuggetType = 153; o.motionModules.effects.path.retarget(runtime, parent); assert.equal(runtime.targetB, dead);
        o.gameplayGlobals.gameAux().nuggetType = 154; o.motionModules.effects.path.retarget(runtime, parent); assert.equal(runtime.targetB, 0);
    } finally { close(o, routing); }
});
