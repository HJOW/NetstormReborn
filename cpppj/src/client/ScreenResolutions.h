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
// 7차 FSR 1.0: 아래 두 해상도는 실제 논리 화면을 1920x1080으로 그린 뒤 FSR 1.0으로 업스케일해 만드는 표시 해상도다.
// FSR을 쓸 수 있는 환경(Direct3D 11 장치 생성 가능)에서만 선택 목록에 나타나며 선택하면 FSR이 자동으로 켜진다.
inline constexpr std::array<std::array<std::uint32_t,2>,2> kFsrScreenResolutionChoices{{{2560,1440},{3840,2160}}};
// FSR 해상도의 실질(논리) 렌더링 크기다. 그래픽 크기는 이 1920x1080 기준이며 FSR이 표시 크기로 늘린다.
inline constexpr std::uint32_t kFsrLogicalWidth = 1920, kFsrLogicalHeight = 1080;
// 폭/높이가 FSR 전용 표시 해상도인지 확인한다.
inline constexpr bool IsFsrScreenResolution(std::int64_t width, std::int64_t height) {
    for (const auto& size : kFsrScreenResolutionChoices) if (static_cast<std::int64_t>(size[0]) == width && static_cast<std::int64_t>(size[1]) == height) return true; // 두 FSR 행과 폭/높이를 함께 대조한다.
    return false;
}
// 폭/높이가 선택 목록에 있는 지원 해상도인지 확인한다. 설정 저장·해상도 변경이 목록 밖의 임의 크기를 받지 않게 한다.
// fsrAvailable이 참이면(7차) FSR 전용 표시 해상도도 허용한다. 기본값은 거짓이라 기존 호출의 판정은 그대로다.
inline constexpr bool IsSupportedScreenResolution(std::int64_t width, std::int64_t height, bool fsrAvailable = false) {
    for (const auto& size : kScreenResolutionChoices) if (static_cast<std::int64_t>(size[0]) == width && static_cast<std::int64_t>(size[1]) == height) return true; // 목록의 각 행과 폭/높이를 함께 대조한다.
    return fsrAvailable && IsFsrScreenResolution(width, height);
}
// 표시 해상도에 대응하는 실제 논리(렌더링) 화면 크기를 돌려준다. FSR 해상도는 1920x1080이고 나머지는 그대로다.
inline constexpr std::array<std::uint32_t,2> LogicalScreenSize(std::int64_t width, std::int64_t height) {
    if (IsFsrScreenResolution(width, height)) return {kFsrLogicalWidth, kFsrLogicalHeight};
    return {static_cast<std::uint32_t>(width), static_cast<std::uint32_t>(height)};
}
}
