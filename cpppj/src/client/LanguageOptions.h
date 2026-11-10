// 원문 언어 이름/OS 감지와 Interpret Options의 선택 부분을 같은 현재 전역에 적용한다.
#pragma once
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>

namespace netstorm::client { // 기존 시나리오 경로/용어표와 공유할 언어 번호 규칙이다.
inline constexpr std::uint32_t kKoreanLanguageNumber = 7; // 6차 한국어: 원문 일곱 번호 뒤에 추가한 클론 전용 언어 번호이며 OS 언어 자동 선택은 하지 않고 옵션 메뉴/설정의 명시 선택으로만 쓴다.
inline constexpr std::uint32_t kJapaneseLanguageNumber = 5; // 11차 일본어: 원문 언어 이름표(english,english,french,german,spanish,japanese,...)의 일본어 번호다. 번역표/글꼴만 클론이 추가한다.
// 클론이 추가한 확장 문자(한글/일본어) 번역표와 외부 글꼴을 쓰는 언어인지 판정한다.
inline constexpr bool IsExtendedLanguageNumber(std::uint32_t number) { return number == kKoreanLanguageNumber || number == kJapaneseLanguageNumber; }
std::uint32_t DetectOriginalLanguage(std::uint16_t langid); //004de390: primary ID 다섯 개와 나머지 sublanguage2를 판정한다.
std::string_view OriginalLanguageName(std::uint32_t number); //004de410: 영어0/1을 포함한 원문 일곱 이름이다.
std::uint32_t FindOriginalLanguage(std::string_view name); //004de420: 정확한 C문자열 비교이며 미등록/빈 이름은0이다.
struct LanguageOptionHooks { // 원문128BYTE 읽기와 실제 용어표 재적재의 선택된 하위다.
    std::function<std::string()> read; //현재 currentLanguage이며 누락은 빈 문자열이다.
    std::function<void(std::string_view)> load; //같은 LanguageSpec/Registry에 읽으며 부재도 이전 표를 비운다.
};
bool ApplyOriginalLanguageOption(std::uint32_t detected,std::int32_t& current,std::int32_t& published,const LanguageOptionHooks& hooks); //435248..43529f/4dea40의 부분효과·두 번 읽기 순서다.
}
