// 유효 입력의 BYTE 감김·중복/상한·이동과 음향 전후 부분 쓰기를 보존한다.
#include "o/RawBattleOptionMutation.h"
#include "o/RawBattleOptionText.h"
#include <cstring>
#include <stdexcept>

namespace netstorm::o { // 현재 UI/CS가 같은 옵션 물리를 사용한다.
namespace { // 원문에 없는 전체87BYTE 초기 검사를 추가하지 않는다.
std::uint8_t Byte(std::span<const std::uint8_t> options,std::size_t at) { if (at>=options.size()) throw std::out_of_range("BattleOptions 변경의 실제 BYTE 읽기 범위");return options[at]; } // 실제 선택한 필드 위치에서만 읽기를 검증한다.
void Store(std::span<std::uint8_t> options,std::size_t at,std::uint8_t value) { if (at>=options.size()) throw std::out_of_range("BattleOptions 변경의 실제 BYTE 쓰기 범위");options[at]=value; } // 현재 한 BYTE만 쓰며 이웃 슬롯을 초기화하지 않는다.
void Sound(const std::function<void()>& play) { if (!play) throw std::logic_error("BattleOptions 변경의 실제 guy.wav 장치 미연결");play(); } // 선택한 원문 재생 위치에서만 장치를 요구한다.
}
// 원문의 movzx 첫 BYTE이며 다른 현재 옵션/표를 읽지 않는다.
std::uint32_t BattleOptionGameType(std::span<const std::uint8_t> options) { return Byte(options,0); }
//1105에서 보호 PE와 확인한 동일 문자열 표가 포함 최댓값의 기준이다.
std::int32_t MaximumBattleOptionChoice(std::int32_t index) { return static_cast<std::int32_t>(BattleOptionText(static_cast<std::uint32_t>(index)).values.size())-1; }
// 255+1의0도 BYTE에서 먼저 감기며 소리가 복귀하기 전에는 실제 필드를 수정하지 않는다.
void CycleBattleOption(std::span<std::uint8_t> options,std::int32_t index,const std::function<void()>& guySound) {
    const auto at=static_cast<std::uint32_t>(index);auto next=static_cast<std::uint8_t>(Byte(options,at)+1U);if (static_cast<std::int32_t>(next)>MaximumBattleOptionChoice(index)) next=0;Sound(guySound);Store(options,at,next);
}
// 음수/큰 선택은 옵션 필드를 읽거나 쓰지 않으며 유효값도 BYTE 저장만 수행한다.
void SetBattleOptionChoice(std::span<std::uint8_t> options,std::int32_t index,std::int32_t value) { if (value<0) return;if (value<=MaximumBattleOptionChoice(index)) Store(options,static_cast<std::uint32_t>(index),static_cast<std::uint8_t>(value)); }
// 이미 같은 BYTE가 있으면 음향 없이1이다. 비교 전에 type을BYTE로 잘라 잘못 중복으로 판단하지 않는다.
std::uint32_t AddBattleExcludedType(std::span<std::uint8_t> options,std::uint32_t type,const std::function<void()>& guySound) {
    const auto initial=Byte(options,0x16);std::uint32_t index{};
    if (initial) do { //원문처럼 각 다음 반복의 현재count를 다시 읽고 최초 일치에서 즉시 반환한다.
        if (std::uint32_t(Byte(options,index+0x17U))==type) return 1;++index;
    } while (index<Byte(options,0x16));
    if (initial>=40) return 0;Store(options,index+0x17U,static_cast<std::uint8_t>(type));Store(options,0x16,static_cast<std::uint8_t>(Byte(options,0x16)+1U));Sound(guySound);return 1;
}
// 이동 길이는 첫count를 사용하고 음향 뒤 감소는 그때의 현재count를 다시 읽는다.
std::uint32_t RemoveBattleExcludedType(std::span<std::uint8_t> options,std::uint32_t type,const std::function<void()>& guySound) {
    const auto initial=Byte(options,0x16);std::uint32_t index{};
    if (initial) do { //같은 현재count의 반복 조건으로 최초 일치만 제거한다.
        if (std::uint32_t(Byte(options,index+0x17U))==type) {
            const auto count=std::uint32_t(initial)-index-1U,destination=index+0x17U,source=index+0x18U;
            if (destination>options.size() || count>options.size()-destination || source>options.size() || count>options.size()-source) throw std::out_of_range("BattleOptions 제외 목록의 실제 memmove 범위");std::memmove(options.data()+destination,options.data()+source,count);Sound(guySound);Store(options,0x16,static_cast<std::uint8_t>(std::uint32_t(Byte(options,0x16))-1U));return 1;
        }
        ++index;
    } while (index<Byte(options,0x16));
    return 0;
}
// getter만 최초count를 캡처한다. 추가/제거의 재조회 반복과 임의 통합하지 않는다.
std::uint32_t BattleTypeExcluded(std::span<const std::uint8_t> options,std::uint32_t type) {
    const auto count=Byte(options,0x16);for (std::uint32_t index=0;index<count;++index) { if (std::uint32_t(Byte(options,index+0x17U))==type) return 1; }return 0; //현재 목록의 각 BYTE를 원문 DWORD 인자와 차례로 비교한다.
}
}
