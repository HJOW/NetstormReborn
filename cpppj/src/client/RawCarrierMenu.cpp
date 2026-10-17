// 원본 Carrier 메뉴의 현재 조회/분기/문자열/명령 인자 순서를 유지한다.
#include "client/RawCarrierMenu.h"
#include <bit>
#include <cmath>
#include <stdexcept>

namespace netstorm::client {
namespace {
// 도달한 실제 하위를 빈 성공/금액/위치로 대체하지 않는다.
template<class Function> const Function& Required(const Function& function) {
    if (!function) throw std::logic_error("실제 Carrier 메뉴 하위 미연결");return function;
}
}
// 판본별 현재 TYPE 개수를 확인하고 실제 효과는 아직 실행하지 않는다.
RawCarrierMenu::RawCarrierMenu(const o::SidPool& pool,std::span<const o::RiftTypeRecord> types,const o::ContainedFinderState& contained,CarrierMenuState& state,CarrierMenuHooks hooks)
    :pool_(pool),types_(types),contained_(contained),state_(state),hooks_(std::move(hooks)) {
    if (types.size()!=(pool.Edition()==o::OriginalEdition::Patch1078 ? 188U : 171U)) throw std::invalid_argument("Carrier 메뉴 TYPE 표 크기 오류");
}
// 메뉴가 실제 읽는 물리 SID 범위만 확인한다.
std::span<const std::uint8_t> RawCarrierMenu::Raw(o::Sid sid) const {
    if (!sid.value || sid.value>=pool_.Capacity()) throw std::out_of_range("Carrier 메뉴 SID 범위 오류");return pool_.Slot(sid);
}
// 현재 TYPE 참조를 복사하지 않는다.
const o::RiftTypeRecord& RawCarrierMenu::Type(std::uint32_t type) const {
    if (type>=types_.size()) throw std::out_of_range("Carrier 메뉴 TYPE 범위 오류");return types_[type];
}
// PATCH+22/CD+20의 현재 소유자BYTE다.
std::uint32_t RawCarrierMenu::Owner(o::Sid sid) const { return Raw(sid)[pool_.Edition()==o::OriginalEdition::Patch1078 ? 34 : 32]; }
// 정렬되지 않은 contained WORD를 원본 little endian으로 읽는다.
std::uint32_t RawCarrierMenu::Word(std::span<const std::uint8_t> raw,std::size_t offset) { return raw[offset]|(std::uint32_t(raw[offset+1])<<8); }
// 원본 printf의 NUL 뒤 바이트는 메뉴 텍스트에 포함하지 않는다.
std::string RawCarrierMenu::Format(std::string_view key,std::initializer_list<PriestMenuArgument> arguments) const {
    auto text=Required(hooks_.format)(key,{arguments.begin(),arguments.size()});const auto nul=text.find('\0');if (nul!=std::string::npos) text.resize(nul);
    if (text.size()>=256) throw std::length_error("Carrier 메뉴 원본256바이트 서식 초과");return text;
}
// x87 불확정64비트 값의 하위DWORD는0이다. 유한 정상 범위는0쪽으로 절삭한다.
std::int32_t RawCarrierMenu::Truncate(float value) {
    const double wide=value;if (!std::isfinite(wide) || wide<-9223372036854775808.0 || wide>=9223372036854775808.0) return 0;
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::int64_t>(wide)));
}
// CD의 정수 검사는 수치 절삭이 아니라 float의 원문DWORD 부호 검사다.
bool RawCarrierMenu::ValidPoint(std::array<float,2> point) const {
    const double bound=state_.mapSize;
    if (pool_.Edition()==o::OriginalEdition::Patch1078) return point[0]>0.0F && point[0]<bound && point[1]>0.0F && point[1]<bound;
    return std::bit_cast<std::int32_t>(point[0])>0 && point[0]<bound && std::bit_cast<std::int32_t>(point[1])>0 && point[1]<static_cast<float>(state_.mapSize);
}
// 이동·위치·SP·사건 콜백 뒤 다시 읽는 필드와 이미 보존한 내용물 타입/flags를 구분한다.
void RawCarrierMenu::Open(o::Sid carrier) const {
    const bool patch=pool_.Edition()==o::OriginalEdition::Patch1078;
    if (!(Type(Raw(carrier)[10]).flags1&0x1000000)) {
        const bool immobile=(Raw(carrier)[patch ? 40 : 35]&0x20)!=0 || Required(hooks_.groundImmobile)(carrier);
        o::RawContainedFinder finder(pool_,contained_);auto content=finder.Begin(carrier);Format("~wNothing");
        // 기존 finder는 다음SID를 미리 저장하므로 현재 내용물의 next 변경에 영향을 받지 않는다.
        for (;content.value;content=finder.Next()) {
            const auto type=Word(Raw(content),18);const auto description=Required(hooks_.description)(type);bool flying=false;
            if (Type(Raw(carrier)[10]).flags2&0x20000) {
                const auto process=Required(hooks_.movementType)(Raw(carrier)[10]);const auto path=Required(hooks_.findPath)(carrier,process);
                flying=path.value && Required(hooks_.pathActive)(path);
            }
            bool enabled=ValidPoint(Required(hooks_.dropPoint)(carrier,type));const auto flags=Word(Raw(content),20);std::string suffix;
            if (immobile || flying) { suffix=Format(immobile ? "~r~E(Not while paralyzed)" : "~r~E(Not while flying)");enabled=false; }
            else if (flags&1) {
                const auto amount=patch ? Required(hooks_.patchCost)(type) : Truncate(Type(type).cost);
                if (patch) { const auto sp=Required(hooks_.money)(LocalOwner());enabled=Editor() || static_cast<double>(amount)<=static_cast<double>(sp); }
                else enabled=Editor() || !(state_.cdSp<static_cast<float>(amount));
                suffix=Format("for %s%d~[IfortGump.3]",{std::string(enabled ? "" : "~r~E"),amount});
            }
            if (suffix.size()>=128) throw std::length_error("Carrier 메뉴 원본128바이트 안내 초과");
            if (!(flags&1)) {
                const auto label=Format("Carrying: ~i%s~.\n",{description});Required(hooks_.header)(label);
                if (Owner(carrier)==LocalOwner()) {
                    const auto text=Format("%sDrop~. ~i%s~. %s",{std::string(enabled ? "" : "~r~E"),description,suffix});Required(hooks_.item)(text,state_.dropCommand,carrier.value,!immobile && enabled);
                }
            } else if (Owner(carrier)==LocalOwner()) {
                const bool blocked=Required(hooks_.hasEvent)(carrier,0x493);bool building=false;
                if (patch && Raw(carrier)[10]==state_.priestType) building=Required(hooks_.hasEvent)(carrier,0x25d);
                const auto text=Format("%sCast~. ~i%s~.~[I%d.*] %s",{std::string(enabled ? "" : "~r~E"),description,static_cast<std::int32_t>(type),suffix});
                Required(hooks_.item)(text,patch ? state_.castCommand : state_.dropCommand,patch ? carrier.value : carrier.value|(std::uint32_t(content.value)<<16),!immobile && enabled && !blocked && !building);
                const auto about=Format("About %s",{description});Required(hooks_.item)(about,state_.helpCommand,type,true);
            }
        }
    }
    Required(hooks_.damageableMenu)(carrier);
}
// 현재 세션 연결이 있으면 명령/금액 callback 뒤에도 같은 Player 전역을 다시 읽는다.
std::uint32_t RawCarrierMenu::LocalOwner() const { return state_.localOwnerRead ? state_.localOwnerRead() : state_.localOwner; }
// 기존 두 판본의 값형 fixture 경로는 그대로 유지한다.
bool RawCarrierMenu::Editor() const { return state_.editorRead ? state_.editorRead() : state_.editor; }
// 작성할 문자열과 전체DWORD 인자를 기존 실제 controlled 메뉴에 넘긴다.
void BindCarrierMenuControls(CarrierMenuHooks& hooks,const MenuGumpControls& controls) {
    hooks.header=[&controls](std::string_view text) { controls.AddHeader(text); };
    hooks.item=[&controls](std::string_view text,std::uint32_t command,std::uint32_t parameter,bool enabled) { controls.AddText(text,command,parameter,true,enabled,false); };
}
// 실제 회수/공통 객체 메뉴를 항상 마지막에 호출한다.
void BindCarrierMenuBase(CarrierMenuHooks& hooks,const RawObjectMenu& objects) { hooks.damageableMenu=[&objects](o::Sid sid) { objects.OpenDamageable(sid); }; }
// 사제의 원본 기본 객체 호출을 실제 Carrier 몸체에 연결한다.
void BindPriestMenuCarrier(PriestMenuHooks& hooks,const RawCarrierMenu& carrier) { hooks.baseMenu=[&carrier](o::Sid sid) { carrier.Open(sid); }; }
// 조회/사건/내용물/비용은 같은 실제 월드 저장소만 허용한다.
void BindCarrierMenuWorld(CarrierMenuHooks& hooks,const o::SidPool& pool,const o::RawPriestState& priest,const o::SquidProcessHost& processes,const o::RawContainedLookup& lookup,const o::SquidReward& reward,
    std::function<std::uint32_t(std::uint32_t)> movementType,std::function<bool(o::Sid)> pathActive) {
    if (&pool!=&priest.Pool() || &pool!=&processes.Pool() || &pool!=&lookup.Pool() || &pool!=&reward.Pool()) throw std::invalid_argument("Carrier 메뉴 월드 Pool 연결 오류");
    hooks.groundImmobile=[&priest](o::Sid sid) { return priest.GroundImmobile(sid); };hooks.patchCost=[&reward](std::uint32_t type) { return reward.TypeCostUnits(type); };
    hooks.hasEvent=[&processes](o::Sid sid,std::uint32_t event) { return processes.FindEvent(sid,event)!=nullptr; };
    hooks.movementType=std::move(movementType);hooks.pathActive=std::move(pathActive);hooks.findPath=[&lookup](o::Sid sid,std::uint32_t type) { return lookup.Find(sid,type,0,false); };
}
}
