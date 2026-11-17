// 실제 Session의 사제 의식과 공유 Regular 예약 수명을 검사한다(WB04-E645).
import test from 'node:test';
import assert from 'node:assert/strict';
import { campaignOwners } from './campaign-session-support.mjs';
import { RawCampaignSessionRitual } from '../www/core/raw-campaign-session-ritual.js';

// 실제 설치 순서를 사용하며 저장 미션 Load는 실행하지 않는다.
function scene(temple = false) {
    const o = campaignOwners({ sound: { playOnceAt: () => 0 } });
    try {
        o.connectWorldInstall({ sections: Array.from({ length: 35 }, () => new Uint8Array(0)) });
        o.connectOutcome(); o.connectAiBrains(); o.connectAiWorld(); o.connectAiProduction(); o.connectBuilding();
        o.connectMotionArrivals(); o.connectMotionInputs(); o.connectCarrierRouting(); o.connectMotionRuntime(); o.connectPriestPanic(); o.connectRitualPriest();
        if (temple) o.connectRitualTemple(); return o;
    } catch (error) { o.dispose(); throw error; }
}

test('사제 의식: 설치 순서·실제 Pool·반복 및 반납', () => {
    const early = campaignOwners(), partial = new RawCampaignSessionRitual(early);
    try { assert.throws(() => partial.bind(), /순서/); } finally { early.dispose(); partial.dispose(); }
    const o = scene(), module = o.ritualModules;
    try {
        assert.equal(o.ritualActions.pool, o.core.pool); assert.equal(o.loaded, false);
        assert.throws(() => o.connectRitualPriest(), /순서/);
    } finally { o.dispose(); }
    assert.throws(() => module.reserveRegular(), /반납/);
});

test('신전 의식: 같은 Pool·현재 사제 TYPE/권한과 반복 설치 관문', () => {
    const o = scene(true);
    try {
        const state = o.state.ritualState;
        assert.equal(o.templeRitual.pool, o.core.pool); assert.equal(o.runePostPop.pool, o.core.pool);
        o.priestState.movement.priest = 157; assert.equal(state.currentPriestType, 157);
        assert.equal(state.currentBoss, o.state.assetState.readBoss());
        assert.equal(o.ritualModules.templeReady, true); assert.equal(o.loaded, false);
        assert.throws(() => o.connectRitualTemple(), /반복/);
    } finally { o.dispose(); }
});

test('룬 후처리: 실제 등록 몸체의 최초 Pop은 같은 Host에 304를 예약한다', () => {
    const o = scene(true);
    try {
        const rune = o.world.create(166); o.world.pop(rune, 32, 32, 1);
        const task = o.core.processes.findEvent(rune, 0x304);
        assert.ok(task); assert.equal(task.shared, false); assert.equal(task.payload, Math.fround(0.01));
        assert.equal(o.ritualModules.pendingRegular, null);
    } finally { o.dispose(); }
});

test('의식 예약: 실제 동일 객체와 중첩·미예약 관문, 공유 Host 시각', () => {
    const o = scene();
    try {
        const m = o.ritualModules, priest = o.world.create(158);
        assert.throws(() => m.addRegular(priest, 604, 0.01, 61), /예약 누락/);
        assert.equal(m.reserveRegular(), true); const reserved = m.pendingRegular;
        assert.throws(() => m.reserveRegular(), /중첩/);
        m.addRegular(priest, 604, Math.fround(0.01), 61);
        assert.equal(m.pendingRegular, null); assert.equal(o.core.processes.findEvent(priest, 604), reserved);
        assert.equal(reserved.time, o.core.processState.now); assert.equal(reserved.shared, true);
    } finally { o.dispose(); }
});

test('사제 의식 동작: 실제 K 프레임과 같은 Host에 604 공유 예약', () => {
    const o = scene();
    try {
        const priest = o.world.create(158);
        o.ritualActions.continue(priest, 0);
        const raw = o.core.pool.slot(priest), view = new DataView(raw.buffer, raw.byteOffset, raw.byteLength);
        assert.equal(view.getInt32(36, true), o.core.frames[158].run(0x4b).first);
        assert.ok(o.core.processes.findEvent(priest, 604));
        assert.equal(o.ritualModules.pendingRegular, null);
    } finally { o.dispose(); }
});
