#include "o/RawFortPriestPreparation.h"
#include <bit>
#include <memory>
#include <stdexcept>
#include <utility>

namespace netstorm::o { // 원문 부모의 순서/지역 snapshot과 하위의 현재 값 재조회를 구분한다.
namespace { // 선택된 장치와 실제 little endian DWORD만 접근한다.
template<class F> const F& Need(const F& f) { if (!f) throw std::logic_error("Fort 사제 준비의 선택 장치 미연결");return f; } // 미선택 하위를 미리 요구하지 않는다.
std::uint32_t Word(std::span<const std::uint8_t> bytes,std::size_t at) { if (at+4>bytes.size()) throw std::out_of_range("Fort 사제 준비 DWORD 물리 범위");return std::uint32_t(bytes[at])|(std::uint32_t(bytes[at+1])<<8)|(std::uint32_t(bytes[at+2])<<16)|(std::uint32_t(bytes[at+3])<<24); } // 원문 float/목록 전체32비트다.
}
RawFortPriestPreparation::RawFortPriestPreparation(const SidPool& pool,const PriestPostPopState& priests,FortPriestPreparationGlobals globals,FortPriestPreparationHooks hooks):pool_(pool),priests_(priests),globals_(globals),hooks_(std::move(hooks)) { if (pool.Edition()!=OriginalEdition::Patch1078) throw std::invalid_argument("Fort 사제 준비의 PATCH10.78 계약"); }
std::span<const std::uint8_t> RawFortPriestPreparation::Raw(std::uint32_t sid) const { if (sid>65535 || sid>=pool_.Capacity()) throw std::out_of_range("Fort 사제 준비의 실제 DWORD SID 범위");return pool_.Slot(Sid{static_cast<std::uint16_t>(sid)}); }
void RawFortPriestPreparation::Collect(std::int32_t region,std::span<std::uint32_t> entries,std::uint32_t& count) const {
    for (std::uint32_t index=0;std::bit_cast<std::int32_t>(index)<std::bit_cast<std::int32_t>(priests_.Priests().count);++index) { // callback이 바꾼 목록 개수와 다음 항목도 원문처럼 매번 다시 읽는다.
        const auto& list=priests_.Priests();if (index>=list.entries.size()) throw std::out_of_range("Fort 사제 준비 현재 목록 물리 범위");const auto sid=list.entries[index];const auto raw=Raw(sid);
        if (Need(hooks_.region)(std::bit_cast<float>(Word(raw,14)),std::bit_cast<float>(Word(raw,18)))==region && count<entries.size()) entries[count++]=sid;
    }
}
std::uint32_t RawFortPriestPreparation::Marked(std::span<const std::uint32_t> entries) const { for (const auto sid:entries) if (Raw(sid)[12]&0x80U) return sid;return 0; } // 실제 순서의 첫 bit80을 반환한다.
void RawFortPriestPreparation::Run() const {
    if (!globals_.missionPost) {
        for (std::uint32_t index=0;index<4;++index) { // TYPE는 먼저, 현재 기술 부모는 그 다음에 읽으며 네 번째만 현재 subscriber%3을 사용한다.
            const auto type=index<3 ? Need(hooks_.baseType)(index) : Need(hooks_.themeType)(globals_.subscriber%3U);const auto parent=Need(hooks_.technology)();
            if (!Need(hooks_.contains)(parent,type)) Need(hooks_.createTechnology)(parent,type);
        }
    }
    if (globals_.editor || globals_.missionPost) return;
    if (Need(hooks_.templatePresent)(globals_.templateOwner)) { const auto state=Need(hooks_.state)(globals_.templateOwner);if (state.size()<18) throw std::out_of_range("Fort 사제 준비 State+17 물리 범위");if (state[17]) return; }
    std::array<std::uint8_t,120> records{};Need(hooks_.territory)(globals_.templateOwner,records);
    for (std::uint32_t index=0;index<20;++index) { // 사본 Territory의 각 record BYTE1 bit0만 판정하고 조회 owner는 그 시점의 현재값이다.
        if (!(records[index*6+1]&1U)) continue;const auto region=Need(hooks_.island)(globals_.playerOwner,index);if (region==127) continue;
        std::unique_ptr<std::uint32_t[]> entries(new std::uint32_t[20]);std::uint32_t count{};Collect(region,{entries.get(),20},count);auto selected=Marked({entries.get(),count});
        if (!selected && !count) Need(hooks_.populate)(region);
        else {
            if (!selected) { selected=entries[0];Need(hooks_.setOwner)(selected,globals_.playerOwner); }
            std::uint32_t remaining{};for (std::uint32_t at=0;at<count;++at) if (entries[at]!=selected) entries[remaining++]=entries[at]; //0040ea00처럼 모든 같은 번호를 안정적으로 제거한다.
            count=remaining;
        }
        const auto remaining=count; // 가상 owner가 원래 목록을 바꿔도 지역 임시 목록의 반복 수는 원문 지역값이다.
        for (std::uint32_t at=0;at<remaining;++at) Need(hooks_.setOwner)(entries[at],0); // 남은 사제마다 현재 실제 가상74를 실행한다.
        if (remaining) Need(hooks_.tellPriests)(remaining);
    }
}
}
