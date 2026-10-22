// 간헐천122의 고정 본체와 현재 애니메이션을 실제 캡처/표시에 공유한다.
#pragma once
#include <cstdint>
#include <functional>
#include <span>

namespace netstorm::o {
// 보호10.78의 TYPE122/VT50da88 쌍만 받아 다른 자원 객체를 추정하지 않는다.
bool IsGeyserCapture(std::span<const std::uint8_t> raw);
// 00485480은 현재 TYPE/프레임을497280에 한 번 전달한다. 추상색 관문은 실제 출력 소비자가 처리한다.
void DrawGeyserCapture(std::span<const std::uint8_t> raw,
    const std::function<void(std::uint32_t,std::uint32_t,std::int32_t,std::int32_t)>& draw,std::int32_t x,std::int32_t y);
}
