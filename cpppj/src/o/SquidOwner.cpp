// Squid 소유자 지정. 세 실제 PE의 제한 x86 기대값(owner-x86.tsv)으로 검사한다.
#include "o/SquidOwner.h"
#include "o/RawCommonOwner.h"
#include <utility>
#include <stdexcept>

namespace netstorm::o {
// 보호막 등 생성 효과의 풀 연결을 읽기 전용으로 확인한다.
const SidPool& SquidOwner::Pool() const { return pool_; }
namespace {
// 00490050 ↔ CD 004071f0: 가득 찬 목록에는 넣지 않고 기존 항목과 남은 메모리를 그대로 둔다.
void Append(SquidPostPopList& list, Sid sid) {
    list.Append(sid.value);
}
// 004900a0→0040ea00 ↔ CD 00407250: 같은 번호를 모두 빼고 나머지를 앞으로 당긴다. 개수 밖의 값은 건드리지 않는다.
void Remove(SquidPostPopList& list, Sid sid) {
    list.RemoveAll(sid.value);
}
}
SquidOwner::SquidOwner(SidPool& pool, std::span<const RiftTypeRecord> types, SquidPostPopState& bookkeeping, const SquidOwnerMode& mode)
    : pool_(pool), types_(types.begin(), types.end()), bookkeeping_(bookkeeping), mode_(mode) {
    if (types.size() != (pool.Edition() == OriginalEdition::Patch1078 ? 188U : 171U)) throw std::invalid_argument("소유자 지정 타입 판본/크기 오류");
}
void SquidOwner::BindDiagnostics(SquidOwnerDiagnostics diagnostics) { if (!diagnostics.types.empty() && diagnostics.types.size()!=types_.size()) throw std::invalid_argument("공통 Owner 현재 TYPE 판본/길이 오류");diagnostics_=std::move(diagnostics); } // 실제 World 표는 복사하지 않는다.
std::uint32_t SquidOwner::TypeGenus(std::uint8_t type) const { const auto current=diagnostics_.types.empty() ? std::span<const RiftTypeRecord>(types_) : diagnostics_.types;if (type>=current.size()) throw std::out_of_range("공통 Owner 현재 TYPE 물리 범위");return current[type].flags2; } // 모드가 선택하지 않은 TYPE는 읽지 않는다.
void SquidOwner::Set(Sid sid,std::uint32_t player) {
    CommonOwnerHooks hooks;hooks.fort=[this] { return mode_.fort; };hooks.battle=[this] { return mode_.battle; };hooks.challenge=[this] { return mode_.challenge; };hooks.genus=[this](std::uint8_t type) { return TypeGenus(type); };hooks.assertion=diagnostics_.assertion;
    hooks.remove=[this](std::uint8_t owner,Sid value) { if (!owner) return;if (owner>=bookkeeping_.ownerFactories.size()) throw std::out_of_range("공통 Owner 제거의 현재 Player 물리 범위");Remove(bookkeeping_.ownerFactories[owner],value); };
    hooks.append=[this](std::uint8_t owner,Sid value) { if (!owner) return;if (owner>=bookkeeping_.ownerFactories.size()) throw std::out_of_range("공통 Owner 추가의 현재 Player 물리 범위");Append(bookkeeping_.ownerFactories[owner],value); };
    SetCommonSquidOwner(pool_,sid,player,hooks);
}

}
