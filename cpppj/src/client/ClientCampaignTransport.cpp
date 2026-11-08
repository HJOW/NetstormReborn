// 지난 Login의 raw this를 장기 backend에 저장하지 않고 실제 현재 World와 장기 전역을 빌린다.
#include "client/ClientCampaignTransport.h"
#include "client/ClientMain.h"
#include "client/ClientPacketRecord.h" //새 World도 같은 장기 기록 FILE/현재 프레임을 빌린다.
#include "client/ClientConnectionGlobals.h"
#include "client/ClientGameplayGlobals.h"
#include "client/ClientPlayerRecords.h"
#include "client/ClientLocalStateGlobals.h"
#include "client/ClientConnectionActivityGlobals.h"
#include "o/RawConnectionEnded.h"
#include "client/RawSurfaceWorld.h" //기본 관찰기가 실제 storage의 AssertDisplay를 Session 없이 빌린다.
#include "o/RawMissionClientPackets.h" //2a/2b/67/81의 같은 순수 가변 길이 관찰기 본체를 재사용한다.
#include <array> //원문 네 관찰기만 기본 표에 설치하는 고정 목록이다.
#include <bit>
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
// Login 없는 Editor/Meta와 Session 반납 뒤에도 같은 원문 길이 쓰기/진단을 장기 기본 표에서 실행한다.
void InstallClientMissionSendObservers(Client& client,o::RawPacketDispatch& dispatch) {
    const auto storage=client.SurfaceStorage(); //실제 Client storage만 보유하고 Session/World/Meta this는 캡처하지 않는다.
    if (!storage) throw std::invalid_argument("기본 미션 송신 관찰기의 실제 Client storage 없음");
    constexpr std::array<std::uint32_t,4> types{0x2a,0x2b,0x67,0x81}; //원문40a540/40a590/40ae80/40b460의 네 mutable 관찰기다.
    for (const auto type:types) { //각 종류의 현재 송신 버퍼를 같은 순수 본체에 한 번 전달하며 receiver/version/길이 표는 바꾸지 않는다.
        dispatch.ObservePacket(type,[storage,type](auto packet,auto sending) {
            o::ObserveMissionClientPacket(type,packet,sending,[storage](auto expression,auto file,auto line) { storage->AssertDisplay(expression,file,line); }); //진단 정상 복귀 뒤 같은 현재 버퍼를 재조회한다.
        });
    }
}
// 같은 Client/Session의 물리를 검증하고 소켓을 열지 않는 실제 coordinator를 만든다.
std::shared_ptr<CompositeMissionTransport> MakeClientCampaignTransport(Client& client,CompositeTransportRole role,const std::shared_ptr<GameWorld>& world,MissionNetwork network) {
    if (!world || client.Edition()!=o::OriginalEdition::Patch1078 || !client.PlayerRecords()->Initialized() ||
        world->Session().PlayerRecords().data()!=client.PlayerRecords()->Records().data())
        throw std::invalid_argument("현재 Client/World/Player 물리의 복합 전송 수명 계약 오류");
    auto globals=std::make_shared<TransportGlobals>(TransportGlobals{client.PlayerRecords(),client.GameplayGlobals(),client.GameClockOwner()}); // 명목 토큰 대신 실제 세 소유권을 함께 보유한다.
    o::PacketDispatchHooks hooks;
    hooks.players=[players=globals->players] { return std::span<const std::uint8_t>(players->Records()); }; //State 정리/재초기화 뒤에도 원문 송신은 현재 Player base를 읽는다.
    hooks.now=[clock=globals->clock]() -> std::uint32_t {
#ifdef _WIN32
        return static_cast<std::uint32_t>(clock->WallSeconds(timeGetTime())); // 기존 Login과 같은 현재 OS 밀리초/원점 계산이며 프레임 시각 사본을 읽지 않는다.
#else
        throw std::logic_error("현재 Client 전송의 OS 벽시계 미연결");
#endif
    };
    const auto record=client.PacketRecordOwner();hooks.sent=[record](auto& connection,auto packet,auto length) { record->Write(true,connection,packet,length); }; //실제 전송 본체의 복사/쓰기 뒤에만 방향1을 기록한다.
    hooks.record=[runtime=std::weak_ptr<GameWorld>(world),record](o::PacketConnection& connection,std::span<const std::uint8_t> packet,std::uint16_t length) {
        record->Write(false,connection,packet,length); //원문 방향0 기록은 수신기/기존 읽기 관측보다 먼저 실행하며 현재 Login 존재 여부와 별개다.
        if (const auto current=runtime.lock()) current->Session().RecordConnectionPacket(connection,packet); // 기록하는 동안에도 실제 World 전체를 보유하며 이전 Login을 캐시하지 않는다.
    };
    hooks.terminate=[](o::PacketConnection& connection) { connection.flags|=0x40;std::clog<<"로컬 미션 연결 종료\n"; }; // 원문 분배기의 종료 표시만 쓰고 물리 반납은 실제 backend에 맡긴다.
    hooks.close=[](o::PacketConnection&) { std::clog<<"로컬 연결 큐/소켓 종료\n"; }; // 실제 노드 owner가 close를 수행하는 같은 위치의 진단이다.
    hooks.release=[](o::PacketConnection&) { std::clog<<"로컬 연결 반납\n"; }; // 실제 노드 owner가 free를 수행하는 같은 위치의 진단이다.
    auto transport=network.composite;
    if (transport) { if (transport->Registry()!=client.ConnectionGlobals() || !transport->SharesNetwork(network)) throw std::logic_error("일반 State7 재진입의 같은 실제 전송 owner 불일치");transport->RebindWorld(role,globals->players->Records(),globals->game->Server(),std::move(hooks),globals); }
    else transport=CompositeMissionTransport::Create(role,globals->players->Records(),globals->game->Server(),std::move(hooks),client.ConnectionGlobals(),globals,std::move(network)); //첫 메타 인계와 기존 독립 경로는 원래 생성 위치를 유지한다.
    InstallClientMissionSendObservers(client,transport->Dispatch()); //호출자의 BindSession보다 먼저 기본 표를 채워 임대 반납 때 이 네 관찰기가 복구되게 한다.
    transport->Dispatch().Register(2,[runtime=std::weak_ptr<GameWorld>(world),globals,registry=client.ConnectionGlobals(),local=client.LocalStateGlobals(),activity=client.ConnectionActivityGlobals()](auto sender,auto& connection,auto) {
        const auto current=runtime.lock();if (!current) throw std::logic_error("연결 종료 통지의 실제 현재 World 수명 종료");o::ConnectionEndedHooks effects;
        effects.saveServerFailure=[current] { current->Session().SaveServerFailure(); };effects.disconnect=[registry](auto id) { registry->Disconnect(id); };effects.broadcastPlayer=[current](auto owner) { current->Session().BroadcastConnectionPlayer(owner); };
        o::ReceiveConnectionEnded(std::bit_cast<std::int32_t>(sender),registry->TokenFor(connection),{globals->game->Server(),local->ClockSynchronization().connection,local->State().phase[0],local->State().next[0],activity->Frames(),[globals] { return globals->players->Records(); }},effects);
    }); //004e2270의2번 수신기만 실제 Session 대여 전에 등록한다. 버전/정책/observer를 임의로 바꾸지 않는다.
    return transport;
}
}
