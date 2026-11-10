// 서버/응답 본문은 기존 모듈 하나만 사용하고 전역 F는 현재 Client만 보유한다.
#include "client/NativeLoginPacketReceivers.h"
#include "client/ClientMain.h"
#include "client/ClientGameplayGlobals.h"
#include "client/ClientPlayerRecords.h"
#include "client/ClientConnectionActivityGlobals.h"
#include "client/RawCampaignSession.h"
#include "client/RawSurfaceWorld.h"
#include "client/RawProcessHostStorage.h"
#include "client/RawBattleLeave.h"
#include "client/UberGump.h"
#include "o/RawMissionClientPackets.h"
#include "o/RawMissionServerPackets.h"
#include <array>
#include <bit>
#include <stdexcept>
#include <utility> //실제 활동 hooks만 지역 처리기로 이동한다.
namespace netstorm::client { // 같은 장기 owner와 불변 코드값으로 실제 현재 몸체를 선택한다.
namespace { // 원문 등록15행과 순수27h observer만 한정한다.
enum class ObserverKind { Empty,Login,Client,Metadata }; //40b380·40a450·Client 가변4종·40b110의 실제 슬롯이다.
struct ReceiverSpec { // 보호10.78의 실제 수신 등록 한 행이다.
    std::uint32_t type,entry; // 메시지 번호와 불변 Native 코드 주소다.
    std::uint16_t length; // 보호 PE 길이WORD이며ffff는 가변이다.
    ObserverKind observer; // 비NULL 빈 observer와 실제 mutable observer를 구별한다.
};
constexpr std::array<ReceiverSpec,15> kReceivers{{ //67h는 기존 앱 전역004b4900을 설치하지 않고 그대로 대여한다.
    {0x27,0x004a7800U,0xffff,ObserverKind::Login},{0x63,0x004a7510U,8,ObserverKind::Empty},{0x29,0x004b3b40U,34,ObserverKind::Empty},
    {0x67,0x004b4900U,0xffff,ObserverKind::Client},{0x2b,0x004912d0U,0xffff,ObserverKind::Client},{0x2a,0x004b3d30U,0xffff,ObserverKind::Client},
    {0x28,0x004b8940U,18,ObserverKind::Empty},{0x81,0x004b3e90U,0xffff,ObserverKind::Client},{0x84,0x004a6f30U,8,ObserverKind::Empty},
    {0x6e,0x004a7050U,8,ObserverKind::Empty},{0x72,0x004b3bc0U,0xffff,ObserverKind::Metadata},{0x3d,0x00491220U,9,ObserverKind::Empty},
    {0x78,0x0048f880U,6,ObserverKind::Empty},{0x52,0x004a71a0U,5,ObserverKind::Empty},{0x64,0x004b6180U,5,ObserverKind::Empty}
}};
// 실제 현재 버퍼의 지정 BYTE/WORD/DWORD만 little endian으로 읽는다.
std::uint32_t ReadLoginField(std::span<const std::uint8_t> packet,std::size_t at,std::size_t width) {
    if (at>packet.size() || width>packet.size()-at) throw std::out_of_range("Login observer의 실제 필드 물리 부족");
    std::uint32_t value{}; //요청한 폭 밖의 미작성 바이트는 읽지 않는다.
    for (std::size_t byte=0;byte<width;++byte) value|=std::uint32_t(packet[at+byte])<<(byte*8); //원문 비정렬 낮은 바이트부터 같은 현재 값을 조립한다.
    return value;
}
//40a450은 수신 때 필드를 읽지 않고 두 진단 뒤 현재 BYTE/WORD로 선언WORD만 쓴다.
void ObserveLogin(std::span<std::uint8_t> packet,std::uint32_t sending,const std::shared_ptr<RawSurfaceStorage>& storage) {
    if (!sending) return;
    if (ReadLoginField(packet,12,1)==0xffU) storage->AssertDisplay("len != (unsigned char)-1","\\Ns\\O\\_p_packets.cpp",0x611);
    if (ReadLoginField(packet,13,4)==0xffffffffU) storage->AssertDisplay("fortFileSize != (int)-1","\\Ns\\O\\_p_packets.cpp",0x612);
    const auto length=ReadLoginField(packet,12,1)+ReadLoginField(packet,13,2)+17U; //진단 정상 복귀 뒤 같은 현재 필드를 다시 읽는 원문 순서다.
    packet[2]=static_cast<std::uint8_t>(length);packet[3]=static_cast<std::uint8_t>(length>>8);
}
}
// 전역 설치 자체는 현재 Session/Login/Player/모드를 조회하지 않는다.
void InstallNativeLoginPacketReceivers(Client& client,o::RawPacketDispatch& dispatch) {
    const auto storage=client.SurfaceStorage();const auto game=client.GameplayGlobals(); //같은 장기 물리/현재 게임 전역만 보유한다.
    if (!storage || !game || !storage->UsesNativeTransmissionDevices()) throw std::invalid_argument("Native Login의 실제 Client owner 미연결");
    const auto assertion=[storage](auto expression,auto file,auto line) { storage->AssertDisplay(expression,file,line); }; //선택된 송신 진단도 같은 Client 장치에 전달한다.
    for (const auto& spec:kReceivers) { //기존67h를 제외한14F만 설치하고 다른 앱 전역 표는 건드리지 않는다.
        if (spec.type==0x67) continue;
        if (spec.observer==ObserverKind::Login) dispatch.ObservePacket(spec.type,[storage](auto packet,auto sending) { ObserveLogin(packet,sending,storage); });
        else if (spec.observer==ObserverKind::Client) dispatch.ObservePacket(spec.type,[assertion,type=spec.type](auto packet,auto sending) { o::ObserveMissionClientPacket(type,packet,sending,assertion); });
        else if (spec.observer==ObserverKind::Metadata) dispatch.ObservePacket(spec.type,[storage](auto packet,auto sending) { o::ObserveMissionMetadataPacket(packet,sending,[storage](auto expression) { storage->AssertDisplay(expression,"\\Ns\\O\\_p_packets.cpp",0xb03); }); });
        else dispatch.Observe(spec.type,[](std::uint32_t) {}); //40b380의 실제 비NULL ret4다.
        dispatch.Version(spec.type,1,0);dispatch.RequireLength(spec.type,spec.length);
        if (spec.type==0x52) dispatch.RegisterCanonicalDirect(spec.type,spec.entry,[players=client.PlayerRecords()](auto sender,auto*,auto) { ReceiveBattleLeave(sender,players->Records()); }); //같은 현재 Player에 직접 쓰며 Session/연결/payload를 요구하지 않는다.
        else if (spec.type==0x64) dispatch.RegisterCanonicalDirect(spec.type,spec.entry,[client=&client](auto sender,auto*,auto) {
            o::ConnectionActivityHooks hooks; //실제 Wall/Frame/server는 원문 호출 위치에서 그때 읽는다.
            hooks.wallSeconds=[client] { return client->WallSeconds(); };hooks.frameNumber=[client] { return static_cast<std::uint32_t>(client->Time().number); };hooks.server=[client] { return client->SurfaceStorage()->Processes().State().Server(); };
            hooks.eraseStatus=[client](auto kind,auto player) { if (client->GameplayGlobals()->world.missionPost && kind!=11 && kind!=16) return;if (!client->Menu()) throw std::logic_error("활동 안내 제거의 실제 현재 UI 미연결");client->Menu()->RemoveNativePacketMessage(static_cast<std::int32_t>(kind),static_cast<std::int32_t>(player),0); };
            o::RawConnectionActivity(client->ConnectionActivityGlobals()->Tables(),std::move(hooks)).Receive(std::bit_cast<std::int32_t>(sender)); //현재 두 표의 앞선 store를 보존한 뒤 같은 실제 안내 제거를 호출한다.
        });
        else dispatch.RegisterCanonicalDirect(spec.type,spec.entry,[game,entry=spec.entry](auto sender,auto* connection,auto packet) {
            auto& session=game->RequireSession();const auto runtime=game->PinCurrentRuntime(session);session.ReceiveFixedLoginPacket(entry,sender,connection,packet); //이전 Login/Session/lease를 저장하거나 현재 packet종류/표로 G를 다시 선택하지 않는다.
        });
    }
}
// 원래 caller 위치에서 같은 불변 F/metadata를 확인하고 이번 lease만 붙인다.
void BindNativeLoginPacketReceiver(o::RawPacketDispatch& dispatch,std::uint32_t type) {
    for (const auto& expected:kReceivers) { //검토한 같은 행만 선택하며 미지 타입은 명시 실패한다.
        if (expected.type!=type) continue;
        const auto& row=dispatch.Registration(type);const auto receiver=dispatch.SnapshotReceiver(type);
        const bool observerMatches=expected.observer==ObserverKind::Empty ? bool(row.observer) && !row.packetObserver:bool(row.packetObserver) && !row.observer;
        if (!receiver || receiver->nativeEntry!=expected.entry || row.version!=1 || row.mismatch!=0 || row.length!=expected.length || !observerMatches) throw std::logic_error("Native Login의 실제 고정F/metadata 불일치");
        dispatch.RegisterCanonical(type,receiver);return;
    }
    throw std::invalid_argument("알 수 없는 Native Login 수신 등록 종류");
}
}
