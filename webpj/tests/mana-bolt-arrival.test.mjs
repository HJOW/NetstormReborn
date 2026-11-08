// 마나 볼트 도착 raw-mana-bolt-arrival.js(RawManaBoltArrival·ManaBoltArrivalState)를 원본 10.78 독립 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: mana-bolt-arrival-x86.tsv 101행 — 후보의 extra·상태·소유자와 새 Regular 확보 여부·대상 존재에서 대상 검색 결과(Target)·도착 반환(1000/1001)과 효과 순서
//    (S 소리 위치 비트/R 프로세스 제거/N 확보 시도/A Regular 부착/K 삭제)가 같다.
//  - 계약: 선택된 위치의 효과 경계 누락은 생략하지 않고 거부, 첫 제거 뒤 공유 번호를 다시 읽기, 판본·타입 표 크기 거부.
// 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E143: 마나 볼트 도착 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, bitsToFloat, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { ManaBoltArrivalState, RawManaBoltArrival } from '../www/core/raw-mana-bolt-arrival.js';

// 원본 관측 행이다(첫 줄은 열 이름 주석). 없으면 해당 검사를 건너뛴다. (2026-11-09 WB04-E143)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'mana-bolt-arrival-x86.tsv')) ?? [];

// 볼트 SID, 후보 SID, 풀 크기, 볼트 좌표 비트와 후보 좌표 비트(같은 ftol 칸), 관측 행 수이다. (2026-11-09 WB04-E143)
const BOLT = 50;
const CANDIDATE = 60;
const CAPACITY = 24000;
const BOLT_X = 1101398016;
const BOLT_Y = 1102000947;
const CANDIDATE_X = 1101266944;
const CANDIDATE_Y = 1101791232;
const ROWS = 101;

// 예외가 나는지 본다. (2026-11-09 WB04-E143)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 볼트와 (있다면) 후보 하나를 같은 해시 머리에 연결한 장면이다. extra·state·owner 는 행 입력이고 나머지는 실행기와 같다. (2026-11-09 WB04-E143)
function makeScene(candidate) {
    const pool = new SidPool(CAPACITY, { server: true });
    const hash = new SquidHash();
    const types = Array.from({ length: 188 }, () => ({ flags1: 0, flags2: 0, footX: 0, footY: 0 }));
    for (const sid of [BOLT, CANDIDATE]) pool.physical(sid).fill(0);
    const bolt = pool.physical(BOLT);
    put(bolt, 14, BOLT_X);
    put(bolt, 18, BOLT_Y);
    bolt[34] = 1;
    if (candidate) {
        const raw = pool.physical(CANDIDATE);
        raw[10] = 100;
        raw[11] = candidate.state;
        raw[40] = candidate.extra;
        put(raw, 14, CANDIDATE_X);
        put(raw, 18, CANDIDATE_Y);
        raw[34] = candidate.owner;
        types[100].footX = 1;
        types[100].footY = 1;
        // 후보를 1단계 버킷 머리에 연결한다(볼트 칸과 후보 칸이 같은 ftol 칸이 되도록 실행기와 같은 좌표를 쓴다).
        put(raw, 4, hash.bucket(1, bitsToFloat(CANDIDATE_X), bitsToFloat(CANDIDATE_Y)), 2);
        hash.setBucket(1, bitsToFloat(CANDIDATE_X), bitsToFloat(CANDIDATE_Y), CANDIDATE);
    }
    return { pool, hash, types };
}

// 효과를 한 줄 사건으로 기록하는 경계 묶음이다. (2026-11-09 WB04-E143)
function makeHooks(events, allocate) {
    return {
        sound: (x, y, name) => { assert.equal(name, 'sunCannonRaiseLower5.wav'); events.push(`S:${floatToBits(x)}:${floatToBits(y)}`); },
        removeProcesses: (sid, type) => events.push(`R:${sid}:${type}`),
        reserveRegular: () => { events.push(`N:${allocate}`); return allocate !== 0; },
        addRegular: (sid, event, delay) => events.push(`A:${event}:${sid}:${floatToBits(delay)}`),
        destroy: (sid, flags) => events.push(`K:${sid}:${flags}`),
    };
}

test('마나 볼트 도착: 원본 10.78 관측 101행의 대상 검색·반환·효과 순서가 같다', { skip: rows.length === 0 }, () => {
    assert.equal(rows.length, ROWS);
    // 각 행은 서로 독립된 후보/소유권/할당 입력이다.
    for (const row of rows) {
        assert.equal(row.length, 8);
        const present = Number(row[4]) !== 0;
        const scene = makeScene(present ? { extra: Number(row[0]), state: Number(row[1]), owner: Number(row[2]) } : null);
        const events = [];
        const arrival = new RawManaBoltArrival(scene.pool, scene.hash, scene.types, new ManaBoltArrivalState(), makeHooks(events, Number(row[3])));
        assert.equal(arrival.target(BOLT), Number(row[5]), row.join(' '));
        assert.equal(arrival.arrive(BOLT), Number(row[6]), row.join(' '));
        assert.equal(events.join(';'), row[7], row.join(' '));
    }
});

test('마나 볼트 도착: 효과 경계가 없으면 생략하지 않고 거부하며 판본·타입 표 크기를 확인한다', () => {
    const scene = makeScene(null);
    const state = new ManaBoltArrivalState();
    // 후보가 없으니 소리 → 삭제 순서이고, 각 경계는 도달할 때 확인한다.
    assert.ok(throws(() => new RawManaBoltArrival(scene.pool, scene.hash, scene.types, state).arrive(BOLT)));
    assert.ok(throws(() => new RawManaBoltArrival(scene.pool, scene.hash, scene.types, state, { sound: () => {} }).arrive(BOLT)));
    assert.ok(!throws(() => new RawManaBoltArrival(scene.pool, scene.hash, scene.types, state, { sound: () => {}, destroy: () => {} }).arrive(BOLT)));
    assert.ok(throws(() => new RawManaBoltArrival({ edition: 'cd1072' }, scene.hash, scene.types, state, {})));
    assert.ok(throws(() => new RawManaBoltArrival(scene.pool, scene.hash, scene.types.slice(0, 171), state, {})));
    assert.equal(new RawManaBoltArrival(scene.pool, scene.hash, scene.types, state, {}).pool, scene.pool);
});

test('마나 볼트 도착: 첫 제거의 외부 효과 뒤 공유 번호를 다시 읽는다', () => {
    const scene = makeScene({ extra: 1, state: 0, owner: 1 });
    let shared = 61;
    const state = new ManaBoltArrivalState();
    state.currentRegularType = () => 46;
    state.currentSharedRegularType = () => shared;
    const events = [];
    const hooks = { ...makeHooks(events, 1), removeProcesses: (sid, type) => { events.push(`R:${sid}:${type}`); shared = 99; } };
    const arrival = new RawManaBoltArrival(scene.pool, scene.hash, scene.types, state, hooks);
    assert.equal(arrival.arrive(BOLT), 1000);
    // 첫 제거(46) 뒤 공유 번호가 바뀌어 둘째 제거는 바뀐 번호(99)를 쓴다.
    assert.ok(events.includes('R:50:46') && events.includes('R:50:99'), events.join(';'));
});
