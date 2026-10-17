// 실제 Boardcoord 접근점의 모든 Graph 조회/사각형 변경/반환을 원본 fixture와 대조한다.
#include "RawSceneSupport.h"
#include "o/RawBoardApproach.h"
#include <cmath>
using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 현재 float 입력을 손실 없이 해석한다.
float Float(const std::string& v) { return std::bit_cast<float>(Number(v)); }
// 원본 외부 효과와 같은 구분자 순서를 유지한다.
void Event(std::string& events,std::string value) { if (!events.empty()) events+=';';events+=value; }
}
// 현재 Graph 응답은 입력 스크립트이며 좌표 선택/순회/재귀 자체는 복원 모듈이 실행한다.
TEST_CASE(BoardApproach_Native696) {
    std::size_t count=0;
    // 두 정밀도에서 동일했던696개 전체 입력을 순서대로 대조한다.
    for (const auto& row:LoadFixture(NETSTORM_BOARD_APPROACH_FIXTURE).rows) {
        CHECK(row.size()==4);const auto input=Split(row[0],',');CHECK(input.size()==9);std::array<float,4> rectangle{Float(input[0]),Float(input[1]),Float(input[2]),Float(input[3])};const auto original=rectangle;
        const float x=Float(input[4]),y=Float(input[5]);const auto mode=Number(input[7]);std::size_t calls=0;std::string events;
        RawBoardApproach approach([&](float px,float py) {
            const auto xb=std::bit_cast<std::uint32_t>(px),yb=std::bit_cast<std::uint32_t>(py);std::uint32_t value;
            if (mode==0) value=1;
            else if (mode==1) value=xb==Number(input[4]) && yb==Number(input[5]) ? 1U : 2U;
            else if (mode==2) value=(static_cast<int>(px)+static_cast<int>(py))%3==0 ? 1U : 2U;
            else if (mode==3) value=!calls || calls%5==0 ? 1U : 2U;
            else if (mode==4) value=254;
            else value=(xb==Number(input[4]) && yb==Number(input[5])) || (static_cast<int>(px)==static_cast<int>(original[0]) && static_cast<int>(py)==static_cast<int>(original[3])) ? 1U : 2U;
            Event(events,"G:"+std::to_string(xb)+':'+std::to_string(yb)+':'+std::to_string(value));++calls;return value;
        },[&] { Event(events,"A:565"); });
        const auto result=approach.Find(rectangle,x,y,Number(input[6]),Number(input[8])!=0);const auto expected=Split(row[1],',');
        // 사각형의 복구 또는 flag4 확장 유지도 원본의 전체 DWORD와 같아야 한다.
        for (std::size_t i=0;i<4;++i) CHECK(std::bit_cast<std::uint32_t>(rectangle[i])==Number(expected[i]));
        CHECK(std::to_string(std::bit_cast<std::uint32_t>(result.x))+','+std::to_string(std::bit_cast<std::uint32_t>(result.y))==row[2]);CHECK(events==row[3]);++count;
    }
    CHECK(count==696);
}
