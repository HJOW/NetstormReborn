// 현재Factory목록과종속체인을원문순서로읽고같은Player의5BYTE Deck을갱신한다.
#include "client/RawCampaignProductionBindings.h"
#include <bit>
#include <limits>
#include <stdexcept>
#include <utility>

namespace netstorm::client {
namespace {
// 실제Deck의개수/가중합과내용물type/mask필드위치다.
constexpr std::size_t kDeckCount=0x14c,kDeckChance=0x150,kContentKind=18,kContentFlags=20;
// 비정렬little endian필드의필요한폭만읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset,std::size_t width=4) {
    std::uint32_t value=0;
    // BYTE/WORD/DWORD뒤의다른원본필드를섞지않는다.
    for (std::size_t index=0;index<width;++index) value|=static_cast<std::uint32_t>(raw[offset+index])<<(index*8);
    return value;
}
// 실제Deck의DWORD만쓴다.다른항목과물리꼬리는보존한다.
void Put(std::span<std::uint8_t> raw,std::size_t offset,std::uint32_t value) {
    // 정확한네바이트를낮은쪽부터기록한다.
    for (std::size_t index=0;index<4;++index) raw[offset+index]=static_cast<std::uint8_t>(value>>(index*8));
}
// signed음수개수는빈순회이며양수의물리66개한계만검사한다.
std::int32_t Count(std::span<const std::uint8_t> raw) {
    const auto count=std::bit_cast<std::int32_t>(Read(raw,kDeckCount));if (count>static_cast<std::int32_t>(o::kProductionEntryCapacity)) throw std::out_of_range("생산재계산Deck물리범위오류");return count;
}
}
// 실제계산몸체를공유하며GUI표시나설정결과를상수성공으로대체하지않는다.
RawCampaignProductionBindings::RawCampaignProductionBindings(RawWorldCore core,o::RawPlayerWorld& player,const o::RawCarrierPreDestroy& carrier,
    const o::RawContainedCreate& create,o::RawProductionDeck& deck,const o::RawTotalMade& allowed,const o::ContainedFinderState& contained,
    CampaignProductionState state,CampaignProductionEffects effects)
    :core_(core),player_(player),deck_(deck),allowed_(allowed),contained_(contained),state_(state),effects_(std::move(effects)),indicate_(player,carrier,create) {
    if (&core.pool!=&player.Pool() || &core.pool!=&deck.Pool() || &player!=&deck.Player() || core.pool.Edition()!=o::OriginalEdition::Patch1078 || core.types.size()!=188)
        throw std::invalid_argument("생산재계산의같은실제월드/Player/Deck연결누락");
}
// 0044f970자체는dirty나현재건물수를검사하지않는다.
bool RawCampaignProductionBindings::Contains(std::uint32_t type) const { return deck_.Contains(type); }
// 원본004929e0의실제kindWORD/생성체인을사용한다.
void RawCampaignProductionBindings::IndicateTechnology(std::uint32_t type) const { indicate_.Indicate(type); }
// 조회시현재templateOwner를다시읽는다.
std::uint32_t RawCampaignProductionBindings::RefreshCurrent() const { return Refresh(player_.State().templateOwner); }
// 현재FactoryDWORD가손상됐을때호스트주소를만들기전에거부한다.
o::Sid RawCampaignProductionBindings::Factory(std::uint32_t value) const {
    if (value==0 || value>=core_.pool.Capacity() || value>std::numeric_limits<std::uint16_t>::max()) throw std::out_of_range("생산Factory실제SID범위오류");return {static_cast<std::uint16_t>(value)};
}
// 실제두Factory가상표의+b4는같은12BYTE몸체00453f60이고+80은BYTE상위3비트다.
std::int32_t RawCampaignProductionBindings::FactoryCapacity(o::Sid factory) const {
    const auto raw=core_.pool.Slot(factory);const auto table=Read(raw,0);
    if (table!=0x00509148U && table!=0x00509210U) throw std::logic_error("생산Factory가상+b4의현재실제몸체미복원");
    return static_cast<std::int32_t>(raw[11]>>5)+2;
}
// mode0의빈Mana표식문자열은004735c0에서도빈문자열그대로이며표시Gump효과는발생하지않는다.
std::int32_t RawCampaignProductionBindings::AppendFactoryProducts(o::Sid factory,std::uint32_t deckToken) const {
    auto remaining=FactoryCapacity(factory);std::array<std::uint8_t,255> seen{};o::RawContainedFinder finder(core_.pool,contained_);
    // 기존커서가후보반환전에저장한next를보존하며매후보의현재WORD조건을읽는다.
    for (auto content=finder.Begin(factory);content.value;content=finder.Next()) {
        const auto raw=core_.pool.Slot(content);
        if (!(raw[kContentFlags]&0x10U)) continue;
        const auto kind=Read(raw,kContentKind,2);
        if (kind>=core_.types.size() || kind>=seen.size()) throw std::out_of_range("생산Factory의현재TYPE/중복표범위오류");
        static_cast<void>(core_.types[kind]);static_cast<void>(allowed_.Allowed(static_cast<std::int32_t>(kind)));
        // 원본mode0도허용표는조회하지만Deck추가를금지표로거르지는않는다.
        if (deckToken) deck_.Activate(deckToken,kind,1);
        if (!seen[kind]) { --remaining;seen[kind]=1; }
    }
    return remaining;
}
// 0044f7c0은별도Factory목록의현재owner/type/내용물과같은Deck을재계산한다.
std::uint32_t RawCampaignProductionBindings::Refresh(std::uint32_t owner) const {
    const auto token=deck_.Get(owner);
    if (!core_.ownerMode.fort && !core_.ownerMode.battle) { if (!effects_.modeAssertion) throw std::logic_error("생산재계산실제Fort/Battle진단미연결");effects_.modeAssertion(); }
    if (!state_.mission && state_.fullDeck) return token;
    auto raw=deck_.Bytes(token);
    // 기존항목의power만-1로바꾸며기존chance/remaining/order를유지한다.
    for (std::int32_t index=0;index<Count(raw);++index) raw[static_cast<std::size_t>(index)*o::kProductionEntryBytes+2]=255;
    Put(raw,kDeckChance,0);
    if (!effects_.configInt) throw std::logic_error("생산재계산getTeleportTechFree실제설정미연결");
    if (effects_.configInt("getTeleportTechFree")) deck_.Activate(token,state_.freeTechnologyType,1);
    // 같은공통PostPop/삭제가변경하는현재Factory개수와DWORD목록을매반복읽는다.
    for (std::uint32_t index=0;index<core_.bookkeeping.Factories().count;++index) {
        const auto current=Factory(core_.bookkeeping.Factories().Items()[index]);const auto object=core_.pool.Slot(current);
        if (static_cast<std::uint32_t>(object[34])!=owner) continue;
        const auto type=object[10];if (type>=core_.types.size()) throw std::out_of_range("생산Factory현재TYPE범위오류");
        if (core_.types[type].flags2&0x200U) { if (allowed_.Allowed(static_cast<std::int32_t>(state_.templeProduct))) deck_.Activate(token,state_.templeProduct,1); }
        else if (core_.types[type].flags1&0x04000000U) static_cast<void>(AppendFactoryProducts(current,token));
    }
    deck_.Compact(token);raw=deck_.Bytes(token);std::uint32_t total=0;
    // signed power가양수인항목의BYTEchance만DWORD감김합산한다.
    for (std::int32_t index=0;index<Count(raw);++index) { const auto offset=static_cast<std::size_t>(index)*o::kProductionEntryBytes;if (std::bit_cast<std::int8_t>(raw[offset+2])>0) total+=raw[offset+1]; }
    Put(raw,kDeckChance,total);return token;
}
// 같은현재허용표를표시소비자에게공급한다.
bool RawCampaignProductionBindings::Allowed(std::int32_t type) const { return allowed_.Allowed(type); }
// 단일월드조립을확인한다.
const o::SidPool& RawCampaignProductionBindings::Pool() const { return core_.pool; }
// 실제Player포인터와같은수명저장소를빌린다.
const o::RawProductionDeck& RawCampaignProductionBindings::Deck() const { return deck_; }
}
