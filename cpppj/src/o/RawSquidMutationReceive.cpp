// Take 전 상태와 콜백 뒤 현재 상태/TYPE를 구별하고 삭제 뒤 SP 추적을 연결한다.
#include "o/RawSquidMutationReceive.h"
#include <stdexcept>

namespace netstorm::o { // 별도 객체/자유 목록/Player 사본을 만들지 않는다.
namespace { // 원문 물리 폭과 선택된 장치를 공유한다.
template<class F> F NeedMutation(const F& function) { if (!function) throw std::logic_error("객체 변경 수신의 선택된 실제 장치 미연결");return function; } // 현재 callable의 수명을 보존한다.
std::uint16_t MutationWord(std::span<const std::uint8_t> bytes,std::size_t offset) { return static_cast<std::uint16_t>(bytes[offset]|(std::uint16_t(bytes[offset+1])<<8)); } // 주변 payload를 섞지 않는다.
void MutationPutWord(std::span<std::uint8_t> raw,std::size_t offset,std::uint16_t value) { raw[offset]=static_cast<std::uint8_t>(value);raw[offset+1]=static_cast<std::uint8_t>(value>>8); } // 원문 WORD만 저장한다.
}
void ReceiveContentSquid(SidPool& pool,std::span<const std::uint8_t> packet,const SquidMutationHooks& hooks) {
    if (pool.Edition()!=OriginalEdition::Patch1078 || packet.size()<18) throw std::invalid_argument("내용물 수신의 PATCH18BYTE 계약");const Sid incoming{MutationWord(packet,4)};const auto old=pool.Slot(incoming)[11];
    const auto object=NeedMutation(hooks.take)(NeedMutation(hooks.contentType)(),incoming);NeedMutation(hooks.word)(object,MutationWord(packet,6));
    const auto slot=pool.Slot(object);auto raw=std::span(const_cast<std::uint8_t*>(slot.data()),slot.size());MutationPutWord(raw,18,MutationWord(packet,8));MutationPutWord(raw,20,MutationWord(packet,10));MutationPutWord(raw,24,MutationWord(packet,12));raw[23]=packet[16];
    const auto received=packet[17];raw[11]=received|4U;std::uint32_t flags=8;
    if ((received&0x10U) || (old&1U)) { flags=NeedMutation(hooks.firstPop)(object)|8U;raw[11]&=0xefU; }
    NeedMutation(hooks.attach)(object,Sid{MutationWord(packet,14)},flags);
}
void ReceiveDeletedSquid(const SidPool& pool,std::span<const std::uint8_t> packet,const SquidMutationHooks& hooks) {
    if (pool.Edition()!=OriginalEdition::Patch1078 || packet.size()<8) throw std::invalid_argument("삭제 수신의 PATCH8BYTE 계약");const Sid object{MutationWord(packet,4)};const auto raw=pool.Slot(object);
    if (NeedMutation(hooks.boss)()) NeedMutation(hooks.assertion)("!boss",".\\Clientboard.cpp",0x260);
    const auto name=NeedMutation(hooks.typeName)(object);NeedMutation(hooks.log)(0x1d,"destroying "+name+"\n");
    if (!(raw[11]&1U)) {
        const auto flags=std::uint32_t(MutationWord(packet,6))<<16;if (raw[10]>=70 && (flags&0x200000U)) NeedMutation(hooks.refund)(object,raw[34]);
        NeedMutation(hooks.destroy)(object,flags|8U);
    } else if (raw[10]!=NeedMutation(hooks.dependType)()) NeedMutation(hooks.log)(0x1d,"~rdouble destroy");
    NeedMutation(hooks.auditSp)();
}
void ReceiveDependencySquid(SidPool& pool,std::span<const std::uint8_t> packet,const SquidMutationHooks& hooks) { //65h는 firstPop/소유자 지정 없이 현재 감시 가상74를 호출한다.
    if (pool.Edition()!=OriginalEdition::Patch1078 || packet.size()<13) throw std::invalid_argument("감시 수신의 PATCH13BYTE 계약");const auto object=NeedMutation(hooks.take)(NeedMutation(hooks.dependType)(),Sid{MutationWord(packet,4)});
    const auto slot=pool.Slot(object);auto raw=std::span(const_cast<std::uint8_t*>(slot.data()),slot.size());raw[11]=packet[8];MutationPutWord(raw,18,MutationWord(packet,9));MutationPutWord(raw,20,MutationWord(packet,11));raw[11]|=4U;
    NeedMutation(hooks.dependencyAttach)(object,Sid{MutationWord(packet,6)},0);
}
}
