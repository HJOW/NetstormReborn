// 원본0041c800/1c840/1c870의 현재 일반 SP/간헐천 옵션 표다.
#pragma once
#include <cstdint>
#include <span>
namespace netstorm::o { // 호출 시점의 같은87BYTE에서 선택값을 읽는다.
std::int32_t BattleIncomePeriod(std::span<const std::uint8_t> options); //+14의 현재 선택0..5를0/30/60/120/300/600초로 읽는다.
std::int32_t BattleIncomeAmount(std::span<const std::uint8_t> options); //+15의 현재 선택0..3을500/1000/2500/5000으로 읽는다.
std::int32_t BattleGeyserPeriod(std::span<const std::uint8_t> options); //+13의 현재 선택0..4를0/30/60/120/300초로 읽는다.
}
