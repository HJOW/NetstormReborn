// 개발자 객체 명령의 권한 분기·실제 장치 순서·SID 경계를 확인한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { executeDeveloperObjects } from '../www/core/raw-campaign-developer-objects.js';

// 같은 슬롯과 선택 효과를 기록하는 독립 Session 입력이다.
function scene() {
    const calls = [], slots = Array.from({ length: 32 }, () => new Uint8Array(50));
    const pool = { capacity: slots.length, slot: (sid) => slots[sid] }, owner = {
        core: { pool, processState: { server: true } },
        world: { unpop: (...args) => calls.push(['unpop', ...args]), repop: (...args) => calls.push(['repop', ...args]), destroy: (...args) => calls.push(['destroy', ...args]) },
        ownership: { setOwner: (...args) => calls.push(['owner', ...args]) },
        services: { developerHighlight: (...args) => calls.push(['highlight', ...args]), developerTraceObject: (...args) => calls.push(['dump', ...args]) },
    };
    const trace = (...args) => calls.push(['trace', ...args]);
    new DataView(slots[5].buffer).setFloat32(14, 2.5, true); new DataView(slots[5].buffer).setFloat32(18, 7.5, true);
    return { owner, slots, calls, trace };
}

// 서버 소유자 변경은 Unpop·부호 BYTE 소유자·Repop 순이고 삭제는 빈 슬롯을 건너뛴다.
test('개발자 객체: 서버 소유자·삭제 순서', () => {
    const s = scene();
    assert.equal(executeDeveloperObjects('so 5 255', s.owner, s.trace), true);
    assert.deepEqual(s.calls, [['unpop', 5, 0], ['owner', 5, 0xffffffff], ['repop', 5, 8]]);
    s.calls.length = 0; executeDeveloperObjects('de 5', s.owner, s.trace); s.slots[5][11] = 1;
    executeDeveloperObjects('destroy 5', s.owner, s.trace); assert.deepEqual(s.calls, [['destroy', 5, 0]]);
    assert.throws(() => executeDeveloperObjects('so 5', s.owner, s.trace), /미정 정수/);
});

// 클라이언트는 소유자를 바꾸지 않고 좌표 강조와 로그만 낸다.
test('개발자 객체: 클라이언트 강조와 조회 경계', () => {
    const s = scene(); s.owner.core.processState.server = false;
    executeDeveloperObjects('setowner 5 255', s.owner, s.trace);
    executeDeveloperObjects('dump 5', s.owner, s.trace);
    assert.deepEqual(s.calls, [['highlight', 2.5, 7.5], ['trace', 29, 'requested owner on 5 set to -1\n'], ['highlight', 2.5, 7.5], ['dump', 5]]);
    s.calls.length = 0; executeDeveloperObjects('de 2147483648', s.owner, s.trace); executeDeveloperObjects('dump', s.owner, s.trace);
    assert.deepEqual(s.calls, []); assert.throws(() => executeDeveloperObjects('dump 32', s.owner, s.trace), /SID 물리 범위/);
    assert.equal(executeDeveloperObjects('other', s.owner, s.trace), false);
});
