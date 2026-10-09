// Squid 소유자 지정. 세 실제 PE의 제한 x86 기대값(owner-x86.tsv)으로 검사한다.
#include "o/SquidOwner.h"
#include <stdexcept>

namespace netstorm::o {
// 보호막 등 생성 효과의 풀 연결을 읽기 전용으로 확인한다.
const SidPool& SquidOwner::Pool() const { return pool_; }
namespace {
// 공통 raw 필드: 타입(+10), 상태(+11). 상태의 void 비트와 extra의 buried 비트가 목록 추가를 막는다.
constexpr std::size_t kType = 10, kState = 11;
constexpr std::uint8_t kVoid = 4, kBuried = 8;
// 작업장 목록 대상 genus: vortex(0x200)·factory(0x4000).
constexpr std::uint32_t kFactoryMask = 0x4200;
// 다른 모드에서 유효한 소유자 번호의 끝(원본 CL_NUM_CHAL_PLAYERS = 0x28).
constexpr std::uint32_t kChallengePlayers = 0x28;
// 00490050 ↔ CD 004071f0: 가득 찬 목록에는 넣지 않고 기존 항목과 남은 메모리를 그대로 둔다.
void Append(SquidPostPopList& list, Sid sid) {
    if (list.count < list.entries.size()) list.entries[list.count++] = sid.value;
}
// 004900a0→0040ea00 ↔ CD 00407250: 같은 번호를 모두 빼고 나머지를 앞으로 당긴다. 개수 밖의 값은 건드리지 않는다.
void Remove(SquidPostPopList& list, Sid sid) {
    static_cast<void>(list.Items());
    std::uint32_t removed = 0;
    // 남는 항목의 상대 순서를 유지한다.
    for (std::uint32_t i = 0; i < list.count; ++i) {
        const auto value = list.entries[i];
        if (value == sid.value) ++removed;
        else list.entries[i - removed] = value;
    }
    list.count -= removed;
}
}
SquidOwner::SquidOwner(SidPool& pool, std::span<const RiftTypeRecord> types, SquidPostPopState& bookkeeping, const SquidOwnerMode& mode)
    : pool_(pool), types_(types.begin(), types.end()), bookkeeping_(bookkeeping), mode_(mode) {
    if (types.size() != (pool.Edition() == OriginalEdition::Patch1078 ? 188U : 171U)) throw std::invalid_argument("소유자 지정 타입 판본/크기 오류");
}
void SquidOwner::Set(Sid sid, std::uint32_t player) {
    const bool patch = pool_.Edition() == OriginalEdition::Patch1078;
    // 소유자·extra 필드 위치는 판본마다 다르다.
    const std::size_t ownerOffset = patch ? 0x22 : 0x20, extraOffset = patch ? 0x28 : 0x23;
    // 원본 범위 검사: 평소에는 0 또는 1~8, 다른 모드에서는 1~39. 장부의 목록은 1~8만 있으므로 그 밖은 지원하지 않는다.
    const bool valid = mode_.challenge ? (player > 0 && player < kChallengePlayers) : player < bookkeeping_.ownerFactories.size();
    if (!valid) throw std::invalid_argument("소유자 번호 범위 오류(원본 assert)");
    if (player >= bookkeeping_.ownerFactories.size()) throw std::invalid_argument("목록이 없는 소유자 번호는 지원하지 않는다");
    auto raw = pool_.AllocatedBytes(sid);
    const std::uint32_t type = raw[kType];
    if (type >= types_.size()) throw std::out_of_range("소유자 지정 타입 범위 오류");
    const bool listed = (mode_.fort || mode_.battle) && (types_[type].flags2 & kFactoryMask) != 0;
    // 이전 소유자의 목록에서 뺀다. 소유자 0은 목록이 없다.
    if (listed) {
        const std::uint8_t previous = raw[ownerOffset];
        if (previous) {
            if (previous >= bookkeeping_.ownerFactories.size()) throw std::out_of_range("이전 소유자 번호 범위 오류");
            Remove(bookkeeping_.ownerFactories[previous], sid);
        }
    }
    raw[ownerOffset] = static_cast<std::uint8_t>(player);
    // void이거나 buried이면 새 소유자의 목록에 넣지 않는다.
    if (listed && !(raw[kState] & kVoid) && !(raw[extraOffset] & kBuried) && player) Append(bookkeeping_.ownerFactories[player], sid);
}
}
