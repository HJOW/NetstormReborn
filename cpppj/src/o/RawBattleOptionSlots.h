// 실제87BYTE BattleOptions의8개 슬롯 색/랭크/특수 상태를 원문 폭으로 읽고 쓴다.
#pragma once
#include <cstdint>
#include <span>

namespace netstorm::o { //현재 CS 전투 옵션을 빌리며 별도 옵션 사본을 만들지 않는다.
std::uint32_t IncreaseBattleSlotColor(std::span<std::uint8_t> options,std::int32_t slot); //0041c4f0: BYTE 증가를 먼저 저장하고7보다 클 때만0으로 쓴다.
std::uint32_t DecreaseBattleSlotColor(std::span<std::uint8_t> options,std::int32_t slot); //0041c520: BYTE 감소를 먼저 저장하고ff일 때만7로 쓴다.
std::uint32_t IncreaseBattleSlotRank(std::span<std::uint8_t> options,std::int32_t slot); //0041c550: zero확장한 값+1을23과 비교한 뒤 최종BYTE만 쓴다.
std::uint32_t DecreaseBattleSlotRank(std::span<std::uint8_t> options,std::int32_t slot); //0041c580: zero확장한 값-1이음수일 때만23으로 바꿔 쓴다.
std::int32_t BattleSlotRank(std::span<const std::uint8_t> options,std::int32_t slot); //0041c5b0: 유효slot만+4f BYTE를 실제24DWORD 표로 변환한다.
std::uint32_t ToggleBattleSlotSpecial(std::span<std::uint8_t> options,std::int32_t slot); //0041c6a0: +3f BYTE가정확히1이면0,그외면1이다.각writer는무효slot에서읽기없이0을반환한다.
}
