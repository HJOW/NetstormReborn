// 운반체 회피 raw-carrier-escape.js(RawCarrierEscape)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: carrier-escape-x86.tsv 1807행(두 정밀도 열이 같음) — 입력 20 값(거리 float·event·초기 깊이·경로 유무·검사 전역·genus·부모 TYPE·spot·시작 성공 순번·난수·후보 체인·후보 flags·운반체 마스크·경로 마스크/모드·변이 번호·부모 좌표·이동 속도·현재 시각 비트)
//    → 반환(1000/1001), 외부 효과 순서(P 경로/R 난수/S 새 경로 시작/F 탐색 영역/O 후보 운반체 조회/B 임시 BFS/A 진단), 상태(새 경로 깊이·기존 경로 삭제 여부·검사 전역·시각 비트), 부모와 두 후보 슬롯 전체 raw.
//  - 계약: 단위 거리 비행체의 조기 반환은 미선택 경계를 요구하지 않고, 실제 시작 누락과 CD 판본 혼합은 거부한다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E101: 운반체 회피 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get, bitsToFloat, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { RawPathRuntime } from '../www/core/raw-path-runtime.js';
import { CarrierEscapeState, RawCarrierEscape } from '../www/core/raw-carrier-escape.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E101)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'carrier-escape-x86.tsv'));

// 예외가 나는지 본다. (2026-11-08 WB04-E101)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

const hex = (bytes) => Buffer.from(bytes).toString('hex');
const bits = (value) => floatToBits(value);

// 시각 double 의 uint64 비트이다. (2026-11-08 WB04-E101)
function bits64(value) {
    const view = new DataView(new ArrayBuffer(8));
    view.setFloat64(0, value, true);
    return view.getBigUint64(0, true);
}

// uint64 비트에서 double 을 만든다. (2026-11-08 WB04-E101)
function double64(value) {
    const view = new DataView(new ArrayBuffer(8));
    view.setBigUint64(0, value, true);
    return view.getFloat64(0, true);
}

// 같은 명시 BFS 배열만 제공하며 기대 시각이나 최적 후보는 계산하지 않는다. (2026-11-08 WB04-E101)
function escapeRoute(output, mode, which, mask) {
    if (!(mask & (1 << (which - 60)))) return;
    const lengths = [[2, 4], [2, 2], [4, 2], [0.125, 0.25]];
    output.setPath([{ x: 0, y: 0 }, { x: lengths[mode][which - 60], y: 0 }], [0, 0]);
}

test('CarrierEscape: 원본 10.78 1807입력의 반환·효과 순서·새 경로 깊이·삭제 여부·검사 전역·시각·세 슬롯 raw 가 두 정밀도 열과 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    const pool = new SidPool(24000);
    let count = 0;
    // 원본 기대값은 읽기만 하며 각 행을 같은 물리 입력으로 실행한다.
    for (const row of rows) {
        assert.equal(row.length, 9);
        const c = row[0].split(',').map((value) => BigInt(value));
        assert.equal(c.length, 20);
        const d = (index) => Number(BigInt.asUintN(32, c[index]));
        const hash = new SquidHash();
        const types = Array.from({ length: 188 }, () => ({ flags1: 0, flags2: 0, footX: 0, footY: 0 }));
        const scene = [[], [60], [60, 61], [61, 60]][d(10)];
        // 부모와 후보는 관측기와 같은 raw 바이트로 준비한다.
        for (const sid of [50, 60, 61]) {
            const raw = pool.physical(sid);
            raw.fill(0xa5);
            raw[10] = sid === 50 ? d(6) : sid === 60 ? 70 : 71;
            raw[11] = 0;
            raw[40] = 0;
            raw[33] = 1;
            put(raw, 14, sid === 50 ? d(16) : bits(sid === 60 ? 22.25 : 22.75));
            put(raw, 18, sid === 50 ? d(17) : bits(22.5));
            put(raw, 4, 0, 2);
        }
        // 같은 bucket 에서 원래 목록 순서를 사용하며 마지막 next 는 0 이다.
        for (let index = 0; index < scene.length; index += 1) put(pool.physical(scene[index]), 4, index + 1 < scene.length ? scene[index + 1] : 0, 2);
        if (scene.length) hash.setBucket(1, 22.25, 22.5, scene[0]);
        types[70].footX = types[70].footY = types[71].footX = types[71].footY = 1;
        types[70].flags1 = (d(11) & 1) ? 0x800 : 0;
        types[71].flags1 = (d(11) & 2) ? 0x800 : 0;
        // 현재 바뀔 수 있는 네 부모 타입의 genus 입력이다.
        for (const kind of [133, 137, 135, 158]) types[kind].flags2 = d(5);
        const spots = new Uint8Array(65536).fill(d(7) ? 16 : 0);
        const live = { now: double64(c[19]), checking: d(4) !== 0 };
        const state = new CarrierEscapeState();
        let old = new RawPathRuntime();
        old.setMoveAwayDepth(d(2) | 0);
        const fresh = new RawPathRuntime();
        fresh.setMoveAwayDepth(12345);
        let starts = 0;
        let active = 0;
        let deleted = false;
        const events = [];
        // 실제 효과와 관측한 탐색 영역을 동일 DWORD 순서로 기록한다.
        const event = (name, args = []) => events.push(args.reduce((text, value) => `${text}:${value >>> 0}`, name));
        const parent = pool.physical(50);
        const hooks = {
            path: (sid) => {
                event('P', [sid]);
                if (d(15) === 1) { put(parent, 14, bits(25.25)); put(parent, 18, bits(26.25)); old.setMoveAwayDepth(1); }
                return d(3) ? old : null;
            },
            random: (low, high) => {
                event('R', [low, high]);
                if (d(15) === 2) { put(parent, 14, bits(1)); put(parent, 18, bits(2)); }
                return d(9) | 0;
            },
            start: (sid, point) => {
                starts += 1;
                event('S', [sid, bits(point.x), bits(point.y)]);
                if (d(15) === 3) { put(parent, 14, bits(42.25)); put(parent, 18, bits(43.5)); live.checking = false; }
                if (d(3) && old) { old = null; deleted = true; }
                return d(8) && starts === d(8) ? fresh : null;
            },
            scan: (area) => event('F', [area.left, area.top, area.right, area.bottom]),
            blocker: (point) => {
                event('O', [bits(point.x), bits(point.y)]);
                active = 0;
                // 현재 물리 후보의 조회 좌표만 대조해 명시 경계 입력을 고른다.
                for (const sid of [60, 61]) {
                    const raw = pool.physical(sid);
                    if (get(raw, 14) === bits(point.x) && get(raw, 18) === bits(point.y)) { active = sid; break; }
                }
                if (!active) throw new Error('회피 검사 후보 좌표 입력 불일치');
                if (d(15) === 4) { parent[10] = 137; put(pool.physical(active), 18, bits(23.5)); }
                return (d(12) & (1 << (active - 60))) ? 70 : 0;
            },
            search: (output, sid, source, destination, flags) => {
                assert.equal(sid, 50);
                event('B', [bits(source.x), bits(source.y), bits(destination.x), bits(destination.y), flags]);
                assert.equal(output.count, 0);
                escapeRoute(output, d(14), active, d(13));
                if (d(15) === 5) { live.now = 1000.25; parent[10] = 135; }
                if (d(15) === 6 || d(15) === 8) { put(pool.physical(active), 14, bits(d(15) === 6 ? 23 : 0)); put(pool.physical(active), 18, bits(23)); }
            },
            movement: (sid) => [pool.physical(sid)[10] === 135 ? 3.25 : bitsToFloat(d(18)), 90],
            valid: (p) => p.x > 0 && p.x < 256 && p.y > 0 && p.y < 256,
            assertion: (line) => {
                event('A', [line]);
                if (d(15) === 7 || d(15) === 9) old.setMoveAwayDepth(0);
                if (d(15) === 7) live.checking = false;
            }
        };
        const escape = new RawCarrierEscape(pool, hash, types, spots, () => live.now, () => live.checking, [137, 156, 135, 187], state, hooks);
        assert.equal(escape.pool, pool);
        const result = escape.run(50, bitsToFloat(d(0)), d(1));
        const current = `${fresh.moveAwayDepth >>> 0},${deleted ? 1 : 0},${live.checking ? 1 : 0},${bits64(live.now)}`;
        const raws = [50, 60, 61].map((sid) => hex(pool.slot(sid))).join(',');
        const label = `행 ${count}: 입력 ${row[0]}`;
        assert.equal(String(result), row[1], label);
        assert.equal(events.join(';'), row[2], label);
        assert.equal(current, row[3], label);
        assert.equal(raws, row[4], label);
        assert.deepEqual(row.slice(1, 5), row.slice(5, 9), `${label} (64비트 열)`);
        count += 1;
    }
    assert.equal(count, 1807);
});

test('CarrierEscape: 단위 거리 비행체의 조기 반환은 미선택 경계를 요구하지 않고 실제 시작 누락과 CD 판본 혼합은 거부한다', () => {
    const pool = new SidPool(24000);
    const hash = new SquidHash();
    const types = Array.from({ length: 188 }, () => ({ flags1: 0, flags2: 0, footX: 0, footY: 0 }));
    const spots = new Uint8Array(65536);
    const raw = pool.physical(50);
    raw[10] = 133;
    put(raw, 14, bits(20));
    put(raw, 18, bits(21));
    types[133].flags2 = 0x20000;
    const cardinal = [137, 156, 135, 187];
    const escape = new RawCarrierEscape(pool, hash, types, spots, () => 0, () => false, cardinal, new CarrierEscapeState(), { path: () => null });
    assert.equal(escape.run(50, 1, 0), 1000);
    types[133].flags2 = 0x10000;
    assert.ok(throws(() => escape.run(50, 1, 0)));
    assert.ok(throws(() => new RawCarrierEscape({ edition: 'cd1072' }, hash, types, spots, () => 0, () => false, cardinal, new CarrierEscapeState(), {})));
    assert.ok(throws(() => new RawCarrierEscape(pool, hash, types, spots.subarray(0, 100), () => 0, () => false, cardinal, new CarrierEscapeState(), {})));
});
