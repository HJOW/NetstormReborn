// 건설 사제 이동 raw-construction-builder.js(RawConstructionBuilder·ConstructionBuilderState·세 어댑터)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: constructionbuilder-x86.tsv 의 `originals` 370행 — 이동(Move)과 공급원 소비(Take)의 입력 21개(플레이어·기준점·소유자·접근 mask·genus·목표 좌표·발자국·요청 도착 시각·now·경로·속도·공급원 윤곽·할당 결과·프레임·변동·다리·개수)에서
//    사건 순서(A 기준점/C 생성/O 소유자/P 접근점/B Pop/M 경로/V 속도/Q 자원 선택/S 정렬/N 표면 알림/L 타이머 확보/T Regular)와 반환 double 비트·생성 raw 전체·최종 공급원 목록·개수가 같다.
//    `originalCD`·`original1037` 행은 다른 판본이라 사용하지 않는다.
//  - 계약: signed 개수 조기 반환과 손상 목록/메타 인덱스/경로 인덱스/풀 밖 기준점 거부, 기준점 실패는 생성 전 반환하고 첫 경로 실패는 앞선 효과를 유지하며 타이머 실패는 이동을 계속함, 어댑터의 풀 불일치 거부와 실제 모듈 연결.
// 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E142: 건설 사제 이동 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, bitsToFloat, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { typeConstructorAddress } from '../www/core/squid-constructors.js';
import { ConstructionBuilderState, RawConstructionBuilder, makeConstructionBuilderHooks, makeConstructionBuilderLocalHooks, makeConstructionBuilderNoticeHooks } from '../www/core/raw-construction-builder.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-09 WB04-E142)
const allRows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'constructionbuilder-x86.tsv')) ?? [];
const rows = allRows.filter((row) => row[0] === 'originals');

// 실행기 입력 열의 순서이다. 기대 계산에 사용하지 않고 경계 응답과 장면 자료만 공급한다. (2026-11-09 WB04-E142)
const [PLAYER, ANCHOR, OWNER, MASK, GENUS, X, Y, FOOT_X, FOOT_Y, TIME_LOW, TIME_HIGH, NOW_LOW, NOW_HIGH, PATH, SPEED, SOURCES, ALLOCATION, FRAME, MUTATE, BRIDGE, COUNT] = Array.from({ length: 21 }, (_, index) => index);
// 각 PE 의 독립 원본 행 수와 고정 입력 타입 번호, 가상 표 기록값, 풀 크기이다. (2026-11-09 WB04-E142)
const ROWS = 370;
const BUILDER = 134;
const CARRIER = 105;
const CHANGED_CARRIER = 106;
const VTABLE = 0x11010000;
const CAPACITY = 24000;

// 예외가 나는지 본다. (2026-11-09 WB04-E142)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 바이트를 소문자 16진수로 만든다. (2026-11-09 WB04-E142)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 시간 입력 비트를 손실 없이 double 로 조합한다. (2026-11-09 WB04-E142)
const view = new DataView(new ArrayBuffer(8));
function time(low, high) {
    view.setUint32(0, low, true);
    view.setUint32(4, high, true);
    return view.getFloat64(0, true);
}

// 실행기와 같은 네 항목 목록 입력이다. 소비 결과는 JS 모듈에서만 얻는다. (2026-11-09 WB04-E142)
const sourceInput = (profile) => [[0, 0, 0, 0], [70, 0, 0, 0], [0, 70, 71, 70], [71, 0, 70, 0]][profile];

// 공통 ProcessForm 생성이 읽는 원문 내장 Form 생성자 주소를 입력에 명시한 타입 표이다. (2026-11-09 WB04-E142)
const formTypeInputs = () => Array.from({ length: 188 }, (_, number) => ({ cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, maxHitPoints: 0, ...([1, 2, 3, 6].includes(number) ? { constructorAddress: typeConstructorAddress(number) } : {}) }));

// 실제 풀과 입력 메타 자료, 동작 밖 명시 효과의 관찰 장면이다. (2026-11-09 WB04-E142)
class Scene {
    constructor() {
        this.pool = new SidPool(CAPACITY, { server: false });
        this.types = formTypeInputs();
        this.metadata = Array.from({ length: 188 }, () => ({ speed: 0, sourceFrameIndex: 0, firstFrames: new Array(16).fill(0) }));
        this.state = new ConstructionBuilderState();
        this.path = { currentStep: 0, arrivalTimes: [10, 12.5] };
        this.input = [];
        this.events = [];
        this.born = 0;
        // 생성/직접 쓰기의 호스트 계약을 만족하도록 입력 슬롯을 실제로 확보한다.
        for (let sid = 5; sid <= 127; sid += 1) assert.equal(this.pool.allocate(2), sid);
    }

    // 실제 호출 인자들을 실행기와 같은 비트 문자열로 기록한다. (2026-11-09 WB04-E142)
    event(letter, values) {
        this.events.push(`${letter}${values.map((value) => `:${value >>> 0}`).join('')}`);
    }

    // raw 입력만 공급한다. 이동 결과나 기대 중심은 계산하지 않는다. (2026-11-09 WB04-E142)
    node(sid, type, x, y) {
        const raw = this.pool.allocated(sid);
        raw.fill(0);
        raw[10] = type;
        raw[34] = this.input[OWNER] & 0xff;
        put(raw, 14, floatToBits(x));
        put(raw, 18, floatToBits(y));
    }

    // 이전 행의 상태를 제거하고 실제 PE 에 공급했던 입력 자료로 채운다. (2026-11-09 WB04-E142)
    prepare(values) {
        assert.equal(values.length, 21);
        this.input = values;
        this.events = [];
        this.born = 0;
        this.path = { currentStep: 0, arrivalTimes: [10, 12.5] };
        const input = values;
        // 객체 주소는 유지하고 내용만 새 값으로 바꾼다(이미 만든 몸체가 같은 전역을 계속 읽는다).
        Object.assign(this.state, new ConstructionBuilderState(), { builderType: BUILDER, carrierType: CARRIER, bridgeType: input[BRIDGE], approachMask: input[MASK], timerPayload: bitsToFloat(0x25b), now: time(input[NOW_LOW], input[NOW_HIGH]) });
        this.state.sources.entries = sourceInput(input[SOURCES]).slice();
        this.state.sources.count = input[COUNT];
        // helper 가 조회할 모든 타입에 발자국/속도/프레임 입력을 준비한다.
        for (const type of [100, 83, 84, 85, BUILDER, CARRIER, CHANGED_CARRIER]) {
            const record = this.types[type];
            Object.assign(record, { flags2: input[GENUS], footX: type === 100 ? input[FOOT_X] | 0 : type >= 83 && type <= 85 ? 3 : 1, footY: type === 100 ? input[FOOT_Y] | 0 : type >= 83 && type <= 85 ? 2 : 1 });
            const meta = this.metadata[type];
            meta.speed = bitsToFloat(input[SPEED]);
            meta.sourceFrameIndex = type === 84 ? 1 : type === 85 ? 2 : 0;
            // 변경된 운반 타입은 서로 다른 표로 현재 타입 재조회를 확인한다.
            for (let index = 0; index < meta.firstFrames.length; index += 1) meta.firstFrames[index] = (input[FRAME] + index + (type === CHANGED_CARRIER ? 256 : 0)) | 0;
        }
        this.node(50, 100, bitsToFloat(input[X]), bitsToFloat(input[Y]));
        this.node(60, 83, 10.25, 11.5);
        this.node(70, 84, 30.25, 31.5);
        this.node(71, 85, 40.5, 41.75);
    }

    // 메타 계산·소비를 대체하지 않고 경로/가상 공간/자원 정책/할당 응답만 공급한다. (2026-11-09 WB04-E142)
    hooks() {
        const bits = floatToBits;
        const v = () => this.input;
        return {
            anchor: (player, type, x, y) => { this.event('A', [player, type, bits(x), bits(y)]); return v()[ANCHOR]; },
            // 원본 factory 경계와 같은 raw 패턴으로 주변 바이트 보존을 비교한다.
            create: (type, flags) => {
                const sid = 100 + this.born;
                this.born += 1;
                const raw = this.pool.allocated(sid);
                for (let index = 0; index < raw.length; index += 1) raw[index] = (index * 37 + 11) & 0xff;
                put(raw, 0, VTABLE);
                raw[10] = v()[MUTATE] === 6 && this.born > 1 ? CHANGED_CARRIER : type;
                raw[11] = 4;
                this.event('C', [type, flags, sid]);
                return sid;
            },
            // 현재 소유자를 쓰고 첫 대상 좌표가 경계 뒤 바뀌는 입력도 공급한다.
            setOwner: (sid, owner) => {
                this.event('O', [sid, owner]);
                this.pool.allocated(sid)[34] = owner & 0xff;
                if (v()[MUTATE] === 3 && sid === 100) {
                    const raw = this.pool.allocated(50);
                    put(raw, 14, bits(22.5));
                    put(raw, 18, bits(24.5));
                }
            },
            // 접근점 정책이 받은 실제 기준점 발자국과 목표/플래그를 관찰한다.
            approach: (rect, x, y, flags) => { this.event('P', [bits(x), bits(y), flags, bits(rect[0]), bits(rect[1]), bits(rect[2]), bits(rect[3])]); return { x: 10.25, y: 11.5 }; },
            // 가상 공간 Pop 은 입력 인자만 저장한다.
            pop: (sid, x, y, flags) => this.event('B', [sid, bits(x), bits(y), flags]),
            // 최초 경로만 돌려주며 운반 경로 실패 뒤에도 원본 root 가 계속하는지 비교한다.
            move: (sid, x, y) => {
                this.event('M', [sid, bits(x), bits(y), 0, 0, 0, 0, 0]);
                if (v()[MUTATE] === 2 && sid !== 100) this.path.currentStep = 1;
                return sid === 100 && v()[PATH] ? this.path : null;
            },
            // 속도 적용 뒤 첫 경로의 단계가 바뀌는 원본 입력 경계이다.
            speed: (route, speed, flags) => {
                assert.equal(route, this.path);
                this.event('V', [bits(speed), flags]);
                if (v()[MUTATE] === 1) this.path.currentStep = 1;
            },
            // 자원 선택 정책은 공유 목록을 공급하며 현재 대상/기준점 소유자도 바꿀 수 있다.
            chooseSources: (x, y, type, owner) => {
                this.event('Q', [bits(x), bits(y), type, owner]);
                this.state.sources.entries = sourceInput(v()[SOURCES]).slice();
                if (v()[MUTATE] === 4) {
                    const raw = this.pool.allocated(50);
                    put(raw, 14, bits(25.5));
                    put(raw, 18, bits(26.5));
                    this.pool.allocated(60)[34] = 8;
                }
            },
            // 정렬 응답으로 순서/현재 운반 타입 또는 공급원 현재 좌표의 변경을 공급한다.
            sortSources: (x, y) => {
                this.event('S', [bits(x), bits(y)]);
                if (v()[MUTATE] === 7) { this.state.sources.entries = [71, 70, 0, 0]; this.state.carrierType = CHANGED_CARRIER; }
                if (v()[MUTATE] === 8) {
                    const raw = this.pool.allocated(70);
                    put(raw, 14, bits(32.75));
                    put(raw, 18, bits(33.25));
                }
            },
            // 현재 타입이 현재 다리 전역과 같을 때만 실제 단어 helper 가 호출하는 경계이다.
            notifySurface: (sid) => this.event('N', [sid]),
            // 할당 실패와 할당 중 이벤트 변경을 입력으로 공급한다.
            reserveTimer: () => {
                this.event('L', [40]);
                if (v()[MUTATE] === 5) this.state.timerPayload = bitsToFloat(0x12345678);
                return v()[ALLOCATION] !== 0;
            },
            // 원본 생성자의 실제 인자 순서(event,parent,payload)로 호출 비트를 기록한다.
            addTimer: (sid, event, payload) => this.event('T', [event, sid, bits(payload)]),
        };
    }

    // 현재 장면으로 이동 몸체를 만든다. (2026-11-09 WB04-E142)
    builder(hooks = this.hooks()) {
        return new RawConstructionBuilder(this.pool, this.types, this.metadata, this.state, hooks);
    }

    // 관찰 문자열을 구분자로 묶는다. 빈 관찰은 fixture 의 '-' 로 표시한다. (2026-11-09 WB04-E142)
    eventsText() { return this.events.length === 0 ? '-' : this.events.join(';'); }
    // 실제 생성 raw 전체를 새 객체 번호 순서로 직렬화한다. (2026-11-09 WB04-E142)
    raws() { return this.born === 0 ? '-' : Array.from({ length: this.born }, (_, index) => hex(this.pool.slot(100 + index))).join(';'); }
    // 최종 공유 목록은 개수 밖 꼬리도 그대로 비교한다. (2026-11-09 WB04-E142)
    sourcesText() { return this.state.sources.entries.join(','); }
}

// 기본 장면의 입력만 제공한다. 유효 대상/기준점과 공급원 두 종류이다. (2026-11-09 WB04-E142)
const defaultInput = () => [3, 60, 3, 4, 4, floatToBits(20.5), floatToBits(21.25), 3, 2, 0, 0, 0, 0, 1, floatToBits(1.5), 2, 1, 0x12345678, 0, 82, 4];

test('건설 사제 이동: 원본 10.78 관측 370행의 사건 순서·반환 비트·생성 raw·공급원 목록이 같다', { skip: rows.length === 0 }, () => {
    assert.equal(rows.length, ROWS);
    const scene = new Scene();
    const builder = scene.builder();
    // 저장 행마다 장면만 공급하며 기대 반환/기하/프레임은 재계산하지 않는다.
    for (const row of rows) {
        assert.equal(row.length, 7);
        scene.prepare(row[2].split(',').map(Number));
        let result;
        if (row[1] === 'Move') {
            const value = builder.move(scene.input[PLAYER], 50, scene.input[TIME_LOW], scene.input[TIME_HIGH]);
            const bytes = new Uint8Array(8);
            view.setFloat64(0, value, true);
            for (let index = 0; index < 8; index += 1) bytes[index] = view.getUint8(index);
            result = hex(bytes);
        } else {
            const sid = builder.takeSource();
            result = `${sid ? '1:' : '0:'}${sid}`;
        }
        const where = `${row[1]} ${row[2]}`;
        assert.equal(scene.eventsText(), row[3], where);
        assert.equal(result, row[4], where);
        assert.equal(scene.raws(), row[5], where);
        assert.equal(scene.sourcesText(), row[6], where);
        assert.equal(scene.state.sources.count, scene.input[COUNT], where);
    }
});

test('건설 사제 이동: signed 개수 조기 반환을 보존하고 손상 목록·메타·경로·풀 밖 기준점은 거부한다', () => {
    const scene = new Scene();
    scene.prepare(defaultInput());
    const hooks = scene.hooks();
    const builder = scene.builder(hooks);
    scene.state.sources.entries = [];
    scene.state.sources.count = 0xffffffff;
    assert.equal(builder.takeSource(), 0);
    scene.state.sources.count = 1;
    assert.ok(throws(() => builder.takeSource()));
    scene.prepare(defaultInput());
    scene.metadata[84].sourceFrameIndex = 16;
    assert.ok(throws(() => builder.move(3, 50, 0, 0)));
    scene.prepare(defaultInput());
    scene.path.currentStep = 2;
    assert.ok(throws(() => builder.move(3, 50, 0, 0)));
    scene.prepare(defaultInput());
    scene.input[ANCHOR] = 0x1003c;
    assert.ok(throws(() => builder.move(3, 50, 0, 0)));
    for (const name of ['anchor', 'create', 'setOwner', 'approach', 'pop', 'move', 'speed', 'notifySurface', 'reserveTimer', 'addTimer', 'chooseSources', 'sortSources']) {
        assert.ok(throws(() => scene.builder({ ...hooks, [name]: undefined })), name);
    }
    assert.ok(throws(() => new RawConstructionBuilder({ edition: 'cd1072' }, scene.types, scene.metadata, scene.state, hooks)));
});

test('건설 사제 이동: 기준점 실패는 생성 전에 반환하고 첫 경로 실패는 앞선 효과를 유지하며 타이머 확보 실패에도 이동은 계속된다', () => {
    const scene = new Scene();
    scene.prepare(defaultInput());
    const builder = scene.builder();
    scene.input[ANCHOR] = 0;
    assert.equal(builder.move(3, 50, 0, 0), 5);
    assert.equal(scene.born, 0);
    assert.equal(scene.events.length, 1);
    scene.prepare(defaultInput());
    scene.input[PATH] = 0;
    assert.equal(builder.move(259, 50, 0, 0), 5);
    assert.equal(scene.born, 1);
    assert.equal(scene.events.length, 6);
    scene.prepare(defaultInput());
    scene.input[ALLOCATION] = 0;
    assert.equal(builder.move(3, 50, 0, 0), 10);
    assert.equal(scene.born, 4);
    assert.equal(scene.events.filter((event) => event.startsWith('M:')).length, 4);
    assert.ok(!scene.events.some((event) => event.startsWith('T:')));
});

test('건설 사제 이동: 어댑터는 같은 풀의 기준점/factory/소유자/Regular 와 로컬·통지 이동 경계를 잇는다', () => {
    const scene = new Scene();
    scene.prepare(defaultInput());
    const log = [];
    const anchor = { pool: scene.pool, locate: (owner, type, x, y) => { log.push(['locate', owner, type]); return 60; } };
    const factory = { pool: scene.pool, create: (type, flags) => { log.push(['create', type, flags]); return 105; } };
    const owner = { pool: scene.pool, set: (sid, player) => log.push(['set', sid, player]) };
    const process = { pool: scene.pool, addRegular: (sid, event, payload) => log.push(['regular', sid, event, payload]) };
    const other = new SidPool(CAPACITY, { server: false });
    const base = scene.hooks();
    assert.ok(throws(() => makeConstructionBuilderHooks(other, anchor, factory, owner, base)));
    assert.ok(throws(() => makeConstructionBuilderHooks(scene.pool, anchor, { pool: other, create: () => 0 }, owner, base)));
    assert.ok(throws(() => makeConstructionBuilderHooks(scene.pool, anchor, factory, owner, base, { pool: other })));
    const connected = makeConstructionBuilderHooks(scene.pool, anchor, factory, owner, base, process);
    assert.equal(connected.pop, base.pop);
    assert.equal(connected.anchor(3, 100, 20, 21), 60);
    assert.equal(connected.create(100, 2), 105);
    connected.setOwner(100, 3);
    connected.addTimer(105, 0, 0.5);
    assert.deepEqual(log, [['locate', 3, 100], ['create', 100, 2], ['set', 100, 3], ['regular', 105, 0, 0.5]]);
    // 로컬 배치·통지의 moveBuilder 는 같은 이동을 부르고 double 결과를 버린다.
    const builder = scene.builder();
    assert.ok(throws(() => makeConstructionBuilderLocalHooks(other, builder, {})));
    assert.ok(throws(() => makeConstructionBuilderNoticeHooks(other, builder, {})));
    const local = makeConstructionBuilderLocalHooks(scene.pool, builder, { keep: 1 });
    assert.equal(local.keep, 1);
    scene.events.length = 0;
    assert.equal(local.moveBuilder(3, 50, 0, 0), undefined);
    assert.ok(scene.events.length > 0 && scene.events[0].startsWith('A:'));
    scene.prepare(defaultInput());
    const notice = makeConstructionBuilderNoticeHooks(scene.pool, builder, {});
    notice.moveBuilder(3, 50, 0, 0);
    assert.ok(scene.events[0].startsWith('A:'));
});
