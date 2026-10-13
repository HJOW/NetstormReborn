// 원본burning의유무와절반HP를현재raw필드에서다시읽는다.
#include "o/RawDamageableBurning.h"
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 원본비정렬 little endian 필드를폭만큼읽는다.
std::uint32_t ReadBurn(std::span<const std::uint8_t> raw,std::size_t offset,std::size_t width=4) {
    std::uint32_t value=0;
    // 현재필드의하위바이트만조합한다.
    for (std::size_t i=0;i<width;++i) value|=static_cast<std::uint32_t>(raw[offset+i])<<(8*i);
    return value;
}
}
// 다른풀의최대HP조회가같은SID를읽지못하도록막는다.
RawDamageableBurning::RawDamageableBurning(SidPool& pool,std::span<const RiftTypeRecord> types,const SquidReward& hp,
    DamageableBurningState& state,DamageableBurningHooks hooks):pool_(pool),types_(types),hp_(hp),state_(state),hooks_(std::move(hooks)) {
    if (&hp.Pool()!=&pool || types.size()!=(pool.Edition()==OriginalEdition::Patch1078 ? 188U : 171U)) throw std::invalid_argument("burning HP/타입/풀 연결 오류");
}
// zero damageEffect는조회/생성callback을요구하지않는실제원본반환이다.
Sid RawDamageableBurning::Refresh(Sid victim) const {
    if (!victim.value || victim.value>=pool_.Capacity()) throw std::out_of_range("burning 피해 SID 오류");const auto raw=pool_.Slot(victim);if (raw[11]&1) return {};
    if (raw[10]>=types_.size()) throw std::out_of_range("burning 피해 타입 오류");if (!types_[raw[10]].damageEffect) return {};
    if (!hooks_.find) throw std::logic_error("burning 실제현재프로세스조회 누락");const bool patch=pool_.Edition()==OriginalEdition::Patch1078;
    const auto current=patch ? std::bit_cast<std::int32_t>(ReadBurn(raw,26)) : std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(ReadBurn(raw,26,2)));const auto maximum=hp_.TypeHitPoints(raw[10]);const auto doubled=std::bit_cast<std::int32_t>(std::bit_cast<std::uint32_t>(maximum)*2U);const auto half=doubled/4;
    auto found=hooks_.find(victim,state_.burningType,0);
    if (patch && state_.checking && hooks_.find(victim,state_.burningType,1).value) throw std::logic_error("burning 두번째프로세스 원본 assert");
    if (current<half) {
        if (!found.value) {
            if (!hooks_.create || (raw[10]==state_.bulfType && !hooks_.bulfSound)) throw std::logic_error("burning 실제생성/소리몸체 누락");
            if (raw[10]==state_.bulfType) hooks_.bulfSound(victim,std::bit_cast<float>(ReadBurn(raw,14)),std::bit_cast<float>(ReadBurn(raw,18)));
            found=hooks_.create(victim);
        }
    } else if (found.value) {
        if (!hooks_.destroy) throw std::logic_error("burning 실제삭제몸체 누락");hooks_.destroy(found,0);return {};
    }
    return found;
}
// 실제피격/burning 연결기의동일풀검사다.
const SidPool& RawDamageableBurning::Pool() const { return pool_; }
}
