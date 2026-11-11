// 패킷 등록 표(원문 전역 수신/관찰/버전/길이 표 256칸)와 Session 대여를 옮긴 모듈이다(cpppj `o/PacketRegistrationTable.cpp/.h`, 10.78 전용: 마지막 타입 137).
// 사용: const table = new PacketRegistrationTable(); const lease = table.bindSession(owner); (RawPacketDispatch 가 등록 API 마다 table.touch(type) 로 변경 칸을 기록하고 entry(type) 을 고친다.)
//   lease.detach() 는 이 대여가 바꾼 칸만 대여 시작 때의 값으로 되돌린다. lease.pin() 은 수신기/관찰기 호출 동안 쓰는 활동 표식(release 로 반납)이며 활동 중에는 새 대여와 전체 초기화가 거부된다.
// 웹에는 소멸자와 약한 참조가 없다: C++ 에서 `Session 이 사라지면 대여가 스스로 반납`되던 일은 호출자가 `lease.detach()` 를 명시 호출한다(소유 Session 이 사라진 뒤 남은 등록이 호출되던 오류 경로는 도달할 수 없다).
// 기준선 뒤 cpppj 에서 늘어난 `Registration(type)`(0..last 읽기)·`ResetCallbacks()`(R00482b60 의 함수/버전 비우기)는 옮겼다. `PersistCanonicalBaseline`/`canonicalReceive`(장기 고정 수신기 승격)는
// 실제 Native 장기 등록 4-D 몫이라 옮기지 않았다(3.6절 ③). CD(10.72)는 웹 범위 밖이라 옮기지 않았다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-11 WB04-E305: 4-C PacketRegistrationTable 이식)
import { PATCH_PACKET_LENGTHS, VARIABLE_PACKET_LENGTH } from './raw-packet-lengths.js';

// 등록 슬롯 수(원문 호스트 물리 폭)와 10.78 의 마지막 지원 타입이다. (2026-11-11 WB04-E305)
const SLOT_COUNT = 256;
const LAST_TYPE = 137;

// 대여를 쓰지 않는 항목의 핀(활동 표식)이다: 반납해도 아무 일도 하지 않는다. (2026-11-11 WB04-E305)
const NO_PIN = Object.freeze({ release() {} });

// 원본 세 등록 표를 호스트 함수/값으로 옮긴 한 항목이다. receive(sender, connection, packet)는 처리기, observer(sending)는 방향 인자만 받는 관찰기,
// packetObserver(packet, sending)는 같은 원문 observer 슬롯의 실제 패킷을 받는 관찰기이다(둘은 동시에 등록하지 않는다). length 0xffff 는 가변 길이이다. (2026-11-11 WB04-E305)
export class PacketRegistration {
    // 이 항목을 등록한 대여(전역 기본 등록이면 null)와 대여 등록 여부이다.
    registrationLease = null;
    leased = false;
    // 처리기·관찰기 두 종류·길이(0xffff 가변)·버전 BYTE·불일치 정책 BYTE 이다.
    receive = null;
    observer = null;
    packetObserver = null;
    length = VARIABLE_PACKET_LENGTH;
    version = 0;
    mismatch = 0;

    // 등록한 대여의 활동 표식을 만든다(전역 기본 등록은 표식이 없다). 반납된 대여의 등록이면 오류이다. 호출이 끝나면 반환값의 release() 를 부른다. (2026-11-11 WB04-E305)
    pin() {
        if (!this.leased) return NO_PIN;
        if (!this.registrationLease) throw new Error('이미 반납된 등록 세대의 콜백');
        return this.registrationLease.pin();
    }

    // 현재 등록된 한 종류의 관찰기만 실제 패킷/방향 인자로 호출한다: 패킷 관찰기가 있으면 그것, 없으면 방향 전용 관찰기이다. 호출 동안 핀을 유지하고
    // 콜백이 같은 표를 교체해도 이미 읽은 함수 사본을 끝까지 쓴다. (2026-11-11 WB04-E305)
    notifyObserver(packet, sending) {
        const pin = this.pin();
        try {
            const withPacket = this.packetObserver;
            if (withPacket) withPacket(packet, sending);
            else {
                const directionOnly = this.observer;
                if (directionOnly) directionOnly(sending);
            }
        } finally {
            pin.release();
        }
    }

    // 대여 시작 시 표 전체의 snapshot 에 쓰는 값 사본이다(함수 참조는 그대로 공유). (2026-11-11 WB04-E305)
    clone() {
        const copy = new PacketRegistration();
        Object.assign(copy, this);
        return copy;
    }
}

// 한 대여의 수신기/관찰기 활동 수를 올리는 표식이다. release 는 멱등이다. (2026-11-11 WB04-E305)
class CallbackPin {
    // 활동 수를 올린 대여(반납하면 null)이다.
    #lease;

    // 대여의 활동 수를 한 번 올린다. (2026-11-11 WB04-E305)
    constructor(lease) {
        this.#lease = lease;
        lease._enter();
    }

    // 활동 수를 한 번만 줄인다. (2026-11-11 WB04-E305)
    release() {
        if (!this.#lease) return;
        const lease = this.#lease;
        this.#lease = null;
        lease._leave();
    }
}

// 실제 원문 256개 등록 슬롯의 단일 owner 이다. 기본 길이는 10.78 표로 한 번만 설정하고 receiver/observer/version 은 0 으로 둔다. (2026-11-11 WB04-E305)
export class PacketRegistrationTable {
    // 256 칸의 등록 항목, 활성 등록 세대와 아직 활동이 남은 반납 세대이다.
    #entries;
    #current = null;
    #retiring = null;

    // 판본은 10.78('patch1078')만 받는다. (2026-11-11 WB04-E305)
    constructor(edition = 'patch1078') {
        if (edition !== 'patch1078') throw new TypeError('패킷 등록 표는 10.78(patch1078)만 지원합니다.');
        this.#entries = Array.from({ length: SLOT_COUNT }, () => new PacketRegistration());
        // 판본의 실제 지원 타입까지만 길이를 복사한다.
        for (let index = 0; index <= LAST_TYPE; index += 1) this.#entries[index].length = PATCH_PACKET_LENGTHS[index];
    }

    // 다른 판본 표의 잘못된 공유를 구성 때 거부하는 한도(마지막 지원 타입)이다. (2026-11-11 WB04-E305)
    get lastType() {
        return LAST_TYPE;
    }

    // 활성/반납 세대의 콜백이 실행 중이면 참이다: 전체 Runtime 정리를 시작하지 않는다. (2026-11-11 WB04-E305)
    connectionCallbackActive() {
        return Boolean((this.#current && this.#current.calls) || (this.#retiring && this.#retiring.calls));
    }

    // 실제 Session 전체 owner 를 빌려 새 등록 세대를 만든다. 진행 중이거나 중복 대여는 거부하고 표를 바꾸지 않는다. (2026-11-11 WB04-E305)
    bindSession(owner) {
        if (!owner) throw new TypeError('등록 대여는 실제 Session 전체 owner가 필요합니다');
        if (this.#current) throw new Error('패킷 등록 Session 중복 대여');
        if (this.#retiring && this.#retiring.calls) throw new Error('이전 Session 콜백 복귀 전 새 등록 대여 금지');
        const lease = new PacketRegistrationLease(this, owner, this.#entries.map((entry) => entry.clone()));
        this.#current = lease;
        return lease;
    }

    // 등록 API 가 변경 전에 호출한다: 현재 대여의 변경 칸과 항목의 대여 표식을 기록한다. 대여가 없으면 전역 등록으로 표식을 지우되
    // 반납 도중이나 반납된 세대의 콜백 안에서는 전역 등록 수정을 거부한다. (2026-11-11 WB04-E305)
    touch(type) {
        const entry = this.entry(type);
        if (this.#current && this.#current.active) {
            this.#current.touched.add(type);
            entry.registrationLease = this.#current;
            entry.leased = true;
            return;
        }
        if (this.#current) throw new Error('등록 대여 반납 도중 전역 등록 수정 금지');
        if (this.#retiring && this.#retiring.calls) throw new Error('반납된 Session 콜백 안에서 전역 등록 수정 금지');
        entry.registrationLease = null;
        entry.leased = false;
    }

    // 분배기가 읽고 고치는 변경 가능한 항목이다(0..255 정수). 읽기 전용 조회는 `registration` 을 쓴다. (2026-11-11 WB04-E305)
    entry(type) {
        if (!Number.isInteger(type) || type < 0 || type >= SLOT_COUNT) throw new RangeError('패킷 등록 슬롯 범위 오류');
        return this.#entries[type];
    }

    // Native 4dedb0 의 Last+1 할당/0 초기화 범위(0..last)만 읽는다. 0 은 정상 슬롯이며 호스트 256 물리 폭으로 범위를 넓히지 않는다. (2026-11-11 WB04-E305: 기준선 뒤 cpppj 반영)
    registration(type) {
        if (!Number.isInteger(type) || type < 0 || type > LAST_TYPE) throw new RangeError('패킷 observer 표의 실제 할당 범위 밖');
        return this.#entries[type];
    }

    // R00482b60 처럼 지원 슬롯 0..last 의 함수/관찰기/버전/정책을 모두 비우고 길이표와 항목 주소는 보존한다. Session 대여/실행 중에는 거부한다. (2026-11-11 WB04-E305: 기준선 뒤 cpppj 반영)
    resetCallbacks() {
        if (this.#current || this.connectionCallbackActive()) throw new Error('Session 대여/콜백 실행 중 패킷 전역 초기화 금지');
        // 원문 last+1 개 receiver/observer·두 버전 BYTE 를 같은 순서로 비운다.
        for (let type = 0; type <= LAST_TYPE; type += 1) {
            const entry = this.#entries[type];
            entry.receive = null;
            entry.observer = null;
            entry.packetObserver = null;
            entry.version = 0;
            entry.mismatch = 0;
            entry.registrationLease = null;
            entry.leased = false;
        }
    }

    // 대여 반납 때 표가 이 세대의 칸을 되돌리는 내부 연결이다(대여 전용). (2026-11-11 WB04-E305)
    _release(lease, previous) {
        if (this.#current !== lease) return;
        // 이 세대가 바꾼 칸만 대여 시작 때의 값으로 제자리(같은 항목 객체)에서 되돌린다. 바꾸지 않은 전역 칸은 보존한다.
        for (let type = 1; type <= LAST_TYPE; type += 1) if (lease.touched.has(type)) Object.assign(this.#entries[type], previous[type]);
        this.#retiring = lease;
        this.#current = null;
    }
}

// 한 Session 이 등록한 칸을 추적해 반납 때 되돌리는 등록 세대이다. (2026-11-11 WB04-E305)
export class PacketRegistrationLease {
    // 표·소유 Session·시작 시 snapshot 이다.
    #table;
    #owner;
    #previous;
    #active = true;
    // 이 세대의 등록 API 가 바꾼 칸 집합과 실행 중인 콜백 수(반납 뒤 진행 중 콜백이 끝나기 전 새 세대 등록을 거부)이다.
    touched = new Set();
    calls = 0;

    // 표만 만들 수 있다(bindSession). (2026-11-11 WB04-E305)
    constructor(table, owner, previous) {
        this.#table = table;
        this.#owner = owner;
        this.#previous = previous;
    }

    // 이 대여의 소유 Session 이다. (2026-11-11 WB04-E305)
    get owner() {
        return this.#owner;
    }

    // 종료된 대여를 현재 수신기로 다시 쓰지 않는다. (2026-11-11 WB04-E305)
    get active() {
        return this.#active;
    }

    // 한 번만 현재 표를 복원한다: 새 세대나 다른 전역 칸을 지우지 않고 변경 칸만 대여 시작 값으로 되돌린다. (2026-11-11 WB04-E305)
    detach() {
        if (!this.#active) return;
        this.#active = false;
        this.#table._release(this, this.#previous);
    }

    // 수신기/관찰기 호출 동안 쓰는 활동 표식을 만든다. 반납된 대여의 콜백은 거부한다. (2026-11-11 WB04-E305)
    pin() {
        if (!this.#active) throw new Error('반납된 Session 등록 콜백 호출');
        return new CallbackPin(this);
    }

    // 활동 수를 올리고 내린다(핀 전용). (2026-11-11 WB04-E305)
    _enter() {
        this.calls += 1;
    }

    // 활동 수를 내린다. (2026-11-11 WB04-E305)
    _leave() {
        this.calls -= 1;
    }
}
