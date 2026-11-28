// 원본4:3 세 항목과 Windows10/11 호환 출력의16:9/16:10 추가 선택을 두 메뉴에서 공유한다.
#pragma once
#include <array>
#include <cstdint>
#include <span>

namespace netstorm::client { // 실제 논리 화면 크기이며 모니터 장치 모드를 바꾸는 목록이 아니다.
// 원본 세 행 뒤에 최대 폭1280의16:9·16:10·4:3·5:4를 둔다. 그래픽 크기는 그대로다.
inline constexpr std::array<std::array<std::uint32_t,2>,7> kScreenResolutionChoices{{{640,480},{800,600},{1024,768},{1280,720},{1280,800},{1280,960},{1280,1024}}};
// 원문 몸체의 기본 입력/조회 순서를 유지하는 세4:3 행이다.
inline constexpr std::span<const std::array<std::uint32_t,2>> kOriginalScreenResolutions{kScreenResolutionChoices.data(),3};
// 실제 호환 Client가 추가하는 화면비 네 행이다.
inline constexpr std::span<const std::array<std::uint32_t,2>> kCompatibilityScreenResolutions{kScreenResolutionChoices.data()+3,4};
// FSR 출력은1920·2560·3840 폭마다 같은 네 화면비를 유지한다.
// FSR을 쓸 수 있는 환경(Direct3D 11 장치 생성 가능)에서만 선택 목록에 나타나며 선택하면 FSR이 자동으로 켜진다.
inline constexpr std::array<std::array<std::uint32_t,2>,12> kFsrScreenResolutionChoices{{{1920,1080},{1920,1200},{1920,1440},{1920,1536},{2560,1440},{2560,1600},{2560,1920},{2560,2048},{3840,2160},{3840,2400},{3840,2880},{3840,3072}}};
// FSR의 논리 폭과16:9 기본 높이다. 다른 화면비의 높이는 LogicalScreenSize가 비례 계산한다.
inline constexpr std::uint32_t kFsrLogicalWidth = 1280, kFsrLogicalHeight = 720;
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
// FSR 출력은 폭1280과 같은 화면비의720·800·960·1024 높이로 렌더링한다.
inline constexpr std::array<std::uint32_t,2> LogicalScreenSize(std::int64_t width, std::int64_t height) {
    if (IsFsrScreenResolution(width, height)) return {kFsrLogicalWidth, static_cast<std::uint32_t>(height*kFsrLogicalWidth/width)};
    return {static_cast<std::uint32_t>(width), static_cast<std::uint32_t>(height)};
}
// 옛 큰 해상도와 사용 불가능한 FSR 설정은 가장 가까운 화면비의1280 화면으로 옮긴다. 잘못된 작은 크기는 원본1024x768로 복구한다.
inline constexpr std::array<std::uint32_t,2> NormalizeScreenResolution(std::int64_t width, std::int64_t height, bool fsrAvailable) {
    if (IsSupportedScreenResolution(width,height,fsrAvailable)) return {static_cast<std::uint32_t>(width),static_cast<std::uint32_t>(height)};
    if (width<=0 || height<=0 || (width<=1280 && height<=1024)) return {1024,768};
    std::array<std::uint32_t,2> best{1280,720}; double distance=1e100; // 화면비 거리와 가장 가까운 후보를 보관한다.
    for (const auto& size:kCompatibilityScreenResolutions) { // 작은 쪽으로 비례 환산한 세로 길이를 비교한다.
        const double difference=static_cast<double>(height)*1280/static_cast<double>(width)-size[1];
        const double absolute=difference<0 ? -difference:difference;
        if (absolute<distance) {distance=absolute;best=size;}
    }
    return best;
}
}
