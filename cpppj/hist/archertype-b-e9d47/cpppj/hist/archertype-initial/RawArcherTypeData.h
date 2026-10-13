// 원본 TYPE 초기화와 .type callback의 실제 방향수/물리프레임수를 Archer 소비자에 공급한다.
#pragma once
#include "o/RawArcherAnimation.h"

namespace netstorm::o {
// TYPE+110은 초기값0을 유지하며 명시적인 orientations만 float 절삭으로 덮어쓴다.
// TYPE+114는 .type 클러스터 개수다. sunArcher의 실제 값0/35는 Fire에서 사용한다.
ArcherTypeData ReadRawArcherTypeData(const RiftTypeDefinition& definition);
}
