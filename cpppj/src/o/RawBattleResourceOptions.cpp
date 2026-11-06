// 원본 표의 signed DWORD 값과 호스트의 손상 인덱스 경계를 분리한다.
#include "o/RawBattleResourceOptions.h"
#include <array>
#include <stdexcept>
namespace netstorm::o { // 기본값 사본을 만들지 않고 실제 현재 옵션을 조회한다.
namespace { // 각 표의 선택 범위를 실제 읽기 전에 확인한다.
template<std::size_t N> std::int32_t Read(std::span<const std::uint8_t> options,std::size_t offset,const std::array<std::int32_t,N>& values) { if (options.size()!=87 || options[offset]>=N) throw std::out_of_range("전투 자원 옵션의 실제 표 범위");return values[options[offset]]; } // Native 범위 밖 스택 읽기는 호스트 오류로 구분한다.
}
std::int32_t BattleIncomePeriod(std::span<const std::uint8_t> options) { constexpr std::array<std::int32_t,6> periods{0,30,60,120,300,600};return Read(options,20,periods); } // 원문 여섯 SP 지급 간격이다.
std::int32_t BattleIncomeAmount(std::span<const std::uint8_t> options) { constexpr std::array<std::int32_t,4> amounts{500,1000,2500,5000};return Read(options,21,amounts); } // 현재 지급액은 패킷 종류2 검증에서도 같은 표를 쓴다.
std::int32_t BattleGeyserPeriod(std::span<const std::uint8_t> options) { constexpr std::array<std::int32_t,5> periods{0,30,60,120,300};return Read(options,19,periods); } // 일반 간헐천의 원문 다섯 간격이다.
}
