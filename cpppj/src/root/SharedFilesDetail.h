// 원문 iostream/C문자열 경계와 숫자 표 삽입을 관리 파일 부모가 공유한다.
#pragma once
#include <algorithm>
#include <bit>
#include <cstdint>
#include <istream>
#include <limits>
#include <locale>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace netstorm::root::shared_files_detail { // 실제 파일 스트림을 읽으며 검사 전용 자료/별도 parser 상태를 만들지 않는다.
inline std::string_view CString(std::string_view text) { return text.substr(0,text.find('\0')); } // 첫NUL 이후 바이트를 C문자열로 소비하지 않는다.
inline bool ReadInteger(std::istream& input,std::int32_t& value) {
    std::int64_t parsed{};if (!(input>>parsed)) return false;if (parsed<std::numeric_limits<std::int32_t>::min() || parsed>std::numeric_limits<std::int32_t>::max()) { input.setstate(std::ios::failbit);return false; }value=static_cast<std::int32_t>(parsed);return true;
} //0045d030/5d180처럼 실패/32비트 범위 밖이면 기존 출력 DWORD를 보존한다.
inline bool Insert(std::vector<std::uint32_t>& values,std::uint32_t value) {
    if (std::find(values.begin(),values.end(),value)!=values.end()) return false;try { values.push_back(value);return true; }catch (const std::bad_alloc&) { return false; }
} //004741f0의 순서/중복 제외·할당 실패 시 미삽입을 호스트 소유 저장소로 수행한다.
inline std::optional<std::string> ReadChars(std::istream& input,std::size_t capacity) {
    std::istream::sentry sentry(input);if (!sentry) return std::nullopt;std::string text;const auto& facet=std::use_facet<std::ctype<char>>(input.getloc());
    for (;;) { //00465620처럼 공백/EOF/NUL 앞에서 멈추고 NUL 자체는 소비하지 않는다.
        const auto next=input.peek();if (next==std::char_traits<char>::eof() || next==0 || facet.is(std::ctype_base::space,static_cast<char>(next))) break;if (text.size()+1>=capacity) throw std::out_of_range("R 관리 파일 char 지역 버퍼 범위");text+=static_cast<char>(input.get());
    }
    if (text.empty()) { input.setstate(std::ios::failbit);return std::nullopt; }return text;
} // 원문 무제한 char* 추출의 지역 버퍼 넘침은 호스트 물리 오류로 거부한다.
inline std::int32_t RequiredInteger(std::istream& input) { std::int32_t value{};if (!ReadInteger(input,value)) throw std::out_of_range("R 길드 파일의 미초기화 정수 입력 거부");return value; } // 원문 미초기화 지역을 가짜0으로 채우지 않는다.
inline std::string RequiredChars(std::istream& input,std::size_t capacity) { auto text=ReadChars(input,capacity);if (!text) throw std::out_of_range("R 길드 파일의 필수 문자열 입력 없음");return std::move(*text); } // 정상 행의 지정 물리/C문자열을 소유한다.
}
