// 최초 예약과 현재 공간 조회를 복원하며 외부 효과 뒤의 raw 변화를 덮어쓰지 않는다.
#include "o/RawForcefieldLifecycle.h"
#include "o/RawPriestPostPop.h"
#include <bit>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 실제 PE의 두 주기 상수 비트를 단정도로 보존한다.
constexpr float kFrameInterval=std::bit_cast<float>(0x3da3d70aU),kCheckInterval=std::bit_cast<float>(0x3c23d70aU);
// 비정렬 raw DWORD를 호스트 포인터/정렬과 무관하게 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset) {
    std::uint32_t value=0;
    // 낮은 바이트부터 네 바이트를 조합한다.
    for (std::size_t i=0;i<4;++i) value|=static_cast<std::uint32_t>(raw[offset+i])<<(8*i);
    return value;
}
// 원본 float→int→float→int의 정상 범위에서 첫 절삭 값이 유지된다.
int Cell(float value) {
    if (!std::isfinite(value) || static_cast<double>(value)<std::numeric_limits<std::int32_t>::min() || static_cast<double>(value)>std::numeric_limits<std::int32_t>::max())
        throw std::out_of_range("보호막 사제 검사 좌표 범위 오류");
    return static_cast<int>(value);
}
// 실제 보호막 클래스만 명시 연결하며 잘못된 SID/free/contained는 효과 전에 거부한다.
void ValidateObject(const SidPool& pool,Sid sid) {
    const auto raw=pool.Slot(sid);
    if (sid.value<5 || (raw[11]&9) || raw[10]!=kForcefieldType || Read(raw,0)!=(pool.Edition()==OriginalEdition::Patch1078 ? kPatchForcefieldVtable : kCdForcefieldVtable))
        throw std::invalid_argument("보호막 타입/가상 표 불일치");
}
}
// 두 확보/생성 효과는 첫 Pop 전에 모두 연결되어야 한다.
RawForcefieldPostPop::RawForcefieldPostPop(const SidPool& pool,ForcefieldPostPopHooks hooks):pool_(pool),hooks_(std::move(hooks)) {
    if (!hooks_.reserveRegular || !hooks_.addRegular) throw std::invalid_argument("보호막 예약 효과 누락");
}
// 사전 검사는 현재 공간/효과를 바꾸지 않는다.
void RawForcefieldPostPop::Validate(Sid sid) const { ValidateObject(pool_,sid); }
// 첫 확보 실패 여부와 무관하게 두 번째 확보를 실행한다. 기존 사건은 검색하지 않는다.
void RawForcefieldPostPop::Prefix(Sid sid,std::uint32_t flags) const {
    Validate(sid);
    if (!(flags&1)) return;
    if (hooks_.reserveRegular()) hooks_.addRegular(sid,kForcefieldFrameEvent,kFrameInterval);
    if (hooks_.reserveRegular()) hooks_.addRegular(sid,kForcefieldCheckEvent,kCheckInterval);
}
// 일반 Pop의 풀 연결을 읽기 전용으로 확인한다.
const SidPool& RawForcefieldPostPop::Pool() const { return pool_; }
// 현재 타입 표를 참조하며 조회마다 독립 finder를 만든다.
RawForcefieldRegular::RawForcefieldRegular(const SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,
    const ForcefieldRegularState& state,ForcefieldRegularHooks hooks):pool_(pool),hash_(hash),types_(types),state_(state),hooks_(std::move(hooks)) {
    if (types.size()!=(pool.Edition()==OriginalEdition::Patch1078 ? 188U : 171U) || !hooks_.advance || !hooks_.destroy)
        throw std::invalid_argument("보호막 Regular 타입/효과 연결 오류");
}
// 후보의 소유자/HP/state는 별도 필터가 아니다. 일반 finder의 매장 제외/발자국 조건만 적용한다.
Sid RawForcefieldRegular::FindPriest(Sid sid) const {
    const auto raw=pool_.Slot(sid);const int x=Cell(std::bit_cast<float>(Read(raw,14))),y=Cell(std::bit_cast<float>(Read(raw,18)));
    RawSquidFinder finder(pool_,hash_,types_);
    // 현재 해시 체인의 순서를 유지하며 DWORD 타입을 BYTE로 줄이지 않는다.
    for (Sid current=finder.Begin({x,y,x,y});current.value;current=finder.Next())
        if (pool_.Slot(current)[10]==state_.priestType) return current;
    return {};
}
// 삭제 효과가 실행 중인 Regular도 지울 수 있으므로 이후 그 객체/슬롯을 다시 쓰지 않는다.
float RawForcefieldRegular::Handle(Sid sid,std::uint32_t event,std::uint32_t,float) const {
    ValidateObject(pool_,sid);
    if (event==kForcefieldFrameEvent) { hooks_.advance(sid,1,0);return kFrameInterval; }
    if (event==kForcefieldCheckEvent) {
        if (FindPriest(sid).value) return kCheckInterval;
        hooks_.destroy(sid,0);return -1.0f;
    }
    if (pool_.Edition()==OriginalEdition::Patch1078 && state_.debug) throw std::logic_error("보호막 Regular 미지원 사건 assert");
    return 0.0f;
}
// 실제 프레임 진행의 같은 월드 연결을 확인한다.
const SidPool& RawForcefieldRegular::Pool() const { return pool_; }
// 예약 성공 입력 이후에는 실제 타입 46/form/부모 체인/Kernel을 사용한다.
ForcefieldPostPopHooks MakeForcefieldPostPopProcessHooks(const SidPool& pool,SquidProcessHost& host,std::function<bool()> reserve) {
    if (&pool!=&host.Pool() || !reserve) throw std::invalid_argument("보호막/프로세스 풀 또는 확보 효과 오류");
    return {std::move(reserve),[&host](Sid sid,std::uint32_t event,float payload) { host.AddRegular(sid,event,payload); }};
}
// 진행이 사용하는 실제 프레임 지정/공간 효과를 그대로 재사용한다.
ForcefieldRegularHooks MakeForcefieldFrameHooks(const SidPool& pool,const RawFrameAdvance& advance,ForcefieldRegularHooks hooks) {
    if (&pool!=&advance.Pool()) throw std::invalid_argument("보호막/프레임 진행 풀 불일치");
    hooks.advance=[&advance](Sid sid,std::int32_t delta,std::uint32_t flags) { static_cast<void>(advance.Advance(sid,delta,flags)); };return hooks;
}
}
