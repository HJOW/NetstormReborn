// 원문 서버 템플릿의 텍스트 입력·부분 문자열과 현재 OS 시각/존 치환을 구현한다.
#include "root/CommandPages.h"
#include <algorithm>
#include <bit>
#include <ctime>
#include <limits>
#include <stdexcept>

namespace netstorm::root { // 원문 파일 내용을 만들거나 누락 템플릿을 임의 HTML로 대체하지 않는다.
namespace { // CharString의 첫NUL/정수 폭과 실제 검색 물리 범위를 공유한다.
std::string_view CString(std::string_view text) { return text.substr(0,text.find('\0')); } // CharString의 strlen/strcmp가 읽는 첫NUL 전 바이트만 빌린다.
std::uint32_t Length(std::string_view text) { if (text.size()>static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max())) throw std::out_of_range("R 명령 페이지의 signed DWORD 문자열 범위");return static_cast<std::uint32_t>(text.size()); } // 원문 strlen의 signed 검색 길이와 호스트 할당을 구분한다.
std::int32_t Find(std::string_view text,std::string_view tag,std::int32_t first) {
    Length(text);if (first<0 || static_cast<std::size_t>(first)>text.size()) throw std::out_of_range("R 명령 섹션 검색의 실제 문자열 범위");
    for (auto at=static_cast<std::size_t>(first);at<text.size();++at) if (text.substr(at).starts_with(tag)) return static_cast<std::int32_t>(at);return -1; //449ef0은 각 원문 BYTE를 탐색하며 끝NUL 위치에서는 일치 검사를 하지 않는다.
}
}
CommandPages::CommandPages(ConnectionState& state,SharedFileStorage& storage):state_(state),storage_(storage) {}
std::string CommandPages::Load() {
    std::string result;constexpr std::array<std::string_view,3> files{"Admin.server","Update.server","Guild.server"}; // 원문 순서이며 파일 사이에 LF/구분자를 추가하지 않는다.
    for (const auto name:files) { // 매 요청마다 현재 파일을 새로 열며 CCh별 캐시나 시작 시각 사본을 만들지 않는다.
        auto input=storage_.OpenWorking(name);std::array<char,33> buffer{};
        while (!input.eof() && !input.fail()) { input.get(buffer.data(),30,'\0');result+=buffer.data();Length(result); } //459fc0의 최대29BYTE/끝NUL·NUL delimiter 미소비와 EOF/fail 다음 반복 관문을 실제 텍스트 stream에 맡긴다.
        if (input.is_open()) input.close();
    }
    return result;
}
std::string CommandPages::Section(std::string_view source,std::string_view name) {
    source=CString(source);name=CString(name);const auto marker='['+std::string(name)+']';const auto first=std::bit_cast<std::int32_t>(std::bit_cast<std::uint32_t>(Find(source,marker,0))+Length(marker));const auto last=Find(source,"[END]",first); // 원문은 시작 marker가 없어도 -1에 marker 길이를 더한 뒤 END 검색을 먼저 수행한다.
    if (first<0) return std::string(source);const auto end=last<0 ? Length(source):static_cast<std::uint32_t>(last);if (end<static_cast<std::uint32_t>(first)) throw std::out_of_range("R 명령 섹션 부분 문자열의 실제 범위");return std::string(source.substr(static_cast<std::size_t>(first),end-static_cast<std::uint32_t>(first))); // END 부재면 원문처럼 현재 문자열 끝까지 자른다.
}
void CommandPages::Replace(std::string& page,std::string_view tag,std::string_view value,std::int32_t limit) {
    const std::string original(CString(page)),needle(CString(tag)),replacement(CString(value));const auto oldLength=Length(needle),newLength=Length(replacement);Length(original);if (!oldLength) throw std::invalid_argument("R 명령 페이지의 비어 있지 않은 치환 tag 필요");std::uint32_t ordinal{},completed{};
    for (std::size_t at=0;at<original.size();++at) { //449990/449ef0은 원문 사본의 겹치는 일치도 BYTE마다 세며 새로 삽입한 문자열을 재검색하지 않는다.
        if (!std::string_view(original).substr(at).starts_with(needle)) continue;const auto offset=static_cast<std::uint32_t>(at)-oldLength*ordinal+newLength*ordinal;++ordinal;const auto signedOffset=std::bit_cast<std::int32_t>(offset);const auto current=CString(page);const auto size=Length(current);
        if (signedOffset>=0 && offset<=size && offset+oldLength-1U<=size) page.erase(offset,std::min<std::size_t>(oldLength,page.size()-offset)); //44a470의 inclusive 마지막 문자/관문 복귀를 유지하며 실제 NUL은 호스트 string이 보관한다.
        if (signedOffset>=0 && offset<=CString(page).size()) { page.insert(offset,replacement);Length(page); } //44a8e0은 범위 밖 index이면 아무 쓰기도 하지 않는다.
        ++completed;if (limit!=-1 && limit<std::bit_cast<std::int32_t>(completed)) return; // 원문 limit0도 첫 교체 뒤 반환하며 -1만 무제한이다.
    }
}
void CommandPages::ReplaceInteger(std::string& page,std::string_view tag,std::int32_t value,std::int32_t limit) { Replace(page,tag,std::to_string(value),limit); }
std::string CommandPages::Time() {
    const auto now=::_time32(nullptr);std::tm local{};if (::_localtime32_s(&local,&now)) throw std::runtime_error("R 명령 페이지의 실제32비트 현지시각 변환 실패");std::array<char,26> text{};if (::asctime_s(text.data(),text.size(),&local)) throw std::runtime_error("R 명령 페이지의 실제 asctime 서식 실패");return std::string(text.data(),19)+": "; // 게임 FullTime이 아닌 원문 OS 달력 시각이며 LF/연도는 앞19BYTE 밖이다.
}
std::string CommandPages::Zones() {
    std::string result;
    for (std::size_t index=0;index<state_.challenges.size();++index) if (state_.challenges[index]) result+="<a herf=\"cmd:Tell,ZSEND(ADMIN,GETPLAYER,"+std::to_string(index)+"X1\">"+std::to_string(index)+"</a> "; // 원문의 herf 철자/괄호 없는 X1 링크와 현재0..50 순서를 유지하며 활성 참가자 수를 검사하지 않는다.
    return result;
}
void CommandPages::Expand(std::string& page) { Replace(page,"{TIME}",Time());Replace(page,"{ZONELIST}",Zones()); }
void CommandPages::Bind(ChallengeRuntimeHooks& challenge) { challenge.commandTemplates=[this] { return Load(); };challenge.expandCommandPage=[this](auto& page) { Expand(page); }; } // 실제 부모는 같은 owner 수명에서 현재 파일/존을 조회한다.
}
