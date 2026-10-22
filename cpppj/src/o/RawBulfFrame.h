// 원본 Bulf41의 회전 구간과 보행 프레임·발소리 갱신을 같은 raw 부모에 적용한다.
#pragma once
#include "o/RawPathRuntime.h"
#include "o/SquidDisplay.h"
#include <memory>

namespace netstorm::o {
// 이동 계산은 기존 Step이 수행하며 이 처리기는 원본41 고유 프레임과 효과 순서만 담당한다.
struct BulfFrameHooks {
    std::function<bool()> immobile; // 첫004287a0의 현재 부모Fall 사건 조회다.
    std::function<bool()> halt; // 현재 경로의 기존00489ff0이다.
    std::function<RawPathSample()> step; // 실제0048a330을 호출하고 같은 runtime 각도/단계를 갱신한다.
    std::function<void()> arrive; // 실제 가상+24의0048b340이다.
    std::function<void(Sid,std::uint32_t)> unpop; // 부모의 현재 가상+48이다.
    std::function<void(Sid,RawPathPoint,std::uint32_t)> pop; // 변경한 프레임과 Step 좌표의 가상+90이다.
    std::function<std::int32_t(std::int32_t,std::int32_t)> randomInt; // 공용004558f0의 반열린 정수 범위다.
    std::function<void(float,float,std::string_view)> sound; // 현재 부모 좌표의004a9d70/loop0이다.
};
// 0048ba30: E/F 구간을 선택하고 angle*(1/360)*(count-1)의 float 저장 후 +0.5 절삭을 적용한다.
std::int32_t BulfTurningFrame(const RiftTypeFrames& frames,float angle,std::uint32_t counterClockwise);
// 0048b760 전체 분기다. 표/shape/현재 슬롯은 호출 동안 살아 있어야 하며 watch는 자기 삭제를 감시한다.
void RunBulfFrame(SidPool& pool,Sid parent,RawPathRuntime& runtime,
    std::span<const RiftTypeFrames> frames,std::span<const SquidDisplayShape> shapes,
    const BulfFrameHooks& hooks,const std::shared_ptr<bool>& watch);
}
