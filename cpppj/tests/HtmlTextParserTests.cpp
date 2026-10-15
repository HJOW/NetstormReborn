// 두 실제 PE의 기본 HTML 출력과 현재 상태·외부 효과 경계·StyleText 전달을 검사한다.
#include "TestSupport.h"
#include "client/HtmlTextParser.h"
#include "client/StyleText.h"
#include <fstream>
#include <filesystem>
#include <sstream>
#include <stdexcept>

using namespace netstorm::client;
namespace {
// fixture의 UTF-8 문서 표현을 원본 byte 문자열로 바꾼다. 기대 변환 자체는 계산하지 않는다.
std::string Unhex(const std::string& text) {
    std::string result;
    // 원본 high byte와 NUL도 손실 없이 복원한다.
    for (std::size_t offset=0;offset<text.size();offset+=2) result.push_back(static_cast<char>(std::stoul(text.substr(offset,2),nullptr,16)));
    return result;
}
// 빈 입력 hex열도 보존하는 탭 구분 fixture reader다.
std::vector<std::string> Columns(const std::string& line) {
    std::vector<std::string> result;std::istringstream stream(line);std::string column;
    // 공백 추출 연산자와 달리 연속 탭 사이 빈 열도 하나의 입력으로 남긴다.
    while (std::getline(stream,column,'\t')) result.push_back(column);
    return result;
}
}
// native 출력/커서/모든 순회 상태를 같은 초기 shown/depth/P전역에서 대조한다.
TEST_CASE(HtmlTextParser_IndependentPeFixture) {
    std::ifstream input(NETSTORM_HTMLTEXT_FIXTURE);CHECK(input.good());std::string line;std::size_t count=0;
    // 원본은 linkTape=0으로 설정했다. 링크 helper 호출은 실제 null 저장소 계약만 공급한다.
    while (std::getline(input,line)) {
        if (line.empty() || line.front()=='#') continue;
        const auto row=Columns(line);CHECK(row.size()==14);if (row.size()!=14) continue;
        HtmlTextParserState state;state.shown=static_cast<std::uint32_t>(std::stoul(row[3]));state.hiddenDepth=static_cast<std::uint32_t>(std::stoul(row[4]));
        HtmlTextParserGlobals globals{static_cast<std::uint32_t>(std::stoul(row[5]))};HtmlTextParserHooks hooks;
        hooks.link=[](auto&,auto,bool) { return std::string{}; }; // 실제 native의 null linkTape와 같은 명시 효과다.
        HtmlTextParser parser(state,globals,hooks,row[0]=="originals" ? netstorm::o::OriginalEdition::Patch1078 : netstorm::o::OriginalEdition::Cd1072);
        const auto converted=parser.Parse(Unhex(row[1]),std::stoi(row[2]));
        if (converted!=Unhex(row[6])) std::fprintf(stderr,"HTML native row %zu edition %s input %s expected %s\n",count,row[0].c_str(),row[1].c_str(),row[6].c_str());
        CHECK(converted==Unhex(row[6]));
        CHECK(state.inputOffset==std::stoul(row[7]));CHECK(state.shown==std::stoul(row[8]));CHECK(state.hiddenDepth==std::stoul(row[9]));
        CHECK(state.inTag==std::stoul(row[10]));CHECK(state.textAllowed==std::stoul(row[11]));CHECK(state.afterHeading==std::stoul(row[12]));CHECK(globals.suppressParagraph==std::stoul(row[13]));++count;
    }
    CHECK(count==816);
}
// 구현하지 않은 실제 효과가 선택되면 조용히 성공하지 않으며 숨긴 비조건 태그는 호출하지 않는다.
TEST_CASE(HtmlTextParser_ExplicitEffectBoundaries) {
    HtmlTextParserState state;HtmlTextParserGlobals globals;HtmlTextParser parser(state,globals);
    // 각각 실제 원본에서 외부 효과를 요구하는 분기다.
    for (const auto* source : {"<$cmd 1>","<?condition>","<!\"picture\">","<a href=\"target\">","</a>","<background=\"cloud\">"}) {
        bool rejected=false;try { parser.Parse(source); } catch (const std::logic_error&) { rejected=true; }CHECK(rejected);
    }
    state.shown=0;CHECK(parser.Parse("<$cmd><!\"picture\"><background=\"cloud\">")=="\n");
    state.shown=1;CHECK(parser.Parse("<unknown>text")=="text\n");CHECK(parser.Parse("<background>text")=="text\n");
    bool overflow=false;try { parser.Parse("<b><b><b>",2); } catch (const std::length_error&) { overflow=true; }CHECK(overflow);
}
// 조건과 명령 callback은 현재 상태를 직접 바꾸고 그 뒤 분기부터 새 값을 읽는다.
TEST_CASE(HtmlTextParser_CurrentConditionAndCommandState) {
    HtmlTextParserState state;HtmlTextParserGlobals globals;HtmlTextParserHooks hooks;std::vector<std::string> calls;
    hooks.condition=[&](auto& current,auto raw,bool close) { // 별도 조건 계산기 대신 명시한 효과만 공급한다.
        calls.push_back(std::string(raw));current.shown=close ? 1U : 0U;current.hiddenDepth=close ? 0U : 1U;
    };
    hooks.command=[&](auto& current,auto raw) { calls.push_back(std::string(raw));current.output+="Q"; }; // 큐 경계는 '$'가 제거된 나머지 원문을 받는다.
    HtmlTextParser parser(state,globals,hooks);CHECK(parser.Parse("a<?off>hidden</?off>b<$raw 7>c")=="abQc\n");
    CHECK(calls.size()==3 && calls[0]=="?off>hidden</?off>b<$raw 7>c" && calls[1]=="?off>b<$raw 7>c" && calls[2]=="raw 7>c");
    CHECK(state.shown==1 && state.hiddenDepth==0 && state.textAllowed==1);
    CHECK(parser.Parse("next")=="next\n");CHECK(state.shown==1 && state.hiddenDepth==0);
}
// 기본 VT의 noop와 달리 파생 슬롯은 현재 커서/문자열을 관찰하고 다음 조회 위치를 바꿀 수 있다.
TEST_CASE(HtmlTextParser_DerivedSlotOrderAndCursor) {
    HtmlTextParserState state;HtmlTextParserGlobals globals;HtmlTextParserHooks hooks;std::vector<std::string> calls;
    hooks.insideTag=[&](auto& current,auto source) { calls.push_back("in:"+std::to_string(current.inputOffset));CHECK(source[current.inputOffset]!='<'); }; // virtual0은 태그 내부 일반 문자에 도달한다.
    hooks.beforeClose=[&](auto& current,auto source) { calls.push_back("close:"+std::to_string(current.inputOffset));CHECK(source[current.inputOffset]=='/'); }; // virtual1 직전 slash 이동을 검사한다.
    hooks.afterTag=[&](auto& current,auto source) { calls.push_back("after:"+std::to_string(current.inputOffset));CHECK(source[current.inputOffset]=='>' && current.inTag==0); }; // virtual2는 >에서 수행한다.
    HtmlTextParser parser(state,globals,hooks);CHECK(parser.Parse("<i>x</i>")=="~N~jx~.\n");
    CHECK((calls==std::vector<std::string>{"in:1","after:2","close:5","in:6","after:7"}));
    hooks.afterTag=[](auto& current,auto) { current.inputOffset+=2; }; // 원본 raw 복사처럼 callback 뒤 포인터를 다시 읽는지 검사한다.
    HtmlTextParser moved(state,globals,hooks);CHECK(moved.Parse("<b>12X")=="~N~BX\n");
}
// Early1 본문의 i/b는 줄을 나누지 않고 색/굵기 제어로 기존 StyleText 조각에 전달한다.
TEST_CASE(HtmlTextParser_InlineStyleTextIntegration) {
    HtmlTextParserState html;HtmlTextParserGlobals globals;HtmlTextParser parser(html,globals);
    const auto converted=parser.Parse("walk <i>edge</i> and <b>open</b>.");CHECK(converted=="walk ~N~jedge~. and ~N~Bopen~N.\n");
    StyleTextState state;StyleTextHooks hooks;hooks.color=[](char command) { CHECK(command=='j');return 77U; }; // 색만 공급하며 i 폰트 치환은 하지 않는다.
    StyleText styled(state,hooks);std::uint32_t head=0;std::array<std::uint32_t,8> context{};context[2]=17;styled.Parse(head,converted,context);
    std::string plain;bool edge=false,open=false;
    // 실제 생성된 조각의 본문과 저장 문맥을 순회해 색/스타일/원문 줄 수를 확인한다.
    for (auto token=head;token;token=state.fragments.at(token).next) {
        const auto& node=state.fragments.at(token);const std::string text(node.text.data());plain+=text;
        if (text=="edge") { edge=true;CHECK(node.context[2]==77 && node.context[3]==0); }
        if (text=="open") { open=true;CHECK(node.context[3]==2); }
    }
    CHECK(plain=="walk edge and open.\n" && edge && open);
}
// 새 그림 helper 입력만 두 PE에서 실제 실행한 바이트/커서/현재 상태를 전수 대조한다.
TEST_CASE(HtmlTextParser_PictureIndependentPeFixture) {
    std::ifstream input(std::filesystem::path(__FILE__).parent_path()/"fixtures/htmlpicture-x86.tsv");CHECK(input.good());std::string line;std::size_t count=0;
    // 그림 helper는 실제 출력만 공급하며 자산 이름 해석은 후속 StyleTextMarkup의 책임이다.
    while (std::getline(input,line)) {
        if (line.empty() || line.front()=='#') continue;
        const auto row=Columns(line);CHECK(row.size()==14);if (row.size()!=14) continue;
        HtmlTextParserState state;state.shown=static_cast<std::uint32_t>(std::stoul(row[3]));state.hiddenDepth=static_cast<std::uint32_t>(std::stoul(row[4]));
        HtmlTextParserGlobals globals{static_cast<std::uint32_t>(std::stoul(row[5]))};HtmlTextParserHooks hooks;BindHtmlPictureText(hooks);
        HtmlTextParser parser(state,globals,hooks,row[0]=="originals" ? netstorm::o::OriginalEdition::Patch1078 : netstorm::o::OriginalEdition::Cd1072);
        CHECK(parser.Parse(Unhex(row[1]),std::stoi(row[2]))==Unhex(row[6]));CHECK(state.inputOffset==std::stoul(row[7]));
        CHECK(state.shown==std::stoul(row[8]) && state.hiddenDepth==std::stoul(row[9]));CHECK(state.inTag==std::stoul(row[10]));
        CHECK(state.textAllowed==std::stoul(row[11]) && state.afterHeading==std::stoul(row[12]));CHECK(globals.suppressParagraph==std::stoul(row[13]));++count;
    }
    CHECK(count==276);
}
