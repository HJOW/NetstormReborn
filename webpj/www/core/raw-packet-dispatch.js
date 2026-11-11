// 패킷 등록·길이 검증·버전 정책·즉시 수신기 분배를 원본 Zocket 몸체에서 옮긴 모듈이다(cpppj `o/RawPacketDispatch.cpp/.h`, 10.78 전용: 004ded50/70/90·004df240·004df740·004df010).
// 사용: const dispatch = new RawPacketDispatch({ now, record, terminate, close, release }, registrationTable); dispatch.register(type, (sender, connection, packet) => …);
//   dispatch.observe/observePacket/version/requireLength 로 해당 타입의 표만 바꾸고, dispatch.validate(packet) 은 헤더 4바이트·타입·선언 길이를 검사하며,
//   dispatch.dispatch(connection, packet) 은 닫힌 연결/버전 종료/콜백 뒤 소멸이면 false, 무시한 패킷/성공이면 true 를 돌려준다. 콜백에 전달한 버퍼는 호출 동안만 유효하다.
// 등록 API 는 변경 전에 등록 표의 `touch(type)` 로 현재 대여의 변경 칸을 기록한다(PacketRegistrationTable). 같은 표를 여러 backend 가 공유하며 시계·기록·연결 수명 효과(hooks)는 backend 고유이다.
// 기준선 뒤 cpppj 에서 늘어난 즉시 직접 호출(`registerDirect`/`invokeDirect`: NULL 연결을 허용하는 내부 호출)과 선택 장치(`sent` 송신 기록, `players` 현재 Player9 조회)·`packetPlayerActive` 를 옮겼다.
// 옮기지 않은 것: 고정 Native 수신기(CanonicalPacketReceiver)와 실제 소켓 지연 FIFO(delayQueue)는 LAN/실소켓 전송이라 웹 범위 밖이며(1절), 타입 4 집합 패킷은 공통 프레임 부모(후속 RawPacketFrame)가 필요해 기준선처럼
// 거부한다. 지연 표시가 있는 연결에 수신기가 있으면 거부한다. CD(10.72)는 옮기지 않았다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-11 WB04-E306: 4-C RawPacketDispatch 이식)
import { PacketRegistrationTable } from './packet-registration-table.js';

// 10.78 의 마지막 지원 타입이다. (2026-11-11 WB04-E306)
const LAST_TYPE = 137;

// 연결이 닫혔음을 뜻하는 flags 비트(0x40)이다: 이 비트가 켜진 연결에는 진입하지 않는다. (2026-11-11 WB04-E306)
const CLOSED = 0x40;

// 건설 수신 패킷 타입(0x37)과 Player 기록 폭/활성 오프셋이다. (2026-11-11 WB04-E306)
const CONSTRUCTION_TYPE = 0x37;
const PLAYER_STRIDE = 180;
const PLAYER_COUNT = 9;
const PLAYER_ACTIVE_OFFSET = 0x50;

// 원본 연결 객체에서 단일 분배가 읽고 쓰는 값이다(호스트 소켓 포인터 대신 명시적인 참조를 전달한다). flags(+0x34) 0x40 이면 닫힘, sender(+0x40)는 처리기의 첫 인자,
// received(+0x44)/heartbeat(+0x48)는 일반/타입 6 패킷의 마지막 수신 시각, depth(+0x8c)는 즉시 콜백 중 증가하는 중첩 깊이, delayed(+0x7c)는 지연 큐 표시, address(+0x4c..5f)는 20BYTE 주소이다. (2026-11-11 WB04-E306)
export class PacketConnection {
    flags = 0;
    sender = 0;
    received = 0;
    heartbeat = 0;
    depth = 0;
    delayed = false;
    address = new Uint8Array(20);

    // 필드를 한꺼번에 지정해 만든다(생략한 필드는 0). (2026-11-11 WB04-E306)
    constructor(fields = {}) {
        Object.assign(this, fields);
    }
}

// 패킷 선언 길이(헤더 +2 WORD, little endian)이다. 호출자가 최소 4바이트를 확인한 뒤 읽는다. (2026-11-11 WB04-E306)
function declaredLength(packet) {
    return packet[2] | (packet[3] << 8);
}

// 필수 hooks 의 종류를 확인한다: 시계(now)·기록(record)·종료(terminate)·소멸(close)·해제(release)는 비울 수 없다. sent/players 는 선택 장치이다. (2026-11-11 WB04-E306)
function checkHooks(hooks) {
    // 다섯 필수 효과가 모두 함수인지 본다.
    for (const name of ['now', 'record', 'terminate', 'close', 'release']) {
        if (typeof hooks?.[name] !== 'function') throw new TypeError('패킷 분배 경계/공유 등록표 판본 불일치');
    }
    // 선택 장치는 있으면 함수여야 한다.
    for (const name of ['sent', 'players']) {
        if (hooks[name] !== undefined && hooks[name] !== null && typeof hooks[name] !== 'function') throw new TypeError(`패킷 분배 선택 장치 ${name} 는 함수여야 합니다.`);
    }
}

// 4e2dc0 의 선택된 owner 기록 +0x50 의 낮은 BYTE 만 현재 실제 Player9 에서 읽는다. current 가 있으면 매 호출 현재 base(Player 재할당 뒤에도 최신)를, 없으면 fallback 을 쓴다. (2026-11-11 WB04-E306)
export function packetPlayerActive(fallback, current, owner) {
    const players = current ? current() : fallback;
    if (players?.length !== PLAYER_COUNT * PLAYER_STRIDE || !Number.isInteger(owner) || owner < 0 || owner >= PLAYER_COUNT) throw new RangeError('송신의 실제 현재 Player9 물리 부족');
    return (players[owner * PLAYER_STRIDE + PLAYER_ACTIVE_OFFSET] & 1) !== 0;
}

// 단일 검증/기록과 즉시 수신기 선택을 수행하는 분배기이다. (2026-11-11 WB04-E306)
export class RawPacketDispatch {
    // 공유 등록 표와 backend 고유 시계·기록·연결 수명 효과이다.
    #registrations;
    #hooks;

    // 등록 표를 주지 않으면 새 표를 만든다. 다른 판본의 표나 빠진 필수 효과는 거부한다. (2026-11-11 WB04-E306)
    constructor(hooks, registrations = null) {
        checkHooks(hooks);
        const table = registrations ?? new PacketRegistrationTable();
        if (!(table instanceof PacketRegistrationTable) || table.lastType !== LAST_TYPE) throw new TypeError('패킷 분배 경계/공유 등록표 판본 불일치');
        this.#registrations = table;
        this.#hooks = hooks;
    }

    // Local/TCP/Editor 가 같은 등록 표 owner 를 구성 때 공유한다. (2026-11-11 WB04-E306)
    get registrations() {
        return this.#registrations;
    }

    // 현재 Player9 조회와 송신 기록 장치는 선택이며 없으면 null 이다. (2026-11-11 WB04-E306)
    get hooks() {
        return this.#hooks;
    }

    // 잘못된 등록 번호가 다른 항목을 덮어쓰지 않게 한다: 타입 0 과 마지막 지원 타입 밖은 거부한다. (2026-11-11 WB04-E306)
    #check(type) {
        if (!Number.isInteger(type) || type < 1 || type > LAST_TYPE) throw new RangeError('패킷 등록 타입 범위 오류');
    }

    // 각 원본 등록 함수처럼 한 표만 바꾼다: 일반 등록은 이전 직접 수신기를 함께 제거한다. receive(sender, connection, packet). (2026-11-11 WB04-E306)
    register(type, receive) {
        this.#check(type);
        this.#registrations.touch(type);
        const entry = this.#registrations.entry(type);
        entry.receive = receive ?? null;
        entry.directReceive = null;
    }

    // NULL 연결을 허용하는 직접 수신기를 등록한다: 일반 프레임 분배는 실제 연결을, 직접 호출은 선택한 연결(null 가능)을 넘기며 두 입구가 같은 함수 상태를 쓴다. 빈 값이면 두 입구를 모두 지운다. (2026-11-11 WB04-E306)
    registerDirect(type, receive) {
        this.#check(type);
        this.#registrations.touch(type);
        const entry = this.#registrations.entry(type);
        if (!receive) {
            entry.receive = null;
            entry.directReceive = null;
            return;
        }
        const shared = receive;
        // 모든 래퍼를 먼저 만든 뒤 같은 항목에 한꺼번에 교체한다.
        entry.directReceive = (sender, connection, packet) => shared(sender, connection, packet);
        entry.receive = (sender, connection, packet) => shared(sender, connection, packet);
    }

    // 해당 타입의 관찰기 표만 바꾼다. 단일 분배는 등록된 관찰기를 호출하지 않는다. 새 방향 전용 등록은 이전 패킷 관찰기를 완전히 교체한다. observer(sending). (2026-11-11 WB04-E306)
    observe(type, observer) {
        this.#check(type);
        this.#registrations.touch(type);
        const entry = this.#registrations.entry(type);
        entry.observer = observer ?? null;
        entry.packetObserver = null;
    }

    // 원본 thiscall observer 에 실제 변경 가능 packet 을 전달하는 같은 슬롯의 등록이다: 이전 방향 전용 장치를 완전히 교체한다. observer(packet, sending). (2026-11-11 WB04-E306)
    observePacket(type, observer) {
        this.#check(type);
        this.#registrations.touch(type);
        const entry = this.#registrations.entry(type);
        entry.packetObserver = observer ?? null;
        entry.observer = null;
    }

    // 해당 타입의 버전 BYTE 와 불일치 정책 BYTE 만 바꾼다: 정책은 높은 버전이면 4 종료/1 무시, 낮은 버전이면 8 종료/2 무시, 0 이면 계속 분배한다. (2026-11-11 WB04-E306)
    version(type, version, mismatch) {
        this.#check(type);
        this.#registrations.touch(type);
        const entry = this.#registrations.entry(type);
        entry.version = version & 0xff;
        entry.mismatch = mismatch & 0xff;
    }

    // 원본 길이 표의 호출자 공급/검증용 변경이다. 0xffff 이면 선언 길이로 제한하지 않는다. (2026-11-11 WB04-E306)
    requireLength(type, length) {
        this.#check(type);
        this.#registrations.touch(type);
        this.#registrations.entry(type).length = length & 0xffff;
    }

    // 등록 상태를 읽거나 독립 원본 표 관찰과 비교한다(타입 1..last). (2026-11-11 WB04-E306)
    registration(type) {
        this.#check(type);
        return this.#registrations.entry(type);
    }

    // 최소 헤더 4바이트가 필요하다. 타입 10 이상은 처리기가 있어야 하며 그 밖에는 타입/선언 길이만 검사한다. 지원 밖 타입이면 false 이다. (2026-11-11 WB04-E306)
    validate(packet) {
        if (packet.length < 4) throw new RangeError('패킷 헤더 길이 부족');
        const type = packet[1];
        if (!type || type > LAST_TYPE) return false;
        const entry = this.#registrations.entry(type);
        return (type <= 9 || Boolean(entry.receive)) && (entry.length === 0xffff || entry.length === declaredLength(packet));
    }

    // 닫힌 연결/버전 종료/콜백 뒤 소멸이면 false, 무시한 패킷/성공이면 true 이다. 선언 길이와 실제 길이를 같게 강제하지 않으며 실제 필드 길이는 처리기가 검사한다.
    // 순서: 닫힘 → 검증 → (타입 4 거부) → 버전 정책 → 등록 핀 → 시각 기록 → 타입 10 이상 기록 → 수신기 → 깊이 0 이고 닫힘이면 소멸/해제. (2026-11-11 WB04-E306)
    dispatch(connection, packet) {
        if (connection.flags & CLOSED) return false;
        if (!this.validate(packet)) return true;
        const type = packet[1];
        if (type === 4) throw new Error('단일 분배의 집합 패킷은 공통 프레임 부모가 필요합니다');
        const incoming = packet[0] & 0x7f;
        const entry = this.#registrations.entry(type);
        if (incoming !== entry.version) {
            const lower = incoming < entry.version;
            if (entry.mismatch & (lower ? 8 : 4)) {
                this.#hooks.terminate(connection);
                return false;
            }
            if (entry.mismatch & (lower ? 2 : 1)) return true;
        }
        // 기록/시계 효과 안의 상태 전환도 같은 전체 정리 관문을 거치도록 등록 핀을 호출 끝까지 유지한다.
        const recordPin = entry.pin();
        try {
            const now = this.#hooks.now();
            if (type === 6) connection.heartbeat = now;
            else connection.received = now;
            if (type > 9) this.#hooks.record(connection, packet, declaredLength(packet));
            // 기록 경계가 등록을 바꾸면 현재 슬롯의 수신기를 한 번 선택한다.
            this.#deliver(type, connection, packet);
            if (!connection.depth && (connection.flags & CLOSED)) {
                this.#hooks.close(connection);
                this.#hooks.release(connection);
                return false;
            }
            return true;
        } finally {
            recordPin.release();
        }
    }

    // 4df740 이 효과 전에 저장한 type 의 현재 수신기를 선택해 호출한다: 수신기가 없으면 아무 일도 하지 않고, 지연 표시가 있는 연결(실소켓 FIFO)은 웹에 없어 거부한다.
    // 즉시 콜백 중에는 연결 깊이를 올리고 정상/예외 종료에서 복구하며 호출 중 표가 바뀌어도 이미 읽은 함수를 끝까지 쓴다. (2026-11-11 WB04-E306)
    #deliver(type, connection, packet) {
        const entry = this.#registrations.entry(type);
        if (!entry.receive && !entry.directReceive) return;
        if (connection.delayed) throw new Error('지연 표시가 있는 연결의 실제 FIFO 미연결');
        const pin = entry.pin();
        try {
            const receive = entry.receive;
            if (!receive) return;
            connection.depth += 1;
            try {
                receive(connection.sender, connection, packet);
            } finally {
                connection.depth -= 1;
            }
        } finally {
            pin.release();
        }
    }

    // 원문 함수 포인터 호출처럼 검증/observer·기록/시각/연결 깊이·닫힘을 추가하지 않고 등록 owner 핀만 유지한다. 직접 수신기는 선택한(null 가능) 연결로,
    // 일반 수신기는 실제 연결이 있어야 호출한다. (2026-11-11 WB04-E306)
    invokeDirect(type, sender, connection, packet) {
        this.#check(type);
        const entry = this.#registrations.entry(type);
        if (!entry.receive && !entry.directReceive) return;
        const pin = entry.pin();
        try {
            const direct = entry.directReceive;
            if (direct) {
                direct(sender, connection, packet);
                return;
            }
            const receive = entry.receive;
            if (!connection) throw new TypeError('NULL 연결을 지원하지 않는 일반 수신기 직접 호출');
            if (receive) receive(sender, connection, packet);
        } finally {
            pin.release();
        }
    }
}

// 두 원본 초기화처럼 0x37 의 observer=null/version=1/flags=0/건설 수신을 등록한다. data 는 분배기보다 오래 살아야 한다(`data.receive(packet)`). (2026-11-11 WB04-E306)
export function registerConstructionData(dispatch, data) {
    dispatch.observe(CONSTRUCTION_TYPE, null);
    dispatch.version(CONSTRUCTION_TYPE, 1, 0);
    // 원본 처리기의 sender/연결 인자는 건설 수신 몸체에서 사용하지 않는다.
    dispatch.register(CONSTRUCTION_TYPE, (sender, connection, packet) => { data.receive(packet); });
}
