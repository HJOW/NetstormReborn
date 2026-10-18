// 윈드·레인·썬더의 실제 기본 프레임과 서로 다른 전용 그림 순서를 구별한다.
#pragma once
#include "o/RiftType.h"
#include <functional>

namespace netstorm::o {
bool IsPatchTemple(std::span<const std::uint8_t> raw); // 정확한10.78 세 템플 가상표만 확인한다.
std::int32_t PatchTempleMainFrame(std::span<const std::uint8_t> raw,std::span<const std::int32_t> baseFrames); // 실제4acfe0의 현재TYPE+128이다. -1도 그대로 반환한다.
void DrawWindThunderTemple(std::span<const std::uint8_t> raw,const RiftTypeFrames& frames,std::int32_t baseFrame,
    const std::function<void(std::int32_t)>& frame); // 실제4dcf10/4dd0f0 추가Draw다. 본체는 일반 SpriteDraw가 먼저 그린다.
}
