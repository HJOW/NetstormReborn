// 개발자 객체 명령의 권한 분기·실제 장치 순서·SID 경계를 확인한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { GameAuxState } from '../www/core/game-aux-state.js';
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

// create는 두 signed 입력 뒤 서버에서만 frame을 직접 쓰고 같은 좌표 20·20에 배치한다.
test('개발자 객체: create의 입력·서버 권한·frame과 Pop 순서', () => {
    const s = scene(), raw = s.slots[7];
    s.owner.world.create = (...args) => { s.calls.push(['create', ...args]); return 7; };
    s.owner.world.pop = (...args) => s.calls.push(['pop', ...args]);
    executeDeveloperObjects('c -1 -2', s.owner, s.trace);
    assert.deepEqual(s.calls, [['create', 0xffffffff, 0], ['pop', 7, 20, 20, 0]]);
    assert.equal(new DataView(raw.buffer).getUint32(36, true), 0xfffffffe);
    s.calls.length = 0; s.owner.core.processState.server = false;
    assert.equal(executeDeveloperObjects('create 70 2', s.owner, s.trace), true);
    assert.deepEqual(s.calls, []);
    s.owner.core.processState.server = true; executeDeveloperObjects('create 70', s.owner, s.trace);
    assert.deepEqual(s.calls, [['trace', 29, 'bad command.\n']]);
    s.calls.length = 0; s.owner.world.create = () => 0;
    assert.throws(() => executeDeveloperObjects('c 70 2', s.owner, s.trace), /NULL 슬롯/);
});

// scatter는 열마다 생성·현재 TYPE 프레임 수·CRT 소비·frame/Pop 순서를 반복하고 행은 마지막에 한 번만 회전한다.
test('개발자 객체: scatter의 열·CRT·행 회전과 클라이언트 생략', () => {
    const s = scene(), aux = new GameAuxState(), frames = Array.from({ length: 188 }, () => ({ codes: [] }));
    const rolls = [9, 2, 7, 4, 1, 6, 3, 8, 5, 0], created = [];
    frames[70].codes = Array.from({ length: 4 }, () => ({})); s.owner.core.frames = frames;
    s.owner.gameplayGlobals = { gameAux: () => aux };
    s.owner.particleCrt = { next: () => { const roll = rolls.shift(); s.calls.push(['crt', roll]); return roll; } };
    s.owner.world.create = (type, flags) => {
        const target = created.length + 1; created.push(target); s.slots[target][10] = 70;
        s.calls.push(['create', target, type, flags]); return target;
    };
    s.owner.world.pop = (...args) => s.calls.push(['pop', ...args]);
    aux.developerScatterRow = 9;
    assert.equal(executeDeveloperObjects('sct 70', s.owner, s.trace), true);
    assert.deepEqual(created, [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]);
    assert.equal(aux.developerScatterRow, 1); assert.deepEqual(rolls, []);
    assert.deepEqual(created.map((target) => new DataView(s.slots[target].buffer).getUint32(36, true)), [1, 2, 3, 0, 1, 2, 3, 0, 1, 0]);
    assert.deepEqual(s.calls.filter(([name]) => name === 'pop'), created.map((target, index) => ['pop', target, 3 + index * 3, 9, 0]));
    // 각 생성 슬롯의 CRT 소비가 해당 Pop보다 먼저 일어났는지 확인한다.
    for (let index = 0; index < created.length; index += 1) {
        const create = s.calls.indexOf(s.calls.find((call) => call[0] === 'create' && call[1] === created[index]));
        const crt = s.calls.findIndex((call, callIndex) => callIndex > create && call[0] === 'crt');
        const pop = s.calls.findIndex((call, callIndex) => callIndex > crt && call[0] === 'pop' && call[1] === created[index]);
        assert.ok(create < crt && crt < pop);
    }
    s.calls.length = 0; s.owner.core.processState.server = false; rolls.push(123);
    executeDeveloperObjects('scatter 70 trailing', s.owner, s.trace);
    assert.deepEqual(s.calls, []); assert.deepEqual(rolls, [123]); assert.equal(aux.developerScatterRow, 1);
    s.owner.core.processState.server = true; executeDeveloperObjects('scatter', s.owner, s.trace);
    assert.deepEqual(s.calls, [['trace', 29, 'bad command.\n']]);
});
