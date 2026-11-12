// 비행 제어 raw-flyer-frame.js(RawFlyerFrame)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: flyer-frame-x86.tsv 744행 — 초기화 I 18행(기본/맨 오 워의 시각·padding), 맨 오 워 애니메이션 A 122행(프레임 6 발사와 타입별 소리 억제), 표적 해제 C 26행(의존 해제·dirty·WORD·통지·갱신 순서), 통지 N 74행(미선택 사건의 자연 반환 포함),
//    홈 통지 D 30행(현재 홈의 타입/생존/범위 관문), 제어 F 474행(이동·착륙·공격·시간·표적 갱신 전체)이 필드 112바이트·객체 350바이트·전역 20바이트·타입별 소리 장부 20바이트·훨리베이스 48바이트·결과·호출 순서까지 같다.
//    cpppj FlyerFrameTests.cpp 의 FlyerFrameScene 을 같은 순서로 옮겼다.
//  - cpppj 시험 이식: 선택 누락과 부분 쓰기 보존(FlyerFrame_RequiredBoundaries).
//  - 웹 계약: 생성자 검사, 훅 누락 문구, std::min/max 의 NaN 방향, 좌표 절삭 범위 오류, 맨 오 워 프레임의 나머지 부호·0/오버플로 거부, 보충 표 번호 범위.
// 10.72(CD)는 범위 밖이다. 실제 게임·브라우저 실행이 아니다. (2026-11-13 WB04-E355: 7-P1 RawFlyerFrame 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { FlyerProcessFields } from '../www/core/raw-flyer-process.js';
import { SunAviaryProcessFields } from '../www/core/raw-sun-aviary-process.js';
import { RawFlyerFrame } from '../www/core/raw-flyer-frame.js';

// 원본 관측 행이다(첫 행은 열 이름). 없으면 해당 검사를 건너뛴다. (2026-11-13 WB04-E355)
const rows = (() => {
    const all = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'flyer-frame-x86.tsv'));
    return all ? all.slice(1) : null;
})();

// 두 부모/적/홈/free/dead/Form 입력의 실제 SID 와 소리 장부 다섯 타입이다. (2026-11-13 WB04-E355)
const FRAME_IDS = [50, 51, 60, 61, 62, 63, 70];
const FRAME_TYPES = [72, 73, 80, 138, 144];

// 예외가 나는지 본다. (2026-11-13 WB04-E355)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 바이트 배열의 소문자 16진 문자열이다. (2026-11-13 WB04-E355)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 결과를 계산하지 않고 Native 와 같은 raw 입력과 큰 효과의 변경 입력만 공급하는 장면이다(cpppj FlyerFrameScene). (2026-11-13 WB04-E355)
class FlyerFrameScene {
    constructor(kind, input) {
        this.pool = new SidPool(24000, { server: false });
        this.types = Array.from({ length: 188 }, () => ({ cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0 }));
        this.details = Array.from({ length: 188 }, () => ({ range: 0, sound: 0 }));
        this.stamps = new Uint32Array(188);
        this.fields = new FlyerProcessFields();
        this.sun = new SunAviaryProcessFields();
        this.server = true;
        this.boss = true;
        this.checking = false;
        this.now = 100;
        this.frame = 7;
        this.sunType = 73;
        this.windType = 138;
        this.rainType = 144;
        this.sunProcessType = 32;
        this.soundType = 47;
        this.mutation = 0;
        this.may = 1;
        this.closer = 0;
        this.trace = [];
        // 관찰할 현재 슬롯까지 실제 확보한다.
        for (;;) {
            const sid = this.pool.allocate(2);
            if (sid === 70) break;
            if (!sid || sid > 70) throw new Error('비행 Frame 검사 SID 준비 오류');
        }
        // Native 와 같은 전체 350바이트를 준비한다.
        for (const value of FRAME_IDS) {
            const raw = this.pool.allocated(value);
            raw.fill(0xa5);
            put(raw, 0, 0x50a388);
            put(raw, 4, 0, 2);
            put(raw, 6, 0, 2);
            put(raw, 8, 0, 2);
            raw[10] = value < 60 ? 144 : value === 61 ? 72 : 80;
            raw[11] = value === 62 ? 1 : value === 63 ? 2 : 0;
            put(raw, 12, value === 61 ? 50 : 17, 2);
            put(raw, 14, floatToBits(value < 60 ? 50.25 : 60.25));
            put(raw, 18, floatToBits(value < 60 ? 50.5 : 60.5));
            raw[34] = 1;
            raw[40] = 0;
            put(raw, 36, 0);
            put(raw, 42, 50);
            put(raw, 46, 50);
        }
        // 실제 부모 getter 가 요구하는 부착 head 를 준비한다.
        for (const value of [50, 51]) put(this.pool.allocated(value), 6, 70, 2);
        const attached = this.pool.allocated(70);
        attached[10] = 31;
        attached[11] = 8;
        put(this.pool.allocated(51), 36, 3);
        // 실제 TYPE 입력만 준비한다.
        for (const type of FRAME_TYPES) {
            this.types[type].footX = 2;
            this.types[type].footY = 2;
            this.details[type] = { range: 100, sound: 400 + type };
        }
        this.fields.bytes.fill(0xa5);
        put(this.fields.bytes, 0, 0x509f60);
        this.fields.bytes[8] = 0x10;
        put(this.fields.bytes, 10, 61, 2);
        put(this.fields.bytes, 12, 0, 2);
        put(this.fields.bytes, 16, floatToBits(0.75));
        put(this.fields.bytes, 20, floatToBits(0.5));
        this.timeAt(32, 99.75);
        this.timeAt(40, 0);
        put(this.fields.bytes, 48, 0x509f40);
        put(this.fields.bytes, 52, 70);
        put(this.fields.bytes, 56, 50);
        this.timeAt(64, 0);
        this.timeAt(72, 101);
        put(this.fields.bytes, 80, 0);
        put(this.fields.bytes, 84, 1);
        put(this.fields.bytes, 88, 0);
        this.timeAt(96, 101);
        this.timeAt(104, 160);
        this.sun.bytes.fill(0x5a);
        put(this.sun.bytes, 24, 8);
        if (kind === 'I') {
            this.fields.bytes.fill(Number(input[2]) & 0xff);
            this.now = [0, 100.25, -5.25][Number(input[1])];
        } else if (kind === 'A') {
            put(this.pool.allocated(50), 36, Number(input[0]));
            this.timeAt(96, [99, 100, 101][Number(input[1])]);
            put(this.fields.bytes, 12, Number(input[2]), 2);
            put(this.fields.bytes, 80, Number(input[3]));
            this.stamps[144] = Number(input[4]);
            this.mutation = Number(input[5]);
        } else if (kind === 'C' || kind === 'N' || kind === 'D') {
            this.server = Number(input[0]) !== 0;
            this.boss = Number(input[1]) !== 0;
            put(this.fields.bytes, 12, Number(input[2]), 2);
            put(this.fields.bytes, 10, Number(input[3]), 2);
            this.pool.allocated(50)[10] = Number(input[4]) & 0xff;
            this.pool.allocated(61)[11] = Number(input[5]) & 0xff;
            this.mutation = Number(input[8]);
        } else {
            this.server = Number(input[1]) !== 0;
            this.boss = Number(input[2]) !== 0;
            this.pool.allocated(50)[11] = Number(input[3]) & 0xff;
            // free 입력도 이미 확보한 같은 메모리에 직접 준비한다.
            const writable = this.pool.physical(50);
            writable[40] = Number(input[4]) & 0xff;
            put(this.fields.bytes, 12, Number(input[5]), 2);
            put(this.fields.bytes, 84, Number(input[8]));
            put(this.fields.bytes, 80, Number(input[9]));
            put(this.fields.bytes, 88, Number(input[10]));
            this.timeAt(32, 100 - [0, 0.025, 0.25, 1, -0.25][Number(input[7])]);
            this.timeAt(40, [0, 99, 101][Number(input[11])]);
            this.timeAt(64, [99, 100, 101][Number(input[12])]);
            this.may = Number(input[13]) | 0;
            this.closer = Number(input[14]) | 0;
            this.timeAt(96, [99, 101][Number(input[15])]);
            this.mutation = Number(input[16]);
            const point = [[60.25, 60.5], [50.25, 51.5], [50.5, 51.5], [50.75, 51.5], [300, 300]][Number(input[6])];
            // free/dead 도 입력 메모리 자체는 그대로 준비한다.
            for (const value of [60, 61, 62, 63]) {
                const target = this.pool.physical(value);
                put(target, 14, floatToBits(point[0]));
                put(target, 18, floatToBits(point[1]));
            }
            if (Number(input[10]) === 2) put(this.sun.bytes, 24, 9);
            if (Number(input[11]) === 1 || Number(input[5]) === 61) writable[10] = 73;
            if (this.mutation >= 13 && this.mutation <= 15) this.timeAt(104, 99 + (this.mutation - 13));
            if (this.mutation === 16) put(this.pool.allocated(61), 12, 51, 2);
            if (this.mutation >= 17 && this.mutation <= 19) {
                const child = this.pool.allocated(70);
                child[11] = this.mutation === 17 ? 12 : this.mutation === 18 ? 10 : 9;
            }
        }
        this.body = new RawFlyerFrame(this.pool, this.types, this.details, this.stamps, this.globals(), this.hooks());
    }

    // 원문 double 의 입력 비트만 쓴다. (2026-11-13 WB04-E355)
    timeAt(at, value) {
        const view = new DataView(new ArrayBuffer(8));
        view.setFloat64(0, value, true);
        put(this.fields.bytes, at, view.getUint32(0, true));
        put(this.fields.bytes, at + 4, view.getUint32(4, true));
    }

    // 실제 호출 순서만 기록한다. (2026-11-13 WB04-E355)
    event(value) { this.trace.push(value); }

    // 같은 현재 값을 매번 읽는 전역 함수 묶음이다. (2026-11-13 WB04-E355)
    globals() {
        return {
            server: () => this.server,
            boss: () => this.boss,
            checking: () => (this.checking ? 1 : 0),
            now: () => this.now,
            frame: () => this.frame,
            sunType: () => this.sunType,
            windType: () => this.windType,
            rainType: () => this.rainType,
            sunAviaryProcess: () => this.sunProcessType,
            soundProcess: () => this.soundType,
        };
    }

    // Native 의 명시 경계 인수/변경 입력만 공급하며 제품 제어 결과를 계산하지 않는다. (2026-11-13 WB04-E355)
    hooks() {
        const mutateRefresh = () => {
            if (this.mutation === 4) {
                put(this.fields.bytes, 12, 60, 2);
                this.fields.bytes[8] = 0;
                this.now = 200;
            }
        };
        return {
            assertion: (line) => { this.event(`Assert,${line}`); },
            robust: () => { this.event('Robust'); return true; },
            sidRobust: (sid, live) => { this.event(`Sid,${sid},${live ? '1' : '0'}`); return sid > 0 && sid < 65535; },
            checkForms: (sid) => { this.event(`Check,${sid}`); return this.body.checkForms(sid); },
            runCount: (sid) => { this.event(`Count,${sid}`); return 16; },
            destroy: (sid, flags) => { this.event(`Destroy,${sid},${flags}`); },
            finish: () => { this.event('Finish'); },
            launch: (sid, target, flags) => {
                this.event(`Launch,${sid},${target},${flags}`);
                if (this.mutation === 2) {
                    this.pool.allocated(50)[10] = 138;
                    put(this.pool.allocated(50), 36, 9);
                    put(this.fields.bytes, 56, 51);
                    this.now = 200;
                }
            },
            sound: (x, y, token) => {
                this.event(`Sound,${floatToBits(x)},${floatToBits(y)},${token}`);
                if (this.mutation === 11) {
                    put(this.pool.allocated(50), 36, 9);
                    put(this.fields.bytes, 56, 51);
                }
            },
            removeDepend: (form, target) => {
                this.event(`Remove,${form},${target}`);
                if (this.mutation === 3) {
                    put(this.fields.bytes, 12, 60, 2);
                    put(this.fields.bytes, 52, 71);
                    this.fields.bytes[8] = 2;
                    this.now = 200;
                }
            },
            addDepend: (target, form) => { this.event(`Add,${target},${form}`); },
            refreshForm: () => { this.event('Refresh'); mutateRefresh(); },
            refreshSun: () => { this.event('RefreshSun'); mutateRefresh(); },
            refreshTime: (sid) => {
                this.event(`Time,${sid}`);
                if (this.mutation === 12) {
                    put(this.fields.bytes, 20, floatToBits(4));
                    put(this.fields.bytes, 12, 61, 2);
                    this.boss = false;
                }
            },
            hit: (sid, target) => {
                this.event(`Hit,${sid},${target}`);
                if (this.mutation === 9) {
                    put(this.fields.bytes, 80, 7);
                    put(this.pool.allocated(50), 36, 6);
                    this.now = 200;
                }
            },
            offset: () => {
                if (this.mutation === 8) {
                    put(this.fields.bytes, 10, 0, 2);
                    put(this.fields.bytes, 12, 61, 2);
                }
                return [1, 0];
            },
            scatter: (sid, parent) => { this.event(`Scatter,${sid},${parent}`); return [0, 0]; },
            unpop: (sid, flags) => {
                this.event(`Unpop,${sid},${flags}`);
                if (this.mutation === 1) {
                    put(this.pool.allocated(50), 14, floatToBits(71.25));
                    put(this.pool.allocated(50), 18, floatToBits(72.5));
                    put(this.fields.bytes, 56, 51);
                    this.now = 200;
                }
            },
            pop: (sid, x, y, flags) => {
                this.event(`Pop,${sid},${floatToBits(x)},${floatToBits(y)},${flags}`);
                put(this.pool.allocated(sid), 14, floatToBits(x));
                put(this.pool.allocated(sid), 18, floatToBits(y));
            },
            repop: (sid, flags) => { this.event(`Repop,${sid},${flags}`); },
            movedCell: (sid) => { this.event(`Cell,${sid}`); },
            homeNotice: (sid, a, b) => { this.event(`Home,${sid},${a},${b}`); },
            mayTarget: (sid, target) => {
                this.event(`May,${sid},${target}`);
                if (this.mutation === 5) {
                    put(this.fields.bytes, 12, 0, 2);
                    put(this.fields.bytes, 10, 0, 2);
                }
                return this.may !== 0;
            },
            closerCount: (target, sid) => { this.event(`Closer,${target},${sid}`); return this.closer; },
            capacity: (sid) => { this.event(`Capacity,${sid}`); return 3; },
            findTarget: () => {
                this.event('Find');
                if (this.mutation === 6) put(this.fields.bytes, 12, 60, 2);
                if (this.mutation === 7) this.now = 200;
            },
            removeSound: (sid, type) => { this.event(`RemoveSound,${sid},${type}`); },
            sunProcess: (sid, type) => { this.event(`Sun,${sid},${type}`); return this.sun; },
            returnDelay: () => { this.event('Delay'); return 10000000; },
            command: (sid, command) => {
                this.event(`Command,${sid},${command}`);
                if (this.mutation === 10) put(this.fields.bytes, 12, 0, 2);
            },
        };
    }

    // 일곱 전체 기록을 같은 순서로 읽는다. (2026-11-13 WB04-E355)
    objects() { return FRAME_IDS.map((sid) => hex(this.pool.slot(sid))).join(''); }

    // 현재 20바이트 전역이다. (2026-11-13 WB04-E355)
    globalsImage() {
        const raw = new Uint8Array(20);
        const view = new DataView(new ArrayBuffer(8));
        view.setFloat64(0, this.now, true);
        put(raw, 0, this.server ? 1 : 0);
        put(raw, 4, this.boss ? 1 : 0);
        put(raw, 8, view.getUint32(0, true));
        put(raw, 12, view.getUint32(4, true));
        put(raw, 16, this.frame);
        return hex(raw);
    }

    // 공유 TYPE 다섯 기록이다. (2026-11-13 WB04-E355)
    stampsImage() {
        const raw = new Uint8Array(20);
        let at = 0;
        // 다섯 타입의 장부를 4바이트씩 쓴다.
        for (const type of FRAME_TYPES) {
            put(raw, at, this.stamps[type]);
            at += 4;
        }
        return hex(raw);
    }
}

// 각 범위를 독립 실행해 한 실패가 다른 원문 비교를 감추지 않게 한다. (2026-11-13 WB04-E355)
function compareFrame(kind, expected) {
    let compared = 0;
    // 헤더 이외의 선택한 원문 입력만 처리한다.
    for (const row of rows) {
        assert.equal(row.length, 9);
        if (row[0] !== kind) continue;
        compared += 1;
        const input = row[1].split(',');
        const scene = new FlyerFrameScene(kind, input);
        let result = '-';
        if (kind === 'I') scene.body.initialize(scene.fields, Number(input[0]) !== 0);
        else if (kind === 'A') result = String(scene.body.rainFrame(scene.fields));
        else if (kind === 'C') scene.body.clearTarget(scene.fields);
        else if (kind === 'N') scene.body.notify(scene.fields, Number(input[6]), 777, Number(input[7]));
        else if (kind === 'D') scene.body.parentDying(scene.fields);
        else if (Number(input[0])) scene.body.frame(scene.fields);
        else scene.body.baseFrame(scene.fields);
        const where = `${kind}/${row[1]}`;
        assert.equal(hex(scene.fields.bytes), row[2], `${where} 필드`);
        assert.equal(scene.objects(), row[3], `${where} 객체`);
        assert.equal(scene.globalsImage(), row[4], `${where} 전역`);
        assert.equal(scene.stampsImage(), row[5], `${where} 소리 장부`);
        assert.equal(hex(scene.sun.bytes), row[6], `${where} 훨리베이스`);
        assert.equal(result, row[7], `${where} 결과`);
        assert.equal(scene.trace.length === 0 ? '-' : scene.trace.join(';'), row[8], `${where} 호출 순서`);
    }
    assert.equal(compared, expected);
}

test('FlyerFrame: 원본 10.78 초기화 18행 — 기본/맨 오 워의 시각·padding 이 같다', { skip: !rows && '원본 fixture 없음' }, () => { compareFrame('I', 18); });
test('FlyerFrame: 원본 10.78 애니메이션 122행 — 프레임 6 발사와 현재 타입 음향 억제가 같다', { skip: !rows && '원본 fixture 없음' }, () => { compareFrame('A', 122); });
test('FlyerFrame: 원본 10.78 표적 해제 26행 — 의존 해제/dirty/WORD/통지/갱신 순서가 같다', { skip: !rows && '원본 fixture 없음' }, () => { compareFrame('C', 26); });
test('FlyerFrame: 원본 10.78 통지 74행 — 미선택 사건의 자연 반환도 같다', { skip: !rows && '원본 fixture 없음' }, () => { compareFrame('N', 74); });
test('FlyerFrame: 원본 10.78 홈 통지 30행 — 현재 홈의 타입/생존/범위 관문이 같다', { skip: !rows && '원본 fixture 없음' }, () => { compareFrame('D', 30); });
test('FlyerFrame: 원본 10.78 제어 474행 — 이동/착륙/공격/시간/표적 갱신 전체가 같다', { skip: !rows && '원본 fixture 없음' }, () => { compareFrame('F', 474); });

test('FlyerFrame: 선택 누락은 앞선 쓰기를 남기고 거부한다', () => {
    const scene = new FlyerFrameScene('A', '0,1,0,0,0,0'.split(','));
    const hooks = scene.hooks();
    delete hooks.runCount;
    const missing = new RawFlyerFrame(scene.pool, scene.types, scene.details, scene.stamps, scene.globals(), hooks);
    const before = Uint8Array.from(scene.fields.bytes);
    assert.ok(throws(() => missing.rainFrame(scene.fields)));
    assert.notDeepEqual(scene.fields.bytes, before);
    assert.equal(scene.trace.length, 0);
    scene.timeAt(96, 101);
    assert.equal(missing.rainFrame(scene.fields), 0);
    const dependHooks = scene.hooks();
    delete dependHooks.removeDepend;
    const missingDepend = new RawFlyerFrame(scene.pool, scene.types, scene.details, scene.stamps, scene.globals(), dependHooks);
    put(scene.fields.bytes, 12, 60, 2);
    const current = Uint8Array.from(scene.fields.bytes);
    assert.ok(throws(() => missingDepend.clearTarget(scene.fields)));
    assert.deepEqual(scene.fields.bytes, current);
});

// 이동·표적 유지 경로(Center·사거리)를 모두 지나는 제어 입력이다(관측 행 하나와 같은 입력). (2026-11-13 WB04-E355)
const MOVE_INPUT = '0,1,1,0,0,60,0,1,1,0,0,0,2,1,0,1,0';

test('FlyerFrame: 생성자는 PATCH 풀·188칸 표와 장부·현재 전역 조회기 열 개를 요구하고 pool 접근자는 같은 풀을 돌려준다', () => {
    const scene = new FlyerFrameScene('A', '0,1,0,0,0,0'.split(','));
    assert.equal(scene.body.pool, scene.pool);
    const build = (pool, types, details, stamps, globals) => () => new RawFlyerFrame(pool, types, details, stamps, globals, scene.hooks());
    assert.throws(build({ edition: 'cd1072' }, scene.types, scene.details, scene.stamps, scene.globals()), /PATCH188/);
    assert.throws(build(null, scene.types, scene.details, scene.stamps, scene.globals()), TypeError);
    assert.throws(build(scene.pool, scene.types.slice(1), scene.details, scene.stamps, scene.globals()), /PATCH188/);
    assert.throws(build(scene.pool, scene.types, scene.details.slice(1), scene.stamps, scene.globals()), /PATCH188/);
    assert.throws(build(scene.pool, scene.types, scene.details, scene.stamps.subarray(1), scene.globals()), /PATCH188/);
    assert.throws(build(scene.pool, scene.types, scene.details, scene.stamps, null), TypeError);
    // 전역은 값이 아니라 조회 함수여야 하며 빠진 이름을 알려 준다.
    for (const name of ['server', 'boss', 'checking', 'now', 'frame', 'sunType', 'windType', 'rainType', 'sunAviaryProcess', 'soundProcess']) {
        const globals = scene.globals();
        delete globals[name];
        assert.throws(build(scene.pool, scene.types, scene.details, scene.stamps, globals), (error) => error instanceof TypeError && error.message.endsWith(name), name);
        globals[name] = 5;
        assert.throws(build(scene.pool, scene.types, scene.details, scene.stamps, globals), TypeError, `${name} 값`);
    }
    // 훅 묶음을 생략해도 만들 수 있고 도달한 위치에서만 요구한다.
    assert.doesNotThrow(() => new RawFlyerFrame(scene.pool, scene.types, scene.details, scene.stamps, scene.globals()));
});

test('FlyerFrame: 훅 누락은 원인을 알려 주는 오류 문구로 거부하고 훅 묶음 생략도 같다(일반 호출 오류와 구별)', () => {
    const scene = new FlyerFrameScene('C', '1,1,60,0,144,0,0,0,0'.split(','));
    const hooks = scene.hooks();
    delete hooks.removeDepend;
    const message = (error) => error.message === '비행 Frame 실제 하위 몸체 미연결';
    assert.throws(() => new RawFlyerFrame(scene.pool, scene.types, scene.details, scene.stamps, scene.globals(), hooks).clearTarget(scene.fields), message);
    assert.throws(() => new RawFlyerFrame(scene.pool, scene.types, scene.details, scene.stamps, scene.globals()).clearTarget(scene.fields), message);
});

test('FlyerFrame: TYPE/보충 표는 만들 때 사본이라 만든 뒤 입력 표를 바꿔도 같은 결과를 낸다', () => {
    const input = MOVE_INPUT.split(',');
    const baseline = new FlyerFrameScene('F', input);
    baseline.body.baseFrame(baseline.fields);
    const changed = new FlyerFrameScene('F', input);
    // 만든 뒤 입력 표를 모두 바꾼다(발자국 50·사거리 0·소리 0).
    for (const type of changed.types) { type.footX = 50; type.footY = 50; }
    for (const detail of changed.details) { detail.range = 0; detail.sound = 0; }
    changed.body.baseFrame(changed.fields);
    assert.equal(hex(changed.fields.bytes), hex(baseline.fields.bytes));
    assert.equal(changed.objects(), baseline.objects());
    assert.deepEqual(changed.trace, baseline.trace);
    assert.ok(baseline.trace.some((line) => line.startsWith('Pop,')) && baseline.trace.includes('Capacity,60'));
});

test('FlyerFrame 이동: 속도가 NaN 이면 C++ std::min(1.0, NaN) 처럼 비율 1(표적 위치 그대로)이고 std::max 는 NaN 거리를 유지한다', () => {
    const scene = new FlyerFrameScene('F', '0,1,1,0,0,60,0,3,1,0,0,0,1,1,0,1,0'.split(','));
    // 속도 필드를 NaN 비트로 바꾼다.
    put(scene.fields.bytes, 16, 0x7fc00000);
    scene.body.baseFrame(scene.fields);
    // 표적 60 의 중심(59.25, 59.5)에 오프셋 (1, 0)을 더한 위치가 그대로 Pop 좌표이다.
    assert.ok(scene.trace.includes(`Pop,50,${floatToBits(60.25)},${floatToBits(59.5)},64`), scene.trace.join(';'));
});

test('FlyerFrame 이동: 이전 좌표가 정수 범위 밖이면 Unpop·프레임 쓰기·Pop 뒤에 절삭 범위 오류로 거부한다', () => {
    const scene = new FlyerFrameScene('F', '0,1,1,0,0,0,0,0,1,0,0,0,1,1,0,1,0'.split(','));
    // 전용 표가 아닌 외부 가상 표라 otherFrame 이 프레임을 정하고 그 효과가 부모 좌표를 NaN 으로 바꾼다.
    put(scene.fields.bytes, 0, 0x50a000);
    const hooks = scene.hooks();
    hooks.otherFrame = () => {
        put(scene.pool.allocated(50), 14, 0x7fc00000);
        return 5;
    };
    const body = new RawFlyerFrame(scene.pool, scene.types, scene.details, scene.stamps, scene.globals(), hooks);
    assert.throws(() => body.baseFrame(scene.fields), RangeError);
    assert.ok(scene.trace.some((line) => line.startsWith('Unpop,50,')) && scene.trace.some((line) => line.startsWith('Pop,50,')));
});

test('FlyerFrame 맨 오 워 프레임: 다음 프레임은 부호 있는 나머지(−0 은 0)이고 0 나눗셈·INT_MIN/−1 은 거부한다', () => {
    const run = (frameBits, count) => {
        const scene = new FlyerFrameScene('A', '0,0,0,0,0,0'.split(','));
        put(scene.pool.allocated(50), 36, frameBits);
        const hooks = scene.hooks();
        hooks.runCount = () => count;
        return new RawFlyerFrame(scene.pool, scene.types, scene.details, scene.stamps, scene.globals(), hooks).rainFrame(scene.fields);
    };
    // 프레임 −5 → 다음 −4 → 나머지는 피제수의 부호를 따른다.
    assert.equal(run(0xfffffffb, 16), -4);
    // 0x7fffffff + 1 은 INT_MIN 으로 감기고 16 으로 나눈 나머지는 부호 없는 0 이다.
    assert.ok(Object.is(run(0x7fffffff, 16), 0));
    assert.equal(run(7, 16), 8);
    assert.throws(() => run(7, 0), RangeError);
    assert.throws(() => run(0x7fffffff, -1), RangeError);
    assert.equal(run(7, -1), 0);
});

test('FlyerFrame: 표 밖 TYPE 번호는 발자국·사거리·소리 표를 읽기 전에 거부한다', () => {
    // 표적의 TYPE 번호가 188 이상이면 발자국 중심 계산이 거부된다.
    const moving = new FlyerFrameScene('F', MOVE_INPUT.split(','));
    moving.pool.physical(60)[10] = 200;
    assert.throws(() => moving.body.baseFrame(moving.fields), RangeError);
    // 부모의 TYPE 번호가 188 이상이면 맨 오 워 소리 토큰 조회가 거부된다(프레임 6·표적·공격).
    const rain = new FlyerFrameScene('A', '6,0,60,1,0,0'.split(','));
    rain.pool.physical(50)[10] = 200;
    assert.throws(() => rain.body.rainFrame(rain.fields), RangeError);
    assert.ok(rain.trace.includes('Launch,50,60,0'));
});

// 제어(F) 장면의 기본 입력이다: base 호출, 서버, boss, 부모 상태 0, +40 0, 표적 0, 점 0, 경과 0, 초기화 1, 공격 0, 착륙 0, 귀환 시각 0, 검색 시각 100, 발사 가능, 가까운 수 0, 애니메이션 101, 변이 0. (2026-11-13 WB04-E355)
const BASE_INPUT = ['0', '1', '1', '0', '0', '0', '0', '0', '1', '0', '0', '0', '1', '1', '0', '1', '0'];

// 기본 입력에서 호출자가 지정한 칸(색인 → 값)만 바꾼 제어 장면을 만든다. (2026-11-13 WB04-E355)
function frameScene(overrides = {}) {
    const input = BASE_INPUT.slice();
    for (const [index, value] of Object.entries(overrides)) input[Number(index)] = String(value);
    return new FlyerFrameScene('F', input);
}

// 장면의 훅을 호출자가 바꾼 본체를 만든다(원본 장면과 같은 풀·표·장부를 쓴다). (2026-11-13 WB04-E355)
function bodyOf(scene, change) {
    const hooks = scene.hooks();
    change(hooks);
    return new RawFlyerFrame(scene.pool, scene.types, scene.details, scene.stamps, scene.globals(), hooks);
}

// 필드의 raw double 을 읽는다. (2026-11-13 WB04-E355)
function timeOf(scene, at) {
    const buffer = Buffer.from(scene.fields.bytes);
    const view = new DataView(new ArrayBuffer(8));
    view.setUint32(0, buffer.readUInt32LE(at), true);
    view.setUint32(4, buffer.readUInt32LE(at + 4), true);
    return view.getFloat64(0, true);
}

test('FlyerFrame 초기화: 전용 표별로 갈린다 — 맨 오 워는 수명 타이머, 그 밖은 공격·착륙 0 과 표식만, 훨리기그는 sunInitialize 만', () => {
    const probe = (table) => {
        const scene = frameScene({ 8: 0 });
        put(scene.fields.bytes, 0, table);
        // 수명 타이머 사전값 5(now + 60 = 160 과 구별)와 공격 사전값 7(initialize 는 0 으로 쓴다).
        scene.timeAt(104, 5);
        put(scene.fields.bytes, 80, 7);
        let sunInit = 0;
        const body = bodyOf(scene, (hooks) => {
            hooks.sunInitialize = () => { sunInit += 1; };
            hooks.otherFrame = () => 0;
        });
        body.baseFrame(scene.fields);
        return { scene, sunInit };
    };
    const rain = probe(0x509f60);
    assert.deepEqual([rain.sunInit, timeOf(rain.scene, 104), rain.scene.fields.bytes[80], rain.scene.fields.bytes[84], timeOf(rain.scene, 72)], [0, 160, 0, 1, 120]);
    const other = probe(0x50a000);
    assert.deepEqual([other.sunInit, timeOf(other.scene, 104), other.scene.fields.bytes[80], other.scene.fields.bytes[84], timeOf(other.scene, 72)], [0, 5, 0, 1, 120]);
    // 훨리기그 전용 표는 initialize 를 쓰지 않고 sunInitialize 만 부른다(훅이 아무것도 쓰지 않으면 필드는 그대로).
    const sun = probe(0x509f24);
    assert.deepEqual([sun.sunInit, timeOf(sun.scene, 104), sun.scene.fields.bytes[80], sun.scene.fields.bytes[84]], [1, 5, 7, 0]);
    // 이미 초기화한 필드는 어느 전용 표에서도 다시 초기화하지 않는다.
    const done = frameScene();
    put(done.fields.bytes, 0, 0x509f24);
    done.timeAt(104, 5);
    let again = 0;
    bodyOf(done, (hooks) => { hooks.sunInitialize = () => { again += 1; }; hooks.otherFrame = () => 0; }).baseFrame(done.fields);
    assert.equal(again, 0);
    assert.equal(timeOf(done, 104), 5);
});

test('FlyerFrame 수명: 맨 오 워 wrapper 는 초기화 전이면 수명 타이머를 먼저 쓴 뒤 boss 일 때만 수명/정지로 삭제한다', () => {
    const run = ({ initialized, lifetime, boss, flag = 0 }) => {
        const scene = frameScene({ 2: boss ? 1 : 0, 8: initialized });
        scene.timeAt(104, lifetime);
        scene.pool.physical(50)[40] = flag;
        scene.body.frame(scene.fields);
        return scene.trace.filter((line) => line.startsWith('Destroy,'));
    };
    // 초기화 전: 수명 0 이어도 initialize(true) 가 now + 60 을 써서 삭제하지 않는다.
    assert.deepEqual(run({ initialized: 0, lifetime: 0, boss: true }), []);
    // 초기화 뒤: 수명이 지났으면(경계 포함) boss 만 삭제한다.
    assert.deepEqual(run({ initialized: 1, lifetime: 0, boss: true }), ['Destroy,50,2097152']);
    assert.deepEqual(run({ initialized: 1, lifetime: 100, boss: true }), ['Destroy,50,2097152']);
    assert.deepEqual(run({ initialized: 1, lifetime: 100.5, boss: true }), []);
    assert.deepEqual(run({ initialized: 1, lifetime: 0, boss: false }), []);
    // 부모 정지(+40 의 0x20)도 boss 일 때만 삭제하고, 다른 비트는 삭제하지 않는다.
    assert.deepEqual(run({ initialized: 1, lifetime: 160, boss: true, flag: 0x20 }), ['Destroy,50,2097152']);
    assert.deepEqual(run({ initialized: 1, lifetime: 160, boss: true, flag: 0x40 }), []);
});

test('FlyerFrame 훨리기그 표: 통지는 sunNotify 로 분배하고(공통 해제는 하지 않음) 표적 해제와 공격 통지도 전용 통지로 간다', () => {
    const scene = frameScene({ 5: 60 });
    put(scene.fields.bytes, 0, 0x509f24);
    const calls = [];
    const body = bodyOf(scene, (hooks) => {
        hooks.sunNotify = (fields, message, info1, info2) => { assert.equal(fields, scene.fields); calls.push([message, info1, info2]); scene.event('SunNotify'); };
        hooks.otherFrame = () => 0;
    });
    // 직접 통지: 인수 순서를 그대로 넘기고 공통 표적 해제(Remove)로 새지 않는다.
    body.notify(scene.fields, 2, 11, 60);
    assert.deepEqual(calls, [[2, 11, 60]]);
    assert.ok(!scene.trace.some((line) => line.startsWith('Remove')));
    // 표적 해제: 의존 해제 뒤 전용 통지(1, 0, 0) → 갱신 순서이다.
    scene.trace.length = 0;
    calls.length = 0;
    put(scene.fields.bytes, 12, 60, 2);
    body.clearTarget(scene.fields);
    assert.deepEqual(scene.trace, ['Remove,70,60', 'SunNotify', 'Refresh']);
    assert.deepEqual(calls, [[1, 0, 0]]);
    // 이동 중 공격 해제(거리 ≥ 반경)와 공격 시작(거리 < 반경)의 통지이다.
    const attackEnd = frameScene({ 5: 60, 7: 3, 9: 1 });
    put(attackEnd.fields.bytes, 0, 0x509f24);
    const endCalls = [];
    bodyOf(attackEnd, (hooks) => { hooks.sunNotify = (f, m, a, b) => { endCalls.push([m, a, b]); }; hooks.otherFrame = () => 0; }).baseFrame(attackEnd.fields);
    assert.deepEqual(endCalls, [[1, 0, 0]]);
    const attackStart = frameScene({ 5: 60, 7: 3, 9: 0 });
    put(attackStart.fields.bytes, 0, 0x509f24);
    put(attackStart.fields.bytes, 20, floatToBits(100));
    const startCalls = [];
    bodyOf(attackStart, (hooks) => { hooks.sunNotify = (f, m, a, b) => { startCalls.push([m, a, b]); }; hooks.otherFrame = () => 0; }).baseFrame(attackStart.fields);
    assert.deepEqual(startCalls, [[1, 1, 0]]);
    assert.equal(attackStart.fields.bytes[80], 1);
});

test('FlyerFrame 진단: 줄 번호 0x42c·0x431·0x434·0x447·0x44b·0x46f 는 각 조건에서만 정확한 번호로 낸다', () => {
    const lines = (scene, body = scene.body) => { body.baseFrame(scene.fields); return scene.trace.filter((line) => line.startsWith('Assert,')); };
    // 0x42c(1068): checking 이 켜졌고 robust 가 거짓일 때만. 꺼져 있으면 robust 를 부르지도 않는다.
    const robustFalse = frameScene();
    robustFalse.checking = true;
    assert.deepEqual(lines(robustFalse, bodyOf(robustFalse, (hooks) => { hooks.robust = () => { robustFalse.event('Robust'); return false; }; })), ['Assert,1068']);
    const robustTrue = frameScene();
    robustTrue.checking = true;
    assert.deepEqual(lines(robustTrue), []);
    assert.ok(robustTrue.trace.includes('Robust'));
    const quiet = frameScene();
    assert.deepEqual(lines(quiet, bodyOf(quiet, (hooks) => { hooks.robust = () => { quiet.event('Robust'); return false; }; })), []);
    assert.ok(!quiet.trace.includes('Robust'));
    // 0x431(1073): 홈이 0 이 아닌데 유효하지 않을 때만(홈 0 은 검사하지 않는다).
    const badHome = frameScene();
    put(badHome.fields.bytes, 10, 65535, 2);
    assert.deepEqual(lines(badHome), ['Assert,1073']);
    assert.ok(badHome.trace.includes('Sid,65535,0'));
    const noHome = frameScene();
    put(noHome.fields.bytes, 10, 0, 2);
    assert.deepEqual(lines(noHome), []);
    assert.ok(!noHome.trace.some((line) => line.startsWith('Sid,0,')));
    // 0x434(1076): 부모가 유효하지 않을 때(부모 0).
    const badParent = frameScene();
    put(badParent.fields.bytes, 56, 0);
    badParent.pool.physical(0)[11] = 0;
    assert.deepEqual(lines(badParent), ['Assert,1076']);
    // 0x447(1095): 경과 시간이 양수인데 부모 위치가 유효하지 않을 때만.
    const badPoint = frameScene({ 7: 3 });
    put(badPoint.pool.physical(50), 14, floatToBits(300));
    assert.deepEqual(lines(badPoint), ['Assert,1095']);
    const goodPoint = frameScene({ 7: 3 });
    assert.deepEqual(lines(goodPoint), []);
    const idle = frameScene({ 7: 0 });
    put(idle.pool.physical(50), 14, floatToBits(300));
    assert.deepEqual(lines(idle), []);
    // 0x44b(1099): 서버의 귀환 기한에서 부모가 훨리기그 타입이 아닐 때만.
    const wrongType = frameScene({ 7: 3, 11: 1 });
    wrongType.pool.physical(50)[10] = 144;
    assert.ok(lines(wrongType).includes('Assert,1099'));
    const rightType = frameScene({ 7: 3, 11: 1 });
    assert.ok(!lines(rightType).includes('Assert,1099'));
    // 0x46f(1135): 표적이 홈일 때 부모가 훨리기그 타입이 아니면.
    const homeTarget = frameScene({ 5: 61, 7: 3 });
    homeTarget.pool.physical(50)[10] = 144;
    assert.ok(lines(homeTarget).includes('Assert,1135'));
    const homeSun = frameScene({ 5: 61, 7: 3 });
    assert.ok(!lines(homeSun).includes('Assert,1135'));
    const foreign = frameScene({ 5: 60, 7: 3 });
    foreign.pool.physical(50)[10] = 144;
    assert.ok(!lines(foreign).includes('Assert,1135'));
});

test('FlyerFrame 위치 검사: 0 이하·256 이상·NaN 은 유효하지 않아 진단 0x447 을 내고 Unpop/Pop 을 하지 않는다', () => {
    // [x, y, 유효 여부]
    for (const [x, y, ok] of [[1, 1, true], [255.5, 255.5, true], [0, 1, false], [1, 0, false], [-1, 1, false], [1, -1, false], [256, 1, false], [1, 256, false], [Number.NaN, 1, false], [1, Number.NaN, false]]) {
        const scene = frameScene({ 7: 3 });
        put(scene.fields.bytes, 0, 0x50a000);
        put(scene.pool.physical(50), 14, floatToBits(x));
        put(scene.pool.physical(50), 18, floatToBits(y));
        // 프레임이 달라 위치가 유효할 때만 Unpop → Pop 이 일어난다.
        bodyOf(scene, (hooks) => { hooks.otherFrame = () => 5; }).baseFrame(scene.fields);
        assert.equal(scene.trace.includes('Assert,1095'), !ok, `(${x}, ${y}) 진단`);
        assert.equal(scene.trace.some((line) => line.startsWith('Pop,')), ok, `(${x}, ${y}) Pop`);
    }
});

test('FlyerFrame 이동: 표적 위치는 1..255 로 자른 뒤 쓰고(NaN 은 1) NaN 속도에서는 비율 1 로 그 위치에 닿는다', () => {
    const popAt = (x, y) => {
        const scene = frameScene({ 5: 60, 7: 3 });
        put(scene.fields.bytes, 16, 0x7fc00000);
        // 표적 중심 = 위치 − 1(발자국 2 의 절반), 오프셋 (1, 0)을 더한 뒤 자른다.
        put(scene.pool.physical(60), 14, floatToBits(x));
        put(scene.pool.physical(60), 18, floatToBits(y));
        const popped = [];
        bodyOf(scene, (hooks) => {
            const pop = hooks.pop;
            hooks.pop = (sid, px, py, flags) => { popped.push([px, py]); pop(sid, px, py, flags); };
        }).baseFrame(scene.fields);
        return { popped, scene };
    };
    // y 중심 0.5 → 1 로 올린다(0 이나 0.5 가 아님).
    assert.deepEqual(popAt(1.5, 1.5).popped, [[1.5, 1]]);
    // 255.5 − 1 + 1 = 255.5 → x 는 255 로 줄이고 y 254.5 는 그대로 둔다.
    assert.deepEqual(popAt(255.5, 255.5).popped, [[255, 254.5]]);
    // NaN 위치는 원본 비교 순서대로 1 이 된다.
    assert.deepEqual(popAt(Number.NaN, 10).popped[0][0], 1);
    // 자른 값이 유효하므로 0x487 진단은 나오지 않는다.
    assert.ok(!popAt(1.5, 1.5).scene.trace.includes('Assert,1159'));
});

test('FlyerFrame 이동: 거리가 0 이어도 하한 거리(float 0.0001)로 반경과 비교하고, 반경 안의 홈은 거리 1 미만일 때만 서버가 착륙한다', () => {
    // 표적 중심 + 오프셋 = 부모 위치(거리 0): 하한 거리 0.0001 이 반경 0.00005 이상이라 시각 갱신 분기로 간다.
    const exact = frameScene({ 5: 60, 6: 1, 7: 3 });
    put(exact.fields.bytes, 20, floatToBits(0.00005));
    exact.body.baseFrame(exact.fields);
    assert.ok(exact.trace.includes('Time,50'));
    // 홈 표적: 거리 0.5(< 1)면 서버가 착륙, 거리 1.5 이거나 서버가 아니면 착륙하지 않는다.
    const landing = (x, server) => {
        const scene = frameScene({ 5: 61, 7: 3, 1: server ? 1 : 0 });
        put(scene.fields.bytes, 20, floatToBits(100));
        put(scene.pool.physical(61), 14, floatToBits(x));
        put(scene.pool.physical(61), 18, floatToBits(51.5));
        scene.body.baseFrame(scene.fields);
        return scene.trace.filter((line) => line.startsWith('Sun,'));
    };
    assert.deepEqual(landing(50.75, true), ['Sun,61,32']);
    assert.deepEqual(landing(51.75, true), []);
    assert.deepEqual(landing(50.75, false), []);
});

test('FlyerFrame 이동: 위치 변화는 두 좌표를 모두 비교하고 칸 변화는 절삭(0 방향) 기준이며 정수 범위 끝과 밖을 구별한다', () => {
    // 부모 위치가 이동 도중 old 로 바뀐 뒤 현재 위치(point)로 Pop 하는 경우를 만든다(otherFrame 효과).
    const run = (old, point, frame = 0) => {
        const scene = frameScene({ 7: 0 });
        put(scene.fields.bytes, 0, 0x50a000);
        put(scene.pool.physical(50), 14, floatToBits(point[0]));
        put(scene.pool.physical(50), 18, floatToBits(point[1]));
        const body = bodyOf(scene, (hooks) => {
            hooks.otherFrame = () => {
                put(scene.pool.physical(50), 14, floatToBits(old[0]));
                put(scene.pool.physical(50), 18, floatToBits(old[1]));
                return frame;
            };
        });
        let error = null;
        try { body.baseFrame(scene.fields); } catch (caught) { error = caught; }
        return { trace: scene.trace, error };
    };
    const popped = (result) => result.trace.some((line) => line.startsWith('Pop,'));
    const cell = (result) => result.trace.some((line) => line.startsWith('Cell,'));
    // y 만 다르면 위치가 바뀐 것이다(Unpop/Pop). 같은 칸이면 칸 변화 통지는 없다.
    const yOnly = run([10, 20.5], [10, 20.2]);
    assert.ok(popped(yOnly) && !cell(yOnly));
    // 같은 위치·같은 프레임이면 아무것도 하지 않는다.
    const same = run([10, 20.5], [10, 20.5]);
    assert.ok(!popped(same));
    // 칸은 x 또는 y 중 하나만 달라도 바뀐 것이다.
    assert.ok(cell(run([10, 21.5], [10, 20.2])));
    assert.ok(cell(run([11.5, 20.5], [10.2, 20.5])));
    // 0 방향 절삭: −0.5 와 0.25 는 둘 다 칸 0, 10.6 과 10.2 는 둘 다 칸 10 이다(floor/반올림이면 달라진다).
    assert.ok(!cell(run([-0.5, 10], [0.25, 10])));
    assert.ok(!cell(run([10.6, 10], [10.2, 10])));
    // 정수 범위: −2^31 은 허용하고 2^31 과 그 아래 다음 float 는 효과(Pop) 뒤에 거부한다.
    const lowest = run([-2147483648, 10], [10, 10]);
    assert.equal(lowest.error, null);
    assert.ok(cell(lowest));
    const above = run([2147483648, 10], [10, 10]);
    assert.ok(above.error instanceof RangeError && popped(above));
    const below = run([-2147483904, 10], [10, 10]);
    assert.ok(below.error instanceof RangeError && popped(below));
    const missing = run([Number.NaN, 10], [10, 10]);
    assert.ok(missing.error instanceof RangeError);
});

test('FlyerFrame 홈 통지: 더스트 데빌·빈 홈·범위 밖 홈은 생략하고 살아 있는 홈(free/dead 아님)만 가상 34 를 부른다', () => {
    const run = ({ home, state, parentType = 144 }) => {
        const scene = frameScene();
        scene.pool.physical(50)[10] = parentType;
        put(scene.fields.bytes, 10, home, 2);
        if (state !== undefined) scene.pool.physical(home)[11] = state;
        scene.body.parentDying(scene.fields);
        return scene.trace;
    };
    assert.deepEqual(run({ home: 61, state: 0 }), ['Home,61,0,0']);
    // 더스트 데빌(windType)은 생략한다.
    assert.deepEqual(run({ home: 61, state: 0, parentType: 138 }), []);
    // free(1)·dead(2) 홈은 생략하고 다른 비트(4)만 있으면 통지한다.
    assert.deepEqual(run({ home: 61, state: 1 }), []);
    assert.deepEqual(run({ home: 61, state: 2 }), []);
    assert.deepEqual(run({ home: 61, state: 4 }), ['Home,61,0,0']);
    // 홈 0 은 슬롯 0 이 살아 있어도 생략한다.
    assert.deepEqual(run({ home: 0, state: 0 }), []);
    // 풀 크기와 같은 번호(경계)·0xffff 도 읽지 않고 생략한다.
    assert.deepEqual(run({ home: 24000 }), []);
    assert.deepEqual(run({ home: 0xffff }), []);
});

test('FlyerFrame 귀환 기한: 서버가 기한이 된 훨리기그는 홈 조건이 모두 맞을 때만 홈을 표적으로 삼고 하나라도 어긋나면 종료한다', () => {
    const run = ({ home = 61, state, link = 50, dword, target = 0, server = true }) => {
        const scene = frameScene({ 5: target, 7: 3, 11: 1, 1: server ? 1 : 0 });
        put(scene.fields.bytes, 10, home, 2);
        if (dword !== undefined) put(scene.fields.bytes, 56, dword);
        if (home < 24000) {
            const raw = scene.pool.physical(home);
            if (state !== undefined) raw[11] = state;
            put(raw, 12, link, 2);
        }
        scene.body.baseFrame(scene.fields);
        return scene.trace;
    };
    const finished = (trace) => trace.includes('Finish');
    const adopted = (trace) => trace.includes('Command,50,3');
    // 모든 조건이 맞으면 홈을 표적으로 삼고(의존 추가·갱신·명령 3) 종료하지 않는다.
    const ok = run({});
    assert.ok(adopted(ok) && !finished(ok));
    assert.ok(ok.includes('Add,61,70'));
    // 서버가 아니면 이 분기를 건너뛴다.
    assert.ok(!adopted(run({ server: false })) && !finished(run({ server: false })));
    // 홈 0·풀 크기·free(1) 홈·부모 WORD 불일치·부모 DWORD 불일치는 모두 종료한다. dead(2) 홈은 허용한다.
    const zeroHome = (() => {
        const scene = frameScene({ 7: 3, 11: 1 });
        put(scene.fields.bytes, 10, 0, 2);
        // 슬롯 0 이 부모와 연결돼 있어도 홈 0 은 종료해야 한다.
        const raw = scene.pool.physical(0);
        raw[11] = 0;
        put(raw, 12, 50, 2);
        scene.body.baseFrame(scene.fields);
        return scene.trace;
    })();
    assert.ok(finished(zeroHome) && !adopted(zeroHome));
    assert.ok(finished(run({ home: 24000 })));
    assert.ok(finished(run({ state: 1 })));
    assert.ok(!finished(run({ state: 2 })) && adopted(run({ state: 2 })));
    assert.ok(finished(run({ link: 51 })));
    assert.ok(finished(run({ dword: 0x10032 })));
    // 표적이 있으면 홈으로 바꾸기 전에 의존을 해제하고, 없으면 해제하지 않는다.
    assert.ok(run({ target: 60 }).includes('Remove,70,60'));
    assert.ok(!run({ target: 0 }).some((line) => line.startsWith('Remove,')));
});

test('FlyerFrame 공격: 더스트 데빌이 아닐 때만 hit 을 부르고 표적은 효과 뒤 현재 값을 쓴다', () => {
    const run = (type, retarget) => {
        const scene = frameScene({ 5: 60, 7: 3, 9: 1 });
        put(scene.fields.bytes, 20, floatToBits(100));
        scene.pool.physical(50)[10] = type;
        bodyOf(scene, (hooks) => {
            const offset = hooks.offset;
            hooks.offset = (sid, home) => {
                if (retarget) put(scene.fields.bytes, 12, 63, 2);
                return offset(sid, home);
            };
        }).baseFrame(scene.fields);
        return scene.trace.filter((line) => line.startsWith('Hit,'));
    };
    assert.deepEqual(run(144, false), ['Hit,50,60']);
    assert.deepEqual(run(138, false), []);
    assert.deepEqual(run(144, true), ['Hit,50,63']);
});

test('FlyerFrame 서버 후반: 서버가 아니면 표적 유지/찾기를 하지 않고, 서버면 효과 뒤 현재 부모의 사거리와 슬롯 값을 쓴다', () => {
    const client = frameScene({ 1: 0, 5: 60 });
    client.body.baseFrame(client.fields);
    assert.ok(!client.trace.some((line) => /^(Check|May|Closer|Capacity|Find)/.test(line)));
    // unpop 효과가 부모를 51 로 바꾸면 사거리는 슬롯 51 의 TYPE(73, 사거리 7)에서 읽는다.
    const scene = frameScene({ 5: 60, 7: 0 });
    put(scene.fields.bytes, 0, 0x50a000);
    scene.pool.physical(51)[10] = 73;
    scene.details[73] = { range: 7, sound: 473 };
    const hooks = scene.hooks();
    const unpop = hooks.unpop;
    hooks.unpop = (sid, flags) => { unpop(sid, flags); put(scene.fields.bytes, 56, 51); };
    hooks.otherFrame = () => 5;
    // 본체는 TYPE/보충 표의 만들 때 사본을 쓰므로 만들기 전에 표를 바꿔 둔다.
    const body = new RawFlyerFrame(scene.pool, scene.types, scene.details, scene.stamps, scene.globals(), hooks);
    body.baseFrame(scene.fields);
    assert.ok(scene.trace.includes('Check,51'));
    assert.ok(scene.trace.some((line) => line.startsWith('Remove,')));
    // 훨리기그/표적 소스 좌표(+42/+46)는 부호 있는 정수이며 x 가 먼저이다.
    const reach = (x, y, range) => {
        const next = frameScene({ 5: 60, 7: 0 });
        put(next.fields.bytes, 0, 0x50a000);
        put(next.pool.physical(50), 42, x);
        put(next.pool.physical(50), 46, y);
        next.details[144] = { range, sound: 544 };
        const probeBody = bodyOf(next, (probeHooks) => { probeHooks.otherFrame = () => 0; });
        probeBody.baseFrame(next.fields);
        return next.trace.some((line) => line.startsWith('Remove,'));
    };
    // (+42, +46) = (50, 0): 표적 중심(59.25, 59.5)까지 거리 60.21(교환하면 60.01). 사거리 60.1 은 둘을 가른다.
    assert.equal(reach(50, 0, 60.1), true);
    // 음수 좌표(−5, 7)는 부호 있는 정수로 읽는다(부호 없이 읽으면 사거리를 한참 벗어난다).
    assert.equal(reach(0xfffffffb, 7, 100), false);
    assert.equal(reach(0xfffffffb, 7, 82.9), true);
});

test('FlyerFrame 이동 계산: float 로 저장하는 중간 단계(오프셋 합·포위 합·delta·경과 snapshot·최종 위치)를 원문 순서로 반올림한다', () => {
    const f = Math.fround;
    // C++ 반올림 순서를 단계별로 쓴 독립 계산이다(발자국 2 의 절반 1 을 뺀 표적 중심 → 두 벡터 합 → 1..255 → delta → 거리 → 비율 → 최종).
    const expected = ({ target, offset, orbit, point, speed, last }) => {
        let t = [f(target[0] - 1), f(target[1] - 1)];
        t = [f(t[0] + offset[0]), f(t[1] + offset[1])];
        t = [f(t[0] + orbit[0]), f(t[1] + orbit[1])];
        t = t.map((value) => Math.min(255, value >= 1 ? value : 1));
        const delta = [f(t[0] - point[0]), f(t[1] - point[1])];
        const dx = t[0] - point[0];
        const dy = t[1] - point[1];
        const measured = Math.sqrt(dx * dx + dy * dy);
        const separation = measured < 0.00009999999747378752 ? 0.00009999999747378752 : measured;
        const scaled = (speed / separation) * f(100 - last);
        const ratio = scaled < 1 ? scaled : 1;
        return [f(ratio * delta[0] + point[0]), f(ratio * delta[1] + point[1])];
    };
    // 단계마다 그 반올림을 생략하면 최종 위치가 달라지는 입력(탐색으로 찾은 값)이다.
    const cases = [
        { target: [118.80742645263672, 65.60808563232422], offset: [-0.4079, 0.5738], orbit: [0.535, 0.6613], point: [126.83639526367188, 63.71217727661133], speed: 0.8659999966621399, last: 98.024 },
        { target: [122.51282501220703, 121.18647766113281], offset: [1.486, -0.2266], orbit: [0.0142, 0.2337], point: [148.84605407714844, 121.95648956298828], speed: 0.7239999771118164, last: 98.1587 },
        { target: [150.05520629882812, 176.91200256347656], offset: [0.2181, 1.2128], orbit: [1.426, -0.1812], point: [134.1929473876953, 133.36773681640625], speed: 1.2769999504089355, last: 99.1351 },
        { target: [71.02458190917969, 76.18573760986328], offset: [-0.0803, -0.0246], orbit: [0.9772, -0.3591], point: [144.88673400878906, 114.49522399902344], speed: 0.5649999976158142, last: 98.6892 },
        { target: [140.5410919189453, 127.52780151367188], offset: [0.5105, -0.1717], orbit: [-0.3099, -0.3552], point: [49.26841354370117, 138.47572326660156], speed: 2.0920000076293945, last: 98.9883 },
    ];
    for (const params of cases) {
        const scene = frameScene({ 5: 60, 7: 0 });
        put(scene.pool.physical(50), 14, floatToBits(params.point[0]));
        put(scene.pool.physical(50), 18, floatToBits(params.point[1]));
        put(scene.pool.physical(60), 14, floatToBits(params.target[0]));
        put(scene.pool.physical(60), 18, floatToBits(params.target[1]));
        put(scene.fields.bytes, 16, floatToBits(params.speed));
        scene.timeAt(32, params.last);
        const popped = [];
        bodyOf(scene, (hooks) => {
            const pop = hooks.pop;
            hooks.pop = (sid, x, y, flags) => { popped.push([x, y]); pop(sid, x, y, flags); };
            hooks.offset = () => params.offset;
            hooks.scatter = () => params.orbit;
        }).baseFrame(scene.fields);
        assert.deepEqual(popped, [expected(params)], JSON.stringify(params));
    }
});

test('FlyerFrame 맨 오 워 프레임: 애니메이션 기한 전에는 현재 프레임을 부호 있는 수로 돌려준다', () => {
    const scene = new FlyerFrameScene('A', '4294967291,2,0,0,0,0'.split(','));
    assert.equal(scene.body.rainFrame(scene.fields), -5);
});

test('FlyerFrame 이동: 표적 중심은 x 에 발자국 X, y 에 발자국 Y 의 절반을 빼고, 거리가 0 이면 하한 거리가 반경 비교에 쓰인다', () => {
    // 발자국 (2, 6) → 중심 = 위치 − (1, 3). NaN 속도로 비율 1 이라 Pop 이 곧 표적 위치(오프셋 (1, 0) 포함)이다.
    const scene = frameScene({ 5: 60, 7: 3 });
    scene.types[80].footY = 6;
    put(scene.fields.bytes, 16, 0x7fc00000);
    put(scene.pool.physical(60), 14, floatToBits(50.5));
    put(scene.pool.physical(60), 18, floatToBits(50.5));
    const popped = [];
    bodyOf(scene, (hooks) => {
        const pop = hooks.pop;
        hooks.pop = (sid, x, y, flags) => { popped.push([x, y]); pop(sid, x, y, flags); };
    }).baseFrame(scene.fields);
    assert.deepEqual(popped, [[50.5, 47.5]]);
    // 거리 0(표적 중심 + 오프셋 = 부모 위치)이면 하한 거리가 반경 0.00005 이상이라 공격이 시작되지 않는다.
    const exact = frameScene({ 5: 60, 6: 1, 7: 3 });
    put(exact.fields.bytes, 20, floatToBits(0.00005));
    exact.body.baseFrame(exact.fields);
    assert.ok(exact.trace.includes('Time,50'));
    assert.equal(exact.fields.bytes[80], 0);
});

test('FlyerFrame 귀환 기한: 기한이 지금과 같아도 기한이 된 것이고, 명령 효과로 바뀐 부모 위치와 부모 번호를 이후에 다시 읽는다', () => {
    // 기한 == now 이면 기한이다(< 가 아니라 <=).
    const edge = frameScene({ 7: 3, 11: 1 });
    edge.timeAt(40, 100);
    edge.body.baseFrame(edge.fields);
    assert.ok(edge.trace.includes('Command,50,3'));
    // 명령 효과가 표적을 지우고 부모 위치를 옮기면 이동 위치는 그 현재 위치이다(옛 사본이 아님).
    const moved = frameScene({ 7: 3, 11: 1 });
    bodyOf(moved, (hooks) => {
        const command = hooks.command;
        hooks.command = (sid, value) => {
            command(sid, value);
            put(moved.fields.bytes, 12, 0, 2);
            put(moved.pool.physical(50), 14, floatToBits(70.5));
            put(moved.pool.physical(50), 18, floatToBits(71.5));
        };
    }).baseFrame(moved.fields);
    assert.ok(!moved.trace.some((line) => line.startsWith('Pop,')));
    // 홈 표적 이동의 오프셋은 (진입 때 부모, 홈 여부)로, 포위 위치는 (표적, 현재 부모)로 요청한다.
    const homing = frameScene({ 7: 3, 11: 1 });
    const offsets = [];
    bodyOf(homing, (hooks) => {
        const command = hooks.command;
        const offset = hooks.offset;
        hooks.command = (sid, value) => { command(sid, value); put(homing.fields.bytes, 56, 51); };
        hooks.offset = (sid, home) => { offsets.push([sid, home]); return offset(sid, home); };
    }).baseFrame(homing.fields);
    assert.deepEqual(offsets, [[50, true]]);
    assert.ok(homing.trace.includes('Scatter,61,51'));
    const other = frameScene({ 5: 60, 7: 3 });
    const otherOffsets = [];
    bodyOf(other, (hooks) => {
        const offset = hooks.offset;
        hooks.offset = (sid, home) => { otherOffsets.push([sid, home]); return offset(sid, home); };
    }).baseFrame(other.fields);
    assert.deepEqual(otherOffsets, [[50, false]]);
});

test('FlyerFrame 서버 후반: 표적은 발사 가능 효과 전에 읽은 값으로 중심을 구하고, 사거리 비교는 경계·음수 좌표·큰 좌표·NaN 에서도 원문과 같다', () => {
    // mayTarget 효과가 표적 필드를 바꿔도 중심은 처음 읽은 표적(60)의 것이다(63 은 먼 위치).
    const scene = frameScene({ 5: 60, 7: 0 });
    put(scene.fields.bytes, 0, 0x50a000);
    put(scene.pool.physical(63), 14, floatToBits(200));
    put(scene.pool.physical(63), 18, floatToBits(200));
    bodyOf(scene, (hooks) => {
        const may = hooks.mayTarget;
        hooks.mayTarget = (sid, target) => { const result = may(sid, target); put(scene.fields.bytes, 12, 63, 2); return result; };
        hooks.otherFrame = () => 0;
    }).baseFrame(scene.fields);
    assert.ok(!scene.trace.some((line) => line.startsWith('Remove,')));
    // 사거리 비교: [원본 +42, +46, 표적 위치, 사거리, 해제 여부]
    const reach = (x, y, spot, range) => {
        const next = frameScene({ 5: 60, 7: 0 });
        put(next.fields.bytes, 0, 0x50a000);
        put(next.pool.physical(50), 42, x);
        put(next.pool.physical(50), 46, y);
        put(next.pool.physical(60), 14, floatToBits(spot));
        put(next.pool.physical(60), 18, floatToBits(spot));
        next.details[144] = { range, sound: 544 };
        bodyOf(next, (hooks) => { hooks.otherFrame = () => 0; }).baseFrame(next.fields);
        return next.trace.some((line) => line.startsWith('Remove,'));
    };
    // 표적 중심 (60, 60), 소스 (57, 56) → 거리 정확히 5: 사거리 5 는 벗어난 것(해제), 5.5 는 안.
    assert.equal(reach(57, 56, 61, 5), true);
    assert.equal(reach(57, 56, 61, 5.5), false);
    // y 가 음수(−5)여도 부호 있는 정수로 읽는다: 중심 (59.25, 59.5) → delta (52.25, 64.5) 거리 83.0.
    assert.equal(reach(7, 0xfffffffb, 61.25, 100), false);
    assert.equal(reach(7, 0xfffffffb, 61.25, 82), true);
    // 큰 좌표: float 로 줄인 delta(1073741760)가 사거리와 같아져 해제된다(줄이지 않은 1073741738.75 는 사거리 안).
    assert.equal(reach(1073741799, 60, 61.25, 1073741760), true);
    // 사거리가 NaN 이면 비교가 거짓이라 항상 해제한다.
    assert.equal(reach(57, 56, 61, Number.NaN), true);
});

test('FlyerFrame 이동 계산: delta 의 y 축도 float 로 저장한다', () => {
    const f = Math.fround;
    const params = { target: [150.70188903808594, 146.71328735351562], offset: [1.0066, 1.1077], orbit: [1.3987, -0.1208], point: [154.4752197265625, 31.98451042175293], speed: 2.2990000247955322, last: 98.563 };
    const scene = frameScene({ 5: 60, 7: 0 });
    put(scene.pool.physical(50), 14, floatToBits(params.point[0]));
    put(scene.pool.physical(50), 18, floatToBits(params.point[1]));
    put(scene.pool.physical(60), 14, floatToBits(params.target[0]));
    put(scene.pool.physical(60), 18, floatToBits(params.target[1]));
    put(scene.fields.bytes, 16, floatToBits(params.speed));
    scene.timeAt(32, params.last);
    const popped = [];
    bodyOf(scene, (hooks) => {
        const pop = hooks.pop;
        hooks.pop = (sid, x, y, flags) => { popped.push([x, y]); pop(sid, x, y, flags); };
        hooks.offset = () => params.offset;
        hooks.scatter = () => params.orbit;
    }).baseFrame(scene.fields);
    let t = [f(params.target[0] - 1), f(params.target[1] - 1)];
    t = [f(t[0] + params.offset[0]), f(t[1] + params.offset[1])];
    t = [f(t[0] + params.orbit[0]), f(t[1] + params.orbit[1])];
    const delta = [f(t[0] - params.point[0]), f(t[1] - params.point[1])];
    const measured = Math.sqrt((t[0] - params.point[0]) ** 2 + (t[1] - params.point[1]) ** 2);
    const ratio = Math.min(1, (params.speed / measured) * f(100 - params.last));
    assert.deepEqual(popped, [[f(ratio * delta[0] + params.point[0]), f(ratio * delta[1] + params.point[1])]]);
});
