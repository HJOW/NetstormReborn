// 원본 양방향 탐색/편향 절삭/현재 경계·표식 효과를 검사한다. 작성·컴파일만 한다.
#include "o/RawAiDryGeometry.h"
#include "RawSceneSupport.h"
#include <limits>
using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 같은 실제 풀과 명시 지도/표식 효과 입력을 준비한다.
struct DryGeometryScene {
    SidPool pool{OriginalEdition::Patch1078,32768,false};AiBuildSearchBounds bounds{10,10,246,246};std::uint32_t debug{99},marker{104}; //원본 풀과 현재 전역이다.
    std::vector<std::uint16_t> surface{std::vector<std::uint16_t>(65536)};std::vector<std::array<std::int32_t,2>> cells; //실제 WORD 지도와 방문 순서다.
    AiDryGeometryHooks hooks;std::vector<RawPathPoint> points;std::vector<Sid> created,destroyed;std::vector<std::string> trace; //선택된 원문 수명 효과를 관찰한다.
    std::function<void(std::size_t)> duringPop;std::function<void(Sid)> duringDestroy; //각 실제 효과 뒤 명시 현재 상태 변경이다.
    // 외부 효과의 결과를 주입하지 않고 각 실제 슬롯/현재 지도를 읽고 쓴다.
    DryGeometryScene() {
        hooks.surface=[this](auto x,auto y) { cells.push_back({x,y});return surface.at(static_cast<std::size_t>(y*256+x)); };
        hooks.create=[this](auto type,auto flags) { CHECK(flags==2);const auto sid=pool.Allocate(flags);auto raw=pool.AllocatedBytes(sid);std::fill(raw.begin(),raw.end(),std::uint8_t{});raw[10]=static_cast<std::uint8_t>(type);created.push_back(sid);trace.push_back("create");return sid; };
        hooks.word=[this](Sid sid,auto word) { CHECK(word==93);Put(pool.AllocatedBytes(sid),12,word,2);trace.push_back("word"); };
        hooks.pop=[this](Sid sid,RawPathPoint point,auto flags) { CHECK(!flags && Get(pool.Slot(sid),12,2)==93);Put(pool.AllocatedBytes(sid),14,std::bit_cast<std::uint32_t>(point.x));Put(pool.AllocatedBytes(sid),18,std::bit_cast<std::uint32_t>(point.y));points.push_back(point);trace.push_back("pop");if (duringPop) duringPop(points.size()); };
        hooks.destroy=[this](Sid sid,auto flags) { CHECK(!flags);destroyed.push_back(sid);trace.push_back("destroy");if (duringDestroy) duringDestroy(sid); };
    }
    // 같은 현재 풀/전역/선택 효과를 빌린다.
    RawAiDryGeometry Make() { return RawAiDryGeometry(pool,{bounds,debug,marker},hooks); }
    // 현재 풀에 TYPE/WORD가 다른 실제 표식 후보를 준비한다.
    Sid Object(std::uint32_t type,std::uint16_t word) { const auto sid=pool.Allocate(2);auto raw=pool.AllocatedBytes(sid);std::fill(raw.begin(),raw.end(),std::uint8_t{});raw[10]=static_cast<std::uint8_t>(type);Put(raw,12,word,2);return sid; }
};
}
// 원본 두 float의 차이/제곱합은 중간 float 저장 없이 계산한다.
TEST_CASE(AiDryGeometry_DistanceFloatOperands) {
    CHECK(AiPointDistance({0,0},{3,4})==5 && AiPointDistance({-1,-1},{-1,4})==5);const auto huge=std::numeric_limits<float>::max();CHECK(std::isfinite(AiPointDistance({huge,0},{-huge,0})));CHECK(std::isnan(AiPointDistance({std::numeric_limits<float>::quiet_NaN(),0},{0,0})));
}
// 동률은 왼쪽 우선이며 두 방향을 각 반복에서 모두 방문한다.
TEST_CASE(AiDryGeometry_HorizontalVerticalTie) {
    DryGeometryScene input;const auto point=input.Make().Candidate({30,40},{34,40});CHECK(point.x==32 && point.y==44 && input.cells.size()==8 && input.created.empty() && input.destroyed.empty());CHECK((input.cells[0]==std::array<std::int32_t,2>({32,41}) && input.cells[1]==std::array<std::int32_t,2>({32,39}) && input.cells[6]==std::array<std::int32_t,2>({32,44}) && input.cells[7]==std::array<std::int32_t,2>({32,36})));DryGeometryScene vertical;const auto other=vertical.Make().Candidate({40,40},{40,44});CHECK(other.x==36 && other.y==42);
}
// ceil이나 사제용0.99999가 아닌 float0.9999 편향 합 뒤 ftol을 사용한다.
TEST_CASE(AiDryGeometry_RoundsCellAfterBias) {
    DryGeometryScene input;input.Make().Candidate({30.00005F,40.00005F},{34.00005F,40.00005F});CHECK((input.cells[0]==std::array<std::int32_t,2>({32,41})));DryGeometryScene next;next.Make().Candidate({30.0002F,40.0002F},{34.0002F,40.0002F});CHECK((next.cells[0]==std::array<std::int32_t,2>({33,42})));
}
// 점유 칸은 각 방향 연속 횟수를0으로 만들며 오른쪽만 먼저4에 도달할 수 있다.
TEST_CASE(AiDryGeometry_ResetsBothRunsAndChoosesRight) {
    DryGeometryScene input;input.surface[42*256+32]=5;const auto point=input.Make().Candidate({30,40},{34,40});CHECK(point.x==32 && point.y==36 && input.cells.size()==8);DryGeometryScene both;both.surface[42*256+32]=5;both.surface[39*256+32]=6;const auto other=both.Make().Candidate({30,40},{34,40});CHECK(other.x==32 && other.y==35 && both.cells.size()==10);
}
// 현재 점이 경계 끝값에 있으면 이동하고 이동 뒤 경계 밖 한 칸도 원본처럼 조회한다.
TEST_CASE(AiDryGeometry_IncludesBoundsBeforeMoving) {
    DryGeometryScene input;const auto point=input.Make().Candidate({30,10},{34,10});CHECK((point.x==32 && point.y==14 && input.cells.size()==5 && input.cells[1]==std::array<std::int32_t,2>({32,9})));
}
// 물리256범위 밖은 빈칸으로 계산하고 현재 지도 효과를 요청하지 않는다.
TEST_CASE(AiDryGeometry_OutOfGridAvoidsSurface) {
    DryGeometryScene input;input.bounds={-500,-500,500,500};input.hooks.surface={};const auto point=input.Make().Candidate({256,40},{260,40});CHECK(point.x==258 && point.y==44 && input.cells.empty());
}
// 디버그 표식은 초기 두 점→선분 네 점→수직 양쪽 여덟 점 순서다.현재 종류도 재조회한다.
TEST_CASE(AiDryGeometry_DebugTraceOrderAndCurrentKind) {
    DryGeometryScene input;const auto old=input.Object(104,93);input.debug=29;input.Make().Candidate({30,40},{34,40});CHECK(input.destroyed==std::vector<Sid>({old}) && input.points.size()==14 && input.created.size()==14 && input.trace.front()=="destroy");CHECK(input.points[0].x==30 && input.points[1].x==34 && input.points[2].x==31 && input.points[5].x==34 && input.points[6].y==41 && input.points[7].y==39 && input.points[12].y==44 && input.points[13].y==36);
    DryGeometryScene changed;changed.debug=29;changed.duringPop=[&](auto count) { if (count==1) { changed.debug=99;changed.marker=105; } };changed.Make().Candidate({30,40},{34,40});CHECK(changed.points.size()==2 && changed.pool.Slot(changed.created[0])[10]==104 && changed.pool.Slot(changed.created[1])[10]==105);
}
// 원본 선택 위치에서 누락 효과를 거부하고 생성/kind 쓰기 등 이미 수행한 상태를 보존한다.
TEST_CASE(AiDryGeometry_MarkPartialWrites) {
    DryGeometryScene input;input.hooks.word={};CHECK(Throws([&] { input.Make().Mark({1,2}); }) && input.created.size()==1 && Get(input.pool.Slot(input.created[0]),12,2)==0 && input.points.empty());DryGeometryScene after;after.hooks.pop={};CHECK(Throws([&] { after.Make().Mark({1,2}); }) && after.created.size()==1 && Get(after.pool.Slot(after.created[0]),12,2)==93 && after.points.empty());
}
// TYPE BYTE와 WORD93이 모두 일치하는 것만 지우며 dead는 방문하고 free는 건너뛴다.
TEST_CASE(AiDryGeometry_ClearMatchesWordTypeAndDead) {
    DryGeometryScene input;const auto first=input.Object(104,93);input.Object(104,92);input.Object(105,93);const auto dead=input.Object(104,93);input.pool.AllocatedBytes(dead)[11]|=2;const auto free=input.Object(104,93);input.pool.AllocatedBytes(free)[11]|=1;input.Make().ClearMarkers();CHECK(input.destroyed==std::vector<Sid>({first,dead}));
}
// 삭제 전에 다음 SID를 저장하므로 삭제 효과에서 다음이 free가 되어도 저장 SID를 방문한다.
TEST_CASE(AiDryGeometry_ClearCachesNextBeforeDelete) {
    DryGeometryScene input;const auto first=input.Object(104,93),second=input.Object(104,93);input.duringDestroy=[&](Sid sid) { if (sid==first) input.pool.AllocatedBytes(second)[11]|=1; };input.Make().ClearMarkers();CHECK(input.destroyed==std::vector<Sid>({first,second}));
}
// 각 삭제 뒤 현재 전체DWORD TYPE를 읽고 상위 비트가 있으면 BYTE를 잘라 일치시키지 않는다.
TEST_CASE(AiDryGeometry_ClearUsesCurrentMarkerType) {
    DryGeometryScene input;const auto first=input.Object(104,93),second=input.Object(105,93);input.Object(104,93);input.duringDestroy=[&](Sid sid) { if (sid==first) input.marker=105; };input.Make().ClearMarkers();CHECK(input.destroyed==std::vector<Sid>({first,second}));DryGeometryScene upper;upper.Object(104,93);upper.marker=0x168;upper.Make().ClearMarkers();CHECK(upper.destroyed.empty());
}
// 처음 도달하는 지도 경계만 요구하며 미연결이면 후보를 위조하지 않고 거부한다.
TEST_CASE(AiDryGeometry_MissingSelectedSurface) {
    DryGeometryScene input;input.hooks.surface={};CHECK(Throws([&] { input.Make().Candidate({30,40},{34,40}); }) && input.created.empty());
}
