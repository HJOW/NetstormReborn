// x87 비교의 unordered 선택과 실제 TYPE 비용 필드의 현재 쓰기를 보존한다.
#include "o/RawMissionBattleOptions.h"
#include "o/SpStore.h" // 지형/경제와 같은 실제 GameRandom의 상위16비트 난수를 사용한다.
#include <algorithm>
#include <bit>
#include <stdexcept>

namespace netstorm::o {
namespace {
constexpr std::array<std::uint8_t,22> kInitialOptions{0,2,2,0,3,0,2,1,1,0,0,1,0,1,1,2,2,1,2,2,0,0}; // PE0052f5e0부터24BYTE 간격의 실제 기본 선택 BYTE22개다.
constexpr std::array<std::int32_t,4> kTypeCosts{1000,2000,3000,5000}; //0041c8b0의실제스택표다.
constexpr std::array<std::uint32_t,6> kValues{0,25,50,75,100,150}; //0041ca70의실제스택표다.
constexpr std::array<std::int32_t,4> kMinimumDensity{10,15,20,25},kMaximumDensity{30,35,40,45}; //0041c9b0/0041c9e0의 서로 다른 현재 선택표다.
//004b4867의FLD/FSTP32는signaling NaN을quiet로저장한다. 다른비트는보존한다.
float StoreX87Float(float value) {
    auto bits=std::bit_cast<std::uint32_t>(value);if ((bits&0x7f800000U)==0x7f800000U && (bits&0x007fffffU)) bits|=0x00400000U;return std::bit_cast<float>(bits);
}
std::uint8_t OptionByte(std::span<const std::uint8_t> options,std::size_t index) { if (index>=options.size()) throw std::out_of_range("현재 전투 옵션 BYTE 물리 범위");return options[index]; } //선택한 현재BYTE만 읽고 전체87BYTE를 선복사하지 않는다.
float RangeChoice(std::span<const float> choices,std::size_t index) { if (index>=choices.size()) throw std::out_of_range("현재 공급 반경 선택표 물리 범위");return choices[index]; } //원문 순서에서 실제 필요한float만 조회한다.
}
// 원본은 전체87바이트를 지우지 않는다. 미작성 예약40바이트와 양쪽 이웃은 건드리지 않는다.
void ResetMissionBattleOptions(OriginalEdition edition,std::span<std::uint8_t> options) {
    if (edition!=OriginalEdition::Patch1078 || options.size()!=87) throw std::invalid_argument("미션 옵션 초기화의 PATCH87BYTE 계약 오류");
    std::fill_n(options.begin(),23,std::uint8_t{0});std::fill(options.begin()+63,options.end(),std::uint8_t{0});
    // 실제 색상0..7을 BYTE71..78에 설치한다.
    for (std::size_t index=0;index<8;++index) options[71+index]=static_cast<std::uint8_t>(index);
    std::copy(kInitialOptions.begin(),kInitialOptions.end(),options.begin());
}
// 범위 밖 인덱스가 원본 스택/인접 전역을 읽는 손상 경로는 호스트의 명시 오류로 구분한다.
RawMissionBattleOptions::RawMissionBattleOptions(OriginalEdition edition,MissionBattleOptionState state):state_(state) {
    if (edition!=OriginalEdition::Patch1078 || state.options.size()!=87 || state.types.size()!=188 || state.typeUse.size()!=256 || (state.rangeTable.size()!=3 && state.rangeTable.size()!=4))
        throw std::invalid_argument("미션 옵션의 실제 PATCH 저장소 크기 오류");
}
// 원본은 초기 선택값을 먼저 저장한다. NaN에서 std::clamp/min/max로 바꾸지 않는다.
void UpdateMissionOptionSupplyRange(std::span<const std::uint8_t> options,std::span<const float> choices,float& range,float& squared) {
    const auto index=OptionByte(options,4);range=StoreX87Float(RangeChoice(choices,index));
    const auto candidate=RangeChoice(choices,2)<range ? RangeChoice(choices,2):range;
    if (RangeChoice(choices,0)>candidate) range=StoreX87Float(RangeChoice(choices,0));else if (RangeChoice(choices,2)<range) range=StoreX87Float(RangeChoice(choices,2));squared=range*range;
}
void RawMissionBattleOptions::UpdateRange() const { UpdateMissionOptionSupplyRange(state_.options,state_.rangeTable,state_.range,state_.rangeSquared); } //같은 실제 필드에 공통 본체를 적용한다.
// 옵션 BYTE는 호출 때마다 같은 원문에서 읽는다.
std::int32_t MissionOptionTypeCost(std::span<const std::uint8_t> options) {
    const auto index=OptionByte(options,0x10);if (index>=kTypeCosts.size()) throw std::out_of_range("미션 TYPE 비용 옵션 인덱스");return kTypeCosts[index];
}
std::int32_t RawMissionBattleOptions::TypeCost() const { return MissionOptionTypeCost(state_.options); } //현재 동일 옵션의 순수 getter를 재사용한다.
// 이후 설정 변경을 이전 파생값 캐시로 가리지 않는다.
std::uint32_t MissionOptionRefundPercent(std::span<const std::uint8_t> options) {
    const auto index=OptionByte(options,0xf);if (index>=kValues.size()) throw std::out_of_range("미션 파생값 옵션 인덱스");return kValues[index];
}
std::uint32_t RawMissionBattleOptions::CurrentValue() const { return MissionOptionRefundPercent(state_.options); } //실제 현재 비율 선택을 별도 사본 없이 읽는다.
// 원문 옵션 BYTE를 매 getter마다 다시 읽으며 잘못된 인덱스의 스택 외부 읽기는 호스트 오류로 구별한다.
std::int32_t RawMissionBattleOptions::MinimumDensity() const { const auto index=state_.options[7];if (index>=kMinimumDensity.size()) throw std::out_of_range("일반 전투 최소 밀도 옵션 범위");return kMinimumDensity[index]; }
// 상한 조회를 하한 조회의 선택값으로 고정하지 않는다.
std::int32_t RawMissionBattleOptions::MaximumDensity() const { const auto index=state_.options[7];if (index>=kMaximumDensity.size()) throw std::out_of_range("일반 전투 최대 밀도 옵션 범위");return kMaximumDensity[index]; }
// PE004b7666..769e의 signed 반값·두004558f0 호출과 현재 공유 전역 쓰기를 그대로 연결한다.
void RawMissionBattleOptions::InitializeDensity(GameRandom& random,std::int32_t& minimum,std::int32_t& maximum,std::uint32_t& percent) const {
    minimum=MinimumDensity();maximum=MaximumDensity();
    const auto sample=[&] { const auto low=minimum/2,high=maximum/2;return random.Next(static_cast<std::uint32_t>(high)-static_cast<std::uint32_t>(low))+static_cast<std::uint32_t>(low); }; // 각 호출이 같은 현재 한계를 다시 읽고 원문 DWORD 범위 차이/합계 감김을 보존한다.
    const auto first=sample();const auto second=sample();percent=first+second;
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
