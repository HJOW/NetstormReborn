// 표 끝의 미정235BYTE를 채우지 않으며 SNART 변환은 실제 선택할 때만 수행한다.
#include "o/RawTemplateSectionNames.h"
#include <array>
#include <stdexcept>
#include <string_view>
namespace netstorm::o { // 반환포인터는 Client전역과 같은 현재token이다.
namespace { // 보호PE의 기본문자열과 앞으로복사/append만 표현한다.
constexpr std::string_view kBaseNames="/Subscriber/State/Mission/CoreData/Chaff/Badges/Territory/TypeNames/CompressedData/Technology/Money/Deck/Reserved2/Reserved3/Reserved4/"; // 원문 기본135문자다.
template<class Function> const Function& Need(const Function& function) { if (!function) throw std::logic_error("Template 이름표의 선택된 실제 하위 미연결");return function; } // 기존표 반환에는 미사용하위를 요구하지 않는다.
void Byte(std::span<std::uint8_t> bytes,std::size_t at,std::uint8_t value) { if (at>=bytes.size()) throw std::out_of_range("Template 이름표 실제 쓰기범위");bytes[at]=value; } // 이미 쓴 앞BYTE는 물리 실패 때도 보존한다.
std::size_t End(std::span<const std::uint8_t> bytes) { // 같은 현재표의 최초NUL 위치를 매append마다 찾는다.
    for (std::size_t at=0;at<bytes.size();++at) if (!bytes[at]) return at;throw std::out_of_range("Template 이름표 현재NUL 누락"); // 현재실제범위만앞으로읽는다.
}
void Append(std::span<std::uint8_t> bytes,std::span<const std::uint8_t> text) { const auto length=End(text)+1,end=End(bytes);for (std::size_t at=0;at<length;++at) Byte(bytes,end+at,text[at]); } // 처음읽은NUL포함길이만현재끝에앞으로복사한다.
}
std::uint32_t GetTemplateSectionNames(std::uint32_t& current,const TemplateSectionNamesHooks& hooks) { // 실제004437f0은RET이므로 별도성공callback을만들지않는다.
    if (current) return current;current=Need(hooks.allocate)(kTemplateSectionNamesCapacity);auto bytes=Need(hooks.buffer)(current);
    if (!current) throw std::out_of_range("원문004bcced의NULL 이름표복사");
    // 원문0x22DWORD의136BYTE를같은순서로복사한다.할당나머지는읽거나쓰지않는다.
    for (std::size_t at=0;at<kBaseNames.size();++at) Byte(bytes,at,static_cast<std::uint8_t>(kBaseNames[at]));Byte(bytes,kBaseNames.size(),0);
    constexpr std::array<std::uint8_t,2> slash{'/',0}; // 원문DAT00507bd8의동일한2BYTE다.
    // 각Terr서식은ASCII숫자두자리이며실제표의현재NUL뒤로붙인다.
    for (std::uint32_t index=0;index<20;++index) { const auto name=Need(hooks.format)(index);Append(Need(hooks.buffer)(current),name);Append(Need(hooks.buffer)(current),slash); }
    Append(Need(hooks.buffer)(current),slash);return current;
}
}
