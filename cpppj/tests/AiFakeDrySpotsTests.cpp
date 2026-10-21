// 원본 전체 가짜 부족좌표의 현재 상태/선행 제거/순서·일부 효과를 검사한다.작성·컴파일만 한다.
#include "o/RawAiFakeDrySpots.h"
#include "o/RawAiDrySpots.h"
#include "RawSceneSupport.h"
#include <limits>
using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 실제 본체/12바이트 목록과 기존 실제 Add/Remove/Seed를 같은 저장소에서 사용한다.
struct FakeDryScene {
    RawAiBrainMemory memory{0x54000000,[](auto raw) { std::fill(raw.begin(),raw.end(),std::uint8_t{}); }};std::uint32_t token{memory.Allocate(RawAiBrainConstructor::kBytes)},records{memory.Allocate(120)},suppliers{memory.Allocate(32)}; //실제 본체와 두 목록 backing이다.
    SidPool pool{OriginalEdition::Patch1078,32768,false};GameRandom random{8};float radius{22};std::int32_t limit{256};std::uint32_t debug{99}; //같은 실제 풀/공용 난수와 현재 전역이다.
    RawPathPoint center{40,60},candidate{100,100};std::vector<float> scores{100};std::vector<std::int32_t> indices{0}; //명시 중심/경로/점수 하위의 입력이다.
    AiFakeDryHooks hooks;std::vector<std::string> trace;std::vector<RawPathPoint> scorePoints,sources,targets;std::vector<Sid> centers;std::vector<AiFakeDryLog> logs;std::vector<std::array<std::int32_t,2>> logPoints; //원문 호출 순서/현재 인자다.
    std::function<void(std::size_t)> duringScore;std::function<void()> duringCenter,duringCandidate,duringAdd,duringMark,duringLog; //실제 효과 뒤의 명시 상태 변경이다.
    // 기본은 유효한 현재 가짜좌표와 한 부족기록이며 공급시설/신전은 caller가 선택한다.
    FakeDryScene() {
        Field(0x280,73);Field(0xadc,10);Field(0xae0,0);Field(0xae4,records);Field(0xaec,2);Field(0x2f4,suppliers);Field(0x2f8,8);Field(0x2fc,0);Field(0xaf0,std::bit_cast<std::uint32_t>(90.0F));Field(0xaf4,std::bit_cast<std::uint32_t>(90.0F));Field(0x260,2);Field(0xb00,1);CHECK(Dry().Add(token,150,160,71));
        hooks.score=[this](auto brain,RawPathPoint point,auto& index) { CHECK(brain==token);const auto call=scorePoints.size();scorePoints.push_back(point);trace.push_back("score");index=indices.at(std::min(call,indices.size()-1));const auto value=scores.at(std::min(call,scores.size()-1));if (duringScore) duringScore(call);return value; };
        hooks.center=[this](Sid sid) { centers.push_back(sid);trace.push_back("center");if (duringCenter) duringCenter();return center; };hooks.candidate=[this](RawPathPoint source,RawPathPoint target) { sources.push_back(source);targets.push_back(target);trace.push_back("candidate");if (duringCandidate) duringCandidate();return candidate; };
        hooks.add=[this](auto brain,RawPathPoint point,auto type) { CHECK(brain==token);trace.push_back("add");const auto result=Dry().Add(brain,point.x,point.y,type);if (duringAdd) duringAdd();return result; };hooks.remove=[this](auto brain,auto index) { CHECK(brain==token && index==0);trace.push_back("remove");Dry().Remove(brain,index); };hooks.seed=[this](auto brain) { CHECK(brain==token);trace.push_back("seed");Dry().Seed(brain,random); };
        hooks.mark=[this](RawPathPoint point) { CHECK(point.x==candidate.x && point.y==candidate.y);trace.push_back("mark");if (duringMark) duringMark(); };hooks.log=[this](auto brain,AiFakeDryLog action,auto x,auto y) { CHECK(brain==token);trace.push_back(action==AiFakeDryLog::Added ? "added" : "discarded");logs.push_back(action);logPoints.push_back({x,y});if (duringLog) duringLog(); };
    }
    // 현재 본체 DWORD를 읽는다.
    std::uint32_t Field(std::size_t at) const { return Get(memory.Bytes(token),at); }
    // 지정 본체 DWORD를 쓴다.
    void Field(std::size_t at,std::uint32_t value) { Put(memory.Bytes(token),at,value); }
    // 기존 실제 Dry Spot 모듈을 같은 저장소에 연결한다.
    RawAiDrySpots Dry() { return RawAiDrySpots(memory,pool,{}); }
    // 같은 실제 풀/본체와 명시 하위로 전체 루틴을 조립한다.
    RawAiFakeDrySpots Make() { return RawAiFakeDrySpots(memory,pool,{radius,limit,debug},hooks); }
    // 원래 공급시설 목록에 사용할 실제 raw 객체를 만든다.
    Sid Object(float x,float y) { const auto sid=pool.Allocate(2);auto raw=pool.AllocatedBytes(sid);std::fill(raw.begin(),raw.end(),std::uint8_t{});raw[10]=136;Put(raw,14,std::bit_cast<std::uint32_t>(x));Put(raw,18,std::bit_cast<std::uint32_t>(y));return sid; }
    // 현재 공급시설의 SID DWORD 목록을 설정한다.
    void Supply(std::initializer_list<Sid> values) { Field(0x2fc,static_cast<std::uint32_t>(values.size()));std::size_t index{};for (auto sid:values) Put(memory.Bytes(suppliers),index++*4,sid.value); } //반복문은 지정한 SID 순서만 기록한다.
};
}
// fake TYPE0이면 다른 관문/목록/실제 효과를 전혀 요청하지 않는다.
TEST_CASE(AiFakeDrySpots_ZeroTypeSkipsFullAndAllEffects) {
    FakeDryScene input;input.Field(0x280,0);input.Field(0xadc,0);input.Field(0xae4,0);input.hooks={};input.Make().Extend(input.token);CHECK(input.Field(0xae0)==1 && input.trace.empty() && input.random.State()==8);
}
// full 제거는 신전이 없는 반환보다 먼저이며 cursor/가짜좌표를 보정하지 않는다.
TEST_CASE(AiFakeDrySpots_FullRemovalPrecedesMissingTemple) {
    FakeDryScene input;input.Field(0xadc,1);input.Field(0xae8,7);input.Make().Extend(input.token);CHECK(input.trace==std::vector<std::string>({"remove"}) && !input.Field(0xae0) && input.Field(0xae8)==7 && input.Field(0xaf0)==std::bit_cast<std::uint32_t>(90.0F));
}
// 공급시설0은 중심 대신 신전 raw좌표를 사용하며 중복 실패에서도 원래 Added 로그를 남긴다.
TEST_CASE(AiFakeDrySpots_TempleUsesRawPointAndIgnoresDuplicate) {
    FakeDryScene input;const auto temple=input.Object(150,160);input.Field(0x294,temple.value);input.Make().Extend(input.token);CHECK(input.trace==std::vector<std::string>({"add","added"}) && input.Field(0xae0)==1 && input.centers.empty() && input.sources.empty() && input.Field(0xaf0)==std::bit_cast<std::uint32_t>(90.0F));CHECK((input.logs==std::vector<AiFakeDryLog>({AiFakeDryLog::Added}) && input.logPoints[0]==std::array<std::int32_t,2>({150,160})));
}
// 공급목록 포인터/count는 진입cache이며 각 객체의 raw좌표와 첫 최고점수 규칙은 유지한다.
TEST_CASE(AiFakeDrySpots_CachedSupplyListAndFirstMaximum) {
    FakeDryScene input;const auto first=input.Object(20,30),second=input.Object(50,60);input.Supply({first,second});input.scores={10,10};const auto other=input.memory.Allocate(4);Put(input.memory.Bytes(other),0,65535);input.duringScore=[&](auto call) { if (!call) { input.Field(0x2f4,other);input.Field(0x2fc,0);Put(input.pool.AllocatedBytes(second),14,std::bit_cast<std::uint32_t>(55.0F)); } };input.Make().Extend(input.token);CHECK(input.scorePoints.size()==2 && input.scorePoints[1].x==55 && input.centers==std::vector<Sid>({first}) && input.sources[0].x==60 && input.sources[0].y==60 && input.targets[0].x==149 && input.targets[0].y==159);
}
// 중심 효과 뒤 현재 Dry Spot pointer를 사용하되 점수에서 선택한 index는 보존한다.
TEST_CASE(AiFakeDrySpots_CurrentDryPointerAfterCenter) {
    FakeDryScene input;CHECK(input.Dry().Add(input.token,180,190,71));input.Supply({input.Object(179,190)});input.scores={999};input.indices={1};const auto replacement=input.memory.Allocate(24);Put(input.memory.Bytes(replacement),12,std::bit_cast<std::uint32_t>(181.0F));Put(input.memory.Bytes(replacement),16,std::bit_cast<std::uint32_t>(191.0F));input.duringCenter=[&] { input.Field(0xae4,replacement);input.Field(0xae0,1);input.Field(0xadc,2); };input.Make().Extend(input.token);CHECK(input.targets[0].x==180 && input.targets[0].y==190 && input.Field(0xae0)==2);
}
// 현재 공급반경의0.7배보다 커야 채택하며 NaN 반경도 폐기한다.거리 중심은 X/Y 원래 쌍이다.
TEST_CASE(AiFakeDrySpots_RadiusOrderedGateUsesOriginalCenter) {
    // 원본 float0.7을 넓힌 임계값과 거리의 양쪽을 서로 다른 실제 반경으로 지정한다.
    for (float radius:{20.0F,30.0F,std::numeric_limits<float>::quiet_NaN()}) { FakeDryScene input;input.Supply({input.Object(20,30)});input.center={40,60};input.candidate={60,60};input.radius=radius;input.Make().Extend(input.token);const bool accepted=radius==20.0F;CHECK((input.Field(0xae0)==2)==accepted && input.logs.back()==(accepted ? AiFakeDryLog::Added : AiFakeDryLog::Discarded)); }
}
// 유효성 경계/NaN 후보에서는 거리/등록/폐기 로그 없이 현재 가짜좌표만 확인한다.
TEST_CASE(AiFakeDrySpots_InvalidCandidateHasNoDiscardLog) {
    // 축0/상한/NaN은 원본 strict 좌표 검사의 서로 다른 거부 입력이다.
    for (float coordinate:{0.0F,256.0F,std::numeric_limits<float>::quiet_NaN()}) { FakeDryScene input;input.Supply({input.Object(20,30)});input.candidate={coordinate,100};input.Make().Extend(input.token);CHECK(input.Field(0xae0)==1 && input.logs.empty() && input.trace==std::vector<std::string>({"score","center","candidate"})); }
}
// signed 음수 공급count는 탐색을 건너뛰고 유효하지 않은 현재 가짜좌표만 실제Seed로 보충한다.
TEST_CASE(AiFakeDrySpots_NegativeCountFallsBackWithoutLookup) {
    FakeDryScene input;input.Field(0x2fc,0xffffffff);input.Field(0x2f4,0);input.Field(0xaf0,0);input.Field(0x248,0);input.Make().Extend(input.token);CHECK(input.trace==std::vector<std::string>({"seed"}) && input.random.State()==8 && input.logs.empty());
}
// 실제Add가false여도 선택좌표/전략4를 갱신하고 현재 디버그 표식/로그를 원래 순서로 실행한다.
TEST_CASE(AiFakeDrySpots_AddFailureStillUpdatesStrategyAndMarks) {
    FakeDryScene input;input.Supply({input.Object(20,30)});CHECK(input.Dry().Add(input.token,100,100,99));input.Field(0x2d4,4);input.Field(0x3cc,7);input.Field(0x3d0,9);input.Field(0x3d4,11);input.duringAdd=[&] { input.Field(0x280,77);input.debug=29; };input.Make().Extend(input.token);CHECK(input.Field(0xae0)==2 && input.Field(0xaf0)==std::bit_cast<std::uint32_t>(100.0F) && input.Field(0xaf4)==std::bit_cast<std::uint32_t>(100.0F) && !input.Field(0x3cc) && input.Field(0x3d0)==0xc61c4000 && input.Field(0x3d4)==0xc61c4000 && input.Field(0x3d8)==1);CHECK(input.trace==std::vector<std::string>({"score","center","candidate","add","mark","added"}) && Get(input.memory.Bytes(input.records),20)==99);
}
// 채택 뒤 표식이 현재 가짜좌표를 무효화하면 실제Seed가 뒤따르지만 최종 로그는 원래 후보다.
TEST_CASE(AiFakeDrySpots_SeedAfterMarkKeepsOriginalLogPoint) {
    FakeDryScene input;input.Supply({input.Object(20,30)});input.debug=29;input.Field(0x248,0x20);input.Field(0x270,std::bit_cast<std::uint32_t>(10.0F));input.Field(0x274,std::bit_cast<std::uint32_t>(20.0F));input.Field(0x278,std::bit_cast<std::uint32_t>(30.0F));input.Field(0x27c,std::bit_cast<std::uint32_t>(40.0F));input.duringMark=[&] { input.Field(0xaf0,0); };input.Make().Extend(input.token);CHECK(input.trace==std::vector<std::string>({"score","center","candidate","add","mark","seed","added"}));CHECK((input.logPoints[0]==std::array<std::int32_t,2>({100,100})));CHECK(input.Field(0xaf0)==std::bit_cast<std::uint32_t>(10.0F) && input.Field(0xaf4)==std::bit_cast<std::uint32_t>(20.0F) && input.Field(0xae0)==3 && input.random.State()!=8);
}
// 후보 계산 뒤 현재 TYPE/전략과 b00을 읽고 출력이 꺼진 경우 실제 로그를 요구하지 않는다.
TEST_CASE(AiFakeDrySpots_CurrentTypeAndDebugAfterCandidate) {
    FakeDryScene input;input.Supply({input.Object(20,30)});input.duringCandidate=[&] { input.Field(0x280,0xdeadbeef);input.Field(0xb00,0);input.Field(0x2d4,3); };input.hooks.log={};input.Make().Extend(input.token);CHECK(input.Field(0xae0)==2 && Get(input.memory.Bytes(input.records),20)==0xdeadbeef && !input.Field(0x3d8) && input.logs.empty());
}
// 선택된 실제 경로/표식 누락에서도 이미 완료한 full제거·좌표/전략 쓰기는 유지한다.
TEST_CASE(AiFakeDrySpots_SelectedMissingPreservesPartialWrites) {
    FakeDryScene input;CHECK(input.Dry().Add(input.token,180,190,71));input.Field(0xadc,2);input.Supply({input.Object(20,30)});input.hooks.candidate={};CHECK(Throws([&] { input.Make().Extend(input.token); }) && input.Field(0xae0)==1);FakeDryScene marked;marked.Supply({marked.Object(20,30)});marked.debug=29;marked.Field(0x2d4,4);marked.hooks.mark={};CHECK(Throws([&] { marked.Make().Extend(marked.token); }) && marked.Field(0xae0)==2 && marked.Field(0xaf0)==std::bit_cast<std::uint32_t>(100.0F) && marked.Field(0x3d8)==1 && marked.logs.empty());
}
