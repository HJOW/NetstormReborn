// 비용 표시는 정수이며 회수는 원본의 float 비용 저장 뒤 넓은 합/quarter를 유지한다.
#include "client/InventoryAboutMenu.h"
#include <bit>
#include <stdexcept>
namespace netstorm::client {
// 이 처리기는10.78 전용이며 실제 경계가 누락된 빈 성공을 허용하지 않는다.
InventoryAboutMenu::InventoryAboutMenu(const o::SidPool& pool,InventoryAboutState& state,InventoryAboutHooks hooks):pool_(pool),state_(state),hooks_(std::move(hooks)) {
    if (pool.Edition()!=o::OriginalEdition::Patch1078 || !hooks_.begin || !hooks_.root || !hooks_.templateOwner || !hooks_.editor || !hooks_.cost || !hooks_.affordable || !hooks_.helpWord || !hooks_.idle || !hooks_.reference || !hooks_.describe || !hooks_.translate || !hooks_.print || !hooks_.header || !hooks_.sound || !hooks_.entry || !hooks_.attach) throw std::invalid_argument("생산 정보 실제 경계 누락");
}
// callback 뒤에도 같은 현재 내용물의 원문을 다시 읽는다.
std::uint32_t InventoryAboutMenu::Word(o::Sid sid,std::size_t offset) const { const auto raw=pool_.Slot(sid);return raw[offset]|(std::uint32_t(raw[offset+1])<<8); }
//43cb53/43cb57은 정수 비용을 먼저 float로 저장한다. 개수0의 합/quarter에는 그 저장값을 승격한다.
std::int32_t InventoryAboutMenu::Refund(std::uint32_t kind) const {
    const float cost=static_cast<float>(hooks_.cost(kind));double sum=(kind==state_.none || kind==state_.dais) ? 0. : static_cast<double>(cost);if (kind!=state_.fullValue) sum*=0.25;
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::int64_t>(sum)));
}
// 설명/비용/부족 머리/선택 회수/Help/현재 루트 부착/음향의 원본 순서다.
std::uint32_t InventoryAboutMenu::Open(const InventoryItemNode& item,ScreenPoint point) const {
    const o::Sid content{static_cast<std::uint16_t>(item.content)};pool_.Slot(content);const auto menu=hooks_.begin();hooks_.idle(menu,true);hooks_.reference(menu,item.content);
    const auto owner=hooks_.templateOwner();hooks_.describe(Word(content,18),0,owner,0xffffffffU);
    const auto kind=Word(content,18);const auto costPattern=hooks_.translate("Cost: ~i%d~[IfortGump.3]~.\n");const auto cost=hooks_.cost(kind);hooks_.header(hooks_.print(costPattern,cost));
    if (!hooks_.affordable(content)) hooks_.header(hooks_.translate("~r~ENeed more Storm Power to Build!\n"));
    if (!hooks_.editor() && (Word(content,20)&0x20) && pool_.Slot(content)[10]!=state_.suppressedClass) {
        const auto pattern=hooks_.translate("Salvage for %d~[IfortGump.3]");const auto refund=Refund(Word(content,18));const auto label=hooks_.print(pattern,refund);hooks_.entry(label,state_.salvageCommand,item.object,true,true,false);
    }
    const auto helpKind=Word(content,18);const auto help=hooks_.helpWord();hooks_.entry(help ? std::optional<std::string_view>(*help) : std::nullopt,state_.helpCommand,helpKind,true,true,false);
    hooks_.attach(point,hooks_.root());hooks_.sound("opengump.wav");return 1;
}
}
