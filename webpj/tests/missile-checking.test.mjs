// 투사체의 현재 checking 관문과 항상 참인 Baseprocess 범위 진단(0041be80)의 두 위치, 진단 뒤 현재 필드 재조회를 합성 입력으로 검사한다(raw-missile.js).
//  - 비충돌 직선 이동과 사거리 종료만 준비하며 실제 공격/게임 틱은 실행하지 않는다. 진단 장치가 선택되기 전의 이동과 빗나감 요청 소비를 일괄 사전검사로 되돌리지 않는다.
//  - 실제 Host 의 자기 삭제 관찰(alive)은 `frame(fields, alive)` 의 alive 함수로 합성한다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E125: RawMissile 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import { put, get, bitsToFloat, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { MissileFields } from '../www/core/raw-missile-fields.js';
import { MissileState, RawMissile } from '../www/core/raw-missile.js';

// 풀 용량(진단이 읽는 현재 상한)이다. (2026-11-08 WB04-E125)
const CAPACITY = 24000;

// 예외가 나는지 본다. (2026-11-08 WB04-E125)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 비충돌 직선 이동과 사거리 종료만 준비하는 합성 장면이다. (2026-11-08 WB04-E125)
function makeScene() {
    const scene = {
        pool: new SidPool(CAPACITY, { server: false }),
        types: Array.from({ length: 188 }, () => ({ footX: 0, footY: 0, flags1: 0, flags2: 0, group: 0, hotspotX: 0, hotspotY: 0, maxHitPoints: 0 })),
        supplement: Array.from({ length: 188 }, () => ({ range: 0, frameCount: 0, firingSound: 0, impactSound: 0, damagePerSecond: 0, specialFlyerDamage: false, flyerDamagePerSecond: 0 })),
        fields: new MissileFields(),
        checking: 0,
        events: [],
        hooks: null
    };
    scene.state = new MissileState({ now: 1, checking: () => scene.checking });
    // 원문 입력 세 자산과 후반 재조회 대상을 실제 할당한다.
    for (let id = 5; id <= 16; id += 1) {
        assert.equal(scene.pool.allocate(2), id);
        scene.pool.allocated(id).fill(0);
    }
    scene.pool.allocated(10)[10] = 128;
    scene.pool.allocated(11)[10] = 71;
    scene.pool.allocated(12)[10] = 80;
    scene.pool.allocated(12)[34] = 3;
    scene.types[128].group = 1;
    scene.types[0].group = 7;
    scene.supplement[0].damagePerSecond = 10;
    put(scene.fields.bytes, 4, 13);
    put(scene.fields.bytes, 8, 12);
    put(scene.fields.bytes, 20, 10);
    put(scene.fields.bytes, 24, 11);
    // 입력의 float 비트를 그대로 저장한다.
    scene.point = (offset, x, y) => {
        put(scene.fields.bytes, offset, floatToBits(x));
        put(scene.fields.bytes, offset + 4, floatToBits(y));
    };
    // 판본 시각 8바이트를 준비한다.
    scene.double = (offset, value) => {
        const view = new DataView(new ArrayBuffer(8));
        view.setFloat64(0, value, true);
        put(scene.fields.bytes, offset, view.getUint32(0, true));
        put(scene.fields.bytes, offset + 4, view.getUint32(4, true));
    };
    scene.point(56, 10, 10);
    scene.point(72, 20, 10);
    scene.point(64, 20, 10);
    scene.point(120, -10000, -10000);
    scene.double(104, 2);
    scene.double(112, 0);
    scene.hooks = {
        unpop: (sid, flags) => { assert.equal(flags, 0); scene.events.push(`U${sid}`); },
        pop: (sid, x, y, flags) => { assert.ok(sid === 12 && y === 10 && flags === 0); scene.events.push(`P${floatToBits(x)}`); },
        targetBounds: (sid) => { assert.equal(sid, 11); scene.events.push('B'); return [100, 100, 101, 101]; },
        center: (sid) => { assert.equal(sid, 10); scene.events.push('C'); return [10, 10]; },
        refresh: (sid) => scene.events.push(`Q${sid}`),
        destroy: (sid, flags) => { assert.equal(flags, 0); scene.events.push(`X${sid}`); },
        hit: () => { throw new Error('투사체 진단 검사의 미선택 직접 피해'); },
        damage: (sid, amount, owner, attacker) => {
            assert.ok(sid === 11 && amount === 6 && owner === 3 && attacker === 0);
            scene.events.push('D');
        }
    };
    // 미도달 Missile.cpp false 단언을 성공 사건으로 만들지 않는다.
    scene.diagnostic = (expression, file, line) => {
        assert.equal(file, '\\Ns\\O\\Baseprocess.cpp');
        assert.equal(expression, line === 0x69 ? 'formSid == 0 || form.isValid()' : 'attachToSid == 0 || s.isValid()');
        assert.ok(line === 0x69 || line === 0x6c);
        scene.events.push(`A${line}`);
    };
    // 후반 진단보다 뒤에 기록되는 이전 좌표 필드를 읽는다.
    scene.previous = () => bitsToFloat(get(scene.fields.bytes, 120));
    // 검사마다 같은 입력 주소를 직접 빌린다.
    scene.run = () => new RawMissile(scene.pool, scene.types, scene.supplement, scene.state, scene.hooks).frame(scene.fields);
    return scene;
}

test('MissileChecking: 첫 진단은 전체 DWORD 를 쓰고 조회 전에 현재 SID/시각을 고치며 flag 가 꺼져도 두 번째 범위 진단은 계속된다', () => {
    const scene = makeScene();
    scene.checking = 0x80000000;
    put(scene.fields.bytes, 4, 0x10005);
    put(scene.fields.bytes, 8, 0x10006);
    put(scene.fields.bytes, 20, 65535);
    scene.hooks.assertion = (expression, file, line) => {
        scene.diagnostic(expression, file, line);
        if (line === 0x69) {
            put(scene.fields.bytes, 4, 0);
            scene.checking = 0;
            scene.state.now = 0.5;
        } else {
            put(scene.fields.bytes, 8, 12);
            put(scene.fields.bytes, 20, 10);
        }
    };
    scene.run();
    assert.deepEqual(scene.events, ['A105', 'A108', 'U12', 'P1095237632', 'B', 'C']);
    assert.equal(scene.previous(), 12.5);
});

test('MissileChecking: 후반 진단은 이전 좌표 저장보다 먼저이며 최종 발사자만 새 SID 로 다시 조회한다', () => {
    const scene = makeScene();
    const ordinaryPop = scene.hooks.pop;
    scene.hooks.pop = (sid, x, y, flags) => {
        ordinaryPop(sid, x, y, flags);
        scene.checking = 0x80000000;
        put(scene.fields.bytes, 4, 0x10005);
    };
    scene.hooks.assertion = (expression, file, line) => {
        scene.diagnostic(expression, file, line);
        assert.equal(scene.previous(), -10000.0);
        put(scene.fields.bytes, 4, 0);
        put(scene.fields.bytes, 8, 14);
        put(scene.fields.bytes, 20, 14);
        scene.pool.allocated(11)[11] = 2;
    };
    scene.run();
    assert.deepEqual(scene.events, ['U12', 'P1097859072', 'B', 'C', 'A105', 'D', 'X12']);
    assert.equal(scene.previous(), 15.0);
});

test('MissileChecking: 진단 장치가 선택되기 전의 이동과 빗나감 요청 소비를 일괄 사전검사로 되돌리지 않는다', () => {
    const scene = makeScene();
    scene.supplement[128].range = 40;
    put(scene.fields.bytes, 40, 1);
    const ordinaryPop = scene.hooks.pop;
    scene.hooks.pop = (sid, x, y, flags) => {
        ordinaryPop(sid, x, y, flags);
        scene.checking = 1;
        put(scene.fields.bytes, 8, CAPACITY);
    };
    assert.ok(throws(() => scene.run()));
    assert.deepEqual(scene.events, ['U12', 'P1097859072', 'B', 'C']);
    assert.ok(get(scene.fields.bytes, 40) === 0 && get(scene.fields.bytes, 44) === 1 && scene.previous() === -10000.0);
    scene.hooks.pop = ordinaryPop;
    put(scene.fields.bytes, 8, 12);
    scene.events.length = 0;
    scene.run();
    // 유효 SID 면 checking 1 이어도 assertion 장치는 필요 없다.
    assert.ok(scene.previous() === 15.0 && scene.events.length === 4);
});

test('MissileChecking: 좌표 실패 조기 종료는 첫 진단만 거쳐 빗나감 음향과 저장 표시 삭제를 요청하고 이전 좌표를 쓰지 않는다', () => {
    const scene = makeScene();
    scene.checking = 1;
    put(scene.fields.bytes, 4, CAPACITY);
    put(scene.fields.bytes, 40, 1);
    scene.double(104, 0);
    scene.double(112, 0);
    scene.hooks.assertion = (expression, file, line) => {
        scene.diagnostic(expression, file, line);
        put(scene.fields.bytes, 4, 0);
    };
    scene.hooks.impactSound = () => {
        scene.events.push('S');
        put(scene.fields.bytes, 4, CAPACITY);
    };
    scene.run();
    assert.deepEqual(scene.events, ['A105', 'S', 'X12']);
    assert.equal(scene.previous(), -10000.0);
});

test('MissileChecking: 이동 중 현재 flag 를 끄면 후반 Form 진단을 실행하지 않는다', () => {
    const scene = makeScene();
    scene.checking = 0x80000000;
    const ordinaryPop = scene.hooks.pop;
    scene.hooks.pop = (sid, x, y, flags) => {
        ordinaryPop(sid, x, y, flags);
        scene.checking = 0;
        put(scene.fields.bytes, 4, CAPACITY);
    };
    scene.run();
    assert.deepEqual(scene.events, ['U12', 'P1097859072', 'B', 'C', 'Q10', 'X12']);
    assert.equal(get(scene.fields.bytes, 4), CAPACITY);
});

test('MissileChecking: 호스트가 진단 중 자기 삭제하면(alive 거짓) 남은 필드를 읽거나 이전 좌표/종료를 반복하지 않는다', () => {
    // 첫 진단에서 삭제: 진단 하나만 실행하고 끝난다.
    const early = makeScene();
    let aliveEarly = true;
    early.checking = 0x80000000;
    put(early.fields.bytes, 4, 0x10005);
    early.hooks.assertion = (expression, file, line) => { early.diagnostic(expression, file, line); aliveEarly = false; };
    new RawMissile(early.pool, early.types, early.supplement, early.state, early.hooks).frame(early.fields, () => aliveEarly);
    assert.deepEqual(early.events, ['A105']);
    assert.equal(early.previous(), -10000.0);
    // 후반 진단에서 삭제: 이동/범위 계산 뒤라도 이전 좌표 저장과 종료를 반복하지 않는다.
    const late = makeScene();
    let aliveLate = true;
    const ordinaryPop = late.hooks.pop;
    late.hooks.pop = (sid, x, y, flags) => {
        ordinaryPop(sid, x, y, flags);
        late.checking = 0x80000000;
        put(late.fields.bytes, 4, 0x10005);
    };
    late.hooks.assertion = (expression, file, line) => { late.diagnostic(expression, file, line); aliveLate = false; };
    new RawMissile(late.pool, late.types, late.supplement, late.state, late.hooks).frame(late.fields, () => aliveLate);
    assert.deepEqual(late.events, ['U12', 'P1097859072', 'B', 'C', 'A105']);
    assert.equal(late.previous(), -10000.0);
});
