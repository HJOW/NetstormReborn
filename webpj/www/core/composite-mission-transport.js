// 단독 미션의 내부 연결을 원문 하나의 Registry/등록표/현재 ID 순서로 조립하는 복합 전송 조정자를 옮긴 모듈이다
// (cpppj `client/CompositeMissionTransport.cpp/.h`, 10.78 전용, 기준선 1f00a25a 뒤 +97줄 중 웹 범위인 `InternalMission` 역할 부분).
// 조정자는 전송별 전체 Poll 을 이어 붙이지 않고 **현재 전역 ID 별로 실제 하위 호출만 선택**한다: Select(id) 는 현재 등록소 토큰이 없으면 Missing, 내부 전송(`LocalMissionTransport`, E324)의 끝이면 Internal, 같은 등록소의 다른 owner 가 가진 노드면 Unknown 이다.
// 사용: const transport = CompositeMissionTransport.create(CompositeTransportRole.InternalMission, players, server, hooks, registry, globalsLifetime);
//   transport.dispatch.register(0x27, …); transport.serve(port); transport.connect(address20, port); transport.poll(); transport.send(id, packet); transport.closeRoom();
// 웹 범위(LEFT_JOBS 1절): 역할은 `InternalMission` 만 지원한다. `Editor`(Edit 제외)·`LanHost`·`LanClient`(LAN TCP/UDP·Meta 제외)로 만들면 생성 때 명시 오류이며, 거기에만 쓰이는 TCP/UDP/Editor 하위 호출(`listenLan`·`SharedNetwork`·`SharesNetwork`·
//   `BindMetaSource`·`MetaSource`·`RebindWorld`·`EditorTransport`)과 기준선 뒤 TCP/UDP/Editor 분기는 옮기지 않았다. `globalsLifetime`(C++ `shared_ptr<void>`) 은 GC 가 대신하지만 호출 계약상 필수 인자로 유지한다.
// 기준선 뒤 cpppj 변경 판정(LEFT_JOBS 3.6절): ② 반영 — Serve 의 `IsOpen()` 이면 다시 열지 않음(`f98d50d5`), 내부 → Editor → TCP 수락 순서(`6b700418`), `ConnectPeerText`(`f7210282`), `ReceiveOne` 의 Unknown 노드 처리(같은 등록소의 다른 owner 가 소유한 살아 있는 노드는 그 전송에게 맡기고 등록표에도 없는 별칭만 오류, `a40c0028`),
//   `OwnsConnection`(`a76499e3`). 내부 끝은 생성자에서 만들어지므로 Poll 의 외곽 대여는 처음부터 시작된 상태라 `beginLateLocal`(LAN 서버 이전으로 내부 끝을 늦게 만들 때만 필요)은 옮기지 않았다.
//   ① — `shared_from_this` pin/`Registration(type).Pin()` 의 shared_ptr 수명 보유는 GC 가 대신함. ③ — `BindCompositeCampaignTransport`(campaign-transport-services.js 에 이 단위 뒤 추가).
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-12 WB04-E327: 4-C CompositeMissionTransport 이식)
import { LocalMissionTransport } from './local-mission-transport.js';
import { PacketRegistrationTable } from './packet-registration-table.js';
import { RawConnectionRegistry } from './raw-connection-registry.js';
import { currentTransportToken, transportPollResult } from './raw-transport-polling.js';

// 복합 전송이 고르는 역할 이름이다(C++ enum class CompositeTransportRole). 웹은 InternalMission 만 지원한다. (2026-11-12 WB04-E327)
export const CompositeTransportRole = Object.freeze({
    InternalMission: 'InternalMission',
    Editor: 'Editor',
    LanHost: 'LanHost',
    LanClient: 'LanClient',
});

// Player 한 명 기록의 크기와 Player 수이다(Player9 전체 = 9×180). (2026-11-12 WB04-E327)
const PLAYER_BYTES = 180;
const PLAYERS = 9;
// 원문 정상 연결 칸(0..21)과 pending 칸(-1..-31) 개수이다. (2026-11-12 WB04-E327)
const TRANSCEIVER_SLOTS = 22;
const PENDING_SLOTS = 32;

// 현재 토큰의 실제 하위 전송 구분이다. Unknown 은 같은 등록소의 다른 owner 노드라 이 조정자가 분배하지 않는다. (2026-11-12 WB04-E327)
const BACKEND = Object.freeze({ Missing: 'Missing', Internal: 'Internal', Unknown: 'Unknown' });

// 서버 권한 인자(boolean·{ value } 상자·함수)를 현재 값 조회 함수로 바꾼다. LocalMissionTransport 와 같은 형식만 받는다. (2026-11-12 WB04-E327)
function liveServer(source) {
    if (typeof source === 'function') return () => Boolean(source());
    if (source && typeof source === 'object' && 'value' in source) return () => Boolean(source.value);
    if (typeof source === 'boolean') return () => source;
    throw new TypeError('복합 전송의 서버 권한은 boolean·{ value } 상자·함수여야 합니다');
}

// 복합 전송 조정자이다. 단독 미션에서는 내부 두 끝 하나만 조립하지만 모든 호출이 현재 ID 의 실제 owner 를 매번 새로 선택하는 원문 구조를 보존한다. (2026-11-12 WB04-E327)
export class CompositeMissionTransport {
    // 선택된 역할이다(API 호출이 다른 역할을 조용히 성공시키지 않게 하는 값).
    #role;
    // 원문 현재 서버 권한 조회이며 값을 강제로 바꾸지 않는다.
    #server;
    // Player/server/시계·진단 hooks 가 빌리는 Client 물리 전체를 유지하는 장기 참조이다(GC 수명 보유).
    #globalsLifetime;
    // 모든 하위 전송과 State 가 함께 쓰는 고정 표이다.
    #registry;
    // 한 번만 초기화한 실제 전역 등록표이다.
    #registrations;
    // 단독/LAN host 자기끝의 실제 내부 listener/client/accepted 이다.
    #internal;
    // 조정자 자체의 Poll 재진입 방지 표시이다.
    #polling = false;

    // 사용: 정적 create 가 쓰는 실제 생성이다. 실제 Player9(1620BYTE)·등록소·장기 참조를 확인하고 내부 전송을 만든다. 생성만으로 listener/client 를 열지 않는다. (2026-11-12 WB04-E327)
    constructor(role, players, server, hooks, registry, globalsLifetime) {
        if (!globalsLifetime || !registry || !(players instanceof Uint8Array) || players.length !== PLAYERS * PLAYER_BYTES) throw new TypeError('복합 전송의 실제 Client 물리/Registry/Player9 누락');
        if (!Object.values(CompositeTransportRole).includes(role)) throw new TypeError('지원하지 않는 복합 전송 역할');
        if (role !== CompositeTransportRole.InternalMission) throw new Error(`웹 범위 밖: 복합 전송 역할 ${role} (Edit·LAN TCP/UDP·Meta 제외, InternalMission 만 지원)`);
        if (!(registry instanceof RawConnectionRegistry)) throw new TypeError('복합 전송의 연결 등록소 형식 오류');
        this.#role = role;
        this.#server = liveServer(server);
        this.#globalsLifetime = globalsLifetime;
        this.#registry = registry;
        this.#registrations = new PacketRegistrationTable('patch1078');
        this.#internal = new LocalMissionTransport(players, server, hooks, registry, this.#registrations);
    }

    // C++ `Create` 와 같은 정적 입구이다. (2026-11-12 WB04-E327)
    static create(role, players, server, hooks, registry, globalsLifetime) {
        return new CompositeMissionTransport(role, players, server, hooks, registry, globalsLifetime);
    }

    // C++ 소멸자: 자기 두 끝의 남은 별칭/장치만 반납한다(Player/등록 성공을 합성하지 않음). (2026-11-12 WB04-E327)
    dispose() {
        this.#internal.dispose();
    }

    // 현재 토큰의 실제 하위 전송을 고른다. 칸을 초기화하지 않고 callback 뒤의 현재 등록소를 매번 새로 읽는다. 범위 밖 ID 는 오류이다. (2026-11-12 WB04-E327)
    #select(id) {
        if (!currentTransportToken(this.#registry, id)) return BACKEND.Missing;
        if (this.#internal.connection(id)) return BACKEND.Internal;
        return BACKEND.Unknown;
    }

    // 서버 권한이 있을 때 내부 listener 만 연다. 이미 열린 내부 listener 는 다시 만들지 않는다(004e2430). 서버 권한이 없으면 거부한다. (2026-11-12 WB04-E327: 기준선 뒤 IsOpen 관문 반영)
    serve(port) {
        if (!this.#server()) throw new Error('서버 권한 없는 내부/LAN host listener 요청');
        if (!this.#internal.isOpen) this.#internal.open(port, false);
    }

    // LAN TCP listener 는 웹 범위 밖이다(simulconnect LAN client 용). 조용한 성공으로 바꾸지 않고 명시 거부한다. (2026-11-12 WB04-E327)
    listenLan() {
        throw new Error('웹 범위 밖: LAN TCP listener 요청');
    }

    // 단독 자기 로그인은 주소를 해석하지 않고 내부 client9 를 만든다. State8 주소는 20BYTE 여야 한다. (2026-11-12 WB04-E327)
    connect(address, port) {
        if (!address || address.length !== 20) throw new RangeError('현재 State8 주소20BYTE 누락');
        this.#internal.connect();
    }

    // 원문 저장 상태의 문자 주소도 명시 내부 역할에서는 기존 zlocal 끝만 연다(TCP/DNS 없음). (2026-11-12 WB04-E327: 기준선 뒤 ConnectPeerText 반영)
    connectPeerText(address, port) {
        this.#internal.connect();
    }

    // 현재 accepted 노드(내부 끝)에만 원문 재배치를 실행하며 별도 pending 번호를 만들지 않는다. (2026-11-12 WB04-E327)
    assign(pending, owner) {
        if (this.#select(pending) !== BACKEND.Internal) throw new Error('현재 accepted 노드 없는 복합 전송 재배치');
        this.#internal.assignConnection(pending, owner);
    }

    // 이미 길이를 확정한 writer 의 바이트를 현재 ID 의 실제 owner 가 보낸다. 연결이 없으면 false, 헤더 4BYTE 미만은 거부한다. (2026-11-12 WB04-E327)
    send(target, packet) {
        const selected = this.#select(target);
        if (selected === BACKEND.Missing) return false;
        if (packet.length < 4) throw new RangeError('복합 송신의 원문 헤더 부족');
        if (selected === BACKEND.Internal) return this.#internal.send(target, packet);
        throw new Error('복합 전송이 소유하지 않은 현재 송신 노드');
    }

    // observer(1) 와 현재 관문/큐는 선택된 하위 전송에서 한 번만 실행한다. (2026-11-12 WB04-E327)
    sendMutable(target, packet) {
        const selected = this.#select(target);
        if (selected === BACKEND.Missing) return false;
        if (packet.length < 4) throw new RangeError('복합 mutable 송신의 원문 헤더 부족');
        if (selected === BACKEND.Internal) return this.#internal.sendMutable(target, packet);
        throw new Error('복합 전송이 소유하지 않은 현재 mutable 송신 노드');
    }

    // 004e2920 의 현재 0..21 순서로 같은 ID 의 실제 하위 전송만 flush 한다. 다음 ID 의 토큰은 callback 뒤 새로 읽는다. (2026-11-12 WB04-E327)
    flush() {
        // 정상 연결 칸을 번호순으로 방문한다.
        for (let id = 0; id < TRANSCEIVER_SLOTS; id += 1) {
            const selected = this.#select(id);
            if (selected === BACKEND.Internal) this.#internal.flushOne(id);
            else if (selected === BACKEND.Unknown) throw new Error('복합 전송이 소유하지 않은 현재 flush 노드');
        }
    }

    // 현재 ID 의 한 프레임만 실제 하위 전송에 요청한다. 같은 등록소의 다른 owner 가 가진 살아 있는 노드는 그 전송의 Poll 몫이라 빈 결과이고, 등록소에도 없는 별칭만 오류이다. (2026-11-12 WB04-E327: 기준선 뒤 Unknown 처리 반영)
    #receiveOne(id) {
        const selected = this.#select(id);
        if (selected === BACKEND.Internal) return this.#internal.receiveOne(id);
        if (selected === BACKEND.Missing) return transportPollResult();
        const token = currentTransportToken(this.#registry, id);
        if (this.#registry.owns(token)) return transportPollResult();
        throw new Error(`복합 전송이 소유하지 않은 현재 수신 노드: id=${id} token=${token} 역할=${this.#role}`);
    }

    // 004e28b0: 내부 listener 수락 → 각 pending 한 번 → 정상 0..21(ID 별 again 이 거짓일 때까지)을 한 번만 조립한다. 중첩 호출은 거부하고, 수신 중 오류가 나도 내부 전송의 외곽 대여를 반납한 뒤 원래 오류를 전파한다. (2026-11-12 WB04-E327)
    poll() {
        if (this.#polling) throw new Error('복합 전송 Poll 중첩 호출');
        this.#polling = true;
        let count = 0;
        let begun = false;
        try {
            try {
                this.#internal.beginPoll();
                begun = true;
                this.#internal.acceptPending();
                // 004df560: 현재 각 pending 칸을 한 번씩 방문한다.
                for (let id = -1; id > -PENDING_SLOTS; id -= 1) {
                    if (this.#receiveOne(id).consumed) count += 1;
                }
                // callback 의 재배치/반납을 매 회 현재 표에서 반영하며 ID 별 sentinel 까지 반복한다.
                for (let id = 0; id < TRANSCEIVER_SLOTS; id += 1) {
                    for (;;) {
                        const result = this.#receiveOne(id);
                        if (result.consumed) count += 1;
                        if (!result.again) break;
                    }
                }
            } catch (failure) {
                // 원래 수신 실패가 우선이며 반납 실패는 삼킨다.
                if (begun) {
                    try { this.#internal.endPoll(); } catch { /* 원래 오류 보존 */ }
                }
                throw failure;
            }
            // 정상 수신 뒤 반납 실패는 그대로 전파한다.
            this.#internal.endPoll();
            return count;
        } finally {
            this.#polling = false;
        }
    }

    // 같은 등록소의 원문 개별 종료를 쓴다(현재 ID/버퍼/동기화/깊이와 실제 owner close/free). (2026-11-12 WB04-E327)
    close(id) {
        this.#registry.disconnect(id);
    }

    // 방 종료: 현재 자기 정상 노드(번호순)와 pending 노드만 Native 종료하고 다른 owner 의 공유 표는 보존한 뒤 내부 두 끝의 지연 반납을 마무리한다. (2026-11-12 WB04-E327)
    closeRoom() {
        // 정상 칸을 원문 번호순으로 닫는다.
        for (let id = 0; id < TRANSCEIVER_SLOTS; id += 1) {
            const backend = this.#select(id);
            if (backend !== BACKEND.Missing && backend !== BACKEND.Unknown) this.#registry.disconnect(id);
        }
        // pending 은 -1..-31 순서로 닫는다(실제 개수/깊이는 같은 등록소 부모가 처리).
        for (let id = -1; id > -PENDING_SLOTS; id -= 1) {
            const backend = this.#select(id);
            if (backend !== BACKEND.Missing && backend !== BACKEND.Unknown) this.#registry.disconnect(id);
        }
        this.#internal.closeAll();
    }

    // ID 가 현재 자기 노드인지 확인한 뒤에만 같은 고정 노드를 빌려 준다. 없으면 null, 다른 owner 의 것이면 오류이다. (2026-11-12 WB04-E327)
    connection(id) {
        const selected = this.#select(id);
        if (selected === BACKEND.Missing) return null;
        if (selected === BACKEND.Internal) return this.#internal.connection(id);
        throw new Error('다른 owner의 복합 연결 주소 조회');
    }

    // 객체 존재만 읽으며 접속 완료/Ready 를 만들지 않는다. (2026-11-12 WB04-E327)
    exists(id) {
        return this.connection(id) !== null;
    }

    // 하위 전송의 수명과 상위 닫힘을 실제 선택 전송에서 읽는다. (2026-11-12 WB04-E327)
    isOpen(id) {
        const selected = this.#select(id);
        if (selected === BACKEND.Missing) return false;
        if (selected === BACKEND.Internal) return this.#internal.connectionOpen(id);
        throw new Error('다른 owner의 복합 연결 open 조회');
    }

    // 내부 전송의 즉시 완료(1)/아니면 0 이며 buffered bit 2 는 읽지 않는다. (2026-11-12 WB04-E327)
    status(id) {
        const selected = this.#select(id);
        if (selected === BACKEND.Missing) return 0;
        if (selected === BACKEND.Internal) return this.#internal.connectionStatus(id);
        throw new Error('다른 owner의 복합 연결 status 조회');
    }

    // 같은 등록소의 현재 20BYTE 주소를 읽는다(먼저 현재 자기 노드인지 확인, IPv4/포트 바이트 순서를 고치지 않음). (2026-11-12 WB04-E327)
    address(id) {
        this.connection(id);
        return this.#registry.address(id);
    }

    // 매 송신마다 같은 현재 ID 를 다시 조회해 보내는 함수를 만든다(예전 하위 전송/노드를 보관하지 않음). owner 는 32비트 부호 있는 ID 로 해석한다. (2026-11-12 WB04-E327)
    peer(owner) {
        return (packet) => { this.send(owner | 0, packet); };
    }

    // 모든 하위 전송이 공유하는 등록표의 canonical 편집 API(분배기)이다. (2026-11-12 WB04-E327)
    get dispatch() {
        return this.#internal.dispatch;
    }

    // State/Chat/cleanup 에 같은 현재 표를 빌려 준다. (2026-11-12 WB04-E327)
    get registry() {
        return this.#registry;
    }

    // 실제 Session/World 전체 owner 를 빌려 등록 대여를 만든다(약하게 빌리고 콜백 때만 pin). (2026-11-12 WB04-E327)
    bindSession(owner) {
        return this.#registrations.bindSession(owner);
    }

    // 반납 중인 콜백까지 포함해 전체 State/Kernel 정리 진입을 막는 관문이다. (2026-11-12 WB04-E327)
    connectionCallbackActive() {
        return this.#registrations.connectionCallbackActive();
    }

    // 이미 선택된 역할 조회(게임 권한을 쓰지 않음). (2026-11-12 WB04-E327)
    get role() {
        return this.#role;
    }

    // 실제 하위 전송이 소유한 노드인지(반납 대기 중인 노드 포함) 식별한다. (2026-11-12 WB04-E327: 기준선 뒤 OwnsConnection 반영)
    ownsConnection(connection) {
        return this.#internal.ownsConnection(connection);
    }
}
