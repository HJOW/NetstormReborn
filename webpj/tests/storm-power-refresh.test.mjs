// 현재 serial의 SP 예산 Gump 재부착(RawStormPowerRefresh)을 기준선 소스 계약으로 검증한다. (2026-11-14 WB04-E437)
// cpppj에도 직접 시험이 없어 원본 직접 관측 수치가 아니다. 순서/경계는 기준선 소스(RawStormPowerRefresh.cpp)에서 옮겼다. 등록은 실제 GumpCreate 로 하고 Push/Attach 는 기록 장치를 쓴다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { GumpCreateNode, GumpCreateState, GumpCreate, GUMP_REGISTRY_CAPACITY } from '../www/ui/gump-create.js';
import { findCurrentGumpSerial, refreshStormPowerGump } from '../www/ui/storm-power-refresh.js';

// 실제 GumpCreate 로 노드를 등록하는 틀이다. token 100+번호의 노드가 등록 1,2,… 칸에 serial 1,2,… 로 들어간다.
function rig(count = 3) {
    const nodes = new Map(); const state = new GumpCreateState(); const create = new GumpCreate(state, { node: (token) => nodes.get(token) ?? null });
    const log = []; const lookups = [];
    // 루트(0칸)를 먼저 등록해 serial 1 을 차지하게 한다.
    nodes.set(99, new GumpCreateNode()); create.construct(99, 0, 640, 480, 0);
    for (let index = 0; index < count; index += 1) { nodes.set(100 + index, new GumpCreateNode()); create.construct(100 + index, 7, 10, 10, 0); }
    const hooks = { node: (token) => { lookups.push(token); return nodes.get(token) ?? null; }, push: (token) => log.push(['push', token]), attach: (token, x, y, a, b) => log.push(['attach', token, x, y, a, b]) };
    return { nodes, state, entries: state.entries, log, lookups, hooks };
}

test('findCurrentGumpSerial: 등록 1..999만 앞에서부터 조회해 serial 이 같고 dead 가 아닌 첫 토큰을 돌려준다(0칸은 건너뛰고 빈 칸은 조회하지 않는다)', () => {
    const r = rig();
    // 등록표는 0번 99(첫 구멍), 1번 100, 2번 101, 3번 102 이며 serial 은 1,2,3,4 다.
    assert.deepEqual(Array.from(r.entries.subarray(0, 5)), [99, 100, 101, 102, 0]);
    assert.equal(findCurrentGumpSerial(r.entries, 3, r.hooks.node), 101); assert.deepEqual(r.lookups, [100, 101]);
    // 0칸의 루트(serial 1)는 후보가 아니라 못 찾는다. 앞 칸이 비어 있으면 조회 없이 넘어간다.
    r.lookups.length = 0; assert.equal(findCurrentGumpSerial(r.entries, 1, r.hooks.node), 0); assert.deepEqual(r.lookups, [100, 101, 102]);
    // 없는 serial 은 0 이며 다른 값 비교는 DWORD 라 -1 과 0xffffffff 가 같다.
    r.nodes.get(102).serial = 0xffffffff; assert.equal(findCurrentGumpSerial(r.entries, -1, r.hooks.node), 102); assert.equal(findCurrentGumpSerial(r.entries, 0xffffffff, r.hooks.node), 102);
    assert.equal(findCurrentGumpSerial(r.entries, 77, r.hooks.node), 0);
});

test('findCurrentGumpSerial: dead 비트(0x10000)가 켜진 노드는 serial 이 같아도 건너뛰고 다른 비트는 영향이 없다', () => {
    const r = rig(); const second = r.nodes.get(101);
    second.flags |= 0x10000; assert.equal(findCurrentGumpSerial(r.entries, 3, r.hooks.node), 0);
    // 같은 serial 의 뒤 칸이 있으면 그것을 찾는다.
    r.nodes.get(102).serial = 3; assert.equal(findCurrentGumpSerial(r.entries, 3, r.hooks.node), 102);
    second.flags = (second.flags & ~0x10000) | 0x20000 | 0x8000; assert.equal(findCurrentGumpSerial(r.entries, 3, r.hooks.node), 101);
    // 0x10000 과 인접한 비트(최하위 비트 1, 0x8000, 0x20000)만 켜져 있으면 살아 있는 노드다.
    second.flags = 1; assert.equal(findCurrentGumpSerial(r.entries, 3, r.hooks.node), 101); second.flags = 0x1ffff & ~0x10000; assert.equal(findCurrentGumpSerial(r.entries, 3, r.hooks.node), 101);
});

test('findCurrentGumpSerial: 1000칸 미만 표는 TypeError, 장치/노드 누락은 해당 조회 위치에서만 Error/RangeError 이고 999번 칸까지 본다', () => {
    const r = rig();
    assert.throws(() => findCurrentGumpSerial(new Uint32Array(999), 1, r.hooks.node), TypeError); assert.throws(() => findCurrentGumpSerial(null, 1, r.hooks.node), TypeError);
    // 빈 표는 node 를 요구하지 않는다(조회할 토큰이 없다).
    assert.equal(findCurrentGumpSerial(new Uint32Array(GUMP_REGISTRY_CAPACITY), 1, undefined), 0);
    assert.throws(() => findCurrentGumpSerial(r.entries, 3, undefined), { name: 'Error', message: /주소 조회 누락/ });
    // 노드가 사라진 토큰은 그 위치에서 RangeError 이며 뒤 칸은 보지 않는다.
    r.lookups.length = 0; assert.throws(() => findCurrentGumpSerial(r.entries, 4, (token) => { r.lookups.push(token); return token === 101 ? null : r.nodes.get(token); }), RangeError); assert.deepEqual(r.lookups, [100, 101]);
    // 999번 칸은 조회 대상이고 1000번 이상은 없다.
    const wide = new Uint32Array(1200); wide[999] = 100; wide[1000] = 101; r.lookups.length = 0; assert.equal(findCurrentGumpSerial(wide, 2, r.hooks.node), 100); assert.deepEqual(r.lookups, [100]);
    wide[999] = 0; assert.equal(findCurrentGumpSerial(wide, 3, r.hooks.node), 0); assert.equal(r.lookups.length, 1);
});

test('refreshStormPowerGump: 못 찾으면 장치를 부르지 않고, 찾으면 깊이 보관→Push→Push 뒤 현재 x/y 와 0/0 으로 Attach→깊이 복원 순서다', () => {
    const r = rig(); assert.equal(refreshStormPowerGump(r.entries, 99, r.hooks), undefined); assert.deepEqual(r.log, []);
    const node = r.nodes.get(101); node.x = 3.5; node.y = -2.25; node.depth = 0xffffffd8;
    // Push 가 좌표와 깊이를 바꾼다. Attach 는 그 뒤 좌표를 받고 Attach 도 깊이를 바꾼다.
    r.hooks.push = (token) => { r.log.push(['push', token]); node.x = 40; node.y = 50; node.depth = 11; };
    r.hooks.attach = (token, x, y, a, b) => { r.log.push(['attach', token, x, y, a, b, node.depth]); node.depth = 22; };
    refreshStormPowerGump(r.entries, 3, r.hooks);
    assert.deepEqual(r.log, [['push', 101], ['attach', 101, 40, 50, 0, 0, 11]]); assert.equal(node.depth, 0xffffffd8);
});

test('refreshStormPowerGump: Push/Attach 장치 누락과 도중 예외는 깊이를 복원하지 않으며 해당 위치에서 거부한다', () => {
    const r = rig(); const node = r.nodes.get(100); node.depth = 5;
    r.hooks.push = undefined; assert.throws(() => refreshStormPowerGump(r.entries, 2, r.hooks), { name: 'Error', message: /Push 누락/ }); assert.deepEqual(r.log, []);
    // Push 는 실행되고 깊이를 바꾼 뒤 Attach 누락에서 중단한다. 복원은 일어나지 않는다.
    r.hooks.push = (token) => { r.log.push(['push', token]); node.depth = 9; }; r.hooks.attach = undefined;
    assert.throws(() => refreshStormPowerGump(r.entries, 2, r.hooks), { name: 'Error', message: /Attach 누락/ }); assert.deepEqual(r.log, [['push', 100]]); assert.equal(node.depth, 9);
    // Push 가 던지면 Attach 는 부르지 않는다.
    r.log.length = 0; r.hooks.push = () => { throw new Error('push fail'); }; r.hooks.attach = () => r.log.push(['attach']);
    assert.throws(() => refreshStormPowerGump(r.entries, 2, r.hooks), /push fail/); assert.deepEqual(r.log, []);
    // Attach 가 던지면 깊이는 그 시점 값으로 남는다.
    node.depth = 5; r.hooks.push = () => { node.depth = 8; }; r.hooks.attach = () => { throw new Error('attach fail'); };
    assert.throws(() => refreshStormPowerGump(r.entries, 2, r.hooks), /attach fail/); assert.equal(node.depth, 8);
    // node 조회 장치가 없으면 찾는 단계에서 Error 다.
    assert.throws(() => refreshStormPowerGump(r.entries, 2, { push() {}, attach() {} }), { name: 'Error', message: /주소 조회 누락/ });
});
