// 일반/메타 71h 전투 설명 등록 raw-state-meta-battle-description.js(RawStateMetaBattleDescription, 원본 0040b0d0·004b49c0)를 cpppj 소스 계약과 실제 RawPacketDispatch 로 대조한다(WB04-E558).
// 원본 관측 fixture 가 없는 소단위이므로 송신 observer 의 길이 확정·sentinel 진단·버전/길이 등록·CS 관문 수신을 단언한다. 실제 게임·브라우저 실행이 아니다. (2026-11-15 WB04-E558)
import test from 'node:test';
import assert from 'node:assert/strict';
import { PacketConnection, RawPacketDispatch } from '../www/core/raw-packet-dispatch.js';
import { registerStateMetaBattleDescription } from '../www/core/raw-state-meta-battle-description.js';

// 실제 분배기이다(필수 다섯 효과는 호출 기록만 남긴다). (2026-11-15 WB04-E558)
const makeDispatch = (log = []) => new RawPacketDispatch({ now: () => 1, record: (connection, packet, length) => log.push(['record', length]), terminate: () => log.push(['terminate']), close: () => {}, release: () => {} });
// 71h 패킷을 만든다: 버전 1·타입 0x71·선언 길이 WORD·설명 길이 BYTE(+9)와 그 뒤 설명 본문이다. (2026-11-15 WB04-E558)
function makePacket(descriptionLength, size = 10 + descriptionLength, version = 1) {
    const packet = new Uint8Array(size).fill(0xaa);
    packet[0] = version;
    packet[1] = 0x71;
    packet[2] = 0x11;
    packet[3] = 0x22;
    packet[9] = descriptionLength;
    return packet;
}

test('등록: 71h 에 방향 전용 observer·버전 1/정책 0·가변 길이·수신기만 설치하고 다른 종류는 건드리지 않는다', () => {
    const dispatch = makeDispatch();
    const before = JSON.stringify([0x70, 0x72, 0x20].map((type) => { const row = dispatch.registration(type); return [row.version, row.mismatch, row.length, Boolean(row.receive), Boolean(row.observer), Boolean(row.packetObserver)]; }));
    registerStateMetaBattleDescription(dispatch, { value: 0 }, () => {}, () => {});
    const row = dispatch.registration(0x71);
    assert.equal(row.version, 1);
    assert.equal(row.mismatch, 0);
    assert.equal(row.length, 0xffff);
    assert.equal(typeof row.packetObserver, 'function');
    assert.equal(row.observer, null);
    assert.equal(typeof row.receive, 'function');
    const after = JSON.stringify([0x70, 0x72, 0x20].map((type) => { const row2 = dispatch.registration(type); return [row2.version, row2.mismatch, row2.length, Boolean(row2.receive), Boolean(row2.observer), Boolean(row2.packetObserver)]; }));
    assert.equal(after, before);
});

test('송신 observer: 설명 길이 BYTE+10 을 길이 WORD(+2, 낮은 BYTE 먼저)로 확정한다', () => {
    const dispatch = makeDispatch();
    registerStateMetaBattleDescription(dispatch, 0, () => {}, () => { throw new Error('진단이 불리면 안 된다'); });
    const observe = dispatch.registration(0x71).packetObserver;
    for (const [description, low, high] of [[0, 10, 0], [5, 15, 0], [100, 110, 0], [245, 255, 0], [246, 0, 1], [254, 8, 1]]) {
        const packet = makePacket(description);
        observe(packet, true);
        assert.equal(packet[2], low, `낮은 BYTE ${description}`);
        assert.equal(packet[3], high, `높은 BYTE ${description}`);
        // 설명 길이 BYTE 와 다른 위치는 그대로이다.
        assert.equal(packet[9], description);
        assert.equal(packet[0], 1);
        assert.equal(packet[1], 0x71);
        assert.ok(packet.subarray(4, 9).every((byte) => byte === 0xaa));
    }
});

test('수신 방향이면 observer 는 아무것도 읽거나 쓰지 않는다', () => {
    const dispatch = makeDispatch();
    registerStateMetaBattleDescription(dispatch, 0, () => {}, () => { throw new Error('진단이 불리면 안 된다'); });
    const observe = dispatch.registration(0x71).packetObserver;
    const packet = makePacket(9);
    const snapshot = Uint8Array.from(packet);
    observe(packet, false);
    assert.deepEqual(packet, snapshot);
    // 짧은 패킷도 수신 방향이면 물리 검사를 하지 않는다.
    assert.doesNotThrow(() => observe(new Uint8Array(2), false));
});

test('sentinel 0xff: 진단을 먼저 호출하고 복귀 뒤 현재 BYTE 로 길이를 확정한다', () => {
    const dispatch = makeDispatch();
    const calls = [];
    registerStateMetaBattleDescription(dispatch, 0, () => {}, (text) => calls.push(text));
    const observe = dispatch.registration(0x71).packetObserver;
    const packet = makePacket(0xff, 20);
    observe(packet, true);
    assert.deepEqual(calls, ['descLen != (unsigned char)-1']);
    // 진단이 값을 바꾸지 않으면 255+10=265 → 0x109 이다.
    assert.equal(packet[2], 0x09);
    assert.equal(packet[3], 0x01);
    // 진단이 설명 길이를 고치면 고친 현재 값으로 계산한다.
    const fixing = makeDispatch();
    registerStateMetaBattleDescription(fixing, 0, () => {}, () => { fixed.packet[9] = 3; });
    const fixed = { packet: makePacket(0xff, 20) };
    fixing.registration(0x71).packetObserver(fixed.packet, true);
    assert.equal(fixed.packet[2], 13);
    assert.equal(fixed.packet[3], 0);
    // sentinel 이 아니면 진단 장치를 요구하지도 않는다. 장치가 없으면 쓰기 전에 명시 오류이다.
    const missing = makeDispatch();
    registerStateMetaBattleDescription(missing, 0, () => {}, undefined);
    const ok = makePacket(7);
    assert.doesNotThrow(() => missing.registration(0x71).packetObserver(ok, true));
    assert.equal(ok[2], 17);
    const bad = makePacket(0xff, 20);
    assert.throws(() => missing.registration(0x71).packetObserver(bad, true), /미연결/);
    assert.equal(bad[2], 0x11);
    assert.equal(bad[3], 0x22);
});

test('observer 의 패킷 물리 범위: 설명 길이 BYTE 가 패킷 밖이면 RangeError 이고 쓰지 않는다', () => {
    const dispatch = makeDispatch();
    registerStateMetaBattleDescription(dispatch, 0, () => {}, () => {});
    const observe = dispatch.registration(0x71).packetObserver;
    for (const size of [0, 4, 9]) {
        const packet = new Uint8Array(size).fill(0x55);
        assert.throws(() => observe(packet, true), RangeError);
        assert.ok(packet.every((byte) => byte === 0x55));
    }
    // 정확히 10바이트는 통과한다.
    const exact = makePacket(0, 10);
    assert.doesNotThrow(() => observe(exact, true));
    assert.equal(exact[2], 10);
});

test('수신기: 현재 CS 가 비영일 때만 자식을 호출하고 mode/sender 관문이 없다', () => {
    const dispatch = makeDispatch();
    const calls = [];
    const cs = { value: 0 };
    registerStateMetaBattleDescription(dispatch, cs, (sender, packet) => calls.push([sender, packet]), () => {});
    const connection = new PacketConnection({ sender: 250 });
    const packet = makePacket(4);
    // CS 가 0 이면 자식을 호출하지 않는다.
    assert.equal(dispatch.dispatch(connection, packet), true);
    assert.deepEqual(calls, []);
    // 비영이면 같은 패킷 객체와 연결의 현재 sender 를 그대로 넘기며 호출마다 현재 CS 를 다시 읽는다.
    cs.value = 0x1234;
    assert.equal(dispatch.dispatch(connection, packet), true);
    assert.equal(calls.length, 1);
    assert.equal(calls[0][0], 250);
    assert.equal(calls[0][1], packet);
    cs.value = 0;
    dispatch.dispatch(connection, packet);
    assert.equal(calls.length, 1);
    // 함수/단순 값 형태의 현재 CS 도 받는다.
    const byFunction = makeDispatch();
    let current = 0;
    const functionCalls = [];
    registerStateMetaBattleDescription(byFunction, () => current, (sender) => functionCalls.push(sender), () => {});
    byFunction.dispatch(new PacketConnection({ sender: 3 }), packet);
    current = 5;
    byFunction.dispatch(new PacketConnection({ sender: 4 }), packet);
    assert.deepEqual(functionCalls, [4]);
    const plain = makeDispatch();
    const plainCalls = [];
    registerStateMetaBattleDescription(plain, 7, (sender) => plainCalls.push(sender), () => {});
    plain.dispatch(new PacketConnection({ sender: 8 }), packet);
    assert.deepEqual(plainCalls, [8]);
});

test('수신기: CS 가 0 이면 자식 장치가 없어도 되고 비영인데 없으면 명시 오류이다', () => {
    const dispatch = makeDispatch();
    registerStateMetaBattleDescription(dispatch, { value: 0 }, undefined, () => {});
    assert.doesNotThrow(() => dispatch.dispatch(new PacketConnection({ sender: 1 }), makePacket(1)));
    const needs = makeDispatch();
    registerStateMetaBattleDescription(needs, { value: 1 }, undefined, () => {});
    assert.throws(() => needs.dispatch(new PacketConnection({ sender: 1 }), makePacket(1)), /미연결/);
});

test('분배: 버전 정책 0 이라 다른 버전도 수신하며 선언 길이와 무관하게 통과한다', () => {
    const log = [];
    const dispatch = makeDispatch(log);
    const calls = [];
    registerStateMetaBattleDescription(dispatch, 1, (sender, packet) => calls.push(packet.length), () => {});
    // 선언 길이 0x2211(가변 길이 0xffff 라 검사하지 않음)·버전 5 이어도 수신한다.
    const packet = makePacket(3, 13, 5);
    assert.equal(dispatch.dispatch(new PacketConnection({ sender: 2 }), packet), true);
    assert.deepEqual(calls, [13]);
    assert.deepEqual(log, [['record', 0x2211]]);
});

test('Native 연결 수신(004b49c0)이 이미 있으면 같은 값만 재등록하고 독립 본문을 설치하지 않는다', () => {
    const baseline = { nativeEntry: 0x004b49c0 };
    const calls = [];
    const fake = {
        registration: (type) => { calls.push(['registration', type]); return { canonicalReceive: baseline }; },
        registerCanonical: (type, receiver) => calls.push(['registerCanonical', type, receiver]),
        observePacket: () => calls.push(['observePacket']), version: () => calls.push(['version']), requireLength: () => calls.push(['requireLength']), register: () => calls.push(['register'])
    };
    registerStateMetaBattleDescription(fake, 0, () => {}, () => {});
    assert.deepEqual(calls, [['registration', 0x71], ['registerCanonical', 0x71, baseline]]);
    // 다른 진입점이면 독립 본문을 쓴다.
    const other = [];
    const independent = {
        registration: () => ({ canonicalReceive: { nativeEntry: 0x004b49c1 } }),
        registerCanonical: () => other.push('canonical'),
        observePacket: () => other.push('observePacket'), version: () => other.push('version'), requireLength: () => other.push('requireLength'), register: () => other.push('register')
    };
    registerStateMetaBattleDescription(independent, 0, () => {}, () => {});
    assert.deepEqual(other, ['observePacket', 'version', 'requireLength', 'register']);
    // 등록 장치 자체가 없으면 명시 오류이다(이 웹 분배기에는 없다).
    assert.throws(() => registerStateMetaBattleDescription({ registration: () => ({ canonicalReceive: baseline }) }, 0, () => {}, () => {}), /미연결/);
});
