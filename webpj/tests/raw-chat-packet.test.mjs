// Chat 패킷 raw-chat-packet(sendChatPacket·observeChatPacket)을 기준선 소스 계약과 손 계산 바이트로 검증한다.
// cpppj에도 직접 시험이 없어 원본 직접 관측 수치가 아니다. 헤더/이름/본문/수신자 배열의 정확한 바이트와 채널 선택, 한도, 길이 확정을 확인한다. 실제 게임·브라우저 실행이 아니다. (2026-11-14 WB04-E446)
import test from 'node:test';
import assert from 'node:assert/strict';
import { observeChatPacket, sendChatPacket } from '../www/ui/raw-chat-packet.js';

// 바이트 문자열과 DWORD 쓰기 보조 함수다.
const ascii = (value) => Uint8Array.from(Array.from(value, (char) => char.charCodeAt(0) & 0xff));
const putDword = (array, at, value) => { for (let index = 0; index < 4; index += 1) array[at + index] = Math.floor(value / 2 ** (8 * index)) & 0xff; };

// 기록 틀이다. manager 1(+12 직접 표시/+16 flags), 수신자 2(+0 count/+8 배열 토큰), 배열 3 이다. 전역: subscriber 7, 52edf6=0x12, 544c5c=0x1234.
function rig({ count = 1, array = [7], flags = 0, direct = 0, globals = {}, name = 'Bob' } = {}) {
    const log = []; const manager = new Uint8Array(32); const recipients = new Uint8Array(16); const list = new Uint8Array(64); const memory = new Map([[1, manager], [2, recipients], [3, list]]);
    putDword(manager, 12, direct); putDword(manager, 16, flags); putDword(recipients, 0, count); putDword(recipients, 8, 3); array.forEach((value, index) => putDword(list, index * 4, value));
    const table = { 0x540c74: 7, 0x594fbc: 0, 0x594fb8: 0, 0x594fc8: 0, 0x52edf6: 0x12, 0x544c5c: 0x1234, ...globals };
    const state = { name: Uint8Array.from([...ascii(name), 0]) };
    const hooks = {
        memory: (token) => memory.get(token), currentName: () => state.name, read: (address) => { log.push(['read', address]); return table[address] ?? 0; },
        addRecipient: (target, subscriber) => log.push(['add', target, subscriber]), send: (channel, packet) => log.push(['send', channel, Array.from(packet)]),
        log: (level, value) => log.push(['log', level, value]), zeroAddress: () => log.push(['zero']), localReceive: (packet) => log.push(['local', Array.from(packet)]),
    };
    return { hooks, manager, recipients, list, log, table, state };
}
const sends = (log) => log.filter((entry) => entry[0] === 'send');

test('sendChatPacket 비전투: 15바이트 헤더·이름·본문·수신자 배열을 정확한 바이트로 만들어 채널 1 로 보낸다(미작성 +4 는 0xff)', () => {
    const r = rig({ globals: { 0x594fb8: 1 } }); assert.equal(sendChatPacket(1, 2, ascii('hi'), r.hooks), 1);
    const expected = [3, 0x12, 0x34, 0x12, 0xff, 1, 7, 0, 0, 0, 4, 0, 2, 0, 1, 0x42, 0x6f, 0x62, 0, 0x68, 0x69, 7, 0, 0, 0];
    assert.deepEqual(sends(r.log), [['send', 1, expected]]); assert.equal(r.log.some((entry) => entry[0] === 'add'), false);
});

test('sendChatPacket 채널 선택: 594fb8 → 1, 아니면 메타맵 → 11, 아니면 전투 → 9, 모두 0 이면 오류 로그 뒤 영 주소와 지역 수신이며 전역은 순서대로 읽는다', () => {
    const cases = [[{ 0x594fb8: 1, 0x594fc8: 1, 0x594fbc: 1 }, 1], [{ 0x594fc8: 1, 0x594fbc: 1 }, 11], [{ 0x594fc8: 1 }, 11], [{ 0x594fbc: 1 }, 9]];
    for (const [globals, channel] of cases) { const r = rig({ globals }); sendChatPacket(1, 2, ascii('x\0'), r.hooks); assert.deepEqual(sends(r.log).map((entry) => entry[1]), [channel], JSON.stringify(globals)); }
    const r = rig(); sendChatPacket(1, 2, ascii('x'), r.hooks); const tail = r.log.filter((entry) => ['log', 'zero', 'local', 'send'].includes(entry[0]));
    assert.deepEqual(tail.map((entry) => entry[0]), ['log', 'zero', 'local']); assert.deepEqual(tail[0], ['log', 9, 'ERROR: No transceiver on which to chat!\n']); assert.equal(tail[2][1].length, 15 + 4 + 1 + 4);
    const reads = r.log.filter((entry) => entry[0] === 'read').map((entry) => entry[1]).slice(-3); assert.deepEqual(reads, [0x594fb8, 0x594fc8, 0x594fbc]);
});

test('sendChatPacket 전투 본문: flags(+16, 직접 대상이면 |0x100)를 세 자리 이상 십진 앞 0 패딩으로 본문(첫 NUL 앞) 앞에 붙인다', () => {
    const cases = [[5, 0, '005'], [0x105, 0, '261'], [5, 1, '261'], [0x1234, 0, '4660'], [0xffffffff, 0, '0-1'], [0, 0, '000']];
    for (const [flags, direct, prefix] of cases) { const r = rig({ flags, direct, globals: { 0x594fbc: 1, 0x594fb8: 1 } }); sendChatPacket(1, 2, Uint8Array.from([0x68, 0x69, 0, 0x7a]), r.hooks);
        const packet = sends(r.log)[0][2]; const messageLength = packet[12] | (packet[13] << 8); assert.equal(String.fromCharCode(...packet.slice(19, 19 + messageLength)), `${prefix}hi`, prefix); }
    // 비전투는 본문 전체(NUL 포함)를 그대로 복사한다.
    const plain = rig({ globals: { 0x594fb8: 1 } }); sendChatPacket(1, 2, Uint8Array.from([0x68, 0, 0x7a]), plain.hooks); assert.deepEqual(sends(plain.log)[0][2].slice(19, 22), [0x68, 0, 0x7a]);
});

test('sendChatPacket 수신자: 자기 subscriber 가 없으면 추가를 요청하고(추가 뒤 현재 count 를 다시 읽는다) 있으면 요청하지 않는다', () => {
    const missing = rig({ count: 1, array: [5], globals: { 0x594fb8: 1 } }); missing.hooks.addRecipient = (target, subscriber) => { missing.log.push(['add', target, subscriber]); putDword(missing.recipients, 0, 2); putDword(missing.list, 4, subscriber); };
    sendChatPacket(1, 2, ascii('x'), missing.hooks); const packet = sends(missing.log)[0][2]; assert.deepEqual(missing.log.find((entry) => entry[0] === 'add'), ['add', 2, 7]); assert.equal(packet[14], 2); assert.deepEqual(packet.slice(-8), [5, 0, 0, 0, 7, 0, 0, 0]);
    const found = rig({ count: 2, array: [5, 7], globals: { 0x594fb8: 1 } }); sendChatPacket(1, 2, ascii('x'), found.hooks); assert.equal(found.log.some((entry) => entry[0] === 'add'), false); assert.equal(sends(found.log)[0][2][14], 2);
    const none = rig({ count: 0, array: [], globals: { 0x594fb8: 1 } }); sendChatPacket(1, 2, ascii('x'), none.hooks); assert.deepEqual(none.log.find((entry) => entry[0] === 'add'), ['add', 2, 7]);
    // 음수 count 는 부호 있는 비교라 탐색하지 않는다.
    const negative = rig({ count: 0xffffffff, array: [7] }); assert.throws(() => sendChatPacket(1, 2, ascii('x'), negative.hooks), RangeError);
});

test('sendChatPacket 이름: 빈 현재 이름은 ++Nameless++, 이름 크기 0x7fff 까지 허용하고 넘으면 RangeError 이며 이름 NUL 이 없으면 RangeError 다', () => {
    const empty = rig({ name: '', globals: { 0x594fb8: 1 } }); sendChatPacket(1, 2, ascii('x'), empty.hooks); const packet = sends(empty.log)[0][2]; assert.equal(packet[10], 13); assert.equal(String.fromCharCode(...packet.slice(15, 27)), '++Nameless++'); assert.equal(packet[27], 0);
    const biggest = rig({ name: 'n'.repeat(0x7ffe), globals: { 0x594fb8: 1 } }); sendChatPacket(1, 2, ascii('x'), biggest.hooks); assert.equal(sends(biggest.log)[0][2][10] | (sends(biggest.log)[0][2][11] << 8), 0x7fff);
    assert.throws(() => sendChatPacket(1, 2, ascii('x'), rig({ name: 'n'.repeat(0x7fff) }).hooks), { name: 'RangeError', message: /복사 범위 부족/ });
    const noNul = rig(); noNul.state.name = ascii('Bob'); assert.throws(() => sendChatPacket(1, 2, ascii('x'), noNul.hooks), { name: 'RangeError', message: /NUL 없음/ });
});

test('sendChatPacket 한도: 본문 0xffff 까지 허용하고 넘으면 RangeError, 수신자 배열이 count×4 보다 작으면 RangeError 이며 발송 전에 거부한다', () => {
    const ok = rig({ globals: { 0x594fb8: 1 } }); sendChatPacket(1, 2, new Uint8Array(0xffff), ok.hooks); const packet = sends(ok.log)[0][2]; assert.equal(packet[12] | (packet[13] << 8), 0xffff);
    const big = rig({ globals: { 0x594fb8: 1 } }); assert.throws(() => sendChatPacket(1, 2, new Uint8Array(0x10000), big.hooks), RangeError); assert.deepEqual(sends(big.log), []);
    const many = rig({ count: 17, array: [7], globals: { 0x594fb8: 1 } }); assert.throws(() => sendChatPacket(1, 2, ascii('x'), many.hooks), RangeError); assert.deepEqual(sends(many.log), []);
    const full = rig({ count: 16, array: [7], globals: { 0x594fb8: 1 } }); sendChatPacket(1, 2, ascii('x'), full.hooks); assert.equal(sends(full.log)[0][2][14], 16);
    // 수신자 수 BYTE 는 절단된다(256 → 0).
    const wrap = rig({ count: 256, array: [7], globals: { 0x594fb8: 1 } }); wrap.hooks.memory = (token) => (token === 3 ? new Uint8Array(1024) : rig().hooks.memory(token)); const keep = wrap.hooks.memory; wrap.hooks.memory = (token) => (token === 2 ? wrap.recipients : token === 3 ? new Uint8Array(1024).fill(7) : keep(token));
    sendChatPacket(1, 2, ascii('x'), wrap.hooks); assert.equal(sends(wrap.log)[0][2][14], 0);
});

test('sendChatPacket 필수 장치: 도달한 위치에서만 Error 이고 채널 하나가 있으면 로그/영 주소/지역 수신은 필요 없다', () => {
    for (const name of ['memory', 'currentName', 'read']) { const r = rig(); delete r.hooks[name]; assert.throws(() => sendChatPacket(1, 2, ascii('x'), r.hooks), { name: 'Error', message: /실제 장치 미연결/ }, name); }
    const addless = rig({ array: [5] }); delete addless.hooks.addRecipient; assert.throws(() => sendChatPacket(1, 2, ascii('x'), addless.hooks), /실제 장치 미연결/);
    const sendless = rig({ globals: { 0x594fb8: 1 } }); delete sendless.hooks.send; assert.throws(() => sendChatPacket(1, 2, ascii('x'), sendless.hooks), /실제 장치 미연결/);
    const ok = rig({ globals: { 0x594fb8: 1 } }); delete ok.hooks.log; delete ok.hooks.zeroAddress; delete ok.hooks.localReceive; sendChatPacket(1, 2, ascii('x'), ok.hooks);
    for (const name of ['log', 'zeroAddress', 'localReceive']) { const r = rig(); delete r.hooks[name]; assert.throws(() => sendChatPacket(1, 2, ascii('x'), r.hooks), /실제 장치 미연결/, name); }
});

test('observeChatPacket: 송신이 0 이면 아무것도 하지 않고, 아니면 +2 WORD 에 수신자 수×4+본문 크기+이름 크기+15 를 쓰며 이름 0xffff/수신자 0xff 는 진단 0x81d/0x81f 를 알리고 계속한다', () => {
    const packet = Uint8Array.from([3, 0, 0xaa, 0xbb, 0xff, 1, 7, 0, 0, 0, 4, 0, 2, 0, 1, 0, 0, 0, 0]); const copy = Uint8Array.from(packet);
    observeChatPacket(packet, 0, () => { throw new Error('호출 불가'); }); assert.deepEqual(packet, copy);
    observeChatPacket(packet, 1, undefined); assert.deepEqual([packet[2], packet[3]], [25, 0]);
    const calls = []; const wide = Uint8Array.from(packet); wide[10] = 0xff; wide[11] = 0xff; wide[14] = 0xff; observeChatPacket(wide, 5, (value) => calls.push(value)); assert.deepEqual(calls, [0x81d, 0x81f]);
    assert.equal(wide[2] | (wide[3] << 8), (0xff * 4 + 2 + 0xffff + 15) & 0xffff); assert.throws(() => observeChatPacket(Uint8Array.from(wide), 1, undefined), { name: 'Error', message: /실제 장치 미연결/ });
    const onlyName = Uint8Array.from(packet); onlyName[10] = 0xff; onlyName[11] = 0xff; observeChatPacket(onlyName, 1, (value) => calls.push(value)); assert.deepEqual(calls.slice(2), [0x81d]);
    assert.throws(() => observeChatPacket(new Uint8Array(14), 1, () => {}), { name: 'RangeError', message: /읽기의 실제 물리 부족/ }); observeChatPacket(new Uint8Array(15), 0, () => {});
});

test('경계: 전투 직접 대상 비트 0x100 정확 값, 길이 WORD 의 최소 4', () => {
    for (const [flags, expected] of [[0, '256'], [2, '258'], [0x101, '257']]) { const r = rig({ flags, direct: 1, globals: { 0x594fbc: 1, 0x594fb8: 1 } }); sendChatPacket(1, 2, Uint8Array.from([0x68, 0]), r.hooks); const packet = sends(r.log)[0][2]; assert.equal(String.fromCharCode(...packet.slice(19, 19 + (packet[12] | (packet[13] << 8)))), expected + 'h', String(flags)); }
    for (const [value, word] of [[0, 4], [2, 4], [4, 4], [5, 5], [0x1ffff, 0xffff]]) { const r = rig({ globals: { 0x594fb8: 1, 0x544c5c: value } }); sendChatPacket(1, 2, ascii('x'), r.hooks); const packet = sends(r.log)[0][2]; assert.equal(packet[2] | (packet[3] << 8), word, String(value)); }
});

test('observeChatPacket 경계: 이름 크기·본문 크기·수신자 수의 정확한 폭만 읽고 +2 WORD 만 쓴다(+4 는 보존, 이름 첫 BYTE 가 있는 실제 패킷)', () => {
    const real = rig({ globals: { 0x594fb8: 1 } }); sendChatPacket(1, 2, ascii('hi'), real.hooks); const packet = Uint8Array.from(sends(real.log)[0][2]); packet[2] = 0; packet[3] = 0;
    const calls = []; observeChatPacket(packet, 1, (value) => calls.push(value)); assert.deepEqual(calls, []); assert.deepEqual([packet[2], packet[3], packet[4]], [25, 0, 0xff]);
    // 수신자 수가 0xff 이고 이름 첫 BYTE(+15)가 0 이 아니어도 0x81f 진단을 알린다.
    const wide = Uint8Array.from(packet); wide[14] = 0xff; const hit = []; observeChatPacket(wide, 1, (value) => hit.push(value)); assert.deepEqual(hit, [0x81f]);
    // 이름 크기 0xffff 도 인접 BYTE 와 무관하게 알린다.
    const name = Uint8Array.from(packet); name[10] = 0xff; name[11] = 0xff; const hit2 = []; observeChatPacket(name, 1, (value) => hit2.push(value)); assert.deepEqual(hit2, [0x81d]); assert.equal(name[4], 0xff);
});
