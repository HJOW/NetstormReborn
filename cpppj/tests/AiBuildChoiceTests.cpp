// 실제본체/현재TYPE로건설선택·비용절삭·자금분기와선호중복을검사한다.작성·컴파일만한다.
#include "o/RawAiBuildChoice.h"
#include "RawSceneSupport.h"
#include <limits>
using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 원본입력과현재상태를읽는명시하위효과만준비한다.
struct AiChoiceScene {
    RawAiBrainMemory memory{0x4e000000,[](auto raw) { std::fill(raw.begin(),raw.end(),std::uint8_t{}); }};std::uint32_t token{memory.Allocate(RawAiBrainConstructor::kBytes)}; //같은실제본체/핸들이다.
    std::vector<RiftTypeRecord> types{188};std::uint32_t local{1},known{1},collector{133},dais{165};std::array<std::uint32_t,4> workshops{86,87,88,85};std::array<std::uint32_t,3> temples{123,124,125}; //현재TYPE와전역입력이다.
    bool captured{},dead{},afford{true};std::int32_t remote{999};float stored{999.9F};AiBuildChoiceHooks hooks;std::vector<std::string> trace; //선택효과/자금입력과실제호출순서다.
    // 기본은현재신전/자금600/수집자2개분여유/부족한sun워크샾이다.
    AiChoiceScene() {
        types[133].cost=100.9F;types[165].cost=200;types[123].cost=300.9F;types[124].cost=400;types[125].cost=500;types[86].cost=50;Field(0x264,2);Field(0x294,1);Field(0x2d8,600);Field(0x234,2);Field(0x394,86);Preferences({0});
        hooks.captured=[this](auto owner) { trace.push_back("captured:"+std::to_string(owner));return captured; };hooks.dead=[this](auto owner) { trace.push_back("dead:"+std::to_string(owner));return dead; };hooks.canAfford=[this](auto brain,auto type) { CHECK(brain==token);trace.push_back("afford:"+std::to_string(type));return afford; };hooks.wallet=[this](auto owner) { trace.push_back("wallet:"+std::to_string(owner));return remote; };hooks.storedSp=[this](auto owner) { trace.push_back("sp:"+std::to_string(owner));return stored; };hooks.assertion=[this](std::string_view text,auto line) { CHECK(text==(line==0x1a ? "theIndex>=0 && theIndex<=(MAX_THEMES)" : "0"));trace.push_back("assert:"+std::to_string(line)); };
    }
    // 현재본체DWORD만입력한다.
    void Field(std::size_t at,std::uint32_t value) { Put(memory.Bytes(token),at,value); }
    // 현재TYPE별실제수량을입력한다.
    void Count(std::uint32_t type,std::uint32_t value) { Field(0x6d4+static_cast<std::size_t>(type)*4,value); }
    // 같은본체의선호순서를입력한다.
    void Preferences(std::initializer_list<std::uint32_t> values) { std::size_t index{};for (auto value:values) Field(0x398+index++*4,value);Field(0x3b0,static_cast<std::uint32_t>(values.size())); } //반복문은입력한선호순서만쓴다.
    // 현재전역은참조/조회기로빌리며TYPE값을생성시복제하지않는다.
    RawAiBuildChoice Make() { return RawAiBuildChoice(memory,types,{local,known,collector,dais,workshops,[this](auto index) { trace.push_back("temple:"+std::to_string(index));return temples[index]; }},hooks); }
};
}
// 수집자cost는곱하기전에절삭하며하위DWORD곱감김과현재전역TYPE를사용한다.
TEST_CASE(AiBuildChoice_CollectorReserveTruncationWrapAndCurrentType) {
    AiChoiceScene input;input.Field(0x2d8,200);CHECK(input.Make().ReserveForCollectors(input.token));input.Field(0x2d8,199);CHECK(!input.Make().ReserveForCollectors(input.token));input.types[133].cost=0.9F;input.Field(0x234,1000);input.Field(0x2d8,0);CHECK(input.Make().ReserveForCollectors(input.token));input.types[133].cost=2147483648.0F;input.Field(0x234,2);CHECK(input.Make().ReserveForCollectors(input.token));auto choice=input.Make();input.collector=134;input.types[134].cost=20.9F;input.Field(0x2d8,40);CHECK(choice.ReserveForCollectors(input.token));
}
// 로컬확정SP는현재float절삭/미확정은0이며원격은signed실제지갑을쓴다.
TEST_CASE(AiBuildChoice_EnemyLocalSpRemoteWalletAndUnknownPlayer) {
    AiChoiceScene input;CHECK(input.Make().EnemyLowFunds(input.token) && input.trace==std::vector<std::string>({"wallet:2"}));input.remote=1000;CHECK(!input.Make().EnemyLowFunds(input.token));input.Field(0x264,1);input.trace.clear();CHECK(input.Make().EnemyLowFunds(input.token) && input.trace==std::vector<std::string>({"sp:1"}));input.stored=1000.9F;CHECK(!input.Make().EnemyLowFunds(input.token));input.stored=std::numeric_limits<float>::quiet_NaN();CHECK(input.Make().EnemyLowFunds(input.token));input.known=0;input.trace.clear();input.hooks.storedSp={};input.hooks.wallet={};CHECK(input.Make().EnemyLowFunds(input.token) && input.trace.empty());
}
// 신전이있으면적자금관문없이포획다이스를선택하고뒤의잘못된후보TYPE를읽지않는다.
TEST_CASE(AiBuildChoice_DaisPrioritySkipsLaterFundsAndCandidate) {
    AiChoiceScene input;input.Field(0x248,0x200);input.Field(0x394,200);input.captured=true;input.hooks.wallet={};input.hooks.storedSp={};CHECK(input.Make().Select(input.token)==165 && input.trace==std::vector<std::string>({"captured:2","dead:2","afford:165"}));
}
// 각사제/비용효과뒤현재적과다이스를다시읽으며이미확인한관문은반복하지않는다.
TEST_CASE(AiBuildChoice_DaisEffectsReadCurrentEnemyAndReturnType) {
    AiChoiceScene input;input.Field(0x248,0x200);input.Field(0x294,0);input.hooks.captured=[&](auto owner) { CHECK(owner==2);input.trace.push_back("captured");input.Field(0x264,3);input.Field(0x2a4,1);return true; };input.hooks.dead=[&](auto owner) { CHECK(owner==3);input.trace.push_back("dead");return false; };input.hooks.canAfford=[&](auto brain,auto type) { CHECK(brain==input.token && type==165);input.dais=166;input.trace.push_back("afford");return true; };input.hooks.wallet=[&](auto owner) { CHECK(owner==3);input.dais=167;input.trace.push_back("wallet");return 999; };CHECK(input.Make().Select(input.token)==167 && input.trace==std::vector<std::string>({"captured","dead","afford","wallet"}));
}
// 이미다이스가있거나포획/생존관문탈락이면후속효과를선택하지않는다.
TEST_CASE(AiBuildChoice_DaisShortCircuitAndFallback) {
    AiChoiceScene input;input.Field(0x248,0x200);input.Field(0x2a4,1);input.hooks.captured={};CHECK(input.Make().Select(input.token)==86 && input.trace.empty());input.Field(0x2a4,0);input.hooks.captured=[&](auto) { input.trace.push_back("captured");return false; };input.hooks.dead={};CHECK(input.Make().Select(input.token)==86 && input.trace==std::vector<std::string>({"captured"}));input.trace.clear();input.captured=true;input.hooks.captured=[&](auto) { input.trace.push_back("captured");return true; };input.hooks.dead=[&](auto) { input.trace.push_back("dead");return true; };input.hooks.canAfford={};CHECK(input.Make().Select(input.token)==86 && input.trace==std::vector<std::string>({"captured","dead"}));
}
// 신전없는다이스는적자금1000미만이필요하며실패하면현재원소신전으로간다.
TEST_CASE(AiBuildChoice_DaisFundsFallbackToTempleAndCostBoundary) {
    AiChoiceScene input;input.Field(0x248,0x200);input.Field(0x294,0);input.captured=true;CHECK(input.Make().Select(input.token)==165);input.remote=1000;input.trace.clear();CHECK(input.Make().Select(input.token)==123 && input.trace.back()=="temple:0");input.Field(0x248,0);input.Field(0x2d8,300);CHECK(input.Make().Select(input.token)==123);input.Field(0x2d8,299);CHECK(!input.Make().Select(input.token));
}
// 실제getter의0..3/4·5/진단과signed%5를구별하며5를별도신전으로해석하지않는다.
TEST_CASE(AiBuildChoice_CurrentThemeMappingsAndSignedRemainder) {
    AiChoiceScene input;auto choice=input.Make();CHECK(choice.WorkshopType(0)==86 && choice.WorkshopType(4)==0 && choice.WorkshopType(5)==0);CHECK(choice.WorkshopType(-1)==0 && input.trace.back()=="assert:26");input.workshops[0]=87;CHECK(choice.WorkshopType(0)==87);input.temples[0]=126;CHECK(choice.TempleType(5)==126 && choice.TempleType(-5)==126 && choice.TempleType(6)==124 && choice.TempleType(7)==125);CHECK(choice.TempleType(8)==0 && input.trace.back()=="assert:71");CHECK(choice.TempleType(-1)==0 && input.trace.back()=="assert:71");
}
// theme3의무신전은신전getter를건너뛰고workshop후보0은비용/수집자TYPE도읽지않는다.
TEST_CASE(AiBuildChoice_CommonThemeAndNullCandidateGates) {
    AiChoiceScene input;input.Field(0x294,0);input.Field(0x390,3);input.Field(0x394,0);input.collector=200;CHECK(!input.Make().Select(input.token) && input.trace.empty());input.Field(0x294,1);input.Field(0x394,86);input.Field(0x2d8,49);CHECK(!input.Make().Select(input.token));input.Field(0x2d8,50);CHECK(Throws([&] { input.Make().Select(input.token); }));
}
// 현재지갑이각각의비용관문을통과하면되며건설비용을여유비용에추가하지않는다.
TEST_CASE(AiBuildChoice_WorkshopCostAndReserveAreSeparateGates) {
    AiChoiceScene input;input.types[86].cost=150;input.Field(0x2d8,200);CHECK(input.Make().Select(input.token)==86);input.Field(0x2d8,199);CHECK(!input.Make().Select(input.token));
}
// 중복선호마다같은지역복사수량을감소시키고실제본체수량은바꾸지않는다.
TEST_CASE(AiBuildChoice_DuplicatePreferencesAndReadOnlyCounts) {
    AiChoiceScene input;input.Count(86,1);input.Count(87,1);input.Preferences({0,1,0});const auto raw=input.memory.Bytes(input.token);const auto before=std::vector<std::uint8_t>(raw.begin(),raw.end());CHECK(input.Make().Select(input.token)==86 && std::equal(before.begin(),before.end(),raw.begin()));input.Preferences({0,1});CHECK(!input.Make().Select(input.token));input.Preferences({1,2});CHECK(input.Make().Select(input.token)==88);
}
// 다섯번째수량은TYPE0이며음수감김도원본DWORD감소뒤signed판정을따른다.
TEST_CASE(AiBuildChoice_FifthThemeAndSignedCounterWrap) {
    AiChoiceScene input;input.Preferences({4,0});CHECK(!input.Make().Select(input.token));input.Count(0,1);CHECK(input.Make().Select(input.token)==86);input.Preferences({0});input.Count(86,0x80000000U);CHECK(!input.Make().Select(input.token));input.Count(86,0xffffffffU);CHECK(input.Make().Select(input.token)==86);
}
// 선호count0이어도현재다섯수량을먼저읽으며손상된임시인덱스는가짜TYPE로보정하지않는다.
TEST_CASE(AiBuildChoice_CountCopyOrderAndPreferenceBounds) {
    AiChoiceScene input;input.Preferences({});input.workshops[0]=0xffffffffU;CHECK(Throws([&] { input.Make().Select(input.token); }));input.workshops[0]=86;input.Preferences({5});CHECK(Throws([&] { input.Make().Select(input.token); }));
}
// 선택된다이스자금하위누락은이미호출한사제/비용효과를보존한다.
TEST_CASE(AiBuildChoice_SelectedMissingFundsKeepsPrefix) {
    AiChoiceScene input;input.Field(0x248,0x200);input.Field(0x294,0);input.captured=true;input.hooks.wallet={};CHECK(Throws([&] { input.Make().Select(input.token); }) && input.trace==std::vector<std::string>({"captured:2","dead:2","afford:165"}));
}
