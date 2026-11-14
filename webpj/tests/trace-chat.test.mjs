// 공통 로그 → 현재 Chat 전달(TraceChat)을 기준선 소스 계약으로 검증한다. (2026-11-14 WB04-E437)
// cpppj에도 직접 시험이 없어 원본 직접 관측 수치가 아니다. 순서/경계는 기준선 소스(TraceChat.cpp)에서 옮겼다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { forwardTraceToChat } from '../www/ui/trace-chat.js';

// 모든 호출을 기록하는 장치 묶음이다. color/currentChat 는 호출마다 현재 값을 읽는다.
function rig({ color = 0x7f7f7f, chat = 0x1234 } = {}) {
    const log = []; const state = { color, chat };
    return { log, state, hooks: { color: () => { log.push(['color']); return state.color; }, currentChat: () => { log.push(['chat']); return state.chat; }, append: (root, text, context) => log.push(['append', root, text, Array.from(context), context instanceof Uint32Array]) } };
}

test('forwardTraceToChat: enabled 가 0 이면 아무 장치도 읽지 않고 서식도 처리하지 않는다(장치 없음/잘못된 서식도 통과)', () => {
    const r = rig(); forwardTraceToChat(0, 'abc', r.hooks); forwardTraceToChat(0, '%d', {}); forwardTraceToChat(0x100000000, 'x', {}); assert.deepEqual(r.log, []);
});

test('forwardTraceToChat: 비영이면 색→(서식)→현재 Chat 순으로 읽고 문맥 [0,1,색,0,0,0,1,1]·완성 문자열로 append 한다', () => {
    const r = rig(); forwardTraceToChat(1, 'hello', r.hooks);
    assert.deepEqual(r.log, [['color'], ['chat'], ['append', 0x1234, 'hello', [0, 1, 0x7f7f7f, 0, 0, 0, 1, 1], true]]);
    // enabled 는 DWORD 라 2 이든 0xffffffff 이든 켜진 것이다. 색은 DWORD 로 읽는다.
    const wide = rig({ color: -1 }); forwardTraceToChat(0xffffffff, 'a', wide.hooks); assert.deepEqual(wide.log[2].slice(3, 4), [[0, 1, 0xffffffff, 0, 0, 0, 1, 1]]);
});

test('forwardTraceToChat: 현재 Chat 이 0 이면 서식은 처리하되 append 하지 않고, 서식 오류는 root 0 이어도 그대로 난다', () => {
    const r = rig({ chat: 0 }); forwardTraceToChat(1, 'x', r.hooks); assert.deepEqual(r.log, [['color'], ['chat']]);
    const bad = rig({ chat: 0 }); assert.throws(() => forwardTraceToChat(1, 'a%d', bad.hooks), { name: 'Error', message: /가변인자/ }); assert.deepEqual(bad.log, [['color']]);
});

test('forwardTraceToChat: 서식은 첫 NUL 앞까지만 보며 %% 는 % 한 글자, 뒤가 %% 가 아닌 % 나 끝의 % 는 미제공 가변인자로 거부한다', () => {
    const r = rig(); forwardTraceToChat(1, 'ab\0cd%d', r.hooks); assert.equal(r.log.at(-1)[2], 'ab');
    // 선두 NUL 은 빈 서식이다(뒤 % 도 읽지 않는다).
    forwardTraceToChat(1, '\0abc%d', r.hooks); assert.equal(r.log.at(-1)[2], '');
    forwardTraceToChat(1, '100%% done %%', r.hooks); assert.equal(r.log.at(-1)[2], '100% done %');
    forwardTraceToChat(1, '%%%%', r.hooks); assert.equal(r.log.at(-1)[2], '%%'); forwardTraceToChat(1, '', r.hooks); assert.equal(r.log.at(-1)[2], '');
    for (const text of ['%', 'a%', '%d', '%%%', '%s %%', '%%%d']) assert.throws(() => forwardTraceToChat(1, text, rig().hooks), { name: 'Error', message: /가변인자/ }, text);
    // 바이트 배열도 같은 규칙이며 확장 바이트는 그대로 둔다.
    forwardTraceToChat(1, Uint8Array.from([0x41, 0x25, 0x25, 0xe9, 0x00, 0x42]), r.hooks); assert.equal(r.log.at(-1)[2], 'A%\xe9');
});

test('forwardTraceToChat: 출력은 1023자까지이며 1024번째 글자에서 RangeError 다(%% 도 한 글자로 센다)', () => {
    const r = rig(); forwardTraceToChat(1, 'a'.repeat(1023), r.hooks); assert.equal(r.log.at(-1)[2].length, 1023);
    assert.throws(() => forwardTraceToChat(1, 'a'.repeat(1024), rig().hooks), RangeError);
    forwardTraceToChat(1, `${'a'.repeat(1022)}%%`, r.hooks); assert.equal(r.log.at(-1)[2].length, 1023); assert.equal(r.log.at(-1)[2].at(-1), '%');
    assert.throws(() => forwardTraceToChat(1, `${'a'.repeat(1023)}%%`, rig().hooks), RangeError);
    // 첫 NUL 뒤 긴 입력은 입력이 아니다. 범위 오류는 append 이전이다.
    forwardTraceToChat(1, `ok\0${'b'.repeat(5000)}`, r.hooks); assert.equal(r.log.at(-1)[2], 'ok');
    const over = rig(); assert.throws(() => forwardTraceToChat(1, 'a'.repeat(1030), over.hooks), RangeError); assert.deepEqual(over.log, [['color']]);
});

test('forwardTraceToChat: 장치 누락은 도달한 위치에서만 Error 이며 앞선 효과는 남는다', () => {
    assert.throws(() => forwardTraceToChat(1, 'a', {}), { name: 'Error', message: /선택 장치/ });
    const noChat = rig(); assert.throws(() => forwardTraceToChat(1, 'a', { color: noChat.hooks.color }), /선택 장치/); assert.deepEqual(noChat.log, [['color']]);
    // 서식 오류가 현재 Chat 조회보다 먼저다.
    const order = rig(); assert.throws(() => forwardTraceToChat(1, '%', { color: order.hooks.color }), /가변인자/);
    const noAppend = rig(); assert.throws(() => forwardTraceToChat(1, 'a', { color: noAppend.hooks.color, currentChat: noAppend.hooks.currentChat }), /선택 장치/); assert.deepEqual(noAppend.log, [['color'], ['chat']]);
    // 현재 Chat 이 0 이면 append 장치가 없어도 통과한다.
    const zero = rig({ chat: 0 }); forwardTraceToChat(1, 'a', { color: zero.hooks.color, currentChat: zero.hooks.currentChat });
    // 색은 서식 처리 전에 읽어 고정하고 Chat 은 서식 처리 뒤에 읽는다.
    const seq = []; forwardTraceToChat(1, 'a', { color: () => { seq.push('color'); return 1; }, currentChat: () => { seq.push('chat'); return 2; }, append: (root, text, context) => seq.push(root, text, context[2]) }); assert.deepEqual(seq, ['color', 'chat', 2, 'a', 1]);
});
