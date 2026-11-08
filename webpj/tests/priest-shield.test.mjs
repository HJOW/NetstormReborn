// 사제 보호막 raw-priest-shield.js(RawPriestShield·PriestShieldState·makePriestShieldCreationHooks)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: priestshield-x86.tsv 10.78(originals) 832행 — 장면 번호·소유자·소리 예약 성공·시각 종류·로딩·로컬 플레이어·변경 단계에서 생성/소유자/Pop/소리/안내 호출 기록과 사제·보호막 슬롯·전역 세 값(시각은 double 비트).
//    조회와 공통 소유자는 실제 모듈이며 자산 생성/Pop·소리/문구 하위만 대체한다. CD 10.72·10.37 행은 웹 범위(10.78) 밖이다.
//  - 계약: PriestShield_CreatesOnceAndCanBeFoundByPriestDeletion(실제 Factory/소유자 연결로 한 번만 생성, 사제 삭제 준비가 보호막을 찾아 삭제 요청)·PriestShield_RejectsMissingEffectsAndForeignPools.
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E63: 사제 보호막 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get, floatToBits, bitsToFloat } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { SquidFactory } from '../www/core/squid-factory.js';
import { SquidOwner, SquidOwnerMode } from '../www/core/squid-owner.js';
import { SquidPostPopState } from '../www/core/squid-postpop-state.js';
import { typeConstructorAddress } from '../www/core/squid-constructors.js';
import { PRIEST_TYPE, PRIEST_VTABLE } from '../www/core/raw-priest-owner.js';
import { PriestPostPopState } from '../www/core/raw-priest-post-pop.js';
import { RawPriestPreDestroy } from '../www/core/raw-priest-pre-destroy.js';
import { RawPriestForcefield, PriestForcefieldState, makePriestForcefieldHooks } from '../www/core/raw-priest-forcefield.js';
import { RawPriestShield, PriestShieldState, makePriestShieldCreationHooks } from '../www/core/raw-priest-shield.js';

// 원본 관측 10.78 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E63)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'priestshield-x86.tsv'))?.filter((row) => row[0] === 'originals') ?? null;

// 새 자산 입력 번호, 소리 조회 반환값, 판본당 관찰 행 수, 시각 double 입력(비트 → 수)과 공통 가상 표이다. (2026-11-08 WB04-E63)
const SOURCE = 50;
const SHIELD = 100;
const SOUND = 0x12345678;
const ROWS = 832;
const COMMON_VTABLE = 0x11010000;
const CLOCKS = [0n, 0x8000000000000000n, 0x3ff0000000000000n, 0x7ff8000000001234n];

// double 비트와 수의 상호 변환이다(NaN 페이로드 보존을 위해 같은 DataView 로 옮긴다). (2026-11-08 WB04-E63)
const clockView = new DataView(new ArrayBuffer(8));
function clockOf(bits) {
    clockView.setBigUint64(0, bits, true);
    return clockView.getFloat64(0, true);
}
function clockBits(value) {
    clockView.setFloat64(0, value, true);
    return clockView.getBigUint64(0, true);
}

// 예외가 나는지 본다. (2026-11-08 WB04-E63)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 바이트 배열을 소문자 hex 로 만든다. (2026-11-08 WB04-E63)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 타입 표와 해시/현재 전역은 행마다 바꾸되 풀 객체 자체는 안정된 주소를 유지하는 장면이다. (2026-11-08 WB04-E63)
function shieldScene() {
    const pool = new SidPool(24000, { server: false });
    const scene = { pool, hash: new SquidHash(), types: null, forcefield: new PriestForcefieldState(), state: new PriestShieldState() };
    // 원본 장면의 raw 자산을 준비하고 지정한 해시 단계에만 넣는다.
    scene.node = (sid, type, status, extra, x, y, owner, level, registered) => {
        const raw = pool.physical(sid);
        raw.fill(0);
        put(raw, 0, sid === SOURCE ? PRIEST_VTABLE : COMMON_VTABLE);
        raw[10] = type;
        raw[11] = status;
        raw[40] = extra;
        raw[34] = owner;
        put(raw, 14, floatToBits(x));
        put(raw, 18, floatToBits(y));
        scene.types[type].footX = 1;
        scene.types[type].footY = 1;
        if (registered) {
            put(raw, 4, scene.hash.bucket(level, x, y), 2);
            scene.hash.setBucket(level, x, y, sid);
        }
    };
    // 결과를 계산하지 않고 원본 실행기와 같은 등록 순서/입력만 준비한다.
    scene.prepare = (row) => {
        const index = Number(row[1]);
        const owner = Number(row[2]);
        const x = index % 2 === 0 ? 20.75 : 254.9;
        const y = index % 2 === 0 ? 21.9 : 254.25;
        const status = [0, 2, 4, 6];
        const extra = [0, 1, 8, 9];
        scene.hash.reset();
        scene.types = Array.from({ length: 188 }, () => ({ footX: 0, footY: 0, flags1: 0, flags2: 0 }));
        scene.node(SOURCE, PRIEST_TYPE, status[index % 4], extra[index % 4], x, y, owner, 0, false);
        // 입력 후보는 다른 소유자/타입·매장·다른 위치·해시 순서를 가진다.
        if (index) {
            for (const i of index === 7 ? [1, 0] : [0, 1]) {
                scene.node(60 + i, index === 3 ? 166 : 167, status[(index + i) % 4], index === 4 ? 8 : 0, index === 6 ? x - 1 : x, y, index === 2 ? (owner + 1) % 9 : owner, (index + i) % 4, true);
            }
        }
        scene.forcefield.type = index === 5 ? 168 : 167;
        scene.state = new PriestShieldState({ localPlayer: Number(row[6]), loadingDepth: Number(row[5]), noticeClock: clockOf(CLOCKS[Number(row[4])]) });
        pool.physical(SHIELD).fill(0xab);
    };
    // 외부 효과가 바꾸는 입력만 제공하며 생성/알림 판단을 구현하지 않는다.
    scene.change = (row, stage) => {
        if (Number(row[7]) !== stage) return;
        const raw = pool.physical(SOURCE);
        if (stage === 1) {
            put(raw, 14, 0x7fc12345);
            put(raw, 18, 0x80000000);
            raw[34] = (Number(row[2]) + 1) % 9;
        } else if (stage === 2) {
            put(raw, 14, floatToBits(99.5));
            put(raw, 18, floatToBits(98.5));
        } else if (stage === 3) {
            raw[34] = 8;
            scene.state.localPlayer = 8;
            scene.state.loadingDepth = 0;
            scene.state.noticeClock = 0;
        } else if (stage === 4) scene.state.noticeClock = clockOf(CLOCKS[3]);
    };
    return scene;
}

test('PriestShield: 원본 10.78 832행의 생성·소유자·Pop·소리·안내 기록과 슬롯·전역이 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    const scene = shieldScene();
    let count = 0;
    // 각 원본 행의 전체 raw/전역/사건 순서를 그대로 대조한다.
    for (const row of rows) {
        assert.equal(row.length, 14);
        scene.prepare(row);
        const events = [];
        const append = (value) => events.push(value);
        const owner = new SquidOwner(scene.pool, scene.types, new SquidPostPopState(), new SquidOwnerMode());
        const lookup = new RawPriestForcefield(scene.pool, scene.hash, scene.types, scene.forcefield);
        const shield = new RawPriestShield(scene.pool, lookup, scene.state, {
            create: (type, flags) => {
                append(`C:${type}:${flags}`);
                const raw = scene.pool.physical(SHIELD);
                raw.fill(0);
                put(raw, 0, COMMON_VTABLE);
                raw[10] = type;
                raw[11] = 4;
                scene.change(row, 1);
                return SHIELD;
            },
            setOwner: (sid, value) => {
                append(`O:${sid}:${value}`);
                scene.change(row, 2);
                owner.set(sid, value);
            },
            pop: (sid, x, y, flags) => {
                append(`P:${sid}:${floatToBits(x)}:${floatToBits(y)}:${flags}`);
                const raw = scene.pool.physical(sid);
                put(raw, 14, floatToBits(x));
                put(raw, 18, floatToBits(y));
                raw[11] = 0;
                scene.change(row, 3);
            },
            reserveSound: () => { append(`A:40:${row[3]}`); return Number(row[3]) !== 0; },
            loadSound: (file) => { append(`L:${file}`); scene.change(row, 4); return SOUND; },
            attachSound: (sid, sound, first, second) => append(`S:${sid}:${sound}:${first}:${second}`),
            noticeSound: (notice) => append(`N:${notice.file}:${notice.arguments.join(':')}`),
            translate: (key) => { append(`T:${key}`); return 'notice-token'; },
            tell: (message) => append(`D:${message}`)
        });
        shield.ensure(SOURCE);
        assert.equal(events.length === 0 ? '-' : events.join(';'), row[8], `행 ${count}`);
        assert.equal(hex(scene.pool.slot(SOURCE)), row[9], `행 ${count}`);
        assert.equal(hex(scene.pool.slot(SHIELD)), row[10], `행 ${count}`);
        assert.equal(scene.state.localPlayer, Number(row[11]), `행 ${count}`);
        assert.equal(scene.state.loadingDepth, Number(row[12]), `행 ${count}`);
        assert.equal(clockBits(scene.state.noticeClock), BigInt(row[13]), `행 ${count}`);
        count += 1;
    }
    assert.equal(count, ROWS);
});

test('PriestShield: 실제 Factory/소유자 연결로 한 번만 생성하고 사제 삭제 준비가 보호막을 찾아 삭제를 요청한다', () => {
    const pool = new SidPool(24000, { server: false });
    const types = Array.from({ length: 188 }, () => ({ cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, maxHitPoints: 0 }));
    types[167].constructorAddress = typeConstructorAddress(167);
    types[167].footX = 1;
    types[167].footY = 1;
    const factory = new SquidFactory(pool, types);
    const priest = pool.allocate(2);
    const raw = pool.allocated(priest);
    raw[10] = PRIEST_TYPE;
    put(raw, 0, PRIEST_VTABLE);
    raw[34] = 1;
    put(raw, 14, floatToBits(20.75));
    put(raw, 18, floatToBits(21.9));
    raw[11] = 0;
    const hash = new SquidHash();
    const forcefield = new PriestForcefieldState();
    const lookup = new RawPriestForcefield(pool, hash, types, forcefield);
    const owner = new SquidOwner(pool, types, new SquidPostPopState(), new SquidOwnerMode());
    const state = new PriestShieldState({ localPlayer: 1, loadingDepth: 0, noticeClock: 0 });
    const count = { pops: 0, sounds: 0, notices: 0, tells: 0, reservations: 0 };
    let created = 0;
    const hooks = makePriestShieldCreationHooks(pool, factory, owner, {
        pop: (sid, x, y, flags) => {
            assert.ok(flags === 0 && x === Math.fround(20.75) && y === Math.fround(21.9));
            created = sid;
            count.pops += 1;
            // 미복원 Pop 경계가 등록 입력만 공급하며 전체 가상 Pop 의 완료를 뜻하지 않는다.
            const bytes = pool.allocated(sid);
            put(bytes, 14, floatToBits(x));
            put(bytes, 18, floatToBits(y));
            bytes[11] = 0;
            put(bytes, 4, hash.bucket(0, x, y), 2);
            hash.setBucket(0, x, y, sid);
        },
        reserveSound: () => { count.reservations += 1; return count.reservations > 1; },
        loadSound: (file) => { assert.equal(file, 'priestForceField.wav'); return SOUND; },
        attachSound: (sid, sound, first, second) => { assert.ok(sid === created && sound === SOUND && first === 0 && second === 1); count.sounds += 1; },
        noticeSound: (notice) => { assert.equal(notice.file, 'ourPriestImmobile.wav'); count.notices += 1; },
        translate: (key) => { assert.equal(key, 'PriestImmobile'); return '사제 이동 불가'; },
        tell: (text) => { assert.equal(text, '사제 이동 불가'); count.tells += 1; }
    });
    const shield = new RawPriestShield(pool, lookup, state, hooks);
    assert.equal(lookup.find(priest), 0);
    shield.ensure(priest);
    assert.ok(lookup.find(priest) === created && pool.slot(created)[34] === 1 && count.pops === 1 && count.sounds === 0 && count.notices === 1);
    const freeCount = pool.freeCount;
    shield.ensure(priest);
    assert.ok(pool.freeCount === freeCount && count.pops === 1 && count.notices === 1 && count.reservations === 1);
    const priests = new PriestPostPopState();
    priests.priests.entries = [priest];
    priests.priests.count = 1;
    let destroyed = 0;
    let carrier = 0;
    const pre = new RawPriestPreDestroy(pool, priests, makePriestForcefieldHooks(pool, lookup, {
        destroyForcefield: (sid, flags) => {
            assert.ok(sid === created && flags === 0);
            destroyed += 1;
            // 미복원 가상 destroy 경계의 공간 해제/반납 입력을 제공한다.
            hash.setBucket(0, 20.75, 21.9, 0);
            pool.allocated(sid)[11] = 4;
            pool.release(sid);
        },
        validateCarrier: () => {},
        carrierPre: (sid) => { assert.equal(sid, priest); carrier += 1; }
    }));
    pre.preDestroy(priest, 0);
    assert.ok(destroyed === 1 && carrier === 1 && priests.priests.count === 0 && lookup.find(priest) === 0);
    shield.ensure(priest);
    assert.ok(lookup.find(priest) === created && count.pops === 2 && count.sounds === 1 && count.notices === 2 && count.tells === 2);
});

test('PriestShield: 누락 경계와 다른 풀은 생성 전에 거부하고 실패한 생성 SID 를 묵시적으로 사용하지 않는다', () => {
    const pool = new SidPool(24000, { server: false });
    const other = new SidPool(24000, { server: false });
    const hash = new SquidHash();
    const types = Array.from({ length: 188 }, () => ({ cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, maxHitPoints: 0 }));
    const lookup = new RawPriestForcefield(pool, hash, types, new PriestForcefieldState());
    const state = new PriestShieldState();
    let created = 0;
    const hooks = {
        create: () => { created += 1; return 0; },
        setOwner: () => {},
        pop: () => {},
        reserveSound: () => false,
        loadSound: () => 0,
        attachSound: () => {},
        noticeSound: () => {},
        translate: () => '',
        tell: () => {}
    };
    assert.ok(throws(() => new RawPriestShield(pool, lookup, state, { ...hooks, pop: undefined })));
    assert.ok(throws(() => new RawPriestShield(pool, lookup, state, { ...hooks, tell: undefined })));
    assert.ok(throws(() => new RawPriestShield(other, lookup, state, hooks)) && created === 0);
    const factory = new SquidFactory(pool, types);
    const foreignFactory = new SquidFactory(other, types);
    const owner = new SquidOwner(pool, types, new SquidPostPopState(), new SquidOwnerMode());
    const foreignOwner = new SquidOwner(other, types, new SquidPostPopState(), new SquidOwnerMode());
    assert.ok(throws(() => makePriestShieldCreationHooks(pool, foreignFactory, owner, hooks)));
    assert.ok(throws(() => makePriestShieldCreationHooks(pool, factory, foreignOwner, hooks)));
    const raw = pool.physical(SOURCE);
    raw.fill(0);
    raw[10] = PRIEST_TYPE;
    put(raw, 0, PRIEST_VTABLE);
    const shield = new RawPriestShield(pool, lookup, state, hooks);
    assert.ok(throws(() => shield.ensure(SOURCE)) && created === 1);
    raw[11] = 1;
    assert.ok(throws(() => shield.ensure(SOURCE)) && created === 1);
    assert.ok(get(raw, 14) === 0 && bitsToFloat(0) === 0);
});
