//00457460의 현재 동맹 이웃/사거리/재생성 호출 순서를 처리한다.
#pragma once
#include "o/SidPool.h"
#include <array>
#include <functional>

namespace netstorm::o {
//각 방향에서 현재 값을 읽는 실제 소비자이며 누락된 선택 하위는 거부한다.
struct FenceRebuildHooks {
    std::function<Sid(Sid,std::uint32_t)> neighbor; //00455fb0의 동맹 최근접 검색이다.
    std::function<bool(Sid,std::array<float,4>)> withinRange; //00455f00의 실제 현재 사거리 판정이다.
    std::function<void(Sid,Sid)> markers; //00456460의 내부 표식 생성이다.
    std::function<void(std::array<float,4>,std::uint32_t,std::uint32_t,std::int32_t,std::int32_t)> shields; //00455b20(mode1)의 현재 좌표/owner/끝점SID 생성이다.
};
//원본의 부모extra9/대상extra21을 원래 위치에서 검사한다.
class RawFenceRebuild {
public:
    RawFenceRebuild(const SidPool& pool,FenceRebuildHooks hooks); //PATCH 현재 풀과 하위 소비자를 빌린다.
    void Rebuild(Sid parent) const; //동맹 이웃→거리→대상extra→표식→현재 포스필드 인수의 순서다.
private:
    std::array<float,4> Endpoints(Sid parent,Sid other) const; //호출 직전의 현재 두 좌표를 읽는다.
    const SidPool& pool_; //현재 TYPE/extra/owner/좌표를 소유한 실제 풀이다.
    FenceRebuildHooks hooks_; //네 방향을 처리하는 실제 소비자다.
};
}
