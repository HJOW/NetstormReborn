// PATCH0043ca70의 같은 소유자 색 표와 모든 섬 받침/종유석의 프레임 변경을 복원한다.
#pragma once
#include "o/RawBridgeConnect.h"
#include <bit>
#include <functional>
#include <stdexcept>

namespace netstorm::o {
// 현재 표를 먼저 쓰고 원본 iterator처럼 SID2부터 free만 건너뛴다. 프레임 효과 뒤 현재 슬롯/표를 다시 읽는다.
inline void ChangePlayerColor(SidPool& pool,BridgeConnectState& state,std::uint32_t owner,std::uint32_t color,
    const std::function<void(Sid,std::int32_t,std::uint32_t)>& setFrame) {
    if (pool.Edition()!=OriginalEdition::Patch1078 || owner>=state.ownerColors.size()) throw std::out_of_range("PATCH 소유자 색 변경 입력 범위 오류");
    const auto limit=pool.Capacity(); // 원본 iterator+4는 진입 당시 풀 길이를 보존한다.
    state.ownerColors[owner]=std::bit_cast<std::int32_t>(color);
    std::uint32_t index=2; // iterator의 초기1 뒤0040eb40이 먼저 증가하므로0/1번은 방문하지 않는다.
    while (index<limit && (pool.Slot(Sid{static_cast<std::uint16_t>(index)})[11]&1U)) ++index; // 최초 살아 있는 슬롯까지 free만 건너뛴다.
    while (index>=1 && index<pool.Capacity()) { // 원본 본문은 현재 풀의 상한을 다시 읽는다.
        const Sid sid{static_cast<std::uint16_t>(index)};const auto raw=pool.Slot(sid); // 소유자가 다른 섬도 현재 색 표로 함께 갱신한다.
        if (raw[10]==state.islandType || raw[10]==state.stalagType) {
            if (!setFrame) throw std::logic_error("실제 소유자 색 변경의 프레임 소비자 누락");
            setFrame(sid,OwnerColorFrame(state,raw[34]),0);
        }
        ++index;
        while (index<limit && (pool.Slot(Sid{static_cast<std::uint16_t>(index)})[11]&1U)) ++index; // dead/void/abstract는 제외하지 않는다.
    }
}
}
