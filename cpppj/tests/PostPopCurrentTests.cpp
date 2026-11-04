// 공통후처리의실제순서·현재조회·진단복귀와장부앞부분효과를비멀티명시입력으로대조한다.
#include "TestSupport.h"
#include "o/RawSquidPostPopParent.h"
#include "o/SquidPostPop.h"
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

using namespace netstorm::o;
namespace { //원문외부경계는명시변경/실패입력이며부모분기를대신계산하지않는다.
template<class F> bool PostFailed(F&& action) { try { action(); }catch (const std::exception&) { return true; }return false; } //도달한하위실패뒤부분효과를검사한다.
struct PostParentScene { //같은슬롯과현재188TYPE를callback사이에도유지한다.
    SidPool pool{OriginalEdition::Patch1078,24000,false}; //네트워크를열지않는실제client풀이다.
    Sid sid{pool.Allocate(2)}; //한객체의실제물리슬롯이다.
    std::array<RiftTypeRecord,188> types{}; //현재TYPE필드는주소를바꾸지않고수정한다.
    bool suppressed{},graphs{}; //현재메타맵/그래프전역입력이다.
    std::uint32_t depth{2},owner{1},graphReads{},typeReads{}; //깊이와현재로컬owner/조회관측이다.
    std::vector<std::string> events; //실제로호출된하위/번호/순서다.
    std::function<void(std::string_view)> change; //해당외부경계의명시효과나중단이다.
    SquidPostPopParentHooks hooks; //실제제품부모에공급하는경계다.
    std::span<std::uint8_t> Raw() { const auto data=pool.Slot(sid);return {const_cast<std::uint8_t*>(data.data()),data.size()}; } //free변경뒤에도동일한물리를읽는다.
    void Event(std::string value) { events.push_back(value);const auto action=change;if (action) action(value); } //자기연결교체도현재호출을보존한다.
    PostParentScene() { //부모가도달한경계만인자와현재값을기록한다.
        Raw()[10]=74;Raw()[34]=1;
        hooks.suppressed=[this] { return suppressed; };hooks.graphsEnabled=[this] { ++graphReads;return graphs; };
        hooks.type=[this](std::uint8_t type)->const RiftTypeRecord& { ++typeReads;if (type>=types.size()) throw std::out_of_range("명시현재TYPE밖");return types[type]; };
        hooks.region=[this](Sid value) { CHECK(value==sid);Event("region"); };hooks.add=[this](Sid value) { CHECK(value==sid);Event("add"); };hooks.resetGraph=[this](Sid value) { CHECK(value==sid);Event("reset"); };
        hooks.assertion=[this](auto expression,auto file,auto line) { CHECK(expression=="!(popFlags&pfDESTROY_GRAPH)" && file=="\\Ns\\O\\Squid.cpp" && line==0x80b);Event("assert"); };
        hooks.cost=[this](Sid) { Event("cost"); };hooks.provider=[this](Sid) { Event("provider"); };hooks.factory=[this](Sid) { Event("factory"); };
        hooks.ownerFactory=[this](auto current,Sid) { Event("owner:"+std::to_string(current)); };hooks.aiNotice=[this](auto current,Sid) { Event("ai:"+std::to_string(current)); };
        hooks.localOwner=[this] { return owner; };hooks.localCount=[this](auto type) { Event("local:"+std::to_string(type)); };hooks.globalCount=[this](auto type) { Event("global:"+std::to_string(type)); };
        hooks.recheckSelection=[this] { Event("selection"); };
    }
    void Run(std::uint32_t flags) { RunSquidPostPopParent(pool,sid,flags,depth,hooks); } //호출자가증가해둔깊이를공통몸체가한번줄인다.
};
}

TEST_CASE(PostPopCurrent_SuppressedReturnsBeforeSlotOrTypeAndWrapsDepth) { //메타맵은범위밖SID/TYPE나미연결장치를조회하지않는다.
    PostParentScene scene;scene.suppressed=true;scene.sid={65535};scene.depth=0;scene.hooks.type={};scene.hooks.graphsEnabled={};scene.Run(1);CHECK(scene.depth==std::numeric_limits<std::uint32_t>::max() && scene.typeReads==0 && scene.events.empty());
}
TEST_CASE(PostPopCurrent_RegionDisabledBranchDoesNotRecheckGraphs) { //최초genus가영역이고활성이0이면그다음그래프재조회도생략한다.
    PostParentScene scene;scene.types[74].flags2=8;scene.hooks.graphsEnabled=[&] { return ++scene.graphReads>1; };scene.Run(0x200);CHECK(scene.graphReads==1 && scene.events.empty() && scene.depth==1);
}
TEST_CASE(PostPopCurrent_RegionCallbackDisablesAddBeforeLedger) { //영역하위가바꾼현재활성을다시읽고Add를생략한다.
    PostParentScene scene;scene.graphs=true;scene.types[74].flags2=8;scene.types[74].flags1=0x800;scene.change=[&](auto event) { if (event=="region") scene.graphs=false; };scene.Run(3);CHECK(scene.graphReads==2 && scene.events==(std::vector<std::string>{"region","cost","ai:1","local:74","global:74","selection"}) && scene.depth==1);
}
TEST_CASE(PostPopCurrent_DestroyDiagnosticResumesWithCurrentType) { //checking이없어도80b정상복귀뒤현재TYPE의표면분기를선택한다.
    PostParentScene scene;scene.graphs=true;scene.change=[&](auto event) { if (event=="assert") { scene.Raw()[10]=75;scene.types[75].flags1=0x800; } };scene.Run(0xa00);CHECK(scene.events==(std::vector<std::string>{"assert","add"}) && scene.depth==1);
}
TEST_CASE(PostPopCurrent_AbortingDiagnosticKeepsDepthAndEarlierRegion) { //진단중단은먼저실행한영역효과를지우거나깊이를끝내지않는다.
    PostParentScene scene;scene.graphs=true;scene.types[74].flags2=8;scene.change=[](auto event) { if (event=="assert") throw std::logic_error("명시80b중단"); };CHECK(PostFailed([&] { scene.Run(0x801); }) && scene.depth==2 && scene.events==(std::vector<std::string>{"region","assert"}));
}
TEST_CASE(PostPopCurrent_NoGraphSurfaceUsesResetBeforeCost) { //noGraph의현재표면번호리셋은장부앞이며Add/영역은호출하지않는다.
    PostParentScene scene;scene.graphs=true;scene.types[74].flags1=0x800;scene.Run(0x2000001);CHECK(scene.events==(std::vector<std::string>{"reset","cost","ai:1","local:74","global:74","selection"}) && scene.depth==1);
}
TEST_CASE(PostPopCurrent_CurrentTypeAndSavedGenusHaveIndependentRoles) { //각외부효과가TYPE/owner를바꾸며공장분기만그래프뒤저장genus를쓴다.
    PostParentScene scene;scene.graphs=true;scene.types[74].flags2=8;
    scene.change=[&](auto event) {
        if (event=="region") { scene.Raw()[10]=75;scene.types[75].flags1=0x800; }
        else if (event=="add") { scene.Raw()[10]=76;scene.types[76].flags2=0x4200; }
        else if (event=="cost") { scene.Raw()[10]=77;scene.types[77].flags1=0x10000000; }
        else if (event=="provider") { scene.Raw()[10]=78;scene.Raw()[34]=3; }
        else if (event=="owner:3") scene.Raw()[34]=4;
        else if (event=="factory") scene.Raw()[34]=5;
        else if (event=="ai:5") { scene.Raw()[34]=6;scene.owner=6;scene.Raw()[10]=255; }
        else if (event=="local:255") scene.Raw()[10]=254;
    };
    scene.Run(3);CHECK(scene.events==(std::vector<std::string>{"region","add","cost","provider","owner:3","factory","ai:5","local:255","global:254","selection"}) && scene.depth==1);
}
TEST_CASE(PostPopCurrent_AbstractExtraSkipsLedgerButStillFinishesGraph) { //현재extra1/8/9는그래프실행뒤장부만생략한다.
    for (const auto extra:{1U,8U,9U}) { PostParentScene scene;scene.graphs=true;scene.types[74].flags1=0x800;scene.Raw()[40]=static_cast<std::uint8_t>(extra);scene.Run(1);CHECK(scene.events==(std::vector<std::string>{"add"}) && scene.depth==1); } //세실제비트입력을별도로확인한다.
}
TEST_CASE(PostPopCurrent_SelectionFailureKeepsAllEarlierStatisticsAndDepth) { //마지막선택하위실패도이미기록한통계와현재깊이를되돌리지않는다.
    PostParentScene scene;scene.change=[](auto event) { if (event=="selection") throw std::logic_error("명시선택하위실패"); };CHECK(PostFailed([&] { scene.Run(1); }) && scene.depth==2 && scene.events==(std::vector<std::string>{"cost","ai:1","local:74","global:74","selection"}));
}
TEST_CASE(PostPopCurrent_ActualManaDiagnosticRetainsSavedFactoryGenusAndCurrentOwner) { //실제중복진단뒤현재TYPE/owner와앞서저장한공장군을같은장부에서구별한다.
    PostParentScene scene;scene.types[74].flags1=0x10000000;scene.types[74].flags2=0x4200;scene.types[74].cost=3;SquidPostPopState state;state.depth=1;state.localOwner=1;state.providers.entries={scene.sid.value};state.providers.count=1;state.factories.entries.resize(1);state.ownerFactories[3].entries.resize(1);
    SquidPostPop post(scene.pool,scene.types,state);post.BindCurrentTypes(scene.types);std::uint32_t diagnoses{},rechecks{};post.BindPatchLocalOwner([] { return 3U; });post.BindManaDiagnostics({RawCheckingFlag{1},[&](auto expression,auto file,auto line) { CHECK(expression=="!alreadyThere" && file=="\\Ns\\O\\Mana.cpp" && line==0x44 && state.totalCost==3);++diagnoses;scene.Raw()[10]=75;scene.Raw()[34]=3; }});
    post.BindPatchSelectionRecheck([&] { ++rechecks;CHECK(state.globalCounts[75]==1 && state.localCounts[75]==1 && state.depth==1); });post.PostPopBase(scene.sid,1);
    CHECK(diagnoses==1 && rechecks==1 && state.providers.count==1 && state.factories.count==1 && state.ownerFactories[3].count==1 && state.productionDirty==2 && state.depth==0);
}

TEST_CASE(PostPopCurrent_OwnerReadPrecedesCurrentLocalOwnerGetter) { //원문두mov의순서를C++비교피연산자의미정평가순서에맡기지않는다.
    PostParentScene scene;scene.hooks.localOwner=[&] { scene.Raw()[34]=2;return 1U; };scene.Run(1);
    CHECK(scene.Raw()[34]==2 && scene.events==(std::vector<std::string>{"cost","ai:1","local:74","global:74","selection"}) && scene.depth==1);
}
