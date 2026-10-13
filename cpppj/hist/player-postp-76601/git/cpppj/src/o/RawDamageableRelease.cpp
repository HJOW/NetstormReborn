// 두 좌표 보정 상수와 콜백 뒤 WORD/현재 전역 재읽기를 구별한다.
#include "o/RawDamageableRelease.h"
#include <bit>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 공통 raw 필드와 해방 후보/사제 genus/부모 중심 보정 마스크다.
constexpr std::size_t kType=10,kState=11,kWord=12,kX=14,kY=18,kKind=18,kMask=20;
constexpr std::uint32_t kReleasable=0x20,kPriest=0x200000,kCenteredParent=0x80400000;
// 좌표 스냅은 0.99999, 공간 조회는 0.9999다. 중간 float 반올림을 넣지 않는다.
constexpr float kSnapBias=0.99999f,kSpotBias=0.9999f;
// 원본 공간 배열의 한 변과 전체 바이트 수다.
constexpr std::size_t kSide=256,kCells=kSide*kSide;
// 비정렬 WORD/DWORD를 원본 리틀엔디언 순서로 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset,std::size_t width) {
    std::uint32_t value=0;
    // 필요한 바이트만 조합한다.
    for (std::size_t i=0;i<width;++i) value|=static_cast<std::uint32_t>(raw[offset+i])<<(8*i);
    return value;
}
// x87의 덧셈 뒤 CRT 절삭 결과를 유효한 공간 인덱스로 제한한다.
int Cell(float coordinate,float bias) {
    const double value=static_cast<double>(coordinate)+static_cast<double>(bias);
    if (!std::isfinite(value) || value<=-1 || value>=kSide) throw std::out_of_range("Damageable 해방 좌표 범위 오류");
    return static_cast<int>(value);
}
}
// 실제 조회 자료와 모든 하위 효과의 연결을 확인한다.
RawDamageableRelease::RawDamageableRelease(SidPool& pool,std::span<const RiftTypeRecord> types,std::span<const std::uint8_t> spots,
    const ContainedFinderState& finderState,const DamageableReleaseState& state,DamageableReleaseHooks hooks)
    :pool_(pool),types_(types),spots_(spots),finderState_(finderState),state_(state),hooks_(std::move(hooks)) {
    const auto count=pool.Edition()==OriginalEdition::Patch1078 ? 188U : 171U;
    if (types.size()!=count || spots.size()!=kCells || !hooks_.spawnPriest || !hooks_.create || !hooks_.setOwner || !hooks_.pop || !hooks_.notifySurface)
        throw std::invalid_argument("Damageable 해방 자료/하위 효과 누락");
}
// 각 후보에서 현재 표의 genus를 읽되 WORD를 바이트로 줄이지 않는다.
const RiftTypeRecord& RawDamageableRelease::Type(std::uint32_t number) const {
    if (number>=types_.size()) throw std::out_of_range("Damageable 해방 타입 범위 오류");
    return types_[number];
}
// raw float의 비트는 부모의 현재 슬롯에서 읽는다.
std::array<float,2> RawDamageableRelease::Position(Sid parent) const {
    const auto raw=pool_.Slot(parent);
    return {std::bit_cast<float>(Read(raw,kX,4)),std::bit_cast<float>(Read(raw,kY,4))};
}
// 실제 중심 함수는 발자국의 절반을 빼고 각 좌표를 float에 저장한다.
std::array<float,2> RawDamageableRelease::Center(Sid parent) const {
    const auto& type=Type(pool_.Slot(parent)[kType]);
    if (type.footX<1 || type.footY<1 || type.footX>kSide || type.footY>kSide) throw std::invalid_argument("Damageable 해방 발자국 범위 오류");
    const auto point=Position(parent);
    return {static_cast<float>(static_cast<double>(point[0])-static_cast<double>(type.footX)*0.5),
        static_cast<float>(static_cast<double>(point[1])-static_cast<double>(type.footY)*0.5)};
}
// 공간은 각 일반 자산 후보에서 다시 읽으며 좌표는 순회 전 스냅한 값을 유지한다.
std::uint8_t RawDamageableRelease::Spot(const std::array<float,2>& point) const {
    return spots_[static_cast<std::size_t>(Cell(point[1],kSpotBias))*kSide+static_cast<std::size_t>(Cell(point[0],kSpotBias))];
}
// 권한 해방 몸체의 조건·호출 순서·WORD 복사는 실제 코드로 수행한다.
void RawDamageableRelease::Release(Sid parent) const {
    const auto raw=pool_.AllocatedBytes(parent);
    if ((raw[kState]&9) || raw[kType]<kFirstAssetTypeNumber) throw std::invalid_argument("Damageable 해방 부모 상태 오류");
    auto point=Type(raw[kType]).footX==1 ? Position(parent) : Center(parent);
    // 스냅한 정수는 float로 저장되며 이후 일반 자산의 위치로 고정된다.
    for (auto& coordinate:point) coordinate=static_cast<float>(Cell(coordinate,kSnapBias));
    RawContainedFinder finder(pool_,finderState_);
    // 반환 전 저장된 next를 사용하므로 현재 후보의 연결 변경에도 원래 다음 후보로 간다.
    for (Sid current=finder.Begin(parent,true);current.value;current=finder.Next()) {
        if (!(pool_.Slot(current)[kMask]&kReleasable)) continue;
        const auto kind=Read(pool_.Slot(current),kKind,2);
        if (!state_.battle && !(Type(kind).flags2&state_.allowedGenus)) continue;
        if (Type(kind).flags2&kPriest) {
            auto position=Position(parent);
            if (Type(pool_.Slot(parent)[kType]).flags2&kCenteredParent) {
                position=Center(parent);position[0]+=0.5f;position[1]+=0.5f;
            }
            hooks_.spawnPriest(position[0],position[1],kind,static_cast<std::uint16_t>(Read(pool_.Slot(current),kWord,2)));
        } else {
            if ((Spot(point)&0x10) || !(Spot(point)&6)) continue;
            const Sid created=hooks_.create(kind,0);
            const auto word=Read(pool_.Slot(current),kWord,2);auto target=pool_.AllocatedBytes(created);
            target[kWord]=static_cast<std::uint8_t>(word);target[kWord+1]=static_cast<std::uint8_t>(word>>8);
            if (target[kType]==state_.bridgeType) hooks_.notifySurface(created);
            hooks_.setOwner(created,0);hooks_.pop(created,point[0],point[1],0);
        }
    }
}
// 연결 대상이 같은 풀을 사용하는지 확인한다.
const SidPool& RawDamageableRelease::Pool() const { return pool_; }
// 상위에서 이미 결정한 해방 진입을 실제 몸체로 전달한다.
DamageablePreDestroyHooks MakeDamageableReleaseHooks(const SidPool& pool,const RawDamageableRelease& release,DamageablePreDestroyHooks hooks) {
    if (&pool!=&release.Pool()) throw std::invalid_argument("Damageable 해방의 SID 풀이 다릅니다");
    hooks.releaseContained=[&release](Sid sid) { release.Release(sid); };return hooks;
}
}
