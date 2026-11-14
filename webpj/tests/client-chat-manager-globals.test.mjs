// Chat manager 전역 소유자 ClientChatManagerGlobals(현재 상자·서식 캡처·manager/수신자 할당·반납 규칙)와 세 훅 공장을 기준선 소스 계약으로 검증한다.
// cpppj에도 직접 시험이 없어 원본 직접 관측 수치가 아니다. 실제 gating 은 기존 raw-chat-manager-prefix·raw-chat-submit 과 연결해 확인한다. 실제 게임·브라우저 실행이 아니다. (2026-11-14 WB04-E447)
import test from 'node:test';
import assert from 'node:assert/strict';
import { ClientChatManagerGlobals, makeClientChatManagerHooks, makeClientChatPacketHooks, makeClientChatSubmitHooks } from '../www/core/client-chat-manager-globals.js';
import { CHAT_MANAGER_BUFFER_BYTES, CHAT_MANAGER_BYTES } from '../www/core/raw-chat-manager-prefix.js';
import { CHAT_FORMAT_CAPTURE_BYTES } from '../www/ui/raw-chat-format.js';
import { submitChatInput } from '../www/ui/raw-chat-submit.js';

// 바이트 DWORD 보조 함수다.
const getDword = (bytes, at) => bytes[at] + bytes[at + 1] * 256 + bytes[at + 2] * 65536 + bytes[at + 3] * 16777216;
const putDword = (bytes, at, value) => { for (let index = 0; index < 4; index += 1) bytes[at + index] = Math.floor(value / 2 ** (8 * index)) & 0xff; };

test('current/formatCapture: 현재 토큰 상자는 최초 0 이고 같은 객체를 돌려주며, 서식 캡처는 81바이트 0 이고 같은 뷰를 계속 빌려 준다', () => {
    const owner = new ClientChatManagerGlobals(); assert.equal(owner.current().value, 0); assert.equal(owner.current(), owner.current()); owner.current().value = 7; assert.equal(owner.current().value, 7);
    const capture = owner.formatCapture(); assert.equal(capture.length, 81); assert.equal(CHAT_FORMAT_CAPTURE_BYTES, 81); assert.ok(capture.every((value) => value === 0)); capture[3] = 9; assert.equal(owner.formatCapture()[3], 9); assert.equal(owner.formatCapture(), capture);
});

test('allocate: manager 0xa98 / 내부 0x80 만 허용하고 그 밖은 TypeError 이며 1 부터 늘어나는 비영 토큰을 주고 current 는 바꾸지 않는다', () => {
    const owner = new ClientChatManagerGlobals(); const first = owner.allocate(CHAT_MANAGER_BYTES); const second = owner.allocate(CHAT_MANAGER_BUFFER_BYTES); assert.deepEqual([first, second], [1, 2]); assert.equal(owner.current().value, 0);
    assert.equal(owner.bytes(first).length, 0xa98); assert.equal(owner.bytes(second).length, 0x80); assert.equal(owner.bytes(first), owner.bytes(first));
    for (const size of [0, 1, 0x7f, 0x81, 0xa97, 0xa99, 4096]) assert.throws(() => owner.allocate(size), { name: 'TypeError', message: /할당 크기 오류/ }, String(size));
    assert.equal(owner.allocate(0xa98), 3);
});

test('allocateRecipientBuffer: 양수이며 4 의 배수인 BYTE 폭만 허용하고 TypeError 아니면 반납 가능한 토큰을 준다', () => {
    const owner = new ClientChatManagerGlobals(); for (const size of [0, 1, 2, 3, 5, 129]) assert.throws(() => owner.allocateRecipientBuffer(size), { name: 'TypeError', message: /DWORD 폭 오류/ }, String(size));
    const token = owner.allocateRecipientBuffer(132); assert.equal(token, 1); assert.equal(owner.bytes(token).length, 132); const small = owner.allocateRecipientBuffer(4); assert.equal(owner.bytes(small).length, 4);
});

test('releaseRecipientBuffer: 0 은 허용하고 수신자 저장소(내부 128바이트 배열 포함)만 반납하며 manager 와 이미 반납된/미등록 토큰은 RangeError 이다', () => {
    const owner = new ClientChatManagerGlobals(); const manager = owner.allocate(0xa98); const internal = owner.allocate(0x80); const recipients = owner.allocateRecipientBuffer(132);
    owner.releaseRecipientBuffer(0); assert.throws(() => owner.releaseRecipientBuffer(manager), { name: 'RangeError', message: /반납의 실제 할당 없음/ }); owner.releaseRecipientBuffer(internal); assert.throws(() => owner.bytes(internal), RangeError);
    owner.releaseRecipientBuffer(recipients); assert.throws(() => owner.bytes(recipients), { name: 'RangeError', message: /할당 토큰 없음/ }); assert.throws(() => owner.releaseRecipientBuffer(recipients), RangeError); assert.throws(() => owner.releaseRecipientBuffer(99), RangeError);
    assert.equal(owner.bytes(manager).length, 0xa98);
});

test('bytes: 0/미등록 토큰은 RangeError 이고 새 current 교체 뒤에도 이전 manager 는 살아 있다(재초기화가 free 하지 않음)', () => {
    const owner = new ClientChatManagerGlobals(); assert.throws(() => owner.bytes(0), { name: 'RangeError', message: /할당 토큰 없음/ }); assert.throws(() => owner.bytes(5), RangeError);
    const old = owner.allocate(0xa98); owner.bytes(old)[10] = 0x77; owner.current().value = old; const fresh = owner.allocate(0xa98); owner.current().value = fresh;
    assert.equal(owner.bytes(old)[10], 0x77); assert.notEqual(old, fresh); assert.equal(owner.bytes(fresh)[10], 0);
});

test('할당 토큰: 잘못된 크기는 토큰을 소모하지 않는다', () => {
    const owner = new ClientChatManagerGlobals(); owner.allocate(0x80); assert.throws(() => owner.allocate(0x81), TypeError); assert.equal(owner.allocate(0x80), 2);
});

test('makeClientChatManagerHooks: 같은 소유자의 allocate/memory 와 기존 CRT 복사를 연결하고 소유자가 없으면 TypeError 다', () => {
    const owner = new ClientChatManagerGlobals(); const hooks = makeClientChatManagerHooks(owner); const token = hooks.allocate(0xa98); assert.equal(token, 1); assert.equal(hooks.memory(token), owner.bytes(token)); assert.equal(typeof hooks.copyFormat, 'function');
    assert.throws(() => hooks.allocate(5), TypeError); assert.throws(() => makeClientChatManagerHooks(null), { name: 'TypeError', message: /Chatmanager 실제 소유자 없음/ });
    for (const factory of [makeClientChatSubmitHooks, makeClientChatPacketHooks]) assert.throws(() => factory(undefined), TypeError);
});

test('makeClientChatSubmitHooks/PacketHooks: 제출의 수신자 확장과 송신의 자기 추가가 같은 소유자 할당에서 확장·반납된다', () => {
    const owner = new ClientChatManagerGlobals(); const manager = owner.allocate(0xa98); const array = owner.allocateRecipientBuffer(8); const bytes = owner.bytes(manager); putDword(bytes, 0, 2); putDword(bytes, 4, 2); putDword(bytes, 8, array);
    putDword(owner.bytes(array), 0, 5); putDword(owner.bytes(array), 4, 6);
    const submit = makeClientChatSubmitHooks(owner); assert.equal(submit.memory(manager), bytes); const packet = makeClientChatPacketHooks(owner); assert.equal(packet.memory(manager), bytes);
    packet.addRecipient(manager, 7); assert.deepEqual([getDword(bytes, 0), getDword(bytes, 4)], [3, 34]); const grown = getDword(bytes, 8); assert.notEqual(grown, array); assert.throws(() => owner.bytes(array), RangeError);
    assert.deepEqual([getDword(owner.bytes(grown), 0), getDword(owner.bytes(grown), 4), getDword(owner.bytes(grown), 8)], [5, 6, 7]); assert.equal(owner.bytes(grown).length, 2 * 4 + 128);
    packet.addRecipient(manager, 6); assert.equal(getDword(bytes, 0), 3);
    // 제출 훅의 allocate/release 는 같은 소유자의 수신자 저장소를 직접 쓴다.
    const direct = submit.allocate(16); assert.equal(owner.bytes(direct).length, 16); submit.release(direct); assert.throws(() => owner.bytes(direct), RangeError); assert.throws(() => submit.allocate(3), TypeError);
});

test('제출 연결: 실제 submitChatInput 이 소유자 할당 위에서 동작한다(수신자 배열 확장 포함)', () => {
    const owner = new ClientChatManagerGlobals(); const manager = owner.allocate(0xa98); const array = owner.allocateRecipientBuffer(4); const bytes = owner.bytes(manager); putDword(bytes, 4, 1); putDword(bytes, 8, array);
    const log = []; const hooks = { ...makeClientChatSubmitHooks(owner), resolve: (record) => { putDword(record, 4, 40 + log.length); log.push('r'); return true; }, translate: (key) => key, records: () => new Uint8Array(4800), inputVisible: () => log.push('v'), dispatch: () => log.push('d'), assertion: () => log.push('a') };
    submitChatInput(manager, Uint8Array.from([0x61, 0x20, 0x62, 0x3a, 0x20, 0x78, 0]), new Uint8Array(60), owner.bytes(owner.allocateRecipientBuffer(4100)), hooks);
    assert.deepEqual(log, ['r', 'r', 'd']); assert.equal(getDword(bytes, 0), 2); assert.equal(getDword(bytes, 4), 33); const grown = owner.bytes(getDword(bytes, 8)); assert.deepEqual([getDword(grown, 0), getDword(grown, 4)], [40, 41]); assert.throws(() => owner.bytes(array), RangeError);
});

test('호스트 할당 실패: 배열을 만들 수 없으면 0 을 돌려주고 토큰/표를 바꾸지 않는다', () => {
    const owner = new ClientChatManagerGlobals(); const Real = globalThis.Uint8Array;
    globalThis.Uint8Array = class extends Real { constructor(...args) { if (args[0] === 132) throw new RangeError('Array buffer allocation failed'); super(...args); } };
    try { assert.equal(owner.allocateRecipientBuffer(132), 0); assert.equal(owner.allocateRecipientBuffer(8), 1); } finally { globalThis.Uint8Array = Real; }
    assert.equal(owner.allocateRecipientBuffer(132), 2); assert.equal(owner.bytes(2).length, 132);
});
