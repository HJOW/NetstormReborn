// 원문은 sender/owner/TYPE 조건을 추가하지 않으며 부모의free bit1만 먼저 검사한다.
#include "o/RawRitualChoice.h"
#include <stdexcept>

namespace netstorm::o { //패치10.78의 실제 body만 정적으로 작성하며CD/네트워크 검증을 주장하지 않는다.
void ReceiveRitualChoice(SidPool& pool,const ContainedFinderState& state,const RawContainedCreate& create,
    std::span<const std::uint8_t> packet,const std::function<void(Sid,std::uint32_t)>& destroy) { //수신시점의실제물리와종속체인을사용한다.
    if (pool.Edition()!=OriginalEdition::Patch1078 || &create.Pool()!=&pool || packet.size()<10) throw std::invalid_argument("의식 선택 수신의 판본/동일Pool/10BYTE 계약");
    const Sid parent{static_cast<std::uint16_t>(packet[4]|(std::uint32_t(packet[5])<<8))};if (pool.Slot(parent)[11]&1U) return;
    RawContainedFinder finder(pool,state);
    for (auto current=finder.Begin(parent);current.value;current=finder.Next()) { //현재후보의다음링크를삭제전에저장하는실제finder다.
        if (!(pool.Slot(current)[20]&4U)) continue;pool.AllocatedBytes(current)[22]=1;if (!destroy) throw std::logic_error("의식 선택의 실제 가상 삭제 미연결");destroy(current,0);
    }
    const auto kind=std::uint32_t(packet[6])|(std::uint32_t(packet[7])<<8)|(std::uint32_t(packet[8])<<16)|(std::uint32_t(packet[9])<<24);static_cast<void>(create.Create(parent,kind,0,4,0,1)); //원문은삭제효과뒤현재packet+6을다시읽는다.
}
}
