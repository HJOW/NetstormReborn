// 객체 방어 raw-squid-defense.js(rawSquidDefaultDefense·makeRawSquidDefenseHooks)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: squiddefense-x86.tsv 의 10.78(originals) 24행(원본 PE 의 5바이트 xor/ret 4 몸체가 SID·공격 SID 와 무관하게 0 을 반환하고 전체 raw 가 변하지 않음).
//    CD 10.72(24행)는 웹 범위(10.78) 밖이다.
//  - 계약: SquidDefense_ActualFactoryTablesAndFallback(실제 Factory 가 만든 캠페인71/128/사제/anim 가상 표만 분배하고 다른 파생 callback 보존)·
//    SquidDefense_MissingOtherDerivedRejected(다른 파생·빈 표·범위 밖 SID 를 기본 false 로 우회하지 않음).
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E58: 객체 방어 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidFactory } from '../www/core/squid-factory.js';
import { typeConstructorAddress } from '../www/core/squid-constructors.js';
import { rawSquidDefaultDefense, makeRawSquidDefenseHooks, SQUID_DEFENSE_ENTRIES } from '../www/core/raw-squid-defense.js';

// 원본 관측 10.78 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E58)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'squiddefense-x86.tsv'))?.filter((row) => row[0] === 'originals') ?? null;

// 예외가 나는지 본다. (2026-11-08 WB04-E58)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 바이트 배열을 소문자 hex 로 만든다. (2026-11-08 WB04-E58)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

test('SquidDefense: 원본 10.78 기본 방어 몸체는 인자와 무관하게 false 를 반환하고 raw 를 바꾸지 않는다(24행)', { skip: !rows && '원본 fixture 없음' }, () => {
    const pool = new SidPool(24000, { server: false });
    // 원본과 같은 유효 victim 을 확보한다(5..127).
    for (let id = 5; id <= 127; id += 1) assert.equal(pool.allocate(2), id);
    let count = 0;
    // 저장된 각각의 정상 반환을 한 번씩 대조한다.
    for (const row of rows) {
        const sid = Number(row[1]);
        const raw = pool.physical(sid);
        // 관측이 보존한 padding 패턴을 그대로 입력한다.
        for (let index = 0; index < raw.length; index += 1) raw[index] = (index * 11 + sid) & 0xff;
        const before = hex(raw);
        assert.equal(Number(rawSquidDefaultDefense(sid, Number(row[2]))), Number(row[3]));
        assert.equal(hex(raw), before);
        assert.equal(hex(raw), row[4]);
        count += 1;
    }
    assert.equal(count, 24);
});

test('SquidDefense: 실제 Factory 가 만든 캠페인71/128/사제/anim 가상 표만 분배하고 다른 파생 callback 을 보존한다', () => {
    const pool = new SidPool(24000, { server: true });
    const types = Array.from({ length: 188 }, () => ({ cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0 }));
    // 실제 생성자 주소는 타입 초기화 표의 값을 쓴다.
    for (const type of [71, 128, 158, 160]) types[type].constructorAddress = typeConstructorAddress(type);
    const factory = new SquidFactory(pool, types);
    let fallbackCalls = 0;
    const hooks = makeRawSquidDefenseHooks(pool, SQUID_DEFENSE_ENTRIES, { rejectDamage: () => { fallbackCalls += 1; return true; } });
    let last = 0;
    // 각 원본 타입은 현재 생성한 raw 가상 표에 따라 기본 방어 false 를 소비한다.
    for (const type of [71, 128, 158, 160]) {
        last = factory.create(type, 2);
        assert.equal(hooks.rejectDamage(last, 65535), false);
        assert.equal(fallbackCalls, 0);
    }
    put(pool.allocated(last), 0, 0xdead1234);
    assert.equal(hooks.rejectDamage(last, 60), true);
    assert.equal(fallbackCalls, 1);
});

test('SquidDefense: 다른 파생·빈 표·범위 밖 표적을 기본 false 로 우회하지 않는다', () => {
    const pool = new SidPool(24000, { server: false });
    assert.equal(pool.allocate(2), 5);
    const hooks = makeRawSquidDefenseHooks(pool, SQUID_DEFENSE_ENTRIES, {});
    assert.ok(throws(() => hooks.rejectDamage(5, 60)), '표에 없는 가상 표는 fallback 없이 거부');
    assert.ok(throws(() => makeRawSquidDefenseHooks(pool, [], {})));
    assert.ok(throws(() => hooks.rejectDamage(0, 60)));
    assert.ok(throws(() => hooks.rejectDamage(24000, 60)));
});
