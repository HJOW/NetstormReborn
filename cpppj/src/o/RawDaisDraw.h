// 다이스165의 별도169TYPE 추가 그림 요청을 원본 현재 프레임으로 계산한다.
#pragma once
#include <cstdint>
#include <functional>
#include <span>

namespace netstorm::o {
inline constexpr std::uint32_t kPatchDaisExtraType=169; // 원본005412fc의 고정 자산 로딩 번호(daisExtraFrames)다.
bool IsPatchDais(std::span<const std::uint8_t> raw); // 실제165/507950 쌍을 확인하며 다른 자산에 추가 그림을 적용하지 않는다.
void DrawDaisAdditional(std::span<const std::uint8_t> raw,std::int32_t extraFrameCount,
    const std::function<void(std::uint32_t,std::int32_t)>& draw); // 4484b0의 signed 나머지를497280 소비자에 넘긴다. 추상/로컬 관문은 그 소비자가 처리한다.
}
