// 공통 경로 도착 부모 raw-path-arrival-parent.js(pathArrivalBlocked·runPathBaseRobust·runPathArrivalParent)를 실제 CPU 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: path-arrival-parent-hardware-x86.tsv 860행 — checking·저장 부모·Form 번호·논리 용량·부모 상태·Halt 응답·도착 반환·삭제 입력·막힘 윤곽·변동(mutation 0~12)·감시 토큰·finished·도착 좌표에서
//    경로 소멸·checking·Form/부모 번호·용량·부모 상태·finished·토큰 존재·두 목표·부모 좌표/TYPE/소유자·robust 호출 수와 호출 순서(S Step/U Unpop/P Pop/Q robust/T Halt/R 도착/K kill/C 정리/A 진단)가 같다.
//    실제 일반 finder(E1 RawSquidFinder)와 실제 경로 이미지(E98 RawPathRuntime)를 사용하고, 원본 관측기가 기록한 finder 호출(F/N) 항목은 비교에서 제외한다(내부 관측 API 없음).
//  - 계약: 선택된 진단 장치만 요구하고 앞선 Step/공간 효과는 예외 뒤에도 남는다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E145: 공통 경로 도착 부모 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get, bitsToFloat } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { PATH_LAYOUT, RawPathRuntime } from '../www/core/raw-path-runtime.js';
import { pathArrivalBlocked, runPathArrivalParent, runPathBaseRobust } from '../www/core/raw-path-arrival-parent.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-09 WB04-E145)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'path-arrival-parent-hardware-x86.tsv')) ?? [];

// 관측 행 수와 물리 풀 크기이다. (2026-11-09 WB04-E145)
const ROWS = 860;
const CAPACITY = 24000;

// 예외가 나는지 본다. (2026-11-09 WB04-E145)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 물리 풀과 논리 상한을 구별해 원문의 범위 진단을 확인하는 장면이다. 결과는 fixture 에서 읽고 외부 장치의 응답만 구성한다. (2026-11-09 WB04-E145)
class Scene {
    constructor(row) {
        this.pool = new SidPool(CAPACITY, { server: false });
        this.hash = new SquidHash();
        this.runtime = new RawPathRuntime();
        this.types = Array.from({ length: 188 }, () => ({ flags1: 0, flags2: 0, footX: 0, footY: 0 }));
        this.spots = new Uint8Array(65536);
        this.alive = true;
        this.events = '';
        this.robustCalls = 0;
        this.mask = 0x200;
        this.checking = Number(row[1]);
        this.parent = Number(row[2]);
        this.capacity = Number(row[4]);
        this.halted = Number(row[6]);
        this.result = Number(row[7]);
        this.deletedInput = Number(row[8]);
        this.blockers = Number(row[9]);
        this.mutation = Number(row[10]);
        this.xb = Number(row[13]);
        this.yb = Number(row[14]);
        put(this.runtime.bytes, 4, Number(row[3]));
        put(this.runtime.bytes, 8, this.parent);
        this.runtime.setTargets(60, 61);
        put(this.runtime.bytes, PATH_LAYOUT.finished, Number(row[12]));
        this.runtime.setDeathWatchToken(Number(row[11]));
        // 범위 끝/예약 0 도 물리 메모리는 유효하다.
        for (const sid of [0, 5, 50, 70, 71, 90, 128]) {
            const raw = this.raw(sid);
            raw.fill(0);
            raw[10] = sid === 70 ? 2 : sid === 71 ? 3 : 1;
            raw[34] = 2;
            put(raw, 14, 0x3fa00000);
            put(raw, 18, 0xc0200000);
        }
        this.raw(this.parent)[11] = Number(row[5]);
        // 후보의 실제 발자국 입력이다.
        for (const type of [1, 2, 3]) this.types[type].footX = this.types[type].footY = 1;
        this.types[3].flags2 = this.mask;
        this.spots[21 * 256 + 11] = this.blockers ? 8 : 0;
        // 실제 finder 가 교차 판정할 양수 좌표이다.
        for (const sid of [70, 71]) {
            put(this.raw(sid), 14, this.xb);
            put(this.raw(sid), 18, this.yb);
        }
        this.hash.setCell(0, Math.trunc(bitsToFloat(this.xb)), Math.trunc(bitsToFloat(this.yb)), this.blockers === 4 ? 71 : this.blockers >= 2 ? 70 : 0);
        put(this.raw(70), 4, this.blockers === 3 ? 71 : 0, 2);
        this.hooks = this.makeHooks();
    }

    // 물리 SID 슬롯을 빌린다. (2026-11-09 WB04-E145)
    raw(sid) { return this.pool.physical(sid); }

    // 진단/도착 뒤 현재 metadata 입력이다. (2026-11-09 WB04-E145)
    changeParent(x, y, owner) {
        const raw = this.raw(this.parent);
        put(raw, 14, x);
        put(raw, 18, y);
        raw[10] = 2;
        raw[34] = owner;
    }

    // 원문 진단 복귀에서 현재 상태를 바꾼다. (2026-11-09 WB04-E145)
    assertion(expression, file, line) {
        this.events += `A,${line}:${expression}:${file};`;
        if (this.mutation === 10 || this.mutation === 11) {
            this.checking = this.mutation === 10 ? 0 : 0x80000000;
            put(this.runtime.bytes, 4, 5);
            put(this.runtime.bytes, 8, 50);
            this.runtime.setDeathWatchToken(0);
            this.runtime.setTargets(95, 96);
            this.raw(this.parent)[11] = 0;
            this.changeParent(0x40700000, 0x40900000, 7);
        }
        if (this.mutation === 12 && line === 0x69) {
            put(this.runtime.bytes, 4, 5);
            put(this.runtime.bytes, 8, 90);
            this.capacity = 64;
        }
    }

    // CPU 와 같은 외부 응답/현재 전역을 공급하는 경계 묶음이다. (2026-11-09 WB04-E145)
    makeHooks() {
        const hooks = {
            parent: () => get(this.runtime.bytes, 8),
            validAlive: (sid) => sid !== 0 && sid < this.capacity && (this.raw(sid)[11] & 3) === 0,
            alive: () => this.alive,
            step: () => {
                this.events += `S,${this.xb},${this.yb};`;
                if (this.mutation === 1) { this.checking = 0x80000000; put(this.runtime.bytes, 4, 128); }
                return { x: bitsToFloat(this.xb), y: bitsToFloat(this.yb) };
            },
            unpop: (sid) => { this.events += `U,${sid},0;`; },
            pop: (sid, point) => {
                const dv = new DataView(new ArrayBuffer(8));
                dv.setFloat32(0, point.x, true);
                dv.setFloat32(4, point.y, true);
                const x = dv.getUint32(0, true);
                const y = dv.getUint32(4, true);
                this.events += `P,${sid},${x},${y},0;`;
                put(this.raw(sid), 14, x);
                put(this.raw(sid), 18, y);
                if (this.mutation === 2) { this.checking = 0; put(this.runtime.bytes, 4, 128); }
                if (this.mutation === 3) this.raw(sid)[11] = 3;
            },
            assertion: (expression, file, line) => this.assertion(expression, file, line),
            robust: () => {
                this.robustCalls += 1;
                this.events += `Q,${get(this.runtime.bytes, 4)},${get(this.runtime.bytes, 8)};`;
                assert.equal(runPathBaseRobust({ form: () => get(this.runtime.bytes, 4), parent: () => get(this.runtime.bytes, 8), capacity: () => this.capacity, assertion: hooks.assertion }), true);
            },
            blocked: (point) => pathArrivalBlocked(this.pool, this.hash, this.types, this.spots, this.mask, point),
            halt: () => {
                this.events += `T,${this.halted};`;
                this.runtime.setDeathWatchToken(0);
                if (this.mutation === 4) this.checking = 0;
                if (this.mutation === 5) this.checking = 0x80000000;
                if (this.mutation === 6) this.runtime.setDeathWatchToken(0x12345678);
                if (this.mutation === 7) { this.runtime.setTargets(85, 86); this.changeParent(0x40700000, 0x40900000, 7); }
                return this.halted !== 0;
            },
            finished: (value) => this.runtime.setFinished(value),
            deadOrFree: (sid) => (this.raw(sid)[11] & 3) !== 0,
            watchPresent: () => this.runtime.deathWatchToken !== 0,
            armWatch: () => this.runtime.setDeathWatchToken(1),
            clearWatch: () => this.runtime.setDeathWatchToken(0),
            arrival: (sid) => {
                this.events += `R,${sid},${this.runtime.targetA},${this.runtime.targetB},${this.result};`;
                if (this.mutation === 8) { this.checking = 0; this.changeParent(0x40b00000, 0x40d00000, 6); }
                if (this.mutation === 9) { this.checking = 0x80000000; put(this.runtime.bytes, 4, 128); }
                if (this.deletedInput) this.alive = false;
                return this.result;
            },
            kill: () => { this.events += `K,${get(this.runtime.bytes, 4)},0;`; this.alive = false; },
            clear: (sid) => {
                const raw = this.raw(sid);
                this.events += `C,${get(raw, 14)},${get(raw, 18)},${raw[10]},${raw[34]};`;
            },
        };
        return hooks;
    }

    // 관측기가 삭제한 payload 도 출력만 보존하며 실제 Host 삭제는 별도 검사한다. (2026-11-09 WB04-E145)
    run() {
        runPathArrivalParent(() => this.checking, this.hooks);
        const raw = this.raw(this.parent);
        return [String(this.alive ? 0 : 1), String(this.checking >>> 0), String(get(this.runtime.bytes, 4)), String(get(this.runtime.bytes, 8)), String(this.capacity), String(raw[11]),
            String(get(this.runtime.bytes, PATH_LAYOUT.finished)), String(this.runtime.deathWatchToken !== 0 ? 1 : 0), String(this.runtime.targetA), String(this.runtime.targetB),
            String(get(raw, 14)), String(get(raw, 18)), String(raw[10]), String(raw[34]), String(this.robustCalls), this.events];
    }
}

// 실제 finder 는 내부 관측 API 가 없으므로 명시 경계의 F/N 요청만 비교에서 제외한다. (2026-11-09 WB04-E145)
function arrivalTrace(trace) {
    return trace.split(';').filter((event) => event && event[0] !== 'F' && event[0] !== 'N').map((event) => `${event};`).join('');
}

test('경로 도착 부모: 실제 CPU 관측 860행의 현재 필드·진단·도착·제거와 실제 finder 판정이 같다', { skip: rows.length === 0 }, () => {
    assert.equal(rows.length, ROWS);
    for (const row of rows) {
        assert.equal(row.length, 33);
        const actual = new Scene(row).run();
        const expected = [...row.slice(15, 30), arrivalTrace(row[32])];
        assert.deepEqual(actual, expected, `행 ${row[0]}`);
    }
});

test('경로 도착 부모: 선택된 진단 장치만 요구하고 앞선 Step/공간 효과는 예외 뒤에도 남는다', { skip: rows.length === 0 }, () => {
    const scene = new Scene(rows[0]);
    scene.result = 1003;
    scene.hooks.assertion = undefined;
    assert.ok(throws(() => scene.run()));
    assert.ok(scene.events.startsWith('S,'));
    assert.ok(scene.events.includes('R,50,60,61,1003;'));
    assert.ok(scene.alive);
    scene.hooks.assertion = (expression, file, line) => { assert.equal(line, 0x28d); scene.checking = 0; };
    scene.events = '';
    scene.run();
    assert.ok(!scene.alive);
    assert.ok(scene.events.includes('K,5,0;'));
});

test('경로 도착 부모: 지도 밖 도착점은 거부하고 기준 robust 는 항상 참이다', () => {
    const pool = new SidPool(CAPACITY, { server: false });
    const hash = new SquidHash();
    const types = Array.from({ length: 188 }, () => ({ flags1: 0, flags2: 0, footX: 0, footY: 0 }));
    const spots = new Uint8Array(65536);
    assert.ok(throws(() => pathArrivalBlocked(pool, hash, types, spots, 0x200, { x: -1, y: 5 })));
    assert.ok(throws(() => pathArrivalBlocked(pool, hash, types, spots, 0x200, { x: 5, y: 256 })));
    assert.ok(throws(() => pathArrivalBlocked(pool, hash, types, spots, 0x200, { x: NaN, y: 5 })));
    // spot 도착 비트가 없으면 finder 를 돌리지 않고 거짓이다.
    assert.equal(pathArrivalBlocked(pool, hash, types, spots, 0x200, { x: 10, y: 10 }), false);
    const diagnostics = [];
    assert.equal(runPathBaseRobust({ form: () => 70, parent: () => 80, capacity: () => 64, assertion: (expression, file, line) => diagnostics.push([expression, file, line]) }), true);
    assert.deepEqual(diagnostics.map((item) => item[2]), [0x69, 0x6c]);
    assert.ok(throws(() => runPathBaseRobust({ form: () => 70, parent: () => 0, capacity: () => 64 })));
});
