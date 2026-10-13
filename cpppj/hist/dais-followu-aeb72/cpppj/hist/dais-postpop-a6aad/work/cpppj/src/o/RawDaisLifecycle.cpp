// 원본생성자+20/+14/+18의실제몸체를같은공통후처리및현재Player상태에연결한다.
#include "o/RawDaisLifecycle.h"
#include "o/SquidFactory.h"
#include "o/SquidOwner.h"
#include <bit>
#include <cmath>
#include <stdexcept>

namespace netstorm::o {
namespace {
constexpr std::array<std::int32_t,4> kNextFirst{{3,64,120,0}},kStageFirst{{0,3,64,120}}; // 원본00532624/CD0052e2b8 및 count표00532620/CD0052e2b4다.
// 비정렬 현재raw필드를지정폭으로만읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset,std::size_t width=4) {
    std::uint32_t value=0;
    // 인접필드를읽지않고낮은바이트부터합친다.
    for (std::size_t index=0;index<width;++index) value|=static_cast<std::uint32_t>(raw[offset+index])<<(index*8);
    return value;
}
// 정상지도좌표를원본CRT의0방향절삭으로읽는다.
int Cell(float value) { if (!std::isfinite(value) || static_cast<double>(value)<-2147483648.0 || static_cast<double>(value)>=2147483648.0) throw std::out_of_range("다이스 수명 좌표절삭범위");return static_cast<int>(value); }
}
// 필수몸체와상태를서로다른월드에연결하면아직효과가없을때거부한다.
RawDaisLifecycle::RawDaisLifecycle(SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,
    const RawSquidNeighbors& footprint,const RawTempleRitual& temple,const RawAltarSacrifice& altar,
    const RawCarrierPostPop& postPop,const RawDamageablePreDestroy& preDestroy,
    DaisRegularState& daisState,AltarSacrificeState& altarState,DaisLifecycleState& state,DaisLifecycleHooks hooks)
    :pool_(pool),hash_(hash),types_(types),footprint_(footprint),temple_(temple),altar_(altar),postPop_(postPop),preDestroy_(preDestroy),daisState_(daisState),altarState_(altarState),state_(state),hooks_(std::move(hooks)) {
    if (&pool!=&footprint.Pool() || &pool!=&temple.Pool() || &pool!=&altar.Pool() || &pool!=&postPop.Pool() || &pool!=&preDestroy.Pool() || types.size()!=(pool.Edition()==OriginalEdition::Patch1078 ? 188U : 171U)
        || !hooks_.setFrame || !hooks_.unpop || !hooks_.repop || !hooks_.create || !hooks_.setOwner || !hooks_.pop || !hooks_.reserveRegular || !hooks_.addRegular || !hooks_.surface || !hooks_.hasIsland || !hooks_.islandNumber || !hooks_.option || !hooks_.chat || !hooks_.destroy || !hooks_.damageablePostDestroy)
        throw std::invalid_argument("다이스 수명의 실제공통몸체/필수효과/같은풀 연결누락");
}
// 효과뒤항상현재ownerBYTE를사용한다.
std::uint32_t RawDaisLifecycle::Owner(Sid sid) const { return pool_.Slot(sid)[pool_.Edition()==OriginalEdition::Patch1078 ? 34 : 32]; }
// 원본의중립0도등록표범위에포함한다.
void RawDaisLifecycle::Player(std::uint32_t owner) const { if (owner>kPlayerCount) throw std::out_of_range("다이스 수명 현재Player범위"); }
// 원본최종VT에없는파생수명은허용하지않는다.
bool RawDaisLifecycle::Handles(Sid sid) const { const auto raw=pool_.Slot(sid);const bool patch=pool_.Edition()==OriginalEdition::Patch1078;const auto vt=Read(raw,0);return (raw[10]==daisState_.rune.daisType && vt==(patch ? kPatchDaisVtable : kCdDaisVtable)) || (raw[10]==daisState_.rune.altarType && vt==(patch ? kPatchAltarVtable : kCdAltarVtable)); }
// 로딩초기화에서는setter표시를추가하지않고정확한원본폭만쓴다.
void RawDaisLifecycle::WriteFrame(Sid sid,std::int32_t frame) const {
    auto raw=pool_.AllocatedBytes(sid);const bool patch=pool_.Edition()==OriginalEdition::Patch1078;const auto bits=std::bit_cast<std::uint32_t>(frame);const std::size_t offset=patch ? 36 : 34,width=patch ? 4 : 1;
    // 정확한프레임필드의바이트만쓴다.
    for (std::size_t index=0;index<width;++index) raw[offset+index]=static_cast<std::uint8_t>(bits>>(index*8));
}
// player개수갱신뒤의현재owner와로딩상태가최초생성/경고조건을결정한다.
void RawDaisLifecycle::DaisPostPop(Sid sid,std::uint32_t flags) const {
    if (!daisState_.rune.suppressDuration) { Player(Owner(sid));if (state_.gameTime==0 && (flags&7)) { const auto saved=daisState_.grantFlags[Owner(sid)];if (saved>=kNextFirst.size()) throw std::out_of_range("다이스 로딩개수첫번호범위");hooks_.setFrame(sid,kNextFirst[saved],0); }Player(Owner(sid));daisState_.grantFlags[Owner(sid)]=static_cast<std::uint8_t>(temple_.Count(sid)-1); }
    if ((flags&2) && Owner(sid)) { Player(Owner(sid));auto& player=state_.players[Owner(sid)];player.second=player.secondMirror=sid; }
    if ((flags&1) && Owner(sid)) { Player(Owner(sid));auto& player=state_.players[Owner(sid)];player.first=player.firstMirror=sid; }
    if (flags&1) {
        if (daisState_.ritual.boss) {
            if (state_.loading.loadingDepth && temple_.Stage(sid)!=0) { hooks_.unpop(sid,0);const auto count=temple_.Count(sid);if (count>=kStageFirst.size()) throw std::out_of_range("다이스 현재단계첫번호범위");WriteFrame(sid,kStageFirst[count]);hooks_.repop(sid,0); }
            const auto created=hooks_.create(daisState_.rune.altarType,0);hooks_.setOwner(created,Owner(sid));const auto raw=pool_.Slot(sid);hooks_.pop(created,std::bit_cast<float>(Read(raw,14)),std::bit_cast<float>(Read(raw,18)),0);
        }
        if (Owner(sid)==daisState_.localOwner && !state_.loading.loadingDepth) { const auto raw=pool_.Slot(sid);const auto surface=hooks_.surface(std::bit_cast<float>(Read(raw,14)),std::bit_cast<float>(Read(raw,18)));if (surface.value && surface.value<pool_.Capacity() && hooks_.hasIsland(surface)) { Player(Owner(sid));if (hooks_.islandNumber(surface)!=state_.players[Owner(sid)].homeIsland && !hooks_.option("noShowWarnCantUpgrade") && !daisState_.rune.suppressDuration) hooks_.chat("WarnCantUpgradeAltarOffHomeIsland"); } }
    }
    postPop_.DamageablePostPop(sid,flags);
}
// 알터의21은로컬전용이며최초예약뒤항상공통Damageable후처리를실행한다.
void RawDaisLifecycle::PostPop(Sid sid,std::uint32_t flags) const {
    if (!Handles(sid)) throw std::invalid_argument("다이스 수명 Pop 타입/가상표오류");if (pool_.Slot(sid)[10]==daisState_.rune.daisType) { DaisPostPop(sid,flags);return; }if ((flags&1) && hooks_.reserveRegular()) hooks_.addRegular(sid,21,altarState_.struggleSeconds);postPop_.DamageablePostPop(sid,flags);
}
// 알터삭제가같은살아있는다이스삭제를요청해도공통Destroy의dead재진입차단을재사용한다.
void RawDaisLifecycle::AltarPreDestroy(Sid altar,std::uint32_t flags) const {
    if (!Handles(altar) || pool_.Slot(altar)[10]!=daisState_.rune.altarType) throw std::invalid_argument("알터 파생전처리 타입/가상표오류");if (!(flags&0x400000)) { const auto dais=altar_.DaisAt(altar);if (dais.value && dais.value<pool_.Capacity() && !(pool_.Slot(dais)[11]&2)) hooks_.destroy(dais,flags); }
    const auto area=footprint_.Footprint(altar);RawSquidFinder finder(pool_,hash_,types_);
    // 다음링크저장/현재extra/서버또는client번호권한의원본순서다.
    for (auto current=finder.Begin({Cell(area.left),Cell(area.top),Cell(area.right),Cell(area.bottom)},0);current.value;current=finder.Next()) if (pool_.Slot(current)[10]==daisState_.ritual.runeType && (pool_.Slot(current)[pool_.Edition()==OriginalEdition::Patch1078 ? 40 : 35]&1) && (pool_.IsServer() || (current.value>4 && current.value<pool_.Layout().serverFirst))) hooks_.destroy(current,0);
    preDestroy_.PreDestroy(altar,flags);
}
// 현재owner와정확히일치하는SID쌍만지운다. 마지막전체Damageable의flags는그대로전달한다.
void RawDaisLifecycle::PostDestroy(Sid sid,std::uint32_t flags) const {
    if (!Handles(sid)) throw std::invalid_argument("다이스 수명 삭제후처리 타입/가상표오류");if (pool_.Slot(sid)[10]==daisState_.rune.daisType && Owner(sid)) { Player(Owner(sid));auto& player=state_.players[Owner(sid)];if (player.second==sid) player.second=player.secondMirror={};if (player.first==sid) player.first=player.firstMirror={}; }hooks_.damageablePostDestroy(sid,flags);
}
// 후속가상등록의같은풀검증이다.
const SidPool& RawDaisLifecycle::Pool() const { return pool_; }
// 실제생성자/owner/frame/로컬Regular몸체를재사용한다.
DaisLifecycleHooks MakeDaisLifecycleHooks(const SidPool& pool,const SquidFrame& frame,SquidProcessHost& host,SquidFactory& factory,SquidOwner& owner,DaisLifecycleHooks hooks) {
    if (&pool!=&frame.Pool() || &pool!=&host.Pool() || &pool!=&factory.Pool() || &pool!=&owner.Pool()) throw std::invalid_argument("다이스 수명 실제Frame/Host/Factory 풀불일치");hooks.setFrame=[&frame](Sid sid,std::int32_t value,std::uint32_t flags) { frame.Set(sid,value,flags); };hooks.create=[&factory](std::uint32_t type,std::uint32_t flags) { return factory.Create(type,flags); };hooks.setOwner=[&owner](Sid sid,std::uint32_t value) { owner.Set(sid,value); };hooks.addRegular=[&host](Sid sid,std::uint32_t event,float payload) { host.AddRegular(sid,event,payload); };return hooks;
}
}
