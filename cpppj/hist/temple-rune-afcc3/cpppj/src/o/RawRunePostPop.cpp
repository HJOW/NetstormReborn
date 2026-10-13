// Rune후처리의예약후현재조회와항상실행하는공통호출을원본순서대로유지한다.
#include "o/RawRunePostPop.h"
#include "o/RawConstructionProcess.h"
#include "o/SquidProcess.h"
#include <bit>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
constexpr float kRunePayload=0.01F,kFinderBias=0.9999899864196777F; //304예약payload와좌표almostceil의원본float다.
constexpr std::uint32_t kRuneEvent=0x304; //두PE의최초Rune Regular사건번호다.
// 정렬되지않은littleendian필드를지정폭만읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset,std::size_t width=4) {
    std::uint32_t value=0;
    // 낮은바이트부터조합해ownerWORD와vtableDWORD를구별한다.
    for (std::size_t i=0;i<width;++i) value|=static_cast<std::uint32_t>(raw[offset+i])<<(8*i);
    return value;
}
// 유한정상지도좌표를원본CRT처럼0방향으로절삭한다.
int Cell(double value) {
    if (!std::isfinite(value) || value<-2147483648.0 || value>=2147483648.0) throw std::out_of_range("Rune 후처리 절삭 좌표 범위");return static_cast<int>(value);
}
}
// 모든실제하위연결을효과전에검사한다.
RawRunePostPop::RawRunePostPop(SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,RunePostPopState& state,SquidPostPop& base,RunePostPopHooks hooks)
    :pool_(pool),hash_(hash),types_(types),state_(state),base_(base),hooks_(std::move(hooks)) {
    if (&pool!=&base.Pool() || types.size()!=(pool.Edition()==OriginalEdition::Patch1078 ? 188U : 171U) || !hooks_.reserveRegular || !hooks_.addRegular || !hooks_.startConstruction) throw std::invalid_argument("Rune 후처리 실제몸체/판본 연결 누락");
}
// Rune생성자의실제가상표와현재타입번호둘다필요하다.
bool RawRunePostPop::Handles(Sid sid) const {
    const auto raw=pool_.Slot(sid);return raw[10]==state_.runeType && Read(raw,0)==(pool_.Edition()==OriginalEdition::Patch1078 ? kPatchRuneVtable : kCdRuneVtable);
}
// 공통장부의원래예외/Graph조건도공간쓰기전에그대로검사한다.
void RawRunePostPop::Validate(Sid sid,std::uint32_t flags,const RawGraphPop* projected) const {
    if (!Handles(sid)) throw std::invalid_argument("Rune 후처리 타입/가상표 오류");base_.ValidateBase(sid,flags,projected);
}
// 현재raw좌표를효과뒤에도각조회시다시읽는다.
std::array<float,2> RawRunePostPop::Position(Sid sid) const { const auto raw=pool_.Slot(sid);return {std::bit_cast<float>(Read(raw,14)),std::bit_cast<float>(Read(raw,18))}; }
// owner는원래ownerWORD가아닌patch+34/CD+32다.
std::uint32_t RawRunePostPop::Owner(Sid sid) const { return pool_.Slot(sid)[pool_.Edition()==OriginalEdition::Patch1078 ? 34 : 32]; }
// 반경1의기존일반finder를같은해시의현재후보에적용한다.
Sid RawRunePostPop::FindOwnedDais(Sid rune) const {
    const auto point=Position(rune);const auto x=Cell(static_cast<double>(point[0])+kFinderBias),y=Cell(static_cast<double>(point[1])+kFinderBias);RawSquidFinder finder(pool_,hash_,types_);
    // free/dead/abstract는추가필터가아니며genericfinder가buried를제외한다.
    for (auto sid=finder.Begin({x-1,y-1,x+1,y+1});sid.value;sid=finder.Next()) if (pool_.Slot(sid)[10]==state_.daisType && Owner(sid)==Owner(rune)) return sid;
    return {};
}
// patch의char7/10/25와CD의int7/10/25를동일정수로반환한다.
std::int32_t RawRunePostPop::ConstructionSeconds(Sid rune) const {
    const auto type=state_.altarType;if (type>=types_.size()) throw std::out_of_range("Rune 현재altar 타입범위");const auto point=Position(rune);const auto x=Cell(point[0]),y=Cell(point[1]);RawSquidFinder finder(pool_,hash_,types_);Sid altar;
    // 현재타입genus&6에따른시작단계와원본첫타입일치를재사용한다.
    for (auto sid=finder.Begin({x,y,x,y},(types_[type].flags2&6)==0 ? 1U : 0U);sid.value;sid=finder.Next()) if (pool_.Slot(sid)[10]==type) { altar=sid;break; }
    if (!altar.value) return 10;const auto originalOwner=Read(pool_.Slot(altar),12,2);if (!originalOwner || state_.suppressDuration) return 10;
    if (originalOwner>=state_.playerFlags.size()) throw std::out_of_range("Rune altar 원래ownerWORD/player범위");return (state_.playerFlags[originalOwner]&1) ? 25 : 7;
}
// 예약확보가실패해도신전/건설/공통후처리를계속한다.
void RawRunePostPop::PostPop(Sid rune,std::uint32_t flags) const {
    Validate(rune,flags);if ((flags&1) && hooks_.reserveRegular()) hooks_.addRegular(rune,kRuneEvent,kRunePayload);
    if (flags&2) { const auto dais=FindOwnedDais(rune);if (dais.value && dais.value<pool_.Capacity()) hooks_.startConstruction(rune,static_cast<float>(ConstructionSeconds(rune))); }
    base_.PostPopBase(rune,flags);
}
// 등록/어댑터에동일한실제풀을제공한다.
const SidPool& RawRunePostPop::Pool() const { return pool_; }
// 기존localRegular와실제건설프로세스를직접재사용한다.
RunePostPopHooks MakeRunePostPopHooks(const SidPool& pool,SquidProcessHost& host,const RawConstructionProcess& construction,RunePostPopHooks hooks) {
    if (&pool!=&host.Pool() || &pool!=&construction.Pool()) throw std::invalid_argument("Rune Host/Construction SID 풀 불일치");
    hooks.addRegular=[&host](Sid sid,std::uint32_t event,float payload) { host.AddRegular(sid,event,payload); };
    hooks.startConstruction=[&construction](Sid sid,float seconds) { construction.Start(sid,seconds); };return hooks;
}
}
