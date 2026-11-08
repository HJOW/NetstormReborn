// 건설 프로세스 raw-construction-process.js(RawConstructionProcess·ConstructionProcess·두 어댑터)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: constructionprocess-x86.tsv 의 `originals` 956행(Start/Run/Complete/FormDestroyed) — 부모 raw·전역·시각(64비트 double)·확보 결과·경계 변동(Mutate 0~16)을 주었을 때
//    외부 요청 순서(A 확보/E·I 표시 효과/J 부착/S 소리 조회/P 소리 프로세스/R 갱신/H 최대 HP/D 표시/X contained 삭제/W 완료음/G 준비 소리/U Unpop/Q 재등록/B 밀어내기/K Kill),
//    프로세스 필드(dirty·시작·끝·소리), 슬롯 50바이트 전체가 같다. construction-form-parent-x86.tsv 64행은 선택된 checking 의 현재 Form 조회·연결 진단·진단 복귀 접두를 비교한다.
//    `originalCD`/`original1037` 행은 다른 판본이라 사용하지 않는다.
//  - 계약/통합: 경계 누락·다른 판본/풀 거부, 확보 실패 효과, 지속 시간별 기본 시간·진행음, Unpop 이 void 를 켜지 않을 때 거부, 통지 어댑터, 실제 host/Kernel/삭제기/보상 모듈 위의 진행→완료,
//    abstract 가 아닌 부모의 둘째 Kill 생략, 권한 없음/quickBuild, 죽은 부모, 빌린 boss 재확인, 실제 Form 연결 진단 뒤 복귀.
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E132: 건설 프로세스 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get, floatToBits, bitsToFloat } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { GameRandom } from '../www/core/random.js';
import { ScrambledSpStore } from '../www/core/sp-store.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { SquidUnpop } from '../www/core/squid-unpop.js';
import { RawSquidDestroy } from '../www/core/squid-destroy.js';
import { SquidPostPopState } from '../www/core/squid-postpop-state.js';
import { SquidReward, SquidRewardState } from '../www/core/squid-reward.js';
import { typeConstructorAddress } from '../www/core/squid-constructors.js';
import { Kernel } from '../www/core/kernel.js';
import { SquidProcessHost, SquidProcessState } from '../www/core/squid-process.js';
import { checkRawFormLinked, readRawProcessFormParent } from '../www/core/raw-form-linked.js';
import { ConstructionProcessState, RawConstructionProcess, createConstructionProcessFields, makeConstructionProcessHooks, makeConstructionProcessNoticeHooks } from '../www/core/raw-construction-process.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E132)
const allRows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'constructionprocess-x86.tsv')) ?? [];
const rows = allRows.filter((row) => row[0] === 'originals');
const formParentPath = path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'construction-form-parent-x86.tsv');
const formParentRows = readRows(formParentPath) ?? [];

// 실행기의 고정 입력값(부착된 form 번호, 소리 항목, 경계 변동이 쓰는 값)과 풀 크기, 합성 가상 표, 판본 행 수이다. (2026-11-08 WB04-E132)
const ROWS = 956;
const FORM = 90;
const SOUND = 0x15000040;
const CHANGED_SOUND = 0x15000200;
const CHANGED_READY = 0x15000400;
const CAPACITY = 24000;
const VTABLE = 0x11010000;
const HAS_HP = 0x10;

// 실행기 입력 열의 순서이다. 기대 계산에 사용하지 않고 장면 자료와 경계 응답만 공급한다. (2026-11-08 WB04-E132)
const [PARENT, DURATION, NOW_LOW, NOW_HIGH, ALLOCATION, MUTATE, TYPE, STATE, EXTRA, HP, OWNER, X, Y, GENUS, READY, MAX, START_LOW, START_HIGH, END_LOW, END_HIGH,
    LOCAL, BOSS, PUSH_GENUS, DAIS, FINISH_ALL, FINISH_GENUS, PROCESS_TYPE, SOUND_TYPE, FLAGS, INPUT_COUNT] = Array.from({ length: 30 }, (_, index) => index);

// 예외가 나는지 본다. (2026-11-08 WB04-E132)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 바이트 배열을 소문자 16진수로 만든다. (2026-11-08 WB04-E132)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 시각 입력 비트(하위/상위 DWORD)를 손실 없이 double 로 조합한다. (2026-11-08 WB04-E132)
function time(low, high) {
    const view = new DataView(new ArrayBuffer(8));
    view.setUint32(0, low, true);
    view.setUint32(4, high, true);
    return view.getFloat64(0, true);
}

// double 의 하위/상위 DWORD 이다. (2026-11-08 WB04-E132)
function doubleWords(value) {
    const view = new DataView(new ArrayBuffer(8));
    view.setFloat64(0, value, true);
    return [view.getUint32(0, true), view.getUint32(4, true)];
}

// 내장 Form 네 생성자 주소만 넣은 188칸 TYPE 입력이다(cpppj MakeFormTypeInputs). (2026-11-08 WB04-E132)
const formTypeInputs = () => Array.from({ length: 188 }, (_, number) => ({ cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, maxHitPoints: 0, ...([1, 2, 3, 6].includes(number) ? { constructorAddress: typeConstructorAddress(number) } : {}) }));

// 실제 풀과 입력 타입, 동작 밖 명시 효과의 관찰 장면이다. (2026-11-08 WB04-E132)
class ProcessScene {
    constructor() {
        this.pool = new SidPool(CAPACITY, { server: false });
        this.types = formTypeInputs();
        this.state = new ConstructionProcessState();
        this.fields = createConstructionProcessFields();
        this.input = [];
        this.events = [];
        this.ready = 0;
        this.hpCalls = 0;
    }

    // 이 행의 부모 번호와 그 raw 슬롯이다.
    get parentSid() { return this.input[PARENT]; }
    raw() { return this.pool.physical(this.parentSid); }

    // 실제 호출 인자들을 실행기와 같은 10진 문자열로 기록한다.
    event(letter, values) {
        this.events.push([letter, ...values].join(':'));
    }

    // 이전 행의 상태를 제거하고 실제 PE 에 공급했던 입력 자료로 채운다. 시작 연산만 필드를 비워 둔다.
    prepare(values, start) {
        assert.equal(values.length, INPUT_COUNT);
        this.input = values;
        this.events = [];
        this.hpCalls = 0;
        this.ready = values[READY];
        Object.assign(this.state, new ConstructionProcessState(), {
            now: time(values[NOW_LOW], values[NOW_HIGH]), processType: values[PROCESS_TYPE], soundProcessType: values[SOUND_TYPE], localPlayer: values[LOCAL],
            boss: values[BOSS] !== 0, pushGenus: values[PUSH_GENUS], daisType: values[DAIS], quickBuild: values[FINISH_ALL] !== 0, quickVortex: values[FINISH_GENUS] !== 0,
        });
        this.types[values[TYPE]] = { cost: 0, flags1: 0, flags2: values[GENUS], footX: 0, footY: 0, maxHitPoints: 0 };
        const raw = this.raw();
        // 실행기와 같은 표식 바이트로 주변 필드 보존을 비교한다.
        for (let index = 0; index < raw.length; index += 1) raw[index] = (index * 29 + 7) & 0xff;
        put(raw, 0, VTABLE);
        put(raw, 6, FORM, 2);
        raw[10] = values[TYPE] & 0xff;
        raw[11] = values[STATE] & 0xff;
        put(raw, 14, values[X]);
        put(raw, 18, values[Y]);
        put(raw, 26, values[HP], 4);
        raw[34] = values[OWNER] & 0xff;
        raw[40] = values[EXTRA] & 0xff;
        this.fields = createConstructionProcessFields();
        if (!start) {
            this.fields = { dirty: 0, start: time(values[START_LOW], values[START_HIGH]), end: time(values[END_LOW], values[END_HIGH]), sound: SOUND, additional: 0 };
        }
    }

    // 시각/비율/조건 계산을 대체하지 않고 효과 응답과 실행기와 같은 경계 변동만 공급한다.
    hooks() {
        const input = () => this.input;
        return {
            // 객체 확보: 실패하면 생성자를 부르지 않는다.
            reserve: () => { this.event('A', [56]); return (input()[ALLOCATION] & 1) !== 0; },
            // 표시 효과 경계 뒤 현재 시각/프로세스 타입이 바뀌는 입력을 공급한다.
            effect: (parent, instant) => {
                this.event(instant ? 'I' : 'E', [parent]);
                if (input()[MUTATE] === 1) this.state.now = 7.75;
                if (input()[MUTATE] === 2) this.state.processType = 44;
            },
            // 부착은 받은 필드를 보관하고 그 저장소를 돌려준다. 부착 뒤의 시각 변경은 필드에 영향이 없어야 한다.
            attach: (initial, type, parent, flags) => {
                this.event('J', [type, parent, 0, flags]);
                this.fields = { ...initial };
                if (input()[MUTATE] === 5) this.state.now = 99.5;
                return this.fields;
            },
            // 소리 조회는 이름을 그대로 기록한다.
            loadSound: (name) => { this.events.push(`S:${name}`); return input()[MUTATE] === 4 ? 0 : SOUND; },
            // 소리 프로세스 확보 경계가 현재 소리 필드를 바꾸는 입력이다.
            reserveSound: () => {
                this.event('A', [40]);
                if (input()[MUTATE] === 3) this.fields.sound = CHANGED_SOUND;
                return (input()[ALLOCATION] & 2) !== 0;
            },
            attachSound: (parent, sound, alternate, flags) => { this.event('P', [parent, sound, alternate, flags]); },
            // 갱신 요청 뒤 로컬 플레이어 전역이 바뀌는 입력이다.
            refresh: () => { this.events.push('R'); if (input()[MUTATE] === 10) this.state.localPlayer = 2; },
            // 최대 HP 경계: 현재 시각/부모 HP 변경과 둘째 호출의 다른 반환을 공급한다.
            maxHitPoints: (sid) => {
                this.event('H', [sid]);
                this.hpCalls += 1;
                let result = input()[MAX];
                if (input()[MUTATE] === 6) this.state.now = 6;
                if (input()[MUTATE] === 9) put(this.raw(), 26, input()[MAX], 4);
                if (input()[MUTATE] === 14 && this.hpCalls > 1) result = (result + 7) >>> 0;
                return result | 0;
            },
            // 표시 경계가 abstract 비트를 뒤집는 입력이다.
            updateDisplay: (sid) => { this.event('D', [sid]); if (input()[MUTATE] === 7) this.raw()[40] ^= 1; },
            // contained 삭제 경계가 부모 소유자를 바꾸는 입력이다.
            removeContained: (sid, type) => { this.event('X', [sid, type, 0, 1]); if (input()[MUTATE] === 15) this.raw()[34] = 2; },
            // 완료음 경계가 권한/준비 소리를 바꾸는 입력이다.
            playAt: (x, y, name, loop) => {
                this.events.push(`W:${floatToBits(x)}:${floatToBits(y)}:${name}:${loop}`);
                if (input()[MUTATE] === 11) this.state.boss = !this.state.boss;
                if (input()[MUTATE] === 16) this.ready = CHANGED_READY;
            },
            readySound: (type) => { assert.equal(type, input()[TYPE]); return this.ready; },
            play: (sound) => { this.event('G', [sound, 0, 0, 0, 0, 0]); },
            // 실제 Unpop/재등록처럼 void 를 켜고 끈다. Unpop 은 abstract 바이트의 다른 비트도 바꿀 수 있다.
            unpop: (sid, flags) => {
                this.event('U', [sid, flags]);
                this.raw()[11] |= 4;
                if (input()[MUTATE] === 13) this.raw()[40] |= 0x80;
            },
            repop: (sid, flags) => {
                this.event('Q', [sid, flags]);
                this.raw()[11] &= 0xfb;
                if (input()[MUTATE] === 12) { put(this.raw(), 14, floatToBits(40.5)); put(this.raw(), 18, floatToBits(41.25)); this.state.pushGenus = 0x200; }
            },
            push: (x, y, type, owner) => { this.event('B', [floatToBits(x), floatToBits(y), type, owner]); },
        };
    }

    // 실행기의 Kill 경계는 프로세스를 없애지 않으므로 항상 남아 있다고 답한다. 권한 전역 변경 입력도 공급한다.
    kill() {
        return (flags) => { this.event('K', [flags]); if (this.input[MUTATE] === 8) this.state.boss = !this.state.boss; return true; };
    }

    // 관찰 문자열을 구분자로 묶는다. 빈 관찰은 fixture 의 '-' 로 표시한다.
    eventsText() {
        return this.events.length ? this.events.join(';') : '-';
    }

    // dirty 와 두 시각의 비트, 소리 항목을 실행기와 같은 순서로 적는다.
    fieldsText() {
        const [startLow, startHigh] = doubleWords(this.fields.start);
        const [endLow, endHigh] = doubleWords(this.fields.end);
        return [this.fields.dirty, startLow, startHigh, endLow, endHigh, this.fields.sound].join(':');
    }
}

// 기본 장면의 입력만 제공한다: 서버 번호의 abstract 부모, HP 가 최대와 같고 로컬 소유이며 권한이 있다. (2026-11-08 WB04-E132)
function defaultInput() {
    const input = new Array(INPUT_COUNT).fill(0);
    Object.assign(input, { [PARENT]: 20000, [DURATION]: floatToBits(2.5), [NOW_HIGH]: 0x40000000, [ALLOCATION]: 3, [TYPE]: 100, [EXTRA]: 1, [HP]: 100, [OWNER]: 1,
        [X]: floatToBits(20.5), [Y]: floatToBits(21.25), [GENUS]: 0x4000, [MAX]: 100, [END_HIGH]: 0x40200000, [LOCAL]: 1, [BOSS]: 1, [PUSH_GENUS]: 0x10444200, [DAIS]: 165,
        [PROCESS_TYPE]: 30, [SOUND_TYPE]: 47 });
    return input;
}

// 실제 모듈 장면의 타입 표이다. 타입 100 은 HP 300 의 factory genus 이다. host 와 보상 모듈이 생성 때 표를 복사하므로 먼저 채운다. (2026-11-08 WB04-E132)
function realTypes() {
    const types = formTypeInputs();
    types[100] = { ...types[100], flags1: HAS_HP, flags2: 0x4000, maxHitPoints: 300, footX: 1, footY: 1 };
    return types;
}

// 실제 프로세스 host/Kernel/공통 삭제/보상 모듈 위에 건설 프로세스를 올리고 나머지 효과만 기록하는 장면이다. (2026-11-08 WB04-E132)
class RealScene {
    constructor() {
        this.pool = new SidPool(CAPACITY, { server: true });
        this.hash = new SquidHash();
        this.spots = new Uint8Array(65536);
        this.types = realTypes();
        this.unpop = new SquidUnpop(this.pool, this.hash, this.spots);
        this.destroy = new RawSquidDestroy(this.pool, this.unpop, this.types);
        this.kernel = new Kernel();
        this.clock = new SquidProcessState();
        this.host = new SquidProcessHost(this.pool, this.types, this.kernel, this.destroy, this.clock);
        this.books = new SquidPostPopState();
        this.rewardState = new SquidRewardState();
        this.rng = new GameRandom();
        this.store = new ScrambledSpStore(this.rng, () => 1);
        this.reward = new SquidReward(this.pool, this.types, this.books, this.rewardState, this.store);
        this.state = new ConstructionProcessState();
        this.events = [];
        this.parent = this.pool.allocate(0);
        assert.ok(this.parent >= this.pool.layout.serverFirst);
        const raw = this.pool.allocated(this.parent);
        raw[10] = 100;
        raw[11] = 0;
        raw[34] = 1;
        raw[40] = 1;
        put(raw, 14, floatToBits(20.5));
        put(raw, 18, floatToBits(21.25));
        this.state.now = 2;
    }

    // 보상 모듈의 실제 최대 HP 외의 효과는 이름만 기록한다. Unpop/재등록은 실제처럼 void 를 켜고 끈다.
    hooks() {
        const push = (text) => { this.events.push(text); };
        return makeConstructionProcessHooks(this.pool, this.reward, null, {
            reserve: () => true,
            effect: (_, instant) => push(instant ? 'I' : 'E'),
            loadSound: () => 77,
            reserveSound: () => true,
            attachSound: (_, sound, __, flags) => push(`P:${sound}:${flags}`),
            refresh: () => push('R'),
            updateDisplay: () => push('D'),
            removeContained: (_, type) => push(`X:${type}`),
            playAt: (_x, _y, name) => push(`W:${name}`),
            readySound: () => 85,
            play: (sound) => push(`G:${sound}`),
            unpop: (sid) => { push('U'); this.pool.allocated(sid)[11] |= 4; },
            repop: (sid, flags) => { push(`Q:${flags}`); this.pool.allocated(sid)[11] &= 0xfb; },
            push: (_x, _y, type, owner) => push(`B:${type}:${owner}`),
        });
    }

    // 부모의 현재 HP 이다. 
    hp() { return get(this.pool.slot(this.parent), 26, 4); }

    // 기록한 사건을 ';' 로 잇는다.
    eventsText() { return this.events.join(';'); }
}

test('ConstructionProcess: 원본 10.78 Start/Run/Complete/FormDestroyed 956행의 사건 순서·프로세스 필드·부모 슬롯 전체가 같다', { skip: rows.length === 0 && '원본 fixture 없음' }, () => {
    const scene = new ProcessScene();
    const process = new RawConstructionProcess(scene.pool, scene.types, scene.state, scene.hooks());
    let count = 0;
    // 저장 행마다 장면만 공급하며 기대 시각/HP/조건은 재계산하지 않는다.
    for (const row of rows) {
        assert.equal(row.length, 6);
        const start = row[1] === 'Start';
        scene.prepare(row[2].split(',').map(Number), start);
        const parent = scene.parentSid;
        if (start) process.start(parent, bitsToFloat(scene.input[DURATION]));
        else if (row[1] === 'Run') process.run(scene.fields, parent, scene.kill());
        else if (row[1] === 'Complete') process.complete(parent);
        else process.formDestroyed(parent);
        const fields = start && !(scene.input[ALLOCATION] & 1) ? '-' : scene.fieldsText();
        assert.deepEqual([scene.eventsText(), fields, hex(scene.pool.slot(parent))], [row[3], row[4], row[5]], `건설 프로세스 행 ${count} ${row[1]}`);
        count += 1;
    }
    assert.equal(count, ROWS);
});

test('ConstructionProcess: 필수 효과가 비었거나 타입 표/host 풀이 맞지 않으면 첫 효과 전에 거부한다(host 가 있으면 attach 는 비워도 된다)', () => {
    const scene = new ProcessScene();
    scene.prepare(defaultInput(), true);
    let hooks = scene.hooks();
    delete hooks.push;
    assert.ok(throws(() => new RawConstructionProcess(scene.pool, scene.types, scene.state, hooks)));
    hooks = scene.hooks();
    delete hooks.attach;
    assert.ok(throws(() => new RawConstructionProcess(scene.pool, scene.types, scene.state, hooks)));
    assert.ok(throws(() => new RawConstructionProcess(scene.pool, scene.types.slice(0, 171), scene.state, scene.hooks())));
    const real = new RealScene();
    assert.ok(throws(() => new RawConstructionProcess(scene.pool, scene.types, scene.state, hooks, real.host)));
    assert.ok(!throws(() => new RawConstructionProcess(real.pool, real.types, real.state, hooks, real.host)));
});

test('ConstructionProcess: 객체 확보 실패는 효과가 없고 소리 프로세스 확보 실패는 소리 항목을 남긴 채 갱신까지 진행한다', () => {
    const scene = new ProcessScene();
    const process = new RawConstructionProcess(scene.pool, scene.types, scene.state, scene.hooks());
    const input = defaultInput();
    input[ALLOCATION] = 0;
    scene.prepare(input, true);
    process.start(scene.parentSid, 2.5);
    assert.equal(scene.eventsText(), 'A:56');
    input[ALLOCATION] = 1;
    scene.prepare(input, true);
    process.start(scene.parentSid, 2.5);
    assert.ok(scene.eventsText() === 'A:56;E:20000;J:30:20000:0:16;S:jimBuild.wav;A:40;R' && scene.fields.sound === SOUND);
    assert.ok(scene.fields.dirty === 0xff && scene.fields.start === 2 && scene.fields.end === 4.5);
});

test('ConstructionProcess: 지속 시간 0 은 10초 건설이 되고 1초 이하는 진행음이 없으며 NaN 은 소리가 없다', () => {
    const scene = new ProcessScene();
    const process = new RawConstructionProcess(scene.pool, scene.types, scene.state, scene.hooks());
    scene.prepare(defaultInput(), true);
    process.start(scene.parentSid, -0);
    assert.ok(scene.events[1] === 'I:20000' && scene.fields.end === 12 && scene.fields.sound === SOUND);
    scene.prepare(defaultInput(), true);
    process.start(scene.parentSid, 1);
    assert.ok(scene.events[1] === 'E:20000' && scene.fields.end === 3 && scene.fields.sound === 0 && !scene.eventsText().includes('S:'));
    scene.prepare(defaultInput(), true);
    process.start(scene.parentSid, NaN);
    assert.equal(scene.fields.sound, 0);
});

test('ConstructionProcess: Unpop 경계가 void 를 켜지 않으면 abstract 를 풀지 않고 거부한다', () => {
    const scene = new ProcessScene();
    scene.prepare(defaultInput(), false);
    const hooks = scene.hooks();
    hooks.unpop = () => {};
    const process = new RawConstructionProcess(scene.pool, scene.types, scene.state, hooks);
    assert.ok(throws(() => process.complete(scene.parentSid)) && (scene.raw()[40] & 1));
});

test('ConstructionProcess: 통지의 건설 시작 경계는 단정도 비트 0 을 지속 시간 0 으로 넘겨 기본 10초 건설을 시작한다', () => {
    const scene = new ProcessScene();
    scene.prepare(defaultInput(), true);
    const process = new RawConstructionProcess(scene.pool, scene.types, scene.state, scene.hooks());
    const notice = makeConstructionProcessNoticeHooks(scene.pool, process, {});
    notice.start(scene.parentSid, 0);
    assert.ok(scene.eventsText() === `A:56;I:20000;J:30:20000:0:16;S:jimBuild.wav;A:40;P:20000:${SOUND}:0:1;R` && scene.fields.end === 12);
    const other = new SidPool(CAPACITY, { server: false });
    assert.ok(throws(() => makeConstructionProcessNoticeHooks(other, process, {})));
});

test('ConstructionProcess: 실제 host 에 붙은 프로세스가 Kernel 프레임마다 HP 를 올리고 다 차면 스스로 끝나며 abstract 를 푼다', () => {
    const scene = new RealScene();
    const process = new RawConstructionProcess(scene.pool, scene.types, scene.state, scene.hooks(), scene.host);
    process.start(scene.parent, 8);
    assert.ok(scene.eventsText() === 'E;P:77:1;R' && scene.kernel.size === 1 && get(scene.pool.slot(scene.parent), 6, 2) !== 0);
    scene.events = [];
    scene.state.now = 4;
    scene.kernel.runFrame();
    assert.ok(scene.hp() === 75 && scene.eventsText() === 'D' && scene.kernel.size === 1 && (scene.pool.slot(scene.parent)[40] & 1));
    scene.events = [];
    scene.state.now = 10;
    scene.kernel.runFrame();
    // 진행의 표시 → Kill → form 삭제 통지의 완료 처리 순서이다.
    assert.equal(scene.eventsText(), 'D;X:47;R;W:buildDone.wav;G:85;U;Q:9;B:100:1');
    assert.ok(scene.kernel.size === 0 && scene.hp() === 300 && !(scene.pool.slot(scene.parent)[40] & 1) && !(scene.pool.slot(scene.parent)[11] & 4));
    assert.equal(get(scene.pool.slot(scene.parent), 6, 2), 0);
});

test('ConstructionProcess: abstract 가 아닌 부모는 첫 Kill 에서 프로세스가 사라지므로 HP 가 최대여도 둘째 Kill 을 하지 않는다', () => {
    const scene = new RealScene();
    const process = new RawConstructionProcess(scene.pool, scene.types, scene.state, scene.hooks(), scene.host);
    process.start(scene.parent, 8);
    scene.pool.allocated(scene.parent)[40] = 0;
    scene.events = [];
    scene.state.now = 10;
    scene.kernel.runFrame();
    // 완료 처리는 한 번만 실행되며 abstract 가 아니므로 Unpop/재등록은 없다.
    assert.ok(scene.eventsText() === 'D;X:47;R;W:buildDone.wav;G:85' && scene.kernel.size === 0 && scene.hp() === 300);
});

test('ConstructionProcess: 권한이 없으면 HP 가 최대여도 Kill 을 요청하지 않아 프로세스가 남고 quickBuild 는 시각과 무관하게 한 프레임에 끝낸다', () => {
    const scene = new RealScene();
    const process = new RawConstructionProcess(scene.pool, scene.types, scene.state, scene.hooks(), scene.host);
    process.start(scene.parent, 8);
    scene.state.boss = false;
    scene.clock.boss = false;
    scene.state.now = 10;
    scene.events = [];
    scene.kernel.runFrame();
    assert.ok(scene.eventsText() === 'D' && scene.kernel.size === 1 && scene.hp() === 300);
    scene.state.boss = true;
    scene.clock.boss = true;
    scene.state.now = 2;
    scene.state.quickBuild = true;
    scene.events = [];
    scene.kernel.runFrame();
    assert.ok(scene.kernel.size === 0 && scene.hp() === 300 && !(scene.pool.slot(scene.parent)[40] & 1));
});

test('ConstructionProcess: 패치판은 free/dead 부모에 붙지 않고 남은 효과를 계속하며 다른 풀의 보상 모듈은 거부한다', () => {
    const scene = new RealScene();
    const process = new RawConstructionProcess(scene.pool, scene.types, scene.state, scene.hooks(), scene.host);
    scene.pool.allocated(scene.parent)[11] = 2;
    process.start(scene.parent, 8);
    assert.ok(scene.eventsText() === 'E;P:77:1;R' && scene.kernel.size === 0);
    const other = new RealScene();
    assert.ok(throws(() => makeConstructionProcessHooks(scene.pool, other.reward, null, {})));
});

test('ConstructionProcess: 실제 Kernel 의 진행과 완료가 서로 다른 시점의 빌린 boss 를 각각 다시 읽는다', () => {
    const scene = new RealScene();
    let boss = false;
    let flipOnRefresh = false;
    scene.state.sharedBoss = () => boss;
    const hooks = scene.hooks();
    const refresh = hooks.refresh;
    hooks.refresh = () => { refresh(); if (flipOnRefresh) boss = false; };
    const process = new RawConstructionProcess(scene.pool, scene.types, scene.state, hooks, scene.host);
    process.start(scene.parent, 8);
    scene.state.quickBuild = true;
    scene.state.now = 10;
    scene.kernel.runFrame();
    assert.ok(scene.kernel.size === 1 && scene.hp() === 300 && (scene.pool.slot(scene.parent)[40] & 1));
    boss = true;
    scene.state.boss = false;
    flipOnRefresh = true;
    scene.kernel.runFrame();
    assert.ok(scene.kernel.size === 0 && scene.hp() === 300 && (scene.pool.slot(scene.parent)[40] & 1) && !boss);
});

test('ConstructionProcess: 현재 checking/Form/진단 복귀까지 원문 64입력과 비교하고 기존 HP 본문 진입에서 멈춘다', { skip: formParentRows.length === 0 && 'Form 부모 fixture 없음' }, () => {
    let count = 0;
    // 동결 원문 입력과 접두를 전부 비교한다.
    for (const row of formParentRows) {
        assert.equal(row.length, 5);
        const p = row[0].split(',').map(Number);
        assert.equal(p.length, 5);
        let checking = p[0];
        let current = p[1];
        const [, , robust, resume, mutate] = p;
        let events = '';
        let outcome = 'body';
        let parentReads = 0;
        const scene = new RealScene();
        scene.state.checking = () => checking;
        const hooks = scene.hooks();
        const event = (text) => { events += (events ? ';' : '') + text; };
        hooks.formRobust = (form) => { event(`R:${form}:${robust}`); return robust !== 0; };
        hooks.assertion = (condition, source, line) => {
            event(`D:${hex(Buffer.from(condition, 'latin1'))}:${hex(Buffer.from(source, 'latin1'))}:${line}`);
            if (mutate) checking = 0;
            if (!resume) throw new Error('assertion');
        };
        hooks.maxHitPoints = () => { event('B'); throw new Error('body'); };
        const logic = new RawConstructionProcess(scene.pool, scene.types, scene.state, hooks, scene.host);
        const fields = createConstructionProcessFields();
        try {
            logic.run(fields, scene.parent, () => true, () => {
                const form = current;
                event(`F:${form}`);
                if (mutate) { checking = 0; current = 1; }
                return form;
            }, () => { parentReads += 1; return scene.parent; });
        } catch (failure) { outcome = failure.message; }
        assert.deepEqual([outcome, String(checking), String(current), events, parentReads], [row[1], row[2], row[3], row[4], outcome === 'body' ? 1 : 0], `Form 부모 행 ${count}`);
        count += 1;
    }
    assert.equal(count, 64);
});

test('ConstructionProcess: 건설 한 개의 실제 Form/부모 체인과 진단 복귀 뒤 기존 HP 진행만 확인한다', () => {
    const scene = new RealScene();
    let checking = 0x80000000 | 0;
    scene.state.checking = () => checking >>> 0;
    let checked = 0;
    let diagnostics = 0;
    let form = 0;
    const hooks = scene.hooks();
    hooks.formRobust = (current) => {
        checked += 1;
        assert.equal(current, form);
        return checkRawFormLinked(scene.pool, current, (value) => readRawProcessFormParent(scene.pool, value));
    };
    hooks.assertion = (condition, source, line) => {
        assert.ok(condition === 'getForm().isRobust()' && source === '\\Ns\\O\\Construction.cpp' && line === 0x1c4);
        diagnostics += 1;
        checking = 0;
        put(scene.pool.allocated(scene.parent), 6, form, 2);
    };
    const logic = new RawConstructionProcess(scene.pool, scene.types, scene.state, hooks, scene.host);
    logic.start(scene.parent, 8);
    form = get(scene.pool.slot(scene.parent), 6, 2);
    assert.notEqual(form, 0);
    scene.state.now = 4;
    scene.events = [];
    scene.kernel.runFrame();
    assert.ok(checked === 1 && diagnostics === 0 && scene.hp() === 75 && scene.kernel.size === 1);
    put(scene.pool.allocated(scene.parent), 6, 0, 2);
    scene.state.now = 4.5;
    scene.kernel.runFrame();
    assert.ok(checked === 2 && diagnostics === 1 && !checking && get(scene.pool.slot(scene.parent), 6, 2) === form && scene.hp() > 75 && scene.hp() < 300 && scene.kernel.size === 1);
    scene.kernel.clear();
});
