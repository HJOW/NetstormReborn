// 개발자 문자열 명령(31h) 패킷 raw-developer-command-packet.js(RawDeveloperCommandPacket, 원본 0040a6b0·0044dba0→0040a6f0)를 cpppj 소스 계약과 실제 RawPacketDispatch 로 대조한다(WB04-E559).
// 원본 관측 fixture 가 없는 소단위이므로 송신 관찰기의 sentinel 재조회·길이 WORD 감김·NUL 문자열 수신·해석기 요구 순서·등록을 직접 단언한다. 실제 게임·브라우저 실행이 아니다. (2026-11-15 WB04-E559)
import test from 'node:test';
import assert from 'node:assert/strict';
import { PacketConnection, RawPacketDispatch } from '../www/core/raw-packet-dispatch.js';
import { observeDeveloperCommandPacket, receiveDeveloperCommandPacket, registerDeveloperCommandPacket } from '../www/core/raw-developer-command-packet.js';

// 진단 호출을 모으는 장치이다. (2026-11-15 WB04-E559)
const makeAssertion = (log) => (text, file, line) => log.push([text, file, line]);
// 헤더 8바이트와 본문을 가진 31h 패킷을 만든다(길이 DWORD 는 +4). (2026-11-15 WB04-E559)
function makePacket(body, length = body.length + 1, size = 8 + body.length + 1) {
    const packet = new Uint8Array(size).fill(0xee);
    packet[0] = 1;
    packet[1] = 0x31;
    packet[2] = 0x11;
    packet[3] = 0x22;
    for (let index = 0; index < 4; index += 1) packet[4 + index] = (length >>> (index * 8)) & 0xff;
    for (let index = 0; index < body.length; index += 1) packet[8 + index] = body.charCodeAt(index);
    packet[8 + body.length] = 0;
    return packet;
}

test('송신 관찰기: 수신 방향은 무동작이고 송신은 현재 len+8 의 하위 WORD 를 선언 길이로 쓴다', () => {
    const log = [];
    const assertion = makeAssertion(log);
    const received = makePacket('cmd');
    const snapshot = Uint8Array.from(received);
    observeDeveloperCommandPacket(received, false, assertion);
    assert.deepEqual(received, snapshot);
    assert.doesNotThrow(() => observeDeveloperCommandPacket(new Uint8Array(2), false, assertion));
    const sent = makePacket('cmd');
    observeDeveloperCommandPacket(sent, true, assertion);
    assert.deepEqual([sent[2], sent[3]], [12, 0]);
    assert.deepEqual(log, []);
    // 하위 WORD 만 쓰고 감김을 보존한다.
    const wrapped = makePacket('', 0x00ffff00);
    observeDeveloperCommandPacket(wrapped, true, assertion);
    assert.deepEqual([wrapped[2], wrapped[3]], [0x08, 0xff]);
    const carry = makePacket('', 0xfa);
    observeDeveloperCommandPacket(carry, true, assertion);
    assert.deepEqual([carry[2], carry[3]], [0x02, 0x01]);
    // 헤더 8바이트가 없으면 물리 오류이다.
    assert.throws(() => observeDeveloperCommandPacket(new Uint8Array(7), true, assertion), RangeError);
    // 헤더가 정확히 8바이트이면 통과해 선언 길이를 쓴다(본문 없음).
    const header = new Uint8Array(8);
    header[4] = 1;
    observeDeveloperCommandPacket(header, true, assertion);
    assert.deepEqual([header[2], header[3]], [9, 0]);
});

test('송신 관찰기의 sentinel: 진단 뒤 현재 len 을 다시 읽고 장치가 없으면 쓰기 전에 거부한다', () => {
    const log = [];
    const packet = makePacket('cmd', 0xffffffff);
    observeDeveloperCommandPacket(packet, true, makeAssertion(log));
    assert.deepEqual(log, [['len != (int)-1', '\\Ns\\O\\_p_packets.cpp', 0x74b]]);
    assert.deepEqual([packet[2], packet[3]], [7, 0]);
    const fixed = makePacket('cmd', 0xffffffff);
    observeDeveloperCommandPacket(fixed, true, () => { fixed[4] = 100; fixed[5] = 0; fixed[6] = 0; fixed[7] = 0; });
    assert.deepEqual([fixed[2], fixed[3]], [108, 0]);
    const missing = makePacket('cmd', 0xffffffff);
    assert.throws(() => observeDeveloperCommandPacket(missing, true, undefined), /미연결/);
    assert.deepEqual([missing[2], missing[3]], [0x11, 0x22]);
    // sentinel 이 아니면 진단 장치를 요구하지 않는다.
    assert.doesNotThrow(() => observeDeveloperCommandPacket(makePacket('cmd'), true, undefined));
});

test('수신: +8 의 첫 NUL 문자열을 해석기에 전달하며 송신 길이 검사를 하지 않는다', () => {
    const received = [];
    const execute = (text) => received.push(text);
    receiveDeveloperCommandPacket(makePacket('give 100', 0xffffffff), execute);
    receiveDeveloperCommandPacket(makePacket('', 0), execute);
    const trailing = makePacket('ab', 9, 20);
    trailing.set([0x63, 0x64, 0], 11);
    receiveDeveloperCommandPacket(trailing, execute);
    const high = makePacket('', 1, 12);
    high.set([0xe9, 0x41, 0], 8);
    receiveDeveloperCommandPacket(high, execute);
    assert.deepEqual(received, ['give 100', '', 'ab', 'éA']);
});

test('수신 오류: 헤더만·NUL 없음은 물리 오류이고 해석기 누락은 이 검사들 뒤의 명시 오류이다', () => {
    const received = [];
    const execute = (text) => received.push(text);
    const physical = { name: 'RangeError', message: /문자열 물리 부족/ };
    const missingNul = { name: 'RangeError', message: /NUL 누락/ };
    assert.throws(() => receiveDeveloperCommandPacket(new Uint8Array(8), execute), physical);
    assert.throws(() => receiveDeveloperCommandPacket(new Uint8Array(0), execute), physical);
    assert.throws(() => receiveDeveloperCommandPacket(new Uint8Array(12).fill(0x41), execute), missingNul);
    const headerNul = new Uint8Array(12).fill(0x41);
    headerNul[6] = 0;
    assert.throws(() => receiveDeveloperCommandPacket(headerNul, execute), missingNul);
    assert.deepEqual(received, []);
    assert.throws(() => receiveDeveloperCommandPacket(makePacket('x'), undefined), /미연결/);
    // 짧은/NUL 없는 패킷은 해석기가 없어도 물리 오류가 먼저이다.
    assert.throws(() => receiveDeveloperCommandPacket(new Uint8Array(8), undefined), RangeError);
    assert.throws(() => receiveDeveloperCommandPacket(new Uint8Array(12).fill(0x41), undefined), RangeError);
});

test('등록: 31h 에 관찰기·버전 1/정책 0·가변 길이·수신기를 설치하고 실제 분배로 수신한다', () => {
    const dispatch = new RawPacketDispatch({ now: () => 1, record: () => {}, terminate: () => {}, close: () => {}, release: () => {} });
    const received = [];
    const log = [];
    registerDeveloperCommandPacket(dispatch, (text) => received.push(text), makeAssertion(log));
    const row = dispatch.registration(0x31);
    assert.equal(row.version, 1);
    assert.equal(row.mismatch, 0);
    assert.equal(row.length, 0xffff);
    assert.equal(typeof row.packetObserver, 'function');
    assert.equal(row.observer, null);
    const sent = makePacket('cmd', 0xffffffff);
    row.packetObserver(sent, true);
    assert.deepEqual(log, [['len != (int)-1', '\\Ns\\O\\_p_packets.cpp', 0x74b]]);
    // 70h(원격 Tell)는 건드리지 않는다.
    assert.equal(dispatch.registration(0x70).receive, null);
    // 분배된 수신은 sender/연결과 무관하게 문자열만 전달한다(다른 버전은 정책 0 이라 그대로 수신).
    assert.equal(dispatch.dispatch(new PacketConnection({ sender: 123 }), makePacket('abc', 4, 12)), true);
    const other = makePacket('zz');
    other[0] = 6;
    assert.equal(dispatch.dispatch(new PacketConnection({ sender: 4 }), other), true);
    assert.deepEqual(received, ['abc', 'zz']);
});
