// 순수 미니맵 기하를 실제 PE/CRT의 독립 관측과 대조한다. 외부 callback의 순서 효과는 별도 범위다.
#include "TestSupport.h"
#include "client/MinimapGeometry.h"
#include <array>
#include <bit>
#include <cstdint>
#include <fstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

using namespace netstorm::client;
namespace {
constexpr std::uint32_t kBitmapStride=128; // native blit의 source 주소를 같은 행 폭으로 비교한다.
constexpr std::array<std::string_view,5> kOperations{"rebuild","move","idle","attach","draw"}; // 독립 실행한 다섯 순수 기하 진입점이다.
constexpr std::array<std::size_t,5> kExpectedPureRows{254,36,216,272,250}; // callback 변경이 없는 실제 관측의 연산별 행 수다.

// 마지막 빈 필드까지 보존하며 TSV 및 원문 trace 인자를 나눈다.
std::vector<std::string> Split(std::string_view text,char separator) {
    std::vector<std::string> parts;std::size_t begin{};
    for (;;) { // 지정한 구분자 사이의 실제 원문을 순서대로 보관한다.
        const auto end=text.find(separator,begin);parts.emplace_back(text.substr(begin,end==text.npos ? text.npos : end-begin));
        if (end==text.npos) return parts;begin=end+1;
    }
}
// signed 입력과 전체 DWORD 비트 입력을 같은 원시 폭으로 읽는다.
std::uint32_t Number(std::string_view text) { return static_cast<std::uint32_t>(std::stoll(std::string(text))); }
// 실제 DWORD의 비트를 유지하며 signed32를 복원한다.
std::int32_t Signed(std::uint32_t value) { return std::bit_cast<std::int32_t>(value); }
// float32 입력 및 목표를 근삿값 비교 없이 같은 비트로 읽는다.
float Float(std::string_view text) { return std::bit_cast<float>(Number(text)); }
// 제품이 실행 후 보유한 전체 기하 필드를 독립 stateout과 같은 순서로 출력한다.
std::string StateOutput(const MinimapGeometryState& state) {
    return std::to_string(std::bit_cast<std::uint32_t>(state.width))+':'+std::to_string(std::bit_cast<std::uint32_t>(state.height))+':'+
        std::to_string(static_cast<std::uint32_t>(state.cropX))+':'+std::to_string(static_cast<std::uint32_t>(state.cropY))+':'+std::to_string(state.scrolling);
}
// 순수 호출 뒤에도 입력의 카메라/viewport/배율 전체 원시 값이 보존되는지 비교한다.
std::string ViewOutput(const MinimapGeometryView& view) {
    return std::to_string(view.camera[0])+':'+std::to_string(view.camera[1])+':'+std::to_string(static_cast<std::uint32_t>(view.viewport.left))+':'+
        std::to_string(static_cast<std::uint32_t>(view.viewport.top))+':'+std::to_string(static_cast<std::uint32_t>(view.viewport.right))+':'+
        std::to_string(static_cast<std::uint32_t>(view.viewport.bottom))+':'+std::to_string(static_cast<std::uint32_t>(view.pixelsX))+':'+std::to_string(static_cast<std::uint32_t>(view.pixelsY));
}
// 실제 원문이 해당 경계에 전달한 인자 한 묶음만 찾는다. 기대값으로 제품 입력을 만들지 않는다.
std::vector<std::string> NativeEvent(const std::vector<std::string>& row,std::string_view name) {
    std::vector<std::string> result;std::size_t count{};const auto prefix=std::string(name)+':';
    for (const auto& event:Split(row[23],';')) { // 원문 trace에서 같은 이름의 실제 외부 호출만 선택한다.
        if (!event.starts_with(prefix)) continue;result=Split(event,':');++count;
    }
    CHECK(count==1);if (count!=1) throw std::invalid_argument("미니맵 원문 경계의 관측 수");return result;
}
// 실제497f20 경계가 받은 두 float32 비트를 순수 API의 목표와 비교한다.
void CheckCenter(const std::vector<std::string>& row,const std::array<float,2>& target) {
    const auto event=NativeEvent(row,"center");CHECK(event.size()==3);if (event.size()!=3) return;
    CHECK(std::bit_cast<std::uint32_t>(target[0])==Number(event[1]));CHECK(std::bit_cast<std::uint32_t>(target[1])==Number(event[2]));
}
// 원문 source 주소/대상/크기와 exclusive camera frame을 제품의 전체 Draw 계획과 비교한다.
void CheckDraw(const std::vector<std::string>& row,const MinimapDrawPlan& plan) {
    const auto blit=NativeEvent(row,"blit"),frame=NativeEvent(row,"frame");CHECK(blit.size()==9 && frame.size()==6);if (blit.size()!=9 || frame.size()!=6) return;
    CHECK(plan.destination.x==Signed(Number(blit[1])));CHECK(plan.destination.y==Signed(Number(blit[2])));
    const auto offset=Signed(static_cast<std::uint32_t>(plan.source.y)*kBitmapStride+static_cast<std::uint32_t>(plan.source.x)); // 원문의 DWORD 주소 산술로 2차원 source를 같은 바이트 오프셋으로 표현한다.
    CHECK(offset==Signed(Number(blit[3])));CHECK(Number(blit[4])==kBitmapStride && blit[5]=="0" && blit[6]=="0");
    CHECK(plan.source.x==Signed(Number(row[4])));CHECK(plan.source.y==Signed(Number(row[5])));CHECK(plan.size.x==Signed(Number(blit[7])));CHECK(plan.size.y==Signed(Number(blit[8])));
    CHECK(plan.camera.left==Signed(Number(frame[1])));CHECK(plan.camera.top==Signed(Number(frame[2])));CHECK(plan.camera.right==Signed(Number(frame[3])));CHECK(plan.camera.bottom==Signed(Number(frame[4])));
    // frame[5]의155는 oracle이 공급한 외부 팔레트 장치값이다. 순수 계획 API는 색·clip·실제 픽셀을 생성하지 않는다.
    // right/bottom은 실제004a24b0의 인자를 비교한다. 그 외부 함수가 네 선을 그릴 때 수행하는 -1은 계획에서 미리 적용하지 않는다.
}
}

// callback 변경 없는1028관측을 전부 비교하고 제외한26관측을 별도로 계수한다.
TEST_CASE(MinimapGeometry_IndependentNativePureOperations) {
    std::ifstream input(NETSTORM_MINIMAP_GEOMETRY_FIXTURE);CHECK(input.good());std::string line;std::size_t compared{},excluded{},total{};std::array<std::size_t,5> counts{};
    while (std::getline(input,line)) { // 앞20열만 제품 state/view/local/destination 입력에 사용한다.
        if (line.empty() || line.starts_with('#')) continue;const auto row=Split(line,'\t');CHECK(row.size()==27);if (row.size()!=27) continue;++total;
        if (row[19]!="-") { // 이26행의 callback 재조회·부분 중단은 순수 API가 소유하지 않는 외부 호출 순서다. 해당 효과를 흉내 내어 통과시키지 않는다.
            ++excluded;continue;
        }
        MinimapGeometryState state{Float(row[2]),Float(row[3]),Signed(Number(row[4])),Signed(Number(row[5])),Number(row[6])};
        MinimapGeometryView view{{Number(row[7]),Number(row[8])},{Signed(Number(row[9])),Signed(Number(row[10])),Signed(Number(row[11])),Signed(Number(row[12]))},Signed(Number(row[13])),Signed(Number(row[14]))};
        const auto localX=Float(row[15]),localY=Float(row[16]);const ScreenPoint destination{Signed(Number(row[17])),Signed(Number(row[18]))};const auto failures=netstorm::test::FailureCount();
        std::size_t operation{};
        while (operation<kOperations.size() && row[1]!=kOperations[operation]) ++operation; // 문자열 입력을 다섯 실제 진입점 중 하나로만 분배한다.
        CHECK(operation<kOperations.size());if (operation>=kOperations.size()) continue;++counts[operation];
        if (row[1]=="rebuild") {
            const bool rebuilt=RebuildMinimap(state,view);CHECK(rebuilt==(row[23]=="dirty:0")); // 원문의Scrolling 억제는 Dirty 호출의 유무로 독립 확인한다.
        } else if (row[1]=="move") CheckCenter(row,MinimapTarget(state,localX,localY));
        else if (row[1]=="idle") {
            CheckCenter(row,DragMinimap(state,localX,localY)); // polling/local 변환과 scrolling 진단은 외부 장치가 정상 복귀한 뒤의 순수 기하만 비교한다.
        } else if (row[1]=="attach") ClampMinimapAttachment(state);
        else CheckDraw(row,PlanMinimapDraw(state,view,destination));
        CHECK(StateOutput(state)==row[21]);CHECK(ViewOutput(view)==row[22]);CHECK(row[20]=="1" && row[24]=="0");CHECK(std::stoull(row[25])>0 && std::stoull(row[25])>=std::stoull(row[26]));++compared;
        if (netstorm::test::FailureCount()!=failures) std::fprintf(stderr,"미니맵 순수 기하 입력: %s\n",row[0].c_str());
    }
    CHECK(total==1054);CHECK(compared==1028);CHECK(excluded==26);CHECK(counts==kExpectedPureRows);
}
