// 운반물 내려놓기 위치 raw-carrier-drop-point.js(RawCarrierDropPoint)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: carrier-drop-point-x86.tsv 998행 — 입력 18 DWORD(연산 0 Find/1 FindFootprint/2 Snap·x·y·소유자·TYPE flags2·footX·footY·중심 x/y·지도 한계·실패점·성공 순번·결과·재진입 변이·인자·flags·소유자)
//    → 결과 점(53비트 x87 정밀도 열), 배치/중심 훅 호출 순서(P/C), 현재 전역 여섯 값(지도 한계·실패점·flags2·footX·footY), 운반물 슬롯 전체 50바이트 raw.
//    Snap 256행은 64비트 정밀도 열이 다른 31행을 포함하며 기본 53비트 열만 비교한다.
//  - 계약: 누락 훅·판본·TYPE/SID 범위 오류를 해당 분기를 쓰는 시점에 거부한다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E95: 운반물 내려놓기 위치 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, floatToBits, bitsToFloat } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { RawCarrierDropPoint } from '../www/core/raw-carrier-drop-point.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E95)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'carrier-drop-point-x86.tsv'));

// 예외가 나는지 본다. (2026-11-08 WB04-E95)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 입력 열 위치이다. (2026-11-08 WB04-E95)
const C = { op: 0, x: 1, y: 2, owner: 3, typeFlags: 4, footX: 5, footY: 6, centerX: 7, centerY: 8, limit: 9, invalidX: 10, invalidY: 11, success: 12, result: 13, mutation: 14, argument: 15, queryFlags: 16, queryOwner: 17 };
const hex = (bytes) => Buffer.from(bytes).toString('hex');
const bits = (value) => floatToBits(value);

// 원본 관측과 같은 입력 저장소와 명시 경계 효과만 소유하는 장면이다. (2026-11-08 WB04-E95)
function makeScene() {
    const scene = {
        pool: new SidPool(24000, { server: false }),
        types: Array.from({ length: 188 }, () => ({ flags2: 0, footX: 0, footY: 0 })),
        state: { mapLimit: 256, invalidPoint: [0, 0] },
        input: [],
        trace: [],
        queries: 0
    };
    // 운반물 슬롯(50)을 A5 로 채우고 원문이 지정한 필드만 덮어쓴다.
    scene.prepare = (values) => {
        scene.input = values;
        scene.queries = 0;
        scene.trace = [];
        const raw = scene.pool.physical(50);
        raw.fill(0xa5);
        raw[10] = 158;
        raw[34] = values[C.owner] & 0xff;
        put(raw, 14, values[C.x]);
        put(raw, 18, values[C.y]);
        scene.types[123].flags2 = values[C.typeFlags];
        scene.types[123].footX = values[C.footX] | 0;
        scene.types[123].footY = values[C.footY] | 0;
        scene.state.mapLimit = values[C.limit] | 0;
        scene.state.invalidPoint = [bitsToFloat(values[C.invalidX]), bitsToFloat(values[C.invalidY])];
    };
    // 배치 반환·첫 호출의 재진입 변경과 중심 입력만 공급하며 위치는 계산하지 않는다.
    scene.hooks = {
        mayPlace: (q) => {
            scene.queries += 1;
            scene.trace.push(`P,${q.type},${q.argument},${bits(q.x)},${bits(q.y)},${q.flags >>> 0},${q.owner >>> 0},${q.mode}`);
            if (scene.queries === 1) {
                const m = scene.input[C.mutation];
                const raw = scene.pool.physical(50);
                if (m === 1 || m === 8) raw[34] = (scene.input[C.owner] + 3) & 0xff;
                if (m === 2) { scene.types[123].footX = 0; scene.types[123].footY = 0; }
                if (m === 3 || m === 8) { scene.types[123].footX = 3; scene.types[123].footY = 3; }
                if (m === 4 || m === 8) scene.state.mapLimit = 64;
                if (m === 5 || m === 8) scene.state.invalidPoint = [17.25, 18.5];
                if (m === 6) { put(raw, 14, bits(99.0)); put(raw, 18, bits(-77.0)); }
                if (m === 7) scene.types[123].flags2 = 0x100;
                if (m === 9) scene.types[123].footY = 1;
            }
            return scene.queries === scene.input[C.success] && scene.input[C.result] !== 0;
        },
        center: (sid) => {
            scene.trace.push(`C,${sid}`);
            return [bitsToFloat(scene.input[C.centerX]), bitsToFloat(scene.input[C.centerY])];
        }
    };
    // 지도 한계·실패점·TYPE 의 현재 여섯 DWORD 이다.
    scene.snapshot = () => [scene.state.mapLimit >>> 0, bits(scene.state.invalidPoint[0]), bits(scene.state.invalidPoint[1]), scene.types[123].flags2 >>> 0, scene.types[123].footX >>> 0, scene.types[123].footY >>> 0].join(',');
    return scene;
}

// 결과 두 float 의 저장 비트이다. (2026-11-08 WB04-E95)
const pointBits = (point) => `${bits(point[0])},${bits(point[1])}`;

test('CarrierDropPoint: 원본 10.78 742입력(Find 609+FindFootprint 133)의 결과 점·배치/중심 호출 순서·현재 전역·슬롯 raw 가 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    const scene = makeScene();
    let count = 0;
    // 이전 결과를 새 입력으로 쓰지 않고 매 행 전체 입력을 다시 공급한다.
    for (const row of rows) {
        assert.equal(row.length, 6);
        const input = row[0].split(',').map(Number);
        assert.equal(input.length, 18);
        if (input[C.op] === 2) continue;
        scene.prepare(input);
        const point = new RawCarrierDropPoint(scene.pool, scene.types, scene.state, scene.hooks);
        const result = input[C.op] === 0
            ? point.find(50, 123)
            : point.findFootprint(123, input[C.argument], bitsToFloat(input[C.x]), bitsToFloat(input[C.y]), input[C.queryFlags], input[C.queryOwner]);
        const label = `행 ${count}: 입력 ${row[0]}`;
        assert.equal(pointBits(result), row[1], label);
        assert.equal(scene.trace.length === 0 ? '-' : scene.trace.join(';'), row[2], label);
        assert.equal(scene.snapshot(), row[3], label);
        assert.equal(hex(scene.pool.slot(50)), row[4], label);
        count += 1;
    }
    assert.equal(count, 742);
});

test('CarrierDropPoint: 원본 10.78 Snap 256입력의 기본 53비트 결과가 같고 64비트 정밀도 열과 다른 31행도 구분된다', { skip: !rows && '원본 fixture 없음' }, () => {
    let count = 0;
    let different = 0;
    // 비유한 값·하위 DWORD 경계도 원문 비트로 대조한다.
    for (const row of rows) {
        const input = row[0].split(',').map(Number);
        if (input[C.op] !== 2) continue;
        const point = RawCarrierDropPoint.snap([bitsToFloat(input[C.x]), bitsToFloat(input[C.y])]);
        assert.equal(pointBits(point), row[1], `행 ${count}: 입력 ${row[0]}`);
        if (row[5] !== '-') { assert.notEqual(row[1], row[5]); different += 1; }
        count += 1;
    }
    assert.equal(count, 256);
    assert.equal(different, 31);
});

test('CarrierDropPoint: 누락 훅·판본·TYPE/SID 범위 오류는 해당 분기를 쓰는 시점에 거부하고 foot 0 은 실패점을 돌려준다', () => {
    const scene = makeScene();
    scene.prepare([0, bits(41.5), bits(51.0), 1, 0, 2, 3, 0, 0, 256, 0, 0, 0, 1, 0, 0, 0, 1]);
    const missing = new RawCarrierDropPoint(scene.pool, scene.types, scene.state, {});
    assert.ok(throws(() => missing.find(50, 123)));
    scene.types[123].flags2 = 0x100;
    assert.ok(throws(() => missing.find(50, 123)));
    assert.ok(throws(() => missing.find(50, 188)));
    assert.ok(throws(() => missing.find(24000, 123)));
    assert.ok(throws(() => missing.findFootprint(123, 0, 20, 30, 0, 1)));
    scene.types[123].footX = 0;
    assert.deepEqual(missing.findFootprint(123, 0, 20, 30, 0, 1), scene.state.invalidPoint);
    assert.ok(throws(() => new RawCarrierDropPoint({ edition: 'cd1072' }, scene.types, scene.state, {})));
    assert.ok(throws(() => new RawCarrierDropPoint(scene.pool, [], scene.state, {})));
    assert.equal(new RawCarrierDropPoint(scene.pool, scene.types, scene.state, {}).pool, scene.pool);
});
