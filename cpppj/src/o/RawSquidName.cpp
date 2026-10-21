// 현재 언어 읽기·설명 치환·Factory 접미사 순서와 원본 버퍼 경계를 보존한다.
#include "o/RawSquidName.h"
#include <array>
#include <cstdio>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
#include "o/SquidNameVtables.inc"
constexpr std::size_t kNameBytes=80,kSuffixBytes=256; //0049af80의지역버퍼와00454d30의접미사버퍼크기다.
// 실제 선택된 이름 하위 누락을 빈 문자열로 대신하지 않는다.
template<class Function> const Function& Need(const Function& function) { if (!function) throw std::logic_error("객체 이름 실제 하위 미연결");return function; }
// 원본 C 문자열의 첫NUL과 주어진 버퍼 경계를 확인한다.
std::string Text(std::string value,std::size_t capacity) { const auto end=value.find('\0');if (end!=std::string::npos) value.resize(end);if (value.size()>=capacity) throw std::length_error("객체 이름 원본 버퍼 범위");return value; }
}
// 미확인 가상 표에 base 이름을 임의로 부여하지 않는다.
std::uint32_t SquidNameEntry(std::uint32_t table) { for (const auto& entry:kSquidNameEntries) { if (entry[0]==table) return entry[1]; }throw std::logic_error("객체 이름 가상 표 미복원"); } //반복문은같은83개실제표만조사한다.
// 원본 표의 첫0/I부터9/X와 범위 뒤ff 바이트를 유지한다.
std::string SquidLevelName(std::uint32_t level) { constexpr std::array<std::string_view,10> names{"I","II","III","IV","V","VI","VII","VIII","IX","X"};return level<names.size() ? std::string(names[level]) : std::string(1,static_cast<char>(0xff)); } //names는PE00542420의실제순서다.
// 생성 시 이름/장치 조회를 실행하지 않는다.
RawSquidName::RawSquidName(const SidPool& pool,std::span<const RiftTypeRecord> types,SquidNameHooks hooks)
    :pool_(pool),types_(types),hooks_(std::move(hooks)) { if (pool.Edition()!=OriginalEdition::Patch1078 || types.size()!=188) throw std::invalid_argument("객체 이름 같은PATCH/188TYPE 누락"); }
// TYPE 포인터는 언어 조회 전에 선택하고 설명 바이트는 그 효과 뒤에 읽는다.
std::string RawSquidName::Base(Sid parent) const {
    const auto type=pool_.Slot(parent)[10];if (type>=types_.size()) throw std::out_of_range("객체 이름 TYPE 범위");const auto first=Need(hooks_.language)();const bool translated=first && Need(hooks_.language)()!=1;
    auto description=types_[type].description;const auto end=description.find('\0');if (end!=std::string::npos) description.resize(end);if (translated) description=Need(hooks_.translate)(description);description=Text(std::move(description),kNameBytes);return Text(Need(hooks_.substitute)(description),kNameBytes);
}
// Factory는 base 이름 뒤 현재 번역 패턴→현재 가상레벨→로마숫자→접미사 순서다.
std::string RawSquidName::Name(Sid parent) const {
    const auto raw=pool_.Slot(parent);std::uint32_t table{};
    // 비정렬 가상 표 DWORD를 낮은 BYTE부터 읽는다.
    for (std::size_t index=0;index<4;++index) table|=static_cast<std::uint32_t>(raw[index])<<(index*8);
    const auto entry=SquidNameEntry(table);auto name=Base(parent);if (entry==0x4ace20U) return name;
    const auto pattern=Need(hooks_.translate)(" Level %s");const auto level=SquidLevelName(Need(hooks_.level)(parent));std::array<char,kSuffixBytes> suffix{};const auto count=std::snprintf(suffix.data(),suffix.size(),pattern.c_str(),level.c_str());if (count<0 || static_cast<std::size_t>(count)>=suffix.size()) throw std::length_error("Factory 이름 접미사 원본 버퍼 범위");name+=suffix.data();return Text(std::move(name),kNameBytes);
}
// 같은 현재 월드 연결을 확인한다.
const SidPool& RawSquidName::Pool() const { return pool_; }
}
