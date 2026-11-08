// 경로 런타임 raw-path-runtime.js(RawPathRuntime)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: path-constructor-x86.tsv 32행(PATCH 종류 8종 × heapFill 0/A5 × 잔여 인자의 100080바이트 전체 이미지 RLE), pathruntime-x86.tsv 의 `originals` ctor 4행(FNV32)·prepare 24행(꼭짓점 시각 QWORD 4개)·
//    step 80행(profile 별 연쇄 Step: 반환 위치/각도/완료/발소리 수와 현재 단계·각도 필드), flypath-x86.tsv 의 ctor 3행(FNV32)·init 24행(기존 개수 0/1/2 × 도착 좌표 8종의 이륙/수평/착륙 꼭짓점 추가 뒤 FNV32).
//    `originalCD` 행(10.72)과 RawPathProcess 를 쓰는 arrival/frame 행(해당 단위)은 사용하지 않는다.
//  - 계약: 거부해야 하는 입력(빈 경로 Prepare·속도 0·빈 경로 Step), 방향 차이의 최단 회전, 필드 접근자의 폭/위치.
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E98: 경로 런타임 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, bitsToFloat, floatToBits, fnv32 } from './raw-support.mjs';
import { RawPathRuntime, PATH_LAYOUT } from '../www/core/raw-path-runtime.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E98)
const fixtures = path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures');
const constructorRows = readRows(path.join(fixtures, 'path-constructor-x86.tsv'));
const runtimeRows = readRows(path.join(fixtures, 'pathruntime-x86.tsv'));
const flightRows = readRows(path.join(fixtures, 'flypath-x86.tsv'));

// 예외가 나는지 본다. (2026-11-08 WB04-E98)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 원문 비교 입력의 꼭짓점·방향·시각이다. (2026-11-08 WB04-E98)
const POINTS = [{ x: 10, y: 20 }, { x: 13.1, y: 24.7 }, { x: 15.2, y: 19.8 }, { x: 23.9, y: 27.6 }].map((point) => ({ x: Math.fround(point.x), y: Math.fround(point.y) }));
const DIRECTIONS = [7, 0, 4, 6];
const TIMES = [0, 1, 2, 3];

// uint64 십진 문자열의 비트 패턴을 double 로 읽는다. (2026-11-08 WB04-E98)
function doubleOf(text) {
    const view = new DataView(new ArrayBuffer(8));
    view.setBigUint64(0, BigInt(text), true);
    return view.getFloat64(0, true);
}

// double 의 비트 패턴을 uint64 BigInt 로 바꾼다. (2026-11-08 WB04-E98)
function bits64(value) {
    const view = new DataView(new ArrayBuffer(8));
    view.setFloat64(0, value, true);
    return view.getBigUint64(0, true);
}

// 반복 BYTE 압축 `횟수:바이트,...` 를 전체 이미지로 푼다. (2026-11-08 WB04-E98)
function expand(rle) {
    const result = new Uint8Array(PATH_LAYOUT.size);
    let at = 0;
    // 모든 반복 구간을 한 번 소비한다.
    for (const run of rle.split(',')) {
        const [count, byte] = run.split(':').map(Number);
        assert.ok(at + count <= result.length);
        result.fill(byte, at, at + count);
        at += count;
    }
    assert.equal(at, PATH_LAYOUT.size);
    return result;
}

test('PathRuntime: 원본 10.78 생성자 전체 이미지 — 32행 RLE(8종×heap 0/A5)와 ctor 4행·비행 ctor 3행 FNV 가 같다', { skip: (!constructorRows || !runtimeRows || !flightRows) && '원본 fixture 없음' }, () => {
    assert.equal(constructorRows.length, 32);
    // 종류 번호로 생성자 종류를 고른다(23 기본, 54 사제, 39 fly, 40 worm, 42 balloon, 24 dude, 41 bulf, 38 walker).
    for (const row of constructorRows) {
        assert.equal(row.length, 5);
        const kind = Number(row[0]);
        const runtime = new RawPathRuntime({ priest: kind === 54, heapFill: Number(row[2]), flying: kind === 39, worm: kind === 40, balloon: kind === 42, dude: kind === 24, bulf: kind === 41, walker: kind === 38 });
        assert.deepEqual(Buffer.from(runtime.bytes), Buffer.from(expand(row[4])), `종류 ${kind} heap ${row[2]}`);
    }
    let count = 0;
    // 기본/사제 ctor 의 FNV32 이다.
    for (const row of runtimeRows.filter((candidate) => candidate[0] === 'ctor' && candidate[1] === 'originals')) {
        const runtime = new RawPathRuntime({ priest: Number(row[2]) !== 0, heapFill: Number(row[3]) });
        assert.equal(fnv32(runtime.bytes), Number(row[4]));
        count += 1;
    }
    assert.equal(count, 4);
    // 비행 ctor 의 FNV32 이다.
    for (const row of flightRows.filter((candidate) => candidate[0] === 'ctor')) {
        assert.equal(fnv32(new RawPathRuntime({ heapFill: Number(row[1]), flying: true }).bytes), Number(row[2]));
        count += 1;
    }
    assert.equal(count, 7);
});

test('PathRuntime: 원본 10.78 Prepare 24행의 꼭짓점 도착 시각 QWORD 와 Step 80행의 연쇄 반환·필드·발소리 수가 같다', { skip: !runtimeRows && '원본 fixture 없음' }, () => {
    let prepared = 0;
    // 속도·회전 속도·2초 경계의 독립 입력이다.
    for (const row of runtimeRows.filter((candidate) => candidate[0] === 'prepare' && candidate[1] === 'originals')) {
        const runtime = new RawPathRuntime();
        runtime.setPath(POINTS, DIRECTIONS);
        runtime.prepare(doubleOf(row[2]), doubleOf(row[3]), bitsToFloat(Number(row[4])), bitsToFloat(Number(row[5])), 7);
        const expected = row[6].split(',');
        // 예상 시각을 다시 계산하지 않고 원본 QWORD 와 대조한다.
        for (let index = 0; index < 4; index += 1) assert.equal(bits64(runtime.time(index)), BigInt(expected[index]), `Prepare ${row.join(' ')} 꼭짓점 ${index}`);
        prepared += 1;
    }
    assert.equal(prepared, 24);
    let stepped = 0;
    let runtime = null;
    let lastProfile = -1;
    const observation = { flags: 0, stamp: 0 };
    // 각 profile 은 처음만 초기화하고 원본처럼 한 꼭짓점씩 다음 호출에 이어진다.
    for (const row of runtimeRows.filter((candidate) => candidate[0] === 'step' && candidate[1] === 'originals')) {
        const profile = Number(row[2]);
        if (lastProfile !== profile) {
            runtime = new RawPathRuntime();
            runtime.setPath(POINTS, [profile, (profile + 1) % 8, (profile + 5) % 8, profile], TIMES);
            observation.flags = 0;
            observation.stamp = 0;
            lastProfile = profile;
        }
        assert.equal(runtime.current, Number(row[4]) | 0);
        assert.equal(floatToBits(runtime.angle), Number(row[5]));
        let sounds = 0;
        const sample = runtime.step(doubleOf(row[3]), { x: 10, y: 20 }, observation, () => { sounds += 1; });
        const expected = row[6].split(',').map(Number);
        const label = `Step ${row.join(' ')}`;
        assert.equal(floatToBits(sample.position.x), expected[0], label);
        assert.equal(floatToBits(sample.position.y), expected[1], label);
        assert.equal(floatToBits(sample.angle), expected[2], label);
        assert.equal(runtime.current, expected[3] | 0, label);
        assert.equal(floatToBits(runtime.angle), expected[4], label);
        assert.equal(sample.finished, expected[5] !== 0, label);
        assert.equal(sounds, expected[6], label);
        stepped += 1;
    }
    assert.equal(stepped, 80);
});

test('PathRuntime: 원본 10.78 비행 Init 24행이 기존 개수를 보존한 채 이륙/수평/착륙 꼭짓점을 추가한 전체 이미지 FNV 와 같다', { skip: !flightRows && '원본 fixture 없음' }, () => {
    let count = 0;
    // 기존 count 0/1/2 와 8방향·축/동률 도착 좌표 전부를 대조한다.
    for (const row of flightRows.filter((candidate) => candidate[0] === 'init')) {
        const runtime = new RawPathRuntime({ flying: true });
        const existing = Number(row[1]);
        runtime.setPath(Array.from({ length: existing }, () => ({ x: 0, y: 0 })), Array.from({ length: existing }, () => 0));
        runtime.restoreStep(existing, 0);
        runtime.initializeFlight({ x: 10, y: 20 }, { x: bitsToFloat(Number(row[2])), y: bitsToFloat(Number(row[3])) });
        assert.equal(fnv32(runtime.bytes), Number(row[4]), `Init ${row.join(' ')}`);
        count += 1;
    }
    assert.equal(count, 24);
});

test('PathRuntime: 거부 입력·최단 회전·필드 접근자의 위치와 폭이 cpppj 계약과 같다', () => {
    const runtime = new RawPathRuntime();
    const observation = { flags: 0, stamp: 0 };
    assert.ok(throws(() => runtime.prepare(0, 0, 1, 1, 0)));
    assert.ok(throws(() => runtime.step(0, { x: 1, y: 1 }, observation, null)));
    runtime.setPath([{ x: 1, y: 1 }], [0]);
    assert.ok(throws(() => runtime.prepare(0, 0, 0, 1, 0)));
    assert.ok(throws(() => runtime.prepare(0, 0, 1, 0, 0)));
    assert.equal(RawPathRuntime.directionDelta(0, 4), 180);
    assert.equal(RawPathRuntime.directionDelta(7, 0), 45);
    assert.equal(RawPathRuntime.directionDelta(0, 7), -45);
    // 크기 불일치/용량 초과/시각 개수 불일치 입력은 거부한다.
    assert.ok(throws(() => runtime.setPath([{ x: 0, y: 0 }], [])));
    assert.ok(throws(() => runtime.setPath([{ x: 0, y: 0 }], [0], [0, 1])));
    assert.ok(throws(() => runtime.point(1)));
    assert.ok(throws(() => runtime.replacePoint(-1, { x: 0, y: 0 })));
    // 필드 접근자는 원본 위치/폭만 쓰고 옆의 값을 보존한다.
    const fresh = new RawPathRuntime({ heapFill: 0xa5 });
    fresh.setTargets(0x11223344, 0x55667788);
    fresh.setCommandFields(7, 8, 9, { x: 1.5, y: 2.5 });
    fresh.setMoveAwayDepth(-3);
    fresh.setDeathWatchToken(1);
    fresh.setAttachment(12, 34);
    fresh.setNextFrame(0.5);
    fresh.setFinished(true);
    assert.equal(fresh.targetA, 0x11223344);
    assert.equal(fresh.targetB, 0x55667788);
    assert.equal(fresh.helper, 7);
    assert.equal(fresh.moveAwayDepth, -3);
    assert.equal(fresh.deathWatchToken, 1);
    assert.equal(fresh.nextFrame, 0.5);
    assert.ok(fresh.finished);
    const view = new DataView(fresh.bytes.buffer);
    assert.equal(view.getUint32(4, true), 12);
    assert.equal(view.getUint32(8, true), 34);
    assert.equal(view.getFloat32(0x186dc, true), 1.5);
    assert.equal(view.getFloat32(0x186e0, true), 2.5);
    assert.equal(view.getUint32(0x186e4, true), 8);
    assert.equal(view.getUint32(0x186e8, true), 9);
    fresh.clearHelper();
    assert.equal(fresh.helper, 0);
    assert.equal(fresh.moveAwayDepth, -3);
    // 종류 배타와 상태 이관 범위를 확인한다.
    assert.ok(throws(() => new RawPathRuntime({ priest: true, flying: true })));
    assert.ok(throws(() => new RawPathRuntime({ flying: true, worm: true })));
    assert.ok(throws(() => fresh.restoreStep(1, 0)));
    assert.ok(throws(() => runtime.restoreStep(0, Number.NaN)));
    assert.equal(runtime.physicalTime(-1), new DataView(runtime.bytes.buffer).getFloat64(PATH_LAYOUT.times - 8, true));
    assert.ok(throws(() => runtime.physicalTime(5000)));
    assert.ok(throws(() => runtime.physicalTime(-2)));
});
