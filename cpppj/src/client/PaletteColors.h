// 원본 Screen.cpp 팔레트 로더가 채우는 기본/표시/날씨 색 번호 표다. 색의 용도가 미확정인 항목은 목표 RGB로 식별한다.
#pragma once
#include "o/RiftType.h"
#include <array>
#include <span>
#include <vector>

namespace netstorm::client {
// 10.78의 색 검색 호출은 중복 RGB를 포함해 58개, CD판은 추가 여덟 색을 제외한 50개다.
inline constexpr std::size_t kPatchPaletteColorCount=58;
inline constexpr std::size_t kCdPaletteColorCount=50;
// 원본 색 표의 한 항목이다. 같은 RGB를 서로 다른 전역에 적는 호출도 원본 순서대로 별도 항목으로 남긴다.
struct NamedPaletteColor {
    std::uint8_t red{},green{},blue{};
    std::uint32_t index{};
};
// 기본 9색(검정/빨강/초록/파랑/자홍/노랑/주황/흰색/회색)과 날씨 4색의 별칭, 전체 호출 순서의 색 표다.
struct PaletteColorTable {
    std::array<std::uint32_t,9> basic{};
    std::array<std::uint32_t,4> weather{}; // 바람/비/천둥/해: 기본 노랑/파랑/빨강과 갈색 항목을 그대로 복사한다.
    std::vector<NamedPaletteColor> named;
};
// 실제 논리 팔레트의 검색 함수로 원본 전체 색 표를 계산한다. 기본색/날씨 별칭은 새 검색 없이 계산된 번호를 복사한다.
// 사용: logical은 R/G/B/플래그 순서의 256 DWORD다. edition으로 CD판에 없는 여덟 목표 RGB를 제외한다.
PaletteColorTable BuildPaletteColorTable(std::span<const std::uint32_t,256> logical,o::OriginalEdition edition=o::OriginalEdition::Patch1078);
}
