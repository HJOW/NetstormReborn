// checking에서만 선택되는 공통 Form 검사와 총의 순수 지면 소유자 진단이다.
#pragma once
#include "o/RawCheckingFlag.h"
#include "o/RawSquidNeighbors.h"
#include <bit>
#include <cmath>
#include <functional>
#include <stdexcept>

namespace netstorm::o { // 게임 동작/생성/공격을 실행하지 않는 같은 raw 자료의 진단 부모다.
inline bool CheckRawObjectForms(const SidPool& pool,Sid parent) { //004ad730: 자산 부모와 비자산 Form의 free/void만 검사하며 dead는 허용한다.
    const auto raw=pool.Slot(parent);if (raw[10]<=69 || (raw[11]&5U)) return false;
    std::uint32_t next=raw[6]|(std::uint32_t(raw[7])<<8);std::size_t guard{}; // 원문 head WORD와 호스트의 순환 한계다.
    while (next) { // 각 실제 next를 소비하며 이전 진단 결과를 캐시하지 않는다.
        if (next>=pool.Capacity() || ++guard>pool.Capacity()) throw std::logic_error("checkForms 실제 물리 범위 밖/순환 체인");
        const auto child=pool.Slot(Sid{static_cast<std::uint16_t>(next)});if ((child[11]&5U) || child[10]>69) return false;
        next=child[4]|(std::uint32_t(child[5])<<8);
    }
    return true;
}
namespace checking_detail { // 원문41d770/41d7a0/41def0의 float 저장 위치를 같은 진단에서만 사용한다.
inline std::int32_t Integer(double value) { // ftol signed64 불확정 하위0과 정상 DWORD 감김을 보존한다.
    if (!std::isfinite(value) || value < -0x1p63 || value>=0x1p63) return 0;
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::int64_t>(value)));
}
inline std::int32_t StoredInteger(float value,bool upper) { // 상한은 PE502310의 0.999float를 더하고 int→float 저장 뒤 다시 ftol한다.
    constexpr float kUpperBias=0.9999899864196777F; // 보호 PE502310의 원시 float이며 비슷한0.999 상수와 구별한다.
    const auto first=Integer(static_cast<double>(value)+(upper ? static_cast<double>(kUpperBias) : 0.0));
    return Integer(static_cast<float>(first));
}
}
inline void DiagnoseRawGunIslandOwners(const SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,
    Sid parent,SquidFootprint footprint,RawCheckingFlag checking,const std::uint32_t& noIslandType,
    const std::function<void(std::uint32_t)>& assertion) { //0046b830 case1의41def0(flags0)→현재 noIsland/owner 진단만 실행한다.
    if (!checking) return; // 원문46ba98의 바깥 관문은 finder/발자국 읽기 전 즉시 반환한다.
    RawSquidFinder finder(pool,hash,types); // 같은 일반4단계 finder를 쓰며 표면 지도/결과 사본으로 바꾸지 않는다.
    const SquidSearchArea area{checking_detail::StoredInteger(footprint.left,false),checking_detail::StoredInteger(footprint.top,false),
        checking_detail::StoredInteger(footprint.right,true),checking_detail::StoredInteger(footprint.bottom,true)};
    auto current=finder.Begin(area,0); // 원문 기본 필터는 모든 현재 후보를 허용한다.
    while (current.value) { // 다음은 finder가 보존한 링크를 따르며 진단 callback 뒤 전역/부모를 다시 읽는다.
        if (pool.Slot(current)[10]==noIslandType && checking && pool.Slot(current)[34]!=pool.Slot(parent)[34]) {
            if (!assertion) throw std::logic_error("Gunprocess.cpp 0xad noIsland 소유자 원본 진단 미연결");
            assertion(0xad);
        }
        current=finder.Next();
    }
}
}
