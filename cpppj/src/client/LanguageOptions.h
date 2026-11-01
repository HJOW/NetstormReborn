// 원문 언어 이름/OS 감지와 Interpret Options의 선택 부분을 같은 현재 전역에 적용한다.
#pragma once
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>

namespace netstorm::client { // 기존 시나리오 경로/용어표와 공유할 언어 번호 규칙이다.
std::uint32_t DetectOriginalLanguage(std::uint16_t langid); //004de390: primary ID 다섯 개와 나머지 sublanguage2를 판정한다.
std::string_view OriginalLanguageName(std::uint32_t number); //004de410: 영어0/1을 포함한 원문 일곱 이름이다.
std::uint32_t FindOriginalLanguage(std::string_view name); //004de420: 정확한 C문자열 비교이며 미등록/빈 이름은0이다.
struct LanguageOptionHooks { // 원문128BYTE 읽기와 실제 용어표 재적재의 선택된 하위다.
    std::function<std::string()> read; //현재 currentLanguage이며 누락은 빈 문자열이다.
    std::function<void(std::string_view)> load; //같은 LanguageSpec/Registry에 읽으며 부재도 이전 표를 비운다.
};
bool ApplyOriginalLanguageOption(std::uint32_t detected,std::int32_t& current,std::int32_t& published,const LanguageOptionHooks& hooks); //435248..43529f/4dea40의 부분효과·두 번 읽기 순서다.
}
