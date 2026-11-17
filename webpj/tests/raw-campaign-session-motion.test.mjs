// Session의 실제 기준점/현재 경로 효과/일반 자원 줍기 결선을 검사한다(WB04-E641).
// 저장 미션 전체 경로/포획·희생·공중3종 전용 동작 검사는 포함하지 않는다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { campaignOwners } from './campaign-session-support.mjs';
import { GameRandom } from '../www/core/random.js';
import { RawCarrierArrival } from '../www/core/raw-carrier-arrival.js';
import { RawPathCleanup } from '../www/core/raw-path-cleanup.js';
import { get } from './raw-support.mjs';

// 실제 생성자/건설/도착을 설치하며 음향·화면·입력 큐 경계만 관측한다.
function scene(extra = {}, bind = true) {
    const calls = [], o = campaignOwners({ ...extra, deletion: { selected: () => 0 }, construction: { refresh: () => {} },
        sound: { play: (sound) => sound, playOnceAt: (...args) => { calls.push(args); return args[2]; }, playLoopAt: (_x, _y, current, sound) => current || sound, stop: () => {}, isPlaying: () => 0 },
    });
    try {
        o.connectWorldInstall({ sections: Array.from({ length: 35 }, () => new Uint8Array(0)) }); o.connectOutcome(); o.connectAiBrains(); o.connectAiWorld(); o.connectAiProduction(); o.connectBuilding(); o.connectMotionArrivals();
        if (bind) o.connectMotionInputs(); return { o, calls };
    } catch (error) { o.dispose(); throw error; }
}

test('Session 이동 입력: 순서/반복/반납·같은 Pool 기준점과 줍기·설치 시 물리/난수 불변', () => {
    const unready = campaignOwners(); try { assert.throws(() => unready.connectMotionInputs(), /초기화 전/); } finally { unready.dispose(); }
    const { o } = scene({}, false);
    try {
        const bytes = o.core.pool.bytes(), seed = o.random.state; o.connectMotionInputs();
        assert.equal(o.motionModules.inputsReady, true); assert.equal(o.anchor.pool, o.core.pool); assert.equal(o.carrierPickup.pool, o.core.pool);
        assert.equal(o.state.motionState.halt.battle, true); assert.equal(o.anchor.graphAt(32, 32), 254); assert.equal(o.state.carrierPickupState.refuseHeldPriest, true);
        assert.deepEqual(o.core.pool.bytes(), bytes); assert.equal(o.random.state, seed); assert.equal(o.motion, null); assert.equal(o.loaded, false);
        assert.throws(() => o.connectMotionInputs(), /반복/);
    } finally { o.dispose(); }
    assert.throws(() => o.connectMotionInputs(), /반납/); assert.throws(() => o.motionModules.effects.carrier.currentAlliance(1, 2), /반납/);
});

test('Session 경로 효과: 같은 방향 동맹 현재값·서비스 값형 복사·복귀 가능한 현재 진단', () => {
    const input = { carrier: { currentAlliance: () => 99 }, path: { stepSound: () => 23 }, cleanup: { refund: () => 17 } }, diagnostics = [];
    const { o } = scene({ motion: input, ai: { assertion: (expression) => diagnostics.push(expression) } });
    try {
        input.path.stepSound = () => 0; const e = o.motionModules.effects;
        assert.equal(e.path.stepSound(), 23); assert.equal(e.cleanup.refund(), 17); assert.equal(e.cleanup.reserveShared(), true);
        o.alliances[11] = 0x80000001; assert.equal(e.carrier.currentAlliance(1, 2), 0x80000001); o.alliances[11] = 7; assert.equal(e.carrier.currentAlliance(1, 2), 7);
        assert.throws(() => e.carrier.currentAlliance(9, 0), /범위/);
        e.path.assertion('first', 'Path.cpp', 7); o.services.ai.assertion = (expression) => diagnostics.push(`current:${expression}`); e.path.assertion('second', 'Path.cpp', 8);
        assert.deepEqual(diagnostics, ['first', 'current:second']); o.services.ai.assertion = null; assert.throws(() => e.path.assertion('third', 'Path.cpp', 9), /Path.cpp:9/);
        assert.throws(() => e.path.stagePoint(0, 0), /목표점 미설치/); assert.throws(() => e.cleanup.ritual(0, 0, 0), /의식 설치 전/);
    } finally { o.dispose(); }
});

test('Session 경로 효과: 공유 난수 소비와 실제 현재 SoundList·목표 타입별 도착 분배', () => {
    const { o, calls } = scene();
    try {
        const e = o.motionModules.effects, expected = new GameRandom(o.random.state), seed = o.random.state;
        assert.throws(() => e.path.bulfRandom(4, 4), /난수 범위/); assert.equal(o.random.state, seed);
        assert.equal(e.path.bulfRandom(-7, 12), expected.next(19) - 7); assert.equal(o.random.state, expected.state);
        e.path.balloonFlame(4, 5); e.path.bulfSound(6, 7, 'bulfStep.wav');
        assert.deepEqual(calls.slice(-2), [[4, 5, o.soundNames.lookup('balloonFlame.wav'), 0, 0], [6, 7, o.soundNames.lookup('bulfStep.wav'), 0, 0]]);
        const bolt = o.world.create(105, 2); o.core.owner.set(bolt, 1); o.world.pop(bolt, 32, 32);
        e.path.constructionArrival(bolt, 0, 0); assert.equal(o.core.pool.slot(bolt)[11] & 1, 1);
        const nugget = o.world.create(154, 2); assert.throws(() => e.path.constructionArrival(nugget, 0, 0), /도착 타입 오류/);
    } finally { o.dispose(); }
});

test('Session 줍기/공간: 실제 현재 genus 탐색·너겟 수신→같은 내용물/입력 큐·SP 보존', () => {
    const queued = [], sent = [], { o } = scene({ assets: { queueCommand: (...args) => queued.push(args) }, carrierRequest: (owner, packet) => sent.push([owner, packet.slice()]) });
    try {
        const e = o.motionModules.effects, parent = o.world.create(133), nugget = o.world.create(154);
        o.world.pop(parent, 32, 32); o.core.owner.set(parent, 1); o.world.pop(nugget, 32, 32);
        assert.equal(e.carrier.genusAt(32.875, 32.875, o.core.types[154].flags2), nugget);
        const argument = get(o.core.pool.slot(nugget), 12, 2), sp = o.sp.get(1);
        assert.throws(() => o.carrierPickup.request(parent), /이동 설치 전/);
        // 아직 전체 Motion은 없지만 같은 실제 Pool의 도착 조회기만 설치해 두 모듈의 반환 계약을 대조한다.
        const forbidden = () => { throw new Error('자원 후보 조회에서 선택하지 않은 경로 효과'); }, c = o.core;
        const cleanup = new RawPathCleanup(c.pool, c.hash, c.types, o.state.motionState.cleanup, Object.fromEntries(['reserveShared', 'addShared', 'contained', 'graphFor', 'ritual', 'refund', 'destroy'].map((key) => [key, forbidden])));
        const hooks = Object.fromEntries(['path', 'genusAt', 'pickup', 'escape', 'chooseReturn', 'chooseGeyser', 'goReturn', 'goGeyser', 'minedPacket', 'destroy', 'robust'].map((key) => [key, forbidden]));
        const arrival = new RawCarrierArrival(c.pool, c.hash, c.types, o.priests.carrier, cleanup, o.priestState, o.anchor, o.contained, c.neighbors, o.state.motionState.carrier, hooks);
        o.motion = { carrierArrival: arrival }; assert.deepEqual(o.motionModules.pickupHooks.candidate(parent, 32, 32), [nugget, false]);
        o.carrierPickup.request(parent); assert.equal(sent.length, 1); assert.equal(sent[0][0], 9); assert.equal(get(sent[0][1], 6, 2), nugget);
        const packet = Uint8Array.of(1, 0x4f, 9, 0, parent & 255, parent >>> 8, nugget & 255, nugget >>> 8, 154);
        o.carrierPickup.receive(1, 0, packet);
        const content = o.priests.carrier.findContained(parent, 0x20, 154, 0); assert.ok(content); assert.equal(o.core.pool.slot(content)[10], 6); assert.equal(get(o.core.pool.slot(content), 12, 2), argument);
        assert.equal(o.core.pool.slot(nugget)[11] & 1, 1); assert.equal(o.sp.get(1), sp); assert.deepEqual(queued.at(-1), [0x030000b1, nugget, parent]);
        assert.throws(() => o.motionModules.pickupHooks.detach(content, 1), /Unpop flags/);
    } finally { o.dispose(); }
});

test('Session 이동 누락 경계: refund/CD 효과·현재 큐·미설치 의식·반납한 줍기 거부', () => {
    const { o } = scene(); const e = o.motionModules.effects, hooks = o.motionModules.pickupHooks;
    try {
        assert.throws(() => e.cleanup.refund(), /refund 미연결/); assert.throws(() => e.carrier.cdPriestPickupPriest(), /미연결/);
        assert.throws(() => hooks.queue(1, 2, 3), /입력 큐 미연결/); assert.throws(() => hooks.assertion(0x20c), /524/);
    } finally { o.dispose(); }
    assert.throws(() => hooks.find(1, 1, 0, 0), /반납/); assert.throws(() => e.path.bulfRandom(1, 2), /반납/);
});
