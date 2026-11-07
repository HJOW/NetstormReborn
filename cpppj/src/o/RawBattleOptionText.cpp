// 선택값은 원문 최대 인덱스를 포함하며 없는 값을 임의 기본값으로 바꾸지 않는다.
#include "o/RawBattleOptionText.h"
#include <array>
#include <stdexcept>

namespace netstorm::o { //같은 현재 번역기는 선택된 원문 위치에서만 호출한다.
namespace { //22개 표의 같은 정적 문자열 수명을 유지한다.
constexpr std::array<std::string_view,5> kValues0{"Netstorm Standard","Netstorm Classic","Netstorm Flexible","Netstorm Geyserless","Player Customizable"}; //원문 Game Type의 모든 선택 문자열이다.
constexpr std::array<std::string_view,3> kValues1{"2","4","6"}; //원문 Bridge Slots의 모든 선택 문자열이다.
constexpr std::array<std::string_view,3> kValues2{"Slow","Medium","Fast"}; //원문 Unit Rate의 모든 선택 문자열이다.
constexpr std::array<std::string_view,2> kValues3{"Individual","Shared"}; //원문 Knowledge의 모든 선택 문자열이다.
constexpr std::array<std::string_view,4> kValues4{"Short","Normal","Long","Very Long"}; //원문 Generator Range의 모든 선택 문자열이다.
constexpr std::array<std::string_view,2> kValues5{"Archipelago","Single Island"}; //원문 Map Mode의 모든 선택 문자열이다.
constexpr std::array<std::string_view,4> kValues6{"Small","Medium","Large","Extra Large"}; //원문 Map Size의 모든 선택 문자열이다.
constexpr std::array<std::string_view,4> kValues7{"Low","Medium","High","Extreme"}; //원문 Map Density의 모든 선택 문자열이다.
constexpr std::array<std::string_view,2> kValues8{"No","Yes"}; //원문 Spells의 모든 선택 문자열이다.
constexpr std::array<std::string_view,4> kValues9{"Off","Low","Medium","High"}; //원문 Island Dynamics의 모든 선택 문자열이다.
constexpr std::array<std::string_view,2> kValues10{"Off","On"}; //원문 Living World의 모든 선택 문자열이다.
constexpr std::array<std::string_view,2> kValues11{"No","Yes"}; //원문 Alliances의 모든 선택 문자열이다.
constexpr std::array<std::string_view,2> kValues12{"No","Yes"}; //원문 Alliances Breakable 의 모든 선택 문자열이다.
constexpr std::array<std::string_view,2> kValues13{"No","Yes"}; //원문 Allow Sending Money의 모든 선택 문자열이다.
constexpr std::array<std::string_view,3> kValues14{"5000","6500","7000"}; //원문 Starting Cash의 모든 선택 문자열이다.
constexpr std::array<std::string_view,6> kValues15{"0%","25%","50%","75%","100%","150%"}; //원문 Kill Reward의 모든 선택 문자열이다.
constexpr std::array<std::string_view,4> kValues16{"1000","2000","3000","5000"}; //원문 Money per Geyser의 모든 선택 문자열이다.
constexpr std::array<std::string_view,4> kValues17{"Low","Medium","High","Extreme"}; //원문 Geyser Amount의 모든 선택 문자열이다.
constexpr std::array<std::string_view,3> kValues18{"None","Random","Even"}; //원문 Geysers Placement의 모든 선택 문자열이다.
constexpr std::array<std::string_view,5> kValues19{"None","30 Sec","1 Min","2 Min","5 Min"}; //원문 Geysers Respawns의 모든 선택 문자열이다.
constexpr std::array<std::string_view,6> kValues20{"None","30 Sec","1 Min","2 Min","5 Min","10 Min"}; //원문 Resource Injections의 모든 선택 문자열이다.
constexpr std::array<std::string_view,4> kValues21{"500 SP","1000 SP","2500 SP","5000 SP"}; //원문 Injection Value의 모든 선택 문자열이다.
const std::array<BattleOptionTextDefinition,22> kDefinitions{{ //원문 순서와 각 선택 번역 플래그다.
    {"Game Type",kValues0,false},
    {"Bridge Slots",kValues1,false},
    {"Unit Rate",kValues2,true},
    {"Knowledge",kValues3,true},
    {"Generator Range",kValues4,true},
    {"Map Mode",kValues5,false},
    {"Map Size",kValues6,false},
    {"Map Density",kValues7,false},
    {"Spells",kValues8,false},
    {"Island Dynamics",kValues9,false},
    {"Living World",kValues10,false},
    {"Alliances",kValues11,false},
    {"Alliances Breakable ",kValues12,true},
    {"Allow Sending Money",kValues13,false},
    {"Starting Cash",kValues14,false},
    {"Kill Reward",kValues15,true},
    {"Money per Geyser",kValues16,false},
    {"Geyser Amount",kValues17,false},
    {"Geysers Placement",kValues18,false},
    {"Geysers Respawns",kValues19,false},
    {"Resource Injections",kValues20,false},
    {"Injection Value",kValues21,false},
}};
}
// 원문 필드 접근 위치에서 실제 정적 표의 범위만 확인한다.
const BattleOptionTextDefinition& BattleOptionText(std::uint32_t index) { return kDefinitions.at(index); }
// 현재옵션을읽지않고원문선택포인터/번역플래그를같은정적표에서조회한다.
std::string BattleOptionChoiceText(std::uint32_t index,std::uint32_t selected,const std::function<std::string(std::string_view)>& translate) {
    const auto& definition=BattleOptionText(index);if (selected>=definition.values.size()) throw std::out_of_range("BattleOptions 실제 선택 포인터 표 범위");const auto value=definition.values[selected];if (!definition.translateValue) return std::string(value);if (!translate) throw std::logic_error("BattleOptions 현재 번역기 미연결");return translate(value);
}
namespace { //보호10.78의정적선택문자열주소는호스트메모리로역참조하지않는다.
constexpr std::array<std::uint32_t,76> kChoiceSourceAddresses{0x502118U,0x502104U,0x5020f0U,0x5020dcU,0x5020c8U,0x501614U,0x5015f4U,0x501610U,0x502278U,0x502270U,0x502268U,0x50225cU,0x502254U,0x50224cU,0x502244U,0x50223cU,0x502230U,0x5021dcU,0x5021ccU,0x5021b0U,0x502270U,0x5021a8U,0x50219cU,0x5021c8U,0x502270U,0x5021c0U,0x5021b8U,0x50220cU,0x502208U,0x502140U,0x5021c8U,0x502270U,0x5021c0U,0x502140U,0x50213cU,0x50220cU,0x502208U,0x50220cU,0x502208U,0x50220cU,0x502208U,0x5021e8U,0x502134U,0x50212cU,0x50222cU,0x502228U,0x502224U,0x502220U,0x502218U,0x502210U,0x502200U,0x5021f8U,0x5021f0U,0x5021e8U,0x5021c8U,0x502270U,0x5021c0U,0x5021b8U,0x500fecU,0x502194U,0x50218cU,0x500fecU,0x502184U,0x50217cU,0x502174U,0x50216cU,0x500fecU,0x502184U,0x50217cU,0x502174U,0x50216cU,0x502164U,0x50215cU,0x502154U,0x50214cU,0x502144U}; //원문22레코드의모든76선택포인터다.
constexpr std::array<std::size_t,23> kChoiceSourceOffsets{0,5,8,11,13,17,19,23,27,29,33,35,37,39,41,44,50,54,58,61,66,72,76}; //각레코드의시작과최종76경계다.
}
//현재표의정상선택범위를먼저검증하고그선택의원문주소값을반환한다.
std::uint32_t BattleOptionChoiceAddress(std::uint32_t index,std::uint32_t selected) { if (selected>=BattleOptionText(index).values.size()) throw std::out_of_range("BattleOptions 실제 선택 주소 범위");return kChoiceSourceAddresses[kChoiceSourceOffsets[index]+selected]; }
// BYTE 읽기와 포인터 조회 뒤에만 조건부 번역을 실행한다.
std::string BattleOptionValueText(std::span<const std::uint8_t> options,std::uint32_t index,const std::function<std::string(std::string_view)>& translate) {
    if (index>=options.size()) throw std::out_of_range("BattleOptions 현재 BYTE 물리 범위");const auto selected=options[index];const auto& definition=BattleOptionText(index);if (selected>=definition.values.size()) throw std::out_of_range("BattleOptions 실제 선택 포인터 표 범위");const auto value=definition.values[selected];if (!definition.translateValue) return std::string(value);if (!translate) throw std::logic_error("BattleOptions 현재 번역기 미연결");return translate(value);
}
// 이름은 선택 문자열의 번역 플래그와 독립적으로 현재 번역기를 요구한다.
std::string BattleOptionNameText(std::uint32_t index,const std::function<std::string(std::string_view)>& translate) { const auto name=BattleOptionText(index).name;if (!translate) throw std::logic_error("BattleOptions 현재 이름 번역기 미연결");return translate(name); }
}
