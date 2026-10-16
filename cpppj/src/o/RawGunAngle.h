// PATCH00467610의 Gunanim 프레임→Graph 방향 계산을 복원한다.
#pragma once
#include <array>
#include <cstdint>
#include <stdexcept>

namespace netstorm::o {
// 실제 프레임을5로 나눈 인덱스를 사용하며 상한19와 원본 각도 경계를 보존한다.
inline std::uint32_t PatchGunGraphFromFrame(std::int32_t frame) {
    // DAT00540940의 실제20개 binary32 각도다. 프레임 번호 자체를 각도로 간주하지 않는다.
    constexpr std::array<float,20> angles{0,21,35,52,71,86,97,115,121,148,180,205,237,256,272,282,291,314,328,345};
    auto index=frame/5; // MSVC signed 정수 나눗셈은 음수도0 방향으로 절삭한다.
    if (index>18) index=19;
    if (index<0) throw std::out_of_range("Gunanim 음수 인덱스의 원본 무효 메모리 경로");
    const auto angle=angles[static_cast<std::size_t>(index)]; // 원본은 범위 밖 음수에서 진단 후 무효 메모리를 읽는다.
    if (angle>=45) { if (angle<135) return 1;if (angle<225) return 2;if (angle<315) return 3; }
    return 0;
}
}
