// 원본 목록의 캐시 재사용과 행 형식을 실제 현재 저장소에서 계산한다.
#include "o/RawSoloMapList.h"
#include <bit>
#include <cstdio>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 선택된 하위 경계만 요구한다.
template<class Function> const Function& Need(const Function& function) {
    if (!function) throw std::logic_error("단독 맵 목록의 실제 하위 미연결");return function;
}
// CRT 기본 로케일의 ASCII 대소문자 비교와 첫 NUL 경계를 사용한다.
bool Same(std::string_view left,std::string_view right) {
    left=left.substr(0,left.find('\0'));right=right.substr(0,right.find('\0'));if (left.size()!=right.size()) return false;
    for (std::size_t index=0;index<left.size();++index) { // 원본 파일/구독자 이름의 같은 ASCII BYTE만 비교한다.
        auto a=static_cast<unsigned char>(left[index]),b=static_cast<unsigned char>(right[index]);if (a>='A' && a<='Z') a+=32;if (b>='A' && b<='Z') b+=32;if (a!=b) return false;
    }return true;
}
// 원본256BYTE 행 버퍼의 실제 형식과 길이 경계를 보존한다.
std::string Format(std::string pattern,const std::string& name) {
    std::array<char,256> buffer{};const auto count=std::snprintf(buffer.data(),buffer.size(),pattern.c_str(),name.c_str());
    if (count<0 || static_cast<std::size_t>(count)>=buffer.size()) throw std::length_error("단독 맵 목록 행 버퍼 범위");return {buffer.data(),static_cast<std::size_t>(count)};
}
}
// 외부 효과는 Refresh/Row의 실제 분기에서만 선택한다.
RawSoloMapList::RawSoloMapList(SoloMapListState& state,SoloMapListHooks hooks):state_(state),hooks_(std::move(hooks)) {}
// signed 개수를 다른 정상 인덱스로 자르지 않는다.
std::size_t RawSoloMapList::Index(std::int32_t value) const {
    if (value<0 || value>=static_cast<std::int32_t>(state_.names.size())) throw std::out_of_range("단독 맵 목록 물리 인덱스 범위");return static_cast<std::size_t>(value);
}
// null 포인터와 빈 C문자열을 구분한다.
const std::string& RawSoloMapList::Name(std::int32_t value) const {
    const auto& name=state_.names[Index(value)];if (!name) throw std::logic_error("단독 맵 목록의 현재 이름 포인터 누락");return *name;
}
// dirty 정리는 이름이 존재하는 칸의 flags만 지운다. 정상 캐시는 mode가 달라도 원본처럼 재사용한다.
std::int32_t RawSoloMapList::Refresh(std::int32_t mode) const {
    if (state_.dirty) {
        for (std::size_t index=0;index<state_.names.size();++index) { // 원본50개 포인터 순서로 소유 문자열과 같은 flags를 정리한다.
            if (state_.names[index]) { state_.names[index].reset();state_.flags[index]=0; }
        }state_.count=0;state_.dirty=0;
    }
    const auto mayEdit=Need(hooks_.mayEditMissions)();if (state_.names[0]) return state_.count;
    const auto files=Need(hooks_.enumerate)();
    for (const auto& file:files) { // 원본 열거 순서대로 검사하며 수락한 이름만50개까지 담는다.
        if (state_.count>=50) break;const auto kind=Need(hooks_.kind)(file);const auto selected=state_.count;
        if (kind!=mode && !(mayEdit && mode==2 && kind==1)) continue;
        const auto slot=Index(selected);state_.names[slot]=file.substr(0,file.find('\0'));Need(hooks_.construct)();Need(hooks_.load)(Name(state_.count));
        if (!Need(hooks_.present)()) state_.flags[slot]|=1U;
        else {
            const auto subscriber=Need(hooks_.subscriber)();if (Need(hooks_.stateByte)()&1U) state_.flags[slot]|=2U;
            if (!Same(subscriber,Need(hooks_.userName)())) state_.flags[slot]|=4U;
            if (Need(hooks_.reserved)()) state_.flags[slot]|=8U;
        }
        state_.count=std::bit_cast<std::int32_t>(std::bit_cast<std::uint32_t>(state_.count)+1U); //원본DWORD증가의signed감김을보존한다.
    }
    return state_.count;
}
// 정상/읽기 실패 형식은 번역하지 않으며 손상 패턴만 현재 번역을 조회한다.
std::optional<SoloMapRow> RawSoloMapList::Row(std::int32_t index,std::string_view current,std::string_view fallback) const {
    if (index>=state_.count) return std::nullopt;const auto& name=Name(index);current=current.substr(0,current.find('\0'));const bool checked=(current.empty() && Same(name,fallback)) || Same(name,current);
    const auto flags=state_.flags[Index(index)];const auto pattern=(flags&1U) ? std::string("~r%s") : (flags&2U) ? Need(hooks_.translate)("%s (~rCORRUPT~.)") : std::string("%s");
    const auto label=Format(pattern,Name(index));return SoloMapRow{label,Name(index),checked,(flags&8U)==0};
}
}
