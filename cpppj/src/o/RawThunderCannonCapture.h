// 썬더 캐논131의 방향별 기본 프레임과 현재 프레임 추가 그림을 실제 캡처에 제공한다.
#pragma once
#include "o/RiftType.h"
#include <functional>
#include <span>

namespace netstorm::o {
// 보호10.78의 TYPE131/VT50bad0 쌍만 받아 다른 파생 캐논을 추정하지 않는다.
bool IsThunderCannonCapture(std::span<const std::uint8_t> raw);
// 0046ae10은 현재 프레임 글자에 해당하는 TYPE+130 방향 구간의 첫 번호를 반환한다.
std::int32_t ThunderCannonCaptureMainFrame(std::span<const std::uint8_t> raw,const RiftTypeFrames& frames);
// 0046ae30은 현재 타입/프레임을 한 번 제출하며 extra·출력WORD·base 일치로 생략하지 않는다.
void DrawThunderCannonCapture(std::span<const std::uint8_t> raw,
    const std::function<void(std::uint32_t,std::uint32_t,std::int32_t,std::int32_t)>& draw,std::int32_t x,std::int32_t y);
}
