// 실제 본체/목록으로 전체 수집·포획 순서와 genus/다이스 하위를 검사한다.작성·컴파일만 한다.
#include "o/RawAiCarrierActions.h"
#include "RawSceneSupport.h"
#include <limits>
using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 원본 raw 입력과 선택된 하위 효과/호출 순서를 준비한다.
struct AiCarrierScene {
    SidPool pool{OriginalEdition::Patch1078,32768,true};RawAiBrainMemory memory{0x4c000000,[](auto raw) { std::fill(raw.begin(),raw.end(),std::uint8_t{}); }}; //같은 실제 풀/할당 저장소다.
    std::uint32_t token{memory.Allocate(RawAiBrainConstructor::kBytes)},invalid{254},checking{};double now{};bool boss{true}; //실제 본체와 현재 전역 입력이다.
    std::vector<RiftTypeRecord> types{188};Sid actor{},enemy{},dais{},geyser{},other{},crystal{}; //현재TYPE와 실제 물리 슬롯이다.
    std::map<std::uint16_t,Sid> prisoners,paths;std::map<std::uint16_t,std::uint32_t> graphs; //명시 하위의 현재 결과 입력이다.
    AiCarrierActionHooks hooks;std::vector<std::string> trace;std::int32_t crystals{};bool available{true},vulnerable{true},flying{};std::array<std::uint32_t,2> targets{}; //선택 효과/순서와 상태 입력이다.
    // 기본은 정지한 일반 수집자 한 명이며 결과 판단은 입력에 합성하지 않는다.
    AiCarrierScene() {
        actor=New(133);enemy=New(158);dais=New(165);geyser=New(80);other=New(133);crystal=New(6);types[158].flags2=0x200000;types[165].flags2=0x400000;types[80].flags2=0x10000000;Field(0x2a0,enemy.value);Field(0x264,2);Field(0x294,dais.value);Field(0x298,7);List(0x2dc,{actor});List(0x384,{});List(0x330,{});graphs={{actor.value,7},{enemy.value,7},{geyser.value,7}};
        hooks.maxHitPoints=[this](Sid) { trace.push_back("max");return 100; };hooks.prisoner=[this](Sid sid) { trace.push_back("prisoner:"+Role(sid));return prisoners[sid.value]; };hooks.path=[this](Sid sid) { trace.push_back("path:"+Role(sid));return paths[sid.value]; };hooks.daisPoint=[this](Sid sid) { CHECK(sid==dais);trace.push_back("dais");return RawPathPoint{10,12}; };hooks.point=[this](auto brain,Sid sid,RawPathPoint point) { CHECK(brain==token && point.x==10 && point.y==12);trace.push_back("point:"+Role(sid)); };hooks.home=[this](auto brain,Sid sid) { CHECK(brain==token);trace.push_back("home:"+Role(sid)); };hooks.enemyAvailable=[this](auto owner) { CHECK(owner==2);trace.push_back("enemy");return available; };hooks.vulnerable=[this](Sid) { trace.push_back("vulnerable");return vulnerable; };hooks.flyingGetter=[this](auto brain) { CHECK(brain==token);trace.push_back("flying");return flying; };hooks.graph=[this](Sid sid) { trace.push_back("graph:"+Role(sid));return graphs[sid.value]; };
        hooks.crystals=[this](Sid sid) { CHECK(sid==actor);trace.push_back("crystals");return crystals; };hooks.firstCrystal=[this](Sid sid) { CHECK(sid==actor);trace.push_back("first");return crystal; };hooks.dropRequest=[this](Sid sid) { trace.push_back("drop:"+Role(sid)); };hooks.destroy=[this](Sid sid) { CHECK(sid==crystal);trace.push_back("destroy"); };hooks.target=[this](auto brain,Sid sid,Sid target) { CHECK(brain==token && sid==actor);trace.push_back("target:"+Role(target)); };hooks.remember=[this](auto brain,Sid sid) { CHECK(brain==token);trace.push_back("remember:"+Role(sid)); };hooks.collect=[this](auto brain,Sid sid) { CHECK(brain==token);trace.push_back("collect:"+Role(sid)); };hooks.targets=[this](Sid sid) { CHECK(sid==actor);trace.push_back("targets");return targets; };hooks.reclassify=[this](auto brain) { CHECK(brain==token);trace.push_back("reclassify"); };hooks.stop=[this](Sid sid) { trace.push_back("stop:"+Role(sid)); };hooks.assertion=[this](std::string_view,std::uint32_t line) { trace.push_back("assert:"+std::to_string(line)); };
    }
    // 확보한 실제 payload만 명시0으로 초기화한다.
    Sid New(std::uint8_t type) { const auto sid=pool.Allocate(2);auto raw=pool.AllocatedBytes(sid);std::fill(raw.begin(),raw.end(),std::uint8_t{});raw[10]=type;return sid; }
    // 호출 인자를 쉽게 구별하는 고정 입력 이름이다.
    std::string Role(Sid sid) const { if (sid==actor) return "A";if (sid==enemy) return "E";if (sid==other) return "O";if (sid==geyser) return "G";return "other"; }
    // 현재 본체 DWORD만 쓴다.
    void Field(std::size_t at,std::uint32_t value) { Put(memory.Bytes(token),at,value); }
    // 현재 본체 DWORD를 읽는다.
    std::uint32_t Field(std::size_t at) const { return Get(memory.Bytes(token),at); }
    // 원본 QWORD 기한의 두 DWORD를 입력한다.
    void Due(std::size_t at,double value) { const auto bits=std::bit_cast<std::uint64_t>(value);Field(at,static_cast<std::uint32_t>(bits));Field(at+4,static_cast<std::uint32_t>(bits>>32)); }
    // 실제 backing에 입력 목록을 연결한다.
    std::uint32_t List(std::size_t at,std::initializer_list<Sid> values,std::uint32_t capacity=8) { const auto pointer=memory.Allocate(capacity*4);std::size_t index{};for (auto sid:values) Put(memory.Bytes(pointer),index++*4,sid.value);Field(at,pointer);Field(at+4,capacity);Field(at+8,static_cast<std::uint32_t>(values.size()));return pointer; } //반복문은 명시 입력 순서만 쓴다.
    // 같은 현재 전역과 실제 저장소를 빌린다.
    RawAiCarrierActions Make() { return RawAiCarrierActions(memory,pool,types,{now,boss,checking,[this] { return invalid; }},hooks); }
};
}
// 물리 적 SID면 free/dead여도 최대HP prefix를 실행하며 빈 일반 목록은 그 뒤 종료한다.
TEST_CASE(AiCarrierActions_PrefixAndEmptyListOrder) {
    AiCarrierScene input;input.Field(0x2e4,0);input.pool.AllocatedBytes(input.enemy)[11]=3;input.Make().Act(input.token);CHECK(input.trace==std::vector<std::string>({"max"}));input.trace.clear();input.Field(0x2a0,0);input.hooks={};input.Make().Act(input.token);CHECK(input.trace.empty());
}
// 포획 귀환은 빈 일반 목록/미도달 기한보다 우선하고 이동 효과 뒤 현재 큐를 한 actor로 축소한다.
TEST_CASE(AiCarrierActions_PrisonerQueuePriorityAndCollapse) {
    AiCarrierScene input;const auto backing=input.List(0x384,{Sid{},input.actor,input.other});input.prisoners[input.actor.value]=input.crystal;input.Field(0x2e4,0);input.Due(0x5a8,std::numeric_limits<double>::quiet_NaN());input.Field(0x2a4,input.dais.value);input.hooks.point=[&](auto,Sid actor,RawPathPoint point) { CHECK(actor==input.actor && point.x==10);input.trace.push_back("point:A");input.Field(0x38c,7); };input.Make().Act(input.token);CHECK(input.trace==std::vector<std::string>({"max","prisoner:A","path:A","dais","point:A"}) && input.Field(0x38c)==1 && Get(input.memory.Bytes(backing),0)==input.actor.value && Get(input.memory.Bytes(backing),8)==input.other.value);input.trace.clear();input.Field(0x388,0);input.Field(0x2a4,0);input.Make().Act(input.token);CHECK(input.trace.back()=="home:A" && input.Field(0x38c)==0);
}
// less/unordered 기한은 순환 인덱스도 바꾸지 않고 prefix까지만 진행한다.
TEST_CASE(AiCarrierActions_ThinkTimerOrderedAndUnordered) {
    AiCarrierScene input;input.Field(0x6d0,77);input.Due(0x5a8,1);input.Make().Act(input.token);CHECK(input.Field(0x6d0)==77 && input.trace==std::vector<std::string>({"max"}));input.trace.clear();input.Due(0x5a8,std::numeric_limits<double>::quiet_NaN());input.Make().Act(input.token);CHECK(input.Field(0x6d0)==77 && input.trace.size()==1);input.trace.clear();input.Due(0x5a8,0);input.now=std::numeric_limits<double>::quiet_NaN();input.Make().Act(input.token);CHECK(input.Field(0x6d0)==77 && input.trace.size()==1);input.now=0;input.Make().Act(input.token);CHECK(input.Field(0x6d0)==0 && input.trace.back()=="collect:A");
}
// 순환은 다음 항목부터 시작하며 free는 모든 중복을 안정 제거하고 cursor를 보정하지 않는다.
TEST_CASE(AiCarrierActions_RoundRobinAndFreeStableRemoval) {
    AiCarrierScene input;const auto backing=input.List(0x2dc,{input.actor,input.other,input.other});input.pool.AllocatedBytes(input.other)[11]=1;input.checking=1;input.Make().Act(input.token);CHECK(input.Field(0x6d0)==1 && input.Field(0x2e4)==1 && Get(input.memory.Bytes(backing),0)==input.actor.value && Get(input.memory.Bytes(backing),8)==input.other.value && input.trace==std::vector<std::string>({"max","assert:3076"}));
}
// free 진단이 반환하며 상태를 고치면 계속하고 priest genus 진단은 checking0에서도 호출한다.
TEST_CASE(AiCarrierActions_ReturningDiagnosticRereadsState) {
    AiCarrierScene input;auto raw=input.pool.AllocatedBytes(input.actor);input.checking=1;raw[11]=1;input.hooks.assertion=[&](std::string_view text,auto line) { CHECK(text=="!s.isFree()" && line==0xc04);raw[11]=0;input.trace.push_back("fixed"); };input.Make().Act(input.token);CHECK(input.Field(0x2e4)==1 && input.trace==std::vector<std::string>({"max","fixed","path:A","prisoner:A","collect:A"}));input.trace.clear();input.checking=0;input.types[133].flags2=0x200000;input.hooks.assertion=[&](std::string_view text,auto line) { CHECK(text=="!( s.getGenus() & gPRIEST )" && line==0xc0a);input.trace.push_back("priestAssert"); };input.Make().Act(input.token);CHECK(input.trace[1]=="priestAssert");
}
// 서버 포획은 첫 크리스털만 삭제하고 삭제 효과 뒤 현재 적으로 이동한 다음 등록한다.
TEST_CASE(AiCarrierActions_ServerCaptureCrystalAndCurrentEnemy) {
    AiCarrierScene input;input.Field(0x248,0x100);input.crystals=2;input.hooks.destroy=[&](Sid crystal) { CHECK(crystal==input.crystal);input.trace.push_back("destroy");input.Field(0x2a0,input.other.value); };input.Make().Act(input.token);CHECK(input.trace==std::vector<std::string>({"max","path:A","enemy","vulnerable","graph:A","graph:E","crystals","first","destroy","target:O","remember:A"}));
}
// 현재 boss는 첫 크리스털 조회 뒤 재판정하며 비서버 내려놓기는 이동/등록을 생략한다.
TEST_CASE(AiCarrierActions_CaptureAuthorityAfterCrystalLookup) {
    AiCarrierScene input;input.Field(0x248,0x100);input.crystals=1;input.hooks.firstCrystal=[&](Sid) { input.trace.push_back("first");input.boss=false;return input.crystal; };input.Make().Act(input.token);CHECK(input.trace.back()=="drop:A" && std::find(input.trace.begin(),input.trace.end(),"destroy")==input.trace.end() && std::find(input.trace.begin(),input.trace.end(),"remember:A")==input.trace.end());input.trace.clear();input.hooks.firstCrystal=[&](Sid) { input.trace.push_back("first");input.boss=true;return input.crystal; };input.Make().Act(input.token);CHECK(input.trace[input.trace.size()-3]=="destroy" && input.trace.back()=="remember:A");
}
// 첫 비행 포획 운반자는 그래프를 요구하지 않으며 비행 운반자가 이미 있으면 그래프로 판단한다.
TEST_CASE(AiCarrierActions_FlyingCaptureAndGraphFallback) {
    AiCarrierScene input;input.Field(0x248,0x100);input.types[133].flags2=0x20000;input.hooks.graph={};input.Make().Act(input.token);CHECK(input.trace==std::vector<std::string>({"max","path:A","enemy","vulnerable","flying","crystals","target:E","remember:A"}));input.trace.clear();input.flying=true;input.hooks.graph=[&](Sid sid) { input.trace.push_back("graph:"+input.Role(sid));return sid==input.actor ? 7U : 8U; };input.Make().Act(input.token);CHECK(input.trace[input.trace.size()-2]=="prisoner:A" && input.trace.back()=="collect:A");
}
// 참여 효과 뒤 현재 적을 HP판정에 전달하고 포획 기한NaN이면비행/그래프를조회하지않는다.
TEST_CASE(AiCarrierActions_CaptureShortCircuitAndTimerEffects) {
    AiCarrierScene input;input.Field(0x248,0x100);input.hooks.enemyAvailable=[&](auto) { input.trace.push_back("enemy");input.Field(0x2a0,input.other.value);return true; };input.hooks.vulnerable=[&](Sid sid) { CHECK(sid==input.other);input.trace.push_back("vulnerable");input.Due(0x5e0,std::numeric_limits<double>::quiet_NaN());return true; };input.hooks.graph={};input.Make().Act(input.token);CHECK(input.trace==std::vector<std::string>({"max","path:A","enemy","vulnerable","prisoner:A","collect:A"}));
}
// 일반 선택의 포획 사제 귀환은 큐를 축소하지 않으며 수집 뒤에는 현재 큐의 모든 중복을 제거한다.
TEST_CASE(AiCarrierActions_DeliveryAndCollectionCurrentQueue) {
    AiCarrierScene input;input.prisoners[input.actor.value]=input.crystal;input.Make().Act(input.token);CHECK(input.trace==std::vector<std::string>({"max","path:A","prisoner:A","home:A"}) && input.Field(0x38c)==0);input.trace.clear();input.prisoners.clear();input.hooks.collect=[&](auto,Sid) { input.trace.push_back("collect:A");input.List(0x384,{input.other,input.actor,input.actor,input.geyser}); };input.Make().Act(input.token);const auto list=input.memory.Bytes(input.Field(0x384));CHECK(input.Field(0x38c)==2 && Get(list,0)==input.other.value && Get(list,4)==input.geyser.value && Get(list,12)==input.geyser.value);
}
// 포획 대기중 경로는 유지하고 신전/유효그래프 없이는목표조회없이정지하며targetA0은정지하지않는다.
TEST_CASE(AiCarrierActions_ActivePathProtectionAndMissingTarget) {
    AiCarrierScene input;input.paths[input.actor.value]=Sid{100};input.List(0x384,{input.actor});input.Make().Act(input.token);CHECK(input.trace.back()=="path:A" && std::find(input.trace.begin(),input.trace.end(),"targets")==input.trace.end());input.trace.clear();input.Field(0x38c,0);input.Field(0x294,0);input.Make().Act(input.token);CHECK(input.trace.back()=="stop:A");input.trace.clear();input.Field(0x294,input.dais.value);input.Make().Act(input.token);CHECK(input.trace.back()=="targets");
}
// 살아있는등록간헐천의비행/같은그래프경로는유지하고다른그래프는재분류한뒤정지한다.
TEST_CASE(AiCarrierActions_GeyserPathRetentionAndCancellation) {
    AiCarrierScene input;input.paths[input.actor.value]=Sid{100};input.targets={input.geyser.value,0xffffffffU};input.List(0x330,{input.geyser});input.types[133].flags2=0x20000;input.Make().Act(input.token);CHECK(input.trace.back()=="targets");input.trace.clear();input.types[133].flags2=0;input.Make().Act(input.token);CHECK(input.trace[input.trace.size()-2]=="graph:G" && input.trace.back()=="graph:A");input.trace.clear();input.graphs[input.geyser.value]=8;input.Make().Act(input.token);CHECK(input.trace[input.trace.size()-2]=="reclassify" && input.trace.back()=="stop:A");input.trace.clear();input.pool.AllocatedBytes(input.geyser)[11]=2;input.hooks.graph={};input.Make().Act(input.token);CHECK(input.trace==std::vector<std::string>({"max","path:A","targets","reclassify","stop:A"}));
}
// 재분류 뒤 정지 하위 누락은 앞선 효과를 보존하며 signed 음수 cursor는 쓴 뒤 물리 접근을 거부한다.
TEST_CASE(AiCarrierActions_SelectedMissingEffectAndSignedCursor) {
    AiCarrierScene input;input.paths[input.actor.value]=Sid{100};input.targets={input.other.value,0};input.hooks.stop={};CHECK(Throws([&] { input.Make().Act(input.token); }) && input.trace.back()=="reclassify");AiCarrierScene cursor;cursor.List(0x2dc,{cursor.actor,cursor.other});cursor.Field(0x6d0,0xfffffffeU);CHECK(Throws([&] { cursor.Make().Act(cursor.token); }) && cursor.Field(0x6d0)==0xffffffffU && cursor.trace==std::vector<std::string>({"max"}));
}
// genus는kind전체WORD의TYPE로먼저계산하며부분비트/상위mask/순번을원본대로처리한다.
TEST_CASE(AiCarrierActions_ContainedGenusWordKindAndFilterOrder) {
    AiCarrierScene input;ContainedFinderState state;auto parent=input.pool.AllocatedBytes(input.actor),first=input.pool.AllocatedBytes(input.crystal),second=input.pool.AllocatedBytes(input.other);Put(parent,6,input.crystal.value,2);Put(first,4,input.other.value,2);first[10]=second[10]=6;first[11]=3;Put(first,18,158,2);Put(first,20,0x120,2);Put(second,18,158,2);Put(second,20,0x20,2);input.types[158].flags2=0x200001;CHECK(FindContainedGenus(input.pool,state,input.types,input.actor,0x100,0x200000,0)==input.crystal && FindContainedGenus(input.pool,state,input.types,input.actor,0x20,0x200000,1)==input.other && !FindContainedGenus(input.pool,state,input.types,input.actor,0,0x200003,0).value);Put(first,18,0x19e,2);Put(first,20,0,2);CHECK(Throws([&] { FindContainedGenus(input.pool,state,input.types,input.actor,0x20,0x200000,0); }));
}
// 다이스는free/dead도좌표-3이며진단효과뒤현재TYPE/좌표/공유실패점을재조회한다.
TEST_CASE(AiCarrierActions_DaisPointCurrentDiagnosticAndFallback) {
    AiCarrierScene input;auto raw=input.pool.AllocatedBytes(input.dais);Put(raw,14,std::bit_cast<std::uint32_t>(10.0F));Put(raw,18,std::bit_cast<std::uint32_t>(12.0F));raw[11]=3;RawPathPoint invalid{-10000,-10000};auto point=DaisApproachPoint(input.pool,input.types,input.checking,invalid,{},input.dais);CHECK(point.x==7 && point.y==9);input.checking=1;input.types[165].flags2=0;std::function<void(std::string_view)> assertion=[&](auto text) { CHECK(text=="dais.isValid() && dais.getGenus() & gDAIS");input.types[165].flags2=0x400000;Put(raw,14,std::bit_cast<std::uint32_t>(20.0F)); };point=DaisApproachPoint(input.pool,input.types,input.checking,invalid,assertion,input.dais);CHECK(point.x==17 && point.y==9);assertion=[&](auto) { invalid={88,99}; };point=DaisApproachPoint(input.pool,input.types,input.checking,invalid,assertion,Sid{});CHECK(point.x==88 && point.y==99);
}
