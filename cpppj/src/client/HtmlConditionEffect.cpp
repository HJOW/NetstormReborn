// 원본의 오른쪽 치환/atol→왼쪽 치환/atol 순서와 숨김 상태의 선택적 조회를 유지한다.
#include "client/HtmlConditionEffect.h"
#include "o/RawTextToken.h"
#include <stdexcept>
#include <utility>

namespace netstorm::client {
namespace {
constexpr std::size_t kConditionOutputCapacity=100; // 원본 Config 치환 helper에 전달하는 임시 버퍼 크기다.
// NUL 뒤/호스트 범위 밖의 가상0을 읽어 짧거나 미완성인 조건도 같은 분기를 고른다.
char Byte(std::string_view source,std::size_t offset) { return offset<source.size() ? source[offset] : '\0'; }
// 연산자의 각 바이트는 원본처럼 해당 영문자 한 글자의 대소문자만 허용한다.
bool Is(char byte,char lower) { return byte==lower || byte==lower-'a'+'A'; }
}
// 훅은 boolean/비교 평가가 실제 선택될 때 확인한다. 닫힘/숨김 상태에는 필요하지 않다.
HtmlConditionEffect::HtmlConditionEffect(HtmlConditionEffectHooks hooks,o::OriginalEdition edition):hooks_(std::move(hooks)),edition_(edition) {}
// 원본 C문자열의 첫 NUL까지만 비교하며 미복원 메모리 오버런은 성공으로 처리하지 않는다.
std::string HtmlConditionEffect::Substitute(std::string_view token) const {
    if (!hooks_.substitute) throw std::logic_error("HTML 조건의 현재 Config 치환 미연결");
    auto value=hooks_.substitute(token);value.resize(value.find('\0')==std::string::npos ? value.size() : value.find('\0'));
    if (value.size()>=kConditionOutputCapacity) throw std::length_error("HTML 조건 원본100바이트 치환 버퍼 초과");
    return value;
}
// 표시/깊이를 복사하지 않고 실제 현재 객체에 쓰며 원본의 숨긴 열림 조건 무평가 규칙도 보존한다.
void HtmlConditionEffect::Apply(HtmlTextParserState& state,std::string_view raw,bool closing) const {
    raw=raw.substr(0,raw.find('\0'));
    if (Byte(raw,0)!='?') throw std::invalid_argument("HTML 조건 원문은 ?에서 시작해야 함");
    if (!state.shown && !state.hiddenDepth) return;
    if (closing) {
        if (state.hiddenDepth && !--state.hiddenDepth) state.shown=1;
        return;
    }
    std::size_t offset=1;const bool leadingNegation=Byte(raw,offset)=='!';if (leadingNegation) ++offset;
    std::uint32_t operation=0; // 0:boolean,1:문자열동등,2:g,3:ge,4:l,5:le다.
    bool operatorNegation=false; // PATCH는 연산자 뒤 !도 앞 !와 같은 OR 부정으로 처리한다.
    if (edition_==o::OriginalEdition::Patch1078) {
        if (Byte(raw,offset)=='?') { ++offset;operation=1; }
        if (Is(Byte(raw,offset),'g')) { ++offset;operation=2;if (Is(Byte(raw,offset),'e')) { ++offset;operation=3; } }
        if (Is(Byte(raw,offset),'l')) { ++offset;operation=4;if (Is(Byte(raw,offset),'e')) { ++offset;operation=5; } }
        operatorNegation=Byte(raw,offset)=='!';if (operatorNegation) ++offset;
    }
    if (!state.shown) return;
    bool predicate=false;
    if (operation) {
        const auto left=o::ReadRawTextToken(raw,offset,"=");const auto right=o::ReadRawTextToken(raw,left.next,">");
        const auto rightValue=Substitute(right.text); // 원본은 오른쪽을 먼저 치환한다.
        if (operation==1) predicate=Substitute(left.text)==rightValue;
        else {
            const auto rightNumber=o::ConfigParseLong(rightValue); // 왼쪽 치환 이전에 오른쪽 atol을 완료한다.
            const auto leftNumber=o::ConfigParseLong(Substitute(left.text));
            switch (operation) {
            case 2:predicate=rightNumber<leftNumber;break;
            case 3:predicate=rightNumber<=leftNumber;break;
            case 4:predicate=leftNumber<rightNumber;break;
            case 5:predicate=leftNumber<=rightNumber;break;
            default:throw std::logic_error("HTML 조건 연산자 내부 범위 오류");
            }
        }
    } else {
        const auto token=o::ReadRawTextToken(raw,offset,"=>");
        predicate=o::ConfigParseLong(Substitute(token.text))!=0;
    }
    if (predicate!=(!leadingNegation && !operatorNegation)) { state.shown=0;++state.hiddenDepth; }
}
// local 임시 Config 생성/치환/해제는 기존 실제 인터페이스가 담당한다. 값을 캐시하지 않는다.
void BindHtmlConditionConfig(HtmlTextParserHooks& hooks,o::ConfigInterface& config,o::OriginalEdition edition) {
    HtmlConditionEffectHooks effects;effects.substitute=[&config](std::string_view token) { return config.SubstituteLocal(token); }; // 두 인자 모두 현재 Config에서 독립 조회한다.
    const HtmlConditionEffect condition(std::move(effects),edition);
    hooks.condition=[condition](HtmlTextParserState& state,std::string_view raw,bool closing) { condition.Apply(state,raw,closing); }; // 같은 상태 참조를 원본 효과에 넘긴다.
}
}
