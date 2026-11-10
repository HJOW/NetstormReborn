// 이동·Squid·전투·문자열26F는 기존 모듈 본문을 복제하지 않는다.
#include "client/NativeGameplayPacketReceivers.h"
#include "client/ClientGameplayGlobals.h"
#include "client/RawCampaignSession.h"
#include "client/RawSurfaceWorld.h"
#include "o/RawPathData.h"
#include "o/RawDeveloperCommandPacket.h"
#include "o/RawRemoteTellPacket.h"
#include "o/RawSquidMutationReceive.h"
#include "o/RawSquidCompressedReceive.h"
#include <array>
#include <stdexcept>
namespace netstorm::client { //불변 코드값으로 현재 같은 몸체를 직접 호출한다.
namespace { //원문40b4b0의 검토한26행과 그 대여 검사를 한정한다.
enum class ObserverKind { None,Empty,Path,Developer,Delete,Tell,Compressed }; //NULL/비NULL 빈/다섯 실제 mutable observer를 구별한다.
struct ReceiverSpec { //보호10.78의 실제 등록 한 행이다.
    std::uint32_t type,entry; //메시지 번호와 불변 Native 코드 주소다.
    std::uint16_t length; //보호 PE 길이WORD이며ffff는 가변이다.
    ObserverKind observer; //원문 observer의 실제 종류다.
};
constexpr std::array<ReceiverSpec,26> kReceivers{{ //32h는 기존 장기7DWORD 수신기와 등록 부모를 그대로 사용한다.
    {0x2c,0x00433c30U,35,ObserverKind::Empty},
    {0x2d,0x004327b0U,18,ObserverKind::Empty},
    {0x2e,0x00433170U,8,ObserverKind::Empty},
    {0x30,0x00489ee0U,0xffff,ObserverKind::Path},
    {0x31,0x0044dba0U,0xffff,ObserverKind::Developer},
    {0x33,0x004a71c0U,18,ObserverKind::Empty},
    {0x34,0x004333f0U,12,ObserverKind::Empty},
    {0x35,0x0046e940U,20,ObserverKind::Empty},
    {0x38,0x0045e410U,0xffff,ObserverKind::None},
    {0x39,0x0045e2b0U,0xffff,ObserverKind::None},
    {0x3a,0x0046a0c0U,0xffff,ObserverKind::None},
    {0x3b,0x00466d30U,0xffff,ObserverKind::None},
    {0x3c,0x00466b60U,0xffff,ObserverKind::None},
    {0x4a,0x00432710U,7,ObserverKind::Empty},
    {0x4b,0x004326e0U,6,ObserverKind::Empty},
    {0x53,0x004436b0U,6,ObserverKind::Empty},
    {0x5b,0x004266c0U,6,ObserverKind::Empty},
    {0x62,0x00433120U,0xffff,ObserverKind::Delete},
    {0x65,0x00432850U,13,ObserverKind::Empty},
    {0x6a,0x0044a280U,0xffff,ObserverKind::None},
    {0x70,0x004cf980U,0xffff,ObserverKind::Tell},
    {0x73,0x0045e5d0U,0xffff,ObserverKind::None},
    {0x74,0x0045e4f0U,0xffff,ObserverKind::None},
    {0x7a,0x00434080U,0xffff,ObserverKind::Compressed},
    {0x7e,0x00497030U,28,ObserverKind::Empty},
    {0x86,0x00426490U,20,ObserverKind::Empty},
}};
}
//같은 장기 game/진단만 보유하며 초기 등록은 현재 Session/Player/모드를 조회하지 않는다.
void InstallNativeGameplayPacketReceivers(o::RawPacketDispatch& dispatch,const std::shared_ptr<ClientGameplayGlobals>& game,const std::shared_ptr<RawSurfaceStorage>& storage) {
    if (!game || !storage || !storage->UsesNativeTransmissionDevices()) throw std::invalid_argument("Native gameplay의 실제 Client 소유자 미연결");
    const auto assertion=[storage](auto expression,auto file,auto line) { storage->AssertDisplay(expression,file,line); }; //원문 송신 진단의 실제 Client 소유자만 보유한다.
    for (const auto& spec:kReceivers) { //검토한26행만 설치하고 기존32h와 다른 현재 표는 보존한다.
        switch (spec.observer) {
        case ObserverKind::None:dispatch.Observe(spec.type,{});break;
        case ObserverKind::Empty:dispatch.Observe(spec.type,[](std::uint32_t) {});break; //40b380의 실제 비NULL ret4다.
        case ObserverKind::Path:dispatch.ObservePacket(spec.type,[assertion](auto packet,auto sending) { o::ObservePathData(packet,sending,assertion); });break;
        case ObserverKind::Developer:dispatch.ObservePacket(spec.type,[assertion](auto packet,auto sending) { o::ObserveDeveloperCommandPacket(packet,sending,assertion); });break;
        case ObserverKind::Delete:dispatch.ObservePacket(spec.type,[assertion](auto packet,auto sending) { o::ObserveRemoteDeletedSquids(packet,sending,assertion); });break;
        case ObserverKind::Tell:dispatch.ObservePacket(spec.type,[assertion](auto packet,auto sending) { o::ObserveRemoteTellPacket(packet,sending,assertion); });break;
        case ObserverKind::Compressed:dispatch.ObservePacket(spec.type,[assertion](auto packet,auto sending) { o::ObserveCompressedSquid(packet,sending,assertion); });break;
        }
        dispatch.Version(spec.type,1,0);dispatch.RequireLength(spec.type,spec.length);
        dispatch.RegisterCanonicalDirect(spec.type,spec.entry,[game,entry=spec.entry](auto,auto*,auto packet) {
            auto& session=game->RequireSession();const auto runtime=game->PinCurrentRuntime(session);session.ReceiveFixedGameplayPacket(entry,packet); //26F는 sender/연결을 읽지 않으며 packet종류/현재 표로 G를 다시 고르지 않는다.
        });
    }
}
//원래 caller 위치에서 같은 불변 F와 metadata만 확인해 lease를 붙인다.
void BindNativeGameplayPacketReceiver(o::RawPacketDispatch& dispatch,std::uint32_t type) {
    for (const auto& expected:kReceivers) { //검토한 한 행만 선택하고 미지 타입을 다른 F에 넘기지 않는다.
        if (expected.type!=type) continue;
        const auto& row=dispatch.Registration(type);const auto receiver=dispatch.SnapshotReceiver(type);
        const bool observerMatches=expected.observer==ObserverKind::None ? !row.observer && !row.packetObserver:expected.observer==ObserverKind::Empty ? bool(row.observer) && !row.packetObserver:bool(row.packetObserver) && !row.observer;
        if (!receiver || receiver->nativeEntry!=expected.entry || row.version!=1 || row.mismatch!=0 || row.length!=expected.length || !observerMatches) throw std::logic_error("Native gameplay의 실제 고정F/metadata 불일치");
        dispatch.RegisterCanonical(type,receiver);return;
    }
    throw std::invalid_argument("알 수 없는 Native gameplay 수신 등록 종류");
}
}
