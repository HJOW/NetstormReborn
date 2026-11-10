// 기존 표시 인코딩을 재사용하며 문자를 네 개의 WM_CHAR로 분해하지 않는다.
#include "client/DisplayTextInput.h"
#include "o/OriginalText.h"
#include <algorithm>
#include <stdexcept>

namespace netstorm::client { // 플랫폼/IME/MCP와 TextEntry가 같은 변환 규칙을 공유한다.
namespace { // 외부에 노출하지 않는 표시 BYTE 조회와 UTF-8 부호화다.
// 문자 하나의 물리 BYTE 폭이다. 정확한 네 BYTE만 원자 문자로 취급한다.
std::size_t Width(std::string_view text,std::size_t at) {
    const std::span<const std::uint8_t> bytes{reinterpret_cast<const std::uint8_t*>(text.data()),text.size()};
    return o::DecodeExtendedSequence(bytes,at) ? o::kExtendedTextLength : 1U;
}
// 검증된 scalar를 UTF-8 뒤에 기록한다. surrogate/Unicode 범위 밖을 거부한다.
void AppendUtf8(std::string& out,std::uint32_t cp) {
    if (cp>0x10ffffU || (cp>=0xd800U && cp<=0xdfffU)) throw std::invalid_argument("확정 입력 Unicode 범위");
    if (cp<0x80U) out.push_back(static_cast<char>(cp));
    else if (cp<0x800U) { out.push_back(static_cast<char>(0xc0U|(cp>>6)));out.push_back(static_cast<char>(0x80U|(cp&63U))); }
    else if (cp<0x10000U) { out.push_back(static_cast<char>(0xe0U|(cp>>12)));out.push_back(static_cast<char>(0x80U|((cp>>6)&63U)));out.push_back(static_cast<char>(0x80U|(cp&63U))); }
    else { out.push_back(static_cast<char>(0xf0U|(cp>>18)));out.push_back(static_cast<char>(0x80U|((cp>>12)&63U)));out.push_back(static_cast<char>(0x80U|((cp>>6)&63U)));out.push_back(static_cast<char>(0x80U|(cp&63U))); }
}
}
bool IsCommittedTextInput(std::uint32_t event) { return (event&0x03e00000U)==kCommittedTextInput && (event&0x001fffffU)<=0x10ffffU; }
std::string CommittedTextBytes(std::uint32_t event) {
    if (!IsCommittedTextInput(event)) throw std::invalid_argument("확정 입력 태그 범위");
    const auto encoded=o::EncodeExtendedCodePoint(event&0x001fffffU);if (!encoded) throw std::invalid_argument("표시 인코딩 미지원 확정 문자");
    return {encoded->begin(),encoded->end()};
}
std::vector<InputEvent> MakeCommittedTextEvents(std::string_view utf8,std::int32_t x,std::int32_t y) {
    const auto bytes=o::EncodeDisplayText(utf8);std::vector<InputEvent> result;result.reserve(bytes.size());
    for (std::size_t at=0;at<bytes.size();) { // 전체 검증 뒤에만 사건 목록을 만들므로 실패가 일부 입력을 남기지 않는다.
        if (const auto cp=o::DecodeExtendedSequence(bytes,at)) { result.push_back({kCommittedTextInput|*cp,x,y});at+=o::kExtendedTextLength; }
        else { const auto byte=bytes[at++];if (byte<0x20U || byte==0x7fU) throw std::invalid_argument("text 입력의 제어문자는 key 동작으로 전달해야 함");result.push_back({byte,x,y}); }
    }
    return result;
}
std::string CommittedUtf16ToUtf8(std::u16string_view text) {
    std::string result;
    for (std::size_t at=0;at<text.size();++at) { // UTF-16 surrogate 쌍을 하나의 scalar로 검증한다.
        std::uint32_t cp=text[at];
        if (cp>=0xd800U && cp<=0xdbffU) { if (++at==text.size() || text[at]<0xdc00U || text[at]>0xdfffU) throw std::invalid_argument("잘린 UTF-16 확정 입력");cp=0x10000U+((cp-0xd800U)<<10)+(text[at]-0xdc00U); }
        AppendUtf8(result,cp);
    }
    return result;
}
std::u16string DisplayTextToUtf16(std::span<const char> text) {
    std::u16string result;const std::span<const std::uint8_t> bytes{reinterpret_cast<const std::uint8_t*>(text.data()),text.size()};
    for (std::size_t at=0;at<bytes.size();) { // 기존 1252/확장 부호화는 BMP만 표현하므로 UTF-16 한 칸이다.
        if (const auto cp=o::DecodeExtendedSequence(bytes,at)) { result.push_back(static_cast<char16_t>(*cp));at+=o::kExtendedTextLength; }
        else { const auto utf8=o::DecodeOriginalText(bytes.subspan(at++,1));const auto lead=static_cast<std::uint8_t>(utf8[0]);std::uint32_t scalar=lead; // if 초기화의 cp 범위가 else까지 이어지므로 다른 이름을 쓴다(K1 컴파일 교정).
            if (utf8.size()==2) scalar=((lead&31U)<<6)|(static_cast<std::uint8_t>(utf8[1])&63U);
            else if (utf8.size()==3) scalar=((lead&15U)<<12)|((static_cast<std::uint8_t>(utf8[1])&63U)<<6)|(static_cast<std::uint8_t>(utf8[2])&63U);
            result.push_back(static_cast<char16_t>(scalar)); }
    }
    return result;
}
bool HasExtendedDisplayText(std::string_view text) {
    for (std::size_t at=0;at<text.size();++at) if (Width(text,at)>1) return true; // 원문 일반 BYTE만 있으면 편집 동작을 그대로 보존한다.
    return false;
}
std::size_t DisplayTextFloor(std::string_view text,std::size_t position) {
    position=std::min(position,text.size());std::size_t at{};
    while (at<position) { const auto next=at+Width(text,at);if (next>position) return at;at=next; } // 문자의 일부를 선택 시작으로 만들지 않는다.
    return at;
}
std::size_t DisplayTextCeil(std::string_view text,std::size_t position) {
    position=std::min(position,text.size());const auto floor=DisplayTextFloor(text,position);return floor==position ? floor : floor+Width(text,floor);
}
std::size_t PreviousDisplayText(std::string_view text,std::size_t position) { const auto at=DisplayTextFloor(text,position);return at ? DisplayTextFloor(text,at-1) : 0U; }
std::size_t NextDisplayText(std::string_view text,std::size_t position) { const auto at=DisplayTextFloor(text,position);return at<text.size() ? at+Width(text,at) : text.size(); }
std::string_view DisplayTextLine(std::string_view text,std::int32_t line) {
    if (line<0) return {};
    for (std::int32_t at=0;at<line;++at) { const auto end=text.find('\n');if (end==text.npos) return {};text.remove_prefix(end+1); } // LF 줄번호는 원문과 같은 BYTE 본문에서 읽는다.
    return text.substr(0,text.find('\n'));
}
std::string FilterDisplayText(std::string_view text,std::string_view excluded,std::size_t maximum) {
    std::string result;const auto nul=text.find('\0');text=text.substr(0,nul);
    for (std::size_t at=0;at<text.size();) { // 확장 숫자의 0x08/공백 판정을 금지하고 완전한 문자만 복사한다.
        const auto count=Width(text,at);if (excluded.find(text[at])!=excluded.npos) { at+=count;continue; } // 숫자 전용처럼 lead가 제외되면 확장 문자 전체를 거부한다.
        if (count>maximum-result.size()) break;result.append(text.substr(at,count));at+=count;
    }
    return result;
}
}
