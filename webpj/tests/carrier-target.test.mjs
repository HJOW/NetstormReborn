// 운반 대상 접근 raw-board-target.js(RawBoardTarget)·raw-carrier-target.js(RawCarrierTarget)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: carrier-target-x86.tsv 2245행(입력 22 DWORD — 연산 0 변 중간점/1 일반 접근점/2 대상 접근/3 귀환/4 간헐천/5 집으로/6 초기 실패점·사각형·좌표·flags·AI 소유자·상태·재시도 성공 순번·변이 4종 등)과
//    실제 CPU 53/64비트 x87 정밀도로 측정한 기하 결과 carrier-target-hardware.tsv 2040행(연산 0/1 의 1020행은 이 실제 CPU 열을 기대값으로 사용하고 두 정밀도가 같음을 확인).
//    비교 항목: 외부 효과 순서(! 진단/R 발자국/S 경로 시작 8값/C 귀환 후보), 상태(반환 주소·직접 결과 두 DWORD·실패점 두 DWORD·boss·시도 횟수), 두 슬롯(부모 50·대상 60) 전체 raw.
//  - 계약: 조기 반환이 필요 없는 하위를 요구하지 않고 AI 판정은 현재 Player 포인터를 직접 읽는다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E100: 운반 대상 접근 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, bitsToFloat, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { BoardTargetState, RawBoardTarget } from '../www/core/raw-board-target.js';
import { RawCarrierTarget } from '../www/core/raw-carrier-target.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E100)
const fixtures = path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures');
const rows = readRows(path.join(fixtures, 'carrier-target-x86.tsv'));
const hardwareRows = readRows(path.join(fixtures, 'carrier-target-hardware.tsv'));

// 예외가 나는지 본다. (2026-11-08 WB04-E100)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

const hex = (bytes) => Buffer.from(bytes).toString('hex');
const bits = (value) => floatToBits(value);
const numbers = (text) => text.split(',').map((value) => Number(BigInt.asUintN(32, BigInt(value))));

// JS Number 는 signaling NaN 의 quiet 비트를 보존하지 못한다(원본은 실패점 복사 때 0x7f800001 을 그대로 두고 JS 는 0x7fc00001 이 된다).
// 두 쪽 문자열의 십진 토큰을 같은 규칙(지수 전부 1·가수 있음·quiet 비트 없음이면 quiet 비트 설정)으로 정규화해 NaN 계열 일치만 확인한다. (2026-11-08 WB04-E100)
function quiet(text) {
    return text.replace(/[0-9]+/g, (token) => {
        const value = Number(token);
        if (value > 0xffffffff || (value & 0x7f800000) !== 0x7f800000 || !(value & 0x7fffff) || (value & 0x400000)) return token;
        return String((value | 0x400000) >>> 0);
    });
}

// 발자국/실제 경로 시작/귀환 후보만 외부 경계로 제공하는 장면이다. (2026-11-08 WB04-E100)
function makeScene() {
    const scene = {
        pool: new SidPool(24000, { server: true }),
        types: Array.from({ length: 188 }, () => ({ flags2: 0 })),
        players: new Uint8Array(9 * 0xb4),
        boss: false,
        state: new BoardTargetState(),
        row: [],
        out: [0xa5a5a5a5, 0xa5a5a5a5],
        attempts: 0,
        trace: []
    };
    const event = (name, values) => scene.trace.push(values.reduce((text, value) => `${text}:${value >>> 0}`, name));
    const raw = (sid) => scene.pool.physical(sid);
    // 입력 사각형만 읽고 접근점은 계산하지 않는다.
    scene.rectangle = () => ({ left: bitsToFloat(scene.row[1]), top: bitsToFloat(scene.row[2]), right: bitsToFloat(scene.row[3]), bottom: bitsToFloat(scene.row[4]) });
    // 현재 raw/AI/genus/실패점 입력을 원문과 같게 설치한다.
    scene.prepare = (input) => {
        const row = input;
        scene.row = row;
        assert.equal(row.length, 22);
        scene.boss = row[16] !== 0;
        scene.state.invalid = { x: bitsToFloat(row[19]), y: bitsToFloat(row[20]) };
        scene.out = [0xa5a5a5a5, 0xa5a5a5a5];
        scene.attempts = 0;
        scene.trace = [];
        scene.players.fill(0);
        put(scene.players, row[12] * 0xb4 + 0xb0, row[13]);
        scene.types[122].flags2 = row[15];
        // 원문 부모와 대상의 전체 초기 raw 를 보존한다.
        for (const sid of [50, 60]) {
            const slot = raw(sid);
            slot.fill(0xa5);
            slot[10] = sid === 50 ? 133 : 122;
            slot[11] = sid === 60 ? row[14] : 0;
            slot[34] = sid === 50 ? row[12] : 2;
            put(slot, 14, row[5]);
            put(slot, 18, row[6]);
        }
    };
    // 호출 시에만 진단을 요구하며 원본의 현재 실패점 재조회를 관측한다.
    scene.diagnostic = (line) => {
        event('!', [line]);
        if (scene.row[21]) scene.state.invalid = { x: 42.125, y: bitsToFloat(0x7f800001) };
    };
    // 반환을 대신 계산하지 않고 명시 큰 하위 효과만 제공한다.
    scene.hooks = {
        footprint: (sid) => {
            event('R', [sid, 0]);
            if (scene.row[18] === 1) { put(raw(50), 14, bits(42.25)); put(raw(50), 18, bits(43.5)); }
            return scene.rectangle();
        },
        start: (request) => {
            event('S', [request.parent, bits(request.destination.x), bits(request.destination.y), request.replaceStart, request.helper, request.tailWord, request.tailByte, request.reserved]);
            scene.attempts += 1;
            if (scene.row[18] === 2) {
                put(raw(50), 14, bits(100)); put(raw(50), 18, bits(101));
                put(raw(60), 14, bits(200)); put(raw(60), 18, bits(201));
            }
            return scene.row[11] === scene.attempts ? 0x14600000 + scene.attempts * 40 : 0;
        },
        chooseReturn: (sid) => {
            event('C', [sid]);
            if (scene.row[18] === 3) { scene.boss = false; put(raw(50), 14, bits(21.75)); put(raw(50), 18, bits(8.5)); }
            return scene.row[17];
        }
    };
    // 원문 순서의 현재 결과와 전역/횟수이다.
    scene.status = (result) => [result, scene.out[0], scene.out[1], bits(scene.state.invalid.x), bits(scene.state.invalid.y), scene.boss ? 1 : 0, scene.attempts].map(String).join(',');
    return scene;
}

test('CarrierTarget: 원본 10.78 2245입력(기하 1020은 실제 CPU 열)의 반환·호출 인자·실패점·두 슬롯 raw 가 같다', { skip: (!rows || !hardwareRows) && '원본 fixture 없음' }, () => {
    const scene = makeScene();
    const hardware = new Map();
    // 실제 CPU 자료를 별도로 읽으며 에뮬레이터 원문을 덮어쓰지 않는다.
    for (const row of hardwareRows) {
        assert.equal(row.length, 5);
        hardware.set(`${Number(row[0])}:${Number(row[1])}`, row);
    }
    assert.equal(hardware.size, 2040);
    let count = 0;
    let geometry = 0;
    // 두 정밀도 원문을 보존하며 기하는 실제 CPU 의 53비트 결과로 대조한다.
    for (const fields of rows) {
        assert.equal(fields.length, 7);
        scene.prepare(numbers(fields[0]));
        const board = new RawBoardTarget(scene.state, scene.diagnostic);
        const target = new RawCarrierTarget(scene.pool, scene.types, scene.players, () => scene.boss, board, scene.hooks);
        const row = scene.row;
        let result = 0;
        if (row[0] === 6) {
            const initialized = new BoardTargetState();
            const native = numbers(fields[2]);
            assert.ok(native[3] === bits(initialized.invalid.x) && native[4] === bits(initialized.invalid.y));
            count += 1;
            continue;
        }
        if (row[0] < 2) {
            const point = row[0] === 0
                ? board.side(scene.rectangle(), row[8], row[7])
                : board.nearest(scene.rectangle(), { x: bitsToFloat(row[5]), y: bitsToFloat(row[6]) }, row[7]);
            scene.out = [bits(point.x), bits(point.y)];
        } else if (row[0] === 2) result = target.startTarget(50, 60, row[9], row[10]);
        else if (row[0] === 3) result = target.goReturn(50, 60);
        else if (row[0] === 4) result = target.goGeyser(50, 60);
        else result = target.goHome(50);
        const trace = scene.trace.length === 0 ? '-' : scene.trace.join(';');
        const state = scene.status(result);
        const raws = `${hex(scene.pool.slot(50))},${hex(scene.pool.slot(60))}`;
        let expectedTrace = fields[1];
        let expectedState = fields[2];
        // 기하는 실제 CPU, 나머지 상위는 기존 독립 에뮬레이터 관측을 사용한다.
        if (row[0] < 2) {
            const measured = hardware.get(`${count}:${0x27f}`);
            const extended = hardware.get(`${count}:${0x37f}`);
            assert.ok(measured[2] === extended[2] && measured[3] === extended[3]);
            const old = fields[2].split(',');
            expectedTrace = measured[2];
            expectedState = `${old[0]},${measured[3]},${old[5]},${old[6]}`;
            geometry += 1;
        }
        const label = `행 ${count}: 입력 ${fields[0]}`;
        assert.equal(quiet(trace), quiet(expectedTrace), label);
        assert.equal(quiet(state), quiet(expectedState), label);
        assert.equal(raws, fields[3], label);
        count += 1;
    }
    assert.equal(count, 2245);
    assert.equal(geometry, 1020);
});

test('CarrierTarget: 조기 반환은 필요 없는 하위를 요구하지 않고 AI 판정은 현재 Player 포인터를 직접 읽는다', () => {
    const scene = makeScene();
    const pointState = new BoardTargetState();
    const board = new RawBoardTarget(pointState);
    const target = new RawCarrierTarget(scene.pool, scene.types, scene.players, () => scene.boss, board, {});
    assert.equal(target.goHome(50), 0);
    scene.pool.physical(50)[34] = 1;
    put(scene.players, 0xb4 + 0xb0, 1);
    assert.equal(target.goReturn(50, 60), 0);
    assert.equal(target.goGeyser(50, 60), 0);
    put(scene.players, 0xb4 + 0xb0, 0);
    scene.pool.physical(60)[11] = 1;
    assert.equal(target.goGeyser(50, 60), 0);
    assert.equal(target.pool, scene.pool);
    assert.ok(throws(() => target.goReturn(50, 60)));
    assert.ok(throws(() => board.side({ left: 1, top: 2, right: 3, bottom: 4 }, 1, 1)));
    assert.ok(throws(() => new RawCarrierTarget(scene.pool, scene.types, new Uint8Array(0), () => false, board, {})));
    assert.ok(throws(() => new RawCarrierTarget({ edition: 'cd1072' }, scene.types, scene.players, () => false, board, {})));
});
