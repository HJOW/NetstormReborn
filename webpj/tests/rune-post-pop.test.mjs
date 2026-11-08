// Rune 후처리 raw-rune-post-pop.js(RawRunePostPop·RunePostPopState·makeRunePostPopHooks)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: runepostpop-x86.tsv 의 10.78(originals) 행(입력 8개: flags·후보 프로필·예약 성공·altar owner WORD·Player 플래그·시간 억제·변이·extra → 건설 시간·Dais·효과 기록·후보 세 슬롯 이미지·시간 억제 전역).
//    각 입력은 독립 pool/hash/Player 표를 쓴다. CD 10.72 행은 웹 범위(10.78) 밖이다.
//  - 계약: BuriedRuneCurrent_Rune*(실제 PATCH 가상 표 진입의 사전 검사 비삽입, 최초 예약 누락이 finder/공통 부모보다 앞섬, 확보 실패 시 미연결 생성자 비요구, 생성자 누락의 부분 쓰기,
//    304 생성 효과 뒤 현재 Dais TYPE/소유자 재조회, 후보 없을 때 건설 장치 비요구, 시간 helper 의 타입 표 범위 오류가 건설 장치 검사보다 앞섬, 예약 뒤 공유 전역 재조회, 시간 억제가 Player 접근보다 앞섬).
//    실제 Rune 생성자→Pop→Construction 완료→304 Regular 통합(rawRunePostPop_real_pop_construction_complete_and_304)은 RawConstructionProcess 이식 단위의 몫이다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E61: Rune 후처리 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { SquidPostPop } from '../www/core/squid-postpop.js';
import { SquidPostPopState } from '../www/core/squid-postpop-state.js';
import { RawRunePostPop, RunePostPopState, RUNE_VTABLE, makeRunePostPopHooks } from '../www/core/raw-rune-post-pop.js';

// 원본 관측 10.78 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E61)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'runepostpop-x86.tsv'))?.filter((row) => row[0] === 'originals') ?? null;

// 공통 가상 표(관측 후보용)이다. (2026-11-08 WB04-E61)
const COMMON_VTABLE = 0x11010000;

// 예외가 나는지 본다. (2026-11-08 WB04-E61)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 바이트 배열을 소문자 hex 로 만든다. (2026-11-08 WB04-E61)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 188칸 TYPE 표이다. (2026-11-08 WB04-E61)
const runeTypes = () => Array.from({ length: 188 }, () => ({ cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, maxHitPoints: 0, zOrder: 0 }));

// 효과 기록을 원본 fixture 와 같은 문자열로 합친다. (2026-11-08 WB04-E61)
const effectsText = (effects) => (effects.length === 0 ? '-' : effects.join(';'));

test('RunePostPop: 원본 10.78 행의 건설 시간·Dais·효과 순서·후보 슬롯 이미지가 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    let checked = 0;
    // 각 입력은 독립 pool/hash/player 표를 사용해 이전 기대값을 재사용하지 않는다.
    for (const row of rows) {
        assert.equal(row.length, 7);
        const input = row[1].split(',').map((part) => Number(part) >>> 0);
        assert.equal(input.length, 8);
        const [flags, profile, reserve, word, playerFlag, suppress, mutation, extra] = input;
        const pool = new SidPool(24000, { server: true });
        const hash = new SquidHash();
        const types = runeTypes();
        const slots = [];
        // 후보 세 개의 사용하지 않는 필드까지 native 와 같은 0 초기값으로 만든다.
        for (let sid = 5; sid <= 8; sid += 1) {
            assert.equal(pool.allocate(2), sid);
            slots[sid] = pool.allocated(sid);
            slots[sid].fill(0);
        }
        const add = (sid, type, x, y, currentExtra, rawState, foot) => {
            const raw = slots[sid];
            put(raw, 0, COMMON_VTABLE);
            raw[10] = type;
            raw[11] = rawState;
            raw[40] = currentExtra;
            put(raw, 14, floatToBits(x));
            put(raw, 18, floatToBits(y));
            types[type].footX = foot;
            types[type].footY = foot;
            put(raw, 4, hash.bucket(1, x, y), 2);
            hash.setBucket(1, x, y, sid);
        };
        add(8, 166, 40.75, 41.25, extra === 0 ? 0 : extra === 1 ? 1 : 8, 0, 1);
        if (profile) add(6, 165, profile === 7 ? 80.75 : profile === 6 ? 42.75 : 40.75, profile === 7 ? 81.25 : profile === 6 ? 43.25 : 41.25, profile === 3 ? 8 : profile === 5 ? 1 : 0, profile === 4 ? 2 : 0, profile === 6 ? 3 : 1);
        if (profile !== 8) add(7, 164, profile === 9 ? 80.75 : 40.75, profile === 9 ? 81.25 : 41.25, 0, 0, 1);
        put(slots[8], 0, RUNE_VTABLE);
        slots[8][34] = 1;
        slots[6][34] = profile === 2 ? 2 : 1;
        put(slots[7], 12, word, 2);
        const state = new RunePostPopState();
        state.suppressDuration = suppress !== 0;
        state.playerFlags[word] = playerFlag;
        const book = new SquidPostPopState();
        book.depth = 1;
        const base = new SquidPostPop(pool, types, book);
        const effects = [];
        const hooks = {
            reserveRegular: () => {
                effects.push('N:40');
                if (mutation === 1) state.suppressDuration = !state.suppressDuration;
                return reserve !== 0;
            },
            addRegular: (sid, event, payload) => {
                assert.ok(sid === 8 && event === 0x304 && payload === Math.fround(0.01));
                effects.push(`R:${event},8,${floatToBits(payload)}`);
                if (mutation === 2) slots[8][34] = 2;
            },
            startConstruction: (sid, seconds) => {
                assert.equal(sid, 8);
                effects.push(`C:8,${floatToBits(seconds)}`);
            }
        };
        const rune = new RawRunePostPop(pool, hash, types, state, base, hooks);
        assert.equal(rune.constructionSeconds(8), Number(row[2]), `행 ${checked}`);
        assert.equal(rune.findOwnedDais(8), Number(row[3]), `행 ${checked}`);
        rune.postPop(8, flags);
        assert.equal(book.depth, 0);
        effects.push(`B:8,${flags}`);
        assert.equal(effectsText(effects), row[4], `행 ${checked}`);
        assert.equal(state.suppressDuration, Number(row[6]) !== 0);
        const images = row[5].split(';');
        assert.equal(images.length, 3);
        // 가상 표 주입 외 후보/raw Rune 의 모든 바이트를 비교한다.
        for (let sid = 6; sid <= 8; sid += 1) {
            const image = hex(pool.slot(sid));
            assert.equal(image.length, images[sid - 6].length);
            assert.equal(image.slice(8), images[sid - 6].slice(8), `행 ${checked} 슬롯 ${sid}`);
        }
        checked += 1;
    }
    assert.ok(checked > 500, `원본 10.78 행 수 ${checked}`);
});

// 실제 finder/시간 helper/공통 부모를 작은 독립 현재 월드에 연결한 장면이다: Rune5 · Dais6 · Altar7 를 같은 위치에 둔다. (2026-11-08 WB04-E61)
function runeScene() {
    const pool = new SidPool(24000, { server: true });
    const hash = new SquidHash();
    const types = runeTypes();
    const state = new RunePostPopState();
    const book = new SquidPostPopState();
    const base = new SquidPostPop(pool, types, book);
    const slots = [];
    const events = [];
    // finder 입력만 준비하고 Rune 제품의 선택 결과는 계산하지 않는다.
    for (let value = 5; value <= 7; value += 1) {
        assert.equal(pool.allocate(2), value);
        const raw = pool.allocated(value);
        slots[value] = raw;
        raw.fill(0);
        const type = value === 5 ? 166 : value === 6 ? 165 : 164;
        raw[10] = type;
        raw[34] = 1;
        put(raw, 14, floatToBits(40.75));
        put(raw, 18, floatToBits(41.25));
        types[type].footX = 1;
        types[type].footY = 1;
        put(raw, 0, value === 5 ? RUNE_VTABLE : COMMON_VTABLE);
        if (value !== 5) {
            put(raw, 4, hash.bucket(1, 40.75, 41.25), 2);
            hash.setBucket(1, 40.75, 41.25, value);
        }
    }
    put(slots[7], 12, 1, 2);
    book.depth = 1;
    book.suppressed = true;
    base.bindCurrentTypes(types);
    const scene = { pool, hash, types, state, book, base, slots, events };
    // 호출의 인자/순서만 관찰하고 finder·시간 계산은 실제 제품에 맡긴다.
    scene.hooks = () => ({
        reserveRegular: () => { events.push('N'); return true; },
        addRegular: (sid, event, payload) => { assert.ok(sid === 5 && event === 0x304 && payload === Math.fround(0.01)); events.push('R'); },
        startConstruction: (sid, seconds) => { assert.equal(sid, 5); events.push(`C${Math.trunc(seconds)}`); }
    });
    scene.make = (hooks = scene.hooks()) => new RawRunePostPop(pool, hash, types, state, base, hooks);
    return scene;
}

test('BuriedRuneCurrent: 실제 PATCH 가상 표 진입은 TYPE/free 사전 검사를 추가하지 않고 공통 부모로 간다', () => {
    const scene = runeScene();
    const raw = scene.slots[5];
    raw[10] = 74;
    raw[11] = 1;
    raw[40] = 8;
    const rune = scene.make({});
    scene.base.setBody('rune', rune);
    assert.ok(rune.handles(5) && throws(() => rune.validate(5, 0)));
    scene.base.postPopVirtual(5, 0);
    assert.ok(scene.book.depth === 0 && scene.events.length === 0);
});

test('BuriedRuneCurrent: 최초 확보가 없으면 뒤의 좌표 오류나 공통 부모보다 먼저 실패하고, 확보 실패는 미연결 생성자를 요구하지 않는다', () => {
    const missing = runeScene();
    put(missing.slots[5], 14, 0x7fc00000);
    const hooks = missing.hooks();
    hooks.reserveRegular = undefined;
    assert.ok(throws(() => missing.make(hooks).postPop(5, 3)) && missing.events.length === 0 && missing.book.depth === 1);
    const failed = runeScene();
    const failHooks = failed.hooks();
    failHooks.reserveRegular = () => { failed.events.push('N'); return false; };
    failHooks.addRegular = undefined;
    failed.make(failHooks).postPop(5, 3);
    assert.ok(effectsText(failed.events) === 'N;C7' && failed.book.depth === 0);
    const noCtor = runeScene();
    put(noCtor.slots[5], 14, 0x7fc00000);
    const noCtorHooks = noCtor.hooks();
    noCtorHooks.addRegular = undefined;
    assert.ok(throws(() => noCtor.make(noCtorHooks).postPop(5, 3)) && effectsText(noCtor.events) === 'N' && noCtor.book.depth === 1);
});

test('BuriedRuneCurrent: 304 생성 효과 뒤 현재 Dais TYPE/소유자를 읽고 free/VT 변경을 재검사하지 않는다', () => {
    const scene = runeScene();
    const hooks = scene.hooks();
    hooks.addRegular = () => {
        scene.events.push('R');
        scene.slots[5][11] = 1;
        scene.slots[5][10] = 255;
        scene.slots[5][34] = 2;
        put(scene.slots[5], 0, 0);
        scene.slots[6][34] = 2;
        scene.slots[6][10] = 74;
        scene.state.daisType = 74;
        scene.types[74].footX = 1;
        scene.types[74].footY = 1;
    };
    scene.make(hooks).postPop(5, 3);
    assert.ok(effectsText(scene.events) === 'N;R;C7' && scene.book.depth === 0 && scene.slots[5][11] === 1 && scene.slots[5][10] === 255);
});

test('BuriedRuneCurrent: 후보가 없으면 건설 장치를 미리 검사하지 않고, 시간 helper 의 타입 표 범위 오류가 건설 장치 검사보다 앞선다', () => {
    const none = runeScene();
    none.state.daisType = 65535;
    const hooks = none.hooks();
    hooks.startConstruction = undefined;
    none.make(hooks).postPop(5, 2);
    assert.ok(none.events.length === 0 && none.book.depth === 0);
    const range = runeScene();
    range.state.altarType = 188;
    const rangeHooks = range.hooks();
    rangeHooks.startConstruction = undefined;
    assert.ok(throws(() => range.make(rangeHooks).postPop(5, 2)) && range.book.depth === 1 && range.events.length === 0);
});

test('BuriedRuneCurrent: 예약 뒤 공유 전역을 다시 읽어 25초를 선택하고, 시간 억제는 Player 접근보다 앞서 선택된다', () => {
    const scene = runeScene();
    let suppressed = true;
    let flags = 0;
    scene.state.suppressDuration = true;
    const hooks = scene.hooks();
    hooks.reserveRegular = () => { scene.events.push('N'); suppressed = false; flags = 1; return false; };
    hooks.suppressDuration = () => { scene.events.push('S'); return suppressed; };
    hooks.playerFlags = (owner) => { assert.equal(owner, 1); scene.events.push('P'); return flags; };
    scene.make(hooks).postPop(5, 3);
    assert.ok(effectsText(scene.events) === 'N;S;P;C25' && scene.book.depth === 0);
    const guard = runeScene();
    put(guard.slots[7], 12, 65535, 2);
    const guardHooks = guard.hooks();
    guardHooks.suppressDuration = () => { guard.events.push('S'); return true; };
    guardHooks.playerFlags = () => { throw new RangeError('접근하지 않을 Player'); };
    guard.make(guardHooks).postPop(5, 2);
    assert.ok(effectsText(guard.events) === 'S;C10' && guard.book.depth === 0);
});

test('RunePostPop: 실제 Host/Construction 훅 연결은 같은 풀만 받고 현재 전역 getter 를 매번 읽는다', () => {
    const scene = runeScene();
    const calls = [];
    const host = { pool: scene.pool, addRegular: (sid, event, payload) => calls.push(`H${sid}:${event}:${payload}`) };
    const construction = { pool: scene.pool, start: (sid, seconds) => calls.push(`K${sid}:${seconds}`) };
    const hooks = makeRunePostPopHooks(scene.pool, host, construction, { reserveRegular: () => true });
    scene.make(hooks).postPop(5, 3);
    assert.deepEqual(calls, [`H5:772:${Math.fround(0.01)}`, 'K5:7']);
    assert.ok(throws(() => makeRunePostPopHooks(scene.pool, { pool: new SidPool(24000, { server: true }) }, construction, {})));
    let runeType = 166;
    scene.state.sharedRuneType = () => runeType;
    assert.equal(scene.state.readRuneType(), 166);
    runeType = 74;
    assert.equal(scene.state.readRuneType(), 74);
    assert.ok(throws(() => new RawRunePostPop(scene.pool, scene.hash, scene.types.slice(0, 100), scene.state, scene.base, {})));
});
