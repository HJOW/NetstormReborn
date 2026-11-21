// 공통 객체 메뉴 열기가 가상 표(+a8/+ac)에 따라 지원 여부를 판정하고 Damageable·일반 작성과 원본 열기 효과를 호출하는지 대역 Session 으로 확인한다. 게임을 실행하지 않는다. (2026-11-20 WB04-E1086)
import test from 'node:test';
import assert from 'node:assert/strict';
import { SidPool } from '../www/core/sid-pool.js';
import { ClientObjectMenuOpen } from '../www/ui/client-object-menu-open.js';
import { objectMenuSlots } from '../www/render/object-menu-slots.js';

// 발사기(회수 포함 Damageable)·일반 객체·Factory 의 가상 표 주소를 원본 슬롯 표에서 찾는다.
const table = (populate, open) => { for (let address = 0x00500000; address < 0x00520000; address += 4) { try { const slots = objectMenuSlots(address); if (slots.populate === populate && slots.open === open) return address; } catch (error) { if (!(error instanceof RangeError)) throw error; } } throw new Error('가상 표 없음'); };

// 슬롯 하나에 가상 표와 TYPE 을 쓴 대역 World·작성기를 만든다.
function scene(vtable) {
    // 원본 풀의 클라이언트 번호 5 를 실제로 확보한 뒤 슬롯 바이트를 쓴다.
    const pool = new SidPool(24000, { server: true }), sid = pool.allocate(2), raw = pool.allocated(sid); new DataView(raw.buffer, raw.byteOffset, raw.byteLength).setUint32(0, vtable, true); raw[10] = 3; raw[40] = 0;
    const types = Array.from({ length: 188 }, () => ({ flags2: 0, range: 1 })), calls = [];
    const world = { session: () => ({ core: () => ({ pool, types }) }), openSelectionMenu: (object, event, archer, common) => { calls.push(['selection', object, archer]); return common(object, event); } };
    const writer = { beginFancy: () => calls.push('fancy'), currentWidth: () => 100, attachObject: (object, x, y) => calls.push(['attach', object, x, y]) };
    const summary = { summary: (object) => calls.push(['summary', object]) };
    const objects = { open: (object) => calls.push(['open', object]), openDamageable: (object) => calls.push(['damageable', object]), openVariant: (object, populate) => calls.push(['variant', object, populate]), openCarrier: (object) => calls.push(['carrier', object]) };
    const devices = { altar: () => 0, delegate: () => 0, project: () => [10, 10], content: () => 0, range: () => 1, screenWidth: () => 1024, sound: (name) => calls.push(['sound', name]) };
    return { opener: new ClientObjectMenuOpen(world, writer, summary, objects, devices), calls, sid, raw };
}

test('공통 객체 열기: Damageable(발사기 등)은 회수 포함 작성을, 일반 객체는 공통 작성을 쓰고 열릴 때만 효과음을 낸다', () => {
    const damageable = scene(table(0x0044cbd0, 0x004adb30)); assert.equal(damageable.opener.supports(damageable.sid), true);
    assert.equal(damageable.opener.open(damageable.sid, { code: 0x20000, x: 30, y: 40 }), 1);
    assert.deepEqual(damageable.calls.map((call) => Array.isArray(call) ? call[0] : call), ['fancy', 'summary', 'damageable', 'attach', 'sound']); assert.deepEqual(damageable.calls.find((call) => call[0] === 'attach'), ['attach', damageable.sid, 30, 40]);
    const plain = scene(table(0x004b1270, 0x004adb30)); plain.opener.open(plain.sid, { code: 0x20000, x: 1, y: 2 }); assert.ok(plain.calls.some((call) => call[0] === 'open'));
    // 건설 중(+28 bit0)인 객체는 열지 않고 효과음도 없다.
    const building = scene(table(0x0044cbd0, 0x004adb30)); building.raw[40] = 1; assert.equal(building.opener.open(building.sid, { code: 0x20000, x: 1, y: 2 }), 0); assert.equal(building.calls.length, 0);
});

test('공통 객체 열기: Factory·사제 같은 다른 가상 표와 빈 SID 는 지원하지 않고 직접 열면 거부한다', () => {
    const factory = scene(table(0x00454c00, 0x004754b0)); assert.equal(factory.opener.supports(factory.sid), false); assert.throws(() => factory.opener.open(factory.sid, { code: 0, x: 0, y: 0 }), /미연결/u);
    const priest = scene(table(0x00494870, 0x00492080)); assert.equal(priest.opener.supports(priest.sid), false); assert.equal(priest.opener.supports(0), false); assert.equal(priest.opener.supports(30000), false);
    assert.throws(() => new ClientObjectMenuOpen(null, {}, {}, {}, {}), /장치 누락/u);
});

// 신전(4dcca0)·배터리(41c210)·발전기(485600) 같은 파생 본체는 공통 열기(+ac 4adb30)에서 파생 작성(openVariant)을 쓴다. 이전에는 지원하지 않아 우클릭해도 메뉴가 열리지 않았다. (2026-11-20 WB04-E1127)
test('공통 객체 열기: 파생 본체 일곱 주소는 파생 작성을 쓴다', () => {
    for (const populate of [0x0041c210, 0x00422000, 0x00462390, 0x00462550, 0x00485600, 0x0048eca0, 0x004dcca0]) {
        const vtable = table(populate, 0x004adb30); assert.ok(vtable, `슬롯 ${populate.toString(16)}`);
        const variant = scene(vtable); assert.equal(variant.opener.supports(variant.sid), true, populate.toString(16));
        assert.equal(variant.opener.open(variant.sid, { code: 0x20000, x: 5, y: 6 }), 1); assert.deepEqual(variant.calls.find((call) => call[0] === 'variant'), ['variant', variant.sid, populate]);
        assert.deepEqual(variant.calls.map((call) => Array.isArray(call) ? call[0] : call), ['fancy', 'summary', 'variant', 'attach', 'sound']);
    }
});

// Archer(+a8 420b30 Detonate!, +ac 41e660)는 같은 선택 관문(archer=true)을 지난 뒤 공통 열기로 파생 작성을 쓴다. 이전에는 열기가 달라 우클릭해도 메뉴가 열리지 않았다. (2026-11-21 WB04-E1138)
test('공통 객체 열기: Archer(41e660)는 선택 관문(archer)을 지나 파생 작성(420b30)으로 열린다', () => {
    const archer = scene(table(0x00420b30, 0x0041e660)); assert.equal(archer.opener.supports(archer.sid), true);
    assert.equal(archer.opener.open(archer.sid, { code: 0x20000, x: 7, y: 8 }), 1);
    assert.deepEqual(archer.calls.map((call) => Array.isArray(call) ? call[0] : call), ['selection', 'fancy', 'summary', 'variant', 'attach', 'sound']);
    assert.deepEqual(archer.calls[0], ['selection', archer.sid, true]); assert.deepEqual(archer.calls.find((call) => call[0] === 'variant'), ['variant', archer.sid, 0x00420b30]);
});

// 운반체(Bulf·Walker, +a8 4288f0, +ac 4268c0)는 같은 선택 관문(archer=false)을 지난 뒤 공통 열기로 운반체 작성(openCarrier)을 쓴다. (2026-11-21 WB04-E1138)
test('공통 객체 열기: 운반체(4268c0)는 선택 관문(carrier)을 지나 운반체 작성(4288f0)으로 열린다', () => {
    const carrier = scene(table(0x004288f0, 0x004268c0)); assert.equal(carrier.opener.supports(carrier.sid), true);
    assert.equal(carrier.opener.open(carrier.sid, { code: 0x20000, x: 9, y: 10 }), 1);
    assert.deepEqual(carrier.calls.map((call) => Array.isArray(call) ? call[0] : call), ['selection', 'fancy', 'summary', 'carrier', 'attach', 'sound']);
    assert.deepEqual(carrier.calls[0], ['selection', carrier.sid, false]);
    // 건설 중(+28 bit0)인 운반체는 선택 관문 뒤 열기에서 거부되어 메뉴·효과음이 없다.
    const building = scene(table(0x004288f0, 0x004268c0)); building.raw[40] = 1; assert.equal(building.opener.open(building.sid, { code: 0, x: 1, y: 1 }), 0); assert.equal(building.calls.some((call) => call === 'sound'), false);
});
