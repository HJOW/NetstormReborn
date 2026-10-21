// 부족 좌표 점수/시설별 분배/후보 승인과 실제 목록 효과를 검사한다. 작성·컴파일만 한다.
#include "o/RawAiDropEvaluation.h"
#include "o/RawAiDrySpots.h"
#include "RawSceneSupport.h"
#include <limits>
using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 같은 본체/실제 부족 좌표 backing/실제 표적 평가를 연결한 명시 입력 장면이다.
struct EvaluationScene {
    RawAiBrainMemory memory{0x51000000,[](auto raw) { std::fill(raw.begin(),raw.end(),std::uint8_t{}); }};std::uint32_t token{memory.Allocate(RawAiBrainConstructor::kBytes)},records{memory.Allocate(120)}; // 본체/12BYTE 기록의 실제 핸들이다.
    SidPool pool{OriginalEdition::Patch1078,32768,false};std::vector<RiftTypeRecord> types{188}; // 실제 물리 풀과 현재 시설/표적 TYPE다.
    std::uint32_t altar{164},directional{142},probe{71};float radius{22},rate{};Sid found{5}; // 현재 TYPE/공급반경과 명시 표적 입력이다.
    bool placement{true};std::uint32_t energy{1};std::string supply{"s"}; // 실제 하위의 명시 반환 입력이다.
    AiDropEvaluationHooks hooks;std::unique_ptr<RawAiDropTargets> targets;std::vector<std::string> trace; // 같은 실제 표적 계산기/선택된 하위와 호출 기록이다.
    std::function<void()> duringPlace,duringRate,duringCollect,duringEnergy; // 각 하위에서 현재 본체/출력 참조가 달라지는 명시 효과다.
    CanonPlacementQuery lastPlacement;std::uint32_t lastCollectOwner{},lastCollectType{},lastEnergyOwner{}; // 값으로 전달한 원본 인자의 관찰값이다.
    // 기본은 같은 적 소유자2의10점 표적, TYPE90 Gun과 near dry spot 한 개다.
    EvaluationScene() {
        CHECK(pool.Allocate(2)==Sid{5});auto raw=pool.AllocatedBytes(Sid{5});std::fill(raw.begin(),raw.end(),std::uint8_t{});raw[10]=80;raw[34]=2;Put(raw,26,1000);Field(0x260,1);Field(0x264,2);Field(0xad8,2);Field(0xadc,10);Field(0xae4,records);Field(0xaec,1);
        types[90].group=0;types[91].group=3;types[92].flags2=0x4000000;types[93].flags2=0x800;types[93].group=3;types[94].group=6;types[95].group=7;
        AiDropTargetHooks target;target.rate=[this](Sid actor) { CHECK(actor==Sid{5});trace.push_back("rate");if (duringRate) duringRate();return rate; };target.gun=[this](float,float,auto,auto direction,auto owner) { CHECK(direction==2);trace.push_back("gun:"+std::to_string(owner));return found; };target.ground=[this](float,float,auto,auto owner) { trace.push_back("ground:"+std::to_string(owner));return found; };target.blockers=[this](auto,RawPathPoint,auto capacity) { CHECK(capacity==30);trace.push_back("blockers");return std::vector<std::uint32_t>{5}; };
        targets=std::make_unique<RawAiDropTargets>(memory,pool,types,AiDropTargetGlobals{altar,directional},std::move(target));
        hooks.collect=[this](std::string& text,float,float,auto type,auto owner,auto theme) { CHECK(text.empty() && theme==-1);trace.push_back("collect");lastCollectOwner=owner;lastCollectType=type;if (duringCollect) duringCollect();text=supply; };
        hooks.mayPlace=[this](const CanonPlacementQuery& query) { trace.push_back("place");lastPlacement=query;if (duringPlace) duringPlace();return placement; };
        hooks.energy=[this](std::string& text,float,float,auto,auto owner) { CHECK(text.empty());trace.push_back("energy");lastEnergyOwner=owner;if (duringEnergy) duringEnergy();return energy; };
        hooks.dry=[this](auto brain,float x,float y,auto type) { CHECK(brain==token);trace.push_back("dry");RawAiDrySpots spots(memory,pool,{});return spots.Add(brain,x,y,type); };
        hooks.unsupported=[this](auto brain,auto type) { CHECK(brain==token);trace.push_back("unknown:"+std::to_string(type)); };hooks.dryLog=[this](auto brain,auto x,auto y) { CHECK(brain==token);trace.push_back("drylog:"+std::to_string(x)+":"+std::to_string(y)); };
        Dry({{21.25F,30.75F}});
    }
    // 현재 본체 DWORD만 입력한다.
    void Field(std::size_t at,std::uint32_t value) { Put(memory.Bytes(token),at,value); }
    // 지정 본체 DWORD를 읽는다.
    std::uint32_t Field(std::size_t at) const { return Get(memory.Bytes(token),at); }
    // 기록 TYPE와 무관한 명시 XY 입력을 현재 backing에 쓴다.
    void Dry(std::initializer_list<RawPathPoint> values) { Field(0xae0,static_cast<std::uint32_t>(values.size()));std::size_t at{};for (auto point:values) { Put(memory.Bytes(records),at,std::bit_cast<std::uint32_t>(point.x));Put(memory.Bytes(records),at+4,std::bit_cast<std::uint32_t>(point.y));Put(memory.Bytes(records),at+8,0xdeadbeefU);at+=12; } } // 반복문은 원본 기록 순서를 유지한다.
    // 같은 현재 실제 표적 계산기를 평가/승인 하위와 연결한다.
    RawAiDropEvaluation Actions() { return RawAiDropEvaluation(memory,pool,types,*targets,{radius,probe},hooks); }
};
}

// 빈/음수 count는 포인터를 읽지 않고 출력 index를 보존한다.
TEST_CASE(AiDropEvaluation_DryEmptySignedCountPreservesIndex) {
    EvaluationScene scene;scene.Field(0xae0,0);scene.Field(0xae4,0);std::int32_t index{99};CHECK(scene.Actions().DryDistance(scene.token,{20.25F,30.75F},index)==0 && index==99);scene.Field(0xae0,0xffffffffU);CHECK(scene.Actions().DryDistance(scene.token,{20.25F,30.75F},index)==0 && index==99);
}
// 정확한0거리는 제외하고 NaN/거리1000은 건너뛰며 같은 최고는 첫 index다.
TEST_CASE(AiDropEvaluation_DryNonzeroManhattanFirstTieAndNan) {
    EvaluationScene scene;scene.Dry({{20.25F,30.75F},{21.25F,30.75F},{19.25F,30.75F},{1020.25F,30.75F},{std::numeric_limits<float>::quiet_NaN(),30.75F}});std::int32_t index{99};CHECK(scene.Actions().DryDistance(scene.token,{20.25F,30.75F},index)==999 && index==1 && scene.trace.empty());
}
// 최고값 float 저장 때문에 더 가까운 둘째 기록도 축소 전 값이 저장 최고보다 작으면 유지한다.
TEST_CASE(AiDropEvaluation_DryComparesExtendedThenStoresFloat) {
    EvaluationScene scene;scene.Dry({{0.000002F,0},{0.000001F,0}});std::int32_t index{99};CHECK(scene.Actions().DryDistance(scene.token,{0,0},index)==1000 && index==0);
}
// 반경22의 거리44 경계는 제외하며 점수는 float 저장 후 절삭/길이+1로 나눈다.
TEST_CASE(AiDropEvaluation_SupplyStrictRadiusAndStoredScore) {
    EvaluationScene scene;scene.Dry({{64.25F,30.75F}});CHECK(scene.Actions().SupplyScore(scene.token,{20.25F,30.75F})==0 && scene.trace.empty());scene.Dry({{63.75F,30.75F}});CHECK(scene.Actions().SupplyScore(scene.token,{20.25F,30.75F})==478 && scene.trace==std::vector<std::string>{"collect"});scene.trace.clear();scene.radius=std::numeric_limits<float>::quiet_NaN();CHECK(scene.Actions().SupplyScore(scene.token,{20.25F,30.75F})==0 && scene.trace.empty());
}
// 공급 문자열은 첫 NUL까지 세며 빈 문자열과 signed 길이 한도를 원본대로 처리한다.
TEST_CASE(AiDropEvaluation_SupplyCstrlenAndCurrentSignedLimit) {
    EvaluationScene scene;scene.supply=std::string("s\0sss",5);scene.probe=75;scene.Field(0x260,0x101);scene.Field(0xad8,0);scene.duringCollect=[&] { scene.Field(0xad8,2);scene.Field(0x260,2);scene.probe=76; };CHECK(scene.Actions().SupplyScore(scene.token,{20.25F,30.75F})==499 && scene.lastCollectOwner==0x101 && scene.lastCollectType==75);scene.duringCollect={};scene.Field(0xad8,1);CHECK(!scene.Actions().SupplyScore(scene.token,{20.25F,30.75F}));scene.Field(0xad8,0xffffffffU);CHECK(!scene.Actions().SupplyScore(scene.token,{20.25F,30.75F}));scene.supply.clear();scene.Field(0xad8,10);CHECK(!scene.Actions().SupplyScore(scene.token,{20.25F,30.75F}));
}
// flags800은 블로커/group3보다 우선하며 공급 평가는 표적 출력을 바꾸지 않는다.
TEST_CASE(AiDropEvaluation_DispatchFlagsPrecedeGroupsAndPreserveOutput) {
    EvaluationScene scene;scene.types[93].flags2|=0x4000000;Sid target{9};CHECK(scene.Actions().Evaluate(scene.token,93,{20.25F,30.75F},2,target)==499 && target==Sid{9} && scene.trace==std::vector<std::string>{"collect"});scene.types[93].flags2=0x4000000;scene.trace.clear();CHECK(scene.Actions().Evaluate(scene.token,93,{20.25F,30.75F},2,target)==10 && target==Sid{5} && scene.trace==std::vector<std::string>({"blockers","rate"}));
}
// group3은 별도 지상 검색이고0/1은 실제 Gun 경로이며6은 진단 없이0이다.
TEST_CASE(AiDropEvaluation_DispatchGroundGunAndSilentGroupSix) {
    EvaluationScene scene;Sid target{9};CHECK(scene.Actions().Evaluate(scene.token,91,{20.25F,30.75F},2,target)==10 && scene.trace==std::vector<std::string>({"ground:1","rate"}));scene.trace.clear();CHECK(scene.Actions().Evaluate(scene.token,90,{20.25F,30.75F},2,target)==10 && scene.trace==std::vector<std::string>({"gun:1","rate"}));scene.types[90].group=1;scene.trace.clear();CHECK(scene.Actions().Evaluate(scene.token,90,{20.25F,30.75F},2,target)==10 && scene.trace==std::vector<std::string>({"gun:1","rate"}));scene.trace.clear();scene.Field(0xb00,1);target=Sid{9};CHECK(!scene.Actions().Evaluate(scene.token,94,{20.25F,30.75F},2,target) && target==Sid{9} && scene.trace.empty());
}
// 미지원 group은 출력 보존/현재b00 진단만 수행하며 잘못된 TYPE는 명시 거부한다.
TEST_CASE(AiDropEvaluation_UnknownGroupDiagnosticAndTypeBounds) {
    EvaluationScene scene;Sid target{9};CHECK(!scene.Actions().Evaluate(scene.token,95,{20.25F,30.75F},2,target) && target==Sid{9} && scene.trace.empty());scene.Field(0xb00,1);CHECK(!scene.Actions().Evaluate(scene.token,95,{20.25F,30.75F},2,target) && scene.trace==std::vector<std::string>{"unknown:95"});CHECK(Throws([&] { scene.Actions().Evaluate(scene.token,188,{20.25F,30.75F},2,target); }));
}
// 별도 검색은0도 출력하고 점수 하위 실패보다 먼저 실제 SID를 쓴다.
TEST_CASE(AiDropEvaluation_GroundResultWrittenBeforeScoreFailure) {
    EvaluationScene scene;Sid target{9};scene.found={};CHECK(!scene.Actions().Evaluate(scene.token,91,{20.25F,30.75F},2,target) && !target.value);scene.found=Sid{5};scene.duringRate=[&] { CHECK(target==Sid{5});throw std::logic_error("명시 점수 getter 실패"); };CHECK(Throws([&] { scene.Actions().Evaluate(scene.token,91,{20.25F,30.75F},2,target); }) && target==Sid{5});
}
// 전체 배치 검사는 방향0/소유자BYTE/mode1이며 거부 시 이후 평가·출력도 없다.
TEST_CASE(AiDropEvaluation_PlacementGateCanonicalArguments) {
    EvaluationScene scene;scene.Field(0x260,0x101);scene.placement=false;RawPathPoint best{7,8};std::int32_t score{3};Sid target{9};CHECK(!scene.Actions().Consider(scene.token,90,{20.25F,30.75F},2,best,score,target));const auto& query=scene.lastPlacement;CHECK(query.type==90 && !query.argument && !query.flags && query.owner==1 && query.mode==1 && query.x==20.25F && query.y==30.75F);CHECK(best.x==7 && best.y==8 && score==3 && target==Sid{9} && scene.trace==std::vector<std::string>{"place"});
}
// 평가/에너지는 앞선 하위 효과 뒤 현재 full owner를 다시 읽는다.
TEST_CASE(AiDropEvaluation_CurrentOwnerBetweenPlacementScoreAndEnergy) {
    EvaluationScene scene;scene.Field(0x260,0x101);scene.duringPlace=[&] { scene.Field(0x260,0x102); };scene.duringRate=[&] { scene.Field(0x260,0x103); };RawPathPoint best{7,8};std::int32_t score{};Sid target{9};scene.energy=0x80000000U;CHECK(scene.Actions().Consider(scene.token,90,{20.25F,30.75F},2,best,score,target));CHECK(scene.lastPlacement.owner==1 && scene.lastEnergyOwner==0x103 && best.x==20.25F && best.y==30.75F && score==10 && target==Sid{5});CHECK(scene.trace==std::vector<std::string>({"place","gun:258","rate","energy"}));
}
// 양수 동률은 승인하고0/음수 동률은 승인하지 않는다. 음수도 개선이면 승인할 수 있다.
TEST_CASE(AiDropEvaluation_PositiveTieAndSignedImprovement) {
    EvaluationScene scene;RawPathPoint best{7,8};std::int32_t score{10};Sid target{9};CHECK(scene.Actions().Consider(scene.token,90,{20.25F,30.75F},2,best,score,target) && score==10 && best.x==20.25F);scene.trace.clear();score=0;best={7,8};CHECK(!scene.Actions().Consider(scene.token,94,{20.25F,30.75F},2,best,score,target) && scene.trace==std::vector<std::string>{"place"});scene.rate=-11;scene.trace.clear();score=-1;CHECK(!scene.Actions().Consider(scene.token,90,{20.25F,30.75F},2,best,score,target) && scene.trace==std::vector<std::string>({"place","gun:1","rate"}));score=-2;CHECK(scene.Actions().Consider(scene.token,90,{20.25F,30.75F},2,best,score,target) && score==-1);
}
// 후보를 채택하지 않아도 실제 표적 SID의 평가 효과를 보존하고 최고 좌표/점수는 유지한다.
TEST_CASE(AiDropEvaluation_WorseScoreKeepsTargetSideEffect) {
    EvaluationScene scene;RawPathPoint best{7,8};std::int32_t score{20};Sid target{9};CHECK(!scene.Actions().Consider(scene.token,90,{20.25F,30.75F},2,best,score,target));CHECK(target==Sid{5} && best.x==7 && best.y==8 && score==20 && scene.trace==std::vector<std::string>({"place","gun:1","rate"}));
}
// 실제 부족 좌표 추가는 에너지 거부 뒤이며 중복 실패는 로그도 반복하지 않는다.
TEST_CASE(AiDropEvaluation_EnergyFailureUsesActualDryBackingAndCurrentLog) {
    EvaluationScene scene;scene.Field(0xae0,0);scene.energy=0;scene.duringEnergy=[&] { scene.Field(0xb00,1); };RawPathPoint best{7,8};std::int32_t score{};Sid target{9};CHECK(!scene.Actions().Consider(scene.token,90,{20.25F,30.75F},2,best,score,target));CHECK(scene.Field(0xae0)==1 && Get(scene.memory.Bytes(scene.records),0)==std::bit_cast<std::uint32_t>(20.25F) && Get(scene.memory.Bytes(scene.records),4)==std::bit_cast<std::uint32_t>(30.75F) && Get(scene.memory.Bytes(scene.records),8)==90);CHECK(best.x==7 && score==0 && target==Sid{5} && scene.trace==std::vector<std::string>({"place","gun:1","rate","energy","dry","drylog:20:30"}));scene.trace.clear();CHECK(!scene.Actions().Consider(scene.token,90,{20.25F,30.75F},2,best,score,target) && scene.Field(0xae0)==1 && scene.trace==std::vector<std::string>({"place","gun:1","rate","energy","dry"}));
}
// 선택된 하위 미연결은 평가가 이미 쓴 표적을 보존하고 최고 좌표/점수는 바꾸지 않는다.
TEST_CASE(AiDropEvaluation_SelectedMissingHooksPreservePrefix) {
    EvaluationScene scene;scene.hooks.energy={};RawPathPoint best{7,8};std::int32_t score{};Sid target{9};CHECK(Throws([&] { scene.Actions().Consider(scene.token,90,{20.25F,30.75F},2,best,score,target); }) && target==Sid{5} && best.x==7 && score==0);scene.energy=0;scene.hooks.energy=[&](std::string&,float,float,auto,auto) { return 0U; };scene.hooks.dry={};CHECK(Throws([&] { scene.Actions().Consider(scene.token,90,{20.25F,30.75F},2,best,score,target); }) && !scene.Field(0xb00));
}
