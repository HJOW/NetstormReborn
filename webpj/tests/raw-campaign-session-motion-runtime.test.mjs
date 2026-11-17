// 같은 실제 Session의 채취/전달/현재 TYPE/이동 본체 결선 검사다(WB04-E643).
import test from 'node:test';
import assert from 'node:assert/strict';
import { campaignOwners } from './campaign-session-support.mjs';
import { RawCampaignSessionMotionRuntime } from '../www/core/raw-campaign-session-motion-runtime.js';
import { get, put } from './raw-support.mjs';

// 외부 음향/입력만 관측하며 실제 생성자/Load 앞단 설치 순서를 사용한다.
function scene(bind = true, extra = {}) {
    const sounds = [], queued = [], o = campaignOwners({ ...extra, assets: { queueCommand: (...args) => queued.push(args) }, deletion: { selected: () => 0 }, construction: { refresh: () => {} }, sound: { play: (sound) => sound, playOnceAt: (...args) => { sounds.push(args); return args[2]; }, playLoopAt: (_x, _y, current, sound) => current || sound, stop: () => {}, isPlaying: () => 0 } });
    let runtime = null;
    try {
        o.connectWorldInstall({ sections: Array.from({ length: 35 }, () => new Uint8Array(0)) }); o.connectOutcome(); o.connectAiBrains(); o.connectAiWorld(); o.connectAiProduction(); o.connectBuilding(); o.connectMotionArrivals(); o.connectMotionInputs(); o.connectCarrierRouting();
        if (bind) { o.connectMotionRuntime(); runtime = o.motionRuntimeModules; } else { runtime = new RawCampaignSessionMotionRuntime(o); } return { o, runtime, sounds, queued };
    } catch (error) { o.dispose(); runtime?.dispose(); throw error; }
}
// 부모의 실제 Kernel/Host를 먼저 비우고 지역 callback 대여를 닫는다.
function close(o, runtime) { try { o.dispose(); } finally { runtime.dispose(); } }

test('Session 이동 본체: 선행/반복/반납·같은 Pool/Host/시계와 설치 시 물리/난수 불변', () => {
    const early = campaignOwners(), unready = new RawCampaignSessionMotionRuntime(early);
    try { assert.throws(() => unready.bind(), /초기화 전/); } finally { close(early, unready); }
    const { o, runtime } = scene(false);
    try {
        const bytes = o.core.pool.bytes(), seed = o.random.state; runtime.bind();
        for (const body of [o.geyserMined, o.carrierMining, o.carrierTransfer, o.motion, o.motion.paths]) assert.equal(body.pool, o.core.pool); // 실제 하위는 모두 기존 단일 Pool을 빌린다.
        assert.equal(runtime.ready, true); assert.equal(o.loaded, false); assert.equal(o.random.state, seed); assert.deepEqual(o.core.pool.bytes(), bytes);
        assert.throws(() => runtime.bind(), /반복/);
    } finally { close(o, runtime); }
    assert.throws(() => o.motionModules.effects.path.metadata(0), /반납/);
});

test('Session 이동 메타데이터: 보호 사제 TYPE과 현재 Dude 속도·stepSound0·미설치 panic 경계', () => {
    const { o, runtime } = scene();
    try {
        const sid = o.world.create(158), e = o.motionModules.effects;
        o.dudeSpeeds[158].speed = 5.5; assert.equal(e.path.metadata(sid).speed, 5.5); assert.equal(e.path.soundId(sid), 0);
        assert.ok(e.path.metadata(sid).turningSpeed > 0); assert.throws(() => e.priest.neutralReturn(sid), /panic 실제 설치 전/);
        assert.throws(() => e.path.stepSound(sid, 0, 0, 0), /stepSound 미연결/);
    } finally { close(o, runtime); }
});

test('Session 채취: 실제6BYTE 통지→로컬 수신·현재 너겟 cost 차감·같은 위치 음향/난수', () => {
    const { o, runtime, sounds } = scene();
    try {
        const geyser = o.world.create(122); o.world.pop(geyser, 32, 32); put(o.core.pool.allocated(geyser), 12, 600, 2);
        const seed = o.random.state, cost = o.core.types[154].cost;
        o.motionModules.effects.carrier.minedPacket(geyser);
        assert.equal(get(o.core.pool.slot(geyser), 12, 2), 600 - cost); assert.notEqual(o.random.state, seed); assert.equal(sounds.at(-1)[0], 32); assert.equal(sounds.at(-1)[1], 32);
        assert.deepEqual(runtime.miningHeader, { message: 0x5b, protocol: 1, length: 6 });
    } finally { close(o, runtime); }
});

test('Session 너겟 전달: 같은 Form을 분리/부착하고 실제491 미리보기 예약·SP 보존', () => {
    const { o, runtime } = scene();
    try {
        const donor = o.world.create(133), recipient = o.world.create(133); o.core.owner.set(donor, 1); o.core.owner.set(recipient, 1);
        o.world.pop(donor, 32, 32); o.world.pop(recipient, 34, 32);
        const content = o.contained.create(donor, 154, 9, 0x20, 0, 0), sp = o.sp.get(1);
        const packet = Uint8Array.of(1, 0x60, 10, 0, recipient & 255, recipient >>> 8, content & 255, content >>> 8, 0, 0);
        o.carrierTransfer.receive(1, 0, packet);
        assert.equal(o.priests.carrier.findContained(donor, 0x20, 154, 0), 0); assert.equal(o.priests.carrier.findContained(recipient, 0x20, 154, 0), content);
        assert.ok(o.core.processes.findEvent(recipient, 0x491)); assert.equal(o.sp.get(1), sp); assert.throws(() => runtime.transferHooks.detach(content, 1), /Unpop flags/);
    } finally { close(o, runtime); }
});

test('Session 실제 이동 본체의 자원 후보: 실제 도착 조회→4f 요청·수신→같은 내용물', () => {
    const sent = [], { o, runtime, queued } = scene(true, { carrierRequest: (owner, packet) => sent.push([owner, packet.slice()]) });
    try {
        const parent = o.world.create(133), nugget = o.world.create(154); o.core.owner.set(parent, 1); o.world.pop(parent, 32, 32); o.world.pop(nugget, 32, 32);
        assert.deepEqual(o.motion.carrierArrival.pickupCandidate(parent, 32, 32), { sid: nugget, flag: false });
        const sp = o.sp.get(1); o.motionModules.effects.carrier.pickup(parent);
        assert.equal(sent.length, 1); assert.equal(sent[0][0], 9); assert.equal(sent[0][1][1], 0x4f); assert.equal(get(sent[0][1], 6, 2), nugget);
        assert.equal(o.priests.carrier.findContained(parent, 0x20, 154, 0), 0);
        o.carrierPickup.receive(1, 0, sent[0][1]); assert.ok(o.priests.carrier.findContained(parent, 0x20, 154, 0)); assert.equal(o.core.pool.slot(nugget)[11] & 1, 1);
        assert.deepEqual(queued.at(-1), [0x030000b1, nugget, parent]); assert.equal(o.sp.get(1), sp);
    } finally { close(o, runtime); }
});
