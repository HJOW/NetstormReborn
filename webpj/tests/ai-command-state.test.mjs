// AI 직접 명령 raw-ai-command-state.js(RawAiCommandState)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: ai-command-state-x86.tsv(10.78 전용) 1940행 — Money(CrankAiMoney 256)와 Param(SetAiParameter 1684)의 입력(AI 부착 마스크·packed·재조회 변경 여부) → 아홉 본체의 7DWORD(+220/228/230/234/24c/2d8/600) 결과,
//    본체가 쓴 DWORD 기록 순서(소유자:위치:폭:값). 나머지 모든 0xb08 본체 바이트와 Player 포인터는 보존되어야 한다.
//  - 계약: AiCommandState_LazyParentsCurrentPointersAndExplicitPhysicalFailures(선택하지 않은 하위 경계를 요구하지 않고 물리 폭 어긋남을 거부), AiCommandState_MoneyVisitsCurrentLaterBrainAndStopsAtFailure(후속 Player 변경을 현재 포인터로 읽음).
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E76: AI 직접 명령 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get } from './raw-support.mjs';
import { RawAiCommandState } from '../www/core/raw-ai-command-state.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E76)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'ai-command-state-x86.tsv'));

// 독립 입력 수, 본체 물리 폭, Player9 표 폭, 독립 fixture 가 기록한 7DWORD 의 본체 위치 순서이다. (2026-11-08 WB04-E76)
const ROWS = 1940;
const BODY = 0xb08;
const PLAYERS = 1620;
const COMMAND_OFFSETS = Object.freeze([0x220, 0x228, 0x230, 0x234, 0x24c, 0x2d8, 0x600]);

// 예외가 나는지 본다. (2026-11-08 WB04-E76)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 본체의 바이트 쓰기를 기록하는 Proxy 이다. 숫자 인덱스 쓰기만 [인덱스, 값]으로 모은다. (2026-11-08 WB04-E76)
function traced(bytes, log, name) {
    return new Proxy(bytes, {
        get: (target, key) => Reflect.get(target, key, target),
        set: (target, key, value) => {
            if (typeof key === 'string' && /^\d+$/.test(key)) log.push([name, Number(key), value]);
            return Reflect.set(target, key, value, target);
        }
    });
}

// 연속 네 BYTE 쓰기를 하나의 `소유자:위치:폭:값` DWORD 기록으로 묶는다. (2026-11-08 WB04-E76)
function dwordWrites(log) {
    const result = [];
    assert.equal(log.length % 4, 0);
    // 한 DWORD 는 같은 본체에 연속한 네 위치를 낮은 바이트부터 쓴다.
    for (let at = 0; at < log.length; at += 4) {
        const group = log.slice(at, at + 4);
        assert.ok(group.every((entry, index) => entry[0] === group[0][0] && entry[1] === group[0][1] + index));
        result.push(`${group[0][0]}:${group[0][1]}:4:${group.reduce((sum, entry, index) => sum + entry[2] * 2 ** (8 * index), 0)}`);
    }
    return result.length === 0 ? '-' : result.join(';');
}

test('AiCommandState: 원본 10.78 1940입력의 7DWORD 변경·재조회·전체 본체와 Player 포인터 보존이 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    let count = 0;
    // 기대값은 원본 관측이며 제품 출력에서 만들지 않는다.
    for (const row of rows) {
        assert.equal(row.length, 6);
        const players = new Uint8Array(PLAYERS);
        const log = [];
        const tokens = [];
        const bodies = new Map();
        // 아홉 실제 본체는 owner0 보존과 selector0 재조회 대상을 포함한다.
        for (let owner = 0; owner < 9; owner += 1) {
            tokens[owner] = 0x4f000000 + owner * 0x1000;
            const body = new Uint8Array(BODY).fill(0xa0 + owner);
            bodies.set(tokens[owner], traced(body, log, owner));
            if (!owner || (Number(row[1]) & (1 << (owner - 1)))) put(players, owner * 180 + 0xb0, tokens[owner]);
        }
        const packed = Number(row[2]);
        const selected = packed % 8;
        let flipped = false;
        let bodyCalls = 0;
        const hooks = {
            players: () => players,
            body: (token) => {
                bodyCalls += 1;
                if (row[3] === '1' && !flipped) {
                    put(players, selected * 180 + 0xb0, tokens[8]);
                    flipped = true;
                }
                return bodies.get(token);
            }
        };
        const state = new RawAiCommandState(hooks);
        if (row[0] === 'Money') state.crankAiMoney();
        else state.setAiParameter(packed);
        const owners = row[4].split('/');
        assert.equal(owners.length, 9);
        // 7DWORD 이외의 실제 0xb08 바이트도 보존해야 한다.
        for (let owner = 0; owner < owners.length; owner += 1) {
            const fields = owners[owner].split(',');
            assert.equal(fields.length, 7);
            const expected = new Uint8Array(BODY).fill(0xa0 + owner);
            // 기대 변경은 원본 fixture 에서 읽은 DWORD 뿐이다.
            fields.forEach((value, field) => put(expected, COMMAND_OFFSETS[field], Number(value)));
            const actual = Buffer.from(bodies.get(tokens[owner]).buffer, 0, BODY);
            assert.ok(Buffer.from(expected).equals(actual), `행 ${count}: 소유자 ${owner} 본체`);
            const attached = !owner || (Number(row[1]) & (1 << (owner - 1))) !== 0;
            const expectedToken = flipped && owner === selected ? tokens[8] : attached ? tokens[owner] : 0;
            assert.equal(get(players, owner * 180 + 0xb0), expectedToken, `행 ${count}: 소유자 ${owner} 포인터`);
        }
        // 본체가 받은 DWORD 기록 순서(소유자:위치:폭:값)도 원본 관측과 같아야 한다.
        assert.equal(dwordWrites(log), row[5], `행 ${count}: 쓰기 순서`);
        if (row[3] === '1') assert.ok(flipped && bodyCalls === 2, `행 ${count}: 재조회`);
        count += 1;
    }
    assert.equal(count, ROWS);
});

test('AiCommandState: 소유자 0·본체 없음·미지원 선택자는 선택하지 않은 경계를 요구하지 않고 물리 폭 어긋남을 거부한다', () => {
    new RawAiCommandState({}).setAiParameter(0);
    new RawAiCommandState({}).setAiParameter(8);
    const players = new Uint8Array(PLAYERS);
    let reads = 0;
    const hooks = { players: () => { reads += 1; return players; } };
    const state = new RawAiCommandState(hooks);
    state.crankAiMoney();
    assert.equal(reads, 8);
    state.setAiParameter(1);
    assert.equal(reads, 9);
    put(players, 180 + 0xb0, 0xdeadbeef);
    state.setAiParameter(1 | (255 << 4));
    assert.equal(reads, 10);
    assert.ok(throws(() => state.setAiParameter(1)));
    const shortBody = new Uint8Array(8);
    hooks.body = () => shortBody;
    assert.ok(throws(() => state.crankAiMoney()));
    hooks.players = () => new Uint8Array(0);
    assert.ok(throws(() => state.setAiParameter(1)));
    assert.ok(throws(() => new RawAiCommandState({}).crankAiMoney()));
});

test('AiCommandState: 앞선 본체 해석이 바꾼 후속 Player 를 다음 money 방문이 현재 포인터로 읽고 실패에서 멈춘다', () => {
    const players = new Uint8Array(PLAYERS);
    const first = 0x4f000000;
    const second = 0x4f001000;
    const bodies = new Map([[first, new Uint8Array(BODY).fill(0x7b)], [second, new Uint8Array(BODY).fill(0x7b)]]);
    put(players, 180 + 0xb0, first);
    const hooks = {
        players: () => players,
        body: (token) => {
            if (token === first) put(players, 2 * 180 + 0xb0, second);
            return bodies.get(token);
        }
    };
    const state = new RawAiCommandState(hooks);
    state.crankAiMoney();
    assert.ok(get(bodies.get(first), 0x2d8) === 30000 && get(bodies.get(second), 0x2d8) === 30000);
    put(bodies.get(first), 0x2d8, 17);
    put(bodies.get(second), 0x2d8, 19);
    hooks.body = (token) => {
        if (token === second) throw new Error('명시 두 번째 현재 본체 실패');
        return bodies.get(token);
    };
    assert.ok(throws(() => state.crankAiMoney()));
    assert.ok(get(bodies.get(first), 0x2d8) === 30000 && get(bodies.get(second), 0x2d8) === 19);
});
