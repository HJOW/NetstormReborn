// 소유 사제 조회 raw-priest-lookup.js(RawPriestLookup)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: priestlookup-x86.tsv 10.78(originals) 1617행 — 소유자 인자·활성 개수·목록·노드(sid, 타입, 상태, 소유자, 원래 소유자, extra) → 반환 SID, 전체 128슬롯 raw 와 목록의 Adler-32(조회의 순수성 포함).
//    CD 10.72·10.37 행은 웹 범위(10.78) 밖이다.
//  - 계약: PriestLookup_LazyGuardsPreserveEarlyMatchAndDwordWidth(손상 개수/SID 의 늦은 거부와 DWORD 소유자 폭)·PriestLookup_ObservesRealRegistrationOwnershipAndPreDestroy(실제 사제 등록·소유자 지정·삭제 준비의 목록 압축).
//    건설 확정/통지 어댑터(RawConstructionConfirm·RawConstructionNotice)와 기준점 어댑터는 해당 파일 이식 때 대조한다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E66: 사제 소유자 조회 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, adler32 } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { GameRandom } from '../www/core/random.js';
import { ScrambledSpStore } from '../www/core/sp-store.js';
import { SquidFactory } from '../www/core/squid-factory.js';
import { SquidOwner, SquidOwnerMode } from '../www/core/squid-owner.js';
import { SquidPostPopState } from '../www/core/squid-postpop-state.js';
import { SquidReward, SquidRewardState } from '../www/core/squid-reward.js';
import { typeConstructorAddress } from '../www/core/squid-constructors.js';
import { PRIEST_TYPE } from '../www/core/raw-priest-owner.js';
import { PriestPostPopState, RawPriestPostPop } from '../www/core/raw-priest-post-pop.js';
import { RawPriestPreDestroy } from '../www/core/raw-priest-pre-destroy.js';
import { RawPriestLookup } from '../www/core/raw-priest-lookup.js';

// 원본 관측 10.78 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E66)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'priestlookup-x86.tsv'))?.filter((row) => row[0] === 'originals') ?? null;

// 판본당 독립 행 수, 입력 슬롯 개수, 풀 용량, 작업장 기준 SID 이다. (2026-11-08 WB04-E66)
const ROWS = 1617;
const SLOTS = 128;
const CAPACITY = 24000;
const WORKSHOP = 50;

// 예외가 나는지 본다. (2026-11-08 WB04-E66)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 바이트 배열을 소문자 hex 로 만든다. (2026-11-08 WB04-E66)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 쉼표로 구분한 수 목록을 읽는다. 빈 문자열은 빈 목록이다. (2026-11-08 WB04-E66)
const inputs = (text) => (text === '' ? [] : text.split(',').map(Number));

// 현재 소유자와 상태/원래 소유자/extra 를 공급한다. 기대 선택은 계산하지 않는다. (2026-11-08 WB04-E66)
function node(pool, sid, type, state, owner, original = 0, extra = 0) {
    const raw = pool.physical(sid);
    raw.fill(0);
    raw[10] = type;
    raw[11] = state;
    raw[34] = owner;
    raw[12] = original;
    raw[40] = extra;
}

// 입력 슬롯 128개의 raw 를 하나로 이어 독립 사본으로 만든다(조회의 순수성과 checksum 대조용). (2026-11-08 WB04-E66)
function region(pool) {
    const stride = pool.layout.stride;
    const merged = new Uint8Array(SLOTS * stride);
    // 슬롯 순서대로 복사한다.
    for (let sid = 0; sid < SLOTS; sid += 1) merged.set(pool.physical(sid), sid * stride);
    return merged;
}

test('PriestLookup: 원본 10.78 1617행의 반환 SID·128슬롯 raw·목록 checksum 이 같고 조회는 입력을 바꾸지 않는다', { skip: !rows && '원본 fixture 없음' }, () => {
    const pool = new SidPool(CAPACITY, { server: false });
    const state = new PriestPostPopState();
    const lookup = new RawPriestLookup(pool, state);
    let count = 0;
    // 매 행에서 동일한 raw·목록 입력을 만들고 조회의 순수성을 함께 확인한다.
    for (const row of rows) {
        assert.equal(row.length, 8);
        for (let sid = 0; sid < SLOTS; sid += 1) pool.physical(sid).fill(0);
        state.priests.entries = inputs(row[3]);
        state.priests.count = Number(row[2]);
        // 타입과 original/extra 는 원본이 읽지 않는 필드지만 같은 값으로 채운다.
        for (const text of row[4].split(';')) {
            const values = inputs(text);
            assert.equal(values.length, 6);
            node(pool, ...values);
        }
        const list = new Uint8Array(16);
        // 저장 목록 checksum 은 개수 밖의 꼬리를 포함한다.
        state.priests.entries.forEach((entry, index) => put(list, index * 4, entry));
        const before = hex(region(pool));
        const entries = [...state.priests.entries];
        const result = lookup.find(Number(row[1]));
        const after = region(pool);
        assert.equal(result, Number(row[5]), `행 ${count}`);
        assert.equal(adler32(after), Number(row[6]), `행 ${count}`);
        assert.equal(adler32(list), Number(row[7]), `행 ${count}`);
        assert.equal(hex(after), before, `행 ${count}`);
        assert.deepEqual(state.priests.entries, entries, `행 ${count}`);
        assert.equal(state.priests.count, Number(row[2]), `행 ${count}`);
        count += 1;
    }
    assert.equal(count, ROWS);
});

test('PriestLookup: 조기 일치는 보존하고 실제 접근하는 손상 목록만 WORD 절단 전에 거부한다', () => {
    const pool = new SidPool(CAPACITY, { server: false });
    const state = new PriestPostPopState();
    const lookup = new RawPriestLookup(pool, state);
    state.priests.count = 0xffffffff;
    assert.equal(lookup.find(3), 0);
    state.priests.count = 0x80000000;
    assert.equal(lookup.find(3), 0);
    state.priests.count = 1;
    assert.ok(throws(() => lookup.find(3)));
    node(pool, WORKSHOP, 82, 5, 3, 1, 255);
    state.priests.entries = [50, 0x10032];
    state.priests.count = 2;
    assert.equal(lookup.find(3), 50);
    assert.ok(throws(() => lookup.find(259)));
    state.priests.entries = [0x10032];
    state.priests.count = 1;
    assert.ok(throws(() => lookup.find(3)));
    state.priests.entries = [CAPACITY];
    assert.ok(throws(() => lookup.find(3)));
    state.priests.entries = [50];
    assert.equal(lookup.find(259), 0);
    state.priests.entries = [0, 0x10032];
    state.priests.count = 2;
    node(pool, 0, 1, 0, 3);
    assert.equal(lookup.find(3), 0);
    state.priests.count = 0;
    assert.equal(lookup.find(0xffffffff), 0);
    assert.ok(throws(() => new RawPriestLookup(pool, {})));
});

test('PriestLookup: 실제 사제 등록/중복 억제·소유자 지정·삭제 준비의 목록 압축이 같은 조회 객체에 반영된다', () => {
    const pool = new SidPool(CAPACITY, { server: true });
    const types = Array.from({ length: 188 }, () => ({ cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, maxHitPoints: 0 }));
    types[PRIEST_TYPE].flags1 = 0x10;
    types[PRIEST_TYPE].flags2 = 0x210000;
    types[PRIEST_TYPE].constructorAddress = typeConstructorAddress(PRIEST_TYPE);
    const factory = new SquidFactory(pool, types);
    const book = new SquidPostPopState();
    const owner = new SquidOwner(pool, types, book, new SquidOwnerMode());
    const reward = new SquidReward(pool, types, book, new SquidRewardState(), new ScrambledSpStore(new GameRandom(), () => 1));
    const state = new PriestPostPopState();
    state.priests.entries = [0, 0, 0, 0];
    const lookup = new RawPriestLookup(pool, state);
    const post = new RawPriestPostPop(pool, reward, state, { findRegular: () => true, reserveRegular: () => false, addRegular: () => {} });
    const first = factory.create(PRIEST_TYPE, 2);
    const second = factory.create(PRIEST_TYPE, 2);
    // 가상 표는 실제 생성자가 쓴 값을 보존하고 공간 접두 입력만 준비한다.
    for (const sid of [first, second]) {
        const raw = pool.allocated(sid);
        raw[11] = 0;
        raw[40] = 0;
        raw[12] = 1;
        owner.set(sid, 1);
        post.prefix(sid, 1);
        post.prefix(sid, 1);
    }
    assert.ok(state.priests.count === 2 && lookup.find(1) === first);
    owner.set(first, 3);
    assert.ok(lookup.find(1) === second && lookup.find(3) === first);
    pool.allocated(first)[11] |= 2;
    assert.equal(lookup.find(3), 0);
    let carrierCalls = 0;
    const pre = new RawPriestPreDestroy(pool, state, {
        findForcefield: () => 0,
        destroyForcefield: () => {},
        validateCarrier: () => {},
        carrierPre: (sid, flags) => { assert.ok(sid === first && flags === 0); carrierCalls += 1; }
    });
    pre.preDestroy(first, 0);
    assert.ok(state.priests.count === 1 && state.priests.entries[0] === second && carrierCalls === 1);
    pool.allocated(first)[11] &= ~2;
    assert.equal(lookup.find(3), 0);
});
