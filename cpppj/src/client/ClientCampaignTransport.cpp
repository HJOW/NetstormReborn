// 지난 Login의 raw this를 장기 backend에 저장하지 않고 실제 현재 World와 장기 전역을 빌린다.
#include "client/ClientCampaignTransport.h"
#include "o/RawConstructionPacket.h" // Session 전후에도 같은3fh 순수 길이 관찰기를 실제 기본 표에 보관한다.
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
#include <array> //같은 네 송신 관찰기와 Client 기본18행의 고정 목록이다.
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
// Client 원문 기본 등록 한 행이며 receiver NULL과 실제 C3를 별도로 구별한다.
struct ClientPacketDefaultRegistration {
    std::uint32_t type; //0040b4b0의 원문 패킷 종류다.
    std::uint16_t length; //0052ec40의 고정 길이WORD이며ffff는 가변이다.
    bool c3Receiver; //4437f0의 비NULL 빈 수신기만 설치하고 관찰 전용 receiver에는 손대지 않는다.
    bool mutableObserver; //실제 this 버퍼를 읽는 일곱 원문 관찰기와 비NULL 빈40b380을 구별한다.
};
constexpr std::array<ClientPacketDefaultRegistration,18> kClientPacketDefaults{{ //원문 C3 7종과 폐기80h를 뺀 관찰 전용11종의 정확한 version1/policy0 목록이다.
    {0x0c,0xffff,true,true},{0x42,12,true,false},{0x4d,10,true,false},
    {0x55,0xffff,true,true},{0x56,0xffff,true,true},{0x57,0xffff,true,true},{0x5a,5,true,false},
    {0x10,8,false,false},{0x11,12,false,false},{0x12,12,false,false},{0x16,0xffff,false,true},
    {0x17,14,false,false},{0x1c,0xffff,false,true},{0x1e,14,false,false},{0x1f,12,false,false},
    {0x68,5,false,false},{0x85,0xffff,false,true},{0x87,274,false,false}
}};
// 원문 비정렬 필드의 지정된 현재BYTE/WORD/DWORD만 little endian으로 읽는다.
std::uint32_t ReadClientPacketDefault(std::span<const std::uint8_t> packet,std::size_t offset,std::size_t width) {
    if (offset>packet.size() || width>packet.size()-offset) throw std::out_of_range("Client 기본 관찰기의 실제 필드 물리 부족");
    std::uint32_t value{}; //현재 지정 폭 바깥의 미작성 tail은 읽지 않는다.
    for (std::size_t byte=0;byte<width;++byte) value|=std::uint32_t(packet[offset+byte])<<(8*byte); //원문 낮은 BYTE부터 같은 현재 버퍼를 조립한다.
    return value;
}
// 원문의 선언WORD만 갱신하며16비트 ADD/저장의 감김과 그 밖의 원본 버퍼를 보존한다.
void WriteClientPacketDefaultLength(std::span<std::uint8_t> packet,std::uint32_t length) {
    if (packet.size()<4) throw std::out_of_range("Client 기본 관찰기의 선언WORD 물리 부족");
    packet[2]=static_cast<std::uint8_t>(length);packet[3]=static_cast<std::uint8_t>(length>>8);
}
// 원문409fe0/40a020/40aae0/40ab20/40ab60/40a190/40a270/40aed0의 송신 효과만 실행한다.
void ObserveClientPacketDefault(std::uint32_t type,std::span<std::uint8_t> packet,std::uint32_t sending,const std::shared_ptr<RawSurfaceStorage>& storage) {
    if (!sending) return; //원문 수신 방향은 실제 packet 필드를 읽기 전에 복귀한다.
    switch (type) {
    case 0x0b:
    case 0x0c:
        if (ReadClientPacketDefault(packet,8,4)==0xffffffffU) storage->AssertDisplay("extraDataLen != (int)-1","\\Ns\\O\\_p_packets.cpp",type==0x0b ? 0x36c:0x38c);
        WriteClientPacketDefaultLength(packet,ReadClientPacketDefault(packet,8,2)+12U);return;
    case 0x55:
        if (ReadClientPacketDefault(packet,4,4)==0xffffffffU) storage->AssertDisplay("count != (int)-1","\\Ns\\O\\_p_packets.cpp",0x8dc);
        WriteClientPacketDefaultLength(packet,ReadClientPacketDefault(packet,4,2)+8U);return;
    case 0x56:
        if (ReadClientPacketDefault(packet,8,4)==0xffffffffU) storage->AssertDisplay("dataLength != (int)-1","\\Ns\\O\\_p_packets.cpp",0x8f8);
        WriteClientPacketDefaultLength(packet,ReadClientPacketDefault(packet,8,2)+12U);return;
    case 0x57:
        if (ReadClientPacketDefault(packet,8,4)==0xffffffffU) storage->AssertDisplay("specLength != (int)-1","\\Ns\\O\\_p_packets.cpp",0x916);
        WriteClientPacketDefaultLength(packet,ReadClientPacketDefault(packet,8,2)+12U);return;
    case 0x16: {
        if (ReadClientPacketDefault(packet,5,1)==0xffU) storage->AssertDisplay("numFriends != (char)-1","\\Ns\\O\\_p_packets.cpp",0x458);
        const auto count=std::bit_cast<std::int8_t>(static_cast<std::uint8_t>(ReadClientPacketDefault(packet,5,1))); //진단 복귀 뒤 현재 signed BYTE를 다시 읽는 MOVSX AX다.
        WriteClientPacketDefaultLength(packet,static_cast<std::uint16_t>(std::int32_t(count)*4+14));return;
    }
    case 0x1c:
        if (ReadClientPacketDefault(packet,45,1)==0xffU) storage->AssertDisplay("nameLen != (unsigned char)-1","\\Ns\\O\\_p_packets.cpp",0x4ca);
        WriteClientPacketDefaultLength(packet,ReadClientPacketDefault(packet,45,1)+46U);return;
    case 0x85:
        if (ReadClientPacketDefault(packet,5,2)==0xffffU) storage->AssertDisplay("dataLen != (unsigned short)-1","\\Ns\\O\\_p_packets.cpp",0xa2c);
        WriteClientPacketDefaultLength(packet,ReadClientPacketDefault(packet,5,2)+7U);return;
    default:throw std::invalid_argument("Client 기본 표의 미등록 mutable 관찰기 종류");
    }
}
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
// 원문 C3 수신/관찰 전용 metadata와 a2 관찰기를 실제 같은 표의 비임대 기본 등록으로 묶는다.
void InstallClientPacketDefaults(Client& client,o::RawPacketDispatch& dispatch) {
    InstallClientMissionSendObservers(client,dispatch); //같은 네 순수 관찰기를 재사용하며 실제 기능 수신기를 추가하지 않는다.
    const auto storage=client.SurfaceStorage(); //진단의 장기 Client storage만 보유하고 Session/World/Meta this는 빌리지 않는다.
    dispatch.ObservePacket(0x3f,[storage](auto packet,auto sending) { o::FinalizeConstructionPacket(packet,sending!=0,[storage] { storage->AssertDisplay("sidCount != (unsigned char)-1","\\Ns\\O\\_p_packets.cpp",0x7c9); }); }); //40a800의 같은 버퍼/진단 정상 복귀 뒤 현재count 재조회다.
    dispatch.Version(0x3f,1,0);dispatch.RequireLength(0x3f,0xffff); // receiver만 Session 대여에 맡기며 원문 기본 metadata/observer는 반납 뒤에도 남긴다.
    for (const auto& entry:kClientPacketDefaults) { //이18종만 version1/policy0/보호 PE 길이에 맞추고 다른 등록에는 손대지 않는다.
        if (entry.mutableObserver) dispatch.ObservePacket(entry.type,[storage,type=entry.type](auto packet,auto sending) { ObserveClientPacketDefault(type,packet,sending,storage); });
        else dispatch.Observe(entry.type,[](std::uint32_t) {}); //40b380의 비NULL ret4이며 NULL observer로 바꾸지 않는다.
        dispatch.Version(entry.type,1,0);dispatch.RequireLength(entry.type,entry.length);
        if (entry.c3Receiver) dispatch.Register(entry.type,[](std::uint32_t,o::PacketConnection&,std::span<const std::uint8_t>) {}); //4437f0의 실제 ret이며 ACK/반환 성공값/게임 State를 합성하지 않는다.
    }
    dispatch.ObservePacket(0x0b,[storage](auto packet,auto sending) { ObserveClientPacketDefault(0x0b,packet,sending,storage); }); //409fe0은수신필드를읽지않고송신때만현재extraDataLen/선언WORD를처리한다.
    dispatch.Version(0x0b,1,0);dispatch.RequireLength(0x0b,0xffff); //Client40b4b0의실제버전조회등록이며R의0b인사수신과다르다.
    dispatch.Register(0x0b,[aux=storage->GameAux(),registry=client.ConnectionGlobals(),players=client.PlayerRecords()](auto sender,auto&,auto) {
        const auto message=aux->Packet0cHeader()[0],protocol=aux->Packet0cHeader()[1];const auto initial=aux->Packet0cInitialLength(); //4b3af0이현재BYTE두개/WORD하나를df590인자로읽는경계다.
        std::array<std::uint8_t,50> packet;packet.fill(0xff); //df590의실제50BYTE예약전체를FF로채우며extraDataLenDWORD도-1그대로남긴다.
        packet[1]=message;packet[0]=static_cast<std::uint8_t>(protocol&0x7fU);WriteClientPacketDefaultLength(packet,initial<4 ? 4:initial); //생성자는선언길이를최소4로올리고실제wire길이는현재송신observer가나중에정한다.
        packet[4]=10;packet[5]=0;packet[6]=78;packet[7]=0; //원문4b3b20/26은WORD10/78만쓰며제품표시11.01과기존wire10.78을구별한다.
        static_cast<void>(registry->SendMutable(std::bit_cast<std::int32_t>(sender),packet,players->Records())); //현재sender/Player/NULL관문뒤선택된실제가상송신에서만0c관찰기를한번호출한다.
    }); //실제장기세owner만보유하며수신payload/connection/phase/known을읽거나별도Session을붙잡지않는다.
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
    InstallClientPacketDefaults(client,transport->Dispatch()); //호출자의 BindSession보다 먼저 같은 기본 표를 채워 임대 반납 때 원문 C3/관찰기/metadata가 복구되게 한다.
    transport->Dispatch().Register(2,[runtime=std::weak_ptr<GameWorld>(world),globals,registry=client.ConnectionGlobals(),local=client.LocalStateGlobals(),activity=client.ConnectionActivityGlobals()](auto sender,auto& connection,auto) {
        const auto current=runtime.lock();if (!current) throw std::logic_error("연결 종료 통지의 실제 현재 World 수명 종료");o::ConnectionEndedHooks effects;
        effects.saveServerFailure=[current] { current->Session().SaveServerFailure(); };effects.disconnect=[registry](auto id) { registry->Disconnect(id); };effects.broadcastPlayer=[current](auto owner) { current->Session().BroadcastConnectionPlayer(owner); };
        o::ReceiveConnectionEnded(std::bit_cast<std::int32_t>(sender),registry->TokenFor(connection),{globals->game->Server(),local->ClockSynchronization().connection,local->State().phase[0],local->State().next[0],activity->Frames(),[globals] { return globals->players->Records(); }},effects);
    }); //004e2270의2번 수신기만 실제 Session 대여 전에 등록한다. 버전/정책/observer를 임의로 바꾸지 않는다.
    return transport;
}
}
