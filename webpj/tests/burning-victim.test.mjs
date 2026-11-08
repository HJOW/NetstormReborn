// 연소 피해 대상 앞단 raw-burning-victim.js(runBurningVictimParent·makeBurningVictimHooks)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: burning-victim-parent-x86.tsv(10.78 전용) 504행(세 부모 1008관측 중 고유 행) — 입력 8열(부모 종류·현재 SID·checking·robust 결과 비트마스크·가상 표·HP·진단 복귀 여부·변이 종류) → 결과(empty/body/assertion)·
//    checking·현재 SID·선택 SID·호출 순서(R robust / V 가상 표 / H HP / D 진단 조건·파일·줄 hex).
//  - 계약: BurningVictimParent_SelectedHooksAndDwordLifetime(미선택 장치 없이 조기 반환하고 checking 상위 비트 보존), 그리고 makeBurningVictimHooks 의 실제 풀 연결(가상 표·Dais/일반 HP·독립 진단)을 새로 확인한다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E76: 연소 피해 대상 앞단 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { BurningVictimEntry, runBurningVictimParent, makeBurningVictimHooks } from '../www/core/raw-burning-victim.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E76)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'burning-victim-parent-x86.tsv'));

// 독립 입력 수이다. (2026-11-08 WB04-E76)
const ROWS = 504;

// 예외가 나는지 본다. (2026-11-08 WB04-E76)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 조건/파일 문자열의 원문 바이트를 소문자 hex 로 만든다. (2026-11-08 WB04-E76)
const textHex = (text) => Buffer.from(text, 'latin1').toString('hex');

test('BurningVictimParent: 원본 10.78 504입력의 현재 SID·선택 슬롯·진단 복귀 순서가 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    let count = 0;
    // 최초 관측에서 현재 입력만 읽고 제품 결과로 기대값을 만들지 않는다.
    for (const row of rows) {
        assert.equal(row.length, 6);
        const p = row[0].split(',').map(Number);
        assert.equal(p.length, 8);
        const [entry, start, initialChecking, mask, vtable, hp, resume, mutate] = p;
        let current = start;
        let checking = initialChecking;
        let calls = 0;
        const events = [];
        let outcome = 'empty';
        let selected = '-';
        const hooks = {
            robust: (sid, live) => {
                assert.ok(live);
                const result = ((mask >> Math.min(calls, 1)) & 1) !== 0;
                events.push(`R:${sid}:${result ? 1 : 0}`);
                calls += 1;
                if (mutate === 1 && calls === 1) { current = 1; checking = 0; }
                return result;
            },
            vtable: (sid) => { events.push(`V:${sid}:${vtable}`); return vtable !== 0; },
            hasHitPoints: (sid) => {
                events.push(`H:${sid}:${hp}`);
                if (mutate === 2) { current = 1; checking = 0; }
                return hp !== 0;
            },
            assertion: (condition, source, line) => {
                events.push(`D:${textHex(condition)}:${textHex(source)}:${line}`);
                if (!resume) throw new Error('assertion');
            }
        };
        try {
            const victim = runBurningVictimParent(() => current, entry, () => checking, hooks);
            if (victim !== null) { outcome = 'body'; selected = String(victim); }
        } catch (failure) {
            if (failure.message !== 'assertion') throw failure;
            outcome = failure.message;
        }
        const label = `행 ${count}: 입력 ${row[0]}`;
        assert.equal(outcome, row[1], label);
        assert.equal(String(checking), row[2], label);
        assert.equal(String(current), row[3], label);
        assert.equal(selected, row[4], label);
        assert.equal(events.join(';'), row[5], label);
        count += 1;
    }
    assert.equal(count, ROWS);
});

test('BurningVictimParent: 미선택 가상 표/HP/진단 없이 조기 반환하고 checking 상위 비트와 두 번째 조회를 보존한다', () => {
    let current = 50;
    let checking = 0;
    let calls = 0;
    const hooks = { robust: (sid, live) => { assert.ok(sid === current && live); calls += 1; return false; } };
    assert.equal(runBurningVictimParent(() => current, BurningVictimEntry.Capture, () => checking, hooks), null);
    assert.equal(calls, 1);
    checking = 0x80000000;
    assert.ok(throws(() => runBurningVictimParent(() => current, BurningVictimEntry.Sources, () => checking, hooks)));
    assert.ok(calls === 2 && checking === 0x80000000);
    hooks.assertion = (condition, source, line) => {
        assert.ok(condition === 'isSidRobust(burningSid)' && source === '.\\Burning.cpp' && line === 0x11f);
        checking = 0;
        current = 0;
    };
    // 진단 복귀 뒤에도 두 번째 실제 조회를 생략하지 않는다.
    assert.equal(runBurningVictimParent(() => current, BurningVictimEntry.Frame, () => checking, hooks), null);
    assert.ok(calls === 4 && current === 0 && checking === 0);
    assert.ok(throws(() => runBurningVictimParent(50, BurningVictimEntry.Frame, () => 0, hooks)));
    assert.ok(throws(() => runBurningVictimParent(() => 50, 7, () => 0, hooks)));
});

test('makeBurningVictimHooks: 실제 풀의 가상 표·Dais/일반 HP 와 독립 진단 장치를 선택 지점에서만 요구한다', () => {
    const pool = new SidPool(24000);
    for (let sid = 5; sid <= 52; sid += 1) assert.equal(pool.allocate(2), sid);
    const types = Array.from({ length: 188 }, () => ({ cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, maxHitPoints: 0 }));
    types[10].flags1 = 0x10;
    types[11].flags2 = 0x400000;
    let checking = 0;
    const hooks = makeBurningVictimHooks(pool, types, { checking: () => checking });
    const raw = pool.allocated(50);
    raw[10] = 10;
    assert.ok(!hooks.vtable(50));
    put(raw, 0, 0x01000000);
    assert.ok(hooks.vtable(50) && hooks.hasHitPoints(50));
    // Dais 는 extra(+40) bit0 이고 일반 flags1 bit4 는 보지 않는다.
    raw[10] = 11;
    types[11].flags1 = 0x10;
    assert.ok(!hooks.hasHitPoints(50));
    raw[40] = 3;
    assert.ok(hooks.hasHitPoints(50));
    raw[10] = 200;
    assert.ok(throws(() => hooks.hasHitPoints(50)));
    // 독립 robust 는 번호 판정만 쓰고 checking 이 비영이면 미연결 상세 장치를 거부한다.
    assert.ok(hooks.robust(50, true) && !hooks.robust(0, true) && !hooks.robust(65535, true) && hooks.robust(50, false) === true && !hooks.robust(0, false));
    checking = 1;
    assert.ok(throws(() => hooks.robust(50, true)));
    assert.ok(throws(() => hooks.assertion('조건', '.\\Burning.cpp', 1)));
    const seen = [];
    const custom = makeBurningVictimHooks(pool, types, {
        robust: (sid, live) => sid === 77 && live,
        assertion: (condition, source, line) => seen.push(`${condition}|${source}|${line}`)
    });
    assert.ok(custom.robust(77, true) && !custom.robust(50, true));
    custom.assertion('a', 'b', 3);
    assert.deepEqual(seen, ['a|b|3']);
    assert.ok(throws(() => makeBurningVictimHooks({ edition: 'cd1072' }, types, {})));
});
