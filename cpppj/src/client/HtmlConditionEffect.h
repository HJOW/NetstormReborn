// 원본 HTML ? 분기의 토큰·Config 치환 시점·비교·현재 shown/depth 효과를 제공한다.
#pragma once
#include "client/HtmlTextParser.h"
#include "o/ConfigInterface.h"

namespace netstorm::client {
// 값은 매 실제 평가 시점에 다시 치환한다. 두 피연산자를 미리 snapshot하지 않는다.
struct HtmlConditionEffectHooks {
    std::function<std::string(std::string_view)> substitute; // 원본00441740/CD004a9bd0의 현재 Config 치환 결과다.
};
// PATCH0046c090 ? 분기와 CD00495420 boolean 분기를 현재 파서 객체에 적용한다.
class HtmlConditionEffect {
public:
    // 토큰 읽기와 CRT 숫자는 공통 복원기를 쓰고 Config는 선택한 실제 평가 시점에 요구한다.
    explicit HtmlConditionEffect(HtmlConditionEffectHooks hooks,o::OriginalEdition edition=o::OriginalEdition::Patch1078);
    // raw는 '?'부터 본문 NUL까지의 원문이다. 닫힘은 Config를 읽지 않고 현재 깊이만 처리한다.
    void Apply(HtmlTextParserState& state,std::string_view raw,bool closing) const;
private:
    // 실제100바이트 치환 임시 저장소의 안전한 결과를 문자열로 보관한다. 원본 오버런은 거부한다.
    std::string Substitute(std::string_view token) const;
    HtmlConditionEffectHooks hooks_; // 현재 Config 조회/치환 공급자다.
    o::OriginalEdition edition_; // PATCH 비교 연산과 CD boolean 문법을 구분한다.
};
// 실제 ConfigInterface의 local 인자 없는 치환을 매 호출마다 수행하는 effect를 기본 파서에 연결한다.
void BindHtmlConditionConfig(HtmlTextParserHooks& hooks,o::ConfigInterface& config,o::OriginalEdition edition=o::OriginalEdition::Patch1078);
}
