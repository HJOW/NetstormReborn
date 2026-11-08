// Take 전 상태와 콜백 뒤 현재 상태/TYPE를 구별하고 삭제 뒤 SP 추적을 연결한다.
#include "o/RawSquidMutationReceive.h"
#include <stdexcept>
#include <bit>

namespace netstorm::o { // 별도 객체/자유 목록/Player 사본을 만들지 않는다.
namespace { // 원문 물리 폭과 선택된 장치를 공유한다.
template<class F> F NeedMutation(const F& function) { if (!function) throw std::logic_error("객체 변경 수신의 선택된 실제 장치 미연결");return function; } // 현재 callable의 수명을 보존한다.
std::uint16_t MutationWord(std::span<const std::uint8_t> bytes,std::size_t offset) { return static_cast<std::uint16_t>(bytes[offset]|(std::uint16_t(bytes[offset+1])<<8)); } // 주변 payload를 섞지 않는다.
void MutationPutWord(std::span<std::uint8_t> raw,std::size_t offset,std::uint16_t value) { raw[offset]=static_cast<std::uint8_t>(value);raw[offset+1]=static_cast<std::uint8_t>(value>>8); } // 원문 WORD만 저장한다.
// 원격 삭제의 각 필드를 선택한 순간 물리 폭만 확인하며 전체 목록을 선검사하지 않는다.
std::uint16_t RemoteDeletionWord(std::span<const std::uint8_t> packet,std::size_t offset) {
    if (offset>packet.size() || packet.size()-offset<2) throw std::out_of_range("원격 삭제의 현재 WORD 물리 부족");
    return MutationWord(packet,offset);
}
// count 전체 DWORD와 부호 있는 반복 비교를 구별한다. callback 뒤에도 이 함수로 같은 필드를 재조회한다.
std::uint32_t RemoteDeletionCount(std::span<const std::uint8_t> packet) {
    if (packet.size()<8) throw std::out_of_range("원격 다중 삭제의 현재 count DWORD 물리 부족");
    return std::uint32_t(packet[4])|(std::uint32_t(packet[5])<<8)|(std::uint32_t(packet[6])<<16)|(std::uint32_t(packet[7])<<24);
}
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
// 53h의 범위는 SID1 이상이며 일반 할당의 첫 번호5나 dead/TYPE 검사를 수신 관문에 더하지 않는다.
void ReceiveRemoteDeletedSquid(const SidPool& pool,std::span<const std::uint8_t> packet,const std::function<void(Sid,std::uint32_t)>& destroy) {
    if (pool.Edition()!=OriginalEdition::Patch1078) throw std::invalid_argument("53h 원격 삭제의 PATCH 판본 불일치");
    const Sid sid{RemoteDeletionWord(packet,4)};
    if (!sid.value || sid.value>=pool.Capacity()) return;
    if (!(pool.Slot(sid)[11]&1U)) NeedMutation(destroy)(sid,0);
}
// 62h에는53h의 포인터 범위 조건이 없으며 앞 객체 효과 뒤 현재 packet과 슬롯을 그대로 사용한다.
void ReceiveRemoteDeletedSquids(const SidPool& pool,std::span<const std::uint8_t> packet,const std::function<void(Sid,std::uint32_t)>& destroy) {
    if (pool.Edition()!=OriginalEdition::Patch1078) throw std::invalid_argument("62h 원격 삭제의 PATCH 판본 불일치");
    auto count=std::bit_cast<std::int32_t>(RemoteDeletionCount(packet));
    if (count<=0) return;
    std::int32_t index=0; // 원문 EDI는0에서 시작하며 현재 count와 signed less로 비교한다.
    // 전체 payload 사본·전체 SID 선검사·중복 제거 없이 각 실제 호출 위치만 보호한다.
    do {
        const Sid sid{RemoteDeletionWord(packet,8U+static_cast<std::size_t>(index)*2U)};
        if (!(pool.Slot(sid)[11]&1U)) NeedMutation(destroy)(sid,0);
        count=std::bit_cast<std::int32_t>(RemoteDeletionCount(packet));
        ++index;
    } while (index<count);
}
// 원문 송신 관찰기는 -1만 진단하며 다른 음수 count를 정상화하거나 수신 전에 길이를 덧붙이지 않는다.
void ObserveRemoteDeletedSquids(std::span<std::uint8_t> packet,std::uint32_t sending,const std::function<void(std::string_view,std::string_view,std::uint32_t)>& assertion) {
    if (!sending) return;
    if (RemoteDeletionCount(packet)==0xffffffffU) NeedMutation(assertion)("count != (int)-1","\\Ns\\O\\_p_packets.cpp",0x9ac);
    const auto current=RemoteDeletionWord(packet,4);
    MutationPutWord(packet,2,static_cast<std::uint16_t>((static_cast<std::uint32_t>(current)+4U)*2U));
}
}
