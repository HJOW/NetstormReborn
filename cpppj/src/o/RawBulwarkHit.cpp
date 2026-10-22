// 원본은공격자free/dead/extra/owner를검사하지않고현재TYPE의비행genus만읽는다.
#include "o/RawBulwarkHit.h"
#include <stdexcept>

namespace netstorm::o {
// 관문통과뒤일반피해의HP/방어/소유자통지/삭제/연소처리를그대로공유한다.
std::uint32_t RawBulwarkHit(const SidPool& pool,std::span<const RiftTypeRecord> types,Sid victim,
    std::int32_t amount,std::uint32_t owner,Sid attacker,
    const std::function<std::uint32_t(Sid,std::int32_t,std::uint32_t,Sid)>& general) {
    if (pool.Edition()!=OriginalEdition::Patch1078 || types.size()!=188) throw std::invalid_argument("불워크 피해 판본/현재TYPE 표 오류");
    if (attacker.value) {
        const auto type=pool.Slot(attacker)[10];if (type>=types.size()) throw std::out_of_range("불워크 현재공격자TYPE 오류");
        if (types[type].flags2&0x100000U) return 0; //비행체genus이며벌룬20000이나genus이름으로대체하지않는다.
    }
    if (!general) throw std::logic_error("불워크 실제 일반 피해 미연결");return general(victim,amount,owner,attacker);
}
}
