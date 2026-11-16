// Session1427..1462의 같은 실제 자산·월드·팔레트·장기 카메라·현재 장치를 대조한다(WB04-E584).
// 고정 소스 계약 입력이며 원본/클론 게임·브라우저·새 원격MP·공중3종 전용 생성/행동 관측은 하지 않는다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { readData } from './helpers.mjs';
import { realCaptureAssets } from './capture-assets-support.mjs';
import { get, put, floatToBits } from './raw-support.mjs';
import { GameAssets, DEFAULT_GAME_PALETTE_PATH } from '../www/assets/game-assets.js';
import { GamePalette } from '../www/assets/palette.js';
import { SoundList } from '../www/audio/sound-list.js';
import { RawCampaignSessionOwners } from '../www/core/raw-campaign-session-owners.js';
import { ClientColorSortInput } from '../www/core/client-color-sort-input.js';
import { RawCampaignParticles } from '../www/render/raw-campaign-particles.js';
import { RawShapeToBuffer, createShapeToBufferState } from '../www/render/raw-shape-to-buffer.js';
import { drawMappedShape } from '../www/render/vfx-mapped-draw.js';
import { drawShape } from '../www/render/vfx-draw.js';
import { TypeFlag2 } from '../www/assets/type-flags.js';

// 기존 실제 COL/116TYPE/SHP를 같은 GameAssets에 연결한다. 파일시스템 읽기에는 보존된 대문자 파일명을 쓴다.
const data = realCaptureAssets(); const palette = GamePalette.parse(readData('d/GIFCLOUD.COL')); const game = new GameAssets(data.table, data.database, palette);
// 선택하지 않은 외부 장치는 빈 성공으로 대체하지 않는다.
function forbidden() { throw new Error('선택되지 않은 외부 장치'); }
// 실제 기반 owner와 앞 생성 구간을 조립한다. 전체 Session/Load를 가장하지 않는다.
function scene(extra = {}, selected = game) { return new RawCampaignSessionOwners(selected, { read: forbidden }, 'thewarbegins', { unixSeconds: () => 1700000000, soundNames: new SoundList(), sound: { wallSeconds: () => 0, play: forbidden, playOnceAt: forbidden, playLoopAt: forbidden, stop: forbidden, isPlaying: forbidden }, ...extra }); }
// 실제 앞 구간의 정상 복귀 뒤 이번 결선을 실행한다.
function before(o) { o.initializePlayersAndSp(); o.connectPlayerTablesAndPackets(); o.connectIslandsAndPriests(); o.connectPlayersAndOwnership(); o.connectBodiesAndTypeTables(); o.connectGunAndCursorViews(); o.connectHitBodies(); o.connectGunBodies(); }
// 같은 실제 자산 생성자 위에 고정 수치 입력만 넣는다. Pop/전투/주문 시전을 대신하지 않는다.
function actor(o, type, owner = 2) { const sid = o.world.create(type, 2); const raw = o.core.pool.allocated(sid); raw[34] = owner; put(raw, 14, floatToBits(20)); put(raw, 18, floatToBits(20)); put(raw, 26, 10000); return sid; }

test('GameAssets 팔레트: 실제 기본 COL과 같은 객체를 빌리고 누락/가짜 입력을 거부한다', () => {
    assert.equal(DEFAULT_GAME_PALETTE_PATH, 'd/gifcloud.col'); assert.equal(game.palette, palette); assert.equal(game.palette.logical.length, 256);
    const old = new GameAssets(data.table, data.database); assert.equal(old.shapes, data.database); assert.throws(() => old.palette, /실제 팔레트 미연결/); assert.throws(() => new GameAssets(data.table, data.database, { logical: palette.logical }), /실제 GamePalette/);
});

test('Session 입자: 실제 Gun 뒤 초기 멤버·같은 Pool/CRT와 반복/반납/미공개 관문을 유지한다', () => {
    const o = scene();
    try {
        assert.equal(o.shrapnelMapTokens.length, 188); assert.ok(o.shrapnelMapTokens.every((value) => value === 0)); assert.equal(o.shrapnelOwnerColors.size, 0); assert.equal(o.bridgeDrawState.lifeNumbers, 0); assert.equal(o.shrapnelCaptureState.addTypeHotspot, false);
        assert.throws(() => o.connectParticles(), /초기화 전/); before(o); const gameState = o.random.state; const crtState = o.particleCrt.state; o.connectParticles();
        assert.ok(o.particles instanceof RawCampaignParticles); assert.equal(o.particleModules.ready, true); assert.equal(o.shrapnelCapture.pool, o.core.pool); assert.equal(o.particleModules.shrapnel.crt, o.particleCrt); assert.equal(o.random.state, gameState); assert.equal(o.particleCrt.state, crtState); assert.equal(o.shrapnelCaptureState.addTypeHotspot, true);
        assert.throws(() => o.connectParticles(), /반복/); assert.throws(() => o.gameplayGlobals.requireSession(), /미연결/);
    } finally { o.dispose(); }
    o.dispose(); assert.throws(() => o.connectParticles(), /반납/);
});

test('Session 입자 실패 접두: 팔레트 누락/다리 TYPE 범위에서 앞선 효과와 재실행 거부를 보존한다', () => {
    const o = scene({}, new GameAssets(data.table, data.database));
    try { before(o); assert.throws(() => o.connectParticles(), /실제 팔레트 미연결/); assert.equal(o.particleModules.ready, false); assert.equal(o.shrapnelOwnerColors.size, 3); assert.ok(o.shrapnelCapture); assert.equal(o.particles, null); assert.throws(() => o.connectParticles(), /반복/); } finally { o.dispose(); }
    // 원문의 std::array::at는 경계/최대 DWORD를 첫 토큰 쓰기 전에 거부한다.
    for (const type of [188, 0xffffffff]) {
        const invalid = scene();
        try { before(invalid); invalid.core.bridgeState.bridgeType = type; assert.throws(() => invalid.connectParticles(), /파편 소유자 색표 TYPE 물리 범위/); assert.ok(invalid.shrapnelMapTokens.every((value) => value === 0)); assert.equal(invalid.shrapnelOwnerColors.size, 0); assert.equal(invalid.shrapnelCapture, null); assert.equal(invalid.particleModules.ready, false); assert.throws(() => invalid.connectParticles(), /반복/); } finally { invalid.dispose(); }
    }
});

test('Session 파편 지역 색표: 정확한 세 TYPE/188토큰과 현재 owner→색 번호를 다시 읽는다', () => {
    const o = scene();
    try {
        before(o); o.connectParticles(); const s = o.shrapnelCaptureState; assert.equal(s.ownerTypeMaps, o.shrapnelMapTokens); assert.deepEqual([...o.shrapnelOwnerColors.keys()], [82, 73, 144]); assert.deepEqual([...o.shrapnelMapTokens.entries()].filter(([, value]) => value).map(([type]) => type), [73, 82, 144]);
        o.core.bridgeState.ownerColors[2] = 5; assert.equal(s.ownerColors(82, 2), o.shrapnelOwnerColors.get(82)[5]); o.core.bridgeState.ownerColors[2] = 7; assert.equal(s.ownerColors(82, 2), o.shrapnelOwnerColors.get(82)[7]);
        assert.throws(() => s.ownerColors(75, 2), /현재 타입\/owner/); assert.throws(() => s.ownerColors(82, 9), /현재 타입\/owner/); o.core.bridgeState.ownerColors[2] = -1; assert.throws(() => s.ownerColors(82, 2), /현재 소유자 색 번호/); o.core.bridgeState.ownerColors[2] = 9; assert.throws(() => s.ownerColors(82, 2), /현재 소유자 색 번호/);
    } finally { o.dispose(); }
});

test('Session 파편 공통 색표: 같은 실제 OwnerColorTableState의 토큰/현재 색 행을 빌린다', () => {
    const colors = new ClientColorSortInput(); colors.captureWithPrimary(palette.logical); colors.ownerTables.initialize(colors.primary, 188, 82, { value: 0 }); const o = scene({ ownerColorTables: colors.ownerTables });
    try { before(o); o.connectParticles(); const s = o.shrapnelCaptureState; assert.equal(s.ownerTypeMaps, colors.ownerTables.tokens); o.core.bridgeState.ownerColors[2] = 5; assert.equal(s.ownerColors(82, 2), colors.ownerTables.color(82, 5)); assert.throws(() => s.ownerColors(82, 9), /공통.*owner/); o.core.bridgeState.ownerColors[2] = -1; assert.throws(() => s.ownerColors(82, 2), /공통.*색 번호/); o.core.bridgeState.ownerColors[2] = 9; assert.throws(() => s.ownerColors(82, 2), /공통.*색 번호/); } finally { o.dispose(); }
});

test('Session 다리 파편 캡처: 실제 RLE 전체 픽셀/현재 색표·TYPE 기준점과 기본0 추가 그림을 연결한다', () => {
    const o = scene();
    try {
        before(o); o.connectParticles(); const sid = actor(o, 82); const raw = o.core.pool.allocated(sid); raw[40] |= 2; put(raw, 36, 0); o.core.bridgeState.ownerColors[2] = 5; o.core.types[82].hotspotX = -2; o.core.types[82].hotspotY = 3;
        const header = o.core.shapes[82].frames[0]; const expected = new Uint8Array(header.width * header.height).fill(255); drawMappedShape(expected, header.width, header.height, game.shapes, 12, 0, header.hotspotX, header.hotspotY, { left: 0, top: 0, right: header.width - 1, bottom: header.height - 1 }, o.shrapnelOwnerColors.get(82)[5]);
        const image = o.particleModules.shrapnel.lifetime.capture(sid); assert.equal(image.width, header.width); assert.equal(image.height, header.height); assert.equal(image.hotspotX, header.hotspotX - 2); assert.equal(image.hotspotY, header.hotspotY + 3); assert.deepEqual(image.pixels, expected);
        o.bridgeDrawState.lifeNumbers = 1; assert.throws(() => o.particleModules.capture(sid), /수명 숫자 표시 미복원/);
    } finally { o.dispose(); }
});

test('Session Devastation 추가 그림: 같은16VT 선택·현재 signed 프레임과 실제 plain RLE/도움말 제외를 연결한다', () => {
    const o = scene();
    try {
        before(o); o.connectParticles(); const sid = actor(o, 168, 0); const raw = o.core.pool.allocated(sid); const s = o.shrapnelCaptureState;
        // 자격 표만 바꾸며 원문과 같은16VT의 빈 기본 그림을 확인한다. 실제 주문/공중 유닛은 생성하지 않는다.
        for (const table of [0x503188, 0x5026a0, 0x502778, 0x502840, 0x502908, 0x502b70, 0x5029d0, 0x502aa8, 0x5030c0, 0x502448, 0x502370, 0x502510, 0x502ff8, 0x5025d8, 0x502e68, 0x502f30]) { put(raw, 0, table); assert.equal(s.mainFrame(sid), -1); }
        // TYPE168의 frame0은 실제 투명 프레임이므로 SHP의 기존 불투명 프레임을 입력으로 선택한다.
        const frame = game.shapes.blocks[98].frames.findIndex((header, index) => !header.special && index !== game.types()[98].definition.helpFrame && game.shapes.decode(98, index).opacity.some((value) => value)); assert.ok(frame >= 0); put(raw, 36, frame);
        const image = { width: 1024, height: 1024, pixels: new Uint8Array(1048576).fill(255) }; const expected = image.pixels.slice(); drawShape(expected, 1024, 1024, game.shapes, 98, frame, 512, 512, { left: 0, top: 0, right: 1023, bottom: 1023 }); s.additional(sid, image, 512, 512); assert.deepEqual(image.pixels, expected); assert.ok(expected.some((value) => value !== 255));
        put(raw, 36, -1); assert.throws(() => s.additional(sid, image, 0, 0), /TYPE\/프레임 범위/); raw[10] = 69; put(raw, 36, 0); assert.throws(() => s.additional(sid, image, 0, 0), /TYPE\/프레임 범위/);
        const type = o.core.types.findIndex((record, number) => number >= 70 && (record.flags2 & TypeFlag2.bomb) && game.types()[number - 70].definition.helpFrame >= 0); assert.ok(type >= 70); raw[10] = type; put(raw, 36, game.types()[type - 70].definition.helpFrame); image.pixels.fill(255); s.additional(sid, image, 100, 100); assert.ok(image.pixels.every((value) => value === 255));
    } finally { o.dispose(); }
});

test('Session 캡처 분배: 정확한 건물 자격의 설치 전 오류와 같은 실제 전용 ShapeToBuffer를 선택한다', () => {
    const o = scene();
    try { before(o); o.connectParticles(); const sid = actor(o, 75, 0); const raw = o.core.pool.allocated(sid); assert.throws(() => o.particleModules.capture(sid), /건물\/유닛 캡처 설치 전/); raw[40] &= ~2; const actual = new RawShapeToBuffer(o.core.pool, o.core.types, o.core.shapes, game.shapes, createShapeToBufferState()); o.buildingCapture = actual; assert.deepEqual(o.particleModules.capture(sid), actual.capture(sid)); put(raw, 0, 0x12345678); assert.throws(() => o.particleModules.capture(sid), /미복원 TYPE75/); } finally { o.dispose(); }
});

test('Session 파편 장치: 같은 실제 World 생성/Pop·의존 목록과 미짝 삭제를 사용한다', () => {
    const o = scene();
    try {
        before(o); o.connectParticles(); const hooks = o.particleModules.shrapnel; const source = actor(o, 75); const dummy = hooks.lifetime.create(80, 2); assert.equal(o.core.pool.slot(dummy)[10], 80); hooks.lifetime.pop(dummy, 20, 20, 0); assert.equal(get(o.core.pool.slot(dummy), 14), floatToBits(20)); assert.equal(o.core.dependencies.has(source, dummy), false); const free = o.core.pool.freeCount;
        hooks.lifetime.depend(source, dummy); assert.equal(o.core.pool.freeCount, free - 2); assert.equal(o.core.dependencies.has(source, dummy), true); o.core.dependencies.remove(dummy, source); assert.equal(o.core.dependencies.has(source, dummy), false); hooks.destroyUnpaired(dummy); assert.ok(o.core.pool.slot(dummy)[11] & 1);
    } finally { o.dispose(); }
});

test('Session 빔/EvilRay 음향: 같은 SoundList와 현재 TYPE 발사/충돌·선택 재생 장치를 사용한다', () => {
    const calls = []; const o = scene();
    try {
        before(o); o.connectParticles(); o.services.sound.playOnceAt = (...args) => calls.push(args); o.particleModules.beamSound(2, 3, 'shortZap.wav'); assert.deepEqual(calls.shift(), [2, 3, o.soundNames.lookup('shortZap.wav'), 0, 0]);
        const sid = actor(o, 75); const devices = o.particleModules.evilDevices; o.missileTypes[75].firingSound = 123; o.missileTypes[75].impactSound = 456; assert.equal(devices.soundId(sid, false), 123); assert.equal(devices.soundId(sid, true), 456); o.missileTypes[75].firingSound = 789; assert.equal(devices.soundId(sid, false), 789);
        o.services.sound.playOnceAt = (...args) => { calls.push(['A', ...args]); o.services.sound.playOnceAt = (...next) => calls.push(['B', ...next]); }; devices.sound(4, 5, 789); devices.sound(6, 7, 456); assert.deepEqual(calls, [['A', 4, 5, 789, 0, 0], ['B', 6, 7, 456, 0, 0]]);
        o.core.pool.allocated(sid)[10] = 250; assert.throws(() => devices.soundId(sid, false), /TYPE 물리 범위/); o.services.sound.playOnceAt = null; assert.throws(() => devices.sound(0, 0, 0), /위치 재생 장치 미연결/); assert.throws(() => o.particleModules.beamSound(0, 0, 'x.wav'), /파일명 음향 미연결/);
    } finally { o.dispose(); }
});

test('Session EvilRay 진단: 같은 실제 Client 상세 owner와 현재 선택 진단 장치로 전달한다', () => {
    const o = scene(); const detail = { value: '' }; const calls = [];
    try {
        before(o); o.connectParticles(); o.world.storage.bindDisplayDiagnostics(detail, forbidden); const devices = o.particleModules.evilDevices; devices.frameError(75, -1, 2); assert.equal(detail.value, `Frame assertion failed. Asking for type=75 (${o.core.types[75].name}) frame=-1 abs=2\n\nType has ${o.core.shapes[75].frameCount} frames\n`);
        o.services.ai.assertion = (text) => { calls.push(['A', text]); o.services.ai.assertion = (next) => calls.push(['B', next]); }; devices.assertion('first', 'ignored', 99); devices.assertion('second', 'ignored', 99); assert.deepEqual(calls, [['A', 'first'], ['B', 'second']]); o.services.ai.assertion = null; assert.throws(() => devices.assertion('missing'), /Client 진단 장치 미연결/);
    } finally { o.dispose(); }
});

test('Session 입자 현재 전역: 같은 카메라 snapshot과 Streak Process TYPE을 복사하지 않는다', () => {
    const o = scene();
    try {
        before(o); o.connectParticles(); const camera = o.gameplayGlobals.camera().snapshot(); const reads = []; const original = camera.data.getFloat32.bind(camera.data); camera.data.getFloat32 = (offset, little) => { const value = original(offset, little); reads.push([offset, value]); return value; };
        camera.data.setFloat32(56, 48.5, true); camera.data.setFloat32(60, 22.25, true); assert.deepEqual(o.particles.sprites({ left: 0, top: 0, right: 640, bottom: 480 }), []); assert.ok(reads.some(([offset, value]) => offset === 56 && value === 48.5)); assert.ok(reads.some(([offset, value]) => offset === 60 && value === 22.25));
        const aux = o.gameplayGlobals.gameAux(); aux.streakProcessType = 111; assert.equal(o.state.buildArrivalState.currentStreakProcessType(), 111); aux.streakProcessType = 222; assert.equal(o.state.buildArrivalState.currentStreakProcessType(), 222);
    } finally { o.dispose(); }
});

test('Session 실제 입자 수명: 같은 팔레트/World Host에 불꽃·빔을 부착하고 부모 종료 때 정리한다', () => {
    const o = scene(); const calls = [];
    try {
        before(o); o.connectParticles(); o.services.sound.playOnceAt = (...args) => calls.push(args); const parent = actor(o, 75); const target = actor(o, 128); put(o.core.pool.allocated(target), 14, floatToBits(30));
        const initial = o.core.kernel.size; o.particles.add(11, parent); assert.ok(o.core.kernel.size > initial); const beam = o.particles.addBeam(parent, target, 'shortZap.wav'); assert.ok(beam.process); assert.equal(calls.length, 0);
        // 빔 소리는 생성자가 아니라 실제 첫 프레임에서 한 번 재생된다. 표시 장치의 dirty만 기록한다.
        const dirty = []; o.particles.setView({ left: 0, top: 0, right: 640, bottom: 480, cameraX: 0, cameraY: 0 }, { suppressed: () => false, invalidate: (...args) => dirty.push(args) }); o.particles.prepareFrame(1, 1); o.core.kernel.runFrame(); assert.ok(calls.some((args) => args[2] === o.soundNames.lookup('shortZap.wav'))); assert.ok(dirty.length > 0);
    } finally { o.dispose(); }
    assert.equal(o.core.kernel.size, 0);
});
