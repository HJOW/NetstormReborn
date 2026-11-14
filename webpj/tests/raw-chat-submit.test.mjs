// Chat 제출 raw-chat-submit(addChatRecipient·submitChatInput)의 수신자 배열 확장·입력 파싱·채널 마스크·본문 분배를 기준선 소스 계약과 손 계산 바이트로 검증한다.
// cpppj에도 직접 시험이 없어 원본 직접 관측 수치가 아니다. 실제 RawPlayerIdentity.initialize 와 같은 60바이트 지역을 쓴다. 실제 게임·브라우저 실행이 아니다. (2026-11-14 WB04-E446)
import test from 'node:test';
import assert from 'node:assert/strict';
import { CHAT_SUBMIT_WORK_BYTES, addChatRecipient, submitChatInput } from '../www/ui/raw-chat-submit.js';
import { PLAYER_IDENTITY_RECORD_SIZE } from '../www/core/raw-player-identity.js';

// 바이트 문자열 보조 함수다.
const ascii = (value) => Uint8Array.from(Array.from(value, (char) => char.charCodeAt(0) & 0xff));
const getDword = (bytes, at) => bytes[at] + bytes[at + 1] * 256 + bytes[at + 2] * 65536 + bytes[at + 3] * 16777216;
const putDword = (bytes, at, value) => { for (let index = 0; index < 4; index += 1) bytes[at + index] = Math.floor(value / 2 ** (8 * index)) & 0xff; };
const textAt = (bytes, at = 0) => { let value = ''; for (let index = at; bytes[index]; index += 1) value += String.fromCharCode(bytes[index]); return value; };

// 기록 틀이다. manager 토큰 1(+0 count/+4 capacity/+8 배열 토큰 3/+12 직접 표시/+16 마스크), 처음 배열 3(용량 capacity 칸). 식별 장치는 이름→subscriber 표다. Chat 기록 100×48 은 rows 로 만든다.
function rig({ count = 0, capacity = 4, subscribers = {}, rows = [] } = {}) {
    const log = []; const manager = new Uint8Array(32); const memory = new Map([[1, manager], [3, new Uint8Array(1024)]]); putDword(manager, 0, count); putDword(manager, 4, capacity); putDword(manager, 8, 3);
    let next = 100; const records = new Uint8Array(4800); for (const [index, row] of rows.entries()) { records[index * 48 + 40] = row.flags & 0xff; putDword(records, index * 48 + 44, row.subscriber); }
    const hooks = {
        fields: [],
        memory: (token) => { const found = memory.get(token); if (!found) throw new RangeError(`토큰 없음 ${token}`); return found; },
        allocate: (bytes) => { log.push(['allocate', bytes]); if (hooks.failAllocate) return 0; const token = next; next += 1; memory.set(token, new Uint8Array(bytes)); return token; }, release: (token) => { log.push(['release', token]); },
        assertion: (line) => log.push(['assertion', line]),
        resolve: (record) => { const name = textAt(record, 8); log.push(['resolve', name]); hooks.fields.push([getDword(record, 0), getDword(record, 4), getDword(record, 48), getDword(record, 52)]); const found = subscribers[name.toLowerCase()]; if (found === undefined) return false; putDword(record, 4, found); putDword(record, 0, 3); putDword(record, 48, 0x1234); return true; },
        translate: (key) => { log.push(['translate', key]); return key; }, records: () => records, inputVisible: (value) => log.push(['visible', value]),
        dispatch: (body) => log.push(['dispatch', textAt(body), body.length]),
    };
    return { hooks, manager, memory, log, records, work: new Uint8Array(CHAT_SUBMIT_WORK_BYTES + 10).fill(0x55), identity: new Uint8Array(PLAYER_IDENTITY_RECORD_SIZE).fill(0x44) };
}
const list = (manager, memory) => Array.from({ length: getDword(manager, 0) }, (_, index) => getDword(memory.get(getDword(manager, 8)), index * 4));

test('addChatRecipient: 중복은 추가하지 않고, 빈 칸이 있으면 현재 count 위치에 쓰고 count 를 늘린다(signed count 로 비교)', () => {
    const r = rig({ count: 1 }); putDword(r.memory.get(3), 0, 11); addChatRecipient(1, 22, r.hooks); addChatRecipient(1, 11, r.hooks); addChatRecipient(1, 22, r.hooks); assert.deepEqual(list(r.manager, r.memory), [11, 22]); assert.equal(getDword(r.manager, 4), 4);
    const negative = rig({ count: 0xffffffff }); assert.throws(() => addChatRecipient(1, 5, negative.hooks), { name: 'RangeError', message: /store의 실제 물리 범위/ }); assert.equal(getDword(negative.manager, 0), 0xffffffff); const wide = rig({ count: 0 }); addChatRecipient(1, 0xffffffff, wide.hooks); assert.deepEqual(list(wide.manager, wide.memory), [0xffffffff]);
});

test('addChatRecipient 확장: capacity 가 count 이하면 capacity×4+128 바이트를 새로 받아 capacity 칸 전체를 복사하고 이전 배열을 반납한 뒤 capacity+32 로 쓴다', () => {
    const r = rig({ count: 2, capacity: 2 }); putDword(r.memory.get(3), 0, 7); putDword(r.memory.get(3), 4, 8); addChatRecipient(1, 9, r.hooks);
    assert.deepEqual(r.log, [['allocate', 136], ['release', 3]]); assert.deepEqual([getDword(r.manager, 0), getDword(r.manager, 4), getDword(r.manager, 8)], [3, 34, 100]); assert.deepEqual(list(r.manager, r.memory), [7, 8, 9]);
    // capacity 가 count 보다 크면 확장하지 않는다. 같으면 확장한다. 음수 capacity 는 signed 비교로 확장한다(복사 칸 0).
    const roomy = rig({ count: 1, capacity: 2 }); addChatRecipient(1, 9, roomy.hooks); assert.deepEqual(roomy.log, []); const edge = rig({ count: 4, capacity: 4 }); addChatRecipient(1, 9, edge.hooks); assert.equal(edge.log[0][0], 'allocate');
    const neg = rig({ count: 0, capacity: 0xfffffff0 }); addChatRecipient(1, 9, neg.hooks); assert.equal(neg.log[0][0], 'allocate'); assert.equal(neg.log[0][1], ((0xfffffff0 * 4) + 128) >>> 0 || 0);
});

test('addChatRecipient 실패: 할당 실패(0)는 진단 0x112 를 알리고 원문 접근을 계속(토큰 0 물리 없음 RangeError)하며, 필수 장치는 도달한 위치에서만 요구한다', () => {
    const r = rig({ count: 1, capacity: 1 }); r.hooks.failAllocate = true; assert.throws(() => addChatRecipient(1, 9, r.hooks), { name: 'RangeError', message: /토큰 없음 0/ }); assert.deepEqual(r.log, [['allocate', 132], ['assertion', 0x112]]);
    const noAssertion = rig({ count: 1, capacity: 1 }); noAssertion.hooks.failAllocate = true; delete noAssertion.hooks.assertion; assert.throws(() => addChatRecipient(1, 9, noAssertion.hooks), { name: 'Error', message: /실제 선택 장치 미연결/ });
    const roomy = rig({ count: 0 }); delete roomy.hooks.allocate; delete roomy.hooks.release; delete roomy.hooks.assertion; addChatRecipient(1, 9, roomy.hooks); assert.equal(getDword(roomy.manager, 0), 1);
    const noRelease = rig({ count: 1, capacity: 1 }); delete noRelease.hooks.release; assert.throws(() => addChatRecipient(1, 9, noRelease.hooks), /실제 선택 장치 미연결/); const noMemory = rig(); delete noMemory.hooks.memory; assert.throws(() => addChatRecipient(1, 9, noMemory.hooks), /실제 선택 장치 미연결/);
    const thin = rig(); thin.hooks.memory = (token) => (token === 1 ? new Uint8Array(10) : new Uint8Array(8)); assert.throws(() => addChatRecipient(1, 9, thin.hooks), { name: 'RangeError', message: /DWORD의 실제 물리 범위/ });
});

test('submitChatInput 대상 없음: ":" 가 없으면 입력 전체(선행 공백 제거)가 본문이고 마스크 0x20 이며 수신자 count/직접 표시는 0 이다', () => {
    const r = rig({ count: 3 }); putDword(r.manager, 12, 7); submitChatInput(1, ascii('  \t hello world'), r.identity, r.work, r.hooks);
    assert.deepEqual([getDword(r.manager, 0), getDword(r.manager, 12), getDword(r.manager, 16)], [0, 0, 0x20]); assert.deepEqual(r.log.filter((entry) => ['dispatch', 'visible'].includes(entry[0])), [['dispatch', 'hello world', 12]]);
    assert.equal(textAt(r.work), 'hello world'); assert.equal(r.work[CHAT_SUBMIT_WORK_BYTES], 0x55);
    const blank = rig(); submitChatInput(1, ascii('   '), blank.identity, blank.work, blank.hooks); assert.deepEqual(blank.log, [['visible', 0]]); const empty = rig(); submitChatInput(1, ascii(''), empty.identity, empty.work, empty.hooks); assert.deepEqual(empty.log, [['visible', 0]]);
});

test('submitChatInput 마스크 0x20: Chat 기록 100행에서 플래그가 겹치는 행의 subscriber 를 수신자에 추가한다(0 subscriber/겹치지 않는 행은 제외, signed 플래그)', () => {
    const rows = [{ flags: 0x20, subscriber: 41 }, { flags: 0x01, subscriber: 42 }, { flags: 0x20, subscriber: 0 }, { flags: 0xe0, subscriber: 43 }, { flags: 0x20, subscriber: 41 }];
    const r = rig({ rows }); submitChatInput(1, ascii('hi'), r.identity, r.work, r.hooks); assert.deepEqual(list(r.manager, r.memory), [41, 43]);
    const thin = rig(); thin.hooks.records = () => new Uint8Array(4799); assert.throws(() => submitChatInput(1, ascii('hi'), thin.identity, thin.work, thin.hooks), { name: 'RangeError', message: /100행의 실제 물리 부족/ });
    const none = rig(); delete none.hooks.records; assert.throws(() => submitChatInput(1, ascii('hi'), none.identity, none.work, none.hooks), /실제 선택 장치 미연결/);
});

test('submitChatInput 식별된 대상: ":" 앞 이름마다 식별자 지역에 쓰고 resolve 가 성공하면 수신자에 추가하며 직접 표시(+12)는 1, 본문은 ":" 뒤 앞 공백 제거다', () => {
    const r = rig({ subscribers: { alice: 11, bob: 22 } }); submitChatInput(1, ascii('  alice  BOB :\t hi there'), r.identity, r.work, r.hooks);
    assert.deepEqual(r.log.filter((entry) => entry[0] === 'resolve'), [['resolve', 'alice'], ['resolve', 'BOB']]); assert.deepEqual(list(r.manager, r.memory), [11, 22]); assert.deepEqual([getDword(r.manager, 12), getDword(r.manager, 16)], [1, 0]);
    assert.deepEqual(r.log.filter((entry) => entry[0] === 'dispatch'), [['dispatch', 'hi there', 9]]); assert.equal(textAt(r.work, 2048), 'alice  BOB '); assert.deepEqual(Array.from(r.identity.subarray(0, 8)), [3, 0, 0, 0, 22, 0, 0, 0]);
    const emptyBody = rig({ subscribers: { alice: 11 } }); submitChatInput(1, ascii('alice:'), emptyBody.identity, emptyBody.work, emptyBody.hooks); assert.equal(emptyBody.log.at(-1)[0], 'visible'); assert.equal(getDword(emptyBody.manager, 12), 1);
    // 이름 사이/뒤의 공백만 있는 접두와 콜론 앞 공백뿐인 입력은 이름 없이 끝난다.
    const noNames = rig(); submitChatInput(1, ascii('   : hello'), noNames.identity, noNames.work, noNames.hooks); assert.deepEqual([noNames.log.some((entry) => entry[0] === 'resolve'), getDword(noNames.manager, 12), getDword(noNames.manager, 16)], [false, 1, 0]);
    assert.deepEqual(noNames.log.filter((entry) => entry[0] === 'dispatch'), [['dispatch', 'hello', 6]]);
});

test('submitChatInput 채널 이름: 식별 실패 이름은 여덟 채널 키(빈 키 포함)를 원문 순서로 번역해 비교하고 일치하면 마스크 비트를 켜며 직접 표시를 0 으로 쓴다', () => {
    const rows = [{ flags: 8, subscriber: 51 }, { flags: 0x20, subscriber: 52 }, { flags: 0x40, subscriber: 53 }];
    const r = rig({ rows }); submitChatInput(1, ascii('everyone Battle: ping'), r.identity, r.work, r.hooks);
    assert.deepEqual(r.log.filter((entry) => entry[0] === 'translate').map((entry) => entry[1]), ['', '', '', 'BATTLE', '', 'EVERYONE', '', '', '', 'BATTLE']); assert.equal(getDword(r.manager, 16), 0x28); assert.equal(getDword(r.manager, 12), 0); assert.deepEqual(list(r.manager, r.memory), [51, 52]);
    const allies = rig({ rows }); submitChatInput(1, ascii('ALLIES: x'), allies.identity, allies.work, allies.hooks); assert.equal(getDword(allies.manager, 16), 0x40); assert.deepEqual(list(allies.manager, allies.memory), [53]);
    const observer = rig(); submitChatInput(1, ascii('observer: x'), observer.identity, observer.work, observer.hooks); assert.equal(getDword(observer.manager, 16), 0x80);
    const noTranslate = rig(); delete noTranslate.hooks.translate; assert.throws(() => submitChatInput(1, ascii('zzz: x'), noTranslate.identity, noTranslate.work, noTranslate.hooks), /실제 선택 장치 미연결/);
});

test('submitChatInput 미식별 이름: 식별/채널 모두 실패하면 수신자 0·마스크 0x20·입력 전체(2048바이트까지)를 본문으로 되돌리고 2047 에 NUL 을 쓰며 중단한다', () => {
    const rows = [{ flags: 0x20, subscriber: 61 }]; const r = rig({ rows, subscribers: { alice: 11 } }); putDword(r.manager, 0, 5); submitChatInput(1, ascii('alice nobody: not a command'), r.identity, r.work, r.hooks);
    assert.deepEqual([getDword(r.manager, 12), getDword(r.manager, 16)], [0, 0x20]); assert.equal(textAt(r.work), 'alice nobody: not a command'); assert.deepEqual(list(r.manager, r.memory), [61]);
    assert.deepEqual(r.log.filter((entry) => entry[0] === 'dispatch'), [['dispatch', 'alice nobody: not a command', 28]]); assert.equal(r.work[2047], 0); assert.equal(r.work[27], 0);
    // 입력이 2048바이트를 넘으면 앞 2048 만 쓰고 2047 번째를 NUL 로 덮어쓴다.
    const long = rig({ subscribers: {} }); submitChatInput(1, ascii(`x: ${'y'.repeat(3000)}`), long.identity, long.work, long.hooks); assert.equal(textAt(long.work).length, 2047); assert.equal(long.work[2047], 0);
});

test('submitChatInput 이름 절단: 이름은 40바이트까지만 지역에 쓰되 다음 이름 위치는 전체 길이만큼 전진한다', () => {
    const long = 'n'.repeat(45); const r = rig({ subscribers: { [long.slice(0, 40)]: 77 } }); submitChatInput(1, ascii(`${long} next: body`), r.identity, r.work, r.hooks);
    assert.deepEqual(r.log.filter((entry) => entry[0] === 'resolve').map((entry) => entry[1].length), [40, 4]); assert.equal(getDword(r.manager, 0), 0);
    assert.equal(r.identity[48], 0); assert.equal(r.identity[8 + 40], 0);
});

test('submitChatInput 본문 캡처와 오류: 본문/접두는 2048바이트까지, 작업 공간이 4097 미만이면 RangeError, 필수 장치는 도달한 위치에서만 Error 다', () => {
    const r = rig({ subscribers: { a: 1 } }); submitChatInput(1, ascii(`a: ${'z'.repeat(3000)}`), r.identity, r.work, r.hooks); assert.equal(textAt(r.work).length, 2049); assert.equal(r.work[2048], 0x61); assert.equal(r.work[2049], 0); // 접두 캡처가 본문 상한 NUL 을 덮어쓰는 원문 효과다.
    assert.throws(() => submitChatInput(1, ascii('hi'), r.identity, new Uint8Array(4096), r.hooks), { name: 'RangeError', message: /두 인접 캡처 물리 부족/ }); assert.doesNotThrow(() => submitChatInput(1, ascii('hi'), rig().identity, new Uint8Array(4097), rig().hooks));
    for (const name of ['dispatch']) { const q = rig(); delete q.hooks[name]; assert.throws(() => submitChatInput(1, ascii('hi'), q.identity, q.work, q.hooks), /실제 선택 장치 미연결/, name); }
    const noVisible = rig(); delete noVisible.hooks.inputVisible; assert.throws(() => submitChatInput(1, ascii(''), noVisible.identity, noVisible.work, noVisible.hooks), /실제 선택 장치 미연결/); const noResolve = rig(); delete noResolve.hooks.resolve; submitChatInput(1, ascii('hi'), noResolve.identity, noResolve.work, noResolve.hooks);
    const lacking = rig(); delete lacking.hooks.resolve; assert.throws(() => submitChatInput(1, ascii('a: b'), lacking.identity, lacking.work, lacking.hooks), /실제 선택 장치 미연결/);
    // NUL 이 있는 입력은 첫 NUL 앞만 쓴다.
    const nul = rig(); submitChatInput(1, Uint8Array.of(0x68, 0x69, 0, 0x7a), nul.identity, nul.work, nul.hooks); assert.equal(textAt(nul.work), 'hi');
});

test('경계: 관리자 물리 크기(정확히 20바이트는 성공, 19바이트는 마스크 store 에서 RangeError, 14바이트는 직접 표시 store 에서 RangeError)', () => {
    const exact = rig(); const original = exact.hooks.memory; exact.hooks.memory = (token) => (token === 1 ? exact.manager.subarray(0, 20) : original(token)); submitChatInput(1, ascii('hi'), exact.identity, exact.work, exact.hooks); assert.equal(getDword(exact.manager, 16), 0x20);
    const short = rig(); const raw = short.hooks.memory; short.hooks.memory = (token) => (token === 1 ? short.manager.subarray(0, 19) : raw(token)); assert.throws(() => submitChatInput(1, ascii('hi'), short.identity, short.work, short.hooks), { name: 'RangeError', message: /store의 실제 물리 범위/ });
    const tiny = rig(); const base = tiny.hooks.memory; tiny.hooks.memory = (token) => (token === 1 ? tiny.manager.subarray(0, 14) : base(token)); assert.throws(() => submitChatInput(1, ascii('hi'), tiny.identity, tiny.work, tiny.hooks), RangeError);
});

test('경계: 접두 캡처 2048바이트 상한(정확히 상한까지 쓰고 한 칸 넘는 입력도 거부하지 않음), 홀수 간격 공백, 붙은 본문, 같은 위치의 첫 이름', () => {
    const r = rig(); assert.doesNotThrow(() => submitChatInput(1, ascii(`${'a'.repeat(2100)}: x`), r.identity, r.work, r.hooks)); assert.equal(r.work[2048 + 2047], 0x61); assert.equal(r.work[2048 + 2048], 0);
    // 선행 공백 한 칸과 콜론 앞뒤 공백 한 칸씩(홀수 간격)도 건너뛴다.
    const odd = rig({ subscribers: { alice: 5 } }); submitChatInput(1, ascii(' alice : bob'), odd.identity, odd.work, odd.hooks); assert.deepEqual(odd.log.filter((entry) => entry[0] === 'resolve'), [['resolve', 'alice']]); assert.equal(textAt(odd.work), 'bob');
    const single = rig(); submitChatInput(1, ascii(' hi'), single.identity, single.work, single.hooks); assert.equal(textAt(single.work), 'hi');
    // 이름 사이의 한 칸 공백(홀수 위치)에서 이름을 끊는다.
    const split = rig({ subscribers: { a: 1, b: 2 } }); submitChatInput(1, ascii('a b: x'), split.identity, split.work, split.hooks); assert.deepEqual(split.log.filter((entry) => entry[0] === 'resolve').map((entry) => entry[1]), ['a', 'b']);
    // 콜론 바로 뒤의 본문은 한 글자도 잃지 않는다.
    const tight = rig({ subscribers: { alice: 5 } }); submitChatInput(1, ascii('alice:hi'), tight.identity, tight.work, tight.hooks); assert.equal(textAt(tight.work), 'hi'); const space = rig({ subscribers: { alice: 5 } }); submitChatInput(1, ascii('alice: hi'), space.identity, space.work, space.hooks); assert.equal(textAt(space.work), 'hi');
});

test('경계: 채널 이름 비교의 ASCII 대소문자 접기(Z/z 포함, @ [ 는 접지 않음), 앞/가운데 글자 불일치, 인덱스 0 의 일치', () => {
    const matches = (input, translated, key = 'BATTLE') => { const r = rig(); r.hooks.translate = (value) => (value === key ? translated : 'NO-MATCH'); submitChatInput(1, ascii(`${input}: x`), r.identity, r.work, r.hooks); return getDword(r.manager, 16) === (key === 'BATTLE' ? 8 : 0x20) && getDword(r.manager, 12) === 0 && !textAt(r.work).includes(':'); };
    assert.equal(matches('zed', 'ZED'), true); assert.equal(matches('ZED', 'zed'), true); assert.equal(matches('abc', 'ABC'), true);
    assert.equal(matches('@x', '`x'), false); assert.equal(matches('[x', '{x'), false); assert.equal(matches('@x', '@X'), true); assert.equal(matches('[x', '[X'), true);
    assert.equal(matches('axc', 'abc'), false); assert.equal(matches('bbc', 'abc'), false); assert.equal(matches('abd', 'abc'), false); assert.equal(matches('ab', 'abc'), false);
    // 인덱스 0(빈 키)에 번역이 이름과 같으면 마스크 비트 1 이다.
    const zero = rig(); zero.hooks.translate = (key) => (key === '' ? 'zzz' : 'NO'); submitChatInput(1, ascii('zzz: x'), zero.identity, zero.work, zero.hooks); assert.equal(getDword(zero.manager, 16), 1); assert.equal(getDword(zero.manager, 12), 0);
});

test('경계: 식별자 지역은 이름마다 +0/+4/+48 을 0 으로 지우고(모드 +52 는 한 번 0 으로 초기화), 알려지지 않은 이름의 직접 표시 변경', () => {
    const r = rig({ subscribers: { alice: 11, bob: 22 } }); r.identity.fill(0x44); submitChatInput(1, ascii('alice bob: x'), r.identity, r.work, r.hooks);
    assert.deepEqual(r.hooks.fields, [[0, 0, 0, 0], [0, 0, 0, 0]]); assert.equal(getDword(r.identity, 4), 22); assert.equal(getDword(r.identity, 48), 0x1234); assert.equal(getDword(r.identity, 52), 0);
});

test('addChatRecipient 경계: count 1 의 첫 칸 중복, count 뒤 남은 값은 중복이 아님, count 2/capacity 3 은 확장하지 않고 capacity 가 count 보다 작아도 capacity 만큼만 복사한다', () => {
    const first = rig({ count: 1 }); putDword(first.memory.get(3), 0, 11); addChatRecipient(1, 11, first.hooks); assert.equal(getDword(first.manager, 0), 1);
    const stale = rig({ count: 2, capacity: 8 }); putDword(stale.memory.get(3), 0, 1); putDword(stale.memory.get(3), 4, 2); putDword(stale.memory.get(3), 8, 9); addChatRecipient(1, 9, stale.hooks); assert.deepEqual(list(stale.manager, stale.memory), [1, 2, 9]);
    const nogrow = rig({ count: 2, capacity: 3 }); addChatRecipient(1, 5, nogrow.hooks); assert.deepEqual(nogrow.log, []); assert.equal(getDword(nogrow.manager, 4), 3);
    const three = rig({ count: 3, capacity: 3 }); for (const [index, value] of [7, 8, 9].entries()) putDword(three.memory.get(3), index * 4, value); addChatRecipient(1, 10, three.hooks); assert.deepEqual(list(three.manager, three.memory), [7, 8, 9, 10]);
    const small = rig({ count: 3, capacity: 2 }); for (const [index, value] of [7, 8, 77].entries()) putDword(small.memory.get(3), index * 4, value); addChatRecipient(1, 9, small.hooks);
    const fresh = small.memory.get(getDword(small.manager, 8)); assert.deepEqual([getDword(fresh, 0), getDword(fresh, 4), getDword(fresh, 8), getDword(fresh, 12)], [7, 8, 0, 9]); assert.deepEqual([getDword(small.manager, 0), getDword(small.manager, 4)], [4, 34]);
});
