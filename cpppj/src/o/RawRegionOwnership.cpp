// 원본의 반복 조회·중립 처리·마지막 작업장 테마·도장 후 재투표를 보존한다.
#include "o/RawRegionOwnership.h"
#include "o/RawCanonPlacementTerrain.h"
#include <bit>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 비정렬 raw 좌표와 두 판본에 같은 float snap 편향의 정확한 비트다.
constexpr std::size_t kX=14,kY=18;
constexpr float kSnapBias=std::bit_cast<float>(std::uint32_t{0x3f7fff58});
// little endian 좌표 비트를 조립하며 원본 float 값을 보존한다.
float Coordinate(std::span<const std::uint8_t> raw,std::size_t offset) {
    std::uint32_t bits=0;
    // 각 바이트를 원본 DWORD 위치에 넣는다.
    for (std::size_t i=0;i<4;++i) bits|=static_cast<std::uint32_t>(raw[offset+i])<<(8*i);
    return std::bit_cast<float>(bits);
}
// 유효한 int 범위만 지원하며 x87처럼 double 합산 후 0 방향으로 절삭한다.
int Truncate(double value) {
    const auto result=std::trunc(value);
    if (!std::isfinite(result) || result<std::numeric_limits<int>::min() || result>std::numeric_limits<int>::max())
        throw std::out_of_range("지역 소유 좌표 절삭 범위 오류");
    return static_cast<int>(result);
}
}
// 필수 경계와 서로 다른 전역 목록/테마 표를 먼저 확인한다.
RawRegionOwnership::RawRegionOwnership(const SidPool& pool,SquidPostPopList& additional,SquidPostPopList& workshops,
    std::span<const std::uint32_t> themes,RegionOwnershipState& state,RegionOwnershipHooks hooks)
    :pool_(pool),additional_(additional),workshops_(workshops),themes_(themes),state_(state),hooks_(std::move(hooks)) {
    if (&additional==&workshops || !hooks_.regionAt || !hooks_.paint ||
        (pool.Edition()==OriginalEdition::Patch1078 && (!hooks_.themeMode || themes.size()<188)))
        throw std::invalid_argument("지역 소유 목록/테마/필수 효과 연결 오류");
}
// getter 호출 뒤의 현재 소유자를 읽으며 query 지역은 항목마다 다시 조회한다.
void RawRegionOwnership::Vote(SquidPostPopList& list,float x,float y,std::array<std::int32_t,9>& votes,
    const std::size_t* affectedIndex) const {
    // 동적 count/목록 변경을 반영하고 중복 SID도 독립적인 표로 센다.
    for (std::size_t i=0;i<list.Items().size();++i) {
        const auto number=list.Items()[i];if (number>65535) throw std::out_of_range("지역 투표 SID 범위 오류");
        const Sid sid{static_cast<std::uint16_t>(number)};const auto raw=pool_.Slot(sid);
        const auto region=hooks_.regionAt(Coordinate(raw,kX),Coordinate(raw,kY));
        const auto target=affectedIndex ? state_.affected.at(*affectedIndex).region : hooks_.regionAt(x,y);
        if (region!=target) continue;
        const auto owner=pool_.Slot(sid)[pool_.Edition()==OriginalEdition::Patch1078 ? 34 : 32];
        if (owner>=votes.size() || votes[owner]==std::numeric_limits<std::int32_t>::max())
            throw std::out_of_range("지역 투표 소유자/표 개수 오류");
        ++votes[owner];
    }
}
// CD는 한 번 도장하며 패치는 초기 도장·최종 도장·필요한 누적 재투표를 수행한다.
void RawRegionOwnership::Update(float x,float y,std::uint32_t mode) const {
    const bool patch=pool_.Edition()==OriginalEdition::Patch1078,special=patch && mode==10;
    const int paintX=Truncate(x),paintY=Truncate(y);std::array<std::int32_t,9> votes{};
    if (special) {
        // 지역마다 표를 초기화하지 않으며 중복 지역도 다시 누적한다.
        for (std::size_t index=0;index<state_.affected.size();++index) {
            Vote(additional_,x,y,votes,&index);Vote(workshops_,x,y,votes,&index);
        }
        mode=0;
    } else {
        if (patch) state_.affected.clear();Vote(additional_,x,y,votes,nullptr);Vote(workshops_,x,y,votes,nullptr);
    }
    std::uint32_t owner=0;std::int32_t best=0;bool tied=false;
    // 소유자 0도 정상 투표 대상이며 더 큰 최대값이 나오면 앞선 동률을 해제한다.
    for (std::uint32_t i=0;i<votes.size();++i) {
        if (votes[i]>best) { best=votes[i];owner=i;tied=false; } else if (votes[i]==best) tied=true;
    }
    if (!patch) { hooks_.paint({paintX,paintY,tied ? 0U : owner,0});return; }
    std::uint32_t theme=0;
    if (tied || owner==0) {
        hooks_.paint({paintX,paintY,1,0});state_.affected.clear();owner=0;
    } else {
        if (mode==0) {
            // 지역과 무관하게 같은 소유자의 마지막 작업장 테마가 남는다.
            for (std::size_t i=0;i<workshops_.Items().size();++i) {
                const auto number=workshops_.Items()[i];if (number>65535) throw std::out_of_range("지역 테마 SID 범위 오류");
                const auto raw=pool_.Slot(Sid{static_cast<std::uint16_t>(number)});if (raw[34]!=owner) continue;
                if (raw[10]>=themes_.size()) throw std::out_of_range("지역 테마 타입 오류");
                const auto kind=themes_[raw[10]];const auto enabled=hooks_.themeMode();
                theme=!enabled ? 0U : kind==0 ? 2U : kind==1 ? 3U : kind==2 ? 1U : 0U;
            }
        }
        hooks_.paint({paintX,paintY,0,0});
    }
    hooks_.paint({paintX,paintY,owner,theme});++state_.revision;state_.dirty=1;
    if (!special && state_.affected.size()>1) Update(x,y,10);
}
// 연결 객체들이 같은 실제 raw 슬롯을 조회하는지 확인할 수 있다.
const SidPool& RawRegionOwnership::Pool() const { return pool_; }
// double 정밀도로 편향을 더하고 float에 저장한 정수값을 기존 지역 getter에 넘긴다.
RegionOwnershipHooks MakeTerrainRegionOwnershipHooks(const SidPool& pool,const RawCanonPlacementTerrain& terrain,RegionOwnershipHooks hooks) {
    if (&pool!=&terrain.Pool()) throw std::invalid_argument("지역 소유/지형 SID 풀이 다릅니다");
    hooks.regionAt=[&terrain](float x,float y) {
        const auto sx=static_cast<float>(Truncate(static_cast<double>(x)+kSnapBias));
        const auto sy=static_cast<float>(Truncate(static_cast<double>(y)+kSnapBias));
        return terrain.RegionAt(Truncate(sx),Truncate(sy));
    };return hooks;
}
// outpost의 원본 지역 통지는 항상 mode 0으로 투표를 시작한다.
OutpostLifecycleHooks MakeOutpostRegionOwnershipHooks(const SidPool& pool,const RawRegionOwnership& ownership,OutpostLifecycleHooks hooks) {
    if (&pool!=&ownership.Pool()) throw std::invalid_argument("outpost/지역 소유 SID 풀이 다릅니다");
    hooks.regionChanged=[&ownership](float x,float y) { ownership.Update(x,y); };return hooks;
}
}
