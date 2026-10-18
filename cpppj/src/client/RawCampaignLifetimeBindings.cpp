// base84/사정거리104와 마나103의 입증된 VT 경로를 연결하고 다른 실제 파생/종속 삭제 분배는 보존한다.
#include "client/RawCampaignLifetimeBindings.h"
#include <stdexcept>
#include <utility>

namespace netstorm::client {
namespace {
constexpr std::uint32_t kBaseVtable=0x501dd0,kPlayerVtable=0x50ee08,kEmptyGeyser=84; // 실제PATCH생성자와PE의+10/+14/+18/+24를대조한경로다.
// 원본포인터기록DWORD를호스트실행주소로변환하지않고읽는다.
std::uint32_t LifetimeVtable(std::span<const std::uint8_t> raw) {
    std::uint32_t result=0;
    // 실제첫네BYTE만조합한다.
    for (std::size_t index=0;index<4;++index) result|=std::uint32_t(raw[index])<<(index*8);return result;
}
}
// 원본00463ff0의같은등록배열과대상자신의현재가상handle을사용한다.
o::PlayerContentNoticeHooks BindPlayerContentNoticeGumps(GumpCreateState& registry,GumpEventHooks events,o::PlayerContentNoticeHooks hooks) {
    if (registry.collection.edition!=o::OriginalEdition::Patch1078 || !events.handle) throw std::invalid_argument("Player 내용물의 같은 판본/실제 Gump handle 누락");
    hooks.gump=[&registry](std::uint32_t id) { if (id>=registry.entries.size()) throw std::out_of_range("Player 내용물 현재 Gump 등록ID 물리 범위");return registry.entries[id]; };
    hooks.handle=[handle=std::move(events.handle)](std::uint32_t token,std::uint32_t event,std::uint32_t form,std::uint32_t reason) { static_cast<void>(handle(token,event,form,reason)); };
    return hooks;
}
// 관찰카운터대신현재선택UI/실제소리를사용하며다른효과를조용히버리지않는다.
o::SquidDeletionHooks MakeCampaignDeletionEffects(o::SquidDestroyHooks effects,std::function<void(o::Sid)> unitLost) {
    if (!effects.emit || !unitLost) throw std::invalid_argument("공통 삭제의 실제 선택/소리/파생 효과 누락");
    o::SquidDeletionHooks hooks;hooks.forward=effects.emit;
    hooks.emit=[effects=std::move(effects),unitLost=std::move(unitLost)](const o::SquidDeletionEvent& event) {
        if (event.effect==o::SquidDeletionEffect::ClearSelection) { effects.emit({o::SquidDestroyEffect::ClearSelection,event.sid,{},0});return; }
        if (event.effect==o::SquidDeletionEffect::UnitLostSound) { unitLost(event.sid);return; }
        throw std::logic_error("공통 삭제 Refund는 같은 실제 SquidReward를 공급해야 합니다");
    };return hooks;
}
// 기존Form분배기의타입6만등록하며다른Host/감시/프로세스경로를덮어쓰지않는다.
RawCampaignLifetimeBindings::RawCampaignLifetimeBindings(RawSurfaceWorld& world,o::ContentFormHost& forms,const o::ContentLifetimeState& state,
    o::ContentLifetimeHooks effects,o::SquidReward& reward,const o::RawPlayerContentNotice& player,o::ContentFormNotice fallbackParent)
    :core_(world.Core()),player_(player),fallbackParent_(std::move(fallbackParent)),
     contents_(core_.pool,core_.types,forms,core_.destroy,state,std::move(effects),core_.forms.Hooks(),&reward) {
    if (core_.pool.Edition()!=o::OriginalEdition::Patch1078 || &player.Pool()!=&core_.pool) throw std::invalid_argument("캠페인 내용물/Player 통지는 같은 PATCH 풀이어야 합니다");
    core_.forms.Bind(contents_);
}
// baseVT가실제값일때만84의세공통가상몸체를사용한다.
bool RawCampaignLifetimeBindings::EmptyGeyser(o::Sid sid) const { const auto raw=core_.pool.Slot(sid);return raw[10]==kEmptyGeyser && LifetimeVtable(raw)==kBaseVtable; }
// 마나와 플레어의 +10/+14/+18/+24/+48은 base와 같다. range104는 실제 생성자 표의0으로 baseVT를 사용한다.
bool RawCampaignLifetimeBindings::CommonAsset(o::Sid sid) const { const auto raw=core_.pool.Slot(sid);return EmptyGeyser(sid) || (raw[10]==103 && LifetimeVtable(raw)==0x50c3b0) || (raw[10]==104 && LifetimeVtable(raw)==kBaseVtable) || (raw[10]==105 && LifetimeVtable(raw)==0x509810) || (raw[10]==106 && LifetimeVtable(raw)==0x5096a0) || (raw[10]==134 && LifetimeVtable(raw)==0x5070c0); }
// Pre/Post에만입증된공통모듈을선택하고종속/전송/선택은현재기존분배를유지한다.
o::SquidDestroyHooks RawCampaignLifetimeBindings::DestroyHooks(o::SquidDestroyHooks fallback) const {
    auto hooks=fallback;
    hooks.emit=[this,fallback=std::move(fallback)](const o::SquidDestroyEvent& event) {
        if (CommonAsset(event.sid) && (event.effect==o::SquidDestroyEffect::PreDestroy || event.effect==o::SquidDestroyEffect::PostDestroy)) {
            core_.surfaces.Hooks().emit(event);return;
        }
        if (!fallback.emit) throw std::logic_error("다른 캠페인 실제 삭제 효과 미연결");fallback.emit(event);
    };return hooks;
}
// 같은RawSquidDestroy가dead/종속/전파/Unpop/PrePost깊이/반납을실제로처리한다.
o::SquidAssetDestroy RawCampaignLifetimeBindings::Destroy(o::SquidAssetDestroy fallback) const {
    return [this,fallback=std::move(fallback)](o::Sid sid,std::uint32_t flags,const o::SquidDestroyHooks& hooks) {
        if (CommonAsset(sid)) return core_.virtualDestroy.Destroy(sid,flags,hooks);
        if (!fallback) throw std::logic_error("다른 캠페인 실제 파생 삭제 미연결");return fallback(sid,flags,hooks);
    };
}
// PlayerVT가아닌실제부모를Player의통지로잘못확장하지않는다.
void RawCampaignLifetimeBindings::ParentNotice(o::Sid parent,o::Sid form,std::uint32_t reason) const {
    if (LifetimeVtable(core_.pool.Slot(parent))==kPlayerVtable) { player_.Notify(parent,form.value,reason);return; }
    if (!fallbackParent_) throw std::logic_error("다른 캠페인 내용물 실제 부모 통지 미연결");fallbackParent_(parent,form,reason);
}
// 상위실제세션의동일성검증과수명조립을돕는다.
const o::ContentFormLifetime& RawCampaignLifetimeBindings::Contents() const { return contents_; }
const o::SidPool& RawCampaignLifetimeBindings::Pool() const { return core_.pool; }
}
