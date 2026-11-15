// 실제 Session의 Carrier/Balloon·TYPE 보충 표와 미부착 Regular 예약을 대조한다(WB04-E579).
// 보호 자료·기존 실제 World/Host를 사용하며 전체 Session/Load·공중3종 전용 관측은 수행하지 않는다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { GameAssets } from '../www/assets/game-assets.js';
import { ShapeDatabase } from '../www/assets/shapes.js';
import { TypeTable, typeNumber } from '../www/assets/type-table.js';
import { SoundList } from '../www/audio/sound-list.js';
import { ClientGameplayGlobals } from '../www/core/client-gameplay-globals.js';
import { ClientPlayerRecords } from '../www/core/client-player-records.js';
import { RawCampaignSessionOwners } from '../www/core/raw-campaign-session-owners.js';
import { RawProjectileGeometry } from '../www/core/raw-projectile-geometry.js';
import { buildProjectileGeometryTypes } from '../www/core/raw-projectile-geometry-types.js';
import { realCaptureAssets } from './capture-assets-support.mjs';
import { put, floatToBits } from './raw-support.mjs';

// 같은 보호 자료를 읽고 실제 owner/월드/Player/SP는 시험마다 새로 만든다.
const data = realCaptureAssets(); const game = new GameAssets(data.table, data.database);
const files = { read() { throw new Error('이 생성 구간의 VFS 읽기 금지'); } };
// 이 구간에서 선택하지 않아야 하는 외부 음향 효과다.
function forbidden() { throw new Error('선택되지 않아야 하는 효과'); }
// 필수 경계만 공급한다. 이름 조회/예약은 실제 SoundList/Host가 수행한다.
function scene(extra = {}, source = game) { return new RawCampaignSessionOwners(source, files, 'thewarbegins', { unixSeconds: () => 1700000000, soundNames: new SoundList(), sound: { wallSeconds: () => 0, playLoopAt: forbidden, playOnceAt: forbidden, play: forbidden, stop: forbidden, isPlaying: forbidden }, ...extra }); }
// 실제 앞 생성자 구간까지 연결한다. 이번 구간의 실패 입력에도 재사용한다.
function beforeTypes(o) { o.initializePlayersAndSp(); o.connectPlayerTablesAndPackets(); o.connectIslandsAndPriests(); o.connectPlayersAndOwnership(); }
// 실제 전체 TYPE 보충 구간까지 순서대로 연결한다.
function connect(o) { beforeTypes(o); o.connectBodiesAndTypeTables(); return o.typeModules; }
// 같은 보호 정의의 지정 블록만 계약 입력으로 확장한 실제 TypeTable/GameAssets다. 원본 객체/파일은 바꾸지 않는다.
function modifiedGame(block, properties, shapes = data.database) {
    const definitions = data.table.definitions.slice(); const source = definitions[block]; definitions[block] = { ...source, properties: [...source.properties, ...properties] };
    return new GameAssets(new TypeTable(definitions), shapes);
}

test('Session 몸체/TYPE: 실제 앞 단계·벡터 참조와 원문0초기값/미지정Run-1을 유지한다', () => {
    const o = scene();
    try {
        assert.throws(() => o.connectBodiesAndTypeTables(), /초기화 전/); assert.equal(o.carrier, null); const rows = o.gunTypes; const first = rows[70]; const themes = o.themes; const archer = o.archerTypes[70];
        assert.equal(rows.length, 188); assert.equal(o.builderTypes[0].firstFrames.every((value) => value === 0), true); assert.notEqual(rows[0].directionFrames, rows[1].directionFrames);
        o.core.types[0].theme = -7; const m = connect(o); assert.equal(m.ready, true); assert.equal(o.gunTypes, rows); assert.equal(rows[70], first); assert.equal(o.themes, themes); assert.equal(o.archerTypes[70], archer);
        assert.equal(o.carrier.pool, o.core.pool); assert.equal(o.balloonBody.pool, o.core.pool); assert.equal(o.gunTypes[0].theme, -7); assert.equal(o.themes[0], 0xfffffff9); assert.equal(o.elements[0], -7); assert.equal(o.builderTypes[0].sourceFrameIndex, 0xfffffff9);
        assert.equal(o.builderTypes[0].firstFrames.every((value) => value === -1), true); assert.equal(o.gunTypes[0].directionFrames.every((value) => value === 0), true); assert.equal(o.gunTypes[0].range, 0);
        assert.throws(() => o.connectBodiesAndTypeTables(), /반복/); assert.throws(() => o.gameplayGlobals.requireSession(), /미연결/);
    } finally { o.dispose(); }
    assert.throws(() => o.connectBodiesAndTypeTables(), /반납/);
});

test('Session TYPE 보충 표: 보호116정의/188행이 기존 기하 빌더·실제 Run/숫자와 일치한다', () => {
    const o = scene();
    try {
        connect(o); const expected = buildProjectileGeometryTypes(o.assets.game.typeTable.definitions, o.core.shapes);
        assert.deepEqual(o.projectileTypes, expected); let physicalFrames = 0;
        // 내장/빈 타입까지 현재 theme와A..P 첫 프레임을 같은 번호로 전달한다.
        for (let type = 0; type < 188; type += 1) {
            assert.equal(o.themes[type], o.core.types[type].theme >>> 0); assert.equal(o.elements[type], o.core.types[type].theme); assert.equal(o.gunTypes[type].theme, o.core.types[type].theme);
            for (let index = 0; index < 16; index += 1) assert.equal(o.builderTypes[type].firstFrames[index], o.core.frames[type].run(String.fromCharCode(65 + index)).first);
        }
        // 실제116블록의 숫자·방향·피해·소리를 읽으며 내부 타입의 기본 행과 혼합하지 않는다.
        for (const asset of game.types()) {
            const type = 70 + asset.block; const definition = asset.definition; physicalFrames += o.projectileTypes[type].shotOffsets.length;
            assert.equal(o.gunTypes[type].range, Math.fround(typeNumber(definition, 'range') ?? 0)); assert.equal(o.dudeSpeeds[type].speed, Math.fround(typeNumber(definition, 'speed') ?? 0)); assert.equal(o.builderTypes[type].speed, o.dudeSpeeds[type].speed);
            assert.equal(o.hitTypes[type].damagePerSecond, Math.fround(typeNumber(definition, 'hpPerSec') ?? 0)); assert.equal(o.missileTypes[type].damagePerSecond, o.hitTypes[type].damagePerSecond); assert.equal(o.missileTypes[type].specialFlyerDamage, o.hitTypes[type].specialFlyerDamage); assert.equal(o.missileTypes[type].flyerDamagePerSecond, o.hitTypes[type].flyerDamagePerSecond);
            assert.equal(o.firingIntervals[type], Math.fround(typeNumber(definition, 'delayBetweenShots') ?? 0)); assert.equal(o.missileTypes[type].frameCount, o.archerTypes[type].directions); assert.equal(o.projectileTypes[type].frameCount, definition.clusters.length);
            for (let direction = 0; direction < 4; direction += 1) assert.equal(o.gunTypes[type].directionFrames[direction], o.core.frames[type].run(String.fromCharCode(76 + direction)).first);
        }
        assert.ok(physicalFrames > 1000);
    } finally { o.dispose(); }
});

test('Session 숫자/이름 계약: 마지막 AST·float 저장·int 절삭·fireSound 정확한 키를 보존한다', () => {
    const source = modifiedGame(0, [{ name: 'range', value: 1 }, { name: 'RANGE', value: 12.3456789 }, { name: 'height', value: -3.75 }, { name: 'speed', value: 0.123456789 }, { name: 'hpPerSec', value: 3.456789 }, { name: 'useairdamage', value: 0.75 }, { name: 'airdamage', value: 5.6789123 }, { name: 'delayBetweenShots', value: 0.0123456789 }, { name: 'orientations', value: 4.9 }, { name: 'fireSound', value: '' }, { name: 'firingSound', value: 'ignored-alias.wav' }, { name: 'impactSound', value: 'type-impact.wav' }]); const o = scene({}, source); const names = []; const lookup = o.soundNames.lookup.bind(o.soundNames);
    o.soundNames.lookup = (name) => { names.push(name); return lookup(name); };
    try {
        connect(o); assert.equal(o.gunTypes[70].range, Math.fround(12.3456789)); assert.equal(o.gunTypes[70].heightExtension, -3); assert.equal(o.hitTypes[70].specialFlyerDamage, false); assert.equal(o.missileTypes[70].flyerDamagePerSecond, Math.fround(5.6789123)); assert.equal(o.archerTypes[70].directions, 4);
        assert.equal(o.dudeSpeeds[70].speed, Math.fround(0.123456789)); assert.equal(o.firingIntervals[70], Math.fround(0.0123456789)); assert.equal(o.missileTypes[70].firingSound, 0); assert.equal(o.missileTypes[70].impactSound, lookup('type-impact.wav')); assert.equal(names.includes('ignored-alias.wav'), false);
    } finally { o.dispose(); }
    const edge = scene({}, modifiedGame(0, [{ name: 'height', value: 2147483648 }, { name: 'useairdamage', value: -1.75 }, { name: 'range', value: 0 }]));
    try { connect(edge); assert.equal(edge.gunTypes[70].heightExtension, -2147483648); assert.equal(edge.hitTypes[70].specialFlyerDamage, true); assert.equal(edge.gunTypes[70].range, 0); } finally { edge.dispose(); }
});

test('Session 투사체 입력: 실제 썬 디스크 발사기의 포구는 같은 SHP signed WORD를 사용한다', () => {
    const o = scene();
    try {
        connect(o); const sid = o.world.create(71, 2); const raw = o.core.pool.allocated(sid); put(raw, 14, floatToBits(20)); put(raw, 18, floatToBits(30)); put(raw, 36, 1);
        const body = new RawProjectileGeometry(o.core.pool, o.core.types, o.projectileTypes); const first = o.projectileTypes[71].defaultFrame;
        const a = game.shapes.squidHeader(1, first); const b = game.shapes.squidHeader(1, 1); const av = new DataView(a.buffer, a.byteOffset, a.byteLength); const bv = new DataView(b.buffer, b.byteOffset, b.byteLength);
        assert.deepEqual(body.archerMuzzle(sid), [Math.fround((av.getInt16(32, true) + bv.getInt16(32, true)) / 16 + 20), Math.fround((av.getInt16(34, true) + bv.getInt16(34, true)) / 11 + 30)]);
    } finally { o.dispose(); }
});

test('Session 클라우드 플로터: 실제 생성/Pop이 같은 Balloon/Carrier와 Regular3을 사용한다', () => {
    const o = scene();
    try {
        connect(o); o.core.processState.now = 17.25; const type = 70 + game.find('rainBalloon').block; assert.equal(type, 146); const sid = o.world.create(type, 2); assert.equal(o.balloonBody.handles(sid), true); assert.equal(o.balloonBody.robust(sid), true);
        o.world.pop(sid, 20, 20, 0); const regular = o.core.processes.findEvent(sid, 3); assert.ok(regular); assert.equal(regular.payload, Math.fround(0.1)); assert.equal(regular.time, 17.25); assert.equal(regular.count, 0); assert.equal(o.pendingCloudRegular, null); assert.equal(o.core.bookkeeping.depth, 0);
    } finally { o.dispose(); }
});

test('Session Regular 예약: 같은 객체를 재초기화하고 실제 부착 뒤의 현재 double 시각을 쓴다', () => {
    const o = scene();
    try {
        const m = connect(o); const parent = o.world.create(82, 2); assert.throws(() => m.attachCloudRegular(parent, 3, 0.1), /예약 누락/); assert.equal(m.reserveCloudRegular(), true);
        const reserved = o.pendingCloudRegular; assert.equal(reserved.host, null); assert.equal(o.core.kernel.size, 0); reserved.setInitialCount(9); reserved.setNextTime(99); assert.throws(() => m.reserveCloudRegular(), /중복 예약/);
        // 부착 경계만 관찰하며 실제 Host의 등록/연결은 그대로 실행한다.
        reserved.onAttached = () => { assert.deepEqual([reserved.event, reserved.payload, reserved.count, reserved.time], [3, Math.fround(0.1), 0, 0]); o.core.processState.now = 22.75; };
        o.core.processState.now = 17.25; m.attachCloudRegular(parent, 3, 0.1); assert.equal(o.core.processes.findEvent(parent, 3), reserved); assert.equal(o.core.processes.fromForm(reserved.form), reserved); assert.equal(reserved.time, 22.75); assert.equal(o.pendingCloudRegular, null);
        assert.throws(() => reserved.initializeDetached(7, 1), /부착한 Regular/); assert.equal(reserved.event, 3);
    } finally { o.dispose(); }
});

test('Session TYPE 종료: 미소비 예약/현재 프로세스만 반납하고 벡터 값을 임의로 초기화하지 않는다', () => {
    const o = scene(); const m = connect(o); const range = o.gunTypes[128].range; assert.equal(m.reserveCloudRegular(), true); const pending = o.pendingCloudRegular;
    o.dispose(); assert.equal(o.pendingCloudRegular, null); assert.equal(pending.host, null); assert.equal(o.core.kernel.size, 0); assert.equal(o.gunTypes[128].range, range); assert.equal(o.world.storage.borrowed, false); o.dispose();
});

test('Session SHP 부분 실패: 같은 이름 조회 뒤 헤더를 읽고 앞선 벡터/몸체/물리 프레임을 유지한다', () => {
    const shapes = new ShapeDatabase(data.database.bytes); const source = modifiedGame(1, [{ name: 'fireSound', value: 'before-header-fire.wav' }, { name: 'impactSound', value: 'before-header-impact.wav' }], shapes); const o = scene({}, source); const events = []; const lookup = o.soundNames.lookup.bind(o.soundNames); const header = shapes.squidHeader.bind(shapes);
    try {
        beforeTypes(o); o.soundNames.lookup = (name) => { events.push(name); return lookup(name); }; shapes.squidHeader = (block, frame) => { if (block === 1 && frame === 0) { events.push('header-stop'); throw new Error('헤더 경계 중단'); } return header(block, frame); };
        assert.throws(() => o.connectBodiesAndTypeTables(), /헤더 경계 중단/); assert.deepEqual(events.slice(-3), ['before-header-fire.wav', 'before-header-impact.wav', 'header-stop']);
        assert.ok(o.carrier); assert.ok(o.balloonBody); assert.equal(o.typeModules.ready, false); assert.equal(o.projectileTypes[70].shotOffsets.length, o.core.shapes[70].frames.length); assert.deepEqual(o.projectileTypes[71].shotOffsets, []); assert.equal(o.missileTypes[71].firingSound, lookup('before-header-fire.wav')); assert.equal(o.gunTypes[72].range, 0);
        assert.throws(() => o.connectBodiesAndTypeTables(), /반복/);
    } finally { o.dispose(); }
});

test('Session AST 검증: 잘못된 숫자는 선행 자산 구성에서 거부하고 실제 전역/Player 대여를 반환한다', () => {
    // TYPE 기본 구성에서 먼저 읽는 필드는 후속 보충 단계까지 통과한 것처럼 시험하지 않는다.
    for (const [name, message] of [['range', /숫자여야/], ['orientations', /directions 숫자 AST/]]) {
        const gameplayGlobals = new ClientGameplayGlobals(); const playerRecords = new ClientPlayerRecords(); const source = modifiedGame(0, [{ name, value: 'wrong-number' }]);
        assert.throws(() => scene({ gameplayGlobals, playerRecords }, source), message);
        const gameplay = gameplayGlobals.acquire(); const players = playerRecords.acquire(); players.release(); gameplay.release(); assert.equal(playerRecords.base(), 0);
    }
});
