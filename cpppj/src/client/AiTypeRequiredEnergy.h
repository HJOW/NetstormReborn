// 실제GameAssets의mana AST와현재TYPE를두AI몸체의같은getter에연결한다.
#pragma once
#include "client/GameAssets.h"
#include <functional>

namespace netstorm::client {
// TYPE/AST를복사하지않고매조회현재원문을읽는다. assets/currentTypes는반환getter보다오래살아야한다.
std::function<std::string(std::uint32_t)> BindAiTypeRequiredEnergy(const GameAssets& assets,std::span<const o::RiftTypeRecord> currentTypes);
}
