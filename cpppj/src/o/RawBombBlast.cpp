// 원본 폭발의 분기 순서·캡처 중심·현재 가상 표/권한을 보존한다.
#include "o/RawBombBlast.h"
#include <bit>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
constexpr std::uint32_t kBlastTable=0x502370,kSurface=4,kDais=0x400000,kHasHp=0x10,kAllowedGenus=0x80074200; // 실제 가상 표와 자격에서 읽는 원본 비트다.
constexpr std::uint32_t kProtected=0x40,kSurfaceDestroy=0x100048,kGrowingDestroy=0x8100008; // 보호 프레임과 두 삭제 분기의 실제 flags다.
constexpr std::int32_t kDamage=1000; // 원본 가상98에 전달하는 고정 피해량이다.
// 선택한 큰 하위의 누락을 무동작 성공으로 바꾸지 않는다.
template<class F> const F& Need(const F& hook) {
    if (!hook) throw std::logic_error("Bomb 폭발 실제 하위 미연결");
    return hook;
}
}
// 다른 판본/풀의 좌표 계산과 프레임을 섞지 않는다.
RawBombBlast::RawBombBlast(const SidPool& pool,std::span<const RiftTypeRecord> types,std::span<const RiftTypeFrames> frames,const RawBombTargets& coordinates,BombBlastState& state,BombBlastHooks hooks)
    :pool_(pool),types_(types),frames_(frames),coordinates_(coordinates),state_(state),hooks_(std::move(hooks)) {
    if (pool.Edition()!=OriginalEdition::Patch1078 || types.size()!=188 || frames.size()!=188 || &coordinates.Pool()!=&pool) throw std::invalid_argument("Bomb 폭발 판본/TYPE/Pool 오류");
}
// 실제 비정렬 원문 필드의 현재 값만 읽는다.
std::uint32_t RawBombBlast::Read(Sid sid,std::size_t offset,std::size_t width) const {
    const auto raw=pool_.Slot(sid);std::uint32_t value{};
    // 요청한 원본 폭을 little endian으로 조합한다.
    for (std::size_t i=0;i<width;++i) value|=std::uint32_t(raw[offset+i])<<(8*i);
    return value;
}
// 효과 뒤 바뀐 현재 TYPE를 다시 읽는다.
const RiftTypeRecord& RawBombBlast::Type(Sid sid) const {
    const auto kind=Read(sid,10,1);if (kind>=types_.size()) throw std::out_of_range("Bomb 폭발 현재TYPE 범위");return types_[kind];
}
// 실제 함수는 현재 HP26을 읽지 않고 HP를 보유하는 형태인지 판단한다.
bool RawBombBlast::HasHitPoints(Sid sid) const {
    const auto& type=Type(sid);return (type.flags2&kDais) ? (Read(sid,40,1)&1U)!=0 : (type.flags1&kHasHp)!=0;
}
// 비표면 피해 효과의 위치도 호출 전에 이 중심으로 캡처한다.
std::array<float,2> RawBombBlast::TargetCenter(Sid target) const {
    const auto& type=Type(target);
    return {static_cast<float>(static_cast<double>(std::bit_cast<float>(Read(target,14)))-type.footX*0.5),static_cast<float>(static_cast<double>(std::bit_cast<float>(Read(target,18)))-type.footY*0.5)};
}
// 유효 owner0..8만 실제 동맹 배열을 조회한다.
bool RawBombBlast::Allied(std::uint32_t first,std::uint32_t second) const {
    if (first>=9 || second>=9) throw std::out_of_range("Bomb 폭발 동맹 owner 범위");return state_.alliances[first*9+second]!=0;
}
// 실제 주소로만 이 계열을 분배하며 타입 이름으로 넓히지 않는다.
bool RawBombBlast::Handles(Sid parent) const { return Read(parent,0)==kBlastTable; }
// protected 표면은 제외하고 비표면은 특별 대상 또는 HP 보유/허용 genus를 요구한다.
bool RawBombBlast::MayTarget(Sid target) const {
    const auto genus=Type(target).flags2;
    if (genus&kSurface) {
        if (!target.value) return true; // 422560의0번 조기 반환이다.
        const auto kind=Read(target,10,1);const auto frame=std::bit_cast<std::int32_t>(Read(target,36));const auto codes=frames_[kind].Codes();
        if (frame<0 || static_cast<std::size_t>(frame)>=codes.size()) throw std::out_of_range("Bomb 보호 프레임 범위");
        return !(codes[static_cast<std::size_t>(frame)].flags&kProtected);
    }
    if (Read(target,10,1)==state_.growingRainBlockerType) return true;
    return HasHitPoints(target) && (genus&kAllowedGenus)!=0;
}
// 자격 호출과 독립된 실제 효과 몸체다. 추가 free/dead/HP값 관문을 넣지 않는다.
void RawBombBlast::Apply(Sid parent,Sid target) const {
    if (target.value==Read(parent,12,2)) return;
    const auto genus=Type(target).flags2;
    if (genus&kSurface) {
        const auto center=coordinates_.Center(parent),victimCenter=TargetCenter(target);
        const float halfWidth=static_cast<float>(static_cast<double>(Type(target).footX)*0.5); // 가상b4 이전에 현재 발자국을float로 저장한다.
        const auto radius=Need(hooks_.radius)(parent,Read(parent,0));
        const float sum=static_cast<float>(static_cast<double>(radius)+halfWidth);
        const double dx=static_cast<double>(center[0])-victimCenter[0],dy=static_cast<double>(center[1])-victimCenter[1];
        const double distance=std::sqrt(dx*dx+dy*dy),limit=static_cast<double>(sum)-1.0;
        if (!(limit<distance)) Need(hooks_.destroy)(target,Read(target,0),kSurfaceDestroy); // 원본 unordered 분기도 안쪽 삭제로 들어간다.
        else if (state_.boss) Need(hooks_.edge)(target,center[0],center[1]);
        return;
    }
    const auto center=TargetCenter(target);const auto kind=Read(target,10,1);
    if (kind==state_.priestType && Allied(Read(parent,34,1),Read(target,34,1))) return;
    if (kind==state_.growingRainBlockerType) {
        if (state_.boss) Need(hooks_.destroy)(target,Read(target,0),kGrowingDestroy);
    } else Need(hooks_.damage)(target,Read(target,0),kDamage,static_cast<std::uint8_t>(Read(parent,34,1)),parent);
    Need(hooks_.ground)(center[0],center[1]);
}
// 같은 실제 월드의 저장소인지 확인한다.
const SidPool& RawBombBlast::Pool() const { return pool_; }
}
