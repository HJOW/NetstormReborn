// 범용 정규식 대신 원문00504a00의 한 고정 패턴만 같은 캡처/부분 쓰기로 실행한다.
#include "client/RawChatFormat.h"
#include <algorithm>
#include <bit>
#include <optional>
#include <stdexcept>
#include <string>

namespace netstorm::client { // 원문 현재 전역과 외부 설정 장치의 호출 순서를 유지한다.
namespace { // 고정 패턴과 같은 물리의 NUL 조회만 사용하는 내부 부품이다.
// 실제로 선택한 장치를 빈 성공으로 바꾸지 않는다.
template<class F> const F& Need(const F& function) { if (!function) throw std::logic_error("Chat 서식 명령의 실제 장치 미연결");return function; }
// Regexp와 CRT가 사용하는 원문 C문자열의 첫 NUL 앞부분만 빌린다.
std::string_view Text(std::span<const std::uint8_t> bytes) {
    const auto end=std::find(bytes.begin(),bytes.end(),0);if (end==bytes.end()) throw std::out_of_range("Chat 서식의 실제 NUL 물리 없음");
    return {reinterpret_cast<const char*>(bytes.data()),static_cast<std::size_t>(end-bytes.begin())};
}
// 원문 부정 문자 클래스는 ~와 CR/TAB/LF/space만 제외하고 VT/FF·비ASCII는 허용한다.
bool StyleByte(unsigned char value) { return value!='~' && value!='\r' && value!='\t' && value!='\n' && value!=' '; }
// (~[^~CR TAB LF space]+)+의 첫 부분 검색과 greedy 연속 그룹0을 같은 원문 바이트에서 찾는다.
std::optional<std::string_view> Match(std::string_view text) {
    text=text.substr(0,text.find('\0'));
    for (std::size_t first=0;first<text.size();++first) { // 먼저 시작할 수 있는 ~부터 원문 부분 검색을 시도한다.
        if (text[first]!='~') continue;std::size_t cursor=first,end=first;
        while (cursor<text.size() && text[cursor]=='~' && cursor+1<text.size() && StyleByte(static_cast<unsigned char>(text[cursor+1]))) { // 한 글자 이상인 스타일 그룹만 연속 취한다.
            ++cursor;
            while (cursor<text.size() && StyleByte(static_cast<unsigned char>(text[cursor]))) ++cursor; // 해당 그룹의 부정 문자 클래스를 끝까지 읽는다.
            end=cursor;
        }
        if (end!=first) return text.substr(first,end-first);
    }
    return std::nullopt;
}
}
// 제거된 위치를 다시 검사하고 끝 NUL까지 앞으로 복사하며 뒤의 원래 바이트는 지우지 않는다.
void StripChatFormatSlots(std::span<std::uint8_t> text) {
    std::size_t cursor{};
    for (;;) { // 현재 NUL까지 매번 실제 바이트를 읽으며 같은 위치의 연속 ~0~6도 제거한다.
        if (cursor>=text.size()) throw std::out_of_range("Chat 슬롯 제거의 실제 물리 범위");if (!text[cursor]) return;
        if (text[cursor]=='~' && cursor+1<text.size() && text[cursor+1]>='0' && text[cursor+1]<='6') {
            std::size_t source=cursor+2,destination=cursor;
            for (;;) { // 원문은 한 바이트씩 NUL을 포함해 앞으로 옮기며 기존 꼬리를 보존한다.
                if (source>=text.size()) throw std::out_of_range("Chat 슬롯 제거 복사의 실제 NUL 없음");
                const auto value=text[source++];text[destination++]=value;if (!value) break;
            }
        } else ++cursor;
    }
}
// copy callback 뒤 강제 NUL은 앞서 고정한 manager에, Config 값은 같은 현재 캡처에 적용한다.
std::uint32_t ApplyChatFormat(bool front,ChatFormatGlobals globals,std::string_view input,std::span<std::uint8_t> capture,const ChatFormatHooks& hooks) {
    if (!globals.current) Need(hooks.assertion)(front ? 184U : 157U);
    const auto match=Match(input);if (!match) { Need(hooks.illegalFormat)(input.substr(0,input.find('\0')));return 0; }
    if (capture.size()<kChatFormatCaptureBytes) throw std::out_of_range("Chat 서식 캡처의 실제81BYTE 필요");
    const auto length=std::min<std::size_t>(match->size(),80);
    for (std::size_t index=0;index<length;++index) capture[index]=static_cast<std::uint8_t>((*match)[index]); // 실제 그룹0의 최대80BYTE만 순방향으로 쓰고 뒤는 보존한다.
    capture[length]=0;Need(hooks.stripSlots)(capture);
    const auto manager=globals.current;Need(hooks.copyFormat)(manager,front ? 0x50U : 0x14U,Text(capture),front ? 500U : 60U);
    const auto bytes=Need(hooks.memory)(manager);const auto terminal=front ? 0x243U : 0x4fU;if (terminal>=bytes.size()) throw std::out_of_range("Chat 서식 강제 NUL의 실제 manager 범위");bytes[terminal]=0;
    const auto key=std::to_string(std::bit_cast<std::int32_t>(globals.subscriber))+(front ? "chatFormat2" : "chatFormat");
    Need(hooks.writeConfig)(key,Text(capture));return 0;
}
}
