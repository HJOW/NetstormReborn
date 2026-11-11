// 패킷 등록 표와 Session 대여(cpppj `o/PacketRegistrationTable`)의 웹 대응 `core/packet-registration-table.js` 를 검사한다(4-C, WB04-E305).
// 원본 관측 파일이 없는 단위 계약 검사이다(cpppj 시험은 표를 쓰는 Dispatch/Session 시험 안에서 간접 확인): 구성(길이 표·0 초기값), 대여의 중복·활동 중 거부, 변경 칸만 복원하는 반납,
// 활동 핀과 반납 뒤 콜백 거부, 읽기 범위, 전역 초기화 거부 조건, 관찰기 선택. 실제 게임·브라우저 실행이 아니다. (2026-11-11 WB04-E305)
import assert from 'node:assert/strict';
import test from 'node:test';
import { PacketRegistration, PacketRegistrationLease, PacketRegistrationTable } from '../www/core/packet-registration-table.js';
import { PATCH_PACKET_LENGTHS } from '../www/core/raw-packet-lengths.js';

// 구성: 10.78 만 받고, 마지막 타입 137, 길이는 표에서, 나머지는 0/가변이다. (2026-11-11 WB04-E305)
test('등록 표: 구성과 초기값', () => {
    assert.throws(() => new PacketRegistrationTable('cd1072'), TypeError);
    const table = new PacketRegistrationTable();
    assert.equal(table.lastType, 137);
    // 지원 타입 0..137 의 모든 칸이 표 길이와 0 초기값을 가진다.
    for (let type = 0; type <= 137; type += 1) {
        const entry = table.entry(type);
        assert.deepEqual([entry.length, entry.version, entry.mismatch, entry.receive, entry.observer, entry.packetObserver, entry.leased], [PATCH_PACKET_LENGTHS[type], 0, 0, null, null, null, false], `type ${type}`);
    }
    // 지원 타입 밖의 물리 칸은 가변 길이 기본값이며 읽기 전용 조회는 0..last 만 허용한다.
    assert.equal(table.entry(200).length, 0xffff);
    assert.doesNotThrow(() => table.entry(255));
    assert.equal(table.registration(0), table.entry(0));
    assert.throws(() => table.registration(138), RangeError);
    assert.throws(() => table.entry(256), RangeError);
    assert.throws(() => table.entry(-1), RangeError);
    assert.throws(() => table.entry(1.5), RangeError);
});

// 대여: owner 필요, 중복 거부, 반납 뒤 새 대여, 활동 중 새 대여 거부. (2026-11-11 WB04-E305)
test('등록 표: 대여의 중복·활동 중 거부', () => {
    const table = new PacketRegistrationTable();
    assert.throws(() => table.bindSession(null), TypeError);
    const owner = {};
    const lease = table.bindSession(owner);
    assert.ok(lease instanceof PacketRegistrationLease);
    assert.deepEqual([lease.owner, lease.active, lease.calls], [owner, true, 0]);
    assert.throws(() => table.bindSession({}), /중복 대여/);
    // 반납한 뒤에는 새 대여를 받는다.
    lease.detach();
    assert.equal(lease.active, false);
    const second = table.bindSession({});
    // 반납된 세대의 콜백이 실행 중이면 새 대여를 거부한다.
    const pin = second.pin();
    second.detach();
    assert.equal(table.connectionCallbackActive(), true);
    assert.throws(() => table.bindSession({}), /콜백 복귀 전/);
    pin.release();
    pin.release();
    assert.equal(table.connectionCallbackActive(), false);
    table.bindSession({}).detach();
});

// 반납: 대여가 touch 한 칸만 시작 값으로 되돌리고 전역 칸과 같은 항목 객체는 보존한다. (2026-11-11 WB04-E305)
test('등록 표: 반납은 변경 칸만 같은 항목에서 복원', () => {
    const table = new PacketRegistrationTable();
    // 대여 전 전역 등록 두 개.
    table.touch(0x37);
    table.entry(0x37).version = 1;
    table.entry(0x37).receive = 'global';
    table.touch(20);
    table.entry(20).version = 5;
    assert.equal(table.entry(0x37).leased, false);
    const lease = table.bindSession({});
    const held = table.entry(0x37);
    // 대여 중 0x37 을 바꾸고 20 은 touch 없이 직접 바꾼다(전역 직접 수정은 복원되지 않음).
    table.touch(0x37);
    table.entry(0x37).version = 2;
    table.entry(0x37).receive = 'session';
    table.entry(20).version = 9;
    assert.deepEqual([held.leased, held.registrationLease === lease, table.entry(20).leased], [true, true, false]);
    lease.detach();
    // 같은 항목 객체가 대여 시작 값으로 돌아오고 touch 하지 않은 칸은 그대로이다.
    assert.equal(table.entry(0x37), held);
    assert.deepEqual([held.version, held.receive, held.leased, held.registrationLease], [1, 'global', false, null]);
    assert.equal(table.entry(20).version, 9);
    // 반납 뒤 detach 는 아무 것도 바꾸지 않는다.
    held.version = 4;
    lease.detach();
    assert.equal(held.version, 4);
});

// touch: 대여가 없으면 전역 등록(표식 지움), 반납 직후 활동이 남은 세대의 콜백 안에서는 전역 수정 거부. (2026-11-11 WB04-E305)
test('등록 표: touch 의 전역·반납 도중 규칙', () => {
    const table = new PacketRegistrationTable();
    const lease = table.bindSession({});
    table.touch(30);
    assert.equal(table.entry(30).leased, true);
    const pin = lease.pin();
    lease.detach();
    assert.throws(() => table.touch(31), /반납된 Session 콜백/);
    pin.release();
    table.touch(31);
    assert.equal(table.entry(31).leased, false);
    assert.throws(() => table.touch(300), RangeError);
});

// 핀: 활동 수, 전역 항목은 표식 없음, 반납 뒤 대여 등록의 핀은 거부, 대여 등록 항목의 핀은 그 대여의 활동이다. (2026-11-11 WB04-E305)
test('등록 표: 활동 핀', () => {
    const table = new PacketRegistrationTable();
    const noPin = table.entry(40).pin();
    assert.doesNotThrow(() => noPin.release());
    assert.equal(table.connectionCallbackActive(), false);
    const lease = table.bindSession({});
    table.touch(40);
    const pin = table.entry(40).pin();
    assert.deepEqual([lease.calls, table.connectionCallbackActive()], [1, true]);
    pin.release();
    assert.deepEqual([lease.calls, table.connectionCallbackActive()], [0, false]);
    // 반납된 대여의 등록은 복사본을 통해서도 핀을 만들 수 없다.
    const copy = table.entry(40).clone();
    lease.detach();
    assert.throws(() => copy.pin(), /반납된 Session 등록 콜백/);
    assert.throws(() => lease.pin(), /반납된 Session 등록 콜백/);
    // 항목이 대여 표식을 잃은 채 leased 만 남은 상태는 오류이다.
    const broken = new PacketRegistration();
    broken.leased = true;
    assert.throws(() => broken.pin(), /반납된 등록 세대/);
});

// 관찰기: 패킷 관찰기가 우선하고 없으면 방향 전용, 둘 다 없으면 아무 일도 없다. 호출 중 핀을 유지하고 예외에도 반납한다. (2026-11-11 WB04-E305)
test('등록 표: 관찰기 선택과 핀 유지', () => {
    const table = new PacketRegistrationTable();
    const lease = table.bindSession({});
    table.touch(50);
    const entry = table.entry(50);
    const calls = [];
    entry.notifyObserver(Uint8Array.of(1), 1);
    entry.observer = (sending) => { calls.push(['direction', sending, lease.calls]); };
    entry.notifyObserver(Uint8Array.of(1), 0);
    entry.packetObserver = (packet, sending) => { calls.push(['packet', Array.from(packet), sending, lease.calls]); };
    entry.notifyObserver(Uint8Array.of(7), 1);
    // 호출 중 활동 수는 1 이고 끝나면 0 이다.
    assert.deepEqual(calls, [['direction', 0, 1], ['packet', [7], 1, 1]]);
    assert.equal(lease.calls, 0);
    entry.packetObserver = () => { throw new Error('관찰기 오류'); };
    assert.throws(() => entry.notifyObserver(Uint8Array.of(1), 1), /관찰기 오류/);
    assert.equal(lease.calls, 0);
    // 콜백이 표를 교체해도 이미 읽은 함수를 끝까지 쓴다.
    const seen = [];
    entry.packetObserver = () => { seen.push('first'); entry.packetObserver = () => { seen.push('second'); }; };
    entry.notifyObserver(Uint8Array.of(1), 1);
    assert.deepEqual(seen, ['first']);
    lease.detach();
});

// 전역 초기화: 0..last 의 함수/관찰기/버전/정책을 비우고 길이표와 항목 객체는 보존하며 대여·콜백 중에는 거부한다. (2026-11-11 WB04-E305)
test('등록 표: 전역 초기화', () => {
    const table = new PacketRegistrationTable();
    const entry = table.entry(0x37);
    Object.assign(entry, { receive: () => {}, observer: () => {}, packetObserver: () => {}, version: 3, mismatch: 7, length: 25 });
    table.entry(200).version = 5;
    table.entry(0).version = 4;
    table.resetCallbacks();
    assert.equal(table.entry(0).version, 0);
    assert.equal(table.entry(0x37), entry);
    assert.deepEqual([entry.receive, entry.observer, entry.packetObserver, entry.version, entry.mismatch, entry.length], [null, null, null, 0, 0, 25]);
    // 지원 범위 밖 칸은 건드리지 않는다.
    assert.equal(table.entry(200).version, 5);
    const lease = table.bindSession({});
    assert.throws(() => table.resetCallbacks(), /초기화 금지/);
    const pin = lease.pin();
    lease.detach();
    assert.throws(() => table.resetCallbacks(), /초기화 금지/);
    pin.release();
    table.resetCallbacks();
});
