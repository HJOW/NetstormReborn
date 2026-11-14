// 원문 단독 자기 중계 132행의 전체 패킷/마스크/효과 순서와 실제 분배표 연결을 검증한다. (2026-11-14 WB04-E449)
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import { relayChatPacket, registerChatPacket } from '../www/ui/raw-chat-relay.js';
import { RawPacketDispatch, PacketConnection } from '../www/core/raw-packet-dispatch.js';

// 실제 필드를 리틀엔디언으로 입력한다. 미작성 꼬리는 지우지 않는다.
function put(bytes, at, value, width = 4) {
    // 요청한 폭만 쓴다.
    for (let index = 0; index < width; index += 1) bytes[at + index] = Math.floor(value / 2 ** (8 * index)) & 0xff;
}
// cpp 검사와 같은 256BYTE 패킷이며 본문과 자기 수신자만 입력한다.
function packet(body = Buffer.from('032hello\n'), count = 1) {
    const bytes = new Uint8Array(256).fill(0xd3);
    put(bytes, 0, 3, 1); put(bytes, 1, 0x41, 1); put(bytes, 2, 22 + body.length + count * 4, 2);
    put(bytes, 5, 1, 1); put(bytes, 6, 100); put(bytes, 10, 7, 2); put(bytes, 12, body.length, 2); put(bytes, 14, count, 1);
    bytes.set([76, 111, 99, 97, 108, 0, 0], 15); bytes.set(body, 22);
    // 최초 count개의 같은 자기 subscriber를 쓴다.
    for (let index = 0; index < count; index += 1) put(bytes, 22 + body.length + index * 4, 100);
    return bytes;
}
// 전체 물리를 원문 fixture와 같은 소문자 hex로 표시한다.
const hex = (bytes) => Buffer.from(bytes).toString('hex');

test('ChatRelay_IndependentNative132WholePacketAndMask: 원문 132행 전체가 일치한다', () => {
    const rows = fs.readFileSync(new URL('../../cpppj/tests/fixtures/chat-relay-x86.tsv', import.meta.url), 'utf8').trimEnd().split(/\r?\n/).slice(1);
    assert.equal(rows.length, 132);
    // 고정된 독립 원문 행마다 새 패킷/현재 전역/효과 기록을 만든다.
    for (const line of rows) {
        const row = line.split('\t'); assert.equal(row.length, 9);
        let battle = Number(row[0]); const server = Number(row[1]); const count = Number(row[2]); const mode = row[4];
        const bytes = packet(Buffer.from(row[3], 'hex'), count); const mask = { value: 0xaabbccdd }; const trace = [];
        const hooks = {
            read: (at) => { if (at === 0x594fbc) return battle; if (at === 0x540bc0) return server; if (at === 0x540c74) return 100; throw new Error('미정 전역'); },
            address: (sender) => { trace.push(`address:${sender}`); if (mode === 'stopAddress') throw new Error('address'); if (mode === 'addressBattleOff') battle = 0; return Uint8Array.from({ length: 20 }, (_, index) => index); },
            receive: (data, address) => { trace.push(`receive:${hex(data)}:${hex(address)}`); if (mode === 'stopReceive') throw new Error('receive'); },
        };
        let status = 'normal'; try { relayChatPacket(9, bytes, mask, hooks); } catch (error) { status = error.message; }
        assert.deepEqual([status, String(mask.value), hex(bytes), trace.join(';') || '-'], row.slice(5), line.slice(0, 70));
    }
});

test('원문 signed 주소·마지막 BYTE 범위를 거부하고 앞선 효과는 유지한다', () => {
    const hooks = { address: () => new Uint8Array(20), read: () => 1 }; const mask = { value: 123 };
    const negative = packet(); put(negative, 10, 0x8000, 2); assert.throws(() => relayChatPacket(0, negative, mask, hooks), /signed 본문/);
    const far = packet(); put(far, 12, 0xffff, 2); assert.throws(() => relayChatPacket(0, far, mask, hooks), /마지막 본문/);
    const short = new Uint8Array(11); assert.throws(() => relayChatPacket(0, short, mask, hooks), RangeError); assert.equal(mask.value, 123);
    const noRecipient = packet().subarray(0, 31); assert.throws(() => relayChatPacket(0, noRecipient, mask, hooks), RangeError);
    assert.equal(mask.value, 32); assert.equal(noRecipient[30], 0); assert.equal(noRecipient[14], 1);
});

test('다른 subscriber는 수신자 첫 항목 변경 뒤 명시적인 웹 제외 오류다', () => {
    const bytes = packet(); put(bytes, 31, 101); const mask = { value: 0 };
    assert.throws(() => relayChatPacket(0, bytes, mask, { address: () => new Uint8Array(20), read: (at) => at === 0x540c74 ? 100 : 1 }), /웹 제외.*LAN/);
    assert.equal(bytes[14], 1); assert.equal(bytes[31], 101); assert.equal(mask.value, 32);
});

test('실제 RawPacketDispatch 등록/송신 관찰/수신은 같은 패킷과 mask 접근자를 사용한다', () => {
    const trace = []; let receivedMask = 9; const mask = { get value() { return receivedMask; }, set value(value) { receivedMask = value; } };
    const dispatch = new RawPacketDispatch({ now: () => 1, record: () => {}, terminate: () => {}, close: () => {}, release: () => {} });
    const hooks = { read: (at) => at === 0x540c74 ? 100 : 1, address: (sender) => { assert.equal(sender, 9); return new Uint8Array(20); }, receive: (bytes) => trace.push(bytes) };
    registerChatPacket(dispatch, mask, hooks); hooks.receive = () => { throw new Error('등록 뒤 훅 객체 변경이 기존 값 캡처를 바꿈'); };
    const entry = dispatch.registration(0x41); assert.equal(entry.version, 1); assert.equal(entry.mismatch, 0); assert.equal(entry.observer, null);
    const bytes = packet(); put(bytes, 2, 4, 2); entry.packetObserver(bytes, 0); assert.equal(bytes[2], 4); entry.packetObserver(bytes, 1); assert.equal(bytes[2], 35);
    assert.equal(dispatch.dispatch(new PacketConnection({ sender: 9 }), bytes), true);
    assert.deepEqual(trace, [bytes]); assert.equal(receivedMask, 32); assert.equal(bytes[30], 0);
});

test('장치는 선택된 위치에서만 요구하고 주소 효과에서 바뀐 현재 모드를 읽는다', () => {
    const bytes = packet(Buffer.from('x'), 0); const mask = { value: 7 };
    relayChatPacket(0, bytes, mask, { address: () => new Uint8Array(20), read: (at) => at === 0x540bc0 ? 1 : 0 });
    assert.equal(bytes[14], 1); assert.equal(mask.value, 7);
    assert.throws(() => relayChatPacket(0, bytes, mask, {}), /선택된 실제 장치/);
    assert.throws(() => relayChatPacket(0, packet(), mask, { address: () => 1, read: () => 0 }), /선택된 실제 장치/);
});
