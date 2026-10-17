// 벌룬 경로42의 실제 고도 갱신과 화염·도착·공간 호출 순서를 복원한다.
#pragma once
#include "o/RawPathRuntime.h"
#include <memory>

namespace netstorm::o {
// 계산은 본문에서 수행하며 기존 실제 Step/Halt/도착·장치만 빌린다.
struct BalloonFrameHooks {
    std::function<bool()> halt; //489ff0의 실제 중단 판정이다.
    std::function<RawPathSample()> step; //48a330의 현재 좌표/단계/시각 갱신이다.
    std::function<void()> arrive; //완료 뒤 현재 가상+24의48b340이다.
    std::function<void(Sid,std::uint32_t)> unpop; //현재 부모 가상+48이다.
    std::function<void(Sid,RawPathPoint,std::uint32_t)> pop; //고도 BYTE를 쓴 뒤 현재 부모 가상+90이다.
    std::function<void(float,float)> flame; //현재 좌표 balloonFlame.wav/loop0이다.
};
// 48ad60 전체다. watch는 외부 효과의 자기 삭제 뒤 호스트에서 역참조하지 않도록 한다.
void RunBalloonFrame(SidPool& pool,Sid parent,RawPathRuntime& runtime,const double& now,std::uint32_t balloonType,
    const BalloonFrameHooks& hooks,const std::shared_ptr<bool>& watch);
}
