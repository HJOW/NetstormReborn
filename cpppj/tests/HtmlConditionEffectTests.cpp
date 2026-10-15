// 두 PE의 실제 HTML조건/토큰/CRT·분리된 실제Config치환과 현재 상태 효과를 전수 대조한다.
#include "TestSupport.h"
#include "client/HtmlConditionEffect.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>

using namespace netstorm::client;
namespace {
// 원본 바이트 hex를 UTF-8 문서 변환 없이 복원한다.
std::string Bytes(std::string_view encoded) {
    if (encoded=="-") return {};std::string output;
    // signed high byte/NUL/공백도 실제 fixture 입력과 같게 유지한다.
    for (std::size_t offset=0;offset<encoded.size();offset+=2) output+=static_cast<char>(std::stoul(std::string(encoded.substr(offset,2)),nullptr,16));
    return output;
}
// 원시 인자/치환 출력의 실제 비교 순서를 안전한 hex로 기록한다.
std::string Hex(std::string_view value) {
    constexpr char digits[]="0123456789abcdef"; // 각 원본 바이트를 두 ASCII 문자로 고정하는 표다.
    std::string output;
    // char 부호와 무관하게 하위 BYTE를 같은 순서로 기록한다.
    for (const unsigned char byte:value) { output+=digits[byte>>4];output+=digits[byte&15]; }
    return output;
}
// 빈 출력 열을 잃지 않는 fixture/계획 구분자 reader다.
std::vector<std::string> Split(std::string_view text,char delimiter) {
    std::vector<std::string> output;std::istringstream stream{std::string(text)};std::string part;
    // 연속 구분자 사이의 빈 값도 한 열로 남긴다.
    while (std::getline(stream,part,delimiter)) output.push_back(part);
    return output;
}
}
// 결과는 C++ 모델이 아니라 실제 PE의 가림/출력/커서/토큰/Config 호출 순서에서 읽는다.
TEST_CASE(HtmlConditionEffect_IndependentPeFixture) {
    std::ifstream input(std::filesystem::path(__FILE__).parent_path()/"fixtures/htmlcondition-x86.tsv");CHECK(input.good());std::string line;std::size_t count=0;
    // 매 입력의 현재 Config를 독립적으로 만들고 원본과 같은 시점에만 치환한다.
    while (std::getline(input,line)) {
        if (line.empty() || line.front()=='#') continue;
        const auto row=Split(line,'\t');CHECK(row.size()==16);if (row.size()!=16) continue;
        const auto edition=row[0]=="originals" ? netstorm::o::OriginalEdition::Patch1078 : netstorm::o::OriginalEdition::Cd1072;
        const auto configurations=Split(row[6],',');std::size_t call=0;std::string trace;netstorm::o::ConfigInterface config({});
        HtmlConditionEffectHooks effects;effects.substitute=[&](std::string_view token) {
            config.Configuration().Assign(Bytes(configurations[std::min(call,configurations.size()-1)]));++call;
            const auto value=config.SubstituteLocal(token);if (!trace.empty()) trace+=';';trace+=Hex(token)+'='+Hex(value);return value;
        }; // native의 Config ABI 경계마다 같은 현재 설정을 실제 ConfigInterface에 공급한다.
        HtmlConditionEffect condition(effects,edition);HtmlTextParserState state;state.shown=static_cast<std::uint32_t>(std::stoul(row[3]));state.hiddenDepth=static_cast<std::uint32_t>(std::stoul(row[4]));HtmlTextParserGlobals globals;
        if (row[1]=="Tag") condition.Apply(state,Bytes(row[2]),std::stoul(row[5])!=0);
        else {
            HtmlTextParserHooks hooks;hooks.condition=[&](auto& current,auto raw,bool closing) { condition.Apply(current,raw,closing); }; // 전체 순회의 동일한 현재 객체를 사용한다.
            hooks.link=[](auto&,auto,bool) { return std::string{}; }; // native 입력의 linkTape0 계약만 공급한다.
            HtmlTextParser(state,globals,hooks,edition).Parse(Bytes(row[2]));
        }
        const bool matched=state.output==Bytes(row[7]) && state.inputOffset==std::stoul(row[8]) && state.shown==std::stoul(row[9]) && state.hiddenDepth==std::stoul(row[10]) &&
            state.inTag==std::stoul(row[11]) && state.textAllowed==std::stoul(row[12]) && state.afterHeading==std::stoul(row[13]) && globals.suppressParagraph==std::stoul(row[14]) && (trace.empty() ? "-" : trace)==row[15];
        if (!matched) std::fprintf(stderr,"HTML condition row %zu %s %s raw %s actual shown %u depth %u trace %s expected %s\n",count,row[0].c_str(),row[1].c_str(),row[2].c_str(),state.shown,state.hiddenDepth,trace.c_str(),row[15].c_str());
        CHECK(matched);++count;
    }
    CHECK(count==754);
}
// 오른쪽 치환 후 바뀐 현재 Config를 왼쪽에서 다시 읽으며 닫힘/숨김은 조회하지 않는다.
TEST_CASE(HtmlConditionEffect_LiveConfigAndHiddenDepth) {
    netstorm::o::ConfigInterface config({});config.Configuration().Assign("X=1\r\n");HtmlTextParserState state;std::vector<std::string> calls;
    HtmlConditionEffectHooks effects;effects.substitute=[&](std::string_view token) {
        calls.emplace_back(token);const auto value=config.SubstituteLocal(token);config.Configuration().Set("X","2");return value;
    }; // 첫 오른쪽1 뒤 실제 설정을2로 바꾸어 stale snapshot을 검출한다.
    HtmlConditionEffect condition(effects);condition.Apply(state,"?g{X}={X}>",false);CHECK(state.shown==1 && state.hiddenDepth==0 && calls.size()==2);
    state.shown=0;state.hiddenDepth=2;condition.Apply(state,"?{X}>",false);CHECK(calls.size()==2 && state.hiddenDepth==2);
    condition.Apply(state,"?",true);CHECK(state.shown==0 && state.hiddenDepth==1 && calls.size()==2);
    condition.Apply(state,"?",true);CHECK(state.shown==1 && state.hiddenDepth==0 && calls.size()==2);
}
// 실제 binder는 기존 ConfigInterface의 이름 객체/기본값/현재 값 변경을 그대로 사용한다.
TEST_CASE(HtmlConditionEffect_BindsActualConfigNamespaces) {
    netstorm::o::ConfigInterface config({});config.Configuration().Assign("ShardtoStone=100\r\n");netstorm::o::Config parameter(config.Registry(),"param");parameter.Assign("3=99\r\n");
    HtmlTextParserState state;HtmlTextParserGlobals globals;HtmlTextParserHooks hooks;BindHtmlConditionConfig(hooks,config);
    HtmlTextParser parser(state,globals,hooks);CHECK(parser.Parse("<?l {param.3}={ShardtoStone}>less</?>end")=="lessend\n");
    parameter.Set("3","100");CHECK(parser.Parse("<?l! {param.3}={ShardtoStone}>enough</?>end")=="enoughend\n");
    CHECK(parser.Parse("<?{missing|0}>hidden</?>end")=="end\n");CHECK(parser.Parse("<?!{missing|0}>visible</?>end")=="visibleend\n");
}
// 미공급 효과와 원본100바이트 버퍼를 넘는 치환은 빈 성공으로 바꾸지 않는다.
TEST_CASE(HtmlConditionEffect_ExplicitMissingAndOverflowBoundaries) {
    HtmlTextParserState state;HtmlConditionEffect missing({});bool rejected=false;
    try { missing.Apply(state,"?1>",false); } catch (const std::logic_error&) { rejected=true; }CHECK(rejected);
    state.shown=0;state.hiddenDepth=1;missing.Apply(state,"?1>",false);missing.Apply(state,"?",true);CHECK(state.shown==1 && state.hiddenDepth==0);
    HtmlConditionEffectHooks hooks;hooks.substitute=[](auto) { return std::string(100,'1'); }; // 원본의 유효 버퍼 밖 결과를 명시하여 메모리 오버런 경계를 확인한다.
    rejected=false;try { HtmlConditionEffect(hooks).Apply(state,"?1>",false); } catch (const std::length_error&) { rejected=true; }CHECK(rejected);
}
