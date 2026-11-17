// Session 운반 투하의 공통 명령·현재 진단 뷰와 실제 위치 앞단을 확인한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { campaignOwners } from './campaign-session-support.mjs';

// 같은 실제 기반 owner에서 게임 Load 없이 이동과 파괴 주문까지 설치한다.
function scene() {
    const o = campaignOwners({ carrierRequest: () => {}, deletion: { selected: () => 0 } });
    try {
        o.connectWorldInstall({ sections: Array.from({ length: 35 }, () => new Uint8Array(0)) });
        o.connectOutcome(); o.connectAiBrains(); o.connectAiWorld(); o.connectAiProduction(); o.connectBuilding(); o.connectMotion();
        return o;
    } catch (error) { o.dispose(); throw error; }
}

// 사제 포획이나 투하를 실행하지 않고 현재 상태와 선행 배치 경계를 검증한다.
test('Session 운반 투하 앞단: 공통 명령과 현재 지도·진단 뷰', () => {
    const o = scene(); let module;
    try {
        assert.throws(() => o.connectCarrierDropFront(), /순서/);
        o.connectDevastation(); o.connectCarrierDropFront(); module = o.carrierDropModules;
        assert.equal(module.ready, true); assert.equal(o.carrierDropPoint.pool, o.core.pool);
        assert.equal(o.objectCommand.command(0, 0), 0);
        o.playerState.battleOwner = 3; o.cursorManaGlobals.mapLimit = 96;
        const checking = o.gameplayGlobals.aiDebug().checking(); checking.value = 11;
        o.prepareCarrierDrop(); assert.equal(o.state.carrierDropState.localOwner, 3); assert.equal(module.pointState.mapLimit, 96);
        checking.value = 22; assert.equal(o.state.carrierDropState.checking, 22);
        o.state.gunState.checking = 0; checking.value = 33; assert.equal(o.state.carrierDropState.checking, 33);
        const sid = o.world.create(82, 2); assert.deepEqual(module.pointHooks.center(sid), o.gunTarget.center(sid));
        assert.throws(() => module.pointHooks.mayPlace({}), /MayPlace 미설치/);
        assert.equal(o.loaded, false); assert.throws(() => o.connectCarrierDropFront(), /순서/);
    } finally { o.dispose(); }
    assert.throws(() => module.prepare(), /반납/);
});

// 프로세스 체인의 다음 링크와 미연결 화면 경계만 확인하고 투하는 실행하지 않는다.
test('Session 운반 투하 요청: 프로세스 체인과 현재 카메라', () => {
    const o = scene(); let hooks;
    try {
        o.connectDevastation(); o.connectCarrierDropFront(); o.connectCarrierDropRequest(); hooks = o.carrierDropModules.dropHooks;
        assert.equal(o.carrierDropModules.requestReady, true); assert.equal(o.carrierDrop, null);
        const pool = o.core.pool, parent = pool.allocate(2), first = pool.allocate(2), second = pool.allocate(2);
        const a = pool.physical(parent), b = pool.physical(first), c = pool.physical(second);
        new DataView(a.buffer, a.byteOffset, a.byteLength).setUint16(6, first, true);
        new DataView(b.buffer, b.byteOffset, b.byteLength).setUint16(4, second, true); b[10] = 30; c[10] = 31;
        const seen = []; hooks.attached(parent, (sid) => { seen.push(sid); if (sid === first) { b[4] = 0; b[5] = 0; } });
        assert.deepEqual(seen, [first, second]);
        new DataView(b.buffer, b.byteOffset, b.byteLength).setUint16(4, second, true);
        new DataView(c.buffer, c.byteOffset, c.byteLength).setUint16(4, first, true);
        assert.throws(() => hooks.attached(parent, () => {}), /체인 손상/);
        o.gameplayGlobals.camera().snapshot().setPixels(16, 11); assert.deepEqual(hooks.project([2, 3]), [16, 22]);
        assert.equal(hooks.baseMenu(parent, 0, 0), 0); assert.throws(() => hooks.select(0, 0, 1), /선택 수명/);
        assert.throws(() => hooks.popup(1, [0, 0], 0), /Gump 미연결/);
        assert.equal(o.loaded, false); assert.throws(() => o.connectCarrierDropRequest(), /순서/);
    } finally { o.dispose(); }
    assert.throws(() => hooks.project([0, 0]), /반납/);
});

// 실제 투하를 실행하지 않고 서버 수신의 인자 폭과 현재 Player 상태를 검증한다.
test('Session 운반 투하 전체: 서버 효과와 현재 상태', () => {
    const o = scene(); let hooks, originalRequest;
    try {
        o.connectDevastation(); o.connectCarrierDrop(); hooks = o.carrierDropModules.dropHooks;
        assert.equal(o.carrierDropModules.receiveReady, true); assert.equal(o.carrierDrop.pool, o.core.pool);
        const queue = []; o.services.assets.queueCommand = (...args) => queue.push(args);
        hooks.queue(0x030000b2, 0xffffffff, 0x80000000); assert.deepEqual(queue, [[0x030000b2, -1, -2147483648]]);
        const sid = o.core.pool.allocate(2), raw = o.core.pool.allocated(sid); raw[34] = 2;
        hooks.argument(sid, 0x12345678); assert.deepEqual([raw[12], raw[13]], [0x78, 0x56]);
        const players = new DataView(o.players.buffer, o.players.byteOffset, o.players.byteLength);
        assert.equal(hooks.standing(sid), false); players.setUint32(2 * 180 + 0xb0, 1, true);
        assert.equal(hooks.standing(sid), true); players.setUint32(2 * 180 + 0xb0, 0, true);
        const requests = []; originalRequest = o.construction.request; o.construction.request = (value) => { requests.push(value); return true; };
        assert.equal(hooks.place(82, [4, 5], 2, 6, 7, 8, 1.5, 9), true);
        assert.deepEqual(requests[0], { type: 82, x: 4, y: 5, player: 2, argument: 6, direction: 7, flags: 8, timeLow: 0, timeHigh: 0x3ff80000, quality: 9 });
        assert.throws(() => hooks.create(999, 0), /TYPE 범위/); assert.throws(() => o.receiveCarrierDrop(1, new Uint8Array(0)), /패킷 길이/);
        assert.throws(() => hooks.activate(0, 0, new Uint8Array(0)), /부모 WORD 부족/);
        assert.equal(o.loaded, false); assert.throws(() => o.connectCarrierDrop(), /중복/);
    } finally { if (originalRequest) o.construction.request = originalRequest; o.dispose(); }
    assert.throws(() => hooks.money(1), /반납/);
});
