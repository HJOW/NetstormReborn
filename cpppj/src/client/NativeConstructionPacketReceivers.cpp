// 실제 건설·연구 본문은 기존 모듈 하나만 사용하며 enqueue 시점의 Session/lease를 고정F에 넣지 않는다.
#include "client/NativeConstructionPacketReceivers.h"
#include "client/ClientGameplayGlobals.h"
#include "client/RawCampaignSession.h"
#include "client/RawSurfaceWorld.h"
#include "o/RawConstructionPacket.h"
#include <array>
#include <stdexcept>

namespace netstorm::client { // 같은 Client 전역과 불변 코드 식별자만 장기 수신기에 보유한다.
namespace { //원문40b4b0의 여덟 행과 이번 Session 대여 검사를 한정한다.
enum class ObserverKind { None,Empty,Construction }; //NULL·40b380·40a800의 실제 서로 다른 observer 슬롯이다.
struct ReceiverSpec { //실제 원문 수신/metadata 한 행이다.
    std::uint32_t type,entry; //메시지 번호와 고정 Native F의 코드 식별자다.
    std::uint16_t length; //원문 고정WORD 또는 가변ffff다.
    ObserverKind observer; //해당 행의 NULL/빈/건설 최종화 observer다.
};
constexpr std::array<ReceiverSpec,8> kReceivers{{{0x3e,0x00444af0U,25,ObserverKind::Empty},{0x3f,0x004441b0U,0xffff,ObserverKind::Construction},{0x37,0x004427b0U,0xffff,ObserverKind::None},{0x58,0x00453d70U,16,ObserverKind::Empty},{0x54,0x004436f0U,8,ObserverKind::Empty},{0x51,0x004dc050U,8,ObserverKind::Empty},{0x2f,0x004332c0U,15,ObserverKind::Empty},{0x6c,0x004d56f0U,11,ObserverKind::Empty}}}; //보호10.78의 실제 등록 함수/길이 표와 대조한 건설계 여덟 행이다.
//metadata/코드 정체성을 검증하고 같은 F값만 이번 등록 lease에 다시 붙인다.
void BindReceiver(o::RawPacketDispatch& dispatch,const ReceiverSpec& expected) {
    const auto& row=dispatch.Registration(expected.type);const auto receiver=dispatch.SnapshotReceiver(expected.type);
    const bool observerMatches=expected.observer==ObserverKind::None ? !row.observer && !row.packetObserver:expected.observer==ObserverKind::Construction ? bool(row.packetObserver) && !row.observer:bool(row.observer) && !row.packetObserver;
    if (!receiver || receiver->nativeEntry!=expected.entry || row.version!=1 || row.mismatch!=0 || row.length!=expected.length || !observerMatches) throw std::logic_error("Native 건설/연구의 실제 고정F/metadata 불일치");
    dispatch.RegisterCanonical(expected.type,receiver);
}
}
//초기 등록은 같은 장기 전역만 캡처하고 현재 Session/Player/모드를 읽지 않는다.
void InstallNativeConstructionPacketReceivers(o::RawPacketDispatch& dispatch,const std::shared_ptr<ClientGameplayGlobals>& game,const std::shared_ptr<RawSurfaceStorage>& storage) {
    if (!game || !storage || !storage->UsesNativeTransmissionDevices()) throw std::invalid_argument("Native 건설/연구의 실제 Client 소유자 미연결");
    for (const auto& spec:kReceivers) { //여덟 원문 행만 설치하고 다른 현재 receiver/observer에는 손대지 않는다.
        if (spec.observer==ObserverKind::Construction) dispatch.ObservePacket(spec.type,[storage](auto packet,auto sending) { o::FinalizeConstructionPacket(packet,sending!=0,[storage] { storage->AssertDisplay("sidCount != (unsigned char)-1","\\Ns\\O\\_p_packets.cpp",0x7c9); }); });
        else if (spec.observer==ObserverKind::None) dispatch.Observe(spec.type,{});
        else dispatch.Observe(spec.type,[](std::uint32_t) {}); //40b380의 실제 비NULL 빈 ret4다.
        dispatch.Version(spec.type,1,0);dispatch.RequireLength(spec.type,spec.length);
        dispatch.RegisterCanonicalDirect(spec.type,spec.entry,[game,entry=spec.entry](auto sender,auto*,auto packet) {
            auto& session=game->RequireSession();const auto runtime=game->PinCurrentRuntime(session);session.ReceiveFixedConstructionPacket(entry,sender,packet); //저장된 F값으로 현재 실제 본문만 호출하며 packet[1]/현재 표의 G로 재분배하지 않는다.
        });
    }
}
//3e→3f의 원래 등록 순서를 유지한다.
void BindNativeConstructionCommandReceivers(o::RawPacketDispatch& dispatch) { BindReceiver(dispatch,kReceivers[0]);BindReceiver(dispatch,kReceivers[1]); }
//buildingData 확인 뒤에만37을 대여하는 기존 caller 순서를 유지한다.
void BindNativeConstructionDataReceiver(o::RawPacketDispatch& dispatch) { BindReceiver(dispatch,kReceivers[2]); }
//연구의 기존 현재 본문/metadata와 같은 F값을 대여한다.
void BindNativeFactoryResearchReceiver(o::RawPacketDispatch& dispatch) { BindReceiver(dispatch,kReceivers[3]); }
//각 caller의 기존 등록 순서에서만 해당 한 행을 대여하고 다른 행의 쓰기를 앞당기지 않는다.
void BindNativeConstructionReceiver(o::RawPacketDispatch& dispatch,std::uint32_t type) {
    for (const auto& spec:kReceivers) if (spec.type==type) { BindReceiver(dispatch,spec);return; } //검토한 같은 행만 선택하며 미지 타입을 다른 본문으로 보내지 않는다.
    throw std::invalid_argument("알 수 없는 Native 건설 수신 등록 종류");
}
}
