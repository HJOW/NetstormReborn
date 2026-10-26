// 내장 설명 조회/HtmlBody 부모의 원본82행과 실제 임시 Config 수명을 검증한다.
#include "RawSceneSupport.h"
#include "client/EmbeddedTell.h"
#include "client/HtmlBodyGump.h"
#include "o/ConfigInterface.h"
#include <cstdio>

using namespace netstorm::client;
using namespace netstorm::test::rawscene;
namespace { // Native 입력의 좁은 바이트와 전체 영역 지문만 변환한다.
std::string EmbeddedUnhex(std::string_view value) { std::string result;for (std::size_t offset=0;offset<value.size();offset+=2) result+=static_cast<char>(std::stoul(std::string(value.substr(offset,2)),nullptr,16));return result; } // 두16진 문자를 한 원문 BYTE로 되돌린다.
std::string EmbeddedHex(std::string_view value) { return Hex({reinterpret_cast<const std::uint8_t*>(value.data()),value.size()}); } // 한국어/로케일 변환 없이 관측값을 만든다.
std::uint32_t EmbeddedFingerprint(std::span<const char> bytes) { std::uint32_t result=2166136261U;for (const unsigned char value:bytes) result=(result^value)*16777619U;return result; } // NUL 뒤8192BYTE도 실제 Native 지문에 포함한다.
}

// Config/파서/다중 줄/가상 장치만 명시하고 제품 부모의 현재 저장소와 순서를 비교한다.
TEST_CASE(EmbeddedTell_ReplaysNativeQueriesBodyInstallAndSerial) {
    const auto fixture=LoadFixture(NETSTORM_EMBEDDED_TELL_FIXTURE);std::uint32_t checked{};
    for (const auto& row:fixture.rows) { // 두 x87 정밀도의 모든 원문 관측을 재생한다.
        if (row[0]=="op") continue;
        const auto mask=Number(row[1]);const auto key=EmbeddedUnhex(row[2]),source=EmbeddedUnhex(row[3]);HtmlBodyGumpNode node;node.body.fill('Z');node.links.fill('Z');GumpCreateNode child,oldChild;std::vector<std::string> trace;
        GumpCreateState create;create.collection.edition=OriginalEdition::Patch1078;HtmlBodyGumpState state{640,480,20,7};const auto base=[&](auto token)->GumpCreateNode* { return token==1 ? &node : token==2 ? &child : token==3 ? &oldChild : nullptr; };GumpCreate factory(create,{base,{}});
        HtmlBodyGumpHooks hooks;hooks.node=[&](auto token) { return token==1 ? &node : nullptr; };hooks.base=base;hooks.constructBase=[&](auto token,auto command,float width,float height,auto flags) { factory.Construct(token,command,width,height,flags); };
        hooks.allocate=[&](auto bytes) { CHECK(bytes==0xd8);trace.push_back("allocateText");return 2U; };hooks.constructText=[&](auto token,auto width,auto height,auto font,auto color) { CHECK(token==2);trace.push_back("constructText:"+std::to_string(width)+":"+std::to_string(height)+":"+std::to_string(font)+":"+std::to_string(color)); };
        hooks.kill=[&](auto) { trace.push_back("kill"); };hooks.push=[&](auto token) { trace.push_back("push");base(token)->flags|=8; };
        hooks.attach=[&](auto token,float x,float y,auto,auto mode) { trace.push_back("attach:"+std::string(token==1 ? "parent" : "text")+":"+std::to_string(std::bit_cast<std::uint32_t>(x))+":"+std::to_string(std::bit_cast<std::uint32_t>(y))+":"+std::to_string(mode));base(token)->x=x;base(token)->y=y; };
        hooks.styled=[&](auto,std::string_view text) { trace.push_back("styled:"+EmbeddedHex(text)); };hooks.scroll=[&](auto,auto value) { trace.push_back("scroll:"+std::to_string(value)); };hooks.slider=[&](auto,auto value) { trace.push_back("slider:"+std::to_string(value)); };hooks.scrollable=[&](auto,auto value) { trace.push_back("scrollable:"+std::to_string(value)); };HtmlBodyGump body(state,std::move(hooks));
        const auto parse=[&](std::string_view input,std::span<char> output,std::span<char> links) { input=input.substr(0,input.find('\0'));CHECK(output[0]==0 && links[0]==0);trace.push_back("parse:"+EmbeddedHex(input));const auto value="["+std::string(input)+"]";std::copy(value.begin(),value.end(),output.begin());output[value.size()]=0;std::copy_n("link",5,links.begin());if (mask&32) node.serial=9001; }; // 명시 Native 파서 장치와 같은 물리 출력만 공급한다.
        std::uint32_t returned{};
        if (row[0]=="SetHtml") { body.Construct(1);node.x=2.25F;node.y=-3.5F;if (mask&16) node.text=3;if (mask&8) node.flags&=~8U;trace.clear();body.SetHtml(1,source,parse); }
        else {
            EmbeddedTellHooks tell;if (mask&1) tell.mission=[&](auto name)->std::optional<std::string> { trace.push_back("mission:"+EmbeddedHex(name));return mask&2 ? std::optional(source) : std::nullopt; };
            tell.fallback=[&](auto name)->std::optional<std::string> { trace.push_back("fallback:"+EmbeddedHex(name));return mask&4 ? std::optional(source) : std::nullopt; };tell.missing=[&](auto name) { trace.push_back("missing:"+EmbeddedHex(name)); };
            tell.construct=[&] { trace.push_back("new");return body.Construct(1); };tell.setHtml=[&](auto token,auto input) { body.SetHtml(token,input,parse); };tell.serial=[&](auto token) { CHECK(token==1);return node.serial; };returned=ShowEmbeddedTell(key,tell);
        }
        std::string joined;for (const auto& entry:trace) { if (!joined.empty()) joined+=';';joined+=entry; } // 실제 호출 순서를 그대로 비교한다.
        const bool same=returned==Number(row[5]) && node.flags==Number(row[6]) && node.vtable==Number(row[7]) && node.serial==Number(row[8]) && static_cast<std::uint32_t>(node.text!=0)==Number(row[9]) && EmbeddedFingerprint(node.body)==Number(row[10]) && EmbeddedFingerprint(node.links)==Number(row[11]) && joined==row[12];
        if (!same) std::fprintf(stderr,"EmbeddedTell Native행%u %s 불일치: %s\n",checked,row[0].c_str(),joined.c_str());CHECK(same);++checked;
    }
    CHECK(checked==82);
}

// 내장 TELL은 실제 새 파일을 읽되 UI 생성 전에 임시 설정을 반납하고 팝업 이력을 바꾸지 않는다.
TEST_CASE(EmbeddedTell_ActualConfigPriorityFreshFallbackAndNoPopupParameters) {
    ConfigInterface* active{};std::size_t reads{},objectsDuring{};std::string version="First";
    ConfigInterface config([&](auto path)->std::optional<std::vector<std::uint8_t>> { CHECK(path=="d/tell.english");++reads;objectsDuring=active->Registry().Objects().size();const auto text="[PlacingIslands]\n"+version+"\n[BLANK]\n[END]\n";return std::vector<std::uint8_t>(text.begin(),text.end()); });active=&config;config.Configuration().Assign("missionSpec=d/{local.1}.english\n");config.Set("global.priorTell","untouched");Config old(config.Registry(),"tell");old.Assign("[PlacingIslands]\nCached\n[END]\n");Config named(config.Registry(),"file");named.Assign("[part]\nWrong prefix\n[END]\n");Config param(config.Registry(),"param");param.Assign("1=outer\n");const auto count=config.Registry().Objects().size();std::string installed;std::uint32_t constructed{};
    EmbeddedTellHooks hooks;hooks.construct=[&] { CHECK(config.Registry().Objects().size()==count && config.Registry().FindByName("tell")==&old);return ++constructed; };hooks.setHtml=[&](auto,std::string_view text) { installed=text; };hooks.serial=[](auto token) { return token+700U; };bool missing{};hooks.missing=[&](auto key) { CHECK(key=="file.part");missing=true; };
    CHECK(ShowEmbeddedTellInConfig("PlacingIslands",config,{},hooks)==701 && installed=="First\n" && reads==1 && objectsDuring==count+1);
    version="Second";CHECK(ShowEmbeddedTellInConfig("PlacingIslands",config,{},hooks)==702 && installed=="Second\n" && reads==2);
    CHECK(ShowEmbeddedTellInConfig("BLANK",config,{},hooks)==703 && installed.empty() && reads==3);
    CHECK(ShowEmbeddedTellInConfig("file.part",config,[](auto key)->std::optional<std::string> { CHECK(key=="file.part");return "Mission"; },hooks)==704 && installed=="Mission" && reads==3);
    CHECK(ShowEmbeddedTellInConfig("file.part",config,{},hooks)==0 && missing && reads==4 && constructed==4);
    CHECK(config.Registry().Objects().size()==count && config.Registry().FindByName("tell")==&old && config.SubstituteLocal("{param.1}")=="outer" && config.SubstituteLocal("{global.priorTell}")=="untouched");
}
