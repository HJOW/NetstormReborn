// 고도 계산은 남은 시각/구간 길이의 원본 float 저장 뒤 x87 정수 절삭을 따른다.
#include "o/RawBalloonFrame.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <stdexcept>

namespace netstorm::o {
namespace {
constexpr float kCruiseHeight=44.0F,kMinimumDuration=0.0010000000474974513F; //원본 ROM50481c/509144다.
constexpr double kFtolLimit=9223372036854775808.0; //CRT signed64 불확정 반환의 경계다.
// 원본_ftol의 signed64 절삭 뒤 EAX 하위 DWORD를 보존한다.
std::int32_t Truncate(double value) {
    if (!std::isfinite(value) || value>=kFtolLimit || value< -kFtolLimit) return 0;
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::int64_t>(value)));
}
// 현재 부모의 비정렬 float 좌표를 직접 읽는다.
RawPathPoint Position(const SidPool& pool,Sid parent) {
    const auto raw=pool.Slot(parent);std::array<std::uint32_t,2> words{}; //X/Y의 정확한 원문 비트다.
    // 좌표 두 개의 네 BYTE를 각각 조립한다.
    for (std::size_t axis=0;axis<2;++axis) {
        // 각 DWORD의 낮은 BYTE부터 읽는다.
        for (std::size_t byte=0;byte<4;++byte) words[axis]|=std::uint32_t(raw[14+axis*4+byte])<<(byte*8);
    }
    return {std::bit_cast<float>(words[0]),std::bit_cast<float>(words[1])};
}
}
// 화염 효과 뒤 현재 좌표는 다시 읽지만 고도 비교의 BYTE는 원본 ebp처럼 보존한다.
void RunBalloonFrame(SidPool& pool,Sid parent,RawPathRuntime& runtime,const double& now,std::uint32_t balloonType,
    const BalloonFrameHooks& hooks,const std::shared_ptr<bool>& watch) {
    if (runtime.Finished() || hooks.halt()) return;
    const auto sample=hooks.step();if (!*watch) return;
    const auto current=runtime.Current();const auto remaining=static_cast<float>(runtime.PhysicalTime(current)-now); //원본 fstp DWORD의 저장 지점이다.
    const auto difference=runtime.PhysicalTime(current)-runtime.PhysicalTime(current-1);const auto duration=difference<kMinimumDuration ? kMinimumDuration : static_cast<float>(difference); //NaN 비교도 원문 분기를 따른다.
    const auto oldHeight=static_cast<std::int32_t>(pool.Slot(parent)[30]);std::int32_t height=-1; //ffffffff는 고도를 쓰지 않는 표식이다.
    if (current==1) {
        height=std::max(oldHeight,Truncate(static_cast<double>(kCruiseHeight)-static_cast<double>(remaining)*kCruiseHeight/duration));
        if (!height && pool.Slot(parent)[10]==balloonType) hooks.flame(Position(pool,parent).x,Position(pool,parent).y);
        if (!*watch) return;
    }
    if (current==runtime.Count()-1) height=std::min(oldHeight,Truncate(static_cast<double>(remaining)*kCruiseHeight/duration));
    if (sample.finished) { hooks.arrive();return; }
    const auto position=Position(pool,parent); //현재 좌표가 그대로면 고도 BYTE도 쓰지 않는다.
    if (sample.position.x!=position.x || sample.position.y!=position.y) {
        hooks.unpop(parent,0);if (!*watch) return;
        if (height!=-1) pool.AllocatedBytes(parent)[30]=static_cast<std::uint8_t>(height);
        hooks.pop(parent,sample.position,0x40);
    }
}
}
