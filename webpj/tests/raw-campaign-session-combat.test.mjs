// Session1484..1536의 실제 Archer/Missile 결선·값 사본·현재 장치·아이스 반사를 대조한다(WB04-E586).
// 저장 미션 Load/교전·포획/희생·공중3종 전용 생성/행동·브라우저 출력은 실행하지 않는다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { readData } from './helpers.mjs';
import { realCaptureAssets } from './capture-assets-support.mjs';
import { get, put, floatToBits } from './raw-support.mjs';
import { GameAssets } from '../www/assets/game-assets.js';
import { GamePalette } from '../www/assets/palette.js';
import { SoundList } from '../www/audio/sound-list.js';
import { RawCampaignSessionOwners } from '../www/core/raw-campaign-session-owners.js';
import { RawCampaignArcherBindings } from '../www/core/raw-campaign-archer-bindings.js';
import { RawIceRicochet } from '../www/core/raw-ice-ricochet.js';
import { MissileProcess } from '../www/core/raw-missile.js';

// 실제 보호 COL/116TYPE/SHP를 같은 현재188TYPE 월드에 빌린다.
const data = realCaptureAssets(); const game = new GameAssets(data.table, data.database, GamePalette.parse(readData('d/GIFCLOUD.COL')));
// 선택하지 않은 외부 장치를 빈 성공으로 대체하지 않는다.
function forbidden() { throw new Error('선택되지 않은 외부 효과'); }
// 기반 owner만 만든다. 서비스 사본을 검사할 때 앞 생성 구간은 별도로 실행한다.
function owner(extra = {}) { return new RawCampaignSessionOwners(game, { read: forbidden }, 'thewarbegins', { unixSeconds: () => 1700000000, soundNames: new SoundList(), sound: { wallSeconds: () => 0, play: forbidden, playOnceAt: forbidden, playLoopAt: forbidden, stop: forbidden, isPlaying: forbidden }, ...extra }); }
// 앞 생성 구간을 원문 순서로 실제 조립한다. 전체 Session/Load 공개는 하지 않는다.
function before(o) { o.initializePlayersAndSp(); o.connectPlayerTablesAndPackets(); o.connectIslandsAndPriests(); o.connectPlayersAndOwnership(); o.connectBodiesAndTypeTables(); o.connectGunAndCursorViews(); o.connectHitBodies(); o.connectGunBodies(); o.connectParticles(); o.connectTeleport(); }
// 같은 실제 모듈의 이번 생성 구간까지 연결한다.
function scene(extra = {}) { const o = owner(extra); before(o); o.connectCombat(); return o; }
// 실제 자산 생성자 위에 고정 좌표/owner 입력만 놓는다. 저장 미션 Pop/교전 성공을 가장하지 않는다.
function actor(o, type) { const sid = o.world.create(type, 2); const raw = o.core.pool.allocated(sid); raw[34] = 1; put(raw, 14, floatToBits(20)); put(raw, 18, floatToBits(20)); return sid; }

test('Session Archer: Teleport 선행·한 번 결선·같은 Pool/난수·미공개와 반복 반납을 유지한다', () => {
    const o = owner();
    try { assert.throws(() => o.connectCombat(), /초기화 전/); before(o); const seed = o.random.state; o.connectCombat(); assert.ok(o.combat instanceof RawCampaignArcherBindings); assert.ok(o.iceRicochet instanceof RawIceRicochet); assert.equal(o.combat.pool, o.core.pool); assert.equal(o.random.state, seed); assert.equal(o.combatModules.ready, true); assert.throws(() => o.connectCombat(), /반복/); assert.throws(() => o.gameplayGlobals.requireSession(), /미연결/); } finally { o.dispose(); }
    o.dispose(); assert.throws(() => o.connectCombat(), /반납/);
});

test('Session Archer 실패 접두: 반사 생성 뒤 표적 Pool 오류는 상태를 보존하고 재시작을 거부한다', () => {
    const o = owner(); const other = owner();
    try { before(o); before(other); o.gunTarget = other.gunTarget; const size = o.core.kernel.size; assert.throws(() => o.connectCombat(), /현재 표적 Pool/); assert.equal(o.combat, null); assert.ok(o.iceRicochet instanceof RawIceRicochet); assert.ok(o.combatModules.effects); assert.equal(o.combatModules.ready, false); assert.equal(o.core.kernel.size, size); assert.throws(() => o.connectCombat(), /반복/); } finally { o.dispose(); other.dispose(); }
});

test('Session Archer 값 사본: 입력/현재 서비스 묶음을 바꿔도 선택된 사용자 효과와 외부 참조를 보존한다', () => {
    const calls = []; const first = (...args) => calls.push(args); const later = forbidden;
    const input = { iceCooling: first, animation: { common: { sound: first, targetPoint: first } }, missile: { firingSound: first, impactSound: first, impactName: first, damage: first, flightEffect: first }, beam: { create: forbidden }, ricochetPointBytes: first };
    const o = owner({ archer: input });
    try {
        input.iceCooling = later; input.animation.common.sound = later; input.missile.firingSound = later; input.missile.flightEffect = later; before(o); o.connectCombat(); const e = o.combatModules.effects;
        assert.equal(e.iceCooling, first); assert.equal(e.animation.common.sound, first); assert.equal(e.animation.common.targetPoint, first); assert.equal(e.missile.firingSound, first); assert.equal(e.missile.impactSound, first); assert.equal(e.missile.impactName, first); assert.equal(e.missile.damage, first); assert.equal(o.combatModules.ricochetHooks.initialPointBytes, first); assert.notEqual(e.beam.create, input.beam.create);
        o.services.archer.missile.flightEffect = later; e.missile.flightEffect(77, 123); assert.deepEqual(calls, [[77, 123]]);
    } finally { o.dispose(); }
});

test('Session Archer 기본 음향: 현재 비정렬 float 좌표/실제 SoundList와 현재 재생 장치를 읽는다', () => {
    const o = scene(); const calls = [];
    try {
        const sid = actor(o, 128); put(o.core.pool.allocated(sid), 14, floatToBits(-3.25)); put(o.core.pool.allocated(sid), 18, floatToBits(12.5)); const e = o.combatModules.effects;
        o.services.sound.playOnceAt = (...args) => { calls.push(['A', ...args]); o.services.sound.playOnceAt = (...next) => calls.push(['B', ...next]); };
        e.animation.common.sound(sid, 'shortZap.wav'); e.missile.firingSound(1.25, 2.5, 123); e.missile.impactSound(3.5, 4.25, 456); e.missile.impactName(5.75, 6.5, 'iceCannonImpact1.wav');
        assert.deepEqual(calls, [['A', -3.25, 12.5, o.soundNames.lookup('shortZap.wav'), 0, 0], ['B', 1.25, 2.5, 123, 0, 0], ['B', 3.5, 4.25, 456, 0, 0], ['B', 5.75, 6.5, o.soundNames.lookup('iceCannonImpact1.wav'), 0, 0]]);
        o.services.sound.playOnceAt = null; assert.throws(() => e.animation.common.sound(sid, 'x'), /발사 실제 위치/); assert.throws(() => e.missile.firingSound(0, 0, 0), /투사체 실제 위치/); assert.throws(() => e.missile.impactName(0, 0, 'x'), /파일명 위치/);
    } finally { o.dispose(); }
});

test('Session Archer 기하: 실제 현재 SHP/포구·사정선·방향·치수·표적 경계·FencePoint를 연결한다', () => {
    const o = scene();
    try {
        const sid = actor(o, 128); const target = actor(o, 78); const e = o.combatModules.effects; const g = o.projectileGeometry; const from = [20, 20]; const to = [30, 20];
        assert.deepEqual(e.missile.muzzle(sid), g.muzzle(sid)); assert.deepEqual(e.missile.hotspot(sid), g.hotspot(sid)); assert.deepEqual(e.missile.rayEnd(from, to, sid), g.cannonRayEnd(from, to, sid)); assert.deepEqual(e.missile.directionFrame(from, to, 130), g.directionFrame(from, to, 130)); assert.deepEqual(e.missile.targetDimensions(target), g.dimensions(target)); assert.deepEqual(e.missile.targetBounds(target, to), g.targetBounds(target, to)); assert.deepEqual(e.animation.common.targetPoint(sid, target), o.fenceTarget.point(sid, target));
        o.projectileTypes[128].hotspotX = -11; assert.deepEqual(e.missile.hotspot(sid), g.hotspot(sid));
    } finally { o.dispose(); }
});

test('Session Archer 현재 진단: Missile checking은 장기 DWORD를 읽고 진단 함수는 결선 당시 사본이다', () => {
    const calls = []; const o = scene({ ai: { assertion: (expression) => calls.push(['A', expression]) } });
    try {
        const state = o.state.missileState; const checking = o.gameplayGlobals.aiDebug().checking(); checking.value = 0x80000000; assert.equal(state.checking(), 0x80000000); checking.value = 7; assert.equal(state.checking(), 7);
        o.services.ai.assertion = (expression) => calls.push(['B', expression]); o.combatModules.effects.missile.assertion('original', 'ignored', 99); assert.deepEqual(calls, [['A', 'original']]); state.checking = 0; checking.value = 1; assert.equal(state.checking, 0);
    } finally { o.dispose(); }
    const missing = scene(); try { assert.throws(() => missing.combatModules.effects.missile.assertion('x', 'ignored', 123), /Baseprocess.*123/); } finally { missing.dispose(); }
});

test('Session Missile 입력 큐: 보호 명령과 두 DWORD의 signed 비트를 현재 서비스에 전달한다', () => {
    const o = scene(); const calls = [];
    try {
        assert.equal(o.state.missileState.hitCommand, 0x020000b5); const e = o.combatModules.effects;
        assert.throws(() => e.missile.queueCommand(1, 2, 3), /입력 큐 미연결/);
        o.services.assets.queueCommand = (...args) => { calls.push(['A', ...args]); o.services.assets.queueCommand = (...next) => calls.push(['B', ...next]); };
        e.missile.queueCommand(o.state.missileState.hitCommand, 0x80000000, 0xffffffff); e.missile.queueCommand(0xffffffff, 3, 4); assert.deepEqual(calls, [['A', 0x020000b5, -2147483648, -1], ['B', 0xffffffff, 3, 4]]);
    } finally { o.dispose(); }
});

test('Session Missile/빔 시각: Missile 상태와 실제 Core의 현재 시각을 각각 같은 GunHit에 전달한다', () => {
    const o = scene();
    try { const sid = actor(o, 128); const e = o.combatModules.effects; o.state.missileState.now = 8.5; o.core.processState.now = 22.25; e.missile.refresh(sid); assert.equal(o.gunHit.time(sid), 8.5); e.beam.refresh(sid); assert.equal(o.gunHit.time(sid), 22.25); assert.equal(typeof e.missile.damage, 'function'); } finally { o.dispose(); }
});

test('Session Archer 실제 입자: 불꽃11·아이스 냉각21·빔이 같은 Host에 부착되고 종료 때 정리된다', () => {
    const o = scene();
    try {
        const parent = actor(o, 129); const target = actor(o, 78); const e = o.combatModules.effects; const size = o.core.kernel.size;
        e.missile.flightEffect(11, parent); assert.ok(o.core.kernel.size > size); const afterTail = o.core.kernel.size; e.iceCooling(parent); assert.ok(o.core.kernel.size > afterTail); const afterCooling = o.core.kernel.size; e.beam.create(parent, target); assert.ok(o.core.kernel.size > afterCooling); assert.throws(() => e.missile.flightEffect(77, parent), /미복원 TYPE77/);
    } finally { o.dispose(); }
    assert.equal(o.core.kernel.size, 0);
});

test('Session 아이스 반사: 앱 서비스에 변위0 지역점 장치가 없으면 0점(무효)을 공급해 오류 없이 반사만 건너뛴다(E1226)', () => {
    // 앱(netstorm.js)은 ricochetPointBytes 를 공급하지 않는다. 이전에는 변위0 반사에서 '현재 지역점 8BYTE 입력 미연결' 오류가 프레임 루프를 멈췄다.
    const o = scene({});
    try {
        const source = actor(o, 130); const c = o.core; const before = c.kernel.size;
        const request = { firing: source, target: 0, speed: 5, displayType: 130, effectType: 0, targetPosition: [25, 20], targetCenter: [-10000, -10000], ricochet: 1, firingPosition: [20, 20] };
        o.combatModules.ricochetHooks.createProjectile(request); assert.equal(c.kernel.size, before + 1);
        let missile = null; for (let pid = 1; pid <= c.kernel.capacity; ++pid) { const p = c.kernel.get(pid); if (p instanceof MissileProcess) { missile = p; break; } }
        assert.ok(missile);
        const bytes = Uint8Array.of(1, 2, 3, 4, 5, 6, 7, 8); o.combatModules.ricochetHooks.initialPointBytes(bytes); assert.deepEqual([...bytes], [0, 0, 0, 0, 0, 0, 0, 0]);
        // 변위0 반사: 점이 유효하지 않아 다음 반사탄을 만들지 않고 오류 없이 돌아온다.
        o.random.setState(55); o.combatModules.effects.missile.ricochet(missile.fields); assert.equal(c.kernel.size, before + 1);
    } finally { o.dispose(); }
});

test('Session 아이스 반사: 같은 현재 Missile에 실제 표시/ProcessForm을 만들며 프레임/교전은 시작하지 않는다', () => {
    // 변위0에서 원문이 읽는 지역8BYTE를 명시 입력한다. 조준/게임 난수를 대체하지 않는다.
    const pointReads = []; const o = scene({ archer: { ricochetPointBytes: (bytes) => { pointReads.push(bytes.length); const view = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength); view.setFloat32(0, 30, true); view.setFloat32(4, 25, true); } } });
    try {
        const source = actor(o, 130); const c = o.core; const before = c.kernel.size;
        const request = { firing: source, target: 0, speed: 5, displayType: 130, effectType: 0, targetPosition: [25, 20], targetCenter: [-10000, -10000], ricochet: 1, firingPosition: [20, 20] };
        const combat = o.combat; o.combat = null; assert.throws(() => o.combatModules.ricochetHooks.createProjectile(request), /설치 전/); o.combat = combat;
        o.combatModules.ricochetHooks.createProjectile(request); assert.equal(c.kernel.size, before + 1);
        // 같은 실제 Kernel의 새 Missile과 표시 자산의 현재 TYPE/owner만 읽는다.
        let missile = null; for (let pid = 1; pid <= c.kernel.capacity; ++pid) { const p = c.kernel.get(pid); if (p instanceof MissileProcess) { missile = p; break; } }
        assert.ok(missile); assert.equal(c.pool.slot(missile.form)[10], o.state.missileState.processType); assert.equal(c.pool.slot(missile.parent)[10], 130); assert.equal(c.pool.slot(missile.parent)[34], 1); assert.equal(get(missile.fields.bytes, 12), 1);
        // 실제 생성한 표시의 변위0·현재 Game 난수55·같은 Finder/HP를 거쳐 다음 반사탄까지 부착한다.
        o.random.setState(55); o.combatModules.effects.missile.ricochet(missile.fields); assert.deepEqual(pointReads, [8]); assert.equal(c.kernel.size, before + 2);
    } finally { o.dispose(); }
});
