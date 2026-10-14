// 원본 현재 getter를 연결하며 표적 검색을 별도 거리 모형으로 바꾸지 않는다.
#include "client/RawCampaignGunQueries.h"
#include <bit>
#include <stdexcept>

namespace netstorm::client {
namespace {
// 정렬되지 않은 패치DWORD/CD BYTE 프레임만 현재 raw 슬롯에서 읽는다.
std::uint32_t Frame(const o::SidPool& pool,o::Sid sid) {
    const auto raw=pool.Slot(sid);if (pool.Edition()!=o::OriginalEdition::Patch1078) return raw[34];std::uint32_t value=0;
    // 현재 DWORD의 네 바이트를 원본 little endian으로 조합한다.
    for (std::size_t i=0;i<4;++i) value|=static_cast<std::uint32_t>(raw[36+i])<<(8*i);return value;
}
// 현재 float 필드를 원본 비트로 해석한다.
float Position(std::span<const std::uint8_t> raw,std::size_t offset) {
    std::uint32_t value=0;
    // 원본DWORD 전체를 읽으며 인접 필드를 섞지 않는다.
    for (std::size_t i=0;i<4;++i) value|=static_cast<std::uint32_t>(raw[offset+i])<<(8*i);return std::bit_cast<float>(value);
}
}
// 같은 Pool/TYPE/시각 자료만 합성하고 감시/애니메이션의 큰 하위는 제공한 실제 경계에 유지한다.
o::GunProcessHooks MakeCampaignGunQueryHooks(RawWorldCore core,const o::RawGunMode& mode,
    const o::RawGunTarget& target,const o::RawGunHit& hit,const o::GunProcessState& state,
    std::span<const float> intervals,o::GunProcessHooks hooks) {
    if (&mode.Pool()!=&core.pool || &target.Pool()!=&core.pool || &hit.Pool()!=&core.pool || intervals.size()!=core.types.size())
        throw std::invalid_argument("캠페인 Gun query의 같은 Pool/TYPE 자료 불일치");
    hooks.targetingMode=[&mode](o::Sid sid,std::uint8_t) { return mode.Get(sid); };
    hooks.findTarget=[&target](float x,float y,std::uint32_t type,std::uint32_t direction,std::uint8_t owner) { return target.FindTarget(x,y,type,std::bit_cast<std::int32_t>(direction),owner); };
    hooks.refreshRange=[&hit,&state](o::Sid sid) { hit.SetTime(sid,state.now); };
    hooks.shotContext=[pool=&core.pool,&mode,&target](o::Sid sid) {
        const auto raw=pool->Slot(sid);const auto center=target.Center(sid);const auto owner=raw[pool->Edition()==o::OriginalEdition::Patch1078 ? 34U : 32U];
        return std::array<std::uint32_t,8>{std::bit_cast<std::uint32_t>(Position(raw,14)),std::bit_cast<std::uint32_t>(Position(raw,18)),
            std::bit_cast<std::uint32_t>(center[0]),std::bit_cast<std::uint32_t>(center[1]),raw[10],mode.Get(sid),owner,Frame(*pool,sid)};
    };
    hooks.firingInterval=[pool=&core.pool,intervals](o::Sid sid) { const auto type=pool->Slot(sid)[10];if (type>=intervals.size()) throw std::out_of_range("캠페인 현재 발사간격 TYPE 범위");return static_cast<double>(intervals[type]); };
    return hooks;
}
}
