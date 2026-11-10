// 실제 모듈 본문은 Session의 공통 진입점에만 두고 고정 F는 장기 전역만 보유한다.
#include "client/NativeCarrierPacketReceivers.h"
#include "client/ClientGameplayGlobals.h"
#include "client/RawCampaignSession.h"
#include "client/RawSurfaceWorld.h"
#include "o/RawBombEffect.h"
#include "o/RawBombTimerPacket.h"
#include "o/RawMeltdown.h"
#include <array>
#include <stdexcept>

namespace netstorm::client { // 고정 코드값으로 현재 실제 몸체를 직접 호출한다.
namespace { // 원문40b4b0의 검토한15행과 그 metadata 검사만 한정한다.
enum class ObserverKind { Empty,Effect,Timer,Meltdown }; // 비NULL40b380과 세 실제 가변 길이 observer를 구별한다.
struct ReceiverSpec { // 보호10.78의 고정 F와 등록 정보 한 행이다.
    std::uint32_t type,entry; // 메시지 번호와 불변 Native 코드 주소다.
    std::uint16_t length; // 보호 PE 길이WORD이며ffff는 가변이다.
    ObserverKind observer; // 현재 mutable observer와 비NULL 빈 observer의 구분이다.
};
constexpr std::array<ReceiverSpec,15> kReceivers{{ // 원문 등록 함수의 전체 해당 쓰기와 PE0052ec40을 대조한 값이다.
    {0x77,0x0048f7f0U,6,ObserverKind::Empty},{0x79,0x0048f8f0U,6,ObserverKind::Empty},{0x6f,0x004900e0U,6,ObserverKind::Empty},
    {0x40,0x00428570U,18,ObserverKind::Empty},{0x4f,0x00426520U,9,ObserverKind::Empty},{0x50,0x00444bc0U,9,ObserverKind::Empty},
    {0x60,0x004267c0U,10,ObserverKind::Empty},{0x61,0x0048bfe0U,11,ObserverKind::Empty},{0x5c,0x0041e860U,6,ObserverKind::Empty},
    {0x5f,0x00420bf0U,0xffff,ObserverKind::Effect},{0x7d,0x0041e320U,0xffff,ObserverKind::Timer},{0x6b,0x00448e90U,10,ObserverKind::Empty},
    {0x6d,0x00493f80U,8,ObserverKind::Empty},{0x75,0x004205f0U,0xffff,ObserverKind::Meltdown},{0x76,0x004207a0U,6,ObserverKind::Empty}
}};
}
// 같은 장기 game/진단만 보유하며 등록 시점에는 현재 Session/Player/모드를 조회하지 않는다.
void InstallNativeCarrierPacketReceivers(o::RawPacketDispatch& dispatch,const std::shared_ptr<ClientGameplayGlobals>& game,const std::shared_ptr<RawSurfaceStorage>& storage) {
    if (!game || !storage || !storage->UsesNativeTransmissionDevices()) throw std::invalid_argument("Native Carrier의 실제 Client 소유자 미연결");
    for (const auto& spec:kReceivers) { // 검토한15행만 설치하고 나머지 현재 표는 보존한다.
        if (spec.observer==ObserverKind::Effect) dispatch.ObservePacket(spec.type,[storage](auto packet,auto sending) { o::ObserveBombEffectPacket(packet,sending,[storage] { storage->AssertDisplay("count != (int)-1","\\Ns\\O\\_p_packets.cpp",0x96b); }); });
        else if (spec.observer==ObserverKind::Timer) dispatch.ObservePacket(spec.type,[](auto packet,auto sending) { o::FinalizeBombTimerPacket(packet,sending!=0); });
        else if (spec.observer==ObserverKind::Meltdown) dispatch.ObservePacket(spec.type,[storage](auto packet,auto sending) { o::RawMeltdown::FinalizePacket(packet,sending!=0,[storage] { storage->AssertDisplay("count != (int)-1","\\Ns\\O\\_p_packets.cpp",0xb23); }); });
        else dispatch.Observe(spec.type,[](std::uint32_t) {}); //40b380은 실제 비NULL ret4다.
        dispatch.Version(spec.type,1,0);dispatch.RequireLength(spec.type,spec.length);
        dispatch.RegisterCanonicalDirect(spec.type,spec.entry,[game,entry=spec.entry](auto sender,auto*,auto packet) {
            auto& session=game->RequireSession();const auto runtime=game->PinCurrentRuntime(session);session.ReceiveFixedCarrierPacket(entry,sender,packet); // 저장된 F값으로 같은 몸체를 직접 호출하며 packet[1]/현재 표로 G를 재선택하지 않는다.
        });
    }
}
// 원래 caller 위치에서 같은 불변 F와 현재 metadata를 확인한 뒤 lease만 다시 붙인다.
void BindNativeCarrierPacketReceiver(o::RawPacketDispatch& dispatch,std::uint32_t type) {
    for (const auto& expected:kReceivers) { // 검토한 한 행만 선택하며 미지 타입을 다른 함수에 넘기지 않는다.
        if (expected.type!=type) continue;
        const auto& row=dispatch.Registration(type);const auto receiver=dispatch.SnapshotReceiver(type);
        const bool observerMatches=expected.observer==ObserverKind::Empty ? bool(row.observer) && !row.packetObserver:bool(row.packetObserver) && !row.observer;
        if (!receiver || receiver->nativeEntry!=expected.entry || row.version!=1 || row.mismatch!=0 || row.length!=expected.length || !observerMatches) throw std::logic_error("Native Carrier의 실제 고정F/metadata 불일치");
        dispatch.RegisterCanonical(type,receiver);return;
    }
    throw std::invalid_argument("알 수 없는 Native Carrier 수신 등록 종류");
}
}
