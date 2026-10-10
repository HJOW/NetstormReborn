// 건설 확정의 그래프 기준점과 배치 통지의 소유 사제 조회를 실제 몸체에 연결한다.
#pragma once
#include "o/RawConstructionNotice.h"
#include "o/RawPlayerPlacementAnchor.h"
#include "o/RawPriestLookup.h"

namespace netstorm::o {
// 0048fdb0 / CD 00406f80을 확정의 findBuilder에 연결한다. 반환은 작업장/그래프 기준점이다.
// anchor와 그 공유 상태는 훅보다 오래 살아야 한다. 나머지 확정 효과는 기존 훅을 유지한다.
ConstructionConfirmHooks MakeConstructionAnchorHooks(const SidPool& pool,const RawPlayerPlacementAnchor& anchor,ConstructionConfirmHooks hooks);
// 004921f0 / CD 0040d940을 배치 통지의 findPriest에 연결한다. 매 호출마다 현재 공유 목록을 읽는다.
// lookup과 그 공유 상태는 훅보다 오래 살아야 한다. 이동/contained 등의 효과는 별도 연결한다.
ConstructionNoticeHooks MakeConstructionPriestLookupHooks(const SidPool& pool,const RawPriestLookup& lookup,ConstructionNoticeHooks hooks);
}
