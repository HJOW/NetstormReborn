// 실제 기계어의 BYTE 옵션 쓰기·현재 선택 재조회·2/4초 포함 기한·후속 효과 순서를 보존한다.
#include "o/RawTutorialTwo.h"
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
constexpr std::uint32_t kCommandMask=0x03ffffff,kTempleGenus=0x200,kWorkshopGenus=0x4000; // 원본 사건 마스크와 제작/선택 부류다.
constexpr double kCorrectionSeconds=2.0,kEnergySeconds=4.0; // 원본 ROM00506590/00506588의 double 지연이다.
}
// 원본 판본과 물리 저장소/필수 하위 경계를 효과 실행 전에 확인한다.
RawTutorialTwo::RawTutorialTwo(const SidPool& pool,std::span<const RiftTypeRecord> types,RawTotalMade& made,
    RawMissionFrameGate& gate,TutorialTwoState state,TutorialTwoHooks hooks)
    :pool_(pool),types_(types),made_(made),gate_(gate),state_(state),hooks_(std::move(hooks)) {
    if (pool.Edition()!=OriginalEdition::Patch1078 || types.size()!=188 || state.battleOptions.size()<5 ||
        !hooks_.gameSeconds || !hooks_.refreshEnergyRange || !hooks_.selected || !hooks_.productionContains ||
        !hooks_.resetSelection || !hooks_.clearSelection || !hooks_.tell || !hooks_.closeTransientGumps)
        throw std::invalid_argument("튜토리얼2 현재 원본 저장소/하위 연결 오류");
}
// 원본004d5430을 실제로 두 번 호출한다. 두 조회 사이 상태 변화도 두 번째 raw 슬롯에 반영한다.
bool RawTutorialTwo::TempleSelected() const {
    if (!hooks_.selected()) return false;
    const auto selected=hooks_.selected(); // 첫 조회의 번호를 재사용하지 않는 현재SID다.
    if (selected>=pool_.Capacity() || selected>0xffffU) throw std::out_of_range("튜토리얼2 현재 선택SID 물리 범위");
    const auto type=pool_.Slot(Sid{static_cast<std::uint16_t>(selected)})[10]; // 원본004ac200이 읽는 실제 타입 BYTE다.
    if (type>=types_.size()) throw std::out_of_range("튜토리얼2 현재 선택TYPE 물리 범위");
    return (types_[type].flags2&kTempleGenus)!=0;
}
// 어느 한 단계 예약이 거부되어도 원본 선택 초기화/해제의 후속 호출은 생략하지 않는다.
void RawTutorialTwo::ClearSelection() const { hooks_.resetSelection();hooks_.clearSelection(); }
// 단계는 switch 진입 때 선택하며 callback이 다음 단계로 바꿔도 같은 호출에서 다른 분기로 넘어가지 않는다.
std::uint32_t RawTutorialTwo::Handle(std::uint32_t event) const {
    auto& mission=gate_.State(); // 단계/임시 타이머의 유일한 실제 저장소다.
    switch (mission.stage) {
    case 'A':
        state_.battleOptions[4]=0;state_.battleOptions[2]=2;hooks_.refreshEnergyRange();
        if (made_.BuiltForGenus(kTempleGenus)>0) static_cast<void>(gate_.Advance());
        break;
    case 'B':
        made_.Allow(std::bit_cast<std::int32_t>(state_.workshopType),1);
        if (made_.BuiltForGenus(kWorkshopGenus)>0) { static_cast<void>(gate_.Advance());ClearSelection(); }
        break;
    case 'C':
        if (TempleSelected()) {
            if (mission.timerSeconds==0.0) mission.timerSeconds=hooks_.gameSeconds()+kCorrectionSeconds;
            else if (mission.timerSeconds<=hooks_.gameSeconds()) {
                if (hooks_.tell("NotVortex")) { hooks_.closeTransientGumps();gate_.PauseForDialog(); }
                ClearSelection();
            }
        }
        if (hooks_.productionContains(state_.archerType)) static_cast<void>(gate_.Advance());
        break;
    case 'D':
        if (std::bit_cast<std::int32_t>(made_.Built(std::bit_cast<std::int32_t>(state_.archerType)))>0) static_cast<void>(gate_.Advance());
        break;
    case 'E':
        if (std::bit_cast<std::int32_t>(made_.Built(std::bit_cast<std::int32_t>(state_.archerType)))>1) { static_cast<void>(gate_.Advance());ClearSelection(); }
        break;
    case 'F':
        if (mission.timerSeconds==0.0) {
            if (TempleSelected()) mission.timerSeconds=hooks_.gameSeconds()+kEnergySeconds;
        } else if (mission.timerSeconds<=hooks_.gameSeconds()) { static_cast<void>(gate_.Advance());ClearSelection(); }
        break;
    case 'G':
        if (std::bit_cast<std::int32_t>(made_.Built(std::bit_cast<std::int32_t>(state_.archerType)))>3) {
            static_cast<void>(gate_.Advance());ClearSelection();mission.timerSeconds=0.0;
        }
        break;
    case 'H':
        state_.denySalvage=0;
        if (mission.timerSeconds==0.0) {
            if ((event&kCommandMask)==state_.salvageCommand) mission.timerSeconds=hooks_.gameSeconds()+kCorrectionSeconds;
        } else if (mission.timerSeconds<=hooks_.gameSeconds()) static_cast<void>(gate_.Advance());
        break;
    default: break;
    }
    return 0;
}
}
