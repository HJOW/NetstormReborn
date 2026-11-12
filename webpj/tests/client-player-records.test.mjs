// Client 수명 Player 기록 소유자(ClientPlayerRecords)와 공통 시작 연결(ClientPlayerRuntime)의 수명 계약을 cpppj 시험과 같은 장면으로 대조한다(WB04-E332, 4-D).
//  - cpppj `ClientPlayerRecordsTests`(6건: 공장 헤더와 단일 저장소·대여와 재할당 뒤 현재 뷰 재조회·초기화 실패의 부분 수명·Client 소유자 지연 할당·Native 재사용 둘)와
//    `ClientPlayerRuntimeTests`(4건: 공통 시작·Core0 선행·초기화된 저장소 채택·할당 실패 뒤 채택 거부)를 옮겼다. 실제 `Client` 는 4-D 조립이라 최소 덕타입 장치로 대신한다.
//  - 웹 계약: 현재 할당 입력 모듈, 대여 해제, 주소 토큰 발급과 해제 종류 검사.
// (2026-11-12 WB04-E332: 4-D Player 기록 소유자 이식과 함께 추가)
import assert from 'node:assert/strict';
import test from 'node:test';
import { ClientPlayerRecords } from '../www/core/client-player-records.js';
import { initializeClientPlayerRecords } from '../www/core/client-player-runtime.js';
import { ClientCoreDataGlobals } from '../www/core/client-core-data-globals.js';
import { currentAllocationBytes } from '../www/core/current-allocation-bytes.js';
import { PlayerWorldState } from '../www/core/raw-player-world.js';
import { PlayerParticipationState, RawPlayerParticipation } from '../www/core/raw-player-participation.js';
import { SquidPostPopList, SquidPostPopState } from '../www/core/squid-postpop-state.js';

// 기대하는 보호 경계의 실제 예외만 확인한다. (2026-11-12 WB04-E332)
const rejects = (operation) => { try { operation(); } catch { return true; } return false; };

// raw 헤더의 4BYTE 를 입력/결과 확인에 쓴다. (2026-11-12 WB04-E332)
function ownerPut(bytes, offset, value) {
    for (let byte = 0; byte < 4; byte += 1) bytes[offset + byte] = (value >>> (byte * 8)) & 0xff;
}

// little endian DWORD 를 읽는다. (2026-11-12 WB04-E332)
function ownerRead(bytes, offset) {
    return (bytes[offset] | (bytes[offset + 1] << 8) | (bytes[offset + 2] << 16) | (bytes[offset + 3] << 24)) >>> 0;
}

// 원문 236BYTE 의 동맹 자식은 합성 행 쓰기 대신 기존 실제 00490ea0 몸체로 실행한다. (2026-11-12 WB04-E332)
function ownerInitialize(records, use) {
    const state = new PlayerParticipationState();
    const hidden = new Uint8Array(9);
    use.initialize((owner, mask) => {
        const initial = new RawPlayerParticipation(records.records(), records.alliances(), hidden, state, {});
        initial.setAlliance(owner, mask);
    });
}

// 채우기 할당 입력을 만든다(공장 120BYTE 의 미초기화 입력). (2026-11-12 WB04-E332)
const filler = (value, onCall = null) => (bytes) => { onCall?.(bytes); bytes.fill(value); };

// ---------------------------------------------------------------- ClientPlayerRecords

// 실제 +0/+4/+8 과 공장 배열이 추가/중복 삭제/포인터 변경에 같은 값을 읽고 쓰는지 검사한다.
test('Player 기록: 현재 공장 헤더와 단일 저장소', () => {
    let allocations = 0;
    const records = new ClientPlayerRecords(filler(0x79, (bytes) => { assert.equal(bytes.length, 120); allocations += 1; }));
    assert.deepEqual([records.base(), records.records().length], [0, 0]);
    const use = records.acquire();
    ownerInitialize(records, use);
    assert.equal(allocations, 9);
    const books = new SquidPostPopState();
    const bytes = records.records();
    // 모든 원문 행의 초기화와 비초기화 공장 입력을 확인한다.
    for (let owner = 0; owner < 9; owner += 1) {
        use.bindFactory(books.ownerFactories[owner], owner);
        const list = books.ownerFactories[owner];
        assert.ok(ownerRead(bytes, owner * 180) !== 0);
        assert.deepEqual([ownerRead(bytes, owner * 180 + 4), ownerRead(bytes, owner * 180 + 8)], [30, 0]);
        assert.deepEqual([ownerRead(bytes, owner * 180 + 0x78), ownerRead(bytes, owner * 180 + 0x54)], [127, 1 << owner]);
        assert.deepEqual([list.entries.length, list.count, list.capacity(), list.readCount()], [0, 0, 30, 0]);
        assert.ok([...list.storage()].every((value) => value === 0x79797979));
        // 방향표의 실제 자식 결과이다.
        for (let target = 0; target < 9; target += 1) assert.equal(records.alliances()[owner * 9 + target], owner === target ? 1 : 0);
    }
    const list = books.ownerFactories[1];
    for (const value of [51, 52, 51, 53]) list.append(value);
    list.removeAll(51);
    assert.equal(ownerRead(bytes, 180 + 8), 2);
    assert.deepEqual([...list.items()], [52, 53]);
    assert.deepEqual([list.storage()[2], list.storage()[3]], [51, 53]);
    // 현재 포인터 변경으로 두 목록이 같은 배열을 읽는지 검사한다.
    const original = ownerRead(bytes, 180);
    const other = ownerRead(bytes, 360);
    ownerPut(bytes, 180, other);
    ownerPut(bytes, 180 + 4, 2);
    ownerPut(bytes, 180 + 8, 1);
    list.append(54);
    list.append(55);
    assert.deepEqual([list.readCount(), ownerRead(bytes, 180 + 8)], [2, 2]);
    const second = books.ownerFactories[2];
    assert.deepEqual([list.storage().buffer === second.storage().buffer, list.storage().byteOffset === second.storage().byteOffset], [true, true]);
    assert.deepEqual([second.storage()[1], second.readCount()], [54, 0]);
    // 원문 다음 free 가 같은 할당을 두 번 지우지 않도록 입력 포인터를 복원한다.
    ownerPut(bytes, 180, original);
    ownerPut(bytes, 180 + 4, 30);
    ownerPut(bytes, 180 + 8, 2);
    // 용량 0·NULL 배열이면 full 이라 추가하지 않고, 활성 개수가 용량을 넘으면 항목 조회를 거부한다.
    ownerPut(bytes, 180 + 4, 0);
    ownerPut(bytes, 180, 0);
    list.append(99);
    assert.equal(list.readCount(), 2);
    assert.ok(rejects(() => list.items()));
    ownerPut(bytes, 180, original);
    ownerPut(bytes, 180 + 4, 30);
    use.release();
});

// 살아 있는 대여의 재할당은 막고 이전 대여 반납 뒤에는 현재 목록을 새 Player 로 재조회한다.
test('Player 기록: 대여와 재할당 뒤 현재 뷰 재조회', () => {
    const records = new ClientPlayerRecords(filler(0x43));
    const first = records.acquire();
    ownerInitialize(records, first);
    const list = new SquidPostPopList();
    first.bindFactory(list, 3);
    list.append(88);
    const previous = records.base();
    records.fortCount().value = 7;
    records.lastLoginPlayer().value = 8;
    assert.ok(rejects(() => records.acquire()));
    assert.ok(rejects(() => ownerInitialize(records, first)));
    assert.deepEqual([records.base(), [...list.items()], records.fortCount().value], [previous, [88], 7]);
    first.release();
    const second = records.acquire();
    ownerInitialize(records, second);
    assert.notEqual(records.base(), previous);
    assert.deepEqual([records.fortCount().value, records.lastLoginPlayer().value], [0, 0]);
    // 보관한 목록은 새 현재 base 를 재조회하며 옛 뷰를 읽지 않는다.
    assert.deepEqual([list.readCount(), list.capacity(), list.items().length, list.storage()[0]], [0, 30, 0, 0x43434343]);
    assert.ok(rejects(() => second.bindFactory(list, 3)));
    const populated = new SquidPostPopList();
    populated.entries = [1];
    populated.count = 1;
    assert.ok(rejects(() => second.bindFactory(populated, 2)));
    second.release();
});

// 두 번째 공장 할당 실패가 원문의 앞선 행과 동맹 효과를 되돌리지 않는지 검사한다.
test('Player 기록: 초기화 실패는 도달한 접두를 남긴다', () => {
    let allocations = 0;
    const records = new ClientPlayerRecords((bytes) => {
        allocations += 1;
        if (allocations === 2) throw new RangeError('두 번째 new 실패');
        bytes.fill(0x29);
    });
    const use = records.acquire();
    assert.ok(rejects(() => ownerInitialize(records, use)));
    const raw = records.records();
    assert.deepEqual([raw.length, records.base() !== 0, ownerRead(raw, 0) !== 0, ownerRead(raw, 4), ownerRead(raw, 0x78)], [1620, true, true, 30, 127]);
    assert.deepEqual([ownerRead(raw, 180), ownerRead(raw, 180 + 4), ownerRead(raw, 180 + 0x78), allocations], [0, 0, 0, 2]);
    assert.deepEqual([records.alliances()[0], records.alliances()[10]], [1, 0]);
    assert.ok(rejects(() => ownerInitialize(records, use)));
    assert.ok(rejects(() => use.bindFactory(new SquidPostPopList(), 0)));
    assert.equal(records.initialized(), false);
});

// 새 소유자는 조회만으로 Player 를 할당하지 않는다.
test('Player 기록: 소유자는 조회만으로 할당하지 않는다', () => {
    const owner = new ClientPlayerRecords();
    assert.deepEqual([owner.base(), owner.records().length, owner.initialized(), owner.boundCoreData()], [0, 0, false, null]);
    assert.deepEqual([owner.fortCount().value, owner.lastLoginPlayer().value, owner.alliances().length], [0, 0, 81]);
    const use = owner.acquire();
    ownerInitialize(owner, use);
    // 같은 소유자에서 같은 실제 뷰를 돌려준다.
    assert.equal(owner.records(), owner.records());
    assert.equal(owner.records().length, 1620);
    assert.ok(owner.productionDeckMemory());
    assert.equal(owner.productionDeckMemory(), owner.productionDeckMemory());
    use.release();
    use.release();
    assert.ok(owner.acquire());
});

// Native 공유 목록은 같은 Client/행 getter 를 재사용하며 실제 재할당 뒤 새 base 를 계속 조회한다.
test('Player 기록: Native 재사용은 재할당 뒤에도 현재 공장을 유지한다', () => {
    const records = new ClientPlayerRecords(filler(0x67));
    const list = new SquidPostPopList();
    const first = records.acquire();
    ownerInitialize(records, first);
    first.reuseOrBindFactory(list, 3);
    list.append(88);
    first.reuseOrBindFactory(list, 3);
    assert.deepEqual([list.readCount(), [...list.items()]], [1, [88]]);
    const base = records.base();
    first.release();
    const second = records.acquire();
    // 초기화되지 않은 대여는 재사용도 거부한다.
    assert.ok(rejects(() => second.reuseOrBindFactory(list, 3)));
    ownerInitialize(records, second);
    second.reuseOrBindFactory(list, 3);
    assert.notEqual(records.base(), base);
    assert.deepEqual([list.readCount(), list.capacity(), list.storage()[0]], [0, 30, 0x67676767]);
    list.append(99);
    second.reuseOrBindFactory(list, 3);
    assert.deepEqual([list.readCount(), [...list.items()]], [1, [99]]);
    // 기존 독립 Bind 의 중복 거부는 그대로 유지한다.
    assert.ok(rejects(() => second.bindFactory(list, 3)));
    second.release();
});

// 명시 재사용도 다른 실제 Client/행이나 독립 데이터가 있는 목록을 덮지 않는다.
test('Player 기록: Native 재사용은 다른 소유자·행·독립 데이터를 거부한다', () => {
    const first = new ClientPlayerRecords(filler(0));
    const other = new ClientPlayerRecords(filler(0));
    const a = first.acquire();
    const b = other.acquire();
    ownerInitialize(first, a);
    ownerInitialize(other, b);
    const list = new SquidPostPopList();
    a.reuseOrBindFactory(list, 2);
    list.append(77);
    assert.ok(rejects(() => a.reuseOrBindFactory(list, 3)));
    assert.ok(rejects(() => b.reuseOrBindFactory(list, 2)));
    assert.deepEqual([list.readCount(), [...list.items()]], [1, [77]]);
    const independent = new SquidPostPopList();
    independent.entries = [55];
    independent.count = 1;
    assert.ok(rejects(() => a.reuseOrBindFactory(independent, 2)));
    assert.deepEqual([independent.entries, independent.count], [[55], 1]);
    // owner 번호 범위.
    assert.ok(rejects(() => a.reuseOrBindFactory(new SquidPostPopList(), 9)));
    assert.ok(rejects(() => a.bindFactory(new SquidPostPopList(), 9)));
});

// 주소 토큰 발급은 겹치지 않고, 잘못된 해제 종류/주소와 범위 밖 요청은 거부한다(웹 계약).
test('Player 기록: 주소 토큰·해제 종류·요청 폭 검사', () => {
    const records = new ClientPlayerRecords();
    const use = records.acquire();
    ownerInitialize(records, use);
    const base = records.base();
    // 초기화 순서상 Player 영역이 먼저 확보되고 공장 9개가 뒤를 이어 0x1000 간격으로 발급된다.
    assert.equal(base, 0x30000000);
    assert.equal(ownerRead(records.records(), 0), 0x30001000);
    assert.equal(ownerRead(records.records(), 180), 0x30002000);
    // 바인딩된 목록이 범위 밖 owner 의 용량 조회를 하지 못한다.
    const list = new SquidPostPopList();
    use.bindFactory(list, 8);
    assert.equal(list.capacity(), 30);
    // 공장 포인터를 존재하지 않는 주소로 바꾸면 배열 접근에서 거부한다.
    ownerPut(records.records(), 8 * 180, 0x7000);
    assert.throws(() => list.storage(), /물리 주소 없음/);
    ownerPut(records.records(), 8 * 180, 0x30001000 + 8 * 0x1000);
    // 용량이 블록보다 크면 요청 폭 범위 오류이다.
    ownerPut(records.records(), 8 * 180 + 4, 31);
    assert.throws(() => list.storage(), /요청폭 범위 오류/);
    ownerPut(records.records(), 8 * 180 + 4, 30);
    use.release();
});

// 현재 할당 입력 모듈: 웹은 새 할당의 0 을 그대로 입력으로 쓰며 Uint8Array 가 아니면 거부한다.
test('현재 할당 입력: 입력을 바꾸지 않는다', () => {
    const bytes = new Uint8Array(8);
    currentAllocationBytes(bytes);
    assert.ok(bytes.every((byte) => byte === 0));
    const marked = Uint8Array.of(1, 2, 3);
    currentAllocationBytes(marked);
    assert.deepEqual([...marked], [1, 2, 3]);
    assert.throws(() => currentAllocationBytes([1, 2]), TypeError);
    // 기본 할당 입력을 쓰는 소유자는 공장 120BYTE 를 0 으로 둔다.
    const records = new ClientPlayerRecords();
    const use = records.acquire();
    ownerInitialize(records, use);
    const list = new SquidPostPopList();
    use.bindFactory(list, 4);
    assert.ok([...list.storage()].every((value) => value === 0));
    use.release();
});

// ---------------------------------------------------------------- ClientPlayerRuntime

// 실제 Client 대신 쓰는 최소 장치: 같은 소유자들과 PlayerWorldState 를 가진다. (2026-11-12 WB04-E332)
function makeClient(allocationBytes = null) {
    const records = new ClientPlayerRecords(allocationBytes);
    const core = new ClientCoreDataGlobals();
    records.bindCoreData(core);
    const player = new PlayerWorldState();
    player.playerKnown = 0;
    return { records, core, player, playerRecords: () => records, gameplayGlobals: () => ({ player }) };
}

// 공통 시작: 실제 소유자에 9기록/9공장/81방향 동맹이 구성되고 Core 뷰가 같은 Player 를 가리킨다.
test('Player 공통 시작: 실제 기록·Core 뷰·동맹 구성', () => {
    const client = makeClient();
    const { records, core, player } = client;
    assert.deepEqual([records.base(), records.initialized(), core.players.length, records.boundCoreData() === core], [0, false, 0, true]);
    initializeClientPlayerRecords(client);
    assert.ok(records.base() !== 0 && records.initialized());
    assert.deepEqual([core.players.buffer === records.records().buffer, core.players.byteOffset === records.records().byteOffset, core.players.length], [true, true, 1620]);
    assert.deepEqual([core.current, core.active, player.playerKnown], [null, 0, 0]);
    for (let owner = 0; owner < 9; owner += 1) {
        assert.deepEqual([ownerRead(records.records(), owner * 180 + 4), ownerRead(records.records(), owner * 180 + 8), ownerRead(records.records(), owner * 180 + 0x54)], [30, 0, 1 << owner]);
        // 대칭 표를 별도 계산해 주입하지 않는다: 자기 자신만 1 이다.
        for (let target = 0; target < 9; target += 1) assert.equal(records.alliances()[owner * 9 + target], owner === target ? 1 : 0);
    }
    // known 이 0 이 아니면 공통 시작의 관문이 거부한다.
    player.playerKnown = 1;
    assert.throws(() => initializeClientPlayerRecords(client), /known0 관문/);
});

// 살아 있는 Core 는 해제 전에 원문 Core 0 선택을 요구하고, 그 뒤 재초기화는 새 base 에 Core 뷰를 다시 잇는다.
test('Player 공통 시작: 살아 있는 Core 는 해제 전 Core0 선택이 필요하다', () => {
    const client = makeClient();
    const { records, core, player } = client;
    initializeClientPlayerRecords(client);
    core.select(player, 0, 3, 1);
    core.active = 7;
    const source = new Uint8Array(10);
    // 이전 Player+6c 에 명시 입력을 쓴다(해제 뒤 그 뷰를 읽지 않음).
    for (let index = 0; index < source.length; index += 1) source[index] = core.current[index] = 71 + index;
    const base = records.base();
    const before = Uint8Array.from(records.records());
    player.playerKnown = 0;
    assert.ok(rejects(() => initializeClientPlayerRecords(client)));
    assert.deepEqual([records.base(), records.initialized(), [...records.records()].every((byte, index) => byte === before[index]), core.active, core.current !== null], [base, true, true, 7, true]);
    core.select(player, 0, 0, 1);
    assert.deepEqual([core.current, core.active, [...core.cached()]], [null, 0, [...source]]);
    initializeClientPlayerRecords(client);
    assert.notEqual(records.base(), base);
    assert.deepEqual([core.players.buffer === records.records().buffer, core.current, [...core.cached()]], [true, null, [...source]]);
    core.select(player, 0, 1, 1);
    assert.equal(core.current.buffer, records.records().buffer);
    assert.deepEqual([core.current.byteOffset - records.records().byteOffset, player.playerKnown], [180 + 0x6c, 1]);
});

// 초기화된 저장소를 채택해도 Native 초기화를 다시 실행하지 않고, 대여 반납은 Player 데이터를 초기화하지 않는다.
test('Player 공통 시작: 초기화된 저장소 채택은 재초기화하지 않는다', () => {
    const client = makeClient();
    const { records } = client;
    initializeClientPlayerRecords(client);
    const base = records.base();
    const before = Uint8Array.from(records.records());
    records.fortCount().value = 5;
    records.lastLoginPlayer().value = 7;
    const use = records.acquire();
    use.adoptCurrent();
    const list = new SquidPostPopList();
    use.bindFactory(list, 1);
    assert.deepEqual([list.readCount(), records.base(), records.fortCount().value, records.lastLoginPlayer().value], [0, base, 5, 7]);
    assert.deepEqual([...records.records()], [...before]);
    assert.ok(rejects(() => initializeClientPlayerRecords(client)));
    assert.ok(rejects(() => use.adoptCurrent()));
    use.release();
    assert.deepEqual([records.base(), records.initialized()], [base, true]);
    // 초기화 전이나 이미 시도한 대여는 채택할 수 없다.
    const fresh = new ClientPlayerRecords();
    assert.ok(rejects(() => fresh.acquire().adoptCurrent()));
});

// 할당 실패는 도달한 부분 뷰를 Core 에 남기되 초기화 완료로 대여하지 않는다.
test('Player 공통 시작: 할당 실패는 부분 뷰를 남기고 채택을 거부한다', () => {
    const core = new ClientCoreDataGlobals();
    const records = new ClientPlayerRecords(() => { throw new Error('검사 명시 factory 할당 입력 중단'); });
    records.bindCoreData(core);
    assert.throws(() => records.initialize(() => {}), /factory 할당 입력 중단/);
    assert.ok(records.base() !== 0);
    assert.equal(records.records().length, 1620);
    assert.deepEqual([core.players.buffer === records.records().buffer, core.players.byteOffset === records.records().byteOffset], [true, true]);
    assert.equal(records.initialized(), false);
    const use = records.acquire();
    assert.ok(rejects(() => use.adoptCurrent()));
    // 대여가 살아 있는 동안은 Core 연결도 거부한다.
    assert.throws(() => records.bindCoreData(core), TypeError);
    use.release();
    // Core 소유자는 한 번만 연결할 수 있다(같은 소유자 재연결은 허용).
    assert.throws(() => records.bindCoreData(new ClientCoreDataGlobals()), TypeError);
    assert.throws(() => records.bindCoreData(null), TypeError);
    records.bindCoreData(core);
    assert.equal(core.players.buffer, records.records().buffer);
});

// ---------------------------------------------------------------- 변형 확인으로 보강한 경계 (WB04-E332)

// 재할당은 새 블록·새 뷰를 만들고 이전 블록 주소는 더 이상 해석되지 않는다. 부분 실패 뒤 재초기화는 NULL 공장 해제를 허용하고 잘못된 종류의 해제는 거부한다.
test('Player 기록: 재할당 뒤 이전 블록·NULL 해제·잘못된 해제 종류', () => {
    const records = new ClientPlayerRecords(filler(0x11));
    const first = records.acquire();
    ownerInitialize(records, first);
    const oldBase = records.base();
    const oldView = records.records();
    first.release();
    const second = records.acquire();
    ownerInitialize(records, second);
    // 새 base·새 뷰이고 이전 뷰 객체와 buffer 가 다르다.
    assert.notEqual(records.base(), oldBase);
    assert.notEqual(records.records().buffer, oldView.buffer);
    assert.equal(ownerRead(records.records(), 4), 30);
    // 이전 Player 블록은 해제됐으므로 그 주소를 공장 포인터로 쓰면 해석에 실패한다.
    const list = new SquidPostPopList();
    second.bindFactory(list, 3);
    ownerPut(records.records(), 3 * 180, oldBase);
    assert.throws(() => list.storage(), /물리 주소 없음/);
    // 공장 포인터를 NULL(토큰 0)·용량 0 으로 두면 배열 접근은 빈 배열이다(NULL 배열을 읽지 않음).
    ownerPut(records.records(), 3 * 180, 0);
    ownerPut(records.records(), 3 * 180 + 4, 0);
    assert.equal(list.storage().length, 0);
    ownerPut(records.records(), 3 * 180 + 4, 0xffffffff);
    assert.throws(() => list.storage(), /용량 넘침/);
    ownerPut(records.records(), 3 * 180 + 4, 31);
    // 현재 살아 있는 다른 행의 공장 블록(120BYTE)을 가리키면 31칸(124BYTE) 요청이 블록 폭을 넘는다.
    ownerPut(records.records(), 3 * 180, ownerRead(records.records(), 4 * 180));
    assert.throws(() => list.storage(), /요청폭 범위 오류/);
    // 큰 개수는 DWORD 네 바이트로 쓴다.
    ownerPut(records.records(), 3 * 180 + 4, 30);
    list.setCount(0x12345678);
    assert.equal(ownerRead(records.records(), 3 * 180 + 8), 0x12345678);
    second.release();
});

// 부분 실패 뒤 다시 초기화하면 아직 할당되지 않은 공장 칸(NULL)의 해제는 생략이 아니라 허용되고, 완료 표시는 새 초기화가 끝나야 켜진다.
test('Player 기록: 부분 실패 뒤 재초기화와 완료 표시', () => {
    let fail = true;
    let allocations = 0;
    const records = new ClientPlayerRecords((bytes) => {
        allocations += 1;
        if (fail && allocations === 2) throw new Error('두 번째 공장 실패');
        bytes.fill(0x21);
    });
    const use = records.acquire();
    assert.ok(rejects(() => ownerInitialize(records, use)));
    use.release();
    assert.equal(records.initialized(), false);
    fail = false;
    const again = records.acquire();
    ownerInitialize(records, again);
    assert.equal(records.initialized(), true);
    again.release();
    // 정상 초기화 뒤의 실패한 재초기화는 완료 표시를 내린다.
    fail = true;
    allocations = 1;
    const third = records.acquire();
    assert.ok(rejects(() => ownerInitialize(records, third)));
    assert.equal(records.initialized(), false);
    third.release();
});

// 공장 칸에 Player 블록 주소를 넣으면 해제 종류가 맞지 않아 거부한다.
test('Player 기록: 잘못된 해제 종류는 거부한다', () => {
    const records = new ClientPlayerRecords(filler(0));
    const use = records.acquire();
    ownerInitialize(records, use);
    use.release();
    ownerPut(records.records(), 0, records.base());
    const second = records.acquire();
    assert.throws(() => ownerInitialize(records, second), /반납 주소\/종류 오류/);
    second.release();
});

// 중복 반납은 무시되어 다음 대여의 선점을 풀지 않는다.
test('Player 기록: 중복 반납은 다음 대여를 풀지 않는다', () => {
    const records = new ClientPlayerRecords(filler(0));
    const first = records.acquire();
    first.release();
    const next = records.acquire();
    first.release();
    assert.ok(rejects(() => records.acquire()));
    next.release();
    assert.ok(records.acquire());
});

// 주소 발급이 소진되면 새 할당을 거부한다(시험용 첫 토큰).
test('Player 기록: 주소 소진은 새 할당을 거부한다', () => {
    const records = new ClientPlayerRecords(filler(0), 0xffffe000);
    const use = records.acquire();
    assert.throws(() => ownerInitialize(records, use), /주소 소진/);
});
