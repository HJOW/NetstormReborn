// 보호 PE의 실제41e400/462290 입력과17개 가상6c 슬롯을 복원한다.
#include "o/RawObjectConversion.h"
#include <array>
#include <stdexcept>
#include <utility>

namespace netstorm::o { // 10.78의 현재 객체 입력만 다루며 CD는 별도 경로로 남긴다.
namespace { // 실제 슬롯 표와 선택된 외부 효과의 계약이다.
constexpr std::array<std::uint32_t,16> kSpellTables{0x502370,0x502448,0x502510,0x5025d8,0x5026a0,0x502778,0x502840,0x502908,0x5029d0,0x502aa8,0x502b70,0x502e68,0x502f30,0x502ff8,0x5030c0,0x503188}; // 원본6c가41e400인 생성자 표다.
template<class F> const F& Need(const F& hook) { if (!hook) throw std::logic_error("객체 전환의 실제 하위 미연결");return hook; } // 도달한 효과가 없으면 앞선 쓰기를 보존하고 중단한다.
}
// 모체·자산 이름이나 Bomb genus만으로 서로 다른 원문 가상 입력을 합치지 않는다.
std::uint32_t ObjectConversionBody(std::uint32_t vtable) {
    if (vtable==0x50a748) return 0x462290;
    for (const auto table:kSpellTables) if (table==vtable) return 0x41e400; // 현재 실제 표에 처음 일치하면 끝낸다.
    return 0;
}
// 생성 중 외부 효과나 원문 객체 변경은 실행하지 않는다.
RawObjectConversion::RawObjectConversion(const SidPool& pool,const std::uint32_t& buriedType,ObjectConversionHooks hooks)
    :pool_(pool),buriedType_(buriedType),hooks_(std::move(hooks)) {
    if (pool.Edition()!=OriginalEdition::Patch1078) throw std::invalid_argument("객체 전환은10.78만 복원했습니다");
}
// 0번/예약 슬롯도 원문 범위 안이면 읽고 free/dead 여부는 선택한 하위 처리에 맡긴다.
std::span<const std::uint8_t> RawObjectConversion::Raw(Sid sid) const { return pool_.Slot(sid); }
// 두 원문은 release를 별도 거부하지 않으며 미처리 명령은 같은 부모로 전달한다.
std::uint32_t RawObjectConversion::Command(Sid sid,std::uint32_t code,std::uint32_t body) const {
    if (body!=0x462290 && body!=0x41e400) throw std::logic_error("선택한 객체 전환의 실제 가상 입력 미복원");
    if ((code&0x03ffffffU)!=0x030000ceU) return Need(hooks_.base)(sid,code);
    Need(hooks_.unpop)(sid,0);
    if (body==0x462290) {
        const auto raw=Raw(sid);const auto type=std::uint32_t(raw[12])|(std::uint32_t(raw[13])<<8); // Unpop 뒤 현재 QA WORD다.
        Need(hooks_.retype)(sid,type);const auto frame=Need(hooks_.defaultFrame)(Raw(sid)[10]);Need(hooks_.frame)(sid,frame);
    } else {
        const auto previous=Raw(sid)[10]; // 원문은 Unpop 뒤 현재 TYPE를 보관해 마지막 QA에 쓴다.
        Need(hooks_.retype)(sid,buriedType_);Need(hooks_.frame)(sid,0);Need(hooks_.qa)(sid,previous);
    }
    Need(hooks_.repop)(sid,0);return 1;
}
}
