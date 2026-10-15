// 레인 신전의 기본 그림·현재 물결·상태별 추가 그림을 원본 호출 순서로 제출한다.
#pragma once
#include "o/RiftType.h"
#include <functional>

namespace netstorm::o {
// 픽셀 출력과 실제 templeAscend.wav 재생을 받는 클라이언트 경계다.
struct RainVortexDrawHooks {
    std::function<void(std::int32_t)> frame; // 원본 00497280/CD00455ee0의 그림 한 장 제출이다.
    std::function<void()> ascendSound; // 원본의 조건부 신전 상승 효과음 호출이다.
};
// 실제 레인 신전 가상표인지 확인한다. extra의 abstract/특수 Draw 선택은 호출자가 처리한다.
bool IsRainVortex(OriginalEdition edition,std::span<const std::uint8_t> raw);
// PATCH004dcd00/CD0044fe00. 콜백 뒤 현재 raw를 다시 읽어 원본의 조회 시점을 보존한다.
void DrawRainVortex(OriginalEdition edition,std::span<const std::uint8_t> raw,
    const RiftTypeFrames& frames,const RainVortexDrawHooks& hooks);
}
