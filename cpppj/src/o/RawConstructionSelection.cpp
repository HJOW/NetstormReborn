// 동일 풀의 두 서로 다른 조회를 원본의 각 호출 지점에 연결한다.
#include "o/RawConstructionSelection.h"
#include <stdexcept>

namespace netstorm::o {
// 기준점 몸체가 검증한 DWORD 결과를 기존 SID 경계에 전달한다.
ConstructionConfirmHooks MakeConstructionAnchorHooks(const SidPool& pool,const RawPlayerPlacementAnchor& anchor,ConstructionConfirmHooks hooks) {
    if (&pool!=&anchor.Pool()) throw std::invalid_argument("건설 기준점 어댑터의 풀 연결 오류");
    hooks.findBuilder=[&anchor](std::uint32_t player,std::uint32_t type,float x,float y) {
        return Sid{static_cast<std::uint16_t>(anchor.Locate(player,type,x,y))};
    };
    return hooks;
}
// 목록의 DWORD SID는 실제 조회가 풀 범위를 검사한 뒤에만 WORD로 전달한다.
ConstructionNoticeHooks MakeConstructionPriestLookupHooks(const SidPool& pool,const RawPriestLookup& lookup,ConstructionNoticeHooks hooks) {
    if (&pool!=&lookup.Pool()) throw std::invalid_argument("건설 소유 사제 어댑터의 풀 연결 오류");
    hooks.findPriest=[&lookup](std::uint32_t player) { return Sid{static_cast<std::uint16_t>(lookup.Find(player))}; };
    return hooks;
}
}
