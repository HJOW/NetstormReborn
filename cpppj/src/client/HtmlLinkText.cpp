// 원본은 링크 대상의 대소문자·공백·프로토콜을 정규화하지 않고 인용 안 바이트를 보존한다.
#include "client/HtmlLinkText.h"
#include <algorithm>
#include <limits>
#include <stdexcept>
namespace netstorm::client {
// 실제0046bfc0의 인용 검색/현재 포인터 서식/NUL 추가 순서다.
std::string HtmlLinkText(HtmlLinkTextState& links,std::string_view raw,bool closing) {
    if (!links.enabled) return {};std::uint32_t token{};raw=raw.substr(0,raw.find('\0'));
    const auto quote=raw.find('"'),end=raw.find('>');
    if (!closing && quote!=raw.npos && (end==raw.npos || quote<end)) {
        auto first=quote+1;if (first<raw.size() && raw[first]=='#') ++first;auto last=first;
        // 첫 닫힘 인용/꺾쇠/NUL 앞까지만 복사한다. entity/공백을 바꾸지 않는다.
        while (last<raw.size() && raw[last]!='"' && raw[last]!='>') ++last;
        if (!links.base || links.cursor>links.bytes.size() || links.cursor+last-first+1>links.capacity ||
            static_cast<std::uint64_t>(links.base)+links.cursor+last-first>=std::numeric_limits<std::uint32_t>::max())
            throw std::length_error("HTML 링크 실제 저장소/토큰 범위 초과");
        token=links.base+static_cast<std::uint32_t>(links.cursor);const auto required=links.cursor+last-first+1;if (links.bytes.size()<required) links.bytes.resize(required);
        std::copy(raw.begin()+first,raw.begin()+last,links.bytes.begin()+links.cursor);links.cursor=required;links.bytes[required-1]=0;
    }
    return "~[T"+std::to_string(token)+']';
}
// 읽은 view는 다음 링크 추가/저장소 해제 전에만 유효하다.
std::string_view ResolveHtmlLink(const HtmlLinkTextState& links,std::uint32_t token) {
    if (!links.enabled || token<links.base || token-links.base>=links.bytes.size()) throw std::out_of_range("현재 HTML 링크 토큰 누락");
    const auto first=links.bytes.begin()+(token-links.base),last=std::find(first,links.bytes.end(),0);
    if (last==links.bytes.end()) throw std::length_error("HTML 링크 문자열 NUL 누락");return {&*first,static_cast<std::size_t>(last-first)};
}
// 기본 Tag의 현재 링크 효과 슬롯만 실제 문자열 저장소에 연결한다.
void BindHtmlLinkText(HtmlTextParserHooks& hooks,HtmlLinkTextState& links) {
    hooks.link=[&links](HtmlTextParserState&,std::string_view raw,bool closing) { return HtmlLinkText(links,raw,closing); };
}
}
