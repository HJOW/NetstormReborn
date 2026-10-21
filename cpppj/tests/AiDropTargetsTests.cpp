// 디컴파일에서 누락된 점수·실제 조회 순서·첫 SID 출력의 경계를 검사한다. 작성·컴파일만 한다.
#include "o/RawAiDropTargets.h"
#include "RawSceneSupport.h"
#include <limits>
using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 실제 본체/목록/슬롯에 명시 입력을 넣고 실제 하위의 호출 경계를 관찰한다.
struct AiDropScene {
    RawAiBrainMemory memory{0x50000000,[](auto raw) { std::fill(raw.begin(),raw.end(),std::uint8_t{}); }}; // 실제 AI 본체와 두 목록 저장소다.
    std::uint32_t token{memory.Allocate(RawAiBrainConstructor::kBytes)},first{memory.Allocate(32)},second{memory.Allocate(32)}; // 각 실제 블록의 핸들이다.
    SidPool pool{OriginalEdition::Patch1078,32768,false};std::vector<RiftTypeRecord> types{188}; // 실제 PATCH 슬롯과 현재 타입 표다.
    Sid target{5},other{6};std::uint32_t altar{164},directional{142}; // 입력 SID와 현재 TYPE 전역이다.
    std::array<float,188> rates{};std::int32_t maximum{1000};Sid captive{},found{};std::vector<std::uint32_t> candidates;RawPathPoint center{20,29}; // 명시 하위 입력이다.
    AiDropTargetHooks hooks;std::vector<std::string> trace; // 선택된 실제 하위와 호출 순서다.
    // 기본은 적 owner2의 평범한 TYPE80/81 표적과 비어 있는 원본 두 목록이다.
    AiDropScene() {
        // 실제 할당 순서와 지정 슬롯 입력만 준비한다.
        for (std::uint16_t id=5;id<=9;++id) { const Sid actor{id};CHECK(pool.Allocate(2)==actor);auto raw=pool.AllocatedBytes(actor);std::fill(raw.begin(),raw.end(),std::uint8_t{});raw[10]=static_cast<std::uint8_t>(75+id);raw[34]=2;Put(raw,26,1000); }
        Field(0x36c,first);Field(0x378,second);Field(0x260,1);Field(0x264,2);
        hooks.captive=[this](Sid actor) { trace.push_back("captive:"+std::to_string(actor.value));return captive; };
        hooks.rate=[this](Sid actor) { trace.push_back("rate:"+std::to_string(actor.value));return rates[pool.Slot(actor)[10]]; };
        hooks.maxHitPoints=[this](Sid actor) { trace.push_back("max:"+std::to_string(actor.value));return maximum; };
        hooks.gun=[this](float x,float y,auto type,auto direction,auto owner) { CHECK(x==20 && y==30 && type==128 && direction==2 && owner==1);trace.push_back("gun");return found; };
        hooks.blockers=[this](auto owner,RawPathPoint point,auto capacity) { CHECK(owner==1 && point.x==20 && point.y==30 && capacity==30);trace.push_back("blockers");return candidates; };
        hooks.center=[this](Sid actor) { trace.push_back("center:"+std::to_string(actor.value));return center; };
        hooks.assertion=[this](std::string_view text,auto line) { CHECK(text=="sid" && line==0xaeb);trace.push_back("assert"); };
    }
    // 지정 본체 DWORD만 입력한다.
    void Field(std::size_t at,std::uint32_t value) { Put(memory.Bytes(token),at,value); }
    // 지정 목록의 실제 backing에 중복을 그대로 입력한다.
    void List(std::size_t pointer,std::initializer_list<Sid> values) { Field(pointer+8,static_cast<std::uint32_t>(values.size()));const auto raw=memory.Bytes(Get(memory.Bytes(token),pointer));std::size_t at{};for (Sid value:values) { Put(raw,at,value.value);at+=4; } } // 반복문은 입력 등장 순서를 보존한다.
    // 같은 실제 저장소에 처리기를 연결하며 호출 자체는 각 검사에서 선택한다.
    RawAiDropTargets Actions() { return RawAiDropTargets(memory,pool,types,{altar,directional},hooks); }
    // 현재 TYPE/호출 하위의 점수를 읽는다.
    std::int32_t Score() { auto actions=Actions();CHECK(&actions.Pool()==&pool);return actions.Score(token,target); }
};
}

// 원본 정수 기본/사제/건물/group6/제단 가산을 각각 검사한다.
TEST_CASE(AiDropTargets_NativeBasePriestBuildingGroupAndAltarBonuses) {
    AiDropScene scene;CHECK(scene.Score()==10);scene.types[80].flags2=0x4000000;CHECK(scene.Score()==5);scene.types[80].flags2=0x200000;CHECK(scene.Score()==210);scene.Field(0x2a8,9);CHECK(scene.Score()==15);scene.types[80].flags2=0x200;CHECK(scene.Score()==16);scene.types[80].flags2=0;scene.types[80].group=6;CHECK(scene.Score()==17);scene.types[80].group=0;scene.altar=80;CHECK(scene.Score()==18);
}
// 정수 가산과 현재 hpPerSec를 합한 다음 ftol하며 NaN의 하위DWORD0을 보존한다.
TEST_CASE(AiDropTargets_RateIsAddedBeforeTruncationAndNonfinite) {
    AiDropScene scene;scene.rates[80]=-0.9F;CHECK(scene.Score()==9);scene.rates[80]=std::numeric_limits<float>::quiet_NaN();CHECK(scene.Score()==0);scene.rates[80]=1.75F;CHECK(scene.Score()==11);
}
// 포획 사제400점은 운반 genus30000에서만 조회하며 앞선 flags/group은 보존한다.
TEST_CASE(AiDropTargets_CaptiveBonusAndCurrentRateAfterLookup) {
    AiDropScene scene;scene.types[80].flags2=0x10000;scene.types[80].group=6;scene.rates[81]=23;scene.hooks.captive=[&](Sid actor) { CHECK(actor==scene.target);scene.trace.push_back("captive");scene.pool.AllocatedBytes(actor)[10]=81;return scene.other; };CHECK(scene.Score()==440);CHECK(scene.trace==std::vector<std::string>({"captive","rate:5"}));scene.trace.clear();CHECK(scene.Score()==33 && scene.trace==std::vector<std::string>{"rate:5"});
}
// 모든 실제 가산을 한 번씩 더하고 제단 TYPE는 현재 전역을 사용한다.
TEST_CASE(AiDropTargets_AllNativeBonusesAreCombined) {
    AiDropScene scene;scene.types[80].flags2=0x4000000|0x200000|0x200|0x10000;scene.types[80].group=6;scene.altar=80;scene.captive=scene.other;scene.rates[80]=1.75F;CHECK(scene.Score()==627);
}
// 두 목록은 각 첫 일치에만100을 더하며 하위 효과 뒤 현재 목록을 읽는다.
TEST_CASE(AiDropTargets_ListBonusesDuplicateAndCurrentBacking) {
    AiDropScene scene;scene.List(0x36c,{scene.target,scene.target});scene.List(0x378,{scene.target,scene.target});CHECK(scene.Score()==210);scene.hooks.rate=[&](Sid actor) { CHECK(actor==scene.target);scene.Field(0x374,0);return 0.0F; };CHECK(scene.Score()==110);scene.Field(0x380,0xffffffffU);CHECK(scene.Score()==10);
}
// 집중 대상의 HP는 getter 전 저장하고 signed 최대HP/2는0쪽 나눗셈이다.
TEST_CASE(AiDropTargets_FocusSavesHpAndUsesSignedHalf) {
    AiDropScene scene;scene.Field(0x3f8,scene.target.value);Put(scene.pool.AllocatedBytes(scene.target),26,0xffffffffU);scene.hooks.maxHitPoints=[&](Sid actor) { CHECK(actor==scene.target);scene.trace.push_back("max");Put(scene.pool.AllocatedBytes(actor),26,0xfffffff6U);return -3; };CHECK(scene.Score()==60 && scene.trace==std::vector<std::string>({"rate:5","max"}));
}
// 마지막 ownerBYTE는 전체 enemyDWORD와 비교하고 음수 점수도0쪽으로 절반을 낸다.
TEST_CASE(AiDropTargets_EnemyOwnerFullDwordAndCurrentGetterEffects) {
    AiDropScene scene;scene.rates[80]=-21;CHECK(scene.Score()==-11);scene.Field(0x264,0x102);CHECK(scene.Score()==-5);scene.Field(0x264,2);scene.Field(0x3f8,scene.target.value);scene.hooks.maxHitPoints=[&](Sid actor) { scene.pool.AllocatedBytes(actor)[34]=3;scene.Field(0x264,3);return 99999; };CHECK(scene.Score()==-11);
}
// 32비트 가산은 signed overflow 대신 원본 하위DWORD 감김을 유지한다.
TEST_CASE(AiDropTargets_ListAdditionWrapsDword) {
    AiDropScene scene;scene.rates[80]=2147483520.0F;scene.List(0x36c,{scene.target});scene.List(0x378,{scene.target});CHECK(scene.Score()==-2147483566);
}
// 물리SID0의 운반체 진단은 선택된 곳에서 호출하고 실제 내용물 조회를 계속한다.
TEST_CASE(AiDropTargets_PhysicalZeroCarrierAssertionContinues) {
    AiDropScene scene;const auto zeroType=scene.pool.Slot(Sid{})[10];scene.types[zeroType].flags2=0x10000;scene.Field(0x3f8,9);scene.Actions().Score(scene.token,Sid{});CHECK(scene.trace==std::vector<std::string>({"assert","captive:0","rate:0"}));
}
// 실제 검색 결과는 점수 계산보다 먼저 쓰고 빈 검색도0으로 덮어쓴다.
TEST_CASE(AiDropTargets_GunWritesResultBeforeScoreAndZero) {
    AiDropScene scene;Sid result{9};auto actions=scene.Actions();CHECK(actions.Gun(scene.token,128,{20,30},2,result)==0 && !result.value);scene.found=scene.target;scene.hooks.rate=[&](Sid actor) { CHECK(actor==scene.target && result==scene.target);return 5.0F; };CHECK(scene.Actions().Gun(scene.token,128,{20,30},2,result)==15 && result==scene.target);scene.hooks.rate={};CHECK(Throws([&] { scene.Actions().Gun(scene.token,128,{20,30},2,result); }) && result==scene.target);
}
// 최고 점수의 항목이 둘째여도 원본은 첫 SID를 출력하며 중복 총점은 모두 센다.
TEST_CASE(AiDropTargets_BlockersSumAndNativeFirstEntryOutput) {
    AiDropScene scene;scene.rates[80]=-10;scene.rates[81]=20;scene.candidates={5,6,6};Sid result{9};CHECK(scene.Actions().Blockers(scene.token,78,{20,30},2,result)==60 && result==scene.target);CHECK(scene.trace==std::vector<std::string>({"blockers","rate:5","rate:6","rate:6"}));
}
// 양의 최고가 없으면 출력은 보존하고 음수/0도 총점에 포함한다.
TEST_CASE(AiDropTargets_BlockersNonpositivePreservesOutputAndWrapsTotal) {
    AiDropScene scene;scene.rates[80]=-11;scene.candidates={5,5};Sid result{9};CHECK(scene.Actions().Blockers(scene.token,78,{20,30},2,result)==-2 && result==Sid{9});scene.rates[80]=2147483520.0F;CHECK(scene.Actions().Blockers(scene.token,78,{20,30},2,result)==-236 && result==scene.target);scene.candidates.clear();result=Sid{9};CHECK(scene.Actions().Blockers(scene.token,78,{20,30},2,result)==0 && result==Sid{9});
}
// 방향 블로커 필터는 점수 뒤 두 축-1을 저장하고 현재 전역 TYPE에만 적용한다.
TEST_CASE(AiDropTargets_DirectionalFilterAfterScoringAndCurrentGlobal) {
    AiDropScene scene;scene.candidates={5};scene.center={19,29.5F};Sid result{9};CHECK(scene.Actions().Blockers(scene.token,142,{20,30},4,result)==10 && result==scene.target);CHECK(scene.trace==std::vector<std::string>({"blockers","rate:5","center:5"}));scene.trace.clear();result=Sid{9};CHECK(scene.Actions().Blockers(scene.token,142,{20,30},2,result)==0 && result==Sid{9});scene.types[80].flags2=0x100000;scene.trace.clear();CHECK(scene.Actions().Blockers(scene.token,142,{20,30},0,result)==0);CHECK(scene.trace==std::vector<std::string>({"blockers","rate:5"}));
    scene.types[80].flags2=0;scene.directional=78;scene.trace.clear();CHECK(scene.Actions().Blockers(scene.token,142,{20,30},2,result)==10 && scene.trace==std::vector<std::string>({"blockers","rate:5"}));
}
// 홀수 방향도 원문대로 중심을 먼저 읽고 제외하며 대각 동률은 세로다.
TEST_CASE(AiDropTargets_FacingOddDirectionAndVerticalTie) {
    AiDropScene scene;scene.center={21,29};CHECK(scene.Actions().Facing({20,30},0,scene.target));scene.trace.clear();CHECK(!scene.Actions().Facing({20,30},1,scene.target));CHECK(scene.trace==std::vector<std::string>{"center:5"});
}
