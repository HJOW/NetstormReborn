// 기술 타입 69h 패킷 raw-technology-types-packet.js(RawTechnologyTypesPacket, 원본 0041c340·004b49e0·0040af10)를 cpppj 소스 계약과 실제 RawPacketDispatch 로 대조한다(WB04-E560).
// 원본 관측 fixture 가 없는 소단위이므로 목록 누적(첫 NUL/중복/첫 빈칸)·합집합 뒤 전체 재순회·송신 관찰기의 sentinel 재조회·직접/분배 겸용 등록을 직접 단언한다. 실제 게임·브라우저 실행이 아니다. (2026-11-15 WB04-E560)
import test from 'node:test';
import assert from 'node:assert/strict';
import { PacketConnection, RawPacketDispatch } from '../www/core/raw-packet-dispatch.js';
import { addTechnologyTypes, observeTechnologyTypesPacket, receiveTechnologyTypesPacket, registerTechnologyTypesPacket } from '../www/core/raw-technology-types-packet.js';

// 256바이트 목록을 만든다. values 를 앞에서부터 채우고 나머지는 NUL 이다. (2026-11-15 WB04-E560)
const makeList = (values = []) => { const list = new Uint8Array(256); list.set(values); return list; };
// 69h 패킷을 만든다: 버전 1·타입 0x69·선언 길이 WORD·길이 BYTE(+4)·payload(+5, 호출자가 NUL 종료를 포함). (2026-11-15 WB04-E560)
function makePacket(payload, lengthByte = payload.length) {
    const packet = new Uint8Array(5 + payload.length).fill(0xee);
    packet[0] = 1;
    packet[1] = 0x69;
    packet[2] = 0x11;
    packet[3] = 0x22;
    packet[4] = lengthByte;
    packet.set(payload, 5);
    return packet;
}
// 목록의 첫 NUL 앞까지를 배열로 읽는다. (2026-11-15 WB04-E560)
const listed = (list) => { const end = list.indexOf(0); return [...list.subarray(0, end < 0 ? 256 : end)]; };

test('누적: 입력 BYTE 마다 현재 목록의 첫 일치 또는 첫 빈칸에 합치며 입력 순서를 보존한다', () => {
    const list = makeList();
    addTechnologyTypes(Uint8Array.of(5, 9, 5, 7, 0, 99), list);
    assert.deepEqual(listed(list), [5, 9, 7]);
    // 기존 목록 뒤에 새 값만 붙이고 이미 있는 값은 건드리지 않는다.
    addTechnologyTypes(Uint8Array.of(9, 11, 5, 0), list);
    assert.deepEqual(listed(list), [5, 9, 7, 11]);
    // 입력의 첫 NUL 뒤는 읽지 않는다. 0x80..0xff 도 그대로 비교한다.
    const high = makeList();
    addTechnologyTypes(Uint8Array.of(0x80, 0xff, 0x80, 0), high);
    assert.deepEqual(listed(high), [0x80, 0xff]);
    // 빈 입력은 아무 것도 쓰지 않는다. 종료 NUL 은 별도로 쓰지 않는다(이미 0 이다).
    const same = makeList([1, 2]);
    addTechnologyTypes(Uint8Array.of(0), same);
    assert.deepEqual(listed(same), [1, 2]);
    assert.equal(same[2], 0);
});

test('누적 경계: 목록의 마지막 칸(255)까지만 쓰고 가득 찬 상태는 선택된 index 256 읽기에서 오류이다', () => {
    // 255종이 있고 마지막 칸이 NUL 이면 새 값은 255 번째 칸에 들어간다.
    const nearlyFull = makeList(Array.from({ length: 255 }, (_, index) => (index % 250) + 1));
    nearlyFull[255] = 0;
    addTechnologyTypes(Uint8Array.of(251, 0), nearlyFull);
    assert.equal(nearlyFull[255], 251);
    // 가득 찬 목록에 이미 있는 값은 오류 없이 지나가지만 없는 값은 index 256 읽기에서 거부한다.
    const full = Uint8Array.from({ length: 256 }, (_, index) => (index % 250) + 1);
    assert.doesNotThrow(() => addTechnologyTypes(Uint8Array.of(7, 0), full));
    assert.throws(() => addTechnologyTypes(Uint8Array.of(255, 0), full), RangeError);
});

test('누적 오류: 종료 NUL 이 없는 입력은 앞선 저장을 남긴 채 선택된 BYTE 에서 물리 오류이다', () => {
    const list = makeList();
    assert.throws(() => addTechnologyTypes(Uint8Array.of(3, 4, 5), list), RangeError);
    assert.deepEqual(listed(list), [3, 4, 5]);
    // 완전히 빈 입력은 첫 BYTE 읽기에서 거부한다.
    assert.throws(() => addTechnologyTypes(new Uint8Array(0), makeList()), RangeError);
});

test('수신: payload+5 합집합 뒤 기존/신규 전체 목록을 첫 NUL 까지 순서대로 표시한다', () => {
    const list = makeList([2, 4]);
    const shown = [];
    // 길이 BYTE(+4)는 읽지 않는다: 길이가 0 이어도 payload 는 끝까지 합친다.
    receiveTechnologyTypesPacket(makePacket(Uint8Array.of(4, 6, 8, 0), 0), list, (type) => shown.push(type));
    assert.deepEqual(shown, [2, 4, 6, 8]);
    assert.deepEqual(listed(list), [2, 4, 6, 8]);
    // 변경이 없어도 현재 목록 전체를 다시 전달한다.
    shown.length = 0;
    receiveTechnologyTypesPacket(makePacket(Uint8Array.of(0)), list, (type) => shown.push(type));
    assert.deepEqual(shown, [2, 4, 6, 8]);
    // 높은 BYTE 도 unsigned 로 표시한다.
    const high = makeList();
    const out = [];
    receiveTechnologyTypesPacket(makePacket(Uint8Array.of(0xc8, 0)), high, (type) => out.push(type));
    assert.deepEqual(out, [0xc8]);
});

test('수신: 표시 효과가 바꾼 다음 목록 BYTE 를 다시 읽고 빈 목록은 표시 장치를 요구하지 않는다', () => {
    const list = makeList();
    const shown = [];
    // 첫 항목을 표시할 때 다음 칸을 지워 순회가 거기서 끝난다.
    receiveTechnologyTypesPacket(makePacket(Uint8Array.of(1, 2, 3, 0)), list, (type) => { shown.push(type); list[1] = 0; });
    assert.deepEqual(shown, [1]);
    // 표시 효과가 새 항목을 덧붙이면 그것도 순회에 포함된다.
    const appended = makeList();
    const out = [];
    receiveTechnologyTypesPacket(makePacket(Uint8Array.of(7, 0)), appended, (type) => { out.push(type); if (type === 7) appended[1] = 9; });
    assert.deepEqual(out, [7, 9]);
    // 빈 목록에서는 표시 장치가 없어도 되고, 항목이 있으면 합집합 뒤 명시 오류이다.
    assert.doesNotThrow(() => receiveTechnologyTypesPacket(makePacket(Uint8Array.of(0)), makeList(), undefined));
    const withItem = makeList();
    assert.throws(() => receiveTechnologyTypesPacket(makePacket(Uint8Array.of(5, 0)), withItem, undefined), /미연결/);
    assert.deepEqual(listed(withItem), [5]);
});

test('수신 오류: payload+5 가 없거나 NUL 이 없으면 물리 오류이고 앞선 합집합은 남는다', () => {
    const list = makeList();
    assert.throws(() => receiveTechnologyTypesPacket(new Uint8Array(4), list, () => {}), { name: 'RangeError', message: /payload\+5/ });
    // 정확히 5바이트(빈 payload)는 payload+5 검사를 통과해 첫 BYTE 읽기에서 거부한다.
    assert.throws(() => receiveTechnologyTypesPacket(new Uint8Array(5), list, () => {}), { name: 'RangeError', message: /선택된 현재 BYTE/ });
    const shown = [];
    assert.throws(() => receiveTechnologyTypesPacket(makePacket(Uint8Array.of(3, 4)), list, (type) => shown.push(type)), RangeError);
    assert.deepEqual(listed(list), [3, 4]);
    assert.deepEqual(shown, []);
    // 종료 NUL 이 없는 가득 찬 목록은 순회 중 index 256 읽기에서 거부한다(표시는 255 번째까지 이미 일어난다).
    const full = Uint8Array.from({ length: 256 }, (_, index) => (index % 250) + 1);
    const out = [];
    assert.throws(() => receiveTechnologyTypesPacket(makePacket(Uint8Array.of(1, 0)), full, (type) => out.push(type)), RangeError);
    assert.equal(out.length, 256);
});

test('송신 관찰기: 수신 방향은 무동작이고 송신은 길이 BYTE+5 를 선언 길이로 쓴다', () => {
    const calls = [];
    const assertion = (text, file, line) => calls.push([text, file, line]);
    const received = makePacket(Uint8Array.of(1, 0));
    const snapshot = Uint8Array.from(received);
    observeTechnologyTypesPacket(received, false, assertion);
    assert.deepEqual(received, snapshot);
    assert.doesNotThrow(() => observeTechnologyTypesPacket(new Uint8Array(1), false, assertion));
    const sent = makePacket(Uint8Array.of(1, 0), 0x10);
    observeTechnologyTypesPacket(sent, true, assertion);
    assert.deepEqual([sent[2], sent[3]], [0x15, 0]);
    // 0xfe+5 = 259 = 0x103.
    const large = makePacket(Uint8Array.of(0), 0xfe);
    observeTechnologyTypesPacket(large, true, assertion);
    assert.deepEqual([large[2], large[3]], [0x03, 0x01]);
    assert.deepEqual(calls, []);
    // 정확히 5바이트면 통과한다. 4바이트면 물리 오류이다.
    const exact = new Uint8Array(5);
    exact[4] = 2;
    observeTechnologyTypesPacket(exact, true, assertion);
    assert.deepEqual([exact[2], exact[3]], [7, 0]);
    assert.throws(() => observeTechnologyTypesPacket(new Uint8Array(4), true, assertion), RangeError);
});

test('송신 관찰기의 sentinel: 진단 뒤 현재 BYTE 를 다시 읽고 그대로면 260 이며 장치가 없으면 쓰기 전에 거부한다', () => {
    const calls = [];
    const packet = makePacket(Uint8Array.of(0), 0xff);
    observeTechnologyTypesPacket(packet, true, (text, file, line) => calls.push([text, file, line]));
    assert.deepEqual(calls, [['len != (unsigned char)-1', '\\Ns\\O\\_p_packets.cpp', 0xa5f]]);
    assert.deepEqual([packet[2], packet[3]], [0x04, 0x01]);
    // 진단이 BYTE 를 고치면 고친 값으로 계산한다.
    const fixed = makePacket(Uint8Array.of(0), 0xff);
    observeTechnologyTypesPacket(fixed, true, () => { fixed[4] = 0x20; });
    assert.deepEqual([fixed[2], fixed[3]], [0x25, 0]);
    const missing = makePacket(Uint8Array.of(0), 0xff);
    assert.throws(() => observeTechnologyTypesPacket(missing, true, undefined), /미연결/);
    assert.deepEqual([missing[2], missing[3]], [0x11, 0x22]);
    assert.doesNotThrow(() => observeTechnologyTypesPacket(makePacket(Uint8Array.of(0), 0xfe), true, undefined));
});

test('등록: 69h 에 관찰기·버전 1/정책 0·가변 길이·직접 겸용 수신기를 설치하고 분배/직접 호출이 같은 본문을 쓴다', () => {
    const dispatch = new RawPacketDispatch({ now: () => 1, record: () => {}, terminate: () => {}, close: () => {}, release: () => {} });
    const list = makeList();
    const shown = [];
    const log = [];
    registerTechnologyTypesPacket(dispatch, list, (type) => shown.push(type), (text, file, line) => log.push([text, file, line]));
    const row = dispatch.registration(0x69);
    assert.equal(row.version, 1);
    assert.equal(row.mismatch, 0);
    assert.equal(row.length, 0xffff);
    assert.equal(typeof row.packetObserver, 'function');
    assert.equal(row.observer, null);
    assert.equal(typeof row.receive, 'function');
    assert.equal(typeof row.directReceive, 'function');
    // 관찰기는 등록된 진단을 쓴다.
    row.packetObserver(makePacket(Uint8Array.of(0), 0xff), true);
    assert.equal(log.length, 1);
    // 분배된 수신은 sender/연결과 무관하게 같은 목록에 합치고 전체를 표시한다.
    assert.equal(dispatch.dispatch(new PacketConnection({ sender: 77 }), makePacket(Uint8Array.of(10, 20, 0))), true);
    assert.deepEqual(shown, [10, 20]);
    // 직접 호출은 NULL 연결을 허용하고 같은 목록을 쓴다.
    shown.length = 0;
    dispatch.invokeDirect(0x69, 3, null, makePacket(Uint8Array.of(20, 30, 0)));
    assert.deepEqual(shown, [10, 20, 30]);
    assert.deepEqual(listed(list), [10, 20, 30]);
    // 다른 종류를 건드리지 않고, 256바이트가 아닌 목록은 등록 때 거부한다.
    assert.equal(dispatch.registration(0x6a).receive, null);
    assert.throws(() => registerTechnologyTypesPacket(dispatch, new Uint8Array(255), () => {}, () => {}), TypeError);
});
