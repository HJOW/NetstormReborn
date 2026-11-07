// 유효 슬롯 관문 뒤에만 실제 BYTE를 접근하며 원문에 없는 전체 초기화/범위 보정을 하지 않는다.
#include "o/RawBattleOptionSlots.h"
#include <array>
#include <stdexcept>

namespace netstorm::o { //동일 전투의 옵션을 UI/송신 부모가 공유한다.
namespace { //원문 필드 위치와 읽기/쓰기 폭을 분리한다.
constexpr std::array<std::int32_t,24> kRanks{0,1,2,3,4,5,6,7,8,9,10,15,20,25,30,35,40,45,50,60,70,80,90,100}; //0041c5b0의실제지역24DWORD 표다.
bool Valid(std::int32_t slot) { return slot>=0 && slot<8; } //원문 signed0..7 관문이다.옵션span보다먼저판정한다.
std::size_t Position(std::span<const std::uint8_t> options,std::size_t field,std::int32_t slot) { const auto at=field+static_cast<std::uint32_t>(slot);if (at>=options.size()) throw std::out_of_range("BattleOptions 슬롯의 실제 BYTE 물리 범위");return at; } //유효slot caller의 실제 필드 접근 위치에서만 물리 길이를 확인한다.
}
// 255+1은 BYTE0으로 먼저 감긴 뒤 비교한다.
std::uint32_t IncreaseBattleSlotColor(std::span<std::uint8_t> options,std::int32_t slot) { if (!Valid(slot)) return 0;const auto at=Position(options,0x47,slot);const auto next=static_cast<std::uint8_t>(options[at]+1U);options[at]=next;if (next>7) options[at]=0;return 1; }
// 0-1의 BYTEff만7로 바꾸고 그외 비정상 입력값을 임의 clamp하지 않는다.
std::uint32_t DecreaseBattleSlotColor(std::span<std::uint8_t> options,std::int32_t slot) { if (!Valid(slot)) return 0;const auto at=Position(options,0x47,slot);const auto next=static_cast<std::uint8_t>(std::uint32_t(options[at])-1U);options[at]=next;if (next==0xffU) options[at]=7;return 1; }
// 랭크 증가는 BYTE 저장 전에 원문 DWORD의23초과를 판정한다.
std::uint32_t IncreaseBattleSlotRank(std::span<std::uint8_t> options,std::int32_t slot) { if (!Valid(slot)) return 0;const auto at=Position(options,0x4f,slot);auto next=std::uint32_t(options[at])+1U;if (next>23) next=0;options[at]=static_cast<std::uint8_t>(next);return 1; }
// 랭크 감소는0에서만23으로 순환하며 기타 큰 입력값의 감소를 유지한다.
std::uint32_t DecreaseBattleSlotRank(std::span<std::uint8_t> options,std::int32_t slot) { if (!Valid(slot)) return 0;const auto at=Position(options,0x4f,slot);auto next=std::int32_t(options[at])-1;if (next<0) next=23;options[at]=static_cast<std::uint8_t>(next);return 1; }
// 원문24DWORD 밖의 읽기는 메모리 오염 대신 호스트의 물리 오류로 드러낸다.
std::int32_t BattleSlotRank(std::span<const std::uint8_t> options,std::int32_t slot) { if (!Valid(slot)) return 0;return kRanks.at(options[Position(options,0x4f,slot)]); }
// 원문의 !=1은 단순 bool반전/XOR와 다르므로 실제 비교를 유지한다.
std::uint32_t ToggleBattleSlotSpecial(std::span<std::uint8_t> options,std::int32_t slot) { if (!Valid(slot)) return 0;const auto at=Position(options,0x3f,slot);options[at]=options[at]!=1U ? 1U:0U;return 1; }
}
