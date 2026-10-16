// 객체 메뉴의 원본 분기/서식/현재 전역 재조회 순서를 구현한다.
#include "client/RawObjectMenu.h"
#include <bit>
#include <stdexcept>

namespace netstorm::client {
namespace {
// 미연결 실제 효과를 빈 메뉴나 성공으로 바꾸지 않는다.
template<class Function> const Function& Required(const Function& function) {
    if (!function) throw std::logic_error("실제 객체 메뉴 하위 미연결");return function;
}
}
// 판본별 필드 조회에 필요한 실제 TYPE 표의 크기를 확인한다.
RawObjectMenu::RawObjectMenu(const o::SidPool& pool,std::span<const o::RiftTypeRecord> types,ObjectMenuState& state,ObjectMenuHooks hooks)
    :pool_(pool),types_(types),state_(state),hooks_(std::move(hooks)) {
    if (types.size()!=(pool.Edition()==o::OriginalEdition::Patch1078 ? 188U : 171U)) throw std::invalid_argument("객체 메뉴 실제 TYPE 표 크기 오류");
}
// 원본 물리 SID를 조회한다. free/dead를 임의로 생략하지 않는다.
std::span<const std::uint8_t> RawObjectMenu::Raw(o::Sid sid) const {
    if (!sid.value || sid.value>=pool_.Capacity()) throw std::out_of_range("객체 메뉴 SID 범위 오류");return pool_.Slot(sid);
}
// 호출 시점의 TYPE 인자를 읽는다.
const o::RiftTypeRecord& RawObjectMenu::Type(std::uint32_t type) const {
    if (type>=types_.size()) throw std::out_of_range("객체 메뉴 TYPE 범위 오류");return types_[type];
}
// PATCH+22/CD+20의 BYTE를 현재 슬롯에서 읽는다.
std::uint32_t RawObjectMenu::Owner(o::Sid sid) const { return Raw(sid)[pool_.Edition()==o::OriginalEdition::Patch1078 ? 34 : 32]; }
// 번역된 고정 서식만 허용하고 호스트 스택 버퍼 초과는 예외로 거부한다.
std::string RawObjectMenu::Format(std::string_view key,std::initializer_list<PriestMenuArgument> arguments) const {
    auto text=Required(hooks_.format)(key,{arguments.begin(),arguments.size()});const auto nul=text.find('\0');if (nul!=std::string::npos) text.resize(nul);
    if (text.size()>=256) throw std::length_error("객체 메뉴 원본 서식 버퍼 초과");return text;
}
// Player 하위 메뉴의 번역 시점은 PATCH가 생성 전이고 CD는 생성 후다.
void RawObjectMenu::Open(o::Sid sid) const {
    Raw(sid);
    if (state_.editor) Required(hooks_.item)("Destroy",state_.destroyCommand,std::uint32_t(sid.value));
    else if (state_.special && state_.chargeBlocked) {
        if (Required(hooks_.hasHitPoints)(sid)) Required(hooks_.item)("CHEAT Hurt Me!",state_.hurtCommand,std::uint32_t(sid.value));
        Required(hooks_.item)("CHEAT Destroy",state_.destroyCommand,std::uint32_t(sid.value));
        Required(hooks_.item)("CHEAT Make Yours",state_.makeYoursCommand,std::uint32_t(sid.value));
    }
    if (!state_.helpWord) throw std::logic_error("원본 객체 메뉴 helpWord NULL");
    Required(hooks_.item)(*state_.helpWord,state_.helpCommand,std::uint32_t(Raw(sid)[10]));
    if (state_.editor || state_.special) {
        const auto label=Format("CHEAT Set Owner");Required(hooks_.item)(label,state_.tellCommand,std::string("SetOwner"));
    }
    if (Owner(sid)) {
        const auto subscriber=Required(hooks_.ownerSubscriber)(Owner(sid));
        if (pool_.Edition()==o::OriginalEdition::Patch1078) {
            const auto label=Format("Player");const auto menu=Required(hooks_.playerMenu)(subscriber,true,false);Required(hooks_.submenu)(label,menu);
        } else {
            const auto menu=Required(hooks_.playerMenu)(subscriber,true,false);const auto label=Format("Player");Required(hooks_.submenu)(label,menu);
        }
    }
}
// 가상 조회 콜백이 슬롯의 타입을 바꿀 수 있으므로 판본별 원본 인자 snapshot 시점을 지킨다.
void RawObjectMenu::OpenDamageable(o::Sid sid) const {
    if (Owner(sid)==state_.localOwner) {
        if (pool_.Edition()==o::OriginalEdition::Patch1078) {
            const auto type=Raw(sid)[10];const auto count=Required(hooks_.count)(sid);AddSalvage(sid,type,count);
        } else {
            const auto count=Required(hooks_.count)(sid);const auto type=Raw(sid)[10];AddSalvage(sid,type,count);
        }
    }
    Open(sid);
}
// raw 타입 조건과 명시 타입의 flags2를 구분한다. 음수 환불은DWORD부호 반전 후 원본%d로 출력한다.
void RawObjectMenu::AddSalvage(o::Sid sid,std::uint32_t type,std::uint32_t count) const {
    if (Owner(sid)!=state_.localOwner) return;
    const auto currentType=Raw(sid)[10];const auto& current=Type(currentType);
    if (!(current.flags2&0x200) && currentType!=state_.growingRainBlocker && current.cost==0.0F) return;
    if (type==state_.nugget || state_.editor) return;
    const auto refund=Required(hooks_.salvageRefund)(sid,type,count);
    const auto amount=refund<0 ? std::bit_cast<std::int32_t>(0U-std::bit_cast<std::uint32_t>(refund)) : refund;
    const auto label=Format(refund<0 ? "Salvage costs %d~[IfortGump.3]" : "Salvage gains %d~[IfortGump.3]",{amount});
    const auto flags=Type(type).flags2;
    if (Required(hooks_.itemCount)()) Required(hooks_.separator)();
    if (state_.denySalvage) { Required(hooks_.item)(label,state_.tellCommand,std::string("DenySalvage"));return; }
    Required(hooks_.setConfig)("global.salvageSid",std::to_string(sid.value));
    if (flags&0x200) { Required(hooks_.item)(label,state_.tellCommand,std::string("SalvageVortex"));return; }
    if (flags&0x4000) { Required(hooks_.item)(label,state_.tellCommand,std::string("SalvageInf"));return; }
    if (pool_.Edition()==o::OriginalEdition::Patch1078 && Raw(sid)[10]==state_.suppressedSalvage) return;
    Required(hooks_.item)(label,state_.salvageCommand,std::uint32_t(sid.value));
}
// 정적 문자열 인자는 실제 명령 소비자가 해석하는 같은 상수 표의DWORD로 공급한다.
void BindObjectMenuControls(ObjectMenuHooks& hooks,const MenuGumpControls& controls,const MenuGump& menus,o::ConfigInterface& config,std::function<std::uint32_t(std::string_view)> encodeString) {
    hooks.itemCount=[&menus] { return menus.ItemCount(); };hooks.separator=[&controls] { controls.AddSeparator(); };
    hooks.item=[&controls,encodeString=std::move(encodeString)](std::string_view label,std::uint32_t command,const ObjectMenuParameter& parameter) {
        const auto word=std::holds_alternative<std::uint32_t>(parameter) ? std::get<std::uint32_t>(parameter) : Required(encodeString)(std::get<std::string>(parameter));
        controls.AddText(label,command,word,true,true,false);
    };
    hooks.submenu=[&controls](std::string_view label,std::uint32_t child) { controls.AddSubmenu(label,child,true,false); };
    hooks.setConfig=[&config](std::string_view key,std::string_view value) { config.Set(key,value); };
}
// 네이티브 정수 식별자를 문자열/호스트 주소로 재해석하지 않는다.
void BindObjectMenuIdentity(ObjectMenuHooks& hooks,const o::RawPlayerIdentity& identity) { hooks.ownerSubscriber=[&identity](std::uint32_t owner) { return identity.SubscriberForOwner(owner); }; }
}
