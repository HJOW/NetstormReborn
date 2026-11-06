// 지난 Login의 raw this를 장기 backend에 저장하지 않고 실제 현재 World와 장기 전역을 빌린다.
#include "client/ClientCampaignTransport.h"
#include "client/ClientMain.h"
#include "client/ClientConnectionGlobals.h"
#include "client/ClientGameplayGlobals.h"
#include "client/ClientPlayerRecords.h"
#include "client/GameWorld.h"
#include "client/RawCampaignSession.h"
#include <iostream>
#include <stdexcept>
#include <utility>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX // Windows 매크로가 표준 C++ 선언에 간섭하지 않게 한다.
#endif
#include <windows.h>
#include <mmsystem.h>
#endif

namespace netstorm::client { // 현재 연결을 만드는 효과는 원문 Serve/Connect 위치에 남긴다.
namespace { // 장기 전송이 실제로 참조하는 물리만 보유하며 World를 강하게 되참조하지 않는다.
// Player/서버 권한/시계의 실제 소유권만 묶고 현재 World는 약하게 참조하게 한다.
struct TransportGlobals {
    std::shared_ptr<ClientPlayerRecords> players; // backend가 읽는 같은 Player1620BYTE 할당의 소유자다.
    std::shared_ptr<ClientGameplayGlobals> game; // 실제 서버 권한 bool과 현재 Session 대여를 보유한다.
    std::shared_ptr<o::GameClock> clock; // 원점/정지 상태를 복제하지 않는 실제 Client 시계다.
};
}
// 같은 Client/Session의 물리를 검증하고 소켓을 열지 않는 실제 coordinator를 만든다.
std::shared_ptr<CompositeMissionTransport> MakeClientCampaignTransport(Client& client,CompositeTransportRole role,const std::shared_ptr<GameWorld>& world) {
    if (!world || client.Edition()!=o::OriginalEdition::Patch1078 || !client.PlayerRecords()->Initialized() ||
        world->Session().PlayerRecords().data()!=client.PlayerRecords()->Records().data())
        throw std::invalid_argument("현재 Client/World/Player 물리의 복합 전송 수명 계약 오류");
    auto globals=std::make_shared<TransportGlobals>(TransportGlobals{client.PlayerRecords(),client.GameplayGlobals(),client.GameClockOwner()}); // 명목 토큰 대신 실제 세 소유권을 함께 보유한다.
    o::PacketDispatchHooks hooks;
    hooks.now=[clock=globals->clock]() -> std::uint32_t {
#ifdef _WIN32
        return static_cast<std::uint32_t>(clock->WallSeconds(timeGetTime())); // 기존 Login과 같은 현재 OS 밀리초/원점 계산이며 프레임 시각 사본을 읽지 않는다.
#else
        throw std::logic_error("현재 Client 전송의 OS 벽시계 미연결");
#endif
    };
    hooks.record=[runtime=std::weak_ptr<GameWorld>(world)](o::PacketConnection& connection,std::span<const std::uint8_t> packet,std::uint16_t) {
        if (const auto current=runtime.lock()) current->Session().RecordConnectionPacket(connection,packet); // 기록하는 동안에도 실제 World 전체를 보유하며 이전 Login을 캐시하지 않는다.
    };
    hooks.terminate=[](o::PacketConnection& connection) { connection.flags|=0x40;std::clog<<"로컬 미션 연결 종료\n"; }; // 원문 분배기의 종료 표시만 쓰고 물리 반납은 실제 backend에 맡긴다.
    hooks.close=[](o::PacketConnection&) { std::clog<<"로컬 연결 큐/소켓 종료\n"; }; // 실제 노드 owner가 close를 수행하는 같은 위치의 진단이다.
    hooks.release=[](o::PacketConnection&) { std::clog<<"로컬 연결 반납\n"; }; // 실제 노드 owner가 free를 수행하는 같은 위치의 진단이다.
    return CompositeMissionTransport::Create(role,globals->players->Records(),globals->game->Server(),std::move(hooks),client.ConnectionGlobals(),globals);
}
}
