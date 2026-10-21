// AI 자원 선택의 실제 원문·절삭·그래프/목록 효과 순서를 검사한다.작성·컴파일만 한다.
#include "o/RawAiResourceChoice.h"
#include "o/RawContainedFinder.h"
#include "RawSceneSupport.h"
#include <limits>
using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 같은 실제 풀/본체/등록 목록과 명시 하위 효과를 준비한다.
struct AiResourceScene {
    SidPool pool{OriginalEdition::Patch1078,32768,true};RawAiBrainMemory memory{0x4b000000,[](auto raw) { std::fill(raw.begin(),raw.end(),std::uint8_t{}); }}; //현재 슬롯과 실제 본체/목록 소유자다.
    std::uint32_t token{memory.Allocate(RawAiBrainConstructor::kBytes)},invalid{254};std::vector<RiftTypeRecord> types{188}; //실제 본체/현재 무효 그래프/TYPE 입력이다.
    Sid actor{},temple{},first{},second{},third{};std::map<std::uint16_t,std::uint32_t> graphs; //입력 슬롯과 명시 그래프 하위의 결과다.
    AiResourceChoiceHooks hooks;std::vector<std::string> trace;std::int32_t crystals{};Sid moved{};RawPathPoint center{20,30}; //실제 호출 순서와 내용물/중심 하위 입력이다.
    // 판단 결과를 계산하지 않고 원문과 선택된 하위만 공급한다.
    AiResourceScene() {
        actor=New(133);temple=New(75);first=New(80);second=New(80);third=New(80);types[80].flags2=0x10000000;Field(0x294,temple.value);Field(0x298,7);Field(0x248,0x10);Field(0xb00,1);
        hooks.center=[this](Sid sid) { CHECK(sid==temple);trace.push_back("center");return center; };hooks.graph=[this](Sid sid) { trace.push_back("graph:"+std::to_string(sid.value));return graphs[sid.value]; };hooks.crystals=[this](Sid sid) { CHECK(sid==actor);trace.push_back("crystals");return crystals; };hooks.target=[this](auto brain,Sid sid,Sid target) { CHECK(brain==token && sid==actor);moved=target;trace.push_back("target"); };hooks.home=[this](auto brain,Sid sid) { CHECK(brain==token && sid==actor);trace.push_back("home"); };hooks.captureLog=[this](auto brain,Sid sid) { CHECK(brain==token);trace.push_back("capture:"+std::to_string(sid.value)); };hooks.disconnectedLog=[this](auto brain,Sid sid) { CHECK(brain==token);trace.push_back("disconnected:"+std::to_string(sid.value)); };hooks.assertion=[this](std::string_view text) { CHECK(text=="s.isValid() && (s.getGenus()&gGEYSER)");trace.push_back("assert"); };
    }
    // 실제 확보한 payload만 명시0으로 준비한다.
    Sid New(std::uint8_t type) { const auto sid=pool.Allocate(2);auto raw=pool.AllocatedBytes(sid);std::fill(raw.begin(),raw.end(),std::uint8_t{});raw[10]=type;return sid; }
    // 지정 현재 본체 DWORD만 쓴다.
    void Field(std::size_t at,std::uint32_t value) { Put(memory.Bytes(token),at,value); }
    // 실제 현재 본체 DWORD를 읽는다.
    std::uint32_t Field(std::size_t at) const { return Get(memory.Bytes(token),at); }
    // 현재 실제 슬롯 위치를 입력한다.
    void Position(Sid sid,float x,float y) { auto raw=pool.AllocatedBytes(sid);Put(raw,14,std::bit_cast<std::uint32_t>(x));Put(raw,18,std::bit_cast<std::uint32_t>(y)); }
    // 새 실제 목록에 입력을 쓰고 count/cap/포인터를 같은 본체에 연결한다.
    std::uint32_t List(std::size_t at,std::initializer_list<Sid> values,std::uint32_t capacity=8) { const auto backing=memory.Allocate(capacity*4);std::size_t index{};for (auto sid:values) Put(memory.Bytes(backing),index++*4,sid.value);Field(at,backing);Field(at+4,capacity);Field(at+8,static_cast<std::uint32_t>(values.size()));return backing; } //반복문은 명시 입력 순서대로 DWORD를 쓴다.
    // 같은 실제 저장소/현재 전역을 빌린다.
    RawAiResourceChoice Make() { return RawAiResourceChoice(memory,pool,types,[this] { return invalid; },hooks); }
};
}
// 신전이 없으면 NaN/부호0을 포함한 fallback 원문을 유지하고 center를 요구하지 않는다.
TEST_CASE(AiResourceChoice_HomeCenterRawFallbackAndCurrentTemple) {
    AiResourceScene input;input.Field(0x294,0);input.Field(0x268,0x7fc01234);input.Field(0x26c,0x80000000);input.hooks.center={};const auto point=input.Make().HomeCenter(input.token);CHECK(std::bit_cast<std::uint32_t>(point.x)==0x7fc01234 && std::bit_cast<std::uint32_t>(point.y)==0x80000000);input.Field(0x294,input.temple.value);input.hooks.center=[&](Sid sid) { CHECK(sid==input.temple);input.Field(0x294,input.first.value);return RawPathPoint{7,8}; };const auto current=input.Make().HomeCenter(input.token);CHECK(current.x==7 && current.y==8 && input.Field(0x294)==input.first.value);
}
// 소수 거리의 정수 절삭 동률은 먼저 방문한 항목이며65536 경계는 선택하지 않는다.
TEST_CASE(AiResourceChoice_TruncatedDistanceTieAndInitialLimit) {
    AiResourceScene input;input.List(0x330,{input.first,input.second,input.third});input.Position(input.first,2.2F,0);input.Position(input.second,2,0);input.Position(input.third,256,0);CHECK(input.Make().Select(input.token,0x330,{0,0},254,254)==input.first);CHECK(input.trace==std::vector<std::string>({"graph:7","graph:8","graph:9"}));input.List(0x330,{input.third});CHECK(!input.Make().Select(input.token,0x330,{0,0},254,254).value);input.Field(0x338,0xffffffffU);input.hooks={};CHECK(!input.Make().Select(input.token,0x330,{0,0},254,254).value);
}
// 모든 후보의 그래프를 읽고 제외/필수 그래프는 현재 무효 sentinel과 비교한다.
TEST_CASE(AiResourceChoice_GraphFiltersAndCurrentInvalidSentinel) {
    AiResourceScene input;input.List(0x330,{input.first,input.second,input.third});input.Position(input.first,1,0);input.Position(input.second,3,0);input.Position(input.third,2,0);input.graphs={{input.first.value,9},{input.second.value,7},{input.third.value,8}};CHECK(input.Make().Select(input.token,0x330,{0,0},9,7)==input.second && input.trace.size()==3);input.invalid=7;CHECK(input.Make().Select(input.token,0x330,{0,0},9,7)==input.third);
}
// 그래프 효과가 현재 포인터/count/좌표를 바꿔도 이번 거리·후보는 이미 저장되어 있다.
TEST_CASE(AiResourceChoice_GraphEffectRereadsCurrentList) {
    AiResourceScene input;input.List(0x330,{input.first,input.third});input.Position(input.first,1,0);input.Position(input.second,2,0);input.hooks.graph=[&](Sid sid) { input.trace.push_back("graph:"+std::to_string(sid.value));if (sid==input.first) { input.Position(input.first,100,0);input.List(0x330,{input.third,input.second}); }return 7U; };CHECK(input.Make().Select(input.token,0x330,{0,0},254,7)==input.first && input.trace==std::vector<std::string>({"graph:7","graph:8"}));input.trace.clear();input.hooks.graph=[&](Sid sid) { CHECK(sid==input.third);input.Field(0x338,0);return 7U; };CHECK(input.Make().Select(input.token,0x330,{0,0},254,7)==input.third && input.Field(0x338)==0);
}
// 원본 무효 ftol 하위DWORD0을 보존하며 잘못된 물리목록은 성공으로 바꾸지 않는다.
TEST_CASE(AiResourceChoice_NonfiniteFtolAndPhysicalBounds) {
    AiResourceScene input;input.List(0x330,{input.first,input.second});input.Position(input.first,std::numeric_limits<float>::quiet_NaN(),0);CHECK(input.Make().Select(input.token,0x330,{0,0},254,254)==input.first);input.Field(0x330,0xdeadbeefU);CHECK(Throws([&] { input.Make().Select(input.token,0x330,{0,0},254,254); }));
}
// 관문 탈락은 내용물도 읽지 않고,크리스털/수집능력 탈락은 실제 귀환만 호출한다.
TEST_CASE(AiResourceChoice_CollectionGatesAndCarriedCrystal) {
    AiResourceScene input;input.Field(0x294,0);input.hooks={};input.Make().CollectOrHome(input.token,input.actor);input.Field(0x294,input.temple.value);input.Field(0x298,254);input.Make().CollectOrHome(input.token,input.actor);CHECK(input.trace.empty());AiResourceScene carried;carried.crystals=1;carried.Make().CollectOrHome(carried.token,carried.actor);CHECK(carried.trace==std::vector<std::string>({"crystals","home"}));carried.trace.clear();carried.crystals=0;carried.Field(0x248,0);carried.Make().CollectOrHome(carried.token,carried.actor);CHECK(carried.trace==std::vector<std::string>({"crystals","home"}));
}
// 현재 신전 중심→actor 그래프→간헐천 목록→실제 이동 순서다.목표가 없으면 귀환한다.
TEST_CASE(AiResourceChoice_CollectionUsesHomeCenterAndActualMovement) {
    AiResourceScene input;input.List(0x330,{input.first,input.second});input.Position(input.first,21,30);input.Position(input.second,20,32);input.graphs={{input.actor.value,7},{input.first.value,8},{input.second.value,7}};input.Make().CollectOrHome(input.token,input.actor);CHECK(input.moved==input.second && input.trace==std::vector<std::string>({"crystals","center","graph:5","graph:7","graph:8","target"}));input.trace.clear();input.Field(0x338,0);input.Make().CollectOrHome(input.token,input.actor);CHECK(input.trace==std::vector<std::string>({"crystals","center","graph:5","home"}));
}
// 내용물 효과 뒤 능력을 다시 읽으며 actor 그래프 효과 뒤 현재 sentinel을 인자로 쓴다.
TEST_CASE(AiResourceChoice_CollectionEffectsUseCurrentCapabilityAndSentinel) {
    AiResourceScene input;input.hooks.crystals=[&](Sid) { input.Field(0x248,0);return 0; };input.Make().CollectOrHome(input.token,input.actor);CHECK(input.trace==std::vector<std::string>({"home"}));input.trace.clear();input.Field(0x248,0x10);input.hooks.crystals=[](Sid) { return 0; };input.List(0x330,{input.first});input.hooks.graph=[&](Sid sid) { if (sid==input.actor) { input.invalid=7;return 7U; }return 8U; };input.Make().CollectOrHome(input.token,input.actor);CHECK(input.moved==input.first && input.trace==std::vector<std::string>({"center","target"}));
}
// 비행 판정은 현재TYPE만 보며 중복은 로그도 생략하고full은 로그를 유지한다.
TEST_CASE(AiResourceChoice_CaptureQueueFlyingDuplicateCapacityAndNoTimer) {
    AiResourceScene input;const auto backing=input.List(0x384,{input.first},2);input.types[80].flags2|=0x20000;input.pool.AllocatedBytes(input.first)[11]=3;CHECK(input.Make().HasFlyingCaptureCarrier(input.token));const auto before=std::vector<std::uint8_t>(input.memory.Bytes(input.token).begin(),input.memory.Bytes(input.token).end());input.Make().RememberCaptureCarrier(input.token,input.first);CHECK(input.trace.empty());input.Make().RememberCaptureCarrier(input.token,input.actor);CHECK(input.Field(0x38c)==2 && Get(input.memory.Bytes(backing),4)==input.actor.value);input.Make().RememberCaptureCarrier(input.token,input.second);CHECK(input.Field(0x38c)==2 && input.trace==std::vector<std::string>({"capture:5","capture:8"}));CHECK(std::equal(before.begin()+0x5e0,before.begin()+0x5e8,input.memory.Bytes(input.token).begin()+0x5e0));input.Field(0x38c,0);input.Field(0x388,0);input.Field(0x384,0xdeadbeefU);input.Field(0xb00,0);input.hooks={};CHECK(!input.Make().HasFlyingCaptureCarrier(input.token));input.Make().RememberCaptureCarrier(input.token,input.actor);
}
// 끊긴 중복을 모두 안정 제거하며 대상 목록full은 제거를 되돌리지 않는다.
TEST_CASE(AiResourceChoice_DisconnectedStableRemovalAndFullDestination) {
    AiResourceScene input;const auto source=input.List(0x330,{input.first,input.first,input.second,input.third});const auto destination=input.List(0x348,{input.third},2);input.graphs={{input.first.value,8},{input.second.value,7},{input.third.value,9}};input.Make().ReclassifyDisconnected(input.token);CHECK(input.Field(0x338)==1 && Get(input.memory.Bytes(source),0)==input.second.value && input.Field(0x350)==2 && Get(input.memory.Bytes(destination),0)==input.third.value && Get(input.memory.Bytes(destination),4)==input.first.value);CHECK(input.trace==std::vector<std::string>({"graph:7","disconnected:7","graph:8","graph:9","disconnected:9"}));CHECK(Get(input.memory.Bytes(source),12)==input.third.value);
}
// SID0 진단이 반환하면 무효 그래프를 사용하고,로그 누락이면 이동 전 원문을 유지한다.
TEST_CASE(AiResourceChoice_DisconnectedAssertionAndSelectedMissingLog) {
    AiResourceScene zero;zero.List(0x330,{Sid{}});zero.Field(0x298,254);zero.hooks.graph={};zero.Make().ReclassifyDisconnected(zero.token);CHECK(zero.trace==std::vector<std::string>({"assert"}) && zero.Field(0x338)==1);AiResourceScene missing;const auto backing=missing.List(0x330,{missing.first});missing.graphs[missing.first.value]=8;missing.hooks.disconnectedLog={};CHECK(Throws([&] { missing.Make().ReclassifyDisconnected(missing.token); }) && missing.Field(0x338)==1 && Get(missing.memory.Bytes(backing),0)==missing.first.value);
}
// 공용 내용물 총수는 전체WORD mask와DWORD kind를 비교하고free/dead 후보를 임의 제외하지 않는다.
TEST_CASE(AiResourceChoice_CountContainedUsesFullWordAndDwordFilters) {
    AiResourceScene input;ContainedFinderState state;auto actor=input.pool.AllocatedBytes(input.actor),first=input.pool.AllocatedBytes(input.first),second=input.pool.AllocatedBytes(input.second),third=input.pool.AllocatedBytes(input.third);Put(actor,6,input.first.value,2);Put(first,4,input.second.value,2);Put(second,4,input.third.value,2);first[10]=second[10]=third[10]=6;first[11]=3;Put(first,18,154,2);Put(first,20,0x120,2);Put(second,18,154,2);Put(second,20,0x20,2);Put(third,18,155,2);Put(third,20,0x100,2);CHECK(CountContained(input.pool,state,input.actor,0x20,154)==2 && CountContained(input.pool,state,input.actor,0x100,0)==2 && CountContained(input.pool,state,input.actor,0,0)==3 && CountContained(input.pool,state,input.actor,0,0x10000)==0);
}
