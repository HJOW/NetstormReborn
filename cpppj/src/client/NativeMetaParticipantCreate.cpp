// 원문의 읽기/쓰기·복귀형 진단과 하위 효과 뒤 현재 값 재조회를 유지한다.
#include "client/NativeMetaParticipantCreate.h"
#include "client/RawChatChannels.h"
#include "client/CurrentAllocationBytes.h"
#include <bit>
#include <stdexcept>

namespace netstorm::client { //메타 모드/자기 Core 선택은 원문 State 수신 부모에서만 처리한다.
namespace { //원문 기록 폭과 실제 C문자열 물리만 확인한다.
template<class F> const F& Need(const F& hook) { if (!hook) throw std::logic_error("메타 참가자 생성의 실제 장치 미연결");return hook; } //도달한 원문 효과 위치에서만 요구한다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t at,std::size_t width=4) { if (at>bytes.size() || width>bytes.size()-at) throw std::out_of_range("참가자 생성의 실제 읽기 물리 부족");std::uint32_t result{};for (std::size_t byte=0;byte<width;++byte) result|=std::uint32_t(bytes[at+byte])<<(8*byte);return result; } //선택한 BYTE/WORD/DWORD만 낮은 BYTE부터 읽는다.
void Put(std::span<std::uint8_t> bytes,std::size_t at,std::uint32_t value,std::size_t width=4) { if (at>bytes.size() || width>bytes.size()-at) throw std::out_of_range("참가자 생성의 실제 쓰기 물리 부족");for (std::size_t byte=0;byte<width;++byte) bytes[at+byte]=static_cast<std::uint8_t>(value>>(8*byte)); } //같은 기록의 지정 폭만 쓰며 미작성 필드는 보존한다.
std::string_view Text(std::span<const std::uint8_t> bytes,std::size_t at=0) { if (at>bytes.size()) throw std::out_of_range("참가자 생성 문자열 시작 범위");const auto tail=bytes.subspan(at);std::size_t count{};while (count<tail.size() && tail[count]) ++count;if (count==tail.size()) throw std::out_of_range("참가자 생성 문자열 NUL 물리 부족");return {reinterpret_cast<const char*>(tail.data()),count}; } //현재 첫 NUL 뒤 stale BYTE를 읽지 않는다.
std::span<std::uint8_t> Record(std::span<std::uint8_t> cs,std::uint32_t index,bool battle) { const auto count=battle ? o::kChallengeBattleCount:o::kChallengeParticipantCount,offset=battle ? o::kChallengeBattleOffset:o::kChallengeParticipantOffset,stride=battle ? o::kChallengeBattleBytes:o::kChallengeParticipantBytes;if (index>=count || cs.size()!=o::kChallengeBytes) throw std::out_of_range("참가자 수신의 캡처 CS 기록 물리 범위");return cs.subspan(offset+index*stride,stride); } //current가 바뀌어도 진입CS의 같은 실제 기록을 반환한다.
}
// 수신 observer는 필드를 읽지 않으며 진단이 복귀하면 현재 길이 BYTE를 다시 읽는다.
void ObserveNativeMetaParticipantAdd(std::span<std::uint8_t> packet,bool sending,const std::function<void(std::string_view,std::uint32_t)>& assertion) {
    if (!sending) return;if (Read(packet,0x12,1)==0xffU) Need(assertion)("nameLen != (unsigned char)-1",0x534);Put(packet,2,Read(packet,0x12,1)+0x35U,2);
}
// prefix/이름 포인터와 localflag는 원문 지역값이며 후행 현재 전역과 구별한다.
void ConstructNativeMetaParticipant(std::span<std::uint8_t> r,std::span<const std::uint8_t> p,std::uint32_t& marker,const std::uint32_t& subscriber,const NativeMetaParticipantCreateHooks& h) {
    Put(r,0,Read(p,4,1));Put(r,4,Read(p,5));Put(r,0x4e,Read(p,0x13));Put(r,0x52,Read(p,0x17,2),2);Put(r,0x54,Read(p,0x19));Put(r,0x58,Read(p,0x1d));Put(r,0x5c,Read(p,0x21,2),2);Put(r,0x64,Read(p,0x23)&0xffffffbfU);Put(r,0x9c,Read(p,0x27));Put(r,0xa0,Read(p,0x2b));Put(r,0xa4,Read(p,0x2f,1));Put(r,0xa8,Read(p,0x30,1));Put(r,0x60,Read(p,0x31));
    std::array<std::uint8_t,60> guild;CurrentAllocationBytes(guild);guild[0]=0;
    for (std::size_t index=0;;++index) { const auto value=Read(p,0x35+index,1);Put(r,0x26+index,value,1);if (!value) break; } //strcpy의 실제 BYTE 부분 효과/NUL을 보존한다.
    if (Need(h.guildChanged)(Read(r,4))) { Need(h.guild)(Read(r,4),guild);if (Read(guild,0,1)) { const auto end=Text(guild).size();Put(guild,end,0x003a797eU); } } //첫 확인 뒤 현재subscriber를 다시 읽고60BYTE 목적 끝에 PE '~y:'/NUL을 DWORD로 붙인다.
    SanitizePlayerName(Read(p,5),r.subspan(0x26));Put(r,0xb4,Read(p,9,2),2);Put(r,0xb2,Read(p,0xb,2),2);Put(r,0xb0,10,2);Put(r,0xac,Read(p,0xd,1));Need(h.refreshCore)();bool local=false;
    if (marker || Read(r,4)==subscriber) { Need(h.status)(5,Read(r,0),false);Need(h.status)(3,Read(r,0),true);local=true;marker=1; }
    std::array<std::uint8_t,6> identity;Put(identity,0,Read(r,0x4e));Put(identity,4,Read(r,0x52,2),2);Need(h.image)(r.subspan(0xbc,20),identity);if (!Read(r,0xc0)) Need(h.display.assertion)("image.shape != NULL","\\Ns\\O\\Ch.cpp",0x2ee);
    const auto type=Need(h.participantType)();const auto object=Need(h.create)(type,2);Put(r,0xb8,object);Need(h.display.setKind)(object,Read(r,0));const auto raw=Need(h.display.raw)(object);if (!(Read(raw,0xb,1)&4U)) Need(h.display.assertion)("isVoid()","c:\\ns\\o\\squid.h",0x282);Put(raw,40,Read(raw,40,1)&0xfeU,1);Need(h.display.paintOwner)(object,Read(r,0));Put(raw,8,0,2);Put(raw,0x1a,0);const auto y=Read(r,0xa0),x=Read(r,0x9c);Need(h.display.move)(object,std::bit_cast<float>(x),std::bit_cast<float>(y),0);
    WriteNativeMetaParticipantLabel(r,Text(guild),h.name);const auto label=Need(h.createName)();Need(h.selectable)(label,1);Need(h.display.replaceText)(label,"",0);Need(h.scrollable)(label,0);Need(h.reference)(label,object);Put(r,0xd0,Need(h.display.gump)(label).serial);Put(r,0xd4,label);Need(h.display.attach)(label,0,0,Need(h.root)(),1);
    if (!(Read(p,0x23,1)&0x40U)) Need(h.enter)(Read(r,0xb8),Read(r,4)==subscriber);
    if (Read(r,4)==subscriber) { Need(h.intro)();if (!Need(h.noIntro)()) Need(h.tell)("ChalIntro(1)"); }
    if (local) Need(h.monitor)(r.subspan(0x26));
}
// 원래this CS와 참가자 포인터를 보유하고 생성/진단/표시 이후 필요한 필드를 다시 읽는다.
void ReceiveNativeMetaParticipantAdd(ClientChallengeGlobals& challenge,std::span<const std::uint8_t> packet,const NativeMetaParticipantAddHooks& h) {
    const auto cs=challenge.Bytes();const auto r=Record(cs,Read(packet,4,1),false);if (Read(r,0x64,1)&1U) Need(h.assertion)("!p->isValid()","\\Ns\\O\\Ch.cpp",0x5b4);Need(h.construct)(r,packet);
    const auto battle=Read(r,0xa4);if (battle) { const auto seat=Read(r,0xa8),participant=Read(packet,4,1);const auto b=Record(cs,battle,true);if (seat>=8) throw std::out_of_range("참가자 수신의 실제 전투 좌석8칸 범위");if (Read(b,0x38+seat*4)) Need(h.assertion)("!playerIds[slot]","\\Ns\\O\\Ch.cpp",0x1c8);Put(b,0x38+seat*4,participant);Need(h.battle)(Read(b,4));challenge.UpdateBattleVersionBounds(cs,std::bit_cast<std::int32_t>(Read(r,0xa4)),[&h](auto expression,auto line) { Need(h.assertion)(expression,"\\Ns\\O\\Ch.cpp",line); }); }
    const auto currentBattle=Read(r,0xa4);if (currentBattle) { const auto position=Need(h.seat)(r,currentBattle,Read(r,0xa8));Put(r,0x9c,std::bit_cast<std::uint32_t>(position[0]));Put(r,0xa0,std::bit_cast<std::uint32_t>(position[1])); }
    if (packet.size()<0x35) throw std::out_of_range("참가자 Chat 이름 주소의 실제 packet 물리 부족");const auto name=packet.subspan(0x35);Need(h.chat)(Read(packet,5),name,0x20);Need(h.update)(Read(packet,4,1),0xff);Need(h.refreshInfo)();
}
}
