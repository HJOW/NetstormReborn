// Root의 실제 CCh 직접 전달과41h 전체 안내 패킷을 같은 현재 등록표에 연결한다.
#include "root/Routing.h"
#include "root/ChallengeDetail.h"
#include "o/PacketRegistrationTable.h"
#include <limits>

namespace netstorm::root { // 가짜 연결/새 Player를 만들지 않는 Root 현재 경로다.
using namespace challenge_detail; // 원문의 ff allocator·지정 폭·문자열 경계를 공유한다.
void Routing::SendChallenge(std::int32_t challenge,std::span<std::uint8_t> packet,std::int32_t descriptor) {
    if (!(state_.modeFlags&kRootMode)) Assert("smModeFlags & smROOT",321);if (challenge<1 || challenge>=state_.challengeStateCount) return;const auto token=state_.challenges.at(static_cast<std::size_t>(challenge));if (!token || !(Read(challenges_.Bytes(token),4)&1U)) return;if (descriptor!=-1) Assert("fd == INVALID_SOCKET",325);const auto type=Read(packet,1,1);if (type==0x7f || type==0x80) return;
    auto& dispatch=transport_.PacketHandlers();if (type>0 && type<=dispatch.Registrations()->LastType() && dispatch.Registration(type).receive) dispatch.InvokeDirect(type,static_cast<std::uint32_t>(challenge)<<8U,nullptr,packet);
}
void Routing::BroadcastText(std::string_view text) {
    text=text.substr(0,text.find('\0'));std::vector<std::uint8_t> bytes(text.begin(),text.end());bytes.push_back(0);BroadcastBuffer(bytes); // 원문 param2==0은 strlen+1을 실제 전달 길이로 사용한다.
}
void Routing::BroadcastBuffer(std::span<const std::uint8_t> text) {
    if (text.empty() || text.size()>std::numeric_limits<std::uint16_t>::max()) throw std::out_of_range("R Root 안내의 실제 비어 있지 않은 WORD 본문 길이 필요");auto packet=Packet(0x41,text.size()+20);Put(packet,0,3,1);Put(packet,5,1,1);Put(packet,6,0);Put(packet,14,1,1);Put(packet,12,static_cast<std::uint32_t>(text.size()),2);Put(packet,10,1,2);Copy(packet,16,text);Put(packet,15,0,1);Put(packet,16+text.size(),0);
    for (std::int32_t challenge=1;challenge<51;++challenge) if (state_.challenges[static_cast<std::size_t>(challenge)]) SendChallenge(challenge,packet,-1); // 원문 전체51표의 현재 비영 항목을 읽고 실제 local/개수 관문을 하위에서 다시 적용한다.
}
}
