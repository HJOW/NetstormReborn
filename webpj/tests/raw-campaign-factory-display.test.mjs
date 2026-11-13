// 실제 자산/Factory/공간 해시/프로세스 호스트를 공유한 표시·선택 조립 검사다. GameWorld/Session 부모 시험은 아니다. (2026-11-14 WB04-E409)
import test from 'node:test';
import assert from 'node:assert/strict';
import { realCaptureAssets } from './capture-assets-support.mjs';
import { realPlayerAssets } from './player-scene-support.mjs';
import { readData } from './helpers.mjs';
import { get, put, floatToBits } from './raw-support.mjs';
import { GameAssets } from '../www/assets/game-assets.js';
import { GamePalette, buildScreenPalette } from '../www/assets/palette.js';
import { FrameCodeTable } from '../www/assets/terrain-tiles.js';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidFactory } from '../www/core/squid-factory.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { SquidUnpop } from '../www/core/squid-unpop.js';
import { RawSquidDestroy } from '../www/core/squid-destroy.js';
import { Kernel } from '../www/core/kernel.js';
import { SquidProcessHost, SquidProcessState } from '../www/core/squid-process.js';
import { BridgeConnectState } from '../www/core/raw-bridge-connect.js';
import { ClientColorSortInput } from '../www/core/client-color-sort-input.js';
import { OwnerColorTableState } from '../www/core/owner-color-table-state.js';
import { buildPaletteColorTable } from '../www/core/palette-colors.js';
import { drawWindThunderTemple } from '../www/core/raw-temple-draw.js';
import { BombEffectTimerProcess } from '../www/core/raw-bomb-effect-timer.js';
import { OBJECT_HIT_VTABLES } from '../www/render/object-hit-vtables.js';
import { RawCampaignFactoryDisplay } from '../www/render/raw-campaign-factory-display.js';
import { cursorDisplayLogicalPalette } from '../www/render/raw-campaign-cursor-display.js';
import { drawIndexedImage } from '../www/render/indexed-surface.js';

// 명시 표시 표면과 카메라다. 모든 기대 그림은 Decode 원색으로 따로 합성한다.
const W = 640; const H = 480; const CLIP = Object.freeze({ left: 0, top: 0, right: W, bottom: H }); const CAMERA = [32, 11];
// 같은 파일 팔레트와 실제 고정9표를 한 번 만들고 표시기에는 복사하여 준다.
let cachedPalette;
function paletteData() {
    if (cachedPalette) return cachedPalette;
    const p = buildScreenPalette(GamePalette.parse(readData('d/GIFCLOUD.COL'))); const bytes = new Uint8Array(1024);
    // 파일 RGB를 실제 화면 BGRX로 변환한다.
    for (let i = 0; i < 256; i += 1) { bytes[i * 4] = p.rgb[i * 3 + 2]; bytes[i * 4 + 1] = p.rgb[i * 3 + 1]; bytes[i * 4 + 2] = p.rgb[i * 3]; }
    const logical = cursorDisplayLogicalPalette(bytes); const sort = new ClientColorSortInput(); sort.captureWithPrimary(logical); const maps = sort.rebuild(buildPaletteColorTable(logical).basic[1]); return cachedPalette = { bytes, logical, maps, sort };
}

// 표시기가 빌리는 실제 장치만 조립하며 미션/AI/진행을 흉내 내는 월드는 만들지 않는다.
function scene(type = 75, ready = true) {
    const a = realCaptureAssets(); const assets = new GameAssets(a.table, a.database); const types = realPlayerAssets().types.map((r) => ({ ...r, hotspotX: 0, hotspotY: 0 }));
    const frames = types.map((_, i) => ({ frames: i >= 70 && i < 186 ? FrameCodeTable.fromDefinition(a.table.definitions[i - 70]) : new FrameCodeTable([]) }));
    const pool = new SidPool(24000, { server: true }); const hash = new SquidHash(); const spots = new Uint8Array(65536); const unpop = new SquidUnpop(pool, hash, spots); const factory = new SquidFactory(pool, types, { unpop }); const destroy = new RawSquidDestroy(pool, unpop, types); const kernel = new Kernel(); const processState = new SquidProcessState(); const processes = new SquidProcessHost(pool, types, kernel, destroy, processState);
    const bridgeState = new BridgeConnectState(); bridgeState.ownerColors[1] = 1; bridgeState.ownerColors[2] = 2;
    const core = { pool, hash, spots, types, frames, shapes: a.shapes, factory, kernel, processState, processes, bridgeState, ownerMode: { battle: false } };
    const sid = factory.create(type, 2); const raw = pool.physical(sid); raw[34] = 1; put(raw, 14, floatToBits(20)); put(raw, 18, floatToBits(21)); put(raw, 22, 320, 2); put(raw, 24, 231, 2); put(raw, 36, a.table.definitions[type - 70].defaultFrame); hash.setBucket(1, 20, 21, sid);
    // TYPE123의 실제 윈드 신전 몸체를 연결한다. 레인 전용 장치는 이 검사 대상이 아니다.
    const temple = (object, emit) => { const current = pool.physical(object); drawWindThunderTemple(current, frames[current[10]].frames, a.table.definitions[current[10] - 70].baseFrame, emit); };
    const display = new RawCampaignFactoryDisplay(core, assets, temple); const palette = paletteData(); if (ready) display.setPalette(palette.bytes, palette.logical, palette.maps);
    const entry = new Uint8Array(16); const data = new DataView(entry.buffer); data.setFloat32(0, 20, true); data.setFloat32(4, 21, true); data.setUint32(8, sid * 50, true); data.setInt16(12, (raw[35] << 24) >> 24, true);
    return { assets, types, frames, pool, hash, factory, kernel, processState, processes, bridgeState, core, sid, raw, display, entry, palette };
}

// 실제 SHP Decode와 clip을 사용하는 독립 단일 레이어 기대값이다.
function layer(s, target, type, frame, dx = 0, dy = 0, colors = null, clip = CLIP, translated = false) {
    const block = type - 70; const picture = s.assets.shapes.blocks[block].frames[frame]; if (picture.special) return;
    const x = 320 - CAMERA[0] - s.types[s.raw[10]].hotspotX + dx; const y = 231 - CAMERA[1] - s.types[s.raw[10]].hotspotY + dy;
    drawIndexedImage(target, W, H, s.assets.shapes.decode(block, frame), x + picture.rect.left, y + picture.rect.top, clip, colors, translated);
}

// 실제 kind0 한 항목을 통합 원문 Draw로 출력한다.
function render(s, clip = CLIP, local = 1, meta = null) { const pixels = new Uint8Array(W * H).fill(199); s.display.drawEntry(s.entry, pixels, W, H, clip, CAMERA, local, 0, null, meta); return pixels; }

test('FactoryDisplay 실제83VT·기본/추가 프레임: Factory·마나·볼트·간헐천·신전·캐논의 원색/그림자 픽셀이 일치한다', () => {
    assert.equal(OBJECT_HIT_VTABLES.length, 83); assert.equal(new Set(OBJECT_HIT_VTABLES.map((v) => v.table)).size, 83); assert.ok(Object.isFrozen(OBJECT_HIT_VTABLES));
    // 명시 비공중 일곱 종류만 만든다. 타입 전수 생성/공중 전용 검사는 하지 않는다.
    for (const type of [75, 86, 103, 105, 122, 123, 131]) {
        const s = scene(type); assert.equal(s.display.handles(s.sid), type !== 103, `TYPE${type} 원문 Handles`); const expected = new Uint8Array(W * H).fill(199); const frame = get(s.raw, 36); const base = s.assets.types()[type - 70].definition.baseFrame;
        if (type === 103 || type === 105) { if (type === 103) put(s.raw, 12, 1, 2); layer(s, expected, type, frame, 10, 10, s.palette.maps[7], CLIP, true); layer(s, expected, type, frame); }
        else if (type === 122) { layer(s, expected, type, base); layer(s, expected, type, frame); }
        else if (type === 131) { layer(s, expected, type, s.frames[type].frames.run(s.frames[type].frames.codes[frame].side).first); layer(s, expected, type, frame); }
        else layer(s, expected, type, base === -1 ? frame : base);
        assert.deepEqual(render(s), expected, `TYPE ${type}`); assert.ok(expected.some((p) => p !== 199), `TYPE ${type} 실제 그림`);
    }
});

test('FactoryDisplay 공간 수집: 본체/그림자·현재 카메라·dirty clip과 legacy/통합 Draw가 같은 풀을 읽는다', () => {
    const s = scene(86); const clip = { left: 260, top: 170, right: 330, bottom: 250 }; const sprites = s.display.sprites(CLIP, ...CAMERA, 1); assert.equal(sprites.length, 2); assert.equal(sprites[0].sourceSid, s.sid); assert.equal(sprites[0].database, null);
    const main = sprites.find((v) => !v.shadow); assert.deepEqual(main.order, { x: 20, y: 21, depth: (s.raw[35] << 24) >> 24 }); const pixels = new Uint8Array(W * H).fill(199); main.procedure.draw(pixels, W, H, clip); assert.deepEqual(pixels, render(s, clip));
    // 현재 변경 영역 밖은 같은 표면을 보존한다.
    for (let y = 0; y < H; y += 1) for (let x = 0; x < W; x += 1) if (x < clip.left || x >= clip.right || y < clip.top || y >= clip.bottom) assert.equal(pixels[y * W + x], 199);
    put(s.raw, 36, 1); const changed = new Uint8Array(W * H).fill(199); main.procedure.draw(changed, W, H, CLIP); const expected = new Uint8Array(W * H).fill(199); layer(s, expected, 86, 0); layer(s, expected, 86, 1); assert.deepEqual(changed, expected);
    const shadow = sprites.find((v) => v.shadow); assert.equal(shadow.order.y, Math.fround(21 - 0.1)); expected.fill(199); layer(s, expected, 86, s.core.shapes[86].frameCount + 1, 0, 0, s.palette.maps[7], CLIP, true); changed.fill(199); shadow.procedure.draw(changed, W, H, CLIP); assert.deepEqual(changed, expected); s.raw[40] |= 1; changed.fill(199); shadow.procedure.draw(changed, W, H, CLIP); assert.ok(changed.every((v) => v === 199));
});

test('FactoryDisplay 실제 SHP 선택: 사제·윈드 타워·크리스탈·워크샾·신전의 투명구멍과 현재 카메라/raw를 대조한다', () => {
    let accepted = 0; let rejected = 0;
    // CPP 실제 Session 시험과 같은 비공중 다섯 타입을 부품 장치에 공급한다.
    for (const type of [158, 142, 154, 86, 123]) {
        const s = scene(type); s.raw[40] = 0; put(s.raw, 14, floatToBits(50.25)); put(s.raw, 18, floatToBits(51.5)); const frame = get(s.raw, 36); const def = s.assets.types()[type - 70].definition; const hf = type === 86 ? s.frames[type].frames.run(0x41 + (s.raw[11] >> 5)).first : (def.baseFrame === -1 ? frame : def.baseFrame); const header = s.core.shapes[type].frames[hf]; const picture = s.assets.shapes.blocks[type - 70].frames[frame]; const image = s.assets.shapes.decode(type - 70, frame); const before = s.raw.slice();
        // 같은 실제 슬롯을 두 카메라에서 관측하며 현재 raw에 게임 진행을 더하지 않는다.
        for (const camera of [[700, 500], [-16, 33]]) {
            // 헤더보다 큰 표본 영역에서 투명·불투명·200밖을 모두 포함한다.
            for (let ry = 0; ry < 202; ry += 17) for (let rx = 0; rx < 202; rx += 17) {
                const sx = rx - header.hotspotX - picture.rect.left; const sy = ry - header.hotspotY - picture.rect.top; let expected = false;
                if (s.types[type].flags1 & 0x800000) expected = rx < header.width && ry < header.height;
                else if (rx < 200 && ry < 200 && sx >= 0 && sy >= 0 && sx < image.width && sy < image.height) expected = !!image.opacity[sy * image.width + sx] && image.indices[sy * image.width + sx] !== 255;
                const actual = s.display.hit(s.sid, 804 - camera[0] - header.hotspotX + rx, 567 - camera[1] - header.hotspotY + ry, camera, 1); assert.equal(actual, expected, `TYPE${type} ${camera} ${rx},${ry}`); if (actual) accepted += 1; else rejected += 1;
            }
        }
        assert.deepEqual(s.raw, before);
    }
    assert.ok(accepted > 0 && rejected > 0);
});

test('FactoryDisplay 현재 공통 소유자 표·선택 HP: 같은 토큰과 Player 색·공통 getter를 실제 출력에서 다시 읽는다', () => {
    const s = scene(158); const owner = new OwnerColorTableState(); owner.initialize(s.palette.sort.primary, 188, 82, { value: 0 }); s.display.bindOwnerColors(owner);
    const expected = new Uint8Array(W * H).fill(199); layer(s, expected, 158, get(s.raw, 36), 0, 0, owner.color(158, 1)); assert.deepEqual(render(s), expected); s.bridgeState.ownerColors[1] = 2; expected.fill(199); layer(s, expected, 158, get(s.raw, 36), 0, 0, owner.color(158, 2)); assert.deepEqual(render(s), expected);
    let hp = 0; const readHp = s.factory.readSelectedHitPoints.bind(s.factory); s.factory.readSelectedHitPoints = (...args) => { hp += 1; return readHp(...args); }; s.raw[40] = 4; const selected = render(s); assert.equal(hp, 3); assert.notDeepEqual(selected, expected); assert.equal(s.processes.findEvent(s.sid, 0x493), null);
    const saved = owner.color(158, 2); s.display.setPalette(new Uint8Array(1024), s.palette.logical, s.palette.maps); assert.equal(owner.color(158, 2), saved); assert.throws(() => s.display.bindOwnerColors(new OwnerColorTableState()), /최초 등록/);
});

test('FactoryDisplay 메타 실제 장치 변경: 설치는 무출력이며 일반 Draw/200선택 버퍼에 도달한 때만 호출한다', () => {
    const s = scene(75); const meta = OBJECT_HIT_VTABLES.find((v) => v.additional === 0x478d10); put(s.raw, 0, meta.table); s.raw[40] = 2;
    assert.throws(() => render(s), /메타 섬.*미연결/); const header = s.core.shapes[75].frames[0]; const x = 320 - CAMERA[0] - header.hotspotX; const y = 231 - CAMERA[1] - header.hotspotY;
    assert.throws(() => s.display.hit(s.sid, x, y, CAMERA, 1), /메타 선택.*미연결/); assert.throws(() => s.display.bindMetaPixels(null), /누락/); let calls = 0;
    s.display.bindMetaPixels((sid, point, pixels, width, height, clip) => { calls += 1; assert.equal(sid, s.sid); assert.deepEqual([width, height, clip], [200, 200, { left: 0, top: 0, right: 200, bottom: 200 }]); assert.deepEqual(point, { x: header.hotspotX, y: header.hotspotY }); pixels[0] = 7; }); assert.equal(calls, 0); assert.equal(s.display.hit(s.sid, x, y, CAMERA, 1), true); assert.equal(calls, 1);
    const pixels = render(s, CLIP, 1, (sid, px, py) => { assert.deepEqual([sid, px, py], [s.sid, 288, 220]); calls += 1; }); assert.ok(pixels.every((v) => v === 199)); assert.equal(calls, 2);
});

test('FactoryDisplay 상태 막대: 실제 Kernel의 타입57/현재 멤버·기한과 현재 시각을 출력할 때 다시 읽는다', () => {
    const s = scene(142); s.raw[40] = 0x14; const process = new BombEffectTimerProcess(s.pool, s.types, () => s.processState.now, {}); s.processes.attach(process, 57, s.sid, 0x50); Object.assign(process.fields, { flags: 1, members: [s.sid], endTime: 30 }); const initial = render(s);
    s.processState.now = 20; const later = render(s); assert.notDeepEqual(initial, later); process.fields.members = []; const absent = render(s); assert.notDeepEqual(later, absent); process.fields.members = [s.sid]; process.fields.endTime = 30; assert.deepEqual(render(s), later);
});

test('FactoryDisplay 도달 관문: 미설치 팔레트·미확인 VT·선택된 다리/신전 장치 누락은 실패하고 opaque는 장치를 생략한다', () => {
    const s = scene(75, false); assert.throws(() => render(s), /팔레트 미설치/); assert.doesNotThrow(() => s.display.hit(s.sid, -100, -100, CAMERA, 1)); const opaque = s.core.shapes[75].frames[0]; s.types[75].flags1 |= 0x800000; assert.equal(s.display.hit(s.sid, 288 - opaque.hotspotX, 220 - opaque.hotspotY, CAMERA, 1), true); put(s.raw, 0, 0x12345678); assert.equal(s.display.handles(s.sid), false); assert.throws(() => s.display.hit(s.sid, 320, 231, CAMERA, 1), /가상 표 미복원/);
    const bridge = scene(82); bridge.raw[40] = 2; const h = bridge.core.shapes[82].frames[0]; assert.throws(() => bridge.display.hit(bridge.sid, 288 - h.hotspotX, 220 - h.hotspotY, CAMERA, 1), /다리.*미연결/);
    const temple = scene(123); const bare = new RawCampaignFactoryDisplay(temple.core, temple.assets); bare.setPalette(temple.palette.bytes, temple.palette.logical, temple.palette.maps); assert.throws(() => bare.drawEntry(temple.entry, new Uint8Array(W * H), W, H, CLIP, CAMERA, 1, 0, null), /템플.*미연결/);
});
