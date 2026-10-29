// 토큰 순서와 Identity 우선·빈 채널 키·번역 뒤 현재 이름 재조회를 원문대로 유지한다.
#include "client/ChatRecipients.h"
#include <algorithm>
#include <array>
#include <stdexcept>

namespace netstorm::client { // 원문 바이트를 UTF-8/유니코드 공백으로 바꾸지 않는 검사다.
namespace { // 고정 표현식과 C locale 비교의 작은 부품만 공유한다.
constexpr std::string_view kSeparators="\r\n\t "; //005049e8의 정확한 네 구분자다. VT/FF/NBSP는 이름의 일부다.
constexpr std::array<std::string_view,8> kChannels{"","","","BATTLE","","EVERYONE","ALLIES","OBSERVER"}; //00531768의 첫NULL 앞 여덟 포인터이며 빈 문자열도 건너뛰지 않는다.
// 하위가 실제 선택되기 전에는 장치 누락을 검사하지 않는다.
template<class Function> const Function& Need(const Function& function) { if (!function) throw std::logic_error("채팅 대상 이름 검사의 선택 장치 미연결");return function; }
// C 문자열 첫NUL 뒤 바이트는 이름/번역 비교에 포함하지 않는다.
std::string_view Z(std::string_view text) { return text.substr(0,text.find('\0')); }
// 이름40BYTE 복사 뒤 NUL은 인접 주소 필드의 첫BYTE일 수도 있다. 원문 지역 전체 범위에서만 찾는다.
std::string_view Name(const o::PlayerIdentityRecord& record) {
    const auto begin=reinterpret_cast<const char*>(record.data()+8),end=reinterpret_cast<const char*>(record.data()+record.size());const auto nul=std::find(begin,end,0);
    if (nul==end) throw std::out_of_range("채팅 대상 지역 이름의 NUL 물리 끝");return {begin,static_cast<std::size_t>(nul-begin)};
}
//004841b0은 같은 지역의 선택 필드만 지우며 앞선 Resolve가 바꾼 mode/미작성 이름 꼬리를 보존한다.
void Reset(o::PlayerIdentityRecord& record) { std::fill_n(record.begin(),8,std::uint8_t{});record[8]=0;std::fill_n(record.begin()+48,4,std::uint8_t{}); }
// CRT stricmp의 C locale처럼 ASCII 알파벳만 접고 확장 바이트는 그대로 비교한다.
bool Equal(std::string_view first,std::string_view second) {
    first=Z(first);second=Z(second);if (first.size()!=second.size()) return false;
    const auto fold=[](unsigned char byte) { return byte>='A' && byte<='Z' ? byte+('a'-'A') : byte; }; // 원문 좁은 문자열의 한BYTE 비교다.
    for (std::size_t index=0;index<first.size();++index) { // 같은 길이의 각 바이트를 앞에서부터 비교한다.
        if (fold(static_cast<unsigned char>(first[index]))!=fold(static_cast<unsigned char>(second[index]))) return false;
    }return true;
}
}
// 고정 ^[CR LF TAB SPACE]*([^CR LF TAB SPACE]+)(.*)를 반복하며 전체 토큰의 다음 시작점은 절단 전 끝이다.
bool CheckChatRecipients(std::uint32_t manager,std::optional<std::string_view> text,o::PlayerIdentityRecord& scratch,const ChatRecipientHooks& hooks) {
    if (!manager || !text) return false;o::RawPlayerIdentity::Initialize(scratch,0);auto remaining=Z(*text);
    for (;;) { // 공백뿐인 나머지도 원문 regex 검색 실패처럼true이며 하위 조회를 하지 않는다.
        const auto begin=remaining.find_first_not_of(kSeparators);if (begin==remaining.npos) return true;
        remaining.remove_prefix(begin);const auto end=remaining.find_first_of(kSeparators);const auto length=end==remaining.npos ? remaining.size() : end;
        Reset(scratch);
        const auto copied=std::min<std::size_t>(length,40);std::copy_n(remaining.begin(),copied,scratch.begin()+8);scratch[8+copied]=0;
        if (!Need(hooks.resolve)(scratch)) {
            bool found=false;
            for (const auto key:kChannels) { // 원문은 Identity 실패 때만 여덟 포인터를 순서대로 번역/비교한다.
                const auto translated=Need(hooks.translate)(key);if (Equal(Name(scratch),translated)) { found=true;break; }
            }
            if (!found) return false;
        }
        remaining.remove_prefix(length); //40BYTE로 절단한 이름과 별개로 실제 캡처2 시작점까지 전진한다.
    }
}
}
