//Native 42dc90의 현재 주소→전투 접두→서버 중계→선택된 로컬 UI를 같은 장치로 연결한다.
#include "client/ClientChatPacketReceiver.h"
#include "client/ClientMain.h"
#include "client/ClientGameplayGlobals.h"
#include "client/ClientConnectionGlobals.h"
#include "client/ClientPlayerRecords.h"
#include "client/ClientChatWindowGlobals.h"
#include "client/RawSurfaceWorld.h"
#include "client/RawChatRelay.h"
#include "client/UberGump.h"
#include <bit>
#include <stdexcept>

namespace netstorm::client { //현재 실제 Client는 등록표/Source보다 오래 살아야 한다.
namespace { //원문이 선택한 폭/기록만 읽는다.
//Player의현재subscriber DWORD만낮은BYTE부터조립한다.
std::uint32_t ReadClientChatSubscriber(std::span<const std::uint8_t> bytes,std::size_t at) { if (at>bytes.size() || 4U>bytes.size()-at) throw std::out_of_range("Chat 현재 Player subscriber 물리 범위");std::uint32_t result{};for (std::size_t byte=0;byte<4;++byte) result|=std::uint32_t(bytes[at+byte])<<(byte*8);return result; } //인접 상태/새 active 조건은 읽지 않는다.
}
//장치 구성은 순수 바인딩이며 원문의 각 실제 선택 위치에서만 데이터/UI를 조회한다.
void InstallClientChatPacketReceiver(Client& client,o::RawPacketDispatch& dispatch) {
    ChatRelayHooks hooks;
    hooks.read=[client=&client](auto address)->std::uint32_t { switch (address) { case 0x594fbc:return client->SurfaceStorage()->ownerMode.battle;case 0x540bc0:return client->GameplayGlobals()->Server();case 0x540c74:return client->CurrentSubscriber();default:throw std::out_of_range("Client41h의 현재 전역 주소 범위"); } };
    hooks.address=[client=&client](auto sender) { return client->ConnectionGlobals()->Address(std::bit_cast<std::int32_t>(sender)); };
    hooks.player=[client=&client](auto subscriber) { if (!subscriber) client->SurfaceStorage()->AssertDisplay("subId != INVALID_SUBSCRIBER_ID","\\Ns\\O\\Player.cpp",0xa5);const auto players=client->PlayerRecords()->Records();for (std::uint32_t owner=1;owner<9;++owner) if (ReadClientChatSubscriber(players,owner*180U+44U)==subscriber) return owner;return 0U; }; //같은 첫subscriber 일치만 찾고 active/known 관문을 더하지 않는다.
    hooks.channel=[client=&client](auto owner) { if (client->SurfaceStorage()->ownerMode.challenge) client->SurfaceStorage()->AssertDisplay("!inMetamapMode","\\Ns\\O\\Player.cpp",0x134);if (owner>=9) throw std::out_of_range("Client41h의 현재 Player 번호 물리 범위");return static_cast<std::int32_t>(owner&255U); };
    hooks.connected=[client=&client](auto id) { const auto registry=client->ConnectionGlobals();const auto globals=registry->Cleanup();if (id<0 || static_cast<std::size_t>(id)>=globals.transceivers.size()) throw std::out_of_range("Client41h의 현재 연결 번호 범위");const auto token=globals.transceivers[static_cast<std::size_t>(id)];return token && !(registry->Node(token).flags&0x40U); };
    hooks.send=[client=&client](auto id,auto packet) { const auto storage=client->SurfaceStorage();static_cast<void>(client->ConnectionGlobals()->SendMutableNative(id,packet,client->PlayerRecords()->Records(),[storage](auto expression,auto file,auto line) { storage->AssertDisplay(expression,file,line); })); }; //실제 번호/Player/NULL 관문 뒤 선택된 node owner만 pin한다.
    hooks.receive=[client=&client](auto packet,auto) {
        if (!client->ChatWindowGlobals()->window.current) return; //42f0e0은548740이0이면 packet/UI를 조회하지 않고 자연 반환한다.548714의 Chatmanager와 구별한다.
        if (!client->Menu()) throw std::logic_error("Client41h의 현재 실제 Chat UI 없음");client->Menu()->ReceiveNativePacketChat(packet);
    }; //주소 준비/중계는 Relay가 먼저 수행한다.42ed30은 전달된 추가 주소를 소비하지 않으며 실제 수신/필터/로그는 기존 부모를 공유한다.
    hooks.assertion=[client=&client](auto line) { const auto expression=line==0x81dU ? "nameLen != (short)-1":line==0x81eU ? "bufferLen != (short)-1":"addrLen != (unsigned char)-1";client->SurfaceStorage()->AssertDisplay(expression,"\\Ns\\O\\_p_packets.cpp",line); };
    RegisterCanonicalChatPacket(dispatch,client.ChatWindowGlobals()->receivedMask,std::move(hooks)); //실제 장기 마스크 주소와 Client 장치만 고정 F에 보관한다.
}
}
