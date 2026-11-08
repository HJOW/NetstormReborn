// 사제 낙하 raw-priest-fall.js(RawPriestFall·PriestFallState·makePriestFallHandler/Walker/Hooks/FrameHooks)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: priestfall-x86.tsv 10.78(originals) 456행 — 낙하 요청(004941f0: 보호막 확보→예약→공유 Regular→Unpop/Repop→소리)과 0x25b 이벤트(00494580→00493e60: 이동 불가 보호막→프레임
//    진행/낙하 프레임→지면 판정→도착 프레임→보호막 해제)의 호출 기록·슬롯 50바이트·반환 비트·현재 권한. CD 10.72·10.37 행은 웹 범위(10.78) 밖이다.
//  - 계약: PriestFall_ComposesSharedProcessAndShieldLifetime(실제 공유 Form/Kernel 재예약·삭제와 실제 보호막 생성/소유자/조회·해제)·
//    PriestFall_SharedAuthorityRequeriesAfterActualProcessEffect(권한 재조회)·분배기 fallback/미복원 사건 거부/잘못된 풀·미연결 훅 거부.
//    cpppj 의 회복 분배기 합성(RawPriestRegen)은 아직 옮기지 않아 같은 fallback 연결 계약만 확인한다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E65: 사제 낙하 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { GameRandom } from '../www/core/random.js';
import { ScrambledSpStore } from '../www/core/sp-store.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { SquidUnpop } from '../www/core/squid-unpop.js';
import { RawSquidDestroy } from '../www/core/squid-destroy.js';
import { SquidFactory } from '../www/core/squid-factory.js';
import { SquidOwner, SquidOwnerMode } from '../www/core/squid-owner.js';
import { SquidPostPopState } from '../www/core/squid-postpop-state.js';
import { SquidReward, SquidRewardState } from '../www/core/squid-reward.js';
import { typeConstructorAddress } from '../www/core/squid-constructors.js';
import { Kernel } from '../www/core/kernel.js';
import { SquidProcessHost, SquidProcessState } from '../www/core/squid-process.js';
import { FrameCodeTable } from '../www/assets/terrain-tiles.js';
import { PRIEST_TYPE, PRIEST_VTABLE } from '../www/core/raw-priest-owner.js';
import { RawPriestState } from '../www/core/raw-priest-state.js';
import { RawPriestForcefield, PriestForcefieldState } from '../www/core/raw-priest-forcefield.js';
import { RawPriestShield, PriestShieldState, makePriestShieldCreationHooks } from '../www/core/raw-priest-shield.js';
import { RawPriestFall, PriestFallState, PRIEST_FALL_EVENT, makePriestFallHandler, makePriestFallWalker, makePriestFallHooks, makePriestFallFrameHooks } from '../www/core/raw-priest-fall.js';
import { SquidFrame } from '../www/core/squid-frame.js';
import { RawFrameAdvance } from '../www/core/raw-frame-advance.js';

// 원본 관측 10.78 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E65)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'priestfall-x86.tsv'))?.filter((row) => row[0] === 'originals') ?? null;

// 원본 관찰 SID, 판본당 독립 행 수, 0x25b 호출에 주는 개수/payload(NaN 비트 보존 확인), 공유 Regular 형식 번호이다. (2026-11-08 WB04-E65)
const SOURCE = 50;
const ROWS = 456;
const SHARED_REGULAR_TYPE = 61;

// 예외가 나는지 본다. (2026-11-08 WB04-E65)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 바이트 배열을 소문자 hex 로 만든다. (2026-11-08 WB04-E65)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 단정도 float 비트를 수로 바꾼다. (2026-11-08 WB04-E65)
const bitsToFloat = (bits) => {
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, bits >>> 0, true);
    return view.getFloat32(0, true);
};

// 내장 Form 네 생성자 주소만 넣은 188칸 TYPE 입력이다(cpppj MakeFormTypeInputs). (2026-11-08 WB04-E65)
const formTypeInputs = () => Array.from({ length: 188 }, (_, number) => ({ cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, maxHitPoints: 0, ...([1, 2, 3, 6].includes(number) ? { constructorAddress: typeConstructorAddress(number) } : {}) }));

// 동일 월드의 raw 풀·상태·프레임·지도와 명시 외부 효과 입력을 소유하는 장면이다. (2026-11-08 WB04-E65)
class FallScene {
    constructor() {
        this.pool = new SidPool(24000, { server: false });
        // 원본 번호 정책에 따라 예약 슬롯부터 SID 50 까지 실제 풀로 확보한다.
        for (let id = 5; id <= SOURCE; id += 1) assert.equal(this.pool.allocate(2), id);
        this.store = new ScrambledSpStore(new GameRandom(), () => 1);
        this.book = new SquidPostPopState();
        this.hpMode = new SquidRewardState();
        this.types = formTypeInputs();
        this.frames = this.types.map(() => new FrameCodeTable([]));
        this.choices = this.types.map(() => ({ falling: 0, landed: 0 }));
        this.spots = new Uint8Array(65536);
        this.surfaces = new Uint16Array(65536);
        this.mode = new PriestFallState();
        this.events = [];
        this.begin = false;
        this.allocated = false;
        this.change = 0;
    }

    // 입력 한 행을 복구한다. 기대값이나 분기 결과는 계산하지 않는다.
    prepare(row) {
        assert.equal(row.length, 16);
        this.events = [];
        this.begin = row[1] === 'Begin';
        this.mode.authority = Number(row[2]) !== 0;
        this.allocated = Number(row[3]) !== 0;
        this.change = Number(row[9]);
        this.types[PRIEST_TYPE].maxHitPoints = 200;
        this.types[PRIEST_TYPE].flags1 = 0x69012;
        this.types[PRIEST_TYPE].flags2 = 0x210000;
        this.hp = new SquidReward(this.pool, this.types, this.book, this.hpMode, this.store);
        this.frames[PRIEST_TYPE] = new FrameCodeTable([{ side: Number(row[7]), variant: 80, flags: 0 }, { side: 74, variant: 80, flags: 0 }]);
        this.choices[PRIEST_TYPE] = { falling: 3, landed: 4 };
        this.spots.fill(Number(row[6]));
        this.surfaces.fill(Number(row[8]));
        const raw = this.pool.allocated(SOURCE);
        raw.fill(0);
        put(raw, 0, PRIEST_VTABLE);
        raw[10] = PRIEST_TYPE;
        raw[34] = 1;
        raw[40] = Number(row[4]);
        put(raw, 26, Number(row[5]));
        put(raw, 14, Number(row[10]));
        put(raw, 18, Number(row[11]));
    }

    // 원본 실행기에 공급한 외부 효과의 입력 변이만 수행한다.
    mutate(stage) {
        const raw = this.pool.allocated(SOURCE);
        if (this.begin) {
            if ((stage === 'shield' && this.change === 1) || (stage === 'new' && this.change === 2)) this.mode.authority = !this.mode.authority;
            if (stage === 'repop' && this.change === 3) {
                put(raw, 14, floatToBits(30.75));
                put(raw, 18, floatToBits(31.9));
            }
            return;
        }
        if (stage === 'shield' && this.change === 1) put(raw, 36, 1);
        if ((stage === 'set' || stage === 'advance') && (this.change === 2 || this.change === 4)) {
            put(raw, 14, floatToBits(30.75));
            put(raw, 18, floatToBits(31.9));
        }
        if (stage === 'land' && (this.change === 3 || this.change === 4)) {
            raw[40] = 0;
            put(raw, 26, 150);
        }
    }

    // 원본의 명시 경계 인자를 기록한다. 실제 낙하 조건/반환은 생산 코드가 결정한다.
    hooks() {
        return {
            ensureShield: (sid) => { this.events.push(`S:${sid}`); this.mutate('shield'); },
            reserveRegular: () => { this.events.push('A:40'); this.mutate('new'); return this.allocated; },
            addSharedRegular: (sid, event, payload) => { this.events.push(`R:${sid}:${event}:${floatToBits(payload)}`); },
            unpop: (sid, flags) => { this.events.push(`U:${sid}:${flags}`); },
            repop: (sid, flags) => { this.events.push(`P:${sid}:${flags}`); this.mutate('repop'); },
            sound: (sid, x, y) => { this.events.push(`W:${sid}:${floatToBits(x)}:${floatToBits(y)}`); },
            setFrame: (sid, frame, flags) => { this.events.push(`F:${sid}:${frame}:${flags}`); this.mutate(frame === 4 ? 'land' : 'set'); },
            advanceFrame: (sid, steps, flags) => { this.events.push(`N:${sid}:${steps}:${flags}`); this.mutate('advance'); },
            clearShield: (sid) => { this.events.push(`X:${sid}`); }
        };
    }

    // 호출 순서를 원본 fixture 와 같은 문자열로 합친다.
    text() {
        return this.events.length === 0 ? '-' : this.events.join(';');
    }

    // 현재 장면의 사제 상태 조회기이다.
    priest() {
        return new RawPriestState(this.pool, this.hp, this.types, this.spots);
    }
}

test('PriestFall: 원본 10.78 456행의 호출 기록·슬롯 50바이트·반환 비트·현재 권한이 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    const scene = new FallScene();
    let count = 0;
    // 기대값을 계산하지 않고 입력별 생산 모듈을 실행한다.
    for (const row of rows) {
        scene.prepare(row);
        const fall = new RawPriestFall(scene.pool, scene.priest(), scene.frames, scene.choices, scene.surfaces, scene.mode, scene.hooks());
        let result = '-';
        if (scene.begin) fall.begin(SOURCE);
        else result = String(floatToBits(fall.handle(SOURCE, PRIEST_FALL_EVENT, 0xabcdef01, bitsToFloat(0x7fc12345))));
        assert.equal(scene.text(), row[12], `행 ${count}`);
        assert.equal(hex(scene.pool.slot(SOURCE)), row[13], `행 ${count}`);
        assert.equal(result, row[14], `행 ${count}`);
        assert.equal(scene.mode.authority, Number(row[15]) !== 0, `행 ${count}`);
        count += 1;
    }
    assert.equal(count, ROWS);
    assert.equal(scene.store.initialized, false);
});

test('PriestFall: 실제 공유 Form/Kernel 재예약·삭제와 실제 보호막 생성/조회/해제를 합성한다', () => {
    const scene = new FallScene();
    scene.prepare(rows ? rows[0] : ['originals', 'Begin', '0', '0', '32', '20', '0', '65', '0', '0', String(floatToBits(20.75)), String(floatToBits(21.9)), '', '', '-', '0']);
    scene.begin = false;
    scene.change = 0;
    scene.types[167].footX = 1;
    scene.types[167].footY = 1;
    scene.spots.fill(6);
    const raw = scene.pool.allocated(SOURCE);
    put(raw, 14, floatToBits(20.75));
    put(raw, 18, floatToBits(21.9));
    const hash = new SquidHash();
    const unpop = new SquidUnpop(scene.pool, hash, scene.spots);
    const destroy = new RawSquidDestroy(scene.pool, unpop, scene.types);
    const kernel = new Kernel(16);
    const processMode = new SquidProcessState();
    processMode.now = 12.5;
    let fall = null;
    const host = new SquidProcessHost(scene.pool, scene.types, kernel, destroy, processMode, {
        regular: (sid, event, count, payload) => makePriestFallHandler(scene.pool, fall)(sid, event, count, payload)
    });
    const factory = new SquidFactory(scene.pool, scene.types);
    const owner = new SquidOwner(scene.pool, scene.types, scene.book, new SquidOwnerMode());
    const lookup = new RawPriestForcefield(scene.pool, hash, scene.types, new PriestForcefieldState());
    const shieldMode = new PriestShieldState({ localPlayer: 1, loadingDepth: 1, noticeClock: 0 });
    let shieldSid = 0;
    let creates = 0;
    let clears = 0;
    const effects = makePriestShieldCreationHooks(scene.pool, factory, owner, {
        pop: (sid, x, y, flags) => {
            assert.equal(flags, 0);
            shieldSid = sid;
            creates += 1;
            const born = scene.pool.allocated(sid);
            // 보호막 Pop 경계가 실제 finder 입력을 등록한다.
            put(born, 14, floatToBits(x));
            put(born, 18, floatToBits(y));
            born[11] = 0;
            put(born, 4, hash.bucket(0, x, y), 2);
            hash.setBucket(0, x, y, sid);
        },
        reserveSound: () => false,
        loadSound: () => 0,
        attachSound: () => {},
        noticeSound: () => {},
        translate: () => '',
        tell: () => {}
    });
    const shield = new RawPriestShield(scene.pool, lookup, shieldMode, effects);
    const hooks = makePriestFallHooks(host, shield, lookup, (sid, flags) => {
        assert.ok(sid === shieldSid && flags === 0);
        clears += 1;
        hash.setBucket(0, 20.75, 21.9, 0);
        scene.pool.allocated(sid)[11] = 4;
        scene.pool.release(sid);
    }, scene.hooks());
    // 도착 때 J 방향을 풀고 현재 HP 를 회복하는 미복원 프레임 효과의 입력이다.
    hooks.setFrame = (sid, frame, flags) => {
        assert.equal(flags, 0);
        const current = scene.pool.allocated(sid);
        put(current, 36, frame === 3 ? 1 : 0);
        if (frame === 4) {
            current[40] = 0;
            put(current, 26, 150);
        }
    };
    fall = new RawPriestFall(scene.pool, scene.priest(), scene.frames, scene.choices, scene.surfaces, scene.mode, hooks);
    fall.begin(SOURCE);
    let process = host.findEvent(SOURCE, PRIEST_FALL_EVENT);
    assert.ok(process && kernel.size === 1 && scene.pool.slot(process.form)[10] === SHARED_REGULAR_TYPE && creates === 1 && lookup.find(SOURCE) === shieldSid);
    assert.ok(process.time === 12.5 && process.count === 0 && process.payload === Math.fround(0.01));
    // 낙하 요청은 기존 이벤트를 검색하지 않는다. 가장 새 공유 form 을 지워도 이전 예약이 남는다.
    processMode.now = 13.0;
    fall.begin(SOURCE);
    const newest = host.findEvent(SOURCE, PRIEST_FALL_EVENT);
    assert.ok(newest && newest !== process && newest.time === 13.0 && kernel.size === 2 && creates === 1);
    host.kill(newest, 0x10);
    assert.ok(kernel.size === 1 && host.findEvent(SOURCE, PRIEST_FALL_EVENT) === process);
    processMode.now = 12.5;
    kernel.runFrame();
    process = host.findEventMasked(SOURCE, 0xffff, PRIEST_FALL_EVENT);
    assert.ok(process && process.count === 1 && process.payload === Math.fround(0.1) && process.time === 12.5 + Math.fround(0.1));
    processMode.now = process.time - 0.000001;
    kernel.runFrame();
    assert.ok(process.count === 1 && creates === 1);
    processMode.now = process.time;
    scene.surfaces.fill(65535);
    kernel.runFrame();
    assert.ok(kernel.size === 0 && host.findEvent(SOURCE, PRIEST_FALL_EVENT) === null && clears === 1 && lookup.find(SOURCE) === 0);
});

test('PriestFall: 실제 프로세스 예약 효과가 권한을 바꾸면 이후 Unpop/Repop 관문이 변경 후 값을 읽는다', () => {
    const scene = new FallScene();
    const row = Array.from({ length: 16 }, () => '0');
    row[1] = 'Begin';
    row[2] = '1';
    row[3] = '1';
    row[5] = '200';
    row[6] = '4';
    row[7] = '74';
    row[10] = String(floatToBits(30.0));
    row[11] = String(floatToBits(31.0));
    scene.prepare(row);
    let authority = true;
    scene.mode.sharedAuthority = () => authority;
    const hooks = scene.hooks();
    const original = { add: hooks.addSharedRegular, unpop: hooks.unpop, repop: hooks.repop };
    let added = 0;
    let unpops = 0;
    let repops = 0;
    // 원문 자식 호출 뒤 전역 변경만 명시 공급한다.
    hooks.addSharedRegular = (sid, event, payload) => { original.add(sid, event, payload); added += 1; authority = false; };
    hooks.unpop = (sid, flags) => { unpops += 1; original.unpop(sid, flags); };
    hooks.repop = (sid, flags) => { repops += 1; original.repop(sid, flags); };
    const fall = new RawPriestFall(scene.pool, scene.priest(), scene.frames, scene.choices, scene.surfaces, scene.mode, hooks);
    fall.begin(SOURCE);
    assert.ok(added === 1 && !authority && unpops === 0 && repops === 0);
    scene.allocated = false;
    scene.mode.authority = false;
    authority = true;
    fall.begin(SOURCE);
    assert.ok(added === 1 && authority && unpops === 1 && repops === 1);
});

test('PriestFall: 낙하 방향이면 요청을 반복하지 않고 항상 참을 반환하며 분배기는 미복원 사제 사건을 거부한다', () => {
    const scene = new FallScene();
    const row = Array.from({ length: 16 }, () => '0');
    row[1] = 'Begin';
    row[5] = '20';
    row[7] = '65';
    scene.prepare(row);
    const fall = new RawPriestFall(scene.pool, scene.priest(), scene.frames, scene.choices, scene.surfaces, scene.mode, scene.hooks());
    // 방향 A(프레임 0)는 낙하를 시작하고, J(프레임 1)는 시작하지 않는다.
    assert.equal(fall.tryFall(SOURCE), true);
    assert.ok(scene.events[0] === `S:${SOURCE}`);
    scene.events = [];
    put(scene.pool.allocated(SOURCE), 36, 1);
    assert.equal(fall.tryFall(SOURCE), true);
    assert.deepEqual(scene.events, []);
    // 공통 Regular 분배: 낙하는 0.1(지면 없음), 다른 사건은 명시 fallback, 사제의 미복원 사건은 거부한다.
    scene.surfaces.fill(0);
    const handler = makePriestFallHandler(scene.pool, fall, (sid, event, count, payload) => { assert.equal(sid, SOURCE); return event === 0x25a ? payload : -1; });
    assert.equal(handler(SOURCE, PRIEST_FALL_EVENT, 42, -7.0), Math.fround(0.1));
    assert.equal(handler(SOURCE, 0x25a, 0, 2.0), 2.0);
    const strict = makePriestFallHandler(scene.pool, fall);
    assert.ok(throws(() => strict(SOURCE, 0x25c, 0, 1.0)));
    assert.ok(throws(() => fall.handle(SOURCE, 0x25a, 0, 1.0)));
    // 사제가 아닌 vtable 은 payload 를 그대로 돌려준다.
    put(scene.pool.allocated(SOURCE), 0, 0x11010000);
    assert.equal(strict(SOURCE, 0x25c, 0, 3.0), 3.0);
});

test('PriestFall: 잘못된 풀·표·미연결 훅은 구성 때 거부하고 프레임 훅은 같은 지정기만 받는다', () => {
    const scene = new FallScene();
    const row = Array.from({ length: 16 }, () => '0');
    row[1] = 'Begin';
    row[7] = '65';
    scene.prepare(row);
    const state = scene.priest();
    assert.ok(throws(() => new RawPriestFall(scene.pool, state, [], scene.choices, scene.surfaces, scene.mode, scene.hooks())));
    assert.ok(throws(() => new RawPriestFall(scene.pool, state, scene.frames, scene.choices, scene.surfaces, scene.mode, {})));
    assert.ok(throws(() => new RawPriestFall(scene.pool, state, scene.frames, scene.choices.slice(1), scene.surfaces, scene.mode, scene.hooks())));
    const hooks = scene.hooks();
    delete hooks.clearShield;
    assert.ok(throws(() => new RawPriestFall(scene.pool, state, scene.frames, scene.choices, scene.surfaces, scene.mode, hooks)));
    const fall = new RawPriestFall(scene.pool, state, scene.frames, scene.choices, scene.surfaces, scene.mode, scene.hooks());
    const other = new SidPool(24000, { server: false });
    assert.ok(throws(() => makePriestFallHandler(other, fall)));
    assert.ok(throws(() => makePriestFallWalker(other, fall)));
    // 프레임 훅은 같은 풀의 같은 지정기에서 만든 진행기만 허용한다.
    const frameHooks = { frameSize: () => [1, 1], update: () => {}, unpop: () => {}, pop: () => {} };
    const setter = new SquidFrame(scene.pool, scene.types, frameHooks);
    const advance = new RawFrameAdvance(scene.pool, scene.frames, setter);
    const bound = makePriestFallFrameHooks(setter, advance, scene.hooks());
    assert.ok(typeof bound.setFrame === 'function' && typeof bound.advanceFrame === 'function');
    assert.ok(throws(() => makePriestFallFrameHooks(new SquidFrame(scene.pool, scene.types, frameHooks), advance, scene.hooks())));
    // 실제 사제가 아닌 vtable 의 walker 는 명시 fallback 이 있어야 처리한다.
    const walker = makePriestFallWalker(scene.pool, fall);
    put(scene.pool.allocated(SOURCE), 0, 0x11313000);
    assert.ok(throws(() => walker(SOURCE)));
    const fallback = makePriestFallWalker(scene.pool, fall, (sid) => { assert.equal(sid, SOURCE); return true; });
    assert.equal(fallback(SOURCE), true);
    assert.deepEqual(scene.events, []);
});
