// 지역record의별도local난수와실제군집순서/실패budget을보존한다.
#include "o/RawFarmCluster.h"
#include <bit>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 원본시드배율/수량배율과중심실패한도다.
constexpr std::uint32_t kSeedFactor=77,kQuantityFactor=20,kFailureLimit=30000;
// 두판본의localRandom helper가쓰는CRT64비트절삭의하위DWORD다.
std::int32_t Truncate(double value) {
    if (!std::isfinite(value) || std::fabs(value)>=9223372036854775808.0) return 0;
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::int64_t>(value)));
}
// limit이1보다작으면원본localRandom을소비하지않고0을반환한다.
std::uint32_t Next(GameRandom& random,std::int32_t limit) { return limit<1 ? 0U : random.Next(static_cast<std::uint32_t>(limit)); }
}
// 자료와필수효과가서로다른월드에연결되지않게검사한다.
RawFarmCluster::RawFarmCluster(const SidPool& pool,const ChunkMap& chunks,const RawIslandRuntime& islands,
    const FortRandomPlacementState& placementState,const FarmClusterState& state,FarmClusterHooks hooks)
    :pool_(pool),chunks_(chunks),islands_(islands),placementState_(placementState),state_(state),hooks_(std::move(hooks)) {
    if (&pool!=&islands.Pool() || !hooks_.place || !hooks_.assertion) throw std::invalid_argument("FarmCluster 같은월드/필수효과누락");
}
// 실제지역record조회는currentowner가아니라청크의원래Territory record를읽는다.
std::uint32_t RawFarmCluster::RegionSeed(std::int32_t region) const {
    const bool cd=pool_.Edition()==OriginalEdition::Cd1072;
    if (cd && !islands_.Exists(region)) hooks_.assertion(FarmClusterAssertion::MissingIsland);
    // 원본선형검색은x가먼저증가하고끝에서y가증가한다.
    for (std::int32_t y=0;y<chunks_.YLength();++y) {
        // 같은현재ChunkMap::At getter를재사용한다.
        for (std::int32_t x=0;x<chunks_.XLength();++x) {
            const auto& entry=chunks_.At(x,y);
            if (static_cast<std::int32_t>(std::bit_cast<std::int8_t>(entry.island))==region) return static_cast<std::uint32_t>(entry.record[3])*kSeedFactor;
        }
    }
    if (cd) hooks_.assertion(FarmClusterAssertion::MissingRegionRecord);return 0;
}
// 원본요청값은snapshot이고실제edgeFarm타입은각중심/주변호출에서현재값을읽는다.
void RawFarmCluster::Place(FortRandomRectangle area,std::int32_t region,std::uint32_t quantity) const {
    GameRandom random(RegionSeed(region));std::uint32_t remaining=quantity*kQuantityFactor,force=0;std::int32_t radius=1;
    if (state_.Archipelago() && region==0) {
        const auto span=std::bit_cast<std::uint32_t>(state_.archipelagoSpan),extension=span*16U;
        const auto start=std::bit_cast<std::int32_t>(extension-16U),end=std::bit_cast<std::int32_t>(extension);
        area.left=static_cast<float>(static_cast<double>(area.left)-start);area.top=static_cast<float>(static_cast<double>(area.top)-start);
        area.right=static_cast<float>(static_cast<double>(area.right)+end);area.bottom=static_cast<float>(static_cast<double>(area.bottom)+end);
        const auto width=span*2U+2U;remaining=static_cast<std::uint32_t>(std::bit_cast<std::int32_t>(remaining)/9)*width*width;force=1;radius=2;
    }
    const auto height=Truncate(static_cast<double>(area.bottom)-area.top),width=Truncate(static_cast<double>(area.right)-area.left);
    std::uint32_t failures=kFailureLimit;
    // 원본은최초중심을항상시도하며성공중심마다실패한도를재설정한다.
    do {
        const auto dy=Next(random,height),dx=Next(random,width);
        const auto x=static_cast<float>(static_cast<double>(dx)+area.left),y=static_cast<float>(static_cast<double>(dy)+area.top);
        if (hooks_.place(random,{placementState_.edgeFarmType,x,y,region,1,force})) {
            --remaining;failures=kFailureLimit;
            // 원본주변순회는x바깥/y안쪽이며중심좌표도다시시도한다.
            for (std::int32_t ox=-radius;ox<=radius && remaining;++ox) {
                // 성공할때만remaining을감소시키고0이면남은주변시도를멈춘다.
                for (std::int32_t oy=-radius;oy<=radius && remaining;++oy) {
                    const auto nx=static_cast<float>(static_cast<double>(x)+ox),ny=static_cast<float>(static_cast<double>(y)+oy);
                    if (hooks_.place(random,{placementState_.edgeFarmType,nx,ny,region,1,force})) --remaining;
                }
            }
        }
        if (!remaining) break;
    } while (--failures);
}
// 같은단일pool을후속실제월드소비자에게제공한다.
const SidPool& RawFarmCluster::Pool() const { return pool_; }
// 완료된단일배치의실제공간검사/owner/Retype/Pop을군집에서그대로소비한다.
FarmClusterHooks MakeFarmClusterPlacementHook(const SidPool& pool,const RawFortRandomPlacement& placement,FarmClusterHooks hooks) {
    if (&pool!=&placement.Pool()) throw std::invalid_argument("FarmCluster 단일배치pool불일치");
    hooks.place=[&placement](GameRandom& random,const FortRandomPlaceRequest& request) { return placement.Place(random,request); };return hooks;
}
}
