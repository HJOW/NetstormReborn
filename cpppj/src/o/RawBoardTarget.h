// 운반 명령의 일반 접근점41d530과 사각형 변 중간점41d400을 복원한다.
#pragma once
#include "o/RawPathRuntime.h"
#include "o/RawSquidNeighbors.h"
namespace netstorm::o {
// 월드 초기화4af170이 기록한 실패점이며 진단 뒤 현재 값을 다시 읽는다.
struct BoardTargetState {
    RawPathPoint invalid{-10000.0F,-10000.0F}; // 545370/545374의 실제 초기화 후 두 float다.
};
// 건설 사제의41db70 Graph 순회와 구별되는 원본 기하 계산이다.
class RawBoardTarget {
public:
    RawBoardTarget(BoardTargetState& state,std::function<void(std::uint32_t)> assertion={}); // 진단은 해당 분기에 도달할 때만 필요하다.
    RawPathPoint Nearest(SquidFootprint rectangle,RawPathPoint source,std::uint32_t flags) const; //41d530: 저장한 중심/방향과flags2의 북쪽 특례다.
    RawPathPoint Side(SquidFootprint rectangle,std::uint32_t side,std::uint32_t flags) const; //41d400:0/2/4/6 변의 중간점과flags4 확장이다.
private:
    BoardTargetState& state_; // 실패 시 진단 callback 뒤 현재 원문 두 DWORD를 사용한다.
    std::function<void(std::uint32_t)> assertion_; // Boardcoord.cpp의1f9/262 진단을 공급한다.
};
}
