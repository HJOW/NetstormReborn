// 실제 Session의 사제 의식과 공유 Regular 예약 수명을 검사한다(WB04-E645).
import test from 'node:test';
import assert from 'node:assert/strict';
import { campaignOwners } from './campaign-session-support.mjs';
import { RawCampaignSessionRitual } from '../www/core/raw-campaign-session-ritual.js';

// 실제 설치 순서를 사용하며 저장 미션 Load는 실행하지 않는다.
function scene(temple = false, extra = {}) {
    const o = campaignOwners({ ...extra, sound: { playOnceAt: () => 0, ...extra.sound } });
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

test('다이스 의식: 같은 현재 장부·동맹·사제 TYPE과 설치 순서', () => {
    const early = scene();
    try { assert.throws(() => early.connectRitualDais(), /순서/); } finally { early.dispose(); }
    const o = scene(true);
    try {
        o.players[0x50] = 3; o.alliances[2] = 0xffffffff; o.connectRitualDais();
        assert.equal(o.daisRegular.pool, o.core.pool); assert.equal(o.daisState.playerRecords, o.players);
        assert.equal(o.state.runeState.playerFlags[0], 3); assert.equal(o.daisState.alliances[2], -1);
        o.priestState.movement.priest = 157; assert.equal(o.ritualModules.daisHooks.priestType(), 157);
        assert.equal(o.ritualModules.daisReady, true); assert.equal(o.loaded, false);
        assert.throws(() => o.connectRitualDais(), /순서/);
    } finally { o.dispose(); }
});

// 제단은 같은 Pool과 예약 객체를 공유하고 선행 결선 및 반납을 확인한다.
test('제단 의식: 실제 몸체와 공유 Regular 예약 및 설치 순서', () => {
    const early = scene(true);
    try { assert.throws(() => early.connectRitualAltar(), /순서/); } finally { early.dispose(); }
    const o = scene(true);
    const module = o.ritualModules;
    try {
        o.connectRitualDais(); o.connectRitualAltar();
        assert.equal(o.altarSacrifice.pool, o.core.pool);
        assert.equal(module.altarState.escapePercent, 40);
        assert.equal(module.altarHooks.boss(), o.state.assetState.readBoss());
        assert.equal(module.altarHooks.reserveShared(), true);
        const reserved = module.pendingRegular;
        assert.throws(() => module.altarHooks.reserveShared(), /중첩/);
        const altar = o.world.create(164);
        module.altarHooks.addShared(altar, 20, Math.fround(0.08));
        assert.equal(o.core.processes.findEvent(altar, 20), reserved);
        assert.equal(reserved.shared, true);
        assert.equal(module.pendingRegular, null);
        assert.throws(() => o.connectRitualAltar(), /순서/);
        assert.equal(o.loaded, false);
    } finally { o.dispose(); }
    assert.throws(() => module.altarHooks.boss(), /반납/);
});

test('다이스 효과: 동일 Sound47 예약·지연 시각과 Tell 명령 경계', () => {
    const o = scene(true);
    try {
        o.connectRitualDais(); const hooks = o.ritualModules.daisHooks, dais = o.world.create(165);
        assert.equal(hooks.reserveSound(), true); const pending = o.ritualModules.pendingSound;
        assert.throws(() => hooks.reserveSound(), /중첩/);
        const sound = hooks.loadSound('priestSacrifice2.wav', 0, 12), form = hooks.attachSound(dais, sound, 0, 12);
        assert.equal(o.core.processes.fromForm(form), pending); hooks.setSoundStart(form, 12.5); assert.equal(pending.startTime, 12.5);
        assert.throws(() => hooks.chat({ kind: 'LocalPriestDead' }), /멀티플레이/);
        assert.throws(() => hooks.stopVoice(1, 0, 0), /명령 큐 미연결/);
    } finally { o.dispose(); }
});

test('다이스 보상·안내: 실제 Core/SP 장부와 입력 큐·Tell을 공유한다', () => {
    const queued = [], tells = [], o = scene(true, { assets: { queueCommand: (...args) => queued.push(args) }, priest: { shield: { tell: (key) => tells.push(key) } } });
    try {
        o.connectRitualDais(); const h = o.ritualModules.daisHooks;
        h.stopVoice(0x020000df, 0xffffffff, 2); h.chat({ kind: 'Stormpower' }); h.chat({ kind: 'Upgrade' });
        assert.deepEqual(queued, [[0x020000df, -1, 2]]); assert.deepEqual(tells, ['SacrificeForStormpower', 'AltarUpgrade']);
        const bytes = new DataView(new ArrayBuffer(4)); bytes.setFloat32(0, 123, true); o.sp.set(2, bytes.getUint32(0, true));
        h.normalizeForeignSp(2); bytes.setUint32(0, o.sp.get(2), true); assert.equal(bytes.getFloat32(0, true), 5123);
        const type = o.core.types.findIndex((item) => item.techBitIndex >= 0 && item.techBitIndex < 32);
        assert.ok(type >= 0); const bit = o.core.types[type].techBitIndex, records = o.coreDataGlobals.records();
        h.grantForeignType(2, type); const view = new DataView(records.buffer, records.byteOffset, records.byteLength);
        assert.notEqual(view.getUint32(42, true) & (1 << bit), 0);
    } finally { o.dispose(); }
});

test('희생 요청: 실제 내용물 탐색은 너겟만 제거하고 같은 다이스 자격을 조회한다', () => {
    const sounds = [], notices = [], o = scene(true, { deletion: { selected: () => 0 }, contentLifetime: { sound: (...args) => sounds.push(args), localNotice: (value) => notices.push(value) }, assets: { queueCommand: () => {} } });
    try {
        o.connectRitualDais(); const priest = o.world.create(158), dais = o.world.create(165);
        const nugget = o.contained.create(priest, 154, 9, 0x20, 0, 0);
        assert.ok(nugget); o.ritualModules.sacrificeHooks.remove(priest, 154, 0, 1);
        assert.equal(o.priests.carrier.findContained(priest, 0x20, 154, 0), 0);
        assert.equal(sounds.length, 1); assert.equal(notices.length, 0);
        assert.equal(typeof o.sacrificeRequest.allowed(dais, priest), 'boolean');
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
