// 정수 감김과 x87의 정수 정밀도·float 저장 경계를 보존한다.
#include "o/RawNuggetReplenish.h"
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// DWORD 연산 뒤 signed 해석만 수행한다.
std::int32_t Signed(std::uint32_t value) { return std::bit_cast<std::int32_t>(value); }
// 0040d900의 정수<<4 후 float 저장을 수행한다.
float Cell(std::int32_t value) { return static_cast<float>(Signed(static_cast<std::uint32_t>(value)<<4)); }
}
// 모드 관문에서 사용하지 않는 큰 하위는 생성 시 요구하지 않는다.
RawNuggetReplenish::RawNuggetReplenish(const RawPlayerGeyserBounds& bounds,NuggetReplenishState state,NuggetReplenishHooks hooks)
    :bounds_(bounds),state_(state),hooks_(std::move(hooks)) {}
// 현재 boss는 진단 뒤 재독하고 확장 단위는 실제 패턴 계산 뒤에 읽는다.
void RawNuggetReplenish::Run() const {
    if (!state_.boss) {
        if (!hooks_.assertion) throw std::logic_error("너겟 보충의 권한 진단 미연결");
        hooks_.assertion(0x246);if (!state_.boss) return;
    }
    if (state_.single || state_.fort || state_.editor) return;
    const auto record=bounds_.RegionRecord(0);const auto area=bounds_.OriginPlacement(record); //현재 지역0의 첫 record를 값으로 보존한다.
    const auto scaled=static_cast<std::uint32_t>(state_.archipelagoSpan)<<4;const auto trim=Signed(scaled-16U); //두 signed 정수 모두 float로 먼저 축약하지 않는다.
    const FortRandomRectangle rectangle{static_cast<float>(static_cast<double>(Cell(area.left))-trim),static_cast<float>(static_cast<double>(Cell(area.top))-trim),
        static_cast<float>(static_cast<double>(Cell(area.right))+Signed(scaled)),static_cast<float>(static_cast<double>(Cell(area.bottom))+Signed(scaled))}; //원본의 각 fstp DWORD에서만 반올림한다.
    if (!hooks_.place) throw std::logic_error("비미션 너겟 보충의 실제 무작위 배치 미연결");
    hooks_.place(rectangle,1);
}
}
