// 객체 전환 raw-object-conversion.js(RawObjectConversion·objectConversionBody·makeObjectConversionHooks)와 SquidFactory.retype(004b1620)을 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: squid-retype-x86.tsv 128행(이전/새 TYPE·상태·발자국 불일치 비트 → 타입 변경 뒤 50바이트 전체 raw 와 진단 행 순서·풀 free 개수 불변·다른 슬롯 보존),
//    object-conversion-x86.tsv 36행(비석/주문 Command 0x030000ce 의 상위 비트·미처리 명령 부모 위임·Unpop 뒤 TYPE/QA 재조회 → 반환·50바이트 raw·U/P/B/A 순서).
//    실제 Session 바인더(makeObjectConversionHooks)로도 36행과 128행을 다시 재생한다.
//  - 계약: 선택하지 않은 경계는 요구하지 않고 도달한 누락은 앞선 쓰기를 보존한 채 거부하며, 진단 행 범위·풀 불일치를 거부한다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E106: 객체 전환 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidFactory } from '../www/core/squid-factory.js';
import { RawObjectConversion, makeObjectConversionHooks, objectConversionBody } from '../www/core/raw-object-conversion.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E106)
const fixtures = path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures');
const retypeRows = readRows(path.join(fixtures, 'squid-retype-x86.tsv'));
const conversionRows = readRows(path.join(fixtures, 'object-conversion-x86.tsv'));

// 예외가 나는지 본다. (2026-11-08 WB04-E106)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

const hex = (bytes) => Buffer.from(bytes).toString('hex');
const SID = 50;

// 큰 게임 장치 없이 같은 원본 슬롯/타입/생성자 몸체를 조립하는 장면이다. (2026-11-08 WB04-E106)
function makeScene() {
    const scene = {
        pool: new SidPool(24000),
        types: [],
        factory: null,
        buried: { value: 112 },
        mutation: 0,
        trace: []
    };
    const raw = () => scene.pool.physical(SID);
    const emit = (text) => scene.trace.push(text);
    // 원문 부모의 진단 복귀를 기록하며 기대 출력이나 상태를 고치지 않는다.
    const diagnose = (line) => emit(`A,${line}`);
    scene.raw = raw;
    scene.diagnose = diagnose;
    // 원본과 같은 sentinel/생성자/foot 값을 새 입력으로 공급한다.
    scene.prepare = (old, next, state, mismatch) => {
        scene.trace = [];
        scene.mutation = 0;
        const slot = raw();
        // 모든 미사용 BYTE 도 독립 입력과 일치시킨다.
        for (let index = 0; index < slot.length; index += 1) slot[index] = (index * 7 + 9) & 0xff;
        put(slot, 0, old === 112 ? 0x50a748 : 0x502370);
        slot[10] = old;
        slot[11] = state;
        put(slot, 12, next, 2);
        put(slot, 14, floatToBits(100.5));
        put(slot, 18, floatToBits(122.25));
        scene.types = Array.from({ length: 188 }, () => ({ constructorAddress: 0, flags1: 0, flags2: 0, maxHitPoints: 0, zOrder: 0, footX: 0, footY: 0, name: '' }));
        // 원문의 현재 네 TYPE 입력이다.
        for (const kind of [112, 114, 116, 80]) { scene.types[kind].footX = 1; scene.types[kind].footY = 1; }
        scene.types[112].constructorAddress = 0x4625f0;
        scene.types[114].constructorAddress = 0x420e40;
        scene.types[116].constructorAddress = 0x420e60;
        if (old !== next) {
            scene.types[next].footX = (mismatch & 1) ? 2 : 1;
            scene.types[next].footY = (mismatch & 2) ? 3 : 1;
        }
        scene.factory = new SquidFactory(scene.pool, scene.types);
    };
    // 원문에서 분리한 실제 장치 효과만 제공하며 객체 전환은 제품 본체가 수행한다.
    scene.hooks = (bindWorld = false) => {
        const hooks = {
            unpop: (sid, flags) => {
                assert.equal(sid, SID);
                emit(`U,${flags}`);
                const slot = raw();
                slot[11] |= 4;
                if (scene.mutation === 1) put(slot, 12, 116, 2);
                else if (scene.mutation === 2) slot[10] = 116;
            },
            repop: (sid, flags) => {
                assert.equal(sid, SID);
                const slot = raw();
                emit(`P,${get(slot, 14)},${get(slot, 18)},${flags}`);
                slot[11] &= ~4 & 0xff;
            },
            retype: (sid, type) => scene.factory.retype(sid, type, diagnose),
            defaultFrame: (type) => {
                switch (type) {
                case 112: return 17;
                case 114: return 21;
                case 116: return 33;
                case 80: return 5;
                default: throw new Error('명시 원문 기본프레임 TYPE 범위');
                }
            },
            frame: (sid, value) => { assert.equal(sid, SID); put(raw(), 36, value); },
            qa: (sid, value) => { assert.equal(sid, SID); put(raw(), 12, value, 2); },
            base: (sid, code) => { assert.equal(sid, SID); emit(`B,${code}`); return 77; }
        };
        if (!bindWorld) return hooks;
        // 실제 Session 과 같은 Factory/필드 쓰기 바인더를 원문 기대값에 재생한다.
        return makeObjectConversionHooks(scene.pool, scene.factory, hooks, (expression, file, line) => {
            assert.ok(expression.length > 0 && file === '\\Ns\\O\\Squid.cpp');
            diagnose(line);
        });
    };
    // fixture 50바이트만 읽고 TYPE/VT/프레임/QA 밖 바이트까지 그대로 대조한다.
    scene.same = (expected) => expected.length === 100 && hex(raw()) === expected;
    return scene;
}

test('SquidRetype: 원본 10.78 128입력의 타입 변경 전체 슬롯·진단 행 순서·풀 free 개수·다른 슬롯 보존이 같다', { skip: !retypeRows && '원본 fixture 없음' }, () => {
    let checked = 0;
    // Editor 바깥 호출도 원문의 진단 복귀 의미를 유지한다.
    for (const row of retypeRows) {
        if (row[0] === 'old') continue;
        assert.equal(row.length, 6);
        const scene = makeScene();
        scene.prepare(Number(row[0]), Number(row[1]), Number(row[2]), Number(row[3]));
        const before = scene.pool.bytes();
        const free = scene.pool.freeCount;
        scene.factory.retype(SID, Number(row[1]), scene.diagnose);
        const label = `행 ${checked}: ${row.join(' ')}`;
        assert.ok(scene.same(row[4]), label);
        assert.equal(scene.trace.join(';'), row[5], label);
        assert.equal(scene.pool.freeCount, free, label);
        // 부모 슬롯 밖은 모두 보존된다.
        const after = scene.pool.bytes();
        before.set(after.subarray(SID * 50, SID * 50 + 50), SID * 50);
        assert.deepEqual(Buffer.from(before), Buffer.from(after), label);
        checked += 1;
    }
    assert.equal(checked, 128);
});

test('ObjectConversion: 원본 10.78 36입력의 비석/주문 전환 반환·전체 슬롯·U/P/B/A 순서를 직접 hooks 와 실제 Session 바인더로 재생해 같다', { skip: !conversionRows && '원본 fixture 없음' }, () => {
    for (const bound of [false, true]) {
        let checked = 0;
        // 원문이 처리하는 release 플래그도 따로 제거하지 않는다.
        for (const row of conversionRows) {
            if (row[0] === 'body') continue;
            assert.equal(row.length, 7);
            const buried = row[0] === 'Buried';
            const scene = makeScene();
            scene.prepare(buried ? 112 : 114, 114, Number(row[1]), 0);
            scene.mutation = Number(row[3]);
            const body = objectConversionBody(get(scene.raw(), 0));
            assert.equal(body, buried ? 0x462290 : 0x41e400);
            const conversion = new RawObjectConversion(scene.pool, scene.buried, scene.hooks(bound));
            const result = conversion.command(SID, Number(row[2]), body);
            const label = `${bound ? '바인더 ' : ''}행 ${checked}: ${row.join(' ')}`;
            assert.equal(result, Number(row[4]), label);
            assert.ok(scene.same(row[5]), label);
            assert.equal(scene.trace.join(';'), row[6], label);
            checked += 1;
        }
        assert.equal(checked, 36);
    }
    assert.equal(objectConversionBody(0), 0);
    assert.equal(objectConversionBody(0x509930), 0);
});

test('ObjectConversion: 실제 Session 바인더의 타입 변경 진단 128입력이 원문 전체 타입/생성자 결과를 보존한다', { skip: !retypeRows && '원본 fixture 없음' }, () => {
    let checked = 0;
    // void/발자국 진단 조합과 원문 생성자 없는 fallback 까지 비교한다.
    for (const row of retypeRows) {
        if (row[0] === 'old') continue;
        const scene = makeScene();
        scene.prepare(Number(row[0]), Number(row[1]), Number(row[2]), Number(row[3]));
        const free = scene.pool.freeCount;
        const hooks = scene.hooks(true);
        hooks.retype(SID, Number(row[1]));
        const label = `행 ${checked}: ${row.join(' ')}`;
        assert.ok(scene.same(row[4]), label);
        assert.equal(scene.trace.join(';'), row[5], label);
        assert.equal(scene.pool.freeCount, free, label);
        checked += 1;
    }
    assert.equal(checked, 128);
});

test('ObjectConversion: 선택하지 않은 경계는 요구하지 않고 도달한 누락은 앞선 쓰기를 보존하며 진단 행·풀 불일치를 거부한다', () => {
    const scene = makeScene();
    scene.prepare(112, 114, 4, 0);
    const parent = new RawObjectConversion(scene.pool, scene.buried, { base: () => 99 });
    assert.equal(parent.command(SID, 0x03000015, 0x462290), 99);
    assert.ok(throws(() => parent.command(SID, 0x030000ce, 0)));
    assert.ok(throws(() => parent.command(SID, 0x030000ce, 0x462290)));
    scene.types[116].constructorAddress = 0xffffffff;
    scene.factory = new SquidFactory(scene.pool, scene.types);
    assert.ok(throws(() => scene.factory.retype(SID, 116)));
    assert.equal(scene.raw()[10], 116);
    assert.equal(get(scene.raw(), 0), 0x50a748);
    scene.prepare(112, 114, 0, 0);
    assert.ok(throws(() => scene.factory.retype(SID, 114)));
    assert.equal(scene.raw()[10], 112);
    // 다른 풀의 Factory 와 진단 장치 미연결/행 범위/물리 필드 쓰기 폭을 확인한다.
    scene.prepare(112, 114, 4, 0);
    const foreign = new SquidFactory(new SidPool(24000), scene.types);
    assert.ok(throws(() => makeObjectConversionHooks(scene.pool, foreign, {}, null)));
    assert.ok(throws(() => new RawObjectConversion({ edition: 'cd1072' }, scene.buried, {})));
    const healthy = makeObjectConversionHooks(scene.pool, scene.factory, {}, null);
    healthy.retype(SID, 114);
    assert.equal(scene.raw()[10], 114);
    scene.prepare(112, 114, 0, 3);
    const before = Buffer.from(scene.raw());
    const missing = makeObjectConversionHooks(scene.pool, scene.factory, {}, null);
    assert.ok(throws(() => missing.retype(SID, 114)));
    assert.deepEqual(Buffer.from(scene.raw()), before);
    const lines = [];
    const hooks = makeObjectConversionHooks(scene.pool, scene.factory, {}, (expression, file, line) => {
        assert.equal(file, '\\Ns\\O\\Squid.cpp');
        assert.equal(expression, ['isVoid()', '::getType(typeNum)->footX == ::getType(newType)->footX', '::getType(typeNum)->footY == ::getType(newType)->footY'][line - 0x22d]);
        lines.push(line);
    });
    hooks.retype(SID, 114);
    assert.deepEqual(lines, [0x22d, 0x22e, 0x22f]);
    assert.equal(scene.raw()[10], 114);
    const expected = Buffer.from(scene.raw());
    put(expected, 36, 0x89abcdef);
    put(expected, 12, 0x12345678, 2);
    hooks.frame(SID, 0x89abcdef);
    hooks.qa(SID, 0x12345678);
    assert.deepEqual(Buffer.from(scene.raw()), expected);
});
