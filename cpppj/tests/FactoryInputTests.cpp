// Factory 사용자 입력의 자금/진행·현재 전역·위임 순서 검사다. 작성·컴파일만 한다.
#include "o/RawFactoryInput.h"
#include "RawSceneSupport.h"
#include <limits>
using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 실제 raw Factory와 명시 비용/SP/예약·UI 경계를 구성한다.
struct FactoryInputScene {
    SidPool pool{OriginalEdition::Patch1078,32768,true};Sid factory{}; //현재같은실제원문풀과부모다.
    std::uint32_t editor{},owner{1},level{},stored{}; //현재모드/로컬소유자/가상레벨/기록레벨이다.
    FactoryInputState state{editor,owner};FactoryInputHooks hooks; //원본명령전역과선택하위다.
    std::int32_t cost{500};float money{500};bool upgrading{}; //복호화정수비용/SP비트/현재예약입력이다.
    std::vector<std::string> trace;std::vector<std::string> tells; //실제하위호출순서와Tell섹션이다.
    // 각 분기의 관찰만 준비하며 판단을 미리 계산하지 않는다.
    FactoryInputScene() {
        factory=pool.Allocate(2);auto raw=pool.AllocatedBytes(factory);std::fill(raw.begin(),raw.end(),std::uint8_t{});Put(raw,0,0x509148);raw[10]=85;raw[34]=2;
        hooks.level=[this](Sid sid) { CHECK(sid==factory);trace.push_back("level");return level; };hooks.setLevel=[this](Sid sid,auto value) { CHECK(sid==factory);stored=value;trace.push_back("set"); };hooks.adjustFrame=[this](Sid sid) { CHECK(sid==factory);trace.push_back("frame"); };
        hooks.cost=[this](auto type) { CHECK(type==85);trace.push_back("cost");return cost; };hooks.money=[this](auto local) { CHECK(local==owner);trace.push_back("money");return money; };hooks.upgrading=[this](Sid sid) { CHECK(sid==factory);trace.push_back("upgrading");return upgrading; };hooks.request=[this](Sid sid) { CHECK(sid==factory);trace.push_back("request"); };hooks.tell=[this](std::string_view section) { tells.emplace_back(section);trace.push_back("tell"); };
        hooks.research=[this](Sid sid,auto code,auto parameter) { CHECK(sid==factory && code==0xff000080U && parameter==0xabcd0005U);trace.push_back("research");return 17U; };hooks.parent=[this](Sid sid,auto code,auto parameter) { CHECK(sid==factory && code==0xff0000afU && parameter==0xabcd0005U);trace.push_back("parent");return 27U; };
    }
    // 같은 현재 상태/풀과 선택 효과를 빌린다.
    RawFactoryInput Make() { return RawFactoryInput(pool,state,hooks); }
    // 상위 수정키가 있는 같은 실제 업그레이드 명령이다.
    std::uint32_t Upgrade() { return Make().Input(factory,0xff000032U,0xabcd0005U); }
};
}
// 비용과 정확히같은SP도요청하며상한/타소유자같은추가관문을두지않는다.
TEST_CASE(FactoryInput_AffordableBoundaryAndNoExtraGates) {
    FactoryInputScene input;input.level=3;CHECK(input.Upgrade()==1 && input.tells.empty());CHECK(input.trace==std::vector<std::string>({"cost","money","upgrading","request"}));CHECK(input.pool.Slot(input.factory)[34]==2 && input.owner==1);
}
// 진행중이어도자금부족안내가먼저이며예약조회/요청은하지않는다.
TEST_CASE(FactoryInput_NoBucksPrecedesExistingUpgrade) {
    FactoryInputScene input;input.money=499;input.upgrading=true;CHECK(input.Upgrade()==1 && input.tells==std::vector<std::string>({"upgradeNoBucks"}));CHECK(input.trace==std::vector<std::string>({"cost","money","tell"}));
}
// x87 비교의unordered는자금부족이며정수비용은비교전에float로축소된다.
TEST_CASE(FactoryInput_UnorderedAndFloatCostRounding) {
    FactoryInputScene nan;nan.money=std::numeric_limits<float>::quiet_NaN();CHECK(nan.Upgrade()==1 && nan.tells==std::vector<std::string>({"upgradeNoBucks"}));FactoryInputScene rounded;rounded.cost=16777217;rounded.money=16777216.0F;CHECK(rounded.Upgrade()==1 && rounded.trace.back()=="request");FactoryInputScene infinity;infinity.money=std::numeric_limits<float>::infinity();CHECK(infinity.Upgrade()==1 && infinity.trace.back()=="request");FactoryInputScene negative;negative.cost=-500;negative.money=-500;CHECK(negative.Upgrade()==1 && negative.trace.back()=="request");
}
// 편집기에서도SP를먼저조회하고이미진행중이면요청하지않는다.
TEST_CASE(FactoryInput_EditorReadsMoneyAndKeepsPendingGate) {
    FactoryInputScene input;input.editor=1;input.money=0;input.upgrading=true;CHECK(input.Upgrade()==1 && input.tells==std::vector<std::string>({"alreadyUpgrading"}));CHECK(input.trace==std::vector<std::string>({"cost","money","upgrading","tell"}));
}
// 비용효과뒤현재로컬번호,SP효과뒤현재editor를다시읽는다.
TEST_CASE(FactoryInput_CurrentOwnerAndEditorAfterChildren) {
    FactoryInputScene input;input.hooks.cost=[&](auto type) { CHECK(type==85);input.owner=7;input.trace.push_back("cost");return 500; };input.hooks.money=[&](auto owner) { CHECK(owner==7);input.editor=1;input.trace.push_back("money");return 0.0F; };CHECK(input.Upgrade()==1 && input.trace.back()=="request" && input.tells.empty());
}
// 상태순환은editor관문없이한번조회하고signedDWORD감김/나머지를보존한다.
TEST_CASE(FactoryInput_CyclePreservesSignedRemainder) {
    FactoryInputScene input;input.level=2;CHECK(input.Make().Input(input.factory,0xff000063U,0)==1 && input.stored==0);CHECK(input.trace==std::vector<std::string>({"level","set","frame"}));input.level=0x7fffffffU;CHECK(input.Make().Input(input.factory,0x03000063U,0)==1 && input.stored==0xfffffffeU);
}
// 연구/공통부모는수정키와상위종류WORD를잘라내지않고위임한다.
TEST_CASE(FactoryInput_DelegatesOriginalCodeAndPackedArgument) {
    FactoryInputScene input;CHECK(input.Make().Input(input.factory,0xff000080U,0xabcd0005U)==17);CHECK(input.Make().Input(input.factory,0xff0000afU,0xabcd0005U)==27);CHECK(input.trace==std::vector<std::string>({"research","parent"}));input.state.cycleCommand=input.state.upgradeCommand;CHECK(input.Upgrade()==1 && input.trace.back()=="frame");
}
// 선택된효과만필수이며누락시앞선현재조회/레벨쓰기를보존한다.
TEST_CASE(FactoryInput_MissingSelectedEffectsKeepPrefix) {
    FactoryInputScene input;input.money=0;input.hooks.tell={};CHECK(Throws([&] { input.Upgrade(); }));CHECK(input.trace==std::vector<std::string>({"cost","money"}));FactoryInputScene request;request.hooks.request={};CHECK(Throws([&] { request.Upgrade(); }) && request.trace.back()=="upgrading");FactoryInputScene cycle;cycle.level=1;cycle.hooks.adjustFrame={};CHECK(Throws([&] { cycle.Make().Input(cycle.factory,0x03000063U,0); }) && cycle.stored==2 && cycle.trace.back()=="set");
}
