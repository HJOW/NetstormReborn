// 원문6b 수신은 기존 선택Form을 교체한다. 기술 지급/희생 완료를 앞당기지 않는다.
#pragma once
#include "o/RawContainedCreate.h"
#include "o/RawContainedFinder.h"

namespace netstorm::o { //비미션 의식 선택의 디컴파일 기반 사전 구현이며 새 MP 검사는 없다.
void ReceiveRitualChoice(SidPool& pool,const ContainedFinderState& finder,const RawContainedCreate& create,
    std::span<const std::uint8_t> packet,const std::function<void(Sid,std::uint32_t)>& destroy); //00448e90의free관문→mask4삭제표식/가상삭제→동일부모Create(type,0,4,0,1)다.
}
