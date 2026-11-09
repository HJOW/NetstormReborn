// 보행 경로 프레임 raw-walker-frame.js(runWalkerFrame)와 보행 경로 생성자를 원본 10.78 관측과 cpppj 계약으로 대조한다(cpppj `WalkerPathTests`).
//  - 원본 관측: 보행 경로 독립 PE 관측 348행(두 x87 정밀도 동일) — 입력 12 DWORD(종류 0 생성자/1 프레임·heap 값·할당 여부·TYPE(137 윈드/156 레인/그 밖)·Step 각도 DWORD·현재 프레임·공유 counter·
//    이동 dx/dy·중단/완료/지면 정지 선택·예약 시각 번호(0·10·10 다음 double·NaN·10 이전 double·±무한)·A/B 구간 배치) → 외부 효과 순서(I/H/S/G 지면/A/U/P), 경로 100080바이트 Adler-32, 부모 50바이트 raw, 공유 counter.
//  - 계약: WalkerPath_DeletionBoundaries(Halt/Step/지면/Unpop 각각에서 자기 삭제되면 부모나 다음 공간 효과를 읽지 않음).
// 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E209: 이동 유닛 경로 프레임 3종 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, adler32, bitsToFloat, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { RawPathRuntime, PATH_LAYOUT } from '../www/core/raw-path-runtime.js';
import { FrameCodeTable } from '../www/assets/terrain-tiles.js';
import { runWalkerFrame } from '../www/core/raw-walker-frame.js';

// 원본 관측 행이다. 없으면 검사를 건너뛴다. (2026-11-09 WB04-E209)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'walker-path-x86.tsv'));
const ROWS = 348;
const PARENT = 50;
const WIND_TYPE = 137;
const RAIN_TYPE = 156;

// 바이트 배열을 소문자 hex 로 만든다. (2026-11-09 WB04-E209)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 원본 외부 효과의 DWORD 순서와 같은 표기를 만든다. (2026-11-09 WB04-E209)
function record(events, name, values = []) {
    events.push(values.reduce((text, value) => `${text}:${value >>> 0}`, name));
}

// double 의 비트를 한 칸 옮긴 인접 값이다(nextafter). 유한 양수 입력 전용이다. (2026-11-09 WB04-E209)
function adjacent(value, step) {
    const view = new DataView(new ArrayBuffer(8));
    view.setFloat64(0, value, true);
    view.setBigUint64(0, view.getBigUint64(0, true) + BigInt(step), true);
    return view.getFloat64(0, true);
}

// 예약 경계는 now 10초의 인접 double 과 unordered 도 포함한다. (2026-11-09 WB04-E209)
const TIMES = [0, 10, adjacent(10, 1), Number.NaN, adjacent(10, -1), Number.POSITIVE_INFINITY, Number.NEGATIVE_INFINITY];

// 실제 TYPE helper 가 읽는 A/B 구간을 독립 명시 입력으로 만든다. 합성 구간에는 보행 앞/사이의 다른 side 도 넣는다. (2026-11-09 WB04-E209)
function walkerFrames(alternate) {
    const codes = [];
    const append = (side, count) => {
        // 코드의 물리 순서가 run.first/count 가 된다.
        for (let index = 0; index < count; index += 1) codes.push({ side, variant: 0, flags: 0 });
    };
    if (alternate) {
        append(0x43, 3); append(0x41, 5); append(0x44, 3); append(0x42, 7);
    } else {
        append(0x41, 8); append(0x42, 8);
    }
    return new FrameCodeTable(codes);
}

test('WalkerPath: 원본 10.78 348입력의 생성자/프레임 효과 순서·경로 Adler·부모 50바이트·공유 counter 가 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    let count = 0;
    // 동결 원문은 바꾸지 않고 모든 입력을 실행한다.
    for (const row of rows) {
        assert.equal(row.length, 9);
        const a = row[0].split(',').map(Number);
        assert.equal(a.length, 12);
        // x87 53/64비트 정밀도에서 저장 결과와 효과가 같아야 한다.
        for (let index = 1; index <= 4; index += 1) assert.equal(row[index], row[index + 4]);
        if (a[0] === 0) {
            const checksum = a[2] ? adler32(new RawPathRuntime({ heapFill: a[1], walker: true }).bytes) : adler32(new Uint8Array(PATH_LAYOUT.size).fill(a[1] & 0xff));
            assert.ok(checksum === Number(row[2]) && row[1] === 'N:100080', `생성자 행 ${count}`);
            count += 1;
            continue;
        }
        const pool = new SidPool(32768);
        const raw = pool.physical(PARENT);
        raw.fill(0);
        put(raw, 0, 0x12200000);
        raw[10] = a[3] & 0xff;
        put(raw, 14, floatToBits(10));
        put(raw, 18, floatToBits(20));
        put(raw, 36, a[5]);
        const runtime = new RawPathRuntime({ walker: true });
        runtime.setPath([{ x: 0, y: 0 }], [0]);
        runtime.setAttachment(0, PARENT);
        runtime.restoreStep(0, 0, a[9] === 2);
        runtime.setNextFrame(TIMES[a[10]]);
        const frames = Array.from({ length: 188 }, () => new FrameCodeTable([]));
        frames[RAIN_TYPE] = walkerFrames(a[11] !== 0);
        const events = [];
        const counter = { value: a[6] >>> 0 };
        const hooks = {
            immobile: () => { record(events, 'I'); return a[9] === 1; },
            halt: () => { record(events, 'H'); return a[9] === 3; },
            step: () => {
                record(events, 'S');
                // 실제 Step 경계가 쓴 각도 DWORD 를 직접 옮겨 유한값 전용 restoreStep 과 구별한다.
                put(runtime.bytes, PATH_LAYOUT.angle, a[4]);
                return { position: { x: Math.fround(10 + bitsToFloat(a[7])), y: Math.fround(20 + bitsToFloat(a[8])) }, angle: bitsToFloat(a[4]), finished: a[9] === 4 };
            },
            groundStop: (point) => { record(events, 'G', [floatToBits(point.x), floatToBits(point.y)]); return a[9] === 5; },
            arrive: () => { record(events, 'A'); runtime.setFinished(true); },
            unpop: (sid, flags) => { assert.equal(sid, PARENT); record(events, 'U', [flags]); },
            pop: (sid, point, flags) => {
                assert.equal(sid, PARENT);
                record(events, 'P', [floatToBits(point.x), floatToBits(point.y), flags]);
                put(raw, 14, floatToBits(point.x));
                put(raw, 18, floatToBits(point.y));
            }
        };
        runWalkerFrame(pool, PARENT, runtime, 10, frames, WIND_TYPE, RAIN_TYPE, counter, hooks, { value: true });
        const effects = events.length === 0 ? '-' : events.join(';');
        assert.equal(effects, row[1], `행 ${count} 입력 ${row[0]} 효과`);
        assert.equal(adler32(runtime.bytes), Number(row[2]), `행 ${count} 입력 ${row[0]} Adler`);
        assert.equal(hex(raw), row[3], `행 ${count} 입력 ${row[0]} raw`);
        assert.equal(counter.value, Number(row[4]), `행 ${count} 입력 ${row[0]} counter`);
        count += 1;
    }
    assert.equal(count, ROWS);
});

test('WalkerPath: 계약 — Halt/Step/지면/Unpop 에서 자기 삭제되면 부모나 다음 공간 효과를 읽지 않는다(DeletionBoundaries)', () => {
    // Halt/Step/지면/Unpop 각각에서 삭제 통지를 일으킨다.
    for (const stage of [0, 1, 2, 3]) {
        const pool = new SidPool(32768);
        const raw = pool.physical(PARENT);
        raw.fill(0);
        raw[10] = WIND_TYPE;
        const runtime = new RawPathRuntime({ walker: true });
        const counter = { value: 0 };
        const watch = { value: true };
        const calls = [0, 0, 0, 0, 0, 0];
        const hooks = {
            immobile: () => false,
            halt: () => { calls[0] += 1; if (stage === 0) watch.value = false; return false; },
            step: () => { calls[1] += 1; if (stage === 1) watch.value = false; return { position: { x: 10, y: 20 }, angle: 0, finished: false }; },
            groundStop: () => { calls[2] += 1; if (stage === 2) watch.value = false; return false; },
            arrive: () => { calls[3] += 1; },
            unpop: () => { calls[4] += 1; if (stage === 3) watch.value = false; },
            pop: () => { calls[5] += 1; }
        };
        runWalkerFrame(pool, PARENT, runtime, 10, [], WIND_TYPE, RAIN_TYPE, counter, hooks, watch);
        assert.ok(!watch.value && calls[5] === 0 && calls[3] === 0 && calls[0] === 1, `삭제 시점 ${stage}`);
        assert.ok(calls[1] === (stage >= 1 ? 1 : 0) && calls[2] === (stage >= 2 ? 1 : 0) && calls[4] === (stage === 3 ? 1 : 0), `삭제 시점 ${stage} 호출 수`);
    }
});

test('WalkerPath: 계약 — 10.72 풀·필수 효과 누락은 거부하고, 윈드/레인 밖 TYPE 은 원본 진단 553 으로 멈춘다', () => {
    const runtime = new RawPathRuntime({ walker: true });
    runtime.setPath([{ x: 0, y: 0 }], [0]);
    assert.throws(() => runWalkerFrame({ edition: 'cd1072' }, PARENT, runtime, 10, [], WIND_TYPE, RAIN_TYPE, { value: 0 }, {}, { value: true }), TypeError);
    const pool = new SidPool(32768);
    const raw = pool.physical(PARENT);
    raw.fill(0);
    raw[10] = 77;
    const hooks = {
        immobile: () => false, halt: () => false, groundStop: () => false, arrive: () => {}, unpop: () => {}, pop: () => {},
        step: () => ({ position: { x: 1, y: 2 }, angle: 0, finished: false })
    };
    assert.throws(() => runWalkerFrame(pool, PARENT, runtime, 10, [], WIND_TYPE, RAIN_TYPE, { value: 0 }, { ...hooks, groundStop: undefined }, { value: true }), TypeError);
    assert.throws(() => runWalkerFrame(pool, PARENT, runtime, 10, [], WIND_TYPE, RAIN_TYPE, { value: 0 }, hooks, { value: true }), /진단 553/);
});
