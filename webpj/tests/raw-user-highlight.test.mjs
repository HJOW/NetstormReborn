// 사용자 강조(RawUserHighlight)의 모드 변경/마우스 타입 초기화/모드별 효과를 기준선 소스 계약으로 검증한다. (2026-11-14 WB04-E442)
// cpppj에도 직접 시험이 없어 원본 직접 관측 수치가 아니다. 순서/경계는 기준선 소스(RawUserHighlight.cpp)에서 옮겼다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { RawUserHighlight } from '../www/core/raw-user-highlight.js';

// 효과를 기록하는 장치 묶음이다. project 가 선택 SID 를 바꾸는 부작용을 낼 수 있다.
function rig({ mode = 0, selectedSid = 9, mouseType = 77, mouseX = 40, mouseY = -30, projectChangesSelection = false } = {}) {
    const log = []; const state = { mode, selectedSid, mouseType, mouseX, mouseY };
    const hooks = { selectCursor: (value) => log.push(['cursor', value, state.mode, state.mouseType]), project: (x, y) => { log.push(['project', x, y]); if (projectChangesSelection) state.selectedSid = 123; return [0x3f800000, 0xc0000000]; }, move: (...args) => log.push(['move', ...args]) };
    return { log, state, gump: new RawUserHighlight(state, hooks), hooks };
}

test('resetMouse: 마우스 TYPE 만 0 으로 쓰고 모드/선택/좌표는 보존한다', () => {
    const r = rig({ mode: 4 }); r.gump.resetMouse(); assert.deepEqual([r.state.mouseType, r.state.mode, r.state.selectedSid, r.state.mouseX, r.state.mouseY], [0, 4, 9, 40, -30]); assert.deepEqual(r.log, []);
});

test('highlight: 같은 모드는 효과도 마우스 TYPE 초기화도 없이 반환하고 변경 때만 모드 쓰기 → 초기화 → 효과 순서다', () => {
    const same = rig({ mode: 2 }); same.gump.highlight(2); assert.deepEqual(same.log, []); assert.equal(same.state.mouseType, 77);
    const changed = rig({ mode: 2 }); changed.gump.highlight(1); assert.equal(changed.state.mode, 1); assert.equal(changed.state.mouseType, 0); assert.deepEqual(changed.log, [['cursor', 3, 1, 0]]);
    // 모드는 DWORD 로 비교한다(2^32+2 는 2 와 같다, -1 은 0xffffffff).
    const wrap = rig({ mode: 2 }); wrap.gump.highlight(0x100000002); assert.deepEqual(wrap.log, []); wrap.gump.highlight(-1); assert.equal(wrap.state.mode, 0xffffffff);
});

test('highlight 모드별 효과: 1~3 은 커서 3, 4 는 커서 2, 그 밖(0·6 이상)은 커서 1 이다', () => {
    for (const [mode, cursor] of [[1, 3], [2, 3], [3, 3], [4, 2], [0, 1], [6, 1], [99, 1], [0xffffffff, 1]]) { const r = rig({ mode: 50 }); r.gump.highlight(mode); assert.deepEqual(r.log, [['cursor', cursor, mode, 0]], String(mode)); assert.equal(r.state.mouseType, 0); }
    const zeroFromOther = rig({ mode: 5 }); zeroFromOther.gump.highlight(0); assert.deepEqual(zeroFromOther.log.map((entry) => entry[1]), [1]);
});

test('highlight 모드 5: 역투영 전에 선택 SID 를 보관하고 현재 정수 좌표로 역투영한 두 DWORD 를 그대로 이동 강조에 넘긴다', () => {
    const r = rig({ projectChangesSelection: true }); r.gump.highlight(5);
    assert.deepEqual(r.log, [['project', 40, -30], ['move', 9, 0x3f800000, 0xc0000000, 0]]); assert.equal(r.state.selectedSid, 123); assert.equal(r.state.mouseType, 0);
});

test('필수 장치 누락은 앞선 원본 쓰기(모드/마우스 TYPE)를 남긴 채 그 위치에서 Error 이다', () => {
    const r = rig({ mode: 0 }); const gump = new RawUserHighlight(r.state, {});
    assert.throws(() => gump.highlight(4), { name: 'Error', message: /실제 사용자 강조 경계 미연결/ }); assert.deepEqual([r.state.mode, r.state.mouseType], [4, 0]);
    const noProject = rig({ mode: 0 }); assert.throws(() => new RawUserHighlight(noProject.state, { move: noProject.hooks.move }).highlight(5), /미연결/); assert.equal(noProject.state.mode, 5);
    const noMove = rig({ mode: 0 }); assert.throws(() => new RawUserHighlight(noMove.state, { project: noMove.hooks.project }).highlight(5), /미연결/); assert.deepEqual(noMove.log, [['project', 40, -30]]);
    // 같은 모드는 장치를 요구하지 않는다.
    assert.doesNotThrow(() => new RawUserHighlight({ mode: 3, mouseType: 1 }, {}).highlight(3));
});
