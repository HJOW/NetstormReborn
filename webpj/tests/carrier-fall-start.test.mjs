// Carrier task raw-carrier-task.js(RawCarrierTask)와 낙하 시작 raw-carrier-fall-start.js(RawCarrierFallStart)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: carrier-fall-start-x86.tsv(10.78 전용; 기본 CRT 53비트 열) 2850행 — 모드(0 task 설정 / 1 낙하 시작)·task·지연 비트·현재 시각(double 두 DWORD)·기존 프로세스 유무·event/count/payload·권한·extra·낙하 중 여부·포획 Form·원래 소유자·좌표·사제 종류·이동 종류·변이 단계·확보 성공 →
//    종료 상태(ok / null=확보 실패), 하위 호출 순서(Q 조회 / A 확보 / C 생성 / ! 진단 / H·M·D·S·K·R 시작), 프로세스 event/count/payload/기한/현재 시각/권한/사제 종류/Player 기록 Adler-32, 부모와 Form 의 raw. NULL 실패 480행 포함.
//  - 계약: CarrierFallStart_LazyBoundaries(이미 낙하 중이면 다른 경계 없이 반환, 미연결 task 하위 거부, 물리 Player 범위는 쓰기 전에 검증). 웜40 경로 객체(RawPathRuntime)는 해당 파일 이식 때 대조한다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E72: Carrier task·낙하 시작 이식과 함께 추가)
import './nan-exact.mjs'; // NaN 비트 대조는 최적화 JIT 를 끈 프로세스에서만 Node v22/v24/v26 에서 같다.
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get, adler32, floatToBits, bitsToFloat } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { RegularProcess } from '../www/core/squid-process.js';
import { RawCarrierTask, CarrierTaskAllocationError } from '../www/core/raw-carrier-task.js';
import { RawCarrierFallStart } from '../www/core/raw-carrier-fall-start.js';

// 원본 관측 행이다(단일 판본 파일). 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E72)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'carrier-fall-start-x86.tsv'));

// 독립 입력 수와 그 중 확보 실패(NULL) 행 수, 부모/포획 Form SID 이다. (2026-11-08 WB04-E72)
const ROWS = 2850;
const NULL_ROWS = 480;
const PARENT = 50;
const FORM = 51;

// 예외가 나는지 본다. (2026-11-08 WB04-E72)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 바이트 배열을 소문자 hex 로 만든다. (2026-11-08 WB04-E72)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// double 의 하위/상위 DWORD 와 그 역변환이다(NaN payload 보존). (2026-11-08 WB04-E72)
const dview = new DataView(new ArrayBuffer(8));
const doubleWords = (value) => { dview.setFloat64(0, value, true); return [dview.getUint32(0, true), dview.getUint32(4, true)]; };
const wordsDouble = (low, high) => { dview.setUint32(0, low, true); dview.setUint32(4, high, true); return dview.getFloat64(0, true); };

// 원문과 같은 큰 외부 효과만 공급하며 내부 task 는 실제 RawCarrierTask 를 실행하는 장면이다. (2026-11-08 WB04-E72)
class CarrierStartScene {
    constructor() {
        this.pool = new SidPool(24000, { server: true });
        this.players = new Uint8Array(128 * 0xb4);
        this.row = [];
        this.now = 0;
        this.boss = false;
        this.priest = 0;
        this.process = null;
        this.trace = [];
        // 원본은 같은 주소에서 객체를 다시 만들므로, 호출자가 쥔 포인터는 최신 객체를 가리킨다. 최신 객체에 위임하는 안정 뷰이다.
        this.view = {
            get event() { return scene.process.event; },
            resetTask: (event, time) => scene.process.resetTask(event, time),
            setNextTime: (time) => scene.process.setNextTime(time)
        };
        const scene = this;
    }

    // 원본은 할당 필터 없이 현재 물리 슬롯을 읽는다.
    raw(sid) {
        return this.pool.physical(sid);
    }

    // 생성자 경계의 필드 입력만 기존 같은 주소에 설치한다(주소 대신 새 객체로 교체).
    seed(event, count, payload, time) {
        this.process = new RegularProcess(event, payload);
        this.process.resetTask(event, time);
        this.process.setInitialCount(count);
    }

    // 원문의 전체 DWORD 인자를 효과 발생 순서로 기록한다.
    event(name, values) {
        this.trace.push([name, ...values].join(':'));
    }

    // 입력 자료만 설치하고 예상 좌표/기한/분기는 계산하지 않는다.
    prepare(input) {
        assert.equal(input.length, 20);
        this.row = input;
        this.now = wordsDouble(input[3], input[4]);
        this.boss = input[9] !== 0;
        this.priest = input[16];
        this.trace = [];
        this.seed(input[6], input[7], bitsToFloat(input[8]), -100.5);
        // 부모와 포획 Form 의 실제 원문 상태다.
        for (const sid of [PARENT, FORM]) {
            const raw = this.raw(sid);
            raw.fill(0xa5);
            raw[10] = 135;
            raw[40] = input[10];
            put(raw, 14, input[14]);
            put(raw, 18, input[15]);
        }
        put(this.raw(FORM), 12, input[13], 2);
        // 원문 고정 Player 패턴이며 쓰는 owner/위치의 기대 색인을 주입하지 않는다.
        for (let index = 0; index < this.players.length; index += 1) this.players[index] = (index * 17 + 29) & 0xff;
    }

    // 기존 객체 검색/할당/두 생성자/진단을 각각 독립 효과로 제공한다.
    taskHooks() {
        return {
            find: (sid) => { this.event('Q', [sid, 0xfff0, 0x5100]); if (this.row[18] === 5) this.now = 42.5; return this.row[5] ? this.view : null; },
            allocate: () => { this.event('A', [40]); return this.row[19] !== 0; },
            construct: (sid, event, shared) => {
                this.event('C', [shared ? 61 : 46, event, sid, floatToBits(5.0)]);
                this.seed(event, 0, 5.0, 0);
                if (this.row[18] === 6) { this.seed(0x515a, 99, 2.5, 0); this.now = 123.25; }
                return this.view;
            },
            assertion: () => {
                this.event('!', [0x407]);
                if (this.row[18] === 7) { const time = this.process.time; this.seed(0x6161, 77, 0.5, time); this.now = 64.25; }
            }
        };
    }

    // 시작의 큰 효과만 경계로 두고 마지막 예약은 실제 task 모듈을 사용한다.
    startHooks(task) {
        return {
            hasRegular: (sid, event) => { this.event('H', [sid, event]); return this.row[11] !== 0; },
            movementType: (type) => { this.event('M', [type, 0, 1]); return this.row[17]; },
            removeMovement: (sid, type) => {
                this.event('D', [sid, type, 0, 1]);
                if (this.row[18] === 1) {
                    this.boss = false;
                    this.raw(PARENT)[40] = 4;
                    this.priest = 159;
                    put(this.raw(PARENT), 14, floatToBits(42.125));
                    put(this.raw(PARENT), 18, floatToBits(43.25));
                }
            },
            clearSelection: () => {
                this.event('S', [0, 0, 1]);
                this.raw(PARENT)[40] &= ~4 & 0xff;
                if (this.row[18] === 2) {
                    this.boss = true;
                    this.priest = 159;
                    put(this.raw(PARENT), 14, floatToBits(43.5));
                    put(this.raw(PARENT), 18, floatToBits(44.5));
                    put(this.raw(FORM), 12, 0x2342, 2);
                }
            },
            findKind: (sid, kind) => {
                this.event('K', [sid, kind, 0, 0]);
                if (this.row[18] === 3) {
                    this.boss = true;
                    this.priest = 160;
                    put(this.raw(PARENT), 14, floatToBits(0.00001));
                    put(this.raw(PARENT), 18, floatToBits(254.99999));
                    put(this.raw(FORM), 12, 0x3443, 2);
                }
                return this.row[12];
            },
            releasePriest: (sid) => {
                this.event('R', [sid]);
                if (this.row[18] === 4) {
                    this.now = 88.875;
                    put(this.raw(FORM), 12, 0, 2);
                    put(this.raw(PARENT), 14, floatToBits(55.0));
                    put(this.raw(PARENT), 18, floatToBits(66.0));
                }
            },
            task: (sid, event, delay) => task.set(sid, event, delay)
        };
    }

    // 원문의 반환값과 현재 필드를 정해진 순서로 기록한다.
    state(result) {
        const time = doubleWords(this.process.time);
        const clock = doubleWords(this.now);
        return [result, this.process.event, this.process.count, floatToBits(this.process.payload), time[0], time[1], clock[0], clock[1], this.boss ? 1 : 0, this.priest, adler32(this.players)].join(',');
    }
}

test('CarrierFallStart: 원본 10.78 2850행의 task 설정/낙하 시작 효과 순서·프로세스 필드·현재 시각·Player 기록·부모/Form raw 가 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    const scene = new CarrierStartScene();
    let count = 0;
    let nulls = 0;
    // 기본 CRT 의 53비트 원문 전체를 실행하고 NULL 실패도 정상 성공과 별도로 재생한다.
    for (const fields of rows) {
        assert.equal(fields.length, 9);
        scene.prepare(fields[0].split(',').map(Number));
        const task = new RawCarrierTask(scene.pool, () => scene.now, scene.taskHooks());
        const begin = new RawCarrierFallStart(scene.pool, scene.players, () => scene.boss, () => scene.priest, scene.startHooks(task));
        let result = 0;
        let status = 'ok';
        try {
            if (scene.row[0] === 0) task.set(PARENT, scene.row[1], bitsToFloat(scene.row[2]));
            else result = begin.begin(PARENT) ? 1 : 0;
        } catch (error) {
            if (!(error instanceof CarrierTaskAllocationError)) throw error;
            status = 'null';
            nulls += 1;
        }
        const raws = `${hex(scene.raw(PARENT))},${hex(scene.raw(FORM))}`;
        assert.equal(status, fields[1], `행 ${count}: 입력 ${fields[0]}`);
        assert.equal(scene.trace.length === 0 ? '-' : scene.trace.join(';'), fields[2], `행 ${count}: 입력 ${fields[0]}`);
        assert.equal(scene.state(result), fields[3], `행 ${count}: 입력 ${fields[0]}`);
        assert.equal(raws, fields[4], `행 ${count}: 입력 ${fields[0]}`);
        count += 1;
    }
    assert.ok(count === ROWS && nulls === NULL_ROWS);
});

test('CarrierFallStart: 이미 낙하 중인 객체는 다른 경계 없이 반환하며 미연결 하위와 물리 Player 범위는 쓰기 전에 거부한다', { skip: !rows && '원본 fixture 없음' }, () => {
    const scene = new CarrierStartScene();
    scene.prepare([1, 0x5105, 0, 0, 0, 1, 0x5101, 7, 0, 1, 0, 1, 51, 0x1281, 0, 0, 158, 35, 0, 1]);
    const begin = new RawCarrierFallStart(scene.pool, scene.players, () => scene.boss, () => scene.priest, { hasRegular: () => true });
    assert.ok(begin.begin(PARENT));
    assert.equal(begin.pool, scene.pool);
    const missing = new RawCarrierTask(scene.pool, () => scene.now, {});
    assert.ok(throws(() => missing.set(PARENT, 0x5105, 0)));
    scene.row[11] = 0;
    scene.boss = false;
    put(scene.raw(FORM), 12, 0x7f, 2);
    const before = adler32(scene.players);
    const task = new RawCarrierTask(scene.pool, () => scene.now, scene.taskHooks());
    const limited = new RawCarrierFallStart(scene.pool, scene.players.subarray(0, 9 * 0xb4), () => scene.boss, () => scene.priest, scene.startHooks(task));
    assert.ok(throws(() => limited.begin(PARENT)));
    assert.equal(adler32(scene.players), before);
    assert.ok(throws(() => new RawCarrierFallStart(scene.pool, new Uint8Array(0xb4 + 1), () => true, () => 158, {})));
    assert.ok(throws(() => new RawCarrierTask({ edition: 'cd1072' }, () => 0, {})));
    assert.equal(get(scene.raw(FORM), 12, 2), 0x7f);
});
