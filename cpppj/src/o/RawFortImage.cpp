// 실제DataManager의고정배열/이전섹션길이수정/엄격한남은공간진단을보존한다.
#include "o/RawFortImage.h"
#include <algorithm>
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 선택된진단이누락되면빈성공으로계속하지않는다.
void Assert(const FortImageHooks& hooks,FortImageAssertion kind) {
    if (!hooks.assertion) throw std::logic_error("Fort 이미지의 실제 선택 진단 미연결");hooks.assertion(kind);
}
// 길이WORD는원본처럼자기2바이트를포함하고낮은바이트부터저장한다.
void Length(std::span<std::uint8_t> bytes,std::size_t offset,std::size_t length) {
    bytes[offset]=static_cast<std::uint8_t>(length);bytes[offset+1]=static_cast<std::uint8_t>(length>>8);
}
}
// 원본논리끝과실제배열의물리계약을함께확인한다.
std::span<const std::uint8_t> FortImageData::Bytes() const {
    if (end>storage.size()) throw std::out_of_range("Fort 이미지의 논리 끝이 물리 배열을 초과함");return std::span(storage).first(end);
}
// 미확정CD의비슷한몸체나누락된현재타입개수를추정하지않는다.
RawFortImage::RawFortImage(OriginalEdition edition,const FortTemplate& fort,FortImageHooks hooks):fort_(fort),hooks_(std::move(hooks)) {
    if (edition!=OriginalEdition::Patch1078 || !hooks_.typeCount || fort.sections.size()>FortTemplate::kSectionCount)
        throw std::invalid_argument("Fort 이미지의 PATCH 현재 Template/타입 개수 계약 오류");
}
// 기존해석된Money/Technology대신현재각raw섹션끝까지원문그대로쓴다.
FortImageData RawFortImage::Save() const {
    FortImageData result;result.storage.resize(kCapacity,0); //실제0044cc20의고정배열전체0초기화다.
    if (std::bit_cast<std::int32_t>(hooks_.typeCount())>=256) Assert(hooks_,FortImageAssertion::TypeCount); //원본004bcd8f의signed JL 관문이다.
    result.storage[0]=0x46;result.storage[1]=hooks_.signatureFlag ? hooks_.signatureFlag() : fort_.flag==0xfe ? std::uint8_t{0xfe} : std::uint8_t{};
    std::size_t position=2,previous=kCapacity; //이전길이필드는다음섹션진입직전과마지막반환직전에수정한다.
    // 고정35이름순서로없는tail섹션도실제빈stream의길이2를쓴다.
    for (std::size_t index=0;index<FortTemplate::kSectionCount;++index) {
        const auto section=hooks_.section ? hooks_.section(index) : index<fort_.sections.size() ? std::span<const std::uint8_t>(fort_.sections[index]) : std::span<const std::uint8_t>{}; //원본 검색/current는 이전 길이 수정에 앞서 수행한다.
        if (previous!=kCapacity) Length(result.storage,previous,position-previous);
        if (position>kCapacity-2) { Assert(hooks_,FortImageAssertion::StreamCapacity);throw std::out_of_range("Fort 이미지 길이 필드의 물리 범위 초과"); }
        previous=position;Length(result.storage,position,0);position+=2;
        if (section.size()>=kCapacity-position) Assert(hooks_,FortImageAssertion::SectionCapacity);
        if (section.size()>kCapacity-position) throw std::out_of_range("Fort 이미지 payload의 물리 범위 초과");
        std::copy(section.begin(),section.end(),result.storage.begin()+static_cast<std::ptrdiff_t>(position));position+=section.size();
    }
    Length(result.storage,previous,position-previous);result.end=position;return result;
}
}
