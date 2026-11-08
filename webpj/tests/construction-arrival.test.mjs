// 건설 도착 raw-construction-arrival.js(RawConstructionArrival·ConstructionArrivalState·makeConstructionArrivalHooks)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: constructionarrival-x86.tsv 의 `originals` 153행 — 도착 객체 좌표·checking·streak 타입·지속 시간·후보 목록(SID·TYPE·상태·abstract·좌표·발자국·flags1·해시 단계)·
//    현재 자료 변형(Mutate 0~5: abstract 해제·타입 flags1 해제·next 변경·좌표 변경·지속 시간 변경)에서 반환 코드(1001/1002)와 하위 효과 순서(S 시작[SID·지속 시간 비트]/R contained 삭제/K 자기 삭제)가 같다.
//    `originalCD` 153행은 10.72 라 사용하지 않는다.
//  - 계약/통합: 필수 하위 효과·타입 표 크기·판본 거부, 삭제가 선택된 위치에서 공유 streak 타입을 현재 값으로 읽기, 타입 표 밖 후보 거부, start 어댑터(다른 풀 거부)와 실제 RawConstructionProcess 연결.
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E133: 건설 도착 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, bitsToFloat, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { ConstructionArrivalState, RawConstructionArrival, makeConstructionArrivalHooks } from '../www/core/raw-construction-arrival.js';
import { ConstructionProcessState, RawConstructionProcess } from '../www/core/raw-construction-process.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E133)
const allRows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'constructionarrival-x86.tsv')) ?? [];
const rows = allRows.filter((row) => row[0] === 'originals');

// 도착 운반 객체의 SID, 사용하는 SID 목록(예측 영역 끝 보호 번호 14999·서버 영역 첫 세 번호 포함), 서버 영역 시작, 10.78 abstract extra 위치, 판본 행 수, 풀 크기이다. (2026-11-08 WB04-E133)
const BUILDER = 50;
const USED = [50, 60, 14999, 15000, 15001, 15002];
const SERVER_FIRST = 15000;
const EXTRA = 40;
const ROWS = 153;
const CAPACITY = 24000;

// 예외가 나는지 본다. (2026-11-08 WB04-E133)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 합성 장면이다: 같은 풀의 슬롯·해시·타입 표·전역 위에서 하위 효과를 문자열 사건으로 기록한다. (2026-11-08 WB04-E133)
class Scene {
    constructor() {
        this.pool = new SidPool(CAPACITY, { server: true });
        this.hash = new SquidHash();
        this.types = Array.from({ length: 188 }, () => ({ flags1: 0, flags2: 0, footX: 0, footY: 0 }));
        this.state = new ConstructionArrivalState();
        this.events = [];
        this.mutation = 0;
        this.mutated = false;
    }

    // 사건을 기록한다. (2026-11-08 WB04-E133)
    event(text) { this.events.push(text); }

    // 입력 행(좌표·checking·streak 타입·지속 시간·변형 번호)과 후보 목록으로 현재 raw/해시/전역을 준비한다. 후보는 입력 순서대로 네 단계 해시 머리에 끼운다. (2026-11-08 WB04-E133)
    setup(input, description) {
        const parameters = input.split(',').map(Number);
        assert.equal(parameters.length, 6);
        this.events = [];
        this.mutated = false;
        this.hash.reset();
        this.state.checking = parameters[2] !== 0;
        this.state.streakProcessType = parameters[3];
        this.state.duration = bitsToFloat(parameters[4]);
        this.mutation = parameters[5];
        for (const type of this.types) Object.assign(type, { flags1: 0, flags2: 0, footX: 0, footY: 0 });
        // 남은 옛 체인/raw 필드가 다음 합성 장면에 영향을 주지 않게 쓰는 모든 슬롯을 지운다.
        for (const sid of USED) this.pool.physical(sid).fill(0);
        const builder = this.pool.physical(BUILDER);
        put(builder, 14, parameters[0]);
        put(builder, 18, parameters[1]);
        if (description === '-') return;
        // 입력 순서대로 후보 슬롯을 만들고 같은 해시 머리에 연결한다(머리 = 마지막 후보).
        for (const text of description.split(';')) {
            const fields = text.split(',').map(Number);
            assert.equal(fields.length, 10);
            const [sid, type, state, extra, xBits, yBits, footX, footY, flags1, level] = fields;
            const raw = this.pool.physical(sid);
            raw[10] = type;
            raw[11] = state;
            raw[EXTRA] = extra;
            put(raw, 14, xBits);
            put(raw, 18, yBits);
            Object.assign(this.types[type], { footX, footY, flags1 });
            const x = bitsToFloat(xBits);
            const y = bitsToFloat(yBits);
            put(raw, 4, this.hash.bucket(level, x, y), 2);
            this.hash.setBucket(level, x, y, sid);
        }
    }

    // 세 하위 효과만 기록하고, 첫 start 뒤 현재 자료 변형을 적용한다. (2026-11-08 WB04-E133)
    hooks() {
        return {
            start: (sid, duration) => {
                this.event(`S:${sid}:${floatToBits(duration)}`);
                if (this.mutated) return;
                this.mutated = true;
                const target = sid === SERVER_FIRST + 1 ? SERVER_FIRST + 2 : SERVER_FIRST + 1;
                const raw = this.pool.physical(target);
                if (this.mutation === 1) raw[EXTRA] = 0;
                else if (this.mutation === 2) this.types[target === SERVER_FIRST + 1 ? 100 : 101].flags1 = 0;
                else if (this.mutation === 3) put(this.pool.physical(sid), 4, 0, 2);
                else if (this.mutation === 4) put(raw, 14, floatToBits(200));
                else if (this.mutation === 5) this.state.duration = Math.fround(0.9);
            },
            removeContained: (sid, type) => this.event(`R:${sid}:${type}:0:1`),
            destroy: (sid, flags) => this.event(`K:${sid}:${flags}`),
        };
    }

    // 현재 자료로 도착 몸체를 만든다. (2026-11-08 WB04-E133)
    arrival() {
        return new RawConstructionArrival(this.pool, this.hash, this.types, this.state, this.hooks());
    }
}

test('도착: 원본 10.78 관측 153행의 반환 코드와 하위 효과 순서가 같다', { skip: rows.length === 0 }, () => {
    assert.equal(rows.length, ROWS);
    const scene = new Scene();
    const arrival = scene.arrival();
    // 정상/예측/무대상/변형 관찰을 같은 객체로 순서대로 재생한다.
    for (const row of rows) {
        assert.equal(row.length, 6);
        scene.setup(row[1], row[2]);
        assert.equal(arrival.arrive(BUILDER), Number(row[3]), row.join(' '));
        assert.equal(scene.events.length === 0 ? '-' : scene.events.join(';'), row[5], row.join(' '));
    }
});

test('도착: 누락된 하위 효과·잘못된 타입 표·다른 판본을 첫 효과 전에 거부한다', () => {
    const scene = new Scene();
    const hooks = scene.hooks();
    for (const name of ['start', 'removeContained', 'destroy']) {
        assert.ok(throws(() => new RawConstructionArrival(scene.pool, scene.hash, scene.types, scene.state, { ...hooks, [name]: undefined })), name);
    }
    assert.ok(throws(() => new RawConstructionArrival(scene.pool, scene.hash, scene.types.slice(0, 171), scene.state, hooks)));
    assert.ok(throws(() => new RawConstructionArrival(scene.pool, scene.hash, scene.types, null, hooks)));
    assert.ok(throws(() => new RawConstructionArrival({ edition: 'cd1072' }, scene.hash, scene.types, scene.state, hooks)));
    assert.ok(!throws(() => scene.arrival()));
    assert.equal(scene.arrival().pool, scene.pool);
});

test('도착: 예측 대상만 있을 때 삭제가 도달한 위치에서 공유 streak 타입을 현재 값으로 읽는다', () => {
    const scene = new Scene();
    const candidate = scene.pool.physical(60);
    candidate[10] = 100;
    candidate[11] = 0;
    candidate[EXTRA] = 1;
    put(candidate, 14, floatToBits(20));
    put(candidate, 18, floatToBits(21));
    put(scene.pool.physical(BUILDER), 14, floatToBits(20));
    put(scene.pool.physical(BUILDER), 18, floatToBits(21));
    Object.assign(scene.types[100], { footX: 1, footY: 1, flags1: 0x8000 });
    scene.hash.setBucket(1, 20, 21, 60);
    let current = 56;
    scene.state.streakProcessType = 12;
    scene.state.sharedStreakProcessType = () => current;
    const arrival = scene.arrival();
    assert.equal(arrival.arrive(BUILDER), 1002);
    assert.deepEqual(scene.events, ['R:50:56:0:1']);
    scene.events = [];
    current = 57;
    assert.equal(arrival.arrive(BUILDER), 1002);
    assert.deepEqual(scene.events, ['R:50:57:0:1']);
    // checking 이 켜져 있으면 삭제 없이 대기만 한다.
    scene.events = [];
    scene.state.checking = () => 0x100;
    assert.equal(arrival.arrive(BUILDER), 1002);
    assert.deepEqual(scene.events, []);
    // 빌린 전역이 없으면 독립 입력의 값을 쓴다.
    scene.state.checking = false;
    scene.state.sharedStreakProcessType = null;
    assert.equal(arrival.arrive(BUILDER), 1002);
    assert.deepEqual(scene.events, ['R:50:12:0:1']);
});

test('도착: abstract 후보의 타입이 타입 표 밖이면 효과 없이 거부한다', () => {
    const scene = new Scene();
    const candidate = scene.pool.physical(60);
    candidate[10] = 200;
    candidate[EXTRA] = 1;
    put(candidate, 14, floatToBits(20));
    put(candidate, 18, floatToBits(21));
    put(scene.pool.physical(BUILDER), 14, floatToBits(20));
    put(scene.pool.physical(BUILDER), 18, floatToBits(21));
    // 타입 표 밖 번호는 finder 의 발자국 조회 또는 도착 몸체의 타입 확인 중 한쪽에서 효과 전에 거부된다.
    assert.ok(throws(() => {
        scene.hash.setBucket(1, 20, 21, 60);
        scene.arrival().arrive(BUILDER);
    }));
    assert.deepEqual(scene.events, []);
});

test('도착: 좌표를 ftol → float 저장 → ftol 한 칸으로 만들어 큰 값·음수·비유한 값을 원본처럼 처리한다', () => {
    const scene = new Scene();
    const arrival = scene.arrival();
    const cases = [
        // [x 비트, y 비트, 기대 반환 코드] — 후보 없음이므로 모두 종료이다. 보드 밖 좌표도 finder 가 자르므로 예외 없이 끝난다.
        [floatToBits(-3.5), floatToBits(-0.25), 1001],
        [floatToBits(1e12), floatToBits(2e12), 1001],
        [0x7fc00000, 0xffc00000, 1001],
        [0x7f800000, 0xff800000, 1001],
        [floatToBits(16777217), floatToBits(255.99), 1001],
    ];
    // 좌표마다 도착 객체만 두고 후보가 없는 장면을 만든다.
    for (const [xBits, yBits, expected] of cases) {
        scene.setup(`${xBits},${yBits},0,12,${floatToBits(0.6)},0`, '-');
        assert.equal(arrival.arrive(BUILDER), expected);
        assert.deepEqual(scene.events, ['K:50:0']);
    }
});

test('도착: start 어댑터가 같은 풀의 실제 RawConstructionProcess.start 를 부르고 다른 풀은 거부한다', () => {
    const scene = new Scene();
    const calls = [];
    const fake = { pool: scene.pool, start: (sid, duration) => calls.push([sid, floatToBits(duration)]) };
    const other = new SidPool(CAPACITY, { server: true });
    assert.ok(throws(() => makeConstructionArrivalHooks(other, fake, scene.hooks())));
    assert.ok(throws(() => makeConstructionArrivalHooks(scene.pool, null, scene.hooks())));
    const base = scene.hooks();
    const adapted = makeConstructionArrivalHooks(scene.pool, fake, base);
    // 나머지 경계는 그대로 두고 start 만 바뀐다.
    assert.equal(adapted.removeContained, base.removeContained);
    assert.equal(adapted.destroy, base.destroy);
    assert.notEqual(adapted.start, base.start);
    adapted.start(15001, Math.fround(0.6));
    assert.deepEqual(calls, [[15001, floatToBits(0.6)]]);

    // 실제 건설 프로세스(host 없이 attach 경계만 기록)와 연결하면 도착 판정의 start 가 그 프로세스의 시작을 실행한다.
    const record = [];
    const processState = new ConstructionProcessState();
    processState.now = 10;
    const hooks = {
        reserve: () => true,
        effect: (parent, instant) => record.push(`E:${parent}:${instant ? 1 : 0}`),
        attach: (fields, type, parent, flags) => { record.push(`J:${parent}:${flags}`); return fields; },
        loadSound: () => 0, reserveSound: () => false, attachSound: () => {}, refresh: () => record.push('R'),
        maxHitPoints: () => 0, updateDisplay: () => {}, removeContained: () => {}, playAt: () => {}, readySound: () => 0, play: () => {},
        unpop: () => {}, repop: () => {}, push: () => {},
    };
    const process = new RawConstructionProcess(scene.pool, scene.types, processState, hooks);
    scene.setup(`${floatToBits(20)},${floatToBits(21)},0,12,${floatToBits(0.6)},0`, `15001,100,0,1,${floatToBits(20)},${floatToBits(21)},1,1,32768,1`);
    const real = makeConstructionArrivalHooks(scene.pool, process, scene.hooks());
    const arrival = new RawConstructionArrival(scene.pool, scene.hash, scene.types, scene.state, {
        ...real,
        // 시작을 실제 프로세스로 잇되 효과 순서를 같은 목록에 남긴다.
        destroy: (sid, flags) => record.push(`K:${sid}:${flags}`),
    });
    assert.equal(arrival.arrive(BUILDER), 1001);
    assert.deepEqual(record.slice(0, 1), ['E:15001:0']);
    assert.ok(record.includes('J:15001:16'));
    assert.deepEqual(record.slice(-1), ['K:50:0']);
});
