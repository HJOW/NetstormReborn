// 건설 사제의 Session 연결을 확인하며 실제 생성·포획·희생이나 공중 유닛 행동은 실행하지 않는다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { campaignOwners } from './campaign-session-support.mjs';
import { RegularProcess } from '../www/core/squid-process.js';
import { typeNumber } from '../www/assets/type-table.js';

// 실제 선행 설치를 수행하되 저장 미션과 게임을 시작하지 않는다.
function scene() {
    const o = campaignOwners({ carrierRequest: () => {}, deletion: { selected: () => 0 } });
    try {
        o.connectWorldInstall({ sections: Array.from({ length: 35 }, () => new Uint8Array(0)) });
        o.connectOutcome(); o.connectAiBrains(); o.connectAiWorld(); o.connectAiProduction(); o.connectBuilding(); o.connectMotion(); o.connectDevastation(); o.connectCarrierDrop();
        return o;
    } catch (error) { o.dispose(); throw error; }
}

// 설치 순서와 현재 TYPE·전역 및 실제 Regular 예약 객체를 확인한다.
test('Session 건설 사제: 설치와 현재 상태 및 같은 Regular 예약', () => {
    const o = scene(); let hooks, module;
    try {
        assert.throws(() => o.connectBuilder(), /순서/); o.connectPriestPrayer(); o.connectBuilder();
        module = o.builderModules; hooks = module.hooks; const reads = module.reads;
        assert.equal(o.buildingMover.pool, o.core.pool); assert.equal(o.loaded, false); assert.throws(() => o.connectBuilder(), /중복/);
        o.priestState.movement.teleport = 134; o.priestState.movement.manaBolt = 105; o.core.bridgeState.bridgeType = 82; o.core.processState.now = 27.25;
        assert.deepEqual([reads.builderType(), reads.carrierType(), reads.bridgeType(), reads.approachMask(), reads.now()], [134, 105, 82, 0x50444200, 27.25]);
        const parent = o.core.pool.allocate(2); o.core.pool.allocated(parent)[10] = 82;
        assert.throws(() => hooks.addTimer(parent, 0, reads.timerPayload()), /예약/);
        assert.equal(hooks.reserveTimer(), true); const reserved = module.pendingRegular; assert.ok(reserved instanceof RegularProcess);
        assert.throws(() => hooks.addTimer(parent, 0, 1), /payload/); assert.equal(module.pendingRegular, reserved);
        hooks.addTimer(parent, 7, reads.timerPayload()); assert.equal(o.core.processes.findEvent(parent, 7), reserved);
        assert.equal(reserved.payload, Math.fround(0.05)); assert.equal(reserved.time, 27.25); assert.equal(module.pendingRegular, null);
    } finally { o.dispose(); }
    assert.throws(() => hooks.reserveTimer(), /반납/); assert.throws(() => hooks.anchor(1, 82, 1, 1), /반납/);
});

// 이동 시작의 인수와 경로 현재 시각·TYPE 속도·Kernel 수명 관문만 검사한다.
test('Session 건설 사제: 경로 뷰는 같은 Kernel과 현재 TYPE을 읽는다', () => {
    const o = scene(); let original, pid;
    try {
        o.connectPriestPrayer(); o.connectBuilder(); const hooks = o.builderModules.hooks; const sid = o.core.pool.allocate(2); o.core.pool.allocated(sid)[10] = 134;
        const calls = [], path = { runFrame() {}, destroy() {}, runtime: { count: 2, time: (index) => [5, 9][index] }, prepare: (...args) => calls.push(args) };
        pid = o.core.kernel.add(path); original = o.motion.paths.startConstruction;
        o.motion.paths.startConstruction = (request) => { calls.push(request); return path; };
        const view = hooks.move(sid, 12, 14); assert.deepEqual(calls[0], { parent: sid, destination: { x: 12, y: 14 }, replaceStart: 0, helper: 0, tailWord: 0, tailByte: 0, reserved: 0 });
        assert.equal(view.readArrival(), 9); hooks.speed(view, 3, 0);
        const definition = o.assets.game.types()[134 - 70].definition;
        assert.deepEqual(calls[1], [3, Math.fround(typeNumber(definition, 'speed')), Math.fround(typeNumber(definition, 'turningSpeed'))]);
        assert.throws(() => view.prepare(3, 1), /경로\/flags/); o.core.kernel.remove(pid); pid = null;
        assert.throws(() => view.readArrival(), /수명 종료/);
    } finally { if (original) o.motion.paths.startConstruction = original; if (pid) o.core.kernel.remove(pid); o.dispose(); }
});
