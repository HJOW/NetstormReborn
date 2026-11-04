// 공통 WORD setter의 정확한 저장 폭과 현재 다리 후속 통지를 공유한다.
#pragma once
#include "o/SidPool.h"
#include <functional>
#include <stdexcept>

namespace netstorm::o { // 원문004ac220/CD004abb00의 같은 SID 물리와 현재 다리 번호를 사용한다.
inline void SetRawSquidWord(SidPool& pool,Sid sid,std::uint16_t word,const std::uint32_t& bridgeType,const std::function<void(Sid)>& notify) { // WORD를 먼저 저장하며 선택된 통지 부재가 앞선 저장을 되돌리지 않는다. CD는 기존 할당 슬롯 계약을 유지한다.
    const auto physical=pool.Slot(sid);auto raw=pool.Edition()==OriginalEdition::Patch1078 ? std::span<std::uint8_t>(const_cast<std::uint8_t*>(physical.data()),physical.size()) : pool.AllocatedBytes(sid); // PATCH는 free/contained 여부로 실제 메모리 쓰기를 미리 거부하지 않는다.
    raw[12]=static_cast<std::uint8_t>(word);raw[13]=static_cast<std::uint8_t>(word>>8);
    if (raw[10]==bridgeType) { const auto selected=notify;if (!selected) throw std::logic_error("공통 WORD setter의 선택된 표면 통지 미연결");selected(sid); } // 현재 DWORD와 TYPE BYTE를 비교한 뒤 callable 수명을 지킨다.
}
}
