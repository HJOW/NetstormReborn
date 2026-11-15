// 하드너 주문 대상 본문 raw-bomb-hardener.js(RawBombHardener 자격·강화 프레임 선택·플레어)를 원본 10.78 관측 21행과 cpppj 시험 계약으로 대조한다(WB04-E551).
//  - bomb-hardener-x86.tsv 21행: 자격 마스크(E)와 적용(A)의 후보 상한·난수 상태·QA 제외·검색 실패 플레어 순서(S=표시 갱신 두 번째, F=플레어)·프레임·난수 상태.
//  - 계약: 판본/표 크기/풀 불일치 거부, 플레어 미연결 시 이미 수행한 프레임 지정과 난수 소비를 되돌리지 않고 거부.
// 실제 게임·브라우저 실행이 아니다. (2026-11-15 WB04-E551)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { GameRandom } from '../www/core/random.js';
import { SquidFrame } from '../www/core/squid-frame.js';
import { RawBombHardener } from '../www/core/raw-bomb-hardener.js';

// 원본 관측 fixture 이다. 없으면 해당 검사를 건너뛴다. (2026-11-15 WB04-E551)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'bomb-hardener-x86.tsv'));

// 프레임 지정기의 선택 안 한 가상 효과는 호출되면 실패한다(검사 전용 금지 경계). (2026-11-15 WB04-E551)
const forbidden = () => { throw new Error('하드너 시험에서 호출되면 안 되는 효과'); };

// 새 풀·부모(119)·대상(94)·타입/프레임 표·난수와 표시 갱신 추적을 가진 장면을 만든다. (2026-11-15 WB04-E551)
function scene({ genus, seed, qa, current, codes }) {
    const pool = new SidPool(24000, { server: false });
    const parent = pool.allocate(2);
    const target = pool.allocate(2);
    const source = pool.allocated(parent);
    const raw = pool.allocated(target);
    source.fill(0);
    raw.fill(0);
    source[10] = 119;
    raw[10] = 94;
    put(source, 12, qa ? target : 0, 2);
    put(raw, 36, current);
    const types = Array.from({ length: 188 }, () => ({ flags2: 0 }));
    types[94].flags2 = genus;
    const frames = Array.from({ length: 188 }, () => ({ codes: [] }));
    frames[94] = { codes };
    const random = new GameRandom(seed);
    const state = { trace: '', updates: 0 };
    const hooks = {
        frameSize: forbidden,
        unpop: forbidden,
        pop: forbidden,
        // 실제 4acee0 의 두 갱신을 원문 장치 경계 S 한 번으로 정규화한다.
        update: (sid, flags) => { assert.ok(sid === target && flags === 0); state.updates += 1; if (state.updates === 2) state.trace += 'S'; }
    };
    const setter = new SquidFrame(pool, types, hooks);
    const flare = (sid, n, event) => {
        assert.ok(sid === target && n === 1 && event === 2 && (state.updates === 0 || state.updates === 2));
        state.trace += 'F';
    };
    return { pool, parent, target, raw, types, frames, random, state, setter, flare };
}

// fixture 의 4바이트 코드(side, variant, number, flags)를 순서대로 읽는다. (2026-11-15 WB04-E551)
function decodeCodes(text) {
    const codes = [];
    for (let start = 0; start < text.length; start += 8) {
        const byte = (index) => Number.parseInt(text.slice(start + index * 2, start + index * 2 + 2), 16);
        codes.push({ side: byte(0), variant: byte(1), number: byte(2), flags: byte(3) });
    }
    return codes;
}

test('RawBombHardener: 원본 10.78 자격·강화 프레임 선택·플레어 21행의 결과·프레임·난수 상태·효과 순서가 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    let count = 0;
    // 매 행은 같은 입력에서 새 실제 풀/현재 TYPE/프레임/게임 난수로 시작한다.
    for (const row of rows.filter((fields) => fields[0] !== 'mode')) {
        const s = scene({ genus: Number(row[1]), seed: Number(row[2]), qa: Number(row[3]), current: Number(row[4]) >>> 0, codes: decodeCodes(row[5]) });
        const body = new RawBombHardener(s.pool, s.types, s.frames, s.random, s.setter, s.flare);
        if (row[0] === 'E') {
            // 자격에 상태/소유자 검사를 추가하지 않는다.
            s.raw[11] = 3;
            s.raw[34] = 255;
            assert.equal(body.mayTarget(s.target), Number(row[6]), `행 ${count} 자격`);
        } else body.apply(s.parent, s.target);
        assert.equal(get(s.raw, 36), Number(row[7]), `행 ${count} 프레임`);
        assert.equal(s.random.state, Number(row[8]), `행 ${count} 난수`);
        assert.equal(s.state.trace || '-', row[9], `행 ${count} 효과`);
        count += 1;
    }
    assert.equal(count, 21);
});

test('RawBombHardener: 판본/표 크기/풀 불일치 거부와 플레어 미연결 시 앞선 효과 보존', () => {
    const s = scene({ genus: 4, seed: 55, qa: 0, current: 0, codes: [{ side: 65, variant: 80, number: 0, flags: 0 }, { side: 65, variant: 80, number: 1, flags: 64 }] });
    assert.throws(() => new RawBombHardener({ edition: 'cd1072', capacity: 24000 }, s.types, s.frames, s.random, s.setter, s.flare), TypeError);
    assert.throws(() => new RawBombHardener(s.pool, s.types.slice(0, 187), s.frames, s.random, s.setter, s.flare), TypeError);
    assert.throws(() => new RawBombHardener(s.pool, s.types, s.frames.slice(0, 187), s.random, s.setter, s.flare), TypeError);
    const other = new SidPool(24000, { server: false });
    assert.throws(() => new RawBombHardener(other, s.types, s.frames, s.random, s.setter, s.flare), TypeError);
    // 플레어가 없으면 이미 수행한 강화 프레임 지정(프레임 1)과 난수 소비(상태 0x003700a8)는 그대로이고 명시 오류이다.
    s.random.setState(55);
    const body = new RawBombHardener(s.pool, s.types, s.frames, s.random, s.setter, undefined);
    assert.throws(() => body.apply(s.parent, s.target), /플레어 미연결/);
    assert.equal(get(s.raw, 36), 1);
    assert.equal(s.random.state, 0x003700a8);
    assert.equal(s.state.updates, 2);
    // 현재 TYPE 이 표 크기 이상이면 자격 조회부터 거부한다.
    s.raw[10] = 188;
    assert.throws(() => body.mayTarget(s.target), RangeError);
});

test('RawBombHardener: 현재 프레임 방향·바이트 폭 읽기와 판본/표 크기(189칸)·지정기 경계', () => {
    // 현재 프레임(+36)이 1 이면 둘째 코드의 방향(B)으로 후보를 모아 프레임 1 을 지정한다(프레임 위치를 다른 오프셋에서 읽으면 0 이 된다).
    const s = scene({ genus: 4, seed: 55, qa: 0, current: 1, codes: [{ side: 65, variant: 80, number: 0, flags: 64 }, { side: 66, variant: 80, number: 0, flags: 64 }] });
    put(s.raw, 32, 0, 4);
    const body = new RawBombHardener(s.pool, s.types, s.frames, s.random, s.setter, s.flare);
    body.apply(s.parent, s.target);
    assert.equal(get(s.raw, 36), 1);
    // 운반체 WORD 의 높은 바이트까지 읽는다: 대상 SID 가 255 를 넘을 때도 같은 대상이면 아무 효과 없이 끝낸다.
    const pool = new SidPool(24000, { server: false });
    for (let index = 0; index < 300; index += 1) pool.allocate(2);
    const parent = pool.allocate(2);
    const target = pool.allocate(2);
    assert.ok(target > 255);
    put(pool.allocated(parent), 12, target, 2);
    pool.allocated(target)[10] = 94;
    const types = Array.from({ length: 188 }, () => ({ flags2: 0 }));
    const frames = Array.from({ length: 188 }, () => ({ codes: [] }));
    const calls = [];
    const setter = new SquidFrame(pool, types, { frameSize: forbidden, unpop: forbidden, pop: forbidden, update: () => calls.push('update') });
    new RawBombHardener(pool, types, frames, new GameRandom(55), setter, () => calls.push('flare')).apply(parent, target);
    assert.deepEqual(calls, []);
    // 판본 검사는 단독으로도 거부하고(지정기 풀을 같은 가짜 풀로 맞춰 다른 검사와 분리), 189칸 표도 거부한다.
    const fake = { edition: 'cd1072', capacity: 24000 };
    assert.throws(() => new RawBombHardener(fake, types, frames, new GameRandom(55), { pool: fake }, () => {}), TypeError);
    assert.throws(() => new RawBombHardener(pool, [...types, types[0]], frames, new GameRandom(55), setter, () => {}), TypeError);
    assert.throws(() => new RawBombHardener(pool, types, [...frames, frames[0]], new GameRandom(55), setter, () => {}), TypeError);
});
