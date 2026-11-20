// 같은 실제 Session 기반 자료의 섬·소리·사제 생성 순서/공유 참조/종료를 대조한다(WB04-E577).
// 전체 Session/Load·미션 Login·실제 브라우저 소리 재생의 증거로 사용하지 않는다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { GameAssets } from '../www/assets/game-assets.js';
import { SoundList } from '../www/audio/sound-list.js';
import { BaseProcess, Kernel } from '../www/core/kernel.js';
import { RawCampaignSessionOwners } from '../www/core/raw-campaign-session-owners.js';
import { ClientKencloudGlobals } from '../www/core/client-kencloud-globals.js';
import { ClientRandomGlobals } from '../www/core/random.js';
import { ClientSpGlobals } from '../www/core/client-sp-globals.js';
import { RawContainedLookup } from '../www/core/raw-contained-lookup.js';
import { realCaptureAssets } from './capture-assets-support.mjs';
import { put, floatToBits } from './raw-support.mjs';

// 같은 보호 자산을 빌리며 실제 월드/Player/SP는 시험마다 새로 만든다.
const data = realCaptureAssets(); const game = new GameAssets(data.table, data.database);
const files = { read() { throw new Error('이 생성 구간의 VFS 읽기 금지'); } };
// 아직 이 구간에서 선택하면 안 되는 외부 효과다.
function forbidden() { throw new Error('선택되지 않아야 하는 효과'); }
// 실제 장치의 호출만 기록한다. 게임/Host/소리 프로세스 계산은 기존 실제 몸체가 수행한다.
function soundHooks(events = []) { return { wallSeconds: () => 100, playLoopAt: (...args) => { events.push(['loop', ...args]); return 123; }, playOnceAt: (...args) => events.push(['once', ...args]), play: (...args) => { events.push(['play', ...args]); return 123; }, stop: (handle) => events.push(['stop', handle]), isPlaying: () => 0 }; }
// 독립 캠페인의 필수 시각/소리 장치를 공급한다.
function scene(extra = {}) { return new RawCampaignSessionOwners(game, files, 'thewarbegins', { unixSeconds: () => 1700000000, soundNames: new SoundList(), sound: soundHooks(), ...extra }); }
// 원문 Player/SP→테이블→섬/사제 생성 순서를 실행한다.
function connect(o) { o.initializePlayersAndSp(); o.connectPlayerTablesAndPackets(); o.connectIslandsAndPriests(); return o.priestModules; }
// 같은 실제 풀의 사제를 원문 생성자로 만들고 이 조회 검사의 소유자/상태만 설정한다.
function priest(o, owner = 1) { const sid = o.world.create(158, 2); const raw = o.core.pool.allocated(sid); raw[34] = owner; raw[11] &= ~2; return sid; }

test('Session 섬/사제: 테이블 뒤 생성하며 같은 물리·HP·Player·권한을 빌리고 전체 준비와 구별한다', () => {
    const o = scene();
    try {
        assert.throws(() => o.connectIslandsAndPriests(), /초기화 전/); assert.equal(o.islands, null);
        o.world.storage.priestOwnerState[1] = 0x87;
        const m = connect(o); assert.equal(m.ready, true); assert.equal(o.world.storage.priestOwnerState[1], 0x87); assert.equal(o.islands.bytes, o.islandBytes); assert.equal(o.islands.pool, o.core.pool);
        assert.equal(o.priests.pool, o.core.pool); assert.equal(o.priests.priests, o.priestState.priests); assert.equal(o.priestState.hitPoints.playerRecords, o.players);
        assert.equal(o.sounds.pool, o.core.pool); assert.equal(o.sounds.state, o.state.soundState); assert.equal(o.priestParticipation.pool, o.core.pool); assert.equal(o.priestParticipation.priests, o.priestState.priests);
        assert.deepEqual([o.priestState.owner.battlePlayer, o.priestState.owner.fortPlayer, o.priestState.shield.localPlayer], [1, 1, 1]);
        assert.throws(() => o.priests.setAuthority(false), /공유 권한/); assert.throws(() => o.priests.validateReady(), /Damageable/); assert.throws(() => o.gameplayGlobals.requireSession(), /미연결/); assert.throws(() => o.connectIslandsAndPriests(), /반복/);
    } finally { o.dispose(); }
    assert.equal(o.priestState.hitPoints.playerRecords.length, 0); assert.throws(() => o.connectIslandsAndPriests(), /반납/);
});

test('Session 섬: 생성 때 기존 기록을 Reset하지 않고 현재 Client redraw/개정과 같은 목록을 사용한다', () => {
    const kencloud = new ClientKencloudGlobals(); const o = scene({ kencloudGlobals: kencloud });
    try {
        o.initializePlayersAndSp(); o.connectPlayerTablesAndPackets(); const bytes = o.islandBytes; bytes[18] = 0x9a; bytes[19] = 0xbc; const before = bytes.slice();
        o.connectIslandsAndPriests(); assert.deepEqual(bytes, before); assert.equal(o.worldRedraw, kencloud.worldRedraw()); const island = o.islands.add(1, 2);
        o.islands.setBounds(island, { left: 1, top: 1, right: 20, bottom: 20 }); kencloud.worldRedraw().value = 41; o.revision = 9;
        o.islands.setOwner(island, 2); assert.equal(kencloud.worldRedraw().value, 42); assert.equal(o.revision, 1); assert.equal(o.gameplayGlobals.mapRevision().value, 1); assert.equal(o.finishState.playerRegions[2], island); assert.equal(bytes[18], 0x9a); assert.equal(bytes[19], 0xbc);
        // 없는 섬 번호는 원본 Client 처럼 정확한 식과 원본 파일·행으로 진단을 표시하고 계속한다(예전에는 '미연결 1189' 오류로 프레임이 멈췄다). (2026-11-20 WB04-E1124)
        const shown = [], original = o.world.storage.assertDisplay.bind(o.world.storage); o.world.storage.assertDisplay = (...args) => { shown.push(args); };
        try { assert.doesNotThrow(() => o.islands.owner(99)); } finally { o.world.storage.assertDisplay = original; }
        assert.equal(shown.length, 1); assert.equal(shown[0][0], 'exists( islandId )'); assert.equal(shown[0][1], '\\Ns\\O\\Islandlist.cpp'); assert.ok(shown[0][2] > 0);
    } finally { o.dispose(); }
});

test('Session 소리: 실제 이름 표와 같은 Host의 Sound47을 부착하고 종료 전 현재 모듈을 유지한다', () => {
    const events = []; const kernel = new Kernel(); kernel.add(new BaseProcess()); let o;
    const sound = soundHooks(events); sound.stop = (handle) => { assert.equal(o.world.storage.borrowed, true); assert.equal(o.priestState.hitPoints.playerRecords, o.players); events.push(['stop', handle]); };
    o = scene({ sound, processKernel: kernel });
    try {
        const m = connect(o); const parent = o.world.create(82, 2); const raw = o.core.pool.allocated(parent); put(raw, 14, floatToBits(10)); put(raw, 18, floatToBits(11)); o.state.soundState.frame = 1;
        const name = m.effects.shield.loadSound('priestForceField.wav'); assert.equal(name, o.soundNames.lookup('priestForceField.wav')); assert.equal(m.effects.shield.reserveSound(), true);
        m.effects.shield.attachSound(parent, name, 0, 1); const form = new RawContainedLookup(o.core.pool, forbidden).find(parent, 47); const process = o.core.processes.fromForm(form); assert.ok(process); assert.equal(process.flags, 1);
        process.runFrame(); assert.deepEqual(events, [['loop', 10, 11, 0, name]]); assert.equal(kernel.size, 2);
        o.core.processes.kill(process, 0x10); assert.deepEqual(events.at(-1), ['stop', 123]); assert.equal(kernel.size, 1);
        // 최종 Kernel 반납은 Form 삭제 통지가 아니다. 미실행 소리를 남겨 현재 World 소유권만 정리되는지 본다.
        assert.ok(o.sounds.add(parent, name, 0, 1)); o.dispose(); assert.equal(kernel.size, 1); assert.equal(o.world.storage.borrowed, false); assert.equal(o.priestState.hitPoints.playerRecords.length, 0);
        assert.throws(() => o.world.drainOwnedProcessesForShutdown(), /반납/);
    } finally { o.dispose(); kernel.clear(); }
});

test('Session 서비스 값형 수명: 원래 priest/shield/regen/sound의 재대입과 생성자의 덮어쓰기를 분리한다', () => {
    const savedFall = () => 17; const savedWalker = () => 23; const savedMeasurement = () => 29; const notice = () => 31;
    const priestInput = { fallSound: savedFall, otherWalkerFall: savedWalker, ownerNotice: notice, shield: { noticeSound: notice, translate: notice, tell: notice, reserveSound: forbidden }, regen: { measurement: savedMeasurement } }; const sound = soundHooks(); const originalPlay = sound.play;
    const o = scene({ priest: priestInput, sound });
    try {
        priestInput.fallSound = forbidden; priestInput.otherWalkerFall = forbidden; priestInput.shield.tell = forbidden; priestInput.regen.measurement = forbidden; sound.play = forbidden;
        const m = connect(o); assert.equal(m.effects.fallSound, savedFall); assert.equal(m.effects.ownerNotice, notice); assert.equal(m.effects.shield.tell, notice); assert.equal(m.effects.regen.measurement, savedMeasurement); assert.equal(m.effects.otherWalkerFall(123), 23); assert.equal(o.sounds.hooks.play, originalPlay);
        assert.equal(priestInput.shield.reserveSound, forbidden); assert.equal(priestInput.shield.loadSound, undefined); assert.equal(priestInput.pop, undefined); assert.equal(priestInput.regen.spReads, undefined);
    } finally { o.dispose(); }
});

test('Session 사제 낙하: 현재 위치 소리 장치를 조회하고 원문 이름/flags0을 같은 표에서 보낸다', () => {
    const events = []; const o = scene();
    try {
        const m = connect(o); o.services.sound.playOnceAt = (...args) => events.push(args); m.effects.fallSound(20000, 1.25, -2.5);
        assert.deepEqual(events, [[1.25, -2.5, o.soundNames.lookup('priestFall.wav'), 0, 0]]); o.services.sound.playOnceAt = null; assert.throws(() => m.effects.fallSound(0, 0, 0), /위치 음향 미연결/);
        assert.throws(() => m.effects.otherWalkerFall(0), /Walker 낙하 미연결/);
    } finally { o.dispose(); }
});

test('Session 사제 진단: 현재 AI 장치와 같은 Storage 원문 진단을 구별하고 선택한 FillMissing만 실패한다', () => {
    const events = []; const o = scene({ ai: { assertion: (expression) => events.push(expression) } });
    try {
        const m = connect(o); const e = m.effects; e.forcefieldAssertion('first', 'ignored', 123); o.services.ai = { assertion: (expression) => events.push(`new:${expression}`) }; e.forcefieldAssertion('second', 'ignored', 456);
        o.world.storage.bindDisplayDiagnostics({ value: '' }, (...args) => events.push(args)); e.ownerAssertion('owner', 'file', 789); assert.deepEqual(events, ['first', 'new:second', ['owner', 'file', 789]]);
        o.services.ai.assertion = null; assert.throws(() => e.forcefieldAssertion('x', '', 77), /미연결 77/); assert.throws(() => e.shield.tell('x'), /1213/); assert.throws(() => e.regen.measurement(), /1228/);
        assert.equal(e.reserveRegular(), true); assert.throws(() => e.dependentFound(), /지도 범위 미설치/); assert.throws(() => e.regen.neutralAction(0), /panic 실제 설치 전/); assert.throws(() => e.regen.damageable(0), /피해 실제 설치 전/);
    } finally { o.dispose(); }
});

test('Session 사제 참여: 등록/조회가 같은 장기 목록과 owner 표를 사용하고 현재 사제 TYPE를 읽는다', () => {
    const o = scene();
    try {
        connect(o); const sid = priest(o); const list = o.world.storage.sidLists.priests; list.append(sid); assert.equal(o.priestState.priests.priests.readCount(), 0);
        assert.equal(o.priestParticipation.find(1), sid); o.world.storage.priestOwnerState[1] = 0x99; o.priestParticipation.refresh(1); assert.equal(o.world.storage.priestOwnerState[1], 0);
        o.gameplayGlobals.gameAux().priestType = 157; o.priestParticipation.refresh(1); assert.equal(o.world.storage.priestOwnerState[1], 1); assert.equal(o.priestState.priests.ownerState[1], 0);
    } finally { o.dispose(); }
});

test('Session 회복/SP: 같은 추적 holder와 현재 저장소 조회를 사용하고 감사 회전 뒤 다시 읽는다', () => {
    const o = scene();
    try {
        const m = connect(o); const reads = m.effects.regen.spReads; assert.equal(o.sp.trackingOwner, o.priestState.regen.patch.tracking); assert.equal(reads.trackingSequence, undefined);
        assert.equal(reads.observedSequence(), o.sp.offset); assert.equal(reads.firstPermutation(), o.sp.permutation()[0]); const before = o.sp.offset;
        m.effects.regen.patchAudit(); assert.notEqual(o.sp.offset, before); assert.equal(reads.observedSequence(), o.sp.offset); assert.equal(reads.firstPool(), o.sp.firstPoolReady ? true : null);
    } finally { o.dispose(); }
});

test('Session 회복 표시: 실제 display 몸체에 현재 raw 슬롯의 같은 물리를 전달한다', () => {
    const o = scene();
    try {
        const m = connect(o); const sid = priest(o); const raw = o.core.pool.allocated(sid); const update = o.core.display.update.bind(o.core.display); let calls = 0;
        // 연결 경계의 참조만 관찰한 뒤 실제 표시 계산을 실행한다. 읽기 사본을 넘기면 같은 물리 계약이 깨진다.
        o.core.display.update = (bytes) => { calls += 1; assert.equal(bytes.buffer, raw.buffer); assert.equal(bytes.byteOffset, raw.byteOffset); assert.equal(bytes.byteLength, raw.byteLength); update(bytes); };
        m.effects.regen.refresh(sid); assert.equal(calls, 1);
    } finally { o.dispose(); }
});

test('Session 공통 회복/SP: Client 추적 holder를 지역 사제 추적으로 다시 연결하지 않는다', () => {
    const randomGlobals = new ClientRandomGlobals(); const spGlobals = new ClientSpGlobals(randomGlobals, () => 1700000000); spGlobals.store().initialize(); const holder = spGlobals.store().trackingOwner;
    const o = scene({ randomGlobals, spGlobals });
    try { const m = connect(o); assert.equal(m.effects.regen.spReads.trackingSequence(), holder); assert.notEqual(holder, o.priestState.regen.patch.tracking); assert.equal(o.sp.trackingOwner, holder); } finally { o.dispose(); }
});

test('Session 부분 실패: Sound 장치 생성 실패는 앞선 섬 기록/결선을 남기고 사제 등록을 하지 않는다', () => {
    const o = scene({ sound: {} });
    try {
        o.initializePlayersAndSp(); o.connectPlayerTablesAndPackets(); o.islandBytes[18] = 0x7a; assert.throws(() => o.connectIslandsAndPriests(), /소리 프로세스 필수/);
        assert.ok(o.islands); assert.equal(o.sounds, null); assert.equal(o.priests, null); assert.equal(o.priestModules.ready, false); assert.equal(o.islandBytes[18], 0x7a); assert.equal(typeof o.priestModules.effects.pop, 'function'); assert.throws(() => o.connectIslandsAndPriests(), /반복/);
    } finally { o.dispose(); }
    assert.equal(o.world.storage.borrowed, false); const p = o.playerRecords.acquire(); p.release(); const g = o.gameplayGlobals.acquire(); g.release();
});

test('Session 부분 실패: 실제 SP holder 불일치는 Sound 생성 뒤 사제 생성 전에 멈춘다', () => {
    const o = scene();
    try {
        o.initializePlayersAndSp(); o.connectPlayerTablesAndPackets(); o.priestState.regen.patch.tracking = { value: 0 }; assert.throws(() => o.connectIslandsAndPriests(), /추적 주소 불일치/);
        assert.ok(o.sounds); assert.equal(o.priests, null); assert.equal(o.priestModules.ready, false); assert.throws(() => o.connectIslandsAndPriests(), /반복/);
    } finally { o.dispose(); }
});
