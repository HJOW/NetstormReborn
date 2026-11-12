// 경로 프로세스 raw-path-process.js(RawPathProcess·RawPathProcessState)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: pathruntime-x86.tsv 의 `originals` frame 60행(텔레포트/사제 × 현재 단계 × 시각 × 각도의 실제 Kernel RunFrame 뒤 부모 필드·경로 이미지·가상 요청 순서)과 arrival 12행(사제 여부 × 도착 반환 × Halt),
//    flypath-x86.tsv 의 frame 63행(비행 39 의 20ms 예약·Halt·Step 순서와 다음 프레임 시각 QWORD), walk-shape-x86.tsv 1,170행(실제 보호 사제/골렘 SHP 의 모든 논리 프레임에서 RunFrame 결과).
//    `pathprepare-x86.tsv` originals 24행(가상 Prepare 의 nonzero override/타입 속도 선택별 꼭짓점 시각 QWORD)도 쓴다.
//    cpppj RawPathRuntimeTests.cpp 의 PathScene(실제 풀·해시·Pop/Unpop·Kernel·호스트 위에서 부모의 외부 가상 효과만 입력 스크립트로 응답)을 웹 모듈로 같은 순서에 옮겼다.
//  - cpppj 시험 이식: 존재하지 않는 검색 경계와 얼어붙음(0x5105) 부모, 자기 삭제 뒤 진단 복귀(Halt checking 3입력·도착 4변형·예약 Form 0 세 변형).
//  - 웹 계약: 생성자 연결 검사·종류별 프로세스 번호·부착 검사/기존 프로세스 제거·Prepare 의 속도 선택·길 밖 정지·종료 통지(onFormDestroy)·삭제 감시(destroy)·상태 객체의 checking/마스크 대여.
// 10.72(CD) 행은 범위 밖이다. 실제 게임·브라우저 실행이 아니다. (2026-11-13 WB04-E349: 6-P RawPathProcess 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get, bitsToFloat, floatToBits } from './raw-support.mjs';
import { dudeAssets } from './dude-spawn-support.mjs';
import { FrameCodeTable } from '../www/assets/terrain-tiles.js';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { SquidUnpop } from '../www/core/squid-unpop.js';
import { SquidPostPopState } from '../www/core/squid-postpop-state.js';
import { SquidPostPop } from '../www/core/squid-postpop.js';
import { SquidPop, RAW_POP_RESULT } from '../www/core/squid-pop.js';
import { SquidFactory } from '../www/core/squid-factory.js';
import { RawSquidDestroy } from '../www/core/squid-destroy.js';
import { typeConstructorAddress } from '../www/core/squid-constructors.js';
import { Kernel } from '../www/core/kernel.js';
import { SquidProcess, SquidProcessHost, SquidProcessState } from '../www/core/squid-process.js';
import { PATH_LAYOUT, RawPathRuntime } from '../www/core/raw-path-runtime.js';
import { RawPathProcess, RawPathProcessState } from '../www/core/raw-path-process.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-13 WB04-E349)
const fixtures = path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures');
const runtimeRows = readRows(path.join(fixtures, 'pathruntime-x86.tsv'));
const flightRows = readRows(path.join(fixtures, 'flypath-x86.tsv'));
const walkRows = readRows(path.join(fixtures, 'walk-shape-x86.tsv'));
const prepareRows = readRows(path.join(fixtures, 'pathprepare-x86.tsv'));

// 도착 진단이 보고하는 원문 파일 이름이다. (2026-11-13 WB04-E349)
const PATH_FILE = '\\Ns\\O\\Pathprocess.cpp';

// 예외가 나는지 본다. (2026-11-13 WB04-E349)
function throws(action, kind) {
    try { action(); } catch (error) { return kind ? error instanceof kind : true; }
    return false;
}

// uint64 십진 문자열의 비트 패턴을 double 로 읽는다. (2026-11-13 WB04-E349)
function doubleOf(text) {
    const view = new DataView(new ArrayBuffer(8));
    view.setBigUint64(0, BigInt(text), true);
    return view.getFloat64(0, true);
}

// 내장 Form 네 생성자 주소만 넣고 사제(158)/골렘(134)의 발자국을 1 로 둔 188칸 TYPE 입력이다(cpppj ProcessTypes). (2026-11-13 WB04-E349)
function pathTypes() {
    const types = Array.from({ length: 188 }, (_, number) => ({ cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, ...([1, 2, 3, 6].includes(number) ? { constructorAddress: typeConstructorAddress(number) } : {}) }));
    types[134].footX = types[134].footY = types[158].footX = types[158].footY = 1;
    return types;
}

// 실제 풀·해시·Pop/Unpop·Kernel·호스트 위에서 경로 프로세스를 실행하는 장면이다. 부모의 외부 가상 효과(Halt/도착/소리/정리)만 입력 스크립트로 응답하고 기록한다.
// parent 는 SID 5(client 번호), 경로 Form 은 SID 7 이다(중간 SID 6 은 할당만 한 void 입력). (2026-11-13 WB04-E349)
class PathScene {
    // server 가 참이면 서버 번호 Form 도 할당할 수 있는 풀을 쓴다(원본 검사는 항상 false, 서버 번호 변형 시험만 참).
    constructor(priest, flying = false, server = false) {
        const pool = new SidPool(32768, { server });
        this.pool = pool;
        this.hash = new SquidHash();
        this.types = pathTypes();
        this.frames = this.types.map(() => new FrameCodeTable([]));
        this.shapes = this.types.map(() => ({ frames: [] }));
        this.spots = new Uint8Array(65536).fill(6);
        const book = new SquidPostPopState();
        book.suppressed = true;
        this.unpop = new SquidUnpop(pool, this.hash, this.spots);
        this.pop = new SquidPop(pool, this.hash, this.spots, { postPop: new SquidPostPop(pool, this.types, book) });
        this.kernel = new Kernel(32);
        this.clock = new SquidProcessState();
        // 호스트 진단 기록(정상 복귀)이다: 서버 번호 Form 부착이 먼저 부르는 원문 진단을 시험이 관찰한다.
        this.hostDiagnostics = [];
        this.clock.assertion = (expression, file, line) => { this.hostDiagnostics.push(`${file}:${line}:${expression}`); };
        this.externalDeletion = [];
        this.host = new SquidProcessHost(pool, this.types, this.kernel, new RawSquidDestroy(pool, this.unpop, this.types), this.clock, {
            asset: { emit: (event) => { this.externalDeletion.push(event); }, selected: () => 0 },
            // 서버 번호 Form 부착만 쓰는 송신 경계이다: 4바이트 직렬화(0 이면 원문 진단 transmitSize!=0 이 난다)와 무동작 송신이다.
            transmit: { writeData: () => 4, transmit: () => {} },
        });
        this.state = new RawPathProcessState(this.clock);
        this.factory = new SquidFactory(pool, this.types);
        // 부모 가상 효과의 응답/동작 입력이다: 도착 반환·Halt 반환, 삭제 예정/삭제됨, 마지막 이미지, 요청 순서.
        this.returnCode = 1002;
        this.haltCode = 10;
        this.willDelete = false;
        this.deleted = false;
        this.finalImage = null;
        this.events = [];
        this.afterArrival = null;
        this.afterHalt = null;
        // 겹침 정리 훅이 마지막으로 받은 [sid, x 비트, y 비트, 타입, 소유자]이다(없으면 null).
        this.clearArgs = null;
        this.haltDiagnostic = null;
        // 실제 letterRun 입력은 방향마다 두 프레임이다.
        const codes = [];
        for (let direction = 0; direction < 8; direction += 1) {
            codes.push({ side: 0x41 + direction, variant: 0x50, number: 1, flags: 0 }, { side: 0x41 + direction, variant: 0x50, number: 2, flags: 0 });
        }
        const type = priest ? 158 : 134;
        this.frames[type] = new FrameCodeTable(codes);
        // 보행용 +10/+14 와 수집용 +8/+c 를 다른 값으로 줘 필드 혼동을 검출한다.
        this.shapes[type] = { frames: Array.from({ length: 16 }, () => ({ cellWidth: 1, cellHeight: 1, walkStepX: Math.fround(0.3), walkStepY: Math.fround(0.2), cellHotspotX: 2, cellHotspotY: 3 })) };
        this.parent = this.factory.create(type, 2);
        assert.equal(this.pop.pop(this.parent, this.types[type], 1, 1, 10, 20, 0), RAW_POP_RESULT.registered);
        // 원본 입력의 Form 7 을 맞춘다. 중간 번호 6 은 할당된 void 입력이며 경로/부모에 부착하지 않는다.
        assert.equal(this.parent, 5);
        assert.equal(pool.allocate(2), 6);
        const next = new RawPathProcess(pool, this.hash, this.types, this.frames, this.shapes, this.spots, this.state, this.hooks(), { priest, flying });
        next.runtime.setPath([{ x: 10, y: 20 }, { x: 15, y: 20 }, { x: 17, y: 22 }], [0, 0, 2], [0, 1, 3]);
        this.process = RawPathProcess.attach(this.host, next, this.parent, 0x10);
        assert.ok(this.process);
        assert.equal(this.process.form, 7);
        this.pid = this.kernel.find(this.process);
        this.events = [];
    }

    // 요청 순서를 fixture 와 같은 문자열로 만든다.
    event(value) { this.events.push(value); }

    // 부모 가상 효과: Pop/Unpop 은 실제 등록을 바꾸고 Halt/도착/소리/정리는 입력 스크립트로 응답하며 순서를 기록한다.
    hooks() {
        const types = this.types;
        const pool = this.pool;
        return {
            unpop: (sid, flags) => {
                if (!this.willDelete) this.event(`unpop,${flags}`);
                this.unpop.unpop(sid, types[pool.slot(sid)[10]], flags);
            },
            pop: (sid, point, flags) => {
                if (!this.willDelete) this.event(`pop,${floatToBits(point.x)},${floatToBits(point.y)},${flags}`);
                assert.equal(this.pop.pop(sid, types[pool.slot(sid)[10]], 1, 1, point.x, point.y, flags), RAW_POP_RESULT.registered);
            },
            repop: (sid, flags) => {
                if (!this.willDelete) this.event(`repop,${flags}`);
                const raw = pool.slot(sid);
                assert.equal(this.pop.pop(sid, types[raw[10]], 1, 1, bitsToFloat(get(raw, 14)), bitsToFloat(get(raw, 18)), flags), RAW_POP_RESULT.registered);
            },
            halt: (parent) => {
                this.event(`halt,${this.haltCode}`);
                if (this.haltCode === 11) this.willDelete = true;
                if (this.afterHalt) this.afterHalt(parent);
                return this.haltCode;
            },
            assertion: (expression, file, line) => {
                if (!this.haltDiagnostic) throw new Error('경로 검사 진단 장치 미연결');
                this.haltDiagnostic(expression, file, line);
            },
            arrival: (parent, first, second) => {
                this.event(`arrival,${first},${second},${this.returnCode}`);
                if (this.returnCode !== 1002) this.willDelete = true;
                if (this.returnCode === 1001) this.host.kill(this.process, 0);
                if (this.afterArrival) this.afterArrival(parent);
                return this.returnCode;
            },
            soundId: () => 321,
            stepSound: (sid, x, y, sound) => { this.event(`sound,${floatToBits(x)},${floatToBits(y)},${sound}`); },
            observationStamp: () => 0x12345678,
            clearBlocked: (sid, point, type, owner) => {
                this.clearArgs = [sid, floatToBits(point.x), floatToBits(point.y), type, owner];
                this.event('clearBlocked');
            },
            cleanup: (runtime, sid, flags) => {
                this.deleted = true;
                this.finalImage = Uint8Array.from(runtime.bytes);
                if (this.returnCode !== 1001 || this.haltCode === 11) this.event(`kill,${flags}`);
            },
        };
    }

    // 원본 current/angle 를 입력에서 공급하는 공개 API 를 쓴다.
    advance(current, angle) {
        this.process.runtime.restoreStep(current, angle);
    }

    // 살아 있는 객체 또는 삭제 직전의 raw 이미지를 같은 필드 폭으로 비교한다: 부모 x/y/frame, 경로 current/angle/finished, 삭제 여부.
    fields() {
        const bytes = this.deleted ? this.finalImage : this.process.runtime.bytes;
        const raw = this.pool.slot(this.parent);
        return [get(raw, 14), get(raw, 18), get(raw, 36), get(bytes, PATH_LAYOUT.current), get(bytes, PATH_LAYOUT.angle), get(bytes, PATH_LAYOUT.finished), this.deleted ? 1 : 0].join(',');
    }

    // 외부 가상 요청과 실제 공간 요청을 한 순서로 비교한다.
    eventText() {
        return this.events.length === 0 ? '-' : this.events.join(';');
    }

    // 같은 부모에 종류가 다른 경로 프로세스를 하나 더 붙이고(기본 경로 3꼭짓점) 이 장면의 대상 프로세스로 삼는다. 요청 기록은 비운다. (2026-11-13 WB04-E349)
    // extra 는 부모 가상 효과 묶음에 덮어쓰는 훅(search·retarget 같은 선택 훅)이다.
    attachKind(kinds, flags = 0x10, extra = {}) {
        const next = new RawPathProcess(this.pool, this.hash, this.types, this.frames, this.shapes, this.spots, this.state, { ...this.hooks(), ...extra }, kinds);
        next.runtime.setPath([{ x: 10, y: 20 }, { x: 15, y: 20 }, { x: 17, y: 22 }], [0, 0, 2], [0, 1, 3]);
        const process = RawPathProcess.attach(this.host, next, this.parent, flags);
        assert.ok(process);
        this.process = process;
        this.events = [];
        return process;
    }

    // 살아 있는 Kernel 포인터만 제거하며 부모/효과의 수명보다 앞서 정리한다.
    dispose() { this.kernel.clear(); }
}

// 장면을 만들고 실행한 뒤 반드시 정리한다. (2026-11-13 WB04-E349)
function withScene(priest, flying, work, server = false) {
    const scene = new PathScene(priest, flying, server);
    try { work(scene); } finally { scene.dispose(); }
}

test('RawPathProcess: 원본 10.78 frame 60행 — 텔레포트/사제 RunFrame 뒤 부모 필드·경로 이미지·요청 순서가 같다', { skip: !runtimeRows && '원본 fixture 없음' }, () => {
    let count = 0;
    // 각 입력마다 같은 실제 공간/Form/Kernel 을 새로 준비한다.
    for (const row of runtimeRows.filter((candidate) => candidate[0] === 'frame' && candidate[1] === 'originals')) {
        withScene(Number(row[2]) !== 0, false, (scene) => {
            scene.advance(Number(row[3]), bitsToFloat(Number(row[5])));
            scene.clock.now = doubleOf(row[4]);
            scene.kernel.runFrame();
            assert.equal(scene.fields(), row[6], `행 ${count} 필드`);
            assert.equal(scene.eventText(), row[7], `행 ${count} 요청`);
        });
        count += 1;
    }
    assert.equal(count, 60);
});

test('RawPathProcess: 원본 10.78 arrival 12행 — 사제 여부 × 반환 1000/1001/1002 × Halt 10/11, 1001 자기 삭제 뒤 this 를 읽지 않고 1002 는 Kernel 에 남는다', { skip: !runtimeRows && '원본 fixture 없음' }, () => {
    let count = 0;
    // 사제 여부/세 반환값/두 Halt 를 전부 대조한다.
    for (const row of runtimeRows.filter((candidate) => candidate[0] === 'arrival' && candidate[1] === 'originals')) {
        withScene(Number(row[2]) !== 0, false, (scene) => {
            scene.returnCode = Number(row[3]);
            scene.haltCode = Number(row[4]);
            scene.advance(3, 0);
            scene.clock.now = 3;
            scene.process.arrive();
            assert.equal(scene.fields(), row[5], `행 ${count} 필드`);
            assert.equal(scene.eventText(), row[6], `행 ${count} 요청`);
            assert.equal(scene.kernel.size !== 0, scene.returnCode === 1002 && scene.haltCode === 10);
        });
        count += 1;
    }
    assert.equal(count, 12);
});

test('RawPathProcess: 원본 비행 39 frame 63행 — 20ms 예약은 Halt 뒤에 검사하고 같은 Step/도착 몸체를 호출하며 다음 프레임 시각 QWORD 가 같다', { skip: !flightRows && '원본 fixture 없음' }, () => {
    let count = 0;
    // 현재 단계/시각/예약 시각의 입력을 같은 Kernel/Form 위에서 대조한다.
    for (const row of flightRows.filter((candidate) => candidate[0] === 'frame')) {
        withScene(false, true, (scene) => {
            scene.advance(Number(row[1]), 0);
            scene.clock.now = doubleOf(row[2]);
            scene.process.runtime.setNextFrame(doubleOf(row[3]));
            scene.process.runFrame();
            assert.equal(scene.fields(), row[4], `행 ${count} 필드`);
            assert.equal(scene.eventText(), row[5], `행 ${count} 요청`);
            const view = new DataView(new ArrayBuffer(8));
            view.setFloat64(0, scene.process.runtime.nextFrame, true);
            assert.equal(view.getUint32(0, true), Number(row[6]), `행 ${count} 시각 하위`);
            assert.equal(view.getUint32(4, true), Number(row[7]), `행 ${count} 시각 상위`);
        });
        count += 1;
    }
    assert.equal(count, 63);
});

test('RawPathProcess: 실제 보호 사제/골렘 SHP 의 모든 논리 프레임 1,170행이 원본 RunFrame 결과·요청과 같다', { skip: !walkRows && '원본 fixture 없음' }, () => {
    const assets = dudeAssets();
    let count = 0;
    // 각 행은 별도 원본 초기 상태이며 이전 프레임 결과를 입력으로 대체하지 않는다.
    for (const row of walkRows) {
        withScene(true, false, (scene) => {
            const kind = Number(row[0]);
            scene.pool.allocated(scene.parent)[10] = kind;
            scene.types[kind] = { ...scene.types[158] };
            scene.frames[kind] = assets.frames[kind];
            scene.shapes[kind] = assets.shapes[kind];
            put(scene.pool.allocated(scene.parent), 36, Number(row[1]));
            scene.clock.now = doubleOf(row[2]);
            scene.advance(1, 0);
            scene.process.runFrame();
            assert.equal(scene.fields(), row[3], `행 ${count} 필드`);
            assert.equal(scene.eventText(), row[4], `행 ${count} 요청`);
        });
        count += 1;
    }
    assert.equal(count, 1170);
});

test('RawPathProcess: 실제 경로 탐색을 연결하지 않으면 Init 은 명시적으로 멈추고, 얼어붙은(0x5105) 사제는 프레임을 건너뛴다', () => {
    withScene(true, false, (scene) => {
        // Init 은 실제 표면 번호가 있을 때만 검색 경계를 요구한다. 이 검사는 해당 WORD 입력을 명시 공급한다.
        const surface = scene.factory.create(83, 2);
        scene.hash.setCell(0, 10, 20, surface);
        assert.throws(() => scene.process.initializePath(scene.parent, { x: 15, y: 20 }, { x: 10, y: 20 }), /실제 Path BFS 미복원 경계/);
        assert.equal(scene.kernel.size, 1);
        assert.equal(scene.process.runtime.current, 0);
        scene.types[158].flags2 |= 0x10000;
        assert.ok(scene.host.addRegular(scene.parent, 0x5105, 100));
        scene.clock.now = 0.5;
        scene.kernel.runFrame();
        assert.equal(scene.process.runtime.current, 0);
        assert.equal(scene.eventText(), '-');
    });
});

test('RawPathProcess: 시작 표면이 0 이면 Init 은 Pop/검색/current 쓰기를 모두 건너뛰고 검색 훅 없이도 반환한다', () => {
    withScene(true, false, (scene) => {
        scene.process.runtime.restoreStep(2, 0);
        // 표면 칸이 비어 있으면(0) 검색 훅 미연결도 도달하지 않는다.
        scene.process.initializePath(scene.parent, { x: 15, y: 20 }, { x: 10, y: 20 });
        assert.equal(scene.process.runtime.current, 2);
        assert.equal(scene.eventText(), '-');
    });
});

test('RawPathProcess: 자기 삭제 뒤 진단 복귀 — Halt checking 0/0x80000000/0xffffffff 입력에서 삭제된 경로를 다시 읽지 않는다', () => {
    // 현재 진단 0/상위 비트/전체 비트를 각각 같은 실제 Host 에 연결한다.
    for (const initial of [0, 0x80000000, 0xffffffff]) {
        withScene(false, false, (scene) => {
            const checking = { value: initial };
            scene.state.bindChecking(() => checking.value);
            scene.haltCode = 0;
            let diagnostics = 0;
            scene.haltDiagnostic = (expression, file, line) => {
                assert.ok(expression === 'result == haltALREADY_DEAD' && file === PATH_FILE && line === 0x2d5);
                assert.ok(!scene.kernel.get(scene.pid) && scene.deleted);
                diagnostics += 1;
                checking.value = 0;
            };
            scene.afterHalt = (parent) => {
                assert.equal(parent, scene.parent);
                scene.host.kill(scene.process, 0);
            };
            scene.process.runFrame();
            assert.ok(!scene.kernel.get(scene.pid) && scene.deleted);
            assert.equal(diagnostics, initial ? 1 : 0);
            assert.equal(checking.value, 0);
        });
    }
});

test('RawPathProcess: 도착의 현재 진단·겹침 정리 — 1000 정상/1001·삭제/999 비정상 반환 4변형이 같은 Host 삭제 뒤 clearBlocked 를 한 번 하고 진단 줄이 같다', () => {
    // 정상/비정상 반환과 살아남은/자기 삭제 경로를 구별한다.
    for (const variant of [0, 1, 2, 3]) {
        withScene(false, false, (scene) => {
            const checking = { value: variant ? 0x80000000 : 0 };
            scene.state.bindChecking(() => checking.value);
            scene.returnCode = variant < 2 ? 1000 : 999;
            const diagnostics = [];
            // 도착 칸에 현재 겹침 부류(0x200)가 겹치는 후보를 spot 8 로 둔다.
            const candidate = scene.pool.allocate(2);
            const raw = scene.pool.allocated(candidate);
            raw[10] = 1;
            raw[11] = 0;
            put(raw, 4, 0, 2);
            put(raw, 14, floatToBits(10));
            put(raw, 18, floatToBits(20));
            scene.types[1].footX = scene.types[1].footY = 1;
            scene.types[1].flags2 = 0x200;
            scene.state.blockingGenus = 0x200;
            scene.hash.setCell(0, 10, 20, candidate);
            scene.spots[20 * 256 + 10] |= 8;
            scene.haltDiagnostic = (expression, file, line) => {
                assert.equal(file, PATH_FILE);
                diagnostics.push(line);
                if (line === 0x28d) {
                    assert.equal(expression, 'returnCode == pathKILL || returnCode == pathALREADY_DEAD || returnCode == pathLINGER');
                } else {
                    assert.ok(line === 0x290 && expression === 'returnCode == pathALREADY_DEAD');
                    assert.ok(!scene.kernel.get(scene.pid) && scene.deleted);
                }
                checking.value = 0;
            };
            if (variant < 3) {
                scene.afterArrival = (parent) => {
                    assert.equal(parent, scene.parent);
                    scene.host.kill(scene.process, 0);
                };
            }
            scene.process.arrive();
            assert.ok(!scene.kernel.get(scene.pid) && scene.deleted);
            assert.equal(checking.value, 0);
            assert.equal(scene.events.filter((entry) => entry === 'clearBlocked').length, 1);
            assert.deepEqual(diagnostics, variant === 0 ? [] : variant === 1 ? [0x290] : [0x28d]);
        });
    }
});

test('RawPathProcess: 예약 Form 0 도 항상 참 robust 를 지나 가상 도착하며 부모 free/dead 진단의 정상 복귀를 보존한다', () => {
    // 처음 부모 상태/가상 Halt 뒤 상태를 각각 현재 진단으로 복원한다.
    for (const variant of [0, 1, 2]) {
        withScene(false, false, (scene) => {
            const checking = { value: 0xffffffff };
            scene.state.bindChecking(() => checking.value);
            let arrivals = 0;
            let diagnostics = 0;
            put(scene.process.runtime.bytes, 4, 0);
            // free 입력 진단도 같은 물리 슬롯에서 복구한다.
            const parentRaw = scene.pool.physical(scene.parent);
            if (variant === 1) parentRaw[11] |= 3;
            if (variant === 2) scene.afterHalt = () => { parentRaw[11] |= 3; };
            scene.haltDiagnostic = (expression, file, line) => {
                assert.ok(file === PATH_FILE && line === (variant === 1 ? 0x25c : 0x286));
                diagnostics += 1;
                parentRaw[11] &= ~3;
            };
            scene.afterArrival = (parent) => {
                assert.equal(parent, scene.parent);
                arrivals += 1;
            };
            scene.process.arrive();
            assert.ok(scene.kernel.get(scene.pid) === scene.process && !scene.deleted);
            assert.equal(arrivals, 1);
            assert.equal(diagnostics, variant ? 1 : 0);
            assert.ok(!scene.process.runtime.finished && !scene.process.runtime.deathWatchToken);
        });
    }
});

test('RawPathProcess: 생성자 연결 검사 — 필수 훅 누락·표 길이 불일치·spot 크기·풀/상태 형식은 부착 전에 거부한다', () => {
    const scene = new PathScene(true);
    try {
        const make = (overrides = {}, spots = scene.spots, shapes = scene.shapes, hooks = scene.hooks(), state = scene.state, pool = scene.pool) =>
            new RawPathProcess(pool, scene.hash, scene.types, scene.frames, shapes, spots, state, hooks, overrides);
        assert.ok(make());
        // 필수 훅 아홉 개는 하나라도 빠지면 거부한다.
        for (const name of ['unpop', 'repop', 'pop', 'halt', 'arrival', 'soundId', 'stepSound', 'clearBlocked', 'cleanup']) {
            const hooks = scene.hooks();
            delete hooks[name];
            assert.throws(() => make({}, scene.spots, scene.shapes, hooks), /경로 월드 연결 누락/, name);
        }
        // 선택 훅(search·retarget·assertion·balloonFlame·bulfRandom·bulfSound·observationStamp)은 없어도 만들 수 있다.
        const optional = scene.hooks();
        delete optional.assertion;
        delete optional.observationStamp;
        assert.ok(make({}, scene.spots, scene.shapes, optional));
        assert.throws(() => make({}, new Uint8Array(65535)), /경로 월드 연결 누락/);
        assert.throws(() => make({}, scene.spots, scene.shapes.slice(1)), /경로 월드 연결 누락/);
        assert.throws(() => make({}, scene.spots, scene.shapes, null), /경로 월드 연결 누락/);
        assert.throws(() => make({}, scene.spots, scene.shapes, scene.hooks(), {}), TypeError);
        assert.throws(() => make({}, scene.spots, scene.shapes, scene.hooks(), scene.state, { edition: 'cd1072' }), TypeError);
        // 종류 플래그는 서로 배타적이다(경로 이미지 생성자가 거부).
        assert.throws(() => make({ priest: true, flying: true }));
        assert.throws(() => make({ flying: true, balloon: true }));
    } finally {
        scene.dispose();
    }
});

test('RawPathProcess: 종류별 프로세스 번호 — 23/54/39/40/42/24/41/38 이며 상태의 현재 번호를 읽는다', () => {
    const scene = new PathScene(true);
    try {
        const make = (kinds) => new RawPathProcess(scene.pool, scene.hash, scene.types, scene.frames, scene.shapes, scene.spots, scene.state, scene.hooks(), kinds);
        assert.equal(make({}).movementType(), 23);
        assert.equal(make({ priest: true }).movementType(), 54);
        assert.equal(make({ flying: true }).movementType(), 39);
        assert.equal(make({ worm: true }).movementType(), 40);
        assert.equal(make({ balloon: true }).movementType(), 42);
        assert.equal(make({ dude: true }).movementType(), 24);
        assert.equal(make({ bulf: true }).movementType(), 41);
        assert.equal(make({ walker: true }).movementType(), 38);
        // 번호는 mutable 전역이라 현재 값을 매번 읽는다.
        scene.state.teleportProcess = 91;
        scene.state.priestProcess = 92;
        scene.state.flyProcess = 93;
        scene.state.wormProcess = 94;
        scene.state.balloonProcess = 95;
        scene.state.dudeProcess = 96;
        scene.state.bulfProcess = 97;
        scene.state.walkerProcess = 98;
        const kinds = [{}, { priest: true }, { flying: true }, { worm: true }, { balloon: true }, { dude: true }, { bulf: true }, { walker: true }];
        assert.deepEqual(kinds.map((kind) => make(kind).movementType()), [91, 92, 93, 94, 95, 96, 97, 98]);
    } finally {
        scene.dispose();
    }
});

test('RawPathProcess: 부착 검사 — 풀 불일치와 사제/Dude/웜 부모 타입을 거부하고 비행 39 는 부착 전에 같은 종류 기존 프로세스를 제거한다', () => {
    const scene = new PathScene(true);
    try {
        const make = (kinds) => new RawPathProcess(scene.pool, scene.hash, scene.types, scene.frames, scene.shapes, scene.spots, scene.state, scene.hooks(), kinds);
        // 다른 풀의 호스트와는 부착하지 않는다.
        const other = new PathScene(false);
        try {
            assert.throws(() => RawPathProcess.attach(other.host, make({}), scene.parent, 0x10), /경로 부착 풀 불일치/);
            assert.throws(() => RawPathProcess.attach(scene.host, {}, scene.parent, 0x10), /경로 부착 풀 불일치/);
        } finally {
            other.dispose();
        }
        // 현재 부모(타입 158)가 아닌 타입 전역이면 사제는 거부하고, Dude 는 70 이어야 하며, 웜은 133/187 이어야 한다.
        scene.state.priestType = 77;
        assert.throws(() => RawPathProcess.attach(scene.host, make({ priest: true }), scene.parent, 0x10), /사제 경로 부모 타입/);
        scene.state.priestType = 158;
        assert.throws(() => RawPathProcess.attach(scene.host, make({ dude: true }), scene.parent, 0x10), /Dude24 경로 부모 타입/);
        assert.throws(() => RawPathProcess.attach(scene.host, make({ worm: true }), scene.parent, 0x10), /웜40/);
        scene.state.sunWalkerType = 158;
        assert.ok(RawPathProcess.attach(scene.host, make({ worm: true }), scene.parent, 0x10));
        scene.state.sunWalkerType = 133;
        // 비행: 두 번째 비행 프로세스를 붙이면 첫 번째는 제거된다(같은 종류만).
        const first = RawPathProcess.attach(scene.host, make({ flying: true }), scene.parent, 0x10);
        const firstPid = scene.kernel.find(first);
        assert.ok(first && firstPid);
        const second = RawPathProcess.attach(scene.host, make({ flying: true }), scene.parent, 0x10);
        assert.ok(second && scene.kernel.get(firstPid) !== first);
        assert.ok(scene.kernel.find(second));
        // 기존 사제 경로(54)는 같은 종류가 아니므로 남는다.
        assert.ok(scene.kernel.find(scene.process));
    } finally {
        scene.dispose();
    }
});

test('RawPathProcess: Prepare — nonzero override 가 있으면 그 속도를, 0 이면 타입 속도를 쓰고 0 속도는 거부한다', () => {
    withScene(true, false, (scene) => {
        const expected = (speed) => {
            const runtime = new RawPathRuntime({ priest: true });
            runtime.setPath([{ x: 10, y: 20 }, { x: 15, y: 20 }, { x: 17, y: 22 }], [0, 0, 2], [0, 1, 3]);
            runtime.prepare(7.5, 7.5, speed, 90, runtime.direction(0));
            return Array.from({ length: 3 }, (_, index) => runtime.time(index));
        };
        scene.clock.now = 7.5;
        scene.process.prepare(0, 2, 90);
        assert.deepEqual(Array.from({ length: 3 }, (_, index) => scene.process.runtime.time(index)), expected(2));
        scene.process.prepare(5, 2, 90);
        assert.deepEqual(Array.from({ length: 3 }, (_, index) => scene.process.runtime.time(index)), expected(5));
        assert.throws(() => scene.process.prepare(5, 0, 90), /경로 타입 속도0/);
        assert.throws(() => scene.process.prepare(5, 2, 0), /경로 타입 속도0/);
    });
});

test('RawPathProcess: 비행 39/벌룬 42 의 Init 은 직선 비행 Init 만 하고(벌룬은 순항 44·현재 고도 BYTE) 검색 훅을 요구하지 않는다', () => {
    const scene = new PathScene(false);
    try {
        const make = (kinds) => new RawPathProcess(scene.pool, scene.hash, scene.types, scene.frames, scene.shapes, scene.spots, scene.state, scene.hooks(), kinds);
        const flight = make({ flying: true });
        flight.initializePath(scene.parent, { x: 20, y: 30 }, { x: 0, y: 0 });
        const expectedFlight = new RawPathRuntime({ flying: true });
        expectedFlight.initializeFlight({ x: 10, y: 20 }, { x: 20, y: 30 }, 0, 0);
        assert.deepEqual(Buffer.from(flight.runtime.bytes), Buffer.from(expectedFlight.bytes));
        // 벌룬: 부모 +30 의 고도 BYTE 를 현재 고도로 넘긴다.
        scene.pool.allocated(scene.parent)[30] = 12;
        const balloon = make({ balloon: true });
        balloon.initializePath(scene.parent, { x: 20, y: 30 }, { x: 0, y: 0 });
        const expectedBalloon = new RawPathRuntime({ balloon: true });
        expectedBalloon.initializeFlight({ x: 10, y: 20 }, { x: 20, y: 30 }, 44, 12);
        assert.deepEqual(Buffer.from(balloon.runtime.bytes), Buffer.from(expectedBalloon.bytes));
    } finally {
        scene.dispose();
    }
});

// 같은 부모에 flags 0 으로 사제 경로를 하나 더 붙인다. Form 이 서버 번호(15000 이상)라 권한이 없으면 이동 허가가 없다. (2026-11-13 WB04-E349)
function attachServerForm(scene) {
    const next = new RawPathProcess(scene.pool, scene.hash, scene.types, scene.frames, scene.shapes, scene.spots, scene.state, scene.hooks(), { priest: true });
    next.runtime.setPath([{ x: 10, y: 20 }, { x: 15, y: 20 }, { x: 17, y: 22 }], [0, 0, 2], [0, 1, 3]);
    const process = RawPathProcess.attach(scene.host, next, scene.parent, 0);
    assert.ok(process && process.form >= scene.pool.layout.serverFirst);
    assert.deepEqual(scene.hostDiagnostics, []);
    return process;
}

test('RawPathProcess: 길 밖 정지 — 올림/내림 칸 중 하나라도 걸을 수 없고 권한이 있으며 주변 3×3 에 gDAIS 가 없으면 자신을 삭제하고, 그 밖에는 계속한다', () => {
    // 꼭짓점 (15.5, 20)의 올림 칸은 (16, 20), 내림 칸은 (15, 20), 주변은 (15..17, 19..21) 이다(편향 0.9999).
    // [이름, 올림 spot, 내림 spot, 주변 준비, 권한, 정지 기대]
    const around = (scene, x, y) => {
        scene.types[134].flags2 = 0x400000;
        const dais = scene.factory.create(134, 2);
        assert.equal(scene.pop.pop(dais, scene.types[134], 1, 1, x, y, 0), RAW_POP_RESULT.registered);
    };
    const surface = (scene) => {
        const sid = scene.factory.create(83, 2);
        scene.types[83].flags1 = 0x800;
        scene.types[83].flags2 = 0x400000;
        scene.types[83].footX = scene.types[83].footY = 1;
        // finder 의 발자국 교차 검사를 통과하도록 올림 칸 (16, 20) 의 현재 위치를 준다.
        const raw = scene.pool.allocated(sid);
        put(raw, 14, floatToBits(16));
        put(raw, 18, floatToBits(20));
        scene.hash.setCell(0, 16, 20, sid);
    };
    const cases = [
        ['올림 칸만 막힘', 0, 6, null, 'local-boss', true],
        ['내림 칸만 막힘', 6, 0, null, 'local-boss', true],
        ['두 칸 모두 막힘', 0, 0, null, 'local-boss', true],
        ['두 칸 모두 걸을 수 있음', 6, 6, null, 'local-boss', false],
        ['주변 모서리(우하)에 gDAIS', 0, 0, (scene) => around(scene, 17, 21), 'local-boss', false],
        ['주변 모서리(좌상)에 gDAIS', 0, 0, (scene) => around(scene, 15, 19), 'local-boss', false],
        ['표면 단계의 gDAIS 는 건너뜀', 0, 0, surface, 'local-boss', true],
        ['권한 없음 + 로컬 Form 번호', 0, 0, null, 'local-guest', true],
        ['권한 있음 + 서버 번호 Form', 0, 0, null, 'server-boss', true],
        ['권한 없음 + 서버 번호 Form', 0, 0, null, 'server-guest', false],
    ];
    for (const [name, ceilSpot, floorSpot, prepare, mode, stops] of cases) {
        const server = mode.startsWith('server');
        withScene(true, false, (scene) => {
            scene.spots[20 * 256 + 16] = ceilSpot;
            scene.spots[20 * 256 + 15] = floorSpot;
            if (prepare) prepare(scene);
            const process = server ? attachServerForm(scene) : scene.process;
            scene.clock.boss = mode.endsWith('boss');
            process.runtime.setPath([{ x: 10, y: 20 }, { x: 15.5, y: 20 }, { x: 17, y: 22 }], [0, 0, 2], [0, 1, 3]);
            process.runtime.restoreStep(1, 0);
            scene.clock.now = 1;
            scene.events = [];
            process.runFrame();
            assert.equal(scene.kernel.find(process) === 0, stops, name);
            assert.equal(scene.deleted, stops, name);
            // 정지하지 않으면 같은 프레임에서 SHP 프레임 선택이 이어져 Unpop/Pop 이 일어난다.
            if (!stops) assert.ok(scene.events.some((entry) => entry.startsWith('unpop')) && scene.events.some((entry) => entry.startsWith('pop')), name);
            if (stops) assert.ok(scene.events.includes('kill,0'), name);
        }, server);
    }
});

test('RawPathProcess: 종료 통지 — 권한이 있고 부모가 Dead 가 아니면 Unpop→Repop 뒤 cleanup 훅에 경로 이미지·부모·flags 를 넘긴다', () => {
    // 변형: 0 일반(공간 접두 있음), 1 부모 Dead(접두 생략), 2 권한 없음+서버 번호 Form(접두 생략).
    for (const variant of [0, 1, 2]) {
        withScene(true, false, (scene) => {
            const seen = [];
            const hooks = scene.hooks();
            const cleanup = hooks.cleanup;
            const next = new RawPathProcess(scene.pool, scene.hash, scene.types, scene.frames, scene.shapes, scene.spots, scene.state, {
                ...hooks,
                cleanup: (runtime, sid, flags) => { seen.push([runtime === next.runtime, sid, flags]); cleanup(runtime, sid, flags); },
            }, { priest: true });
            next.runtime.setPath([{ x: 10, y: 20 }], [0]);
            const process = RawPathProcess.attach(scene.host, next, scene.parent, variant === 2 ? 0 : 0x10);
            assert.ok(process);
            // 부착 뒤에 부모 상태/권한을 바꾼다: 1 부모 Dead, 2 권한 없음 + 서버 번호 Form.
            if (variant === 1) scene.pool.physical(scene.parent)[11] |= 2;
            if (variant === 2) scene.clock.boss = false;
            scene.events = [];
            // 프로세스를 직접 지운다(0x10 은 client 삭제 허가 비트, 0x40 은 종속 삭제 비트).
            scene.host.kill(process, variant === 2 ? 0x10 : 0x40);
            assert.equal(seen.length, 1);
            assert.deepEqual(seen[0].slice(0, 2), [true, scene.parent]);
            const prefix = scene.events.filter((entry) => entry.startsWith('unpop') || entry.startsWith('repop'));
            assert.deepEqual(prefix, variant === 0 ? ['unpop,0', 'repop,0'] : [], `변형 ${variant}`);
        }, variant === 2);
    }
});

test('RawPathProcess: Kernel 이 지우면 감시 상자가 꺼져 destroy 가 deadFlag 를 켠다(원본 소멸자)', () => {
    withScene(true, false, (scene) => {
        const process = scene.process;
        assert.ok(scene.kernel.find(process));
        scene.host.kill(process, 0);
        assert.equal(scene.kernel.find(process), 0);
        assert.equal(scene.deleted, true);
        // 미부착(또는 삭제 뒤) 프로세스의 RunFrame 은 부착 호스트가 없다.
        const loose = new RawPathProcess(scene.pool, scene.hash, scene.types, scene.frames, scene.shapes, scene.spots, scene.state, scene.hooks(), {});
        assert.throws(() => loose.runFrame(), /미부착 경로 RunFrame/);
    });
});

test('RawPathProcessState: checking 은 독립 값과 빌린 getter 를 구별하고 매번 현재 값을 읽으며, 겹침 마스크는 shared getter 가 우선한다', () => {
    const state = new RawPathProcessState(new SquidProcessState());
    assert.equal(state.checking, 0);
    state.checking = 0xffffffff;
    assert.equal(state.checking, 0xffffffff);
    let live = 7;
    state.bindChecking(() => live);
    assert.equal(state.checking, 7);
    live = -1; // 부호 있는 DWORD 도 전체 비트로 읽는다.
    assert.equal(state.checking, 0xffffffff);
    state.checking = 3; // 독립 값을 쓰면 빌린 getter 는 해제된다.
    live = 99;
    assert.equal(state.checking, 3);
    state.bindChecking(() => live);
    state.bindChecking(null);
    assert.equal(state.checking, 3);
    assert.throws(() => state.bindChecking(5), TypeError);
    assert.throws(() => new RawPathProcessState(null), TypeError);
    // 겹침 마스크: 기본값 0x10444200, shared getter 가 있으면 그 현재 값이다.
    assert.equal(state.currentBlockingGenus(), 0x10444200);
    let shared = 0x200;
    state.sharedBlockingGenus = () => shared;
    assert.equal(state.currentBlockingGenus(), 0x200);
    shared = 0xffffffff;
    assert.equal(state.currentBlockingGenus(), 0xffffffff);
    // 기본 전역 번호와 윈드 counter 상자이다.
    assert.deepEqual([state.teleportProcess, state.priestProcess, state.priestType, state.flyProcess, state.wormProcess, state.sunWalkerType, state.emptyType, state.balloonProcess, state.balloonType,
        state.dudeProcess, state.dudeType, state.walkerProcess, state.windWalkerType, state.rainWalkerType, state.bulfProcess], [23, 54, 158, 39, 40, 133, 187, 42, 111, 24, 70, 38, 137, 156, 41]);
    assert.deepEqual(state.groundTypes, [158, 137, 156, 135, 187]);
    assert.deepEqual(state.windWalkerCounter, { value: 0 });
    assert.deepEqual(state.observation, { flags: 0, stamp: 0 });
});

test('RawPathProcess: 가상 Process 형식 — SquidProcess 를 확장하고 부착 뒤 경로 이미지에 Form/부모 번호를 기록한다', () => {
    withScene(true, false, (scene) => {
        assert.ok(scene.process instanceof SquidProcess);
        assert.equal(scene.process.pool, scene.pool);
        assert.equal(get(scene.process.runtime.bytes, 4), scene.process.form);
        assert.equal(get(scene.process.runtime.bytes, 8), scene.parent);
        assert.ok(throws(() => new RawPathRuntime({ priest: true, flying: true })));
    });
});

test('RawPathProcess: 사제·Dude·웜은 같은 SHP 보행 프레임 선택과 같은 도착 정지 프레임 접두를 쓰고 텔레포트는 쓰지 않는다', () => {
    const frameEvents = [];
    const arrivalEvents = [];
    // 세 종류가 같은 부모(사제 158)에서 같은 결과를 내는지 본다. Dude/웜은 부착 검사를 통과하도록 현재 전역 타입을 맞춘다.
    for (const kind of ['priest', 'dude', 'worm', 'teleport']) {
        withScene(true, false, (scene) => {
            scene.state.dudeType = 158;
            scene.state.sunWalkerType = 158;
            const process = scene.attachKind(kind === 'teleport' ? {} : { [kind]: true });
            process.runtime.restoreStep(1, 0);
            scene.clock.now = 1;
            process.runFrame();
            frameEvents.push(`${scene.eventText()}|${scene.fields()}`);
            scene.events = [];
            process.runtime.restoreStep(3, 0);
            scene.clock.now = 3;
            process.arrive();
            arrivalEvents.push(scene.events.filter((entry) => entry.startsWith('repop')).join(';'));
        });
    }
    assert.equal(frameEvents[1], frameEvents[0]);
    assert.equal(frameEvents[2], frameEvents[0]);
    assert.notEqual(frameEvents[3], frameEvents[0]);
    assert.ok(frameEvents[0].includes('unpop,0') && frameEvents[0].includes(',64'));
    assert.deepEqual(arrivalEvents, ['repop,64', 'repop,64', 'repop,64', '']);
});

test('RawPathProcess: 프레임 몸체 연결 — 얼어붙음(0x5105)은 사제/웜/Dude/Walker/Bulf 만 막고 텔레포트/비행/벌룬은 막지 않는다', () => {
    // 사건이 걸린 부모 genus(0x10000)에서 프레임이 Halt 까지 가는지(요청 halt 존재) 종류별로 본다.
    const cases = [['priest', { priest: true }, true], ['worm', { worm: true }, true], ['dude', { dude: true }, true], ['walker', { walker: true }, true],
        ['bulf', { bulf: true }, true], ['teleport', {}, false], ['flying', { flying: true }, false]];
    for (const [name, kinds, blocks] of cases) {
        withScene(true, false, (scene) => {
            scene.state.dudeType = 158;
            scene.state.sunWalkerType = 158;
            scene.types[158].flags2 |= 0x10000;
            assert.ok(scene.host.addRegular(scene.parent, 0x5105, 100));
            const process = scene.attachKind(kinds);
            process.runtime.restoreStep(1, 0);
            if (kinds.flying) process.runtime.setNextFrame(0);
            scene.clock.now = 1;
            process.runFrame();
            assert.equal(scene.events.some((entry) => entry.startsWith('halt')), !blocks, name);
        });
    }
});

test('RawPathProcess: Walker/Bulf 도 Halt 정지 결과(11)에서 자신을 삭제하고, Walker 윈드 counter 는 세션 공유 상자를 갱신한다', () => {
    for (const kinds of [{ walker: true }, { bulf: true }]) {
        withScene(true, false, (scene) => {
            scene.haltCode = 11;
            const process = scene.attachKind(kinds);
            process.runtime.restoreStep(1, 0);
            scene.clock.now = 1;
            process.runFrame();
            assert.ok(scene.events.includes('halt,11') && scene.events.includes('kill,0'));
            assert.equal(scene.kernel.find(process), 0);
        });
    }
    // 윈드 보행 TYPE(137) 부모가 직각(90도) 꼭짓점을 지나면 공유 counter 가 (c+1)&3 으로 한 번 움직인다.
    withScene(true, false, (scene) => {
        scene.pool.allocated(scene.parent)[10] = 137;
        scene.types[137] = { ...scene.types[158] };
        const process = scene.attachKind({ walker: true });
        process.runtime.setPath([{ x: 10, y: 20 }, { x: 10, y: 25 }, { x: 10, y: 28 }], [0, 2, 2], [0, 1, 3]);
        process.runtime.restoreStep(1, 0);
        scene.state.windWalkerCounter.value = 3;
        scene.clock.now = 1;
        process.runFrame();
        assert.equal(scene.state.windWalkerCounter.value, 0);
    });
});

test('RawPathProcess: 벌룬 프레임은 화염 소리 훅이 없으면 거부하고 있으면 현재 부모 좌표로 부른다', () => {
    // 이륙 첫 꼭짓점에서 고도 0 인 벌룬 TYPE 부모는 화염 소리를 낸다(고도 BYTE 0, 현재 단계 1).
    for (const connected of [false, true]) {
        withScene(true, false, (scene) => {
            scene.pool.allocated(scene.parent)[10] = scene.state.balloonType;
            scene.types[scene.state.balloonType] = { ...scene.types[158] };
            const flames = [];
            const hooks = scene.hooks();
            if (connected) hooks.balloonFlame = (x, y) => { flames.push([floatToBits(x), floatToBits(y)]); };
            const next = new RawPathProcess(scene.pool, scene.hash, scene.types, scene.frames, scene.shapes, scene.spots, scene.state, hooks, { balloon: true });
            next.initializePath(scene.parent, { x: 20, y: 30 }, { x: 10, y: 20 });
            const process = RawPathProcess.attach(scene.host, next, scene.parent, 0x10);
            assert.ok(process);
            process.prepare(0, 1, 90);
            scene.clock.now = process.runtime.time(0) + 0.0001;
            if (connected) {
                process.runFrame();
                assert.equal(flames.length, 1);
                assert.deepEqual(flames[0], [floatToBits(10), floatToBits(20)]);
            } else {
                assert.throws(() => process.runFrame(), /벌룬 화염 음향 미연결/);
            }
        });
    }
});

test('RawPathProcess: 길 밖 정지 허용 타입 목록은 상태의 다섯 칸이며 목록 밖 타입은 정지하지 않고 SHP 프레임 범위를 확인한다', () => {
    // 변형: 0 마지막(다섯째) 허용 타입 187 → 정지(삭제), 1 목록 밖 타입 133(SHP 표 비어 있음) → 정지하지 않고 SHP 범위 오류.
    for (const variant of [0, 1]) {
        withScene(true, false, (scene) => {
            scene.spots[20 * 256 + 15] = 0;
            const type = variant === 0 ? 187 : 133;
            scene.pool.allocated(scene.parent)[10] = type;
            scene.types[type] = { ...scene.types[158] };
            scene.process.runtime.restoreStep(1, 0);
            scene.clock.now = 1;
            if (variant === 0) {
                scene.process.runFrame();
                assert.equal(scene.kernel.find(scene.process), 0);
                assert.ok(scene.events.includes('kill,0'));
            } else {
                assert.throws(() => scene.process.runFrame(), /경로 SHP 프레임/);
                assert.ok(scene.kernel.find(scene.process) !== 0);
            }
        });
    }
});

test('RawPathProcess: 길 밖 칸 좌표는 0.9999 편향으로 절삭하고 지도 밖(비유한/음수/256 이상)은 호스트 오류로 거부한다', () => {
    // [x, 오류 여부]: 255 는 편향 뒤 255.9999 로 지도 안, 255.01 은 256 이상, -0.5 는 편향 뒤 0.4999 로 안, -1 은 편향 뒤 음수로 밖이다.
    for (const [x, rejected] of [[255, false], [255.01, true], [-0.5, false], [-1, true]]) {
        withScene(true, false, (scene) => {
            scene.process.runtime.setPath([{ x: 10, y: 20 }, { x, y: 20 }], [0, 0], [0, 1]);
            scene.process.runtime.restoreStep(1, 0);
            scene.clock.now = 1;
            if (rejected) assert.throws(() => scene.process.runFrame(), /경로 지도 좌표/, String(x));
            else scene.process.runFrame();
        });
    }
});

test('RawPathProcess: 부모가 Free 이면 이동 프레임 선택을 건너뛰고(Unpop/Pop 없음), 아니면 같은 입력에서 프레임을 바꾼다', () => {
    // 변형 0: 정상 부모, 1: 부모 Free 비트가 켜진 입력.
    for (const free of [false, true]) {
        withScene(true, false, (scene) => {
            if (free) scene.pool.physical(scene.parent)[11] |= 1;
            scene.process.runtime.restoreStep(1, 0);
            scene.clock.now = 1;
            scene.process.runFrame();
            assert.equal(scene.events.some((entry) => entry.startsWith('unpop') || entry.startsWith('pop')), !free);
        });
    }
});

test('RawPathProcess: 벌룬·Walker·비행 부착도 같은 종류 기존 프로세스를 제거하고, Prepare 의 음수 override 도 nonzero 로 쓴다', () => {
    for (const kinds of [{ balloon: true }, { walker: true }, { flying: true }]) {
        withScene(true, false, (scene) => {
            const first = scene.attachKind(kinds);
            const firstPid = scene.kernel.find(first);
            assert.ok(firstPid);
            const second = scene.attachKind(kinds);
            assert.ok(second && scene.kernel.get(firstPid) !== first && scene.kernel.find(second));
        });
    }
    // 같은 종류가 아닌 프로세스(텔레포트끼리)는 부착 때 서로를 제거하지 않는다.
    withScene(true, false, (scene) => {
        const first = scene.attachKind({});
        const second = scene.attachKind({});
        assert.ok(scene.kernel.find(first) !== 0 && scene.kernel.find(second) !== 0);
        // 음수 override: 0 과만 구별하며 부호는 그대로 쓴다.
        scene.clock.now = 2;
        second.prepare(-3, 2, 90);
        const expected = new RawPathRuntime();
        expected.setPath([{ x: 10, y: 20 }, { x: 15, y: 20 }, { x: 17, y: 22 }], [0, 0, 2], [0, 1, 3]);
        expected.prepare(2, 2, -3, 90, expected.direction(0));
        assert.deepEqual(Array.from({ length: 3 }, (_, index) => second.runtime.time(index)), Array.from({ length: 3 }, (_, index) => expected.time(index)));
    });
});

test('RawPathProcess: 도착 훅과 겹침 정리 훅은 경로 이미지의 두 목표와 부모의 현재 좌표·TYPE·소유자를 그대로 받는다', () => {
    withScene(false, false, (scene) => {
        scene.process.runtime.setTargets(60, 61);
        scene.returnCode = 1002;
        scene.advance(3, 0);
        scene.clock.now = 3;
        scene.process.arrive();
        assert.ok(scene.events.includes('arrival,60,61,1002'));
    });
    // 도착 칸이 막혀 있으면(spot 8 + 같은 부류 후보) 정리 훅이 현재 부모 좌표·TYPE·소유자를 받는다. 겹침 마스크를 공유 getter 로 빌리면 그 현재 값(0)이 우선해 막히지 않는다.
    for (const shared of [false, true]) {
        withScene(false, false, (scene) => {
            const candidate = scene.pool.allocate(2);
            const raw = scene.pool.allocated(candidate);
            raw[10] = 1;
            put(raw, 4, 0, 2);
            put(raw, 14, floatToBits(10));
            put(raw, 18, floatToBits(20));
            scene.types[1].footX = scene.types[1].footY = 1;
            scene.types[1].flags2 = 0x200;
            scene.state.blockingGenus = 0x200;
            if (shared) scene.state.sharedBlockingGenus = () => 0;
            scene.hash.setCell(0, 10, 20, candidate);
            scene.spots[20 * 256 + 10] |= 8;
            scene.pool.allocated(scene.parent)[34] = 3;
            scene.process.arrive();
            const parent = scene.pool.slot(scene.parent);
            if (shared) assert.equal(scene.clearArgs, null);
            else assert.deepEqual(scene.clearArgs, [scene.parent, get(parent, 14), get(parent, 18), parent[10], 3]);
        });
    }
});

test('RawPathProcess: Halt 연결 — 부모 강제 정지 비트는 계속(10)을 정지로 바꾸고, 운반 부모는 권한과 목표가 있을 때만 재탐색 훅을 부른다', () => {
    // 강제 정지: 부모 +40 의 0x20 비트가 켜지면 Halt 반환 10 이 11(종료)로 바뀐다.
    withScene(false, false, (scene) => {
        scene.pool.allocated(scene.parent)[40] |= 0x20;
        scene.advance(1, 0);
        scene.clock.now = 1;
        scene.process.runFrame();
        assert.ok(scene.events.includes('halt,10') && scene.events.includes('kill,0'));
        assert.equal(scene.kernel.find(scene.process), 0);
    });
    // 운반 부모(genus 0x30000)의 재탐색: [권한, 목표, 훅 연결] → 호출 여부 또는 오류.
    const cases = [[true, true, true, 1], [false, true, true, 0], [true, false, true, 0], [true, true, false, -1]];
    for (const [boss, targets, connected, calls] of cases) {
        withScene(false, false, (scene) => {
            scene.types[134].flags2 = 0x30000;
            scene.clock.boss = boss;
            const seen = [];
            const process = scene.attachKind({}, 0x10, connected ? { retarget: (runtime, sid) => { seen.push([runtime === process.runtime, sid]); } } : {});
            if (targets) process.runtime.setTargets(60, 0);
            process.runtime.restoreStep(1, 0);
            scene.clock.now = 1;
            if (calls < 0) assert.throws(() => process.runFrame(), /실제 운반 재탐색 몸체 미복원 경계/);
            else process.runFrame();
            if (calls >= 0) assert.deepEqual(seen, calls ? [[true, scene.parent]] : []);
        });
    }
});

test('RawPathProcess: Init 의 flags — Bulf/Walker 는 항상 1(직선 축약)을, 그 밖은 받은 flags 를 검색 훅에 넘기고 void 부모면 시작점으로 Pop(0)을 먼저 한다', () => {
    // 종류별로 시작 표면을 두고 검색 훅이 받은 인자를 기록한다.
    for (const [kinds, flags, expected] of [[{}, 0, 0], [{}, 4, 4], [{ priest: true }, 2, 2], [{ bulf: true }, 0, 1], [{ walker: true }, 6, 1]]) {
        withScene(true, false, (scene) => {
            const surface = scene.factory.create(83, 2);
            scene.hash.setCell(0, 10, 20, surface);
            const seen = [];
            const process = scene.attachKind(kinds, 0x10, { search: (runtime, sid, start, destination, given) => { seen.push([runtime === process.runtime, sid, start.x, start.y, destination.x, destination.y, given]); } });
            process.runtime.restoreStep(2, 0);
            // 부모가 void(실제 Unpop 뒤)이면 검색 전에 시작점으로 Pop(0)을 한다. 그 Pop 뒤에는 void 가 아니라 다시 부르면 하지 않는다.
            scene.unpop.unpop(scene.parent, scene.types[158], 0);
            assert.notEqual(scene.pool.slot(scene.parent)[11] & 4, 0);
            scene.events = [];
            process.initializePath(scene.parent, { x: 15, y: 20 }, { x: 10, y: 20 }, flags);
            assert.deepEqual(seen, [[true, scene.parent, 10, 20, 15, 20, expected]]);
            assert.deepEqual(scene.events, [`pop,${floatToBits(10)},${floatToBits(20)},0`]);
            assert.equal(process.runtime.current, 0);
            assert.equal(scene.pool.slot(scene.parent)[11] & 4, 0);
            scene.events = [];
            process.runtime.restoreStep(2, 0);
            process.initializePath(scene.parent, { x: 15, y: 20 }, { x: 10, y: 20 }, flags);
            assert.deepEqual(scene.events, []);
        });
    }
});

test('RawPathProcess: 텔레포트/비행은 한 축만 바뀌어도 Unpop(0)→Pop(0x40)을 한다(x 만 / y 만 / 둘 다 같음)', () => {
    // [도착 좌표, Unpop/Pop 기대]
    for (const [point, moves] of [[{ x: 15, y: 20 }, true], [{ x: 10, y: 25 }, true], [{ x: 10, y: 20 }, false]]) {
        withScene(false, false, (scene) => {
            scene.process.runtime.setPath([{ x: 10, y: 20 }, point, { x: 20, y: 30 }], [0, 0, 2], [0, 1, 3]);
            scene.process.runtime.restoreStep(1, 0);
            scene.clock.now = 1;
            scene.events = [];
            scene.process.runFrame();
            assert.equal(scene.events.some((entry) => entry.startsWith('pop,')), moves, JSON.stringify(point));
            assert.equal(scene.events.some((entry) => entry === 'unpop,0'), moves, JSON.stringify(point));
        });
    }
});

test('RawPathProcess: SHP 발자국 거리가 이동 거리와 같으면(엄격 비교) 프레임을 바꾸지 않고, 더 짧으면 다음 프레임으로 넘어간다', () => {
    // 도착 꼭짓점을 현재 좌표에 발자국(+0.3,+0.2)을 더한 float 값으로 두면 두 거리 제곱이 같다.
    const foot = { x: Math.fround(10 + Math.fround(0.3)), y: Math.fround(20 + Math.fround(0.2)) };
    for (const [point, changes] of [[foot, false], [{ x: foot.x + 0.01, y: foot.y }, true]]) {
        withScene(true, false, (scene) => {
            scene.process.runtime.setPath([{ x: 10, y: 20 }, point, { x: 20, y: 30 }], [0, 0, 2], [0, 1, 3]);
            scene.process.runtime.restoreStep(1, 0);
            scene.clock.now = 1;
            scene.events = [];
            scene.process.runFrame();
            assert.equal(scene.events.some((entry) => entry.startsWith('unpop')), changes, JSON.stringify(point));
        });
    }
});

test('RawPathProcess: 감시 토큰은 부모 Halt 동안 켜졌다가 끝나면 꺼지고, 훅은 만든 시점의 사본을 쓴다', () => {
    withScene(false, false, (scene) => {
        let seen = -1;
        let process = null;
        const hooks = {
            ...scene.hooks(),
            halt: () => { seen = process.runtime.deathWatchToken; return 10; },
        };
        const next = new RawPathProcess(scene.pool, scene.hash, scene.types, scene.frames, scene.shapes, scene.spots, scene.state, hooks, {});
        next.runtime.setPath([{ x: 10, y: 20 }, { x: 15, y: 20 }, { x: 17, y: 22 }], [0, 0, 2], [0, 1, 3]);
        process = RawPathProcess.attach(scene.host, next, scene.parent, 0x10);
        assert.ok(process);
        // 만든 뒤 원본 훅 객체를 바꿔도 프로세스는 만든 시점의 효과를 쓴다.
        hooks.halt = () => { throw new Error('훅 사본이 아님'); };
        scene.process = process;
        process.runtime.restoreStep(1, 0);
        scene.clock.now = 1;
        process.runFrame();
        assert.equal(seen, 1);
        assert.equal(process.runtime.deathWatchToken, 0);
    });
});

test('RawPathProcess: 관찰 시계 훅은 벌룬 Step 에는 부르지 않고 그 밖의 Step 마다 한 번 부른다', () => {
    // 벌룬은 관찰 시계 인자 없이 Step 하고, 텔레포트/사제는 Step 마다 훅을 한 번 읽는다.
    for (const [kinds, calls] of [[{ balloon: true }, 0], [{}, 1], [{ priest: true }, 1], [{ walker: true }, 1], [{ bulf: true }, 1]]) {
        withScene(true, false, (scene) => {
            let stamps = 0;
            const process = scene.attachKind(kinds, 0x10, { observationStamp: () => { stamps += 1; return 0; } });
            if (kinds.balloon) {
                scene.pool.allocated(scene.parent)[10] = scene.state.balloonType;
                scene.types[scene.state.balloonType] = { ...scene.types[158] };
                process.initializePath(scene.parent, { x: 20, y: 30 }, { x: 10, y: 20 });
                process.prepare(0, 1, 90);
                scene.clock.now = process.runtime.time(0) + 0.0001;
            } else {
                process.runtime.restoreStep(1, 0);
                scene.clock.now = 1;
            }
            try { process.runFrame(); } catch (error) { if (!/벌룬 화염 음향 미연결|부모 진단 553|공용 난수.발소리 미연결/.test(error.message)) throw error; }
            assert.equal(stamps, calls, JSON.stringify(kinds));
        });
    }
});

test('RawPathProcess: 원본 10.78 pathprepare 24행 — Prepare 의 nonzero override 선택과 타입 속도 선택이 꼭짓점 시각 QWORD 와 같다', { skip: !prepareRows && '원본 fixture 없음' }, () => {
    let count = 0;
    // 실제 가상 +0x30 의 결과를 원본 53비트(x87) 시각 배열과 대조한다. 호출되지 않는 효과는 명시 예외 경계다.
    for (const row of prepareRows.filter((candidate) => candidate[0] === 'originals')) {
        assert.equal(row.length, 6);
        const pool = new SidPool(32768, { server: true });
        const types = pathTypes();
        const clock = new SquidProcessState();
        clock.now = doubleOf(row[4]);
        const forbid = (message) => () => { throw new Error(message); };
        const process = new RawPathProcess(pool, new SquidHash(), types, types.map(() => new FrameCodeTable([])), types.map(() => ({ frames: [] })), new Uint8Array(65536), new RawPathProcessState(clock), {
            unpop: forbid('Prepare 검사에서 공간 변경 미실행'), repop: forbid('Prepare 검사에서 공간 변경 미실행'), pop: forbid('Prepare 검사에서 Pop 미실행'),
            halt: forbid('Prepare 검사에서 Halt 미실행'), arrival: forbid('Prepare 검사에서 도착 미실행'), soundId: forbid('Prepare 검사에서 효과음 조회 미실행'),
            stepSound: forbid('Prepare 검사에서 효과음 미실행'), clearBlocked: forbid('Prepare 검사에서 겹침 정리 미실행'), cleanup: forbid('Prepare 검사에서 수명 정리 미실행'),
        }, {});
        const points = [{ x: 10, y: 20 }, { x: 13.1, y: 24.7 }, { x: 15.2, y: 19.8 }, { x: 23.9, y: 27.6 }].map((point) => ({ x: Math.fround(point.x), y: Math.fround(point.y) }));
        process.runtime.setPath(points, [7, 0, 4, 6]);
        process.prepare(bitsToFloat(Number(row[1])), bitsToFloat(Number(row[2])), bitsToFloat(Number(row[3])));
        const expected = row[5].split(',');
        assert.equal(expected.length, 4);
        // 실제 하위 Path 시각 함수가 저장한 각 QWORD 를 비트 단위로 비교한다.
        for (let index = 0; index < 4; index += 1) {
            const view = new DataView(new ArrayBuffer(8));
            view.setFloat64(0, process.runtime.time(index), true);
            assert.equal(view.getBigUint64(0, true), BigInt(expected[index]), `행 ${count} 시각 ${index}`);
        }
        count += 1;
    }
    assert.equal(count, 24);
});
