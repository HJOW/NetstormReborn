// 사제 회복 raw-priest-regen.js(RawPriestRegen·PriestRegenState·PriestRegenPatchState·bindPriestRegenSpStore·makePriestRegenHandler)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: priestregen-x86.tsv 10.78(originals) 1513행 — 최대 HP·1/4 모드·HP·권한·회복 허용·소유자·extra·좌표·차단 전역·점유·난수 상태·추적 상태·측정값·사후 HP/소유자·감사 변경 → 반환 float 비트, 호출 순서(V/D/N/U/R/F/O/A/M),
//    패치 추적 상태, 난수 상태, 슬롯 50 전체 raw. CD 10.72·10.37 행은 웹 범위(10.78) 밖이다.
//  - 계약: priest_regen_binding_and_dispatch_guards·current_type_uses_global_numerator_with_same_actor·binds_factory_postpop_process_and_kernel·reads_current_store_after_neutral_effect·uses_shared_startup_tracking_without_session_copy·
//    reads_store_after_measurement_in_original_order·store_binding_and_independent_word_read·shared_diagnostic_writes_at_each_native_condition.
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E69: 사제 회복 이식과 함께 추가)
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
import { SquidFactory } from '../www/core/squid-factory.js';
import { SquidPostPopState } from '../www/core/squid-postpop-state.js';
import { SquidReward, SquidRewardState } from '../www/core/squid-reward.js';
import { typeConstructorAddress } from '../www/core/squid-constructors.js';
import { Kernel } from '../www/core/kernel.js';
import { SquidProcessHost, SquidProcessState } from '../www/core/squid-process.js';
import { PRIEST_TYPE, PRIEST_VTABLE } from '../www/core/raw-priest-owner.js';
import { RawPriestState, PriestHitPointMode } from '../www/core/raw-priest-state.js';
import { PRIEST_REGEN_EVENT, PriestPostPopState, RawPriestPostPop, makePriestPostPopProcessHooks } from '../www/core/raw-priest-post-pop.js';
import { RawPriestRegen, PriestRegenState, PriestRegenPatchState, bindPriestRegenSpStore, makePriestRegenHandler } from '../www/core/raw-priest-regen.js';

// 원본 관측 10.78 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E69)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'priestregen-x86.tsv'))?.filter((row) => row[0] === 'originals') ?? null;

// 원본 관찰 SID, 판본당 독립 행 수, 비사제 가상 표(합성 값)이다. (2026-11-08 WB04-E69)
const SOURCE = 50;
const ROWS = 1513;
const COMMON_VTABLE = 0x11010000;

// 예외가 나는지 본다. (2026-11-08 WB04-E69)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 바이트 배열을 소문자 hex 로 만든다. (2026-11-08 WB04-E69)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 내장 Form 네 생성자 주소만 넣은 188칸 TYPE 입력이다. (2026-11-08 WB04-E69)
const regenTypes = () => Array.from({ length: 188 }, (_, number) => ({ cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, maxHitPoints: 0, ...([1, 2, 3, 6].includes(number) ? { constructorAddress: typeConstructorAddress(number) } : {}) }));

// 같은 풀·상태·처리기를 조립해 입력과 결과를 대조하는 장면이다. (2026-11-08 WB04-E69)
class RegenScene {
    constructor() {
        this.pool = new SidPool(24000, { server: false });
        this.random = new GameRandom();
        this.store = new ScrambledSpStore(this.random, () => 1);
        this.book = new SquidPostPopState();
        this.hpMode = new SquidRewardState();
        this.types = regenTypes();
        this.spots = new Uint8Array(65536);
        this.mode = new PriestHitPointMode();
        this.state = new PriestRegenState();
        this.occupied = false;
        this.auditChange = false;
        this.events = [];
        this.after = '-';
        this.afterOwner = 0;
        this.measurement = 0;
        // 각 관찰이 같은 SID 와 stale 슬롯 내용으로 시작한다.
        for (let id = 5; id <= SOURCE; id += 1) assert.equal(this.pool.allocate(2), id);
    }

    // 외부 호출 순서에 구분자를 붙여 기록한다.
    event(value) {
        this.events.push(value);
    }

    // 기록 대체는 호출 시점의 현재 raw 값을 관찰한다.
    record(marker, sid) {
        const raw = this.pool.slot(sid);
        this.event(`${marker}:${sid}:${this.priest.currentHitPoints(sid)}:${raw[34]}:${get(raw, 14)}:${get(raw, 18)}`);
        if (marker === 'V' && this.after !== '-') {
            const target = this.pool.allocated(sid);
            put(target, 26, parseInt(this.after, 10) >>> 0);
            target[34] = this.afterOwner;
        }
    }

    // 가상 공간 진입 시 HP/좌표를 읽는 U/R 호출 기록이며 외부 raw 변화는 V 에만 공급한다.
    space() {
        const record = (marker, sid, flags) => {
            const raw = this.pool.slot(sid);
            this.event(`${marker}:${sid}:${flags}:${this.priest.currentHitPoints(sid)}:${get(raw, 14)}:${get(raw, 18)}`);
        };
        return { unpop: (sid, flags) => record('U', sid, flags), repop: (sid, flags) => record('R', sid, flags) };
    }

    // 실제 공간/표시가 아직 없는 경계를 독립 실행기의 입력과 같은 방식으로 공급한다.
    hooks() {
        return {
            refresh: (sid) => this.record('V', sid),
            damageable: (sid) => this.record('D', sid),
            shape: (type) => { this.event(`F:${type}`); return 0x12345678; },
            occupied: (sid, shape) => { this.event(`O:${sid}:${shape}:0:0`); return this.occupied; },
            neutralAction: (sid) => this.record('N', sid),
            patchAudit: () => {
                this.event('A');
                if (this.auditChange) { this.state.patch.observedSequence = this.state.patch.sequence; this.state.patch.remaining = 1; }
            },
            measurement: () => { this.event('M'); return this.measurement; }
        };
    }

    // 현재 타입 표로 HP/사제 상태 처리기를 다시 만든다. (2026-11-08 WB04-E69)
    remake() {
        this.hp = new SquidReward(this.pool, this.types, this.book, this.hpMode, this.store);
        this.priest = new RawPriestState(this.pool, this.hp, this.types, this.spots);
    }

    // fixture 는 입력만 공급한다. HP 계산/분기 기대값은 준비 코드에 없다.
    prepare(row) {
        assert.equal(row.length, 31);
        this.events = [];
        const type = this.types[PRIEST_TYPE];
        type.maxHitPoints = parseInt(row[1], 10) | 0;
        type.flags1 = 0x69012;
        type.flags2 = 0x210000;
        this.hpMode.priestQuarterHp = Number(row[2]) !== 0;
        this.remake();
        const raw = this.pool.allocated(SOURCE);
        raw.fill(0xab);
        put(raw, 0, PRIEST_VTABLE);
        raw[10] = PRIEST_TYPE;
        raw[11] = 0;
        put(raw, 26, Number(row[3]));
        raw[34] = Number(row[6]);
        raw[40] = Number(row[7]);
        put(raw, 14, Number(row[8]));
        put(raw, 18, Number(row[9]));
        this.mode.authority = Number(row[4]) !== 0;
        this.mode.recoveryAllowed.fill(false);
        this.mode.recoveryAllowed[Number(row[6])] = Number(row[5]) !== 0;
        this.state.neutralBlocked = Number(row[10]) !== 0;
        this.state.modeBlocked = Number(row[11]) !== 0;
        this.occupied = Number(row[12]) !== 0;
        this.random.setState(Number(row[15]));
        const patch = new PriestRegenPatchState();
        patch.sequence = parseInt(row[16], 10);
        patch.observedSequence = parseInt(row[17], 10) | 0;
        patch.changedSample = Number(row[18]);
        patch.remaining = Number(row[19]);
        patch.gate = Number(row[20]);
        patch.sentinel = (parseInt(row[21], 10) << 16) >> 16;
        this.state.patch = patch;
        this.measurement = parseFloat(row[22]);
        this.after = row[23];
        this.afterOwner = Number(row[24]);
        this.auditChange = Number(row[25]) !== 0;
        this.regen = new RawPriestRegen(this.pool, this.hp, this.priest, this.mode, this.space(), this.random, this.state, this.hooks());
    }

    // 추적 sequence 는 signed 판단 뒤 raw DWORD 의 출력 비트로 비교한다.
    tracking() {
        const t = this.state.patch;
        return `${t.sample},${t.sequence >>> 0},${t.observedSequence >>> 0},${t.changedSample},${t.remaining},${t.gate},${t.sentinel}`;
    }

    // 이벤트 기록을 원본 형식 문자열로 만든다. (2026-11-08 WB04-E69)
    text() {
        return this.events.length === 0 ? '-' : this.events.join(';');
    }
}

test('PriestRegen: 원본 10.78 1513행의 반환 float 비트·호출 순서·패치 추적·난수 상태·슬롯 50 raw 가 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    const scene = new RegenScene();
    let count = 0;
    // 입력마다 원본 관찰과 같은 독립 초기 상태를 만든다.
    for (const row of rows) {
        scene.prepare(row);
        const result = scene.regen.handle(SOURCE, PRIEST_REGEN_EVENT, Number(row[13]), bitsToFloat(Number(row[14])));
        assert.equal(floatToBits(result), Number(row[26]), `행 ${count}`);
        assert.equal(scene.text(), row[27], `행 ${count}`);
        assert.equal(scene.tracking(), row[28], `행 ${count}`);
        assert.equal(scene.random.state, Number(row[29]), `행 ${count}`);
        assert.equal(hex(scene.pool.slot(SOURCE)), row[30], `행 ${count}`);
        count += 1;
    }
    assert.equal(count, ROWS);
    assert.equal(scene.store.initialized, false);
});

test('PriestRegen: 빠진 외부 효과/풀/미지원 사건과 잘못된 vtable 은 회복 부작용 전에 거부한다', { skip: !rows && '원본 fixture 없음' }, () => {
    const scene = new RegenScene();
    scene.prepare(rows[0]);
    const hooks = scene.hooks();
    delete hooks.patchAudit;
    assert.ok(throws(() => new RawPriestRegen(scene.pool, scene.hp, scene.priest, scene.mode, scene.space(), scene.random, scene.state, hooks)));
    assert.ok(throws(() => makePriestRegenHandler(new SidPool(24000, { server: false }), scene.regen)));
    const before = hex(scene.pool.slot(SOURCE));
    const rng = scene.random.state;
    const tracking = scene.tracking();
    const handler = makePriestRegenHandler(scene.pool, scene.regen);
    assert.ok(throws(() => handler(SOURCE, 0x25b, 0, 1)));
    assert.ok(hex(scene.pool.slot(SOURCE)) === before && scene.random.state === rng && scene.tracking() === tracking && scene.events.length === 0);
    const fallback = makePriestRegenHandler(scene.pool, scene.regen, () => 3.0);
    assert.equal(fallback(SOURCE, 0x25b, 0, 1), 3.0);
    put(scene.pool.allocated(SOURCE), 0, COMMON_VTABLE);
    const wrong = hex(scene.pool.slot(SOURCE));
    assert.ok(throws(() => scene.regen.handle(SOURCE, PRIEST_REGEN_EVENT, 0, 1)));
    assert.ok(hex(scene.pool.slot(SOURCE)) === wrong && scene.random.state === rng);
    assert.equal(handler(SOURCE, 0x25b, 0, 2), 2);
});

test('PriestRegen: 실제 회복 처리기는 Actor 158 을 유지하며 증분의 현재 TYPE 만 74/75 로 바꾼다', { skip: !rows && '원본 fixture 없음' }, () => {
    const scene = new RegenScene();
    scene.prepare(rows[0]);
    scene.types[158].maxHitPoints = 200;
    scene.types[74].maxHitPoints = 60;
    scene.types[74].flags1 = 16;
    scene.types[75].maxHitPoints = 120;
    scene.types[75].flags1 = 16;
    scene.hpMode.priestQuarterHp = false;
    scene.remake();
    const raw = scene.pool.allocated(SOURCE);
    put(raw, 26, 20);
    raw[34] = 1;
    raw[40] = 0;
    scene.mode.authority = true;
    scene.mode.recoveryAllowed.fill(true);
    scene.state.neutralBlocked = true;
    scene.state.patch.remaining = 0;
    scene.after = '-';
    scene.auditChange = false;
    let currentType = 74;
    scene.state.sharedPriestType = () => currentType;
    const regen = new RawPriestRegen(scene.pool, scene.hp, scene.priest, scene.mode, scene.space(), scene.random, scene.state, scene.hooks());
    regen.handle(SOURCE, PRIEST_REGEN_EVENT, 0, 8.25);
    assert.ok(scene.priest.currentHitPoints(SOURCE) === 30 && raw[10] === 158);
    currentType = 75;
    regen.handle(SOURCE, PRIEST_REGEN_EVENT, 0, 8.25);
    assert.ok(scene.priest.currentHitPoints(SOURCE) === 50 && raw[10] === 158);
});

test('PriestRegen: 실제 생성자와 Regular/Kernel 을 연결해 HP 회복과 절반 경계의 재등록 호출을 확인한다', () => {
    // server/client 생성자를 각각 확인한다.
    for (const server of [false, true]) {
        const pool = new SidPool(32768, { server });
        const types = regenTypes();
        const type = types[PRIEST_TYPE];
        type.constructorAddress = typeConstructorAddress(PRIEST_TYPE);
        type.flags1 = 0x69012;
        type.flags2 = 0x210000;
        type.maxHitPoints = 200;
        const factory = new SquidFactory(pool, types);
        const sid = factory.create(PRIEST_TYPE, server ? 0 : 2);
        const raw = pool.allocated(sid);
        raw[11] = 0;
        raw[34] = 1;
        put(raw, 26, 80);
        put(raw, 14, floatToBits(20.75));
        put(raw, 18, floatToBits(21.9));
        const random = new GameRandom();
        const store = new ScrambledSpStore(random, () => 1);
        const hp = new SquidReward(pool, types, new SquidPostPopState(), new SquidRewardState(), store);
        const spots = new Uint8Array(65536).fill(6);
        const priest = new RawPriestState(pool, hp, types, spots);
        const mode = new PriestHitPointMode();
        mode.recoveryAllowed.fill(true);
        const state = new PriestRegenState();
        const counts = { unpops: 0, repops: 0, refreshes: 0, damageables: 0, audits: 0 };
        const regen = new RawPriestRegen(pool, hp, priest, mode, {
            unpop: (s, flags) => { assert.ok(s === sid && flags === 0); counts.unpops += 1; },
            repop: (s, flags) => { assert.ok(s === sid && flags === 0); counts.repops += 1; }
        }, random, state, {
            refresh: (s) => { assert.equal(s, sid); counts.refreshes += 1; },
            damageable: (s) => { assert.equal(s, sid); counts.damageables += 1; },
            shape: () => 1,
            occupied: () => false,
            neutralAction: () => {},
            patchAudit: () => { counts.audits += 1; },
            measurement: () => 0
        });
        const destroy = new RawSquidDestroy(pool, new SquidUnpop(pool, new SquidHash(), spots), types);
        const kernel = new Kernel(16);
        const processState = new SquidProcessState();
        processState.now = 11.5;
        const host = new SquidProcessHost(pool, types, kernel, destroy, processState, { regular: makePriestRegenHandler(pool, regen) });
        const list = new PriestPostPopState();
        list.priests.entries = [0, 0];
        const prefix = new RawPriestPostPop(pool, hp, list, makePriestPostPopProcessHooks(host));
        const free = pool.freeCount;
        prefix.prefix(sid, 1);
        const regular = host.findEvent(sid, PRIEST_REGEN_EVENT);
        assert.ok(regular && regular.payload === 8.25 && regular.time === 11.5 && kernel.size === 1);
        kernel.runFrame();
        assert.ok(priest.currentHitPoints(sid) === 113 && regular.count === 1 && regular.time === 19.75);
        assert.ok(counts.unpops === 1 && counts.repops === 1 && counts.refreshes === 1 && counts.damageables === 1);
        kernel.runFrame();
        assert.equal(regular.count, 1);
        // 원본 예약 시각에 맞춰 최대 HP 까지 진행하고 가득 찬 뒤에도 예약이 계속되는지 확인한다.
        for (let tick = 0; tick < 4; tick += 1) {
            processState.now = regular.time;
            kernel.runFrame();
        }
        assert.ok(priest.currentHitPoints(sid) === 200 && regular.count === 5 && regular.payload === 8.25);
        assert.ok(counts.refreshes === 4 && counts.damageables === 4 && counts.unpops === 1 && counts.repops === 1 && counts.audits === 5);
        host.kill(regular, 0x10);
        assert.ok(kernel.size === 0 && pool.freeCount === free && !store.initialized);
    }
});

// 회복 추적 시험이 쓰는 변형 행이다(원본 첫 입력에서 지정한 칸만 바꾼다). (2026-11-08 WB04-E69)
function variant(base, changes) {
    const row = [...base];
    for (const [index, value] of Object.entries(changes)) row[Number(index)] = value;
    return row;
}

test('PriestRegen: 같은 SP 몸체가 중립 효과에서 다시 회전하면 audit 직후 사본 대신 최종 현재 offset 을 읽는다', { skip: !rows && '원본 fixture 없음' }, () => {
    const scene = new RegenScene();
    scene.prepare(variant(rows[0], { 1: '200', 2: '0', 3: '200', 4: '1', 5: '1', 6: '0', 10: '0', 11: '0', 12: '0', 18: '19', 19: '0', 20: '0', 21: '-1', 23: '-' }));
    scene.store.bindTrackingOffset(scene.state.patch.tracking);
    scene.store.initialize();
    scene.store.set(1, floatToBits(1234.5));
    const hooks = scene.hooks();
    let rotateNeutral = false;
    hooks.patchAudit = () => { scene.store.rotate(); };
    hooks.neutralAction = () => { if (rotateNeutral) scene.store.rotate(); };
    hooks.spReads = bindPriestRegenSpStore(scene.store, scene.state.patch);
    const regen = new RawPriestRegen(scene.pool, scene.hp, scene.priest, scene.mode, scene.space(), scene.random, scene.state, hooks);
    regen.handle(SOURCE, PRIEST_REGEN_EVENT, 0, 8.25);
    assert.ok(scene.state.patch.changedSample === 19 && scene.store.offset === (scene.state.patch.sequence >>> 0));
    rotateNeutral = true;
    regen.handle(SOURCE, PRIEST_REGEN_EVENT, 0, 8.25);
    assert.notEqual(scene.store.offset, scene.state.patch.sequence >>> 0);
    assert.equal(scene.state.patch.changedSample, scene.state.patch.sample);
    assert.equal(scene.store.get(1), floatToBits(1234.5));
    assert.ok(scene.store.trackingOwner === scene.state.patch.tracking);
});

test('PriestRegen: 공통 시작의 같은 추적 DWORD 를 직접 갱신하고 Session 지역 사본을 쓰지 않는다', { skip: !rows && '원본 fixture 없음' }, () => {
    const scene = new RegenScene();
    scene.prepare(variant(rows[0], { 1: '200', 2: '0', 3: '200', 4: '1', 5: '1', 6: '0', 10: '0', 11: '0', 12: '0', 18: '19', 19: '0', 20: '0', 21: '-1', 23: '-' }));
    const sharedTracking = { value: 0 };
    scene.store.bindTrackingOffset(sharedTracking);
    scene.store.initialize();
    scene.state.patch.sequence = -300;
    scene.store.set(1, floatToBits(1234.5));
    const hooks = scene.hooks();
    hooks.patchAudit = () => { scene.store.rotate(); };
    hooks.spReads = bindPriestRegenSpStore(scene.store);
    const regen = new RawPriestRegen(scene.pool, scene.hp, scene.priest, scene.mode, scene.space(), scene.random, scene.state, hooks);
    const previous = sharedTracking.value;
    regen.handle(SOURCE, PRIEST_REGEN_EVENT, 0, 8.25);
    assert.ok(sharedTracking.value === previous + 3 && scene.store.offset === (sharedTracking.value >>> 0) && scene.state.patch.sequence === -300 && scene.state.patch.changedSample === 19);
    assert.equal(scene.store.get(1), floatToBits(1234.5));
});

test('PriestRegen: 원본의 observed→측정→첫 포인터→signed WORD 읽기 순서와 측정 안의 lazy 초기화를 확인한다', { skip: !rows && '원본 fixture 없음' }, () => {
    const scene = new RegenScene();
    scene.prepare(variant(rows[0], { 1: '200', 2: '0', 3: '200', 4: '0', 16: '-3', 18: '19', 19: '1', 20: '0', 21: '-1' }));
    scene.store.bindTrackingOffset(scene.state.patch.tracking);
    const hooks = scene.hooks();
    let order = '';
    const reads = bindPriestRegenSpStore(scene.store, scene.state.patch);
    // 독립 경계 입력으로 초기화 전 상태를 측정까지 유지한다.
    hooks.patchAudit = () => {};
    hooks.measurement = () => { order += 'M'; scene.store.initialize(); return 7000.0; };
    hooks.spReads = {
        observedSequence: () => { order += 'O'; return reads.observedSequence(); },
        firstPool: () => { order += 'P'; return reads.firstPool(); },
        firstPermutation: () => { order += 'S'; return reads.firstPermutation(); }
    };
    const regen = new RawPriestRegen(scene.pool, scene.hp, scene.priest, scene.mode, scene.space(), scene.random, scene.state, hooks);
    assert.equal(scene.store.initialized, false);
    regen.handle(SOURCE, PRIEST_REGEN_EVENT, 0, 8.25);
    assert.ok(order === 'OMPS' && scene.store.initialized && scene.state.patch.remaining === 0);
    assert.equal(scene.state.patch.changedSample, 19);
});

test('PriestRegen: NULL 배열이어도 WORD 를 읽고 미연결/다른 추적 주소/부분 getter 는 부작용 전에 거부한다', { skip: !rows && '원본 fixture 없음' }, () => {
    const scene = new RegenScene();
    scene.prepare(rows[0]);
    assert.ok(throws(() => bindPriestRegenSpStore(scene.store, scene.state.patch)));
    const other = new PriestRegenPatchState();
    scene.store.bindTrackingOffset(other.tracking);
    assert.ok(throws(() => bindPriestRegenSpStore(scene.store, scene.state.patch)));
    const hooks = scene.hooks();
    hooks.spReads = {};
    assert.ok(throws(() => new RawPriestRegen(scene.pool, scene.hp, scene.priest, scene.mode, scene.space(), scene.random, scene.state, hooks)));
    let order = '';
    scene.state.patch.remaining = 0;
    scene.mode.authority = false;
    hooks.patchAudit = () => {};
    hooks.spReads = {
        observedSequence: () => { order += 'O'; return scene.state.patch.sequence; },
        firstPool: () => { order += 'P'; return null; },
        firstPermutation: () => { order += 'S'; return -1; }
    };
    const regen = new RawPriestRegen(scene.pool, scene.hp, scene.priest, scene.mode, scene.space(), scene.random, scene.state, hooks);
    regen.handle(SOURCE, PRIEST_REGEN_EVENT, 0, 8.25);
    assert.equal(order, 'OPS');
    assert.equal(scene.state.patch.changedSample, scene.state.patch.sample);
});

test('PriestRegen: 네 추적 조건은 지역 fixture 사본을 건드리지 않고 같은 장기 진단 주소에 쓰며 정상 입력은 보존한다', { skip: !rows && '원본 fixture 없음' }, () => {
    // 불일치·측정·NULL 배열·signed WORD 와 조건 없는 입력을 각각 분리한다.
    for (let condition = 0; condition < 5; condition += 1) {
        const scene = new RegenScene();
        scene.prepare(variant(rows[0], { 1: '200', 3: '200', 4: '0', 16: '0', 17: '3', 18: '12345', 19: '0', 20: '1', 21: '0' }));
        const diagnostic = { value: 0x13579bdf };
        scene.state.patch.sharedChangedSample = diagnostic;
        const hooks = scene.hooks();
        // 추적 관문만 독립 입력으로 공급한다.
        hooks.patchAudit = () => {};
        if (condition === 0) scene.state.patch.observedSequence = 4;
        if (condition === 1) { scene.state.patch.remaining = 1; hooks.measurement = () => 7001.0; }
        if (condition === 2) scene.state.patch.gate = 0;
        if (condition === 3) scene.state.patch.sentinel = -1;
        const regen = new RawPriestRegen(scene.pool, scene.hp, scene.priest, scene.mode, scene.space(), scene.random, scene.state, hooks);
        regen.handle(SOURCE, PRIEST_REGEN_EVENT, 0, 8.25);
        assert.equal(scene.state.patch.changedSample, 12345);
        assert.equal(diagnostic.value, condition === 4 ? 0x13579bdf : scene.state.patch.sample);
        assert.ok(scene.state.patch.sample <= 8);
    }
});
