// 원본4:3 세 항목과 Windows10/11 호환 출력의16:9/16:10 추가 선택을 두 메뉴에서 공유한다.
#pragma once
#include <array>
#include <cstdint>
#include <span>

namespace netstorm::client { // 실제 논리 화면 크기이며 모니터 장치 모드를 바꾸는 목록이 아니다.
// 앞 세 항목은 원본 순서다. 뒤 다섯 항목은 원본과 다른 Windows10/11 DIB/화면비 지원 요구에 따라 추가했다.
// 1600x900·1920x1080은 4차(와이드 16:9, 최대 1920x1080) 요구다. 그래픽 크기는 그대로이며 화면 영역만 넓어진다.
inline constexpr std::array<std::array<std::uint32_t,2>,8> kScreenResolutionChoices{{{640,480},{800,600},{1024,768},{1280,720},{1280,800},{1366,768},{1600,900},{1920,1080}}};
// 원문 몸체의 기본 입력/조회 순서를 유지하는 세4:3 행이다.
inline constexpr std::span<const std::array<std::uint32_t,2>> kOriginalScreenResolutions{kScreenResolutionChoices.data(),3};
// 실제 호환 Client만 명시적으로 추가하는16:9·16:10·DIB 홀수 폭 다섯 행이다.
inline constexpr std::span<const std::array<std::uint32_t,2>> kCompatibilityScreenResolutions{kScreenResolutionChoices.data()+3,5};
// 폭/높이가 선택 목록에 있는 지원 해상도인지 확인한다. 설정 저장·해상도 변경이 목록 밖의 임의 크기를 받지 않게 한다.
inline constexpr bool IsSupportedScreenResolution(std::int64_t width, std::int64_t height) {
    for (const auto& size : kScreenResolutionChoices) if (static_cast<std::int64_t>(size[0]) == width && static_cast<std::int64_t>(size[1]) == height) return true; // 목록의 각 행과 폭/높이를 함께 대조한다.
    return false;
}
}
