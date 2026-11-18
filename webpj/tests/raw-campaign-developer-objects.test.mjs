// 개발자 객체 명령의 권한 분기·실제 장치 순서·SID 경계를 확인한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { executeDeveloperObjects } from '../www/core/raw-campaign-developer-objects.js';

// 같은 슬롯과 선택 효과를 기록하는 독립 Session 입력이다.
function scene() {
    const calls = [], slots = Array.from({ length: 32 }, () => new Uint8Array(50));
    const pool = { edition: 'patch1078', capacity: slots.length, slot: (sid) => slots[sid], allocated: (sid) => slots[sid] }, owner = {
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

// HP와 추상 상태는 Unpop 뒤 현재 슬롯에 쓰고 Repop을 호출한다.
test('개발자 객체: HP·추상 상태와 피해 전달', () => {
    const s = scene(), raw = s.slots[5];
    s.owner.hp = { maxHitPoints: () => 100 };
    s.owner.world.storage = { assertDisplay: (...args) => { s.calls.push(['assert', ...args]); raw[40] = 0xfe; } };
    s.owner.developerDamage = (...args) => s.calls.push(['damage', ...args]);
    executeDeveloperObjects('slhp 5 -33', s.owner, s.trace);
    assert.equal(new DataView(raw.buffer).getInt32(26, true), -33);
    assert.deepEqual(s.calls, [['unpop', 5, 0], ['repop', 5, 0], ['highlight', 2.5, 7.5], ['trace', 29, 'sid 5 changed to -33 hp.  (max for type = 100)\n']]);
    s.calls.length = 0; executeDeveloperObjects('sa 5 1', s.owner, s.trace);
    assert.equal(raw[40], 0xff);
    assert.deepEqual(s.calls, [['unpop', 5, 0], ['assert', 'isVoid()', 'c:\\ns\\o\\squid.h', 0x282], ['repop', 5, 0]]);
    s.calls.length = 0; executeDeveloperObjects('ad 5 -1', s.owner, s.trace);
    assert.deepEqual(s.calls, [['damage', 5, 0xffffffff, 0, 0]]);
    s.calls.length = 0; executeDeveloperObjects('ad -1 4', s.owner, s.trace); executeDeveloperObjects('slhp 5', s.owner, s.trace);
    assert.deepEqual(s.calls, [['trace', 29, 'bad command\n'], ['trace', 29, 'bad command.\n']]);
});
