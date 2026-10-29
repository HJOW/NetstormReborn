// 실제 원문 고정 패턴의 바이트 문법만 구현하며 범용 정규식이나 네트워크 전송을 대체하지 않는다.
#include "client/RawDotCommandDispatch.h"
#include <algorithm>
#include <array>
#include <bit>
#include <optional>
#include <stdexcept>

namespace netstorm::client { // 원문 현재 명령표/검색 상태와 외부 콜백을 같은 호출 순서로 잇는다.
namespace { // 고정 Regexp 문법과 실제 물리 접근을 구현하는 내부 부품이다.
constexpr std::uint32_t kCallbacks=0x800,kCount=0xa04; // 이름표 시작에 대한 원문 함수표와 signed count의 오프셋이다.
// 현재 실제 DWORD 네 바이트를 정렬/alias 가정 없이 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::uint32_t at) {
    if (at>bytes.size() || bytes.size()-at<4) throw std::out_of_range("점 명령 분배 DWORD의 실제 물리 범위");
    std::uint32_t value{}; // 원문 little endian DWORD를 구성할 누적 값이다.
    for (std::uint32_t index=0;index<4;++index) value|=std::uint32_t(bytes[at+index])<<(index*8); // 네 실제 바이트만 순서대로 읽는다.
    return value;
}
// NUL을 포함한 원문 물리를 확인하고 그 앞의 바이트만 빌린다.
std::string_view Text(std::span<const std::uint8_t> bytes,std::size_t at) {
    if (at>=bytes.size()) throw std::out_of_range("점 명령 문자열의 시작 물리 범위");
    const auto tail=bytes.subspan(at);const auto end=std::find(tail.begin(),tail.end(),0); // 인접 슬롯도 같은 물리 안에서는 원문처럼 읽는다.
    if (end==tail.end()) throw std::out_of_range("점 명령 문자열의 실제 NUL 없음");
    return {reinterpret_cast<const char*>(tail.data()),static_cast<std::size_t>(end-tail.begin())};
}
// 원문 패턴에 직접 적힌 네 바이트만 공백으로 취급한다.
bool Space(unsigned char value) { return value==' ' || value=='\t' || value=='\r' || value=='\n'; }
// C locale stricmp의 동등 비교에 쓰이는 ASCII 소문자를 반환한다.
unsigned char Lower(unsigned char value) { return value>='A' && value<='Z' ? static_cast<unsigned char>(value+('a'-'A')) : value; }
// 원문 이름 비교는 locale C의 대소문자만 무시하며 비ASCII 바이트를 변환하지 않는다.
bool Equal(std::string_view first,std::string_view second) {
    if (first.size()!=second.size()) return false;
    for (std::size_t index=0;index<first.size();++index) if (Lower(static_cast<unsigned char>(first[index]))!=Lower(static_cast<unsigned char>(second[index]))) return false; // 끝까지 같은 바이트일 때만 같은 명령이다.
    return true;
}
// 원문 capture1/2와 그룹3 상대 위치를 나타내는 한 번의 고정 패턴 검색 결과다.
struct Match {
    std::string_view name; // capture1의80BYTE 상한을 적용한 명령 이름이다.
    std::string_view argument; // capture2의127BYTE 상한을 적용한 앞 공백 포함 인자다.
};
// 00506a30의 고정 패턴을 평가하고 실패를 포함해 공유 검색 상태를 갱신한다.
std::optional<Match> Search(std::string_view text,DotCommandDispatchState& state) {
    state.separatorOffset=-1;std::size_t cursor{}; // 실제 Regexp의 실패 플래그를 먼저 나타낸다.
    while (cursor<text.size() && Space(static_cast<unsigned char>(text[cursor]))) ++cursor; // 패턴 맨 앞의 네 공백만 소비한다.
    if (cursor==text.size() || text[cursor++]!='.') return std::nullopt;
    const auto start=cursor; // 점 다음부터 이름 캡처가 시작된다.
    while (cursor<text.size() && !Space(static_cast<unsigned char>(text[cursor])) && text[cursor]!=';' && text[cursor]!='$') ++cursor; // 이름 클래스는 점/콜론/VT/FF를 허용한다.
    if (cursor==start) return std::nullopt;
    const auto name=text.substr(start,std::min<std::size_t>(cursor-start,80));const auto argument=cursor; // 캡처 복사 상한은 원문 호출 인자다.
    while (cursor<text.size() && text[cursor]!=';' && text[cursor]!='$') ++cursor; // 두 번째 캡처는 개행을 포함하며 두 종료 문자 앞에서 멈춘다.
    if (cursor<text.size() && text[cursor]=='$') return std::nullopt;
    if (cursor>static_cast<std::size_t>(INT32_MAX)) throw std::out_of_range("점 명령 Regexp의 signed 상대 위치 범위");
    state.separatorOffset=static_cast<std::int32_t>(cursor);
    return Match{name,text.substr(argument,std::min<std::size_t>(cursor-argument,127))};
}
// 선택된 함수만 요구하며 반환 코드는 원문 비트 그대로 보존한다.
std::uint32_t Invoke(std::span<const std::uint8_t> table,std::uint32_t index,std::span<std::uint8_t> argument,const DotCommandDispatchHooks& hooks) {
    const auto callback=Read(table,kCallbacks+index*4U); // CPU DWORD 주소 계산과 같은 unsigned 감김 뒤 실제 범위를 검사한다.
    if (!hooks.invoke) throw std::logic_error("점 명령 실제 함수 장치 미연결");
    return hooks.invoke(callback,argument);
}
// 오류 이름은 명령 장치가 같은 표를 바꾼 뒤의 현재 물리에서 읽는다.
void Error(std::span<const std::uint8_t> table,std::uint32_t index,std::uint32_t result,const DotCommandDispatchHooks& hooks) {
    if (!result) return;
    const auto name=Text(table,index*32U); // callback 뒤 이름 변경/기본 슬롯 변경을 별도 사본으로 가리지 않는다.
    if (!hooks.error) throw std::logic_error("점 명령 오류 출력 장치 미연결");
    hooks.error(name,result);
}
}

// 각 명령 뒤 현재 공유 캡처와 현재 입력 바이트를 재조회한 뒤 나머지를 분배한다.
std::uint32_t DispatchDotCommand(std::span<std::uint8_t> table,DotCommandDispatchState& state,
    std::span<std::uint8_t> input,const DotCommandDispatchHooks& hooks) {
    std::size_t cursor{}; // 원문 param_1이 이동한 실제 입력 안의 위치다.
    if (Text(input,cursor).empty()) return 1;
    for (;;) { // 성공한 점 명령은 같은 남은 입력에서 다음 명령 검색으로 이어진다.
        const auto source=Text(input,cursor);const auto match=Search(source,state);
        bool found{};std::uint32_t index{}; // 원문 지역 iVar1과 현재 표 검색 결과다.
        if (match) {
            while (std::bit_cast<std::int32_t>(Read(table,kCount))>std::bit_cast<std::int32_t>(index)) { // 원문의 signed count를 현재 표에서 읽으며 첫 중복 이름이 선택된다.
                if (Equal(Text(table,index*32U),match->name)) { found=true;break; }
                ++index;
            }
        }
        if (!found) {
            const auto result=Invoke(table,state.fallbackIndex,input.subspan(cursor,source.size()+1),hooks);
            Error(table,state.fallbackIndex,result,hooks); // 기본 슬롯은 명령 호출 뒤 다시 읽는다.
            return 1;
        }
        std::array<std::uint8_t,128> argument; // 원문 지역128BYTE에서 실제 캡처와 NUL이 쓴 범위만 장치에 제공한다.
        std::copy(match->argument.begin(),match->argument.end(),argument.begin());argument[match->argument.size()]=0;
        const auto result=Invoke(table,index,std::span(argument).first(match->argument.size()+1),hooks);
        Error(table,index,result,hooks);
        const auto next=static_cast<std::int64_t>(cursor)+state.separatorOffset; // 중첩 callback이 바꾼 공유 Regexp 상대 위치도 그대로 사용한다.
        if (next<0 || static_cast<std::uint64_t>(next)>=input.size()) throw std::out_of_range("점 명령 다음 구분자의 실제 입력 물리 범위");
        cursor=static_cast<std::size_t>(next);
        if (input[cursor]==';') ++cursor;
        if (Text(input,cursor).empty()) return 1;
    }
}
}
