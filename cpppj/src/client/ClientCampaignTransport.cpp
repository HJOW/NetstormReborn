// 지난 Login의 raw this를 장기 backend에 저장하지 않고 실제 현재 World와 장기 전역을 빌린다.
#include "client/ClientCampaignTransport.h"
#include "client/NativeConstructionPacketReceivers.h" //같은 건설계 고정F를 실제 Source의 장기 기본표에 설치한다.
#include "client/ClientChatPacketReceiver.h" //41h도 현재 Client의 같은 장기 중계/표시 처리를 사용한다.
#include "client/ClientMetaPacketReceivers.h" //Meta 전용 Source 없이 같은 Client 기본 8F를 설치한다.
#include "o/RawMissionSettingsPacket.h" //장기 GameAux의 같은7DWORD를32h 기본F가 직접 쓴다.
#include "client/CurrentAllocationBytes.h" //7ch응답의미작성stacktail은기존실제할당입력을유지한다.
#include "client/NativeMetaConnectedState.h" //현재CS의같은master검색을공유한다.
#include "client/ClientPacketSources.h" //actual device/network는 배송 지점에서만 선택한다.
#include "client/UberGump.h" //현재 실제 UI 분배기의 같은 State7 부모만 호출한다.
#include "o/PacketRegistrationTable.h" //Zone 복제는 실제 현재 observer 표만 읽는다.
#include "client/ClientNsInfoGlobals.h" //0fh의 현재 서버행/checksum 저장소는 Client 수명이다.
#include "client/ClientStateStorageCleanup.h" //실제 marker가 있을 때만 현재 World 삭제 하위를 선택한다.
#include "client/RootServerListDisk.h" //기존 사용자 Saves의 동일 서버목록 쓰기 부모다.
#include "client/ClientMetaBattleDescription.h" //71h는현재CS본문과같은가상표시하위만공유한다.
#include "o/RawStateMetaBattleDescription.h" //일반등록도같은장기F/observer를다시대여한다.
#include "client/NativeMetaGeometry.h" //26h의 현재 geometry/Camera 한계 순수본문을 공유한다.
#include "client/Screen.h" //26h의 화면 논리 폭/높이를 현재 장치에서 읽는다.
#include "o/RawRootLoginReply.h" //0fh의 기존 버전/목록/서버행/다음상태 전체본문이다.
#include "o/RawChallengeLoginReply.h" //1dh의 현재 State/Subscriber와 동일 버전 거부 순서를 공유한다.
#include "o/RawLocalServerDiscovery.h" //66h는 현재36 쓰기 뒤 현재 주소와 실제 Run을 선택한다.
#include "client/CampaignTransportServices.h" //실제 sender 주소의 기존 문자열 변환을 그대로 공유한다.
#include "o/RawStateBattleStart.h" //1a/1b의 기존 원문 순수 본문을 공유한다.
#include "o/RawConstructionPacket.h" // Session 전후에도 같은3fh 순수 길이 관찰기를 실제 기본 표에 보관한다.
#include "client/ClientMain.h"
#include "client/ClientPacketRecord.h" //새 World도 같은 장기 기록 FILE/현재 프레임을 빌린다.
#include "client/ClientConnectionGlobals.h"
#include "client/ClientGameplayGlobals.h"
#include "client/ClientPlayerRecords.h"
#include "client/ClientSubscriberGlobals.h" //1a/1b의 같은 현재 Subscriber DWORD를 완전한 장기 형식으로 조회한다.
#include "client/ClientLocalStateGlobals.h"
#include "client/ClientConnectionActivityGlobals.h"
#include "client/ClientChallengeGlobals.h" //7bh는 Session/Meta raw this 대신 같은 장기 CS owner만 빌린다.
#include "o/RawConnectionEnded.h"
#include "client/RawSurfaceWorld.h" //기본 관찰기가 실제 storage의 AssertDisplay를 Session 없이 빌린다.
#include "o/RawTechnologyTypesPacket.h" //69h는 같은Client의장기목록/현재Session표시와Client관찰기진단을사용한다.
#include "o/RawPathData.h" //장기NoWorld/lease반납뒤에도실제40a660송신observer를사용한다.
#include "o/RawSquidCompressedReceive.h" //공통Squid7a의실제40b2a0길이관찰기를공유한다.
#include "o/RawBombEffect.h" //5fh의 실제40aca0을 같은 Client 등록표에 장기 보관한다.
#include "o/RawBombTimerPacket.h" //7d의실제40b390은같은mutable버퍼의현재count로길이를정한다.
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
#include "client/ClientTimeSource.h" // Client 시각 공급원이다(2026-11-09 추가).
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
    case 0x7b: {
        if (ReadClientPacketDefault(packet,9,1)==0xffU) storage->AssertDisplay("addrLen != (char)-1","\\Ns\\O\\_p_packets.cpp",0xba8);
        const auto length=std::bit_cast<std::int8_t>(static_cast<std::uint8_t>(ReadClientPacketDefault(packet,9,1))); // 진단 정상 복귀 뒤 현재 BYTE를 MOVSX AX처럼 다시 읽는다.
        WriteClientPacketDefaultLength(packet,static_cast<std::uint16_t>(std::int32_t(length)+10));return;
    }
    default:throw std::invalid_argument("Client 기본 표의 미등록 mutable 관찰기 종류");
    }
}
//원문지정폭의현재BYTE만순서대로쓰고다른header/payloadtail을초기화하지않는다.
void PutClientNativeMeta(std::span<std::uint8_t> bytes,std::size_t at,std::uint32_t value,std::size_t width=4) {
    if (at>bytes.size() || width>bytes.size()-at) throw std::out_of_range("Client Native메타쓰기물리부족");
    for (std::size_t byte=0;byte<width;++byte) bytes[at+byte]=static_cast<std::uint8_t>(value>>(8*byte)); //선택된폭의낮은BYTE부터같은현재버퍼에쓴다.
}
// Player/서버 권한/시계의 실제 소유권만 묶고 현재 World는 약하게 참조하게 한다.
struct TransportGlobals {
    std::shared_ptr<ClientPlayerRecords> players; // backend가 읽는 같은 Player1620BYTE 할당의 소유자다.
    std::shared_ptr<ClientGameplayGlobals> game; // 실제 서버 권한 bool과 현재 Session 대여를 보유한다.
    std::shared_ptr<o::GameClock> clock; // 원점/정지 상태를 복제하지 않는 실제 Client 시계다.
};
}
// Login 없는 Editor/Meta와 Session 반납 뒤에도 같은 원문 길이 쓰기/진단을 장기 기본 표에서 실행한다.
//원문2번의 NULL param2 비교와 전역 자연 분기를 Session 요구보다 먼저 유지한다.
void InstallClientConnectionEndedReceiver(Client& client,o::RawPacketDispatch& dispatch) {
    dispatch.RegisterCanonicalDirect(2,0x004b6660U,[game=client.GameplayGlobals(),players=client.PlayerRecords(),registry=client.ConnectionGlobals(),local=client.LocalStateGlobals(),activity=client.ConnectionActivityGlobals()](auto sender,o::PacketConnection* connection,auto) { //저장 F는 같은 Client 장기 저장소만 보유하며 지난 World/Meta/Session을 캡처하지 않는다.
        o::ConnectionEndedHooks effects;
        effects.saveServerFailure=[game] { auto& session=game->RequireSession();const auto pin=game->PinCurrentRuntime(session);session.SaveServerFailure(); }; //실제9번 장애 저장이 선택된 하위에서만 현재 전체 owner를 보유한다.
        effects.disconnect=[registry](auto id) { registry->Disconnect(id); }; //원문 sender의 현재 가상 연결을 조회하며 Source를 sender/phase로 추정하지 않는다.
        effects.broadcastPlayer=[game](auto owner) { auto& session=game->RequireSession();const auto pin=game->PinCurrentRuntime(session);session.BroadcastConnectionPlayer(owner); }; //선행 세 flags store 뒤 선택된 현재 방송 하위만 보유한다.
        const auto token=connection ? registry->TokenFor(*connection):0U; //원문 param2는 비교만 하므로 실제 NULL은0으로 전달하고 가짜 노드/주소를 만들지 않는다.
        o::ReceiveConnectionEnded(std::bit_cast<std::int32_t>(sender),token,{game->Server(),local->ClockSynchronization().connection,local->State().phase[0],local->State().next[0],activity->Frames(),[players] { return players->Records(); }},effects);
    }); //같은4BYTE 통지의 observer/version/length 표는 이 등록 부모에서 바꾸지 않는다.
}
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
namespace { //고정 F의 Source만 선택하며 수신함수/type을 현재 표에서 재선택하지 않는다.
NativeBattleSource ResolveNativePacketSource(Client& client,o::PacketConnection* connection) {
    if (!connection) return client.Menu() ? client.Menu()->CurrentNativeBattleSource():NativeBattleSource{}; //원문 NULL 직접caller는 현재 존재하는 실제 장치만 인계하며 없으면 State8의 새 생성을 허용한다.
    const auto source=client.ConnectionGlobals()->PacketSources()->Resolve(*connection);if (!source) throw std::logic_error("배송 connection의 실제 Source device 경로 누락");return *source; //sender/phase/마지막 생성 Source로 대체하지 않는다.
}
}
//Global F는 Client 장기 데이터만 보유하고 Source는 원문의 실제 선택 지점에서만 빌린다.
void InstallClientStateBattleStartReceivers(Client& client,o::RawPacketDispatch& dispatch) {
    client.PublishNativePacketTable(dispatch.Registrations()); //현재Client의실제표만공개하며저장F는이포인터변경과독립이다.
    dispatch.ObservePacket(0x1a,[client=&client](auto packet,auto sending) { o::StateBattleStartHooks hooks;hooks.assertion=[client](auto expression,auto file,auto line) { client->SurfaceStorage()->AssertDisplay(expression,file,line); };o::ObserveBattleStartZone(packet,sending,hooks); });dispatch.Version(0x1a,1,0);dispatch.RequireLength(0x1a,0xffff);
    dispatch.RegisterCanonicalDirect(0x1a,0x004b4df0U,[client=&client](auto,o::PacketConnection* connection,auto packet) { //고정F는Client장치만보유하며과거표/Source/Session을저장하지않는다.
        auto& local=*client->LocalStateGlobals();auto& game=*client->GameplayGlobals();const auto record=client->PacketRecordOwner();NativeBattleSource source; //실제 Source는 h.duplicate가 선택될 때만 채운다.
        o::StateBattleStartHooks hooks;hooks.assertion=[client](auto expression,auto file,auto line) { client->SurfaceStorage()->AssertDisplay(expression,file,line); };
        hooks.duplicate=[client,connection,&source,&local](auto bytes) { source=ResolveNativePacketSource(*client,connection);const auto current=client->NativePacketTable();return local.ZonePackets().Duplicate(bytes,*current); }; //진단/next주소·7·port 이후 원문의 observer1/현재 WORD/실제 복제를 수행한다.
        hooks.openRecord=[record](auto name) { record->Open(name); };hooks.disconnect=[registry=client->ConnectionGlobals()](auto id) { registry->Disconnect(id); };
        o::ReceiveBattleStartZone({const_cast<std::uint8_t*>(packet.data()),packet.size()},{local.State(),local.ZonePending(),game.GameServerPort(),game.Subscribers()->MySubscriber(),record->Requested(),client->SurfaceStorage()->Checking()},hooks);
        if (!client->Menu()) throw std::logic_error("next7 인계의 실제 UI State 부모 미연결");client->Menu()->DispatchNativeBattleStart(std::move(source),false); //원래 next7만 companion weak Source에 연결하며 즉시 Run/새명령을 만들지 않는다.
    });
    dispatch.Observe(0x1b,[](std::uint32_t) {});dispatch.Version(0x1b,1,0);dispatch.RequireLength(0x1b,38); //원문40b380의 실제 비NULL 빈 observer이며 ACK/성공효과가 아니다.
    dispatch.RegisterCanonicalDirect(0x1b,0x004b8b20U,[client=&client](auto,o::PacketConnection* connection,auto packet) { //server/pending 자연 반환 앞에 Source/Runtime을 요구하지 않는다.
        auto& local=*client->LocalStateGlobals();auto& game=*client->GameplayGlobals();const auto record=client->PacketRecordOwner();o::StateBattleStartHooks hooks;
        hooks.assertion=[client](auto expression,auto file,auto line) { client->SurfaceStorage()->AssertDisplay(expression,file,line); };hooks.openRecord=[record](auto name) { record->Open(name); };
        hooks.run=[client,connection] { auto source=ResolveNativePacketSource(*client,connection);if (!client->Menu()) throw std::logic_error("current7의 실제 UI State 부모 미연결");client->Menu()->DispatchNativeBattleStart(std::move(source),true); }; //주소 NUL 포함 부분쓰기/port/current7 뒤에만 실제 Source를 선택하며 old Runtime은 보유하지 않는다.
        o::ReceiveBattleStartPeer(packet,{local.State(),local.ZonePending(),game.GameServerPort(),game.Subscribers()->MySubscriber(),record->Requested(),client->SurfaceStorage()->Checking()},game.Server(),hooks);
    });
}
//004b4160의 현재 Client 진단/Config/Tell만 사용하는 고정 버전 본문이다. 실제 Source나 Runtime을 요구하지 않는다.
bool CheckClientMetaVersion(Client& client,const std::function<void(std::string_view)>& tell,std::uint32_t major,std::uint32_t minor) {
    if (major!=10 || minor!=78) {
        client.WriteDiagnosticLog(0x1d,"VERSION DIFFERENCE: You are 10.78. Meta Server is "+std::to_string(major)+'.'+std::to_string(minor)+".\n");
        if (major!=10 && !client.Configuration().GetInt("suppressVersionMessage")) { tell("MetaVerDiff("+std::to_string(major)+'.'+std::to_string(minor)+')');return false; }
    }return true;
}
// 1dh/66h는 현재 Client의 실제 UI만 선택하며 Meta 생성 이력이나 Session을 요구하지 않는다.
void InstallClientStateReplyReceivers(Client& client,o::RawPacketDispatch& dispatch) {
    dispatch.Observe(0x1d,[](std::uint32_t) {});dispatch.Version(0x1d,1,0);dispatch.RequireLength(0x1d,13); //40b380은 비NULL 빈 observer이며 원문 고정 길이는13이다.
    dispatch.RegisterCanonicalDirect(0x1d,0x004b8ac0U,[client=&client](auto,auto*,auto packet) { //고정 F는 장기 Client만 빌리고 현재 원문 하위에서 UI를 다시 조회한다.
        o::ChallengeLoginReplyHooks reply;reply.log=[client](auto text) { client->WriteDiagnosticLog(0x1d,text); };
        reply.version=[client](auto major,auto minor) { return CheckClientMetaVersion(*client,[client](auto section) { if (!client->Menu()) throw std::logic_error("1dh 버전 안내의 현재 UI 없음");client->Menu()->ShowNativePacketTell(section); },major,minor); };
        reply.run=[client] { if (!client->Menu()) throw std::logic_error("1dh 현재 State Run의 실제 UI 없음");client->Menu()->RunNativePacketState(); }; //현재28/12를 쓴 뒤에만 동기 Run을 수행한다.
        o::ReceiveChallengeLoginReply(packet,client->LocalStateGlobals()->State(),client->GameplayGlobals()->Subscribers()->MySubscriber(),reply);
    });
    dispatch.Observe(0x66,[](std::uint32_t) {});dispatch.Version(0x66,1,0);dispatch.RequireLength(0x66,12); //검색 명령 두 DWORD를 가진 원문12BYTE다.
    dispatch.RegisterCanonicalDirect(0x66,0x004b8a10U,[client=&client](auto sender,auto*,auto packet) { //현재type/Meta/phase로 F나 Source를 다시 선택하지 않는다.
        o::LocalServerDiscoveryReceiveHooks receive;receive.wall=[client] { return client->WallSeconds(); };
        receive.peerAddress=[client](auto id) { return CampaignPeerAddressText(client->ConnectionGlobals()->Address(std::bit_cast<std::int32_t>(id))); };
        receive.processState=[client] { if (!client->Menu()) throw std::logic_error("66h 현재 State Run의 실제 UI 없음");client->Menu()->RunNativePacketState(); };
        auto& local=*client->LocalStateGlobals();o::ReceiveLocalServerDiscovery(local.State(),{local.LocalServerToken(),local.LocalServerSearchEnd(),local.LocalServerRetryEnd()},sender,packet,receive);
    }); //명령1의 현재36/주소/port 뒤만 Run하며 경쟁 토큰 경로는 UI를 조회하지 않는다.
}
//0fh는 버전 거부 앞에 Meta/Source/World를 요구하지 않으며 실제 marker 삭제만 해당 하위에서 pin한다.
void ReceiveClientRootReply(Client& client,std::span<const std::uint8_t> packet) {
    auto& local=*client.LocalStateGlobals();o::RootLoginReplyHooks hooks;
    hooks.version=[&client](auto major,auto minor) { return CheckClientMetaVersion(client,[&client](auto section) { if (!client.Menu()) throw std::logic_error("0fh 버전 안내의 현재 UI 없음");client.Menu()->ShowNativePacketTell(section); },major,minor); };
    hooks.clearZones=[&client] { ClearClientMetaZoneRecords(client,*client.SurfaceStorage()); };
    hooks.checksum=[&client](auto value) { client.NsInfoGlobals()->SetServerValue(std::bit_cast<std::int32_t>(client.LocalStateGlobals()->State().phase[0x44/4]),value); };
    hooks.server=[&client](auto row) { client.NsInfoGlobals()->UpsertServer(row,true); };hooks.saveServers=[&client] { SaveClientRootServerList(client); };
    o::ReceiveRootLoginReply(packet,{local.State(),local.MetaZones().records,local.MetaRefresh()},hooks); //기존10.71 compact 구별/부분쓰기/파일위치/phase23분기를 바꾸지 않는다.
}
//0fh/26h의 observer도 같은 Client 장기 진단만 보유하며 실제 Meta 객체를 캡처하지 않는다.
void InstallClientMetaGlobalReceivers(Client& client,o::RawPacketDispatch& dispatch) {
    const auto storage=client.SurfaceStorage(); //observer의 실제 장기 storage이며 이전 World/CS 할당을 고정하지 않는다.
    dispatch.ObservePacket(0x0f,[storage](auto packet,auto sending) { o::ObserveRootLoginReply(packet,sending,[storage](auto expression,auto line) { storage->AssertDisplay(expression,"\\Ns\\O\\_p_packets.cpp",line); }); });dispatch.Version(0x0f,1,0);dispatch.RequireLength(0x0f,0xffff);
    dispatch.RegisterCanonicalDirect(0x0f,0x004b4250U,[client=&client](auto,auto*,auto packet) { ReceiveClientRootReply(*client,packet); }); //sender/nullable연결/phase/known을 앞단 필터로 추가하지 않는다.
    dispatch.ObservePacket(0x26,[storage](auto packet,auto sending) { ObserveNativeMetaGeometry(packet,sending,[storage](auto expression,auto line) { storage->AssertDisplay(expression,"\\Ns\\O\\_p_packets.cpp",line); }); });dispatch.Version(0x26,1,0);dispatch.RequireLength(0x26,0xffff);
    dispatch.RegisterCanonicalDirect(0x26,0x004b45a0U,[client=&client](auto,auto*,auto packet) { //CS0/메타모드/송신자/known/World 선행 관문이 없는 원문 F다.
        auto& challenge=*client->ChallengeGlobals();ReceiveNativeMetaGeometry(packet,challenge.Geometry(),challenge.GeometryMode(),[client](float width,float height) { auto& screen=client->GetScreen();client->GameplayGlobals()->Camera().UpdateTargetLimits(screen.Width(),screen.Height(),width,height); });
    }); //Camera한계→mode0→현재contact길이→선택geometry부분쓰기→mode1을 기존 본문 그대로 유지한다.
}
//71h는현재CS0이면본문을읽지않고모드/known/송신자/World를앞에서요구하지않는다.
void InstallClientMetaDescriptionReceiver(Client& client,o::RawPacketDispatch& dispatch) {
    const auto challenge=client.ChallengeGlobals();const auto storage=client.SurfaceStorage(); //각각같은Client의장기CS슬롯과현재진단owner다.
    o::RegisterStateMetaBattleDescription(dispatch,challenge->Current(),[client=&client](auto,auto packet) { ReceiveNativeMetaBattleDescription(*client->ChallengeGlobals(),packet,MakeClientMetaBattleDescriptionHooks(*client)); },[storage](auto expression) { storage->AssertDisplay(expression,"\\Ns\\O\\_p_packets.cpp",0xae3); }); //40b0d0의실제observer/버전/길이와장기진단을공유한다.
    dispatch.RegisterCanonicalDirect(0x71,0x004b49c0U,[client=&client,challenge](auto,auto*,auto packet) { //저장F는Native본문정체성을유지하고현재표/type으로다시고르지않는다.
        if (!challenge->Current()) return;ReceiveNativeMetaBattleDescription(*challenge,packet,MakeClientMetaBattleDescriptionHooks(*client));
    });
}
//4b2c00의현재자기CS기록을먼저캡처하고flags쓰기뒤전역11의실제가상송신만선택한다.
void NotifyClientMetaMaster(Client& client) {
    const auto own=client.ChallengeGlobals()->Participant(client.GameplayGlobals()->player.battleOwner);std::array<std::uint8_t,12> packet;packet[0]=1;packet[1]=0x1f;packet[2]=12;packet[3]=0;
    PutClientNativeMeta(packet,4,client.CurrentSubscriber());const auto flags=ReadClientPacketDefault(own,0x64,4)|0x2000U;PutClientNativeMeta(own,0x64,flags);PutClientNativeMeta(packet,8,flags);
    const auto storage=client.SurfaceStorage();static_cast<void>(client.ConnectionGlobals()->SendMutableNative(11,packet,client.PlayerRecords()->Records(),[storage](auto expression,auto file,auto line) { storage->AssertDisplay(expression,file,line); })); //Source는실제11송신Binding선택뒤에만pin한다.
}
//7ch는원문status각분기에서선택한현재CS/Registry만쓰고Meta/Session을앞에서요구하지않는다.
void ReceiveClientMetaGameLogin(Client& client,std::uint32_t sender,std::span<const std::uint8_t> packet) {
    const auto status=ReadClientPacketDefault(packet,4,4);
    if (status==0) {
        auto& challenge=*client.ChallengeGlobals();bool accepted=false;const auto master=FindMetaBattleMaster(challenge,client.GameplayGlobals()->player.battleOwner);
        if (std::bit_cast<std::uint32_t>(master)==client.GameplayGlobals()->player.battleOwner) {
            const auto record=challenge.Participant(ReadClientPacketDefault(packet,12,4));const auto subscriber=ReadClientPacketDefault(record,4,4);
            if (subscriber==ReadClientPacketDefault(packet,16,4)) { const auto battle=ReadClientPacketDefault(record,0xa4,4);if (battle==ReadClientPacketDefault(packet,20,4)) { const auto seat=ReadClientPacketDefault(record,0xa8,4);if (seat==ReadClientPacketDefault(packet,24,4)) accepted=true; } }
        }
        std::array<std::uint8_t,28> reply;CurrentAllocationBytes(reply);reply[0]=1;reply[1]=0x7c;reply[2]=28;reply[3]=0;PutClientNativeMeta(reply,4,accepted ? 1U:2U);PutClientNativeMeta(reply,8,client.CurrentSubscriber());
        const auto storage=client.SurfaceStorage();const auto assertion=[storage](auto expression,auto file,auto line) { storage->AssertDisplay(expression,file,line); };const auto registry=client.ConnectionGlobals();const auto pending=std::bit_cast<std::int32_t>(sender);
        static_cast<void>(registry->SendMutableNative(pending,reply,client.PlayerRecords()->Records(),assertion));if (!accepted) return;
        const auto disconnectTarget=std::bit_cast<std::int32_t>(ReadClientPacketDefault(packet,24,4)+1U);registry->DisconnectNative(disconnectTarget,assertion);client.GameplayGlobals()->Server()=true;
        const auto assignTarget=std::bit_cast<std::int32_t>(ReadClientPacketDefault(packet,24,4)+1U);registry->AssignNative(pending,assignTarget,client.GameplayGlobals()->Server() ? 1U:0U,assertion);client.GameplayGlobals()->Server()=false;return; //정상뒤0이며예외때이전서버값으로RAII복원하지않는다.
    }
    if (status==1) { auto& challenge=*client.ChallengeGlobals();const auto master=FindMetaBattleMaster(challenge,client.GameplayGlobals()->player.battleOwner);if (master<0) return;const auto subscriber=ReadClientPacketDefault(challenge.Participant(std::bit_cast<std::uint32_t>(master)),4,4);if (subscriber==ReadClientPacketDefault(packet,8,4)) NotifyClientMetaMaster(client);return; }
    client.LocalStateGlobals()->CachedPeerIndex()=0xffffffffU; //그외는현재캐시한DWORD만쓰고CS/Source/Session을읽지않는다.
}
//실제앱전역metadata/고정F를장기Client기본표에설치하고직접Meta호출도동일본문을공유한다.
void InstallClientMetaGameLoginReceiver(Client& client,o::RawPacketDispatch& dispatch) {
    dispatch.Observe(0x7c,[](std::uint32_t) {});dispatch.Version(0x7c,1,0);dispatch.RequireLength(0x7c,28);
    dispatch.RegisterCanonicalDirect(0x7c,0x004b2cc0U,[client=&client](auto sender,auto*,auto packet) { ReceiveClientMetaGameLogin(*client,sender,packet); }); //NULLconnection/현재phase/known/CS0을선행검사하지않는다.
}
void InstallClientPacketDefaults(Client& client,o::RawPacketDispatch& dispatch) {
    InstallNativeConstructionPacketReceivers(dispatch,client.GameplayGlobals(),client.SurfaceStorage()); //수신기는 실제 현재 Runtime을 호출 시점에만 선택하며 이전 Session을 보관하지 않는다.
    InstallClientChatPacketReceiver(client,dispatch); //Meta/일반/Editor와 Session 반납 뒤 같은42dc90/40a8d0을 보존한다.
    InstallClientMetaPacketReceivers(client,dispatch); //20/21/22/23/24/25/67/88의 같은 전역 본문·관찰기를 모든 실제 Client 기본 표에 둔다.
    InstallClientMetaGameLoginReceiver(client,dispatch); //Meta/일반/Editor및Session반납뒤에도같은전역F와비NULL40b380을보존한다.
    InstallClientMetaDescriptionReceiver(client,dispatch); //Meta/일반/Editor의같은장기기본F이며Session반납뒤에도유지한다.
    InstallClientMetaGlobalReceivers(client,dispatch); //원문 앱 전역F이므로 Meta 생성 전후/일반/Editor에 같은 baseline이 남는다.
    InstallClientConnectionEndedReceiver(client,dispatch); //Editor도 같은 전역2번 F를 Session 등록 전에 보유한다.
    InstallClientStateReplyReceivers(client,dispatch); //원문 앱 전역1dh/66h를 Meta/일반/Editor의 같은 기본 표에 설치한다.
    InstallClientStateBattleStartReceivers(client,dispatch); //같은 Client 기본 F는 Meta/일반/Editor와 lease 반납 뒤에도 존재한다.
    InstallClientMissionSendObservers(client,dispatch); //같은 네 순수 관찰기를 재사용하며 실제 기능 수신기를 추가하지 않는다.
    const auto storage=client.SurfaceStorage(); //진단의 장기 Client storage만 보유하고 Session/World/Meta this는 빌리지 않는다.
    const auto game=client.GameplayGlobals(); //69h 기본 receiver는장기전역만보유하고이전Session/World를고정하지않는다.
    dispatch.Observe(0x32,[](std::uint32_t) {});dispatch.Version(0x32,1,0);dispatch.RequireLength(0x32,32); //40b380의 비NULL 빈observer와 version1/policy0/고정32BYTE다.
    dispatch.RegisterCanonicalDirect(0x32,0x0044dc20U,[aux=game->GameAux()](auto,auto*,auto packet) { o::ReceiveCurrentMissionSettingsPacket(aux->MissionSettings(),packet); }); //장기7DWORD만 보유하고 송신자/연결/Session/phase를 조회하지 않는다.
    o::RegisterTechnologyTypesPacket(dispatch,game->GameAux()->TypeUse(),[game](auto type) { auto& session=game->RequireSession();const auto runtimePin=game->PinCurrentRuntime(session);session.IndicateTechnology(type); },
        [storage](auto expression,auto file,auto line) { storage->AssertDisplay(expression,file,line); }); //합집합뒤비영항목마다현재Session/실제전체owner를선택하며반환/예외까지pin한다.빈목록은World를요구하지않는다.
    dispatch.ObservePacket(0x7b,[storage](auto packet,auto sending) { ObserveClientPacketDefault(0x7b,packet,sending,storage); }); //40b2f0은 수신 방향에서 payload를 읽지 않고 송신 진단 뒤 signed BYTE 길이를 재조회한다.
    dispatch.Version(0x7b,1,0);dispatch.RequireLength(0x7b,0xffff);
    dispatch.RegisterCanonicalDirect(0x7b,0x004b4790U,[challenge=client.ChallengeGlobals()](std::uint32_t,o::PacketConnection*,std::span<const std::uint8_t> packet) { challenge->ReceivePeerAddress(packet); }); // 고정F004b4790은 같은 장기 CS만 보유하며 CS0 조기반환 앞에 Session/Source/lease pin이나 sender/nullable 연결 조회를 추가하지 않는다.
    dispatch.ObservePacket(0x3f,[storage](auto packet,auto sending) { o::FinalizeConstructionPacket(packet,sending!=0,[storage] { storage->AssertDisplay("sidCount != (unsigned char)-1","\\Ns\\O\\_p_packets.cpp",0x7c9); }); }); //40a800의 같은 버퍼/진단 정상 복귀 뒤 현재count 재조회다.
    dispatch.Version(0x3f,1,0);dispatch.RequireLength(0x3f,0xffff); // receiver만 Session 대여에 맡기며 원문 기본 metadata/observer는 반납 뒤에도 남긴다.
    dispatch.ObservePacket(0x30,[storage](auto packet,auto sending) { o::ObservePathData(packet,sending,[storage](auto expression,auto file,auto line) { storage->AssertDisplay(expression,file,line); }); });dispatch.Version(0x30,1,0);dispatch.RequireLength(0x30,0xffff); //40a660은현재nodeCount재조회뒤같은WORD를쓰며receiver는추가하지않는다.
    dispatch.ObservePacket(0x7a,[storage](auto packet,auto sending) { o::ObserveCompressedSquid(packet,sending,[storage](auto expression,auto file,auto line) { storage->AssertDisplay(expression,file,line); }); });dispatch.Version(0x7a,1,0);dispatch.RequireLength(0x7a,0xffff); //40b2a0은dataLength진단복귀뒤현재BYTE를읽고같은버퍼의길이를정한다.
    dispatch.ObservePacket(0x5f,[storage](auto packet,auto sending) { o::ObserveBombEffectPacket(packet,sending,[storage] { storage->AssertDisplay("count != (int)-1","\\Ns\\O\\_p_packets.cpp",0x96b); }); });dispatch.Version(0x5f,1,0);dispatch.RequireLength(0x5f,0xffff); //실제Source송신때만현재WORD를쓰며Session반납뒤에도같은observer를사용한다.
    dispatch.ObservePacket(0x7d,[](auto packet,auto sending) { o::FinalizeBombTimerPacket(packet,sending!=0); });dispatch.Version(0x7d,1,0);dispatch.RequireLength(0x7d,0xffff); //40b390은송신때만현재count WORD로길이를정하며lease반납뒤에도남는다.
    for (const auto& entry:kClientPacketDefaults) { //이18종만 version1/policy0/보호 PE 길이에 맞추고 다른 등록에는 손대지 않는다.
        if (entry.mutableObserver) dispatch.ObservePacket(entry.type,[storage,type=entry.type](auto packet,auto sending) { ObserveClientPacketDefault(type,packet,sending,storage); });
        else dispatch.Observe(entry.type,[](std::uint32_t) {}); //40b380의 비NULL ret4이며 NULL observer로 바꾸지 않는다.
        dispatch.Version(entry.type,1,0);dispatch.RequireLength(entry.type,entry.length);
        if (entry.c3Receiver) dispatch.RegisterCanonicalDirect(entry.type,0x004437f0U,[](std::uint32_t,o::PacketConnection*,std::span<const std::uint8_t>) {}); //4437f0의 실제 ret이며 ACK/반환 성공값/게임 State를 합성하지 않는다.
    }
    dispatch.ObservePacket(0x0b,[storage](auto packet,auto sending) { ObserveClientPacketDefault(0x0b,packet,sending,storage); }); //409fe0은수신필드를읽지않고송신때만현재extraDataLen/선언WORD를처리한다.
    dispatch.Version(0x0b,1,0);dispatch.RequireLength(0x0b,0xffff); //Client40b4b0의실제버전조회등록이며R의0b인사수신과다르다.
    dispatch.RegisterCanonicalDirect(0x0b,0x004b3af0U,[aux=storage->GameAux(),registry=client.ConnectionGlobals(),players=client.PlayerRecords()](auto sender,auto*,auto) {
        const auto message=aux->Packet0cHeader()[0],protocol=aux->Packet0cHeader()[1];const auto initial=aux->Packet0cInitialLength(); //4b3af0이현재BYTE두개/WORD하나를df590인자로읽는경계다.
        std::array<std::uint8_t,50> packet;packet.fill(0xff); //df590의실제50BYTE예약전체를FF로채우며extraDataLenDWORD도-1그대로남긴다.
        packet[1]=message;packet[0]=static_cast<std::uint8_t>(protocol&0x7fU);WriteClientPacketDefaultLength(packet,initial<4 ? 4:initial); //생성자는선언길이를최소4로올리고실제wire길이는현재송신observer가나중에정한다.
        packet[4]=10;packet[5]=0;packet[6]=78;packet[7]=0; //원문4b3b20/26은WORD10/78만쓰며제품표시11.01과기존wire10.78을구별한다.
        static_cast<void>(registry->SendMutable(std::bit_cast<std::int32_t>(sender),packet,players->Records())); //현재sender/Player/NULL관문뒤선택된실제가상송신에서만0c관찰기를한번호출한다.
    }); //실제장기세owner만보유하며수신payload/connection/phase/known을읽거나별도Session을붙잡지않는다.
}
// 같은 Client/Session의 물리를 검증하고 소켓을 열지 않는 실제 coordinator를 만든다.
std::shared_ptr<CompositeMissionTransport> MakeClientCampaignTransport(Client& client,CompositeTransportRole role,const std::shared_ptr<GameWorld>& world,MissionNetwork network,std::shared_ptr<MetaConnectionRuntime> meta) {
    if (!world || client.Edition()!=o::OriginalEdition::Patch1078 || !client.PlayerRecords()->Initialized() ||
        world->Session().PlayerRecords().data()!=client.PlayerRecords()->Records().data())
        throw std::invalid_argument("현재 Client/World/Player 물리의 복합 전송 수명 계약 오류");
    auto globals=std::make_shared<TransportGlobals>(TransportGlobals{client.PlayerRecords(),client.GameplayGlobals(),client.GameClockOwner()}); // 명목 토큰 대신 실제 세 소유권을 함께 보유한다.
    o::PacketDispatchHooks hooks;
    hooks.delay=client.ConnectionGlobals()->PacketDelays(); //재진입한 TCP/UDP도 같은 실제 Client 분포/시계/CRT를 사용하며 내부 미션에는 큐를 설치하지 않는다.
    hooks.players=[players=globals->players] { return std::span<const std::uint8_t>(players->Records()); }; //State 정리/재초기화 뒤에도 원문 송신은 현재 Player base를 읽는다.
    hooks.now=[clock=globals->clock]() -> std::uint32_t {
#ifdef _WIN32
        return static_cast<std::uint32_t>(clock->WallSeconds(ClientMilliseconds())); // 2026-11-09: Client 시각 공급원을 거친다(기본은 timeGetTime 그대로). // 기존 Login과 같은 현재 OS 밀리초/원점 계산이며 프레임 시각 사본을 읽지 않는다.
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
    transport->BindMetaSource(meta);client.ConnectionGlobals()->PacketSources()->Publish(transport); //실제 shared coordinator가 완성된 뒤 weak node 경로를 공개한다.
    InstallClientPacketDefaults(client,transport->Dispatch()); //호출자의 BindSession보다 먼저 같은 기본 표를 채워 임대 반납 때 원문 C3/관찰기/metadata가 복구되게 한다.
    return transport;
}
}
