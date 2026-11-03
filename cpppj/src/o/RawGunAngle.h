// PATCH00467610의 Gunanim 프레임→Graph 방향 계산을 복원한다.
#pragma once
#include <array>
#include <cstdint>
#include <stdexcept>
#include "o/RawCheckingFlag.h"
#include <functional>
#include <string_view>

namespace netstorm::o {
// 현재 진단을 선택한 뒤 복귀해도 지역 인덱스를 바꾸지 않는다. 음수 반환은 표 역참조의 허가가 아니다.
inline std::int32_t PatchGunFrameIndex(std::int32_t frame,RawCheckingFlag checking={},
    const std::function<void(std::string_view,std::string_view,std::uint32_t)>& assertion={}) {
    auto index=frame/5; // 음수도0 방향으로 절삭하며 원문 signed 나눗셈을 유지한다.
    if (index>18) index=19;
    if (checking && (index<0 || index>=20)) {
        if (!assertion) throw std::logic_error("Gunanim 인덱스 원문 진단 장치 미연결");
        assertion("0 <= index && index < 20","\\Ns\\O\\Gunanim.cpp",0x387);
    }
    return index;
}
// 실제 프레임을5로 나눈 인덱스를 사용하며 상한19와 원본 각도 경계를 보존한다.
inline std::uint32_t PatchGunGraphFromFrame(std::int32_t frame,RawCheckingFlag checking={},
    const std::function<void(std::string_view,std::string_view,std::uint32_t)>& assertion={}) {
    // DAT00540940의 실제20개 binary32 각도다. 프레임 번호 자체를 각도로 간주하지 않는다.
    constexpr std::array<float,20> angles{0,21,35,52,71,86,97,115,121,148,180,205,237,256,272,282,291,314,328,345};
    const auto index=PatchGunFrameIndex(frame,checking,assertion); // 진단에서 복귀한 동일 지역 인덱스를 사용한다.
    if (index<0) throw std::out_of_range("Gunanim 음수 인덱스의 원본 무효 메모리 경로");
    const auto angle=angles[static_cast<std::size_t>(index)]; // 원본은 범위 밖 음수에서 진단 후 무효 메모리를 읽는다.
    if (angle>=45) { if (angle<135) return 1;if (angle<225) return 2;if (angle<315) return 3; }
    return 0;
}
}
