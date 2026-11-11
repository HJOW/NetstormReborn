// 연결 숫자 토큰·고정 연결표·수명 장치의 등록소를 옮긴 모듈이다(cpppj `o/RawConnectionRegistry.cpp/.h`, 10.78 기준선 110줄 + 기준선 뒤 145줄 중 웹 범위 부분).
// 사용: const registry = new RawConnectionRegistry(); const token = registry.track(node, destroy, release[, send, readiness, buffered]);
//   registry.register(id, token) / accept(token) / assign(pending, target, server, buffered) / disconnect(id) / closeAll() / endPlayers(server) / closePending() …
//   Client 와 단독 미션의 두 연결 끝이 같은 등록소를 빌려 같은 22 연결·22 송신버퍼·32 pending 표와 동기화 토큰을 공유한다(`cleanup()` 이 같은 표 묶음의 보기를 돌려준다).
// 숫자 토큰은 연결 객체 주소가 아니라 이 등록소가 발급하는 재사용하지 않는 식별자(0x6d000000 부터)이며, 표와 장치는 해제된 노드를 다시 읽지 않는다.
// 정리·등록·재배치의 store 순서는 `raw-connection-cleanup.js`·`raw-connection-registration.js`(원본 관측 fixture 로 검증)의 자유 함수가 맡고, 이 클래스는 토큰 → 실제 연결/장치 해석과 수명만 더한다.
// 웹 범위 판정(LEFT_JOBS 3.6절): 기준선 뒤 추가분 중 ① 장치 소유자 고정(C++ shared_ptr 수명 보유: BindDevicePin/PinBinding/ReleaseDevicePins/deviceOwner)은 GC 가 대신하므로 옮기지 않음,
//   ③ 접속 통지(StoreConnectPacket/SendConnectPacket/RegisterConnectPacketReceiver, 004e2620·004e2f00: 실제 TCP 접속의 초기 패킷 복제)는 대응표 밖 신규 파일 RawPacketCopy 와 고정 Native 수신기에 기대며 LAN 전용이라 재기준선 때(4-D) 판정한다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-11 WB04-E308: 4-C RawConnectionRegistry 이식)
import { createConnectionCleanupGlobals, closeAllPendingConnections, closeAllTransceivers, deferConnectionClose, disconnectTransceiver, endPlayerConnections } from './raw-connection-cleanup.js';
import { assignPendingConnection, registerTransceiver } from './raw-connection-registration.js';

// 등록소가 발급하는 첫 숫자 토큰이며 호스트 식별자이다(원문 주소 아님, 재사용하지 않음). (2026-11-11 WB04-E308)
export const FIRST_CONNECTION_TOKEN = 0x6d000000;

// 원문 Ztrans.cpp 의 진단 줄 번호별 PE 문자열 표이다. 줄마다 실제 문자열을 쓰고 임의 공통 실패로 바꾸지 않는다. (2026-11-11 WB04-E308)
const NATIVE_ASSERTIONS = new Map([
    [0x1e, '-zaMAX_PENDING <= id && id < NUM_TRANSCEIVERS'],
    [0xb0, 'FIRST_REAL_PLAYER <= transId && transId < END_REAL_PLAYERS'],
    [0xb3, 'transId == GAME_SERVER_ID'],
    [0xb6, 'transceivers[transId] == NULL'],
    [0xbb, 'pendingId < 0'],
    [0xfa, 'id < NUM_TRANSCEIVERS'],
]);

// 선택한 실제 Client 의 진단 장치를 줄 번호의 원문 식과 함께 부른다. 장치가 반환하면 호출한 쪽이 이어서 진행한다. (2026-11-11 WB04-E308: 기준선 뒤 Native 경로)
function assertNative(assertion, line) {
    const expression = NATIVE_ASSERTIONS.get(line);
    if (expression === undefined) throw new RangeError('Native 연결의 미등록 원문 진단 행');
    if (typeof assertion !== 'function') throw new Error('Native 연결의 선택된 Client 진단 미연결');
    assertion(expression, '\\Ns\\O\\Ztrans.cpp', line);
}

// 표에서 index 칸을 읽는다(범위 밖은 오류, C++ `.at()` 와 같다). (2026-11-11 WB04-E308)
function entry(table, index) {
    if (!Number.isInteger(index) || index < 0 || index >= table.length) throw new RangeError('연결 등록소의 실제 표 범위');
    return table[index];
}

// 선택 장치 인자를 정규화한다: 비었으면 undefined, 함수가 아니면 형 오류이다. (2026-11-11 WB04-E308)
function optionalDevice(device, name) {
    if (device === undefined || device === null) return undefined;
    if (typeof device !== 'function') throw new TypeError(`연결 등록소의 ${name} 장치는 함수여야 합니다.`);
    return device;
}

// 연결 숫자 토큰·고정 표·수명 장치의 등록소이다. 실제 Client 가 선택한 Native 경로와 독립 typed 경로가 같은 본체를 쓴다. (2026-11-11 WB04-E308)
export class RawConnectionRegistry {
    // 사용: 독립 전송의 첫 BSS 0 표와 발급기만 만든다(복제·이동 없음: 같은 객체를 빌린다). 동기화 토큰은 외부에서 빌리기 전까지 이 등록소의 독립 상자이다. (2026-11-11 WB04-E308)
    constructor() {
        // 22 연결·22 송신버퍼·32 pending 표와 pending 개수 상자이다(동기화 상자는 아래 별도로 둔다).
        this.globals = createConnectionCleanupGlobals();
        // 토큰 → { node, destroy, release, send, buffered, readiness } 이다. 현재 생존 연결만 두고 해제된 노드의 주소는 남기지 않는다. 삽입 순서가 토큰(증가) 순서이다.
        this.bindings = new Map();
        // 다음 발급 토큰이다. unsigned 로 감겨 0 이 되면 소진으로 본다.
        this.nextToken = FIRST_CONNECTION_TOKEN;
        // 현재 동기화 토큰 상자이다. bindSynchronization 으로 Client 의 실제 상자를 빌리기 전에는 독립 값이다.
        this.synchronization = { value: 0 };
        // 응용 최종 종료 뒤 새 노드 발급을 막는 표시이다.
        this.deviceShutdown = false;
        // 정리 부모(원문 개별/전체 종료)가 자신의 칸·개수 쓰기를 마칠 때까지 일반 별칭 해제를 미루는 깊이이다.
        this.cleanupDepth = 0;
        // 최외곽 Native 부모의 호출 범위 깊이이다(마지막 표 쓰기까지). 응용 종료가 진행 중 부모 안에서 불리지 않게 구별한다.
        this.deviceCallDepth = 0;
    }

    // 같은 현재 동기화 토큰을 포함한 실제 표의 보기를 돌려준다(복사 아님: 표·상자는 모두 이 등록소의 것). 명시 상자를 주면 그것을 동기화로 빌린다(기존 직접 owner 검사용). (2026-11-11 WB04-E308)
    cleanup(synchronization = this.synchronization) {
        return { transceivers: this.globals.transceivers, buffers: this.globals.buffers, pending: this.globals.pending, synchronization, pendingCount: this.globals.pendingCount };
    }

    // 실제 005c8bf4 동기화 토큰 상자를 빌린다(현재 값은 쓰지 않는다). 소유자 수명 보유(원문 shared_ptr)는 GC 가 대신한다. (2026-11-11 WB04-E308: 기준선 뒤 Client 장기 표 연결)
    bindSynchronization(box) {
        if (!box || typeof box !== 'object' || !('value' in box)) throw new TypeError('동기화 토큰은 { value } 상자여야 합니다.');
        this.synchronization = box;
    }

    // 실제 노드와 수명(destroy/release)·선택 가상 송신(send)·소켓 준비 조회(readiness)·공통 buffered 입구를 같은 토큰에 묶고 토큰을 돌려준다. 선택하지 않은 장치는 쓰는 위치에서만 요구한다. (2026-11-11 WB04-E308)
    track(connection, destroy, release, send, readiness, buffered) {
        if (this.deviceShutdown) throw new Error('App최종종료뒤새Node등록거부');
        if (typeof destroy !== 'function' || typeof release !== 'function') throw new TypeError('실제 연결 close/free 장치 누락');
        const binding = { node: connection, destroy, release, send: optionalDevice(send, 'send'), buffered: optionalDevice(buffered, 'buffered'), readiness: optionalDevice(readiness, 'readiness') };
        // 한 노드의 살아 있는 토큰을 중복 발급하지 않는다.
        for (const existing of this.bindings.values()) {
            if (existing.node === connection) throw new Error('현재 연결의 중복 주소 등록');
        }
        if (!this.nextToken) throw new RangeError('연결 opaque 토큰 소진');
        const token = this.nextToken;
        this.nextToken = (token + 1) >>> 0;
        // 실제 할당이 성공한 뒤에만 공개한다(등록 실패의 raw 효과를 남기지 않음).
        this.bindings.set(token, binding);
        return token;
    }

    // 해제 뒤 노드를 다시 읽지 않는 순수 존재 조회이다. (2026-11-11 WB04-E308)
    owns(token) {
        return this.bindings.has(token);
    }

    // 같은 현재 생존 노드만 읽는다(해제되었거나 다른 owner 의 토큰은 오류). (2026-11-11 WB04-E308)
    node(token) {
        const binding = this.bindings.get(token);
        if (!binding) throw new RangeError('해제되거나 다른 owner의 연결 토큰');
        return binding.node;
    }

    // 실제 고정 노드 객체의 현재 토큰을 찾는다(sender 번호로 다른 노드를 대신하지 않음, ID 재배치와 구별). (2026-11-11 WB04-E308: 기준선 뒤 직접 송신)
    tokenFor(connection) {
        // 살아 있는 연결 중 같은 객체를 찾는다.
        for (const [token, binding] of this.bindings) {
            if (binding.node === connection) return token;
        }
        throw new RangeError('직접 송신의 현재 노드 token 누락');
    }

    // 004e1800 의 실제 소켓 준비 장치(0/1/-1)를 토큰으로 조회한다(Player/Packet flags 로 대체하지 않음). (2026-11-11 WB04-E308: 기준선 뒤 기존 연결 재사용)
    socketReadiness(token) {
        const binding = this.bindings.get(token);
        if (!binding || !binding.readiness) throw new Error('현재 연결의 실제 socket 준비 장치 미연결');
        return binding.readiness();
    }

    // 004e2dc0: 현재 슬롯을 Player flag 1 관문보다 먼저 읽고(pending 은 -32..-1, 양수는 0..21), 1..8 은 Player 필드 bit 0 이 꺼져 있으면 false, 슬롯이 비면 false, 아니면 같은 가상 송신 장치를 부르고 true 이다. 반환은 장치 호출 여부이지 전달 성공이 아니다. (2026-11-11 WB04-E308)
    sendMutable(id, packet, players) {
        if (id < -32 || id > 21) throw new RangeError('원문 송신의 현재 transceiver 번호 범위');
        const token = id < 0 ? entry(this.globals.pending, -id) : entry(this.globals.transceivers, id);
        return this.sendSelected(id, token, packet, players);
    }

    // 004e2dc0 의 줄 1e 진단이 복귀한 뒤 현재 실제 슬롯만 접근하는 Native 경로이다(새 pending/Player 조건을 넣지 않음). (2026-11-11 WB04-E308: 기준선 뒤 Native 경로)
    sendMutableNative(id, packet, players, assertion) {
        if (id < -32 || id > 21) assertNative(assertion, 0x1e);
        const token = id < 0 ? entry(this.globals.pending, -id) : entry(this.globals.transceivers, id);
        return this.sendSelected(id, token, packet, players);
    }

    // sendMutable 두 입구가 공유하는 뒷부분이다: Player 관문 → 빈 슬롯 → 가상 송신 호출(호출 뒤 같은 노드를 다시 조회하지 않음). (2026-11-11 WB04-E308)
    sendSelected(id, token, packet, players) {
        // 1..8 은 Player 레코드 180BYTE 칸의 +0x50 bit 0 이 켜져 있어야 보낸다.
        if (id >= 1 && id <= 8) {
            const offset = id * 0xb4 + 0x50;
            if (offset >= players.length) throw new RangeError('원문 송신의 현재 Player 필드 범위');
            if (!(players[offset] & 1)) return false;
        }
        if (!token) return false;
        const binding = this.bindings.get(token);
        if (!binding || !binding.send) throw new Error('현재 살아 있는 송신기의 실제 가상+4 장치 미연결');
        binding.send(packet);
        return true;
    }

    // 이미 선택된 연결의 가상+4 를 직접 호출한다(Player/번호 관문을 반복하지 않음). (2026-11-11 WB04-E308: 기준선 뒤 접속 통지 선행)
    sendDirect(token, packet) {
        const binding = this.bindings.get(token);
        if (!binding || !binding.send) throw new Error('직접 연결의 실제 가상 송신 장치 미연결');
        binding.send(packet);
    }

    // 004e2a50: 현재 슬롯 연결의 주소 20BYTE 를 읽거나, 연결이 없으면 소켓 없이 같은 0.0.0.0:0 주소(첫 BYTE 2)를 만든다. 범위는 -32..21. (2026-11-11 WB04-E308)
    address(id) {
        if (id < -32 || id > 21) throw new RangeError('Chat 현재 주소의 원본 전송 ID 범위');
        const token = id < 0 ? entry(this.globals.pending, -id) : entry(this.globals.transceivers, id);
        if (token) return Uint8Array.from(this.node(token).address);
        const address = new Uint8Array(20);
        address[0] = 2;
        return address;
    }

    // 004e2870: 현재 슬롯/sender 를 쓴다(노드 존재를 먼저 확인). 진단은 반환 없는 오류로 처리한다. (2026-11-11 WB04-E308)
    register(id, token) {
        this.node(token);
        registerTransceiver(id, token, this.cleanup(), this.registrationHooks());
    }

    // 004e2620: 새 노드 성공 뒤 직접 칸 store 이다. 기존 노드나 초기 패킷을 먼저 닫거나 지우지 않고 sender 도 쓰지 않는다. (2026-11-11 WB04-E308: 기준선 뒤 성공 후 공개)
    publishConnect(id, token) {
        this.node(token);
        if (!Number.isInteger(id) || id < 0 || id >= this.globals.transceivers.length) throw new RangeError('접속 공개의 실제22칸 범위');
        this.globals.transceivers[id] = token;
    }

    // 004e02d0/004e038c: pending 개수가 signed 로 31 미만일 때만 새 연결을 받는다(저장 DWORD 의 unsigned 감산과 구별). (2026-11-11 WB04-E308)
    canAccept() {
        return (this.globals.pendingCount.value | 0) < 31;
    }

    // 현재 첫 빈 pending 1..31 칸에 같은 노드를 등록하고 sender 를 -slot(unsigned)으로 쓰고 개수를 증가한다. 음수 id(-slot)를 돌려준다. (2026-11-11 WB04-E308)
    accept(token) {
        const connection = this.node(token);
        if (!this.canAccept()) throw new Error('현재 pending 개수31 관문');
        // 원문 순서대로 1 번 칸부터 첫 빈 칸을 찾는다.
        for (let slot = 1; slot < this.globals.pending.length; slot += 1) {
            if (this.globals.pending[slot]) continue;
            this.globals.pending[slot] = token;
            connection.sender = (0 - slot) >>> 0;
            this.globals.pendingCount.value = (this.globals.pendingCount.value + 1) >>> 0;
            return -slot;
        }
        throw new Error('pending 개수와 현재31칸의 불일치');
    }

    // 004e27b0: 전체 현재 재배치를 부른다. buffered(token) 은 재배치 중간에 같은 노드의 공통 buffered 입구를 대신하는 선택 장치이다. (2026-11-11 WB04-E308)
    assign(pending, target, server, buffered) {
        this.runDeviceScope(() => { assignPendingConnection(pending, target, server, this.cleanup(), this.registrationHooks(buffered)); });
    }

    // 원문 b0/b3/b6/bb 진단 뒤 현재 pending/target 토큰을 다시 읽는 실제 Native 재배치이다. buffered 를 생략하면 같은 토큰의 공통 buffered 장치(setBufferedNative)를 쓴다. (2026-11-11 WB04-E308: 기준선 뒤 Native 경로)
    assignNative(pending, target, server, assertion, buffered) {
        const selected = buffered ?? ((token) => { this.setBufferedNative(token); });
        this.runDeviceScope(() => { assignPendingConnection(pending, target, server, this.cleanup(), this.nativeRegistrationHooks(selected, assertion)); });
    }

    // 4df470(this,1): 같은 실제 노드에 묶인 공통 buffered 장치(flags 2·송신 커서 초기화)를 부른다. 장치가 없으면 오류이다. (2026-11-11 WB04-E308: 기준선 뒤 Native 경로)
    setBufferedNative(token) {
        const binding = this.bindings.get(token);
        if (!binding || !binding.buffered) throw new Error('현재node의실제공통buffered장치없음');
        binding.buffered();
    }

    // 현재 노드의 실제 소멸 장치(004e03d0)만 부른다. (2026-11-11 WB04-E308)
    destroy(token) {
        const binding = this.bindings.get(token);
        if (!binding) throw new RangeError('현재 연결 소멸 장치 누락');
        binding.destroy();
    }

    // 004e4354: 실제 free 장치를 부르고 성공한 뒤에만 별칭(정리 부모 안에서는 부모가 직접 store 하므로 생략)과 등록을 지운다. (2026-11-11 WB04-E308)
    release(token) {
        const binding = this.bindings.get(token);
        if (!binding) throw new RangeError('현재 연결/송신버퍼 free 장치 누락');
        binding.release();
        if (!this.cleanupDepth) this.removeAliases(token);
        this.bindings.delete(token);
    }

    // 같은 토큰의 현재 별칭만 지운다: 양수 칸(동기화 토큰이 같으면 함께 0)과 pending 1..31 칸(칸마다 개수 1 감소). 새 세대의 칸은 보존한다. (2026-11-11 WB04-E308)
    removeAliases(token) {
        let mainAlias = false;
        // 양수 22칸 중 같은 연결의 칸을 비운다.
        for (let index = 0; index < this.globals.transceivers.length; index += 1) {
            if (this.globals.transceivers[index] === token) {
                this.globals.transceivers[index] = 0;
                mainAlias = true;
            }
        }
        // pending 종료는 원문처럼 동기화 토큰을 쓰지 않는다.
        if (mainAlias && this.synchronization.value === token) this.synchronization.value = 0;
        // pending 1..31 칸 중 같은 연결의 칸을 비우고 현재 개수를 줄인다.
        for (let slot = 1; slot < this.globals.pending.length; slot += 1) {
            if (this.globals.pending[slot] === token) {
                this.globals.pending[slot] = 0;
                this.globals.pendingCount.value = (this.globals.pendingCount.value - 1) >>> 0;
            }
        }
    }

    // 호스트 owner 가 이미 없어지는 자기 노드의 별칭과 등록만 반납한다(Native 전체 정리의 대체가 아님). (2026-11-11 WB04-E308)
    forget(token) {
        if (!token) return;
        this.removeAliases(token);
        this.bindings.delete(token);
    }

    // 004e2940: 개별 종료를 같은 실제 표/장치에 실행한다. (2026-11-11 WB04-E308)
    disconnect(id) {
        this.runCleanup(() => { disconnectTransceiver(id, this.cleanup(), this.cleanupHooks()); });
    }

    // 004e2940 의 fa 진단이 복귀하면 기존 현재 슬롯/노드/버퍼 정리를 이어간다(close callback 이 남긴 앞 효과를 되돌리지 않음). (2026-11-11 WB04-E308: 기준선 뒤 Native 경로)
    disconnectNative(id, assertion) {
        this.runCleanup(() => { disconnectTransceiver(id, this.cleanup(), this.nativeCleanupHooks(assertion)); });
    }

    // 004e29c0: 양수 22칸 정리이며 버퍼/pending 전체 정리를 추가하지 않는다. (2026-11-11 WB04-E308)
    closeAll() {
        this.runCleanup(() => { closeAllTransceivers(this.cleanup(), this.cleanupHooks()); });
    }

    // 004e2a10: 원문 server 관문과 owner 1..8 종료이다. (2026-11-11 WB04-E308)
    endPlayers(server) {
        this.runCleanup(() => { endPlayerConnections(server, this.cleanup(), this.cleanupHooks()); });
    }

    // 004df4a0/004e2a00: 32 pending 정리이며 개수를 임의로 다시 계산하지 않는다. (2026-11-11 WB04-E308)
    closePending() {
        this.runCleanup(() => { closeAllPendingConnections(this.cleanup(), this.cleanupHooks()); });
    }

    // 응용 최종 종료 전용: Native 정리 부모의 표 쓰기를 마친 뒤 전역 칸에서 덮여 남은 노드를 등록 순서대로 명시 close/free 하고 초기 패킷 버퍼를 반납한다. 진행 중 부모 안이나 수신 중(깊이>0) 노드가 있으면 거부한다. (2026-11-11 WB04-E308: 기준선 뒤 App 종료 수명)
    shutdownTrackedDevices() {
        if (this.deviceCallDepth || this.cleanupDepth) throw new Error('진행중Native device부모안의App최종종료');
        // 깊이를 조작하거나 현재 패킷을 강제로 free 하지 않는다.
        for (const binding of this.bindings.values()) {
            if (!binding.node || binding.node.depth) throw new Error('진행중Node배송안의App최종종료');
        }
        this.deviceShutdown = true;
        this.runDeviceScope(() => {
            // 기존 0..21/0..31 Native 방문 순서와 pendingCount 미정규화를 그대로 사용한다.
            this.closeAll();
            this.closePending();
            // 전역 슬롯에서 덮인 잔류 노드를 현재 등록 순서(토큰 오름차순)대로 닫는다.
            while (this.bindings.size) {
                const token = this.bindings.keys().next().value;
                const connection = this.node(token);
                deferConnectionClose(connection, () => { this.destroy(token); }, () => { this.release(token); });
            }
            // Native 종료 부모의 후속 store 완료 뒤에만 같은 현재 초기 패킷 토큰을 free 하고 칸을 0 으로 쓴다.
            for (let index = 0; index < this.globals.buffers.length; index += 1) {
                const token = this.globals.buffers[index];
                if (token) {
                    this.release(token);
                    this.globals.buffers[index] = 0;
                }
            }
        });
    }

    // 최외곽 Native 부모의 마지막 표 쓰기까지의 호출 범위를 센다(정상/예외 모두 깊이를 되돌린다). 원문 shared_ptr 장치 보유는 GC 가 대신한다. (2026-11-11 WB04-E308)
    runDeviceScope(operation) {
        this.deviceCallDepth += 1;
        try {
            operation();
        } finally {
            this.deviceCallDepth -= 1;
        }
    }

    // 정리 부모가 자신의 칸·개수 store 를 마칠 때까지 일반 별칭 해제를 미룬다(정상/예외 모두 깊이를 되돌린다). (2026-11-11 WB04-E308)
    runCleanup(operation) {
        this.runDeviceScope(() => {
            this.cleanupDepth += 1;
            try {
                operation();
            } finally {
                this.cleanupDepth -= 1;
            }
        });
    }

    // 등록/재배치 자유 함수의 hooks 를 만든다: 같은 노드, 선택 buffered(현재 노드가 없으면 오류), 반환 없는 진단. buffered 를 주지 않으면 hooks 에도 넣지 않아 도달한 위치에서 누락으로 보고된다. (2026-11-11 WB04-E308)
    registrationHooks(buffered) {
        const hooks = {
            node: (token) => this.node(token),
            assertion: (line) => { throw new Error(`원문 연결 등록 진단:${line}`); },
        };
        if (buffered !== undefined && buffered !== null) {
            if (typeof buffered !== 'function') throw new TypeError('buffered 장치는 함수여야 합니다.');
            // 같은 원문 선택 지점에서 현재 노드가 살아 있는지 확인한 뒤 선택 장치를 부른다.
            hooks.buffered = (token) => {
                if (!this.bindings.has(token)) throw new Error('buffered단계의현재node없음');
                buffered(token);
            };
        }
        return hooks;
    }

    // Native 재배치용 hooks: 기존 node/쓰기 부모에 선택된 Client 진단(복귀형)만 교체한다. (2026-11-11 WB04-E308: 기준선 뒤 Native 경로)
    nativeRegistrationHooks(buffered, assertion) {
        const hooks = this.registrationHooks(buffered);
        hooks.assertion = (line) => { assertNative(assertion, line); };
        return hooks;
    }

    // 정리 자유 함수의 hooks 를 만든다: 같은 노드·실제 소멸/해제 장치와 반환 없는 진단이다. (2026-11-11 WB04-E308)
    cleanupHooks() {
        return {
            node: (token) => this.node(token),
            destroy: (token) => { this.destroy(token); },
            release: (token) => { this.release(token); },
            assertion: (line) => { throw new Error(`원문 연결 정리 진단:${line}`); },
        };
    }

    // Native 정리용 hooks: 같은 destroy/release 수명에 복귀형 fa 진단만 제공한다. (2026-11-11 WB04-E308: 기준선 뒤 Native 경로)
    nativeCleanupHooks(assertion) {
        const hooks = this.cleanupHooks();
        hooks.assertion = (line) => { assertNative(assertion, line); };
        return hooks;
    }
}
