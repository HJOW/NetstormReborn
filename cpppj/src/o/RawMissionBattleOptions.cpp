// x87 비교의 unordered 선택과 실제 TYPE 비용 필드의 현재 쓰기를 보존한다.
#include "o/RawMissionBattleOptions.h"
#include <algorithm>
#include <bit>
#include <stdexcept>

namespace netstorm::o {
namespace {
constexpr std::array<std::int32_t,4> kTypeCosts{1000,2000,3000,5000}; //0041c8b0의실제스택표다.
constexpr std::array<std::uint32_t,6> kValues{0,25,50,75,100,150}; //0041ca70의실제스택표다.
//004b4867의FLD/FSTP32는signaling NaN을quiet로저장한다. 다른비트는보존한다.
float StoreX87Float(float value) {
    auto bits=std::bit_cast<std::uint32_t>(value);if ((bits&0x7f800000U)==0x7f800000U && (bits&0x007fffffU)) bits|=0x00400000U;return std::bit_cast<float>(bits);
}
}
// 범위 밖 인덱스가 원본 스택/인접 전역을 읽는 손상 경로는 호스트의 명시 오류로 구분한다.
RawMissionBattleOptions::RawMissionBattleOptions(OriginalEdition edition,MissionBattleOptionState state):state_(state) {
    if (edition!=OriginalEdition::Patch1078 || state.options.size()!=87 || state.types.size()!=188 || state.typeUse.size()!=256 || state.rangeTable.size()!=3)
        throw std::invalid_argument("미션 옵션의 실제 PATCH 저장소 크기 오류");
}
// 원본은 초기 선택값을 먼저 저장한다. NaN에서 std::clamp/min/max로 바꾸지 않는다.
void RawMissionBattleOptions::UpdateRange() const {
    const auto index=state_.options[4];if (index>=state_.rangeTable.size()) throw std::out_of_range("미션 범위 옵션 인덱스");state_.range=StoreX87Float(state_.rangeTable[index]);
    const auto candidate=state_.rangeTable[2]<state_.range ? state_.rangeTable[2] : state_.range;
    if (state_.rangeTable[0]>candidate) state_.range=StoreX87Float(state_.rangeTable[0]);
    else if (state_.rangeTable[2]<state_.range) state_.range=StoreX87Float(state_.rangeTable[2]);
    state_.rangeSquared=state_.range*state_.range;
}
// 옵션 BYTE는 호출 때마다 같은 원문에서 읽는다.
std::int32_t RawMissionBattleOptions::TypeCost() const {
    const auto index=state_.options[0x10];if (index>=kTypeCosts.size()) throw std::out_of_range("미션 TYPE 비용 옵션 인덱스");return kTypeCosts[index];
}
// 이후 설정 변경을 이전 파생값 캐시로 가리지 않는다.
std::uint32_t RawMissionBattleOptions::CurrentValue() const {
    const auto index=state_.options[0xf];if (index>=kValues.size()) throw std::out_of_range("미션 파생값 옵션 인덱스");return kValues[index];
}
// 원본0049a840의현재번호를쓰기직전에확인한다.
void RawMissionBattleOptions::SetSelectedTypeCost(float cost) const {
    const auto type=state_.selectedType;if (type>=state_.types.size()) throw std::out_of_range("미션 비용 대상 TYPE 범위");state_.types[type].cost=cost;
}
// 같은 호출 순서를 작은 실제 몸체로 연결하며 별도 성공 표식을 만들지 않는다.
void RawMissionBattleOptions::ApplyDerived() const { UpdateRange();const auto cost=TypeCost();SetSelectedTypeCost(static_cast<float>(cost));state_.value=CurrentValue(); }
// 원본64DWORD쓰기의현재256바이트결과만적용한다.
void RawMissionBattleOptions::ClearTypes() const { std::fill(state_.typeUse.begin(),state_.typeUse.end(),std::uint8_t{}); }
}
