// AI 본체 지갑 raw-ai-wallet.js(RawAiWallet)와 보상 부모의 연결을 cpppj 계약 시험(손으로 쓴 입력과 기대값)으로 검사한다.
//  - 이 몸체에는 독립 원본 관측 fixture 가 없다(cpppj 시험이 원본 분기 순서를 직접 단언한다). 같은 입력과 단언을 옮겼다: 모드/owner/NULL brain 조기 반환은 실제 본체 소유자를 선택하지 않음, signed owner 비양수 거부,
//    DWORD 감김과 데모 하한 1190(signed)과 본체 나머지 바이트 보존, 데모 조회가 Player 포인터/지갑을 바꿔도 저장한 brain 의 현재 지갑 재조회, 해제/작은 본체 거부, 조회는 쓰기 없음, 현재 Player base 재할당 추적.
//  - 보상 부모 연결 두 건(SP 고정 두 모드를 매번 현재 값으로 읽음, 보상은 같은 brain 지갑에 정수 절삭만 전달하고 독립 배열은 건드리지 않음)은 SquidReward 의 현재 고정 SP 모드 읽기(cpppj 2b048419)와 함께 검사한다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E166: AI 지갑 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import { put, get, bitsToFloat, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { GameRandom } from '../www/core/random.js';
import { ScrambledSpStore } from '../www/core/sp-store.js';
import { SquidPostPopState } from '../www/core/squid-postpop-state.js';
import { SquidReward, SquidRewardState } from '../www/core/squid-reward.js';
import { AI_BRAIN_BYTES, RawAiBrainMemory } from '../www/core/raw-ai-brain.js';
import { RawAiWallet } from '../www/core/raw-ai-wallet.js';

// 예외가 나는지 본다. (2026-11-09 WB04-E166)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 지정 바이트로 채우는 본체 메모리를 만든다(첫 핸들 지정). (2026-11-09 WB04-E166)
const makeMemory = (base, fill = 0) => new RawAiBrainMemory(base, (bytes) => bytes.fill(fill));

test('AI 지갑: 모드/owner/NULL brain 의 조기 반환은 실제 본체 소유자를 선택하지 않고 선택한 소유자는 데모 재조회까지 유지한다', () => {
    const players = new Uint8Array(1620);
    let disabled = true;
    let acquisitions = 0;
    const memory = makeMemory(0x51000000);
    const token = memory.allocate(AI_BRAIN_BYTES);
    put(memory.bytes(token), 0x2d8, 5);
    // 선택된 실제 메모리의 외부 보유자이며 데모 조회 중에 반납해도 같은 호출은 선택한 소유자를 끝까지 쓴다.
    let holder = memory;
    const wallet = new RawAiWallet(() => { acquisitions += 1; return { memory: holder, owner: holder }; }, players, {
        disabled: () => disabled,
        demo: () => {
            holder = null;
            assert.equal(get(memory.bytes(token), 0x2d8), 12);
            put(players, 180 + 0xb0, 0);
            return true;
        }
    });
    put(players, 180 + 0xb0, token);
    wallet.credit(1, 7);
    assert.ok(wallet.balance(1) === 0 && acquisitions === 0);
    disabled = false;
    wallet.credit(0, 7);
    wallet.credit(0xffffffff, 7);
    assert.ok(wallet.balance(0x80000000) === 0 && acquisitions === 0);
    put(players, 180 + 0xb0, 0);
    wallet.credit(1, 7);
    assert.ok(wallet.balance(1) === 0 && acquisitions === 0);
    put(players, 180 + 0xb0, token);
    wallet.credit(1, 7);
    assert.ok(acquisitions === 1 && holder === null);
    // 데모 하한 뒤 같은 본체의 현재 지갑을 다시 읽어 1190 으로 올린다.
    assert.equal(get(memory.bytes(token), 0x2d8), 1190);
});

test('AI 지갑: 보상 부모는 SP 고정 두 모드를 매 가산 위치에서 현재 값으로 읽고 값형 사본으로 되돌리지 않는다', () => {
    const pool = new SidPool(32768, { server: false });
    const types = Array.from({ length: 188 }, () => ({ cost: 0, maxHitPoints: 0, flags1: 0, flags2: 0 }));
    const book = new SquidPostPopState();
    const state = new SquidRewardState();
    const store = new ScrambledSpStore(new GameRandom(123), () => 100);
    // 실제 Client 의 fort bool 과 editor DWORD 를 빌리는 두 모드와 표시 관측이다.
    const modes = { fort: false, editor: 0 };
    let refreshes = 0;
    state.share('fixedSpA', modes, 'fort');
    state.share('fixedSpB', modes, 'editor');
    const reward = new SquidReward(pool, types, book, state, store, { refreshSp: () => { refreshes += 1; } });
    store.set(1, floatToBits(1000));
    reward.addSp(1, 10);
    assert.equal(bitsToFloat(store.get(1)), 1010);
    modes.editor = 0x80000000;
    reward.addSp(1, 1);
    assert.ok(bitsToFloat(store.get(1)) === 194162 && !state.fixedSpB);
    modes.editor = 0;
    modes.fort = true;
    store.set(1, floatToBits(2000));
    reward.addSp(1, 1);
    assert.ok(bitsToFloat(store.get(1)) === 194162 && !state.fixedSpA);
    modes.fort = false;
    reward.addSp(1, 2);
    assert.ok(bitsToFloat(store.get(1)) === 194164 && refreshes === 4);
    // 빌림을 풀면 독립 값형 입력으로 돌아가며 기본 키가 없는 칸은 소유자 칸 이름 없이 빌릴 수 없다.
    state.share('fixedSpA', null);
    state.share('fixedSpB', null);
    assert.ok(!state.currentFixedSpA() && !state.currentFixedSpB());
    state.fixedSpB = true;
    assert.ok(state.currentFixedSpB());
    assert.ok(throws(() => state.share('fixedSpA', modes)) && throws(() => state.share('fixedSpA', modes, 'missing')));
});

test('AI 지갑: 조회는 현재 Player 포인터의 signed 지갑만 읽으며 데모/SP/본체 바이트를 바꾸지 않는다', () => {
    const players = new Uint8Array(1620);
    const memory = makeMemory(0x4f000000, 0x5a);
    const first = memory.allocate(AI_BRAIN_BYTES);
    const second = memory.allocate(AI_BRAIN_BYTES);
    put(players, 2 * 180 + 0xb0, first);
    put(memory.bytes(first), 0x2d8, 0x80000001);
    put(memory.bytes(second), 0x2d8, 1200);
    let demos = 0;
    const wallet = new RawAiWallet(memory, players, { disabled: () => false, demo: () => { demos += 1; return true; } });
    const before = Buffer.from(memory.bytes(first));
    assert.equal(wallet.balance(2), 0x80000001 | 0);
    put(players, 2 * 180 + 0xb0, second);
    assert.ok(wallet.balance(2) === 1200 && demos === 0 && Buffer.from(memory.bytes(first)).equals(before));
    put(players, 2 * 180 + 0xb0, 0);
    assert.ok(wallet.balance(2) === 0 && demos === 0);
});

test('AI 지갑: 조회는 모드 우선, signed 비양수 owner 와 NULL brain 은 0, 손상된 물리 포인터/표 밖 owner 는 거부한다', () => {
    const players = new Uint8Array(1620);
    const memory = makeMemory(0x4f000000);
    let disabled = true;
    let modes = 0;
    put(players, 180 + 0xb0, 0xdeadbeef);
    const wallet = new RawAiWallet(memory, players, { disabled: () => { modes += 1; return disabled; }, demo: () => { throw new Error('조회에서 데모 호출 금지'); } });
    assert.ok(wallet.balance(1) === 0 && modes === 1);
    disabled = false;
    assert.ok(wallet.balance(0) === 0 && wallet.balance(0xffffffff) === 0 && wallet.balance(0x80000000) === 0 && wallet.balance(8) === 0);
    assert.ok(throws(() => wallet.balance(9)) && throws(() => wallet.balance(1)));
});

test('AI 지갑: 비활성/signed 비양수 owner/NULL brain 은 지갑과 데모 경계를 선택하지 않는다', () => {
    const players = new Uint8Array(1620);
    const memory = makeMemory(0x10000000, 0xa7);
    let disabled = true;
    let modeReads = 0;
    let demoReads = 0;
    const wallet = new RawAiWallet(memory, players, { disabled: () => { modeReads += 1; return disabled; }, demo: () => { demoReads += 1; return false; } });
    put(players, 180 + 0xb0, 0xdead0000);
    wallet.credit(1, 42);
    assert.ok(modeReads === 1 && !demoReads && memory.allocations.length === 0);
    disabled = false;
    // signed DWORD 비양수는 표 밖 주소로 해석하지 않는다.
    for (const owner of [0, 0xffffffff, 0x80000000]) wallet.credit(owner, 42);
    wallet.credit(8, 42);
    assert.ok(modeReads === 5 && !demoReads);
    assert.ok(throws(() => wallet.credit(9, 42)));
    assert.ok(throws(() => wallet.credit(1, 42)));
    assert.ok(!demoReads);
});

test('AI 지갑: 독립 기대 DWORD 로 signed 감김/데모 하한과 본체 전체의 나머지 바이트 보존을 확인한다', () => {
    const players = new Uint8Array(1620);
    const memory = makeMemory(0x10000000, 0xa7);
    const token = memory.allocate(AI_BRAIN_BYTES);
    put(players, 180 + 0xb0, token);
    let demo = false;
    let demoReads = 0;
    const wallet = new RawAiWallet(memory, players, { disabled: () => false, demo: () => { demoReads += 1; return demo; } });
    // 초기값/가산 비트/데모/기대값을 원본 분기에서 명시한 입력이다.
    const rows = [[0x7fffffff, 1, 0, 0x80000000], [0xffffffff, 2, 0, 1], [1200, 0xfffffff6, 1, 1190], [1191, 0, 1, 1191], [0x80000000, 0, 1, 1190], [0x7fffffff, 1, 1, 1190], [0, 0xffffffff, 0, 0xffffffff], [1201, 0, 1, 1201]];
    // 각 행은 계산기를 기대값으로 쓰지 않고 본체 전체와 Player 표를 직접 대조한다.
    for (const row of rows) {
        const bytes = memory.bytes(token);
        put(bytes, 0x2d8, row[0]);
        const expected = Buffer.from(bytes);
        put(expected, 0x2d8, row[3]);
        const before = Buffer.from(players);
        demo = row[2] !== 0;
        wallet.credit(1, row[1] | 0);
        assert.ok(Buffer.from(memory.bytes(token)).equals(expected) && Buffer.from(players).equals(before));
    }
    assert.ok(demoReads === 8 && memory.allocations.length === 1 && memory.frees.length === 0);
});

test('AI 지갑: 데모 조회가 Player/지갑을 바꾸면 현재 값을 읽되 저장한 brain 포인터는 유지한다', () => {
    const players = new Uint8Array(1620);
    const memory = makeMemory(0x10000000);
    const first = memory.allocate(AI_BRAIN_BYTES);
    const second = memory.allocate(AI_BRAIN_BYTES);
    put(players, 180 + 0xb0, first);
    put(memory.bytes(first), 0x2d8, 7);
    put(memory.bytes(second), 0x2d8, 5000);
    let reads = 0;
    const wallet = new RawAiWallet(memory, players, {
        disabled: () => false,
        demo: () => {
            reads += 1;
            if (reads !== 1) return false;
            assert.equal(get(memory.bytes(first), 0x2d8), 8);
            put(players, 180 + 0xb0, second);
            put(memory.bytes(first), 0x2d8, 0x80000001);
            return true;
        }
    });
    wallet.credit(1, 1);
    assert.ok(get(memory.bytes(first), 0x2d8) === 1190 && get(memory.bytes(second), 0x2d8) === 5000 && get(players, 180 + 0xb0) === second);
    wallet.credit(1, 20);
    assert.ok(get(memory.bytes(first), 0x2d8) === 1190 && get(memory.bytes(second), 0x2d8) === 5020 && reads === 2);
});

test('AI 지갑: 크기 없는/해제된 포인터와 누락 조회기를 지갑 성공으로 대신하지 않는다', () => {
    const players = new Uint8Array(1620);
    const memory = makeMemory(0x10000000);
    const reads = { disabled: () => false, demo: () => true };
    assert.ok(throws(() => new RawAiWallet(memory, players.subarray(0, 1619), reads)));
    assert.ok(throws(() => new RawAiWallet(memory, players, {})));
    assert.ok(throws(() => new RawAiWallet(undefined, players, reads)));
    const wallet = new RawAiWallet(memory, players, reads);
    const small = memory.allocate(0x2dc);
    put(players, 180 + 0xb0, small);
    assert.ok(throws(() => wallet.credit(1, 5)) && !get(memory.bytes(small), 0x2d8));
    memory.free(small);
    assert.ok(throws(() => wallet.credit(1, 5)));
    // 선택된 소유자가 없는 acquire 는 새 메모리로 대체하지 않고 거부한다.
    const lacking = new RawAiWallet(() => ({ memory: null }), players, reads);
    put(players, 180 + 0xb0, 0x5555);
    assert.ok(throws(() => lacking.credit(1, 5)) && throws(() => lacking.balance(1)));
});

test('AI 지갑: 보상 부모는 같은 brain 지갑에 정수 절삭만 전달하고 값형 지갑 배열은 읽거나 복사하지 않는다', () => {
    const players = new Uint8Array(1620);
    const memory = makeMemory(0x10000000);
    const token = memory.allocate(AI_BRAIN_BYTES);
    put(players, 180 + 0xb0, token);
    put(memory.bytes(token), 0x2d8, 777);
    const wallet = new RawAiWallet(memory, players, { disabled: () => false, demo: () => false });
    // 실제 지갑은 777 이고 독립 배열은 다른 값을 가진다.
    const pool = new SidPool(32768, { server: false });
    const types = Array.from({ length: 188 }, () => ({ cost: 0, maxHitPoints: 0, flags1: 0, flags2: 0 }));
    const book = new SquidPostPopState();
    const state = new SquidRewardState();
    state.aiWallet.fill(9000);
    state.aiMirrorDisabled = true;
    state.walletClamp = true;
    const random = new GameRandom(123);
    const store = new ScrambledSpStore(random, () => 100);
    store.set(1, floatToBits(1000));
    let refreshes = 0;
    const reward = new SquidReward(pool, types, book, state, store, {
        refreshSp: () => {
            refreshes += 1;
            assert.equal(get(memory.bytes(token), 0x2d8), 877);
            assert.equal(bitsToFloat(store.get(1)), 1100.75);
        }
    });
    reward.bindPatchWallet((owner, amount) => wallet.credit(owner, amount));
    // 가산 float 는 SP 에 그대로, 정수 절삭 100 만 실제 지갑에 전달한다.
    reward.addSp(1, 100.75);
    assert.ok(refreshes === 1 && !book.aiAttached[1] && state.aiWallet[1] === 9000);
    const before = random.state;
    reward.creditWallet(1, -4);
    assert.ok(get(memory.bytes(token), 0x2d8) === 873 && random.state === before && refreshes === 1 && bitsToFloat(store.get(1)) === 1100.75);
    assert.ok(throws(() => reward.bindPatchWallet(undefined)));
    reward.creditWallet(1, 1);
    assert.equal(get(memory.bytes(token), 0x2d8), 874);
});

test('AI 지갑: 현재 Player base 가 재할당되면 오래된 표를 쓰지 않고 현재 표의 brain 을 조기 반환 뒤에만 선택한다', () => {
    const firstPlayers = new Uint8Array(1620);
    const secondPlayers = new Uint8Array(1620);
    let disabled = true;
    let second = false;
    let playerReads = 0;
    let memoryReads = 0;
    const memory = makeMemory(0x52000000);
    const first = memory.allocate(AI_BRAIN_BYTES);
    const next = memory.allocate(AI_BRAIN_BYTES);
    // 서로 다른 현재 Player base 와 실제 본체이다.
    put(firstPlayers, 180 + 0xb0, first);
    put(secondPlayers, 180 + 0xb0, next);
    put(memory.bytes(first), 0x2d8, 7);
    put(memory.bytes(next), 0x2d8, 100);
    const wallet = new RawAiWallet(() => { memoryReads += 1; return { memory, owner: null }; }, () => { playerReads += 1; return second ? secondPlayers : firstPlayers; }, { disabled: () => disabled, demo: () => false });
    wallet.credit(1, 4);
    assert.ok(wallet.balance(1) === 0 && playerReads === 0 && memoryReads === 0);
    disabled = false;
    wallet.credit(0, 4);
    assert.ok(wallet.balance(0xffffffff) === 0 && playerReads === 0 && memoryReads === 0);
    wallet.credit(1, 4);
    second = true;
    wallet.credit(1, 5);
    assert.ok(wallet.balance(1) === 105 && get(memory.bytes(first), 0x2d8) === 11 && playerReads === 3 && memoryReads === 3);
    // 현재 base 가 9행 폭이 아니면 거부한다.
    second = false;
    const broken = new RawAiWallet(memory, () => new Uint8Array(10), { disabled: () => false, demo: () => false });
    assert.ok(throws(() => broken.balance(1)));
});
