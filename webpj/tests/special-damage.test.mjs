// 신전/윈드 타워 특수 피해 raw-special-damage.js(rawVortexDamage·rawWindTowerDefense)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: special-damage-x86.tsv 432행 — 연산 0 신전 피해(건설 중 여부·건설 Form·피해량 DWORD·소유자·일반 피해 반환)와 연산 1 윈드 타워 방어(공격자 genus·TYPE·좌표 비트·발자국·피해자 방향 글자) → 반환과 외부 효과 순서(F 건설 조회/C 끝 시각/R 표시 갱신/H 일반 피해).
//    wind-tower-zero-x86.tsv 216행 — 예약 피해의 공격자 SID 0 도 원본처럼 현재 풀 0 번 슬롯의 TYPE/좌표를 읽는 윈드 타워 방어 반환.
//  - 계약: 무효 SID/판본·TYPE 범위·미연결 하위를 해당 분기를 쓰는 시점에 거부한다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E104: 특수 피해 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, bitsToFloat, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { rawVortexDamage, rawWindTowerDefense } from '../www/core/raw-special-damage.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E104)
const fixtures = path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures');
const damageRows = readRows(path.join(fixtures, 'special-damage-x86.tsv'));
const zeroRows = readRows(path.join(fixtures, 'wind-tower-zero-x86.tsv'));

// 예외가 나는지 본다. (2026-11-08 WB04-E104)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

const numbers = (text) => text.split(',').map((value) => Number(BigInt.asUintN(32, BigInt(value))));
const freshTypes = () => Array.from({ length: 188 }, () => ({ flags1: 0, flags2: 0, footX: 0, footY: 0 }));

test('SpecialDamage: 원본 10.78 432입력의 신전 분배/윈드 타워 방어 반환과 외부 효과 순서가 같다', { skip: !damageRows && '원본 fixture 없음' }, () => {
    const pool = new SidPool(24000);
    const types = freshTypes();
    const victim = 15000;
    const attacker = 15001;
    let count = 0;
    // 실제 입력마다 같은 raw 상태를 다시 설치하고 실제 반환/순서를 대조한다.
    for (const row of damageRows) {
        assert.equal(row.length, 3);
        const input = numbers(row[0]);
        const target = pool.physical(victim);
        const source = pool.physical(attacker);
        target.fill(0);
        source.fill(0);
        target[10] = 142;
        source[10] = 128;
        put(target, 14, floatToBits(50));
        put(target, 18, floatToBits(50));
        types[142].footX = types[142].footY = 2;
        const events = [];
        let actual;
        if (input[0] === 0) {
            target[40] = input[1];
            const hooks = {
                constructionType: () => 30,
                find: (sid, type, index, remove) => { events.push(`F:${sid},${type},${index},${remove ? 1 : 0}`); return input[2]; },
                resetEnd: (sid, amount) => events.push(`C:${sid},${amount}`),
                refresh: (sid) => events.push(`R:${sid}`),
                general: (sid, amount, owner, firing) => { events.push(`H:${sid},${amount >>> 0},${owner},${firing}`); return input[5] !== 0; }
            };
            actual = rawVortexDamage(pool, victim, input[3] | 0, input[4], attacker, hooks);
        } else {
            source[10] = input[2];
            put(source, 14, input[4]);
            put(source, 18, input[5]);
            types[input[2]].flags2 = input[1];
            types[input[2]].footX = input[6] | 0;
            types[input[2]].footY = input[7] | 0;
            // 타입별 표는 같은 개수를 유지하고 실제 방어자의 첫 side BYTE 만 설치한다.
            const frames = Array.from({ length: 188 }, (_, type) => ({ codes: [{ side: type === 142 ? input[3] : 0, variant: 0, flags: 0 }] }));
            actual = rawWindTowerDefense(pool, types, frames, victim, attacker);
        }
        const label = `행 ${count}: 입력 ${row[0]}`;
        assert.equal(Number(actual), Number(row[1]), label);
        assert.equal(events.join(';'), row[2], label);
        count += 1;
    }
    assert.equal(count, 432);
});

test('SpecialDamage: 예약 피해의 공격자 0 도 현재 풀 0 번 슬롯을 읽는 윈드 타워 방어 216입력이 같다', { skip: !zeroRows && '원본 fixture 없음' }, () => {
    let count = 0;
    // 새로 관측한 0번 슬롯의 타입/기하/반환을 그대로 대조한다.
    for (const row of zeroRows) {
        assert.equal(row.length, 3);
        const a = numbers(row[0]);
        const pool = new SidPool(24000);
        const types = freshTypes();
        const victim = 15000;
        const target = pool.physical(victim);
        const source = pool.physical(0);
        target.fill(0);
        source.fill(0);
        target[10] = 142;
        source[10] = a[2];
        put(target, 14, floatToBits(50));
        put(target, 18, floatToBits(50));
        put(source, 14, a[4]);
        put(source, 18, a[5]);
        types[142].footX = types[142].footY = 2;
        types[a[2]].flags2 = a[1];
        types[a[2]].footX = a[6] | 0;
        types[a[2]].footY = a[7] | 0;
        const frames = Array.from({ length: 188 }, () => ({ codes: [] }));
        frames[142] = { codes: [{ side: a[3], variant: 0, flags: 0 }] };
        const actual = rawWindTowerDefense(pool, types, frames, victim, 0);
        assert.equal(actual, Number(row[1]) !== 0, `행 ${count}: 입력 ${row[0]}`);
        assert.equal(row[2], '');
        count += 1;
    }
    assert.equal(count, 216);
});

test('SpecialDamage: 무효 SID/판본·TYPE 범위·미연결 하위는 해당 분기를 쓰는 시점에 거부한다', () => {
    const pool = new SidPool(24000);
    const types = freshTypes();
    pool.physical(15000)[40] = 1;
    assert.ok(throws(() => rawVortexDamage(pool, 0, 1, 0, 0, {})));
    assert.ok(throws(() => rawVortexDamage({ edition: 'cd1072' }, 15000, 1, 0, 0, {})));
    assert.ok(throws(() => rawVortexDamage(pool, 15000, 1, 0, 0, {})));
    assert.ok(throws(() => rawVortexDamage(pool, 15000, 1, 0, 0, { constructionType: () => 30, find: () => 15001 })));
    pool.physical(15000)[40] = 0;
    assert.ok(throws(() => rawVortexDamage(pool, 15000, 1, 0, 0, {})));
    assert.equal(rawVortexDamage(pool, 15000, -7, 1, 2, { general: (sid, amount) => amount === -7 }), true);
    // 윈드 타워: 공격자 SID 범위·TYPE 범위·제외 genus 의 방어 면제(피해자 미조회)를 확인한다.
    const frames = Array.from({ length: 188 }, () => ({ codes: [] }));
    assert.ok(throws(() => rawWindTowerDefense(pool, types, frames, 15000, 24000)));
    pool.physical(15001)[10] = 200;
    assert.ok(throws(() => rawWindTowerDefense(pool, types, frames, 15000, 15001)));
    pool.physical(15001)[10] = 128;
    types[128].flags2 = 0x100000;
    assert.equal(rawWindTowerDefense(pool, types, frames, 0, 15001), false);
    types[128].flags2 = 0;
    assert.equal(rawWindTowerDefense(pool, types, frames, 15000, 15001, 128), false);
    assert.ok(throws(() => rawWindTowerDefense(pool, types, frames, 0, 15001)));
    assert.ok(throws(() => rawWindTowerDefense(pool, types, frames, 15000, 15001)));
    assert.equal(bitsToFloat(floatToBits(50)), 50);
});
