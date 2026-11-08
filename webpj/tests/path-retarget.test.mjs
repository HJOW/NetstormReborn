// 경로 대상 재확인 raw-path-retarget.js(RawPathRetarget)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: path-retarget-x86.tsv 4801행(두 정밀도 열이 같음) — 입력 12 DWORD(귀환/간헐천 대상 SID·두 대상 상태·현재 부모·첫 조회/둘째 조회·두 후보·삭제 여부·변이 11종·너겟 타입)
//    → 외부 효과 순서(F 내용물 조회/C·Y 후보 선택/R·G 이동 시작), 상태(삭제 여부·삭제 직전 마지막 두 대상·부모 DWORD·너겟 타입·조회 횟수·앞 0x10000바이트 Adler-32).
//  - 계약: 선택하지 않은 경계는 요구하지 않고, 실제 죽은 대상의 미연결은 거부한다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E103: 경로 대상 재확인 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get, adler32 } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { RawPathRuntime } from '../www/core/raw-path-runtime.js';
import { RawPathRetarget } from '../www/core/raw-path-retarget.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E103)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'path-retarget-x86.tsv'));

// 예외가 나는지 본다. (2026-11-08 WB04-E103)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 큰 내용물/선택/새 이동은 native 도구와 동일한 명시 효과 경계로 한 행을 재생한다. (2026-11-08 WB04-E103)
function replay(pool, c) {
    const raw = new Uint8Array(pool.physical(0).buffer, 0, 0x10000);
    raw.fill(0x2a);
    // 부모 가상 주소는 비교 대상인 원래 raw 입력이며 실제 함수 포인터가 아니다.
    for (const sid of [50, 51, 52]) {
        put(raw, sid * 50, 0x12200000);
        put(raw, sid * 50 + 10, 133, 1);
        put(raw, sid * 50 + 11, 0, 1);
        put(raw, sid * 50 + 40, 0, 1);
    }
    put(raw, c[0] * 50 + 11, c[2], 1);
    put(raw, c[1] * 50 + 11, c[3], 1);
    let runtime = new RawPathRuntime();
    runtime.setAttachment(5, 50);
    runtime.setTargets(c[0], c[1]);
    put(runtime.bytes, 8, c[4]);
    let nuggetType = c[11];
    let finds = 0;
    const events = [];
    let snapshot = [0, 0, 0];
    let deleted = false;
    // 실제 효과의 인자를 원본 로그와 같은 문자열로 기록한다.
    const event = (name, args) => events.push(args.reduce((text, arg) => `${text}:${arg >>> 0}`, name));
    // 첫 조회/선택/이동의 외부 효과는 결과를 계산하지 않고 입력 모드만 적용한다.
    const mutate = (phase) => {
        const mode = c[10];
        const bytes = runtime.bytes;
        if (phase === 'F' && finds === 1) {
            if (mode === 1) put(bytes, 0x186cc, 0);
            if (mode === 2) put(bytes, 0x186cc, 62);
            if (mode === 3) put(bytes, 0x186d0, 0);
            if (mode === 4) { put(bytes, 8, 52); nuggetType = 155; }
            if (mode === 8) put(raw, c[0] * 50 + 11, 0, 1);
            if (mode === 9) put(raw, c[1] * 50 + 11, 0, 1);
            if (mode === 10) nuggetType = 155;
            if (mode === 11) put(bytes, 0x186d0, 62);
        }
        if (phase === 'C' && mode === 5) { put(bytes, 0x186cc, 62); put(bytes, 0x186d0, 0); }
        if (phase === 'Y' && mode === 6) { put(bytes, 0x186d0, 62); put(bytes, 0x186cc, 0); }
        if (phase === 'G' && mode === 7) { put(bytes, 0x186cc, 64); put(bytes, 0x186d0, 65); }
    };
    // 원본의 go 경계와 같이 호출 중 Runtime 을 실제 해제한다. update 는 이후 이를 읽으면 안 된다.
    const go = (name, parent, target) => {
        event(name, [parent, target, runtime.targetA, runtime.targetB, get(runtime.bytes, 8)]);
        mutate('G');
        snapshot = [runtime.targetA, runtime.targetB, get(runtime.bytes, 8)];
        if (c[9]) { deleted = true; runtime = null; }
    };
    const hooks = {
        find: (parent, mask, type, index) => { finds += 1; event('F', [parent, mask, type, index]); mutate('F'); return finds === 1 ? c[5] : c[6]; },
        chooseReturn: (parent) => { event('C', [parent]); mutate('C'); return c[7]; },
        chooseGeyser: (parent) => { event('Y', [parent]); mutate('Y'); return c[8]; },
        goReturn: (parent, target) => go('R', parent, target),
        goGeyser: (parent, target) => go('G', parent, target)
    };
    const retarget = new RawPathRetarget(pool, () => nuggetType, hooks);
    assert.equal(retarget.pool, pool);
    retarget.update(runtime, 50);
    if (!deleted) snapshot = [runtime.targetA, runtime.targetB, get(runtime.bytes, 8)];
    const effects = events.length === 0 ? '-' : events.join(';');
    const state = `${deleted ? 1 : 0},${snapshot[0]},${snapshot[1]},${snapshot[2]},${nuggetType},${finds},${adler32(raw)}`;
    return { effects, state };
}

test('PathRetarget: 원본 10.78 4801입력의 조회/선택/이동 효과 순서·삭제 직전 대상·부모·너겟 타입·조회 횟수·raw Adler 가 두 정밀도 열과 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    const pool = new SidPool(24000);
    let count = 0;
    // 원본의 53/64비트 정밀도 결과는 같고 JS 는 같은 정수 분기를 한 번 실행한다.
    for (const fields of rows) {
        assert.equal(fields.length, 5);
        const input = fields[0].split(',').map((value) => Number(BigInt.asUintN(32, BigInt(value))));
        assert.equal(input.length, 12);
        const { effects, state } = replay(pool, input);
        assert.equal(effects, fields[1], `행 ${count}: 입력 ${fields[0]}`);
        assert.equal(state, fields[2], `행 ${count}: 입력 ${fields[0]}`);
        assert.ok(fields[1] === fields[3] && fields[2] === fields[4]);
        count += 1;
    }
    assert.equal(count, 4801);
});

test('PathRetarget: 선택하지 않은 경계는 요구하지 않고 실제 죽은 대상의 미연결과 CD 판본은 거부한다', () => {
    const pool = new SidPool(24000);
    let nuggetType = 154;
    const retarget = new RawPathRetarget(pool, { get value() { return nuggetType; } }, {});
    const runtime = new RawPathRuntime();
    runtime.setTargets(60, 61);
    put(runtime.bytes, 8, 0xffffffff);
    pool.physical(60)[11] = 1;
    pool.physical(61)[11] = 1;
    assert.ok(!throws(() => retarget.update(runtime, 50)));
    pool.physical(61)[11] = 2;
    assert.ok(throws(() => retarget.update(runtime, 50)));
    assert.ok(throws(() => new RawPathRetarget({ edition: 'cd1072' }, () => nuggetType, {})));
});
