// 원본4:3 세 항목과 Windows10/11 호환 출력의16:9/16:10 추가 선택을 두 메뉴에서 공유한다.
#pragma once
#include <array>
#include <cstdint>
#include <span>

namespace netstorm::client { // 실제 논리 화면 크기이며 모니터 장치 모드를 바꾸는 목록이 아니다.
// 앞 세 항목은 원본 순서다. 뒤 세 항목은 원본과 다른 Windows10/11 DIB/화면비 지원 요구에 따라 추가했다.
inline constexpr std::array<std::array<std::uint32_t,2>,6> kScreenResolutionChoices{{{640,480},{800,600},{1024,768},{1280,720},{1280,800},{1366,768}}};
// 원문 몸체의 기본 입력/조회 순서를 유지하는 세4:3 행이다.
inline constexpr std::span<const std::array<std::uint32_t,2>> kOriginalScreenResolutions{kScreenResolutionChoices.data(),3};
// 실제 호환 Client만 명시적으로 추가하는16:9·16:10·DIB 홀수 폭 세 행이다.
inline constexpr std::span<const std::array<std::uint32_t,2>> kCompatibilityScreenResolutions{kScreenResolutionChoices.data()+3,3};
}
