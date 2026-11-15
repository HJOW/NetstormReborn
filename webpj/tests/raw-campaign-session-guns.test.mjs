// Session1398..1426의 실제 Gun 피해·기하·기존 Form 조회를 보호 자산/World/Host에서 대조한다(WB04-E582).
// 고정 소스 계약 입력이며 원본 게임/실제 교전·공중3종 전용 생성/행동·새 원격MP 실행은 하지 않는다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { GameAssets } from '../www/assets/game-assets.js';
import { SoundList } from '../www/audio/sound-list.js';
import { RawCampaignSessionOwners } from '../www/core/raw-campaign-session-owners.js';
import { RawCampaignAssetBindings } from '../www/core/raw-campaign-asset-bindings.js';
import { RawGunProcess, GunProcess, GunProcessFields } from '../www/core/raw-gun-process.js';
import { SquidProcess } from '../www/core/squid-process.js';
import { realCaptureAssets } from './capture-assets-support.mjs';
import { get, put, floatToBits } from './raw-support.mjs';

// 같은 읽기 전용 보호 자산과 생성 구간에서 읽지 않는 VFS 경계다.
const data = realCaptureAssets(); const game = new GameAssets(data.table, data.database); const files = { read() { throw new Error('생성 구간의 VFS 읽기 금지'); } };
// 이 구간에서 선택하지 않은 외부 효과는 명시 오류다.
function forbidden() { throw new Error('선택되지 않은 외부 장치'); }
// 필수 외부 소리 장치만 보관하며 실제 게임/풀/프로세스는 owner가 소유한다.
function scene(extra = {}) { return new RawCampaignSessionOwners(game, files, 'thewarbegins', { unixSeconds: () => 1700000000, soundNames: new SoundList(), sound: { wallSeconds: () => 0, play: forbidden, playOnceAt: forbidden, playLoopAt: forbidden, stop: forbidden, isPlaying: forbidden }, ...extra }); }
// 실제 앞 생성 구간까지만 순서대로 연결한다.
function beforeGuns(o) { o.initializePlayersAndSp(); o.connectPlayerTablesAndPackets(); o.connectIslandsAndPriests(); o.connectPlayersAndOwnership(); o.connectBodiesAndTypeTables(); o.connectGunAndCursorViews(); o.connectHitBodies(); }
// 실제 이번 생성 구간까지 연결한다.
function connect(o) { beforeGuns(o); o.connectGunBodies(); }
// 실제 자산 생성자 위에 고정 좌표/HP/owner 입력만 둔다. Pop/교전 성공을 대신하지 않는다.
function actor(o, type, owner = 2) { const sid = o.world.create(type, 2); const raw = o.core.pool.allocated(sid); raw[34] = owner; put(raw, 14, floatToBits(20)); put(raw, 18, floatToBits(20)); put(raw, 26, 10000); return sid; }
// 기존 Gun 조회용 실제 Kernel 객체다. 무전송 flags50으로 부착하며 생성 부모/송신 완료 증거로 세지 않는다.
function gun(o, parent, logic = null) {
    logic ??= new RawGunProcess(o.core.pool, o.core.types, o.state.gunState.globals()); const fields = new GunProcessFields(); logic.initialize(fields, parent); const process = new GunProcess(logic, fields); o.core.processes.attach(process, o.state.gunState.processType, parent, 0x50); return process;
}

test('Session Gun 몸체: 실제 Hit 뒤 같은 Pool/표·초기 최대값·반복/반납/미공개 관문을 유지한다', () => {
    const o = scene();
    try { assert.deepEqual(o.state.gunBlockerState, { cannonRange: 0, archerRange: 0, stoneTowerType: 78 }); assert.throws(() => o.connectGunBodies(), /초기화 전/); assert.equal(o.gunHit, null); connect(o); assert.equal(o.gunModules.ready, true); assert.equal(o.gunHit.pool, o.core.pool); assert.equal(o.projectileGeometry.pool, o.core.pool); assert.ok(o.fenceTarget); assert.ok(o.gunBlocker); assert.throws(() => o.connectGunBodies(), /반복/); assert.throws(() => o.gameplayGlobals.requireSession(), /미연결/); } finally { o.dispose(); }
    assert.throws(() => o.connectGunBodies(), /반납/);
});

test('Session 검색 최대값: 실제 전체 TYPE와 float32 여유·NaN 후보의 std::max 순서를 보존한다', () => {
    const o = scene();
    try {
        beforeGuns(o); let cannon = 0; let archer = 0;
        // 실제 보호 자료의 유한 범위를 독립 집계해 부모가 같은 표를 소비하는지 확인한다.
        for (let type = 0; type < 188; type += 1) { const group = o.core.types[type].group; if (group === 1) cannon = Math.max(cannon, Math.fround(o.gunTypes[type].range + 10)); else if (group === 0) archer = Math.max(archer, Math.fround(o.gunTypes[type].range + 3)); }
        o.connectGunBodies(); assert.equal(o.state.gunBlockerState.cannonRange, cannon); assert.equal(o.state.gunBlockerState.archerRange, archer);
    } finally { o.dispose(); }
    const edge = scene();
    try {
        beforeGuns(edge);
        // 구성 메타데이터 입력만 바꾼다. 공중 유닛을 생성하거나 행동을 관측하지 않는다.
        for (const type of edge.core.types) type.group = 2;
        edge.core.types[0].group = 1; edge.gunTypes[0].range = -100; edge.core.types[1].group = 1; edge.gunTypes[1].range = 16777216; edge.core.types[2].group = 0; edge.gunTypes[2].range = 16777216;
        edge.core.types[3].group = 1; edge.gunTypes[3].range = NaN; edge.core.types[4].group = 0; edge.gunTypes[4].range = NaN; edge.gunTypes[5].range = Infinity;
        edge.connectGunBodies(); assert.equal(edge.state.gunBlockerState.cannonRange, 16777226); assert.equal(edge.state.gunBlockerState.archerRange, 16777220);
    } finally { edge.dispose(); }
});

test('Session GunHit: 실제 일반 피해와 target0/free 선관문·미복원 TYPE 진단을 연결한다', () => {
    const o = scene();
    try {
        connect(o); const source = actor(o, 71, 3); const target = actor(o, 75); assert.equal(o.gunModules.applyFlyerHit(source, 0, 1), false); o.gunHit.apply(source, 0, 5); o.gunHit.apply(source, target, 7); assert.equal(get(o.core.pool.slot(target), 26), 9993);
        o.core.pool.allocated(target)[11] |= 1; o.gunHit.apply(source, target, 5); assert.equal(get(o.core.pool.slot(target), 26), 9993); o.core.pool.physical(target)[11] &= ~1;
        put(o.core.pool.physical(target), 0, 0x12345678); assert.throws(() => o.gunHit.apply(source, target, 1), /실제 세션 총 피해 미복원: 공격 TYPE71 → 표적 TYPE75/);
    } finally { o.dispose(); }
});

test('Session GunHit 시각: 같은 WORD/현재 clock과 실제 Building 통지를 사용한다', () => {
    const o = scene();
    try {
        connect(o); const sid = actor(o, 128); const raw = o.core.pool.allocated(sid); raw[12] |= 1; const events = []; const notify = o.core.building.notifySurface.bind(o.core.building); o.core.building.notifySurface = (value) => { events.push(value); return notify(value); };
        o.gunHit.setTime(sid, 12.5); assert.equal(o.gunHit.time(sid), 12.5); assert.equal(get(raw, 12, 2) & 1, 1); assert.deepEqual(events, [sid]); o.gunHit.setTime(sid, 13.875); assert.equal(o.gunHit.time(sid), 13.875); assert.deepEqual(events, [sid, sid]);
    } finally { o.dispose(); }
});

test('Session 투사체: 같은 실제 SHP 포구와 현재 Gun 모드로 캐논 사정선을 보정한다', () => {
    const o = scene();
    try {
        connect(o); const archer = actor(o, 71); const raw = o.core.pool.allocated(archer); put(raw, 36, 1); const a = game.shapes.squidHeader(1, o.projectileTypes[71].defaultFrame); const b = game.shapes.squidHeader(1, 1); const av = new DataView(a.buffer, a.byteOffset, a.byteLength); const bv = new DataView(b.buffer, b.byteOffset, b.byteLength);
        assert.deepEqual(o.projectileGeometry.archerMuzzle(archer), [Math.fround((av.getInt16(32, true) + bv.getInt16(32, true)) / 16 + 20), Math.fround((av.getInt16(34, true) + bv.getInt16(34, true)) / 11 + 20)]);
        const cannon = actor(o, 129); const c = o.core.pool.allocated(cannon); o.projectileTypes[129].range = 7.75; put(c, 36, o.core.frames[129].run('L').first); assert.deepEqual(o.projectileGeometry.cannonRayEnd([20, 20], [30, 20], cannon), [20, 13]); put(c, 36, o.core.frames[129].run('M').first); assert.deepEqual(o.projectileGeometry.cannonRayEnd([20, 20], [30, 20], cannon), [30, 20]);
    } finally { o.dispose(); }
});

test('Session 배리어 포구: 같은 현재 SHP 표·타입 hotspot·픽셀 배율과 모드를 읽는다', () => {
    const o = scene();
    try {
        connect(o); const sid = actor(o, 128); const h = o.gunModules.blockerHooks; const type = o.core.types[128]; o.projectileTypes[128].shotOffsets[0] = [37, -21];
        const expected = (x, y) => [Math.fround(Math.fround(20 + 37 / x) - type.hotspotX / x), Math.fround(Math.fround(20 - 21 / y) - type.hotspotY / y)]; assert.deepEqual(h.muzzle(sid), expected(16, 11)); o.state.gunTargetState.scaleX = 32; assert.deepEqual(h.muzzle(sid), expected(32, 11));
        const frame = o.core.frames[128].run('L').first; put(o.core.pool.allocated(sid), 36, frame); assert.equal(h.mode(sid), o.gunMode.get(sid)); assert.deepEqual(o.projectileGeometry.muzzle(sid), [20, 20]);
    } finally { o.dispose(); }
});

test('Session 기존 Gun 조회: 같은 Form/PID·실제 표적0과 현재 processType을 재조회한다', () => {
    const o = scene();
    try {
        connect(o); const sid = actor(o, 128); const h = o.gunModules.blockerHooks; assert.equal(h.currentTarget(sid), null); h.setTarget(sid, 0); assert.equal(o.core.kernel.size, 0);
        const current = gun(o, sid); assert.equal(h.currentTarget(sid), 0); const target = actor(o, 75); put(current.fields.bytes, 0x18, target, 2); assert.equal(h.currentTarget(sid), target); assert.equal(o.core.processes.fromForm(current.form), current); assert.throws(() => h.setTarget(sid, target), /실제 Gun 처리기 미설치/);
        o.state.gunState.processType = 33; assert.equal(h.currentTarget(sid), null); h.setTarget(sid, target); assert.equal(o.core.kernel.size, 1);
    } finally { o.dispose(); }
});

test('Session 기존 Gun 불일치: 현재 Form의 실제 다른 Kernel 타입을 새 Gun으로 바꾸지 않는다', () => {
    const o = scene();
    try { connect(o); const sid = actor(o, 128); const process = new SquidProcess(); o.core.processes.attach(process, 34, sid, 0x50); const h = o.gunModules.blockerHooks; assert.throws(() => h.currentTarget(sid), /현재 Gun 타입 불일치/); assert.throws(() => h.setTarget(sid, 0), /기존 Gun 수명 불일치/); assert.equal(o.core.kernel.size, 1); } finally { o.dispose(); }
});

test('Session 기존 Gun 표적 지정: 실제 자산 Gun 상태 기계·감시 종속·같은 Fields를 사용한다', () => {
    const o = scene(); const events = [];
    try {
        connect(o); const s = o.state; o.assetBindings = new RawCampaignAssetBindings(o.world, o.carrier, s.assetState, o.random, o.islands, o.players, o.playerState, { gun: s.gunState, regular: s.regularState, elements: o.elements }, { gun: { refreshRange: (sid) => o.gunHit.setTime(sid, o.core.processState.now) }, sendGunPacket: (packet) => events.push(packet.length) });
        o.core.processes.setFormTransmit(o.assetBindings.transmission()); const sid = actor(o, 128); const current = gun(o, sid, o.assetBindings.gun); const target = actor(o, 75); o.gunModules.blockerHooks.setTarget(sid, target);
        assert.equal(current.fields.target, target); assert.equal(current.fields.stage, 2); assert.equal(o.gunModules.blockerHooks.currentTarget(sid), target); assert.equal(o.core.kernel.size, 1); assert.ok(events.length > 0);
        const watcher = get(o.core.pool.slot(target), 6, 2); assert.ok(watcher); assert.equal(o.core.dependencies.watcher(watcher), current.form);
    } finally { o.dispose(); }
});

test('Session 장애물 수집: 빈 실제 해시와 현재 Boss의 조기 반환은 Gun을 생성하지 않는다', () => {
    const o = scene();
    try { connect(o); assert.deepEqual(o.gunBlocker.collect(1, [20, 20]), []); o.gameplayGlobals.authority().boss = false; o.gunBlocker.notify(0); assert.equal(o.core.kernel.size, 0); } finally { o.dispose(); }
});

test('Session Gun 부분 실패: 실제 앞선 세 몸체/최대값을 유지하고 반복을 거부한 뒤 대여를 반환한다', () => {
    const o = scene();
    try { beforeGuns(o); o.state.gunBlockerState.stoneTowerType = 188; assert.throws(() => o.connectGunBodies(), /장애물 통지/); assert.ok(o.gunHit); assert.ok(o.projectileGeometry); assert.ok(o.fenceTarget); assert.equal(o.gunBlocker, null); assert.ok(o.state.gunBlockerState.cannonRange > 0); assert.equal(o.gunModules.ready, false); assert.throws(() => o.connectGunBodies(), /반복/); } finally { o.dispose(); }
    const p = o.playerRecords.acquire(); p.release(); const g = o.gameplayGlobals.acquire(); g.release();
});
