// 실제 Player wrapper의 현재 전역 재조회와 파생Post의 직접 기록/예약 순서를 보존한다.
#include "o/RawPlayerWorld.h"
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
constexpr std::uint32_t kPatchPlayerVtable=0x50ee08,kCdPlayerVtable=0x5002a8; // 실제 Player152 생성자가 쓰는 두 PE 가상표다.
// 정렬되지 않은 원본 little endian 필드의 정확한 폭을 쓰고 주변 바이트를 보존한다.
void PlayerWrite(std::span<std::uint8_t> raw,std::size_t field,std::uint32_t value,std::size_t width=4) {
    // 원본이 지정한 낮은 BYTE부터 필드 폭만큼 저장한다.
    for (std::size_t index=0;index<width;++index) raw[field+index]=static_cast<std::uint8_t>(value>>(8*index));
}
// 실제 DWORD를 BYTE 저장소에서 직접 읽으며 호스트 정렬/엔디언을 요구하지 않는다.
std::uint32_t PlayerRead(std::span<const std::uint8_t> raw,std::size_t field) {
    std::uint32_t value=0;
    // 실제 네 BYTE를 낮은 비트부터 조립한다.
    for (std::size_t index=0;index<4;++index) value|=static_cast<std::uint32_t>(raw[field+index])<<(8*index);
    return value;
}
}
// 동일 월드와 모든 필수 하위 경계를 확인해 실제9기록을 참조한다.
RawPlayerWorld::RawPlayerWorld(SidPool& pool,SquidPostPop& base,std::span<std::uint8_t> playerRecords,
    PlayerWorldState& state,PlayerWorldHooks hooks)
    :pool_(pool),base_(base),players_(playerRecords),state_(state),hooks_(std::move(hooks)) {
    const auto stride=pool.Edition()==OriginalEdition::Patch1078 ? 0xb4U : 0xacU; // 현재 PE의 실제 Player 한 기록 크기다.
    if (&base.Pool()!=&pool || players_.size()!=9U*stride || !hooks_.assertion || !hooks_.find || !hooks_.create ||
        !hooks_.setOwner || !hooks_.pop || !hooks_.reserveRegular || !hooks_.addRegular)
        throw std::invalid_argument("Player 실제 동일Pool/9기록/필수효과 누락");
}
// 원본 wrapper에는 owner 범위 진단이 없으므로 안전한 현재9기록 범위 밖은 명시 계약 오류다.
std::size_t RawPlayerWorld::TechnologyOffset(std::uint32_t owner) const {
    if (owner>=9) throw std::out_of_range("Player Technology owner의 실제9기록 범위 오류");
    const bool patch=pool_.Edition()==OriginalEdition::Patch1078;return owner*(patch ? 0xb4U : 0xacU)+(patch ? 0x9cU : 0x94U);
}
// 실제 Create(type,2) 뒤 가상Owner/첫 공간Pop을 순서대로 연결한다.
Sid RawPlayerWorld::Create(std::int32_t owner) const {
    if (!state_.playerKnown) hooks_.assertion(PlayerWorldAssertion::PlayerIdKnown);
    const auto sum=std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(owner)+10U); // 원본 signed32 좌표의 감김 덧셈을 보존한다.
    const float x=static_cast<float>(sum),y=state_.special ? 10.0f : 1.0f;
    if (hooks_.find(x,y,state_.playerType).value) hooks_.assertion(PlayerWorldAssertion::PlayerAlreadyExists);
    const auto sid=hooks_.create(state_.playerType,2);hooks_.setOwner(sid,static_cast<std::uint32_t>(owner));
    PlayerWrite(pool_.AllocatedBytes(sid),pool_.Edition()==OriginalEdition::Patch1078 ? 36U : 34U,0,
        pool_.Edition()==OriginalEdition::Patch1078 ? 4U : 1U);
    hooks_.pop(sid,x,y,0);return sid;
}
// 실제 생성자의 VT만 분배 대상으로 인정한다.
bool RawPlayerWorld::Handles(Sid sid) const {
    const auto raw=pool_.AllocatedBytes(sid);return PlayerRead(raw,0)==(pool_.Edition()==OriginalEdition::Patch1078 ? kPatchPlayerVtable : kCdPlayerVtable);
}
// 현재 owner의 실제기록과 비가상 공통 장부를 공간 효과 전에 확인한다.
void RawPlayerWorld::Validate(Sid sid,std::uint32_t flags,const RawGraphPop* pop) const {
    if (!Handles(sid)) throw std::logic_error("미복원 Player 가상PostPop 표");
    const auto raw=pool_.AllocatedBytes(sid);TechnologyOffset(raw[pool_.Edition()==OriginalEdition::Patch1078 ? 34U : 32U]);base_.ValidateBase(sid,flags,pop);
}
// 현재 원본 owner/SID를 기록한 뒤 실제 new 성공 조건과 마지막 공통Post를 유지한다.
void RawPlayerWorld::PostPop(Sid sid,std::uint32_t flags) const {
    Validate(sid,flags);const bool patch=pool_.Edition()==OriginalEdition::Patch1078;auto raw=pool_.AllocatedBytes(sid);
    PlayerWrite(players_,TechnologyOffset(raw[patch ? 34U : 32U]),sid.value);
    if ((flags&1U) && raw[patch ? 34U : 32U]==state_.battleOwner && !state_.fortMode) {
        if (hooks_.reserveRegular()) hooks_.addRegular(sid,1,5.0f);
    }
    if ((flags&1U) && state_.special) PlayerWrite(pool_.AllocatedBytes(sid),patch ? 36U : 34U,1,patch ? 4U : 1U);
    base_.PostPopBase(sid,flags);
}
// 아직 Player 번호가 알려지지 않았으면 원본처럼 실제기록을 조회하지 않고0을 반환한다.
std::uint32_t RawPlayerWorld::CurrentTechnology() const {
    return state_.playerKnown ? PlayerRead(players_,TechnologyOffset(state_.battleOwner)) : 0U;
}
// 명시 owner 조회는 원본 PlayerKnown 진단 후 같은9기록 DWORD를 읽는다.
std::uint32_t RawPlayerWorld::Technology(std::int32_t owner) const {
    if (!state_.playerKnown) hooks_.assertion(PlayerWorldAssertion::PlayerIdKnown);
    return PlayerRead(players_,TechnologyOffset(static_cast<std::uint32_t>(owner)));
}
// 같은 Pool 분배/Host 연결을 검증할 때 참조를 제공한다.
const SidPool& RawPlayerWorld::Pool() const { return pool_; }
}
