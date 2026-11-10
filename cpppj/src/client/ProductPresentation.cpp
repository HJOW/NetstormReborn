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
// 한국어 모드의 11.01 패치 크레딧 본문이다. 영어 본문과 같은 구성이며 새 항목 이름과 제목은 사용자 지정 표기를 따른다.
constexpr std::string_view kPatchCreditsKorean=R"(<background="nsstart">
<h2>{H2}NetStorm 11.01 패치 크레딧</h2>
<p>{Text}
버전: {version}<br>
개발 및 NetStorm 11.01 패치: HJOW.<br>
<p>
OpenAI Codex와 ChatGPT를 활용한 AI 지원으로 개발했습니다.<br>
AI는 분석, 프로그래밍, 문서화, 검증을 돕는 데 사용했습니다.<br>
<p>
원작 제작자는 원본 크레딧 항목에서 확인할 수 있습니다.
$Button=뒤로,Tell,Credits
$Button=취소,Tell,Blank
)"; // 한국어 UI 글꼴로 표시하며 K4에서 영어 잔재로 남아 있던 세 번째 항목을 번역했다.
// 일본어 모드의 11.01 패치 크레딧 본문이다. 한국어 본문과 같은 구성이며 제목/버튼 이름만 일본어다(11차 일본어).
constexpr std::string_view kPatchCreditsJapanese=R"(<background="nsstart">
<h2>{H2}NetStorm 11.01 パッチ クレジット</h2>
<p>{Text}
バージョン: {version}<br>
開発およびNetStorm 11.01パッチ: HJOW。<br>
<p>
OpenAI CodexとChatGPTを活用したAI支援で開発しました。<br>
AIは分析、プログラミング、ドキュメント作成、検証の補助に使用しました。<br>
<p>
原作の制作者は、元のクレジット項目で確認できます。
$Button=戻る,Tell,Credits
$Button=キャンセル,Tell,Blank
)";
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
std::string ProductTellBody(std::string_view target,std::string_view original,std::uint32_t language) {
    const bool korean=language==7U,japanese=language==5U; // 원문 언어 번호(LanguageOptions.h의 한국어 7, 일본어 5)다.
    if (korean && o::AsciiLower(target)==o::AsciiLower(kProductCreditsTarget)) return std::string(kPatchCreditsKorean); // 한국어의 새 항목 본문은 고정 번역본이다.
    if (japanese && o::AsciiLower(target)==o::AsciiLower(kProductCreditsTarget)) return std::string(kPatchCreditsJapanese); // 11차 일본어: 같은 구성의 고정 번역본이다.
    std::string body(original);
    if (o::AsciiLower(target)=="credits" && original.find(kProductCreditsTarget)==original.npos)
        body+=korean ? "\n$Menu=NetStorm 11.01 패치 크레딧,Tell,Patch1101.Credits\n" : japanese ? "\n$Menu=NetStorm 11.01 パッチ クレジット,Tell,Patch1101.Credits\n" : "\n$Menu=Netstorm 11.01 Patch Credits,Tell,Patch1101.Credits\n";
    return body;
}
}
