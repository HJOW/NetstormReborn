// 원본 Boardcoord의 사각형 가장자리와 같은 Graph 접근점 탐색이다.
#pragma once
#include "o/RawConstructionNotice.h"
#include <functional>
namespace netstorm::o {
// 10.78의41db70/41d400을 복원한다. Graph 번호254도 원본처럼 일반 동등 비교에 포함한다.
class RawBoardApproach {
public:
    RawBoardApproach(std::function<std::uint32_t(float,float)> graph,std::function<void()> assertion={}); // 현재 GraphAt와 선택된 debug assertion 경계를 연결한다.
    ConstructionNoticePoint Find(std::array<float,4>& rectangle,float x,float y,std::uint32_t flags,bool checking=false) const; // 사각형 변경/복구를 포함한 원본 결과다.
private:
    std::function<std::uint32_t(float,float)> graph_; // 실제462b80의 현재 GraphAt 번호다.
    std::function<void()> assertion_; // checking에서 가장자리 순회가 경계에 없을 때만 요청한다.
};
}
