// 원본 생산 아이콘의 비활성 및 선행 출력 색표다.
#pragma once
#include "client/Renderer.h"
namespace netstorm::client {
struct ProductionColorMaps { ColorMap disabled{},underlay{}; }; // 556038/556338의 실제256바이트다.
ProductionColorMaps BuildProductionColorMaps(std::span<const std::uint32_t,256> logical); // 전체43c010의 해당 두 출력을 계산한다.
}
