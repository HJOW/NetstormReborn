// 벌룬 경로 프레임 raw-balloon-frame.js(runBalloonFrame)와 벌룬 경로 생성자·비행 Init 을 원본 10.78 관측으로 대조한다(cpppj `BalloonPathTests` 의 BalloonPath_IndependentNativeBodies).
//  - 원본 관측: 벌룬 경로 독립 PE 관측 688행(두 x87 정밀도 동일) — 입력 9 DWORD(종류 0 생성자/1 비행 Init/2 프레임·현재 단계 또는 heap 값·고도 또는 할당 여부·TYPE·now/목적지 x·y·
//    중단/완료 선택·외부 변이 선택·경로 시각 모양) → 외부 효과 순서(H 중단/S 발소리/F 화염/A 도착/U Unpop/P Pop), 경로 100080바이트 Adler-32, 부모 50바이트 raw, 관찰 flags/stamp.
//  - 생성자 행(종류 0)은 할당 실패(heap 값으로 채운 이미지)와 실제 벌룬 생성자 이미지를 구분한다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E209: 이동 유닛 경로 프레임 3종 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get, adler32, bitsToFloat, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { RawPathRuntime, PATH_LAYOUT } from '../www/core/raw-path-runtime.js';
import { runBalloonFrame } from '../www/core/raw-balloon-frame.js';

// 원본 관측 행이다. 없으면 검사를 건너뛴다. (2026-11-09 WB04-E209)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'balloon-path-x86.tsv'));
const ROWS = 688;
const PARENT = 50;
const BALLOON_TYPE = 111;

// 바이트 배열을 소문자 hex 로 만든다. (2026-11-09 WB04-E209)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 원문 외부 효과의 모든 인자를 같은 순서로 기록한다(부호 없는 DWORD). (2026-11-09 WB04-E209)
function record(events, name, values = []) {
    events.push(values.reduce((text, value) => `${text}:${value >>> 0}`, name));
}

test('BalloonPath: 원본 10.78 688입력의 생성자/비행 Init/프레임 효과 순서·경로 100080바이트 Adler·부모 50바이트·관찰 flags/stamp 가 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    let count = 0;
    // 각 입력은 새 실제 runtime 과 같은 물리 바이트로 시작한다.
    for (const row of rows) {
        assert.equal(row.length, 11);
        const a = row[0].split(',').map(Number);
        assert.equal(a.length, 9);
        // 두 정밀도의 저장 원문은 같아야 한다.
        for (let index = 1; index <= 5; index += 1) assert.equal(row[index], row[index + 5]);
        if (a[0] === 0) {
            // 실제 할당 실패 때에는 생성자가 실행되지 않는다.
            const checksum = a[2] ? adler32(new RawPathRuntime({ heapFill: a[1], balloon: true }).bytes) : adler32(new Uint8Array(PATH_LAYOUT.size).fill(a[1] & 0xff));
            assert.equal(checksum, Number(row[2]), `생성자 행 ${count}`);
            assert.ok(row[1] === 'N:100080' && Number(row[3]) === a[2]);
            count += 1;
            continue;
        }
        const pool = new SidPool(32768);
        const raw = pool.physical(PARENT);
        raw.fill(0);
        put(raw, 0, 0x5040c0);
        put(raw, 6, 7, 2);
        raw[10] = a[3] & 0xff;
        raw[30] = a[2] & 0xff;
        put(raw, 14, floatToBits(10));
        put(raw, 18, floatToBits(20));
        const runtime = new RawPathRuntime({ balloon: true });
        runtime.setAttachment(7, PARENT);
        const observation = { flags: 0, stamp: 0 };
        const events = [];
        if (a[0] === 1) {
            const prefix = Array.from({ length: a[1] }, () => ({ x: 0, y: 0 }));
            runtime.setPath(prefix, new Array(a[1]).fill(0));
            runtime.initializeFlight({ x: 10, y: 20 }, { x: bitsToFloat(a[4]), y: bitsToFloat(a[5]) }, 44, a[2] & 0xff);
        } else {
            // 원문 경로/시각 입력이다.
            const points = a[8] === 2 ? Array.from({ length: 4 }, () => ({ x: 10, y: 20 })) : [{ x: 10, y: 20 }, { x: 10, y: 16 }, { x: 14, y: 16 }, { x: 14, y: 20 }];
            const times = a[8] === 1 ? [0, 1, 1.00001, 1.00002] : [0, 1, 3, 4];
            runtime.setPath(points, [0, 0, 2, 2], times);
            runtime.restoreStep(a[1] | 0, 0, a[6] === 1);
            // 원본 현재 게임 시각과 자기 삭제 관찰자이다.
            const now = bitsToFloat(a[4]);
            const watch = { value: true };
            const parentPoint = () => ({ x: bitsToFloat(get(raw, 14)), y: bitsToFloat(get(raw, 18)) });
            const hooks = {
                halt: () => { record(events, 'H'); return a[6] === 2; },
                step: () => runtime.step(now, parentPoint(), observation, () => { const point = parentPoint(); record(events, 'S', [floatToBits(point.x), floatToBits(point.y), 123]); }),
                arrive: () => { record(events, 'A'); runtime.setFinished(true); },
                unpop: (sid, flags) => { record(events, 'U', [sid, flags]); if (a[7] === 2) raw[30] = 99; },
                pop: (_sid, point, flags) => { record(events, 'P', [floatToBits(point.x), floatToBits(point.y), flags]); put(raw, 14, floatToBits(point.x)); put(raw, 18, floatToBits(point.y)); },
                flame: (x, y) => {
                    record(events, 'F', [floatToBits(x), floatToBits(y), 0]);
                    if (a[7] === 1) { put(raw, 14, floatToBits(30)); put(raw, 18, floatToBits(40)); }
                }
            };
            runBalloonFrame(pool, PARENT, runtime, now, BALLOON_TYPE, hooks, watch);
        }
        const effects = events.length === 0 ? '-' : events.join(';');
        assert.equal(effects, row[1], `행 ${count} 입력 ${row[0]} 효과`);
        assert.equal(adler32(runtime.bytes), Number(row[2]), `행 ${count} 입력 ${row[0]} Adler`);
        assert.equal(hex(raw), row[3], `행 ${count} 입력 ${row[0]} raw`);
        assert.equal(observation.flags, Number(row[4]), `행 ${count} flags`);
        assert.equal(observation.stamp, Number(row[5]), `행 ${count} stamp`);
        count += 1;
    }
    assert.equal(count, ROWS);
});

test('BalloonPath: 계약 — Step 뒤·화염 뒤·Unpop 뒤 자기 삭제면 고도 BYTE 와 Pop 을 건너뛰고, 완료/중단이면 Step 을 부르지 않는다', () => {
    // 삭제 시점별(0 Step, 1 화염, 2 Unpop)로 이후 효과가 없는지 본다.
    for (const stage of [0, 1, 2]) {
        const pool = new SidPool(32768);
        const raw = pool.physical(PARENT);
        raw.fill(0);
        raw[10] = BALLOON_TYPE;
        put(raw, 14, floatToBits(10));
        put(raw, 18, floatToBits(20));
        const runtime = new RawPathRuntime({ balloon: true });
        runtime.setPath([{ x: 10, y: 20 }, { x: 10, y: 16 }, { x: 14, y: 16 }, { x: 14, y: 20 }], [0, 0, 2, 2], [0, 1, 3, 4]);
        runtime.restoreStep(1, 0, false);
        const watch = { value: true };
        const calls = [];
        const hooks = {
            halt: () => { calls.push('H'); return false; },
            step: () => { calls.push('S'); if (stage === 0) watch.value = false; return { position: { x: 10, y: 19 }, angle: 0, finished: false }; },
            arrive: () => calls.push('A'),
            unpop: () => { calls.push('U'); if (stage === 2) watch.value = false; },
            pop: () => calls.push('P'),
            flame: () => { calls.push('F'); if (stage === 1) watch.value = false; }
        };
        // now=1 이면 이륙 구간 남은 시각 0 → 고도 44, now=0 이면 남은 시각 1 → 고도 0 이라 화염이 난다.
        runBalloonFrame(pool, PARENT, runtime, stage === 2 ? 1 : 0, BALLOON_TYPE, hooks, watch);
        const expected = [['H', 'S'], ['H', 'S', 'F'], ['H', 'S', 'U']][stage];
        assert.deepEqual(calls, expected, `삭제 시점 ${stage}`);
        assert.equal(raw[30], 0, `삭제 시점 ${stage} 고도 BYTE 보존`);
    }
    const pool = new SidPool(32768);
    const runtime = new RawPathRuntime({ balloon: true });
    runtime.setPath([{ x: 0, y: 0 }], [0]);
    runtime.restoreStep(0, 0, true);
    const calls = [];
    runBalloonFrame(pool, PARENT, runtime, 0, BALLOON_TYPE, { halt: () => calls.push('H'), step: () => calls.push('S') }, { value: true });
    assert.deepEqual(calls, []);
});

test('BalloonPath: 계약 — 착륙 구간 남은 시각은 float 로 한 번 저장한 뒤 고도를 계산한다(fstp 저장 지점, fixture 가 구분하지 못하는 경계)', () => {
    // 마지막 구간(현재 3, 구간 길이 1)에서 남은 시각 0.25−2^-40 은 float 저장 뒤 0.25 가 되어 고도 ftol(11)=11 이다(저장하지 않으면 10.99… → 10).
    const pool = new SidPool(32768);
    const raw = pool.physical(PARENT);
    raw.fill(0);
    raw[10] = BALLOON_TYPE;
    raw[30] = 255;
    put(raw, 14, floatToBits(14));
    put(raw, 18, floatToBits(16));
    const runtime = new RawPathRuntime({ balloon: true });
    runtime.setPath([{ x: 10, y: 20 }, { x: 10, y: 16 }, { x: 14, y: 16 }, { x: 14, y: 20 }], [0, 0, 2, 2], [0, 1, 3, 4]);
    runtime.restoreStep(3, 0, false);
    const now = 3.75 + 2 ** -40;
    assert.equal(4 - now, 0.25 - 2 ** -40);
    const hooks = {
        halt: () => false, arrive: () => assert.fail('도착 아님'), flame: () => assert.fail('화염 아님'), unpop: () => {}, pop: () => {},
        step: () => ({ position: { x: 14, y: 19 }, angle: 0, finished: false })
    };
    runBalloonFrame(pool, PARENT, runtime, now, BALLOON_TYPE, hooks, { value: true });
    assert.equal(raw[30], 11);
});
