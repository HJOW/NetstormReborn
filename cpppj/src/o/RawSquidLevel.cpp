// 원본의 항상 참 hasLevel과 signed 진단 후 bit 저장 순서를 보존한다.
#include "o/RawSquidLevel.h"
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
// 판본의 전체 TYPE 표와 원본 진단 경계를 처음 쓰기 전에 확인한다.
RawSquidLevel::RawSquidLevel(SidPool& pool,std::span<const RiftTypeRecord> types,std::function<void(SquidLevelAssertion)> assertion)
    :pool_(pool),types_(types),assertion_(std::move(assertion)) {
    if (types.size()!=(pool.Edition()==OriginalEdition::Patch1078 ? 188U : 171U) || !assertion_)
        throw std::invalid_argument("Squid level 판본 TYPE/진단 누락");
}
// group10의 flags1|04000000==0이 양 판본 모두 불가능하므로 타입조회 후true다.
bool RawSquidLevel::Has(Sid sid) const {
    if (pool_.AllocatedBytes(sid)[10]>=types_.size()) throw std::out_of_range("Squid level 현재 TYPE 범위 오류");
    return true;
}
// base 가상80의 BYTE 상위비트만 읽는다.
std::uint32_t RawSquidLevel::Get(Sid sid) const { return Has(sid) ? pool_.Slot(sid)[11]>>5 : 0; }
// 진단 callback이 같은 슬롯의 하위비트를 바꾸어도 실제 저장 직전에 현재 값을 다시 읽는다.
void RawSquidLevel::Set(Sid sid,std::uint32_t value) const {
    (void)Has(sid);if (std::bit_cast<std::int32_t>(value)<0 || value>=8) assertion_(SquidLevelAssertion::OutOfRange);
    auto raw=pool_.AllocatedBytes(sid);raw[11]=static_cast<std::uint8_t>((raw[11]&31U)|((value&7U)<<5));
}
// 실제 연결의 동일 Pool을 확인한다.
const SidPool& RawSquidLevel::Pool() const { return pool_; }
}
