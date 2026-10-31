// 원문0044cf20의정적계약만명시입력으로검사하며Nativefixture/게임을실행하지않는다.
#include "TestSupport.h"
#include "o/RawDataManagerDump.h"
#include "RawSceneSupport.h"
#include <algorithm>
#include <array>
#include <stdexcept>
#include <string>
#include <vector>
using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace { // 실제276BYTE와독립노드/이름표/trace관측의수명을같이소유한다.
struct DumpScene { // 결과를주입하지않고가상호출/CRT비교/로그경계만명시입력으로연결한다.
    std::array<std::uint8_t,284> bytes{}; // manager양쪽4BYTE를포함하며current밖의쓰기를탐지한다.
    std::array<std::array<std::uint8_t,60>,3> nodes{}; // 원본data/pos/end/alloc/name/next의같은물리다.
    std::vector<std::uint8_t> names; // 원본SNART제자리변환과미래이름변경의현재버퍼다.
    std::vector<std::string> events,lines; // 가상호출/비교/trace순서와완성출력을각각기록한다.
    DataManagerDumpHooks hooks; // 이장면보다오래살지않는실제주소대여장치다.
    // 검사입력의이름/끝길이를설치하며source버퍼내용은덤프가읽지않도록두지않는다.
    DumpScene() {
        bytes.fill(0xa5);Put(Manager(),4,1);Put(Manager(),8,99);
        // 세독립노드는입력순서대로다음링크를가지며pos/alloc은끝합계에영향을주지않아야한다.
        for (std::uint32_t index=0;index<3;++index) { Put(nodes[index],0,0xdead0000U+index);Put(nodes[index],4,17+index);Put(nodes[index],8,index+1);Put(nodes[index],12,100+index);Put(nodes[index],56,index<2 ? index+2 : 0); }
        NodeName(1,"One");NodeName(2,"Two");NodeName(3,"Three");Names("/One/Two/Three//");
        hooks.fileName=[this](auto manager) { CHECK(manager.data()==Manager().data());events.emplace_back("file");return std::string_view("NONAME"); };
        hooks.names=[this](auto manager) { CHECK(manager.data()==Manager().data());events.emplace_back("names");return std::span(names); };
        hooks.selection.stream=[this](auto token) { if (!token || token>nodes.size()) throw std::out_of_range("덤프검사의실제노드번호");return std::span(nodes[token-1]); };
        hooks.selection.compare=[this](auto first,auto second) { events.emplace_back("compare:"+std::string(first)+":"+std::string(second));return CompareDataManagerSectionNames(first,second); };
        hooks.trace=[this](auto level,auto text) { CHECK(level==29);events.emplace_back("trace");lines.emplace_back(text); };
        hooks.separatorAssertion=[] { throw std::logic_error("정상덤프이름표에없는separator진단"); };
    }
    // 같은manager의보호구간을제외한276BYTE를동기호출동안빌린다.
    std::span<std::uint8_t> Manager() { return std::span(bytes).subspan(4,276); }
    // 실제40BYTE이름공간의첫NUL까지복사하는검사입력작성기다.
    void NodeName(std::uint32_t token,std::string_view name) { std::fill(nodes.at(token-1).begin()+16,nodes.at(token-1).begin()+56,std::uint8_t{});std::copy(name.begin(),name.end(),nodes.at(token-1).begin()+16); }
    // mutableSNART의최초슬래시문자열과마지막NUL을함께소유한다.
    void Names(std::string_view text) { names.assign(text.begin(),text.end());names.push_back(0); }
};
// 선택된명시예외만문자열로반환하며제품의부분current/이름표상태는되돌리지않는다.
template<class Function> std::string DumpFailure(Function function) { try { function(); }catch (const std::exception& error) { return error.what(); }return {}; }
}

TEST_CASE(DataManagerDump_NormalOrderAndPhysicalCurrent) { //가상파일명/첫로그/가상이름표후실제검색하며pos와양쪽보호BYTE를보존한다.
    DumpScene scene;const auto before=scene.bytes;const auto nodes=scene.nodes;DumpDataManager(scene.Manager(),"Stripped Image:",scene.hooks);
    CHECK(scene.lines==(std::vector<std::string>{"File NONAME: Stripped Image:\n","0006 bytes total.\n"}));CHECK(scene.events.front()=="file" && scene.events[1]=="trace" && scene.events[2]=="names" && scene.events.back()=="trace");
    CHECK(Get(scene.Manager(),8)==3 && scene.nodes==nodes && scene.names[0]==0 && scene.names[4]==0 && scene.names.back()==0xff);
    // 검색이쓰는current4BYTE이외의실제manager와양쪽보호BYTE를비교한다.
    for (std::size_t index=0;index<scene.bytes.size();++index) if (index<12 || index>=16) CHECK(scene.bytes[index]==before[index]);
}

TEST_CASE(DataManagerDump_CurrentAndFutureNamesFollowCallbackChanges) { //비교뒤current와누락로그뒤미래이름표/길이의같은현재값을읽는다.
    DumpScene scene;scene.Names("/One/Missing/Three//");std::size_t comparisons{},namesCalls{};
    scene.hooks.fileName=[&](auto manager) { Put(manager,8,88);scene.events.emplace_back("file");return std::string_view("changed.fort\0ignored",20); };
    scene.hooks.names=[&](auto) { ++namesCalls;CHECK(scene.lines.size()==1);return std::span(scene.names); };
    scene.hooks.selection.compare=[&](auto first,auto second) { if (!comparisons++) { CHECK(first=="One" && second=="One");Put(scene.Manager(),8,3);return 0; }return CompareDataManagerSectionNames(first,second); };
    scene.hooks.trace=[&](auto level,auto text) {
        CHECK(level==29);scene.lines.emplace_back(text);
        if (scene.lines.size()==1) CHECK(Get(scene.Manager(),8)==88);
        if (text=="ERROR: Missing section Missing\n") { CHECK(!Get(scene.Manager(),8));const auto future=SelectSnartName(scene.names,2,{});std::copy_n("Other",5,future.begin());scene.NodeName(2,"Other");Put(scene.nodes[1],8,27); }
    };
    DumpDataManager(scene.Manager(),std::nullopt,scene.hooks);CHECK(namesCalls==1 && scene.lines==(std::vector<std::string>{"File changed.fort: \n","ERROR: Missing section Missing\n","0030 bytes total.\n"}) && Get(scene.Manager(),8)==2);
}

TEST_CASE(DataManagerDump_DwordWrapAndSignedMinimumWidth) { //end합계는signed연산오버플로없이원문DWORD로감기고signed%04d로표시한다.
    DumpScene scene;scene.Names("/One/Two//");Put(scene.nodes[0],8,0xffffffffU);Put(scene.nodes[1],8,2);DumpDataManager(scene.Manager(),"\0ignored",scene.hooks);CHECK(scene.lines.back()=="0001 bytes total.\n" && Get(scene.Manager(),8)==2);
    scene.lines.clear();scene.Names("/One//");Put(scene.nodes[0],8,0x80000000U);DumpDataManager(scene.Manager(),std::nullopt,scene.hooks);CHECK(scene.lines.back()=="-2147483648 bytes total.\n");
    scene.lines.clear();Put(scene.nodes[0],8,0xffffffffU);DumpDataManager(scene.Manager(),std::nullopt,scene.hooks);CHECK(scene.lines.back()=="-001 bytes total.\n");
}

TEST_CASE(DataManagerDump_SelectedFailurePreservesPartialState) { //separator/비교/로그예외는이미쓴current와SNART변환을보존한다.
    DumpScene scene;scene.Names("XOne//");scene.hooks.separatorAssertion=[] { throw std::runtime_error("명시separator중단"); };CHECK(DumpFailure([&] { DumpDataManager(scene.Manager(),std::nullopt,scene.hooks); })=="명시separator중단" && Get(scene.Manager(),8)==99 && scene.names[0]=='X' && scene.lines.size()==1);
    scene.Names("/One//");scene.hooks.selection.compare=[&](auto,auto)->int { Put(scene.Manager(),8,2);throw std::runtime_error("명시비교중단"); };CHECK(DumpFailure([&] { DumpDataManager(scene.Manager(),std::nullopt,scene.hooks); })=="명시비교중단" && Get(scene.Manager(),8)==2 && scene.names[0]==0);
    scene.hooks.selection.compare={};scene.Names("/Missing//");scene.hooks.trace=[&](auto,auto text) { if (text.starts_with("ERROR:")) { Put(scene.Manager(),8,3);throw std::runtime_error("명시trace중단"); } };CHECK(DumpFailure([&] { DumpDataManager(scene.Manager(),std::nullopt,scene.hooks); })=="명시trace중단" && Get(scene.Manager(),8)==3);
}
