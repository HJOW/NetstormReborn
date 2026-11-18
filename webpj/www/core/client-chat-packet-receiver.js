// Client 장기 Chat 상태와 현재 전역을 41h 기본 관찰기·수신기에 연결한다.
// 사용: installClientChatPacketReceiver(client, dispatch). Session 대여보다 먼저 설치한다.
import { observeChatPacket } from '../ui/raw-chat-packet.js';
import { relayChatPacket } from '../ui/raw-chat-relay.js';

// 현재 서버 전역이 상자 또는 직접 값으로 제공되어도 원문의 현재 값을 읽는다.
function current(value) { return value && typeof value === 'object' ? value.value : value; }

// 42dc90/40a8d0의 장기 기본 표를 설치하고 UI는 실제 수신 시점에만 조회한다.
export function installClientChatPacketReceiver(client, dispatch) {
    const chat = client.chatWindowGlobals(), storage = client.surfaceStorage(), game = client.gameplayGlobals(), registry = client.connectionGlobals();
    const mask = { get value() { return chat.receivedMask; }, set value(value) { chat.receivedMask = value >>> 0; } };
    const hooks = {
        address: (sender) => registry.address(sender | 0),
        read: (address) => {
            if (address === 0x594fbc) return Number(storage.ownerMode.battle);
            if (address === 0x540bc0) return Number(current(game.server()));
            if (address === 0x540c74) return client.currentSubscriber();
            throw new RangeError('Client41h의 현재 전역 주소 범위');
        },
        receive: (packet) => {
            if (!chat.window.current) return;
            const menu = client.menu();
            if (!menu || typeof menu.receiveNativePacketChat !== 'function') throw new Error('Client41h의 현재 실제 Chat UI 없음');
            menu.receiveNativePacketChat(packet);
        },
        assertion: (line) => {
            const expression = line === 0x81d ? 'nameLen != (short)-1' : line === 0x81e ? 'bufferLen != (short)-1' : 'addrLen != (unsigned char)-1';
            storage.assertDisplay(expression, '\\Ns\\O\\_p_packets.cpp', line);
        },
    };
    dispatch.version(0x41, 1, 0); dispatch.requireLength(0x41, 0xffff);
    dispatch.observePacket(0x41, (packet, sending) => observeChatPacket(packet, sending, hooks.assertion));
    dispatch.registerDirect(0x41, (sender, _connection, packet) => relayChatPacket(sender, packet, mask, hooks));
}
