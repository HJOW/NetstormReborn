// 로컬 미리보기는 배치 허용 결과와 별개이며, 패치의 지도 가장자리 거부도 부분 배열과 함께 보존한다.
#include "o/RawCanonPlacementPreview.h"
#include "o/RawCanonPlacementGeometry.h"
#include <algorithm>
#include <bit>
#include <limits>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 원본 지도/미리보기 한 변과 표면 genus, raw 타입/소유자 오프셋이다.
constexpr int kMapSide=256,kPreviewSide=12;
// 패치판의 미리보기는 y=254부터 거부한다. CD판에는 이 조기 거부가 없다.
constexpr int kPatchRowEnd=254;
constexpr std::uint32_t kSurface=2;
constexpr std::size_t kType=10,kPatchOwner=34,kCdOwner=32;
// 반복 범위 +1/-1과 미리보기 인덱스 산술을 정의할 수 없는 입력은 진단한다.
void CheckBounds(SquidSearchArea area) {
    if (area.right==std::numeric_limits<int>::max() || area.bottom==std::numeric_limits<int>::max() ||
        area.right<std::numeric_limits<int>::min()+kPreviewSide || area.bottom<std::numeric_limits<int>::min()+kPreviewSide)
        throw std::out_of_range("일반 배치 미리보기 사각형 산술 범위 오류");
}
}
// 판본별 타입 표와 두 256×256 지도 및 하위 경계를 확인한다.
RawCanonPlacementPreview::RawCanonPlacementPreview(const SidPool& pool,std::span<const RiftTypeRecord> types,
    std::span<const std::uint8_t> spots,std::span<const std::uint16_t> surfaceMap,
    CanonPlacementPreviewState& state,CanonPlacementPreviewHooks hooks):pool_(pool),types_(types),spots_(spots),surfaceMap_(surfaceMap),state_(state),hooks_(std::move(hooks)) {
    const auto count=pool.Edition()==OriginalEdition::Patch1078 ? 188U : 171U;
    if (types.size()!=count || spots.size()!=kMapSide*kMapSide || surfaceMap.size()!=spots.size() || !hooks_.bounds || !hooks_.inspectCollisions)
        throw std::invalid_argument("일반 배치 미리보기 자료/하위 경계 누락");
}
// 원본 함수는 요청자의 BYTE를 부호 확장하지만 표면 소유자는 부호 없이 읽는다.
bool RawCanonPlacementPreview::Related(std::uint8_t surfaceOwner,std::int32_t owner) const {
    if (state_.editor || surfaceOwner==owner) return true;
    if (!state_.useAlliances) return false;
    // 요청자 -1도 표면 소유자 1과 조합하면 인덱스 8이다. 개별 소유자 대신 실제 인덱스를 검사한다.
    const auto index=static_cast<std::int64_t>(surfaceOwner)*9+owner;
    if (index<0 || index>=static_cast<std::int64_t>(state_.alliances.size())) throw std::out_of_range("일반 배치 미리보기 관계 표 범위 오류");
    return state_.alliances[static_cast<std::size_t>(index)]!=0;
}
// CD는 먼저 linear spot을 읽고 표면 조회에서만 x/y를 검사한다. 메모리 밖 읽기는 C++에서 진단한다.
bool RawCanonPlacementPreview::CellBlocked(int x,int y,std::int32_t owner) const {
    const auto index=static_cast<std::int64_t>(y)*kMapSide+x;
    if (index<0 || index>=static_cast<std::int64_t>(spots_.size())) throw std::out_of_range("일반 배치 미리보기 공간 읽기 범위 오류");
    if (spots_[static_cast<std::size_t>(index)]&0x10) return true;
    if (x<0 || x>=kMapSide || y<0 || y>=kMapSide) return true;
    const Sid surface{surfaceMap_[static_cast<std::size_t>(index)]};if (!surface.value) return true;
    const auto raw=pool_.Slot(surface);const auto type=raw[kType];
    if (type>=types_.size()) throw std::out_of_range("일반 배치 미리보기 표면 타입 범위 오류");
    if (!(types_[type].flags2&kSurface)) return false;
    return !Related(raw[pool_.Edition()==OriginalEdition::Patch1078 ? kPatchOwner : kCdOwner],owner);
}
// y를 먼저 감소시키고 각 행에서 x를 감소시킨다. 배열 인덱스는 x 쪽을 바깥 차원으로 쓴다.
bool RawCanonPlacementPreview::Inspect(CanonPlacementQuery query,bool localOwner) const {
    if (!localOwner) return hooks_.inspectCollisions(query,localOwner);
    state_.blocked.fill(0);auto area=hooks_.bounds(query);CheckBounds(area);area.left=std::max(area.left,1);area.top=std::max(area.top,1);
    // 비어 있는 축에는 지도/배열 효과가 없다. 다른 축이 길어도 불필요한 빈 반복을 수행하지 않는다.
    if (area.right+1<area.left-1 || area.bottom+1<area.top-1) return hooks_.inspectCollisions(query,localOwner);
    const auto byte=static_cast<std::uint8_t>(query.owner);const auto owner=byte<128 ? static_cast<std::int32_t>(byte) : static_cast<std::int32_t>(byte)-256;
    // 오른쪽 아래+1에서 왼쪽 위-1까지 포함한다. 빈 사각형이면 모양 조회와 초기화만 수행한다.
    for (int y=area.bottom+1;y>=area.top-1;--y) {
        // 각 행에서 오른쪽 주변 칸부터 왼쪽 주변 칸까지 검사한다.
        for (int x=area.right+1;x>=area.left-1;--x) {
            if (pool_.Edition()==OriginalEdition::Patch1078 && (x<0 || y<0 || x>=kMapSide || y>=kPatchRowEnd)) return false;
            const auto px=static_cast<std::int64_t>(area.right)-x+1,py=static_cast<std::int64_t>(area.bottom)-y+1;
            if (px<0 || py<0 || px>=kPreviewSide || py>=kPreviewSide) throw std::out_of_range("일반 배치 미리보기 12×12 배열 범위 오류");
            if (CellBlocked(x,y,owner)) state_.blocked[static_cast<std::size_t>(px)*kPreviewSide+static_cast<std::size_t>(py)]=1;
        }
    }
    return hooks_.inspectCollisions(query,localOwner);
}
// 같은 판본/풀의 배치 접두에 연결한다.
const SidPool& RawCanonPlacementPreview::Pool() const { return pool_; }
// 모양과 실제 충돌 경계의 수명은 호출자가 보장한다.
CanonPlacementHooks MakeCanonPlacementPreviewHooks(const SidPool& pool,const RawCanonPlacementPreview& preview,CanonPlacementHooks hooks) {
    if (&pool!=&preview.Pool()) throw std::invalid_argument("일반 배치 미리보기/배치의 SID 풀이 다릅니다");
    hooks.inspectGeometry=[&preview](const CanonPlacementQuery& query,bool localOwner) { return preview.Inspect(query,localOwner); };return hooks;
}
// 별도 패턴 인자와 방향의 비트를 보존하며 명시 프레임을 사용하지 않는다.
CanonPlacementPreviewHooks MakeCanonGeometryPreviewHooks(const SidPool& pool,const RawCanonPlacementGeometry& geometry,CanonPlacementPreviewHooks hooks) {
    if (&pool!=&geometry.Pool()) throw std::invalid_argument("일반 모양/미리보기의 SID 풀이 다릅니다");
    hooks.bounds=[&geometry](const CanonPlacementQuery& query) {
        return geometry.Bounds({query.type,std::bit_cast<std::int32_t>(query.argument),std::bit_cast<std::int32_t>(query.flags),query.x,query.y,false});
    };
    return hooks;
}
}
