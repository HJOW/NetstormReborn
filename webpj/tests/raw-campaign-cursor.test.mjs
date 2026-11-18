// RawCampaignCursor의 실제 하위 조립과 현재 상태/장치/송신을 검사한다. CampaignSessionTests의 저장 미션·Client/UserInput 조립은 D/E 통합에서 재대조한다. (WB04-E569)
import test from 'node:test';
import assert from 'node:assert/strict';
import { CampaignCursorScene } from './campaign-cursor-support.mjs';
import { get, floatToBits } from './raw-support.mjs';
import { InputCode } from '../www/core/input-queue.js';
import { ClientCursorGlobals } from '../www/core/client-cursor-globals.js';
import { SquidForms } from '../www/core/squid-forms.js';
import { RawCursorPosition } from '../www/core/raw-cursor-position.js';
import { RegularProcess } from '../www/core/squid-process.js';
import { readData } from './helpers.mjs';
import { GamePalette, buildScreenPalette } from '../www/assets/palette.js';
import { bindCampaignCursorGumps, makeCampaignCursorClientEffects } from '../www/core/campaign-cursor-client-effects.js';
import { FontSet, ALL_FONT_PATHS } from '../www/assets/font-set.js';

// 같은 실제 풀에서 현재 살아 있는 TYPE만 번호 순서대로 관찰한다.
function live(s, type) { const found = []; for (let sid = 5; sid < s.core.pool.capacity; sid += 1) { const raw = s.core.pool.slot(sid); if (!(raw[11] & 3) && raw[10] === type) found.push(sid); } return found; }

test('CampaignCursor 구성: 같은 풀과 필수 game만 요구하고 실제 장기 상태·참조를 빌린다', () => {
    const s = new CampaignCursorScene();
    assert.equal(s.cursor.lifecycle, s.owner.lifecycle()); assert.equal(s.cursor.placement, s.owner.placement()); assert.equal(s.cursor.snapToGrid, s.owner.snapToGrid()); assert.equal(s.cursor.contents, s.contained);
    // 다른 풀의 각 소비자와 필수 게임 누락은 실제 구성에서 거부한다.
    for (const name of ['forms', 'contained', 'reward', 'construction']) assert.throws(() => s.make({ [name]: { pool: {} } }), /단일 월드/);
    // 필수 공개 명령 준비/Builder/소리는 빈 함수로 보충하지 않는다.
    for (const name of ['prepare', 'moveBuilder', 'playDrop']) assert.throws(() => s.make({}, { ...s.game, [name]: null }), /단일 월드/);
    const fallback = s.make({ currentState: null }); assert.notEqual(fallback.lifecycle, s.cursor.lifecycle); assert.notEqual(fallback.snapToGrid, s.cursor.snapToGrid);
    const state = new ClientCursorGlobals(); state.lifecycle().previews[29] = 0xfedcba98; const other = s.make({ currentState: state }); assert.equal(other.lifecycle.previews[29], 0xfedcba98);
});

test('CampaignCursor 실제 취득·마우스·취소: Form·Canon·미리보기·SHP와 같은 목록이 반납된다', () => {
    const s = new CampaignCursorScene(); const form = s.form(); s.cursor.take(form, 0x100); s.point();
    assert.equal(s.drop.preview, form); assert.equal(s.drop.type, 157); assert.equal(s.drop.lockDepth, 0); assert.equal(s.drop.legal, 1); assert.equal(s.drop.x, 20); assert.equal(s.drop.y, 21);
    assert.ok(s.cursor.lifecycle.previewCount > 0); assert.equal(s.cursor.lifecycle.costText, String(Math.trunc(s.core.types[157].cost))); assert.ok(s.cursor.lifecycle.costImage > 0);
    const previews = s.cursor.lifecycle.previews.slice(); s.cursor.cancel(); assert.ok(s.core.pool.slot(form)[11] & 1); assert.equal(s.drop.preview, 0); assert.equal(s.drop.lockDepth, 1); assert.equal(s.cursor.lifecycle.previewCount, 0);
    // 생성된 임시 자산 모두 실제 가상 삭제가 같은 SID 풀에 반납한다.
    for (const sid of previews) if (sid) assert.ok(s.core.pool.slot(sid)[11] & 1);
    assert.equal(s.sp.get(1), floatToBits(5000)); assert.equal(s.sent.length, 0);
});

// 실제 커서 수명이 Client 글꼴·마우스·화면 장치를 사용해 취득과 취소를 수행한다.
test('CampaignCursor 실제 Client 글꼴과 화면 장치: 취득과 취소', () => {
    const s = new CampaignCursorScene(), fonts = new FontSet(new Map(ALL_FONT_PATHS.map((name) => [name, readData(name)]))), events = [];
    const client = { fonts: () => fonts, translate: (text) => text, getRenderer: () => ({ invalidate: (...args) => events.push(['dirty', ...args]) }),
        getCursor: () => ({ set: (value) => events.push(['cursor', value]) }), poll: () => ({ x: 320, y: 240 }) };
    const controls = { pickGump: () => 0, dispatchGump: () => 0, gumpParentSid: () => 0, handleGump: () => 0,
        requestClosePopups: () => events.push(['close']), menus: () => ({ showSp: (...args) => events.push(['sp', ...args]) }) };
    s.cursor.setEffects(makeCampaignCursorClientEffects(client, controls, s.ui())); const form = s.form(); s.cursor.take(form, 0x100); s.point();
    assert.equal(s.drop.preview, form); assert.ok(s.cursor.lifecycle.costTextWidth > 0); s.cursor.cancel();
    assert.equal(s.drop.preview, 0); assert.ok(events.some(([kind]) => kind === 'dirty')); assert.ok(events.some(([kind]) => kind === 'cursor'));
});

test('CampaignCursor 실제 장치: 도달한 dirty 누락 거부·교체 즉시 적용과 값형 함수 사본', () => {
    const s = new CampaignCursorScene(); s.cursor.setEffects({}); assert.throws(() => s.cursor.cancel(), /화면 dirty/);
    const first = s.ui('first'); s.cursor.setEffects(first); first.placement.invalidate = () => { throw new Error('호출자 변형'); }; s.cursor.cancel();
    assert.ok(s.events.some((row) => row[0] === 'first' && row[1] === 'dirty'));
    const before = s.events.length; s.cursor.setEffects(s.ui('second')); s.cursor.cancel(); assert.ok(s.events.slice(before).every((row) => row[0] !== 'first'));
    const prepared = s.prepared; s.cursor.cancelForState(); assert.equal(s.prepared, prepared); s.cursor.cancel(); assert.equal(s.prepared, prepared + 1);
});

test('CampaignCursor 메뉴·입력: 같은 Take 바인딩과 현재 중첩 잠금·회전·release/outside·ESC 분배', () => {
    const s = new CampaignCursorScene(); const hooks = {}; s.cursor.bind(hooks); hooks.takePreview(s.form(), 0x100); s.point();
    assert.equal(s.cursor.handleInput({ code: InputCode.rightButton | InputCode.release }, 0), false); assert.equal(s.drop.direction, 0);
    assert.equal(s.cursor.handleInput({ code: InputCode.rightButton | InputCode.outside }, 0), true); assert.equal(s.drop.direction, 2);
    assert.equal(s.cursor.handleInput({ code: InputCode.rightButton }, 1), true); assert.equal(s.drop.direction, 0);
    assert.equal(s.cursor.handleInput({ code: 0x250000 | InputCode.shift }, 0), true); assert.equal(s.drop.direction, 6);
    assert.equal(s.cursor.handleInput({ code: 0x270000 }, 0), true); assert.equal(s.drop.direction, 0); assert.equal(s.cursor.handleInput({ code: 0x41 }, 0), false);
    s.cursor.lock(1); assert.equal(s.drop.lockDepth, 1); assert.equal(s.cursor.handleInput({ code: InputCode.leftButton }, 0), false); s.cursor.unlock(); assert.equal(s.drop.lockDepth, 0);
    assert.equal(s.cursor.handleInput({ code: 0x1b }, 0), true); assert.equal(s.drop.preview, 0); assert.equal(s.drop.lockDepth, 1);
});

test('CampaignCursor 정지 마우스 Gump: 현재 UI 토큰 재조회·준비 순서·잠금/빈 커서는 조기 반환한다', () => {
    const s = new CampaignCursorScene(); s.cursor.gumpsChanged(); assert.equal(s.prepared, 0); s.cursor.take(s.form(), 0x100); s.point(); const x = s.drop.x; const y = s.drop.y;
    let token = 10; const effects = s.ui(); effects.placement.gumpAt = () => token; effects.placement.accept = () => token; effects.placement.parent = () => s.parent; s.cursor.setEffects(effects);
    s.cursor.gumpsChanged(); assert.equal(s.cursor.lifecycle.currentGump, 10); token = 20; s.cursor.gumpsChanged(); assert.equal(s.cursor.lifecycle.currentGump, 20); assert.deepEqual([s.drop.x, s.drop.y], [x, y]);
    const before = s.prepared; s.cursor.lock(0); const locked = s.prepared; s.cursor.gumpsChanged(); assert.equal(s.prepared, locked); assert.equal(locked, before + 1);
});

test('CampaignCursor 실제 GUI Drop: 같은 Form 재부착·수신과 game 프로세스 통지를 사용한다', () => {
    const s = new CampaignCursorScene(); const target = s.world.create(97, 2); const effects = s.ui();
    bindCampaignCursorGumps(effects, { pickGump: () => 77, dispatchGump: () => 77, gumpParentSid: () => target,
        handleGump: (gump, command, form) => { if (command === 0x0200005e) s.events.push(['receive', gump, form]); return 0; }, requestClosePopups: () => {} });
    effects.drop.closeGump = () => { throw new Error('UI close는 사용하면 안 됨'); };
    s.cursor.setEffects(effects); const form = s.form(); s.cursor.take(form, 0x100); s.point(); s.drop.cursorGump = 123; s.cursor.drop();
    assert.equal(new SquidForms(s.core.pool).parent(form), target); assert.ok(s.events.some((row) => row[0] === 'receive' && row[1] === 77 && row[2] === form)); assert.ok(s.events.some((row) => row[0] === 'process-event' && row[1] === 123 && row[2] === 3)); assert.equal(s.drop.preview, 0); assert.equal(s.drop.cursorGump, 0);
});

test('CampaignCursor 실제 월드 Drop: 예측 Pop·반환 Form/Regular·25BYTE 송신이 상위 비용 차감보다 먼저다', () => {
    const s = new CampaignCursorScene(); const form = s.form(84, 8, 3); const effects = s.ui(); let predicted = 0;
    effects.send = (destination, packet) => { assert.equal(destination, 9); assert.equal(s.sp.get(1), floatToBits(5000)); assert.ok(s.core.pool.slot(form)[11] & 1); const current = live(s, 84); assert.equal(current.length, 1); predicted = current[0]; assert.equal(s.core.pool.slot(predicted)[40] & 1, 1); s.sent.push(Uint8Array.from(packet)); };
    s.cursor.setEffects(effects); s.cursor.returnSettings.lifetimeIndex = 1; s.cursor.take(form, 0x140); s.point(); s.cursor.drop();
    assert.equal(s.sent.length, 1); assert.equal(s.sp.get(1), floatToBits(3000)); assert.equal(s.drop.preview, 0); assert.equal(s.drop.lockDepth, 2); assert.equal(s.cursor.lifecycle.previewCount, 0);
    const packet = s.sent[0]; assert.deepEqual([...packet.subarray(0, 5)], [1, 0x3e, 25, 0, 84]); assert.deepEqual([get(packet, 5), get(packet, 9), get(packet, 17), get(packet, 21)], [floatToBits(20), floatToBits(21), 0, 0]);
    const returned = live(s, 6); assert.equal(returned.length, 1); const raw = s.core.pool.slot(returned[0]); assert.equal(get(raw, 18, 2), 84); assert.equal(get(raw, 20, 2) & 0x108, 0x108); assert.equal(raw[30], 2); assert.equal(new SquidForms(s.core.pool).parent(returned[0]), s.parent);
    const timer = s.core.processes.fromForm(live(s, 46)[0]); assert.ok(timer instanceof RegularProcess); assert.equal(timer.parent, returned[0]); assert.equal(timer.event, 0); assert.equal(timer.payload, 10); assert.equal(timer.time, s.core.processState.now);
    assert.equal(s.core.pool.slot(predicted)[11] & 4, 0);
});

test('CampaignCursor 실제 직접 Local: 현재 요청 헤더·현재 설정을 읽고 취득·상위 비용·소리를 추가하지 않는다', () => {
    const s = new CampaignCursorScene(); const aux = s.world.storage.gameAux; aux.constructionRequestHeader.set([0x5d, 0x83]); aux.constructionRequestLength = 0xabcd;
    const form = s.form(84, 8); s.drop.type = 84; s.rate = 2; s.cursor.placeLocal({ preview: form, x: 31, y: 32, argument: 0, direction: 0 });
    assert.equal(s.sent.length, 1); assert.deepEqual([...s.sent[0].subarray(0, 5)], [3, 0x5d, 0xcd, 0xab, 84]); assert.equal(s.sp.get(1), floatToBits(5000)); assert.equal(s.prepared, 1); assert.equal(s.drop.preview, 0);
    assert.ok(!s.events.some((row) => ['charge', 'drop-sound'].includes(row[0]))); const timer = s.core.processes.fromForm(live(s, 46)[0]); assert.equal(timer.payload, 1);
    // 같은 실제 getter가 켜진 다음 반환에는 타이머를 더 만들지 않는다.
    s.timers = 1; s.cursor.placeLocal({ preview: s.form(84, 8), x: 35, y: 36, argument: 0, direction: 0 }); assert.equal(live(s, 46).length, 1); assert.equal(live(s, 6).length, 2);
});

test('CampaignCursor 현재 카메라·팔레트·표시: 공유 원점/스냅과 같은 수집기의 실제 kind4를 사용한다', () => {
    const s = new CampaignCursorScene(); const p = buildScreenPalette(GamePalette.parse(readData('d/GIFCLOUD.COL'))); const palette = new Uint8Array(1024);
    // 보호 COL을 실제 Screen BGRX 입력으로 변환한다.
    for (let i = 0; i < 256; i += 1) palette.set([p.rgb[i * 3 + 2], p.rgb[i * 3 + 1], p.rgb[i * 3], 0], i * 4);
    s.cursor.setPalette(palette); s.cursor.setView({ cameraX: 137, cameraY: 91, left: 0, top: 0, right: 1024, bottom: 768 }); assert.equal(s.cursor.placement.screenOriginX(), 0);
    s.cameraView.pixels.set([137, 91]); s.camera.set(RawCursorPosition.cameraFromPixels(137, 91)); s.cursor.take(s.form(), 0x100); s.point();
    assert.deepEqual([s.drop.x, s.drop.y], [20, 21]); assert.equal(s.cursor.placement.screenOriginX(), 137); assert.equal(s.cursor.placement.screenOriginY(), 91); assert.equal(s.cursor.lifecycle.warningContext[2], s.cursor.display.textColors()[0]); assert.equal(s.cursor.lifecycle.costContext[2], s.cursor.display.textColors()[1]);
    const entries = s.cursor.display.sprites({ left: 0, top: 0, right: 1024, bottom: 768 }, 0); assert.ok(entries.length >= 2); assert.equal(entries[0].order.depth, s.core.types[157].zOrder); const pixels = new Uint8Array(1024 * 768); entries[0].procedure.draw(pixels, 1024, 768, { left: 0, top: 0, right: 1024, bottom: 768 }); assert.ok(pixels.some((value) => value));
    // 독립 호출의 명시 fallback만 SetView의 원점을 직접 저장한다.
    const independent = s.make({ currentState: null, cameraView: null }); independent.setView({ cameraX: 4, cameraY: 5, left: 0, top: 0, right: 100, bottom: 100 }); assert.deepEqual([independent.placement.screenOriginX(), independent.placement.screenOriginY()], [4, 5]);
});

test('CampaignCursor 값형 data/game: 호출자 속성 교체는 분리하고 원래 참조의 현재 값은 보존한다', () => {
    const s = new CampaignCursorScene(); const game = { ...s.game, lifecycle: { ...s.game.lifecycle }, placement: { ...s.game.placement } }; const owner = new ClientCursorGlobals(); const data = { ...s.data, currentState: owner }; const cursor = new (s.cursor.constructor)(data, game); cursor.setEffects(s.ui());
    data.drop = {}; data.world = {}; data.geometry = {}; game.prepare = () => { throw new Error('외부 준비 변형'); }; game.lifecycle.sound = () => { throw new Error('외부 소리 변형'); };
    cursor.cancel(); assert.equal(s.prepared, 1); assert.equal(s.drop.lockDepth, 1); assert.equal(cursor.lifecycle, owner.lifecycle());
});

test('CampaignCursor 명시 실패: 실제 가상 삭제 거부와 누락된 게임 소비자를 성공으로 바꾸지 않는다', () => {
    const s = new CampaignCursorScene(); const form = s.form(84); s.world.destroy = () => false;
    assert.throws(() => s.cursor.placeLocal({ preview: form, x: 20, y: 21, argument: 0, direction: 0 }), /실제 커서 가상 삭제 거부/); assert.equal(s.sent.length, 0);
    const other = new CampaignCursorScene(); assert.throws(() => other.cursor.take(other.form(75), 0x100), /실제 Session 마나 소비자/); assert.equal(other.drop.type, 75); assert.ok(other.cursor.lifecycle.previewCount > 0);
});
