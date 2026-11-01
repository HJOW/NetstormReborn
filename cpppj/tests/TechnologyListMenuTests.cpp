// 실제 제단/희생/생산 없이 원문 기술 필터·40칸 목록과 HTML 원소별 호출을 대조한다.
#include "RawSceneSupport.h"
#include "client/TechnologyListMenu.h"
#include "client/HtmlWindowMetadata.h"
#include <bit>
#include <filesystem>
#include <stdexcept>
using namespace netstorm::client;
using namespace netstorm::test::rawscene;

namespace { //독립 타입 입력/경계 기록이며 실제 공중3종이나 멀티플레이 행위를 실행하지 않는다.
std::vector<std::uint32_t> TechnologyNumbers(std::string_view text) { std::vector<std::uint32_t> result;for (const auto& value:Split(text,',')) result.push_back(Number(value));return result; } //signed 입력도 원시 하위DWORD로 복원한다.
struct TechnologyScene { //현재 TYPE 표와 원문 명시 경계를 각 입력에서 새로 설치한다.
    std::array<RiftTypeRecord,80> types{};std::vector<std::uint32_t> row;std::string trace; //독립 실제 표/이번 입력/경계 순서다.
    void Event(std::string_view name,std::initializer_list<std::uint32_t> values={}) { if (!trace.empty()) trace+=';';trace+=name;for (auto value:values) trace+=':'+std::to_string(value); } //원문 trace와 같은 순서/전체 DWORD를 기록한다.
    void Prepare(std::vector<std::uint32_t> input) { row=std::move(input);trace.clear();for (std::uint32_t type=0;type<types.size();++type) { types[type].level=static_cast<std::int32_t>(type%4)-1;types[type].theme=static_cast<std::int32_t>(type%6);types[type].group=type%11 ? 0 : 10; } } //모든 기대값과 독립적으로 원시 TYPE만 초기화한다.
    TechnologyListMenuHooks Hooks() { //필터/목록 본체는 제품에서 실행하고 명시 장치만 공급한다.
        TechnologyListMenuHooks hooks;hooks.typeCount=[this] { return static_cast<std::int32_t>(row[3]); };hooks.type=[this](auto type)->const RiftTypeRecord& { return types.at(type); };hooks.priestType=[] { return 5U; };hooks.templeProduct=[] { return 9U; };hooks.currentTechnology=[this] { Event("T");return 50U; };hooks.level=[this](auto target) { CHECK(target==10);Event("L",{target});return row[2]+1U; };hooks.owns=[this](auto parent,auto type) { Event("O",{parent,type});return row[4] && type%3==1 ? type : 0U; };
        hooks.translate=[this](auto text) { CHECK(text=="%s (Level %d)");Event("X");return std::string(text); };hooks.description=[this](auto type) { Event("D",{type});if (row[6]) types[type].level=7;return "T"+std::to_string(type); };
        hooks.format=[this](auto pattern,auto description,auto level) { CHECK(pattern=="%s (Level %d)");Event("F",{static_cast<std::uint32_t>(std::stoul(std::string(description.substr(1)))),static_cast<std::uint32_t>(level)});return std::string(description)+" (Level "+std::to_string(level)+')'; };
        hooks.learnCommand=[] { return 0x020000d5U; };hooks.item=[this](auto label,auto command,auto type,bool enabled) { CHECK(label.starts_with("T"));Event("I",{command,type,enabled ? 1U : 0U}); };hooks.assertion=[this](auto line) { Event("A",{line}); };return hooks;
    }
};
}
//원문367입력의 전체 반환/저장개수/40DWORD와 메뉴 경계 순서를 대조한다.
TEST_CASE(TechnologyListMenu_IndependentNative) {
    const auto root=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path();const auto fixture=LoadFixture((root/"cpppj/tests/fixtures/technology-list-menu-x86.tsv").string().c_str());CHECK(!fixture.rows.empty());TechnologyScene scene;std::size_t count{};
    for (const auto& fields:fixture.rows) { //제품 계산으로 원문 기대 fixture를 수정하지 않는다.
        CHECK(fields.size()==3);if (fields.size()!=3) continue;scene.Prepare(TechnologyNumbers(fields[0]));CHECK(scene.row.size()==7);TechnologyListMenu menu(scene.Hooks());std::array<std::uint32_t,40> output;output.fill(0xa5a5a5a5U);std::int32_t stored{},result{};const auto theme=std::bit_cast<std::int32_t>(scene.row[1]),level=std::bit_cast<std::int32_t>(scene.row[2]);
        if (scene.row[0]==2) menu.AppendTheme(10,theme);else if (!scene.row[0]) result=menu.CountKnown(std::span(output).first(scene.row[5]),stored,theme,level);else result=menu.CountCandidates(std::span(output).first(scene.row[5]),stored,theme,level);
        std::vector<std::uint32_t> current{static_cast<std::uint32_t>(result),static_cast<std::uint32_t>(stored)};current.insert(current.end(),output.begin(),output.end());const auto expected=TechnologyNumbers(fields[2]);const auto trace=scene.trace.empty() ? "-" : scene.trace;
        if (trace!=fields[1] || current!=expected) std::fprintf(stderr,"기술 목록 원문 입력 %s / 기대 %s / 실제 %s\n",fields[0].c_str(),fields[1].c_str(),trace.c_str());CHECK(trace==fields[1]);CHECK(current==expected);++count;
    }
    CHECK(count==367 && count==fixture.rows.size()); //고정 용량 넘침과 명시 설명 변경 입력까지 모두 재생한다.
}
//실제 HTML 메타가 packed SID/원소를 해석해 같은 생성기와 현재 메뉴 열을 사용한다.
TEST_CASE(TechnologyListMenu_HtmlMetadataThemeDispatch) {
    TechnologyScene scene;scene.Prepare({2,1,2,80,1,40,0});HtmlWindowGumpNode node;HtmlWindowMetadataHooks hooks;hooks.node=[&](auto token) { CHECK(token==1);return &node; };hooks.clearButtons=[](auto) {};hooks.commands.lookup=[](auto name) { CHECK(name=="DoNothing");return 0x02000089U; };hooks.beginMenu=[](auto flags) { CHECK(flags==0x40000); };
    hooks.themeTech=[&](auto target,auto theme) { CHECK(target==10 && theme==1);TechnologyListMenu(scene.Hooks()).AppendTheme(target,theme); };std::array<char,8192> buffer{};const std::string text="$Menu=*THEMETECHLIST*,DoNothing,65546\nTAIL";std::copy(text.begin(),text.end(),buffer.begin());
    CHECK(HtmlWindowMetadata(hooks).Process(1,buffer)==1 && node.menuColumns==1);CHECK(std::string(buffer.data())=="TAIL" && scene.trace.starts_with("L:10;T") && scene.trace.find("I:33554645:")!=std::string::npos);
}
//NULL 목록은 전체 수집 없이 count를 반환하고 후보 helper는 소유 getter를 요구하지 않는다.
TEST_CASE(TechnologyListMenu_LazyBoundariesAndPartialFailure) {
    std::uint32_t reads{};TechnologyListMenuHooks hooks;hooks.currentTechnology=[&] { ++reads;return 50U; };hooks.typeCount=[] { return 0; };hooks.level=[](auto) { return 0U; };std::int32_t stored=7;TechnologyListMenu menu(hooks);CHECK(menu.CountCandidates({},stored,0,-1)==0 && stored==7 && reads==1);menu.AppendTheme(10,1);CHECK(reads==4);
    TechnologyScene scene;scene.Prepare({2,1,2,80,1,40,0});auto actual=scene.Hooks();actual.item={};bool failed{};try { TechnologyListMenu(actual).AppendTheme(10,1); }catch (const std::logic_error&) { failed=true; }CHECK(failed && scene.trace.find(";X;D:")!=std::string::npos && scene.trace.find(";F:")!=std::string::npos && scene.trace.find(";I:")==std::string::npos);
}
