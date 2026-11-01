// 사용자 요청의 제품 표기는 원문 데이터 판본과 분리하고 같은 HTML/Config/Tell 경로를 사용한다.
#include "client/ProductPresentation.h"
#include "o/OriginalText.h"

namespace netstorm::client { // 원본 파일에 쓰지 않는 제품 전용 표시 경계다.
namespace { // 추가 Credits의 고정 본문이며 기존 HTML 창 공장을 그대로 사용한다.
constexpr std::string_view kPatchCredits=R"([Credits]
<background="nsstart">
<h2>{H2}NetStorm 11.01 Patch Credits</h2>
<p>{Text}
Version: {version}<br>
Development and NetStorm 11.01 patch: HJOW.<br>
<p>
Developed with AI assistance using OpenAI Codex and ChatGPT.<br>
AI was used to assist analysis, programming, documentation, and verification.<br>
<p>
The original game creators are acknowledged in the original Credits entries.
$Button=Back,Tell,Credits
$Button=Cancel,Tell,Blank
)"; // 요청한 제목/HJOW 개발/AI 사용 설명을 현재 영문 UI 글꼴로 표시한다.
}
// Startup의 원문 v주.부 정수 표기 뒤 제품 표기11.01의 선행0을 명시적으로 보존한다.
void ApplyProductVersion(o::ConfigInterface& config) {
    config.SetInt("MAJOR_VERSION",kProductMajorVersion);config.SetInt("MINOR_VERSION",kProductMinorVersion);
    config.Set("version",std::string("v")+std::string(kProductVersion)); // 원문과 다른 제품 표기는 사용자 요청에 따른다.
}
// 등록한 이름의 실제 Config를 반환하므로 별도 호스트 팝업이나 HTML 파서를 만들지 않는다.
std::unique_ptr<o::Config> MakeProductCreditsConfig(o::ConfigRegistry& registry) {
    auto credits=std::make_unique<o::Config>(registry,kProductCreditsObject);credits->Append(kPatchCredits);return credits;
}
// 원문 두 항목/Cancel을 유지하고 중복 확장도 피한다. 새 항목의 대소문자는 사용자 지정 표기다.
std::string ProductTellBody(std::string_view target,std::string_view original) {
    std::string body(original);
    if (o::AsciiLower(target)=="credits" && original.find(kProductCreditsTarget)==original.npos)
        body+="\n$Menu=Netstorm 11.01 Patch Credits,Tell,Patch1101.Credits\n";
    return body;
}
}
