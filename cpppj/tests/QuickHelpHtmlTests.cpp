// 제품 작성 전 동결한 전용 Help83행·추가 중첩3행과 실제 기본 HTML/링크/원본 파일을 대조한다.
#include "GumpRawSupport.h"
#include "client/QuickHelpHtml.h"
#include "o/BaseFile.h"
#include <filesystem>
using namespace netstorm::client;
using namespace netstorm::test::gumpraw;
namespace {
// 실제 바이트만16진수로 직렬화한다.
std::string Text(std::string_view value) { return Hex({reinterpret_cast<const std::uint8_t*>(value.data()),value.size()}); }
// 독립 fixture의 평평한 JSON 배열만 읽는다. 제품/기대값을 계산하는 파서는 아니다.
std::vector<PriestMenuArgument> Input(std::string_view source) {
    std::vector<PriestMenuArgument> result;std::size_t cursor=1; // 첫 '[' 다음부터 읽는다.
    // ASCII 문자열과 signed 정수를 원문 순서로 읽으며 마지막 빈 문자열도 보존한다.
    while (cursor<source.size() && source[cursor]!=']') {
        if (source[cursor]=='"') {
            ++cursor;std::string value;
            // JSON의 실제 escape만 복원한다. HTML 인용/태그는 해석하지 않는다.
            while (cursor<source.size() && source[cursor]!='"') {
                auto byte=source[cursor++];if (byte=='\\') { byte=source.at(cursor++);if (byte=='n') byte='\n';else if (byte=='r') byte='\r';else if (byte=='t') byte='\t';else CHECK(byte=='"' || byte=='\\'); }value+=byte;
            }
            CHECK(cursor<source.size());++cursor;result.emplace_back(std::move(value));
        } else { const auto end=source.find_first_of(",]",cursor);result.emplace_back(static_cast<std::int32_t>(std::stoi(std::string(source.substr(cursor,end-cursor)))));cursor=end; }
        if (cursor<source.size() && source[cursor]==',') ++cursor;
    }
    CHECK(cursor+1==source.size());return result;
}
// 실제 보호 도움말을 같은 디스크 우선 VFS에서 읽으며 게임/창을 생성하지 않는다.
std::string Help() {
    const auto root=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/"originals";netstorm::o::BaseFileSystem files(root);files.RegisterArchive(root/"netstorm.tarc");const auto bytes=files.Read("d/help.english");return {bytes.begin(),bytes.end()};
}
// native의 명시 고정%s/%d 서식과 같은 계약을 공급한다. 번역/정답 분기는 제품이 수행한다.
std::string Print(std::string_view pattern,std::span<const PriestMenuArgument> arguments) {
    std::string result;std::size_t argument{};
    // 원본의 실제 서식 슬롯만 읽으며 문구/TYPE를 새로 결정하지 않는다.
    for (std::size_t i=0;i<pattern.size();++i) {
        if (pattern[i]!='%') { result+=pattern[i];continue; }CHECK(i+1<pattern.size());const auto code=pattern[++i];
        if (code=='%') { result+='%';continue; }CHECK(argument<arguments.size());const auto& value=arguments[argument++];CHECK(code=='s' || code=='d');result+=code=='s' ? std::get<std::string>(value) : std::to_string(std::get<std::int32_t>(value));
    }
    CHECK(argument==arguments.size());return result;
}
// native와 같은 TYPE 입력/외부 pane·번역·Config/파일 효과이며 파생 HTML/마나는 실제 제품이다.
struct HelpHtmlScene {
    HtmlTextParserState state;HtmlTextParserGlobals globals;std::uint32_t mode{};HtmlLinkTextState links; // 같은 파서/단락/정보/링크 저장소다.
    QuickHelpTypeInfo type;GumpCreateNode pane;std::uint32_t profile{};std::string description{"Unit Label"},trace,stream; // 합성 TYPE와 명시 pane/파일 결과다.
    // 실제 원문에 공급했던 값만 준비하고 표시 문자열/분기 결과는 공급하지 않는다.
    HelpHtmlScene(std::uint32_t value,bool linked,std::uint32_t flags):profile(value) {
        state.shown=state.hiddenDepth=state.inTag=state.textAllowed=state.afterHeading=99;state.inputOffset=37;state.output="prior"; // 실제 기본 생성자가 이전 저장값을 초기화해야 한다.
        links.base=0x13200000;links.enabled=linked;pane.flags=flags;type.theme=static_cast<std::int32_t>(profile%6);type.flags1=profile==7 ? 0 : 0x10;type.flags2=profile==3 ? 0x100 : 0;
        type.classification=profile==4 ? "" : "Class Label";type.mana=profile==2 || profile==5 ? "w.r.t" : "";
        const auto number=[&](float rawValue) { return profile==8 ? Float(0x7fc00000) : profile==5 ? -rawValue : profile==3 || profile==4 ? 0.0f : rawValue; }; // 단정도 입력의 원본 비트다.
        type.range=number(23.75f);type.damage=number(3.25f);type.cost=number(17.75f);
    }
    // 현재 효과의 실제 순서만 기록한다.
    void Event(std::string value) { if (!trace.empty()) trace+=';';trace+=value; }
    // 모든 TYPE getter는 같은 현재 입력을 읽고 pane·stream은 원문 바이트를 받는다.
    QuickHelpHtmlHooks Hooks() {
        QuickHelpHtmlHooks h;h.type=[this]() -> const QuickHelpTypeInfo& { return type; };h.description=[this] { return description; };h.hitPoints=[this] { return profile==6 ? 0 : profile==2 ? 25 : 101; };
        h.translate=h.substitute=h.config=[](std::string_view value) { return std::string(value); };h.print=Print;h.pane=[this] { return &pane; };
        h.paneText=[this](std::string_view value) { Event("Pane,"+Text(value)); };h.paneScroll=[this](auto value) { Event("Scroll,"+std::to_string(value)); };h.panePush=[this] { Event("Push"); };h.paneAttach=[this] { Event("PaneAttach"); };h.stream=[this](std::string_view value) { stream+=value; };return h;
    }
    // 기본/파생이 같은 실제 링크/그림 helper를 사용한다. 중첩 링크 바이트와 부모 cursor도 공유한다.
    std::string Parse(std::string_view source,std::string_view topic,bool streaming) {
        HtmlTextParserHooks base;BindHtmlPictureText(base);BindHtmlLinkText(base,links);QuickHelpHtml html(state,globals,mode,profile ? 1U : 0U,topic,streaming,{},Hooks(),std::move(base),&links);const auto output=html.Parse(source);
        const auto fields=std::to_string(state.inputOffset)+','+std::to_string(state.shown)+','+std::to_string(state.hiddenDepth)+','+std::to_string(state.inTag)+','+std::to_string(state.textAllowed)+','+std::to_string(state.afterHeading)+','+std::to_string(globals.suppressParagraph)+','+std::to_string(mode);
        return Text(output)+'|'+fields+'|'+Hex({reinterpret_cast<const std::uint8_t*>(links.bytes.data()),links.cursor})+'|'+Text(stream)+'|'+trace;
    }
};
}
// 전체83행의 출력/필드·링크 원문·스트림·pane 순서를 대조한다.
TEST_CASE(QuickHelpHtml_Native83) {
    std::size_t count{}; // 실제 대조한 첫 관측 행 수다.
    // 모든 입력은 독립 저장소에서 처리하며 기대값을 다시 만들지 않는다.
    for (const auto& row:netstorm::test::rawscene::LoadFixture(NETSTORM_QUICK_HELP_HTML_FIXTURE).rows) {
        CHECK(row.size()==3);if (row.size()!=3) return;const auto input=Input(row[1]);std::string result;
        const auto number=[&](std::size_t index) { return static_cast<std::uint32_t>(std::get<std::int32_t>(input[index])); }; // 원문 DWORD를 읽는다.
        const auto text=[&](std::size_t index) -> const std::string& { return std::get<std::string>(input[index]); }; // NUL 포함 문자열을 유지한다.
        if (row[0]=="M") result=Text(ManaPictureText(text(0),{},std::get<std::int32_t>(input[1])));
        else if (row[0]=="B") { HtmlTextParserState state;state.inputOffset=1;state.textAllowed=number(1);QuickHelpBeforeClose(state,text(0)+"/");result=std::to_string(state.textAllowed); }
        else if (row[0]=="F") { HelpHtmlScene scene(number(1),number(3)!=0,number(4));result=scene.Parse(Help(),text(0),number(2)!=0); }
        else { CHECK(row[0]=="D");HelpHtmlScene scene(number(2),number(4)!=0,number(5));result=scene.Parse(text(0),text(1),number(3)!=0); }
        if (result!=row[2]) std::printf("Quick Help HTML 원문 차이 %zu %s\n%s\n",count,row[1].c_str(),result.c_str());CHECK(result==row[2]);++count;
    }
    CHECK(count==83);
}
// 중첩 정보의 링크가 같은 바이트를 쓰되 부모 cursor를 전진시키지 않는 원본3행을 대조한다.
TEST_CASE(QuickHelpHtml_NativeNested3) {
    std::size_t count{}; // 실제 대조한 추가 독립 행 수다.
    // description getter만 독립 원문 입력으로 바꾸며 두 파서/링크는 실제 제품이다.
    for (const auto& row:netstorm::test::rawscene::LoadFixture(NETSTORM_QUICK_HELP_HTML_NESTED_FIXTURE).rows) {
        CHECK(row.size()==2);if (row.size()!=2) return;HelpHtmlScene scene(1,true,8);const auto bytes=Bytes(row[0]);scene.description.assign(bytes.begin(),bytes.end());CHECK(scene.Parse("<a name=\"unitType\"><info><a href=\"#outer\">Out</a></a>\nSTOP","unitType",true)==row[1]);CHECK(ResolveHtmlLink(scene.links,scene.links.base)=="outer");++count;
    }
    CHECK(count==3);
}
