// 이전에 검증한 원본 몸체를 재작성하지 않고 현재 월드 참조와 가상 소유자 분배에 연결한다.
#include "client/RawTerrainWorldBinding.h"
#include <stdexcept>
#include <utility>

namespace netstorm::client {
// 첫 생성 이전에 외부 지역 조회/진단을 확인하고 같은 Core의 실제 소비자만 설치한다.
o::TerrainStampHooks MakeTerrainStampWorldHooks(RawSurfaceWorld& world,const o::TerrainStampState& state,
    std::function<void(o::SquidRegionAssert)> regionDiagnostic,o::TerrainStampHooks effects) {
    if (!effects.chunkRegion || !effects.assertion || !regionDiagnostic)
        throw std::invalid_argument("초기 지형 월드의 지역 조회/진단 연결 누락");
    const auto core=world.Core();
    effects.create=[&world](std::uint32_t type,std::uint32_t flags) { return world.Create(type,flags); };
    // 본섬과 파생 받침/종유석의 실제 vtable owner 규칙을 같은 mutable 전역에 분배한다.
    effects.setOwner=o::MakeOwnerDispatch(core.pool,core.bridgeState,
        [&owner=core.owner](o::Sid sid,std::uint32_t player) { owner.Set(sid,player); });
    effects.pop=[&world](o::Sid sid,float x,float y,std::uint32_t flags) { world.Pop(sid,x,y,flags); };
    effects.setRegion=[&pool=core.pool,types=core.types,&state,diagnostic=std::move(regionDiagnostic)](o::Sid sid,std::int32_t region) {
        o::SetSquidRegion(pool,types,sid,static_cast<std::uint32_t>(region),static_cast<std::uint32_t>(state.noRegion),diagnostic);
    };
    return effects;
}
}
