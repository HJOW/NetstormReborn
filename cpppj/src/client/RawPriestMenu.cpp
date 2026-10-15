// 실제 사제 메뉴의 조건/순서와 두 번의 금액 조회를 원본 기계어대로 유지한다.
#include "client/RawPriestMenu.h"
#include <algorithm>
#include <bit>
#include <stdexcept>

namespace netstorm::client {
namespace {
// 원본 Player 기록 크기·개수와 메뉴/금액 포맷버퍼 한도다.
constexpr std::size_t kPlayerBytes=0xb4,kPlayers=9,kFormatBytes=256;
// 실제 Regular 사건과 contained 기술/기도 플래그다.
constexpr std::uint32_t kBlocked=0x493,kBuilding=0x25d,kTechnology=4,kPrayer=1;
// 도달한 미연결 효과를 성공이나 빈 목록으로 대체하지 않는다.
template<class Function> const Function& Required(const Function& function) {
    if (!function) throw std::logic_error("사제 메뉴의 실제 효과/조회 미연결");return function;
}
// 정렬되지 않은 원본 WORD/DWORD 필드를 낮은 바이트부터 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset,std::size_t width=4) {
    if (offset+width>raw.size()) throw std::out_of_range("사제 메뉴 raw 필드 범위 오류");std::uint32_t result=0;
    // 원본 little endian 바이트의 순서를 유지한다.
    for (std::size_t index=0;index<width;++index) result|=std::uint32_t(raw[offset+index])<<(8*index);
    return result;
}
}
// PATCH의 실제 저장소 크기를 확인하며 게임효과는 생성 중 실행하지 않는다.
RawPriestMenu::RawPriestMenu(const o::SidPool& pool,std::span<const o::RiftTypeRecord> types,std::span<const o::RiftTypeFrames> frames,
    std::span<const std::uint8_t> players,const o::SquidPostPopList& excluded,const o::ContainedFinderState& contained,
    PriestMenuState& state,PriestMenuHooks hooks):pool_(pool),types_(types),frames_(frames),players_(players),excluded_(excluded),contained_(contained),state_(state),hooks_(std::move(hooks)) {
    if (pool.Edition()!=o::OriginalEdition::Patch1078 || types.size()!=188 || frames.size()!=types.size() || players.size()!=kPlayerBytes*kPlayers)
        throw std::invalid_argument("사제 메뉴 PATCH 공유 저장소 크기 오류");
}
// 원본 메뉴는 상태 비트로 별도free/dead 필터를 하지 않는다. 잘못된 물리 SID만 거부한다.
std::span<const std::uint8_t> RawPriestMenu::Raw(o::Sid sid) const {
    if (!sid.value || sid.value>=pool_.Capacity()) throw std::out_of_range("사제 메뉴 SID 범위 오류");return pool_.Slot(sid);
}
// 메뉴에서 실제 도달한 타입 속성만 확인한다.
const o::RiftTypeRecord& RawPriestMenu::Type(std::uint32_t type) const {
    if (type>=types_.size()) throw std::out_of_range("사제 메뉴 TYPE 범위 오류");return types_[type];
}
// 콜백 뒤에도 현재 동일 Player span을 다시 읽는다.
std::uint32_t RawPriestMenu::Player(std::uint32_t owner,std::size_t offset) const {
    if (owner>=kPlayers || offset+4>kPlayerBytes) throw std::out_of_range("사제 메뉴 Player 범위 오류");return Read(players_,owner*kPlayerBytes+offset);
}
// 번역기는 원본 %s/%d 인자 순서를 보존한다. 반환 NUL 뒤 바이트는 원본 문자열이 아니므로 제거한다.
std::string RawPriestMenu::Format(std::string_view key,std::initializer_list<PriestMenuArgument> arguments) const {
    auto result=Required(hooks_.format)(key,{arguments.begin(),arguments.size()});result.resize(result.find('\0')==std::string::npos ? result.size() : result.find('\0'));
    if (result.size()>=kFormatBytes) throw std::length_error("사제 메뉴 원본 포맷 버퍼 한도 초과");return result;
}
// 실제FrameCode의 첫바이트는 MSVC signed char다. 큰 frame이나 음수frame의 원본 무효메모리는 호스트 예외다.
std::int32_t RawPriestMenu::FrameState(o::Sid priest) const {
    const auto raw=Raw(priest);const auto type=raw[10];Type(type);const auto codes=frames_[type].Codes();const auto frame=Read(raw,36);
    if (frame>=codes.size()) throw std::out_of_range("사제 메뉴 현재 물리프레임 범위 오류");return std::bit_cast<std::int8_t>(codes[frame].side)-'A';
}
// 동일 내용을 여러 번 묻더라도 현재 Technology와 체인을 매 호출 새로 조회한다.
bool RawPriestMenu::HasTheme(std::int32_t theme) const {
    const auto technology=Required(hooks_.currentTechnology)();
    if (!technology || technology>=pool_.Capacity()) throw std::out_of_range("사제 메뉴 현재 Technology SID 오류");
    o::RawContainedFinder finder(pool_,contained_);
    // finder는 다음번호를 반환 전에 저장하는 기존 실제 몸체를 공유한다.
    for (auto sid=finder.Begin(o::Sid{static_cast<std::uint16_t>(technology)});sid.value;sid=finder.Next()) {
        const auto raw=Raw(sid);if (!(Read(raw,20,2)&kTechnology)) continue;const auto type=Read(raw,18,2);const auto& record=Type(type);
        if (record.theme==theme && !(record.flags2&0x800)) return true;
    }
    return false;
}
// 금액부족은 접두 ~r와 mana 아이콘군만 바꾸며 enabled에는 포함하지 않는다.
void RawPriestMenu::AddBuild(std::uint32_t type,bool disabled,o::Sid priest,bool manaIcon) const {
    if (!Required(hooks_.allowed)(type)) return;
    const auto amount=Required(hooks_.cost)(type);const auto money=Required(hooks_.money)(state_.localOwner);
    // x87은 fild의 정수를 단정도로 저장하지 않고 SP float와 비교한다. NaN은 지불불가다.
    const bool affordable=state_.editor || static_cast<double>(amount)<=static_cast<double>(money);
    const auto price=amount ? Format(" for %s%d~[IfortGump.3]",{std::string(affordable ? "" : "~r"),amount}) : std::string{};
    const auto excluded=excluded_.Items();disabled=disabled || std::find(excluded.begin(),excluded.end(),type)!=excluded.end();std::string label;
    if (type==state_.dais || type==state_.altar) {
        const auto level=(Player(state_.localOwner,0x70)>>8)&255U;
        label=Format("Build Level %d %s%s",{static_cast<std::int32_t>(level)+1,Required(hooks_.description)(type),price});
    } else if (manaIcon) {
        const auto theme=std::bit_cast<std::int32_t>(std::bit_cast<std::uint32_t>(Type(type).theme)+(affordable ? 4U : 0U));
        label=Format("~[Imana.%d] Build %s%s",{theme,Required(hooks_.description)(type),price});
    } else label=Format("Build %s%s",{Required(hooks_.description)(type),price});
    Required(hooks_.item)(label,state_.buildCommand,(type<<16)|priest.value,!disabled);
}
// 두 번째 SP 조회 전에 비용도 다시 조회하는 원본 순서를 유지한다.
bool RawPriestMenu::SelectBuild(std::uint32_t type,o::Sid priest) const {
    const auto& record=Type(type);
    if (!state_.editor && (record.flags2&0x4000) && !HasTheme(record.theme)) { Required(hooks_.tell)("NoTechTheme");return false; }
    const float first=static_cast<float>(Required(hooks_.cost)(type));const auto firstMoney=Required(hooks_.money)(state_.localOwner);
    if (!state_.editor && !(first<=firstMoney)) { Required(hooks_.tell)("BuildNoBucks");return false; }
    const auto second=Required(hooks_.cost)(type);const auto secondMoney=Required(hooks_.money)(state_.localOwner);
    // 둘째 fild 비교는 int를 float로 축소하지 않는다. 시점 사이 잔액이 줄면 안내 없이 반환한다.
    if (!state_.editor && !(static_cast<double>(second)<=static_cast<double>(secondMoney))) return false;
    const auto preview=Required(hooks_.createPreview)(priest,type);Required(hooks_.takePreview)(preview,0x160);return true;
}
// 사제의 비가상 기본 객체메뉴 호출을 포함한 전체 원본 분기 순서다.
void RawPriestMenu::Open(o::Sid priest) const {
    const auto raw=Raw(priest);
    if (raw[34]!=state_.localOwner || Required(hooks_.hasEvent)(priest,kBlocked)) { Required(hooks_.baseMenu)(priest);return; }
    Required(hooks_.beginFancy)();Required(hooks_.title)(Format("Construct Building\n"));
    const auto owner=raw[34];const bool templePresent=Player(owner,0x7c)!=0 || Player(owner,0x84)!=0;
    Required(hooks_.beginVertical)();
    // 세 신전 타입은 원본 전역 순서대로 현재 값을 읽는다.
    for (std::size_t index=0;index<state_.temples.size();++index) AddBuild(state_.temples[index],templePresent,priest,true);
    const auto templeLabel=Format("%sTemple",{std::string(templePresent ? "~x" : "")});const auto templeMenu=Required(hooks_.takeMenu)();Required(hooks_.submenu)(templeLabel,templeMenu,!templePresent);
    // 원본의 || 단락평가 순서와 workshop 모드의 뒤늦은 조회를 유지한다.
    bool workshopAllowed=false;
    for (std::size_t index=0;index<state_.workshops.size();++index) if (Required(hooks_.allowed)(state_.workshops[index])) { workshopAllowed=true;break; }
    if (workshopAllowed && (state_.workshopMode || state_.special || state_.editor)) {
        Required(hooks_.beginVertical)();
        // 원본 workshop 타입 순서에 대응하는 theme 인자는3,0,1,2다.
        for (std::size_t index=0;index<state_.workshops.size();++index) AddBuild(state_.workshops[index],!state_.editor && !HasTheme(static_cast<std::int32_t>((index+3)%4)),priest,true);
        const auto label=Format("Workshop");const auto menu=Required(hooks_.takeMenu)();Required(hooks_.submenu)(label,menu,true);
    }
    const auto outpost=state_.outpost;const auto technology=Required(hooks_.currentTechnology)();
    if (technology>=pool_.Capacity()) throw std::out_of_range("사제 메뉴 Technology 범위 오류");
    if (Required(hooks_.containedType)(o::Sid{static_cast<std::uint16_t>(technology)},kTechnology,outpost) || !state_.chargeBlocked || state_.editor) AddBuild(outpost,false,priest,false);
    const bool daisPresent=Player(owner,0x8c)!=0 || Player(owner,0x94)!=0;AddBuild(state_.dais,daisPresent,priest,false);
    const bool immobile=Required(hooks_.immobile)(priest);bool constructDisabled=immobile;
    if (!constructDisabled) constructDisabled=Required(hooks_.hasEvent)(priest,kBuilding);
    if (!constructDisabled) { const auto frame=FrameState(priest);constructDisabled=frame>=10 && frame<=14; }
    const auto label=Format("%sConstruct~.",{std::string(immobile ? "~r" : "")});const auto menu=Required(hooks_.takeMenu)();Required(hooks_.submenu)(label,menu,!constructDisabled);
    if (!state_.chargeBlocked && !Required(hooks_.containedType)(priest,kPrayer,state_.devastation) && !immobile && !Required(hooks_.hasEvent)(priest,kBuilding)) {
        const auto frame=FrameState(priest);
        if (!(frame>=10 && frame<=14) && !Required(hooks_.hasEvent)(priest,kBlocked)) {
            Required(hooks_.beginFancy)();Required(hooks_.title)(Format("Prayer\n"));Required(hooks_.item)(Format("Devastation"),state_.prayCommand,(state_.devastation<<16)|priest.value,true);
            const auto prayerLabel=Format("%sPray~.",{std::string{}});const auto prayerMenu=Required(hooks_.takeMenu)();Required(hooks_.submenu)(prayerLabel,prayerMenu,true);
        }
    }
    Required(hooks_.item)(Format("View Netstorm Knowledge"),state_.knowledgeCommand,0,true);Required(hooks_.separator)();Required(hooks_.baseMenu)(priest);
}
// 메뉴 저장소나 수명을 복제하지 않고 기존 API에만 위임한다.
void BindPriestMenuControls(PriestMenuHooks& hooks,const MenuGumpWidgets& widgets,const MenuGumpControls& controls,const MenuGump& menus) {
    hooks.beginFancy=[&widgets] { widgets.BeginFancy(); };hooks.beginVertical=[&widgets] { widgets.BeginVertical(0); };
    hooks.title=[&controls](std::string_view text) { controls.AddTitle(text); };hooks.takeMenu=[&menus] { return menus.TakeCurrent(); };
    hooks.submenu=[&controls](std::string_view text,std::uint32_t child,bool enabled) { controls.AddSubmenu(text,child,enabled,false); };
    hooks.item=[&controls](std::string_view text,std::uint32_t command,std::uint32_t parameter,bool enabled) { controls.AddText(text,command,parameter,true,enabled,false); };
    hooks.separator=[&controls] { controls.AddSeparator(); };
}
// 같은 월드의 조회와 contained 생성 몸체만 묶으며, 실제 커서 소유권/번역은 상위에 남긴다.
void BindPriestMenuWorld(PriestMenuHooks& hooks,const o::SidPool& pool,const o::RawPriestState& priest,
    const o::SquidProcessHost& processes,const o::RawPlayerWorld& players,const o::RawCarrierPreDestroy& carrier,
    const o::SquidReward& reward,const o::RawContainedCreate& create) {
    if (&pool!=&priest.Pool() || &pool!=&processes.Pool() || &pool!=&players.Pool() || &pool!=&carrier.Pool() || &pool!=&reward.Pool() || &pool!=&create.Pool())
        throw std::invalid_argument("사제 메뉴 월드 Pool 연결 오류");
    hooks.immobile=[&priest](o::Sid sid) { return priest.Immobile(sid); };hooks.hasEvent=[&processes](o::Sid sid,std::uint32_t event) { return processes.FindEvent(sid,event)!=nullptr; };
    hooks.currentTechnology=[&players] { return players.CurrentTechnology(); };hooks.cost=[&reward](std::uint32_t type) { return reward.TypeCostUnits(type); };
    hooks.containedType=[&pool,&carrier](o::Sid parent,std::uint32_t mask,std::uint32_t type) { const auto found=carrier.FindContained(parent,mask,type,0);return found.value ? Read(pool.Slot(found),18,2) : 0U; };
    hooks.createPreview=[&create](o::Sid parent,std::uint32_t type) { return create.Create(parent,type,0,0x20,0x10,0); };
}
}
