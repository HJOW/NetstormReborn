// 원본 Factory의 기본 프레임 가상+78과 추가 그림 가상+a4를 복원한다.
#pragma once
#include "o/SidPool.h"
#include "o/RiftType.h"
#include <functional>

namespace netstorm::o {
// 현재 슬롯/TYPE 프레임 표를 읽으며 그림 출력은 같은 대상의 실제0044c180 소비자에 전달한다.
class RawFactoryDraw {
public:
    // 현재 복원은10.78의 Factory/아웃포스트 두 가상 표다. 다른 판본/파생을 추정하지 않는다.
    RawFactoryDraw(const SidPool& pool,std::span<const RiftTypeFrames> frames);
    bool Handles(Sid sid) const; // 실제+78/+a4/+80이 같은 두 원본 가상 표인지 확인한다.
    std::int32_t MainFrame(Sid sid) const; // 00454cd0: 현재 Level의 TYPE+A..H 첫 번호다. 없는 글자는-1이다.
    // 00454cf0: extra&9/dead이면 생략하고 현재 프레임과 기본 프레임이 다를 때만 추가 그림을 낸다.
    void Additional(Sid sid,std::int32_t x,std::int32_t y,
        const std::function<void(std::int32_t,std::int32_t,std::uint32_t,std::int32_t)>& draw) const;
private:
    std::span<const std::uint8_t> Raw(Sid sid) const; // 실제 가상 표/프레임 표 범위를 사용 시 검사한다.
    const SidPool& pool_; // 같은 실제50바이트 슬롯이다.
    std::span<const RiftTypeFrames> frames_; // 같은 현재 TYPE의 A..P 첫 프레임 표다.
};
}
