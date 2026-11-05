// 원문 영어0/1 구별과 대소문자 구별을 보존하고 선택되지 않은 장치를 요구하지 않는다.
#include "client/LanguageOptions.h"
#include <array>
#include <stdexcept>

namespace netstorm::client { // 실제 Client 전역과 독립 원문 관측이 같은 선택 본체를 사용한다.
namespace { // 보호 PE005435b8의 이름 순서다.
constexpr std::array<std::string_view,8> kLanguageNames{"english","english","french","german","spanish","japanese","portuguese","korean"}; //번호0/1은 별칭 합치기 없이 그대로 둔다. 6차: 마지막 korean(7)은 원본에 없는 클론 추가 언어이며 앞의 일곱은 원문 순서 그대로다.
}
// OS API가 반환한 LANGID의 낮은 WORD만 원문과 같은 분기로 해석한다.
std::uint32_t DetectOriginalLanguage(std::uint16_t langid) {
    switch (langid&0x3ffU) { case 7:return 3;case 10:return 4;case 12:return 2;case 17:return 5;case 22:return 6;default:return (langid>>10)==2 ? 1U : 0U; }
}
// 유효한 원문 일곱 인덱스만 읽으며 무효 주소를 영어로 대체하지 않는다.
std::string_view OriginalLanguageName(std::uint32_t number) { return kLanguageNames.at(number); }
// 원문 strcmp는 ASCII 대소문자도 구별하며 첫NUL 뒤는 보지 않는다.
std::uint32_t FindOriginalLanguage(std::string_view name) {
    name=name.substr(0,name.find('\0'));
    for (std::uint32_t index=0;index<kLanguageNames.size();++index) if (name==kLanguageNames[index]) return index; //첫 english는 항상0이다.
    return 0;
}
// 4dea40이 먼저 current를 쓰고 정규 이름을 읽은 뒤 호출자435220이 raw 이름을 다시 읽는다.
bool ApplyOriginalLanguageOption(std::uint32_t detected,std::int32_t& current,std::int32_t& published,const LanguageOptionHooks& hooks) {
    if (!hooks.read) throw std::logic_error("언어 옵션 currentLanguage 읽기 미연결");auto name=hooks.read();name.resize(name.find('\0')==std::string::npos ? name.size() : name.find('\0'));
    if (name.size()>=128) throw std::length_error("언어 옵션 원문128BYTE 지역 범위");auto selected=FindOriginalLanguage(name);
    if (detected!=selected && !selected) selected=detected==1 ? 1U : 0U;
    if (current==static_cast<std::int32_t>(selected)) return false;
    current=static_cast<std::int32_t>(selected); //첫 load가 실패해도 이미 쓴 원문 current는 되돌리지 않는다.
    if (!hooks.load) throw std::logic_error("언어 옵션 실제 용어표 읽기 미연결");hooks.load(OriginalLanguageName(selected));hooks.load(name);published=static_cast<std::int32_t>(selected);return true;
}
}
