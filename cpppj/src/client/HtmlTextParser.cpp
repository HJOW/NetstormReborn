// 태그의 원문 규칙·조건별 현재 상태 재조회·원본 출력 바이트 순서를 유지한다.
#include "client/HtmlTextParser.h"
#include <algorithm>
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::client {
namespace {
constexpr std::int32_t kOutputReserve=256; // 원본 평문 순회가 태그/마지막 LF를 위해 남기는 논리 여유 바이트다.
// NUL 문자열 끝의 가상0을 제공하며 호스트 string_view 밖은 읽지 않는다.
char Byte(std::string_view source,std::size_t offset) { return offset<source.size() ? source[offset] : '\0'; }
// 필수 효과가 없으면 해당 실제 분기를 거부한다. 기본 VT의 검증된 noop와 구분한다.
template<class Function> const Function& Required(const Function& hook,const char* name) {
    if (!hook) throw std::logic_error(std::string("HTML 실제 효과 미연결: ")+name);return hook;
}
}
// 현재 객체의 수명을 빌리며 생성 시 게임/UI/외부 효과는 실행하지 않는다.
HtmlTextParser::HtmlTextParser(HtmlTextParserState& state,HtmlTextParserGlobals& globals,HtmlTextParserHooks hooks,o::OriginalEdition edition)
    :state_(state),globals_(globals),hooks_(std::move(hooks)),edition_(edition) {}
// 원본의 NUL 쓰기 자리도 물리 한도에 포함한다.
void HtmlTextParser::Append(std::string_view text,std::int32_t capacity) const {
    if (state_.output.size()+text.size()>=static_cast<std::size_t>(capacity)) throw std::length_error("HTML 원본 물리 출력 버퍼 초과");
    state_.output.append(text);
}
// 파생 효과가 source 포인터나 출력 포인터를 현재 버퍼 밖으로 바꾸는 경우만 호스트에서 거부한다.
void HtmlTextParser::Validate(std::string_view source,std::int32_t capacity) const {
    if (state_.inputOffset>source.size() || state_.output.size()>=static_cast<std::size_t>(capacity))
        throw std::out_of_range("HTML callback 현재 버퍼 위치 오류");
}
// 첫 글자의 case만 정규화하며 둘째/셋째 글자는 원본 literal 비교를 그대로 적용한다.
void HtmlTextParser::Tag(std::string_view source,std::size_t offset,bool closing,std::int32_t capacity) const {
    const auto raw=std::bit_cast<std::int8_t>(static_cast<std::uint8_t>(Byte(source,offset)));
    const int first=raw>='A' && raw<='Z' ? raw+'a'-'A' : raw;const auto second=Byte(source,offset+1);
    const auto remaining=source.substr(std::min(offset,source.size()));std::string_view result;
    switch (first) {
    case '!': Append(Required(hooks_.picture,"picture")(state_,remaining),capacity);break;
    case '$': Required(hooks_.command,"command")(state_,remaining.substr(1));break;
    case '?': Required(hooks_.condition,"condition")(state_,remaining,closing);break;
    case 'a':
        if (closing) { Append(Required(hooks_.link,"link-close")(state_,{},true),capacity);result="~N~."; }
        else if (Byte(source,offset+2)=='h') { Append(Required(hooks_.link,"link-open")(state_,remaining,false),capacity);result="~l~E"; }
        break;
    case 'b':
        if (second=='r') { result="\n";state_.afterHeading=0; }
        else if (second=='a') {
            const auto end=remaining.find('>'),equal=remaining.find('=');
            if (equal!=std::string_view::npos && (end==std::string_view::npos || equal<end)) {
                auto start=equal+1;if (Byte(remaining,start)=='"') ++start;auto finish=start;
                // 배경 이름은 첫 따옴표/꺾쇠/NUL 앞에서 끝난다. 공백이나 대소문자는 바꾸지 않는다.
                while (finish<remaining.size() && remaining[finish]!='>' && remaining[finish]!='"') ++finish;
                Required(hooks_.background,"background")(state_,remaining.substr(start,finish-start));
            }
        } else result=closing ? "~N" : "~N~B";
        break;
    case 'c': result=closing ? "~." : "~N~i";break;
    case 'h':
        if (closing) { result=edition_==o::OriginalEdition::Cd1072 && second=='1' ? "~*~N~.\n \n" : "~*~N~.\n";state_.afterHeading=1; }
        else if (second=='3') result="~N~5~B~E";
        else if (edition_==o::OriginalEdition::Cd1072) result="~N~3~E";
        else if (second=='1') result="~N~4~E";
        else if (second=='2') result="~N~3~E";
        else result="~N~1~E";
        break;
    case 'i': result=closing ? "~." : "~N~j";break;
    case 'p':
        if (globals_.suppressParagraph) globals_.suppressParagraph=0;
        else { result=state_.afterHeading ? "~*\n" : "~*\n \n";state_.afterHeading=0; }
        break;
    case 'q': result=closing ? "~." : "~N~q";break;
    case '~': Append("~",capacity);if (second) Append(std::string_view(&source[offset+1],1),capacity);break;
    default:break;
    }
    Validate(source,capacity);Append(result,capacity);
}
// base 가상 표의 빈 동작만 기본 제공하며 파생 TextHtml의 동적 본문/원문 복사는 명시 콜백에 맡긴다.
std::string HtmlTextParser::Parse(std::string_view source,std::int32_t capacity) const {
    if (capacity<2) throw std::invalid_argument("HTML 마지막 LF/NUL을 담을 물리 버퍼 필요");
    source=source.substr(0,source.find('\0'));state_.inputOffset=0;state_.output.clear();state_.afterHeading=0;
    state_.textAllowed=state_.shown;state_.inTag=0;
    // 태그 callback이 바꾼 현재 cursor를 다음 반복에서 다시 읽는다. 태그 전체를 한 번에 건너뛰지 않는다.
    while (Byte(source,state_.inputOffset)) {
        const auto byte=static_cast<std::uint8_t>(Byte(source,state_.inputOffset));
        if (byte=='<') {
            state_.inTag=1;
            if (!state_.hiddenDepth && Byte(source,state_.inputOffset+1)=='a') ++state_.textAllowed;
            const bool closing=Byte(source,state_.inputOffset+1)=='/';
            if (state_.shown || (state_.hiddenDepth && closing && Byte(source,state_.inputOffset+2)=='?')) {
                if (closing) {
                    ++state_.inputOffset;if (Byte(source,state_.inputOffset+1)=='a') --state_.textAllowed;
                    if (hooks_.beforeClose) hooks_.beforeClose(state_,source);Validate(source,capacity);
                }
                Tag(source,state_.inputOffset+1,closing,capacity);
            }
        } else if (byte=='>') {
            state_.inTag=0;if (state_.shown && hooks_.afterTag) hooks_.afterTag(state_,source);
        } else if (!state_.inTag) {
            if (state_.shown) {
                if (std::bit_cast<std::int32_t>(state_.textAllowed)<1 ||
                    static_cast<std::int64_t>(state_.output.size())>static_cast<std::int64_t>(capacity)-kOutputReserve) break;
                if (byte>0x1f) {
                    Append(source.substr(state_.inputOffset,1),capacity);
                    if (state_.afterHeading) state_.afterHeading=Byte(source,state_.inputOffset)==' ';
                }
                if (Byte(source,state_.inputOffset)=='\n' && !state_.output.empty() && state_.output.back()!=' ' && state_.output.back()!='\n') Append(" ",capacity);
            }
        } else if (hooks_.insideTag) hooks_.insideTag(state_,source);
        Validate(source,capacity);++state_.inputOffset;
    }
    Append("\n",capacity);return state_.output;
}
}
