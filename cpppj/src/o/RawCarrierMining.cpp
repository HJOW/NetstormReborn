// 원본은 지면을 미리 검사하지 않고 송신을 완료한 뒤 같은6바이트를 로컬 처리한다.
#include "o/RawCarrierMining.h"
#include <array>
#include <stdexcept>
#include <utility>
namespace netstorm::o {
namespace {
// 실제 선택된 경계의 누락을 성공이나 빈 처리로 바꾸지 않는다.
template<class F> const F& Need(const F& hook) { if (!hook) throw std::logic_error("채취 통지 실제 하위 미연결");return hook; }
}
//10.72의 다른 상위 도착 몸체를 같은 것으로 가정하지 않는다.
RawCarrierMining::RawCarrierMining(const SidPool& pool,const FormPacketHeader& header,CarrierMiningHooks hooks):pool_(pool),header_(header),hooks_(std::move(hooks)) {
    if (pool.Edition()!=OriginalEdition::Patch1078) throw std::invalid_argument("채취 통지 판본 오류");
}
// 현재 헤더를 한 번 작성한 뒤 송신 효과가 헤더 전역을 바꾸어도 로컬 버퍼는 보존한다.
void RawCarrierMining::Notify(Sid geyser) const {
    std::array<std::uint8_t,6> packet{static_cast<std::uint8_t>(header_.protocol&0x7f),header_.message,static_cast<std::uint8_t>(header_.length),static_cast<std::uint8_t>(header_.length>>8),static_cast<std::uint8_t>(geyser.value),static_cast<std::uint8_t>(geyser.value>>8)};
    Need(hooks_.broadcast)(packet);Need(hooks_.receive)(packet);
}
// 같은 실제 월드의 풀이다.
const SidPool& RawCarrierMining::Pool() const { return pool_; }
}
