// 벌프 경로 프레임 raw-bulf-frame.js(runBulfFrame·bulfTurningFrame)와 벌프 경로 생성자를 원본 10.78 관측으로 대조한다(cpppj `BulfPathTests` 의 BulfPath_IndependentNativeBodies).
//  - 원본 관측: 벌프 경로 독립 PE 관측 314행(두 x87 정밀도 동일) — 입력 12 DWORD(종류 0 생성자/1 프레임/2 회전 helper·Step 전 각도 또는 heap 값·Step 각도·현재 프레임·경로 방향 또는 회전 방향·
//    이동 dx/dy·walkStepX·중단/완료 선택·난수 값·Pop 좌표 변이·실제 24프레임 E/F 여부) → 외부 효과 순서(I/H/S/A/U/P/R 난수/W 발소리), 경로 100080바이트 Adler-32, 부모 50바이트 raw, helper 반환값.
//  - 프레임 표는 cpppj 시험과 같은 독립 합성 입력(A~D 각 8프레임, E/F 각 8 또는 24프레임)이다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E209: 이동 유닛 경로 프레임 3종 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, adler32, bitsToFloat, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { RawPathRuntime, PATH_LAYOUT } from '../www/core/raw-path-runtime.js';
import { FrameCodeTable } from '../www/assets/terrain-tiles.js';
import { runBulfFrame, bulfTurningFrame } from '../www/core/raw-bulf-frame.js';

// 원본 관측 행이다. 없으면 검사를 건너뛴다. (2026-11-09 WB04-E209)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'bulf-path-x86.tsv'));
const ROWS = 314;
const PARENT = 50;
const BULF_TYPE = 135;

// 바이트 배열을 소문자 hex 로 만든다. (2026-11-09 WB04-E209)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 원본 외부 효과의 DWORD 순서와 같은 표기를 만든다. (2026-11-09 WB04-E209)
function record(events, name, values = []) {
    events.push(values.reduce((text, value) => `${text}:${value >>> 0}`, name));
}

// 실제 Bulf 와 같은 보행 A~D 각 8프레임, E/F 각 8(합성) 또는 24(실제) 프레임 표이다. (2026-11-09 WB04-E209)
function bulfFrames(originalCounts) {
    const codes = [];
    // 여섯 구간 저장 규칙만 독립 입력으로 쓴다.
    for (let side = 0x41; side <= 0x46; side += 1) {
        // E/F 만 실제 프레임 수가 다르다.
        for (let frame = 0; frame < (originalCounts && side >= 0x45 ? 24 : 8); frame += 1) codes.push({ side, variant: 0, flags: 0 });
    }
    return new FrameCodeTable(codes);
}

test('BulfPath: 원본 10.78 314입력의 생성자/프레임/회전 helper 효과 순서·경로 Adler·부모 50바이트·반환값이 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    let count = 0;
    // 새 원문 행은 바꾸지 않으며 두 x87 정밀도 결과가 같은지도 본다.
    for (const row of rows) {
        assert.equal(row.length, 9);
        const a = row[0].split(',').map(Number);
        assert.equal(a.length, 12);
        // 두 정밀도는 같은 저장값과 효과 순서를 남긴다.
        for (let index = 1; index <= 4; index += 1) assert.equal(row[index], row[index + 4]);
        if (a[0] === 0) {
            const checksum = a[2] ? adler32(new RawPathRuntime({ heapFill: a[1], bulf: true }).bytes) : adler32(new Uint8Array(PATH_LAYOUT.size).fill(a[1] & 0xff));
            assert.ok(checksum === Number(row[2]) && row[1] === 'N:100080', `생성자 행 ${count}`);
            count += 1;
            continue;
        }
        const pool = new SidPool(32768);
        const raw = pool.physical(PARENT);
        raw.fill(0);
        put(raw, 0, 0x12200000);
        raw[10] = BULF_TYPE;
        put(raw, 14, floatToBits(10));
        put(raw, 18, floatToBits(20));
        put(raw, 36, a[3]);
        const runtime = new RawPathRuntime({ bulf: true });
        runtime.setPath([{ x: 0, y: 0 }], [a[4] | 0]);
        runtime.setAttachment(0, PARENT);
        runtime.restoreStep(0, bitsToFloat(a[1]), a[8] === 2);
        const frames = Array.from({ length: 188 }, () => new FrameCodeTable([]));
        frames[BULF_TYPE] = bulfFrames(a[11] !== 0);
        const shapes = Array.from({ length: 188 }, () => ({ frames: [] }));
        // 메타 경계가 돌려준 같은 보행 간격을 현재 프레임에 공급한다.
        shapes[BULF_TYPE].frames = Array.from({ length: a[11] ? 80 : 48 }, () => ({ walkStepX: bitsToFloat(a[7]), walkStepY: 0 }));
        const events = [];
        let result = 0;
        if (a[0] === 2) result = bulfTurningFrame(frames[BULF_TYPE], bitsToFloat(a[2]), a[4]) >>> 0;
        else {
            const hooks = {
                immobile: () => { record(events, 'I'); return a[8] === 1; },
                halt: () => { record(events, 'H'); return a[8] === 3; },
                step: () => {
                    record(events, 'S');
                    const angle = bitsToFloat(a[2]);
                    runtime.restoreStep(0, angle, false);
                    return { position: { x: Math.fround(10 + bitsToFloat(a[5])), y: Math.fround(20 + bitsToFloat(a[6])) }, angle, finished: a[8] === 4 };
                },
                arrive: () => { record(events, 'A'); runtime.setFinished(true); },
                unpop: (sid, flags) => { assert.equal(sid, PARENT); record(events, 'U', [flags]); },
                pop: (sid, point, flags) => {
                    assert.equal(sid, PARENT);
                    record(events, 'P', [floatToBits(point.x), floatToBits(point.y), flags]);
                    put(raw, 14, floatToBits(a[10] ? 33 : point.x));
                    put(raw, 18, floatToBits(a[10] ? 44 : point.y));
                },
                randomInt: (low, high) => { record(events, 'R', [low, high]); return a[9] | 0; },
                sound: (x, y, name) => { record(events, 'W', [floatToBits(x), floatToBits(y)]); events[events.length - 1] += `:${name}:0`; }
            };
            runBulfFrame(pool, PARENT, runtime, frames, shapes, hooks, { value: true });
        }
        const effects = events.length === 0 ? '-' : events.join(';');
        assert.equal(effects, row[1], `행 ${count} 입력 ${row[0]} 효과`);
        assert.equal(adler32(runtime.bytes), Number(row[2]), `행 ${count} 입력 ${row[0]} Adler`);
        assert.equal(hex(raw), row[3], `행 ${count} 입력 ${row[0]} raw`);
        assert.equal(result, Number(row[4]), `행 ${count} 입력 ${row[0]} 반환`);
        count += 1;
    }
    assert.equal(count, ROWS);
});

test('BulfPath: 계약 — 10.72 풀·필수 효과 누락·회전 방향 2 는 거부하고, Unpop/Pop 뒤 자기 삭제면 발소리를 내지 않는다', () => {
    const frames = Array.from({ length: 188 }, () => new FrameCodeTable([]));
    frames[BULF_TYPE] = bulfFrames(true);
    assert.throws(() => bulfTurningFrame(frames[BULF_TYPE], 0, 2), RangeError);
    const runtime = new RawPathRuntime({ bulf: true });
    assert.throws(() => runBulfFrame({ edition: 'cd1072' }, PARENT, runtime, frames, [], {}, { value: true }), TypeError);
    assert.throws(() => runBulfFrame(new SidPool(32768), PARENT, runtime, frames, [], { immobile: () => false }, { value: true }), TypeError);
    // 삭제 시점별(0 Unpop, 1 Pop)로 이후 효과가 없는지 본다. 프레임 2 → 회전 F 구간으로 바뀌는 입력이다.
    for (const stage of [0, 1]) {
        const pool = new SidPool(32768);
        const raw = pool.physical(PARENT);
        raw.fill(0);
        raw[10] = BULF_TYPE;
        put(raw, 36, 2);
        const path = new RawPathRuntime({ bulf: true });
        path.setPath([{ x: 0, y: 0 }], [0]);
        path.restoreStep(0, 90, false);
        const shapes = Array.from({ length: 188 }, () => ({ frames: [] }));
        shapes[BULF_TYPE].frames = Array.from({ length: 80 }, () => ({ walkStepX: 0, walkStepY: 0 }));
        const watch = { value: true };
        const calls = [];
        const hooks = {
            immobile: () => false, halt: () => false, arrive: () => calls.push('A'),
            step: () => ({ position: { x: 1, y: 0 }, angle: 45, finished: false }),
            unpop: () => { calls.push('U'); if (stage === 0) watch.value = false; },
            pop: () => { calls.push('P'); if (stage === 1) watch.value = false; },
            randomInt: () => { calls.push('R'); return 1000; }, sound: () => calls.push('W')
        };
        runBulfFrame(pool, PARENT, path, frames, shapes, hooks, watch);
        assert.deepEqual(calls, stage === 0 ? ['U'] : ['U', 'P'], `삭제 시점 ${stage}`);
    }
});
