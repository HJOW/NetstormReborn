// 원본 GUI 질감의 밝음/어두움 색 번호 변환표다. 일시 팔레트 효과와 별도로 파일 팔레트에서 만든다.
#pragma once
#include <array>
#include <cstdint>
#include <span>

namespace netstorm::client {
// 0043c010/CD 00496530의 두 RGB 명암 표다. 소스 팔레트 번호를 목표 팔레트 번호로 변환한다.
struct GumpShadeMaps {
    std::array<std::uint8_t,256> bright{},dark{};
};
// R/G/B/flags 순서 논리 팔레트의 RGB를 정규화하고 원본 거리 비교·float32 최솟값 저장으로 표를 만든다.
// 밝음 후보 229~245는 제외한다. flags 바이트는 검색에 쓰지 않는다. 다른 일곱 GUI 표/파일 캐시는 후속이다.
GumpShadeMaps BuildGumpShadeMaps(std::span<const std::uint32_t,256> logical);
}
