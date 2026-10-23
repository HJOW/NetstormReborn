// 현재 manager/owner/두 모드 읽기와 고정 stream·부분 효과를 원본 부모 순서로 보존한다.
#include "o/RawFortSnapshotParent.h"
#include <bit>
#include <stdexcept>

namespace netstorm::o {
namespace {
constexpr std::uint32_t kTemplateBase=0x005c89f0U,kTemplateStride=20U; // 원본 this/현재칸의 DWORD 주소 비교에만 사용하는 PE 배열 주소·폭이다.
// 선택되지 않은 child는 요구하지 않고 도달한 누락 경계만 앞선 효과를 보존한 채 거부한다.
template<class Function> const Function& Need(const Function& function) {
    if (!function) throw std::logic_error("전체 Fort snapshot 부모의 실제 경계 미연결");return function;
}
// 원본 LEA와 같은 DWORD 감김 주소다. owner 상위 두 비트가 같은 this 주소를 만드는 경우도 보존한다.
std::uint32_t TemplateAddress(std::uint32_t slot) { return kTemplateBase+slot*kTemplateStride; }
// 현재 owner를 호스트 슬롯에 매핑한다. 실제 20BYTE 기록 사이를 가리키는 원본 밖 물리 입력은 거부한다.
std::int32_t CurrentSlot(std::uint32_t owner) {
    const auto delta=static_cast<std::int64_t>(TemplateAddress(owner))-kTemplateBase;
    if (delta%kTemplateStride) throw std::out_of_range("현재 Template 주소가 실제20BYTE 기록 경계 밖입니다");
    return static_cast<std::int32_t>(delta/kTemplateStride);
}
// 현재 기술 부모를 먼저 캡처한 뒤 현재 manager/Create 반환 stream을 따로 보관한다.
void Technology(std::int32_t slot,const FortSnapshotParentHooks& hooks) {
    if (!Need(hooks.manager)(slot)) Need(hooks.assertion)(FortSnapshotParentAssertion::MissingTechnologyTemplate);
    const auto parent=Need(hooks.technologyParent)();const auto manager=Need(hooks.manager)(slot);
    const auto stream=Need(hooks.create)(manager,"Technology");Need(hooks.technology)(parent,stream);
}
// 공통 끝부분도 manager를 미리 고정하지 않으며 각 Create의 반환 stream을 그대로 child에 전달한다.
void MoneyMission(std::int32_t slot,const FortSnapshotParentHooks& hooks) {
    const auto moneyManager=Need(hooks.manager)(slot);const auto moneyStream=Need(hooks.create)(moneyManager,"Money");Need(hooks.money)(moneyStream);
    if (!Need(hooks.manager)(slot)) Need(hooks.assertion)(FortSnapshotParentAssertion::MissingMissionTemplate);
    const auto missionManager=Need(hooks.manager)(slot);const auto missionStream=Need(hooks.create)(missionManager,"Mission");Need(hooks.mission)(missionStream);
}
}
// 처음 this를 유지하면서 각 진단·큰 child 뒤의 현재 전역을 원문 위치에서 다시 읽는다.
void SnapshotFortTemplate(std::int32_t slot,const FortSnapshotParentGlobals& globals,const FortSnapshotParentHooks& hooks) {
    if (!Need(hooks.manager)(slot)) Need(hooks.assertion)(FortSnapshotParentAssertion::MissingFortTemplate);
    if (!Need(globals.fort)()) Need(hooks.assertion)(FortSnapshotParentAssertion::NotFortMode);
    if (TemplateAddress(std::bit_cast<std::uint32_t>(slot))!=TemplateAddress(Need(globals.templateOwner)())) Need(hooks.assertion)(FortSnapshotParentAssertion::FortNotMine);
    Need(hooks.typeNames)(slot);Technology(slot,hooks);Need(hooks.objects)(slot);Need(hooks.territory)(slot);Need(hooks.deck)(slot);
    MoneyMission(slot,hooks);Need(hooks.badges)(slot);Need(hooks.notify)(slot);
}
// TypeNames/Badges를 추가하지 않으며 Deck은 Objects 앞에 두고 CoreData를 마지막 Notify 앞에 둔다.
void SnapshotBattleTemplate(std::int32_t slot,const FortSnapshotParentGlobals& globals,const FortSnapshotParentHooks& hooks) {
    if (!Need(hooks.manager)(slot)) Need(hooks.assertion)(FortSnapshotParentAssertion::MissingBattleTemplate);
    if (!Need(globals.battle)()) Need(hooks.assertion)(FortSnapshotParentAssertion::NotBattleMode);
    if (TemplateAddress(std::bit_cast<std::uint32_t>(slot))!=TemplateAddress(Need(globals.templateOwner)())) Need(hooks.assertion)(FortSnapshotParentAssertion::BattleNotMine);
    Technology(slot,hooks);Need(hooks.deck)(slot);Need(hooks.objects)(slot);Need(hooks.territory)(slot);
    MoneyMission(slot,hooks);Need(hooks.coreData)(slot);Need(hooks.notify)(slot);
}
// 두 번째 if는 첫 번째 전체 Snapshot이 바꾼 현재 모드/owner를 그대로 따른다.
void SnapshotCurrentTemplate(const FortSnapshotParentGlobals& globals,const FortSnapshotParentHooks& hooks) {
    if (Need(globals.battle)()) { const auto slot=CurrentSlot(Need(globals.templateOwner)());SnapshotBattleTemplate(slot,globals,hooks); }
    if (Need(globals.fort)()) { const auto slot=CurrentSlot(Need(globals.templateOwner)());SnapshotFortTemplate(slot,globals,hooks); }
}
// 최초 manager 관문을 반복하지 않으며 Battle이 생략되면 진입 owner의 원본 EAX를 유지한다.
void SnapshotCurrentTemplateIfPresent(const FortSnapshotParentGlobals& globals,const FortSnapshotParentHooks& hooks) {
    auto owner=Need(globals.templateOwner)();const auto slot=CurrentSlot(owner);if (!Need(hooks.manager)(slot)) return;
    if (Need(globals.battle)()) { SnapshotBattleTemplate(slot,globals,hooks);owner=Need(globals.templateOwner)(); }
    if (Need(globals.fort)()) SnapshotFortTemplate(CurrentSlot(owner),globals,hooks);
}
}
