// AI 다리의 실제 단계 함수 출력과 배치/내용물/timeout 경계를 검사한다.
#include "o/RawAiBridgePath.h"
#include "RawSceneSupport.h"
#include <limits>
using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 같은 실제 본체/풀과 하위 호출의 순서·현재 값 변경을 조립한다.
struct BridgePathScene {
    RawAiBrainMemory memory{0x55000000,[](auto raw) { std::fill(raw.begin(),raw.end(),std::uint8_t{}); }}; //원본 본체의 실제 소유 메모리다.
    std::uint32_t token{memory.Allocate(RawAiBrainConstructor::kBytes)}; //현재 살아 있는 본체 핸들이다.
    SidPool pool{OriginalEdition::Patch1078,32768,false}; //내용물 체인의 실제 풀이다.
    ContainedFinderState finder; //현재 내용물 TYPE/권한이다.
    double now{100.00000001}; //QWORD 기한의 정밀도를 확인하는 현재 시각이다.
    std::int32_t limit{256}; //원본 지도 상한이다.
    std::uint32_t bridge{82},kind{1},startMarker{93},endMarker{94},targetMarker{95}; //각 원본 전역 TYPE/로그 값이다.
    AiBridgePathHooks hooks; //실제 하위 경계의 명시 입력이다.
    std::vector<std::string> trace; //하위 진입 순서다.
    std::vector<Sid> destroyed; //소비한 실제 내용물 번호다.
    std::uint32_t mode{}; //native fixture와 같은 false/true/현재 상태 변경 입력이다.
    RawPathPoint first{20,21}; //시작점 하위가 반환하는 명시 좌표다.
    std::function<void()> duringDeploy,duringTechnology,duringDestroy,duringRefresh,duringMarker; //효과 뒤 현재 본체/전역 변경이다.
    // 실제 부모/내용물 다섯 슬롯을 생성하고 원문 단계 하위만 연결한다.
    BridgePathScene() {
        // 연속된 실제 SID의 링크/필드 초기값을 명확히 한다.
        for (std::uint16_t number=5;number<=9;++number) { const auto sid=pool.Allocate(2);CHECK(sid.value==number);auto raw=pool.AllocatedBytes(sid);std::fill(raw.begin(),raw.end(),std::uint8_t{});raw[10]=number==5 ? 1 : 6; }
        Field(0x260,1);Field(0x3d8,7);Point(0x6bc,{101,202});Point(0x6ac,{11,12});Field(0x6b4,3);Field(0x6b8,2);Field(0x600,std::bit_cast<std::uint32_t>(0.25F));
        hooks.initialize=[this](auto brain,RawPathPoint source,RawPathPoint target) { CHECK(brain==token && source.x==10 && source.y==20 && target.x==30 && target.y==40);trace.push_back("initialize");if (mode==2) Field(0x5f0,0xffffffffU); };
        hooks.prepare=[this](auto brain) { CHECK(brain==token);trace.push_back("prepare");if (mode==2) Field(0x5f0,0xffffffffU);return mode!=0; };
        hooks.search=[this](auto brain) { CHECK(brain==token);trace.push_back("search");if (mode==2) Field(0x5f0,0xffffffffU);return mode!=0; };
        hooks.start=[this](auto brain,RawPathPoint) { CHECK(brain==token);trace.push_back("start");return first; };
        hooks.deploy=[this](auto brain,auto type,RawPathPoint point,auto pattern,auto direction) { CHECK(brain==token && type==bridge && point.x==11 && point.y==12 && pattern==3 && direction==2);trace.push_back("deploy");if (duringDeploy) duringDeploy(); };
        hooks.technology=[this](auto owner) { trace.push_back("technology"+std::to_string(owner));if (duringTechnology) duringTechnology();return Sid{5}; };
        hooks.destroy=[this](Sid sid,auto flags) { CHECK(flags==0);trace.push_back("destroy");destroyed.push_back(sid);if (duringDestroy) duringDestroy(); };
        hooks.refresh=[this](auto brain,Sid parent) { CHECK(brain==token && parent==Sid{5});trace.push_back("refresh");if (duringRefresh) duringRefresh(); };
        hooks.marker=[this](auto brain,auto type,RawPathPoint) { CHECK(brain==token);trace.push_back("marker"+std::to_string(type));if (duringMarker) duringMarker(); };
        hooks.log=[this](auto brain,auto logKind,std::string_view text) { CHECK(brain==token && logKind==kind && text=="Bridge timeout\n");trace.push_back("timeout"); };
    }
    // 지정한 실제 본체 DWORD를 읽는다.
    std::uint32_t Field(std::size_t at) const { return Get(memory.Bytes(token),at); }
    // 주변 BYTE를 보존하며 실제 본체 DWORD를 쓴다.
    void Field(std::size_t at,std::uint32_t value) { Put(memory.Bytes(token),at,value); }
    // 두 float의 원문 비트를 기록한다.
    void Point(std::size_t at,RawPathPoint point) { Field(at,std::bit_cast<std::uint32_t>(point.x));Field(at+4,std::bit_cast<std::uint32_t>(point.y)); }
    // 다음 링크/패턴/현재 flags를 지정한 내용물 슬롯에 기록한다.
    void Content(std::uint16_t number,std::uint16_t next,std::uint32_t pattern,std::uint32_t flags) { auto raw=pool.AllocatedBytes(Sid{number});Put(raw,4,next,2);Put(raw,12,pattern,2);Put(raw,20,flags,2); }
    // 같은 저장소의 부모 첫 링크를 기록한다.
    void Head(std::uint16_t number) { Put(pool.AllocatedBytes(Sid{5}),6,number,2); }
    // 현재 전역 참조와 같은 풀을 사용하는 복원 모듈을 만든다.
    RawAiBridgePath Path() { return {memory,pool,finder,{now,limit,bridge,kind,startMarker,endMarker,targetMarker},hooks}; }
};
}
// 실제 원본 명령의600개 입력/정상 반환과 하위 진입 순서를 재생한다.
TEST_CASE(AiBridgePath_NativeStep600Inputs) {
    std::ifstream input(NETSTORM_AI_BRIDGE_STEP_FIXTURE);CHECK(input.good());std::string line;std::size_t rows{};
    // 각 행은 독립 본체/원본 입력이며 기대 상태를 C++에서 재계산하지 않는다.
    while (std::getline(input,line)) {
        if (line.empty() || line[0]=='#') continue;const auto columns=Split(line,'\t');CHECK(columns.size()==9);BridgePathScene scene;scene.Field(0x5f0,Number(columns[0]));scene.Field(0x604,Number(columns[1]));scene.Field(0x628,Number(columns[2]));scene.mode=Number(columns[3]);
        RawPathPoint source{10,20};const auto done=scene.Path().Step(scene.token,source,{30,40});CHECK(scene.Field(0x5f0)==Number(columns[4]) && done==(Number(columns[5])!=0));CHECK(std::bit_cast<std::uint32_t>(source.x)==Number(columns[6]) && std::bit_cast<std::uint32_t>(source.y)==Number(columns[7]));
        CHECK((scene.trace.empty() ? "-" : scene.trace[0])==columns[8]);++rows;
    }
    CHECK(rows==600);
}
// pending/다른TYPE/다른패턴을 건너뛰며 첫 사용 가능 조각만 삭제한다.
TEST_CASE(AiBridgePath_ConsumeFiltersAndFirstMatch) {
    BridgePathScene scene;scene.Head(6);scene.Content(6,7,3,0x140);scene.Content(7,8,4,0x40);scene.Content(8,9,3,0x40);scene.Content(9,0,3,0x40);scene.Path().Consume(scene.token,Sid{5},3);
    CHECK(scene.destroyed==std::vector<Sid>({Sid{8}}));const auto due=std::bit_cast<double>((static_cast<std::uint64_t>(scene.Field(0x634))<<32)|scene.Field(0x630));CHECK(due==scene.now+0.25);
}
// 패턴 인자는 DWORD 전체이므로65536을 WORD0으로 잘라 소비하지 않는다.
TEST_CASE(AiBridgePath_ConsumeWholeDwordAndNoMatchPreservesDeadline) {
    BridgePathScene scene;scene.Head(6);scene.Content(6,0,0,0x40);scene.Field(0x630,123);scene.Field(0x634,456);scene.Path().Consume(scene.token,Sid{5},65536);CHECK(scene.trace.empty() && scene.Field(0x630)==123 && scene.Field(0x634)==456);
}
// 삭제 효과 뒤 현재 rate와 double now를 사용한다.
TEST_CASE(AiBridgePath_ConsumeReadsCurrentDeadlineAfterDeletion) {
    BridgePathScene scene;scene.Head(6);scene.Content(6,0,3,0x40);scene.duringDestroy=[&] { scene.now=123.00000001;scene.Field(0x600,std::bit_cast<std::uint32_t>(0.125F)); };scene.Path().Consume(scene.token,Sid{5},3);
    CHECK(std::bit_cast<double>((static_cast<std::uint64_t>(scene.Field(0x634))<<32)|scene.Field(0x630))==123.12500001);
}
// 상태가 완료되지 않았으면 입력 시작점과 모든 배치/소비 효과를 보존한다.
TEST_CASE(AiBridgePath_ExtendWaitDoesNotDeploy) {
    BridgePathScene scene;scene.Field(0x5f0,4);scene.Field(0x604,8);scene.Field(0x628,7);const auto result=scene.Path().Extend(scene.token,{10,20},{30,40});CHECK(result.x==10 && result.y==20 && scene.trace.empty() && scene.Field(0x5f0)==4);
}
// 배치 뒤 현재 owner/패턴을 소비하고 다시 현재 owner로 재고를 갱신한다.
TEST_CASE(AiBridgePath_ExtendUsesCurrentFieldsAfterEffects) {
    BridgePathScene scene;scene.Field(0x5f0,5);scene.Head(6);scene.Content(6,0,7,0x40);scene.duringDeploy=[&] { scene.Field(0x260,2);scene.Field(0x6b4,7);scene.Point(0x6bc,{220,221}); };scene.duringDestroy=[&] { scene.Field(0x260,3); };const auto result=scene.Path().Extend(scene.token,{10,20},{30,40});
    CHECK(result.x==101 && result.y==202 && scene.trace==std::vector<std::string>({"deploy","technology2","destroy","technology3","refresh"}) && scene.Field(0x5f0)==0);
}
// 무효 결과/경계/NaN은 상태0 복귀만 유지하고 배치·기술 조회를 생략한다.
TEST_CASE(AiBridgePath_InvalidCompletionSkipsAllEffects) {
    // 각 좌표는 원본 strict0..상한 검사에서 독립적으로 거부된다.
    for (auto point:{RawPathPoint{0,20},RawPathPoint{256,20},RawPathPoint{10,0},RawPathPoint{10,256},RawPathPoint{-10000,-10000},RawPathPoint{std::numeric_limits<float>::quiet_NaN(),20}}) { BridgePathScene scene;scene.Field(0x5f0,5);scene.Point(0x6bc,point);static_cast<void>(scene.Path().Extend(scene.token,{10,20},{30,40}));CHECK(scene.trace.empty() && scene.Field(0x5f0)==0); }
}
// 디버그 표식은 배치/소비/갱신 뒤 현재 종류를 읽으며 둘째 TYPE도 다시 읽는다.
TEST_CASE(AiBridgePath_DebugMarkersFollowRefresh) {
    BridgePathScene scene;scene.Field(0x5f0,5);scene.duringRefresh=[&] { scene.kind=29; };scene.duringMarker=[&] { scene.targetMarker=99; };static_cast<void>(scene.Path().Extend(scene.token,{10,20},{30,40}));CHECK(scene.trace==std::vector<std::string>({"deploy","technology1","technology1","refresh","marker94","marker99"}));
}
// 시작점 없음은 성공 반환을 유지하고 timeout을 감소하지 않는다.
TEST_CASE(AiBridgePath_AdvanceMissingStartKeepsTimeout) {
    BridgePathScene scene;scene.Point(0x3d0,{-10000,-10000});scene.first={-10000,-10000};CHECK(scene.Path().Advance(scene.token,{30,40}) && scene.Field(0x3d8)==7 && scene.trace==std::vector<std::string>({"start"}));
}
// 유효한 첫 시작점은 저장한 뒤 표식을 놓고 기존 timeout을 유지한다.
TEST_CASE(AiBridgePath_AdvanceFirstPointAndMarker) {
    BridgePathScene scene;scene.kind=29;CHECK(scene.Path().Advance(scene.token,{30,40}) && scene.Field(0x3d0)==std::bit_cast<std::uint32_t>(20.0F) && scene.Field(0x3d8)==7 && scene.trace==std::vector<std::string>({"start","marker93"}));
}
// 아직 기다리는 유효 연결점도 성공으로 취급해 timeout1을 쓴다.
TEST_CASE(AiBridgePath_AdvanceWaitingPointResetsTimeout) {
    BridgePathScene scene;scene.Point(0x3d0,{10,20});scene.Field(0x5f0,4);scene.Field(0x604,8);scene.Field(0x628,7);CHECK(scene.Path().Advance(scene.token,{30,40}) && scene.Field(0x3d8)==1 && scene.trace.empty());
}
// 무효 결과에서만 signed 감소/실패/원래 출력 관문을 적용한다.
TEST_CASE(AiBridgePath_AdvanceTimeoutAndOutputGate) {
    // 양수2·1·0·부호경계의 감김과 signed 판정을 확인한다.
    for (auto timeout:{2U,1U,0U,0x80000000U}) { BridgePathScene scene;scene.Point(0x3d0,{10,20});scene.Field(0x5f0,5);scene.Point(0x6bc,{-10000,-10000});scene.Field(0x3d8,timeout);scene.Field(0xb00,1);const auto result=scene.Path().Advance(scene.token,{30,40});const bool success=timeout==2 || timeout==0x80000000U;CHECK(result==success && scene.Field(0x3d8)==(timeout==0x80000000U ? 0x7fffffffU : 1U) && scene.trace==(success ? std::vector<std::string>{} : std::vector<std::string>{"timeout"})); }
    BridgePathScene quiet;quiet.Point(0x3d0,{10,20});quiet.Field(0x5f0,5);quiet.Point(0x6bc,{-10000,-10000});quiet.Field(0x3d8,1);quiet.hooks.log={};CHECK(!quiet.Path().Advance(quiet.token,{30,40}) && quiet.Field(0x3d8)==1);
}
// 미연결 하위에서 앞선 상태0 복귀/배치 효과를 취소하지 않는다.
TEST_CASE(AiBridgePath_MissingBoundaryPreservesPriorEffects) {
    BridgePathScene scene;scene.Field(0x5f0,5);scene.hooks.technology={};bool rejected{};try { static_cast<void>(scene.Path().Extend(scene.token,{10,20},{30,40})); } catch (const std::logic_error&) { rejected=true; }CHECK(rejected && scene.Field(0x5f0)==0 && scene.trace==std::vector<std::string>({"deploy"}));
}
