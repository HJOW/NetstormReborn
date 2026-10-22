// 세일 스케이터/크리스탈 크랩의 실제 Walker38 프레임을 같은 부모에 적용한다.
#pragma once
#include "o/RawPathRuntime.h"
#include <memory>

namespace netstorm::o {
// Step/지면/도착/공간의 기존 실제 몸체를 호출하는 경계다.
struct WalkerFrameHooks {
    std::function<bool()> immobile,halt; // 4287a0 및 현재 부모+38의 원본 정지 검사다.
    std::function<RawPathSample()> step; // 48a330의 같은 경로/각도 갱신이다.
    std::function<bool(RawPathPoint)> groundStop; // 48a7c0은 자신을 삭제할 수 있다.
    std::function<void()> arrive; // 48b340의 공통 도착을 호출한다.
    std::function<void(Sid,std::uint32_t)> unpop; // 부모+48의 현재 공간 해제다.
    std::function<void(Sid,RawPathPoint,std::uint32_t)> pop; // 부모+90의 현재 좌표/flags다.
};
// 48b080: 윈드의 공유 counter와 레인의 예약 시각은 서로 다른 애니메이션 입력이다.
void RunWalkerFrame(SidPool& pool,Sid parent,RawPathRuntime& runtime,double now,
    std::span<const RiftTypeFrames> frames,std::uint32_t windType,std::uint32_t rainType,
    std::uint32_t& windCounter,const WalkerFrameHooks& hooks,const std::shared_ptr<bool>& watch);
}
