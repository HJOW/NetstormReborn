// 원격 Tell(70h) 패킷 raw-remote-tell-packet.js(RawRemoteTellPacket, 원본 004c9150·004c9200·0040b090·004cf980)를 cpppj 소스 계약과 실제 RawPacketDispatch 로 대조한다(WB04-E559).
// 원본 관측 fixture 가 없는 소단위이므로 264바이트 쓰기 규격·길이 DWORD 선기록·진단 경계(255/256)·송신 관찰기의 sentinel 재조회·NUL 문자열 수신·등록을 직접 단언한다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-15 WB04-E559)
import test from 'node:test';
import assert from 'node:assert/strict';
import { PacketConnection, RawPacketDispatch } from '../www/core/raw-packet-dispatch.js';
import { observeRemoteTellPacket, receiveRemoteTellPacket, registerRemoteTellPacket, writeRemoteTellPacket } from '../www/core/raw-remote-tell-packet.js';

// 진단 호출을 모으는 장치이다. (2026-11-15 WB04-E559)
const makeAssertion = (log) => (text, file, line) => log.push([text, file, line]);
// 헤더 8바이트와 본문을 가진 70h 패킷을 만든다(길이 DWORD 는 +4). (2026-11-15 WB04-E559)
function makePacket(body, length = body.length + 1, size = 8 + body.length + 1) {
    const packet = new Uint8Array(size).fill(0xee);
    packet[0] = 1;
    packet[1] = 0x70;
    packet[2] = 0x11;
    packet[3] = 0x22;
    for (let index = 0; index < 4; index += 1) packet[4 + index] = (length >>> (index * 8)) & 0xff;
    for (let index = 0; index < body.length; index += 1) packet[8 + index] = body.charCodeAt(index);
    packet[8 + body.length] = 0;
    return packet;
}

test('쓰기: 264바이트를 0xff 로 채우고 헤더·길이 DWORD·문자열·NUL 을 쓴다', () => {
    const log = [];
    const packet = writeRemoteTellPacket('hi', makeAssertion(log), 0x4ef);
    assert.equal(packet.length, 264);
    assert.deepEqual([...packet.subarray(0, 11)], [1, 0x70, 4, 0, 3, 0, 0, 0, 0x68, 0x69, 0]);
    assert.ok(packet.subarray(11).every((byte) => byte === 0xff));
    assert.deepEqual(log, []);
    // 빈 문자열은 길이 1 과 NUL 만 쓴다.
    const empty = writeRemoteTellPacket('', undefined, 0x4fb);
    assert.deepEqual([...empty.subarray(0, 10)], [1, 0x70, 4, 0, 1, 0, 0, 0, 0, 0xff]);
    // 첫 NUL 뒤는 버리고 문자 코드는 BYTE 로 쓴다.
    const nul = writeRemoteTellPacket('ab\0cd', undefined, 0x4ef);
    assert.deepEqual([...nul.subarray(4, 12)], [3, 0, 0, 0, 0x61, 0x62, 0, 0xff]);
    const wide = writeRemoteTellPacket('éŁ', undefined, 0x4ef);
    assert.deepEqual([...wide.subarray(8, 11)], [0xe9, 0x41, 0]);
});

test('쓰기 길이 경계: 길이+1 이 255 이하이면 진단 없음, 256 은 진단 뒤 복사, 257 이상은 진단 뒤 물리 범위 오류이다', () => {
    const none = [];
    const at255 = writeRemoteTellPacket('x'.repeat(254), makeAssertion(none), 0x4ef);
    assert.deepEqual(none, []);
    assert.equal(at255[4], 255);
    assert.equal(at255[8 + 254], 0);
    // 길이 256: 진단을 내고 복사는 끝까지 한다(+8 에 255자, NUL 은 마지막 BYTE).
    const log = [];
    const at256 = writeRemoteTellPacket('y'.repeat(255), makeAssertion(log), 0x4fb);
    assert.deepEqual(log, [['z->tellLength < 256', '.\\Ui.cpp', 0x4fb]]);
    assert.equal(at256[4], 0);
    assert.equal(at256[5], 1);
    assert.equal(at256[8], 0x79);
    assert.equal(at256[8 + 254], 0x79);
    assert.equal(at256[263], 0);
    // 길이 257: 진단 뒤 복사 물리 범위 오류이다. 진단 장치가 없으면 진단 전에 명시 오류이다.
    const over = [];
    assert.throws(() => writeRemoteTellPacket('z'.repeat(256), makeAssertion(over), 0x4ef), RangeError);
    assert.deepEqual(over, [['z->tellLength < 256', '.\\Ui.cpp', 0x4ef]]);
    assert.throws(() => writeRemoteTellPacket('z'.repeat(255), undefined, 0x4ef), /미연결/);
    // 줄 번호는 호출자가 공급한다.
    const other = [];
    writeRemoteTellPacket('w'.repeat(255), makeAssertion(other), 1234);
    assert.equal(other[0][2], 1234);
});

test('송신 관찰기: 수신 방향은 무동작이고 송신은 현재 길이+8 의 하위 WORD 를 선언 길이로 쓴다', () => {
    const log = [];
    const assertion = makeAssertion(log);
    const received = makePacket('abc');
    const snapshot = Uint8Array.from(received);
    observeRemoteTellPacket(received, false, assertion);
    assert.deepEqual(received, snapshot);
    // 짧은 패킷도 수신 방향이면 물리 검사를 하지 않는다.
    assert.doesNotThrow(() => observeRemoteTellPacket(new Uint8Array(2), false, assertion));
    const sent = makePacket('abc');
    observeRemoteTellPacket(sent, true, assertion);
    assert.equal(sent[2], 12);
    assert.equal(sent[3], 0);
    assert.deepEqual(log, []);
    // 32비트 합의 하위 WORD 만 쓴다: 0x00ffff00+8 → 0xff08.
    const wrapped = makePacket('', 0x00ffff00);
    observeRemoteTellPacket(wrapped, true, assertion);
    assert.deepEqual([wrapped[2], wrapped[3]], [0x08, 0xff]);
    const carry = makePacket('', 0xfa);
    observeRemoteTellPacket(carry, true, assertion);
    assert.deepEqual([carry[2], carry[3]], [0x02, 0x01]);
});

test('송신 관찰기의 sentinel: 진단 뒤 현재 +4 를 다시 읽고 장치가 없으면 쓰기 전에 거부한다', () => {
    const log = [];
    const packet = makePacket('abc', 0xffffffff);
    observeRemoteTellPacket(packet, true, makeAssertion(log));
    assert.deepEqual(log, [['tellLength != (int)-1', '\\Ns\\O\\_p_packets.cpp', 0xac7]]);
    // 진단이 값을 바꾸지 않으면 (0xffffffff+8) 의 32비트 감김 7 이다.
    assert.deepEqual([packet[2], packet[3]], [7, 0]);
    // 진단이 길이를 고치면 고친 현재 값으로 계산한다.
    const fixed = makePacket('abc', 0xffffffff);
    observeRemoteTellPacket(fixed, true, () => { fixed[4] = 20; fixed[5] = 0; fixed[6] = 0; fixed[7] = 0; });
    assert.deepEqual([fixed[2], fixed[3]], [28, 0]);
    // 장치가 없으면 선언 길이를 건드리지 않고 명시 오류이다. sentinel 이 아니면 장치를 요구하지도 않는다.
    const missing = makePacket('abc', 0xffffffff);
    assert.throws(() => observeRemoteTellPacket(missing, true, undefined), /미연결/);
    assert.deepEqual([missing[2], missing[3]], [0x11, 0x22]);
    const ok = makePacket('abc');
    assert.doesNotThrow(() => observeRemoteTellPacket(ok, true, undefined));
    // 헤더 8바이트가 없으면 물리 오류이다.
    assert.throws(() => observeRemoteTellPacket(new Uint8Array(7), true, makeAssertion([])), RangeError);
    // 헤더가 정확히 8바이트이면 통과해 선언 길이를 쓴다(본문 없음).
    const header = new Uint8Array(8);
    header[4] = 1;
    observeRemoteTellPacket(header, true, makeAssertion([]));
    assert.deepEqual([header[2], header[3]], [9, 0]);
});

test('수신: +8 의 첫 NUL 문자열을 전달하며 선언 길이·길이 DWORD 는 읽지 않는다', () => {
    const received = [];
    const tell = (text) => received.push(text);
    receiveRemoteTellPacket(makePacket('hello', 0x7fffffff), tell);
    receiveRemoteTellPacket(makePacket('', 0), tell);
    // 첫 NUL 뒤의 바이트는 무시하고 높은 코드도 그대로 문자로 전달한다.
    const trailing = makePacket('ab', 9, 20);
    trailing.set([0x63, 0x64, 0], 11);
    receiveRemoteTellPacket(trailing, tell);
    const high = makePacket('', 1, 12);
    high.set([0xe9, 0x41, 0], 8);
    receiveRemoteTellPacket(high, tell);
    assert.deepEqual(received, ['hello', '', 'ab', 'éA']);
});

test('수신 오류: 헤더만·NUL 없음은 물리 오류이고 소비자는 검사 뒤 요구한다', () => {
    const received = [];
    const tell = (text) => received.push(text);
    const physical = { name: 'RangeError', message: /문자열 물리 누락/ };
    const missingNul = { name: 'RangeError', message: /NUL 누락/ };
    assert.throws(() => receiveRemoteTellPacket(new Uint8Array(8), tell), physical);
    assert.throws(() => receiveRemoteTellPacket(new Uint8Array(0), tell), physical);
    const noNul = new Uint8Array(12).fill(0x41);
    assert.throws(() => receiveRemoteTellPacket(noNul, tell), missingNul);
    assert.deepEqual(received, []);
    // NUL 이 헤더 영역(+4..7)에만 있어도 +8 이전은 찾지 않는다.
    const headerNul = new Uint8Array(12).fill(0x41);
    headerNul[5] = 0;
    assert.throws(() => receiveRemoteTellPacket(headerNul, tell), missingNul);
    // 소비자가 없으면 문자열 검사 뒤 명시 오류이다.
    assert.throws(() => receiveRemoteTellPacket(makePacket('x'), undefined), /미연결/);
    assert.throws(() => receiveRemoteTellPacket(new Uint8Array(12).fill(0x41), undefined), RangeError);
});

test('등록: 70h 에 관찰기·버전 1/정책 0·가변 길이·수신기를 설치하고 실제 분배로 수신한다', () => {
    const dispatch = new RawPacketDispatch({ now: () => 1, record: () => {}, terminate: () => {}, close: () => {}, release: () => {} });
    const received = [];
    const log = [];
    registerRemoteTellPacket(dispatch, (text) => received.push(text), makeAssertion(log));
    const row = dispatch.registration(0x70);
    assert.equal(row.version, 1);
    assert.equal(row.mismatch, 0);
    assert.equal(row.length, 0xffff);
    assert.equal(typeof row.packetObserver, 'function');
    assert.equal(row.observer, null);
    // 송신 관찰기는 등록된 진단을 쓰고, 다른 종류(0x71)는 건드리지 않는다.
    const sent = makePacket('hey', 0xffffffff);
    row.packetObserver(sent, true);
    assert.deepEqual(log, [['tellLength != (int)-1', '\\Ns\\O\\_p_packets.cpp', 0xac7]]);
    assert.equal(dispatch.registration(0x71).receive, null);
    // 분배된 수신은 sender/연결과 무관하게 문자열만 전달한다(다른 버전·선언 길이 무관).
    assert.equal(dispatch.dispatch(new PacketConnection({ sender: 200 }), makePacket('abc', 4, 12)), true);
    assert.deepEqual(received, ['abc']);
    const later = makePacket('zz');
    later[0] = 5;
    assert.equal(dispatch.dispatch(new PacketConnection({ sender: 3 }), later), true);
    assert.deepEqual(received, ['abc', 'zz']);
});
