// Root 내부 직접 호출과 원격 CCh 요약의 실제 갱신 순서를 보존한다.
#include "root/Routing.h"
#include "root/ChallengeDetail.h"
#include "o/PacketRegistrationTable.h"

namespace netstorm::root { // Player/가짜 연결을 만들지 않는 같은 R 등록표의 실제 부모다.
using namespace challenge_detail; // 현재 비정렬 바이트/선택 장치만 공유한다.
Routing::Routing(ConnectionState& state,TransceiverTransport& transport,ChallengeStore& challenges,MirrorPlayerDefaults& defaults,std::function<void(std::int32_t,std::string_view)> log)
    :state_(state),transport_(transport),challenges_(challenges),defaults_(defaults),log_(std::move(log)) {}
void Routing::Assert(std::string_view expression,std::uint32_t line) { Need(transport_.Lifecycle().assertion)(expression,"c:\\ns\\r\\r.cpp",line); }
void Routing::Install() {
    auto& dispatch=transport_.PacketHandlers();dispatch.ObservePacket(0x17,[](auto,auto) {});dispatch.Version(0x17,1,0);dispatch.RequireLength(0x17,14);dispatch.RegisterDirect(0x17,[this](auto sender,auto*,auto packet) { ReceiveChange(std::bit_cast<std::int32_t>(sender),packet); }); // 현재 실제 등록표가 네트워크/NULL 직접 호출 양쪽을 소유한다.
}
void Routing::Relay(std::uint32_t challenge,std::span<std::uint8_t> packet) {
    static_cast<void>(challenge);if (!(state_.modeFlags&kChallengeMode)) Assert("smModeFlags & smCHAL",247);const auto type=Read(packet,1,1);if (type==0x7f || type==0x80) return; // 사용자 지시의 폐기 검증은 내부 직접 경로에서도 제외한다.
    if (!(state_.modeFlags&kRootMode)) { const auto current=state_.transceivers[1];if (current) transport_.Send(current,packet);return; }
    auto& dispatch=transport_.PacketHandlers();if (type>0 && type<=dispatch.Registrations()->LastType() && dispatch.Registration(type).receive) dispatch.InvokeDirect(type,0,nullptr,packet); // 현재 타입/현재 등록을 읽고 프레임 부작용 없이 호출한다.
}
void Routing::RootPostChange() { if (!(state_.modeFlags&kRootMode)) Assert("smModeFlags & smROOT",358); } // 제공 R의 실제 함수는 이 진단 외에는 수행하지 않는다.
void Routing::ReceiveChange(std::int32_t sender,std::span<const std::uint8_t> packet) {
    if (state_.exitRequested) { Need(transport_.Lifecycle().exit)(0);throw std::logic_error("R Root 통지의 _exit 장치가 복귀했습니다"); }
    if (sender<0) { Need(log_)(9,"RootNotifyPlayerChange received from invalid transceiver, id == "+std::to_string(sender)+"\n");return; }
    const auto zone=std::bit_cast<std::int32_t>(Read(packet,5));Need(log_)(29,"RootNotifyPlayerChange: id == "+std::to_string(sender)+", chalId == "+std::to_string(zone)+"\n");
    if (zone<1 || zone>=state_.challengeStateCount || !state_.challenges.at(static_cast<std::size_t>(zone))) { Need(log_)(9,"notify player change came from unexpected challenge state id\n");return; }
    if (!(Read(challenges_.Bytes(state_.challenges[static_cast<std::size_t>(zone)]),4)&1U)) ApplyChange(state_.challenges[static_cast<std::size_t>(zone)],packet);if (state_.modeFlags&kRootMode) RootPostChange();
}
void Routing::ApplyChange(std::uint32_t challenge,std::span<const std::uint8_t> packet) {
    const auto subscriber=Read(packet,9);
    if (!Read(packet,4,1)) {
        for (std::size_t participant=0;participant<40;++participant) { // 원문 삭제 검색은 활성 여부와 무관하게0번도 포함한 첫subscriber만 선택한다.
            auto player=challenges_.Bytes(challenge).subspan(kChallengeParticipantOffset+participant*kChallengeParticipantBytes,kChallengeParticipantBytes);if (Read(player,4)!=subscriber) continue;std::fill(player.begin(),player.end(),std::uint8_t{});return;
        }
        return;
    }
    for (std::uint32_t participant=1;participant<40;++participant) { // 중복 subscriber 관문 없이 실제 첫 빈1..39를 선택한다.
        auto player=challenges_.Bytes(challenge).subspan(kChallengeParticipantOffset+participant*kChallengeParticipantBytes,kChallengeParticipantBytes);if (Read(player,0x64)&1U) continue;InitializeParticipant(player,participant,subscriber,std::nullopt,defaults_.identity,defaults_.core,defaults_.position,0);Put(player,0x5e,Read(packet,13,1),1);return;
    }
}
}
