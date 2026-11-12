// 비행체 표적 검색 raw-flyer-target.js(RawFlyerTarget)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: flyer-target-x86.tsv 1,302행 — 가까운 비행체 수 C 334행(현재 거리·상태·종속 타입과 선저장 next), 자리 수 P 60행(free/dead·1×1 경계), 자리 번호 I 334행(의존 순서와 SID 수치 차이), 포위 위치 S 334행(세 오프셋과 자리 1 의 (0, 0)),
//    표적 검색 F 240행(출발점·검색 사각형·필터·엄격한 거리·귀환 타이머)이 필드 112바이트·객체 22개 1,100바이트·전역 28바이트·결과·호출 순서까지 같다. cpppj FlyerTargetTests.cpp 의 FlyerTargetScene 을 같은 순서로 옮겼다(종속 검사는 E355 RawFlyerFrame 을 쓴다).
//  - cpppj 시험 이식: 선택 누락과 부분 쓰기 보존(FlyerTarget_RequiredBoundaries).
//  - 웹 계약: 생성자 검사, 훅 누락 문구, TYPE 표의 살아 있는 참조와 사거리 표 사본, 절삭 범위 오류, 표 밖 TYPE 번호.
// 10.72(CD)는 범위 밖이다. 실제 게임·브라우저 실행이 아니다. (2026-11-13 WB04-E356: 7-P1 RawFlyerTarget 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get, bitsToFloat, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { FlyerProcessFields } from '../www/core/raw-flyer-process.js';
import { RawFlyerFrame } from '../www/core/raw-flyer-frame.js';
import { RawFlyerTarget } from '../www/core/raw-flyer-target.js';

// 원본 관측 행이다(첫 행은 열 이름). 없으면 해당 검사를 건너뛴다. (2026-11-13 WB04-E356)
const rows = (() => {
    const all = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'flyer-target-x86.tsv'));
    return all ? all.slice(1) : null;
})();

// 원문과 같은 전체 22객체 순서와 실제 입력 TYPE 번호이다. (2026-11-13 WB04-E356)
const TARGET_IDS = [50, 51, 52, 53, 60, 61, 62, 63, 70, 71, 72, 73, 80, 81, 82, 83, 84, 90, 91, 92, 93, 94];
const TARGET_TYPES = [1, 6, 9, 31, 63, 73, 80, 81, 82, 83, 144];

// 예외가 나는지 본다. (2026-11-13 WB04-E356)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 바이트 배열의 소문자 16진 문자열이다. (2026-11-13 WB04-E356)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 원문과 같은 입력만 설치하며 선택 결과와 기대 좌표를 미리 계산하지 않는 장면이다(cpppj FlyerTargetScene). (2026-11-13 WB04-E356)
class FlyerTargetScene {
    constructor(kind, v) {
        this.pool = new SidPool(24000, { server: false });
        this.types = Array.from({ length: 188 }, () => ({ cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0 }));
        this.ranges = new Array(188).fill(0);
        this.frameDetails = Array.from({ length: 188 }, () => ({ range: 0, sound: 0 }));
        this.stamps = new Uint32Array(188);
        this.fields = new FlyerProcessFields();
        this.server = true;
        this.checking = false;
        this.boss = true;
        this.now = 100;
        this.sun = 73;
        this.end = 63;
        this.depend = 1;
        this.wind = 138;
        this.rain = 144;
        this.sunProcess = 32;
        this.soundProcess = 47;
        this.frame = 0;
        this.mutation = 0;
        this.may = 15;
        this.index = 0;
        this.candidates = [60, 61, 62, 63];
        this.trace = [];
        // 관찰 번호를 실제 풀에서 확보한다.
        for (;;) {
            const sid = this.pool.allocate(2);
            if (sid === 94) break;
            if (!sid || sid > 94) throw new Error('비행 표적 검사 SID 확보 오류');
        }
        // 결과와 독립된 실제 TYPE 입력이다.
        for (const type of TARGET_TYPES) {
            this.types[type].footX = 2;
            this.types[type].footY = 2;
            this.types[type].flags2 = (type === 73 || type === 144) ? 0x100000 : 0;
            this.ranges[type] = 30;
        }
        // 전체 50바이트를 원문과 같은 패턴으로 준비한다.
        for (const sid of TARGET_IDS) {
            const raw = this.pool.allocated(sid);
            raw.fill(0xa5);
            put(raw, 0, 0x50a388);
            put(raw, 4, 0, 2);
            put(raw, 6, 0, 2);
            put(raw, 8, 0, 2);
            raw[10] = 144;
            raw[11] = 0;
            put(raw, 14, floatToBits(50.25));
            put(raw, 18, floatToBits(50.5));
            raw[34] = 1;
            raw[40] = 0;
            put(raw, 36, 0);
            put(raw, 42, 0);
            put(raw, 46, 0);
        }
        // 각 부모/후보/프로세스 Form 의 독립 명시 연결이다.
        for (let i = 0; i < 4; i += 1) {
            let raw = this.pool.allocated(50 + i);
            put(raw, 6, 70 + i, 2);
            put(raw, 14, floatToBits([50.25, 55.25, 70.25, 58.25][i]));
            raw[10] = i === 3 ? 80 : 144;
            raw = this.pool.allocated(60 + i);
            raw[10] = 80 + i;
            put(raw, 14, floatToBits([60.25, 55.25, 52.25, 80.25][i]));
            put(raw, 18, floatToBits(50.5));
            raw = this.pool.allocated(70 + i);
            put(raw, 0, 0x512098);
            raw[10] = 31;
            raw[11] = 8;
            put(raw, 16, 50 + i, 2);
        }
        // 감시 쌍의 반대 Form 부모는 실제 process Form 이다.
        for (let i = 0; i < 5; i += 1) {
            let raw = this.pool.allocated(80 + i);
            put(raw, 0, 0x512110);
            raw[10] = i === 4 ? 6 : 1;
            raw[11] = 8;
            put(raw, 18, 90 + i, 2);
            raw = this.pool.allocated(90 + i);
            put(raw, 0, 0x512110);
            raw[10] = 1;
            raw[11] = 8;
            put(raw, 16, i < 4 ? 70 + i : 0, 2);
        }
        this.fields.bytes.fill(0xa5);
        put(this.fields.bytes, 0, 0x509f60);
        this.fields.bytes[8] = 0x10;
        put(this.fields.bytes, 10, 61, 2);
        put(this.fields.bytes, 12, 0, 2);
        this.timeAt(40, 0);
        put(this.fields.bytes, 48, 0x509f40);
        put(this.fields.bytes, 52, 70);
        put(this.fields.bytes, 56, 50);
        // 순회 순서와 숫자 순서가 다른 의존 입력이다.
        let chain = [84, 82, 80, 81, 83];
        if (kind === 'F') {
            const scene = Number(v[0]);
            const origin = Number(v[1]);
            const parentType = Number(v[3]);
            this.server = Number(v[8]) !== 0;
            this.checking = Number(v[9]) !== 0;
            this.may = Number(v[5]);
            this.mutation = Number(v[11]);
            this.pool.allocated(50)[10] = parentType;
            this.ranges[parentType] = bitsToFloat(Number(v[2]));
            put(this.fields.bytes, 10, Number(v[4]), 2);
            this.timeAt(40, [0, 99, 101][Number(v[10])]);
            this.types[80].flags2 = Number(v[6]);
            this.types[80].footX = this.types[80].footY = Number(v[7]) | 0;
            const point = [[0, 0], [48, 49], [0, 49], [-2, -3], [17000001, 17000003]][origin];
            put(this.pool.allocated(50), 42, point[0]);
            put(this.pool.allocated(50), 46, point[1]);
            if (scene === 1) this.candidates.reverse();
            if (scene === 2) this.candidates = [];
            if (scene === 3) this.candidates = [60, 60, 62];
            if (scene === 4) put(this.pool.allocated(61), 14, floatToBits(60.25));
            if (scene === 5) {
                this.candidates = [60];
                put(this.pool.allocated(60), 14, floatToBits(79.25));
            }
            if (scene === 6) this.candidates = [61];
            if (scene === 7) this.candidates = [63];
            if (scene === 8) this.candidates = [62, 60, 61];
        } else {
            this.types[80].footX = Number(v[2]) | 0;
            this.types[80].footY = Number(v[3]) | 0;
            this.checking = Number(v[7]) !== 0;
            this.mutation = Number(v[8]);
            const scene = Number(v[6]);
            if (scene === 1) chain.reverse();
            if (scene === 2) chain = [];
            if (scene === 3) chain = chain.filter((value) => value !== 80);
            if (scene === 4) chain = [80];
            if (scene === 5) put(this.pool.allocated(80), 18, 0, 2);
            if (scene === 6) this.pool.allocated(71)[10] = 9;
            if (scene === 7) this.pool.allocated(71)[10] = 63;
            if (scene === 8) this.pool.allocated(51)[10] = 80;
            if (scene === 9) this.pool.allocated(51)[11] = 2;
            if (scene === 10) put(this.pool.allocated(51), 14, floatToBits(50.25));
            if (scene === 11) put(this.pool.allocated(51), 14, floatToBits(50.250003814697266));
            if (scene === 12) this.end = 31;
            if (scene === 13) put(this.pool.allocated(51), 6, 0, 2);
            if (scene === 14) {
                this.pool.allocated(50)[10] = 73;
                this.pool.allocated(52)[10] = 73;
            }
        }
        put(this.pool.allocated(60), 6, chain.length === 0 ? 0 : chain[0], 2);
        // 원문과 같은 현재 next 입력이다.
        for (let i = 0; i < chain.length; i += 1) put(this.pool.allocated(chain[i]), 4, i + 1 < chain.length ? chain[i + 1] : 0, 2);
        // free/dead 입력은 전체 링크 준비 뒤 마지막에 설치한다.
        if (kind !== 'F') {
            this.pool.allocated(60)[11] = Number(v[4]) & 0xff;
            this.pool.allocated(50)[11] = Number(v[5]) & 0xff;
        }
        this.checks = new RawFlyerFrame(this.pool, this.types, this.frameDetails, this.stamps, {
            server: () => this.server, boss: () => this.boss, checking: () => (this.checking ? 1 : 0), now: () => this.now, frame: () => this.frame,
            sunType: () => this.sun, windType: () => this.wind, rainType: () => this.rain, sunAviaryProcess: () => this.sunProcess, soundProcess: () => this.soundProcess,
        }, {});
        this.body = new RawFlyerTarget(this.pool, this.types, this.ranges, this.globals(), this.hooks());
    }

    // 원문 double 입력의 비트만 쓴다. (2026-11-13 WB04-E356)
    timeAt(at, value) {
        const view = new DataView(new ArrayBuffer(8));
        view.setFloat64(0, value, true);
        put(this.fields.bytes, at, view.getUint32(0, true));
        put(this.fields.bytes, at + 4, view.getUint32(4, true));
    }

    // 선택된 실제 효과 순서만 기록한다. (2026-11-13 WB04-E356)
    event(value) { this.trace.push(value); }

    // 같은 현재 값을 매번 읽는 전역 함수 묶음이다. (2026-11-13 WB04-E356)
    globals() {
        return {
            server: () => this.server,
            checking: () => (this.checking ? 1 : 0),
            now: () => this.now,
            sunType: () => this.sun,
            processEnd: () => this.end,
            dependType: () => this.depend,
        };
    }

    // 원문 경계에 공급한 현재 입력 변경만 수행한다. (2026-11-13 WB04-E356)
    mutate(where) {
        const pool = this.pool;
        if (where === 'Offset' && this.mutation === 1) {
            put(this.fields.bytes, 56, 51);
            put(pool.allocated(50), 14, floatToBits(71.25));
            put(pool.allocated(50), 18, floatToBits(72.5));
        }
        if (where === 'May' && this.mutation === 2) {
            put(this.fields.bytes, 10, 0, 2);
            put(this.fields.bytes, 56, 51);
            pool.allocated(50)[10] = 73;
            this.types[80].flags2 = 0x120000;
        }
        if (where === 'Begin' && this.mutation === 3) this.sun = 144;
        if (where === 'Next' && this.mutation === 4) {
            this.now = 200;
            put(pool.allocated(62), 14, floatToBits(49.25));
        }
        if (where === 'Process' && this.mutation === 5) put(pool.allocated(51), 14, floatToBits(49.75));
        if (where === 'Watch' && this.mutation === 6) {
            put(pool.allocated(82), 4, 0, 2);
            put(pool.allocated(71), 16, 52, 2);
        }
        if (where === 'Command' && this.mutation === 7) {
            put(this.fields.bytes, 12, 0, 2);
            put(this.fields.bytes, 56, 51);
            this.now = 200;
        }
        if (where === 'Delay' && this.mutation === 8) {
            this.now = 200;
            this.timeAt(40, 777);
        }
        if (where === 'Check' && this.mutation === 9) {
            put(pool.allocated(50), 42, 17);
            put(pool.allocated(50), 46, 18);
        }
    }

    // 원문에 선언한 큰 외부 반환/변경만 공급하는 훅 묶음이다. (2026-11-13 WB04-E356)
    hooks() {
        return {
            assertion: (line) => { this.event(`Assert,${line}`); },
            checkForms: (sid) => { this.event(`Check,${sid}`); this.mutate('Check'); return this.checks.checkForms(sid); },
            offset: (sid) => { this.event(`Offset,${sid}`); this.mutate('Offset'); return [1, 0]; },
            begin: (area, flags) => {
                this.event(`Begin,${area.left},${area.top},${area.right},${area.bottom},${flags}`);
                this.mutate('Begin');
                this.index = 0;
                return this.candidates.length === 0 ? 0 : this.candidates[0];
            },
            next: () => {
                this.event('Next');
                this.mutate('Next');
                this.index += 1;
                return this.index < this.candidates.length ? this.candidates[this.index] : 0;
            },
            mayTarget: (parent, target) => {
                this.event(`May,${parent},${target}`);
                this.mutate('May');
                return (this.may & (1 << (target - 60))) !== 0;
            },
            watcher: (form) => {
                this.event(`Watch,${form}`);
                this.mutate('Watch');
                const other = get(this.pool.slot(form), 18, 2);
                return other ? get(this.pool.slot(other), 16, 2) : 0;
            },
            processParent: (form) => { this.event(`Process,${form}`); this.mutate('Process'); return get(this.pool.slot(form), 16, 2); },
            returnDelay: () => { this.event('Delay'); this.mutate('Delay'); return 10000000; },
            command: (sid, command) => { this.event(`Command,${sid},${command}`); this.mutate('Command'); },
        };
    }

    // 원문 22객체 전체 1100바이트이다. (2026-11-13 WB04-E356)
    objects() { return TARGET_IDS.map((sid) => hex(this.pool.slot(sid))).join(''); }

    // 현재 전역 28바이트이다. (2026-11-13 WB04-E356)
    globalsImage() {
        const raw = new Uint8Array(28);
        const view = new DataView(new ArrayBuffer(8));
        view.setFloat64(0, this.now, true);
        put(raw, 0, this.server ? 1 : 0);
        put(raw, 4, this.checking ? 1 : 0);
        put(raw, 8, view.getUint32(0, true));
        put(raw, 12, view.getUint32(4, true));
        put(raw, 16, this.sun);
        put(raw, 20, this.end);
        put(raw, 24, this.depend);
        return hex(raw);
    }
}

// 독립 범위별 전수 대조로 한 실패가 다른 원문 증거를 감추지 않게 한다. (2026-11-13 WB04-E356)
function compareTarget(kind, expected) {
    let compared = 0;
    // 헤더 이외의 선택한 원본 입력만 실행한다.
    for (const row of rows) {
        assert.equal(row.length, 7);
        if (row[0] !== kind) continue;
        compared += 1;
        const input = row[1].split(',');
        const scene = new FlyerTargetScene(kind, input);
        let result = '-';
        if (kind === 'F') scene.body.find(scene.fields);
        else {
            const target = Number(input[0]);
            const flyer = Number(input[1]);
            if (kind === 'C') result = String(scene.body.closerCount(target, flyer));
            if (kind === 'P') result = String(scene.body.capacity(target));
            if (kind === 'I') result = String(scene.body.slotIndex(target, flyer));
            if (kind === 'S') {
                const point = scene.body.scatter(target, flyer);
                put(scene.fields.bytes, 16, floatToBits(point[0]));
                put(scene.fields.bytes, 20, floatToBits(point[1]));
            }
        }
        const where = `${kind}/${row[1]}`;
        assert.equal(hex(scene.fields.bytes), row[2], `${where} 필드`);
        assert.equal(scene.objects(), row[3], `${where} 객체`);
        assert.equal(scene.globalsImage(), row[4], `${where} 전역`);
        assert.equal(result, row[5], `${where} 결과`);
        assert.equal(scene.trace.length === 0 ? '-' : scene.trace.join(';'), row[6], `${where} 호출 순서`);
    }
    assert.equal(compared, expected);
}

test('FlyerTarget: 원본 10.78 가까운 비행체 수 334행 — 현재 거리/상태/종속 타입과 선저장 next 가 같다', { skip: !rows && '원본 fixture 없음' }, () => { compareTarget('C', 334); });
test('FlyerTarget: 원본 10.78 자리 수 60행 — free/dead·1×1/그 밖 경계가 같다', { skip: !rows && '원본 fixture 없음' }, () => { compareTarget('P', 60); });
test('FlyerTarget: 원본 10.78 자리 번호 334행 — 의존 순서와 SID 수치의 차이가 같다', { skip: !rows && '원본 fixture 없음' }, () => { compareTarget('I', 334); });
test('FlyerTarget: 원본 10.78 포위 위치 334행 — 세 오프셋과 자리 1 의 (0, 0)이 같다', { skip: !rows && '원본 fixture 없음' }, () => { compareTarget('S', 334); });
test('FlyerTarget: 원본 10.78 표적 검색 240행 — 출발점·검색 사각형·필터·엄격한 거리·타이머가 같다', { skip: !rows && '원본 fixture 없음' }, () => { compareTarget('F', 240); });

test('FlyerTarget: 선택 누락은 앞선 쓰기를 남기고 거부하며 아직 선택하지 않은 경계는 요구하지 않는다', () => {
    const scene = new FlyerTargetScene('F', '2,0,1106247680,144,61,15,0,2,1,0,0,0'.split(','));
    const hooks = scene.hooks();
    delete hooks.next;
    const noNext = new RawFlyerTarget(scene.pool, scene.types, scene.ranges, scene.globals(), hooks);
    noNext.find(scene.fields);
    assert.equal(get(scene.fields.bytes, 12, 2), 0);
    const fresh = new FlyerTargetScene('F', '2,0,1106247680,144,61,15,0,2,1,0,0,0'.split(','));
    const freshHooks = fresh.hooks();
    delete freshHooks.begin;
    const missing = new RawFlyerTarget(fresh.pool, fresh.types, fresh.ranges, fresh.globals(), freshHooks);
    const before = Uint8Array.from(fresh.fields.bytes);
    assert.ok(throws(() => missing.find(fresh.fields)));
    assert.ok(fresh.fields.bytes[8] & 4);
    assert.notEqual(get(fresh.pool.slot(50), 42), 0);
    assert.notDeepEqual(fresh.fields.bytes, before);
});

// 표적 검색(F) 장면의 기본 입력이다: 후보 60..63, 출발점 (48, 49), 사거리 30.0f, 부모 TYPE 144, 홈 61, 발사 가능 15, 80번 flags2 0, 발자국 2, 서버, 진단 꺼짐, 귀환 시각 0, 변이 0. (2026-11-13 WB04-E356)
const FIND_INPUT = ['0', '1', '1106247680', '144', '61', '15', '0', '2', '1', '0', '0', '0'];

// 기본 입력에서 호출자가 지정한 칸(색인 → 값)만 바꾼 표적 검색 장면을 만든다. (2026-11-13 WB04-E356)
function findScene(overrides = {}) {
    const input = FIND_INPUT.slice();
    for (const [index, value] of Object.entries(overrides)) input[Number(index)] = String(value);
    return new FlyerTargetScene('F', input);
}

// 장면의 훅을 호출자가 바꾼 본체를 만든다(원본 장면과 같은 풀·표를 쓴다). (2026-11-13 WB04-E356)
function bodyOf(scene, change) {
    const hooks = scene.hooks();
    change(hooks);
    return new RawFlyerTarget(scene.pool, scene.types, scene.ranges, scene.globals(), hooks);
}

// 출발점(부모 +42/+46 정수)과 사거리를 정해 검색 사각형(일반 finder 의 첫 인수)을 읽는다. 후보는 없다. (2026-11-13 WB04-E356)
function areaOf(origin, range = 30) {
    const scene = findScene({ 2: floatToBits(range) });
    put(scene.pool.allocated(50), 42, origin[0]);
    put(scene.pool.allocated(50), 46, origin[1]);
    scene.candidates = [];
    scene.body.find(scene.fields);
    const begin = scene.trace.find((line) => line.startsWith('Begin,'));
    const [left, top, right, bottom] = begin.split(',').slice(1, 5).map(Number);
    return { left, top, right, bottom };
}

test('FlyerTarget: 생성자는 PATCH 풀·188칸 표·현재 전역 조회기 여섯 개를 요구하고, TYPE 표는 살아 있는 참조·사거리 표는 만들 때 사본이다', () => {
    const scene = findScene();
    assert.equal(scene.body.pool, scene.pool);
    const build = (pool, types, ranges, globals) => () => new RawFlyerTarget(pool, types, ranges, globals, scene.hooks());
    assert.throws(build({ edition: 'cd1072' }, scene.types, scene.ranges, scene.globals()), /PATCH188/);
    assert.throws(build(null, scene.types, scene.ranges, scene.globals()), TypeError);
    assert.throws(build(scene.pool, scene.types.slice(1), scene.ranges, scene.globals()), /PATCH188/);
    assert.throws(build(scene.pool, scene.types, scene.ranges.slice(1), scene.globals()), /PATCH188/);
    assert.throws(build(scene.pool, scene.types, scene.ranges, null), TypeError);
    // 전역은 값이 아니라 조회 함수여야 하며 빠진 이름을 알려 준다.
    for (const name of ['server', 'checking', 'now', 'sunType', 'processEnd', 'dependType']) {
        const globals = scene.globals();
        delete globals[name];
        assert.throws(build(scene.pool, scene.types, scene.ranges, globals), (error) => error instanceof TypeError && error.message.endsWith(name), name);
        globals[name] = 5;
        assert.throws(build(scene.pool, scene.types, scene.ranges, globals), TypeError, `${name} 값`);
    }
    // 훅 묶음을 생략해도 만들 수 있다.
    assert.doesNotThrow(() => new RawFlyerTarget(scene.pool, scene.types, scene.ranges, scene.globals()));
    // TYPE 표는 살아 있는 참조라 만든 뒤 바꾼 발자국을 읽고, 사거리 표는 만들 때 사본이다.
    assert.equal(scene.body.capacity(60), 3);
    scene.types[80].footX = 1;
    scene.types[80].footY = 1;
    assert.equal(scene.body.capacity(60), 1);
    scene.ranges[144] = 0;
    scene.candidates = [];
    scene.body.find(scene.fields);
    const area = scene.trace.find((line) => line.startsWith('Begin,'));
    assert.equal(area, 'Begin,18,19,78,79,1');
});

test('FlyerTarget: 훅 누락은 원인을 알려 주는 오류 문구로 거부하고 표 밖 TYPE 번호도 명시 거부한다', () => {
    const message = (error) => error.message === '비행 표적 실제 하위 몸체 미연결';
    const scene = findScene({ 2: floatToBits(30) });
    assert.throws(() => new RawFlyerTarget(scene.pool, scene.types, scene.ranges, scene.globals()).find(scene.fields), message);
    // 후보 TYPE 번호가 188 이상이면 표 밖이다.
    const wild = findScene();
    wild.pool.physical(60)[10] = 200;
    assert.throws(() => wild.body.capacity(60), (error) => error instanceof RangeError && error.message === '비행 표적 TYPE 번호가 표 밖입니다');
});

test('FlyerTarget 검색 사각형: 출발점의 0/256 경계·한 축만 0 인 저장 정수·1..255 로 자르기·NaN 사거리를 원문 순서로 처리한다', () => {
    // 정상 출발점 (48, 49), 사거리 30 → 사각형 (18, 19)~(78, 79).
    assert.deepEqual(areaOf([48, 49]), { left: 18, top: 19, right: 78, bottom: 79 });
    // 한 축만 0 이면 저장된 정수를 쓴다(둘 다 0 일 때만 계산). 0 은 유효하지 않아 확장 점이 모서리를 대신한다.
    assert.deepEqual(areaOf([10, 0]), { left: 1, top: 1, right: 40, bottom: 30 });
    assert.deepEqual(areaOf([0, 10]), { left: 1, top: 1, right: 30, bottom: 40 });
    // 256 은 유효하지 않다(x, y 각각).
    assert.deepEqual(areaOf([256, 10]), { left: 226, top: 1, right: 255, bottom: 40 });
    assert.deepEqual(areaOf([10, 256]), { left: 1, top: 226, right: 40, bottom: 255 });
    // 255 로 자른 값은 254.5 와 구별된다(285 − 30.5 = 254.5).
    assert.deepEqual(areaOf([285, 100], 30.5), { left: 254, top: 69, right: 255, bottom: 131 });
    // NaN 사거리는 확장 점이 1 로 잘려 사각형이 (1, 1)~출발점이 된다.
    assert.deepEqual(areaOf([48, 49], Number.NaN), { left: 1, top: 1, right: 48, bottom: 49 });
});

test('FlyerTarget 검색: 출발점 계산(이동 보정·정수 절삭 저장)은 0 방향 절삭의 하위 DWORD 이고 범위 밖·비유한은 앞선 쓰기를 남기고 거부한다', () => {
    const run = (x, y) => {
        const scene = findScene();
        put(scene.pool.allocated(50), 42, 0);
        put(scene.pool.allocated(50), 46, 0);
        put(scene.pool.allocated(50), 14, floatToBits(x));
        put(scene.pool.allocated(50), 18, floatToBits(y));
        scene.candidates = [];
        let error = null;
        try { scene.body.find(scene.fields); } catch (caught) { error = caught; }
        return { scene, error, stored: [get(scene.pool.slot(50), 42), get(scene.pool.slot(50), 46)] };
    };
    // 중심 = 위치 − 1, 이동 보정 (1, 0)을 빼 출발점 = (x − 2, y − 1). 음수 소수는 0 방향으로 절삭한다(−2.5 → −2, 10.5 → 10).
    const negative = run(-0.5, 11.5);
    assert.equal(negative.error, null);
    assert.deepEqual(negative.stored, [0xfffffffe, 10]);
    // 큰 값은 signed64 절삭 뒤 하위 DWORD 만 저장한다(3e9 → 0xb2d05e00).
    assert.deepEqual(run(3e9, 20.5).stored, [0xb2d05e00, 19]);
    // 정확히 −2^63 은 허용하고(하위 DWORD 0) 2^63 은 거부한다.
    assert.equal(run(-(2 ** 63), 20.5).error, null);
    const high = run(2 ** 63, 20.5);
    assert.ok(high.error instanceof RangeError && high.stored[0] === 0);
    // NaN/무한 y: x 는 이미 저장됐고 y 에서 거부한다(오류 문구도 범위 오류).
    const bad = run(30.5, Number.NaN);
    assert.ok(bad.error instanceof RangeError && /ftol/.test(bad.error.message));
    assert.deepEqual(bad.stored, [28, 0]);
    assert.ok(run(Number.POSITIVE_INFINITY, 20.5).error instanceof RangeError);
});

test('FlyerTarget 진단: 줄 번호 0xa4·0x3b7·0x3d5·0x413 은 각 조건에서만 정확한 번호로 낸다', () => {
    const lines = (scene, body = scene.body) => { body.find(scene.fields); return scene.trace.filter((line) => line.startsWith('Assert,')); };
    // 평범한 입력은 진단이 없다.
    assert.deepEqual(lines(findScene()), []);
    // 0xa4(164): 부모 head 가 비어 있으면 진단한 뒤 계속한다.
    const empty = findScene();
    put(empty.pool.allocated(50), 6, 0, 2);
    assert.ok(lines(empty).includes('Assert,164'));
    // 0x3b7(951)과 0x413(1043): 검색 전/후 종속 검사가 거짓일 때.
    const before = findScene();
    let calls = 0;
    assert.deepEqual(lines(before, bodyOf(before, (hooks) => { hooks.checkForms = () => { calls += 1; return calls !== 1; }; })), ['Assert,951']);
    const after = findScene();
    let again = 0;
    assert.deepEqual(lines(after, bodyOf(after, (hooks) => { hooks.checkForms = () => { again += 1; return again !== 2; }; })), ['Assert,1043']);
    // 0x3d5(981): 이미 표적이 있으면 진단한 뒤 지우고 계속한다.
    const had = findScene();
    put(had.fields.bytes, 12, 61, 2);
    assert.ok(lines(had).includes('Assert,981'));
    assert.ok(had.fields.bytes[8] & 4);
});

test('FlyerTarget 검색: 같은 거리의 후보는 먼저 반환된 쪽을 유지하고 표적 필드는 WORD 로 쓰며 큰 SID 후보도 읽는다', () => {
    // 출발점 (50, 50)에서 같은 거리 10 에 있는 두 후보 60(오른쪽)·62(왼쪽): 먼저 나온 60 을 유지한다.
    const tie = (order) => {
        const scene = findScene();
        put(scene.pool.allocated(50), 42, 50);
        put(scene.pool.allocated(50), 46, 50);
        put(scene.pool.allocated(60), 6, 0, 2);
        put(scene.pool.allocated(60), 14, floatToBits(61));
        put(scene.pool.allocated(60), 18, floatToBits(51));
        put(scene.pool.allocated(62), 14, floatToBits(41));
        put(scene.pool.allocated(62), 18, floatToBits(51));
        scene.candidates = order;
        scene.body.find(scene.fields);
        return get(scene.fields.bytes, 12, 2);
    };
    assert.equal(tie([60, 62]), 60);
    assert.equal(tie([62, 60]), 62);
    // 후보가 SID 300 이면 표적 필드의 두 바이트에 모두 쓴다.
    const wide = findScene();
    put(wide.pool.allocated(50), 42, 50);
    put(wide.pool.allocated(50), 46, 50);
    const raw = wide.pool.physical(300);
    raw.fill(0);
    raw[10] = 80;
    put(raw, 14, floatToBits(61));
    put(raw, 18, floatToBits(51));
    wide.candidates = [300];
    bodyOf(wide, (hooks) => { hooks.mayTarget = () => true; }).find(wide.fields);
    assert.equal(get(wide.fields.bytes, 12, 2), 300);
});

test('FlyerTarget 검색: 발사 가능 효과 뒤에도 TYPE 필터는 효과 전에 읽은 값, 자리 검사는 처음 부모, 훨리기그 귀환 명령은 효과 뒤 현재 부모를 쓴다', () => {
    // mayTarget 이 80번 flags2 를 금지 비트로 바꿔도 이미 읽은 flags(0)로 후보를 유지한다.
    const filtered = findScene();
    filtered.candidates = [60];
    put(filtered.pool.allocated(60), 6, 0, 2);
    bodyOf(filtered, (hooks) => {
        const may = hooks.mayTarget;
        hooks.mayTarget = (parent, target) => { const result = may(parent, target); filtered.types[80].flags2 = 0x120000; return result; };
    }).find(filtered.fields);
    assert.notEqual(get(filtered.fields.bytes, 12, 2), 0);
    // 자리 검사는 효과 전에 읽은 부모(50)를 쓴다: 부모를 51(free)로 바꿔도 50 으로 가까운 비행체를 센다(감시 조회가 일어남).
    const counted = findScene();
    counted.candidates = [60];
    counted.pool.physical(51)[11] = 1;
    bodyOf(counted, (hooks) => {
        const may = hooks.mayTarget;
        hooks.mayTarget = (parent, target) => { const result = may(parent, target); put(counted.fields.bytes, 56, 51); return result; };
    }).find(counted.fields);
    assert.ok(counted.trace.some((line) => line.startsWith('Watch,')));
    // 훨리기그 부모(TYPE 73)는 귀환 기한이 0 일 때 복귀 지연과 명령 2 를 내고, 명령은 지연 효과 뒤 현재 부모에게 간다.
    const sun = findScene({ 3: 73 });
    sun.candidates = [60];
    put(sun.pool.allocated(60), 6, 0, 2);
    bodyOf(sun, (hooks) => {
        const delay = hooks.returnDelay;
        hooks.returnDelay = (fields) => { const result = delay(fields); put(sun.fields.bytes, 56, 51); return result; };
    }).find(sun.fields);
    assert.ok(sun.trace.includes('Command,51,2'));
    assert.equal(timeOf(sun, 40), 10000100);
});

// 필드의 raw double 을 읽는다. (2026-11-13 WB04-E356)
function timeOf(scene, at) {
    const buffer = Buffer.from(scene.fields.bytes);
    const view = new DataView(new ArrayBuffer(8));
    view.setUint32(0, buffer.readUInt32LE(at), true);
    view.setUint32(4, buffer.readUInt32LE(at + 4), true);
    return view.getFloat64(0, true);
}

test('FlyerTarget 의존 목록: 큰 SID 체인·의존 TYPE 경계(69/70)·순환/범위 밖 next 오류와 표적/비행체 0 의 즉시 반환', () => {
    // 목록 SID 300 → 301 (둘 다 의존 TYPE), 감시 쌍은 같은 상대 프로세스 부모 51 로 이어진다.
    const make = (types = [1, 1], depend = 1) => {
        const scene = new FlyerTargetScene('C', '60,50,2,2,0,0,0,0,0'.split(','));
        scene.depend = depend;
        // 표적 60 의 위치·중심은 (60.25, 50.5)−1, 비행체 50 은 (50.25, 50.5), 다른 비행체 51 은 표적에 훨씬 가깝다.
        put(scene.pool.allocated(51), 14, floatToBits(58.25));
        put(scene.pool.allocated(60), 6, 300, 2);
        for (const [index, sid] of [300, 301].entries()) {
            const raw = scene.pool.physical(sid);
            raw.fill(0);
            raw[10] = types[index];
            put(raw, 4, index === 0 ? 301 : 0, 2);
        }
        const body = bodyOf(scene, (hooks) => {
            hooks.watcher = () => 70;
            hooks.processParent = () => 51;
        });
        return { scene, body };
    };
    assert.equal(make().body.closerCount(60, 50), 2);
    // 의존 TYPE 전역은 후보마다 다시 읽고 69 이하만 인정한다.
    assert.equal(make([69, 70], 69).body.closerCount(60, 50), 1);
    assert.equal(make([69, 70], 70).body.closerCount(60, 50), 0);
    // 순환 체인은 풀 크기 만큼 돈 뒤 거부한다.
    const cyclic = make();
    put(cyclic.scene.pool.physical(301), 4, 300, 2);
    assert.throws(() => cyclic.body.closerCount(60, 50), (error) => error.message === '비행 의존 finder 범위/순환 오류');
    // 풀 크기와 같은 next 도 범위 오류이다(풀 접근 오류와 구별).
    const outside = make();
    put(outside.scene.pool.physical(301), 4, 24000, 2);
    assert.throws(() => outside.body.closerCount(60, 50), (error) => error.message === '비행 의존 finder 범위/순환 오류');
    // 표적 0·비행체 0 은 즉시 0, 자리 수는 표적 0 이면 3.
    const zero = make();
    assert.equal(zero.body.closerCount(0, 50), 0);
    assert.equal(zero.body.closerCount(60, 0), 0);
    assert.equal(zero.body.capacity(0), 3);
});

test('FlyerTarget 의존 목록: 감시 Form 의 프로세스 TYPE 은 [10, 상한)만 세고 감시자가 없는 Form 은 건너뛴다', () => {
    const make = (setup = () => {}) => {
        const scene = new FlyerTargetScene('C', '60,50,2,2,0,0,0,0,0'.split(','));
        put(scene.pool.allocated(51), 14, floatToBits(58.25));
        setup(scene);
        return { scene, body: bodyOf(scene, (hooks) => { hooks.processParent = () => 51; hooks.watcher = (form) => (form === 82 || form === 80 ? 70 : 0); }) };
    };
    // 감시 Form 두 개(82, 80)가 같은 상대 프로세스(70)와 이어진다(Form 84 는 TYPE 6 이라 의존 목록에서 제외). TYPE 10·62 는 세고 9·63 은 세지 않는다.
    for (const [type, expected] of [[10, 2], [9, 0], [62, 2], [63, 0]]) {
        const made = make((scene) => { scene.pool.physical(70)[10] = type; });
        assert.equal(made.body.closerCount(60, 50), expected, `TYPE ${type}`);
    }
    // 감시자가 없는 Form(81, 83)은 번호 0 슬롯을 읽지 않고 건너뛴다(0 번 슬롯이 범위 안 TYPE 이어도).
    const zero = make((scene) => { scene.pool.physical(0)[10] = 31; });
    assert.equal(zero.body.closerCount(60, 50), 2);
});

test('FlyerTarget 자리 번호: 자기 감시 Form 은 첫 일치이고 더 작은 번호의 비행체 Form 만 세며 부모 head 가 비면 진단 0xa4, 못 찾으면 checking 일 때만 0x374', () => {
    const make = (watchers, setup = () => {}) => {
        const scene = new FlyerTargetScene('I', '60,50,2,2,0,0,0,0,0'.split(','));
        setup(scene);
        const body = bodyOf(scene, (hooks) => { hooks.watcher = (form) => watchers[form] ?? 0; });
        return { scene, body };
    };
    // 의존 목록 순서는 82, 80, 81, 83(Form 84 는 TYPE 6 이라 제외). 감시 Form 82→70(부모 50), 80→71(부모 51): 비행체 50 은 Form 82(더 작은 80 이 하나), 비행체 51 은 Form 80(더 작은 번호 없음).
    const base = make({ 82: 70, 80: 71 });
    assert.equal(base.body.slotIndex(60, 50), 1);
    assert.equal(base.body.slotIndex(60, 51), 0);
    // 상대 비행체 TYPE 에 0x100000 이 없으면 세지 않는다.
    const plain = make({ 82: 70, 80: 71 }, (scene) => { scene.pool.physical(51)[10] = 80; });
    assert.equal(plain.body.slotIndex(60, 50), 0);
    // 감시자가 없는 Form(번호 0)은 0 번 슬롯을 읽지 않고 건너뛴다.
    const none = make({ 80: 71 }, (scene) => {
        scene.pool.physical(0)[10] = 31;
        put(scene.pool.physical(0), 16, 51, 2);
    });
    assert.equal(none.body.slotIndex(60, 51), 0);
    // 같은 비행체의 감시 Form 이 둘이면 먼저 만난 Form 을 자기 번호로 삼는다(뒤의 82 가 아니라 84).
    const first = make({ 82: 71, 80: 71 });
    assert.equal(first.body.slotIndex(60, 51), 1);
    // 부모 head 가 비어 있으면 진단 0xa4 뒤 계속한다.
    const empty = make({ 82: 70, 80: 71 }, (scene) => { put(scene.pool.allocated(51), 6, 0, 2); });
    assert.equal(empty.body.slotIndex(60, 50), 1);
    assert.ok(empty.scene.trace.includes('Assert,164'));
    // 비행체 번호를 못 찾으면 checking 이 꺼진 동안은 진단 없이 0, 켜지면 0x374(884)를 낸다.
    const lost = make({ 82: 70 });
    assert.equal(lost.body.slotIndex(60, 55), 0);
    assert.ok(!lost.scene.trace.some((line) => line.startsWith('Assert,')));
    const checked = make({ 82: 70 });
    checked.scene.checking = true;
    assert.equal(checked.body.slotIndex(60, 55), 0);
    assert.ok(checked.scene.trace.includes('Assert,884'));
});

test('FlyerTarget 검색: 발자국 X/Y 를 각 축에 쓰고, 출발점 사각형은 이동 보정 효과 뒤의 부모 정수를 읽으며, 후보가 없으면 이전 표적을 비운다', () => {
    // 발자국 (2, 6): 표적 60 의 중심 y = 위치 − 3 이라 출발점 (50, 50)에서 거리 29.7(< 30)로 선택된다(발자국 X 를 쓰면 31.6 이라 탈락).
    const foot = findScene();
    foot.types[80].footY = 6;
    put(foot.pool.allocated(50), 42, 50);
    put(foot.pool.allocated(50), 46, 50);
    put(foot.pool.allocated(60), 6, 0, 2);
    put(foot.pool.allocated(60), 14, floatToBits(61));
    put(foot.pool.allocated(60), 18, floatToBits(81));
    foot.candidates = [60];
    foot.body.find(foot.fields);
    assert.equal(get(foot.fields.bytes, 12, 2), 60);
    // 이동 보정 훅이 부모 +42/+46 을 채우면 그 뒤 읽기가 저장된 정수(30, 31)를 출발점으로 쓰고 덮어쓰지 않는다.
    const live = findScene();
    put(live.pool.allocated(50), 42, 0);
    put(live.pool.allocated(50), 46, 0);
    live.candidates = [];
    bodyOf(live, (hooks) => {
        const offset = hooks.offset;
        hooks.offset = (sid) => { const result = offset(sid); put(live.pool.allocated(50), 42, 30); put(live.pool.allocated(50), 46, 31); return result; };
    }).find(live.fields);
    assert.ok(live.trace.includes('Begin,1,1,60,61,1'));
    assert.deepEqual([get(live.pool.slot(50), 42), get(live.pool.slot(50), 46)], [30, 31]);
    // 후보가 없으면 이전 표적 필드를 0 으로 비운다.
    const cleared = findScene();
    put(cleared.fields.bytes, 12, 61, 2);
    cleared.candidates = [];
    cleared.body.find(cleared.fields);
    assert.equal(get(cleared.fields.bytes, 12, 2), 0);
});

test('FlyerTarget 검색: 사거리 검사는 x·y 를 각 출발점 축과 비교하고, 자리 수와 같은 가까운 비행체 수는 후보를 탈락시킨다', () => {
    // 출발점 (10, 100), 표적 중심 (30, 100): 거리 20(< 25)이라 선택(축을 바꾸면 114 라 탈락).
    const axes = findScene({ 2: floatToBits(25) });
    put(axes.pool.allocated(50), 42, 10);
    put(axes.pool.allocated(50), 46, 100);
    put(axes.pool.allocated(60), 6, 0, 2);
    put(axes.pool.allocated(60), 14, floatToBits(31));
    put(axes.pool.allocated(60), 18, floatToBits(101));
    axes.candidates = [60];
    axes.body.find(axes.fields);
    assert.equal(get(axes.fields.bytes, 12, 2), 60);
    // 1×1 표적(자리 1)에 더 가까운 다른 비행체가 1 마리면 가까운 수 == 자리 수라 후보에서 탈락한다.
    const crowded = (closer) => {
        const scene = findScene();
        scene.types[80].footX = 1;
        scene.types[80].footY = 1;
        put(scene.pool.allocated(50), 42, 50);
        put(scene.pool.allocated(50), 46, 50);
        put(scene.pool.allocated(51), 14, floatToBits(58.25));
        scene.candidates = [60];
        put(scene.pool.allocated(60), 14, floatToBits(61));
        put(scene.pool.allocated(60), 18, floatToBits(51));
        bodyOf(scene, (hooks) => {
            hooks.watcher = (form) => (form === 82 ? 70 : 0);
            hooks.processParent = () => (closer ? 51 : 50);
        }).find(scene.fields);
        return get(scene.fields.bytes, 12, 2);
    };
    assert.equal(crowded(true), 0);
    assert.equal(crowded(false), 60);
});

test('FlyerTarget 가까운 비행체 수: 거리는 x·y 를 모두 쓰고 표적 0 은 슬롯 0 을 읽지 않으며 자리 수 0 번 슬롯 TYPE 의 영향을 받지 않는다', () => {
    const make = (setup = () => {}) => {
        const scene = new FlyerTargetScene('C', '60,50,2,2,0,0,0,0,0'.split(','));
        put(scene.pool.allocated(51), 14, floatToBits(60.25));
        put(scene.pool.allocated(51), 18, floatToBits(39.5));
        setup(scene);
        return { scene, body: bodyOf(scene, (hooks) => { hooks.watcher = (form) => (form === 82 ? 70 : 0); hooks.processParent = () => 51; }) };
    };
    // 비행체 50 은 표적까지 거리 10(dx), 다른 비행체 51 은 dy 11 이라 더 멀다(y 를 빼먹으면 0 이라 센다).
    assert.equal(make().body.closerCount(60, 50), 0);
    // dy 9 로 가까우면 센다.
    assert.equal(make((scene) => { put(scene.pool.allocated(51), 18, floatToBits(41.5)); }).body.closerCount(60, 50), 1);
    // 표적 0 은 번호 0 슬롯에 종속이 있어도 즉시 0, 자리 수 3 도 0 번 슬롯 발자국과 무관하다.
    const zero = make((scene) => {
        scene.types[0].footX = 1;
        scene.types[0].footY = 1;
        const raw = scene.pool.physical(0);
        raw[10] = 0;
        raw[11] = 0;
        put(raw, 6, 82, 2);
        put(raw, 14, floatToBits(60.25));
        put(raw, 18, floatToBits(40.5));
    });
    assert.equal(zero.body.closerCount(0, 50), 0);
    assert.equal(zero.body.capacity(0), 3);
});

test('FlyerTarget float 저장: 출발점·포함 점·최소 거리 기준·비교 기준 거리를 원문처럼 float 로 줄여 저장한다', () => {
    // 출발점: 부모 중심 40.5 에서 이동 보정 0.5000001(float 가 아님)을 빼면 39.9999999 → float 40 으로 저장되어 정수 40 이다(줄이지 않으면 39).
    const origin = findScene();
    put(origin.pool.allocated(50), 42, 0);
    put(origin.pool.allocated(50), 46, 0);
    put(origin.pool.allocated(50), 14, floatToBits(41.5));
    put(origin.pool.allocated(50), 18, floatToBits(41.5));
    origin.candidates = [];
    bodyOf(origin, (hooks) => { hooks.offset = () => [0.5000001, 0.5000001]; }).find(origin.fields);
    assert.deepEqual([get(origin.pool.slot(50), 42), get(origin.pool.slot(50), 46)], [40, 40]);
    assert.ok(origin.trace.includes('Begin,10,10,70,70,1'));
    // 포함 점(오른쪽): 출발점 1.5 + 82·2^-23 에 사거리 38.5 를 더한 40.0000098 은 float 로 40.0000114 가 되어 오른쪽 경계가 41 이다.
    const plus = findScene({ 2: floatToBits(38.5) });
    put(plus.pool.allocated(50), 42, 0);
    put(plus.pool.allocated(50), 46, 0);
    put(plus.pool.allocated(50), 14, floatToBits(3.5 + 82 * 2 ** -23));
    put(plus.pool.allocated(50), 18, floatToBits(10.5));
    plus.candidates = [];
    plus.body.find(plus.fields);
    assert.ok(plus.trace.some((line) => line.startsWith('Begin,1,1,41,')));
    // 포함 점(왼쪽): 60 − (20 + 2^-19) = 39.99999809 는 float 로 40 이 되어 왼쪽 경계가 40 이다(줄이지 않으면 39).
    assert.equal(areaOf([60, 100], 20 + 2 ** -19).left, 40);
    // 최소 거리 기준: 첫 후보 A(거리 d1)가 float 로 올림되면 그 사이의 거리 d2 에 있는 후보 B 를 받아들인다(원본 62, 줄이지 않으면 60).
    const best = findScene();
    put(best.pool.allocated(50), 42, 50);
    put(best.pool.allocated(50), 46, 50);
    for (const [sid, x, y] of [[60, 42.17170715332031, 44.64498519897461], [62, 56.80299758911133, 60.20055389404297]]) {
        put(best.pool.allocated(sid), 6, 0, 2);
        put(best.pool.allocated(sid), 14, floatToBits(x));
        put(best.pool.allocated(sid), 18, floatToBits(y));
    }
    best.candidates = [60, 62];
    best.body.find(best.fields);
    assert.equal(get(best.fields.bytes, 12, 2), 62);
    // 비교 기준 거리: 비행체 50 의 표적까지 거리 d1 은 float 로 내림돼 저장되고, 그 사이 거리의 다른 비행체 51 은 세지 않는다.
    const mine = new FlyerTargetScene('C', '60,50,2,2,0,0,0,0,0'.split(','));
    put(mine.pool.allocated(60), 14, floatToBits(40));
    put(mine.pool.allocated(60), 18, floatToBits(40));
    put(mine.pool.allocated(60), 6, 300, 2);
    const form = mine.pool.physical(300);
    form.fill(0);
    form[10] = 1;
    put(mine.pool.allocated(50), 14, floatToBits(46.97922134399414));
    put(mine.pool.allocated(50), 18, floatToBits(35.601993560791016));
    put(mine.pool.allocated(51), 14, floatToBits(48.24936294555664));
    put(mine.pool.allocated(51), 18, floatToBits(40));
    const mineBody = bodyOf(mine, (hooks) => { hooks.watcher = () => 70; hooks.processParent = () => 51; });
    assert.equal(mineBody.closerCount(60, 50), 0);
});
