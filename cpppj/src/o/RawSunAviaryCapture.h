// 훨리베이스72의 생산 상태별 기본/추가 가상 그림을 실제 픽셀 캡처에 제공한다.
#pragma once
#include <cstdint>
#include <functional>
#include <span>

namespace netstorm::o {
// 보호10.78의 TYPE72/VT509930 쌍만 받고 다른 홈의 파생 그림을 추정하지 않는다.
bool IsSunAviaryCapture(std::span<const std::uint8_t> raw);
// 0045b530의 extra 관문·signed 프레임 구간·기본 그림 없음(-1)을 그대로 반환한다.
std::int32_t SunAviaryCaptureMainFrame(std::span<const std::uint8_t> raw);
// 0045b560은 extra&9이면 생략하고 나머지는 현재 타입/프레임을 한 번 제출한다.
void DrawSunAviaryCapture(std::span<const std::uint8_t> raw,
    const std::function<void(std::uint32_t,std::uint32_t,std::int32_t,std::int32_t)>& draw,std::int32_t x,std::int32_t y);
}
