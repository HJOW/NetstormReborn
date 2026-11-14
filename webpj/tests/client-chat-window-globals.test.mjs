// 기준선 소스의 장기 상태/문자열 표 계약이다. 전용 PE fixture나 실제 창 실행은 없다. (2026-11-14 WB04-E453)
import test from 'node:test';
import assert from 'node:assert/strict';
import { ClientChatWindowGlobals } from '../www/core/client-chat-window-globals.js';
import { ChatWindowState } from '../www/ui/chat-window.js';
import { ChatNameCompletionState } from '../www/ui/chat-name-completion.js';
import { StyleTextBufferState } from '../www/ui/style-text-buffer.js';
import { relayChatPacket } from '../www/ui/raw-chat-relay.js';
import { AllianceRequestState } from '../www/core/raw-player-participation.js';

test('최초 Client 상태는 실제 하위 owner와 BSS 0이며 getter는 같은 참조를 반환한다', () => {
    const owner = new ClientChatWindowGlobals(); assert.ok(owner.window instanceof ChatWindowState); assert.ok(owner.log instanceof StyleTextBufferState); assert.ok(owner.completion instanceof ChatNameCompletionState);
    assert.deepEqual([owner.logFile, owner.receivedMask, owner.newsTime, owner.inputLineOff, owner.uiOwner], [0, 0, 0, 0, null]); assert.equal(owner.nextString, 0x71000000);
    assert.strictEqual(owner.window, owner.window); assert.strictEqual(owner.log, owner.log); assert.strictEqual(owner.completion, owner.completion); assert.deepEqual(owner.ignored, []);
    assert.ok(owner.allianceRequests instanceof AllianceRequestState); assert.equal(owner.newsKey.length, 128); assert.ok(owner.newsKey.every((value) => value === 0));
    const requests = owner.allianceRequests; assert.equal(requests.counters.length, 9); assert.equal(requests.clocks.length, 9); assert.deepEqual([requests.initialized, requests.windowSeconds, requests.limit], [0, 60, 3]);
    assert.ok(requests.counters.every((value) => value === 0)); assert.ok(requests.clocks.every((pair) => pair.length === 2 && pair[0] === 0 && pair[1] === 0));
    assert.strictEqual(requests.counters.buffer, requests.clocks[8].buffer); assert.equal(requests.clocks[8].byteOffset, 100); assert.equal(requests.counters.buffer.byteLength, 112);
});

test('Chat Gump 토큰 교체가 로그/자동완성/무시/문자열/받은 마스크의 장기 주소와 값을 지우지 않는다', () => {
    const owner = new ClientChatWindowGlobals(); const log = owner.log; const completion = owner.completion; const ignored = owner.ignored; const token = owner.store('same');
    owner.ignored.push(7); owner.receivedMask = 0x123; owner.newsTime = 17; owner.logFile = 4; owner.inputLineOff = 2; owner.uiOwner = {};
    const requests = owner.allianceRequests; const key = owner.newsKey; requests.counters[2] = 9; requests.clocks[2][1] = 77; requests.initialized = 0x100000001; key[3] = 255;
    owner.window.current = 11; owner.window.current = 0; owner.window.current = 12;
    assert.strictEqual(owner.log, log); assert.strictEqual(owner.completion, completion); assert.strictEqual(owner.ignored, ignored); assert.deepEqual(ignored, [7]); assert.equal(owner.receivedMask, 0x123); assert.equal(owner.newsTime, 17); assert.equal(owner.store('same'), token);
    assert.throws(() => { owner.log = new StyleTextBufferState(); }, TypeError);
    assert.strictEqual(owner.allianceRequests, requests); assert.strictEqual(owner.newsKey, key); assert.equal(requests.counters[2], 9); assert.equal(requests.clocks[2][1], 77); assert.equal(requests.initialized, 1); assert.equal(new DataView(requests.counters.buffer).getUint32(108, true), 1); assert.equal(key[3], 255);
});

test('문자열 표는 빈 문자열·NUL 뒤 바이트·동일 내용의 별칭과 토큰 증가를 보존한다', () => {
    const owner = new ClientChatWindowGlobals(); const first = owner.store(''); const second = owner.store('A\0tail'); const third = owner.store('A');
    assert.deepEqual([first, second, third, owner.nextString], [0x71000000, 0x71000001, 0x71000002, 0x71000003]); assert.equal(owner.store('A\0tail'), second); assert.equal(owner.strings.get(first), ''); assert.equal(owner.strings.get(second), 'A\0tail'); assert.equal(owner.stringTokens.get('A'), third);
    assert.equal(owner.strings.size, 3); assert.equal(owner.stringTokens.size, 3);
});

test('마지막 유효 토큰 다음은 새 값만 거부하며 재조회와 선행 표/카운터는 그대로다', () => {
    const owner = new ClientChatWindowGlobals(); const token = owner.store('existing'); owner.nextString = 0x7ffffffe;
    assert.equal(owner.store('last'), 0x7ffffffe); assert.equal(owner.nextString, 0x7fffffff); assert.equal(owner.store('existing'), token);
    assert.throws(() => owner.store('overflow'), /소진/); assert.equal(owner.nextString, 0x7fffffff); assert.equal(owner.strings.size, 2); assert.equal(owner.stringTokens.has('overflow'), false);
});

test('실제 채팅 중계가 같은 owner의 마스크를 직접 대여하고 무시 목록과 공유할 수 있다', () => {
    const owner = new ClientChatWindowGlobals(); const packet = new Uint8Array(20); packet[10] = 1; packet[12] = 4; packet.set([48, 51, 50, 10], 16); let seen;
    const mask = { get value() { return owner.receivedMask; }, set value(value) { owner.receivedMask = value; } };
    relayChatPacket(0, packet, mask, { address: () => new Uint8Array(20), read: (at) => at === 0x594fbc ? 1 : 0, receive: () => { seen = owner.receivedMask; owner.ignored.push(9); } });
    assert.equal(seen, 32); assert.equal(owner.receivedMask, 32); assert.deepEqual(owner.ignored, [9]); assert.equal(packet[19], 0);
});
